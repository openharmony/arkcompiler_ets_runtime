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

namespace panda::ecmascript {
constexpr size_t PANDA_POOL_ALIGNMENT_IN_BYTES = 256_KB;

Region *HeapRegionAllocator::AllocateAlignedRegion(Space *space, size_t capacity, JSThread* thread, BaseHeap *heap,
                                                   bool isFresh)
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
    bool isMachineCode = (flags == RegionSpaceFlag::IN_MACHINE_CODE_SPACE ||
        flags == RegionSpaceFlag::IN_HUGE_MACHINE_CODE_SPACE);
    auto tid = thread ? thread->GetThreadId() : JSThread::GetCurrentThreadId();
    auto pool = MemMapAllocator::GetInstance()->Allocate(tid, capacity, DEFAULT_REGION_SIZE,
                                                         ToSpaceTypeName(space->GetSpaceType()),
                                                         isRegular, isMachineCode,
                                                         Jit::GetInstance()->IsEnableJitFort());
    void *mapMem = pool.GetMem();
    if (mapMem == nullptr) { // LOCV_EXCL_BR_LINE
        if (thread != nullptr && thread->GetEcmaVM()->IsInitialized()) {
            Heap *localHeap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            localHeap->DumpHeapSnapshotBeforeOOM();
            heap->ThrowOutOfMemoryErrorForDefault(thread, DEFAULT_REGION_SIZE,
                "HeapRegionAllocator::AllocateAlignedRegion", false);
        }
        LOG_ECMA_MEM(FATAL) << "pool is empty " << annoMemoryUsage_.load(std::memory_order_relaxed);
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(mapMem, capacity, 0, capacity) != EOK) { // LOCV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    IncreaseAnnoMemoryUsage(capacity);

    uintptr_t mem = ToUintPtr(mapMem);
    // Check that the address is 256K byte aligned
    LOG_ECMA_IF(AlignUp(mem, PANDA_POOL_ALIGNMENT_IN_BYTES) != mem, FATAL) << "region not align by 256KB";
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uintptr_t begin = AlignUp(mem + sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
    uintptr_t end = mem + capacity;

    Region *region = new (ToVoidPtr(mem)) Region(heap->GetNativeAreaAllocator(), mem, begin, end, flags, typeFlag);
    region->Initialize();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return region;
}

void HeapRegionAllocator::FreeRegion(Region *region, size_t cachedSize)
{
    auto size = region->GetCapacity();
    bool isRegular = !region->InHugeObjectSpace() && !region->InHugeMachineCodeSpace() &&
        !region->InSharedHugeObjectSpace();
    auto allocateBase = region->GetAllocateBase();

    DecreaseAnnoMemoryUsage(size);
    region->Invalidate();
    region->ClearMembers();
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ToVoidPtr(allocateBase), size, INVALID_VALUE, size) != EOK) { // LOCV_EXCL_BR_LINE
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    MemMapAllocator::GetInstance()->CacheOrFree(ToVoidPtr(allocateBase),
                                                size, isRegular, cachedSize);
}
}  // namespace panda::ecmascript
