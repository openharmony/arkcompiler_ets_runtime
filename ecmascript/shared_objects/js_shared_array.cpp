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

#include "ecmascript/shared_objects/js_shared_array.h"

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/object_fast_operator-inl.h"

namespace panda::ecmascript {
using base::ArrayHelper;

JSTaggedValue JSSharedArray::LengthGetter([[maybe_unused]] JSThread *thread, const JSHandle<JSObject> &self,
                                          SCheckMode checkMode)
{
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, JSHandle<JSTaggedValue>::Cast(self),
                                                             checkMode);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    return JSTaggedValue(JSSharedArray::Cast(*self)->GetLength());
}

bool JSSharedArray::DummyLengthSetter([[maybe_unused]] JSThread *thread,
                                      [[maybe_unused]] const JSHandle<JSObject> &self,
                                      [[maybe_unused]] const JSHandle<JSTaggedValue> &value,
                                      [[maybe_unused]] bool mayThrow)
{
    // length is the read only property for Shared Array
    return true;
}

bool JSSharedArray::LengthSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                                 bool mayThrow)
{
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    uint32_t newLen = 0;
    if (!JSTaggedValue::ToArrayLength(thread, value, &newLen) && mayThrow) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    }

    uint32_t oldLen = JSSharedArray::Cast(*self)->GetArrayLength();
    if (oldLen == newLen) {
        return true;
    }

    if (!IsArrayLengthWritable(thread, self)) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetReadOnlyProperty), false);
        }
        return false;
    }

    JSSharedArray::SetCapacity(thread, self, oldLen, newLen);
    uint32_t actualLen = JSSharedArray::Cast(*self)->GetArrayLength();
    if (actualLen != newLen) { // LCOV_EXCL_START
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not all array elements is configurable", false);
        }
        return false;
    } // LCOV_EXCL_STOP

    return true;
}

JSHandle<JSTaggedValue> JSSharedArray::ArrayCreate(JSThread *thread, JSTaggedNumber length, ArrayMode mode)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> sharedArrayFunction = env->GetSharedArrayFunction();
    return JSSharedArray::ArrayCreate(thread, length, sharedArrayFunction, mode);
}

// 9.4.2.2 ArrayCreate(length, proto)
JSHandle<JSTaggedValue> JSSharedArray::ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                                   const JSHandle<JSTaggedValue> &newTarget, ArrayMode mode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Assert: length is an integer Number ≥ 0.
    ASSERT_PRINT(length.IsInteger() && length.GetNumber() >= 0, "length must be positive integer");
    // 2. If length is −0, let length be +0.
    double arrayLength = length.GetNumber();
    if (arrayLength > MAX_ARRAY_INDEX) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::TYPE_ERROR,
                                                               "Parameter error.Array length must less than 2^32.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, exception);
    }
    uint32_t normalArrayLength = length.ToUint32();

    // 8. Set the [[Prototype]] internal slot of A to proto.
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayFunc(env->GetSharedArrayFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(arrayFunc, newTarget);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // 9. Set the [[Extensible]] internal slot of A to true.

    // 10. Perform OrdinaryDefineOwnProperty(A, "length", PropertyDescriptor{[[Value]]: length, [[Writable]]:
    // true, [[Enumerable]]: false, [[Configurable]]: false}).
    if (mode == ArrayMode::LITERAL) {
        JSSharedArray::Cast(*obj)->SetArrayLength(thread, normalArrayLength);
    } else {
        JSSharedArray::SetCapacity(thread, obj, 0, normalArrayLength, true);
    }
    return JSHandle<JSTaggedValue>(obj);
}

// 9.4.2.3 ArraySpeciesCreate(originalArray, length)
JSTaggedValue JSSharedArray::ArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &originalArray,
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
    bool isSArray = originalValue->IsJSSharedArray();
    // ReturnIfAbrupt(isArray).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If isArray is true, then
    JSHandle<JSTaggedValue> constructor(thread, JSTaggedValue::Undefined());
    if (isSArray) {
        // Let C be Get(originalArray, "constructor").
        auto *hclass = originalArray->GetJSHClass();
        JSTaggedValue proto = hclass->GetPrototype();
        if (hclass->IsJSSharedArray() && !hclass->HasConstructor() && proto.IsJSSharedArray()) {
            return JSSharedArray::ArrayCreate(thread, length).GetTaggedValue();
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
                JSTaggedValue realmArrayConstructor = realmC->GetSharedArrayFunction().GetTaggedValue();
                // If SameValue(C, realmC.[[intrinsics]].[[%Array%]]) is true, let C be undefined.
                if (JSTaggedValue::SameValue(constructor.GetTaggedValue(), realmArrayConstructor)) {
                    return JSSharedArray::ArrayCreate(thread, length).GetTaggedValue();
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
                return JSSharedArray::ArrayCreate(thread, length).GetTaggedValue();
            }
        }
    }

    // If C is undefined, return ArrayCreate(length).
    if (constructor->IsUndefined()) {
        return JSSharedArray::ArrayCreate(thread, length).GetTaggedValue();
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

JSHandle<TaggedArray> JSSharedArray::SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                                 uint32_t capa)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t oldLength = array->GetLength();
    JSHandle<TaggedArray> newArray = factory->CopySArray(array, oldLength, capa);
    return newArray;
}

void JSSharedArray::SetCapacity(JSThread *thread, const JSHandle<JSObject> &array, uint32_t oldLen, uint32_t newLen,
                                bool isNew)
{
    TaggedArray *element = TaggedArray::Cast(array->GetElements().GetTaggedObject());

    if (element->IsDictionaryMode()) { // LCOV_EXCL_START
        THROW_TYPE_ERROR(thread, "SendableArray don't support dictionary mode.");
    } // LCOV_EXCL_STOP
    uint32_t capacity = element->GetLength();
    if (newLen <= capacity) {
        // judge if need to cut down the array size, else fill the unused tail with holes
        CheckAndCopyArray(thread, JSHandle<JSSharedArray>(array));
        JSObject::FillElementsWithHoles(thread, array, newLen, oldLen < capacity ? oldLen : capacity);
    }
    if (newLen > capacity) {
        JSObject::GrowElementsCapacity(thread, array, newLen, isNew);
    }
    JSSharedArray::Cast(*array)->SetArrayLength(thread, newLen);
}

bool JSSharedArray::ArraySetLength(JSThread *thread, const JSHandle<JSObject> &array, const PropertyDescriptor &desc)
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
        // JSSharedArray::SetLength instead of OrdinaryDefineOwnProperty to change
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

    // Most of steps 16 through 19 is implemented by JSSharedArray::SetCapacity.
    JSSharedArray::SetCapacity(thread, array, oldLen, newLen);
    // Steps 19d-ii, 20.
    if (!newWritable) { // LCOV_EXCL_START
        PropertyDescriptor readonly(thread);
        readonly.SetWritable(false);
        success = JSObject::DefineOwnProperty(thread, array, lengthKeyHandle, readonly);
        ASSERT_PRINT(success, "DefineOwnProperty of length must be success here!");
    } // LCOV_EXCL_STOP

    // Steps 19d-v, 21. Return false if there were non-deletable elements.
    uint32_t arrayLength = JSSharedArray::Cast(*array)->GetArrayLength();
    return arrayLength == newLen;
}

bool JSSharedArray::PropertyKeyToArrayIndex(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint32_t *output)
{
    return JSTaggedValue::ToArrayLength(thread, key, output) && *output <= JSSharedArray::MAX_ARRAY_INDEX;
}

// 9.4.2.1 [[DefineOwnProperty]] ( P, Desc)
bool JSSharedArray::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array,
                                      const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc,
                                      SCheckMode sCheckMode)
{
    if (!desc.GetValue()->IsSharedType() || (desc.HasGetter() && !desc.GetGetter()->IsSharedType()) ||
        (desc.HasSetter() && !desc.GetSetter()->IsSharedType())) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::TYPE_ERROR,
                                                               "Parameter error. Only accept sendable value.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    if (sCheckMode == SCheckMode::CHECK && !(JSSharedArray::Cast(*array)->IsKeyInRange(key))) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::RANGE_ERROR,
                                                               "Key out of length.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

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
    return JSObject::OrdinaryDefineOwnProperty(thread, array, key, desc);
}

bool JSSharedArray::IsLengthString(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    return key.GetTaggedValue() == thread->GlobalConstants()->GetLengthString();
}

JSHandle<JSSharedArray> JSSharedArray::CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements)
{
    // Assert: elements is a List whose elements are all ECMAScript language values.
    // 2. Let array be ArrayCreate(0).
    uint32_t length = elements->GetLength();

    // 4. For each element e of elements
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> arrayFunc(env->GetSharedArrayFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(arrayFunc);
    JSSharedArray::Cast(*obj)->SetArrayLength(thread, length);
    obj->SetElements(thread, elements);
    obj->GetJSHClass()->SetExtensible(false);
    JSHandle<JSSharedArray> arr(obj);

    return arr;
}

JSHandle<JSTaggedValue> JSSharedArray::FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                              uint32_t index)
{
    auto result = ObjectFastOperator::FastGetPropertyByIndex(thread, obj.GetTaggedValue(), index);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>(thread, result);
}

JSHandle<JSTaggedValue> JSSharedArray::FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                              const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode)
{
    auto result =
        ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue(), sCheckMode);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>(thread, result);
}

bool JSSharedArray::FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                                           const JSHandle<JSTaggedValue> &value)
{
    return ObjectFastOperator::FastSetPropertyByIndex(thread, obj.GetTaggedValue(), index, value.GetTaggedValue());
}

bool JSSharedArray::FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                           const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    return ObjectFastOperator::FastSetPropertyByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue(),
                                                      value.GetTaggedValue(), SCheckMode::SKIP);
}

OperationResult JSSharedArray::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                           const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode)
{
    // Add Concurrent check for shared array
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray> scope(thread, obj,
                                                             sCheckMode);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread,
                                      OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));

    ObjectOperator op(thread, obj, key);
    // Out of bounds check for shared array
    if ((obj->IsJSSharedArray() && sCheckMode == SCheckMode::CHECK) && op.IsElement() && !op.IsFound()) {
        return OperationResult(thread, JSTaggedValue::Undefined(), PropertyMetaData(false));
    }
    return OperationResult(thread, JSObject::GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

bool JSSharedArray::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value, bool mayThrow,
                                SCheckMode sCheckMode)
{
    // Concurrent check for shared array
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(
        thread, obj, sCheckMode);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 2 ~ 4 findProperty in Receiver, Obj and its parents
    ObjectOperator op(thread, obj, key);
    // Out of bounds check for shared array
    if ((obj->IsJSSharedArray() && sCheckMode == SCheckMode::CHECK) && op.IsElement() && !op.IsFound()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::RANGE_ERROR,
                                                               "The value of index is out of range.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }
    return JSObject::SetProperty(&op, value, mayThrow);
}

bool JSSharedArray::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                uint32_t index, const JSHandle<JSTaggedValue> &value, bool mayThrow,
                                SCheckMode sCheckMode)
{
    // Concurrent check for shared array
    [[maybe_unused]] ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(
        thread, obj, sCheckMode);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 2 ~ 4 findProperty in Receiver, Obj and its parents
    ObjectOperator op(thread, obj, index);
    // Out of bounds check for shared array
    if ((obj->IsJSSharedArray() && sCheckMode == SCheckMode::CHECK) && op.IsElement() && !op.IsFound()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::RANGE_ERROR,
                                                               "The value of index is out of range.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }
    return JSObject::SetProperty(&op, value, mayThrow);
}

// ecma2024 23.1.3.20 Array.prototype.sort(comparefn)
JSTaggedValue JSSharedArray::Sort(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                  const JSHandle<JSTaggedValue> &fn)
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
        JSSharedArray::FastSetPropertyByValue(thread, obj, j, item);
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

bool JSSharedArray::IncludeInSortedValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                         const JSHandle<JSTaggedValue> &value)
{
    ASSERT(obj->IsJSSharedArray());
    JSHandle<JSSharedArray> arrayObj = JSHandle<JSSharedArray>::Cast(obj);
    int32_t length = static_cast<int32_t>(arrayObj->GetArrayLength());
    if (length == 0) {
        return false;
    }
    int32_t left = 0;
    int32_t right = length - 1;
    while (left <= right) {
        int32_t middle = (left + right) / 2;
        JSHandle<JSTaggedValue> vv = JSSharedArray::FastGetPropertyByValue(thread, obj, middle);
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

void JSSharedArray::CheckAndCopyArray(const JSThread *thread, JSHandle<JSSharedArray> obj)
{
    JSHandle<TaggedArray> arr(thread, obj->GetElements());
    // Check whether array is shared in the nonmovable space before set properties and elements.
    // If true, then really copy array in the semi space.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (arr.GetTaggedValue().IsCOWArray()) {
        auto newArray = factory->CopyArray(arr, arr->GetLength(), arr->GetLength(),
            JSTaggedValue::Hole(), MemSpaceType::SHARED_OLD_SPACE);
        obj->SetElements(thread, newArray.GetTaggedValue());
    }
    JSHandle<TaggedArray> prop(thread, obj->GetProperties());
    if (prop.GetTaggedValue().IsCOWArray()) {
        auto newProps = factory->CopyArray(prop, prop->GetLength(), prop->GetLength(),
            JSTaggedValue::Hole(), MemSpaceType::SHARED_OLD_SPACE);
        obj->SetProperties(thread, newProps.GetTaggedValue());
    }
}

void JSSharedArray::DeleteInElementMode(const JSThread *thread, JSHandle<JSSharedArray> &obj)
{
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    ASSERT(!obj->GetJSHClass()->IsDictionaryElement());
    uint32_t length = elements->GetLength();
    // fixme(hzzhouzebin) Optimize Delete later.
    uint32_t size = 0;
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue value = ElementAccessor::Get(JSHandle<JSObject>(obj), i);
        if (value.IsHole()) {
            continue;
        }
        ++size;
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newElements(
        factory->NewTaggedArray(length, JSTaggedValue::Hole(), MemSpaceType::SHARED_OLD_SPACE));
    uint32_t newCurr = 0;
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue value = ElementAccessor::Get(JSHandle<JSObject>(obj), i);
        if (value.IsHole()) {
            continue;
        }
        newElements->Set(thread, newCurr, value);
        ++newCurr;
    }
    obj->SetElements(thread, newElements);
}
}  // namespace panda::ecmascript
