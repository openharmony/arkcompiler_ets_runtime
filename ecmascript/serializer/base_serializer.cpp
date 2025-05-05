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

#include "ecmascript/serializer/base_serializer-inl.h"

namespace panda::ecmascript {

SerializedObjectSpace BaseSerializer::GetSerializedObjectSpace(TaggedObject *object) const
{
    auto region = Region::ObjectAddressToRange(object);
    auto flag = region->GetRegionSpaceFlag();
    switch (flag) {
        case RegionSpaceFlag::IN_OLD_SPACE:
        case RegionSpaceFlag::IN_YOUNG_SPACE:
        case RegionSpaceFlag::IN_APPSPAWN_SPACE:
            return SerializedObjectSpace::OLD_SPACE;
        case RegionSpaceFlag::IN_NON_MOVABLE_SPACE:
        case RegionSpaceFlag::IN_READ_ONLY_SPACE:
            return SerializedObjectSpace::NON_MOVABLE_SPACE;
        case RegionSpaceFlag::IN_MACHINE_CODE_SPACE:
            return SerializedObjectSpace::MACHINE_CODE_SPACE;
        case RegionSpaceFlag::IN_HUGE_OBJECT_SPACE:
            return SerializedObjectSpace::HUGE_SPACE;
        case RegionSpaceFlag::IN_SHARED_APPSPAWN_SPACE:
        case RegionSpaceFlag::IN_SHARED_OLD_SPACE:
            return SerializedObjectSpace::SHARED_OLD_SPACE;
        case RegionSpaceFlag::IN_SHARED_NON_MOVABLE:
            return SerializedObjectSpace::SHARED_NON_MOVABLE_SPACE;
        case RegionSpaceFlag::IN_SHARED_HUGE_OBJECT_SPACE:
            return SerializedObjectSpace::SHARED_HUGE_SPACE;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void BaseSerializer::WriteMultiRawData(uintptr_t beginAddr, size_t fieldSize)
{
    if (fieldSize > 0) {
        data_->WriteEncodeFlag(EncodeFlag::MULTI_RAW_DATA);
        data_->WriteUint32(fieldSize);
        data_->WriteRawData(reinterpret_cast<uint8_t *>(beginAddr), fieldSize);
    }
}

// Write JSTaggedValue could be either a pointer to a HeapObject or a value
void BaseSerializer::SerializeJSTaggedValue(JSTaggedValue value)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (!value.IsHeapObject()) {
        data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
        data_->WriteJSTaggedValue(value);
    } else {
        TaggedObject *object = nullptr;
        bool isWeak = value.IsWeak();
        object = isWeak ? value.GetWeakReferent() : value.GetTaggedObject();
        SerializeObjectImpl(object, isWeak);
    }
}

bool BaseSerializer::SerializeReference(TaggedObject *object)
{
    if (referenceMap_.find(object) != referenceMap_.end()) {
        uint32_t objectIndex = referenceMap_.find(object)->second;
        data_->WriteEncodeFlag(EncodeFlag::REFERENCE);
        data_->WriteUint32(objectIndex);
        return true;
    }
    return false;
}

bool BaseSerializer::SerializeRootObject(TaggedObject *object)
{
    size_t index = vm_->GetSnapshotEnv()->FindEnvObjectIndex(ToUintPtr(object));
    if (index != SnapshotEnv::MAX_UINT_32) {
        data_->WriteEncodeFlag(EncodeFlag::ROOT_OBJECT);
        data_->WriteUint32(index);
        return true;
    }

    return false;
}

void BaseSerializer::SerializeSharedObject(TaggedObject *object)
{
    data_->WriteEncodeFlag(EncodeFlag::SHARED_OBJECT);
    data_->WriteUint32(sharedObjChunk_->Size());
    referenceMap_.emplace(object, objectIndex_++);
    sharedObjChunk_->Emplace(static_cast<JSTaggedType>(ToUintPtr(object)));
}

bool BaseSerializer::SerializeSpecialObjIndividually(JSType objectType, TaggedObject *root,
                                                     ObjectSlot start, ObjectSlot end)
{
    switch (objectType) {
        case JSType::HCLASS:
            SerializeHClassFieldIndividually(root, start, end);
            return true;
        case JSType::LEXICAL_ENV:
            SerializeLexicalEnvFieldIndividually(root, start, end);
            return true;
        case JSType::SENDABLE_ENV:
            SerializeSendableEnvFieldIndividually(root, start, end);
            return true;
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            SerializeSFunctionFieldIndividually(root, start, end);
            return true;
        case JSType::JS_ASYNC_FUNCTION:
            SerializeAsyncFunctionFieldIndividually(root, start, end);
            return true;
        default:
            return false;
    }
}

void BaseSerializer::SerializeHClassFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end)
{
    ASSERT(root->GetClass()->IsHClass());
    ObjectSlot slot = start;
    while (slot < end) {
        size_t fieldOffset = slot.SlotAddress() - ToUintPtr(root);
        switch (fieldOffset) {
            case JSHClass::PROTOTYPE_OFFSET: {
                JSHClass *kclass = reinterpret_cast<JSHClass *>(root);
                JSTaggedValue proto = kclass->GetPrototype();
                JSType type = kclass->GetObjectType();
                if ((serializeSharedEvent_ > 0) &&
                    (type == JSType::JS_SHARED_OBJECT || type == JSType::JS_SHARED_FUNCTION)) {
                    SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                } else {
                    SerializeObjectProto(kclass, proto);
                }
                slot++;
                break;
            }
            case JSHClass::TRANSTIONS_OFFSET:
            case JSHClass::PARENT_OFFSET: {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue::Undefined());
                slot++;
                break;
            }
            case JSHClass::PROTO_CHANGE_MARKER_OFFSET:
            case JSHClass::PROTO_CHANGE_DETAILS_OFFSET:
            case JSHClass::ENUM_CACHE_OFFSET: {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue::Null());
                slot++;
                break;
            }
            default: {
                SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                slot++;
                break;
            }
        }
    }
}

void BaseSerializer::SerializeSFunctionFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end)
{
    ASSERT(root->GetClass()->GetObjectType() == JSType::JS_SHARED_FUNCTION ||
        root->GetClass()->GetObjectType() == JSType::JS_SHARED_ASYNC_FUNCTION);
    ObjectSlot slot = start;
    while (slot < end) {
        size_t fieldOffset = slot.SlotAddress() - ToUintPtr(root);
        switch (fieldOffset) {
            case JSFunction::MACHINECODE_OFFSET:
            case JSFunction::BASELINECODE_OFFSET:
            case JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET: {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue::Undefined());
                slot++;
                break;
            }
            case JSFunction::ECMA_MODULE_OFFSET: {
                SerializeSFunctionModule(JSFunction::Cast(root));
                slot++;
                break;
            }
            case JSFunction::WORK_NODE_POINTER_OFFSET: {
                data_->WriteEncodeFlag(EncodeFlag::MULTI_RAW_DATA);
                data_->WriteUint32(sizeof(uintptr_t));
                data_->WriteRawData(reinterpret_cast<uint8_t *>(slot.SlotAddress()), sizeof(uintptr_t));
                break;
            }
            default: {
                SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                slot++;
                break;
            }
        }
    }
}

void BaseSerializer::SerializeSFunctionModule(JSFunction *func)
{
    JSTaggedValue moduleValue = func->GetModule();
    if (moduleValue.IsHeapObject()) {
        if (!Region::ObjectAddressToRange(moduleValue.GetTaggedObject())->InSharedHeap()) {
            LOG_ECMA(ERROR) << "Shared function reference to local module";
        }
        if (!SerializeReference(moduleValue.GetTaggedObject())) {
            // Module of shared function should write pointer directly when serialize
            SerializeSharedObject(moduleValue.GetTaggedObject());
        }
    } else {
        data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
        data_->WriteJSTaggedValue(moduleValue);
    }
}

void BaseSerializer::SerializeLexicalEnvFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end)
{
    ASSERT(root->GetClass()->GetObjectType() == JSType::LEXICAL_ENV);
    ObjectSlot slot = start;
    while (slot < end) {
        size_t fieldOffset = slot.SlotAddress() - ToUintPtr(root);
        switch (fieldOffset) {
            case PARENT_ENV_SLOT:
            case SCOPE_INFO_SLOT: {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue::Hole());
                slot++;
                break;
            }
            default: {
                SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                slot++;
                break;
            }
        }
    }
}

void BaseSerializer::SerializeSendableEnvFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end)
{
    ASSERT(root->GetClass()->GetObjectType() == JSType::SENDABLE_ENV);
    ObjectSlot slot = start;
    while (slot < end) {
        size_t fieldOffset = slot.SlotAddress() - ToUintPtr(root);
        switch (fieldOffset) {
            case PARENT_ENV_SLOT:
            case SCOPE_INFO_SLOT: {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue::Hole());
                slot++;
                break;
            }
            default: {
                SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                slot++;
                break;
            }
        }
    }
}

void BaseSerializer::SerializeAsyncFunctionFieldIndividually(TaggedObject *root, ObjectSlot start, ObjectSlot end)
{
    ASSERT(root->GetClass()->GetObjectType() == JSType::JS_ASYNC_FUNCTION);
    ObjectSlot slot = start;
    while (slot < end) {
        size_t fieldOffset = slot.SlotAddress() - ToUintPtr(root);
        switch (fieldOffset) {
            // hash filed
            case sizeof(TaggedObject): {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue(0)); // 0: reset hash filed
                slot++;
                break;
            }
            case JSFunction::PROTO_OR_DYNCLASS_OFFSET:
            case JSFunction::LEXICAL_ENV_OFFSET:
            case JSFunction::MACHINECODE_OFFSET:
            case JSFunction::BASELINECODE_OFFSET:
            case JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET:
            case JSFunction::HOME_OBJECT_OFFSET:
            case JSFunction::ECMA_MODULE_OFFSET: {
                data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
                data_->WriteJSTaggedValue(JSTaggedValue::Undefined());
                slot++;
                break;
            }
            case JSFunction::WORK_NODE_POINTER_OFFSET: {
                data_->WriteEncodeFlag(EncodeFlag::MULTI_RAW_DATA);
                data_->WriteUint32(sizeof(uintptr_t));
                data_->WriteRawData(reinterpret_cast<uint8_t *>(slot.SlotAddress()), sizeof(uintptr_t));
                slot++;
                break;
            }
            default: {
                SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
                slot++;
                break;
            }
        }
    }
}

void BaseSerializer::SerializeObjectProto(JSHClass *kclass, JSTaggedValue proto)
{
    if (!proto.IsHeapObject()) {
        data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
        data_->WriteJSTaggedValue(proto);
    } else if (!SerializeReference(proto.GetTaggedObject()) && !SerializeRootObject(proto.GetTaggedObject())) {
        data_->WriteEncodeFlag(EncodeFlag::OBJECT_PROTO);
        data_->WriteUint8(static_cast<uint8_t>(kclass->GetObjectType()));
    }
}

void BaseSerializer::SerializeTaggedObjField(SerializeType serializeType, TaggedObject *root,
                                             ObjectSlot start, ObjectSlot end)
{
    JSType objectType = root->GetClass()->GetObjectType();
    if (serializeType != SerializeType::VALUE_SERIALIZE
        || !SerializeSpecialObjIndividually(objectType, root, start, end)) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
        }
    }
}

void BaseSerializer::SerializeInObjField(TaggedObject *object, ObjectSlot start, ObjectSlot end)
{
    auto hclass = object->GetClass();
    auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    size_t index = 0;
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto attr = layout->GetAttr(index++);
        if (attr.GetRepresentation() == Representation::DOUBLE || attr.GetRepresentation() == Representation::INT) {
            auto fieldAddr = slot.SlotAddress();
            data_->WriteEncodeFlag(EncodeFlag::PRIMITIVE);
            data_->WriteRawData(reinterpret_cast<uint8_t *>(fieldAddr), sizeof(JSTaggedType));
        } else {
            SerializeJSTaggedValue(JSTaggedValue(slot.GetTaggedType()));
        }
    }
}
}  // namespace panda::ecmascript

