/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_object-inl.h"

namespace panda::ecmascript {
int NameDictionary::Hash(const JSTaggedValue &key)
{
    if (key.IsHeapObject()) {
        JSTaggedValue jsKey(key);
        if (jsKey.IsSymbol()) {
            auto symbolString = JSSymbol::Cast(key.GetTaggedObject());
            return symbolString->GetHashField();
        }
        if (jsKey.IsString()) {
            auto keyString = reinterpret_cast<EcmaString *>(key.GetTaggedObject());
            return EcmaStringAccessor(keyString).GetHashcode();
        }
    }
    // key must be object
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

// for ohmurl path to compute hash code
int NameDictionary::Hash(const uint8_t* str, int strSize)
{
    return BaseString::ComputeHashForData(str, strSize, 0);
}

bool NameDictionary::IsMatch(const JSTaggedValue &key, const JSTaggedValue &other)
{
    return key == other;
}

bool NameDictionary::IsMatch(const uint8_t* str, int size, const JSTaggedValue &other)
{
    if (!other.IsString()) {
        return false;
    }
    EcmaString *keyString = reinterpret_cast<EcmaString *>(other.GetTaggedObject());

    Span<const uint8_t> data1(EcmaStringAccessor(keyString).GetDataUtf8(), keyString->GetLength());
    Span<const uint8_t> data2(str, size);
    if (data1.Size() != data2.Size()) {
        return false;
    }
    return EcmaString::StringsAreEquals(data1, data2);
}

void NameDictionary::GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const
{
    int arrayIndex = 0;
    int size = Size();
    CVector<std::pair<JSTaggedValue, PropertyAttributes>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            std::pair<JSTaggedValue, PropertyAttributes> pair(key, attr);
            sortArr.push_back(pair);
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    for (const auto &entry : sortArr) {
        keyArray->Set(thread, arrayIndex + offset, entry.first);
        arrayIndex++;
    }
}

void NameDictionary::UpdateAllAttributesToNoWitable(const JSThread *thread)
{
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            attr.SetWritable(false);
            SetAttributes(thread, hashIndex, attr);
        }
    }
}

void NameDictionary::GetAllKeysByFilter(const JSThread *thread, uint32_t &keyArrayEffectivelength,
                                        TaggedArray *keyArray, uint32_t filter) const
{
    int size = Size();
    CVector<std::pair<JSTaggedValue, PropertyAttributes>> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            bool bIgnore = FilterHelper::IgnoreKeyByFilter<PropertyAttributes>(attr, filter);
            if (bIgnore) {
                continue;
            }
            if (key.IsString() && (filter & NATIVE_KEY_SKIP_STRINGS)) {
                continue;
            }
            if (key.IsSymbol() && (filter & NATIVE_KEY_SKIP_SYMBOLS)) {
                continue;
            }
            std::pair<JSTaggedValue, PropertyAttributes> pair(key, attr);
            sortArr.push_back(pair);
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    for (const auto &entry : sortArr) {
        keyArray->Set(thread, keyArrayEffectivelength, entry.first);
        keyArrayEffectivelength++;
    }
}

std::pair<uint32_t, uint32_t> NameDictionary::GetNumOfEnumKeys() const
{
    uint32_t enumKeys = 0;
    uint32_t shadowKeys = 0;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = GetKey(hashIndex);
        if (key.IsString()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            if (attr.IsEnumerable()) {
                enumKeys++;
            } else {
                shadowKeys++;
            }
        }
    }
    return std::make_pair(enumKeys, shadowKeys);
}

void NameDictionary::GetAllEnumKeys(JSThread *thread, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys,
                                    JSHandle<TaggedQueue> shadowQueue, int32_t lastLength) const
{
    uint32_t arrayIndex = 0;
    CVector<std::pair<JSHandle<JSTaggedValue>, PropertyAttributes>> sortArr;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSHandle<JSTaggedValue> keyHandle(thread, GetKey(hashIndex));
        if (keyHandle->IsString()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            if (attr.IsEnumerable()) {
                std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> pair(keyHandle, attr);
                bool isDuplicated = JSObject::IsDepulicateKeys(thread, keyArray, lastLength, shadowQueue, keyHandle);
                if (isDuplicated) {
                    continue;
                }
                sortArr.push_back(pair);
            } else {
                TaggedQueue::PushFixedQueue(thread, shadowQueue, keyHandle);
            }
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompHandleKey);
    for (auto entry : sortArr) {
        keyArray->Set(thread, arrayIndex + static_cast<uint32_t>(offset), entry.first);
        arrayIndex++;
    }
    *keys += arrayIndex;
}

void NameDictionary::GetAllEnumKeys(JSThread *thread, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys) const
{
    uint32_t arrayIndex = 0;
    CVector<std::pair<JSHandle<JSTaggedValue>, PropertyAttributes>> sortArr;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSHandle<JSTaggedValue> keyHandle(thread, GetKey(hashIndex));
        if (keyHandle->IsString()) {
            PropertyAttributes attr = GetAttributes(hashIndex);
            if (attr.IsEnumerable()) {
                std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> pair(keyHandle, attr);
                sortArr.push_back(pair);
            }
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompHandleKey);
    for (auto entry : sortArr) {
        keyArray->Set(thread, arrayIndex + static_cast<uint32_t>(offset), entry.first);
        arrayIndex++;
    }
    *keys += arrayIndex;
}

JSHandle<NameDictionary> NameDictionary::Create(const JSThread *thread, int numberOfElements)
{
    return OrderHashTableT::Create(thread, numberOfElements);
}

JSHandle<NameDictionary> NameDictionary::CreateInSharedHeap(const JSThread *thread, int numberOfElements)
{
    return OrderHashTableT::Create(thread, numberOfElements, MemSpaceKind::SHARED);
}

PropertyAttributes NameDictionary::GetAttributes(int entry) const
{
    int index = GetEntryIndex(entry) + ENTRY_DETAILS_INDEX;
    return PropertyAttributes(Get(index));
}

void NameDictionary::SetAttributes(const JSThread *thread, int entry, const PropertyAttributes &metaData)
{
    int index = GetEntryIndex(entry) + ENTRY_DETAILS_INDEX;
    Set(thread, index, metaData.GetTaggedValue());
}

void NameDictionary::SetEntry(const JSThread *thread, int entry, const JSTaggedValue &key, const JSTaggedValue &value,
                              const PropertyAttributes &metaData)
{
    SetKey(thread, entry, key);
    SetValue(thread, entry, value);
    SetAttributes(thread, entry, metaData);
}

void NameDictionary::UpdateValueAndAttributes(const JSThread *thread, int entry, const JSTaggedValue &value,
                                              const PropertyAttributes &metaData)
{
    SetValue(thread, entry, value);
    SetAttributes(thread, entry, metaData);
}

void NameDictionary::UpdateValue(const JSThread *thread, int entry, const JSTaggedValue &value)
{
    SetValue(thread, entry, value);
}

void NameDictionary::ClearEntry(const JSThread *thread, int entry)
{
    JSTaggedValue hole = JSTaggedValue::Hole();
    PropertyAttributes metaData;
    SetEntry(thread, entry, hole, hole, metaData);
}

int NumberDictionary::Hash(const JSTaggedValue &key)
{
    if (key.IsInt()) {
        int keyValue = key.GetInt();
        return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
    }
    if (key.IsDouble()) {
        int32_t keyValue = static_cast<int32_t>(static_cast<uint32_t>(key.GetDouble()));
        return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
    }
    // key must be object
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

bool NumberDictionary::IsMatch(const JSTaggedValue &key, const JSTaggedValue &other)
{
    if (key.IsHole() || key.IsUndefined()) {
        return false;
    }

    if (key.IsInt()) {
        if (other.IsInt()) {
            return key.GetInt() == other.GetInt();
        }
        return false;
    }

    if (key.IsDouble()) {
        if (other.IsInt()) {
            int32_t keyValue = static_cast<int32_t>(static_cast<uint32_t>(key.GetDouble()));
            return keyValue == other.GetInt();
        }
        if (other.IsDouble()) {
            return key.GetDouble() == other.GetDouble();
        }
        return false;
    }
    // key must be integer
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

void NumberDictionary::GetAllKeys(const JSThread *thread, const JSHandle<NumberDictionary> &obj, int offset,
                                  const JSHandle<TaggedArray> &keyArray)
{
    ASSERT_PRINT(offset + obj->EntriesCount() <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");
    int arrayIndex = 0;
    int size = obj->Size();
    CVector<JSTaggedValue> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = obj->GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            sortArr.push_back(JSTaggedValue(static_cast<uint32_t>(key.GetInt())));
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    for (auto entry : sortArr) {
        JSHandle<JSTaggedValue> keyHandle(thread, entry);
        JSHandle<EcmaString> str = JSTaggedValue::ToString(const_cast<JSThread *>(thread), keyHandle);
        ASSERT_NO_ABRUPT_COMPLETION(thread);
        keyArray->Set(thread, arrayIndex + offset, str.GetTaggedValue());
        arrayIndex++;
    }
}

void NumberDictionary::GetAllKeysByFilter(const JSThread *thread, const JSHandle<NumberDictionary> &obj,
                                          uint32_t &keyArrayEffectivelength, const JSHandle<TaggedArray> &keyArray,
                                          uint32_t filter)
{
    ASSERT_PRINT(keyArrayEffectivelength + static_cast<uint32_t>(obj->EntriesCount()) <= keyArray->GetLength(),
                 "keyArray capacity is not enough for dictionary");

    CVector<JSTaggedValue> sortArr;
    uint32_t size = static_cast<uint32_t>(obj->Size());
    for (uint32_t hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = obj->GetKey(hashIndex);
        if (key.IsUndefined() || key.IsHole()) {
            continue;
        }

        PropertyAttributes attr = obj->GetAttributes(hashIndex);
        bool bIgnore = FilterHelper::IgnoreKeyByFilter<PropertyAttributes>(attr, filter);
        if (!bIgnore) {
            sortArr.push_back(JSTaggedValue(static_cast<uint32_t>(key.GetInt())));
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    ASSERT_NO_ABRUPT_COMPLETION(thread);
    for (auto entry : sortArr) {
        keyArray->Set(thread, keyArrayEffectivelength, entry);
        keyArrayEffectivelength++;
    }
}

void NumberDictionary::GetAllEnumKeys(JSThread *thread, const JSHandle<NumberDictionary> &obj, int offset,
                                      const JSHandle<TaggedArray> &keyArray, uint32_t *keys, int32_t lastLength)
{
    ASSERT_PRINT(offset + obj->EntriesCount() <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");
    uint32_t arrayIndex = 0;
    int size = obj->Size();
    CVector<JSTaggedValue> sortArr;
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key = obj->GetKey(hashIndex);
        if (!key.IsUndefined() && !key.IsHole()) {
            PropertyAttributes attr = obj->GetAttributes(hashIndex);
            if (attr.IsEnumerable()) {
                sortArr.push_back(JSTaggedValue(static_cast<uint32_t>(key.GetInt())));
            }
        }
    }
    std::sort(sortArr.begin(), sortArr.end(), CompKey);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedQueue> emptyQueue = factory->GetEmptyTaggedQueue();
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    for (auto entry : sortArr) {
        keyHandle.Update(entry);
        JSHandle<EcmaString> str = JSTaggedValue::ToString(const_cast<JSThread *>(thread), keyHandle);
        ASSERT_NO_ABRUPT_COMPLETION(thread);
        bool isDuplicated = JSObject::IsDepulicateKeys(thread, keyArray, lastLength, emptyQueue,
                                                       JSHandle<JSTaggedValue>(str));
        if (isDuplicated) {
            continue;
        }
        keyArray->Set(thread, arrayIndex + static_cast<uint32_t>(offset), str.GetTaggedValue());
        arrayIndex++;
    }
    *keys += arrayIndex;
}

JSHandle<NumberDictionary> NumberDictionary::Create(const JSThread *thread, int numberOfElements)
{
    return OrderHashTableT::Create(thread, numberOfElements);
}

JSHandle<NumberDictionary> NumberDictionary::CreateInSharedHeap(const JSThread *thread, int numberOfElements)
{
    return OrderHashTableT::Create(thread, numberOfElements, MemSpaceKind::SHARED);
}

PropertyAttributes NumberDictionary::GetAttributes(int entry) const
{
    int index = GetEntryIndex(entry) + ENTRY_DETAILS_INDEX;
    return PropertyAttributes(Get(index));
}

void NumberDictionary::SetAttributes(const JSThread *thread, int entry, const PropertyAttributes &metaData)
{
    int index = GetEntryIndex(entry) + ENTRY_DETAILS_INDEX;
    Set(thread, index, metaData.GetTaggedValue());
}

void NumberDictionary::SetEntry(const JSThread *thread, int entry, const JSTaggedValue &key, const JSTaggedValue &value,
                                const PropertyAttributes &metaData)
{
    SetKey(thread, entry, key);
    SetValue(thread, entry, value);
    SetAttributes(thread, entry, metaData);
}

void NumberDictionary::UpdateValueAndAttributes(const JSThread *thread, int entry, const JSTaggedValue &value,
                                                const PropertyAttributes &metaData)
{
    SetValue(thread, entry, value);
    SetAttributes(thread, entry, metaData);
}

void NumberDictionary::UpdateValue(const JSThread *thread, int entry, const JSTaggedValue &value)
{
    SetValue(thread, entry, value);
}

void NumberDictionary::ClearEntry(const JSThread *thread, int entry)
{
    JSTaggedValue hole = JSTaggedValue::Hole();
    PropertyAttributes metaData;
    SetEntry(thread, entry, hole, hole, metaData);
}

JSHandle<PointerToIndexDictionary> PointerToIndexDictionary::Create(const JSThread *thread, int numberOfElements)
{
    return OrderHashTableT::Create(thread, numberOfElements);
}

JSHandle<PointerToIndexDictionary> PointerToIndexDictionary::PutIfAbsent(
    const JSThread *thread,
    const JSHandle<PointerToIndexDictionary> &dictionary,
    const JSHandle<JSTaggedValue> &key,
    const JSHandle<JSTaggedValue> &value)
{
    /* no need to add key if exist */
    int entry = dictionary->FindEntry(key.GetTaggedValue());
    if (entry != -1) {
        return dictionary;
    }
    // Check whether the table should be growed
    JSHandle<PointerToIndexDictionary> newDictionary = HashTableT::GrowHashTable(thread, dictionary);

    // Compute the key object
    uint32_t hash = static_cast<uint32_t>(PointerToIndexDictionary::Hash(key.GetTaggedValue()));
    entry = newDictionary->FindInsertIndex(hash);
    newDictionary->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue());
    newDictionary->IncreaseEntries(thread);
    return newDictionary;
}

using ProtoArray = FunctionProtoTransitionTable::ProtoArray;
FunctionProtoTransitionTable::FunctionProtoTransitionTable(const JSThread *thread)
{
    protoTransitionTable_ = PointerToIndexDictionary::Create(thread, DEFAULT_FIRST_LEVEL_SIZE).GetTaggedValue();
}

FunctionProtoTransitionTable::~FunctionProtoTransitionTable()
{
    fakeParentMap_.clear();
}

void FunctionProtoTransitionTable::Iterate(RootVisitor &v)
{
    v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&protoTransitionTable_)));
    for (auto &iter : fakeParentMap_) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.first)));
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.second)));
    }
    for (auto &iter : fakeChildMap_) {
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.first)));
        v.VisitRoot(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&iter.second)));
    }
}

void FunctionProtoTransitionTable::UpdateProtoTransitionTable(const JSThread *thread,
                                                              JSHandle<PointerToIndexDictionary> map)
{
    // PointerToIndexDictionary's hash value is a hclass pointer,
    // and the hclass pointer could change during deserialized,
    // so rehash is needed after deserialized.
    auto newMap = PointerToIndexDictionary::Create(thread, map->Size());
    map->Rehash(thread, *newMap);
    protoTransitionTable_ = newMap.GetTaggedValue();
}

void FunctionProtoTransitionTable::InsertTransitionItem(const JSThread *thread,
                                                        JSHandle<JSTaggedValue> ihc,
                                                        JSHandle<JSTaggedValue> baseIhc,
                                                        JSHandle<JSTaggedValue> transIhc,
                                                        JSHandle<JSTaggedValue> transPhc)
{
    JSHandle<PointerToIndexDictionary> protoTransitionHandle(thread, protoTransitionTable_);
    int32_t entry1 = protoTransitionHandle->FindEntry(ihc.GetTaggedValue());
    if (entry1 != -1) {
        JSHandle<ProtoArray> protoArray(thread, protoTransitionHandle->GetValue(entry1));
        auto entry2 = protoArray->FindEntry(baseIhc.GetTaggedValue());
        if (entry2 != -1) {
            LOG_ECMA(DEBUG) << "Record prototype for current function already!";
            return;
        }
        uint32_t insertEntry = ProtoArray::GetEntry(JSHandle<JSHClass>(baseIhc));
        protoArray->SetEntry(
            thread, insertEntry, baseIhc.GetTaggedValue(), transIhc.GetTaggedValue(), transPhc.GetTaggedValue());
        return;
    }
    JSHandle<ProtoArray> protoArray = ProtoArray::Create(thread);
    uint32_t insertEntry = ProtoArray::GetEntry(JSHandle<JSHClass>(baseIhc));
    protoArray->SetEntry(
        thread, insertEntry, baseIhc.GetTaggedValue(), transIhc.GetTaggedValue(), transPhc.GetTaggedValue());
    JSHandle<PointerToIndexDictionary> newTransitionTable =
        PointerToIndexDictionary::PutIfAbsent(thread, protoTransitionHandle, ihc, JSHandle<JSTaggedValue>(protoArray));
    protoTransitionTable_ = newTransitionTable.GetTaggedValue();
}

std::pair<JSTaggedValue, JSTaggedValue> FunctionProtoTransitionTable::FindTransitionByHClass(
    const JSThread *thread, JSHandle<JSTaggedValue> ihc, JSHandle<JSTaggedValue> baseIhc) const
{
    ASSERT(ihc->IsJSHClass());
    ASSERT(baseIhc->IsJSHClass());
    JSHandle<PointerToIndexDictionary> protoTransitionHandle(thread, protoTransitionTable_);
    int32_t entry1 = protoTransitionHandle->FindEntry(ihc.GetTaggedValue());
    if (entry1 == -1) {
        LOG_ECMA(DEBUG) << "Selected ihc not found2!";
        return std::make_pair(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
    }
    JSHandle<ProtoArray> protoArray(thread, protoTransitionHandle->GetValue(entry1));
    int32_t entry2 = protoArray->FindEntry(baseIhc.GetTaggedValue());
    if (entry2 == -1) {
        LOG_ECMA(ERROR) << "Selected baseIhc not found!";
        return std::make_pair(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
    }
    return protoArray->GetTransition(entry2);
}

bool FunctionProtoTransitionTable::TryInsertFakeParentItem(JSTaggedType child, JSTaggedType parent)
{
    LockHolder lock(fakeParentMutex_);
    // Situation:
    // 1: d1.prototype = p
    // 2: d2.prototype = p
    // this check is true for step 2
    auto iter1 = fakeParentMap_.find(child);
    if (child == parent && iter1 != fakeParentMap_.end()) {
        return true;
    }
    auto iter2 = fakeChildMap_.find(parent);
    if (iter2 != fakeChildMap_.end()) {
        if (iter2->second != child) {
            LOG_ECMA(DEBUG) << "Unsupported mapping for a parent to more than 1 childs!";
            return false;
        }
    }

    if (iter1 == fakeParentMap_.end()) {
        fakeParentMap_[child] = parent;
        fakeChildMap_[parent] = child;
        return true;
    }
    if (iter1->second == parent) {
        return true;
    }
    LOG_ECMA(ERROR) << "Conflict mapping for the same child";
    return false;
}

JSTaggedType FunctionProtoTransitionTable::GetFakeParent(JSTaggedType child)
{
    LockHolder lock(fakeParentMutex_);
    auto iter = fakeParentMap_.find(child);
    if (iter != fakeParentMap_.end()) {
        return iter->second;
    }
    return 0;
}

// static
JSHandle<ProtoArray> ProtoArray::Create(const JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> result = factory->NewTaggedArray(ENTRY_NUMBER * ENTRY_SIZE);
    return JSHandle<ProtoArray>(result);
}

// static
bool ProtoArray::GetEntry(JSHandle<JSHClass> hclass)
{
    return JSHandle<JSHClass>(hclass)->IsPrototype() ? ProtoArray::CLONED_ENTRY_INDEX
                                                     : ProtoArray::INIT_ENTRY_INDEX;
}

void ProtoArray::SetEntry(const JSThread *thread, uint32_t index, JSTaggedValue key, JSTaggedValue transIhc,
                          JSTaggedValue transPhc)
{
    uint32_t entryIndex = index * ENTRY_SIZE;
    uint32_t keyIndex = entryIndex + KEY_INDEX;
    uint32_t ihcIndex = entryIndex + TRANS_IHC_INDEX;
    uint32_t phcIndex = entryIndex + TRANS_PHC_INDEX;
    Set(thread, keyIndex, key);
    Set(thread, ihcIndex, transIhc);
    Set(thread, phcIndex, transPhc);
}

JSTaggedValue ProtoArray::GetKey(uint32_t index) const
{
    uint32_t entryIndex = index * ENTRY_SIZE;
    uint32_t keyIndex = entryIndex + KEY_INDEX;
    return Get(keyIndex);
}

std::pair<JSTaggedValue, JSTaggedValue> ProtoArray::GetTransition(uint32_t index) const
{
    uint32_t entryIndex = index * ENTRY_SIZE + KEY_INDEX;
    JSTaggedValue transIhc = Get(entryIndex + TRANS_IHC_INDEX);
    JSTaggedValue transPhc = Get(entryIndex + TRANS_PHC_INDEX);
    return std::make_pair(transIhc, transPhc);
}

std::pair<JSTaggedValue, JSTaggedValue> ProtoArray::FindTransition(JSTaggedValue key) const
{
    for (uint32_t i = 0; i < ENTRY_NUMBER; i++) {
        uint32_t index = i * ENTRY_SIZE + KEY_INDEX;
        JSTaggedValue keyValue = Get(index);
        if (keyValue == key) {
            JSTaggedValue transIhc = Get(index + TRANS_IHC_INDEX);
            JSTaggedValue transPhc = Get(index + TRANS_PHC_INDEX);
            return std::make_pair(transIhc, transPhc);
        }
    }
    return std::make_pair(JSTaggedValue::Undefined(), JSTaggedValue::Undefined());
}

int32_t ProtoArray::FindEntry(JSTaggedValue key) const
{
    for (uint32_t i = 0; i < ENTRY_NUMBER; i++) {
        uint32_t index = i * ENTRY_SIZE + KEY_INDEX;
        JSTaggedValue keyValue = Get(index);
        if (keyValue == key) {
            return i;
        }
    }
    return -1;
}
}  // namespace panda::ecmascript
