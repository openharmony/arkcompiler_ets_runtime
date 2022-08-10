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
void FreeObjectSet::Free(uintptr_t begin, size_t size)
{
    auto freeObject = FreeObject::Cast(begin);
    ASSERT(freeObject->IsFreeObject());
    freeObject->SetNext(freeObject_);
    freeObject_ = freeObject;
    available_ += size;
}

void FreeObjectSet::Rebuild()
{
    freeObject_ = INVALID_OBJECT;
    available_ = 0;
    isAdded_ = false;
    next_ = nullptr;
    prev_ = nullptr;
}

FreeObject *FreeObjectSet::ObtainSmallFreeObject(size_t size)
{
    FreeObject *curFreeObject = INVALID_OBJECT;
    if (freeObject_ != INVALID_OBJECT) {
        freeObject_->AsanUnPoisonFreeObject();
        if (freeObject_->Available() >= size) {
            curFreeObject = freeObject_;
            freeObject_ = freeObject_->GetNext();
            curFreeObject->SetNext(INVALID_OBJECT);
            available_ -= curFreeObject->Available();
            // It need to mark unpoison when object being allocated in freelist.
            ASAN_UNPOISON_MEMORY_REGION(curFreeObject, curFreeObject->Available());
        } else {
            freeObject_->AsanPoisonFreeObject();
        }
    }
    return curFreeObject;
}

FreeObject *FreeObjectSet::ObtainLargeFreeObject(size_t size)
{
    FreeObject *prevFreeObject = freeObject_;
    FreeObject *curFreeObject = freeObject_;
    while (curFreeObject != INVALID_OBJECT) {
        curFreeObject->AsanUnPoisonFreeObject();
        if (curFreeObject->Available() >= size) {
            if (curFreeObject == freeObject_) {
                freeObject_ = curFreeObject->GetNext();
            } else {
                prevFreeObject->SetNext(curFreeObject->GetNext());
                prevFreeObject->AsanPoisonFreeObject();
            }
            curFreeObject->SetNext(INVALID_OBJECT);
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
    return INVALID_OBJECT;
}

FreeObject *FreeObjectSet::LookupSmallFreeObject(size_t size)
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

FreeObject *FreeObjectSet::LookupLargeFreeObject(size_t size)
{
    if (available_ < size) {
        return INVALID_OBJECT;
    }
    FreeObject *curFreeObject = freeObject_;
    while (curFreeObject != INVALID_OBJECT) {
        curFreeObject->AsanUnPoisonFreeObject();
        if (curFreeObject->Available() >= size) {
            curFreeObject->AsanPoisonFreeObject();
            return curFreeObject;
        }
        FreeObject *preFreeObject = curFreeObject;
        curFreeObject = curFreeObject->GetNext();
        preFreeObject->AsanPoisonFreeObject();
    }
    return INVALID_OBJECT;
}
}  // namespace panda::ecmascript
