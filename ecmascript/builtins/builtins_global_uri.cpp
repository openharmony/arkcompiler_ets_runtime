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

#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/utf_helper.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_global_uri.h"
#include "ecmascript/ecma_string-inl.h"

namespace panda::ecmascript::builtins {
using StringHelper = base::StringHelper;

#if ENABLE_NEXT_OPTIMIZATION
void BuiltinsGlobal::AppendPercentEncodedByte(std::u16string &sStr, uint8_t byte, uint8_t &len)
{
    sStr[++len] = base::utf_helper::GetHexChar16((byte >> 4) & BIT_MASK); // 4: high 4 bits
    sStr[++len] = base::utf_helper::GetHexChar16(byte & BIT_MASK);        // low 4 bits
    ++len;
}

void BuiltinsGlobal::AppendU32Data(std::u16string &resStr, uint32_t data)
{
    uint8_t len = 0;
    static std::u16string sStr(u"%00%00%00%00");
    if (data <= 0x7F) {          // 0x7F: 1 byte
        AppendPercentEncodedByte(sStr, data, len);
    } else if (data <= 0x7FF) {  // 0x7FF: 2 bytes
        AppendPercentEncodedByte(sStr, BIT_MASK_TWO + (data >> 6), len);                   // 6: high 5 bits
        AppendPercentEncodedByte(sStr, BIT_MASK_ONE + (data & SIX_BIT_MASK), len);         // low 6 bits
    } else if (data <= 0xFFFF) { // 0xFFFF: 3 bytes
        AppendPercentEncodedByte(sStr, BIT_MASK_THR + (data >> 12), len);                  // 12: highest 4 bits
        AppendPercentEncodedByte(sStr, BIT_MASK_ONE + ((data >> 6) & SIX_BIT_MASK), len);  // 6: middle 6 bits
        AppendPercentEncodedByte(sStr, BIT_MASK_ONE + (data & SIX_BIT_MASK), len);         // lowest 6 bits
    } else {                     // 4 bytes
        AppendPercentEncodedByte(sStr, BIT_MASK_FOR + (data >> 18), len);                  // 18: highest 3 bits
        AppendPercentEncodedByte(sStr, BIT_MASK_ONE + ((data >> 12) & SIX_BIT_MASK), len); // 12: higher 6 bits
        AppendPercentEncodedByte(sStr, BIT_MASK_ONE + ((data >> 6) & SIX_BIT_MASK), len);  // 6: lower 6 bits
        AppendPercentEncodedByte(sStr, BIT_MASK_ONE + (data & SIX_BIT_MASK), len);         // lowest 6 bits
    }
    resStr.append(sStr, 0, len);
}

template <typename T>
uint16_t BuiltinsGlobal::GetCodeUnit(Span<T> &sp, int32_t index, int32_t length)
{
    if ((index < 0) || (index >= length)) {
        return 0;
    }
    return sp[index];
}

// Runtime Semantics
JSTaggedValue BuiltinsGlobal::Encode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet)
{
    BUILTINS_API_TRACE(thread, Global, Encode);
    // 1. Let strLen be the number of code units in string.
    CString errorMsg;
    auto stringAcc = EcmaStringAccessor(str);
    uint32_t strLen = stringAcc.GetLength();
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;
    resStr.reserve(strLen);
    JSHandle<EcmaString> string;
    bool isTreeString = stringAcc.IsTreeString();
    if (isTreeString) {
        string = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), str));
        stringAcc = EcmaStringAccessor(string);
    }
    // 3. Let k be 0.
    // 4. Repeat
    uint32_t k = 0;
    while (true) {
        // a. If k equals strLen, return R.
        if (k == strLen) {
            auto *uint16tData = reinterpret_cast<uint16_t *>(resStr.data());
            uint32_t resSize = resStr.size();
            return factory->NewFromUtf16Literal(uint16tData, resSize).GetTaggedValue();
        }

        // b. Let C be the code unit at index k within string.
        // c. If C is in unescapedSet, then
        //   i. Let S be a String containing only the code unit C.
        //   ii. Let R be a new String value computed by concatenating the previous value of R and S.
        // d. Else C is not in unescapedSet,
        uint16_t cc = stringAcc.Get(k);
        if (LIKELY(IsInURISet(cc))) {
            resStr.push_back(static_cast<const char16_t>(cc));
        } else {
            // i. If the code unit value of C is not less than 0xDC00 and not greater than 0xDFFF,
            //    throw a URIError exception.
            if (cc >= base::utf_helper::DECODE_TRAIL_LOW && cc <= base::utf_helper::DECODE_TRAIL_HIGH) {
                JSTaggedValue strVal = isTreeString ? string.GetTaggedValue() : str.GetTaggedValue();
                errorMsg = "DecodeURI: invalid character: " + ConvertToString(strVal);
                THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
            }

            // ii. If the code unit value of C is less than 0xD800 or greater than 0xDBFF, then
            //    1. Let V be the code unit value of C.
            // iii. Else,
            //    1. Increase k by 1.
            //    2. If k equals strLen, throw a URIError exception.
            //    3. Let kChar be the code unit value of the code unit at index k within string.
            //    4. If kChar is less than 0xDC00 or greater than 0xDFFF, throw a URIError exception.
            //    5. Let V be UTF16Decode(C, kChar).
            uint32_t vv;
            if (cc < base::utf_helper::DECODE_LEAD_LOW || cc > base::utf_helper::DECODE_LEAD_HIGH) {
                vv = cc;
            } else {
                k++;
                if (k == strLen) {
                    JSTaggedValue strVal = isTreeString ? string.GetTaggedValue() : str.GetTaggedValue();
                    errorMsg = "DecodeURI: invalid character: " + ConvertToString(strVal);
                    THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
                }
                uint16_t kc = stringAcc.Get(k);
                if (kc < base::utf_helper::DECODE_TRAIL_LOW || kc > base::utf_helper::DECODE_TRAIL_HIGH) {
                    JSTaggedValue strVal = isTreeString ? string.GetTaggedValue() : str.GetTaggedValue();
                    errorMsg = "DecodeURI: invalid character: " + ConvertToString(strVal);
                    THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
                }
                vv = base::utf_helper::UTF16Decode(cc, kc);
            }

            // iv. Encode V and append it to resStr
            AppendU32Data(resStr, vv);
        }
        // e. Increase k by 1.
        k++;
    }
}

// Runtime Semantics
JSTaggedValue BuiltinsGlobal::Decode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet)
{
    BUILTINS_API_TRACE(thread, Global, Decode);
    JSHandle<EcmaString> string = str;
    if (EcmaStringAccessor(str).IsTreeString()) {
        string = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), str));
    }
    
    auto stringAcc = EcmaStringAccessor(string);
    JSTaggedValue result;
    if (stringAcc.IsLineString()) {
        // line string or flatten tree string
        if (!stringAcc.IsUtf16()) {
            result = DoDecode<uint8_t>(thread, string, IsInURISet, stringAcc.GetDataUtf8());
        } else {
            result = DoDecode<uint16_t>(thread, string, IsInURISet, stringAcc.GetDataUtf16());
        }
    } else {
        ASSERT(stringAcc.IsSlicedString());
        auto parent = SlicedString::Cast(string.GetTaggedValue())->GetParent();
        auto parentStrAcc = EcmaStringAccessor(parent);
        auto startIndex = SlicedString::Cast(string.GetTaggedValue())->GetStartIndex();
        if (parentStrAcc.IsLineString() && !parentStrAcc.IsUtf8()) {
            result = DoDecode<uint16_t>(thread, string, IsInURISet, parentStrAcc.GetDataUtf16() + startIndex);
        } else {
            result = DoDecode<uint8_t>(thread, string, IsInURISet, parentStrAcc.GetDataUtf8() + startIndex);
        }
    }
    return result;
}

template <typename T>
JSTaggedValue BuiltinsGlobal::DoDecode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet,
                                       const T *data)
{
    // 1. Let strLen be the number of code units in string.
    int32_t strLen = static_cast<int32_t>(EcmaStringAccessor(str).GetLength());
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;
    resStr.reserve(strLen);
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
        if (cc != '%') {
            if (cc == 0 && strLen == 1) {
                JSHandle<EcmaString> tmpEcmaString = factory->NewFromUtf16Literal(&cc, 1);
                return tmpEcmaString.GetTaggedValue();
            }
            resStr.push_back(static_cast<const char16_t>(cc));
        } else {
            DecodePercentEncoding<T>(thread, str, k, IsInURISet, strLen, resStr, sp);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        k++;
    }
}

void BuiltinsGlobal::HandleSingleByteCharacter(JSThread *thread, uint8_t &bb,
                                               const JSHandle<EcmaString> &str,
                                               uint32_t &start, int32_t &k,
                                               std::u16string &resStr, judgURIFunc IsInURISet)
{
    if (!IsInURISet(bb)) {
        resStr.push_back(static_cast<const char16_t>(bb));
    } else {
        auto substr = EcmaStringAccessor::FastSubString(
            thread->GetEcmaVM(), str, start, k - start + 1U);
        resStr.append(StringHelper::StringToU16string(
            EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION)));
    }
}

template <typename T>
JSTaggedValue BuiltinsGlobal::DecodePercentEncoding(JSThread *thread, const JSHandle<EcmaString> &str, int32_t &k,
                                                    judgURIFunc IsInURISet, int32_t strLen, std::u16string &resStr,
                                                    Span<T> &sp)
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
    if (!(base::utf_helper::IsHexDigits(frontChar) && base::utf_helper::IsHexDigits(behindChar))) {
        errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
    uint8_t bb = base::utf_helper::GetValueFromTwoHex(frontChar, behindChar);
    k += 2;  // 2: means plus 2
    if ((bb & BIT_MASK_ONE) == 0) {
        HandleSingleByteCharacter(thread, bb, str, start, k, resStr, IsInURISet);
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
        while ((((static_cast<uint32_t>(bb) << static_cast<uint32_t>(n)) & BIT_MASK_ONE) != 0)) {
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
        UTF16EncodeCodePoint(thread, IsInURISet, oct, str, start, k, resStr);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return JSTaggedValue::True();
}

template <typename T>
JSTaggedValue BuiltinsGlobal::DecodePercentEncoding(JSThread *thread, int32_t &n,
                                                    int32_t &k, const JSHandle<EcmaString> &str,
                                                    uint8_t &bb, std::vector<uint8_t> &oct, Span<T> &sp, int32_t strLen)
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
        if (!(base::utf_helper::IsHexDigits(frontChart) && base::utf_helper::IsHexDigits(behindChart))) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        bb = base::utf_helper::GetValueFromTwoHex(frontChart, behindChart);
        // e. If the two most significant bits in B are not 10, throw a URIError exception.
        if (!((bb & BIT_MASK_TWO) == BIT_MASK_ONE)) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        k += 2;  // 2: means plus 2
        oct.push_back(bb);
        j++;
    }
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsGlobal::UTF16EncodeCodePoint(JSThread *thread, judgURIFunc IsInURISet,
                                                   const std::vector<uint8_t> &oct, const JSHandle<EcmaString> &str,
                                                   uint32_t &start, int32_t &k, std::u16string &resStr)
{
    if (!base::utf_helper::IsValidUTF8(oct)) {
        CString errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
    uint32_t vv = StringHelper::Utf8ToU32String(oct);
    if (vv < base::utf_helper::DECODE_SECOND_FACTOR) {
        if (!IsInURISet(vv)) {
            resStr.append(StringHelper::Utf16ToU16String(reinterpret_cast<uint16_t *>(&vv), 1));
        } else {
            auto substr = EcmaStringAccessor::FastSubString(
                thread->GetEcmaVM(), str, start, static_cast<uint32_t>(k) - start + 1U);
            resStr.append(StringHelper::StringToU16string(
                EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION)));
        }
    } else {
        uint16_t lv = (((vv - base::utf_helper::DECODE_SECOND_FACTOR) & BIT16_MASK) +
            base::utf_helper::DECODE_TRAIL_LOW);
        uint16_t hv = ((((vv - base::utf_helper::DECODE_SECOND_FACTOR) >> 10U) & BIT16_MASK) +  // NOLINT
            base::utf_helper::DECODE_LEAD_LOW);  // 10: means shift left by 10 digits
            resStr.push_back(static_cast<const char16_t>(hv));
            resStr.push_back(static_cast<const char16_t>(lv));
    }
    return JSTaggedValue::True();
}
#endif // ENABLE_NEXT_OPTIMIZATION
}  // namespace panda::ecmascript::builtins