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

#include "ecmascript/builtins/builtins_string.h"


#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/property_detector-inl.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_collator.h"
#include "ecmascript/js_locale.h"
#else
#ifndef ARK_NOT_SUPPORT_INTL_GLOBAL
#include "ecmascript/intl/global_intl_helper.h"
#endif
#endif

#include "unicode/normlzr.h"

namespace panda::ecmascript::builtins {
using ObjectFactory = ecmascript::ObjectFactory;
using JSArray = ecmascript::JSArray;
constexpr std::uint16_t CHAR16_LETTER_NULL = u'\0';

// 21.1.1.1 String(value)
JSTaggedValue BuiltinsString::StringConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (argv->GetArgsNumber() > 0) {
        JSHandle<JSTaggedValue> valTagNew = GetCallArg(argv, 0);
        if (newTarget->IsUndefined() && valTagNew->IsSymbol()) {
            return BuiltinsSymbol::SymbolDescriptiveString(thread, valTagNew.GetTaggedValue());
        }
        JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, valTagNew);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (newTarget->IsUndefined()) {
            return str.GetTaggedValue();
        }
        JSHandle<JSTaggedValue> strTag(str);
        return JSPrimitiveRef::StringCreate(thread, strTag, newTarget).GetTaggedValue();
    }
    JSHandle<EcmaString> val = factory->GetEmptyString();
    JSHandle<JSTaggedValue> valTag(val);
    if (newTarget->IsUndefined()) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    return JSPrimitiveRef::StringCreate(thread, valTag, newTarget).GetTaggedValue();
}

// 21.1.2.1
JSTaggedValue BuiltinsString::FromCharCode(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, FromCharCode);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t argLength = argv->GetArgsNumber();
    if (argLength == 0) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    if (argLength == 1) {
        JSHandle<JSTaggedValue> codePointTag = BuiltinsString::GetCallArg(argv, 0);
        uint16_t codePointValue = JSTaggedValue::ToUint16(thread, codePointTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (EcmaStringAccessor::CanBeCompressed(&codePointValue, 1)) {
            JSHandle<SingleCharTable> singleCharTable(thread, thread->GetSingleCharTable());
            return singleCharTable->GetStringFromSingleCharTable(codePointValue);
        }
        JSHandle<EcmaString> strHandle = factory->NewFromUtf16Literal(&codePointValue, 1);
        return strHandle.GetTaggedValue();
    }
    CVector<uint16_t> valueTable;
    valueTable.reserve(argLength);
    for (uint32_t i = 0; i < argLength; i++) {
        JSHandle<JSTaggedValue> nextCp = BuiltinsString::GetCallArg(argv, i);
        uint16_t nextCv = JSTaggedValue::ToUint16(thread, nextCp);
        valueTable.emplace_back(nextCv);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return factory->NewFromUtf16Literal(valueTable.data(), valueTable.size()).GetTaggedValue();
}

// 21.1.2.2
JSTaggedValue BuiltinsString::FromCodePoint(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, FromCodePoint);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t argLength = argv->GetArgsNumber();
    if (argLength == 0) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    std::u16string u16str;
    uint32_t u16strSize = argLength;
    for (uint32_t i = 0; i < argLength; i++) {
        JSHandle<JSTaggedValue> nextCpTag = BuiltinsString::GetCallArg(argv, i);
        JSTaggedNumber nextCpVal = JSTaggedValue::ToNumber(thread, nextCpTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!nextCpVal.IsInteger()) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "is not integer", JSTaggedValue::Exception());
        }
        int32_t cp = nextCpVal.ToInt32();
        if (cp < 0 || cp > ENCODE_MAX_UTF16) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "CodePoint < 0 or CodePoint > 0x10FFFF", JSTaggedValue::Exception());
        }
        if (cp > UINT16_MAX) {
            uint16_t cu1 =
                std::floor((static_cast<uint32_t>(cp) - ENCODE_SECOND_FACTOR) / ENCODE_FIRST_FACTOR) + ENCODE_LEAD_LOW;
            uint16_t cu2 =
                ((static_cast<uint32_t>(cp) - ENCODE_SECOND_FACTOR) % ENCODE_FIRST_FACTOR) + ENCODE_TRAIL_LOW;
            std::u16string nextU16str1 = base::StringHelper::Utf16ToU16String(&cu1, 1);
            std::u16string nextU16str2 = base::StringHelper::Utf16ToU16String(&cu2, 1);
            base::StringHelper::InplaceAppend(u16str, nextU16str1);
            base::StringHelper::InplaceAppend(u16str, nextU16str2);
            u16strSize++;
        } else {
            auto u16tCp = static_cast<uint16_t>(cp);
            std::u16string nextU16str = base::StringHelper::Utf16ToU16String(&u16tCp, 1);
            base::StringHelper::InplaceAppend(u16str, nextU16str);
        }
    }
    const char16_t *constChar16tData = u16str.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
}

// 21.1.2.4
JSTaggedValue BuiltinsString::Raw(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Raw);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Let cooked be ToObject(template).
    JSHandle<JSObject> cooked = JSTaggedValue::ToObject(thread, BuiltinsString::GetCallArg(argv, 0));
    // ReturnIfAbrupt(cooked).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let raw be ToObject(Get(cooked, "raw")).
    JSHandle<JSTaggedValue> rawKey(factory->NewFromASCII("raw"));
    JSHandle<JSTaggedValue> rawTag =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(cooked), rawKey).GetValue();
    JSHandle<JSObject> rawObj = JSTaggedValue::ToObject(thread, rawTag);
    // ReturnIfAbrupt(rawObj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> rawLen =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(rawObj), lengthKey).GetValue();
    // ReturnIfAbrupt(rawLen).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedNumber lengthNumber = JSTaggedValue::ToLength(thread, rawLen);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int length = static_cast<int>(lengthNumber.ToUint32());
    if (length <= 0) {
        return factory->GetEmptyString().GetTaggedValue();
    }

    std::u16string u16str;
    uint32_t argc = argv->GetArgsNumber() - 1;
    bool canBeCompress = true;
    for (uint32_t i = 0, argsI = 1; i < static_cast<uint32_t>(length); ++i, ++argsI) {
        // Let nextSeg be ToString(Get(raw, nextKey)).
        JSHandle<JSTaggedValue> elementString =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(rawObj), i).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaString *nextSeg = *JSTaggedValue::ToString(thread, elementString);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        u16str += EcmaStringAccessor(nextSeg).ToU16String();
        if (EcmaStringAccessor(nextSeg).IsUtf16()) {
            canBeCompress = false;
        }
        if (i + 1 == static_cast<uint32_t>(length)) {
            break;
        }
        if (argsI <= argc) {
            EcmaString *nextSub = *JSTaggedValue::ToString(thread, GetCallArg(argv, argsI));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            u16str += EcmaStringAccessor(nextSub).ToU16String();
            if (EcmaStringAccessor(nextSub).IsUtf16()) {
                canBeCompress = false;
            }
        }
    }
    // return the result string
    auto *uint16tData = reinterpret_cast<uint16_t *>(const_cast<char16_t *>(u16str.data()));
    return canBeCompress ? factory->NewFromUtf16LiteralCompress(uint16tData, u16str.size()).GetTaggedValue() :
                           factory->NewFromUtf16LiteralNotCompress(uint16tData, u16str.size()).GetTaggedValue();
}

// 21.1.3.1
JSTaggedValue BuiltinsString::CharAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, CharAt);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisFlat(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), thisHandle));
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisFlat).GetLength());
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 0);
    int32_t pos = 0;
    if (posTag->IsInt()) {
        pos = posTag->GetInt();
    } else if (posTag->IsUndefined()) {
        pos = 0;
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double valueNumber = posVal.GetNumber();
        if (!std::isfinite(valueNumber)) {
            return factory->GetEmptyString().GetTaggedValue();
        }
        pos = posVal.ToInt32();
    }
    if (pos < 0 || pos >= thisLen) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    uint16_t res = EcmaStringAccessor(thisFlat).Get<false>(pos);
    if (EcmaStringAccessor::CanBeCompressed(&res, 1)) {
        JSHandle<SingleCharTable> singleCharTable(thread, thread->GetSingleCharTable());
        return singleCharTable->GetStringFromSingleCharTable(res);
    }
    return factory->NewFromUtf16Literal(&res, 1).GetTaggedValue();
}

// 21.1.3.2
JSTaggedValue BuiltinsString::CharCodeAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, CharCodeAt);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisFlat(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), thisHandle));
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisFlat).GetLength());
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 0);
    int32_t pos = 0;
    if (posTag->IsInt()) {
        pos = posTag->GetInt();
    } else if (posTag->IsUndefined()) {
        pos = 0;
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double valueNumber = posVal.GetNumber();
        if (!std::isfinite(valueNumber)) {
            return GetTaggedDouble(base::NAN_VALUE);
        }
        pos = posVal.ToInt32();
    }
    if (pos < 0 || pos >= thisLen) {
        return GetTaggedDouble(base::NAN_VALUE);
    }
    uint16_t ret = EcmaStringAccessor(thisFlat).Get<false>(pos);
    return GetTaggedInt(ret);
}

// 21.1.3.3
JSTaggedValue BuiltinsString::CodePointAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, CodePointAt);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisFlat(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), thisHandle));
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 0);

    JSTaggedNumber posVal = JSTaggedValue::ToNumber(thread, posTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t pos = base::NumberHelper::DoubleInRangeInt32(posVal.GetNumber());
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisFlat).GetLength());
    if (pos < 0 || pos >= thisLen) {
        return JSTaggedValue::Undefined();
    }
    uint16_t first = EcmaStringAccessor(thisFlat).Get<false>(pos);
    if (first < base::utf_helper::DECODE_LEAD_LOW || first > base::utf_helper::DECODE_LEAD_HIGH || pos + 1 == thisLen) {
        return GetTaggedInt(first);
    }
    uint16_t second = EcmaStringAccessor(thisFlat).Get<false>(pos + 1);
    if (second < base::utf_helper::DECODE_TRAIL_LOW || second > base::utf_helper::DECODE_TRAIL_HIGH) {
        return GetTaggedInt(first);
    }
    uint32_t res = base::utf_helper::UTF16Decode(first, second);
    return GetTaggedInt(res);
}

// 21.1.3.4
JSTaggedValue BuiltinsString::Concat(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Concat);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t argLength = argv->GetArgsNumber();
    if (argLength == 0) {
        return thisHandle.GetTaggedValue();
    }
    for (uint32_t i = 0; i < argLength; i++) {
        JSHandle<JSTaggedValue> nextTag = BuiltinsString::GetCallArg(argv, i);
        JSHandle<EcmaString> nextHandle = JSTaggedValue::ToString(thread, nextTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaString *tempStr = EcmaStringAccessor::Concat(ecmaVm, thisHandle, nextHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        thisHandle = JSHandle<EcmaString>(thread, tempStr);
    }
    return thisHandle.GetTaggedValue();
}

// 21.1.3.5 String.prototype.constructor
// 21.1.3.6
JSTaggedValue BuiltinsString::EndsWith(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, EndsWith);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isRegexp = JSObject::IsRegExp(thread, searchTag);
    if (isRegexp) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "is regexp", JSTaggedValue::Exception());
    }
    JSHandle<EcmaString> searchHandle = JSTaggedValue::ToString(thread, searchTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t thisLen = EcmaStringAccessor(thisHandle).GetLength();
    uint32_t searchLen = EcmaStringAccessor(searchHandle).GetLength();
    int32_t pos = 0;
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 1);
    if (posTag->IsUndefined()) {
        pos = static_cast<int32_t>(thisLen);
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (posVal.GetNumber() == BuiltinsNumber::POSITIVE_INFINITY) {
            pos = static_cast<int32_t>(thisLen);
        } else {
            pos = base::NumberHelper::DoubleInRangeInt32(posVal.GetNumber());
        }
    }
    pos = std::min(std::max(pos, 0), static_cast<int32_t>(thisLen));
    int32_t start = pos - static_cast<int32_t>(searchLen);
    if (start < 0) {
        return BuiltinsString::GetTaggedBoolean(false);
    }

    int32_t idx = EcmaStringAccessor::IndexOf(thread->GetEcmaVM(), thisHandle, searchHandle, start);
    if (idx == start) {
        return BuiltinsString::GetTaggedBoolean(true);
    }
    return BuiltinsString::GetTaggedBoolean(false);
}

// 21.1.3.7
JSTaggedValue BuiltinsString::Includes(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Includes);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isRegexp = JSObject::IsRegExp(thread, searchTag);
    if (isRegexp) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "is regexp", JSTaggedValue::Exception());
    }
    JSHandle<EcmaString> searchHandle = JSTaggedValue::ToString(thread, searchTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t thisLen = EcmaStringAccessor(thisHandle).GetLength();
    int32_t pos = 0;
    JSHandle<JSTaggedValue> posTag = BuiltinsBase::GetCallArg(argv, 1);
    if (argv->GetArgsNumber() == 1) {
        pos = 0;
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToNumber(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        pos = base::NumberHelper::DoubleInRangeInt32(posVal.GetNumber());
    }
    int32_t start = std::min(std::max(pos, 0), static_cast<int32_t>(thisLen));
    int32_t idx = EcmaStringAccessor::IndexOf(thread->GetEcmaVM(), thisHandle, searchHandle, start);
    if (idx < 0 || idx > static_cast<int32_t>(thisLen)) {
        return BuiltinsString::GetTaggedBoolean(false);
    }
    return BuiltinsString::GetTaggedBoolean(true);
}

// 21.1.3.8
JSTaggedValue BuiltinsString::IndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, IndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t thisLen = EcmaStringAccessor(thisHandle).GetLength();
    JSHandle<EcmaString> searchHandle = JSTaggedValue::ToString(thread, searchTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 1);
    int32_t pos = 0;
    if (posTag->IsInt()) {
        pos = posTag->GetInt();
    } else if (posTag->IsUndefined()) {
        pos = 0;
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        pos = posVal.ToInt32();
    }
    pos = std::min(std::max(pos, 0), static_cast<int32_t>(thisLen));
    // If searching for an null string.
    if (EcmaStringAccessor(searchHandle).GetLength() == 0) {
        return GetTaggedInt(pos);
    }
    int32_t res = EcmaStringAccessor::IndexOf(thread->GetEcmaVM(), thisHandle, searchHandle, pos);
    if (res >= 0 && res < static_cast<int32_t>(thisLen)) {
        return GetTaggedInt(res);
    }
    return GetTaggedInt(-1);
}

// 21.1.3.9
JSTaggedValue BuiltinsString::LastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, LastIndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisHandle).GetLength());
    JSHandle<EcmaString> searchHandle = JSTaggedValue::ToString(thread, searchTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t pos = 0;
    if (argv->GetArgsNumber() == 1) {
        pos = thisLen;
    } else {
        JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 1);
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (std::isnan(JSTaggedValue::ToNumber(thread, posTag).GetNumber())) {
            pos = thisLen;
        } else {
            pos = posVal.ToInt32();
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    pos = std::min(std::max(pos, 0), thisLen);
    int32_t res = EcmaStringAccessor::LastIndexOf(thread->GetEcmaVM(), thisHandle, searchHandle, pos);
    if (res >= 0 && res <= thisLen) {
        return GetTaggedInt(res);
    }
    res = -1;
    return GetTaggedInt(static_cast<int32_t>(res));
}

// 21.1.3.10
JSTaggedValue BuiltinsString::LocaleCompare(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, LocaleCompare);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thatTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] JSHandle<EcmaString> thatHandle = JSTaggedValue::ToString(thread, thatTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 2); // 2: the second argument
    return DoLocaleCompare(thread, thisHandle, thatHandle, locales, options);
}

JSTaggedValue BuiltinsString::DoLocaleCompare(JSThread *thread,
                                              const JSHandle<EcmaString> &thisHandle,
                                              const JSHandle<EcmaString> &thatHandle,
                                              const JSHandle<JSTaggedValue> &locales,
                                              const JSHandle<JSTaggedValue> &options)
{
    [[maybe_unused]] bool cacheable = (locales->IsUndefined() || locales->IsString()) && options->IsUndefined();
    const CompareStringsOption csOption = JSCollator::CompareStringsOptionFor(thread, locales, options);
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto collator = JSCollator::GetCachedIcuCollator(thread, locales);
        if (collator != nullptr) {
            JSTaggedValue result = JSCollator::CompareStrings(thread, collator, thisHandle, thatHandle, csOption);
            return result;
        }
    }
    return LocaleCompareGC(thread, thisHandle, thatHandle, locales, options, csOption, cacheable);
#else
#ifdef ARK_NOT_SUPPORT_INTL_GLOBAL
    ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, "LocaleCompare");
#else
    intl::GlobalIntlHelper gh(thread, intl::GlobalFormatterType::Collator);
    auto collator = gh.GetGlobalObject<intl::GlobalCollator>(thread,
        locales, options, intl::GlobalFormatterType::Collator, cacheable);
    if (collator == nullptr) {
        LOG_ECMA(ERROR) << "BuiltinsString::LocaleCompare:collator is nullptr";
    }
    ASSERT(collator != nullptr);
    auto result = collator->Compare(EcmaStringAccessor(thisHandle).ToStdString(),
        EcmaStringAccessor(thatHandle).ToStdString());
    return JSTaggedValue(result);
#endif
#endif
}

JSTaggedValue BuiltinsString::LocaleCompareGC(JSThread *thread,
                                              const JSHandle<EcmaString> &thisHandle,
                                              const JSHandle<EcmaString> &thatHandle,
                                              const JSHandle<JSTaggedValue> &locales,
                                              const JSHandle<JSTaggedValue> &options,
                                              CompareStringsOption csOption,
                                              bool cacheable)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> ctor = ecmaVm->GetGlobalEnv()->GetCollatorFunction();
    JSHandle<JSCollator> collator =
        JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor)));
    JSHandle<JSCollator> initCollator =
        JSCollator::InitializeCollator(thread, collator, locales, options, cacheable);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    icu::Collator *icuCollator = nullptr;
    if (cacheable) {
        icuCollator = JSCollator::GetCachedIcuCollator(thread, locales);
        ASSERT(icuCollator != nullptr);
    } else {
        icuCollator = initCollator->GetIcuCollator();
    }
    JSTaggedValue result = JSCollator::CompareStrings(thread, icuCollator, thisHandle, thatHandle, csOption);
    return result;
}


// 21.1.3.11
JSTaggedValue BuiltinsString::Match(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Match);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> regexp = BuiltinsString::GetCallArg(argv, 0);
    if (thisTag->IsString() && regexp->IsECMAObject()) {
        if (BuiltinsRegExp::IsFastRegExp(thread, regexp.GetTaggedValue(), BuiltinsRegExp::RegExpSymbol::MATCH)) {
            return BuiltinsRegExp::RegExpMatch(thread, regexp, thisTag, true);
        }
    }

    JSHandle<JSTaggedValue> matchTag = thread->GetEcmaVM()->GetGlobalEnv()->GetMatchSymbol();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    if (!regexp->IsUndefined() && !regexp->IsNull()) {
        JSHandle<JSTaggedValue> matcher = JSObject::GetMethod(thread, regexp, matchTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!matcher->IsUndefined()) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, matcher, regexp, undefined, 1);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(thisTag.GetTaggedValue());
            return JSFunction::Call(info);
        }
    }
    JSHandle<EcmaString> thisVal = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> undifinedHandle = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> rx(thread, BuiltinsRegExp::RegExpCreate(thread, regexp, undifinedHandle));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, rx, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(thisVal.GetTaggedValue());
    return JSFunction::Invoke(info, matchTag);
}

JSTaggedValue BuiltinsString::MatchAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, MatchAll);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Let O be ? RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> regexp = BuiltinsString::GetCallArg(argv, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> matchAllTag = env->GetMatchAllSymbol();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();

    // 2. If regexp is neither undefined nor null, then
    if (!regexp->IsUndefined() && !regexp->IsNull()) {
        // a. Let isRegExp be ? IsRegExp(searchValue).
        if (regexp->IsECMAObject() &&
            BuiltinsRegExp::IsFastRegExp(thread, regexp.GetTaggedValue(), BuiltinsRegExp::RegExpSymbol::MATCH)) {
            bool isGlobal = BuiltinsRegExp::GetOriginalFlag(thread, regexp, RegExpParser::FLAG_GLOBAL);
            if (!isGlobal) {
                THROW_TYPE_ERROR_AND_RETURN(thread,
                                            "matchAll called with a non-global RegExp argument",
                                            JSTaggedValue::Exception());
            }
        } else if (JSObject::IsRegExp(thread, regexp)) {
            // i. Let flags be ? Get(searchValue, "flags").
            JSHandle<JSTaggedValue> flagsString(globalConst->GetHandledFlagsString());
            JSHandle<JSTaggedValue> flags = JSObject::GetProperty(thread, regexp, flagsString).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // ii. Perform ? RequireObjectCoercible(flags).
            JSTaggedValue::RequireObjectCoercible(thread, flags);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. If ? ToString(flags) does not contain "g", throw a TypeError exception.
            JSHandle<EcmaString> flagString = JSTaggedValue::ToString(thread, flags);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm,
                flagString, ecmaVm->GetFactory()->NewFromASCII("g"));
            if (pos == -1) {
                THROW_TYPE_ERROR_AND_RETURN(thread,
                                            "matchAll called with a non-global RegExp argument",
                                            JSTaggedValue::Exception());
            }
        }

        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (thisTag->IsString() && regexp->IsECMAObject()) {
            if (PropertyDetector::IsRegExpSpeciesDetectorValid(env) &&
                BuiltinsRegExp::IsFastRegExp(thread, regexp.GetTaggedValue(), BuiltinsRegExp::RegExpSymbol::MATCHALL)) {
                JSHandle<EcmaString> string = JSHandle<EcmaString>::Cast(thisTag);
                return BuiltinsRegExp::RegExpMatchAll(thread, regexp, string, true);
            }
        }
        // c. Let matcher be ? GetMethod(regexp, @@matchAll).
        // d. If matcher is not undefined, then
        bool canSkip = (PropertyDetector::IsNumberStringNotRegexpLikeDetectorValid(env) &&
                       (regexp->IsString() || regexp->IsNumber()));
        if (!canSkip) {
            JSHandle<JSTaggedValue> matcher = JSObject::GetMethod(thread, regexp, matchAllTag);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!matcher->IsUndefined()) {
                // i. Return ? Call(matcher, regexp, « O »).
                EcmaRuntimeCallInfo *info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, matcher, regexp, undefined, 1);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                info->SetCallArg(thisTag.GetTaggedValue());
                return JSFunction::Call(info);
            }
        }
    }
    // 3. Let S be ? ToString(O).
    JSHandle<EcmaString> thisVal = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Let rx be ? RegExpCreate(regexp, "g").
    JSHandle<JSTaggedValue> rx(thread, BuiltinsRegExp::RegExpCreate(
        thread, regexp, JSHandle<JSTaggedValue>(ecmaVm->GetFactory()->NewFromASCII("g"))));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, rx, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(thisVal.GetTaggedValue());
    return JSFunction::Invoke(info, matchAllTag);
}

JSTaggedValue BuiltinsString::IsWellFormed(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, IsWellFormed);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ? RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> obj(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let S be ? ToString(O).
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Return IsStringWellFormedUnicode(S).
    uint32_t size = EcmaStringAccessor(string).GetLength();
    uint32_t position = 0;
    while (position < size) {
        // i.Let first be the code unit at index position within string.
        uint16_t first = EcmaStringAccessor(string).Get(position);
        uint32_t cp = first - CHAR16_LETTER_NULL;
        uint8_t codeUnitCount = 0;
        bool isUnpairedSurrogate = false;
        // ii. If first is neither a leading surrogate nor a trailing surrogate, then
        //   a. Return the Record { [[CodePoint]]: cp, [[CodeUnitCount]]: 1, [[IsUnpairedSurrogate]]: false }.
        if (!IsUTF16HighSurrogate(first) && !IsUTF16LowSurrogate(first)) {
            codeUnitCount = 1; // 1 means: code unit count
            isUnpairedSurrogate = false;
        } else if (IsUTF16HighSurrogate(first) && position + 1 == size) {
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
        if (isUnpairedSurrogate) {
            return JSTaggedValue::False();
        } else {
            position = position + codeUnitCount;
        }
        thread->CheckSafepointIfSuspended();
    }
    return JSTaggedValue::True();
}

JSTaggedValue BuiltinsString::ToWellFormed(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, ToWellFormed);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // 1. Let O be ? RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> obj(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let S be ? ToString(O).
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let strLen be the length of S.
    // 4. Let k be 0.
    uint32_t size = EcmaStringAccessor(string).GetLength();
    uint32_t position = 0;

    // 5. Let result be the empty String.
    std::u16string r;

    // Repeat, while k < strLen,
    //     a. Let cp be CodePointAt(S, k).
    //     b. If cp.[[IsUnpairedSurrogate]] is true, then
    //         i. Set result to the string-concatenation of result and 0xFFFD (REPLACEMENT CHARACTER).
    //     c. Else,
    //         i. Set result to the string-concatenation of result and UTF16EncodeCodePoint(cp.[[CodePoint]]).
    //     d. Set k to k + cp.[[CodeUnitCount]].
    while (position < size) {
        // i.Let first be the code unit at index position within string.
        uint16_t first = EcmaStringAccessor(string).Get(position);
        uint32_t cp = first - CHAR16_LETTER_NULL;
        uint8_t codeUnitCount = 0;
        bool isUnpairedSurrogate = false;
        // ii. If first is neither a leading surrogate nor a trailing surrogate, then
        //   a. Return the Record { [[CodePoint]]: cp, [[CodeUnitCount]]: 1, [[IsUnpairedSurrogate]]: false }.
        if (!IsUTF16HighSurrogate(first) && !IsUTF16LowSurrogate(first)) {
            codeUnitCount = 1; // 1 means: code unit count
            isUnpairedSurrogate = false;
        } else if (IsUTF16HighSurrogate(first) && position + 1 == size) {
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
        if (isUnpairedSurrogate) {
            r.push_back(0xFFFD);
        } else {
            if (cp < 0 || cp > ENCODE_MAX_UTF16) {
                THROW_RANGE_ERROR_AND_RETURN(thread, "CodePoint < 0 or CodePoint > 0x10FFFF",
                                             JSTaggedValue::Exception());
            }
            if (cp > UINT16_MAX) {
                uint16_t cu1 = std::floor((static_cast<uint32_t>(cp) - ENCODE_SECOND_FACTOR) /
                                           ENCODE_FIRST_FACTOR) + ENCODE_LEAD_LOW;
                uint16_t cu2 = ((static_cast<uint32_t>(cp) - ENCODE_SECOND_FACTOR) %
                                 ENCODE_FIRST_FACTOR) + ENCODE_TRAIL_LOW;
                std::u16string nextU16str1 = base::StringHelper::Utf16ToU16String(&cu1, 1);
                std::u16string nextU16str2 = base::StringHelper::Utf16ToU16String(&cu2, 1);
                base::StringHelper::InplaceAppend(r, nextU16str1);
                base::StringHelper::InplaceAppend(r, nextU16str2);
            } else {
                auto u16tCp = static_cast<uint16_t>(cp);
                std::u16string nextU16str = base::StringHelper::Utf16ToU16String(&u16tCp, 1);
                base::StringHelper::InplaceAppend(r, nextU16str);
            }
        }
        position = position + codeUnitCount;
        thread->CheckSafepointIfSuspended();
    }
    const char16_t *constChar16tData = r.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = r.size();
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
}

// Static Semantics: UTF16SurrogatePairToCodePoint ( lead, trail )
uint32_t BuiltinsString::UTF16SurrogatePairToCodePoint(uint16_t lead, uint16_t trail)
{
    // 1. Assert: lead is a leading surrogate and trail is a trailing surrogate.
    ASSERT(IsUTF16HighSurrogate(lead) && IsUTF16LowSurrogate(trail));
    // 2. Let cp be (lead - 0xD800) × 0x400 + (trail - 0xDC00) + 0x10000.
    uint32_t cp = ((lead - 0xD800) << 10UL) + (trail - 0xDC00) + 0x10000;
    // 3. Return the code point cp.
    return cp;
}

// 21.1.3.12
JSTaggedValue BuiltinsString::Normalize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Normalize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto vm = thread->GetEcmaVM();
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<EcmaString> formValue;
    if (argv->GetArgsNumber() == 0) {
        formValue = JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfcString());
    } else {
        JSHandle<JSTaggedValue> formTag = BuiltinsString::GetCallArg(argv, 0);
        if (formTag->IsUndefined()) {
            formValue = JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfcString());
        } else {
            formValue = JSTaggedValue::ToString(thread, formTag);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        }
    }
    UNormalizationMode uForm;
    if (JSHandle<EcmaString> nfc =
        JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfcString());
        EcmaStringAccessor::StringsAreEqual(vm, formValue, nfc)) {
        uForm = UNORM_NFC;
    } else if (JSHandle<EcmaString> nfd =
        JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfdString());
        EcmaStringAccessor::StringsAreEqual(vm, formValue, nfd)) {
        uForm = UNORM_NFD;
    } else if (JSHandle<EcmaString> nfkc =
        JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfkcString());
        EcmaStringAccessor::StringsAreEqual(vm, formValue, nfkc)) {
        uForm = UNORM_NFKC;
    } else if (JSHandle<EcmaString> nfkd =
        JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfkdString());
        EcmaStringAccessor::StringsAreEqual(vm, formValue, nfkd)) {
        uForm = UNORM_NFKD;
    } else {
        THROW_RANGE_ERROR_AND_RETURN(thread, "compare not equal", JSTaggedValue::Exception());
    }

    std::u16string u16strThis = EcmaStringAccessor(thisHandle).ToU16String();
    const char16_t *constChar16tData = u16strThis.data();
    icu::UnicodeString src(constChar16tData, u16strThis.size());
    icu::UnicodeString res;
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t option = 0;

    icu::Normalizer::normalize(src, uForm, option, res, errorCode);
    JSHandle<EcmaString> str = intl::LocaleHelper::UStringToString(thread, res);
    return JSTaggedValue(*str);
}

JSTaggedValue BuiltinsString::PadStart(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, PadStart);
    return BuiltinsString::Pad(argv, true);
}

JSTaggedValue BuiltinsString::PadEnd(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, PadEnd);
    return BuiltinsString::Pad(argv, false);
}

// 21.1.3.13
JSTaggedValue BuiltinsString::Repeat(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Repeat);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t thisLen = EcmaStringAccessor(thisHandle).GetLength();
    JSHandle<JSTaggedValue> countTag = BuiltinsString::GetCallArg(argv, 0);
    int32_t count = 0;
    if (countTag->IsInt()) {
        count = countTag->GetInt();
    } else {
        JSTaggedNumber num = JSTaggedValue::ToInteger(thread, countTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double d = num.GetNumber();
        if (d == base::POSITIVE_INFINITY) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "is infinity", JSTaggedValue::Exception());
        }
        count = base::NumberHelper::DoubleInRangeInt32(d);
    }
    if (count < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "less than 0", JSTaggedValue::Exception());
    }
    if (count == 0) {
        auto emptyStr = thread->GetEcmaVM()->GetFactory()->GetEmptyString();
        return emptyStr.GetTaggedValue();
    }
    if (thisLen == 0) {
        return thisHandle.GetTaggedValue();
    }
    if (static_cast<uint32_t>(count) >= static_cast<uint32_t>(EcmaString::MAX_STRING_LENGTH) / thisLen) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
    }
    bool isUtf8 = EcmaStringAccessor(thisHandle).IsUtf8();
    EcmaString *result = EcmaStringAccessor::CreateLineString(thread->GetEcmaVM(), thisLen * count, isUtf8);
    for (uint32_t index = 0; index < static_cast<uint32_t>(count); ++index) {
        EcmaStringAccessor::ReadData(result, *thisHandle, index * thisLen, (count - index) * thisLen, thisLen);
    }
    return JSTaggedValue(result);
}

// 21.1.3.14
JSTaggedValue BuiltinsString::Replace(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Replace);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, BuiltinsString::GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> replaceTag = BuiltinsString::GetCallArg(argv, 1);

    ObjectFactory *factory = ecmaVm->GetFactory();

    if (searchTag->IsJSRegExp() && replaceTag->IsString()) {
        JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
        JSHandle<JSRegExp> re(searchTag);
        JSHandle<JSTaggedValue> pattern(thread, re->GetOriginalSource());
        JSHandle<JSTaggedValue> flags(thread, re->GetOriginalFlags());
        bool isFastPath = BuiltinsRegExp::IsFastRegExp(thread, searchTag.GetTaggedValue());
        if (isFastPath) {
            uint32_t lastIndex = static_cast<uint32_t>(BuiltinsRegExp::GetLastIndex(thread, searchTag, true));
            JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, thisTag,
                RegExpExecResultCache::REPLACE_TYPE, searchTag, JSTaggedValue(lastIndex),
                replaceTag);
            if (!cacheResult.IsUndefined()) {
                return cacheResult;
            }
        }
    }

    if (searchTag->IsJSRegExp() && PropertyDetector::IsRegExpReplaceDetectorValid(env)) {
        JSTaggedValue proto = JSObject::GetPrototype(JSHandle<JSObject>(searchTag));
        if (proto == env->GetTaggedRegExpPrototype()) {
            return BuiltinsRegExp::ReplaceInternal(thread, searchTag, thisTag, replaceTag);
        }
    }

    // If searchValue is neither undefined nor null, then
    if (!searchTag->IsUndefined() && !searchTag->IsNull()) {
        JSHandle<JSTaggedValue> replaceKey = env->GetReplaceSymbol();
        // Let replacer be GetMethod(searchValue, @@replace).
        JSHandle<JSTaggedValue> replaceMethod = JSObject::GetMethod(thread, searchTag, replaceKey);
        // ReturnIfAbrupt(replacer).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // If replacer is not undefined, then
        if (!replaceMethod->IsUndefined()) {
            // Return Call(replacer, searchValue, «O, replaceValue»).
            const uint32_t argsLength = 2;
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, replaceMethod, searchTag, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(thisTag.GetTaggedValue(), replaceTag.GetTaggedValue());
            return JSFunction::Call(info);
        }
    }

    // Let string be ToString(O).
    JSHandle<EcmaString> thisString = JSTaggedValue::ToString(thread, thisTag);
    // ReturnIfAbrupt(string).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let searchString be ToString(searchValue).
    JSHandle<EcmaString> searchString = JSTaggedValue::ToString(thread, searchTag);
    // ReturnIfAbrupt(searchString).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let functionalReplace be IsCallable(replaceValue).
    if (!replaceTag->IsCallable()) {
        // If functionalReplace is false, then
        // Let replaceValue be ToString(replaceValue).
        // ReturnIfAbrupt(replaceValue)
        replaceTag = JSHandle<JSTaggedValue>(JSTaggedValue::ToString(thread, replaceTag));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // Search string for the first occurrence of searchString and let pos be the index within string of the first code
    // unit of the matched substring and let matched be searchString. If no occurrences of searchString were found,
    // return string.
    int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, searchString);
    if (pos == -1) {
        return thisString.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> replHandle(thread, factory->GetEmptyString().GetTaggedValue());
    // If functionalReplace is true, then
    if (replaceTag->IsCallable()) {
        // Let replValue be Call(replaceValue, undefined,«matched, pos, and string»).
        const uint32_t argsLength = 3; // 3: «matched, pos, and string»
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, replaceTag, undefined, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(searchString.GetTaggedValue(), JSTaggedValue(pos), thisString.GetTaggedValue());
        JSTaggedValue replStrDeocodeValue = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        replHandle.Update(replStrDeocodeValue);
    } else {
        // Let captures be an empty List.
        JSHandle<TaggedArray> capturesList = factory->EmptyArray();
        ASSERT_PRINT(replaceTag->IsString(), "replace must be string");
        JSHandle<EcmaString> replacement(thread, replaceTag->GetTaggedObject());
        // Let replStr be GetSubstitution(matched, string, pos, captures, replaceValue)
        replHandle.Update(GetSubstitution(thread, searchString, thisString, pos, capturesList, undefined, replacement));
    }
    JSHandle<EcmaString> realReplaceStr = JSTaggedValue::ToString(thread, replHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let tailPos be pos + the number of code units in matched.
    int32_t tailPos = pos + static_cast<int32_t>(EcmaStringAccessor(searchString).GetLength());
    // Let newString be the String formed by concatenating the first pos code units of string,
    // replStr, and the trailing
    // substring of string starting at index tailPos. If pos is 0,
    // the first element of the concatenation will be the
    // empty String.
    // Return newString.
    JSHandle<EcmaString> prefixString(thread, EcmaStringAccessor::FastSubString(ecmaVm, thisString, 0, pos));
    auto thisLen = EcmaStringAccessor(thisString).GetLength();
    JSHandle<EcmaString> suffixString(thread,
        EcmaStringAccessor::FastSubString(ecmaVm, thisString, tailPos, thisLen - tailPos));
    EcmaString *tempStr = EcmaStringAccessor::Concat(ecmaVm, prefixString, realReplaceStr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> tempString(thread, tempStr);
    EcmaString *resultStr = EcmaStringAccessor::Concat(ecmaVm, tempString, suffixString);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue(resultStr);
}

JSTaggedValue BuiltinsString::ReplaceAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, String, ReplaceAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, BuiltinsString::GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> replaceTag = BuiltinsString::GetCallArg(argv, 1);

    ObjectFactory *factory = ecmaVm->GetFactory();

    if (!searchTag->IsUndefined() && !searchTag->IsNull()) {
        // a. Let isRegExp be ? IsRegExp(searchValue).
        bool isJSRegExp = JSObject::IsRegExp(thread, searchTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // b. If isRegExp is true, then
        if (isJSRegExp) {
            // i. Let flags be ? Get(searchValue, "flags").
            JSHandle<JSTaggedValue> flagsString(globalConst->GetHandledFlagsString());
            JSHandle<JSTaggedValue> flags = JSObject::GetProperty(thread, searchTag, flagsString).GetValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // ii. Perform ? RequireObjectCoercible(flags).
            JSTaggedValue::RequireObjectCoercible(thread, flags);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. If ? ToString(flags) does not contain "g", throw a TypeError exception.
            JSHandle<EcmaString> flagString = JSTaggedValue::ToString(thread, flags);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<EcmaString> gString(globalConst->GetHandledGString());
            int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, flagString, gString);
            if (pos == -1) {
                THROW_TYPE_ERROR_AND_RETURN(thread,
                                            "string.prototype.replaceAll called with a non-global RegExp argument",
                                            JSTaggedValue::Exception());
            }
        }
        // c. Let replacer be ? GetMethod(searchValue, @@replace).
        JSHandle<JSTaggedValue> replaceKey = env->GetReplaceSymbol();
        JSHandle<JSTaggedValue> replaceMethod = JSObject::GetMethod(thread, searchTag, replaceKey);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // d. If replacer is not undefined, then
        if (!replaceMethod->IsUndefined()) {
            // i. Return ? Call(replacer, searchValue, «O, replaceValue»).
            const size_t argsLength = 2;
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, replaceMethod, searchTag, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(thisTag.GetTaggedValue(), replaceTag.GetTaggedValue());
            return JSFunction::Call(info);
        }
    }

    // 3. Let string be ? ToString(O).
    JSHandle<EcmaString> thisString = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Let searchString be ? ToString(searchValue).
    JSHandle<EcmaString> searchString = JSTaggedValue::ToString(thread, searchTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Let functionalReplace be IsCallable(replaceValue).
    // 6. If functionalReplace is false, then
    if (!replaceTag->IsCallable()) {
        // a. Set replaceValue to ? ToString(replaceValue).
        replaceTag = JSHandle<JSTaggedValue>(JSTaggedValue::ToString(thread, replaceTag));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // 7. Let searchLength be the length of searchString.
    // 8. Let advanceBy be max(1, searchLength).
    int32_t searchLength = static_cast<int32_t>(EcmaStringAccessor(searchString).GetLength());
    int32_t advanceBy = std::max(1, searchLength);
    // 9. Let matchPositions be a new empty List.
    JSMutableHandle<EcmaString> accumulatedResult(thread, factory->GetEmptyString());
    // 10. Let position be ! StringIndexOf(string, searchString, 0).
    int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, searchString);
    int32_t endOfLastMatch = 0;
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> replHandle(thread, factory->GetEmptyString().GetTaggedValue());
    while (pos != -1) {
        // If functionalReplace is true, then
        if (replaceTag->IsCallable()) {
            // Let replValue be Call(replaceValue, undefined,«matched, pos, and string»).
            const uint32_t argsLength = 3; // 3: «matched, pos, and string»
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, replaceTag, undefined, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(searchString.GetTaggedValue(), JSTaggedValue(pos), thisString.GetTaggedValue());
            JSTaggedValue replStrDeocodeValue = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            replHandle.Update(replStrDeocodeValue);
        } else {
            // Let captures be an empty List.
            JSHandle<TaggedArray> capturesList = factory->NewTaggedArray(0);
            ASSERT_PRINT(replaceTag->IsString(), "replace must be string");
            JSHandle<EcmaString> replacement(thread, replaceTag->GetTaggedObject());
            // Let replStr be GetSubstitution(matched, string, pos, captures, replaceValue)
            replHandle.Update(GetSubstitution(thread, searchString, thisString, pos,
                                              capturesList, undefined, replacement));
        }
        JSHandle<EcmaString> realReplaceStr = JSTaggedValue::ToString(thread, replHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // Let tailPos be pos + the number of code units in matched.
        // Let newString be the String formed by concatenating the first pos code units of string,
        // replStr, and the trailing substring of string starting at index tailPos.
        // If pos is 0, the first element of the concatenation will be the
        // empty String.
        // Return newString.
        JSHandle<EcmaString> prefixString(thread,
                                          EcmaStringAccessor::FastSubString(ecmaVm, thisString, endOfLastMatch,
                                                                            pos - endOfLastMatch));
        accumulatedResult.Update(JSTaggedValue(EcmaStringAccessor::Concat(ecmaVm, accumulatedResult, prefixString)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        accumulatedResult.Update(JSTaggedValue(EcmaStringAccessor::Concat(ecmaVm, accumulatedResult, realReplaceStr)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        endOfLastMatch = pos + searchLength;
        pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, searchString, pos + advanceBy);
        thread->CheckSafepointIfSuspended();
    }

    if (endOfLastMatch < static_cast<int32_t>(EcmaStringAccessor(thisString).GetLength())) {
        auto thisLen = EcmaStringAccessor(thisString).GetLength();
        JSHandle<EcmaString> suffixString(thread,
            EcmaStringAccessor::FastSubString(ecmaVm, thisString, endOfLastMatch, thisLen - endOfLastMatch));
        accumulatedResult.Update(JSTaggedValue(EcmaStringAccessor::Concat(ecmaVm, accumulatedResult, suffixString)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    return accumulatedResult.GetTaggedValue();
}

// Handle $& - match case
void ProcessDollarAmpersand(std::u16string &stringBuilder, const JSHandle<EcmaString> &matched, bool &canBeCompress)
{
    stringBuilder += EcmaStringAccessor(matched).ToU16String();
    if (EcmaStringAccessor(matched).IsUtf16()) {
        canBeCompress = false;
    }
}

// Handle $` - prefix case
void ProcessDollarBacktick(EcmaVM *ecmaVm, std::u16string &stringBuilder, const JSHandle<EcmaString> &srcString,
                           int position, bool &canBeCompress)
{
    if (position > 0) {
        EcmaString *prefix = EcmaStringAccessor::FastSubString(ecmaVm, srcString, 0, position);
        stringBuilder += EcmaStringAccessor(prefix).ToU16String();
        if (EcmaStringAccessor(prefix).IsUtf16()) {
            canBeCompress = false;
        }
    }
}

// Handle $' - suffix case
void ProcessDollarSingleQuote(EcmaVM *ecmaVm, std::u16string &stringBuilder, const JSHandle<EcmaString> &srcString,
                              int tailPos, bool &canBeCompress)
{
    int32_t srcLength = static_cast<int32_t>(EcmaStringAccessor(srcString).GetLength());
    if (tailPos < srcLength) {
        EcmaString *suffix = EcmaStringAccessor::FastSubString(ecmaVm, srcString, tailPos, srcLength - tailPos);
        stringBuilder += EcmaStringAccessor(suffix).ToU16String();
        if (EcmaStringAccessor(suffix).IsUtf16()) {
            canBeCompress = false;
        }
    }
}

std::pair<int32_t, bool> ProcessDigitCapture(const JSHandle<EcmaString> &replacementFlat, uint32_t peekIndex,
                                             uint32_t replaceLength, const JSHandle<TaggedArray> &captureList,
                                             std::u16string &stringBuilder)
{
    uint32_t capturesLength = captureList->GetLength();
    uint16_t peek = EcmaStringAccessor(replacementFlat).Get(peekIndex);
    uint32_t scaledIndex = peek - '0';
    int32_t advance = 1;
    bool canBeCompress = true;

    if (peekIndex + 1 < replaceLength) {
        uint16_t nextPeek = EcmaStringAccessor(replacementFlat).Get(peekIndex + 1);
        if (nextPeek >= '0' && nextPeek <= '9') {
            constexpr uint32_t TEN_BASE = 10;
            uint32_t newScaledIndex = scaledIndex * TEN_BASE + (nextPeek - '0');
            if (newScaledIndex <= capturesLength) {
                scaledIndex = newScaledIndex;
                advance = 2;  // 2: 2 means from index needs to add two.
            }
        }
    }

    if (scaledIndex == 0 || scaledIndex > capturesLength) {
        stringBuilder += '$';
        return {peekIndex, canBeCompress};  // No change in compressibility, just return the next index.
    }

    JSTaggedValue capturesVal(captureList->Get(scaledIndex - 1));
    if (!capturesVal.IsUndefined()) {
        EcmaString *captureString = EcmaString::Cast(capturesVal.GetTaggedObject());
        stringBuilder += EcmaStringAccessor(captureString).ToU16String();
        if (EcmaStringAccessor(captureString).IsUtf16()) {
            canBeCompress = false;
        }
    }
    return {static_cast<int32_t>(peekIndex) + advance, canBeCompress};
}

// Handle $< case
std::pair<int32_t, bool> ProcessNamedCaptures(JSThread *thread, const JSHandle<EcmaString> &replacementFlat,
                                              int32_t peekIndex, const JSHandle<JSTaggedValue> &namedCaptures,
                                              std::u16string &stringBuilder)
{
    bool canBeCompress = true;
    if (namedCaptures->IsUndefined()) {
        stringBuilder += '$';
        return {peekIndex, canBeCompress};
    }
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> greaterSymString = factory->NewFromASCII(">");
    int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, replacementFlat, greaterSymString, peekIndex);
    if (pos == -1) {
        stringBuilder += '$';
        return {peekIndex, canBeCompress};
    }
    JSHandle<EcmaString> groupName = JSHandle<EcmaString>(
        thread, EcmaStringAccessor::FastSubString(ecmaVm, replacementFlat, peekIndex + 1, pos - peekIndex - 1));
    JSHandle<JSTaggedValue> names(groupName);
    JSHandle<JSTaggedValue> capture = JSObject::GetProperty(thread, namedCaptures, names).GetValue();
    if (capture->IsUndefined()) {
        return {pos + 1, canBeCompress};
    }
    JSHandle<EcmaString> captureName = JSTaggedValue::ToString(thread, capture);
    stringBuilder += EcmaStringAccessor(captureName).ToU16String();
    if (EcmaStringAccessor(captureName).IsUtf16()) {
        canBeCompress = false;
    }
    return {pos + 1, canBeCompress};
}

JSTaggedValue BuiltinsString::GetSubstitution(JSThread *thread, const JSHandle<EcmaString> &matched,
                                              const JSHandle<EcmaString> &srcString, int position,
                                              const JSHandle<TaggedArray> &captureList,
                                              const JSHandle<JSTaggedValue> &namedCaptures,
                                              const JSHandle<EcmaString> &replacement)
{
    BUILTINS_API_TRACE(thread, String, GetSubstitution);
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> dollarString = JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledDollarString());
    JSHandle<EcmaString> replacementFlat(thread, EcmaStringAccessor::Flatten(ecmaVm, replacement));
    int32_t replaceLength = static_cast<int32_t>(EcmaStringAccessor(replacementFlat).GetLength());
    int32_t tailPos = position + static_cast<int32_t>(EcmaStringAccessor(matched).GetLength());

    int32_t nextDollarIndex = EcmaStringAccessor::IndexOf(ecmaVm, replacementFlat, dollarString);
    if (nextDollarIndex < 0) {
        return replacementFlat.GetTaggedValue();
    }
    std::u16string stringBuilder;
    bool canBeCompress = true;
    if (nextDollarIndex > 0) {
        stringBuilder = EcmaStringAccessor(replacementFlat).ToU16String(nextDollarIndex);
        if (EcmaStringAccessor(replacementFlat).IsUtf16()) {
            canBeCompress = false;
        }
    }

    while (true) {
        int peekIndex = nextDollarIndex + 1;
        if (peekIndex >= replaceLength) {
            stringBuilder += '$';
            auto *char16tData = const_cast<char16_t *>(stringBuilder.c_str());
            auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
            return canBeCompress ?
                   factory->NewFromUtf16LiteralCompress(uint16tData, stringBuilder.length()).GetTaggedValue() :
                   factory->NewFromUtf16LiteralNotCompress(uint16tData, stringBuilder.length()).GetTaggedValue();
        }
        int continueFromIndex = -1;
        uint16_t peek = EcmaStringAccessor(replacementFlat).Get(peekIndex);
        switch (peek) {
            case '$':  // $$
                stringBuilder += '$';
                continueFromIndex = peekIndex + 1;
                break;
            case '&':  // $& - match
                ProcessDollarAmpersand(stringBuilder, matched, canBeCompress);
                continueFromIndex = peekIndex + 1;
                break;
            case '`':  // $` - prefix
                ProcessDollarBacktick(ecmaVm, stringBuilder, srcString, position, canBeCompress);
                continueFromIndex = peekIndex + 1;
                break;
            case '\'': {  // $' - suffix
                ProcessDollarSingleQuote(ecmaVm, stringBuilder, srcString, tailPos, canBeCompress);
                continueFromIndex = peekIndex + 1;
                break;
            }
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                auto result =
                    ProcessDigitCapture(replacementFlat, peekIndex, replaceLength, captureList, stringBuilder);
                continueFromIndex = result.first;
                canBeCompress = result.second && canBeCompress;  // 保留canBeCompress的值，只在需要时更新为false
                break;
            }
            case '<': {
                auto result = ProcessNamedCaptures(thread, replacementFlat, peekIndex, namedCaptures, stringBuilder);
                continueFromIndex = result.first;
                canBeCompress = result.second && canBeCompress;  // 保留canBeCompress的值，只在需要时更新为false
                break;
            }
            default:
                stringBuilder += '$';
                continueFromIndex = peekIndex;
                break;
        }
        // Go the the next $ in the replacement.
        nextDollarIndex = EcmaStringAccessor::IndexOf(ecmaVm, replacementFlat, dollarString, continueFromIndex);
        if (nextDollarIndex < 0) {
            if (continueFromIndex < replaceLength) {
                EcmaString *nextAppend = EcmaStringAccessor::FastSubString(ecmaVm, replacementFlat, continueFromIndex,
                                                                           replaceLength - continueFromIndex);
                stringBuilder += EcmaStringAccessor(nextAppend).ToU16String();
                if (EcmaStringAccessor(nextAppend).IsUtf16()) {
                    canBeCompress = false;
                }
            }
            auto *char16tData = const_cast<char16_t *>(stringBuilder.c_str());
            auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
            return canBeCompress ?
                   factory->NewFromUtf16LiteralCompress(uint16tData, stringBuilder.length()).GetTaggedValue() :
                   factory->NewFromUtf16LiteralNotCompress(uint16tData, stringBuilder.length()).GetTaggedValue();
        }
        // Append substring between the previous and the next $ character.
        if (nextDollarIndex > continueFromIndex) {
            EcmaString *nextAppend = EcmaStringAccessor::FastSubString(
                ecmaVm, replacementFlat, continueFromIndex, nextDollarIndex - continueFromIndex);
            stringBuilder += EcmaStringAccessor(nextAppend).ToU16String();
            if (EcmaStringAccessor(nextAppend).IsUtf16()) {
                canBeCompress = false;
            }
        }
        thread->CheckSafepointIfSuspended();
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

// 21.1.3.15
JSTaggedValue BuiltinsString::Search(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Search);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> regexp = BuiltinsString::GetCallArg(argv, 0);
    if (thisTag->IsString() && regexp->IsECMAObject()) {
        if (BuiltinsRegExp::IsFastRegExp(thread, regexp.GetTaggedValue(), BuiltinsRegExp::RegExpSymbol::SEARCH)) {
            return BuiltinsRegExp::RegExpSearchFast(thread, regexp, thisTag);
        }
    }
    JSHandle<JSTaggedValue> searchTag = thread->GetEcmaVM()->GetGlobalEnv()->GetSearchSymbol();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    if (!regexp->IsUndefined() && !regexp->IsNull()) {
        JSHandle<JSTaggedValue> searcher = JSObject::GetMethod(thread, regexp, searchTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!searcher->IsUndefined()) {
            ASSERT(searcher->IsJSFunctionBase());
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, searcher, regexp, undefined, 1);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(thisTag.GetTaggedValue());
            return JSFunction::Call(info);
        }
    }
    JSHandle<EcmaString> thisVal = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> rx(thread, BuiltinsRegExp::RegExpCreate(thread, regexp, undefined));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, rx, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(thisVal.GetTaggedValue());
    return JSFunction::Invoke(info, searchTag);
}

// 21.1.3.16
JSTaggedValue BuiltinsString::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Slice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisHandle).GetLength());
    JSHandle<JSTaggedValue> startTag = BuiltinsString::GetCallArg(argv, 0);
    JSTaggedNumber startVal = JSTaggedValue::ToInteger(thread, startTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t start = base::NumberHelper::SaturateTruncDoubleToInt32(startVal.GetNumber());
    int32_t end = 0;
    JSHandle<JSTaggedValue> endTag = BuiltinsString::GetCallArg(argv, 1);
    if (endTag->IsUndefined()) {
        end = thisLen;
    } else {
        JSTaggedNumber endVal = JSTaggedValue::ToInteger(thread, endTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = base::NumberHelper::SaturateTruncDoubleToInt32(endVal.GetNumber());
    }
    int32_t from = 0;
    int32_t to = 0;
    if (start < 0) {
        from = std::max(start + thisLen, 0);
    } else {
        from = std::min(start, thisLen);
    }
    if (end < 0) {
        to = std::max(end + thisLen, 0);
    } else {
        to = std::min(end, thisLen);
    }
    int32_t len = std::max(to - from, 0);
    return JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(), thisHandle, from, len));
}

// 21.1.3.17
JSTaggedValue BuiltinsString::Split(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Split);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();

    // Let O be RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> seperatorTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> limitTag = BuiltinsString::GetCallArg(argv, 1);

    if (thisTag->IsString() && seperatorTag->IsECMAObject()) {
        // this condition need change, all regexp should use RegExpSplit
        if (BuiltinsRegExp::IsFastRegExp(thread, seperatorTag.GetTaggedValue())) {
            return BuiltinsRegExp::RegExpSplit(thread, seperatorTag, thisTag, limitTag, true);
        }
    }
    if (thisTag->IsString() && seperatorTag->IsString()) {
        JSHandle<EcmaString> thisString(thisTag);
        JSHandle<EcmaString> seperatorString(seperatorTag);
        auto thisLength = EcmaStringAccessor(thisString).GetLength();
        auto seperatorLength = EcmaStringAccessor(seperatorString).GetLength();
        if (limitTag->IsUndefined() && thisLength != 0 && seperatorLength != 0) {
            return CreateArrayThisStringAndSeperatorStringAreNotEmpty(
                thread, ecmaVm, thisString, seperatorString, thisLength, seperatorLength);
        }
        uint32_t lim = UINT32_MAX - 1;
        if (!limitTag->IsUndefined()) {
            JSTaggedNumber limitIntValue = JSTaggedValue::ToInteger(thread, limitTag);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            lim = limitIntValue.ToUint32();
        }
        // ReturnIfAbrupt(lim).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (lim == 0) {
            JSHandle<JSObject> resultArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return resultArray.GetTaggedValue();
        }
        return CreateArrayBySplitString(thread, ecmaVm, thisString, seperatorString, thisLength, seperatorLength, lim);
    }

    // If separator is neither undefined nor null, then
    if (!seperatorTag->IsUndefined() && !seperatorTag->IsNull()) {
        JSHandle<JSTaggedValue> splitKey = env->GetSplitSymbol();
        // Let splitter be GetMethod(separator, @@split).
        JSHandle<JSTaggedValue> splitter = JSObject::GetMethod(thread, seperatorTag, splitKey);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!splitter->IsUndefined()) {
            // Return Call(splitter, separator, «‍O, limit»).
            const uint32_t argsLength = 2;
            JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, splitter, seperatorTag, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(thisTag.GetTaggedValue(), limitTag.GetTaggedValue());
            return JSFunction::Call(info);
        }
    }
    // Let S be ToString(O).
    JSHandle<EcmaString> thisString = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If limit is undefined, let lim = 2^53–1; else let lim = ToLength(limit).
    uint32_t lim = UINT32_MAX - 1;
    if (!limitTag->IsUndefined()) {
        JSTaggedNumber limitIntValue = JSTaggedValue::ToInteger(thread, limitTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        lim = limitIntValue.ToUint32();
    }
    // ReturnIfAbrupt(lim).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let s be the number of elements in S.
    auto thisLength = EcmaStringAccessor(thisString).GetLength();
    JSHandle<EcmaString> seperatorString = JSTaggedValue::ToString(thread, seperatorTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If lim = 0, return A.
    if (lim == 0) {
        JSHandle<JSObject> resultArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return resultArray.GetTaggedValue();
    }
    auto seperatorLength = EcmaStringAccessor(seperatorString).GetLength();
    // If S is undefined or (this.length = 0 and S.length != 0), return array of size is 1 containing this string
    if (seperatorTag->IsUndefined()) {
        JSHandle<JSObject> resultArray(JSArray::ArrayCreate(thread, JSTaggedNumber(1)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // Perform CreateDataProperty(A, "0", S), CreateDataProperty's fast path
        JSObject::CreateDataProperty(thread, resultArray, 0, JSHandle<JSTaggedValue>(thisString));
        ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty(A, \"0\", S) can't throw exception");
        return resultArray.GetTaggedValue();
    }
    return CreateArrayBySplitString(thread, ecmaVm, thisString, seperatorString, thisLength, seperatorLength, lim);
}

JSTaggedValue BuiltinsString::CreateArrayFromString(JSThread *thread, EcmaVM *ecmaVm,
    const JSHandle<EcmaString> &thisString, uint32_t thisLength, uint32_t lim)
{
    bool isUtf8 = EcmaStringAccessor(thisString).IsUtf8();
    bool canBeCompressed = false;
    if (EcmaStringAccessor(thisString).IsLineOrConstantString()) {
        canBeCompressed = EcmaStringAccessor::CanBeCompressed(*thisString);
    }
    bool isOneByte = isUtf8 & canBeCompressed;
    JSHandle<EcmaString> seperatorString = thread->GetEcmaVM()->GetFactory()->GetEmptyString();
    if (lim == UINT32_MAX - 1) {
        JSHandle<StringSplitResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetStringSplitResultCache());
        JSTaggedValue cacheResult = StringSplitResultCache::FindCachedResult(thread, cacheTable, thisString,
            seperatorString);
        if (cacheResult != JSTaggedValue::Undefined()) {
            JSHandle<JSTaggedValue> resultArray(JSArray::CreateArrayFromList(thread,
                JSHandle<TaggedArray>(thread, cacheResult)));
            return resultArray.GetTaggedValue();
        }
    }
    uint32_t actualLength = std::min(thisLength, lim);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(actualLength);
    for (uint32_t i = 0; i < actualLength; ++i) {
        EcmaString *elementString = EcmaStringAccessor::GetSubString(ecmaVm, thisString, i, 1);
        // Perform CreateDataProperty(A, "0", S), CreateDataProperty's fast path
        if (isOneByte) {
            array->Set<false>(thread, i, JSTaggedValue(elementString));
        } else {
            array->Set(thread, i, JSTaggedValue(elementString));
        }
        ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty can't throw exception");
    }
    JSHandle<JSArray> resultArray = JSArray::CreateArrayFromList(thread, array);
    if (lim == UINT32_MAX - 1) {
        JSHandle<StringSplitResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetStringSplitResultCache());
        StringSplitResultCache::SetCachedResult(thread, cacheTable, thisString, seperatorString, array);
    }
    return resultArray.GetTaggedValue();
}

JSTaggedValue BuiltinsString::CreateArrayBySplitString(JSThread *thread, EcmaVM *ecmaVm,
    const JSHandle<EcmaString> &thisString, const JSHandle<EcmaString> &seperatorString,
    uint32_t thisLength, uint32_t seperatorLength, uint32_t lim)
{
    if (thisLength != 0) {
        if (seperatorLength != 0) {
            return CreateArrayThisStringAndSeperatorStringAreNotEmpty(
                thread, ecmaVm, thisString, seperatorString, thisLength, seperatorLength, lim);
        }
        return CreateArrayFromString(thread, ecmaVm, thisString, thisLength, lim);
    } else {
        if (seperatorLength != 0) {
            JSHandle<JSObject> resultArray(JSArray::ArrayCreate(thread, JSTaggedNumber(1)));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // Perform CreateDataProperty(A, "0", S), CreateDataProperty's fast path
            JSObject::CreateDataProperty(thread, resultArray, 0, JSHandle<JSTaggedValue>(thisString));
            ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty(A, \"0\", S) can't throw exception");
            return resultArray.GetTaggedValue();
        }
        JSHandle<JSObject> resultArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return resultArray.GetTaggedValue();
    }
}

JSTaggedValue BuiltinsString::CreateArrayThisStringAndSeperatorStringAreNotEmpty(JSThread *thread,
    EcmaVM *ecmaVm, const JSHandle<EcmaString> &thisString, const JSHandle<EcmaString> &seperatorString,
    uint32_t thisLength, uint32_t seperatorLength, uint32_t lim)
{
    if (lim == UINT32_MAX - 1) {
        JSHandle<StringSplitResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetStringSplitResultCache());
        JSTaggedValue cacheResult = StringSplitResultCache::FindCachedResult(thread, cacheTable, thisString,
            seperatorString);
        if (cacheResult != JSTaggedValue::Undefined()) {
            JSHandle<JSTaggedValue> resultArray(JSArray::CreateArrayFromList(thread,
                JSHandle<TaggedArray>(thread, cacheResult)));
            return resultArray.GetTaggedValue();
        }
    }
    uint32_t arrayLength = 0;
    std::vector<int32_t> posArray;
    int32_t index = 0;
    int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, seperatorString);
    while (pos != -1) {
        posArray.emplace_back(pos);
        ++arrayLength;
        if (arrayLength == lim) {
            break;
        }
        index = pos + static_cast<int32_t>(seperatorLength);
        pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, seperatorString, index);
        thread->CheckSafepointIfSuspended();
    }
    uint32_t posArrLength = posArray.size();
    arrayLength = lim > posArrLength ? posArrLength + 1 : posArrLength;
    return JSArray::ArrayCreateWithInit(thread, arrayLength,
        [thread, ecmaVm, &thisString, &seperatorString, &posArray, thisLength, seperatorLength, lim, posArrLength]
        (const JSHandle<TaggedArray> &newElements, [[maybe_unused]] uint32_t length) {
        int32_t index = 0;
        int32_t pos = 0;
        JSMutableHandle<JSTaggedValue> elementTag(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < posArrLength; i++) {
            pos = posArray[i];
            EcmaString *elementString = EcmaStringAccessor::GetSubString(ecmaVm, thisString, index, pos - index);
            elementTag.Update(JSTaggedValue(elementString));
            newElements->Set(thread, i, elementTag);
            index = pos + static_cast<int32_t>(seperatorLength);
        }
        if (lim > posArrLength) {
            EcmaString *elementString =
                EcmaStringAccessor::GetSubString(ecmaVm, thisString, index, thisLength - index);
            elementTag.Update(JSTaggedValue(elementString));
            newElements->Set(thread, posArrLength, elementTag);
        }
        if (lim == UINT32_MAX - 1) {
            JSHandle<StringSplitResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetStringSplitResultCache());
            StringSplitResultCache::SetCachedResult(thread, cacheTable, thisString, seperatorString, newElements);
        }
    });
}

// 21.1.3.18
JSTaggedValue BuiltinsString::StartsWith(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, StartsWith);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> searchTag = BuiltinsString::GetCallArg(argv, 0);

    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool isRegexp = JSObject::IsRegExp(thread, searchTag);
    if (isRegexp) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "is regexp", JSTaggedValue::Exception());
    }

    JSHandle<EcmaString> searchHandle = JSTaggedValue::ToString(thread, searchTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t thisLen = EcmaStringAccessor(thisHandle).GetLength();
    uint32_t searchLen = EcmaStringAccessor(searchHandle).GetLength();
    int32_t pos = 0;
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 1);
    if (posTag->IsUndefined()) {
        pos = 0;
    } else if (posTag->IsInt()) {
        pos = posTag->GetInt();
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (posVal.GetNumber() == BuiltinsNumber::POSITIVE_INFINITY) {
            pos = thisLen;
        } else {
            pos = posVal.ToInt32();
        }
    }
    pos = std::min(std::max(pos, 0), static_cast<int32_t>(thisLen));
    if (static_cast<uint32_t>(pos) + searchLen > thisLen) {
        return BuiltinsString::GetTaggedBoolean(false);
    }

    bool result = EcmaStringAccessor::IsSubStringAt(thread->GetEcmaVM(), thisHandle, searchHandle, pos);

    return BuiltinsString::GetTaggedBoolean(result);
}

// 21.1.3.19
JSTaggedValue BuiltinsString::Substring(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Substring);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisHandle).GetLength());
    JSHandle<JSTaggedValue> startTag = BuiltinsString::GetCallArg(argv, 0);
    JSTaggedNumber startVal = JSTaggedValue::ToInteger(thread, startTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t start = base::NumberHelper::SaturateTruncDoubleToInt32(startVal.GetNumber());
    int32_t end = 0;
    JSHandle<JSTaggedValue> endTag = BuiltinsString::GetCallArg(argv, 1);
    if (endTag->IsUndefined()) {
        end = thisLen;
    } else {
        JSTaggedNumber endVal = JSTaggedValue::ToInteger(thread, endTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = base::NumberHelper::SaturateTruncDoubleToInt32(endVal.GetNumber());
    }
    start = std::min(std::max(start, 0), thisLen);
    end = std::min(std::max(end, 0), thisLen);
    int32_t from = std::min(start, end);
    int32_t to = std::max(start, end);
    int32_t len = to - from;
    return JSTaggedValue(EcmaStringAccessor::GetSubString(thread->GetEcmaVM(), thisHandle, from, len));
}

// 21.1.3.20
JSTaggedValue BuiltinsString::ToLocaleLowerCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, ToLocaleLowerCase);
    JSThread *thread = argv->GetThread();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let O be RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> obj(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let S be ? ToString(O).
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    // Fast path
    if (locales->IsUndefined() && EcmaStringAccessor(string).IsUtf8()) {
        EcmaString *result = EcmaStringAccessor::TryToLower(ecmaVm, string);
        return JSTaggedValue(result);
    }
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If requestedLocales is not an empty List, then Let requestedLocale be requestedLocales[0].
    // Else, Let requestedLocale be DefaultLocale().
    JSHandle<EcmaString> requestedLocale = intl::LocaleHelper::DefaultLocale(thread);
    if (requestedLocales->GetLength() != 0) {
        requestedLocale = JSHandle<EcmaString>(thread, requestedLocales->Get(0));
    }

    // Let noExtensionsLocale be the String value that is requestedLocale with all Unicode locale extension sequences
    // removed.
    intl::LocaleHelper::ParsedLocale noExtensionsLocale = intl::LocaleHelper::HandleLocale(requestedLocale);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let availableLocales be a List with language tags that includes the languages for which the Unicode Character
    // Database contains language sensitive case mappings. Implementations may add additional language tags
    // if they support case mapping for additional locales.
    std::vector<std::string> availableLocales = intl::LocaleHelper::GetAvailableLocales(thread, nullptr, nullptr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let locale be BestAvailableLocale(availableLocales, noExtensionsLocale).
    std::string locale = intl::LocaleHelper::BestAvailableLocale(availableLocales, noExtensionsLocale.base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If locale is undefined, let locale be "und".
    if (locale.empty()) {
        locale = "und";
    }

    // Let uString be a List containing in order the code points of S as defined in ES2020, 6.1.4,
    // starting at the first element of S.
    // Transform those elements in uString to the to the Unicode Default Case Conversion algorithm
    icu::Locale icuLocale = icu::Locale::createFromName(locale.c_str());
    EcmaString *result = EcmaStringAccessor::ToLocaleLower(ecmaVm, string, icuLocale);
    return JSTaggedValue(result);
}

// 21.1.3.21
JSTaggedValue BuiltinsString::ToLocaleUpperCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, ToLocaleLowerCase);
    JSThread *thread = argv->GetThread();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let O be RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> obj(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let S be ? ToString(O).
    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, obj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    // Fast path
    if (locales->IsUndefined() && EcmaStringAccessor(string).IsUtf8()) {
        EcmaString *result = EcmaStringAccessor::TryToUpper(ecmaVm, string);
        return JSTaggedValue(result);
    }
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If requestedLocales is not an empty List, then Let requestedLocale be requestedLocales[0].
    // Else, Let requestedLocale be DefaultLocale().
    JSHandle<EcmaString> requestedLocale = intl::LocaleHelper::DefaultLocale(thread);
    if (requestedLocales->GetLength() != 0) {
        requestedLocale = JSHandle<EcmaString>(thread, requestedLocales->Get(0));
    }

    // Let noExtensionsLocale be the String value that is requestedLocale with all Unicode locale extension sequences
    // removed.
    intl::LocaleHelper::ParsedLocale noExtensionsLocale = intl::LocaleHelper::HandleLocale(requestedLocale);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let availableLocales be a List with language tags that includes the languages for which the Unicode Character
    // Database contains language sensitive case mappings. Implementations may add additional language tags
    // if they support case mapping for additional locales.
    std::vector<std::string> availableLocales = intl::LocaleHelper::GetAvailableLocales(thread, nullptr, nullptr);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Let locale be BestAvailableLocale(availableLocales, noExtensionsLocale).
    std::string locale = intl::LocaleHelper::BestAvailableLocale(availableLocales, noExtensionsLocale.base);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If locale is undefined, let locale be "und".
    if (locale.empty()) {
        locale = "und";
    }

    // Let uString be a List containing in order the code points of S as defined in ES2020, 6.1.4,
    // starting at the first element of S.
    // Transform those elements in uString to the to the Unicode Default Case Conversion algorithm
    icu::Locale icuLocale = icu::Locale::createFromName(locale.c_str());
    EcmaString *result = EcmaStringAccessor::ToLocaleUpper(ecmaVm, string, icuLocale);
    return JSTaggedValue(result);
}

// 21.1.3.22
JSTaggedValue BuiltinsString::ToLowerCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, ToLowerCase);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *result = EcmaStringAccessor::ToLower(thread->GetEcmaVM(), thisHandle);
    return JSTaggedValue(result);
}

// 21.1.3.23
JSTaggedValue BuiltinsString::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    return ThisStringValue(argv->GetThread(), GetThis(argv).GetTaggedValue());
}

// 21.1.3.24
JSTaggedValue BuiltinsString::ToUpperCase(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, ToUpperCase);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *result = EcmaStringAccessor::ToUpper(thread->GetEcmaVM(), thisHandle);
    return JSTaggedValue(result);
}

// 21.1.3.25
JSTaggedValue BuiltinsString::Trim(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Trim);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *res = EcmaStringAccessor::Trim(thread, thisHandle, EcmaString::TrimMode::TRIM);
    return JSTaggedValue(res);
}

JSTaggedValue BuiltinsString::TrimStart(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, TrimStart);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *res = EcmaStringAccessor::Trim(thread, thisHandle, EcmaString::TrimMode::TRIM_START);
    return JSTaggedValue(res);
}

JSTaggedValue BuiltinsString::TrimEnd(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, TrimEnd);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *res = EcmaStringAccessor::Trim(thread, thisHandle, EcmaString::TrimMode::TRIM_END);
    return JSTaggedValue(res);
}

JSTaggedValue BuiltinsString::TrimLeft(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, TrimLeft);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *res = EcmaStringAccessor::Trim(thread, thisHandle, EcmaString::TrimMode::TRIM_START);
    return JSTaggedValue(res);
}

JSTaggedValue BuiltinsString::TrimRight(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, TrimRight);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaString *res = EcmaStringAccessor::Trim(thread, thisHandle, EcmaString::TrimMode::TRIM_END);
    return JSTaggedValue(res);
}

// 21.1.3.26
JSTaggedValue BuiltinsString::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    return ThisStringValue(argv->GetThread(), GetThis(argv).GetTaggedValue());
}

// 21.1.3.27
JSTaggedValue BuiltinsString::GetStringIterator(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, GetStringIterator);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be RequireObjectCoercible(this value).
    JSHandle<JSTaggedValue> current(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let S be ToString(O).

    JSHandle<EcmaString> string = JSTaggedValue::ToString(thread, current);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    // Return CreateStringIterator(S).
    return JSStringIterator::CreateStringIterator(thread, string).GetTaggedValue();
}

//  B.2.3.1
JSTaggedValue BuiltinsString::SubStr(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, SubStr);
    JSThread *thread = argv->GetThread();

    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be RequireObjectCoercible(this value).
    // 2. Let S be ToString(O).

    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisString = JSTaggedValue::ToString(thread, thisTag);

    // 3. ReturnIfAbrupt(S).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> intStart = GetCallArg(argv, 0);
    // 4. Let intStart be ToInteger(start).
    JSTaggedNumber numStart = JSTaggedValue::ToInteger(thread, intStart);
    // 5. ReturnIfAbrupt(intStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t start = base::NumberHelper::DoubleInRangeInt32(numStart.GetNumber());
    JSHandle<JSTaggedValue> lengthTag = GetCallArg(argv, 1);
    // 6. If length is undefined, let end be +; otherwise let end be ToInteger(length).
    int32_t end = 0;
    if (lengthTag->IsUndefined()) {
        end = INT_MAX;
    } else {
        JSTaggedNumber lengthNumber = JSTaggedValue::ToInteger(thread, lengthTag);
        // 7. ReturnIfAbrupt(end).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = base::NumberHelper::DoubleInRangeInt32(lengthNumber.GetNumber());
    }
    // 8. Let size be the number of code units in S.
    int32_t size = static_cast<int32_t>(EcmaStringAccessor(thisString).GetLength());
    // 9. If intStart < 0, let intStart be max(size + intStart,0).
    if (start < 0) {
        start = std::max(size + start, 0);
    }
    // 10. Let resultLength be min(max(end,0), size – intStart).
    int32_t resultLength = std::min(std::max(end, 0), size - start);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 11. If resultLength  0, return the empty String "".
    if (resultLength <= 0) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    return JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(), thisString, start, resultLength));
}

// 22.1.3.1
JSTaggedValue BuiltinsString::At(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, At);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be RequireObjectCoercible(this value).
    // 2. Let S be ToString(O).
    JSHandle<JSTaggedValue> thisTag(JSTaggedValue::RequireObjectCoercible(thread, GetThis(argv)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be the length of S.
    int32_t thisLen = static_cast<int32_t>(EcmaStringAccessor(thisHandle).GetLength());

    // 4. Let relativeIndex be ? ToIntegerOrInfinity(index).
    JSHandle<JSTaggedValue> indexTag = BuiltinsString::GetCallArg(argv, 0);
    JSTaggedNumber indexVal = JSTaggedValue::ToInteger(thread, indexTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t relativeIndex = base::NumberHelper::DoubleInRangeInt32(indexVal.GetNumber());

    // 5. If relativeIndex ≥ 0, then Let k be relativeIndex. 6. Else, Let k be len + relativeIndex.
    int32_t k = 0;
    if (relativeIndex >= 0) {
        k = relativeIndex;
    } else {
        k = thisLen + relativeIndex;
    }
    // 7. If k < 0 or k ≥ len, return undefined.
    if (k < 0 || k >= thisLen) {
        return JSTaggedValue::Undefined();
    }
    // 8. Return the substring of S from k to k + 1.
    return JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(), thisHandle, k, 1));
}

JSTaggedValue BuiltinsString::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, String, GetLength);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);

    JSHandle<EcmaString> thisString = JSTaggedValue::ToString(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return GetTaggedInt(EcmaStringAccessor(thisString).GetLength());
}

// 21.1.3
JSTaggedValue BuiltinsString::ThisStringValue(JSThread *thread, JSTaggedValue value)
{
    BUILTINS_API_TRACE(thread, String, ThisStringValue);
    if (value.IsString()) {
        return value;
    }
    if (value.IsECMAObject()) {
        auto jshclass = value.GetTaggedObject()->GetClass();
        if (jshclass->GetObjectType() == JSType::JS_PRIMITIVE_REF) {
            JSTaggedValue primitive = JSPrimitiveRef::Cast(value.GetTaggedObject())->GetValue();
            if (primitive.IsString()) {
                return primitive;
            }
        }
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "can not convert to String", JSTaggedValue::Exception());
}

JSTaggedValue BuiltinsString::Pad(EcmaRuntimeCallInfo *argv, bool isStart)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, String, Pad);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisTag = JSTaggedValue::RequireObjectCoercible(thread, BuiltinsString::GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<EcmaString> thisHandle = JSTaggedValue::ToString(thread, thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> lengthTag = GetCallArg(argv, 0);
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, lengthTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t intMaxLength  = base::NumberHelper::DoubleToInt64(number.GetNumber());
    int32_t stringLength = static_cast<int32_t>(EcmaStringAccessor(thisHandle).GetLength());
    if (intMaxLength <= stringLength) {
        return thisHandle.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> fillString = GetCallArg(argv, 1);
    std::u16string stringBuilder;
    if (fillString->IsUndefined()) {
        stringBuilder = u" ";
    } else {
        JSHandle<EcmaString> filler = JSTaggedValue::ToString(thread, fillString);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        stringBuilder = EcmaStringAccessor(filler).ToU16String();
    }
    if (stringBuilder.size() == 0) {
        return thisHandle.GetTaggedValue();
    }
    std::u16string u16strSearch = EcmaStringAccessor(thisHandle).ToU16String();
    int64_t fillLen = intMaxLength - stringLength;
    int64_t len = static_cast<int64_t>(stringBuilder.length());
    if (static_cast<size_t>(intMaxLength) >= EcmaString::MAX_STRING_LENGTH) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
    }
    std::u16string fiString;
    for (int32_t i = 0; i < fillLen; ++i) {
        fiString += stringBuilder[i % len];
    }
    std::u16string resultString;
    if (isStart) {
        resultString = fiString + u16strSearch;
    } else {
        resultString = u16strSearch + fiString;
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromUtf16Literal(reinterpret_cast<const uint16_t *>(resultString.c_str()),
                                        resultString.size()).GetTaggedValue();
}

JSTaggedValue BuiltinsString::StringToList(JSThread *thread, JSHandle<EcmaString> &str)
{
    JSHandle<StringToListResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetStringToListResultCache());
    JSTaggedValue cacheResult = StringToListResultCache::FindCachedResult(thread, cacheTable, str);
    if (cacheResult != JSTaggedValue::Undefined()) {
        JSHandle<JSTaggedValue> resultArray(JSArray::CreateArrayFromList(thread,
            JSHandle<TaggedArray>(thread, cacheResult)));
        return resultArray.GetTaggedValue();
    }

    JSTaggedValue newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    JSHandle<EcmaString> iteratedString(str);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> oldElements(thread, newArrayHandle->GetElements());
    uint32_t totalElements = EcmaStringAccessor(iteratedString).GetLength();
    JSHandle<TaggedArray> elements = (oldElements->GetLength() < totalElements) ?
        factory->ExtendArray(oldElements, totalElements) : oldElements;
    uint32_t index = 0;
    newArrayHandle->SetElements(thread, elements);
    while (index < totalElements) {
        uint16_t c = EcmaStringAccessor(iteratedString).Get(index);
        JSHandle<EcmaString> newStr = factory->NewFromUtf16Literal(&c, 1);
        ElementAccessor::Set(thread, newArrayHandle, index, newStr, true);
        index++;
        thread->CheckSafepointIfSuspended();
    }
    JSHandle<JSArray>(newArrayHandle)->SetArrayLength(thread, totalElements);

    StringToListResultCache::SetCachedResult(thread, cacheTable, str, elements);

    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue BuiltinsString::StringToSList(JSThread *thread, JSHandle<EcmaString> &str)
{
    JSHandle<StringToListResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetStringToListResultCache());
    JSTaggedValue cacheResult = StringToListResultCache::FindCachedResult(thread, cacheTable, str);
    if (cacheResult != JSTaggedValue::Undefined()) {
        JSHandle<JSTaggedValue> resultArray(
            JSSharedArray::CreateArrayFromList(thread, JSHandle<TaggedArray>(thread, cacheResult)));
        return resultArray.GetTaggedValue();
    }

    JSTaggedValue newSharedArray = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newSharedArrayHandle(thread, newSharedArray);
    JSHandle<EcmaString> iteratedString(str);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> oldElements(thread, newSharedArrayHandle->GetElements());
    uint32_t totalElements = EcmaStringAccessor(iteratedString).GetLength();
    JSHandle<TaggedArray> elements =
        (oldElements->GetLength() < totalElements)
            ? factory->ExtendArray(oldElements, totalElements, JSTaggedValue::Hole(), MemSpaceType::SHARED_OLD_SPACE)
            : oldElements;
    uint32_t index = 0;
    newSharedArrayHandle->SetElements(thread, elements);
    while (index < totalElements) {
        uint16_t c = EcmaStringAccessor(iteratedString).Get(index);
        JSHandle<EcmaString> newStr = factory->NewFromUtf16Literal(&c, 1);
        ElementAccessor::Set(thread, newSharedArrayHandle, index, newStr, true);
        index++;
        thread->CheckSafepointIfSuspended();
    }
    JSHandle<JSSharedArray>(newSharedArrayHandle)->SetArrayLength(thread, totalElements);

    StringToListResultCache::SetCachedResult(thread, cacheTable, str, elements);
    newSharedArrayHandle->GetJSHClass()->SetExtensible(false);
    return newSharedArrayHandle.GetTaggedValue();
}

JSTaggedValue StringSplitResultCache::CreateCacheTable(const JSThread *thread)
{
    int length = CACHE_SIZE * ENTRY_SIZE;
    auto table = static_cast<StringSplitResultCache*>(
        *thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length, JSTaggedValue::Undefined()));
    return JSTaggedValue(table);
}

JSTaggedValue StringSplitResultCache::FindCachedResult(const JSThread *thread,
    const JSHandle<StringSplitResultCache> &cache, const JSHandle<EcmaString> &thisString,
    const JSHandle<EcmaString> &pattern)
{
    uint32_t hash = EcmaStringAccessor(thisString).GetHashcode();
    uint32_t entry = hash & (CACHE_SIZE - 1);
    uint32_t index = entry * ENTRY_SIZE;
    JSTaggedValue cacheThis = cache->Get(index + STRING_INDEX);
    JSTaggedValue cachePattern = cache->Get(index + PATTERN_INDEX);
    if (!cacheThis.IsString() || !cachePattern.IsString()) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<EcmaString> cacheStringHandle(thread, cacheThis);
    JSHandle<EcmaString> cachePatternHandle(thread, cachePattern);

    if (EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), thisString, cacheStringHandle) &&
        EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), pattern, cachePatternHandle)) {
        return cache->Get(index + ARRAY_INDEX);
    }
    return JSTaggedValue::Undefined();
}

void StringSplitResultCache::SetCachedResult(const JSThread *thread, const JSHandle<StringSplitResultCache> &cache,
    const JSHandle<EcmaString> &thisString, const JSHandle<EcmaString> &pattern,
    const JSHandle<TaggedArray> &resultArray)
{
    // clone to cache array
    uint32_t arrayLength = resultArray->GetLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newElements(factory->NewCOWTaggedArray(arrayLength));
    for (uint32_t i = 0; i < arrayLength; i++) {
        newElements->Set(thread, i, resultArray->Get(i));
    }
    uint32_t hash = EcmaStringAccessor(thisString).GetHashcode();
    uint32_t entry = hash & (CACHE_SIZE - 1);
    uint32_t index = entry * ENTRY_SIZE;

    cache->Set(thread, index + STRING_INDEX, thisString);
    cache->Set(thread, index + PATTERN_INDEX, pattern);
    cache->Set(thread, index + ARRAY_INDEX, newElements);
}

JSTaggedValue StringToListResultCache::CreateCacheTable(const JSThread *thread)
{
    int length = CACHE_SIZE * ENTRY_SIZE;
    auto table = static_cast<StringToListResultCache*>(
        *thread->GetEcmaVM()->GetFactory()->NewTaggedArray(length, JSTaggedValue::Undefined()));
    return JSTaggedValue(table);
}

JSTaggedValue StringToListResultCache::FindCachedResult(const JSThread *thread,
    const JSHandle<StringToListResultCache> &cache, const JSHandle<EcmaString> &thisString)
{
    if (EcmaStringAccessor(thisString).GetLength() > MAX_STRING_LENGTH) {
        return JSTaggedValue::Undefined();
    }
    uint32_t hash = EcmaStringAccessor(thisString).GetHashcode();
    uint32_t entry = hash & (CACHE_SIZE - 1);
    uint32_t index = entry * ENTRY_SIZE;
    JSHandle<JSTaggedValue> cacheThis(thread, cache->Get(index + STRING_INDEX));
    if (!cacheThis->IsString()) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<EcmaString> cacheStr(cacheThis);
    if (EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), thisString, cacheStr)) {
        return cache->Get(index + ARRAY_INDEX);
    }
    return JSTaggedValue::Undefined();
}

void StringToListResultCache::SetCachedResult(const JSThread *thread, const JSHandle<StringToListResultCache> &cache,
    const JSHandle<EcmaString> &thisString, const JSHandle<TaggedArray> &resultArray)
{
    if (EcmaStringAccessor(thisString).GetLength() > MAX_STRING_LENGTH ||
        EcmaStringAccessor(thisString).GetLength() == 0) {
        return;
    }
    if (!EcmaStringAccessor(thisString).IsInternString()) {
        return;
    }
    // clone to cache array
    uint32_t arrayLength = resultArray->GetLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newElements;
    if (resultArray.GetTaggedValue().IsInSharedHeap()) {
        newElements = JSHandle<TaggedArray>(factory->NewSCOWTaggedArray(arrayLength));
    } else {
        newElements = JSHandle<TaggedArray>(factory->NewCOWTaggedArray(arrayLength));
    }
    for (uint32_t i = 0; i < arrayLength; i++) {
        newElements->Set(thread, i, resultArray->Get(i));
    }
    uint32_t hash = EcmaStringAccessor(thisString).GetHashcode();
    uint32_t entry = hash & (CACHE_SIZE - 1);
    uint32_t index = entry * ENTRY_SIZE;
    cache->Set(thread, index + STRING_INDEX, thisString);
    cache->Set(thread, index + ARRAY_INDEX, newElements);
}
}  // namespace panda::ecmascript::builtins
