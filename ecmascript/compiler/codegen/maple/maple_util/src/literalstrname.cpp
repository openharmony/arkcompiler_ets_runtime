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

#include "literalstrname.h"

// literal string name is shared between maple compiler and runtime, thus not in namespace maplert
// note there is a macor kConstString "_C_STR_" in literalstrname.h
// which need to match
static const std::string mplConstStr("_C_STR_00000000000000000000000000000000");
const uint32_t kMaxBytesLength = 15;

namespace {
const char *kMplDigits = "0123456789abcdef";
}

// Return the hex string of bytes. The result is the combination of prefix "_C_STR_" and hex string of bytes.
// The upper 4 bits and lower 4 bits of bytes[i] are transformed to hex form and restored separately in hex string.
std::string LiteralStrName::GetHexStr(const uint8_t *bytes, uint32_t len)
{
    if (bytes == nullptr) {
        return std::string();
    }
    constexpr uint8_t k16BitShift = 4; // 16 is 1 << 4
    std::string str(mplConstStr, 0, (len << 1) + kConstStringLen);
    for (unsigned i = 0; i < len; ++i) {
        str[(i << 1) + kConstStringLen] =
            kMplDigits[(bytes[i] & 0xf0) >> k16BitShift];  // get the hex value of upper 4 bits of bytes[i]
        str[(i << 1) + kConstStringLen + 1] =
            kMplDigits[bytes[i] & 0x0f];  // get the hex value of lower 4 bits of bytes[i]
    }
    return str;
}

// Return the hash code of data. The hash code is computed as
// s[0] * 31 ^ (len - 1) + s[1] * 31 ^ (len - 2) + ... + s[len - 1],
// where s[i] is the value of swapping the upper 8 bits and lower 8 bits of data[i].
int32_t LiteralStrName::CalculateHashSwapByte(const char16_t *data, uint32_t len)
{
    constexpr uint32_t k32BitShift = 5; // 32 is 1 << 5
    constexpr char16_t kByteShift = 8;
    uint32_t hash = 0;
    const char16_t *end = data + len;
    while (data < end) {
        hash = (hash << k32BitShift) - hash;
        char16_t val = *data++;
        hash += (((val << kByteShift) & 0xff00) | ((val >> kByteShift) & 0xff));
    }
    return static_cast<int32_t>(hash);
}

std::string LiteralStrName::GetLiteralStrName(const uint8_t *bytes, uint32_t len)
{
    if (len <= kMaxBytesLength) {
        return GetHexStr(bytes, len);
    }
    return ComputeMuid(bytes, len);
}

std::string LiteralStrName::ComputeMuid(const uint8_t *bytes, uint32_t len)
{
    DigestHash digestHash = GetDigestHash(*bytes, len);
    return GetHexStr(digestHash.bytes, kDigestHashLength);
}