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

#include "ecmascript/js_api/js_api_vector.h"

#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"

#include <codecvt>
#include <locale>

namespace panda::ecmascript {
static const uint32_t MAX_VALUE = 0x7fffffff;
static const uint32_t MAX_ARRAY_SIZE = MAX_VALUE - 8;
bool JSAPIVector::Add(JSThread *thread, const JSHandle<JSAPIVector> &vector, const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = vector->GetSize();
    GrowCapacity(thread, vector, length + 1);

    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    elements->Set(thread, length, value);
    vector->SetLength(++length);

    return true;
}

void JSAPIVector::Insert(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                         const JSHandle<JSTaggedValue> &value, int32_t index)
{
    uint32_t length = vector->GetSize();
    if (index < 0 || index > static_cast<int32_t>(length)) {
        THROW_ERROR(thread, ErrorType::RANGE_ERROR, "the index is out-of-bounds");
    }
    GrowCapacity(thread, vector, length + 1);

    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    for (int32_t i = static_cast<int32_t>(length) - 1; i >= index; i--) {
        elements->Set(thread, i + 1, elements->Get(i));
    }

    elements->Set(thread, index, value);
    vector->SetLength(++length);
}

void JSAPIVector::SetLength(JSThread *thread, const JSHandle<JSAPIVector> &vector, uint32_t newSize)
{
    uint32_t len = vector->GetSize();
    if (newSize > len) {
        GrowCapacity(thread, vector, newSize);
    }
    vector->SetLength(newSize);
}

uint32_t JSAPIVector::GetCapacity()
{
    TaggedArray *elementData = TaggedArray::Cast(GetElements().GetTaggedObject());
    ASSERT(!elementData->IsDictionaryMode());
    return elementData->GetLength();
}

void JSAPIVector::IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPIVector> &vector, int32_t newCapacity)
{
    if (newCapacity < 0) {
        THROW_ERROR(thread, ErrorType::RANGE_ERROR, "An incorrect size was set");
    }

    JSHandle<TaggedArray> elementData(thread, vector->GetElements());
    ASSERT(!elementData->IsDictionaryMode());
    uint32_t oldCapacity = elementData->GetLength();
    uint32_t tempCapacity = static_cast<uint32_t>(newCapacity);
    if (oldCapacity < tempCapacity) {
        JSHandle<TaggedArray> newElements =
            thread->GetEcmaVM()->GetFactory()->CopyArray(elementData, oldCapacity, tempCapacity);
        vector->SetElements(thread, newElements);
    }
}

int32_t JSAPIVector::GetIndexOf(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                const JSHandle<JSTaggedValue> &obj)
{
    return JSAPIVector::GetIndexFrom(thread, vector, obj, 0);
}

int32_t JSAPIVector::GetIndexFrom(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                  const JSHandle<JSTaggedValue> &obj, int32_t index)
{
    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    uint32_t length = vector->GetSize();
    if (index < 0) {
        index = 0;
    } else if (index >= static_cast<int32_t>(length)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "no-such-element", -1);
    }

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = static_cast<uint32_t>(index); i < length; i++) {
        value.Update(JSTaggedValue(elements->Get(i)));
        if (JSTaggedValue::StrictEqual(thread, obj, value)) {
            return i;
        }
    }
    return -1;
}

bool JSAPIVector::IsEmpty() const
{
    return GetSize() == 0;
}

JSTaggedValue JSAPIVector::GetLastElement()
{
    uint32_t length = GetSize();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    return elements->Get(length - 1);
}

int32_t JSAPIVector::GetLastIndexOf(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                    const JSHandle<JSTaggedValue> &obj)
{
    int32_t index = static_cast<int32_t>(vector->GetSize()) - 1;
    if (index < 0) {
        return -1; // vector isEmpty, defalut return -1
    }
    return JSAPIVector::GetLastIndexFrom(thread, vector, obj, index);
}

int32_t JSAPIVector::GetLastIndexFrom(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                      const JSHandle<JSTaggedValue> &obj, int32_t index)
{
    uint32_t length = vector->GetSize();
    if (index >= static_cast<int32_t>(length)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "index-out-of-bounds", -1);
    } else if (index < 0) {
        index = 0;
    }
    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (int32_t i = index; i >= 0; i--) {
        value.Update(elements->Get(i));
        if (JSTaggedValue::StrictEqual(thread, obj, value)) {
            return i;
        }
    }
    return -1;
}

bool JSAPIVector::Remove(JSThread *thread, const JSHandle<JSAPIVector> &vector, const JSHandle<JSTaggedValue> &obj)
{
    int32_t index = GetIndexOf(thread, vector, obj);
    uint32_t length = vector->GetSize();
    if (index >= 0) {
        JSHandle<TaggedArray> elements(thread, vector->GetElements());
        ASSERT(!elements->IsDictionaryMode());
        TaggedArray::RemoveElementByIndex(thread, elements, index, length);
        length--;
        vector->SetLength(length);
        return true;
    }
    return false;
}

JSTaggedValue JSAPIVector::RemoveByIndex(JSThread *thread, const JSHandle<JSAPIVector> &vector, int32_t index)
{
    uint32_t length = vector->GetSize();
    if (index < 0 || index >= static_cast<int32_t>(length)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }
    TaggedArray *resElements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    ASSERT(!resElements->IsDictionaryMode());
    JSTaggedValue oldValue = resElements->Get(index);

    if (index >= 0) {
        JSHandle<TaggedArray> elements(thread, vector->GetElements());
        ASSERT(!elements->IsDictionaryMode() && length > 0);
        TaggedArray::RemoveElementByIndex(thread, elements, index, length);
        vector->SetLength(length - 1);
    }
    length--;
    vector->SetLength(length);
    return oldValue;
}

JSTaggedValue JSAPIVector::RemoveByRange(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                         int32_t fromIndex, int32_t toIndex)
{
    int32_t length = static_cast<int32_t>(vector->GetSize());
    if (toIndex <= fromIndex) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the fromIndex cannot be less than or equal to toIndex",
                                     JSTaggedValue::Exception());
    }
    if (fromIndex < 0 || fromIndex >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the fromIndex or the toIndex is out-of-bounds",
                                     JSTaggedValue::Exception());
    }

    int32_t endIndex = toIndex >= length ? length : toIndex;
    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    int32_t numMoved = length - endIndex;
    for (int32_t i = 0; i < numMoved; i++) {
        elements->Set(thread, fromIndex + i, elements->Get(endIndex + i));
    }

    int32_t newLength = length - (endIndex - fromIndex);
    elements->Trim(thread, newLength);
    vector->SetLength(newLength);
    return JSTaggedValue::True();
}

JSHandle<JSAPIVector> JSAPIVector::SubVector(JSThread *thread, const JSHandle<JSAPIVector> &vector,
                                             int32_t fromIndex, int32_t toIndex)
{
    int32_t length = static_cast<int32_t>(vector->GetSize());
    if (fromIndex < 0 || toIndex < 0 ||
        fromIndex >= length || toIndex >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the fromIndex or the toIndex is out-of-bounds",
                                     JSHandle<JSAPIVector>());
    }
    if (toIndex <= fromIndex) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the fromIndex cannot be less than or equal to toIndex",
                                     JSHandle<JSAPIVector>());
    }

    uint32_t newLength = static_cast<uint32_t>(toIndex - fromIndex);
    JSHandle<JSAPIVector> subVector = thread->GetEcmaVM()->GetFactory()->NewJSAPIVector(newLength);
    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());

    subVector->SetLength(newLength);
    for (uint32_t i = 0; i < newLength; i++) {
        subVector->Set(thread, i, elements->Get(fromIndex + i));
    }

    return subVector;
}

JSTaggedValue JSAPIVector::ToString(JSThread *thread, const JSHandle<JSAPIVector> &vector)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string sepHandle = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(",");

    uint32_t length = vector->GetSize();
    std::u16string concatStr;
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t k = 0; k < length; k++) {
        std::u16string nextStr;
        element.Update(Get(thread, vector, k));
        if (!element->IsUndefined() && !element->IsNull()) {
            JSHandle<EcmaString> nextStringHandle = JSTaggedValue::ToString(thread, element);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            nextStr = EcmaStringAccessor(nextStringHandle).ToU16String();
        }
        if (k > 0) {
            concatStr.append(sepHandle);
            concatStr.append(nextStr);
            continue;
        }
        concatStr.append(nextStr);
    }

    char16_t *char16tData = concatStr.data();
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = concatStr.size();
    return factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
}

JSTaggedValue JSAPIVector::ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                   const JSHandle<JSTaggedValue> &callbackFn,
                                   const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(thisHandle);
    uint32_t length = vector->GetSize();
    JSTaggedValue key = JSTaggedValue::Undefined();
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = NUM_MANDATORY_JSFUNC_ARGS;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();

    for (uint32_t k = 0; k < length; k++) {
        kValue.Update(Get(thread, vector, k));
        key = JSTaggedValue(k);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFn, thisArg, undefined, argsLength);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        info->SetCallArg(kValue.GetTaggedValue(), key, thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        if (length != vector->GetSize()) {  // prevent length change
            length = vector->GetSize();
        }
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIVector::ReplaceAllElements(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                              const JSHandle<JSTaggedValue> &callbackFn,
                                              const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(thisHandle);
    uint32_t length = vector->GetSize();
    JSTaggedValue key = JSTaggedValue::Undefined();
    JSMutableHandle<JSTaggedValue> kValue(thread, JSTaggedValue::Undefined());
    const uint32_t argsLength = NUM_MANDATORY_JSFUNC_ARGS;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();

    for (uint32_t k = 0; k < length; k++) {
        kValue.Update(Get(thread, vector, k));
        key = JSTaggedValue(k);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFn, thisArg, undefined, argsLength);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        info->SetCallArg(kValue.GetTaggedValue(), key, thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        if (length != vector->GetSize()) {  // prevent length change
            length = vector->GetSize();
            if (k >= length) {
                break;
            }
        }
        vector->Set(thread, k, funcResult);
    }

    return JSTaggedValue::Undefined();
}

void JSAPIVector::GrowCapacity(JSThread *thread, const JSHandle<JSAPIVector> &vector, uint32_t minCapacity)
{
    JSHandle<TaggedArray> elementData(thread, vector->GetElements());
    ASSERT(!elementData->IsDictionaryMode());
    uint32_t curCapacity = elementData->GetLength();
    if (minCapacity > curCapacity) {
        uint32_t oldCapacity = elementData->GetLength();
        // 2 : 2 Capacity doubled
        uint32_t newCapacity = oldCapacity * 2;
        if (newCapacity < minCapacity) {
            newCapacity = minCapacity;
        }

        if (newCapacity > MAX_ARRAY_SIZE) {
            newCapacity = (minCapacity > MAX_ARRAY_SIZE) ? MAX_VALUE : MAX_ARRAY_SIZE;
        }
        JSHandle<TaggedArray> newElements =
            thread->GetEcmaVM()->GetFactory()->CopyArray(elementData, oldCapacity, newCapacity);

        vector->SetElements(thread, newElements);
    }
}

JSTaggedValue JSAPIVector::Get(JSThread *thread, const JSHandle<JSAPIVector> &vector, int32_t index)
{
    uint32_t len = vector->GetSize();
    if (index < 0 || index >= static_cast<int32_t>(len)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "the index is out-of-bounds", JSTaggedValue::Exception());
    }

    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    return elements->Get(index);
}

JSTaggedValue JSAPIVector::Set(JSThread *thread, int32_t index, const JSTaggedValue &value)
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    elements->Set(thread, index, value);
    return JSTaggedValue::Undefined();
}

bool JSAPIVector::Has(const JSTaggedValue &value) const
{
    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    uint32_t length = GetSize();
    if (length == 0) {
        return false;
    }

    for (uint32_t i = 0; i < length; i++) {
        if (JSTaggedValue::SameValue(elements->Get(i), value)) {
            return true;
        }
    }
    return false;
}

JSHandle<TaggedArray> JSAPIVector::OwnKeys(JSThread *thread, const JSHandle<JSAPIVector> &obj)
{
    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

JSHandle<TaggedArray> JSAPIVector::OwnEnumKeys(JSThread *thread, const JSHandle<JSAPIVector> &obj)
{
    return JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>::Cast(obj));
}

bool JSAPIVector::GetOwnProperty(JSThread *thread, const JSHandle<JSAPIVector> &obj,
                                 const JSHandle<JSTaggedValue> &key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not obtain attributes of no-number type", false);
    }

    uint32_t length = obj->GetSize();
    if (index >= length) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetOwnProperty index out-of-bounds", false);
    }

    JSAPIVector::Get(thread, obj, index);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return true;
}

void JSAPIVector::TrimToCurrentLength(JSThread *thread, const JSHandle<JSAPIVector> &obj)
{
    uint32_t length = obj->GetSize();
    uint32_t capacity = obj->GetCapacity();
    TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetTaggedObject());
    ASSERT(!elements->IsDictionaryMode());
    if (capacity > length) {
        elements->Trim(thread, length);
    }
}

void JSAPIVector::Clear(JSThread *thread, const JSHandle<JSAPIVector> &obj)
{
    uint32_t length = obj->GetLength();
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    ASSERT(!elements->IsDictionaryMode());
    for (uint32_t i = 0; i < length; ++i) {
        elements->Set(thread, i, JSTaggedValue::Hole());
    }
    obj->SetLength(0);
}

JSHandle<JSAPIVector> JSAPIVector::Clone(JSThread *thread, const JSHandle<JSAPIVector> &obj)
{
    JSHandle<TaggedArray> srcElements(thread, obj->GetElements());
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIVector> newVector = factory->NewJSAPIVector(0);

    uint32_t length = obj->GetSize();
    newVector->SetLength(length);

    JSHandle<TaggedArray> dstElements = factory->NewAndCopyTaggedArray(srcElements, length, length);
    newVector->SetElements(thread, dstElements);
    return newVector;
}

JSTaggedValue JSAPIVector::GetFirstElement(const JSHandle<JSAPIVector> &vector)
{
    uint32_t length = vector->GetSize();
    if (length == 0) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *elements = TaggedArray::Cast(vector->GetElements().GetTaggedObject());
    return elements->Get(0);
}

JSTaggedValue JSAPIVector::GetIteratorObj(JSThread *thread, const JSHandle<JSAPIVector> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIVectorIterator> iter(factory->NewJSAPIVectorIterator(obj));

    return iter.GetTaggedValue();
}

OperationResult JSAPIVector::GetProperty(JSThread *thread, const JSHandle<JSAPIVector> &obj,
                                         const JSHandle<JSTaggedValue> &key)
{
    uint32_t length = obj->GetSize();
    int index = key->GetInt();
    if (index < 0 || index >= static_cast<int>(length)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "GetProperty index out-of-bounds",
                                     OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }

    return OperationResult(thread, JSAPIVector::Get(thread, obj, index), PropertyMetaData(false));
}

bool JSAPIVector::SetProperty(JSThread *thread, const JSHandle<JSAPIVector> &obj,
                              const JSHandle<JSTaggedValue> &key,
                              const JSHandle<JSTaggedValue> &value)
{
    uint32_t length = obj->GetSize();
    int index = key->GetInt();
    if (index < 0 || index >= static_cast<int>(length)) {
        return false;
    }

    obj->Set(thread, index, value.GetTaggedValue());
    return true;
}
} // namespace panda::ecmascript
