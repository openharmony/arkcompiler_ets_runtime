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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_STRING_H
#define ECMASCRIPT_BUILTINS_BUILTINS_STRING_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"

// List of functions in String, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsString::func refers to the native implementation of String[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_STRING_FUNCTIONS(V)                             \
    /* String.fromCharCode ( ...codeUnits ) */                  \
    V("fromCharCode",  FromCharCode,  1, StringFromCharCode)    \
    /* String.fromCodePoint ( ...codePoints ) */                \
    V("fromCodePoint", FromCodePoint, 1, INVALID)               \
    /* String.raw ( template, ...substitutions ) */             \
    V("raw",           Raw,           1, INVALID)

// List of functions in String.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsString::func refers to the native implementation of String.prototype[name].
// The following functions in String.prototype are not implemented yet:
//   - String.prototype.isWellFormed ( )
//   - String.prototype.toWellFormed ( )
#define BUILTIN_STRING_PROTOTYPE_FUNCTIONS(V)                                       \
    /* String.prototype.at ( index ) */                                             \
    V("at",                At,                1, INVALID)                           \
    /* String.prototype.charAt ( pos ) */                                           \
    V("charAt",            CharAt,            1, StringCharAt)                      \
    /* String.prototype.charCodeAt ( pos ) */                                       \
    V("charCodeAt",        CharCodeAt,        1, StringCharCodeAt)                  \
    /* String.prototype.codePointAt ( pos ) */                                      \
    V("codePointAt",       CodePointAt,       1, StringCodePointAt)                 \
    /* String.prototype.concat ( ...args ) */                                       \
    V("concat",            Concat,            1, StringConcat)                      \
    /* String.prototype.endsWith ( searchString [ , endPosition ] ) */              \
    V("endsWith",          EndsWith,          1, StringEndsWith)                    \
    /* String.prototype.includes ( searchString [ , position ] ) */                 \
    V("includes",          Includes,          1, INVALID)                           \
    /* String.prototype.indexOf ( searchString [ , position ] ) */                  \
    V("indexOf",           IndexOf,           1, StringIndexOf)                     \
    /* String.prototype.lastIndexOf ( searchString [ , position ] ) */              \
    V("lastIndexOf",       LastIndexOf,       1, INVALID)                           \
    /* String.prototype.localeCompare ( that [ , reserved1 [ , reserved2 ] ] ) */   \
    V("localeCompare",     LocaleCompare,     1, StringLocaleCompare)               \
    /* String.prototype.match ( regexp ) */                                         \
    V("match",             Match,             1, INVALID)                           \
    /* String.prototype.matchAll ( regexp ) */                                      \
    V("matchAll",          MatchAll,          1, INVALID)                           \
    /* String.prototype.normalize ( [ form ] ) */                                   \
    V("normalize",         Normalize,         0, INVALID)                           \
    /* String.prototype.isWellFormed ( ) */                                         \
    V("isWellFormed",      IsWellFormed,      0, INVALID)                           \
    /* String.prototype.toWellFormed ( ) */                                         \
    V("toWellFormed",      ToWellFormed,      0, INVALID)                           \
    /* String.prototype.padEnd ( maxLength [ , fillString ] ) */                    \
    V("padEnd",            PadEnd,            1, StringPadEnd)                      \
    /* String.prototype.padStart ( maxLength [ , fillString ] ) */                  \
    V("padStart",          PadStart,          1, StringPadStart)                    \
    /* String.prototype.repeat ( count ) */                                         \
    V("repeat",            Repeat,            1, INVALID)                           \
    /* String.prototype.replace ( searchValue, replaceValue ) */                    \
    V("replace",           Replace,           2, StringReplace)                     \
    /* String.prototype.replaceAll ( searchValue, replaceValue ) */                 \
    V("replaceAll",        ReplaceAll,        2, INVALID)                           \
    /* String.prototype.search ( regexp ) */                                        \
    V("search",            Search,            1, INVALID)                           \
    /* String.prototype.slice ( start, end ) */                                     \
    V("slice",             Slice,             2, StringSlice)                       \
    /* String.prototype.split ( separator, limit ) */                               \
    V("split",             Split,             2, INVALID)                           \
    /* String.prototype.startsWith ( searchString [ , position ] ) */               \
    V("startsWith",        StartsWith,        1, StringStartsWith)                  \
    /* In Annex B.2.2: Additional Properties of the String.prototype Object */      \
    /* String.prototype.substr ( start, length ) */                                 \
    V("substr",            SubStr,            2, StringSubStr)                      \
    /* String.prototype.substring ( start, end ) */                                 \
    V("substring",         Substring,         2, StringSubstring)                   \
    /* String.prototype.toLocaleLowerCase ( [ reserved1 [ , reserved2 ] ] ) */      \
    V("toLocaleLowerCase", ToLocaleLowerCase, 0, INVALID)                           \
    /* String.prototype.toLocaleUpperCase ( [ reserved1 [ , reserved2 ] ] ) */      \
    V("toLocaleUpperCase", ToLocaleUpperCase, 0, INVALID)                           \
    /* String.prototype.toLowerCase ( ) */                                          \
    V("toLowerCase",       ToLowerCase,       0, StringToLowerCase)                 \
    /* String.prototype.toString ( ) */                                             \
    V("toString",          ToString,          0, INVALID)                           \
    /* String.prototype.toUpperCase ( ) */                                          \
    V("toUpperCase",       ToUpperCase,       0, INVALID)                           \
    /* String.prototype.trim ( ) */                                                 \
    V("trim",              Trim,              0, StringTrim)                        \
    /* String.prototype.trimEnd ( ) */                                              \
    V("trimEnd",           TrimEnd,           0, StringTrimEnd)                     \
    /* In Annex B.2.2: Additional Properties of the String.prototype Object */      \
    /* Equivalent to trimStart. For compatibility only. */                          \
    /* String.prototype.trimLeft ( ) */                                             \
    V("trimLeft",          TrimLeft,          0, StringTrimLeft)                    \
    /* In Annex B.2.2: Additional Properties of the String.prototype Object */      \
    /* Equivalent to trimEnd. For compatibility only. */                            \
    /* String.prototype.trimEnd ( ) */                                              \
    V("trimRight",         TrimRight,         0, StringTrimRight)                   \
    /* String.prototype.trimStart ( ) */                                            \
    V("trimStart",         TrimStart,         0, StringTrimStart)                   \
    /* String.prototype.valueOf ( ) */                                              \
    V("valueOf",           ValueOf,           0, INVALID)

namespace panda::ecmascript {
enum class CompareStringsOption : uint8_t;
}

namespace panda::ecmascript::builtins {
constexpr int32_t ENCODE_MAX_UTF16 = 0X10FFFF;
constexpr uint16_t ENCODE_LEAD_LOW = 0xD800;
constexpr uint16_t ENCODE_TRAIL_LOW = 0xDC00;
constexpr uint32_t ENCODE_FIRST_FACTOR = 0x400;
constexpr uint32_t ENCODE_SECOND_FACTOR = 0x10000;
constexpr double DOUBLE_INT_MAX = static_cast<double>(INT_MAX);
constexpr double DOUBLE_INT_MIN = static_cast<double>(INT_MIN);

class BuiltinsString : public base::BuiltinsBase {
public:
    // 21.1.1.1
    static JSTaggedValue StringConstructor(EcmaRuntimeCallInfo *argv);
    // 21.1.2.1
    static JSTaggedValue FromCharCode(EcmaRuntimeCallInfo *argv);
    // 21.1.2.2
    static JSTaggedValue FromCodePoint(EcmaRuntimeCallInfo *argv);
    // 21.1.2.4
    static JSTaggedValue Raw(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetSubstitution(JSThread *thread, const JSHandle<EcmaString> &matched,
                                         const JSHandle<EcmaString> &srcString, int position,
                                         const JSHandle<TaggedArray> &captureList,
                                         const JSHandle<JSTaggedValue> &namedCaptures,
                                         const JSHandle<EcmaString> &replacement);
    // 21.1.3.1
    static JSTaggedValue CharAt(EcmaRuntimeCallInfo *argv);
    // 21.1.3.2
    static JSTaggedValue CharCodeAt(EcmaRuntimeCallInfo *argv);
    // 21.1.3.3
    static JSTaggedValue CodePointAt(EcmaRuntimeCallInfo *argv);
    // 21.1.3.4
    static JSTaggedValue Concat(EcmaRuntimeCallInfo *argv);
    // 21.1.3.5 String.prototype.constructor
    // 21.1.3.6
    static JSTaggedValue EndsWith(EcmaRuntimeCallInfo *argv);
    // 21.1.3.7
    static JSTaggedValue Includes(EcmaRuntimeCallInfo *argv);
    // 21.1.3.8
    static JSTaggedValue IndexOf(EcmaRuntimeCallInfo *argv);
    // 21.1.3.9
    static JSTaggedValue LastIndexOf(EcmaRuntimeCallInfo *argv);
    // 21.1.3.10
    static JSTaggedValue LocaleCompare(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue DoLocaleCompare(JSThread *thread,
                                         const JSHandle<EcmaString> &thisHandle,
                                         const JSHandle<EcmaString> &thatHandle,
                                         const JSHandle<JSTaggedValue> &locales,
                                         const JSHandle<JSTaggedValue> &options);
    static JSTaggedValue LocaleCompareGC(JSThread *thread,
                                         const JSHandle<EcmaString> &thisHandle,
                                         const JSHandle<EcmaString> &thatHandle,
                                         const JSHandle<JSTaggedValue> &locales,
                                         const JSHandle<JSTaggedValue> &options,
                                         CompareStringsOption csOption,
                                         bool cacheable);
    // 21.1.3.11
    static JSTaggedValue Match(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue MatchAll(EcmaRuntimeCallInfo *argv);
    // 21.1.3.12
    static JSTaggedValue Normalize(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue IsWellFormed(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ToWellFormed(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue PadStart(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue PadEnd(EcmaRuntimeCallInfo *argv);
    // 21.1.3.13
    static JSTaggedValue Repeat(EcmaRuntimeCallInfo *argv);
    // 21.1.3.14
    static JSTaggedValue Replace(EcmaRuntimeCallInfo *argv);
    // 21.1.3.14.1 Runtime Semantics: GetSubstitution()
    static JSTaggedValue ReplaceAll(EcmaRuntimeCallInfo *argv);
    // 21.1.3.15
    static JSTaggedValue Search(EcmaRuntimeCallInfo *argv);
    // 21.1.3.16
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 21.1.3.17
    static JSTaggedValue Split(EcmaRuntimeCallInfo *argv);
    // 21.1.3.18
    static JSTaggedValue StartsWith(EcmaRuntimeCallInfo *argv);
    // 21.1.3.19
    static JSTaggedValue Substring(EcmaRuntimeCallInfo *argv);
    // 21.1.3.20
    static JSTaggedValue ToLocaleLowerCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.21
    static JSTaggedValue ToLocaleUpperCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.22
    static JSTaggedValue ToLowerCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.23
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 21.1.3.24
    static JSTaggedValue ToUpperCase(EcmaRuntimeCallInfo *argv);
    // 21.1.3.25
    static JSTaggedValue Trim(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimStart(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimEnd(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimLeft(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue TrimRight(EcmaRuntimeCallInfo *argv);
    // 21.1.3.26
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);
    // 21.1.3.27
    static JSTaggedValue GetStringIterator(EcmaRuntimeCallInfo *argv);
    // 21.1.3
    static JSTaggedValue ThisStringValue(JSThread *thread, JSTaggedValue value);
    // 21.1.2.27
    static JSTaggedValue CreateIterator(EcmaRuntimeCallInfo *argv);
    // 10.1.2
    static uint16_t UTF16Decode(uint16_t lead, uint16_t trail);
    // annexB B.2.3.1
    static JSTaggedValue SubStr(EcmaRuntimeCallInfo *argv);
    // 22.1.3.1
    static JSTaggedValue At(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetLength(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetStringFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(STRING_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetStringPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(STRING_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 3 : 3 more inline properties in String.prototype:
        //   (1) String.prototype.constructor
        //   (2) String.prototype [ @@iterator ]
        //   (3) get length
        return GetStringPrototypeFunctions().Size() + 3;
    }
    static JSTaggedValue StringToList(JSThread *thread, JSHandle<EcmaString> &str);
    static JSTaggedValue StringToSList(JSThread *thread, JSHandle<EcmaString> &str);

private:
#define BUILTIN_STRING_FUNCTION_ENTRY(name, method, length, builtinId) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsString::method, length, kungfu::BuiltinsStubCSigns::builtinId),

    static constexpr std::array STRING_FUNCTIONS = {
        BUILTIN_STRING_FUNCTIONS(BUILTIN_STRING_FUNCTION_ENTRY)
    };
    static constexpr std::array STRING_PROTOTYPE_FUNCTIONS = {
        BUILTIN_STRING_PROTOTYPE_FUNCTIONS(BUILTIN_STRING_FUNCTION_ENTRY)
    };
#undef BUILTIN_STRING_FUNCTION_ENTRY

    static JSTaggedValue Pad(EcmaRuntimeCallInfo *argv, bool isStart);
    static JSTaggedValue CreateArrayFromString(JSThread *thread, EcmaVM *ecmaVm,
        const JSHandle<EcmaString> &thisString, uint32_t thisLength, uint32_t lim = UINT32_MAX - 1);
    static JSTaggedValue CreateArrayBySplitString(JSThread *thread, EcmaVM *ecmaVm,
        const JSHandle<EcmaString> &thisString, const JSHandle<EcmaString> &seperatorString,
        uint32_t thisLength, uint32_t seperatorLength, uint32_t lim);
    static JSTaggedValue CreateArrayThisStringAndSeperatorStringAreNotEmpty(
        JSThread *thread, EcmaVM *ecmaVm,
        const JSHandle<EcmaString> &thisString, const JSHandle<EcmaString> &seperatorString,
        uint32_t thisLength, uint32_t seperatorLength, uint32_t lim = UINT32_MAX - 1);
    static bool IsUTF16HighSurrogate(uint16_t ch)
    {
        return base::utf_helper::DECODE_LEAD_LOW <= ch && ch <= base::utf_helper::DECODE_LEAD_HIGH;
    }
    static bool IsUTF16LowSurrogate(uint16_t ch)
    {
        return base::utf_helper::DECODE_TRAIL_LOW <= ch && ch <= base::utf_helper::DECODE_TRAIL_HIGH;
    }
    static uint32_t UTF16SurrogatePairToCodePoint(uint16_t lead, uint16_t trail);
    // 21.1.3.17.1
};

class StringSplitResultCache : public TaggedArray {
public:
    static StringSplitResultCache *Cast(TaggedObject *object)
    {
        return reinterpret_cast<StringSplitResultCache*>(object);
    }
    static JSTaggedValue CreateCacheTable(const JSThread *thread);
    static JSTaggedValue FindCachedResult(const JSThread *thread, const JSHandle<StringSplitResultCache> &cache,
        const JSHandle<EcmaString> &string, const JSHandle<EcmaString> &pattern);
    static void SetCachedResult(const JSThread *thread, const JSHandle<StringSplitResultCache> &cache,
        const JSHandle<EcmaString> &string, const JSHandle<EcmaString> &pattern,
        const JSHandle<TaggedArray> &result);

private:
    static constexpr int CACHE_SIZE = 256;
    static constexpr int STRING_INDEX = 0;
    static constexpr int PATTERN_INDEX = 1;
    static constexpr int ARRAY_INDEX = 2;
    static constexpr int ENTRY_SIZE = 3;
};

class StringToListResultCache : public TaggedArray {
public:
    static StringToListResultCache *Cast(TaggedObject *object)
    {
        return reinterpret_cast<StringToListResultCache*>(object);
    }
    static JSTaggedValue CreateCacheTable(const JSThread *thread);
    static JSTaggedValue FindCachedResult(const JSThread *thread, const JSHandle<StringToListResultCache> &cache,
        const JSHandle<EcmaString> &string);
    static void SetCachedResult(const JSThread *thread, const JSHandle<StringToListResultCache> &cache,
        const JSHandle<EcmaString> &string, const JSHandle<TaggedArray> &result);

    static constexpr int MAX_STRING_LENGTH = 20;
    static constexpr int CACHE_SIZE = 128;
    static constexpr int STRING_INDEX = 0;
    static constexpr int ARRAY_INDEX = 1;
    static constexpr int ENTRY_SIZE = 2;
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_STRING_H
