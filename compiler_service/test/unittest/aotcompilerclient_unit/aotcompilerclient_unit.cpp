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
class AotCompilerClientTest : public testing::Test {
public:
    AotCompilerClientTest() {}
    virtual ~AotCompilerClientTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override;
    sptr<IRemoteObject> GetAotRemoteObject(AotCompilerClient &aotClient);
};

void AotCompilerClientTest::TearDown()
{
    sptr<ISystemAbilityManager> samgr =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr != nullptr) {
        (void)samgr->UnloadSystemAbility(AOT_COMPILER_SERVICE_ID);
    }
}

sptr<IRemoteObject> AotCompilerClientTest::GetAotRemoteObject(AotCompilerClient &aotClient)
{
    (void)aotClient.GetAotCompilerProxy();
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        return nullptr;
    }
    return systemAbilityMgr->CheckSystemAbility(AOT_COMPILER_SERVICE_ID);
}

/**
 * @tc.name: AotCompilerClientTest_001
 * @tc.desc: AotCompilerClient::GetInstance()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_001, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    EXPECT_NE(aotClient.aotCompilerDiedRecipient_, nullptr);

    auto &client_1 = AotCompilerClient::GetInstance();
    auto &client_2 = AotCompilerClient::GetInstance();
    EXPECT_EQ(&client_1, &client_2);
    EXPECT_EQ(client_1.aotCompilerDiedRecipient_, client_2.aotCompilerDiedRecipient_);
}

/**
 * @tc.name: AotCompilerClientTest_002
 * @tc.desc: invoke aot_compiler service in client program.
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_002, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;
    int32_t ret = aotClient.AotCompiler(argsMap, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
    EXPECT_TRUE(argsMap.empty());
    EXPECT_TRUE(sigData.empty());
}

/**
 * @tc.name: AotCompilerClientTest_003
 * @tc.desc: LoadSystemAbility(AOT_COMPILER_SERVICE_ID, loadCallback)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_003, TestSize.Level0)
{
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(systemAbilityMgr, nullptr);

    sptr<AotCompilerLoadCallback> loadCallback = new (std::nothrow) AotCompilerLoadCallback();
    EXPECT_NE(loadCallback, nullptr);

    auto ret = systemAbilityMgr->LoadSystemAbility(AOT_COMPILER_SERVICE_ID, loadCallback);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerClientTest_004
 * @tc.desc: aotClient.LoadAotCompilerService()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_004, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    aotClient.SetAotCompiler(nullptr);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);

    bool retLoad = aotClient.LoadAotCompilerService();
    EXPECT_TRUE(retLoad);
    EXPECT_NE(aotClient.GetAotCompiler(), nullptr);
}

/**
 * @tc.name: AotCompilerClientTest_005
 * @tc.desc: aotClient.GetAotCompilerProxy()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_005, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    sptr<IRemoteObject> remoteObject = GetAotRemoteObject(aotClient);
    EXPECT_NE(aotClient.GetAotCompilerProxy(), nullptr);

    sptr<IAotCompilerInterface> aotCompilerProxy_check = iface_cast<IAotCompilerInterface>(remoteObject);
    EXPECT_NE(aotCompilerProxy_check, nullptr);
}

/**
 * @tc.name: AotCompilerClientTest_006
 * @tc.desc: aotClient.StopAotCompiler()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_006, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    int32_t ret = aotClient.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);
}

/**
 * @tc.name: AotCompilerClientTest_007
 * @tc.desc: aotClient.loadSaFinished_
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_007, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    auto aotCompilerProxy = aotClient.GetAotCompilerProxy();
    EXPECT_TRUE(aotClient.loadSaFinished_);
    EXPECT_NE(aotClient.GetAotCompiler(), nullptr);
}

/**
 * @tc.name: AotCompilerClientTest_008
 * @tc.desc: aotClient.AotCompilerOnRemoteDied()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_008, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    sptr<IRemoteObject> remoteObject = GetAotRemoteObject(aotClient);
    EXPECT_NE(aotClient.GetAotCompiler(), nullptr);

    wptr<IRemoteObject> remoteObject_weak = remoteObject;
    aotClient.AotCompilerOnRemoteDied(remoteObject_weak);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);
}

/**
 * @tc.name: AotCompilerClientTest_009
 * @tc.desc: callback.OnLoadSystemAbilitySuccess()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_009, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    AotCompilerLoadCallback callback;
    sptr<IRemoteObject> remoteObject = GetAotRemoteObject(aotClient);
    aotClient.SetAotCompiler(nullptr);

    callback.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID, nullptr);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);

    callback.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID - 1, remoteObject);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);

    callback.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID + 1, remoteObject);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);

    callback.OnLoadSystemAbilitySuccess(AOT_COMPILER_SERVICE_ID, remoteObject);
    EXPECT_NE(aotClient.GetAotCompiler(), nullptr);
}

/**
 * @tc.name: AotCompilerClientTest_010
 * @tc.desc: callback.OnLoadSystemAbilityFail()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_010, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    AotCompilerLoadCallback callback;
    sptr<IRemoteObject> remoteObject = GetAotRemoteObject(aotClient);
    aotClient.SetAotCompiler(remoteObject);

    callback.OnLoadSystemAbilityFail(AOT_COMPILER_SERVICE_ID - 1);
    EXPECT_NE(aotClient.GetAotCompiler(), nullptr);

    callback.OnLoadSystemAbilityFail(AOT_COMPILER_SERVICE_ID + 1);
    EXPECT_NE(aotClient.GetAotCompiler(), nullptr);

    callback.OnLoadSystemAbilityFail(AOT_COMPILER_SERVICE_ID);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);
}

/**
 * @tc.name: AotCompilerClientTest_011
 * @tc.desc: aotClient.NeedReCompile(oldVersion, sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_011, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    std::string oldVersion = "4.0.0.0";
    bool sigData = false;
    int32_t ret = aotClient.NeedReCompile(oldVersion, sigData);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerClientTest_012
 * @tc.desc: aotClient.GetAOTVersion(sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_012, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    std::string sigData = "test_string";
    int32_t ret = aotClient.GetAOTVersion(sigData);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_STRNE(sigData.c_str(), "test_string");
}

/**
 * @tc.name: AotCompilerClientTest_013
 * @tc.desc: aotClient.OnRemoteDied(const wptr<IRemoteObject> &remote)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerClientTest, AotCompilerClientTest_013, TestSize.Level0)
{
    AotCompilerClient &aotClient = AotCompilerClient::GetInstance();
    aotClient.aotCompilerDiedRecipient_->OnRemoteDied(nullptr);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);

    wptr<IRemoteObject> remoteObject_weak = GetAotRemoteObject(aotClient);
    EXPECT_NE(remoteObject_weak, nullptr);
    aotClient.aotCompilerDiedRecipient_->OnRemoteDied(remoteObject_weak);
    EXPECT_EQ(aotClient.GetAotCompiler(), nullptr);
}
} // namespace OHOS::ArkCompiler