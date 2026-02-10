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

#include "ecmascript/mem/heap_region_allocator.h"

#include "ecmascript/jit/jit.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/runtime.h"
#include "ecmascript/runtime_lock.h"

namespace panda::ecmascript {

HeapRegionAllocator::HeapRegionAllocator(JSRuntimeOptions &option)
{
    enablePageTagThreadId_ = option.EnablePageTagThreadId();
}

Region *HeapRegionAllocator::AllocateAlignedRegion(Space *space, size_t capacity, JSThread* thread, BaseHeap *heap,
                                                   bool isFresh, size_t slotSize, Mutex *allocateLock)
{
    if (capacity == 0) { // LOCV_EXCL_BR_LINE
        LOG_ECMA_MEM(FATAL) << "capacity must have a size bigger than 0";
        UNREACHABLE();
    }
    RegionSpaceFlag flags = space->GetRegionFlag();
    RegionTypeFlag typeFlag = isFresh ? RegionTypeFlag::FRESH : RegionTypeFlag::DEFAULT;
    bool isRegular = (flags != RegionSpaceFlag::IN_HUGE_OBJECT_SPACE &&
        flags != RegionSpaceFlag::IN_HUGE_MACHINE_CODE_SPACE &&
        flags != RegionSpaceFlag::IN_SHARED_HUGE_OBJECT_SPACE);
    bool isCompress = flags == RegionSpaceFlag::IN_NON_MOVABLE_SPACE ||
        flags == RegionSpaceFlag::IN_SHARED_NON_MOVABLE || flags == RegionSpaceFlag::IN_READ_ONLY_SPACE ||
        flags == RegionSpaceFlag::IN_SHARED_READ_ONLY_SPACE;
    bool isMachineCode = (flags == RegionSpaceFlag::IN_MACHINE_CODE_SPACE ||
        flags == RegionSpaceFlag::IN_HUGE_MACHINE_CODE_SPACE);
    JSThread::ThreadId tid = 0;
    bool shouldPageTag = AllocateRegionShouldPageTag(space);
    if (enablePageTagThreadId_) {
        tid = thread ? thread->GetThreadId() : JSThread::GetCurrentThreadId();
    }
    auto pool = MemMapAllocator::GetInstance()->Allocate(tid, capacity, DEFAULT_REGION_SIZE,
        ToSpaceTypeName(space->GetSpaceType()), isRegular, isCompress, isMachineCode,
        Jit::GetInstance()->IsEnableJitFort(), shouldPageTag);
    void *mapMem = pool.GetMem();
    if (mapMem == nullptr) { // LOCV_EXCL_BR_LINE
        if (heap->InGC()) {
            // Donot crash in GC.
            TemporarilyEnsureAllocateionAlwaysSuccess(heap);
            pool = MemMapAllocator::GetInstance()->Allocate(tid, capacity, DEFAULT_REGION_SIZE,
                ToSpaceTypeName(space->GetSpaceType()), isRegular, isCompress, isMachineCode,
                Jit::GetInstance()->IsEnableJitFort(), shouldPageTag);
            mapMem = pool.GetMem();
            if (mapMem == nullptr) {
                // This should not happen
                LOG_ECMA_MEM(FATAL) << "pool is empty in GC unexpectedly, process out of memory";
                UNREACHABLE();
            }
        } else {
            // allocateLock and suspendLock can lead to a deadlock, so allocateLock needs to unlock in advance.
            // This solution is not perfect and will be improved in the future.
            if (allocateLock != nullptr) {
                allocateLock->Unlock();
            }
            if (thread != nullptr && thread->GetEcmaVM()->IsInitialized()) {
                Heap *localHeap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
                // Ensure that Dump is executed only once across all threads.
                // Other threads will block here until the first thread completes the dump.
                {
                    static bool needDumpHeapSnapshot = true;
                    static Mutex dumpMutex;
                    RuntimeLockHolder lock(thread, dumpMutex);
                    if (needDumpHeapSnapshot) {
                        localHeap->DumpHeapSnapshotBeforeOOM(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());
                        needDumpHeapSnapshot = false;
                    }
                }
                heap->ThrowOutOfMemoryErrorForDefault(thread, DEFAULT_REGION_SIZE,
                    "HeapRegionAllocator::AllocateAlignedRegion", false);
            }
            LOG_ECMA_MEM(FATAL) << "pool is empty " << annoMemoryUsage_.load(std::memory_order_relaxed)
                                << ", process out of memory";
            UNREACHABLE();
        }
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(mapMem, capacity, 0, capacity) != EOK) { // LOCV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    IncreaseAnnoMemoryUsage(capacity);

    uintptr_t mem = ToUintPtr(mapMem);
    uintptr_t end = mem + capacity;

    Region *region = nullptr;
    if (slotSize == 0) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        uintptr_t begin = AlignUp(mem + sizeof(DefaultRegion), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        region = new (ToVoidPtr(mem)) DefaultRegion(heap->GetNativeAreaAllocator(), mem, begin, end, flags, typeFlag);
    } else {
        ASSERT(G_USE_CMS_GC);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        uintptr_t begin = AlignUp(mem + sizeof(CMSRegion), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        region = new (ToVoidPtr(mem)) CMSRegion(heap->GetNativeAreaAllocator(), mem, begin, end, flags, typeFlag,
                                                slotSize);
    }
    // Check that the address is 256K byte aligned
    uintptr_t addr = ToUintPtr(region);
    LOG_ECMA_IF(AlignUp(addr, DEFAULT_REGION_SIZE) != addr, FATAL) << "region not align by " << DEFAULT_REGION_SIZE;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return region;
}

template<bool asyncFreeMem>
void HeapRegionAllocator::FreeRegion(Region *region, size_t cachedSize, bool skipCache)
{
    auto size = region->GetCapacity();
    bool isRegular = !region->InHugeObjectSpace() && !region->InHugeMachineCodeSpace() &&
        !region->InSharedHugeObjectSpace();
    bool isCompress = region->InNonMovableSpace() || region->InSharedNonMovableSpace() ||
        region->InReadOnlySpace() || region->InSharedReadOnlySpace();
    auto allocateBase = region->GetAllocateBase();
    bool shouldPageTag = FreeRegionShouldPageTag(region);
    bool inHugeMachineCodeSpace = region->InHugeMachineCodeSpace();

    DecreaseAnnoMemoryUsage(size);
    region->Invalidate();

    // Use the mmap interface to clean up the MAP_JIT tag bits on memory.
    // The MAP_JIT flag prevents the mprotect interface from setting PROT_WRITE for memory.
    if (inHugeMachineCodeSpace) {
        MemMap memMap = PageMap(size, PAGE_PROT_NONE, 0, ToVoidPtr(allocateBase), PAGE_FLAG_MAP_FIXED);
        if (memMap.GetMem() == nullptr) {
            LOG_ECMA_MEM(FATAL) << "Failed to clear the MAP_JIT flag for the huge machine code space.";
        }
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ToVoidPtr(allocateBase), size, INVALID_VALUE, size) != EOK) { // LOCV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    if constexpr (asyncFreeMem) {
        MemMapAllocator::GetInstance()->AsyncFree(ToVoidPtr(allocateBase), size, isRegular, isCompress, shouldPageTag);
    } else {
        MemMapAllocator::GetInstance()->CacheOrFree(ToVoidPtr(allocateBase), size, isRegular, isCompress, cachedSize,
                                                    shouldPageTag, skipCache);
    }
    MEMORY_TRACE_FREEREGION(allocateBase, size);
}

// Only decrease memory usage here. And the memory will be released at background thread later.
void HeapRegionAllocator::DecreaseMemMapUsage(Region *region)
{
    bool isRegular = !region->InHugeObjectSpace() && !region->InHugeMachineCodeSpace() &&
        !region->InSharedHugeObjectSpace();
    MemMapAllocator::GetInstance()->DecreaseMemUsage(region->GetCapacity(), isRegular);
}

void HeapRegionAllocator::TemporarilyEnsureAllocateionAlwaysSuccess(BaseHeap *heap)
{
    // Make MemMapAllocator infinite, and record to Dump&Fatal when GC completed.
    heap->ShouldForceThrowOOMError();
    MemMapAllocator::GetInstance()->TransferToInfiniteModeForGC();
}

bool HeapRegionAllocator::AllocateRegionShouldPageTag(Space *space) const
{
    if (enablePageTagThreadId_) {
        return true;
    }
    MemSpaceType type = space->GetSpaceType();
    // Both LocalSpace and OldSpace belong to OldSpace.
    return type != MemSpaceType::OLD_SPACE && type != MemSpaceType::LOCAL_SPACE;
}

bool HeapRegionAllocator::FreeRegionShouldPageTag(Region *region) const
{
    if (enablePageTagThreadId_) {
        return true;
    }
    // There is no LocalSpace tag in region.
    return !region->InOldSpace();
}

template void HeapRegionAllocator::FreeRegion<true>(Region *region, size_t cachedSize, bool skipCache);
template void HeapRegionAllocator::FreeRegion<false>(Region *region, size_t cachedSize, bool skipCache);
}  // namespace panda::ecmascript
