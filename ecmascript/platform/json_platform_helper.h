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

#include <cstddef>
#include <cstdint>
#if defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS)
#include "ecmascript/platform/arm64/json_helper_internal.h"
#elif defined(PANDA_TARGET_AMD64)
#include "ecmascript/platform/x64/json_helper_internal.h"
#else
#include "ecmascript/platform/common/json_helper_internal.h"
#endif

namespace panda::ecmascript::base {
class JsonPlatformHelper {
public:
    static bool ReadJsonStringRangeForPlatformForUtf8(
        bool &isFastString, const uint8_t *&current, const uint8_t *range, const uint8_t *&end)
    {
        return JsonHelperInternal::ReadJsonStringRangeForUtf8(isFastString, current, range, end);
    }

    static bool ReadJsonStringRangeForPlatformForUtf16(
        bool &isFastString, bool &isAscii, const uint16_t *&current, const uint16_t *range, const uint16_t *&end)
    {
        return JsonHelperInternal::ReadJsonStringRangeForUtf16(isFastString, isAscii, current, range, end);
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthForPlatformForUtf8(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                                    const uint8_t *&current,
                                                    const uint8_t *range,
                                                    const uint8_t *&end,
                                                    CheckBackslashFunc checkBackslash)
    {
        return JsonHelperInternal::ParseStringLengthForUtf8(
            length, isAscii, inObjOrArrOrMap, current, range, end, checkBackslash);
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthForPlatformForUtf16(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                                     const uint16_t *&current,
                                                     const uint16_t *range,
                                                     const uint16_t *&end,
                                                     CheckBackslashFunc checkBackslash)
    {
        return JsonHelperInternal::ParseStringLengthForUtf16(
            length, isAscii, inObjOrArrOrMap, current, range, end, checkBackslash);
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForPlatformForUtf8(Char *&p,
                                                        const uint8_t *&current,
                                                        const uint8_t *end,
                                                        ParseBackslashFunc parseBackslash)
    {
        JsonHelperInternal::CopyCharWithBackslashForUtf8(p, current, end, parseBackslash);
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForPlatformForUtf16(Char *&p,
                                                         const uint16_t *&current,
                                                         const uint16_t *end,
                                                         ParseBackslashFunc parseBackslash)
    {
        JsonHelperInternal::CopyCharWithBackslashForUtf16(p, current, end, parseBackslash);
    }
};
}
#endif //ECMASCRIPT_PLATFORM_JSON_PLATFORM_HELPER_H
