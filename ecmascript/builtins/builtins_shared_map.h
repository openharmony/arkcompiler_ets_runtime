/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SHAREAD_MAP_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SHAREAD_MAP_H

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
    V("get",     Get,     1, INVALID)                           \
    /* Map.prototype.has ( key ) */                             \
    V("has",     Has,     1, MapHas)                            \
    /* Map.prototype.keys ( ) */                                \
    V("keys",    Keys,    0, MapKeys)                           \
    /* Map.prototype.set ( key, value ) */                      \
    V("set",     Set,     2, MapSet)                            \
    /* Map.prototype.values ( ) */                              \
    V("values",  Values,  0, MapValues)

namespace panda::ecmascript::builtins {
class BuiltinsSharedMap : public base::BuiltinsBase {
public:
    static JSTaggedValue Constructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Delete(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
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
        //   (3) Map.prototype [ @@iterator ]
        //   (4) get Map.prototype.size
        return GetMapPrototypeFunctions().Size() + 4;
    }

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(MAP_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(MAP_FUNCTION_PROPERTIES);
    }
private:
#define BUILTIN_MAP_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsSharedMap::func, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array MAP_PROTOTYPE_FUNCTIONS = {
        BUILTIN_MAP_PROTOTYPE_FUNCTIONS(BUILTIN_MAP_FUNCTION_ENTRY)
    };

#undef BUILTIN_MAP_FUNCTION_ENTRY

#define MAP_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array MAP_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_MAP_PROTOTYPE_FUNCTIONS(MAP_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("[Symbol.toStringTag]", false),
        std::pair<std::string_view, bool>("size", true),
        std::pair<std::string_view, bool>("[Symbol.iterator]", false)
    };

    static constexpr std::array MAP_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        std::pair<std::string_view, bool>("[Symbol.species]", true),
    };
#undef MAP_PROPERTIES_PAIR
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHAREAD_MAP_H
