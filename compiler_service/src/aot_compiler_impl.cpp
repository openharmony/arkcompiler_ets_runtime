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

#include "aot_compiler_impl.h"

#include <charconv>
#include <sys/capability.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>

#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"
#ifdef CODE_SIGN_ENABLE
#include "local_code_sign_kit.h"
#endif
#ifdef ENABLE_HISYSEVENT
#include "ecmascript/compiler/aot_compiler_stats.h"
#endif

namespace OHOS::ArkCompiler {
AotCompilerImpl& AotCompilerImpl::GetInstance()
{
    static AotCompilerImpl aotCompiler;
    return aotCompiler;
}

void AotCompilerImpl::DropCapabilities() const
{
    LOG_SA(INFO) << "begin to drop capabilities";
    int32_t bundleUid = 0;
    int32_t bundleGid = 0;
    argsHandler_->GetBundleId(bundleUid, bundleGid);
    if (setgid(bundleGid)) {
        LOG_SA(ERROR) << "dropCapabilities setgid failed : " << strerror(errno);
        exit(-1);
    }
    if (setuid(bundleUid)) {
        LOG_SA(ERROR) << "dropCapabilities setuid failed : " << strerror(errno);
        exit(-1);
    }
    struct __user_cap_header_struct capHeader;
    if (memset_s(&capHeader, sizeof(capHeader), 0, sizeof(capHeader)) != EOK) {
        LOG_SA(ERROR) << "memset_s capHeader failed : " << strerror(errno);
        exit(-1);
    }
    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;
    struct __user_cap_data_struct capData[2];
    if (memset_s(&capData, sizeof(capData), 0, sizeof(capData)) != EOK) {
        LOG_SA(ERROR) << "memset_s capData failed : " << strerror(errno);
        exit(-1);
    }
    if (capset(&capHeader, capData) != 0) {
        LOG_SA(ERROR) << "capset failed : " << strerror(errno);
        exit(-1);
    }
    LOG_SA(INFO) << "drop capabilities success";
}
void AotCompilerImpl::ExecuteInChildProcess() const
{
    std::vector<const char*> argv = argsHandler_->GetAotArgs();
    LOG_SA(INFO) << "ark_aot_compiler argv size : " << argv.size();
    for (const auto &arg : argv) {
        LOG_SA(INFO) << arg;
    }
    argv.emplace_back(nullptr);
    LOG_SA(INFO) << "begin to execute ark_aot_compiler";
    execv(argv[0], const_cast<char* const*>(argv.data()));
    LOG_SA(ERROR) << "execv failed : " << strerror(errno);
    exit(-1);
}

int32_t AotCompilerImpl::PrintAOTCompilerResult(const int compilerStatus) const
{
    if (RetInfoOfCompiler.find(compilerStatus) == RetInfoOfCompiler.end()) {
        LOG_SA(ERROR) << OtherInfoOfCompiler.mesg;
        return OtherInfoOfCompiler.retCode;
    }
    if (RetInfoOfCompiler.at(compilerStatus).retCode == ERR_AOT_COMPILER_CALL_FAILED) {
        LOG_SA(ERROR) << RetInfoOfCompiler.at(compilerStatus).mesg;
    } else {
        LOG_SA(INFO) << RetInfoOfCompiler.at(compilerStatus).mesg;
    }
    return RetInfoOfCompiler.at(compilerStatus).retCode;
}

void AotCompilerImpl::ExecuteInParentProcess(const pid_t childPid, int32_t &ret)
{
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        InitState(childPid);
    }
    int status;
    int waitRet = waitpid(childPid, &status, 0);
    if (waitRet == -1) {
        LOG_SA(ERROR) << "waitpid failed";
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFEXITED(status)) {
        int exitStatus = WEXITSTATUS(status);
        LOG_SA(INFO) << "child process exited with status: " << exitStatus;
        ret = PrintAOTCompilerResult(exitStatus);
    } else if (WIFSIGNALED(status)) {
        int signalNumber = WTERMSIG(status);
        LOG_SA(WARN) << "child process terminated by signal: " << signalNumber;
        ret = signalNumber == SIGKILL ? ERR_AOT_COMPILER_CALL_CANCELLED : ERR_AOT_COMPILER_CALL_CRASH;
    } else if (WIFSTOPPED(status)) {
        int signalNumber = WSTOPSIG(status);
        LOG_SA(WARN) << "child process was stopped by signal: " << signalNumber;
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFCONTINUED(status)) {
        LOG_SA(WARN) << "child process was resumed";
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else {
        LOG_SA(WARN) << "unknown";
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    }
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        ResetState();
    }
}

bool AotCompilerImpl::VerifyCompilerModeAndPkgInfo(const std::unordered_map<std::string, std::string> &argsMap)
{
    auto targetCompilerMode = argsMap.find(ArgsIdx::TARGET_COMPILER_MODE);
    if (targetCompilerMode == argsMap.end() || targetCompilerMode->second.empty()) {
        return false;
    }
    auto compilerPkgInfo = argsMap.find(ArgsIdx::COMPILER_PKG_INFO);
    if (compilerPkgInfo == argsMap.end() || compilerPkgInfo->second.empty()) {
        return false;
    }
    return true;
}

int32_t AotCompilerImpl::EcmascriptAotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                               std::vector<int16_t> &sigData)
{
#ifdef CODE_SIGN_ENABLE
    if (!allowAotCompiler_) {
        LOG_SA(ERROR) << "aot compiler is not allowed now";
        return ERR_AOT_COMPILER_CALL_CANCELLED;
    }
    if (!VerifyCompilerModeAndPkgInfo(argsMap)) {
        LOG_SA(ERROR) << "aot compiler mode or pkginfo arguments error";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    argsHandler_ = std::make_unique<AOTArgsHandler>(argsMap);
    if (argsHandler_->Handle(thermalLevel_) != ERR_OK) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    int32_t ret = ERR_OK;
    LOG_SA(INFO) << "begin to fork";
    pid_t pid = fork();
    if (pid == -1) {
        LOG_SA(ERROR) << "fork process failed : " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    } else if (pid == 0) {
        DropCapabilities();
        ExecuteInChildProcess();
    } else {
        ExecuteInParentProcess(pid, ret);
    }
    if (ret == ERR_OK_NO_AOT_FILE) {
        return ERR_OK;
    }
    SendSysEvent(argsMap);
    return ret != ERR_OK ? ret : AOTLocalCodeSign(sigData);
#else
    LOG_SA(ERROR) << "no need to AOT compile when code signature disable";
    return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
#endif
}

int32_t AotCompilerImpl::SendSysEvent(const std::unordered_map<std::string, std::string> &argsMap) const
{
#ifdef ENABLE_HISYSEVENT
    panda::ecmascript::AotCompilerStats aotCompilerStats;
    return aotCompilerStats.SendDataPartitionSysEvent(ParseArkCacheFromArgs(argsMap));
#endif
    return 0;
}

std::string AotCompilerImpl::ParseArkCacheFromArgs(const std::unordered_map<std::string, std::string> &argsMap) const
{
    if (argsMap.find(ArgsIdx::AOT_FILE) == argsMap.end()) {
        LOG_SA(ERROR) << "aot compiler SendSysEvent parsing an or ap path error";
        return "";
    }
    std::string aotFile = argsMap.at(ArgsIdx::AOT_FILE);
    auto index = aotFile.find_last_of('/');
    if (index == std::string::npos) {
        LOG_SA(ERROR) << "ark-cache path find / error";
        return "";
    }
    return aotFile.substr(0, index);
}

int32_t AotCompilerImpl::GetAOTVersion(std::string& sigData)
{
    LOG_SA(INFO) << "AotCompilerImpl::GetAOTVersion";
    sigData = panda::ecmascript::AOTFileVersion::GetAOTVersion();

    return ERR_OK;
}

int32_t AotCompilerImpl::NeedReCompile(const std::string& args, bool& sigData)
{
    LOG_SA(INFO) << "AotCompilerImpl::NeedReCompile";
    sigData = panda::ecmascript::AOTFileVersion::CheckAOTVersion(args);
    return ERR_OK;
}

int32_t AotCompilerImpl::AOTLocalCodeSign(std::vector<int16_t> &sigData) const
{
#ifdef CODE_SIGN_ENABLE
    std::string appSignature = argsHandler_->GetCodeSignArgs();
    std::string fileName = argsHandler_->GetFileName();
    Security::CodeSign::ByteBuffer sig;
    if (Security::CodeSign::LocalCodeSignKit::SignLocalCode(appSignature, fileName, sig)
                        != CommonErrCode::CS_SUCCESS) {
        LOG_SA(ERROR) << "failed to sign the aot file";
        return ERR_AOT_COMPILER_SIGNATURE_FAILED;
    }
    LOG_SA(INFO) << "aot file local sign success";
    uint8_t *dataPtr = sig.GetBuffer();
    for (uint32_t i = 0; i < sig.GetSize(); ++i) {
        sigData.emplace_back(static_cast<int16_t>(dataPtr[i]));
    }
    return ERR_OK;
#else
    LOG_SA(ERROR) << "no need to AOT local code sign when code signature disable";
    return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
#endif
}

int32_t AotCompilerImpl::StopAotCompiler()
{
    LOG_SA(INFO) << "begin to stop AOT";
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (!state_.running) {
        LOG_SA(INFO) << "AOT not running, return directly";
        return ERR_AOT_COMPILER_STOP_FAILED;
    }
    if (state_.childPid <= 0) {
        LOG_SA(ERROR) << "invalid child pid";
        return ERR_AOT_COMPILER_STOP_FAILED;
    }
    LOG_SA(INFO) << "begin to kill child process : " << state_.childPid;
    auto result = kill(state_.childPid, SIGKILL);
    int32_t ret = RemoveAotFiles();
    if (result != 0) {
        LOG_SA(INFO) << "kill child process failed: " << result;
        ret = ERR_AOT_COMPILER_STOP_FAILED;
    } else {
        LOG_SA(INFO) << "kill child process success";
    }
    ResetState();
    return ret;
}

int32_t AotCompilerImpl::RemoveAotFiles() const
{
    std::string fileName = argsHandler_->GetFileName();
    if (access(fileName.c_str(), ERR_OK) != ERR_FAIL) {
        auto delRes = std::remove(fileName.c_str());
        if (delRes != ERR_OK) {
            LOG_SA(INFO) << "delete invalid aot file failed: " << delRes;
            return ERR_AOT_COMPILER_STOP_FAILED;
        } else {
            LOG_SA(INFO) << "delete invalid aot file success";
        }
    }
    return ERR_OK;
}

void AotCompilerImpl::HandlePowerDisconnected()
{
    LOG_SA(INFO) << "AotCompilerImpl::HandlePowerDisconnected";
    PauseAotCompiler();
    std::thread t([]() {
        (void)AotCompilerImpl::GetInstance().StopAotCompiler();
        sleep(30);  // wait for 30 seconds
        AotCompilerImpl::GetInstance().AllowAotCompiler();
    });
    if (t.joinable()) {
        t.detach();
    } else {
        LOG_SA(ERROR) << "Failed to create thread for AotCompilerImpl::HandlePowerDisconnected";
    }
}

void AotCompilerImpl::HandleScreenOn()
{
    LOG_SA(INFO) << "AotCompilerImpl::HandleScreenOn";
    PauseAotCompiler();
    std::thread t([]() {
        (void)AotCompilerImpl::GetInstance().StopAotCompiler();
        sleep(40);  // wait for 40 seconds
        AotCompilerImpl::GetInstance().AllowAotCompiler();
    });
    if (t.joinable()) {
        t.detach();
    } else {
        LOG_SA(ERROR) << "Failed to create thread for AotCompilerImpl::HandleScreenOn";
    }
}

void AotCompilerImpl::HandleThermalLevelChanged(const int32_t level)
{
    LOG_SA(INFO) << "AotCompilerImpl::HandleThermalLevelChanged";
    thermalLevel_ = level;
    // thermal level >= 2, stop aot compile
    if (thermalLevel_ >= AOT_COMPILE_STOP_LEVEL) {
        PauseAotCompiler();
    } else {
        AllowAotCompiler();
    }
}

void AotCompilerImpl::PauseAotCompiler()
{
    LOG_SA(INFO) << "AotCompilerImpl::PauseAotCompiler";
    allowAotCompiler_ = false;
}

void AotCompilerImpl::AllowAotCompiler()
{
    LOG_SA(INFO) << "AotCompilerImpl::AllowAotCompiler";
    allowAotCompiler_ = true;
}

void AotCompilerImpl::InitState(const pid_t childPid)
{
    state_.running = true;
    state_.childPid = childPid;
}

void AotCompilerImpl::ResetState()
{
    state_.running = false;
    state_.childPid = -1;
}
} // namespace OHOS::ArkCompiler
