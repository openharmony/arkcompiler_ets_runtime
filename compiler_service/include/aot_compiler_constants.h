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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_CONSTANTS_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_CONSTANTS_H

#include <string>
#include <unordered_map>

#include "aot_compiler_error_utils.h"

namespace OHOS::ArkCompiler {
namespace ArgsIdx {
const std::string BUNDLE_UID = "BundleUid";
const std::string BUNDLE_GID = "BundleGid";
const std::string AN_FILE_NAME = "anFileName";
const std::string APP_SIGNATURE = "appIdentifier";
const std::string ABC_PATH = "ABC-Path";
const std::string TARGET_COMPILER_MODE = "target-compiler-mode";
const std::string COMPILER_PKG_INFO = "compiler-pkg-info";
const std::string COMPILER_ENABLE_AOT_CODE_COMMENT = "compiler-enable-aot-code-comment";
const std::string COMPILER_LOG_OPT = "compiler-log";
const std::string COMPILER_AN_FILE_MAX_SIZE = "compiler-an-file-max-size";
const std::string AOT_FILE = "aot-file";

const std::string CODE_LANGUAGE = "codeLanguage";
} // namespace ArgsIdx

namespace Symbols {
constexpr const char* PREFIX = "--";
constexpr const char* EQ = "=";
} // namespace Symbols

/**
 * @param RetStatusOfCompiler return code of ark_aot_compiler
 * @attention it must sync with ErrCode of "ets_runtime/ecmascript/aot_compiler.cpp"
 */
enum class RetStatusOfCompiler {
    ERR_OK = (0),   // IMPORTANT: Only if aot compiler SUCCESS and save an/ai SUCCESS, return ERR_OK.
    ERR_FAIL = (-1),
    ERR_HELP = (1),
    ERR_NO_AP = (2),
    ERR_MERGE_AP = (3),
    ERR_CHECK_VERSION = (4),
    ERR_AN_EMPTY = (5),
    ERR_AN_FAIL = (6),
    ERR_AI_FAIL = (7),
};

struct InfoOfCompiler {
    int32_t retCode { -1 };
    std::string mesg;
};

const std::unordered_map<int, InfoOfCompiler> RetInfoOfCompiler {
    {static_cast<int>(RetStatusOfCompiler::ERR_OK), {ERR_OK, "AOT compiler success"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_NO_AP), {ERR_OK_NO_AOT_FILE, "AOT compiler not run: no ap file"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_CHECK_VERSION),
        {ERR_OK_NO_AOT_FILE, "AOT compiler not run: check version"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_MERGE_AP),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: merge ap error"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_AN_EMPTY),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: empty an file"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_AN_FAIL),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: save an error"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_AI_FAIL),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: save ai error"}},
};

const InfoOfCompiler OtherInfoOfCompiler = {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: other error"};
} // namespace OHOS::ArkCompiler
#endif  // OHOS_ARKCOMPILER_AOTCOMPILER_CONSTANTS_H