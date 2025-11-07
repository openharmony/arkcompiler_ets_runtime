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

#ifndef ECMASCRIPT_MEM_CMS_MEM_CMS_REGION_CHAIN_MANAGER_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_CMS_REGION_CHAIN_MANAGER_INL_H

#include "ecmascript/mem/cms_mem/cms_region_chain_manager.h"

#include "ecmascript/free_object.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {

void CMSRegionChainManager::AddNewRegion(Region *region)
{
    regionList_.AddNode(region);
}

SlotFreeListMetaInfo CMSRegionChainManager::TryTakeSlotFreeList()
{
    {
        SlotFreeListMetaInfo maybeFreeList = TryTakeUsableSlotFreeList();
        if (LIKELY(sweepState_ == SweepingState::NO_SWEEP)) {
            return maybeFreeList;
        }

        if (maybeFreeList.firstFreeObject_ != nullptr) {
            return maybeFreeList;
        }
    }

    {
        RetrieveSweptSlotFreeLists();
        SlotFreeListMetaInfo maybeFreeList = TryTakeUsableSlotFreeList();
        if (maybeFreeList.firstFreeObject_ != nullptr) {
            return maybeFreeList;
        }
    }

    ConcurrentSweep(SweepMode::SWEEP_UNTIL_ONE_FREE_LIST);
    RetrieveSweptSlotFreeLists();

    return TryTakeUsableSlotFreeList();
}

SlotFreeListMetaInfo CMSRegionChainManager::TryTakeUsableSlotFreeList()
{
    if (!usableSlotFreeList_.empty()) {
        SlotFreeListMetaInfo freeList = usableSlotFreeList_.back();
        usableSlotFreeList_.pop_back();
        return freeList;
    }
    return {nullptr, 0};
}

void CMSRegionChainManager::RetrieveSweptSlotFreeLists()
{
    LockHolder holder(mutex_);
    while (!sweptSlotFreeList_.empty()) {
        SlotFreeListMetaInfo freeList = sweptSlotFreeList_.back();
        sweptSlotFreeList_.pop_back();
        // fixme: move outside lock
        Region *region = Region::ObjectAddressToRange(const_cast<FreeObject *>(freeList.firstFreeObject_));
        region->ResetSwept();
        region->MergeLocalToShareRSetForCS();
        usableSlotFreeList_.emplace_back(freeList);
    }
}

template <typename Callback>
void CMSRegionChainManager::EnumerateRegions(Callback &&cb)
{
    Region *current = regionList_.GetFirst();
    while (current != nullptr) {
        Region *next = current->GetNext();
        cb(current);
        current = next;
    }
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_CMS_REGION_CHAIN_MANAGER_INL_H
