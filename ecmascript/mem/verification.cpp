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

#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void LogErrorForObjSlot(const BaseHeap *heap, const char *headerInfo, TaggedObject *obj, ObjectSlot slot,
                        TaggedObject *value)
{ // LCOV_EXCL_START
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
                  << ", obj slot oldToNew bit=" << region->TestOldToNew(slot.SlotAddress())
                  << ", obj slot newToEden bit=" << region->TestNewToEden(slot.SlotAddress())
                  << ", obj slot value mark bit=" << slotRegion->Test(slotValue)
                  << ", value mark bit=" << valueRegion->Test(value);
    UNREACHABLE();
}

void LogErrorForObj(const BaseHeap *heap, const char *headerInfo, TaggedObject *obj)
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
} // LCOV_EXCL_STOP

// Only used for verify InactiveSemiSpace
void VerifyObjectVisitor::VerifyInactiveSemiSpaceMarkedObject(const BaseHeap *heap, void *addr)
{
    TaggedObject *object = reinterpret_cast<TaggedObject*>(addr);
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InInactiveSemiSpace()) { // LCOV_EXCL_START
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
    } // LCOV_EXCL_STOP
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
    if (value.IsWeak()) { // LCOV_EXCL_START
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
        VerifyHeapObjectSlotLegal(slot, value, object);
    } // LCOV_EXCL_STOP
}

void VerifyObjectVisitor::VerifyHeapObjectSlotLegal(ObjectSlot slot,
                                                    JSTaggedValue slotValue,
                                                    TaggedObject *object) const
{
    ASSERT(slotValue.IsHeapObject());
    if (ToUintPtr(slotValue.GetTaggedObject()) < INVALID_THRESHOLD) { // LCOV_EXCL_START
        LogErrorForObjSlot(heap_, "Heap verify detected an invalid value.",
            object, slot, slotValue.GetTaggedObject());
    }
    if (!heap_->IsAlive(slotValue.GetTaggedObject())) {
        LogErrorForObjSlot(heap_, "Heap verify detected a dead object.",
            object, slot, slotValue.GetTaggedObject());
        ++(*failCount_);
    } // LCOV_EXCL_STOP
    switch (verifyKind_) {
        case VerifyKind::VERIFY_PRE_GC:
        case VerifyKind::VERIFY_POST_GC:
            break;
        case VerifyKind::VERIFY_MARK_EDEN:
            VerifyMarkEden(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_EVACUATE_EDEN:
            VerifyEvacuateEden(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_MARK_YOUNG:
            VerifyMarkYoung(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_EVACUATE_YOUNG:
            VerifyEvacuateYoung(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_MARK_FULL:
            VerifyMarkFull(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_EVACUATE_OLD:
            VerifyEvacuateOld(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_EVACUATE_FULL:
            VerifyEvacuateFull(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_SHARED_RSET_POST_FULL_GC:
            VerifySharedRSetPostFullGC(object, slot, slotValue.GetTaggedObject());
            break;
        case VerifyKind::VERIFY_PRE_SHARED_GC:
        case VerifyKind::VERIFY_POST_SHARED_GC:
            VerifySharedObjectReference(object, slot, slotValue.GetTaggedObject());
            break;
        default: // LCOV_EXCL_START
            LOG_GC(FATAL) << "unknown verify kind:" << static_cast<size_t>(verifyKind_);
            UNREACHABLE(); // LCOV_EXCL_STOP
    }
}

void VerifyObjectVisitor::VerifyMarkEden(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    JSTaggedValue value1(object);
    JSTaggedValue value2(value);
    if (objectRegion->InGeneralOldSpace() && valueRegion->InEdenSpace()) { // LCOV_EXCL_START
        if (!objectRegion->TestOldToNew(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify MarkEden: Old object, slot miss old_to_new bit.", object, slot, value);
        } else if (!valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkEden: Old object, slot has old_to_new bit, miss gc_mark bit.",
                object, slot, value);
        }
    }
    
    if (objectRegion->InYoungSpace() && valueRegion->InEdenSpace()) {
        if (!objectRegion->TestNewToEden(slot.SlotAddress())) {
            value1.D();
            value2.D();
            LogErrorForObjSlot(heap_, "Verify MarkEden: Young object, slot miss new_to_eden bit.", object, slot, value);
        } else if (!valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkEden: Young object, slot has new_to_eden bit, miss gc_mark bit.",
                object, slot, value);
        }
    }

    if (objectRegion->Test(object)) {
        if (!objectRegion->InEdenSpace() && !objectRegion->InAppSpawnSpace()
                && !objectRegion->InReadOnlySpace()) {
            LogErrorForObj(heap_, "Verify MarkYoung: Marked object, NOT in Eden Space", object);
        }
        if (valueRegion->InEdenSpace() && !valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkEden: Marked object, slot in EdenSpace, miss gc_mark bit.",
                object, slot, value);
        }
        if (valueRegion->Test(value) && !(valueRegion->InEdenSpace() || valueRegion->InAppSpawnSpace()
                || valueRegion->InReadOnlySpace() || valueRegion->InSharedHeap())) {
            LogErrorForObjSlot(heap_, "Verify MarkEden: Marked object, slot marked, but NOT in Eden Space.",
                object, slot, value);
        }
    } // LCOV_EXCL_STOP
}


void VerifyObjectVisitor::VerifyMarkYoung(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    ASSERT(!objectRegion->InSharedHeap());
    if (objectRegion->InGeneralOldSpace() && valueRegion->InGeneralNewSpace()) { // LCOV_EXCL_START
        if (!objectRegion->TestOldToNew(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Old object, slot miss old_to_new bit.", object, slot, value);
        } else if (!valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Old object, slot has old_to_new bit, miss gc_mark bit.",
                object, slot, value);
        }
    }
    if (!objectRegion->InSharedHeap() && valueRegion->InSharedSweepableSpace()) {
        if (!objectRegion->TestLocalToShare(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Local object, slot local_to_share bit = 0, "
                "but SharedHeap object.", object, slot, value);
        }
    }
    if (objectRegion->Test(object)) {
        if (!objectRegion->InGeneralNewSpace() && !objectRegion->InAppSpawnSpace() &&
            !objectRegion->InReadOnlySpace()) {
            LogErrorForObj(heap_, "Verify MarkYoung: Marked object, NOT in Young/AppSpawn/ReadOnly Space", object);
        }
        if (valueRegion->InGeneralNewSpace() && !valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Marked object, slot in YoungSpace, miss gc_mark bit.",
                object, slot, value);
        }
        if (valueRegion->Test(value) &&
            !(valueRegion->InYoungSpace() || valueRegion->InAppSpawnSpace() || valueRegion->InReadOnlySpace() ||
              valueRegion->InSharedHeap() || valueRegion->InEdenSpace())) {
            LogErrorForObjSlot(heap_, "Verify MarkYoung: Marked object, slot marked, but NOT in "
                "Young/AppSpawn/ReadOnly Space.", object, slot, value);
        }
    } // LCOV_EXCL_STOP
}

void VerifyObjectVisitor::VerifyEvacuateEden(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (objectRegion->InGeneralOldSpace()) { // LCOV_EXCL_START
        if (objectRegion->TestOldToNew(slot.SlotAddress())) {
            if (!valueRegion->InActiveSemiSpace()) {
                if (valueRegion->InEdenSpace()) {
                    LogErrorForObjSlot(heap_, "Verify EvacuateEden: Old object, slot old_to_new bit = 1, "
                        "but in EdenSpace object.", object, slot, value);
                }
                LogErrorForObjSlot(heap_, "Verify EvacuateEden: Old object, slot old_to_new bit = 1, "
                    "but NOT ActiveSpace(ToSpace) object.", object, slot, value);
            }
        } else {
            if (valueRegion->InGeneralNewSpace()) {
                LogErrorForObjSlot(heap_, "Verify EvacuateEden: Old object, slot old_to_new bit = 0, "
                    "but YoungSpace object.", object, slot, value);
            }
        }
    }
    if (objectRegion->InYoungSpace()) {
        if (objectRegion->TestNewToEden(slot.SlotAddress())) {
            if (!valueRegion->InEdenSpace()) {
                LogErrorForObjSlot(heap_, "Verify EvacuateEden: Young object, slot young_to_eden bit = 1, "
                    "but NOT Eden object.", object, slot, value);
            } else {
                LogErrorForObjSlot(heap_, "Verify EvacuateEden: Young object, slot young_to_eden bit = 1, "
                    "but Eden object.", object, slot, value);
            }
        } else {
            if (valueRegion->InEdenSpace()) {
                LogErrorForObjSlot(heap_, "Verify EvacuateEden: Young object, slot young_to_eden bit = 0, "
                    "but Eden object.", object, slot, value);
            }
        }
    } // LCOV_EXCL_STOP
}

void VerifyObjectVisitor::VerifyEvacuateYoung(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);

    if (objectRegion->InGeneralOldSpace()) { // LCOV_EXCL_START
        if (objectRegion->TestOldToNew(slot.SlotAddress())) {
            if (!valueRegion->InActiveSemiSpace()) {
                if (valueRegion->InEdenSpace()) {
                    LogErrorForObjSlot(heap_, "Verify EvacuateYoung: Old object, slot old_to_new bit = 1, "
                        "but in EdenSpace object.", object, slot, value);
                }
                LogErrorForObjSlot(heap_, "Verify EvacuateYoung: Old object, slot old_to_new bit = 1, "
                    "but NOT ActiveSpace(ToSpace) object.", object, slot, value);
            }
        } else {
            if (valueRegion->InGeneralNewSpace()) {
                LogErrorForObjSlot(heap_, "Verify EvacuateYoung: Old object, slot old_to_new bit = 0, "
                    "but YoungSpace object.", object, slot, value);
            }
        }
    }
    if (!objectRegion->InSharedHeap() && valueRegion->InSharedSweepableSpace()) {
        if (!objectRegion->TestLocalToShare(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify EvacuateYoung: Local object, slot local_to_share bit = 0, "
                "but SharedHeap object.", object, slot, value);
        }
    }
    if (objectRegion->InActiveSemiSpace()) {
        if (valueRegion->InInactiveSemiSpace()) {
            LogErrorForObjSlot(heap_, "Verify EvacuateYoung: ActiveSpace object, slot in InactiveSpace(FromSpace).",
                object, slot, value);
        } else if (valueRegion->InEdenSpace()) {
            LogErrorForObjSlot(heap_, "Verify EvacuateYoung: ActiveSpace object, slot in EdenSpace.",
                object, slot, value);
        }
    } // LCOV_EXCL_STOP
}

void VerifyObjectVisitor::VerifyMarkFull(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (objectRegion->InGeneralOldSpace() && valueRegion->InGeneralNewSpace()) { // LCOV_EXCL_START
        if (!objectRegion->TestOldToNew(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify MarkFull: Old object, slot miss old_to_new bit.", object, slot, value);
        }
    }
    if (objectRegion->Test(object)) {
        if (!valueRegion->InSharedHeap() && !valueRegion->Test(value)) {
            LogErrorForObjSlot(heap_, "Verify MarkFull: Marked object, slot miss gc_mark bit.", object, slot, value);
        }
    }
    if (!objectRegion->InSharedHeap() && valueRegion->InSharedSweepableSpace()) {
        if (!objectRegion->TestLocalToShare(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify VerifyMarkFull: Local object, slot local_to_share bit = 0, "
                "but SharedHeap object.", object, slot, value);
        }
    } // LCOV_EXCL_STOP
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

void VerifyObjectVisitor::VerifySharedObjectReference(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (objectRegion->InSharedHeap()) { // LCOV_EXCL_START
        if (!valueRegion->InSharedHeap()) {
            LogErrorForObjSlot(heap_, "Verify SharedObjectReference: Shared object references a local object",
                object, slot, value);
        }
    } else if (valueRegion->InSharedSweepableSpace()) {
        if (!objectRegion->TestLocalToShare(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify SharedObjectReference: Local object, slot local_to_share bit = 0, "
                "but SharedHeap object.", object, slot, value);
        }
    } // LCOV_EXCL_STOP
}

void VerifyObjectVisitor::VerifySharedRSetPostFullGC(TaggedObject *object, ObjectSlot slot, TaggedObject *value) const
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (!objectRegion->InSharedHeap() && valueRegion->InSharedSweepableSpace()) { // LCOV_EXCL_START
        if (!objectRegion->TestLocalToShare(slot.SlotAddress())) {
            LogErrorForObjSlot(heap_, "Verify SharedRSetPostFullGC: Local object, slot local_to_share bit = 0, "
                "but SharedHeap object.", object, slot, value);
        }
    } // LCOV_EXCL_STOP
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
    if (region->InGeneralOldSpace()) { // LCOV_EXCL_START
        LOG_GC(ERROR) << "Heap object(" << slot.GetTaggedType() << ") old to new rset fail: value("
                      << slot.GetTaggedObject() << "/"
                      << JSHClass::DumpJSType(slot.GetTaggedObject()->GetClass()->GetObjectType())
                      << ")" << " in " << region->GetSpaceTypeName();
        ++(*failCount_);
    } // LCOV_EXCL_STOP
}

void Verification::VerifyAll() const
{
    [[maybe_unused]] VerifyScope verifyScope(heap_);
    heap_->GetSweeper()->EnsureAllTaskFinished();
    size_t result = VerifyRoot();
    result += VerifyHeap();
    if (result > 0) { // LCOV_EXCL_START
        LOG_GC(FATAL) << "Verify (type=" << static_cast<uint8_t>(verifyKind_)
                      << ") corrupted and " << result << " corruptions";
    } // LCOV_EXCL_STOP
}

size_t Verification::VerifyRoot() const
{
    size_t failCount = 0;
    RootVisitor visitor = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        // Skip verifying shared object in local gc verification.
        if (value.IsInSharedHeap()) {
            return;
        }
        VerifyObjectSlot(slot, &failCount);
    };
    RootRangeVisitor rangeVisitor = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            JSTaggedValue value(slot.GetTaggedType());
            // Skip verifying shared object in local gc verification.
            if (value.IsInSharedHeap()) {
                return;
            }
            VerifyObjectSlot(slot, &failCount);
        }
    };
    RootBaseAndDerivedVisitor derivedVisitor =
        []([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base, [[maybe_unused]] ObjectSlot derived,
           [[maybe_unused]] uintptr_t baseOldObject) {
    };
    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), visitor, rangeVisitor, derivedVisitor, VMRootVisitType::VERIFY);
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

void Verification::VerifyMark(Heap *heap)
{
    LOG_ECMA(DEBUG) << "start verify mark";
    switch (heap->GetMarkType()) {
        case MarkType::MARK_EDEN:
            Verification(heap, VerifyKind::VERIFY_MARK_EDEN).VerifyAll();
            break;
        case MarkType::MARK_YOUNG:
            Verification(heap, VerifyKind::VERIFY_MARK_YOUNG).VerifyAll();
            break;
        case MarkType::MARK_FULL:
            Verification(heap, VerifyKind::VERIFY_MARK_FULL).VerifyAll();
            break;
    }
}

void Verification::VerifyEvacuate(Heap *heap)
{
    LOG_ECMA(DEBUG) << "start verify evacuate and sweep";
    switch (heap->GetMarkType()) {
        case MarkType::MARK_EDEN:
            Verification(heap, VerifyKind::VERIFY_EVACUATE_EDEN).VerifyAll();
            break;
        case MarkType::MARK_YOUNG:
            Verification(heap, VerifyKind::VERIFY_EVACUATE_YOUNG).VerifyAll();
            break;
        case MarkType::MARK_FULL:
            Verification(heap, VerifyKind::VERIFY_EVACUATE_OLD).VerifyAll();
            break;
    }
}

void SharedHeapVerification::VerifyAll() const
{
    [[maybe_unused]] VerifyScope verifyScope(sHeap_);
    sHeap_->GetSweeper()->EnsureAllTaskFinished();
    size_t result = VerifyRoot();
    result += VerifyHeap();
    if (result > 0) { // LCOV_EXCL_START
        LOG_GC(FATAL) << "Verify (type=" << static_cast<uint8_t>(verifyKind_)
                      << ") corrupted and " << result << " corruptions";
    } // LCOV_EXCL_STOP
}

void SharedHeapVerification::VerifyMark(bool cm) const
{
    LOG_GC(DEBUG) << "start verify shared mark";
    [[maybe_unused]] VerifyScope verifyScope(sHeap_);
    sHeap_->GetSweeper()->EnsureAllTaskFinished();
    Runtime::GetInstance()->GCIterateThreadList([cm](JSThread *thread) {
        auto vm = thread->GetEcmaVM();
        auto heap = vm->GetHeap();
        heap->GetSweeper()->EnsureAllTaskFinished();
        const_cast<Heap*>(heap)->FillBumpPointerForTlab();
        auto localBuffer = const_cast<Heap*>(heap)->GetMarkingObjectLocalBuffer();
        if (localBuffer != nullptr) { // LCOV_EXCL_START
            LOG_GC(FATAL) << "verify shared node not null " << cm << ':' << thread;
            UNREACHABLE();
        } // LCOV_EXCL_STOP
        heap->IterateOverObjects([cm](TaggedObject *obj) {
            auto jsHclass = obj->GetClass();
            auto f = [cm] (ObjectSlot slot, TaggedObject *obj) {
                Region *objectRegion = Region::ObjectAddressToRange(obj);
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsWeak() || !value.IsHeapObject()) {
                    return;
                }
                Region *valueRegion = Region::ObjectAddressToRange(value.GetTaggedObject());
                if (!valueRegion->InSharedSweepableSpace()) {
                    return;
                }
                if (!objectRegion->TestLocalToShare(slot.SlotAddress())) { // LCOV_EXCL_START
                    LOG_GC(FATAL) << "verify shared1 " << cm << ':' << slot.SlotAddress()
                                  << ' ' << value.GetTaggedObject();
                    UNREACHABLE();
                }
                if (!valueRegion->Test(value.GetTaggedObject())) {
                    LOG_GC(FATAL) << "verify shared2 " << cm << ':' << slot.SlotAddress()
                                  << ' ' << value.GetTaggedObject();
                    UNREACHABLE();
                }
                if (value.GetTaggedObject()->GetClass()->IsFreeObject()) {
                    LOG_GC(FATAL) << "verify shared3 " << cm << ':' << slot.SlotAddress()
                                  << ' ' << value.GetTaggedObject();
                    UNREACHABLE();
                } // LCOV_EXCL_STOP
            };
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(
                obj, jsHclass, [f](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                VisitObjectArea area) {
                if (area == VisitObjectArea::IN_OBJECT) {
                    auto hclass = root->GetClass();
                    ASSERT(!hclass->IsAllTaggedProp());
                    int index = 0;
                    for (ObjectSlot slot = start; slot < end; slot++) {
                        auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
                        auto attr = layout->GetAttr(index++);
                        if (attr.IsTaggedRep()) {
                            f(slot, root);
                        }
                    }
                    return;
                }
                for (ObjectSlot slot = start; slot < end; slot++) {
                    f(slot, root);
                }
            });
        });
    });
    sHeap_->IterateOverObjects([cm](TaggedObject *obj) {
        auto jsHclass = obj->GetClass();
        auto f = [cm] (ObjectSlot slot, TaggedObject *obj) {
            Region *objectRegion = Region::ObjectAddressToRange(obj);
            if (!objectRegion->Test(obj)) {
                return;
            }
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsWeak() || !value.IsHeapObject()) {
                return;
            }
            [[maybe_unused]] Region *valueRegion = Region::ObjectAddressToRange(value.GetTaggedObject());
            if (!valueRegion->InSharedHeap()) { // LCOV_EXCL_START
                LOG_GC(FATAL) << "verify shared4 " << cm << ':' << slot.SlotAddress()
                              << ' ' << value.GetTaggedObject();
                UNREACHABLE();
            }
            if (!valueRegion->InSharedReadOnlySpace()
                    && !valueRegion->Test(value.GetTaggedObject())) {
                LOG_GC(FATAL) << "verify shared5 " << cm << ':' << slot.SlotAddress()
                              << ' ' << value.GetTaggedObject();
                UNREACHABLE();
            }
            if (value.GetTaggedObject()->GetClass()->IsFreeObject()) {
                LOG_GC(FATAL) << "verify shared6 " << cm << ':' << slot.SlotAddress()
                              << ' ' << value.GetTaggedObject();
                UNREACHABLE();
            } // LCOV_EXCL_STOP
        };
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(
            obj, jsHclass, [f](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                               VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                auto hclass = root->GetClass();
                ASSERT(!hclass->IsAllTaggedProp());
                int index = 0;
                for (ObjectSlot slot = start; slot < end; slot++) {
                    auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
                    auto attr = layout->GetAttr(index++);
                    if (attr.IsTaggedRep()) {
                        f(slot, root);
                    }
                }
                return;
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                f(slot, root);
            }
        });
    });
}

void SharedHeapVerification::VerifySweep(bool cm) const
{
    LOG_GC(DEBUG) << "start verify shared sweep";
    [[maybe_unused]] VerifyScope verifyScope(sHeap_);
    sHeap_->GetSweeper()->EnsureAllTaskFinished();
    Runtime::GetInstance()->GCIterateThreadList([cm](JSThread *thread) {
        auto vm = thread->GetEcmaVM();
        auto heap = vm->GetHeap();
        heap->GetSweeper()->EnsureAllTaskFinished();
        const_cast<Heap*>(heap)->FillBumpPointerForTlab();
        heap->IterateOverObjects([cm](TaggedObject *obj) {
            auto jsHclass = obj->GetClass();
            auto f = [cm] (ObjectSlot slot, TaggedObject *obj) {
                Region *objectRegion = Region::ObjectAddressToRange(obj);
                JSTaggedValue value(slot.GetTaggedType());
                if (value.IsWeak() || !value.IsHeapObject()) {
                    return;
                }
                Region *valueRegion = Region::ObjectAddressToRange(value.GetTaggedObject());
                if (!valueRegion->InSharedSweepableSpace()) {
                    return;
                }
                if (!objectRegion->TestLocalToShare(slot.SlotAddress())) { // LCOV_EXCL_START
                    LOG_GC(FATAL) << "verify shared7 " << cm << ':' << slot.SlotAddress()
                                  << ' ' << value.GetTaggedObject();
                    UNREACHABLE();
                }
                if (!valueRegion->Test(value.GetTaggedObject())) {
                    LOG_GC(FATAL) << "verify shared8 " << cm << ':' << slot.SlotAddress()
                                  << ' ' << value.GetTaggedObject();
                    UNREACHABLE();
                }
                if (value.GetTaggedObject()->GetClass()->IsFreeObject()) {
                    LOG_GC(FATAL) << "verify shared9 " << cm << ':' << slot.SlotAddress()
                                  << ' ' << value.GetTaggedObject();
                    UNREACHABLE();
                } // LCOV_EXCL_STOP
            };
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(
                obj, jsHclass, [f](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                VisitObjectArea area) {
                if (area == VisitObjectArea::IN_OBJECT) {
                    auto hclass = root->GetClass();
                    ASSERT(!hclass->IsAllTaggedProp());
                    int index = 0;
                    for (ObjectSlot slot = start; slot < end; slot++) {
                        auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
                        auto attr = layout->GetAttr(index++);
                        if (attr.IsTaggedRep()) {
                            f(slot, root);
                        }
                    }
                    return;
                }
                for (ObjectSlot slot = start; slot < end; slot++) {
                    f(slot, root);
                }
            });
        });
    });
    sHeap_->IterateOverObjects([cm](TaggedObject *obj) {
        auto jsHclass = obj->GetClass();
        auto f = [cm] (ObjectSlot slot, TaggedObject *obj) {
            [[maybe_unused]] Region *objectRegion = Region::ObjectAddressToRange(obj);
            if (!objectRegion->Test(obj)) { // LCOV_EXCL_START
                LOG_GC(FATAL) << "verify shared10 " << cm << ':' << obj;
                UNREACHABLE();
            }
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsWeak() || !value.IsHeapObject()) {
                return;
            }
            [[maybe_unused]] Region *valueRegion = Region::ObjectAddressToRange(value.GetTaggedObject());
            if (!valueRegion->InSharedHeap()) {
                LOG_GC(FATAL) << "verify shared11 " << cm << ':' << slot.SlotAddress()
                              << ' ' << value.GetTaggedObject();
                UNREACHABLE();
            }
            if (!valueRegion->InSharedReadOnlySpace()
                    && !valueRegion->Test(value.GetTaggedObject())) {
                LOG_GC(FATAL) << "verify shared12 " << cm << ':' << slot.SlotAddress()
                              << ' ' << value.GetTaggedObject();
                UNREACHABLE();
            }
            if (value.GetTaggedObject()->GetClass()->IsFreeObject()) {
                LOG_GC(FATAL) << "verify shared13 " << cm << ':' << slot.SlotAddress()
                              << ' ' << value.GetTaggedObject();
                UNREACHABLE();
            } // LCOV_EXCL_STOP
        };
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(
            obj, jsHclass, [f](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                auto hclass = root->GetClass();
                ASSERT(!hclass->IsAllTaggedProp());
                int index = 0;
                for (ObjectSlot slot = start; slot < end; slot++) {
                    auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
                    auto attr = layout->GetAttr(index++);
                    if (attr.IsTaggedRep()) {
                        f(slot, root);
                    }
                }
                return;
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                f(slot, root);
            }
        });
    });
}

size_t SharedHeapVerification::VerifyRoot() const
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
    RootVisitor serializeVisitor = [this, &failCount]([[maybe_unused]] Root type, ObjectSlot slot) {
        JSTaggedValue value(slot.GetTaggedType());
        if (!sHeap_->IsAlive(value.GetTaggedObject())) {
            LOG_ECMA(ERROR) << "Serialize Heap verify detected a dead object. " << value.GetTaggedObject();
            ++failCount;
        }
    };
    Runtime::GetInstance()->IterateSerializeRoot(serializeVisitor);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        auto localHeap = const_cast<Heap*>(vm->GetHeap());
        localHeap->GetSweeper()->EnsureAllTaskFinished();
        ObjectXRay::VisitVMRoots(vm, visitor, rangeVisitor, derivedVisitor, VMRootVisitType::VERIFY);
        if (failCount > 0) {
            LOG_GC(ERROR) << "SharedHeap VerifyRoot detects deadObject count is " << failCount;
        }
    });
    return failCount;
}

size_t SharedHeapVerification::VerifyHeap() const
{
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->FillBumpPointerForTlab();
    });
    size_t failCount = sHeap_->VerifyHeapObjects(verifyKind_);
    if (failCount > 0) {
        LOG_GC(ERROR) << "SharedHeap VerifyHeap detects deadObject count is " << failCount;
    }
    VerifyKind localVerifyKind = VerifyKind::VERIFY_END;
    if (verifyKind_ == VerifyKind::VERIFY_PRE_SHARED_GC) {
        localVerifyKind = VerifyKind::VERIFY_PRE_GC;
    } else if (verifyKind_ == VerifyKind::VERIFY_POST_SHARED_GC) {
        localVerifyKind = VerifyKind::VERIFY_POST_GC;
    }

    Runtime::GetInstance()->GCIterateThreadList([&, localVerifyKind](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        auto localHeap = const_cast<Heap*>(vm->GetHeap());
        localHeap->GetSweeper()->EnsureAllTaskFinished();
        if (localVerifyKind != VerifyKind::VERIFY_END) {
            Verification(localHeap, localVerifyKind).VerifyAll();
        }
        if (failCount > 0) {
            LOG_GC(ERROR) << "SharedHeap VerifyRoot detects deadObject in local heap count is " << failCount;
        }
    });
    return failCount;
}

void SharedHeapVerification::VerifyObjectSlot(const ObjectSlot &slot, size_t *failCount) const
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsWeak()) {
        VerifyObjectVisitor(sHeap_, failCount, verifyKind_)(value.GetTaggedWeakRef());
    } else if (value.IsHeapObject()) {
        VerifyObjectVisitor(sHeap_, failCount, verifyKind_)(value.GetTaggedObject());
    }
}
}  // namespace panda::ecmascript
