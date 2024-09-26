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

#include "ecmascript/js_api/js_api_bitvector.h"

#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
bool JSAPIBitVector::Push(
    JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, const JSHandle<JSTaggedValue>& value)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    uint32_t length = static_cast<uint32_t>(bitVector->GetLength());
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    if ((length & TAGGED_VALUE_BIT_OFFSET) == 0) {
        std::bitset<BIT_SET_LENGTH> increaseSet;
        if (!value->IsZero()) {
            increaseSet.set(0);
        }
        elements->push_back(increaseSet);
    } else {
        SetBit(elements, length, value.GetTaggedValue());
    }
    bitVector->SetLength(++length);
    return true;
}

JSTaggedValue JSAPIBitVector::Pop(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    if (bitVector->GetLength() <= 0) {
        return JSTaggedValue::Undefined();
    }
    uint32_t lastIndex = static_cast<uint32_t>(bitVector->GetLength() - 1);
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());

    JSTaggedValue bit = GetBit(elements, lastIndex);
    if ((lastIndex & TAGGED_VALUE_BIT_OFFSET) == 0) {
        elements->pop_back();
    } else {
        SetBit(elements, lastIndex, JSTaggedValue(0));
    }
    bitVector->SetLength(lastIndex);
    return bit;
}

JSTaggedValue JSAPIBitVector::Set(JSThread* thread, const uint32_t index, JSTaggedValue value)
{
    uint32_t length = static_cast<uint32_t>(GetLength());
    if (index >= length) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSNativePointer* np = JSNativePointer::Cast(GetNativePointer().GetTaggedObject());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    SetBit(elements, index, value);
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIBitVector::Get(JSThread* thread, const uint32_t index)
{
    uint32_t length = static_cast<uint32_t>(GetLength());
    if (index >= length) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSNativePointer* np = JSNativePointer::Cast(GetNativePointer().GetTaggedObject());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    return GetBit(elements, index);
}

bool JSAPIBitVector::Has(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }
    if (length == 0) {
        return false;
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (int index = startIndex; index <= endIndex; index++) {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        if ((value->IsZero() && (*elements)[elementId].test(bitId) == 0) ||
            (!value->IsZero() && (*elements)[elementId].test(bitId) != 0)) {
            return true;
        }
    }
    return false;
}

bool JSAPIBitVector::Has(const JSTaggedValue& value) const
{
    uint32_t length = GetSize();
    if (length == 0) {
        return false;
    }
    JSNativePointer* np = JSNativePointer::Cast(GetNativePointer().GetTaggedObject());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (uint32_t index = 0; index < length; index++) {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        if ((value.IsZero() && (*elements)[elementId].test(bitId) == 0) ||
            (!value.IsZero() && (*elements)[elementId].test(bitId) != 0)) {
            return true;
        }
    }
    return false;
}

JSTaggedValue JSAPIBitVector::SetBitsByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (int32_t index = startIndex; index < endIndex; index++) {
        SetBit(elements, index, value.GetTaggedValue());
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIBitVector::GetBitsByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    int32_t dstLength = endIndex - startIndex;
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSNativePointer> srcNp(thread, bitVector->GetNativePointer());
    auto srcElements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(srcNp->GetExternalPointer());

    JSHandle<JSAPIBitVector> newBitVector = factory->NewJSAPIBitVector(dstLength);
    JSHandle<JSNativePointer> dstNp(thread, newBitVector->GetNativePointer());
    auto dstElements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(dstNp->GetExternalPointer());

    for (int32_t index = 0; index < dstLength; index++) {
        JSTaggedValue value = GetBit(srcElements, index + startIndex);
        SetBit(dstElements, index, value);
    }
    newBitVector->SetLength(dstLength);
    newBitVector->SetNativePointer(thread, dstNp);

    return newBitVector.GetTaggedValue();
}

JSTaggedValue JSAPIBitVector::SetAllBits(
    JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, const JSHandle<JSTaggedValue>& value)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    int size = static_cast<int>(elements->size());
    if (value->IsZero()) {
        for (int index = 0; index < size; index++) {
            (*elements)[index] = std::bitset<BIT_SET_LENGTH>(0);
        }
    } else {
        for (int index = 0; index < size; index++) {
            (*elements)[index] = std::bitset<BIT_SET_LENGTH>(UINT64_MAX);
        }
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIBitVector::GetBitCountByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector> scope(thread, JSHandle<JSTaggedValue>::Cast(bitVector));
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    int32_t count = 0;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (int32_t index = startIndex; index < endIndex; index++) {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        if ((value->IsZero() && (*elements)[elementId].test(bitId) == 0) ||
            (!value->IsZero() && (*elements)[elementId].test(bitId) != 0)) {
            count++;
        }
    }
    return JSTaggedValue(count);
}

int JSAPIBitVector::GetIndexOf(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector> scope(thread, JSHandle<JSTaggedValue>::Cast(bitVector));
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, -1);
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, -1);
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (int32_t index = startIndex; index < endIndex; index++) {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        if ((value->IsZero() && (*elements)[elementId].test(bitId) == 0) ||
            (!value->IsZero() && (*elements)[elementId].test(bitId) != 0)) {
            return index;
        }
    }
    return -1;
}

int JSAPIBitVector::GetLastIndexOf(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector> scope(thread, JSHandle<JSTaggedValue>::Cast(bitVector));
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, -1);
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, -1);
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (int32_t index = endIndex - 1; index >= startIndex; index--) {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        if ((value->IsZero() && (*elements)[elementId].test(bitId) == 0) ||
            (!value->IsZero() && (*elements)[elementId].test(bitId) != 0)) {
            return index;
        }
    }
    return -1;
}

JSTaggedValue JSAPIBitVector::FlipBitByIndex(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, int index)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    if (index >= bitVector->GetLength()) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (bitVector->GetLength() - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());

    std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
    uint32_t elementId = pair.first;
    uint32_t bitId = pair.second;
    (*elements)[elementId].flip(bitId);
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSAPIBitVector::FlipBitsByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
    const JSHandle<JSTaggedValue>& start, const JSHandle<JSTaggedValue>& end)
{
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    int32_t startIndex = JSTaggedValue::ToInt32(thread, start);
    int32_t endIndex = JSTaggedValue::ToInt32(thread, end);
    int32_t length = bitVector->GetLength();
    int32_t size = length > endIndex ? endIndex : length;
    if (endIndex < 0 || endIndex > length) {
        std::ostringstream oss;
        oss << "The value of \"toIndex\" is out of range. It must be >= 0 && <= " << length
            << ". Received value is: " << endIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (startIndex < 0 || startIndex >= size) {
        std::ostringstream oss;
        oss << "The value of \"fromIndex\" is out of range. It must be >= 0 && <= " << (size - 1)
            << ". Received value is: " << startIndex;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    for (int32_t index = startIndex; index < endIndex; index++) {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        (*elements)[elementId].flip(bitId);
    }
    return JSTaggedValue::Undefined();
}

void JSAPIBitVector::Resize(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, int newSize)
{
    if (newSize < 0) {
        std::ostringstream oss;
        oss << "The value of \"length\" is out of range. It must be >= 0" << ". Received value is: " << newSize;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    [[maybe_unused]] ConcurrentApiScope<JSAPIBitVector, ModType::WRITE> scope(thread,
        JSHandle<JSTaggedValue>::Cast(bitVector));
    int length = bitVector->GetLength();
    uint32_t elementsLength = static_cast<uint32_t>((length - 1) / BIT_SET_LENGTH + 1);
    uint32_t newElementsLength = static_cast<uint32_t>((newSize - 1) / BIT_SET_LENGTH + 1);

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements = reinterpret_cast<std::vector<std::bitset<BIT_SET_LENGTH>>*>(np->GetExternalPointer());
    if (elementsLength == newElementsLength && length < newSize) {
        for (int32_t index = length; index < newSize; index++) {
            SetBit(elements, index, JSTaggedValue(0));
        }
    } else if (elementsLength < newElementsLength) {
        std::bitset<JSAPIBitVector::BIT_SET_LENGTH> initBitSet;
        elements->resize(newElementsLength, initBitSet);
    } else if (elementsLength > newElementsLength) {
        elements->resize(newElementsLength);
    }
    bitVector->SetLength(newSize);
}

JSHandle<TaggedArray> JSAPIBitVector::OwnKeys(JSThread* thread, const JSHandle<JSAPIBitVector>& obj)
{
    uint32_t numOfElements = static_cast<uint32_t>(obj->GetLength());
    JSHandle<TaggedArray> keyArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(numOfElements);

    if (numOfElements > 0) {
        for (uint32_t i = 0; i < numOfElements; ++i) {
            auto key = base::NumberHelper::IntToEcmaString(thread, i);
            keyArray->Set(thread, i, key);
        }
    }

    return keyArray;
}

JSHandle<TaggedArray> JSAPIBitVector::OwnEnumKeys(JSThread* thread, const JSHandle<JSAPIBitVector>& obj)
{
    return OwnKeys(thread, obj);
}

bool JSAPIBitVector::GetOwnProperty(
    JSThread* thread, const JSHandle<JSAPIBitVector>& obj, const JSHandle<JSTaggedValue>& key)
{
    uint32_t index = 0;
    if (UNLIKELY(!JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        CString errorMsg = "The type of \"index\" can not obtain attributes of no-number type. Received value is: " +
                           ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    uint32_t length = static_cast<uint32_t>(obj->GetLength());
    if (index >= length) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be > " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, false);
    }

    obj->Get(thread, index);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    return true;
}

JSTaggedValue JSAPIBitVector::GetIteratorObj(JSThread* thread, const JSHandle<JSAPIBitVector>& obj)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIBitVectorIterator> iter(factory->NewJSAPIBitVectorIterator(obj));

    return iter.GetTaggedValue();
}

OperationResult JSAPIBitVector::GetProperty(
    JSThread* thread, const JSHandle<JSAPIBitVector>& obj, const JSHandle<JSTaggedValue>& key)
{
    int length = obj->GetLength();
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(
            thread, error, OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }

    return OperationResult(thread, obj->Get(thread, index), PropertyMetaData(false));
}
bool JSAPIBitVector::SetProperty(JSThread* thread, const JSHandle<JSAPIBitVector>& obj,
    const JSHandle<JSTaggedValue>& key, const JSHandle<JSTaggedValue>& value)
{
    int length = obj->GetLength();
    int index = key->GetInt();
    if (index < 0 || index >= length) {
        return false;
    }

    obj->Set(thread, index, value.GetTaggedValue());
    return true;
}

} // namespace panda::ecmascript
