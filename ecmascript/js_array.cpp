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
    if (!JSTaggedValue::ToArrayLength(thread, value, &newLen)) {
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

// Used to check whether newArrayHandle's proto == Array.prototype.
void JSArray::CheckAndSetPrototypeModified(JSThread* thread, const JSHandle<JSObject> &newArrayHandle)
{
    if (!JSArray::IsProtoNotChangeJSArray(thread, newArrayHandle)) {
        newArrayHandle->GetJSHClass()->SetIsJSArrayPrototypeModified(true);
    }
};

// 9.4.2.2 ArrayCreate(length, proto)
JSHandle<JSTaggedValue> JSArray::ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                             const JSHandle<JSTaggedValue> &newTarget, ArrayMode mode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Assert: length is an integer Number ≥ 0.
    ASSERT_PRINT(length.IsInteger() && length.GetNumber() >= 0, "length must be positive integer");
    // 2. If length is −0, let length  be +0.
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
    CheckAndSetPrototypeModified(thread, obj);
    return JSHandle<JSTaggedValue>(obj);
}

JSTaggedValue JSArray::GetConstructorOrSpeciesInlinedProp(JSTaggedValue object, uint32_t inlinePropIndex)
{
    JSHClass *hclass = JSObject::Cast(object)->GetJSHClass();
    ASSERT(!hclass->IsDictionaryMode() && "object can't be dictionary");
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    PropertyAttributes attr(layoutInfo->GetAttr(inlinePropIndex));
    ASSERT(attr.GetOffset() == inlinePropIndex && "offset of Attr must be inlinePropIndex");
    ASSERT(attr.IsInlinedProps() && "attr must be inline prop");
    JSTaggedValue value = JSObject::Cast(object)->GetPropertyInlinedPropsWithRep(hclass, attr.GetOffset(), attr);
    ASSERT(!value.IsHole() && "object must have inlinePropIndex");
    return value;
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
    // 1. Let isArray be ? IsArray(originalArray).
    JSHandle<JSTaggedValue> originalValue(originalArray);
    bool isArray = originalValue->IsArray(thread);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. If isArray is false, return ? ArrayCreate(length).
    if (!isArray) {
        return ArrayCreate(thread, length).GetTaggedValue();
    }

    JSMutableHandle<JSTaggedValue> constructor(thread, globalConst->GetHandledHole());
    // 3. Let C be ? Get(originalArray, "constructor").
    // 3. fastpath: if object has no custom constructor, the constructor may be on the proto.
    if (originalArray->IsJSArray() && !originalArray->GetJSHClass()->HasConstructor()) {
        JSTaggedValue proto = JSObject::GetPrototype(originalArray);
        // fastpath: if the hclass of proto is the default Array Prototype hclass,
        // the constructor must in the inline properties.
        if LIKELY(proto.IsECMAObject()
            && JSObject::Cast(proto)->GetJSHClass() == thread->GetBuiltinPrototypeHClass(BuiltinTypeId::ARRAY)) {
            constructor.Update(GetConstructorOrSpeciesInlinedProp(proto, CONSTRUCTOR_INLINE_PROPERTY_INDEX));
        }
    }

    if (constructor->IsHole()) {
        // 3. slowpath: Let C be ? Get(originalArray, "constructor").
        JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
        JSTaggedValue c = ObjectFastOperator::FastGetPropertyByValue(thread, originalValue.GetTaggedValue(),
                                                                     constructorKey.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        constructor.Update(c);
    }

    // fastpath: if constructor is the default constructor. don't need check the realm.
    // check the species of it.
    if (constructor == env->GetArrayFunction() && constructor->IsECMAObject()) {
        JSTaggedValue taggedCtor = constructor.GetTaggedValue();
        JSHClass *chc = JSObject::Cast(taggedCtor)->GetJSHClass();
        // if the hclass of constructor is the default Array Function hclass,
        // the species must in the inline properties.
        if LIKELY(chc == thread->GetBuiltinHClass(BuiltinTypeId::ARRAY)) {
            JSTaggedValue species = GetConstructorOrSpeciesInlinedProp(taggedCtor, ARRAY_FUNCTION_SPECIES_INDEX);
            if (species == globalConst->GetArraySpeciesAccessor()) {
                // fast path: means using default constructor, do ArrayCreate directly.
                return ArrayCreate(thread, length).GetTaggedValue();
            }
        }
    }

    // 4. If IsConstructor(C) is true, then
    if (constructor->IsConstructor()) {
        // a. Let thisRealm be the current Realm Record.
        // b. Let realmC be ? GetFunctionRealm(C).
        JSHandle<GlobalEnv> realmC = JSObject::GetFunctionRealm(thread, constructor);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. If thisRealm and realmC are not the same Realm Record, then
        if (*realmC != *env) {
            JSTaggedValue realmArrayConstructor = realmC->GetArrayFunction().GetTaggedValue();
            // i. If SameValue(C, realmC.[[Intrinsics]].[[%Array%]]) is true, set C to undefined.
            if (JSTaggedValue::SameValue(constructor.GetTaggedValue(), realmArrayConstructor)) {
                constructor.Update(globalConst->GetUndefined());
            }
        }
    }

    // 5. slowpath: If Type(C) is Object, then
    if (constructor->IsECMAObject()) {
        // a. Set C to ? Get(C, @@species).
        JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
        JSTaggedValue speciesConstructor = ObjectFastOperator::FastGetPropertyByValue(
            thread, constructor.GetTaggedValue(), speciesSymbol.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // b. If C is null, set C to undefined.
        if (speciesConstructor.IsNull()) {
            // fast path: use ArrayCreate instead pf set C to undefined.
            return ArrayCreate(thread, length).GetTaggedValue();
        }
        constructor.Update(speciesConstructor);
    }

    // 6. If C is undefined, return ? ArrayCreate(length).
    if (constructor->IsUndefined()) {
        return JSArray::ArrayCreate(thread, length).GetTaggedValue();
    }
    // 7. If IsConstructor(C) is false, throw a TypeError exception.
    if (!constructor->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a constructor", JSTaggedValue::Exception());
    }
    // 8. Return ? Construct(C, « 𝔽(length)»).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    info->SetCallArg(JSTaggedValue(arrayLength));
    JSTaggedValue result = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // The abstract operation ArraySpeciesCreate takes arguments originalArray and length (a non-negative integer).
    // It is used to specify the creation of a new Array object using a constructor function that is derived from
    // originalArray. It performs the following steps when called:
    // 1. Let isArray be ? IsArray(originalArray).
    // 2. If isArray is false, return ? ArrayCreate(length).
    // 3. Let C be ? Get(originalArray, "constructor").
    // 4. If IsConstructor(C) is true, then
    //      a. Let thisRealm be the current Realm Record.
    //      b. Let realmC be ? GetFunctionRealm(C).
    //      c. If thisRealm and realmC are not the same Realm Record, then
    //          i. If SameValue(C, realmC.[[Intrinsics]].[[%Array%]]) is true, set C to undefined.
    // 5. If Type(C) is Object, then
    //      a. Set C to ? Get(C, @@species).
    //      b. If C is null, set C to undefined.
    // 6. If C is undefined, return ? ArrayCreate(length).
    // 7. If IsConstructor(C) is false, throw a TypeError exception.
    // 8. Return ? Construct(C, « 𝔽(length) »).

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
        uint32_t numOfElements = array->GetNumberOfElements(thread);
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
    JSArray::TransformElementsKindAfterSetCapacity(thread, array, oldLen, newLen, isNew);
}

void JSArray::TransformElementsKindAfterSetCapacity(JSThread *thread, const JSHandle<JSObject> &array,
                                                    [[maybe_unused]] uint32_t oldLen, uint32_t newLen,
                                                    [[maybe_unused]] bool isNew)
{
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
            JSTaggedValue val = ElementAccessor::Get(thread, array, i);
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
    return JSObject::OrdinaryDefineOwnProperty(thread, array, key, desc);
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

// static
// Check whether the element of the array is dictionary element,
// proto of the array has not been modified
// the element of the array prototype has not been modified
// the attribute of the array has not been modified
bool JSArray::IsProtoNotModifiedDictionaryJSArray(JSThread *thread, const JSHandle<JSObject> &obj)
{
    return obj->GetJSHClass()->IsDictionaryElement() &&
           !thread->IsArrayPrototypeChangedGuardiansInvalid() &&
           !obj->GetClass()->IsJSArrayPrototypeModifiedFromBitField() &&
           JSObject::AttributesUnchanged(thread, obj);
}

#if ENABLE_NEXT_OPTIMIZATION
// ecma6 7.3 Operations on Objects
JSHandle<JSArray> JSArray::CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements)
{
    // Assert: elements is a List whose elements are all ECMAScript language values.
    uint32_t length = elements->GetLength();
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    
    // New JSObject by Constructor.
    JSTaggedValue protoOrHClass = JSHandle<JSFunction>::Cast(env->GetArrayFunction())->GetProtoOrHClass();
    JSHandle<JSHClass> jsHClass = JSHandle<JSHClass>(thread,
        reinterpret_cast<JSHClass *>(protoOrHClass.GetTaggedObject()));
    
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObject(jsHClass);
    
    JSArray::Cast(*obj)->SetLength(0);
    JSArray::Cast(*obj)->SetTrackInfo(thread, JSTaggedValue::Undefined());
    auto accessor = thread->GlobalConstants()->GetArrayLengthAccessor();
    JSArray::Cast(*obj)->SetPropertyInlinedProps(thread, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
    
    obj->GetJSHClass()->SetExtensible(true);
    JSArray::Cast(*obj)->SetArrayLength(thread, length);
    obj->SetElements(thread, elements);

    JSHandle<JSArray> arr(obj);
    JSHClass::TransitToElementsKind(thread, arr, ElementsKind::GENERIC);
    return arr;
}
# else
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
#endif

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

    CheckAndSetPrototypeModified(thread, obj);
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
    uint32_t len = JSHandle<JSArray>::Cast(obj)->GetArrayLength();
    if UNLIKELY(index > len) {
        // goto slowPath
        return JSObject::CreateDataPropertyOrThrow(thread, obj, index, value, sCheckMode);
    }

    if UNLIKELY(index == len) {
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
    if LIKELY(!thread->IsEnableMutantArray()) {
        TaggedArray::Cast(obj->GetElements())->Set(thread, index, value);
        if LIKELY(thread->IsEnableElementsKind()) {
            JSHClass::TransitToElementsKind(thread, obj, value, ElementsKind::NONE);
        }
    } else {
        ElementAccessor::Set(thread, obj, index, value, true);
    }
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


JSTaggedValue JSArray::FastConcatDictionaryArray(JSThread *thread, JSHandle<JSObject> obj,
    JSHandle<JSObject> &newArrayHandle, JSMutableHandle<JSTaggedValue> &fromValHandle,
    JSMutableHandle<JSTaggedValue> &toKey, int64_t &n)
{
    bool isArrayHandleDictionary = newArrayHandle->GetJSHClass()->IsDictionaryElement();
    if (!isArrayHandleDictionary) {
        JSObject::ElementsToDictionary(thread, newArrayHandle);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    ASSERT(newArrayHandle->GetJSHClass()->IsDictionaryElement());
    JSHandle<JSTaggedValue> objVal(obj);
    int64_t len = base::ArrayHelper::GetArrayLength(thread, objVal);
    JSHandle<NumberDictionary> elements(thread, obj->GetElements());
    uint32_t size = static_cast<uint32_t>(elements->Size());
    JSMutableHandle<NumberDictionary> dict(thread, newArrayHandle->GetElements());
    auto attr = PropertyAttributes(PropertyAttributes::GetDefaultAttributes());
    for (uint32_t hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = elements->GetKey(hashIndex);
        if (key.IsUndefined() || key.IsHole()) {
            continue;
        }
        ASSERT(key.IsInt());
        uint32_t uintKey = static_cast<uint32_t>(key.GetInt());
        if (uintKey < len) {
            JSTaggedValue value = elements->GetValue(hashIndex);
            toKey.Update(JSTaggedValue(static_cast<int32_t>(n + uintKey))); // guarantee the toKey IsInt
            fromValHandle.Update(value);
            JSHandle<NumberDictionary> newDict = \
                NumberDictionary::PutIfAbsent(thread, dict, toKey, fromValHandle, attr);
            dict.Update(newDict);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    }
    newArrayHandle->SetElements(thread, dict);
    n += len;
    return JSTaggedValue(true);
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
        presentValue.Update(ElementAccessor::Get(thread, thisObjHandle, i));
        while (beginIndex < endIndex) {
            uint32_t middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(ElementAccessor::Get(thread, thisObjHandle, middleIndex));
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
                previousValue.Update(ElementAccessor::Get(thread, thisObjHandle, j - 1));
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

// static
bool JSArray::IsProtoNotChangeJSArray(JSThread *thread, const JSHandle<JSObject> &obj)
{
    if (obj->IsJSArray()) {
        if (obj->GetJSHClass()->GetElementsKind() != ElementsKind::GENERIC) {
            return true;
        }
        JSTaggedValue arrayProtoValue = JSObject::GetPrototype(obj);
        JSTaggedValue genericArrayHClass = thread->GlobalConstants()->GetElementHoleTaggedClass();
        JSTaggedValue genericArrayProtoValue = \
            JSHClass::Cast(genericArrayHClass.GetTaggedObject())->GetProto();
        return genericArrayProtoValue == arrayProtoValue;
    }
    return false;
}

JSHandle<JSHClass> JSArray::CreateJSArrayPrototypeClass(const JSThread *thread, ObjectFactory *factory,
                                                        JSHandle<JSTaggedValue> proto, uint32_t inlinedProps)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSHClass> arrayClass = factory->NewEcmaHClass(JSArray::SIZE, inlinedProps, JSType::JS_ARRAY, proto);

    uint32_t fieldOrder = 0;
    ASSERT(JSArray::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(inlinedProps);
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        JSTaggedValue key = globalConst->GetLengthString();
        layoutInfoHandle->AddKey(thread, JSArray::LENGTH_INLINE_PROPERTY_INDEX, key, attributes);
    }
    ASSERT(JSArray::CONSTRUCTOR_INLINE_PROPERTY_INDEX == fieldOrder);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        JSTaggedValue key = globalConst->GetConstructorString();
        layoutInfoHandle->AddKey(thread, JSArray::CONSTRUCTOR_INLINE_PROPERTY_INDEX, key, attributes);
    }
    {
        arrayClass->SetLayout(thread, layoutInfoHandle);
        arrayClass->SetNumberOfProps(fieldOrder);
    }
    arrayClass->SetIsStableElements(true);
    arrayClass->SetHasConstructor(false);

    return arrayClass;
}

JSHandle<JSHClass> JSArray::CreateJSArrayFunctionClass(const JSThread *thread, ObjectFactory *factory,
                                                       const JSHandle<GlobalEnv> &env)
{
    JSHandle<JSHClass> arrayFunctionClass =
        factory->NewEcmaHClass(JSFunction::SIZE, JSArray::ARRAY_FUNCTION_INLINE_PROPERTY_NUM, JSType::JS_FUNCTION,
                               env->GetFunctionPrototype());
    arrayFunctionClass->SetConstructor(true);

    uint32_t fieldOrder = 0;
    JSHandle<LayoutInfo> layoutInfoHandle = factory->CreateLayoutInfo(1);
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        JSTaggedValue key = env->GetSpeciesSymbol().GetTaggedValue();
        layoutInfoHandle->AddKey(thread, JSArray::ARRAY_FUNCTION_SPECIES_INDEX, key, attributes);
    }
    {
        arrayFunctionClass->SetLayout(thread, layoutInfoHandle);
        arrayFunctionClass->SetNumberOfProps(fieldOrder);
    }
    return arrayFunctionClass;
}

void JSArray::UpdateTrackInfo(const JSThread *thread)
{
    JSTaggedValue trackInfoVal = GetTrackInfo();
    if (trackInfoVal.IsHeapObject() && trackInfoVal.IsWeak()) {
        TrackInfo *trackInfo = TrackInfo::Cast(trackInfoVal.GetWeakReferentUnChecked());
        ElementsKind oldKind = trackInfo->GetElementsKind();
        if (Elements::IsGeneric(oldKind)) {
            return;
        }

        JSHClass *hclass = GetJSHClass();
        ElementsKind newKind = hclass->GetElementsKind();
        trackInfo->SetElementsKind(newKind);
        const GlobalEnvConstants *globalConst = thread->GlobalConstants();
        // Since trackInfo is only used at define point,
        // we update cachedHClass with initial array hclass which does not have IsPrototype set.
        ConstantIndex constantId = thread->GetArrayHClassIndexMap().at(newKind).first;
        JSTaggedValue cachedHClass = globalConst->GetGlobalConstantObject(static_cast<size_t>(constantId));
        trackInfo->SetCachedHClass(thread, cachedHClass);
    }
}
}  // namespace panda::ecmascript
