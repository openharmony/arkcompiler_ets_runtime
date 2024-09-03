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

    inline bool  IsMachineCodeGC()
    {
        return isMachineCodeGC_.load(std::memory_order_acquire);
    }

    inline void SetMachineCodeGC(bool flag)
    {
        LOG_JIT(DEBUG) << "SetMachineCodeGC " << flag;
        isMachineCodeGC_.store(flag, std::memory_order_release);
    }

    bool InRange(uintptr_t address) const;
    MemDesc *RecordLiveJitCode(uintptr_t addr, size_t size, bool installed = true);
    MemDesc *RecordLiveJitCodeNoLock(uintptr_t addr, size_t size, bool installed = false);
    void CollectFreeRanges(JitFortRegion  *region);
    void SortLiveMemDescList();
    void UpdateFreeSpace();
    JitFortRegion *ObjectAddressToRange(uintptr_t objAddress);
    static void InitJitFortResource();
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
    static constexpr size_t MAX_JIT_FORT_REGIONS = JIT_FORT_REG_SPACE_MAX/DEFAULT_REGION_SIZE;
    std::array<JitFortRegion *, MAX_JIT_FORT_REGIONS>regions_;
    size_t nextFreeRegionIdx_ {0};
    EcmaList<JitFortRegion> regionList_ {}; // regions in use by Jit Fort allocator

    // Jit Fort MemDesc to track live code blocks in Jit Fort space
    std::vector<MemDesc*>liveJitCodeBlks_;
    MemDescPool *memDescPool_ {nullptr};

    bool freeListUpdated_ {false};  // use atomic if not mutext protected
    Mutex mutex_;
    Mutex liveJitCodeBlksLock_;
    std::atomic<bool> isMachineCodeGC_ {false};
    friend class HugeMachineCodeSpace;
};

class JitFortRegion : public Region {
public:
    JitFortRegion(NativeAreaAllocator *allocator, uintptr_t allocateBase, uintptr_t end,
        RegionSpaceFlag spaceType, MemDescPool *pool) : Region(allocator, allocateBase, end, spaceType),
        memDescPool_(pool) {}

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

private:
    Span<FreeObjectSet<MemDesc> *> fortFreeObjectSets_;
    JitFortRegion *next_ {nullptr};
    JitFortRegion *prev_ {nullptr};
    MemDescPool *memDescPool_ {nullptr};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPARSE_SPACE_H
