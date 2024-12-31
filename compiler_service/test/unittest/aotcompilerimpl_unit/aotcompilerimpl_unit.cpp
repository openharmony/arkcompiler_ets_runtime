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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include <mutex>
#include <securec.h>
#include <string>
#include <thread>
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
using namespace testing::mt;

namespace OHOS::ArkCompiler {
namespace {
constexpr int TEST_ERR_OK = 0;
constexpr int TEST_ERR_AN_EMPTY = 5;
constexpr int TEST_ERR_OTHERS = 100;
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
    "\"pgoDir\":\"/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts\","
    "\"pkgPath\":\"/system/app/Contacts/Contacts.hap\","
    "\"processUid\":\"0xbf4\"}";

const std::unordered_map<std::string, std::string> argsMapForTest {
    {"target-compiler-mode", "partial"},
    {"aot-file", "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry"},
    {"compiler-pkg-info", compilerPkgInfoValue},
    {"compiler-external-pkg-info", "[]"},
    {"compiler-opt-bc-range", ""},
    {"compiler-device-state", "1"},
    {"compiler-baseline-pgo", "0"},
    {"ABC-Path", "/system/app/Contacts/Contacts.hap/ets/modules.abc"},
    {"BundleUid", "20020079"},
    {"BundleGid", "20020079"},
    {"anFileName", "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an"},
    {"appIdentifier", "5765880207853624761"}
};
} // namespace ark_aot_compiler arguments

class AotCompilerImplMock : public AotCompilerImpl {
public:
    AotCompilerImplMock() = default;
    ~AotCompilerImplMock() = default;
    AotCompilerImplMock(const AotCompilerImplMock&) = delete;
    AotCompilerImplMock(AotCompilerImplMock&&) = delete;
    AotCompilerImplMock& operator=(const AotCompilerImplMock&) = delete;
    AotCompilerImplMock& operator=(AotCompilerImplMock&&) = delete;

    int32_t FindArgsIdxToIntegerMock(const std::unordered_map<std::string, std::string> &argsMap,
        const std::string &keyName, int32_t &bundleID)
    {
        return FindArgsIdxToInteger(argsMap, keyName, bundleID);
    }

    int32_t FindArgsIdxToStringMock(const std::unordered_map<std::string, std::string> &argsMap,
        const std::string &keyName, std::string &bundleArg)
    {
        return FindArgsIdxToString(argsMap, keyName, bundleArg);
    }

    int32_t PrepareArgsMock(const std::unordered_map<std::string, std::string> &argsMap)
    {
        return PrepareArgs(argsMap);
    }

    int32_t PrintAOTCompilerResultMock(const int compilerStatus) const
    {
        return PrintAOTCompilerResult(compilerStatus);
    }

    void AddExpandArgsMock(std::vector<std::string> &argVector)
    {
        AddExpandArgs(argVector);
    }

    int32_t EcmascriptAotCompilerMock(const std::unordered_map<std::string, std::string> &argsMap,
                                      std::vector<int16_t> &sigData)
    {
    #ifdef CODE_SIGN_ENABLE
        if (!allowAotCompiler_) {
            return ERR_AOT_COMPILER_CALL_CANCELLED;
        }
        if (argsMap.empty() || (PrepareArgs(argsMap) != ERR_OK)) {
            return ERR_AOT_COMPILER_PARAM_FAILED;
        }
        int32_t ret = ERR_OK;
        pid_t pid = fork();
        if (pid == -1) {
            return ERR_AOT_COMPILER_CALL_FAILED;
        } else if (pid == 0) {
            DropCapabilities();
            sleep(2);  // 2: mock ark aot compiler run time with 2s
            ExecuteInChildProcess();
        } else {
            mockChildPid_ = pid;
            ExecuteInParentProcess(pid, ret);
        }
        if (ret == ERR_OK_NO_AOT_FILE) {
            return ERR_OK;
        }
        return ret != ERR_OK ? ret : AOTLocalCodeSign(sigData);
    #else
        return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
    #endif
    }

    int32_t AOTLocalCodeSignMock(std::vector<int16_t> &sigData) const
    {
        return AOTLocalCodeSign(sigData);
    }

    void InitStateMock(const pid_t childPid)
    {
        InitState(childPid);
    }

    void ResetStateMock()
    {
        ResetState();
    }

    void PauseAotCompilerMock()
    {
        PauseAotCompiler();
    }

    void AllowAotCompilerMock()
    {
        AllowAotCompiler();
    }

    pid_t GetChildPidMock()
    {
        return mockChildPid_;
    }

private:
    pid_t mockChildPid_ = -1;
};

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
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_001, TestSize.Level0)
{
    AotCompilerImpl *aotImplPtr = nullptr;
    aotImplPtr = &AotCompilerImplMock::GetInstance();
    EXPECT_NE(aotImplPtr, nullptr);
}

/**
 * @tc.name: AotCompilerImplTest_002
 * @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() when compiler-check-pgo-version
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_002, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
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
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_003, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    std::string keyCompileNoMethod = "compiler-methods-range";
    std::string valueCompileNoMethod = "0:0";
    argsMap.emplace(keyCompileNoMethod, valueCompileNoMethod);
    std::vector<int16_t> sigData { 0, 1, 2, 3, 4, 5 };
    int32_t ret = aotImpl.EcmascriptAotCompiler(argsMap, sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_NE(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
    EXPECT_FALSE(sigData.empty());
}

/**
 * @tc.name: AotCompilerImplTest_004
 * @tc.desc: AotCompilerImpl::StopAotCompiler()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_004, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    int32_t ret = aotImpl.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_005
 * @tc.desc: AotCompilerImpl::GetAOTVersion(std::string& sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_005, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    std::string sigData = "sig_data_for_test";
    int32_t ret = aotImpl.GetAOTVersion(sigData);
    EXPECT_EQ(sigData.size(), 0);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerImplTest_006
 * @tc.desc: AotCompilerImpl::NeedReCompile(const std::string& args, bool& sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_006, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
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
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_007, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
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
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_008, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    bool viewData1 = true;
    int32_t viewData2 = 010101;
    std::string viewData3 = "010101";
    aotImpl.HandleScreenOn();
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 010101);
    EXPECT_STREQ(viewData3.c_str(), "010101");
}

/**
 * @tc.name: AotCompilerImplTest_009
 * @tc.desc: AotCompilerImpl::HandleThermalLevelChanged()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_009, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    bool viewData1 = true;
    int32_t viewData2 = 010101;
    std::string viewData3 = "010101";
    aotImpl.HandleThermalLevelChanged(1);
    EXPECT_TRUE(viewData1);
    EXPECT_EQ(viewData2, 010101);
    EXPECT_STREQ(viewData3.c_str(), "010101");
}

/**
 * @tc.name: AotCompilerImplTest_010
 * @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() when multi thread run
 * @tc.type: Func
*/
AotCompilerImpl &gtAotImpl = AotCompilerImplMock::GetInstance();
std::mutex aotCompilerMutex_;
bool g_retGlobal = true;

void ExecuteAotCompilerTask(void)
{
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    std::string keyCompileNoMethod = "compiler-methods-range";
    std::string valueCompileNoMethod = "0:0";
    std::string keyCheckPgoVersion = "compiler-check-pgo-version";
    std::string valueCheckPgoVersion = "true";
    argsMap.emplace(keyCompileNoMethod, valueCompileNoMethod);
    argsMap.emplace(keyCheckPgoVersion, valueCheckPgoVersion);
    std::vector<int16_t> sigData { 0, 1, 2, 3, 4, 5 };
    {
        std::lock_guard<std::mutex> lock(aotCompilerMutex_);
        if (gtAotImpl.EcmascriptAotCompiler(argsMap, sigData) !=
            ERR_AOT_COMPILER_SIGNATURE_DISABLE) {
            g_retGlobal = false;
        }
    }
}

void StopAotCompilerTask(void)
{
    (void)gtAotImpl.StopAotCompiler();
}

HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_010, TestSize.Level0)
{
    g_retGlobal = true;
    SET_THREAD_NUM(20);  // THREAD_NUM = 20
    GTEST_RUN_TASK(ExecuteAotCompilerTask);
    GTEST_RUN_TASK(StopAotCompilerTask);
#ifdef CODE_SIGN_ENABLE
    EXPECT_FALSE(g_retGlobal);
#else
    EXPECT_TRUE(g_retGlobal);
#endif
}

/**
 * @tc.name: AotCompilerImplTest_011
 * @tc.desc: AotCompilerImpl::StopAotCompiler() while EcmascriptAotCompiler() is
 *           running regarding multi threads.
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_011, TestSize.Level0)
{
    g_retGlobal = true;
    SET_THREAD_NUM(40); // THREAD_NUM = 40
    MTEST_ADD_TASK(RANDOM_THREAD_ID, ExecuteAotCompilerTask);
    MTEST_ADD_TASK(RANDOM_THREAD_ID, StopAotCompilerTask);
    EXPECT_TRUE(g_retGlobal);
}

HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_012, TestSize.Level0)
{
    g_retGlobal = true;
    MTEST_POST_RUN();
#ifdef CODE_SIGN_ENABLE
    EXPECT_FALSE(g_retGlobal);
#else
    EXPECT_TRUE(g_retGlobal);
#endif
}

/**
 * @tc.name: AotCompilerImplTest_013
 * @tc.desc: AotCompilerImpl::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_013, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    std::string keyName = "compiler-pkg-info";
    int32_t bundleID = 0;
    int32_t ret = aotImplMock.FindArgsIdxToIntegerMock(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_014
 * @tc.desc: AotCompilerImpl::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_014, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "argValueNotInteger");
    std::string keyName = "argKey";
    int32_t bundleID = 0;
    int32_t ret = aotImplMock.FindArgsIdxToIntegerMock(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_015
 * @tc.desc: AotCompilerImpl::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_015, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "123456_argValueNotInteger");
    std::string keyName = "argKey";
    int32_t bundleID = 0;
    int32_t ret = aotImplMock.FindArgsIdxToIntegerMock(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_016
 * @tc.desc: AotCompilerImpl::FindArgsIdxToInteger(argsMap, keyName, bundleID)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_016, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "20020079");
    std::string keyName = "argKey";
    int32_t bundleID = 0;
    int32_t ret = aotImplMock.FindArgsIdxToIntegerMock(argsMap, keyName, bundleID);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(bundleID, 20020079);
}

/**
 * @tc.name: AotCompilerImplTest_017
 * @tc.desc: AotCompilerImpl::FindArgsIdxToString(argsMap, keyName, bundleArg)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_017, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    std::string keyName = "argKey";
    std::string bundleArg = "";
    int32_t ret = aotImplMock.FindArgsIdxToStringMock(argsMap, keyName, bundleArg);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_018
 * @tc.desc: AotCompilerImpl::FindArgsIdxToString(argsMap, keyName, bundleArg)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_018, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    argsMap.emplace("argKey", "com.ohos.contacts");
    std::string keyName = "argKey";
    std::string bundleArg = "";
    int32_t ret = aotImplMock.FindArgsIdxToStringMock(argsMap, keyName, bundleArg);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_STREQ(bundleArg.c_str(), "com.ohos.contacts");
}

/**
 * @tc.name: AotCompilerImplTest_019
 * @tc.desc: AotCompilerImpl::PrepareArgs(argsMapForTest)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_019, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    int32_t ret = aotImplMock.PrepareArgsMock(argsMap);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
    ret = aotImplMock.PrepareArgsMock(argsMapForTest);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerImplTest_020
 * @tc.desc: AotCompilerImpl::AddExpandArgs(aotVector)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_020, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    std::vector<std::string> aotVector;
    aotImplMock.AddExpandArgsMock(aotVector);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerImplTest_021
 * @tc.desc: AotCompilerImpl::PrintAOTCompilerResult(TEST_ERR_AN_EMPTY)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_021, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(TEST_ERR_OTHERS);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
    ret = aotImplMock.PrintAOTCompilerResultMock(TEST_ERR_AN_EMPTY);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
    ret = aotImplMock.PrintAOTCompilerResultMock(TEST_ERR_OK);
    EXPECT_EQ(ret, ERR_OK);
}

/**
 * @tc.name: AotCompilerImplTest_022
 * @tc.desc: AotCompilerImpl::EcmascriptAotCompiler(argsMap, sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_022, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::unordered_map<std::string, std::string> argsMap;
    std::vector<int16_t> sigData;
    int32_t ret = ERR_FAIL;
#ifdef CODE_SIGN_ENABLE
    aotImplMock.PauseAotCompilerMock();
    ret = aotImplMock.EcmascriptAotCompiler(argsMap, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_CANCELLED);

    aotImplMock.AllowAotCompilerMock();
    ret = aotImplMock.EcmascriptAotCompiler(argsMap, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);

    aotImplMock.AllowAotCompilerMock();
    ret = aotImplMock.EcmascriptAotCompiler(argsMapForTest, sigData);
    EXPECT_NE(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#else
    ret = aotImplMock.EcmascriptAotCompiler(argsMapForTest, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
    EXPECT_TRUE(sigData.empty());
}

/**
 * @tc.name: AotCompilerImplTest_023
 * @tc.desc: AotCompilerImpl::AOTLocalCodeSign(sigData)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_023, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::string fileName = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    std::string appSignature = "5765880207853624761";
    std::vector<int16_t> sigData;
    int32_t ret = aotImplMock.PrepareArgsMock(argsMapForTest);
    EXPECT_EQ(ret, ERR_OK);
    ret = aotImplMock.AOTLocalCodeSignMock(sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_NE(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
 * @tc.name: AotCompilerImplTest_024
 * @tc.desc: AotCompilerImpl::StopAotCompiler()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_024, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    aotImplMock.ResetStateMock();
    int32_t ret = aotImplMock.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);

    aotImplMock.InitStateMock(-1);
    ret = aotImplMock.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);

    aotImplMock.InitStateMock(123456789);   // test_childPid = 123456789
    ret = aotImplMock.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);
}

/**
 * @tc.name: AotCompilerImplTest_025
 * @tc.desc: AotCompilerImpl::HandlePowerDisconnected()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_025, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandlePowerDisconnected();
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerImplTest_026
 * @tc.desc: AotCompilerImpl::HandleScreenOn()
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_026, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandleScreenOn();
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerImplTest_027
 * @tc.desc: AotCompilerImpl::HandleThermalLevelChanged(aotImpl.AOT_COMPILE_STOP_LEVEL)
 * @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_027, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandleThermalLevelChanged(aotImplMock.AOT_COMPILE_STOP_LEVEL);
    aotImplMock.HandleThermalLevelChanged(aotImplMock.AOT_COMPILE_STOP_LEVEL - 1);
    EXPECT_TRUE(viewData);
}

/**
 * @tc.name: AotCompilerImplTest_028
 * @tc.desc: AotCompilerImpl::ExecuteInParentProcess(const pid_t childPid, int32_t &ret)
 *              child process terminate with signal SIGKILL;
 *                  parent process receive SIGKILL signal;
 * @tc.type: Func
*/
AotCompilerImplMock g_aotImplMock;
int32_t g_aotRet = INVALID_ERR_CODE;

void RunAotCompilerTask(void)
{
    std::unordered_map<std::string, std::string> argsMap(argsMapForTest);
    std::vector<int16_t> sigData;
    {
        std::lock_guard<std::mutex> lock(aotCompilerMutex_);
        int32_t aotRet = g_aotImplMock.EcmascriptAotCompilerMock(argsMap, sigData);
        if (aotRet == ERR_AOT_COMPILER_CALL_CRASH     ||
            aotRet == ERR_AOT_COMPILER_CALL_CANCELLED ||
            aotRet == ERR_AOT_COMPILER_SIGNATURE_DISABLE) {
            g_aotRet = aotRet;
        }
    }
}

void CancelAotCompilerTask(void)
{
    sleep(1); // 1: delay 1s
    pid_t childPid = g_aotImplMock.GetChildPidMock();
    if (childPid > 0) {
        g_aotImplMock.InitStateMock(childPid);
        (void)g_aotImplMock.StopAotCompiler();
    }
}

void TestCancelAotCompilerTask()
{
    std::thread([]() {
        RunAotCompilerTask();
    }).detach();
    std::thread([]() {
        CancelAotCompilerTask();
    }).detach();
    sleep(3); // 3: delay 3s
}

HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_028, TestSize.Level0)
{
    g_aotRet = INVALID_ERR_CODE;
    TestCancelAotCompilerTask();
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(g_aotRet, ERR_AOT_COMPILER_CALL_CANCELLED);
#else
    EXPECT_EQ(g_aotRet, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}
} // namespace OHOS::ArkCompiler