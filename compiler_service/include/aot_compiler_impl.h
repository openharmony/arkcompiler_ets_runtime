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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "ecmascript/compiler/aot_file/aot_version.h"

namespace OHOS::ArkCompiler {
class AotCompilerImpl {
public:
    static constexpr int32_t AOT_COMPILE_STOP_LEVEL = 2;
    static AotCompilerImpl &GetInstance();
    /**
     * @brief ecmascript aot_compiler
     * @param argsMap input command strings
     * @param sigData  contain signature string data for EnforceCodeSignForFile()
     * @return err code
     */
    int32_t EcmascriptAotCompiler(const std::unordered_map<std::string, std::string> &argsMap,
                                  std::vector<int16_t> &sigData);
    int32_t StopAotCompiler();
    int32_t GetAOTVersion(std::string& sigData);
    int32_t NeedReCompile(const std::string& args, bool& sigData);
    void HandlePowerDisconnected();
    void HandleScreenOn();
    void HandleThermalLevelChanged(const int32_t level);

protected:
    int32_t FindArgsIdxToInteger(const std::unordered_map<std::string, std::string> &argsMap,
                                 const std::string &keyName, int32_t &bundleID);
    int32_t FindArgsIdxToString(const std::unordered_map<std::string, std::string> &argsMap,
                                const std::string &keyName, std::string &bundleArg);
    int32_t PrepareArgs(const std::unordered_map<std::string, std::string> &argsMap);
    void GetBundleId(int32_t &bundleUid, int32_t &bundleGid) const;
    void DropCapabilities() const;
    void GetAotArgsVector(std::vector<const char*> &argv) const;
    void ExecuteInChildProcess() const;
    int32_t PrintAOTCompilerResult(const int compilerStatus) const;
    void ExecuteInParentProcess(pid_t childPid, int32_t &ret);
    void GetCodeSignArgs(std::string &appSignature, std::string &fileName) const;
    int32_t AOTLocalCodeSign(std::vector<int16_t> &sigData) const;
    int32_t RemoveAotFiles() const;
    void InitState(const pid_t childPid);
    void AddExpandArgs(std::vector<std::string> &argVector);
    void ResetState();
    void PauseAotCompiler();
    void AllowAotCompiler();
    bool VerifyCompilerModeAndPkgInfo(const std::unordered_map<std::string, std::string> &argsMap);

    AotCompilerImpl() = default;
    ~AotCompilerImpl() = default;
    AotCompilerImpl(const AotCompilerImpl&) = delete;
    AotCompilerImpl(AotCompilerImpl&&) = delete;
    AotCompilerImpl& operator=(const AotCompilerImpl&) = delete;
    AotCompilerImpl& operator=(AotCompilerImpl&&) = delete;
protected:
    std::atomic<bool> allowAotCompiler_ {true};
    mutable std::mutex hapArgsMutex_;
    mutable std::mutex stateMutex_;
    struct HapArgs {
        std::vector<std::string> argVector;
        std::string fileName;
        std::string signature;
        int32_t bundleUid {0};
        int32_t bundleGid {0};
    } hapArgs_;
    struct AOTState {
        bool running {false};
        pid_t childPid {-1};
    } state_;
    int32_t thermalLevel_ {0};
};
} // namespace OHOS::ArkCompiler
#endif  // OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H