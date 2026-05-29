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

namespace panda::ecmascript::arksteed {

constexpr uint32_t NAMED_IC_POLY_CASE_WIDTH = 2;

bool ArkSteedFeedbackReader::TryGetFeedbackSlotId(int index, bool allowImmediate, uint32_t *slotId) const
{
    if (index < 0 || static_cast<size_t>(index) >= bytecodeInfo_.inputs.size()) {
        return false;
    }

    const auto &input = bytecodeInfo_.inputs[index];
    if (std::holds_alternative<panda::ecmascript::kungfu::ICSlotId>(input)) {
        *slotId = static_cast<uint32_t>(std::get<panda::ecmascript::kungfu::ICSlotId>(input).GetId());
        return true;
    }
    if (allowImmediate && std::holds_alternative<panda::ecmascript::kungfu::Immediate>(input)) {
        *slotId = static_cast<uint32_t>(std::get<panda::ecmascript::kungfu::Immediate>(input).GetValue());
        return true;
    }
    return false;
}

bool ArkSteedFeedbackReader::TryGetConstDataId(int index, uint16_t *constDataId) const
{
    if (index < 0 || static_cast<size_t>(index) >= bytecodeInfo_.inputs.size()) {
        return false;
    }

    const auto &input = bytecodeInfo_.inputs[index];
    if (!std::holds_alternative<panda::ecmascript::kungfu::ConstDataId>(input)) {
        return false;
    }
    *constDataId = std::get<panda::ecmascript::kungfu::ConstDataId>(input).GetId();
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

    JSTaggedValue first = profileTypeArray->Get(compilerThread_, slotId);
    JSTaggedValue second = profileTypeArray->Get(compilerThread_, slotId + 1);
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

    JSTaggedValue first = profileTypeArray->Get(compilerThread_, slotId);
    JSTaggedValue second = profileTypeArray->Get(compilerThread_, slotId + 1);
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

}  // namespace panda::ecmascript::arksteed
