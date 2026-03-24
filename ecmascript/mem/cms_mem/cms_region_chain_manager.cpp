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

#include "ecmascript/mem/cms_mem/cms_region_chain_manager-inl.h"

#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {

void CMSRegionChainManager::Initialize(Heap *localHeap, size_t slotSize)
{
    ASSERT(localHeap_ == nullptr && localHeap != nullptr);
    ASSERT(slotSize_ == 0);
    localHeap_ = localHeap;
    slotSize_ = slotSize;
}

size_t CMSRegionChainManager::Sweep(std::vector<Region *> &pendingReclaimFromRegions)
{
    ASSERT(TryTakeSweepingRegion() == nullptr);
    ASSERT(sweptSlotFreeList_.empty());
    ASSERT(sweptFullRegionList_.empty());

    usableSlotFreeList_.clear();
    size_t survivalObjectSize = 0;
    EnumerateRegions([this, &pendingReclaimFromRegions, &survivalObjectSize](Region *region) {
        size_t survivalSize = region->AliveObject();
        region->ResetAliveObject();
        survivalObjectSize += survivalSize;
        if (survivalSize == 0) {
            regionList_.RemoveNode(region);
            pendingReclaimFromRegions.emplace_back(region);
            localHeap_->GetSlotSpace()->DecreaseCommitted(region->GetCapacity());
        } else {
            // fixme: Full alived region do not need to sweep
            SlotFreeListMetaInfo freeList = SweepRegionImpl(region);
            if (freeList.firstFreeSegment_ != nullptr) {
                ASSERT(Region::ObjectAddressToRange(ToUintPtr(freeList.firstFreeSegment_)) == region);
                usableSlotFreeList_.emplace_back(freeList);
            }
        }
    });
    return survivalObjectSize;
}

size_t CMSRegionChainManager::PrepareSweeping(std::vector<Region *> &pendingReclaimFromRegions)
{
    ASSERT(TryTakeSweepingRegion() == nullptr);
    ASSERT(sweptSlotFreeList_.empty());
    ASSERT(sweptFullRegionList_.empty());

    // In stw, so do not need lock
    usableSlotFreeList_.clear();
    size_t survivalObjectSize = 0;
    EnumerateRegions([this, &pendingReclaimFromRegions, &survivalObjectSize](Region *region) {
        ASSERT(!region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
        size_t survivalSize = region->AliveObject();
        region->ResetAliveObject();
        survivalObjectSize += survivalSize;
        if (survivalSize == 0) {
            regionList_.RemoveNode(region);
            pendingReclaimFromRegions.emplace_back(region);
            localHeap_->GetSlotSpace()->DecreaseCommitted(region->GetCapacity());
        } else {
            // fixme: Full alived region do not need to sweep
            region->SwapLocalToShareRSetForCS();
            sweepingRegionList_.emplace_back(region);
        }
    });
    size_t num = sweepingRegionList_.size();
    if (num > 0) {
        numPendingSweepingRegions_.store(num, std::memory_order_relaxed);
        sweepState_ = SweepingState::SWEEPING;
    }
    return survivalObjectSize;
}

void CMSRegionChainManager::ConcurrentSweep(const SweepMode sweepMode)
{
    size_t swept = 0;
    while (true) {
        Region *region = TryTakeSweepingRegion();
        if (region == nullptr) {
            break;
        }

        ++swept;
        if (ConcurrentSweepRegion(region) && sweepMode == SweepMode::SWEEP_UNTIL_ONE_FREE_LIST) {
            break;
        }
    }
    if (swept > 0 && numPendingSweepingRegions_.fetch_sub(swept, std::memory_order_relaxed) == swept) {
        localHeap_->GetSlotSpace()->AddSweptRegionChainManager(this);
    }
}

bool CMSRegionChainManager::ConcurrentSweepRegion(Region *region)
{
    SlotFreeListMetaInfo maybeFreeList = SweepRegionImpl(region);
    return SaveSweptRegion(region, maybeFreeList);
}

Region *CMSRegionChainManager::TryTakeSweepingRegion()
{
    LockHolder holder(mutex_);
    if (sweepingRegionList_.empty()) {
        return nullptr;
    }
    Region *region = sweepingRegionList_.back();
    sweepingRegionList_.pop_back();
    return region;
}

SlotFreeListMetaInfo CMSRegionChainManager::SweepRegionImpl(Region *region)
{
    uintptr_t freeStart = region->GetBegin();
    SlotFreeSegment *prevFreeSegment = nullptr;
    size_t totalUsableSize = 0;
    auto appendToFreeSegmentsChain = [&prevFreeSegment, &totalUsableSize](uintptr_t freeStart, uintptr_t freeEnd) {
        size_t freeSize = freeEnd - freeStart;
        totalUsableSize += freeSize;
        SlotFreeSegment *currentFreeSegment = SlotFreeSegment::Cast(freeStart);
        if (LIKELY(prevFreeSegment != nullptr)) {
            currentFreeSegment->SetNextFreeSegment(prevFreeSegment);
        }
        prevFreeSegment = currentFreeSegment;
    };
    region->IterateAllMarkedBits([this, region, &freeStart, &appendToFreeSegmentsChain](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));

        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            FreeLiveRange(region, freeStart, freeEnd);
            appendToFreeSegmentsChain(freeStart, freeEnd);
        }
        freeStart = freeEnd + slotSize_;
    });
    uintptr_t freeEnd = CMSRegion::GetAllocatableEnd(CMSRegion::FromRegion(region), slotSize_);
    if (freeStart != freeEnd) {
        FreeLiveRange(region, freeStart, freeEnd);
        appendToFreeSegmentsChain(freeStart, freeEnd);
    }

    return {prevFreeSegment, totalUsableSize};
}

void CMSRegionChainManager::FreeLiveRange(Region *region, uintptr_t freeStart, uintptr_t freeEnd)
{
    size_t freeSize = freeEnd - freeStart;
    ASSERT(freeSize % slotSize_ == 0);
    SlotFreeSegment::FillFreeSegment(freeStart, freeSize);
    localHeap_->GetSweeper()->ClearRSetInRange(region, freeStart, freeEnd);
}

bool CMSRegionChainManager::SaveSweptRegion(Region *region, SlotFreeListMetaInfo maybeFreeList)
{
    if (maybeFreeList.firstFreeSegment_ != nullptr) {
        ASSERT(Region::ObjectAddressToRange(ToUintPtr(maybeFreeList.firstFreeSegment_)) == region);
        {
            LockHolder holder(mutex_);
            region->SetSwept();
            sweptSlotFreeList_.emplace_back(maybeFreeList);
        }
        return true;
    }
    {
        LockHolder holder(mutex_);
        region->SetSwept();
        sweptFullRegionList_.emplace_back(region);
    }
    return false;
}

void CMSRegionChainManager::FinishSweeping()
{
    ASSERT(TryTakeSweepingRegion() == nullptr);
    RetrieveSweptSlotFreeLists();
    for (Region *region : sweptFullRegionList_) {
        region->ResetSwept();
        region->MergeLocalToShareRSetForCS();
    }
    sweptFullRegionList_.clear();
    sweepState_ = SweepingState::NO_SWEEP;
}

void CMSRegionChainManager::PrepareCompact(std::vector<Region *> &pendingReclaimFromRegions)
{
    ASSERT(TryTakeSweepingRegion() == nullptr);
    ASSERT(sweptSlotFreeList_.empty());
    ASSERT(sweptFullRegionList_.empty());
    EnumerateRegions([&pendingReclaimFromRegions](Region *region) {
        pendingReclaimFromRegions.emplace_back(region);
    });
    regionList_.Clear();
    usableSlotFreeList_.clear();
}
}  // namespace panda::ecmascript