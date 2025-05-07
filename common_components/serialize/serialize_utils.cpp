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

#include "common_components/serialize/serialize_utils.h"

#include "common_components/common_runtime/src/heap/allocator/region_desc.h"

namespace panda {
SerializedObjectSpace SerializeUtils::GetSerializeObjectSpace(uintptr_t obj)
{
    RegionDesc *info = RegionDesc::GetRegionDescAt(obj);
    RegionDesc::RegionType type = info->GetRegionType();
    switch (type) {
        case RegionDesc::RegionType::THREAD_LOCAL_REGION:
        case RegionDesc::RegionType::RECENT_FULL_REGION:
        case RegionDesc::RegionType::FROM_REGION:
        case RegionDesc::RegionType::LONE_FROM_REGION:
        case RegionDesc::RegionType::EXEMPTED_FROM_REGION:
        case RegionDesc::RegionType::TO_REGION:
            return SerializedObjectSpace::REGULAR_SPACE;
        case RegionDesc::RegionType::FULL_PINNED_REGION:
        case RegionDesc::RegionType::RECENT_PINNED_REGION:
            return SerializedObjectSpace::PIN_SPACE;
        case RegionDesc::RegionType::RECENT_LARGE_REGION:
        case RegionDesc::RegionType::OLD_LARGE_REGION:
            return SerializedObjectSpace::LARGE_SPACE;
        default:
            return SerializedObjectSpace::OTHER;
    }
}

}  // namespace panda