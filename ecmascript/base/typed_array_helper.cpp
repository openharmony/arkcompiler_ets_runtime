/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/typed_array_helper.h"

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"

namespace panda::ecmascript::base {
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;
using BuiltinsSendableArrayBuffer = builtins::BuiltinsSendableArrayBuffer;
using ContainerError = containers::ContainerError;

// es11 22.2.4 The TypedArray Constructors
JSTaggedValue TypedArrayHelper::TypedArrayConstructor(EcmaRuntimeCallInfo *argv,
                                                      const JSHandle<JSTaggedValue> &constructorName,
                                                      const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = BuiltinsBase::GetNewTarget(argv);
    // 2. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The NewTarget is undefined.", JSTaggedValue::Exception());
    }
    // 3. Let constructorName be the String value of the Constructor Name value specified in Table 61 for this
    // TypedArray constructor.
    // 4. Let O be ? AllocateTypedArray(constructorName, NewTarget, "%TypedArray.prototype%").
    JSHandle<JSTaggedValue> firstArg = BuiltinsBase::GetCallArg(argv, 0);
    if (!firstArg->IsECMAObject()) {
        // es11 22.2.4.1 TypedArray ( )
        uint32_t elementLength = 0;
        // es11 22.2.4.2 TypedArray ( length )
        if (!firstArg->IsUndefined()) {
            JSTaggedNumber index = JSTaggedValue::ToIndex(thread, firstArg);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            elementLength = static_cast<uint32_t>(index.GetNumber());
        }
        JSHandle<JSObject> obj = TypedArrayHelper::AllocateTypedArray(thread, constructorName, newTarget,
                                                                      elementLength, arrayType);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return obj.GetTaggedValue();
    }

    JSHandle<JSObject> obj = TypedArrayHelper::AllocateTypedArray(thread, constructorName, newTarget, arrayType);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (firstArg->IsTypedArray()) {
        return TypedArrayHelper::CreateFromTypedArray(argv, obj, arrayType);
    }
    if (firstArg->IsArrayBuffer() || firstArg->IsSharedArrayBuffer()) {
        return TypedArrayHelper::CreateFromArrayBuffer(argv, obj, arrayType);
    }
    if (firstArg->IsStableJSArray(thread)) {
        return TypedArrayHelper::FastCopyElementFromArray(argv, obj, arrayType);
    }
    if (firstArg->IsSendableArrayBuffer()) {
        auto error = ContainerError::BusinessError(thread, containers::ErrorFlag::TYPE_ERROR,
            "Parameter error. Not support Created from SendableArrayBuffer yet.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    return TypedArrayHelper::CreateFromOrdinaryObject(argv, obj, arrayType);
}

// es11 22.2.4 The TypedArray Constructors
JSTaggedValue TypedArrayHelper::SharedTypedArrayConstructor(EcmaRuntimeCallInfo *argv,
                                                            const JSHandle<JSTaggedValue> &constructorName,
                                                            const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = BuiltinsBase::GetNewTarget(argv);
    // 2. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, containers::ErrorFlag::IS_NULL_ERROR,
            "The ArkTS TypedArray's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 3. Let constructorName be the String value of the Constructor Name value specified in Table 61 for this
    // TypedArray constructor.
    // 4. Let O be ? AllocateTypedArray(constructorName, NewTarget, "%TypedArray.prototype%").
    JSHandle<JSTaggedValue> firstArg = BuiltinsBase::GetCallArg(argv, 0);
    if (!firstArg->IsECMAObject()) {
        // es11 22.2.4.1 TypedArray ( )
        uint32_t elementLength = 0;
        // es11 22.2.4.2 TypedArray ( length )
        if (!firstArg->IsUndefined()) {
            JSTaggedNumber index = JSTaggedValue::ToIndex(thread, firstArg);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            elementLength = static_cast<uint32_t>(index.GetNumber());
        }
        JSHandle<JSObject> obj = TypedArrayHelper::AllocateSharedTypedArray(thread, constructorName, newTarget,
                                                                            elementLength, arrayType);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return obj.GetTaggedValue();
    }

    JSHandle<JSObject> obj = TypedArrayHelper::AllocateSharedTypedArray(thread, constructorName, newTarget, arrayType);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (firstArg->IsTypedArray() || firstArg->IsSharedTypedArray()) {
        return TypedArrayHelper::CreateSharedFromTypedArray(argv, obj, arrayType);
    }
    if (firstArg->IsSendableArrayBuffer()) {
        return TypedArrayHelper::CreateFromSendableArrayBuffer(argv, obj, arrayType);
    }
    if (firstArg->IsArrayBuffer() || firstArg->IsSharedArrayBuffer()) {
        auto error = ContainerError::BusinessError(thread, containers::ErrorFlag::TYPE_ERROR,
                                                   "Parameter error. Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (firstArg->IsStableJSArray(thread)) {
        return TypedArrayHelper::FastCopyElementFromArray<TypedArrayKind::SHARED>(argv, obj, arrayType);
    }
    return TypedArrayHelper::CreateFromOrdinaryObject<TypedArrayKind::SHARED>(argv, obj, arrayType);
}

// Fastpath for create a typedarray. Do not need to create an EcmaRuntimeCallInfo.
JSHandle<JSObject> TypedArrayHelper::FastCreateTypedArray(JSThread *thread,
                                                          const JSHandle<JSTaggedValue> &constructorName,
                                                          uint32_t length,
                                                          const DataViewType arrayType)
{
    JSHandle<JSObject> exception(thread, JSTaggedValue::Exception());
    if (length > JSTypedArray::MAX_TYPED_ARRAY_INDEX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must less than 2^32 - 1", exception);
    }

    // Create TypedArray
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> typedArrayFunc = TypedArrayHelper::GetConstructorFromType(thread, arrayType);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(typedArrayFunc,
        JSHandle<JSTaggedValue>::Cast(typedArrayFunc));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    JSTypedArray::Cast(*obj)->SetTypedArrayName(thread, constructorName);

    // Create ArrayBuffer
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    uint32_t arrayLength = static_cast<uint32_t>(length);
    uint64_t byteLength = static_cast<uint64_t>(elementSize) * length;
    JSHandle<JSTaggedValue> data;

    JSHandle<JSTaggedValue> constructor = thread->GetEcmaVM()->GetGlobalEnv()->GetArrayBufferFunction();
    data = JSHandle<JSTaggedValue>(thread,
        BuiltinsArrayBuffer::AllocateArrayBuffer(thread, constructor, byteLength));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exception);

    // Assign ArrayBuffer to TypedArray
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    if (arrayType == DataViewType::BIGINT64 ||
        arrayType == DataViewType::BIGUINT64) {
        jsTypedArray->SetContentType(ContentType::BigInt);
    } else {
        jsTypedArray->SetContentType(ContentType::Number);
    }
    // Set O.[[ViewedArrayBuffer]] to data.
    // Set O.[[ByteLength]] to byteLength.
    // Set O.[[ByteOffset]] to 0.
    // Set O.[[ArrayLength]] to length.
    jsTypedArray->SetViewedArrayBufferOrByteArray(thread, data);
    jsTypedArray->SetByteLength(byteLength);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(arrayLength);
    // Return O.
    return obj;
}

template<>
JSHandle<JSObject> TypedArrayHelper::AllocateTypedArrayBuffer<TypedArrayKind::NON_SHARED>(
    JSThread *thread, const JSHandle<JSObject> &obj, uint64_t length, const DataViewType arrayType)
{
    // 1. Assert: O is an Object that has a [[ViewedArrayBuffer]] internal slot.
    // 2. Assert: O.[[ViewedArrayBuffer]] is undefined.
    // 3. Assert: ! IsNonNegativeInteger(length) is true.
    JSHandle<JSObject> exception(thread, JSTaggedValue::Exception());
    if (length > JSTypedArray::MAX_TYPED_ARRAY_INDEX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must less than 2^32 - 1", exception);
    }
    // 4. Let constructorName be the String value of O.[[TypedArrayName]].
    //     we use type to get size
    // 5. Let elementSize be the Element Size value specified in Table 61 for constructorName.
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    // 6. Let byteLength be elementSize × length.
    uint32_t arrayLength = static_cast<uint32_t>(length);
    uint64_t byteLength = static_cast<uint64_t>(elementSize) * length;
    // 7. Let data be ? AllocateArrayBuffer(%ArrayBuffer%, byteLength).
    JSHandle<JSTaggedValue> data;
    if (byteLength > JSTypedArray::MAX_ONHEAP_LENGTH) {
        JSHandle<JSTaggedValue> constructor = thread->GetEcmaVM()->GetGlobalEnv()->GetArrayBufferFunction();
        data = JSHandle<JSTaggedValue>(thread,
            BuiltinsArrayBuffer::AllocateArrayBuffer(thread, constructor, byteLength));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        ASSERT_PRINT(!JSHandle<TaggedObject>(obj)->GetClass()->IsOnHeapFromBitField(), "must be not on heap");
    } else {
        data = JSHandle<JSTaggedValue>(thread,
            thread->GetEcmaVM()->GetFactory()->NewByteArray(arrayLength, elementSize).GetTaggedValue());
        JSHandle<JSHClass> onHeapHclass = TypedArrayHelper::GetOnHeapHclassFromType(
            thread, JSHandle<JSTypedArray>(obj), arrayType);
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, JSHandle<JSHClass>(thread, obj->GetJSHClass()), onHeapHclass);
#endif
        TaggedObject::Cast(*obj)->SynchronizedTransitionClass(thread, *onHeapHclass); // notOnHeap->onHeap
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exception);
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    if (arrayType == DataViewType::BIGINT64 ||
        arrayType == DataViewType::BIGUINT64) {
        jsTypedArray->SetContentType(ContentType::BigInt);
    } else {
        jsTypedArray->SetContentType(ContentType::Number);
    }
    // 8. Set O.[[ViewedArrayBuffer]] to data.
    // 9. Set O.[[ByteLength]] to byteLength.
    // 10. Set O.[[ByteOffset]] to 0.
    // 11. Set O.[[ArrayLength]] to length.
    jsTypedArray->SetViewedArrayBufferOrByteArray(thread, data);
    jsTypedArray->SetByteLength(byteLength);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(arrayLength);
    // 12. Return O.
    return obj;
}

template<>
JSHandle<JSObject> TypedArrayHelper::AllocateTypedArrayBuffer<TypedArrayKind::SHARED>(
    JSThread *thread, const JSHandle<JSObject> &obj, uint64_t length, const DataViewType arrayType)
{
    // 1. Assert: O is an Object that has a [[ViewedArrayBuffer]] internal slot.
    // 2. Assert: O.[[ViewedArrayBuffer]] is undefined.
    // 3. Assert: ! IsNonNegativeInteger(length) is true.
    JSHandle<JSObject> exception(thread, JSTaggedValue::Exception());
    if (length > JSTypedArray::MAX_TYPED_ARRAY_INDEX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must less than 2^32 - 1", exception);
    }
    // 4. Let constructorName be the String value of O.[[TypedArrayName]].
    //     we use type to get size
    // 5. Let elementSize be the Element Size value specified in Table 61 for constructorName.
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    // 6. Let byteLength be elementSize × length.
    uint32_t arrayLength = static_cast<uint32_t>(length);
    uint64_t byteLength = static_cast<uint64_t>(elementSize) * length;
    // 7. Let data be ? AllocateArrayBuffer(%ArrayBuffer%, byteLength).
    JSHandle<JSTaggedValue> data;
    if (byteLength > JSTypedArray::MAX_ONHEAP_LENGTH) {
        JSHandle<JSTaggedValue> constructor = thread->GetEcmaVM()->GetGlobalEnv()->GetSBuiltininArrayBufferFunction();
        data = JSHandle<JSTaggedValue>(thread,
            BuiltinsSendableArrayBuffer::AllocateSendableArrayBuffer(thread, constructor, byteLength));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        ASSERT_PRINT(!JSHandle<TaggedObject>(obj)->GetClass()->IsOnHeapFromBitField(), "must be not on heap");
    } else {
        data = JSHandle<JSTaggedValue>(thread,
            thread->GetEcmaVM()->GetFactory()->NewByteArray(arrayLength, elementSize, nullptr,
            MemSpaceType::SHARED_OLD_SPACE).GetTaggedValue());
        JSHandle<JSHClass> onHeapHclass = TypedArrayHelper::GetSharedOnHeapHclassFromType(
            thread, JSHandle<JSTypedArray>(obj), arrayType);
#if ECMASCRIPT_ENABLE_IC
        JSHClass::NotifyHclassChanged(thread, JSHandle<JSHClass>(thread, obj->GetJSHClass()), onHeapHclass);
#endif
        TaggedObject::Cast(*obj)->SynchronizedTransitionClass(thread, *onHeapHclass); // notOnHeap->onHeap
    }
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, exception);
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    if (arrayType == DataViewType::BIGINT64 ||
        arrayType == DataViewType::BIGUINT64) {
        jsTypedArray->SetContentType(ContentType::BigInt);
    } else {
        jsTypedArray->SetContentType(ContentType::Number);
    }
    // 8. Set O.[[ViewedArrayBuffer]] to data.
    // 9. Set O.[[ByteLength]] to byteLength.
    // 10. Set O.[[ByteOffset]] to 0.
    // 11. Set O.[[ArrayLength]] to length.
    jsTypedArray->SetViewedArrayBufferOrByteArray(thread, data);
    jsTypedArray->SetByteLength(byteLength);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(arrayLength);
    // 12. Return O.
    return obj;
}

template JSTaggedValue TypedArrayHelper::FastCopyElementFromArray<TypedArrayKind::NON_SHARED>(
    EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj, const DataViewType arrayType);
template JSTaggedValue TypedArrayHelper::FastCopyElementFromArray<TypedArrayKind::SHARED>(
    EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj, const DataViewType arrayType);

template<TypedArrayKind typedArrayKind>
JSTaggedValue TypedArrayHelper::FastCopyElementFromArray(
    EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj, const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> argArray = BuiltinsBase::GetCallArg(argv, 0);
    uint32_t len = JSHandle<JSArray>::Cast(argArray)->GetArrayLength();
    JSHandle<JSObject> argObj(argArray);
    // load on demand check
    if (ElementAccessor::GetElementsLength(argObj) < len) {
        TypedArrayHelper::CreateFromOrdinaryObject<typedArrayKind>(argv, obj, arrayType);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return obj.GetTaggedValue();
    }

    TypedArrayHelper::AllocateTypedArrayBuffer<typedArrayKind>(thread, obj, len, arrayType);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTypedArray> targetObj = JSHandle<JSTypedArray>::Cast(obj);

    JSStableArray::FastCopyFromArrayToTypedArray<typedArrayKind>(
        thread, targetObj, arrayType, 0, len, argObj);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSHandle<JSObject>::Cast(targetObj).GetTaggedValue();
}

template JSTaggedValue TypedArrayHelper::CreateFromOrdinaryObject<TypedArrayKind::SHARED>(
    EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj, const DataViewType arrayType);
template JSTaggedValue TypedArrayHelper::CreateFromOrdinaryObject<TypedArrayKind::NON_SHARED>(
    EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj, const DataViewType arrayType);

// es11 22.2.4.4 TypedArray ( object )
template <TypedArrayKind typedArrayKind>
JSTaggedValue TypedArrayHelper::CreateFromOrdinaryObject(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                                         const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectArg = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSObject> object(objectArg);
    // 5. Let usingIterator be ? GetMethod(object, @@iterator).
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> usingIterator =
        JSObject::GetMethod(thread, JSHandle<JSTaggedValue>::Cast(object), iteratorSymbol);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 6. If usingIterator is not undefined, then
    if (!usingIterator->IsUndefined()) {
        CVector<JSHandle<JSTaggedValue>> vec;
        // a. Let values be ? IterableToList(object, usingIterator).
        // b. Let len be the number of elements in values.
        // c. Perform ? AllocateTypedArrayBuffer(O, len).
        JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, objectArg, usingIterator);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> next(thread, JSTaggedValue::True());
        while (!next->IsFalse()) {
            next = JSIterator::IteratorStep(thread, iterator);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!next->IsFalse()) {
                JSHandle<JSTaggedValue> nextValue = JSIterator::IteratorValue(thread, next);
                vec.push_back(nextValue);
            }
        }
        uint32_t len = static_cast<uint32_t>(vec.size());
        TypedArrayHelper::AllocateTypedArrayBuffer<typedArrayKind>(thread, obj, len, arrayType);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // d. Let k be 0.
        // e. Repeat, while k < len
        //   i. Let Pk be ! ToString(k).
        //   ii. Let kValue be the first element of values and remove that element from values.
        //   iii. Perform ? Set(O, Pk, kValue, true).
        //   iv. Set k to k + 1.
        JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
        uint32_t k = 0;
        while (k < len) {
            tKey.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSHandle<JSTaggedValue> kValue = vec[k];
            JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), kKey, kValue, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            k++;
        }
        // f. Assert: values is now an empty List.
        // g. Return O.
        return obj.GetTaggedValue();
    }

    // 7. NOTE: object is not an Iterable so assume it is already an array-like object.
    // 8. Let arrayLike be object.
    // 9. Let len be ? LengthOfArrayLike(arrayLike).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSTaggedNumber lenTemp =
        JSTaggedValue::ToLength(thread, JSTaggedValue::GetProperty(thread, objectArg, lengthKey).GetValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint64_t rawLen = lenTemp.GetNumber();
    // 10. Perform ? AllocateTypedArrayBuffer(O, len).
    TypedArrayHelper::AllocateTypedArrayBuffer<typedArrayKind>(thread, obj, rawLen, arrayType);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Let k be 0.
    // 12. Repeat, while k < len
    //   a. Let Pk be ! ToString(k).
    //   b. Let kValue be ? Get(arrayLike, Pk).
    //   c. Perform ? Set(O, Pk, kValue, true).
    //   d. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> tKey(thread, JSTaggedValue::Undefined());
    uint32_t len = static_cast<uint32_t>(rawLen);
    uint32_t k = 0;
    while (k < len) {
        tKey.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kKey(JSTaggedValue::ToString(thread, tKey));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> kValue = JSTaggedValue::GetProperty(thread, objectArg, kKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), kKey, kValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    // 13. Return O.
    return obj.GetTaggedValue();
}

// es11 22.2.4.3 TypedArray ( typedArray )
JSTaggedValue TypedArrayHelper::CreateFromTypedArray(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                                     const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 5. Let srcArray be typedArray.
    JSHandle<JSTaggedValue> srcArray = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTypedArray> srcObj(srcArray);
    // 6. Let srcData be srcArray.[[ViewedArrayBuffer]].
    JSHandle<JSTaggedValue> srcData(thread, JSTypedArray::GetOffHeapBuffer(thread, srcObj));
    // 7. If IsDetachedBuffer(srcData) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(srcData.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The srcData is detached buffer.", JSTaggedValue::Exception());
    }
    // 8. Let elementType be the Element Type value in Table 61 for constructorName.
    //     which is arrayType passed in.
    // 9. Let elementLength be srcArray.[[ArrayLength]].
    // 10. Let srcName be the String value of srcArray.[[TypedArrayName]].
    // 11. Let srcType be the Element Type value in Table 61 for srcName.
    // 12. Let srcElementSize be the Element Size value specified in Table 61 for srcName.
    uint32_t elementLength = srcObj->GetArrayLength();
    JSHandle<JSTaggedValue> srcName(thread, srcObj->GetTypedArrayName());
    DataViewType srcType = JSTypedArray::GetTypeFromName(thread, srcName);
    uint32_t srcElementSize = TypedArrayHelper::GetSizeFromType(srcType);
    // 13. Let srcByteOffset be srcArray.[[ByteOffset]].
    // 14. Let elementSize be the Element Size value specified in Table 61 for constructorName.
    // 15. Let byteLength be elementSize × elementLength.
    uint32_t srcByteOffset = srcObj->GetByteOffset();
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    // If elementLength is a large number, the multiplication of elementSize and elementLength may exceed
    //     the maximum value of uint32, resulting in data overflow. Therefore, the type of byteLength is uint64_t.
    uint64_t byteLength = elementSize * static_cast<uint64_t>(elementLength);
    // 16. If IsSharedArrayBuffer(srcData) is false, then
    //   a. Let bufferConstructor be ? SpeciesConstructor(srcData, %ArrayBuffer%).

    JSMutableHandle<JSTaggedValue> data(thread, JSTaggedValue::Undefined());
    // 18. If elementType is the same as srcType, then
    //   a. Let data be ? CloneArrayBuffer(srcData, srcByteOffset, byteLength, bufferConstructor).
    if (arrayType == srcType) {
        JSTaggedValue tmp =
            BuiltinsArrayBuffer::CloneArrayBuffer(thread, srcData, srcByteOffset, globalConst->GetHandledUndefined());
        data.Update(tmp);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        // 19. Else,
        //   a. Let data be ? AllocateArrayBuffer(bufferConstructor, byteLength).
        JSHandle<JSTaggedValue> bufferConstructor =
            JSObject::SpeciesConstructor(thread, JSHandle<JSObject>(srcData), env->GetArrayBufferFunction());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSTaggedValue tmp = BuiltinsArrayBuffer::AllocateArrayBuffer(thread, bufferConstructor, byteLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        data.Update(tmp);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        //   b. If IsDetachedBuffer(srcData) is true, throw a TypeError exception.
        if (BuiltinsArrayBuffer::IsDetachedBuffer(srcData.GetTaggedValue())) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The srcData is detached buffer.", JSTaggedValue::Exception());
        }
        ContentType objContentType = JSHandle<JSTypedArray>::Cast(obj)->GetContentType();
        ContentType srcArrayContentType = JSHandle<JSTypedArray>::Cast(srcArray)->GetContentType();
        if (srcArrayContentType != objContentType) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "srcArrayContentType is not equal objContentType.",
                                        JSTaggedValue::Exception());
        }
        //   d. Let srcByteIndex be srcByteOffset.
        //   e. Let targetByteIndex be 0.
        uint32_t srcByteIndex = srcByteOffset;
        uint32_t targetByteIndex = 0;
        //   f. Let count be elementLength.
        //   g. Repeat, while count > 0
        JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
        for (uint32_t count = elementLength; count > 0; count--) {
            // i. Let value be GetValueFromBuffer(srcData, srcByteIndex, srcType, true, Unordered).
            JSTaggedValue taggedData =
                BuiltinsArrayBuffer::GetValueFromBuffer(thread, srcData.GetTaggedValue(), srcByteIndex, srcType, true);
            value.Update(taggedData);
            // ii. Perform SetValueInBuffer(data, targetByteIndex, elementType, value, true, Unordered).
            BuiltinsArrayBuffer::SetValueInBuffer(thread, data.GetTaggedValue(),
                                                  targetByteIndex, arrayType, value, true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. Set srcByteIndex to srcByteIndex + srcElementSize.
            // iv. Set targetByteIndex to targetByteIndex + elementSize.
            // v. Set count to count - 1.
            srcByteIndex = srcByteIndex + srcElementSize;
            targetByteIndex = targetByteIndex + elementSize;
        }
    }
    // 19. Set O’s [[ViewedArrayBuffer]] internal slot to data.
    // 20. Set O’s [[ByteLength]] internal slot to byteLength.
    // 21. Set O’s [[ByteOffset]] internal slot to 0.
    // 22. Set O’s [[ArrayLength]] internal slot to elementLength.
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    jsTypedArray->SetViewedArrayBufferOrByteArray(thread, data);
    jsTypedArray->SetByteLength(byteLength);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(elementLength);
    ASSERT_PRINT(!JSHandle<TaggedObject>(obj)->GetClass()->IsOnHeapFromBitField(), "must be not on heap");
    // 23. Return O.
    return obj.GetTaggedValue();
}

JSTaggedValue GetTypedArrayBuffer(EcmaRuntimeCallInfo *argv, JSHandle<JSTypedArray> &srcObj,
                                  bool isIsSharedTypedArray)
{
    JSThread *thread = argv->GetThread();
    JSTaggedValue buffer;
    if (isIsSharedTypedArray) {
        buffer = JSSharedTypedArray::GetSharedOffHeapBuffer(thread, JSHandle<JSSharedTypedArray>(srcObj));
    } else {
        buffer = JSTypedArray::GetOffHeapBuffer(thread, srcObj);
    }
    if (buffer.IsSendableArrayBuffer() && BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    if (!buffer.IsSendableArrayBuffer() && BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    return buffer;
}

JSTaggedValue CheckBufferAndType(JSTaggedValue buffer, JSThread *thread, const JSHandle<JSObject> &obj,
                                 JSHandle<JSTaggedValue> srcArray)
{
    //   b. If IsDetachedBuffer(srcData) is true, throw a TypeError exception.
    if (buffer.IsSendableArrayBuffer() && BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    if (!buffer.IsSendableArrayBuffer() && BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }

    ContentType objContentType = JSHandle<JSTypedArray>::Cast(obj)->GetContentType();
    ContentType srcArrayContentType = JSHandle<JSTypedArray>::Cast(srcArray)->GetContentType();
    if (srcArrayContentType != objContentType) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "srcArrayContentType is not equal objContentType.",
                                    JSTaggedValue::Exception());
    }
    return JSTaggedValue::True();
}

void SetTypedArrayProperties(JSThread *thread, const JSHandle<JSObject> &obj, JSMutableHandle<JSTaggedValue> &data,
                             const uint64_t byteLength, const uint32_t elementLength)
{
    // 19. Set O’s [[ViewedArrayBuffer]] internal slot to data.
    // 20. Set O’s [[ByteLength]] internal slot to byteLength.
    // 21. Set O’s [[ByteOffset]] internal slot to 0.
    // 22. Set O’s [[ArrayLength]] internal slot to elementLength.
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    jsTypedArray->SetViewedArrayBufferOrByteArray(thread, data);
    jsTypedArray->SetByteLength(byteLength);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(elementLength);
    ASSERT_PRINT(!JSHandle<TaggedObject>(obj)->GetClass()->IsOnHeapFromBitField(), "must be not on heap");
}

void CloneAndUpdateArrayBuffer(JSThread *thread, JSHandle<JSTaggedValue> srcData, uint32_t srcByteOffset,
                               JSMutableHandle<JSTaggedValue> &data)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSTaggedValue tmp = BuiltinsSendableArrayBuffer::CloneArrayBuffer(thread, srcData, srcByteOffset,
                                                                      globalConst->GetHandledUndefined());
    data.Update(tmp);
}

JSTaggedValue AllocatAndUpdateArrayBuffer(JSThread *thread,  uint64_t byteLength,
                                          JSMutableHandle<JSTaggedValue> &data)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 19. Else,
    //   a. Let data be ? AllocateArrayBuffer(bufferConstructor, byteLength).
    JSHandle<JSTaggedValue> bufferConstructor = env->GetSBuiltininArrayBufferFunction();
    JSTaggedValue tmp =
        BuiltinsSendableArrayBuffer::AllocateSendableArrayBuffer(thread, bufferConstructor, byteLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    data.Update(tmp);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::True();
}

JSTaggedValue TypedArrayHelper::CreateSharedFromTypedArray(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                                           const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 5. Let srcArray be typedArray.
    JSHandle<JSTaggedValue> srcArray = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTypedArray> srcObj(srcArray);
    JSTaggedValue buffer = GetTypedArrayBuffer(argv, srcObj, srcArray->IsSharedTypedArray());
    if (buffer == JSTaggedValue::Exception()) {
        return buffer;
    }
    // 6. Let srcData be srcArray.[[ViewedArrayBuffer]].
    JSHandle<JSTaggedValue> srcData(thread, buffer);
    // 8. Let elementType be the Element Type value in Table 61 for constructorName.
    //     which is arrayType passed in.
    // 9. Let elementLength be srcArray.[[ArrayLength]].
    // 10. Let srcName be the String value of srcArray.[[TypedArrayName]].
    // 11. Let srcType be the Element Type value in Table 61 for srcName.
    // 12. Let srcElementSize be the Element Size value specified in Table 61 for srcName.
    uint32_t elementLength = srcObj->GetArrayLength();
    JSHandle<JSTaggedValue> srcName(thread, srcObj->GetTypedArrayName());
    DataViewType srcType = JSTypedArray::GetTypeFromName(thread, srcName);
    uint32_t srcElementSize = TypedArrayHelper::GetSizeFromType(srcType);
    // 13. Let srcByteOffset be srcArray.[[ByteOffset]].
    // 14. Let elementSize be the Element Size value specified in Table 61 for constructorName.
    // 15. Let byteLength be elementSize × elementLength.
    uint32_t srcByteOffset = srcObj->GetByteOffset();
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    // If elementLength is a large number, the multiplication of elementSize and elementLength may exceed
    //     the maximum value of uint32, resulting in data overflow. Therefore, the type of byteLength is uint64_t.
    uint64_t byteLength = elementSize * static_cast<uint64_t>(elementLength);
    // 16. If IsSharedArrayBuffer(srcData) is false, then
    //   a. Let bufferConstructor be ? SpeciesConstructor(srcData, %ArrayBuffer%).
    JSMutableHandle<JSTaggedValue> data(thread, JSTaggedValue::Undefined());
    // 18. If elementType is the same as srcType, then
    //   a. Let data be ? CloneArrayBuffer(srcData, srcByteOffset, byteLength, bufferConstructor).
    if (arrayType == srcType && srcArray->IsSharedTypedArray()) {
        CloneAndUpdateArrayBuffer(thread, srcData, srcByteOffset, data);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        JSTaggedValue allocateResult = AllocatAndUpdateArrayBuffer(thread, byteLength, data);
        if (allocateResult == JSTaggedValue::Exception()) {
            return allocateResult;
        }
        JSTaggedValue checkResult = CheckBufferAndType(srcData.GetTaggedValue(), thread, obj, srcArray);
        if (checkResult == JSTaggedValue::Exception()) {
            return checkResult;
        }
        //   d. Let srcByteIndex be srcByteOffset.
        //   e. Let targetByteIndex be 0.
        uint32_t srcByteIndex = srcByteOffset;
        uint32_t targetByteIndex = 0;
        //   f. Let count be elementLength.
        //   g. Repeat, while count > 0
        JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
        for (uint32_t count = elementLength; count > 0; count--) {
            // i. Let value be GetValueFromBuffer(srcData, srcByteIndex, srcType, true, Unordered).
            JSTaggedValue taggedData = BuiltinsArrayBuffer::GetValueFromBuffer(thread, srcData.GetTaggedValue(),
                                                                               srcByteIndex, srcType, true);
            value.Update(taggedData);
            // ii. Perform SetValueInBuffer(data, targetByteIndex, elementType, value, true, Unordered).
            BuiltinsArrayBuffer::SetValueInBuffer(thread, data.GetTaggedValue(), targetByteIndex, arrayType, value,
                                                  true);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // iii. Set srcByteIndex to srcByteIndex + srcElementSize.
            // iv. Set targetByteIndex to targetByteIndex + elementSize.
            // v. Set count to count - 1.
            srcByteIndex = srcByteIndex + srcElementSize;
            targetByteIndex = targetByteIndex + elementSize;
        }
    }
    SetTypedArrayProperties(thread, obj, data, byteLength, elementLength);
    // 23. Return O.
    return obj.GetTaggedValue();
}

void SetArrayBufferProperties(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj, const uint64_t newByteLength,
                              const uint32_t offset, const uint32_t arrayLength)
{
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> buffer = BuiltinsBase::GetCallArg(argv, 0);
    // 13. Set O.[[ViewedArrayBuffer]] to buffer.
    // 14. Set O.[[ByteLength]] to newByteLength.
    // 15. Set O.[[ByteOffset]] to offset.
    // 16. Set O.[[ArrayLength]] to newByteLength / elementSize.
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    jsTypedArray->SetViewedArrayBufferOrByteArray(thread, buffer);
    jsTypedArray->SetByteLength(newByteLength);
    jsTypedArray->SetByteOffset(offset);
    jsTypedArray->SetArrayLength(arrayLength);
}

// es11 22.2.4.5 TypedArray ( buffer [ , byteOffset [ , length ] ] )
JSTaggedValue TypedArrayHelper::CreateFromArrayBuffer(EcmaRuntimeCallInfo *argv, const JSHandle<JSObject> &obj,
                                                      const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 5. Let elementSize be the Element Size value specified in Table 61 for constructorName.
    // 6. Let offset be ? ToIndex(byteOffset).
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    JSHandle<JSTaggedValue> byteOffset = BuiltinsBase::GetCallArg(argv, 1);
    JSTaggedNumber index = JSTaggedValue::ToIndex(thread, byteOffset);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    auto offset = static_cast<uint32_t>(index.GetNumber());
    // 7. If offset modulo elementSize ≠ 0, throw a RangeError exception.
    if (offset % elementSize != 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The offset cannot be an integral multiple of elementSize.",
                                     JSTaggedValue::Exception());
    }
    // 8. If length is not undefined, then
    //   a. Let newLength be ? ToIndex(length).
    JSHandle<JSTaggedValue> length = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    uint64_t newLength = 0;
    if (!length->IsUndefined()) {
        index = JSTaggedValue::ToIndex(thread, length);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        newLength = static_cast<uint64_t>(index.GetNumber());
    }
    // 9. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    JSHandle<JSTaggedValue> buffer = BuiltinsBase::GetCallArg(argv, 0);
    if (BuiltinsArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The srcData is detached buffer.", JSTaggedValue::Exception());
    }
    // 10. Let bufferByteLength be buffer.[[ArrayBufferByteLength]].
    uint32_t bufferByteLength = JSHandle<JSArrayBuffer>(buffer)->GetArrayBufferByteLength();
    // 11. If length is undefined, then
    //   a. If bufferByteLength modulo elementSize ≠ 0, throw a RangeError exception.
    //   b. Let newByteLength be bufferByteLength - offset.
    //   c. If newByteLength < 0, throw a RangeError exception.
    uint64_t newByteLength = 0;
    if (length->IsUndefined()) {
        if (bufferByteLength % elementSize != 0) {
            std::string ctorName = EcmaStringAccessor(
                JSTaggedValue::ToString(thread, GetConstructorNameFromType(thread, arrayType))).ToStdString();
            std::string message = "The byte length of " + ctorName + " should be a multiple of " +
                std::to_string(elementSize);
            THROW_RANGE_ERROR_AND_RETURN(thread, message.c_str(), JSTaggedValue::Exception());
        }
        if (bufferByteLength < offset) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The newByteLength is less than 0.", JSTaggedValue::Exception());
        }
        newByteLength = bufferByteLength - offset;
    } else {
        // 12. Else,
        //   a. Let newByteLength be newLength × elementSize.
        //   b. If offset + newByteLength > bufferByteLength, throw a RangeError exception.
        newByteLength = newLength * elementSize;
        if (offset + newByteLength > bufferByteLength) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The newByteLength is out of range.", JSTaggedValue::Exception());
        }
    }
    SetArrayBufferProperties(argv, obj, newByteLength, offset, newByteLength / elementSize);
    // 17. Return O.
    return obj.GetTaggedValue();
}

// es11 22.2.4.5 TypedArray ( buffer [ , byteOffset [ , length ] ] )
JSTaggedValue TypedArrayHelper::CreateFromSendableArrayBuffer(EcmaRuntimeCallInfo *argv,
                                                              const JSHandle<JSObject> &obj,
                                                              const DataViewType arrayType)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 5. Let elementSize be the Element Size value specified in Table 61 for constructorName.
    // 6. Let offset be ? ToIndex(byteOffset).
    uint32_t elementSize = TypedArrayHelper::GetSizeFromType(arrayType);
    JSHandle<JSTaggedValue> byteOffset = BuiltinsBase::GetCallArg(argv, 1);
    JSTaggedNumber index = JSTaggedValue::ToIndex(thread, byteOffset);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    auto offset = static_cast<uint32_t>(index.GetNumber());
    // 7. If offset modulo elementSize ≠ 0, throw a RangeError exception.
    if (offset % elementSize != 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The offset cannot be an integral multiple of elementSize.",
                                     JSTaggedValue::Exception());
    }
    // 8. If length is not undefined, then
    //   a. Let newLength be ? ToIndex(length).
    JSHandle<JSTaggedValue> length = BuiltinsBase::GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    uint64_t newLength = 0;
    if (!length->IsUndefined()) {
        index = JSTaggedValue::ToIndex(thread, length);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        newLength = static_cast<uint64_t>(index.GetNumber());
    }
    // 9. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    JSHandle<JSTaggedValue> buffer = BuiltinsBase::GetCallArg(argv, 0);
    if (BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The srcData is detached buffer.", JSTaggedValue::Exception());
    }
    // 10. Let bufferByteLength be buffer.[[ArrayBufferByteLength]].
    uint32_t bufferByteLength = JSHandle<JSSendableArrayBuffer>(buffer)->GetArrayBufferByteLength();
    // 11. If length is undefined, then
    //   a. If bufferByteLength modulo elementSize ≠ 0, throw a RangeError exception.
    //   b. Let newByteLength be bufferByteLength - offset.
    //   c. If newByteLength < 0, throw a RangeError exception.
    uint64_t newByteLength = 0;
    if (length->IsUndefined()) {
        if (bufferByteLength % elementSize != 0) {
            std::string ctorName = EcmaStringAccessor(
                JSTaggedValue::ToString(thread, GetConstructorNameFromType(thread, arrayType))).ToStdString();
            std::string message = "The byte length of " + ctorName + " should be a multiple of " +
                std::to_string(elementSize);
            THROW_RANGE_ERROR_AND_RETURN(thread, message.c_str(), JSTaggedValue::Exception());
        }
        if (bufferByteLength < offset) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The newByteLength is less than 0.", JSTaggedValue::Exception());
        }
        newByteLength = bufferByteLength - offset;
    } else {
        // 12. Else,
        //   a. Let newByteLength be newLength × elementSize.
        //   b. If offset + newByteLength > bufferByteLength, throw a RangeError exception.
        newByteLength = newLength * elementSize;
        if (offset + newByteLength > bufferByteLength) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "The newByteLength is out of range.", JSTaggedValue::Exception());
        }
    }
    SetArrayBufferProperties(argv, obj, newByteLength, offset, newByteLength / elementSize);
    // 17. Return O.
    return obj.GetTaggedValue();
}

// es11 22.2.4.2.1 Runtime Semantics: AllocateTypedArray ( constructorName, newTarget, defaultProto )
JSHandle<JSObject> TypedArrayHelper::AllocateTypedArray(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &constructorName,
                                                        const JSHandle<JSTaggedValue> &newTarget,
                                                        const DataViewType arrayType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let proto be ? GetPrototypeFromConstructor(newTarget, defaultProto).
    // 2. Let obj be ! IntegerIndexedObjectCreate(proto).
    JSHandle<JSFunction> typedArrayFunc = TypedArrayHelper::GetConstructorFromType(thread, arrayType);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(typedArrayFunc, newTarget);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 3. Assert: obj.[[ViewedArrayBuffer]] is undefined.
    // 4. Set obj.[[TypedArrayName]] to constructorName.

    // 5. If constructorName is "BigInt64Array" or "BigUint64Array", set obj.[[ContentType]] to BigInt.
    // 6. Otherwise, set obj.[[ContentType]] to Number.
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    if (arrayType == DataViewType::BIGINT64 ||
        arrayType == DataViewType::BIGUINT64) {
        jsTypedArray->SetContentType(ContentType::BigInt);
    } else {
        jsTypedArray->SetContentType(ContentType::Number);
    }
    // 7. If length is not present, then
    //   a. Set obj.[[ByteLength]] to 0.
    //   b. Set obj.[[ByteOffset]] to 0.
    //   c. Set obj.[[ArrayLength]] to 0.
    jsTypedArray->SetTypedArrayName(thread, constructorName);
    jsTypedArray->SetByteLength(0);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(0);
    ASSERT_PRINT(!JSHandle<TaggedObject>(obj)->GetClass()->IsOnHeapFromBitField(), "must be not on heap");
    // 9. Return obj.
    return obj;
}

// es11 22.2.4.2.1 Runtime Semantics: AllocateTypedArray ( constructorName, newTarget, defaultProto )
JSHandle<JSObject> TypedArrayHelper::AllocateSharedTypedArray(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &constructorName,
                                                              const JSHandle<JSTaggedValue> &newTarget,
                                                              const DataViewType arrayType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let proto be ? GetPrototypeFromConstructor(newTarget, defaultProto).
    // 2. Let obj be ! IntegerIndexedObjectCreate(proto).
    JSHandle<JSFunction> typedArrayFunc = TypedArrayHelper::GetSharedConstructorFromType(thread, arrayType);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(typedArrayFunc, newTarget);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 3. Assert: obj.[[ViewedArrayBuffer]] is undefined.
    // 4. Set obj.[[TypedArrayName]] to constructorName.

    // 5. If constructorName is "BigInt64Array" or "BigUint64Array", set obj.[[ContentType]] to BigInt.
    // 6. Otherwise, set obj.[[ContentType]] to Number.
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*obj);
    if (arrayType == DataViewType::BIGINT64 ||
        arrayType == DataViewType::BIGUINT64) {
        jsTypedArray->SetContentType(ContentType::BigInt);
    } else {
        jsTypedArray->SetContentType(ContentType::Number);
    }
    // 7. If length is not present, then
    //   a. Set obj.[[ByteLength]] to 0.
    //   b. Set obj.[[ByteOffset]] to 0.
    //   c. Set obj.[[ArrayLength]] to 0.
    jsTypedArray->SetTypedArrayName(thread, constructorName);
    jsTypedArray->SetByteLength(0);
    jsTypedArray->SetByteOffset(0);
    jsTypedArray->SetArrayLength(0);
    ASSERT_PRINT(!JSHandle<TaggedObject>(obj)->GetClass()->IsOnHeapFromBitField(), "must be not on heap");
    // 9. Return obj.
    return obj;
}

// es11 22.2.4.2.1 Runtime Semantics: AllocateTypedArray ( constructorName, newTarget, defaultProto, length )
JSHandle<JSObject> TypedArrayHelper::AllocateTypedArray(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &constructorName,
                                                        const JSHandle<JSTaggedValue> &newTarget, uint32_t length,
                                                        const DataViewType arrayType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let proto be ? GetPrototypeFromConstructor(newTarget, defaultProto).
    // 2. Let obj be ! IntegerIndexedObjectCreate(proto).
    JSHandle<JSFunction> typedArrayFunc = TypedArrayHelper::GetConstructorFromType(thread, arrayType);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(typedArrayFunc, newTarget);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 3. Assert: obj.[[ViewedArrayBuffer]] is undefined.
    // 4. Set obj.[[TypedArrayName]] to constructorName.
    JSTypedArray::Cast(*obj)->SetTypedArrayName(thread, constructorName);
    // 7. If length is not present, then
    // 8. Else,
    //   a. Perform ? AllocateTypedArrayBuffer(obj, length).
    TypedArrayHelper::AllocateTypedArrayBuffer(thread, obj, length, arrayType);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 9. Return obj.
    return obj;
}

// es11 22.2.4.2.1 Runtime Semantics: AllocateTypedArray ( constructorName, newTarget, defaultProto, length )
JSHandle<JSObject> TypedArrayHelper::AllocateSharedTypedArray(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &constructorName,
                                                              const JSHandle<JSTaggedValue> &newTarget,
                                                              uint32_t length, const DataViewType arrayType)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let proto be ? GetPrototypeFromConstructor(newTarget, defaultProto).
    // 2. Let obj be ! IntegerIndexedObjectCreate(proto).
    JSHandle<JSFunction> typedArrayFunc = TypedArrayHelper::GetSharedConstructorFromType(thread, arrayType);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(typedArrayFunc, newTarget);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 3. Assert: obj.[[ViewedArrayBuffer]] is undefined.
    // 4. Set obj.[[TypedArrayName]] to constructorName.
    JSSharedTypedArray::Cast(*obj)->SetTypedArrayName(thread, constructorName);
    // 7. If length is not present, then
    // 8. Else,
    //   a. Perform ? AllocateTypedArrayBuffer(obj, length).
    TypedArrayHelper::AllocateTypedArrayBuffer<TypedArrayKind::SHARED>(thread, obj, length, arrayType);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 9. Return obj.
    return obj;
}

template JSHandle<JSObject> TypedArrayHelper::TypedArraySpeciesCreate<TypedArrayKind::SHARED>(
    JSThread *thread, const JSHandle<JSTypedArray> &obj, uint32_t argc, JSTaggedType argv[]);
template JSHandle<JSObject> TypedArrayHelper::TypedArraySpeciesCreate<TypedArrayKind::NON_SHARED>(
    JSThread *thread, const JSHandle<JSTypedArray> &obj, uint32_t argc, JSTaggedType argv[]);

// es11 22.2.4.7 TypedArraySpeciesCreate ( exemplar, argumentList )
template <TypedArrayKind typedArrayKind>
JSHandle<JSObject> TypedArrayHelper::TypedArraySpeciesCreate(JSThread *thread, const JSHandle<JSTypedArray> &obj,
                                                             uint32_t argc, JSTaggedType argv[])
{
    // 1. Assert: exemplar is an Object that has [[TypedArrayName]] and [[ContentType]] internal slots.
    // 2. Let defaultConstructor be the intrinsic object listed in column one of Table 61 for
    // exemplar.[[TypedArrayName]].
    JSHandle<JSTaggedValue> buffHandle(thread, JSTaggedValue(argv[0]));
    JSHandle<JSTaggedValue> defaultConstructor =
        TypedArrayHelper::GetConstructor(thread, JSHandle<JSTaggedValue>(obj));
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> result;
    JSHandle<JSTaggedValue> proto(thread, obj->GetJSHClass()->GetPrototype());
    bool isCtrUnchanged = !env->GetTypedArraySpeciesProtectDetector() &&
        !TypedArrayHelper::IsAccessorHasChanged(proto) &&
        !obj->GetJSHClass()->HasConstructor();
    bool isCtrBylen = buffHandle->IsInt();
    if (isCtrUnchanged && isCtrBylen) {
        JSType type = obj->GetJSHClass()->GetObjectType();
        DataViewType arrayType = GetType(type);
        uint32_t length = static_cast<uint32_t>(buffHandle->GetInt());
        // 3. Let result be ? AllocateTypedArray(constructorName, defaultConstructor, length, arrayType).
        if constexpr (typedArrayKind == TypedArrayKind::NON_SHARED) {
            JSHandle<JSTaggedValue> constructorName = GetConstructorNameFromType(thread, arrayType);
            result = TypedArrayHelper::AllocateTypedArray(thread, constructorName,
                                                          defaultConstructor, length, arrayType);
        } else {
            JSHandle<JSTaggedValue> constructorName = GetSharedConstructorNameFromType(thread, arrayType);
            result = TypedArrayHelper::AllocateSharedTypedArray(thread, constructorName,
                                                                defaultConstructor, length, arrayType);
        }
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    } else {
        JSHandle<JSTaggedValue> key = thread->GlobalConstants()->GetHandledConstructorString();
        JSHandle<JSTaggedValue> objConstructor =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key, JSHandle<JSTaggedValue>(obj)).GetValue();
        // 3. Let constructor be ? SpeciesConstructor(exemplar, defaultConstructor).
        JSHandle<JSTaggedValue> thisConstructor =
            JSObject::SlowSpeciesConstructor(thread, objConstructor, defaultConstructor);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        // 4. Let result be ? TypedArrayCreate(constructor, argumentList).
        argv[0] = buffHandle.GetTaggedType();
        result = TypedArrayHelper::TypedArrayCreate(thread, thisConstructor, argc, argv);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    }
    // 5. If result.[[ContentType]] ≠ exemplar.[[ContentType]], throw a TypeError exception.
    ContentType objContentType = obj->GetContentType();
    ContentType resultContentType = JSHandle<JSTypedArray>::Cast(result)->GetContentType();
    if (objContentType != resultContentType) {
        JSHandle<JSObject> exception(thread, JSTaggedValue::Exception());
        THROW_TYPE_ERROR_AND_RETURN(thread, "resultContentType is not equal objContentType.", exception);
    }
    return result;
}

// es11 22.2.4.6 TypedArrayCreate ( constructor, argumentList )
JSHandle<JSObject> TypedArrayHelper::TypedArrayCreate(JSThread *thread, const JSHandle<JSTaggedValue> &constructor,
                                                      uint32_t argc, const JSTaggedType argv[])
{
    // 1. Let newTypedArray be ? Construct(constructor, argumentList).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, argc);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    info->SetCallArg(argc, argv);
    JSTaggedValue taggedArray = JSFunction::Construct(info);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    if (!taggedArray.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to construct the Typedarray.",
                                    JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    }
    JSHandle<JSTaggedValue> taggedArrayHandle(thread, taggedArray);
    // 2. Perform ? ValidateTypedArray(newTypedArray).
    TypedArrayHelper::ValidateTypedArray(thread, taggedArrayHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    JSHandle<JSObject> newTypedArray(taggedArrayHandle);
    // 3. If argumentList is a List of a single Number, then
    //   a. If newTypedArray.[[ArrayLength]] < argumentList[0], throw a TypeError exception.
    if (argc == 1) {
        JSHandle<JSTaggedValue> singleNumber(thread, JSTaggedValue(argv[0]));
        if (JSHandle<JSTypedArray>::Cast(newTypedArray)->GetArrayLength() <
            JSTaggedValue::ToUint32(thread, singleNumber)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "the length of newTypedArray is not a correct value.",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
    }
    // 4. Return newTypedArray.
    return newTypedArray;
}

// TypedArrayCreateSameType ( exemplar, argumentList )
JSHandle<JSObject> TypedArrayHelper::TypedArrayCreateSameType(JSThread *thread, const JSHandle<JSTypedArray> &obj,
                                                              uint32_t argc, JSTaggedType argv[])
{
    // 1. Let constructor be the intrinsic object associated with the constructor name exemplar.[[TypedArrayName]]
    // in Table 70.
    JSHandle<JSTaggedValue> buffHandle(thread, JSTaggedValue(argv[0]));
    JSHandle<JSTaggedValue> constructor =
        TypedArrayHelper::GetConstructor(thread, JSHandle<JSTaggedValue>(obj));
    argv[0] = buffHandle.GetTaggedType();
    // 2. Let result be ? TypedArrayCreate(constructor, argumentList).
    JSHandle<JSObject> result = TypedArrayHelper::TypedArrayCreate(thread, constructor, argc, argv);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
    // 3. Assert: result has [[TypedArrayName]] and [[ContentType]] internal slots.
    // 4. Assert: result.[[ContentType]] is exemplar.[[ContentType]].
    [[maybe_unused]] ContentType objContentType = obj->GetContentType();
    [[maybe_unused]] ContentType resultContentType = JSHandle<JSTypedArray>::Cast(result)->GetContentType();
    ASSERT(objContentType == resultContentType);
    return result;
}

// es11 22.2.3.5.1 Runtime Semantics: ValidateTypedArray ( O )
JSTaggedValue TypedArrayHelper::ValidateTypedArray(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    // 1. Perform ? RequireInternalSlot(O, [[TypedArrayName]]).
    // 2. Assert: O has a [[ViewedArrayBuffer]] internal slot.
    if (!value->IsTypedArray() && !value->IsSharedTypedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The O is not a TypedArray.", JSTaggedValue::Exception());
    }
    // 3. Let buffer be O.[[ViewedArrayBuffer]].
    // 4. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    JSTaggedValue buffer = JSHandle<JSTypedArray>::Cast(value)->GetViewedArrayBufferOrByteArray();
    if (buffer.IsSendableArrayBuffer() && BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    if (!buffer.IsSendableArrayBuffer() && BuiltinsArrayBuffer::IsDetachedBuffer(buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The ViewedArrayBuffer of O is detached buffer.",
                                    JSTaggedValue::Exception());
    }
    // 5. Return buffer.
    return buffer;
}

int32_t TypedArrayHelper::SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                                      const JSHandle<JSTaggedValue> &buffer, const JSHandle<JSTaggedValue> &firstValue,
                                      const JSHandle<JSTaggedValue> &secondValue)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Assert: Both Type(x) and Type(y) are Number or both are BigInt.
    ASSERT((firstValue->IsNumber() && secondValue->IsNumber()) || (firstValue->IsBigInt() && secondValue->IsBigInt()));
    // 2. If the argument comparefn is not undefined, then
    //   a. Let v be Call(comparefn, undefined, «x, y»).
    //   b. ReturnIfAbrupt(v).
    //   c. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    //   d. If v is NaN, return +0.
    //   e. Return v.
    if (!callbackfnHandle->IsUndefined()) {
        const uint32_t argsLength = 2;
        JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackfnHandle, undefined, undefined, argsLength);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        info->SetCallArg(firstValue.GetTaggedValue(), secondValue.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        if (buffer->IsSendableArrayBuffer() &&
            BuiltinsSendableArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The buffer is detached sendable buffer.", 0);
        }
        if (!buffer->IsSendableArrayBuffer() &&
            BuiltinsArrayBuffer::IsDetachedBuffer(buffer.GetTaggedValue())) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "The buffer is detached buffer.", 0);
        }
        JSHandle<JSTaggedValue> testResult(thread, callResult);
        if (testResult->IsBigInt()) {
            JSHandle<BigInt> bigIntResult = JSHandle<BigInt>::Cast(testResult);
            JSHandle<JSTaggedValue> zero(thread, JSTaggedValue(0));
            ComparisonResult compareResult = BigInt::CompareWithNumber(bigIntResult, zero);
            if (compareResult == ComparisonResult::LESS) {
                return -1;
            }
            if (compareResult == ComparisonResult::GREAT) {
                return 1;
            }
            return +0;
        }
        JSTaggedNumber v = JSTaggedValue::ToNumber(thread, testResult);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        double value = v.GetNumber();
        if (std::isnan(value)) {
            return +0;
        }
        return value;
    }
    if (firstValue->IsNumber()) {
        // 3. If x and y are both NaN, return +0.
        if (NumberHelper::IsNaN(firstValue.GetTaggedValue())) {
            if (NumberHelper::IsNaN(secondValue.GetTaggedValue())) {
                return +0;
            }
            // 4. If x is NaN, return 1.
            return 1;
        }
        // 5. If y is NaN, return -1.
        if (NumberHelper::IsNaN(secondValue.GetTaggedValue())) {
            return -1;
        }
        ComparisonResult compareResult = JSTaggedValue::Compare(thread, firstValue, secondValue);
        // 6. If x < y, return -1.
        // 7. If x > y, return 1.
        // 8. If x is -0 and y is +0, return -1.
        // 9. If x is +0 and y is -0, return 1.
        // 10. Return +0.
        if (compareResult == ComparisonResult::LESS) {
            return -1;
        }
        if (compareResult == ComparisonResult::GREAT) {
            return 1;
        }
        JSTaggedNumber xNumber = JSTaggedValue::ToNumber(thread, firstValue);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        JSTaggedNumber yNumber = JSTaggedValue::ToNumber(thread, secondValue);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
        double eZeroTemp = -0.0;
        auto eZero = JSTaggedNumber(eZeroTemp);
        double pZeroTemp = +0.0;
        auto pZero = JSTaggedNumber(pZeroTemp);
        if (JSTaggedNumber::SameValue(xNumber, eZero) && JSTaggedNumber::SameValue(yNumber, pZero)) {
            return -1;
        }
        if (JSTaggedNumber::SameValue(xNumber, pZero) && JSTaggedNumber::SameValue(yNumber, eZero)) {
            return 1;
        }
    } else {
        ComparisonResult compareResult = JSTaggedValue::Compare(thread, firstValue, secondValue);
        if (compareResult == ComparisonResult::LESS) {
            return -1;
        }
        if (compareResult == ComparisonResult::GREAT) {
            return 1;
        }
    }
    return +0;
}

bool TypedArrayHelper::IsNativeArrayIterator(JSThread *thread,
    const JSHandle<JSTaggedValue> &obj, JSHandle<JSTaggedValue> &iterMethod)
{
    if (iterMethod->IsUndefined() || (!obj->IsTypedArray() && !obj->IsArray(thread))) {
        return false;
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    if (!JSTaggedValue::SameValue(iterMethod, env->GetTypedArrayProtoValuesFunction()) &&
        !JSTaggedValue::SameValue(iterMethod, env->GetArrayProtoValuesFunction())) {
        return false;
    }

    JSHandle<JSTaggedValue> iterator = JSIterator::GetIterator(thread, obj, iterMethod);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<JSTaggedValue> nextKey = thread->GlobalConstants()->GetHandledNextString();
    JSHandle<JSTaggedValue> iterNext = JSObject::GetMethod(thread, iterator, nextKey);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!iterNext->IsJSFunction()) {
        return false;
    }
    // Array and TypedArray use the same JSArrayIterator.
    return JSHandle<JSFunction>::Cast(iterNext)->GetNativePointer() == reinterpret_cast<void *>(JSArrayIterator::Next);
}
}  // namespace panda::ecmascript::base
