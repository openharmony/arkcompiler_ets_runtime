/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/frame_states.h"

namespace panda::ecmascript::kungfu {
FrameStateInfo *FrameStateInfo::Clone()
{
    return new FrameStateInfo(this);
}

FrameStateBuilder::FrameStateBuilder(Circuit *circuit, const MethodLiteral *literal) :
    literal_(literal),
    circuit_(circuit),
    gateAcc_(circuit)
{
    currentInfo_ = new FrameStateInfo(literal_);
}

FrameStateBuilder::~FrameStateBuilder()
{
    for (auto &it : stateInfos_) {
        auto state = it.second;
        delete state;
    }
    stateInfos_.clear();
    currentInfo_ = nullptr;
}

void FrameStateBuilder::BuildArgsValues(ArgumentAccessor *argAcc)
{
    auto callFieldNumVregs = literal_->GetNumVregsWithCallField();
    auto undefinedGate = circuit_->GetConstantGate(MachineType::I64,
                                                   JSTaggedValue::VALUE_UNDEFINED,
                                                   GateType::TaggedValue());
    auto values = currentInfo_->GetValues();
    for (size_t i = 0; i < callFieldNumVregs; i++) {
        values->push_back(undefinedGate);
    }
    if (literal_->HaveFuncWithCallField()) {
        auto gate = argAcc->GetCommonArgGate(CommonArgIdx::FUNC);
        values->push_back(gate);
    }
    if (literal_->HaveNewTargetWithCallField()) {
        auto gate = argAcc->GetCommonArgGate(CommonArgIdx::NEW_TARGET);
        values->push_back(gate);
    }
    if (literal_->HaveThisWithCallField()) {
        auto gate = argAcc->GetCommonArgGate(CommonArgIdx::THIS);
        values->push_back(gate);
    }

    auto numArgs = literal_->GetNumArgsWithCallField();
    for (size_t i = 0; i < numArgs; i++) {
        auto gate = argAcc->ArgsAt(i + static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS));
        values->push_back(gate);
    }
    // push acc
    values->push_back(undefinedGate);
    ASSERT(currentInfo_->GetNumberVRegs() == values->size());
}

GateRef FrameStateBuilder::FrameState(size_t pcOffset)
{
    auto numVregs = currentInfo_->GetNumberVRegs();
    size_t frameStateInputs = numVregs + 1; // +1: for pc
    std::vector<GateRef> inList(frameStateInputs, Circuit::NullGate());
    for (size_t i = 0; i < numVregs; i++) {
        inList[i] = ValuesAt(i);
    }
    auto pcGate = circuit_->GetConstantGate(MachineType::I64,
                                            pcOffset,
                                            GateType::NJSValue());
    inList[numVregs] = pcGate;
    return circuit_->NewGate(OpCode(OpCode::FRAME_STATE), frameStateInputs, inList, GateType::Empty());
}

void FrameStateBuilder::BindCheckPoint(GateRef gate, size_t pcOffset)
{
    auto depend = gateAcc_.GetDep(gate);
    GateRef frameState = FrameState(pcOffset);
    GateRef checkPoint = circuit_->NewGate(
        OpCode(OpCode::GUARD), 1, {depend, frameState}, GateType::Empty());
    gateAcc_.ReplaceDependIn(gate, checkPoint);
}

void FrameStateBuilder::AdvenceToSuccessor(const uint8_t *predPc, const uint8_t *endPc)
{
    size_t pcOffset = static_cast<size_t>(endPc - literal_->GetBytecodeArray());
    if (predPc != nullptr) {
        size_t predPcOffset = static_cast<size_t>(predPc - literal_->GetBytecodeArray());
        auto predFrameInfo = stateInfos_[predPcOffset];
        ASSERT(predFrameInfo != nullptr);
        currentInfo_ = predFrameInfo;
        currentInfo_ = CloneFrameState(pcOffset);
    } else {
        // for first block
        stateInfos_[pcOffset] = currentInfo_;
    }
}
}