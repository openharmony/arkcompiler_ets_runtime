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

#include "common_components/heap/heap_proxy.h"

#include "common_components/common_runtime/src/heap/allocator/region_space.h"
#include "common_components/common_runtime/src/heap/heap.h"

namespace panda {
uintptr_t HeapProxy::GetLastRegionForTest()
{
    RegionManager& manager = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator()).GetRegionManager();
    return manager.GetLastRegionForTest();
}

uintptr_t HeapProxy::GetLastLargeRegionForTest()
{
    RegionManager& manager = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator()).GetRegionManager();
    return manager.GetLastLargeRegionForTest();
}
}  // namespace panda