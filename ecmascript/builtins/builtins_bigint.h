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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_BIGINT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_BIGINT_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_tagged_value.h"

// List of functions in BigInt, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsBigInt::func refers to the native implementation of BigInt[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_BIGINT_FUNCTIONS(V)                                         \
    /* BigInt.asIntN (bits, bigint) */                                      \
    V("asIntN", AsIntN, 2, BigIntAsIntN)                                    \
    /* BigInt.asUintN ( bits, bigint ) */                                   \
    V("asUintN", AsUintN, 2, BigIntAsUintN)
// List of functions in BigInt.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsBigInt::func refers to the native implementation of BigInt.prototype[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_BIGINT_PROTOTYPE_FUNCTIONS(V)                               \
    /* BigInt.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */           \
    V("toLocaleString", ToLocaleString, 0, INVALID)                         \
    /* BigInt.toString ( [radix] ) */                                       \
    V("toString", ToString, 0, INVALID)                                     \
    /* BigInt.valueOf ( ) */                                                \
    V("valueOf", ValueOf, 0, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsBigInt : public base::BuiltinsBase {
public:
    // 21.2.1.1
    static JSTaggedValue BigIntConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue BigIntConstructorInternal(JSThread *thread, JSHandle<JSTaggedValue> value);
    // 21.2.2.1
    static JSTaggedValue AsIntN(EcmaRuntimeCallInfo *argv);
    // 21.2.2.2
    static JSTaggedValue AsUintN(EcmaRuntimeCallInfo *argv);
    // 21.2.3.2
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 21.2.3.3
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 21.2.3.4
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);
private:
    static JSTaggedValue ThisBigIntValue(EcmaRuntimeCallInfo *argv);

public:
    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetBigIntFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(BIGINT_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetBigIntPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(BIGINT_PROTOTYPE_FUNCTIONS);
    }

private:
#define BUILTIN_BIGINT_FUNCTION_ENTRY(name, func, length, builtinId) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsBigInt::func, length, BUILTINS_STUB_ID(builtinId)),

    static inline std::array BIGINT_FUNCTIONS = {
        BUILTIN_BIGINT_FUNCTIONS(BUILTIN_BIGINT_FUNCTION_ENTRY)
    };
    static inline std::array BIGINT_PROTOTYPE_FUNCTIONS = {
        BUILTIN_BIGINT_PROTOTYPE_FUNCTIONS(BUILTIN_BIGINT_FUNCTION_ENTRY)
    };
#undef BUILTIN_BIGINT_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_BIGINT_H