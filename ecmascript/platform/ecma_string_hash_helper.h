/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_HELPER_H
#define ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_HELPER_H

#include <cstdint>
#include "ecmascript/platform/ecma_string_hash.h"
#if defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS)
#include "ecmascript/platform/arm64/ecma_string_hash_internal.h"
#else
#include "ecmascript/platform/common/ecma_string_hash_internal.h"
#endif

namespace panda::ecmascript {
class EcmaStringHashHelper {
public:
    template <typename T>
    static uint32_t ComputeHashForDataPlatform(const T *data, size_t size,
                                               uint32_t hashSeed)
    {
        return EcmaStringHashInternal::ComputeHashForDataOfLongString(data, size, hashSeed);
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_HELPER_H
