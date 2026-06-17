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

#include "ecmascript/arksteed/arksteed_graph_builder_new.h"
#include "ecmascript/arksteed/arksteed_framestate.h"
#include "ecmascript/js_function.h"
#include "ecmascript/lexical_env.h"

#define REGISTER_VERTEX_TO_LABELLER(vertex)                             \
    do {                                                                \
        ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();    \
        if (labeller != nullptr) {                                      \
            labeller->RegisterVertex(vertex);                           \
        }                                                               \
    } while (false)

namespace panda::ecmascript::arksteed {
namespace {
const char *ValueRepresentationName(ValueRepresentation repr)
{
    switch (repr) {
        case ValueRepresentation::TAGGED:
            return "tagged";
        case ValueRepresentation::INT32:
            return "int32";
        case ValueRepresentation::UINT32:
            return "uint32";
        case ValueRepresentation::FLOAT64:
            return "float64";
        case ValueRepresentation::HOLEY_FLOAT64:
            return "holey_float64";
        case ValueRepresentation::INT_PTR:
            return "intptr";
        case ValueRepresentation::NONE:
            return "none";
    }
    return "unknown";
}

bool IsTaggedCallType(kungfu::VariableType type)
{
    kungfu::GateType gateType = type.GetGateType();
    return gateType == kungfu::GateType::TaggedValue() || gateType == kungfu::GateType::TaggedPointer() ||
           gateType == kungfu::GateType::TaggedNPointer();
}

bool MatchesCallSignatureType(const ValueVertex *value, kungfu::VariableType type)
{
    if (IsTaggedCallType(type)) {
        return value->IsTagged();
    }

    switch (type.GetMachineType()) {
        case kungfu::MachineType::NOVALUE:
        case kungfu::MachineType::ANYVALUE:
        case kungfu::MachineType::FLEX:
            return true;
        case kungfu::MachineType::ARCH:
        case kungfu::MachineType::I64:
            return value->IsIntPtr();
        case kungfu::MachineType::I1:
        case kungfu::MachineType::I8:
        case kungfu::MachineType::I16:
        case kungfu::MachineType::I32:
            return value->IsInt32() || value->IsUint32() || value->IsIntPtr();
        case kungfu::MachineType::F32:
        case kungfu::MachineType::F64:
            return value->IsAnyFloat64();
    }
    return true;
}
}  // namespace

// Condensed storage: [vA, vA, vA, vB, vB, vB, vB, vB, vB, vC, vC, vC, vC]
//                 => [(vA, 3),    (vB, 6),                (vC, 4)]
struct GraphBuilderNew::CatchBlockInputData {
    struct InputEntry {
        ValueVertex *vertex;
        uint32_t count;
    };

    uint32_t totalCount;
    const kungfu::BitSet *liveIn;
    // inputs[i] = List of inputs for the i-th live-in virtual register
    ChunkVector<ChunkVector<InputEntry>> inputs;

    explicit CatchBlockInputData(const kungfu::BitSet &liveIn, Chunk *chunk)
        : totalCount(0), liveIn(&liveIn), inputs(chunk)
    {
        size_t numLive = liveIn.Count();
        inputs.reserve(numLive);
        for (size_t i = 0; i < numLive; i++) {
            inputs.emplace_back(chunk);
        }
    }

    // Returns the index of this catch predecessor
    uint32_t AddCatchPredecessor(const SharedBCFrameState frameState, ValueVertex *alt)
    {
        VRegIDType liveIndex = 0;
        // -1: Skips acc, which will be overwritten by the exception object
        for (VRegIDType i = 0, n = frameState.NumVRegs(); i < n - 1; i++) {
            if (!liveIn->TestBit(i)) {
                continue;
            }
            ValueVertex *value = frameState.Get(i);
            if (value == nullptr) {
                value = alt;
            }
            auto &curInputList = inputs[liveIndex++];
            if (curInputList.empty() || curInputList.back().vertex != value) {
                curInputList.push_back({.vertex = value, .count = 1});
            } else {
                curInputList.back().count += 1;
            }
        }
        uint32_t curInputIndex = totalCount;
        totalCount += 1;
        return curInputIndex;
    }
};

GraphBuilderNew::GraphBuilderNew(JSThread *compilerThread,
                                 Graph *destGraph,
                                 uintptr_t glueAddr,
                                 BytecodePreprocessorNew *preproc,
                                 BytecodeAnalysisNew *analysis)
    : graph_(destGraph),
      glueAddr_(glueAddr),
      preproc_(preproc),
      analysis_(analysis),
      pgoContext_(compilerThread, preproc->GetEnv()),
      numLocal_(preproc->GetNumLocalVRegs()),
      numParams_(preproc->GetNumParamVRegs()),
      chunk_(preproc->GetChunk()),
      blocks_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      frameStates_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      catchBlockInputs_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk())
{}

bool GraphBuilderNew::Run()
{
    ASSERT(preproc_->GetNumLiveBasicBlocks() > 0);
    DebugLog();

    SharedBCFrameState frameState(preproc_->GetNumVRegs(), nullptr, chunk_);
    InitializeStartBlock(frameState);
    frameStates_[0] = CondensedBCFrameState(frameState, analysis_->GetLiveOut(0), chunk_);

    // 1 : Skips the start block (which is initialized before)
    for (uint32_t i = 1, n = preproc_->GetNumLiveBasicBlocks(); i < n; i++) {
        if (blocks_[i] == nullptr) {
            ProcessDeadBasicBlock(i);
            continue;
        }
        frameState.Reset(nullptr);
        ProcessBasicBlock(frameState, i);
        frameStates_[i] = CondensedBCFrameState(frameState, analysis_->GetLiveOut(i), chunk_);
    }
    return true;
}

void GraphBuilderNew::DebugLog()
{
    if (!common::Log::LogIsLoggable(Level::DEBUG, Component::COMPILER)) {
        return;
    }
    LOG_COMPILER(DEBUG) << "arksteed::GraphBuilder: Starts graph building with "
                           "NumLocalVRegs = " << numLocal_ << ", NumParamVRegs = " << numParams_;

    std::string dumpStr = preproc_->Dump();
    std::istringstream preprocStream(dumpStr);
    std::string line;
    while (std::getline(preprocStream, line)) {
        LOG_COMPILER(DEBUG) << line;
    }

    dumpStr = analysis_->Dump();
    std::istringstream analysisStream(dumpStr);
    while (std::getline(analysisStream, line)) {
        LOG_COMPILER(DEBUG) << line;
    }
}

void GraphBuilderNew::InitializeStartBlock(SharedBCFrameState frameState)
{
    glue_ = graph_->GetIntPtrConstant(glueAddr_);
    undefinedValue_ = graph_->GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);

    blocks_[0] = BB::New(chunk_);
    // caller argument area (in fp-slot words): +2 argc, +3 call-target, +4 new-target, +5 this, +6... user args.
    const int32_t CALL_TARGET_FP_SLOT_INDEX = 3;
    for (uint32_t i = 0, n = numParams_; i < n; i++) {
        int32_t slotIndex = static_cast<int32_t>(i + CALL_TARGET_FP_SLOT_INDEX);
        auto *v = NewVertex<InitialValueVertex>(blocks_[0], {}, slotIndex);
        graph_->AddParameter(v);
        frameState.Set(VRegOfParam(numLocal_, i).GetId(), v);
    }
    // -3 : Fixed header lexicalEnv is at slot -3 in word units.
    initialLexicalEnv_ = NewVertex<InitialValueVertex>(blocks_[0], {}, -3);
    frameState.SetLexicalEnv(initialLexicalEnv_);

    FinishBlockWithJump(blocks_[0], ActivateNonCatchBlock(1));
}

void GraphBuilderNew::ProcessDeadBasicBlock(uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    if (!bcBlock->IsEndOfLoop()) {
        LOG_COMPILER(DEBUG) << "Skips block #" << rpoIndex << " which is dead.";
        return;
    }
    uint32_t headerRpoIndex = bcBlock->jumpBlock->rpoIndex;
    if (blocks_[headerRpoIndex] == nullptr) {
        LOG_COMPILER(DEBUG) << "Skips block #" << rpoIndex << " which is dead.";
        return;
    }
    // For the rare case where the loop-back block is dead (due to pattern like if (true) break),
    // a dummy basic block is created, which simply jumps the loop header.
    // Uses JumpLoopVertex so that LivenessProcessor can pop the loop from loopUsedVertices_.
    blocks_[rpoIndex] = BB::New(chunk_);
    FinishBlockWithJumpLoop(blocks_[rpoIndex], blocks_[headerRpoIndex]);

    for (PhiVertex *phi : blocks_[headerRpoIndex]->GetPhis()) {
        ASSERT(phi->GetInputCount() == 2);  // 2 : Two jumpPredecessors: one is entry, the other is loop-back
        phi->SetInput(1, undefinedValue_);
    }
}

void GraphBuilderNew::ProcessBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    if (bcBlock->IsCatchBlockHeader()) {
        ProcessCatchBlockHead(frameState, rpoIndex);
        return;
    }
    if (bcBlock->IsLoopHeader()) {
        blocks_[rpoIndex]->SetLoopHeader(true);
    }

    bcBlock->IsLoopHeader()
        ? InitFrameStateForLoopHeader(frameState, rpoIndex)
        : InitFrameState(frameState, rpoIndex);

    if (bcBlock->IsSynthetic()) {
        // Edge-split block, etc. No bytecode inside.
        BB *target = ActivateNonCatchBlock(bcBlock->jumpBlock->rpoIndex);
        if (bcBlock->IsEndOfLoop()) {
            FinishBlockWithJumpLoop(blocks_[rpoIndex], target);
        } else {
            FinishBlockWithJump(blocks_[rpoIndex], target);
        }
    } else {
        VisitBytecodesOfBasicBlock(frameState, rpoIndex);
    }
    if (bcBlock->IsEndOfLoop()) {
        WriteBackFrameStateToLoopHeader(frameState, rpoIndex);
    }
}

void GraphBuilderNew::ProcessCatchBlockHead(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    blocks_[rpoIndex]->SetExceptionHandler(true);
    InitFrameStateForCatchBlockHeader(frameState, rpoIndex);

    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    // Catch block header is always synthetic. Only an unconditional jump.
    ASSERT(bcBlock->IsJump());
    FinishBlockWithJump(blocks_[rpoIndex], ActivateNonCatchBlock(bcBlock->jumpBlock->rpoIndex));
}

void GraphBuilderNew::InitFrameState(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);

    uint32_t bcNumPreds = static_cast<uint32_t>(bcBlock->jumpPredecessors.size());
    uint32_t actualNumPreds = 0;
    for (uint32_t j = 0; j < bcNumPreds; j++) {
        uint32_t predRpoIndex = bcBlock->jumpPredecessors[j]->rpoIndex;
        if (blocks_[predRpoIndex] != nullptr) {
            actualNumPreds += 1;
        }
    }
    uint32_t actualPredIndex = 0;
    for (uint32_t j = 0; j < bcNumPreds; j++) {
        uint32_t predRpoIndex = bcBlock->jumpPredecessors[j]->rpoIndex;
        if (blocks_[predRpoIndex] != nullptr) {
            MergeFrameState(frameState, rpoIndex, predRpoIndex, actualPredIndex++, actualNumPreds);
        }
    }
}

void GraphBuilderNew::InitFrameStateForLoopHeader(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(blockInfo->jumpPredecessors.size() == 2);  // 2 : One is entry, the other is loop-back

    kungfu::BitSet phiCandidates(chunk_, frameState.NumVRegs());
    phiCandidates.CopyFrom(analysis_->GetLiveIn(rpoIndex));
    phiCandidates.Intersect(analysis_->GetKillSet(rpoIndex));

    for (uint32_t vregIndex = 0, n = frameState.NumVRegs(); vregIndex < n; vregIndex++) {
        if (phiCandidates.TestBit(vregIndex)) {
            // 2 : One is entry, the other is loop-back
            frameState.Set(vregIndex, NewPhiVertex(blocks_[rpoIndex], 2, vregIndex));
        }
    }
    uint32_t predRpoIndex = blockInfo->jumpPredecessors[0]->rpoIndex;
    ASSERT(blocks_[predRpoIndex] != nullptr);
    // 0 : The first predecessor which is loop entry; 2 : Two jumpPredecessors, one is entry, the other is loop-back
    MergeFrameState(frameState, rpoIndex, predRpoIndex, 0, 2);
}

void GraphBuilderNew::InitFrameStateForCatchBlockHeader(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    // For catch blocks, acc is always initialized as the exception object
    frameState.SetAcc(NewVertex<LoadExceptionVertex>(blocks_[rpoIndex], {glue_}));

    CatchBlockInputData *data = catchBlockInputs_[rpoIndex];
    ASSERT(data != nullptr);
    ASSERT(data->totalCount >= 1);

    const kungfu::BitSet &liveIn = analysis_->GetLiveIn(rpoIndex);
    VRegIDType liveIndex = 0;
    // -1: Skips acc, which is numbered as the last
    for (VRegIDType vregIndex = 0, n = frameState.NumVRegs(); vregIndex < n - 1; vregIndex++) {
        if (!liveIn.TestBit(vregIndex)) {
            continue;
        }
        const auto &curInputList = data->inputs[liveIndex++];
        ASSERT(!curInputList.empty());

        if (curInputList.size() == 1) {
            // Same inputs
            frameState.Set(vregIndex, curInputList[0].vertex);
            continue;
        }
        PhiVertex *phi = NewPhiVertex(blocks_[rpoIndex], data->totalCount, vregIndex);
        uint32_t inputIndex = 0;
        for (const auto &[input, count] : curInputList) {
            // Decompress input list
            for (uint32_t j = 0; j < count; j++) {
                phi->SetInput(inputIndex++, input);
            }
        }
        frameState.Set(vregIndex, phi);
    }
}

void GraphBuilderNew::WriteBackFrameStateToLoopHeader(SharedBCFrameState current, uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(blockInfo->IsJump());
    ASSERT(blockInfo->jumpBlock->jumpPredecessors.size() == 2);  // 2 : One is entry, the other is loop-back

    uint32_t headerIndex = blockInfo->jumpBlock->rpoIndex;
    for (PhiVertex *phi : blocks_[headerIndex]->GetPhis()) {
        ASSERT(phi->GetInputCount() == 2);  // 2 : One is entry, the other is loop-back
        ValueVertex *fromCurrent = current.Get(phi->GetOwner().GetId());
        phi->SetInput(1, fromCurrent != nullptr ? fromCurrent : undefinedValue_);
    }
}

void GraphBuilderNew::MergeFrameState(SharedBCFrameState dest, uint32_t rpoIndex, uint32_t predRpoIndex,
                                      uint32_t actualPredIndex, uint32_t actualNumPreds)
{
    const kungfu::BitSet &liveIn = analysis_->GetLiveIn(rpoIndex);
    frameStates_[predRpoIndex].ForEach([&, this](ValueVertex *fromPred, VRegIDType vregIndex) {
        if (!liveIn.TestBit(vregIndex)) {
            return;
        }
        ValueVertex *cur = dest.Get(vregIndex);
        if (cur == fromPred) {
            return;
        }
        if (cur == nullptr) {
            dest.Set(vregIndex, fromPred);
            return;
        }
        PhiVertex *phi = cur->TryCast<PhiVertex>();
        if (phi != nullptr && cur->GetOwner() == blocks_[rpoIndex]) {
            phi->SetInput(actualPredIndex, fromPred);
            return;
        }
        phi = NewPhiVertex(blocks_[rpoIndex], actualNumPreds, vregIndex);
        for (uint32_t k = 0; k < actualPredIndex; k++) {
            phi->SetInput(k, cur);
        }
        phi->SetInput(actualPredIndex, fromPred);
        dest.Set(vregIndex, phi);
    });
}

PhiVertex *GraphBuilderNew::NewPhiVertex(BB *owner, uint32_t numPredecessors, VRegIDType vreg)
{
    // nullptr: Old MergePointFrameState pointer. To be removed after refactoring done.
    PhiVertex *phi = PhiVertex::New(chunk_, numPredecessors, nullptr, VirtualRegister(vreg));
    phi->SetOwner(owner);
    owner->AddPhiVertex(phi);
    REGISTER_VERTEX_TO_LABELLER(phi);
    return phi;
}

template <class InputRange>
PhiVertex *GraphBuilderNew::NewPhiVertexWith(BB *owner, const InputRange &inputs, VRegIDType vreg)
{
    uint32_t numInputs = static_cast<uint32_t>(std::size(inputs));
    PhiVertex *phi = NewPhiVertex(owner, numInputs, vreg);

    auto iter = std::begin(inputs);
    for (uint32_t i = 0; i < numInputs; i++) {
        phi->SetInput(i, *iter);
        ++iter;
    }
    return phi;
}

template <class VertexT, class InputRange, class... Args>
VertexT *GraphBuilderNew::NewVertex(BB *owner, const InputRange &inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
    vertex->SetOwner(owner);
    owner->AddVertex(vertex);
    REGISTER_VERTEX_TO_LABELLER(vertex);

    constexpr VertexProperties props = VertexT::PROPERTIES;
    // At most one of: deopt_checkpoint, eager_deopt, lazy_deopt
    static_assert(props.IsDeoptCheckpoint() + props.CanEagerDeopt() + props.CanLazyDeopt() <= 1);

    return vertex;
}

JumpVertex *GraphBuilderNew::FinishBlockWithJump(BB *owner, BB *target)
{
    auto *jumpVertex = FinishBlockWith<JumpVertex>(owner, {}, target);
    jumpVertex->SetPredecessorId(target->PredecessorCount());
    target->AddPredecessor(owner);
    // TODO: For legacy code only. To be removed.
    target->SetPredecessorCount(target->PredecessorCount() + 1);
    return jumpVertex;
}

JumpLoopVertex *GraphBuilderNew::FinishBlockWithJumpLoop(BB *owner, BB *target)
{
    auto *jumpLoopVertex = FinishBlockWith<JumpLoopVertex>(owner, {}, target);
    jumpLoopVertex->SetPredecessorId(target->PredecessorCount());
    target->AddPredecessor(owner);
    // TODO: For legacy code only. To be removed.
    target->SetPredecessorCount(target->PredecessorCount() + 1);
    return jumpLoopVertex;
}

BranchIfTrueVertex *GraphBuilderNew::FinishBlockWithBranch(
    BB *owner, ValueVertex *input, BB *targetIfTrue, BB *targetIfFalse)
{
    auto *branchVertex = FinishBlockWith<BranchIfTrueVertex>(owner, {input}, targetIfTrue, targetIfFalse);
    targetIfTrue->AddPredecessor(owner);
    targetIfFalse->AddPredecessor(owner);
    // TODO: For legacy code only. To be removed.
    targetIfTrue->SetPredecessorCount(targetIfTrue->PredecessorCount() + 1);
    targetIfFalse->SetPredecessorCount(targetIfFalse->PredecessorCount() + 1);
    return branchVertex;
}

template <class VertexT, class... Args>
VertexT *GraphBuilderNew::FinishBlockWith(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
    vertex->SetOwner(owner);
    owner->SetControlVertex(vertex);
    graph_->Add(owner);
    REGISTER_VERTEX_TO_LABELLER(vertex);

    constexpr VertexProperties props = VertexT::PROPERTIES;
    // Control vertices cannot have lazy deopt, throw, or write side effects
    // Note: ThrowVertex is a special case that can throw
    static_assert(!props.CanLazyDeopt() && !props.CanWrite());

    return vertex;
}

BB *GraphBuilderNew::NewBlock()
{
    BB *result = BB::New(chunk_);
    // TODO: For legacy code only. To be removed.
    result->SetRegisterMergeState(chunk_->New<RegisterMergeState>());
    return result;
}

BB *GraphBuilderNew::ActivateNonCatchBlock(uint32_t rpoIndex)
{
    if (blocks_[rpoIndex] == nullptr) {
        blocks_[rpoIndex] = NewBlock();
    }
    return blocks_[rpoIndex];
}

BB *GraphBuilderNew::ActivateCatchBlock(GraphBuilderNew::CatchBlockInputData **inputData, uint32_t rpoIndex)
{
    if (UNLIKELY(blocks_[rpoIndex] == nullptr)) {
        blocks_[rpoIndex] = NewBlock();
        catchBlockInputs_[rpoIndex] = chunk_->New<CatchBlockInputData>(analysis_->GetLiveIn(rpoIndex), chunk_);
    }
    ASSERT(catchBlockInputs_[rpoIndex] != nullptr);
    *inputData = catchBlockInputs_[rpoIndex];
    return blocks_[rpoIndex];
}

LoadTaggedFieldVertex *GraphBuilderNew::ActivateGlobalEnv()
{
    if (UNLIKELY(lazyGlobalEnv_ == nullptr)) {
        int32_t globalEnvOffset = static_cast<int32_t>(GlobalEnv::HEADER_SIZE);
        lazyGlobalEnv_ = NewVertex<LoadTaggedFieldVertex>(blocks_[0], {initialLexicalEnv_}, globalEnvOffset);
    }
    return lazyGlobalEnv_;
}

constexpr uintptr_t NO_CATCH_BLOCK_TAG = 1;

struct GraphBuilderNew::BytecodeVisitor {
    void Visit(const BytecodeInfo *bcInfo)
    {
        switch (bcInfo->GetOpcode()) {
            case kungfu::EcmaOpcode::NOP:  // Nop: Nothing to do
                break;
            // -------- Category #1: Register Moves --------
            case kungfu::EcmaOpcode::MOV_V4_V4:
            case kungfu::EcmaOpcode::MOV_V8_V8:
            case kungfu::EcmaOpcode::MOV_V16_V16:
                frameState.Set(bcInfo->vregOut[0], LoadRegister(bcInfo, 0));
                break;
            case kungfu::EcmaOpcode::STA_V8:
                frameState.Set(bcInfo->vregOut[0], frameState.GetAcc());
                break;
            case kungfu::EcmaOpcode::LDA_V8:
                frameState.SetAcc(LoadRegister(bcInfo, 0));
                break;
            case kungfu::EcmaOpcode::LDFUNCTION:
                frameState.SetAcc(LoadParam(CALL_TARGET_PARAM_INDEX));
                break;
            case kungfu::EcmaOpcode::LDNEWTARGET:
                frameState.SetAcc(LoadParam(NEW_TARGET_PARAM_INDEX));
                break;
            case kungfu::EcmaOpcode::LDTHIS:
                frameState.SetAcc(LoadParam(THIS_OBJECT_PARAM_INDEX));
                break;
            // -------- Category #2: Constant Loads --------
            case kungfu::EcmaOpcode::LDNAN:
                LowerLdTaggedConstant(base::NumberHelper::GetNaN());
                break;
            case kungfu::EcmaOpcode::LDINFINITY:
                LowerLdTaggedConstant(base::NumberHelper::GetPositiveInfinity());
                break;
            case kungfu::EcmaOpcode::LDUNDEFINED:
                LowerLdRootConstant(RootConstantVertex::RootIndex::UNDEFINED);
                break;
            case kungfu::EcmaOpcode::LDNULL:
                LowerLdRootConstant(RootConstantVertex::RootIndex::NULL_VALUE);
                break;
            case kungfu::EcmaOpcode::LDTRUE:
                LowerLdRootConstant(RootConstantVertex::RootIndex::TRUE_VALUE);
                break;
            case kungfu::EcmaOpcode::LDFALSE:
                LowerLdRootConstant(RootConstantVertex::RootIndex::FALSE_VALUE);
                break;
            case kungfu::EcmaOpcode::LDHOLE:
                LowerLdTaggedConstant(JSTaggedValue::VALUE_HOLE);
                break;
            case kungfu::EcmaOpcode::LDAI_IMM32:
                LowerLdaiImm32(bcInfo);
                break;
            case kungfu::EcmaOpcode::FLDAI_IMM64:
                LowerFldaiImm64(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDBIGINT_ID16:
                LowerLdBigInt(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDA_STR_ID16:
                LowerLdString(bcInfo);
                break;
            // -------- Category #3: Unary Arithmetic --------
            case kungfu::EcmaOpcode::INC_IMM8:
                LowerInc();
                break;
            case kungfu::EcmaOpcode::DEC_IMM8:
                LowerDec();
                break;
            case kungfu::EcmaOpcode::NEG_IMM8:
                LowerNeg();
                break;
            case kungfu::EcmaOpcode::NOT_IMM8:
                LowerNot();
                break;
            // -------- Category #4: Binary Arithmetic --------
            case kungfu::EcmaOpcode::ADD2_IMM8_V8:
                LowerAdd2(bcInfo);
                break;
            case kungfu::EcmaOpcode::SUB2_IMM8_V8:
                LowerSub2(bcInfo);
                break;
            case kungfu::EcmaOpcode::MUL2_IMM8_V8:
                LowerMul2(bcInfo);
                break;
            case kungfu::EcmaOpcode::DIV2_IMM8_V8:
                LowerDiv2(bcInfo);
                break;
            case kungfu::EcmaOpcode::MOD2_IMM8_V8:
                LowerMod2(bcInfo);
                break;
            case kungfu::EcmaOpcode::EXP_IMM8_V8:
                LowerExp(bcInfo);
                break;
            case kungfu::EcmaOpcode::SHL2_IMM8_V8:
                LowerShl2(bcInfo);
                break;
            case kungfu::EcmaOpcode::SHR2_IMM8_V8:
                LowerShr2(bcInfo);
                break;
            case kungfu::EcmaOpcode::ASHR2_IMM8_V8:
                LowerAshr2(bcInfo);
                break;
            case kungfu::EcmaOpcode::AND2_IMM8_V8:
                LowerAnd2(bcInfo);
                break;
            case kungfu::EcmaOpcode::OR2_IMM8_V8:
                LowerOr2(bcInfo);
                break;
            case kungfu::EcmaOpcode::XOR2_IMM8_V8:
                LowerXor2(bcInfo);
                break;
            // -------- Category #5: Comparisons --------
            case kungfu::EcmaOpcode::EQ_IMM8_V8:
                LowerEq(bcInfo);
                break;
            case kungfu::EcmaOpcode::NOTEQ_IMM8_V8:
                LowerNotEq(bcInfo);
                break;
            case kungfu::EcmaOpcode::LESS_IMM8_V8:
                LowerLess(bcInfo);
                break;
            case kungfu::EcmaOpcode::LESSEQ_IMM8_V8:
                LowerLessEq(bcInfo);
                break;
            case kungfu::EcmaOpcode::GREATER_IMM8_V8:
                LowerGreater(bcInfo);
                break;
            case kungfu::EcmaOpcode::GREATEREQ_IMM8_V8:
                LowerGreaterEq(bcInfo);
                break;
            case kungfu::EcmaOpcode::STRICTNOTEQ_IMM8_V8:
                LowerStrictNotEq(bcInfo);
                break;
            case kungfu::EcmaOpcode::STRICTEQ_IMM8_V8:
                LowerStrictEq(bcInfo);
                break;
            case kungfu::EcmaOpcode::ISTRUE:
            case kungfu::EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
                LowerIsTrue();
                break;
            case kungfu::EcmaOpcode::ISFALSE:
            case kungfu::EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
                LowerIsFalse();
                break;
            // -------- Category #6: Type Conversions --------
            case kungfu::EcmaOpcode::TONUMBER_IMM8:
                LowerToNumber();
                break;
            case kungfu::EcmaOpcode::TONUMERIC_IMM8:
                LowerToNumeric();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_TOPROPERTYKEY_PREF_NONE:
                LowerToPropertyKey();
                break;
            // -------- Category #7: Property Access --------
            case kungfu::EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
                LowerTryLdGlobalByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
                LowerLdGlobalVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::STGLOBALVAR_IMM16_ID16:
                LowerStGlobalVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDSYMBOL:
                LowerLdSymbol();
                break;
            case kungfu::EcmaOpcode::LDGLOBAL:
                LowerLdGlobal();
                break;
            case kungfu::EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
                LowerLdObjByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
                LowerStObjByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
            case kungfu::EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
            case kungfu::EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
                LowerLdObjByIndex(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
            case kungfu::EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
            case kungfu::EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
                LowerStObjByIndex(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case kungfu::EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
                LowerLdObjByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
                LowerStObjByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STOWNBYVALUE_IMM16_V8_V8:
                LowerStOwnByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
            case kungfu::EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
            case kungfu::EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
                LowerStOwnByIndex(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
                LowerStOwnByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDTHISBYVALUE_IMM8:
            case kungfu::EcmaOpcode::LDTHISBYVALUE_IMM16:
                LowerLdThisByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::STTHISBYVALUE_IMM8_V8:
            case kungfu::EcmaOpcode::STTHISBYVALUE_IMM16_V8:
                LowerStThisByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
                LowerLdThisByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::STTHISBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::STTHISBYNAME_IMM16_ID16:
                LowerStThisByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
            case kungfu::EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
                LowerLdSuperByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8:
                LowerStSuperByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
                LowerLdSuperByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8:
                LowerStSuperByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16:
                LowerTryStGlobalByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
                LowerStConstToGlobalRecord(bcInfo, true);
                break;
            case kungfu::EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16:
                LowerStConstToGlobalRecord(bcInfo, false);
                break;
            case kungfu::EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
                LowerStOwnByValueWithNameSet(bcInfo);
                break;
            case kungfu::EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
                LowerStOwnByNameWithNameSet(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16:
                LowerLdPrivateProperty(bcInfo);
                break;
            case kungfu::EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8:
                LowerStPrivateProperty(bcInfo);
                break;
            // -------- Category #8: Function Calls --------
            case kungfu::EcmaOpcode::CALLARG0_IMM8:
                LowerCallArg0();
                break;
            case kungfu::EcmaOpcode::CALLARG1_IMM8_V8:
                LowerCallArg1(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLARGS2_IMM8_V8_V8:
                LowerCallArgs2(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
                LowerCallArgs3(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
                LowerCallThis0(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLTHIS0_IMM8_V8:
                LowerCallThis0(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
                LowerCallThis1(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
                LowerCallThis2(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
                LowerCallThis3(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
                LowerCallRange(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
                LowerCallThisRange(bcInfo);
                break;
            case kungfu::EcmaOpcode::APPLY_IMM8_V8_V8:
                LowerCallSpread(bcInfo);
                break;
            case kungfu::EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
                LowerSuperCallThisRange(bcInfo);
                break;
            case kungfu::EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
                LowerSuperCallArrowRange(bcInfo);
                break;
            case kungfu::EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
                LowerSuperCallSpread(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8:
                LowerSuperCallForwardAllArgs(bcInfo);
                break;
            case kungfu::EcmaOpcode::NEWOBJAPPLY_IMM8_V8:
            case kungfu::EcmaOpcode::NEWOBJAPPLY_IMM16_V8:
                LowerNewObjApply(bcInfo);
                break;
            case kungfu::EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
                LowerNewObjRange(bcInfo);
                break;
            // -------- Category #9: Object/Array Creation --------
            case kungfu::EcmaOpcode::CREATEITERRESULTOBJ_V8_V8:
                LowerCreateIterResultObj(bcInfo);
                break;
            case kungfu::EcmaOpcode::CREATEEMPTYARRAY_IMM8:
            case kungfu::EcmaOpcode::CREATEEMPTYARRAY_IMM16:
                LowerCreateEmptyArray();
                break;
            case kungfu::EcmaOpcode::CREATEEMPTYOBJECT:
                LowerCreateEmptyObject();
                break;
            case kungfu::EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
            case kungfu::EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
                LowerCreateObjectWithBuffer(bcInfo);
                break;
            case kungfu::EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
            case kungfu::EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
                LowerCreateObjectWithExcludedKeys(bcInfo);
                break;
            case kungfu::EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case kungfu::EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
                LowerCreateArrayWithBuffer(bcInfo);
                break;
            case kungfu::EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
            case kungfu::EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
                LowerCreateRegExpWithLiteral(bcInfo);
                break;
            // -------- Category #10: Class/Function/Field Definition --------
            case kungfu::EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
            case kungfu::EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
                LowerDefineMethod(bcInfo);
                break;
            case kungfu::EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
                LowerDefineGetterSetterByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
            case kungfu::EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
                LowerDefineClassWithBuffer(bcInfo);
                break;
            case kungfu::EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
            case kungfu::EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
                LowerDefineFunc(bcInfo);
                break;
            case kungfu::EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
                LowerDefinePropertyByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
                LowerDefineFieldByName(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8:
                LowerDefineFieldByValue(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8:
                LowerDefineFieldByIndex(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16:
                LowerCreatePrivateProperty(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8:
                LowerDefinePrivateProperty(bcInfo);
                break;
            // -------- Category #11: Iterators --------
            case kungfu::EcmaOpcode::GETPROPITERATOR:
                LowerGetPropIterator();
                break;
            case kungfu::EcmaOpcode::CLOSEITERATOR_IMM8_V8:
            case kungfu::EcmaOpcode::CLOSEITERATOR_IMM16_V8:
                LowerCloseIterator(bcInfo);
                break;
            case kungfu::EcmaOpcode::GETITERATOR_IMM8:
            case kungfu::EcmaOpcode::GETITERATOR_IMM16:
                LowerGetIterator();
                break;
            case kungfu::EcmaOpcode::GETNEXTPROPNAME_V8:
                LowerGetNextPropName(bcInfo);
                break;
            // -------- Category #12: Lexical Environment --------
            case kungfu::EcmaOpcode::NEWLEXENV_IMM8:
            case kungfu::EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
                LowerNewLexicalEnv(bcInfo);
                break;
            case kungfu::EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
                LowerNewLexicalEnvWithName(bcInfo);
                break;
            case kungfu::EcmaOpcode::POPLEXENV:
                LowerPopLexicalEnv(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDLEXVAR_IMM4_IMM4:
            case kungfu::EcmaOpcode::LDLEXVAR_IMM8_IMM8:
            case kungfu::EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
                LowerLdLexVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::STLEXVAR_IMM4_IMM4:
            case kungfu::EcmaOpcode::STLEXVAR_IMM8_IMM8:
            case kungfu::EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
                LowerStLexVar(bcInfo);
                break;
            // -------- Category #13: Modules --------
            case kungfu::EcmaOpcode::STMODULEVAR_IMM8:
            case kungfu::EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
                LowerStModuleVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::DYNAMICIMPORT:
                LowerDynamicImport();
                break;
            case kungfu::EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
            case kungfu::EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
            case kungfu::EcmaOpcode::CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8:
            case kungfu::EcmaOpcode::CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16:
                LowerLdExternalModuleVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::GETMODULENAMESPACE_IMM8:
            case kungfu::EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
                LowerGetModuleNamespace(bcInfo);
                break;
            case kungfu::EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16:
                LowerLdPatchVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::WIDE_STPATCHVAR_PREF_IMM16:
                LowerStPatchVar(bcInfo);
                break;
            case kungfu::EcmaOpcode::LDLOCALMODULEVAR_IMM8:
            case kungfu::EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
                LowerLdLocalModuleVar(bcInfo);
                break;
            // -------- Category #14: Miscellaneous --------
            case kungfu::EcmaOpcode::GETUNMAPPEDARGS:
                LowerGetUnmappedArgs();
                break;
            case kungfu::EcmaOpcode::TYPEOF_IMM8:
            case kungfu::EcmaOpcode::TYPEOF_IMM16:
                LowerTypeOf();
                break;
            case kungfu::EcmaOpcode::DELOBJPROP_V8:
                LowerDelObjProp(bcInfo);
                break;
            case kungfu::EcmaOpcode::ISIN_IMM8_V8:
                LowerIsIn(bcInfo);
                break;
            case kungfu::EcmaOpcode::INSTANCEOF_IMM8_V8:
                LowerInstanceOf(bcInfo);
                break;
            case kungfu::EcmaOpcode::GETTEMPLATEOBJECT_IMM8:
            case kungfu::EcmaOpcode::GETTEMPLATEOBJECT_IMM16:
                LowerGetTemplateObject();
                break;
            case kungfu::EcmaOpcode::SETOBJECTWITHPROTO_IMM8_V8:
            case kungfu::EcmaOpcode::SETOBJECTWITHPROTO_IMM16_V8:
                LowerSetObjectWithProto(bcInfo);
                break;
            case kungfu::EcmaOpcode::COPYDATAPROPERTIES_V8:
                LowerCopyDataProperties(bcInfo);
                break;
            case kungfu::EcmaOpcode::STARRAYSPREAD_V8_V8:
                LowerStoreArraySpread(bcInfo);
                break;
            case kungfu::EcmaOpcode::COPYRESTARGS_IMM8:
            case kungfu::EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
                LowerCopyRestArgs(bcInfo);
                break;
            case kungfu::EcmaOpcode::TESTIN_IMM8_IMM16_IMM16:
                LowerTestIn(bcInfo);
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE:
                LowerNotifyConcurrentResult();
                break;
            // -------- Category #15: Exceptions --------
            case kungfu::EcmaOpcode::THROW_PREF_NONE:
                LowerThrow();
                break;
            case kungfu::EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
                LowerThrowConstAssignment(bcInfo);
                break;
            case kungfu::EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
                LowerThrowNotExists();
                break;
            case kungfu::EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
                LowerThrowPatternNonCoercible();
                break;
            case kungfu::EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
                LowerThrowDeleteSuperProperty();
                break;
            case kungfu::EcmaOpcode::THROW_IFNOTOBJECT_PREF_V8:
                LowerThrowIfNotObject(bcInfo);
                break;
            case kungfu::EcmaOpcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
                LowerThrowUndefinedIfHole(bcInfo);
                break;
            case kungfu::EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
                LowerThrowUndefinedIfHoleWithName(bcInfo);
                break;
            case kungfu::EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
            case kungfu::EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
                LowerThrowIfSuperNotCorrectCall(bcInfo);
                break;
            // -------- Category #16: Control Flow --------
            case kungfu::EcmaOpcode::JEQZ_IMM8:
            case kungfu::EcmaOpcode::JEQZ_IMM16:
            case kungfu::EcmaOpcode::JEQZ_IMM32:
                LowerJumpIfZero();
                break;
            case kungfu::EcmaOpcode::JNEZ_IMM8:
            case kungfu::EcmaOpcode::JNEZ_IMM16:
            case kungfu::EcmaOpcode::JNEZ_IMM32:
                LowerJumpIfNonZero();
                break;
            case kungfu::EcmaOpcode::JMP_IMM8:
            case kungfu::EcmaOpcode::JMP_IMM16:
            case kungfu::EcmaOpcode::JMP_IMM32:
                LowerJumpConstant();
                break;
            case kungfu::EcmaOpcode::RETURNUNDEFINED:
                self->FinishBlockWith<ReturnVertex>(currentBlock, {self->undefinedValue_});
                break;
            case kungfu::EcmaOpcode::RETURN:
                self->FinishBlockWith<ReturnVertex>(currentBlock, {frameState.GetAcc()});
                break;
            default:
                UNREACHABLE();
        }
    }

    // -------- Category #2: Constant Loads --------

    void LowerLdTaggedConstant(JSTaggedType taggedValue)
    {
        frameState.SetAcc(self->graph_->GetTaggedConstant(taggedValue));
    }

    void LowerLdRootConstant(RootConstantVertex::RootIndex index)
    {
        frameState.SetAcc(self->graph_->GetRootConstant(index));
    }

    void LowerLdaiImm32(const BytecodeInfo *bcInfo)
    {
        frameState.SetAcc(TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0)));
    }

    void LowerFldaiImm64(const BytecodeInfo *bcInfo)
    {
        JSTaggedType taggedValue = JSTaggedValue(base::bit_cast<double>(GetImmediate(bcInfo, 0))).GetRawData();
        frameState.SetAcc(self->graph_->GetTaggedConstant(taggedValue));
    }

    void LowerLdString(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *res = StringFromConstPool(stringId);
        frameState.SetAcc(res);
    }

    void LowerLdBigInt(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *numberBigInt = StringFromConstPool(stringId);
        frameState.SetAcc(RuntimeCall({numberBigInt}, RTSTUB_ID(LdBigInt)));
    }

    // -------- Category #3: Unary Arithmetic --------

    void LowerInc()
    {
        ValueVertex *x = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Inc));
    }

    void LowerDec()
    {
        ValueVertex *x = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Dec));
    }

    void LowerNeg()
    {
        ValueVertex *x = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Neg));
    }

    void LowerNot()
    {
        ValueVertex *x = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Not));
    }

    // -------- Category #4: Binary Arithmetic --------

    // TODO: DEMONSTRATION ONLY. The following code simply serves as an example of subgraph builder.
    void LowerAdd2_DemoOnly(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();

        BB *trueBranch = self->NewBlock();
        BB *falseBranch = self->NewBlock();
        BB *doneBlock = self->NewBlock();

        // Dummy logic: if (x) acc <- x + y
        //              else   acc <- x + y
        ValueVertex *xIsTrue = CommonStubCall({glue, x}, CommonStubCSigns::ToBooleanTrue);
        // Submit currentBlock to the graph.
        // Note: Blocks should be submitted to the graph by RPO order.
        self->FinishBlockWithBranch(currentBlock, xIsTrue, trueBranch, falseBranch);

        // Bind currentBlock to trueBranch
        currentBlock = trueBranch;
        // Add result1 to currentBlock (which is trueBranch)
        ValueVertex *result1 = CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Add);
        // Submit currentBlock (which is trueBranch) to the graph.
        self->FinishBlockWithJump(currentBlock, doneBlock);

        // Bind currentBlock to falseBranch
        currentBlock = falseBranch;
        // Add result2 to currentBlock (which is falseBranch)
        ValueVertex *result2 = CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Add);
        // Submit currentBlock (which is falseBranch) to the graph.
        self->FinishBlockWithJump(currentBlock, doneBlock);

        // Bind currentBlock to doneBlock
        currentBlock = doneBlock;
        // Add Phi to currentBlock (which is doneBlock)
        frameState.SetAcc(self->NewPhiVertexWith(currentBlock, {result1, result2}, self->AccIndex()));

        // ... Continue processing the subsequent bytecodes with doneBlock
    }

    void LowerAdd2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Add));
    }

    void LowerSub2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Sub));
    }

    void LowerMul2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Mul));
    }

    void LowerDiv2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Div));
    }

    void LowerMod2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Mod));
    }

    void LowerExp(const BytecodeInfo *bcInfo)
    {
        ValueVertex *left = LoadRegister(bcInfo, 0);
        ValueVertex *right = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({left, right}, RTSTUB_ID(Exp)));
    }

    void LowerShl2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Shl));
    }

    void LowerShr2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Shr));
    }

    void LowerAshr2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Ashr));
    }

    void LowerAnd2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::And));
    }

    void LowerOr2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Or));
    }

    void LowerXor2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Xor));
    }

    // -------- Category #5: Comparisons --------

    void LowerEq(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Equal));
    }

    void LowerNotEq(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::NotEqual));
    }

    void LowerLess(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Less));
    }

    void LowerLessEq(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::LessEq));
    }

    void LowerGreater(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::Greater));
    }

    void LowerGreaterEq(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::GreaterEq));
    }

    void LowerStrictNotEq(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::StrictNotEqual));
    }

    void LowerStrictEq(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubCSigns::StrictEqual));
    }

    void LowerIsTrue()
    {
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *result = nullptr;
        if (auto *asConstant = value->TryCast<RootConstantVertex>(); asConstant != nullptr) {
            auto id = asConstant->GetIndex();

            if (id == RootConstantVertex::RootIndex::TRUE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerIsTrue(): TRUE -> TRUE";
                result = self->graph_->GetRootConstant(RootConstantVertex::RootIndex::TRUE_VALUE);
            } else if (id == RootConstantVertex::RootIndex::FALSE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerIsTrue(): FALSE -> FALSE";
                result = self->graph_->GetRootConstant(RootConstantVertex::RootIndex::FALSE_VALUE);
            }
        }
        if (result == nullptr) {
            result = CommonStubCall({glue, value}, CommonStubCSigns::ToBooleanTrue);
        }
        frameState.SetAcc(result);
    }

    void LowerIsFalse()
    {
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *result = nullptr;
        if (auto *asConstant = value->TryCast<RootConstantVertex>(); asConstant != nullptr) {
            auto id = asConstant->GetIndex();

            if (id == RootConstantVertex::RootIndex::TRUE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerIsFalse(): TRUE -> FALSE";
                result = self->graph_->GetRootConstant(RootConstantVertex::RootIndex::FALSE_VALUE);
            } else if (id == RootConstantVertex::RootIndex::FALSE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerIsFalse(): FALSE -> TRUE";
                result = self->graph_->GetRootConstant(RootConstantVertex::RootIndex::TRUE_VALUE);
            }
        }
        if (result == nullptr) {
            result = CommonStubCall({glue, value}, CommonStubCSigns::ToBooleanFalse);
        }
        frameState.SetAcc(result);
    }

    // -------- Category #6: Type Conversions --------

    void LowerToNumber()
    {
        ValueVertex *value = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(ToNumber)));
    }

    void LowerToNumeric()
    {
        ValueVertex *value = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(ToNumeric)));
    }

    void LowerToPropertyKey()
    {
        ValueVertex *value = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(ToPropertyKey)));
    }

    // -------- Category #7: Property Access --------

    void LowerLdObjByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = frameState.GetAcc();
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, id, GlobalEnv()}, CommonStubCSigns::GetPropertyByName));
    }

    void LowerStObjByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 2);  // 2: receiver register index
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, id, value, GlobalEnv()}, CommonStubCSigns::SetPropertyByName);
    }

    void LowerLdObjByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *key = frameState.GetAcc();
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, key, GlobalEnv()}, CommonStubCSigns::GetPropertyByValue));
    }

    void LowerStObjByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *key = LoadRegister(bcInfo, 2);  // 2: key register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, key, value, GlobalEnv()}, CommonStubCSigns::SetPropertyByValue);
    }

    void LowerLdObjByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(CommonStubCall({glue, receiver, index, GlobalEnv()}, CommonStubCSigns::LdObjByIndex));
    }

    void LowerStObjByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, receiver, index, value, GlobalEnv()}, CommonStubCSigns::StObjByIndex);
    }

    void LowerLdThisByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = frameState.GetAcc();
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, key, GlobalEnv()}, CommonStubCSigns::GetPropertyByValue));
    }

    void LowerStThisByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, key, value, GlobalEnv()}, CommonStubCSigns::SetPropertyByValue);
    }

    void LowerLdThisByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, id, GlobalEnv()}, CommonStubCSigns::GetPropertyByName));
    }

    void LowerStThisByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, id, value, GlobalEnv()}, CommonStubCSigns::SetPropertyByName);
    }

    void LowerLdSuperByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({thisObj, propKey, jsFunc}, RTSTUB_ID(OptLdSuperByValue)));
    }

    void LowerStSuperByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        RuntimeCall({thisObj, propKey, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
    }

    void LowerLdSuperByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = frameState.GetAcc();
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *prop = StringFromConstPool(stringId);
        frameState.SetAcc(RuntimeCall({thisObj, prop, jsFunc}, RTSTUB_ID(OptLdSuperByValue)));
    }

    void LowerStSuperByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *prop = StringFromConstPool(stringId);
        RuntimeCall({thisObj, prop, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
    }

    void LowerStOwnByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *key = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, receiver, key, value, GlobalEnv()}, CommonStubCSigns::StOwnByValue);
    }

    void LowerStOwnByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, receiver, index, value, GlobalEnv()}, CommonStubCSigns::StOwnByIndex);
    }

    void LowerStOwnByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubCSigns::StOwnByName);
    }

    void LowerStOwnByValueWithNameSet(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubCSigns::StOwnByValueWithNameSet);
    }

    void LowerStOwnByNameWithNameSet(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubCSigns::StOwnByNameWithNameSet);
    }

    void LowerTryLdGlobalByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(bcInfo, {id, GlobalEnv()}, CommonStubCSigns::TryLdGlobalByName));
    }

    void LowerTryStGlobalByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {id, value, GlobalEnv()}, CommonStubCSigns::TryStGlobalByName);
    }

    void LowerLdGlobalVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(bcInfo, {id, GlobalEnv()}, CommonStubCSigns::LdGlobalVar));
    }

    void LowerStGlobalVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {id, value, GlobalEnv()}, CommonStubCSigns::StGlobalVar);
    }

    void LowerStConstToGlobalRecord(const BytecodeInfo *bcInfo, bool isConst)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *isConstGate = isConst
            ? self->graph_->GetTaggedConstant(JSTaggedValue::True().GetRawData())
            : self->graph_->GetTaggedConstant(JSTaggedValue::False().GetRawData());
        RuntimeCall({propKey, value, isConstGate}, RTSTUB_ID(StGlobalRecord));
    }

    void LowerLdGlobal()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::JS_GLOBAL_OBJECT_INDEX * JSTaggedValue::TaggedTypeSize());

        frameState.SetAcc(self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {GlobalEnv()}, offset));
    }

    void LowerLdSymbol()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::SYMBOL_FUNCTION_INDEX * JSTaggedValue::TaggedTypeSize());

        frameState.SetAcc(self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {GlobalEnv()}, offset));
    }

    void LowerLdPrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);  // 3: lexicalEnv register index
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(LdPrivateProperty)));
    }

    void LowerStPrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *obj = LoadRegister(bcInfo, 3);  // 3: obj register index
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 4);  // 4: lexicalEnv register index
        ValueVertex *value = frameState.GetAcc();
        RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(StPrivateProperty));
    }

    // -------- Category #8: Function Calls --------

    void LowerCallArg0()
    {
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func}, CommonStubCSigns::CallArg0Stub);
        frameState.SetAcc(result);
    }

    void LowerCallArg1(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, a0Value}, CommonStubCSigns::CallArg1Stub);
        frameState.SetAcc(result);
    }

    void LowerCallArgs2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *a1Value = LoadRegister(bcInfo, 1);
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, a0Value, a1Value}, CommonStubCSigns::CallArg2Stub);
        frameState.SetAcc(result);
    }

    void LowerCallArgs3(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *a1Value = LoadRegister(bcInfo, 1);
        ValueVertex *a2Value = LoadRegister(bcInfo, 2);  // 2: third argument register index
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, a0Value, a1Value, a2Value}, CommonStubCSigns::CallArg3Stub);
        frameState.SetAcc(result);
    }

    void LowerCallThis0(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj}, CommonStubCSigns::CallThis0Stub);
        frameState.SetAcc(result);
    }

    void LowerCallThis1(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj, a0Value}, CommonStubCSigns::CallThis1Stub);
        frameState.SetAcc(result);
    }

    void LowerCallThis2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *a1Value = LoadRegister(bcInfo, 2);  // 2: second argument register index
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj, a0Value, a1Value}, CommonStubCSigns::CallThis2Stub);
        frameState.SetAcc(result);
    }

    void LowerCallThis3(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *a1Value = LoadRegister(bcInfo, 2);  // 2: second argument register index
        ValueVertex *a2Value = LoadRegister(bcInfo, 3);  // 3: third argument register index
        ValueVertex *func = frameState.GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj, a0Value, a1Value, a2Value}, CommonStubCSigns::CallThis3Stub);
        frameState.SetAcc(result);
    }

    void LowerCallRange(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();

        ValueVertex *func = frameState.GetAcc();
        ValueVertex *taggedInputSize = TaggedConstantFromInt32(static_cast<int>(inputSize));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(bcInfo, taggedInputSize, inputSize);

        ValueVertex *result = RuntimeCall({func, taggedArray, taggedInputSize}, RTSTUB_ID(CallRange));
        frameState.SetAcc(result);
    }

    void LowerCallThisRange(const BytecodeInfo *bcInfo)
    {
        // -1 : Skips the receiver
        uint32_t argc = bcInfo->inputs.size() - 1;

        ValueVertex *func = frameState.GetAcc();
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *taggedArgc = TaggedConstantFromInt32(static_cast<int>(argc));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(bcInfo, taggedArgc, argc, 1);

        ValueVertex *result = RuntimeCall({thisObj, func, taggedArray, taggedArgc}, RTSTUB_ID(CallThisRange));
        frameState.SetAcc(result);
    }

    void LowerCallSpread(const BytecodeInfo *bcInfo)
    {
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *thisArg = LoadRegister(bcInfo, 0);
        ValueVertex *argsArray = LoadRegister(bcInfo, 1);
        frameState.SetAcc(RuntimeCall({func, thisArg, argsArray}, RTSTUB_ID(CallSpread)));
    }

    void LowerSuperCallThisRange(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();

        ValueVertex *thisFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedInputSize = TaggedConstantFromInt32(static_cast<int>(inputSize));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(bcInfo, taggedInputSize, inputSize);

        frameState.SetAcc(RuntimeCall({thisFunc, newTarget, taggedArray, taggedInputSize}, RTSTUB_ID(OptSuperCall)));
    }

    void LowerSuperCallArrowRange(const BytecodeInfo *bcInfo)
    {
        uint32_t argc = bcInfo->inputs.size() - 1;
        ValueVertex *func = LoadRegister(bcInfo, argc);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedArgc = TaggedConstantFromInt32(static_cast<int>(argc));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(bcInfo, taggedArgc, argc);

        frameState.SetAcc(RuntimeCall({func, newTarget, taggedArray, taggedArgc}, RTSTUB_ID(OptSuperCall)));
    }

    void LowerSuperCallSpread(const BytecodeInfo *bcInfo)
    {
        ValueVertex *array = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);

        ValueVertex *argsArray = CommonStubCall({glue, array, GlobalEnv()}, CommonStubCSigns::GetCallSpreadArgs);
        frameState.SetAcc(RuntimeCall({func, newTarget, argsArray}, RTSTUB_ID(OptSuperCallSpread)));
    }

    void LowerSuperCallForwardAllArgs(const BytecodeInfo *bcInfo)
    {
        ValueVertex *func = LoadRegister(bcInfo, 0);
        ValueVertex *superFunc = CommonStubCall({glue, func}, CommonStubCSigns::GetPrototype);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedActualArgc = TaggedActualArgc();

        frameState.SetAcc(RuntimeCall({
            superFunc, newTarget, taggedActualArgc}, RTSTUB_ID(OptSuperCallForwardAllArgs)));
    }

    void LowerNewObjApply(const BytecodeInfo *bcInfo)
    {
        ValueVertex *target = LoadRegister(bcInfo, 0);
        ValueVertex *args = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({target, args}, RTSTUB_ID(NewObjApply)));
    }

    void LowerNewObjRange(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();
        ChunkVector<ValueVertex *> args(self->chunk_);
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(bcInfo, idx));
        }
        frameState.SetAcc(RuntimeCall(args, RTSTUB_ID(OptNewObjRange)));
    }

    // -------- Category #9: Object/Array Creation --------

    void LowerCreateEmptyObject()
    {
        frameState.SetAcc(RuntimeCall({}, RTSTUB_ID(CreateEmptyObject)));
    }

    void LowerCreateEmptyArray()
    {
        frameState.SetAcc(CommonStubCall({glue, GlobalEnv()}, CommonStubCSigns::CreateEmptyArray));
    }

    void LowerCreateObjectWithBuffer(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *obj = ObjectFromConstPool(index);
        ValueVertex *lexEnv = LoadRegister(bcInfo, 1);
        frameState.SetAcc(CommonStubCall({glue, obj, lexEnv}, CommonStubCSigns::CreateObjectHavingMethod));
    }

    void LowerCreateObjectWithExcludedKeys(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();
        ChunkVector<ValueVertex *> args(self->chunk_);
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(bcInfo, idx));
        }
        frameState.SetAcc(RuntimeCall(args, RTSTUB_ID(OptCreateObjectWithExcludedKeys)));
    }

    void LowerCreateArrayWithBuffer(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *slotId = self->graph_->GetInt32Constant(GetICSlotId<int>(bcInfo, 1));

        frameState.SetAcc(CommonStubCall(
            {glue, index, jsFunc, slotId, GlobalEnv()}, CommonStubCSigns::CreateArrayWithBuffer));
    }

    void LowerCreateRegExpWithLiteral(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *pattern = StringFromConstPool(stringId);
        ValueVertex *flags = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));

        frameState.SetAcc(RuntimeCall({pattern, flags}, RTSTUB_ID(CreateRegExpWithLiteral)));
    }

    void LowerCreateIterResultObj(const BytecodeInfo *bcInfo)
    {
        ValueVertex *value = LoadRegister(bcInfo, 0);
        ValueVertex *done = LoadRegister(bcInfo, 1);
        frameState.SetAcc(RuntimeCall({value, done}, RTSTUB_ID(CreateIterResultObj)));
    }

    // -------- Category #10: Class/Function/Field Definition --------

    void LowerDefineMethod(const BytecodeInfo *bcInfo)
    {
        ValueVertex *taggedMethodId = TaggedConstantFromInt32(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *length = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *env = LoadRegister(bcInfo, 2);  // 2: env register index
        ValueVertex *homeObject = frameState.GetAcc();
        ValueVertex *module = ModuleFromFunction();
        ValueVertex *method = MethodFromConstPool(taggedMethodId);

#if ECMASCRIPT_ENABLE_IC
        // 3 : slotId operand index
        ValueVertex *slotId = TaggedConstantFromInt32(GetICSlotId<int>(bcInfo, 3));
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);

        frameState.SetAcc(RuntimeCall(
            {method, homeObject, length, env, module, slotId, jsFunc}, RTSTUB_ID(DefineMethod)));
#else
        frameState.SetAcc(RuntimeCall(
            {method, homeObject, length, env, module}, RTSTUB_ID(DefineMethod)));
#endif
    }

    void LowerDefineFunc(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *slotId = self->graph_->GetInt32Constant(GetICSlotId<int>(bcInfo, 0));
        ValueVertex *methodId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 1));
        // 2: length operand index
        ValueVertex *length = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 2));
        // 3: lexicalEnv register index
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);

        frameState.SetAcc(CommonStubCall(
            {glue, jsFunc, methodId, length, lexicalEnv, slotId, GlobalEnv()},
            CommonStubCSigns::Definefunc));
    }

    void LowerDefineClassWithBuffer(const BytecodeInfo *bcInfo)
    {
        ValueVertex *methodId = TaggedConstantFromInt32(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *literalId = TaggedConstantFromInt32(GetConstDataId<int>(bcInfo, 1));
        ValueVertex *length = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *proto = LoadRegister(bcInfo, 3);  // 3: proto register index
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 4);  // 4: lexicalEnv register index
        ValueVertex *sharedConstPool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();

#if ECMASCRIPT_ENABLE_IC
        ValueVertex *slotId = TaggedConstantFromInt32(GetICSlotId<int>(bcInfo, 5));  // 5 : Slot ID index
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);

        frameState.SetAcc(RuntimeCall(
            {proto, lexicalEnv, sharedConstPool, methodId, literalId, module, length, slotId, jsFunc},
            RTSTUB_ID(CreateClassWithBuffer)));
#else
        frameState.SetAcc(RuntimeCall(
            {proto, lexicalEnv, sharedConstPool, methodId, literalId, module, length},
            RTSTUB_ID(CreateClassWithBuffer)));
#endif
    }

    void LowerDefineGetterSetterByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *obj = LoadRegister(bcInfo, 0);
        ValueVertex *prop = LoadRegister(bcInfo, 1);
        ValueVertex *getter = LoadRegister(bcInfo, 2);  // 2: getter register index
        ValueVertex *setter = LoadRegister(bcInfo, 3);  // 3: setter register index
        ValueVertex *acc = frameState.GetAcc();
        ValueVertex *undefinedValue = self->undefinedValue_;
        ValueVertex *taggedOne = TaggedConstantFromInt32(1);

        frameState.SetAcc(RuntimeCall(
            {obj, prop, getter, setter, acc, undefinedValue, taggedOne},
            RTSTUB_ID(DefineGetterSetterByValue)));
    }

    void LowerDefinePropertyByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 1));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, obj, prop, value, GlobalEnv()}, CommonStubCSigns::DefineField);
    }

    void LowerDefineFieldByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 1));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, obj, prop, value, GlobalEnv()}, CommonStubCSigns::DefineField);
    }

    void LowerDefineFieldByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *propKey = LoadRegister(bcInfo, 0);
        ValueVertex *acc = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, acc, GlobalEnv()}, CommonStubCSigns::DefineField);
    }

    void LowerDefineFieldByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *propKey = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *acc = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, acc, GlobalEnv()}, CommonStubCSigns::DefineField);
    }

    void LowerCreatePrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *count = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *literalId = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 2);  // 2: lexicalEnv register index
        ValueVertex *constpool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();

        RuntimeCall({lexicalEnv, count, constpool, literalId, module}, RTSTUB_ID(CreatePrivateProperty));
    }

    void LowerDefinePrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);  // 3: lexicalEnv register index
        ValueVertex *value = frameState.GetAcc();
        RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(DefinePrivateProperty));
    }

    // -------- Category #11: Iterators --------

    void LowerGetIterator()
    {
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, obj, GlobalEnv()}, CommonStubCSigns::GetIterator));
    }

    void LowerGetPropIterator()
    {
        ValueVertex *object = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, object, GlobalEnv()}, CommonStubCSigns::Getpropiterator));
    }

    void LowerCloseIterator(const BytecodeInfo *bcInfo)
    {
        ValueVertex *iterator = LoadRegister(bcInfo, 0);
        frameState.SetAcc(RuntimeCall({iterator}, RTSTUB_ID(CloseIterator)));
    }

    void LowerGetNextPropName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *iterator = LoadRegister(bcInfo, 0);
        frameState.SetAcc(RuntimeCall({iterator}, RTSTUB_ID(GetNextPropNameSlowpath)));
    }

    // -------- Category #12: Lexical Environment --------

    void LowerNewLexicalEnv(const BytecodeInfo *bcInfo)
    {
        ValueVertex *parent = LoadRegister(bcInfo, 1);
        ValueVertex *scope = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        ValueVertex *newEnv = CommonStubCall({glue, parent, scope}, CommonStubCSigns::NewLexicalEnv);

        frameState.SetAcc(newEnv);
        frameState.SetLexicalEnv(newEnv);
    }

    void LowerNewLexicalEnvWithName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *level = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *slotId = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        // 2: env register index
        ValueVertex *newEnv = RuntimeCall(
            {level, slotId, LoadRegister(bcInfo, 2), jsFunc}, RTSTUB_ID(OptNewLexicalEnvWithName));

        frameState.SetAcc(newEnv);
        frameState.SetLexicalEnv(newEnv);
    }

    void LowerPopLexicalEnv(const BytecodeInfo *bcInfo)
    {
        ValueVertex *currentEnv = LoadRegister(bcInfo, 0);
        ValueVertex *parentEnv = GetValueFromTaggedArray(currentEnv, LexicalEnv::PARENT_ENV_INDEX);

        frameState.SetAcc(parentEnv);
        frameState.SetLexicalEnv(parentEnv);
    }

    void LowerLdLexVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *level = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        ValueVertex *slot = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 2);  // 2: lexicalEnv register index
        frameState.SetAcc(CommonStubCall({glue, level, slot, lexicalEnv}, CommonStubCSigns::LdLexVar));
    }

    void LowerStLexVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *level = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        ValueVertex *slot = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 2);  // 2: lexicalEnv register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, level, slot, lexicalEnv, value}, CommonStubCSigns::StLexVar);
    }

    // -------- Category #13: Modules --------

    void LowerLdExternalModuleVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(RuntimeCall({index, jsFunc}, RTSTUB_ID(LdExternalModuleVarByIndexOnJSFunc)));
    }

    void LowerGetModuleNamespace(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(RuntimeCall({index, jsFunc}, RTSTUB_ID(GetModuleNamespaceByIndexOnJSFunc)));
    }

    void LowerLdLocalModuleVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(RuntimeCall({index, jsFunc}, RTSTUB_ID(LdLocalModuleVarByIndexOnJSFunc)));
    }

    void LowerStModuleVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *value = frameState.GetAcc();
        RuntimeCall({index, value, jsFunc}, RTSTUB_ID(StModuleVarByIndexOnJSFunc));
    }

    void LowerDynamicImport()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *specifier = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({specifier, jsFunc}, RTSTUB_ID(DynamicImport)));
    }

    void LowerLdPatchVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(RuntimeCall({index}, RTSTUB_ID(LdPatchVar)));
    }

    void LowerStPatchVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *value = frameState.GetAcc();
        RuntimeCall({index, value}, RTSTUB_ID(StPatchVar));
    }

    // -------- Category #14: Miscellaneous --------

    void LowerTypeOf()
    {
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, obj}, CommonStubCSigns::TypeOf));
    }

    void LowerGetUnmappedArgs()
    {
        ValueVertex *argv = self->graph_->GetIntPtrConstant(0);
        ValueVertex *numArgs = ActualArgc();
        ValueVertex *argvTaggedArray = self->undefinedValue_;

        frameState.SetAcc(CommonStubCall(
            {glue, argv, numArgs, argvTaggedArray, GlobalEnv()}, CommonStubCSigns::GetUnmappedArgs));
    }

    void LowerCopyRestArgs(const BytecodeInfo *bcInfo)
    {
        ValueVertex *taggedArgc = TaggedActualArgc();
        ValueVertex *taggedRestIdx = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(RuntimeCall({taggedArgc, taggedRestIdx}, RTSTUB_ID(OptCopyRestArgs)));
    }

    void LowerDelObjProp(const BytecodeInfo *bcInfo)
    {
        ValueVertex *object = LoadRegister(bcInfo, 0);
        ValueVertex *prop = frameState.GetAcc();

        frameState.SetAcc(CommonStubCall(
            {glue, object, prop, GlobalEnv()}, CommonStubCSigns::DeleteObjectProperty));
    }

    void LowerIsIn(const BytecodeInfo *bcInfo)
    {
        ValueVertex *prop = LoadRegister(bcInfo, 0);
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, prop, obj, GlobalEnv()}, CommonStubCSigns::IsIn));
    }

    void LowerInstanceOf(const BytecodeInfo *bcInfo)
    {
        ValueVertex *object = LoadRegister(bcInfo, 1);
        ValueVertex *target = frameState.GetAcc();
        frameState.SetAcc(CommonStubCallWithIC(bcInfo, {object, target, GlobalEnv()}, CommonStubCSigns::Instanceof));
    }

    void LowerTestIn(const BytecodeInfo *bcInfo)
    {
        // 1: level operand index
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        // 2: slot operand index
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);  // 3: lexicalEnv register index
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(TestIn)));
    }

    void LowerCopyDataProperties(const BytecodeInfo *bcInfo)
    {
        ValueVertex *target = LoadRegister(bcInfo, 0);
        ValueVertex *source = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({target, source}, RTSTUB_ID(CopyDataProperties)));
    }

    void LowerStoreArraySpread(const BytecodeInfo *bcInfo)
    {
        ValueVertex *array = LoadRegister(bcInfo, 0);
        ValueVertex *index = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({array, index, value}, RTSTUB_ID(StArraySpread)));
    }

    void LowerGetTemplateObject()
    {
        ValueVertex *value = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(GetTemplateObject)));
    }

    void LowerSetObjectWithProto(const BytecodeInfo *bcInfo)
    {
        ValueVertex *proto = LoadRegister(bcInfo, 0);
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({proto, obj}, RTSTUB_ID(SetObjectWithProto)));
    }

    void LowerNotifyConcurrentResult()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *result = frameState.GetAcc();
        RuntimeCall({result, jsFunc}, RTSTUB_ID(NotifyConcurrentResult));
    }

    // -------- Category #15: Exceptions --------

    void LowerThrow()
    {
        constexpr bool HAS_INPUT = true;
        ValueVertex *exception = frameState.GetAcc();
        auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {exception}, RTSTUB_ID(Throw), HAS_INPUT);
        MergeCurrentFrameStateToCatchBlock(vertex);
    }

    void LowerThrowConstAssignment(const BytecodeInfo *bcInfo)
    {
        constexpr bool HAS_INPUT = true;
        ValueVertex *value = LoadRegister(bcInfo, 0);
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {value}, RTSTUB_ID(ThrowConstAssignment), HAS_INPUT);
        MergeCurrentFrameStateToCatchBlock(vertex);
    }

    void LowerThrowNotExists()
    {
        constexpr bool HAS_INPUT = false;
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {self->undefinedValue_}, RTSTUB_ID(ThrowThrowNotExists), HAS_INPUT);
        MergeCurrentFrameStateToCatchBlock(vertex);
    }

    void LowerThrowPatternNonCoercible()
    {
        constexpr bool HAS_INPUT = false;
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {self->undefinedValue_}, RTSTUB_ID(ThrowPatternNonCoercible), HAS_INPUT);
        MergeCurrentFrameStateToCatchBlock(vertex);
    }

    void LowerThrowDeleteSuperProperty()
    {
        constexpr bool HAS_INPUT = false;
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {self->undefinedValue_}, RTSTUB_ID(ThrowDeleteSuperProperty), HAS_INPUT);
        MergeCurrentFrameStateToCatchBlock(vertex);
    }

    void LowerThrowIfNotObject(const BytecodeInfo *bcInfo)
    {
        // Requires sub-graph mechanism which is unsupported currently.
        (void)bcInfo;
        LOG_COMPILER(WARN) << "Unimplemented: LowerThrowIfNotObject";
    }

    void LowerThrowUndefinedIfHole(const BytecodeInfo *bcInfo)
    {
        // Requires sub-graph mechanism which is unsupported currently.
        (void)bcInfo;
        LOG_COMPILER(WARN) << "Unimplemented: LowerThrowUndefinedIfHole";
    }

    void LowerThrowUndefinedIfHoleWithName(const BytecodeInfo *bcInfo)
    {
        // Requires sub-graph mechanism which is unsupported currently.
        (void)bcInfo;
        LOG_COMPILER(WARN) << "Unimplemented: LowerThrowUndefinedIfHoleWithName";
    }

    void LowerThrowIfSuperNotCorrectCall(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *thisValue = frameState.GetAcc();

        auto *vertex = self->NewVertex<ThrowIfSuperNotCorrectCallVertex>(
            currentBlock, {index, thisValue}, RTSTUB_ID(ThrowIfSuperNotCorrectCall));
        MergeCurrentFrameStateToCatchBlock(vertex);
    }

    // -------- Category #16: Control Flow --------

    void LowerJumpIfZero()
    {
        ValueVertex *acc = frameState.GetAcc();
        if (auto *asConstant = acc->TryCast<RootConstantVertex>(); asConstant != nullptr) {
            auto id = asConstant->GetIndex();

            if (id == RootConstantVertex::RootIndex::TRUE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerJumpIfZero(): TRUE -> Fallthrough";
                self->FinishBlockWithJump(currentBlock, FallthroughTarget());
                return;
            }
            if (id == RootConstantVertex::RootIndex::FALSE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerJumpIfZero(): FALSE -> Jump";
                self->FinishBlockWithJump(currentBlock, JumpTarget());
                return;
            }
        }
        self->FinishBlockWithBranch(currentBlock, acc, FallthroughTarget(), JumpTarget());
    }

    void LowerJumpIfNonZero()
    {
        ValueVertex *acc = frameState.GetAcc();
        if (auto *asConstant = acc->TryCast<RootConstantVertex>(); asConstant != nullptr) {
            auto id = asConstant->GetIndex();

            if (id == RootConstantVertex::RootIndex::TRUE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerJumpIfNonZero(): TRUE -> Jump";
                self->FinishBlockWithJump(currentBlock, JumpTarget());
                return;
            }
            if (id == RootConstantVertex::RootIndex::FALSE_VALUE) {
                LOG_COMPILER(DEBUG) << "LowerJumpIfNonZero(): FALSE -> Fallthrough";
                self->FinishBlockWithJump(currentBlock, FallthroughTarget());
                return;
            }
        }
        self->FinishBlockWithBranch(currentBlock, acc, JumpTarget(), FallthroughTarget());
    }

    void LowerJumpConstant()
    {
        ASSERT(blockInfo->IsJump());
        BB *target = self->ActivateNonCatchBlock(blockInfo->jumpBlock->rpoIndex);

        if (blockInfo->jumpBlock->loopBackBlock == blockInfo) {
            self->FinishBlockWithJumpLoop(currentBlock, target);
        } else {
            self->FinishBlockWithJump(currentBlock, target);
        }
    }

    // -------- Helpers --------

    BB *JumpTarget() const
    {
        ASSERT(blockInfo->IsJump());
        return self->ActivateNonCatchBlock(blockInfo->jumpBlock->rpoIndex);
    }

    BB *FallthroughTarget() const
    {
        ASSERT(blockInfo->HasFallthrough());
        return self->ActivateNonCatchBlock(blockInfo->fallthroughBlock->rpoIndex);
    }

    ValueVertex *LoadRegister(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *vreg = std::get_if<VirtualRegister>(bcInfo->inputs.data() + inputIndex);
        ASSERT(vreg != nullptr);
        return frameState.Get(vreg->GetId());
    }

    ValueVertex *LoadParam(VRegIDType paramIndex) const
    {
        VirtualRegister vreg = VRegOfParam(self->numLocal_, paramIndex);
        return frameState.Get(vreg.GetId());
    }

    template <class CastsTo = uint16_t>
    CastsTo GetConstDataId(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *constDataId = std::get_if<ConstDataId>(bcInfo->inputs.data() + inputIndex);
        ASSERT(constDataId != nullptr);
        return static_cast<CastsTo>(constDataId->GetId());
    }

    template <class CastsTo = ICSlotIdType>
    CastsTo GetICSlotId(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *icSlotId = std::get_if<ICSlotId>(bcInfo->inputs.data() + inputIndex);
        ASSERT(icSlotId != nullptr);
        return static_cast<CastsTo>(icSlotId->GetId());
    }

    template <class CastsTo = ImmValueType>
    CastsTo GetImmediate(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *imm = std::get_if<Immediate>(bcInfo->inputs.data() + inputIndex);
        ASSERT(imm != nullptr);
        return static_cast<CastsTo>(imm->GetValue());
    }

    ValueVertex *GlobalEnv()
    {
        if (UNLIKELY(lazyGlobalEnv == nullptr)) {
            lazyGlobalEnv = self->ActivateGlobalEnv();  // Update self->lazyGlobalEnv_
        }
        return lazyGlobalEnv;
    }

    ValueVertex *ActualArgc()
    {
        return self->NewVertex<ActualArgcVertex>(currentBlock, {});
    }

    ValueVertex *TaggedActualArgc()
    {
        ValueVertex *argc = ActualArgc();
        return self->NewVertex<ToTaggedIntVertex>(currentBlock, {argc});
    }

    void MergeCurrentFrameStateToCatchBlock(ThrowableMixin *mixin)
    {
        if (reinterpret_cast<uintptr_t>(lazyCatchBlock) == NO_CATCH_BLOCK_TAG) {
            return;
        }
        if (UNLIKELY(lazyCatchBlock == nullptr)) {
            lazyCatchBlock = self->ActivateCatchBlock(&lazyCatchBlockInputs, blockInfo->catchBlock->rpoIndex);
        }
        ASSERT(lazyCatchBlockInputs != nullptr);
        uint32_t catchPredIndex = lazyCatchBlockInputs->AddCatchPredecessor(frameState, self->undefinedValue_);
        mixin->SetCaughtBy(lazyCatchBlock);
        mixin->SetCatchPredecessorIndex(catchPredIndex);
    }

    void ValidateCommonStubCallArgs(Span<ValueVertex *const> inputs, CommonStubID id)
    {
#ifndef NDEBUG
        const CallSignature *signature = CommonStubCSigns::Get(id);
        size_t actualCount = inputs.size();
        size_t expectedCount = signature->GetParametersCount();
        if (actualCount != expectedCount) {
            LOG_ECMA(FATAL) << "ArkSteed CommonStub argument count mismatch, stub: " << signature->GetName()
                            << ", expected: " << expectedCount << ", actual: " << actualCount;
            UNREACHABLE();
        }
        kungfu::VariableType *params = signature->GetParametersType();
        if (params != nullptr) {
            const auto *inputArr = inputs.begin();
            for (size_t i = 0; i < expectedCount; ++i) {
                if (MatchesCallSignatureType(inputArr[i], params[i])) {
                    continue;
                }
                const char *reprName = ValueRepresentationName(inputArr[i]->GetValueRepresentation());
                LOG_ECMA(FATAL) << "ArkSteed CommonStub argument type mismatch, stub: " << signature->GetName()
                                << ", index: " << i
                                << ", expected machine type: " << MachineTypeToStr(params[i].GetMachineType())
                                << ", actual representation: " << reprName;
                UNREACHABLE();
            }
        }
#else
        (void)inputs;  // // No-op in Release build
        (void)id;
#endif
    }

    ValueVertex *CommonStubCall(std::initializer_list<ValueVertex *> inputs, CommonStubID id)
    {
        ValidateCommonStubCallArgs({inputs.begin(), inputs.end()}, id);
        auto *vertex = self->NewVertex<CallCommonStubVertex>(currentBlock, inputs, id);
        MergeCurrentFrameStateToCatchBlock(vertex);
        return vertex;
    }

    ValueVertex *CommonStubCallWithIC(
        const BytecodeInfo *bcInfo, std::initializer_list<ValueVertex *> inputs, CommonStubID id)
    {
        ChunkVector<ValueVertex *> allArgs(self->chunk_);
        allArgs.reserve(inputs.size() + 3);  // 3: glue + jsFunc + slotId

        allArgs.push_back(glue);
        allArgs.insert(allArgs.end(), inputs.begin(), inputs.end());
        allArgs.push_back(LoadParam(CALL_TARGET_PARAM_INDEX));
        allArgs.push_back(self->graph_->GetInt32Constant(GetICSlotId<int>(bcInfo, 0)));

        ValidateCommonStubCallArgs({allArgs.data(), allArgs.size()}, id);
        auto *vertex = self->NewVertex<CallCommonStubVertex>(currentBlock, allArgs, id);
        MergeCurrentFrameStateToCatchBlock(vertex);
        return vertex;
    }

    template <class InputRange = std::initializer_list<ValueVertex *>>
    ValueVertex *RuntimeCall(const InputRange &inputs, RuntimeStubID id)
    {
        auto *vertex = self->NewVertex<CallRuntimeVertex>(currentBlock, inputs, id);
        MergeCurrentFrameStateToCatchBlock(vertex);
        return vertex;
    }

    ValueVertex *TaggedConstantFromInt32(int value)
    {
        JSTaggedType taggedValue = JSTaggedValue(value).GetRawData();
        return self->graph_->GetTaggedConstant(taggedValue);
    }

    ValueVertex *TaggedArrayFromValueIn(
        const BytecodeInfo *bcInfo, ValueVertex *taggedInputSize, uint32_t inputSize, uint32_t startIndex = 0)
    {
        ValueVertex *taggedArray = RuntimeCall({taggedInputSize}, RTSTUB_ID(NewTaggedArray));
        for (uint32_t idx = 0; idx < inputSize; ++idx) {
            ValueVertex *arg = LoadRegister(bcInfo, startIndex + idx);
            SetValueToTaggedArray(taggedArray, idx, arg);
        }
        return taggedArray;
    }

    ValueVertex *GetValueFromTaggedArray(ValueVertex *array, uint32_t index)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {array}, offset);
    }

    ValueVertex *SetValueToTaggedArray(ValueVertex *array, uint32_t index, ValueVertex *value)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<StoreTaggedFieldVertex>(currentBlock, {array, value}, offset);
    }

    ValueVertex *SharedConstPool()
    {
        int32_t methodOffset = static_cast<int32_t>(JSFunctionBase::METHOD_OFFSET);
        int32_t constpoolOffset = static_cast<int32_t>(Method::CONSTANT_POOL_OFFSET);

        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *method = self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {jsFunc}, methodOffset);
        return self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {method}, constpoolOffset);
    }

    ValueVertex *ModuleFromFunction()
    {
        int32_t moduleOffset = static_cast<int32_t>(JSFunction::ECMA_MODULE_OFFSET);
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        return self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {jsFunc}, moduleOffset);
    }

    ValueVertex *StringFromConstPool(ValueVertex *stringId)
    {
        ValueVertex *constpool = SharedConstPool();
        return CommonStubCall({glue, constpool, stringId}, CommonStubCSigns::GetStringFromConstPool);
    }

    ValueVertex *ObjectFromConstPool(ValueVertex *index)
    {
        ValueVertex *constpool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();
        return CommonStubCall({glue, constpool, index, module}, CommonStubCSigns::GetObjectFromConstPool);
    }

    ValueVertex *MethodFromConstPool(ValueVertex *index)
    {
        ValueVertex *constpool = SharedConstPool();
        return RuntimeCall({constpool, index}, RTSTUB_ID(GetMethodFromCache));
    }

    GraphBuilderNew *self;
    ValueVertex *glue;           // Equivalent to self->glue_. Cached for performance.
    ValueVertex *lazyGlobalEnv;  // Equivalent to self->lazyGlobalEnv_. Cached for performance.
    const BasicBlockInfo *blockInfo;
    BB *currentBlock;    // Equivalent to self->blocks_[blockInfo->rpoIndex]. Cached for performance.
    BB *lazyCatchBlock;  // Equivalent to self->blocks_[blockInfo->catchBlock->rpoIndex]. Cached for performance.
    SharedBCFrameState frameState;
    CatchBlockInputData *lazyCatchBlockInputs;
};

void GraphBuilderNew::VisitBytecodesOfBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);

    BB *catchBlock = reinterpret_cast<BB *>(NO_CATCH_BLOCK_TAG);
    CatchBlockInputData *caughtByData = reinterpret_cast<CatchBlockInputData *>(NO_CATCH_BLOCK_TAG);
    if (blockInfo->catchBlock != nullptr) {
        uint32_t catchBlockIndex = blockInfo->catchBlock->rpoIndex;
        // May be nullptr (indicating that the catch block is not activated yet)
        catchBlock = blocks_[catchBlockIndex];
        caughtByData = catchBlockInputs_[catchBlockIndex];
    }

    BytecodeVisitor visitor{
        .self = this,
        .glue = glue_,
        .lazyGlobalEnv = lazyGlobalEnv_,
        .blockInfo = blockInfo,
        .currentBlock = blocks_[rpoIndex],  // visitor.currentBlock may be updated by subgraph creation
        .lazyCatchBlock = catchBlock,
        .frameState = frameState,
        .lazyCatchBlockInputs = caughtByData,
    };
    for (uint32_t bcIndex = blockInfo->startBcIndex; bcIndex <= blockInfo->endBcIndex; ++bcIndex) {
        visitor.Visit(preproc_->GetBytecode(bcIndex));
    }

    if (visitor.currentBlock->GetControlVertex() == nullptr) {
        ASSERT(blockInfo->IsFallthrough());
        BB *target = ActivateNonCatchBlock(blockInfo->fallthroughBlock->rpoIndex);
        FinishBlockWithJump(visitor.currentBlock, target);
    }
}
}  // namespace panda::ecmascript::arksteed
