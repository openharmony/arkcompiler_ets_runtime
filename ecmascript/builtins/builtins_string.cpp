/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <vector>
#include <map>

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_array.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_collator.h"
#include "ecmascript/js_locale.h"
#else
#ifndef ARK_NOT_SUPPORT_INTL_GLOBAL
#include "ecmascript/intl/global_intl_helper.h"
#endif
#endif

#include "unicode/normalizer2.h"
#include "unicode/normlzr.h"
#include "unicode/unistr.h"

namespace panda::ecmascript::builtins {
using ObjectFactory = ecmascript::ObjectFactory;
using JSArray = ecmascript::JSArray;

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
    std::u16string u16str = base::StringHelper::Utf16ToU16String(valueTable.data(), argLength);
    const char16_t *constChar16tData = u16str.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = u16str.size();
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
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
        if (cp == 0) {
            CVector<uint16_t> data {0x00};
            return factory->NewFromUtf16Literal(data.data(), 1).GetTaggedValue();
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
        pos = posVal.ToInt32();
    }
    if (pos < 0 || pos >= thisLen) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    uint16_t res = EcmaStringAccessor(thisFlat).Get<false>(pos);
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
    uint32_t pos = 0;
    JSHandle<JSTaggedValue> posTag = BuiltinsString::GetCallArg(argv, 1);
    if (posTag->IsUndefined()) {
        pos = thisLen;
    } else {
        JSTaggedNumber posVal = JSTaggedValue::ToInteger(thread, posTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        pos = static_cast<uint32_t>(posVal.ToInt32());
    }
    uint32_t end = std::min(std::max(pos, 0U), thisLen);
    int32_t start = static_cast<int32_t>(end - searchLen);
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
    }
    pos = std::min(std::max(pos, 0), thisLen);
    int32_t res = EcmaStringAccessor::LastIndexOf(thread->GetEcmaVM(), thisHandle, searchHandle, pos);
    if (res >= 0 && res < thisLen) {
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
    [[maybe_unused]] bool cacheable = (locales->IsUndefined() || locales->IsString()) && options->IsUndefined();
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto collator = JSCollator::GetCachedIcuCollator(thread, locales);
        if (collator != nullptr) {
            JSTaggedValue result = JSCollator::CompareStrings(collator, thisHandle, thatHandle);
            return result;
        }
    }
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
    JSTaggedValue result = JSCollator::CompareStrings(icuCollator, thisHandle, thatHandle);
    return result;
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
    JSHandle<JSTaggedValue> matchTag = thread->GetEcmaVM()->GetGlobalEnv()->GetMatchSymbol();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    if (regexp->IsJSRegExp()) {
        JSHandle<RegExpExecResultCache> cacheTable(thread->GetCurrentEcmaContext()->GetRegExpCache());
        JSHandle<JSRegExp> re(regexp);
        JSHandle<JSTaggedValue> pattern(thread, re->GetOriginalSource());
        JSHandle<JSTaggedValue> flags(thread, re->GetOriginalFlags());
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flags, thisTag,
                                                                 RegExpExecResultCache::MATCH_TYPE, regexp);
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }
    if (!regexp->IsUndefined() && !regexp->IsNull()) {
        if (regexp->IsECMAObject()) {
            JSHandle<JSTaggedValue> matcher = JSObject::GetMethod(thread, regexp, matchTag);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!matcher->IsUndefined()) {
                ASSERT(matcher->IsJSFunction());
                EcmaRuntimeCallInfo *info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, matcher, regexp, undefined, 1);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                info->SetCallArg(thisTag.GetTaggedValue());
                return JSFunction::Call(info);
            }
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
    JSHandle<JSTaggedValue> matchAllTag = thread->GetEcmaVM()->GetGlobalEnv()->GetMatchAllSymbol();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSTaggedValue> gvalue(factory->NewFromASCII("g"));

    // 2. If regexp is neither undefined nor null, then
    if (!regexp->IsUndefined() && !regexp->IsNull()) {
        // a. Let isRegExp be ? IsRegExp(searchValue).
        bool isJSRegExp = JSObject::IsRegExp(thread, regexp);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // b. If isRegExp is true, then
        if (isJSRegExp) {
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
                flagString, JSHandle<EcmaString>(gvalue));
            if (pos == -1) {
                THROW_TYPE_ERROR_AND_RETURN(thread,
                                            "matchAll called with a non-global RegExp argument",
                                            JSTaggedValue::Exception());
            }
        }

        if (regexp->IsECMAObject()) {
            // c. c. Let matcher be ? GetMethod(regexp, @@matchAll).
            // d. d. If matcher is not undefined, then
            JSHandle<JSTaggedValue> matcher = JSObject::GetMethod(thread, regexp, matchAllTag);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!matcher->IsUndefined()) {
                ASSERT(matcher->IsJSFunction());
                // i. i. Return ? Call(matcher, regexp, « O »).
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
    JSHandle<JSTaggedValue> rx(thread, BuiltinsRegExp::RegExpCreate(thread, regexp, gvalue));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, rx, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(thisVal.GetTaggedValue());
    return JSFunction::Invoke(info, matchAllTag);
}

// 21.1.3.12
JSTaggedValue BuiltinsString::Normalize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), String, Normalize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
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
    JSHandle<EcmaString> nfc = JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNfcString());
    JSHandle<EcmaString> nfd = factory->NewFromASCII("NFD");
    JSHandle<EcmaString> nfkc = factory->NewFromASCII("NFKC");
    JSHandle<EcmaString> nfkd = factory->NewFromASCII("NFKD");
    UNormalizationMode uForm;
    if (EcmaStringAccessor::StringsAreEqual(vm, formValue, nfc)) {
        uForm = UNORM_NFC;
    } else if (EcmaStringAccessor::StringsAreEqual(vm, formValue, nfd)) {
        uForm = UNORM_NFD;
    } else if (EcmaStringAccessor::StringsAreEqual(vm, formValue, nfkc)) {
        uForm = UNORM_NFKC;
    } else if (EcmaStringAccessor::StringsAreEqual(vm, formValue, nfkd)) {
        uForm = UNORM_NFKD;
    } else {
        THROW_RANGE_ERROR_AND_RETURN(thread, "compare not equal", JSTaggedValue::Exception());
    }

    std::u16string u16strThis = EcmaStringAccessor(thisHandle).ToU16String();
    const char16_t *constChar16tData = u16strThis.data();
    icu::UnicodeString src(constChar16tData);
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
        JSTaggedValue cacheResult = cacheTable->FindCachedResult(thread, pattern, flags, thisTag,
                                                                 RegExpExecResultCache::REPLACE_TYPE, searchTag,
                                                                 replaceTag.GetTaggedValue());
        if (!cacheResult.IsUndefined()) {
            return cacheResult;
        }
    }

    // If searchValue is neither undefined nor null, then
    if (searchTag->IsECMAObject()) {
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
    std::u16string stringBuilder;
    std::u16string stringPrefixString;
    std::u16string stringRealReplaceStr;
    std::u16string stringSuffixString;
    // 10. Let position be ! StringIndexOf(string, searchString, 0).
    int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, searchString);
    int32_t endOfLastMatch = 0;
    bool canBeCompress = true;
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
        stringPrefixString = EcmaStringAccessor(prefixString).ToU16String();
        if (EcmaStringAccessor(prefixString).IsUtf16()) {
            canBeCompress = false;
        }
        stringRealReplaceStr = EcmaStringAccessor(realReplaceStr).ToU16String();
        if (EcmaStringAccessor(realReplaceStr).IsUtf16()) {
            canBeCompress = false;
        }
        stringBuilder = stringBuilder + stringPrefixString + stringRealReplaceStr;
        endOfLastMatch = pos + searchLength;
        pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, searchString, pos + advanceBy);
    }

    if (endOfLastMatch < static_cast<int32_t>(EcmaStringAccessor(thisString).GetLength())) {
        auto thisLen = EcmaStringAccessor(thisString).GetLength();
        JSHandle<EcmaString> suffixString(thread,
            EcmaStringAccessor::FastSubString(ecmaVm, thisString, endOfLastMatch, thisLen - endOfLastMatch));
        stringSuffixString = EcmaStringAccessor(suffixString).ToU16String();
        if (EcmaStringAccessor(suffixString).IsUtf16()) {
            canBeCompress = false;
        }
        stringBuilder = stringBuilder + stringSuffixString;
    }

    auto *char16tData = const_cast<char16_t *>(stringBuilder.c_str());
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    return canBeCompress ?
        factory->NewFromUtf16LiteralCompress(uint16tData, stringBuilder.length()).GetTaggedValue() :
        factory->NewFromUtf16LiteralNotCompress(uint16tData, stringBuilder.length()).GetTaggedValue();
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
                stringBuilder += EcmaStringAccessor(matched).ToU16String();
                if (EcmaStringAccessor(matched).IsUtf16()) {
                    canBeCompress = false;
                }
                continueFromIndex = peekIndex + 1;
                break;
            case '`':  // $` - prefix
                if (position > 0) {
                    EcmaString *prefix = EcmaStringAccessor::FastSubString(ecmaVm, srcString, 0, position);
                    stringBuilder += EcmaStringAccessor(prefix).ToU16String();
                    if (EcmaStringAccessor(prefix).IsUtf16()) {
                        canBeCompress = false;
                    }
                }
                continueFromIndex = peekIndex + 1;
                break;
            case '\'': {
                // $' - suffix
                int32_t srcLength = static_cast<int32_t>(EcmaStringAccessor(srcString).GetLength());
                if (tailPos < srcLength) {
                    EcmaString *sufffix = EcmaStringAccessor::FastSubString(
                        ecmaVm, srcString, tailPos, srcLength - tailPos);
                    stringBuilder += EcmaStringAccessor(sufffix).ToU16String();
                    if (EcmaStringAccessor(sufffix).IsUtf16()) {
                        canBeCompress = false;
                    }
                }
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
                uint32_t capturesLength = captureList->GetLength();
                // Valid indices are $1 .. $9, $01 .. $09 and $10 .. $99
                uint32_t scaledIndex = peek - '0';
                int32_t advance = 1;
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
                    continueFromIndex = peekIndex;
                    break;
                }

                JSTaggedValue capturesVal(captureList->Get(scaledIndex - 1));
                if (!capturesVal.IsUndefined()) {
                    EcmaString *captureString = EcmaString::Cast(capturesVal.GetTaggedObject());
                    stringBuilder += EcmaStringAccessor(captureString).ToU16String();
                    if (EcmaStringAccessor(captureString).IsUtf16()) {
                        canBeCompress = false;
                    }
                }
                continueFromIndex = peekIndex + advance;
                break;
            }
            case '<': {
                if (namedCaptures->IsUndefined()) {
                    stringBuilder += '$';
                    continueFromIndex = peekIndex;
                    break;
                }
                JSHandle<EcmaString> greaterSymString = factory->NewFromASCII(">");
                int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, replacementFlat, greaterSymString, peekIndex);
                if (pos == -1) {
                    stringBuilder += '$';
                    continueFromIndex = peekIndex;
                    break;
                }
                JSHandle<EcmaString> groupName(thread,
                                               EcmaStringAccessor::FastSubString(ecmaVm, replacementFlat,
                                                                                 peekIndex + 1, pos - peekIndex - 1));
                JSHandle<JSTaggedValue> names(groupName);
                JSHandle<JSTaggedValue> capture = JSObject::GetProperty(thread, namedCaptures, names).GetValue();
                if (capture->IsUndefined()) {
                    continueFromIndex = pos + 1;
                    break;
                }
                JSHandle<EcmaString> captureName(capture);
                stringBuilder += EcmaStringAccessor(captureName).ToU16String();
                if (EcmaStringAccessor(captureName).IsUtf16()) {
                    canBeCompress = false;
                }
                continueFromIndex = pos + 1;
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
    JSHandle<JSTaggedValue> searchTag = thread->GetEcmaVM()->GetGlobalEnv()->GetSearchSymbol();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    if (!regexp->IsUndefined() && !regexp->IsNull()) {
        if (regexp->IsECMAObject()) {
            JSHandle<JSTaggedValue> searcher = JSObject::GetMethod(thread, regexp, searchTag);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!searcher->IsUndefined()) {
                ASSERT(searcher->IsJSFunction());
                EcmaRuntimeCallInfo *info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, searcher, regexp, undefined, 1);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                info->SetCallArg(thisTag.GetTaggedValue());
                return JSFunction::Call(info);
            }
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
    int32_t start = ConvertDoubleToInt(startVal.GetNumber());
    int32_t end = 0;
    JSHandle<JSTaggedValue> endTag = BuiltinsString::GetCallArg(argv, 1);
    if (endTag->IsUndefined()) {
        end = thisLen;
    } else {
        JSTaggedNumber endVal = JSTaggedValue::ToInteger(thread, endTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = ConvertDoubleToInt(endVal.GetNumber());
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
    JSHandle<JSObject> thisObj(thisTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> seperatorTag = BuiltinsString::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> limitTag = BuiltinsString::GetCallArg(argv, 1);
    // If separator is neither undefined nor null, then
    if (seperatorTag->IsECMAObject()) {
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
    // Let A be ArrayCreate(0).
    JSHandle<JSObject> resultArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t arrayLength = 0;
    // If limit is undefined, let lim = 2^53–1; else let lim = ToLength(limit).
    uint32_t lim = 0;
    if (limitTag->IsUndefined()) {
        lim = UINT32_MAX - 1;
    } else {
        lim = JSTaggedValue::ToInteger(thread, limitTag).ToUint32();
    }
    // ReturnIfAbrupt(lim).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If lim = 0, return A.
    if (lim == 0) {
        return resultArray.GetTaggedValue();
    }
    // Let s be the number of elements in S.
    int32_t thisLength = static_cast<int32_t>(EcmaStringAccessor(thisString).GetLength());
    JSHandle<EcmaString> seperatorString = JSTaggedValue::ToString(thread, seperatorTag);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (seperatorTag->IsUndefined()) {
        // Perform CreateDataProperty(A, "0", S).
        JSObject::CreateDataProperty(thread, resultArray, 0, JSHandle<JSTaggedValue>(thisString));
        ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty(A, \"0\", S) can't throw exception");
        return resultArray.GetTaggedValue();
    }
    // If S.length = 0, then
    if (thisLength == 0) {
        if (EcmaStringAccessor::IndexOf(ecmaVm, thisString, seperatorString) != -1) {
            return resultArray.GetTaggedValue();
        }
        JSObject::CreateDataProperty(thread, resultArray, 0, JSHandle<JSTaggedValue>(thisString));
        ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty(A, \"0\", S) can't throw exception");
        return resultArray.GetTaggedValue();
    }

    int32_t seperatorLength = static_cast<int32_t>(EcmaStringAccessor(seperatorString).GetLength());
    if (seperatorLength == 0) {
        for (int32_t i = 0; i < thisLength; ++i) {
            EcmaString *elementString = EcmaStringAccessor::FastSubString(ecmaVm, thisString, i, 1);
            JSHandle<JSTaggedValue> elementTag(thread, elementString);
            JSObject::CreateDataProperty(thread, resultArray, arrayLength, elementTag);
            ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty can't throw exception");
            ++arrayLength;
            if (arrayLength == lim) {
                return resultArray.GetTaggedValue();
            }
        }
        return resultArray.GetTaggedValue();
    }
    int32_t index = 0;
    int32_t pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, seperatorString);
    while (pos != -1) {
        EcmaString *elementString = EcmaStringAccessor::FastSubString(ecmaVm, thisString, index, pos - index);
        JSHandle<JSTaggedValue> elementTag(thread, elementString);
        JSObject::CreateDataProperty(thread, resultArray, arrayLength, elementTag);
        ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty can't throw exception");
        ++arrayLength;
        if (arrayLength == lim) {
            return resultArray.GetTaggedValue();
        }
        index = pos + seperatorLength;
        pos = EcmaStringAccessor::IndexOf(ecmaVm, thisString, seperatorString, index);
    }
    EcmaString *elementString = EcmaStringAccessor::FastSubString(ecmaVm, thisString, index, thisLength - index);
    JSHandle<JSTaggedValue> elementTag(thread, elementString);
    JSObject::CreateDataProperty(thread, resultArray, arrayLength, elementTag);
    ASSERT_PRINT(!thread->HasPendingException(), "CreateDataProperty can't throw exception");
    return resultArray.GetTaggedValue();
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
        pos = posVal.ToInt32();
    }
    pos = std::min(std::max(pos, 0), static_cast<int32_t>(thisLen));
    if (static_cast<uint32_t>(pos) + searchLen > thisLen) {
        return BuiltinsString::GetTaggedBoolean(false);
    }
    int32_t res = EcmaStringAccessor::IndexOf(thread->GetEcmaVM(), thisHandle, searchHandle, pos);
    if (res == pos) {
        return BuiltinsString::GetTaggedBoolean(true);
    }
    return BuiltinsString::GetTaggedBoolean(false);
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
    int32_t start = ConvertDoubleToInt(startVal.GetNumber());
    int32_t end = 0;
    JSHandle<JSTaggedValue> endTag = BuiltinsString::GetCallArg(argv, 1);
    if (endTag->IsUndefined()) {
        end = thisLen;
    } else {
        JSTaggedNumber endVal = JSTaggedValue::ToInteger(thread, endTag);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = ConvertDoubleToInt(endVal.GetNumber());
    }
    start = std::min(std::max(start, 0), thisLen);
    end = std::min(std::max(end, 0), thisLen);
    int32_t from = std::min(start, end);
    int32_t to = std::max(start, end);
    int32_t len = to - from;
    return JSTaggedValue(EcmaStringAccessor::FastSubString(thread->GetEcmaVM(), thisHandle, from, len));
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
    int32_t start = numStart.ToInt32();
    JSHandle<JSTaggedValue> lengthTag = GetCallArg(argv, 1);
    // 6. If length is undefined, let end be +; otherwise let end be ToInteger(length).
    int32_t end = 0;
    if (lengthTag->IsUndefined()) {
        end = INT_MAX;
    } else {
        JSTaggedNumber lengthNumber = JSTaggedValue::ToInteger(thread, lengthTag);
        // 7. ReturnIfAbrupt(end).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = lengthNumber.ToInt32();
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
    BUILTINS_API_TRACE(argv->GetThread(), String, Substring);
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
    int32_t relativeIndex = ConvertDoubleToInt(indexVal.GetNumber());

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
    int32_t intMaxLength = JSTaggedValue::ToInt32(thread, lengthTag);
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
    int32_t fillLen = intMaxLength - stringLength;
    int32_t len = static_cast<int32_t>(stringBuilder.length());
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

int32_t BuiltinsString::ConvertDoubleToInt(double d)
{
    if (std::isnan(d) || d == -base::POSITIVE_INFINITY) {
        return 0;
    }
    if (d >= static_cast<double>(INT_MAX)) {
        return INT_MAX;
    }
    if (d <= static_cast<double>(INT_MIN)) {
        return INT_MIN;
    }
    return base::NumberHelper::DoubleToInt(d, base::INT32_BITS);
}
}  // namespace panda::ecmascript::builtins
