/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include <string>
#include <vector>

#define private public
#define protected public
#include "aot_compiler_client.h"
#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_load_callback.h"
#undef protected
#undef private
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {
namespace {
const std::string TASK_ID = "UnLoadSA";
constexpr int32_t NO_DELAY_TIME = 0;
constexpr int32_t SLEEP_TIME_FOR_WAITTING_UNLOAD_SA = 5 * 1000; // 5 ms
}

class AotCompilerServiceTest : public testing::Test {
public:
    AotCompilerServiceTest() {}
    virtual ~AotCompilerServiceTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
    void OnStart(AotCompilerService &aotService);
};

void AotCompilerServiceTest::OnStart(AotCompilerService &aotService)
{
    if (aotService.state_ == ServiceRunningState::STATE_RUNNING) {
        return;
    }
    if (!aotService.Init()) {
        return;
    }
    aotService.state_ = ServiceRunningState::STATE_RUNNING;
    aotService.RegisterPowerDisconnectedListener();
}

/**
 * @tc.name: AotCompilerServiceTest_001
 * @tc.desc: AotCompilerService instance initialization
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_001, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
}

/**
 * @tc.name: AotCompilerServiceTest_002
 * @tc.desc: Init() in AotCompilerService
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_002, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_TRUE(aotService.Init());
    EXPECT_NE(aotService.unLoadHandler_, nullptr);
}

/**
 * @tc.name: AotCompilerServiceTest_003
 * @tc.desc: RegisterPowerDisconnectedListener() && UnRegisterPowerDisconnectedListener()
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_003, TestSize.Level0)
{
    AotCompilerService aotService;
    aotService.RegisterPowerDisconnectedListener();
    EXPECT_TRUE(aotService.isPowerEventSubscribered_);

    aotService.UnRegisterPowerDisconnectedListener();
    EXPECT_FALSE(aotService.isPowerEventSubscribered_);
}

/**
 * @tc.name: AotCompilerServiceTest_004
 * @tc.desc: OnStart()/OnStop() in AotCompilerService
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_004, TestSize.Level0)
{
    AotCompilerService aotService;
    OnStart(aotService);
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_RUNNING);

    aotService.OnStop();
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
}

/**
 * @tc.name: AotCompilerServiceTest_005
 * @tc.desc: DelayUnloadTask(TASK_ID, DELAY_TIME)
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_005, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_TRUE(aotService.Init());
    EXPECT_NE(aotService.unLoadHandler_, nullptr);

    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    EXPECT_NE(aotClient.GetAotCompilerProxy(), nullptr);
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(systemAbilityMgr, nullptr);
    auto remoteObject = systemAbilityMgr->CheckSystemAbility(AOT_COMPILER_SERVICE_ID);
    EXPECT_NE(remoteObject, nullptr);

    aotService.DelayUnloadTask(TASK_ID, NO_DELAY_TIME);
    usleep(SLEEP_TIME_FOR_WAITTING_UNLOAD_SA);
    aotService.OnStop();
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);
}
} // namespace OHOS::ArkCompiler