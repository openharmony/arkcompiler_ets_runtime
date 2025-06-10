/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_STUBS_RUNTIME_OPTIMIZED_STUBS_INL_H
#define ECMASCRIPT_STUBS_RUNTIME_OPTIMIZED_STUBS_INL_H

#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/ecma_string-inl.h"

namespace panda::ecmascript {
template <typename T>
uint16_t RuntimeStubs::GetCodeUnit(Span<T> &sp, int32_t index, int32_t length)
{
    if ((index < 0) || (index >= length)) {
        return 0;
    }
    return sp[index];
}

template <typename T>
JSTaggedValue RuntimeStubs::DecodePercentEncoding(JSThread *thread, int32_t &n, int32_t &k,
                                                  const JSHandle<EcmaString> &str, uint8_t &bb,
                                                  std::vector<uint8_t> &oct, Span<T> &sp, int32_t strLen)
{
    CString errorMsg;
    int32_t j = 1;
    while (j < n) {
        k++;
        uint16_t codeUnit = GetCodeUnit<T>(sp, k, strLen);
        // b. If the code unit at index k within string is not "%", throw a URIError exception.
        // c. If the code units at index (k +1) and (k + 2) within string do not represent hexadecimal
        //    digits, throw a URIError exception.
        if (!(codeUnit == '%')) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }

        uint16_t frontChart = GetCodeUnit<T>(sp, k + 1, strLen);
        uint16_t behindChart = GetCodeUnit<T>(sp, k + 2, strLen);  // 2: means plus 2
        if (!(common::utf_helper::IsHexDigits(frontChart) && common::utf_helper::IsHexDigits(behindChart))) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
#if !ENABLE_NEXT_OPTIMIZATION
        bb = GetValueFromTwoHex(frontChart, behindChart);
#else
        bb = common::utf_helper::GetValueFromTwoHex(frontChart, behindChart);
#endif
        // e. If the two most significant bits in B are not 10, throw a URIError exception.
        if (!((bb & common::utf_helper::BIT_MASK_2) == common::utf_helper::BIT_MASK_1)) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        k += 2;  // 2: means plus 2
        oct.push_back(bb);
        j++;
    }
    return JSTaggedValue::True();
}

JSTaggedValue RuntimeStubs::UTF16EncodeCodePoint(JSThread *thread, const std::vector<uint8_t> &oct,
                                                 const JSHandle<EcmaString> &str, std::u16string &resStr)
{
    if (!common::utf_helper::IsValidUTF8(oct)) {
        CString errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
    uint32_t vv = base::StringHelper::Utf8ToU32String(oct);
    if (vv < common::utf_helper::DECODE_SECOND_FACTOR) {
#if !ENABLE_NEXT_OPTIMIZATION
        resStr = base::StringHelper::Utf16ToU16String(reinterpret_cast<uint16_t *>(&vv), 1);
    } else {
        uint16_t lv = (((vv - common::utf_helper::DECODE_SECOND_FACTOR) & common::utf_helper::BIT16_MASK) +
            common::utf_helper::DECODE_TRAIL_LOW);
        // NOLINT
        uint16_t hv = ((((vv - common::utf_helper::DECODE_SECOND_FACTOR) >> 10U) & common::utf_helper::BIT16_MASK) +
            common::utf_helper::DECODE_LEAD_LOW);  // 10: means shift left by 10 digits
        resStr = base::StringHelper::Append(base::StringHelper::Utf16ToU16String(&hv, 1),
                                            base::StringHelper::Utf16ToU16String(&lv, 1));
#else
        resStr.append(base::StringHelper::Utf16ToU16String(reinterpret_cast<uint16_t *>(&vv), 1));
    } else {
        uint16_t lv = (((vv - common::utf_helper::DECODE_SECOND_FACTOR) & common::utf_helper::BIT16_MASK) +
            common::utf_helper::DECODE_TRAIL_LOW);
        // NOLINT
        uint16_t hv = ((((vv - common::utf_helper::DECODE_SECOND_FACTOR) >> 10U) & common::utf_helper::BIT16_MASK) +
            common::utf_helper::DECODE_LEAD_LOW);  // 10: means shift left by 10 digits
        resStr.push_back(static_cast<const char16_t>(hv));
        resStr.push_back(static_cast<const char16_t>(lv));
#endif
    }
    return JSTaggedValue::True();
}

template <typename T>
JSTaggedValue RuntimeStubs::DecodePercentEncoding(JSThread *thread, const JSHandle<EcmaString> &str, int32_t &k,
                                                  int32_t strLen, std::u16string &resStr, Span<T> &sp)
{
    [[maybe_unused]] uint32_t start = static_cast<uint32_t>(k);
    CString errorMsg;
    // ii. If k + 2 is greater than or equal to strLen, throw a URIError exception.
    // iii. If the code units at index (k+1) and (k + 2) within string do not represent hexadecimal digits,
    //      throw a URIError exception.
    if ((k + 2) >= strLen) {  // 2: means plus 2
        errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
    uint16_t frontChar = GetCodeUnit<T>(sp, k + 1, strLen);
    uint16_t behindChar = GetCodeUnit<T>(sp, k + 2, strLen);  // 2: means plus 2
    if (!(common::utf_helper::IsHexDigits(frontChar) && common::utf_helper::IsHexDigits(behindChar))) {
        errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
#if !ENABLE_NEXT_OPTIMIZATION
    uint8_t bb = GetValueFromTwoHex(frontChar, behindChar);
    k += 2;  // 2: means plus 2
    if ((bb & common::utf_helper::BIT_MASK_1) == 0) {
        resStr = base::StringHelper::Utf8ToU16String(&bb, 1);
#else
    uint8_t bb = common::utf_helper::GetValueFromTwoHex(frontChar, behindChar);
    k += 2;  // 2: means plus 2
    if ((bb & common::utf_helper::BIT_MASK_1) == 0) {
        resStr.push_back(bb);
#endif
    } else {
        // vii. Else the most significant bit in B is 1,
        //   1. Let n be the smallest nonnegative integer such that (B << n) & 0x80 is equal to 0.
        //   3. Let Octets be an array of 8-bit integers of size n.
        //   4. Put B into Octets at index 0.
        //   6. Let j be 1.
        //   7. Repeat, while j < n
        //     a. Increase k by 1.
        //     d. Let B be the 8-bit value represented by the two hexadecimal digits at
        //        index (k + 1) and (k + 2).
        //     f. Increase k by 2.
        //     g. Put B into Octets at index j.
        //     h. Increase j by 1.
        //   9. If V < 0x10000, then
        //     a. Let C be the code unit V.
        //     b. If C is not in reservedSet, then
        //        i. Let S be the String containing only the code unit C.
        //     c. Else C is in reservedSet,
        //        i. Let S be the substring of string from index start to index k inclusive.
        //   10. Else V ≥ 0x10000,
        //     a. Let L be (((V – 0x10000) & 0x3FF) + 0xDC00).
        //     b. Let H be ((((V – 0x10000) >> 10) & 0x3FF) + 0xD800).
        //     c. Let S be the String containing the two code units H and L.
        int32_t n = 0;
        while ((((static_cast<uint32_t>(bb) << static_cast<uint32_t>(n)) & common::utf_helper::BIT_MASK_1) != 0)) {
            n++;
            if (n > 4) { // 4 : 4 means less than 4
                break;
            }
        }
        // 2. If n equals 1 or n is greater than 4, throw a URIError exception.
        if ((n == 1) || (n > 4)) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }

        std::vector<uint8_t> oct = {bb};

        // 5. If k + (3 × (n – 1)) is greater than or equal to strLen, throw a URIError exception.
        if (k + (3 * (n - 1)) >= strLen) {  // 3: means multiply by 3
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        DecodePercentEncoding<T>(thread, n, k, str, bb, oct, sp, strLen);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        UTF16EncodeCodePoint(thread, oct, str, resStr);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return JSTaggedValue::True();
}

template <typename T>
JSTaggedValue RuntimeStubs::RuntimeDecodeURIComponent(JSThread *thread, const JSHandle<EcmaString> &string,
                                                      const T *data)
{
    // 1. Let strLen be the number of code units in string.
    CString errorMsg;
    auto stringAcc = EcmaStringAccessor(string);
    int32_t strLen = static_cast<int32_t>(stringAcc.GetLength());
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;
#if ENABLE_NEXT_OPTIMIZATION
    resStr.reserve(strLen);
#endif
    std::vector<T> tmpVec;
    tmpVec.resize(strLen);
    if (LIKELY(strLen != 0)) {
        if (memcpy_s(tmpVec.data(), sizeof(T) * strLen, data, sizeof(T) * strLen) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    Span<T> sp(tmpVec.data(), strLen);
    // 3. Let k be 0.
    // 4. Repeat
    int32_t k = 0;
    while (true) {
        if (k == strLen) {
            // a. If k equals strLen, return R.
            auto *uint16tData = reinterpret_cast<uint16_t *>(resStr.data());
            uint32_t resSize = resStr.size();
            return factory->NewFromUtf16Literal(uint16tData, resSize).GetTaggedValue();
        }

        // b. Let C be the code unit at index k within string.
        // c. If C is not "%", then
        //    i. Let S be the String containing only the code unit C.
        // d. Else C is "%",
        //   i. Let start be k.
        //   iv. Let B be the 8-bit value represented by the two hexadecimal digits at index (k + 1) and (k + 2).
        //   v. Increase k by 2.
        //   vi. If the most significant bit in B is 0, then
        //      1. Let C be the code unit with code unit value B.
        //      2. If C is not in reservedSet, then
        //         a. Let S be the String containing only the code unit C.
        //      3. Else C is in reservedSet,
        //         a. Let S be the substring of string from index start to index k inclusive.
        uint16_t cc = GetCodeUnit<T>(sp, k, strLen);
#if !ENABLE_NEXT_OPTIMIZATION
        std::u16string sStr;
        if (cc != '%') {
            if (cc == 0 && strLen == 1) {
                JSHandle<EcmaString> tmpEcmaString = factory->NewFromUtf16Literal(&cc, 1);
                return tmpEcmaString.GetTaggedValue();
            }
            sStr = base::StringHelper::Utf16ToU16String(&cc, 1);
        } else {
            DecodePercentEncoding<T>(thread, string, k, strLen, sStr, sp);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        resStr += sStr;
#else // ENABLE_NEXT_OPTIMIZATION
        if (cc != '%') {
            if (cc == 0 && strLen == 1) {
                JSHandle<EcmaString> tmpEcmaString = factory->NewFromUtf16Literal(&cc, 1);
                return tmpEcmaString.GetTaggedValue();
            }
            resStr.push_back(cc);
        } else {
            DecodePercentEncoding<T>(thread, string, k, strLen, resStr, sp);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
#endif // ENABLE_NEXT_OPTIMIZATION
        k++;
    }
}

#if !ENABLE_NEXT_OPTIMIZATION
uint8_t RuntimeStubs::GetValueFromTwoHex(uint8_t front, uint8_t behind)
{
    std::string hexString("0123456789ABCDEF");
    size_t idxf = base::StringHelper::FindFromU8ToUpper(hexString, &front);
    size_t idxb = base::StringHelper::FindFromU8ToUpper(hexString, &behind);

    uint8_t res = ((idxf << 4U) | idxb) & common::utf_helper::BIT_MASK_FF;  // NOLINT 4: means shift left by 4 digits
    return res;
}
#endif // ENABLE_NEXT_OPTIMIZATION
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_STUBS_RUNTIME_OPTIMIZED_STUBS_INL_H