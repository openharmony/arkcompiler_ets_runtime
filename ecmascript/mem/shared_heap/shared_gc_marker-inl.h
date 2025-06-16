/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H

#include "ecmascript/mem/shared_heap/shared_gc_marker.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
inline void SharedGCMarker::MarkObject(uint32_t threadId, TaggedObject *object, [[maybe_unused]] ObjectSlot &slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
        ASSERT(objectRegion->InSharedSweepableSpace());
        sWorkManager_->Push(threadId, object);
    }
}

inline void SharedGCMarkerBase::MarkObjectFromJSThread(WorkNode *&localBuffer, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
        sWorkManager_->PushToLocalMarkingBuffer(localBuffer, object);
    }
}

inline void SharedGCMarkerBase::HandleLocalRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        MarkObject(threadId, value.GetTaggedObject(), slot);
    }
}

inline void SharedGCMarkerBase::HandleLocalRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsInSharedSweepableSpace()) {
            if (value.IsWeakForHeapObject()) {
                LOG_ECMA_MEM(FATAL) << "Weak Reference in SharedGCMarker roots";
            }
            MarkObject(threadId, value.GetTaggedObject(), slot);
        }
    }
}

inline void SharedGCMarkerBase::RecordWeakReference(uint32_t threadId, JSTaggedType *slot)
{
    sWorkManager_->PushWeakReference(threadId, slot);
}

inline void SharedGCMarkerBase::RecordObject(JSTaggedValue value, uint32_t threadId, void *mem)
{
    if (value.IsWeakForHeapObject()) {
        RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem));
    } else {
        ObjectSlot slot(ToUintPtr(mem));
        MarkObject(threadId, value.GetTaggedObject(), slot);
    }
}

template<SharedMarkType markType>
inline bool SharedGCMarkerBase::GetVisitor(JSTaggedValue value, uint32_t threadId, void *mem)
{
    if (value.IsInSharedSweepableSpace()) {
        if constexpr (markType == SharedMarkType::CONCURRENT_MARK_INITIAL_MARK) {
            // For now if record weak references from local to share in marking root, the slots
            // may be invalid due to LocalGC, so just mark them as strong-reference.
            ObjectSlot slot(ToUintPtr(mem));
            MarkObject(threadId, value.GetHeapObject(), slot);
        } else {
            static_assert(markType == SharedMarkType::NOT_CONCURRENT_MARK);
            RecordObject(value, threadId, mem);
        }
        return true;
    }
    return false;
}

template<SharedMarkType markType>
inline auto SharedGCMarkerBase::GenerateRSetVisitor(uint32_t threadId)
{
    auto visitor = [this, threadId](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());
        return GetVisitor<markType>(value, threadId, mem);
    };
    return visitor;
}

template<SharedMarkType markType>
inline void SharedGCMarkerBase::ProcessVisitorOfDoMark(uint32_t threadId)
{
    auto rSetVisitor = GenerateRSetVisitor<markType>(threadId);
    auto visitor = [rSetVisitor](Region *region, RememberedSet *rSet) {
        rSet->IterateAllMarkedBits(ToUintPtr(region), rSetVisitor);
    };
    for (RSetWorkListHandler *handler : rSetHandlers_) {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarker::ProcessRSet", "");
        handler->ProcessAll(visitor);
        // To ensure the accuracy of the state range, notify finished is executed on js thread and deamon thread.
        // Reentrant does not cause exceptions because all the values are set to false.
        NotifyThreadProcessRsetFinished(handler->GetOwnerThreadUnsafe());
    }
}

template<SharedMarkType markType>
inline void SharedGCMarkerBase::DoMark(uint32_t threadId)
{
    if constexpr (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        ProcessVisitorOfDoMark<markType>(threadId);
    }
    ProcessMarkStack(threadId);
}

inline bool SharedGCMarkerBase::MarkObjectOfProcessVisitor(void *mem, WorkNode *&localBuffer)
{
    ObjectSlot slot(ToUintPtr(mem));
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        // For now if record weak references from local to share in marking root, the slots
        // may be invalid due to LocalGC, so just mark them as strong-reference.
        MarkObjectFromJSThread(localBuffer, value.GetHeapObject());
        return true;
    }

    // clear bit.
    return false;
}

inline void SharedGCMarkerBase::ProcessVisitor(RSetWorkListHandler *handler)
{
    WorkNode *&localBuffer = handler->GetHeap()->GetMarkingObjectLocalBuffer();
    auto rSetVisitor = [this, &localBuffer](void *mem) -> bool {
        return MarkObjectOfProcessVisitor(mem, localBuffer);
    };
    auto visitor = [rSetVisitor](Region *region, RememberedSet *rSet) {
        rSet->IterateAllMarkedBits(ToUintPtr(region), rSetVisitor);
    };
    handler->ProcessAll(visitor);
}

inline void SharedGCMarkerBase::ProcessThenMergeBackRSetFromBoundJSThread(RSetWorkListHandler *handler)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarker::ProcessRSet", "");
    ASSERT(JSThread::GetCurrent() == handler->GetHeap()->GetEcmaVM()->GetJSThread());
    ASSERT(JSThread::GetCurrent()->IsInRunningState());
    ProcessVisitor(handler);
    handler->WaitFinishedThenMergeBack();
}

inline void SharedGCMarkerBase::NotifyThreadProcessRsetStart(JSThread *localThread)
{
    // This method is called within the GCIterateThreadList method,
    // so the thread lock problem does not need to be considered.
    ASSERT(localThread != nullptr);
    localThread->SetProcessingLocalToSharedRset(true);
}

inline void SharedGCMarkerBase::NotifyThreadProcessRsetFinished(JSThread *localThread)
{
    // The localThread may have been released or reused.
    Runtime::GetInstance()->GCIterateThreadList([localThread](JSThread *thread) {
        if (localThread == thread) {
            thread->SetProcessingLocalToSharedRset(false);
            return;
        }
    });
}

void SharedGCMovableMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot &slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!NeedEvacuate(objectRegion)) {
        if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
            auto size = object->GetSize();
            objectRegion->IncreaseAliveObject(size);
            sWorkManager_->Push(threadId, object);
        }
        return;
    }

    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        return;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

void SharedGCMovableMarker::MarkValue(uint32_t threadId, ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        if (!value.IsWeakForHeapObject()) {
            MarkObject(threadId, value.GetTaggedObject(), slot);
        } else {
            RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
        }
    }
}

bool SharedGCMovableMarker::NeedEvacuate(Region *region)
{
    return region->InSharedOldSpace();
}

void SharedGCMovableMarker::EvacuateObject(uint32_t threadId, TaggedObject *object,
    const MarkWord &markWord, ObjectSlot slot)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    uintptr_t forwardAddress = AllocateForwardAddress(threadId, size);
    RawCopyObject(ToUintPtr(object), forwardAddress, size, markWord);

    auto oldValue = markWord.GetValue();
    auto result = Barriers::AtomicSetPrimitive(object, 0, oldValue,
                                               MarkWord::FromForwardingAddress(forwardAddress));
    if (result == oldValue) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, slot);
        return;
    }
    UpdateForwardAddressIfFailed(object, forwardAddress, size, slot);
}

uintptr_t SharedGCMovableMarker::AllocateDstSpace(uint32_t threadId, size_t size)
{
    uintptr_t forwardAddress = 0;
    forwardAddress = sWorkManager_->GetTlabAllocator(threadId)->Allocate(size, SHARED_COMPRESS_SPACE);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: "
                            << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

inline void SharedGCMovableMarker::RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size,
    const MarkWord &markWord)
{
    if (memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(fromAddress + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
    }
    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
}

void SharedGCMovableMarker::UpdateForwardAddressIfSuccess(uint32_t threadId, TaggedObject *object, JSHClass *klass,
                                                          uintptr_t toAddress, size_t size, ObjectSlot slot)
{
    sWorkManager_->IncreaseAliveSize(threadId, size);
    if (klass->HasReferenceField()) {
        sWorkManager_->Push(threadId, reinterpret_cast<TaggedObject *>(toAddress));
    }

    if (UNLIKELY(sHeap_->InHeapProfiler())) {
        sHeap_->OnMoveEvent(reinterpret_cast<intptr_t>(object), reinterpret_cast<TaggedObject *>(toAddress), size);
    }

    slot.Update(reinterpret_cast<TaggedObject *>(toAddress));
}

void SharedGCMovableMarker::UpdateForwardAddressIfFailed(TaggedObject *object, uintptr_t toAddress, size_t size,
    ObjectSlot slot)
{
    FreeObject::FillFreeObject(sHeap_, toAddress, size);
    TaggedObject *dst = MarkWord(object).ToForwardingAddress();
    slot.Update(dst);
}

uintptr_t SharedGCMovableMarker::AllocateForwardAddress(uint32_t threadId, size_t size)
{
    return AllocateDstSpace(threadId, size);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H
