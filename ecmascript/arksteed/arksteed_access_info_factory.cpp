/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_access_info_factory.h"

#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::arksteed {

namespace {
constexpr size_t NAMED_STORE_INPUT_COUNT = 3;
constexpr size_t NAMED_ACCESS_SLOT_INPUT = 0;
constexpr size_t NAMED_ACCESS_NAME_INPUT = 1;
constexpr size_t NAMED_STORE_RECEIVER_INPUT = 2;

struct ParsedStoreHandler {
    uint64_t handlerInfo {0};
    ArkSteedProtoCellRef protoCell {};
    bool hasProtoCell {false};
};

struct ParsedLoadHandler {
    uint64_t handlerInfo {0};
    ArkSteedObjectRef holder {};
    ArkSteedProtoCellRef protoCell {};
    bool holderIsReceiver {true};
    bool hasProtoCell {false};
};

bool IsNamedLoadBytecode(kungfu::EcmaOpcode opcode)
{
    return opcode == kungfu::EcmaOpcode::LDOBJBYNAME_IMM8_ID16 ||
        opcode == kungfu::EcmaOpcode::LDOBJBYNAME_IMM16_ID16;
}

bool IsNamedStoreInputShape(const kungfu::BytecodeInfo &bytecodeInfo)
{
    return bytecodeInfo.inputs.size() == NAMED_STORE_INPUT_COUNT &&
        std::holds_alternative<kungfu::ICSlotId>(bytecodeInfo.inputs[NAMED_ACCESS_SLOT_INPUT]) &&
        std::holds_alternative<kungfu::ConstDataId>(bytecodeInfo.inputs[NAMED_ACCESS_NAME_INPUT]) &&
        std::holds_alternative<kungfu::VirtualRegister>(bytecodeInfo.inputs[NAMED_STORE_RECEIVER_INPUT]);
}

void SetFieldLocation(uint64_t handlerInfo, PropertyAccessInfo *info)
{
    info->fieldIndex = HandlerBase::GetOffset(handlerInfo);
    info->fieldStorage = HandlerBase::IsInlinedProps(handlerInfo) ? AccessFieldStorage::IN_OBJECT :
                                                                   AccessFieldStorage::PROPERTIES_ARRAY;
    uint32_t baseOffset = info->fieldStorage == AccessFieldStorage::IN_OBJECT ? 0 : TaggedArray::DATA_OFFSET;
    info->fieldOffset = static_cast<int32_t>(baseOffset + info->fieldIndex * JSTaggedValue::TaggedTypeSize());
    info->fieldRepresentation = AccessFieldRepresentation::TAGGED;
    info->fieldType = AccessFieldType::ANY;
}

bool TryReadPrototypeStoreHandler(const ArkSteedHeapBroker *broker, JSThread *compilerThread,
                                  JSTaggedValue cachedHandler, ParsedStoreHandler *result)
{
    if (cachedHandler.IsWeak() || !cachedHandler.IsPrototypeHandler()) {
        return false;
    }
    auto *prototypeHandler = PrototypeHandler::Cast(cachedHandler.GetTaggedObject());
    JSTaggedValue protoCellValue = prototypeHandler->GetProtoCell(compilerThread);
    if (protoCellValue.IsNull() || !protoCellValue.IsProtoChangeMarker()) {
        return false;
    }
    JSTaggedValue handlerInfoValue = prototypeHandler->GetHandlerInfo(compilerThread);
    if (!handlerInfoValue.IsInt()) {
        return false;
    }
    result->handlerInfo = handlerInfoValue.GetLargeUInt();
    result->protoCell = broker->MakeProtoCellRef(protoCellValue);
    result->hasProtoCell = true;
    return result->protoCell.IsSafeForCompile();
}

bool TryReadStoreHandler(const ArkSteedHeapBroker *broker, JSThread *compilerThread,
                         const ArkSteedHandlerRef &handler, ParsedStoreHandler *result)
{
    JSTaggedValue cachedHandler = handler;
    if (cachedHandler.IsInt()) {
        result->handlerInfo = cachedHandler.GetLargeUInt();
        return true;
    }
    return TryReadPrototypeStoreHandler(broker, compilerThread, cachedHandler, result);
}

bool TryReadPrototypeLoadHandler(const ArkSteedHeapBroker *broker, JSThread *compilerThread,
                                 JSTaggedValue cachedHandler, ParsedLoadHandler *result)
{
    if (cachedHandler.IsWeak() || !cachedHandler.IsPrototypeHandler()) {
        return false;
    }
    auto *prototypeHandler = PrototypeHandler::Cast(cachedHandler.GetTaggedObject());
    JSTaggedValue protoCellValue = prototypeHandler->GetProtoCell(compilerThread);
    if (!protoCellValue.IsProtoChangeMarker()) {
        return false;
    }
    JSTaggedValue handlerInfoValue = prototypeHandler->GetHandlerInfo(compilerThread);
    if (!handlerInfoValue.IsInt()) {
        return false;
    }
    result->handlerInfo = handlerInfoValue.GetLargeUInt();
    result->holderIsReceiver = false;
    JSTaggedValue holderValue = prototypeHandler->GetHolder(compilerThread);
    if (!holderValue.IsUndefined()) {
        result->holder = broker->MakeObjectRef(holderValue);
    }
    result->protoCell = broker->MakeProtoCellRef(protoCellValue);
    result->hasProtoCell = true;
    bool hasValidHolder = holderValue.IsUndefined() ? HandlerBase::IsNonExist(result->handlerInfo) :
                                                      result->holder.IsSafeForCompile();
    return hasValidHolder && result->protoCell.IsSafeForCompile();
}

bool TryReadLoadHandler(const ArkSteedHeapBroker *broker, JSThread *compilerThread,
                        const ArkSteedHandlerRef &handler, ParsedLoadHandler *result)
{
    JSTaggedValue cachedHandler = handler;
    if (cachedHandler.IsInt()) {
        result->handlerInfo = cachedHandler.GetLargeUInt();
        return true;
    }
    return TryReadPrototypeLoadHandler(broker, compilerThread, cachedHandler, result);
}

void FillNamedAccessInfo(const NamedAccessCaseFeedback &caseFeedback, uint64_t handlerInfo,
                         ArkSteedProtoCellRef protoCell, bool hasProtoCell, PropertyAccessInfo *info)
{
    info->expectedHClass = caseFeedback.expectedHClass;
    info->protoCell = protoCell;
    info->handlerInfo = handlerInfo;
    info->hasProtoCell = hasProtoCell;
    info->guards.expectedHClass = caseFeedback.expectedHClass;
    info->guards.protoCell = protoCell;
    info->guards.hasHClassGuard = true;
    info->guards.hasProtoCellGuard = hasProtoCell;
    info->dependencies.hclassDependency = AccessDependencyKind::HCLASS;
    if (hasProtoCell) {
        info->dependencies.protoCellDependency = AccessDependencyKind::PROTOTYPE_CELL;
    }
}
}  // namespace

bool ArkSteedAccessInfoFactory::TryMakeNamedStoreAccessInfo(const NamedAccessCaseFeedback &caseFeedback,
                                                            NamedStoreAccessInfo *info) const
{
    *info = {};
    if (!caseFeedback.expectedHClass.IsSafeForCompile() || !caseFeedback.handler.IsSafeForCompile()) {
        return false;
    }

    ParsedStoreHandler parsed;
    if (!TryReadStoreHandler(broker_, compilerThread_, caseFeedback.handler, &parsed)) {
        return false;
    }

    if (!HandlerBase::IsNonSharedStoreField(parsed.handlerInfo) ||
        HandlerBase::RepresentationBit::Get(parsed.handlerInfo) != Representation::TAGGED) {
        return false;
    }

    info->mode = AccessMode::NAMED_STORE;
    info->kind = parsed.hasProtoCell ? AccessKind::PROTOTYPE_FIELD : AccessKind::FIELD;
    info->holderIsReceiver = true;
    info->guards.holderIsReceiver = true;
    FillNamedAccessInfo(caseFeedback, parsed.handlerInfo, parsed.protoCell, parsed.hasProtoCell, info);
    SetFieldLocation(parsed.handlerInfo, info);
    return true;
}

bool ArkSteedAccessInfoFactory::TryMakeNamedLoadAccessInfo(const NamedAccessCaseFeedback &caseFeedback,
                                                           NamedLoadAccessInfo *info) const
{
    *info = {};
    if (!caseFeedback.expectedHClass.IsSafeForCompile() || !caseFeedback.handler.IsSafeForCompile()) {
        return false;
    }

    ParsedLoadHandler parsed;
    if (!TryReadLoadHandler(broker_, compilerThread_, caseFeedback.handler, &parsed)) {
        return false;
    }

    if (!HandlerBase::IsNonExist(parsed.handlerInfo)) {
        if (!HandlerBase::IsField(parsed.handlerInfo) || HandlerBase::IsAccessor(parsed.handlerInfo) ||
            HandlerBase::RepresentationBit::Get(parsed.handlerInfo) != Representation::TAGGED) {
            return false;
        }
        SetFieldLocation(parsed.handlerInfo, info);
    }

    info->mode = AccessMode::NAMED_LOAD;
    info->kind = HandlerBase::IsNonExist(parsed.handlerInfo) ? AccessKind::NON_EXIST :
        (parsed.hasProtoCell ? AccessKind::PROTOTYPE_FIELD : AccessKind::FIELD);
    info->holder = parsed.holder;
    info->holderIsReceiver = parsed.holderIsReceiver;
    info->hasNotFoundProtoCellGuard = parsed.hasProtoCell && HandlerBase::IsNonExist(parsed.handlerInfo);
    info->guards.holder = parsed.holder;
    info->guards.hasHolder = !parsed.holderIsReceiver && !parsed.holder.IsUndefined();
    info->guards.holderIsReceiver = parsed.holderIsReceiver;
    info->guards.hasNotFoundProtoCellGuard = info->hasNotFoundProtoCellGuard;
    FillNamedAccessInfo(caseFeedback, parsed.handlerInfo, parsed.protoCell, parsed.hasProtoCell, info);
    return true;
}

bool ArkSteedAccessInfoFactory::TryBuildNamedStoreAccessInfo(int slotIndex, NamedStoreAccessSet *access) const
{
    *access = {};
    if (!IsNamedStoreInputShape(bytecodeInfo_)) {
        return false;
    }
    NamedAccessFeedback feedback;
    if (!broker_->GetFeedbackForNamedAccess(feedbackReader_, slotIndex, &feedback)) {
        return false;
    }
    ArkSteedHeapBroker::SerializingScope scope(broker_, "ArkSteedAccessInfoFactory::TryBuildNamedStoreAccessInfo");
    return ComputeNamedStoreAccessInfo(feedback, access);
}

bool ArkSteedAccessInfoFactory::ComputeNamedStoreAccessInfo(const NamedAccessFeedback &feedback,
                                                            NamedStoreAccessSet *access) const
{
    *access = {};
    access->slotId = feedback.base.source.slotId;
    access->isPoly = feedback.base.source.isPoly;
    access->feedback = feedback.base.source;
    access->feedback.slotKind = AccessFeedbackSlotKind::NAMED_STORE;

    for (uint32_t i = 0; i < feedback.caseCount && access->caseCount < MAX_NAMED_IC_POLY_CASES; ++i) {
        NamedStoreAccessInfo info;
        if (!TryMakeNamedStoreAccessInfo(feedback.cases[i], &info)) {
            *access = {};
            return false;
        }
        access->cases[access->caseCount] = info;
        access->cases[access->caseCount].name = feedback.name;
        access->caseCount++;
    }
    bool success = feedback.base.source.isPoly ? access->caseCount >= 2 : access->caseCount == 1;
    if (!success) {
        *access = {};
        return false;
    }
    if (!RegisterDependencies(*access)) {
        *access = {};
        return false;
    }
    return true;
}

bool ArkSteedAccessInfoFactory::TryBuildNamedLoadAccessInfo(int slotIndex, NamedLoadAccessSet *access) const
{
    *access = {};
    if (!IsNamedLoadBytecode(bytecodeInfo_.GetOpcode())) {
        return false;
    }
    NamedAccessFeedback feedback;
    if (!broker_->GetFeedbackForNamedAccess(feedbackReader_, slotIndex, &feedback)) {
        return false;
    }
    ArkSteedHeapBroker::SerializingScope scope(broker_, "ArkSteedAccessInfoFactory::TryBuildNamedLoadAccessInfo");
    return ComputeNamedLoadAccessInfo(feedback, access);
}

bool ArkSteedAccessInfoFactory::ComputeNamedLoadAccessInfo(const NamedAccessFeedback &feedback,
                                                           NamedLoadAccessSet *access) const
{
    *access = {};
    access->slotId = feedback.base.source.slotId;
    access->isPoly = feedback.base.source.isPoly;
    access->feedback = feedback.base.source;
    access->feedback.slotKind = AccessFeedbackSlotKind::NAMED_LOAD;

    for (uint32_t i = 0; i < feedback.caseCount && access->caseCount < MAX_NAMED_IC_POLY_CASES; ++i) {
        NamedLoadAccessInfo info;
        if (!TryMakeNamedLoadAccessInfo(feedback.cases[i], &info)) {
            *access = {};
            return false;
        }
        access->cases[access->caseCount] = info;
        access->cases[access->caseCount].name = feedback.name;
        access->caseCount++;
    }
    bool success = feedback.base.source.isPoly ? access->caseCount >= 2 : access->caseCount == 1;
    if (!success) {
        *access = {};
        return false;
    }
    if (!RegisterDependencies(*access)) {
        *access = {};
        return false;
    }
    return true;
}

bool ArkSteedAccessInfoFactory::RegisterDependencies(const PropertyAccessSet &access) const
{
    return dependencyRecorder_.Install(access);
}

}  // namespace panda::ecmascript::arksteed
