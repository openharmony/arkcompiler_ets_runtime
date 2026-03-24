/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_SEGMENT_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_SEGMENT_H

#include <cstddef>

#include "common_components/base/asan_interface.h"
#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/js_tagged_value_internals.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {

class SlotFreeSegment : public base::AlignedStruct<sizeof(JSTaggedType),
                                                   base::AlignedSize,
                                                   base::AlignedPointer> {
public:
    static constexpr size_t SLOT_FREE_SEGMENT_MASK_IN_SIZE = 1;
    enum class Index : size_t {
        MaybeEncodedSizeIndex = 0,
        NextFreeSegmentIndex,
    };
    static_assert(static_cast<size_t>(Index::MaybeEncodedSizeIndex) == 0);

    static size_t GetMaybeEncodedSizeIndex(bool isArch32)
    {
        static_assert(offsetof(SlotFreeSegment, maybeEncodedSize_) == 0);
        return GetOffset<static_cast<size_t>(Index::MaybeEncodedSizeIndex)>(isArch32);
    }

    static size_t GetNextFreeSegmentIndex(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::NextFreeSegmentIndex)>(isArch32);
    }

    static SlotFreeSegment *Cast(uintptr_t addr)
    {
        return reinterpret_cast<SlotFreeSegment *>(addr);
    }

    inline static SlotFreeSegment *FillFreeSegment(uintptr_t addr, uintptr_t size);

    bool IsSlotFreeSegment() const
    {
        return IsEncodedSize(maybeEncodedSize_);
    }

    size_t GetSize() const
    {
        return DecodeSize(maybeEncodedSize_);
    }

    void SetSize(size_t size)
    {
        ASSERT(size >= sizeof(SlotFreeSegment));
        ASSERT(size % sizeof(JSTaggedType) == 0);
        maybeEncodedSize_ = EncodeSize(size);
    }

    SlotFreeSegment *GetNextFreeSegment() const
    {
        return reinterpret_cast<SlotFreeSegment *>(nextFreeSegment_);
    }

    void SetNextFreeSegment(SlotFreeSegment *nextFreeSegment)
    {
        nextFreeSegment_ = reinterpret_cast<uintptr_t>(nextFreeSegment);
    }

    // Before operating any FreeSegment, need to mark unpoison when is_asan is true.
    void AsanUnPoisonFreeSegment() const
    {
        ASAN_UNPOISON_MEMORY_REGION(this, sizeof(SlotFreeSegment));
    }

    // After operating any FreeSegment, need to marked poison again when is_asan is true
    void AsanPoisonFreeSegment() const
    {
        ASAN_POISON_MEMORY_REGION(this, sizeof(SlotFreeSegment));
    }

private:
    static size_t EncodeSize(size_t size)
    {
        ASSERT(!IsEncodedSize(size));
        return size | SLOT_FREE_SEGMENT_MASK_IN_SIZE;
    }

    static size_t DecodeSize(size_t size)
    {
        ASSERT(IsEncodedSize(size));
        return size & ~SLOT_FREE_SEGMENT_MASK_IN_SIZE;
    }

    static size_t IsEncodedSize(size_t size)
    {
        return (size & SLOT_FREE_SEGMENT_MASK_IN_SIZE) != 0;
    }

    SlotFreeSegment() = delete;
    ~SlotFreeSegment() = default;

    alignas(EAS) size_t maybeEncodedSize_ {0};
    alignas(EAS) uintptr_t nextFreeSegment_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_SEGMENT_H