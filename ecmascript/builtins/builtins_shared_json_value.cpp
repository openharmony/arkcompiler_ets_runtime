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

#include "ecmascript/builtins/builtins_shared_json_value.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/shared_objects/js_shared_json_value.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsJsonValue::JSONObjectConstructor([[maybe_unused]]EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONObjectConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONObject's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2. Get the initial value.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsJSSharedMap()) {
        JSTaggedValue error = containers::ContainerError::ParamError(
            thread, "Parameter error. Only accept sendable map.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, value);
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsJsonValue::JSONTrueConstructor([[maybe_unused]]EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONFalseConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONFalse's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, JSTaggedValue::True());
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsJsonValue::JSONFalseConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONFalseConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONFalse's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, JSTaggedValue::False());
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsJsonValue::JSONNullConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONNullConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONNull's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, JSTaggedValue::Null());
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsJsonValue::JSONNumberConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONNumberConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONObject's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2. Get the initial value.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsNumber()) {
        JSTaggedValue error = containers::ContainerError::ParamError(
            thread, "Parameter error. Only accept number.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, value);
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsJsonValue::JSONStringConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONNumberConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONObject's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2. Get the initial value.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsString()) {
        JSTaggedValue error = containers::ContainerError::ParamError(
            thread, "Parameter error. Only accept string.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, value);
    return obj.GetTaggedValue();
}

JSTaggedValue BuiltinsJsonValue::JSONArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, JSONNumberConstructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONObject's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2. Get the initial value.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsJSSharedArray()) {
        JSTaggedValue error = containers::ContainerError::ParamError(
            thread, "Parameter error. Only accept ArkTS Array.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
    jsonValue->SetValue(thread, value);
    return obj.GetTaggedValue();
}


// JSTaggedValue BuiltinsJsonValue::ConstructorForObject([[maybe_unused]]EcmaRuntimeCallInfo *argv)
// {
//     BUILTINS_API_TRACE(argv->GetThread(), BuiltinsJsonValue, ConstructorForObject);
//     JSThread *thread = argv->GetThread();
//     [[maybe_unused]] EcmaHandleScope handleScope(thread);
//     ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
//     // 1. If NewTarget is undefined, throw exception
//     JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
//     if (newTarget->IsUndefined()) {
//         JSTaggedValue error = containers::ContainerError::BusinessError(
//             thread, containers::ErrorFlag::IS_NULL_ERROR, "The JSONValue's constructor cannot be directly invoked.");
//         THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
//     }
//     // 2. Get the initial value.
//     JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
//     if (!value->IsJSSharedMap()) {
//         JSTaggedValue error = containers::ContainerError::ParamError(
//             thread, "Parameter error. Only accept sendable map.");
//         THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
//     }
//     // 3.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
//     JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
//     ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
//     JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
//     ASSERT(obj.GetTaggedValue().IsInSharedHeap());
//     // 3.returnIfAbrupt()
//     RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
//     JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(obj);
//     jsonValue->SetValue(thread, value);

//     jsonValue->GetValue().D();
//     jsonValue.Dump();
//     return obj.GetTaggedValue();
// }


JSTaggedValue BuiltinsJsonValue::Get([[maybe_unused]]EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    JSHandle<JSSharedJSONValue> jsonValue = JSHandle<JSSharedJSONValue>::Cast(self);
    return jsonValue->GetValue();
    // JSTaggedValue value = jsonValue->GetValue();
    // if (!value.IsJSSharedMap()) {
    //     auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
    //                                                            "The get method cannot be bound.");
    //     THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    // }
    // //这个是不是要先getvalue拿到sharedmap，再去查找？
    // JSSharedMap *jsMap = JSSharedMap::Cast(value.GetTaggedObject());
    // JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    // JSTaggedValue value1 = jsMap->Get(thread, key.GetTaggedValue());
    // return value1;
    // return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::builtins
