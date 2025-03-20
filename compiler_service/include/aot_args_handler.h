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

#ifndef OHOS_ARKCOMPILER_AOT_ARGS_HANDLER_H
#define OHOS_ARKCOMPILER_AOT_ARGS_HANDLER_H

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace OHOS::ArkCompiler {
struct HapArgs {
    std::vector<std::string> argVector;
    std::string fileName;
    std::string signature;
    int32_t bundleUid {0};
    int32_t bundleGid {0};
};

class AOTArgsParserBase;

class AOTArgsHandler {
public:
    AOTArgsHandler(const std::unordered_map<std::string, std::string> &argsMap);

    AOTArgsHandler() = default;
    ~AOTArgsHandler() = default;

    int32_t Handle(int32_t thermalLevel = 0);

    std::vector<const char*> GetAotArgs() const;

    void GetBundleId(int32_t &bundleUid, int32_t &bundleGid) const;

    std::string GetFileName() const;

    std::string GetCodeSignArgs() const;

private:
    std::unique_ptr<AOTArgsParserBase> parser_;
    const std::unordered_map<std::string, std::string> argsMap_;
    HapArgs hapArgs_;
    mutable std::mutex hapArgsMutex_;
};

class AOTArgsParserBase {
public:
    virtual ~AOTArgsParserBase() = default;

    virtual int32_t Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                          int32_t thermalLevel) = 0;

    static int32_t FindArgsIdxToInteger(const std::unordered_map<std::string, std::string> &argsMap,
                                        const std::string &keyName, int32_t &bundleID);

    static int32_t FindArgsIdxToString(const std::unordered_map<std::string, std::string> &argsMap,
                                       const std::string &keyName, std::string &bundleArg);
};

class AOTArgsParser final : public AOTArgsParserBase {
public:
    int32_t Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                  int32_t thermalLevel) override;

    void AddExpandArgs(std::vector<std::string> &argVector, int32_t thermalLevel);

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    void SetAnFileMaxSizeBySysParam(HapArgs &hapArgs);
    void SetEnableCodeCommentBySysParam(HapArgs &hapArgs);
#endif
};

class StaticAOTArgsParser final : public AOTArgsParserBase {
public:
    int32_t Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                  int32_t thermalLevel) override;

    bool ParseBootPandaFiles(std::string &bootfiles);

    std::string ParseLocation(std::string &anfilePath);
};

class AOTArgsParserFactory {
public:
    static std::optional<std::unique_ptr<AOTArgsParserBase>> GetParser(const std::unordered_map<std::string,
                                                                       std::string> &argsMap);
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOT_ARGS_HANDLER_H
