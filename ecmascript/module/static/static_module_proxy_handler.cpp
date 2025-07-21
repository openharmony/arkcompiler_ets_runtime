/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/module/static/static_module_proxy_handler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_data_extractor.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript {
JSHandle<JSProxy> StaticModuleProxyHandler::CreateStaticModuleProxyHandler(JSThread *thread,
    const JSHandle<JSTaggedValue> exportObject)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> targetHandler(factory->NewJSFunction(vm->GetGlobalEnv()));

    JSHandle<JSTaggedValue> setPrototype(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::SetPrototype)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledSetPrototypeOfString(), setPrototype);
    
    JSHandle<JSTaggedValue> preventExtensions(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::PreventExtensions)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledPreventExtensionsString(), preventExtensions);

    JSHandle<JSTaggedValue> getOwnProperty(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::GetOwnProperty)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledGetOwnPropertyDescriptorString(), getOwnProperty);

    JSHandle<JSTaggedValue> defineOwnProperty(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::DefineOwnProperty)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledDefinePropertyString(), defineOwnProperty);

    JSHandle<JSTaggedValue> hasProperty(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::HasProperty)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledHasString(), hasProperty);

    JSHandle<JSTaggedValue> getProperty(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::GetProperty)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledGetString(), getProperty);

    JSHandle<JSTaggedValue> setProperty(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::SetProperty)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledSetString(), setProperty);

    JSHandle<JSTaggedValue> deleteProperty(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::DeleteProperty)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledDeletePropertyString(), deleteProperty);
    
    JSHandle<JSTaggedValue> ownPropertyKeys(factory->NewJSFunction(vm->GetGlobalEnv(),
        reinterpret_cast<void *>(StaticModuleProxyHandler::OwnPropertyKeys)));
    JSTaggedValue::SetProperty(thread, targetHandler,
        thread->GlobalConstants()->GetHandledOwnKeysString(), ownPropertyKeys);

    return JSProxy::ProxyCreate(thread, exportObject, targetHandler);
}

JSTaggedValue StaticModuleProxyHandler::GetProperty(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSHandle<JSTaggedValue> key(argv->GetCallArg(SECOND));
    // 1. Assert: IsPropertyKey(P) is true.
    // 2. If Type(P) is Symbol, then
    //   a. Return ? OrdinaryGet(O, P, Receiver).
    return GetPropertyInternal(thread, obj, key);
}

JSTaggedValue StaticModuleProxyHandler::GetPropertyInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
    const JSHandle<JSTaggedValue> &key)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. If Type(P) is Symbol, then
    //   a. Return ? OrdinaryGet(O, P, Receiver).
    return JSObject::GetProperty(thread, obj, key).GetValue().GetTaggedValue();
}


JSTaggedValue StaticModuleProxyHandler::OwnPropertyKeys(EcmaRuntimeCallInfo *argv)
{
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSThread *thread = argv->GetThread();
    JSHandle<TaggedArray> symbolKeys = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj)); //todo copy array
    JSHandle<JSArray> propertyKeys = JSArray::CreateArrayFromList(thread, symbolKeys);
    return propertyKeys.GetTaggedValue();
}

bool StaticModuleProxyHandler::GetOwnPropertyInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
    const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 1. If Type(P) is Symbol, return OrdinaryGetOwnProperty(O, P).
    if (key->IsSymbol()) {
        return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
    }
    // 2. Let value be ? O.[[Get]](P, O).
    JSHandle<JSTaggedValue> value(thread, StaticModuleProxyHandler::GetPropertyInternal(thread, obj, key));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (value->IsUndefined()) {
        return false;
    }
    // 3. Return PropertyDescriptor {
    //    [[Value]]: value, [[Writable]]: true, [[Enumerable]]: true, [[Configurable]]: false }.
    desc.SetValue(value);
    desc.SetEnumerable(true);
    desc.SetWritable(true);
    desc.SetConfigurable(false);
    return true;
}

JSTaggedValue StaticModuleProxyHandler::OwnEnumPropertyKeys(EcmaRuntimeCallInfo *argv)
{
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSThread *thread = argv->GetThread();
    JSHandle<TaggedArray> symbolKeys = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj)); //todo copy array
    JSHandle<JSArray> propertyKeys = JSArray::CreateArrayFromList(thread, symbolKeys);
    return propertyKeys.GetTaggedValue();
}

JSTaggedValue StaticModuleProxyHandler::PreventExtensions(EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue::True();
}

JSTaggedValue StaticModuleProxyHandler::DefineOwnProperty(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSHandle<JSTaggedValue> key(argv->GetCallArg(SECOND));
    JSHandle<JSTaggedValue> descObj(argv->GetCallArg(THIRD));
    PropertyDescriptor desc(thread);
    if (!descObj->IsUndefined()) {
        JSObject::ToPropertyDescriptor(thread, descObj, desc);
    }
    // 1. If Type(P) is Symbol, return ! OrdinaryDefineOwnProperty(O, P, Desc).
    if (key->IsSymbol()) {
        bool res = JSObject::OrdinaryDefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
        return JSTaggedValue(res);
    }

    // 2. Let current be ? O.[[GetOwnProperty]](P).
    PropertyDescriptor current(thread);
    // 3. If current is undefined, return false.
    if (!GetOwnPropertyInternal(thread, obj, key, current)) {
        return JSTaggedValue::False();
    }
    // 4. If Desc has a [[Configurable]] field and Desc.[[Configurable]] is true, return false.
    // 5. If Desc has an [[Enumerable]] field and Desc.[[Enumerable]] is false, return false.
    // 6. If IsAccessorDescriptor(Desc) is true, return false.
    // 7. If Desc has a [[Writable]] field and Desc.[[Writable]] is false, return false.
    if (desc.IsAccessorDescriptor()) {
        return JSTaggedValue::False();
    }
    if (desc.HasConfigurable() && desc.IsConfigurable()) {
        return JSTaggedValue::False();
    }
    if (desc.HasEnumerable() && !desc.IsEnumerable()) {
        return JSTaggedValue::False();
    }
    if (desc.HasWritable() && !desc.IsWritable()) {
        return JSTaggedValue::False();
    }

    // 8. If Desc has a [[Value]] field, return SameValue(Desc.[[Value]], current.[[Value]]).
    if (desc.HasValue()) {
        JSHandle<JSTaggedValue> descValue = desc.GetValue();
        JSHandle<JSTaggedValue> currentValue = current.GetValue();
        return JSTaggedValue(JSTaggedValue::SameValue(thread, descValue, currentValue));
    }

    // 9. Return true.
    return JSTaggedValue::True();
}

JSTaggedValue StaticModuleProxyHandler::HasProperty(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSHandle<JSTaggedValue> key(argv->GetCallArg(SECOND));
    return JSTaggedValue(JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key));
}

JSTaggedValue StaticModuleProxyHandler::SetPrototype(EcmaRuntimeCallInfo *argv)
{
    // 1. Assert: Either Type(V) is Object or Type(V) is Null.
    JSHandle<JSTaggedValue> proto(argv->GetCallArg(SECOND));
    ASSERT(proto->IsECMAObject() || proto->IsNull());
    return JSTaggedValue(proto->IsNull());
}

JSTaggedValue StaticModuleProxyHandler::GetPrototype(EcmaRuntimeCallInfo *argv)
{
    // 1. Assert: Either Type(V) is Object or Type(V) is Null.
    return JSTaggedValue::Null();
}


JSTaggedValue StaticModuleProxyHandler::GetOwnProperty(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSHandle<JSTaggedValue> key(argv->GetCallArg(SECOND));
    JSHandle<JSTaggedValue> descObj(argv->GetCallArg(THIRD));
    PropertyDescriptor desc(thread);
    if (!descObj->IsUndefined()) {
        JSObject::ToPropertyDescriptor(thread, descObj, desc);
    }
    return JSTaggedValue(GetOwnPropertyInternal(thread, obj, key, desc));
}

JSTaggedValue StaticModuleProxyHandler::SetProperty(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> mayThrow(argv->GetCallArg(FIRST));
    if (mayThrow->ToBoolean()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot assign to read only property of Object Module",
            JSTaggedValue::False());
    }
    return JSTaggedValue::False();
}

JSTaggedValue StaticModuleProxyHandler::DeleteProperty(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> obj(argv->GetCallArg(FIRST));
    JSHandle<JSTaggedValue> key(argv->GetCallArg(SECOND));
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. If Type(P) is Symbol, then
    //    Return ? OrdinaryDelete(O, P).
    if (key->IsSymbol()) {
        return JSTaggedValue(JSObject::DeleteProperty(thread, JSHandle<JSObject>(obj), key));
    }
    // 3. Let exports be O.[[Exports]].
    JSHandle<JSTaggedValue> value(thread, GetPropertyInternal(thread, obj, key));
    // 4. If P is an element of exports, return false.
    if (value->IsUndefined()) {
        return JSTaggedValue::True();
    }
    return JSTaggedValue::False();
}

JSTaggedValue StaticModuleProxyHandler::IsExtensible(EcmaRuntimeCallInfo *argv)
{
    return JSTaggedValue::False();
}
}  // namespace panda::ecmascript
