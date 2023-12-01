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

#ifndef ECMASCRIPT_BYTE_ARRAY_H
#define ECMASCRIPT_BYTE_ARRAY_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_dataview.h"

namespace panda::ecmascript {
class ObjectFactory;
class JSThread;

class ByteArray : public TaggedObject {
public:
    CAST_CHECK(ByteArray, IsByteArray);

    static inline size_t ComputeSize(size_t elemSize, uint32_t length)
    {
        ASSERT(elemSize != 0);
        size_t size = DATA_OFFSET + elemSize * length;
        return size;
    }

    static inline size_t ComputeDataSize(size_t elemSize, uint32_t length)
    {
        ASSERT(elemSize != 0);
        return elemSize * length;
    }

    size_t GetPointerLength()
    {
        size_t byteSize = ComputeDataSize(GetArrayLength(), GetByteLength());
        return AlignUp(byteSize, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) / sizeof(JSTaggedType);
    }

    inline void *GetData(uint32_t index = 0) const
    {
        return reinterpret_cast<void *>(ToUintPtr(this) + DATA_OFFSET + index * GetByteLength());
    }

    void Set(JSThread* thread, uint32_t idx, DataViewType type, JSTaggedType val, uint32_t offset = 0);
    JSTaggedValue Get(JSThread *thread, uint32_t idx, DataViewType type, uint32_t offset = 0);

    static constexpr size_t ARRAY_LENGTH_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(ArrayLength, uint32_t, ARRAY_LENGTH_OFFSET, BYTE_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ByteLength, uint32_t, BYTE_LENGTH_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t DATA_OFFSET = SIZE;  // DATA_OFFSET equal to Empty ByteArray size

    DECL_VISIT_ARRAY(DATA_OFFSET, 0, GetPointerLength());
    DECL_DUMP()

private:
    friend class ObjectFactory;
};

static_assert(ByteArray::ARRAY_LENGTH_OFFSET == sizeof(TaggedObject));
static_assert((ByteArray::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_BYTE_ARRAY_H
