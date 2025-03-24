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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SHARED_SET_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SHARED_SET_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// List of functions in Set, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsSet::func refers to the native implementation of Set.prototype[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
// The following functions are not listed:
//   - Set.prototype.keys ( ), which is strictly equal to Set.prototype.values
#define BUILTIN_SET_PROTOTYPE_FUNCTIONS(V)                      \
    /* Set.prototype.add ( value ) */                           \
    V("add",     Add,     1, SetAdd)                            \
    /* Set.prototype.clear ( ) */                               \
    V("clear",   Clear,   0, SetClear)                          \
    /* Set.prototype.delete ( value ) */                        \
    V("delete",  Delete,  1, SetDelete)                         \
    /* Set.prototype.entries ( ) */                             \
    V("entries", Entries, 0, SetEntries)                        \
    /* Set.prototype.forEach ( callbackfn [ , thisArg ] ) */    \
    V("forEach", ForEach, 1, SetForEach)                        \
    /* Set.prototype.has ( value ) */                           \
    V("has",     Has,     1, SetHas)                            \
    /* Set.prototype.values ( ) */                              \
    V("values",  Values,  0, SetValues)

namespace panda::ecmascript::builtins {
class BuiltinsSharedSet : public base::BuiltinsBase {
public:
    static JSTaggedValue Constructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Delete(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetSetPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(SET_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 5 : 5 more inline properties in Set.prototype
        //   (1) Set.prototype.constructor
        //   (2) Set.prototype [ @@toStringTag ]
        //   (3) Set.prototype [ @@iterator ]
        //   (4) get Set.prototype.size
        //   (5) Set.prototype.keys, which is not included in BuiltinsSharedSet::GetSetPrototypeFunctions()
        return GetSetPrototypeFunctions().Size() + 5;
    }

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(SET_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(SET_FUNCTION_PROPERTIES);
    }
private:
#define BUILTIN_SET_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsSharedSet::func, length, BUILTINS_STUB_ID(id)),

    static constexpr std::array SET_PROTOTYPE_FUNCTIONS = {
        BUILTIN_SET_PROTOTYPE_FUNCTIONS(BUILTIN_SET_FUNCTION_ENTRY)
    };

#undef BUILTIN_SET_FUNCTION_ENTRY

#define SET_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array SET_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_SET_PROTOTYPE_FUNCTIONS(SET_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("keys", false),
        std::pair<std::string_view, bool>("[Symbol.toStringTag]", false),
        std::pair<std::string_view, bool>("size", true),
        std::pair<std::string_view, bool>("[Symbol.iterator]", false)
    };

    static constexpr std::array SET_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        std::pair<std::string_view, bool>("[Symbol.species]", true),
    };
#undef SET_PROPERTIES_PAIR
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHARED_SET_H
