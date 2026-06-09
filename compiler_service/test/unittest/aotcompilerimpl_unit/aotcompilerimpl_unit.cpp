/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include <mutex>
#include <securec.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "aot_compiler_client.h"
#include "aot_compiler_service.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_impl.h"
#include "aot_compiler_constants.h"
#include "aot_compiler_load_callback.h"
#include "ecmascript/platform/file.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

using namespace testing::ext;
using namespace testing::mt;

namespace OHOS::ArkCompiler {
namespace {
constexpr int TEST_ERR_OK = 0;
constexpr int TEST_ERR_AN_EMPTY = 5;
constexpr int TEST_ERR_OTHERS = 100;
constexpr int32_t TEST_BUNDLE_UID = 20020079;
constexpr int32_t TEST_PROCESS_UID = 3060;
constexpr int32_t TEST_FULL_ARGS_UID = 100000;
constexpr int32_t TEST_FULL_ARGS_PID = 1234;
AotCompilerArgs MakeTestArgs()
{
    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = "dynamic";
    args.bundleName = "com.ohos.contacts";
    args.moduleName = "entry";
    args.appIdentifier = "5765880207853624761";
    args.bundleUid = TEST_BUNDLE_UID;
    args.bundleGid = TEST_BUNDLE_UID;
    args.processUid = TEST_PROCESS_UID;
    args.hapPath = "/system/app/Contacts/Contacts.hap";
    args.abcPath = "/system/app/Contacts/Contacts.hap/ets/modules.abc";
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    args.outputPath = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64";
    args.pgoDir = "/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts";
    args.isScreenOff = 1;
    args.isEncryptedBundle = 0;
    args.isEnableBaselinePgo = 0;
    return args;
}

void SetUpFullArgs(AotCompilerArgs &args)
{
    args.isSysComp = false;
    args.compileMode = "partial";
    args.moduleArkTSMode = "dynamic";
    args.bundleName = "com.test.bundle";
    args.moduleName = "entry";
    args.appIdentifier = "1234567890";
    args.bundleUid = TEST_FULL_ARGS_UID;
    args.bundleGid = TEST_FULL_ARGS_UID;
    args.processUid = TEST_FULL_ARGS_PID;
    args.hapPath = "/system/app/Test/Test.hap";
    args.abcPath = "/system/app/Test/Test.hap/ets/modules.abc";
    args.anFileName = "/data/app/ark_cache/com.test.bundle/arm64/entry.an";
    args.outputPath = "/data/app/ark_cache/com.test.bundle/arm64";
    args.sysCompPath = "";
    args.pgoDir = "/data/app/ark_profile/com.test.bundle";
    args.optBCRangeList = "range1";
    args.isScreenOff = 1;
    args.isEncryptedBundle = 0;
    args.isEnableBaselinePgo = 1;
    args.bundleType = 0;
    args.triggerType = 1;
}

void VerifyArgsEqual(const AotCompilerArgs &expected, const AotCompilerArgs &actual)
{
    EXPECT_EQ(actual.isSysComp, expected.isSysComp);
    EXPECT_EQ(actual.compileMode, expected.compileMode);
    EXPECT_EQ(actual.moduleArkTSMode, expected.moduleArkTSMode);
    EXPECT_EQ(actual.bundleName, expected.bundleName);
    EXPECT_EQ(actual.hostBundleName, expected.hostBundleName);
    EXPECT_EQ(actual.moduleName, expected.moduleName);
    EXPECT_EQ(actual.appIdentifier, expected.appIdentifier);
    EXPECT_EQ(actual.bundleUid, expected.bundleUid);
    EXPECT_EQ(actual.bundleGid, expected.bundleGid);
    EXPECT_EQ(actual.processUid, expected.processUid);
    EXPECT_EQ(actual.hapPath, expected.hapPath);
    EXPECT_EQ(actual.abcPath, expected.abcPath);
    EXPECT_EQ(actual.anFileName, expected.anFileName);
    EXPECT_EQ(actual.outputPath, expected.outputPath);
    EXPECT_EQ(actual.sysCompPath, expected.sysCompPath);
    EXPECT_EQ(actual.pgoDir, expected.pgoDir);
    EXPECT_EQ(actual.optBCRangeList, expected.optBCRangeList);
    EXPECT_EQ(actual.isScreenOff, expected.isScreenOff);
    EXPECT_EQ(actual.isEncryptedBundle, expected.isEncryptedBundle);
    EXPECT_EQ(actual.isEnableBaselinePgo, expected.isEnableBaselinePgo);
    EXPECT_EQ(actual.bundleType, expected.bundleType);
    EXPECT_EQ(actual.triggerType, expected.triggerType);
}
} // namespace

class AotCompilerImplMock : public AotCompilerImpl {
public:
    AotCompilerImplMock() = default;
    ~AotCompilerImplMock() = default;
    AotCompilerImplMock(const AotCompilerImplMock&) = delete;
    AotCompilerImplMock(AotCompilerImplMock&&) = delete;
    AotCompilerImplMock& operator=(const AotCompilerImplMock&) = delete;
    AotCompilerImplMock& operator=(AotCompilerImplMock&&) = delete;

    int32_t PrintAOTCompilerResultMock(const int compilerStatus) const
    {
        return PrintAOTCompilerResult(compilerStatus);
    }

    int32_t EcmascriptAotCompilerMock(const AotCompilerArgs &args,
                                      std::vector<uint8_t> &sigData)
    {
    #ifdef CODE_SIGN_ENABLE
        if (!allowAotCompiler_) {
            return ERR_AOT_COMPILER_CALL_CANCELLED;
        }

        std::unique_ptr<AOTArgsHandler> argsHandler = std::make_unique<AOTArgsHandler>(args);
        if (argsHandler->Handle(0) != ERR_OK) {
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
        return ret != ERR_OK ? ret : AOTLocalCodeSign(sigData, -1);
    #else
        return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
    #endif
    }

    int32_t AOTLocalCodeSignMock(std::vector<uint8_t> &sigData, int32_t anFd) const
    {
        return AOTLocalCodeSign(sigData, anFd);
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

    bool IsStopRequestedMock() const
    {
        return stopRequested_.load();
    }

    void SetStopRequestedMock(bool stopRequested)
    {
        stopRequested_.store(stopRequested);
    }

    int32_t CheckStopBeforeForkMock(const AotCompilerArgs &args)
    {
    #ifdef CODE_SIGN_ENABLE
        argsHandler_ = std::make_unique<AOTArgsHandler>(args);
        if (argsHandler_->Handle(thermalLevel_) != ERR_OK) {
            return ERR_AOT_COMPILER_PARAM_FAILED;
        }
        if (stopRequested_.load() || !allowAotCompiler_.load()) {
            return ERR_AOT_COMPILER_CALL_CANCELLED;
        }
        return ERR_OK;
    #else
        return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
    #endif
    }

    int32_t ExecuteInParentProcessWithStopRequestedMock(const AotCompilerArgs &args)
    {
    #ifdef CODE_SIGN_ENABLE
        argsHandler_ = std::make_unique<AOTArgsHandler>(args);
        if (argsHandler_->Handle(thermalLevel_) != ERR_OK) {
            return ERR_AOT_COMPILER_PARAM_FAILED;
        }
        stopRequested_.store(true);
        int32_t ret = ERR_OK;
        pid_t pid = fork();
        if (pid == -1) {
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        if (pid == 0) {
            sleep(10); // 10: keep child alive until parent consumes the pending stop request.
            _exit(0);
        }
        mockChildPid_ = pid;
        ExecuteInParentProcess(pid, ret);
        return ret;
    #else
        return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
    #endif
    }

    int32_t ExecuteInParentProcessAfterScreenOnStopMock(const AotCompilerArgs &args,
                                                        bool restoreAllowBeforeParent)
    {
    #ifdef CODE_SIGN_ENABLE
        argsHandler_ = std::make_unique<AOTArgsHandler>(args);
        if (argsHandler_->Handle(thermalLevel_) != ERR_OK) {
            return ERR_AOT_COMPILER_PARAM_FAILED;
        }
        AllowAotCompiler();
        stopRequested_.store(false);
        PauseAotCompiler();
        int32_t stopRet = StopAotCompiler();
        if (stopRet != ERR_AOT_COMPILER_STOP_FAILED || !stopRequested_.load()) {
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        if (IsAllowAotCompiler()) {
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        if (restoreAllowBeforeParent) {
            AllowAotCompiler();
        }
        if (restoreAllowBeforeParent != IsAllowAotCompiler()) {
            return ERR_AOT_COMPILER_CALL_FAILED;
        }

        int32_t ret = ERR_OK;
        pid_t pid = fork();
        if (pid == -1) {
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        if (pid == 0) {
            sleep(2); // 2: keep child alive unless the pending stop request kills it.
            _exit(0);
        }
        mockChildPid_ = pid;
        ExecuteInParentProcess(pid, ret);
        return ret;
    #else
        return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
    #endif
    }

    pid_t GetChildPidMock()
    {
        return mockChildPid_;
    }

    void SetAOTArgsHandler(std::unique_ptr<AOTArgsHandler> argsHandler)
    {
        argsHandler_ = std::move(argsHandler);
    }

    std::string GetAiFilePathMock(const std::string &anFilePath)
    {
        return GetAiFilePath(anFilePath);
    }

#ifdef CODE_SIGN_ENABLE
    int32_t PrepareOutputFilesMock(AotParserType parserType, uid_t bundleUid, gid_t bundleGid, int32_t &outFd)
    {
        return PrepareOutputFiles(parserType, bundleUid, bundleGid, outFd);
    }

    int32_t PreCreateAotFilesMock(AotParserType parserType, uid_t bundleUid, gid_t bundleGid, int32_t &outFd)
    {
        return PreCreateAotFiles(parserType, bundleUid, bundleGid, outFd);
    }

    int32_t CreateAnFileMock(const std::string &anFilePath, int32_t &outFd)
    {
        return CreateAnFile(anFilePath, outFd);
    }

    int32_t CreateAiFileMock(const std::string &aiFilePath, uid_t bundleUid, gid_t bundleGid)
    {
        return CreateAiFile(aiFilePath, bundleUid, bundleGid);
    }

    int32_t ChownAotFilesToBundleMock()
    {
        return ChownAotFilesToBundle();
    }

    int32_t HandlePostCompilationMock(const AotCompilerArgs &args, int32_t ret,
                                       std::vector<uint8_t> &sigData,
                                       CompilationCleanupGuard &guard)
    {
        return HandlePostCompilation(args, ret, sigData, guard);
    }

    void EnsureDirPermissionMock(const std::string &dirPath)
    {
        EnsureDirPermission(dirPath);
    }

    int32_t CreateAnMemfdMock(int32_t &outFd)
    {
        return CreateAnMemfd(outFd);
    }

    int32_t PersistAnFileMock(int32_t memfd, const std::string &anPath)
    {
        return PersistAnFile(memfd, anPath);
    }

    static int32_t SpliceFdToFdMock(int srcFd, int dstFd, off_t size)
    {
        return SpliceFdToFd(srcFd, dstFd, size);
    }
#endif
private:
    pid_t mockChildPid_ = -1;
};

class AotCompilerImplTest : public testing::Test {
public:
    AotCompilerImplTest() {}
    virtual ~AotCompilerImplTest() {}

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override
    {
        mkdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts",
              S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir("/data/app/el1/public/aot_compiler/ark_cache", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts",
              S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64",
              S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        std::ofstream anFile("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an");
        anFile.close();
    }

    void TearDown() override
    {
        unlink("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an");
        rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64");
        rmdir("/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts");
        rmdir("/data/app/el1/public/aot_compiler/ark_cache");
        rmdir("/data/app/el1/100/aot_compiler/ark_profile/com.ohos.contacts");
    }
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
    AotCompilerArgs args = MakeTestArgs();
    std::vector<uint8_t> sigData;
    int32_t ret = aotImpl.EcmascriptAotCompiler(args, sigData);
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
    AotCompilerArgs args = MakeTestArgs();
    std::vector<uint8_t> sigData { 0, 1, 2, 3, 4, 5 };
    int32_t ret = aotImpl.EcmascriptAotCompiler(args, sigData);
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
    EXPECT_EQ(ret, ERR_OK);
}

/**
* @tc.name: AotCompilerImplTest_009
* @tc.desc: AotCompilerImpl::HandleThermalLevelChanged()
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_009, TestSize.Level0)
{
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    aotImpl.HandleThermalLevelChanged(1);
    EXPECT_TRUE(aotImpl.IsAllowAotCompiler());
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
    AotCompilerArgs args = MakeTestArgs();
    std::vector<uint8_t> sigData { 0, 1, 2, 3, 4, 5 };
    {
        std::lock_guard<std::mutex> lock(aotCompilerMutex_);
        if (gtAotImpl.EcmascriptAotCompiler(args, sigData) !=
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
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult(TEST_ERR_AN_EMPTY)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_013, TestSize.Level0)
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
* @tc.name: AotCompilerImplTest_014
* @tc.desc: AotCompilerImpl::EcmascriptAotCompiler(args, sigData)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_014, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs emptyArgs;
    AotCompilerArgs testArgs = MakeTestArgs();
    std::vector<uint8_t> sigData;
    int32_t ret = ERR_FAIL;
#ifdef CODE_SIGN_ENABLE
    aotImplMock.PauseAotCompilerMock();
    ret = aotImplMock.EcmascriptAotCompiler(emptyArgs, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_CANCELLED);

    aotImplMock.AllowAotCompilerMock();
    ret = aotImplMock.EcmascriptAotCompiler(emptyArgs, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);

    aotImplMock.AllowAotCompilerMock();
    ret = aotImplMock.EcmascriptAotCompiler(testArgs, sigData);
    EXPECT_NE(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#else
    ret = aotImplMock.EcmascriptAotCompiler(testArgs, sigData);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
    EXPECT_TRUE(sigData.empty());
}

/**
* @tc.name: AotCompilerImplTest_017
* @tc.desc: AotCompilerImpl::HandlePowerDisconnected()
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_017, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandlePowerDisconnected();
    EXPECT_TRUE(viewData);
}

/**
* @tc.name: AotCompilerImplTest_018
* @tc.desc: AotCompilerImpl::HandleScreenOn()
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_018, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandleScreenOn();
    EXPECT_TRUE(viewData);
}

/**
* @tc.name: AotCompilerImplTest_019
* @tc.desc: AotCompilerImpl::HandleThermalLevelChanged(aotImpl.AOT_COMPILE_STOP_LEVEL)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_019, TestSize.Level0)
{
    bool viewData = true;
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandleThermalLevelChanged(aotImplMock.AOT_COMPILE_STOP_LEVEL);
    aotImplMock.HandleThermalLevelChanged(aotImplMock.AOT_COMPILE_STOP_LEVEL - 1);
    EXPECT_TRUE(viewData);
}

/**
* @tc.name: AotCompilerImplTest_021
* @tc.desc: AotCompilerImpl::ParseArkCacheFromArgs()
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_021, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    args.appIdentifier = "5765880207853624761";
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    std::string arkCachePath = aotImpl.ParseArkCacheFromArgs(args);
    EXPECT_EQ(arkCachePath, "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts");
}

HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_022, TestSize.Level0)
{
    AotCompilerArgs emptyArgs;
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    std::string arkCachePath = aotImpl.ParseArkCacheFromArgs(emptyArgs);
    EXPECT_TRUE(arkCachePath.empty());
}

HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_023, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    args.appIdentifier = "5765880207853624761";
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    int32_t result = aotImpl.SendSysEvent(args);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: AotCompilerImplTest_024
* @tc.desc: AotCompilerImpl::ParseArkCacheFromArgs() when aot-file has no slash
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_024, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "entry_without_slash";
    args.appIdentifier = "5765880207853624761";
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    std::string arkCachePath = aotImpl.ParseArkCacheFromArgs(args);
    EXPECT_TRUE(arkCachePath.empty());
}

/**
* @tc.name: AotCompilerImplTest_025
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult() with ERR_NO_AP status
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_025, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(2);
    EXPECT_EQ(ret, ERR_OK_NO_AOT_FILE);
}

/**
* @tc.name: AotCompilerImplTest_026
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult() with ERR_CHECK_VERSION status
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_026, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(4);
    EXPECT_EQ(ret, ERR_OK_NO_AOT_FILE);
}

/**
* @tc.name: AotCompilerImplTest_028
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult() with ERR_AN_FAIL status
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_028, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(6);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
* @tc.name: AotCompilerImplTest_029
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult() with ERR_AI_FAIL status
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_029, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(7);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
* @tc.name: AotCompilerImplTest_030
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult() with ERR_FAIL status
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_030, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(-1);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
* @tc.name: AotCompilerImplTest_031
* @tc.desc: AotCompilerImpl::PrintAOTCompilerResult() with ERR_HELP status
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_031, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t ret = aotImplMock.PrintAOTCompilerResultMock(1);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
* @tc.name: AotCompilerImplTest_032
* @tc.desc: AotCompilerImpl::IsAllowAotCompiler() after pause and allow
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_032, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    EXPECT_TRUE(aotImplMock.IsAllowAotCompiler());

    aotImplMock.PauseAotCompilerMock();
    EXPECT_FALSE(aotImplMock.IsAllowAotCompiler());

    aotImplMock.AllowAotCompilerMock();
    EXPECT_TRUE(aotImplMock.IsAllowAotCompiler());
}

/**
* @tc.name: AotCompilerImplTest_033
* @tc.desc: AotCompilerImpl::HandleThermalLevelChanged() with various levels
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_033, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    aotImplMock.HandleThermalLevelChanged(0);
    EXPECT_TRUE(aotImplMock.IsAllowAotCompiler());

    aotImplMock.HandleThermalLevelChanged(aotImplMock.AOT_COMPILE_STOP_LEVEL);
    EXPECT_FALSE(aotImplMock.IsAllowAotCompiler());

    aotImplMock.AllowAotCompilerMock();
    aotImplMock.HandleThermalLevelChanged(3);
    EXPECT_FALSE(aotImplMock.IsAllowAotCompiler());
}

/**
* @tc.name: AotCompilerImplTest_034
* @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() with system component
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_034, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args;
    args.isSysComp = true;
    args.bundleUid = TEST_BUNDLE_UID;
    args.bundleGid = TEST_BUNDLE_UID;
    args.anFileName = "/data/app/el1/public/aot_compiler/ark_cache/com.ohos.contacts/arm64/entry.an";
    args.appIdentifier = "5765880207853624761";
    args.sysCompPath = "/system/app/Contacts/Contacts.hap/ets/modules.abc";
    std::vector<uint8_t> sigData;
    aotImplMock.AllowAotCompilerMock();
    int32_t ret = aotImplMock.EcmascriptAotCompiler(args, sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_NE(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_035
* @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() with empty compileMode
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_035, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args;
    args.compileMode = "";
    args.bundleName = "test";
    args.bundleUid = TEST_BUNDLE_UID;
    args.bundleGid = TEST_BUNDLE_UID;
    std::vector<uint8_t> sigData;
    aotImplMock.AllowAotCompilerMock();
    int32_t ret = aotImplMock.EcmascriptAotCompiler(args, sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_036
* @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() with empty bundleName
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_036, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args;
    args.compileMode = "partial";
    args.bundleName = "";
    args.bundleUid = TEST_BUNDLE_UID;
    args.bundleGid = TEST_BUNDLE_UID;
    std::vector<uint8_t> sigData;
    aotImplMock.AllowAotCompilerMock();
    int32_t ret = aotImplMock.EcmascriptAotCompiler(args, sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_037
* @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() with isSysComp = false
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_037, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args;
    args.isSysComp = false;
    args.bundleUid = TEST_BUNDLE_UID;
    args.bundleGid = TEST_BUNDLE_UID;
    std::vector<uint8_t> sigData;
    aotImplMock.AllowAotCompilerMock();
    int32_t ret = aotImplMock.EcmascriptAotCompiler(args, sigData);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_PARAM_FAILED);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_039
* @tc.desc: AotCompilerImpl::ParseArkCacheFromArgs() with path ending at root
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_039, TestSize.Level0)
{
    AotCompilerArgs args;
    args.anFileName = "/entry";
    args.appIdentifier = "5765880207853624761";
    AotCompilerImpl &aotImpl = AotCompilerImplMock::GetInstance();
    std::string arkCachePath = aotImpl.ParseArkCacheFromArgs(args);
    EXPECT_EQ(arkCachePath, "");
}

/**
* @tc.name: AotCompilerImplTest_040
* @tc.desc: AotCompilerImpl::StopAotCompiler() keeps stop request when child state is not initialized
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_040, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    aotImplMock.SetStopRequestedMock(false);
    int32_t ret = aotImplMock.StopAotCompiler();
    EXPECT_EQ(ret, ERR_AOT_COMPILER_STOP_FAILED);
    EXPECT_TRUE(aotImplMock.IsStopRequestedMock());
}

/**
* @tc.name: AotCompilerImplTest_041
* @tc.desc: AotCompilerImpl::EcmascriptAotCompiler() cancels pending stop before fork
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_041, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args = MakeTestArgs();
    aotImplMock.AllowAotCompilerMock();
    aotImplMock.SetStopRequestedMock(true);
    int32_t ret = aotImplMock.CheckStopBeforeForkMock(args);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_CANCELLED);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_042
* @tc.desc: AotCompilerImpl::ExecuteInParentProcess() kills child when stop request arrives before state init
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_042, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args = MakeTestArgs();
    aotImplMock.AllowAotCompilerMock();
    int32_t ret = aotImplMock.ExecuteInParentProcessWithStopRequestedMock(args);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_CANCELLED);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_043
* @tc.desc: AotCompilerImpl cancels and cleans child when screen-on stop fails before state init
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_043, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args = MakeTestArgs();
    int32_t ret = aotImplMock.ExecuteInParentProcessAfterScreenOnStopMock(args, false);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_CANCELLED);
    EXPECT_NE(access(args.anFileName.c_str(), F_OK), 0);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

/**
* @tc.name: AotCompilerImplTest_044
* @tc.desc: AotCompilerImpl cancels child by pending stop request after allow state is restored
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_044, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args = MakeTestArgs();
    int32_t ret = aotImplMock.ExecuteInParentProcessAfterScreenOnStopMock(args, true);
#ifdef CODE_SIGN_ENABLE
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_CANCELLED);
    EXPECT_NE(access(args.anFileName.c_str(), F_OK), 0);
#else
    EXPECT_EQ(ret, ERR_AOT_COMPILER_SIGNATURE_DISABLE);
#endif
}

// =========================================================================
// Group 6: AotCompilerArgs Marshalling/Unmarshalling Round-trip
// =========================================================================

/**
* @tc.name: AotCompilerImplTest_061
* @tc.desc: AotCompilerArgs full args Marshalling/Unmarshalling round-trip
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_061, TestSize.Level0)
{
    AotCompilerArgs args;
    SetUpFullArgs(args);

    Parcel parcel;
    ASSERT_TRUE(args.Marshalling(parcel));
    parcel.RewindRead(0);

    auto deserialized = AotCompilerArgs::Unmarshalling(parcel);
    ASSERT_NE(deserialized, nullptr);
    VerifyArgsEqual(args, *deserialized);
    EXPECT_EQ(deserialized->hspModules.size(), 0u);
    delete deserialized;
}

/**
* @tc.name: AotCompilerImplTest_062
* @tc.desc: AotCompilerArgs empty args Marshalling/Unmarshalling round-trip
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_062, TestSize.Level0)
{
    AotCompilerArgs args;
    Parcel parcel;
    ASSERT_TRUE(args.Marshalling(parcel));
    parcel.RewindRead(0);

    auto deserialized = AotCompilerArgs::Unmarshalling(parcel);
    ASSERT_NE(deserialized, nullptr);
    EXPECT_EQ(deserialized->isSysComp, false);
    EXPECT_EQ(deserialized->compileMode, "");
    EXPECT_EQ(deserialized->bundleName, "");
    EXPECT_EQ(deserialized->hostBundleName, "");
    EXPECT_EQ(deserialized->bundleUid, -1);
    EXPECT_EQ(deserialized->hspModules.size(), 0u);
    delete deserialized;
}

/**
* @tc.name: AotCompilerImplTest_063
* @tc.desc: HspModuleInfo Marshalling/Unmarshalling round-trip
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_063, TestSize.Level0)
{
    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = "dynamic";
    args.bundleName = "com.test.hsp";
    args.moduleName = "hspModule";
    HspModuleInfo hsp1;
    hsp1.bundleName = "com.test.hsp";
    hsp1.moduleName = "feature";
    hsp1.hapPath = "/path/to/feature.hap";
    hsp1.moduleArkTSMode = "static";
    args.hspModules.push_back(hsp1);

    Parcel parcel;
    ASSERT_TRUE(args.Marshalling(parcel));
    parcel.RewindRead(0);

    auto deserialized = AotCompilerArgs::Unmarshalling(parcel);
    ASSERT_NE(deserialized, nullptr);
    ASSERT_EQ(deserialized->hspModules.size(), 1u);
    const auto &hsp = deserialized->hspModules[0];
    EXPECT_EQ(hsp.bundleName, "com.test.hsp");
    EXPECT_EQ(hsp.moduleName, "feature");
    EXPECT_EQ(hsp.hapPath, "/path/to/feature.hap");
    EXPECT_EQ(hsp.moduleArkTSMode, "static");
    delete deserialized;
}

/**
* @tc.name: AotCompilerImplTest_064
* @tc.desc: AotCompilerArgs with multiple HSP modules
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_064, TestSize.Level0)
{
    AotCompilerArgs args;
    args.compileMode = "partial";
    args.moduleArkTSMode = "dynamic";
    args.bundleName = "com.test.multi";
    args.moduleName = "entry";
    HspModuleInfo hspM1;
    hspM1.bundleName = "com.test.multi";
    hspM1.moduleName = "hsp1";
    hspM1.hapPath = "/path/hsp1.hap";
    hspM1.moduleArkTSMode = "dynamic";
    args.hspModules.push_back(hspM1);
    HspModuleInfo hspM2;
    hspM2.bundleName = "com.test.multi";
    hspM2.moduleName = "hsp2";
    hspM2.hapPath = "/path/hsp2.hap";
    hspM2.moduleArkTSMode = "static";
    args.hspModules.push_back(hspM2);
    HspModuleInfo hspM3;
    hspM3.bundleName = "com.test.multi";
    hspM3.moduleName = "hsp3";
    hspM3.hapPath = "/path/hsp3.hap";
    hspM3.moduleArkTSMode = "hybrid";
    args.hspModules.push_back(hspM3);

    Parcel parcel;
    ASSERT_TRUE(args.Marshalling(parcel));
    parcel.RewindRead(0);

    auto deserialized = AotCompilerArgs::Unmarshalling(parcel);
    ASSERT_NE(deserialized, nullptr);
    ASSERT_EQ(deserialized->hspModules.size(), 3u);
    EXPECT_EQ(deserialized->hspModules[0].moduleName, "hsp1");
    EXPECT_EQ(deserialized->hspModules[1].moduleName, "hsp2");
    EXPECT_EQ(deserialized->hspModules[2].moduleName, "hsp3");
    delete deserialized;
}

// =========================================================================
// Group 7: FdHolder RAII
// =========================================================================

/**
* @tc.name: AotCompilerImplTest_065
* @tc.desc: FdHolder default state is -1
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_065, TestSize.Level0)
{
    FdHolder holder;
    EXPECT_EQ(holder.Get(), -1);
}

/**
* @tc.name: AotCompilerImplTest_066
* @tc.desc: FdHolder Acquire and Get returns the fd
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_066, TestSize.Level0)
{
    FdHolder holder;
    // Create a temp fd via memfd (or pipe as fallback)
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    panda::ecmascript::FdsanExchangeOwnerTag(fds[1]);
    holder.Acquire(fds[1]);
    EXPECT_EQ(holder.Get(), fds[1]);
    // Clean up: holder takes ownership of fds[1], close fds[0] manually
    panda::ecmascript::Close(fds[0]);
}

/**
* @tc.name: AotCompilerImplTest_067
* @tc.desc: FdHolder Reset closes the fd and sets back to -1
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_067, TestSize.Level0)
{
    FdHolder holder;
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    panda::ecmascript::FdsanExchangeOwnerTag(fds[1]);
    holder.Acquire(fds[1]);
    EXPECT_GE(holder.Get(), 0);

    holder.Reset();
    EXPECT_EQ(holder.Get(), -1);

    // Verify the fd was actually closed — writing to read end should fail (broken pipe)
    // or we can verify by checking fds[0] is still valid but the pipe is broken
    panda::ecmascript::Close(fds[0]);
}

/**
* @tc.name: AotCompilerImplTest_068
* @tc.desc: FdHolder double Reset is safe (no double-close crash)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_068, TestSize.Level0)
{
    FdHolder holder;
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    panda::ecmascript::FdsanExchangeOwnerTag(fds[1]);
    holder.Acquire(fds[1]);
    holder.Reset();
    EXPECT_EQ(holder.Get(), -1);
    // Second Reset should be a no-op (fd already -1)
    holder.Reset();
    EXPECT_EQ(holder.Get(), -1);
    panda::ecmascript::Close(fds[0]);
}

/**
* @tc.name: AotCompilerImplTest_069
* @tc.desc: FdHolder Acquire replaces previous fd (old fd is closed)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_069, TestSize.Level0)
{
    FdHolder holder;
    int fds1[2] = {-1, -1};
    int fds2[2] = {-1, -1};
    ASSERT_EQ(pipe(fds1), 0);
    ASSERT_EQ(pipe(fds2), 0);
    panda::ecmascript::FdsanExchangeOwnerTag(fds1[1]);
    panda::ecmascript::FdsanExchangeOwnerTag(fds2[1]);

    holder.Acquire(fds1[1]);
    EXPECT_EQ(holder.Get(), fds1[1]);

    // Acquire new fd — old fds1[1] should be closed
    holder.Acquire(fds2[1]);
    EXPECT_EQ(holder.Get(), fds2[1]);

    holder.Reset();
    EXPECT_EQ(holder.Get(), -1);
    panda::ecmascript::Close(fds1[0]);
    panda::ecmascript::Close(fds2[0]);
}

/**
* @tc.name: AotCompilerImplTest_070
* @tc.desc: FdHolder destructor closes the fd
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_070, TestSize.Level0)
{
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    panda::ecmascript::FdsanExchangeOwnerTag(fds[1]);
    int writeFd = fds[1];
    int readFd = fds[0];
    {
        FdHolder holder;
        holder.Acquire(writeFd);
        EXPECT_EQ(holder.Get(), writeFd);
        // holder goes out of scope — destructor should close writeFd
    }
    // writeFd should be closed now, write should fail
    char c = 'x';
    ssize_t ret = write(writeFd, &c, 1);
    EXPECT_EQ(ret, -1);
    panda::ecmascript::Close(readFd);
}

/**
* @tc.name: AotCompilerImplTest_071
* @tc.desc: FdHolder Acquire with negative fd is stored as-is but Reset won't try to close
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_071, TestSize.Level0)
{
    FdHolder holder;
    holder.Acquire(-1);
    EXPECT_EQ(holder.Get(), -1);
    // Reset should be safe even with -1 (no close call)
    holder.Reset();
    EXPECT_EQ(holder.Get(), -1);
}

/**
* @tc.name: AotCompilerImplTest_072
* @tc.desc: FdHolder Acquire with -5 (invalid fd) — Reset should be safe
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_072, TestSize.Level0)
{
    FdHolder holder;
    // Negative fds should never be passed to Acquire in practice,
    // but Reset must be safe regardless
    holder.Acquire(-5);
    EXPECT_EQ(holder.Get(), -5);
    holder.Reset(); // Should NOT call close(-5)
    EXPECT_EQ(holder.Get(), -1);
}

// ===================== P0: Core FD functionality tests =====================

#ifdef CODE_SIGN_ENABLE

static const std::string TEST_AN_DIR = "/data/app/el1/public/aot_compiler/ark_cache/test_tdd";
static const std::string TEST_AN_PATH = TEST_AN_DIR + "/entry.an";
static const std::string TEST_AI_PATH = TEST_AN_DIR + "/entry.ai";

class AotCompilerImplFDTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        mkdir("/data/app/el1/public/aot_compiler", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir("/data/app/el1/public/aot_compiler/ark_cache", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        mkdir(TEST_AN_DIR.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    static void TearDownTestCase()
    {
        unlink(TEST_AN_PATH.c_str());
        unlink(TEST_AI_PATH.c_str());
        rmdir(TEST_AN_DIR.c_str());
        rmdir("/data/app/el1/public/aot_compiler/ark_cache");
        rmdir("/data/app/el1/public/aot_compiler");
    }

    void SetUp() override {}
    void TearDown() override
    {
        unlink(TEST_AN_PATH.c_str());
        unlink(TEST_AI_PATH.c_str());
    }
};

/**
* @tc.name: AotCompilerImplTest_073
* @tc.desc: PrepareOutputFiles: non-AOT parserType returns ERR_OK
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_073, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    int32_t outFd = -1;
    auto ret = aotImplMock.PrepareOutputFilesMock(
        static_cast<AotParserType>(99), getuid(), getgid(), outFd);
    EXPECT_EQ(ret, ERR_OK);
}

/**
* @tc.name: AotCompilerImplTest_075
* @tc.desc: PreCreateAotFiles: empty anFilePath returns ERR_OK
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_075, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args;
    args.anFileName = "";
    aotImplMock.SetAOTArgsHandler(std::make_unique<AOTArgsHandler>(args));

    int32_t outFd75 = -1;
    auto ret = aotImplMock.PreCreateAotFilesMock(AotParserType::DYNAMIC_AOT, getuid(), getgid(), outFd75);
    EXPECT_EQ(ret, ERR_OK);
}

/**
* @tc.name: AotCompilerImplTest_076
* @tc.desc: CreateAnFile: normal creation returns ERR_OK
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_076, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::string testPath = TEST_AN_DIR + "/create_test.an";
    unlink(testPath.c_str());
    int32_t outFd76 = -1;
    auto ret = aotImplMock.CreateAnFileMock(testPath, outFd76);
    EXPECT_EQ(ret, ERR_OK);
    // Verify file was created
    struct stat st;
    EXPECT_EQ(stat(testPath.c_str(), &st), 0);
    unlink(testPath.c_str());
}

/**
* @tc.name: AotCompilerImplTest_077
* @tc.desc: CreateAnFile: open fails returns ERR_AOT_COMPILER_CALL_FAILED
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_077, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    // Path with null component should fail open()
    std::string badPath = "/nonexistent_dir_xyz/deeply/nested/path/test.an";
    int32_t outFd77 = -1;
    auto ret = aotImplMock.CreateAnFileMock(badPath, outFd77);
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
* @tc.name: AotCompilerImplTest_078
* @tc.desc: CreateAiFile: normal creation returns ERR_OK
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_078, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::string testPath = TEST_AN_DIR + "/create_test.ai";
    unlink(testPath.c_str());
    auto ret = aotImplMock.CreateAiFileMock(testPath, getuid(), getgid());
    EXPECT_EQ(ret, ERR_OK);
    struct stat st;
    EXPECT_EQ(stat(testPath.c_str(), &st), 0);
    unlink(testPath.c_str());
}

/**
* @tc.name: AotCompilerImplTest_079
* @tc.desc: CreateAiFile: open fails returns ERR_AOT_COMPILER_CALL_FAILED
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_079, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    std::string badPath = "/nonexistent_dir_xyz/deeply/nested/test.ai";
    auto ret = aotImplMock.CreateAiFileMock(badPath, getuid(), getgid());
    EXPECT_EQ(ret, ERR_AOT_COMPILER_CALL_FAILED);
}

/**
* @tc.name: AotCompilerImplTest_081
* @tc.desc: ChownAotFilesToBundle: empty anFilePath returns ERR_OK
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_081, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    AotCompilerArgs args;
    args.anFileName = "";
    aotImplMock.SetAOTArgsHandler(std::make_unique<AOTArgsHandler>(args));
    auto ret = aotImplMock.ChownAotFilesToBundleMock();
    EXPECT_EQ(ret, ERR_OK);
}

/**
* @tc.name: AotCompilerImplTest_083
* @tc.desc: CompilationCleanupGuard: files do not exist — no crash on cleanup
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_083, TestSize.Level0)
{
    // Guard with nonexistent path should not crash on destruction
    {
        CompilationCleanupGuard guard(TEST_AN_DIR + "/nonexistent_12345.an");
        // Guard goes out of scope — should not crash when files don't exist
    }
    EXPECT_TRUE(true);
}

/**
* @tc.name: AotCompilerImplTest_085
* @tc.desc: GetAiFilePath: normal .an → .ai replacement
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_085, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    auto result = aotImplMock.GetAiFilePathMock("/data/app/entry.an");
    EXPECT_EQ(result, "/data/app/entry.ai");
}

/**
* @tc.name: AotCompilerImplTest_086
* @tc.desc: GetAiFilePath: path without .an extension
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_086, TestSize.Level0)
{
    AotCompilerImplMock aotImplMock;
    auto result = aotImplMock.GetAiFilePathMock("/data/app/entry.bin");
    // .bin doesn't end with .an, so find_last_of('.') replaces from .bin
    EXPECT_NE(result.find(".ai"), std::string::npos);
}

/**
* @tc.name: AotCompilerImplTest_087
* @tc.desc: HspModuleInfo Unmarshalling: read failure returns nullptr
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_087, TestSize.Level0)
{
    Parcel parcel;
    // Empty parcel — reads should fail
    auto *result = HspModuleInfo::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: AotCompilerImplTest_088
* @tc.desc: ReadHspModules: hspSize > 1024 returns false (AotCompilerArgs Unmarshalling)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_088, TestSize.Level0)
{
    // Write a valid identity + package + paths + options + extra ints
    // then write an oversized hspSize
    Parcel parcel;
    // isSysComp
    ASSERT_TRUE(parcel.WriteBool(false));
    // compileMode
    ASSERT_TRUE(parcel.WriteString("partial"));
    // moduleArkTSMode
    ASSERT_TRUE(parcel.WriteString("dynamic"));
    // bundleName
    ASSERT_TRUE(parcel.WriteString("com.test"));
    // hostBundleName
    ASSERT_TRUE(parcel.WriteString(""));
    // moduleName
    ASSERT_TRUE(parcel.WriteString("entry"));
    // appIdentifier
    ASSERT_TRUE(parcel.WriteString("12345"));
    // bundleUid
    ASSERT_TRUE(parcel.WriteInt32(100));
    // bundleGid
    ASSERT_TRUE(parcel.WriteInt32(100));
    // processUid
    ASSERT_TRUE(parcel.WriteInt32(200));
    // hapPath
    ASSERT_TRUE(parcel.WriteString("/test.hap"));
    // abcPath
    ASSERT_TRUE(parcel.WriteString("/test.abc"));
    // anFileName
    ASSERT_TRUE(parcel.WriteString("/test.an"));
    // outputPath
    ASSERT_TRUE(parcel.WriteString("/output"));
    // sysCompPath
    ASSERT_TRUE(parcel.WriteString(""));
    // pgoDir
    ASSERT_TRUE(parcel.WriteString("/pgo"));
    // optBCRangeList
    ASSERT_TRUE(parcel.WriteString(""));
    // isScreenOff
    ASSERT_TRUE(parcel.WriteUint32(0));
    // isEncryptedBundle
    ASSERT_TRUE(parcel.WriteUint32(0));
    // isEnableBaselinePgo
    ASSERT_TRUE(parcel.WriteUint32(0));
    // bundleType
    ASSERT_TRUE(parcel.WriteInt32(0));
    // triggerType
    ASSERT_TRUE(parcel.WriteInt32(0));
    // hspSize = 2000 (> 1024)
    ASSERT_TRUE(parcel.WriteInt32(2000));

    auto *result = AotCompilerArgs::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: AotCompilerImplTest_089
* @tc.desc: ReadHspModules: hspSize < 0 returns false (AotCompilerArgs Unmarshalling)
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplFDTest, AotCompilerImplTest_089, TestSize.Level0)
{
    Parcel parcel;
    ASSERT_TRUE(parcel.WriteBool(false));
    ASSERT_TRUE(parcel.WriteString("partial"));
    ASSERT_TRUE(parcel.WriteString("dynamic"));
    ASSERT_TRUE(parcel.WriteString("com.test"));
    ASSERT_TRUE(parcel.WriteString(""));
    ASSERT_TRUE(parcel.WriteString("entry"));
    ASSERT_TRUE(parcel.WriteString("12345"));
    ASSERT_TRUE(parcel.WriteInt32(100));
    ASSERT_TRUE(parcel.WriteInt32(100));
    ASSERT_TRUE(parcel.WriteInt32(200));
    ASSERT_TRUE(parcel.WriteString("/test.hap"));
    ASSERT_TRUE(parcel.WriteString("/test.abc"));
    ASSERT_TRUE(parcel.WriteString("/test.an"));
    ASSERT_TRUE(parcel.WriteString("/output"));
    ASSERT_TRUE(parcel.WriteString(""));
    ASSERT_TRUE(parcel.WriteString("/pgo"));
    ASSERT_TRUE(parcel.WriteString(""));
    ASSERT_TRUE(parcel.WriteUint32(0));
    ASSERT_TRUE(parcel.WriteUint32(0));
    ASSERT_TRUE(parcel.WriteUint32(0));
    ASSERT_TRUE(parcel.WriteInt32(0));
    ASSERT_TRUE(parcel.WriteInt32(0));
    // hspSize = -1
    ASSERT_TRUE(parcel.WriteInt32(-1));

    auto *result = AotCompilerArgs::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: AotCompilerImplTest_CreateAnMemfd_001
* @tc.desc: CreateAnMemfd returns a valid writable fd
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_CreateAnMemfd_001, TestSize.Level0)
{
    AotCompilerImplMock mock;
    int32_t memfd = -1;
    int32_t ret = mock.CreateAnMemfdMock(memfd);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_GE(memfd, 0);
    // Verify the fd is writable
    const char data[] = "memfd test";
    ssize_t written = write(memfd, data, sizeof(data));
    EXPECT_EQ(written, static_cast<ssize_t>(sizeof(data)));
    panda::ecmascript::Close(memfd);
}

/**
* @tc.name: AotCompilerImplTest_PersistAnFile_001
* @tc.desc: PersistAnFile writes memfd content to a disk file
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_PersistAnFile_001, TestSize.Level0)
{
    int32_t memfd = memfd_create("test_persist", 0);
    ASSERT_GE(memfd, 0);
    const char data[] = "persist test data for an file";
    ssize_t written = write(memfd, data, sizeof(data));
    ASSERT_EQ(written, static_cast<ssize_t>(sizeof(data)));

    std::string diskPath = "/data/local/tmp/test_persist_memfd.an";
    AotCompilerImplMock mock;
    int32_t ret = mock.PersistAnFileMock(memfd, diskPath);
    EXPECT_EQ(ret, ERR_OK);

    // Verify disk file content matches
    FILE *diskFp = fopen(diskPath.c_str(), "rb");
    ASSERT_NE(diskFp, nullptr);
    char buf[256] = {0};
    size_t n = fread(buf, 1, sizeof(buf), diskFp);
    fclose(diskFp);
    EXPECT_EQ(n, sizeof(data));
    EXPECT_EQ(memcmp(buf, data, sizeof(data)), 0);

    close(memfd);
    unlink(diskPath.c_str());
}

/**
* @tc.name: AotCompilerImplTest_SpliceFdToFd_001
* @tc.desc: SpliceFdToFd transfers data between fds
* @tc.type: Func
*/
HWTEST_F(AotCompilerImplTest, AotCompilerImplTest_SpliceFdToFd_001, TestSize.Level0)
{
    int32_t srcFd = memfd_create("splice_src", 0);
    ASSERT_GE(srcFd, 0);
    const char data[] = "splice test data";
    ASSERT_EQ(write(srcFd, data, sizeof(data)), static_cast<ssize_t>(sizeof(data)));
    lseek(srcFd, 0, SEEK_SET);

    std::string diskPath = "/data/local/tmp/test_splice_dst.an";
    int32_t dstFd = open(diskPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    ASSERT_GE(dstFd, 0);
    panda::ecmascript::FdsanExchangeOwnerTag(dstFd);

    int32_t ret = AotCompilerImplMock::SpliceFdToFdMock(srcFd, dstFd, sizeof(data));
    EXPECT_EQ(ret, ERR_OK);

    // Verify content
    lseek(dstFd, 0, SEEK_SET);
    char buf[64] = {0};
    ssize_t n = read(dstFd, buf, sizeof(buf));
    panda::ecmascript::Close(dstFd);
    close(srcFd);
    EXPECT_EQ(n, static_cast<ssize_t>(sizeof(data)));
    EXPECT_EQ(memcmp(buf, data, sizeof(data)), 0);

    unlink(diskPath.c_str());
}

#endif // CODE_SIGN_ENABLE

} // namespace OHOS::ArkCompiler
