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
#include <unordered_map>

#include "aot_compiler_client.h"
#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_impl.h"
#include "aot_compiler_load_callback.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace testing::ext;

namespace OHOS::ArkCompiler {
namespace {
const std::string compilerPkgInfoValue =
    "{\"abcName\":\"ets/modules.abc\","
    "\"abcOffset\":\"0x1000\","
    "\"abcSize\":\"0x60b058\","
    "\"appIdentifier\":\"5765880207853624761\","
    "\"bundleName\":\"com.ohos.contacts\","
    "\"BundleUid\":\"0x1317b6f\","
    "\"isEncryptedBundle\":\"0x0\","
    "\"isScreenOff\":\"0x1\","
    "\"moduleName\":\"entry\","
    "\"pgoDir\":\"/data/local/ark-profile/100/com.ohos.contacts\","
    "\"pkgPath\":\"/system/app/Contacts/Contacts.hap\","
    "\"processUid\":\"0xbf4\"}";

const std::unordered_map<std::string, std::string> argsMapForTest {
    {"target-compiler-mode", "partial"},
    {"aot-file", "/data/local/ark-cache/com.ohos.contacts/arm64/entry"},
    {"compiler-pkg-info", compilerPkgInfoValue},
    {"compiler-external-pkg-info", "[]"},
    {"compiler-opt-bc-range", ""},
    {"compiler-device-state", "1"},
    {"compiler-baseline-pgo", "0"},
    {"ABC-Path", "/system/app/Contacts/Contacts.hap/ets/modules.abc"},
    {"BundleUid", "20020079"},
    {"BundleGid", "20020079"},
    {"anFileName", "/data/local/ark-cache/com.ohos.contacts/arm64/entry.an"},
    {"appIdentifier", "5765880207853624761"}
};
} // namespace ark_aot_compiler arguments

class AotCompilerImplTest : public testing::Test {
public:
    AotCompilerImplTest() {}
    virtual ~AotCompilerImplTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: AotCompilerImplTest_001
 * @tc.desc: AotCompilerImpl::GetInstance()
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_001, TestSize.Level0)
{
    AotCompilerImpl *aotImplPtr = nullptr;
    aotImplPtr = &AotCompilerImpl::GetInstance();
    EXPECT_NE(aotImplPtr, nullptr);
}

/**
 * @tc.name: AotCompilerImplTest_002
 * @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() when compiler-check-pgo-version
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_002, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    std::string keyCheckPgoVersion = "compiler-check-pgo-version";
    std::string valueCheckPgoVersion = "true";
    argsMap.emplace(keyCheckPgoVersion, valueCheckPgoVersion);
    std::vector<int16_t> sigData;
    int32_t ret = aotImpl.EcmascriptAotCompiler(argsMap, sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_NE(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
    EXPECT_TRUE(sigData.empty());
}

/**
 * @tc.name: AotCompilerImplTest_003
 * @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() when compile not any method
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_003, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    std::string keyCompileNoMethod = "compiler-methods-range";
    std::string valueCompileNoMethod = "0:0";
    argsMap.emplace(keyCompileNoMethod, valueCompileNoMethod);
    std::vector<int16_t> sigData { 0, 1, 2, 3, 4, 5 };
    int32_t ret = aotImpl.EcmascriptAotCompiler(argsMap, sigData);
    EXPECT_NE(ret, ERR_OK);
    EXPECT_FALSE(sigData.empty());
}

/**
 * @tc.name: AotCompilerImplTest_004
 * @tc.desc: AotCompilerImpl::StopAotCompiler()
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_004, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    int32_t ret = aotImpl.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_005
 * @tc.desc: AotCompilerImpl::GetAOTVersion(std::string& sigData)
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_005, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    std::string sigData = "sig_data_for_test";
    int32_t ret = aotImpl.GetAOTVersion(sigData);
    EXPECT_EQ(sigData.size(), 0);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerImplTest_006
 * @tc.desc: AotCompilerImpl::NeedReCompile(const std::string& args, bool& sigData)
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_006, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    std::string args = "args_for_test";
    bool sigData = true;
    int32_t ret = aotImpl.NeedReCompile(args, sigData);
    EXPECT_FALSE(sigData);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerImplTest_007
 * @tc.desc: AotCompilerImpl::HandlePowerDisconnected()
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_007, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    bool viewData1 = true;
    int32_t viewData2 = 101010;
    std::string viewData3 = "101010";
    aotImpl.HandlePowerDisconnected();
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 101010);
    EXPECT_STREQ(viewData3.c_str(), "101010");
}

/**
 * @tc.name: AotCompilerImplTest_008
 * @tc.desc: AotCompilerImpl::HandleScreenOn()
 * @tc.type: Func
 * @tc.require: IR/AR/SR
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_008, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImpl::GetInstance();
    bool viewData1 = true;
    int32_t viewData2 = 010101;
    std::string viewData3 = "010101";
    aotImpl.HandleScreenOn();
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 010101);
    EXPECT_STREQ(viewData3.c_str(), "010101");
}
} // namespace OHOS::ArkCompiler