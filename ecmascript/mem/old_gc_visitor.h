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

#ifndef ECMASCRIPT_MEM_OLD_GC_VISITOR_H
#define ECMASCRIPT_MEM_OLD_GC_VISITOR_H

#include "ecmascript/mem/slots.h"
#include <ecmascript/mem/visitor.h>
#include <ecmascript/mem/work_manager.h>

namespace panda::ecmascript {
class OldGCMarkRootVisitor final : public RootVisitor {
public:
    inline explicit OldGCMarkRootVisitor(WorkNodeHolder *workNodeHolder);
    ~OldGCMarkRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
private:
    inline void HandleSlot(ObjectSlot slot);

    WorkNodeHolder *workNodeHolder_ {nullptr};
};

class OldGCMarkObjectVisitor final : public BaseObjectVisitor<OldGCMarkObjectVisitor> {
public:
    inline explicit OldGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder);
    ~OldGCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t start, uintptr_t end,
                                     VisitObjectArea area) override;

    inline void VisitObjectHClassImpl(BaseObject *hclass) override;

    inline void VisitJSWeakMapImpl(BaseObject *rootObject) override;
private:
    inline void HandleSlot(ObjectSlot slot, Region *rootRegion, bool rootNeedEvacuate);

    inline void HandleObject(TaggedObject *object, Region *objectRegion);

    inline void MarkAndPush(TaggedObject *object, Region *objectRegion);

    inline void RecordWeakReference(JSTaggedType *weak);

    WorkNodeHolder *workNodeHolder_ {nullptr};

    friend class NonMovableMarker;
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_OLD_GC_VISITOR_H
