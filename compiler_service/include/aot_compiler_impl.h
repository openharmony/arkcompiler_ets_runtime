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

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "ecmascript/compiler/aot_file/aot_version.h"

namespace OHOS::ArkCompiler {
class AotCompilerImpl {
public:
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

private:
    inline int32_t FindArgsIdxToInteger(const std::unordered_map<std::string, std::string> &argsMap,
                                        const std::string &keyName, int32_t &bundleID);
    inline int32_t FindArgsIdxToString(const std::unordered_map<std::string, std::string> &argsMap,
                                       const std::string &keyName, std::string &bundleArg);
    int32_t PrepareArgs(const std::unordered_map<std::string, std::string> &argsMap);
    void DropCapabilities(const int32_t &bundleUid, const int32_t &bundleGid) const;
    void ExecuteInChildProcess(const std::vector<std::string> &aotVector) const;
    int32_t PrintAOTCompilerResult(const int compilerStatus);
    void ExecuteInParentProcess(pid_t childPid, int32_t &ret);
    int32_t AOTLocalCodeSign(const std::string &fileName, const std::string &appSignature,
                             std::vector<int16_t> &sigData);
    void InitState(const pid_t childPid);
    void ResetState();
    void PauseAotCompiler();
    void AllowAotCompiler();

    AotCompilerImpl() = default;
    ~AotCompilerImpl() = default;

    AotCompilerImpl(const AotCompilerImpl&) = delete;
    AotCompilerImpl(AotCompilerImpl&&) = delete;
    AotCompilerImpl& operator=(const AotCompilerImpl&) = delete;
    AotCompilerImpl& operator=(AotCompilerImpl&&) = delete;
private:
    mutable std::mutex mutex_;
    mutable std::mutex stateMutex_;
    struct HapArgs {
        std::vector<std::string> argVector;
        std::string fileName;
        std::string signature;
        int32_t bundleUid;
        int32_t bundleGid;
    } hapArgs;
    struct AOTState {
        bool running = false;
        pid_t childPid = -1;
    } state_;
    bool allowAotCompiler_ {true};
};
} // namespace OHOS::ArkCompiler
#endif  // OHOS_ARKCOMPILER_AOTCOMPILER_IMPL_H