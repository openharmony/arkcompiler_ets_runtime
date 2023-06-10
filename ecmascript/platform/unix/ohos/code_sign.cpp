/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/code_sign.h"

#include <code_sign_utils.h>
#include <local_code_sign_kit.h>

#include "ecmascript/log_wrapper.h"

using namespace OHOS::Security::CodeSign;
namespace panda::ecmascript {
void CodeSignForAOTFile(const std::string &filename)
{
    LOG_ECMA(DEBUG) << "start to sign the aot file!";
    ByteBuffer sig;
    if (LocalCodeSignKit::SignLocalCode(filename, sig) != CommonErrCode::CS_SUCCESS) {
        LOG_ECMA(ERROR) << "Failed to sign the aot file!";
        return;
    }
    if (CodeSignUtils::EnforceCodeSignForFile(filename, sig) != CommonErrCode::CS_SUCCESS) {
        LOG_ECMA(ERROR) << "Failed to enable code signature for the aot file!";
        return;
    }
    LOG_ECMA(DEBUG) << "sign the aot file success";
}
}  // namespace panda::ecmascript