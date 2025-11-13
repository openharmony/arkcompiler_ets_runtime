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
        survivalObjectSize += survivalSize;
        if (survivalSize == 0) {
            regionList_.RemoveNode(region);
            pendingReclaimFromRegions.emplace_back(region);
            localHeap_->GetSlotSpace()->DecreaseCommitted(region->GetCapacity());
        } else {
            // fixme: Full alived region do not need to sweep
            SlotFreeListMetaInfo freeList = SweepRegionImpl(region);
            if (freeList.firstFreeObject_ != nullptr) {
                ASSERT(Region::ObjectAddressToRange(const_cast<FreeObject *>(freeList.firstFreeObject_)) == region);
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
    std::sort(sweepingRegionList_.begin(), sweepingRegionList_.end(), [](Region *first, Region *second) {
        return first->AliveObject() > second->AliveObject();
    });
    sweepState_ = SweepingState::SWEEPING;
    return survivalObjectSize;
}

void CMSRegionChainManager::ConcurrentSweep(const SweepMode sweepMode)
{
    while (true) {
        Region *region = TryTakeSweepingRegion();
        if (region == nullptr) {
            return;
        }

        if (ConcurrentSweepRegion(region) && sweepMode == SweepMode::SWEEP_UNTIL_ONE_FREE_LIST) {
            return;
        }
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
    FreeObject *prevFreeObject = nullptr;
    size_t totalUsableSize = 0;
    auto appendToFreeSegmentsChain = [&prevFreeObject, &totalUsableSize](uintptr_t freeStart, uintptr_t freeEnd) {
        size_t freeSize = freeEnd - freeStart;
        totalUsableSize += freeSize;
        FreeObject *currentFreeObject = FreeObject::Cast(freeStart);
        if (LIKELY(prevFreeObject != nullptr)) {
            currentFreeObject->SetNext(prevFreeObject);
        }
        prevFreeObject = currentFreeObject;
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

    return {prevFreeObject, totalUsableSize};
}

void CMSRegionChainManager::FreeLiveRange(Region *region, uintptr_t freeStart, uintptr_t freeEnd)
{
    size_t freeSize = freeEnd - freeStart;
    ASSERT(freeSize % slotSize_ == 0);
    FreeObject::FillFreeObject(localHeap_, freeStart, freeSize);
    localHeap_->GetSweeper()->ClearRSetInRange(region, freeStart, freeEnd);
}

bool CMSRegionChainManager::SaveSweptRegion(Region *region, SlotFreeListMetaInfo maybeFreeList)
{
    if (maybeFreeList.firstFreeObject_ != nullptr) {
        ASSERT(Region::ObjectAddressToRange(const_cast<FreeObject *>(maybeFreeList.firstFreeObject_)) == region);
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