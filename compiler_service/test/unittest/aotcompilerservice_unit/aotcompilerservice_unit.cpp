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
#include "aot_compiler_args.h"
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
    AotCompilerArgs args;
    std::vector<uint8_t> fileData;
    int32_t ret = aotService.AotCompiler(args, fileData);
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
    aotService.RegisterPowerDisconnectedListener();
    EXPECT_TRUE(aotService.IsPowerEventSubscribered());
    aotService.UnRegisterPowerDisconnectedListener();
    EXPECT_FALSE(aotService.IsPowerEventSubscribered());
    aotService.UnRegisterPowerDisconnectedListener();
    EXPECT_FALSE(aotService.IsPowerEventSubscribered());
}

/**
 * @tc.name: AotCompilerServiceTest_012
 * @tc.desc: AotCompilerService::RegisterScreenStatusSubscriber()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_012, TestSize.Level0)
{
    AotCompilerService aotService;
    aotService.RegisterScreenStatusSubscriber();
    EXPECT_TRUE(aotService.IsScreenStatusSubscribered());
    aotService.UnRegisterScreenStatusSubscriber();
    EXPECT_FALSE(aotService.IsScreenStatusSubscribered());
    aotService.UnRegisterScreenStatusSubscriber();
    EXPECT_FALSE(aotService.IsScreenStatusSubscribered());
}

/**
 * @tc.name: AotCompilerServiceTest_013
 * @tc.desc: AotCompilerService::RegisterThermalMgrListener()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_013, TestSize.Level0)
{
    AotCompilerService aotService;
    aotService.RegisterThermalMgrListener();
    EXPECT_TRUE(aotService.IsThermalLevelEventSubscribered());
    aotService.UnRegisterThermalMgrListener();
    EXPECT_FALSE(aotService.IsThermalLevelEventSubscribered());
    aotService.UnRegisterThermalMgrListener();
    EXPECT_FALSE(aotService.IsThermalLevelEventSubscribered());
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

/**
 * @tc.name: AotCompilerServiceTest_017
 * @tc.desc: AotCompilerService initial state: all listener pointers are nullptr
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_017, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_EQ(aotService.powerDisconnectedListener_, nullptr);
    EXPECT_EQ(aotService.screenStatusSubscriber_, nullptr);
    EXPECT_EQ(aotService.thermalMgrListener_, nullptr);
    EXPECT_EQ(aotService.unLoadHandler_, nullptr);
}

/**
 * @tc.name: AotCompilerServiceTest_018
 * @tc.desc: AotCompilerService initial state: all subscription flags are false
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_018, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_FALSE(aotService.isPowerEventSubscribered_);
    EXPECT_FALSE(aotService.isScreenStatusSubscribered_);
    EXPECT_FALSE(aotService.isThermalLevelEventSubscribered_);
    EXPECT_FALSE(aotService.IsPowerEventSubscribered());
    EXPECT_FALSE(aotService.IsScreenStatusSubscribered());
    EXPECT_FALSE(aotService.IsThermalLevelEventSubscribered());
}

/**
 * @tc.name: AotCompilerServiceTest_019
 * @tc.desc: AotCompilerService::AotCompiler with non-empty argsMap returns error
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_019, TestSize.Level0)
{
    AotCompilerService aotService;
    AotCompilerArgs args;
    args.bundleName = "com.example.test";
    args.moduleName = "entry";
    args.anFileName = "/data/test/entry.an";
    std::vector<uint8_t> fileData;
    int32_t ret = aotService.AotCompiler(args, fileData);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerServiceTest_020
 * @tc.desc: AotCompilerService::NeedReCompile with empty version string
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_020, TestSize.Level0)
{
    AotCompilerService aotService;
    std::string oldVersion = "";
    bool sigData = false;
    int32_t ret = aotService.NeedReCompile(oldVersion, sigData);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerServiceTest_021
 * @tc.desc: AotCompilerService::GetAOTVersion returns non-empty version string
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_021, TestSize.Level0)
{
    AotCompilerService aotService;
    std::string sigData;
    int32_t ret = aotService.GetAOTVersion(sigData);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_FALSE(sigData.empty());
}

/**
 * @tc.name: AotCompilerServiceTest_022
 * @tc.desc: OnStart idempotency: calling OnStart when already STATE_RUNNING keeps state RUNNING
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_022, TestSize.Level0)
{
    AotCompilerService aotService;
    OnStart(aotService);
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_RUNNING);
    OnStart(aotService);
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_RUNNING);
    aotService.OnStop();
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
}

/**
 * @tc.name: AotCompilerServiceTest_023
 * @tc.desc: OnStop when service is not started: state stays STATE_NOT_START, no crash
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_023, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
    aotService.OnStop();
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
}

/**
 * @tc.name: AotCompilerServiceTest_024
 * @tc.desc: Init() called twice: unLoadHandler_ is not replaced on second call
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_024, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_TRUE(aotService.Init());
    EXPECT_NE(aotService.unLoadHandler_, nullptr);
    auto firstHandler = aotService.unLoadHandler_;
    EXPECT_TRUE(aotService.Init());
    EXPECT_NE(aotService.unLoadHandler_, nullptr);
    EXPECT_EQ(aotService.unLoadHandler_, firstHandler);
}

/**
 * @tc.name: AotCompilerServiceTest_025
 * @tc.desc: DelayUnloadTask with nullptr unLoadHandler_: no crash
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_025, TestSize.Level0)
{
    AotCompilerService aotService;
    EXPECT_EQ(aotService.unLoadHandler_, nullptr);
    aotService.DelayUnloadTask(TASK_ID, NO_DELAY_TIME);
    EXPECT_EQ(aotService.unLoadHandler_, nullptr);
}

/**
 * @tc.name: AotCompilerServiceTest_026
 * @tc.desc: Multiple register/unregister cycles for PowerDisconnectedListener
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_026, TestSize.Level0)
{
    AotCompilerService aotService;
    for (int i = 0; i < 3; i++) {
        aotService.RegisterPowerDisconnectedListener();
        EXPECT_TRUE(aotService.IsPowerEventSubscribered());
        aotService.UnRegisterPowerDisconnectedListener();
        EXPECT_FALSE(aotService.IsPowerEventSubscribered());
        EXPECT_EQ(aotService.powerDisconnectedListener_, nullptr);
    }
}

/**
 * @tc.name: AotCompilerServiceTest_027
 * @tc.desc: Multiple register/unregister cycles for ScreenStatusSubscriber
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_027, TestSize.Level0)
{
    AotCompilerService aotService;
    for (int i = 0; i < 3; i++) {
        aotService.RegisterScreenStatusSubscriber();
        EXPECT_TRUE(aotService.IsScreenStatusSubscribered());
        aotService.UnRegisterScreenStatusSubscriber();
        EXPECT_FALSE(aotService.IsScreenStatusSubscribered());
        EXPECT_EQ(aotService.screenStatusSubscriber_, nullptr);
    }
}

/**
 * @tc.name: AotCompilerServiceTest_028
 * @tc.desc: Multiple register/unregister cycles for ThermalMgrListener
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_028, TestSize.Level0)
{
    AotCompilerService aotService;
    for (int i = 0; i < 3; i++) {
        aotService.RegisterThermalMgrListener();
        EXPECT_TRUE(aotService.IsThermalLevelEventSubscribered());
        aotService.UnRegisterThermalMgrListener();
        EXPECT_FALSE(aotService.IsThermalLevelEventSubscribered());
        EXPECT_EQ(aotService.thermalMgrListener_, nullptr);
    }
}

/**
 * @tc.name: AotCompilerServiceTest_029
 * @tc.desc: AotCompilerLoadCallback::OnLoadSystemAbilitySuccess with mismatched SA ID
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_029, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerLoadCallback callback;
    sptr<IRemoteObject> remoteObject = nullptr;
    callback.OnLoadSystemAbilitySuccess(0, remoteObject);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerServiceTest_030
 * @tc.desc: AotCompilerLoadCallback::OnLoadSystemAbilitySuccess with nullptr remoteObject
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_030, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerLoadCallback callback;
    callback.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID, nullptr);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerServiceTest_031
 * @tc.desc: AotCompilerLoadCallback::OnLoadSystemAbilityFail with mismatched SA ID
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_031, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerLoadCallback callback;
    callback.OnLoadSystemAbilityFail(0);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerServiceTest_032
 * @tc.desc: AotCompilerLoadCallback::OnLoadSystemAbilityFail with correct SA ID
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_032, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerLoadCallback callback;
    callback.OnLoadSystemAbilityFail(AOT_COMPILER_SERVICE_ID);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerServiceTest_033
 * @tc.desc: AotCompilerService::AotCompiler with pre-populated fileData still returns error
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_033, TestSize.Level0)
{
    AotCompilerService aotService;
    AotCompilerArgs args;
    std::vector<uint8_t> fileData = {1, 2, 3, 4, 5};
    int32_t ret = aotService.AotCompiler(args, fileData);
    EXPECT_NE(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerServiceTest_034
 * @tc.desc: AotCompilerService::NeedReCompile with various version strings returns ERR_OK
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_034, TestSize.Level0)
{
    AotCompilerService aotService;
    std::vector<std::string> versions = {"1.0.0.0", "3.0.0.0", "5.0.0.0", "99.99.99.99"};
    for (const auto &version : versions) {
        bool sigData = false;
        int32_t ret = aotService.NeedReCompile(version, sigData);
        EXPECT_EQ(ret, ERR_OK);
    }
}

/**
 * @tc.name: AotCompilerServiceTest_035
 * @tc.desc: OnStart followed by OnStop restores all listener subscriptions to initial state
 * @tc.type: Func
*/
HWTEST_F(AotCompilerServiceTest, AotCompilerServiceTest_035, TestSize.Level0)
{
    AotCompilerService aotService;
    OnStart(aotService);
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_RUNNING);
    aotService.OnStop();
    EXPECT_EQ(aotService.state_, ServiceRunningState::STATE_NOT_START);
    EXPECT_FALSE(aotService.IsPowerEventSubscribered());
    EXPECT_FALSE(aotService.IsScreenStatusSubscribered());
    EXPECT_FALSE(aotService.IsThermalLevelEventSubscribered());
    EXPECT_EQ(aotService.powerDisconnectedListener_, nullptr);
    EXPECT_EQ(aotService.screenStatusSubscriber_, nullptr);
    EXPECT_EQ(aotService.thermalMgrListener_, nullptr);
}
} // namespace OHOS::ArkCompiler