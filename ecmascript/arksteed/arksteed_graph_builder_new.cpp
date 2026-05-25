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

const char *MachineTypeName(kungfu::MachineType machineType)
{
    switch (machineType) {
        case kungfu::MachineType::NOVALUE:
            return "novalue";
        case kungfu::MachineType::ANYVALUE:
            return "anyvalue";
        case kungfu::MachineType::ARCH:
            return "arch";
        case kungfu::MachineType::FLEX:
            return "flex";
        case kungfu::MachineType::I1:
            return "i1";
        case kungfu::MachineType::I8:
            return "i8";
        case kungfu::MachineType::I16:
            return "i16";
        case kungfu::MachineType::I32:
            return "i32";
        case kungfu::MachineType::I64:
            return "i64";
        case kungfu::MachineType::F32:
            return "f32";
        case kungfu::MachineType::F64:
            return "f64";
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

GraphBuilderNew::GraphBuilderNew(Graph *destGraph,
                                 uintptr_t glueAddr,
                                 BytecodePreprocessorNew *preproc,
                                 BytecodeAnalysisNew *analysis)
    : graph_(destGraph),
      glueAddr_(glueAddr),
      preproc_(preproc),
      analysis_(analysis),
      blocks_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      frameStates_(preproc->GetNumLiveBasicBlocks(), preproc_->GetChunk())
{}

bool GraphBuilderNew::Run()
{
    JitCompilationEnv *env = preproc_->GetEnv();
    MethodLiteral *method = preproc_->GetMethod();
    const char *recordName = method->GetRecordNameWithSymbol(env->GetJSPandaFile(), method->GetMethodId());
    const char *methodName = method->GetMethodName(env->GetJSPandaFile(), method->GetMethodId());
    LOG_COMPILER(DEBUG) << "Starts compiling " << recordName << " :: " << methodName;
    LOG_COMPILER(DEBUG) << "NumLocalVRegs = " << GetNumLocalVRegs() << ", NumParamVRegs = " << GetNumParamVRegs();
    // 1 : Start block only.
    if (preproc_->GetNumLiveBasicBlocks() <= 1) {
        return false;  // Possibly empty bytecode
    }
    InitializeBasicBlocks();
    InitializeGlobalsAndParameters();
    FinalizeStartBasicBlock();
    // 1 : Skips the start block (which is initialized before)
    for (uint32_t i = 1, n = preproc_->GetNumLiveBasicBlocks(); i < n; i++) {
        ProcessBasicBlock(i);
    }
    FinalizeBasicBlockRelations();
    return true;
}

void GraphBuilderNew::InitializeBasicBlocks()
{
    uint32_t n = preproc_->GetNumLiveBasicBlocks();
    for (uint32_t i = 0; i < n; i++) {
        blocks_[i] = BB::New(GetChunk());
        const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(i);
        // Set block flags directly on BB
        blocks_[i]->SetLoopHeader(bcBlock->IsLoopHeader());
        blocks_[i]->SetExceptionHandler(bcBlock->IsCatchBlockHeader());
        // Compute predecessor count and set RegisterMergeState.
        // RegisterMergeState is needed even for 1-predecessor blocks when they
        // are a conditional jump target (the regalloc merge path handles them
        // correctly, while the fallthrough path assumes consecutive vertex IDs).
        // This matches the old GraphBuilder behavior which always set
        // RegisterMergeState via MergePointFrameState for every block.
        uint32_t numPreds = static_cast<uint32_t>(bcBlock->jumpPredecessors.size() +
                                                  bcBlock->catchPredecessors.size());
        blocks_[i]->SetPredecessorCount(numPreds);
        if (numPreds >= 1) {
            blocks_[i]->SetRegisterMergeState(GetChunk()->New<RegisterMergeState>());
        }
    }
}

void GraphBuilderNew::InitializeGlobalsAndParameters()
{
    glue_ = graph_->GetIntPtrConstant(glueAddr_);
    undefinedValue_ = graph_->GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);

    // caller argument area (in fp-slot words): +2 argc, +3 call-target, +4 new-target, +5 this, +6... user args.
    const int32_t CALL_TARGET_FP_SLOT_INDEX = 3;
    for (uint32_t i = 0, n = GetNumParamVRegs(); i < n; i++) {
        int32_t slotIndex = static_cast<int32_t>(i + CALL_TARGET_FP_SLOT_INDEX);
        ValueVertex *v = NewVertexNoInput<InitialValueVertex>(blocks_[0], slotIndex);
        graph_->AddParameter(v);
    }
}

void GraphBuilderNew::FinalizeStartBasicBlock()
{
    auto frameState = BCFrameState(GetNumVRegs(), GetChunk());
    for (VRegIDType i = 0, n = GetNumParamVRegs(); i < n; i++) {
        VRegIDType curIndex = VRegOfParam(GetNumLocalVRegs(), i).GetId();
        frameState.Set(curIndex, graph_->GetParameter(i));
    }
    // -3 : Fixed header lexicalEnv is at slot -3 in word units.
    frameState.SetEnv(NewVertexNoInput<InitialValueVertex>(blocks_[0], -3));
    frameStates_[0] = CondensedBCFrameState(std::move(frameState), analysis_->GetLiveOut(0), GetChunk());

    NewControlVertex<JumpVertex>(blocks_[0], {}, blocks_[1]);
}

void GraphBuilderNew::FinalizeBasicBlockRelations()
{
    uint32_t n = preproc_->GetNumLiveBasicBlocks();
    for (uint32_t i = 0; i < n; i++) {
        const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(i);
        for (const BasicBlockInfo *predBcBlock : bcBlock->jumpPredecessors) {
            blocks_[i]->AddPredecessor(blocks_[predBcBlock->rpoIndex]);
        }
        uint32_t numPreds = bcBlock->jumpPredecessors.size();
        if (numPreds <= 1) {
            continue;
        }
        for (uint32_t j = 0; j < numPreds; j++) {
            BB *pred = blocks_[bcBlock->jumpPredecessors[j]->rpoIndex];
            if (JumpVertex *asJump = pred->GetControlVertex()->TryCast<JumpVertex>()) {
                asJump->SetPredecessorId(j);
            } else if (JumpLoopVertex *asJumpLoop = pred->GetControlVertex()->TryCast<JumpLoopVertex>()) {
                asJumpLoop->SetPredecessorId(j);
            } else {
                ASSERT(!"Predecessor of a merge point must end with unconditional jump!");
            }
        }
    }
}

void GraphBuilderNew::ProcessBasicBlock(uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    if (bcBlock->IsCatchBlockHeader()) {
        LOG_COMPILER(WARN) << "Try-catch is unimplemented. Skipped.";
    }
    BCFrameState curFrameState =
        bcBlock->IsLoopHeader() ? MakeMergedFrameStateForLoopHeader(rpoIndex) : MakeMergedFrameState(rpoIndex);
    for (uint32_t i = bcBlock->startBcIndex; i <= bcBlock->endBcIndex; i++) {
        const BytecodeInfo *bcInfo = preproc_->GetBytecode(i);
        VisitBytecode(rpoIndex, &curFrameState, bcInfo);
    }
    if (bcBlock->IsSynthetic()) {
        // Edge-split block, etc.
        BB *target = blocks_[bcBlock->jumpBlock->rpoIndex];
        if (bcBlock->IsEndOfLoop()) {
            NewControlVertex<JumpLoopVertex>(blocks_[rpoIndex], {}, target);
        } else {
            NewControlVertex<JumpVertex>(blocks_[rpoIndex], {}, target);
        }
    } else if (bcBlock->fallthroughBlock != nullptr && bcBlock->jumpBlock == nullptr) {
        // Fallthrough block
        ASSERT(!bcBlock->IsEndOfLoop());
        NewControlVertex<JumpVertex>(blocks_[rpoIndex], {}, blocks_[bcBlock->fallthroughBlock->rpoIndex]);
    }
    if (bcBlock->IsEndOfLoop()) {
        WriteBackFrameStateToLoopHeader(curFrameState, rpoIndex);
    }
    frameStates_[rpoIndex] = CondensedBCFrameState(
        std::move(curFrameState), analysis_->GetLiveOut(rpoIndex), GetChunk());
}

BCFrameState GraphBuilderNew::MakeMergedFrameState(uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    uint32_t numPreds = static_cast<uint32_t>(bcBlock->jumpPredecessors.size());

    BCFrameState curFrameState(GetNumVRegs(), GetChunk());
    for (uint32_t j = 0; j < numPreds; j++) {
        MergeFrameState(curFrameState, rpoIndex, j);
    }
    return curFrameState;
}

BCFrameState GraphBuilderNew::MakeMergedFrameStateForLoopHeader(uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(blockInfo->jumpPredecessors.size() == 2);  // 2 : One is entry, the other is loop-back

    kungfu::BitSet phiCandidates = analysis_->GetLiveIn(rpoIndex);
    // Note: We need better policy to initialize Phi nodes for loop header in further implementation
    BCFrameState curFrameState(GetNumVRegs(), GetChunk());
    for (uint32_t vregIndex = 0, n = GetNumVRegs(); vregIndex < n; vregIndex++) {
        if (phiCandidates.TestBit(vregIndex)) {
            // 2 : One is entry, the other is loop-back
            curFrameState.Set(vregIndex, NewPhiVertex(blocks_[rpoIndex], 2, vregIndex));
        }
    }
    MergeFrameState(curFrameState, rpoIndex, 0);  // 0 : The first predecessor which is loop entry
    return curFrameState;
}

void GraphBuilderNew::WriteBackFrameStateToLoopHeader(BCFrameState &current, uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(blockInfo->jumpBlock != nullptr);
    ASSERT(blockInfo->jumpBlock->jumpPredecessors.size() == 2);  // 2 : One is entry, the other is loop-back
    ASSERT(blockInfo->jumpBlock->loopBackBlock == blockInfo);

    uint32_t headerIndex = blockInfo->jumpBlock->rpoIndex;
    for (PhiVertex *phi : blocks_[headerIndex]->GetPhis()) {
        ASSERT(phi->GetInputCount() == 2);  // 2 : One is entry, the other is loop-back
        ValueVertex *fromCurrent = current.Get(phi->GetOwner().GetId());
        phi->SetInput(1, fromCurrent != nullptr ? fromCurrent : undefinedValue_);
    }
}

void GraphBuilderNew::MergeFrameState(BCFrameState &dest, uint32_t rpoIndex, uint32_t predecessorIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    uint32_t numPreds = bcBlock->jumpPredecessors.size();
    uint32_t predRpoIndex = bcBlock->jumpPredecessors[predecessorIndex]->rpoIndex;

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
            phi->SetInput(predecessorIndex, fromPred);
            return;
        }
        phi = NewPhiVertex(blocks_[rpoIndex], numPreds, vregIndex);
        for (uint32_t k = 0; k < predecessorIndex; k++) {
            phi->SetInput(k, cur);
        }
        phi->SetInput(predecessorIndex, fromPred);
        dest.Set(vregIndex, phi);
    });
}

template <class VertexT, class... Args>
VertexT *GraphBuilderNew::NewVertex(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(GetChunk(), inputs, std::forward<Args>(args)...);
    return FinalizeNonControlVertex(owner, vertex);
}

template <class VertexT, class... Args>
VertexT *GraphBuilderNew::NewVertex(BB *owner, const ChunkVector<ValueVertex *> &inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(GetChunk(), inputs, std::forward<Args>(args)...);
    return FinalizeNonControlVertex(owner, vertex);
}

template <class VertexT, class... Args>
VertexT *GraphBuilderNew::NewVertexNoInput(BB *owner, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(GetChunk(), 0, std::forward<Args>(args)...);
    return FinalizeNonControlVertex(owner, vertex);
}

PhiVertex *GraphBuilderNew::NewPhiVertex(BB *owner, uint32_t numPredecessors, VRegIDType vreg)
{
    // nullptr: Old MergePointFrameState pointer. To be removed after refactoring done.
    PhiVertex *phi = PhiVertex::New(GetChunk(), numPredecessors, nullptr, VirtualRegister(vreg));
    phi->SetOwner(owner);
    owner->AddPhiVertex(phi);
    REGISTER_VERTEX_TO_LABELLER(phi);
    return phi;
}

template <class VertexT, class... Args>
VertexT *GraphBuilderNew::NewControlVertex(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(GetChunk(), inputs, std::forward<Args>(args)...);
    return FinalizeControlVertex(owner, vertex);
}

template <class VertexT>
VertexT *GraphBuilderNew::FinalizeNonControlVertex(BB *owner, VertexT *vertex)
{
    constexpr VertexProperties props = VertexT::PROPERTIES;
    // At most one of: deopt_checkpoint, eager_deopt, lazy_deopt
    static_assert(props.IsDeoptCheckpoint() + props.CanEagerDeopt() + props.CanLazyDeopt() <= 1);

    vertex->SetOwner(owner);
    owner->AddVertex(vertex);
    REGISTER_VERTEX_TO_LABELLER(vertex);
    return vertex;
}

template <class VertexT>
VertexT *GraphBuilderNew::FinalizeControlVertex(BB *owner, VertexT *vertex)
{
    constexpr VertexProperties props = VertexT::PROPERTIES;
    // Control vertices cannot have lazy deopt, throw, or write side effects
    // Note: ThrowVertex is a special case that can throw
    static_assert(!props.CanLazyDeopt() && !props.CanWrite());

    vertex->SetOwner(owner);
    owner->SetControlVertex(vertex);
    graph_->Add(owner);
    REGISTER_VERTEX_TO_LABELLER(vertex);
    return vertex;
}

ValueVertex *GraphBuilderNew::GetGlue() const
{
    return glue_;
}

ValueVertex *GraphBuilderNew::GetUndefinedValue() const
{
    return undefinedValue_;
}

struct GraphBuilderNew::BytecodeVisitor {
    void Visit()
    {
        switch (bcInfo->details.GetOpcode()) {
            case kungfu::EcmaOpcode::NOP:  // Nop: Nothing to do
                break;
            case kungfu::EcmaOpcode::CALLARG0_IMM8:
                LowerCallArg0();
                break;
            case kungfu::EcmaOpcode::CALLARG1_IMM8_V8:
                LowerCallArg1();
                break;
            case kungfu::EcmaOpcode::CALLARGS2_IMM8_V8_V8:
                LowerCallArgs2();
                break;
            case kungfu::EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
                LowerCallArgs3();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
                LowerCallThis0();
                break;
            case kungfu::EcmaOpcode::CALLTHIS0_IMM8_V8:
                LowerCallThis0();
                break;
            case kungfu::EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
                LowerCallThis1();
                break;
            case kungfu::EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
                LowerCallThis2();
                break;
            case kungfu::EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
                LowerCallThis3();
                break;
            case kungfu::EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
                LowerCallRange();
                break;
            case kungfu::EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
                LowerCallThisRange();
                break;
            case kungfu::EcmaOpcode::APPLY_IMM8_V8_V8:
                LowerCallSpread();
                break;
            case kungfu::EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
                LowerSuperCallThisRange();
                break;
            case kungfu::EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
                LowerSuperCallArrowRange();
                break;
            case kungfu::EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
                LowerSuperCallSpread();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8:
                LowerSuperCallForwardAllArgs();
                break;
            case kungfu::EcmaOpcode::GETUNMAPPEDARGS:
                LowerGetUnmappedArgs();
                break;
            case kungfu::EcmaOpcode::GETPROPITERATOR:
                LowerGetPropIterator();
                break;
            case kungfu::EcmaOpcode::CLOSEITERATOR_IMM8_V8:
            case kungfu::EcmaOpcode::CLOSEITERATOR_IMM16_V8:
                LowerCloseIterator();
                break;
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
                LowerLdaiImm32();
                break;
            case kungfu::EcmaOpcode::FLDAI_IMM64:
                LowerFldaiImm64();
                break;
            case kungfu::EcmaOpcode::LDFUNCTION:
                frameState->SetAcc(LoadParam(CALL_TARGET_PARAM_INDEX));
                break;
            case kungfu::EcmaOpcode::LDNEWTARGET:
                frameState->SetAcc(LoadParam(NEW_TARGET_PARAM_INDEX));
                break;
            case kungfu::EcmaOpcode::LDTHIS:
                frameState->SetAcc(LoadParam(THIS_OBJECT_PARAM_INDEX));
                break;
            case kungfu::EcmaOpcode::MOV_V4_V4:
            case kungfu::EcmaOpcode::MOV_V8_V8:
            case kungfu::EcmaOpcode::MOV_V16_V16:
            case kungfu::EcmaOpcode::STA_V8:
            case kungfu::EcmaOpcode::LDA_V8:
                LowerMoveValues();
                break;
            case kungfu::EcmaOpcode::INC_IMM8:
                LowerInc();
                break;
            case kungfu::EcmaOpcode::DEC_IMM8:
                LowerDec();
                break;
            case kungfu::EcmaOpcode::ADD2_IMM8_V8:
                LowerAdd2();
                break;
            case kungfu::EcmaOpcode::SUB2_IMM8_V8:
                LowerSub2();
                break;
            case kungfu::EcmaOpcode::MUL2_IMM8_V8:
                LowerMul2();
                break;
            case kungfu::EcmaOpcode::DIV2_IMM8_V8:
                LowerDiv2();
                break;
            case kungfu::EcmaOpcode::MOD2_IMM8_V8:
                LowerMod2();
                break;
            case kungfu::EcmaOpcode::EXP_IMM8_V8:
                LowerExp();
                break;
            case kungfu::EcmaOpcode::EQ_IMM8_V8:
                LowerEq();
                break;
            case kungfu::EcmaOpcode::NOTEQ_IMM8_V8:
                LowerNotEq();
                break;
            case kungfu::EcmaOpcode::LESS_IMM8_V8:
                LowerLess();
                break;
            case kungfu::EcmaOpcode::LESSEQ_IMM8_V8:
                LowerLessEq();
                break;
            case kungfu::EcmaOpcode::GREATER_IMM8_V8:
                LowerGreater();
                break;
            case kungfu::EcmaOpcode::GREATEREQ_IMM8_V8:
                LowerGreaterEq();
                break;
            case kungfu::EcmaOpcode::STRICTNOTEQ_IMM8_V8:
                LowerStrictNotEq();
                break;
            case kungfu::EcmaOpcode::STRICTEQ_IMM8_V8:
                LowerStrictEq();
                break;
            case kungfu::EcmaOpcode::TONUMBER_IMM8:
                LowerToNumber();
                break;
            case kungfu::EcmaOpcode::TONUMERIC_IMM8:
                LowerToNumeric();
                break;
            case kungfu::EcmaOpcode::NEG_IMM8:
                LowerNeg();
                break;
            case kungfu::EcmaOpcode::NOT_IMM8:
                LowerNot();
                break;
            case kungfu::EcmaOpcode::SHL2_IMM8_V8:
                LowerShl2();
                break;
            case kungfu::EcmaOpcode::SHR2_IMM8_V8:
                LowerShr2();
                break;
            case kungfu::EcmaOpcode::ASHR2_IMM8_V8:
                LowerAshr2();
                break;
            case kungfu::EcmaOpcode::AND2_IMM8_V8:
                LowerAnd2();
                break;
            case kungfu::EcmaOpcode::OR2_IMM8_V8:
                LowerOr2();
                break;
            case kungfu::EcmaOpcode::XOR2_IMM8_V8:
                LowerXor2();
                break;
            case kungfu::EcmaOpcode::CREATEITERRESULTOBJ_V8_V8:
                LowerCreateIterResultObj();
                break;
            case kungfu::EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
                LowerTryLdGlobalByName();
                break;
            case kungfu::EcmaOpcode::STGLOBALVAR_IMM16_ID16:
                LowerStGlobalVar();
                break;
            case kungfu::EcmaOpcode::GETITERATOR_IMM8:
            case kungfu::EcmaOpcode::GETITERATOR_IMM16:
                LowerGetIterator();
                break;
            case kungfu::EcmaOpcode::NEWOBJAPPLY_IMM8_V8:
            case kungfu::EcmaOpcode::NEWOBJAPPLY_IMM16_V8:
                LowerNewObjApply();
                break;
            case kungfu::EcmaOpcode::THROW_PREF_NONE:
                LowerThrow();
                break;
            case kungfu::EcmaOpcode::TYPEOF_IMM8:
            case kungfu::EcmaOpcode::TYPEOF_IMM16:
                LowerTypeOf();
                break;
            case kungfu::EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
                LowerThrowConstAssignment();
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
                LowerThrowIfNotObject();
                break;
            case kungfu::EcmaOpcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
                LowerThrowUndefinedIfHole();
                break;
            case kungfu::EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
                LowerThrowUndefinedIfHoleWithName();
                break;
            case kungfu::EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
            case kungfu::EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
                LowerThrowIfSuperNotCorrectCall();
                break;
            case kungfu::EcmaOpcode::LDSYMBOL:
                LowerLdSymbol();
                break;
            case kungfu::EcmaOpcode::LDGLOBAL:
                LowerLdGlobal();
                break;
            case kungfu::EcmaOpcode::DELOBJPROP_V8:
                LowerDelObjProp();
                break;
            case kungfu::EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
            case kungfu::EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
                LowerDefineMethod();
                break;
            case kungfu::EcmaOpcode::ISIN_IMM8_V8:
                LowerIsIn();
                break;
            case kungfu::EcmaOpcode::INSTANCEOF_IMM8_V8:
                LowerInstanceOf();
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
                LowerCreateObjectWithBuffer();
                break;
            case kungfu::EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
            case kungfu::EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
                LowerCreateObjectWithExcludedKeys();
                break;
            case kungfu::EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
            case kungfu::EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
                LowerCreateArrayWithBuffer();
                break;
            case kungfu::EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
            case kungfu::EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
                LowerCreateRegExpWithLiteral();
                break;
            case kungfu::EcmaOpcode::STMODULEVAR_IMM8:
            case kungfu::EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
                LowerStModuleVar();
                break;
            case kungfu::EcmaOpcode::GETTEMPLATEOBJECT_IMM8:
            case kungfu::EcmaOpcode::GETTEMPLATEOBJECT_IMM16:
                LowerGetTemplateObject();
                break;
            case kungfu::EcmaOpcode::SETOBJECTWITHPROTO_IMM8_V8:
            case kungfu::EcmaOpcode::SETOBJECTWITHPROTO_IMM16_V8:
                LowerSetObjectWithProto();
                break;
            case kungfu::EcmaOpcode::LDBIGINT_ID16:
                LowerLoadBigInt();
                break;
            case kungfu::EcmaOpcode::DYNAMICIMPORT:
                LowerDynamicImport();
                break;
            case kungfu::EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
            case kungfu::EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
            case kungfu::EcmaOpcode::CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8:
            case kungfu::EcmaOpcode::CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16:
                LowerLdExternalModuleVar();
                break;
            case kungfu::EcmaOpcode::GETMODULENAMESPACE_IMM8:
            case kungfu::EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
                LowerGetModuleNamespace();
                break;
            case kungfu::EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
            case kungfu::EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
            case kungfu::EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
                LowerNewObjRange();
                break;
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
            case kungfu::EcmaOpcode::ISTRUE:
            case kungfu::EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
                LowerIsTrueOrFalse(true);
                break;
            case kungfu::EcmaOpcode::ISFALSE:
            case kungfu::EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
                LowerIsTrueOrFalse(false);
                break;
            case kungfu::EcmaOpcode::GETNEXTPROPNAME_V8:
                LowerGetNextPropName();
                break;
            case kungfu::EcmaOpcode::COPYDATAPROPERTIES_V8:
                LowerCopyDataProperties();
                break;
            case kungfu::EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
                LowerLoadObjByName();
                break;
            case kungfu::EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
                LowerStoreObjByName();
                break;
            case kungfu::EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
            case kungfu::EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
            case kungfu::EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
                LowerLdObjByIndex();
                break;
            case kungfu::EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
            case kungfu::EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
            case kungfu::EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
                LowerStObjByIndex();
                break;
            case kungfu::EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
            case kungfu::EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
                LowerLoadObjByValue();
                break;
            case kungfu::EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
                LowerStoreObjByValue();
                break;
            case kungfu::EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STOWNBYVALUE_IMM16_V8_V8:
                LowerStOwnByValue();
                break;
            case kungfu::EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
            case kungfu::EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
            case kungfu::EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
                LowerStOwnByIndex();
                break;
            case kungfu::EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
                LowerStOwnByName();
                break;
            case kungfu::EcmaOpcode::LDTHISBYVALUE_IMM8:
            case kungfu::EcmaOpcode::LDTHISBYVALUE_IMM16:
                LowerLdThisByValue();
                break;
            case kungfu::EcmaOpcode::STTHISBYVALUE_IMM8_V8:
            case kungfu::EcmaOpcode::STTHISBYVALUE_IMM16_V8:
                LowerStThisByValue();
                break;
            case kungfu::EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
                LowerLdThisByName();
                break;
            case kungfu::EcmaOpcode::STTHISBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::STTHISBYNAME_IMM16_ID16:
                LowerStThisByName();
                break;
            case kungfu::EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
            case kungfu::EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
                LowerLdSuperByValue();
                break;
            case kungfu::EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8:
            case kungfu::EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8:
                LowerStSuperByValue();
                break;
            case kungfu::EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
                LowerLdSuperByName();
                break;
            case kungfu::EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8:
                LowerStSuperByName();
                break;
            case kungfu::EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16:
                LowerTryStGlobalByName();
                break;
            case kungfu::EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
                LowerStConstToGlobalRecord(true);
                break;
            case kungfu::EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16:
                LowerStConstToGlobalRecord(false);
                break;
            case kungfu::EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
            case kungfu::EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
                LowerStOwnByValueWithNameSet();
                break;
            case kungfu::EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
                LowerStOwnByNameWithNameSet();
                break;
            case kungfu::EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16:
                LowerLdPrivateProperty();
                break;
            case kungfu::EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8:
                LowerStPrivateProperty();
                break;
            case kungfu::EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
                LowerLdGlobalVar();
                break;
            case kungfu::EcmaOpcode::NEWLEXENV_IMM8:
            case kungfu::EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
                LowerNewLexicalEnv();
                break;
            case kungfu::EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
            case kungfu::EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
                LowerNewLexicalEnvWithName();
                break;
            case kungfu::EcmaOpcode::POPLEXENV:
                LowerPopLexicalEnv();
                break;
            case kungfu::EcmaOpcode::LDLEXVAR_IMM4_IMM4:
            case kungfu::EcmaOpcode::LDLEXVAR_IMM8_IMM8:
            case kungfu::EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
                LowerLdLexVar();
                break;
            case kungfu::EcmaOpcode::STLEXVAR_IMM4_IMM4:
            case kungfu::EcmaOpcode::STLEXVAR_IMM8_IMM8:
            case kungfu::EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
                LowerStLexVar();
                break;
            case kungfu::EcmaOpcode::STARRAYSPREAD_V8_V8:
                LowerStoreArraySpread();
                break;
            case kungfu::EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
                LowerDefineGetterSetterByValue();
                break;
            case kungfu::EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
            case kungfu::EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
                LowerDefineClassWithBuffer();
                break;
            case kungfu::EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
            case kungfu::EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
                LowerDefineFunc();
                break;
            case kungfu::EcmaOpcode::COPYRESTARGS_IMM8:
            case kungfu::EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
                LowerCopyRestArgs();
                break;
            case kungfu::EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16:
                LowerLdPatchVar();
                break;
            case kungfu::EcmaOpcode::WIDE_STPATCHVAR_PREF_IMM16:
                LowerStPatchVar();
                break;
            case kungfu::EcmaOpcode::LDLOCALMODULEVAR_IMM8:
            case kungfu::EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
                LowerLdLocalModuleVar();
                break;
            case kungfu::EcmaOpcode::TESTIN_IMM8_IMM16_IMM16:
                LowerTestIn();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE:
                LowerNotifyConcurrentResult();
                break;
            case kungfu::EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
                LowerDefinePropertyByName();
                break;
            case kungfu::EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
                LowerDefineFieldByName();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8:
                LowerDefineFieldByValue();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8:
                LowerDefineFieldByIndex();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_TOPROPERTYKEY_PREF_NONE:
                LowerToPropertyKey();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16:
                LowerCreatePrivateProperty();
                break;
            case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8:
                LowerDefinePrivateProperty();
                break;
            case kungfu::EcmaOpcode::LDA_STR_ID16:
                LowerLoadString();
                break;
            case kungfu::EcmaOpcode::JMP_IMM8:
            case kungfu::EcmaOpcode::JMP_IMM16:
            case kungfu::EcmaOpcode::JMP_IMM32:
                LowerJumpConstant();
                break;
            case kungfu::EcmaOpcode::RETURNUNDEFINED:
                self->NewControlVertex<ReturnVertex>(CurrentBlock(), {self->GetUndefinedValue()});
                break;
            case kungfu::EcmaOpcode::RETURN:
                self->NewControlVertex<ReturnVertex>(CurrentBlock(), {frameState->GetAcc()});
                break;
            default:
                UNREACHABLE();
        }
    }

    void LowerCallArg0()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func}, CommonStubCSigns::CallArg0Stub);
        frameState->SetAcc(result);
    }

    void LowerCallArg1()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *a0Value = LoadRegister(0);
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, a0Value}, CommonStubCSigns::CallArg1Stub);
        frameState->SetAcc(result);
    }

    void LowerCallArgs2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *a0Value = LoadRegister(0);
        ValueVertex *a1Value = LoadRegister(1);
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, a0Value, a1Value}, CommonStubCSigns::CallArg2Stub);
        frameState->SetAcc(result);
    }

    void LowerCallArgs3()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *a0Value = LoadRegister(0);
        ValueVertex *a1Value = LoadRegister(1);
        ValueVertex *a2Value = LoadRegister(2);  // 2: third argument register index
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, a0Value, a1Value, a2Value}, CommonStubCSigns::CallArg3Stub);
        frameState->SetAcc(result);
    }

    void LowerCallThis0()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj}, CommonStubCSigns::CallThis0Stub);
        frameState->SetAcc(result);
    }

    void LowerCallThis1()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *a0Value = LoadRegister(1);
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj, a0Value}, CommonStubCSigns::CallThis1Stub);
        frameState->SetAcc(result);
    }

    void LowerCallThis2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *a0Value = LoadRegister(1);
        ValueVertex *a1Value = LoadRegister(2);  // 2: second argument register index
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj, a0Value, a1Value}, CommonStubCSigns::CallThis2Stub);
        frameState->SetAcc(result);
    }

    void LowerCallThis3()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *a0Value = LoadRegister(1);
        ValueVertex *a1Value = LoadRegister(2);  // 2: second argument register index
        ValueVertex *a2Value = LoadRegister(3);  // 3: third argument register index
        ValueVertex *func = frameState->GetAcc();

        ValueVertex *result = CommonStubCall(
            {glue, func, thisObj, a0Value, a1Value, a2Value}, CommonStubCSigns::CallThis3Stub);
        frameState->SetAcc(result);
    }

    void LowerCallRange()
    {
        uint32_t inputSize = bcInfo->details.inputs.size();

        ValueVertex *func = frameState->GetAcc();
        ValueVertex *taggedInputSize = TaggedConstantFromInt32(static_cast<int>(inputSize));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(taggedInputSize, inputSize);

        ValueVertex *result = RuntimeCall({func, taggedArray, taggedInputSize}, RTSTUB_ID(CallRange));
        frameState->SetAcc(result);
    }

    void LowerCallThisRange()
    {
        // -1 : Skips the receiver
        uint32_t argc = bcInfo->details.inputs.size() - 1;

        ValueVertex *func = frameState->GetAcc();
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *taggedArgc = TaggedConstantFromInt32(static_cast<int>(argc));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(taggedArgc, argc, 1);

        ValueVertex *result = RuntimeCall({thisObj, func, taggedArray, taggedArgc}, RTSTUB_ID(CallThisRange));
        frameState->SetAcc(result);
    }

    void LowerCallSpread()
    {
        ValueVertex *func = frameState->GetAcc();
        ValueVertex *thisArg = LoadRegister(0);
        ValueVertex *argsArray = LoadRegister(1);
        frameState->SetAcc(RuntimeCall({func, thisArg, argsArray}, RTSTUB_ID(CallSpread)));
    }

    void LowerSuperCallThisRange()
    {
        uint32_t inputSize = bcInfo->details.inputs.size();

        ValueVertex *thisFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedInputSize = TaggedConstantFromInt32(static_cast<int>(inputSize));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(taggedInputSize, inputSize);

        frameState->SetAcc(RuntimeCall({thisFunc, newTarget, taggedArray, taggedInputSize}, RTSTUB_ID(OptSuperCall)));
    }

    void LowerSuperCallArrowRange()
    {
        uint32_t argc = bcInfo->details.inputs.size() - 1;
        ValueVertex *func = LoadRegister(argc);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedArgc = TaggedConstantFromInt32(static_cast<int>(argc));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(taggedArgc, argc);

        frameState->SetAcc(RuntimeCall({func, newTarget, taggedArray, taggedArgc}, RTSTUB_ID(OptSuperCall)));
    }

    void LowerSuperCallSpread()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *globalEnv = GlobalEnv();
        ValueVertex *array = LoadRegister(0);
        ValueVertex *func = frameState->GetAcc();
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);

        ValueVertex *argsArray = CommonStubCall({glue, array, globalEnv}, CommonStubCSigns::GetCallSpreadArgs);
        frameState->SetAcc(RuntimeCall({func, newTarget, argsArray}, RTSTUB_ID(OptSuperCallSpread)));
    }

    void LowerSuperCallForwardAllArgs()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *func = LoadRegister(0);
        ValueVertex *superFunc = CommonStubCall({glue, func}, CommonStubCSigns::GetPrototype);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedActualArgc = TaggedActualArgc();

        frameState->SetAcc(RuntimeCall({
            superFunc, newTarget, taggedActualArgc}, RTSTUB_ID(OptSuperCallForwardAllArgs)));
    }

    void LowerGetUnmappedArgs()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *argv = self->graph_->GetIntPtrConstant(0);
        ValueVertex *numArgs = ActualArgc();
        ValueVertex *argvTaggedArray = self->GetUndefinedValue();
        ValueVertex *globalEnv = GlobalEnv();

        frameState->SetAcc(CommonStubCall(
            {glue, argv, numArgs, argvTaggedArray, globalEnv}, CommonStubCSigns::GetUnmappedArgs));
    }

    void LowerGetPropIterator()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *object = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, object, globalEnv}, CommonStubCSigns::Getpropiterator));
    }

    void LowerCloseIterator()
    {
        ValueVertex *iterator = LoadRegister(0);
        frameState->SetAcc(RuntimeCall({iterator}, RTSTUB_ID(CloseIterator)));
    }

    void LowerLdTaggedConstant(JSTaggedType taggedValue)
    {
        frameState->SetAcc(self->graph_->GetTaggedConstant(taggedValue));
    }

    void LowerLdRootConstant(RootConstantVertex::RootIndex index)
    {
        frameState->SetAcc(self->graph_->GetRootConstant(index));
    }

    void LowerLdaiImm32()
    {
        frameState->SetAcc(TaggedConstantFromInt32(static_cast<int>(GetImmediate(0))));
    }

    void LowerFldaiImm64()
    {
        JSTaggedType taggedValue = JSTaggedValue(base::bit_cast<double>(GetImmediate(0))).GetRawData();
        frameState->SetAcc(self->graph_->GetTaggedConstant(taggedValue));
    }

    void LowerMoveValues()
    {
        const auto &info = bcInfo->details;
        ValueVertex *vertex = nullptr;
        // Get input value
        if (info.AccIn()) {
            vertex = frameState->GetAcc();
        } else {
            ASSERT(!info.inputs.empty());
            vertex = LoadRegister(0);
        }
        // Set output value
        if (info.AccOut()) {
            frameState->SetAcc(vertex);
        } else {
            ASSERT(!info.vregOut.empty());
            frameState->Set(info.vregOut[0], vertex);
        }
    }

    void LowerInc()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = frameState->GetAcc();
        frameState->SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Inc));
    }

    void LowerDec()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = frameState->GetAcc();
        frameState->SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Dec));
    }

    void LowerAdd2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Add));
    }

    void LowerSub2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Sub));
    }

    void LowerMul2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Mul));
    }

    void LowerDiv2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Div));
    }

    void LowerMod2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Mod));
    }

    void LowerExp()
    {
        ValueVertex *left = LoadRegister(0);
        ValueVertex *right = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({left, right}, RTSTUB_ID(Exp)));
    }

    void LowerEq()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Equal));
    }

    void LowerNotEq()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::NotEqual));
    }

    void LowerLess()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Less));
    }

    void LowerLessEq()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::LessEq));
    }

    void LowerGreater()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Greater));
    }

    void LowerGreaterEq()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::GreaterEq));
    }

    void LowerStrictEq()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::StrictEqual));
    }

    void LowerStrictNotEq()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::StrictNotEqual));
    }

    void LowerToNumber()
    {
        ValueVertex *value = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({value}, RTSTUB_ID(ToNumber)));
    }

    void LowerToNumeric()
    {
        ValueVertex *value = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({value}, RTSTUB_ID(ToNumeric)));
    }

    void LowerNeg()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = frameState->GetAcc();
        frameState->SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Neg));
    }

    void LowerNot()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = frameState->GetAcc();
        frameState->SetAcc(CommonStubCall({glue, x}, CommonStubCSigns::Not));
    }

    void LowerShl2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Shl));
    }

    void LowerShr2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Shr));
    }

    void LowerAshr2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Ashr));
    }

    void LowerAnd2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::And));
    }

    void LowerOr2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Or));
    }

    void LowerXor2()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *x = LoadRegister(0);
        ValueVertex *y = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Xor));
    }

    void LowerCreateIterResultObj()
    {
        ValueVertex *value = LoadRegister(0);
        ValueVertex *done = LoadRegister(1);
        frameState->SetAcc(RuntimeCall({value, done}, RTSTUB_ID(CreateIterResultObj)));
    }

    void LowerTryLdGlobalByName()
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({id, globalEnv}, CommonStubCSigns::TryLdGlobalByName));
    }

    void LowerStGlobalVar()
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCallWithIC({id, value, globalEnv}, CommonStubCSigns::StGlobalVar);
    }

    void LowerGetIterator()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *obj = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, obj, globalEnv}, CommonStubCSigns::GetIterator));
    }

    void LowerNewObjApply()
    {
        ValueVertex *target = LoadRegister(0);
        ValueVertex *args = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({target, args}, RTSTUB_ID(NewObjApply)));
    }

    void LowerTypeOf()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *obj = frameState->GetAcc();
        frameState->SetAcc(CommonStubCall({glue, obj}, CommonStubCSigns::TypeOf));
    }

    void LowerThrow()
    {
        constexpr bool HAS_INPUT = true;
        ValueVertex *exception = frameState->GetAcc();
        self->NewControlVertex<ThrowVertex>(
            CurrentBlock(), {exception}, RTSTUB_ID(Throw), HAS_INPUT);
    }

    void LowerThrowConstAssignment()
    {
        constexpr bool HAS_INPUT = true;
        ValueVertex *value = LoadRegister(0);
        self->NewControlVertex<ThrowVertex>(
            CurrentBlock(), {value}, RTSTUB_ID(ThrowConstAssignment), HAS_INPUT);
    }

    void LowerThrowNotExists()
    {
        constexpr bool HAS_INPUT = false;
        self->NewControlVertex<ThrowVertex>(
            CurrentBlock(), {self->undefinedValue_}, RTSTUB_ID(ThrowThrowNotExists), HAS_INPUT);
    }

    void LowerThrowPatternNonCoercible()
    {
        constexpr bool HAS_INPUT = false;
        self->NewControlVertex<ThrowVertex>(
            CurrentBlock(), {self->undefinedValue_}, RTSTUB_ID(ThrowPatternNonCoercible), HAS_INPUT);
    }

    void LowerThrowDeleteSuperProperty()
    {
        constexpr bool HAS_INPUT = false;
        self->NewControlVertex<ThrowVertex>(
            CurrentBlock(), {self->undefinedValue_}, RTSTUB_ID(ThrowDeleteSuperProperty), HAS_INPUT);
    }

    void LowerThrowIfNotObject()
    {
        LOG_COMPILER(WARN) << "Unimplemented: LowerThrowIfNotObject";
    }

    void LowerThrowUndefinedIfHole()
    {
        LOG_COMPILER(WARN) << "Unimplemented: LowerThrowUndefinedIfHole";
    }

    void LowerThrowUndefinedIfHoleWithName()
    {
        LOG_COMPILER(WARN) << "Unimplemented: LowerThrowUndefinedIfHoleWithName";
    }

    void LowerThrowIfSuperNotCorrectCall()
    {
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *thisValue = frameState->GetAcc();

        self->NewVertex<ThrowIfSuperNotCorrectCallVertex>(
            CurrentBlock(), {index, thisValue}, RTSTUB_ID(ThrowIfSuperNotCorrectCall));
    }

    void LowerLdSymbol()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::SYMBOL_FUNCTION_INDEX * JSTaggedValue::TaggedTypeSize());

        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {globalEnv}, offset));
    }

    void LowerLdGlobal()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::JS_GLOBAL_OBJECT_INDEX * JSTaggedValue::TaggedTypeSize());

        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {globalEnv}, offset));
    }

    void LowerDelObjProp()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *object = LoadRegister(0);
        ValueVertex *prop = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();

        frameState->SetAcc(CommonStubCall(
            {glue, object, prop, globalEnv}, CommonStubCSigns::DeleteObjectProperty));
    }

    void LowerDefineMethod()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *taggedMethodId = TaggedConstantFromInt32(static_cast<int>(GetConstDataId(0)));
        ValueVertex *length = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        ValueVertex *env = LoadRegister(2);  // 2: env register index
        ValueVertex *homeObject = frameState->GetAcc();
        ValueVertex *module = ModuleFromFunction();
        ValueVertex *method = MethodFromConstPool(taggedMethodId);

#if ECMASCRIPT_ENABLE_IC
        // 3 : slotId operand index
        ValueVertex *slotId = TaggedConstantFromInt32(static_cast<int>(GetICSlotId(3)));
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);

        frameState->SetAcc(RuntimeCall(
            {method, homeObject, length, env, module, slotId, jsFunc}, RTSTUB_ID(DefineMethod)));
#else
        frameState->SetAcc(RuntimeCall(
            {method, homeObject, length, env, module}, RTSTUB_ID(DefineMethod)));
#endif
    }

    void LowerIsIn()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *prop = LoadRegister(0);
        ValueVertex *obj = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, prop, obj, globalEnv}, CommonStubCSigns::IsIn));
    }

    void LowerInstanceOf()
    {
        ValueVertex *object = LoadRegister(1);
        ValueVertex *target = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({object, target, globalEnv}, CommonStubCSigns::Instanceof));
    }

    void LowerCreateEmptyObject()
    {
        frameState->SetAcc(RuntimeCall({}, RTSTUB_ID(CreateEmptyObject)));
    }

    void LowerCreateEmptyArray()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, globalEnv}, CommonStubCSigns::CreateEmptyArray));
    }

    void LowerCreateObjectWithBuffer()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *index = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *obj = ObjectFromConstPool(index);
        ValueVertex *lexEnv = LoadRegister(1);
        frameState->SetAcc(CommonStubCall({glue, obj, lexEnv}, CommonStubCSigns::CreateObjectHavingMethod));
    }

    void LowerCreateObjectWithExcludedKeys()
    {
        uint32_t inputSize = bcInfo->details.inputs.size();
        ChunkVector<ValueVertex *> args(self->GetChunk());
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(idx));
        }
        frameState->SetAcc(RuntimeCall(args, RTSTUB_ID(OptCreateObjectWithExcludedKeys)));
    }

    void LowerCreateArrayWithBuffer()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *index = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *slotId = self->graph_->GetInt32Constant(static_cast<int>(GetICSlotId(1)));
        ValueVertex *globalEnv = GlobalEnv();

        frameState->SetAcc(CommonStubCall(
            {glue, index, jsFunc, slotId, globalEnv}, CommonStubCSigns::CreateArrayWithBuffer));
    }

    void LowerCreateRegExpWithLiteral()
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *pattern = StringFromConstPool(stringId);
        ValueVertex *flags = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));

        frameState->SetAcc(RuntimeCall({pattern, flags}, RTSTUB_ID(CreateRegExpWithLiteral)));
    }

    void LowerStModuleVar()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *value = frameState->GetAcc();
        RuntimeCall({index, value, jsFunc}, RTSTUB_ID(StModuleVarByIndexOnJSFunc));
    }

    void LowerGetTemplateObject()
    {
        ValueVertex *value = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({value}, RTSTUB_ID(GetTemplateObject)));
    }

    void LowerSetObjectWithProto()
    {
        ValueVertex *proto = LoadRegister(0);
        ValueVertex *obj = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({proto, obj}, RTSTUB_ID(SetObjectWithProto)));
    }

    void LowerLoadBigInt()
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *numberBigInt = StringFromConstPool(stringId);
        frameState->SetAcc(RuntimeCall({numberBigInt}, RTSTUB_ID(LdBigInt)));
    }

    void LowerDynamicImport()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *specifier = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({specifier, jsFunc}, RTSTUB_ID(DynamicImport)));
    }

    void LowerLdExternalModuleVar()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        frameState->SetAcc(RuntimeCall({index, jsFunc}, RTSTUB_ID(LdExternalModuleVarByIndexOnJSFunc)));
    }

    void LowerGetModuleNamespace()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        frameState->SetAcc(RuntimeCall({index, jsFunc}, RTSTUB_ID(GetModuleNamespaceByIndexOnJSFunc)));
    }

    void LowerNewObjRange()
    {
        uint32_t inputSize = bcInfo->details.inputs.size();
        ChunkVector<ValueVertex *> args(self->GetChunk());
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(idx));
        }
        frameState->SetAcc(RuntimeCall(args, RTSTUB_ID(OptNewObjRange)));
    }

    void LowerJumpIfZero()
    {
        const BasicBlockInfo *blockInfo = self->preproc_->GetBasicBlockByRPO(rpoIndex);
        ASSERT(blockInfo->jumpBlock != nullptr);
        ASSERT(blockInfo->fallthroughBlock != nullptr);
        uint32_t jumpIndex = blockInfo->jumpBlock->rpoIndex;
        uint32_t fallthroughIndex = blockInfo->fallthroughBlock->rpoIndex;

        ValueVertex *acc = frameState->GetAcc();
        self->NewControlVertex<BranchIfTrueVertex>(
            self->blocks_[rpoIndex],
            {acc},
            self->blocks_[fallthroughIndex],
            self->blocks_[jumpIndex]);
    }

    void LowerJumpIfNonZero()
    {
        const BasicBlockInfo *blockInfo = self->preproc_->GetBasicBlockByRPO(rpoIndex);
        ASSERT(blockInfo->jumpBlock != nullptr);
        ASSERT(blockInfo->fallthroughBlock != nullptr);
        uint32_t jumpIndex = blockInfo->jumpBlock->rpoIndex;
        uint32_t fallthroughIndex = blockInfo->fallthroughBlock->rpoIndex;

        ValueVertex *acc = frameState->GetAcc();
        self->NewControlVertex<BranchIfTrueVertex>(
            self->blocks_[rpoIndex],
            {acc},
            self->blocks_[jumpIndex],
            self->blocks_[fallthroughIndex]);
    }

    void LowerIsTrueOrFalse(bool isTrue)
    {
        CommonStubID id = isTrue ? CommonStubCSigns::ToBooleanTrue : CommonStubCSigns::ToBooleanFalse;
        ValueVertex *glue = self->GetGlue();
        ValueVertex *value = frameState->GetAcc();
        frameState->SetAcc(CommonStubCall({glue, value}, id));
    }

    void LowerGetNextPropName()
    {
        ValueVertex *iterator = LoadRegister(0);
        frameState->SetAcc(RuntimeCall({iterator}, RTSTUB_ID(GetNextPropNameSlowpath)));
    }

    void LowerCopyDataProperties()
    {
        ValueVertex *target = LoadRegister(0);
        ValueVertex *source = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({target, source}, RTSTUB_ID(CopyDataProperties)));
    }

    void LowerLoadObjByName()
    {
        ValueVertex *receiver = frameState->GetAcc();
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({receiver, id, globalEnv}, CommonStubCSigns::GetPropertyByName));
    }

    void LowerStoreObjByName()
    {
        ValueVertex *receiver = LoadRegister(2);  // 2: receiver register index
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCallWithIC({receiver, id, value, globalEnv}, CommonStubCSigns::SetPropertyByName);
    }

    void LowerLoadObjByValue()
    {
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *key = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({receiver, key, globalEnv}, CommonStubCSigns::GetPropertyByValue));
    }

    void LowerStoreObjByValue()
    {
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *key = LoadRegister(2);  // 2: key register index
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCallWithIC({receiver, key, value, globalEnv}, CommonStubCSigns::SetPropertyByValue);
    }

    void LowerLdObjByIndex()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *index = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(0)));
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCall({glue, receiver, index, globalEnv}, CommonStubCSigns::LdObjByIndex));
    }

    void LowerStObjByIndex()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(0);
        ValueVertex *index = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(1)));
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, index, value, globalEnv}, CommonStubCSigns::StObjByIndex);
    }

    void LowerStOwnByValue()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(0);
        ValueVertex *key = LoadRegister(1);
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, key, value, globalEnv}, CommonStubCSigns::StOwnByValue);
    }

    void LowerStOwnByIndex()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(0);
        ValueVertex *index = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(1)));
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, index, value, globalEnv}, CommonStubCSigns::StOwnByIndex);
    }

    void LowerStOwnByName()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *accValue = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, propKey, accValue, globalEnv}, CommonStubCSigns::StOwnByName);
    }

    void LowerLdThisByValue()
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({receiver, key, globalEnv}, CommonStubCSigns::GetPropertyByValue));
    }

    void LowerStThisByValue()
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = LoadRegister(1);
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCallWithIC({receiver, key, value, globalEnv}, CommonStubCSigns::SetPropertyByValue);
    }

    void LowerLdThisByName()
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({receiver, id, globalEnv}, CommonStubCSigns::GetPropertyByName));
    }

    void LowerStThisByName()
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCallWithIC({receiver, id, value, globalEnv}, CommonStubCSigns::SetPropertyByName);
    }

    void LowerLdSuperByValue()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *propKey = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({thisObj, propKey, jsFunc}, RTSTUB_ID(OptLdSuperByValue)));
    }

    void LowerStSuperByValue()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(0);
        ValueVertex *propKey = LoadRegister(1);
        ValueVertex *value = frameState->GetAcc();
        RuntimeCall({thisObj, propKey, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
    }

    void LowerLdSuperByName()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = frameState->GetAcc();
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *prop = StringFromConstPool(stringId);
        frameState->SetAcc(RuntimeCall({thisObj, prop, jsFunc}, RTSTUB_ID(OptLdSuperByValue)));
    }

    void LowerStSuperByName()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(1);
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *prop = StringFromConstPool(stringId);
        RuntimeCall({thisObj, prop, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
    }

    void LowerTryStGlobalByName()
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCallWithIC({id, value, globalEnv}, CommonStubCSigns::TryStGlobalByName);
    }

    void LowerStConstToGlobalRecord(bool isConst)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *isConstGate = isConst
            ? self->graph_->GetTaggedConstant(JSTaggedValue::True().GetRawData())
            : self->graph_->GetTaggedConstant(JSTaggedValue::False().GetRawData());
        RuntimeCall({propKey, value, isConstGate}, RTSTUB_ID(StGlobalRecord));
    }

    void LowerStOwnByValueWithNameSet()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(0);
        ValueVertex *propKey = LoadRegister(1);
        ValueVertex *accValue = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, propKey, accValue, globalEnv}, CommonStubCSigns::StOwnByValueWithNameSet);
    }

    void LowerStOwnByNameWithNameSet()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *accValue = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, propKey, accValue, globalEnv}, CommonStubCSigns::StOwnByNameWithNameSet);
    }

    void LowerLdPrivateProperty()
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        ValueVertex *slotIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(2)));
        ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
        ValueVertex *obj = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(LdPrivateProperty)));
    }

    void LowerStPrivateProperty()
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        ValueVertex *slotIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(2)));
        ValueVertex *obj = LoadRegister(3);  // 3: obj register index
        ValueVertex *lexicalEnv = LoadRegister(4);  // 4: lexicalEnv register index
        ValueVertex *value = frameState->GetAcc();
        RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(StPrivateProperty));
    }

    void LowerLdGlobalVar()
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
        ValueVertex *globalEnv = GlobalEnv();
        frameState->SetAcc(CommonStubCallWithIC({id, globalEnv}, CommonStubCSigns::LdGlobalVar));
    }

    void LowerNewLexicalEnv()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *parent = LoadRegister(1);
        ValueVertex *scope = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(0)));
        ValueVertex *newEnv = CommonStubCall({glue, parent, scope}, CommonStubCSigns::NewLexicalEnv);
        frameState->SetAcc(newEnv);
        frameState->SetEnv(newEnv);
    }

    void LowerNewLexicalEnvWithName()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *level = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *slotId = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        // 2: env register index
        ValueVertex *newEnv = RuntimeCall(
            {level, slotId, LoadRegister(2), jsFunc}, RTSTUB_ID(OptNewLexicalEnvWithName));
        frameState->SetAcc(newEnv);
        frameState->SetEnv(newEnv);
    }

    void LowerPopLexicalEnv()
    {
        ValueVertex *currentEnv = LoadRegister(0);
        ValueVertex *parentEnv = GetValueFromTaggedArray(currentEnv, LexicalEnv::PARENT_ENV_INDEX);
        frameState->SetAcc(parentEnv);
        frameState->SetEnv(parentEnv);
    }

    void LowerLdLexVar()
    {
        ValueVertex *level = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(0)));
        ValueVertex *slot = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(1)));
        ValueVertex *lexicalEnv = LoadRegister(2);  // 2: lexicalEnv register index
        ValueVertex *glue = self->GetGlue();
        frameState->SetAcc(CommonStubCall({glue, level, slot, lexicalEnv}, CommonStubCSigns::LdLexVar));
    }

    void LowerStLexVar()
    {
        ValueVertex *level = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(0)));
        ValueVertex *slot = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(1)));
        ValueVertex *lexicalEnv = LoadRegister(2);  // 2: lexicalEnv register index
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *glue = self->GetGlue();
        CommonStubCall({glue, level, slot, lexicalEnv, value}, CommonStubCSigns::StLexVar);
    }

    void LowerStoreArraySpread()
    {
        ValueVertex *array = LoadRegister(0);
        ValueVertex *index = LoadRegister(1);
        ValueVertex *value = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({array, index, value}, RTSTUB_ID(StArraySpread)));
    }

    void LowerDefineGetterSetterByValue()
    {
        ValueVertex *obj = LoadRegister(0);
        ValueVertex *prop = LoadRegister(1);
        ValueVertex *getter = LoadRegister(2);  // 2: getter register index
        ValueVertex *setter = LoadRegister(3);  // 3: setter register index
        ValueVertex *acc = frameState->GetAcc();
        ValueVertex *undefinedValue = self->GetUndefinedValue();
        ValueVertex *taggedOne = TaggedConstantFromInt32(1);

        frameState->SetAcc(RuntimeCall(
            {obj, prop, getter, setter, acc, undefinedValue, taggedOne},
            RTSTUB_ID(DefineGetterSetterByValue)));
    }

    void LowerDefineClassWithBuffer()
    {
        ValueVertex *methodId = TaggedConstantFromInt32(static_cast<int>(GetConstDataId(0)));
        ValueVertex *literalId = TaggedConstantFromInt32(static_cast<int>(GetConstDataId(1)));
        ValueVertex *length = TaggedConstantFromInt32(static_cast<int>(GetImmediate(2)));
        ValueVertex *proto = LoadRegister(3);  // 3: proto register index
        ValueVertex *lexicalEnv = LoadRegister(4);  // 4: lexicalEnv register index
        ValueVertex *sharedConstPool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();

#if ECMASCRIPT_ENABLE_IC
        ValueVertex *slotId = TaggedConstantFromInt32(static_cast<int>(GetICSlotId(5)));  // 5 : Slot ID index
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);

        frameState->SetAcc(RuntimeCall(
            {proto, lexicalEnv, sharedConstPool, methodId, literalId, module, length, slotId, jsFunc},
            RTSTUB_ID(CreateClassWithBuffer)));
#else
        frameState->SetAcc(RuntimeCall(
            {proto, lexicalEnv, sharedConstPool, methodId, literalId, module, length},
            RTSTUB_ID(CreateClassWithBuffer)));
#endif
    }

    void LowerDefineFunc()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *slotId = self->graph_->GetInt32Constant(static_cast<int>(GetICSlotId(0)));
        ValueVertex *methodId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(1)));
        // 2: length operand index
        ValueVertex *length = self->graph_->GetInt32Constant(static_cast<int>(GetImmediate(2)));
        // 3: lexicalEnv register index
        ValueVertex *lexicalEnv = LoadRegister(3);
        ValueVertex *globalEnv = GlobalEnv();

        frameState->SetAcc(CommonStubCall(
            {glue, jsFunc, methodId, length, lexicalEnv, slotId, globalEnv},
            CommonStubCSigns::Definefunc));
    }

    void LowerCopyRestArgs()
    {
        ValueVertex *taggedArgc = TaggedActualArgc();
        ValueVertex *taggedRestIdx = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        frameState->SetAcc(RuntimeCall({taggedArgc, taggedRestIdx}, RTSTUB_ID(OptCopyRestArgs)));
    }

    void LowerLdPatchVar()
    {
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        frameState->SetAcc(RuntimeCall({index}, RTSTUB_ID(LdPatchVar)));
    }

    void LowerStPatchVar()
    {
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *value = frameState->GetAcc();
        RuntimeCall({index, value}, RTSTUB_ID(StPatchVar));
    }

    void LowerLdLocalModuleVar()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        frameState->SetAcc(RuntimeCall({index, jsFunc}, RTSTUB_ID(LdLocalModuleVarByIndexOnJSFunc)));
    }

    void LowerTestIn()
    {
        // 1: level operand index
        ValueVertex *levelIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        // 2: slot operand index
        ValueVertex *slotIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(2)));
        ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
        ValueVertex *obj = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(TestIn)));
    }

    void LowerNotifyConcurrentResult()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *result = frameState->GetAcc();
        RuntimeCall({result, jsFunc}, RTSTUB_ID(NotifyConcurrentResult));
    }

    void LowerDefinePropertyByName()
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(1)));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(2);  // 2: obj register index
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *glue = self->GetGlue();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, obj, prop, value, globalEnv}, CommonStubCSigns::DefineField);
    }

    void LowerDefineFieldByName()
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(1)));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(2);  // 2: obj register index
        ValueVertex *value = frameState->GetAcc();
        ValueVertex *glue = self->GetGlue();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, obj, prop, value, globalEnv}, CommonStubCSigns::DefineField);
    }

    void LowerDefineFieldByValue()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *propKey = LoadRegister(0);
        ValueVertex *acc = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, propKey, acc, globalEnv}, CommonStubCSigns::DefineField);
    }

    void LowerDefineFieldByIndex()
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *receiver = LoadRegister(1);
        ValueVertex *propKey = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *acc = frameState->GetAcc();
        ValueVertex *globalEnv = GlobalEnv();
        CommonStubCall({glue, receiver, propKey, acc, globalEnv}, CommonStubCSigns::DefineField);
    }

    void LowerToPropertyKey()
    {
        ValueVertex *value = frameState->GetAcc();
        frameState->SetAcc(RuntimeCall({value}, RTSTUB_ID(ToPropertyKey)));
    }

    void LowerCreatePrivateProperty()
    {
        ValueVertex *count = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *literalId = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        ValueVertex *lexicalEnv = LoadRegister(2);  // 2: lexicalEnv register index
        ValueVertex *constpool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();

        RuntimeCall({lexicalEnv, count, constpool, literalId, module}, RTSTUB_ID(CreatePrivateProperty));
    }

    void LowerDefinePrivateProperty()
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(0)));
        ValueVertex *slotIndex = TaggedConstantFromInt32(static_cast<int>(GetImmediate(1)));
        ValueVertex *obj = LoadRegister(2);  // 2: obj register index
        ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
        ValueVertex *value = frameState->GetAcc();
        RuntimeCall({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(DefinePrivateProperty));
    }

    void LowerLoadString()
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(static_cast<int>(GetConstDataId(0)));
        ValueVertex *res = StringFromConstPool(stringId);
        frameState->SetAcc(res);
    }

    void LowerJumpConstant()
    {
        const BasicBlockInfo *blockInfo = self->preproc_->GetBasicBlockByRPO(rpoIndex);
        ASSERT(blockInfo->jumpBlock != nullptr);
        uint32_t destIndex = blockInfo->jumpBlock->rpoIndex;

        if (blockInfo->jumpBlock->loopBackBlock == blockInfo) {
            self->NewControlVertex<JumpLoopVertex>(CurrentBlock(), {}, self->blocks_[destIndex]);
        } else {
            self->NewControlVertex<JumpVertex>(CurrentBlock(), {}, self->blocks_[destIndex]);
        }
    }

    // -------- Helpers --------

    BB *CurrentBlock() const
    {
        return self->blocks_[rpoIndex];
    }

    ValueVertex *LoadRegister(int index) const
    {
        auto &input = bcInfo->details.inputs[index];
        ASSERT(std::holds_alternative<VirtualRegister>(input));
        return frameState->Get(std::get<VirtualRegister>(input).GetId());
    }

    ValueVertex *LoadParam(VRegIDType index) const
    {
        VirtualRegister v = VRegOfParam(self->GetNumLocalVRegs(), index);
        return frameState->Get(v.GetId());
    }

    uint16_t GetConstDataId(int index) const
    {
        auto &input = bcInfo->details.inputs[index];
        ASSERT(std::holds_alternative<ConstDataId>(input));
        return std::get<ConstDataId>(input).GetId();
    }

    ICSlotIdType GetICSlotId(int index) const
    {
        auto &input = bcInfo->details.inputs[index];
        ASSERT(std::holds_alternative<ICSlotId>(input));
        return std::get<ICSlotId>(input).GetId();
    }

    ImmValueType GetImmediate(int index) const
    {
        auto &input = bcInfo->details.inputs[index];
        ASSERT(std::holds_alternative<Immediate>(input));
        return std::get<Immediate>(input).GetValue();
    }

    ValueVertex *ActualArgc()
    {
        return self->NewVertexNoInput<ActualArgcVertex>(CurrentBlock());
    }

    ValueVertex *TaggedActualArgc()
    {
        ValueVertex *argc = ActualArgc();
        return self->NewVertex<ToTaggedIntVertex>(CurrentBlock(), {argc});
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
                const char *machineTypeName = MachineTypeName(params[i].GetMachineType());
                const char *reprName = ValueRepresentationName(inputArr[i]->GetValueRepresentation());
                LOG_ECMA(FATAL) << "ArkSteed CommonStub argument type mismatch, stub: " << signature->GetName()
                                << ", index: " << i
                                << ", expected machine type: " << machineTypeName
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
        return self->NewVertex<CallCommonStubVertex>(CurrentBlock(), inputs, id);
    }

    ValueVertex *CommonStubCallWithIC(std::initializer_list<ValueVertex *> inputs, CommonStubID id)
    {
        ChunkVector<ValueVertex *> allArgs(self->GetChunk());
        allArgs.reserve(inputs.size() + 3);  // 3: glue + jsFunc + slotId

        allArgs.push_back(self->GetGlue());
        allArgs.insert(allArgs.end(), inputs.begin(), inputs.end());
        allArgs.push_back(LoadParam(CALL_TARGET_PARAM_INDEX));
        allArgs.push_back(self->graph_->GetInt32Constant(static_cast<int>(GetICSlotId(0))));

        ValidateCommonStubCallArgs({allArgs.data(), allArgs.size()}, id);
        return self->NewVertex<CallCommonStubVertex>(CurrentBlock(), allArgs, id);
    }

    ValueVertex *RuntimeCall(std::initializer_list<ValueVertex *> inputs, RuntimeStubID id)
    {
        return self->NewVertex<CallRuntimeVertex>(CurrentBlock(), inputs, id);
    }

    ValueVertex *RuntimeCall(const ChunkVector<ValueVertex *> &inputs, RuntimeStubID id)
    {
        return self->NewVertex<CallRuntimeVertex>(CurrentBlock(), inputs, id);
    }

    ValueVertex *TaggedConstantFromInt32(int value)
    {
        JSTaggedType taggedValue = JSTaggedValue(value).GetRawData();
        return self->graph_->GetTaggedConstant(taggedValue);
    }

    ValueVertex *GlobalEnv()
    {
        ValueVertex *lexicalEnv = frameState->GetEnv();
        int32_t globalEnvOffset = static_cast<int32_t>(GlobalEnv::HEADER_SIZE);
        return self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {lexicalEnv}, globalEnvOffset);
    }

    ValueVertex *TaggedArrayFromValueIn(ValueVertex *taggedInputSize, uint32_t inputSize, uint32_t startIndex = 0)
    {
        ValueVertex *taggedArray = RuntimeCall({taggedInputSize}, RTSTUB_ID(NewTaggedArray));
        for (uint32_t idx = 0; idx < inputSize; ++idx) {
            ValueVertex *arg = LoadRegister(startIndex + idx);
            SetValueToTaggedArray(taggedArray, idx, arg);
        }
        return taggedArray;
    }

    ValueVertex *GetValueFromTaggedArray(ValueVertex *array, uint32_t index)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {array}, offset);
    }

    ValueVertex *SetValueToTaggedArray(ValueVertex *array, uint32_t index, ValueVertex *value)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<StoreTaggedFieldVertex>(CurrentBlock(), {array, value}, offset);
    }

    ValueVertex *SharedConstPool()
    {
        int32_t methodOffset = static_cast<int32_t>(JSFunctionBase::METHOD_OFFSET);
        int32_t constpoolOffset = static_cast<int32_t>(Method::CONSTANT_POOL_OFFSET);

        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *method = self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {jsFunc}, methodOffset);
        return self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {method}, constpoolOffset);
    }

    ValueVertex *ModuleFromFunction()
    {
        int32_t moduleOffset = static_cast<int32_t>(JSFunction::ECMA_MODULE_OFFSET);
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        return self->NewVertex<LoadTaggedFieldVertex>(CurrentBlock(), {jsFunc}, moduleOffset);
    }

    ValueVertex *StringFromConstPool(ValueVertex *stringId)
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *constpool = SharedConstPool();
        return CommonStubCall({glue, constpool, stringId}, CommonStubCSigns::GetStringFromConstPool);
    }

    ValueVertex *ObjectFromConstPool(ValueVertex *index)
    {
        ValueVertex *glue = self->GetGlue();
        ValueVertex *constpool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();
        return CommonStubCall({glue, constpool, index, module}, CommonStubCSigns::GetObjectFromConstPool);
    }

    ValueVertex *MethodFromConstPool(ValueVertex *index)
    {
        ValueVertex *constpool = SharedConstPool();
        return self->NewVertex<CallRuntimeVertex>(CurrentBlock(), {constpool, index}, RTSTUB_ID(GetMethodFromCache));
    }

    GraphBuilderNew *self;
    uint32_t rpoIndex;
    BCFrameState *frameState;
    const BytecodeInfo *bcInfo;
};

void GraphBuilderNew::VisitBytecode(uint32_t rpoIndex, BCFrameState *frameState, const BytecodeInfo *bcInfo)
{
    BytecodeVisitor{this, rpoIndex, frameState, bcInfo}.Visit();
}
}  // namespace panda::ecmascript::arksteed
