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

#include "common_components/heap/allocator/allocator.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/heap/collector/heuristic_gc_policy.h"
#include "common_components/heap/heap.h"
#include "common_components/tests/test_helper.h"

using namespace common;
namespace common::test {
class HeuristicGCPolicyTest : public common::test::BaseTestWithScope {
protected:
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
        holder_ = ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr);
        scope_ = new ThreadHolder::TryBindMutatorScope(holder_);
    }

    void TearDown() override
    {
        if (scope_ != nullptr) {
            delete scope_;
            scope_ = nullptr;
        }
    }

    ThreadHolder *holder_ {nullptr};
    ThreadHolder::TryBindMutatorScope *scope_ {nullptr};
};

HWTEST_F_L0(HeuristicGCPolicyTest, ShouldRestrainGCOnStartupOrSensitive_Test1)
{
    HeuristicGCPolicy gcPolicy;
    gcPolicy.Init();
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
    gcPolicy.TryHeuristicGC();
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());
}

HWTEST_F_L0(HeuristicGCPolicyTest, ShouldRestrainGCOnStartupOrSensitive_Test2)
{
    HeuristicGCPolicy gcPolicy;
    gcPolicy.Init();
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP);
    EXPECT_TRUE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    auto allocated = theAllocator.GetAllocatedBytes();
    auto param = BaseRuntime::GetInstance()->GetHeapParam();
    auto size = param.heapSize * KB * HeuristicGCPolicy::COLD_STARTUP_PHASE1_GC_THRESHOLD_RATIO;
    for (int i = 0; allocated < size; i++) {
        uintptr_t addr = theAllocator.AllocOldRegion();
        ASSERT_NE(addr, 0);
        allocated = theAllocator.GetAllocatedBytes();
    }

    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP);
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    theAllocator.GetOldSpace().AssembleRecentFull();
    auto& fromSpace = theAllocator.GetFromSpace();
    theAllocator.GetOldSpace().AssembleGarbageCandidates(fromSpace);
    fromSpace.GetFromRegionList().ClearList();
    allocated = theAllocator.GetAllocatedBytes();
    EXPECT_EQ(allocated, 0);
}

HWTEST_F_L0(HeuristicGCPolicyTest, ShouldRestrainGCOnStartupOrSensitive_Test3)
{
    HeuristicGCPolicy gcPolicy;
    gcPolicy.Init();
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_PARTIALLY_FINISH);
    EXPECT_TRUE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    auto allocated = theAllocator.GetAllocatedBytes();
    auto param = BaseRuntime::GetInstance()->GetHeapParam();
    auto size = param.heapSize * KB * HeuristicGCPolicy::COLD_STARTUP_PHASE2_GC_THRESHOLD_RATIO;
    for (int i = 0; allocated < size; i++) {
        uintptr_t addr = theAllocator.AllocOldRegion();
        ASSERT_NE(addr, 0);
        allocated = theAllocator.GetAllocatedBytes();
    }

    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_PARTIALLY_FINISH);
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    theAllocator.GetOldSpace().AssembleRecentFull();
    auto& fromSpace = theAllocator.GetFromSpace();
    theAllocator.GetOldSpace().AssembleGarbageCandidates(fromSpace);
    fromSpace.GetFromRegionList().ClearList();
    allocated = theAllocator.GetAllocatedBytes();
    EXPECT_EQ(allocated, 0);
}

HWTEST_F_L0(HeuristicGCPolicyTest, NotifyNativeAllocation)
{
    HeuristicGCPolicy gcPolicy;
    size_t initialNotified = gcPolicy.GetNotifiedNativeSize();
    size_t initialObjects = gcPolicy.GetNativeHeapThreshold();

    gcPolicy.NotifyNativeAllocation(NATIVE_IMMEDIATE_THRESHOLD / 2);

    EXPECT_EQ(gcPolicy.GetNotifiedNativeSize(), initialNotified + NATIVE_IMMEDIATE_THRESHOLD / 2);
    EXPECT_NE(gcPolicy.GetNativeHeapThreshold(), initialObjects + 1);
}

HWTEST_F_L0(HeuristicGCPolicyTest, NotifyNativeAllocation_TriggerByBytes)
{
    HeuristicGCPolicy gcPolicy;
    size_t initialNotified = gcPolicy.GetNotifiedNativeSize();
    size_t initialObjects = gcPolicy.GetNativeHeapThreshold();

    gcPolicy.NotifyNativeAllocation(NATIVE_IMMEDIATE_THRESHOLD + 1);

    EXPECT_EQ(gcPolicy.GetNotifiedNativeSize(), initialNotified + NATIVE_IMMEDIATE_THRESHOLD + 1);
    EXPECT_NE(gcPolicy.GetNativeHeapThreshold(), initialObjects + 1);
}

HWTEST_F_L0(HeuristicGCPolicyTest, TryHeuristicGC)
{
    HeuristicGCPolicy gcPolicy;
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
    Heap::GetHeap().GetCollector().GetGCStats().heapThreshold = 0;
    gcPolicy.TryHeuristicGC();
    Heap::GetHeap().GetCollector().GetGCStats().shouldRequestYoung = true;
    gcPolicy.TryHeuristicGC();
    EXPECT_EQ(Heap::GetHeap().GetAllocator().GetAllocatedBytes(),
        Heap::GetHeap().GetCollector().GetGCStats().GetThreshold());
}

HWTEST_F_L0(HeuristicGCPolicyTest, ChangeGCParams)
{
    HeuristicGCPolicy gcPolicy;
    gcPolicy.RecordAliveSizeAfterLastGC(1);
    gcPolicy.ChangeGCParams(true);
    EXPECT_EQ(Heap::GetHeap().GetAllocator().GetAllocatedBytes(), 0);
}

HWTEST_F_L0(HeuristicGCPolicyTest, CheckAndTriggerHintGC)
{
    HeuristicGCPolicy gcPolicy;
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP);
    bool result = gcPolicy.CheckAndTriggerHintGC(MemoryReduceDegree::HIGH);
    ASSERT_FALSE(result);

    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
    result = gcPolicy.CheckAndTriggerHintGC(MemoryReduceDegree::HIGH);
    ASSERT_FALSE(result);

    gcPolicy.RecordAliveSizeAfterLastGC(1);
    result = gcPolicy.CheckAndTriggerHintGC(MemoryReduceDegree::HIGH);
    ASSERT_FALSE(result);

    result = gcPolicy.CheckAndTriggerHintGC(MemoryReduceDegree::LOW);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(HeuristicGCPolicyTest, NotifyNativeAllocation_TriggerByBytes1)
{
    HeuristicGCPolicy gcPolicy;
    size_t initialNotified = gcPolicy.GetNotifiedNativeSize();
    size_t initialObjects = gcPolicy.GetNativeHeapThreshold();

    gcPolicy.SetNativeHeapThreshold(1);
    gcPolicy.NotifyNativeAllocation(NATIVE_IMMEDIATE_THRESHOLD + 1);

    EXPECT_EQ(gcPolicy.GetNotifiedNativeSize(), initialNotified + NATIVE_IMMEDIATE_THRESHOLD + 1);
    EXPECT_NE(gcPolicy.GetNativeHeapThreshold(), initialObjects + 1);
}
} // namespace common::test