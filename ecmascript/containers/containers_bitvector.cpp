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

#include "ecmascript/containers/containers_bitvector.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_api/js_api_bitvector.h"
namespace panda::ecmascript::containers {
JSTaggedValue ContainersBitVector::BitVectorConstructor(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    JSThread* thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BitVector, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = ContainerError::BusinessError(
            thread, ErrorFlag::IS_NULL_ERROR, "The BitVector's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSAPIBitVector> obj =
        JSHandle<JSAPIBitVector>(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> length = GetCallArg(argv, 0);
    if (!length->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, length);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"length\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    auto* newBitSetVector = new std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>>();
    if (!length->IsZero()) {
        int32_t capacity = std::max(0, (length->GetInt() / JSAPIBitVector::BIT_SET_LENGTH) + 1);

        std::bitset<JSAPIBitVector::BIT_SET_LENGTH> initBitSet;
        newBitSetVector->resize(capacity, initBitSet);
    }
    JSHandle<JSNativePointer> pointer = factory->NewSJSNativePointer(newBitSetVector,
                                                                     ContainersBitVector::FreeBitsetVectorPointer,
                                                                     newBitSetVector);
    obj->SetNativePointer(thread, pointer);
    obj->SetLength(std::max(0, length->GetInt()));
    return obj.GetTaggedValue();
}

JSTaggedValue ContainersBitVector::Push(EcmaRuntimeCallInfo* argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, Push);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR, "The push method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    return GetTaggedBoolean(JSAPIBitVector::Push(thread, JSHandle<JSAPIBitVector>::Cast(self), value));
}

JSTaggedValue ContainersBitVector::Pop(EcmaRuntimeCallInfo* argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, Pop);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR, "The pop method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSHandle<JSAPIBitVector>::Cast(self)->Pop(thread, JSHandle<JSAPIBitVector>::Cast(self));
}

JSTaggedValue ContainersBitVector::Has(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, Has);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(thread, BIND_ERROR, "The has method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 2); // 2 means the third arg
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    bool isHas = JSHandle<JSAPIBitVector>::Cast(self)->Has(
        thread, JSHandle<JSAPIBitVector>::Cast(self), value, startIndex, endIndex);

    return GetTaggedBoolean(isHas);
}

JSTaggedValue ContainersBitVector::SetBitsByRange(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, SetBitsByRange);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The setBitsByRange method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 2); // 2 means the third arg
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIBitVector>::Cast(self)->SetBitsByRange(
        thread, JSHandle<JSAPIBitVector>::Cast(self), value, startIndex, endIndex);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersBitVector::GetBitsByRange(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, GetBitsByRange);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The getBitsByRange method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 1);
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSHandle<JSAPIBitVector>::Cast(self)->GetBitsByRange(
        thread, JSHandle<JSAPIBitVector>::Cast(self), startIndex, endIndex);
}

JSTaggedValue ContainersBitVector::Resize(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, Resize);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The resize method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> size = GetCallArg(argv, 0);
    if (!size->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, size);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"size\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIBitVector>::Cast(self)->Resize(
        thread, JSHandle<JSAPIBitVector>::Cast(self), JSTaggedValue::ToUint32(thread, size));
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersBitVector::SetAllBits(EcmaRuntimeCallInfo* argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, SetAllBits);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR, "The setAllBits method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPIBitVector>::Cast(self)->SetAllBits(thread, JSHandle<JSAPIBitVector>::Cast(self), value);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersBitVector::GetBitCountByRange(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, GetBitCountByRange);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The getBitCountByRange method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 2); // 2 means the third arg
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSHandle<JSAPIBitVector>::Cast(self)->GetBitCountByRange(
        thread, JSHandle<JSAPIBitVector>::Cast(self), value, startIndex, endIndex);
}

JSTaggedValue ContainersBitVector::GetIndexOf(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, GetIndexOf);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The getIndexOf method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 2); // 2 means the third arg
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    return JSTaggedValue(
        JSAPIBitVector::GetIndexOf(thread, JSHandle<JSAPIBitVector>::Cast(self), value, startIndex, endIndex));
}

JSTaggedValue ContainersBitVector::GetLastIndexOf(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, GetLastIndexOf);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The getLastIndexOf method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 2); // 2 means the third arg
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return JSTaggedValue(
        JSAPIBitVector::GetLastIndexOf(thread, JSHandle<JSAPIBitVector>::Cast(self), value, startIndex, endIndex));
}

JSTaggedValue ContainersBitVector::FlipBitByIndex(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, FlipBitByIndex);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The flipBitByIndex method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"index\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSAPIBitVector::FlipBitByIndex(
        thread, JSHandle<JSAPIBitVector>::Cast(self), JSTaggedValue::ToUint32(thread, value));
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersBitVector::FlipBitsByRange(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, FlipBitsByRange);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, BIND_ERROR, "The flipBitsByRange method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }
    JSHandle<JSTaggedValue> startIndex = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> endIndex = GetCallArg(argv, 1);
    if (!startIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, startIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"fromIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (!endIndex->IsInteger()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, endIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"toIndex\" must be integer. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIBitVector>::Cast(self)->FlipBitsByRange(
        thread, JSHandle<JSAPIBitVector>::Cast(self), startIndex, endIndex);
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersBitVector::GetSize(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, GetSize);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error =
                ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR, "The getSize method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    return JSTaggedValue(JSHandle<JSAPIBitVector>::Cast(self)->GetSize());
}

JSTaggedValue ContainersBitVector::GetIteratorObj(EcmaRuntimeCallInfo* argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), BitVector, GetIteratorObj);
    JSThread* thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIBitVector()) {
        if (self->IsJSProxy() && JSHandle<JSProxy>::Cast(self)->GetTarget(thread).IsJSAPIBitVector()) {
            self = JSHandle<JSTaggedValue>(thread, JSHandle<JSProxy>::Cast(self)->GetTarget(thread));
        } else {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::BIND_ERROR, "The Symbol.iterator method cannot be bound");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
    }

    JSTaggedValue values = JSAPIBitVector::GetIteratorObj(thread, JSHandle<JSAPIBitVector>::Cast(self));

    return values;
}

void ContainersBitVector::FreeBitsetVectorPointer([[maybe_unused]] void *env, void *pointer,
                                                  [[maybe_unused]] void *data)
{
    if (pointer == nullptr) {
        return;
    }
    auto bitsetVector = reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(pointer);
    delete bitsetVector;
}
} // namespace panda::ecmascript::containers
