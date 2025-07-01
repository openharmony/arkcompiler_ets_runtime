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

#include "ecmascript/containers/containers_lightweightmap.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersLightWeightMap::LightWeightMapConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error =
            ContainerError::BusinessError(thread, ErrorFlag::IS_NULL_ERROR,
                                          "The LightWeightMap's constructor cannot be directly invoked");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSAPILightWeightMap> lwMap = JSHandle<JSAPILightWeightMap>::Cast(obj);
    JSHandle<TaggedArray> hashArray = factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> valueArray = factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH);
    lwMap->SetHashes(thread, hashArray.GetTaggedValue());
    lwMap->SetKeys(thread, keyArray.GetTaggedValue());
    lwMap->SetValues(thread, valueArray.GetTaggedValue());

    return lwMap.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::Length(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Length);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getLength method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSTaggedValue(JSHandle<JSAPILightWeightMap>::Cast(self)->GetLength());
}

JSTaggedValue ContainersLightWeightMap::HasAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, HasAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The hasAll method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> lightWeightMap(GetCallArg(argv, 0));
    if (!lightWeightMap->IsJSAPILightWeightMap()) {
        if (lightWeightMap->IsJSProxy() &&
            JSHandle<JSProxy>::Cast(lightWeightMap)->GetTarget(thread).IsJSAPILightWeightMap()) {
            lightWeightMap =
                JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(lightWeightMap)->GetTarget(thread));
        } else {
            JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, lightWeightMap.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            CString errorMsg =
                "The type of \"map\" must be LightWeightMap. Received value is: " + ConvertToString(thread, *result);
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSAPILightWeightMap::HasAll(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                       JSHandle<JSAPILightWeightMap>::Cast(lightWeightMap));
}

JSTaggedValue ContainersLightWeightMap::HasKey(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, HasKey);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The hasKey method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    return JSAPILightWeightMap::HasKey(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
}

JSTaggedValue ContainersLightWeightMap::HasValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, HasValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The hasValue method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    return JSAPILightWeightMap::HasValue(thread, JSHandle<JSAPILightWeightMap>::Cast(self), value);
}

JSTaggedValue ContainersLightWeightMap::IncreaseCapacityTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, IncreaseCapacityTo);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The increaseCapacityTo method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));

    // for case like Math.foor(1.3), it gives double 1.0;
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }

    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg = "The type of \"minimumCapacity\" must be small integer. Received value is: " +
            ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSAPILightWeightMap::IncreaseCapacityTo(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                            index->GetInt());

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLightWeightMap::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Entries);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightMapIterator::CreateLightWeightMapIterator(thread, self, IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::Get(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Get);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The get method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    return JSAPILightWeightMap::Get(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
}

JSTaggedValue ContainersLightWeightMap::GetIndexOfKey(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetIndexOfKey);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getIndexOfKey method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    int32_t index = JSAPILightWeightMap::GetIndexOfKey(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
    return JSTaggedValue(index);
}

JSTaggedValue ContainersLightWeightMap::GetIndexOfValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetIndexOfValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getIndexOfValue method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));

    int32_t index = JSAPILightWeightMap::GetIndexOfValue(thread, JSHandle<JSAPILightWeightMap>::Cast(self), value);
    return JSTaggedValue(index);
}

JSTaggedValue ContainersLightWeightMap::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, IsEmpty);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The isEmpty method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    return JSHandle<JSAPILightWeightMap>::Cast(self)->IsEmpty();
}

JSTaggedValue ContainersLightWeightMap::GetKeyAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetKeyAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getKeyAt method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));

    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::GetKeyAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                         index->GetInt());
}

JSTaggedValue ContainersLightWeightMap::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Keys);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightMapIterator::CreateLightWeightMapIterator(thread, self, IterationKind::KEY);
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::SetAll(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, SetAll);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The setAll method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> lightWeightMap(GetCallArg(argv, 0));

    if (!lightWeightMap->IsJSAPILightWeightMap()) {
        if (lightWeightMap->IsJSProxy() &&
            JSHandle<JSProxy>::Cast(lightWeightMap)->GetTarget(thread).IsJSAPILightWeightMap()) {
            lightWeightMap =
                JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(lightWeightMap)->GetTarget(thread));
        } else {
            JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, lightWeightMap.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            CString errorMsg =
                "The type of \"map\" must be LightWeightMap. Received value is: " + ConvertToString(thread, *result);
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSAPILightWeightMap::SetAll(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                JSHandle<JSAPILightWeightMap>::Cast(lightWeightMap));
    return JSTaggedValue::True();
}

JSTaggedValue ContainersLightWeightMap::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Set);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The set method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 1));
    JSHandle<JSAPILightWeightMap> lightWeightMap = JSHandle<JSAPILightWeightMap>::Cast(self);
    JSAPILightWeightMap::Set(thread, lightWeightMap, key, value);

    return lightWeightMap.GetTaggedValue();
}

JSTaggedValue ContainersLightWeightMap::Remove(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Remove);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The remove method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> key(GetCallArg(argv, 0));

    return JSAPILightWeightMap::Remove(thread, JSHandle<JSAPILightWeightMap>::Cast(self), key);
}

JSTaggedValue ContainersLightWeightMap::RemoveAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, RemoveAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
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
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::RemoveAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                         index->GetInt());
}

JSTaggedValue ContainersLightWeightMap::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Clear);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The clear method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSAPILightWeightMap::Clear(thread, JSHandle<JSAPILightWeightMap>::Cast(self));
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLightWeightMap::SetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, SetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
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
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    return JSAPILightWeightMap::SetValueAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                           index->GetInt(), value);
}

JSTaggedValue ContainersLightWeightMap::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // get and check lightweightmap object
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The forEach method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    // get and check callback function
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, func.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"callbackfn\" must be callable. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);
    JSHandle<JSAPILightWeightMap> tmap = JSHandle<JSAPILightWeightMap>::Cast(self);
    JSMutableHandle<TaggedArray> keys(thread, tmap->GetKeys(thread));
    JSMutableHandle<TaggedArray> values(thread, tmap->GetValues(thread));

    uint32_t index = 0;
    uint32_t length = tmap->GetSize();
    const uint32_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    while (index < length) {
        // ignore the hash value is required to determine the true index
        // Let funcResult be Call(callbackfn, T, «e, e, S»).
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisArg, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(values->Get(thread, index), keys->Get(thread, index), self.GetTaggedValue());
        JSTaggedValue ret = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);

        // check entries should be update, size will be update in tmap set or remove.
        if (tmap->GetSize() != length) {
            keys.Update(tmap->GetKeys(thread));
            values.Update(tmap->GetValues(thread));
            length = tmap->GetSize();
        }
        index++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersLightWeightMap::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, ToString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The toString method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSAPILightWeightMap::ToString(thread, JSHandle<JSAPILightWeightMap>::Cast(self));
}

JSTaggedValue ContainersLightWeightMap::GetValueAt(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, GetValueAt);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPILightWeightMap()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPILightWeightMap()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                                "The getValueAt method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> index(GetCallArg(argv, 0));
    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    if (!index->IsInt()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, index);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be small integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    if (index->IsDouble()) {
        index = JSHandle<JSTaggedValue>(thread, JSTaggedValue::TryCastDoubleToInt32(index->GetDouble()));
    }
    return JSAPILightWeightMap::GetValueAt(thread, JSHandle<JSAPILightWeightMap>::Cast(self),
                                           index->GetInt());
}

JSTaggedValue ContainersLightWeightMap::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, LightWeightMap, Keys);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter =
        JSAPILightWeightMapIterator::CreateLightWeightMapIterator(thread, self, IterationKind::VALUE);
    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript::containers
