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

#ifndef ECMASCRIPT_MEM_HEAP_INL_H
#define ECMASCRIPT_MEM_HEAP_INL_H

#include "ecmascript/mem/heap.h"

#include "ecmascript/base/block_hook_scope.h"
#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/mem_map_allocator.h"

namespace panda::ecmascript {
using BlockHookScope = base::BlockHookScope;
#define CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, space, message)                                         \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        size_t oomOvershootSize = GetEcmaVM()->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();   \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        object = reinterpret_cast<TaggedObject *>((space)->Allocate(size));                                 \
        ThrowOutOfMemoryError(size, message);                                                               \
    }                                                                                                       \

template<class Callback>
void Heap::EnumerateOldSpaceRegions(const Callback &cb, Region *region) const
{
    oldSpace_->EnumerateRegions(cb, region);
    appSpawnSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateSnapshotSpaceRegions(const Callback &cb) const
{
    snapshotSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonNewSpaceRegions(const Callback &cb) const
{
    oldSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateCollectRegionSet(cb);
    appSpawnSpace_->EnumerateRegions(cb);
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonNewSpaceRegionsWithRecord(const Callback &cb) const
{
    oldSpace_->EnumerateRegionsWithRecord(cb);
    snapshotSpace_->EnumerateRegionsWithRecord(cb);
    nonMovableSpace_->EnumerateRegionsWithRecord(cb);
    hugeObjectSpace_->EnumerateRegionsWithRecord(cb);
    machineCodeSpace_->EnumerateRegionsWithRecord(cb);
}

template<class Callback>
void Heap::EnumerateNewSpaceRegions(const Callback &cb) const
{
    activeSemiSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonMovableRegions(const Callback &cb) const
{
    snapshotSpace_->EnumerateRegions(cb);
    appSpawnSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateRegions(const Callback &cb) const
{
    activeSemiSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateCollectRegionSet(cb);
    appSpawnSpace_->EnumerateRegions(cb);
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::IterateOverObjects(const Callback &cb) const
{
    activeSemiSpace_->IterateOverObjects(cb);
    oldSpace_->IterateOverObjects(cb);
    appSpawnSpace_->IterateOverMarkedObjects(cb);
    nonMovableSpace_->IterateOverObjects(cb);
    hugeObjectSpace_->IterateOverObjects(cb);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateYoungOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(size);
    }

    auto object = reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
    if (object == nullptr) {
        CollectGarbage(SelectGCType(), GCReason::ALLOCATION_LIMIT);
        object = reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
        if (object == nullptr) {
            CollectGarbage(SelectGCType(), GCReason::ALLOCATION_FAILED);
            object = reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
            CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, activeSemiSpace_, "Heap::AllocateYoungOrHugeObject");
        }
    }
    return object;
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass, size_t size)
{
    auto object = AllocateYoungOrHugeObject(size);
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

uintptr_t Heap::AllocateYoungSync(size_t size)
{
    return activeSemiSpace_->AllocateSync(size);
}

bool Heap::MoveYoungRegionSync(Region *region)
{
    return activeSemiSpace_->SwapRegion(region, inactiveSemiSpace_);
}

void Heap::MergeToOldSpaceSync(LocalSpace *localSpace)
{
    oldSpace_->Merge(localSpace);
}

TaggedObject *Heap::TryAllocateYoungGeneration(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    auto object = reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
    if (object != nullptr) {
        object->SetClass(hclass);
    }
    return object;
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateOldOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(oldSpace_->Allocate(size));
    CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, oldSpace_, "Heap::AllocateOldOrHugeObject");
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

TaggedObject *Heap::AllocateReadOnlyOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateReadOnlyOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateReadOnlyOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(readOnlySpace_->Allocate(size));
    CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, readOnlySpace_, "Heap::AllocateReadOnlyOrHugeObject");
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

TaggedObject *Heap::AllocateNonMovableOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateNonMovableOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->CheckAndAllocate(size));
    CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, nonMovableSpace_, "Heap::AllocateNonMovableOrHugeObject");
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

TaggedObject *Heap::AllocateClassClass(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    auto object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->Allocate(size));
    if (UNLIKELY(object == nullptr)) {
        LOG_ECMA_MEM(FATAL) << "Heap::AllocateClassClass can not allocate any space";
        UNREACHABLE();
    }
    *reinterpret_cast<MarkWordType *>(ToUintPtr(object)) = reinterpret_cast<MarkWordType>(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

TaggedObject *Heap::AllocateHugeObject(size_t size)
{
    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    CheckAndTriggerOldGC(size);

    auto *object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
    if (UNLIKELY(object == nullptr)) {
        CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_LIMIT);
        object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
        if (UNLIKELY(object == nullptr)) {
            // if allocate huge object OOM, temporarily increase space size to avoid vm crash
            size_t oomOvershootSize = GetEcmaVM()->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();
            oldSpace_->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);
            object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
            if (UNLIKELY(object == nullptr)) {
                FatalOutOfMemoryError(size, "Heap::AllocateHugeObject");
            }
            ThrowOutOfMemoryError(size, "Heap::AllocateHugeObject");
        }
    }
    return object;
}

TaggedObject *Heap::AllocateHugeObject(JSHClass *hclass, size_t size)
{
    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    CheckAndTriggerOldGC(size);
    auto object = AllocateHugeObject(size);
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

TaggedObject *Heap::AllocateMachineCodeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    auto object = reinterpret_cast<TaggedObject *>(machineCodeSpace_->Allocate(size));
    CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, machineCodeSpace_, "Heap::AllocateMachineCodeObject");
    object->SetClass(hclass);
    OnAllocateEvent(reinterpret_cast<TaggedObject*>(object), size);
    return object;
}

uintptr_t Heap::AllocateSnapshotSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t object = snapshotSpace_->Allocate(size);
    if (UNLIKELY(object == 0)) {
        FatalOutOfMemoryError(size, "Heap::AllocateSnapshotSpaceObject");
    }
    return object;
}

void Heap::SwapNewSpace()
{
    activeSemiSpace_->Stop();
    inactiveSemiSpace_->Restart();

    SemiSpace *newSpace = inactiveSemiSpace_;
    inactiveSemiSpace_ = activeSemiSpace_;
    activeSemiSpace_ = newSpace;
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    activeSemiSpace_->SwapAllocationCounter(inactiveSemiSpace_);
#endif
    auto topAddress = activeSemiSpace_->GetAllocationTopAddress();
    auto endAddress = activeSemiSpace_->GetAllocationEndAddress();
    thread_->ReSetNewSpaceAllocationAddress(topAddress, endAddress);
}

void Heap::SwapOldSpace()
{
    compressSpace_->SetInitialCapacity(oldSpace_->GetInitialCapacity());
    auto *oldSpace = compressSpace_;
    compressSpace_ = oldSpace_;
    oldSpace_ = oldSpace;
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    oldSpace_->SwapAllocationCounter(compressSpace_);
#endif
}

void Heap::ReclaimRegions(TriggerGCType gcType)
{
    activeSemiSpace_->EnumerateRegionsWithRecord([] (Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
        region->ResetAliveObject();
        region->ClearGCFlag(RegionGCFlags::IN_NEW_TO_NEW_SET);
    });
    if (gcType == TriggerGCType::FULL_GC) {
        compressSpace_->Reset();
    } else if (gcType == TriggerGCType::OLD_GC) {
        oldSpace_->ReclaimCSet();
    }
    inactiveSemiSpace_->ReclaimRegions();

    sweeper_->WaitAllTaskFinished();
    EnumerateNonNewSpaceRegionsWithRecord([] (Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
    });
    if (!clearTaskFinished_) {
        os::memory::LockHolder holder(waitClearTaskFinishedMutex_);
        clearTaskFinished_ = true;
        waitClearTaskFinishedCV_.SignalAll();
    }
}

// only call in js-thread
void Heap::ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd)
{
    current->AtomicClearSweepingRSetInRange(freeStart, freeEnd);
    current->ClearOldToNewRSetInRange(freeStart, freeEnd);
    current->AtomicClearCrossRegionRSetInRange(freeStart, freeEnd);
}

size_t Heap::GetCommittedSize() const
{
    size_t result = activeSemiSpace_->GetCommittedSize() +
                    oldSpace_->GetCommittedSize() +
                    hugeObjectSpace_->GetCommittedSize() +
                    nonMovableSpace_->GetCommittedSize() +
                    machineCodeSpace_->GetCommittedSize() +
                    snapshotSpace_->GetCommittedSize();
    return result;
}

size_t Heap::GetHeapObjectSize() const
{
    size_t result = activeSemiSpace_->GetHeapObjectSize() +
                    oldSpace_->GetHeapObjectSize() +
                    hugeObjectSpace_->GetHeapObjectSize() +
                    nonMovableSpace_->GetHeapObjectSize() +
                    machineCodeSpace_->GetCommittedSize() +
                    snapshotSpace_->GetHeapObjectSize();
    return result;
}

uint32_t Heap::GetHeapObjectCount() const
{
    uint32_t count = 0;
    sweeper_->EnsureAllTaskFinished();
    this->IterateOverObjects([&count]([[maybe_unused]] TaggedObject *obj) {
        ++count;
    });
    return count;
}

void Heap::InitializeIdleStatusControl(std::function<void(bool)> callback)
{
    notifyIdleStatusCallback = callback;
    if (callback != nullptr) {
        OPTIONAL_LOG(ecmaVm_, INFO) << "Received idle status control call back";
        enableIdleGC_ = ecmaVm_->GetJSOptions().EnableIdleGC();
    }
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_INL_H
