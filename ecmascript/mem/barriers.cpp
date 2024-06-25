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
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void Barriers::UpdateWithoutEden(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion, TaggedObject *value,
                                 Region *valueRegion, WriteBarrierType writeType)
{
    ASSERT(!valueRegion->InSharedHeap());
    auto heap = thread->GetEcmaVM()->GetHeap();
    if (heap->IsConcurrentFullMark()) {
        if (valueRegion->InCollectSet() && !objectRegion->InGeneralNewSpaceOrCSet()) {
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
        heap->GetWorkManager()->Push(MAIN_THREAD_INDEX, heapValue);
    }
}

void Barriers::Update(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion, TaggedObject *value,
                      Region *valueRegion, WriteBarrierType writeType)
{
    if (valueRegion->InSharedHeap()) {
        return;
    }
    auto heap = thread->GetEcmaVM()->GetHeap();
    if (heap->IsConcurrentFullMark()) {
        if (valueRegion->InCollectSet() && !objectRegion->InGeneralNewSpaceOrCSet()) {
            objectRegion->AtomicInsertCrossRegionRSet(slotAddr);
        }
    } else if (heap->IsYoungMark()) {
        if (!valueRegion->InGeneralNewSpace()) {
            return;
        }
    } else {
        if (!valueRegion->InEdenSpace()) {
            return;
        }
    }

    // Weak ref record and concurrent mark record maybe conflict.
    // This conflict is solved by keeping alive weak reference. A small amount of floating garbage may be added.
    TaggedObject *heapValue = JSTaggedValue(value).GetHeapObject();
    if (writeType != WriteBarrierType::DESERIALIZE && valueRegion->AtomicMark(heapValue)) {
        heap->GetWorkManager()->Push(MAIN_THREAD_INDEX, heapValue);
    }
}

void Barriers::UpdateShared(const JSThread *thread, TaggedObject *value, Region *valueRegion)
{
    ASSERT(DaemonThread::GetInstance()->IsConcurrentMarkingOrFinished());
    ASSERT(valueRegion->InSharedSweepableSpace());

    // Weak ref record and concurrent mark record maybe conflict.
    // This conflict is solved by keeping alive weak reference. A small amount of floating garbage may be added.
    TaggedObject *heapValue = JSTaggedValue(value).GetHeapObject();
    if (valueRegion->AtomicMark(heapValue)) {
        Heap *heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        WorkNode *&localBuffer = heap->GetMarkingObjectLocalBuffer();
        SharedHeap::GetInstance()->GetWorkManager()->PushToLocalMarkingBuffer(localBuffer, heapValue);
    }
}
}  // namespace panda::ecmascript
