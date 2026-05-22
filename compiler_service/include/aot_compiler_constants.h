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
#include <sys/types.h>
#include <unordered_map>

#include "aot_compiler_error_utils.h"

namespace OHOS::ArkCompiler {
namespace ArgsIdx {
const std::string BUNDLE_UID = "BundleUid";
const std::string BUNDLE_GID = "BundleGid";
const std::string BUNDLE_TYPE = "bundleType";
const std::string TRIGGER_TYPE = "triggerType";
const std::string AN_FILE_NAME = "anFileName";
const std::string APP_SIGNATURE = "appIdentifier";
const std::string BUNDLE_NAME = "bundleName";
const std::string HOST_BUNDLE_NAME = "hostBundleName";
const std::string PKG_PATH = "pkgPath";
const std::string ABC_NAME = "abcName";
const std::string MODULE_NAME = "moduleName";
const std::string PGO_DIR = "pgoDir";
const std::string ARKTS_DYNAMIC = "dynamic";
const std::string ARKTS_STATIC = "static";
const std::string ARKTS_HYBRID = "hybrid";
const std::string ABC_MODULES_PATH = "ets/modules.abc";
const std::string ABC_MODULES_STATIC_PATH = "ets/modules_static.abc";
const std::string ABC_PATH = "ABC-Path";
const std::string TARGET_COMPILER_MODE = "target-compiler-mode";
const std::string COMPILER_PKG_INFO = "compiler-pkg-info";
const std::string COMPILER_ENABLE_AOT_CODE_COMMENT = "compiler-enable-aot-code-comment";
const std::string COMPILER_LOG_OPT = "compiler-log";
const std::string COMPILER_AN_FILE_MAX_SIZE = "compiler-an-file-max-size";
const std::string AOT_FILE = "aot-file";
const std::string IS_SYSTEM_COMPONENT = "isSysComp";

const std::string APP_ARK_CACHE_PREFIX = "/data/app/el1/public/aot_compiler/ark_cache/";
const std::string FRAMEWORK_ARK_CACHE_PREFIX = "/data/service/el1/public/for-all-app/framework_ark_cache/";

const std::string ARKTS_MODE = "moduleArkTSMode";
const std::string PARTIAL = "partial";
const std::string FULL = "full";

// Compiler argument names (dynamic AOT)
const std::string COMPILER_OPT_BC_RANGE = "compiler-opt-bc-range";
const std::string COMPILER_DEVICE_STATE = "compiler-device-state";
const std::string COMPILER_BASELINE_PGO = "compiler-baseline-pgo";
const std::string COMPILER_EXTERNAL_PKG_INFO = "compiler-external-pkg-info";

// Compiler argument names (static AOT)
const std::string BOOT_PANDA_FILES = "boot-panda-files";
const std::string PAOC_PANDA_FILES = "paoc-panda-files";
const std::string PAOC_LOCATION = "paoc-location";
const std::string PAOC_OUTPUT = "paoc-output";
const std::string PAOC_USE_PROFILE = "paoc-use-profile";
const std::string COMPILER_REGEX = "compiler-regex";

// FD argument names
const std::string AN_FD = "an-fd";
const std::string PAOC_AN_FD = "paoc-an-fd";
const std::string HAP_FD = "hap-fd";
const std::string PAOC_HAP_FD = "paoc-hap-fd";

// File suffixes
const std::string AN_SUFFIX = ".an";
const std::string AP_SUFFIX = ".ap";
const std::string HSP_SUFFIX = ".hsp";

// Static AOT paths
const std::string STATIC_BOOT_PATH = "/system/framework/bootpath.json";
const std::string STATIC_PAOC_BLACK_LIST_PATH = "/etc/ark/static_aot_methods_black_list.json";

// Path prefixes
const std::string APP_SANDBOX_PATH_PREFIX = "/data/storage/el1/bundle/";
const std::string SYS_OUTER_HSP_PATH_PREFIX = "/system/app/";
const std::string APP_ABC_PHYS_PATH_PREFIX = "/data/app/el1/bundle/public/";
const std::string ETS_PATH = "/ets";

// Tags and misc
const std::string OWNERID_SHARED_TAG = "SHARED_LIB_ID";
const std::string EMPTY_JSON_ARRAY = "[]";
const std::string BUNDLE_PATH_SEGMENT = "bundle";
const std::string LOG_OPT_ALLASM = "allasm";
} // namespace ArgsIdx

// UID and GID of system users
constexpr uid_t OID_SYSTEM = 1000;
constexpr uid_t MIN_APP_UID_GID = 10000;  // Minimum app UID/GID

// AOT trigger type values, must keep consistent with BMS
enum class AotTriggerType {
    IDLE = 0,
    INSTALL = 1,
};

constexpr const char* BOOLEAN_TRUE = "1";

namespace Symbols {
constexpr const char* PREFIX = "--";
constexpr const char* EQ = "=";
constexpr const char* COLON = ":";
} // namespace Symbols

// Bundle type values, must keep consistent with BMS
enum class BundleType {
    APP = 0,
    ATOMIC_SERVICE = 1,
    SHARED = 2,
    APP_SERVICE_FWK = 3,
    APP_PLUGIN = 4,
};

/**
 * @param RetStatusOfCompiler return code of ark_aot_compiler
 * @attention it must sync with ErrCode of "ets_runtime/ecmascript/aot_compiler.cpp"
 */
// AOT Parser type for unified validation
enum class AotParserType {
    UNKNOWN,              // Unknown parser type (default)
    DYNAMIC_AOT,          // Dynamic AOT parser
    STATIC_AOT,           // Static AOT parser
    FRAMEWORK_STATIC_AOT  // Framework static AOT parser
};

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
        {ERR_OK_NO_AOT_FILE, "AOT compiler not run: merge ap error"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_AN_EMPTY),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: empty an file"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_AN_FAIL),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: save an error"}},
    {static_cast<int>(RetStatusOfCompiler::ERR_AI_FAIL),
        {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: save ai error"}},
};

const InfoOfCompiler OtherInfoOfCompiler = {ERR_AOT_COMPILER_CALL_FAILED, "AOT compiler fail: other error"};
} // namespace OHOS::ArkCompiler
#endif
