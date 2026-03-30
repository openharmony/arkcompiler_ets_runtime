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

#ifndef ECMASCRIPT_MEM_LOCAL_CMC_CC_GC_VISITOR_H
#define ECMASCRIPT_MEM_LOCAL_CMC_CC_GC_VISITOR_H

#include "ecmascript/mem/parallel_marker.h"

namespace panda::ecmascript {
class CCMarkRootVisitor final : public RootVisitor {
public:
    inline explicit CCMarkRootVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder) {}
    ~CCMarkRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override {};
private:
    inline void HandleSlot(ObjectSlot slot);

    WorkNodeHolder *workNodeHolder_ {nullptr};
};

class CCMarkObjectVisitor final : public BaseObjectVisitor<CCMarkObjectVisitor> {
public:
    inline explicit CCMarkObjectVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder) {}
    ~CCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t start, uintptr_t end,
                                     VisitObjectArea area) override;

    inline void VisitObjectHClassImpl(BaseObject *hclass) override;

    inline void VisitWeakLinkedHashMapImpl(BaseObject *rootObject) override;
private:
    inline void HandleSlot(ObjectSlot slot);

    inline void HandleObject(TaggedObject *object);

    inline void RecordWeakReference(JSTaggedType *weak);

    WorkNodeHolder *workNodeHolder_ {nullptr};

    friend class CCMarker;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_LOCAL_CMC_CC_GC_VISITOR_H
