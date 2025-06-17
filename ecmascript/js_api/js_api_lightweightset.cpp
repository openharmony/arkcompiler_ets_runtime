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

#include "ecmascript/js_api/js_api_lightweightset.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"

#include <codecvt>

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
bool JSAPILightWeightSet::Add(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                              const JSHandle<JSTaggedValue> &value)
{
    CheckAndCopyValues(thread, obj);
    uint32_t hashCode = obj->Hash(thread, value.GetTaggedValue());
    JSHandle<TaggedArray> hashArray(thread, obj->GetHashes(thread));
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues(thread));
    int32_t size = static_cast<int32_t>(obj->GetLength());
    int32_t index = obj->GetHashIndex(thread, value, size);
    if (index >= 0) {
        return false;
    }
    index ^= JSAPILightWeightSet::HASH_REBELLION;
    if (index < size) {
        obj->AdjustArray(thread, hashArray, index, size, true);
        obj->AdjustArray(thread, valueArray, index, size, true);
    }
    uint32_t capacity = hashArray->GetLength();
    if (size + 1 >= static_cast<int32_t>(capacity)) {
        // need expanding
        uint32_t newCapacity = capacity << 1U;
        hashArray = thread->GetEcmaVM()->GetFactory()->CopyArray(hashArray, capacity, newCapacity);
        valueArray = thread->GetEcmaVM()->GetFactory()->CopyArray(valueArray, capacity, newCapacity);
        obj->SetHashes(thread, hashArray);
        obj->SetValues(thread, valueArray);
    }
    hashArray->Set(thread, index, JSTaggedValue(hashCode));
    valueArray->Set(thread, index, value.GetTaggedValue());
    size++;
    obj->SetLength(size);
    return true;
}

JSTaggedValue JSAPILightWeightSet::Get(const JSThread *thread, const uint32_t index)
{
    TaggedArray *valueArray = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    return valueArray->Get(thread, index);
}

JSHandle<TaggedArray> JSAPILightWeightSet::CreateSlot(const JSThread *thread, const uint32_t capacity)
{
    ASSERT_PRINT(capacity > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(capacity);
    for (uint32_t i = 0; i < capacity; i++) {
        taggedArray->Set(thread, i, JSTaggedValue::Hole());
    }
    return taggedArray;
}

int32_t JSAPILightWeightSet::GetHashIndex(const JSThread *thread, const JSHandle<JSTaggedValue> &value, int32_t size)
{
    uint32_t hashCode = Hash(thread, value.GetTaggedValue());
    int32_t index = BinarySearchHashes(thread, hashCode, size);
    if (index < 0) {
        return index;
    }
    TaggedArray *valueArray = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    if (index < size && (JSTaggedValue::SameValue(thread, valueArray->Get(thread, index), value.GetTaggedValue()))) {
        return index;
    }
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    int32_t right = index;
    while (right < size && (hashArray->Get(thread, right).GetNumber() == hashCode)) {
        if (JSTaggedValue::SameValue(thread, valueArray->Get(thread, right), value.GetTaggedValue())) {
            return right;
        }
        right++;
    }
    int32_t left = index - 1;
    while (left >= 0 && ((hashArray->Get(thread, left).GetNumber() == hashCode))) {
        if (JSTaggedValue::SameValue(thread, valueArray->Get(thread, left), value.GetTaggedValue())) {
            return left;
        }
        left--;
    }
    return -right;
}

int32_t JSAPILightWeightSet::BinarySearchHashes(const JSThread *thread, uint32_t hash, int32_t size)
{
    int32_t low = 0;
    int32_t high = size - 1;
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    while (low <= high) {
        int32_t mid = (low + high) >> 1U;
        uint32_t midVal = (uint32_t)(hashArray->Get(thread, static_cast<uint32_t>(mid)).GetNumber());
        if (midVal < hash) {
            low = mid + 1;
        } else {
            if (midVal <= hash) {
                return mid;
            }
            high = mid - 1;
        }
    }
    return -(low + 1);
}

bool JSAPILightWeightSet::AddAll(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                 const JSHandle<JSTaggedValue> &value)
{
    bool changed = false;
    JSHandle<JSAPILightWeightSet> srcLightWeightSet = JSHandle<JSAPILightWeightSet>::Cast(value);
    uint32_t srcSize = srcLightWeightSet->GetSize();
    uint32_t size = obj->GetSize();
    obj->EnsureCapacity(thread, obj, size + srcSize);
    JSMutableHandle<JSTaggedValue> element(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < srcSize; i++) {
        element.Update(srcLightWeightSet->GetValueAt(thread, i));
        changed |= JSAPILightWeightSet::Add(thread, obj, element);
    }
    return changed;
}

void JSAPILightWeightSet::EnsureCapacity(const JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                         uint32_t minimumCapacity)
{
    TaggedArray *hashes = TaggedArray::Cast(obj->GetValues(thread).GetTaggedObject());
    uint32_t capacity = hashes->GetLength();
    uint32_t newCapacity = capacity;
    if (capacity > minimumCapacity) {
        return;
    }
    // adjust
    while (newCapacity <= minimumCapacity) {
        newCapacity = newCapacity << 1U;
    }
    obj->SizeCopy(thread, obj, capacity, newCapacity);
}

void JSAPILightWeightSet::SizeCopy(const JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                   uint32_t capacity, uint32_t newCapacity)
{
    JSHandle<TaggedArray> hashArray(thread, obj->GetHashes(thread));
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues(thread));
    hashArray = thread->GetEcmaVM()->GetFactory()->CopyArray(hashArray, capacity, newCapacity);
    valueArray = thread->GetEcmaVM()->GetFactory()->CopyArray(valueArray, capacity, newCapacity);

    obj->SetValues(thread, hashArray);
    obj->SetHashes(thread, valueArray);
}

bool JSAPILightWeightSet::IsEmpty()
{
    return GetLength() == 0;
}

JSTaggedValue JSAPILightWeightSet::GetValueAt(const JSThread *thread, int32_t index)
{
    int32_t size = static_cast<int32_t>(GetLength());
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    return values->Get(thread, index);
}

JSTaggedValue JSAPILightWeightSet::GetHashAt(const JSThread *thread, int32_t index)
{
    int32_t size = static_cast<int32_t>(GetLength());
    if (index < 0 || index >= size) {
        return JSTaggedValue::Undefined();
    }
    TaggedArray *values = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    return values->Get(thread, index);
}

bool JSAPILightWeightSet::HasAll(const JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    bool result = false;
    uint32_t relocate = 0;
    JSAPILightWeightSet *lightweightSet = JSAPILightWeightSet::Cast(value.GetTaggedValue().GetTaggedObject());
    uint32_t size = GetLength();
    uint32_t destSize = lightweightSet->GetLength();
    TaggedArray *hashes = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    TaggedArray *destHashes = TaggedArray::Cast(lightweightSet->GetHashes(thread).GetTaggedObject());
    if (destSize > size) {
        return result;
    }
    for (uint32_t i = 0; i < destSize; i++) {
        uint32_t destHashCode = destHashes->Get(thread, i).GetNumber();
        result = false;
        for (uint32_t j = relocate; j < size; j++) {
            uint32_t hashCode = hashes->Get(thread, j).GetNumber();
            if (destHashCode == hashCode) {
                result = true;
                relocate = j + 1;
                break;
            }
        }
        if (!result) {
            break;
        }
    }
    return result;
}

bool JSAPILightWeightSet::Has(const JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    uint32_t size = GetLength();
    int32_t index = GetHashIndex(thread, value, size);
    if (index < 0) {
        return false;
    }
    return true;
}

bool JSAPILightWeightSet::HasHash(const JSThread *thread, const JSHandle<JSTaggedValue> &hashCode)
{
    uint32_t size = GetLength();
    int32_t index = BinarySearchHashes(thread, hashCode.GetTaggedValue().GetNumber(), size);
    if (index < 0) {
        return false;
    }
    return true;
}

bool JSAPILightWeightSet::Equal(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                const JSHandle<JSTaggedValue> &value)
{
    bool result = false;
    JSHandle<TaggedArray> destHashes(thread, obj->GetValues(thread));
    uint32_t destSize = obj->GetLength();
    uint32_t srcSize = 0;
    JSMutableHandle<TaggedArray> srcHashes(thread, obj->GetHashes(thread));
    if (value.GetTaggedValue().IsJSAPILightWeightSet()) {
        JSAPILightWeightSet *srcLightWeightSet = JSAPILightWeightSet::Cast(value.GetTaggedValue().GetTaggedObject());
        srcSize = srcLightWeightSet->GetLength();
        if (srcSize == 0 || destSize == 0) {
            return false;
        }
        srcHashes.Update(srcLightWeightSet->GetHashes(thread));
    }
    if (value.GetTaggedValue().IsJSArray()) {
        srcHashes.Update(JSArray::ToTaggedArray(thread, value));
        srcSize = srcHashes->GetLength();
        if (srcSize == 0 || destSize == 0) {
            return false;
        }
    }
    if (srcSize != destSize) {
        return false;
    }
    for (uint32_t i = 0; i < destSize; i++) {
        JSTaggedValue compareValue = destHashes->Get(thread, i);
        JSTaggedValue values = srcHashes->Get(thread, i);
        if (compareValue.IsNumber() && values.IsNumber()) {
            result = JSTaggedValue::SameValueNumberic(compareValue, values);
        }
        if (compareValue.IsString() && values.IsString()) {
            result =
                JSTaggedValue::StringCompare(thread,
                                             EcmaString::Cast(compareValue.GetTaggedObject()),
                                             EcmaString::Cast(values.GetTaggedObject()));
        }
        if (!result) {
            return result;
        }
    }
    return result;
}

void JSAPILightWeightSet::IncreaseCapacityTo(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                             int32_t minCapacity)
{
    uint32_t capacity = TaggedArray::Cast(obj->GetValues(thread).GetTaggedObject())->GetLength();
    int32_t intCapacity = static_cast<int32_t>(capacity);
    if (minCapacity <= 0 || intCapacity >= minCapacity) {
        std::ostringstream oss;
        oss << "The value of \"minimumCapacity\" is out of range. It must be > " << intCapacity
            << ". Received value is: " << minCapacity;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    JSHandle<TaggedArray> hashArray(thread, obj->GetHashes(thread));
    JSHandle<TaggedArray> newElements =
        thread->GetEcmaVM()->GetFactory()->NewAndCopyTaggedArray(hashArray,
                                                                 static_cast<uint32_t>(minCapacity), capacity);
    obj->SetHashes(thread, newElements);
}

JSHandle<JSTaggedValue> JSAPILightWeightSet::GetIteratorObj(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj,
                                                            IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> iter =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSAPILightWeightSetIterator(obj, kind));
    return iter;
}

JSTaggedValue JSAPILightWeightSet::ForEach(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle,
                                           const JSHandle<JSTaggedValue> &callbackFn,
                                           const JSHandle<JSTaggedValue> &thisArg)
{
    JSHandle<JSAPILightWeightSet> lightweightset = JSHandle<JSAPILightWeightSet>::Cast(thisHandle);
    CheckAndCopyValues(thread, lightweightset);
    uint32_t length = lightweightset->GetSize();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    for (uint32_t k = 0; k < length; k++) {
        JSTaggedValue kValue = lightweightset->GetValueAt(thread, k);
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFn, thisArg, undefined, 3); // 3:three args
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        info->SetCallArg(kValue, kValue, thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        if (lightweightset->GetSize() != length) {  // prevent length change
            length = lightweightset->GetSize();
        }
    }
    return JSTaggedValue::Undefined();
}

int32_t JSAPILightWeightSet::GetIndexOf(const JSThread *thread, JSHandle<JSTaggedValue> &value)
{
    uint32_t size = GetLength();
    int32_t index = GetHashIndex(thread, value, size);
    return index;
}

JSTaggedValue JSAPILightWeightSet::Remove(JSThread *thread, JSHandle<JSTaggedValue> &value)
{
    uint32_t size = GetLength();
    TaggedArray *valueArray = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    int32_t index = GetHashIndex(thread, value, size);
    if (index < 0) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue result = valueArray->Get(thread, index);
    RemoveAt(thread, index);
    return result;
}

bool JSAPILightWeightSet::RemoveAt(JSThread *thread, int32_t index)
{
    uint32_t size = GetLength();
    if (index < 0 || index >= static_cast<int32_t>(size)) {
        return false;
    }
    JSHandle<TaggedArray> valueArray(thread, GetValues(thread));
    JSHandle<TaggedArray> hashArray(thread, GetHashes(thread));
    RemoveValue(thread, hashArray, static_cast<uint32_t>(index), true);
    RemoveValue(thread, valueArray, static_cast<uint32_t>(index));
    SetLength(size - 1);
    return true;
}

void JSAPILightWeightSet::RemoveValue(const JSThread *thread, JSHandle<TaggedArray> &taggedArray,
                                      uint32_t index, bool isHash)
{
    uint32_t len = GetLength();
    ASSERT(index < len);
    TaggedArray::RemoveElementByIndex(thread, taggedArray, index, len, isHash);
}

void JSAPILightWeightSet::AdjustArray(JSThread *thread, JSHandle<TaggedArray> srcArray, uint32_t fromIndex,
                                      uint32_t toIndex, bool direction)
{
    uint32_t size = GetLength();
    uint32_t idx = size - 1;
    if (direction) {
        while (fromIndex < toIndex) {
            JSTaggedValue value = srcArray->Get(thread, idx);
            srcArray->Set(thread, idx + 1, value);
            idx--;
            fromIndex++;
        }
    } else {
        uint32_t moveSize = size - fromIndex;
        for (uint32_t i = 0; i < moveSize; i++) {
            if ((fromIndex + i) < size) {
                JSTaggedValue value = srcArray->Get(thread, fromIndex + i);
                srcArray->Set(thread, toIndex + i, value);
            } else {
                srcArray->Set(thread, toIndex + i, JSTaggedValue::Hole());
            }
        }
    }
}

JSTaggedValue JSAPILightWeightSet::ToString(JSThread *thread, const JSHandle<JSAPILightWeightSet> &obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string sepStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(",");

    uint32_t length = obj->GetSize();
    JSHandle<TaggedArray> valueArray(thread, obj->GetValues(thread));
    std::u16string concatStr;
    JSMutableHandle<JSTaggedValue> values(thread, JSTaggedValue::Undefined());
    for (uint32_t k = 0; k < length; k++) {
        std::u16string nextStr;
        values.Update(valueArray->Get(thread, k));
        if (!values->IsUndefined() && !values->IsNull()) {
            JSHandle<EcmaString> nextStringHandle = JSTaggedValue::ToString(thread, values);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            nextStr = EcmaStringAccessor(nextStringHandle).ToU16String(thread);
        }
        if (k > 0) {
            concatStr.append(sepStr);
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

void JSAPILightWeightSet::Clear(JSThread *thread)
{
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    TaggedArray *valueArray = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    uint32_t size = GetLength();
    for (uint32_t index = 0; index < size; index++) {
        hashArray->Set(thread, index, JSTaggedValue::Hole());
        valueArray->Set(thread, index, JSTaggedValue::Hole());
    }
    SetLength(0);
}

uint32_t JSAPILightWeightSet::Hash(const JSThread *thread, JSTaggedValue key)
{
    if (key.IsDouble() && key.GetDouble() == 0.0) {
        key = JSTaggedValue(0);
    }
    if (key.IsSymbol()) {
        auto symbolString = JSSymbol::Cast(key.GetTaggedObject());
        return symbolString->GetHashField();
    }
    if (key.IsString()) {
        auto keyString = EcmaString::Cast(key.GetTaggedObject());
        return EcmaStringAccessor(keyString).GetHashcode(thread);
    }
    if (key.IsECMAObject()) {
        uint32_t hash = static_cast<uint32_t>(ECMAObject::Cast(key.GetTaggedObject())->GetHash(thread));
        if (hash == 0) {
            hash = static_cast<uint32_t>(base::RandomGenerator::GenerateIdentityHash());
            JSHandle<ECMAObject> ecmaObj(thread, key);
            ECMAObject::SetHash(thread, hash, ecmaObj);
        }
        return hash;
    }
    if (key.IsInt()) {
        uint32_t hash = static_cast<uint32_t>(key.GetInt());
        return hash;
    }
    uint64_t keyValue = key.GetRawData();
    return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
}

void JSAPILightWeightSet::CheckAndCopyValues(const JSThread *thread, JSHandle<JSAPILightWeightSet> obj)
{
    JSHandle<TaggedArray> values(thread, obj->GetValues(thread));
    // Check whether array is shared in the nonmovable space before set properties and elements.
    // If true, then really copy array in the semi space.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (values.GetTaggedValue().IsCOWArray()) {
        auto newArray = factory->CopyArray(values, values->GetLength(), values->GetLength(),
            JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        obj->SetValues(thread, newArray.GetTaggedValue());
    }
}
} // namespace panda::ecmascript
