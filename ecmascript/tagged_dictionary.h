/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TAGGED_DICTIONARY_H
#define ECMASCRIPT_TAGGED_DICTIONARY_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_hash_table.h"

namespace panda::ecmascript {
class NameDictionary : public OrderTaggedHashTable<NameDictionary> {
public:
    using OrderHashTableT = OrderTaggedHashTable<NameDictionary>;
    inline static int GetKeyIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_KEY_INDEX;
    }
    inline static int GetValueIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_VALUE_INDEX;
    }
    inline static int GetEntryIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize();
    }
    inline static int GetEntrySize()
    {
        return ENTRY_SIZE;
    }
    static int PUBLIC_API Hash(const JSTaggedValue &key);
    static int Hash(const uint8_t* str, int strSize);

    static bool PUBLIC_API IsMatch(const JSTaggedValue &key, const JSTaggedValue &other);
    static bool IsMatch(const uint8_t* str, int size, const JSTaggedValue &other);

    static JSHandle<NameDictionary> Create(const JSThread *thread,
        int numberOfElements = OrderHashTableT::DEFAULT_ELEMENTS_NUMBER);
    static JSHandle<NameDictionary> PUBLIC_API CreateInSharedHeap(const JSThread *thread,
        int numberOfElements = OrderHashTableT::DEFAULT_ELEMENTS_NUMBER);
    // Returns the property metaData for the property at entry.
    PropertyAttributes PUBLIC_API GetAttributes(int entry) const;
    void PUBLIC_API SetAttributes(const JSThread *thread, int entry, const PropertyAttributes &metaData);
    void PUBLIC_API SetEntry(const JSThread *thread, int entry, const JSTaggedValue &key, const JSTaggedValue &value,
                             const PropertyAttributes &metaData);
    void UpdateValueAndAttributes(const JSThread *thread, int entry, const JSTaggedValue &value,
                                  const PropertyAttributes &metaData);
    void PUBLIC_API UpdateValue(const JSThread *thread, int entry, const JSTaggedValue &value);
    void UpdateAttributes(int entry, const PropertyAttributes &metaData);
    void UpdateAllAttributesToNoWitable(const JSThread *thread);
    void ClearEntry(const JSThread *thread, int entry);
    void GetAllKeys(const JSThread *thread, int offset, TaggedArray *keyArray) const;
    void GetAllKeysByFilter(const JSThread *thread, uint32_t &keyArrayEffectivelength,
        TaggedArray *keyArray, uint32_t filter) const;
    std::pair<uint32_t, uint32_t> GetNumOfEnumKeys() const;
    void GetAllEnumKeys(JSThread *thread, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys,
                        JSHandle<TaggedQueue> shadowQueue, int32_t lastLength) const;
    void GetAllEnumKeys(JSThread *thread, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys) const;
    static inline bool CompHandleKey(const std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> &a,
                                     const std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> &b)
    {
        return a.second.GetDictionaryOrder() < b.second.GetDictionaryOrder();
    }
    static inline bool CompKey(const std::pair<JSTaggedValue, PropertyAttributes> &a,
                               const std::pair<JSTaggedValue, PropertyAttributes> &b)
    {
        return a.second.GetDictionaryOrder() < b.second.GetDictionaryOrder();
    }
    static inline bool CompIndex(const PropertyAttributes &a,
                                 const PropertyAttributes &b)
    {
        return a.GetDictionaryOrder() < b.GetDictionaryOrder();
    }
    void Dump(std::ostream &os, bool isPrivacy = false) const DUMP_API_ATTR;
    void Dump() const DUMP_API_ATTR
    {
        Dump(std::cout);
    }
    void DumpForSnapshot(std::vector<Reference> &vec) const;

    static constexpr int ENTRY_KEY_INDEX = 0;
    static constexpr int ENTRY_VALUE_INDEX = 1;
    static constexpr int ENTRY_DETAILS_INDEX = 2;
    static constexpr int ENTRY_SIZE = 3;
};

class NumberDictionary : public OrderTaggedHashTable<NumberDictionary> {
public:
    using OrderHashTableT = OrderTaggedHashTable<NumberDictionary>;
    inline static int GetKeyIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_KEY_INDEX;
    }
    inline static int GetValueIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_VALUE_INDEX;
    }
    inline static int GetEntryIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize();
    }
    inline static int GetEntrySize()
    {
        return ENTRY_SIZE;
    }
    static int PUBLIC_API Hash(const JSTaggedValue &key);
    static bool PUBLIC_API IsMatch(const JSTaggedValue &key, const JSTaggedValue &other);
    static JSHandle<NumberDictionary> Create(const JSThread *thread,
                                    int numberOfElements = OrderHashTableT::DEFAULT_ELEMENTS_NUMBER);
    static JSHandle<NumberDictionary> CreateInSharedHeap(
        const JSThread *thread, int numberOfElements = OrderHashTableT::DEFAULT_ELEMENTS_NUMBER);
    // Returns the property metaData for the property at entry.
    PropertyAttributes PUBLIC_API GetAttributes(int entry) const;
    void SetAttributes(const JSThread *thread, int entry, const PropertyAttributes &metaData);
    void SetEntry([[maybe_unused]] const JSThread *thread, int entry, const JSTaggedValue &key,
                  const JSTaggedValue &value, const PropertyAttributes &metaData);
    void UpdateValueAndAttributes(const JSThread *thread, int entry, const JSTaggedValue &value,
                                  const PropertyAttributes &metaData);
    void PUBLIC_API UpdateValue(const JSThread *thread, int entry, const JSTaggedValue &value);
    void UpdateAttributes(int entry, const PropertyAttributes &metaData);
    void ClearEntry(const JSThread *thread, int entry);

    static void GetAllKeys(const JSThread *thread, const JSHandle<NumberDictionary> &obj, int offset,
                           const JSHandle<TaggedArray> &keyArray);
    static void GetAllKeysByFilter(const JSThread *thread, const JSHandle<NumberDictionary> &obj,
        uint32_t &keyArrayEffectivelength, const JSHandle<TaggedArray> &keyArray, uint32_t filter);
    static void GetAllEnumKeys(JSThread *thread, const JSHandle<NumberDictionary> &obj, int offset,
                               const JSHandle<TaggedArray> &keyArray, uint32_t *keys, int32_t lastLength = -1);
    static inline bool CompKey(const JSTaggedValue &a, const JSTaggedValue &b)
    {
        ASSERT(a.IsNumber() && b.IsNumber());
        return a.GetNumber() < b.GetNumber();
    }
    void Dump(std::ostream &os, bool isPrivacy = false) const DUMP_API_ATTR;
    void Dump() const DUMP_API_ATTR
    {
        Dump(std::cout);
    }
    void DumpForSnapshot(std::vector<Reference> &vec) const;

    static constexpr int ENTRY_KEY_INDEX = 0;
    static constexpr int ENTRY_VALUE_INDEX = 1;
    static constexpr int ENTRY_DETAILS_INDEX = 2;
    static constexpr int ENTRY_SIZE = 3;
};

class PointerToIndexDictionary : public OrderTaggedHashTable<PointerToIndexDictionary> {
public:
    using OrderHashTableT = OrderTaggedHashTable<PointerToIndexDictionary>;
    inline static int GetKeyIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_KEY_INDEX;
    }
    
    inline static int GetValueIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize() + ENTRY_VALUE_INDEX;
    }

    inline static int GetEntryIndex(int entry)
    {
        return OrderHashTableT::TABLE_HEADER_SIZE + entry * GetEntrySize();
    }
    
    inline static int GetEntrySize()
    {
        return ENTRY_SIZE;
    }

    inline void SetEntry(const JSThread *thread, int entry, const JSTaggedValue &key, const JSTaggedValue &value)
    {
        SetKey(thread, entry, key);
        SetValue(thread, entry, value);
    }

    static int32_t Hash(const JSTaggedValue &key)
    {
        return static_cast<int32_t>(key.GetRawData());
    }

    static bool IsMatch(const JSTaggedValue &key, const JSTaggedValue &other)
    {
        return key == other;
    }
    static JSHandle<PointerToIndexDictionary> Create(const JSThread *thread,
                                        int numberOfElements = OrderHashTableT::DEFAULT_ELEMENTS_NUMBER);
    static JSHandle<PointerToIndexDictionary> PutIfAbsent(
                                        const JSThread *thread, const JSHandle<PointerToIndexDictionary> &dictionary,
                                        const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);

    // DECL_DUMP()

    static constexpr int ENTRY_KEY_INDEX = 0;
    static constexpr int ENTRY_VALUE_INDEX = 1;
    static constexpr int ENTRY_SIZE = 2;
};

class FunctionProtoTransitionTable {
public:
    explicit FunctionProtoTransitionTable(const JSThread *thread);
    virtual ~FunctionProtoTransitionTable();

    void Iterate(const RootVisitor &v);

    void InsertTransitionItem(const JSThread *thread,
                              JSHandle<JSTaggedValue> ihc,
                              JSHandle<JSTaggedValue> baseIhc,
                              JSHandle<JSTaggedValue> transIhc,
                              JSHandle<JSTaggedValue> transPhc);

    std::pair<JSTaggedValue, JSTaggedValue> FindTransitionByHClass(const JSThread *thread,
                                                                   JSHandle<JSTaggedValue> ihc,
                                                                   JSHandle<JSTaggedValue> baseIhc) const;

    bool TryInsertFakeParentItem(JSTaggedType child, JSTaggedType parent);
    JSTaggedType GetFakeParent(JSTaggedType child);

    JSTaggedValue GetProtoTransitionTable() const
    {
        return protoTransitionTable_;
    }

    void UpdateProtoTransitionTable(const JSThread *thread, JSHandle<PointerToIndexDictionary> map);

    class ProtoArray : public TaggedArray {
    public:
        CAST_CHECK(ProtoArray, IsTaggedArray);

        static JSHandle<ProtoArray> Create(const JSThread *thread);
        void SetEntry(const JSThread *thread, uint32_t index, JSTaggedValue key, JSTaggedValue transIhc,
                      JSTaggedValue transPhc);
        JSTaggedValue GetKey(uint32_t index) const;
        std::pair<JSTaggedValue, JSTaggedValue> GetTransition(uint32_t index) const;
        int32_t FindEntry(JSTaggedValue key) const;
        std::pair<JSTaggedValue, JSTaggedValue> FindTransition(JSTaggedValue key) const;
        static bool GetEntry(JSHandle<JSHClass> hclass);

    private:
        static constexpr uint32_t ENTRY_NUMBER = 2;
        static constexpr uint32_t INIT_ENTRY_INDEX = 0;
        static constexpr uint32_t CLONED_ENTRY_INDEX = 1;

        static constexpr uint32_t ENTRY_SIZE = 3;
        static constexpr uint32_t KEY_INDEX = 0;
        static constexpr uint32_t TRANS_IHC_INDEX = 1;
        static constexpr uint32_t TRANS_PHC_INDEX = 2;
    };

private:
    static constexpr uint32_t DEFAULT_FIRST_LEVEL_SIZE = 2;
    // second level table:
    // <PointerToIndexDictionary(n):
    //      [ihc,
    //          <ProtoArray(2):
    //              [base.ihc,
    //                  (trans_ihc, trans_phc)
    //              ]
    //              [base.ihc(cloned),
    //                  (trans_ihc, trans_phc)
    //              ]
    //          >
    //      ]
    //      ...
    //  >
    JSTaggedValue protoTransitionTable_ {JSTaggedValue::Hole()};
    // hclasses after set prototype maps to hclasses before set prototype
    CUnorderedMap<JSTaggedType, JSTaggedType> fakeParentMap_ {};
    // reverse fakeParentMap_
    CUnorderedMap<JSTaggedType, JSTaggedType> fakeChildMap_ {};
    Mutex fakeParentMutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_NEW_DICTIONARY_H
