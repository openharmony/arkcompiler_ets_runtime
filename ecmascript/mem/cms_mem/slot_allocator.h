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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_ALLOCATOR_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_ALLOCATOR_H

#include <memory>

#include "ecmascript/mem/cms_mem/slot_free_list.h"

namespace panda::ecmascript {
class CMSRegionChainManager;

class SlotAllocator {
public:
    SlotAllocator() = default;
    ~SlotAllocator() = default;

    NO_COPY_SEMANTIC(SlotAllocator);
    NO_MOVE_SEMANTIC(SlotAllocator);

    size_t GetSlotSize() const
    {
        return freeList_.GetSlotSize();
    }

    void Initialize(size_t slotSize, CMSRegionChainManager *regionChainManager);

    template <typename FreeListSoldOutCallback>
    inline ARK_INLINE uintptr_t TryAllocate(FreeListSoldOutCallback &&freeListSoldOutCallback);

    void Expand(Region *region);

    /**
     * @brief Discard the remain allocate buffer
     * @return allocated size from this allocate buffer
     */
    size_t Discard();

    inline std::pair<uintptr_t, size_t> TryAllocateBuffer(size_t bufferSize);
private:
    size_t RecordFreeListSoldOut();

    inline ARK_INLINE void ResetFreeList(SlotFreeListMetaInfo slotFreeListMetaInfo);

    SlotFreeList freeList_ {};
    CMSRegionChainManager *regionChainManager_ {nullptr};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_ALLOCATOR_H
