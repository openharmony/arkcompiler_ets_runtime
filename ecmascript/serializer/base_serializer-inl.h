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

template<SerializeType serializeType>
void BaseSerializer::SerializeObjectField(TaggedObject *object, JSType objectType)
{
    auto visitor = [this, objectType](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
        switch (area) {
            case VisitObjectArea::RAW_DATA:
                WriteMultiRawData(start.SlotAddress(), end.SlotAddress() - start.SlotAddress());
                break;
            case VisitObjectArea::NATIVE_POINTER:
                if (serializeType == SerializeType::VALUE_SERIALIZE) {
                    WriteMultiRawData(start.SlotAddress(), end.SlotAddress() - start.SlotAddress());
                }
                break;
            case VisitObjectArea::IN_OBJECT: {
                SerializeInObjField(root, start, end);
                break;
            }
            default:
                if (serializeType != SerializeType::VALUE_SERIALIZE
                    || !SerializeSpecialObjIndividually(objectType, root, start, end)) {
                    for (ObjectSlot slot = start; slot < end; slot++) {
                        SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                    }
                }
                break;
        }
    };

    objXRay_.VisitObjectBody<VisitType::ALL_VISIT>(object, object->GetClass(), visitor);
}

template<SerializeType serializeType>
void BaseSerializer::SerializeTaggedObject(TaggedObject *object)
{
    JSHClass *hclass = object->GetClass();
    size_t objectSize = hclass->SizeFromJSHClass(object);
    JSType objectType = hclass->GetObjectType();
    SerializedObjectSpace space = GetSerializedObjectSpace(object);
    data_->WriteUint8(SerializeData::EncodeNewObject(space));
    data_->WriteUint32(objectSize);
    data_->CalculateSerializedObjectSize(space, objectSize);
    referenceMap_.emplace(object, objectIndex_++);

    SerializeObjectField<serializeType>(object, objectType);
}
}

#endif  // ECMASCRIPT_SERIALIZER_BASE_SERIALIZER_INL_H