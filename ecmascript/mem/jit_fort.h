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

#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript {

class JitFortRegion;
class JitFortMemDescPool;
template <typename T>
class FreeListAllocator;

class JitFort {
public:
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

    inline uintptr_t JitFortBegin()
    {
        return jitFortBegin_;
    }

    inline size_t JitFortSize()
    {
        return jitFortSize_;
    }

    bool InRange(uintptr_t address) const;
    void CollectFreeRanges(JitFortRegion  *region);
    void UpdateFreeSpace();

    JitFortRegion *ObjectAddressToRange(uintptr_t objAddress);
    static void InitJitFortResource();
    void PrepareSweeping();
    void AsyncSweep();
    void Sweep();
    void MarkJitFortMemAlive(MachineCode *obj);
    void MarkJitFortMemAwaitInstall(uintptr_t addr, size_t size);
    void MarkJitFortMemInstalled(MachineCode *obj);
    void FreeRegion(JitFortRegion *region);
    uint32_t AddrToFortRegionIdx(uint64_t addr);
    size_t FortAllocSize(size_t instrSize);
    PUBLIC_API static bool IsResourceAvailable();

private:
    static bool isResourceAvailable_;
    FreeListAllocator<MemDesc> *allocator_ {nullptr};

    // Fort memory space
    static constexpr int MAP_JITFORT = 0x1000;
    static constexpr size_t JIT_FORT_REG_SPACE_MAX = 4_MB;
    static constexpr size_t JIT_FORT_HUGE_SPACE_MAX = 2_MB;
    static constexpr size_t JIT_FORT_MEM_DESC_MAX = 40_KB;
    MemMap jitFortMem_;
    uintptr_t jitFortBegin_ {0};
    size_t jitFortSize_ {0};

    // Fort regions
    static constexpr uint32_t FORT_BUF_ALIGN = 32;
    static constexpr uint32_t FORT_BUF_ALIGN_LOG2 = base::MathHelper::GetIntLog2(FORT_BUF_ALIGN);
    static constexpr size_t FORT_BUF_ADDR_MASK = FORT_BUF_ALIGN - 1;
    static constexpr size_t MAX_JIT_FORT_REGIONS = JIT_FORT_REG_SPACE_MAX/DEFAULT_REGION_SIZE;
    std::array<JitFortRegion *, MAX_JIT_FORT_REGIONS>regions_;
    size_t nextFreeRegionIdx_ {0};
    EcmaList<JitFortRegion> regionList_ {}; // regions in use by Jit Fort allocator

    MemDescPool *memDescPool_ {nullptr};

    bool freeListUpdated_ {false};  // use atomic if not mutext protected
    Mutex mutex_;
    Mutex liveJitCodeBlksLock_;
    std::atomic<bool> isSweeping_ {false};
    friend class HugeMachineCodeSpace;
};

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
        ClearBit((addr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
    }

    inline bool Test(uintptr_t addr)
    {
        return TestBit((addr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
    }
};

class JitFortRegion : public Region {
public:
    JitFortRegion(NativeAreaAllocator *allocator, uintptr_t allocateBase, uintptr_t end,
        RegionSpaceFlag spaceType, MemDescPool *pool) : Region(allocator, allocateBase, end, spaceType),
        memDescPool_(pool)
    {
        markGCBitset_ = new(reinterpret_cast<void *>(gcBitSet_)) JitFortGCBitset();
        markGCBitset_->Clear(bitsetSize_);
        InitializeFreeObjectSets();
    }

    void InitializeFreeObjectSets()
    {
        fortFreeObjectSets_ = Span<FreeObjectSet<MemDesc> *>(new FreeObjectSet<MemDesc>
            *[FreeObjectList<MemDesc>::NumberOfSets()](), FreeObjectList<MemDesc>::NumberOfSets());
    }

    void DestroyFreeObjectSets()
    {
        for (auto set : fortFreeObjectSets_) {
            delete set;
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

    inline JitFortGCBitset *GetGCBitset()
    {
        return markGCBitset_;
    }

    inline size_t GetGCBitsetSize()
    {
        return bitsetSize_;
    }

    inline bool AtomicMark(void *address)
    {
        auto addrPtr = reinterpret_cast<uintptr_t>(address);
        ASSERT(InRange(addrPtr));
        return markGCBitset_->SetBit<AccessType::ATOMIC>(
            (addrPtr & DEFAULT_REGION_MASK) >> TAGGED_TYPE_SIZE_LOG);
    }

private:
    Span<FreeObjectSet<MemDesc> *> fortFreeObjectSets_;
    JitFortRegion *next_ {nullptr};
    JitFortRegion *prev_ {nullptr};
    MemDescPool *memDescPool_ {nullptr};

    static constexpr int FORT_REGION_BITSET_SIZE = 4096;
    size_t bitsetSize_ {FORT_REGION_BITSET_SIZE};
    alignas(uint64_t) uint8_t gcBitSet_[FORT_REGION_BITSET_SIZE];
    alignas(uint64_t) JitFortGCBitset *markGCBitset_ {nullptr};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPARSE_SPACE_H
