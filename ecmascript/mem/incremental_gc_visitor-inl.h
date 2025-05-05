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

#ifndef ECMASCRIPT_MEM_INCREMENTAL_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_INCREMENTAL_GC_VISITOR_INL_H

#include "ecmascript/mem/incremental_gc_visitor.h"

//#iuclude "ecmascript/mem/old_gc_visitor-inl.h"

namespace panda::ecmascript {
IncrementalOldGCMarkObjectVisitor::IncrementalOldGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder,
    uint32_t &visitAddrNum) : visitor_(workNodeHolder), visitAddrNum_(visitAddrNum) {}

void IncrementalOldGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                                             VisitObjectArea area)
{
    visitAddrNum_ += end - start;
    visitor_(root, start, end, area);
}

void IncrementalOldGCMarkObjectVisitor::VisitObjectHClassImpl(BaseObject *hclass)
{
    ASSERT(TaggedObject::Cast(hclass)->GetClass()->IsHClass());
    visitor_.VisitHClass(hclass);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_INCREMENTAL_GC_VISITOR_INL_H
