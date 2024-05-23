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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_REFLECT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_REFLECT_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_array.h"

// List of functions in Reflect, excluding the '@@' properties.
// V(name, func, length, stubIndex)
// where BuiltinsRefject::func refers to the native implementation of Reflect[name].
//       kungfu::BuiltinsStubCSigns::stubIndex refers to the builtin stub index, or INVALID if no stub available.
#define BUILTIN_REFLECT_FUNCTIONS(V)                                                    \
    /* Reflect.apply ( target, thisArgument, argumentsList ) */                         \
    V("apply",                    ReflectApply,                    3, ReflectApply)     \
    /* Reflect.construct ( target, argumentsList [ , newTarget ] ) */                   \
    V("construct",                ReflectConstruct,                2, ReflectConstruct) \
    /* Reflect.defineProperty ( target, propertyKey, attributes ) */                    \
    V("defineProperty",           ReflectDefineProperty,           3, INVALID)          \
    /* Reflect.deleteProperty ( target, propertyKey ) */                                \
    V("deleteProperty",           ReflectDeleteProperty,           2, INVALID)          \
    /* Reflect.get ( target, propertyKey [ , receiver ] ) */                            \
    V("get",                      ReflectGet,                      2, ReflectGet)       \
    /* Reflect.getOwnPropertyDescriptor ( target, propertyKey ) */                      \
    V("getOwnPropertyDescriptor", ReflectGetOwnPropertyDescriptor, 2, INVALID)          \
    /* Reflect.getPrototypeOf ( target ) */                                             \
    V("getPrototypeOf",           ReflectGetPrototypeOf,           1, ReflectGetPrototypeOf)  \
    /* Reflect.has ( target, propertyKey ) */                                           \
    V("has",                      ReflectHas,                      2, ReflectHas)       \
    /* Reflect.isExtensible ( target ) */                                               \
    V("isExtensible",             ReflectIsExtensible,             1, INVALID)          \
    /* Reflect.ownKeys ( target ) */                                                    \
    V("ownKeys",                  ReflectOwnKeys,                  1, INVALID)          \
    /* Reflect.preventExtensions ( target ) */                                          \
    V("preventExtensions",        ReflectPreventExtensions,        1, INVALID)          \
    /* Reflect.set ( target, propertyKey, V [ , receiver ] ) */                         \
    V("set",                      ReflectSet,                      3, INVALID)          \
    /* Reflect.setPrototypeOf ( target, proto ) */                                      \
    V("setPrototypeOf",           ReflectSetPrototypeOf,           2, INVALID)

namespace panda::ecmascript::builtins {
class BuiltinsReflect : public base::BuiltinsBase {
public:
    // ecma 26.1.1
    static JSTaggedValue ReflectApply(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReflectApplyInternal(JSThread *thread, JSHandle<JSTaggedValue> target,
                                              JSHandle<JSTaggedValue> thisArgument,
                                              JSHandle<JSTaggedValue> argumentsList);

    // ecma 26.1.2
    static JSTaggedValue ReflectConstruct(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReflectConstructInternal(JSThread *thread, JSHandle<JSTaggedValue> target,
                                                  JSHandle<TaggedArray> args, JSHandle<JSTaggedValue> newTarget);

    // ecma 26.1.3
    static JSTaggedValue ReflectDefineProperty(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.4
    static JSTaggedValue ReflectDeleteProperty(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.5
    static JSTaggedValue ReflectGet(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.6
    static JSTaggedValue ReflectGetOwnPropertyDescriptor(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.7
    static JSTaggedValue ReflectGetPrototypeOf(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.8
    static JSTaggedValue ReflectHas(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReflectHasInternal(JSThread *thread, JSHandle<JSTaggedValue> target,
                                            JSHandle<JSTaggedValue> key);

    // ecma 26.1.9
    static JSTaggedValue ReflectIsExtensible(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.10
    static JSTaggedValue ReflectOwnKeys(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.11
    static JSTaggedValue ReflectPreventExtensions(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.12
    static JSTaggedValue ReflectSet(EcmaRuntimeCallInfo *argv);

    // ecma 26.1.13
    static JSTaggedValue ReflectSetPrototypeOf(EcmaRuntimeCallInfo *argv);

    // Excluding the '@@' internal properties.
    static Span<const base::BuiltinFunctionEntry> GetReflectFunctions()
    {
        return Span<const base::BuiltinFunctionEntry>(REFLECT_FUNCTIONS);
    }

private:
#define BUILTINS_REFLECT_FUNCTION_ENTRY(name, method, length, id) \
    base::BuiltinFunctionEntry::Create(name, BuiltinsReflect::method, length, kungfu::BuiltinsStubCSigns::id),

    static constexpr std::array REFLECT_FUNCTIONS  = {
        BUILTIN_REFLECT_FUNCTIONS(BUILTINS_REFLECT_FUNCTION_ENTRY)
    };
#undef BUILTINS_REFLECT_FUNCTION_ENTRY
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_REFLECT_H
