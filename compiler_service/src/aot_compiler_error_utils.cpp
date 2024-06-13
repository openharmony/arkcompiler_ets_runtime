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

#include "aot_compiler_error_utils.h"
#include <map>

namespace OHOS::ArkCompiler {
namespace {
const std::map<int32_t, std::string> ERR_MSG_MAP = {
    { ERR_OK,                             "success" },
    { ERR_AOT_COMPILER_PARAM_FAILED,      "aot compiler arguments error" },
    { ERR_AOT_COMPILER_CONNECT_FAILED,    "connect failed" },
    { ERR_AOT_COMPILER_CALL_FAILED,       "call failed" },
    { ERR_AOT_COMPILER_SIGNATURE_FAILED,  "local code sign failed" },
    { ERR_AOT_COMPILER_SIGNATURE_DISABLE, "local code sign disable" },
    { ERR_OK_NO_AOT_FILE,                 "no aot file save" },
    { ERR_AOT_COMPILER_STOP_FAILED,       "aot compiler stop error" }
};
} // namespace

std::string AotCompilerErrorUtil::GetErrorMessage(int32_t errCode)
{
    std::string errMsg;
    auto iter = ERR_MSG_MAP.find(errCode);
    if (iter != ERR_MSG_MAP.end()) {
        errMsg = iter->second;
    } else {
        errMsg = "invalid errCode";
    }
    return errMsg;
}
} // namespace OHOS::ArkCompiler