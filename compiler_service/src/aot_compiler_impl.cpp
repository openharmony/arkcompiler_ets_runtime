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
#include "ecmascript/log_wrapper.h"
#include "hitrace_meter.h"
#ifdef CODE_SIGN_ENABLE
#include "local_code_sign_kit.h"
#include <sys/syscall.h>
#endif
#include "system_ability_definition.h"

namespace OHOS::ArkCompiler {
#ifdef CODE_SIGN_ENABLE
static pid_t ForkBySyscall(void)
{
#ifdef SYS_fork
    return syscall(SYS_fork);
#else
    return syscall(SYS_clone, SIGCHLD, 0);
#endif
}
#endif

AotCompilerImpl& AotCompilerImpl::GetInstance()
{
    static AotCompilerImpl aotCompiler;
    return aotCompiler;
}

inline int32_t AotCompilerImpl::FindArgsIdxToInteger(const std::unordered_map<std::string, std::string> &argsMap,
                                                     const std::string &keyName, int32_t &bundleID)
{
    if (argsMap.find(keyName) == argsMap.end()) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    size_t sz;
    bundleID = static_cast<int32_t>(std::stoi(argsMap.at(keyName), &sz));
    if (sz < static_cast<size_t>(argsMap.at(keyName).size())) {
        LOG_SA(ERROR) << "trigger exception as converting string to integer";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    return ERR_OK;
}

inline int32_t AotCompilerImpl::FindArgsIdxToString(const std::unordered_map<std::string, std::string> &argsMap,
                                                    const std::string &keyName, std::string &bundleArg)
{
    if (argsMap.find(keyName) == argsMap.end()) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    bundleArg = argsMap.at(keyName);
    return ERR_OK;
}

int32_t AotCompilerImpl::PrepareArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    for (const auto &arg : argsMap) {
        LOG_SA(DEBUG) << arg.first << ": " << arg.second;
    }
    std::string abcPath;
    if ((FindArgsIdxToInteger(argsMap, ArgsIdx::BUNDLE_UID, hapArgs.bundleUid) != ERR_OK)   ||
        (FindArgsIdxToInteger(argsMap, ArgsIdx::BUNDLE_GID, hapArgs.bundleGid) != ERR_OK)   ||
        (FindArgsIdxToString(argsMap, ArgsIdx::AN_FILE_NAME, hapArgs.fileName) != ERR_OK)   ||
        (FindArgsIdxToString(argsMap, ArgsIdx::APP_SIGNATURE, hapArgs.signature) != ERR_OK) ||
        (FindArgsIdxToString(argsMap, ArgsIdx::ABC_PATH, abcPath) != ERR_OK)) {
        LOG_SA(ERROR) << "aot compiler Args parsing error";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    hapArgs.argVector.clear();
    hapArgs.argVector.emplace_back(Cmds::ARK_AOT_COMPILER);
    for (auto &argPair : argsMap) {
        if (AotArgsSet.find(argPair.first) != AotArgsSet.end()) {
            hapArgs.argVector.emplace_back(Symbols::PREFIX + argPair.first + Symbols::EQ + argPair.second);
        }
    }
    hapArgs.argVector.emplace_back(abcPath);
    return ERR_OK;
}

void AotCompilerImpl::DropCapabilities(const int32_t &bundleUid, const int32_t &bundleGid) const
{
    if (setuid(bundleUid)) {
        LOG_SA(ERROR) << "dropCapabilities setuid failed : " << strerror(errno);
        exit(-1);
    }
    if (setgid(bundleGid)) {
        LOG_SA(ERROR) << "dropCapabilities setgid failed : " << strerror(errno);
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
}

void AotCompilerImpl::ExecuteInChildProcess(const std::vector<std::string> &aotVector) const
{
    std::vector<const char*> argv;
    argv.reserve(aotVector.size() + 1);
    for (auto &arg : aotVector) {
        argv.emplace_back(arg.c_str());
    }
    LOG_SA(DEBUG) << "argv size : " << argv.size();
    for (const auto &arg : argv) {
        LOG_SA(DEBUG) << arg;
    }
    argv.emplace_back(nullptr);
    execv(argv[0], const_cast<char* const*>(argv.data()));
    LOG_SA(ERROR) << "execv failed : " << strerror(errno);
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
        LOG_SA(ERROR) << "waitpid failed";
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        LOG_SA(INFO) << "child process exited with status: " << exit_status;
        switch (exit_status) {
            case static_cast<int>(ErrOfCompile::COMPILE_OK):
                ret = ERR_OK; break;
            case static_cast<int>(ErrOfCompile::COMPILE_NO_AP):
                ret = ERR_OK_NO_AOT_FILE; break;
            default:
                ret = ERR_AOT_COMPILER_CALL_FAILED; break;
        }
    } else if (WIFSIGNALED(status)) {
        int signal_number = WTERMSIG(status);
        LOG_SA(WARN) << "child process terminated by signal: " << signal_number;
        ret = ERR_AOT_COMPILER_CALL_FAILED;
    } else if (WIFSTOPPED(status)) {
        int signal_number = WSTOPSIG(status);
        LOG_SA(WARN) << "child process was stopped by signal: " << signal_number;
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

int32_t AotCompilerImpl::EcmascriptAotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                               std::vector<int16_t> &sigData)
{
#ifdef CODE_SIGN_ENABLE
    if (!allowAotCompiler_) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    if (argsMap.empty() || (PrepareArgs(argsMap) != ERR_OK)) {
        LOG_SA(ERROR) << "aot compiler arguments error";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    int32_t ret = ERR_OK;
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_SA(DEBUG) << "begin to fork";
    pid_t pid = ForkBySyscall();
    if (pid == -1) {
        LOG_SA(ERROR) << "fork process failed : " << strerror(errno);
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
#else
    LOG_SA(ERROR) << "no need to AOT compile when code signature disable";
    return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
#endif
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

int32_t AotCompilerImpl::AOTLocalCodeSign(const std::string &fileName, const std::string &appSignature,
                                          std::vector<int16_t> &sigData)
{
#ifdef CODE_SIGN_ENABLE
    Security::CodeSign::ByteBuffer sig;
    if (Security::CodeSign::LocalCodeSignKit::SignLocalCode(appSignature, fileName, sig)
                        != CommonErrCode::CS_SUCCESS) {
        LOG_SA(ERROR) << "failed to sign the aot file";
        return ERR_AOT_COMPILER_SIGNATURE_FAILED;
    }
    LOG_SA(DEBUG) << "aot file local sign success";
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
    if (result != 0) {
        LOG_SA(INFO) << "kill child process failed: " << result;
    } else {
        LOG_SA(INFO) << "kill child process success";
    }
    ResetState();
    return ERR_OK;
}

void AotCompilerImpl::HandlePowerDisconnected()
{
    LOG_SA(INFO) << "AotCompilerImpl::HandlePowerDisconnected";
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


} // namespace ArkCompiler::OHOS