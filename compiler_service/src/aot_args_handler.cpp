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
#include <fstream>
#include <nlohmann/json.hpp>

#include "aot_args_list.h"
#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
#include "parameters.h"
#endif

namespace OHOS::ArkCompiler {
const std::string AOT_FILE = "aot-file";

const std::string STATIC_BOOT_PANDA_FILES = "boot-panda-files";
const std::string STATIC_PAOC_PANDA_FILES = "paoc-panda-files";
const std::string STATIC_PAOC_LOCATION = "paoc-location";
const std::string STATIC_PAOC_OUTPUT = "paoc-output";
const std::string STATIC_BOOT_PATH = "/system/framework/bootpath.json";

const std::string ARKTS_1_1 = "1.1";
const std::string ARKTS_1_2 = "1.2";
const std::string ARKTS_HYBRID = "hybrid";

const std::string AN_SUFFIX = ".an";
const std::string APP_SANBOX_PATH_PREFIX = "/data/storage/el1/bundle/";
const std::string ETS_PATH = "/ets";

AOTArgsHandler::AOTArgsHandler(const std::unordered_map<std::string, std::string> &argsMap) : argsMap_(argsMap)
{
    parser_ = *AOTArgsParserFactory::GetParser(argsMap);
}

int32_t AOTArgsHandler::Handle(int32_t thermalLevel)
{
    if (argsMap_.empty()) {
        LOG_SA(ERROR) << "pass empty args to aot sa";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    std::lock_guard<std::mutex> lock(hapArgsMutex_);
    int32_t ret = parser_->Parse(argsMap_, hapArgs_, thermalLevel);
    return ret;
}

std::vector<const char*> AOTArgsHandler::GetAotArgs() const
{
    std::lock_guard<std::mutex> lock(hapArgsMutex_);
    std::vector<const char*> argv;
    argv.reserve(hapArgs_.argVector.size() + 1);  // 1: for nullptr
    for (auto &arg : hapArgs_.argVector) {
        argv.emplace_back(arg.c_str());
    }

    return argv;
}

void AOTArgsHandler::GetBundleId(int32_t &bundleUid, int32_t &bundleGid) const
{
    std::lock_guard<std::mutex> lock(hapArgsMutex_);
    bundleUid = hapArgs_.bundleUid;
    bundleGid = hapArgs_.bundleGid;
}

std::string AOTArgsHandler::GetFileName() const
{
    std::lock_guard<std::mutex> lock(hapArgsMutex_);
    return hapArgs_.fileName;
}

std::string AOTArgsHandler::GetCodeSignArgs() const
{
    std::lock_guard<std::mutex> lock(hapArgsMutex_);
    return hapArgs_.signature;
}

int32_t AOTArgsParserBase::FindArgsIdxToInteger(const std::unordered_map<std::string, std::string> &argsMap,
                                                const std::string &keyName, int32_t &bundleID)
{
    if (argsMap.find(keyName) == argsMap.end()) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    if (argsMap.at(keyName).empty() || !isdigit(argsMap.at(keyName).at(0))) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    const char* beginPtr = argsMap.at(keyName).data();
    const char* endPtr = argsMap.at(keyName).data() + argsMap.at(keyName).size();
    auto res = std::from_chars(beginPtr, endPtr, bundleID);
    if ((res.ec != std::errc()) || (res.ptr != endPtr)) {
        LOG_SA(ERROR) << "trigger exception as converting string to integer";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    return ERR_OK;
}

int32_t AOTArgsParserBase::FindArgsIdxToString(const std::unordered_map<std::string, std::string> &argsMap,
                                               const std::string &keyName, std::string &bundleArg)
{
    if (argsMap.find(keyName) == argsMap.end()) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    bundleArg = argsMap.at(keyName);
    return ERR_OK;
}

int32_t AOTArgsParser::Parse(const std::unordered_map<std::string, std::string> &argsMap, HapArgs &hapArgs,
                             int32_t thermalLevel)
{
    std::string abcPath;
    if ((FindArgsIdxToInteger(argsMap, ArgsIdx::BUNDLE_UID, hapArgs.bundleUid) != ERR_OK)   ||
        (FindArgsIdxToInteger(argsMap, ArgsIdx::BUNDLE_GID, hapArgs.bundleGid) != ERR_OK)   ||
        (FindArgsIdxToString(argsMap, ArgsIdx::AN_FILE_NAME, hapArgs.fileName) != ERR_OK)   ||
        (FindArgsIdxToString(argsMap, ArgsIdx::APP_SIGNATURE, hapArgs.signature) != ERR_OK) ||
        (FindArgsIdxToString(argsMap, ArgsIdx::ABC_PATH, abcPath) != ERR_OK)) {
        LOG_SA(ERROR) << "aot compiler args parsing error";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    hapArgs.argVector.clear();
    hapArgs.argVector.emplace_back(AOT_EXE);

    // service process add aot compile args here
    AddExpandArgs(hapArgs.argVector, thermalLevel);

    for (auto &argPair : argsMap) {
        if (AotArgsList.find(argPair.first) != AotArgsList.end()) {
            hapArgs.argVector.emplace_back(Symbols::PREFIX + argPair.first + Symbols::EQ + argPair.second);
        }
    }

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
    SetEnableCodeCommentBySysParam(hapArgs);
    SetAnFileMaxSizeBySysParam(hapArgs);
#endif

    hapArgs.argVector.emplace_back(abcPath);
    return ERR_OK;
}

#ifdef ENABLE_COMPILER_SERVICE_GET_PARAMETER
void AOTArgsParser::SetAnFileMaxSizeBySysParam(HapArgs &hapArgs)
{
    int anFileMaxSize = OHOS::system::GetIntParameter<int>("ark.aot.compiler_an_file_max_size", -1);
    if (anFileMaxSize >= 0) {
        hapArgs.argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_AN_FILE_MAX_SIZE + Symbols::EQ +
                                       std::to_string(anFileMaxSize));
    }
}

void AOTArgsParser::SetEnableCodeCommentBySysParam(HapArgs &hapArgs)
{
    bool enableAotCodeComment = OHOS::system::GetBoolParameter("ark.aot.code_comment.enable", false);
    if (enableAotCodeComment) {
        hapArgs.argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_ENABLE_AOT_CODE_COMMENT + Symbols::EQ +
                                       "true");
        hapArgs.argVector.emplace_back(Symbols::PREFIX + ArgsIdx::COMPILER_LOG_OPT + Symbols::EQ + "allasm");
    }
}
#endif

void AOTArgsParser::AddExpandArgs(std::vector<std::string> &argVector, int32_t thermalLevel)
{
    std::string thermalLevelArg = "--compiler-thermal-level=" + std::to_string(thermalLevel);
    argVector.emplace_back(thermalLevelArg);
}

int32_t StaticAOTArgsParser::Parse(const std::unordered_map<std::string, std::string> &argsMap,
                                   HapArgs &hapArgs, [[maybe_unused]] int32_t thermalLevel)
{
    std::string abcPath;
    if ((FindArgsIdxToInteger(argsMap, ArgsIdx::BUNDLE_UID, hapArgs.bundleUid) != ERR_OK)   ||
        (FindArgsIdxToInteger(argsMap, ArgsIdx::BUNDLE_GID, hapArgs.bundleGid) != ERR_OK)   ||
        (FindArgsIdxToString(argsMap, ArgsIdx::AN_FILE_NAME, hapArgs.fileName) != ERR_OK)   ||
        (FindArgsIdxToString(argsMap, ArgsIdx::APP_SIGNATURE, hapArgs.signature) != ERR_OK) ||
        (FindArgsIdxToString(argsMap, ArgsIdx::ABC_PATH, abcPath) != ERR_OK)) {
        LOG_SA(ERROR) << "aot compiler args parsing error";
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }

    hapArgs.argVector.clear();
    hapArgs.argVector.emplace_back(STATIC_AOT_EXE);

    for (auto defaultArg : StaticAotDefaultArgs) {
        hapArgs.argVector.emplace_back(defaultArg);
    }

    std::string bootfiles;
    if (!ParseBootPandaFiles(bootfiles)) {
        return ERR_AOT_COMPILER_PARAM_FAILED;
    }
    hapArgs.argVector.emplace_back(Symbols::PREFIX + STATIC_BOOT_PANDA_FILES + Symbols::EQ + bootfiles);

    std::string anfilePath;
    for (auto &argPair : argsMap) {
        // for 1.2, replace aot-file by paoc-output
        if (argPair.first == AOT_FILE) {
            anfilePath = argPair.second;
            std::string anFileName = anfilePath + AN_SUFFIX;
            hapArgs.argVector.emplace_back(Symbols::PREFIX + STATIC_PAOC_OUTPUT + Symbols::EQ + anFileName);
            continue;
        }

        if (StaticAotArgsList.find(argPair.first) != AotArgsList.end()) {
            hapArgs.argVector.emplace_back(Symbols::PREFIX + argPair.first + Symbols::EQ + argPair.second);
        }
    }

    std::string location = ParseLocation(anfilePath);
    hapArgs.argVector.emplace_back(Symbols::PREFIX + STATIC_PAOC_LOCATION + Symbols::EQ + location);
    hapArgs.argVector.emplace_back(Symbols::PREFIX + STATIC_PAOC_PANDA_FILES + Symbols::EQ + abcPath);

    return ERR_OK;
}

bool StaticAOTArgsParser::ParseBootPandaFiles(std::string &bootfiles)
{
    std::ifstream inFile;
    inFile.open(STATIC_BOOT_PATH, std::ios::in);
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

std::string StaticAOTArgsParser::ParseLocation(std::string &anFilePath)
{
    size_t pos = anFilePath.find_last_of("/");
    if (pos == std::string::npos) {
        LOG_SA(FATAL) << "aot sa parse invalid location";
    }
    std::string moduleName = anFilePath.substr(pos + 1);
    std::string location = APP_SANBOX_PATH_PREFIX + moduleName + ETS_PATH;
    return location;
}

std::optional<std::unique_ptr<AOTArgsParserBase>> AOTArgsParserFactory::GetParser(
    const std::unordered_map<std::string, std::string> &argsMap)
{
    std::string codeLanguage = ARKTS_1_1;
    if (AOTArgsParserBase::FindArgsIdxToString(argsMap, ArgsIdx::CODE_LANGUAGE, codeLanguage) != ERR_OK) {
        LOG_SA(INFO) << "aot sa failed to get language version";
    }

    if (codeLanguage == ARKTS_1_1) {
        LOG_SA(INFO) << "aot sa use default compiler";
        return std::make_unique<AOTArgsParser>();
    } else if (codeLanguage == ARKTS_1_2 || codeLanguage == ARKTS_HYBRID) {
        LOG_SA(INFO) << "aot sa use static compiler";
        return std::make_unique<StaticAOTArgsParser>();
    }
    LOG_SA(FATAL) << "aot sa get invalid code language version";
    return std::nullopt;
}
} // namespace OHOS::ArkCompiler
