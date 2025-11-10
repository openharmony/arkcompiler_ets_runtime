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

#ifndef ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_INL_H
#define ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_INL_H

#include "ecmascript/cross_vm/unified_gc/unified_gc_marker.h"

#include "ecmascript/cross_vm/daemon_task_hybrid-inl.h"
#include "ecmascript/cross_vm/js_tagged_value_hybrid-inl.h"
#include "ecmascript/cross_vm/work_manager_hybrid-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {

UnifiedGCMarkRootVisitor::UnifiedGCMarkRootVisitor(WorkNodeHolder *workNodeHolder, UnifiedGCMarker *marker)
    : workNodeHolder_(workNodeHolder), marker_(marker) {}

void UnifiedGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    HandleSlot(slot);
}

void UnifiedGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void UnifiedGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                       [[maybe_unused]] ObjectSlot derived,
                                                       [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of OldGC does not need to update slot
}

void UnifiedGCMarkRootVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }
    TaggedObject *object = value.GetTaggedObject();
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (objectRegion->InSharedHeap()) {
        return;
    }

#ifdef PANDA_JS_ETS_HYBRID_MODE
    marker_->HandleJSXRefObject(object);
#endif // PANDA_JS_ETS_HYBRID_MODE

    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

UnifiedGCMarkObjectVisitor::UnifiedGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder, UnifiedGCMarker *marker)
    : workNodeHolder_(workNodeHolder), marker_(marker) {}

void UnifiedGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr, uintptr_t endAddr,
                                                      VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = TaggedObject::Cast(root)->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        JSThread *thread = workNodeHolder_->GetJSThread();
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout(thread).GetTaggedObject());
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            PropertyAttributes attr = layout->GetAttr(thread, index++);
            if (attr.IsTaggedRep()) {
                HandleSlot(slot);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void UnifiedGCMarkObjectVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject() || value.IsWeakForHeapObject()) {
        return;
    }
    TaggedObject *object = value.GetTaggedObject();
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (objectRegion->InSharedHeap()) {
        return;
    }
#ifdef PANDA_JS_ETS_HYBRID_MODE
    marker_->HandleJSXRefObject(object);
#endif // PANDA_JS_ETS_HYBRID_MODE
    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

void UnifiedGCMarkObjectVisitor::VisitObjectHClassImpl(BaseObject *hclass)
{
    ASSERT(TaggedObject::Cast(hclass)->GetClass()->IsHClass());
    Region *hclassRegion = Region::ObjectAddressToRange(hclass);
    if (!hclassRegion->InSharedHeap()) {
        ASSERT(hclassRegion->InNonMovableSpace() || hclassRegion->InReadOnlySpace());
        if (hclassRegion->AtomicMark(hclass)) {
            workNodeHolder_->Push(TaggedObject::Cast(hclass));
        }
    }
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
inline void UnifiedGCMarker::HandleJSXRefObject(TaggedObject *object)
{
    JSTaggedValue value(object);
    if (value.IsJSXRefObject()) {
        auto stsVMInterface = heap_->GetEcmaVM()->GetCrossVMOperator()->GetSTSVMInterface();
        stsVMInterface->MarkFromObject(JSObject::Cast(object)->GetNativePointerField(heap_->GetJSThread(), 0));
    }
}
#endif // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_INL_H
