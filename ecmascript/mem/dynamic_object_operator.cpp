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

#include "ecmascript/mem/dynamic_object_operator.h"
#include "ecmascript/mem/object_xray.h"

namespace panda::ecmascript {

DynamicObjectOperator DynamicObjectOperator::dynOperator_;

void DynamicObjectOperator::Initialize()
{
    if (g_isEnableCMCGC) {
        BaseObject::RegisterDynamic(&dynOperator_);
    }
}

void RefFieldObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr,
                                                 uintptr_t endAddr, VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        VisitBodyInObj(TaggedObject::Cast(root), start, end, [this](ObjectSlot slot) {
            visit(slot);
        });
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        visit(slot);
    }
}

void RefFieldObjectVisitor::VisitObjectHClassImpl(BaseObject *hclass)
{
    JSTaggedValue clz(reinterpret_cast<TaggedObject *>(hclass));
    visitor_(reinterpret_cast<common::RefField<>&>(clz));
}

void RefFieldObjectVisitor::VisitAllRefFields(TaggedObject *obj)
{
    // Note this will update the stack param, not the slot of object hclass
    // But sinc hclass in non-movable, so current all visitor will not update hlass field, so it's ok
    VisitObjectHClassImpl(obj->GetClass());
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, obj->GetClass(), *this);
}

void RefFieldObjectVisitor::visit(ObjectSlot slot)
{
    visitor_(reinterpret_cast<common::RefField<>&>(*(slot.GetRefFieldAddr())));
}

}  // namespace panda::ecmascript
