/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/verification.h"

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/concurrent_sweeper.h"

namespace panda::ecmascript {
void LogErrorForObjSlot(const Heap *heap, const char *headerInfo, TaggedObject *obj, ObjectSlot slot,
                        TaggedObject *value)
{
    TaggedObject *slotValue = slot.GetTaggedObject();
    Region *region = Region::ObjectAddressToRange(obj);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    Region *slotRegion = Region::ObjectAddressToRange(slotValue);
    LOG_GC(FATAL) << headerInfo
                  << ": gctype=" << heap->GetGCType()
                  << ", obj address=" << obj
                  << ", obj region=" << region
                  << ", obj space type=" << region->GetSpaceTypeName()
                  << ", obj type=" << JSHClass::DumpJSType(obj->GetClass()->GetObjectType())
                  << ", slot address=" << reinterpret_cast<void*>(slot.SlotAddress())
                  << ", slot value=" << slotValue
                  << ", slot value region=" << slotRegion
                  << ", slot value space type=" << slotRegion->GetSpaceTypeName()
                  << ", slot value type=" << JSHClass::DumpJSType(slotValue->GetClass()->GetObjectType())
                  << ", value address=" << value
                  << ", value region=" << valueRegion
                  << ", value space type=" << valueRegion->GetSpaceTypeName()
                  << ", value type=" << JSHClass::DumpJSType(value->GetClass()->GetObjectType())
                  << ", obj mark bit=" << region->Test(obj)
                  << ", obj slot olwToNew bit=" << region->TestOldToNew(slot.SlotAddress())
                  << ", obj slot value mark bit=" << slotRegion->Test(slotValue)
                  << ", value mark bit=" << valueRegion->Test(value);
    UNREACHABLE();
}

void LogErrorForObj(const Heap *heap, const char *headerInfo, TaggedObject *obj)
{
    Region *region = Region::ObjectAddressToRange(obj);
    LOG_GC(FATAL) << headerInfo
                  << ": gctype=" << heap->GetGCType()
                  << ", obj address=" << obj
                  << ", obj value=" << ObjectSlot(ToUintPtr(obj)).GetTaggedObject()
                  << ", obj region=" << region
                  << ", obj space type=" << region->GetSpaceTypeName()
                  << ", obj type=" << JSHClass::DumpJSType(obj->GetClass()->GetObjectType())
                  << ", obj mark bit=" << region->Test(obj);
    UNREACHABLE();
}

// Only used for verify InactiveSemiSpace
void VerifyObjectVisitor::VerifyInactiveSemiSpaceMarkedObject(const Heap *heap, void *addr)
{
    TaggedObject *object = reinterpret_cast<TaggedObject*>(addr);
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InInactiveSemiSpace()) {
        LogErrorForObj(heap, "Verify InactiveSemiSpaceMarkedObject: Object is not in InactiveSemiSpace.", object);
    } else {
        MarkWord word(object);
        if (!word.IsForwardingAddress()) {
            LogErrorForObj(heap, "Verify InactiveSemiSpaceMarkedObject: not forwarding address.", object);
        } else {
            ObjectSlot slot(ToUintPtr(object));
            TaggedObject *value = word.ToForwardingAddress();
            Region *valueRegion = Region::ObjectAddressToRange(value);
            if (valueRegion->InInactiveSemiSpace()) {
                LogErrorForObjSlot(heap, "Verify InactiveSemiSpaceMarkedObject: forwarding address, "
                    "but InactiveSemiSpace(FromSpace) Object.", object, slot, value);
            }
        }
    }
}

// Verify the object body
void VerifyObjectVisitor::VisitAllObjects(TaggedObject *obj)
{
    auto jsHclass = obj->GetClass();
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(
        obj, jsHclass, [this](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                              VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                auto hclass = root->GetClass();
                ASSERT(!hclass->IsAllTaggedProp());
                int index = 0;
                for (ObjectSlot slot = start; slot < end; slot++) {
                    auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
                    auto attr = layout->GetAttr(index++);
                    if (attr.IsTaggedRep()) {
                        VerifyObjectSlotLegal(slot, root);
                    }
                }
                return;
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                VerifyObjectSlotLegal(slot, root);
            }
        });
}

void VerifyObjectVisitor::VerifyObjectSlotLegal(ObjectSlot slot, TaggedObject *object) const
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsWeak()) {
        if (ToUintPtr(value.GetTaggedWeakRef()) < INVALID_THRESHOLD) {
            LogErrorForObjSlot(heap_, "Heap verify detected an invalid value.",
                object, slot, value.GetTaggedWeakRef());
        }
        if (!heap_->IsAlive(value.GetTaggedWeakRef())) {
            LogErrorForObjSlot(heap_, "Heap verify detected a dead weak object.",
                object, slot, value.GetTaggedWeakRef());
            ++(*failCount_);
        }
    } else if (value.IsHeapObject()) {
        if (ToUintPtr(value.GetTaggedObject()) < INVALID_THRESHOLD) {
            LogErrorForObjSlot(heap_, "Heap verify detected an invalid value.",
                object, slot, value.GetTaggedObject());
        }
        if (!heap_->IsAlive(value.GetTaggedObject())) {
            LogErrorForObjSlot(heap_, "Heap verify detected a dead object.",
                object, slot, value.GetTaggedObject());
            ++(*failCount_);
        }
        switch (verifyKind_) {
            case VerifyKind::VERIFY_PRE_GC:
            case VerifyKind::VERIFY_POST_GC:
                break;
            case VerifyKind::VERIFY_CONCURRENT_MARK_YOUNG:
                VerifyMarkYoung(object, slot, value.GetTaggedObject());
                break;
            case VerifyKind::VERIFY_EVACUATE_YOUNG:
                VerifyEvacuateYoung(object, slot, value.GetTaggedObject());
                break;
            case VerifyKind::VERIFY_CONCURRENT_MARK_FULL:
                VerifyMarkFull(object, slot, value.GetTaggedObject());
                break;
            case VerifyKind::VERIFY_EVACUATE_OLD:
                VerifyEvacuateOld(object, slot, value.GetTaggedObject());
                break;
            case VerifyKind::VERIFY_EVACUATE_FULL:
                VerifyEvacuateFull(object, slot, value.GetTaggedObject());
                break;
            default:
                LOG_GC(FATAL) << "unknown verify kind:" << static_cast<size_t>(verifyKind_);
                UNREACHABLE();
        }
    }
}

void VerifyObjectVisitor::VerifyMarkYoung(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (!objectRegion->InYoungSpace() && valueRegion->InYoungSpace()) {
        if (!objectRegion->TestOldToNew(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Old object, slot miss old_to_new bit.", object, slot, value);
        } else if (!valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Old object, slot has old_to_new bit, miss gc_mark bit.",
                object, slot, value);
        }
    }
    if (objectRegion->Test(object)) {
        if (!objectRegion->InYoungSpace() && !objectRegion->InAppSpawnSpace() && !objectRegion->InReadOnlySpace()) {
            LogErrorForObj(heap_, "Verify MarkYoung: Marked object, NOT in Young/AppSpawn/ReadOnly Space", object);
        }
        if (valueRegion->InYoungSpace() && !valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Marked object, slot in YoungSpace, miss gc_mark bit.",
                object, slot, value);
        }
        if (valueRegion->Test(value) && !(valueRegion->InYoungSpace() || valueRegion->InAppSpawnSpace() ||
                                          valueRegion->InReadOnlySpace())) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Marked object, slot marked, but NOT in "
                "Young/AppSpawn/ReadOnly Space.", object, slot, value);
        }
    }
}

void VerifyObjectVisitor::VerifyEvacuateYoung(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (!objectRegion->InYoungSpace()) {
        if (objectRegion->TestOldToNew(slot.SlotAddress())) {
            if (!valueRegion->InActiveSemiSpace()) {
                LogErrorForObjSlot(heap_, "Verify EvacuateYoung: Old object, slot old_to_new bit = 1, "
                    "but NOT ActiveSpace(ToSpace) object.", object, slot, value);
            }
        } else {
            if (valueRegion->InYoungSpace()) {
                LogErrorForObjSlot(heap_, "Verify EvacuateYoung: Old object, slot old_to_new bit = 0, "
                    "but YoungSpace object.", object, slot, value);
            }
        }
    }
    if (objectRegion->InActiveSemiSpace()) {
        if (valueRegion->InInactiveSemiSpace()) {
            LogErrorForObjSlot(heap_, "Verify EvacuateYoung: ActiveSpace object, slot in InactiveSpace(FromSpace).",
                object, slot, value);
        }
    }
}

void VerifyObjectVisitor::VerifyMarkFull(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (!objectRegion->InYoungSpace() && valueRegion->InYoungSpace()) {
        if (!objectRegion->TestOldToNew(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify MarkFull: Old object, slot miss old_to_new bit.", object, slot, value);
        }
    }
    if (objectRegion->Test(object)) {
        if (!valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkFull: Marked object, slot miss gc_mark bit.", object, slot, value);
        }
    }
}

void VerifyObjectVisitor::VerifyEvacuateOld([[maybe_unused]]TaggedObject *root,
                                            [[maybe_unused]]ObjectSlot slot,
                                            [[maybe_unused]]TaggedObject *value) const
{
    VerifyEvacuateYoung(root, slot, value);
}

void VerifyObjectVisitor::VerifyEvacuateFull([[maybe_unused]]TaggedObject *root,
                                             [[maybe_unused]]ObjectSlot slot,
                                             [[maybe_unused]]TaggedObject *value) const
{
    VerifyEvacuateYoung(root, slot, value);
}

void VerifyObjectVisitor::operator()(TaggedObject *obj, JSTaggedValue value)
{
    ObjectSlot slot(reinterpret_cast<uintptr_t>(obj));
    if (!value.IsHeapObject()) {
        LOG_GC(DEBUG) << "Heap object(" << slot.SlotAddress() << ") old to new rset fail: value is "
                      << slot.GetTaggedType();
        return;
    }

    TaggedObject *object = value.GetRawTaggedObject();
    auto region = Region::ObjectAddressToRange(object);
    if (!region->InYoungSpace()) {
        LOG_GC(ERROR) << "Heap object(" << slot.GetTaggedType() << ") old to new rset fail: value("
                      << slot.GetTaggedObject() << "/"
                      << JSHClass::DumpJSType(slot.GetTaggedObject()->GetClass()->GetObjectType())
                      << ")" << " in " << region->GetSpaceTypeName();
        ++(*failCount_);
    }
}

void Verification::VerifyAll() const
{
    [[maybe_unused]] VerifyScope verifyScope(heap_);
    heap_->GetSweeper()->EnsureAllTaskFinished();
    size_t result = VerifyRoot();
    result += VerifyHeap();
    if (result > 0) {
        LOG_GC(FATAL) << "Verify (type=" << static_cast<uint8_t>(verifyKind_)
                      << ") corrupted and " << result << " corruptions";
    }
}

size_t Verification::VerifyRoot() const
{
    size_t failCount = 0;
    RootVisitor visitor = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot slot) {
        VerifyObjectSlot(slot, &failCount);
    };
    RootRangeVisitor rangeVisitor = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            VerifyObjectSlot(slot, &failCount);
        }
    };
    RootBaseAndDerivedVisitor derivedVisitor =
        []([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base, [[maybe_unused]] ObjectSlot derived,
           [[maybe_unused]] uintptr_t baseOldObject) {
    };
    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), visitor, rangeVisitor, derivedVisitor);
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyRoot detects deadObject count is " << failCount;
    }

    return failCount;
}

size_t Verification::VerifyHeap() const
{
    size_t failCount = heap_->VerifyHeapObjects(verifyKind_);
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyHeap detects deadObject count is " << failCount;
    }
    return failCount;
}

size_t Verification::VerifyOldToNewRSet() const
{
    size_t failCount = heap_->VerifyOldToNewRSet(verifyKind_);
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyOldToNewRSet detects non new space count is " << failCount;
    }
    return failCount;
}

void Verification::VerifyObjectSlot(const ObjectSlot &slot, size_t *failCount) const
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsWeak()) {
        VerifyObjectVisitor(heap_, failCount, verifyKind_)(value.GetTaggedWeakRef());
    } else if (value.IsHeapObject()) {
        VerifyObjectVisitor(heap_, failCount, verifyKind_)(value.GetTaggedObject());
    }
}
}  // namespace panda::ecmascript
