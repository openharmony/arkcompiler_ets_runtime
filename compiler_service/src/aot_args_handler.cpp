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

#include "aot_args_handler.h"

#include <charconv>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>

#include "aot_args_list.h"
#include "aot_args_verify.h"
#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
#include "parameters.h"
#endif

namespace OHOS::ArkCompiler {

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
const bool ARK_AOT_ENABLE_STATIC_COMPILER_DEFAULT_VALUE = true;
#endif

AOTArgsHandler::AOTArgsHandler(const AotCompilerArgs &args)
    : args_(args)
{
#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    SetIsEnableStaticCompiler(AOTArgsParserBase::IsEnableStaticCompiler());
#endif
    SetParser();
}

void AOTArgsHandler::SetParser()
{
    auto parserOpt = AOTArgsParserFactory::GetParser(args_, IsEnableStaticCompiler());
    if (parserOpt.has_value()) {
        parser_ = std::move(parserOpt.value());
    } else {
        parser_ = nullptr;
    }
}

int32_t AOTArgsHandler::Handle(int32_t thermalLevel)
{
    if (!parser_) {
        LOG_SA(ERROR) << "AOTArgsParser is null, invalid parameters";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    // Perform validation before parsing
    if (!parser_->Check(args_)) {
        LOG_SA(ERROR) << "Parser check validation failed";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    std::lock_guard<std::mutex> lock(argsMutex_);
    int32_t ret = parser_->Parse(args_, argVector_, thermalLevel);
    return ret;
}

std::vector<const char*> AOTArgsHandler::GetAotArgs() const
{
    std::lock_guard<std::mutex> lock(argsMutex_);
    std::vector<const char*> argv;
    constexpr size_t extraArgsCapacity = 4; // --an-fd + --hap-fd + nullptr terminator + margin
    argv.reserve(argVector_.size() + extraArgsCapacity);
    for (auto &arg : argVector_) {
        argv.emplace_back(arg.c_str());
    }

    // Append FD argument (child process inherits the .an FD via fork)
    if (childAnFd_ >= 0 && parser_) {
        anFdArg_ = Symbols::PREFIX + parser_->GetFdArgName() + Symbols::EQ
            + std::to_string(childAnFd_);
        argv.emplace_back(anFdArg_.c_str());
        LOG_SA(INFO) << "GetAotArgs appended " << anFdArg_;
    }

    // Append HAP FD argument (child process inherits the HAP FD via fork)
    if (childHapFd_ >= 0 && parser_) {
        hapFdArg_ = Symbols::PREFIX + parser_->GetHapFdArgName() + Symbols::EQ
            + std::to_string(childHapFd_);
        argv.emplace_back(hapFdArg_.c_str());
        LOG_SA(INFO) << "GetAotArgs appended " << hapFdArg_;
    }

    return argv;
}

void AOTArgsHandler::GetBundleId(int32_t &bundleUid, int32_t &bundleGid) const
{
    std::lock_guard<std::mutex> lock(argsMutex_);
    bundleUid = args_.bundleUid;
    bundleGid = args_.bundleGid;
}

std::string AOTArgsHandler::GetFileName() const
{
    std::lock_guard<std::mutex> lock(argsMutex_);
    return args_.anFileName;
}

std::string AOTArgsHandler::GetCodeSignArgs() const
{
    std::lock_guard<std::mutex> lock(argsMutex_);
    return args_.appIdentifier;
}

AotParserType AOTArgsHandler::GetParserType() const
{
    if (parser_) {
        return parser_->GetParserType();
    }
    return AotParserType::UNKNOWN;
}

bool AOTArgsHandler::IsDynamicAOT() const
{
    if (!parser_) {
        return false;
    }
    return parser_->GetParserType() == AotParserType::DYNAMIC_AOT;
}

std::string AOTArgsHandler::BuildCompilerPkgInfo(const AotCompilerArgs &args)
{
    nlohmann::json pkgInfo;
    pkgInfo["bundleName"] = args.bundleName;
    pkgInfo["moduleName"] = args.moduleName;
    pkgInfo["pkgPath"] = args.hapPath;
    pkgInfo["pgoDir"] = args.pgoDir;
    pkgInfo["bundleUid"] = std::to_string(args.bundleUid);
    pkgInfo["processUid"] = std::to_string(args.processUid);
    pkgInfo["appIdentifier"] = args.appIdentifier;
    pkgInfo["isEncryptedBundle"] = std::to_string(args.isEncryptedBundle);
    // NOTE: isScreenOff is NOT included here - the engine's UpdateProperty()
    // in ohos_pkg_args.h does not recognize this key. It is only used at the
    // SA level via the separate "compiler-device-state" argument.
    std::string abcName;
    if (args.moduleArkTSMode == ArgsIdx::ARKTS_DYNAMIC) {
        abcName = ArgsIdx::ABC_MODULES_PATH;
    } else {
        abcName = ArgsIdx::ABC_MODULES_STATIC_PATH;
    }
    pkgInfo["abcName"] = abcName;
    return pkgInfo.dump();
}

std::string AOTArgsHandler::BuildExternalPkgInfo(const AotCompilerArgs &args)
{
    nlohmann::json extPkgArray = nlohmann::json::array();
    for (const auto &hsp : args.hspModules) {
        nlohmann::json hspObj;
        hspObj["bundleName"] = hsp.bundleName;
        hspObj["moduleName"] = hsp.moduleName;
        hspObj["pkgPath"] = hsp.hapPath;
        hspObj["moduleArkTSMode"] = hsp.moduleArkTSMode;
        // abcName is required by engine's ParseFromJsObject -> Valid() check
        std::string hspAbcName = (hsp.moduleArkTSMode == ArgsIdx::ARKTS_DYNAMIC)
            ? ArgsIdx::ABC_MODULES_PATH : ArgsIdx::ABC_MODULES_STATIC_PATH;
        hspObj["abcName"] = hspAbcName;
        extPkgArray.push_back(hspObj);
    }
    return extPkgArray.dump();
}

std::string AOTArgsHandler::BuildExternalPkgInfo(const AotCompilerArgs &args,
    const std::map<std::string, int32_t> &hspFdMap)
{
    nlohmann::json extPkgArray = nlohmann::json::array();
    for (const auto &hsp : args.hspModules) {
        nlohmann::json hspObj;
        hspObj["bundleName"] = hsp.bundleName;
        hspObj["moduleName"] = hsp.moduleName;
        hspObj["pkgPath"] = hsp.hapPath;
        hspObj["moduleArkTSMode"] = hsp.moduleArkTSMode;
        std::string hspAbcName = (hsp.moduleArkTSMode == ArgsIdx::ARKTS_DYNAMIC)
            ? ArgsIdx::ABC_MODULES_PATH : ArgsIdx::ABC_MODULES_STATIC_PATH;
        hspObj["abcName"] = hspAbcName;
        auto it = hspFdMap.find(hsp.hapPath);
        if (it != hspFdMap.end() && it->second >= 0) {
            hspObj["hapFd"] = std::to_string(it->second);
        }
        extPkgArray.push_back(hspObj);
    }
    return extPkgArray.dump();
}

void AOTArgsHandler::UpdateExternalPkgInfo(const std::string &extPkgJson)
{
    std::lock_guard<std::mutex> lock(argsMutex_);
    std::string prefix = Symbols::PREFIX + ArgsIdx::COMPILER_EXTERNAL_PKG_INFO + Symbols::EQ;
    for (auto &arg : argVector_) {
        if (arg.compare(0, prefix.size(), prefix) == 0) {
            arg = prefix + extPkgJson;
            LOG_SA(INFO) << "Updated external pkg info with HSP fd data";
            return;
        }
    }
    if (!extPkgJson.empty() && extPkgJson != ArgsIdx::EMPTY_JSON_ARRAY) {
        argVector_.emplace_back(prefix + extPkgJson);
    }
}

int32_t AOTArgsParser::Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
                             int32_t thermalLevel)
{
    std::string abcPath = args.sysCompPath.empty() ? args.abcPath : args.sysCompPath;
    if (abcPath.empty()) {
        LOG_SA(ERROR) << "aot compiler args parsing error: abcPath is empty";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    argVector.clear();
    argVector.emplace_back(AOT_EXE);
    AddThermalLevelArg(argVector, thermalLevel);

    BuildDynamicCompilerArgs(args, argVector);

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    SetEnableCodeCommentBySysParam(argVector);
    SetAnFileMaxSizeBySysParam(argVector);
#endif

    argVector.emplace_back(abcPath);
    return ERR_OK;
}

void AOTArgsParser::BuildDynamicCompilerArgs(const AotCompilerArgs &args, std::vector<std::string> &argVector)
{
    std::string aotFile = args.outputPath.empty() ? "" : args.outputPath + "/" + args.moduleName;
    if (!aotFile.empty()) {
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::AOT_FILE + Symbols::EQ + aotFile);
    }
    if (!args.compileMode.empty()) {
        argVector.emplace_back(
            Symbols::PREFIX + ArgsIdx::TARGET_COMPILER_MODE + Symbols::EQ + args.compileMode);
    }
    if (!args.optBCRangeList.empty()) {
        argVector.emplace_back(
            Symbols::PREFIX + ArgsIdx::COMPILER_OPT_BC_RANGE + Symbols::EQ + args.optBCRangeList);
    }
    argVector.emplace_back(
        Symbols::PREFIX + ArgsIdx::COMPILER_DEVICE_STATE + Symbols::EQ + std::to_string(args.isScreenOff));
    argVector.emplace_back(
        Symbols::PREFIX + ArgsIdx::COMPILER_BASELINE_PGO + Symbols::EQ +
        std::to_string(args.isEnableBaselinePgo));
    argVector.emplace_back(
        Symbols::PREFIX + ArgsIdx::COMPILER_PKG_INFO + Symbols::EQ + AOTArgsHandler::BuildCompilerPkgInfo(args));
    std::string extPkgInfo = AOTArgsHandler::BuildExternalPkgInfo(args);
    if (!extPkgInfo.empty() && extPkgInfo != ArgsIdx::EMPTY_JSON_ARRAY) {
        argVector.emplace_back(
            Symbols::PREFIX + ArgsIdx::COMPILER_EXTERNAL_PKG_INFO + Symbols::EQ + extPkgInfo);
    }
}

bool AOTArgsParserBase::Check(const AotCompilerArgs &args)
{
    switch (GetParserType()) {
        case AotParserType::DYNAMIC_AOT:
            return AotArgsVerify::CheckAOTArgs(args);
        case AotParserType::STATIC_AOT:
            return AotArgsVerify::CheckStaticAotArgs(args);
        case AotParserType::FRAMEWORK_STATIC_AOT:
            return AotArgsVerify::CheckFrameworkStaticAotArgs(args);
        default:
            LOG_SA(ERROR) << "Unknown parser type in Check";
            return false;
    }
}

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
void AOTArgsParser::SetAnFileMaxSizeBySysParam(std::vector<std::string> &argVector)
{
    int anFileMaxSize = OHOS::system::GetIntParameter<int>("ark.aot.compiler_an_file_max_size", -1);
    if (anFileMaxSize >= 0) {
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_AN_FILE_MAX_SIZE + Symbols::EQ +
                                       std::to_string(anFileMaxSize));
    }
}

void AOTArgsParser::SetEnableCodeCommentBySysParam(std::vector<std::string> &argVector)
{
    bool enableAotCodeComment = OHOS::system::GetBoolParameter("ark.aot.code_comment.enable", false);
    if (enableAotCodeComment) {
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_ENABLE_AOT_CODE_COMMENT + Symbols::EQ +
                                       "true");
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_LOG_OPT + Symbols::EQ + ArgsIdx::LOG_OPT_ALLASM);
    }
}

bool AOTArgsParserBase::IsEnableStaticCompiler()
{
    bool enable = OHOS::system::GetBoolParameter("ark.aot.enable_static_compiler",
        ARK_AOT_ENABLE_STATIC_COMPILER_DEFAULT_VALUE);
    LOG_SA(INFO) << "enable static compiler switch is : " << enable;
    return enable;
}
#endif

void AOTArgsParser::AddThermalLevelArg(std::vector<std::string> &argVector, int32_t thermalLevel)
{
    std::string thermalLevelArg = Symbols::PREFIX + std::string("compiler-thermal-level") + Symbols::EQ
        + std::to_string(thermalLevel);
    argVector.emplace_back(thermalLevelArg);
}

void StaticAOTArgsParser::ProcessArgs(const AotCompilerArgs &args, std::string &anfilePath, bool &partialMode,
    std::vector<std::string> &argVector)
{
    // aot-file → paoc-output
    std::string aotFile = args.outputPath.empty() ? "" : args.outputPath + "/" + args.moduleName;
    anfilePath = aotFile;
    if (!anfilePath.empty()) {
        std::string anFileName = anfilePath + ArgsIdx::AN_SUFFIX;
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::PAOC_OUTPUT + Symbols::EQ + anFileName);
    }

    if (args.compileMode == ArgsIdx::PARTIAL) {
        partialMode = true;
    }

    // Emit staticAOTArgsList-matching args
    if (!args.optBCRangeList.empty()) {
        argVector.emplace_back(
            Symbols::PREFIX + ArgsIdx::COMPILER_OPT_BC_RANGE + Symbols::EQ + args.optBCRangeList);
    }
}

void StaticAOTArgsParser::ProcessBlackListMethods(const std::string &bundleName, const std::string &anfilePath,
                                                  std::vector<std::string> &argVector)
{
    std::string moduleName = ParseModuleName(anfilePath);
    std::string blackListMethods = ParseBlackListMethods(bundleName, moduleName);
    if (!blackListMethods.empty()) {
        // Use the generated regex pattern instead of comma-separated list
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_REGEX + Symbols::EQ + blackListMethods);
    }
}

int32_t StaticAOTArgsParser::Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
                                   [[maybe_unused]] int32_t thermalLevel)
{
    std::string abcPath = args.sysCompPath.empty() ? args.abcPath : args.sysCompPath;
    if (abcPath.empty()) {
        LOG_SA(ERROR) << "aot compiler args parsing error: abcPath is empty";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    argVector.clear();
    argVector.emplace_back(STATIC_AOT_EXE);

    for (auto &defaultArg : staticAOTDefaultArgs) {
        argVector.emplace_back(defaultArg);
    }

    return BuildStaticCompilerArgs(args, argVector);
}

int32_t StaticAOTArgsParser::BuildStaticCompilerArgs(AotCompilerArgs &args, std::vector<std::string> &argVector)
{
    std::string abcPath = args.sysCompPath.empty() ? args.abcPath : args.sysCompPath;

    std::string bootfiles;
    if (!ParseBootPandaFiles(bootfiles)) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    argVector.emplace_back(Symbols::PREFIX + ArgsIdx::BOOT_PANDA_FILES + Symbols::EQ + bootfiles);

    std::string anfilePath;
    bool partialMode = false;

    ProcessArgs(args, anfilePath, partialMode, argVector);

    ProcessBlackListMethods(args.bundleName, anfilePath, argVector);

    std::string location;
    if (ParseLocationByBundleType(args.bundleType, abcPath, anfilePath, location) != ERR_OK) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    argVector.emplace_back(Symbols::PREFIX + ArgsIdx::PAOC_LOCATION + Symbols::EQ + location);
    argVector.emplace_back(Symbols::PREFIX + ArgsIdx::PAOC_PANDA_FILES + Symbols::EQ + abcPath);

    std::string moduleName = ParseModuleName(anfilePath);
    if (partialMode && !AddProfilePathArg(argVector, args.pgoDir, moduleName)) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    return ERR_OK;
}

bool StaticAOTArgsParser::ParseBootPandaFiles(std::string &bootfiles)
{
    std::ifstream inFile;
    inFile.open(ArgsIdx::STATIC_BOOT_PATH, std::ios::in);
    if (!inFile.is_open()) {
        LOG_SA(ERROR) << "read json error";
        return false;
    }
    nlohmann::json jsonObject = nlohmann::json::parse(inFile);
    if (jsonObject.is_discarded()) {
        LOG_SA(ERROR) << "json discarded error";
        inFile.close();
        return false;
    }

    if (jsonObject.is_null() || jsonObject.empty()) {
        LOG_SA(ERROR) << "invalid json";
        inFile.close();
        return false;
    }

    for (const auto &[key, value] : jsonObject.items()) {
        if (!value.is_null() && value.is_string()) {
            std::string jsonValue = value.get<std::string>();
            if (jsonValue.empty()) {
                LOG_SA(ERROR) << "json value of " << key << " is empty";
                continue;
            }
            if (!bootfiles.empty()) {
                bootfiles += ":";
            }
            bootfiles += jsonValue.c_str();
        }
    }
    inFile.close();
    return true;
}

// Parse location for Hap & Inner Hsp
std::string StaticAOTArgsParser::ParseLocation(const std::string &anFilePath)
{
    size_t pos = anFilePath.find_last_of("/");
    if (pos == std::string::npos) {
        LOG_SA(FATAL) << "aot sa parse invalid location";
    }
    std::string moduleName = anFilePath.substr(pos + 1);
    std::string location = ArgsIdx::APP_SANDBOX_PATH_PREFIX + moduleName + ArgsIdx::ETS_PATH;
    return location;
}

/*
 * Parse location for Outer Hsp
 *
 * Case1: hsp under /data/app/
 * expected_location: /data/storage/el1/bundle/com.example.app/FLayoutCore/FLayoutCore/ets
 * input_abcPath: /data/app/el1/bundle/public/com.example.app/v122000200/FLayoutCore/FLayoutCore.hsp/
 *                ets/modules_static.abc
 *
 * Case2: hsp under /system/app/
 * expected_location: /system/app/shared_bundles/FlexiableLayout/FlayoutCore/ets
 * input_abcPath: /system/app/shared_bundles/FlexiableLayout/FlayoutCore.hsp
 */
std::string StaticAOTArgsParser::ParseOuterHspLocation(const std::string &abcPath)
{
    // system outer hsp
    if (abcPath.rfind(ArgsIdx::SYS_OUTER_HSP_PATH_PREFIX, 0) == 0) {
        size_t pos = abcPath.find(ArgsIdx::HSP_SUFFIX);
        if (pos == std::string::npos) {
            return "";
        }
        return abcPath.substr(0, pos) + ArgsIdx::ETS_PATH;
    }

    // data outer hsp
    size_t hspSuffixPos = abcPath.find(ArgsIdx::HSP_SUFFIX);
    if (hspSuffixPos == std::string::npos) {
        return "";
    }
    std::string hspLocationIncludingVersionCode = ArgsIdx::APP_SANDBOX_PATH_PREFIX +
        abcPath.substr(ArgsIdx::APP_ABC_PHYS_PATH_PREFIX.size(),
                       hspSuffixPos - ArgsIdx::APP_ABC_PHYS_PATH_PREFIX.size()) +
        ArgsIdx::ETS_PATH;

    // exclude the versionCode for hsp
    // hspLocationIncludingVersionCode: /data/storage/el1/bundle/com.example.app/v122000200/
    //                                  FLayoutCore/FLayoutCore/ets
    // Break hspLocationIncludingVersionCode into segments.
    std::vector<std::string> segments;
    std::stringstream ss(hspLocationIncludingVersionCode);
    std::string item;
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            segments.push_back(item);
        }
    }

    // exlucide the versionCode.
    // versionCode comes after bundleName which follow keywork "bundle"
    std::string hspLocation = "";
    bool foundBundle = false;
    for (size_t i = 0; i < segments.size(); ++i) {
        if (!foundBundle && segments[i] == ArgsIdx::BUNDLE_PATH_SEGMENT) {
            foundBundle = true;
            hspLocation += "/" + segments[i]; // add bundle
            if (i + 1 < segments.size()) {
                hspLocation += "/" + segments[++i]; // add bundleName
            }
            i++; // skip versionCode
            continue;
        }
        hspLocation += "/" + segments[i];
    }

    return hspLocation;
}

std::string StaticAOTArgsParser::ParseModuleName(const std::string &anFilePath)
{
    size_t pos = anFilePath.find_last_of("/");
    if (pos == std::string::npos) {
        LOG_SA(FATAL) << "aot sa parse invalid location";
    }
    std::string moduleName = anFilePath.substr(pos + 1);
    return moduleName;
}

int32_t StaticAOTArgsParser::ParseLocationByBundleType(int32_t bundleType, const std::string &abcPath,
    const std::string &anfilePath, std::string &location)
{
    if (bundleType == static_cast<int32_t>(BundleType::SHARED)) {
        location = ParseOuterHspLocation(abcPath);
    } else if (bundleType == static_cast<int32_t>(BundleType::APP)) {
        location = ParseLocation(anfilePath);
    } else {
        LOG_SA(ERROR) << "bundleType invalid";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    if (location.empty()) {
        LOG_SA(ERROR) << "location is empty";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    return ERR_OK;
}

bool StaticAOTArgsParser::AddProfilePathArg(std::vector<std::string> &argVector, const std::string &pgoDir,
    const std::string &moduleName)
{
    if (pgoDir.empty()) {
        LOG_SA(ERROR) << "pgoDir is empty in partial mode";
        return false;
    }
    std::string profilePath = pgoDir;
    std::string profileName = profilePath + "/" + moduleName + ArgsIdx::AP_SUFFIX;
    std::string pathArg = std::string("path") + Symbols::EQ + profileName;
    argVector.emplace_back(Symbols::PREFIX + ArgsIdx::PAOC_USE_PROFILE + Symbols::COLON + pathArg);
    return true;
}

bool AOTArgsParserBase::ParseBlackListJson(nlohmann::json &jsonObject)
{
    if (!IsFileExists(ArgsIdx::STATIC_PAOC_BLACK_LIST_PATH)) {
        return false;
    }
    std::ifstream inFile;
    inFile.open(ArgsIdx::STATIC_PAOC_BLACK_LIST_PATH, std::ios::in);
    if (!inFile.is_open()) {
        LOG_SA(ERROR) << "read json error";
        return false;
    }
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string fileContent = buffer.str();
    if (!nlohmann::json::accept(fileContent)) {
        LOG_SA(ERROR) << "invalid json when parse profile path";
        return false;
    }
    jsonObject = nlohmann::json::parse(fileContent);
    if (jsonObject.is_discarded()) {
        LOG_SA(ERROR) << "json discarded error";
        inFile.close();
        return false;
    }
    if (jsonObject.is_null()) {
        LOG_SA(ERROR) << "invalid json";
        inFile.close();
        return false;
    }
    inFile.close();
    return true;
}

std::vector<std::string> AOTArgsParserBase::JoinMethodList(const nlohmann::json &methodLists)
{
    std::vector<std::string> methodVec;
    for (size_t i = 0; i < methodLists.size(); ++i) {
        if (!methodLists[i].is_string()) {
            continue;
        }
        methodVec.emplace_back(methodLists[i].get<std::string>());
    }
    return methodVec;
}

std::string AOTArgsParserBase::BuildRegexPattern(const std::vector<std::string> &blacklistedMethods)
{
    if (blacklistedMethods.empty()) {
        return "";
    }

    // Build the regex pattern that excludes these methods
    std::string regexPattern = "^(?!(";
    for (size_t i = 0; i < blacklistedMethods.size(); ++i) {
        if (i > 0) {
            regexPattern += "|";
        }
        // Escape special regex characters in method names
        for (char c : blacklistedMethods[i]) {
            if (c == '\\' || c == '.' || c == '^' || c == '$' || c == '*' ||
                c == '+' || c == '?' || c == '(' || c == ')' || c == '[' ||
                c == ']' || c == '{' || c == '}' || c == '|' || c == '/') {
                regexPattern += "\\";
            }
            regexPattern += c;
        }
    }
    regexPattern += ")$).*";

    return regexPattern;
}

bool StaticAOTArgsParser::CheckBundleNameAndModuleList(const nlohmann::json &item,
    const std::string &bundleName)
{
    if (!item.contains("bundleName") || !item["bundleName"].is_string()) {
        return false;
    }
    if (item["bundleName"].get<std::string>() != bundleName) {
        return false;
    }
    if (!item.contains("moduleLists") || !item["moduleLists"].is_array()) {
        return false;
    }
    return true;
}

bool StaticAOTArgsParser::CheckModuleNameAndMethodList(const nlohmann::json &moduleItem,
    const std::string &moduleName)
{
    if (!moduleItem.contains("name") || !moduleItem["name"].is_string()) {
        return false;
    }
    if (moduleItem["name"].get<std::string>() != moduleName) {
        return false;
    }
    if (!moduleItem.contains("methodLists") || !moduleItem["methodLists"].is_array()) {
        return false;
    }
    return true;
}

std::string StaticAOTArgsParser::ProcessBlackListForBundleAndModule(const nlohmann::json &jsonObject,
                                                                    const std::string &bundleName,
                                                                    const std::string &moduleName)
{
    if (!jsonObject.contains("blackMethodList") || !jsonObject["blackMethodList"].is_array()) {
        return "";
    }
    if (jsonObject["blackMethodList"].empty()) {
        return "";
    }
    std::vector<std::string> blacklistedMethods;
    for (const auto& item : jsonObject["blackMethodList"]) {
        if (!CheckBundleNameAndModuleList(item, bundleName)) {
            continue;
        }
        std::vector<std::string> moduleResults = ProcessMatchingModules(item, moduleName);
        blacklistedMethods.insert(blacklistedMethods.end(), moduleResults.begin(), moduleResults.end());
    }

    if (blacklistedMethods.empty()) {
        return "";
    }

    return AOTArgsParserBase::BuildRegexPattern(blacklistedMethods);
}

std::vector<std::string> StaticAOTArgsParser::ProcessMatchingModules(
    const nlohmann::json &item,
    const std::string &moduleName)
{
    std::vector<std::string> resultVec;
    for (const auto& moduleItem : item["moduleLists"]) {
        if (!CheckModuleNameAndMethodList(moduleItem, moduleName)) {
            continue;
        }
        const auto& methodLists = moduleItem["methodLists"];
        std::vector<std::string> currentMethodVec = JoinMethodList(methodLists);
        resultVec.insert(resultVec.end(), currentMethodVec.begin(), currentMethodVec.end());
    }
    return resultVec;
}

std::string StaticAOTArgsParser::ParseBlackListMethods(const std::string &bundleName,
    const std::string &moduleName)
{
    nlohmann::json jsonObject;
    if (!ParseBlackListJson(jsonObject)) {
        return "";
    }

    if (bundleName.empty()) {
        return "";
    }

    return ProcessBlackListForBundleAndModule(jsonObject, bundleName, moduleName);
}

std::optional<std::unique_ptr<AOTArgsParserBase>> AOTArgsParserFactory::GetParser(
    const AotCompilerArgs &args, bool isEnableStaticCompiler)
{
    if (args.isSysComp && isEnableStaticCompiler) {
        return std::make_unique<StaticFrameworkAOTArgsParser>();
    }
    if (args.moduleArkTSMode == ArgsIdx::ARKTS_DYNAMIC) {
        LOG_SA(INFO) << "aot sa use default compiler";
        return std::make_unique<AOTArgsParser>();
    }
    // After this, only arkTsMode that is static or hybrid will proceed downwards
    if (!isEnableStaticCompiler) {
        return std::nullopt;
    }
    if (args.moduleArkTSMode == ArgsIdx::ARKTS_STATIC || args.moduleArkTSMode == ArgsIdx::ARKTS_HYBRID) {
        return std::make_unique<StaticAOTArgsParser>();
    }
    LOG_SA(ERROR) << "aot sa get invalid code arkTsMode";
    return std::nullopt;
}

bool AOTArgsParserBase::IsFileExists(const std::string &fileName)
{
    std::string realPath;
    if (!panda::ecmascript::RealPath(fileName, realPath)) {
        LOG_SA(INFO) << "get real path failed:" << fileName;
        return false;
    }
    return panda::ecmascript::FileExist(realPath.c_str());
}

int32_t StaticFrameworkAOTArgsParser::Parse(AotCompilerArgs &args, std::vector<std::string> &argVector,
    [[maybe_unused]] int32_t thermalLevel)
{
    std::string abcPath = args.sysCompPath.empty() ? args.abcPath : args.sysCompPath;
    if (abcPath.empty()) {
        LOG_SA(ERROR) << "aot compiler args parsing error: abcPath is empty";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    if (IsFileExists(args.anFileName)) {
        LOG_SA(INFO) << "framework's an is exist";
        return ERR_AOT_COMPILER_CALL_CANCELLED;
    }

    argVector.clear();
    argVector.emplace_back(STATIC_AOT_EXE);

    // Override fields for framework compilation
    args.appIdentifier = ArgsIdx::OWNERID_SHARED_TAG;
    args.bundleUid = OID_SYSTEM;
    args.bundleGid = OID_SYSTEM;

    for (auto &defaultArg : staticFrameworkAOTDefaultArgs) {
        argVector.emplace_back(defaultArg);
    }

    std::string fullBootfiles;
    if (!ParseBootPandaFiles(fullBootfiles)) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    if (fullBootfiles.empty()) {
        LOG_SA(ERROR) << "can not find paoc panda files ";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    argVector.emplace_back(Symbols::PREFIX + ArgsIdx::BOOT_PANDA_FILES + Symbols::EQ + fullBootfiles);

    std::string bundleName = abcPath;
    if (!args.anFileName.empty()) {
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::PAOC_OUTPUT + Symbols::EQ + args.anFileName);
    }
    argVector.emplace_back(Symbols::PREFIX + ArgsIdx::PAOC_PANDA_FILES + Symbols::EQ + abcPath);

    std::string blackListMethods = ParseBlackListMethods(bundleName);
    if (!blackListMethods.empty()) {
        // Use the generated regex pattern instead of comma-separated list
        argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_REGEX + Symbols::EQ + blackListMethods);
    }
    return ERR_OK;
}

bool StaticFrameworkAOTArgsParser::CheckBundleNameAndMethodList(const nlohmann::json &item,
    const std::string &bundleName)
{
    if (!item.contains("bundleName") || !item["bundleName"].is_string()) {
        return false;
    }
    if (item["bundleName"].get<std::string>() != bundleName) {
        return false;
    }
    if (!item.contains("methodLists") || !item["methodLists"].is_array()) {
        return false;
    }
    return true;
}

std::string StaticFrameworkAOTArgsParser::ParseBlackListMethods(const std::string &bundleName)
{
    nlohmann::json jsonObject;
    if (!ParseBlackListJson(jsonObject)) {
        return "";
    }
    std::vector<std::string> blacklistedMethods;
    if (!jsonObject.contains("blackMethodList") || !jsonObject["blackMethodList"].is_array()) {
        return "";
    }
    if (jsonObject["blackMethodList"].empty()) {
        return "";
    }
    for (const auto& item : jsonObject["blackMethodList"]) {
        if (!CheckBundleNameAndMethodList(item, bundleName)) {
            continue;
        }
        const auto& methodLists = item["methodLists"];
        std::vector<std::string> currentMethodVec = JoinMethodList(methodLists);
        blacklistedMethods.insert(blacklistedMethods.end(), currentMethodVec.begin(), currentMethodVec.end());
    }
    if (blacklistedMethods.empty()) {
        return "";
    }

    return AOTArgsParserBase::BuildRegexPattern(blacklistedMethods);
}

std::string StaticFrameworkAOTArgsParser::ParseFrameworkBootPandaFiles(const std::string &bootfiles,
    const std::string &paocPandaFiles)
{
    size_t pos = bootfiles.find(paocPandaFiles);
    std::string frameworkBootPandaFiles;
    if (pos != std::string::npos) {
        frameworkBootPandaFiles += bootfiles.substr(0, pos + paocPandaFiles.length());
    }
    return frameworkBootPandaFiles;
}

} // namespace OHOS::ArkCompiler
