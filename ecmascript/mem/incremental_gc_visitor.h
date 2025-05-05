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

#ifndef ECMASCRIPT_MEM_INCREMENTAL_GC_VISITOR_H
#define ECMASCRIPT_MEM_INCREMENTAL_GC_VISITOR_H

#include <ecmascript/mem/old_gc_visitor.h>

namespace panda::ecmascript {
class IncrementalOldGCMarkObjectVisitor final : public BaseObjectVisitor<IncrementalOldGCMarkObjectVisitor> {
public:
    inline explicit IncrementalOldGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder, uint32_t &visitAddrNum);
    ~IncrementalOldGCMarkObjectVisitor() override = default;

    inline void VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr,
                                     VisitObjectArea area) override;

    inline void VisitObjectHClassImpl(BaseObject *hclass) override;
private:
    OldGCMarkObjectVisitor visitor_;
    uint32_t &visitAddrNum_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_INCREMENTAL_GC_VISITOR_H
