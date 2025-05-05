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

#ifndef ECMASCRIPT_MEM_FULL_GC_H
#define ECMASCRIPT_MEM_FULL_GC_H

#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/work_manager.h"

namespace panda {
namespace ecmascript {
class FullGCRunner;

class FullGC final : public GarbageCollector {
public:
    explicit FullGC(Heap *heap);
    ~FullGC() override = default;

    NO_COPY_SEMANTIC(FullGC);
    NO_MOVE_SEMANTIC(FullGC);

    void RunPhases() override;
    void RunPhasesForAppSpawn();
    void SetForAppSpawn(bool flag);
protected:
    void Initialize() override;
    void Mark() override;
    void Sweep() override;
    void Finish() override;

private:
    void MarkRoots();
    void ProcessSharedGCRSetWorkList();
    bool HasEvacuated(Region *region);

    Heap *heap_;
    size_t youngAndOldAliveSize_ = 0;
    size_t nonMoveSpaceFreeSize_ = 0;
    size_t youngSpaceCommitSize_ = 0;
    size_t oldSpaceCommitSize_ = 0;
    size_t nonMoveSpaceCommitSize_ = 0;
    bool forAppSpawn_ {false};
    // obtain from heap
    WorkManager *workManager_ {nullptr};

    friend class WorkManager;
    friend class Heap;
};

class FullGCMarkRootVisitor final : public RootVisitor {
public:
    inline explicit FullGCMarkRootVisitor(FullGCRunner *runner);
    ~FullGCMarkRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
private:
    FullGCRunner *runner_ {nullptr};
};

class FullGCMarkObjectVisitor final : public BaseObjectVisitor<FullGCMarkObjectVisitor> {
public:
    inline explicit FullGCMarkObjectVisitor(FullGCRunner *runner);
    ~FullGCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                     VisitObjectArea area) override;

    inline void VisitHClassSlot(ObjectSlot slot, TaggedObject *hclass);
private:
    FullGCRunner *runner_ {nullptr};
};

class FullGCUpdateLocalToShareRSetVisitor final : public BaseObjectVisitor<FullGCUpdateLocalToShareRSetVisitor> {
public:
    inline explicit FullGCUpdateLocalToShareRSetVisitor(FullGCRunner *runner);
    ~FullGCUpdateLocalToShareRSetVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                     VisitObjectArea area) override;
private:
    inline void SetLocalToShareRSet(ObjectSlot slot, Region *rootRegion);

    FullGCRunner *runner_ {nullptr};
};

class FullGCRunner {
public:
    inline explicit FullGCRunner(Heap *heap, WorkNodeHolder *workNodeHolder, bool isAppSpawn);
    ~FullGCRunner() = default;

    inline FullGCMarkRootVisitor &GetMarkRootVisitor();
    inline FullGCMarkObjectVisitor &GetMarkObjectVisitor();

protected:
    inline void HandleMarkingSlot(ObjectSlot slot);

    inline void HandleMarkingSlotObject(ObjectSlot slot, TaggedObject *object);

    template <class Callback>
    void VisitBodyInObj(BaseObject *root, uintptr_t start, uintptr_t end, Callback &&cb);

private:
    inline bool NeedEvacuate(Region *region);

    inline void EvacuateObject(ObjectSlot slot, TaggedObject *object, const MarkWord &markWord);

    inline uintptr_t AllocateForwardAddress(size_t size);

    inline uintptr_t AllocateDstSpace(size_t size);

    inline uintptr_t AllocateAppSpawnSpace(size_t size);

    inline void RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size, const MarkWord &markWord);

    inline void UpdateForwardAddressIfSuccess(ObjectSlot slot, TaggedObject *object, JSHClass *klass, size_t size,
        TaggedObject *toAddress);

    inline void UpdateForwardAddressIfFailed(ObjectSlot slot, size_t size, uintptr_t toAddress, TaggedObject *dst);

    inline void PushObject(TaggedObject *object);

    inline void RecordWeakReference(JSTaggedType *weak);

    Heap *heap_ {nullptr};
    WorkNodeHolder *workNodeHolder_ {nullptr};
    bool isAppSpawn_ {false};
    FullGCMarkRootVisitor markRootVisitor_;
    FullGCMarkObjectVisitor markObjectVisitor_;
    FullGCUpdateLocalToShareRSetVisitor updateLocalToShareRSetVisitor_;

    friend class FullGCMarkRootVisitor;
    friend class FullGCMarkObjectVisitor;
    friend class FullGCUpdateLocalToShareRSetVisitor;
    friend class CompressGCMarker;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_FULL_GC_H
