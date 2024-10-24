/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/mem/free_object_set.h"

#include "ecmascript/free_object.h"

namespace panda::ecmascript {
template <typename T>
void FreeObjectSet<T>::Free(uintptr_t begin, size_t size)
{
    auto freeObject = T::Cast(begin);
    ASSERT(freeObject->IsFreeObject());
    freeObject->SetNext(freeObject_);
    freeObject_ = freeObject;
    available_ += size;
    maxAvailableFreeSize_ = std::max(maxAvailableFreeSize_, size);
}

template void FreeObjectSet<FreeObject>::Free(uintptr_t, size_t);
template <>
void FreeObjectSet<MemDesc>::Free(uintptr_t begin, size_t size)
{
    ASSERT(begin >= memDescPool_->JitFortBegin() &&
        size <= memDescPool_->JitFortSize());

    auto freeObject = memDescPool_->GetDescFromPool();
    ASSERT(freeObject != nullptr);
    freeObject->SetMem(begin);
    freeObject->SetSize(size);
    freeObject->SetNext(freeObject_);
    freeObject_ = freeObject;
    available_ += size;
    maxAvailableFreeSize_ = std::max(maxAvailableFreeSize_, size);
}

template <typename T>
void FreeObjectSet<T>::Rebuild()
{
    freeObject_ = T::Cast(INVALID_OBJPTR);
    available_ = 0;
    maxAvailableFreeSize_ = 0;
    isAdded_ = 0;
    next_ = nullptr;
    prev_ = nullptr;
}

template void FreeObjectSet<FreeObject>::Rebuild();
template <>
void FreeObjectSet<MemDesc>::Rebuild()
{
    MemDesc *current = freeObject_;
    while (!MemDescPool::IsEmpty(current)) {
        // put desc back to free pool
        auto next = current->GetNext();
        memDescPool_->ReturnDescToPool(current);
        current = next;
    }
    freeObject_ = MemDesc::Cast(INVALID_OBJPTR);
    available_ = 0;
    maxAvailableFreeSize_ = 0;
    isAdded_ = 0;
    next_ = nullptr;
    prev_ = nullptr;
}

template <typename T>
T *FreeObjectSet<T>::ObtainSmallFreeObject(size_t size)
{
    T *curFreeObject = T::Cast(INVALID_OBJPTR);
    if (freeObject_ != T::Cast(INVALID_OBJPTR)) {
        freeObject_->AsanUnPoisonFreeObject();
        if (freeObject_->Available() >= size) {
            curFreeObject = freeObject_;
            freeObject_ = freeObject_->GetNext();
            curFreeObject->SetNext(T::Cast(INVALID_OBJPTR));
            available_ -= curFreeObject->Available();
            // It need to mark unpoison when object being allocated in freelist.
            ASAN_UNPOISON_MEMORY_REGION(curFreeObject, curFreeObject->Available());
        } else {
            freeObject_->AsanPoisonFreeObject();
        }
    }
    return curFreeObject;
}

template FreeObject *FreeObjectSet<FreeObject>::ObtainSmallFreeObject(size_t);
template MemDesc *FreeObjectSet<MemDesc>::ObtainSmallFreeObject(size_t);

template <typename T>
T *FreeObjectSet<T>::ObtainLargeFreeObject(size_t size)
{
    T *prevFreeObject = freeObject_;
    T *curFreeObject = freeObject_;
    while (curFreeObject != T::Cast(INVALID_OBJPTR)) {
        curFreeObject->AsanUnPoisonFreeObject();
        if (curFreeObject->Available() >= size) {
            if (curFreeObject == freeObject_) {
                freeObject_ = curFreeObject->GetNext();
            } else {
                prevFreeObject->SetNext(curFreeObject->GetNext());
                prevFreeObject->AsanPoisonFreeObject();
            }
            curFreeObject->SetNext(T::Cast(INVALID_OBJPTR));
            available_ -= curFreeObject->Available();
            ASAN_UNPOISON_MEMORY_REGION(curFreeObject, curFreeObject->Available());
            return curFreeObject;
        }
        if (prevFreeObject != curFreeObject) {
            prevFreeObject->AsanPoisonFreeObject();
        }
        prevFreeObject = curFreeObject;
        curFreeObject = curFreeObject->GetNext();
    }
    // When looking up suitable freeobject fails, available free size should update to a value less than size.
    maxAvailableFreeSize_ = std::min(maxAvailableFreeSize_, size - sizeof(JSTaggedType));
    return T::Cast(INVALID_OBJPTR);
}

template FreeObject *FreeObjectSet<FreeObject>::ObtainLargeFreeObject(size_t);
template MemDesc *FreeObjectSet<MemDesc>::ObtainLargeFreeObject(size_t);

template <typename T>
T *FreeObjectSet<T>::LookupSmallFreeObject(size_t size)
{
    if (freeObject_ != INVALID_OBJECT) {
        freeObject_->AsanUnPoisonFreeObject();
        if (freeObject_->Available() >= size) {
            freeObject_->AsanPoisonFreeObject();
            return freeObject_;
        }
        freeObject_->AsanPoisonFreeObject();
    }
    return INVALID_OBJECT;
}

template FreeObject *FreeObjectSet<FreeObject>::LookupSmallFreeObject(size_t);

template <typename T>
T *FreeObjectSet<T>::LookupLargeFreeObject(size_t size)
{
    if (available_ < size) {
        return INVALID_OBJECT;
    }
    T *curFreeObject = freeObject_;
    while (curFreeObject != INVALID_OBJECT) {
        curFreeObject->AsanUnPoisonFreeObject();
        if (curFreeObject->Available() >= size) {
            curFreeObject->AsanPoisonFreeObject();
            return curFreeObject;
        }
        T *preFreeObject = curFreeObject;
        curFreeObject = curFreeObject->GetNext();
        preFreeObject->AsanPoisonFreeObject();
    }
    return INVALID_OBJECT;
}

template FreeObject *FreeObjectSet<FreeObject>::LookupLargeFreeObject(size_t);

}  // namespace panda::ecmascript
