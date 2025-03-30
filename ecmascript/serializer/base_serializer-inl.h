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

template <SerializeType serializeType>
BaseSerializer::SerializeObjectFieldVisitor<serializeType>::SerializeObjectFieldVisitor(BaseSerializer *serializer)
    : serializer_(serializer) {}

template <SerializeType serializeType>
void BaseSerializer::SerializeObjectFieldVisitor<serializeType>::VisitObjectRangeImpl(TaggedObject *root,
    ObjectSlot start, ObjectSlot end, VisitObjectArea area)
{
    switch (area) {
        case VisitObjectArea::RAW_DATA:
            serializer_->WriteMultiRawData(start.SlotAddress(), end.SlotAddress() - start.SlotAddress());
            break;
        case VisitObjectArea::NATIVE_POINTER:
            if (serializeType == SerializeType::VALUE_SERIALIZE) {
                serializer_->WriteMultiRawData(start.SlotAddress(), end.SlotAddress() - start.SlotAddress());
            }
            break;
        case VisitObjectArea::IN_OBJECT: {
            serializer_->SerializeInObjField(root, start, end);
            break;
        }
        default:
            serializer_->SerializeTaggedObjField(serializeType, root, start, end);
            break;
    }
}

template<SerializeType serializeType>
void BaseSerializer::SerializeObjectField(TaggedObject *object)
{
    SerializeObjectFieldVisitor<serializeType> serializeObjectFieldVisitor(this);
    ObjectXRay::VisitObjectBody<VisitType::ALL_VISIT>(object, object->GetClass(), serializeObjectFieldVisitor);
}

template<SerializeType serializeType>
void BaseSerializer::SerializeTaggedObject(TaggedObject *object)
{
    JSHClass *hclass = object->GetClass();
    size_t objectSize = hclass->SizeFromJSHClass(object);
    SerializedObjectSpace space = GetSerializedObjectSpace(object);
    data_->WriteUint8(SerializeData::EncodeNewObject(space));
    data_->WriteUint32(objectSize);
    data_->CalculateSerializedObjectSize(space, objectSize);
    referenceMap_.emplace(object, objectIndex_++);

    SerializeObjectField<serializeType>(object);
}
}

#endif  // ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_INL_H