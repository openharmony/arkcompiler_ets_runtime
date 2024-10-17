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

#include "ecmascript/js_api/js_api_lightweightmap.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_object-inl.h"

#include <codecvt>

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPILightWeightMap::IncreaseCapacityTo(JSThread *thread,
                                                      const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                                      int32_t index)
{
    uint32_t num = lightWeightMap->GetSize();
    if (index < DEFAULT_CAPACITY_LENGTH || static_cast<int32_t>(num) >= index) {
        return JSTaggedValue::False();
    }
    JSHandle<TaggedArray> hashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    JSHandle<TaggedArray> newHashArray = GrowCapacity(thread, hashArray, index);
    JSHandle<TaggedArray> newKeyArray = GrowCapacity(thread, keyArray, index);
    JSHandle<TaggedArray> newValueArray = GrowCapacity(thread, valueArray, index);
    lightWeightMap->SetHashes(thread, newHashArray);
    lightWeightMap->SetKeys(thread, newKeyArray);
    lightWeightMap->SetValues(thread, newValueArray);
    return JSTaggedValue::True();
}

void JSAPILightWeightMap::InsertValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                      int32_t index, const JSHandle<JSTaggedValue> &value, AccossorsKind kind)
{
    JSHandle<TaggedArray> array = GetArrayByKind(thread, lightWeightMap, kind);
    uint32_t len = lightWeightMap->GetSize();
    JSHandle<TaggedArray> newArray = GrowCapacity(thread, array, len + 1);
    TaggedArray::InsertElementByIndex(thread, newArray, value, index, len);
    SetArrayByKind(thread, lightWeightMap, newArray, kind);
}

void JSAPILightWeightMap::ReplaceValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                       int32_t index, const JSHandle<JSTaggedValue> &value, AccossorsKind kind)
{
    JSHandle<TaggedArray> array = GetArrayByKind(thread, lightWeightMap, kind);
    ASSERT(0 <= index || index < static_cast<int32_t>(lightWeightMap->GetSize()));
    array->Set(thread, index, value.GetTaggedValue());
}

void JSAPILightWeightMap::RemoveValue(const JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                      uint32_t index, AccossorsKind kind)
{
    JSHandle<TaggedArray> array = GetArrayByKind(thread, lightWeightMap, kind);
    uint32_t len = lightWeightMap->GetLength();
    ASSERT(index < len);
    TaggedArray::RemoveElementByIndex(thread, array, index, len);
}

void JSAPILightWeightMap::Set(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                              const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    KeyState keyState = GetStateOfKey(thread, lightWeightMap, key);
    int32_t index = keyState.index;
    if (keyState.existed) {
        ReplaceValue(thread, lightWeightMap, index, value, AccossorsKind::VALUE);
    } else {
        JSHandle<JSTaggedValue> hashHandle(thread, JSTaggedValue(keyState.hash));
        InsertValue(thread, lightWeightMap, index, hashHandle, AccossorsKind::HASH);
        InsertValue(thread, lightWeightMap, index, key, AccossorsKind::KEY);
        InsertValue(thread, lightWeightMap, index, value, AccossorsKind::VALUE);
        lightWeightMap->SetLength(lightWeightMap->GetLength() + 1);
    }
}

JSTaggedValue JSAPILightWeightMap::Get(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                       const JSHandle<JSTaggedValue> &key)
{
    int32_t index = GetIndexOfKey(thread, lightWeightMap, key);
    if (index < 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    return valueArray->Get(index);
}

JSTaggedValue JSAPILightWeightMap::HasAll(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          const JSHandle<JSAPILightWeightMap> &newLightWeightMap)
{
    uint32_t length = newLightWeightMap->GetSize();
    uint32_t len = lightWeightMap->GetSize();
    if (length > len) {
        return JSTaggedValue::False();
    }
    JSHandle<TaggedArray> oldHashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    JSHandle<TaggedArray> oldKeyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> oldValueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    JSHandle<TaggedArray> newKeyArray = GetArrayByKind(thread, newLightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> newValueArray = GetArrayByKind(thread, newLightWeightMap, AccossorsKind::VALUE);
    JSTaggedValue dealKey = JSTaggedValue::Undefined();
    int32_t index = -1;
    int32_t hash = 0;

    for (uint32_t num = 0; num < length; num++) {
        dealKey = newKeyArray->Get(num);
        hash = Hash(thread, dealKey);
        index = BinarySearchHashes(oldHashArray, hash, static_cast<int32_t>(len));
        if (index < 0 || index >= static_cast<int32_t>(len)) {
            return JSTaggedValue::False();
        }
        HashParams params { oldHashArray, oldKeyArray, &dealKey };
        index = AvoidHashCollision(params, index, len, hash);
        if (!JSTaggedValue::SameValue(oldKeyArray->Get(index), dealKey) ||
            !JSTaggedValue::SameValue(oldValueArray->Get(index), newValueArray->Get(num))) {
            // avoid Hash collision
            return JSTaggedValue::False();
        }
    }
    return JSTaggedValue::True();
}

JSTaggedValue JSAPILightWeightMap::HasKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          const JSHandle<JSTaggedValue> &key)
{
    KeyState keyState = GetStateOfKey(thread, lightWeightMap, key);
    return keyState.existed ? JSTaggedValue::True() : JSTaggedValue::False();
}

JSTaggedValue JSAPILightWeightMap::HasValue(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                            const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    uint32_t length = lightWeightMap->GetSize();
    for (uint32_t num = 0; num < length; num++) {
        if (JSTaggedValue::SameValue(valueArray->Get(num), value.GetTaggedValue())) {
            return JSTaggedValue::True();
        }
    }
    return JSTaggedValue::False();
}

int32_t JSAPILightWeightMap::GetIndexOfKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                           const JSHandle<JSTaggedValue> &key)
{
    KeyState keyState = GetStateOfKey(thread, lightWeightMap, key);
    return keyState.existed ? keyState.index : -1;
}

KeyState JSAPILightWeightMap::GetStateOfKey(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                            const JSHandle<JSTaggedValue> &key)
{
    int32_t hash = Hash(thread, key.GetTaggedValue());
    int32_t length = static_cast<int32_t>(lightWeightMap->GetSize());
    JSHandle<TaggedArray> hashArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::HASH);
    int32_t index = BinarySearchHashes(hashArray, hash, length);
    if (index >= 0) {
        // avoid Hash Collision
        JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
        int32_t right = index;
        while ((right < length) && (hashArray->Get(right).GetInt() == hash)) {
            if (JSTaggedValue::SameValue(keyArray->Get(right), key.GetTaggedValue())) {
                return KeyState {true, hash, right};
            }
            right++;
        }
        int32_t left = index - 1;
        while ((left >= 0) && ((hashArray->Get(left).GetInt() == hash))) {
            if (JSTaggedValue::SameValue(keyArray->Get(left), key.GetTaggedValue())) {
                return KeyState {true, hash, left};
            }
            left--;
        }
        return KeyState {false, hash, right}; // first index whose element is bigger than hash
    }
    return KeyState {false, hash, index ^ HASH_REBELLION}; // first index whose element is bigger than hash
}

int32_t JSAPILightWeightMap::GetIndexOfValue(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                             const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    uint32_t length = lightWeightMap->GetSize();
    JSTaggedValue compValue = value.GetTaggedValue();
    for (uint32_t i = 0; i < length; i++) {
        if (valueArray->Get(i) == compValue) {
            return i;
        }
    }
    return -1; // not find, default return -1
}

JSTaggedValue JSAPILightWeightMap::GetKeyAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                            int32_t index)
{
    int32_t length = static_cast<int32_t>(lightWeightMap->GetSize());
    if (length <= 0) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, "Container is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (index < 0 || length <= index) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<TaggedArray> keyArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::KEY);
    return keyArray->Get(index);
}

JSTaggedValue JSAPILightWeightMap::GetValueAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                              int32_t index)
{
    int32_t length = static_cast<int32_t>(lightWeightMap->GetSize());
    if (length <= 0) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, "Container is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (index < 0 || length <= index) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    return valueArray->Get(index);
}

void JSAPILightWeightMap::SetAll(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                 const JSHandle<JSAPILightWeightMap> &needLightWeightMap)
{
    JSHandle<TaggedArray> needKeyArray = GetArrayByKind(thread, needLightWeightMap, AccossorsKind::KEY);
    JSHandle<TaggedArray> needValueArray = GetArrayByKind(thread, needLightWeightMap, AccossorsKind::VALUE);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    uint32_t length = needLightWeightMap->GetSize();
    for (uint32_t num = 0; num < length; num++) {
        key.Update(needKeyArray->Get(num));
        value.Update(needValueArray->Get(num));
        JSAPILightWeightMap::Set(thread, lightWeightMap, key, value);
    }
}

JSTaggedValue JSAPILightWeightMap::Remove(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                          const JSHandle<JSTaggedValue> &key)
{
    KeyState keyState = GetStateOfKey(thread, lightWeightMap, key);
    if (!keyState.existed) {
        return JSTaggedValue::Undefined();
    }
    int32_t index = keyState.index;
    JSHandle<TaggedArray> valueArray = GetArrayByKind(thread, lightWeightMap, AccossorsKind::VALUE);
    JSTaggedValue value = valueArray->Get(index);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::HASH);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::VALUE);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::KEY);
    ASSERT(lightWeightMap->GetLength() > 0);
    lightWeightMap->SetLength(lightWeightMap->GetLength() - 1);
    return value;
}

JSTaggedValue JSAPILightWeightMap::RemoveAt(JSThread *thread,
                                            const JSHandle<JSAPILightWeightMap> &lightWeightMap, int32_t index)
{
    uint32_t length = lightWeightMap->GetSize();
    if (index < 0 || static_cast<int32_t>(length) <= index) {
        return JSTaggedValue::False();
    }
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::HASH);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::VALUE);
    RemoveValue(thread, lightWeightMap, index, AccossorsKind::KEY);
    lightWeightMap->SetLength(length - 1);
    return JSTaggedValue::True();
}

JSTaggedValue JSAPILightWeightMap::IsEmpty()
{
    if (GetLength() == 0) {
        return JSTaggedValue::True();
    } else {
        return JSTaggedValue::False();
    }
}

void JSAPILightWeightMap::Clear(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> hashArray = factory->NewTaggedArray(DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> valueArray = factory->NewTaggedArray(DEFAULT_CAPACITY_LENGTH);
    lightWeightMap->SetHashes(thread, hashArray.GetTaggedValue());
    lightWeightMap->SetKeys(thread, keyArray.GetTaggedValue());
    lightWeightMap->SetValues(thread, valueArray.GetTaggedValue());
    lightWeightMap->SetLength(0);
}

JSTaggedValue JSAPILightWeightMap::SetValueAt(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                              int32_t index, const JSHandle<JSTaggedValue> &value)
{
    int32_t length = static_cast<int32_t>(lightWeightMap->GetSize());
    if (length <= 0) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, "Container is empty");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    if (index < 0 || length <= index) {
        std::ostringstream oss;
        oss << "The value of \"index\" is out of range. It must be >= 0 && <= " << (length - 1)
            << ". Received value is: " << index;
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    ReplaceValue(thread, lightWeightMap, index, value, AccossorsKind::VALUE);
    return JSTaggedValue::True();
}

int32_t JSAPILightWeightMap::AvoidHashCollision(HashParams &params, int32_t index, uint32_t size, int32_t hash)
{
    int32_t right = index;
    while ((right < static_cast<int32_t>(size)) && ((params.hashArray)->Get(right).GetInt() == hash)) {
        if (JSTaggedValue::SameValue((params.keyArray)->Get(right), *(params.key))) {
            return right;
        }
        right++;
    }
    int32_t left = index - 1;
    while ((left >= 0) && ((params.hashArray)->Get(left).GetInt() == hash)) {
        if (JSTaggedValue::SameValue((params.keyArray)->Get(left), *(params.key))) {
            return left;
        }
        left--;
    }

    int32_t res = (-right) ^ HASH_REBELLION;
    return res;
}

JSTaggedValue JSAPILightWeightMap::GetIteratorObj(JSThread *thread, const JSHandle<JSAPILightWeightMap> &obj,
                                                  IterationKind type)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightMapIterator> iter(factory->NewJSAPILightWeightMapIterator(obj, type));

    return iter.GetTaggedValue();
}

JSTaggedValue JSAPILightWeightMap::ToString(JSThread *thread, const JSHandle<JSAPILightWeightMap> &lightWeightMap)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::u16string sepStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(",");
    std::u16string colonStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(":");
    uint32_t length = lightWeightMap->GetLength();
    std::u16string concatStr;
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());

    for (uint32_t k = 0; k < length; k++) {
        std::u16string valueStr;
        valueHandle.Update(lightWeightMap->GetValueAt(thread, lightWeightMap, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!valueHandle->IsUndefined() && !valueHandle->IsNull()) {
            JSHandle<EcmaString> valueStringHandle = JSTaggedValue::ToString(thread, valueHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            valueStr = EcmaStringAccessor(valueStringHandle).ToU16String();
        }

        std::u16string nextStr;
        keyHandle.Update(lightWeightMap->GetKeyAt(thread, lightWeightMap, k));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (!keyHandle->IsUndefined() && !keyHandle->IsNull()) {
            JSHandle<EcmaString> keyStringHandle = JSTaggedValue::ToString(thread, keyHandle);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            nextStr = EcmaStringAccessor(keyStringHandle).ToU16String();
        }

        nextStr.append(colonStr);
        nextStr.append(valueStr);

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

JSHandle<TaggedArray> JSAPILightWeightMap::GrowCapacity(const JSThread *thread, JSHandle<TaggedArray> &oldArray,
                                                        uint32_t needCapacity)
{
    uint32_t oldLength = oldArray->GetLength();
    if (needCapacity <= oldLength) {
        return oldArray;
    }
    uint32_t newCapacity = ComputeCapacity(needCapacity);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArray = factory->CopyArray(oldArray, oldLength, newCapacity);
    return newArray;
}

void JSAPILightWeightMap::SetArrayByKind(const JSThread *thread,
                                         const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                         const JSHandle<TaggedArray> &array,
                                         AccossorsKind kind)
{
    switch (kind) {
        case AccossorsKind::HASH:
            lightWeightMap->SetHashes(thread, array);
            break;
        case AccossorsKind::KEY:
            lightWeightMap->SetKeys(thread, array);
            break;
        case AccossorsKind::VALUE:
            lightWeightMap->SetValues(thread, array);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

JSHandle<TaggedArray> JSAPILightWeightMap::GetArrayByKind(const JSThread *thread,
                                                          const JSHandle<JSAPILightWeightMap> &lightWeightMap,
                                                          AccossorsKind kind)
{
    JSHandle<TaggedArray> array;
    switch (kind) {
        case AccossorsKind::HASH:
            array = JSHandle<TaggedArray>(thread, lightWeightMap->GetHashes());
            break;
        case AccossorsKind::KEY:
            array = JSHandle<TaggedArray>(thread, lightWeightMap->GetKeys());
            break;
        case AccossorsKind::VALUE:
            array = JSHandle<TaggedArray>(thread, lightWeightMap->GetValues());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return array;
}

int32_t JSAPILightWeightMap::Hash(const JSThread *thread, JSTaggedValue key)
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
        return EcmaStringAccessor(keyString).GetHashcode();
    }
    if (key.IsECMAObject()) {
        uint32_t hash = static_cast<uint32_t>(ECMAObject::Cast(key.GetTaggedObject())->GetHash());
        if (hash == 0) {
            hash = base::RandomGenerator::GenerateIdentityHash();
            JSHandle<ECMAObject> ecmaObj(thread, key);
            ECMAObject::SetHash(thread, hash, ecmaObj);
        }
        return hash;
    }
    if (key.IsInt()) {
        int32_t hash = key.GetInt();
        return hash;
    }
    if (key.IsBigInt()) {
        uint32_t keyValue = BigInt::Cast(key.GetTaggedObject())->GetDigit(0);
        return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
    }
    uint64_t keyValue = key.GetRawData();
    return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
}

int32_t JSAPILightWeightMap::BinarySearchHashes(JSHandle<TaggedArray> &array, int32_t hash, int32_t size)
{
    int32_t low = 0;
    int32_t high = size - 1;
    while (low <= high) {
        uint32_t mid = static_cast<uint32_t>(low + high) >> 1U;
        int32_t midHash = array->Get(mid).GetInt();
        if (midHash < hash) {
            low = static_cast<int32_t>(mid) + 1;
        } else {
            if (midHash == hash) {
                return mid;
            }
            high = static_cast<int32_t>(mid) - 1;
        }
    }
    return -(low + 1);
}
}  // namespace panda::ecmascript
