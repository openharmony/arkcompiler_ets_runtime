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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SHARED_JSON_VALUE_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SHARED_JSON_VALUE_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

// All types of %TypedArray%.
// V(Type, ctorName, TYPE) where JSType::JS_##TYPE is the type index.
#define BUILTIN_SHARED_JSON_VALUE_TYPES(V)              \
    V(JSONObject, SharedJSONObject, SHARED_JSON_OBJECT) \
    V(JSONTrue, SharedJSONTrue, SHARED_JSON_TRUE)       \
    V(JSONFalse, SharedJSONFalse, SHARED_JSON_FALSE)    \
    V(JSONNull, SharedJSONNull, SHARED_JSON_NULL)       \
    V(JSONNumber, SharedJSONNumber, SHARED_JSON_NUMBER) \
    V(JSONString, SharedJSONString, SHARED_JSON_STRING) \
    V(JSONArray, SharedJSONArray, SHARED_JSON_ARRAY)

// List of functions in Map.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsMap::func refers to the native implementation of Map.prototype[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_JSON_VALUE_PROTOTYPE_FUNCTIONS(V)                      \
    /* json.value.prototype.clear ( ) */                                      \
    V("get",   Get,   0, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsJsonValue : public base::BuiltinsBase {
public:
    static JSTaggedValue JSONObjectConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue JSONTrueConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue JSONFalseConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue JSONNullConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue JSONNumberConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue JSONStringConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue JSONArrayConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);

    // Excluding the constructor and '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetJsonValuePrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(JSON_VALUE_PROTOTYPE_FUNCTIONS);
    }

    static size_t GetNumPrototypeInlinedProperties()
    {
        // 4 : 4 more inline properties in Map.prototype
        //   (1) Map.prototype.constructor
        //   (2) Map.prototype [ @@toStringTag ]
        //   (3) Map.prototype [ @@iterator ] -- removed
        //   (4) get Map.prototype.size -- removed
        return GetJsonValuePrototypeFunctions().Size() + 2; // 2 : constructor and prototype
    }

    static Span<const std::pair<std::string_view, bool>> GetPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(JSON_VALUE_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(JSON_VALUE_FUNCTION_PROPERTIES);
    }
private:
#define BUILTIN_JSON_VALUE_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsJsonValue::func, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array JSON_VALUE_PROTOTYPE_FUNCTIONS = {
        BUILTIN_JSON_VALUE_PROTOTYPE_FUNCTIONS(BUILTIN_JSON_VALUE_FUNCTION_ENTRY)
    };

#undef BUILTIN_JSON_VALUE_FUNCTION_ENTRY

#define JSON_VALUE_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array JSON_VALUE_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_JSON_VALUE_PROTOTYPE_FUNCTIONS(JSON_VALUE_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("[Symbol.toStringTag]", false),
    };

    static constexpr std::array JSON_VALUE_FUNCTION_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
    };
#undef JSON_VALUE_PROPERTIES_PAIR
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHARED_JSON_VALUE_H
