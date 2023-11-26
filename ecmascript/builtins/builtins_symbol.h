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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SYMBOL_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SYMBOL_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"

#define BUILTIN_WELL_KNOWN_SYMBOLS(V)                   \
    V(hasInstance,        HasInstance)                  \
    V(isConcatSpreadable, IsConcatSpreadable)           \
    V(toStringTag,        ToStringTag)

#define BUILTIN_PUBLIC_SYMBOLS(V)                       \
    V(asyncIterator, AsyncIterator)                     \
    V(iterator,      Iterator)                          \
    V(match,         Match)                             \
    V(matchAll,      MatchAll)                          \
    V(nativeBinding, NativeBinding)                     \
    V(replace,       Replace)                           \
    V(search,        Search)                            \
    V(species,       Species)                           \
    V(split,         Split)                             \
    V(toPrimitive,   ToPrimitive)                       \
    V(unscopables,   Unscopables)

#define BUILTIN_ALL_SYMBOLS(V)      \
    BUILTIN_WELL_KNOWN_SYMBOLS(V)   \
    BUILTIN_PUBLIC_SYMBOLS(V)

// List of functions in Symbol, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSymbol::func refers to the native implementation of Symbol[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_SYMBOL_FUNCTIONS(V)             \
    V("for",    For,    1, INVALID)             \
    V("keyFor", KeyFor, 1, INVALID)

// List of get accessors in Symbol.prototype, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSymbol::func refers to the native implementation of Symbol.prototype[name].
#define BUILTIN_SYMBOL_PROTOTYPE_FUNCTIONS(V)   \
    V("toString", ToString, 0, INVALID)         \
    V("valueOf",  ValueOf,  0, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsSymbol : public base::BuiltinsBase {
public:
    // 19.4.1
    static JSTaggedValue SymbolConstructor(EcmaRuntimeCallInfo *argv);

    // prototype
    // 19.4.3.2
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 19.4.3.3
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);
    // 19.4.2.1 Symbol.for (key)
    static JSTaggedValue For(EcmaRuntimeCallInfo *argv);
    // 19.4.2.5 Symbol.keyFor (sym)
    static JSTaggedValue KeyFor(EcmaRuntimeCallInfo *argv);

    // 19.4.3.2 get Symbol.prototype.description
    static JSTaggedValue DescriptionGetter(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ThisSymbolValue(JSThread *thread, const JSHandle<JSTaggedValue> &value);

    // 19.4.3.4
    static JSTaggedValue ToPrimitive(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue SymbolDescriptiveString(JSThread *thread, JSTaggedValue sym);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetSymbolFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(SYMBOL_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetSymbolPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(SYMBOL_PROTOTYPE_FUNCTIONS);
    }

private:
#define BUILTIN_SYMBOL_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsSymbol::func, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array SYMBOL_FUNCTIONS = {
        BUILTIN_SYMBOL_FUNCTIONS(BUILTIN_SYMBOL_FUNCTION_ENTRY)
    };
    static constexpr std::array SYMBOL_PROTOTYPE_FUNCTIONS = {
        BUILTIN_SYMBOL_PROTOTYPE_FUNCTIONS(BUILTIN_SYMBOL_FUNCTION_ENTRY)
    };
#undef BUILTIN_TYPED_ARRAY_FUNCTION_ENTRY
#undef BUILTIN_TYPED_ARRAY_ACCESSOR_ENTRY
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SYMBOL_H
