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
      numVregs_(literal->GetNumberVRegs() + 2), // 2: env and acc
      accumulatorIndex_(literal->GetNumberVRegs() + 1), // 1: acc
      circuit_(circuit),
      gateAcc_(circuit),
      argAcc_(circuit)
{
}

FrameStateBuilder::~FrameStateBuilder()
{
    for (auto state : bcEndStateInfos_) {
        if (state != nullptr) {
            delete state;
        }
    }
    for (auto state : bbBeginStateInfos_) {
        if (state != nullptr) {
            delete state;
        }
    }
    if (liveOutResult_ != nullptr) {
        delete liveOutResult_;
    }
    liveOutResult_ = nullptr;
    bcEndStateInfos_.clear();
    bbBeginStateInfos_.clear();
    builder_ = nullptr;
}

GateRef FrameStateBuilder::FrameState(size_t pcOffset, FrameStateInfo *stateInfo)
{
    size_t frameStateInputs = numVregs_ + 1; // +1: for pc
    std::vector<GateRef> inList(frameStateInputs, Circuit::NullGate());
    auto optimizedGate = circuit_->GetConstantGate(MachineType::I64,
                                                   JSTaggedValue::VALUE_OPTIMIZED_OUT,
                                                   GateType::TaggedValue());
    for (size_t i = 0; i < numVregs_; i++) {
        auto value = stateInfo->ValuesAt(i);
        if (value == Circuit::NullGate()) {
            value = optimizedGate;
        }
        inList[i] = value;
    }
    auto pcGate = circuit_->GetConstantGate(MachineType::I64,
                                            pcOffset,
                                            GateType::NJSValue());
    inList[numVregs_] = pcGate;
    return circuit_->NewGate(circuit_->FrameState(frameStateInputs), inList);
}

void FrameStateBuilder::BindStateSplit(GateRef gate, size_t pcOffset, FrameStateInfo *stateInfo)
{
    auto depend = gateAcc_.GetDep(gate);
    GateRef frameState = FrameState(pcOffset, stateInfo);
    GateRef stateSplit = circuit_->NewGate(circuit_->StateSplit(), {depend, frameState});
    gateAcc_.ReplaceDependIn(gate, stateSplit);
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
    pendingList.emplace_back(entryId);

    while (!pendingList.empty()) {
        size_t curBlockId = pendingList.back();
        visited[curBlockId] = true;

        bool change = false;
        auto &bb = builder_->GetBasicBlockById(curBlockId);
        for (const auto &succBlock: bb.succs) {
            if (!visited[succBlock->id]) {
                pendingList.emplace_back(succBlock->id);
                change = true;
                break;
            }
        }
        if (change) {
            continue;
        }
        for (const auto &succBlock: bb.catchs) {
            if (!visited[succBlock->id]) {
                pendingList.emplace_back(succBlock->id);
                change = true;
                break;
            }
        }
        if (!change) {
            postOrderList_.emplace_back(curBlockId);
            pendingList.pop_back();
        }
    }
}

bool FrameStateBuilder::MergeIntoPredBC(uint32_t predPc)
{
    // liveout next
    auto frameInfo = GetOrOCreateBCEndStateInfo(predPc);
    FrameStateInfo *predFrameInfo = liveOutResult_;
    bool changed = frameInfo->MergeLiveout(predFrameInfo);
    if (!changed) {
        return changed;
    }
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
    if (bb->numOfLoopBacks != 0) {
        ASSERT(bb->loopbackBlocks.size() != 0);
        auto forwardValue = gateAcc_.GetValueIn(phi, 0); // 0: fowward
        auto loopBackValue = gateAcc_.GetValueIn(phi, 1); // 1: back
        size_t backIndex = 0;
        size_t forwardIndex = 0;
        for (size_t i = 0; i < bb->numOfStatePreds; ++i) {
            auto predId = std::get<0>(bb->expandedPreds.at(i));
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
        auto predId = std::get<0>(bb->expandedPreds.at(i));
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
    auto predLiveout = GetOrOCreateBCEndStateInfo(predBb->end);
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
    auto &bb = builder_->GetBasicBlockById(bbId);
    bool changed = false;
    ASSERT(!bb.isDead);
    // iterator bc
    auto &iterator = bb.GetBytecodeIterator();
    iterator.GotoEnd();
    ASSERT(bb.end == iterator.Index());
    auto liveout = GetOrOCreateBCEndStateInfo(bb.end);
    liveOutResult_->CopyFrom(liveout);
    while (true) {
        auto &bytecodeInfo = iterator.GetBytecodeInfo();
        ComputeLiveOutBC(iterator.Index(), bytecodeInfo);
        --iterator;
        if (iterator.Done()) {
            break;
        }
        auto prevPc = iterator.Index();
        changed |= MergeIntoPredBC(prevPc);
    }

    SaveBBBeginStateInfo(bbId);

    bool defPhi = bb.valueSelectorAccGate != Circuit::NullGate() ||
        bb.vregToValSelectorGate.size() != 0;
    // merge current into pred bb
    for (auto bbPred : bb.preds) {
        if (bbPred->isDead) {
            continue;
        }
        if (defPhi) {
            changed |= MergeIntoPredBB(&bb, bbPred);
        } else {
            changed |= MergeIntoPredBC(bbPred->end);
        }
    }
    if (!bb.trys.empty()) {
        // clear GET_EXCEPTION gate if this is a catch block
        UpdateAccumulator(Circuit::NullGate());
        for (auto bbPred : bb.trys) {
            if (bbPred->isDead) {
                continue;
            }
            if (defPhi) {
                changed |= MergeIntoPredBB(&bb, bbPred);
            } else {
                changed |= MergeIntoPredBC(bbPred->end);
            }
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
    bcEndStateInfos_.resize(builder_->GetLastBcIndex() + 1, nullptr); // 1: +1 pcOffsets size
    auto size = builder_->GetBasicBlockCount();
    bbBeginStateInfos_.resize(size, nullptr);
    liveOutResult_ = CreateEmptyStateInfo();
    BuildPostOrderList(size);
    ComputeLiveState();
    BindStateSplit(size);
}

void FrameStateBuilder::ComputeLiveOutBC(uint32_t index, const BytecodeInfo &bytecodeInfo)
{
    if (bytecodeInfo.IsMov()) {
        auto gate = Circuit::NullGate();
        // variable kill
        if (bytecodeInfo.AccOut()) {
            gate = ValuesAtAccumulator();
            UpdateAccumulator(Circuit::NullGate());
        } else if (bytecodeInfo.vregOut.size() != 0) {
            auto out = bytecodeInfo.vregOut[0];
            gate = ValuesAt(out);
            UpdateVirtualRegister(out, Circuit::NullGate());
        }
        // variable use
        if (bytecodeInfo.AccIn()) {
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
    GateRef gate = builder_->GetGateByBcIndex(index);
    // variable kill
    if (bytecodeInfo.AccOut()) {
        UpdateAccumulator(Circuit::NullGate());
    }
    for (const auto &out: bytecodeInfo.vregOut) {
        UpdateVirtualRegister(out, Circuit::NullGate());
    }
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RESUMEGENERATOR) {
        UpdateVirtualRegistersOfResume(gate);
    }

    // variable use
    if (bytecodeInfo.AccIn()) {
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
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::SUSPENDGENERATOR_V8) {
        UpdateVirtualRegistersOfSuspend(gate);
    }
}

void FrameStateBuilder::BindStateSplit(size_t size)
{
    for (size_t i = 0; i < size; i++) {
        auto &bb = builder_->GetBasicBlockById(i);
        if (bb.isDead) {
            continue;
        }
        builder_->EnumerateBlock(bb, [&](const BytecodeInfo &bytecodeInfo) -> bool {
            if (bytecodeInfo.Deopt()) {
                auto &iterator = bb.GetBytecodeIterator();
                auto index = iterator.Index();
                auto gate = builder_->GetGateByBcIndex(index);
                auto pcOffset = builder_->GetPcOffset(index);
                auto stateInfo = GetCurrentFrameInfo(bb, index);
                BindStateSplit(gate, pcOffset, stateInfo);
            }
            return true;
        });
    }
}

FrameStateInfo *FrameStateBuilder::GetCurrentFrameInfo(BytecodeRegion &bb, uint32_t bcId)
{
    if (bcId == bb.start) {
        return GetBBBeginStateInfo(bb.id);
    } else {
        return GetOrOCreateBCEndStateInfo(bcId - 1); // 1: prev pc
    }
}

void FrameStateBuilder::SaveBBBeginStateInfo(size_t bbId)
{
    if (bbBeginStateInfos_[bbId] == nullptr) {
        bbBeginStateInfos_[bbId] = CreateEmptyStateInfo();
    }
    bbBeginStateInfos_[bbId]->CopyFrom(liveOutResult_);
}

void FrameStateBuilder::UpdateVirtualRegistersOfSuspend(GateRef gate)
{
    auto saveRegsGate = gateAcc_.GetDep(gate);
    size_t numOfRegs = gateAcc_.GetNumValueIn(saveRegsGate);
    for (size_t i = 0; i < numOfRegs; i++) {
        GateRef def = gateAcc_.GetValueIn(saveRegsGate, i);
        UpdateVirtualRegister(i, def);
    }
}

void FrameStateBuilder::UpdateVirtualRegistersOfResume(GateRef gate)
{
    auto restoreGate = gateAcc_.GetDep(gate);
    while (gateAcc_.GetOpCode(restoreGate) == OpCode::RESTORE_REGISTER) {
        auto vreg = static_cast<size_t>(gateAcc_.GetVirtualRegisterIndex(restoreGate));
        UpdateVirtualRegister(vreg, Circuit::NullGate());
        restoreGate = gateAcc_.GetDep(restoreGate);
    }
}
}