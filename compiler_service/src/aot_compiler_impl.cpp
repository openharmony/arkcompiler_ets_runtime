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

#include "aot_compiler_impl.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <linux/capability.h>
#include <sys/capability.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>

#include "aot_compiler_constants.h"
#include "aot_compiler_error_utils.h"
#include "aot_compiler_service.h"
#include "hilog/log.h"
#include "hitrace_meter.h"
#include "local_code_sign_kit.h"
#include "system_ability_definition.h"

namespace OHOS::ArkCompiler {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD001800, "aot_compiler_service"};
} // namespace

AotCompilerImpl& AotCompilerImpl::GetInstance()
{
    static AotCompilerImpl aotCompiler;
    return aotCompiler;
}

void AotCompilerImpl::PrepareArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    for (const auto &arg : argsMap) {
        HiviewDFX::HiLog::Debug(LABEL, "%{public}s: %{public}s", arg.first.c_str(), arg.second.c_str());
    }
    hapArgs.bundleUid = static_cast<int32_t>(std::stoi(argsMap.at(ArgsIdx::BUNDLE_UID)));
    hapArgs.bundleGid = static_cast<int32_t>(std::stoi(argsMap.at(ArgsIdx::BUNDLE_GID)));
    hapArgs.fileName = argsMap.at(ArgsIdx::AN_FILE_NAME);
    hapArgs.signature = argsMap.at(ArgsIdx::APP_SIGNATURE);
    hapArgs.argVector.clear();
    hapArgs.argVector.emplace_back(Cmds::ARK_AOT_COMPILER);
    for (auto &argPair : argsMap) {
        if (AotArgsSet.find(argPair.first) != AotArgsSet.end()) {
            hapArgs.argVector.emplace_back(Symbols::PREFIX + argPair.first + Symbols::EQ + argPair.second);
        }
    }
    hapArgs.argVector.emplace_back(argsMap.at(ArgsIdx::ABC_PATH));
}

void AotCompilerImpl::DropCapabilities(const int32_t &bundleUid, const int32_t &bundleGid) const
{
    if (setuid(bundleUid)) {
        HiviewDFX::HiLog::Error(LABEL, "dropCapabilities setuid failed : %{public}s", strerror(errno));
        exit(-1);
    }
    if (setgid(bundleGid)) {
        HiviewDFX::HiLog::Error(LABEL, "dropCapabilities setgid failed : %{public}s", strerror(errno));
        exit(-1);
    }
    struct __user_cap_header_struct capHeader;
    if (memset_s(&capHeader, sizeof(capHeader), 0, sizeof(capHeader)) != EOK) {
        HiviewDFX::HiLog::Error(LABEL, "memset_s capHeader failed : %{public}s", strerror(errno));
        exit(-1);
    }
    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;

    struct __user_cap_data_struct capData[2];
    if (memset_s(&capData, sizeof(capData), 0, sizeof(capData)) != EOK) {
        HiviewDFX::HiLog::Error(LABEL, "memset_s capData failed : %{public}s", strerror(errno));
        exit(-1);
    }
    if (capset(&capHeader, capData) != 0) {
        HiviewDFX::HiLog::Error(LABEL, "capset failed : %{public}s", strerror(errno));
        exit(-1);
    }
}

void AotCompilerImpl::ExecuteInChildProcess(const std::vector<std::string> &aotVector) const
{
    std::vector<const char*> argv;
    argv.reserve(aotVector.size() + 1);
    for (auto &arg : aotVector) {
        argv.emplace_back(arg.c_str());
    }
    argv.emplace_back(nullptr);
    HiviewDFX::HiLog::Debug(LABEL, "argv size : %{public}zu", argv.size());
    for (const auto &arg : argv) {
        HiviewDFX::HiLog::Debug(LABEL, "%{public}s", arg);
    }
    execv(argv[0], const_cast<char* const*>(argv.data()));
    HiviewDFX::HiLog::Error(LABEL, "execv failed : %{public}s", strerror(errno));
    exit(-1);
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
        HiviewDFX::HiLog::Error(LABEL, "waitpid failed");
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        HiviewDFX::HiLog::Info(LABEL, "child process exited with status: %{public}d", exit_status);
        switch (exit_status) {
            case ErrOfCompile::COMPILE_OK:
                ret = ERR_OK; break;
            case ErrOfCompile::COMPILE_NO_AP:
                ret = ERR_OK_NO_AOT_FILE; break;
            default:
                ret = ERR_AOT_COMPILER_CALL_FAILED; break;
        }
    } else if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        HiviewDFX::HiLog::Warn(LABEL, "child process terminated by signal: %{public}d", signal_number);
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFSTOPPED(status)) {
        int signal_number = WSTOPSIG(status);
        HiviewDFX::HiLog::Warn(LABEL, "child process was stopped by signal: %{public}d", signal_number);
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFCONTINUED(status)) {
        HiviewDFX::HiLog::Warn(LABEL, "child process was resumed");
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else {
        HiviewDFX::HiLog::Warn(LABEL, "unknown");
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    }
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        ResetState();
    }
}

int32_t AotCompilerImpl::EcmascriptAotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                               std::vector<int16_t> &sigData)
{
    if (!allowAotCompiler_) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    if (argsMap.empty()) {
        HiviewDFX::HiLog::Error(LABEL, "aot compiler arguments error");
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    PrepareArgs(argsMap);
    int32_t ret = ERR_OK;
    std::lock_guard<std::mutex> lock(mutex_);
    HiviewDFX::HiLog::Debug(LABEL, "begin to fork");
    pid_t pid = fork();
    if (pid == -1) {
        HiviewDFX::HiLog::Error(LABEL, "fork process failed : %{public}s", strerror(errno));
        return ERR_AOT_COMPILER_CALL_FAILED;
    } else if (pid == 0) {
        DropCapabilities(hapArgs.bundleUid, hapArgs.bundleGid);
        ExecuteInChildProcess(hapArgs.argVector);
    } else {
        ExecuteInParentProcess(pid, ret);
    }
    if (ret == ERR_OK_NO_AOT_FILE) {
        return ERR_OK;
    }
    return ret ? ERR_AOT_COMPILER_CALL_FAILED : AOTLocalCodeSign(hapArgs.fileName, hapArgs.signature, sigData);
}

int32_t AotCompilerImpl::AOTLocalCodeSign(const std::string &fileName, const std::string &appSignature,
                                          std::vector<int16_t> &sigData)
{
    Security::CodeSign::ByteBuffer sig;
    if (Security::CodeSign::LocalCodeSignKit::SignLocalCode(appSignature, fileName, sig)
                        != CommonErrCode::CS_SUCCESS) {
        HiviewDFX::HiLog::Error(LABEL, "failed to sign the aot file");
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    HiviewDFX::HiLog::Debug(LABEL, "aot file local sign success");
    uint8_t *dataPtr = sig.GetBuffer();
    for (uint32_t i = 0; i < sig.GetSize(); ++i) {
        sigData.emplace_back(static_cast<int16_t>(dataPtr[i]));
    }
    return ERR_OK;
}

int32_t AotCompilerImpl::StopAotCompiler()
{
    HiviewDFX::HiLog::Info(LABEL, "begin to stop AOT");
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (!state_.running) {
        HiviewDFX::HiLog::Info(LABEL, "AOT not running, return directly");
        return ERR_AOT_COMPILER_STOP_FAILED;
    }
    if (state_.childPid <= 0) {
        HiviewDFX::HiLog::Error(LABEL, "invalid child pid");
        return ERR_AOT_COMPILER_STOP_FAILED;
    }
    HiviewDFX::HiLog::Info(LABEL, "begin to kill child process : %{public}d", state_.childPid);
    auto result = kill(state_.childPid, SIGKILL);
    if (result != 0) {
        HiviewDFX::HiLog::Info(LABEL, "kill child process failed: %{public}d", result);
    } else {
        HiviewDFX::HiLog::Info(LABEL, "kill child process success");
    }
    ResetState();
    return ERR_OK;
}

void AotCompilerImpl::HandlePowerDisconnected()
{
    HiviewDFX::HiLog::Info(LABEL, "AotCompilerImpl::HandlePowerDisconnected");
    PauseAotCompiler();
    auto task = []() {
        AotCompilerImpl::GetInstance().StopAotCompiler();
        sleep(30);
        AotCompilerImpl::GetInstance().AllowAotCompiler();
    };
    std::thread(task).detach();
}

void AotCompilerImpl::PauseAotCompiler()
{
    HiviewDFX::HiLog::Info(LABEL, "AotCompilerImpl::PauseAotCompiler");
    allowAotCompiler_ = false;
}

void AotCompilerImpl::AllowAotCompiler()
{
    HiviewDFX::HiLog::Info(LABEL, "AotCompilerImpl::AllowAotCompiler");
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
} // namespace ArkCompiler::OHOS