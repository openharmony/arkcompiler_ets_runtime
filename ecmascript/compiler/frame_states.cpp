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

#include <cstddef>

#include "ecmascript/compiler/bytecode_circuit_builder.h"

namespace panda::ecmascript::kungfu {
FrameStateBuilder::FrameStateBuilder(BytecodeCircuitBuilder *builder,
    Circuit *circuit, const MethodLiteral *literal)
    : bcBuilder_(builder),
      pgoTypeRecorder_(builder->GetPGOTypeRecorder()),
      numVregs_(literal->GetNumberVRegs() + FIXED_ARGS),
      accumulatorIndex_(literal->GetNumberVRegs() + 1), // 1: acc
      envIndex_(literal->GetNumberVRegs()),
      circuit_(circuit),
      acc_(circuit),
      bcEndStateLiveouts_(circuit->chunk()),
      bbBeginStateLiveouts_(circuit->chunk()),
      bbFrameContext_(circuit->chunk()),
      loops_(circuit->chunk()),
      rpoList_(circuit->chunk()),
      postOrderList_(circuit->chunk())
{
}

FrameStateBuilder::~FrameStateBuilder()
{
    liveOutResult_ = nullptr;
    bcEndStateLiveouts_.clear();
    bbBeginStateLiveouts_.clear();
    bbFrameContext_.clear();
    bcBuilder_ = nullptr;
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
        auto &bb = bcBuilder_->GetBasicBlockById(curBlockId);
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
        for (const auto &succBlock: bb.catches) {
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
    auto liveout = GetOrOCreateBCEndLiveOut(predPc);
    FrameLiveOut *predliveOut = liveOutResult_;
    return liveout->MergeLiveout(predliveOut);
}

bool FrameStateBuilder::MergeFromSuccBB(size_t bbId)
{
    // liveout next
    auto liveout = GetOrOCreateBBLiveOut(bbId);
    return liveOutResult_->MergeLiveout(liveout);
}

void FrameStateBuilder::MergeFromCatchBB(size_t bbId)
{
    // liveout next
    bool accumulatorIsLive = liveOutResult_->TestBit(accumulatorIndex_);
    auto liveout = GetOrOCreateBBLiveOut(bbId);
    liveOutResult_->MergeLiveout(liveout);
    // accumulatorIndex_ is exeception object
    if (!accumulatorIsLive) {
        liveOutResult_->ClearBit(accumulatorIndex_);
    }
}

bool FrameStateBuilder::ComputeLiveOut(size_t bbId)
{
    auto &bb = bcBuilder_->GetBasicBlockById(bbId);
    bool changed = false;
    // iterator bc
    auto &iterator = bb.GetBytecodeIterator();
    iterator.GotoEnd();
    ASSERT(bb.end == iterator.Index());
    auto bbLiveout = GetOrOCreateBBLiveOut(bb.id);
    auto liveout = GetOrOCreateBCEndLiveOut(bb.end);
    liveOutResult_->CopyFrom(liveout);
    // init frameContext
    currentBBliveOut_ = bbLiveout;
    while (true) {
        auto &bytecodeInfo = iterator.GetBytecodeInfo();
        if (!bb.catches.empty() && !bytecodeInfo.NoThrow()) {
            ASSERT(bb.catches.size() == 1); // 1: one catch
            MergeFromCatchBB(bb.catches.at(0)->id);
        }
        ComputeLiveOutBC(bytecodeInfo);
        --iterator;
        if (iterator.Done()) {
            break;
        }
        auto prevPc = iterator.Index();
        changed |= MergeIntoPredBC(prevPc);
    }
    if (!bb.trys.empty()) {
        // clear GET_EXCEPTION gate if this is a catch block
        liveOutResult_->ClearBit(accumulatorIndex_);
    }
    bbLiveout->CopyFrom(liveOutResult_);
    // merge current into pred bb
    for (auto bbPred : bb.preds) {
        changed |= MergeIntoPredBC(bbPred->end);
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

void FrameStateBuilder::DoBytecodeAnalysis()
{
    auto bcSize = bcBuilder_->GetLastBcIndex() + 1; // 1: +1 pcOffsets size
    auto bbSize = bcBuilder_->GetBasicBlockCount();
    bcEndStateLiveouts_.resize(bcSize, nullptr);
    bbBeginStateLiveouts_.resize(bbSize, nullptr);
    auto chunk = circuit_->chunk();
    liveOutResult_ = chunk->New<FrameLiveOut>(chunk, numVregs_);
    bbFrameContext_.resize(bbSize, nullptr);
    BuildPostOrderList(bbSize);
    ComputeLiveState();
    if (bcBuilder_->IsLogEnabled()) {
        DumpLiveState();
    }
    ComputeLoopInfo();
}

void FrameStateBuilder::ComputeLiveOutBC(const BytecodeInfo &bytecodeInfo)
{
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RESUMEGENERATOR) {
        currentBBliveOut_->defRegisters_.Union(liveOutResult_->liveout_);
    }
    // variable kill
    if (bytecodeInfo.AccOut()) {
        liveOutResult_->ClearBit(accumulatorIndex_);
        currentBBliveOut_->defRegisters_.SetBit(accumulatorIndex_);
    }
    for (const auto &out: bytecodeInfo.vregOut) {
        liveOutResult_->ClearBit(out);
        currentBBliveOut_->defRegisters_.SetBit(out);
    }

    // variable use
    if (bytecodeInfo.AccIn()) {
        liveOutResult_->SetBit(accumulatorIndex_);
    }
    for (size_t i = 0; i < bytecodeInfo.inputs.size(); i++) {
        auto in = bytecodeInfo.inputs[i];
        if (std::holds_alternative<VirtualRegister>(in)) {
            auto vreg = std::get<VirtualRegister>(in).GetId();
            liveOutResult_->SetBit(vreg);
        }
    }
}

FrameLiveOut *FrameStateBuilder::GetOrOCreateBCEndLiveOut(uint32_t bcIndex)
{
    auto liveout = bcEndStateLiveouts_[bcIndex];
    if (liveout == nullptr) {
        auto chunk = circuit_->chunk();
        liveout = chunk->New<FrameLiveOut>(chunk, numVregs_);
        bcEndStateLiveouts_[bcIndex] = liveout;
    }
    return liveout;
}

FrameLiveOut *FrameStateBuilder::GetOrOCreateBBLiveOut(size_t bbIndex)
{
    // As BB0 is empty, its bbBeginStateLiveouts is the same as BB1.
    if (bbIndex == 0) {
        if (bcBuilder_->IsOSR()) {
            bbIndex = GetOsrLoopHeadBBId();
        } else {
            bbIndex = 1;
        }
    }
    auto liveout = bbBeginStateLiveouts_[bbIndex];
    if (liveout == nullptr) {
        auto chunk = circuit_->chunk();
        liveout = chunk->New<FrameLiveOut>(chunk, numVregs_);
        bbBeginStateLiveouts_[bbIndex] = liveout;
    }
    return liveout;
}

FrameContext *FrameStateBuilder::GetOrOCreateMergedContext(uint32_t bbIndex)
{
    auto context = bbFrameContext_[bbIndex];
    if (context == nullptr) {
        auto chunk = circuit_->chunk();
        context = chunk->New<FrameContext>(chunk, numVregs_);
        for (size_t i = 0; i < numVregs_; i++) {
            context->SetValuesAt(i, Circuit::NullGate());
        }
        bbFrameContext_[bbIndex] = context;
    }
    return context;
}

void FrameStateBuilder::FillBcInputs(const BytecodeInfo &bytecodeInfo, GateRef gate)
{
    auto pgoType = pgoTypeRecorder_->GetPGOType(acc_.TryGetPcOffset(gate));
    acc_.TrySetPGOType(gate, pgoType);

    auto valueCount = acc_.GetInValueCount(gate);
    [[maybe_unused]] size_t numValueInputs = bytecodeInfo.ComputeValueInputCount();
    [[maybe_unused]] size_t numValueOutputs = bytecodeInfo.ComputeOutCount();
    // RETURNUNDEFINED has value input, but not from acc
    ASSERT(numValueInputs == valueCount || bytecodeInfo.GetOpcode() == EcmaOpcode::RETURNUNDEFINED);
    ASSERT(numValueOutputs <= 1 + (bytecodeInfo.EnvOut() ? 1 : 0));
    auto valueStarts = acc_.GetInValueStarts(gate);
    for (size_t valueIdx = 0; valueIdx < valueCount; valueIdx++) {
        auto inIdx = valueIdx + valueStarts;
        if (!acc_.IsInGateNull(gate, inIdx)) {
            continue;
        }
        if (valueIdx < bytecodeInfo.inputs.size()) {
            auto vregId = std::get<VirtualRegister>(bytecodeInfo.inputs.at(valueIdx)).GetId();
            GateRef defVreg = liveContext_->ValuesAt(vregId);
            acc_.NewIn(gate, inIdx, defVreg);
        } else {
            GateRef defAcc = liveContext_->ValuesAt(accumulatorIndex_);
            acc_.NewIn(gate, inIdx, defAcc);
        }
    }
}

void FrameStateBuilder::AdvanceToNextBc(const BytecodeInfo &bytecodeInfo, FrameLiveOut* liveout, uint32_t bcId)
{
    if (bytecodeInfo.IsGeneral()) {
        BindStateSplitBefore(bytecodeInfo, liveout, bcId);
        if (bytecodeInfo.GetOpcode() == EcmaOpcode::SUSPENDGENERATOR_V8 ||
            bytecodeInfo.GetOpcode() == EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8) {
            auto hole = circuit_->GetConstantGate(MachineType::I64,
                                                  JSTaggedValue::VALUE_HOLE,
                                                  GateType::TaggedValue());
            uint32_t numRegs = accumulatorIndex_;
            std::vector<GateRef> vec(numRegs + 1, hole);
            vec[0] = liveContext_->currentDepend_;
            // accumulator is res
            for (size_t i = 0; i < numRegs; i++) {
                if (liveout->TestBit(i)) {
                    vec[i + 1] = liveContext_->ValuesAt(i);  // 1: skip dep
                } else {
                    vec[i + 1] = hole; // 1: skip dep
                }
            }
            auto res = circuit_->NewGate(circuit_->SaveRegister(numRegs), vec);
            liveContext_->currentDepend_ = res;
        }
    }
}

void FrameStateBuilder::UpdateStateDepend(GateRef state, GateRef depend)
{
    liveContext_->currentState_ = state;
    liveContext_->currentDepend_ = depend;
}

void FrameStateBuilder::UpdateMoveValues(const BytecodeInfo &bytecodeInfo)
{
    GateRef gate = Circuit::NullGate();
    if (bytecodeInfo.AccIn()) {
        gate = liveContext_->ValuesAt(accumulatorIndex_);
    } else if (bytecodeInfo.inputs.size() != 0) {
        auto vreg = std::get<VirtualRegister>(bytecodeInfo.inputs.at(0)).GetId();
        gate = liveContext_->ValuesAt(vreg);
    }
    // variable kill
    if (bytecodeInfo.AccOut()) {
        liveContext_->SetValuesAt(accumulatorIndex_, gate);
    } else if (bytecodeInfo.vregOut.size() != 0) {
        auto vreg = bytecodeInfo.vregOut[0];
        liveContext_->SetValuesAt(vreg, gate);
    }
}

void FrameStateBuilder::UpdateFrameValues(const BytecodeInfo &bytecodeInfo,
    uint32_t bcId, GateRef gate)
{
    ASSERT(!bytecodeInfo.IsDiscarded() && !bytecodeInfo.IsMov());
    if (bytecodeInfo.IsSetConstant()) {
        liveContext_->SetValuesAt(accumulatorIndex_, gate);
        return;
    }
    // jump gate is null
    if (IsGateNotEmpty(gate)) {
        FillBcInputs(bytecodeInfo, gate);
    }
    auto liveout = GetFrameLiveoutAfter(bcId);
    // variable kill
    if (bytecodeInfo.AccOut()) {
        liveContext_->SetValuesAt(accumulatorIndex_, gate);
    }
    for (const auto &out: bytecodeInfo.vregOut) {
        liveContext_->SetValuesAt(out, gate);
    }
    if (bytecodeInfo.GetOpcode() == EcmaOpcode::RESUMEGENERATOR) {
        // accumulator is generator object
        for (size_t i = 0; i < accumulatorIndex_; i++) {
            if (liveout->TestBit(i)) {
                auto restore = circuit_->NewGate(circuit_->RestoreRegister(i),
                    MachineType::I64, { gate }, GateType::AnyType());
                liveContext_->SetValuesAt(i, restore);
            }
        }
    }
    BindStateSplitAfter(bytecodeInfo, bcId, gate);
}

void FrameStateBuilder::SetOsrLoopHeadBB(const BytecodeRegion &osrLoopBodyBB)
{
    auto *loopInfo = GetLoopInfoByLoopBody(osrLoopBodyBB);
    if (loopInfo == nullptr) {
        loopHeadOfOSR_ = nullptr;
    } else {
        loopHeadOfOSR_ = &(bcBuilder_->GetBasicBlockById(loopInfo->loopHeadId));
    }
    return;
}

bool FrameStateBuilder::IsOsrLoopExit(const BytecodeRegion &curBB)
{
    if (loopHeadOfOSR_ == nullptr) {
        return false;
    }
    auto *loopInfo = GetLoopInfoByLoopBody(*loopHeadOfOSR_);
    if (!loopInfo || !loopInfo->loopExits) {
        return false;
    }
    for (auto *exit : *loopInfo->loopExits) {
        if (exit == &curBB) {
            return true;
        }
    }
    return false;
}

bool FrameStateBuilder::OutOfOsrLoop(const BytecodeRegion &curBB)
{
    if (loopHeadOfOSR_ == nullptr) {
        return false;
    }
    const LoopInfo &loopInfoOfOSR = GetLoopInfo(*loopHeadOfOSR_);
    const LoopInfo *curLoop = GetLoopInfoByLoopBody(curBB);
    while (curLoop != nullptr) {
        if (curLoop == &loopInfoOfOSR) {
            return false;
        }
        curLoop = curLoop->parentInfo;
    }
    return true;
}

size_t FrameStateBuilder::GetOsrLoopHeadBBId() const
{
    if (loopHeadOfOSR_ == nullptr) {
        return -1;
    }
    return loopHeadOfOSR_->id;
}

void FrameStateBuilder::InitEntryBB(const BytecodeRegion &bb)
{
    auto frameContext = GetOrOCreateMergedContext(bb.id);
    frameContext->currentState_ = circuit_->GetStateRoot();
    frameContext->currentDepend_ = circuit_->GetDependRoot();
    frameContext->needStateSplit_ = true;
    // initialize argumnets
    ASSERT(bcBuilder_->IsFirstBasicBlock(1)); // 1: is firstBlock
    auto liveout = GetFrameLiveoutBefore(1); // 1: is firstBlock
    GateRef frameArgs = bcBuilder_->GetFrameArgs();
    if (liveout->TestBit(envIndex_)) {
        GateRef jsFunc = acc_.GetValueIn(frameArgs, static_cast<size_t>(FrameArgIdx::FUNC));
        auto env = acc_.GetInitialEnvGate(frameContext->currentDepend_, jsFunc);
        frameContext->SetValuesAt(envIndex_, env);
        frameContext->currentDepend_ = env;
    }
    auto holeGate = circuit_->GetConstantGate(MachineType::I64,
                                              JSTaggedValue::VALUE_HOLE,
                                              GateType::TaggedValue());
    for (size_t i = 0; i < envIndex_; i++) {
        if (liveout->TestBit(i)) {
            if (bcBuilder_->ArgGateNotExisted(i)) {
                frameContext->SetValuesAt(i, holeGate);
            } else {
                GateRef arg = bcBuilder_->GetArgGate(i);
                frameContext->SetValuesAt(i, arg);
            }
        }
    }

    // init live interpreter registers
    if (!bcBuilder_->IsOSR()) {
        return;
    }
    auto *liveOut = GetFrameLiveoutBefore(GetOsrLoopHeadBBId());
    for (size_t i = 0; i < envIndex_; i++) {
        if (!liveOut->TestBit(i)) {
            continue;
        }
        GateRef init = circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(i), MachineType::I64,
                                         {circuit_->GetArgRoot()}, GateType::TaggedValue());
        frameContext->SetValuesAt(i, init);
    }
    if (liveOut->TestBit(envIndex_)) {
        // -7: env
        GateRef env = circuit_->NewGate(circuit_->GetMetaBuilder()->InitVreg(INIT_VRGE_ENV), MachineType::I64,
                                        {circuit_->GetArgRoot()}, GateType::TaggedValue());
        frameContext->SetValuesAt(envIndex_, env);
    }
}

bool FrameStateBuilder::IsLoopHead(const BytecodeRegion &bb)
{
    return !(bcBuilder_->IsOSR() && OutOfOsrLoop(bb)) && bb.loopNumber > 0;
}

bool FrameStateBuilder::IfLoopNeedMerge(const BytecodeRegion &bb) const
{
    return !bcBuilder_->IsOSR() && bb.numOfStatePreds - bb.numOfLoopBack != 1; // 1: loop in
}

GateRef FrameStateBuilder::InitMerge(size_t numOfIns, bool isLoop)
{
    const GateMetaData *metaData = isLoop ? circuit_->LoopBegin(numOfIns) : circuit_->Merge(numOfIns);
    return circuit_->NewGate(metaData, std::vector<GateRef>(numOfIns, Circuit::NullGate()));
}

bool FrameStateBuilder::IsGateNotEmpty(GateRef gate) const
{
    return gate != Circuit::NullGate();
}

void FrameStateBuilder::NewMerge(const BytecodeRegion &bbNext)
{
    auto frameContext = GetMergedBbContext(bbNext.id);
    size_t numOfIns = bbNext.numOfStatePreds;
    if (IsOsrLoopExit(bbNext)) {
        // Only the precursor within the OSR loop is required.
        numOfIns = 0;
        for (const BytecodeRegion *bb : bbNext.preds) {
            if (OutOfOsrLoop(*bb)) {
                continue;
            }
            numOfIns++;
        }
    }
    
    bool isLoopHead = IsLoopHead(bbNext);
    GateRef merge;
    GateRef dependMerge;
    if (isLoopHead) {
        if (!IfLoopNeedMerge(bbNext)) {
            // only generate loop begin
            merge = InitMerge(numOfIns, true);
            dependMerge = circuit_->NewGate(circuit_->DependSelector(numOfIns),
                std::vector<GateRef>(numOfIns + 1, Circuit::NullGate())); // 1: merge
        } else {
            // generate both loop begin and merge
            ASSERT(numOfIns - bbNext.numOfLoopBack > 1); // 1: loop in
            size_t numOfLoopIns = bbNext.numOfLoopBack + 1; // 1: loop in
            merge = InitMerge(numOfLoopIns, true);
            dependMerge = circuit_->NewGate(circuit_->DependSelector(numOfLoopIns),
                std::vector<GateRef>(numOfLoopIns + 1, Circuit::NullGate())); // 1: merge
            size_t numOfMergeIns = numOfIns - bbNext.numOfLoopBack;
            frameContext->mergeState_ = InitMerge(numOfMergeIns, false);
            frameContext->mergeDepend_ = circuit_->NewGate(circuit_->DependSelector(numOfMergeIns),
                std::vector<GateRef>(numOfMergeIns + 1, Circuit::NullGate())); // 1: merge
            acc_.NewIn(frameContext->mergeDepend_, 0, frameContext->mergeState_);
            acc_.NewIn(dependMerge, 1, frameContext->mergeDepend_); // 1: phi of merge
            acc_.NewIn(merge, 0, frameContext->mergeState_);
            frameContext->loopBackIndex_++;
        }
        frameContext->loopBackDepend_ = dependMerge;
        frameContext->loopBackState_ = merge;
    } else {
        // only merge
        merge = InitMerge(numOfIns, false);
        dependMerge = circuit_->NewGate(circuit_->DependSelector(numOfIns),
            std::vector<GateRef>(numOfIns + 1, Circuit::NullGate())); // 1: merge
        frameContext->mergeDepend_ = dependMerge;
        frameContext->mergeState_ = merge;
    }
    acc_.NewIn(dependMerge, 0, merge);  // 0: is state
    // reset current state and depend
    frameContext->currentState_ = merge;
    frameContext->currentDepend_ = dependMerge;

    if (isLoopHead) {
        ChunkVector<GateRef>& headerGates = bcBuilder_->GetLoopHeaderGates();
        auto& loopInfo = GetLoopInfo(bbNext);
        headerGates[loopInfo.sortIndx] = merge;
    }
}

void FrameStateBuilder::MergeStateDepend(const BytecodeRegion &bb, const BytecodeRegion &bbNext)
{
    GateRef entryState = liveContext_->currentState_;
    GateRef entryDepend = liveContext_->currentDepend_;
    auto mergedContext = GetMergedBbContext(bbNext.id);
    if (bbNext.numOfStatePreds == 1) { // 1: one entry edge
        mergedContext->currentState_ = liveContext_->currentState_;
        mergedContext->currentDepend_ = liveContext_->currentDepend_;
        return;
    }
    auto index = mergedContext->currentIndex_;
    // lazy first edge
    if (index == 0) {
        NewMerge(bbNext);
    }
    if (IsLoopBackEdge(bb, bbNext)) {
        if (!(bcBuilder_->IsOSR() && IsOsrLoopExit(bbNext))) {
            ASSERT(index != 0);
        }
        entryState = circuit_->NewGate(circuit_->LoopBack(), { entryState });
    }
    auto mergeInfo = GetCorrespondingState(bb, bbNext);
    acc_.NewIn(mergeInfo.state, mergeInfo.index, entryState);
    acc_.NewIn(mergeInfo.depend, mergeInfo.index + 1, entryDepend); // 1: skip state
    mergedContext->needStateSplit_ = true;
}

size_t FrameStateBuilder::GetNumOfStatePreds(const BytecodeRegion &bb)
{
    size_t numOfIns = bb.numOfStatePreds;
    if (bcBuilder_->IsOSR() && IsOsrLoopExit(bb)) {
        numOfIns = 0;
        for (const BytecodeRegion *b : bb.preds) {
            if (OutOfOsrLoop(*b)) {
                continue;
            }
            numOfIns++;
        }
    }
    return numOfIns;
}

GateRef FrameStateBuilder::MergeValue(const BytecodeRegion &bb,
    GateRef currentValue, GateRef nextValue, bool isLoopBack, bool changedInLoop)
{
    ASSERT(IsGateNotEmpty(currentValue));
    auto mergedContext = GetMergedBbContext(bb.id);
    GateRef result = currentValue;
    GateRef mergeValueSelector;
    // When next is empty, if next has not changed within the loop, there is no need to merge;
    bool skipMergeValues = !changedInLoop && !IsGateNotEmpty(nextValue);

    // if already a merged gate
    if (IsGateNotEmpty(nextValue) &&
        (acc_.GetOpCode(nextValue) == OpCode::VALUE_SELECTOR &&
        acc_.GetState(nextValue) == mergedContext->currentState_)) {
        ASSERT(IsGateNotEmpty(currentValue));
        if (isLoopBack) {
            ASSERT(IsGateNotEmpty(mergedContext->loopBackState_));
            acc_.NewIn(nextValue, mergedContext->loopBackIndex_ + 1, currentValue); // 1: merge
        } else {
            ASSERT(IsGateNotEmpty(mergedContext->mergeState_));
            if (!IsGateNotEmpty(mergedContext->loopBackState_)) {
                acc_.NewIn(nextValue, mergedContext->mergeIndex_ + 1, currentValue);  // 1: merge
            } else {
                mergeValueSelector = acc_.GetIn(nextValue, 1); // 1: index of phi of merge
                acc_.NewIn(mergeValueSelector, mergedContext->mergeIndex_ + 1, currentValue); // 1: merge
            }
        }
        result = nextValue;
    } else if (skipMergeValues) {
        // simply assign the value of current.
    } else if (currentValue != nextValue) {
        bool needMergeValues = IsGateNotEmpty(mergedContext->mergeState_);
        // build value selector for merge.
        if (needMergeValues) {
            size_t numOfIns = acc_.GetNumIns(mergedContext->mergeState_);
            auto inList = std::vector<GateRef>(numOfIns + 1, Circuit::NullGate());  // 1: state
            auto phi = circuit_->NewGate(
                circuit_->ValueSelector(numOfIns), MachineType::I64, inList.size(), inList.data(),
                GateType::AnyType());
            acc_.NewIn(phi, 0, mergedContext->mergeState_);
            for (size_t i = 0; i < mergedContext->mergeIndex_; i++) {
                acc_.NewIn(phi, i + 1, nextValue);  // 1: merge
            }
            if (!isLoopBack) {
                acc_.NewIn(phi, mergedContext->mergeIndex_ + 1, currentValue); // 1: merge
            }
            mergeValueSelector = result = phi;
        }
        // build value selector for loop begin.
        // If the register value of the corresponding bit is changed in the loop or
        // there is a state that needs to be merged (e.g.Multiple values or branches need to be merged into a phi node)
        // need to create or update the phi node.
        if (IsGateNotEmpty(mergedContext->loopBackState_) && (changedInLoop || needMergeValues)) {
            size_t numOfIns = acc_.GetNumIns(mergedContext->loopBackState_);
            auto inList = std::vector<GateRef>(numOfIns + 1, Circuit::NullGate());  // 1: state
            auto phi = circuit_->NewGate(
                circuit_->ValueSelector(numOfIns), MachineType::I64, inList.size(), inList.data(),
                GateType::AnyType());
            acc_.NewIn(phi, 0, mergedContext->loopBackState_);
            if (IsGateNotEmpty(mergedContext->mergeState_)) {
                acc_.NewIn(phi, 1, mergeValueSelector); // 1: merge
            }
            if (IsGateNotEmpty(nextValue)) {
                for (size_t i = 1; i < mergedContext->loopBackIndex_; i++) {
                    acc_.NewIn(phi, i + 1, nextValue); // 1: merge
                }
            }
            if (isLoopBack || !IsGateNotEmpty(mergedContext->mergeState_)) {
                acc_.NewIn(phi, mergedContext->loopBackIndex_ + 1, currentValue); // 1: merge
            }
            result = phi;
        }
    }
    return result;
}

MergeStateDependInfo FrameStateBuilder::GetCorrespondingState(const BytecodeRegion &bb, const BytecodeRegion &bbNext)
{
    auto mergedContext = GetMergedBbContext(bbNext.id);
    if (IsLoopHead(bbNext)) {
        if (!IfLoopNeedMerge(bbNext)) { // 1: only one loop in
            ASSERT(!IsGateNotEmpty(mergedContext->mergeState_));
            ASSERT(!IsGateNotEmpty(mergedContext->mergeDepend_));
            return {mergedContext->loopBackState_, mergedContext->loopBackDepend_, mergedContext->loopBackIndex_};
        }
        if (bbNext.IsLoopBack(bb)) {
            return {mergedContext->loopBackState_, mergedContext->loopBackDepend_, mergedContext->loopBackIndex_};
        } else {
            return {mergedContext->mergeState_, mergedContext->mergeDepend_, mergedContext->mergeIndex_};
        }
    } else {
        ASSERT(!IsGateNotEmpty(mergedContext->loopBackState_));
        ASSERT(!IsGateNotEmpty(mergedContext->loopBackDepend_));
        return {mergedContext->mergeState_, mergedContext->mergeDepend_, mergedContext->mergeIndex_};
    }
}

void FrameStateBuilder::MergeAssignment(const BytecodeRegion &bb, const BytecodeRegion &bbNext)
{
    auto mergedContext = GetMergedBbContext(bbNext.id);
    [[maybe_unused]] auto mergeInfo = GetCorrespondingState(bb, bbNext);
    ASSERT(acc_.IsCFGMerge(mergeInfo.state));
    auto liveout = GetFrameLiveoutBefore(bbNext.id);
    auto *loopAssignment = GetLoopAssignment(bbNext);
    for (size_t i = 0; i < numVregs_; i++) {
        if (liveout->TestBit(i)) {
            auto current = liveContext_->ValuesAt(i);
            auto next = mergedContext->ValuesAt(i);
            GateRef value = Circuit::NullGate();
#ifndef NDEBUG
            if (loopAssignment == nullptr) {
                ASSERT(IsGateNotEmpty(current) && IsGateNotEmpty(next));
            } else if (loopAssignment->TestBit(i)) {
                // next is null or phi
                ASSERT(!IsGateNotEmpty(next) ||
                    ((acc_.GetOpCode(next) == OpCode::VALUE_SELECTOR) &&
                    acc_.GetState(next) == mergedContext->currentState_));
            } else if (!IsGateNotEmpty(mergedContext->mergeState_)) {
                ASSERT(!IsGateNotEmpty(next) || current == next);
            }
#endif
            bool changedInLoop = loopAssignment != nullptr && loopAssignment->TestBit(i);
            value = MergeValue(bbNext, current, next, bbNext.IsLoopBack(bb), changedInLoop);
            mergedContext->SetValuesAt(i, value);
        }
    }
}

void FrameStateBuilder::CopyLiveoutValues(const BytecodeRegion &bbNext,
    FrameContext* dest, FrameContext* src)
{
    auto liveout = GetFrameLiveoutBefore(bbNext.id);
    for (size_t i = 0; i < numVregs_; i++) {
        if (liveout->TestBit(i)) {
            auto value = src->ValuesAt(i);
            dest->SetValuesAt(i, value);
        } else {
            dest->SetValuesAt(i, Circuit::NullGate());
        }
    }
}

FrameContext *FrameStateBuilder::GetCachedContext()
{
    // lazy init cachedContext
    if (cachedContext_ == nullptr) {
        auto chunk = circuit_->chunk();
        cachedContext_ = chunk->New<FrameContext>(chunk, numVregs_);
    }
    auto result = cachedContext_;
    if (cachedContext_ == liveContext_) {
        if (cachedContextBackup_ == nullptr) {
            auto chunk = circuit_->chunk();
            cachedContextBackup_ = chunk->New<FrameContext>(chunk, numVregs_);
        }
        result = cachedContextBackup_;
    }
    return result;
}

void FrameStateBuilder::SaveCurrentContext(const BytecodeRegion &bb)
{
    auto newContext = GetCachedContext();
    ASSERT(newContext != liveContext_);
    newContext->CopyCurrentStatus(liveContext_);
    CopyLiveoutValues(bb, newContext, liveContext_);
    liveContext_ = newContext;
}

void FrameStateBuilder::NewLoopExit(const BytecodeRegion &bbNext, BitSet *loopAssignment)
{
    auto state = liveContext_->currentState_;
    auto depend = liveContext_->currentDepend_;
    auto loopExit = circuit_->NewGate(circuit_->LoopExit(), { state });
    auto loopExitDepend = circuit_->NewGate(circuit_->LoopExitDepend(),
        { loopExit, depend });
    auto liveout = GetFrameLiveoutBefore(bbNext.id);
    for (size_t i = 0; i < numVregs_; i++) {
        if (liveout->TestBit(i)) {
            auto current = liveContext_->ValuesAt(i);
            if (loopAssignment->TestBit(i)) {
                current = circuit_->NewGate(circuit_->LoopExitValue(), acc_.GetMachineType(current),
                    {loopExit, current}, acc_.GetGateType(current));
            }
            liveContext_->SetValuesAt(i, current);
        } else {
            ASSERT(!IsGateNotEmpty(liveContext_->ValuesAt(i)));
        }
    }
    liveContext_->currentState_ = loopExit;
    liveContext_->currentDepend_ = loopExitDepend;
    if (!bcBuilder_->IsTypeLoweringEnabled()) {
        return;
    }
    auto stateSplit = BuildStateSplit(liveContext_, liveout, bbNext.start);
    liveContext_->currentDepend_ = stateSplit;
}

void FrameStateBuilder::TryInsertLoopExit(const BytecodeRegion &bb, const BytecodeRegion &bbNext)
{
    if (!bcBuilder_->EnableLoopOptimization() && !bcBuilder_->IsOSR()) {
        return;
    }
    if (bcBuilder_->IsOSR() && bcBuilder_->IsCacheBBOfOSRLoop(bbNext)) {
        return;
    }
    auto currentLoop = GetLoopInfoByLoopBody(bb);
    if (currentLoop != nullptr && !currentLoop->loopBodys->TestBit(bbNext.id)) {
        // use bbNext as merged values
        SaveCurrentContext(bbNext);
    }
    while (currentLoop != nullptr && !currentLoop->loopBodys->TestBit(bbNext.id)) {
        ASSERT(currentLoop->loopExits != nullptr);
#ifndef NDEBUG
        bool found = false;
        for (auto current : *currentLoop->loopExits) {
            if (current->id == bbNext.id) {
                found = true;
                break;
            }
        }
        ASSERT(found);
#endif
        NewLoopExit(bbNext, currentLoop->loopAssignment);
        currentLoop = currentLoop->parentInfo;
    }
}

void FrameStateBuilder::AdvanceToNextBB(const BytecodeRegion &bb, bool isOsrLoopExit)
{
    liveContext_ = GetMergedBbContext(bb.id);
    ASSERT(liveContext_ != nullptr);
    if (bb.loopNumber > 0) {
        // use bb as merged values
        SaveCurrentContext(bb);
    } else if (!isOsrLoopExit) {
        // all input merged
        ASSERT(liveContext_->currentIndex_ == bb.numOfStatePreds);
    }
    if (liveContext_->needStateSplit_) {
        liveContext_->needStateSplit_ = false;
        if (!bcBuilder_->IsTypeLoweringEnabled()) {
            return;
        }
        auto liveout = GetOrOCreateBBLiveOut(bb.id);
        auto stateSplit = BuildStateSplit(liveContext_, liveout, bb.start);
        liveContext_->currentDepend_ = stateSplit;
    }
}

class SubContextScope {
public:
    explicit SubContextScope(FrameStateBuilder* frameBuilder)
        : frameBuilder_(frameBuilder)
    {
        originContext_ = frameBuilder->liveContext_;
    }

    ~SubContextScope()
    {
        frameBuilder_->liveContext_ = originContext_;
    }
private:
    FrameContext* originContext_ {nullptr};
    FrameStateBuilder* frameBuilder_ {nullptr};
};

void FrameStateBuilder::MergeIntoSuccessor(const BytecodeRegion &bb, const BytecodeRegion &bbNext)
{
    [[maybe_unused]] SubContextScope scope(this);
    TryInsertLoopExit(bb, bbNext);
    auto mergedContext = GetOrOCreateMergedContext(bbNext.id);
    MergeStateDepend(bb, bbNext);
    if (mergedContext->currentIndex_ == 0) {
        if (bbNext.loopNumber > 0) {
            MergeAssignment(bb, bbNext);
        } else {
            CopyLiveoutValues(bbNext, mergedContext, liveContext_);
        }
    } else {
        MergeAssignment(bb, bbNext);
    }
    // We should connect this gate to loop begin although it's not a loop back edge
    // if there is no merge state, which means it's the sole loop in.
    if (bbNext.IsLoopBack(bb) || !IsGateNotEmpty(mergedContext->mergeState_)) {
        mergedContext->loopBackIndex_++;
    } else {
        mergedContext->mergeIndex_++;
    }
    mergedContext->currentIndex_++;
}

bool FrameStateBuilder::IsLoopBackEdge(const BytecodeRegion &bb, const BytecodeRegion &bbNext)
{
    if (bcBuilder_->IsOSR() && OutOfOsrLoop(bbNext)) {
        return false;
    } else if (bbNext.loopNumber > 0) {
        auto& loopInfo = GetLoopInfo(bbNext);
        return loopInfo.loopBodys->TestBit(bb.id);
    }
    return false;
}

FrameStateBuilder::LoopInfo& FrameStateBuilder::GetLoopInfo(const BytecodeRegion &bb)
{
    ASSERT(bb.loopNumber > 0);
    return loops_[bb.loopNumber - 1]; // -1: for index
}

FrameStateBuilder::LoopInfo& FrameStateBuilder::GetLoopInfo(BytecodeRegion &bb)
{
    ASSERT(bb.loopNumber > 0);
    return loops_[bb.loopNumber - 1]; // -1: for index
}

FrameStateBuilder::LoopInfo* FrameStateBuilder::GetLoopInfoByLoopBody(const BytecodeRegion &bb)
{
    if (bb.loopIndex == 0) {
        return nullptr;
    }
    auto& loopInfo = loops_[bb.loopIndex - 1];
    ASSERT(loopInfo.loopBodys->TestBit(bb.id));
    return &loopInfo;
}

BitSet *FrameStateBuilder::GetLoopAssignment(const BytecodeRegion &bb)
{
    if (bb.loopNumber > 0) {
        auto& loopInfo = GetLoopInfo(bb);
        return loopInfo.loopAssignment;
    }
    return nullptr;
}

void FrameStateBuilder::AddEmptyBlock(BytecodeRegion* bb)
{
    bbBeginStateLiveouts_.emplace_back(nullptr);
    bbFrameContext_.emplace_back(nullptr);
    auto liveout = GetOrOCreateBBLiveOut(bb->id);
    liveout->CopyFrom(liveOutResult_);
    GetOrOCreateMergedContext(bb->id);
    bcBuilder_->AddBasicBlock(bb);
}

class BlockLoopAnalysis {
public:
    explicit BlockLoopAnalysis(FrameStateBuilder *builder, Chunk* chunk)
        : frameBuilder_(builder), bcBuilder_(builder->bcBuilder_),
        pendingList_(chunk), loopbacks_(chunk),
        dfsStack_(chunk), visitState_(chunk), chunk_(chunk) {}

    void Run()
    {
        ComputeLoopBack();
        if (bcBuilder_->TerminateAnalysis()) {
            return;
        }
        TryClearDeadBlock();
        bcBuilder_->ComputeNumOfLoopBack();
        frameBuilder_->numLoops_ = numLoops_;
        if (numLoops_ > 0) {
            ComputeLoopInfo();
            if (bcBuilder_->IsLogEnabled()) {
                for (size_t i = 0; i < numLoops_; i++) {
                    auto& loopInfo = frameBuilder_->loops_[i];
                    PrintLoop(loopInfo);
                }
            }
        }
    }

    void CountLoopBackEdge(size_t fromId, size_t toId)
    {
        loopbacks_.push_back({fromId, toId});
        auto &toBlock = bcBuilder_->GetBasicBlockById(toId);
        toBlock.loopBacks.insert(fromId);
        if (toBlock.loopNumber == 0) {
            toBlock.loopNumber = ++numLoops_;
        }
    }

    void CheckDominance()
    {
        for (size_t i = 0; i < loopbacks_.size(); i++) {
            auto loopBackinfo = loopbacks_[i];
            bool isDom = bcBuilder_->IsAncestor(loopBackinfo.toId, loopBackinfo.fromId);
            if (!isDom) {
                bcBuilder_->SetIrreducibleLoop();
                LOG_COMPILER(INFO) << "CFG is not reducible. Compilation aborted.";
                break;
            }
        }
    }

    void ComputeLoopBack()
    {
        auto size = bcBuilder_->GetBasicBlockCount();
        visitState_.resize(size, MarkState::UNVISITED);
        size_t entryId = 0; // entry id
        VisitedInfo info = {0, false};
        visitedInfo_.resize(size, info);
        pendingList_.emplace_back(entryId);
        while (!pendingList_.empty()) {
            size_t bbId = pendingList_.back();
            auto &bb = bcBuilder_->GetBasicBlockById(bbId);
            bool allVisited = true;
            visitState_[bbId] = MarkState::PENDING;
            BytecodeRegion* catchBlock = bb.catches.empty() ? nullptr : bb.catches.at(0);
            for (size_t i = visitedInfo_[bbId].needVisitIndex; i < bb.succs.size(); i++) {
                BytecodeRegion* succBlock = bb.succs[i];
                size_t succId = succBlock->id;
                visitedInfo_[bbId].needVisitIndex = i + 1;
                if (visitState_[succId] == MarkState::UNVISITED) {
                    pendingList_.emplace_back(succId);
                    visitState_[succId] = MarkState::ON_STACK;
                    allVisited = false;
                    break;
                } else if (visitState_[succId] == MarkState::PENDING) {
                    // back edge
                    CountLoopBackEdge(bbId, succId);
                }
            }
            // we consider catchBlock as a special succ, which means if we have visited a succ,
            // visiting catchBlock is not allowed due to the rule of RPO.
            if (allVisited && catchBlock != nullptr && !visitedInfo_[bbId].isVisitedCatchBlock) {
                size_t catchId = catchBlock->id;
                visitedInfo_[bbId].isVisitedCatchBlock = true;
                if (visitState_[catchId] == MarkState::UNVISITED) {
                    pendingList_.emplace_back(catchId);
                    visitState_[catchId] = MarkState::ON_STACK;
                    allVisited = false;
                } else if (visitState_[catchId] == MarkState::PENDING) {
                    // back edge
                    CountLoopBackEdge(bbId, catchId);
                }
            }
            if (allVisited) {
                visitState_[bbId] = MarkState::VISITED;
                pendingList_.pop_back();
                frameBuilder_->rpoList_.push_front(bbId);
            }
        }

        if (bcBuilder_->NeedIrreducibleLoopCheck()) {
            CheckDominance();
        }
    }

    void TryClearDeadBlock()
    {
        if (frameBuilder_->rpoList_.size() == bcBuilder_->NumberOfLiveBlock()) {
            return;
        }
        auto size = bcBuilder_->GetBasicBlockCount();
        for (size_t i = 0; i < size; i++) {
            auto &bb = bcBuilder_->GetBasicBlockById(i);
            if (bb.numOfStatePreds != 0 && visitState_[i] == MarkState::UNVISITED) {
                bb.numOfStatePreds = 0;
            }
        }
        bcBuilder_->RemoveUnreachableRegion();
    }

    void CountLoopBody(FrameStateBuilder::LoopInfo& loopInfo, size_t bbId)
    {
        if (bbId != loopInfo.loopHeadId && !loopInfo.loopBodys->TestBit(bbId)) {
            loopInfo.loopBodys->SetBit(bbId);
            pendingList_.emplace_back(bbId);
            auto liveout = frameBuilder_->GetOrOCreateBBLiveOut(bbId);
            ASSERT(liveout != nullptr);
            loopInfo.loopAssignment->Union(liveout->defRegisters_);
        }
    }

    void PropagateLoopBody(FrameStateBuilder::LoopInfo& loopInfo)
    {
        while (!pendingList_.empty()) {
            auto cur = pendingList_.back();
            auto &curBlock = bcBuilder_->GetBasicBlockById(cur);
            pendingList_.pop_back();
            for (auto pred : curBlock.preds) {
                CountLoopBody(loopInfo, pred->id);
            }
            for (auto pred : curBlock.trys) {
                CountLoopBody(loopInfo, pred->id);
            }
        }
    }

    void InitLoopInfo(FrameStateBuilder::LoopInfo& loopInfo, BytecodeRegion& loopHeader, size_t backId)
    {
        if (loopInfo.loopHeadId == 0) {
            auto size = bcBuilder_->GetBasicBlockCount();
            loopInfo.loopHeadId = loopHeader.id;
            loopInfo.loopIndex = loopHeader.loopNumber;
            loopInfo.loopBodys = chunk_->New<BitSet>(chunk_, size);
            loopInfo.loopAssignment = chunk_->New<BitSet>(chunk_, frameBuilder_->numVregs_);
            loopHeader.loopIndex = loopInfo.loopIndex;
            loopInfo.loopBodys->SetBit(loopInfo.loopHeadId);
            auto liveout = frameBuilder_->GetOrOCreateBBLiveOut(loopInfo.loopHeadId);
            loopInfo.loopAssignment->Union(liveout->defRegisters_);
            loopInfo.numLoopBacks = 1;
            loopInfo.loopBodys->SetBit(backId);
        } else {
            if (!loopInfo.loopBodys->TestBit(backId)) {
                loopInfo.loopBodys->SetBit(backId);
            }
            loopInfo.numLoopBacks++;
        }
    }

    void ComputeLoopInfo()
    {
        frameBuilder_->loops_.resize(numLoops_, FrameStateBuilder::LoopInfo());
        for (auto& info : loopbacks_) {
            auto& toBlock = bcBuilder_->GetBasicBlockById(info.toId);
            auto& loopInfo = frameBuilder_->GetLoopInfo(toBlock);
            InitLoopInfo(loopInfo, toBlock, info.fromId);
        }
        TryMergeLoopEntry();
        ResizeLoopBody(); // tryMerge will insert region, need resize loop body.
        for (auto& info : loopbacks_) {
            auto& toBlock = bcBuilder_->GetBasicBlockById(info.toId);
            auto& loopInfo = frameBuilder_->GetLoopInfo(toBlock);
            CountLoopBody(loopInfo, info.fromId);
            PropagateLoopBody(loopInfo);
        }

        if (!bcBuilder_->EnableLoopOptimization() && !bcBuilder_->IsOSR()) {
            return;
        }

        auto size = bcBuilder_->GetBasicBlockCount();
        dfsStack_.resize(size, DFSState(0, 0));
        ComputeLoopTree();
    }

    void InsertEmptyBytecodeRegion(FrameStateBuilder::LoopInfo& loopInfo,
        BytecodeRegion& loopHeader, size_t numOfEntries)
    {
        auto size = bcBuilder_->GetBasicBlockCount();
        auto block = chunk_->New<BytecodeRegion>(chunk_);
        block->id = size;
        block->numOfStatePreds = numOfEntries;
        block->start = loopHeader.start;
        ASSERT(loopHeader.start != 0);
        block->end = BytecodeIterator::INVALID_INDEX;
        block->bytecodeIterator_.Reset(bcBuilder_, block->start, block->end);

        frameBuilder_->liveOutResult_->Reset();
        for (auto it = loopHeader.preds.begin(); it != loopHeader.preds.end();) {
            auto bbPred = *it;
            // not loop back
            if (!loopInfo.loopBodys->TestBit(bbPred->id)) {
                it = loopHeader.preds.erase(it);
                std::replace(bbPred->succs.begin(), bbPred->succs.end(), &loopHeader, block);
                block->preds.emplace_back(bbPred);
            } else {
                it++;
            }
        }
        frameBuilder_->MergeFromSuccBB(loopHeader.id);
        block->succs.emplace_back(&loopHeader);
        loopHeader.preds.insert(loopHeader.preds.begin(), block);
        frameBuilder_->AddEmptyBlock(block);

        ASSERT(loopHeader.trys.empty() && numOfEntries > 0);
        loopHeader.numOfStatePreds -= (numOfEntries - 1); // 1: one entry
        auto it = std::find(frameBuilder_->rpoList_.begin(), frameBuilder_->rpoList_.end(), loopHeader.id);
        ASSERT(it != frameBuilder_->rpoList_.end());
        frameBuilder_->rpoList_.insert(it, block->id);
        visitState_.emplace_back(MarkState::UNVISITED1);
    }

    void TryMergeLoopEntry()
    {
        for (size_t i = 0; i < numLoops_; i++) {
            auto& loopInfo = frameBuilder_->loops_[i];
            auto& loopHeader = bcBuilder_->GetBasicBlockById(loopInfo.loopHeadId);
            ASSERT(loopHeader.numOfStatePreds > loopInfo.numLoopBacks);
            size_t numOfEntries = static_cast<size_t>(loopHeader.numOfStatePreds - loopInfo.numLoopBacks);
            if (numOfEntries > 1 && loopHeader.trys.size() == 0) {
                InsertEmptyBytecodeRegion(loopInfo, loopHeader, numOfEntries);
            }
            // clear loopback bits for visit body
            loopInfo.loopBodys->Reset();
            loopInfo.loopBodys->SetBit(loopInfo.loopHeadId);
        }
    }

    void ResizeLoopBody()
    {
        for (auto& info : loopbacks_) {
            auto size = bcBuilder_->GetBasicBlockCount();
            auto& toBlock = bcBuilder_->GetBasicBlockById(info.toId);
            auto& loopInfo = frameBuilder_->GetLoopInfo(toBlock);
            if (loopInfo.loopBodys->ShouldExpand(size)) {
                auto tmp = loopInfo.loopBodys;
                loopInfo.loopBodys = chunk_->New<BitSet>(chunk_, size);
                loopInfo.loopBodys->CopyDataFrom(*tmp);
            }
        }
    }

    FrameStateBuilder::LoopInfo* EnterInnerLoop(FrameStateBuilder::LoopInfo* loopInfo, size_t bbId)
    {
        auto &bb = bcBuilder_->GetBasicBlockById(bbId);
        if (bb.loopNumber > 0) {
            auto &innerInfo = frameBuilder_->GetLoopInfo(bb);
            ASSERT(innerInfo.parentInfo == nullptr);
            innerInfo.parentInfo = loopInfo;
            innerInfo.sortIndx = frameBuilder_->sortIndx_++;
            loopInfo = &innerInfo;
        } else if (loopInfo != nullptr) {
            bb.loopIndex = loopInfo->loopIndex;
        }
        return loopInfo;
    }

    void ComputeLoopTree()
    {
        FrameStateBuilder::LoopInfo* loopInfo = nullptr;
        auto currentDepth = Push(0, 0); // entry id
        while (currentDepth > 0) {
            auto &curState = dfsStack_[currentDepth - 1]; // -1: for current
            auto const &bb = bcBuilder_->GetBasicBlockById(curState.bbId);
            if (!bcBuilder_->IsOSR()) {
                ASSERT(bb.catches.empty());
            }
            auto index = curState.index;
            BytecodeRegion* bbNext = nullptr;
            if (index >= bb.succs.size()) {
                if (bb.loopNumber > 0) {
                    if (visitState_[curState.bbId] == MarkState::ON_STACK) {
                        if (loopInfo == nullptr) {
                            continue;
                        }
                        ASSERT(loopInfo->loopHeadId == curState.bbId);
                        loopInfo = loopInfo->parentInfo;
                        visitState_[curState.bbId] = MarkState::VISITED1;
                    }
                    bbNext = PushLoopExist(bb, currentDepth);
                }
            } else {
                bbNext = bb.succs[curState.index++]; // 1: goto next
            }
            if (bbNext != nullptr) {
                if (loopInfo != nullptr && !loopInfo->loopBodys->TestBit(bbNext->id)) {
                    AddLoopExit(bbNext, loopInfo);
                } else if (visitState_[bbNext->id] == MarkState::UNVISITED1) {
                    currentDepth = Push(bbNext->id, currentDepth);
                    loopInfo = EnterInnerLoop(loopInfo, bbNext->id);
                }
            } else {
                if (bb.loopNumber == 0) {
                    visitState_[curState.bbId] = MarkState::VISITED1;
                }
                currentDepth--;
            }
        }
    }

    size_t Push(size_t bbId, size_t depth)
    {
        if (visitState_[bbId] == MarkState::UNVISITED1) {
            dfsStack_[depth].bbId = bbId;
            dfsStack_[depth].index = 0;
            visitState_[bbId] = MarkState::ON_STACK;
            return depth + 1;
        }
        return depth;
    }

    BytecodeRegion* PushLoopExist(const BytecodeRegion& bb, size_t depth)
    {
        ASSERT(depth > 0);
        auto &curState = dfsStack_[depth - 1]; // -1: for current
        auto loopExitIndex = curState.index - bb.succs.size();
        auto& currentInfo = frameBuilder_->GetLoopInfo(bb);
        BytecodeRegion* bbNext = nullptr;
        if (currentInfo.loopExits != nullptr && loopExitIndex < currentInfo.loopExits->size()) {
            bbNext = currentInfo.loopExits->at(loopExitIndex);
            curState.index++; // 1: goto next
        }
        return bbNext;
    }

    void AddLoopExit(BytecodeRegion *bb, FrameStateBuilder::LoopInfo *loopInfo)
    {
        if (loopInfo->loopExits == nullptr) {
            loopInfo->loopExits = chunk_->New<ChunkVector<BytecodeRegion*>>(chunk_);
        }
        loopInfo->loopExits->emplace_back(bb);
    }

    void PrintLoop(FrameStateBuilder::LoopInfo& loopInfo)
    {
        auto size = bcBuilder_->GetBasicBlockCount();
        LOG_COMPILER(INFO) << "--------------------------------- LoopInfo Start ---------------------------------";
        LOG_COMPILER(INFO) << "LoopHead: " << loopInfo.loopHeadId;
        if (loopInfo.parentInfo != nullptr) {
            LOG_COMPILER(INFO) << "ParentLoopHead: " << loopInfo.parentInfo->loopHeadId;
        }
        std::string log = "Body: [";
        for (size_t i = 0; i < size; i++) {
            if (loopInfo.loopBodys->TestBit(i)) {
                log += std::to_string(i) + ", ";
            }
        }
        LOG_COMPILER(INFO) << log << "]";
        std::string log1 = "Exit: [";
        if (loopInfo.loopExits != nullptr) {
            for (auto bb : *loopInfo.loopExits) {
                log1 += std::to_string(bb->id) + ", ";
            }
        }
        LOG_COMPILER(INFO) << log1 << "]";
        std::string log2 = "LoopAssignment [";
        bool firset = true;
        for (size_t i = 0; i < frameBuilder_->numVregs_; i++) {
            if (loopInfo.loopAssignment->TestBit(i)) {
                if (!firset) {
                    log2 += ",";
                }
                firset = false;
                log2 += std::to_string(i);
            }
        }
        LOG_COMPILER(INFO) << log2 << "]";
        LOG_COMPILER(INFO) << "--------------------------------- LoopInfo End ---------------------------------";
    }

private:
    struct EndToHead {
        size_t fromId;
        size_t toId;
    };
    struct DFSState {
        DFSState(size_t bbId, size_t index)
            : bbId(bbId), index(index) {}

        size_t bbId;
        size_t index;
    };
    struct VisitedInfo {
        size_t needVisitIndex;
        bool isVisitedCatchBlock = false;
    };
    enum class MarkState : uint8_t {
        UNVISITED = 0,
        ON_STACK,
        PENDING,
        VISITED,
        VISITED1,
        UNVISITED1 = VISITED
    };
    FrameStateBuilder* frameBuilder_ {nullptr};
    BytecodeCircuitBuilder *bcBuilder_ {nullptr};
    ChunkDeque<size_t> pendingList_;
    ChunkVector<EndToHead> loopbacks_;
    ChunkVector<DFSState> dfsStack_;
    ChunkVector<MarkState> visitState_;
    Chunk* chunk_ {nullptr};
    size_t numLoops_ {0};
    std::vector<VisitedInfo> visitedInfo_;
};

void FrameStateBuilder::ComputeLoopInfo()
{
    BlockLoopAnalysis loopAnalysis(this, circuit_->chunk());
    loopAnalysis.Run();
    if (numLoops_ != 0 && !bcBuilder_->HasIrreducibleLoop()) {
        ChunkVector<GateRef>& headerGates = bcBuilder_->GetLoopHeaderGates();
        headerGates.resize(numLoops_, Circuit::NullGate());
    }
}

void FrameStateBuilder::DumpLiveState()
{
    LOG_COMPILER(INFO) << "DumpLiveState";
    for (size_t i = 0; i < bcEndStateLiveouts_.size(); i++) {
        auto liveout = GetFrameLiveoutAfter(i);
        if (liveout == nullptr) {
            continue;
        }
        std::string log = "BC: " + std::to_string(i) + " {";
        bool firset = true;
        for (size_t j = 0; j < numVregs_; j++) {
            if (liveout->TestBit(j)) {
                if (!firset) {
                    log += ",";
                }
                firset = false;
                log += std::to_string(j);
            }
        }
        log += "}";
        LOG_COMPILER(INFO) << log;
    }
    for (size_t i = 1; i < bbBeginStateLiveouts_.size(); i++) { // 1: skip entry
        auto liveout = GetFrameLiveoutBefore(i);
        if (liveout == nullptr) {
            continue;
        }
        std::string log = "BB: " + std::to_string(i) + " {";
        bool firset = true;
        for (size_t j = 0; j < numVregs_; j++) {
            if (liveout->TestBit(j)) {
                if (!firset) {
                    log += ",";
                }
                firset = false;
                log += std::to_string(j);
            }
        }
        log += "}";
        LOG_COMPILER(INFO) << log;
    }
}

GateRef FrameStateBuilder::BuildFrameState(FrameContext* frameContext, FrameLiveOut* liveout, size_t bcIndex)
{
    auto pcOffset = bcBuilder_->GetPcOffset(bcIndex);
    GateRef gateValues = BuildFrameValues(frameContext, liveout);

    GateRef frameArgs = bcBuilder_->GetFrameArgs();
    GateRef preFrameState = bcBuilder_->GetPreFrameState();
    UInt32PairAccessor accessor(static_cast<uint32_t>(pcOffset),
        FrameStateOutput::Invalid().GetValue());
    auto frameState = circuit_->NewGate(circuit_->FrameState(accessor.ToValue()),
        {frameArgs, gateValues, preFrameState});
    return frameState;
}

GateRef FrameStateBuilder::BuildStateSplit(FrameContext* frameContext, FrameLiveOut* liveout, size_t bcIndex)
{
    auto frameState = BuildFrameState(frameContext, liveout, bcIndex);
    auto state = frameContext->currentState_;
    auto depend = frameContext->currentDepend_;
    ASSERT(IsGateNotEmpty(state));
    ASSERT(IsGateNotEmpty(depend));
    return circuit_->NewGate(circuit_->StateSplit(), {state, depend, frameState});
}

void FrameStateBuilder::BindStateSplitBefore(const BytecodeInfo &bytecodeInfo, FrameLiveOut* liveout, uint32_t bcId)
{
    if (!bcBuilder_->IsTypeLoweringEnabled()) {
        return;
    }
    if (bytecodeInfo.IsCall() || bytecodeInfo.IsAccessorBC()) {
        frameStateCache_ = BuildFrameState(liveContext_, liveout, bcId);
    }
    ASSERT(!liveContext_->needStateSplit_);
}

void FrameStateBuilder::BindStateSplitAfter(const BytecodeInfo &bytecodeInfo,
    uint32_t bcId, GateRef gate)
{
    if (!bcBuilder_->IsTypeLoweringEnabled()) {
        return;
    }
    if (bytecodeInfo.IsCall() || bytecodeInfo.IsAccessorBC()) {
        auto frameState = GetBcFrameStateCache();
        acc_.ReplaceFrameStateIn(gate, frameState);
    }
    if (!bytecodeInfo.NoSideEffects() && !bytecodeInfo.IsThrow()) {
        auto stateSplit = BuildStateSplit(liveContext_, GetOrOCreateBCEndLiveOut(bcId), bcId + 1); // 1: for after
        liveContext_->currentDepend_ = stateSplit;
    }
}

GateRef FrameStateBuilder::BuildFrameValues(FrameContext* frameContext, FrameLiveOut* liveout)
{
    size_t frameStateInputs = numVregs_;
    std::vector<GateRef> inList(frameStateInputs, Circuit::NullGate());
    auto optimizedGate = circuit_->GetConstantGate(MachineType::I64,
                                                   JSTaggedValue::VALUE_OPTIMIZED_OUT,
                                                   GateType::TaggedValue());
    for (size_t i = 0; i < numVregs_; i++) {
        auto value = frameContext->ValuesAt(i);
        if (!IsGateNotEmpty(value) || !liveout->TestBit(i)) {
            value = optimizedGate;
        }
        inList[i] = value;
    }
    return circuit_->NewGate(circuit_->FrameValues(frameStateInputs), inList);
}
}
