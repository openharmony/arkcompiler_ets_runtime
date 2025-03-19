/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_deque.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersDeque::DequeConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error =
            ContainerError::BusinessError(thread, ErrorFlag::IS_NULL_ERROR,
                                          "The Deque's constructor cannot be directly invoked");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
    obj->SetElements(thread, newElements);
    return obj.GetTaggedValue();
}

JSTaggedValue ContainersDeque::InsertFront(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, InsertFront);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The insertFront method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSAPIDeque::InsertFront(thread, JSHandle<JSAPIDeque>::Cast(self), value);

    return JSTaggedValue::True();
}


JSTaggedValue ContainersDeque::InsertEnd(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, InsertEnd);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The insertEnd method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSAPIDeque::InsertEnd(thread, JSHandle<JSAPIDeque>::Cast(self), value);

    return JSTaggedValue::True();
}

JSTaggedValue ContainersDeque::GetFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetFirst);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getFirst method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue firstElement = deque->GetFront();
    return firstElement;
}

JSTaggedValue ContainersDeque::GetLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetLast);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLast method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue lastElement = deque->GetTail();
    return lastElement;
}

JSTaggedValue ContainersDeque::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The has method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    bool isHas = deque->Has(value.GetTaggedValue());
    return GetTaggedBoolean(isHas);
}

JSTaggedValue ContainersDeque::PopFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, PopFirst);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The popFirst method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue firstElement = deque->PopFirst(thread);
    return firstElement;
}

JSTaggedValue ContainersDeque::PopLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, PopLast);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The popLast method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    JSTaggedValue lastElement = deque->PopLast(thread);
    return lastElement;
}

JSTaggedValue ContainersDeque::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPIDeque()) {
        if (thisHandle->IsJSProxy() && JSHandle<JSProxy>::Cast(thisHandle)->GetTarget().IsJSAPIDeque()) {
            thisHandle = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(thisHandle)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The forEach method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, callbackFnHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"callbackfn\" must be callable. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(thisHandle);
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    uint32_t first = deque->GetFirst();
    uint32_t last = deque->GetLast();

    JSHandle<TaggedArray> elements(thread, deque->GetElements());
    uint32_t capacity = elements->GetLength();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t index = 0;
    while (first != last) {
        JSTaggedValue kValue = deque->Get(index);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, 3); // 3:three args
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue, JSTaggedValue(index), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        ASSERT(capacity != 0);
        first = (first + 1) % capacity;
        index = index + 1;
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersDeque::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetIteratorObj);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSTaggedValue values = JSAPIDeque::GetIteratorObj(thread, JSHandle<JSAPIDeque>::Cast(self));

    return values;
}

JSTaggedValue ContainersDeque::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Deque, GetSize);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIDeque()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIDeque()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIDeque> deque = JSHandle<JSAPIDeque>::Cast(self);
    uint32_t length = deque->GetSize();

    return JSTaggedValue(length);
}
} // namespace panda::ecmascript::containers
