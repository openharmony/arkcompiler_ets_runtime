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

#include "ecmascript/arksteed/arksteed_feedback_reader.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"

namespace panda::ecmascript::arksteed {

constexpr uint32_t NAMED_IC_POLY_CASE_WIDTH = 2;
constexpr int NAMED_ACCESS_SLOT_INPUT = 0;

bool ArkSteedFeedbackReader::TryGetFeedbackSlotId(int index, bool allowImmediate, uint32_t *slotId) const
{
    if (slotId == nullptr) {
        return false;
    }
    if (index == NAMED_ACCESS_SLOT_INPUT && !allowImmediate &&
        bytecodeInfo_.slotId.GetId() != kungfu::ICSlotId::INVALID_ID) {
        *slotId = static_cast<uint32_t>(bytecodeInfo_.slotId.GetId());
        return true;
    }

    if (index < 0 || static_cast<size_t>(index) >= bytecodeInfo_.inputs.size()) {
        return false;
    }

    const auto &input = bytecodeInfo_.inputs[index];
    if (std::holds_alternative<kungfu::ICSlotId>(input)) {
        *slotId = static_cast<uint32_t>(std::get<kungfu::ICSlotId>(input).GetId());
        return true;
    }
    if (allowImmediate && std::holds_alternative<kungfu::Immediate>(input)) {
        *slotId = static_cast<uint32_t>(std::get<kungfu::Immediate>(input).GetValue());
        return true;
    }
    return false;
}

bool ArkSteedFeedbackReader::TryGetFeedbackSlotId(uint32_t *slotId) const
{
    return TryGetFeedbackSlotId(NAMED_ACCESS_SLOT_INPUT, false, slotId);
}

bool ArkSteedFeedbackReader::TryGetConstDataId(int index, uint16_t *constDataId) const
{
    if (index < 0 || static_cast<size_t>(index) >= bytecodeInfo_.inputs.size()) {
        return false;
    }

    const auto &input = bytecodeInfo_.inputs[index];
    if (!std::holds_alternative<kungfu::ConstDataId>(input)) {
        return false;
    }
    *constDataId = std::get<kungfu::ConstDataId>(input).GetId();
    return true;
}

bool ArkSteedFeedbackReader::TryGetNamedICMonoSnapshot(int slotIndex, NamedICMonoSnapshot *snapshot) const
{
    uint32_t slotId = 0;
    if (!TryGetFeedbackSlotId(slotIndex, false, &slotId)) {
        return false;
    }

    ProfileTypeInfo *profileTypeArray = nullptr;
    if (!broker_->TryGetProfileTypeInfo(&profileTypeArray)) {
        return false;
    }
    if (slotId >= profileTypeArray->GetLength() || profileTypeArray->GetLength() - slotId <= 1) {
        return false;
    }

    JSTaggedValue first = profileTypeArray->GetICSlot(compilerThread_, slotId);
    JSTaggedValue second = profileTypeArray->GetICSlot(compilerThread_, slotId + 1);
    if (!first.IsWeak()) {
        return false;
    }
    snapshot->expectedHClass = broker_->MakeWeakHClassRef(first);
    if (!snapshot->expectedHClass.IsSafeForCompile()) {
        return false;
    }
    snapshot->handler = second;
    snapshot->slotId = slotId;
    return true;
}

bool ArkSteedFeedbackReader::TryGetNamedICPolySnapshot(int slotIndex, NamedICPolySnapshot *snapshot) const
{
    uint32_t slotId = 0;
    if (!TryGetFeedbackSlotId(slotIndex, false, &slotId)) {
        return false;
    }

    ProfileTypeInfo *profileTypeArray = nullptr;
    if (!broker_->TryGetProfileTypeInfo(&profileTypeArray)) {
        return false;
    }
    if (slotId >= profileTypeArray->GetLength() || profileTypeArray->GetLength() - slotId <= 1) {
        return false;
    }

    JSTaggedValue first = profileTypeArray->GetICSlot(compilerThread_, slotId);
    JSTaggedValue second = profileTypeArray->GetICSlot(compilerThread_, slotId + 1);
    if (first.IsWeak() || !first.IsHeapObject() || !first.IsTaggedArray() || !second.IsHole()) {
        return false;
    }

    auto *polyArray = TaggedArray::Cast(first.GetTaggedObject());
    if (polyArray->GetLength() == 0 || (polyArray->GetLength() % NAMED_IC_POLY_CASE_WIDTH) != 0) {
        return false;
    }
    snapshot->polyArray = polyArray;
    snapshot->caseCount = polyArray->GetLength() / NAMED_IC_POLY_CASE_WIDTH;
    snapshot->slotId = slotId;
    return snapshot->caseCount > 0;
}

bool ArkSteedFeedbackReader::TryGetNamedICPolyCase(const NamedICPolySnapshot &snapshot, uint32_t caseIndex,
                                                   NamedICCaseSnapshot *icCase) const
{
    if (snapshot.polyArray == nullptr || caseIndex >= snapshot.caseCount) {
        return false;
    }

    uint32_t index = caseIndex * NAMED_IC_POLY_CASE_WIDTH;
    JSTaggedValue cachedHClass = snapshot.polyArray->Get(compilerThread_, index);
    if (!cachedHClass.IsWeak()) {
        return false;
    }
    icCase->expectedHClass = broker_->MakeWeakHClassRef(cachedHClass);
    if (!icCase->expectedHClass.IsSafeForCompile()) {
        return false;
    }
    icCase->handler = snapshot.polyArray->Get(compilerThread_, index + 1);
    return true;
}

bool ArkSteedFeedbackReader::TryReadNamedAccessName(ArkSteedNameRef *name) const
{
    uint16_t constDataId = 0;
    if (!TryGetConstDataId(1, &constDataId)) {
        return false;
    }
    return broker_->TryGetNameFromConstantPool(constDataId, name);
}

NamedAccessCaseFeedback ArkSteedFeedbackReader::MakeNamedAccessCaseFeedback(ArkSteedHClassRef expectedHClass,
                                                                            JSTaggedValue handler) const
{
    NamedAccessCaseFeedback feedback;
    feedback.expectedHClass = expectedHClass;
    feedback.handler = broker_->MakeHandlerRef(handler);
    if (!feedback.expectedHClass.IsSafeForCompile() || !feedback.handler.IsSafeForCompile()) {
        return feedback;
    }
    return feedback;
}

bool ArkSteedFeedbackReader::ReadNamedAccessFeedback(int slotIndex, NamedAccessFeedback *feedback) const
{
    *feedback = {};
    if (!TryReadNamedAccessName(&feedback->name)) {
        *feedback = {};
        return false;
    }

    NamedICMonoSnapshot monoSnapshot;
    if (TryGetNamedICMonoSnapshot(slotIndex, &monoSnapshot)) {
        feedback->base.kind = ProcessedFeedbackKind::NAMED_ACCESS;
        feedback->base.source = {monoSnapshot.slotId, false, AccessFeedbackSlotKind::UNKNOWN};
        feedback->cases[0] = MakeNamedAccessCaseFeedback(monoSnapshot.expectedHClass, monoSnapshot.handler);
        if (!feedback->cases[0].expectedHClass.IsSafeForCompile() || !feedback->cases[0].handler.IsSafeForCompile()) {
            *feedback = {};
            return false;
        }
        feedback->caseCount = 1;
        return true;
    }

    NamedICPolySnapshot polySnapshot;
    if (!TryGetNamedICPolySnapshot(slotIndex, &polySnapshot)) {
        *feedback = {};
        return false;
    }
    if (polySnapshot.caseCount > MAX_NAMED_IC_POLY_CASES) {
        *feedback = {};
        return false;
    }

    feedback->base.kind = ProcessedFeedbackKind::NAMED_ACCESS;
    feedback->base.source = {polySnapshot.slotId, true, AccessFeedbackSlotKind::UNKNOWN};
    for (uint32_t i = 0; i < polySnapshot.caseCount && feedback->caseCount < MAX_NAMED_IC_POLY_CASES; ++i) {
        NamedICCaseSnapshot icCase;
        if (!TryGetNamedICPolyCase(polySnapshot, i, &icCase)) {
            *feedback = {};
            return false;
        }
        NamedAccessCaseFeedback caseFeedback = MakeNamedAccessCaseFeedback(icCase.expectedHClass, icCase.handler);
        if (!caseFeedback.expectedHClass.IsSafeForCompile() || !caseFeedback.handler.IsSafeForCompile()) {
            *feedback = {};
            return false;
        }
        feedback->cases[feedback->caseCount] = caseFeedback;
        feedback->caseCount++;
    }
    if (feedback->caseCount == 0) {
        *feedback = {};
        return false;
    }
    return true;
}

bool ArkSteedFeedbackReader::ReadValueAccessFeedback(ValueAccessFeedback *feedback) const
{
    *feedback = {};

    uint32_t slotId = 0;
    if (!TryGetFeedbackSlotId(&slotId)) {
        return false;
    }

    ProfileTypeInfo *profileTypeArray = nullptr;
    if (!broker_->TryGetProfileTypeInfo(&profileTypeArray) ||
        slotId >= profileTypeArray->GetLength() || profileTypeArray->GetLength() - slotId <= 1) {
        return false;
    }

    JSTaggedValue first = profileTypeArray->GetICSlot(compilerThread_, slotId);
    JSTaggedValue second = profileTypeArray->GetICSlot(compilerThread_, slotId + 1);
    TaggedArray *caseArray = nullptr;
    bool isPoly = false;

    if (first.IsWeak()) {
        feedback->kind = ValueAccessFeedbackKind::ELEMENT;
        feedback->cases[0] = MakeNamedAccessCaseFeedback(broker_->MakeWeakHClassRef(first), second);
        if (!feedback->cases[0].expectedHClass.IsSafeForCompile() ||
            !feedback->cases[0].handler.IsSafeForCompile()) {
            *feedback = {};
            return false;
        }
        feedback->caseCount = 1;
    } else if (first.IsTaggedArray() && second.IsHole()) {
        feedback->kind = ValueAccessFeedbackKind::ELEMENT;
        caseArray = TaggedArray::Cast(first.GetTaggedObject());
        isPoly = true;
    } else if ((first.IsString() || first.IsSymbol()) && second.IsTaggedArray()) {
        feedback->kind = ValueAccessFeedbackKind::NAMED;
        feedback->key = broker_->MakeNameRef(first);
        if (!feedback->key.IsSafeForCompile()) {
            *feedback = {};
            return false;
        }
        caseArray = TaggedArray::Cast(second.GetTaggedObject());
    } else {
        return false;
    }

    if (caseArray != nullptr) {
        uint32_t length = caseArray->GetLength();
        if (length == 0 || (length % NAMED_IC_POLY_CASE_WIDTH) != 0 ||
            length / NAMED_IC_POLY_CASE_WIDTH > MAX_NAMED_IC_POLY_CASES) {
            *feedback = {};
            return false;
        }
        for (uint32_t index = 0; index < length; index += NAMED_IC_POLY_CASE_WIDTH) {
            JSTaggedValue cachedHClass = caseArray->Get(compilerThread_, index);
            if (!cachedHClass.IsWeak()) {
                *feedback = {};
                return false;
            }
            NamedAccessCaseFeedback icCase = MakeNamedAccessCaseFeedback(
                broker_->MakeWeakHClassRef(cachedHClass), caseArray->Get(compilerThread_, index + 1));
            if (!icCase.expectedHClass.IsSafeForCompile() || !icCase.handler.IsSafeForCompile()) {
                *feedback = {};
                return false;
            }
            feedback->cases[feedback->caseCount++] = icCase;
        }
    }

    if (feedback->caseCount == 0) {
        *feedback = {};
        return false;
    }
    feedback->base.kind = ProcessedFeedbackKind::VALUE_ACCESS;
    feedback->base.source = {slotId, isPoly || feedback->caseCount > 1, AccessFeedbackSlotKind::VALUE_LOAD};
    return true;
}

ArkSteedOperationHint ArkSteedFeedbackReader::MakeOperationHint(uint32_t rawBits) const
{
    const auto prim = static_cast<pgo::PGOSampleType::Type>(rawBits);
    switch (prim) {
        case pgo::PGOSampleType::Type::NONE:
            return ArkSteedOperationHint::NONE;
        case pgo::PGOSampleType::Type::INT:
            return ArkSteedOperationHint::INT;
        case pgo::PGOSampleType::Type::INT_OVERFLOW:
        case pgo::PGOSampleType::Type::DOUBLE:
        case pgo::PGOSampleType::Type::NUMBER:
        case pgo::PGOSampleType::Type::NUMBER1:
            return ArkSteedOperationHint::NUMBER;
        case pgo::PGOSampleType::Type::STRING:
            return ArkSteedOperationHint::STRING;
        case pgo::PGOSampleType::Type::NUMBER_OR_STRING:
            return ArkSteedOperationHint::NUMBER_OR_STRING;
        default:
            break;
    }
    return ArkSteedOperationHint::ANY;
}

bool ArkSteedFeedbackReader::ReadOperationFeedback(OperationFeedback *feedback) const
{
    *feedback = {};

    uint32_t slotId = 0;
    if (!TryGetFeedbackSlotId(&slotId)) {
        return false;
    }

    ProfileTypeInfo *profileTypeArray = nullptr;
    if (!broker_->TryGetProfileTypeInfo(&profileTypeArray) || slotId >= profileTypeArray->GetICSlotLength()) {
        return false;
    }

    feedback->slotId = slotId;
    JSTaggedValue slotValue = profileTypeArray->GetICSlot(compilerThread_, slotId);
    if (!slotValue.IsInt()) {
        return true;
    }

    pgo::PGOSampleType profile(static_cast<uint32_t>(slotValue.GetInt()));
    uint32_t rawBits = static_cast<uint32_t>(profile.GetPrimitiveType());
    feedback->rawTypeBits = rawBits;
    feedback->trueWeight = profile.GetTrueWeight();
    feedback->falseWeight = profile.GetFalseWeight();
    feedback->hint = MakeOperationHint(rawBits);
    return true;
}

}  // namespace panda::ecmascript::arksteed
