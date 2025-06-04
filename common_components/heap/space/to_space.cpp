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

#include "common_components/heap/space/to_space.h"
#include "common_components/heap/space/mature_space.h"
#if defined(COMMON_SANITIZER_SUPPORT)
#include "common_components/base/asan_interface.h"
#endif

namespace common {
void ToSpace::DumpRegionStats() const
{
    size_t fullToRegions = fullToRegionList_.GetRegionCount();
    size_t fullToUnits = fullToRegionList_.GetUnitCount();
    size_t fullToSize = fullToUnits * RegionDesc::UNIT_SIZE;
    size_t allocfullToSize = fullToRegionList_.GetAllocatedSize();

    size_t tlToRegions = tlToRegionList_.GetRegionCount();
    size_t tlToUnits = tlToRegionList_.GetUnitCount();
    size_t tlToSize = tlToUnits * RegionDesc::UNIT_SIZE;
    size_t allocTLToSize = tlToRegionList_.GetAllocatedSize();

    VLOG(REPORT, "\tfull to-regions %zu: %zu units (%zu B, alloc %zu)",
        fullToRegions,  fullToUnits, fullToSize, allocfullToSize);
    VLOG(REPORT, "\tthread-local to-regions %zu: %zu units (%zu B, alloc %zu)",
        fullToRegions,  fullToUnits, fullToSize, allocTLToSize);
}

void ToSpace::GetPromotedTo(MatureSpace& mspace)
{
    mspace.PromoteRegionList(fullToRegionList_);
    mspace.PromoteRegionList(tlToRegionList_);
}
} // namespace common
