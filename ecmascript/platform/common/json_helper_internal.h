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

#ifndef ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_COMMON_H
#define ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_COMMON_H

#include <cstddef>
#include <cstdint>

namespace panda::ecmascript::base {
class JsonHelperInternal {
friend class JsonPlatformHelper;
private:
    static constexpr uint16_t UTF16_ASCII_END = 0X7F;
    static constexpr uint16_t UTF16_SPACE_CHAR = ' ';

    static bool ReadJsonStringRangeForUtf8(bool &isFastString, const uint8_t *&current,
            const uint8_t *range, const uint8_t *&end)
    {
        for (const uint8_t *cur = current; cur != range; ++cur) {
            uint8_t c = *cur;
            if (c == '"') {
                end = cur;
                return true;
            }
            if (c == '\\') {
                isFastString = false;
                return true;
            }
            if (c < ' ') {
                current = cur;
                return false;
            }
        }
        current = range;
        return false;
    }

    static bool ReadJsonStringRangeForUtf16(bool &isFastString, bool &isAscii,
                                            const uint16_t *&current,
                                            const uint16_t *range,
                                            const uint16_t *&end)
    {
        for (const uint16_t *cur = current; cur != range; ++cur) {
            uint16_t c = *cur;
            if (c == '"') {
                end = cur;
                return true;
            }
            if (c == '\\') {
                isFastString = false;
                return true;
            }
            if (!IsLegalAsciiCharacter(c, isAscii)) {
                current = cur;
                return false;
            }
        }
        current = range;
        return false;
    }

    static bool IsLegalAsciiCharacter(uint16_t c, bool &isAscii)
    {
        if (c <= UTF16_ASCII_END) {
            return c >= UTF16_SPACE_CHAR;
        }
        isAscii = false;
        return true;
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthForUtf8(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                         const uint8_t *&current,
                                         const uint8_t *range,
                                         const uint8_t *&end,
                                         CheckBackslashFunc checkBackslash)
    {
        const uint8_t *last = inObjOrArrOrMap ? range : end;
        for (const uint8_t *cur = current; cur < last; ++cur) {
            uint8_t c = *cur;
            if (inObjOrArrOrMap && c == '"') {
                end = cur;
                return true;
            }
            if (c == '\\') {
                if (!checkBackslash(cur, last, isAscii)) {
                    current = cur;
                    return false;
                }
            } else if (c < ' ') {
                current = cur;
                return false;
            } else if (c > UTF16_ASCII_END) {
                isAscii = false;
            }
            ++length;
        }
        return !inObjOrArrOrMap;
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthForUtf16(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                          const uint16_t *&current,
                                          const uint16_t *range,
                                          const uint16_t *&end,
                                          CheckBackslashFunc checkBackslash)
    {
        const uint16_t *last = inObjOrArrOrMap ? range : end;
        for (const uint16_t *cur = current; cur < last; ++cur) {
            uint16_t c = *cur;
            if (inObjOrArrOrMap && c == '"') {
                end = cur;
                return true;
            }
            if (c == '\\') {
                if (!checkBackslash(cur, last, isAscii)) {
                    current = cur;
                    return false;
                }
            } else if (c < UTF16_SPACE_CHAR) {
                current = cur;
                return false;
            } else if (c > UTF16_ASCII_END) {
                isAscii = false;
            }
            ++length;
        }
        return !inObjOrArrOrMap;
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForUtf8(Char *&p,
                                             const uint8_t *&current,
                                             const uint8_t *end,
                                             ParseBackslashFunc parseBackslash)
    {
        while (current <= end) {
            uint8_t c = *current;
            if (c == '\\') {
                parseBackslash(p);
            } else {
                *p++ = static_cast<Char>(c);
            }
            ++current;
        }
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForUtf16(Char *&p,
                                              const uint16_t *&current,
                                              const uint16_t *end,
                                              ParseBackslashFunc parseBackslash)
    {
        while (current <= end) {
            uint16_t c = *current;
            if (c == '\\') {
                parseBackslash(p);
            } else {
                *p++ = static_cast<Char>(c);
            }
            ++current;
        }
    }
};
}
#endif //ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_COMMON_H
