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

#ifndef ECMASCRIPT_MEM_DYNAMIC_OBJECT_OPERATOR_H
#define ECMASCRIPT_MEM_DYNAMIC_OBJECT_OPERATOR_H

#include "common_interfaces/objects/base_object.h"
#include "common_interfaces/objects/base_object_operator.h"
#include "ecmascript/free_object.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/tagged_object.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class RefFieldObjectVisitor final : public BaseObjectVisitor<RefFieldObjectVisitor> {
   public:
    inline explicit RefFieldObjectVisitor(const RefFieldVisitor &visitor): visitor_(visitor) {};
    ~RefFieldObjectVisitor() override = default;

    void VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                              VisitObjectArea area) override;

    void VisitObjectHClassImpl(BaseObject *hclass) override;

    void VisitAllRefFields(TaggedObject *obj);

   private:
    void visit(ObjectSlot slot);

    template <class Callback>
    void VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback &&cb)
    {
        JSHClass *hclass = root->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());

        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout().GetTaggedObject());
        ObjectSlot realEnd = start;
        start += layout->GetPropertiesCapacity(); // only += operator is supported
        end = std::min(end, realEnd);

        int index = 0;
        for (ObjectSlot slot = start; slot < end; slot++) {
            PropertyAttributes attr = layout->GetAttr(index++);
            if (attr.IsTaggedRep()) {
                cb(slot);
            }
        }
    }

    const RefFieldVisitor &visitor_;
};

static constexpr uint64_t TAG_MARK_BIT = 0x02ULL;
static uint64_t GetHeader(const BaseObject* obj)
{
    return (*((uint64_t*)obj));
}

class DynamicObjectOperator : public BaseObjectOperatorInterfaces {
public:
    static void Initialize();

    bool IsValidObject([[maybe_unused]] const BaseObject *object) const override
    {
        auto taggedObject = TaggedObject::Cast(object);
        auto hclass = taggedObject->GetClass();

        return hclass->GetClass()->IsHClass();
    }

    void ForEachRefField(const BaseObject *object, const RefFieldVisitor &visitor) const override
    {
        auto freeObject = FreeObject::Cast(reinterpret_cast<uintptr_t>(object));
        if (!freeObject->IsFreeObject()) {
            RefFieldObjectVisitor refFieldObjectVisitor(visitor);
            refFieldObjectVisitor.VisitAllRefFields(TaggedObject::Cast(object));
        }
    }

    size_t GetSize(const BaseObject *object) const override
    {
#ifdef USE_CMC_GC
        ASSERT(!object->IsForwarded());
#endif
        auto freeObject = FreeObject::Cast(reinterpret_cast<uintptr_t>(object));
        if (freeObject->IsFreeObject()) {
            return freeObject->Available();
        }
        auto taggedObject = TaggedObject::Cast(object);
        JSHClass *jsHclass = taggedObject->GetClass();
        size_t size = jsHclass->SizeFromJSHClass(taggedObject);
        return AlignUp<size_t>(size, 8); // 8 Byte alignment
    }

    BaseObject *GetForwardingPointer(const BaseObject *object) const override
    {
        return TaggedObject::Cast(object)->GetForwardingPointer();
    }

    void SetForwardingPointerAfterExclusive(BaseObject *object, BaseObject *fwdPtr) override
    {
        TaggedObject::Cast(object)->SetForwardingPointerAfterExclusive(fwdPtr);
    }

private:
    static DynamicObjectOperator dynOperator_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_DYNAMIC_OBJECT_OPERATOR_H
