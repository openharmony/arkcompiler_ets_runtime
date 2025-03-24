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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_DATE_H
#define ECMASCRIPT_BUILTINS_BUILTINS_DATE_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_date.h"

// List of functions in Date, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsDate::func refers to the native implementation of Date[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_DATE_FUNCTIONS(V)                                                                   \
    /* Date.now ( ) */                                                                              \
    V("now",   Now,   0, DateNow)                                                                   \
    /* Date.parse ( string ) */                                                                     \
    V("parse", Parse, 1, INVALID)                                                                   \
    /* Date.UTC ( year [ , month [ , date [ , hours [ , minutes [ , seconds [ , ms ] ] ] ] ] ] ) */ \
    V("UTC",   UTC,   ::panda::ecmascript::builtins::BuiltinsDate::UTC_LENGTH, INVALID)

// List of functions in Date.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsDate::func refers to the native implementation of Date.prototype[name].
#define BUILTIN_DATE_PROTOTYPE_FUNCTIONS(V)                                     \
    /* Date.prototype.getDate ( ) */                                            \
    V("getDate",            GetDate,            0, INVALID)                     \
    /* Date.prototype.getDay ( ) */                                             \
    V("getDay",             GetDay,             0, INVALID)                     \
    /* Date.prototype.getFullYear ( ) */                                        \
    V("getFullYear",        GetFullYear,        0, INVALID)                     \
    /* Date.prototype.getHours ( ) */                                           \
    V("getHours",           GetHours,           0, INVALID)                     \
    /* Date.prototype.getMilliseconds ( ) */                                    \
    V("getMilliseconds",    GetMilliseconds,    0, INVALID)                     \
    /* Date.prototype.getMinutes ( ) */                                         \
    V("getMinutes",         GetMinutes,         0, INVALID)                     \
    /* Date.prototype.getMonth ( ) */                                           \
    V("getMonth",           GetMonth,           0, INVALID)                     \
    /* Date.prototype.getSeconds ( ) */                                         \
    V("getSeconds",         GetSeconds,         0, INVALID)                     \
    /* Date.prototype.getTime ( ) */                                            \
    V("getTime",            GetTime,            0, DateGetTime)                 \
    /* Date.prototype.getTimezoneOffset ( ) */                                  \
    V("getTimezoneOffset",  GetTimezoneOffset,  0, INVALID)                     \
    /* Date.prototype.getUTCDate ( ) */                                         \
    V("getUTCDate",         GetUTCDate,         0, INVALID)                     \
    /* Date.prototype.getUTCDay ( ) */                                          \
    V("getUTCDay",          GetUTCDay,          0, INVALID)                     \
    /* Date.prototype.getUTCFullYear ( ) */                                     \
    V("getUTCFullYear",     GetUTCFullYear,     0, INVALID)                     \
    /* Date.prototype.getUTCHours ( ) */                                        \
    V("getUTCHours",        GetUTCHours,        0, INVALID)                     \
    /* Date.prototype.getUTCMilliseconds ( ) */                                 \
    V("getUTCMilliseconds", GetUTCMilliseconds, 0, INVALID)                     \
    /* Date.prototype.getUTCMinutes ( ) */                                      \
    V("getUTCMinutes",      GetUTCMinutes,      0, INVALID)                     \
    /* Date.prototype.getUTCMonth ( ) */                                        \
    V("getUTCMonth",        GetUTCMonth,        0, INVALID)                     \
    /* Date.prototype.getUTCSeconds ( ) */                                      \
    V("getUTCSeconds",      GetUTCSeconds,      0, INVALID)                     \
    /* Date.prototype.setDate ( date ) */                                       \
    V("setDate",            SetDate,            1, INVALID)                     \
    /* Date.prototype.setFullYear ( year [ , month [ , date ] ] ) */            \
    V("setFullYear",        SetFullYear,        3, INVALID)                     \
    /* Date.prototype.setHours ( hour [ , min [ , sec [ , ms ] ] ] ) */         \
    V("setHours",           SetHours,           4, INVALID)                     \
    /* Date.prototype.setMilliseconds ( ms ) */                                 \
    V("setMilliseconds",    SetMilliseconds,    1, INVALID)                     \
    /* Date.prototype.setMinutes ( min [ , sec [ , ms ] ] ) */                  \
    V("setMinutes",         SetMinutes,         3, INVALID)                     \
    /* Date.prototype.setMonth ( month [ , date ] ) */                          \
    V("setMonth",           SetMonth,           2, INVALID)                     \
    /* Date.prototype.setSeconds ( sec [ , ms ] ) */                            \
    V("setSeconds",         SetSeconds,         2, INVALID)                     \
    /* Date.prototype.setTime ( time ) */                                       \
    V("setTime",            SetTime,            1, INVALID)                     \
    /* Date.prototype.setUTCDate ( date ) */                                    \
    V("setUTCDate",         SetUTCDate,         1, INVALID)                     \
    /* Date.prototype.setUTCFullYear ( year [ , month [ , date ] ] ) */         \
    V("setUTCFullYear",     SetUTCFullYear,     3, INVALID)                     \
    /* Date.prototype.setUTCHours ( hour [ , min [ , sec [ , ms ] ] ] ) */      \
    V("setUTCHours",        SetUTCHours,        4, INVALID)                     \
    /* Date.prototype.setUTCMilliseconds ( ms ) */                              \
    V("setUTCMilliseconds", SetUTCMilliseconds, 1, INVALID)                     \
    /* Date.prototype.setUTCMinutes ( min [ , sec [ , ms ] ] ) */               \
    V("setUTCMinutes",      SetUTCMinutes,      3, INVALID)                     \
    /* Date.prototype.setUTCMonth ( month [ , date ] ) */                       \
    V("setUTCMonth",        SetUTCMonth,        2, INVALID)                     \
    /* Date.prototype.setUTCSeconds ( sec [ , ms ] ) */                         \
    V("setUTCSeconds",      SetUTCSeconds,      2, INVALID)                     \
    /* Date.prototype.toDateString ( ) */                                       \
    V("toDateString",       ToDateString,       0, INVALID)                     \
    /* Date.prototype.toISOString ( ) */                                        \
    V("toISOString",        ToISOString,        0, INVALID)                     \
    /* Date.prototype.toJSON ( key ) */                                         \
    V("toJSON",             ToJSON,             1, INVALID)                     \
    /* Date.prototype.toLocaleDateString ( [ reserved1 [ , reserved2 ] ] ) */   \
    V("toLocaleDateString", ToLocaleDateString, 0, INVALID)                     \
    /* Date.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */       \
    V("toLocaleString",     ToLocaleString,     0, INVALID)                     \
    /* Date.prototype.toLocaleTimeString ( [ reserved1 [ , reserved2 ] ] ) */   \
    V("toLocaleTimeString", ToLocaleTimeString, 0, INVALID)                     \
    /* Date.prototype.toString ( ) */                                           \
    V("toString",           ToString,           0, INVALID)                     \
    /* Date.prototype.toTimeString ( ) */                                       \
    V("toTimeString",       ToTimeString,       0, INVALID)                     \
    /* Date.prototype.toUTCString ( ) */                                        \
    V("toUTCString",        ToUTCString,        0, INVALID)                     \
    /* Date.prototype.valueOf ( ) */                                            \
    V("valueOf",            ValueOf,            0, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsDate : public base::BuiltinsBase {
public:
    static constexpr int UTC_LENGTH = 7;

    // 20.4.2 The Date Constructor
    static JSTaggedValue DateConstructor(EcmaRuntimeCallInfo *argv);

    // 20.4.3.1 Date.now ( )
    static JSTaggedValue Now(EcmaRuntimeCallInfo *argv);

    // 20.4.3.4 Date.UTC ( year [ , month [ , date [ , hours [ , minutes [ , seconds [ , ms ] ] ] ] ] ] )
    static JSTaggedValue UTC(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Parse(EcmaRuntimeCallInfo *argv);

    // 20.4.4.2 Date.prototype.getDate ( )
    GET_DATE_VALUE(GetDate, DAYS, true);

    // 20.4.4.3 Date.prototype.getDay ( )
    GET_DATE_VALUE(GetDay, WEEKDAY, true);

    // 20.4.4.4 Date.prototype.getFullYear ( )
    GET_DATE_VALUE(GetFullYear, YEAR, true);

    // 20.4.4.5 Date.prototype.getHours ( )
    GET_DATE_VALUE(GetHours, HOUR, true);

    // 20.4.4.6 Date.prototype.getMilliseconds ( )
    GET_DATE_VALUE(GetMilliseconds, MS, true);

    // 20.4.4.7 Date.prototype.getMinutes ( )
    GET_DATE_VALUE(GetMinutes, MIN, true);

    // 20.4.4.8 Date.prototype.getMonth ( )
    GET_DATE_VALUE(GetMonth, MONTH, true);

    // 20.4.4.9 Date.prototype.getSeconds ( )
    GET_DATE_VALUE(GetSeconds, SEC, true);

    // 20.4.4.10 Date.prototype.getTime ( )
    static JSTaggedValue GetTime(EcmaRuntimeCallInfo *argv);

    // 20.4.4.11 Date.prototype.getTimezoneOffset ( )
    GET_DATE_VALUE(GetTimezoneOffset, TIMEZONE, true);

    // 20.4.4.12 Date.prototype.getUTCDate ( )
    GET_DATE_VALUE(GetUTCDate, DAYS, false);

    // 20.4.4.13 Date.prototype.getUTCDay ( )
    GET_DATE_VALUE(GetUTCDay, WEEKDAY, false);

    // 20.4.4.14 Date.prototype.getUTCFullYear ( )
    GET_DATE_VALUE(GetUTCFullYear, YEAR, false);

    // 20.4.4.15 Date.prototype.getUTCHours ( )
    GET_DATE_VALUE(GetUTCHours, HOUR, false);

    // 20.4.4.16 Date.prototype.getUTCMilliseconds ( )
    GET_DATE_VALUE(GetUTCMilliseconds, MS, false);

    // 20.4.4.17 Date.prototype.getUTCMinutes ( )
    GET_DATE_VALUE(GetUTCMinutes, MIN, false);

    // 20.4.4.18 Date.prototype.getUTCMonth ( )
    GET_DATE_VALUE(GetUTCMonth, MONTH, false);

    // 20.4.4.19 Date.prototype.getUTCSeconds ( )
    GET_DATE_VALUE(GetUTCSeconds, SEC, false);

    // 20.3.4.20 Date.prototype.setDate ( date )
    SET_DATE_VALUE(SetDate, CODE_SET_DATE, true);

    // 20.3.4.21 Date.prototype.setFullYear ( year [ , month [ , date ] ] )
    SET_DATE_VALUE(SetFullYear, CODE_SET_FULL_YEAR, true);

    // 20.3.4.22 Date.prototype.setHours ( hour [ , min [ , sec [ , ms ] ] ] )
    SET_DATE_VALUE(SetHours, CODE_SET_HOURS, true);

    // 20.3.4.23 Date.prototype.setMilliseconds ( ms )
    SET_DATE_VALUE(SetMilliseconds, CODE_SET_MILLISECONDS, true);

    // 20.3.4.24 Date.prototype.setMinutes ( min [ , sec [ , ms ] ] )
    SET_DATE_VALUE(SetMinutes, CODE_SET_MINUTES, true);

    // 20.3.4.25 Date.prototype.setMonth ( month [ , date ] )
    SET_DATE_VALUE(SetMonth, CODE_SET_MONTH, true);

    // 20.3.4.26 Date.prototype.setSeconds ( sec [ , ms ] )
    SET_DATE_VALUE(SetSeconds, CODE_SET_SECONDS, true);

    // 20.3.4.27 Date.prototype.setTime ( time )
    static JSTaggedValue SetTime(EcmaRuntimeCallInfo *argv);

    // 20.3.4.28 Date.prototype.setUTCDate ( date )
    SET_DATE_VALUE(SetUTCDate, CODE_SET_DATE, false);

    // 20.3.4.29 Date.prototype.setUTCFullYear ( year [ , month [ , date ] ] )
    SET_DATE_VALUE(SetUTCFullYear, CODE_SET_FULL_YEAR, false);

    // 20.3.4.30 Date.prototype.setUTCHours ( hour [ , min [ , sec [ , ms ] ] ] )
    SET_DATE_VALUE(SetUTCHours, CODE_SET_HOURS, false);

    // 20.3.4.31 Date.prototype.setUTCMilliseconds ( ms )
    SET_DATE_VALUE(SetUTCMilliseconds, CODE_SET_MILLISECONDS, false);

    // 20.3.4.32 Date.prototype.setUTCMinutes ( min [ , sec [, ms ] ] )
    SET_DATE_VALUE(SetUTCMinutes, CODE_SET_MINUTES, false);

    // 20.3.4.33 Date.prototype.setUTCMonth ( month [ , date ] )
    SET_DATE_VALUE(SetUTCMonth, CODE_SET_MONTH, false);

    // 20.3.4.34 Date.prototype.setUTCSeconds ( sec [ , ms ] )
    SET_DATE_VALUE(SetUTCSeconds, CODE_SET_SECONDS, false);

    // 20.4.4.35 Date.prototype.toDateString ( )
    DATE_STRING(ToDateString);

    // 20.4.4.36 Date.prototype.toISOString ( )
    DATE_TO_STRING(ToISOString);

    // 20.4.4.37 Date.prototype.toJSON ( key )
    static JSTaggedValue ToJSON(EcmaRuntimeCallInfo *argv);

    // 20.4.4.38 Date.prototype.toLocaleDateString ( [ reserved1 [ , reserved2 ] ] )
    static JSTaggedValue ToLocaleDateString(EcmaRuntimeCallInfo *argv);

    // 20.4.4.39 Date.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] )
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);

    // 20.4.4.40 Date.prototype.toLocaleTimeString ( [ reserved1 [ , reserved2 ] ] )
    static JSTaggedValue ToLocaleTimeString(EcmaRuntimeCallInfo *argv);

    // 20.4.4.41 Date.prototype.toString ( )
    DATE_STRING(ToString);

    // 20.4.4.42 Date.prototype.toTimeString ( )
    DATE_STRING(ToTimeString);

    // 20.4.4.43 Date.prototype.toUTCString ( )
    DATE_STRING(ToUTCString);

    // 20.4.4.44 Date.prototype.valueOf ( )
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);

    // 20.4.4.45 Date.prototype [ @@toPrimitive ]
    static JSTaggedValue ToPrimitive(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetDateFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(DATE_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetDatePrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(DATE_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 2 : 2 more inline properties in Date.prototype:
        //   (1) Date.prototype.constructor
        //   (2) Date.prototype [ @@toPrimitive ]
        return GetDatePrototypeFunctions().Size() + 2;
    }

private:
#define BUILTIN_DATE_FUNCTION_ENTRY(name, func, length, builtinId) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsDate::func, length, BUILTINS_STUB_ID(builtinId)),

    static constexpr std::array DATE_FUNCTIONS = {
        BUILTIN_DATE_FUNCTIONS(BUILTIN_DATE_FUNCTION_ENTRY)
    };
    static constexpr std::array DATE_PROTOTYPE_FUNCTIONS = {
        BUILTIN_DATE_PROTOTYPE_FUNCTIONS(BUILTIN_DATE_FUNCTION_ENTRY)
    };
#undef BUILTIN_DATE_FUNCTION_ENTRY
    static JSTaggedValue ExtractDateFields(JSThread *thread, uint32_t &length, EcmaRuntimeCallInfo *argv,
        JSTaggedValue &timeValue);

    // definition for set data code.
    static constexpr uint32_t CODE_SET_DATE = 0x32;
    static constexpr uint32_t CODE_SET_MILLISECONDS = 0x76;
    static constexpr uint32_t CODE_SET_SECONDS = 0x75;
    static constexpr uint32_t CODE_SET_MINUTES = 0x74;
    static constexpr uint32_t CODE_SET_HOURS = 0x73;
    static constexpr uint32_t CODE_SET_MONTH = 0x31;
    static constexpr uint32_t CODE_SET_FULL_YEAR = 0x30;
    static constexpr uint8_t CONSTRUCTOR_MAX_LENGTH = 7;
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_DATE_H
