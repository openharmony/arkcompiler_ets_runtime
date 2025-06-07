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

#ifndef ECMASCRIPT_PLATFORM_JSON_PLATFORM_HELPER_H
#define ECMASCRIPT_PLATFORM_JSON_PLATFORM_HELPER_H

#include <cstdint>
#if defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS)
#include "ecmascript/platform/arm64/json_helper_internal.h"
#else
#include "ecmascript/platform/common/json_helper_internal.h"
#endif

namespace panda::ecmascript::base {
class JsonPlatformHelper {
public:
    static bool ReadJsonStringRangeForPlatformForUtf8(
        bool &isFastString, const uint8_t *current, const uint8_t *range, const uint8_t *&end)
    {
        return JsonHelperInternal::ReadJsonStringRangeForUtf8(isFastString, current, range, end);
    }
};
}
#endif //ECMASCRIPT_PLATFORM_JSON_PLATFORM_HELPER_H
