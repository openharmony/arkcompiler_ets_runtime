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

#include "common_components/base_runtime/base_runtime_param.h"
#include "common_components/heap/allocator/region_desc.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/mutator/satb_buffer.h"
#include "common_components/tests/test_helper.h"
#include <cstdint>

using namespace common;
namespace common::test {

class SatbBufferTest : public BaseTestWithScope {
protected:
    void* regionMemory_;
    size_t totalUnits_ = 1024;
    size_t heapSize_;
    Mutator* mutator_ = nullptr;

    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase()
    {
        BaseRuntime::GetInstance()->Fini();
    }

    void SetUp() override
    {
        heapSize_ = totalUnits_ * RegionDesc::UNIT_SIZE;
        size_t allocSize = heapSize_ + totalUnits_ * sizeof(RegionDesc);
        regionMemory_ = malloc(allocSize);
        ASSERT_NE(regionMemory_, nullptr);
        uintptr_t unitInfoStart = reinterpret_cast<uintptr_t>(regionMemory_);
        RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
        RegionManager& manager = theAllocator.GetRegionManager();
        manager.Initialize(totalUnits_, reinterpret_cast<uintptr_t>(regionMemory_));
        size_t metadataSize = totalUnits_ * sizeof(RegionDesc);
        uintptr_t heapStartAddress = unitInfoStart + metadataSize;
        RegionDesc::Initialize(totalUnits_, unitInfoStart, heapStartAddress);
        mutator_ = Mutator::NewMutator();
        ASSERT_NE(mutator_, nullptr);
        mutator_->InitTid();
        ThreadLocal::GetThreadLocalData()->mutator = mutator_;
    }

    void TearDown() override
    {
        if (mutator_) {
            delete mutator_;
            mutator_ = nullptr;
        }
        if (regionMemory_) {
            free(regionMemory_);
            regionMemory_ = nullptr;
        }
    }
};

HWTEST_F_L0(SatbBufferTest, NullptrReturnsFalse)\
{
    EXPECT_FALSE(SatbBuffer::Instance().ShouldEnqueue(nullptr));
}

HWTEST_F_L0(SatbBufferTest, IsYoungSpaceObject1)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FREE_REGION);
    Heap::GetHeap().SetGCReason(GC_REASON_YOUNG);

    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(SatbBuffer::Instance().ShouldEnqueue(obj));
}

HWTEST_F_L0(SatbBufferTest, IsYoungSpaceObject2)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Heap::GetHeap().SetGCReason(GC_REASON_HEU);

    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(SatbBuffer::Instance().ShouldEnqueue(obj));
}

HWTEST_F_L0(SatbBufferTest, IsYoungSpaceObject3)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Heap::GetHeap().SetGCReason(GC_REASON_YOUNG);

    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(SatbBuffer::Instance().ShouldEnqueue(obj));
}

HWTEST_F_L0(SatbBufferTest, IsYoungSpaceObject4)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FREE_REGION);
    Heap::GetHeap().SetGCReason(GC_REASON_HEU);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(SatbBuffer::Instance().ShouldEnqueue(obj));
}
}  // namespace common::test
