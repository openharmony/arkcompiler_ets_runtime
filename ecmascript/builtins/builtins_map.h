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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_MAP_H
#define ECMASCRIPT_BUILTINS_BUILTINS_MAP_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in Map.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsMap::func refers to the native implementation of Map.prototype[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_MAP_PROTOTYPE_FUNCTIONS(V)                      \
    /* Map.prototype.clear ( ) */                               \
    V("clear",   Clear,   0, MapClear)                          \
    /* Map.prototype.delete ( key ) */                          \
    V("delete",  Delete,  1, MapDelete)                         \
    /* Map.prototype.entries ( ) */                             \
    V("entries", Entries, 0, MapEntries)                        \
    /* Map.prototype.forEach ( callbackfn [ , thisArg ] ) */    \
    V("forEach", ForEach, 1, MapForEach)                        \
    /* Map.prototype.get ( key ) */                             \
    V("get",     Get,     1, MapGet)                            \
    /* Map.prototype.has ( key ) */                             \
    V("has",     Has,     1, MapHas)                            \
    /* Map.prototype.keys ( ) */                                \
    V("keys",    Keys,    0, MapKeys)                           \
    /* Map.prototype.set ( key, value ) */                      \
    V("set",     Set,     2, MapSet)                            \
    /* Map.prototype.values ( ) */                              \
    V("values",  Values,  0, MapValues)

namespace panda::ecmascript::builtins {
class BuiltinsMap : public base::BuiltinsBase {
public:
    // 23.1.1.1
    static JSTaggedValue MapConstructor(EcmaRuntimeCallInfo *argv);
    // 23.1.2.2
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    // 23.1.3.1
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    // 23.1.3.3
    static JSTaggedValue Delete(EcmaRuntimeCallInfo *argv);
    // 23.1.3.4
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    // 23.1.3.5
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    // 23.1.3.6
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    // 23.1.3.7
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    // 23.1.3.8
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    // 23.1.3.9
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    // 23.1.3.10
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    // 23.1.3.11
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);

    // es12 24.1.1.2 AddEntriesFromIterable ( target, iterable, adder )
    static JSTaggedValue AddEntriesFromIterable(JSThread *thread, const JSHandle<JSObject> &target,
                                                const JSHandle<JSTaggedValue> &iterable,
                                                const JSHandle<JSTaggedValue> &adder, ObjectFactory *factory);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetMapPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(MAP_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 4 : 4 more inline properties in Map.prototype
        //   (1) Map.prototype.constructor
        //   (2) Map.prototype [ @@toStringTag ]
        //   (3) Map.prototype [ @@iterator ] ( )
        //   (4) get Map.prototype.size
        return GetMapPrototypeFunctions().Size() + 4;
    }

private:
#define BUILTIN_MAP_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsMap::func, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array MAP_PROTOTYPE_FUNCTIONS = {
        BUILTIN_MAP_PROTOTYPE_FUNCTIONS(BUILTIN_MAP_FUNCTION_ENTRY)
    };

#undef BUILTIN_MAP_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_MAP_H
