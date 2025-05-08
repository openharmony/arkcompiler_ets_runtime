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
#ifndef ARK_COMMON_WCOLLECTOR_H
#define ARK_COMMON_WCOLLECTOR_H
#include <unordered_map>

#include "common_components/common_runtime/src/heap/allocator/region_space.h"
#include "common_components/common_runtime/src/heap/collector/trace_collector.h"
namespace panda {

class CopyTable {
public:
    explicit CopyTable(RegionSpace& space) : theSpace(space) {}

    // if object is not relocated (forwarded or compacted), return nullptr.
    BaseObject* RouteObject(BaseObject* old, size_t size)
    {
        BaseObject* toAddress = theSpace.RouteObject(old, size);
        return toAddress;
    }

    BaseObject* GetForwardingPointer(BaseObject* old)
    {
        return old->GetForwardingPointer();
    }

    RegionSpace& theSpace;
};

class WCollector : public TraceCollector {
public:
    explicit WCollector(Allocator& allocator, CollectorResources& resources)
        : TraceCollector(allocator, resources), fwdTable_(reinterpret_cast<RegionSpace&>(allocator))
    {
        collectorType_ = CollectorType::SMOOTH_COLLECTOR;
        shouldSTW_ = InitShouldSTWFlag();
    }

    ~WCollector() override = default;

    void Init() override { HeapBitmapManager::GetHeapBitmapManager().InitializeHeapBitmap(); }

    void MarkNewObject(BaseObject* obj) override;

    bool ShouldIgnoreRequest(GCRequest& request) override;
    bool MarkObject(BaseObject* obj) const override;
    bool ResurrectObject(BaseObject* obj) override;

    void EnumRefFieldRoot(RefField<>& ref, RootSet& rootSet) const override;
    void TraceRefField(BaseObject* obj, RefField<>& ref, WorkStack& workStack, WeakStack& weakStack) const;
    void TraceObjectRefFields(BaseObject* obj, WorkStack& workStack, WeakStack& weakStack) override;

    void FixObjectRefFields(BaseObject* obj) const override;
    void FixRefField(BaseObject* obj, RefField<>& field) const;

    BaseObject* ForwardObject(BaseObject* fromVersion) override;

    bool IsOldPointer(RefField<>& ref) const override { return false; }

    bool IsCurrentPointer(RefField<>& ref) const override { return false; }

    uint8_t InitShouldSTWFlag()
    {
        auto useStwGc = std::getenv("arkUseStwGc");
        if (useStwGc == nullptr) {
            return 2; // default to 2
        }
        if (strlen(useStwGc) != 1) {
            LOG_COMMON(ERROR) << "unsupported value of arkUseStwGc, should be 0 or 1.";
            return 2; // default to 2
        }

        switch (useStwGc[0]) {
            case '0':
                return 0;
            case '1':
                return 1; // 1: cm gc
            case '2':
                return 2; // 2: stw-gc
            default:
                LOG_COMMON(ERROR) << "unsupported value of arkUseStwGc, should be 0 or 1.";
        }
        return 2; // default to 2
    }

    void AddRawPointerObject(BaseObject* obj) override
    {
        RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
        space.AddRawPointerObject(obj);
    }

    void RemoveRawPointerObject(BaseObject* obj) override
    {
        RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
        space.RemoveRawPointerObject(obj);
    }

    BaseObject* ForwardUpdateRawRef(ObjectRef& ref);

    bool IsFromObject(BaseObject* obj) const override
    {
        // filter const string object.
        if (Heap::IsHeapAddress(obj)) {
            auto regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<uintptr_t>(obj));
            return regionInfo->IsFromRegion();
        }

        return false;
    }

    bool IsUnmovableFromObject(BaseObject* obj) const override;

    // this is called when caller assures from-object/from-region still exists.
    BaseObject* GetForwardingPointer(BaseObject* fromObj) { return fwdTable_.GetForwardingPointer(fromObj); }

    BaseObject* FindToVersion(BaseObject* obj) const override
    {
        return const_cast<WCollector*>(this)->fwdTable_.GetForwardingPointer(obj);
    }

protected:
    BaseObject* CopyObjectImpl(BaseObject* obj);
    BaseObject* CopyObjectExclusive(BaseObject* obj) override;

    bool TryUntagRefField(BaseObject* obj, RefField<>& field, BaseObject*& target) const override;

    BaseObject* TryForwardObject(BaseObject* fromVersion);

    bool TryUpdateRefField(BaseObject* obj, RefField<>& field, BaseObject*& newRef) const override;
    bool TryForwardRefField(BaseObject* obj, RefField<>& field, BaseObject*& newRef) const override;

    RefField<> GetAndTryTagRefField(BaseObject* target) const override
    {
        if (IsFromObject(target)) {
            return RefField<>(target);
        } else {
            return RefField<>(target);
        }
    }

    void CollectLargeGarbage()
    {
        ARK_COMMON_PHASE_TIMER("Collect large garbage");
        RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
        GCStats& stats = GetGCStats();
        stats.largeSpaceSize = space.LargeObjectSize();
        stats.largeGarbageSize = space.CollectLargeGarbage();
        stats.collectedBytes += stats.largeGarbageSize;
    }

    void CollectPinnedGarbage()
    {
        RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
        GCStats& stats = GetGCStats();
        stats.pinnedSpaceSize = space.PinnedSpaceSize();
        stats.pinnedGarbageSize = space.CollectPinnedGarbage();
        stats.collectedBytes += stats.pinnedGarbageSize;
    }

    void CollectSmallSpace();
    void ClearAllGCInfo();

    void DoGarbageCollection() override;
    void ProcessWeakReferences() override;
    void ProcessFinalizers() override;
    void EnumAndTagRawRoot(ObjectRef& ref, RootSet& rootSet) const override;

private:
    template<bool copy>
    bool TryUpdateRefFieldImpl(BaseObject* obj, RefField<>& ref, BaseObject*& oldRef, BaseObject*& newRef) const;

    void EnumRoots(WorkStack& workStack);

    void TraceHeap(WorkStack& workStack);
    void PostTrace();

    void Preforward();
    void PreforwardStaticRoots();
    void PreforwardConcurrencyModelRoots();
    void PreforwardFinalizerProcessorRoots();

    void FixHeap(); // roots and ref-fields

    CopyTable fwdTable_;

    // gc index 0 or 1 is used to distinguish previous gc and current gc.
    uint8_t shouldSTW_ = true;
};
} // namespace panda
#endif // ~ARK_COMMON_WCOLLECTOR_H
