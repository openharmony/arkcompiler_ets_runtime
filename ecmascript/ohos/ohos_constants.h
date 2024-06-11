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

#ifndef ECMASCRIPT_COMPILER_OHOS_CONSTANTS_H
#define ECMASCRIPT_COMPILER_OHOS_CONSTANTS_H

namespace panda::ecmascript::ohos {
namespace OhosConstants {
constexpr const char* SANDBOX_ARK_PROFILE_PATH = "/data/storage/ark-profile";
constexpr const char* ARM64 = "arm64";
constexpr const char* RUNTIME_SO_PATH = "/system/lib64/platformsdk/libark_jsruntime.so";
constexpr const char* PATH_SEPARATOR = "/";
constexpr const char* AOT_RUNTIME_INFO_NAME = "aot_runtime_info.log";
constexpr const char* SPLIT_STR = "|";
}  // namespace OhosConstants
}  // namespace panda::ecmascript::ohos
#endif  // ECMASCRIPT_COMPILER_OHOS_CONSTANTS_H