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

#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "aot_compiler_args.h"
#include "aot_compiler_constants.h"

namespace OHOS::ArkCompiler {

class AOTArgsParserBase;

class AOTArgsHandler {
public:
    explicit AOTArgsHandler(const AotCompilerArgs &args);

    AOTArgsHandler() = default;
    ~AOTArgsHandler() = default;

    int32_t Handle(int32_t thermalLevel = 0);

    std::vector<const char*> GetAotArgs() const;

    void GetBundleId(int32_t &bundleUid, int32_t &bundleGid) const;

    std::string GetFileName() const;

    std::string GetCodeSignArgs() const;

    bool IsDynamicAOT() const;

    void SetParser();

    void SetIsEnableStaticCompiler(bool value)
    {
        isEnableStaticCompiler_ = value;
    }

    bool IsEnableStaticCompiler() const
    {
        return isEnableStaticCompiler_;
    }

    void SetChildAnFd(int fd)
    {
        childAnFd_ = fd;
    }

    void SetChildHapFd(int fd)
    {
        childHapFd_ = fd;
    }

    AotParserType GetParserType() const;

    static std::string BuildCompilerPkgInfo(const AotCompilerArgs &args);
    static std::string BuildExternalPkgInfo(const AotCompilerArgs &args);
    static std::string BuildExternalPkgInfo(const AotCompilerArgs &args,
                                            const std::map<std::string, int32_t> &hspFdMap);
    void UpdateExternalPkgInfo(const std::string &extPkgJson);

private:
    std::unique_ptr<AOTArgsParserBase> parser_;
    AotCompilerArgs args_;
    std::vector<std::string> argVector_;
    mutable std::mutex argsMutex_;
    bool isEnableStaticCompiler_ { true };

    int childAnFd_ {-1};
    int childHapFd_ {-1};
    mutable std::string anFdArg_;
    mutable std::string hapFdArg_;
};

class AOTArgsParserBase {
public:
    virtual ~AOTArgsParserBase() = default;

    virtual int32_t Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
                          int32_t thermalLevel) = 0;

    virtual AotParserType GetParserType() const = 0;

    virtual std::string GetFdArgName() const = 0;

    virtual std::string GetHapFdArgName() const = 0;

    bool Check(const AotCompilerArgs &args);

    static bool IsFileExists(const std::string &fileName);
    static bool ParseBlackListJson(nlohmann::json &jsonObject);
    static std::vector<std::string> JoinMethodList(const nlohmann::json &methodLists);
    static std::string BuildRegexPattern(const std::vector<std::string> &blacklistedMethods);
#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    static bool IsEnableStaticCompiler();
#endif
};

class AOTArgsParser final : public AOTArgsParserBase {
public:
    int32_t Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
                  int32_t thermalLevel) override;

    void AddThermalLevelArg(std::vector<std::string> &argVector, int32_t thermalLevel);

    AotParserType GetParserType() const override
    {
        return AotParserType::DYNAMIC_AOT;
    }

    std::string GetFdArgName() const override
    {
        return ArgsIdx::AN_FD;
    }

    std::string GetHapFdArgName() const override
    {
        return ArgsIdx::HAP_FD;
    }

    void BuildDynamicCompilerArgs(const AotCompilerArgs &args, std::vector<std::string> &argVector);

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    void SetAnFileMaxSizeBySysParam(std::vector<std::string> &argVector);
    void SetEnableCodeCommentBySysParam(std::vector<std::string> &argVector);
#endif
};

class StaticAOTArgsParser : public AOTArgsParserBase {
public:
    int32_t Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
                  int32_t thermalLevel) override;

    bool ParseBootPandaFiles(std::string &bootfiles);

    std::string ParseLocation(const std::string &anfilePath);
    std::string ParseOuterHspLocation(const std::string &abcPath);
    std::string ParseModuleName(const std::string &anFilePath);

    int32_t ParseLocationByBundleType(int32_t bundleType, const std::string &abcPath, const std::string &anfilePath,
        std::string &location);

    bool AddProfilePathArg(std::vector<std::string> &argVector, const std::string &pgoDir,
                           const std::string &moduleName);

    AotParserType GetParserType() const override
    {
        return AotParserType::STATIC_AOT;
    }

    std::string GetFdArgName() const override
    {
        return ArgsIdx::PAOC_AN_FD;
    }

    std::string GetHapFdArgName() const override
    {
        return ArgsIdx::PAOC_HAP_FD;
    }

    int32_t BuildStaticCompilerArgs(AotCompilerArgs &args, std::vector<std::string> &argVector);

    std::string ParseBlackListMethods(const std::string &bundleName, const std::string &moduleName);
    std::string ProcessBlackListForBundleAndModule(const nlohmann::json &jsonObject,
                                                   const std::string &bundleName,
                                                   const std::string &moduleName);
    std::vector<std::string> ProcessMatchingModules(const nlohmann::json &item,
                                       const std::string &moduleName);
    void ProcessArgs(const AotCompilerArgs &args, std::string &anfilePath, bool &partialMode,
                     std::vector<std::string> &argVector);
    void ProcessBlackListMethods(const std::string &bundleName, const std::string &anfilePath,
                                 std::vector<std::string> &argVector);
    bool CheckBundleNameAndModuleList(const nlohmann::json &item, const std::string &bundleName);
    bool CheckModuleNameAndMethodList(const nlohmann::json &moduleItem, const std::string &moduleName);
};

class StaticFrameworkAOTArgsParser final : public StaticAOTArgsParser {
public:
    int32_t Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
                    int32_t thermalLevel) override;

    std::string ParseFrameworkBootPandaFiles(const std::string &bootfiles, const std::string &paocPandaFiles);

    std::string ParseBlackListMethods(const std::string &bundleName);
    bool CheckBundleNameAndMethodList(const nlohmann::json &item, const std::string &bundleName);

    AotParserType GetParserType() const override
    {
        return AotParserType::FRAMEWORK_STATIC_AOT;
    }
};

class AOTArgsParserFactory {
public:
    static std::optional<std::unique_ptr<AOTArgsParserBase>> GetParser(
        const AotCompilerArgs &args, bool isEnableStaticCompiler = true);
};

} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOT_ARGS_HANDLER_H
