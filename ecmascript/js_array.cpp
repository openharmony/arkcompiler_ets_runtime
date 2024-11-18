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

#include "ecmascript/js_array.h"

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript {
using base::ArrayHelper;

JSTaggedValue JSArray::LengthGetter([[maybe_unused]] JSThread *thread, const JSHandle<JSObject> &self)
{
    return JSTaggedValue(JSArray::Cast(*self)->GetLength());
}

bool JSArray::LengthSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                           bool mayThrow)
{
    uint32_t newLen = 0;
    if (!JSTaggedValue::ToArrayLength(thread, value, &newLen) && mayThrow) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    }

    uint32_t oldLen = JSArray::Cast(*self)->GetArrayLength();
    if (oldLen == newLen) {
        return true;
    }

    if (!IsArrayLengthWritable(thread, self)) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetReadOnlyProperty), false);
        }
        return false;
    }

    JSArray::SetCapacity(thread, self, oldLen, newLen);
    uint32_t actualLen = JSArray::Cast(*self)->GetArrayLength();
    if (actualLen != newLen) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not all array elements is configurable", false);
        }
        return false;
    }

    return true;
}

JSHandle<JSTaggedValue> JSArray::ArrayCreate(JSThread *thread, JSTaggedNumber length, ArrayMode mode)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> arrayFunction = env->GetArrayFunction();
    return JSArray::ArrayCreate(thread, length, arrayFunction, mode);
}

// 9.4.2.2 ArrayCreate(length, proto)
JSHandle<JSTaggedValue> JSArray::ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                             const JSHandle<JSTaggedValue> &newTarget, ArrayMode mode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Assert: length is an integer Number ≥ 0.
    ASSERT_PRINT(length.IsInteger() && length.GetNumber() >= 0, "length must be positive integer");
    // 2. If length is −0, let length be +0.
    double arrayLength = length.GetNumber();
    if (arrayLength > MAX_ARRAY_INDEX) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must equal or less than 2^32.", exception);
    }
    uint32_t normalArrayLength = length.ToUint32();

    // 8. Set the [[Prototype]] internal slot of A to proto.
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayFunc(env->GetArrayFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(arrayFunc, newTarget);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // 9. Set the [[Extensible]] internal slot of A to true.
    obj->GetJSHClass()->SetExtensible(true);

    // 10. Perform OrdinaryDefineOwnProperty(A, "length", PropertyDescriptor{[[Value]]: length, [[Writable]]:
    // true, [[Enumerable]]: false, [[Configurable]]: false}).
    if (mode == ArrayMode::LITERAL) {
        JSArray::Cast(*obj)->SetArrayLength(thread, normalArrayLength);
    } else {
        JSArray::SetCapacity(thread, obj, 0, normalArrayLength, true);
    }

    // For new Array(Len), the elementsKind should be Hole
    if (thread->GetEcmaVM()->IsEnableElementsKind()) {
        if ((newTarget.GetTaggedValue() == arrayFunc.GetTaggedValue()) && normalArrayLength != 0) {
            JSHandle<JSArray> newArray(obj);
            #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
            JSHClass::TransitToElementsKind(thread, newArray, ElementsKind::GENERIC);
            #else
            JSHClass::TransitToElementsKind(thread, newArray, ElementsKind::HOLE);
            #endif
        }
    }

    return JSHandle<JSTaggedValue>(obj);
}

// 9.4.2.3 ArraySpeciesCreate(originalArray, length)
JSTaggedValue JSArray::ArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &originalArray,
                                          JSTaggedNumber length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // Assert: length is an integer Number ≥ 0.
    ASSERT_PRINT(length.IsInteger() && length.GetNumber() >= 0, "length must be positive integer");
    // If length is −0, let length be +0.
    int64_t arrayLength = length.GetNumber();
    if (arrayLength == -0) {
        arrayLength = +0;
    }
    // Let C be undefined.
    // Let isArray be IsArray(originalArray).
    JSHandle<JSTaggedValue> originalValue(originalArray);
    bool isArray = originalValue->IsArray(thread);
    // ReturnIfAbrupt(isArray).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If isArray is true, then
    JSHandle<JSTaggedValue> constructor(thread, JSTaggedValue::Undefined());
    if (isArray) {
        // Let C be Get(originalArray, "constructor").
        auto *hclass = originalArray->GetJSHClass();
        JSTaggedValue proto = hclass->GetPrototype();
        if (hclass->IsJSArray() && !hclass->HasConstructor() && proto.IsJSArray()) {
            return JSArray::ArrayCreate(thread, length).GetTaggedValue();
        }
        JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
        constructor = JSTaggedValue::GetProperty(thread, originalValue, constructorKey).GetValue();
        // ReturnIfAbrupt(C).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // If IsConstructor(C) is true, then
        if (constructor->IsConstructor()) {
            // Let thisRealm be the running execution context’s Realm.
            // Let realmC be GetFunctionRealm(C).
            JSHandle<GlobalEnv> realmC = JSObject::GetFunctionRealm(thread, constructor);
            // ReturnIfAbrupt(realmC).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // If thisRealm and realmC are not the same Realm Record, then
            if (*realmC != *env) {
                JSTaggedValue realmArrayConstructor = realmC->GetArrayFunction().GetTaggedValue();
                // If SameValue(C, realmC.[[intrinsics]].[[%Array%]]) is true, let C be undefined.
                if (JSTaggedValue::SameValue(constructor.GetTaggedValue(), realmArrayConstructor)) {
                    return JSArray::ArrayCreate(thread, length).GetTaggedValue();
                }
            }
        }

        // If Type(C) is Object, then
        if (constructor->IsECMAObject()) {
            // Let C be Get(C, @@species).
            JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
            constructor = JSTaggedValue::GetProperty(thread, constructor, speciesSymbol).GetValue();
            // ReturnIfAbrupt(C).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // If C is null, let C be undefined.
            if (constructor->IsNull()) {
                return JSArray::ArrayCreate(thread, length).GetTaggedValue();
            }
        }
    }

    // If C is undefined, return ArrayCreate(length).
    if (constructor->IsUndefined()) {
        return JSArray::ArrayCreate(thread, length).GetTaggedValue();
    }
    // If IsConstructor(C) is false, throw a TypeError exception.
    if (!constructor->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a constructor", JSTaggedValue::Exception());
    }
    // Return Construct(C, «length»).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    info->SetCallArg(JSTaggedValue(arrayLength));
    JSTaggedValue result = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // NOTEIf originalArray was created using the standard built-in Array constructor for
    // a Realm that is not the Realm of the running execution context, then a new Array is
    // created using the Realm of the running execution context. This maintains compatibility
    // with Web browsers that have historically had that behaviour for the Array.prototype methods
    // that now are defined using ArraySpeciesCreate.
    return result;
}

void JSArray::SetCapacity(JSThread *thread, const JSHandle<JSObject> &array,
                          uint32_t oldLen, uint32_t newLen, bool isNew)
{
    TaggedArray *element = TaggedArray::Cast(array->GetElements().GetTaggedObject());

    if (element->IsDictionaryMode()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        uint32_t numOfElements = array->GetNumberOfElements();
        uint32_t newNumOfElements = newLen;

        if (newLen < oldLen && numOfElements != 0U) {
            JSHandle<NumberDictionary> dictHandle(thread, element);
            JSHandle<TaggedArray> newArr = factory->NewTaggedArray(numOfElements);
            GetAllElementKeys(thread, array, 0, newArr);
            for (uint32_t i = numOfElements - 1; i >= newLen; i--) {
                JSTaggedValue value = newArr->Get(i);
                uint32_t output = 0;
                JSTaggedValue::StringToElementIndex(value, &output);
                JSTaggedValue key(static_cast<int>(output));
                int entry = dictHandle->FindEntry(key);
                auto attr = dictHandle->GetAttributes(entry).GetValue();
                PropertyAttributes propAttr(attr);
                if (propAttr.IsConfigurable()) {
                    JSHandle<NumberDictionary> newDict = NumberDictionary::Remove(thread, dictHandle, entry);
                    array->SetElements(thread, newDict);
                    if (i == 0) {
                        newNumOfElements = i;
                        break;
                    }
                } else {
                    newNumOfElements = i + 1;
                    break;
                }
            }
        }
        JSArray::Cast(*array)->SetArrayLength(thread, newNumOfElements);
        return;
    }

    uint32_t capacity = element->GetLength();
    if (newLen <= capacity) {
        // judge if need to cut down the array size, else fill the unused tail with holes
        CheckAndCopyArray(thread, JSHandle<JSArray>(array));
        JSObject::FillElementsWithHoles(thread, array, newLen, oldLen < capacity ? oldLen : capacity);
    }
    if (JSObject::ShouldTransToDict(oldLen, newLen)) {
        JSObject::ElementsToDictionary(thread, array);
    } else if (newLen > capacity) {
        JSObject::GrowElementsCapacity(thread, array, newLen, isNew);
    }
    JSArray::Cast(*array)->SetArrayLength(thread, newLen);

    // Update ElementsKind after reset array length.
    // Add this switch because we do not support ElementsKind for instance from new Array
    if (!array->IsElementDict()) {
        ElementsKind oldKind = array->GetClass()->GetElementsKind();
        if (Elements::IsGeneric(oldKind)) {
            return;
        }
#if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
        ElementsKind newKind = ElementsKind::GENERIC;
#else
        // 1.When elementsKind is NONE, means thisArray is empty,
        // so we don't need to traverse the elements to transform elementskind.
        // 2.Make sure array is already created.
        // 3.Make sure newLen > 0 for avoid making empty array elementsKind to HOLE accidently.
        // ASSERT: If an array's elementsKind is NONE, its length must be zero.
        if (Elements::IsNone(oldKind) && !isNew && newLen > 0) {
            ASSERT(oldLen == 0);
            JSHClass::TransitToElementsKindUncheck(thread, array, ElementsKind::HOLE);
            return;
        }
        ElementsKind newKind = ElementsKind::NONE;
#endif
        for (uint32_t i = 0; i < newLen; ++i) {
            JSTaggedValue val = ElementAccessor::Get(array, i);
            newKind = Elements::ToElementsKind(val, newKind);
        }
        // elements length might not be zero when newLen is zero
        uint32_t oldElementsLength = ElementAccessor::GetElementsLength(array);
        if (newKind == ElementsKind::NONE && oldElementsLength != 0) {
            JSHandle<TaggedArray> newTaggedArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(oldElementsLength);
            array->SetElements(thread, newTaggedArray);
            if (!JSHClass::TransitToElementsKindUncheck(thread, array, newKind)) {
                ASSERT(array->GetClass()->GetElementsKind() == ElementsKind::GENERIC);
            }
        } else if (newKind != oldKind) {
            if (JSHClass::TransitToElementsKindUncheck(thread, array, newKind)) {
                Elements::MigrateArrayWithKind(thread, array, oldKind, newKind);
            } else {
                // For the case that array has property transition,
                // Its elementsKind should be GENERIC for now.
                ASSERT(array->GetClass()->GetElementsKind() == ElementsKind::GENERIC);
            }
        }
    }
}

bool JSArray::ArraySetLength(JSThread *thread, const JSHandle<JSObject> &array, const PropertyDescriptor &desc)
{
    JSHandle<JSTaggedValue> lengthKeyHandle(thread->GlobalConstants()->GetHandledLengthString());

    // 1. If the [[Value]] field of Desc is absent, then
    if (!desc.HasValue()) {
        // 1a. Return OrdinaryDefineOwnProperty(A, "length", Desc).
        return JSObject::OrdinaryDefineOwnProperty(thread, array, lengthKeyHandle, desc);
    }
    // 2. Let newLenDesc be a copy of Desc.
    // (Actual copying is not necessary.)
    PropertyDescriptor newLenDesc = desc;
    // 3. - 7. Convert Desc.[[Value]] to newLen.
    uint32_t newLen = 0;
    if (!JSTaggedValue::ToArrayLength(thread, desc.GetValue(), &newLen)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must equal or less than 2^32.", false);
    }
    // 8. Set newLenDesc.[[Value]] to newLen.
    // (Done below, if needed.)
    // 9. Let oldLenDesc be OrdinaryGetOwnProperty(A, "length").
    PropertyDescriptor oldLenDesc(thread);
    [[maybe_unused]] bool success = GetOwnProperty(thread, array, lengthKeyHandle, oldLenDesc);
    // 10. (Assert)
    ASSERT(success);

    // 11. Let oldLen be oldLenDesc.[[Value]].
    uint32_t oldLen = 0;
    JSTaggedValue::ToArrayLength(thread, oldLenDesc.GetValue(), &oldLen);
    // 12. If newLen >= oldLen, then
    if (newLen >= oldLen) {
        // 8. Set newLenDesc.[[Value]] to newLen.
        // 12a. Return OrdinaryDefineOwnProperty(A, "length", newLenDesc).
        newLenDesc.SetValue(JSHandle<JSTaggedValue>(thread, JSTaggedValue(newLen)));
        return JSObject::OrdinaryDefineOwnProperty(thread, array, lengthKeyHandle, newLenDesc);
    }
    // 13. If oldLenDesc.[[Writable]] is false, return false.
    if (!oldLenDesc.IsWritable() ||
        // Also handle the {configurable: true} case since we later use
        // JSArray::SetLength instead of OrdinaryDefineOwnProperty to change
        // the length, and it doesn't have access to the descriptor anymore.
        newLenDesc.IsConfigurable() ||
        (newLenDesc.HasEnumerable() && (newLenDesc.IsEnumerable() != oldLenDesc.IsEnumerable()))) {
        return false;
    }
    // 14. If newLenDesc.[[Writable]] is absent or has the value true,
    // let newWritable be true.
    bool newWritable = false;
    if (!newLenDesc.HasWritable() || newLenDesc.IsWritable()) {
        newWritable = true;
    } else {
    // 15. Else,
    // 15a. Need to defer setting the [[Writable]] attribute to false in case
    //      any elements cannot be deleted.
    // 15b. Let newWritable be false. (It's initialized as "false" anyway.)
    // 15c. Set newLenDesc.[[Writable]] to true.
    // (Not needed.)
    }

    // Most of steps 16 through 19 is implemented by JSArray::SetCapacity.
    JSArray::SetCapacity(thread, array, oldLen, newLen);
    // Steps 19d-ii, 20.
    if (!newWritable) {
        PropertyDescriptor readonly(thread);
        readonly.SetWritable(false);
        success = JSObject::DefineOwnProperty(thread, array, lengthKeyHandle, readonly);
        ASSERT_PRINT(success, "DefineOwnProperty of length must be success here!");
    }

    // Steps 19d-v, 21. Return false if there were non-deletable elements.
    uint32_t arrayLength = JSArray::Cast(*array)->GetArrayLength();
    return arrayLength == newLen;
}

bool JSArray::PropertyKeyToArrayIndex(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint32_t *output)
{
    return JSTaggedValue::ToArrayLength(thread, key, output) && *output <= JSArray::MAX_ARRAY_INDEX;
}

// 9.4.2.1 [[DefineOwnProperty]] ( P, Desc)
bool JSArray::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, const JSHandle<JSTaggedValue> &key,
                                const PropertyDescriptor &desc)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key!");
    // 2. If P is "length", then
    if (IsLengthString(thread, key)) {
        // a. Return ArraySetLength(A, Desc).
        return ArraySetLength(thread, array, desc);
    }
    // 3. Else if P is an array index, then
    // already do in step 4.
    // 4. Return OrdinaryDefineOwnProperty(A, P, Desc).
    bool success = JSObject::OrdinaryDefineOwnProperty(thread, array, key, desc);
    if (success) {
        JSTaggedValue constructorKey = thread->GlobalConstants()->GetConstructorString();
        if (key.GetTaggedValue() == constructorKey) {
            array->GetJSHClass()->SetHasConstructor(true);
            return true;
        }
    }
    return success;
}

bool JSArray::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, uint32_t index,
                                const PropertyDescriptor &desc)
{
    return JSObject::OrdinaryDefineOwnProperty(thread, array, index, desc);
}

bool JSArray::IsLengthString(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    return key.GetTaggedValue() == thread->GlobalConstants()->GetLengthString();
}

// ecma6 7.3 Operations on Objects
JSHandle<JSArray> JSArray::CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements)
{
    // Assert: elements is a List whose elements are all ECMAScript language values.
    // 2. Let array be ArrayCreate(0) (see 9.4.2.2).
    uint32_t length = elements->GetLength();

    // 4. For each element e of elements
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> arrayFunc(env->GetArrayFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(arrayFunc);
    obj->GetJSHClass()->SetExtensible(true);
    JSArray::Cast(*obj)->SetArrayLength(thread, length);

    obj->SetElements(thread, elements);
    JSHandle<JSArray> arr(obj);
    JSHClass::TransitToElementsKind(thread, arr, ElementsKind::GENERIC);

    return arr;
}

// used for array contructor with (...items)
JSHandle<JSArray> JSArray::CreateArrayFromList(JSThread *thread, const JSHandle<JSTaggedValue> &newtarget,
                                               const JSHandle<TaggedArray> &elements)
{
    // Assert: elements is a List whose elements are all ECMAScript language values.
    uint32_t length = elements->GetLength();

    // create arr object
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> arrayFunc(env->GetArrayFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(arrayFunc, newtarget);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    obj->GetJSHClass()->SetExtensible(true);

    // set elements with initItems
    JSHandle<JSArray> arr(obj);
    arr->SetArrayLength(thread, length);
    obj->SetElements(thread, elements);

    return arr;
}

JSHandle<JSTaggedValue> JSArray::FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                        uint32_t index)
{
    auto result = ObjectFastOperator::FastGetPropertyByIndex(thread, obj.GetTaggedValue(), index);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>(thread, result);
}

JSHandle<JSTaggedValue> JSArray::FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                        const JSHandle<JSTaggedValue> &key)
{
    auto result = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>(thread, result);
}

bool JSArray::FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                                     const JSHandle<JSTaggedValue> &value)
{
    return ObjectFastOperator::FastSetPropertyByIndex(thread, obj.GetTaggedValue(), index, value.GetTaggedValue());
}

bool JSArray::FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    return ObjectFastOperator::FastSetPropertyByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue(),
                                                      value.GetTaggedValue());
}

bool JSArray::TryFastCreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                        const JSHandle<JSTaggedValue> &value,  SCheckMode sCheckMode)
{
#if ENABLE_NEXT_OPTIMIZATION
    JSHandle<JSTaggedValue> objVal(obj);
    if (!objVal->IsStableJSArray(thread)) {
        // if JSArray is DictionaryMode goto slowPath
        return JSObject::CreateDataPropertyOrThrow(thread, obj, index, value, sCheckMode);
    }

    uint32_t capacity = TaggedArray::Cast(obj->GetElements())->GetLength();
    ElementsKind kind = obj->GetJSHClass()->GetElementsKind();
    uint32_t len = JSHandle<JSArray>::Cast(obj)->GetArrayLength();
    if (index > len || kind != ElementsKind::HOLE_TAGGED) {
        // goto slowPath
        return JSObject::CreateDataPropertyOrThrow(thread, obj, index, value, sCheckMode);
    }

    if (index == len) {
        // append situation
        if (!IsArrayLengthWritable(thread, obj)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "UnWritable ArrayLength", false);
        }

        uint32_t newLen = index + 1;
        if (newLen > capacity) {
            // needs to expand the capacity
            return JSObject::CreateDataPropertyOrThrow(thread, obj, index, value, sCheckMode);
        }
        JSHandle<JSArray>::Cast(obj)->SetArrayLength(thread, newLen);
    }
    
    TaggedArray::Cast(obj->GetElements())->Set(thread, index, value);
    return true;
#else
    return JSObject::CreateDataPropertyOrThrow(thread, obj, index, value, sCheckMode);
#endif
}

JSTaggedValue JSArray::CopySortedListToReceiver(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                JSHandle<TaggedArray> sortedList, uint32_t len)
{
    // 6. Let itemCount be the number of elements in sortedList.
    uint32_t itemCount = sortedList->GetLength();
    // 7. Let j be 0.
    uint32_t j = 0;
    // 8. Repeat, while j < itemCount,
    //     a. Perform ! Set(obj, ! ToString((j)), sortedList[j], true).
    //     b. Set j to j + 1.
    JSMutableHandle<JSTaggedValue> item(thread, JSTaggedValue::Undefined());
    while (j < itemCount) {
        item.Update(sortedList->Get(j));
        JSArray::FastSetPropertyByValue(thread, obj, j, item);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++j;
    }
    // 9. NOTE: The call to SortIndexedProperties in step 5 uses SKIP-HOLES.The remaining indices are deleted to
    // preserve the number of holes that were detected and excluded from the sort.
    // 10. Repeat, while j < len,
    //       a. Perform ? DeletePropertyOrThrow(obj, ! ToString((j))).
    //       b. Set j to j + 1.
    while (j < len) {
        item.Update(JSTaggedValue(j));
        JSTaggedValue::DeletePropertyOrThrow(thread, obj, item);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ++j;
    }
    return obj.GetTaggedValue();
}

// ecma2024 23.1.3.20 Array.prototype.sort(comparefn)
JSTaggedValue JSArray::Sort(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &fn)
{
    ASSERT(fn->IsUndefined() || fn->IsCallable());
    // 3. Let len be ?LengthOfArrayLike(obj).
    int64_t len = ArrayHelper::GetArrayLength(thread, obj);
    // ReturnIfAbrupt(len).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If len is 0 or 1, no need to sort
    if (len == 0 || len == 1) {
        return obj.GetTaggedValue();
    }

    // 4. Let SortCompare be a new Abstract Closure with parameters (x, y) that captures comparefn and performs
    // the following steps when called:
    //    a. Return ? CompareArrayElements(x, y, comparefn).
    // 5. Let sortedList be ? SortIndexedProperties(O, len, SortCompare, SKIP-HOLES).
    JSHandle<TaggedArray> sortedList =
        ArrayHelper::SortIndexedProperties(thread, obj, len, fn, base::HolesType::SKIP_HOLES);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSArray::CopySortedListToReceiver(thread, obj, sortedList, len);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return obj.GetTaggedValue();
}

void JSArray::SortElements(JSThread *thread, const JSHandle<TaggedArray> &elements, const JSHandle<JSTaggedValue> &fn)
{
    ASSERT(fn->IsUndefined() || fn->IsCallable());

    uint32_t len = elements->GetLength();
    // 64: if the elements is more than 64, use merge-sort algorithm.
    if (len < 64) {
        SortElementsByInsertionSort(thread, elements, len, fn);
    } else {
        SortElementsByMergeSort(thread, elements, fn, 0, len - 1);
    }
}

void JSArray::SortElementsByMergeSort(JSThread *thread, const JSHandle<TaggedArray> &elements,
                                      const JSHandle<JSTaggedValue> &fn, int64_t startIdx, int64_t endIdx)
{
    if (startIdx >= endIdx)
        return;

    int64_t middleIdx = startIdx + (endIdx - startIdx) / 2; // 2: half
    SortElementsByMergeSort(thread, elements, fn, startIdx, middleIdx);
    SortElementsByMergeSort(thread, elements, fn, middleIdx + 1, endIdx);
    MergeSortedElements(thread, elements, fn, startIdx, middleIdx, endIdx);
}

void JSArray::MergeSortedElements(JSThread *thread, const JSHandle<TaggedArray> &elements,
                                  const JSHandle<JSTaggedValue> &fn, int64_t startIdx,
                                  int64_t middleIdx, int64_t endIdx)
{
    int64_t leftLength = middleIdx - startIdx + 1;
    int64_t rightLength = endIdx - middleIdx;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> leftArray = factory->NewTaggedArray(leftLength);
    JSHandle<TaggedArray> rightArray = factory->NewTaggedArray(rightLength);

    for (int64_t i = 0; i < leftLength; i++) {
        leftArray->Set(thread, i, elements->Get(startIdx + i));
    }
    for (int64_t j = 0; j < rightLength; j++) {
        rightArray->Set(thread, j, elements->Get(static_cast<int32_t>(middleIdx + 1 + j)));
    }

    int64_t i = 0;
    int64_t j = 0;
    int64_t k = startIdx;
    JSMutableHandle<JSTaggedValue> leftValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> rightValue(thread, JSTaggedValue::Undefined());
    while (i < leftLength && j < rightLength) {
        leftValue.Update(leftArray->Get(i));
        rightValue.Update(rightArray->Get(j));
        int64_t compareRet = base::ArrayHelper::SortCompare(thread, fn, leftValue, rightValue);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        if (compareRet <= 0) {
            elements->Set(thread, k, leftArray->Get(i));
            i++;
        } else {
            elements->Set(thread, k, rightArray->Get(j));
            j++;
        }
        k++;
    }

    while (i < leftLength) {
        elements->Set(thread, k, leftArray->Get(i));
        i++;
        k++;
    }

    while (j < rightLength) {
        elements->Set(thread, k, rightArray->Get(j));
        j++;
        k++;
    }
}

void JSArray::SortElementsByInsertionSort(JSThread *thread, const JSHandle<TaggedArray> &elements, uint32_t len,
    const JSHandle<JSTaggedValue> &fn)
{
    if (len <= 1)
        return;

    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 1; i < len; i++) {
        uint32_t beginIndex = 0;
        uint32_t endIndex = i;
        presentValue.Update(elements->Get(i));
        while (beginIndex < endIndex) {
            uint32_t middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(elements->Get(middleIndex));
            double compareResult = base::ArrayHelper::SortCompare(thread, fn, middleValue, presentValue);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }

        if (endIndex >= 0 && endIndex < i) {
            for (uint32_t j = i; j > endIndex; j--) {
                previousValue.Update(elements->Get(j - 1));
                elements->Set(thread, j, previousValue);
            }
            elements->Set(thread, endIndex, presentValue);
        }
    }
}

void JSArray::SortElementsByObject(JSThread *thread, const JSHandle<JSObject> &thisObjHandle,
                                   const JSHandle<JSTaggedValue> &fn)
{
    ASSERT(fn->IsUndefined() || fn->IsCallable());

    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    uint32_t len = ElementAccessor::GetElementsLength(thisObjHandle);
    for (uint32_t i = 1; i < len; i++) {
        uint32_t beginIndex = 0;
        uint32_t endIndex = i;
        presentValue.Update(ElementAccessor::Get(thisObjHandle, i));
        while (beginIndex < endIndex) {
            uint32_t middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(ElementAccessor::Get(thisObjHandle, middleIndex));
            int32_t compareResult = base::ArrayHelper::SortCompare(thread, fn, middleValue, presentValue);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }

        if (endIndex >= 0 && endIndex < i) {
            for (uint32_t j = i; j > endIndex; j--) {
                previousValue.Update(ElementAccessor::Get(thisObjHandle, j - 1));
                ElementAccessor::Set(thread, thisObjHandle, j, previousValue, false);
            }
            ElementAccessor::Set(thread, thisObjHandle, endIndex, presentValue, false);
        }
    }
}

bool JSArray::IncludeInSortedValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &value)
{
    ASSERT(obj->IsJSArray());
    JSHandle<JSArray> arrayObj = JSHandle<JSArray>::Cast(obj);
    int32_t length = static_cast<int32_t>(arrayObj->GetArrayLength());
    if (length == 0) {
        return false;
    }
    int32_t left = 0;
    int32_t right = length - 1;
    while (left <= right) {
        int32_t middle = (left + right) / 2;
        JSHandle<JSTaggedValue> vv = JSArray::FastGetPropertyByValue(thread, obj, middle);
        ComparisonResult res = JSTaggedValue::Compare(thread, vv, value);
        if (res == ComparisonResult::EQUAL) {
            return true;
        } else if (res == ComparisonResult::LESS) {
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    return false;
}

JSHandle<TaggedArray> JSArray::ToTaggedArray(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ASSERT(obj->IsJSArray());
    JSHandle<JSArray> arrayObj = JSHandle<JSArray>::Cast(obj);
    uint32_t length = arrayObj->GetArrayLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(length);
    for (uint32_t idx = 0; idx < length; idx++) {
        JSHandle<JSTaggedValue> vv = JSArray::FastGetPropertyByValue(thread, obj, idx);
        taggedArray->Set(thread, idx, vv);
    }
    return taggedArray;
}

void JSArray::CheckAndCopyArray(const JSThread *thread, JSHandle<JSArray> obj)
{
    JSHandle<TaggedArray> arr(thread, obj->GetElements());
    // Check whether array is shared in the nonmovable space before set properties and elements.
    // If true, then really copy array in the semi space.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (arr.GetTaggedValue().IsCOWArray()) {
        auto newArray = factory->CopyArray(arr, arr->GetLength(), arr->GetLength(),
            JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        obj->SetElements(thread, newArray.GetTaggedValue());
    }
    JSHandle<TaggedArray> prop(thread, obj->GetProperties());
    if (prop.GetTaggedValue().IsCOWArray()) {
        auto newProps = factory->CopyArray(prop, prop->GetLength(), prop->GetLength(),
            JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        obj->SetProperties(thread, newProps.GetTaggedValue());
    }
}

//static
bool JSArray::IsProtoNotChangeJSArray(JSThread *thread, const JSHandle<JSObject> &obj)
{
    if (obj->IsJSArray()) {
        if (obj->GetJSHClass()->GetElementsKind() != ElementsKind::GENERIC) {
            return true;
        }
        JSTaggedValue arrayProtoValue = obj->GetJSHClass()->GetProto();
        JSTaggedValue genericArrayHClass = thread->GlobalConstants()->GetElementHoleTaggedClass();
        JSTaggedValue genericArrayProtoValue = \
            JSHClass::Cast(genericArrayHClass.GetTaggedObject())->GetProto();
        return genericArrayProtoValue == arrayProtoValue;
    }
    return false;
}
}  // namespace panda::ecmascript
