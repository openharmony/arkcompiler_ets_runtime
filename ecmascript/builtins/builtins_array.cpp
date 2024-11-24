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

#include "ecmascript/builtins/builtins_array.h"

#include <cmath>

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/builtins/builtins_string.h"

namespace panda::ecmascript::builtins {
using ArrayHelper = base::ArrayHelper;
using TypedArrayHelper = base::TypedArrayHelper;
const CString STRING_SEPERATOR = ",";

// 22.1.1
JSTaggedValue BuiltinsArray::ArrayConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_ENTRY_DEBUG_LOG();
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let numberOfArgs be the number of arguments passed to this function call.
    uint32_t argc = argv->GetArgsNumber();

    // 3. If NewTarget is undefined, let newTarget be the active function object, else let newTarget be NewTarget.
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        newTarget = constructor;
    }

    // 4. Let proto be GetPrototypeFromConstructor(newTarget, "%ArrayPrototype%").
    // In NewJSObjectByConstructor(), will get prototype.
    // 5. ReturnIfAbrupt(proto).

    auto CheckAndSetPrototypeModified = [] (JSThread* thread, const JSHandle<JSObject>& newArrayHandle) {
        if (!JSArray::IsProtoNotChangeJSArray(thread, newArrayHandle)) {
            newArrayHandle->GetJSHClass()->SetIsJSArrayPrototypeModified(true);
        }
    };

    // 22.1.1.1 Array ( )
    if (argc == 0) {
        // 6. Return ArrayCreate(0, proto).
        auto arrayHandle = JSArray::ArrayCreate(thread, JSTaggedNumber(0), newTarget);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        auto newArrayHandle = JSHandle<JSObject>::Cast(arrayHandle);
        CheckAndSetPrototypeModified(thread, newArrayHandle);
        return newArrayHandle.GetTaggedValue();
    }

    // 22.1.1.2 Array(len)
    if (argc == 1) {
        // 6. Let array be ArrayCreate(0, proto).
        JSHandle<JSObject> newArrayHandle(JSArray::ArrayCreate(thread, JSTaggedNumber(0), newTarget));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSHandle<JSTaggedValue> len = GetCallArg(argv, 0);
        // 7. If Type(len) is not Number, then
        //   a. Let defineStatus be CreateDataProperty(array, "0", len).
        //   b. Assert: defineStatus is true.
        //   c. Let intLen be 1.
        // 8. Else,
        //   a. Let intLen be ToUint32(len).
        //   b. If intLen ≠ len, throw a RangeError exception.
        // 9. Let setStatus be Set(array, "length", intLen, true).
        // 10. Assert: setStatus is not an abrupt completion.
        uint32_t newLen = 0;
        if (!len->IsNumber()) {
            JSHandle<JSTaggedValue> key0 = thread->GlobalConstants()->GetHandledZeroString();
            JSObject::CreateDataProperty(thread, newArrayHandle, key0, len);
            newLen = 1;
        } else {
            newLen = JSTaggedValue::ToUint32(thread, len);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (JSTaggedNumber(len.GetTaggedValue()).GetNumber() != newLen) {
                THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid array length", JSTaggedValue::Exception());
            }
        }
        JSArray::SetCapacity(thread, newArrayHandle, 0, newLen, true);

        // 11. Return array.
        CheckAndSetPrototypeModified(thread, newArrayHandle);
        return newArrayHandle.GetTaggedValue();
    }

    // not dictionary elements, fast create array from argv elements
    if (argc < JSObject::MAX_GAP) {
        // 6. Create array elements from 'argv'.
#if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
        ElementsKind newKind = ElementsKind::GENERIC;
#else
        auto arrayFunc = thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction();
        ElementsKind newKind = newTarget.GetTaggedValue() == arrayFunc.GetTaggedValue() ?
            ElementsKind::HOLE : ElementsKind::NONE;
#endif
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto elements = factory->NewTaggedArray(argc, JSTaggedValue::Undefined());
        for (uint32_t k = 0; k < argc; k++) {
            auto value = GetCallArg(argv, k);
            newKind = Elements::ToElementsKind(value.GetTaggedValue(), newKind);
            if (value->IsHole()) {
                continue;
            }
            elements->Set(thread, k, value);
        }
        // 7. create array from elements
        auto newArray = JSArray::CreateArrayFromList(thread, newTarget, elements);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (thread->GetEcmaVM()->IsEnableElementsKind()) {
            JSHClass::TransitToElementsKind(thread, newArray, newKind);
        }
        // 8. Return array.
        return newArray.GetTaggedValue();
    }

    // 22.1.1.3 Array(...items )
    JSTaggedValue newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(argc), newTarget).GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!newArray.IsArray(thread)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to create array.", JSTaggedValue::Exception());
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
        if (itemK.GetTaggedValue().IsHole()) {
            itemK.Update(JSTaggedValue::Undefined());
        }
        JSObject::CreateDataProperty(thread, newArrayHandle, key, itemK);
    }
    // 11. Assert: the value of array’s length property is numberOfArgs.
    // 12. Return array.
    JSArray::Cast(*newArrayHandle)->SetArrayLength(thread, argc);
    CheckAndSetPrototypeModified(thread, newArrayHandle);
    return newArrayHandle.GetTaggedValue();
}

// 22.1.2.1 Array.from ( items [ , mapfn [ , thisArg ] ] )
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsArray::From(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, From);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    // 1. Let C be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If mapfn is undefined, let mapping be false.
    bool mapping = false;
    // 3. else
    //   a. If IsCallable(mapfn) is false, throw a TypeError exception.
    //   b. If thisArg was supplied, let T be thisArg; else let T be undefined.
    //   c. Let mapping be true
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
        return BuiltinsString::StringToList(thread, strItems);
    }
    // Fast path for TypedArray
    if (!mapping && items->IsTypedArray()) {
        JSHandle<JSTypedArray> arrayItems(items);
        return BuiltinsArrayBuffer::TypedArrayToList(thread, arrayItems);
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> usingIterator = JSObject::GetMethod(thread, items, iteratorSymbol);
    // 5. ReturnIfAbrupt(usingIterator).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. If usingIterator is not undefined, then
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
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
            newArray = JSFunction::Construct(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (!newArray.IsECMAObject()) {
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
        if (newArrayHandle->IsJSArray() && items->IsJSArray() && iterator->IsJSArrayIterator()) {
            JSHandle<JSObject> arrayLikeObj = JSTaggedValue::ToObject(thread, items);
            JSHandle<JSTaggedValue> arrayLike(arrayLikeObj) ;
            int64_t len = ArrayHelper::GetArrayLength(thread, arrayLike);
            while (k < len) {
                JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, arrayLike, k);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                if (mapping) {
                    key.Update(JSTaggedValue(k));
                    const uint32_t argsLength = 2; // 2: «kValue, k»
                    EcmaRuntimeCallInfo *info =
                        EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue());
                    JSTaggedValue callResult = JSFunction::Call(info);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    mapValue.Update(callResult);
                } else {
                    mapValue.Update(kValue.GetTaggedValue());
                }
                JSArray::TryFastCreateDataProperty(thread, newArrayHandle, k, mapValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                k++;
                len = ArrayHelper::GetArrayLength(thread, arrayLike);
                thread->CheckSafepointIfSuspended();
            }
            return newArrayHandle.GetTaggedValue();
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
                JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, key, true);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
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
                info->SetCallArg(nextValue.GetTaggedValue(), key.GetTaggedValue());
                JSTaggedValue callResult = JSFunction::Call(info);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread,
                    JSIterator::IteratorClose(thread, iterator, mapValue).GetTaggedValue());
                mapValue.Update(callResult);
            } else {
                mapValue.Update(nextValue.GetTaggedValue());
            }
            //     ix. Let defineStatus be CreateDataPropertyOrThrow(A, Pk, mappedValue).
            //     x. If defineStatus is an abrupt completion, return IteratorClose(iterator, defineStatus).
            //     xi. Increase k by 1.
            bool createRes = newArrayHandle->IsJSArray() ?
                                JSArray::TryFastCreateDataProperty(thread, newArrayHandle, k, mapValue) :
                                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, key, mapValue);
                                
            JSHandle<JSTaggedValue> defineStatus(thread, JSTaggedValue(createRes));
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
        info->SetCallArg(JSTaggedValue(len));
        newArray = JSFunction::Construct(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    } else {
        newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(static_cast<double>(len))).GetTaggedValue();
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    if (!newArray.IsECMAObject()) {
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
    while (k < len) {
        JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, arrayLike, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (mapping) {
            key.Update(JSTaggedValue(k));
            const uint32_t argsLength = 2; // 2: «kValue, k»
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, mapfn, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            mapValue.Update(callResult);
        } else {
            mapValue.Update(kValue.GetTaggedValue());
        }
        JSArray::TryFastCreateDataProperty(thread, newArrayHandle, k, mapValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
        thread->CheckSafepointIfSuspended();
    }
    // 17. Let setStatus be Set(A, "length", len, true).
    JSHandle<JSTaggedValue> lenHandle(thread, JSTaggedValue(len));
    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, lenHandle, true);
    // 18. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 19. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.2.2 Array.isArray ( arg )
JSTaggedValue BuiltinsArray::IsArray(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, IsArray);
    // 1. Return IsArray(arg).
    if (GetCallArg(argv, 0)->IsArray(argv->GetThread())) {
        return GetTaggedBoolean(true);
    }
    return GetTaggedBoolean(false);
}

// 22.1.2.3 Array.of ( ...items )
JSTaggedValue BuiltinsArray::Of(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Of);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();

    // 1. Let len be the actual number of arguments passed to this function.
    uint32_t argc = argv->GetArgsNumber();

    // 3. Let C be the this value.
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
        info->SetCallArg(JSTaggedValue(argc));
        JSTaggedValue taggedArray = JSFunction::Construct(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        newArray = JSHandle<JSTaggedValue>(thread, taggedArray);
    } else {
        newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(argc));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    if (!newArray->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to create Object.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> newArrayHandle(newArray);

    // 7. Let k be 0.
    // 8. Repeat, while k < len
    //   a. Let kValue be items[k].
    //   b. Let Pk be ToString(k).
    //   c. Let defineStatus be CreateDataPropertyOrThrow(A,Pk, kValue).
    //   d. ReturnIfAbrupt(defineStatus).
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t k = 0; k < argc; k++) {
        key.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> kValue = GetCallArg(argv, k);
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, key, kValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 9. Let setStatus be Set(A, "length", len, true).
    JSHandle<JSTaggedValue> lenHandle(thread, JSTaggedValue(argc));
    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, lenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 11. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.2.5 get Array [ @@species ]
JSTaggedValue BuiltinsArray::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Species);
    // 1. Return the this value.
    return GetThis(argv).GetTaggedValue();
}

// 22.1.3.1 Array.prototype.concat ( ...arguments )
JSTaggedValue BuiltinsArray::Concat(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Concat);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    int argc = static_cast<int>(argv->GetArgsNumber());

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 2. Let A be ArraySpeciesCreate(O, 0).
    uint32_t arrayLen = 0;
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(arrayLen));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!(newArray.IsECMAObject() || newArray.IsUndefined())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "array must be object or undefined.", JSTaggedValue::Exception());
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();

    // 3. Let n be 0.
    int64_t n = 0;
    JSMutableHandle<JSTaggedValue> ele(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    // 4. Prepend O to items.
    // 5. For each element E of items, do
    for (int i = -1; i < argc; i++) {
        if (i < 0) {
            ele.Update(thisObjHandle.GetTaggedValue());
        } else {
            ele.Update(GetCallArg(argv, i));
        }
        // a. Let spreadable be ? IsConcatSpreadable(E).
        bool isSpreadable = ArrayHelper::IsConcatSpreadable(thread, ele);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // b. If spreadable is true, then
        if (isSpreadable) {
            // i. Let k be 0.
            // ii. Let len be ? LengthOfArrayLike(E).
            // iii. If n + len > 253 - 1, throw a TypeError exception.
            int64_t len = ArrayHelper::GetArrayLength(thread, ele);
            int64_t k = 0;
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (n + len > base::MAX_SAFE_INTEGER) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
            }

            if (ele->IsStableJSArray(thread)) {
                JSStableArray::Concat(thread, newArrayHandle, JSHandle<JSObject>::Cast(ele), k, n);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            // iv. Repeat, while k < len,
            while (k < len) {
                // 1. Let P be ToString(k).
                // 2. Let exists be HasProperty(E, P).
                // 3. If exists is true, then
                fromKey.Update(JSTaggedValue::ToString(thread, JSTaggedValue(k)));
                toKey.Update(JSTaggedValue(n));
                bool exists = JSTaggedValue::HasProperty(thread, ele, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                if (exists) {
                    // a. Let subElement be Get(E, P).
                    JSHandle<JSTaggedValue> fromValHandle =
                        JSArray::FastGetPropertyByValue(thread, ele, fromKey);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                    // b. Perform ? CreateDataPropertyOrThrow(A, ! ToString(𝔽(n)), subElement).
                    JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, fromValHandle);
                    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                }
                // 4. Set n to n + 1.
                // 5. Set k to k + 1.
                n++;
                k++;
                thread->CheckSafepointIfSuspended();
            }
        //c. Else
        } else {
            // ii. If n ≥ 253 - 1, throw a TypeError exception.
            if (n >= base::MAX_SAFE_INTEGER) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
            }
            // iii. Perform ? CreateDataPropertyOrThrow(A, ! ToString(𝔽(n)), E).
            // iv. Set n to n + 1.
            toKey.Update(JSTaggedValue(n));
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, ele);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            n++;
        }
    }
    // 6. Perform ? Set(A, "length", 𝔽(n), true).
    JSHandle<JSTaggedValue> lenHandle(thread, JSTaggedValue(n));
    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, lenHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.3 Array.prototype.copyWithin (target, start [ , end ] )
JSTaggedValue BuiltinsArray::CopyWithin(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, CopyWithin);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, GetThis(argv));
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    int64_t copyTo = 0;
    int64_t copyFrom = 0;
    int64_t copyEnd = len;

    // 5. Let relativeTarget be ToInteger(target).
    JSTaggedNumber targetTemp = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // 6. ReturnIfAbrupt(relativeTarget).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double target = targetTemp.GetNumber();
    // 7. If relativeTarget < 0, let to be max((len + relativeTarget),0); else let to be min(relativeTarget, len).
    if (target < 0) {
        copyTo = target + len > 0 ? target + len : 0;
    } else {
        copyTo = target < len ? target : len;
    }

    // 8. Let relativeStart be ToInteger(start).
    JSTaggedNumber start_t = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 1));
    // 9. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double start = start_t.GetNumber();
    // 10. If relativeStart < 0, let from be max((len + relativeStart),0); else let from be min(relativeStart, len).
    if (start < 0) {
        copyFrom = start + len > 0 ? start + len : 0;
    } else {
        copyFrom = start < len ? start : len;
    }

    // 11. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    double end = len;
    JSHandle<JSTaggedValue> msg3 = GetCallArg(argv, INDEX_TWO);
    if (!msg3->IsUndefined()) {
        JSTaggedNumber temp = JSTaggedValue::ToInteger(thread, msg3);
        // 12. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = temp.GetNumber();
    }

    // 13. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    if (end < 0) {
        copyEnd = end + len > 0 ? end + len : 0;
    } else {
        copyEnd = end < len ? end : len;
    }

    // 14. Let count be min(final-from, len-to).
    int64_t count = (copyEnd - copyFrom < len - copyTo) ? (copyEnd - copyFrom) : (len - copyTo);

    // 15. If from<to and to<from+count
    //   a. Let direction be -1.
    //   b. Let from be from + count -1.
    //   c. Let to be to + count -1.
    // 16. Else,
    //   a. Let direction = 1.
    int64_t direction = 1;
    if (copyFrom < copyTo && copyTo < copyFrom + count) {
        direction = -1;
        copyFrom = copyFrom + count - 1;
        copyTo = copyTo + count - 1;
    }

    // 17. Repeat, while count > 0
    //   a. Let fromKey be ToString(from).
    //   b. Let toKey be ToString(to).
    //   c. Let fromPresent be HasProperty(O, fromKey).
    //   d. ReturnIfAbrupt(fromPresent).
    //   e. If fromPresent is true, then
    //     i. Let fromVal be Get(O, fromKey).
    //     ii. ReturnIfAbrupt(fromVal).
    //     iii. Let setStatus be Set(O, toKey, fromVal, true).
    //     iv. ReturnIfAbrupt(setStatus).
    //   f. Else fromPresent is false,
    //     i. Let deleteStatus be DeletePropertyOrThrow(O, toKey).
    //     ii. ReturnIfAbrupt(deleteStatus).
    //   g. Let from be from + direction.
    //   h. Let to be to + direction.
    //   i. Let count be count − 1.
    JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    while (count > 0) {
        fromKey.Update(JSTaggedValue(copyFrom));
        toKey.Update(JSTaggedValue(copyTo));
        bool exists = (thisObjVal->IsTypedArray() || thisObjVal->IsSharedTypedArray() ||
            JSTaggedValue::HasProperty(thread, thisObjVal, fromKey));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> fromValHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSArray::FastSetPropertyByValue(thread, thisObjVal, toKey, fromValHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            if (thisObjVal->IsJSProxy()) {
                toKey.Update(JSTaggedValue::ToString(thread, toKey).GetTaggedValue());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, toKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        copyFrom = copyFrom + direction;
        copyTo = copyTo + direction;
        count--;
    }

    // 18. Return O.
    return thisObjHandle.GetTaggedValue();
}

// 22.1.3.4 Array.prototype.entries ( )
JSTaggedValue BuiltinsArray::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be ToObject(this value).
    // 2. ReturnIfAbrupt(O).
    JSHandle<JSObject> self = JSTaggedValue::ToObject(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayIterator(O, "key+value").
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(self, IterationKind::KEY_AND_VALUE));
    return iter.GetTaggedValue();
}

// 22.1.3.5 Array.prototype.every ( callbackfn [ , thisArg] )
JSTaggedValue BuiltinsArray::Every(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, Every);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetArrayLength(thread, thisObjVal));
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
    uint32_t k = 0;
    JSTaggedValue callResult = GetTaggedBoolean(true);
    if (thisObjVal->IsStableJSArray(thread)) {
        callResult = JSStableArray::HandleEveryOfStable(thread, thisObjHandle, callbackFnHandle, thisArgHandle, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!callResult.ToBoolean()) {
            return GetTaggedBoolean(false);
        }
    }
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (!callResult.ToBoolean()) {
                return GetTaggedBoolean(false);
            }
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }

    // 9. Return true.
    return GetTaggedBoolean(true);
}

// 22.1.3.6 Array.prototype.fill (value [ , start [ , end ] ] )
JSTaggedValue BuiltinsArray::Fill(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Fill);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    if (thisObjVal->IsJSArray()) {
        bool isDictionary = thisObjHandle->GetJSHClass()->IsDictionaryElement();
        if (isDictionary) {
            uint32_t length = JSArray::Cast(*thisObjHandle)->GetLength();
            uint32_t size = thisObjHandle->GetNumberOfElements();
            if (length - size > JSObject::MAX_GAP) {
                JSObject::TryOptimizeAsFastElements(thread, thisObjHandle);
            }
        }
    }

    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let relativeStart be ToInteger(start).
    JSHandle<JSTaggedValue> startArg = GetCallArg(argv, 1);
    JSTaggedNumber argStartTemp = JSTaggedValue::ToInteger(thread, startArg);
    // 6. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double argStart = argStartTemp.GetNumber();
    // 7. If relativeStart < 0, let k be max((len + relativeStart),0); else let k be min(relativeStart, len).
    int64_t start = 0;
    if (argStart < 0) {
        double tempStart = argStart + len;
        start = tempStart > 0 ? tempStart : 0;
    } else {
        start = argStart < len ? argStart : len;
    }

    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    double argEnd = len;
    JSHandle<JSTaggedValue> endArg = GetCallArg(argv, INDEX_TWO);
    if (!endArg->IsUndefined()) {
        JSTaggedNumber argEndTemp = JSTaggedValue::ToInteger(thread, endArg);
        // 9. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        argEnd = argEndTemp.GetNumber();
    }

    // 10. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    int64_t end = len;
    if (argEnd < 0) {
        double tempEnd = argEnd + len;
        end = tempEnd > 0 ? tempEnd : 0;
    } else {
        end = argEnd < len ? argEnd : len;
    }

    if (start < end) {
        thread->NotifyArrayPrototypeChangedGuardians(thisObjHandle);
    }
    
    // 11. Repeat, while k < final
    //   a. Let Pk be ToString(k).
    //   b. Let setStatus be Set(O, Pk, value, true).
    //   c. ReturnIfAbrupt(setStatus).
    //   d. Increase k by 1.
    if (thisObjVal->IsStableJSArray(thread) && !startArg->IsJSObject() && !endArg->IsJSObject()) {
        return JSStableArray::Fill(thread, thisObjHandle, value, start, end, len);
    }

    int64_t k = start;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    while (k < end) {
        key.Update(JSTaggedValue(k));
        JSArray::FastSetPropertyByValue(thread, thisObjVal, key, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }

    // 12. Return O.
    return thisObjHandle.GetTaggedValue();
}

JSTaggedValue BuiltinsArray::FilterUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
    JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, uint32_t toIndex, JSHandle<JSObject> newArrayHandle,
    JSHandle<JSTaggedValue> &callbackFnHandle)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSTaggedValue callResult = GetTaggedBoolean(true);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toIndexHandle(thread, JSTaggedValue::Undefined());
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (callResult.ToBoolean()) {
                toIndexHandle.Update(JSTaggedValue(toIndex));
                JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toIndexHandle, kValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                toIndex++;
            }
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.7 Array.prototype.filter ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsArray::Filter(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, Filter);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetArrayLength(thread, thisObjVal));
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let A be ArraySpeciesCreate(O, 0).
    int32_t arrayLen = 0;
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(arrayLen));
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
    uint32_t toIndex = 0;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toIndexHandle(thread, JSTaggedValue::Undefined());
    uint32_t k = 0;
    if (thisObjVal->IsStableJSArray(thread)) {
        JSStableArray::Filter(newArrayHandle, thisObjHandle, argv, k, toIndex);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return FilterUnStableJSArray(thread, thisArgHandle, thisObjVal, k, len, toIndex, newArrayHandle, callbackFnHandle);
}

// 22.1.3.8 Array.prototype.find ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsArray::Find(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Find);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
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
    int64_t k = 0;
    while (k < len) {
        JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        const uint32_t argsLength = 3; // 3: «kValue, k, O»
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
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

// 22.1.3.9 Array.prototype.findIndex ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsArray::FindIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, FindIndex);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetLength(thread, thisObjVal));
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
    //   f. If testResult is true, return k.
    //   g. Increase k by 1.
    uint32_t k = 0;
    JSTaggedValue callResult = GetTaggedBoolean(true);
    if (thisObjVal->IsStableJSArray(thread)) {
        callResult = JSStableArray::HandleFindIndexOfStable(thread, thisObjHandle, callbackFnHandle, thisArgHandle, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return GetTaggedDouble(k);
        }
    }
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    while (k < len) {
        JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
        callResult = JSFunction::Call(info);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return GetTaggedDouble(k);
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }

    // 9. Return -1.
    return GetTaggedDouble(-1);
}

// 22.1.3.10 Array.prototype.forEach ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsArray::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    uint64_t len = static_cast<uint64_t>(ArrayHelper::GetArrayLength(thread, thisObjVal));
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
    if (thisObjVal->IsStableJSArray(thread)) {
        JSStableArray::HandleforEachOfStable(thread, thisObjHandle, callbackFnHandle, thisArgHandle, len, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue funcResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }

    // 9. Return undefined.
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArray::IndexOfStable(
    EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    int64_t length = JSHandle<JSArray>::Cast(thisHandle)->GetArrayLength();
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
    if (UNLIKELY(!thisHandle->IsECMAObject())) {
        return IndexOfSlowPath(argv, thread, thisHandle, length, fromIndex);
    }
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    return JSStableArray::IndexOf(
        thread, thisHandle, target, static_cast<uint32_t>(fromIndex), static_cast<uint32_t>(length));
}

JSTaggedValue BuiltinsArray::IndexOfSlowPath(
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

JSTaggedValue BuiltinsArray::IndexOfSlowPath(
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
        thread->CheckSafepointIfSuspended();
    }
    // 12. Return -1.
    return JSTaggedValue(-1);
}

// 22.1.3.11 Array.prototype.indexOf ( searchElement [ , fromIndex ] )
JSTaggedValue BuiltinsArray::IndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, IndexOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (thisHandle->IsStableJSArray(thread)) {
        return IndexOfStable(argv, thread, thisHandle);
    }
    return IndexOfSlowPath(argv, thread, thisHandle);
}

// 22.1.3.12 Array.prototype.join (separator)
JSTaggedValue BuiltinsArray::Join(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Join);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto context = thread->GetCurrentEcmaContext();
    bool noCircular = context->JoinStackPushFastPath(thisHandle);
    if (!noCircular) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    if (thisHandle->IsStableJSArray(thread)) {
        return JSStableArray::Join(JSHandle<JSArray>::Cast(thisHandle), argv);
    }

    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    if (len > UINT32_MAX) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Invalid array length", JSTaggedValue::Exception());
    }
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);

    // 5. If separator is undefined, let separator be the single-element String ",".
    // 6. Let sep be ToString(separator).
    JSHandle<JSTaggedValue> sepHandle;
    if ((GetCallArg(argv, 0)->IsUndefined())) {
        sepHandle = thread->GlobalConstants()->GetHandledCommaString();
    } else {
        sepHandle = GetCallArg(argv, 0);
    }

    JSHandle<EcmaString> sepStringHandle = JSTaggedValue::ToString(thread, sepHandle);
    uint32_t allocateLength = 0;
    uint32_t sepLength = EcmaStringAccessor(sepStringHandle).GetLength();

    if (len > 0) {
        allocateLength = sepLength * (len - 1) + len;
    }
    if (allocateLength > EcmaString::MAX_STRING_LENGTH) {
        context->JoinStackPopFastPath(thisHandle);
        THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
    }
    // 7. ReturnIfAbrupt(sep).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
    std::u16string sepStr = EcmaStringAccessor(sepStringHandle).ToU16String();

    // 8. If len is zero, return the empty String.
    if (len == 0) {
        context->JoinStackPopFastPath(thisHandle);
        return GetTaggedString(thread, "");
    }

    // 9. Let element0 be Get(O, "0").
    // 10. If element0 is undefined or null, let R be the empty String; otherwise, let R be ToString(element0).
    // 11. ReturnIfAbrupt(R).
    // 12. Let k be 1.
    // 13. Repeat, while k < len
    //   a. Let S be the String value produced by concatenating R and sep.
    //   b. Let element be Get(O, ToString(k)).
    //   c. If element is undefined or null, let next be the empty String; otherwise, let next be ToString(element).
    //   d. ReturnIfAbrupt(next).
    //   e. Let R be a String value produced by concatenating S and next.
    //   f. Increase k by 1.
    std::u16string concatStr;
    for (int64_t k = 0; k < len; k++) {
        std::u16string nextStr;
        JSHandle<JSTaggedValue> element = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
        RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
        if (!element->IsUndefined() && !element->IsNull()) {
            JSHandle<EcmaString> nextStringHandle = JSTaggedValue::ToString(thread, element);
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
            nextStr = EcmaStringAccessor(nextStringHandle).ToU16String();
        }
        if (k > 0) {
            concatStr.append(sepStr);
        }
        concatStr.append(nextStr);
        if (concatStr.size() > EcmaString::MAX_STRING_LENGTH) {
            context->JoinStackPopFastPath(thisHandle);
            THROW_RANGE_ERROR_AND_RETURN(thread, "Invalid string length", JSTaggedValue::Exception());
        }
        thread->CheckSafepointIfSuspended();
    }

    // 14. Return R.
    const char16_t *constChar16tData = concatStr.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = concatStr.size();
    context->JoinStackPopFastPath(thisHandle);
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
}

// 22.1.3.13 Array.prototype.keys ( )
JSTaggedValue BuiltinsArray::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be ToObject(this value).
    // 2. ReturnIfAbrupt(O).
    JSHandle<JSObject> self = JSTaggedValue::ToObject(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayIterator(O, "key").
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(self, IterationKind::KEY));
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsArray::LastIndexOfStable(
    EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle)
{
    int64_t length = JSHandle<JSArray>::Cast(thisHandle)->GetArrayLength();
    if (length == 0) {
        return JSTaggedValue(-1);
    }
    int64_t fromIndex = length - 1;
    uint32_t argc = argv->GetArgsNumber();
    // 2: [target, fromIndex]. Note that fromIndex is missing in most usage cases.
    if (UNLIKELY(argc >= 2)) {
        JSHandle<JSTaggedValue> fromIndexHandle = argv->GetCallArg(1);
        fromIndex = ArrayHelper::GetLastStartIndex(thread, fromIndexHandle, length);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // Slow path when fromIndex is obtained from an ECMAObject
        // due to potential side effects in its 'toString' and 'valueOf' methods which modify the array object.
        if (UNLIKELY(fromIndexHandle->IsECMAObject())) {
            return LastIndexOfSlowPath(argv, thread, thisHandle, fromIndex);
        }
    }
    if (fromIndex < 0) {
        return JSTaggedValue(-1);
    }
    JSHandle<JSTaggedValue> target = GetCallArg(argv, 0);
    return JSStableArray::LastIndexOf(
        thread, thisHandle, target, static_cast<uint32_t>(fromIndex), static_cast<uint32_t>(length));
}

JSTaggedValue BuiltinsArray::LastIndexOfSlowPath(
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
    int64_t fromIndex = ArrayHelper::GetLastStartIndexFromArgs(thread, argv, 1, length);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return LastIndexOfSlowPath(argv, thread, thisObjVal, fromIndex);
}

JSTaggedValue BuiltinsArray::LastIndexOfSlowPath(
    EcmaRuntimeCallInfo *argv, JSThread *thread, const JSHandle<JSTaggedValue> &thisObjVal, int64_t fromIndex)
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

// 22.1.3.14 Array.prototype.lastIndexOf ( searchElement [ , fromIndex ] )
JSTaggedValue BuiltinsArray::LastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, IndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (thisHandle->IsStableJSArray(thread)) {
        return LastIndexOfStable(argv, thread, thisHandle);
    }
    return LastIndexOfSlowPath(argv, thread, thisHandle);
}

// 22.1.3.15 Array.prototype.map ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsArray::Map(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Map);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t rawLen = ArrayHelper::GetArrayLength(thread, thisObjVal);
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
        JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(static_cast<double>(rawLen)));
    // 8. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!newArray.IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Failed to create Object.", JSTaggedValue::Exception());
    }
    JSHandle<JSObject> newArrayHandle(thread, newArray);

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
    if (thisObjVal->IsStableJSArray(thread)) {
        JSStableArray::Map(newArrayHandle, thisObjHandle, argv, k, len);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return MapUnStableJSArray(thread, thisArgHandle, thisObjVal, k, len, newArrayHandle, callbackFnHandle);
}

JSTaggedValue BuiltinsArray::MapUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisArgHandle,
    JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, JSHandle<JSObject> newArrayHandle,
    JSHandle<JSTaggedValue> &callbackFnHandle)
{
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> mapResultHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            key.Update(JSTaggedValue(k));
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            JSTaggedValue mapResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            mapResultHandle.Update(mapResult);
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, k, mapResultHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }

    // 11. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.16 Array.prototype.pop ( )
JSTaggedValue BuiltinsArray::Pop(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Pop);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    if (thisHandle->IsStableJSArray(thread) && JSObject::IsArrayLengthWritable(thread, thisObjHandle)) {
        return JSStableArray::Pop(JSHandle<JSArray>::Cast(thisHandle), argv);
    }

    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If len is zero,
    //   a. Let setStatus be Set(O, "length", 0, true).
    //   b. ReturnIfAbrupt(setStatus).
    //   c. Return undefined.
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    if (len == 0) {
        JSHandle<JSTaggedValue> lengthValue(thread, JSTaggedValue(0));
        JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, lengthValue, true);
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
    JSHandle<JSTaggedValue> indexHandle(thread, JSTaggedValue(newLen));
    JSHandle<JSTaggedValue> element = JSTaggedValue::GetProperty(thread, thisObjVal, indexHandle).GetValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, indexHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, indexHandle, true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return element.GetTaggedValue();
}

// 22.1.3.17 Array.prototype.push ( ...items )
JSTaggedValue BuiltinsArray::Push(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Push);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 1. Let O be ToObject(this value).
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let argCount be the number of elements in items.
    uint32_t argc = argv->GetArgsNumber();
    if (argc > 0) {
        thread->NotifyArrayPrototypeChangedGuardians(thisObjHandle);
    }
    if (thisHandle->IsStableJSArray(thread) && JSObject::IsArrayLengthWritable(thread, thisObjHandle)) {
        return JSStableArray::Push(JSHandle<JSArray>::Cast(thisHandle), argv);
    }
    // 3. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 4. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 5. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. If len + argCount > 253-1, throw a TypeError exception.
    if ((len + static_cast<int64_t>(argc)) > base::MAX_SAFE_INTEGER) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }

    // 7. Repeat, while items is not empty
    //   a. Remove the first element from items and let E be the value of the element.
    //   b. Let setStatus be Set(O, ToString(len), E, true).
    //   c. ReturnIfAbrupt(setStatus).
    //   d. Let len be len+1.
    uint32_t k = 0;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    while (k < argc) {
        key.Update(JSTaggedValue(len));
        JSHandle<JSTaggedValue> kValue = GetCallArg(argv, k);
        JSArray::FastSetPropertyByValue(thread, thisObjVal, key, kValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
        len++;
    }

    // 8. Let setStatus be Set(O, "length", len, true).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    key.Update(JSTaggedValue(len));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, key, true);
    // 9. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 10. Return len.
    return GetTaggedDouble(len);
}

JSTaggedValue BuiltinsArray::ReduceUnStableJSArray(JSThread *thread, JSHandle<JSTaggedValue> &thisHandle,
    JSHandle<JSTaggedValue> &thisObjVal, int64_t k, int64_t len, JSMutableHandle<JSTaggedValue> &accumulator,
    JSHandle<JSTaggedValue> &callbackFnHandle)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSTaggedValue callResult = JSTaggedValue::Undefined();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    while (k < len) {
        bool exists = (thisHandle->IsTypedArray() || JSTaggedValue::HasProperty(thread, thisObjVal, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            key.Update(JSTaggedValue(k));
            JSHandle<JSTaggedValue> thisArgHandle = globalConst->GetHandledUndefined();
            const uint32_t argsLength = 4; // 4: «accumulator, kValue, k, O»
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(accumulator.GetTaggedValue(), kValue.GetTaggedValue(), key.GetTaggedValue(),
                thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            accumulator.Update(callResult);
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }
    return accumulator.GetTaggedValue();
}

// 22.1.3.18 Array.prototype.reduce ( callbackfn [ , initialValue ] )
JSTaggedValue BuiltinsArray::Reduce(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Reduce);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ReduceInner(argv, len);
}

JSTaggedValue BuiltinsArray::ReduceInner(EcmaRuntimeCallInfo *argv, int64_t len)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    uint32_t argc = argv->GetArgsNumber();
    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If len is 0 and initialValue is not present, throw a TypeError exception.
    if (len == 0 && argc < 2) {  // 2:2 means the number of parameters
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }

    // 7. Let k be 0.
    // 8. If initialValue is present, then
    //   a. Set accumulator to initialValue.
    // 9. Else initialValue is not present,
    //   a. Let kPresent be false.
    //   b. Repeat, while kPresent is false and k < len
    //     i. Let Pk be ToString(k).
    //     ii. Let kPresent be HasProperty(O, Pk).
    //     iii. ReturnIfAbrupt(kPresent).
    //     iv. If kPresent is true, then
    //       1. Let accumulator be Get(O, Pk).
    //       2. ReturnIfAbrupt(accumulator).
    //     v. Increase k by 1.
    //   c. If kPresent is false, throw a TypeError exception.
    int64_t k = 0;
    JSMutableHandle<JSTaggedValue> accumulator(thread, JSTaggedValue::Undefined());
    if (argc >= 2) {  // 2:2 means the number of parameters
        accumulator.Update(GetCallArg(argv, 1).GetTaggedValue());
    } else {
        bool kPresent = false;
        while (!kPresent && k < len) {
            kPresent = (thisHandle->IsTypedArray() || thisHandle->IsSharedTypedArray() ||
                JSTaggedValue::HasProperty(thread, thisObjVal, k));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (kPresent) {
                accumulator.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k++;
        }
        if (!kPresent) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "accumulator can't be initialized.", JSTaggedValue::Exception());
        }
    }

    if (thisObjVal->IsStableJSArray(thread)) {
        JSStableArray::Reduce(thread, thisObjHandle, callbackFnHandle, accumulator, k, len);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    return ReduceUnStableJSArray(thread, thisHandle, thisObjVal, k, len, accumulator, callbackFnHandle);
}

// 22.1.3.19 Array.prototype.reduceRight ( callbackfn [ , initialValue ] )
JSTaggedValue BuiltinsArray::ReduceRight(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, ReduceRight);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ReduceRightInner(argv, len);
}

JSTaggedValue BuiltinsArray::ReduceRightInner(EcmaRuntimeCallInfo *argv, int64_t len)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    uint32_t argc = argv->GetArgsNumber();

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 5. If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }

    // 6. If len is 0 and initialValue is not present, throw a TypeError exception.
    if (len == 0 && argc < 2) {  // 2:2 means the number of parameters
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }

    // 7. Let k be len-1.
    int64_t k = len - 1;
    // 8. If initialValue is present, then
    //   a. Set accumulator to initialValue.
    // 9. Else initialValue is not present,
    //   a. Let kPresent be false.
    //   b. Repeat, while kPresent is false and k ≥ 0
    //     i. Let Pk be ToString(k).
    //     ii. Let kPresent be HasProperty(O, Pk).
    //     iii. ReturnIfAbrupt(kPresent).
    //     iv. If kPresent is true, then
    //       1. Let accumulator be Get(O, Pk).
    //       2. ReturnIfAbrupt(accumulator).
    //     v. Decrease k by 1.
    //   c. If kPresent is false, throw a TypeError exception.
    JSMutableHandle<JSTaggedValue> accumulator(thread, JSTaggedValue::Undefined());
    if (argc >= 2) {  // 2:2 means the number of parameters
        accumulator.Update(GetCallArg(argv, 1).GetTaggedValue());
    } else {
        bool kPresent = false;
        while (!kPresent && k >= 0) {
            kPresent = (thisHandle->IsTypedArray() || JSTaggedValue::HasProperty(thread, thisObjVal, k));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (kPresent) {
                accumulator.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, k).GetTaggedValue());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k--;
        }
        if (!kPresent) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "accumulator can't be initialized.", JSTaggedValue::Exception());
        }
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
    JSTaggedValue callResult = JSTaggedValue::Undefined();

    JSHandle<JSTaggedValue> thisArgHandle = globalConst->GetHandledUndefined();
    if (thisObjVal->IsStableJSArray(thread)) {
        JSTaggedValue ret = JSStableArray::HandleReduceRightOfStable(thread, thisObjHandle,
            callbackFnHandle, accumulator, thisArgHandle, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (ret.ToBoolean()) {
            return accumulator.GetTaggedValue();
        }
    }

    while (k >= 0) {
        key.Update(JSTaggedValue(k));
        bool exists = (thisHandle->IsTypedArray() || JSTaggedValue::HasProperty(thread, thisObjVal, key));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, key);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            const uint32_t argsLength = 4; // 4: «accumulator, kValue, k, O»
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(accumulator.GetTaggedValue(), kValue.GetTaggedValue(), key.GetTaggedValue(),
                thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            accumulator.Update(callResult);
        }
        k--;
    }

    // 11. Return accumulator.
    return accumulator.GetTaggedValue();
}

// 22.1.3.20 Array.prototype.reverse ( )
JSTaggedValue BuiltinsArray::Reverse(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Reverse);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = 0;
    if (thisHandle->IsJSArray()) {
        len = JSArray::Cast(thisHandle->GetTaggedObject())->GetArrayLength();
    } else {
        JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
        JSHandle<JSTaggedValue> lenResult = JSTaggedValue::GetProperty(thread, thisHandle, lengthKey).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        JSTaggedNumber lenNumber = JSTaggedValue::ToLength(thread, lenResult);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        len = lenNumber.GetNumber();
    }
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. Let middle be floor(len/2).
    int64_t middle = std::floor(len / 2);

    // 6. Let lower be 0.
    int64_t lower = 0;

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
    JSMutableHandle<JSTaggedValue> lowerP(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> upperP(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> lowerValueHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> upperValueHandle(thread, JSTaggedValue::Undefined());

    if (thisObjVal->IsStableJSArray(thread)) {
        JSStableArray::Reverse(thread, thisObjHandle, lower, len);
    }
    while (lower != middle) {
        int64_t upper = len - lower - 1;
        lowerP.Update(JSTaggedValue(lower));
        upperP.Update(JSTaggedValue(upper));
        bool lowerExists = (JSTaggedValue::HasProperty(thread, thisObjVal, lowerP));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (lowerExists) {
            lowerValueHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, lowerP);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        bool upperExists = (JSTaggedValue::HasProperty(thread, thisObjVal, upperP));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (upperExists) {
            upperValueHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, upperP);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        if (lowerExists && upperExists) {
            JSArray::FastSetPropertyByValue(thread, thisObjVal, lowerP, upperValueHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSArray::FastSetPropertyByValue(thread, thisObjVal, upperP, lowerValueHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else if (upperExists) {
            JSArray::FastSetPropertyByValue(thread, thisObjVal, lowerP, upperValueHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, upperP);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else if (lowerExists) {
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, lowerP);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSArray::FastSetPropertyByValue(thread, thisObjVal, upperP, lowerValueHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
        }
        lower++;
    }

    // 8. Return O .
    return thisObjHandle.GetTaggedValue();
}

// 22.1.3.21 Array.prototype.shift ( )
JSTaggedValue BuiltinsArray::Shift(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Shift);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (thisHandle->IsStableJSArray(thread) && JSObject::IsArrayLengthWritable(thread, thisObjHandle)) {
        return JSStableArray::Shift(JSHandle<JSArray>::Cast(thisHandle), argv);
    }
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. If len is zero, then
    //   a. Let setStatus be Set(O, "length", 0, true).
    //   b. ReturnIfAbrupt(setStatus).
    //   c. Return undefined.
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    if (len == 0) {
        JSHandle<JSTaggedValue> zeroLenHandle(thread, JSTaggedValue(len));
        JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, zeroLenHandle, true);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return JSTaggedValue::Undefined();
    }

    // 6. Let first be Get(O, "0").
    JSHandle<JSTaggedValue> firstKey(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> firstValue = JSTaggedValue::GetProperty(thread, thisObjVal, firstKey).GetValue();
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
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    int64_t k = 1;
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSArray::FastSetPropertyByValue(thread, thisObjVal, k - 1, fromValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            toKey.Update(JSTaggedValue(k - 1));
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, toKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }
    // 10. Let deleteStatus be DeletePropertyOrThrow(O, ToString(len–1)).
    JSHandle<JSTaggedValue> deleteKey(thread, JSTaggedValue(len - 1));
    JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, deleteKey);
    // 11. ReturnIfAbrupt(deleteStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 12. Let setStatus be Set(O, "length", len–1, true).
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(len - 1));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    // 13. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 14. Return first.
    return firstValue.GetTaggedValue();
}

// 22.1.3.22 Array.prototype.slice (start, end)
JSTaggedValue BuiltinsArray::Slice(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Array, Slice);
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> msg0 = GetCallArg(argv, 0);
    double argStart;
    if (msg0->IsInt()) {
        argStart = msg0->GetInt();
    } else {
        // 5. Let relativeStart be ToInteger(start).
        JSTaggedNumber argStartTemp = JSTaggedValue::ToInteger(thread, msg0);
        // 6. ReturnIfAbrupt(relativeStart).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        argStart = argStartTemp.GetNumber();
    }

    // 7. If relativeStart < 0, let k be max((len + relativeStart),0); else let k be min(relativeStart, len).
    int64_t k = 0;
    if (argStart < 0) {
        double tempStart = len + argStart;
        k = tempStart > 0 ? tempStart : 0;
    } else {
        k = argStart < len ? argStart : len;
    }

    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    // 9. ReturnIfAbrupt(relativeEnd).
    // 10. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    JSHandle<JSTaggedValue> msg1 = GetCallArg(argv, 1);
    double argEnd = len;
    if (!msg1->IsUndefined()) {
        if (msg1->IsInt()) {
            argEnd = msg1->GetInt();
        } else {
            JSTaggedNumber argEndTemp = JSTaggedValue::ToInteger(thread, msg1);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            argEnd = argEndTemp.GetNumber();
        }
    }
    int64_t final = 0;
    if (argEnd < 0) {
        double tempFinal = len + argEnd;
        final = tempFinal > 0 ? tempFinal : 0;
    } else {
        final = argEnd < len ? argEnd : len;
    }
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 11. Let count be max(final – k, 0).
    int64_t count = final > k ? (final - k) : 0;

    if (thisHandle->IsStableJSArray(thread) && !thisObjHandle->GetJSHClass()->HasConstructor()
        && JSObject::GetPrototype(thisObjHandle).IsJSArray()) {
        return JSStableArray::Slice(thread, thisObjHandle, k, count);
    }

    // 12. Let A be ArraySpeciesCreate(O, count).
    JSTaggedValue newArray =
        JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(static_cast<double>(count)));
    // 13. ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (count == 0) {
        return newArray;
    }
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    // 14. Let n be 0.
    // 15. Repeat, while k < final
    //   a. Let Pk be ToString(k).
    //   b. Let kPresent be HasProperty(O, Pk).
    //   c. ReturnIfAbrupt(kPresent).
    //   d. If kPresent is true, then
    //     i. Let kValue be Get(O, Pk).
    //     ii. ReturnIfAbrupt(kValue).
    //     iii. Let status be CreateDataPropertyOrThrow(A, ToString(n), kValue ).
    //     iv. ReturnIfAbrupt(status).
    //   e. Increase k by 1.
    //   f. Increase n by 1.
    int64_t n = 0;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> nKey(thread, JSTaggedValue::Undefined());
    while (k < final) {
        key.Update(JSTaggedValue(k));
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, key);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            nKey.Update(JSTaggedValue(n));
            JSHandle<JSTaggedValue> kValueHandle = JSArray::FastGetPropertyByValue(thread, thisObjVal, key);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, nKey, kValueHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        k++;
        n++;
    }

    // 16. Let setStatus be Set(A, "length", n, true).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(n));
    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, newLenHandle, true);
    // 17. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 18. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.23 Array.prototype.some ( callbackfn [ , thisArg ] )
JSTaggedValue BuiltinsArray::Some(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Some);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
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
    //     iii. Let testResult be ToBoolean(Call(callbackfn, T, «kValue, k, and O»)).
    //     iv. ReturnIfAbrupt(testResult).
    //     v. If testResult is true, return true.
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    uint32_t k = 0;
    JSTaggedValue callResult = GetTaggedBoolean(false);
    if (thisObjVal->IsStableJSArray(thread)) {
        callResult = JSStableArray::HandleSomeOfStable(thread, thisObjHandle, callbackFnHandle, thisArgHandle, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return GetTaggedBoolean(true);
        }
    }
    while (k < len) {
        bool exists = (thisHandle->IsTypedArray() || thisHandle->IsSharedTypedArray() ||
            JSTaggedValue::HasProperty(thread, thisObjVal, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            key.Update(JSTaggedValue(k));
            kValue.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, key));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            const uint32_t argsLength = 3; // 3: «kValue, k, O»
            JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
            callResult = JSFunction::Call(info);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (callResult.ToBoolean()) {
                return GetTaggedBoolean(true);
            }
        }
        k++;
        thread->CheckSafepointIfSuspended();
    }

    // 9. Return false.
    return GetTaggedBoolean(false);
}

// 22.1.3.24 Array.prototype.sort (comparefn)
JSTaggedValue BuiltinsArray::Sort(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, Sort);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If comparefn is not undefined and IsCallable(comparefn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }

    // 2. Let obj be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // Array sort
#if ENABLE_NEXT_OPTIMIZATION
    if (thisHandle->IsStableJSArray(thread)) {
        JSStableArray::Sort(thread, thisHandle, callbackFnHandle);
    } else {
        JSArray::Sort(thread, JSHandle<JSTaggedValue>::Cast(thisObjHandle), callbackFnHandle);
    }
#else
    if (thisHandle->IsStableJSArray(thread) && callbackFnHandle->IsUndefined()) {
        JSArray::SortElementsByObject(thread, thisObjHandle, callbackFnHandle);
    } else {
        JSArray::Sort(thread, JSHandle<JSTaggedValue>::Cast(thisObjHandle), callbackFnHandle);
    }
#endif
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return thisObjHandle.GetTaggedValue();
}

// 22.1.3.25 Array.prototype.splice (start, deleteCount , ...items )
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsArray::Splice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Splice);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    uint32_t argc = argv->GetArgsNumber();
    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 5. Let relativeStart be ToInteger(start).
    int64_t start = 0;
    int64_t insertCount = 0;
    int64_t actualDeleteCount = 0;
    int64_t end = len;
    double argStart = 0;
    if (argc > 0) {
        JSHandle<JSTaggedValue> msg0 = GetCallArg(argv, 0);
        JSTaggedNumber argStartTemp = JSTaggedValue::ToInteger(thread, msg0);
        // 6. ReturnIfAbrupt(relativeStart).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        argStart = argStartTemp.GetNumber();
        // 7. If relativeStart < 0, let actualStart be max((len + relativeStart),0); else let actualStart be
        // min(relativeStart, len).
        if (argStart < 0) {
            double tempStart = argStart + len;
            start = tempStart > 0 ? tempStart : 0;
        } else {
            start = argStart < end ? argStart : end;
        }
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
        thread->NotifyArrayPrototypeChangedGuardians(thisObjHandle);
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
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle,
                                                         JSTaggedNumber(static_cast<double>(actualDeleteCount)));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    if (thisHandle->IsStableJSArray(thread) && JSObject::IsArrayLengthWritable(thread, thisObjHandle)) {
        return JSStableArray::Splice(JSHandle<JSArray>::Cast(thisHandle), argv, start, insertCount,
            actualDeleteCount, newArrayHandle, len);
    }
    // 14. Let k be 0.
    // 15. Repeat, while k < actualDeleteCount
    //   a. Let from be ToString(actualStart+k).
    //   b. Let fromPresent be HasProperty(O, from).
    //   d. If fromPresent is true, then
    //     i. Let fromValue be Get(O, from).
    //     iii. Let status be CreateDataPropertyOrThrow(A, ToString(k), fromValue).
    //   e. Increase k by 1.
    JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    int64_t k = 0;
    while (k < actualDeleteCount) {
        int64_t from = start + k;
        fromKey.Update(JSTaggedValue(from));
        bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (exists) {
            JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            toKey.Update(JSTaggedValue(k));
            if (newArrayHandle->IsJSProxy()) {
                toKey.Update(JSTaggedValue::ToString(thread, toKey).GetTaggedValue());
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, fromValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        k++;
    }
    // 16. Let setStatus be Set(A, "length", actualDeleteCount, true).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> deleteCountHandle(thread, JSTaggedValue(actualDeleteCount));
    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(newArrayHandle), lengthKey, deleteCountHandle,
                               true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 19. Let itemCount be the number of elements in items.
    // 20. If itemCount < actualDeleteCount, then
    //   a. Let k be actualStart.
    //   b. Repeat, while k < (len – actualDeleteCount)
    //     i. Let from be ToString(k+actualDeleteCount).
    //     ii. Let to be ToString(k+itemCount).
    //     iii. Let fromPresent be HasProperty(O, from).
    //     v. If fromPresent is true, then
    //       1. Let fromValue be Get(O, from).
    //       3. Let setStatus be Set(O, to, fromValue, true).
    //     vi. Else fromPresent is false,
    //       1. Let deleteStatus be DeletePropertyOrThrow(O, to).
    //     vii. Increase k by 1.
    //   c. Let k be len.
    //   d. Repeat, while k > (len – actualDeleteCount + itemCount)
    //     i. Let deleteStatus be DeletePropertyOrThrow(O, ToString(k–1)).
    //     iii. Decrease k by 1.
    if (insertCount < actualDeleteCount) {
        k = start;
        while (k < len - actualDeleteCount) {
            fromKey.Update(JSTaggedValue(k + actualDeleteCount));
            toKey.Update(JSTaggedValue(k + insertCount));
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSArray::FastSetPropertyByValue(thread, thisObjVal, toKey, fromValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            } else {
                JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, toKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k++;
        }
        k = len;
        JSMutableHandle<JSTaggedValue> deleteKey(thread, JSTaggedValue::Undefined());
        while (k > len - actualDeleteCount + insertCount) {
            deleteKey.Update(JSTaggedValue(k - 1));
            JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, deleteKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            k--;
        }
    } else if (insertCount > actualDeleteCount) {
        // 21. Else if itemCount > actualDeleteCount, then
        //   a. Let k be (len – actualDeleteCount).
        //   b. Repeat, while k > actualStart
        //     i. Let from be ToString(k + actualDeleteCount – 1).
        //     ii. Let to be ToString(k + itemCount – 1)
        //     iii. Let fromPresent be HasProperty(O, from).
        //     iv. ReturnIfAbrupt(fromPresent).
        //     v. If fromPresent is true, then
        //       1. Let fromValue be Get(O, from).
        //       2. ReturnIfAbrupt(fromValue).
        //       3. Let setStatus be Set(O, to, fromValue, true).
        //       4. ReturnIfAbrupt(setStatus).
        //     vi. Else fromPresent is false,
        //       1. Let deleteStatus be DeletePropertyOrThrow(O, to).
        //       2. ReturnIfAbrupt(deleteStatus).
        //     vii. Decrease k by 1.
        k = len - actualDeleteCount;
        while (k > start) {
            fromKey.Update(JSTaggedValue(k + actualDeleteCount - 1));
            toKey.Update(JSTaggedValue(k + insertCount - 1));
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSArray::FastSetPropertyByValue(thread, thisObjVal, toKey, fromValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            } else {
                JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, toKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k--;
        }
    }
    // 22. Let k be actualStart.
    k = start;
    // 23. Repeat, while items is not empty
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 2; i < argc; i++) {
        JSHandle<JSTaggedValue> itemValue = GetCallArg(argv, i);
        key.Update(JSTaggedValue(k));
        JSArray::FastSetPropertyByValue(thread, thisObjVal, key, itemValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
    }
    // 24. Let setStatus be Set(O, "length", len – actualDeleteCount + itemCount, true).
    int64_t newLen = len - actualDeleteCount + insertCount;
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newLen));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    // 25. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 26. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 22.1.3.26 Array.prototype.toLocaleString ( [ reserved1 [ , reserved2 ] ] )
JSTaggedValue BuiltinsArray::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, ToLocaleString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // add this to join stack to avoid circular call
    auto context = thread->GetCurrentEcmaContext();
    bool noCircular = context->JoinStackPushFastPath(thisHandle);
    if (!noCircular) {
        return factory->GetEmptyString().GetTaggedValue();
    }
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);

    // 6. If len is zero, return the empty String.
    if (len == 0) {
        // pop this from join stack
        context->JoinStackPopFastPath(thisHandle);
        return GetTaggedString(thread, "");
    }

    // Inject locales and options argument into a taggedArray
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);

    CString concatStr;
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
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
    for (int64_t k = 0; k < len; k++) {
        thread->CheckSafepointIfSuspended();
        JSTaggedValue next = globalConst->GetEmptyString();
        JSHandle<JSTaggedValue> nextElement = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
        RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
        if (!nextElement->IsUndefined() && !nextElement->IsNull()) {
            JSHandle<JSTaggedValue> nextValueHandle = nextElement;
            JSHandle<JSTaggedValue> key = globalConst->GetHandledToLocaleStringString();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, nextValueHandle, undefined, 2); // 2: two args
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
            info->SetCallArg(locales.GetTaggedValue(), options.GetTaggedValue());
            JSTaggedValue callResult = JSFunction::Invoke(info, key);
            RETURN_EXCEPTION_AND_POP_JOINSTACK(thread, thisHandle);
            next = callResult;
        }
        JSHandle<JSTaggedValue> nextHandle(thread, next);
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

// 22.1.3.27 Array.prototype.toString ( )
JSTaggedValue BuiltinsArray::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto ecmaVm = thread->GetEcmaVM();

    // 1. Let array be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
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
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisObjVal, undefined, 0);
    return JSFunction::Call(info);
}

// 22.1.3.28 Array.prototype.unshift ( ...items )
JSTaggedValue BuiltinsArray::Unshift(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Unshift);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 5. Let argCount be the number of actual arguments.
    int64_t argc = argv->GetArgsNumber();
    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. If argCount > 0, then
    //   a. If len+ argCount > 253-1, throw a TypeError exception.
    //   b. Let k be len.
    //   c. Repeat, while k > 0,
    //     i. Let from be ToString(k–1).
    //     ii. Let to be ToString(k+argCount –1).
    //     iii. Let fromPresent be HasProperty(O, from).
    //     iv. ReturnIfAbrupt(fromPresent).
    //     v. If fromPresent is true, then
    //       1. Let fromValue be Get(O, from).
    //       2. ReturnIfAbrupt(fromValue).
    //       3. Let setStatus be Set(O, to, fromValue, true).
    //       4. ReturnIfAbrupt(setStatus).
    //     vi. Else fromPresent is false,
    //       1. Let deleteStatus be DeletePropertyOrThrow(O, to).
    //       2. ReturnIfAbrupt(deleteStatus).
    //     vii. Decrease k by 1.
    if (argc > 0) {
        if (len + argc > base::MAX_SAFE_INTEGER) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
        }
        thread->NotifyArrayPrototypeChangedGuardians(thisObjHandle);
        JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
        int64_t k = len;
        while (k > 0) {
            fromKey.Update(JSTaggedValue(k - 1));
            toKey.Update(JSTaggedValue(k + argc - 1));
            bool exists = JSTaggedValue::HasProperty(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (exists) {
                JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                JSArray::FastSetPropertyByValue(thread, thisObjVal, toKey, fromValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            } else {
                JSTaggedValue::DeletePropertyOrThrow(thread, thisObjVal, toKey);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            }
            k--;
        }
        //   d. Let j be 0.
        //   e. Let items be a List whose elements are, in left to right order, the arguments that were passed to this
        //   function invocation.
        //   f. Repeat, while items is not empty
        //     i. Remove the first element from items and let E be the value of that element.
        //     ii. Let setStatus be Set(O, ToString(j), E, true).
        //     iii. ReturnIfAbrupt(setStatus).
        //     iv. Increase j by 1.
        int64_t j = 0;
        while (j < argc) {
            toKey.Update(JSTaggedValue(j));
            JSHandle<JSTaggedValue> toValue = GetCallArg(argv, j);
            JSArray::FastSetPropertyByValue(thread, thisObjVal, toKey, toValue);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            j++;
        }
    }

    // 7. Let setStatus be Set(O, "length", len+argCount, true).
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    int64_t newLen = len + argc;
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newLen));
    JSTaggedValue::SetProperty(thread, thisObjVal, lengthKey, newLenHandle, true);
    // 8. ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 9. Return len+argCount.
    return GetTaggedDouble(newLen);
}

// 22.1.3.29 Array.prototype.values ( )
JSTaggedValue BuiltinsArray::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let O be ToObject(this value).
    // 2. ReturnIfAbrupt(O).
    JSHandle<JSObject> self = JSTaggedValue::ToObject(thread, GetThis(argv));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Return CreateArrayIterator(O, "value").
    JSHandle<JSArrayIterator> iter(factory->NewJSArrayIterator(self, IterationKind::VALUE));
    return iter.GetTaggedValue();
}

// es12 23.1.3.10
JSTaggedValue BuiltinsArray::Flat(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Flat);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ? ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    uint32_t argc = argv->GetArgsNumber();
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 2. Let sourceLen be ? LengthOfArrayLike(O).
    int64_t sourceLen = ArrayHelper::GetLength(thread, thisObjVal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let depthNum be 1.
    double depthNum = 1;

    // 4. If depth is not undefined, then
    // a. Set depthNum to ? ToIntegerOrInfinity(depth).
    // b. If depthNum < 0, set depthNum to 0.
    if (argc > 0) {
        JSHandle<JSTaggedValue> msg1 = GetCallArg(argv, 0);
        if (!msg1->IsUndefined()) {
            JSTaggedNumber fromIndexTemp = JSTaggedValue::ToNumber(thread, msg1);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            depthNum = base::NumberHelper::TruncateDouble(fromIndexTemp.GetNumber());
            depthNum = depthNum < 0 ? 0 : depthNum;
        }
    }
    // 5. Let A be ? ArraySpeciesCreate(O, 0).
    uint32_t arrayLen = 0;
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(arrayLen));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    base::FlattenArgs args = { sourceLen, 0, depthNum };
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    // 6. Perform ? FlattenIntoArray(A, O, sourceLen, 0, depthNum).
    ArrayHelper::FlattenIntoArray(thread, newArrayHandle, thisObjVal, args,
                                  thread->GlobalConstants()->GetHandledUndefined(),
                                  thread->GlobalConstants()->GetHandledUndefined());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 7. Return A.
    return newArrayHandle.GetTaggedValue();
}

// es12 23.1.3.11
JSTaggedValue BuiltinsArray::FlatMap(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, FlatMap);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ? ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 2. Let sourceLen be ? LengthOfArrayLike(O).
    int64_t sourceLen = ArrayHelper::GetLength(thread, thisObjVal);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If ! IsCallable(mapperFunction) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> mapperFunctionHandle = GetCallArg(argv, 0);
    if (!mapperFunctionHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the mapperFunction is not callable.", JSTaggedValue::Exception());
    }
    // 4. Let A be ? ArraySpeciesCreate(O, 0).
    uint32_t arrayLen = 0;
    JSTaggedValue newArray = JSArray::ArraySpeciesCreate(thread, thisObjHandle, JSTaggedNumber(arrayLen));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    base::FlattenArgs args = { sourceLen, 0, 1 };
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    // 5. Perform ? FlattenIntoArray(A, O, sourceLen, 0, 1, mapperFunction, thisArg).
    ArrayHelper::FlattenIntoArray(thread, newArrayHandle, thisObjVal, args,
                                  mapperFunctionHandle, GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 23.1.3.13 Array.prototype.includes ( searchElement [ , fromIndex ] )
JSTaggedValue BuiltinsArray::Includes(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, Includes);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be ? ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
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
        JSHandle<JSTaggedValue> handledFrom(thread, JSTaggedValue(from));
        fromStr = JSTaggedValue::ToString(thread, handledFrom);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(fromStr.GetTaggedValue());
        kValueHandle.Update(JSArray::FastGetPropertyByValue(thread, thisObjVal, key).GetTaggedValue());
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
JSTaggedValue BuiltinsArray::At(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Array, At);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    if (thisHandle->IsStableJSArray(thread)) {
        return JSStableArray::At(JSHandle<JSArray>::Cast(thisHandle), argv);
    }
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 2. Let len be ? LengthOfArrayLike(O).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
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
    JSHandle<JSTaggedValue> element = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return element.GetTaggedValue();
}

// 23.1.3.39 Array.prototype.with ( index, value )
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsArray::With(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, With);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 2. Let len be ? LengthOfArrayLike(O).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. Let relativeIndex be ? ToIntegerOrInfinity(relativeIndex).
    JSTaggedNumber index = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
    // ReturnIfAbrupt(index).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t relativeIndex = index.GetNumber();
    int64_t actualIndex = 0;
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);
    // 4. If relativeIndex ≥ 0, let actualIndex be relativeIndex.
    // 5. Else, let actualIndex be len + relativeIndex.
    // 6. If actualIndex ≥ len or actualIndex < 0, throw a RangeError exception.
    if (relativeIndex >= 0) {
        actualIndex = relativeIndex;
    } else {
        actualIndex = len + relativeIndex;
    }
    if (actualIndex >= len || actualIndex < 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }
    // 7. Let A be ? ArrayCreate(len).
    JSTaggedValue newArray =
        JSArray::ArrayCreate(thread, JSTaggedNumber(static_cast<double>(len))).GetTaggedValue();
    // ReturnIfAbrupt(A).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);
    if (thisHandle->IsStableJSArray(thread) && !thisObjHandle->GetJSHClass()->HasConstructor()) {
        return JSStableArray::With(thread, JSHandle<JSArray>::Cast(thisHandle), len, actualIndex, value);
    }
    // 8. Let k be 0.
    int64_t k = 0;
    // 9. Repeat, while k < len,
    //     a. Let Pk be ! ToString(𝔽(k)).
    //     b. If k is actualIndex, let fromValue be value.
    //     c. Else, let fromValue be ? Get(O, Pk).
    //     d. Perform ! CreateDataPropertyOrThrow(A, Pk, fromValue).
    //     e. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> fromValue;
    while (k < len) {
        fromKey.Update(JSTaggedValue(k));
        if (k == actualIndex) {
            fromValue = value;
        } else {
            fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, fromKey, fromValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++k;
        thread->CheckSafepointIfSuspended();
    }
    // 10. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 23.1.3.34 Array.prototype.toSorted ( comparefn )
JSTaggedValue BuiltinsArray::ToSorted(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, ToSorted);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. If comparefn is not undefined and IsCallable(comparefn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsUndefined() && !callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Callable is false", JSTaggedValue::Exception());
    }

    // 2. Let obj be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. Let len be ToLength(Get(obj, "length")).
    int64_t len = ArrayHelper::GetArrayLength(thread, JSHandle<JSTaggedValue>(thisObjHandle));
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 4. Let A be ? ArrayCreate(len).
    JSTaggedValue newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(static_cast<double>(len))).GetTaggedValue();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    // 5. Let SortCompare be a new Abstract Closure with parameters (x, y) that captures comparefn and performs
    // the following steps when called:
    //    a. Return ? CompareArrayElements(x, y, comparefn).
    // 6. Let sortedList be ? SortIndexedProperties(O, len, SortCompare, read-through-holes).
    JSHandle<TaggedArray> sortedList =
        ArrayHelper::SortIndexedProperties(thread, JSHandle<JSTaggedValue>::Cast(thisObjHandle), len, callbackFnHandle,
                                           base::HolesType::READ_THROUGH_HOLES);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    //7. Let j be 0.
    int64_t j = 0;
    // 8. Repeat, while j < len,
    //     a. Perform ! CreateDataPropertyOrThrow(A, ! ToString(𝔽(j)), sortedList[j]).
    //     b. Set j to j + 1.
    JSMutableHandle<JSTaggedValue> itemValue(thread, JSTaggedValue::Undefined());
    while (j < len) {
        itemValue.Update(sortedList->Get(j));
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, j, itemValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++j;
    }
    // 9. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 23.1.3.35 Array.prototype.toSpliced ( start, skipCount, ...items )
JSTaggedValue BuiltinsArray::ToSpliced(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, ToSpliced);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    uint32_t argc = argv->GetArgsNumber();
    // 1. Let O be ? ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);
    // 2. Let len be ? LengthOfArrayLike(O).
    int64_t len = ArrayHelper::GetArrayLength(thread, thisObjVal);
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t actualStart = 0;
    int64_t actualSkipCount = 0;
    int64_t newLen = 0;
    int64_t insertCount = 0;
    // 3. Let relativeStart be ? ToIntegerOrInfinity(start).
    if (argc > 0) {
        JSTaggedNumber argStart = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 0));
        // ReturnIfAbrupt(relativeStart).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double relativeStart = argStart.GetNumber();
        // 4. If relativeStart = -∞, let k be 0.
        // 5. Else if relativeStart < 0, let k be max(len + relativeStart, 0).
        // 6. Else, let k be min(relativeStart, len).
        if (relativeStart < 0) {
            double tempStart = relativeStart + len;
            actualStart = tempStart > 0 ? tempStart : 0;
        } else {
            actualStart = relativeStart < len ? relativeStart : len;
        }
        actualSkipCount = len - actualStart;
    }
    // 7. Let insertCount be the number of elements in items.
    // 8. If start is not present, then
    //     a. Let actualSkipCount be 0.
    // 9. Else if skipCount is not present, then
    //     a. Let actualSkipCount be len - actualStart.
    // 10. Else,
    //     a. Let sc be ? ToIntegerOrInfinity(skipCount).
    //     b. Let actualSkipCount be the result of clamping sc between 0 and len - actualStart.
    if (argc > 1) {
        insertCount = argc - 2; // 2:2 means there two arguments before the insert items.
        JSTaggedNumber argSkipCount = JSTaggedValue::ToInteger(thread, GetCallArg(argv, 1));
        // ReturnIfAbrupt(argSkipCount).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double skipCount = argSkipCount.GetNumber();
        skipCount = skipCount > 0 ? skipCount : 0;
        actualSkipCount = skipCount < (len - actualStart) ? skipCount : len - actualStart;
    }
    // 11. Let newLen be len + insertCount - actualSkipCount.
    newLen = len + insertCount - actualSkipCount;
    // 12. If newLen > 2^53 - 1, throw a TypeError exception.
    if (newLen > base::MAX_SAFE_INTEGER) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "out of range.", JSTaggedValue::Exception());
    }
    if (thisHandle->IsStableJSArray(thread) && !thisObjHandle->GetJSHClass()->HasConstructor()) {
        return JSStableArray::ToSpliced(JSHandle<JSArray>::Cast(thisHandle), argv, argc, actualStart,
                                        actualSkipCount, newLen);
    }
    // 13. Let A be ? ArrayCreate(newLen).
    JSHandle<JSTaggedValue> newJsTaggedArray =
        JSArray::ArrayCreate(thread, JSTaggedNumber(static_cast<double>(newLen)));
    // ReturnIfAbrupt(newArray).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newJsTaggedArray.GetTaggedValue());
    // 14. Let i be 0.
    int64_t i = 0;
    // 15. Let r be actualStart + actualSkipCount.
    int64_t r = actualStart + actualSkipCount;
    // 16. Repeat, while i < actualStart,
    //     a. Let Pi be ! ToString(𝔽(i)).
    //     b. Let iValue be ? Get(O, Pi).
    //     c. Perform ! CreateDataPropertyOrThrow(A, Pi, iValue).
    //     d. Set i to i + 1.
    while (i < actualStart) {
        JSHandle<JSTaggedValue> iValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, i);
        // ReturnIfAbrupt(iValue).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, i, iValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++i;
    }
    // 17. For each element E of items, do
    //     a. Let Pi be ! ToString(𝔽(i)).
    //     b. Perform ! CreateDataPropertyOrThrow(A, Pi, E).
    //     c. Set i to i + 1.
    JSMutableHandle<JSTaggedValue> pi(thread, JSTaggedValue::Undefined());
    for (int64_t pos = 2; pos < argc; ++pos) { // 2:2 means there two arguments before the insert items.
        pi.Update(JSTaggedValue(i));
        JSHandle<JSTaggedValue> element = GetCallArg(argv, pos);
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, pi, element);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++i;
    }
    // 18. Repeat, while i < newLen,
    //     a. Let Pi be ! ToString(𝔽(i)).
    //     b. Let from be ! ToString(𝔽(r)).
    //     c. Let fromValue be ? Get(O, from).
    //     d. Perform ! CreateDataPropertyOrThrow(A, Pi, fromValue).
    //     e. Set i to i + 1.
    //     f. Set r to r + 1.
    JSMutableHandle<JSTaggedValue> from(thread, JSTaggedValue::Undefined());
    while (i < newLen) {
        pi.Update(JSTaggedValue(i));
        from.Update(JSTaggedValue(r));
        JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, from);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, pi, fromValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++i;
        ++r;
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> newLenHandle(thread, JSTaggedValue(newLen));
    JSTaggedValue::SetProperty(thread, newJsTaggedArray, lengthKey, newLenHandle, true);
    // ReturnIfAbrupt(setStatus).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 19. Return A.
    return newArrayHandle.GetTaggedValue();
}

// 23.1.3.11 Array.prototype.findLast ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsArray::FindLast(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, FindLast);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(predicate) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the predicate is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be (len - 1).
    // 8. Repeat, while k >= 0
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let testResult be ToBoolean(Call(predicate, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(testResult).
    //   f. If testResult is true, return kValue.
    //   g. Decrease k by 1.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    int64_t k = len - 1;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    JSTaggedValue callResult = GetTaggedBoolean(false);
    if (thisObjVal->IsStableJSArray(thread)) {
        JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
        callResult = JSStableArray::HandleFindLastOfStable(thread, thisObjHandle,
            callbackFnHandle, thisArgHandle, kValue, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return kValue.GetTaggedValue();
        }
    }

    while (k >= 0) {
        JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
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

// 23.1.3.12 Array.prototype.findLastIndex ( predicate [ , thisArg ] )
JSTaggedValue BuiltinsArray::FindLastIndex(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, FindLastIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // 2. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 3. Let len be ToLength(Get(O, "length")).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // 4. ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 5. If IsCallable(predicate) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the predicate is not callable.", JSTaggedValue::Exception());
    }

    // 6. If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);

    // 7. Let k be (len - 1).
    // 8. Repeat, while k >=0
    //   a. Let Pk be ToString(k).
    //   b. Let kValue be Get(O, Pk).
    //   c. ReturnIfAbrupt(kValue).
    //   d. Let testResult be ToBoolean(Call(predicate, T, «kValue, k, O»)).
    //   e. ReturnIfAbrupt(testResult).
    //   f. If testResult is true, return k.
    //   g. Decrease k by 1.
    int64_t k = len - 1;
    JSTaggedValue callResult = GetTaggedBoolean(true);
    if (thisObjVal->IsStableJSArray(thread)) {
        callResult =
            JSStableArray::HandleFindLastIndexOfStable(thread, thisObjHandle, callbackFnHandle, thisArgHandle, k);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (callResult.ToBoolean()) {
            return GetTaggedDouble(k);
        }
    }
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    const uint32_t argsLength = 3; // 3: «kValue, k, O»
    while (k >= 0) {
        JSHandle<JSTaggedValue> kValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, k);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        key.Update(JSTaggedValue(k));
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, argsLength);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        info->SetCallArg(kValue.GetTaggedValue(), key.GetTaggedValue(), thisObjVal.GetTaggedValue());
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

// 23.1.3.33 Array.prototype.toReversed ( )
JSTaggedValue BuiltinsArray::ToReversed(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Array, ToReversed);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSObject> thisObjHandle = JSTaggedValue::ToObject(thread, thisHandle);
    // ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> thisObjVal(thisObjHandle);

    // 2. Let len be ? LengthOfArrayLike(O).
    int64_t len = ArrayHelper::GetLength(thread, thisObjVal);
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (thisHandle->IsStableJSArray(thread) && !thisObjHandle->GetJSHClass()->HasConstructor()) {
        return JSStableArray::ToReversed(thread, JSHandle<JSArray>::Cast(thisHandle), len);
    }
    // 3. Let A be ? ArrayCreate(len).
    JSTaggedValue newArray = JSArray::ArrayCreate(thread, JSTaggedNumber(static_cast<double>(len))).GetTaggedValue();
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSObject> newArrayHandle(thread, newArray);

    // 4. Let k be 0.
    // 5. Repeat, while k < len,
    //     a. Let from be ! ToString(𝔽(len - k - 1)).
    //     b. Let Pk be ! ToString(𝔽(k)).
    //     c. Let fromValue be ? Get(O, from).
    //     d. Perform ! CreateDataPropertyOrThrow(A, Pk, fromValue).
    //     e. Set k to k + 1.
    JSMutableHandle<JSTaggedValue> fromKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> toKey(thread, JSTaggedValue::Undefined());
    int64_t k = 0;
    while (k < len) {
        int64_t from = len - k - 1;
        fromKey.Update(JSTaggedValue(from));
        toKey.Update(JSTaggedValue(k));
        JSHandle<JSTaggedValue> fromValue = JSArray::FastGetPropertyByValue(thread, thisObjVal, fromKey);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        JSObject::CreateDataPropertyOrThrow(thread, newArrayHandle, toKey, fromValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        k++;
        thread->CheckSafepointIfSuspended();
    }
    // 6. Return A.
    return newArrayHandle.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
