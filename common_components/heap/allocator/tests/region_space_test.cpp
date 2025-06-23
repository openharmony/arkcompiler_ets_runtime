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

#include "common_interfaces/heap/heap_allocator.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/heap/collector/trace_collector.h"
#include "common_components/heap/heap.cpp"
#include "common_components/base_runtime/base_runtime_param.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/tests/test_helper.h"
#include <cstdint>

using namespace common;

namespace common::test {
class RegionSpaceTest : public common::test::BaseTestWithScope {
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

HWTEST_F_L0(RegionSpaceTest, ShouldRetryAllocation)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    ThreadLocal::SetThreadType(ThreadType::GC_THREAD);
    Heap::GetHeap().EnableGC(false);
    Heap::GetHeap().GetCollectorResources().SetGcStarted(true);
    Allocator *regionSpace = RegionSpace::CreateAllocator();
    EXPECT_EQ(regionSpace->Allocate(16, AllocType::MOVEABLE_OBJECT), 0);
}

HWTEST_F_L0(RegionSpaceTest, FeedHungryBuffers2)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    RegionManager& manager = theAllocator.GetRegionManager();
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    manager.Initialize(1024, reinterpret_cast<uintptr_t>(regionMemory_));
    AllocationBuffer* buffer1 = new (std::nothrow) AllocationBuffer();
    AllocationBuffer* buffer2 = new (std::nothrow) AllocationBuffer();
    RegionDesc* Region = RegionDesc::InitRegion(0, 1, RegionDesc::UnitRole::SMALL_SIZED_UNITS);
    buffer1->SetPreparedRegion(Region);

    Heap::GetHeap().GetAllocator().AddHungryBuffer(*buffer1);
    Heap::GetHeap().GetAllocator().AddHungryBuffer(*buffer2);
    Heap::GetHeap().GetAllocator().FeedHungryBuffers();
    EXPECT_NE(buffer2->GetPreparedRegion(), nullptr);
}

HWTEST_F_L0(RegionSpaceTest, FeedHungryBuffers3)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    Heap::GetHeap().GetAllocator().FeedHungryBuffers();
    AllocBufferManager::HungryBuffers hungryBuffers;
    Heap::GetHeap().GetAllocator().SwapHungryBuffers(hungryBuffers);
    EXPECT_EQ(hungryBuffers.size(), 0);
}


HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseEnum)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseMark)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseRemarkStab)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhasePostMark)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_POST_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhasePrecopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseCopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_COPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseFix)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetFixLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseUndef)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseEnum)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseMark)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseRemarkStab)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhasePostMark)
{
    ASSERT_NE(mutator_, nullptr);
    mutator_->SetMutatorPhase(GCPhase::GC_PHASE_POST_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhasePrecopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseCopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_COPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseFix)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetFixLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseUndef)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion2)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    RegionDesc* region = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetFixLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion3)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_COPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    RegionDesc* region = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    RegionDesc* region1 = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region1->GetCopyLine(), region1->GetRegionStart());
    EXPECT_EQ(region1->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion4)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    RegionDesc* region = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region->GetTraceLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region->GetFixLine(), std::numeric_limits<uintptr_t>::max());

    mutator->SetMutatorPhase(GCPhase::GC_PHASE_MARK);
    RegionDesc* region2 = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region2->GetTraceLine(), region2->GetRegionStart());
    EXPECT_EQ(region2->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region2->GetFixLine(), std::numeric_limits<uintptr_t>::max());

    mutator->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);
    RegionDesc* region3 = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region3->GetTraceLine(), region3->GetRegionStart());
    EXPECT_EQ(region3->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region3->GetFixLine(), std::numeric_limits<uintptr_t>::max());

    mutator->SetMutatorPhase(GCPhase::GC_PHASE_POST_MARK);
    RegionDesc* region4 = theAllocator.AllocateThreadLocalRegion(false);
    EXPECT_EQ(region4->GetTraceLine(), region4->GetRegionStart());
    EXPECT_EQ(region4->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
    EXPECT_EQ(region4->GetFixLine(), std::numeric_limits<uintptr_t>::max());
}
}
