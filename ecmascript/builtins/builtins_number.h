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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_NUMBER_H
#define ECMASCRIPT_BUILTINS_BUILTINS_NUMBER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_tagged_value.h"

// List of constants in Number, excluding '@@' internal properties.
#define BUILTIN_NUMBER_CONSTANTS(V)                             \
    V(EPSILON)           /* Number.EPSILON */                   \
    V(MAX_SAFE_INTEGER)  /* Number.MAX_SAFE_INTEGER */          \
    V(MAX_VALUE)         /* Number.MAX_VALUE */                 \
    V(MIN_SAFE_INTEGER)  /* Number.MIN_SAFE_INTEGER */          \
    V(MIN_VALUE)         /* Number.MIN_VALUE */                 \
    V(NEGATIVE_INFINITY) /* Number.NEGATIVE_INFINITY */         \
    V(NaN)               /* Number.NaN */                       \
    V(POSITIVE_INFINITY) /* Number.POSITIVE_INFINITY */

// List of functions in Number.
// V(name, func, length, stubIndex)
// where BuiltinsNumber::func refers to the native implementation of Number[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_NUMBER_NON_GLOBAL_FUNCTIONS(V)                                                           \
    V("isFinite",      IsFinite,      1, NumberIsFinite)          /* Number.isFinite ( number ) */       \
    V("isInteger",     IsInteger,     1, NumberIsInteger)         /* Number.isInteger ( number ) */      \
    V("isNaN",         IsNaN,         1, NumberIsNaN)             /* Number.isNaN ( number ) */          \
    V("isSafeInteger", IsSafeInteger, 1, NumberIsSafeInteger)     /* Number.isSafeInteger ( number ) */

// List of functions in Number that can be accessed via globalThis.
// V(name, func, length, stubIndex)
// where BuiltinsNumber::func refers to the native implementation of Number[name].
#define BUILTIN_NUMBER_GLOBAL_FUNCTIONS(V)                                              \
    V("parseFloat", ParseFloat, 1, NumberParseFloat) /* Number.parseFloat ( string ) */ \
    V("parseInt",   ParseInt,   2, NumberParseInt) /* Number.parseInt ( string, radix ) */

#define BUILTIN_NUMBER_FUNCTIONS(V)         \
    BUILTIN_NUMBER_NON_GLOBAL_FUNCTIONS(V)  \
    BUILTIN_NUMBER_GLOBAL_FUNCTIONS(V)

// List of functions in Number.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsNumber::func refers to the native implementation of Number.prototype[name].
#define BUILTIN_NUMBER_PROTOTYPE_FUNCTIONS(V)                               \
    /* Number.prototype.toExponential ( fractionDigits ) */                 \
    V("toExponential",  ToExponential,  1, INVALID)                         \
    /* Number.prototype.toFixed ( fractionDigits ) */                       \
    V("toFixed",        ToFixed,        1, INVALID)                         \
    /* Number.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */ \
    V("toLocaleString", ToLocaleString, 0, INVALID)                         \
    /* Number.prototype.toPrecision ( precision ) */                        \
    V("toPrecision",    ToPrecision,    1, INVALID)                         \
    /* Number.prototype.toString ( [ radix ] ) */                           \
    V("toString",       ToString,       1, NumberToStringFunc)              \
    /* Number.prototype.valueOf ( ) */                                      \
    V("valueOf",        ValueOf,        0, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsNumber : public base::BuiltinsBase {
public:
    // 21.1.2.1 Number.EPSILON
    static constexpr double EPSILON = std::numeric_limits<double>::epsilon();
    // 21.1.2.6 Number.MAX_SAFE_INTEGER (which is 2**53 - 1 = 9007199254740991)
    static constexpr int64_t MAX_SAFE_INTEGER = (1LL << 53) - 1;
    // 21.1.2.8 Number.MIN_SAFE_INTEGER (which is -(2**53 - 1) = -9007199254740991)
    static constexpr int64_t MIN_SAFE_INTEGER = -((1LL << 53) - 1);
    // 21.1.2.7 Number.MAX_VALUE
    static constexpr double MAX_VALUE = std::numeric_limits<double>::max();
    // 21.1.2.9 Number.MIN_VALUE
    static constexpr double MIN_VALUE = std::numeric_limits<double>::denorm_min();
    // 21.1.2.14 Number.POSITIVE_INFINITY
    static constexpr double POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
    // 21.1.2.11 Number.NEGATIVE_INFINITY
    static constexpr double NEGATIVE_INFINITY = -POSITIVE_INFINITY;
    // 21.1.2.10 Number.NaN
    static constexpr double NaN = NAN;

    // 20.1.1.1
    static JSTaggedValue NumberConstructor(EcmaRuntimeCallInfo *argv);

    // 20.1.2.2
    static JSTaggedValue IsFinite(EcmaRuntimeCallInfo *argv);
    // 20.1.2.3
    static JSTaggedValue IsInteger(EcmaRuntimeCallInfo *argv);
    // 20.1.2.4
    static JSTaggedValue IsNaN(EcmaRuntimeCallInfo *argv);
    // 20.1.2.5
    static JSTaggedValue IsSafeInteger(EcmaRuntimeCallInfo *argv);
    // 20.1.2.12
    static JSTaggedValue ParseFloat(EcmaRuntimeCallInfo *argv);
    // 20.1.2.13
    static JSTaggedValue ParseInt(EcmaRuntimeCallInfo *argv);

    // prototype
    // 20.1.3.2
    static JSTaggedValue ToExponential(EcmaRuntimeCallInfo *argv);
    // 20.1.3.3
    static JSTaggedValue ToFixed(EcmaRuntimeCallInfo *argv);
    // 20.1.3.4
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 20.1.3.5
    static JSTaggedValue ToPrecision(EcmaRuntimeCallInfo *argv);
    // 20.1.3.6
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 20.1.3.7
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties.
    static Span<const base::BuiltinConstantEntry> GetNumberConstants()
    {
        return Span<const base::BuiltinConstantEntry>(NUMBER_CONSTANTS);
    }

    // Excluding the '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetNumberNonGlobalFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(NUMBER_NON_GLOBAL_FUNCTIONS);
    }

    // Excluding the '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetNumberGlobalFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(NUMBER_GLOBAL_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetNumberPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(NUMBER_PROTOTYPE_FUNCTIONS);
    }

private:
#define BUILTIN_NUMBER_CONSTANT_ENTRY(name) \
    base::BuiltinConstantEntry::Create(#name, JSTaggedValue(BuiltinsNumber::name)),

    static inline std::array NUMBER_CONSTANTS = {
        BUILTIN_NUMBER_CONSTANTS(BUILTIN_NUMBER_CONSTANT_ENTRY)
    };
#undef BUILTIN_NUMBER_CONSTANT_ENTRY

#define BUILTIN_NUMBER_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsNumber::func, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array NUMBER_NON_GLOBAL_FUNCTIONS = {
        BUILTIN_NUMBER_NON_GLOBAL_FUNCTIONS(BUILTIN_NUMBER_FUNCTION_ENTRY)
    };
    static constexpr std::array NUMBER_GLOBAL_FUNCTIONS = {
        BUILTIN_NUMBER_GLOBAL_FUNCTIONS(BUILTIN_NUMBER_FUNCTION_ENTRY)
    };
    static constexpr std::array NUMBER_PROTOTYPE_FUNCTIONS = {
        BUILTIN_NUMBER_PROTOTYPE_FUNCTIONS(BUILTIN_NUMBER_FUNCTION_ENTRY)
    };
#undef BUILTIN_NUMBER_FUNCTION_ENTRY

    static JSTaggedNumber ThisNumberValue(JSThread *thread, EcmaRuntimeCallInfo *argv);
};

class NumberToStringResultCache : public TaggedArray {
public:
    static NumberToStringResultCache *Cast(TaggedObject *object)
    {
        return reinterpret_cast<NumberToStringResultCache*>(object);
    }
    static JSTaggedValue CreateCacheTable(const JSThread *thread);
    JSTaggedValue FindCachedResult(const JSThread *thread, int entry, JSTaggedValue &target);
    void SetCachedResult(const JSThread *thread, int entry, JSTaggedValue &number, JSHandle<EcmaString> &result);
    int GetNumberHash(JSTaggedValue &number)
    {
        unsigned int mask = INITIAL_CACHE_NUMBER - 1;
        unsigned int value = 0;
        if (number.IsInt()) {
            value = static_cast<unsigned int>(number.GetInt());
        } else {
            int64_t bits = base::bit_cast<int64_t>(number.GetDouble());
            value = static_cast<unsigned int>(bits) ^ static_cast<unsigned int>(bits >> 32); // 32: hight 32 bit
        }
        return value & mask;
    }

private:
    static constexpr int INITIAL_CACHE_NUMBER = 256;
    static constexpr int NUMBER_INDEX = 0;
    static constexpr int RESULT_INDEX = 1;
    static constexpr int ENTRY_SIZE = 2;
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_NUBMER_H
