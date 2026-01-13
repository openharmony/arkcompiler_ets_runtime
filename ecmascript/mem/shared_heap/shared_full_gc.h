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
class SharedFullGCRunner;
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
    void MarkRoots(SharedMarkType markType, RootVisitor &rootVisitor);
    void UpdateRecordWeakReference();

    SharedHeap *sHeap_ {nullptr};
    SharedGCWorkManager *sWorkManager_ {nullptr};
    bool isAppspawn_ {false};
};

class SharedFullGCMarkRootVisitor final : public RootVisitor {
public:
    inline explicit SharedFullGCMarkRootVisitor(SharedFullGCRunner *sRunner);
    ~SharedFullGCMarkRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
private:
    SharedFullGCRunner *sRunner_ {nullptr};
};

class SharedFullGCMarkObjectVisitor final : public BaseObjectVisitor<SharedFullGCMarkObjectVisitor> {
public:
    inline explicit SharedFullGCMarkObjectVisitor(SharedFullGCRunner *sRunner);
    ~SharedFullGCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr,
                                     VisitObjectArea area) override;

    inline void VisitHClassSlot(ObjectSlot slot, TaggedObject *hclass);

private:
    SharedFullGCRunner *sRunner_ {nullptr};
};

class SharedFullGCMarkLocalToShareRSetVisitor {
public:
    inline explicit SharedFullGCMarkLocalToShareRSetVisitor(SharedFullGCRunner *sRunner);
    ~SharedFullGCMarkLocalToShareRSetVisitor() = default;

    inline bool operator()(void *mem) const;

private:
    SharedFullGCRunner *sRunner_ {nullptr};
};

class SharedFullGCRunner {
public:
    inline SharedFullGCRunner(SharedHeap *sHeap, SharedGCWorkNodeHolder *sWorkNodeHolder);
    ~SharedFullGCRunner() = default;

    inline SharedFullGCMarkRootVisitor &GetMarkRootVisitor();
    inline SharedFullGCMarkObjectVisitor &GetMarkObjectVisitor();
    inline SharedFullGCMarkLocalToShareRSetVisitor &GetMarkLocalToShareRSetVisitor();

private:
    inline bool NeedEvacuate(Region *region) const;

    inline void MarkValue(ObjectSlot slot);

    inline void MarkObject(ObjectSlot slot, TaggedObject *object);

    inline uintptr_t AllocateForwardAddress(size_t size);

    inline void EvacuateObject(ObjectSlot slot, TaggedObject *object, const MarkWord &markWord);

    inline uintptr_t AllocateDstSpace(size_t size);

    inline void RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size, const MarkWord &markWord);

    inline void UpdateForwardAddressIfSuccess(ObjectSlot slot, TaggedObject *object, JSHClass *klass, size_t size,
                                              TaggedObject *toObject);

    inline void UpdateForwardAddressIfFailed(ObjectSlot slot, size_t size, uintptr_t toAddress, TaggedObject *dst);

    inline void PushObject(TaggedObject *object);

    inline void RecordWeakReference(JSTaggedType *weak);

    SharedHeap *sHeap_ {nullptr};
    SharedGCWorkNodeHolder *sWorkNodeHolder_ {nullptr};
    SharedFullGCMarkRootVisitor markRootVisitor_;
    SharedFullGCMarkObjectVisitor markObjectVisitor_;
    SharedFullGCMarkLocalToShareRSetVisitor markLocalToShareRSetVisitor_;

    friend class SharedFullGCMarkRootVisitor;
    friend class SharedFullGCMarkObjectVisitor;
    friend class SharedFullGCMarkLocalToShareRSetVisitor;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_H
