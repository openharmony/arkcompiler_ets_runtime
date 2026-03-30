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

#ifndef ECMASCRIPT_MEM_JIT_FORT_H
#define ECMASCRIPT_MEM_JIT_FORT_H

#include <array>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript {

class JitFortRegion;
class JitFortMemDescPool;
template <typename T>
class FreeListAllocator;

/*
 * @class JitFort
 * @brief JIT Code Memory Manager with dual-region architecture
 *
 * JitFort implements a two-tier memory allocation system specialized for
 * JIT-compiled machine code in ArkTS runtime:
 *
 * Tier 1 - Small Fort: 16 regions of 256KB each (4MB total)
 * - Fast allocation for regular-sized JIT code blocks
 * - Region-based memory management for cache locality
 * - 4KB GC bitset per region for marking operations
 *
 * Tier 2 - Huge Fort: 1 region of 4MB (contiguous block)
 * - Allocation for large JIT code blocks that exceed region capacity
 * - Single contiguous memory for allocation of very large functions
 * - 64KB GC bitset for comprehensive marking coverage
 *
 * ┌─────────────────────────────────────────────────────────────┐
 * │                 JIT Fort Memory Manager                     │
 * │                                                             │
 * │  Small Fort: ┌──────────┐ ┌─────────┐ ... (regions*16)      │
 * │              │ Region 0 │ │ Region 1│                       │
 * │              │ (256KB)  │ │ (256KB) │                       │
 * │              └──────────┘ └─────────┘                       │
 * │              total: 4MB (16×256KB)                          │
 * │                                                             │
 * │  Huge Fort:  ┌──────────────────────────────────────────┐   │
 * │              │              Single Region               │   │
 * │              │              (4MB)                       │   │
 * │              └──────────────────────────────────────────┘   │
 * └─────────────────────────────────────────────────────────────┘

 * Key Principles Design:
 * - Separation: JIT code isolated from regular heap objects
 * - Efficiency: Region-based allocation with O(1) free list operations
 * - Safety: Thread-safe operations with proper synchronization
 * - Integration: Seamless GC cooperation with 4-bit per-block encoding
 * - Protection: Memory awareness for security and debugging
 *
 * Memory Block Structure:
 * ┌─────────────────────────────────────────────────────────────┐
 * │ each JIT code need at least 32Byte(using 4bit gc status)    │
 * │                                                             │
 * │  Code Segment (at least 32Byte each):                       │
 * │  ┌─────────┬─────────┬─────────┬─────────┐                  │
 * │  │  code 1 │  code 2 │  ...    │  code n │                  │
 * │  └─────────┴─────────┴─────────┴─────────┘                  │
 * │     ↑                                                       │
 * │   GCBitSet                                                  │
 * ├───────────────────────────┐                                 │
 * │ GC Bitset (4 bit):        │                                 │
 * │ ┌──┬──┬──┬──┐             │                                 │
 * │ │S0│S1│E0│E1│             │                                 │
 * │ └──┴──┴──┴──┘             │                                 │
 * │ │Start│ End │             │                                 │
 * │ │Bits │ Bits│             │                                 │
 * ├───────────────────────────┘                                 │
 * └─────────────────────────────────────────────────────────────┘
 * State Management per 32-byte Block:
 * ┌───────────────────────────────────────────────────────────┐
 * │  Block State   │ Start Bits │ End Bits │ Description      │
 * ├───────────────────────────────────────────────────────────┤
 * │ Free           │     00     │    00    │ Free for alloc   │
 * │ Installed      │     10     │    01    │ Ready execution  │
 * │ Awaiting Inst  │     11     │    01    │ Install pending  │
 * └───────────────────────────────────────────────────────────┘
 *
 */
class JitFort {
public:
    // Fort memory space
    static constexpr int MAP_JITFORT = 0x1000;
    static constexpr size_t JIT_FORT_REG_SPACE_MAX = 4_MB;
    static constexpr size_t JIT_FORT_HUGE_SPACE_MAX = 2_MB;
    static constexpr size_t JIT_FORT_MEM_DESC_MAX = 40_KB;
    static constexpr size_t SMALL_GCBISET_SIZE = 4096;
    static constexpr size_t HUGE_GCBISET_SIZE = 65536;

    // Fort regions
    static constexpr uint32_t FORT_BUF_ALIGN = 32;
    static constexpr uint32_t FORT_BUF_ALIGN_LOG2 = base::MathHelper::GetIntLog2(FORT_BUF_ALIGN);
    static constexpr size_t FORT_BUF_ADDR_MASK = FORT_BUF_ALIGN - 1;
    static constexpr size_t MAX_JIT_FORT_REGIONS = JIT_FORT_REG_SPACE_MAX / DEFAULT_REGION_SIZE;

    JitFort();
    ~JitFort();
    NO_COPY_SEMANTIC(JitFort);
    NO_MOVE_SEMANTIC(JitFort);

    void InitRegions();
    bool AddRegion();
    uintptr_t Allocate(MachineCodeDesc *desc);

    inline JitFortRegion *GetRegionList()
    {
        return regionList_.GetFirst();
    }

    bool InJitFortRange(uintptr_t address) const;
    bool InSmallRange(uintptr_t address) const;
    bool InHugeRange(uintptr_t address) const;
    void UpdateFreeSpace(bool inHuge = false);
    // Used by CMCGC to clear the marking bits in Young GC.
    void ClearMarkBits();

    JitFortRegion *ObjectAddressToRange(uintptr_t objAddress);
    static void InitJitFort();
    static void InitJitFortResource();
    void PrepareSweeping();
    void AsyncSweep(bool inHuge = false);
    void Sweep(bool inHuge = false);
    void MarkJitFortMemAlive(MachineCode *obj, bool inHuge = false);
    void MarkJitFortMemAwaitInstall(uintptr_t addr, size_t size, bool inHuge = false);
    void MarkJitFortMemInstalled(MachineCode *obj, bool inHuge = false);
    void FreeRegion(JitFortRegion *region, bool inHuge = false);

    uint32_t AddrToFortRegionIdx(uint64_t addr);
    size_t FortAllocSize(size_t instrSize);
    PUBLIC_API static bool IsResourceAvailable();

private:
    static bool isResourceAvailable_;

    // For small fort space
    FreeListAllocator<MemDesc> *allocator_ {nullptr};
    MemMap jitFortMem_;
    uintptr_t jitFortBegin_ {0};
    size_t jitFortSize_ {0};
    std::array<JitFortRegion *, MAX_JIT_FORT_REGIONS> regions_ {};
    size_t nextFreeRegionIdx_ {0};
    EcmaList<JitFortRegion> regionList_ {}; // regions in use by Jit Fort allocator
    MemDescPool *memDescPool_ {nullptr};
    uintptr_t AllocateSmall(size_t size);

    // For huge fort space
    FreeListAllocator<MemDesc> *hugeAlloc_ {nullptr};
    MemMap hugeJitFortMem_;
    uintptr_t hugeJitFortBegin_ {0};
    size_t hugeJitFortSize_ {0};
    JitFortRegion *hugeRegion_  {nullptr};
    bool hugeInUse_ {false};
    MemDescPool *hugeMemDescPool_ {nullptr};
    uintptr_t AllocateHuge(size_t size);
    void InitHugeRegion();

    Mutex mutex_;
    std::atomic<bool> isSweeping_ {false};
    friend class HugeMachineCodeSpace;
};

template <size_t RegionMask>
class JitFortGCBitset : public GCBitset {
public:
    JitFortGCBitset() = default;
    ~JitFortGCBitset() = default;

    NO_COPY_SEMANTIC(JitFortGCBitset);
    NO_MOVE_SEMANTIC(JitFortGCBitset);

    template <typename Visitor>
    void IterateMarkedBitsConst(uintptr_t regionAddr, size_t bitsetSize, Visitor visitor);
    void MarkStartAddr(bool awaitInstall, uintptr_t startAddr, uint32_t index, uint32_t &word);
    void MarkEndAddr(bool awaitInstall, uintptr_t endAddr, uint32_t index, uint32_t &word);

    size_t WordCount(size_t size) const
    {
        return size >> BYTE_PER_WORD_LOG2;
    }

    inline void ClearMark(uintptr_t addr)
    {
        LOG_JIT(INFO) << "JitFortGCBitset::ClearMark, addr: " << reinterpret_cast<void*>(addr);
        // The byte offset of addr within the region.
        uintptr_t offset = (addr & RegionMask) >> TAGGED_TYPE_SIZE_LOG;
        ClearBit<AccessType::ATOMIC>(offset);
    }

    inline bool Test(uintptr_t addr)
    {
        return TestBit((addr & RegionMask) >> TAGGED_TYPE_SIZE_LOG);
    }
};

class JitFortRegion : public Region {
public:
    JitFortRegion(NativeAreaAllocator *allocator, uintptr_t allocateBase, uintptr_t end,
        RegionSpaceFlag spaceType, MemDescPool *pool) : Region(allocator, allocateBase, end, spaceType),
        memDescPool_(pool)
    {
        auto regionSize = end - allocateBase;
        ASSERT(regionSize == DEFAULT_REGION_SIZE || regionSize == HUGE_JITFORT_REGION_SIZE);
        bitsetSize_ = GCBitset::SizeOfGCBitset(regionSize);
        ASSERT(bitsetSize_ == JitFort::SMALL_GCBISET_SIZE || bitsetSize_ == JitFort::HUGE_GCBISET_SIZE);
        gcBitSet_ = new uint8_t[bitsetSize_];
        regionMask_ = regionSize - 1;
        if (regionMask_ == DEFAULT_REGION_MASK) {
            markGCBitset_ = new(reinterpret_cast<void *>(gcBitSet_)) JitFortGCBitset<DEFAULT_REGION_MASK>();
        } else {
            markGCBitset_ = new(reinterpret_cast<void *>(gcBitSet_)) JitFortGCBitset<HUGE_JITFORT_REGION_MASK>();
        }
        markGCBitset_->Clear(bitsetSize_);
        InitializeFreeObjectSets();
    }

    ~JitFortRegion()
    {
        delete gcBitSet_;
    }

    void InitializeFreeObjectSets()
    {
        auto numberOfSets = FreeObjectList<MemDesc>::NumberOfSets();
        fortFreeObjectSets_ =
            Span<FreeObjectSet<MemDesc> *>(new FreeObjectSet<MemDesc> *[numberOfSets](), numberOfSets);
    }

    void DestroyFreeObjectSets()
    {
        for (auto set : fortFreeObjectSets_) {
            if (set != nullptr) {
                delete set;
            }
        }
        delete[] fortFreeObjectSets_.data();
    }

    FreeObjectSet<MemDesc> *GetFreeObjectSet(SetType type)
    {
        // Thread safe
        if (fortFreeObjectSets_[type] == nullptr) {
            fortFreeObjectSets_[type] = new FreeObjectSet<MemDesc>(type, memDescPool_);
        }
        return fortFreeObjectSets_[type];
    }

    inline void LinkNext(JitFortRegion *next)
    {
        next_ = next;
    }

    inline JitFortRegion *GetNext() const
    {
        return next_;
    }

    inline void LinkPrev(JitFortRegion *prev)
    {
        prev_ = prev;
    }

    inline JitFortRegion *GetPrev() const
    {
        return prev_;
    }

    inline size_t GetGCBitsetSize()
    {
        return bitsetSize_;
    }

    template <typename Visitor>
    void IterateMarkedBitsConst(Visitor visitor)
    {
        uintptr_t regionAddr = GetBegin();
        if (regionMask_ == DEFAULT_REGION_MASK) {
            auto* bitset = reinterpret_cast<JitFortGCBitset<DEFAULT_REGION_MASK>*>(markGCBitset_);
            bitset->IterateMarkedBitsConst(regionAddr, bitsetSize_, visitor);
        } else {
            auto* bitset = reinterpret_cast<JitFortGCBitset<HUGE_JITFORT_REGION_MASK>*>(markGCBitset_);
            bitset->IterateMarkedBitsConst(regionAddr, bitsetSize_, visitor);
        }
    }

    inline bool AtomicMark(void *address)
    {
        auto addrPtr = reinterpret_cast<uintptr_t>(address);
        ASSERT(InRange(addrPtr));   // Region::InRange, equals to JitFort::InJitFortRange
        return markGCBitset_->SetBit<AccessType::ATOMIC>(
            (addrPtr & regionMask_) >> TAGGED_TYPE_SIZE_LOG);
    }

    inline void ClearMark(void *address)
    {
        auto addrPtr = reinterpret_cast<uintptr_t>(address);
        ASSERT(InRange(addrPtr));   // Region::InRange, equals to JitFort::InJitFortRange
        return markGCBitset_->ClearBit<AccessType::ATOMIC>(
            (addrPtr & regionMask_) >> TAGGED_TYPE_SIZE_LOG);
    }

private:
    Span<FreeObjectSet<MemDesc> *> fortFreeObjectSets_;
    JitFortRegion *next_ {nullptr};
    JitFortRegion *prev_ {nullptr};
    MemDescPool *memDescPool_ {nullptr};

    size_t bitsetSize_ {0};
    size_t regionMask_ {0};
    alignas(uint64_t) uint8_t *gcBitSet_ {nullptr};
    alignas(uint64_t) GCBitset *markGCBitset_ {nullptr};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPARSE_SPACE_H
