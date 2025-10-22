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

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-magic-numbers)

#ifndef ECMASCRIPT_CACHED_EXTERNAL_STRING_INL_H
#define ECMASCRIPT_CACHED_EXTERNAL_STRING_INL_H

#include <vector>

#include "ecmascript/string/base_string.h"
#include "ecmascript/string/external_string.h"
#include "objects/utils/utf_utils.h"
#include "securec.h"

namespace panda::ecmascript {
template <bool VERIFY>
uint16_t CachedExternalString::Get(int32_t index) const
{
    auto length = static_cast<int32_t>(GetLength());
    if constexpr (VERIFY) {
        if ((index < 0) || (index >= length)) {
            return 0;
        }
    }
    if (!IsUtf16()) {
        common::Span<const uint8_t> sp(GetDataUtf8(), length);
        return sp[index];
    }
    common::Span<const uint16_t> sp(GetDataUtf16(), length);
    return sp[index];
}

template <typename Allocator, panda::objects_traits::enable_if_is_allocate<Allocator, common::BaseObject *>>
CachedExternalString *CachedExternalString::Create(Allocator &&allocator, ExternalNonMovableStringResource* resource,
                                                   void* data, size_t length, bool compressed)
{
    BaseObject *obj = std::invoke(std::forward<Allocator>(allocator),
        CachedExternalString::SIZE, EcmaStringType::CACHED_EXTERNAL_STRING);
    CachedExternalString *string = CachedExternalString::Cast(obj);
    string->InitLengthAndFlags(length, compressed);
    string->SetMixHashcode(0);
    string->SetResource(resource);
    string->SetCachedResourceData(data);
    return string;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CACHED_EXTERNAL_STRING_INL_H

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-magic-numbers)