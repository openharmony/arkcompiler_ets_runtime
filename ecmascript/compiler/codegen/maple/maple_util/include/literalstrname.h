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

#ifndef MRT_INCLUDE_LITERALSTRNAME_H
#define MRT_INCLUDE_LITERALSTRNAME_H
#include <map>
#include <cstring>
#include "muid.h"

// literal string naming is shared between maple compiler and runtime, thus not in namespace maplert
const std::string kConstString = "_C_STR_";
const std::string kConstStringPtr = "_PTR_C_STR_";
const std::string kLocalStringPrefix = "L_STR_";
constexpr int kConstStringLen = 7;

class LiteralStrName {
public:
    static int32_t CalculateHashSwapByte(const char16_t *data, uint32_t len);
    static uint32_t CalculateHash(const char16_t *data, uint32_t len, bool dataIsCompress)
    {
        uint32_t hash = 0;
        if (dataIsCompress) {
            const char *dataStart = reinterpret_cast<const char *>(data);
            const char *end = dataStart + len;
            while (dataStart < end) {
                hash = (hash << 5) - hash + static_cast<uint32_t>(*dataStart++); // 5 to calculate the hash code of data
            }
        } else {
            const char16_t *end = data + len;
            while (data < end) {
                hash = (static_cast<unsigned int>(hash) << 5) - hash + *data++; // 5 to calculate the hash code of data
            }
        }
        return hash;
    }

    static std::string GetHexStr(const uint8_t *bytes, uint32_t len);
    static std::string GetLiteralStrName(const uint8_t *bytes, uint32_t len);
    static std::string ComputeMuid(const uint8_t *bytes, uint32_t len);
};

#endif
