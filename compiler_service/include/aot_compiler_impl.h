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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H

#include <atomic>
#include <mutex>
#include <string>
#include <unistd.h>
#include <vector>
#include "aot_args_handler.h"
#include "aot_compiler_args.h"
#include "ecmascript/compiler/aot_file/aot_version.h"

namespace OHOS::ArkCompiler {

// RAII holder for file descriptor, thread-safe via atomic exchange
class FdHolder {
public:
    FdHolder() = default;
    ~FdHolder() { Reset(); }

    FdHolder(const FdHolder&) = delete;
    FdHolder& operator=(const FdHolder&) = delete;

    void Acquire(int32_t fd)
    {
        Reset();
        fd_.store(fd);
    }

    void Reset()
    {
        int32_t fd = fd_.exchange(-1);
        if (fd >= 0) {
            close(fd);
        }
    }

    int32_t Get() const
    {
        return fd_.load();
    }

private:
    std::atomic<int32_t> fd_ {-1};
};

// RAII guard that cleans up fd and aot files on scope exit unless Dismiss()'d.
// Used in EcmascriptAotCompiler to avoid scattered cleanup calls on error paths.
// When using memfd for .an output, the guard only closes the memfd on failure
// (no disk file to clean up). The HAP fd is also managed here.
class CompilationCleanupGuard {
public:
    explicit CompilationCleanupGuard(std::string anFilePath)
        : anFilePath_(std::move(anFilePath)), active_(true) {}

    ~CompilationCleanupGuard()
    {
        if (active_) {
            fdHolder_.Reset();
            hapFdHolder_.Reset();
            CloseHspFds();
            RemoveAotFiles();
        }
    }

    void AcquireFd(int32_t fd) { fdHolder_.Acquire(fd); }
    int32_t GetFd() const { return fdHolder_.Get(); }

    void AcquireHapFd(int32_t fd) { hapFdHolder_.Acquire(fd); }
    int32_t GetHapFd() const { return hapFdHolder_.Get(); }

    void AcquireHspFd(int32_t fd)
    {
        hspFds_.push_back(fd);
    }
    const std::vector<int32_t> &GetHspFds() const
    {
        return hspFds_;
    }

    // Compilation succeeds: close fd but keep .an/.ai files
    void Dismiss()
    {
        active_ = false;
        fdHolder_.Reset();
        hapFdHolder_.Reset();
        CloseHspFds();
    }

    CompilationCleanupGuard(const CompilationCleanupGuard&) = delete;
    CompilationCleanupGuard& operator=(const CompilationCleanupGuard&) = delete;

private:
    void RemoveAotFiles()
    {
        if (!anFilePath_.empty() && access(anFilePath_.c_str(), F_OK) == 0) {
            unlink(anFilePath_.c_str());
            LOG_SA(INFO) << "removed .an file: " << anFilePath_;
        }
        std::string aiFilePath = DeriveAiFilePath(anFilePath_);
        if (!aiFilePath.empty() && access(aiFilePath.c_str(), F_OK) == 0) {
            unlink(aiFilePath.c_str());
            LOG_SA(INFO) << "removed .ai file: " << aiFilePath;
        }
    }

    static std::string DeriveAiFilePath(const std::string &anPath)
    {
        static constexpr size_t AN_SUFFIX_LEN = 3;  // strlen(".an")
        if (anPath.size() < AN_SUFFIX_LEN) {
            return "";
        }
        return anPath.substr(0, anPath.size() - AN_SUFFIX_LEN) + ".ai";
    }

    FdHolder fdHolder_;
    FdHolder hapFdHolder_;
    std::vector<int32_t> hspFds_;
    std::string anFilePath_;
    bool active_;

    void CloseHspFds()
    {
        for (int32_t fd : hspFds_) {
            if (fd >= 0) {
                close(fd);
            }
        }
        hspFds_.clear();
    }
};

class AotCompilerImpl {
public:
    static constexpr int32_t AOT_COMPILE_STOP_LEVEL = 2;
    static AotCompilerImpl &GetInstance();
    int32_t EcmascriptAotCompiler(const AotCompilerArgs &args,
                                  std::vector<uint8_t> &sigData);
    int32_t StopAotCompiler();
    int32_t GetAOTVersion(std::string& sigData);
    int32_t NeedReCompile(const std::string& args, bool& sigData);
    void HandlePowerDisconnected();
    void HandleScreenOn();
    void HandleThermalLevelChanged(const int32_t level);
    int32_t SendSysEvent(const AotCompilerArgs &args) const;
    std::string ParseArkCacheFromArgs(const AotCompilerArgs &args) const;
    bool IsAllowAotCompiler() const;

protected:
    void DropCapabilities() const;
    void ExecuteInChildProcess() const;
    int32_t PrintAOTCompilerResult(const int compilerStatus) const;
    void ExecuteInParentProcess(pid_t childPid, int32_t &ret);
    int32_t AOTLocalCodeSign(std::vector<uint8_t> &sigData, int32_t anFd) const;
    int32_t RemoveAotFiles() const;
    void InitState(const pid_t childPid);
    void ResetState();
    void PauseAotCompiler();
    void AllowAotCompiler();
    bool CheckAotWhitelist(const AotCompilerArgs &args) const;
    static std::string GetAiFilePath(const std::string &anFilePath);

    void PrepareChildProcess(int32_t anFd, int32_t hapFd, const std::vector<int32_t> &hspFds) const;
    int32_t OpenHapFile(const AotCompilerArgs &args, CompilationCleanupGuard &guard);
    int32_t OpenHspFiles(const AotCompilerArgs &args, CompilationCleanupGuard &guard);
    void UpdateExternalPkgFdInfo(const AotCompilerArgs &args, const CompilationCleanupGuard &guard);
    int32_t PrepareCompilationInputs(const AotCompilerArgs &args, AotParserType parserType,
                                     CompilationCleanupGuard &guard);
    int32_t HapVerifyInParent(const AotCompilerArgs &args, pid_t childPid);
    int32_t HandlePostCompilation(const AotCompilerArgs &args, int32_t ret,
                                   std::vector<uint8_t> &sigData,
                                   CompilationCleanupGuard &guard);
    int32_t CheckCompilationResult(const AotCompilerArgs &args, int32_t ret,
                                    CompilationCleanupGuard &guard);
#ifdef CODE_SIGN_ENABLE
    int32_t PersistAndSignAnFile(CompilationCleanupGuard &guard, std::vector<uint8_t> &sigData);
    int32_t FinalizeAotOutput(CompilationCleanupGuard &guard, int32_t ret);
#endif
    int32_t PrepareArgsHandler(const AotCompilerArgs &args);
    int32_t ForkAndExecute(AotParserType parserType, const AotCompilerArgs &args, int32_t &ret, int32_t anFd,
        int32_t hapFd, const std::vector<int32_t> &hspFds);
#ifdef CODE_SIGN_ENABLE
    int32_t PreCreateAotFiles(AotParserType parserType, uid_t bundleUid, gid_t bundleGid, int32_t &outAnFd);
    int32_t CreateOutputDirectory(const std::string &dirPath);
    int32_t CreateAnMemfd(int32_t &outFd);
    int32_t CreateAnFile(const std::string &anFilePath, int32_t &outFd);
    static int32_t SpliceFdToFd(int srcFd, int dstFd, off_t size);
    int32_t PersistAnFile(int32_t memfd, const std::string &anPath);
    int32_t CreateAiFile(const std::string &aiFilePath, uid_t bundleUid, gid_t bundleGid);
    void SetAotSecurityLabels() const;
    int32_t ChownAotFilesToBundle();
    int32_t PrepareOutputFiles(AotParserType parserType, uid_t bundleUid, gid_t bundleGid, int32_t &outAnFd);
    void EnsureDirPermission(const std::string &dirPath);
#endif
    AotCompilerImpl() = default;
    ~AotCompilerImpl() = default;
    AotCompilerImpl(const AotCompilerImpl&) = delete;
    AotCompilerImpl(AotCompilerImpl&&) = delete;
    AotCompilerImpl& operator=(const AotCompilerImpl&) = delete;
    AotCompilerImpl& operator=(AotCompilerImpl&&) = delete;
protected:
    std::atomic<bool> allowAotCompiler_ {true};
    std::atomic<bool> stopRequested_ {false};
    mutable std::mutex stateMutex_;
    std::unique_ptr<AOTArgsHandler> argsHandler_;
    struct AOTState {
        bool running {false};
        pid_t childPid {-1};
    } state_;
    int32_t thermalLevel_ {0};
};
} // namespace OHOS::ArkCompiler
#endif  // OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H
