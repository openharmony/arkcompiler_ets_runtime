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

#include "common_components/heap/collector/heuristic_gc_policy.h"
#include "common_components/tests/test_helper.h"

using namespace common;
namespace common::test {
class HeuristicGCPolicyTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(HeuristicGCPolicyTest, ShouldRestrainGCOnStartupOrSensitive)
{
    HeuristicGCPolicy gcPolicy;
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP);
    EXPECT_TRUE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());
    gcPolicy.TryHeuristicGC();

    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());

    gcPolicy.Init();
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());
    StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_PARTIALLY_FINISH);
    EXPECT_FALSE(gcPolicy.ShouldRestrainGCOnStartupOrSensitive());
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

HWTEST_F_L0(HeuristicGCPolicyTest, NotifyNativeAllocation_TriggerByBytes) {
    HeuristicGCPolicy gcPolicy;
    size_t initialNotified = gcPolicy.GetNotifiedNativeSize();
    size_t initialObjects = gcPolicy.GetNativeHeapThreshold();

    gcPolicy.NotifyNativeAllocation(NATIVE_IMMEDIATE_THRESHOLD + 1);

    EXPECT_EQ(gcPolicy.GetNotifiedNativeSize(), initialNotified + NATIVE_IMMEDIATE_THRESHOLD + 1);
    EXPECT_NE(gcPolicy.GetNativeHeapThreshold(), initialObjects + 1);
}
} // namespace common::test