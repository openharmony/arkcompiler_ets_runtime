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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_GC_ALLOCATOR_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_GC_ALLOCATOR_H

#include "ecmascript/mem/cms_mem/slot_allocator.h"

namespace panda::ecmascript {
class SlotSpace;

class SlotGCAllocator {
public:
    inline SlotGCAllocator();
    inline ~SlotGCAllocator();

    NO_COPY_SEMANTIC(SlotGCAllocator);
    NO_MOVE_SEMANTIC(SlotGCAllocator);

    inline void Setup(SlotSpace *slotSpace);

    inline void Initialize();

    inline uintptr_t Allocate(size_t size);

    inline void Finalize();

private:
    class SlotBumpPointerAllocator {
    public:
        inline explicit SlotBumpPointerAllocator(size_t slotSize);
        ~SlotBumpPointerAllocator() = default;

        inline void Reset(uintptr_t top, uintptr_t end);

        inline void Finalize();

        inline uintptr_t Allocate();

        inline bool IsEmpty() const;

        inline size_t GetSlotSize() const;
    private:
        size_t slotSize_ {0};
        uintptr_t top_ {0};
        uintptr_t end_ {0};
    };

    inline void InitializeAllocators();

    SlotSpace *slotSpace_ {nullptr};
    std::array<SlotBumpPointerAllocator *, SlotSpaceConfig::NUM_SLOTS> tlabAllocators_ {};
    std::vector<SlotBumpPointerAllocator *> tlabAllocatorInstances_ {};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_GC_ALLOCATOR_H
