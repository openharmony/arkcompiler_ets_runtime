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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_H

#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {
class SharedGCMovableMarker;
enum class SharedMarkType : uint8_t;

class SharedFullGC : public GarbageCollector {
public:
    explicit SharedFullGC(SharedHeap *heap) : sHeap_(heap), sWorkManager_(heap->GetWorkManager()) {}
    ~SharedFullGC() override = default;
    NO_COPY_SEMANTIC(SharedFullGC);
    NO_MOVE_SEMANTIC(SharedFullGC);

    void RunPhases() override;
    void ResetWorkManager(SharedGCWorkManager *workManager);
    void SetForAppSpawn(bool flag)
    {
        isAppspawn_ = flag;
    }
protected:
    void Initialize() override;
    void Mark() override;
    void Sweep() override;
    void Finish() override;

private:
    void MarkRoots(SharedMarkType markType, VMRootVisitType type);
    void UpdateRecordWeakReference();
    bool HasEvacuated(Region *region);

    SharedHeap *sHeap_ {nullptr};
    SharedGCWorkManager *sWorkManager_ {nullptr};
    bool isAppspawn_ {false};
};

class SharedFullGCMarkRootVisitor final : public RootVisitor {
public:
    explicit SharedFullGCMarkRootVisitor(SharedGCMovableMarker *marker, uint32_t threadId);
    ~SharedFullGCMarkRootVisitor() override = default;

    void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                 uintptr_t baseOldObject) override;
private:
    void MarkObject(TaggedObject *object);

    SharedGCMovableMarker *marker_ {nullptr};
    uint32_t threadId_ {-1};
};

class SharedFullGCMarkObjectVisitor final : public EcmaObjectRangeVisitor<SharedFullGCMarkObjectVisitor> {
public:
    explicit SharedFullGCMarkObjectVisitor(SharedGCMovableMarker *marker, uint32_t threadId);
    ~SharedFullGCMarkObjectVisitor() override = default;

    void VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) override;

    void VisitObjectHClassImpl(TaggedObject *hclass) override;
private:
    void HandleSlot(ObjectSlot slot, Region *rootRegion, bool rootNeedEvacuate);

    void MarkAndPush(TaggedObject *object, Region *objectRegion);

    SharedGCMovableMarker *marker_ {nullptr};
    uint32_t threadId_ {-1};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_H
