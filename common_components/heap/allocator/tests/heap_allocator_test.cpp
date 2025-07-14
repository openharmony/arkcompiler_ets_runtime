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

#include <cstdint>

#include "common_interfaces/heap/heap_allocator.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/heap/collector/trace_collector.h"
#include "common_components/heap/heap.cpp"
#include "common_components/common_runtime/base_runtime_param.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class HeapAllocatorTest : public BaseTestWithScope {
    void SetUp() override
    {
        BaseRuntime::GetInstance()->Init();
        holder_ = ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr);
        scope_ = new ThreadHolder::TryBindMutatorScope(holder_);
    }

    void TearDown() override
    {
        delete scope_;
        BaseRuntime::GetInstance()->Fini();
    }

    ThreadHolder *holder_ {nullptr};
    ThreadHolder::TryBindMutatorScope *scope_ {nullptr};
};

HWTEST_F_L0(HeapAllocatorTest, AllocLargeObject)
{
    uintptr_t addr = common::HeapAllocator::Allocate(Heap::NORMAL_UNIT_SIZE, common::LanguageType::DYNAMIC);
    ASSERT(addr > 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    ASSERT(region->IsLargeRegion());
}

HWTEST_F_L0(HeapAllocatorTest, AllocLargeRegion)
{
    uintptr_t addr = common::HeapAllocator::AllocateLargeRegion(Heap::NORMAL_UNIT_SIZE);
    ASSERT(addr > 0);
    RegionDesc *region = RegionDesc::GetAliveRegionDescAt(addr);
    ASSERT(region->IsLargeRegion());
}
}