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
#ifndef COMMON_COMPONENTS_HEAP_SPACE_OLD_SPACE_H
#define COMMON_COMPONENTS_HEAP_SPACE_OLD_SPACE_H

#include <assert.h>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include "common_components/heap/allocator/alloc_util.h"
#include "common_components/heap/allocator/allocator.h"
#include "common_components/heap/allocator/region_manager.h"
#include "common_components/heap/space/regional_space.h"
#include "common_components/heap/space/from_space.h"
#include "common_components/heap/space/to_space.h"
#include "common_components/mutator/mutator.h"
#if defined(COMMON_SANITIZER_SUPPORT)
#include "common_components/base/asan_interface.h"
#endif

namespace common {
// regions for small-sized movable objects, which may be moved during gc.
class OldSpace : public RegionalSpace {
public:
    OldSpace(RegionManager& regionManager) : RegionalSpace(regionManager), oldRegionList_("old regions") {}

    void DumpRegionStats() const;

    void FixAllRegions()
    {
        TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
        if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
            RegionManager::FixOldRegionList(collector, oldRegionList_);
        } else {
            RegionManager::FixRegionList(collector, oldRegionList_);
        }
    }

    void AssembleGarbageCandidates(FromSpace& fromSpace)
    {
        fromSpace.AssembleGarbageCandidates(oldRegionList_);
    }

    size_t GetAllocatedSize() const
    {
        return oldRegionList_.GetAllocatedSize();
    }

    size_t GetUsedUnitCount() const
    {
        return oldRegionList_.GetUnitCount();
    }

    void PromoteRegionList(RegionList& list)
    {
        oldRegionList_.MergeRegionList(list, RegionDesc::RegionType::OLD_REGION);
    }

    void ClearRSet()
    {
        RegionDesc* region = oldRegionList_.GetHeadRegion();
        while (region != nullptr) {
            region->ClearRSet();
            region = region->GetNextRegion();
        }
    }

    void ClearAllGCInfo()
    {
        ClearGCInfo(oldRegionList_);
    }

    void VisitRememberSet(const std::function<void(BaseObject*)>& func)
    {
        auto visitFunc = [&func](RegionDesc* region) {
            region->VisitAllObjects([&region, &func](BaseObject* obj) {
                if (region->IsInRSet(obj)) {
                    func(obj);
                }
            });
        };
        oldRegionList_.VisitAllRegions(visitFunc);
    }

private:
    void ClearGCInfo(RegionList& list)
    {
        RegionList tmp("temp region list");
        list.CopyListTo(tmp);
        tmp.VisitAllRegions([](RegionDesc* region) {
            region->ClearTraceCopyFixLine();
            region->ClearLiveInfo();
            region->ResetMarkBit();
        });
    }

    RegionList oldRegionList_;
};
} // namespace common
#endif // COMMON_COMPONENTS_HEAP_SPACE_OLD_SPACE_H
