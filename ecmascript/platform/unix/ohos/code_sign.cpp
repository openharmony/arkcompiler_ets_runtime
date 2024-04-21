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

#include <local_code_sign_kit.h>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"

using namespace OHOS::Security::CodeSign;
namespace panda::ecmascript {
void CodeSignatureForAOTFile(const std::string &filename, const std::string &appSignature)
{
    LOG_ECMA(DEBUG) << "start to sign the aot file!";
    ByteBuffer sig;
    if (LocalCodeSignKit::SignLocalCode(appSignature, filename, sig) != CommonErrCode::CS_SUCCESS) {
        LOG_ECMA(ERROR) << "Failed to sign the aot file!";
        return;
    }

    const std::string codeSignLib = "libcode_sign_utils.z.so";
    // mangle for func: int32_t CodeSignUtils::EnforceCodeSignForFile(const std::string&, const ByteBuffer&)
    const char *codeSignFuncStr = "_ZN4OHOS8Security8CodeSign13CodeSignUtils22EnforceCodeSignForFile\
ERKNSt3__h12basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEERKNS1_10ByteBufferE";
    void *libHandle = LoadLib(codeSignLib);
    if (libHandle == nullptr) {
        LOG_ECMA(FATAL) << "Failed to load libcode_sign_utils.z.so!";
    }
    auto enforceCodeSignForFile = reinterpret_cast<int32_t(*)(const std::string&, const ByteBuffer&)>(
        FindSymbol(libHandle, codeSignFuncStr));
    if (enforceCodeSignForFile == nullptr) {
        LOG_ECMA(FATAL) << "Failed to find symbol enforceCodeSignForFile";
    }

    if (enforceCodeSignForFile(filename, sig) != CommonErrCode::CS_SUCCESS) {
        LOG_ECMA(ERROR) << "Failed to enable code signature for the aot file!";
        return;
    }
    CloseLib(libHandle);
    LOG_ECMA(DEBUG) << "sign the aot file success";
}
}  // namespace panda::ecmascript