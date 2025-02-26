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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_ERROR_UTILS_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_ERROR_UTILS_H

#include <cstdint>
#include <string>

namespace OHOS::ArkCompiler {
enum {
    ERR_OK = 0,
    ERR_FAIL = -1,
    ERR_AOT_COMPILER_OFFSET = 10000,
    ERR_OK_NO_AOT_FILE = 10001,
    ERR_AOT_COMPILER_PARAM_FAILED = 10002,
    ERR_AOT_COMPILER_CONNECT_FAILED = 10003,
    ERR_AOT_COMPILER_SIGNATURE_FAILED = 10004,
    ERR_AOT_COMPILER_SIGNATURE_DISABLE = 10005,
    ERR_AOT_COMPILER_CALL_FAILED = 10006,
    ERR_AOT_COMPILER_STOP_FAILED = 10007,
    ERR_AOT_COMPILER_CALL_CRASH = 10008,
    ERR_AOT_COMPILER_CALL_CANCELLED = 10009,
    INVALID_ERR_CODE = 99999,
};

class AotCompilerErrorUtil {
public:
    static std::string GetErrorMessage(int32_t errCode);
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOTCOMPILER_ERROR_UTILS_H