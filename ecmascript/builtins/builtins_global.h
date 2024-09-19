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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_GLOBAL_H
#define ECMASCRIPT_BUILTINS_BUILTINS_GLOBAL_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_thread.h"

#define BUILTIN_GLOBAL_CONSTANTS(V)     \
    V("Infinity",  INFINITY_VALUE)      \
    V("NaN",       NAN_VALUE)           \
    V("undefined", UNDEFINED_VALUE)

// List of functions in the global object.
// V(name, func, length, stubIndex)
// where BuiltinsGlobal::func refers to the native implementation of globalThis[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
// The following global object properties are not implemented yet:
//   - Encode ( string, extraUnescaped )
//   - Decode ( string, preserveEscapeSet )
//   - ParseHexOctet ( string, position )
// The following global object properties are not listed here:
//   - parseFloat ( string ), listed in builtins_number.h instead.
//   - parseInt ( string ), listed in builtins_number.h instead.
#define BUILTIN_GLOBAL_FUNCTIONS_COMMON(V)                                            \
    /* decodeURI ( encodedURI ) */                                                    \
    V("decodeURI",                DecodeURI,             1, INVALID)                  \
    /* decodeURIComponent ( encodedURIComponent ) */                                  \
    V("decodeURIComponent",       DecodeURIComponent,    1, GlobalDecodeURIComponent) \
    /* encodeURI ( uri ) */                                                           \
    V("encodeURI",                EncodeURI,             1, INVALID)                  \
    /* encodeURIComponent ( uriComponent ) */                                         \
    V("encodeURIComponent",       EncodeURIComponent,    1, INVALID)                  \
    /* escape ( string ), defined in B.2.1 */                                         \
    V("escape",                   Escape,                1, INVALID)                  \
    /* eval ( x ), which is NOT supported in ArkTS engine */                          \
    V("eval",                     NotSupportEval,        1, INVALID)                  \
    /* isFinite ( number ) */                                                         \
    V("isFinite",                 IsFinite,              1, GlobalIsFinite)           \
    /* isNaN ( number ) */                                                            \
    V("isNaN",                    IsNaN,                 1, GlobalIsNan)              \
    /* unescape ( string )*/                                                          \
    V("unescape",                 Unescape,              1, INVALID)                  \
    /* The following are ArkTS extensions */                                          \
    V("markModuleCollectable",    MarkModuleCollectable, 0, INVALID)                  \
    V("loadNativeModule",         LoadNativeModule,      0, INVALID)                  \
    V("print",                    PrintEntrypoint,       0, INVALID)                  \
    V("isSendable",               IsSendable,            0, INVALID)                  \
    V("__getCurrentModuleName__", GetCurrentModuleName,  0, INVALID)                  \
    V("__getCurrentBundleName__", GetCurrentBundleName,  0, INVALID)
#if ECMASCRIPT_ENABLE_RUNTIME_STAT
#define BUILTIN_GLOBAL_FUNCTIONS_RUNTIME_STAT(V)        \
    V("startRuntimeStat", StartRuntimeStat, 0, INVALID) \
    V("stopRuntimeStat",  StopRuntimeStat,  0, INVALID)
#else
#define BUILTIN_GLOBAL_FUNCTIONS_RUNTIME_STAT(V) // Nothing
#endif

#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
#define BUILTIN_GLOBAL_FUNCTIONS_OPT_CODE_PROFILER(V)   \
    V("printOptStat", PrintOptStat, 0, INVALID)
#else
#define BUILTIN_GLOBAL_FUNCTIONS_OPT_CODE_PROFILER(V) // Nothing
#endif

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
#define BUILTIN_GLOBAL_FUNCTIONS_FUNCTION_CALL_TIMER(V) \
    V("printFunctionCallStat", PrintFunctionCallStat, 0, INVALID)
#else
#define BUILTIN_GLOBAL_FUNCTIONS_FUNCTION_CALL_TIMER(V) // Nothing
#endif

#define BUILTIN_GLOBAL_FUNCTIONS(V)                     \
    BUILTIN_GLOBAL_FUNCTIONS_COMMON(V)                  \
    BUILTIN_GLOBAL_FUNCTIONS_RUNTIME_STAT(V)            \
    BUILTIN_GLOBAL_FUNCTIONS_OPT_CODE_PROFILER(V)       \
    BUILTIN_GLOBAL_FUNCTIONS_FUNCTION_CALL_TIMER(V)

namespace panda::ecmascript::builtins {
static constexpr uint8_t BIT_MASK = 0x0F;
static constexpr uint8_t BIT_MASK_FF = 0xFF;
static constexpr uint16_t BIT_MASK_4F = 0xFFFF;
static constexpr uint16_t BIT16_MASK = 0x3FF;
static constexpr uint8_t BIT_MASK_ONE = 0x80;
static constexpr uint8_t BIT_MASK_TWO = 0xC0;
using judgURIFunc = bool (*)(uint16_t);

enum class Placement {
    START = 0,
    END,
};

class BuiltinsGlobal : public base::BuiltinsBase {
public:
    static const inline JSTaggedValue INFINITY_VALUE = JSTaggedValue(base::POSITIVE_INFINITY);
    static const inline JSTaggedValue NAN_VALUE = JSTaggedValue(base::NAN_VALUE);
    static const inline JSTaggedValue UNDEFINED_VALUE = JSTaggedValue::Undefined();

    // 18.2.1
    static JSTaggedValue NotSupportEval(EcmaRuntimeCallInfo *msg);
    // 18.2.2
    static JSTaggedValue IsFinite(EcmaRuntimeCallInfo *msg);
    // 18.2.3
    static JSTaggedValue IsNaN(EcmaRuntimeCallInfo *msg);
    // 18.2.6
    static JSTaggedValue DecodeURI(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue EncodeURI(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue DecodeURIComponent(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue EncodeURIComponent(EcmaRuntimeCallInfo *msg);

    static JSTaggedValue PrintEntrypoint(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue MarkModuleCollectable(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue LoadNativeModule(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue CallJsBoundFunction(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue CallJsProxy(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue IsSendable(EcmaRuntimeCallInfo *msg);

    static JSTaggedValue GetCurrentModuleName(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue GetCurrentBundleName(EcmaRuntimeCallInfo *msg);
#if ECMASCRIPT_ENABLE_RUNTIME_STAT
    static JSTaggedValue StartRuntimeStat(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue StopRuntimeStat(EcmaRuntimeCallInfo *msg);
#endif

#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
    static JSTaggedValue PrintOptStat(EcmaRuntimeCallInfo *msg);
#endif

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    static JSTaggedValue PrintFunctionCallStat(EcmaRuntimeCallInfo *msg);
#endif
    // B.2.1.1 escape ( string )
    static JSTaggedValue Escape(EcmaRuntimeCallInfo *msg);
    // B.2.1.2 unescape ( string )
    static JSTaggedValue Unescape(EcmaRuntimeCallInfo *msg);

    static Span<const base::BuiltinConstantEntry> GetGlobalConstants()
    {
        return Span<const base::BuiltinConstantEntry>(GLOBAL_CONSTANTS);
    }

    static Span<const base::BuiltinFunctionEntry> GetGlobalFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(GLOBAL_FUNCTIONS);
    }

private:
#define BUILTIN_GLOBAL_CONSTANT_ENTRY(name, var) \
    base::BuiltinConstantEntry::Create(name, BuiltinsGlobal::var),
#define BUILTIN_GLOBAL_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsGlobal::func, length, kungfu::BuiltinsStubCSigns::id),

    static inline std::array GLOBAL_CONSTANTS = {
        BUILTIN_GLOBAL_CONSTANTS(BUILTIN_GLOBAL_CONSTANT_ENTRY)
    };
    static constexpr std::array GLOBAL_FUNCTIONS = {
        BUILTIN_GLOBAL_FUNCTIONS(BUILTIN_GLOBAL_FUNCTION_ENTRY)
    };
#undef BUILTIN_GLOBAL_CONSTANT_ENTRY
#undef BUILTIN_GLOBAL_FUNCTION_ENTRY

    static void PrintString(JSThread *thread, EcmaString *string);
    static void PrintValue(int64_t value, int64_t tag);
    static JSTaggedValue Encode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet);
    static JSTaggedValue Decode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet);
    static JSTaggedValue UTF16EncodeCodePoint(JSThread *thread, judgURIFunc IsInURISet,
                                              const std::vector<uint8_t> &oct, const JSHandle<EcmaString> &str,
                                              uint32_t &start, int32_t &k, std::u16string &sStr);
    static void HandleSingleByteCharacter(JSThread *thread, uint8_t &bb,
                                          const JSHandle<EcmaString> &str,
                                          uint32_t &start, int32_t &k,
                                          std::u16string &sStr, judgURIFunc IsInURISet);
    static JSTaggedValue DecodePercentEncoding(JSThread *thread, int32_t &n,
                                               int32_t &k, const JSHandle<EcmaString> &str,
                                               uint8_t &bb, std::vector<uint8_t> &oct);
    static JSTaggedValue DecodePercentEncoding(JSThread *thread, const JSHandle<EcmaString> &str, int32_t &k,
                                               judgURIFunc IsInURISet, int32_t strLen, std::u16string &sStr);
    static bool IsUnescapedURI(uint16_t ch);
    static bool IsInUnescapedURISet(uint16_t ch);
    static bool IsInReservedURISet(uint16_t ch);
    static bool IsReservedURI(uint16_t ch);
    static bool IsInMarkURISet(uint16_t ch);
    static bool IsHexDigits(uint16_t ch);
    static uint8_t GetValueFromTwoHex(uint16_t front, uint16_t behind);
    static uint16_t GetValueFromHexString(const JSHandle<EcmaString> &string);
    // 22.1.3.17.2 StringPad ( S, maxLength, fillString, placement )
    static EcmaString *StringPad(JSThread *thread,
                                 const JSHandle<EcmaString> &string,
                                 uint32_t maxLength,
                                 const JSHandle<EcmaString> &fillString,
                                 Placement placement = Placement::START);
    static bool IsUTF16HighSurrogate(uint16_t ch)
    {
        return base::utf_helper::DECODE_LEAD_LOW <= ch && ch <= base::utf_helper::DECODE_LEAD_HIGH;
    }

    static bool IsUTF16LowSurrogate(uint16_t ch)
    {
        return base::utf_helper::DECODE_TRAIL_LOW <= ch && ch <= base::utf_helper::DECODE_TRAIL_HIGH;
    }

    // 11.1.3 Static Semantics: UTF16SurrogatePairToCodePoint ( lead, trail )
    static uint16_t UTF16SurrogatePairToCodePoint(uint16_t lead, uint16_t trail);
    // 11.1.5 Static Semantics: StringToCodePoints ( string )
    static EcmaString *StringToCodePoints(JSThread *thread, const JSHandle<EcmaString> &string);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ERROR_H
