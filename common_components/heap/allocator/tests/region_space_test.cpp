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

#include "common_components/heap/allocator/region_desc.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/heap/allocator/region_space.cpp"
#include "common_components/heap/collector/collector_resources.h"
#include "common_components/tests/test_helper.h"
#include "common_interfaces/base_runtime.h"

using namespace common;

namespace common::test {
class RegionSpaceTest : public common::test::BaseTestWithScope {
    void SetUp() override
    {
        BaseRuntime::GetInstance()->Init();
        holder_ = ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr);
        scope_ = new ThreadHolder::TryBindMutatorScope(holder_);
    }

    void TearDown() override
    {
        if (scope_ != nullptr) {
            delete scope_;
            scope_ = nullptr;
        }

        BaseRuntime::GetInstance()->Fini();
    }

    ThreadHolder *holder_ {nullptr};
    ThreadHolder::TryBindMutatorScope *scope_ {nullptr};
};

HWTEST_F_L0(RegionSpaceTest, FeedHungryBuffers2)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    AllocationBuffer* buffer1 = new (std::nothrow) AllocationBuffer();
    AllocationBuffer* buffer2 = new (std::nothrow) AllocationBuffer();
    RegionDesc* region = RegionDesc::InitRegion(0, 1, RegionDesc::UnitRole::LARGE_SIZED_UNITS);
    region->InitFreeUnits();
    buffer1->SetPreparedRegion(region);
    buffer2->SetPreparedRegion(region);

    Heap::GetHeap().GetAllocator().AddHungryBuffer(*buffer1);
    Heap::GetHeap().GetAllocator().AddHungryBuffer(*buffer2);
    Heap::GetHeap().GetAllocator().FeedHungryBuffers();
    EXPECT_NE(buffer2->GetPreparedRegion(), nullptr);
    delete buffer1;
    delete buffer2;
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
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseMark)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseRemarkStab)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhasePostMark)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_POST_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhasePrecopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseCopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_COPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseFix)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocRegion_PhaseUndef)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseEnum)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseMark)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseRemarkStab)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhasePostMark)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_POST_MARK);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhasePrecopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseCopy)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_COPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseFix)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocPinnedRegion_PhaseUndef)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocPinnedRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetAliveRegionDescAt(addr);
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion2)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_FIX);
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    RegionDesc* region = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion3)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_COPY);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    RegionDesc* region = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region->GetCopyLine(), region->GetRegionStart());
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    RegionDesc* region1 = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region1->GetCopyLine(), region1->GetRegionStart());
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion4)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
    RegionDesc* region = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region->GetMarkingLine(), region->GetRegionStart());
    EXPECT_EQ(region->GetCopyLine(), std::numeric_limits<uintptr_t>::max());

    mutator->SetMutatorPhase(GCPhase::GC_PHASE_MARK);
    RegionDesc* region2 = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region2->GetMarkingLine(), region2->GetRegionStart());
    EXPECT_EQ(region2->GetCopyLine(), std::numeric_limits<uintptr_t>::max());

    mutator->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);
    RegionDesc* region3 = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region3->GetMarkingLine(), region3->GetRegionStart());
    EXPECT_EQ(region3->GetCopyLine(), std::numeric_limits<uintptr_t>::max());

    mutator->SetMutatorPhase(GCPhase::GC_PHASE_POST_MARK);
    RegionDesc* region4 = theAllocator.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_EQ(region4->GetMarkingLine(), region4->GetRegionStart());
    EXPECT_EQ(region4->GetCopyLine(), std::numeric_limits<uintptr_t>::max());
}

HWTEST_F_L0(RegionSpaceTest, CopyRegion)
{
    auto* mutator = common::Mutator::GetMutator();
    mutator->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    ASSERT(region->IsFromRegion());
    theAllocator.CopyRegion(region);
    EXPECT_EQ(theAllocator.FromSpaceSize(), 0);
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion1_NotGcThread_EntersElseBranch)
{
    auto& heapAllocator = Heap::GetHeap().GetAllocator();
    RegionSpace& regionSpace = reinterpret_cast<RegionSpace&>(heapAllocator);

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);

    auto* region = regionSpace.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_NE(region, nullptr);
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion2_NotGcThread_EntersElseBranch)
{
    auto& heapAllocator = Heap::GetHeap().GetAllocator();
    RegionSpace& regionSpace = reinterpret_cast<RegionSpace&>(heapAllocator);

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);

    auto* region = regionSpace.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_NE(region, nullptr);
}

HWTEST_F_L0(RegionSpaceTest, AllocateThreadLocalRegion3_NotGcThread_EntersElseBranch)
{
    auto& heapAllocator = Heap::GetHeap().GetAllocator();
    RegionSpace& regionSpace = reinterpret_cast<RegionSpace&>(heapAllocator);

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_FIX);

    auto* region = regionSpace.AllocateThreadLocalRegion<AllocBufferType::OLD>(false);
    EXPECT_NE(region, nullptr);
}

HWTEST_F_L0(RegionSpaceTest, Allocate_ValidSize_ReturnsNonNull)
{
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);

    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->InitFreeUnits();
    region->SetRegionType(RegionDesc::RegionType::THREAD_LOCAL_REGION);
    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    Heap::GetHeap().EnableGC(true);
    Heap::GetHeap().GetCollectorResources().SetGcStarted(false);

    uintptr_t result = theAllocator.Allocate(16, AllocType::PINNED_OBJECT);
    EXPECT_NE(result, 0u);
}

HWTEST_F_L0(RegionSpaceTest, FeedHungryBuffers_ShouldProvideValidRegions)
{
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);

    AllocationBuffer* buffer1 = new (std::nothrow) AllocationBuffer();
    AllocationBuffer* buffer2 = new (std::nothrow) AllocationBuffer();
    ASSERT_NE(buffer1, nullptr);
    ASSERT_NE(buffer2, nullptr);

    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    ASSERT_NE(region, nullptr);
    region->InitFreeUnits();
    region->SetRegionType(RegionDesc::RegionType::THREAD_LOCAL_REGION);

    buffer1->SetPreparedRegion(region);
    buffer2->SetPreparedRegion(region);
    Heap::GetHeap().GetAllocator().AddHungryBuffer(*buffer1);
    Heap::GetHeap().GetAllocator().AddHungryBuffer(*buffer2);

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_FIX);

    Heap::GetHeap().GetAllocator().FeedHungryBuffers();

    EXPECT_NE(buffer2->GetPreparedRegion(), nullptr);
    delete buffer1;
    delete buffer2;
}

HWTEST_F_L0(RegionSpaceTest, AllocationBuffer_AllocateRawPointerObject_ValidSize_ReturnsNonNull)
{
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    uintptr_t addr = theAllocator.AllocOldRegion();
    ASSERT_NE(addr, 0);

    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    ASSERT_NE(region, nullptr);
    region->InitFreeUnits();
    region->SetRegionType(RegionDesc::RegionType::THREAD_LOCAL_REGION);

    AllocationBuffer* buffer = new (std::nothrow) AllocationBuffer();
    ASSERT_NE(buffer, nullptr);
    buffer->SetPreparedRegion(region);

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_UNDEF);
    Heap::GetHeap().EnableGC(true);
    Heap::GetHeap().GetCollectorResources().SetGcStarted(false);

    uintptr_t result = theAllocator.Allocate(16, AllocType::PINNED_OBJECT);
    EXPECT_NE(result, 0u);
    delete buffer;
}
}
