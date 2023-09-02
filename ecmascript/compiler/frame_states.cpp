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
#include <cstddef>

namespace panda::ecmascript::kungfu {
FrameStateBuilder::FrameStateBuilder(BytecodeCircuitBuilder *builder,
    Circuit *circuit, const MethodLiteral *literal)
    : builder_(builder),
      numVregs_(literal->GetNumberVRegs() + FIXED_ARGS),
      accumulatorIndex_(literal->GetNumberVRegs() + 1), // 1: acc
      circuit_(circuit),
      gateAcc_(circuit),
      bcEndStateInfos_(circuit->chunk()),
      bbBeginStateInfos_(circuit->chunk()),
      postOrderList_(circuit->chunk())
{
}

FrameStateBuilder::~FrameStateBuilder()
{
    liveOutResult_ = nullptr;
    bcEndStateInfos_.clear();
    bbBeginStateInfos_.clear();
    builder_ = nullptr;
}

GateRef FrameStateBuilder::BuildFrameValues(FrameStateInfo *stateInfo)
{
    size_t frameStateInputs = numVregs_;
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
    return circuit_->NewGate(circuit_->FrameValues(frameStateInputs), inList);
}

GateRef FrameStateBuilder::BuildFrameStateGate(size_t pcOffset, GateRef frameValues, FrameStateOutput output)
{
    GateRef frameArgs = builder_->GetFrameArgs();
    GateRef preFrameState = builder_->GetPreFrameState();
    UInt32PairAccessor accessor(static_cast<uint32_t>(pcOffset), output.GetValue());
    return circuit_->NewGate(circuit_->FrameState(accessor.ToValue()),
        {frameArgs, frameValues, preFrameState});
}

void FrameStateBuilder::BindStateSplit(GateRef state, GateRef depend, GateRef frameState)
{
    GateRef stateSplit = circuit_->NewGate(circuit_->StateSplit(), {state, depend, frameState});
    auto uses = gateAcc_.Uses(depend);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        if (gateAcc_.IsDependIn(useIt) && *useIt != stateSplit) {
            useIt = gateAcc_.ReplaceIn(useIt, stateSplit);
        } else {
            useIt++;
        }
    }
    if (builder_->IsLogEnabled()) {
        gateAcc_.ShortPrint(frameState);
    }
}

void FrameStateBuilder::BindStateSplit(GateRef gate, GateRef frameState)
{
    auto state = gateAcc_.GetState(gate);
    auto depend = gateAcc_.GetDep(gate);
    if (gateAcc_.GetOpCode(state) == OpCode::IF_SUCCESS) {
        state = gateAcc_.GetState(state);
    }
    GateRef stateSplit = circuit_->NewGate(circuit_->StateSplit(), {state, depend, frameState});
    gateAcc_.ReplaceDependIn(gate, stateSplit);
    if (builder_->IsLogEnabled()) {
        gateAcc_.ShortPrint(frameState);
    }
}

FrameStateInfo *FrameStateBuilder::CreateEmptyStateInfo()
{
    auto chunk = circuit_->chunk();
    auto frameInfo = chunk->New<FrameStateInfo>(chunk, numVregs_);
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
    // entry block (bbid=0) is a empty block, need skip
    auto firstBlockId = 1;
    pendingList.emplace_back(firstBlockId);

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

bool FrameStateBuilder::MergeIntoPredBC(uint32_t predPc, size_t diff)
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
            predValue = TryGetLoopExitValue(predValue, diff);
            frameInfo->SetValuesAt(i, predValue);
            changed = true;
        }
    }
    return changed;
}

GateRef FrameStateBuilder::GetPreBBInput(BytecodeRegion *bb, BytecodeRegion *predBb, GateRef gate)
{
    if (gateAcc_.GetOpCode(gate) == OpCode::VALUE_SELECTOR) {
        return GetPhiComponent(bb, predBb, gate);
    }
    return gate;
}

GateRef FrameStateBuilder::GetPhiComponent(BytecodeRegion *bb, BytecodeRegion *predBb, GateRef phi)
{
    ASSERT(gateAcc_.GetOpCode(phi) == OpCode::VALUE_SELECTOR);

    if (bb->phiGate.find(phi) == bb->phiGate.end()) {
        return Circuit::NullGate();
    }

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
                    if (bb->numOfLoopBacks == 1) {
                        return loopBackValue;
                    }
                    return gateAcc_.GetValueIn(loopBackValue, backIndex);
                }
                backIndex++;
            } else {
                if (predId == predBb->id) {
                    auto mergeCount = bb->numOfStatePreds - bb->numOfLoopBacks;
                    if (mergeCount == 1) {
                        return forwardValue;
                    }
                    return gateAcc_.GetValueIn(forwardValue, forwardIndex);
                }
                forwardIndex++;
            }
        }
        return Circuit::NullGate();
    }

    ASSERT(gateAcc_.GetNumValueIn(phi) == bb->numOfStatePreds);
    // The phi input nodes need to be traversed in reverse order, because there is a bb with multiple def points
    for (size_t i = bb->numOfStatePreds - 1; i >= 0; --i) {
        auto predId = std::get<0>(bb->expandedPreds.at(i));
        if (predId == predBb->id) {
            return gateAcc_.GetValueIn(phi, i);
        }
    }
    return Circuit::NullGate();
}

bool FrameStateBuilder::MergeIntoPredBB(BytecodeRegion *bb, BytecodeRegion *predBb)
{
    bool changed = MergeIntoPredBC(predBb->end, LoopExitCount(predBb, bb));
    if (!changed) {
        return changed;
    }
    auto predLiveout = GetOrOCreateBCEndStateInfo(predBb->end);
    // replace phi
    if (bb->valueSelectorAccGate != Circuit::NullGate()) {
        auto phi = bb->valueSelectorAccGate;
        auto value = predLiveout->ValuesAt(accumulatorIndex_);
        if (value == phi) {
            auto target = GetPreBBInput(bb, predBb, phi);
            if (target != Circuit::NullGate()) {
                auto diff = LoopExitCount(predBb, bb);
                target = TryGetLoopExitValue(target, diff);
                predLiveout->SetValuesAt(accumulatorIndex_, target);
            }
        }
    }
    for (auto &it : bb->vregToValueGate) {
        auto reg = it.first;
        auto gate = it.second;
        auto value = predLiveout->ValuesAt(reg);
        if (value == gate) {
            auto target = GetPreBBInput(bb, predBb, gate);
            if (target == Circuit::NullGate()) {
                continue;
            }
            auto diff = LoopExitCount(predBb, bb);
            target = TryGetLoopExitValue(target, diff);
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
        ComputeLiveOutBC(iterator.Index(), bytecodeInfo, bbId);
        --iterator;
        if (iterator.Done()) {
            break;
        }
        auto prevPc = iterator.Index();
        changed |= MergeIntoPredBC(prevPc, 0);
    }

    SaveBBBeginStateInfo(bbId);

    bool defPhi = bb.valueSelectorAccGate != Circuit::NullGate() ||
        bb.vregToValueGate.size() != 0;
    // merge current into pred bb
    for (auto bbPred : bb.preds) {
        if (bbPred->isDead) {
            continue;
        }
        if (defPhi) {
            changed |= MergeIntoPredBB(&bb, bbPred);
        } else {
            changed |= MergeIntoPredBC(bbPred->end, LoopExitCount(bbPred, &bb));
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
                changed |= MergeIntoPredBC(bbPred->end, LoopExitCount(bbPred, &bb));
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
    bcEndStateInfos_.resize(builder_->GetLastBcIndex() + 1, nullptr); // 1: +1 pcOffsets size
    auto size = builder_->GetBasicBlockCount();
    bbBeginStateInfos_.resize(size, nullptr);
    liveOutResult_ = CreateEmptyStateInfo();
    BuildPostOrderList(size);
    ComputeLiveState();
    BindBBStateSplit();
}

void FrameStateBuilder::ComputeLiveOutBC(uint32_t index, const BytecodeInfo &bytecodeInfo, size_t bbId)
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
        // when alive gate is null, find def
        if (bytecodeInfo.AccIn()) {
            if (gate == Circuit::NullGate()) {
                gate = builder_->ResolveDef(bbId, index, 0, true);
            }
            UpdateAccumulator(gate);
        } else if (bytecodeInfo.inputs.size() != 0) {
            auto vreg = std::get<VirtualRegister>(bytecodeInfo.inputs.at(0)).GetId();
            if (gate == Circuit::NullGate()) {
                gate = builder_->ResolveDef(bbId, index, vreg, false);
            }
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
    if (IsAsyncResolveOrSusp(bytecodeInfo)) {
        UpdateVirtualRegistersOfSuspend(gate);
    }
}

bool FrameStateBuilder::IsAsyncResolveOrSusp(const BytecodeInfo &bytecodeInfo)
{
    EcmaOpcode opcode = bytecodeInfo.GetOpcode();
    return opcode == EcmaOpcode::SUSPENDGENERATOR_V8 || opcode == EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8;
}

void FrameStateBuilder::BuildStateSplitAfter(size_t index, BytecodeRegion& bb)
{
    auto gate = builder_->GetGateByBcIndex(index);
    ASSERT(gateAcc_.GetOpCode(gate) == OpCode::JS_BYTECODE);
    auto nextIndex = GetNearestNextIndex(index, bb);
    if (builder_->GetBytecodeInfo(nextIndex).IsCall()) {
        return;
    }
    auto pcOffset = builder_->GetPcOffset(nextIndex);
    auto stateInfo = GetFrameInfoAfter(nextIndex - 1); // 1: after prev bc
    GateRef frameValues = BuildFrameValues(stateInfo);
    GateRef frameStateAfter = BuildFrameStateGate(
        pcOffset, frameValues, FrameStateOutput::Invalid());
    BindStateSplit(gate, gate, frameStateAfter);
}

size_t FrameStateBuilder::GetNearestNextIndex(size_t index, BytecodeRegion& bb) const
{
    index++;
    auto gate = builder_->GetGateByBcIndex(index);
    while ((gate == Circuit::NullGate() || gateAcc_.IsConstant(gate)) && index < bb.end) {
        index++;
        gate = builder_->GetGateByBcIndex(index);
    }
    return index;
}

void FrameStateBuilder::BuildStateSplitBefore(BytecodeRegion& bb, size_t index)
{
    auto pcOffset = builder_->GetPcOffset(index);
    auto stateInfo = GetFrameInfoBefore(bb, index);
    GateRef frameValues = BuildFrameValues(stateInfo);
    GateRef frameStateBefore = BuildFrameStateGate(
        pcOffset, frameValues, FrameStateOutput::Invalid());
    if (index == bb.start) {
        BindStateSplit(bb.stateCurrent, bb.dependCurrent, frameStateBefore);
    } else {
        auto gate = builder_->GetGateByBcIndex(index);
        BindStateSplit(gate, frameStateBefore);
    }
}

bool FrameStateBuilder::ShouldInsertFrameStateBefore(BytecodeRegion& bb, size_t index)
{
    auto gate = builder_->GetGateByBcIndex(index);
    if (index == bb.start) {
        if (bb.numOfStatePreds > 1) { // 1: > 1 is merge
            return true;
        } else if (bb.numOfStatePreds == 1) {   // 1: == 1 maybe loopexit
            auto predBb = (bb.preds.size() > 0) ? bb.preds.at(0) : bb.trys.at(0);
            if (LoopExitCount(predBb, &bb) > 0) {
                return true;
            }
        }
        if (gateAcc_.GetOpCode(bb.dependCurrent) == OpCode::GET_EXCEPTION) {
            return true;
        }
    } else {
        if (gate == Circuit::NullGate() || gateAcc_.GetStateCount(gate) != 1) {
            return false;
        }
        auto state = gateAcc_.GetState(gate);
        if (gateAcc_.GetOpCode(state) == OpCode::IF_SUCCESS) {
            return true;
        }
    }
    return false;
}

void FrameStateBuilder::BuildFrameState(BytecodeRegion& bb,
    const BytecodeInfo &bytecodeInfo, size_t index)
{
    // Not bind state split for Call
    if (bytecodeInfo.IsCall()) {
        BuildCallFrameState(index, bb);
    }

    bool needStateSplitBefore = ShouldInsertFrameStateBefore(bb, index);
    auto gate = builder_->GetGateByBcIndex(index);
    if (needStateSplitBefore && index != bb.start) {
        auto depend = gateAcc_.GetDep(gate);
        if (gateAcc_.GetOpCode(depend) == OpCode::STATE_SPLIT) {
            needStateSplitBefore = false;
        }
    }
    if (needStateSplitBefore) {
        BuildStateSplitBefore(bb, index);
    }

    if (!bytecodeInfo.NoSideEffects() && !bytecodeInfo.IsThrow()) {
        if (!gateAcc_.HasIfExceptionUse(gate)) {
            BuildStateSplitAfter(index, bb);
        }
    }
}

void FrameStateBuilder::BuildCallFrameState(size_t index, BytecodeRegion& bb)
{
    auto pcOffset = builder_->GetPcOffset(index);
    auto stateInfo = GetFrameInfoBefore(bb, index);
    GateRef frameValues = BuildFrameValues(stateInfo);
    GateRef frameState = BuildFrameStateGate(pcOffset, frameValues, FrameStateOutput::Invalid());
    auto gate = builder_->GetGateByBcIndex(index);
    gateAcc_.ReplaceFrameStateIn(gate, frameState);
}

void FrameStateBuilder::BindBBStateSplit()
{
    auto& dfsList = builder_->GetDfsList();
    for (auto &bbId: dfsList) {
        auto &bb = builder_->GetBasicBlockById(bbId);
        if (builder_->IsFirstBasicBlock(bb.id)) {
            BuildStateSplitBefore(bb, bb.start);
        }
        if (builder_->IsEntryBlock(bb.id)) {
            BuildStateSplitBefore(bb, bb.start);
        }
        ASSERT(!bb.isDead);
        builder_->EnumerateBlock(bb, [&](const BytecodeInfo &bytecodeInfo) -> bool {
            auto &iterator = bb.GetBytecodeIterator();
            auto index = iterator.Index();
            BuildFrameState(bb, bytecodeInfo, index);
            return true;
        });
    }
}

FrameStateInfo *FrameStateBuilder::GetFrameInfoBefore(BytecodeRegion &bb, uint32_t bcId)
{
    if (bcId == bb.start) {
        return GetBBBeginStateInfo(bb.id);
    } else {
        return GetOrOCreateBCEndStateInfo(bcId - 1); // 1: prev pc
    }
}

FrameStateInfo *FrameStateBuilder::GetFrameInfoAfter(uint32_t bcId)
{
    return GetOrOCreateBCEndStateInfo(bcId);
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
    auto uses = gateAcc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (gateAcc_.IsValueIn(it) && gateAcc_.GetOpCode(*it) == OpCode::RESTORE_REGISTER) {
            auto vreg = static_cast<size_t>(gateAcc_.GetVirtualRegisterIndex(*it));
            UpdateVirtualRegister(vreg, Circuit::NullGate());
        }
    }
}

size_t FrameStateBuilder::LoopExitCount(BytecodeRegion* bb, BytecodeRegion* bbNext)
{
    return builder_->LoopExitCount(bb->id, bbNext->id);
}

GateRef FrameStateBuilder::TryGetLoopExitValue(GateRef value, size_t diff)
{
    if ((gateAcc_.GetOpCode(value) != OpCode::LOOP_EXIT_VALUE) || (diff == 0)) {
        return value;
    }
    for (size_t i = 0; i < diff; ++i) {
        ASSERT(gateAcc_.GetOpCode(value) == OpCode::LOOP_EXIT_VALUE);
        value = gateAcc_.GetValueIn(value);
    }
    return value;
}
}
