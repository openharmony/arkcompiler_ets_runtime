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

#include "ecmascript/containers/containers_arraylist.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/base/array_helper.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersArrayList::ArrayListConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error =
            ContainerError::BusinessError(thread, ErrorFlag::IS_NULL_ERROR,
                                          "The ArrayList's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> newTaggedArray = factory->NewTaggedArray(JSAPIArrayList::DEFAULT_CAPACITY_LENGTH);
    obj->SetElements(thread, newTaggedArray);
    return obj.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The add method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    return GetTaggedBoolean(JSAPIArrayList::Add(thread, JSHandle<JSAPIArrayList>::Cast(self), value));
}

JSTaggedValue ContainersArrayList::Insert(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Insert);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The insert method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (index->IsDouble()) {
        // Math.floor(1) will produce TaggedDouble, we need to cast into TaggedInt
        // For integer which is greater than INT32_MAX, it will remain TaggedDouble
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIArrayList::Insert(thread, JSHandle<JSAPIArrayList>::Cast(self), value, JSTaggedValue::ToUint32(thread, index));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersArrayList::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The clear method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSAPIArrayList::Clear(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::Clone(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Clone);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, BIND_ERROR,
                                                                "The clone method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIArrayList> newArrayList = JSAPIArrayList::Clone(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return newArrayList.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, BIND_ERROR,
                                                                "The has method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    bool isHas = JSHandle<JSAPIArrayList>::Cast(self)->Has(value.GetTaggedValue());

    return GetTaggedBoolean(isHas);
}

JSTaggedValue ContainersArrayList::GetCapacity(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);

    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetCapacity);
    JSThread *thread = argv->GetThread();

    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getCapacity method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    uint32_t capacity = JSAPIArrayList::GetCapacity(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return JSTaggedValue(capacity);
}

JSTaggedValue ContainersArrayList::IncreaseCapacityTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, IncreaseCapacityTo);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The increaseCapacityTo method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> newCapacity = GetCallArg(argv, 0);
    if (!newCapacity->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, newCapacity);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"newCapacity\" must be number. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSAPIArrayList::IncreaseCapacityTo(thread, JSHandle<JSAPIArrayList>::Cast(self),
                                       JSTaggedValue::ToUint32(thread, newCapacity));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::TrimToCurrentLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, TrimToCurrentLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The trimToCurrentLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSAPIArrayList::TrimToCurrentLength(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The get method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    JSTaggedValue element = JSHandle<JSAPIArrayList>::Cast(self)->Get(thread, JSTaggedValue::ToUint32(thread, value));

    return element;
}

JSTaggedValue ContainersArrayList::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetIndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getIndexOf method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    return JSTaggedValue(JSAPIArrayList::GetIndexOf(thread, JSHandle<JSAPIArrayList>::Cast(self), value));
}

JSTaggedValue ContainersArrayList::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, IsEmpty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The isEmpty method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSTaggedValue(JSAPIArrayList::IsEmpty(JSHandle<JSAPIArrayList>::Cast(self)));
}

JSTaggedValue ContainersArrayList::GetLastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetLastIndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLastIndexOf method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    return JSTaggedValue(JSAPIArrayList::GetLastIndexOf(thread, JSHandle<JSAPIArrayList>::Cast(self), value));
}

JSTaggedValue ContainersArrayList::RemoveByIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, RemoveByIndex);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The removeByIndex method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (value->IsDouble()) {
        value = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(value->GetDouble()));
    }
    if (!value->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, value.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSTaggedValue result =
        JSAPIArrayList::RemoveByIndex(thread,
                                      JSHandle<JSAPIArrayList>::Cast(self), JSTaggedValue::ToUint32(thread, value));

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersArrayList::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Remove);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The remove method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    bool isRemove = JSAPIArrayList::Remove(thread, JSHandle<JSAPIArrayList>::Cast(self), value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return GetTaggedBoolean(isRemove);
}

JSTaggedValue ContainersArrayList::RemoveByRange(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, RemoveByRange);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, BIND_ERROR,
                                                                "The removeByRange method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 1);
    if (!startIndex->IsInteger()) {
        std::ostringstream oss;
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be number. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        std::ostringstream oss;
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be number. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSTaggedValue result =
        JSAPIArrayList::RemoveByRange(thread, JSHandle<JSAPIArrayList>::Cast(self), startIndex, endIndex);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersArrayList::ReplaceAllElements(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, ReplaceAllElements);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The replaceAllElements method cannot be bound");
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
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    return JSAPIArrayList::ReplaceAllElements(thread, self, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersArrayList::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The set method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);
    JSHandle<JSAPIArrayList>::Cast(self)->Set(thread, JSTaggedValue::ToUint32(thread, index), value.GetTaggedValue());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersArrayList::SubArrayList(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, SubArrayList);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The subArrayList method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> value1 = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value2 = GetCallArg(argv, 1);
    if (!value1->IsInteger()) {
        std::ostringstream oss;
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, value1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be number. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!value2->IsInteger()) {
        std::ostringstream oss;
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, value2);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be number. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSTaggedValue newArrayList =
        JSAPIArrayList::SubArrayList(thread, JSHandle<JSAPIArrayList>::Cast(self), value1, value2);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArrayList;
}

JSTaggedValue ContainersArrayList::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(argv->GetThread(), Array, Sort);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The sort method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable() && !callbackFnHandle->IsNull()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, callbackFnHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"comparator\" must be callable. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    auto obj = JSHandle<JSAPIArrayList>::Cast(self);
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    if (thread->GetEcmaVM()->GetVMAPIVersion() < API16) {
        JSAPIArrayList::SortElements(thread, elements, callbackFnHandle);
    } else {
        // elements.length equal or greater than arraylist.length
        uint32_t length = obj->GetLength().GetArrayLength();
        JSHandle<TaggedArray> res = JSHandle<TaggedArray>(thread,
            JSAPIArrayList::SortElementsWithCopy(thread, elements, callbackFnHandle));
        obj->SetElements(thread, res);
        obj->SetLength(thread, JSTaggedValue(length));
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersArrayList::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSTaggedValue(JSHandle<JSAPIArrayList>::Cast(self)->GetSize());
}

JSTaggedValue ContainersArrayList::ConvertToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, ConvertToArray);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The convertToArray method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSAPIArrayList> arrayList = JSHandle<JSAPIArrayList>::Cast(self);
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();

    uint32_t length = arrayList->GetLength().GetArrayLength();
    array->SetArrayLength(thread, length);

    JSHandle<TaggedArray> srcElements(thread, arrayList->GetElements());
    JSHandle<TaggedArray> dstElements = factory->NewAndCopyTaggedArray(srcElements, length, length);
    array->SetElements(thread, dstElements);
    return array.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
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

    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    return JSAPIArrayList::ForEach(thread, self, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersArrayList::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, GetIteratorObj);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIArrayList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIArrayList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSTaggedValue values = JSAPIArrayList::GetIteratorObj(thread, JSHandle<JSAPIArrayList>::Cast(self));

    return values;
}
}  // namespace panda::ecmascript::containers
