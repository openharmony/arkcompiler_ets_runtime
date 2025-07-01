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

#ifndef ECMASCRIPT_TAGGED_HASH_TABLE_H
#define ECMASCRIPT_TAGGED_HASH_TABLE_H

#include <vector>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
template<typename Derived>
class TaggedHashTable : public TaggedArray {
public:
    inline int EntriesCount() const
    {
        return GetPrimitive(NUMBER_OF_ENTRIES_INDEX).GetInt();
    }

    inline int HoleEntriesCount() const
    {
        return GetPrimitive(NUMBER_OF_HOLE_ENTRIES_INDEX).GetInt();
    }

    inline int Size() const
    {
        return GetPrimitive(SIZE_INDEX).GetInt();
    }

    inline void IncreaseEntries(const JSThread *thread)
    {
        SetEntriesCount(thread, EntriesCount() + 1);
    }

    inline void IncreaseHoleEntriesCount(const JSThread *thread, int number = 1)
    {
        SetEntriesCount(thread, EntriesCount() - number);
        SetHoleEntriesCount(thread, HoleEntriesCount() + number);
    }

    inline static int ComputeHashTableSize(uint32_t atLeastSize)
    {
        //  increase size for hash-collision
        uint32_t rawSize = atLeastSize + (atLeastSize >> 1UL);
        int newSize = static_cast<int>(helpers::math::GetPowerOfTwoValue32(rawSize));
        return (newSize > MIN_SIZE) ? newSize : MIN_SIZE;
    }

    static JSHandle<Derived> GrowHashTable(const JSThread *thread, const JSHandle<Derived> &table,
                                           int numOfAddedElements = 1)
    {
        if (!table->IsNeedGrowHashTable(numOfAddedElements)) {
            // if deleted entries are more than half of the free entries, rehash to clear holes.
            int freeSize = table->Size() - table->EntriesCount() - numOfAddedElements;
            if (table->HoleEntriesCount() > freeSize / 2) { // 2: half
                int copyLength = Derived::GetEntryIndex(table->Size());
                JSHandle<Derived> copyTable = table.GetTaggedValue().IsInSharedHeap() ?
                    JSHandle<Derived>(thread->GetEcmaVM()->GetFactory()->NewSDictionaryArray(copyLength)) :
                    JSHandle<Derived>(thread->GetEcmaVM()->GetFactory()->NewDictionaryArray(copyLength));
                copyTable->SetHashTableSize(thread, table->Size());
                table->Rehash(thread, *copyTable);
                return copyTable;
            }
            return table;
        }
        int newSize = Derived::ComputeCompactSize(
            thread, table, ComputeHashTableSize(table->Size() + numOfAddedElements), table->Size(), numOfAddedElements);
        newSize = std::max(newSize, MIN_SHRINK_SIZE);
        int length = Derived::GetEntryIndex(newSize);
        JSHandle<Derived> newTable = table.GetTaggedValue().IsInSharedHeap() ?
            JSHandle<Derived>(thread->GetEcmaVM()->GetFactory()->NewSDictionaryArray(length)) :
            JSHandle<Derived>(thread->GetEcmaVM()->GetFactory()->NewDictionaryArray(length));
        newTable->SetHashTableSize(thread, newSize);
        table->Rehash(thread, *newTable);
        return newTable;
    }

    static JSHandle<Derived> Create(const JSThread *thread, int entriesCount,
                                    MemSpaceKind spaceKind = MemSpaceKind::LOCAL)
    {
        ASSERT_PRINT((entriesCount > 0), "the size must be greater than zero");
        auto size = static_cast<uint32_t>(entriesCount);
        ASSERT_PRINT(helpers::math::IsPowerOfTwo(static_cast<uint32_t>(entriesCount)), "the size must be power of two");

        int length = Derived::GetEntryIndex(entriesCount);

        JSHandle<Derived> table = spaceKind == MemSpaceKind::SHARED ?
            JSHandle<Derived>(thread->GetEcmaVM()->GetFactory()->NewSDictionaryArray(length)) :
            JSHandle<Derived>(thread->GetEcmaVM()->GetFactory()->NewDictionaryArray(length));
        table->SetEntriesCount(thread, 0);
        table->SetHoleEntriesCount(thread, 0);
        table->SetHashTableSize(thread, size);
        return table;
    }

    static JSHandle<Derived> Insert(const JSThread *thread, JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
    {
        int entry = table->FindEntry(thread, key.GetTaggedValue());
        if (entry != -1) {
            table->SetValue(thread, entry, value.GetTaggedValue());
            return table;
        }

        // Make sure the key object has an identity hash code.
        uint32_t hash = static_cast<uint32_t>(Derived::Hash(thread, key.GetTaggedValue()));
        JSHandle<Derived> newTable = GrowHashTable(thread, table);
        newTable->AddElement(thread, newTable->FindInsertIndex(thread, hash), key, value);
        return newTable;
    }

    static JSHandle<Derived> Remove(const JSThread *thread, JSHandle<Derived> &table,
                                    const JSHandle<JSTaggedValue> &key)
    {
        int entry = table->FindEntry(thread, key.GetTaggedValue());
        if (entry == -1) {
            return table;
        }

        table->RemoveElement(thread, entry);
        return Derived::Shrink(thread, *table);
    }

    inline static int RecalculateTableSize(int currentSize, int atLeastSize)
    {
        // When the filled entries is greater than a quart of currentSize
        // it need not to shrink
        if (atLeastSize > (currentSize / 4)) {  // 4 : quarter
            return currentSize;
        }
        // Recalculate table size
        int newSize = ComputeHashTableSize(atLeastSize);
        ASSERT_PRINT(newSize > atLeastSize, "new size must greater than atLeastSize");
        // Don't go lower than room for MIN_SHRINK_SIZE elements.
        if (newSize < MIN_SHRINK_SIZE) {
            return currentSize;
        }
        return newSize;
    }

    inline static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &table, int additionalSize)
    {
        int newSize = RecalculateTableSize(table->Size(), table->EntriesCount() + additionalSize);
        if (newSize == table->Size()) {
            return table;
        }

        JSHandle<Derived> newTable = TaggedHashTable::Create(thread, newSize,
            table.GetTaggedValue().IsInSharedHeap() ? MemSpaceKind::SHARED : MemSpaceKind::LOCAL);

        table->Rehash(thread, *newTable);
        return newTable;
    }

    bool IsNeedGrowHashTable(int numOfAddEntries)
    {
        int entriesCount = EntriesCount();
        int currentSize = Size();
        int numberFilled = entriesCount + numOfAddEntries;
        // needn't to grow table: after adding number entries, table have half free entries.
        const int halfFree = 2;
        int neededFree = numberFilled / halfFree;
        if (numberFilled + neededFree <= currentSize) {
            return false;
        }
        return true;
    }

    JSTaggedValue GetKey(const JSThread *thread, int entry) const
    {
        int index = Derived::GetKeyIndex(entry);
        if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
            return JSTaggedValue::Undefined();
        }
        return Get(thread, index);
    }

    JSTaggedValue GetValue(const JSThread *thread, int entry) const
    {
        int index = Derived::GetValueIndex(entry);
        if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
            return JSTaggedValue::Undefined();
        }
        return Get(thread, index);
    }

    inline void GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const
    {
        ASSERT_PRINT(offset + EntriesCount() <= static_cast<int>(keyArray->GetLength()),
                     "keyArray size is not enough for dictionary");
        int arrayIndex = 0;
        int size = Size();
        for (int hashIndex = 0; hashIndex < size; hashIndex++) {
            JSTaggedValue key = GetKey(thread, hashIndex);
            if (!key.IsUndefined() && !key.IsHole()) {
                keyArray->Set(thread, arrayIndex + offset, key);
                arrayIndex++;
            }
        }
    }

    inline void GetAllKeysIntoVector(const JSThread *thread, std::vector<JSTaggedValue> &vector) const
    {
        int capacity = Size();
        for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
            JSTaggedValue key = GetKey(thread, hashIndex);
            if (!key.IsUndefined() && !key.IsHole()) {
                vector.push_back(key);
            }
        }
    }

    inline void Swap(const JSThread *thread, int src, int dst);

    // Find entry for key otherwise return -1.
    inline int FindEntry(const JSThread *thread, const JSTaggedValue &key)
    {
        int size = Size();
        int count = 1;
        JSTaggedValue keyValue;
        int32_t hash = static_cast<int32_t>(Derived::Hash(thread, key));

        for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
            keyValue = GetKey(thread, entry);
            if (keyValue.IsHole()) {
                continue;
            }
            if (keyValue.IsUndefined()) {
                return -1;
            }
            if (Derived::IsMatch(thread, key, keyValue)) {
                return entry;
            }
        }
        return -1;
    }

    inline int FindEntry(const JSThread *thread, const uint8_t* str, int strSize)
    {
        int size = Size();
        int count = 1;
        JSTaggedValue keyValue;
        int32_t hash = static_cast<int32_t>(Derived::Hash(str, strSize));

        for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
            keyValue = GetKey(thread, entry);
            if (keyValue.IsHole()) {
                continue;
            }
            if (keyValue.IsUndefined()) {
                return -1;
            }
            // keyValue defaults to ecmaString
            if (Derived::IsMatch(str, strSize, keyValue)) {
                return entry;
            }
        }
        return -1;
    }

    inline int FindInsertIndex(const JSThread *thread, uint32_t hash)
    {
        int size = Size();
        int count = 1;
        // GrowHashTable will guarantee the hash table is never full.
        for (uint32_t entry = GetFirstPosition(hash, size);; entry = GetNextPosition(entry, count++, size)) {
            if (!IsKey(GetKey(thread, entry))) {
                return entry;
            }
        }
    }

    inline void SetKey(const JSThread *thread, int entry, const JSTaggedValue &key)
    {
        int index = Derived::GetKeyIndex(entry);
        if (UNLIKELY(index < 0 || index > static_cast<int>(GetLength()))) {
            return;
        }
        Set(thread, index, key);
    }

    inline void SetValue(const JSThread *thread, int entry, const JSTaggedValue &value)
    {
        int index = Derived::GetValueIndex(entry);
        if (UNLIKELY((index < 0 || index > static_cast<int>(GetLength())))) {
            return;
        }
        Set(thread, index, value);
    }

    // Rehash element to new_table
    void Rehash(const JSThread *thread, Derived *newTable)
    {
        if ((newTable == nullptr) || (newTable->Size() < EntriesCount())) {
            return;
        }
        int currentSize = this->Size();
        // Rehash elements to new table
        for (int i = 0; i < currentSize; i++) {
            int fromIndex = Derived::GetKeyIndex(i);
            JSTaggedValue k = this->GetKey(thread, i);
            if (!IsKey(k)) {
                continue;
            }
            uint32_t hash = static_cast<uint32_t>(Derived::Hash(thread, k));
            int insertionIndex = Derived::GetKeyIndex(newTable->FindInsertIndex(thread, hash));
            JSTaggedValue tv = Get(thread, fromIndex);
            newTable->Set(thread, insertionIndex, tv);
            for (int j = 1; j < Derived::GetEntrySize(); j++) {
                tv = Get(thread, fromIndex + j);
                newTable->Set(thread, insertionIndex + j, tv);
            }
        }
        newTable->SetEntriesCount(thread, EntriesCount());
        newTable->SetHoleEntriesCount(thread, 0);
    }

    static constexpr int MIN_SHRINK_SIZE = 16;
    static constexpr int MIN_SIZE = 4;
    static constexpr int NUMBER_OF_ENTRIES_INDEX = 0;
    static constexpr int NUMBER_OF_HOLE_ENTRIES_INDEX = 1;
    static constexpr int SIZE_INDEX = 2;
    static constexpr int TABLE_HEADER_SIZE = 3;

protected:
    inline bool IsKey(const JSTaggedValue &key) const
    {
        return !key.IsHole() && !key.IsUndefined();
    };

    inline static uint32_t GetFirstPosition(uint32_t hash, uint32_t size)
    {
        ASSERT(size > 0);
        return hash & (size - 1);
    }

    inline static uint32_t GetNextPosition(uint32_t last, uint32_t number, uint32_t size)
    {
        ASSERT(size > 0);
        return (last + (number * (number + 1)) / 2) & (size - 1);  // 2 : half
    }

    inline void SetEntriesCount(const JSThread *thread, int nof)
    {
        Set(thread, NUMBER_OF_ENTRIES_INDEX, JSTaggedValue(nof));
    }

    inline void SetHoleEntriesCount(const JSThread *thread, int nod)
    {
        Set(thread, NUMBER_OF_HOLE_ENTRIES_INDEX, JSTaggedValue(nod));
    }

    // Sets the size of the hash table.
    inline void SetHashTableSize(const JSThread *thread, int size)
    {
        Set(thread, SIZE_INDEX, JSTaggedValue(size));
    }

    inline static int GetHeadSizeOfTable();
    inline static int GetEntrySize();
    inline static int GetKeyOffset();
    inline static int GetValueOffset();

    inline void AddElement(const JSThread *thread, int entry, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value)
    {
        this->SetKey(thread, entry, key.GetTaggedValue());
        this->SetValue(thread, entry, value.GetTaggedValue());
        this->IncreaseEntries(thread);
    }

    inline void RemoveElement(const JSThread *thread, int entry)
    {
        JSTaggedValue defaultValue(JSTaggedValue::Hole());
        this->SetKey(thread, entry, defaultValue);
        this->SetValue(thread, entry, defaultValue);
        this->IncreaseHoleEntriesCount(thread);
    }
};

template<typename Derived>
class OrderTaggedHashTable : public TaggedHashTable<Derived> {
public:
    using HashTableT = TaggedHashTable<Derived>;
    static Derived *Cast(TaggedObject *object)
    {
        return reinterpret_cast<Derived *>(object);
    }

    // Attempt to shrink the table after deletion of key.
    static JSHandle<Derived> Shrink(const JSThread *thread, const JSHandle<Derived> &table)
    {
        int index = table->GetNextEnumerationIndex();
        JSHandle<Derived> newTable = HashTableT::Shrink(thread, table, 0);
        newTable->SetNextEnumerationIndex(thread, index);
        return newTable;
    }

    static JSHandle<Derived> Create(const JSThread *thread, int numberOfElements = DEFAULT_ELEMENTS_NUMBER,
        MemSpaceKind spaceKind = MemSpaceKind::LOCAL)
    {
        JSHandle<Derived> dict = HashTableT::Create(thread, numberOfElements, spaceKind);
        dict->SetNextEnumerationIndex(thread, PropertyAttributes::INITIAL_PROPERTY_INDEX);
        return dict;
    }

    static JSHandle<Derived> PutIfAbsent(const JSThread *thread, const JSHandle<Derived> &table,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                         const PropertyAttributes &metaData)
    {
        /* no need to add key if exist */
        int entry = table->FindEntry(thread, key.GetTaggedValue());
        if (entry != -1) {
            return table;
        }

        int enumIndex = table->NextEnumerationIndex(thread);
        PropertyAttributes attr(metaData);
        attr.SetDictionaryOrder(enumIndex);
        attr.SetRepresentation(Representation::TAGGED);
        // Check whether the table should be growed.
        JSHandle<Derived> newTable = HashTableT::GrowHashTable(thread, table);

        // Compute the key object.
        uint32_t hash = static_cast<uint32_t>(Derived::Hash(thread, key.GetTaggedValue()));
        entry = newTable->FindInsertIndex(thread, hash);
        newTable->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), attr);

        newTable->IncreaseEntries(thread);
        newTable->SetNextEnumerationIndex(thread, enumIndex + 1);
        return newTable;
    }

    static JSHandle<Derived> Put(const JSThread *thread, const JSHandle<Derived> &table,
                                 const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                 const PropertyAttributes &metaData)
    {
        int enumIndex = table->NextEnumerationIndex(thread);
        PropertyAttributes attr(metaData);
        attr.SetDictionaryOrder(enumIndex);
        attr.SetRepresentation(Representation::TAGGED);
        attr.SetDictSharedFieldType(metaData.GetSharedFieldType());
        int entry = table->FindEntry(thread, key.GetTaggedValue());
        if (entry != -1) {
            table->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), attr);
            return table;
        }

        // Check whether the table should be extended.
        JSHandle<Derived> newTable = HashTableT::GrowHashTable(thread, table);

        // Compute the key object.
        uint32_t hash = static_cast<uint32_t>(Derived::Hash(thread, key.GetTaggedValue()));
        entry = newTable->FindInsertIndex(thread, hash);
        newTable->SetEntry(thread, entry, key.GetTaggedValue(), value.GetTaggedValue(), attr);

        newTable->IncreaseEntries(thread);
        newTable->SetNextEnumerationIndex(thread, enumIndex + 1);
        return newTable;
    }
    static JSHandle<Derived> Remove(const JSThread *thread, const JSHandle<Derived> &table, int entry)
    {
        if (!(table->IsKey(table->GetKey(thread, entry)))) {
            return table;
        }
        table->ClearEntry(thread, entry);
        table->IncreaseHoleEntriesCount(thread);
        return Shrink(thread, table);
    }

    inline void SetNextEnumerationIndex(const JSThread *thread, int index)
    {
        HashTableT::Set(thread, NEXT_ENUMERATION_INDEX, JSTaggedValue(index));
    }
    inline int GetNextEnumerationIndex() const
    {
        return HashTableT::GetPrimitive(NEXT_ENUMERATION_INDEX).GetInt();
    }

    inline int NextEnumerationIndex(const JSThread *thread)
    {
        int index = GetNextEnumerationIndex();
        auto table = Derived::Cast(this);

        if (!PropertyAttributes::IsValidIndex(index)) {
            std::vector<int> indexOrder = GetEnumerationOrder(thread);
            int length = static_cast<int>(indexOrder.size());
            for (int i = 0; i < length; i++) {
                int oldIndex = indexOrder[i];
                int enumIndex = PropertyAttributes::INITIAL_PROPERTY_INDEX + i;
                PropertyAttributes attr = table->GetAttributes(thread, oldIndex);
                attr.SetDictionaryOrder(enumIndex);
                attr.SetRepresentation(Representation::TAGGED);
                table->SetAttributes(thread, oldIndex, attr);
            }
            index = PropertyAttributes::INITIAL_PROPERTY_INDEX + length;
        }
        return index;
    }

    inline std::vector<int> GetEnumerationOrder(const JSThread *thread)
    {
        std::vector<int> result;
        auto table = Derived::Cast(this);
        int size = table->Size();
        for (int i = 0; i < size; i++) {
            if (table->IsKey(table->GetKey(thread, i))) {
                result.push_back(i);
            }
        }
        std::sort(result.begin(), result.end(), [table, thread](int a, int b) {
            PropertyAttributes attrA = table->GetAttributes(thread, a);
            PropertyAttributes attrB = table->GetAttributes(thread, b);
            return attrA.GetDictionaryOrder() < attrB.GetDictionaryOrder();
        });
        return result;
    }

    static int ComputeCompactSize([[maybe_unused]] const JSThread *thread,
                                  [[maybe_unused]] const JSHandle<Derived> &table, int computeHashTableSize,
                                  [[maybe_unused]] int tableSize, [[maybe_unused]] int addedElements)
    {
        return computeHashTableSize;
    }

    static const int NEXT_ENUMERATION_INDEX = HashTableT::SIZE_INDEX + 1;
    static const int DEFAULT_ELEMENTS_NUMBER = 128;
    static constexpr int TABLE_HEADER_SIZE = 4;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_NEW_HASH_TABLE_H
