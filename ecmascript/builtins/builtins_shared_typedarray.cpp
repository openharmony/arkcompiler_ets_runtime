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

#include "ecmascript/builtins/builtins_shared_typedarray.h"

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/object_fast_operator-inl.h"


namespace panda::ecmascript::builtins {
using TypedArrayHelper = base::TypedArrayHelper;
using TypedArrayKind = base::TypedArrayKind;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;
using BuiltinsSendableArrayBuffer = builtins::BuiltinsSendableArrayBuffer;
using ContainerError = containers::ContainerError;

JSTaggedValue BuiltinsSharedTypedArray::TypedArrayBaseConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, BaseConstructor);
    THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "SharedTypedArray Constructor cannot be called.",
                                JSTaggedValue::Exception());
}

JSTaggedValue BuiltinsSharedTypedArray::Int8ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Int8ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedInt8ArrayString(), DataViewType::INT8);
}

JSTaggedValue BuiltinsSharedTypedArray::Uint8ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Uint8ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedUint8ArrayString(), DataViewType::UINT8);
}

JSTaggedValue BuiltinsSharedTypedArray::Uint8ClampedArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Uint8ClampedArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedUint8ClampedArrayString(), DataViewType::UINT8_CLAMPED);
}

JSTaggedValue BuiltinsSharedTypedArray::Int16ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Int16ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedInt16ArrayString(), DataViewType::INT16);
}

JSTaggedValue BuiltinsSharedTypedArray::Uint16ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Uint16ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedUint16ArrayString(), DataViewType::UINT16);
}

JSTaggedValue BuiltinsSharedTypedArray::Int32ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Int32ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedInt32ArrayString(), DataViewType::INT32);
}

JSTaggedValue BuiltinsSharedTypedArray::Uint32ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Uint32ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedUint32ArrayString(), DataViewType::UINT32);
}

JSTaggedValue BuiltinsSharedTypedArray::Float32ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Float32ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedFloat32ArrayString(), DataViewType::FLOAT32);
}

JSTaggedValue BuiltinsSharedTypedArray::Float64ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Float64ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedFloat64ArrayString(), DataViewType::FLOAT64);
}

JSTaggedValue BuiltinsSharedTypedArray::BigInt64ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, BigInt64ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedBigInt64ArrayString(), DataViewType::BIGINT64);
}

JSTaggedValue BuiltinsSharedTypedArray::BigUint64ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, BigUint64ArrayConstructor);
    JSThread *thread = argv->GetThread();
    return TypedArrayHelper::SharedTypedArrayConstructor(argv,
        thread->GlobalConstants()->GetHandledSharedBigUint64ArrayString(), DataViewType::BIGUINT64);
}

// %TypedArray%.from ( source [ , mapfn [ , thisArg ] ] )
JSTaggedValue BuiltinsSharedTypedArray::From(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, From);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Let C be the this value.
    // 2. If IsConstructor(C) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the this value is not a Constructor.", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    // 3. If mapfn is undefined, let mapping be false.
    // 4. Else,
    //   a. If IsCallable(mapfn) is false, throw a TypeError exception.
    //   b. Let mapping be true.
    bool mapping = false;
    JSHandle<JSTaggedValue> mapfn = GetCallArg(argv, 1);
    if (!mapfn->IsUndefined()) {
        if (!mapfn->IsCallable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "the mapfn is not callable.", JSTaggedValue::Exception());
        }
        mapping = true;
    }

    // 5. Let usingIterator be ? GetMethod(source, @@iterator).
    JSHandle<JSTaggedValue> source = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> usingIterator = JSObject::GetMethod(thread, source, iteratorSymbol);
    // 6. If usingIterator is not undefined, then
    //   a. Let values be ? IterableToList(source, usingIterator).
    //   b. Let len be the number of elements in values.
    //   c. Let targetObj be ? TypedArrayCreate(C, « len »).
    if (!usingIterator->IsUndefined() &&
        !TypedArrayHelper::IsNativeArrayIterator(thread, source, usingIterator)) {
        CVector<JSHandle<JSTaggedValue>> vec;
        JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, source, usingIterator);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> next(thread, JSTaggedValue::True());
        while (!next->IsFalse()) {
            next = JSIterator::IteratorStep(thread, iterator);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!next->IsFalse()) {
                JSHandle<JSTaggedValue> nextValue = JSIterator::IteratorValue(thread, next);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, nextValue.GetTaggedValue());
                vec.push_back(nextValue);
            }
        }
        uint32_t len = vec.size();
        JSTaggedType args[1] = { JSTaggedValue(len).GetRawData() };
        JSHandle<JSObject> targetObj = TypedArrayHelper::TypedArrayCreate(thread, thisHandle, 1, args);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        //   d. Let k be 0.
        //   e. Repeat, while k < len
        //     i. Let Pk be ! ToString(k).
        //     ii. Let kValue be the first element of values and remove that element from values.
        //     iii. If mapping is true, then
        //       1. Let mappedValue be ? Call(mapfn, thisArg, « kValue, k »).
        //     iv. Else, let mappedValue be kValue.
        //     v. Perform ? Set(targetObj, Pk, mappedValue, true).
        //     vi. Set k to k + 1.
        JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
        const uint32_t argsLength = 2;
        uint32_t k = 0;
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        while (k < len) {
            tKey.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kValue = vec[k];
            if (mapping) {
                EcmaRuntimeCallInfo *info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                info->SetCallArg(kValue.GetTaggedValue(), tKey.GetTaggedValue());
                JSTaggedValue callResult = JSFunction::Call(info);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                mapValue.Update(callResult);
            } else {
                mapValue.Update(kValue.GetTaggedValue());
            }
            ObjectFastOperator::FastSetPropertyByIndex(thread, targetObj.GetTaggedValue(), k,
                                                       mapValue.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            k++;
        }
        //   f. Assert: values is now an empty List.
        //   g. Return targetObj.
        return targetObj.GetTaggedValue();
    }

    // 7. NOTE: source is not an Iterable so assume it is already an array-like object.
    // 8. Let arrayLike be ! ToObject(source).
    JSHandle<JSObject> arrayLikeObj = JSTaggedValue::ToObject(thread, source);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> arrayLike(arrayLikeObj);
    // 9. Let len be ? LengthOfArrayLike(arrayLike).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, arrayLike, lengthKey).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedNumber tLen = JSTaggedValue::ToLength(thread, lenResult);
    // 6. ReturnIfAbrupt(relativeTarget).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t len = tLen.GetNumber();

    // 10. Let targetObj be ? TypedArrayCreate(C, « len »).
    JSTaggedType args[1] = { JSTaggedValue(len).GetRawData() };
    JSHandle<JSObject> targetObj = TypedArrayHelper::TypedArrayCreate(thread, thisHandle, 1, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Let k be 0.
    // 12. Repeat, while k < len
    //   a. Let Pk be ! ToString(k).
    //   b. Let kValue be ? Get(arrayLike, Pk).
    //   c. If mapping is true, then
    //     i. Let mappedValue be ? Call(mapfn, thisArg, « kValue, k »).
    //   d. Else, let mappedValue be kValue.
    //   e. Perform ? Set(targetObj, Pk, mappedValue, true).
    //   f. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = 2;
    int64_t k = 0;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> mapValue;
    while (k < len) {
        tKey.Update(JSTaggedValue(k));
        kValue.Update(ObjectFastOperator::FastGetPropertyByValue(thread, arrayLike.GetTaggedValue(),
                                                                 tKey.GetTaggedValue()));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (mapping) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), tKey.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            mapValue = JSHandle<JSTaggedValue>(thread, callResult);
        } else {
            mapValue = kValue;
        }
        ObjectFastOperator::FastSetPropertyByIndex(thread, targetObj.GetTaggedValue(), k, mapValue.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    // 13. Return targetObj.
    return targetObj.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Of(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Of);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let len be the actual number of arguments passed to this function.
    uint32_t len = argv->GetArgsNumber();
    // 2. Let items be the List of arguments passed to this function.
    // 3. Let C be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 4. If IsConstructor(C) is false, throw a TypeError exception.
    if (!thisHandle->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the this value is not a Constructor.", JSTaggedValue::Exception());
    }
    // 5. Let newObj be TypedArrayCreate(C, « len »).
    JSTaggedType args[1] = { JSTaggedValue(len).GetRawData() };
    JSHandle<JSObject> newObj = TypedArrayHelper::TypedArrayCreate(thread, thisHandle, 1, args);
    // 6. ReturnIfAbrupt(newObj).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let kValue be items[k].
    //   b. Let Pk be ! ToString(k).
    //   c. Perform ? Set(newObj, Pk, kValue, true).
    //   d. ReturnIfAbrupt(status).
    //   e. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    uint32_t k = 0;
    while (k < len) {
        tKey.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> kValue = GetCallArg(argv, k);
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newObj), kKey, kValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    // 9. Return newObj.
    return newObj.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Species);
    // 1. Return the this value.
    return GetThis(argv).GetTaggedValue();
}

// prototype
// get %TypedArray%.prototype.buffer
JSTaggedValue BuiltinsSharedTypedArray::GetBuffer(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, GetBuffer);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The buffer property cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSSharedTypedArray> typedArray = JSHandle<JSSharedTypedArray>::Cast(thisHandle);
    JSTaggedValue buffer = JSSharedTypedArray::GetSharedOffHeapBuffer(thread, typedArray);
    // 5. Return buffer.
    return buffer;
}

JSTaggedValue BuiltinsSharedTypedArray::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, GetByteLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The byteLength property cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSTypedArray> typeArrayObj = JSHandle<JSTypedArray>::Cast(thisHandle);
    JSTaggedValue buffer = typeArrayObj->GetViewedArrayBufferOrByteArray();
    // 5. If IsDetachedBuffer(buffer) is true, return 0.
    if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue(0);
    }
    // 6. Let size be the value of O’s [[ByteLength]] internal slot.
    // 7. Return size.
    return JSTaggedValue(typeArrayObj->GetByteLength());
}

JSTaggedValue BuiltinsSharedTypedArray::GetByteOffset(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, GetByteOffset);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The byteOffset property cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSTypedArray> typeArrayObj = JSHandle<JSTypedArray>::Cast(thisHandle);
    JSTaggedValue buffer = typeArrayObj->GetViewedArrayBufferOrByteArray();
    // 5. If IsDetachedBuffer(buffer) is true, return 0.
    if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue(0);
    }
    // 6. Let offset be the value of O’s [[ByteOffset]] internal slot.
    uint32_t offset = typeArrayObj->GetByteOffset();
    // 7. Return offset.
    return JSTaggedValue(offset);
}

JSTaggedValue BuiltinsSharedTypedArray::CopyWithin(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, CopyWithin);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The copyWithin method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray, ModType::WRITE> scope(thread, thisHandle);
    return BuiltinsArray::CopyWithin(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The entries method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    JSHandle<JSObject> self(thisHandle);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 4. Return CreateArrayIterator(O, "key+value").
    JSHandle<JSSharedArrayIterator> iter(factory->NewJSSharedArrayIterator(self, IterationKind::KEY_AND_VALUE));
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Every(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Every);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The every method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    uint32_t len = JSHandle<JSTypedArray>::Cast(thisObjHandle)->GetArrayLength();
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let testResult be ToBoolean(Call(callbackfn, T, «kValue, k, O»)).
    //     iv. ReturnIfAbrupt(testResult).
    //     v. If testResult is false, return false.
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t k = 0;
    while (k < len) {
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, thisObjVal, k).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        bool boolResult = callResult.ToBoolean();
        if (!boolResult) {
            return GetTaggedBoolean(false);
        }
        k++;
    }

    // 9. Return true.
    return GetTaggedBoolean(true);
}

JSTaggedValue BuiltinsSharedTypedArray::Fill(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Fill);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The fill method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray, ModType::WRITE> scope(thread, thisHandle);
    return BuiltinsArray::Fill(argv);
}

// %TypedArray%.prototype.filter ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsSharedTypedArray::Filter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Filter);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The filter method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTypedArray> thisObj(thisHandle);
    // 4. Let len be the value of O’s [[ArrayLength]] internal slot.
    uint32_t len = thisObj->GetArrayLength();
    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    // 10. Let kept be a new empty List.
    JSHandle<TaggedArray> kept(factory->NewTaggedArray(len));

    // 11. Let k be 0.
    // 12. Let captured be 0.
    // 13. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let selected be ToBoolean(Call(callbackfn, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(selected).
    //   f. If selected is true, then
    //     i. Append kValue to the end of kept.
    //     ii. Increase captured by 1.
    //   g. Increase k by 1.
    int32_t captured = 0;
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    for (uint32_t k = 0; k < len; k++) {
        tKey.Update(JSTaggedValue(k));
        kValue.Update(ObjectFastOperator::FastGetPropertyByValue(thread, thisHandle.GetTaggedValue(),
                                                                 tKey.GetTaggedValue()));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle,
            undefined, 3); // 3: «kValue, k, O»
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), tKey.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            kept->Set(thread, captured, kValue);
            captured++;
        }
    }
    // es11 9. Let A be ? TypedArraySpeciesCreate(O, « captured »).
    JSTaggedType args[1] = { JSTaggedValue(captured).GetRawData() };
    JSHandle<JSObject> newArrObj =
        TypedArrayHelper::TypedArraySpeciesCreate<TypedArrayKind::SHARED>(thread, thisObj, 1, args);
    // 15. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 16. Let n be 0.
    // 17. For each element e of kept
    //   a. Let status be Set(A, ToString(n), e, true ).
    //   b. ReturnIfAbrupt(status).
    //   c. Increase n by 1.
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> ntKey(thread, JSTaggedValue::Undefined());
    for (int32_t n = 0; n < captured; n++) {
        valueHandle.Update(kept->Get(n));
        ntKey.Update(JSTaggedValue(n));
        ObjectFastOperator::FastSetPropertyByValue(thread, newArrObj.GetTaggedValue(),
                                                   ntKey.GetTaggedValue(), valueHandle.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 18. Return A.
    return newArrObj.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Find(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Find);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The find method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::Find(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::FindIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, FindIndex);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The findIndex method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::FindIndex(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The forEach method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    uint32_t len = JSHandle<JSTypedArray>::Cast(thisObjHandle)->GetArrayLength();
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let funcResult be Call(callbackfn, T, «kValue, k, O»).
    //     iv. ReturnIfAbrupt(funcResult).
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    uint32_t k = 0;
    while (k < len) {
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, thisObjVal, k).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        k++;
    }

    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedTypedArray::IndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, IndexOf);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The indexOf method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::IndexOf(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::LastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, LastIndexOf);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The lastindexOf method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::LastIndexOf(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::Join(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Join);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The join method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    uint32_t length = JSHandle<JSTypedArray>::Cast(thisHandle)->GetArrayLength();
    JSHandle<JSTaggedValue> sepHandle = GetCallArg(argv, 0);
    int32_t sep = ',';
    uint32_t sepLength = 1;
    JSHandle<EcmaString> sepStringHandle;
    if (!sepHandle->IsUndefined()) {
        if (sepHandle->IsString()) {
            sepStringHandle = JSHandle<EcmaString>::Cast(sepHandle);
        } else {
            sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (EcmaStringAccessor(sepStringHandle).IsUtf8() && EcmaStringAccessor(sepStringHandle).GetLength() == 1) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            sep = EcmaStringAccessor(sepStringHandle).Get(0);
        } else if (EcmaStringAccessor(sepStringHandle).GetLength() == 0) {
            sep = BuiltinsSharedTypedArray::SeparatorFlag::MINUS_TWO;
            sepLength = 0;
        } else {
            sep = BuiltinsSharedTypedArray::SeparatorFlag::MINUS_ONE;
            sepLength = EcmaStringAccessor(sepStringHandle).GetLength();
        }
    }
    if (length == 0) {
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        return globalConst->GetEmptyString();
    }
    uint64_t allocateLength = 0;
    bool isOneByte = (sep != BuiltinsSharedTypedArray::SeparatorFlag::MINUS_ONE) ||
        EcmaStringAccessor(sepStringHandle).IsUtf8();
    CVector<JSHandle<EcmaString>> vec;
    JSMutableHandle<JSTaggedValue> elementHandle(thread, JSTaggedValue::Undefined());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    for (uint32_t k = 0; k < length; k++) {
        JSTaggedValue element = JSTypedArray::GetProperty(thread, thisHandle, k).GetValue().GetTaggedValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!element.IsUndefinedOrNull() && !element.IsHole()) {
            if (!element.IsString()) {
                elementHandle.Update(element);
                JSHandle<EcmaString> strElement = JSTaggedValue::ToString(thread, elementHandle);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                element = strElement.GetTaggedValue();
            }
            auto nextStr = EcmaString::Cast(element.GetTaggedObject());
            JSHandle<EcmaString> nextStrHandle(thread, nextStr);
            vec.push_back(nextStrHandle);
            isOneByte = EcmaStringAccessor(nextStr).IsUtf8() ? isOneByte : false;
            allocateLength += EcmaStringAccessor(nextStr).GetLength();
        } else {
            vec.push_back(JSHandle<EcmaString>(globalConst->GetHandledEmptyString()));
        }
    }
    allocateLength += static_cast<uint64_t>(sepLength) * (length - 1);
    if (allocateLength <= 1) {
        // sep unused, set isOneByte to default(true)
        isOneByte = true;
    }
    if (allocateLength > BaseString::MAX_STRING_LENGTH) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
    }
    auto newString = EcmaStringAccessor::CreateLineString(
        thread->GetEcmaVM(), static_cast<size_t>(allocateLength), isOneByte);
    int current = 0;
    DISALLOW_GARBAGE_COLLECTION;
    for (uint32_t k = 0; k < length; k++) {
        if (k > 0) {
            if (sep >= 0) {
                EcmaStringAccessor(newString).Set(current, static_cast<uint16_t>(sep));
            } else if (sep != BuiltinsSharedTypedArray::SeparatorFlag::MINUS_TWO) {
                EcmaStringAccessor::ReadData(
                    newString, *sepStringHandle, current, allocateLength - static_cast<size_t>(current), sepLength);
            }
            current += static_cast<int>(sepLength);
        }
        JSHandle<EcmaString> nextStr = vec[k];
        int nextLength = static_cast<int>(EcmaStringAccessor(nextStr).GetLength());
        EcmaStringAccessor::ReadData(newString, *nextStr, current,
            allocateLength - static_cast<size_t>(current), nextLength);
        current += nextLength;
    }
    ASSERT_PRINT(
        isOneByte == EcmaStringAccessor::CanBeCompressed(newString), "isOneByte does not match the real value!");
    return JSTaggedValue(newString);
}

JSTaggedValue BuiltinsSharedTypedArray::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The keys method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    JSHandle<JSObject> self(thisHandle);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 4. Return CreateArrayIterator(O, "key").
    JSHandle<JSSharedArrayIterator> iter(factory->NewJSSharedArrayIterator(self, IterationKind::KEY));
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, GetLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[TypedArrayName]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The length property cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 4. Assert: O has [[ViewedArrayBuffer]] and [[ArrayLength]] internal slots.
    // 5. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = JSHandle<JSTypedArray>::Cast(thisHandle)->GetViewedArrayBufferOrByteArray();
    // 6. If IsDetachedBuffer(buffer) is true, return 0.
    if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer)) {
        return JSTaggedValue(0);
    }
    // 7. Let length be the value of O’s [[ArrayLength]] internal slot.
    uint32_t length = JSHandle<JSTypedArray>(thisHandle)->GetArrayLength();
    // 8. Return length.
    return JSTaggedValue(length);
}

// %TypedArray%.prototype.map ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsSharedTypedArray::Map(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Map);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The map method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    JSHandle<JSTypedArray> thisObj(thisHandle);
    // 4. Let len be the value of O’s [[ArrayLength]] internal slot.
    uint32_t len = thisObj->GetArrayLength();
    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackfnHandle = GetCallArg(argv, 0);
    if (!callbackfnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    // es11 5. Let A be ? TypedArraySpeciesCreate(O, « len »).
    JSTaggedType args[1] = { JSTaggedValue(len).GetRawData() };
    JSHandle<JSObject> newArrObj =
        TypedArrayHelper::TypedArraySpeciesCreate<TypedArrayKind::SHARED>(thread, thisObj, 1, args); // 1: one arg.
    // 11. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12. Let k be 0.
    // 13. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let mappedValue be Call(callbackfn, T, «kValue, k, O»).
    //   e. ReturnIfAbrupt(mappedValue).
    //   f. Let status be Set(A, Pk, mappedValue, true ).
    //   g. ReturnIfAbrupt(status).
    //   h. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = 3;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    for (uint32_t k = 0; k < len; k++) {
        key.Update(JSTaggedValue(k));
        kValue.Update(ObjectFastOperator::FastGetPropertyByValue(thread, thisHandle.GetTaggedValue(),
                                                                 key.GetTaggedValue()));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackfnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        mapValue.Update(callResult);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ObjectFastOperator::FastSetPropertyByValue(thread, newArrObj.GetTaggedValue(),
                                                   key.GetTaggedValue(), mapValue.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // 14. Return A.
    return newArrObj.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Reduce(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Reduce);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The reduce method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::Reduce(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::ReduceRight(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, ReduceRight);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The reduceRight method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::ReduceRight(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::Reverse(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Reverse);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The reverse method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray, ModType::WRITE> scope(thread, thisHandle);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be O.[[ArrayLength]]
    int64_t len = JSHandle<JSTypedArray>::Cast(thisHandle)->GetArrayLength();
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Let middle be floor(len/2).
    int64_t middle = std::floor(len / 2);
    // 6. Let lower be 0.
    int64_t lower = 0;

    // 7. Repeat, while lower ≠ middle,
    //     a. Let upper be len - lower - 1.
    //     b. Let upperP be ! ToString(𝔽(upper)).
    //     c. Let lowerP be ! ToString(𝔽(lower)).
    //     d. Let lowerValue be ! Get(O, lowerP).
    //     e. Let upperValue be ! Get(O, upperP).
    //     f. Perform ! Set(O, lowerP, upperValue, true).
    //     g. Perform ! Set(O, upperP, lowerValue, true).
    //     h. Set lower to lower + 1.
    JSMutableHandle<JSTaggedValue> lowerP(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> upperP(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> lowerValueHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> upperValueHandle(thread, JSTaggedValue::Undefined());
    while (lower != middle) {
        int64_t upper = len - lower - 1;
        lowerP.Update(JSTaggedValue(lower));
        upperP.Update(JSTaggedValue(upper));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        lowerValueHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, lowerP);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        upperValueHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, upperP);
        JSArray::FastSetPropertyByValue(thread, thisObjVal, lowerP, upperValueHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSArray::FastSetPropertyByValue(thread, thisObjVal, upperP, lowerValueHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        lower++;
    }

    // 8. Return O .
    return thisObjHandle.GetTaggedValue();
}

// %TypedArray%.prototype.set ( overloaded [ , offset ])
JSTaggedValue BuiltinsSharedTypedArray::Set(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Assert: array is any ECMAScript language value other than an Object with a [[TypedArrayName]] internal slot.
    // If it is such an Object, the definition in 22.2.3.22.2 applies.
    // 2. Let target be the this value.
    JSHandle<JSTaggedValue> target = GetThis(argv);
    // 3. If Type(target) is not Object, throw a TypeError exception.
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    JSHandle<JSTypedArray> targetObj(target);
    // 4. If target does not have a [[TypedArrayName]] internal slot, throw a TypeError exception.
    if (!target->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The set method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray, ModType::WRITE> scope(thread, target);

    // 5. Assert: target has a [[ViewedArrayBuffer]] internal slot.
    // 6. Let targetOffset be ToInteger (offset).
    const JSHandle<JSTaggedValue> srcOffset = GetCallArg(argv, 1);
    uint64_t targetOffset = 0;
    if (srcOffset->IsInt()) {
        if (srcOffset->GetInt() < 0) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The targetOffset of This value is less than 0.",
                                         JSTaggedValue::Exception());
        }
        targetOffset = static_cast<uint64_t>(srcOffset->GetInt());
    } else {
        JSTaggedNumber tTargetOffset = JSTaggedValue::ToInteger(thread, srcOffset);
        // 7. ReturnIfAbrupt(targetOffset).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double rawTargetOffset = tTargetOffset.GetNumber();
        // 8. If targetOffset < 0, throw a RangeError exception.
        if (rawTargetOffset < 0) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The targetOffset of This value is less than 0.",
                                         JSTaggedValue::Exception());
        } else if (rawTargetOffset == base::POSITIVE_INFINITY) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The targetOffset is infinty, which is greater than targetLength.",
                                         JSTaggedValue::Exception());
        } else {
            targetOffset = static_cast<uint64_t>(rawTargetOffset);
        }
    }
    // 9. Let targetBuffer be the value of target’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSTaggedValue> targetBuffer(thread, targetObj->GetViewedArrayBufferOrByteArray());
    // 10. If IsDetachedBuffer(targetBuffer) is true, throw a TypeError exception.
    if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(targetBuffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The targetBuffer of This value is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    // 11. Let targetLength be the value of target’s [[ArrayLength]] internal slot.
    // 12. Let targetName be the String value of target’s [[TypedArrayName]] internal slot.
    // 13. Let targetElementSize be the Number value of the Element Size value specified in Table 49 for targetName.
    // 14. Let targetType be the String value of the Element Type value in Table 49 for targetName.
    // 15. Let targetByteOffset be the value of target’s [[ByteOffset]] internal slot.
    uint32_t targetLength = targetObj->GetArrayLength();
    DataViewType targetType = TypedArrayHelper::GetType(targetObj);
    uint32_t targetElementSize = TypedArrayHelper::GetSizeFromType(targetType);
    uint32_t targetByteOffset = targetObj->GetByteOffset();

    JSHandle<JSTaggedValue> argArray = GetCallArg(argv, 0);
    if (!argArray->IsTypedArray() && !argArray->IsSharedTypedArray()) {
        if (argArray->IsStableJSArray(thread)) {
            uint32_t length = JSHandle<JSArray>::Cast(argArray)->GetArrayLength();
            JSHandle<JSObject> argObj(argArray);
            uint32_t elemLength = ElementAccessor::GetElementsLength(argObj);
            // Load On Demand check
            if (elemLength >= length) {
                return JSStableArray::FastCopyFromArrayToTypedArray<TypedArrayKind::SHARED>(
                    thread, targetObj, targetType, targetOffset, length, argObj);
            }
        }
        // 16. Let src be ToObject(array).
        JSHandle<JSObject> src = JSTaggedValue::ToObject(thread, argArray);
        // 17. ReturnIfAbrupt(src).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // 18. Let srcLength be ToLength(Get(src, "length")).
        JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
        JSHandle<JSTaggedValue> lenResult(thread,
            ObjectFastOperator::FastGetPropertyByValue(thread,
                                                       JSHandle<JSTaggedValue>::Cast(src).GetTaggedValue(),
                                                       lengthKey.GetTaggedValue()));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedNumber tSrcLen = JSTaggedValue::ToLength(thread, lenResult);
        // 19. ReturnIfAbrupt(srcLength).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        uint64_t srcLen = static_cast<uint64_t>(tSrcLen.GetNumber());
        // 20. If srcLength + targetOffset > targetLength, throw a RangeError exception.
        if (srcLen + targetOffset > targetLength) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The sum of srcLength and targetOffset is greater than targetLength.",
                                         JSTaggedValue::Exception());
        }
        // 21. Let targetByteIndex be targetOffset × targetElementSize + targetByteOffset.
        ASSERT((targetOffset * static_cast<uint64_t>(targetElementSize) +
            static_cast<uint64_t>(targetByteOffset)) <= static_cast<uint64_t>(UINT32_MAX));
        uint32_t targetByteIndex = static_cast<uint32_t>(targetOffset * targetElementSize + targetByteOffset);
        // 22. Let k be 0.
        // 23. Let limit be targetByteIndex + targetElementSize × srcLength.
        uint32_t k = 0;
        ASSERT((static_cast<uint64_t>(targetElementSize) * srcLen +
            static_cast<uint64_t>(targetByteIndex)) <= static_cast<uint64_t>(UINT32_MAX));
        uint32_t limit = targetByteIndex + targetElementSize * srcLen;
        // 24. Repeat, while targetByteIndex < limit
        //   a. Let Pk be ToString(k).
        //   b. If target.[[ContentType]] is BigInt, set value to ? ToBigInt(value).
        //   c. Otherwise, set value to ? ToNumber(value).
        //   d. If IsDetachedBuffer(targetBuffer) is true, throw a TypeError exception.
        //   e. Perform SetValueInBuffer(targetBuffer, targetByteIndex, targetType, kNumber).
        //   f. Set k to k + 1.
        //   g. Set targetByteIndex to targetByteIndex + targetElementSize.
        JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Hole());
        JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Hole());
        JSMutableHandle<JSTaggedValue> kNumberHandle(thread, JSTaggedValue::Hole());
        ContentType contentType = JSHandle<JSTypedArray>::Cast(target)->GetContentType();
        while (targetByteIndex < limit) {
            tKey.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            kValue.Update(ObjectFastOperator::FastGetPropertyByValue(
                thread, JSHandle<JSTaggedValue>::Cast(src).GetTaggedValue(), kKey.GetTaggedValue()));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(targetBuffer.GetTaggedValue())) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "The targetBuffer of This value is detached buffer.",
                                            JSTaggedValue::Exception());
            }
            if (contentType == ContentType::BigInt) {
                kNumberHandle.Update(JSTaggedValue::ToBigInt(thread, kValue));
            } else {
                kNumberHandle.Update(JSTaggedValue::ToNumber(thread, kValue));
            }
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(),
                targetByteIndex, targetType, kNumberHandle, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            k++;
            targetByteIndex = targetByteIndex + targetElementSize;
        }
        // 25. Return undefined.
        return JSTaggedValue::Undefined();
    }

    ASSERT(argArray->IsTypedArray() || argArray->IsSharedTypedArray());
    JSHandle<JSTypedArray> typedArray(argArray);
    // 12. Let srcBuffer be the value of typedArray’s [[ViewedArrayBuffer]] internal slot.
    // 13. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
    JSTaggedValue srcBuffer = typedArray->GetViewedArrayBufferOrByteArray();
    JSHandle<JSTaggedValue> srcBufferHandle(thread, srcBuffer);
    if (srcBuffer.IsSendableArrayBuffer() && BuiltinsSendableArrayBuffer::IsDetachedBuffer(srcBuffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    if (!srcBuffer.IsSendableArrayBuffer() && BuiltinsArrayBuffer::IsDetachedBuffer(srcBuffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }

    ContentType objContentType = JSHandle<JSTypedArray>::Cast(target)->GetContentType();
    ContentType argArrayContentType = JSHandle<JSTypedArray>::Cast(argArray)->GetContentType();
    if (argArrayContentType != objContentType) {
        THROW_TYPE_ERROR_AND_RETURN(thread,
                                    "argArrayContentType is not equal objContentType.",
                                    JSTaggedValue::Exception());
    }
    // 18. Let srcName be the String value of typedArray’s [[TypedArrayName]] internal slot.
    // 19. Let srcType be the String value of the Element Type value in Table 49 for srcName .
    // 20. Let srcElementSize be the Number value of the Element Size value specified in Table 49 for srcName.
    // 21. Let srcLength be the value of typedArray’s [[ArrayLength]] internal slot.
    // 22. Let srcByteOffset be the value of typedArray’s [[ByteOffset]] internal slot.
    JSHandle<JSTaggedValue> srcName(thread, typedArray->GetTypedArrayName());
    DataViewType srcType = JSTypedArray::GetTypeFromName(thread, srcName);
    uint32_t srcElementSize = TypedArrayHelper::GetSizeFromType(srcType);
    uint32_t srcLength = typedArray->GetArrayLength();
    uint32_t srcByteOffset = typedArray->GetByteOffset();
    // 23. If srcLength + targetOffset > targetLength, throw a RangeError exception.
    if (srcLength + targetOffset > targetLength) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The sum of srcLength and targetOffset is greater than targetLength.",
                                     JSTaggedValue::Exception());
    }
    // 24. If SameValue(srcBuffer, targetBuffer) is true, then
    //   a. Let srcBuffer be CloneArrayBuffer(targetBuffer, srcByteOffset, %ArrayBuffer%).
    //   b. NOTE: %ArrayBuffer% is used to clone targetBuffer because is it known to not have any observable
    //      side-effects.
    //   c. ReturnIfAbrupt(srcBuffer).
    //   d. Let srcByteIndex be 0.
    // 25. Else, let srcByteIndex be srcByteOffset.
    uint32_t srcByteIndex = 0;
    if (JSTaggedValue::SameValue(srcBufferHandle.GetTaggedValue(), targetBuffer.GetTaggedValue())) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        srcBuffer =
            BuiltinsSendableArrayBuffer::CloneArrayBuffer(thread, targetBuffer, srcByteOffset,
                env->GetSBuiltininArrayBufferFunction());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        srcBufferHandle = JSHandle<JSTaggedValue>(thread, srcBuffer);
        srcByteIndex = 0;
    } else {
        srcByteIndex = srcByteOffset;
    }
    // 26. Let targetByteIndex be targetOffset × targetElementSize + targetByteOffset.
    ASSERT((targetOffset * static_cast<uint64_t>(targetElementSize) +
            static_cast<uint64_t>(targetByteOffset)) <= static_cast<uint64_t>(UINT32_MAX));
    uint32_t targetByteIndex = static_cast<uint32_t>(targetOffset) * targetElementSize + targetByteOffset;
    // 27. Let limit be targetByteIndex + targetElementSize × srcLength.
    ASSERT((static_cast<uint64_t>(targetElementSize) * static_cast<uint64_t>(srcLength) +
            static_cast<uint64_t>(targetByteIndex)) <= static_cast<uint64_t>(UINT32_MAX));
    uint32_t limit = targetByteIndex + targetElementSize * srcLength;
    uint32_t count = (limit - targetByteIndex) > 0 ? (limit - targetByteIndex) : 0;
    // 28. If SameValue(srcType, targetType) is false, then
    //   a. Repeat, while targetByteIndex < limit
    //     i. Let value be GetValueFromBuffer(srcBuffer, srcByteIndex, srcType).
    //     ii. Perform SetValueInBuffer (targetBuffer, targetByteIndex, targetType, value).
    //     iii. Set srcByteIndex to srcByteIndex + srcElementSize.
    //     iv. Set targetByteIndex to targetByteIndex + targetElementSize.
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    if (srcType != targetType) {
        while (targetByteIndex < limit) {
            JSTaggedValue taggedData =
                BuiltinsArrayBuffer::GetValueFromBuffer(thread, srcBufferHandle.GetTaggedValue(),
                                                        srcByteIndex, srcType, true);
            value.Update(taggedData);
            BuiltinsArrayBuffer::SetValueInBuffer(thread, targetBuffer.GetTaggedValue(), targetByteIndex,
                                                  targetType, value, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            srcByteIndex = srcByteIndex + srcElementSize;
            targetByteIndex = targetByteIndex + targetElementSize;
        }
    } else if (count > 0) {
        // 29. Else,
        //   a. NOTE: If srcType and targetType are the same the transfer must be performed in a manner that preserves
        //   the bit-level encoding of the source data.
        //   b. Repeat, while targetByteIndex < limit
        //     i. Let value be GetValueFromBuffer(srcBuffer, srcByteIndex, "Uint8").
        //     ii. Perform SetValueInBuffer (targetBuffer, targetByteIndex, "Uint8", value).
        //     iii. Set srcByteIndex to srcByteIndex + 1.
        //     iv. Set targetByteIndex to targetByteIndex + 1.
        void *srcBuf = nullptr;
        if (argArray->IsSharedTypedArray()) {
            srcBuf = BuiltinsSendableArrayBuffer::GetDataPointFromBuffer(
                srcBufferHandle.GetTaggedValue(), srcByteIndex);
        } else {
            srcBuf = BuiltinsArrayBuffer::GetDataPointFromBuffer(
                srcBufferHandle.GetTaggedValue(), srcByteIndex);
        }
        void *targetBuf = BuiltinsSendableArrayBuffer::GetDataPointFromBuffer(
            targetBuffer.GetTaggedValue(), targetByteIndex);
        if (memcpy_s(targetBuf, srcLength * srcElementSize, srcBuf, srcLength * srcElementSize) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    // 30. Return undefined.
    return JSTaggedValue::Undefined();
}

// %TypedArray%.prototype.slice ( start, end )
JSTaggedValue BuiltinsSharedTypedArray::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Slice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The slice method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    JSHandle<JSTypedArray> thisObj(thisHandle);
    // 4. Let len be the value of O’s [[ArrayLength]] internal slot.
    uint32_t len = thisObj->GetArrayLength();

    // 5. Let relativeStart be ToInteger(start).
    JSTaggedNumber tRelativeStart = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // 6. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double relativeStart = tRelativeStart.GetNumber();
    // 7. If relativeStart < 0, let k be max((len + relativeStart),0); else let k be min(relativeStart, len).

    uint32_t k = relativeStart < 0 ?
                    std::max((static_cast<double>(len) + relativeStart), 0.0) :
                    std::min(relativeStart, static_cast<double>(len));
    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    double relativeEnd = len;
    JSHandle<JSTaggedValue> end = GetCallArg(argv, 1);
    if (!end->IsUndefined()) {
        JSTaggedNumber tRelativeEnd = JSTaggedValue::ToInteger(thread, end);
        // 9. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        relativeEnd = tRelativeEnd.GetNumber();
    }

    // 10. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).

    uint32_t final = relativeEnd < 0 ?
                        std::max((static_cast<double>(len) + relativeEnd), 0.0) :
                        std::min(relativeEnd, static_cast<double>(len));
    // 11. Let count be max(final – k, 0).
    uint32_t count = final > k ? (final - k) : 0;
    // es11 9. Let A be ? TypedArraySpeciesCreate(O, « count »).
    JSTaggedType args[1] = { JSTaggedValue(count).GetRawData() };
    JSHandle<JSObject> newArrObj =
        TypedArrayHelper::TypedArraySpeciesCreate<TypedArrayKind::SHARED>(thread, thisObj, 1, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 17. Let srcName be the String value of O’s [[TypedArrayName]] internal slot.
    // 18. Let srcType be the String value of the Element Type value in Table 49 for srcName.
    JSHandle<JSTaggedValue> srcName(thread, thisObj->GetTypedArrayName());
    DataViewType srcType = JSTypedArray::GetTypeFromName(thread, srcName);
    // 19. Let targetName be the String value of A’s [[TypedArrayName]] internal slot.
    // 20. Let targetType be the String value of the Element Type value in Table 49 for targetName.
    JSHandle<JSTaggedValue> targetName(thread, JSTypedArray::Cast(*newArrObj)->GetTypedArrayName());
    DataViewType targetType = JSTypedArray::GetTypeFromName(thread, targetName);
    // 21. If SameValue(srcType, targetType) is false, then
    //   a. Let n be 0.
    //   b. Repeat, while k < final
    //     i. Let Pk be ToString(k).
    //     ii. Let kValue be Get(O, Pk).
    //     iii. ReturnIfAbrupt(kValue).
    //     iv. Let status be Set(A, ToString(n), kValue, true ).
    //     v. ReturnIfAbrupt(status).
    //     vi. Increase k by 1.
    //     vii. Increase n by 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> ntKey(thread, JSTaggedValue::Undefined());
    if (srcType != targetType) {
        uint32_t n = 0;
        while (k < final) {
            tKey.Update(JSTaggedValue(k));
            kValue.Update(ObjectFastOperator::FastGetPropertyByValue(thread, thisHandle.GetTaggedValue(),
                                                                     tKey.GetTaggedValue()));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            ntKey.Update(JSTaggedValue(n));
            ObjectFastOperator::FastSetPropertyByValue(thread, newArrObj.GetTaggedValue(),
                                                       ntKey.GetTaggedValue(), kValue.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            n++;
            k++;
        }
    } else if (count > 0) {
        // 22. Else if count > 0,
        //   a. Let srcBuffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
        //   b. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
        JSTaggedValue srcBuffer = thisObj->GetViewedArrayBufferOrByteArray();
        if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(srcBuffer)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The ArrayBuffer of this value is detached buffer.",
                                        JSTaggedValue::Exception());
        }
        //   c. Let targetBuffer be the value of A’s [[ViewedArrayBuffer]] internal slot.
        JSTaggedValue targetBuffer = JSTypedArray::Cast(*newArrObj)->GetViewedArrayBufferOrByteArray();
        //   d. Let elementSize be the Number value of the Element Size value specified in Table 49 for srcType.
        uint32_t elementSize = TypedArrayHelper::GetSizeFromType(srcType);
        //   e. NOTE: If srcType and targetType are the same the transfer must be performed in a manner that
        //   preserves the bit-level encoding of the source data.
        //   f. Let srcByteOffset be the value of O’s[[ByteOffset]] internal slot.
        uint32_t srcByteOffset = thisObj->GetByteOffset();
        //   h. Let srcByteIndex be (k × elementSize) + srcByteOffset.
        uint32_t srcByteIndex = k * elementSize + srcByteOffset;
        //   g. Let targetByteIndex be A.[[ByteOffset]].
        uint32_t targetByteIndex = JSTypedArray::Cast(*newArrObj)->GetByteOffset();
        //   i. Repeat, while targetByteIndex < count × elementSize
        //     i. Let value be GetValueFromBuffer(srcBuffer, srcByteIndex, "Uint8").
        //     ii. Perform SetValueInBuffer (targetBuffer, targetByteIndex, "Uint8", value).
        //     iii. Increase srcByteIndex by 1.
        //     iv. Increase targetByteIndex by 1.
        uint8_t *srcBuf = reinterpret_cast<uint8_t *>(
            BuiltinsSendableArrayBuffer::GetDataPointFromBuffer(srcBuffer, srcByteIndex));
        uint8_t *targetBuf = reinterpret_cast<uint8_t *>(
            BuiltinsSendableArrayBuffer::GetDataPointFromBuffer(targetBuffer, targetByteIndex));
        if (srcBuffer != targetBuffer && memmove_s(
            targetBuf, elementSize * count, srcBuf, elementSize * count) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
        while (srcBuffer == targetBuffer && count > 0) {
            count--;
            if (memcpy_s(targetBuf, elementSize, srcBuf, elementSize) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            srcBuf += elementSize;
            targetBuf += elementSize;
        }
    }
    // 23. Return A.
    return newArrObj.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Some(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Some);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The some method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::Some(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Sort);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let obj be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The sort method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray, ModType::WRITE> scope(thread, thisHandle);

    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> buffer;
    buffer = JSHandle<JSTaggedValue>(thread, TypedArrayHelper::ValidateTypedArray(thread, thisHandle));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t len = JSHandle<JSTypedArray>::Cast(thisObjHandle)->GetArrayLength();

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }
    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> key1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> key2(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 1; i < len; i++) {
        uint32_t beginIndex = 0;
        uint32_t endIndex = i;
        key.Update(JSTaggedValue(i));
        presentValue.Update(ObjectFastOperator::FastGetPropertyByValue(thread, thisObjHandle.GetTaggedValue(),
                                                                       key.GetTaggedValue()));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        while (beginIndex < endIndex) {
            uint32_t middleIndex = beginIndex + (endIndex - beginIndex) / 2;  // 2 : half
            key1.Update(JSTaggedValue(middleIndex));
            middleValue.Update(ObjectFastOperator::FastGetPropertyByValue(thread, thisObjHandle.GetTaggedValue(),
                                                                          key1.GetTaggedValue()));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            int32_t compareResult = TypedArrayHelper::SortCompare(thread, callbackFnHandle, buffer,
                                                                  middleValue, presentValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            compareResult > 0 ? (endIndex = middleIndex) : (beginIndex = middleIndex + 1);
        }

        if (endIndex < i) {
            for (uint32_t j = i; j > endIndex; j--) {
                key2.Update(JSTaggedValue(j - 1));
                previousValue.Update(
                    ObjectFastOperator::FastGetPropertyByValue(thread, thisObjHandle.GetTaggedValue(),
                                                               key2.GetTaggedValue()));
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                ObjectFastOperator::FastSetPropertyByIndex(thread, thisObjHandle.GetTaggedValue(), j,
                                                           previousValue.GetTaggedValue());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            ObjectFastOperator::FastSetPropertyByIndex(thread, thisObjHandle.GetTaggedValue(), endIndex,
                                                       presentValue.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }
    return thisObjHandle.GetTaggedValue();
}

// %TypedArray%.prototype.subarray( [ begin [ , end ] ] )
JSTaggedValue BuiltinsSharedTypedArray::Subarray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Subarray);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This value is not an object.", JSTaggedValue::Exception());
    }
    JSHandle<JSTypedArray> thisObj(thisHandle);
    // 3. If O does not have a [[TypedArrayName]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The subarray method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    // 4. Assert: O has a [[ViewedArrayBuffer]] internal slot.
    // 6. Let srcLength be the value of O’s [[ArrayLength]] internal slot.
    uint32_t srcLength = thisObj->GetArrayLength();
    // 7. Let relativeBegin be ToInteger(begin).
    JSTaggedNumber tRelativeBegin = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // 8. ReturnIfAbrupt(relativeBegin).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double relativeBegin = tRelativeBegin.GetNumber();

    uint32_t beginIndex = 0;
    // 9. If relativeBegin < 0, let beginIndex be max((srcLength + relativeBegin), 0); else let beginIndex be
    // min(relativeBegin, srcLength).
    if (relativeBegin < 0) {
        double tempBeginIndex = relativeBegin + static_cast<double>(srcLength);
        beginIndex = tempBeginIndex > 0 ? static_cast<uint32_t>(tempBeginIndex) : 0;
    } else {
        beginIndex = relativeBegin < static_cast<double>(srcLength) ?
                        static_cast<uint32_t>(relativeBegin) : srcLength;
    }

    // 10. If end is undefined, let relativeEnd be srcLength; else, let relativeEnd be ToInteger(end).
    double relativeEnd = srcLength;
    JSHandle<JSTaggedValue> end = GetCallArg(argv, 1);
    if (!end->IsUndefined()) {
        JSTaggedNumber tRelativeEnd = JSTaggedValue::ToInteger(thread, end);
        // 11. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        relativeEnd = tRelativeEnd.GetNumber();
    }
    // 12. If relativeEnd < 0, let endIndex be max((srcLength + relativeEnd), 0); else let endIndex be
    // min(relativeEnd, srcLength).
    uint32_t endIndex;
    if (relativeEnd < 0) {
        double tempEndIndex = relativeEnd + static_cast<double>(srcLength);
        endIndex = tempEndIndex > 0 ? static_cast<uint32_t>(tempEndIndex) : 0;
    } else {
        endIndex = relativeEnd < static_cast<double>(srcLength) ?
                        static_cast<uint32_t>(relativeEnd) : srcLength;
    }
    // 13. Let newLength be max(endIndex – beginIndex, 0).
    uint32_t newLength = endIndex > beginIndex ? (endIndex - beginIndex) : 0;
    // 14. Let constructorName be the String value of O’s [[TypedArrayName]] internal slot.
    // 15. Let elementSize be the Number value of the Element Size value specified in Table 49 for constructorName.
    // 16. Let srcByteOffset be the value of O’s [[ByteOffset]] internal slot.
    // 17. Let beginByteOffset be srcByteOffset + beginIndex × elementSize.
    JSHandle<JSTaggedValue> constructorName(thread, thisObj->GetTypedArrayName());
    DataViewType elementType = JSTypedArray::GetTypeFromName(thread, constructorName);
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(elementType);
    uint32_t srcByteOffset = thisObj->GetByteOffset();
    ASSERT((static_cast<uint64_t>(srcByteOffset) + static_cast<uint64_t>(beginIndex) *
            static_cast<uint64_t>(elementSize)) <= static_cast<uint64_t>(UINT32_MAX));
    uint32_t beginByteOffset = srcByteOffset + beginIndex * elementSize;
    JSTaggedValue buffer = JSSharedTypedArray::GetSharedOffHeapBuffer(thread, JSHandle<JSSharedTypedArray>(thisObj));
    // 21. Let argumentsList be «buffer, beginByteOffset, newLength».
    // 5. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    // 22. Return Construct(constructor, argumentsList).
    const uint32_t argsLength = 3;
    JSTaggedType args[argsLength] = {
        buffer.GetRawData(),
        JSTaggedValue(beginByteOffset).GetRawData(),
        JSTaggedValue(newLength).GetRawData()
    };
    JSHandle<JSObject> newArr =
        TypedArrayHelper::TypedArraySpeciesCreate<TypedArrayKind::SHARED>(thread, thisObj, argsLength, args);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return newArr.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, ToLocaleString);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The toLocaleString method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::ToLocaleString(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, ToString);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The toString method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::ToString(argv);
}

JSTaggedValue BuiltinsSharedTypedArray::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Let valid be ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The values method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3. ReturnIfAbrupt(valid).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    JSHandle<JSObject> self(thisHandle);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 4. Return CreateArrayIterator(O, "value").
    JSHandle<JSSharedArrayIterator> iter(factory->NewJSSharedArrayIterator(self, IterationKind::VALUE));
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::ToStringTag(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, ToStringTag);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, return undefined.
    if (!thisHandle->IsECMAObject()) {
        return JSTaggedValue::Undefined();
    }
    // 3. If O does not have a [[TypedArrayName]] internal slot, return undefined.
    if (!thisHandle->IsSharedTypedArray()) {
        return JSTaggedValue::Undefined();
    }
    // 4. Let name be the value of O’s [[TypedArrayName]] internal slot.
    JSTaggedValue name = JSHandle<JSTypedArray>::Cast(thisHandle)->GetTypedArrayName();
    // 5. Assert: name is a String value.
    ASSERT(name.IsString());
    // 6. Return name.
    return name;
}

JSTaggedValue BuiltinsSharedTypedArray::At(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, At);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. Perform ? ValidateTypedArray(O).
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The at method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);

    // 3. Let len be O.[[ArrayLength]].
    uint32_t len = JSHandle<JSTypedArray>::Cast(thisObjHandle)->GetArrayLength();
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. Let relativeIndex be ? ToIntegerOrInfinity(index).
    JSTaggedNumber indexVal = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // ReturnIfAbrupt(indexVal).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t relativeIndex = indexVal.GetNumber();
    int64_t k = 0;
    // 5. If relativeIndex ≥ 0, then Let k be relativeIndex.
    // 6. Else, Let k be len + relativeIndex.
    k = relativeIndex >= 0 ? relativeIndex : static_cast<int64_t>(len) + relativeIndex;
    // 7. If k < 0 or k ≥ len, return undefined.
    if (k < 0 || k >= len) {
        return JSTaggedValue::Undefined();
    }
    // 8. Return ! Get(O, ! ToString(𝔽(k))).
    JSHandle<JSTaggedValue> kValue = JSTypedArray::GetProperty(thread, thisHandle, k).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return kValue.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedTypedArray::Includes(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedTypedArray, Includes);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSThread *thread = argv->GetThread();
    if (!thisHandle->IsSharedTypedArray()) {
        auto error = ContainerError::BindError(thread, "The includes method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedTypedArray> scope(thread, thisHandle);
    return BuiltinsArray::Includes(argv);
}
}  // namespace panda::ecmascript::builtins
