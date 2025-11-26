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

#ifndef ECMASCRIPT_MEM_LOCAL_CMC_CC_EVACUATOR_H
#define ECMASCRIPT_MEM_LOCAL_CMC_CC_EVACUATOR_H

#include "ecmascript/mem/heap.h"

namespace panda {
namespace ecmascript {
class CCTlabAllocator;
class CCEvacuator {
public:
    explicit CCEvacuator(Heap *heap) : heap_(heap) {}
    CCEvacuator(Heap *heap, CCTlabAllocator *tlab) : heap_(heap), tlab_(tlab) {}
    ~CCEvacuator() = default;
    NO_COPY_SEMANTIC(CCEvacuator);
    NO_MOVE_SEMANTIC(CCEvacuator);

    void Initialize(CCTlabAllocator *tlab)
    {
        tlab_ = tlab;
    }

    TaggedObject* Copy(TaggedObject *fromObj, const MarkWord &markWord);
private:
    Heap *heap_ {nullptr};
    CCTlabAllocator *tlab_ {nullptr};
};

class CCUpdateRootVisitor final : public RootVisitor {
public:
    inline explicit CCUpdateRootVisitor(CCEvacuator *evacuator) : evacuator_(evacuator) {}
    ~CCUpdateRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
    // WeakUpdator
    inline TaggedObject* operator()(TaggedObject *header);
private:
    inline void HandleSlot(ObjectSlot slot);

    CCEvacuator *evacuator_ {nullptr};
};

class CCUpdateVisitor final : public BaseObjectVisitor<CCUpdateVisitor> {
public:
    explicit CCUpdateVisitor(JSThread *thread, bool needBarrier) : thread_(thread), needBarrier_(needBarrier) {}
    ~CCUpdateVisitor() = default;

    inline void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                     VisitObjectArea area) override;
private:
    inline void HandleSlot(ObjectSlot slot, Region *rootRegion);

    JSThread *thread_ {nullptr};
    bool needBarrier_ {false};
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_LOCAL_CMC_CC_EVACUATOR_H
