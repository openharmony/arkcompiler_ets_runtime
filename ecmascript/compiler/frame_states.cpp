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
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/frame_states.h"

namespace panda::ecmascript::kungfu {
FrameStateBuilder::FrameStateBuilder(BytecodeCircuitBuilder *builder,
    Circuit *circuit, const MethodLiteral *literal)
    : builder_(builder),
      numVregs_(literal->GetNumberVRegs() + 1), // 1: +acc
      accumulatorIndex_(literal->GetNumberVRegs()),
      circuit_(circuit),
      gateAcc_(circuit),
      argAcc_(circuit)
{
}

FrameStateBuilder::~FrameStateBuilder()
{
    for (auto &state : stateInfos_) {
        delete state.second;
    }
    delete liveOutResult_;
    stateInfos_.clear();
    liveOutResult_ = nullptr;
    builder_ = nullptr;
}

GateRef FrameStateBuilder::FrameState(size_t pcOffset, FrameStateInfo *stateInfo)
{
    size_t frameStateInputs = numVregs_ + 1; // +1: for pc
    std::vector<GateRef> inList(frameStateInputs, Circuit::NullGate());
    auto undefinedGate = circuit_->GetConstantGate(MachineType::I64,
                                                   JSTaggedValue::VALUE_UNDEFINED,
                                                   GateType::TaggedValue());
    for (size_t i = 0; i < numVregs_; i++) {
        auto value = stateInfo->ValuesAt(i);
        if (value == Circuit::NullGate()) {
            value = undefinedGate;
        }
        inList[i] = value;
    }
    auto pcGate = circuit_->GetConstantGate(MachineType::I64,
                                            pcOffset,
                                            GateType::NJSValue());
    inList[numVregs_] = pcGate;
    return circuit_->NewGate(OpCode(OpCode::FRAME_STATE), frameStateInputs, inList, GateType::Empty());
}

void FrameStateBuilder::BindGuard(GateRef gate, size_t pcOffset, FrameStateInfo *stateInfo)
{
    auto depend = gateAcc_.GetDep(gate);
    GateRef glue = argAcc_.GetCommonArgGate(CommonArgIdx::GLUE);
    GateRef frameState = FrameState(pcOffset, stateInfo);
    auto trueGate = circuit_->GetConstantGate(MachineType::I1,
                                              1, // 1: true
                                              GateType::NJSValue());
    GateRef guard = circuit_->NewGate(
        OpCode(OpCode::GUARD), 3, {depend, trueGate, frameState, glue}, GateType::Empty());
    gateAcc_.ReplaceDependIn(gate, guard);
    if (builder_->IsLogEnabled()) {
        gateAcc_.ShortPrint(frameState);
    }
}

FrameStateInfo *FrameStateBuilder::CreateEmptyStateInfo()
{
    auto frameInfo = new FrameStateInfo(numVregs_);
    for (size_t i = 0; i < numVregs_; i++) {
        frameInfo->SetValuesAt(i, Circuit::NullGate());
    }
    return frameInfo;
}

void FrameStateBuilder::BuildPostOrderList(size_t size)
{
    postOrderList_.clear();
    std::deque<size_t> pendingList;
    std::vector<bool> visited(size, false);
    auto entryId = 0;
    pendingList.push_back(entryId);

    while (!pendingList.empty()) {
        size_t curBlockId = pendingList.back();
        visited[curBlockId] = true;

        bool change = false;
        auto bb = builder_->GetBasicBlockById(curBlockId);
        for (const auto &succBlock: bb.succs) {
            if (!visited[succBlock->id]) {
                pendingList.push_back(succBlock->id);
                change = true;
                break;
            }
        }
        if (change) {
            continue;
        }
        for (const auto &succBlock: bb.catchs) {
            if (!visited[succBlock->id]) {
                pendingList.push_back(succBlock->id);
                change = true;
                break;
            }
        }
        if (!change) {
            postOrderList_.push_back(curBlockId);
            pendingList.pop_back();
        }
    }
}

bool FrameStateBuilder::MergeIntoPredBC(const uint8_t* predPc)
{
    // liveout next
    auto frameInfo = GetOrOCreateStateInfo(predPc);
    FrameStateInfo *predFrameInfo = liveOutResult_;
    bool changed = false;
    for (size_t i = 0; i < numVregs_; i++) {
        auto predValue = predFrameInfo->ValuesAt(i);
        auto value = frameInfo->ValuesAt(i);
        // if value not null, merge pred
        if (value == Circuit::NullGate() && predValue != Circuit::NullGate()) {
            frameInfo->SetValuesAt(i, predValue);
            changed = true;
        }
    }
    return changed;
}

GateRef FrameStateBuilder::GetPhiComponent(BytecodeRegion *bb, BytecodeRegion *predBb, GateRef phi)
{
    ASSERT(gateAcc_.GetOpCode(phi) == OpCode::VALUE_SELECTOR);
    if (bb->numOfLoopBacks) {
        ASSERT(bb->loopbackBlocks.size() != 0);
        auto forwardValue = gateAcc_.GetValueIn(phi, 0); // 0: fowward
        auto loopBackValue = gateAcc_.GetValueIn(phi, 1); // 1: back
        size_t backIndex = 0;
        size_t forwardIndex = 0;
        for (size_t i = 0; i < bb->numOfStatePreds; ++i) {
            auto predId = std::get<size_t>(bb->expandedPreds.at(i));
            if (bb->loopbackBlocks.count(predId)) {
                if (predId == predBb->id) {
                    return gateAcc_.GetValueIn(loopBackValue, backIndex);
                }
                backIndex++;
            } else {
                if (predId == predBb->id) {
                    return gateAcc_.GetValueIn(forwardValue, forwardIndex);
                }
                forwardIndex++;
            }
        }
        return Circuit::NullGate();
    }

    ASSERT(gateAcc_.GetNumValueIn(phi) == bb->numOfStatePreds);
    for (size_t i = 0; i < bb->numOfStatePreds; ++i) {
        auto predId = std::get<size_t>(bb->expandedPreds.at(i));
        if (predId == predBb->id) {
            return gateAcc_.GetValueIn(phi, i);
        }
    }
    return Circuit::NullGate();
}

bool FrameStateBuilder::MergeIntoPredBB(BytecodeRegion *bb, BytecodeRegion *predBb)
{
    bool changed = MergeIntoPredBC(predBb->end);
    if (!changed) {
        return changed;
    }
    auto predLiveout = GetOrOCreateStateInfo(predBb->end);
    // replace phi
    if (bb->valueSelectorAccGate != Circuit::NullGate()) {
        auto phi = bb->valueSelectorAccGate;
        auto value = predLiveout->ValuesAt(accumulatorIndex_);
        if (value == phi) {
            auto target = GetPhiComponent(bb, predBb, phi);
            ASSERT(target != Circuit::NullGate());
            predLiveout->SetValuesAt(accumulatorIndex_, target);
        }
    }
    for (auto &it : bb->vregToValSelectorGate) {
        auto reg = it.first;
        auto phi = it.second;
        auto value = predLiveout->ValuesAt(reg);
        if (value == phi) {
            auto target = GetPhiComponent(bb, predBb, phi);
            ASSERT(target != Circuit::NullGate());
            predLiveout->SetValuesAt(reg, target);
        }
    }
    return changed;
}

bool FrameStateBuilder::ComputeLiveOut(size_t bbId)
{
    auto bb = builder_->GetBasicBlockById(bbId);
    bool changed = false;
    ASSERT(!bb.isDead);
    // iterator bc
    auto pc = bb.end;
    while (true) {
        auto liveout = GetOrOCreateStateInfo(pc);
        liveOutResult_->CopyFrom(liveout);
        ComputeLiveOutBC(pc);
        pc = builder_->GetByteCodeCurPrePc().at(pc);
        if (pc <= bb.start || pc >= bb.end) {
            break;
        }
        changed |= MergeIntoPredBC(pc);
    }

    bool defPhi = bb.valueSelectorAccGate != Circuit::NullGate() ||
        bb.vregToValSelectorGate.size() != 0;
    // merge current into pred bb
    for (auto bbPred : bb.preds) {
        if (defPhi) {
            changed |= MergeIntoPredBB(&bb, bbPred);
        } else {
            changed |= MergeIntoPredBC(pc);
        }
    }
    for (auto bbPred : bb.trys) {
        if (defPhi) {
            changed |= MergeIntoPredBB(&bb, bbPred);
        } else {
            changed |= MergeIntoPredBC(pc);
        }
    }
    return changed;
}

void FrameStateBuilder::ComputeLiveState()
{
    // recompute liveout
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < postOrderList_.size(); i++) {
            changed |= ComputeLiveOut(postOrderList_[i]);
        }
    }
}

void FrameStateBuilder::BuildFrameState()
{
    argAcc_.CollectArgs();
    auto size = builder_->GetBasicBlockCount();
    liveOutResult_ = CreateEmptyStateInfo();
    BuildPostOrderList(size);
    ComputeLiveState();
    BindGuard(size);
}

void FrameStateBuilder::ComputeLiveOutBC(const uint8_t * pc)
{
    auto byteCodeToJSGate = builder_->GetBytecodeToGate();
    auto bytecodeInfo = builder_->GetBytecodeInfo(pc);

    if (bytecodeInfo.IsMov()) {
        auto gate = Circuit::NullGate();
        // variable kill
        if (bytecodeInfo.accOut) {
            UpdateAccumulator(Circuit::NullGate());
            gate = ValuesAtAccumulator();
        } else if (bytecodeInfo.vregOut.size() != 0) {
            auto out = bytecodeInfo.vregOut[0];
            gate = ValuesAt(out);
            UpdateVirtualRegister(out, Circuit::NullGate());
        }
        // variable use
        if (bytecodeInfo.accIn) {
            UpdateAccumulator(gate);
        } else if (bytecodeInfo.inputs.size() != 0) {
            auto vreg = std::get<VirtualRegister>(bytecodeInfo.inputs.at(0)).GetId();
            UpdateVirtualRegister(vreg, gate);
        }
        return;
    }
    if (!bytecodeInfo.IsGeneral() && !bytecodeInfo.IsReturn() && !bytecodeInfo.IsCondJump()) {
        return;
    }
    GateRef gate = byteCodeToJSGate.at(pc);
    // variable kill
    if (bytecodeInfo.accOut) {
        UpdateAccumulator(Circuit::NullGate());
    }
    for (const auto &out: bytecodeInfo.vregOut) {
        UpdateVirtualRegister(out, Circuit::NullGate());
    }
    // variable use
    if (bytecodeInfo.accIn) {
        auto id = bytecodeInfo.inputs.size();
        GateRef def = gateAcc_.GetValueIn(gate, id);
        UpdateAccumulator(def);
    }
    for (size_t i = 0; i < bytecodeInfo.inputs.size(); i++) {
        auto in = bytecodeInfo.inputs[i];
        if (std::holds_alternative<VirtualRegister>(in)) {
            auto vreg = std::get<VirtualRegister>(in).GetId();
            GateRef def = gateAcc_.GetValueIn(gate, i);
            UpdateVirtualRegister(vreg, def);
        }
    }
}

void FrameStateBuilder::BindGuard(size_t size)
{
    auto byteCodeToJSGate = builder_->GetBytecodeToGate();
    for (size_t i = 0; i < size; i++) {
        auto bb = builder_->GetBasicBlockById(i);
        if (bb.isDead) {
            continue;
        }
        builder_->EnumerateBlock(bb, [=](uint8_t *pc, BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.deopt) {
                auto gate = byteCodeToJSGate.at(pc);
                pc = builder_->GetByteCodeCurPrePc().at(pc);
                auto stateInfo = GetOrOCreateStateInfo(pc);
                BindGuard(gate, bytecodeInfo.pcOffset, stateInfo);
            }
            return true;
        });
    }
}

}
