/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_stack.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersStack::StackConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error =
            ContainerError::BusinessError(thread, ErrorFlag::IS_NULL_ERROR,
                                          "The List's constructor cannot be directly invoked");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(obj);
    stack->SetTop(-1);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersStack::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, IsEmpty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The isEmpty method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    bool judge = stack->Empty();

    return GetTaggedBoolean(judge);
}

JSTaggedValue ContainersStack::Push(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Push);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The push method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSTaggedValue jsValue = JSAPIStack::Push(thread, JSHandle<JSAPIStack>::Cast(self), value);
    return jsValue;
}

JSTaggedValue ContainersStack::Peek(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Peek);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The peek method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    JSTaggedValue jsValue = stack->Peek();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return jsValue;
}

JSTaggedValue ContainersStack::Locate(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Locate);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The locate method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    int num = stack->Search(value);
    return JSTaggedValue(num);
}

JSTaggedValue ContainersStack::Pop(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Pop);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The pop method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    JSTaggedValue jsValue = stack->Pop();
    return jsValue;
}

JSTaggedValue ContainersStack::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPIStack()) {
        if (thisHandle->IsJSProxy() && JSHandle<JSProxy>::Cast(thisHandle)->GetTarget().IsJSAPIStack()) {
            thisHandle = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(thisHandle)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The forEach method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(thisHandle);
    int32_t len = stack->GetSize();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, callbackFnHandle.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"callbackfn\" must be callable. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t k = 0;
    uint32_t actLen = static_cast<uint32_t>(len + 1);
    while (k < actLen) {
        JSTaggedValue kValue = stack->Get(k);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, 3); // 3:three args
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue, JSTaggedValue(k), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        k++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersStack::Iterator(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Iterator);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIStackIterator> iter(factory->NewJSAPIStackIterator(JSHandle<JSAPIStack>::Cast(self)));
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersStack::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, GetLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIStack()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    int32_t len = (JSHandle<JSAPIStack>::Cast(self))->GetSize();
    return JSTaggedValue(len + 1);
}
}  // namespace panda::ecmascript::containers
