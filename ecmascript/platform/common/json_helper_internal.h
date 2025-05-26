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
namespace panda::ecmascript::base {
class JsonHelperInternal {
friend class JsonPlatformHelper;
private:
    static bool ReadJsonStringRangeForUtf8(bool &isFastString, const uint8_t *current,
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
                return false;
            }
        }
        return false;
    }
};
}
#endif //ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_COMMON_H
