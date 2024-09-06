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

#ifndef ECMASCRIPT_TAGGED_ARRAY_H
#define ECMASCRIPT_TAGGED_ARRAY_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class ObjectFactory;
class JSThread;

class TaggedArray : public TaggedObject {
public:
    static constexpr uint32_t MAX_ARRAY_INDEX = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t MAX_END_UNUSED = 4;

    CAST_CHECK(TaggedArray, IsTaggedArray);

    JSTaggedValue Get(uint32_t idx) const;

    JSTaggedValue Get([[maybe_unused]] const JSThread *thread, uint32_t idx) const;

    uint32_t GetIdx(const JSTaggedValue &value) const;
    JSTaggedValue GetBit(uint32_t idx, uint32_t bitOffset) const;

    template<typename T>
    inline void Set(const JSThread *thread, uint32_t idx, const JSHandle<T> &value);

    template <bool needBarrier = true>
    inline void Set(const JSThread *thread, uint32_t idx, const JSTaggedValue &value);

    template <bool needBarrier = true>
    inline void Copy(const JSThread* thread, uint32_t dstStart, uint32_t srcStart,
                                  const TaggedArray *srcArray, uint32_t count);

    void Set(uint32_t idx, const JSTaggedValue &value);
    void SetBit(const JSThread* thread, uint32_t idx, uint32_t bitOffset, const JSTaggedValue& value);

    static JSHandle<TaggedArray> Append(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                               const JSHandle<TaggedArray> &second);
    static JSHandle<TaggedArray> AppendSkipHole(const JSThread *thread, const JSHandle<TaggedArray> &first,
                                                       const JSHandle<TaggedArray> &second, uint32_t copyLength);

    static size_t ComputeSize(size_t elemSize, uint32_t length)
    {
        ASSERT(elemSize != 0);
        size_t size = DATA_OFFSET + elemSize * length;
        return size;
    }

    JSTaggedType *GetData() const
    {
        return reinterpret_cast<JSTaggedType *>(ToUintPtr(this) + DATA_OFFSET);
    }

    bool IsDictionaryMode() const;

    bool HasDuplicateEntry() const;

    bool IsGeneralNewAndNotMarking(const JSThread *thread);

    static JSHandle<TaggedArray> SetCapacity(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                             uint32_t capa);

    static JSHandle<TaggedArray> SetCapacityInOldSpace(const JSThread *thread, const JSHandle<TaggedArray> &array,
                                                       uint32_t capa);

    static void RemoveElementByIndex(const JSThread *thread, JSHandle<TaggedArray> &srcArray,
                                            uint32_t index, uint32_t effectiveLength, bool noNeedBarrier = false);
    static void InsertElementByIndex(const JSThread *thread, JSHandle<TaggedArray> &srcArray,
        const JSHandle<JSTaggedValue> &value, uint32_t index, uint32_t effectiveLength);
    static void CopyTaggedArrayElement(const JSThread *thread, JSHandle<TaggedArray> &srcElements,
                                              JSHandle<TaggedArray> &dstElements, uint32_t effectiveLength);

    void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t length, uint32_t extraLength = 0);
    void FillRangeWithSpecialValue(JSTaggedValue initValue, uint32_t start, uint32_t end);

    static bool ShouldTrim(uint32_t oldLength, uint32_t newLength)
    {
        ASSERT(oldLength >= newLength);
        return (oldLength - newLength > MAX_END_UNUSED);
    }
    void Trim(const JSThread *thread, uint32_t newLength);

    static constexpr size_t LENGTH_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, EXTRA_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ExtraLength, uint32_t, EXTRA_LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t DATA_OFFSET = SIZE;  // DATA_OFFSET equal to Empty Array size

    DECL_VISIT_ARRAY(DATA_OFFSET, GetLength(), GetLength());
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

// A Mutant of TaggedArray which has numbers directly stored in Data section.
// Used by JSArrays with specified elementsKind.
class MutantTaggedArray : public TaggedArray {
public:
    void InitializeWithSpecialValue(JSTaggedType initValue, uint32_t length, uint32_t extraLength = 0);

    DECL_VISIT_ARRAY(DATA_OFFSET, 0, GetLength());
    CAST_CHECK(MutantTaggedArray, IsMutantTaggedArray)
    DECL_DUMP()
private:
    friend class ObjectFactory;
};

// Copy On Write MutantTaggedArray is shared in the nonmovable space.
// With raw numbers stored in Data section.
class COWMutantTaggedArray : public MutantTaggedArray {
public:

    DECL_VISIT_ARRAY(DATA_OFFSET, 0, GetLength());
    CAST_CHECK(COWMutantTaggedArray, IsCOWArray)
    DECL_DUMP()
private:
    friend class ObjectFactory;
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_ARRAY_H
