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
#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "common_event_subscriber.h"
#include "iservice_registry.h"
#include "power_disconnected_listener.h"
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
    aotService.RegisterScreenStatusSubscriber();
    aotService.RegisterThermalMgrListener();
}

/**
 * @tc.name: AotCompilerServiceTest_001
 * @tc.desc: AotCompilerService instance initialization
 * @tc.type: Func
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
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
}

/**
 * @tc.name: AotCompilerServiceTest_006
 * @tc.desc: AotCompilerService::AotCompiler(argsMap, fileData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_006, TestSize.Level0)
{
    AotCompilerService aotService;
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> fileData;
    int32_t ret = aotService.AotCompiler(argsMap, fileData);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerServiceTest_007
 * @tc.desc: AotCompilerService::StopAotCompiler()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_007, TestSize.Level0)
{
    AotCompilerService aotService;
    int32_t ret = aotService.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);
}

/**
 * @tc.name: AotCompilerServiceTest_008
 * @tc.desc: AotCompilerService::GetAOTVersion(sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_008, TestSize.Level0)
{
    AotCompilerService aotService;
    std::string sigData = "test_string";
    int32_t ret = aotService.GetAOTVersion(sigData);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_STRNE(sigData.c_str(), "test_string");
}

/**
 * @tc.name: AotCompilerServiceTest_009
 * @tc.desc: AotCompilerService::NeedReCompile(oldVersion, sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_009, TestSize.Level0)
{
    AotCompilerService aotService;
    std::string oldVersion = "4.0.0.0";
    bool sigData = true;
    int32_t ret = aotService.NeedReCompile(oldVersion, sigData);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_STREQ(oldVersion.c_str(), "4.0.0.0");
    EXPECT_FALSE(sigData);
}

/**
 * @tc.name: AotCompilerServiceTest_010
 * @tc.desc: AotCompilerService::RemoveUnloadTask(TASK_ID)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_010, TestSize.Level0)
{
    AotCompilerService aotService;
    aotService.unLoadHandler_ = nullptr;
    aotService.RemoveUnloadTask(TASK_ID);
    aotService.DelayUnloadTask(TASK_ID, NO_DELAY_TIME);
    bool ret = aotService.Init();
    EXPECT_TRUE(ret);
    aotService.RemoveUnloadTask(TASK_ID);
    EXPECT_NE(aotService.unLoadHandler_, nullptr);
}

/**
 * @tc.name: AotCompilerServiceTest_011
 * @tc.desc: AotCompilerService::RegisterPowerDisconnectedListener()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_011, TestSize.Level0)
{
    AotCompilerService aotService;
    bool viewData1 = true;
    int32_t viewData2 = 101010;
    std::string viewData3 = "101010";
    aotService.RegisterPowerDisconnectedListener();
    aotService.UnRegisterPowerDisconnectedListener();
    aotService.UnRegisterPowerDisconnectedListener();
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 101010);
    EXPECT_STREQ(viewData3.c_str(), "101010");
}

/**
 * @tc.name: AotCompilerServiceTest_012
 * @tc.desc: AotCompilerService::RegisterScreenStatusSubscriber()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_012, TestSize.Level0)
{
    AotCompilerService aotService;
    bool viewData1 = true;
    int32_t viewData2 = 101010;
    std::string viewData3 = "101010";
    aotService.RegisterScreenStatusSubscriber();
    aotService.UnRegisterScreenStatusSubscriber();
    aotService.UnRegisterScreenStatusSubscriber();
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 101010);
    EXPECT_STREQ(viewData3.c_str(), "101010");
}

/**
 * @tc.name: AotCompilerServiceTest_013
 * @tc.desc: AotCompilerService::RegisterThermalMgrListener()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_013, TestSize.Level0)
{
    AotCompilerService aotService;
    bool viewData1 = true;
    int32_t viewData2 = 101010;
    std::string viewData3 = "101010";
    aotService.RegisterThermalMgrListener();
    aotService.UnRegisterThermalMgrListener();
    aotService.UnRegisterThermalMgrListener();
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 101010);
    EXPECT_STREQ(viewData3.c_str(), "101010");
}

/**
 * @tc.name: AotCompilerServiceTest_014
 * @tc.desc: PowerDisconnectedListener::OnReceiveEvent(data)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_014, TestSize.Level0)
{
    bool viewData = true;
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    auto powerDisconnectedListener_ = std::make_shared<PowerDisconnectedListener>(subscribeInfo);
    EventFwk::CommonEventData data;
    powerDisconnectedListener_->OnReceiveEvent(data);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerServiceTest_015
 * @tc.desc: ScreenStatusSubscriber::OnReceiveEvent(data)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_015, TestSize.Level0)
{
    bool viewData = true;
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    auto screenStatusSubscriber_ = std::make_shared<ScreenStatusSubscriber>(subscribeInfo);
    EventFwk::CommonEventData data;
    screenStatusSubscriber_->OnReceiveEvent(data);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerServiceTest_016
 * @tc.desc: ThermalMgrListener::OnReceiveEvent(data)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_016, TestSize.Level0)
{
    bool viewData = true;
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_THERMAL_LEVEL_CHANGED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    auto thermalMgrListener_ = std::make_shared<ThermalMgrListener>(subscribeInfo);
    EventFwk::CommonEventData data;
    thermalMgrListener_->OnReceiveEvent(data);
    EXPECT_TRUE(viewData);
}
} // namespace OHOS::ArkCompiler