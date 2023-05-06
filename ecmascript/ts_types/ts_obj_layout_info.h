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

#ifndef ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_H
#define ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_H

#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
/*
 * The TSObjLayoutInfo is organized as follows:
 * The first position is used to store the number of properties.
 * Store the key and gt of properties alternately starting from the second position.
 * +---+-------+------+-------+------+-----+-------+------+
 * | N | key_1 | gt_1 | key_2 | gt_2 | ... | key_N | gt_N |
 * +---+-------+------+-------+------+-----+-------+------+
 */
class TSObjLayoutInfo : private TaggedArray {
public:
    static constexpr int MIN_PROPERTIES_LENGTH = JSObject::MIN_PROPERTIES_LENGTH;
    static constexpr int MAX_PROPERTIES_LENGTH = PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES;
    static constexpr int ELEMENTS_COUNT_INDEX = 0;
    static constexpr int ELEMENTS_START_INDEX = 1;
    static constexpr int ENTRY_SIZE = 2;
    static constexpr int ENTRY_KEY_OFFSET = 0;
    static constexpr int ENTRY_TYPE_OFFSET = 1;
    static constexpr int DEFAULT_CAPACITY = 1;
    static constexpr int INCREASE_CAPACITY_RATE = 1;

    inline static TSObjLayoutInfo *Cast(TaggedObject *obj)
    {
        ASSERT(JSTaggedValue(obj).IsTaggedArray());
        return reinterpret_cast<TSObjLayoutInfo*>(obj);
    }

    inline uint32_t GetPropertiesCapacity() const
    {
        ASSERT(GetLength() >= ELEMENTS_START_INDEX);
        ASSERT((GetLength() - ELEMENTS_START_INDEX) % 2 == 0);  // 2: even number
        return static_cast<uint32_t>((GetLength() - ELEMENTS_START_INDEX) / ENTRY_SIZE);
    }

    inline void SetNumOfProperties(const JSThread *thread, const uint32_t propertiesNum)
    {
        return TaggedArray::Set(thread, ELEMENTS_COUNT_INDEX, JSTaggedValue(propertiesNum));
    }

    inline uint32_t GetNumOfProperties() const
    {
        return TaggedArray::Get(ELEMENTS_COUNT_INDEX).GetInt();
    }

    inline JSTaggedValue GetKey(int index) const
    {
        uint32_t idxInArray = GetKeyIndex(index);
        return TaggedArray::Get(idxInArray);
    }

    inline JSTaggedValue GetTypeId(int index) const
    {
        uint32_t idxInArray = GetTypeIdIndex(index);
        return TaggedArray::Get(idxInArray);
    }

    void AddKeyAndType(const JSThread *thread, const JSTaggedValue &key, const JSTaggedValue &typeIdVal);

    static JSHandle<TSObjLayoutInfo> PushBack(const JSThread *thread,
                                              const JSHandle<TSObjLayoutInfo> &oldLayout,
                                              const JSHandle<JSTaggedValue> &key,
                                              const JSHandle<JSTaggedValue> &value);

    inline uint32_t GetLength() const
    {
        return TaggedArray::GetLength();
    }

    static inline uint32_t ComputeGrowCapacity(const uint32_t oldCapacity)
    {
        uint64_t newCapacity = (static_cast<uint64_t>(oldCapacity) << INCREASE_CAPACITY_RATE);
        ASSERT(newCapacity <= static_cast<uint64_t>(MAX_PROPERTIES_LENGTH));
        if (newCapacity > static_cast<uint64_t>(MAX_PROPERTIES_LENGTH)) {
            return MAX_PROPERTIES_LENGTH;
        }
        return static_cast<uint32_t>(newCapacity);
    }

    static inline uint32_t ComputeArrayLength(const uint32_t numOfProperties)
    {
        return (numOfProperties * ENTRY_SIZE) + ELEMENTS_START_INDEX;
    }

    bool Find(JSTaggedValue key) const;

    int GetElementIndexByKey(JSTaggedValue key) const;

    JSTaggedValue TryGetTypeByIndexSign(const uint32_t typeId);

private:
    inline void SetKey(const JSThread *thread, int index, const JSTaggedValue &key)
    {
        uint32_t idxInArray = GetKeyIndex(index);
        TaggedArray::Set(thread, idxInArray, key);
    }

    inline void SetTypeId(const JSThread *thread, int index, const JSTaggedValue &tsTypeId)
    {
        uint32_t idxInArray = GetTypeIdIndex(index);
        TaggedArray::Set(thread, idxInArray, tsTypeId);
    }

    inline uint32_t GetKeyIndex(int index) const
    {
        return ELEMENTS_START_INDEX + (static_cast<uint32_t>(index) * ENTRY_SIZE) + ENTRY_KEY_OFFSET;
    }

    inline uint32_t GetTypeIdIndex(int index) const
    {
        return ELEMENTS_START_INDEX + (static_cast<uint32_t>(index) * ENTRY_SIZE) + ENTRY_TYPE_OFFSET;
    }

    static JSHandle<TSObjLayoutInfo> ExtendTSObjLayoutInfo(const JSThread *thread,
                                                           const JSHandle<TSObjLayoutInfo> &old,
                                                           JSTaggedValue initVal = JSTaggedValue::Hole());
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_OBJ_LAYOUT_INFO_H
