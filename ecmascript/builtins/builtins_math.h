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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_MATH_H
#define ECMASCRIPT_BUILTINS_BUILTINS_MATH_H

#include "ecmascript/base/builtins_base.h"

// List of constants in Math, excluding '@@' internal properties.
#define BUILTIN_MATH_CONSTANTS(V)   \
    V(E)                            \
    V(LN10)                         \
    V(LN2)                          \
    V(LOG10E)                       \
    V(LOG2E)                        \
    V(PI)                           \
    V(SQRT1_2)                      \
    V(SQRT2)

// List of functions in Math.
// V(name, func, length, stubIndex)
// where BuiltinsMath::func refers to the native implementation of Math[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_MATH_FUNCTIONS(V)                                           \
    V("abs",    Abs,    1, MathAbs)     /* Math.abs ( x ) */                \
    V("acos",   Acos,   1, MathAcos)    /* Math.acos ( x ) */               \
    V("acosh",  Acosh,  1, MathAcosh)   /* Math.acosh ( x ) */              \
    V("asin",   Asin,   1, MathAsin)    /* Math.asin ( x ) */               \
    V("asinh",  Asinh,  1, MathAsinh)   /* Math.asinh ( x ) */              \
    V("atan",   Atan,   1, MathAtan)    /* Math.atan ( x ) */               \
    V("atan2",  Atan2,  2, MathAtan2)   /* Math.atan2 ( y, x ) */           \
    V("atanh",  Atanh,  1, MathAtanh)   /* Math.atanh ( x ) */              \
    V("cbrt",   Cbrt,   1, MathCbrt)     /* Math.cbrt ( x ) */              \
    V("ceil",   Ceil,   1, MathCeil)    /* Math.ceil ( x ) */               \
    V("clz32",  Clz32,  1, MathClz32)   /* Math.clz32 ( x ) */              \
    V("cos",    Cos,    1, MathCos)     /* Math.cos ( x ) */                \
    V("cosh",   Cosh,   1, MathCosh)    /* Math.cosh ( x ) */               \
    V("exp",    Exp,    1, MathExp)     /* Math.exp ( x ) */                \
    V("expm1",  Expm1,  1, MathExpm1)   /* Math.expm1 ( x ) */              \
    V("floor",  Floor,  1, MathFloor)       /* Math.floor ( x ) */              \
    V("fround", Fround, 1, MathFRound)  /* Math.fround ( x ) */             \
    V("hypot",  Hypot,  2, INVALID)     /* Math.hypot ( ...args ) */        \
    V("imul",   Imul,   2, MathImul)    /* Math.imul ( x, y ) */            \
    V("log",    Log,    1, MathLog)     /* Math.log ( x ) */                \
    V("log10",  Log10,  1, MathLog10)   /* Math.log10 ( x ) */              \
    V("log1p",  Log1p,  1, MathLog1p)   /* Math.log1p ( x ) */              \
    V("log2",   Log2,   1, MathLog2)    /* Math.log2 ( x ) */               \
    V("max",    Max,    2, MathMax)     /* Math.max ( ...args ) */          \
    V("min",    Min,    2, MathMin)     /* Math.min ( ...args ) */          \
    V("pow",    Pow,    2, MathPow)     /* Math.pow ( base, exponent ) */   \
    V("random", Random, 0, INVALID)     /* Math.random ( ) */               \
    V("round",  Round,  1, MathRound)   /* Math.round ( x ) */              \
    V("sign",   Sign,   1, MathSign)    /* Math.sign ( x ) */               \
    V("sin",    Sin,    1, MathSin)     /* Math.sin ( x ) */                \
    V("sinh",   Sinh,   1, MathSinh)    /* Math.sinh ( x ) */               \
    V("sqrt",   Sqrt,   1, MathSqrt)    /* Math.sqrt ( x ) */               \
    V("tan",    Tan,    1, MathTan)     /* Math.tan ( x ) */                \
    V("tanh",   Tanh,   1, MathTanh)    /* Math.tanh ( x ) */               \
    V("trunc",  Trunc,  1, MathTrunc)   /* Math.trunc ( x ) */

namespace panda::ecmascript::builtins {
class BuiltinsMath : public base::BuiltinsBase {
public:
    // 20.2.1.1
    static constexpr double E = 2.718281828459045;
    // 20.2.1.2
    static constexpr double LN10 = 2.302585092994046;
    // 20.2.1.3
    static constexpr double LN2 = 0.6931471805599453;
    // 20.2.1.4
    static constexpr double LOG10E = 0.4342944819032518;
    // 20.2.1.5
    static constexpr double LOG2E = 1.4426950408889634;
    // 20.2.1.6
    static constexpr double PI = 3.141592653589793;
    // 20.2.1.7
    static constexpr double SQRT1_2 = 0.7071067811865476;
    // 20.2.1.8
    static constexpr double SQRT2 = 1.4142135623730951;
    // 20.2.2.1
    static JSTaggedValue Abs(EcmaRuntimeCallInfo *argv);
    // 20.2.2.2
    static JSTaggedValue Acos(EcmaRuntimeCallInfo *argv);
    // 20.2.2.3
    static JSTaggedValue Acosh(EcmaRuntimeCallInfo *argv);
    // 20.2.2.4
    static JSTaggedValue Asin(EcmaRuntimeCallInfo *argv);
    // 20.2.2.5
    static JSTaggedValue Asinh(EcmaRuntimeCallInfo *argv);
    // 20.2.2.6
    static JSTaggedValue Atan(EcmaRuntimeCallInfo *argv);
    // 20.2.2.7
    static JSTaggedValue Atanh(EcmaRuntimeCallInfo *argv);
    // 20.2.2.8
    static JSTaggedValue Atan2(EcmaRuntimeCallInfo *argv);
    // 20.2.2.9
    static JSTaggedValue Cbrt(EcmaRuntimeCallInfo *argv);
    // 20.2.2.10
    static JSTaggedValue Ceil(EcmaRuntimeCallInfo *argv);
    // 20.2.2.11
    static JSTaggedValue Clz32(EcmaRuntimeCallInfo *argv);
    // 20.2.2.12
    static JSTaggedValue Cos(EcmaRuntimeCallInfo *argv);
    // 20.2.2.13
    static JSTaggedValue Cosh(EcmaRuntimeCallInfo *argv);
    // 20.2.2.14
    static JSTaggedValue Exp(EcmaRuntimeCallInfo *argv);
    // 20.2.2.15
    static JSTaggedValue Expm1(EcmaRuntimeCallInfo *argv);
    // 20.2.2.16
    static JSTaggedValue Floor(EcmaRuntimeCallInfo *argv);
    // 20.2.2.17
    static JSTaggedValue Fround(EcmaRuntimeCallInfo *argv);
    // 20.2.2.18
    static JSTaggedValue Hypot(EcmaRuntimeCallInfo *argv);
    // 20.2.2.19
    static JSTaggedValue Imul(EcmaRuntimeCallInfo *argv);
    // 20.2.2.20
    static JSTaggedValue Log(EcmaRuntimeCallInfo *argv);
    // 20.2.2.21
    static JSTaggedValue Log1p(EcmaRuntimeCallInfo *argv);
    // 20.2.2.22
    static JSTaggedValue Log10(EcmaRuntimeCallInfo *argv);
    // 20.2.2.23
    static JSTaggedValue Log2(EcmaRuntimeCallInfo *argv);
    // 20.2.2.24
    static JSTaggedValue Max(EcmaRuntimeCallInfo *argv);
    // 20.2.2.25
    static JSTaggedValue Min(EcmaRuntimeCallInfo *argv);
    // 20.2.2.26
    static JSTaggedValue Pow(EcmaRuntimeCallInfo *argv);
    // 20.2.2.27
    static JSTaggedValue Random(EcmaRuntimeCallInfo *argv);
    // 20.2.2.28
    static JSTaggedValue Round(EcmaRuntimeCallInfo *argv);
    // 20.2.2.29
    static JSTaggedValue Sign(EcmaRuntimeCallInfo *argv);
    // 20.2.2.30
    static JSTaggedValue Sin(EcmaRuntimeCallInfo *argv);
    // 20.2.2.31
    static JSTaggedValue Sinh(EcmaRuntimeCallInfo *argv);
    // 20.2.2.32
    static JSTaggedValue Sqrt(EcmaRuntimeCallInfo *argv);
    // 20.2.2.33
    static JSTaggedValue Tan(EcmaRuntimeCallInfo *argv);
    // 20.2.2.34
    static JSTaggedValue Tanh(EcmaRuntimeCallInfo *argv);
    // 20.2.2.35
    static JSTaggedValue Trunc(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties.
    static Span<const base::BuiltinConstantEntry> GetMathConstants()
    {
        return Span<const base::BuiltinConstantEntry>(MATH_CONSTANTS);
    }

    static Span<const base::BuiltinFunctionEntry> GetMathFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(MATH_FUNCTIONS);
    }

private:
#define BUILTIN_MATH_CONSTANT_ENTRY(name) \
    base::BuiltinConstantEntry::Create(#name, JSTaggedValue(BuiltinsMath::name)),

    static inline std::array MATH_CONSTANTS = {
        BUILTIN_MATH_CONSTANTS(BUILTIN_MATH_CONSTANT_ENTRY)
    };
#undef BUILTIN_MATH_CONSTANT_ENTRY

#define BUILTIN_MATH_FUNCTION_ENTRY(name, func, length, builtinId) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsMath::func, length, BUILTINS_STUB_ID(builtinId)),

    static constexpr std::array MATH_FUNCTIONS = {
        BUILTIN_MATH_FUNCTIONS(BUILTIN_MATH_FUNCTION_ENTRY)
    };
#undef BUILTIN_MATH_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_MATH_H
