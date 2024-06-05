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

#include "ecmascript/base/asan_interface.h"
#include "ecmascript/free_object.h"
#include "ecmascript/mem/free_object_list.h"

namespace panda::ecmascript {
#ifdef ENABLE_JITFORT
template <typename T>
void FreeObjectSet<T>::Free(uintptr_t begin, size_t size)
#else
void FreeObjectSet::Free(uintptr_t begin, size_t size)
#endif
{
#ifdef ENABLE_JITFORT
    auto freeObject = T::Cast(begin);
#else
    auto freeObject = FreeObject::Cast(begin);
#endif
    ASSERT(freeObject->IsFreeObject());
    freeObject->SetNext(freeObject_);
    freeObject_ = freeObject;
    available_ += size;
}

#ifdef ENABLE_JITFORT
template void FreeObjectSet<FreeObject>::Free(uintptr_t, size_t);
template <>
void FreeObjectSet<MemDesc>::Free(uintptr_t begin, size_t size)
{
    ASSERT(begin >= JitFort::GetInstance()->JitFortBegin() &&
        size <= JitFort::GetInstance()->JitFortSize());

    auto freeObject = JitFort::GetInstance()->GetMemDescFromPool();
    freeObject->SetMem(begin);
    freeObject->SetSize(size);
    freeObject->SetNext(freeObject_);
    freeObject_ = freeObject;
    available_ += size;
}
#endif

#ifdef ENABLE_JITFORT
template <typename T>
void FreeObjectSet<T>::Rebuild()
#else
void FreeObjectSet::Rebuild()
#endif
{
#ifdef ENABLE_JITFORT
    freeObject_ = T::Cast(INVALID_OBJPTR);
#else
    freeObject_ = INVALID_OBJECT;
#endif
    available_ = 0;
    isAdded_ = false;
    next_ = nullptr;
    prev_ = nullptr;
}
#ifdef ENABLE_JITFORT
template void FreeObjectSet<FreeObject>::Rebuild();
template <>
void FreeObjectSet<MemDesc>::Rebuild()
{
    MemDesc *current = freeObject_;
    while (!MemDescPool::IsEmpty(current)) {
        // put desc back to free pool
        auto next = current->GetNext();
        JitFort::GetInstance()->ReturnMemDescToPool(current);
        current = next;
    }
    freeObject_ = MemDesc::Cast(INVALID_OBJPTR);
    available_ = 0;
    isAdded_ = false;
    next_ = nullptr;
    prev_ = nullptr;
}
#endif

#ifdef ENABLE_JITFORT
template <typename T>
T *FreeObjectSet<T>::ObtainSmallFreeObject(size_t size)
#else
FreeObject *FreeObjectSet::ObtainSmallFreeObject(size_t size)
#endif
{
#ifdef ENABLE_JITFORT
    T *curFreeObject = T::Cast(INVALID_OBJPTR);
    if (freeObject_ != T::Cast(INVALID_OBJPTR)) {
#else
    FreeObject *curFreeObject = INVALID_OBJECT;
    if (freeObject_ != INVALID_OBJECT) {
#endif
        freeObject_->AsanUnPoisonFreeObject();
        if (freeObject_->Available() >= size) {
            curFreeObject = freeObject_;
            freeObject_ = freeObject_->GetNext();
#ifdef ENABLE_JITFORT
            curFreeObject->SetNext(T::Cast(INVALID_OBJPTR));
#else
            curFreeObject->SetNext(INVALID_OBJECT);
#endif
            available_ -= curFreeObject->Available();
            // It need to mark unpoison when object being allocated in freelist.
            ASAN_UNPOISON_MEMORY_REGION(curFreeObject, curFreeObject->Available());
        } else {
            freeObject_->AsanPoisonFreeObject();
        }
    }
    return curFreeObject;
}
#ifdef ENABLE_JITFORT
template FreeObject *FreeObjectSet<FreeObject>::ObtainSmallFreeObject(size_t);
template MemDesc *FreeObjectSet<MemDesc>::ObtainSmallFreeObject(size_t);

template <typename T>
T *FreeObjectSet<T>::ObtainLargeFreeObject(size_t size)
#else
FreeObject *FreeObjectSet::ObtainLargeFreeObject(size_t size)
#endif
{
#ifdef ENABLE_JITFORT
    T *prevFreeObject = freeObject_;
    T *curFreeObject = freeObject_;
    while (curFreeObject != T::Cast(INVALID_OBJPTR)) {
#else
    FreeObject *prevFreeObject = freeObject_;
    FreeObject *curFreeObject = freeObject_;
    while (curFreeObject != INVALID_OBJECT) {
#endif
        curFreeObject->AsanUnPoisonFreeObject();
        if (curFreeObject->Available() >= size) {
            if (curFreeObject == freeObject_) {
                freeObject_ = curFreeObject->GetNext();
            } else {
                prevFreeObject->SetNext(curFreeObject->GetNext());
                prevFreeObject->AsanPoisonFreeObject();
            }
#ifdef ENABLE_JITFORT
            curFreeObject->SetNext(T::Cast(INVALID_OBJPTR));
#else
            curFreeObject->SetNext(INVALID_OBJECT);
#endif
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
#ifdef ENABLE_JITFORT
    return T::Cast(INVALID_OBJPTR);
#else
    return INVALID_OBJECT;
#endif
}
#ifdef ENABLE_JITFORT
template FreeObject *FreeObjectSet<FreeObject>::ObtainLargeFreeObject(size_t);
template MemDesc *FreeObjectSet<MemDesc>::ObtainLargeFreeObject(size_t);

template <typename T>
T *FreeObjectSet<T>::LookupSmallFreeObject(size_t size)
#else
FreeObject *FreeObjectSet::LookupSmallFreeObject(size_t size)
#endif
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
#ifdef ENABLE_JITFORT
template FreeObject *FreeObjectSet<FreeObject>::LookupSmallFreeObject(size_t);

template <typename T>
T *FreeObjectSet<T>::LookupLargeFreeObject(size_t size)
#else
FreeObject *FreeObjectSet::LookupLargeFreeObject(size_t size)
#endif
{
    if (available_ < size) {
        return INVALID_OBJECT;
    }
#ifdef ENABLE_JITFORT
    T *curFreeObject = freeObject_;
#else
    FreeObject *curFreeObject = freeObject_;
#endif
    while (curFreeObject != INVALID_OBJECT) {
        curFreeObject->AsanUnPoisonFreeObject();
        if (curFreeObject->Available() >= size) {
            curFreeObject->AsanPoisonFreeObject();
            return curFreeObject;
        }
#ifdef ENABLE_JITFORT
        T *preFreeObject = curFreeObject;
#else
        FreeObject *preFreeObject = curFreeObject;
#endif
        curFreeObject = curFreeObject->GetNext();
        preFreeObject->AsanPoisonFreeObject();
    }
    return INVALID_OBJECT;
}
#ifdef ENABLE_JITFORT
template FreeObject *FreeObjectSet<FreeObject>::LookupLargeFreeObject(size_t);
#endif
}  // namespace panda::ecmascript
