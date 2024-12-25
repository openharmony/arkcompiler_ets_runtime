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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_OBJECT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_OBJECT_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"

// List of functions in Object, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsObject::func refers to the native implementation of Object[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.

#define BUILTIN_OBJECT_FUNCTIONS(V)                                                                 \
    /* Object.assign ( target, ...sources ) */                                                      \
    V("assign",                   Assign,                   2, ObjectAssign)                        \
    /* Object.create ( O, Properties ) */                                                           \
    V("create",                   Create,                   2, ObjectCreate)                        \
    /* Object.defineProperties ( O, Properties ) */                                                 \
    V("defineProperties",         DefineProperties,         2, INVALID)                             \
    /* Object.defineProperty ( O, P, Attributes ) */                                                \
    V("defineProperty",           DefineProperty,           3, INVALID)                             \
    /* Object.entries ( O ) */                                                                      \
    V("entries",                  Entries,                  1, ObjectEntries)                       \
    /* Object.freeze ( O ) */                                                                       \
    V("freeze",                   Freeze,                   1, INVALID)                             \
    /* Object.fromEntries ( iterable ) */                                                           \
    V("fromEntries",              FromEntries,              1, INVALID)                             \
    /* Object.getOwnPropertyDescriptor ( O, P ) */                                                  \
    V("getOwnPropertyDescriptor", GetOwnPropertyDescriptor, 2, INVALID)                             \
    /* Object.getOwnPropertyDescriptors ( O ) */                                                    \
    V("getOwnPropertyDescriptors", GetOwnPropertyDescriptors, 1, ObjectGetOwnPropertyDescriptors)   \
    /* Object.getOwnPropertyNames ( O ) */                                                          \
    V("getOwnPropertyNames",      GetOwnPropertyNames,      1, ObjectGetOwnPropertyNames)           \
    /* Object.getOwnPropertySymbols ( O ) */                                                        \
    V("getOwnPropertySymbols",    GetOwnPropertySymbols,    1, ObjectGetOwnPropertySymbols)         \
    /* Object.getPrototypeOf ( O ) */                                                               \
    V("getPrototypeOf",           GetPrototypeOf,           1, ObjectGetPrototypeOf)                \
    /* Object.hasOwn ( O, P ) */                                                                    \
    V("hasOwn",                   HasOwn,                   2, INVALID)                             \
    /* Object.is ( value1, value2 ) */                                                              \
    V("is",                       Is,                       2, ObjectIs)                             \
    /* Object.isExtensible ( O ) */                                                                 \
    V("isExtensible",             IsExtensible,             1, INVALID)                             \
    /* Object.isFrozen ( O ) */                                                                     \
    V("isFrozen",                 IsFrozen,                 1, ObjectIsFrozen)                      \
    /* Object.isSealed ( O ) */                                                                     \
    V("isSealed",                 IsSealed,                 1, ObjectIsSealed)                      \
    /* Object.keys ( O ) */                                                                         \
    V("keys",                     Keys,                     1, ObjectKeys)                          \
    /* Object.preventExtensions ( O ) */                                                            \
    V("preventExtensions",        PreventExtensions,        1, INVALID)                             \
    /* Object.seal ( O ) */                                                                         \
    V("seal",                     Seal,                     1, INVALID)                             \
    /* Object.setPrototypeOf ( O, proto ) */                                                        \
    V("setPrototypeOf",           SetPrototypeOf,           2, ObjectSetPrototypeOf)                \
    /* Object.values ( O ) */                                                                       \
    V("values",                   Values,                   1, INVALID)

// List of functions in Object.prototype, excluding the constructor and '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsObject::func refers to the native implementation of Object.prototype[name].
#define BUILTIN_OBJECT_PROTOTYPE_FUNCTIONS(V)                                       \
    V("createRealm",          CreateRealm,          0, INVALID)                     \
    /* Object.prototype.hasOwnProperty ( V ) */                                     \
    V("hasOwnProperty",       HasOwnProperty,       1, ObjectHasOwnProperty)        \
    /* Object.prototype.isPrototypeOf ( V ) */                                      \
    V("isPrototypeOf",        IsPrototypeOf,        1, ObjectIsPrototypeOf)         \
    /* Object.prototype.propertyIsEnumerable ( V ) */                               \
    V("propertyIsEnumerable", PropertyIsEnumerable, 1, INVALID)                     \
    /* Object.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] ) */         \
    V("toLocaleString",       ToLocaleString,       0, INVALID)                     \
    /* Object.prototype.toString ( ) */                                             \
    V("toString",             ToString,             0, ObjectToStringFunc)          \
    /* Object.prototype.valueOf ( ) */                                              \
    V("valueOf",              ValueOf,              0, INVALID)

namespace panda::ecmascript::builtins {
enum class KeyType : uint8_t {
    STRING_TYPE = 0,
    SYMBOL_TYPE,
};

class BuiltinsObject : public base::BuiltinsBase {
public:
    // 19.1.1.1Object ( [ value ] )
    static JSTaggedValue ObjectConstructor(EcmaRuntimeCallInfo *argv);

    // 19.1.2.1Object.assign ( target, ...sources )
    static JSTaggedValue Assign(EcmaRuntimeCallInfo *argv);
    // 19.1.2.2Object.create ( O [ , Properties ] )
    static JSTaggedValue Create(EcmaRuntimeCallInfo *argv);
    // 19.1.2.3Object.defineProperties ( O, Properties )
    static JSTaggedValue DefineProperties(EcmaRuntimeCallInfo *argv);
    // 19.1.2.4Object.defineProperty ( O, P, Attributes )
    static JSTaggedValue DefineProperty(EcmaRuntimeCallInfo *argv);
    // 19.1.2.5Object.freeze ( O )
    static JSTaggedValue Freeze(EcmaRuntimeCallInfo *argv);
    // 19.1.2.6Object.getOwnPropertyDescriptor ( O, P )
    static JSTaggedValue GetOwnPropertyDescriptor(EcmaRuntimeCallInfo *argv);
    // 19.1.2.7Object.getOwnPropertyNames ( O )
    static JSTaggedValue GetOwnPropertyNames(EcmaRuntimeCallInfo *argv);
    // 19.1.2.8Object.getOwnPropertySymbols ( O )
    static JSTaggedValue GetOwnPropertySymbols(EcmaRuntimeCallInfo *argv);
    // 19.1.2.9Object.getPrototypeOf ( O )
    static JSTaggedValue GetPrototypeOf(EcmaRuntimeCallInfo *argv);
    // 19.1.2.10Object.is ( value1, value2 )
    static JSTaggedValue Is(EcmaRuntimeCallInfo *argv);
    // 19.1.2.11Object.isExtensible ( O )
    static JSTaggedValue IsExtensible(EcmaRuntimeCallInfo *argv);
    // 19.1.2.12Object.isFrozen ( O )
    static JSTaggedValue IsFrozen(EcmaRuntimeCallInfo *argv);
    // 19.1.2.13Object.isSealed ( O )
    static JSTaggedValue IsSealed(EcmaRuntimeCallInfo *argv);
    // 19.1.2.14 Object.keys(O)
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    // 20.1.2.22 Object.values(O)
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    // 19.1.2.15 Object.preventExtensions(O)
    static JSTaggedValue PreventExtensions(EcmaRuntimeCallInfo *argv);
    // 19.1.2.17 Object.seal(O)
    static JSTaggedValue Seal(EcmaRuntimeCallInfo *argv);
    // 19.1.2.18 Object.setPrototypeOf(O, proto)
    static JSTaggedValue SetPrototypeOf(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue GetOwnPropertyDescriptors(EcmaRuntimeCallInfo *argv);

    // 19.1.3.2 Object.prototype.hasOwnProperty(V)
    static JSTaggedValue HasOwnProperty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasOwnPropertyInternal(JSThread *thread, JSHandle<JSTaggedValue> thisValue,
                                                JSHandle<JSTaggedValue> prop);
    // 19.1.3.3 Object.prototype.isPrototypeOf(V)
    static JSTaggedValue IsPrototypeOf(EcmaRuntimeCallInfo *argv);
    // 19.1.3.4 Object.prototype.propertyIsEnumerable(V)
    static JSTaggedValue PropertyIsEnumerable(EcmaRuntimeCallInfo *argv);
    // 19.1.3.5 Object.prototype.toLocaleString([reserved1[, reserved2]])
    static JSTaggedValue ToLocaleString(EcmaRuntimeCallInfo *argv);
    // 19.1.3.6 Object.prototype.toString()
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 19.1.3.7 Object.prototype.valueOf()
    static JSTaggedValue ValueOf(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue CreateRealm(EcmaRuntimeCallInfo *argv);
    // 20.1.2.5 Object.entries ( O )
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    // 20.1.2.7 Object.fromEntries ( iterable )
    static JSTaggedValue FromEntries(EcmaRuntimeCallInfo *argv);
    // B.2.2.1 Object.prototype.__proto__
    static JSTaggedValue ProtoGetter(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ProtoSetter(EcmaRuntimeCallInfo *argv);

    // 20.1.2.7.1 CreateDataPropertyOnObject Functions
    static JSTaggedValue CreateDataPropertyOnObjectFunctions(EcmaRuntimeCallInfo *argv);
    // 20.1.2.13 Object.hasOwn ( O, P )
    static JSTaggedValue HasOwn(EcmaRuntimeCallInfo *argv);

    static Span<const base::BuiltinFunctionEntry> GetObjectFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(OBJECT_FUNCTIONS);
    }

    // Excluding the constructor and '@@' internal properties
    static Span<const base::BuiltinFunctionEntry> GetObjectPrototypeFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(OBJECT_PROTOTYPE_FUNCTIONS);
    }

    static JSTaggedValue AssignTaggedValue(JSThread *thread, const JSHandle<JSTaggedValue> &source,
                                           const JSHandle<JSObject> &toAssign);

    static Span<const std::pair<std::string_view, bool>> GetFunctionPrototypeProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(OBJECT_PROTOTYPE_PROPERTIES);
    }

    static Span<const std::pair<std::string_view, bool>> GetFunctionProperties()
    {
        return Span<const std::pair<std::string_view, bool>>(OBJECT_PROPERTIES);
    }

private:
#define BUILTIN_OBJECT_FUNCTION_ENTRY(name, func, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsObject::func, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array OBJECT_FUNCTIONS = {
        BUILTIN_OBJECT_FUNCTIONS(BUILTIN_OBJECT_FUNCTION_ENTRY)
    };
    static constexpr std::array OBJECT_PROTOTYPE_FUNCTIONS = {
        BUILTIN_OBJECT_PROTOTYPE_FUNCTIONS(BUILTIN_OBJECT_FUNCTION_ENTRY)
    };
#undef BUILTIN_OBJECT_FUNCTION_ENTRY

#define OBJECT_PROPERTIES_PAIR(name, func, length, id) \
    std::pair<std::string_view, bool>(name, false),

    static constexpr std::array OBJECT_PROTOTYPE_PROPERTIES = {
        std::pair<std::string_view, bool>("constructor", false),
        BUILTIN_OBJECT_PROTOTYPE_FUNCTIONS(OBJECT_PROPERTIES_PAIR)
        std::pair<std::string_view, bool>("__proto__", true),
    };

    static constexpr std::array OBJECT_PROPERTIES = {
        std::pair<std::string_view, bool>("length", false),
        std::pair<std::string_view, bool>("name", false),
        std::pair<std::string_view, bool>("prototype", false),
        BUILTIN_OBJECT_FUNCTIONS(OBJECT_PROPERTIES_PAIR)

    };
#undef OBJECT_PROPERTIES_PAIR

    static JSTaggedValue ObjectDefineProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &prop);
    static JSTaggedValue GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const KeyType &type);
    static JSTaggedValue GetBuiltinObjectToString(JSThread *thread, const JSHandle<JSObject> &object);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_OBJECT_H
