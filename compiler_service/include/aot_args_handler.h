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
#include <nlohmann/json.hpp>
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

    void SetParser(const std::unordered_map<std::string, std::string> &argsMap);

    void SetIsEnableStaticCompiler(bool value)
    {
        isEnableStaticCompiler_ = value;
    }

    bool IsEnableStaticCompiler() const
    {
        return isEnableStaticCompiler_;
    }

private:
    std::unique_ptr<AOTArgsParserBase> parser_;
    const std::unordered_map<std::string, std::string> argsMap_;
    HapArgs hapArgs_;
    mutable std::mutex hapArgsMutex_;
    bool isEnableStaticCompiler_ { true };
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

    static bool IsFileExists(const std::string &fileName);
    static bool ParseBundleName(const std::string &pkgInfo, std::string &bundleName);
    static bool ParseBlackListJson(nlohmann::json &jsonObject);
    static std::vector<std::string> JoinMethodList(const nlohmann::json &methodLists);
    static std::string BuildRegexPattern(const std::vector<std::string> &blacklistedMethods);
#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    static bool IsEnableStaticCompiler();
#endif
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

class StaticAOTArgsParser : public AOTArgsParserBase {
public:
    int32_t Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                  int32_t thermalLevel) override;

    bool ParseBootPandaFiles(std::string &bootfiles);

    std::string ParseLocation(std::string &anfilePath);
    std::string ParseModuleName(const std::string &anFilePath);

    bool ParseProfilePath(std::string &pkgInfo, std::string &profilePath);

    bool ParseProfileUse(HapArgs &hapArgs, std::string &pkgInfo);

    std::string ParseBlackListMethods(const std::string &pkgInfo, const std::string &moduleName);
    std::string ProcessBlackListForBundleAndModule(const nlohmann::json &jsonObject,
                                                   const std::string &bundleName,
                                                   const std::string &moduleName);
    std::vector<std::string> ProcessMatchingModules(const nlohmann::json &item,
                                       const std::string &moduleName);
    void ProcessArgsMap(const std::unordered_map<std::string, std::string> &argsMap,
                       std::string &anfilePath, std::string &pkgInfo, bool &partialMode,
                       HapArgs &hapArgs);
    void ProcessBlackListMethods(const std::string &pkgInfo, const std::string &anfilePath,
                                 HapArgs &hapArgs);
    bool CheckBundleNameAndModuleList(const nlohmann::json &item, const std::string &bundleName);
    bool CheckModuleNameAndMethodList(const nlohmann::json &moduleItem, const std::string &moduleName);
};

class StaticFrameworkAOTArgsParser final : public StaticAOTArgsParser {
public:
    int32_t Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                    int32_t thermalLevel) override;

    std::string ParseFrameworkBootPandaFiles(const std::string &bootfiles, const std::string &paocPandaFiles);

    std::string ParseBlackListMethods(const std::string &bundleName);
    bool CheckBundleNameAndMethodList(const nlohmann::json &item, const std::string &bundleName);
};

class AOTArgsParserFactory {
public:
    static std::optional<std::unique_ptr<AOTArgsParserBase>> GetParser(
        const std::unordered_map<std::string, std::string> &argsMap, bool IsEnableStaticCompiler = true);
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOT_ARGS_HANDLER_H
 