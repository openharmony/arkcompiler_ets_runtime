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

#ifndef ECMASCRIPT_TAGGED_ARRAY_H
#define ECMASCRIPT_TAGGED_ARRAY_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class ObjectFactory;
class JSThread;

class TaggedArray : public TaggedObject {
public:
    static constexpr uint32_t MAX_ARRAY_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t MAX_END_UNUSED = 4;

    CAST_CHECK(TaggedArray, IsTaggedArray);

    JSTaggedValue Get(uint32_t idx) const;

    uint32_t GetIdx(const JSTaggedValue &value) const;

    template<typename T>
    void Set(const JSThread *thread, uint32_t idx, const JSHandle<T> &value);

    JSTaggedValue Get(const JSThread *thread, uint32_t idx) const;

    template <bool needBarrier = true>
    void Set(const JSThread *thread, uint32_t idx, const JSTaggedValue &value);

    static inline JSHandle<TaggedArray> Append(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                               const JSHandle<TaggedArray> &second);
    static inline JSHandle<TaggedArray> AppendSkipHole(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                                       const JSHandle<TaggedArray> &second, uint32_t copyLength);

    static inline size_t ComputeSize(size_t elemSize, uint32_t length)
    {
        ASSERT(elemSize != 0);
        size_t size = DATA_OFFSET + elemSize * length;
        return size;
    }

    inline JSTaggedType *GetData() const
    {
        return reinterpret_cast<JSTaggedType *>(ToUintPtr(this) + DATA_OFFSET);
    }

    inline bool IsDictionaryMode() const;

    bool HasDuplicateEntry() const;

    static JSHandle<TaggedArray> SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                             uint32_t capa);

    static JSHandle<TaggedArray> SetCapacityInOldSpace(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                                       uint32_t capa);

    static inline void RemoveElementByIndex(const JSThread *thread, JSHandle<TaggedArray> &srcArray,
                                            uint32_t index, uint32_t effectiveLength);
    static inline void InsertElementByIndex(const JSThread *thread, JSHandle<TaggedArray> &srcArray,
        const JSHandle<JSTaggedValue> &value, uint32_t index, uint32_t effectiveLength);
    static inline void CopyTaggedArrayElement(const JSThread *thread, JSHandle<TaggedArray> &srcElements,
                                              JSHandle<TaggedArray> &dstElements, uint32_t effectiveLength);

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t length, uint32_t extraLength = 0);

    static inline bool ShouldTrim(uint32_t oldLength, uint32_t newLength)
    {
        return (oldLength - newLength > MAX_END_UNUSED);
    }
    inline void Trim(const JSThread *thread, uint32_t newLength);

    static constexpr size_t LENGTH_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, EXTRACT_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ExtraLength, uint32_t, EXTRACT_LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t DATA_OFFSET = SIZE;  // DATA_OFFSET equal to Empty Array size

    DECL_VISIT_ARRAY(DATA_OFFSET, GetLength());
    DECL_DUMP()

private:
    friend class ObjectFactory;
};

static_assert(TaggedArray::LENGTH_OFFSET == sizeof(TaggedObject));
static_assert((TaggedArray::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);

// Copy On Write TaggedArray is shared in the nonmovable space.
class COWTaggedArray : public TaggedArray {
public:
    CAST_CHECK(COWTaggedArray, IsCOWArray)
    DECL_DUMP()
private:
    friend class ObjectFactory;
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_ARRAY_H
