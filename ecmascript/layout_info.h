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

#ifndef ECMASCRIPT_LAYOUT_INFO_H
#define ECMASCRIPT_LAYOUT_INFO_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
struct Properties {
    JSTaggedValue key_;
    JSTaggedValue attr_;
};

class LayoutInfo : private TaggedArray {
public:
    static constexpr int MIN_PROPERTIES_LENGTH = JSObject::MIN_PROPERTIES_LENGTH;
    static constexpr int MAX_PROPERTIES_LENGTH = PropertyAttributes::MAX_FAST_PROPS_CAPACITY;
    static constexpr uint32_t ELEMENTS_INDEX_LOG2 = 1;
    static constexpr uint32_t ATTR_INDEX_OFFSET = 1;

    inline static LayoutInfo *Cast(TaggedObject *obj)
    {
        ASSERT(JSTaggedValue(obj).IsTaggedArray());
        return reinterpret_cast<LayoutInfo *>(obj);
    }

    inline static LayoutInfo *UncheckCast(TaggedObject *obj)
    {
        return reinterpret_cast<LayoutInfo *>(obj);
    }

    static LayoutInfo* GetLayoutInfoFromHClass(const JSThread *thread, const JSHClass* hclass)
    {
        return LayoutInfo::Cast(hclass->GetLayout(thread).GetTaggedObject());
    }

    void Initialize(const JSThread *thread, int num = 0);
    int GetPropertiesCapacity() const;
    int NumberOfElements() const;
    void SetNumberOfElements(const JSThread *thread, int properties);
    uint32_t GetKeyIndex(int index) const;
    uint32_t GetAttrIndex(int index) const;
    void SetWithoutBarrier(uint32_t idx, const JSTaggedValue &value);
    void SetPropertyInit(const JSThread *thread, int index, const JSTaggedValue &key, const PropertyAttributes &attr);
    void SetKey(const JSThread *thread, int index, const JSTaggedValue &key);
    void SetNormalAttr(const JSThread *thread, int index, const PropertyAttributes &attr);
    JSTaggedValue GetKey(const JSThread *thread, int index) const;
    PropertyAttributes GetAttr(const JSThread *thread, int index) const;
    template<RBMode mode = RBMode::DEFAULT_RB>
    PropertyAttributes GetAttr(const JSThread *thread, int index) const;
    JSTaggedValue GetSortedKey(const JSThread *thread, int index) const;
    uint32_t GetSortedIndex(const JSThread *thread, int index) const;
    void SetSortedIndex(const JSThread *thread, int index, int sortedIndex);
    bool CheckIsDuplicateKey(const JSThread *thread, int curKeyIdx, uint32_t curKeyHashCode, const JSTaggedValue &key);
    template<bool checkDuplicateKeys = false>
    void AddKey(const JSThread *thread, int index, const JSTaggedValue &key, const PropertyAttributes &attr);
    void SetIsNotHole(const JSThread *thread, int index);
    void UpdateTrackTypeAttr(const JSThread *thread, int index, const PropertyAttributes &attr);
    void SetIsPGODumped(const JSThread *thread, int index);

    inline uint32_t GetLength() const
    {
        return TaggedArray::GetLength();
    }

    inline Properties *GetProperties() const
    {
        return reinterpret_cast<Properties *>(reinterpret_cast<uintptr_t>(this) + TaggedArray::DATA_OFFSET);
    }

    static inline uint32_t ComputeArrayLength(uint32_t properties_number)
    {
        return (properties_number << ELEMENTS_INDEX_LOG2);
    }

    static inline uint32_t ComputeGrowCapacity(uint32_t old_capacity)
    {
        uint32_t new_capacity = old_capacity + MIN_PROPERTIES_LENGTH;
        return new_capacity > MAX_PROPERTIES_LENGTH ? MAX_PROPERTIES_LENGTH : new_capacity;
    }

    int FindElementWithCache(const JSThread *thread, JSHClass *cls, JSTaggedValue key, int propertiesNumber);
    int BinarySearch(const JSThread *thread, JSTaggedValue key, int propertiesNumber);
    void GetAllKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray);
    void GetAllKeysForSerialization(const JSThread *thread, int end, std::vector<JSTaggedValue> &keyVector);
    void GetAllKeysByFilter(const JSThread *thread, uint32_t numberOfProps, uint32_t &keyArrayEffectivelength,
                            TaggedArray *keyArray, uint32_t filter);
    std::pair<uint32_t, uint32_t> GetNumOfEnumKeys(const JSThread *thread, int end) const;
    void GetAllEnumKeys(JSThread *thread, int end, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys,
                        JSHandle<TaggedQueue> shadowQueue, int32_t lastLength);
    void GetAllEnumKeys(JSThread *thread, int end, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys);

    void DumpFieldIndexByPGO(const JSThread *thread, int index, pgo::HClassLayoutDesc* desc);
    bool UpdateFieldIndexByPGO(const JSThread *thread, int index, pgo::HClassLayoutDesc* desc);
    CString GetSymbolKeyString(const JSThread *thread, JSTaggedValue key);
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_LAYOUT_INFO_H
