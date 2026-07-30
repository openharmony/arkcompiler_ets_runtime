/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_INL_H
#define ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_INL_H

#include "ecmascript/serializer/base_serializer.h"

namespace panda::ecmascript {

BaseSerializer::SerializeObjectFieldVisitor::SerializeObjectFieldVisitor(BaseSerializer *serializer)
    : serializer_(serializer) {}

void BaseSerializer::SerializeObjectFieldVisitor::VisitObjectRangeImpl(BaseObject *rootObject,
    ObjectSlot start, ObjectSlot end, VisitObjectArea area)
{
    JSThread *thread = serializer_->GetThread();
    switch (area) {
        case VisitObjectArea::RAW_DATA:
            serializer_->WriteMultiRawData(start.SlotAddress(), end.SlotAddress() - start.SlotAddress());
            break;
        case VisitObjectArea::NATIVE_POINTER:
            serializer_->WriteMultiRawData(start.SlotAddress(), end.SlotAddress() - start.SlotAddress());
            break;
        case VisitObjectArea::IN_OBJECT: {
            serializer_->SerializeInObjField(TaggedObject::Cast(rootObject), start, end);
            break;
        }
        default: {
            for (ObjectSlot slot = start; slot < end; slot++) {
                [[maybe_unused]] JSTaggedValue value =
                    JSTaggedValue(Barriers::GetTaggedValue(thread, slot.SlotAddress()));
            }
            serializer_->SerializeTaggedObjField(TaggedObject::Cast(rootObject), start, end);
            break;
        }
    }
}

void BaseSerializer::SerializeObjectFieldVisitor::VisitCompressedObjectRangeImpl(BaseObject *rootObject,
    CompressedObjectSlot start, CompressedObjectSlot end)
{
    ASSERT(JSTaggedValue(TaggedObject::Cast(rootObject)).IsConstantPool());
    serializer_->SerializeConstantPoolFieldIndividually(TaggedObject::Cast(rootObject), start, end);
}

void BaseSerializer::SerializeObjectFieldVisitor::VisitObjectHClassImpl([[maybe_unused]] BaseObject *rootObject,
                                                                        BaseObject *hclass)
{
    serializer_->SerializeJSTaggedValue(JSTaggedValue(TaggedObject::Cast(hclass)));
}

void BaseSerializer::SerializeObjectField(TaggedObject *object)
{
    SerializeObjectFieldVisitor serializeObjectFieldVisitor(this);
    ObjectXRay::VisitObjectBody<VisitType::ALL_VISIT>(object, object->GetClass(), serializeObjectFieldVisitor);
}

void BaseSerializer::SerializeTaggedObject(TaggedObject *object)
{
    size_t objectSize = object->GetSize();
    SerializedObjectSpace space = GetSerializedObjectSpace(object);
    data_->WriteUint8(SerializeData::EncodeNewObject(space));
    data_->WriteUint32(objectSize);
    data_->CalculateSerializedObjectSize(space, objectSize);
    referenceMap_.emplace(object, objectIndex_++);

    SerializeObjectField(object);
}
}

#endif  // ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_INL_H
