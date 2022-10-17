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
#include "ecmascript/compiler/frame_states.h"

namespace panda::ecmascript::kungfu {
FrameStateInfo *FrameStateInfo::Clone()
{
    return new FrameStateInfo(this);
}

FrameStateBuilder::FrameStateBuilder(Circuit *circuit, const MethodLiteral *literal)
    : literal_(literal), circuit_(circuit), gateAcc_(circuit)
{
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

void FrameStateBuilder::InitFrameValues()
{
    auto values = currentInfo_->GetValues();
    auto undefinedGate = circuit_->GetConstantGate(MachineType::I64,
                                                   JSTaggedValue::VALUE_UNDEFINED,
                                                   GateType::TaggedValue());
    auto numArgs = literal_->GetNumberVRegs();
    for (size_t i = 0; i < numArgs; i++) {
        values->push_back(undefinedGate);
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

void FrameStateBuilder::BindGuard(GateRef gate, size_t pcOffset, GateRef glue)
{
    auto depend = gateAcc_.GetDep(gate);
    GateRef frameState = FrameState(pcOffset);
    auto trueGate = circuit_->GetConstantGate(MachineType::I1,
                                              1, // 1: true
                                              GateType::NJSValue());
    GateRef guard = circuit_->NewGate(
        OpCode(OpCode::GUARD), 3, {depend, trueGate, frameState, glue}, GateType::Empty());
    gateAcc_.ReplaceDependIn(gate, guard);
}

void FrameStateBuilder::AdvenceToBasicBlock(size_t id)
{
    if (currentInfo_ == nullptr) {
        // for last block
        currentInfo_ = new FrameStateInfo(literal_);
        stateInfos_[id] = currentInfo_;
        InitFrameValues();
        return;
    }
    auto frameInfo = stateInfos_[id];
    if (frameInfo == nullptr) {
        frameInfo = CloneFrameState(currentInfo_, id);
    }
    currentInfo_ = frameInfo;
}
}