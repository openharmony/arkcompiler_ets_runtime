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

#ifndef ECMASCRIPT_MEM_BARRIERS_INL_H
#define ECMASCRIPT_MEM_BARRIERS_INL_H

#include "ecmascript/base/config.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {
template<WriteBarrierType writeType = WriteBarrierType::NORMAL>
static ARK_INLINE void WriteBarrier(const JSThread *thread, void *obj, size_t offset, JSTaggedType value)
{
    ASSERT(value != JSTaggedValue::VALUE_UNDEFINED);
    Region *objectRegion = Region::ObjectAddressToRange(static_cast<TaggedObject *>(obj));
    Region *valueRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(value));
#if ECMASCRIPT_ENABLE_BARRIER_CHECK
    if (!thread->GetEcmaVM()->GetHeap()->IsAlive(JSTaggedValue(value).GetHeapObject())) {
        LOG_FULL(FATAL) << "WriteBarrier checked value:" << value << " is invalid!";
    }
#endif
    uintptr_t slotAddr = ToUintPtr(obj) + offset;
    if (objectRegion->InGeneralOldSpace() && valueRegion->InGeneralNewSpace()) {
        // Should align with '8' in 64 and 32 bit platform
        ASSERT((slotAddr % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
        objectRegion->InsertOldToNewRSet(slotAddr);
    } else if (!objectRegion->InSharedHeap() && valueRegion->InSharedSweepableSpace()) {
#ifndef NDEBUG
        if (UNLIKELY(JSTaggedValue(value).IsWeakForHeapObject())) {
            CHECK_NO_LOCAL_TO_SHARE_WEAK_REF_HANDLE;
        }
#endif
        objectRegion->InsertLocalToShareRSet(slotAddr);
    } else if (valueRegion->InEdenSpace() && objectRegion->InYoungSpace()) {
        objectRegion->InsertNewToEdenRSet(slotAddr);
    }
    ASSERT(!objectRegion->InSharedHeap() || valueRegion->InSharedHeap());
    if (!valueRegion->InSharedHeap() && thread->IsConcurrentMarkingOrFinished()) {
        Barriers::Update(thread, slotAddr, objectRegion, reinterpret_cast<TaggedObject *>(value),
                         valueRegion, writeType);
    }
    if (valueRegion->InSharedSweepableSpace() && thread->IsSharedConcurrentMarkingOrFinished()) {
        if constexpr (writeType != WriteBarrierType::DESERIALIZE) {
            Barriers::UpdateShared(thread, reinterpret_cast<TaggedObject *>(value), valueRegion);
        } else {
            // In deserialize, will never add references from old object(not allocated by deserialing) to
            // new object(allocated by deserializing), only two kinds of references(new->old, new->new) will
            // be added, the old object is considered as serialize_root, and be marked and pushed in
            // SharedGC::MarkRoots, so just mark all the new object is enough, do not need to push them to
            // workmanager and recursively visit slots of that.
            ASSERT(DaemonThread::GetInstance()->IsConcurrentMarkingOrFinished());
            valueRegion->AtomicMark(JSTaggedValue(value).GetHeapObject());
        }
    }
}

template<bool needWriteBarrier>
inline void Barriers::SetObject(const JSThread *thread, void *obj, size_t offset, JSTaggedType value)
{
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    *reinterpret_cast<JSTaggedType *>(reinterpret_cast<uintptr_t>(obj) + offset) = value;
    if constexpr (needWriteBarrier) {
        WriteBarrier(thread, obj, offset, value);
    }
}

inline void Barriers::SynchronizedSetClass(const JSThread *thread, void *obj, JSTaggedType value)
{
    reinterpret_cast<volatile std::atomic<JSTaggedType> *>(obj)->store(value, std::memory_order_release);
    WriteBarrier(thread, obj, 0, value);
}

inline void Barriers::SynchronizedSetObject(const JSThread *thread, void *obj, size_t offset, JSTaggedType value,
                                            bool isPrimitive)
{
    reinterpret_cast<volatile std::atomic<JSTaggedType> *>(ToUintPtr(obj) + offset)->store(value,
        std::memory_order_release);
    if (!isPrimitive) {
        WriteBarrier(thread, obj, offset, value);
    }
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_BARRIERS_INL_H
