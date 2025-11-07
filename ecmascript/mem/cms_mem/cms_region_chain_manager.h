/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_CMS_MEM_CMS_REGION_CHAIN_MANAGER_H
#define ECMASCRIPT_MEM_CMS_MEM_CMS_REGION_CHAIN_MANAGER_H

#include "ecmascript/mem/ecma_list.h"

#include "ecmascript/mem/cms_mem/slot_free_list_meta_info.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class Heap;

class CMSRegionChainManager {
public:
    CMSRegionChainManager() = default;
    ~CMSRegionChainManager() = default;

    NO_COPY_SEMANTIC(CMSRegionChainManager);
    NO_MOVE_SEMANTIC(CMSRegionChainManager);
    
    enum class SweepMode {
        SWEEP_ALL,
        SWEEP_UNTIL_ONE_FREE_LIST,
    };

    void Initialize(Heap *localHeap, size_t slotSize);

    Heap *GetLocalHeap() const
    {
        return localHeap_;
    }

    size_t GetSlotSize() const
    {
        return slotSize_;
    }

    uint32_t GetRegionCount() const
    {
        return regionList_.GetLength();
    }

    inline void AddNewRegion(Region *region);

    inline SlotFreeListMetaInfo TryTakeSlotFreeList();

    template <typename Callback>
    inline void EnumerateRegions(Callback &&cb);

    // In stw
    void Sweep();

    // In stw
    void PrepareSweeping();

    void ConcurrentSweep(const SweepMode sweepMode);

    // In stw
    void FinishSweeping();

    void PrepareCompact(std::vector<Region *> &pendingReclaimFromRegions);

private:
    inline SlotFreeListMetaInfo TryTakeUsableSlotFreeList();

    inline void RetrieveSweptSlotFreeLists();

    bool ConcurrentSweepRegion(Region *region);

    Region *TryTakeSweepingRegion();

    bool TrySweepRegion();

    SlotFreeListMetaInfo SweepRegionImpl(Region *region);

    void FreeLiveRange(Region *region, uintptr_t freeStart, uintptr_t freeEnd);

    bool SaveSweptRegion(Region *region, SlotFreeListMetaInfo maybeFreeList);

    enum class SweepingState {
        NO_SWEEP,
        SWEEPING,
    };

    Heap *localHeap_ {nullptr};
    size_t slotSize_ {0};
    EcmaList<Region> regionList_ {};

    Mutex mutex_;
    std::vector<Region *> sweepingRegionList_ {};
    std::vector<SlotFreeListMetaInfo> sweptSlotFreeList_ {};
    // fixme: if a region is full alived, do not need to sweep at all.
    std::vector<Region *> sweptFullRegionList_ {};
    // Do not need lock to access
    std::vector<SlotFreeListMetaInfo> usableSlotFreeList_ {};

    SweepingState sweepState_ {SweepingState::NO_SWEEP};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_CMS_REGION_CHAIN_MANAGER_H
