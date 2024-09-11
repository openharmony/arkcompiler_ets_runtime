/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SERIALIZER_SERIALIZATION_CHUNK_H
#define ECMASCRIPT_SERIALIZER_SERIALIZATION_CHUNK_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {

class SerializationChunk {
public:
    explicit SerializationChunk() {}
    ~SerializationChunk()
    {
        if (begin_ != 0U) {
            free(ToVoidPtr(begin_));
        }
    }
    NO_COPY_SEMANTIC(SerializationChunk);
    NO_MOVE_SEMANTIC(SerializationChunk);

    void Emplace(JSTaggedType value)
    {
        if (top_ == end_) {
            Expand();
        }
        ASSERT(top_ < end_);
        *reinterpret_cast<JSTaggedType *>(top_) = value;
        top_ += JSTaggedValue::TaggedTypeSize();
    }

    JSTaggedType Get(size_t index) const
    {
        ASSERT(begin_ + index * JSTaggedValue::TaggedTypeSize() < top_);
        return *reinterpret_cast<JSTaggedType *>(begin_ + index * JSTaggedValue::TaggedTypeSize());
    }

    void Expand()
    {
        ASSERT(top_ == end_);
        size_t oldCapacity = end_ - begin_;
        size_t newCapacity = 0;
        if (oldCapacity == 0U) {
            newCapacity = INITIAL_CHUNK_CAPACITY;
        } else {
            newCapacity = (end_ - begin_) * CHUNK_CAPACITY_INCREASE_RATE;
        }
        void *newChunk = malloc(newCapacity);
        if (newChunk == nullptr) {
            LOG_ECMA_MEM(FATAL) << "malloc failed, current alloc size = " << newCapacity;
            UNREACHABLE();
        }

        if (begin_ != 0U) {
            if (memcpy_s(newChunk, newCapacity, ToVoidPtr(begin_), end_ - begin_) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s fail";
            }
            free(ToVoidPtr(begin_));
        }
        begin_ = ToUintPtr(newChunk);
        top_ = begin_ + oldCapacity;
        end_ = begin_ + newCapacity;
    }

    bool Empty() const
    {
        return top_ == begin_;
    }

    size_t Size() const
    {
        return (top_ - begin_) / JSTaggedValue::TaggedTypeSize();
    }

    void Iterate(const RootVisitor &v)
    {
        for (uintptr_t slot = begin_; slot < top_; slot += JSTaggedValue::TaggedTypeSize()) {
            v(Root::ROOT_VM, ObjectSlot(slot));
        }
    }

private:
    static constexpr size_t INITIAL_CHUNK_CAPACITY = 1_KB;
    static constexpr int CHUNK_CAPACITY_INCREASE_RATE = 2;
    uintptr_t begin_ {0U};
    uintptr_t end_ {0U};
    uintptr_t top_ {0U};
};
}

#endif  // ECMASCRIPT_SERIALIZER_SERIALIZE_DATA_H
