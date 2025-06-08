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

#include "clang.h"
#ifdef USE_CMC_GC
#include "common_interfaces/heap/heap_allocator.h"
#endif
#include "ecmascript/mem/heap.h"

#include "ecmascript/base/block_hook_scope.h"
#include "ecmascript/daemon/daemon_task-inl.h"
#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/linear_space.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/shared_mem_controller.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/thread_local_allocation_buffer.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/runtime.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
#define CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, space, message)                                         \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        EcmaVM *vm = GetEcmaVM();                                                                           \
        size_t oomOvershootSize = vm->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();            \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        if ((space)->IsOOMDumpSpace()) {                                                                    \
            DumpHeapSnapshotBeforeOOM(false);                                                               \
        }                                                                                                   \
        StatisticHeapDetail();                                                                              \
        ThrowOutOfMemoryError(GetJSThread(), size, message);                                                \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(size));                               \
    }

#define CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, space, message)                                \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        size_t oomOvershootSize = GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();                \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        DumpHeapSnapshotBeforeOOM(false, thread, SharedHeapOOMSource::NORMAL_ALLOCATION);                   \
        ThrowOutOfMemoryError(thread, size, message);                                                       \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(thread, size));                       \
    }

#define CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR_FORT(object, size, space, desc, message)                   \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        EcmaVM *vm = GetEcmaVM();                                                                           \
        size_t oomOvershootSize = vm->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();            \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        SetMachineCodeOutOfMemoryError(GetJSThread(), size, message);                                       \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(size, desc));                         \
    }

#define CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR(object, size, space, message)                              \
    if (UNLIKELY((object) == nullptr)) {                                                                    \
        EcmaVM *vm = GetEcmaVM();                                                                           \
        size_t oomOvershootSize = vm->GetEcmaParamConfiguration().GetOutOfMemoryOvershootSize();            \
        (space)->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);                                        \
        SetMachineCodeOutOfMemoryError(GetJSThread(), size, message);                                       \
        (object) = reinterpret_cast<TaggedObject *>((space)->Allocate(size));                               \
    }

template<class Callback>
void SharedHeap::EnumerateOldSpaceRegions(const Callback &cb) const
{
    sOldSpace_->EnumerateRegions(cb);
    sNonMovableSpace_->EnumerateRegions(cb);
    sHugeObjectSpace_->EnumerateRegions(cb);
    sAppSpawnSpace_->EnumerateRegions(cb);
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
    sAppSpawnSpace_->IterateOverMarkedObjects(cb);
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
    if (!isCSetClearing_.load(std::memory_order_acquire)) {
        oldSpace_->EnumerateCollectRegionSet(cb);
    }
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
    activeSemiSpace_->EnumerateRegions(cb);
    oldSpace_->EnumerateRegions(cb);
    if (!isCSetClearing_.load(std::memory_order_acquire)) {
        oldSpace_->EnumerateCollectRegionSet(cb);
    }
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
#ifdef USE_CMC_GC
    return reinterpret_cast<TaggedObject *>(HeapAllocator::Allocate(size, LanguageType::DYNAMIC));
#else
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = nullptr;
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = AllocateHugeObject(size);
    } else {
        object = AllocateInYoungSpace(size);
        if (object == nullptr) {
            if (!HandleExitHighSensitiveEvent()) {
                CollectGarbage(SelectGCType(), GCReason::ALLOCATION_FAILED);
            }
            object = AllocateInYoungSpace(size);
            if (object == nullptr) {
                CollectGarbage(SelectGCType(), GCReason::ALLOCATION_FAILED);
                object = AllocateInYoungSpace(size);
                CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, activeSemiSpace_, "Heap::AllocateYoungOrHugeObject");
            }
        }
    }
    return object;
#endif
}

TaggedObject *Heap::AllocateInYoungSpace(size_t size)
{
#ifdef USE_CMC_GC
    return reinterpret_cast<TaggedObject *>(HeapAllocator::Allocate(size, LanguageType::DYNAMIC));
#else
    return reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
#endif
}

TaggedObject *Heap::AllocateYoungOrHugeObject(JSHClass *hclass, size_t size)
{
    auto object = AllocateYoungOrHugeObject(size);
    ASSERT(object != nullptr);
    object->SetClass(thread_, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

void BaseHeap::SetHClassAndDoAllocateEvent(JSThread *thread, TaggedObject *object, JSHClass *hclass,
                                           [[maybe_unused]] size_t size)
{
    ASSERT(object != nullptr);
    object->SetClass(thread, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
}

uintptr_t Heap::AllocateYoungSync(size_t size)
{
#ifdef USE_CMC_GC
    // check sync
    return HeapAllocator::Allocate(size, LanguageType::DYNAMIC);
#else
    return activeSemiSpace_->AllocateSync(size);
#endif
}

bool Heap::MoveYoungRegion(Region *region)
{
    return activeSemiSpace_->SwapRegion(region, inactiveSemiSpace_);
}

bool Heap::MoveYoungRegionToOld(Region *region)
{
    return oldSpace_->SwapRegion(region, inactiveSemiSpace_);
}

void Heap::MergeToOldSpaceSync(LocalSpace *localSpace)
{
    oldSpace_->Merge(localSpace);
}

bool Heap::InHeapProfiler()
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    return GetEcmaVM()->GetHeapProfile() != nullptr;
#else
    return false;
#endif
}

void SharedHeap::MergeToOldSpaceSync(SharedLocalSpace *localSpace)
{
    sOldSpace_->Merge(localSpace);
}

TaggedObject *Heap::TryAllocateYoungGeneration(JSHClass *hclass, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
#ifdef USE_CMC_GC
    auto object = reinterpret_cast<TaggedObject*>(HeapAllocator::Allocate(size, LanguageType::DYNAMIC));
#else
    auto object = reinterpret_cast<TaggedObject *>(activeSemiSpace_->Allocate(size));
#endif
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
    return AllocateOldOrHugeObject(hclass, size);
}

TaggedObject *Heap::AllocateOldOrHugeObject(size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    TaggedObject *object = nullptr;

#ifdef USE_CMC_GC
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInHuge(size, LanguageType::DYNAMIC));
    } else {
        object = reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInOld(size, LanguageType::DYNAMIC));
    }
#else
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        object = AllocateHugeObject(size);
    } else {
        object = reinterpret_cast<TaggedObject *>(oldSpace_->AllocateFast(size));
        if (object == nullptr) {
            bool gcSuccess = CheckAndTriggerOldGC();
            object = reinterpret_cast<TaggedObject *>(oldSpace_->AllocateSlow(size, gcSuccess));
        }
        if (object == nullptr) {
            CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_FAILED);
            object = reinterpret_cast<TaggedObject *>(oldSpace_->AllocateSlow(size, true));
        }
        CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, oldSpace_, "Heap::AllocateOldOrHugeObject");
    }
#endif
    return object;
}

TaggedObject *Heap::AllocateOldOrHugeObject(JSHClass *hclass, size_t size)
{
    auto object = AllocateOldOrHugeObject(size);
    object->SetClass(thread_, hclass);
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
        object = AllocateReadOnlyOrHugeObject(size);
        CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, readOnlySpace_, "Heap::AllocateReadOnlyOrHugeObject");
        ASSERT(object != nullptr);
        object->SetClass(thread_, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

TaggedObject* Heap::AllocateReadOnlyOrHugeObject(size_t size)
{
#ifdef USE_CMC_GC
    return reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInReadOnly(size, LanguageType::DYNAMIC));
#else
    return reinterpret_cast<TaggedObject *>(readOnlySpace_->Allocate(size));
#endif
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
#ifdef USE_CMC_GC
        object = reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
        object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->CheckAndAllocate(size));
        CHECK_OBJ_AND_THROW_OOM_ERROR(object, size, nonMovableSpace_, "Heap::AllocateNonMovableOrHugeObject");
#endif
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
#ifdef USE_CMC_GC
    auto object = reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
    auto object = reinterpret_cast<TaggedObject *>(nonMovableSpace_->Allocate(size));
#endif
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
#ifdef USE_CMC_GC
    // check why shareheap allocate in readonly
    auto object = reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
    auto object = reinterpret_cast<TaggedObject *>(sReadOnlySpace_->Allocate(thread, size));
#endif
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
#ifdef USE_CMC_GC
    return reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInHuge(size, LanguageType::DYNAMIC));
#else
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
            DumpHeapSnapshotBeforeOOM(false);
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
#endif
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

TaggedObject *Heap::AllocateHugeMachineCodeObject(size_t size, MachineCodeDesc *desc)
{
#ifdef USE_CMC_GC
    ASSERT(desc != nullptr && "in CMCGC, this path is always jitfort.");
#endif
    TaggedObject *object;
    if (desc) {
        object = reinterpret_cast<TaggedObject *>(hugeMachineCodeSpace_->Allocate(
            size, thread_, reinterpret_cast<void *>(desc)));
    } else {
        object = reinterpret_cast<TaggedObject *>(hugeMachineCodeSpace_->Allocate(
            size, thread_));
    }
    return object;
}

TaggedObject *Heap::AllocateMachineCodeObject(JSHClass *hclass, size_t size, MachineCodeDesc *desc)
{
    TaggedObject *object;
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (!desc) {
        // Jit Fort disabled
        ASSERT(!GetEcmaVM()->GetJSOptions().GetEnableJitFort());
        object = (size > MAX_REGULAR_HEAP_OBJECT_SIZE) ?
#ifdef USE_CMC_GC
            reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInHuge(size, LanguageType::DYNAMIC)) :
            reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
            reinterpret_cast<TaggedObject *>(AllocateHugeMachineCodeObject(size)) :
            reinterpret_cast<TaggedObject *>(machineCodeSpace_->Allocate(size));
#endif
        CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR(object, size, machineCodeSpace_,
            "Heap::AllocateMachineCodeObject");
        object->SetClass(thread_, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
        OnAllocateEvent(GetEcmaVM(), object, size);
#endif
        return object;
    }

    // Jit Fort enabled
    ASSERT(GetEcmaVM()->GetJSOptions().GetEnableJitFort());
    if (!GetEcmaVM()->GetJSOptions().GetEnableAsyncCopyToFort()) {
        desc->instructionsAddr = 0;
        if (size <= MAX_REGULAR_HEAP_OBJECT_SIZE) {
            // for non huge code cache obj, allocate fort space before allocating the code object
            uintptr_t mem = machineCodeSpace_->JitFortAllocate(desc);
            if (mem == ToUintPtr(nullptr)) {
                return nullptr;
            }
            desc->instructionsAddr = mem;
        }
    }
    object = (size > MAX_REGULAR_HEAP_OBJECT_SIZE) ?
#ifdef USE_CMC_GC
        reinterpret_cast<TaggedObject *>(AllocateHugeMachineCodeObject(size, desc)) :
        reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
        reinterpret_cast<TaggedObject *>(AllocateHugeMachineCodeObject(size, desc)) :
        reinterpret_cast<TaggedObject *>(machineCodeSpace_->Allocate(size, desc, true));
#endif
    CHECK_MACHINE_CODE_OBJ_AND_SET_OOM_ERROR_FORT(object, size, machineCodeSpace_, desc,
        "Heap::AllocateMachineCodeObject");
    object->SetClass(thread_, hclass);
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(GetEcmaVM(), object, size);
#endif
    return object;
}

uintptr_t Heap::AllocateSnapshotSpace(size_t size)
{
    ASSERT(false);
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
#ifdef USE_CMC_GC
    return reinterpret_cast<TaggedObject*>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#endif

    ASSERT(!thread->IsJitThread());
    if (GetEcmaVM()->GetThreadCheckStatus()) {
        if (thread->IsJitThread()) {
            LOG_ECMA(FATAL) << "jit thread not allowed";
        }
        if (thread->CheckMultiThread()) {
            LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                            << "thread:" << thread->GetThreadId()
                            << " currentThread:" << JSThread::GetCurrentThreadId();
        }
    }
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
    object = sHeap_->AllocateSNonMovableTlab(thread, newTlabSize);
    if (object == nullptr) {
        sNonMovableTlab_->DisableNewTlab();
        return nullptr;
    }
    uintptr_t begin = reinterpret_cast<uintptr_t>(object);
    sNonMovableTlab_->Reset(begin, begin + newTlabSize, begin + size);
    auto topAddress = sNonMovableTlab_->GetTopAddress();
    auto endAddress = sNonMovableTlab_->GetEndAddress();
    thread->ReSetSNonMovableSpaceAllocationAddress(topAddress, endAddress);
    sHeap_->TryTriggerConcurrentMarking(thread);
    return object;
}

TaggedObject *Heap::AllocateSharedOldSpaceFromTlab(JSThread *thread, size_t size)
{
#ifdef USE_CMC_GC
    // will invoked by asm interpreter stub AllocateInSOld
    return reinterpret_cast<TaggedObject*>(HeapAllocator::AllocateInOld(size, LanguageType::DYNAMIC));
#endif

    ASSERT(!thread->IsJitThread());
    if (GetEcmaVM()->GetThreadCheckStatus()) {
        if (thread->IsJitThread()) {
            LOG_ECMA(FATAL) << "jit thread not allowed";
        }
        if (thread->CheckMultiThread()) {
            LOG_FULL(FATAL) << "Fatal: ecma_vm cannot run in multi-thread!"
                            << "thread:" << thread->GetThreadId()
                            << " currentThread:" << JSThread::GetCurrentThreadId();
        }
    }
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
    object = sHeap_->AllocateSOldTlab(thread, newTlabSize);
    if (object == nullptr) {
        sOldTlab_->DisableNewTlab();
        return nullptr;
    }
    uintptr_t begin = reinterpret_cast<uintptr_t>(object);
    sOldTlab_->Reset(begin, begin + newTlabSize, begin + size);
    auto topAddress = sOldTlab_->GetTopAddress();
    auto endAddress = sOldTlab_->GetEndAddress();
    thread->ReSetSOldSpaceAllocationAddress(topAddress, endAddress);
    sHeap_->TryTriggerConcurrentMarking(thread);
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

void Heap::OnMoveEvent([[maybe_unused]] uintptr_t address, [[maybe_unused]] TaggedObject* forwardAddress,
                       [[maybe_unused]] size_t size)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    HeapProfilerInterface *profiler = GetEcmaVM()->GetHeapProfile();
    if (profiler != nullptr) {
        base::BlockHookScope blockScope;
        profiler->MoveEvent(address, forwardAddress, size);
    }
#endif
}

void SharedHeap::OnMoveEvent([[maybe_unused]] uintptr_t address, [[maybe_unused]] TaggedObject* forwardAddress,
                             [[maybe_unused]] size_t size)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    Runtime::GetInstance()->GCIterateThreadListWithoutLock([&](JSThread *thread) {
        HeapProfilerInterface *profiler = thread->GetEcmaVM()->GetHeapProfile();
        if (profiler != nullptr) {
            base::BlockHookScope blockScope;
            profiler->MoveEvent(address, forwardAddress, size);
        }
    });
#endif
}

void SharedHeap::SwapOldSpace()
{
    sCompressSpace_->SetInitialCapacity(sOldSpace_->GetInitialCapacity());
    auto *oldSpace = sCompressSpace_;
    sCompressSpace_ = sOldSpace_;
    sOldSpace_ = oldSpace;
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    sOldSpace_->SwapAllocationCounter(sCompressSpace_);
#endif
}

void Heap::ReclaimRegions(TriggerGCType gcType)
{
    activeSemiSpace_->EnumerateRegionsWithRecord([] (Region *region) {
        region->ResetRegionTypeFlag();
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
        region->ResetAliveObject();
        region->ClearGCFlag(RegionGCFlags::IN_NEW_TO_NEW_SET);
    });
    size_t cachedSize = inactiveSemiSpace_->GetInitialCapacity();
    if (gcType == TriggerGCType::FULL_GC) {
        compressSpace_->Reset();
        cachedSize = 0;
    } else if (gcType == TriggerGCType::OLD_GC) {
        oldSpace_->ReclaimCSet();
        isCSetClearing_.store(false, std::memory_order_release);
    }

    inactiveSemiSpace_->ReclaimRegions(cachedSize);
    sweeper_->WaitAllTaskFinished();
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
#ifdef USE_CMC_GC
    return;
#endif

    if (!current->InYoungSpace()) {
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
    size_t result = activeSemiSpace_->GetCommittedSize() +
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
    size_t result = activeSemiSpace_->GetHeapObjectSize() +
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

void Heap::NotifyRecordMemorySize()
{
    if (GetRecordObjectSize() == 0) {
        RecordOrResetObjectSize(GetHeapObjectSize());
    }
    if (GetRecordNativeSize() == 0) {
        RecordOrResetNativeSize(GetNativeBindingSize());
    }
}

size_t Heap::GetRegionCount() const
{
    size_t result = activeSemiSpace_->GetRegionCount() +
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
#ifdef USE_CMC_GC
    return;
#endif
    if (!CheckCanTriggerConcurrentMarking(thread)) {
        return;
    }
    bool triggerConcurrentMark = (GetHeapObjectSize() >= globalSpaceConcurrentMarkLimit_);
    if (triggerConcurrentMark && (OnStartupEvent() || IsJustFinishStartup())) {
        triggerConcurrentMark = ObjectExceedJustFinishStartupThresholdForCM();
    }
    if (triggerConcurrentMark) {
        // currently, SharedHeap::TryTriggerConcurrentMarking is called only when allocate object in SharedHeap
        TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::ALLOCATION_LIMIT>(thread);
    }
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

#ifdef USE_CMC_GC
    TaggedObject *object = thread->IsJitThread() ? nullptr
                                                 : reinterpret_cast<TaggedObject *>(
                                                       HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
    object->SetClass(thread, hclass);
#else
    TaggedObject *object = thread->IsJitThread() ? nullptr :
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedNonMovableSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = reinterpret_cast<TaggedObject *>(sNonMovableSpace_->Allocate(thread, size));
        CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sNonMovableSpace_,
            "SharedHeap::AllocateNonMovableOrHugeObject");
        object->SetClass(thread, hclass);
        TryTriggerConcurrentMarking(thread);
    } else {
        object->SetClass(thread, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
#endif // USE_CMC_GC
    return object;
}

TaggedObject *SharedHeap::AllocateNonMovableOrHugeObject(JSThread *thread, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, size);
    }

#ifdef USE_CMC_GC
    TaggedObject *object = thread->IsJitThread() ? nullptr
                                                 : reinterpret_cast<TaggedObject *>(
                                                       HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
    TaggedObject *object = thread->IsJitThread() ? nullptr :
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedNonMovableSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = reinterpret_cast<TaggedObject *>(sNonMovableSpace_->Allocate(thread, size));
        CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sNonMovableSpace_,
            "SharedHeap::AllocateNonMovableOrHugeObject");
        TryTriggerConcurrentMarking(thread);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
#endif //USE_CMC_GC
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

#ifdef USE_CMC_GC
    TaggedObject *object = reinterpret_cast<TaggedObject*>(HeapAllocator::AllocateInOld(size, LanguageType::DYNAMIC));
    object->SetClass(thread, hclass);
#else
    TaggedObject *object = thread->IsJitThread() ? nullptr :
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedOldSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = AllocateInSOldSpace(thread, size);
        CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sOldSpace_, "SharedHeap::AllocateOldOrHugeObject");
        object->SetClass(thread, hclass);
        TryTriggerConcurrentMarking(thread);
    } else {
        object->SetClass(thread, hclass);
    }
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    OnAllocateEvent(thread->GetEcmaVM(), object, size);
#endif
#endif // USE_CMC_GC
    return object;
}

TaggedObject *SharedHeap::AllocateOldOrHugeObject(JSThread *thread, size_t size)
{
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return AllocateHugeObject(thread, size);
    }

#ifdef USE_CMC_GC
    TaggedObject *object = thread->IsJitThread() ? nullptr :
                           reinterpret_cast<TaggedObject*>(HeapAllocator::AllocateInOld(size, LanguageType::DYNAMIC));
#else
    TaggedObject *object = thread->IsJitThread() ? nullptr :
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->AllocateSharedOldSpaceFromTlab(thread, size);
    if (object == nullptr) {
        object = AllocateInSOldSpace(thread, size);
        CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sOldSpace_, "SharedHeap::AllocateOldOrHugeObject");
        TryTriggerConcurrentMarking(thread);
    }
#endif
    return object;
}

TaggedObject *SharedHeap::AllocateInSOldSpace(JSThread *thread, size_t size)
{
#ifdef USE_CMC_GC
    (void)thread;
    ASSERT(!thread->IsJitThread());
    return reinterpret_cast<TaggedObject*>(HeapAllocator::AllocateInOld(size, LanguageType::DYNAMIC));
#else
    // jit thread no heap
    bool allowGC = !thread->IsJitThread();
    if (allowGC) {
        auto localHeap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        localHeap->TryTriggerFullMarkBySharedSize(size);
    }
    TaggedObject *object = reinterpret_cast<TaggedObject *>(sOldSpace_->TryAllocateAndExpand(thread, size, false));
     // Check whether it is necessary to trigger Shared GC before expanding to avoid OOM risk.
    if (object == nullptr) {
        if (allowGC) {
            CheckAndTriggerSharedGC(thread);
        }
        object = reinterpret_cast<TaggedObject *>(sOldSpace_->TryAllocateAndExpand(thread, size, true));
        if (object == nullptr) {
            if (allowGC) {
                CollectGarbageNearOOM(thread);
            }
            object = reinterpret_cast<TaggedObject *>(sOldSpace_->TryAllocateAndExpand(thread, size, true));
        }
    }
    return object;
#endif
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
#ifdef USE_CMC_GC
    (void)thread;
    return reinterpret_cast<TaggedObject*>(HeapAllocator::AllocateInHuge(size, LanguageType::DYNAMIC));
#else
    // Check whether it is necessary to trigger Shared GC before expanding to avoid OOM risk.
    CheckHugeAndTriggerSharedGC(thread, size);
    auto *object = reinterpret_cast<TaggedObject *>(sHugeObjectSpace_->Allocate(thread, size));
    if (UNLIKELY(object == nullptr)) {
        CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED>(thread);
        object = reinterpret_cast<TaggedObject *>(sHugeObjectSpace_->Allocate(thread, size));
        if (UNLIKELY(object == nullptr)) {
            // if allocate huge object OOM, temporarily increase space size to avoid vm crash
            size_t oomOvershootSize = config_.GetOutOfMemoryOvershootSize();
            sHugeObjectSpace_->IncreaseOutOfMemoryOvershootSize(oomOvershootSize);
            DumpHeapSnapshotBeforeOOM(false, thread, SharedHeapOOMSource::NORMAL_ALLOCATION);
            ThrowOutOfMemoryError(thread, size, "SharedHeap::AllocateHugeObject");
            object = reinterpret_cast<TaggedObject *>(sHugeObjectSpace_->Allocate(thread, size));
            if (UNLIKELY(object == nullptr)) {
                FatalOutOfMemoryError(size, "SharedHeap::AllocateHugeObject");
            }
        }
    }
    TryTriggerConcurrentMarking(thread);
    return object;
#endif
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

#ifdef USE_CMC_GC
    auto object = reinterpret_cast<TaggedObject *>(HeapAllocator::AllocateInNonmove(size, LanguageType::DYNAMIC));
#else
    auto object = reinterpret_cast<TaggedObject *>(sReadOnlySpace_->Allocate(thread, size));
    CHECK_SOBJ_AND_THROW_OOM_ERROR(thread, object, size, sReadOnlySpace_, "SharedHeap::AllocateReadOnlyOrHugeObject");
#endif
    ASSERT(object != nullptr);
    object->SetClass(thread, hclass);
    return object;
}

TaggedObject *SharedHeap::AllocateSOldTlab(JSThread *thread, size_t size)
{
#ifdef USE_CMC_GC
    ASSERT(false);
#endif

    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    TaggedObject *object = nullptr;
    if (sOldSpace_->GetCommittedSize() > sOldSpace_->GetInitialCapacity() / 2) { // 2: half
        object = reinterpret_cast<TaggedObject *>(sOldSpace_->AllocateNoGCAndExpand(thread, size));
    } else {
        object = AllocateInSOldSpace(thread, size);
    }
    return object;
}

TaggedObject *SharedHeap::AllocateSNonMovableTlab(JSThread *thread, size_t size)
{
#ifdef USE_CMC_GC
    ASSERT(false);
#endif

    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        return nullptr;
    }
    TaggedObject *object = nullptr;
    object = reinterpret_cast<TaggedObject *>(sNonMovableSpace_->Allocate(thread, size));
    return object;
}

template<TriggerGCType gcType, MarkReason markReason>
void SharedHeap::TriggerConcurrentMarking(JSThread *thread)
{
    ASSERT(gcType == TriggerGCType::SHARED_GC || gcType == TriggerGCType::SHARED_PARTIAL_GC);
    // lock is outside to prevent extreme case, maybe could move update gcFinished_ into CheckAndPostTask
    // instead of an outside locking.
    LockHolder lock(waitGCFinishedMutex_);
    if (dThread_->CheckAndPostTask(TriggerConcurrentMarkTask<gcType, markReason>(thread))) {
        ASSERT(gcFinished_);
        gcFinished_ = false;
    }
}

template<TriggerGCType gcType, GCReason gcReason>
void SharedHeap::CollectGarbage(JSThread *thread)
{
#ifdef USE_CMC_GC
    GcType type = GcType::ASYNC;
    if constexpr (gcType == TriggerGCType::FULL_GC || gcType == TriggerGCType::SHARED_FULL_GC ||
        gcType == TriggerGCType::APPSPAWN_FULL_GC || gcType == TriggerGCType::APPSPAWN_SHARED_FULL_GC ||
        gcReason == GCReason::ALLOCATION_FAILED) {
        type = GcType::FULL;
    }
    BaseRuntime::RequestGC(type);
    return;
#endif
    ASSERT(gcType == TriggerGCType::SHARED_GC || gcType == TriggerGCType::SHARED_PARTIAL_GC ||
        gcType == TriggerGCType::SHARED_FULL_GC);
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

// This method is used only in the idle state and background switchover state.
template<GCReason gcReason>
void SharedHeap::CompressCollectGarbageNotWaiting(JSThread *thread)
{
    {
        // lock here is outside post task to prevent the extreme case: another js thread succeeed posting a
        // concurrentmark task, so here will directly go into WaitGCFinished, but gcFinished_ is somehow
        // not set by that js thread before the WaitGCFinished done, and maybe cause an unexpected OOM
        LockHolder lock(waitGCFinishedMutex_);
        if (dThread_->CheckAndPostTask(TriggerCollectGarbageTask<TriggerGCType::SHARED_FULL_GC, gcReason>(thread))) {
            ASSERT(gcFinished_);
            gcFinished_ = false;
        }
    }
    ASSERT(!gcFinished_);
    SetForceGC(true);
}

template<TriggerGCType gcType, GCReason gcReason>
void SharedHeap::PostGCTaskForTest(JSThread *thread)
{
    ASSERT(gcType == TriggerGCType::SHARED_GC ||gcType == TriggerGCType::SHARED_PARTIAL_GC ||
        gcType == TriggerGCType::SHARED_FULL_GC);
#ifndef NDEBUG
    ASSERT(!thread->HasLaunchedSuspendAll());
#endif
    if (dThread_->IsRunning()) {
        // Some UT may run without Daemon Thread.
        LockHolder lock(waitGCFinishedMutex_);
        if (dThread_->CheckAndPostTask(TriggerCollectGarbageTask<gcType, gcReason>(thread))) {
            ASSERT(gcFinished_);
            gcFinished_ = false;
        }
        ASSERT(!gcFinished_);
    }
}

template<TriggerGCType gcType, GCReason gcReason>
bool SharedHeap::TriggerUnifiedGCMark(JSThread *thread) const
{
    ASSERT(gcType == TriggerGCType::UNIFIED_GC && gcReason == GCReason::CROSSREF_CAUSE);
    return DaemonThread::GetInstance()->CheckAndPostTask(TriggerUnifiedGCMarkTask<gcType, gcReason>(thread));
}

static void SwapBackAndPop(CVector<JSNativePointer*>& vec, CVector<JSNativePointer*>::iterator& iter)
{
    *iter = vec.back();
    if (iter + 1 == vec.end()) {
        vec.pop_back();
        iter = vec.end();
    } else {
        vec.pop_back();
    }
}

static void ShrinkWithFactor(CVector<JSNativePointer*>& vec)
{
    constexpr size_t SHRINK_FACTOR = 2;
    if (vec.size() < vec.capacity() / SHRINK_FACTOR) {
        vec.shrink_to_fit();
    }
}

void SharedHeap::InvokeSharedNativePointerCallbacks()
{
    Runtime *runtime = Runtime::GetInstance();
    if (!runtime->GetSharedNativePointerCallbacks().empty()) {
        runtime->InvokeSharedNativePointerCallbacks();
    }
}

void SharedHeap::PushToSharedNativePointerList(JSNativePointer* pointer)
{
#ifndef USE_CMC_GC
    ASSERT(JSTaggedValue(pointer).IsInSharedHeap());
#endif
    std::lock_guard<std::mutex> lock(sNativePointerListMutex_);
    sharedNativePointerList_.emplace_back(pointer);
}

#ifdef USE_CMC_GC
void SharedHeap::IteratorNativePointerList(WeakVisitor &visitor)
{
    auto& sharedNativePointerCallbacks = Runtime::GetInstance()->GetSharedNativePointerCallbacks();
    auto sharedIter = sharedNativePointerList_.begin();
    while (sharedIter != sharedNativePointerList_.end()) {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&(*sharedIter)));
        bool isAlive = visitor.VisitRoot(Root::ROOT_VM, slot);
        if (!isAlive) {
            JSNativePointer* object = *sharedIter;
            sharedNativePointerCallbacks.emplace_back(
                object->GetDeleter(), std::make_pair(object->GetExternalPointer(), object->GetData()));
            SwapBackAndPop(sharedNativePointerList_, sharedIter);
        } else {
            ++sharedIter;
        }
    }
    ShrinkWithFactor(sharedNativePointerList_);
}
#endif

void SharedHeap::ProcessSharedNativeDelete(const WeakRootVisitor& visitor)
{
#ifndef NDEBUG
    ASSERT(JSThread::GetCurrent()->HasLaunchedSuspendAll());
#endif
    auto& sharedNativePointerCallbacks = Runtime::GetInstance()->GetSharedNativePointerCallbacks();
    auto sharedIter = sharedNativePointerList_.begin();
    while (sharedIter != sharedNativePointerList_.end()) {
        JSNativePointer* object = *sharedIter;
        auto fwd = visitor(reinterpret_cast<TaggedObject*>(object));
        if (fwd == nullptr) {
            sharedNativePointerCallbacks.emplace_back(
                object->GetDeleter(), std::make_pair(object->GetExternalPointer(), object->GetData()));
            SwapBackAndPop(sharedNativePointerList_, sharedIter);
        } else {
            if (fwd != reinterpret_cast<TaggedObject*>(object)) {
                *sharedIter = reinterpret_cast<JSNativePointer*>(fwd);
            }
            ++sharedIter;
        }
    }
    ShrinkWithFactor(sharedNativePointerList_);
}

void Heap::ProcessNativeDelete(const WeakRootVisitor& visitor)
{
    // ProcessNativeDelete should be limited to OldGC or FullGC only
    if (!IsYoungGC()) {
        auto& asyncNativeCallbacksPack = GetEcmaVM()->GetAsyncNativePointerCallbacksPack();
        auto iter = nativePointerList_.begin();
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_MAX, HITRACE_TAG_ARK,
            ("ProcessNativeDeleteNum:" + std::to_string(nativePointerList_.size())).c_str(), "");
        while (iter != nativePointerList_.end()) {
            JSNativePointer* object = *iter;
            auto fwd = visitor(reinterpret_cast<TaggedObject*>(object));
            if (fwd == nullptr) {
                size_t bindingSize = object->GetBindingSize();
                asyncNativeCallbacksPack.AddCallback(std::make_pair(object->GetDeleter(),
                    std::make_tuple(thread_->GetEnv(), object->GetExternalPointer(), object->GetData())), bindingSize);
                nativeAreaAllocator_->DecreaseNativeSizeStats(bindingSize, object->GetNativeFlag());
                SwapBackAndPop(nativePointerList_, iter);
            } else {
                ++iter;
            }
        }
        ShrinkWithFactor(nativePointerList_);

        auto& concurrentNativeCallbacks = GetEcmaVM()->GetConcurrentNativePointerCallbacks();
        auto newIter = concurrentNativePointerList_.begin();
        while (newIter != concurrentNativePointerList_.end()) {
            JSNativePointer* object = *newIter;
            auto fwd = visitor(reinterpret_cast<TaggedObject*>(object));
            if (fwd == nullptr) {
                nativeAreaAllocator_->DecreaseNativeSizeStats(object->GetBindingSize(), object->GetNativeFlag());
                concurrentNativeCallbacks.emplace_back(object->GetDeleter(),
                    std::make_tuple(thread_->GetEnv(), object->GetExternalPointer(), object->GetData()));
                SwapBackAndPop(concurrentNativePointerList_, newIter);
            } else {
                ++newIter;
            }
        }
        ShrinkWithFactor(concurrentNativePointerList_);
    }
}

void Heap::ProcessReferences(const WeakRootVisitor& visitor)
{
    // process native ref should be limited to OldGC or FullGC only
    if (!IsYoungGC()) {
        auto& asyncNativeCallbacksPack = GetEcmaVM()->GetAsyncNativePointerCallbacksPack();
        ResetNativeBindingSize();
        // array buffer
        auto iter = nativePointerList_.begin();
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_MAX, HITRACE_TAG_ARK,
            ("ProcessReferencesNum:" + std::to_string(nativePointerList_.size())).c_str(), "");
        while (iter != nativePointerList_.end()) {
            JSNativePointer* object = *iter;
            auto fwd = visitor(reinterpret_cast<TaggedObject*>(object));
            if (fwd == nullptr) {
                size_t bindingSize = object->GetBindingSize();
                asyncNativeCallbacksPack.AddCallback(std::make_pair(object->GetDeleter(),
                    std::make_tuple(thread_->GetEnv(), object->GetExternalPointer(), object->GetData())), bindingSize);
                nativeAreaAllocator_->DecreaseNativeSizeStats(bindingSize, object->GetNativeFlag());
                SwapBackAndPop(nativePointerList_, iter);
                continue;
            }
            IncreaseNativeBindingSize(JSNativePointer::Cast(fwd));
            if (fwd != reinterpret_cast<TaggedObject*>(object)) {
                *iter = JSNativePointer::Cast(fwd);
            }
            ++iter;
        }
        ShrinkWithFactor(nativePointerList_);

        auto& concurrentNativeCallbacks = GetEcmaVM()->GetConcurrentNativePointerCallbacks();
        auto newIter = concurrentNativePointerList_.begin();
        while (newIter != concurrentNativePointerList_.end()) {
            JSNativePointer* object = *newIter;
            auto fwd = visitor(reinterpret_cast<TaggedObject*>(object));
            if (fwd == nullptr) {
                nativeAreaAllocator_->DecreaseNativeSizeStats(object->GetBindingSize(), object->GetNativeFlag());
                concurrentNativeCallbacks.emplace_back(object->GetDeleter(),
                    std::make_tuple(thread_->GetEnv(), object->GetExternalPointer(), object->GetData()));
                SwapBackAndPop(concurrentNativePointerList_, newIter);
                continue;
            }
            IncreaseNativeBindingSize(JSNativePointer::Cast(fwd));
            if (fwd != reinterpret_cast<TaggedObject*>(object)) {
                *newIter = JSNativePointer::Cast(fwd);
            }
            ++newIter;
        }
        ShrinkWithFactor(concurrentNativePointerList_);
    }
}

void Heap::PushToNativePointerList(JSNativePointer* pointer, bool isConcurrent)
{
#ifndef USE_CMC_GC
    ASSERT(!JSTaggedValue(pointer).IsInSharedHeap());
#endif
    if (isConcurrent) {
        concurrentNativePointerList_.emplace_back(pointer);
    } else {
        nativePointerList_.emplace_back(pointer);
    }
}

#ifdef USE_CMC_GC
void Heap::IteratorNativePointerList(WeakVisitor &visitor)
{
    auto& asyncNativeCallbacksPack = GetEcmaVM()->GetAsyncNativePointerCallbacksPack();
    auto iter = nativePointerList_.begin();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_MAX, HITRACE_TAG_ARK,
        ("ProcessNativeDeleteNum:" + std::to_string(nativePointerList_.size())).c_str(), "");
    while (iter != nativePointerList_.end()) {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&(*iter)));
        bool isAlive = visitor.VisitRoot(Root::ROOT_VM, slot);
        if (!isAlive) {
            JSNativePointer* object = *iter;
            size_t bindingSize = object->GetBindingSize();
            asyncNativeCallbacksPack.AddCallback(std::make_pair(object->GetDeleter(),
                std::make_tuple(thread_->GetEnv(), object->GetExternalPointer(), object->GetData())), bindingSize);
            nativeAreaAllocator_->DecreaseNativeSizeStats(bindingSize, object->GetNativeFlag());
            SwapBackAndPop(nativePointerList_, iter);
        } else {
            ++iter;
        }
    }
    ShrinkWithFactor(nativePointerList_);

    auto& concurrentNativeCallbacks = GetEcmaVM()->GetConcurrentNativePointerCallbacks();
    auto concurrentIter = concurrentNativePointerList_.begin();
    while (concurrentIter != concurrentNativePointerList_.end()) {
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&(*concurrentIter)));
        bool isAlive = visitor.VisitRoot(Root::ROOT_VM, slot);
        if (!isAlive) {
            JSNativePointer* object = *concurrentIter;
            nativeAreaAllocator_->DecreaseNativeSizeStats(object->GetBindingSize(), object->GetNativeFlag());
            concurrentNativeCallbacks.emplace_back(object->GetDeleter(),
                std::make_tuple(thread_->GetEnv(), object->GetExternalPointer(), object->GetData()));
            SwapBackAndPop(concurrentNativePointerList_, concurrentIter);
        } else {
            ++concurrentIter;
        }
    }
    ShrinkWithFactor(concurrentNativePointerList_);
}
#endif

void Heap::RemoveFromNativePointerList(const JSNativePointer* pointer)
{
    auto iter = std::find(nativePointerList_.begin(), nativePointerList_.end(), pointer);
    if (iter != nativePointerList_.end()) {
        JSNativePointer* object = *iter;
        nativeAreaAllocator_->DecreaseNativeSizeStats(object->GetBindingSize(), object->GetNativeFlag());
        object->Destroy(thread_);
        SwapBackAndPop(nativePointerList_, iter);
    }
    auto newIter = std::find(concurrentNativePointerList_.begin(), concurrentNativePointerList_.end(), pointer);
    if (newIter != concurrentNativePointerList_.end()) {
        JSNativePointer* object = *newIter;
        nativeAreaAllocator_->DecreaseNativeSizeStats(object->GetBindingSize(), object->GetNativeFlag());
        object->Destroy(thread_);
        SwapBackAndPop(concurrentNativePointerList_, newIter);
    }
}

void Heap::ClearNativePointerList()
{
    for (auto iter : nativePointerList_) {
        iter->Destroy(thread_);
    }
    for (auto iter : concurrentNativePointerList_) {
        iter->Destroy(thread_);
    }
    nativePointerList_.clear();
}

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_INL_H
