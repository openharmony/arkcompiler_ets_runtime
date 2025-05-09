/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_global.h"

#include "ecmascript/base/utf_helper.h"
#include "ecmascript/builtins/builtins_global_uri-inl.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/stubs/runtime_stubs.h"

namespace panda::ecmascript::builtins {
using NumberHelper = base::NumberHelper;
using StringHelper = base::StringHelper;
using GlobalError = containers::ContainerError;
// bitmap for "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_" + "@*+-./"
constexpr std::uint8_t ESCAPE_BIT_MAP[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
constexpr std::uint8_t ESCAPE_HEX_TO_CHAR[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};
constexpr std::uint8_t ESCAPE_CHAR_TO_HEX[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
constexpr std::uint8_t ESCAPE_HEX_MASK = 0xf;
constexpr std::uint8_t ESCAPE_HEX_BIT4 = 4;
constexpr std::uint8_t ESCAPE_HEX_BIT8 = 8;
constexpr std::uint8_t ESCAPE_HEX_BIT12 = 12;
constexpr std::uint8_t ESCAPE_CHAR_OFFSET2 = 2;
constexpr std::uint8_t ESCAPE_CHAR_OFFSET3 = 3;
constexpr std::uint8_t ESCAPE_CHAR_OFFSET4 = 4;
constexpr std::uint8_t ESCAPE_CHAR_OFFSET5 = 5;
constexpr std::uint16_t CHAR16_LETTER_NULL = u'\0';

// 18.2.1
JSTaggedValue BuiltinsGlobal::NotSupportEval(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, NotSupportEval);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    THROW_TYPE_ERROR_AND_RETURN(thread, "not support eval()", JSTaggedValue::Exception());
}

// 18.2.2
JSTaggedValue BuiltinsGlobal::IsFinite(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, IsFinite);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> numberInput = GetCallArg(msg, 0);
    // 1. Let num be ToNumber(number).
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, numberInput);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If num is NaN, +Infinite, or -Infinite, return false.
    // 4. Otherwise, return true.
    if (std::isfinite(number.GetNumber())) {
        return GetTaggedBoolean(true);
    }
    return GetTaggedBoolean(false);
}

// 18.2.3
JSTaggedValue BuiltinsGlobal::IsNaN(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, IsNaN);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> numberInput = GetCallArg(msg, 0);
    // 1. Let num be ToNumber(number).
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, numberInput);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If num is NaN, return true.
    if (std::isnan(number.GetNumber())) {
        return GetTaggedBoolean(true);
    }
    // 4. Otherwise, return false.
    return GetTaggedBoolean(false);
}

#if !ENABLE_NEXT_OPTIMIZATION
bool BuiltinsGlobal::IsReservedURI(uint16_t ch)
{
    std::u16string str(u";/?:@&=+$,");
    std::u16string::size_type index = str.find(ch);
    return (index != std::u16string::npos);
}

bool BuiltinsGlobal::IsInMarkURISet(uint16_t ch)
{
    std::u16string str(u"-_.!~*'()");
    std::u16string::size_type index = str.find(ch);
    return (index != std::u16string::npos);
}

bool BuiltinsGlobal::IsHexDigits(uint16_t ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}
#endif // ENABLE_NEXT_OPTIMIZATION

// 18.2.6
JSTaggedValue BuiltinsGlobal::DecodeURI(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, DecodeURI);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let uriString be ToString(encodedURI).
    // 2. ReturnIfAbrupt(uriString).
    JSHandle<EcmaString> uriString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let reservedURISet be a String containing one instance of each code unit valid in uriReserved plus "#".
    // 4. Return Decode(uriString, reservedURISet).
    return Decode(thread, uriString, IsInReservedURISet);
}

JSTaggedValue BuiltinsGlobal::EncodeURI(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, EncodeURI);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let uriString be ToString(uri).
    // 2. ReturnIfAbrupt(uriString).
    JSHandle<EcmaString> uriString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let unescapedURISet be a String containing one instance of
    //    each code unit valid in uriReserved and uriUnescaped plus "#".
    // 4. Return Encode(uriString, unescapedURISet).
    return Encode(thread, uriString, IsInUnescapedURISet);
}

JSTaggedValue BuiltinsGlobal::DecodeURIComponent(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, DecodeURIComponent);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let componentString be ToString(encodedURIComponent).
    // 2. ReturnIfAbrupt(componentString).
    JSHandle<EcmaString> componentString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let reservedURIComponentSet be the empty String.
    // 4. Return Decode(componentString, reservedURIComponentSet).
    return Decode(thread, componentString, []([[maybe_unused]] uint16_t unused) { return false; });
}

JSTaggedValue BuiltinsGlobal::EncodeURIComponent(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, EncodeURIComponent);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let componentString be ToString(uriComponent).
    // 2. ReturnIfAbrupt(componentString).
    JSHandle<EcmaString> componentString = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let unescapedURIComponentSet be a String containing one instance of each code unit valid in uriUnescaped.
    // 4. Return Encode(componentString, unescapedURIComponentSet).
    return Encode(thread, componentString, IsUnescapedURI);
}

#if !ENABLE_NEXT_OPTIMIZATION
// Runtime Semantics
JSTaggedValue BuiltinsGlobal::Encode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet)
{
    BUILTINS_API_TRACE(thread, Global, Encode);
    // 1. Let strLen be the number of code units in string.
    CString errorMsg;
    uint32_t strLen = EcmaStringAccessor(str).GetLength();
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;
    JSHandle<EcmaString> string = str;
    if (EcmaStringAccessor(str).IsTreeString()) {
        string = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), str));
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
        uint16_t cc = EcmaStringAccessor(string).Get(k);
        if (IsInURISet(cc)) {
            std::u16string sStr = StringHelper::Utf16ToU16String(&cc, 1);
            resStr.append(sStr);
        } else {
            // i. If the code unit value of C is not less than 0xDC00 and not greater than 0xDFFF,
            //    throw a URIError exception.
            if (cc >= base::utf_helper::DECODE_TRAIL_LOW && cc <= base::utf_helper::DECODE_TRAIL_HIGH) {
                errorMsg = "DecodeURI: invalid character: " + ConvertToString(string.GetTaggedValue());
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
                    errorMsg = "DecodeURI: invalid character: " + ConvertToString(string.GetTaggedValue());
                    THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
                }
                uint16_t kc = EcmaStringAccessor(string).Get(k);
                if (kc < base::utf_helper::DECODE_TRAIL_LOW || kc > base::utf_helper::DECODE_TRAIL_HIGH) {
                    errorMsg = "DecodeURI: invalid character: " + ConvertToString(string.GetTaggedValue());
                    THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
                }
                vv = base::utf_helper::UTF16Decode(cc, kc);
            }

            // iv. Let Octets be the array of octets resulting by applying the UTF-8 transformation to V,
            //     and let L be the array size.
            // v. Let j be 0.
            // vi. Repeat, while j < L
            //    1. Let jOctet be the value at index j within Octets.
            //    2. Let S be a String containing three code units "%XY" where XY are two uppercase hexadecimal
            //       digits encoding the value of jOctet.
            //    3. Let R be a new String value computed by concatenating the previous value of R and S.
            //    4. Increase j by 1.
            std::string oct = StringHelper::Utf32ToString(vv);
            std::string hexStr("0123456789ABCDEF");

            uint32_t length = oct.length();
            std::stringstream tmpStr;
            for (uint32_t j = 0; j < length; j++) {
                uint8_t joct = static_cast<uint8_t>(oct.at(j));
                tmpStr << '%' << hexStr.at((joct >> 4U) & BIT_MASK)  // NOLINT
                       << hexStr.at(joct & BIT_MASK);                // 4: means shift right by 4 digits
            }
            resStr.append(StringHelper::StringToU16string(tmpStr.str()));
        }

        // e. Increase k by 1.
        k++;
    }
}

uint8_t BuiltinsGlobal::GetValueFromTwoHex(uint16_t front, uint16_t behind)
{
    ASSERT(IsHexDigits(front) && IsHexDigits(behind));
    std::u16string hexString(u"0123456789ABCDEF");

    size_t idxf = StringHelper::FindFromU16ToUpper(hexString, &front);
    size_t idxb = StringHelper::FindFromU16ToUpper(hexString, &behind);
    uint8_t res = ((idxf << 4U) | idxb) & BIT_MASK_FF;  // NOLINT 4: means shift left by 4 digits
    return res;
}

uint16_t BuiltinsGlobal::GetValueFromHexString(const JSHandle<EcmaString> &string)
{
    uint32_t size = EcmaStringAccessor(string).GetLength();
    ASSERT(size > 0 && size <= 4); // NOLINT 4: means 4 hex digits
    std::u16string hexString(u"0123456789ABCDEF");

    uint16_t ret = 0;
    for (uint32_t i = 0; i < size; ++i) {
        uint16_t ch = EcmaStringAccessor(string).Get(i);
        size_t idx = StringHelper::FindFromU16ToUpper(hexString, &ch);
        ret = ((ret << 4U) | idx) & BIT_MASK_4F; // NOLINT 4: means shift left by 4
    }
    return ret;
}
#else // ENABLE_NEXT_OPTIMIZATION
uint16_t BuiltinsGlobal::GetValueFromHexString(const JSHandle<EcmaString> &string)
{
    auto stringAcc = EcmaStringAccessor(string);
    uint32_t size = stringAcc.GetLength();
    ASSERT(size > 0 && size <= 4); // NOLINT 4: means 4 hex digits

    uint16_t ret = 0;
    for (uint32_t i = 0; i < size; ++i) {
        uint16_t ch = stringAcc.Get(i);
        size_t val = base::utf_helper::HexChar16Value(ch);
        ret = ((ret << 4U) | val) & BIT_MASK_4F; // NOLINT 4: means shift left by 4
    }
    return ret;
}
#endif // ENABLE_NEXT_OPTIMIZATION

// 22.1.3.17.2 StringPad ( S, maxLength, fillString, placement )
EcmaString *BuiltinsGlobal::StringPad(JSThread *thread, const JSHandle<EcmaString> &source,
                                      uint32_t maxLength, const JSHandle<EcmaString> &fillString,
                                      Placement placement)
{
    // 1. Let stringLength be the length of S.
    uint32_t stringLength = EcmaStringAccessor(source).GetLength();
    // 2. If maxLength ≤ stringLength, return S.
    if (maxLength <= stringLength) {
        return *source;
    }
    // 3. If fillString is the empty String, return S.
    uint32_t targetStrLen = EcmaStringAccessor(fillString).GetLength();
    if (targetStrLen == 0) {
        return *source;
    }
    // 4. Let fillLen be maxLength - stringLength.
    uint32_t fillLen = maxLength - stringLength;
    EcmaVM *vm = thread->GetEcmaVM();
    //5. Let truncatedStringFiller be the String value consisting of repeated concatenations
    // of fillString truncated to length fillLen.
    uint32_t repeatTimes = std::ceil(fillLen / targetStrLen);
    EcmaString *p = nullptr;
    JSHandle<EcmaString> stringFiller = vm->GetFactory()->NewFromStdString(std::string("\0"));
    for (uint32_t k = 0; k < repeatTimes; ++k) {
        p = EcmaStringAccessor::Concat(vm, stringFiller, fillString);
        RETURN_VALUE_IF_ABRUPT(thread, *vm->GetFactory()->GetEmptyString());
        stringFiller = JSHandle<EcmaString>(thread, p);
    }
    JSHandle<EcmaString> truncatedStringFiller(thread,
        EcmaStringAccessor::FastSubString(vm, stringFiller, 0, fillLen));
    // 6. If placement is start, return the string-concatenation of truncatedStringFiller and S.
    // 7. Else, return the string-concatenation of S and truncatedStringFiller.
    if (placement == Placement::START) {
        return EcmaStringAccessor::Concat(vm, truncatedStringFiller, source);
    } else {
        return EcmaStringAccessor::Concat(vm, source, truncatedStringFiller);
    }
}

// Static Semantics: UTF16SurrogatePairToCodePoint ( lead, trail )
uint16_t BuiltinsGlobal::UTF16SurrogatePairToCodePoint(uint16_t lead, uint16_t trail)
{
    // 1. Assert: lead is a leading surrogate and trail is a trailing surrogate.
    ASSERT(IsUTF16HighSurrogate(lead) && IsUTF16LowSurrogate(trail));
    // 2. Let cp be (lead - 0xD800) × 0x400 + (trail - 0xDC00) + 0x10000.
    uint16_t cp = ((lead - 0xD800) << 10UL) + (trail - 0xDC00) + 0x10000;
    // 3. Return the code point cp.
    return cp;
}

// 11.1.5 Static Semantics: StringToCodePoints ( string )
EcmaString *BuiltinsGlobal::StringToCodePoints(JSThread *thread, const JSHandle<EcmaString> &string)
{
    // 1. Let codePoints be a new empty List.
    std::u16string codePoints;
    // 2. Let size be the length of string.
    uint32_t size = EcmaStringAccessor(string).GetLength();
    // 3. Let position be 0.
    uint32_t position = 0;
    // 4. Repeat, while position < size,
    //    a. Let cp be CodePointAt(string, position).
    //    b. Append cp.[[CodePoint]] to codePoints.
    //    c. Set position to position + cp.[[CodeUnitCount]].
    while (position < size) {
        // i.Let first be the code unit at index position within string.
        uint16_t first = EcmaStringAccessor(string).Get(position);
        uint16_t cp = first - CHAR16_LETTER_NULL;
        uint8_t codeUnitCount = 0;
        bool isUnpairedSurrogate = false;
        // ii. If first is neither a leading surrogate nor a trailing surrogate, then
        //   a. Return the Record { [[CodePoint]]: cp, [[CodeUnitCount]]: 1, [[IsUnpairedSurrogate]]: false }.
        if (!IsUTF16HighSurrogate(first) && !IsUTF16LowSurrogate(first)) {
            codeUnitCount = 1; // 1 means: code unit count
            isUnpairedSurrogate = false;
        } else if (IsUTF16HighSurrogate(first) || position + 1 == size) {
            // iii. If first is a trailing surrogate or position + 1 = size, then
            //   a. Return the Record { [[CodePoint]]: cp, [[CodeUnitCount]]: 1, [[IsUnpairedSurrogate]]: true }.
            codeUnitCount = 1;
            isUnpairedSurrogate = true;
        } else {
            // iv. Let second be the code unit at index position + 1 within string.
            uint16_t second = EcmaStringAccessor(string).Get(position + 1);
            // v. If second is not a trailing surrogate, then
            //   a. Return the Record { [[CodePoint]]: cp, [[CodeUnitCount]]: 1, [[IsUnpairedSurrogate]]: true }.
            if (!IsUTF16LowSurrogate(second)) {
                codeUnitCount = 1; // 1 means: code unit count
                isUnpairedSurrogate = true;
            } else {
            // vi. Set cp to UTF16SurrogatePairToCodePoint(first, second).
            // vii. Return the Record { [[CodePoint]]: cp, [[CodeUnitCount]]: 2, [[IsUnpairedSurrogate]]: false }.
                cp = UTF16SurrogatePairToCodePoint(first, second);
                codeUnitCount = 2; // 2 means: code unit count
                isUnpairedSurrogate = false;
            }
        }
        codePoints.push_back(cp);
        position = position + codeUnitCount;
    }
    // 5. Return codePoints.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint16_t *ptr = reinterpret_cast<uint16_t *>(codePoints.data());
    JSHandle<EcmaString> codePointsString = factory->NewFromUtf16Literal(ptr, codePoints.size());
    return *codePointsString;
}

#if !ENABLE_NEXT_OPTIMIZATION
// Runtime Semantics
JSTaggedValue BuiltinsGlobal::Decode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet)
{
    BUILTINS_API_TRACE(thread, Global, Decode);
    // 1. Let strLen be the number of code units in string.
    int32_t strLen = static_cast<int32_t>(EcmaStringAccessor(str).GetLength());
    // 2. Let R be the empty String.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string resStr;
    JSHandle<EcmaString> string = str;
    if (EcmaStringAccessor(str).IsTreeString()) {
        string = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), str));
    }

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
        uint16_t cc = EcmaStringAccessor(string).Get(k);
        std::u16string sStr;
        if (cc != '%') {
            if (cc == 0 && strLen == 1) {
                JSHandle<EcmaString> tmpEcmaString = factory->NewFromUtf16Literal(&cc, 1);
                return tmpEcmaString.GetTaggedValue();
            }
            sStr = StringHelper::Utf16ToU16String(&cc, 1);
        } else {
            DecodePercentEncoding(thread, string, k, IsInURISet, strLen, sStr);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        resStr.append(sStr);
        k++;
    }
}

void BuiltinsGlobal::HandleSingleByteCharacter(JSThread *thread, uint8_t &bb,
                                               const JSHandle<EcmaString> &str,
                                               uint32_t &start, int32_t &k,
                                               std::u16string &sStr, judgURIFunc IsInURISet)
{
    if (!IsInURISet(bb)) {
        sStr = StringHelper::Utf8ToU16String(&bb, 1);
    } else {
        auto substr = EcmaStringAccessor::FastSubString(
            thread->GetEcmaVM(), str, start, k - start + 1U);
        sStr = StringHelper::StringToU16string(
            EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION));
    }
}

JSTaggedValue BuiltinsGlobal::DecodePercentEncoding(JSThread *thread, const JSHandle<EcmaString> &str, int32_t &k,
                                                    judgURIFunc IsInURISet, int32_t strLen, std::u16string &sStr)
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
    uint16_t frontChar = EcmaStringAccessor(str).Get(k + 1);
    uint16_t behindChar = EcmaStringAccessor(str).Get(k + 2);  // 2: means plus 2
    if (!(IsHexDigits(frontChar) && IsHexDigits(behindChar))) {
        errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
    uint8_t bb = GetValueFromTwoHex(frontChar, behindChar);
    k += 2;  // 2: means plus 2
    if ((bb & BIT_MASK_ONE) == 0) {
        HandleSingleByteCharacter(thread, bb, str, start, k, sStr, IsInURISet);
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
        DecodePercentEncoding(thread, n, k, str, bb, oct);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        UTF16EncodeCodePoint(thread, IsInURISet, oct, str, start, k, sStr);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsGlobal::DecodePercentEncoding(JSThread *thread, int32_t &n,
                                                    int32_t &k, const JSHandle<EcmaString> &str,
                                                    uint8_t &bb, std::vector<uint8_t> &oct)
{
    CString errorMsg;
    int32_t j = 1;
    while (j < n) {
        k++;
        uint16_t codeUnit = EcmaStringAccessor(str).Get(k);
        // b. If the code unit at index k within string is not "%", throw a URIError exception.
        // c. If the code units at index (k +1) and (k + 2) within string do not represent hexadecimal
        //    digits, throw a URIError exception.
        if (!(codeUnit == '%')) {
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        if (!(IsHexDigits(EcmaStringAccessor(str).Get(k + 1)) &&
                IsHexDigits(EcmaStringAccessor(str).Get(k + 2)))) {  // 2: means plus 2
            errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
            THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
        }
        uint16_t frontChart = EcmaStringAccessor(str).Get(k + 1);
        uint16_t behindChart = EcmaStringAccessor(str).Get(k + 2);  // 2: means plus 2
        bb = GetValueFromTwoHex(frontChart, behindChart);
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
                                                   uint32_t &start, int32_t &k, std::u16string &sStr)
{
    if (!base::utf_helper::IsValidUTF8(oct)) {
        CString errorMsg = "DecodeURI: invalid character: " + ConvertToString(str.GetTaggedValue());
        THROW_URI_ERROR_AND_RETURN(thread, errorMsg.c_str(), JSTaggedValue::Exception());
    }
    uint32_t vv = StringHelper::Utf8ToU32String(oct);
    if (vv < base::utf_helper::DECODE_SECOND_FACTOR) {
        if (!IsInURISet(vv)) {
            sStr = StringHelper::Utf16ToU16String(reinterpret_cast<uint16_t *>(&vv), 1);
        } else {
            auto substr = EcmaStringAccessor::FastSubString(
                thread->GetEcmaVM(), str, start, static_cast<uint32_t>(k) - start + 1U);
            sStr = StringHelper::StringToU16string(
                EcmaStringAccessor(substr).ToStdString(StringConvertedUsage::LOGICOPERATION));
        }
    } else {
        uint16_t lv = (((vv - base::utf_helper::DECODE_SECOND_FACTOR) & BIT16_MASK) +
            base::utf_helper::DECODE_TRAIL_LOW);
        uint16_t hv = ((((vv - base::utf_helper::DECODE_SECOND_FACTOR) >> 10U) & BIT16_MASK) +  // NOLINT
            base::utf_helper::DECODE_LEAD_LOW);  // 10: means shift left by 10 digits
        sStr = StringHelper::Append(StringHelper::Utf16ToU16String(&hv, 1),
                                    StringHelper::Utf16ToU16String(&lv, 1));
    }
    return JSTaggedValue::True();
}
#endif // ENABLE_NEXT_OPTIMIZATION

void BuiltinsGlobal::PrintString([[maybe_unused]] JSThread *thread, EcmaString *string)
{
    if (string == nullptr) {
        return;
    }
    BUILTINS_API_TRACE(thread, Global, PrintString);
    CString buffer = ConvertToString(string);
    std::cout << buffer;
}

JSTaggedValue BuiltinsGlobal::PrintEntrypoint(EcmaRuntimeCallInfo *msg)
{
    if (msg == nullptr) {
        return JSTaggedValue::Undefined();
    }
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    BUILTINS_API_TRACE(thread, Global, PrintEntryPoint);

    uint32_t numArgs = msg->GetArgsNumber();
    for (uint32_t i = 0; i < numArgs; i++) {
        JSHandle<EcmaString> stringContent = JSTaggedValue::ToString(thread, GetCallArg(msg, i));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        PrintString(thread, *stringContent);

        if (i != numArgs - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGlobal::MarkModuleCollectable(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t numArgs = msg->GetArgsNumber();
    if (numArgs != 1) {
        LOG_FULL(ERROR) << "The number of parameters received by markModuleCollectable is incorrect.";
        return JSTaggedValue::False();
    }
    JSHandle<JSTaggedValue> module = GetCallArg(msg, 0);
    if (!module->IsModuleNamespace()) {
        return JSTaggedValue::False();
    }

    ModuleDeregister::ProcessModuleReference(thread, module);
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsGlobal::LoadNativeModule(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CString errorMsg;
    uint32_t numArgs = msg->GetArgsNumber();
    if (numArgs != 1) {
        errorMsg = "The number of parameters received by loadNativeModule is incorrect.";
        auto error = GlobalError::ParamError(thread, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> input = GetCallArg(msg, 0);
    if (!input->IsString()) {
        errorMsg = "The number of parameters received by loadNativeModule is incorrect.";
        auto error = GlobalError::ParamError(thread, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    
    EcmaVM *vm  = thread->GetEcmaVM();
    auto [moduleName, fileName] = vm->GetCurrentModuleInfo(false);
    std::shared_ptr<JSPandaFile> curJsPandaFile;
    CString requestPath = ModulePathHelper::Utf8ConvertToString(input.GetTaggedValue());
    CString abcFilePath = fileName.c_str();
    if (moduleName.size() != 0) {
        curJsPandaFile = JSPandaFileManager::GetInstance()->LoadJSPandaFile(
            thread, abcFilePath, requestPath, false, ExecuteTypes::STATIC);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (curJsPandaFile == nullptr) {
            errorMsg = "Load native module failed, filename '" + abcFilePath +
                ", module name '" + requestPath;
            auto error = GlobalError::ReferenceError(thread, errorMsg.c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        if (vm->IsNormalizedOhmUrlPack()) {
            ModulePathHelper::TranslateExpressionToNormalized(thread, curJsPandaFile.get(), abcFilePath, "",
                requestPath);
        } else if (ModulePathHelper::NeedTranstale(requestPath)) {
            ModulePathHelper::TranstaleExpressionInput(curJsPandaFile.get(), requestPath);
        }

        size_t pos = requestPath.find(PathHelper::COLON_TAG);
        if (pos == CString::npos) {
            errorMsg = "The module name '"+ requestPath +
                "' of parameters received by loadNativeModule is incorrect.";
            auto error = GlobalError::ParamError(thread, errorMsg.c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    ModuleManager *moduleManager = thread->GetModuleManager();
    auto exportObject = moduleManager->ExecuteNativeModuleMayThrowError(thread, requestPath);
    return exportObject.GetTaggedValue();
}

JSTaggedValue BuiltinsGlobal::CallJsBoundFunction(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, CallJsBoundFunction);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // msg contains jsfunc, this, arg1,...

    JSHandle<JSBoundFunction> boundFunc(GetConstructor(msg));
    JSHandle<JSTaggedValue> thisObj(thread, boundFunc->GetBoundThis());
    msg->SetThis(thisObj.GetTaggedValue());
    return RuntimeStubs::CallBoundFunction(msg);
}

JSTaggedValue BuiltinsGlobal::CallJsProxy(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, CallJsProxy);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // msg contains js_proxy, this, arg1,...
    JSHandle<JSProxy> proxy(GetConstructor(msg));
    if (!proxy->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Proxy target is not callable", JSTaggedValue::Undefined());
    }

    // Calling proxy directly should transfer 'undefined' as this
    return JSProxy::CallInternal(msg);
}

#if ECMASCRIPT_ENABLE_RUNTIME_STAT
JSTaggedValue BuiltinsGlobal::StartRuntimeStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, StartRuntimeStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetEcmaVM()->SetRuntimeStatEnable(true);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsGlobal::StopRuntimeStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, StopRuntimeStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // stop vm runtime stat statistic
    thread->GetEcmaVM()->SetRuntimeStatEnable(false);
    return JSTaggedValue::Undefined();
}
#endif

#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
JSTaggedValue BuiltinsGlobal::PrintOptStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, PrintOptStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetCurrentEcmaContext()->PrintOptStat();
    return JSTaggedValue::Undefined();
}
#endif

#if ECMASCRIPT_ENABLE_MEGA_PROFILER
JSTaggedValue BuiltinsGlobal::PrintMegaICStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, PrintMegaICStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetCurrentEcmaContext()->PrintMegaICStat();
    return JSTaggedValue::Undefined();
}
#endif

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
JSTaggedValue BuiltinsGlobal::PrintFunctionCallStat(EcmaRuntimeCallInfo *msg)
{
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, PrintFunctionCallStat);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // start vm runtime stat statistic
    thread->GetEcmaVM()->DumpCallTimeInfo();
    return JSTaggedValue::Undefined();
}
#endif

// B.2.1.1 escape ( string )
JSTaggedValue BuiltinsGlobal::Escape(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, Escape);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Set string to ? ToString(string).
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. Let len be the length of string.
    uint32_t len = EcmaStringAccessor(string).GetLength();
    // 3. Let R be the empty String.
    std::u16string r;
    // 4. Let unescapedSet be the string-concatenation of the ASCII word characters and "@*+-./".
    // 5. Let k be 0.
    uint32_t k = 0;
    // 6. Repeat, while k < len,
    //   a. Let C be the code unit at index k within string.
    //   b. If unescapedSet contains C, then
    //        i. Let S be C.
    //   c. Else,
    //        i. Let n be the numeric value of C.
    //        ii. If n < 256, then
    //          1. Let hex be the String representation of n, formatted as an uppercase hexadecimal number.
    //          2. Let S be the string-concatenation of "%" and StringPad(hex, 2, "0", start).
    //        iii. Else,
    //          1. Let hex be the String representation of n, formatted as an uppercase hexadecimal number.
    //          2. Let S be the string-concatenation of "%u" and StringPad(hex, 4, "0", start).
    //    d. Set R to the string-concatenation of R and S.
    //    e. Set k to k + 1.
    while (k < len) {
        uint16_t c = EcmaStringAccessor(string).Get(k);
        if (c < std::numeric_limits<int8_t>::max() && ESCAPE_BIT_MAP[c] == 1) {
            r.push_back(c);
        } else {
            r.push_back('%');
            if (c <= std::numeric_limits<uint8_t>::max()) {
                r.push_back(ESCAPE_HEX_TO_CHAR[(c >> ESCAPE_HEX_BIT4) & ESCAPE_HEX_MASK]);
                r.push_back(ESCAPE_HEX_TO_CHAR[c & ESCAPE_HEX_MASK]);
            } else {
                r.push_back('u');
                r.push_back(ESCAPE_HEX_TO_CHAR[(c >> ESCAPE_HEX_BIT12) & ESCAPE_HEX_MASK]);
                r.push_back(ESCAPE_HEX_TO_CHAR[(c >> ESCAPE_HEX_BIT8) & ESCAPE_HEX_MASK]);
                r.push_back(ESCAPE_HEX_TO_CHAR[(c >> ESCAPE_HEX_BIT4) & ESCAPE_HEX_MASK]);
                r.push_back(ESCAPE_HEX_TO_CHAR[c & ESCAPE_HEX_MASK]);
            }
        }
        ++k;
    }
    // 7. Return R.
    auto *returnData = reinterpret_cast<uint16_t *>(r.data());
    uint32_t retSize = r.size();
    return factory->NewFromUtf16Literal(returnData, retSize).GetTaggedValue();
}

// B.2.1.2 unescape ( string )
JSTaggedValue BuiltinsGlobal::Unescape(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, Unescape);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Set string to ? ToString(string).
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, GetCallArg(msg, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let len be the length of string.
    uint32_t len = EcmaStringAccessor(string).GetLength();
    // 3. Let R be the empty String.
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    std::u16string r;
    // 4. Let k be 0.
    uint32_t k = 0;
    // 5. Repeat, while k < len,
    //   a. Let C be the code unit at index k within string.
    //   b. If C is the code unit 0x0025 (PERCENT SIGN), then
    //     i. Let hexDigits be the empty String.
    //     ii. Let optionalAdvance be 0.
    //     iii. If k + 5 < len and the code unit at index k + 1 within string is the code unit
    //          0x0075 (LATIN SMALL LETTER U), then
    //       1. Set hexDigits to the substring of string from k + 2 to k + 6.
    //       2. Set optionalAdvance to 5.
    //     iv. Else if k + 3 ≤ len, then
    //       1. Set hexDigits to the substring of string from k + 1 to k + 3.
    //       2. Set optionalAdvance to 2.
    //     v. Let parseResult be ParseText(StringToCodePoints(hexDigits), HexDigits[~Sep]).
    //     vi. If parseResult is a Parse Node, then
    //       1. Let n be the MV of parseResult.
    //       2. Set C to the code unit whose numeric value is n.
    //       3. Set k to k + optionalAdvance.
    //   c. Set R to the string-concatenation of R and C.
    //   d. Set k to k + 1.
    while (k < len) {
        uint16_t c = EcmaStringAccessor(string).Get(k);
        if (c == '%') {
            uint16_t c1 = EcmaStringAccessor(string).Get(k + 1);
            if (k + ESCAPE_CHAR_OFFSET5 < len && c1 == 'u') {
                uint16_t c2 = EcmaStringAccessor(string).Get(k + ESCAPE_CHAR_OFFSET2);
                uint16_t c3 = EcmaStringAccessor(string).Get(k + ESCAPE_CHAR_OFFSET3);
                uint16_t c4 = EcmaStringAccessor(string).Get(k + ESCAPE_CHAR_OFFSET4);
                uint16_t c5 = EcmaStringAccessor(string).Get(k + ESCAPE_CHAR_OFFSET5);
#if !ENABLE_NEXT_OPTIMIZATION
                bool c2IsHexDigits = IsHexDigits(c2);
                bool c3IsHexDigits = IsHexDigits(c3);
                bool c4IsHexDigits = IsHexDigits(c4);
                bool c5IsHexDigits = IsHexDigits(c5);
#else // ENABLE_NEXT_OPTIMIZATION
                bool c2IsHexDigits = base::utf_helper::IsHexDigits(c2);
                bool c3IsHexDigits = base::utf_helper::IsHexDigits(c3);
                bool c4IsHexDigits = base::utf_helper::IsHexDigits(c4);
                bool c5IsHexDigits = base::utf_helper::IsHexDigits(c5);
#endif // ENABLE_NEXT_OPTIMIZATION
                bool isHexDigits = c2IsHexDigits && c3IsHexDigits && c4IsHexDigits && c5IsHexDigits;
                if (isHexDigits) {
                    c = ESCAPE_CHAR_TO_HEX[c2];
                    c = (c << ESCAPE_HEX_BIT4) | ESCAPE_CHAR_TO_HEX[c3];
                    c = (c << ESCAPE_HEX_BIT4) | ESCAPE_CHAR_TO_HEX[c4];
                    c = (c << ESCAPE_HEX_BIT4) | ESCAPE_CHAR_TO_HEX[c5];
                    k = k + ESCAPE_CHAR_OFFSET5;
                }
            } else if (k + ESCAPE_CHAR_OFFSET3 <= len) {
                uint16_t c2 = EcmaStringAccessor(string).Get(k + ESCAPE_CHAR_OFFSET2);
#if !ENABLE_NEXT_OPTIMIZATION
                bool c1IsHexDigits = IsHexDigits(c1);
                bool c2IsHexDigits = IsHexDigits(c2);
#else // ENABLE_NEXT_OPTIMIZATION
                bool c1IsHexDigits = base::utf_helper::IsHexDigits(c1);
                bool c2IsHexDigits = base::utf_helper::IsHexDigits(c2);
#endif // ENABLE_NEXT_OPTIMIZATION
                bool isHexDigits = c1IsHexDigits && c2IsHexDigits;
                if (isHexDigits) {
                    c = ESCAPE_CHAR_TO_HEX[c1];
                    c = (c << ESCAPE_HEX_BIT4) | ESCAPE_CHAR_TO_HEX[c2];
                    k = k + ESCAPE_CHAR_OFFSET2;
                }
            }
        }
        r.push_back(c);
        ++k;
    }
    // 7. Return R.
    auto *returnData = reinterpret_cast<uint16_t *>(r.data());
    uint32_t retSize = r.size();
    return factory->NewFromUtf16Literal(returnData, retSize).GetTaggedValue();
}

JSTaggedValue BuiltinsGlobal::GetCurrentModuleName(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, GetCurrentModuleName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    std::pair<CString, CString> moduleInfo = EcmaInterpreter::GetCurrentEntryPoint(thread);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    CString fileName = moduleInfo.second;
    CString moduleName = ModulePathHelper::GetModuleNameWithBaseFile(fileName);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> result = factory->NewFromUtf8(moduleName.c_str());
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsGlobal::GetCurrentBundleName(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, Global, GetCurrentBundleName);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    std::pair<CString, CString> moduleInfo = EcmaInterpreter::GetCurrentEntryPoint(thread);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaVM *vm = thread->GetEcmaVM();
    CString recordName = moduleInfo.first;
    CString bundleName = ModulePathHelper::GetBundleNameWithRecordName(vm, recordName);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> result = factory->NewFromUtf8(bundleName.c_str());
    return result.GetTaggedValue();
}

JSTaggedValue BuiltinsGlobal::IsSendable(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t numArgs = msg->GetArgsNumber();
    if (numArgs != 1) {
        LOG_FULL(ERROR) << "The number of parameters received by IsSendable is incorrect.";
        return JSTaggedValue::False();
    }
    JSHandle<JSTaggedValue> obj = GetCallArg(msg, 0);
    if ((obj->IsECMAObject() && obj->IsJSShared()) ||
        obj->IsString() || obj->IsNumber() || obj->IsBoolean() ||
        obj->IsUndefined() || obj->IsNull() || obj->IsBigInt()) {
        return JSTaggedValue::True();
    }

    return JSTaggedValue::False();
}
}  // namespace panda::ecmascript::builtins
