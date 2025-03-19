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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_VISITOR_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_VISITOR_H

#include "ecmascript/mem/slots.h"
#include <ecmascript/mem/visitor.h>
#include <ecmascript/mem/work_manager.h>

namespace panda::ecmascript {
class SharedGCMarkRootVisitor final : public RootVisitor {
public:
    inline explicit SharedGCMarkRootVisitor(SharedGCWorkManager *sWorkManager, uint32_t threadId);
    ~SharedGCMarkRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
private:
    inline void MarkObject(TaggedObject *object);

    SharedGCWorkManager *sWorkManager_ {nullptr};
    uint32_t threadId_ {-1};
};

class SharedGCMarkObjectVisitor final : public EcmaObjectRangeVisitor<SharedGCMarkObjectVisitor> {
public:
    inline explicit SharedGCMarkObjectVisitor(SharedGCWorkManager *sWorkManager, uint32_t threadId);
    ~SharedGCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                     VisitObjectArea area) override;

    inline void VisitObjectHClassImpl(TaggedObject *hclass) override;
private:
    inline void HandleSlot(ObjectSlot slot, Region *rootRegion);

    inline void MarkAndPush(TaggedObject *object, Region *objectRegion);

    inline void RecordWeakReference(JSTaggedType *weak);

    SharedGCWorkManager *sWorkManager_ {nullptr};
    uint32_t threadId_ {-1};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_VISITOR_H
