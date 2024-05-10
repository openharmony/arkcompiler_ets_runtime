/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BITSET_H
#define ECMASCRIPT_COMPILER_BITSET_H

#include <cstddef>
#include <cstdint>
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript::kungfu {
class BitSet {
public:
    static constexpr uint32_t BYTE_PER_WORD = sizeof(uint64_t);
    static constexpr uint32_t BYTE_PER_WORD_LOG2 = 3;
    static constexpr uint32_t BIT_PER_BYTE = 8;
    static constexpr uint32_t BIT_PER_WORD = BYTE_PER_WORD * BIT_PER_BYTE;
    static constexpr uint32_t BIT_PER_WORD_LOG2 = 6;
    static constexpr uint32_t BIT_PER_WORD_MASK = BIT_PER_WORD - 1;

    explicit BitSet(Chunk* chunk, size_t bitSize)
    {
        wordCount_ = SizeOf(bitSize);
        if (UseWords()) {
            data_.words_ = chunk->NewArray<uint64_t>(wordCount_);
            Reset();
        }
    }

    ~BitSet()
    {
        if (UseWords()) {
            // no need delete chunk memory
            data_.words_ = nullptr;
        }
    }

    void Reset()
    {
        if (!UseWords()) {
            data_.inlineWord_ = 0;
        } else {
            for (size_t i = 0; i < wordCount_; i++) {
                data_.words_[i] = 0;
            }
        }
    }

    bool TestBit(size_t offset) const
    {
        if (!UseWords()) {
            return data_.inlineWord_ & Mask(offset);
        }  else {
            ASSERT(wordCount_ > Index(offset));
            return data_.words_[Index(offset)] & Mask(IndexInWord(offset));
        }
    }

    void SetBit(size_t offset)
    {
        if (!UseWords()) {
            data_.inlineWord_ |= Mask(offset);
        } else {
            ASSERT(wordCount_ > Index(offset));
            data_.words_[Index(offset)] |= Mask(IndexInWord(offset));
        }
    }

    void ClearBit(size_t offset)
    {
        if (!UseWords()) {
            data_.inlineWord_ &= ~Mask(offset);
        } else {
            ASSERT(wordCount_ > Index(offset));
            data_.words_[Index(offset)] &= ~Mask(IndexInWord(offset));
        }
    }

    void Union(const BitSet &bitset)
    {
        if (!UseWords()) {
            data_.inlineWord_ |= bitset.data_.inlineWord_;
        } else {
            for (size_t i = 0; i < wordCount_; i++) {
                data_.words_[i] |= bitset.data_.words_[i];
            }
        }
    }

    bool UnionWithChanged(const BitSet &bitset)
    {
        if (!UseWords()) {
            auto oldValue = data_.inlineWord_;
            data_.inlineWord_ |= bitset.data_.inlineWord_;
            return data_.inlineWord_ != oldValue;
        } else {
            bool changed = false;
            for (size_t i = 0; i < wordCount_; i++) {
                auto oldValue = data_.words_[i];
                data_.words_[i] |= bitset.data_.words_[i];
                if (!changed && data_.words_[i] != oldValue) {
                    changed = true;
                }
            }
            return changed;
        }
    }

    void Intersect(const BitSet &bitset)
    {
        if (!UseWords()) {
            data_.inlineWord_ &= bitset.data_.inlineWord_;
        } else {
            for (size_t i = 0; i < wordCount_; i++) {
                data_.words_[i] &= bitset.data_.words_[i];
            }
        }
    }

    void CopyFrom(const BitSet &other)
    {
        ASSERT(wordCount_ == other.wordCount_);
        wordCount_ = other.wordCount_;
        if (!UseWords()) {
            data_.inlineWord_ = other.data_.inlineWord_;
            return;
        }
        for (size_t i = 0; i < wordCount_; i++) {
            data_.words_[i] = other.data_.words_[i];
        }
    }

    void CopyDataFrom(const BitSet &other)
    {
        ASSERT(wordCount_ >= other.wordCount_);
        if (!other.UseWords()) {
            if (UseWords()) {
                data_.words_[0] = other.data_.inlineWord_;
            } else {
                data_.inlineWord_ = other.data_.inlineWord_;
            }
            return;
        }
        for (size_t i = 0; i < other.wordCount_; i++) {
            data_.words_[i] = other.data_.words_[i];
        }
    }

    bool ShouldExpand(size_t size)
    {
        if (SizeOf(size) == this->wordCount_) {
            return false;
        }
        return true;
    }
private:
    union Data {
        uint64_t inlineWord_;
        uint64_t *words_ {nullptr};
    };
    static size_t SizeOf(size_t bitSize)
    {
        ASSERT(bitSize > 0);
        // +1: for word 1
        return ((bitSize - 1) >> BIT_PER_WORD_LOG2) + 1;
    }

    uint64_t Mask(size_t index) const
    {
        return uint64_t{1} << index;
    }

    size_t IndexInWord(size_t offset) const
    {
        return offset & BIT_PER_WORD_MASK;
    }

    size_t Index(size_t offset) const
    {
        return offset >> BIT_PER_WORD_LOG2;
    }

    size_t WordCount(size_t size) const
    {
        return size >> BYTE_PER_WORD_LOG2;
    }

    bool UseWords() const
    {
        return wordCount_ > 1;
    }

    uint32_t wordCount_ {0};
    Data data_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BITSET_H
