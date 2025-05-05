/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/free_object.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/tagged_object-inl.h"
#include "ecmascript/global_env_constants-inl.h"

namespace panda::ecmascript {
FreeObject *FreeObject::FillFreeObject(BaseHeap *heap, uintptr_t address, size_t size)
{
    ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(address), size);
    auto globalConst = heap->GetGlobalConst();
    FreeObject *object = nullptr;
    if (size >= FreeObject::SIZE_OFFSET && size < FreeObject::SIZE) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(
            JSHClass::Cast(globalConst->GetFreeObjectWithOneFieldClass().GetTaggedObject()));
        object->SetNext(INVALID_OBJECT);
    } else if (size >= FreeObject::SIZE) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(
            JSHClass::Cast(globalConst->GetFreeObjectWithTwoFieldClass().GetTaggedObject()));
        object->SetAvailable(size);
        object->SetNext(INVALID_OBJECT);
    } else if (size == FreeObject::NEXT_OFFSET) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(
            JSHClass::Cast(globalConst->GetFreeObjectWithNoneFieldClass().GetTaggedObject()));
    } else {
        LOG_ECMA(DEBUG) << "Fill free object size is smaller";
    }
#ifdef ARK_ASAN_ON
    ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(address), size);
#endif
    return object;
}

uint32_t FreeObject::Available() const
{
    auto hclass = GetClass();
    if (hclass->IsFreeObjectWithShortField()) {
        return hclass->GetObjectSize();
    }
    ASSERT(GetSize().IsInt());
    return GetSize().GetInt();
}

bool FreeObject::IsFreeObject() const
{
    return GetClass()->IsFreeObject();
}

// Before operating any freeobject, need to mark unpoison when is_asan is true.
void FreeObject::AsanUnPoisonFreeObject() const
{
#ifdef ARK_ASAN_ON
    ASAN_UNPOISON_MEMORY_REGION(this, NEXT_OFFSET);
    if (GetClass()->IsFreeObjectWithOneField()) {
        ASAN_UNPOISON_MEMORY_REGION(this, SIZE_OFFSET);
    } else if (GetClass()->IsFreeObjectWithTwoField()) {
        ASAN_UNPOISON_MEMORY_REGION(this, SIZE);
    }
#endif
}

// After operating any freeobject, need to marked poison again when is_asan is true
void FreeObject::AsanPoisonFreeObject() const
{
#ifdef ARK_ASAN_ON
    if (GetClass()->IsFreeObjectWithNoneField()) {
        ASAN_POISON_MEMORY_REGION(this, NEXT_OFFSET);
    } else if (GetClass()->IsFreeObjectWithOneField()) {
        ASAN_POISON_MEMORY_REGION(this, SIZE_OFFSET);
    } else if (GetClass()->IsFreeObjectWithTwoField()) {
        ASAN_POISON_MEMORY_REGION(this, SIZE);
    }
#endif
}

}  // namespace panda::ecmascript
