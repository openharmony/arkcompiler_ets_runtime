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
    static JSTaggedValue CallJsBoundFunction(EcmaRuntimeCallInfo *msg);
    static JSTaggedValue CallJsProxy(EcmaRuntimeCallInfo *msg);
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

private:
    static void PrintString(JSThread *thread, EcmaString *string);
    static void PrintValue(int64_t value, int64_t tag);
    static JSTaggedValue Encode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet);
    static JSTaggedValue Decode(JSThread *thread, const JSHandle<EcmaString> &str, judgURIFunc IsInURISet);
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
