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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_H

#include "ecmascript/mem/cms_mem/cms_region_chain_manager.h"
#include "ecmascript/mem/cms_mem/slot_allocator.h"
#include "ecmascript/mem/cms_mem/slot_space_config.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/space-inl.h"

namespace panda::ecmascript {

class SlotSpace final : public Space, public SweepableSpace {
public:
    SlotSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~SlotSpace() override = default;

    NO_COPY_SEMANTIC(SlotSpace);
    NO_MOVE_SEMANTIC(SlotSpace);

    static inline size_t GetSlotIdxBySize(size_t size);

    static inline size_t GetSlotSizeByIdx(size_t idx);

    inline uint32_t GetRegionCount() const override;

    inline void ReclaimRegions(size_t cachedSize = 0) override;

    template <bool allowGC>
    ARK_INLINE uintptr_t Allocate(size_t size);

    inline std::pair<uintptr_t, size_t> AllocateBufferSyncByIdx(size_t idx);

    template <typename Callback>
    inline void EnumerateRegions(Callback &&cb) const;

    template <typename Callback>
    inline void EnumerateRegionsWithRecord(Callback &&cb) const;

    inline void SetRecordRegion();

    void IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor);

    void PrepareSweeping() override;

    void Sweep() override;

    void AsyncSweep(bool isMain) override;

    bool TryFillSweptRegion() override;

    bool FinishFillSweptRegion() override;

    void PrepareCompact();

    void SetSurvivalObjectSize(size_t survivalObjectSize)
    {
        survivalObjectSize_ = survivalObjectSize;
    }

    size_t GetSurvivalObjectSize() const
    {
        return survivalObjectSize_;
    }

    size_t GetHeapObjectSize() const
    {
        return GetSurvivalObjectSize() + GetAllocateAfterLastGC();
    }

    size_t GetAllocateAfterLastGC() const
    {
        return allocateAfterLastGC_;
    }

    void ReclaimFromRegions();

    void AdjustCapacity(JSThread *thread);

private:
    static constexpr int GROWING_FACTOR = 2;

    static constexpr std::array<size_t, SlotSpaceConfig::NUM_SLOTS> SLOT_SIZES = []() {
        std::array<size_t, SlotSpaceConfig::NUM_SLOTS> res{};
        size_t slotSize = 0;
        for (size_t i = 0; i < SlotSpaceConfig::NUM_SLOTS; ++i, slotSize += SlotSpaceConfig::SLOT_STEP_SIZE) {
            res[i] = slotSize;
        }
        ASSERT(slotSize > SlotSpaceConfig::MAX_REGULAR_HEAP_OBJECT_SLOT_SIZE);
        return res;
    }();

    enum class MemoryCheckerKind {
        SOFT,
        HARD,
    };

    void InitializeAllocators();

    void TryTriggerGCIfNeed();

    void RequestGC();

    template <bool allowGC>
    ARK_INLINE uintptr_t TryAllocateFromAllocator(SlotAllocator *allocator);

    template <bool allowGC>
    ARK_NOINLINE uintptr_t TryExpandAndAllocate(SlotAllocator *allocator);

    bool TryExpandAllocator(SlotAllocator *allocator, MemoryCheckerKind checkerKind);

    void ExpandAllocator(SlotAllocator *allocator);
    
    inline ARK_INLINE void InvokeAllocationInspector(Address object, size_t size, size_t alignedSize);

    Region *AllocateRegion(size_t slotSize);

    std::array<SlotAllocator, SlotSpaceConfig::NUM_SLOTS> allocators_ {};
    std::array<CMSRegionChainManager, SlotSpaceConfig::NUM_SLOTS> regionChainManagers_ {};
    std::vector<Region *> pendingReclaimFromRegions_ {};

    Heap *localHeap_ {nullptr};

    Mutex lock_ {};

    size_t minimumCapacity_ {0};

    size_t survivalObjectSize_ {0};

    size_t allocateAfterLastGC_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_H
