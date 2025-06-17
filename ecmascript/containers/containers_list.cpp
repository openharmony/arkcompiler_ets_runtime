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

#include "ecmascript/containers/containers_list.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersList::ListConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Constructor);
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
    JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(obj);
    list->SetIsOrderedList(true);

    JSTaggedValue singleList = TaggedSingleList::Create(thread);
    list->SetSingleList(thread, singleList);

    return list.GetTaggedValue();
}

JSTaggedValue ContainersList::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Add);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The add method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    JSAPIList::Add(thread, jSAPIList, value);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersList::Insert(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Insert);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The insert method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    JSAPIList::Insert(thread, jSAPIList, value, index->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue ContainersList::GetFirst(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetFirst);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getFirst method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    return jSAPIList->GetFirst(thread);
}

JSTaggedValue ContainersList::GetLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetLast);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLast method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    return jSAPIList->GetLast(thread);
}

JSTaggedValue ContainersList::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Has);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The has method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return GetTaggedBoolean(jSAPIList->Has(thread, element.GetTaggedValue()));
}

JSTaggedValue ContainersList::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, IsEmpty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The isEmpty method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jSAPIList = JSHandle<JSAPIList>::Cast(self);
    return GetTaggedBoolean(jSAPIList->IsEmpty(thread));
}

JSTaggedValue ContainersList::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The get method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return jsAPIList->Get(thread, index->GetInt());
}

JSTaggedValue ContainersList::GetIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getIndexOf method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return jsAPIList->GetIndexOf(thread, element.GetTaggedValue());
}

JSTaggedValue ContainersList::GetLastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetLastIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLastIndexOf method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    return jsAPIList->GetLastIndexOf(thread, element.GetTaggedValue());
}

JSTaggedValue ContainersList::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Set);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The set method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 1);
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSTaggedValue oldValue = JSAPIList::Set(thread, jsAPIList, index->GetInt(), element);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return oldValue;
}

JSTaggedValue ContainersList::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check List object
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPIList()) {
        if (thisHandle->IsJSProxy() && JSHandle<JSProxy>::Cast(thisHandle)->GetTarget(thread).IsJSAPIList()) {
            thisHandle = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(thisHandle)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The forEach method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    // get and check callback function
    JSHandle<JSTaggedValue> callbackFnHandle(GetCallArg(argv, 0));
    if (!callbackFnHandle->IsCallable()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, callbackFnHandle.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"callbackfn\" must be callable. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    // If thisArgHandle was supplied, let T be thisArgHandle; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(thisHandle);
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList(thread));
    uint32_t length = list->Length(thread);

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t index = 0;
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    int valueNode = TaggedSingleList::ELEMENTS_START_INDEX;
    while (index < length) {
        valueNode = singleList->GetNextDataIndex(valueNode);
        JSTaggedValue value = singleList->GetElement(thread, valueNode);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(value, JSTaggedValue(index), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersList::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The clear method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    jsAPIList->Clear(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersList::RemoveByIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, RemoveByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The removeByIndex method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> index = GetCallArg(argv, 0);
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSTaggedValue result = JSAPIList::RemoveByIndex(thread, jsAPIList, index->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result;
}

JSTaggedValue ContainersList::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The remove method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> element = GetCallArg(argv, 0);
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    return jsAPIList->Remove(thread, element.GetTaggedValue());
}

JSTaggedValue ContainersList::ReplaceAllElements(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, ReplaceAllElements);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsJSAPIList()) {
        if (thisHandle->IsJSProxy() && JSHandle<JSProxy>::Cast(thisHandle)->GetTarget(thread).IsJSAPIList()) {
            thisHandle = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(thisHandle)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The replaceAllElements method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, callbackFnHandle.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"callbackfn\" must be callable. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    return JSAPIList::ReplaceAllElements(thread, thisHandle, callbackFnHandle, thisArgHandle);
}

JSTaggedValue ContainersList::Equal(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Equal);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The equal method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSHandle<JSTaggedValue> obj = GetCallArg(argv, 0);
    if (!obj->IsJSAPIList()) {
        if (obj->IsJSProxy() && JSHandle<JSProxy>::Cast(obj)->GetTarget(thread).IsJSAPIList()) {
            obj = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(obj)->GetTarget(thread));
        } else {
            return JSTaggedValue::False();
        }
    }
    JSHandle<JSAPIList> handleObj = JSHandle<JSAPIList>::Cast(obj);
    return jsAPIList->Equal(thread, handleObj);
}

JSTaggedValue ContainersList::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Sort);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The sort method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, callbackFnHandle.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"comparator\" must be callable. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSAPIList::Sort(thread, self, callbackFnHandle);
}

JSTaggedValue ContainersList::GetIteratorObj(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetIteratorObj);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSAPIListIterator::CreateListIterator(thread, self);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersList::ConvertToArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, ConvertToArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The convertToArray method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    return JSAPIList::ConvertToArray(thread, jsAPIList);
}

JSTaggedValue ContainersList::GetSubList(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, GetSubList);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getSubList method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> fromIndex = GetCallArg(argv, 0);

    if (fromIndex->IsDouble()) {
        fromIndex = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(fromIndex->GetDouble()));
    }

    if (!fromIndex->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, fromIndex.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be number. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> toIndex = GetCallArg(argv, 1);

    // for case like Math.foor(1.3), it gives double 1.0;
    if (toIndex->IsDouble()) {
        toIndex = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(toIndex->GetDouble()));
    }

    if (!toIndex->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, toIndex.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be number. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    JSTaggedValue newList = JSAPIList::GetSubList(thread, jsAPIList, fromIndex->GetInt(), toIndex->GetInt());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newList;
}

JSTaggedValue ContainersList::Length(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, List, Length);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIList()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIList()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(self);
    return JSTaggedValue(jsAPIList->Length(thread));
}
}  // namespace panda::ecmascript::containers
