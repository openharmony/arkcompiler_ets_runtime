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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_INL_H

#include "ecmascript/mem/shared_heap/shared_full_gc.h"

namespace panda::ecmascript {

SharedFullGCMarkRootVisitor::SharedFullGCMarkRootVisitor(SharedGCMovableMarker *marker, uint32_t threadId)
    : marker_(marker), threadId_(threadId) {}

void SharedFullGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        ASSERT(!value.IsWeak());
        marker_->MarkObject(threadId_, value.GetTaggedObject(), slot);
    }
}

void SharedFullGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsInSharedSweepableSpace()) {
            ASSERT(!value.IsWeak());
            marker_->MarkObject(threadId_, value.GetTaggedObject(), slot);
        }
    }
}

void SharedFullGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base,
                                                          ObjectSlot derived, uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

void SharedFullGCMarkRootVisitor::MarkObject([[maybe_unused]] TaggedObject *object)
{
    UNREACHABLE();
}

SharedFullGCMarkObjectVisitor::SharedFullGCMarkObjectVisitor(SharedGCMovableMarker *marker, uint32_t threadId)
    : marker_(marker), threadId_(threadId) {}

void SharedFullGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr,
                                                         VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        auto root = TaggedObject::Cast(rootObject);
        JSHClass *hclass = root->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout(THREAD_ARG_PLACEHOLDER).GetTaggedObject());
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            PropertyAttributes attr = layout->GetAttr(THREAD_ARG_PLACEHOLDER, index++);
            if (attr.IsTaggedRep()) {
                marker_->MarkValue(threadId_, slot);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        marker_->MarkValue(threadId_, slot);
    }
}

void SharedFullGCMarkObjectVisitor::VisitObjectHClassImpl([[maybe_unused]] BaseObject *hclass)
{
    UNREACHABLE();
}

void SharedFullGCMarkObjectVisitor::HandleSlot([[maybe_unused]] ObjectSlot slot, [[maybe_unused]] Region *rootRegion,
                                               [[maybe_unused]] bool rootNeedEvacuate)
{
    UNREACHABLE();
}

void SharedFullGCMarkObjectVisitor::MarkAndPush([[maybe_unused]] TaggedObject *object,
                                                [[maybe_unused]] Region *objectRegion)
{
    UNREACHABLE();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_INL_H
