/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_lazy_callback.h"

#include "ecmascript/builtins/builtins.h"
#include "ecmascript/layout_info-inl.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsLazyCallback::Date(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "Date");

    JSHandle<JSTaggedValue> objFuncPrototypeVal = env->GetObjectFunctionPrototype();
    Builtins builtin(thread, factory, vm);
    builtin.InitializeDate(env, objFuncPrototypeVal);
    return env->GetDateFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::Set(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "Set");

    Builtins builtin(thread, factory, vm);
    JSHandle<JSTaggedValue> objFuncPrototypeVal = env->GetObjectFunctionPrototype();
    builtin.InitializeSet(env, objFuncPrototypeVal);
    return env->GetBuiltinsSetFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::Map(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "Map");

    Builtins builtin(thread, factory, vm);
    JSHandle<JSTaggedValue> objFuncPrototypeVal = env->GetObjectFunctionPrototype();
    builtin.InitializeMap(env, objFuncPrototypeVal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return env->GetBuiltinsMapFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::WeakMap(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "WeakMap");
    Builtins builtin(thread, factory, vm);
    JSHandle<JSHClass> objFuncClass(env->GetObjectFunctionClass());
    builtin.InitializeWeakMap(env, objFuncClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return env->GetBuiltinsWeakMapFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::WeakSet(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "WeakSet");
    Builtins builtin(thread, factory, vm);
    JSHandle<JSHClass> objFuncClass(env->GetObjectFunctionClass());
    builtin.InitializeWeakSet(env, objFuncClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return env->GetBuiltinsWeakSetFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::WeakRef(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "WeakRef");
    Builtins builtin(thread, factory, vm);
    JSHandle<JSHClass> objFuncClass(env->GetObjectFunctionClass());
    builtin.InitializeWeakRef(env, objFuncClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return env->GetBuiltinsWeakRefFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::FinalizationRegistry(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "FinalizationRegistry");
    Builtins builtin(thread, factory, vm);
    JSHandle<JSHClass> objFuncClass(env->GetObjectFunctionClass());
    builtin.InitializeFinalizationRegistry(env, objFuncClass);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return env->GetBuiltinsFinalizationRegistryFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::TypedArray(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "TypedArray");

#define RESET_TYPED_ARRAY_INTERNAL_ATTR(type)                                                             \
    ResetLazyInternalAttr(thread, obj, #type);

ITERATE_TYPED_ARRAY(RESET_TYPED_ARRAY_INTERNAL_ATTR)
#undef RESET_TYPED_ARRAY_INTERNAL_ATTR

    Builtins builtin(thread, factory, vm);
    JSHandle<JSTaggedValue> objFuncPrototypeVal = env->GetObjectFunctionPrototype();
    builtin.InitializeTypedArray(env, objFuncPrototypeVal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return env->GetTypedArrayFunction().GetTaggedValue();
}

#define TYPED_ARRAY_CALLBACK(type)                                                                       \
    JSTaggedValue BuiltinsLazyCallback::type(JSThread *thread, const JSHandle<JSObject> &obj)            \
    {                                                                                                    \
        [[maybe_unused]] EcmaHandleScope scope(thread);                                                  \
        LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;                                         \
        EcmaVM *vm = thread->GetEcmaVM();                                                                \
        auto env = vm->GetGlobalEnv();                                                                   \
        TypedArray(thread, obj);                                                                         \
        return env->Get##type##Function().GetTaggedValue();                                              \
    }

ITERATE_TYPED_ARRAY(TYPED_ARRAY_CALLBACK)
#undef TYPED_ARRAY_CALLBACK

JSTaggedValue BuiltinsLazyCallback::ArrayBuffer(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "ArrayBuffer");
    Builtins builtin(thread, factory, vm);
    JSHandle<JSHClass> objFuncClass(env->GetObjectFunctionClass());
    builtin.InitializeArrayBuffer(env, objFuncClass);
    return env->GetArrayBufferFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::DataView(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "DataView");

    Builtins builtin(thread, factory, vm);
    JSHandle<JSTaggedValue> objFuncPrototypeVal = env->GetObjectFunctionPrototype();
    builtin.InitializeDataView(env, objFuncPrototypeVal);
    return env->GetDataViewFunction().GetTaggedValue();
}

JSTaggedValue BuiltinsLazyCallback::SharedArrayBuffer(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    ResetLazyInternalAttr(thread, obj, "SharedArrayBuffer");
    Builtins builtin(thread, factory, vm);
    JSHandle<JSHClass> objFuncClass(env->GetObjectFunctionClass());
    builtin.InitializeSharedArrayBuffer(env, objFuncClass);
    return env->GetSharedArrayBufferFunction().GetTaggedValue();
}

#ifdef ARK_SUPPORT_INTL
#define INTL_CALLBACK(type)                                                                       \
    JSTaggedValue BuiltinsLazyCallback::type(JSThread *thread, const JSHandle<JSObject> &obj)     \
    {                                                                                             \
        [[maybe_unused]] EcmaHandleScope scope(thread);                                           \
        LOG_ECMA(DEBUG) << "BuiltinsLazyCallback::" << __func__;                                  \
        EcmaVM *vm = thread->GetEcmaVM();                                                         \
        auto env = vm->GetGlobalEnv();                                                            \
        ObjectFactory *factory = vm->GetFactory();                                                \
        ResetLazyInternalAttr(thread, obj, #type);                                                \
        Builtins builtin(thread, factory, vm);                                                    \
        builtin.Initialize##type(env);                                                            \
        return env->Get##type##Function().GetTaggedValue();                                       \
    }

ITERATE_INTL(INTL_CALLBACK)
#undef INTL_CALLBACK
#endif

void BuiltinsLazyCallback::ResetLazyInternalAttr(JSThread *thread, const JSHandle<JSObject> &object,
    const char *name)
{
    JSHClass *hclass = object->GetClass();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8ReadOnly(name));
    if (LIKELY(!hclass->IsDictionaryMode())) {
        LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject());
        uint32_t propsNumber = hclass->NumberOfProps();
        int entry = layoutInfo->FindElementWithCache(thread, hclass, key.GetTaggedValue(), propsNumber);
        if (entry != -1) {
            PropertyAttributes attr(layoutInfo->GetAttr(thread, entry));
            attr.SetIsAccessor(false);
            layoutInfo->SetNormalAttr(thread, entry, attr);
        }
    } else {
        TaggedArray *array = TaggedArray::Cast(object->GetProperties(thread).GetTaggedObject());
        ASSERT(array->IsDictionaryMode());
        NameDictionary *dict = NameDictionary::Cast(array);
        int entry = dict->FindEntry(thread, key.GetTaggedValue());
        if (entry != -1) {
            auto attr = dict->GetAttributes(thread, entry);
            attr.SetIsAccessor(false);
            dict->SetAttributes(thread, entry, attr);
        }
    }
}
}  // namespace panda::ecmascript::builtins
