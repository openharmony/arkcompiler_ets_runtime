/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include <cstring>
#include <map>
#include <dlfcn.h>
#include <fcntl.h>
#include <csignal>
#include <sys/capability.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>

#include "aot_args_verify.h"
#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/ohos/enable_aot_list_helper.h"
#include "ecmascript/platform/directory.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/os.h"
#include <directory_ex.h>
#ifdef CODE_SIGN_ENABLE
#include "local_code_sign_kit.h"
#endif
#ifdef ENABLE_HISYSEVENT
#include "ecmascript/compiler/aot_compiler_stats.h"
#endif

namespace OHOS::ArkCompiler {

const std::string USER_MODE_UNSLEEP = "unsleep";
AotCompilerImpl& AotCompilerImpl::GetInstance()
{
    static AotCompilerImpl aotCompiler;
    return aotCompiler;
}

// LCOV_EXCL_START

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
    int32_t stopRet = ERR_OK;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        InitState(childPid);
        if (stopRequested_.load() || !allowAotCompiler_.load()) {
            LOG_SA(INFO) << "aot compiler is not allowed after fork, stop child process: " << childPid;
            auto result = kill(childPid, SIGKILL);
            stopRet = RemoveAotFiles();
            if (result != 0) {
                LOG_SA(ERROR) << "cancel after fork failed to kill child process: " << result;
                stopRet = ERR_AOT_COMPILER_STOP_FAILED;
            } else {
                if (stopRet != ERR_OK) {
                    LOG_SA(ERROR) << "remove aot files failed: " << stopRet;
                }
                LOG_SA(INFO) << "cancel after fork killed child process";
            }
        }
    }
    ret = ERR_AOT_COMPILER_CALL_FAILED;
    int status;
    if (waitpid(childPid, &status, 0) == -1) {
        LOG_SA(ERROR) << "waitpid failed";
    } else if (WIFEXITED(status)) {
        int exitStatus = WEXITSTATUS(status);
        LOG_SA(INFO) << "child process exited with status: " << exitStatus;
        ret = PrintAOTCompilerResult(exitStatus);
    } else if (WIFSIGNALED(status)) {
        int signalNumber = WTERMSIG(status);
        LOG_SA(WARN) << "child process terminated by signal: " << signalNumber;
        ret = signalNumber == SIGKILL ? ERR_AOT_COMPILER_CALL_CANCELLED : ERR_AOT_COMPILER_CALL_CRASH;
    } else if (WIFSTOPPED(status)) {
        LOG_SA(WARN) << "child process was stopped by signal: " << WSTOPSIG(status);
    } else if (WIFCONTINUED(status)) {
        LOG_SA(WARN) << "child process was resumed";
    } else {
        LOG_SA(WARN) << "unknown";
    }
    ret = stopRet != ERR_OK ? stopRet : ret;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        ResetState();
    }
}

bool AotCompilerImpl::CheckAotWhitelist(const AotCompilerArgs &args) const
{
    auto helper = panda::ecmascript::ohos::EnableAotJitListHelper::GetInstance();
    if (helper->IsEnableAot(args.bundleName)) {
        return true;
    }
    return helper->IsEnableAot(args.appIdentifier);
}
// LCOV_EXCL_STOP

int32_t AotCompilerImpl::OpenHapFile(const AotCompilerArgs &args, CompilationCleanupGuard &guard)
{
    if (args.hapPath.empty()) {
        return ERR_OK;
    }
    std::string realHapPath;
    if (!panda::ecmascript::RealPath(args.hapPath.c_str(), realHapPath, true)) {
        LOG_SA(ERROR) << "failed to resolve HAP path: " << args.hapPath;
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    int32_t hapFd = open(realHapPath.c_str(), O_RDONLY);
    if (hapFd < 0) {
        LOG_SA(ERROR) << "failed to open HAP: " << realHapPath
            << " errno=" << errno << " " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    guard.AcquireHapFd(hapFd);
    LOG_SA(INFO) << "pre-opened HAP fd=" << hapFd << " path=" << realHapPath;
    return ERR_OK;
}

int32_t AotCompilerImpl::OpenHspFiles(const AotCompilerArgs &args, CompilationCleanupGuard &guard)
{
    for (const auto &hsp : args.hspModules) {
        if (hsp.hapPath.empty()) {
            continue;
        }
        std::string realHspPath;
        if (!panda::ecmascript::RealPath(hsp.hapPath.c_str(), realHspPath, true)) {
            LOG_SA(ERROR) << "failed to resolve HSP path: " << hsp.hapPath;
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        int32_t hspFd = open(realHspPath.c_str(), O_RDONLY);
        if (hspFd < 0) {
            LOG_SA(ERROR) << "failed to open HSP: " << realHspPath
                << " errno=" << errno << " " << strerror(errno);
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        guard.AcquireHspFd(hspFd);
        LOG_SA(INFO) << "pre-opened HSP fd=" << hspFd << " path=" << realHspPath;
    }
    return ERR_OK;
}

void AotCompilerImpl::UpdateExternalPkgFdInfo(const AotCompilerArgs &args,
    const CompilationCleanupGuard &guard)
{
    if (guard.GetHspFds().empty()) {
        return;
    }
    std::map<std::string, int32_t> hspFdMap;
    for (size_t i = 0; i < args.hspModules.size() && i < guard.GetHspFds().size(); ++i) {
        hspFdMap[args.hspModules[i].hapPath] = guard.GetHspFds()[i];
    }
    std::string extPkgInfo = AOTArgsHandler::BuildExternalPkgInfo(args, hspFdMap);
    argsHandler_->UpdateExternalPkgInfo(extPkgInfo);
}

int32_t AotCompilerImpl::PrepareCompilationInputs(const AotCompilerArgs &args,
    AotParserType parserType, CompilationCleanupGuard &guard)
{
    int32_t hapRet = OpenHapFile(args, guard);
    if (hapRet != ERR_OK) {
        return hapRet;
    }

    int32_t hspRet = OpenHspFiles(args, guard);
    if (hspRet != ERR_OK) {
        return hspRet;
    }
    UpdateExternalPkgFdInfo(args, guard);

    int32_t anFd = -1;
    int32_t outputRet = PrepareOutputFiles(parserType,
        static_cast<uid_t>(args.bundleUid), static_cast<gid_t>(args.bundleGid), anFd);
    if (outputRet == ERR_OK_AOT_FILE_EXIST) {
        LOG_SA(INFO) << "AOT file already exists, skip compilation";
        return ERR_OK_AOT_FILE_EXIST;
    }
    if (outputRet == ERR_OK) {
        guard.AcquireFd(anFd);
    }
    if (outputRet != ERR_OK) {
        return outputRet;
    }
    if (stopRequested_.load() || !allowAotCompiler_.load()) {
        LOG_SA(ERROR) << "aot compiler is not allowed before fork";
        return ERR_AOT_COMPILER_CALL_CANCELLED;
    }
    return ERR_OK;
}

int32_t AotCompilerImpl::EcmascriptAotCompiler(const AotCompilerArgs &args,
                                               std::vector<uint8_t> &sigData)
{
#ifdef CODE_SIGN_ENABLE
    stopRequested_.store(false);
    if (!allowAotCompiler_) {
        LOG_SA(ERROR) << "aot compiler is not allowed now";
        return ERR_AOT_COMPILER_CALL_CANCELLED;
    }
    int32_t prepareRet = PrepareArgsHandler(args);
    if (prepareRet != ERR_OK) {
        return prepareRet;
    }

    AotParserType parserType = argsHandler_->GetParserType();
    LOG_SA(INFO) << "parserType=" << static_cast<int32_t>(parserType);

    if (parserType == AotParserType::DYNAMIC_AOT && !CheckAotWhitelist(args)) {
        LOG_SA(INFO) << "bundle not in AOT whitelist, skip compilation: " << args.bundleName;
        return ERR_AOT_COMPILER_WHITELIST_BLOCKED;
    }

    CompilationCleanupGuard guard(argsHandler_->GetFileName());

    int32_t inputRet = PrepareCompilationInputs(args, parserType, guard);
    if (inputRet == ERR_OK_AOT_FILE_EXIST) {
        return ERR_OK;
    }
    if (inputRet != ERR_OK) {
        return inputRet;
    }

    int32_t ret = ERR_OK;
    int32_t execRet = ForkAndExecute(parserType, args, ret, guard.GetFd(), guard.GetHapFd(), guard.GetHspFds());
    if (execRet != ERR_OK) {
        return execRet;
    }
    return HandlePostCompilation(args, ret, sigData, guard);
#else
    LOG_SA(ERROR) << "no need to AOT compile when code signature disable";
    return ERR_AOT_COMPILER_SIGNATURE_DISABLE;
#endif
}

int32_t AotCompilerImpl::PrepareArgsHandler(const AotCompilerArgs &args)
{
    if (!allowAotCompiler_) {
        LOG_SA(ERROR) << "aot compiler is not allowed now";
        return ERR_AOT_COMPILER_CALL_CANCELLED;
    }
    argsHandler_ = std::make_unique<AOTArgsHandler>(args);
    if (argsHandler_->Handle(thermalLevel_) != ERR_OK) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    return ERR_OK;
}

int32_t AotCompilerImpl::ForkAndExecute(AotParserType parserType,
    const AotCompilerArgs &args, int32_t &ret, int32_t anFd, int32_t hapFd, const std::vector<int32_t> &hspFds)
{
    pid_t pid = fork();
    if (pid == -1) {
        LOG_SA(ERROR) << "ForkAndExecute: fork failed: " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    } else if (pid == 0) {
        PrepareChildProcess(anFd, hapFd, hspFds);
        DropCapabilities();
        ExecuteInChildProcess();
    } else {
        ret = HapVerifyInParent(args, pid);
        if (ret != ERR_OK) {
            return ret;
        }
        ExecuteInParentProcess(pid, ret);
        LOG_SA(INFO) << "ForkAndExecute: child finished, ret=" << ret;
    }
    return ERR_OK;
}

int32_t AotCompilerImpl::SendSysEvent(const AotCompilerArgs &args) const
{
#ifdef ENABLE_HISYSEVENT
    panda::ecmascript::AotCompilerStats aotCompilerStats;
    return aotCompilerStats.SendDataPartitionSysEvent(ParseArkCacheFromArgs(args));
#endif
    return 0;
}

std::string AotCompilerImpl::ParseArkCacheFromArgs(const AotCompilerArgs &args) const
{
    // Use anFileName to derive ark_cache path
    if (args.anFileName.empty()) {
        LOG_SA(ERROR) << "aot compiler SendSysEvent parsing an path error";
        return "";
    }
    // anFileName: .../ark_cache/<bundleName>/arm64/module.an -> find ark_cache/<bundleName>/
    auto arkCacheIdx = args.anFileName.find(ArgsIdx::APP_ARK_CACHE_PREFIX);
    if (arkCacheIdx == std::string::npos) {
        LOG_SA(ERROR) << "ark-cache path not found in anFileName";
        return "";
    }
    auto afterPrefix = args.anFileName.substr(arkCacheIdx + ArgsIdx::APP_ARK_CACHE_PREFIX.size());
    auto slashIdx = afterPrefix.find('/');
    if (slashIdx == std::string::npos) {
        return args.anFileName.substr(arkCacheIdx, args.anFileName.size() - arkCacheIdx);
    }
    return args.anFileName.substr(arkCacheIdx, ArgsIdx::APP_ARK_CACHE_PREFIX.size() + slashIdx);
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

// LCOV_EXCL_START
int32_t AotCompilerImpl::AOTLocalCodeSign(std::vector<uint8_t> &sigData, int32_t anFd) const
{
#ifdef CODE_SIGN_ENABLE
    std::string appSignature = argsHandler_->GetCodeSignArgs();
    std::string fileName = argsHandler_->GetFileName();
    if (!AotArgsVerify::CheckCodeSignArkCacheFilePath(fileName)) {
        LOG_SA(ERROR) << "fileName validation failed in AOTLocalCodeSign";
        return ERR_AOT_COMPILER_SIGNATURE_FAILED;
    }

    int32_t fd = anFd;
    if (fd < 0) {
        LOG_SA(ERROR) << "no valid fd for SignLocalCodeByFd, anFd=" << fd;
        return ERR_AOT_COMPILER_SIGNATURE_FAILED;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        LOG_SA(ERROR) << "lseek failed for fd=" << fd << " errno=" << errno;
        return ERR_AOT_COMPILER_SIGNATURE_FAILED;
    }
    LOG_SA(INFO) << "calling SignLocalCodeByFd with fd=" << fd;
    Security::CodeSign::ByteBuffer sig;
    if (Security::CodeSign::LocalCodeSignKit::SignLocalCodeByFd(appSignature, fd, sig)
            != CommonErrCode::CS_SUCCESS) {
        LOG_SA(ERROR) << "SignLocalCodeByFd failed";
        return ERR_AOT_COMPILER_SIGNATURE_FAILED;
    }
    LOG_SA(INFO) << "SignLocalCodeByFd success, sigSize=" << sig.GetSize();
    uint8_t *dataPtr = sig.GetBuffer();
    for (uint32_t i = 0; i < sig.GetSize(); ++i) {
        sigData.emplace_back(dataPtr[i]);
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
    stopRequested_.store(true);
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

bool AotCompilerImpl::IsAllowAotCompiler() const
{
    return allowAotCompiler_.load();
}

std::string AotCompilerImpl::GetAiFilePath(const std::string &anFilePath)
{
    static constexpr size_t AN_SUFFIX_LEN = 3;  // strlen(".an")
    if (anFilePath.size() < AN_SUFFIX_LEN) {
        return "";
    }
    return anFilePath.substr(0, anFilePath.size() - AN_SUFFIX_LEN) + ".ai";
}

#ifdef CODE_SIGN_ENABLE
int32_t AotCompilerImpl::PrepareOutputFiles(AotParserType parserType, uid_t bundleUid, gid_t bundleGid,
    int32_t &outAnFd)
{
    if (parserType != AotParserType::STATIC_AOT &&
        parserType != AotParserType::FRAMEWORK_STATIC_AOT &&
        parserType != AotParserType::DYNAMIC_AOT) {
        LOG_SA(INFO) << "PrepareOutputFiles: skip, parserType="
            << static_cast<int32_t>(parserType) << " (UNKNOWN)";
        return ERR_OK;
    }
    std::string fileName = argsHandler_->GetFileName();
    if (parserType == AotParserType::DYNAMIC_AOT && panda::ecmascript::FileExist(fileName.c_str())) {
        LOG_SA(INFO) << "AOT file already exists, skip compilation: " << fileName;
        return ERR_OK_AOT_FILE_EXIST;
    }
    int32_t preCreateRet = PreCreateAotFiles(parserType, bundleUid, bundleGid, outAnFd);
    LOG_SA(INFO) << "PreCreateAotFiles ret=" << preCreateRet << " anFd=" << outAnFd;
    return preCreateRet;
}

int32_t AotCompilerImpl::PreCreateAotFiles(AotParserType parserType, uid_t bundleUid, gid_t bundleGid,
    int32_t &outAnFd)
{
    std::string anFilePath = argsHandler_->GetFileName();
    if (anFilePath.empty()) {
        LOG_SA(WARN) << "PreCreateAotFiles: GetFileName() returned empty, skip pre-create";
        return ERR_OK;
    }

    // Ensure the output directory exists (needed for PersistAnFile later)
    std::string dirPath = anFilePath.substr(0, anFilePath.find_last_of('/'));
    if (!dirPath.empty()) {
        EnsureDirPermission(dirPath);
        if (CreateOutputDirectory(dirPath) != ERR_OK) {
            LOG_SA(ERROR) << "PreCreateAotFiles: CreateOutputDirectory failed: " << dirPath;
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
    }

    // Use memfd_create for .an output (no disk file until PersistAnFile after signing)
    if (CreateAnMemfd(outAnFd) != ERR_OK) {
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    if (parserType == AotParserType::DYNAMIC_AOT) {
        std::string aiFilePath = GetAiFilePath(anFilePath);
        if (CreateAiFile(aiFilePath, bundleUid, bundleGid) != ERR_OK) {
            close(outAnFd);
            outAnFd = -1;
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
    }
    LOG_SA(INFO) << "PreCreateAotFiles: done, memfd anFd=" << outAnFd;
    return ERR_OK;
}

int32_t AotCompilerImpl::CreateOutputDirectory(const std::string &dirPath)
{
    if (!panda::ecmascript::ForceCreateDirectory(dirPath)) {
        LOG_SA(ERROR) << "ForceCreateDirectory failed: " << dirPath;
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    return ERR_OK;
}

int32_t AotCompilerImpl::CreateAnFile(const std::string &anFilePath, int32_t &outFd)
{
    int32_t fd = open(anFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (fd < 0) {
        LOG_SA(ERROR) << "failed to create .an file: " << anFilePath
            << " errno=" << errno << " " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    outFd = fd;
    LOG_SA(INFO) << "created .an file, fd=" << fd << " path=" << anFilePath;
    return ERR_OK;
}

int32_t AotCompilerImpl::CreateAnMemfd(int32_t &outFd)
{
    int fd = memfd_create("an_file", MFD_ALLOW_SEALING);
    if (fd < 0) {
        LOG_SA(ERROR) << "memfd_create failed, errno=" << errno << " " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    outFd = fd;
    LOG_SA(INFO) << "created .an memfd, fd=" << fd;
    return ERR_OK;
}

int32_t AotCompilerImpl::SpliceFdToFd(int srcFd, int dstFd, off_t size)
{
    int pipeFds[2];
    if (pipe(pipeFds) != 0) {
        LOG_SA(ERROR) << "pipe failed, errno=" << errno;
        return ERR_AOT_COMPILER_CALL_FAILED;
    }

    off_t remaining = size;
    while (remaining > 0) {
        ssize_t toPipe = splice(srcFd, nullptr, pipeFds[1], nullptr,
            static_cast<size_t>(remaining), SPLICE_F_MOVE);
        if (toPipe <= 0) {
            LOG_SA(ERROR) << "splice to pipe failed, errno=" << errno;
            close(pipeFds[0]);
            close(pipeFds[1]);
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        ssize_t written = 0;
        while (written < toPipe) {
            ssize_t w = splice(pipeFds[0], nullptr, dstFd, nullptr,
                static_cast<size_t>(toPipe - written), SPLICE_F_MOVE);
            if (w <= 0) {
                LOG_SA(ERROR) << "splice to disk failed, errno=" << errno;
                close(pipeFds[0]);
                close(pipeFds[1]);
                return ERR_AOT_COMPILER_CALL_FAILED;
            }
            written += w;
        }
        remaining -= toPipe;
    }
    close(pipeFds[0]);
    close(pipeFds[1]);
    return ERR_OK;
}

int32_t AotCompilerImpl::PersistAnFile(int32_t memfd, const std::string &anPath)
{
    if (memfd < 0 || anPath.empty()) {
        LOG_SA(ERROR) << "PersistAnFile: invalid args, memfd=" << memfd << " anPath=" << anPath;
        return ERR_AOT_COMPILER_CALL_FAILED;
    }

    off_t memfdSize = lseek(memfd, 0, SEEK_END);
    if (memfdSize < 0) {
        LOG_SA(ERROR) << "lseek memfd failed, errno=" << errno;
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    if (memfdSize == 0) {
        LOG_SA(INFO) << "memfd is empty, skipping persist";
        return ERR_OK;
    }

    int diskFd = open(anPath.c_str(), O_RDWR | O_CREAT | O_TRUNC,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (diskFd < 0) {
        LOG_SA(ERROR) << "failed to create disk file: " << anPath
            << " errno=" << errno << " " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    }

    if (lseek(memfd, 0, SEEK_SET) < 0) {
        LOG_SA(ERROR) << "lseek memfd to 0 failed";
        close(diskFd);
        unlink(anPath.c_str());
        return ERR_AOT_COMPILER_CALL_FAILED;
    }

    int32_t ret = SpliceFdToFd(memfd, diskFd, memfdSize);
    if (ret != ERR_OK) {
        close(diskFd);
        unlink(anPath.c_str());
        return ret;
    }

    if (fsync(diskFd) != 0) {
        LOG_SA(WARN) << "fsync failed, errno=" << errno;
    }
    close(diskFd);
    LOG_SA(INFO) << "PersistAnFile: done, size=" << memfdSize << " path=" << anPath;
    return ERR_OK;
}

int32_t AotCompilerImpl::CreateAiFile(const std::string &aiFilePath, uid_t bundleUid, gid_t bundleGid)
{
    unlink(aiFilePath.c_str());
    int aiFd = open(aiFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (aiFd >= 0) {
        if (fchown(aiFd, bundleUid, bundleGid) != 0) {
            LOG_SA(ERROR) << "fchown .ai to bundle failed: fd=" << aiFd
                << " uid=" << bundleUid << " gid=" << bundleGid << " errno=" << errno;
            close(aiFd);
            return ERR_AOT_COMPILER_CALL_FAILED;
        }
        close(aiFd);
        LOG_SA(INFO) << "created .ai file, path=" << aiFilePath
            << " uid=" << bundleUid << " gid=" << bundleGid;
    } else {
        LOG_SA(ERROR) << "failed to create .ai file: " << aiFilePath
            << " errno=" << errno << " " << strerror(errno);
        return ERR_AOT_COMPILER_CALL_FAILED;
    }
    return ERR_OK;
}

void AotCompilerImpl::EnsureDirPermission(const std::string &dirPath)
{
    if (dirPath.empty()) {
        return;
    }
    struct stat st;
    if (stat(dirPath.c_str(), &st) != 0) {
        return;
    }
    // DYNAMIC_AOT: The output directory (e.g. /data/app/.../aot_compiler/ark_cache/<bundle>/)
    // is created by installd with bundle's uid/gid. compiler_service SA runs under its own
    // uid/gid and cannot write files there. Chown the directory to SA's uid/gid so that
    // subsequent PreCreateAotFiles can create .an/.ai files. After compilation,
    // ChownAotFilesToBundle transfers the output files back to the bundle while keeping
    // the directory owned by SA for future re-compilation.
    // STATIC_AOT / FRAMEWORK_STATIC_AOT: The directory may also be owned by a different
    // uid/gid (created by other system processes). The same chown ensures SA has write access.
    uid_t myUid = getuid();
    gid_t myGid = getgid();
    if (st.st_uid != myUid || st.st_gid != myGid) {
        if (chown(dirPath.c_str(), myUid, myGid) != 0) {
            LOG_SA(ERROR) << "EnsureDirPermission chown failed: " << dirPath
                << " errno=" << errno << " " << strerror(errno);
        } else {
            LOG_SA(INFO) << "EnsureDirPermission chown success: " << dirPath
                << " uid=" << myUid << " gid=" << myGid;
        }
    }
}
#endif

void AotCompilerImpl::PrepareChildProcess(int32_t anFd, int32_t hapFd, const std::vector<int32_t> &hspFds) const
{
    LOG_SA(INFO) << "child process started, pid=" << getpid() << " anFd=" << anFd << " hapFd=" << hapFd;
    if (anFd >= 0) {
        int fdFlags = fcntl(anFd, F_GETFD);
        if (fdFlags >= 0) {
            fcntl(anFd, F_SETFD, fdFlags & ~FD_CLOEXEC);
        }
        argsHandler_->SetChildAnFd(anFd);
        LOG_SA(INFO) << "child set --an-fd=" << anFd;
    }
    if (hapFd >= 0) {
        int fdFlags = fcntl(hapFd, F_GETFD);
        if (fdFlags >= 0) {
            fcntl(hapFd, F_SETFD, fdFlags & ~FD_CLOEXEC);
        }
        argsHandler_->SetChildHapFd(hapFd);
        LOG_SA(INFO) << "child set --hap-fd=" << hapFd;
    }
    for (int32_t fd : hspFds) {
        if (fd >= 0) {
            int fdFlags = fcntl(fd, F_GETFD);
            if (fdFlags >= 0) {
                fcntl(fd, F_SETFD, fdFlags & ~FD_CLOEXEC);
            }
            LOG_SA(INFO) << "cleared CLOEXEC for HSP fd=" << fd;
        }
    }
}

int32_t AotCompilerImpl::HapVerifyInParent(const AotCompilerArgs &args, pid_t childPid)
{
    LOG_SA(INFO) << "parent process, child pid=" << childPid << ", waiting for child";
#if defined(PANDA_TARGET_OHOS)
    if (argsHandler_ && argsHandler_->IsDynamicAOT()) {
        if (!AotArgsVerify::CheckBundleNameAndAppIdentifier(args.bundleName, args.hapPath,
            args.appIdentifier, args.hspModules)) {
            LOG_SA(ERROR) << "CheckBundleNameAndAppIdentifier failed, killing child pid=" << childPid;
            if (kill(childPid, SIGKILL) != 0) {
                LOG_SA(ERROR) << "kill child process failed: pid=" << childPid << " errno=" << errno;
            }
            if (waitpid(childPid, nullptr, 0) == -1) {
                LOG_SA(ERROR) << "waitpid for killed child failed: pid=" << childPid << " errno=" << errno;
            }
            return ERR_AOT_COMPILER_SIGNATURE_FAILED;
        }
        LOG_SA(INFO) << "CheckBundleNameAndAppIdentifier success";
    }
#endif
    return ERR_OK;
}

#ifdef CODE_SIGN_ENABLE
void AotCompilerImpl::SetAotSecurityLabels() const
{
    std::string anFilePath = argsHandler_->GetFileName();
    if (anFilePath.empty()) {
        return;
    }
    panda::ecmascript::SetSecurityLabel(anFilePath);
    std::string aiFilePath = GetAiFilePath(anFilePath);
    panda::ecmascript::SetSecurityLabel(aiFilePath);
}

int32_t AotCompilerImpl::ChownAotFilesToBundle()
{
    std::string anFilePath = argsHandler_->GetFileName();
    if (anFilePath.empty()) {
        return ERR_OK;
    }
    int32_t bundleId = -1;
    int32_t groupId = -1;
    argsHandler_->GetBundleId(bundleId, groupId);
    if (bundleId < 0 || groupId < 0) {
        LOG_SA(ERROR) << "invalid bundle uid/gid, bundleId=" << bundleId << " groupId=" << groupId;
        return ERR_AOT_COMPILER_CHOWN_FAILED;
    }
    uid_t bundleUid = static_cast<uid_t>(bundleId);
    gid_t bundleGid = static_cast<gid_t>(groupId);

    std::string dirPath = anFilePath.substr(0, anFilePath.find_last_of('/'));
    if (!dirPath.empty() &&
        dirPath.compare(0, ArgsIdx::APP_ARK_CACHE_PREFIX.size(),
            ArgsIdx::APP_ARK_CACHE_PREFIX) == 0) {
        LOG_SA(INFO) << "skip chown dir for app ark_cache, keep compiler_service write access: " << dirPath;
    } else if (!dirPath.empty() &&
        dirPath.compare(0, ArgsIdx::FRAMEWORK_ARK_CACHE_PREFIX.size(),
            ArgsIdx::FRAMEWORK_ARK_CACHE_PREFIX) != 0 &&
        dirPath.compare(0, ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX.size(),
            ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX) != 0) {
        if (chown(dirPath.c_str(), bundleUid, bundleGid) != 0) {
            LOG_SA(ERROR) << "chown dir to bundle failed: " << dirPath
                << " uid=" << bundleUid << " gid=" << bundleGid << " errno=" << errno;
            return ERR_AOT_COMPILER_CHOWN_FAILED;
        }
    }
    // chown the on-disk .an file (persisted by PersistAnFile after signing)
    if (chown(anFilePath.c_str(), bundleUid, bundleGid) != 0) {
        LOG_SA(ERROR) << "chown .an to bundle failed: " << anFilePath
            << " uid=" << bundleUid << " gid=" << bundleGid << " errno=" << errno;
        return ERR_AOT_COMPILER_CHOWN_FAILED;
    }
    if (argsHandler_->IsDynamicAOT()) {
        std::string aiFilePath = GetAiFilePath(anFilePath);
        if (chown(aiFilePath.c_str(), bundleUid, bundleGid) != 0) {
            LOG_SA(ERROR) << "chown .ai to bundle failed: " << aiFilePath
                << " uid=" << bundleUid << " gid=" << bundleGid << " errno=" << errno;
            return ERR_AOT_COMPILER_CHOWN_FAILED;
        }
    }
    LOG_SA(INFO) << "ChownAotFilesToBundle done, uid=" << bundleUid << " gid=" << bundleGid;
    return ERR_OK;
}
#endif

int32_t AotCompilerImpl::CheckCompilationResult(const AotCompilerArgs &args, int32_t ret,
    CompilationCleanupGuard &guard)
{
    if (ret == ERR_OK_NO_AOT_FILE) {
        LOG_SA(INFO) << "no aot file generated, cleaning up";
        return ERR_OK;
    }
    if (int32_t sysRet = SendSysEvent(args); sysRet != 0) {
        LOG_SA(WARN) << "SendSysEvent failed: ret=" << sysRet;
    }
    if (guard.GetFd() >= 0) {
        off_t memfdSize = lseek(guard.GetFd(), 0, SEEK_END);
        if (memfdSize == 0) {
            LOG_SA(INFO) << ".an memfd is empty after compilation, cleaning up";
            return ret;
        }
    }
    if (ret != ERR_OK) {
        LOG_SA(INFO) << "compilation failed, cleaning up memfd";
        return ret;
    }
    return ERR_OK;
}

#ifdef CODE_SIGN_ENABLE
int32_t AotCompilerImpl::PersistAndSignAnFile(CompilationCleanupGuard &guard, std::vector<uint8_t> &sigData)
{
    std::string anFilePath = argsHandler_->GetFileName();
    int32_t anFd = guard.GetFd();

    // Sign directly from memfd (code_signature stub accepts non-regular fd).
    int32_t ret = AOTLocalCodeSign(sigData, anFd);
    LOG_SA(INFO) << "AOTLocalCodeSign ret=" << ret << " sigDataSize=" << sigData.size();
    if (ret != ERR_OK) {
        LOG_SA(ERROR) << "AOTLocalCodeSign failed for fd=" << anFd;
        return ret;
    }

    // Persist memfd to disk file after signing succeeds.
    if (anFd >= 0 && !anFilePath.empty()) {
        int32_t persistRet = PersistAnFile(anFd, anFilePath);
        if (persistRet != ERR_OK) {
            LOG_SA(ERROR) << "PersistAnFile failed";
            return persistRet;
        }
    }
    return ERR_OK;
}

int32_t AotCompilerImpl::FinalizeAotOutput(CompilationCleanupGuard &guard, int32_t ret)
{
    if (ret == ERR_OK) {
        SetAotSecurityLabels();
    }
    int32_t chownRet = ChownAotFilesToBundle();
    if (chownRet != ERR_OK) {
        LOG_SA(ERROR) << "ChownAotFilesToBundle failed, cleaning up";
        return chownRet;
    }
    LOG_SA(INFO) << "EcmascriptAotCompiler done, final ret=" << ret;
    guard.Dismiss();
    return ret;
}
#endif

int32_t AotCompilerImpl::HandlePostCompilation(const AotCompilerArgs &args, int32_t ret,
    std::vector<uint8_t> &sigData, CompilationCleanupGuard &guard)
{
    if (ret == ERR_OK_NO_AOT_FILE) {
        LOG_SA(INFO) << "no aot file generated, skip sign/persist";
        return ERR_OK;
    }
    int32_t checkRet = CheckCompilationResult(args, ret, guard);
    if (checkRet != ERR_OK) {
        return checkRet;
    }

#ifdef CODE_SIGN_ENABLE
    ret = PersistAndSignAnFile(guard, sigData);
    return FinalizeAotOutput(guard, ret);
#else
    return ret;
#endif
}
// LCOV_EXCL_STOP
} // namespace OHOS::ArkCompiler
