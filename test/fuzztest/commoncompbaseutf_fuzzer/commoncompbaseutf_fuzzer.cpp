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

#include "commoncompbaseutf_fuzzer.h"
#include "common_components/base/utf_helper.h"
#include "test/fuzztest/containersdequecommon_fuzzer/containersdequecommon_fuzzer.h"

using namespace common::utf_helper;

namespace OHOS {
    void UtfHelperGetValueFromTwoHex(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        GetValueFromTwoHex('0' + data[size - 1] % 10, // 10: val in [0x0, 0x9], convert to ['0', '9']
            'a' + data[size - 1] % 6); // 6：val in ['a', 'f'], convert to ['a', 'f']
    }

    void UtfHelperGetHexChar16(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        GetHexChar16(data[size - 1] % 0xff); // 0xff: Maximum value of uint8
    }
    
    void UtfHelperCombineTwoU16(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        CombineTwoU16(data[size - 1] % 0xff, (size / 2) % 0xff);  // 2：half of the size，0xff: Maximum value of uint8
    }

    void UtfHelperUTF16Decode(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        uint16_t lead = DECODE_LEAD_LOW + data[size - 1] % (DECODE_LEAD_HIGH - DECODE_LEAD_LOW);
        uint16_t tail = DECODE_TRAIL_LOW + data[size - 1] % (DECODE_TRAIL_HIGH - DECODE_TRAIL_LOW);

        UTF16Decode(lead, tail);
    }

    void UtfHelperDebuggerConvertRegionUtf16ToUtf8(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        size_t utf16Len = 8; // 8：length of u16
        size_t utf8Len = 100; // 100：buff len
        size_t start = 0;
        bool modify = ((data[size - 1] % 2 == 0) ? true : false); // 2：Check size is an even number
        uint16_t utf16Value[8] = {
            0x00, // 0 or 2 (special case for \u0000 ==> C080 - 1100'0000 1000'0000）
            0x7F, // 1(0x00, 0x7F]
            0x7FF, // 2 [0x80, 0x7FF]
            0x800, // 3 [0x800, 0xD7FF]
            0xD800, // 3 [0xD800, 0xDFFF]
            0xFFFF, // 3 [0xE000, 0xFFFF]
            0xD800, 0xDFFF}; // 4 [0x10000, 0x10FFFF]
        const uint16_t *utf16ValuePtr = utf16Value;
        uint8_t *utf8Out = (uint8_t*)malloc(utf8Len);
        DebuggerConvertRegionUtf16ToUtf8(utf16ValuePtr, utf8Out, utf16Len, utf8Len, start, modify);
        free(utf8Out);
    }

    void UtfHelperConvertUtf8ToUtf16Pair(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        uint8_t utf8Value1[1] = {0x0};
        const uint8_t *utf8ValuePtr1 = utf8Value1;
        ConvertUtf8ToUtf16Pair(utf8ValuePtr1, (data[size - 1] % 2) == 0); // 2：Check size is even number

        uint8_t utf8Value2[1] = {UTF8_1B_MAX}; // 1: array len
        const uint8_t *utf8ValuePtr2 = utf8Value2;
        ConvertUtf8ToUtf16Pair(utf8ValuePtr1, (data[size - 1] % 2) == 1); // 2：Check size is even number
    }

    void UtfHelperConvertUtf8ToUnicodeChar(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        uint8_t utf8Value11[4] = {0xF4, 0x80, 0x80, 0x40}; // invalid
        const uint8_t *utf8ValuePtr11 = utf8Value11;
        ConvertUtf8ToUnicodeChar(utf8ValuePtr11, static_cast<UtfLength>(data[size - 1] % 4)); // 4: lenth of array
    }

    void UtfHelperConvertUtf16ToUtf8(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        uint16_t utf16Data0[5] = {0x0, 0x7f, 0x7ff, 0xd800, 0xdfff}; // invalid
        uint16_t utf16Data1[2] = {LO_SURROGATE_MIN + 1, LO_SURROGATE_MAX - 1};
        ConvertUtf16ToUtf8(utf16Data0[data[size - 1] % 5], // 5：length of utf16Data0
            utf16Data1[data[size - 1] % 2], (data[size - 1] % 2) == 1, // 2：Check size is even number
            (data[size - 1] % 3) >= 2); // 3：Get Remainder
    }

    void UtfHelperIsValidUTF8(const uint8_t* data, size_t size)
    {
        if (size == 0) {
            return;
        }
        const std::vector<uint8_t> utfDataFourBitVaild = {BIT_MASK_4, BIT_MASK_1 + 0x10, BIT_MASK_1, BIT_MASK_1};

        std::vector<uint8_t> tmpUtfData;
        size_t vecLen = (data[size - 1] % 4 == 0 ? 1 : data[size - 1] % 4); // 4：number of utfDataFourBitVaild array
        for (size_t i = 0; i < vecLen; ++i) {
            tmpUtfData.push_back(utfDataFourBitVaild[i]);
        }
        common::utf_helper::IsValidUTF8(tmpUtfData);
    }
}


// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::UtfHelperGetValueFromTwoHex(data, size);
    OHOS::UtfHelperGetHexChar16(data, size);

    OHOS::UtfHelperCombineTwoU16(data, size);
    OHOS::UtfHelperUTF16Decode(data, size);
    OHOS::UtfHelperDebuggerConvertRegionUtf16ToUtf8(data, size);
    OHOS::UtfHelperConvertUtf8ToUtf16Pair(data, size);
    OHOS::UtfHelperConvertUtf8ToUnicodeChar(data, size);
    OHOS::UtfHelperConvertUtf16ToUtf8(data, size);
    OHOS::UtfHelperIsValidUTF8(data, size);

    return 0;
}