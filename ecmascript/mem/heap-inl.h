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

#include "ecmascript/daemon/daemon_task-inl.h"
#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/thread_local_allocation_buffer.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/mem_map_allocator.h"

namespace panda::ecmascript {
#define CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, space, message)                                         \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        EcmaVM *vm = GetEcmaVM();                                                                           \
        size_t oomOvershootSize = vm->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();            \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        if ((space)->IsOOMDumpSpace()) {                                                                    \
            DumpHeapSnapshotBeforeOOM();                                                                    \
        }                                                                                                   \
        StatisticHeapDetail();                                                                              \
        ThrowOutOfMemoryError(GetJSThread(), size, message);                                                \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(size));                               \
    }

#define CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, space, message)                                \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        size_t oomOvershootSize = GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();                \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        DumpHeapSnapshotBeforeOOM(true, thread);                                                            \
        ThrowOutOfMemoryError(thread, size, message);                                                       \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(thread, size));                       \
    }

#ifdef ENABLE_JITFORT
#define CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR_FORT(object, size, space, desc, message)                   \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        EcmaVM *vm = GetEcmaVM();                                                                           \
        size_t oomOvershootSize = vm->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();            \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        SetMachineCodeOutOfMemoryError(GetJSThread(), size, message);                                       \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(size, desc));                         \
    }
#else
#define CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR(object, size, space, message)                              \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        EcmaVM *vm = GetEcmaVM();                                                                           \
        size_t oomOvershootSize = vm->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();            \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        SetMachineCodeOutOfMemoryError(GetJSThread(), size, message);                                       \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(size));                               \
    }

#endif

template<class Callback>
void SharedHeap::EnumerateOldSpaceRegions(const Callback &cb) const
{
    sOldSpace_->EnumerateRegions(cb);
    sNonMovableSpace_->EnumerateRegions(cb);
    sHugeObjectSpace_->EnumerateRegions(cb);
}

template<class Callback>
void SharedHeap::EnumerateOldSpaceRegionsWithRecord(const Callback &cb) const
{
    sOldSpace_->EnumerateRegionsWithRecord(cb);
    sNonMovableSpace_->EnumerateRegionsWithRecord(cb);
    sHugeObjectSpace_->EnumerateRegionsWithRecord(cb);
}

template<class Callback>
void SharedHeap::IterateOverObjects(const Callback &cb) const
{
    sOldSpace_->IterateOverObjects(cb);
    sNonMovableSpace_->IterateOverObjects(cb);
    sHugeObjectSpace_->IterateOverObjects(cb);
}

template<class Callback>
void Heap::EnumerateOldSpaceRegions(const Callback &cb, Region *region) const
{
    oldSpace_->EnumerateRegions(cb, region);
    appSpawnSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
    hugeMachineCodeSpace_->EnumerateRegions(cb);
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
    hugeMachineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateNonNewSpaceRegionsWithRecord(const Callback &cb) const
{
    oldSpace_->EnumerateRegionsWithRecord(cb);
    snapshotSpace_->EnumerateRegionsWithRecord(cb);
    nonMovableSpace_->EnumerateRegionsWithRecord(cb);
    hugeObjectSpace_->EnumerateRegionsWithRecord(cb);
    machineCodeSpace_->EnumerateRegionsWithRecord(cb);
    hugeMachineCodeSpace_->EnumerateRegionsWithRecord(cb);
}

template<class Callback>
void Heap::EnumerateEdenSpaceRegions(const Callback &cb) const
{
    edenSpace_->EnumerateRegions(cb);
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
    hugeMachineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::EnumerateRegions(const Callback &cb) const
{
    edenSpace_->EnumerateRegions(cb);
    activeSemiSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateCollectRegionSet(cb);
    appSpawnSpace_->EnumerateRegions(cb);
    snapshotSpace_->EnumerateRegions(cb);
    nonMovableSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
    hugeMachineCodeSpace_->EnumerateRegions(cb);
}

template<class Callback>
void Heap::IterateOverObjects(const Callback &cb, bool isSimplify) const
{
    edenSpace_->IterateOverObjects(cb);
    activeSemiSpace_->IterateOverObjects(cb);
    oldSpace_->IterateOverObjects(cb);
    nonMovableSpace_->IterateOverObjects(cb);
    hugeObjectSpace_->IterateOverObjects(cb);
    machineCodeSpace_->IterateOverObjects(cb);
    hugeMachineCodeSpace_->IterateOverObjects(cb);
    snapshotSpace_->IterateOverObjects(cb);
    if (!isSimplify) {
        readOnlySpace_->IterateOverObjects(cb);
        appSpawnSpace_->IterateOverMarkedObjects(cb);
    }
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateYoungOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateYoungOrHugeObject(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = nullptr;
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = AllocateHugeObject(size);
    } else {
        object = AllocateInGeneralNewSpace(size);
        if (object == nullptr) {
            CollectGarbage(SelectGCType(), GCReason::ALLOCATION_FAILED);
            object = AllocateInGeneralNewSpace(size);
            if (object == nullptr) {
                CollectGarbage(SelectGCType(), GCReason::ALLOCATION_FAILED);
                object = AllocateInGeneralNewSpace(size);
                CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, activeSemiSpace_, "Heap::AllocateYoungOrHugeObject");
            }
        }
    }
    return object;
}

TaggedObject *Heap::AllocateInGeneralNewSpace(size_t size)
{
    if (enableEdenGC_) {
        auto object = reinterpret_cast<TaggedObject *>(edenSpace_->Allocate(size));
        if (object != nullptr) {
            return object;
        }
    }
    return reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass, size_t size)
{
    auto object = AllocateYoungOrHugeObject(size);
    object->SetClass(thread_, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

void BaseHeap::SetHClassAndDoAllocateEvent(JSThread *thread, TaggedObject *object, JSHClass *hclass,
                                           [[maybe_unused]] size_t size)
{
    object->SetClass(thread, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
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
        object->SetClass(thread_, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    TaggedObject *object = AllocateOldOrHugeObject(hclass, size);
    if (object == nullptr) {
        LOG_ECMA(FATAL) << "Heap::AllocateOldOrHugeObject:object is nullptr";
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = nullptr;
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = AllocateHugeObject(hclass, size);
    } else {
        object = reinterpret_cast<TaggedObject *>(oldSpace_->Allocate(size));
        CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, oldSpace_, "Heap::AllocateOldOrHugeObject");
        object->SetClass(thread_, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), reinterpret_cast<TaggedObject*>(object), size);
#endif
    return object;
}

TaggedObject *Heap::AllocateReadOnlyOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    TaggedObject *object = AllocateReadOnlyOrHugeObject(hclass, size);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *Heap::AllocateReadOnlyOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = nullptr;
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = AllocateHugeObject(hclass, size);
    } else {
        object = reinterpret_cast<TaggedObject *>(readOnlySpace_->Allocate(size));
        CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, readOnlySpace_, "Heap::AllocateReadOnlyOrHugeObject");
        object->SetClass(thread_, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *Heap::AllocateNonMovableOrHugeObject(JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    TaggedObject *object = AllocateNonMovableOrHugeObject(hclass, size);
    if (object == nullptr) {
        LOG_ECMA(FATAL) << "Heap::AllocateNonMovableOrHugeObject:object is nullptr";
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *Heap::AllocateNonMovableOrHugeObject(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = nullptr;
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = AllocateHugeObject(hclass, size);
    } else {
        object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->CheckAndAllocate(size));
        CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, nonMovableSpace_, "Heap::AllocateNonMovableOrHugeObject");
        object->SetClass(thread_, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
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
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *SharedHeap::AllocateClassClass(JSThread *thread, JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    auto object = reinterpret_cast<TaggedObject *>(sReadOnlySpace_->Allocate(thread, size));
    if (UNLIKELY(object == nullptr)) {
        LOG_ECMA_MEM(FATAL) << "Heap::AllocateClassClass can not allocate any space";
        UNREACHABLE();
    }
    *reinterpret_cast<MarkWordType *>(ToUintPtr(object)) = reinterpret_cast<MarkWordType>(hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *Heap::AllocateHugeObject(size_t size)
{
    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    CheckAndTriggerOldGC(size);

    auto *object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
    if (UNLIKELY(object == nullptr)) {
        CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_FAILED);
        object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
        if (UNLIKELY(object == nullptr)) {
            // if allocate huge object OOM, temporarily increase space size to avoid vm crash
            size_t oomOvershootSize = config_.GetOutOfMemoryOvershootSize();
            oldSpace_->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);
            DumpHeapSnapshotBeforeOOM();
            StatisticHeapDetail();
            object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
            ThrowOutOfMemoryError(thread_, size, "Heap::AllocateHugeObject");
            object = reinterpret_cast<TaggedObject *>(hugeObjectSpace_->Allocate(size, thread_));
            if (UNLIKELY(object == nullptr)) {
                FatalOutOfMemoryError(size, "Heap::AllocateHugeObject");
            }
        }
    }
    return object;
}

TaggedObject *Heap::AllocateHugeObject(JSHClass *hclass, size_t size)
{
    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    CheckAndTriggerOldGC(size);
    auto object = AllocateHugeObject(size);
    object->SetClass(thread_, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

#ifdef ENABLE_JITFORT
TaggedObject *Heap::AllocateHugeMachineCodeObject(size_t size, MachineCodeDesc &desc)
{
    auto *object = reinterpret_cast<TaggedObject *>(hugeMachineCodeSpace_->Allocate(
        size, thread_, desc.instructionsSize, desc.instructionsAddr));
    return object;
}
#else
TaggedObject *Heap::AllocateHugeMachineCodeObject(size_t size)
{
    auto *object = reinterpret_cast<TaggedObject *>(hugeMachineCodeSpace_->Allocate(size, thread_));
    return object;
}
#endif

#ifdef ENABLE_JITFORT
TaggedObject *Heap::AllocateMachineCodeObject(JSHClass *hclass, size_t size, MachineCodeDesc &desc)
#else
TaggedObject *Heap::AllocateMachineCodeObject(JSHClass *hclass, size_t size)
#endif
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
#ifdef ENABLE_JITFORT
    desc.instructionsAddr = 0;
    if (size <= MAX_REGULAR_HEAP_OBJECT_SIZE) {
        // for non huge code cache obj, allocate fort space before allocating the code object
        if (machineCodeSpace_->sweepState_ == SweepState::SWEEPING) {
            GetSweeper()->EnsureTaskFinished(MACHINE_CODE_SPACE);
            LOG_JIT(DEBUG) << "Heap::AllocateMachineCode: MachineCode space sweep state = "
                << machineCodeSpace_->GetSweepState();
            JitFort::GetInstance()->UpdateFreeSpace();
        }
        uintptr_t mem = machineCodeSpace_->JitFortAllocate(desc.instructionsSize);
        if (mem == ToUintPtr(nullptr)) {
            SetMachineCodeOutOfMemoryError(GetJSThread(), size, "Heap::JitFortAllocate");
            return nullptr;
        }
        desc.instructionsAddr = mem;
    }
    auto object = (size > MAX_REGULAR_HEAP_OBJECT_SIZE) ?
        reinterpret_cast<TaggedObject *>(AllocateHugeMachineCodeObject(size, desc)) :
        reinterpret_cast<TaggedObject *>(machineCodeSpace_->Allocate(size, desc));
    CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR_FORT(object, size, machineCodeSpace_,
        desc, "Heap::AllocateMachineCodeObject");
#else
    auto object = (size > MAX_REGULAR_HEAP_OBJECT_SIZE) ?
        reinterpret_cast<TaggedObject *>(AllocateHugeMachineCodeObject(size)) :
        reinterpret_cast<TaggedObject *>(machineCodeSpace_->Allocate(size));
    CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR(object, size, machineCodeSpace_, "Heap::AllocateMachineCodeObject");
#endif
    object->SetClass(thread_, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

uintptr_t Heap::AllocateSnapshotSpace(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    uintptr_t object = snapshotSpace_->Allocate(size);
    if (UNLIKELY(object == 0)) {
        FatalOutOfMemoryError(size, "Heap::AllocateSnapshotSpaceObject");
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), reinterpret_cast<TaggedObject *>(object), size);
#endif
    return object;
}

TaggedObject *Heap::AllocateSharedNonMovableSpaceFromTlab(JSThread *thread, size_t size)
{
    ASSERT(!thread->IsJitThread());
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = reinterpret_cast<TaggedObject*>(sNonMovableTlab_->Allocate(size));
    if (object != nullptr) {
        return object;
    }
    if (!sNonMovableTlab_->NeedNewTlab(size)) {
        // slowpath
        return nullptr;
    }
    size_t newTlabSize = sNonMovableTlab_->ComputeSize();
    object = SharedHeap::GetInstance()->AllocateSNonMovableTlab(thread, newTlabSize);
    if (object == nullptr) {
        sNonMovableTlab_->DisableNewTlab();
        return nullptr;
    }
    uintptr_t begin = reinterpret_cast<uintptr_t>(object);
    sNonMovableTlab_->Reset(begin, begin + newTlabSize, begin + size);
    auto topAddress = sNonMovableTlab_->GetTopAddress();
    auto endAddress = sNonMovableTlab_->GetEndAddress();
    thread_->ReSetSNonMovableSpaceAllocationAddress(topAddress, endAddress);
    return object;
}

TaggedObject *Heap::AllocateSharedOldSpaceFromTlab(JSThread *thread, size_t size)
{
    ASSERT(!thread->IsJitThread());
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = reinterpret_cast<TaggedObject*>(sOldTlab_->Allocate(size));
    if (object != nullptr) {
        return object;
    }
    if (!sOldTlab_->NeedNewTlab(size)) {
        // slowpath
        return nullptr;
    }
    size_t newTlabSize = sOldTlab_->ComputeSize();
    object = SharedHeap::GetInstance()->AllocateSOldTlab(thread, newTlabSize);
    if (object == nullptr) {
        sOldTlab_->DisableNewTlab();
        return nullptr;
    }
    uintptr_t begin = reinterpret_cast<uintptr_t>(object);
    sOldTlab_->Reset(begin, begin + newTlabSize, begin + size);
    auto topAddress = sOldTlab_->GetTopAddress();
    auto endAddress = sOldTlab_->GetEndAddress();
    thread_->ReSetSOldSpaceAllocationAddress(topAddress, endAddress);
    return object;
}

void Heap::SwapNewSpace()
{
    activeSemiSpace_->Stop();
    size_t newOverShootSize = 0;
    if (!inBackground_ && gcType_ != TriggerGCType::FULL_GC && gcType_ != TriggerGCType::APPSPAWN_FULL_GC) {
        newOverShootSize = activeSemiSpace_->CalculateNewOverShootSize();
    }
    inactiveSemiSpace_->Restart(newOverShootSize);

    SemiSpace *newSpace = inactiveSemiSpace_;
    inactiveSemiSpace_ = activeSemiSpace_;
    activeSemiSpace_ = newSpace;
    if (UNLIKELY(ShouldVerifyHeap())) {
        inactiveSemiSpace_->EnumerateRegions([](Region *region) {
            region->SetInactiveSemiSpace();
        });
    }
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
        region->DeleteNewToEdenRSet();
        region->ClearGCFlag(RegionGCFlags::IN_NEW_TO_NEW_SET);
    });
    size_t cachedSize = inactiveSemiSpace_->GetInitialCapacity();
    if (gcType == TriggerGCType::FULL_GC) {
        compressSpace_->Reset();
        cachedSize = 0;
    } else if (gcType == TriggerGCType::OLD_GC) {
        oldSpace_->ReclaimCSet();
    }

    inactiveSemiSpace_->ReclaimRegions(cachedSize);
    sweeper_->WaitAllTaskFinished();
#ifdef ENABLE_JITFORT
    // machinecode space is not sweeped in young GC
    if (machineCodeSpace_->sweepState_ != SweepState::NO_SWEEP) {
        JitFort::GetInstance()->UpdateFreeSpace();
    }
#endif
    EnumerateNonNewSpaceRegionsWithRecord([] (Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
    });
    if (!clearTaskFinished_) {
        LockHolder holder(waitClearTaskFinishedMutex_);
        clearTaskFinished_ = true;
        waitClearTaskFinishedCV_.SignalAll();
    }
}

// only call in js-thread
void Heap::ClearSlotsRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd)
{
    if (!current->InGeneralNewSpace()) {
        // This clear may exist data race with concurrent sweeping, so use CAS
        current->AtomicClearSweepingOldToNewRSetInRange(freeStart, freeEnd);
        current->ClearOldToNewRSetInRange(freeStart, freeEnd);
        current->AtomicClearCrossRegionRSetInRange(freeStart, freeEnd);
    }
    current->ClearLocalToShareRSetInRange(freeStart, freeEnd);
    current->AtomicClearSweepingLocalToShareRSetInRange(freeStart, freeEnd);
}

size_t Heap::GetCommittedSize() const
{
    size_t result = edenSpace_->GetCommittedSize() +
                    activeSemiSpace_->GetCommittedSize() +
                    oldSpace_->GetCommittedSize() +
                    hugeObjectSpace_->GetCommittedSize() +
                    nonMovableSpace_->GetCommittedSize() +
                    machineCodeSpace_->GetCommittedSize() +
                    hugeMachineCodeSpace_->GetCommittedSize() +
                    readOnlySpace_->GetCommittedSize() +
                    appSpawnSpace_->GetCommittedSize() +
                    snapshotSpace_->GetCommittedSize();
    return result;
}

size_t Heap::GetHeapObjectSize() const
{
    size_t result = edenSpace_->GetHeapObjectSize() +
                    activeSemiSpace_->GetHeapObjectSize() +
                    oldSpace_->GetHeapObjectSize() +
                    hugeObjectSpace_->GetHeapObjectSize() +
                    nonMovableSpace_->GetHeapObjectSize() +
                    machineCodeSpace_->GetCommittedSize() +
                    hugeMachineCodeSpace_->GetCommittedSize() +
                    readOnlySpace_->GetCommittedSize() +
                    appSpawnSpace_->GetHeapObjectSize() +
                    snapshotSpace_->GetHeapObjectSize();
    return result;
}

size_t Heap::GetRegionCount() const
{
    size_t result = edenSpace_->GetRegionCount() +
        activeSemiSpace_->GetRegionCount() +
        oldSpace_->GetRegionCount() +
        oldSpace_->GetCollectSetRegionCount() +
        appSpawnSpace_->GetRegionCount() +
        snapshotSpace_->GetRegionCount() +
        nonMovableSpace_->GetRegionCount() +
        hugeObjectSpace_->GetRegionCount() +
        machineCodeSpace_->GetRegionCount() +
        hugeMachineCodeSpace_->GetRegionCount();
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

void SharedHeap::TryTriggerConcurrentMarking(JSThread *thread)
{
    if (!CheckCanTriggerConcurrentMarking(thread)) {
        return;
    }
    if (GetHeapObjectSize() >= globalSpaceConcurrentMarkLimit_) {
        TriggerConcurrentMarking<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_LIMIT>(thread);
    }
}

void SharedHeap::CollectGarbageFinish(bool inDaemon)
{
    if (inDaemon) {
        ASSERT(JSThread::GetCurrent() == dThread_);
#ifndef NDEBUG
        ASSERT(dThread_->HasLaunchedSuspendAll());
#endif
        dThread_->FinishRunningTask();
        NotifyGCCompleted();
        // Update to forceGC_ is in DaemeanSuspendAll, and protected by the Runtime::mutatorLock_,
        // so do not need lock.
        smartGCStats_.forceGC_ = false;
    }
    // Record alive object size after shared gc
    NotifyHeapAliveSizeAfterGC(GetHeapObjectSize());
    // Adjust shared gc trigger threshold
    AdjustGlobalSpaceAllocLimit();
    GetEcmaGCStats()->RecordStatisticAfterGC();
    GetEcmaGCStats()->PrintGCStatistic();
}

TaggedObject *SharedHeap::AllocateNonMovableOrHugeObject(JSThread *thread, JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateNonMovableOrHugeObject(thread, hclass, size);
}

TaggedObject *SharedHeap::AllocateNonMovableOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, hclass, size);
    }
    TaggedObject *object =
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedNonMovableSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = reinterpret_cast<TaggedObject *>(sNonMovableSpace_->Allocate(thread, size));
    }
    CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sNonMovableSpace_,
        "SharedHeap::AllocateNonMovableOrHugeObject");
    object->SetClass(thread, hclass);
    TryTriggerConcurrentMarking(thread);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *SharedHeap::AllocateNonMovableOrHugeObject(JSThread *thread, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, size);
    }
    TaggedObject *object =
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedNonMovableSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = reinterpret_cast<TaggedObject *>(sNonMovableSpace_->Allocate(thread, size));
    }
    CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sNonMovableSpace_,
        "SharedHeap::AllocateNonMovableOrHugeObject");
    TryTriggerConcurrentMarking(thread);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *SharedHeap::AllocateOldOrHugeObject(JSThread *thread, JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateOldOrHugeObject(thread, hclass, size);
}

TaggedObject *SharedHeap::AllocateOldOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, hclass, size);
    }
    TaggedObject *object = thread->IsJitThread() ? nullptr :
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedOldSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = reinterpret_cast<TaggedObject *>(sOldSpace_->Allocate(thread, size));
    }
    CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sOldSpace_, "SharedHeap::AllocateOldOrHugeObject");
    object->SetClass(thread, hclass);
    TryTriggerConcurrentMarking(thread);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *SharedHeap::AllocateOldOrHugeObject(JSThread *thread, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, size);
    }
    TaggedObject *object = thread->IsJitThread() ? nullptr :
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedOldSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = reinterpret_cast<TaggedObject *>(sOldSpace_->Allocate(thread, size));
    }
    CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sOldSpace_, "SharedHeap::AllocateOldOrHugeObject");
    TryTriggerConcurrentMarking(thread);
    return object;
}

TaggedObject *SharedHeap::AllocateHugeObject(JSThread *thread, JSHClass *hclass, size_t size)
{
    auto object = AllocateHugeObject(thread, size);
    object->SetClass(thread, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject *SharedHeap::AllocateHugeObject(JSThread *thread, size_t size)
{
    // Check whether it is necessary to trigger Shared GC before expanding to avoid OOM risk.
    CheckHugeAndTriggerSharedGC(thread, size);
    auto *object = reinterpret_cast<TaggedObject *>(sHugeObjectSpace_->Allocate(thread, size));
    if (UNLIKELY(object == nullptr)) {
        CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_LIMIT>(thread);
        object = reinterpret_cast<TaggedObject *>(sHugeObjectSpace_->Allocate(thread, size));
        if (UNLIKELY(object == nullptr)) {
            // if allocate huge object OOM, temporarily increase space size to avoid vm crash
            size_t oomOvershootSize = config_.GetOutOfMemoryOvershootSize();
            sHugeObjectSpace_->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);
            DumpHeapSnapshotBeforeOOM(true, thread);
            ThrowOutOfMemoryError(thread, size, "SharedHeap::AllocateHugeObject");
            object = reinterpret_cast<TaggedObject *>(sHugeObjectSpace_->Allocate(thread, size));
            if (UNLIKELY(object == nullptr)) {
                FatalOutOfMemoryError(size, "SharedHeap::AllocateHugeObject");
            }
        }
    }
    TryTriggerConcurrentMarking(thread);
    return object;
}

TaggedObject *SharedHeap::AllocateReadOnlyOrHugeObject(JSThread *thread, JSHClass *hclass)
{
    size_t size = hclass->GetObjectSize();
    return AllocateReadOnlyOrHugeObject(thread, hclass, size);
}

TaggedObject *SharedHeap::AllocateReadOnlyOrHugeObject(JSThread *thread, JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, hclass, size);
    }
    auto object = reinterpret_cast<TaggedObject *>(sReadOnlySpace_->Allocate(thread, size));
    CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sReadOnlySpace_, "SharedHeap::AllocateReadOnlyOrHugeObject");
    object->SetClass(thread, hclass);
    return object;
}

TaggedObject *SharedHeap::AllocateSOldTlab(JSThread *thread, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    TaggedObject *object = nullptr;
    if (sOldSpace_->GetCommittedSize() > sOldSpace_->GetInitialCapacity() / 2) { // 2: half
        object = reinterpret_cast<TaggedObject *>(sOldSpace_->AllocateNoGCAndExpand(thread, size));
    } else {
        object = reinterpret_cast<TaggedObject *>(sOldSpace_->Allocate(thread, size));
    }
    return object;
}

TaggedObject *SharedHeap::AllocateSNonMovableTlab(JSThread *thread, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    TaggedObject *object = nullptr;
    object = reinterpret_cast<TaggedObject *>(sNonMovableSpace_->Allocate(thread, size));
    return object;
}

template<TriggerGCType gcType, GCReason gcReason>
void SharedHeap::TriggerConcurrentMarking(JSThread *thread)
{
    ASSERT(gcType == TriggerGCType::SHARED_GC);
    // lock is outside to prevent extreme case, maybe could move update gcFinished_ into CheckAndPostTask
    // instead of an outside locking.
    LockHolder lock(waitGCFinishedMutex_);
    if (dThread_->CheckAndPostTask(TriggerConcurrentMarkTask<gcType, gcReason>(thread))) {
        ASSERT(gcFinished_);
        gcFinished_ = false;
    }
}

template<TriggerGCType gcType, GCReason gcReason>
void SharedHeap::CollectGarbage(JSThread *thread)
{
    ASSERT(gcType == TriggerGCType::SHARED_GC);
#ifndef NDEBUG
    ASSERT(!thread->HasLaunchedSuspendAll());
#endif
    if (UNLIKELY(!dThread_->IsRunning())) {
        // Hope this will not happen, unless the AppSpawn run smth after PostFork
        LOG_GC(ERROR) << "Try to collect garbage in shared heap, but daemon thread is not running.";
        ForceCollectGarbageWithoutDaemonThread(gcType, gcReason, thread);
        return;
    }
    {
        // lock here is outside post task to prevent the extreme case: another js thread succeeed posting a
        // concurrentmark task, so here will directly go into WaitGCFinished, but gcFinished_ is somehow
        // not set by that js thread before the WaitGCFinished done, and maybe cause an unexpected OOM
        LockHolder lock(waitGCFinishedMutex_);
        if (dThread_->CheckAndPostTask(TriggerCollectGarbageTask<gcType, gcReason>(thread))) {
            ASSERT(gcFinished_);
            gcFinished_ = false;
        }
    }
    ASSERT(!gcFinished_);
    SetForceGC(true);
    WaitGCFinished(thread);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_INL_H
