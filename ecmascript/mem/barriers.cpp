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

#include "ecmascript/mem/work_manager-inl.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void Barriers::Update(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion, TaggedObject *value,
                      Region *valueRegion, WriteBarrierType writeType)
{
    ASSERT(!g_isEnableCMCGC);
    if (valueRegion->InSharedHeap()) {
        return;
    }
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
    if (valueRegion->IsFreshRegion()) {
        valueRegion->NonAtomicMark(heapValue);
    } else if (writeType != WriteBarrierType::DESERIALIZE && valueRegion->AtomicMark(heapValue)) {
        heap->GetWorkManager()->GetWorkNodeHolder(MAIN_THREAD_INDEX)->Push(heapValue);
    }
}

void Barriers::UpdateShared(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion, TaggedObject *value,
                            Region *valueRegion)
{
    ASSERT(!g_isEnableCMCGC);

    ASSERT(DaemonThread::GetInstance()->IsConcurrentMarkingOrFinished());
    ASSERT(valueRegion->InSharedSweepableSpace());
    if (valueRegion->InSCollectSet() && objectRegion->InSharedHeap()) {
        objectRegion->AtomicInsertCrossRegionRSet(slotAddr);
    }
    // Weak ref record and concurrent mark record maybe conflict.
    // This conflict is solved by keeping alive weak reference. A small amount of floating garbage may be added.
    TaggedObject *heapValue = JSTaggedValue(value).GetHeapObject();
    if (valueRegion->AtomicMark(heapValue)) {
        Heap *heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        WorkNode *&localBuffer = heap->GetMarkingObjectLocalBuffer();
        SharedHeap::GetInstance()->GetWorkManager()->PushToLocalMarkingBuffer(localBuffer, heapValue);
    }
}

template <Region::RegionSpaceKind kind>
ARK_NOINLINE bool BatchBitSet([[maybe_unused]] const JSThread* thread, Region* objectRegion, JSTaggedValue* dst,
                              size_t count)
{
    bool allValueNotHeap = true;
    Region::Updater updater = objectRegion->GetBatchRSetUpdater<kind>(ToUintPtr(dst));
    for (size_t i = 0; i < count; i++, updater.Next()) {
        JSTaggedValue taggedValue = dst[i];
        if (!taggedValue.IsHeapObject()) {
            continue;
        }
        allValueNotHeap = false;
        const Region* valueRegion = Region::ObjectAddressToRange(taggedValue.GetTaggedObject());
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
        ASSERT(taggedValue.GetRawData() != JSTaggedValue::VALUE_UNDEFINED);
        if (!thread->GetEcmaVM()->GetHeap()->IsAlive(taggedValue.GetHeapObject())) {
            LOG_FULL(FATAL) << "WriteBarrier checked value:" << taggedValue.GetRawData() << " is invalid!";
        }
#endif
        if (valueRegion->InSharedSweepableSpace()) {
#ifndef NDEBUG
            if (UNLIKELY(taggedValue.IsWeakForHeapObject())) {
                CHECK_NO_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
            }
#endif
            updater.UpdateLocalToShare();
            continue;
        }
        if constexpr (kind == Region::InGeneralOld) {
            if (valueRegion->InYoungSpace()) {
                updater.UpdateOldToNew();
                continue;
            }
        }
    }
    return allValueNotHeap;
}

void Barriers::CMCWriteBarrier(const JSThread *thread, void *obj, size_t offset, JSTaggedType value)
{
    ASSERT(g_isEnableCMCGC);
    common::BaseRuntime::WriteBarrier(obj, (void *)((uintptr_t)obj + offset), (void*)value);
    return;
}

void Barriers::CMCArrayCopyWriteBarrier(const JSThread *thread, const TaggedObject *dstObj, void* src, void* dst,
                                        size_t count)
{
    // need opt
    ASSERT(g_isEnableCMCGC);
    uintptr_t *dstPtr = reinterpret_cast<uintptr_t *>(dst);
    uintptr_t *srcPtr = reinterpret_cast<uintptr_t *>(src);
    for (size_t i = 0; i < count; i++) {
        uintptr_t offset = i * sizeof(uintptr_t);
        uintptr_t value = srcPtr[i];
        void* obj = reinterpret_cast<void*>(const_cast<TaggedObject*>(dstObj));
        void* field = reinterpret_cast<void*>((uintptr_t)dst + offset);
        common::BaseRuntime::WriteBarrier(obj, field, (void*)value);
    }
    return;
}

template bool BatchBitSet<Region::InYoung>(const JSThread*, Region*, JSTaggedValue*, size_t);
template bool BatchBitSet<Region::InGeneralOld>(const JSThread*, Region*, JSTaggedValue*, size_t);
template bool BatchBitSet<Region::Other>(const JSThread*, Region*, JSTaggedValue*, size_t);

}  // namespace panda::ecmascript
