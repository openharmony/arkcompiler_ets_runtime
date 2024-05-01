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
    ERR_AOT_COMPILER_PARAM_FAILED,
    ERR_AOT_COMPILER_CONNECT_FAILED,
    ERR_AOT_COMPILER_CALL_FAILED,
    ERR_AOT_COMPILER_SIGNATURE_FAILED,
    ERR_OK_NO_AOT_FILE,
    ERR_AOT_COMPILER_STOP_FAILED,
    INVALID_ERR_CODE = 9999,
};

class AotCompilerErrorUtil {
public:
    static std::string GetErrorMessage(int32_t errCode);
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOTCOMPILER_ERROR_UTILS_H