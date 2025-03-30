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

#include "ecmascript/builtins/builtins_shared_array.h"


#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/base/sort_helper.h"

namespace panda::ecmascript::builtins {
namespace {
    constexpr int32_t COUNT_LENGTH_AND_INIT = 2;
} // namespace
using ArrayHelper = base::ArrayHelper;
using TypedArrayHelper = base::TypedArrayHelper;
using ContainerError = containers::ContainerError;
const CString STRING_SEPERATOR = ",";

// 22.1.1
JSTaggedValue BuiltinsSharedArray::ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_ENTRY_DEBUG_LOG();
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let numberOfArgs be the number of arguments passed to this function call.
    uint32_t argc = argv->GetArgsNumber();

    // 3. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The ArkTS Array's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    // 4. Let proto be GetPrototypeFromConstructor(newTarget, "%ArrayPrototype%").
    // In NewJSObjectByConstructor(), will get prototype.
    // 5. ReturnIfAbrupt(proto).

    // 22.1.1.3 Array(...items )
    JSTaggedValue newArray = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(argc), newTarget).GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!newArray.IsJSSharedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to create array.", JSTaggedValue::Exception());
    }

    // 22.1.1.1 Array ( )
    if (argc == 0) {
        // 6. Return ArrayCreate(0, proto).
        return newArray;
    }

    JSHandle<JSObject> newArrayHandle(thread, newArray);
    // 8. Let k be 0.
    // 9. Let items be a zero-origined List containing the argument items in order.
    // 10. Repeat, while k < numberOfArgs
    //   a. Let Pk be ToString(k).
    //   b. Let itemK be items[k].
    //   c. Let defineStatus be CreateDataProperty(array, Pk, itemK).
    //   d. Assert: defineStatus is true.
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> itemK(thread, JSTaggedValue::Undefined());
    for (uint32_t k = 0; k < argc; k++) {
        key.Update(JSTaggedValue(k));
        itemK.Update(GetCallArg(argv, k));
        if (!itemK->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        JSObject::CreateDataProperty(thread, newArrayHandle, key, itemK);
    }
    // 11. Assert: the value of array’s length property is numberOfArgs.
    // 12. Return array.
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, argc);
    newArrayHandle->GetJSHClass()->SetExtensible(false);
    return newArrayHandle.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedArray::FromArrayNoMaping(JSThread *thread, const JSHandle<JSTaggedValue>& items,
                                                     JSHandle<JSObject>& newArrayHandle)
{
    JSHandle<JSObject> arrayLikeObj = JSTaggedValue::ToObject(thread, items);
    JSHandle<JSTaggedValue> arrayLike(arrayLikeObj) ;
    int64_t len = ArrayHelper::GetArrayLength(thread, arrayLike);
    int k = 0;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(len, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    while (k < len) {
        mapValue.Update(JSArray::FastGetPropertyByValue(thread, arrayLike, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        if (!mapValue->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }

        eleArray->Set(thread, k, mapValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
        thread->CheckSafepointIfSuspended();
    }
    newArrayHandle->SetElements(thread, eleArray);
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, k);
    return newArrayHandle.GetTaggedValue();
}

template<bool itemIsSharedArray>
JSTaggedValue BuiltinsSharedArray::FromArray(JSThread *thread, const JSHandle<JSTaggedValue>& items,
                                             const JSHandle<JSTaggedValue>& thisArgHandle,
                                             const JSHandle<JSTaggedValue> mapfn,
                                             JSHandle<JSObject>& newArrayHandle)
{
    JSHandle<JSObject> arrayLikeObj = JSTaggedValue::ToObject(thread, items);
    JSHandle<JSTaggedValue> arrayLike(arrayLikeObj) ;
    int64_t len = ArrayHelper::GetArrayLength(thread, arrayLike);
    int k = 0;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    CVector<JSHandle<JSTaggedValue>> valueVec(0);
    valueVec.reserve(len);
    JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        if constexpr (itemIsSharedArray) {
            mapValue.Update(ElementAccessor::Get(thread, arrayLikeObj, k));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            mapValue.Update(JSArray::FastGetPropertyByValue(thread, arrayLike, k));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }

        key.Update(JSTaggedValue(k));
        const uint32_t argsLength = 2; // 2: «kValue, k»
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(mapValue.GetTaggedValue(), key.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> result = JSHandle<JSTaggedValue>(thread, callResult);
        len = ArrayHelper::GetArrayLength(thread, arrayLike);

        if (!result->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }

        valueVec.push_back(result);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
        thread->CheckSafepointIfSuspended();
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(k, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    for (int idx = 0; idx < k; ++idx) {
        eleArray->Set(thread, idx, valueVec[idx]);
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    newArrayHandle->SetElements(thread, eleArray);
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, k);
    return newArrayHandle.GetTaggedValue();
}

template JSTaggedValue BuiltinsSharedArray::FromArray<true>(JSThread *thread, const JSHandle<JSTaggedValue>& items,
                                                            const JSHandle<JSTaggedValue>& thisArgHandle,
                                                            const JSHandle<JSTaggedValue> mapfn,
                                                            JSHandle<JSObject>& newArrayHandle);

template JSTaggedValue BuiltinsSharedArray::FromArray<false>(JSThread *thread, const JSHandle<JSTaggedValue>& items,
                                                             const JSHandle<JSTaggedValue>& thisArgHandle,
                                                             const JSHandle<JSTaggedValue> mapfn,
                                                             JSHandle<JSObject>& newArrayHandle);

JSTaggedValue BuiltinsSharedArray::FromSharedArray(JSThread *thread, const JSHandle<JSTaggedValue>& items,
                                                   JSHandle<JSObject>& newArrayHandle)
{
    int64_t len = ArrayHelper::GetLength(thread, items);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(len, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, items);
    TaggedArray *element = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    eleArray->Copy(thread, 0, 0, element, len);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    newArrayHandle->SetElements(thread, eleArray);
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, len);
    return newArrayHandle.GetTaggedValue();
}

// 22.1.2.1 Array.from ( items [ , mapfn [ , thisArg ] ] )
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsSharedArray::From(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, From);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let C be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If mapfn is undefined, let mapping be false.
    bool mapping = false;
    // 3. else
    // a. If IsCallable(mapfn) is false, throw a TypeError exception.
    // b. If thisArg was supplied, let T be thisArg; else let T be undefined.
    // c. Let mapping be true
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, INDEX_TWO);
    JSHandle<JSTaggedValue> mapfn = GetCallArg(argv, 1);
    if (!mapfn->IsUndefined()) {
        if (!mapfn->IsCallable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "the mapfn is not callable.", JSTaggedValue::Exception());
        }
        mapping = true;
    }
    // 4. Let usingIterator be GetMethod(items, @@iterator).
    JSHandle<JSTaggedValue> items = GetCallArg(argv, 0);
    if (items->IsNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "The items is null.", JSTaggedValue::Exception());
    }
    if (!mapping && items->IsString()) {
        JSHandle<EcmaString> strItems(items);
        return BuiltinsString::StringToSList(thread, strItems);
    }
    // Fast path for TypedArray
    if (!mapping && (items->IsTypedArray() || items->IsSharedTypedArray())) {
        auto error = ContainerError::ParamError(thread, "Parameter error.TypedArray not support yet.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> usingIterator = JSObject::GetMethod(thread, items, iteratorSymbol);
    // 5. ReturnIfAbrupt(usingIterator).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. If usingIterator is not undefined, then
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!usingIterator->IsUndefined()) {
        // Fast path for MapIterator
        JSHandle<JSTaggedValue> iterator(thread, JSTaggedValue::Hole());
        if (!mapping && items->IsJSMapIterator()) {
            iterator = JSIterator::GetIterator(thread, items, usingIterator);
            if (iterator->IsJSMapIterator()) {
                return JSMapIterator::MapIteratorToList(thread, iterator);
            }
        }

        //   a. If IsConstructor(C) is true, then
        //     i. Let A be Construct(C).
        //   b. Else,
        //     i. Let A be ArrayCreate(0).
        //   c. ReturnIfAbrupt(A).
        JSTaggedValue newArray;
        if (thisHandle->IsConstructor()) {
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, thisHandle, undefined, undefined, 0);
            ASSERT(info != nullptr);
            newArray = JSFunction::Construct(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            newArray = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (!newArray.IsJSSharedArray()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to construct the array.", JSTaggedValue::Exception());
        }
        JSHandle<JSObject> newArrayHandle(thread, newArray);
        //   d. Let iterator be GetIterator(items, usingIterator).
        if (iterator->IsHole()) {
            iterator = JSIterator::GetIterator(thread, items, usingIterator);
            //   e. ReturnIfAbrupt(iterator).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        //   f. Let k be 0.
        int k = 0;
        //   g. Repeat
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
        // fastpath for jsarray
        if ((items->IsJSArray() && iterator->IsJSArrayIterator())) {
            if (mapping) {
                return BuiltinsSharedArray::FromArray<false>(thread, items, thisArgHandle, mapfn, newArrayHandle);
            } else {
                return BuiltinsSharedArray::FromArrayNoMaping(thread, items, newArrayHandle);
            }
        }

        if (items->IsJSSharedArray()) {
            if (mapping) {
                return BuiltinsSharedArray::FromArray<true>(thread, items, thisArgHandle, mapfn, newArrayHandle);
            } else {
                return BuiltinsSharedArray::FromSharedArray(thread, items, newArrayHandle);
            }
        }

        while (true) {
            key.Update(JSTaggedValue(k));
            //     i. Let Pk be ToString(k).
            //     ii. Let next be IteratorStep(iterator).
            JSHandle<JSTaggedValue> next = JSIterator::IteratorStep(thread, iterator);
            //     iii. ReturnIfAbrupt(next).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            //     iv. If next is false, then
            //       1. Let setStatus be Set(A, "length", k, true).
            //       2. ReturnIfAbrupt(setStatus).
            //       3. Return A.
            if (next->IsFalse()) {
                JSSharedArray::LengthSetter(thread, newArrayHandle, key, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                newArrayHandle->GetJSHClass()->SetExtensible(false);
                return newArrayHandle.GetTaggedValue();
            }
            //     v. Let nextValue be IteratorValue(next).
            JSHandle<JSTaggedValue> nextValue = JSIterator::IteratorValue(thread, next);
            //     vi. ReturnIfAbrupt(nextValue).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            //     vii. If mapping is true, then
            //       1. Let mappedValue be Call(mapfn, T, «nextValue, k»).
            //       2. If mappedValue is an abrupt completion, return IteratorClose(iterator, mappedValue).
            //       3. Let mappedValue be mappedValue.[[value]].
            //     viii. Else, let mappedValue be nextValue.
            if (mapping) {
                const uint32_t argsLength = 2; // 2: «nextValue, k»
                EcmaRuntimeCallInfo *info =
                    EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                ASSERT(info != nullptr);
                info->SetCallArg(nextValue.GetTaggedValue(), key.GetTaggedValue());
                JSTaggedValue callResult = JSFunction::Call(info);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread,
                    JSIterator::IteratorClose(thread, iterator, mapValue).GetTaggedValue());
                mapValue.Update(callResult);
            } else {
                mapValue.Update(nextValue.GetTaggedValue());
            }
            if (!mapValue->IsSharedType()) {
                auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
            }
            //     ix. Let defineStatus be CreateDataPropertyOrThrow(A, Pk, mappedValue).
            //     x. If defineStatus is an abrupt completion, return IteratorClose(iterator, defineStatus).
            //     xi. Increase k by 1.
            JSHandle<JSTaggedValue> defineStatus(thread, JSTaggedValue(JSObject::CreateDataPropertyOrThrow(
                thread, newArrayHandle, key, mapValue, SCheckMode::SKIP)));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread,
                JSIterator::IteratorClose(thread, iterator, defineStatus).GetTaggedValue());
            k++;
        }
    }
    // 7. Assert: items is not an Iterable so assume it is an array-like object.
    // 8. Let arrayLike be ToObject(items).
    JSHandle<JSObject> arrayLikeObj = JSTaggedValue::ToObject(thread, items);
    // 9. ReturnIfAbrupt(arrayLike).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> arrayLike(arrayLikeObj);
    // 10. Let len be ToLength(Get(arrayLike, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, arrayLike);
    // 11. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 12. If IsConstructor(C) is true, then
    //   a. Let A be Construct(C, «len»).
    // 13. Else,
    //   a. Let A be ArrayCreate(len).
    // 14. ReturnIfAbrupt(A).
    JSTaggedValue newArray;
    if (thisHandle->IsConstructor()) {
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, thisHandle, undefined, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(JSTaggedValue(len));
        newArray = JSFunction::Construct(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        newArray = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(static_cast<double>(len))).GetTaggedValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    if (!newArray.IsJSSharedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to construct the array.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    // 15. Let k be 0.
    // 16. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(arrayLike, Pk).
    //   d. If mapping is true, then
    //     i. Let mappedValue be Call(mapfn, T, «kValue, k»).
    //   e. Else, let mappedValue be kValue.
    //   f. Let defineStatus be CreateDataPropertyOrThrow(A, Pk, mappedValue).
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mapValue(thread, JSTaggedValue::Undefined());
    int64_t k = 0;
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(len, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    while (k < len) {
        JSHandle<JSTaggedValue> kValue = JSSharedArray::FastGetPropertyByValue(thread, arrayLike, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (mapping) {
            key.Update(JSTaggedValue(k));
            const uint32_t argsLength = 2; // 2: «kValue, k»
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            ASSERT(info != nullptr);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            mapValue.Update(callResult);
        } else {
            mapValue.Update(kValue.GetTaggedValue());
        }
        if (!mapValue->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        eleArray->Set(thread, k, mapValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    newArrayHandle->SetElements(thread, eleArray);
    // 17. Let setStatus be Set(A, "length", len, true).
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, len);
    newArrayHandle->GetJSHClass()->SetExtensible(false);
    // 18. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 19. Return A.
    return newArrayHandle.GetTaggedValue();
}

// Array.create ( arrayLength, initialValue )
JSTaggedValue BuiltinsSharedArray::Create(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Create);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (argv->GetArgsNumber() < COUNT_LENGTH_AND_INIT) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Not enough parameters.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSTaggedValue> arrayLengthValue = GetCallArg(argv, 0);
    if (!arrayLengthValue->IsNumber()) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Invalid array length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    auto arrayLength = JSTaggedValue::ToUint32(thread, arrayLengthValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (JSTaggedNumber(arrayLengthValue.GetTaggedValue()).GetNumber() != arrayLength) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Invalid array length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> initValue = GetCallArg(argv, 1);
    if (!initValue->IsSharedType()) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSTaggedValue newArray;
    if (thisHandle->IsConstructor()) {
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, thisHandle, undefined, undefined, 0);
        ASSERT(info != nullptr);
        newArray = JSFunction::Construct(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        newArray = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    if (!newArray.IsJSSharedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to construct the array.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto elements = factory->NewSOldSpaceTaggedArray(arrayLength, JSTaggedValue::Hole());
    for (uint32_t k = 0; k < arrayLength; k++) {
        elements->Set(thread, k, initValue);
    }
    newArrayHandle->SetElements(thread, elements);
    auto len = JSHandle<JSTaggedValue>(thread, JSTaggedValue(arrayLength));
    JSSharedArray::LengthSetter(thread, newArrayHandle, len, true);
    newArrayHandle->GetJSHClass()->SetExtensible(false);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Return A.
    return newArrayHandle.GetTaggedValue();
}

// Array.isArray ( arg )
JSTaggedValue BuiltinsSharedArray::IsArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, IsArray);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (GetCallArg(argv, 0)->IsJSSharedArray()) {
        return GetTaggedBoolean(true);
    }
    return GetTaggedBoolean(false);
}

// 22.1.2.5 get Array [ @@species ]
JSTaggedValue BuiltinsSharedArray::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Species);
    // 1. Return the this value.
    return GetThis(argv).GetTaggedValue();
}

int64_t BuiltinsSharedArray::CalNewArrayLen(JSThread *thread, EcmaRuntimeCallInfo *argv, int argc)
{
    int64_t newArrayLen = 0;
    JSMutableHandle<JSTaggedValue> ele(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < argc; i++) {
        ele.Update(GetCallArg(argv, i));

        if (!ele->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, newArrayLen);
        }
        // a. Let spreadable be ? IsConcatSpreadable(E).
        bool isSpreadable = ArrayHelper::IsConcatSpreadable(thread, ele);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayLen);
        // b. If spreadable is true, then
        if (!isSpreadable) {
            newArrayLen++;
            continue;
        }

        // i. Let k be 0.
        // ii. Let len be ? LengthOfArrayLike(E).
        // iii. If n + len > 253 - 1, throw a TypeError exception.
        int64_t len = ArrayHelper::GetArrayLength(thread, ele);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayLen);
        if (newArrayLen + len > base::MAX_SAFE_INTEGER || newArrayLen + len < newArrayLen) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", newArrayLen);
        }

        newArrayLen += len;
    }

    return newArrayLen;
}

int64_t BuiltinsSharedArray::FillNewTaggedArray(JSThread *thread, EcmaRuntimeCallInfo *argv, int argc,
                                                int64_t newArrayIdx, JSHandle<TaggedArray> &eleArray)
{
    JSMutableHandle<JSTaggedValue> ele(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < argc; i++) {
        ele.Update(GetCallArg(argv, i));

        // a. Let spreadable be ? IsConcatSpreadable(E).
        bool isSpreadable = ArrayHelper::IsConcatSpreadable(thread, ele);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayIdx);
        // b. If spreadable is true, then
        if (!isSpreadable) {
            // ii. If n ≥ 253 - 1, throw a TypeError exception.
            if (newArrayIdx >= base::MAX_SAFE_INTEGER) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", newArrayIdx);
            }
            // // iii. Perform ? CreateDataPropertyOrThrow(A, ! ToString(𝔽(n)), E).
            // // iv. Set n to n + 1.
            eleArray->Set(thread, newArrayIdx, ele);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayIdx);
            newArrayIdx++;
            continue;
        }

        // i. Let k be 0.
        // ii. Let len be ? LengthOfArrayLike(E).
        // iii. If n + len > 253 - 1, throw a TypeError exception.
        int64_t len = ArrayHelper::GetArrayLength(thread, ele);
        int64_t k = 0;
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayIdx);

        JSHandle<JSObject> eleObj = JSTaggedValue::ToObject(thread, ele);
        while (k < len) {
            toKey.Update(JSTaggedValue(newArrayIdx));
            kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, eleObj, k));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayIdx);

            if (!kValue->IsSharedType()) {
                auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, newArrayIdx);
            }

            if (!kValue->IsHole()) {
                eleArray->Set(thread, newArrayIdx, kValue);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, newArrayIdx);
            }
            newArrayIdx++;
            k++;
        }
    }

    return newArrayIdx;
}

// 22.1.3.1 Array.prototype.concat ( ...arguments )
JSTaggedValue BuiltinsSharedArray::Concat(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Concat);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    int argc = static_cast<int>(argv->GetArgsNumber());

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The concat method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let A be ArraySpeciesCreate(O, 0).
    JSTaggedValue newArray = JSSharedArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!(newArray.IsECMAObject() || newArray.IsUndefined())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "array must be object or undefined.", JSTaggedValue::Exception());
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    int64_t oldArrayLen = ArrayHelper::GetArrayLength(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t newArrayLen = oldArrayLen + BuiltinsSharedArray::CalNewArrayLen(thread, argv, argc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(newArrayLen, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    TaggedArray *oldElement = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    eleArray->Copy(thread, 0, 0, oldElement, oldArrayLen);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    int64_t res = BuiltinsSharedArray::FillNewTaggedArray(thread, argv, argc, oldArrayLen, eleArray);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    if (newArrayLen != res) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    newArrayHandle->SetElements(thread, eleArray);

    // 6. Perform ? Set(A, "length", 𝔽(n), true).
    JSHandle<JSTaggedValue> lenHandle(thread, JSTaggedValue(newArrayLen));
    JSSharedArray::LengthSetter(thread, newArrayHandle, lenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.4 Array.prototype.entries ( )
JSTaggedValue BuiltinsSharedArray::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The entries method cannot be bound.")

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be ToObject(this value).
    // 2. ReturnIfAbrupt(O).
    JSHandle<JSObject> self = JSTaggedValue::ToObject(thread, GetThis(argv));
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayIterator(O, "key+value").
    JSHandle<JSSharedArrayIterator> iter(factory->NewJSSharedArrayIterator(self, IterationKind::KEY_AND_VALUE));
    return iter.GetTaggedValue();
}
JSTaggedValue BuiltinsSharedArray::CheckElementMeetReq(JSThread *thread,
                                                       JSHandle<JSTaggedValue> &thisObjVal,
                                                       JSHandle<JSTaggedValue> &callbackFnHandle, bool isSome)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisObjVal);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSTaggedValue callResult = GetTaggedBoolean(true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetArrayLength(thread, thisObjVal));
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t k = 0;
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisObjVal);
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, undefined, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        // isSome && callResult.ToBoolean() return true(callResult)
        // !isSome && !callResult.ToBoolean() return false(callResult)
        if (!(isSome ^ callResult.ToBoolean())) {
            return GetTaggedBoolean(callResult.ToBoolean());
        }

        k++;
        thread->CheckSafepointIfSuspended();
    }

    return GetTaggedBoolean(!isSome);
}

// Array.prototype.every ( callbackfn [ , thisArg] )
JSTaggedValue BuiltinsSharedArray::Every(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Every);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The every method cannot be bound.")

    // 3. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 5. Let k be 0.
    // 6. Repeat, while k < len
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

    return CheckElementMeetReq(thread, thisHandle, callbackFnHandle, false);
}

// Array.prototype.some ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::Some(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Some);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The some method cannot be bound.")

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    return CheckElementMeetReq(thread, thisHandle, callbackFnHandle, true);
}

// 22.1.3.6 Array.prototype.fill (value [ , start [ , end ] ] )
JSTaggedValue BuiltinsSharedArray::Fill(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Fill);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The fill method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);

    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsSharedType()) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let relativeStart be ToInteger(start).
    // 6. If relativeStart < 0, let k be max((len + relativeStart),0); else let k be min(relativeStart, len).
    int64_t start = GetNumberArgVal(thread, argv, 1, len, 0);
    // 7. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    // 9. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    int64_t end = GetNumberArgVal(thread, argv, INDEX_TWO, len, len);
    // 10. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 11. Repeat, while k < final
    //   a. Let Pk be ToString(k).
    //   b. Let setStatus be Set(O, Pk, value, true).
    //   c. ReturnIfAbrupt(setStatus).
    //   d. Increase k by 1.
    int64_t k = start;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    TaggedArray *elements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    while (k < end) {
        elements->Set(thread, k, value);
        k++;
    }

    // 12. Return O.
    return thisObjHandle.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedArray::FilterArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
    JSHandle<JSTaggedValue> &thisObjVal, JSHandle<JSObject>& newArrayHandle,
    JSHandle<JSTaggedValue> &callbackFnHandle)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSTaggedValue callResult = GetTaggedBoolean(true);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toIndexHandle(thread, JSTaggedValue::Undefined());
    int64_t k = 0;
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t toIndex = 0;
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisObjVal);
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(len, JSTaggedValue::Undefined(),
    MemSpaceType::SHARED_OLD_SPACE);
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            eleArray->Set(thread, toIndex, kValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            toIndex++;
        }
        k++;
    }

    newArrayHandle->SetElements(thread, eleArray);
    if (TaggedArray::ShouldTrim(len, toIndex)) {
        eleArray->Trim(thread, toIndex);
    }
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, toIndex);

    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.7 Array.prototype.filter ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::Filter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, Filter);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The filter method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let A be ArraySpeciesCreate(O, 0).
    JSTaggedValue newArray = JSSharedArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(0));
    // 8. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    // 9. Let k be 0.
    // 10. Let to be 0.
    // 11. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let selected be ToBoolean(Call(callbackfn, T, «kValue, k, O»)).
    //     iv. ReturnIfAbrupt(selected).
    //     v. If selected is true, then
    //       1. Let status be CreateDataPropertyOrThrow (A, ToString(to), kValue).
    //       2. ReturnIfAbrupt(status).
    //       3. Increase to by 1.
    //   e. Increase k by 1.
    auto opResult =
        FilterArray(thread, thisArgHandle, thisHandle, newArrayHandle, callbackFnHandle);

    return opResult;
}

// 22.1.3.8 Array.prototype.find ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::Find(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Find);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The find method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(predicate) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the predicate is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let testResult be ToBoolean(Call(predicate, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(testResult).
    //   f. If testResult is true, return kValue.
    //   g. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    int64_t k = 0;
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        const uint32_t argsLength = 3; // 3: «kValue, k, O»
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return kValue.GetTaggedValue();
        }
        k++;
    }

    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedArray::GetElementByKey(JSThread *thread, JSHandle<JSObject>& thisObjHandle, uint32_t index)
{
    if (UNLIKELY(ElementAccessor::GetElementsLength(thisObjHandle) <= index)) {
        return JSTaggedValue::Undefined();
    }

    return ElementAccessor::Get(thread, thisObjHandle, index);
}

void BuiltinsSharedArray::SetElementValue(JSThread *thread, JSHandle<JSObject> arrHandle, uint32_t key,
                                          const JSHandle<JSTaggedValue> &value)
{
    if (UNLIKELY(ElementAccessor::GetElementsLength(arrHandle) <= key)) {
        auto error = ContainerError::ParamError(thread, "Set element's index is exceeds the array length.");
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    if (UNLIKELY(!value->IsSharedType())) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    ElementAccessor::Set(thread, arrHandle, key, value, false);
}

// 22.1.3.9 Array.prototype.findIndex ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::FindIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, FindIndex);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The findIndex method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetLength(thread, thisHandle));
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(predicate) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the predicate is not callable.", JSTaggedValue::Exception());
    }

    if (UNLIKELY(ElementAccessor::GetElementsLength(thisObjHandle) < len)) {
        len = ElementAccessor::GetElementsLength(thisObjHandle);
    }

    const int32_t argsLength = 3; // 3: ?kValue, k, O?
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    uint32_t k = 0;
    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let testResult be ToBoolean(Call(predicate, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(testResult).
    //   f. If testResult is true, return k.
    //   g. Increase k by 1.
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle.GetTaggedValue(),
            JSTaggedValue::Undefined(), JSTaggedValue::Undefined(), argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), JSTaggedValue(k), thisHandle.GetTaggedValue());
        auto callResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, callResult);
        if (callResult.ToBoolean()) {
            return GetTaggedDouble(k);
        }
        k++;
    }

    // 9. Return -1.
    return GetTaggedDouble(-1);
}

// 22.1.3.10 Array.prototype.forEach ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The forEach method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetArrayLength(thread, thisHandle));
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
    uint32_t k = 0;
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        k++;
    }

    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedArray::IndexOfSlowPath(
    EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t length = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If len is 0, return −1.
    if (length == 0) {
        return JSTaggedValue(-1);
    }
    // 6. If argument fromIndex was passed let n be ToInteger(fromIndex); else let n be 0.
    int64_t fromIndex = ArrayHelper::GetStartIndexFromArgs(thread, argv, 1, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return IndexOfSlowPath(argv, thread, thisObjVal, length, fromIndex);
}

JSTaggedValue BuiltinsSharedArray::IndexOfSlowPath(
    EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal,
    int64_t length, int64_t fromIndex)
{
    if (fromIndex >= length) {
        return JSTaggedValue(-1);
    }
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    // 11. Repeat, while k < len
    for (int64_t curIndex = fromIndex; curIndex < length; ++curIndex) {
        keyHandle.Update(JSTaggedValue(curIndex));
        bool found = ArrayHelper::ElementIsStrictEqualTo(thread, thisObjVal, keyHandle, target);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (UNLIKELY(found)) {
            return JSTaggedValue(curIndex);
        }
    }
    // 12. Return -1.
    return JSTaggedValue(-1);
}

JSTaggedValue BuiltinsSharedArray::IndexOfStable(EcmaRuntimeCallInfo *argv, JSThread *thread,
                                                 const JSHandle<JSTaggedValue> &thisHandle)
{
    int64_t length = ArrayHelper::GetArrayLength(thread, thisHandle);
    if (length == 0) {
        return JSTaggedValue(-1);
    }
    int64_t fromIndex = 0;
    uint32_t argc = argv->GetArgsNumber();
    // 2: [target, fromIndex]. Note that fromIndex is missing in most usage cases.
    if (UNLIKELY(argc >= 2)) {
        JSHandle<JSTaggedValue> fromIndexHandle = argv->GetCallArg(1);
        fromIndex = ArrayHelper::GetStartIndex(thread, fromIndexHandle, length);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // Slow path when fromIndex is obtained from an ECMAObject
        // due to potential side effects in its 'toString' and 'valueOf' methods which modify the array object.
        if (UNLIKELY(fromIndexHandle->IsECMAObject())) {
            return IndexOfSlowPath(argv, thread, thisHandle, length, fromIndex);
        }
    }
    if (fromIndex >= length) {
        return JSTaggedValue(-1);
    }

    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    return JSStableArray::IndexOf(
        thread, thisHandle, target, static_cast<uint32_t>(fromIndex), static_cast<uint32_t>(length));
}

// 22.1.3.11 Array.prototype.indexOf ( searchElement [ , fromIndex ] )
JSTaggedValue BuiltinsSharedArray::IndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, IndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The indexOf method cannot be bound.")

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);

    return IndexOfStable(argv, thread, thisHandle);
}

// 22.1.3.12 Array.prototype.join (separator)
JSTaggedValue BuiltinsSharedArray::Join(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Join);
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The join method cannot be bound.")

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    auto factory = thread->GetEcmaVM()->GetFactory();
    auto context = thread->GetCurrentEcmaContext();
    bool noCircular = context->JoinStackPushFastPath(thisHandle);
    if (!noCircular) {
        return factory->GetEmptyString().GetTaggedValue();
    }

    return JSStableArray::Join(thisHandle, argv);
}

// 22.1.3.13 Array.prototype.keys ( )
JSTaggedValue BuiltinsSharedArray::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Keys);
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The keys method cannot be bound.")

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    auto opResult = BuiltinsArray::Keys(argv);
    return opResult;
}

// 22.1.3.15 Array.prototype.map ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::Map(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Map);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The map method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t rawLen = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let A be ArraySpeciesCreate(O, len).
    JSTaggedValue newArray =
        JSSharedArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(static_cast<double>(rawLen)));
    // 8. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!newArray.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to create Object.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(rawLen, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);

    // 9. Let k be 0.
    // 10. Repeat, while k < len
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let mappedValue be Call(callbackfn, T, «kValue, k, O»).
    //     iv. ReturnIfAbrupt(mappedValue).
    //     v. Let status be CreateDataPropertyOrThrow (A, Pk, mappedValue).
    //     vi. ReturnIfAbrupt(status).
    //   e. Increase k by 1.
    uint32_t k = 0;
    uint32_t len = static_cast<uint32_t>(rawLen);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mapResultHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        JSTaggedValue mapResult = JSFunction::Call(info);
        if (!mapResult.IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        mapResultHandle.Update(mapResult);
        eleArray->Set(thread, k, mapResultHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    newArrayHandle->SetElements(thread, eleArray);

    // 11. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.16 Array.prototype.pop ( )
JSTaggedValue BuiltinsSharedArray::Pop(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Pop);

    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The pop method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSTaggedValue opResult = PopInner(argv, thisHandle, thisObjHandle);
    return opResult;
}

JSTaggedValue BuiltinsSharedArray::PopInner(EcmaRuntimeCallInfo *argv, JSHandle<JSTaggedValue> &thisHandle,
                                            JSHandle<JSObject> &thisObjHandle)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If len is zero,
    //   a. Let setStatus be Set(O, "length", 0, true).
    //   b. ReturnIfAbrupt(setStatus).
    //   c. Return undefined.
    if (len == 0) {
        JSHandle<JSTaggedValue> lengthValue(thread, JSTaggedValue(0));
        JSSharedArray::LengthSetter(thread, thisObjHandle, lengthValue, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return JSTaggedValue::Undefined();
    }

    // 6. Else len > 0,
    //   a. Let newLen be len–1.
    //   b. Let indx be ToString(newLen).
    //   c. Let element be Get(O, indx).
    //   d. ReturnIfAbrupt(element).
    //   e. Let deleteStatus be DeletePropertyOrThrow(O, indx).
    //   f. ReturnIfAbrupt(deleteStatus).
    //   g. Let setStatus be Set(O, "length", newLen, true).
    //   h. ReturnIfAbrupt(setStatus).
    //   i. Return element.
    int64_t newLen = len - 1;
    JSHandle<JSTaggedValue> element(thread, ElementAccessor::Get(thread, thisObjHandle, newLen));
    // BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, newLen));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint32_t capacity = ElementAccessor::GetElementsLength(thisObjHandle);
    if (TaggedArray::ShouldTrim(capacity, newLen)) {
        TaggedArray *elements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
        elements->Trim(thread, newLen);
    }

    JSSharedArray::Cast(*thisObjHandle)->SetArrayLength(thread, newLen);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return element.GetTaggedValue();
}

// 22.1.3.17 Array.prototype.push ( ...items )
JSTaggedValue BuiltinsSharedArray::Push(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Push);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The push method cannot be bound.")

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. Let argCount be the number of elements in items.
    uint32_t argc = argv->GetArgsNumber();

    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 7. If len + argCount > 253-1, throw a TypeError exception.
    if ((len + static_cast<int64_t>(argc)) > base::MAX_SAFE_INTEGER) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }

    uint32_t newLength = argc + static_cast<uint32_t>(len);
    TaggedArray *element = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    if (newLength > ElementAccessor::GetElementsLength(thisObjHandle)) {
        element = *JSObject::GrowElementsCapacity(thread, thisObjHandle, newLength, true);
    }

    // 8. Repeat, while items is not empty
    //   a. Remove the first element from items and let E be the value of the element.
    //   b. Let setStatus be Set(O, ToString(len), E, true).
    //   c. ReturnIfAbrupt(setStatus).
    //   d. Let len be len+1.
    uint32_t k = 0;
    while (k < argc) {
        JSHandle<JSTaggedValue> kValue = GetCallArg(argv, k);
        if (!kValue->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        element->Set(thread, len, kValue);
        k++;
        len++;
    }

    // 9. Let setStatus be Set(O, "length", len, true).
    JSSharedArray::Cast(*thisObjHandle)->SetArrayLength(thread, len);
    // 10. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 11. Return len.
    return GetTaggedDouble(len);
}

JSTaggedValue BuiltinsSharedArray::ReduceUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisHandle,
    JSHandle<JSObject> &thisObjHandle, int64_t k, int64_t len, JSMutableHandle<JSTaggedValue> &accumulator,
    JSHandle<JSTaggedValue> &callbackFnHandle)
{
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Hole());
    JSTaggedValue callResult = JSTaggedValue::Undefined();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const int32_t argsLength = 4; // 4: «accumulator, kValue, k, O»
    while (k < len) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, undefined, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(accumulator.GetTaggedValue(), kValue.GetTaggedValue(),
                         key.GetTaggedValue(), thisHandle.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        accumulator.Update(callResult);
        k++;
    }

    return accumulator.GetTaggedValue();
}

// 22.1.3.18 Array.prototype.reduce ( callbackfn [ , initialValue ] )
JSTaggedValue BuiltinsSharedArray::Reduce(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Reduce);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t argc = argv->GetArgsNumber();
    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The reduce method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If len is 0 and initialValue is not present, throw a TypeError exception.
    const int32_t argcLimitLength = 2; // argc limit length of the number parameters
    if (len == 0 && argc < argcLimitLength) {  // 2:2 means the number of parameters
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }

    // 7. Let k be len-1.
    int64_t k = 0;
    // 8. If initialValue is present, then
    //   a. Set accumulator to initialValue.
    // 9. Else initialValue is not present,
    //   a. Get last element initial accumulator
    JSMutableHandle<JSTaggedValue> accumulator(thread, JSTaggedValue::Undefined());
    if (argc >= argcLimitLength) { // 2:2 means the number of parameters
        accumulator.Update(GetCallArg(argv, 1).GetTaggedValue());
    } else if (len > 0) {
        accumulator.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }

    auto opResult = ReduceUnStableJSArray(thread, thisHandle, thisObjHandle, k, len, accumulator, callbackFnHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return opResult;
}

// 22.1.3.21 Array.prototype.shift ( )
JSTaggedValue BuiltinsSharedArray::Shift(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Shift);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The shift method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If len is zero, then
    //   a. Let setStatus be Set(O, "length", 0, true).
    //   b. ReturnIfAbrupt(setStatus).
    //   c. Return undefined.
    if (len == 0) {
        JSHandle<JSTaggedValue> zeroLenHandle(thread, JSTaggedValue(len));
        JSSharedArray::LengthSetter(thread, thisObjHandle, zeroLenHandle, false);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return JSTaggedValue::Undefined();
    }

    // 6. Let first be Get(O, "0").
    JSHandle<JSTaggedValue> firstValue(thread, BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, 0));

    // 7. ReturnIfAbrupt(first).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 8. Let k be 1.
    // 9. Repeat, while k < len
    //   a. Let from be ToString(k).
    //   b. Let to be ToString(k–1).
    //   c. Let fromPresent be HasProperty(O, from).
    //   d. ReturnIfAbrupt(fromPresent).
    //   e. If fromPresent is true, then
    //     i. Let fromVal be Get(O, from).
    //     ii. ReturnIfAbrupt(fromVal).
    //     iii. Let setStatus be Set(O, to, fromVal, true).
    //     iv. ReturnIfAbrupt(setStatus).
    //   f. Else fromPresent is false,
    //     i. Let deleteStatus be DeletePropertyOrThrow(O, to).
    //     ii. ReturnIfAbrupt(deleteStatus).
    //   g. Increase k by 1.
    TaggedArray *element = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    int64_t newLen = len - 1;
    element->Copy<true, true>(thread, 0, 1, element, newLen);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t capacity = ElementAccessor::GetElementsLength(thisObjHandle);
    if (TaggedArray::ShouldTrim(capacity, newLen)) {
        TaggedArray *elements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
        elements->Trim(thread, newLen);
    }

    // 12. Let setStatus be Set(O, "length", len–1, true).
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newLen));
    JSSharedArray::LengthSetter(thread, thisObjHandle, newLenHandle, true);
    // 13. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 14. Return first.
    return firstValue.GetTaggedValue();
}

// 22.1.3.22 Array.prototype.slice (start, end)
JSTaggedValue BuiltinsSharedArray::Slice(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Slice);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The slice method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let relativeStart be ToInteger(start).
    int64_t start = GetNumberArgVal(thread, argv, 0, len, 0);
    // 6. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    // 9. ReturnIfAbrupt(relativeEnd).
    // 10. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    int64_t final = GetNumberArgVal(thread, argv, 1, len, len);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 11. Let count be max(final – k, 0).
    int64_t count = final > start ? (final - start) : 0;

    // 12. Let A be ArraySpeciesCreate(O, count).
    JSTaggedValue newArray =
        JSSharedArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(static_cast<double>(count)));
    // 13. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (count == 0) {
        return newArray;
    }
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(count, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    // 14. Let n be 0.
    // 15. Repeat, while start < final
    //   a. Let Pk be ToString(start).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let status be CreateDataPropertyOrThrow(A, ToString(n), kValue ).
    //     iv. ReturnIfAbrupt(status).
    //   e. Increase start by 1.
    //   f. Increase n by 1.
    int64_t n = 0;
    JSMutableHandle<JSTaggedValue> kValueHandle(thread, JSTaggedValue::Undefined());
    while (start < final) {
        kValueHandle.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, start));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        eleArray->Set(thread, n, kValueHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        start++;
        n++;
    }

    newArrayHandle->SetElements(thread, eleArray);
    // 16. Let setStatus be Set(A, "length", n, true).
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(n));
    JSSharedArray::LengthSetter(thread, newArrayHandle, newLenHandle, true);
    // 17. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 18. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.24 Array.prototype.sort (comparefn)
JSTaggedValue BuiltinsSharedArray::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, Sort);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If comparefn is not undefined and IsCallable(comparefn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }

    // 2. Let obj be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The sort method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<TaggedArray> elements(thread, thisObjHandle->GetElements());
    base::TimSort::Sort(thread, elements, callbackFnHandle);
    return thisObjHandle.GetTaggedValue();
}

// 22.1.3.25 Array.prototype.splice (start, deleteCount , ...items )
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsSharedArray::Splice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Splice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    uint32_t argc = argv->GetArgsNumber();
    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The splice method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(
        thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Let relativeStart be ToInteger(start).
    int64_t start = 0;
    int64_t insertCount = 0;
    int64_t actualDeleteCount = 0;
    if (argc > 0) {
        // 6. ReturnIfAbrupt(relativeStart).
        start = GetNumberArgVal(thread, argv, 0, len, 0);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        actualDeleteCount = len - start;
    }
    // 8. If the number of actual arguments is 0, then
    //   a. Let insertCount be 0.
    //   b. Let actualDeleteCount be 0.
    // 9. Else if the number of actual arguments is 1, then
    //   a. Let insertCount be 0.
    //   b. Let actualDeleteCount be len – actualStart.
    // 10. Else,
    //   a. Let insertCount be the number of actual arguments minus 2.
    //   b. Let dc be ToInteger(deleteCount).
    //   c. ReturnIfAbrupt(dc).
    //   d. Let actualDeleteCount be min(max(dc,0), len – actualStart).
    if (argc > 1) {
        insertCount = argc - 2;  // 2:2 means there are two arguments before the insert items.
        JSHandle<JSTaggedValue> msg1 = GetCallArg(argv, 1);
        JSTaggedNumber argDeleteCount = JSTaggedValue::ToInteger(thread, msg1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double deleteCount = argDeleteCount.GetNumber();
        deleteCount = deleteCount > 0 ? deleteCount : 0;
        actualDeleteCount = deleteCount < (len - start) ? deleteCount : len - start;
    }
    // 11. If len+insertCount−actualDeleteCount > 253-1, throw a TypeError exception.
    if (len + insertCount - actualDeleteCount > base::MAX_SAFE_INTEGER) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }
    // 12. Let A be ArraySpeciesCreate(O, actualDeleteCount).
    JSTaggedValue newArray = JSSharedArray::ArraySpeciesCreate(
        thread, thisObjHandle, JSTaggedNumber(static_cast<double>(actualDeleteCount)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> DelArrEle = factory->NewTaggedArray(actualDeleteCount, JSTaggedValue::Undefined(),
                                                              MemSpaceType::SHARED_OLD_SPACE);
    TaggedArray *oldElement = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    // 14. Let k be 0.
    // 15. Repeat, while k < actualDeleteCount
    //   a. Let from be ToString(actualStart+k).
    //   b. Let fromPresent be HasProperty(O, from).
    //   d. If fromPresent is true, then
    //     i. Let fromValue be Get(O, from).
    //     iii. Let status be CreateDataPropertyOrThrow(A, ToString(k), fromValue).
    //   e. Increase k by 1.
    DelArrEle->Copy(thread, 0, start, oldElement, actualDeleteCount);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    newArrayHandle->SetElements(thread, DelArrEle);
    // 16. Let setStatus be Set(A, "length", actualDeleteCount, true).
    JSHandle<JSTaggedValue> deleteCountHandle(thread, JSTaggedValue(actualDeleteCount));
    JSSharedArray::LengthSetter(thread, newArrayHandle, deleteCountHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    int64_t newLen = len - actualDeleteCount + insertCount;
    JSHandle<TaggedArray> eleArray = factory->NewTaggedArray(newLen, JSTaggedValue::Undefined(),
                                                             MemSpaceType::SHARED_OLD_SPACE);
    if (start > 0) {
        eleArray->Copy(thread, 0, 0, oldElement, start);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    eleArray->Copy(thread, start + insertCount, start + actualDeleteCount, oldElement,
                   len - actualDeleteCount - start);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 23. Repeat, while items is not empty
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 2; i < argc; i++) {
        JSHandle<JSTaggedValue> itemValue = GetCallArg(argv, i);
        if (!itemValue->IsSharedType()) {
            auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        eleArray->Set(thread, start, itemValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        start++;
    }

    thisObjHandle->SetElements(thread, eleArray);
    // 24. Let setStatus be Set(O, "length", len – actualDeleteCount + itemCount, true).
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newLen));
    JSSharedArray::LengthSetter(thread, thisObjHandle, newLenHandle, true);
    // 25. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 26. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.27 Array.prototype.toString ( )
JSTaggedValue BuiltinsSharedArray::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();

    // 1. Let array be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The toString method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(array).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let func be Get(array, "join").
    JSHandle<JSTaggedValue> joinKey = thread->GlobalConstants()->GetHandledJoinString();
    JSHandle<JSTaggedValue> callbackFnHandle = JSTaggedValue::GetProperty(thread, thisObjVal, joinKey).GetValue();

    // 4. ReturnIfAbrupt(func).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(func) is false, let func be the intrinsic function %ObjProto_toString% (19.1.3.6).
    if (!callbackFnHandle->IsCallable()) {
        JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
        JSHandle<JSTaggedValue> objectPrototype = env->GetObjectFunctionPrototype();
        JSHandle<JSTaggedValue> toStringKey = thread->GlobalConstants()->GetHandledToStringString();
        callbackFnHandle = JSTaggedValue::GetProperty(thread, objectPrototype, toStringKey).GetValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    const uint32_t argsLength = argv->GetArgsNumber();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisObjVal, undefined, argsLength);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(info != nullptr);
    info->SetCallArg(argsLength, 0, argv, 0);
    auto opResult = JSFunction::Call(info);
    return opResult;
}

// Array.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] )
JSTaggedValue BuiltinsSharedArray::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, ToLocaleString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ARRAY_CHECK_SHARED_ARRAY("The ToLocaleString method cannot be bound.")
    // 1. Let O be ToObject(this value).
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // add this to join stack to avoid circular call
    auto context = thread->GetCurrentEcmaContext();
    bool noCircular = context->JoinStackPushFastPath(thisHandle);
    if (!noCircular) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
    // 6. If len is zero, return the empty String.
    if (len == 0) {
        // pop this from join stack
        context->JoinStackPopFastPath(thisHandle);
        return GetTaggedString(thread, "");
    }

    return ToLocaleStringInternalHandle(argv, thread, context, factory, thisHandle, len);
}

JSTaggedValue BuiltinsSharedArray::ToLocaleStringInternalHandle(EcmaRuntimeCallInfo *argv, JSThread *thread,
    EcmaContext *context, ObjectFactory *factory, const JSHandle<JSTaggedValue> &thisHandle, int64_t len)
{
    JSHandle<JSObject> thisObjHandle(thread, JSObject::Cast(thisHandle.GetTaggedValue()));
    // Inject locales and options argument into a taggedArray
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    // 7. Let firstElement be Get(array, "0").
    // 8. ReturnIfAbrupt(firstElement).
    // 9. If firstElement is undefined or null, then
    //   a. Let R be the empty String.
    // 10. Else
    //   a. Let R be ToString(Invoke(firstElement, "toLocaleString")).
    //   b. ReturnIfAbrupt(R).
    // 11. Let k be 1.
    // 12. Repeat, while k < len
    //   a. Let S be a String value produced by concatenating R and separator.
    //   b. Let nextElement be Get(array, ToString(k)).
    //   c. ReturnIfAbrupt(nextElement).
    //   d. If nextElement is undefined or null, then
    //     i. Let R be the empty String.
    //   e. Else
    //     i. Let R be ToString(Invoke(nextElement, "toLocaleString")).
    //     ii. ReturnIfAbrupt(R).
    //   f. Let R be a String value produced by concatenating S and R.
    //   g. Increase k by 1.
    CString concatStr;
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> key = globalConst->GetHandledToLocaleStringString();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> nextValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> nextHandle(thread, JSTaggedValue::Undefined());
    JSTaggedValue next = globalConst->GetEmptyString();
    for (int64_t k = 0; k < len; k++) {
        nextValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
        if (!nextValue->IsUndefined() && !nextValue->IsNull()) {
            // 2: two args
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, nextValue, undefined, 2);
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
            ASSERT(info != nullptr);
            info->SetCallArg(locales.GetTaggedValue(), options.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Invoke(info, key);
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
            next = callResult;
        } else {
            next = globalConst->GetEmptyString();
        }
        nextHandle.Update(next);
        JSHandle<EcmaString> nextStringHandle = JSTaggedValue::ToString(thread, nextHandle);
        RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
        CString nextString = ConvertToString(*nextStringHandle);
        if (k > 0) {
            concatStr += STRING_SEPERATOR;
            concatStr += nextString;
            continue;
        }
        concatStr += nextString;
    }

    // pop this from join stack
    context->JoinStackPopFastPath(thisHandle);
    // 13. Return R.
    return factory->NewFromUtf8(concatStr).GetTaggedValue();
}

// 22.1.3.28 Array.prototype.unshift ( ...items )
JSTaggedValue BuiltinsSharedArray::Unshift(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Unshift);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 5. Let argCount be the number of actual arguments.
    int64_t argc = argv->GetArgsNumber();

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The unshift method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    int64_t newLen = len + argc;
    if (argc > 0) {
        if (newLen > base::MAX_SAFE_INTEGER) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
        }

        TaggedArray *element = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
        if (newLen > ElementAccessor::GetElementsLength(thisObjHandle)) {
            element = *JSObject::GrowElementsCapacity(thread, thisObjHandle, newLen, true);
        }
        element->Copy<true, true>(thread, argc, 0, element, len);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

        int64_t j = 0;
        JSMutableHandle<JSTaggedValue> toValue(thread, JSTaggedValue::Undefined());
        while (j < argc) {
            toValue.Update(GetCallArg(argv, j));
            if (!toValue->IsSharedType()) {
                auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
            }
            element->Set(thread, j, toValue);
            j++;
        }
    }

    // 7. Let setStatus be Set(O, "length", len+argCount, true).
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newLen));
    JSSharedArray::LengthSetter(thread, thisObjHandle, newLenHandle, true);
    // 8. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 9. Return len+argCount.
    return GetTaggedDouble(newLen);
}

// 22.1.3.29 Array.prototype.values ( )
JSTaggedValue BuiltinsSharedArray::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The values method cannot be bound.")

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be ToObject(this value).
    // 2. ReturnIfAbrupt(O).
    JSHandle<JSObject> self = JSTaggedValue::ToObject(thread, GetThis(argv));
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayIterator(O, "value").
    JSHandle<JSSharedArrayIterator> iter(factory->NewJSSharedArrayIterator(self, IterationKind::VALUE));
    return iter.GetTaggedValue();
}
// 22.1.3.31 Array.prototype [ @@unscopables ]
JSTaggedValue BuiltinsSharedArray::Unscopables(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, Unscopables);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSObject> unscopableList = factory->CreateNullJSObject();

    JSHandle<JSTaggedValue> trueVal(thread, JSTaggedValue::True());

    JSHandle<JSTaggedValue> atKey((factory->NewFromASCII("at")));
    JSObject::CreateDataProperty(thread, unscopableList, atKey, trueVal);

    JSHandle<JSTaggedValue> copyWithKey = globalConst->GetHandledCopyWithinString();
    JSObject::CreateDataProperty(thread, unscopableList, copyWithKey, trueVal);

    JSHandle<JSTaggedValue> entriesKey = globalConst->GetHandledEntriesString();
    JSObject::CreateDataProperty(thread, unscopableList, entriesKey, trueVal);

    JSHandle<JSTaggedValue> fillKey = globalConst->GetHandledFillString();
    JSObject::CreateDataProperty(thread, unscopableList, fillKey, trueVal);

    JSHandle<JSTaggedValue> findKey = globalConst->GetHandledFindString();
    JSObject::CreateDataProperty(thread, unscopableList, findKey, trueVal);

    JSHandle<JSTaggedValue> findIndexKey = globalConst->GetHandledFindIndexString();
    JSObject::CreateDataProperty(thread, unscopableList, findIndexKey, trueVal);

    JSHandle<JSTaggedValue> findLastKey((factory->NewFromASCII("findLast")));
    JSObject::CreateDataProperty(thread, unscopableList, findLastKey, trueVal);

    JSHandle<JSTaggedValue> findLastIndexKey((factory->NewFromASCII("findLastIndex")));
    JSObject::CreateDataProperty(thread, unscopableList, findLastIndexKey, trueVal);

    JSHandle<JSTaggedValue> flatKey = globalConst->GetHandledFlatString();
    JSObject::CreateDataProperty(thread, unscopableList, flatKey, trueVal);

    JSHandle<JSTaggedValue> flatMapKey = globalConst->GetHandledFlatMapString();
    JSObject::CreateDataProperty(thread, unscopableList, flatMapKey, trueVal);

    JSHandle<JSTaggedValue> includesKey = globalConst->GetHandledIncludesString();
    JSObject::CreateDataProperty(thread, unscopableList, includesKey, trueVal);

    JSHandle<JSTaggedValue> keysKey = globalConst->GetHandledKeysString();
    JSObject::CreateDataProperty(thread, unscopableList, keysKey, trueVal);

    JSHandle<JSTaggedValue> valuesKey = globalConst->GetHandledValuesString();
    JSObject::CreateDataProperty(thread, unscopableList, valuesKey, trueVal);

    JSHandle<JSTaggedValue> toReversedKey((factory->NewFromASCII("toReversed")));
    JSObject::CreateDataProperty(thread, unscopableList, toReversedKey, trueVal);

    JSHandle<JSTaggedValue> toSortedKey((factory->NewFromASCII("toSorted")));
    JSObject::CreateDataProperty(thread, unscopableList, toSortedKey, trueVal);

    JSHandle<JSTaggedValue> toSplicedKey((factory->NewFromASCII("toSpliced")));
    JSObject::CreateDataProperty(thread, unscopableList, toSplicedKey, trueVal);
    return unscopableList.GetTaggedValue();
}

// 23.1.3.13 Array.prototype.includes ( searchElement [ , fromIndex ] )
JSTaggedValue BuiltinsSharedArray::Includes(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Includes);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be ? ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The includes method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t argc = argv->GetArgsNumber();
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    JSHandle<JSTaggedValue> searchElement = GetCallArg(argv, 0);

    // 2. Let len be ? LengthOfArrayLike(O).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If len is 0, return false.
    if (len == 0) {
        return GetTaggedBoolean(false);
    }
    // 4. Let n be ? ToIntegerOrInfinity(fromIndex).
    // 5. Assert: If fromIndex is undefined, then n is 0.
    double fromIndex = 0;
    if (argc > 1) {
        JSHandle<JSTaggedValue> msg1 = GetCallArg(argv, 1);
        JSTaggedNumber fromIndexTemp = JSTaggedValue::ToNumber(thread, msg1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        fromIndex = base::NumberHelper::TruncateDouble(fromIndexTemp.GetNumber());
    }

    // 6. If n is +∞, return false.
    // 7. Else if n is -∞, set n to 0.
    if (fromIndex >= len) {
        return GetTaggedBoolean(false);
    } else if (fromIndex < -len) {
        fromIndex = 0;
    }
    // 8. If n ≥ 0, then
    //     a. Let k be n.
    // 9. Else,
    //     a. Let k be len + n.
    //     b. If k < 0, let k be 0.
    int64_t from = (fromIndex >= 0) ? fromIndex : ((len + fromIndex) >= 0 ? len + fromIndex : 0);

    // 10. Repeat, while k < len,
    //     a. Let elementK be ? Get(O, ! ToString(!(k))).
    //     b. If SameValueZero(searchElement, elementK) is true, return true.
    //     c. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValueHandle(thread, JSTaggedValue::Undefined());
    JSHandle<EcmaString> fromStr;
    while (from < len) {
        kValueHandle.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, from));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (JSTaggedValue::SameValueZero(searchElement.GetTaggedValue(), kValueHandle.GetTaggedValue())) {
            return GetTaggedBoolean(true);
        }
        from++;
    }
    // 11. Return false.
    return GetTaggedBoolean(false);
}

// 23.1.3.1 Array.prototype.at ( index )
JSTaggedValue BuiltinsSharedArray::At(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, At);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The at method cannot be bound.")

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let len be ? LengthOfArrayLike(O).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let index be ? ToIntegerOrInfinity(index).
    JSTaggedNumber index = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // ReturnIfAbrupt(index).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. If relativeIndex ≥ 0, then
    //     a. Let k be relativeIndex.
    // 5. Else,
    //     a. Let k be len + relativeIndex.
    int64_t relativeIndex = index.GetNumber();
    int64_t k = 0;
    if (relativeIndex >= 0) {
        k = relativeIndex;
    } else {
        k = len + relativeIndex;
    }

    // 6. If k < 0 or k ≥ len, return undefined.
    if (k < 0 || k >= len) {
        // Return undefined.
        return JSTaggedValue::Undefined();
    }
    // 7. Return ? Get(O, ! ToString(𝔽(k))).
    JSHandle<JSTaggedValue> element(thread, BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return element.GetTaggedValue();
}

// Array.prototype.findLast ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::FindLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, FindLast);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    ARRAY_CHECK_SHARED_ARRAY("The FindLast method cannot be bound.")

    JSHandle<JSObject> thisObjHandle(thread, JSObject::Cast(thisHandle.GetTaggedValue()));
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(predicate) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the predicate is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    // 7. Let k be (len - 1).
    int64_t k = len - 1;
    // 8. Repeat, while k >= 0
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let testResult be ToBoolean(Call(predicate, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(testResult).
    //   f. If testResult is true, return kValue.
    //   g. Decrease k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSTaggedValue callResult = GetTaggedBoolean(false);
    while (k >= 0) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return kValue.GetTaggedValue();
        }
        k--;
    }

    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

// Array.prototype.findLastIndex ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsSharedArray::FindLastIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, FindLastIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    ARRAY_CHECK_SHARED_ARRAY("The FindLastIndex method cannot be bound.")

    JSHandle<JSObject> thisObjHandle(thread, JSObject::Cast(thisHandle.GetTaggedValue()));
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(predicate) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the predicate is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    // 7. Let k be (len - 1).
    int64_t k = len - 1;
    // 8. Repeat, while k >= 0
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let testResult be ToBoolean(Call(predicate, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(testResult).
    //   f. If testResult is true, return k.
    //   g. Decrease k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSTaggedValue callResult = GetTaggedBoolean(false);
    while (k >= 0) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisHandle.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return GetTaggedDouble(k);
        }
        k--;
    }

    // 9. Return -1.
    return GetTaggedDouble(-1);
}

// Array.prototype.shrinkTo ( arrayLength )
JSTaggedValue BuiltinsSharedArray::ShrinkTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, ShrinkTo);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (argv->GetArgsNumber() != 1) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Not enough parameter.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The shrinkTo method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    JSHandle<JSTaggedValue> newLengthValue = GetCallArg(argv, 0);
    if (!newLengthValue->IsNumber()) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Invalid array length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    auto newLength = JSTaggedValue::ToUint32(thread, newLengthValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (JSTaggedNumber(newLengthValue.GetTaggedValue()).GetNumber() != newLength) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Invalid array length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (newLength >= len) {
        return JSTaggedValue::Undefined();
    }
    JSSharedArray::LengthSetter(thread, thisObjHandle, newLengthValue, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

// Array.prototype.ExtendTo ( arrayLength, initialValue )
JSTaggedValue BuiltinsSharedArray::ExtendTo(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, ShrinkTo);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (argv->GetArgsNumber() < COUNT_LENGTH_AND_INIT) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Not enough parameters.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The extendTo method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    JSHandle<JSTaggedValue> newLengthValue = GetCallArg(argv, 0);
    if (!newLengthValue->IsNumber()) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Invalid array length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    auto newLength = JSTaggedValue::ToUint32(thread, newLengthValue);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (JSTaggedNumber(newLengthValue.GetTaggedValue()).GetNumber() != newLength) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Invalid array length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    int64_t length = ArrayHelper::GetLength(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (newLength <= length) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<JSTaggedValue> initValue = GetCallArg(argv, 1);
    if (!initValue->IsSharedType()) {
        auto error = ContainerError::ParamError(thread, "Parameter error.Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    if (newLength > ElementAccessor::GetElementsLength(thisObjHandle)) {
        JSObject::GrowElementsCapacity(thread, thisObjHandle, newLength, true);
    }

    for (uint32_t k = static_cast<uint32_t>(length); k < newLength; k++) {
        BuiltinsSharedArray::SetElementValue(thread, thisObjHandle, k, initValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(newLength));
    JSSharedArray::LengthSetter(thread, thisObjHandle, key, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedArray::LastIndexOfSlowPath(EcmaRuntimeCallInfo *argv, JSThread *thread,
                                                       const JSHandle<JSTaggedValue> &thisHandle)
{
    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t length = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If len is 0, return −1.
    if (length == 0) {
        return JSTaggedValue(-1);
    }
    // 6. If argument fromIndex was passed let n be ToInteger(fromIndex); else let n be 0.
    int64_t fromIndex = ArrayHelper::GetLastStartIndexFromArgs(thread, argv, 1, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return LastIndexOfSlowPath(argv, thread, thisObjVal, fromIndex);
}

JSTaggedValue BuiltinsSharedArray::LastIndexOfSlowPath(EcmaRuntimeCallInfo *argv, JSThread *thread,
                                                       const JSHandle<JSTaggedValue> &thisObjVal, int64_t fromIndex)
{
    if (fromIndex < 0) {
        return JSTaggedValue(-1);
    }
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> target = base::BuiltinsBase::GetCallArg(argv, 0);
    // 11. Repeat, while k < len
    for (int64_t curIndex = fromIndex; curIndex >= 0; --curIndex) {
        keyHandle.Update(JSTaggedValue(curIndex));
        bool found = ArrayHelper::ElementIsStrictEqualTo(thread, thisObjVal, keyHandle, target);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (UNLIKELY(found)) {
            return JSTaggedValue(curIndex);
        }
    }
    // 12. Return -1.
    return JSTaggedValue(-1);
}

// Array.prototype.lastIndexOf ( searchElement [ , fromIndex ] )
JSTaggedValue BuiltinsSharedArray::LastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedArray, LastIndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The lastIndexOf method cannot be bound.")

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return LastIndexOfSlowPath(argv, thread, thisHandle);
}

// Array.of ( ...items )
JSTaggedValue BuiltinsSharedArray::Of(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Of);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // 1. Let len be the actual number of arguments passed to this function.
    uint32_t argc = argv->GetArgsNumber();

    // 3. Let C be the this value.
    // thisHandle variable declare this Macro
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);

    // 4. If IsConstructor(C) is true, then
    //   a. Let A be Construct(C, «len»).
    // 5. Else,
    //   a. Let A be ArrayCreate(len).
    // 6. ReturnIfAbrupt(A).
    JSHandle<JSTaggedValue> newArray;
    if (thisHandle->IsConstructor()) {
        JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, thisHandle, undefined, undefined, 1);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(JSTaggedValue(argc));
        JSTaggedValue taggedArray = JSFunction::Construct(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        newArray = JSHandle<JSTaggedValue>(thread, taggedArray);
    } else {
        newArray = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(argc));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    if (!newArray->IsJSSharedArray()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to create Object.", JSTaggedValue::Exception());
    }

    JSHandle<JSObject> newArrayHandle(newArray);
    if (UNLIKELY(argc > ElementAccessor::GetElementsLength(newArrayHandle))) {
        JSObject::GrowElementsCapacity(thread, JSHandle<JSObject>::Cast(newArrayHandle), argc, true);
    }

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let kValue be items[k].
    //   b. Let Pk be ToString(k).
    //   c. Let defineStatus be CreateDataPropertyOrThrow(A,Pk, kValue).
    //   d. ReturnIfAbrupt(defineStatus).
    //   e. Increase k by 1.
    for (uint32_t k = 0; k < argc; k++) {
        JSHandle<JSTaggedValue> value = argv->GetCallArg(k);
        BuiltinsSharedArray::SetElementValue(thread, newArrayHandle, k, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    JSSharedArray::Cast(*newArrayHandle)->SetArrayLength(thread, argc);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return newArrayHandle.GetTaggedValue();
}

int64_t BuiltinsSharedArray::ConvertTagValueToInteger(JSThread *thread, JSHandle<JSTaggedValue>& number, int64_t len)
{
    JSTaggedNumber targetTemp = JSTaggedValue::ToInteger(thread, number);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    double target = targetTemp.GetNumber();
    // If relativeTarget < 0, let to be max((len + relativeTarget),0); else let to be min(relativeTarget, len).
    if (target < 0) {
        return target + len > 0 ? static_cast<int64_t>(target + len) : 0;
    } else {
        return target < len ? static_cast<int64_t>(target) : len;
    }
}

int64_t BuiltinsSharedArray::GetNumberArgVal(JSThread *thread, EcmaRuntimeCallInfo *argv, uint32_t idx, int64_t len,
                                             int64_t defVal)
{
    JSHandle<JSTaggedValue> argValue = GetCallArg(argv, idx);

    return argValue->IsUndefined() ? defVal : BuiltinsSharedArray::ConvertTagValueToInteger(thread, argValue, len);
}

int64_t BuiltinsSharedArray::GetNumberArgValThrow(JSThread *thread, EcmaRuntimeCallInfo *argv, uint32_t idx,
                                                  int64_t len, const char* err)
{
    JSHandle<JSTaggedValue> argValue = GetCallArg(argv, idx);
    if (UNLIKELY(argValue->IsUndefined())) {
        auto error = ContainerError::BindError(thread, err);
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, 0);
    }

    return BuiltinsSharedArray::ConvertTagValueToInteger(thread, argValue, len);
}

// Array.prototype.copyWithin (target, start [ , end ] )
JSTaggedValue BuiltinsSharedArray::CopyWithin(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, CopyWithin);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // 3. Let C be the this value.
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The CopyWithin method cannot be bound.")

    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    int64_t copyTo = GetNumberArgValThrow(thread, argv, 0, len, "Target index cannot be undefined.");
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let relativeStart be ToInteger(start).
    int64_t copyFrom = GetNumberArgVal(thread, argv, 1, len, 0);
    // 6. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    int64_t copyEnd = GetNumberArgVal(thread, argv, 2, len, len);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 14. Let count be min(final-from, len-to).
    int64_t count = std::min(copyEnd - copyFrom, len - copyTo);

    // 15. If from<to and to<from+count
    //   a. Let direction be -1.
    //   b. Let from be from + count -1.
    //   c. Let to be to + count -1.
    // 16. Else,
    //   a. Let direction = 1.
    if (count > 0) {
        TaggedArray *element = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
        element->Copy<true, true>(thread, copyTo, copyFrom, element, count);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    // 18. Return O.
    return thisObjHandle.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedArray::FastReverse(JSThread *thread, JSHandle<TaggedArray> &elements,
                                               uint32_t lower, uint32_t len, ElementsKind kind)
{
    JSMutableHandle<JSTaggedValue> lowerValueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> upperValueHandle(thread, JSTaggedValue::Undefined());
    uint32_t middle = std::floor(len / 2);
    while (lower != middle) {
        uint32_t upper = len - lower - 1;
        lowerValueHandle.Update(ElementAccessor::FastGet(elements, lower, kind));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        upperValueHandle.Update(ElementAccessor::FastGet(elements, upper, kind));
        ElementAccessor::FastSet(thread, elements, lower, upperValueHandle, kind);
        ElementAccessor::FastSet(thread, elements, upper, lowerValueHandle, kind);
        lower++;
    }
    return base::BuiltinsBase::GetTaggedDouble(true);
}

// Array.prototype.reverse ( )
JSTaggedValue BuiltinsSharedArray::Reverse(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, Reverse);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The Reverse method cannot be bound.")
    JSHandle<JSObject> thisObjHandle(thread, JSObject::Cast(thisHandle.GetTaggedValue()));
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. Repeat, while lower != middle
    //   a. Let upper be len-lower-1.
    //   b. Let upperP be ToString(upper).

    //   c. Let lowerP be ToString(lower).
    //   d. Let lowerExists be HasProperty(O, lowerP).
    //   e. ReturnIfAbrupt(lowerExists).
    //   f. If lowerExists is true, then
    //     i. Let lowerValue be Get(O, lowerP).
    //     ii. ReturnIfAbrupt(lowerValue).
    //   g. Let upperExists be HasProperty(O, upperP).
    //   h. ReturnIfAbrupt(upperExists).
    //     i. If upperExists is true, then
    //     i. Let upperValue be Get(O, upperP).
    //     ii. ReturnIfAbrupt(upperValue).
    //   j. If lowerExists is true and upperExists is true, then
    //     i. Let setStatus be Set(O, lowerP, upperValue, true).
    //     ii. ReturnIfAbrupt(setStatus).
    //     iii. Let setStatus be Set(O, upperP, lowerValue, true).
    //     iv. ReturnIfAbrupt(setStatus).
    //   k. Else if lowerExists is false and upperExists is true, then
    //     i. Let setStatus be Set(O, lowerP, upperValue, true).
    //     ii. ReturnIfAbrupt(setStatus).
    //     iii. Let deleteStatus be DeletePropertyOrThrow (O, upperP).
    //     iv. ReturnIfAbrupt(deleteStatus).
    //   l. Else if lowerExists is true and upperExists is false, then
    //     i. Let deleteStatus be DeletePropertyOrThrow (O, lowerP).
    //     ii. ReturnIfAbrupt(deleteStatus).
    //     iii. Let setStatus be Set(O, upperP, lowerValue, true).
    //     iv. ReturnIfAbrupt(setStatus).
    //   m. Else both lowerExists and upperExists are false,
    //     i. No action is required.
    //   n. Increase lower by 1.
    ElementsKind kind = thisObjHandle->GetClass()->GetElementsKind();
    JSHandle<TaggedArray> elements(thread, thisObjHandle->GetElements());
    bool enableElementsKind = thread->GetEcmaVM()->IsEnableElementsKind();
    if (enableElementsKind) {
        if (kind == ElementsKind::INT || kind == ElementsKind::HOLE_INT) {
            FastReverse(thread, elements, 0, len, ElementsKind::INT);
            return thisObjHandle.GetTaggedValue();
        } else if (kind == ElementsKind::NUMBER || kind == ElementsKind::HOLE_NUMBER) {
            FastReverse(thread, elements, 0, len, ElementsKind::NUMBER);
            return thisObjHandle.GetTaggedValue();
        }
    }
    FastReverse(thread, elements, 0, len, ElementsKind::TAGGED);

    // 8. Return O .
    return thisObjHandle.GetTaggedValue();
}

// Array.prototype.reduceRight ( callbackfn [ , initialValue ] )
JSTaggedValue BuiltinsSharedArray::ReduceRight(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedArray, ReduceRight);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t argc = argv->GetArgsNumber();
    // 1. Let O be ToObject(this value).
    // thisHandle variable declare this Macro
    ARRAY_CHECK_SHARED_ARRAY("The ReduceRight method cannot be bound.")

    JSHandle<JSObject> thisObjHandle(thread, JSObject::Cast(thisHandle.GetTaggedValue()));
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ReduceRightInternalHandle(argv, thread, thisHandle, thisObjHandle, argc, len);
}

JSTaggedValue BuiltinsSharedArray::ReduceRightInternalHandle(EcmaRuntimeCallInfo *argv, JSThread *thread,
    const JSHandle<JSTaggedValue> &thisHandle, JSHandle<JSObject> &thisObjHandle, uint32_t argc, int64_t len)
{
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If len is 0 and initialValue is not present, throw a TypeError exception.
    const int32_t argcLimitLength = 2; // argc limit length of the number parameters
    if (len == 0 && argc < argcLimitLength) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }
    // 7. Let k be len-1.
    int64_t k = len - 1;
    // 8. If initialValue is present, then
    //   a. Set accumulator to initialValue.
    // 9. Else initialValue is not present,
    //   a. Get last element initial accumulator
    JSMutableHandle<JSTaggedValue> accumulator(thread, JSTaggedValue::Undefined());
    if (argc >= argcLimitLength) { // 2:2 means the number of parameters
        accumulator.Update(GetCallArg(argv, 1).GetTaggedValue());
    } else if (k >= 0) {
        accumulator.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        k--;
    }

    // 10. Repeat, while k ≥ 0
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let accumulator be Call(callbackfn, undefined, «accumulator, kValue, k, O»).
    //     iv. ReturnIfAbrupt(accumulator).
    //   e. Decrease k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Hole());
    JSTaggedValue callResult = JSTaggedValue::Undefined();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const int32_t argsLength = 4; // 4: «accumulator, kValue, k, O»
    while (k >= 0) {
        kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k));
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, undefined, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ASSERT(info != nullptr);
        info->SetCallArg(accumulator.GetTaggedValue(), kValue.GetTaggedValue(),
            key.GetTaggedValue(), thisHandle.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        accumulator.Update(callResult);
        k--;
    }

    // 11. Return accumulator.
    return accumulator.GetTaggedValue();
}

}  // namespace panda::ecmascript::builtins
