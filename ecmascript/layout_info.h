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

    static LayoutInfo* GetLayoutInfoFromHClass(const JSHClass* hclass)
    {
        return LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    }

    void Initialize(const JSThread *thread, int num = 0);
    int GetPropertiesCapacity() const;
    int NumberOfElements() const;
    void SetNumberOfElements(const JSThread *thread, int properties);
    uint32_t GetKeyIndex(int index) const;
    uint32_t GetAttrIndex(int index) const;
    void SetPropertyInit(const JSThread *thread, int index, const JSTaggedValue &key, const PropertyAttributes &attr);
    void SetKey(const JSThread *thread, int index, const JSTaggedValue &key);
    void SetNormalAttr(const JSThread *thread, int index, const PropertyAttributes &attr);
    JSTaggedValue GetKey(int index) const;
    PropertyAttributes GetAttr(int index) const;
    JSTaggedValue GetSortedKey(int index) const;
    uint32_t GetSortedIndex(int index) const;
    void SetSortedIndex(const JSThread *thread, int index, int sortedIndex);
    template<bool checkDuplicateKeys = false>
    void AddKey(const JSThread *thread, int index, const JSTaggedValue &key, const PropertyAttributes &attr);
    void SetIsNotHole(const JSThread *thread, int index);
    void UpdateTrackTypeAttr(int index, const PropertyAttributes &attr);
    void SetIsPGODumped(int index);

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
    int BinarySearch(JSTaggedValue key, int propertiesNumber);
    void GetAllKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray,
                    const JSHandle<JSObject> object);
    void GetAllKeysForSerialization(int end, std::vector<JSTaggedValue> &keyVector);
    void GetAllKeysByFilter(const JSThread *thread, uint32_t numberOfProps, uint32_t &keyArrayEffectivelength,
        TaggedArray *keyArray, const JSHandle<JSObject> object, uint32_t filter);
    std::pair<uint32_t, uint32_t> GetNumOfEnumKeys(int end, const JSObject *object) const;
    void GetAllEnumKeys(JSThread *thread, int end, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys,
                        JSHandle<TaggedQueue> shadowQueue, const JSHandle<JSObject> object,
                        int32_t lastLength);
    void GetAllEnumKeys(JSThread *thread, int end, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys,
                        const JSHandle<JSObject> object);

    void DumpFieldIndexByPGO(int index, pgo::HClassLayoutDesc* desc);
    bool UpdateFieldIndexByPGO(int index, pgo::HClassLayoutDesc* desc);
    CString GetSymbolKeyString(JSTaggedValue key);
    DECL_DUMP()

private:
    bool IsUninitializedProperty(const JSObject* object, uint32_t index) const;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_LAYOUT_INFO_H
