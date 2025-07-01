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

#ifndef ECMASCRIPT_JSOBJECT_INL_H
#define ECMASCRIPT_JSOBJECT_INL_H

#include "ecmascript/js_object.h"

#include "ecmascript/element_accessor-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
inline void ECMAObject::SetCallable(bool flag)
{
    GetClass()->SetCallable(flag);
}

inline bool ECMAObject::IsCallable() const
{
    return GetClass()->IsCallable();
}

// JSObject
inline bool JSObject::IsExtensible() const
{
    return GetJSHClass()->IsExtensible();
}

// static
inline void JSObject::FillElementsWithHoles(const JSThread *thread, const JSHandle<JSObject> &obj,
                                            uint32_t start, uint32_t end)
{
    if (start >= end) {
        return;
    }
    JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
    for (uint32_t i = start; i < end; i++) {
        ElementAccessor::Set(thread, obj, i, holeHandle, false);
    }
}

inline uint32_t JSObject::GetNonInlinedFastPropsCapacity() const
{
    uint32_t inlineProps = GetJSHClass()->GetInlinedProperties();
    return PropertyAttributes::MAX_FAST_PROPS_CAPACITY - inlineProps;
}

inline bool JSObject::IsJSGlobalObject() const
{
    return GetJSHClass()->IsJSGlobalObject();
}

inline bool JSObject::IsConstructor() const
{
    return GetJSHClass()->IsConstructor();
}

inline bool JSObject::IsECMAObject() const
{
    return GetJSHClass()->IsECMAObject();
}

inline bool JSObject::IsJSError() const
{
    return GetJSHClass()->IsJSError();
}

inline bool JSObject::IsArguments() const
{
    return GetJSHClass()->IsArguments();
}

inline bool JSObject::IsDate() const
{
    return GetJSHClass()->IsDate();
}

inline bool JSObject::IsJSArray() const
{
    return GetJSHClass()->IsJSArray();
}

inline bool JSObject::IsJSSArray() const
{
    return GetJSHClass()->IsJSSharedArray();
}

inline bool JSObject::IsJSShared() const
{
    return GetJSHClass()->IsJSShared();
}

inline bool JSObject::IsJSMap() const
{
    return GetJSHClass()->IsJSMap();
}

inline bool JSObject::IsJSSet() const
{
    return GetJSHClass()->IsJSSet();
}

inline bool JSObject::IsJSRegExp() const
{
    return GetJSHClass()->IsJSRegExp();
}

inline bool JSObject::IsJSFunction() const
{
    return GetJSHClass()->IsJSFunction();
}

inline bool JSObject::IsBoundFunction() const
{
    return GetJSHClass()->IsJsBoundFunction();
}

inline bool JSObject::IsJSIntlBoundFunction() const
{
    return GetJSHClass()->IsJSIntlBoundFunction();
}

inline bool JSObject::IsProxyRevocFunction() const
{
    return GetJSHClass()->IsJSProxyRevocFunction();
}

inline bool JSObject::IsAccessorData() const
{
    return GetJSHClass()->IsAccessorData();
}

inline bool JSObject::IsJSGlobalEnv() const
{
    return GetJSHClass()->IsJsGlobalEnv();
}

inline bool JSObject::IsJSProxy() const
{
    return GetJSHClass()->IsJSProxy();
}

inline bool JSObject::IsGeneratorObject() const
{
    return GetJSHClass()->IsGeneratorObject();
}

inline bool JSObject::IsAsyncGeneratorObject() const
{
    return GetJSHClass()->IsAsyncGeneratorObject();
}

inline bool JSObject::IsForinIterator() const
{
    return GetJSHClass()->IsForinIterator();
}

inline bool JSObject::IsJSSetIterator() const
{
    return GetJSHClass()->IsJSSetIterator();
}

inline bool JSObject::IsJSRegExpIterator() const
{
    return GetJSHClass()->IsJSRegExpIterator();
}

inline bool JSObject::IsJSMapIterator() const
{
    return GetJSHClass()->IsJSMapIterator();
}

inline bool JSObject::IsJSArrayIterator() const
{
    return GetJSHClass()->IsJSArrayIterator();
}

inline bool JSObject::IsJSAPIArrayListIterator() const
{
    return GetJSHClass()->IsJSAPIArrayListIterator();
}

inline bool JSObject::IsJSAPIStackIterator() const
{
    return GetJSHClass()->IsJSAPIStackIterator();
}

inline bool JSObject::IsJSAPIVectorIterator() const
{
    return GetJSHClass()->IsJSAPIVectorIterator();
}

inline bool JSObject::IsJSAPIBitVectorIterator() const
{
    return GetJSHClass()->IsJSAPIBitVectorIterator();
}

inline bool JSObject::IsJSAPILinkedListIterator() const
{
    return GetJSHClass()->IsJSAPILinkedListIterator();
}

inline bool JSObject::IsJSAPIListIterator() const
{
    return GetJSHClass()->IsJSAPIListIterator();
}

inline bool JSObject::IsJSPrimitiveRef() const
{
    return GetJSHClass()->IsJsPrimitiveRef();
}

inline bool JSObject::IsElementDict(const JSThread *thread) const
{
    return TaggedArray::Cast(GetElements(thread).GetTaggedObject())->IsDictionaryMode();
}

inline bool JSObject::IsPropertiesDict(const JSThread *thread) const
{
    return TaggedArray::Cast(GetProperties(thread).GetTaggedObject())->IsDictionaryMode();
}

inline bool JSObject::IsTypedArray() const
{
    return GetJSHClass()->IsTypedArray();
}

inline bool JSObject::IsSharedTypedArray() const
{
    return GetJSHClass()->IsJSSharedTypedArray();
}

std::pair<bool, JSTaggedValue> JSObject::ConvertValueWithRep(PropertyAttributes attr, JSTaggedValue value)
{
    if (attr.IsDoubleRep()) {
        if (value.IsInt()) {
            double doubleValue = value.GetInt();
            return std::pair(true, JSTaggedValue(bit_cast<JSTaggedType>(doubleValue)));
        } else if (value.IsDouble()) {
            return std::pair(true, JSTaggedValue(bit_cast<JSTaggedType>(value.GetDouble())));
        } else {
            return std::pair(false, value);
        }
    } else if (attr.IsIntRep()) {
        if (value.IsInt()) {
            int intValue = value.GetInt();
            return std::pair(true, JSTaggedValue(static_cast<JSTaggedType>(intValue)));
        } else {
            return std::pair(false, value);
        }
    }
    return std::pair(true, value);
}

void JSObject::SetPropertyInlinedPropsWithRep(const JSThread *thread, uint32_t index, JSTaggedValue value)
{
    auto layout = LayoutInfo::Cast(GetJSHClass()->GetLayout(thread).GetTaggedObject());
    auto attr = layout->GetAttr(thread, index);
    if (attr.IsTaggedRep()) {
        SetPropertyInlinedProps<true>(thread, index, value);
    } else {
        SetPropertyInlinedProps<false>(thread, index, value);
    }
}

template <size_t objectSize, uint32_t index, bool needBarrier>
void JSObject::SetPropertyInlinedPropsWithSize(const JSThread *thread, JSTaggedValue value)
{
    constexpr uint32_t offset = static_cast<uint32_t>(objectSize + index * JSTaggedValue::TaggedTypeSize());
    if constexpr (needBarrier) {
        SET_VALUE_WITH_BARRIER(thread, this, offset, value);
    } else {
        SET_VALUE_PRIMITIVE(this, offset, value);
    }
}

template <bool needBarrier>
void JSObject::SetPropertyInlinedProps(const JSThread *thread, uint32_t index, JSTaggedValue value)
{
    SetPropertyInlinedProps<needBarrier>(thread, GetJSHClass(), index, value);
}

JSTaggedValue JSObject::GetPropertyInlinedPropsWithRep(const JSThread *thread, uint32_t index,
                                                       PropertyAttributes attr) const
{
    return GetPropertyInlinedPropsWithRep(thread, GetJSHClass(), index, attr);
}

JSTaggedValue JSObject::GetPropertyInlinedPropsWithRep(const JSThread* thread, const JSHClass *hclass, uint32_t index,
                                                       PropertyAttributes attr) const
{
    auto value = GetPropertyInlinedProps(thread, hclass, index);
    if (attr.IsDoubleRep()) {
        value = JSTaggedValue(bit_cast<double>(value.GetRawData()));
    } else if (attr.IsIntRep()) {
        value = JSTaggedValue(static_cast<int32_t>(value.GetRawData()));
    }
    return value;
}

template <size_t objectSize, uint32_t index>
JSTaggedValue JSObject::GetPropertyInlinedPropsWithSize(const JSThread* thread) const
{
    constexpr uint32_t offset = static_cast<uint32_t>(objectSize + index * JSTaggedValue::TaggedTypeSize());
    return JSTaggedValue(GET_VALUE(thread, this, offset));
}

JSTaggedValue JSObject::GetPropertyInlinedProps(const JSThread* thread, uint32_t index) const
{
    return GetPropertyInlinedProps(thread, GetJSHClass(), index);
}

template <bool needBarrier>
void JSObject::SetPropertyInlinedProps(const JSThread *thread, const JSHClass *hclass, uint32_t index,
                                       JSTaggedValue value)
{
    uint32_t offset = hclass->GetInlinedPropertiesOffset(index);
    ASSERT(hclass->GetObjectSize() > offset);
    if constexpr (needBarrier) {
        SET_VALUE_WITH_BARRIER(thread, this, offset, value);
    } else {
        SET_VALUE_PRIMITIVE(this, offset, value);
    }
}

JSTaggedValue JSObject::GetPropertyInlinedProps(const JSThread* thread, const JSHClass *hclass, uint32_t index) const
{
    uint32_t offset = hclass->GetInlinedPropertiesOffset(index);
    return JSTaggedValue(GET_VALUE(thread, this, offset));
}

JSTaggedValue JSObject::GetProperty(const JSThread* thread, const JSHClass *hclass, PropertyAttributes attr) const
{
    if (attr.IsInlinedProps()) {
        return GetPropertyInlinedPropsWithRep(thread, hclass, attr.GetOffset(), attr);
    }
    TaggedArray *array = TaggedArray::Cast(GetProperties(thread).GetTaggedObject());
    return array->Get(thread, attr.GetOffset() - hclass->GetInlinedProperties());
}

template <bool needBarrier>
void JSObject::SetProperty(const JSThread *thread, const JSHClass *hclass, PropertyAttributes attr, JSTaggedValue value)
{
    if (attr.IsInlinedProps()) {
        SetPropertyInlinedProps<needBarrier>(thread, hclass, attr.GetOffset(), value);
    } else {
        TaggedArray *array = TaggedArray::Cast(GetProperties(thread).GetTaggedObject());
        array->Set<needBarrier>(thread, attr.GetOffset() - hclass->GetInlinedProperties(), value);
    }
}

inline bool JSObject::ShouldTransToDict(uint32_t capacity, uint32_t index)
{
    if (index < capacity) {
        return false;
    }

    if (index - capacity > MAX_GAP) {
        return true;
    }

    if (index >= static_cast<uint32_t>(INT32_MAX)) {
        return true;
    }

    if (capacity >= MIN_GAP) {
        return index > capacity * FAST_ELEMENTS_FACTOR;
    }

    return false;
}

inline bool JSObject::ShouldTransToFastElements(JSThread *thread, TaggedArray *elements,
                                                uint32_t capacity, uint32_t index)
{
    JSHandle<NumberDictionary> dictionary(thread, elements);
    if (index >= static_cast<uint32_t>(INT32_MAX)) {
        return false;
    }
    // Turn fast if only saves 50% space.
    if (static_cast<uint32_t>(dictionary->GetLength()) * SHOULD_TRANS_TO_FAST_ELEMENTS_FACTOR >= capacity ||
        static_cast<uint64_t>(dictionary->NextEnumerationIndex(thread)) >
        PropertyAttributes::DictionaryOrderField::MaxValue()) {
        return true;
    }
    return false;
}

inline uint32_t JSObject::ComputeElementCapacity(uint32_t oldCapacity, bool isNew)
{
    uint32_t newCapacity = isNew ? oldCapacity : (oldCapacity + (oldCapacity >> 1U));
    return newCapacity > MIN_ELEMENTS_LENGTH ? newCapacity : MIN_ELEMENTS_LENGTH;
}

inline uint32_t JSObject::ComputeElementCapacityHighGrowth(uint32_t oldCapacity)
{
    uint32_t newCapacity = oldCapacity * 2;
    return newCapacity > MIN_ELEMENTS_LENGTH ? newCapacity : MIN_ELEMENTS_LENGTH;
}

inline uint32_t JSObject::ComputeElementCapacityWithHint(uint32_t oldCapacity, uint32_t hint)
{
    uint32_t newCapacity = 0;
    if ((oldCapacity >= hint) || (hint < MIN_ELEMENTS_HINT_LENGTH) || (hint >= MAX_ELEMENTS_HINT_LENGTH)) {
        return newCapacity;
    }
    if ((hint / oldCapacity) <= ELEMENTS_HINT_FACTOR) {
        newCapacity = hint;
    }
    return newCapacity;
}

inline uint32_t JSObject::ComputeNonInlinedFastPropsCapacity(JSThread *thread, uint32_t oldCapacity,
                                                             uint32_t maxNonInlinedFastPropsCapacity)
{
    uint32_t newCapacity = oldCapacity + thread->GetPropertiesGrowStep();
    return newCapacity > maxNonInlinedFastPropsCapacity ? maxNonInlinedFastPropsCapacity : newCapacity;
}

// static
template<ElementTypes types>
JSHandle<JSTaggedValue> JSObject::CreateListFromArrayLike(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    // 3. If Type(obj) is not Object, throw a TypeError exception.
    if (!obj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "CreateListFromArrayLike must accept object",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    if (obj->IsTypedArray()) {
        uint32_t len = JSHandle<JSTypedArray>::Cast(obj)->GetArrayLength();
        JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(len);
        JSTypedArray::FastCopyElementToArray(thread, obj, array);
        // c. ReturnIfAbrupt(next).
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
        return JSHandle<JSTaggedValue>(array);
    }
    // 4. Let len be ToLength(Get(obj, "length")).
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();

    JSHandle<JSTaggedValue> value = GetProperty(thread, obj, lengthKeyHandle).GetValue();
    JSTaggedNumber number = JSTaggedValue::ToLength(thread, value);
    // 5. ReturnIfAbrupt(len).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (number.GetNumber() > MAX_ELEMENT_INDEX) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "len is bigger than 2^32 - 1",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }

    uint32_t len = number.ToUint32();
    // 6. Let list be an empty List.
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(len);
    // 8. Repeat while index < len
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue next = JSTaggedValue::GetProperty(thread, obj, i).GetValue().GetTaggedValue();
        // c. ReturnIfAbrupt(next).
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

        if constexpr (types == ElementTypes::STRING_AND_SYMBOL) {
            if (!next.IsString() && !next.IsSymbol()) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "CreateListFromArrayLike: not an element of elementTypes",
                                            JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
            }
        }

        array->Set(thread, i, next);
    }
    return JSHandle<JSTaggedValue>(array);
}

inline JSTaggedValue JSObject::ShouldGetValueFromBox(ObjectOperator *op)
{
    JSTaggedValue result = op->GetValue();
    if (result.IsPropertyBox()) {
        result = PropertyBox::Cast(result.GetTaggedObject())->GetValue(op->GetThread());
    }
    return result;
}

inline bool JSObject::CheckHClassHit(const JSHandle<JSObject> &obj, const JSHandle<JSHClass> &cls)
{
    return obj->GetJSHClass() == *cls;
}

inline uint32_t JSObject::SetValuesOrEntries(JSThread *thread, const JSHandle<TaggedArray> &prop, uint32_t index,
                                             const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                             PropertyKind kind)
{
    if (kind == PropertyKind::VALUE) {
        prop->Set(thread, index++, value);
        return index;
    }
    JSHandle<TaggedArray> keyValue = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(2);  // 2: key-value pair
    keyValue->Set(thread, 0, key);
    keyValue->Set(thread, 1, value);
    JSHandle<JSArray> entry = JSArray::CreateArrayFromList(thread, keyValue);
    prop->Set(thread, index++, entry.GetTaggedValue());
    return index;
}

inline JSTaggedValue JSObject::GetPrototype(const JSThread *thread, JSTaggedValue obj)
{
    JSHClass *hclass = obj.GetTaggedObject()->GetClass();
    return hclass->GetPrototype(thread);
}

inline bool JSObject::IsDepulicateKeys(JSThread *thread, JSHandle<TaggedArray> keys, int32_t lastLength,
                                       JSHandle<TaggedQueue> shadowQueue, JSHandle<JSTaggedValue> key)
{
    if (lastLength < 0) {
        return false;
    }
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (int32_t i = 0; i < lastLength; i++) {
        value.Update(keys->Get(thread, i));
        bool has = JSTaggedValue::Equal(thread, value, key);
        if (has) {
            return true;
        }
    }

    uint32_t shadowSize = shadowQueue->Size(thread);
    for (uint32_t i = 0; i < shadowSize; i++) {
        value.Update(shadowQueue->Get(thread, i));
        bool has = JSTaggedValue::Equal(thread, value, key);
        if (has) {
            return true;
        }
    }
    return false;
}

// For the for-in iterator, we need to determine whether to output this key based on this flag.
// So we need to reset it here.
inline void JSObject::ClearHasDeleteProperty(JSHandle<JSTaggedValue> object)
{
    object->GetTaggedObject()->GetClass()->SetHasDeleteProperty(false);
}

inline std::pair<JSHandle<TaggedArray>, JSHandle<TaggedArray>> JSObject::GetOwnEnumerableNamesInFastMode(
    JSThread *thread, const JSHandle<JSObject> &obj, uint32_t *copyLengthOfKeys, uint32_t *copyLengthOfElements)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::pair<uint32_t, uint32_t> numOfKeys = obj->GetNumberOfEnumKeys(thread);
    uint32_t numOfEnumKeys = numOfKeys.first;
    uint32_t numOfElements = obj->GetNumberOfElements(thread);
    JSHandle<TaggedArray> elementArray = numOfElements > 0 ? JSObject::GetEnumElementKeys(
        thread, obj, 0, numOfElements, copyLengthOfElements) : factory->EmptyArray();
    JSHandle<TaggedArray> keyArray = numOfEnumKeys > 0 ? JSObject::GetAllEnumKeys(
        thread, obj, numOfEnumKeys, copyLengthOfKeys) : factory->EmptyArray();
    return std::make_pair(keyArray, elementArray);
}

inline bool JSObject::HasMutantTaggedArrayElements(const JSThread *thread, const JSHandle<JSObject> &obj)
{
    return obj->GetElements(thread).IsMutantTaggedArray();
}
}  //  namespace panda::ecmascript
#endif  // ECMASCRIPT_JSOBJECT_INL_H
