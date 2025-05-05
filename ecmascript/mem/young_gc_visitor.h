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

#ifndef ECMASCRIPT_MEM_YOUNG_GC_VISITOR_H
#define ECMASCRIPT_MEM_YOUNG_GC_VISITOR_H

#include "ecmascript/mem/slots.h"
#include <ecmascript/mem/visitor.h>
#include <ecmascript/mem/work_manager.h>

namespace panda::ecmascript {
class YoungGCMarkRootVisitor final : public RootVisitor {
public:
    inline explicit YoungGCMarkRootVisitor(WorkNodeHolder *workNodeHolder);
    ~YoungGCMarkRootVisitor() override = default;

    inline void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override;

    inline void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override;

    inline void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                        uintptr_t baseOldObject) override;
private:
    inline void HandleSlot(ObjectSlot slot);

    WorkNodeHolder *workNodeHolder_ {nullptr};
};

class YoungGCMarkObjectVisitor final : public BaseObjectVisitor<YoungGCMarkObjectVisitor> {
public:
    inline explicit YoungGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder);
    ~YoungGCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                     VisitObjectArea area) override;
private:
    inline void HandleSlot(ObjectSlot slot);

    WorkNodeHolder *workNodeHolder_ {nullptr};
};

class YoungGCMarkOldToNewRSetVisitor {
public:
    inline explicit YoungGCMarkOldToNewRSetVisitor(WorkNodeHolder *workNodeHolder);
    ~YoungGCMarkOldToNewRSetVisitor() = default;

    inline void operator()(Region *region) const;
private:
    inline bool HandleSlot(ObjectSlot slot) const;

    WorkNodeHolder *workNodeHolder_ {nullptr};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_YOUNG_GC_VISITOR_H
