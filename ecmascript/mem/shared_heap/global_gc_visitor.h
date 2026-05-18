/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_VISITOR_H
#define ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_VISITOR_H

#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"

namespace panda::ecmascript {

class GlobalMarkRootVisitor final : public RootVisitor {
public:
    explicit GlobalMarkRootVisitor(GlobalGCWorkNodeHolder *holder) : holder_(holder) {}
    ~GlobalMarkRootVisitor() override = default;

    void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;
    void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;
    void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                 [[maybe_unused]] ObjectSlot derived,
                                 [[maybe_unused]] uintptr_t baseOldObject) override {}

private:
    void MarkLocalObject(ObjectSlot slot);
    GlobalGCWorkNodeHolder *holder_ {nullptr};
};

class GlobalMarkObjectVisitor final : public BaseObjectVisitor<GlobalMarkObjectVisitor> {
public:
    explicit GlobalMarkObjectVisitor(GlobalGCWorkNodeHolder *holder) : holder_(holder) {}
    ~GlobalMarkObjectVisitor() override = default;

    void VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr, uintptr_t endAddr,
                              VisitObjectArea area) override;
    void VisitObjectHClassImpl(BaseObject *hclass) override;
    void HandleObject(TaggedObject *object, Region *objectRegion);

private:
    void MarkLocalRefFromSlot(ObjectSlot slot);
    GlobalGCWorkNodeHolder *holder_ {nullptr};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_VISITOR_H
