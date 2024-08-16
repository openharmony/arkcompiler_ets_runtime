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

#include "ecmascript/containers/containers_plainarray.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_iterator.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersPlainArray::PlainArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error =
            ContainerError::BusinessError(thread, ErrorFlag::IS_NULL_ERROR,
                                          "The PlainArray's constructor cannot be directly invoked");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPIPlainArray> plainArray = JSHandle<JSAPIPlainArray>::Cast(obj);
    JSHandle<TaggedArray> keys =
        JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> values =
        JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
    plainArray->SetKeys(thread, keys);
    plainArray->SetValues(thread, values);
    return obj.GetTaggedValue();
}

JSTaggedValue ContainersPlainArray::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The add method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));
    if (key->IsDouble()) {
        key = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(key->GetDouble()));
    }
    if (!key->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"key\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray::Add(thread, JSHandle<JSAPIPlainArray>::Cast(self), key, value);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersPlainArray::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The clear method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    array->Clear(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersPlainArray::Clone(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Clone);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The clone method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIPlainArray> newPlainArray =
        JSAPIPlainArray::Clone(thread, JSHandle<JSAPIPlainArray>::Cast(self));
    return newPlainArray.GetTaggedValue();
}

JSTaggedValue ContainersPlainArray::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The has method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (value->IsDouble()) {
        value = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(value->GetDouble()));
    }
    if (!value->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, value.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"key\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t key = value->GetNumber();
    bool result = array->Has(key);
    return JSTaggedValue(result);
}

JSTaggedValue ContainersPlainArray::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The get method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    if (key->IsDouble()) {
        key = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(key->GetDouble()));
    }
    if (!key->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"key\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->Get(key.GetTaggedValue());

    return value;
}

JSTaggedValue ContainersPlainArray::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> iter =
        JSAPIPlainArray::GetIteratorObj(thread, JSHandle<JSAPIPlainArray>::Cast(self), IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersPlainArray::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, ForEach);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPIPlainArray()) {
        if (thisHandle->IsJSProxy() && JSHandle<JSProxy>::Cast(thisHandle)->GetTarget().IsJSAPIPlainArray()) {
            thisHandle = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(thisHandle)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The forEach method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
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
    return JSAPIPlainArray::ForEach(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersPlainArray::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, ToString);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The toString method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSTaggedValue value = JSAPIPlainArray::ToString(thread, JSHandle<JSAPIPlainArray>::Cast(self));
    return value;
}

JSTaggedValue ContainersPlainArray::GetIndexOfKey(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetIndexOfKey);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getIndexOfKey method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (value->IsDouble()) {
        value = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(value->GetDouble()));
    }
    if (!value->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, value.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"key\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t key = value->GetNumber();
    JSTaggedValue result = array->GetIndexOfKey(key);
    return result;
}

JSTaggedValue ContainersPlainArray::GetIndexOfValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetIndexOfValue);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getIndexOfValue method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSTaggedValue jsValue = array->GetIndexOfValue(value.GetTaggedValue());
    return jsValue;
}

JSTaggedValue ContainersPlainArray::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, IsEmpty);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The isEmpty method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    bool ret = array->IsEmpty();
    return JSTaggedValue(ret);
}

JSTaggedValue ContainersPlainArray::GetKeyAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetKeyAt);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getKeyAt method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
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
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t index = value->GetNumber();
    JSTaggedValue result = array->GetKeyAt(index);
    return result;
}

JSTaggedValue ContainersPlainArray::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, Remove);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The remove method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    if (key->IsDouble()) {
        key = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(key->GetDouble()));
    }
    if (!key->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"key\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->Remove(thread, key.GetTaggedValue());
    return value;
}

JSTaggedValue ContainersPlainArray::RemoveAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, RemoveAt);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The removeAt method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->RemoveAt(thread, index.GetTaggedValue());
    return value;
}

JSTaggedValue ContainersPlainArray::RemoveRangeFrom(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, RemoveRangeFrom);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The removeRangeFrom method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> valueIndex(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> valueSize(GetCallArg(argv, 1));
    if (valueIndex->IsDouble()) {
        valueIndex = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(valueIndex->GetDouble()));
    }
    if (valueSize->IsDouble()) {
        valueSize = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(valueSize->GetDouble()));
    }
    if (!valueIndex->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, valueIndex.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!valueSize->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, valueSize.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"size\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    int32_t index = valueIndex->GetNumber();
    int32_t size = valueSize->GetNumber();
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    JSTaggedValue value = array->RemoveRangeFrom(thread, index, size);
    return value;
}

JSTaggedValue ContainersPlainArray::SetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, SetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The setValueAt method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    array->SetValueAt(thread, index.GetTaggedValue(), value.GetTaggedValue());
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersPlainArray::GetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getValueAt method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> idx(GetCallArg(argv, 0));
    if (idx->IsDouble()) {
        idx = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(idx->GetDouble()));
    }
    if (!idx->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, idx.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(self->GetTaggedObject());
    int32_t index = idx->GetNumber();
    JSTaggedValue value = array->GetValueAt(thread, index);
    return value;
}

JSTaggedValue ContainersPlainArray::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, PlainArray, GetSize);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIPlainArray()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget().IsJSAPIPlainArray()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget());
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    uint32_t length = JSHandle<JSAPIPlainArray>::Cast(self)->GetSize();
    return JSTaggedValue(length);
}
} // namespace panda::ecmascript::containers
