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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_LIST_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_LIST_H

#include "ecmascript/mem/cms_mem/slot_free_list_meta_info.h"
#include "ecmascript/mem/cms_mem/slot_space_config.h"

namespace panda::ecmascript {

class SlotFreeList : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                base::AlignedPointer,
                                                base::AlignedPointer,
                                                base::AlignedPointer,
                                                base::AlignedSize,
                                                base::AlignedSize> {
public:
    static size_t GetTopOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::TopIndex)>(isArch32);
    }

    static size_t GetEndOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::EndIndex)>(isArch32);
    }

    static size_t GetNextTopOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::NextTopIndex)>(isArch32);
    }

    static size_t GetSlotSizeOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::SlotSizeIndex)>(isArch32);
    }

    static size_t GetTotalUsableSizeOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::TotalUsableSizeIndex)>(isArch32);
    }

    SlotFreeList() = default;
    ~SlotFreeList() = default;

    NO_COPY_SEMANTIC(SlotFreeList);
    NO_MOVE_SEMANTIC(SlotFreeList);

    inline void Initialize(size_t slotSize);

    size_t GetSlotSize() const
    {
        return slotSize_;
    }

    size_t GetTotalUsableSize() const
    {
        return totalUsableSize_;
    }

    inline bool IsSoldOut() const;

    inline void Reset(SlotFreeListMetaInfo slotFreeListMetaInfo);

    /**
     * \brief Discard the remain slots
     * @return Allocated size from this free list
     */
    inline size_t Discard();

    inline uintptr_t TryAllocate();

    inline std::pair<uintptr_t, size_t> TryAllocateBuffer(size_t bufferSize);

private:
    enum class Index : size_t {
        TopIndex = 0,
        EndIndex,
        NextTopIndex,
        SlotSizeIndex,
        TotalUsableSizeIndex,
    };

    inline bool HasNextSegment() const;

    inline void FetchAndUpdateSegment(SlotFreeSegment *freeSegment);

    alignas(EAS) uintptr_t top_ {0};
    alignas(EAS) uintptr_t end_ {0};
    alignas(EAS) uintptr_t nextTop_ {0};
    alignas(EAS) size_t slotSize_ {0};
    alignas(EAS) size_t totalUsableSize_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_LIST_H
