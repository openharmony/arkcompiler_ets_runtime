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
#include "ecmascript/string/base_string.h"
#include "ecmascript/string/base_string-inl.h"

#include <codecvt>
#include <locale>
#include "common_components/base/utf_helper.h"
#include "ecmascript/platform/string_hash.h"
#include "ecmascript/platform/string_hash_helper.h"

namespace panda::ecmascript {
// To change the hash algorithm of BaseString, please modify BaseString::CalculateConcatHashCode
// and BaseStringHashHelper::ComputeHashForDataPlatform simultaneously!!
template<typename T>
uint32_t ComputeHashForDataInternal(const T *data, size_t size, uint32_t hashSeed)
{
    if (size <= static_cast<size_t>(StringHash::MIN_SIZE_FOR_UNROLLING)) {
        uint32_t hash = hashSeed;
        for (uint32_t i = 0; i < size; i++) {
            hash = (hash << static_cast<uint32_t>(StringHash::HASH_SHIFT)) - hash + data[i];
        }
        return hash;
    }
    return StringHashHelper::ComputeHashForDataPlatform(data, size, hashSeed);
}


template <typename T1, typename T2>
bool IsSubStringAtSpan(common::Span<T1>& lhsSp, common::Span<T2>& rhsSp, uint32_t offset)
{
    size_t rhsSize = rhsSp.size();
    DCHECK_CC(rhsSize + offset <= lhsSp.size());
    for (size_t i = 0; i < rhsSize; ++i) {
        auto left = static_cast<int32_t>(lhsSp[offset + static_cast<uint32_t>(i)]);
        auto right = static_cast<int32_t>(rhsSp[i]);
        if (left != right) {
            return false;
        }
    }
    return true;
}

template
bool IsSubStringAtSpan<const uint8_t, const uint8_t>(common::Span<const uint8_t>& lhsSp,
                                                     common::Span<const uint8_t>& rhsSp,
                                                     uint32_t offset);
template
bool IsSubStringAtSpan<const uint16_t, const uint16_t>(common::Span<const uint16_t>& lhsSp,
                                                       common::Span<const uint16_t>& rhsSp,
                                                       uint32_t offset);
template
bool IsSubStringAtSpan<const uint8_t, const uint16_t>(common::Span<const uint8_t>& lhsSp,
                                                      common::Span<const uint16_t>& rhsSp,
                                                      uint32_t offset);
template
bool IsSubStringAtSpan<const uint16_t, const uint8_t>(common::Span<const uint16_t>& lhsSp,
                                                      common::Span<const uint8_t>& rhsSp,
                                                      uint32_t offset);


std::u16string Utf16ToU16String(const uint16_t* utf16Data, uint32_t dataLen)
{
    auto* char16tData = reinterpret_cast<const char16_t*>(utf16Data);
    std::u16string u16str(char16tData, dataLen);
    return u16str;
}

std::u16string Utf8ToU16String(const uint8_t* utf8Data, uint32_t dataLen)
{
    auto* charData = reinterpret_cast<const char*>(utf8Data);
    std::string str(charData, dataLen);
    std::u16string u16str = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(str);
    return u16str;
}
} // namespace panda::ecmascript
