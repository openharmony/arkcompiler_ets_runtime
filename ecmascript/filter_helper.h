/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_FILTER_HELPER_H
#define ECMASCRIPT_FILTER_HELPER_H

#include "ecmascript/property_attributes.h"

#define NATIVE_DEFAULT 0
#define NATIVE_WRITABLE 1 << 0
#define NATIVE_ENUMERABLE 1 << 1
#define NATIVE_CONFIGURABLE 1 << 2
#define NATIVE_KEY_SKIP_STRINGS 1 << 3
#define NATIVE_KEY_SKIP_SYMBOLS 1 << 4
#define NATIVE_KEY_INCLUDE_PROTOTYPES 1 << 5
#define NATIVE_KEY_OWN_ONLY 1 << 6
#define NATIVE_KEY_KEEP_NUMBERS 1 << 7
#define NATIVE_KEY_NUMBERS_TO_STRINGS 1 << 8

namespace panda::ecmascript {
class PropertyAttributes;
class ObjectOperator;

class FilterHelper {
public:
    template <typename T>
    static bool IgnoreKeyByFilter(T &desc, uint32_t filter)
    {
        if (filter == NATIVE_DEFAULT) {
            return false;
        }
        if ((filter & NATIVE_WRITABLE) && !desc.IsWritable()) {
            return true;
        }
        if ((filter & NATIVE_ENUMERABLE) && !desc.IsEnumerable()) {
            return true;
        }
        if ((filter & NATIVE_CONFIGURABLE) && !desc.IsConfigurable()) {
            return true;
        }
        return false;
    }
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_GENERATOR_HELPER_H
