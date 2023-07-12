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

namespace panda::ecmascript {
// Verify the object body
void VerifyObjectVisitor::VisitAllObjects(TaggedObject *obj)
{
    auto jsHclass = obj->GetClass();
    objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(
        obj, jsHclass, [this](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                              VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                auto hclass = root->GetClass();
                if (!hclass->IsAllTaggedProp()) {
                    int index = 0;
                    for (ObjectSlot slot = start; slot < end; slot++) {
                        auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
                        auto attr = layout->GetAttr(index++);
                        if (attr.IsTaggedRep()) {
                            VisitObject(slot);
                        }
                    }
                    return;
                }
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                VisitObject(slot);
            }
        });
}

void VerifyObjectVisitor::VisitObject(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsWeak()) {
        if (!heap_->IsAlive(value.GetTaggedWeakRef())) {
            LOG_GC(ERROR) << "Heap verify detected a dead weak object " << value.GetTaggedObject()
                                << " at object:" << slot.SlotAddress();
            ++(*failCount_);
        }
    } else if (value.IsHeapObject()) {
        if (!heap_->IsAlive(value.GetTaggedObject())) {
            LOG_GC(ERROR) << "Heap verify detected a dead object at " << value.GetTaggedObject()
                                << " at object:" << slot.SlotAddress();
            ++(*failCount_);
        }
    }
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
    objXRay_.VisitVMRoots(visitor, rangeVisitor, derivedVisitor);
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyRoot detects deadObject count is " << failCount;
    }

    return failCount;
}

size_t Verification::VerifyHeap() const
{
    size_t failCount = heap_->VerifyHeapObjects();
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyHeap detects deadObject count is " << failCount;
    }
    return failCount;
}

size_t Verification::VerifyOldToNewRSet() const
{
    size_t failCount = heap_->VerifyOldToNewRSet();
    if (failCount > 0) {
        LOG_GC(ERROR) << "VerifyOldToNewRSet detects non new space count is " << failCount;
    }
    return failCount;
}

void Verification::VerifyObjectSlot(const ObjectSlot &slot, size_t *failCount) const
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsWeak()) {
        VerifyObjectVisitor(heap_, failCount)(value.GetTaggedWeakRef());
    } else if (value.IsHeapObject()) {
        VerifyObjectVisitor(heap_, failCount)(value.GetTaggedObject());
    }
}
}  // namespace panda::ecmascript
