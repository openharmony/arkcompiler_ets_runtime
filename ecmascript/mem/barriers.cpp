/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
void Barriers::Update(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion, TaggedObject *value,
                      Region *valueRegion, WriteBarrierType writeType)
{
    auto heap = thread->GetEcmaVM()->GetHeap();
    if (heap->IsConcurrentFullMark()) {
        if (valueRegion->InCollectSet() && !objectRegion->InYoungSpaceOrCSet()) {
            objectRegion->AtomicInsertCrossRegionRSet(slotAddr);
        }
    } else {
        if (!valueRegion->InYoungSpace()) {
            return;
        }
    }

    // Weak ref record and concurrent mark record maybe conflict.
    // This conflict is solved by keeping alive weak reference. A small amount of floating garbage may be added.
    TaggedObject *heapValue = JSTaggedValue(value).GetHeapObject();
    if (writeType != WriteBarrierType::DESERIALIZE && valueRegion->AtomicMark(heapValue)) {
        heap->GetWorkManager()->Push(MAIN_THREAD_INDEX, heapValue, valueRegion);
    }
}

// For work deserialize, deserialize root object will be set to another object, however, this object may have been
// marked by concurrent mark, this may cause deserialize root object miss mark, this is to ensure the deserialize object
// will been marked
void Barriers::MarkAndPushForDeserialize(const JSThread *thread, TaggedObject *object)
{
    auto heap = thread->GetEcmaVM()->GetHeap();
    Region *valueRegion = Region::ObjectAddressToRange(object);
    if ((heap->IsConcurrentFullMark() || valueRegion->InYoungSpace()) && valueRegion->AtomicMark(object)) {
        heap->GetWorkManager()->Push(MAIN_THREAD_INDEX, object, valueRegion);
    }
}
}  // namespace panda::ecmascript
