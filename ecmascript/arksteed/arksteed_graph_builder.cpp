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

#include "ecmascript/arksteed/arksteed_graph_builder.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>

#include "ecmascript/accessor_data.h"
#include "ecmascript/arksteed/arksteed_compile_info_facts.h"
#include "ecmascript/arksteed/arksteed_constant_folding.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_register_merge_state.h"
#include "ecmascript/arksteed/arksteed_side_effect_classifier.h"
#include "ecmascript/arksteed/arksteed_write_barrier_value_kind_pass.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/elements.h"
#include "ecmascript/ic/ic_info.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/profile_type_info_cell.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/string/base_string.h"

namespace panda::ecmascript::arksteed {
namespace {
constexpr uint32_t CALL_ARG0 = 0;
constexpr uint32_t CALL_ARG1 = 1;
constexpr uint32_t CALL_ARG2 = CALL_ARG1 + 1;
constexpr uint32_t CALL_ARG3 = CALL_ARG2 + 1;

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
            return value->IsInt64();
        case kungfu::MachineType::I1:
        case kungfu::MachineType::I8:
        case kungfu::MachineType::I16:
        case kungfu::MachineType::I32:
            return value->IsInt32() || value->IsUInt32() || value->IsInt64();
        case kungfu::MachineType::F32:
        case kungfu::MachineType::F64:
            return value->IsAnyFloat64();
    }
    return true;
}

void ValidateCommonStubCallArgs(Span<ValueVertex *const> inputs, CommonStubID id)
{
#ifndef NDEBUG
    const kungfu::CallSignature *signature = kungfu::CommonStubCSigns::Get(id);
    size_t actualCount = inputs.size();
    size_t expectedCount = signature->GetParametersCount();
    if (actualCount != expectedCount) {
        LOG_ECMA(FATAL) << "ArkSteed CommonStub argument count mismatch, stub: " << signature->GetName()
                        << ", expected: " << expectedCount << ", actual: " << actualCount;
        UNREACHABLE();
    }
    kungfu::VariableType *params = signature->GetParametersType();
    if (params == nullptr) {
        return;
    }
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
#else
    (void)inputs;  // // No-op in Release build
    (void)id;
#endif
}

void ValidateCommonStubCallArgs(std::initializer_list<ValueVertex *> inputs, CommonStubID id)
{
    auto span = Span<ValueVertex * const>{inputs.begin(), inputs.size()};
    ValidateCommonStubCallArgs(span, id);
}

bool SupportsI32CheckedBinOp(BinaryOpKind kind)
{
    switch (kind) {
        case BinaryOpKind::ADD:
        case BinaryOpKind::SUB:
        case BinaryOpKind::MUL:
        case BinaryOpKind::DIV:
        case BinaryOpKind::MOD:
            return true;
        default:
            return false;
    }
}

bool SupportsF64BinOp(BinaryOpKind kind)
{
    switch (kind) {
        case BinaryOpKind::ADD:
        case BinaryOpKind::SUB:
        case BinaryOpKind::MUL:
        case BinaryOpKind::DIV:
            return true;
        default:
            return false;
    }
}

bool IsEqualityCompare(JSCondition kind)
{
    return kind == JSCondition::EQUAL || kind == JSCondition::NOT_EQUAL ||
           kind == JSCondition::STRICT_EQUAL || kind == JSCondition::STRICT_NOT_EQUAL;
}

bool IsStrictEqualityCompare(JSCondition kind)
{
    return kind == JSCondition::STRICT_EQUAL || kind == JSCondition::STRICT_NOT_EQUAL;
}

bool IsEqualCompare(JSCondition kind)
{
    return kind == JSCondition::EQUAL || kind == JSCondition::STRICT_EQUAL;
}

bool IsReferenceComparableRootValue(ValueVertex *node)
{
    if (node == nullptr) {
        return false;
    }
    auto *constant = node->TryCast<TaggedConstantVertex>();
    if (constant == nullptr) {
        return false;
    }
    JSTaggedValue tagged(constant->GetValue());
    return tagged.IsBoolean() || tagged.IsNull() || tagged.IsUndefined() || tagged.IsHole();
}

bool IsReferenceComparableType(NodeInfo::NodeType type)
{
    using NodeType = NodeInfo::NodeType;
    constexpr NodeType referenceComparable = NodeInfo::UnionNodeType(
        NodeInfo::UnionNodeType(NodeType::NULL_OR_UNDEFINED, NodeType::BOOLEAN),
        NodeInfo::UnionNodeType(NodeType::SYMBOL, NodeType::JS_RECEIVER));
    return NodeInfo::NodeTypeIs(type, referenceComparable);
}

bool StrictTypesCanBeEqual(NodeInfo::NodeType leftType, NodeInfo::NodeType rightType)
{
    if (NodeInfo::NodeTypeCanBe(NodeInfo::IntersectNodeType(leftType, rightType), NodeInfo::NodeType::UNKNOWN)) {
        return true;
    }
    return NodeInfo::NodeTypeCanBe(leftType, NodeInfo::NodeType::NUMBER) &&
           NodeInfo::NodeTypeCanBe(rightType, NodeInfo::NodeType::NUMBER);
}

bool EvaluateInt32Compare(JSCondition kind, int32_t left, int32_t right)
{
    switch (kind) {
        case JSCondition::EQUAL:
        case JSCondition::STRICT_EQUAL:
            return left == right;
        case JSCondition::NOT_EQUAL:
        case JSCondition::STRICT_NOT_EQUAL:
            return left != right;
        case JSCondition::LESS_THAN:
            return left < right;
        case JSCondition::LESS_THAN_OR_EQUAL:
            return left <= right;
        case JSCondition::GREATER_THAN:
            return left > right;
        case JSCondition::GREATER_THAN_OR_EQUAL:
            return left >= right;
    }
    UNREACHABLE();
}

bool EvaluateFloat64Compare(JSCondition kind, double left, double right)
{
    bool unordered = std::isnan(left) || std::isnan(right);
    switch (kind) {
        case JSCondition::EQUAL:
        case JSCondition::STRICT_EQUAL:
            return !unordered && left == right;
        case JSCondition::NOT_EQUAL:
        case JSCondition::STRICT_NOT_EQUAL:
            return unordered || left != right;
        case JSCondition::LESS_THAN:
            return !unordered && left < right;
        case JSCondition::LESS_THAN_OR_EQUAL:
            return !unordered && left <= right;
        case JSCondition::GREATER_THAN:
            return !unordered && left > right;
        case JSCondition::GREATER_THAN_OR_EQUAL:
            return !unordered && left >= right;
    }
    UNREACHABLE();
}

Condition Int32ConditionFromCompare(JSCondition kind)
{
    switch (kind) {
        case JSCondition::EQUAL:
        case JSCondition::STRICT_EQUAL:
            return Condition::EQUAL;
        case JSCondition::NOT_EQUAL:
        case JSCondition::STRICT_NOT_EQUAL:
            return Condition::NOT_EQUAL;
        case JSCondition::LESS_THAN:
            return Condition::LESS_THAN;
        case JSCondition::LESS_THAN_OR_EQUAL:
            return Condition::LESS_THAN_OR_EQUAL;
        case JSCondition::GREATER_THAN:
            return Condition::GREATER_THAN;
        case JSCondition::GREATER_THAN_OR_EQUAL:
            return Condition::GREATER_THAN_OR_EQUAL;
    }
    UNREACHABLE();
}

JSCondition InvertCompare(JSCondition kind)
{
    switch (kind) {
        case JSCondition::EQUAL:
            return JSCondition::NOT_EQUAL;
        case JSCondition::NOT_EQUAL:
            return JSCondition::EQUAL;
        case JSCondition::STRICT_EQUAL:
            return JSCondition::STRICT_NOT_EQUAL;
        case JSCondition::STRICT_NOT_EQUAL:
            return JSCondition::STRICT_EQUAL;
        default:
            UNREACHABLE();
    }
}

// ---- Common-subexpression elimination helpers (available expressions) ----
// Standard 64-bit FNV-1a constants. The hash is only an available-expression lookup key;
// CompileInfoFacts::FindExpression still checks opcode, inputs, options, and effect epoch.
constexpr uint64_t CSE_FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t CSE_FNV_PRIME = 1099511628211ULL;

uint64_t CseHashCombine(uint64_t hash, uint64_t value)
{
    hash ^= value;
    hash *= CSE_FNV_PRIME;
    return hash;
}

uint64_t CseHashValue(ValueVertex *value)
{
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value));
}

uint32_t CseHashExpression(VertexOpcode opcode, const CompileInfoFacts::ExpressionInputs &inputs,
                           const CompileInfoFacts::ExpressionOptions &options)
{
    uint64_t hash = CseHashCombine(CSE_FNV_OFFSET_BASIS, static_cast<uint64_t>(opcode));
    for (ValueVertex *input : inputs) {
        hash = CseHashCombine(hash, CseHashValue(input));
    }
    for (uint64_t option : options) {
        hash = CseHashCombine(hash, option);
    }
    return static_cast<uint32_t>(hash ^ (hash >> 32U));  // 32: fold 64-bit hash to 32-bit hash.
}

template <typename T>
void CseAppendExpressionOption(CompileInfoFacts::ExpressionOptions &options, const T &value)
{
    using RawT = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (std::is_enum_v<RawT>) {
        options.push_back(static_cast<uint64_t>(value));
    } else if constexpr (std::is_integral_v<RawT>) {
        options.push_back(static_cast<uint64_t>(value));
    } else if constexpr (std::is_pointer_v<RawT>) {
        options.push_back(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value)));
    } else if constexpr (std::is_floating_point_v<RawT>) {
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        options.push_back(bits);
    } else {
        static_assert(std::is_trivially_copyable_v<RawT>, "Unsupported available-expression option type");
        static_assert(sizeof(RawT) <= sizeof(uint64_t), "Available-expression option is too large");
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(value));
        options.push_back(bits);
    }
}

template <typename... Args>
void CseBuildExpressionOptions(CompileInfoFacts::ExpressionOptions &options, const Args &...args)
{
    (CseAppendExpressionOption(options, args), ...);
}

constexpr bool CseIsExcludedAvailableExpressionOpcode(VertexOpcode opcode)
{
    switch (opcode) {
        case VertexOpcode::Int32Constant:
        case VertexOpcode::Int64Constant:
        case VertexOpcode::Float64Constant:
        case VertexOpcode::TaggedConstant:
        case VertexOpcode::HeapConstant:
        case VertexOpcode::InitialValue:
        case VertexOpcode::ActualArgc:
        case VertexOpcode::Call:
        case VertexOpcode::CallRuntime:
        case VertexOpcode::CallCommonStub:
        case VertexOpcode::Deopt:
        case VertexOpcode::Phi:
            return true;
        default:
            return false;
    }
}

template <typename VertexT>
constexpr bool CseCanUseAvailableExpression()
{
    if constexpr (!std::is_base_of_v<ValueVertex, VertexT>) {
        return false;
    } else {
        return !CseIsExcludedAvailableExpressionOpcode(OpcodeOf<VertexT>) &&
                CanParticipateInCSE(VertexT::PROPERTIES);
    }
}
}  // namespace

// Condensed storage: [vA, vA, vA, vB, vB, vB, vB, vB, vB, vC, vC, vC, vC]
//                 => [(vA, 3),    (vB, 6),                (vC, 4)]
struct GraphBuilder::CatchBlockInputData {
    struct InputEntry {
        ValueVertex *vertex;
        uint32_t count;
    };

    uint32_t totalCount;
    const kungfu::BitSet *liveIn;
    // inputs[i] = List of inputs for the i-th live-in virtual register
    ChunkVector<ChunkVector<InputEntry>> inputs;
    // Merge from all the vertices whose exceptions may be caught here
    CompileInfoFacts *facts;

    explicit CatchBlockInputData(const kungfu::BitSet &liveIn, Chunk *chunk)
        : totalCount(0), liveIn(&liveIn), inputs(chunk), facts(nullptr)
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

    void AddCompileInfoFacts(const CompileInfoFacts &incoming)
    {
        if (facts == nullptr) {
            facts = incoming.Clone();
            return;
        }
        facts->Merge(incoming);
    }
};

GraphBuilder::GraphBuilder(JSThread *compilerThread,
                           Graph *destGraph,
                           uintptr_t glueAddr,
                           BytecodePreprocessor *preproc,
                           BytecodeAnalysis *analysis)
    : graph_(destGraph),
      compilerThread_(compilerThread),
      glueAddr_(glueAddr),
      preproc_(preproc),
      analysis_(analysis),
      pgoContext_(compilerThread, preproc->GetEnv()),
      numLocal_(preproc->GetNumLocalVRegs()),
      numParams_(preproc->GetNumParamVRegs()),
      chunk_(preproc->GetChunk()),
      blocks_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      exitBlocks_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      frameStates_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      compileInfoFacts_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk()),
      catchBlockInputs_(preproc->GetNumLiveBasicBlocks(), preproc->GetChunk())
{}

bool GraphBuilder::Run()
{
    ASSERT(preproc_->GetNumLiveBasicBlocks() > 0);
    DebugLog();

    SharedBCFrameState frameState(preproc_->GetNumVRegs(), nullptr, chunk_);
    InitializeStartBlock(frameState);
    frameStates_[0] = CondensedBCFrameState(frameState, analysis_->GetLiveOutOfBlock(0), chunk_);

    // 1 : Skips the start block (which is initialized before)
    for (uint32_t i = 1, n = preproc_->GetNumLiveBasicBlocks(); i < n; i++) {
        if (blocks_[i] == nullptr) {
            ProcessDeadBasicBlock(i);
            continue;
        }
        frameState.Reset(nullptr);
        ProcessBasicBlock(frameState, i);
        frameStates_[i] = CondensedBCFrameState(frameState, analysis_->GetLiveOutOfBlock(i), chunk_);
    }
    return true;
}

void GraphBuilder::DebugLog()
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

void GraphBuilder::InitializeStartBlock(SharedBCFrameState frameState)
{
    glue_ = graph_->GetIntPtrConstant(glueAddr_);
    undefinedValue_ = graph_->GetTaggedConstant(JSTaggedValue::VALUE_UNDEFINED);

    compileInfoFacts_[0] = chunk_->New<CompileInfoFacts>(chunk_);
    compileInfoFacts_[0]->EnsureType(undefinedValue_, NodeInfo::NodeType::UNDEFINED);

    blocks_[0] = BB::New(chunk_);
    // caller argument area (in fp-slot words): +2 argc, +3 call-target, +4 new-target, +5 this, +6... user args.
    const int32_t ACTUAL_ARGC_FP_SLOT_INDEX = 2;
    const int32_t CALL_TARGET_FP_SLOT_INDEX = 3;
    initialActualArgc_ = NewVertex<InitialValueVertex>(blocks_[0], {}, ACTUAL_ARGC_FP_SLOT_INDEX);
    for (uint32_t i = 0, n = numParams_; i < n; i++) {
        int32_t slotIndex = static_cast<int32_t>(i + CALL_TARGET_FP_SLOT_INDEX);
        auto *v = NewVertex<InitialValueVertex>(blocks_[0], {}, slotIndex);
        graph_->AddParameter(v);
        frameState.Set(VRegOfParam(numLocal_, i), v);
    }
    actualArgc_ = NewVertex<ActualArgcVertex>(blocks_[0], {});
    taggedActualArgc_ = NewVertex<I32ToTaggedIntVertex>(blocks_[0], {actualArgc_});

    // -3 : Fixed header lexicalEnv is at slot -3 in word units.
    initialLexicalEnv_ = NewVertex<InitialValueVertex>(blocks_[0], {}, -3);
    frameState.SetLexicalEnv(initialLexicalEnv_);
    FinishBlockWithJump(blocks_[0], ActivateNonCatchBlock(1));
    exitBlocks_[0] = blocks_[0];
}

void GraphBuilder::ProcessDeadBasicBlock(uint32_t rpoIndex)
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
    FinishDeadLoopBackEdge(blocks_[rpoIndex], rpoIndex);
    exitBlocks_[rpoIndex] = blocks_[rpoIndex];
}

void GraphBuilder::FinishDeadLoopBackEdge(BB *owner, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(bcBlock->IsEndOfLoop());
    uint32_t headerRpoIndex = bcBlock->jumpBlock->rpoIndex;
    ASSERT(blocks_[headerRpoIndex] != nullptr);
    FinishBlockWithJumpLoop(owner, blocks_[headerRpoIndex]);

    for (PhiVertex *phi : blocks_[headerRpoIndex]->GetPhis()) {
        ASSERT(phi->GetInputCount() == 2);  // 2 : Two jumpPredecessors: one is entry, the other is loop-back
        phi->SetInput(1, undefinedValue_);
    }
}

void GraphBuilder::ProcessBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    if (bcBlock->IsCatchBlockHeader()) {
        ProcessCatchBlockHead(frameState, rpoIndex);
        return;
    }
    InitCompileInfoFacts(rpoIndex);
    if (bcBlock->IsLoopHeader()) {
        blocks_[rpoIndex]->SetIsLoopHeader(true);
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
        exitBlocks_[rpoIndex] = blocks_[rpoIndex];
    } else {
        exitBlocks_[rpoIndex] = VisitBytecodesOfBasicBlock(frameState, rpoIndex);
    }
    if (bcBlock->IsEndOfLoop()) {
        ControlVertex *control = exitBlocks_[rpoIndex]->GetControlVertex();
        if (control->Is<DeoptVertex>()) {
            // Keep the structural backedge needed by loop phis and liveness after the real path deopts.
            FinishDeadLoopBackEdge(BB::New(chunk_), rpoIndex);
        } else {
            ASSERT(control->Is<JumpLoopVertex>());
            WriteBackFrameStateToLoopHeader(frameState, rpoIndex);
        }
    }
}

void GraphBuilder::ProcessCatchBlockHead(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    blocks_[rpoIndex]->SetIsExceptionHandler(true);
    InitCompileInfoFactsForCatchBlock(rpoIndex);
    InitFrameStateForCatchBlockHeader(frameState, rpoIndex);

    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(bcBlock->jumpBlock != nullptr && !bcBlock->jumpBlock->IsSynthetic());

    // Catch block header is always synthetic. Only an unconditional jump.
    ASSERT(bcBlock->IsJump());
    FinishBlockWithJump(blocks_[rpoIndex], ActivateNonCatchBlock(bcBlock->jumpBlock->rpoIndex));
    exitBlocks_[rpoIndex] = blocks_[rpoIndex];
}

bool GraphBuilder::HasEmittedNormalEdge(uint32_t predRpoIndex, uint32_t targetRpoIndex) const
{
    BB *predExit = exitBlocks_[predRpoIndex];
    BB *target = blocks_[targetRpoIndex];
    if (predExit == nullptr || target == nullptr) {
        return false;
    }
    const auto &predecessors = target->GetPredecessors();
    return std::find(predecessors.begin(), predecessors.end(), predExit) != predecessors.end();
}

void GraphBuilder::InitFrameState(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);

    BB *target = blocks_[rpoIndex];
    uint32_t actualNumPreds = target->PredecessorCount();
    uint32_t actualPredIndex = 0;
    for (const BasicBlockInfo *predecessor : bcBlock->jumpPredecessors) {
        uint32_t predRpoIndex = predecessor->rpoIndex;
        if (!HasEmittedNormalEdge(predRpoIndex, rpoIndex)) {
            continue;
        }
        ASSERT(actualPredIndex < actualNumPreds);
        ASSERT(target->GetPredecessor(actualPredIndex) == exitBlocks_[predRpoIndex]);
        MergeFrameState(frameState, rpoIndex, predRpoIndex, actualPredIndex++, actualNumPreds);
    }
    ASSERT(actualPredIndex == actualNumPreds);
}

void GraphBuilder::InitFrameStateForLoopHeader(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(blockInfo->jumpPredecessors.size() == 2);  // 2 : One is entry, the other is loop-back

    kungfu::BitSet phiCandidates(chunk_, frameState.NumVRegs());
    phiCandidates.CopyFrom(analysis_->GetLiveInOfBlock(rpoIndex));
    phiCandidates.Intersect(analysis_->GetKillSetOfBlock(rpoIndex));

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

void GraphBuilder::InitFrameStateForCatchBlockHeader(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    // For catch blocks, acc is always initialized as the exception object
    frameState.SetAcc(NewVertex<LoadExceptionVertex>(blocks_[rpoIndex], {glue_}));

    CatchBlockInputData *data = catchBlockInputs_[rpoIndex];
    ASSERT(data != nullptr);
    ASSERT(data->totalCount >= 1);

    const kungfu::BitSet &liveIn = analysis_->GetLiveInOfBlock(rpoIndex);
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

void GraphBuilder::InitCompileInfoFacts(uint32_t rpoIndex)
{
    const BasicBlockInfo *blockInfo = preproc_->GetBasicBlockByRPO(rpoIndex);
    ASSERT(!blockInfo->IsCatchBlockHeader() && "Use InitCompileInfoFactsForCatchBlock() instead.");

    if (blockInfo->IsLoopHeader()) {
        ASSERT(blockInfo->jumpPredecessors.size() == 2);  // 2: loop entry and loop backedge
        uint32_t entryPredIndex = blockInfo->jumpPredecessors[0]->rpoIndex;
        ASSERT(compileInfoFacts_[entryPredIndex] != nullptr);
        compileInfoFacts_[rpoIndex] = compileInfoFacts_[entryPredIndex]->CloneForLoopHeader();
        return;
    }

    CompileInfoFacts *facts = nullptr;
    for (const BasicBlockInfo *predecessor : blockInfo->jumpPredecessors) {
        uint32_t predRpoIndex = predecessor->rpoIndex;
        if (!HasEmittedNormalEdge(predRpoIndex, rpoIndex)) {
            continue;
        }
        ASSERT(compileInfoFacts_[predRpoIndex] != nullptr);
        if (facts == nullptr) {
            facts = compileInfoFacts_[predRpoIndex]->Clone();
        } else {
            facts->Merge(*compileInfoFacts_[predRpoIndex]);
        }
    }
    ASSERT(facts != nullptr);
    compileInfoFacts_[rpoIndex] = facts;
}

void GraphBuilder::InitCompileInfoFactsForCatchBlock(uint32_t rpoIndex)
{
    CatchBlockInputData *data = catchBlockInputs_[rpoIndex];
    ASSERT(data != nullptr);
    ASSERT(data->facts != nullptr);
    compileInfoFacts_[rpoIndex] = data->facts;
}

void GraphBuilder::WriteBackFrameStateToLoopHeader(SharedBCFrameState current, uint32_t rpoIndex)
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

void GraphBuilder::MergeFrameState(SharedBCFrameState dest, uint32_t rpoIndex, uint32_t predRpoIndex,
                                   uint32_t actualPredIndex, uint32_t actualNumPreds)
{
    const kungfu::BitSet &liveIn = analysis_->GetLiveInOfBlock(rpoIndex);
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

PhiVertex *GraphBuilder::NewPhiVertex(BB *owner, uint32_t numPredecessors, VRegIDType vreg)
{
    PhiVertex *phi = PhiVertex::New(chunk_, numPredecessors, VirtualRegister(vreg));
    phi->SetOwner(owner);
    owner->AddPhiVertex(phi);
    return phi;
}

template <class InputRange>
PhiVertex *GraphBuilder::NewPhiVertexWith(BB *owner, const InputRange &inputs, VRegIDType vreg)
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
VertexT *GraphBuilder::NewVertex(BB *owner, const InputRange &inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
    vertex->SetOwner(owner);
    owner->AddVertex(vertex);
    // At most one of: eager_deopt, lazy_deopt
    static_assert(CanEagerDeopt(VertexT::PROPERTIES) + CanLazyDeopt(VertexT::PROPERTIES) <= 1);

    return vertex;
}

template <class VertexT, class InputRange, class... Args>
VertexT *GraphBuilder::NewVertex(
    CompileInfoFacts *compileInfoFacts, BB *owner, const InputRange &inputs, Args &&...args)
{
    if constexpr (CseCanUseAvailableExpression<VertexT>()) {
        CompileInfoFacts::ExpressionInputs expressionInputs(chunk_);
        for (ValueVertex *input : inputs) {
            expressionInputs.push_back(input);
        }

        CompileInfoFacts::ExpressionOptions options(chunk_);
        CseBuildExpressionOptions(options, args...);
        uint32_t hash = CseHashExpression(OpcodeOf<VertexT>, expressionInputs, options);
        bool needsEpochCheck = CanRead(VertexT::PROPERTIES);
        ValueVertex *cached = compileInfoFacts->FindExpression(
            hash, OpcodeOf<VertexT>, expressionInputs, options, needsEpochCheck);
        if (cached != nullptr) {
            return cached->Cast<VertexT>();
        }

        VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
        vertex->SetOwner(owner);
        owner->AddVertex(vertex);
        compileInfoFacts->AddExpression(hash, vertex, expressionInputs, options, needsEpochCheck);
        return vertex;
    }
    VertexT *vertex = NewVertex<VertexT>(owner, inputs, std::forward<Args>(args)...);
    if constexpr (CanWrite(VertexT::PROPERTIES)) {
        compileInfoFacts->MarkPossibleSideEffect(ArkSteedSideEffectClassifier::Classify(vertex));
    }
    return vertex;
}

JumpVertex *GraphBuilder::FinishBlockWithJump(BB *owner, BB *target)
{
    auto *jumpVertex = FinishBlockWith<JumpVertex>(owner, {}, target);
    jumpVertex->SetPredecessorId(target->PredecessorCount());
    target->AddPredecessor(owner);
    return jumpVertex;
}

JumpLoopVertex *GraphBuilder::FinishBlockWithJumpLoop(BB *owner, BB *target)
{
    auto *jumpLoopVertex = FinishBlockWith<JumpLoopVertex>(owner, {}, chunk_, target);
    jumpLoopVertex->SetPredecessorId(target->PredecessorCount());
    target->AddPredecessor(owner);
    return jumpLoopVertex;
}

ControlVertex *GraphBuilder::FinishBlockWithBranch(
    BB *owner, ValueVertex *input, BB *targetIfTrue, BB *targetIfFalse)
{
    auto finishWithTargets = [targetIfTrue, targetIfFalse, owner](ControlVertex *vertex) {
        targetIfTrue->AddPredecessor(owner);
        targetIfFalse->AddPredecessor(owner);
        return vertex;
    };

    if (auto *compare = input->TryCast<I32ConditionCheckVertex>()) {
        return finishWithTargets(FinishBlockWith<BranchIfInt32CompareVertex>(
            owner,
            {compare->GetInput(I32ConditionCheckVertex::LEFT_INDEX),
             compare->GetInput(I32ConditionCheckVertex::RIGHT_INDEX)},
            targetIfTrue, targetIfFalse, compare->GetCondition()));
    }
    if (auto *compare = input->TryCast<F64ConditionCheckVertex>()) {
        return finishWithTargets(FinishBlockWith<BranchIfFloat64CompareVertex>(
            owner,
            {compare->GetInput(F64ConditionCheckVertex::LEFT_INDEX),
             compare->GetInput(F64ConditionCheckVertex::RIGHT_INDEX)},
            targetIfTrue, targetIfFalse, compare->GetCondition()));
    }
    if (auto *equal = input->TryCast<TaggedEqualVertex>()) {
        return finishWithTargets(FinishBlockWith<BranchIfReferenceEqualVertex>(
            owner,
            {equal->GetInput(TaggedEqualVertex::LEFT_INDEX), equal->GetInput(TaggedEqualVertex::RIGHT_INDEX)},
            targetIfTrue, targetIfFalse));
    }
    if (auto *notEqual = input->TryCast<TaggedNotEqualVertex>()) {
        return finishWithTargets(FinishBlockWith<BranchIfReferenceEqualVertex>(
            owner,
            {notEqual->GetInput(TaggedNotEqualVertex::LEFT_INDEX),
             notEqual->GetInput(TaggedNotEqualVertex::RIGHT_INDEX)},
            targetIfFalse, targetIfTrue));
    }

    auto *branchVertex = FinishBlockWith<BranchIfTrueVertex>(owner, {input}, targetIfTrue, targetIfFalse);
    return finishWithTargets(branchVertex);
}

template <class BranchVertexT, class... Args>
BranchVertexT *GraphBuilder::FinishBlockWithBranch(
    BB *owner, std::initializer_list<ValueVertex *> inputs, BB *targetIfTrue, BB *targetIfFalse, Args &&...args)
{
    auto *vertex = FinishBlockWith<BranchVertexT>(
        owner, inputs, targetIfTrue, targetIfFalse, std::forward<Args>(args)...);
    targetIfTrue->AddPredecessor(owner);
    targetIfFalse->AddPredecessor(owner);
    return vertex;
}

template <class VertexT, class... Args>
VertexT *GraphBuilder::FinishBlockWith(BB *owner, std::initializer_list<ValueVertex *> inputs, Args &&...args)
{
    VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
    vertex->SetOwner(owner);
    owner->SetControlVertex(vertex);
    graph_->Add(owner);
    // Control vertices cannot have lazy deopt, throw, or write side effects
    // Note: ThrowVertex is a special case that can throw
    static_assert(!CanLazyDeopt(VertexT::PROPERTIES) && !CanWrite(VertexT::PROPERTIES));

    return vertex;
}

BB *GraphBuilder::NewBlock()
{
    BB *result = BB::New(chunk_);
    // TODO: For legacy code only. To be removed.
    result->SetRegisterMergeState(chunk_->New<RegisterMergeState>());
    return result;
}

BB *GraphBuilder::ActivateNonCatchBlock(uint32_t rpoIndex)
{
    if (blocks_[rpoIndex] == nullptr) {
        blocks_[rpoIndex] = NewBlock();
    }
    return blocks_[rpoIndex];
}

BB *GraphBuilder::ActivateCatchBlock(GraphBuilder::CatchBlockInputData **inputData, uint32_t rpoIndex)
{
    if (UNLIKELY(blocks_[rpoIndex] == nullptr)) {
        blocks_[rpoIndex] = NewBlock();
        catchBlockInputs_[rpoIndex] = chunk_->New<CatchBlockInputData>(analysis_->GetLiveInOfBlock(rpoIndex), chunk_);
    }
    ASSERT(catchBlockInputs_[rpoIndex] != nullptr);
    *inputData = catchBlockInputs_[rpoIndex];
    return blocks_[rpoIndex];
}

LoadTaggedFieldVertex *GraphBuilder::ActivateGlobalEnv()
{
    if (UNLIKELY(lazyGlobalEnv_ == nullptr)) {
        int32_t globalEnvOffset = static_cast<int32_t>(GlobalEnv::HEADER_SIZE);
        lazyGlobalEnv_ = NewVertex<LoadTaggedFieldVertex>(blocks_[0], {initialLexicalEnv_}, globalEnvOffset);
    }
    return lazyGlobalEnv_;
}

constexpr uintptr_t NO_CATCH_BLOCK_TAG = 1;

struct GraphBuilder::BytecodeVisitor {
    struct NamedLoadAccessInfo {
        JSHClass *receiverHClass {nullptr};
        JSHClass *holderHClass {nullptr};
        std::vector<JSHClass *> lookupStartObjectHClasses;
        std::vector<JSHClass *> expectedPrototypeHClasses;
        PropertyLookupResult plr;
        uint32_t holderDepth {0};
        bool isConst {false};
        bool canAssumeStableHClasses {false};
        bool hasStableProtoChain {false};
    };

    using NamedLoadAccessInfoOpt = std::optional<NamedLoadAccessInfo>;
    using NamedLoadAccessInfosOpt = std::optional<std::vector<NamedLoadAccessInfo>>;

    bool Visit(const BytecodeInfo *bcInfo, uint32_t bcIndex)
    {
        currentBcInfo = bcInfo;
        currentBcIndex = bcIndex;
        if (self->GetOptions()->GetCompilerArkSteedDeoptOnInsufficientProfile() &&
            bcInfo->IsInsufficientProfile()) {
            EmitUnconditionalDeopt();
            return false;
        }

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
                LowerLdTaggedConstant(JSTaggedValue::VALUE_UNDEFINED);
                break;
            case kungfu::EcmaOpcode::LDNULL:
                LowerLdTaggedConstant(JSTaggedValue::VALUE_NULL);
                break;
            case kungfu::EcmaOpcode::LDTRUE:
                LowerLdTaggedConstant(JSTaggedValue::VALUE_TRUE);
                break;
            case kungfu::EcmaOpcode::LDFALSE:
                LowerLdTaggedConstant(JSTaggedValue::VALUE_FALSE);
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
                LowerLdObjByName(bcInfo, bcIndex);
                break;
            case kungfu::EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
            case kungfu::EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
                LowerStObjByName(bcInfo, bcIndex);
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
                LowerLdObjByValue(bcInfo, bcIndex);
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
                LowerLdThisByValue(bcInfo, bcIndex);
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
                LowerCallInit(bcInfo);
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
        return true;
    }

    void EmitUnconditionalDeopt()
    {
        currentBlock->SetDeferred(true);
        constexpr auto DEOPT_TYPE = kungfu::DeoptType::INSUFFICIENTPROFILE;
        uint32_t bytecodeOffset = self->preproc_->GetBytecodeOffset(currentBcIndex);
        auto *deopt = self->FinishBlockWith<DeoptVertex>(
            currentBlock, {}, self->chunk_, DEOPT_TYPE, bytecodeOffset);
        deopt->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(currentBcIndex));
    }

    // -------- Category #2: Constant Loads --------

    void LowerLdTaggedConstant(JSTaggedType taggedValue)
    {
        frameState.SetAcc(self->graph_->GetTaggedConstant(taggedValue));
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
        compileInfoFacts_->EnsureType(res, NodeInfo::NodeType::STRING);
        frameState.SetAcc(res);
    }

    void LowerLdBigInt(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *numberBigInt = StringFromConstPool(stringId);
        RuntimeCallToAccWithLazyDeopt({numberBigInt}, RTSTUB_ID(LdBigInt));
    }

    // -------- Category #3: Unary Arithmetic --------

    void LowerInc()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::INC, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildUnaryOperation(CommonStubID::Inc);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerDec()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::DEC, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildUnaryOperation(CommonStubID::Dec);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerNeg()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::NEG, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildUnaryOperation(CommonStubID::Neg);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerNot()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::NOT, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildUnaryOperation(CommonStubID::Not);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    // -------- Category #4: Binary Arithmetic --------

    ValueVertex *TaggedConstantFromFoldedValue(JSTaggedValue value)
    {
        ValueVertex *constant = self->graph_->GetTaggedConstant(value.GetRawData());
        compileInfoFacts_->EnsureType(constant, NodeTypeFromJSTaggedValue(value));
        return constant;
    }

    void LowerAdd2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::ADD, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBinaryOperation(BinaryOpKind::ADD);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerSub2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::SUB, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBinaryOperation(BinaryOpKind::SUB);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerMul2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::MUL, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBinaryOperation(BinaryOpKind::MUL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerDiv2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::DIV, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBinaryOperation(BinaryOpKind::DIV);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerMod2(const BytecodeInfo * /*bcInfo*/)
    {
        ValueVertex *result = BuildBinaryOperation(BinaryOpKind::MOD);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerExp(const BytecodeInfo *bcInfo)
    {
        ValueVertex *left = LoadRegister(bcInfo, 0);
        ValueVertex *right = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({left, right}, RTSTUB_ID(Exp));
    }

    void LowerShl2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::SHL, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBitwiseOperation(IntBitwiseKind::SHIFT_LEFT);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerShr2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::SHR, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBitwiseOperation(IntBitwiseKind::SHIFT_RIGHT_LOGICAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerAshr2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::ASHR, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBitwiseOperation(IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerAnd2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::AND, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBitwiseOperation(IntBitwiseKind::BITWISE_AND);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerOr2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::OR, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBitwiseOperation(IntBitwiseKind::BITWISE_OR);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerXor2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::XOR, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        ValueVertex *result = BuildBitwiseOperation(IntBitwiseKind::BITWISE_XOR);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    // -------- Category #5: Comparisons --------

    bool TryFoldCompareAtBytecode(const BytecodeInfo *bcInfo, BinaryFoldOp foldOp)
    {
        ValueVertex *x = LoadRegister(bcInfo, 0);
        ValueVertex *y = frameState.GetAcc();
        JSTaggedValue folded;
        if (!TryFoldBinaryConstant(x, y, foldOp, &folded)) {
            return false;
        }
        frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
        return true;
    }

    void LowerEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::EQ)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::EQUAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerNotEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::NOT_EQ)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::NOT_EQUAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerLess(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::LESS)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::LESS_THAN);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerLessEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::LESS_EQ)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::LESS_THAN_OR_EQUAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerGreater(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::GREATER)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::GREATER_THAN);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerGreaterEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::GREATER_EQ)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::GREATER_THAN_OR_EQUAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerStrictNotEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::STRICT_NOT_EQ)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::STRICT_NOT_EQUAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerStrictEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::STRICT_EQ)) {
            return;
        }
        ValueVertex *result = BuildCompareOperation(JSCondition::STRICT_EQUAL);
        frameState.SetAcc(result);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, result);
    }

    void LowerIsTrue()
    {
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *result = nullptr;
        bool toBoolean = false;
        if (TryFoldToBooleanConstant(value, &toBoolean)) {
            uint64_t value = toBoolean ? JSTaggedValue::VALUE_TRUE : JSTaggedValue::VALUE_FALSE;
            result = TaggedConstantFromFoldedValue(JSTaggedValue(value));
        }
        if (result == nullptr) {
            result = TryBuildKnownIntToBoolean(value, true);
        }
        if (result == nullptr) {
            result = CommonStubCall({glue, value}, CommonStubID::ToBooleanTrue);
        }
        frameState.SetAcc(result);
    }

    void LowerIsFalse()
    {
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *result = nullptr;
        bool toBoolean = false;
        if (TryFoldToBooleanConstant(value, &toBoolean)) {
            uint64_t value = toBoolean ? JSTaggedValue::VALUE_FALSE : JSTaggedValue::VALUE_TRUE;
            result = TaggedConstantFromFoldedValue(JSTaggedValue(value));
        }
        if (result == nullptr) {
            result = TryBuildKnownIntToBoolean(value, false);
        }
        if (result == nullptr) {
            result = CommonStubCall({glue, value}, CommonStubID::ToBooleanFalse);
        }
        frameState.SetAcc(result);
    }

    // -------- Category #6: Type Conversions --------

    void LowerToNumber()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::TO_NUMBER, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::NUMBER)) {
            frameState.SetAcc(value);
            return;
        }
        RuntimeCallToAccWithLazyDeopt({value}, RTSTUB_ID(ToNumber));
    }

    void LowerToNumeric()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::TO_NUMERIC, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::NUMBER) ||
            compileInfoFacts_->CheckType(value, NodeInfo::NodeType::BIGINT)) {
            frameState.SetAcc(value);
            return;
        }
        RuntimeCallToAccWithLazyDeopt({value}, RTSTUB_ID(ToNumeric));
    }

    void LowerToPropertyKey()
    {
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({value}, RTSTUB_ID(ToPropertyKey));
    }

    // -------- Category #7: Property Access --------

    void LowerLdObjByName(const BytecodeInfo *bcInfo, uint32_t bcIndex)
    {
        ValueVertex *receiver = frameState.GetAcc();
        uint16_t constDataId = GetConstDataId(bcInfo, 1);
        if (TryBuildLoadNamedProperty(bcInfo, bcIndex, receiver, constDataId)) {
            return;
        }
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(constDataId));
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {receiver, id, GlobalEnv()}, CommonStubID::GetPropertyByName);
    }

    void LowerStObjByName(const BytecodeInfo *bcInfo, uint32_t bcIndex)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 2);  // 2: receiver register index
        uint16_t constDataId = GetConstDataId(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        auto serializingScope =
            self->pgoContext_.CreateSerializingScope("GraphBuilder::BytecodeVisitor::LowerStObjByName");
        NamedStoreAccessSet access;
        auto factory = self->pgoContext_.CreateAccessInfoFactory(*bcInfo);
        if (factory.TryBuildNamedStoreAccessInfo(0, &access) &&
            TryLowerNamedStoreAccessSet(bcIndex, access, receiver, value)) {
            return;
        }
        if (TryBuildStoreNamedProperty(bcIndex, receiver, constDataId, value)) {
            return;
        }
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(constDataId));
        CommonStubCallWithICAndLazyDeopt(bcInfo, {receiver, id, value, GlobalEnv()}, CommonStubID::SetPropertyByName);
    }

    void LowerLdObjByValue(const BytecodeInfo *bcInfo, uint32_t bcIndex)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *key = frameState.GetAcc();
        if (TryBuildLoadPropertyByValue(bcInfo, bcIndex, receiver, key)) {
            return;
        }
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {receiver, key, GlobalEnv()}, CommonStubID::GetPropertyByValue);
    }

    void LowerStObjByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *key = LoadRegister(bcInfo, 2);  // 2: key register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithICAndLazyDeopt(bcInfo, {receiver, key, value, GlobalEnv()}, CommonStubID::SetPropertyByValue);
    }

    void LowerLdObjByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        CommonStubCallToAccWithLazyDeopt({glue, receiver, index, GlobalEnv()}, CommonStubID::LdObjByIndex);
    }

    void LowerStObjByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, index, value, GlobalEnv()}, CommonStubID::StObjByIndex);
    }

    void LowerLdThisByValue(const BytecodeInfo *bcInfo, uint32_t bcIndex)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = frameState.GetAcc();
        if (TryBuildLoadPropertyByValue(bcInfo, bcIndex, receiver, key)) {
            return;
        }
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {receiver, key, GlobalEnv()}, CommonStubID::GetPropertyByValue);
    }

    void LowerStThisByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithICAndLazyDeopt(bcInfo, {receiver, key, value, GlobalEnv()}, CommonStubID::SetPropertyByValue);
    }

    void LowerLdThisByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {receiver, id, GlobalEnv()}, CommonStubID::GetPropertyByName);
    }

    void LowerStThisByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithICAndLazyDeopt(bcInfo, {receiver, id, value, GlobalEnv()}, CommonStubID::SetPropertyByName);
    }

    void LowerLdSuperByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({thisObj, propKey, jsFunc}, RTSTUB_ID(OptLdSuperByValue));
    }

    void LowerStSuperByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({thisObj, propKey, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
    }

    void LowerLdSuperByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = frameState.GetAcc();
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *prop = StringFromConstPool(stringId);
        RuntimeCallToAccWithLazyDeopt({thisObj, prop, jsFunc}, RTSTUB_ID(OptLdSuperByValue));
    }

    void LowerStSuperByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *thisObj = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *prop = StringFromConstPool(stringId);
        RuntimeCallWithLazyDeopt({thisObj, prop, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
    }

    void LowerStOwnByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *key = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, key, value, GlobalEnv()}, CommonStubID::StOwnByValue);
    }

    void LowerStOwnByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, index, value, GlobalEnv()}, CommonStubID::StOwnByIndex);
    }

    void LowerStOwnByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubID::StOwnByName);
    }

    void LowerStOwnByValueWithNameSet(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, propKey, accValue, GlobalEnv()},
                                    CommonStubID::StOwnByValueWithNameSet);
    }

    void LowerStOwnByNameWithNameSet(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, propKey, accValue, GlobalEnv()},
                                    CommonStubID::StOwnByNameWithNameSet);
    }

    void LowerTryLdGlobalByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {id, GlobalEnv()}, CommonStubID::TryLdGlobalByName);
    }

    void LowerTryStGlobalByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithICAndLazyDeopt(bcInfo, {id, value, GlobalEnv()}, CommonStubID::TryStGlobalByName);
    }

    void LowerLdGlobalVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {id, GlobalEnv()}, CommonStubID::LdGlobalVar);
    }

    void LowerStGlobalVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithICAndLazyDeopt(bcInfo, {id, value, GlobalEnv()}, CommonStubID::StGlobalVar);
    }

    void LowerStConstToGlobalRecord(const BytecodeInfo *bcInfo, bool isConst)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *value = frameState.GetAcc();
        ValueVertex *isConstGate = isConst
            ? self->graph_->GetTaggedConstant(JSTaggedValue::True().GetRawData())
            : self->graph_->GetTaggedConstant(JSTaggedValue::False().GetRawData());
        RuntimeCallWithLazyDeopt({propKey, value, isConstGate}, RTSTUB_ID(StGlobalRecord));
    }

    void LowerLdGlobal()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::JS_GLOBAL_OBJECT_INDEX * JSTaggedValue::TaggedTypeSize());

        frameState.SetAcc(self->NewVertex<LoadTaggedFieldVertex>(
            compileInfoFacts_, currentBlock, {GlobalEnv()}, offset));
    }

    void LowerLdSymbol()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::SYMBOL_FUNCTION_INDEX * JSTaggedValue::TaggedTypeSize());

        frameState.SetAcc(self->NewVertex<LoadTaggedFieldVertex>(
            compileInfoFacts_, currentBlock, {GlobalEnv()}, offset));
    }

    void LowerLdPrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);  // 3: lexicalEnv register index
        ValueVertex *obj = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(LdPrivateProperty));
    }

    void LowerStPrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *obj = LoadRegister(bcInfo, 3);  // 3: obj register index
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 4);  // 4: lexicalEnv register index
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(StPrivateProperty));
    }

    // -------- Category #8: Function Calls --------

    template <class InputRange = std::initializer_list<ValueVertex *>>
    CallVertex *BuildCallVertex(const InputRange &inputs, uint32_t actualArgc)
    {
        auto *call = self->NewVertex<CallVertex>(compileInfoFacts_, currentBlock, inputs, actualArgc);
        UpdateCatchBlockData(call);
        return call;
    }

    void LowerCallArg0()
    {
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, undefined}, CALL_ARG0);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallArg1(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, undefined, a0Value}, CALL_ARG1);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallArgs2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *a1Value = LoadRegister(bcInfo, 1);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, undefined, a0Value, a1Value}, CALL_ARG2);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallArgs3(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *a1Value = LoadRegister(bcInfo, 1);
        ValueVertex *a2Value = LoadRegister(bcInfo, 2);  // 2: third argument register index
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, undefined, a0Value, a1Value, a2Value}, CALL_ARG3);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallThis0(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, thisObj}, CALL_ARG0);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallInit(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, thisObj}, CALL_ARG0);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallThis1(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, thisObj, a0Value}, CALL_ARG1);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallThis2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *a1Value = LoadRegister(bcInfo, 2);  // 2: second argument register index
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, thisObj, a0Value, a1Value}, CALL_ARG2);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallThis3(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *a1Value = LoadRegister(bcInfo, 2);  // 2: second argument register index
        ValueVertex *a2Value = LoadRegister(bcInfo, 3);  // 3: third argument register index
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = BuildCallVertex({func, undefined, thisObj, a0Value, a1Value, a2Value}, CALL_ARG3);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallRange(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;
        ChunkVector<ValueVertex *> args(self->chunk_);
        args.push_back(func);
        args.push_back(undefined);
        args.push_back(undefined);
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(bcInfo, static_cast<int>(idx)));
        }
        CallVertex *call = BuildCallVertex(args, inputSize);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallThisRange(const BytecodeInfo *bcInfo)
    {
        // -1 : Skips the receiver
        uint32_t argc = bcInfo->inputs.size() - 1;
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *undefined = self->undefinedValue_;
        ChunkVector<ValueVertex *> args(self->chunk_);
        args.push_back(func);
        args.push_back(undefined);
        args.push_back(thisObj);
        for (uint32_t idx = 0; idx < argc; idx++) {
            args.push_back(LoadRegister(bcInfo, static_cast<int>(idx + 1)));
        }
        CallVertex *call = BuildCallVertex(args, argc);
        frameState.SetAcc(call);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, call);
    }

    void LowerCallSpread(const BytecodeInfo *bcInfo)
    {
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *thisArg = LoadRegister(bcInfo, 0);
        ValueVertex *argsArray = LoadRegister(bcInfo, 1);

        RuntimeCallToAccWithLazyDeopt({func, thisArg, argsArray}, RTSTUB_ID(CallSpread));
    }

    void LowerSuperCallThisRange(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();

        ValueVertex *thisFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedInputSize = TaggedConstantFromInt32(static_cast<int>(inputSize));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(bcInfo, taggedInputSize, inputSize);

        RuntimeCallToAccWithLazyDeopt({thisFunc, newTarget, taggedArray, taggedInputSize}, RTSTUB_ID(OptSuperCall));
    }

    void LowerSuperCallArrowRange(const BytecodeInfo *bcInfo)
    {
        uint32_t argc = bcInfo->inputs.size();
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedArgc = TaggedConstantFromInt32(static_cast<int>(argc));
        ValueVertex *taggedArray = TaggedArrayFromValueIn(bcInfo, taggedArgc, argc);

        RuntimeCallToAccWithLazyDeopt({func, newTarget, taggedArray, taggedArgc}, RTSTUB_ID(OptSuperCall));
    }

    void LowerSuperCallSpread(const BytecodeInfo *bcInfo)
    {
        ValueVertex *array = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);

        ValueVertex *argsArray = CommonStubCall({glue, array, GlobalEnv()}, CommonStubID::GetCallSpreadArgs);
        RuntimeCallToAccWithLazyDeopt({func, newTarget, argsArray}, RTSTUB_ID(OptSuperCallSpread));
    }

    void LowerSuperCallForwardAllArgs(const BytecodeInfo *bcInfo)
    {
        ValueVertex *func = LoadRegister(bcInfo, 0);
        ValueVertex *superFunc = CommonStubCall({glue, func}, CommonStubID::GetPrototype);
        ValueVertex *newTarget = LoadParam(NEW_TARGET_PARAM_INDEX);
        ValueVertex *taggedActualArgc = TaggedActualArgc();

        RuntimeCallToAccWithLazyDeopt({superFunc, newTarget, taggedActualArgc}, RTSTUB_ID(OptSuperCallForwardAllArgs));
    }

    void LowerNewObjApply(const BytecodeInfo *bcInfo)
    {
        ValueVertex *target = LoadRegister(bcInfo, 0);
        ValueVertex *args = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({target, args}, RTSTUB_ID(NewObjApply));
    }

    void LowerNewObjRange(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();
        ChunkVector<ValueVertex *> args(self->chunk_);
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(bcInfo, idx));
        }
        RuntimeCallToAccWithLazyDeopt(args, RTSTUB_ID(OptNewObjRange));
    }

    // -------- Category #9: Object/Array Creation --------

    void LowerCreateEmptyObject()
    {
        RuntimeCallToAccWithLazyDeopt({}, RTSTUB_ID(CreateEmptyObject));
    }

    void LowerCreateEmptyArray()
    {
        CommonStubCallToAccWithLazyDeopt({glue, GlobalEnv()}, CommonStubID::CreateEmptyArray);
    }

    void LowerCreateObjectWithBuffer(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *obj = ObjectFromConstPool(index);
        ValueVertex *lexEnv = LoadRegister(bcInfo, 1);
        CommonStubCallToAccWithLazyDeopt({glue, obj, lexEnv}, CommonStubID::CreateObjectHavingMethod);
    }

    void LowerCreateObjectWithExcludedKeys(const BytecodeInfo *bcInfo)
    {
        uint32_t inputSize = bcInfo->inputs.size();
        ChunkVector<ValueVertex *> args(self->chunk_);
        for (uint32_t idx = 0; idx < inputSize; idx++) {
            args.push_back(LoadRegister(bcInfo, idx));
        }
        RuntimeCallToAccWithLazyDeopt(args, RTSTUB_ID(OptCreateObjectWithExcludedKeys));
    }

    void LowerCreateArrayWithBuffer(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *slotId = self->graph_->GetInt32Constant(GetICSlotId<int>(bcInfo, 1));

        CommonStubCallToAccWithLazyDeopt(
            {glue, index, jsFunc, slotId, GlobalEnv()}, CommonStubID::CreateArrayWithBuffer);
    }

    void LowerCreateRegExpWithLiteral(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *pattern = StringFromConstPool(stringId);
        ValueVertex *flags = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));

        RuntimeCallToAccWithLazyDeopt({pattern, flags}, RTSTUB_ID(CreateRegExpWithLiteral));
    }

    void LowerCreateIterResultObj(const BytecodeInfo *bcInfo)
    {
        ValueVertex *value = LoadRegister(bcInfo, 0);
        ValueVertex *done = LoadRegister(bcInfo, 1);
        RuntimeCallToAccWithLazyDeopt({value, done}, RTSTUB_ID(CreateIterResultObj));
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

        RuntimeCallToAccWithLazyDeopt(
            {method, homeObject, length, env, module, slotId, jsFunc}, RTSTUB_ID(DefineMethod));
#else
        RuntimeCallToAcc({method, homeObject, length, env, module}, RTSTUB_ID(DefineMethod));
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

        CommonStubCallToAccWithLazyDeopt(
            {glue, jsFunc, methodId, length, lexicalEnv, slotId, GlobalEnv()}, CommonStubID::Definefunc);
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

        RuntimeCallToAccWithLazyDeopt(
            {proto, lexicalEnv, sharedConstPool, methodId, literalId, module, length, slotId, jsFunc},
            RTSTUB_ID(CreateClassWithBuffer));
#else
        RuntimeCallToAcc(
            {proto, lexicalEnv, sharedConstPool, methodId, literalId, module, length},
            RTSTUB_ID(CreateClassWithBuffer));
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

        RuntimeCallToAccWithLazyDeopt(
            {obj, prop, getter, setter, acc, undefinedValue, taggedOne}, RTSTUB_ID(DefineGetterSetterByValue));
    }

    void LowerDefinePropertyByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 1));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, obj, prop, value, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerDefineFieldByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 1));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, obj, prop, value, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerDefineFieldByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *propKey = LoadRegister(bcInfo, 0);
        ValueVertex *acc = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, propKey, acc, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerDefineFieldByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *propKey = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *acc = frameState.GetAcc();
        CommonStubCallWithLazyDeopt({glue, receiver, propKey, acc, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerCreatePrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *count = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *literalId = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 2);  // 2: lexicalEnv register index
        ValueVertex *constpool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();

        RuntimeCallWithLazyDeopt({lexicalEnv, count, constpool, literalId, module}, RTSTUB_ID(CreatePrivateProperty));
    }

    void LowerDefinePrivateProperty(const BytecodeInfo *bcInfo)
    {
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);  // 3: lexicalEnv register index
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(DefinePrivateProperty));
    }

    // -------- Category #11: Iterators --------

    void LowerGetIterator()
    {
        ValueVertex *obj = frameState.GetAcc();
        CommonStubCallToAccWithLazyDeopt({glue, obj, GlobalEnv()}, CommonStubID::GetIterator);
    }

    void LowerGetPropIterator()
    {
        ValueVertex *object = frameState.GetAcc();
        CommonStubCallToAccWithLazyDeopt({glue, object, GlobalEnv()}, CommonStubID::Getpropiterator);
    }

    void LowerCloseIterator(const BytecodeInfo *bcInfo)
    {
        ValueVertex *iterator = LoadRegister(bcInfo, 0);
        RuntimeCallToAccWithLazyDeopt({iterator}, RTSTUB_ID(CloseIterator));
    }

    void LowerGetNextPropName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *iterator = LoadRegister(bcInfo, 0);
        RuntimeCallToAccWithLazyDeopt({iterator}, RTSTUB_ID(GetNextPropNameSlowpath));
    }

    // -------- Category #12: Lexical Environment --------

    int32_t GetLexicalEnvSlotOffset(uint16_t slot) const
    {
        return static_cast<int32_t>(TaggedArray::DATA_OFFSET +
                                    (LexicalEnv::RESERVED_ENV_LENGTH + slot) * JSTaggedValue::TaggedTypeSize());
    }

    int32_t GetLexicalEnvParentOffset() const
    {
        return static_cast<int32_t>(TaggedArray::DATA_OFFSET +
                                    LexicalEnv::PARENT_ENV_INDEX * JSTaggedValue::TaggedTypeSize());
    }

    ValueVertex *BuildEnvSlotLoad(ValueVertex *env, int32_t offset)
    {
        ASSERT(env != nullptr);
        bool isConstantField = IsEnvConstantFieldOffset(offset);
        ValueVertex *cached = nullptr;
        if (isConstantField) {
            cached = compileInfoFacts_->LookupEnvConstant(env, offset);
        } else {
            cached = compileInfoFacts_->LookupEnvSlot(env, offset);
        }
        if (cached != nullptr) {
            return cached;
        }

        ValueVertex *value = self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {env}, offset);
        if (isConstantField) {
            compileInfoFacts_->RecordEnvConstant(env, offset, value);
        } else {
            compileInfoFacts_->RecordEnvSlot(env, offset, value);
        }
        return value;
    }

    ValueVertex *BuildLexicalEnvAtLevel(ValueVertex *baseEnv, uint16_t level)
    {
        ValueVertex *env = baseEnv;
        for (uint16_t i = 0; i < level; ++i) {
            env = BuildEnvSlotLoad(env, GetLexicalEnvParentOffset());
        }
        return env;
    }

    void LowerNewLexicalEnv(const BytecodeInfo *bcInfo)
    {
        ValueVertex *parent = LoadRegister(bcInfo, 1);
        ValueVertex *numVars = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        ValueVertex *newEnv = CommonStubCall(
            {glue, parent, numVars}, CommonStubID::NewLexicalEnv, SideEffectKind::SAFE_CALL);

        frameState.SetAcc(newEnv);
        frameState.SetLexicalEnv(newEnv);
        compileInfoFacts_->RecordEnvConstant(newEnv, GetLexicalEnvParentOffset(), parent);
    }

    void LowerNewLexicalEnvWithName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *level = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *slotId = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        ValueVertex *parent = LoadRegister(bcInfo, 2);  // 2: env register index
        ValueVertex *newEnv = RuntimeCall(
            {level, slotId, parent, jsFunc}, RTSTUB_ID(OptNewLexicalEnvWithName), SideEffectKind::SAFE_CALL);

        frameState.SetAcc(newEnv);
        frameState.SetLexicalEnv(newEnv);
        compileInfoFacts_->RecordEnvConstant(newEnv, GetLexicalEnvParentOffset(), parent);
    }

    void LowerPopLexicalEnv(const BytecodeInfo *bcInfo)
    {
        ValueVertex *currentEnv = LoadRegister(bcInfo, 0);
        ValueVertex *parentEnv = BuildEnvSlotLoad(currentEnv, GetLexicalEnvParentOffset());

        frameState.SetAcc(parentEnv);
        frameState.SetLexicalEnv(parentEnv);
        compileInfoFacts_->ClearEnvSlotsFor(currentEnv);
    }

    void LowerLdLexVar(const BytecodeInfo *bcInfo)
    {
        uint16_t level = GetImmediate<uint16_t>(bcInfo, 0);
        uint16_t slot = GetImmediate<uint16_t>(bcInfo, 1);
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 2);  // 2: lexicalEnv register index
        ValueVertex *targetEnv = BuildLexicalEnvAtLevel(lexicalEnv, level);
        frameState.SetAcc(BuildEnvSlotLoad(targetEnv, GetLexicalEnvSlotOffset(slot)));
    }

    void LowerStLexVar(const BytecodeInfo *bcInfo)
    {
        uint16_t level = GetImmediate<uint16_t>(bcInfo, 0);
        uint16_t slot = GetImmediate<uint16_t>(bcInfo, 1);
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 2);  // 2: lexicalEnv register index
        ValueVertex *value = frameState.GetAcc();
        ASSERT(value != nullptr);
        ValueVertex *targetEnv = BuildLexicalEnvAtLevel(lexicalEnv, level);
        int32_t offset = GetLexicalEnvSlotOffset(slot);
        self->NewVertex<StoreEnvSlotVertex>(compileInfoFacts_, currentBlock, {targetEnv, value}, offset);
        self->NewVertex<SetValueWithBarrierVertex>(compileInfoFacts_, currentBlock, {glue, targetEnv, value}, offset);
        compileInfoFacts_->RecordEnvSlot(targetEnv, offset, value);
    }

    // -------- Category #13: Modules --------

    void LowerLdExternalModuleVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        RuntimeCallToAccWithLazyDeopt({index, jsFunc}, RTSTUB_ID(LdExternalModuleVarByIndexOnJSFunc));
    }

    void LowerGetModuleNamespace(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        RuntimeCallToAccWithLazyDeopt({index, jsFunc}, RTSTUB_ID(GetModuleNamespaceByIndexOnJSFunc));
    }

    void LowerLdLocalModuleVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        RuntimeCallToAccWithLazyDeopt({index, jsFunc}, RTSTUB_ID(LdLocalModuleVarByIndexOnJSFunc));
    }

    void LowerStModuleVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({index, value, jsFunc}, RTSTUB_ID(StModuleVarByIndexOnJSFunc));
    }

    void LowerDynamicImport()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *specifier = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({specifier, jsFunc}, RTSTUB_ID(DynamicImport));
    }

    void LowerLdPatchVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        RuntimeCallToAccWithLazyDeopt({index}, RTSTUB_ID(LdPatchVar));
    }

    void LowerStPatchVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({index, value}, RTSTUB_ID(StPatchVar));
    }

    // -------- Category #14: Miscellaneous --------

    void LowerTypeOf()
    {
        ValueVertex *obj = frameState.GetAcc();
        CommonStubCallToAccWithLazyDeopt({glue, obj}, CommonStubID::TypeOf);
    }

    void LowerGetUnmappedArgs()
    {
        ValueVertex *argv = self->graph_->GetIntPtrConstant(0);
        ValueVertex *numArgs = ActualArgc();
        ValueVertex *argvTaggedArray = self->undefinedValue_;

        CommonStubCallToAccWithLazyDeopt(
            {glue, argv, numArgs, argvTaggedArray, GlobalEnv()}, CommonStubID::GetUnmappedArgs);
    }

    void LowerCopyRestArgs(const BytecodeInfo *bcInfo)
    {
        ValueVertex *taggedArgc = TaggedActualArgc();
        ValueVertex *taggedRestIdx = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        RuntimeCallToAccWithLazyDeopt({taggedArgc, taggedRestIdx}, RTSTUB_ID(OptCopyRestArgs));
    }

    void LowerDelObjProp(const BytecodeInfo *bcInfo)
    {
        ValueVertex *object = LoadRegister(bcInfo, 0);
        ValueVertex *prop = frameState.GetAcc();
        CommonStubCallToAccWithLazyDeopt({glue, object, prop, GlobalEnv()}, CommonStubID::DeleteObjectProperty);
    }

    void LowerIsIn(const BytecodeInfo *bcInfo)
    {
        ValueVertex *prop = LoadRegister(bcInfo, 0);
        ValueVertex *obj = frameState.GetAcc();
        CommonStubCallToAccWithLazyDeopt({glue, prop, obj, GlobalEnv()}, CommonStubID::IsIn);
    }

    void LowerInstanceOf(const BytecodeInfo *bcInfo)
    {
        ValueVertex *object = LoadRegister(bcInfo, 1);
        ValueVertex *target = frameState.GetAcc();
        CommonStubCallToAccWithICAndLazyDeopt(bcInfo, {object, target, GlobalEnv()}, CommonStubID::Instanceof);
    }

    void LowerTestIn(const BytecodeInfo *bcInfo)
    {
        // 1: level operand index
        ValueVertex *levelIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 1));
        // 2: slot operand index
        ValueVertex *slotIndex = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 2));
        ValueVertex *lexicalEnv = LoadRegister(bcInfo, 3);  // 3: lexicalEnv register index
        ValueVertex *obj = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(TestIn));
    }

    void LowerCopyDataProperties(const BytecodeInfo *bcInfo)
    {
        ValueVertex *target = LoadRegister(bcInfo, 0);
        ValueVertex *source = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({target, source}, RTSTUB_ID(CopyDataProperties));
    }

    void LowerStoreArraySpread(const BytecodeInfo *bcInfo)
    {
        ValueVertex *array = LoadRegister(bcInfo, 0);
        ValueVertex *index = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({array, index, value}, RTSTUB_ID(StArraySpread));
    }

    void LowerGetTemplateObject()
    {
        ValueVertex *value = frameState.GetAcc();
        RuntimeCallToAccWithLazyDeopt({value}, RTSTUB_ID(GetTemplateObject));
    }

    void LowerSetObjectWithProto(const BytecodeInfo *bcInfo)
    {
        ValueVertex *proto = LoadRegister(bcInfo, 0);
        ValueVertex *obj = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({proto, obj}, RTSTUB_ID(SetObjectWithProto));
    }

    void LowerNotifyConcurrentResult()
    {
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *result = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({result, jsFunc}, RTSTUB_ID(NotifyConcurrentResult));
    }

    // -------- Category #15: Exceptions --------

    bool TryBuildColdCatchDeopt()
    {
        if (!self->IsLazyDeoptEnabled() || !HasNeverExecutedCatchBlock()) {
            return false;
        }
        constexpr auto DEOPT_TYPE = kungfu::DeoptType::INSUFFICIENTPROFILE;
        auto *deopt = self->FinishBlockWith<DeoptVertex>(currentBlock, {}, self->chunk_, DEOPT_TYPE, currentBcIndex);
        deopt->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(currentBcIndex));
        return true;
    }

    void LowerThrow()
    {
        if (!TryBuildColdCatchDeopt()) {
            ValueVertex *exception = frameState.GetAcc();
            auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {exception}, RTSTUB_ID(Throw));
            UpdateCatchBlockData(vertex);
        }
    }

    void LowerThrowConstAssignment(const BytecodeInfo *bcInfo)
    {
        if (!TryBuildColdCatchDeopt()) {
            ValueVertex *value = LoadRegister(bcInfo, 0);
            auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {value}, RTSTUB_ID(ThrowConstAssignment));
            UpdateCatchBlockData(vertex);
        }
    }

    void LowerThrowNotExists()
    {
        if (!TryBuildColdCatchDeopt()) {
            auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {}, RTSTUB_ID(ThrowThrowNotExists));
            UpdateCatchBlockData(vertex);
        }
    }

    void LowerThrowPatternNonCoercible()
    {
        if (!TryBuildColdCatchDeopt()) {
            auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {}, RTSTUB_ID(ThrowPatternNonCoercible));
            UpdateCatchBlockData(vertex);
        }
    }

    void LowerThrowDeleteSuperProperty()
    {
        if (!TryBuildColdCatchDeopt()) {
            auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {}, RTSTUB_ID(ThrowDeleteSuperProperty));
            UpdateCatchBlockData(vertex);
        }
    }

    void LowerThrowIfNotObject(const BytecodeInfo *bcInfo)
    {
        ValueVertex *value = LoadRegister(bcInfo, 0);

        BB *isHeapObjectBlock = self->NewBlock();
        BB *checkLowerDoneBlock = self->NewBlock();
        BB *checkUpperDoneBlock = self->NewBlock();

        // Note: each failure branch requires an independent throwing block to prevent critical edges in the subgraph.
        BB *notHeapObjectBlock = self->NewBlock();
        BB *checkLowerFailedBlock = self->NewBlock();
        BB *checkUpperFailedBlock = self->NewBlock();

        self->FinishBlockWithBranch<BranchIfTaggedHeapObjectVertex>(
            currentBlock, {value}, isHeapObjectBlock, notHeapObjectBlock);

        // Hot path: value is a heap object → check HClass type range inline.
        currentBlock = isHeapObjectBlock;

        ValueVertex *hclass = self->NewVertex<LoadTaggedFieldVertex>(
            compileInfoFacts_, currentBlock, {value}, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
        ValueVertex *hclassRaw = self->NewVertex<TaggedToRawI64Vertex>(compileInfoFacts_, currentBlock, {hclass});

        ValueVertex *addrMask = self->graph_->GetInt64Constant(static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
        ValueVertex *hclassMasked = self->NewVertex<I64BitwiseBinaryVertex>(
            compileInfoFacts_, currentBlock, {hclassRaw, addrMask}, IntBitwiseKind::BITWISE_AND);

        ValueVertex *bitField = self->NewVertex<LoadTaggedFromAddressVertex>(
            compileInfoFacts_, currentBlock, {hclassMasked}, static_cast<int32_t>(JSHClass::BIT_FIELD_OFFSET));
        ValueVertex *bitFieldRaw = self->NewVertex<TaggedToRawI64Vertex>(compileInfoFacts_, currentBlock, {bitField});

        // Type is encoded to the first 8 bits of JSHClass::bitfield
        static_assert(JSHClass::ObjectTypeBits::START_BIT == 0);
        ValueVertex *typeMask = self->graph_->GetInt64Constant((1U << JSHClass::ObjectTypeBits::SIZE) - 1);
        ValueVertex *typeBits = self->NewVertex<I64BitwiseBinaryVertex>(
            compileInfoFacts_, currentBlock, {bitFieldRaw, typeMask}, IntBitwiseKind::BITWISE_AND);

        // Whether type is in [ECMA_OBJECT_FIRST, ECMA_OBJECT_LAST]
        ValueVertex *firstType = self->graph_->GetInt64Constant(static_cast<int64_t>(JSType::ECMA_OBJECT_FIRST));
        self->FinishBlockWithBranch<BranchIfInt64CompareVertex>(
            currentBlock, {typeBits, firstType},
            checkLowerDoneBlock, checkLowerFailedBlock, Condition::GREATER_THAN_OR_EQUAL);

        currentBlock = checkLowerDoneBlock;
        ValueVertex *lastType = self->graph_->GetInt64Constant(static_cast<int64_t>(JSType::ECMA_OBJECT_LAST));
        self->FinishBlockWithBranch<BranchIfInt64CompareVertex>(
            currentBlock, {typeBits, lastType},
            checkUpperDoneBlock, checkUpperFailedBlock, Condition::LESS_THAN_OR_EQUAL);

        for (BB *exceptionBlock : {notHeapObjectBlock, checkLowerFailedBlock, checkUpperFailedBlock}) {
            currentBlock = exceptionBlock;
            currentBlock->SetDeferred(true);
            if (!TryBuildColdCatchDeopt()) {
                auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {}, RTSTUB_ID(ThrowIfNotObject));
                UpdateCatchBlockData(vertex);
            }
        }

        // Success: value is an ECMA object.
        currentBlock = checkUpperDoneBlock;
    }

    // WARNING: THIS BYTECODE IS POSSIBLY INACTIVATED AND IS GUARDED BY NO TEST CASES.
    void LowerThrowUndefinedIfHole(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *obj = LoadRegister(bcInfo, 1);

        BB *throwBlock = self->NewBlock();
        BB *doneBlock = self->NewBlock();

        ValueVertex *hole = self->graph_->GetTaggedConstant(JSTaggedValue::VALUE_HOLE);
        self->FinishBlockWith<BranchIfReferenceEqualVertex>(currentBlock, {receiver, hole}, throwBlock, doneBlock);

        currentBlock = throwBlock;
        currentBlock->SetDeferred(true);
        if (!TryBuildColdCatchDeopt()) {
            auto *throwVertex = self->FinishBlockWith<ThrowVertex>(
                currentBlock, {obj}, RTSTUB_ID(ThrowUndefinedIfHole));
            UpdateCatchBlockData(throwVertex);
        }
        currentBlock = doneBlock;
    }

    void LowerThrowUndefinedIfHoleWithName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = frameState.GetAcc();
        ValueVertex *strID = self->graph_->GetInt32Constant(GetICSlotId<int>(bcInfo, 0));
        ValueVertex *str = StringFromConstPool(strID);

        BB *throwBlock = self->NewBlock();
        BB *doneBlock = self->NewBlock();

        ValueVertex *hole = self->graph_->GetTaggedConstant(JSTaggedValue::VALUE_HOLE);
        self->FinishBlockWith<BranchIfReferenceEqualVertex>(currentBlock, {receiver, hole}, throwBlock, doneBlock);

        currentBlock = throwBlock;
        currentBlock->SetDeferred(true);
        if (!TryBuildColdCatchDeopt()) {
            auto *throwVertex = self->FinishBlockWith<ThrowVertex>(
                currentBlock, {str}, RTSTUB_ID(ThrowUndefinedIfHole));
            UpdateCatchBlockData(throwVertex);
        }

        currentBlock = doneBlock;
    }

    void LowerThrowIfSuperNotCorrectCall(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *thisValue = frameState.GetAcc();
        RuntimeCallWithLazyDeopt({index, thisValue}, RTSTUB_ID(ThrowIfSuperNotCorrectCall));
    }

    // -------- Category #16: Control Flow --------

    void LowerJumpIfZero()
    {
        ValueVertex *acc = frameState.GetAcc();
        if (auto *asConstant = acc->TryCast<TaggedConstantVertex>(); asConstant != nullptr) {
            uint64_t rawValue = asConstant->GetValue();

            if (rawValue == JSTaggedValue::VALUE_TRUE) {
                LOG_COMPILER(DEBUG) << "LowerJumpIfZero(): TRUE -> Fallthrough";
                self->FinishBlockWithJump(currentBlock, FallthroughTarget());
                return;
            }
            if (rawValue == JSTaggedValue::VALUE_FALSE) {
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
        if (auto *asConstant = acc->TryCast<TaggedConstantVertex>(); asConstant != nullptr) {
            uint64_t rawValue = asConstant->GetValue();

            if (rawValue == JSTaggedValue::VALUE_TRUE) {
                LOG_COMPILER(DEBUG) << "LowerJumpIfNonZero(): TRUE -> Jump";
                self->FinishBlockWithJump(currentBlock, JumpTarget());
                return;
            }
            if (rawValue == JSTaggedValue::VALUE_FALSE) {
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

    // -------- Deoptimization Helpers --------

    using LazyDeoptFrameState = LazyDeoptimizableMixin::LazyDeoptFrameState;
    using EagerDeoptFrameState = EagerDeoptimizableMixin::EagerDeoptFrameState;

    DeoptTranslationKind GetDeoptValueKind(ValueVertex *value) const
    {
        switch (value->GetValueRepresentation()) {
            case ValueRepresentation::TAGGED:
                return DeoptTranslationKind::TAGGED;
            case ValueRepresentation::INT32:
                return DeoptTranslationKind::INT32_TO_TAGGED;
            case ValueRepresentation::FLOAT64:
            case ValueRepresentation::HOLEY_FLOAT64:
                return DeoptTranslationKind::FLOAT64_TO_TAGGED_DOUBLE;
            case ValueRepresentation::UINT32:
            case ValueRepresentation::INT64:
            case ValueRepresentation::NONE:
                break;
        }
        UNREACHABLE();
    }

    template <class DeoptFrameState>
    void AppendDeoptInput(DeoptFrameState *deoptFrameState, int32_t id, ValueVertex *value)
    {
        ValueVertex *frameValue = value == nullptr ? self->undefinedValue_ : value;
        deoptFrameState->emplace_back(id, frameValue, GetDeoptValueKind(frameValue));
    }

    template <class DeoptFrameState>
    void AppendCommonDeoptInputs(uint32_t bcIndex, DeoptFrameState *deoptFrameState)
    {
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::FUNC_INDEX),
                         LoadParam(CALL_TARGET_PARAM_INDEX));
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::NEWTARGET_INDEX),
                         LoadParam(NEW_TARGET_PARAM_INDEX));
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::THIS_OBJECT_INDEX),
                         LoadParam(THIS_OBJECT_PARAM_INDEX));
        int32_t bcOffset = static_cast<int32_t>(self->preproc_->GetBytecodeOffset(bcIndex));
        deoptFrameState->emplace_back(static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX),
                                      self->graph_->GetInt32Constant(bcOffset), DeoptTranslationKind::RAW_INT32);
    }

    void AppendLazyCommonDeoptInputs(uint32_t bcIndex, LazyDeoptFrameState *deoptFrameState)
    {
        AppendCommonDeoptInputs(bcIndex, deoptFrameState);
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::ACTUAL_ARGC_INDEX), TaggedActualArgc());
    }

    void AppendEagerCommonDeoptInputs(uint32_t bcIndex, EagerDeoptFrameState *deoptFrameState)
    {
        AppendCommonDeoptInputs(bcIndex, deoptFrameState);
        ValueVertex *lexicalEnv = frameState.GetLexicalEnv();
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::ENV_INDEX),
                         lexicalEnv == self->initialLexicalEnv_ ? self->undefinedValue_ : lexicalEnv);
        deoptFrameState->emplace_back(static_cast<int32_t>(SpecVregIndex::ACTUAL_ARGC_INDEX),
                                      self->initialActualArgc_, DeoptTranslationKind::INT32_TO_TAGGED);
    }

    template <class DeoptFrameState, class Predicate>
    void AppendLiveLocalsAndParams(DeoptFrameState *deoptFrameState, VRegIDType firstParamIndex, Predicate predicate)
    {
        for (VRegIDType i = 0; i < self->numLocal_; i++) {
            VRegIDType localIndex = VRegOfLocal(i);
            if (predicate(localIndex)) {
                AppendDeoptInput(deoptFrameState, static_cast<int32_t>(localIndex), frameState.Get(localIndex));
            }
        }
        for (VRegIDType i = firstParamIndex; i < self->numParams_; i++) {
            VRegIDType paramIndex = VRegOfParam(self->numLocal_, i);
            if (predicate(paramIndex)) {
                AppendDeoptInput(deoptFrameState, static_cast<int32_t>(paramIndex), LoadParam(i));
            }
        }
    }

    // D refers to dependency
    void BuildLazyDeoptInputsForDOnly(uint32_t bcIndex, LazyDeoptFrameState *deoptFrameState)
    {
        const BytecodeInfo *bcInfo = self->preproc_->GetBytecode(bcIndex);
        const auto &liveOut = self->analysis_->GetLiveOutOfBytecode(bcIndex);

        auto isInVRegOut = [bcInfo](VRegIDType index) {
            return std::find(bcInfo->vregOut.begin(), bcInfo->vregOut.end(), index) != bcInfo->vregOut.end();
        };

        AppendLazyCommonDeoptInputs(bcIndex, deoptFrameState);
        VRegIDType envIndex = self->LexicalEnvIndex();
        if (!bcInfo->EnvOut() && !isInVRegOut(envIndex)) {
            constexpr int32_t ENV_INDEX = static_cast<int32_t>(SpecVregIndex::ENV_INDEX);
            AppendDeoptInput(deoptFrameState, ENV_INDEX, frameState.GetLexicalEnv());
        }
        if (!bcInfo->AccOut()) {
            constexpr int32_t ACC_INDEX = static_cast<int32_t>(SpecVregIndex::ACC_INDEX);
            AppendDeoptInput(deoptFrameState, ACC_INDEX, frameState.GetAcc());
        }
        AppendLiveLocalsAndParams(deoptFrameState, 0, [&liveOut, &isInVRegOut](VRegIDType index) {
            return liveOut.TestBit(index) && !isInVRegOut(index);
        });
    }

    // E refers to exception
    void BuildLazyDeoptInputsForEOnly(uint32_t bcIndex, LazyDeoptFrameState *deoptFrameState)
    {
        ASSERT(blockInfo->catchBlock != nullptr);
        // Exception lazy-deopt resumes at the catch handler entry. ACC is restored as the exception object by
        // the lazy-deopt trampoline, not from this payload.
        const kungfu::BitSet &catchLiveIn = self->analysis_->GetLiveInOfBlock(blockInfo->catchBlock->rpoIndex);
        AppendLazyCommonDeoptInputs(bcIndex, deoptFrameState);
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::ENV_INDEX), frameState.GetLexicalEnv());
        AppendLiveLocalsAndParams(deoptFrameState, 0, [&catchLiveIn](VRegIDType index) {
            return catchLiveIn.TestBit(index);
        });
    }

    // D+E (dependency + exception) lazy deopt.
    // Lazy-deopt takes live-out - {ACC} which is equivalent to live-in - {ACC} with Ark bytecode.
    void BuildLazyDeoptInputsForDE(uint32_t bcIndex, LazyDeoptFrameState *deoptFrameState, bool includeAcc)
    {
        kungfu::BitSet liveSet(self->chunk_, self->analysis_->GetNumVRegs());
        liveSet.CopyFrom(self->analysis_->GetLiveInOfBytecode(bcIndex));
        if (blockInfo->catchBlock != nullptr) {
            liveSet.Union(self->analysis_->GetLiveInOfBlock(blockInfo->catchBlock->rpoIndex));
        }
        AppendLazyCommonDeoptInputs(bcIndex, deoptFrameState);
        AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::ENV_INDEX), frameState.GetLexicalEnv());
        if (includeAcc) {
            AppendDeoptInput(deoptFrameState, static_cast<int32_t>(SpecVregIndex::ACC_INDEX), frameState.GetAcc());
        }
        AppendLiveLocalsAndParams(deoptFrameState, 0, [&liveSet](VRegIDType index) {
            return liveSet.TestBit(index);
        });
    }

    EagerDeoptFrameState BuildCurrentEagerDeoptFrameState(uint32_t bcIndex)
    {
        EagerDeoptFrameState frameStateValues {self->chunk_};
        kungfu::BitSet liveSet(self->chunk_, self->analysis_->GetNumVRegs());
        liveSet.CopyFrom(self->analysis_->GetLiveInOfBytecode(bcIndex));
        if (blockInfo->catchBlock != nullptr) {
            liveSet.Union(self->analysis_->GetLiveInOfBlock(blockInfo->catchBlock->rpoIndex));
        }

        AppendEagerCommonDeoptInputs(bcIndex, &frameStateValues);
        if (liveSet.TestBit(self->AccIndex())) {
            AppendDeoptInput(&frameStateValues, static_cast<int32_t>(SpecVregIndex::ACC_INDEX), frameState.GetAcc());
        }
        AppendLiveLocalsAndParams(&frameStateValues, FIXED_PARAM_VREG_COUNT, [&liveSet](VRegIDType index) {
            return liveSet.TestBit(index);
        });
        return frameStateValues;
    }

    template <class CallT>
    bool InputMayBeJSReceiver(CallT *call, size_t index) const
    {
        constexpr auto JS_RECEIVER = NodeInfo::NodeType::JS_RECEIVER;
        const NodeInfo::NodeType knownType = compileInfoFacts_->GetKnownType(call->GetInput(index));
        return NodeInfo::NodeTypeCanBe(knownType, JS_RECEIVER);
    }

    bool IsDependencySafeCommonStubCall(CallCommonStubVertex *call) const
    {
        constexpr auto JS_RECEIVER = NodeInfo::NodeType::JS_RECEIVER;
        switch (call->GetCommonStubID()) {
            // Strict equality is a reference/value comparison and never runs user-defined conversion.
            case CommonStubID::StrictEqual:
            case CommonStubID::StrictNotEqual:
                return true;
            // Boolean conversion follows ToBoolean and does not call user-defined conversion hooks.
            case CommonStubID::ToBooleanTrue:
            case CommonStubID::ToBooleanFalse:
                return true;
            // typeof is observable only through the input value category and does not mutate dependencies.
            case CommonStubID::TypeOf:
                return true;
            // These allocation/constant-pool helpers create or load values without observing receiver hooks.
            case CommonStubID::CreateArrayWithBuffer:
            case CommonStubID::CreateEmptyArray:
            case CommonStubID::CreateObjectHavingMethod:
            case CommonStubID::Definefunc:
            case CommonStubID::GetObjectFromConstPool:
            case CommonStubID::GetStringFromConstPool:
            case CommonStubID::GetUnmappedArgs:
            case CommonStubID::NewLexicalEnv:
                return true;
            // Abstract equality may call user-defined conversion on mixed object/non-object operands.
            // Unlike relational comparison, object-object equality is a reference check and skips ToPrimitive.
            case CommonStubID::Equal:
            case CommonStubID::NotEqual: {
                // 1 : left operand, after glue.
                ValueVertex *left = call->GetInput(1);
                // 2 : right operand, after glue and left operand.
                ValueVertex *right = call->GetInput(2);
                if (!NodeInfo::NodeTypeCanBe(compileInfoFacts_->GetKnownType(left), JS_RECEIVER) &&
                    !NodeInfo::NodeTypeCanBe(compileInfoFacts_->GetKnownType(right), JS_RECEIVER)) {
                    // If neither side can be an object, abstract equality cannot run user-defined conversion.
                    return true;
                }
                // If both sides are definitely objects, equality is a reference check and still skips conversion.
                return compileInfoFacts_->CheckType(left, JS_RECEIVER) &&
                       compileInfoFacts_->CheckType(right, JS_RECEIVER);
            }
            // Unary numeric stubs may run ToNumber/ToNumeric on the accumulator operand.
            case CommonStubID::Inc:
            case CommonStubID::Dec:
            case CommonStubID::Neg:
            case CommonStubID::Not:
                // 1 : accumulator operand, after glue.
                return !InputMayBeJSReceiver(call, 1);
            // Binary arithmetic and bitwise stubs may run ToPrimitive/ToNumeric/ToNumber on either operand.
            case CommonStubID::Add:
            case CommonStubID::Sub:
            case CommonStubID::Mul:
            case CommonStubID::Div:
            case CommonStubID::Mod:
            case CommonStubID::And:
            case CommonStubID::Or:
            case CommonStubID::Xor:
            case CommonStubID::Shl:
            case CommonStubID::Shr:
            case CommonStubID::Ashr:
            case CommonStubID::StringAdd:
                // 1 : left operand, after glue.
                // 2 : right operand, after glue and left operand.
                return !InputMayBeJSReceiver(call, 1) && !InputMayBeJSReceiver(call, 2);
            // Relational comparison runs ToPrimitive even for object-object inputs, so it cannot use the
            // object-object equality fast rejection above.
            case CommonStubID::Less:
            case CommonStubID::LessEq:
            case CommonStubID::Greater:
            case CommonStubID::GreaterEq:
                // 1 : left operand, after glue.
                // 2 : right operand, after glue and left operand.
                return !InputMayBeJSReceiver(call, 1) && !InputMayBeJSReceiver(call, 2);
            // The `in` operator can consult receiver-side property lookup hooks.
            case CommonStubID::IsIn:
                // 1 : property key operand, after glue.
                // 2 : object operand, after glue and property key.
                return !InputMayBeJSReceiver(call, 1) && !InputMayBeJSReceiver(call, 2);
            // `instanceof` may call @@hasInstance on the constructor operand.
            case CommonStubID::Instanceof:
                // 2 : constructor operand, after glue and object operand.
                return !InputMayBeJSReceiver(call, 2);
            // Define/st-own operations may run receiver/prototype/proxy paths or mutate object shapes.
            case CommonStubID::DefineField:
            case CommonStubID::StOwnByValue:
            case CommonStubID::StOwnByIndex:
            case CommonStubID::StOwnByName:
            case CommonStubID::StOwnByValueWithNameSet:
            case CommonStubID::StOwnByNameWithNameSet:
                // 1 : receiver operand, after glue.
                // 2 : property key operand, after glue and receiver.
                return !InputMayBeJSReceiver(call, 1) && !InputMayBeJSReceiver(call, 2);
            // Delete can mutate the receiver shape; the key may also run conversion logic.
            case CommonStubID::DeleteObjectProperty:
                // 1 : receiver operand, after glue.
                // 2 : property key operand, after glue and receiver.
                return !InputMayBeJSReceiver(call, 1) && !InputMayBeJSReceiver(call, 2);
            default:
                return false;
        }
    }

    bool IsDependencySafeRuntimeCall(CallRuntimeVertex *call) const
    {
        switch (call->GetRuntimeStubID()) {
            // Allocation and metadata helpers below do not execute user JS and do not invalidate dependencies.
            case RTSTUB_ID(GetMethodFromCache):
            case RTSTUB_ID(LdExternalModuleVarByIndexOnJSFunc):
            case RTSTUB_ID(LdLocalModuleVarByIndexOnJSFunc):
            case RTSTUB_ID(LdPatchVar):
            case RTSTUB_ID(OptNewLexicalEnvWithName):
                return true;
            // NumberToString is dependency-safe only when the input cannot be a receiver.
            case RTSTUB_ID(NumberToString):
                // 0 : converted value operand.
                return !InputMayBeJSReceiver(call, 0);
            // Conversion stubs may invoke user-defined valueOf/toString/Symbol.toPrimitive on receiver inputs.
            case RTSTUB_ID(ToNumber):
            case RTSTUB_ID(ToNumeric):
            case RTSTUB_ID(ToPropertyKey):
                // 0 : converted value operand.
                return !InputMayBeJSReceiver(call, 0);
            // Iterator slow paths may perform property access or iterator-return callbacks on the receiver.
            case RTSTUB_ID(GetNextPropNameSlowpath):
                // 0 : iterator or receiver operand.
                return !InputMayBeJSReceiver(call, 0);
            // Changing an object's prototype directly invalidates hidden-class/prototype dependencies.
            case RTSTUB_ID(SetObjectWithProto):
                // 1 : object operand, after prototype operand.
                return !InputMayBeJSReceiver(call, 1);
            // Defining accessors mutates the receiver object's own-property layout.
            case RTSTUB_ID(DefineGetterSetterByValue):
                // 0 : receiver object operand.
                // 1 : property key operand.
                return !InputMayBeJSReceiver(call, 0) && !InputMayBeJSReceiver(call, 1);
            // Object spread copies through own-property reads and define-own-property writes.
            case RTSTUB_ID(CopyDataProperties):
                // 0 : target object operand.
                // 1 : source object operand.
                return !InputMayBeJSReceiver(call, 0) && !InputMayBeJSReceiver(call, 1);
            // Array spread writes elements to the destination array.
            case RTSTUB_ID(StArraySpread):
                // 0 : destination array operand.
                return !InputMayBeJSReceiver(call, 0);
            // Exponentiation may run ToNumeric on either operand.
            case RTSTUB_ID(Exp):
                // 0 : left operand.
                // 1 : right operand.
                return !InputMayBeJSReceiver(call, 0) && !InputMayBeJSReceiver(call, 1);
            default:
                return false;
        }
    }

    void LoadLazyDeoptFrameStateForThrowableCall(uint32_t bcIndex, Vertex *vertex)
    {
        if (!self->IsLazyDeoptEnabled()) {
            return;
        }
        LazyDeoptimizableMixin *deoptMixin = LazyDeoptimizableMixinOf(vertex);
        if (deoptMixin == nullptr) {
            return;
        }
        // E refers to exception lazy-deopt
        bool needsE = HasNeverExecutedCatchBlock();
        // D refers to dependency lazy-deopt.
        bool needsD = vertex->Is<CallVertex>();
        if (auto *call = vertex->TryCast<CallCommonStubVertex>()) {
            needsD = !IsDependencySafeCommonStubCall(call);
        } else if (auto *call = vertex->TryCast<CallRuntimeVertex>()) {
            needsD = !IsDependencySafeRuntimeCall(call);
        }
        if (!needsE && !needsD) {
            return;
        }
        ThrowableMixin *throwableMixin = ThrowableMixinOf(vertex);
        ASSERT(!needsE || throwableMixin != nullptr);
        if (!deoptMixin->HasLazyDeoptFrameState()) {
            auto *deoptFrameState = self->chunk_->New<LazyDeoptFrameState>(self->chunk_);
            if (needsE && needsD) {
                // D+E uses one live-in payload. We assume only ACC may be modified by the bytecode,
                // so live-in and live-out are equivalent with ACC excluded.
                BuildLazyDeoptInputsForDE(bcIndex, deoptFrameState, false);
            } else if (needsE) {
                // E-only resumes at the catch handler entry, so only the handler live-ins are required.
                BuildLazyDeoptInputsForEOnly(bcIndex, deoptFrameState);
            } else {
                // D-only uses live-out payload
                BuildLazyDeoptInputsForDOnly(bcIndex, deoptFrameState);
            }
            deoptMixin->SetLazyDeoptFrameState(deoptFrameState, self->preproc_->GetBytecodeOffset(bcIndex));
        }
        if (needsE && !throwableMixin->HasExceptionLazyDeopt()) {
            ASSERT(!throwableMixin->HasCatchBlock());
            throwableMixin->MarkExceptionLazyDeopt();
        }
    }

    // -------- Miscellaneous Helpers --------

    bool HasCatchBlock() const
    {
        return reinterpret_cast<uintptr_t>(lazyCatchBlock) != NO_CATCH_BLOCK_TAG;
    }

    bool HasNeverExecutedCatchBlock() const
    {
        return HasCatchBlock() && blockInfo->catchBlockState == CatchBlockProfileState::NEVER_EXECUTED;
    }

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

    template <class BranchVertexT, class BuildTrue, class BuildFalse, class... BranchArgs>
    ValueVertex *BuildSelect(std::initializer_list<ValueVertex *> branchInputs, VRegIDType resultVreg,
                             BuildTrue buildTrue, BuildFalse buildFalse, BranchArgs &&...branchArgs)
    {
        BB *trueBlock = self->NewBlock();
        BB *falseBlock = self->NewBlock();
        BB *doneBlock = self->NewBlock();
        BB *branchBlock = currentBlock;

        self->FinishBlockWith<BranchVertexT>(branchBlock, branchInputs, std::forward<BranchArgs>(branchArgs)...,
                                             trueBlock, falseBlock);
        trueBlock->AddPredecessor(branchBlock);
        falseBlock->AddPredecessor(branchBlock);

        currentBlock = trueBlock;
        ValueVertex *trueResult = buildTrue();
        self->FinishBlockWithJump(currentBlock, doneBlock);

        currentBlock = falseBlock;
        ValueVertex *falseResult = buildFalse();
        self->FinishBlockWithJump(currentBlock, doneBlock);

        currentBlock = doneBlock;
        return self->NewPhiVertexWith(currentBlock, {trueResult, falseResult}, resultVreg);
    }

    ValueVertex *LoadRegister(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *vreg = std::get_if<VirtualRegister>(bcInfo->inputs.data() + inputIndex);
        ASSERT(vreg != nullptr);
        return frameState.Get(vreg->GetId());
    }

    ValueVertex *LoadParam(VRegIDType paramIndex) const
    {
        VRegIDType vreg = VRegOfParam(self->numLocal_, paramIndex);
        return frameState.Get(vreg);
    }

    template <class CastsTo = uint16_t>
    CastsTo GetConstDataId(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *constDataId = std::get_if<kungfu::ConstDataId>(bcInfo->inputs.data() + inputIndex);
        ASSERT(constDataId != nullptr);
        return static_cast<CastsTo>(constDataId->GetId());
    }

    template <class CastsTo = kungfu::ICSlotIdType>
    CastsTo GetICSlotId(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *icSlotId = std::get_if<kungfu::ICSlotId>(bcInfo->inputs.data() + inputIndex);
        ASSERT(icSlotId != nullptr);
        return static_cast<CastsTo>(icSlotId->GetId());
    }

    template <class CastsTo = kungfu::ImmValueType>
    CastsTo GetImmediate(const BytecodeInfo *bcInfo, int inputIndex) const
    {
        auto *imm = std::get_if<kungfu::Immediate>(bcInfo->inputs.data() + inputIndex);
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
        ASSERT(self->actualArgc_ != nullptr);
        return self->actualArgc_;
    }

    ValueVertex *TaggedActualArgc()
    {
        ASSERT(self->taggedActualArgc_ != nullptr);
        return self->taggedActualArgc_;
    }

    std::optional<JSTaggedValue> TryGetConstantHeapObject(ValueVertex *node) const
    {
        if (node == nullptr || !node->IsTagged()) {
            return std::nullopt;
        }

        auto *constant = node->TryCast<TaggedConstantVertex>();
        if (constant == nullptr) {
            return std::nullopt;
        }
        JSTaggedValue value(constant->GetValue());
        return value.IsHole() || value.IsHeapObject() ? std::optional<JSTaggedValue>(value) : std::nullopt;
    }

    // LDA_STR "" is currently represented by GetStringFromConstPool.
    // When heap constants are added, recognize empty string heap constants here too.
    bool IsEmptyStringConstant(ValueVertex *value) const
    {
        auto *call = value->TryCast<CallCommonStubVertex>();
        if (call == nullptr ||
            call->GetCommonStubID() != static_cast<uint32_t>(CommonStubID::GetStringFromConstPool)) {
            return false;
        }

        auto *stringId = call->GetInput(2)->TryCast<Int32ConstantVertex>();
        if (stringId == nullptr) {
            return false;
        }

        int32_t constDataId = stringId->GetValue();
        if (constDataId < 0 || constDataId > std::numeric_limits<uint16_t>::max()) {
            return false;
        }
        std::optional<JSTaggedValue> string = TryGetNameFromConstDataId(static_cast<uint16_t>(constDataId));
        if (!string.has_value()) {
            return false;
        }
        ALLOW_DEREF_HANDLE;
        return EcmaStringAccessor(*string).GetLength() == 0;
    }

    std::optional<JSTaggedValue> TryGetNameFromConstDataId(uint16_t constDataId) const
    {
        JitCompilationEnv *env = self->preproc_->GetEnv();
        JSThread *thread = env->GetJSThread();
        if (thread == nullptr) {
            return std::nullopt;
        }

        ALLOW_DEREF_HANDLE;
        JSHandle<JSFunction> function = env->GetJsFunction();
        JSTaggedValue methodValue = function->GetMethod(thread);
        if (!methodValue.IsMethod()) {
            return std::nullopt;
        }
        JSTaggedValue constpool = Method::Cast(methodValue.GetTaggedObject())->GetConstantPool(thread);
        if (constpool.IsUndefined() || !constpool.IsConstantPool()) {
            return std::nullopt;
        }
        JSTaggedValue name = ConstantPool::GetStringFromCacheForJit(thread, constpool, constDataId, false);
        if (name.IsUndefined() || name.IsHole() || !name.IsString()) {
            return std::nullopt;
        }
        return name;
    }

    std::optional<ArkSteedNameRef> TryGetNameRefFromConstDataId(uint16_t constDataId) const
    {
        if (self->preproc_->GetEnv() == nullptr || self->preproc_->GetEnv()->GetMethodLiteral() == nullptr) {
            return std::nullopt;
        }

        ArkSteedHeapBroker *broker = self->pgoContext_.GetBroker();
        if (broker == nullptr) {
            return std::nullopt;
        }

        ArkSteedNameRef name;
        ArkSteedHeapBroker::SerializingScope scope(broker, "GraphBuilder::TryGetNameRefFromConstDataId");
        if (!broker->TryGetNameFromConstantPool(constDataId, &name)) {
            return std::nullopt;
        }
        return name;
    }

    std::optional<PropertyLookupResult> TryLookupPropertyInPGOHClass(
        JSHClass *hclass, const ArkSteedNameRef &nameRef) const
    {
        ArkSteedHeapBroker *broker = self->pgoContext_.GetBroker();
        JSTaggedValue name = JSTaggedValue::Undefined();
        if (hclass == nullptr || broker == nullptr || !broker->TryResolveRef(nameRef, &name) ||
            (!name.IsString() && !name.IsSymbol())) {
            return std::nullopt;
        }
        return JSHClass::LookupPropertyInPGOHClass(self->compilerThread_, hclass, name);
    }

    std::optional<JSHClass *> TryResolveHClassRef(const ArkSteedHClassRef &hclassRef) const
    {
        ArkSteedHeapBroker *broker = self->pgoContext_.GetBroker();
        JSTaggedValue hclassValue = JSTaggedValue::Undefined();
        if (broker == nullptr || !broker->TryResolveRef(hclassRef, &hclassValue) || !hclassValue.IsJSHClass()) {
            return std::nullopt;
        }
        return JSHClass::Cast(hclassValue.GetTaggedObject());
    }

    static bool IsSupportedNamedLoadAccessInfo(const PropertyAccessInfo &accessInfo)
    {
        if (accessInfo.mode != AccessMode::NAMED_LOAD || !accessInfo.IsDataField() ||
            accessInfo.fieldRepresentation != AccessFieldRepresentation::TAGGED ||
            (accessInfo.fieldStorage != AccessFieldStorage::IN_OBJECT &&
             accessInfo.fieldStorage != AccessFieldStorage::PROPERTIES_ARRAY) ||
            !accessInfo.expectedHClass.IsSafeForCompile()) {
            return false;
        }
        return true;
    }

    std::optional<PropertyLookupResult> TryMakePropertyLookupResultFromAccessInfo(
        const PropertyAccessInfo &accessInfo, JSHClass *holderHClass, const ArkSteedNameRef &nameRef) const
    {
        std::optional<PropertyLookupResult> maybePlr = TryLookupPropertyInPGOHClass(holderHClass, nameRef);
        if (!maybePlr.has_value()) {
            return std::nullopt;
        }
        PropertyLookupResult plr = maybePlr.value();
        bool hasSameStorage = (accessInfo.fieldStorage == AccessFieldStorage::IN_OBJECT && plr.IsInlinedProps() &&
                               plr.GetOffset() == static_cast<uint32_t>(accessInfo.fieldOffset)) ||
                              (accessInfo.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY &&
                               !plr.IsInlinedProps() && plr.GetOffset() == accessInfo.fieldIndex);
        if (!plr.IsFound() || !plr.IsLocal() ||
            plr.IsAccessor() || plr.IsFunction() ||
            plr.IsLoadFromIterResult() || plr.GetRepresentation() != Representation::TAGGED ||
            !hasSameStorage) {
            return std::nullopt;
        }
        return plr;
    }

    std::optional<PropertyLookupResult> TryMakePropertyLookupResultFromAccessInfo(
        const PropertyAccessInfo &accessInfo, JSHClass *holderHClass, uint16_t constDataId) const
    {
        std::optional<ArkSteedNameRef> nameRef = TryGetNameRefFromConstDataId(constDataId);
        if (!nameRef.has_value()) {
            return std::nullopt;
        }
        return TryMakePropertyLookupResultFromAccessInfo(accessInfo, holderHClass, nameRef.value());
    }

    NamedLoadAccessInfoOpt TryConvertNamedLoadAccessInfo(const PropertyAccessInfo &accessInfo,
                                                         const ArkSteedNameRef &nameRef) const
    {
        if (!IsSupportedNamedLoadAccessInfo(accessInfo)) {
            return std::nullopt;
        }
        std::optional<JSHClass *> receiverHClass = TryResolveHClassRef(accessInfo.expectedHClass);
        if (!receiverHClass.has_value() || receiverHClass.value() == nullptr ||
            !receiverHClass.value()->GetLayout(self->compilerThread_).IsTaggedArray()) {
            return std::nullopt;
        }
        JSHClass *holderHClass = receiverHClass.value();
        uint32_t holderDepth = 0;
        std::vector<JSHClass *> expectedPrototypeHClasses;
        if (!accessInfo.holderIsReceiver) {
            std::optional<JSHClass *> holder = TryResolveHClassRef(accessInfo.fieldOwnerHClass);
            if (!holder.has_value()) {
                return std::nullopt;
            }
            holderHClass = holder.value();
            if (holderHClass == nullptr || !holderHClass->GetLayout(self->compilerThread_).IsTaggedArray()) {
                return std::nullopt;
            }
            JSTaggedValue current = receiverHClass.value()->GetPrototype(self->compilerThread_);
            holderDepth = 1;
            while (current.IsHeapObject()) {
                JSHClass *currentHClass = current.GetTaggedObject()->GetClass();
                expectedPrototypeHClasses.push_back(currentHClass);
                if (currentHClass == holderHClass) {
                    break;
                }
                current = currentHClass->GetPrototype(self->compilerThread_);
                holderDepth++;
            }
            if (!current.IsHeapObject()) {
                return std::nullopt;
            }
            if (expectedPrototypeHClasses.empty() || expectedPrototypeHClasses.back() != holderHClass ||
                expectedPrototypeHClasses.size() != holderDepth) {
                return std::nullopt;
            }
        }
        std::optional<PropertyLookupResult> plr =
            TryMakePropertyLookupResultFromAccessInfo(accessInfo, holderHClass, nameRef);
        if (!plr.has_value()) {
            return std::nullopt;
        }
        bool hasStableProtoChain = holderDepth > 0 &&
            accessInfo.dependencies.canAssumeStableProtoChain;
        NamedLoadAccessInfo result {
            .receiverHClass = receiverHClass.value(),
            .holderHClass = holderHClass,
            .lookupStartObjectHClasses = {receiverHClass.value()},
            .expectedPrototypeHClasses = std::move(expectedPrototypeHClasses),
            .plr = plr.value(),
            .holderDepth = holderDepth,
            .isConst = false,
            .canAssumeStableHClasses = accessInfo.dependencies.canAssumeStableHClass,
            .hasStableProtoChain = hasStableProtoChain,
        };
        return result;
    }

    NamedLoadAccessInfoOpt TryConvertNamedLoadAccessInfo(const PropertyAccessInfo &accessInfo,
                                                         uint16_t constDataId) const
    {
        std::optional<ArkSteedNameRef> nameRef = TryGetNameRefFromConstDataId(constDataId);
        if (!nameRef.has_value()) {
            return std::nullopt;
        }
        return TryConvertNamedLoadAccessInfo(accessInfo, nameRef.value());
    }

    static bool HasSameLoadFieldAccess(const NamedLoadAccessInfo &lhs, const NamedLoadAccessInfo &rhs)
    {
        if (lhs.plr.GetData() != rhs.plr.GetData() || lhs.isConst != rhs.isConst ||
            lhs.holderDepth != rhs.holderDepth) {
            return false;
        }
        if (lhs.holderDepth == 0) {
            return true;
        }
        return lhs.holderHClass == rhs.holderHClass &&
               lhs.expectedPrototypeHClasses == rhs.expectedPrototypeHClasses;
    }

    static void AppendHClassIfMissing(std::vector<JSHClass *> *hclasses, JSHClass *hclass)
    {
        if (hclasses != nullptr && hclass != nullptr &&
            std::find(hclasses->begin(), hclasses->end(), hclass) == hclasses->end()) {
            hclasses->push_back(hclass);
        }
    }

    NamedLoadAccessInfosOpt TryGetLoadObjByNameAccessInfos(const PropertyAccessSet &accessSet,
                                                           const ArkSteedNameRef &nameRef) const
    {
        std::vector<NamedLoadAccessInfo> result;
        for (uint32_t i = 0; i < accessSet.caseCount && i < accessSet.cases.size(); ++i) {
            NamedLoadAccessInfoOpt accessInfo = TryConvertNamedLoadAccessInfo(accessSet.cases[i], nameRef);
            if (!accessInfo.has_value()) {
                return std::nullopt;
            }

            bool merged = false;
            for (NamedLoadAccessInfo &existing : result) {
                if (!HasSameLoadFieldAccess(existing, accessInfo.value())) {
                    continue;
                }
                for (JSHClass *hclass : accessInfo->lookupStartObjectHClasses) {
                    AppendHClassIfMissing(&existing.lookupStartObjectHClasses, hclass);
                }
                existing.canAssumeStableHClasses =
                    existing.canAssumeStableHClasses && accessInfo->canAssumeStableHClasses;
                existing.hasStableProtoChain =
                    existing.hasStableProtoChain && accessInfo->hasStableProtoChain;
                merged = true;
                break;
            }
            if (!merged) {
                result.push_back(std::move(accessInfo.value()));
            }
        }
        if (result.empty()) {
            return std::nullopt;
        }
        return result;
    }

    NamedLoadAccessInfosOpt TryGetLoadObjByNameAccessInfos(const PropertyAccessSet &accessSet,
                                                           uint16_t constDataId) const
    {
        std::optional<ArkSteedNameRef> nameRef = TryGetNameRefFromConstDataId(constDataId);
        if (!nameRef.has_value()) {
            return std::nullopt;
        }
        return TryGetLoadObjByNameAccessInfos(accessSet, nameRef.value());
    }
    std::optional<int32_t> TryGetInt32Value(ValueVertex *value) const
    {
        if (value == nullptr) {
            return std::nullopt;
        }
        if (auto *constant = value->TryCast<Int32ConstantVertex>()) {
            return constant->GetValue();
        }
        if (auto *constant = value->TryCast<TaggedConstantVertex>()) {
            JSTaggedValue tagged(constant->GetValue());
            return tagged.IsInt() ? std::optional<int32_t>(tagged.GetInt()) : std::nullopt;
        }
        return std::nullopt;
    }

    // Returns the double value of a double constant operand (a Float64ConstantVertex, or a
    // TaggedConstantVertex holding a tagged double). Int constants return nullopt so that int
    // DIV/MOD are not folded through the Float64 path.
    std::optional<double> TryGetFloat64Value(ValueVertex *value) const
    {
        if (value == nullptr) {
            return std::nullopt;
        }
        if (auto *constant = value->TryCast<Float64ConstantVertex>()) {
            return constant->GetValue();
        }
        if (auto *constant = value->TryCast<TaggedConstantVertex>()) {
            JSTaggedValue tagged(constant->GetValue());
            return tagged.IsDouble() ? std::optional<double>(tagged.GetDouble()) : std::nullopt;
        }
        return std::nullopt;
    }

    ValueVertex *BuildTaggedIntToI32(ValueVertex *value)
    {
        if (std::optional<int32_t> constant = TryGetInt32Value(value)) {
            return self->graph_->GetInt32Constant(*constant);
        }
        if (ValueVertex *alternative = compileInfoFacts_->TryGetAlternative(value, AlternativeNodes::Kind::INT32)) {
            return alternative;
        }
        ValueVertex *i32 = self->NewVertex<TaggedIntToI32Vertex>(compileInfoFacts_, currentBlock, {value});
        compileInfoFacts_->SetAlternative(value, AlternativeNodes::Kind::INT32, i32);
        return i32;
    }

    ValueVertex *BuildCheckedTaggedIntToI32(ValueVertex *value)
    {
        if (std::optional<int32_t> constant = TryGetInt32Value(value)) {
            return self->graph_->GetInt32Constant(*constant);
        }
        if (ValueVertex *alternative = compileInfoFacts_->TryGetAlternative(value, AlternativeNodes::Kind::INT32)) {
            return alternative;
        }
        std::vector<ValueVertex *> inputs {value};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        ValueVertex *i32 = self->NewVertex<CheckedTaggedIntToI32Vertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
        i32->Cast<CheckedTaggedIntToI32Vertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::INT);
        compileInfoFacts_->SetAlternative(value, AlternativeNodes::Kind::INT32, i32);
        return i32;
    }

    ValueVertex *BuildCheckedTaggedString(ValueVertex *value)
    {
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::STRING)) {
            return value;
        }
        std::vector<ValueVertex *> inputs {value};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        ValueVertex *checked = self->NewVertex<CheckedTaggedStringVertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
        checked->Cast<CheckedTaggedStringVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::STRING);
        compileInfoFacts_->EnsureType(checked, NodeInfo::NodeType::STRING);
        return checked;
    }

    void BuildDeoptIfNotNumber(ValueVertex *value)
    {
        std::vector<ValueVertex *> inputs {value};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        self->NewVertex<DeoptIfNotNumberVertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex))
            ->SetEagerDeoptFrameState(std::move(deoptFrameState));
    }

    ValueVertex *BuildNumberToString(ValueVertex *value)
    {
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::STRING)) {
            return value;
        }
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::NUMBER)) {
            ValueVertex *result = RuntimeCall({value}, RTSTUB_ID(NumberToString));
            compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::STRING);
            return result;
        }

        ValueVertex *result = BuildSelect<BranchIfTaggedStringVertex>(
            {value}, self->AccIndex(),
            [&]() -> ValueVertex * { return value; },
            [&]() -> ValueVertex * {
                BuildDeoptIfNotNumber(value);
                ValueVertex *numberResult = RuntimeCall({value}, RTSTUB_ID(NumberToString));
                compileInfoFacts_->EnsureType(numberResult, NodeInfo::NodeType::STRING);
                return numberResult;
            });
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::STRING);
        return result;
    }

    ValueVertex *BuildTaggedI32Result(ValueVertex *rawResult)
    {
        ValueVertex *taggedResult = self->NewVertex<I32ToTaggedIntVertex>(compileInfoFacts_, currentBlock, {rawResult});
        compileInfoFacts_->EnsureType(taggedResult, NodeInfo::NodeType::INT);
        compileInfoFacts_->SetAlternative(taggedResult, AlternativeNodes::Kind::INT32, rawResult);
        return taggedResult;
    }

    ValueVertex *BuildI32WithOverflowTagged(BinaryOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *leftI32 = BuildTaggedIntToI32(left);
        ValueVertex *rightI32 = BuildTaggedIntToI32(right);
        ValueVertex *rawResult = BuildI32BinOpWithOverflow(kind, leftI32, rightI32);
        ASSERT(rawResult != nullptr);
        return BuildTaggedI32Result(rawResult);
    }

    ValueVertex *BuildI32BinOpValue(BinaryOpKind kind, ValueVertex *leftI32, ValueVertex *rightI32)
    {
        auto inputs = {leftI32, rightI32};
        switch (kind) {
            case BinaryOpKind::ADD:
                return self->NewVertex<I32AddVertex>(compileInfoFacts_, currentBlock, inputs);
            case BinaryOpKind::SUB:
                return self->NewVertex<I32SubVertex>(compileInfoFacts_, currentBlock, inputs);
            case BinaryOpKind::MUL:
                return self->NewVertex<I32MulVertex>(compileInfoFacts_, currentBlock, inputs);
            case BinaryOpKind::DIV:
                return self->NewVertex<I32DivVertex>(compileInfoFacts_, currentBlock, inputs);
            default:
                return nullptr;
        }
    }

    ValueVertex *BuildI32TaggedBinOpValue(BinaryOpKind kind, ValueVertex *leftI32, ValueVertex *rightI32)
    {
        ValueVertex *rawResult = BuildI32BinOpValue(kind, leftI32, rightI32);
        ASSERT(rawResult != nullptr);
        return BuildTaggedI32Result(rawResult);
    }

    ValueVertex *BuildI32BinOpWithOverflow(BinaryOpKind kind, ValueVertex *leftI32, ValueVertex *rightI32)
    {
        std::vector<ValueVertex *> inputs {leftI32, rightI32};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        switch (kind) {
            case BinaryOpKind::ADD: {
                ValueVertex *result = self->NewVertex<I32AddWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                result->Cast<I32AddWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return result;
            }
            case BinaryOpKind::SUB: {
                ValueVertex *result = self->NewVertex<I32SubWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                result->Cast<I32SubWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return result;
            }
            case BinaryOpKind::MUL: {
                ValueVertex *result = self->NewVertex<I32MulWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                result->Cast<I32MulWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return result;
            }
            case BinaryOpKind::DIV: {
                ValueVertex *result = self->NewVertex<I32DivWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                result->Cast<I32DivWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return result;
            }
            case BinaryOpKind::MOD: {
                ValueVertex *result = self->NewVertex<CheckedI32ModVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                result->Cast<CheckedI32ModVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return result;
            }
            default:
                return nullptr;
        }
    }

    struct SignedDivisorMagic {
        int32_t magic;
        uint32_t shift;
    };

    static SignedDivisorMagic ComputeSignedDivisorMagic(int32_t divisor)
    {
        ASSERT(divisor <= -2 || divisor >= 2);
        constexpr uint32_t BIT_WIDTH = 32;
        uint64_t highOne = 1ULL << (BIT_WIDTH - 1U);
        uint64_t ad = divisor < 0 ? static_cast<uint64_t>(-static_cast<int64_t>(divisor)) :
                                     static_cast<uint64_t>(divisor);
        uint64_t divisorBits = static_cast<uint64_t>(static_cast<int64_t>(divisor));
        uint64_t t = highOne + (divisorBits >> 63U);
        uint64_t anc = t - 1U - t % ad;
        int64_t p = BIT_WIDTH - 1U;
        uint64_t q1 = highOne / anc;
        uint64_t r1 = highOne - q1 * anc;
        uint64_t q2 = highOne / ad;
        uint64_t r2 = highOne - q2 * ad;
        uint64_t delta = 0U;

        do {
            ++p;
            q1 *= 2U;
            r1 *= 2U;
            if (r1 >= anc) {
                ++q1;
                r1 -= anc;
            }
            q2 *= 2U;
            r2 *= 2U;
            if (r2 >= ad) {
                ++q2;
                r2 -= ad;
            }
            delta = ad - r2;
        } while (q1 < delta || (q1 == delta && r1 == 0));

        int64_t magic = static_cast<int64_t>(q2) + 1;
        if (divisor < 0) {
            magic = -magic;
        }
        return {static_cast<int32_t>(magic), static_cast<uint32_t>(p - BIT_WIDTH)};
    }

    ValueVertex *BuildI32Operand(ValueVertex *value, bool knownInt)
    {
        return knownInt ? BuildTaggedIntToI32(value) : BuildCheckedTaggedIntToI32(value);
    }

    ValueVertex *TryReuseKnownIntOperand(ValueVertex *value, bool knownInt)
    {
        if (!knownInt) {
            return nullptr;
        }
        compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::INT);
        return value;
    }

    ValueVertex *BuildI32DivByConstWithCheckTagged(ValueVertex *left, bool leftKnownInt, int32_t divisor)
    {
        SignedDivisorMagic magic = ComputeSignedDivisorMagic(divisor);
        ValueVertex *leftI32 = BuildI32Operand(left, leftKnownInt);
        std::vector<ValueVertex *> inputs {leftI32};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        ValueVertex *rawResult = self->NewVertex<I32DivByConstWithCheckVertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex),
            divisor, magic.magic, magic.shift);
        rawResult->Cast<I32DivByConstWithCheckVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        return BuildTaggedI32Result(rawResult);
    }

    void BuildDeoptIfInt32Condition(ValueVertex *leftI32, ValueVertex *rightI32, Condition condition,
                                    kungfu::DeoptType deoptType)
    {
        std::vector<ValueVertex *> inputs {leftI32, rightI32};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        self->NewVertex<DeoptIfInt32ConditionVertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex), condition,
            deoptType)
            ->SetEagerDeoptFrameState(std::move(deoptFrameState));
    }

    ValueVertex *BuildTaggedIntConstant(int32_t value)
    {
        ValueVertex *constant = self->graph_->GetTaggedConstant(JSTaggedValue(value).GetRawData());
        compileInfoFacts_->EnsureType(constant, NodeInfo::NodeType::INT);
        return constant;
    }

    ValueVertex *TryBuildI32MulByZeroReduction(ValueVertex *value, bool valueKnownInt)
    {
        if (std::optional<int32_t> constant = TryGetInt32Value(value)) {
            if (*constant < 0) {
                return nullptr;
            }
            return BuildTaggedIntConstant(0);
        }

        ValueVertex *valueI32 = BuildI32Operand(value, valueKnownInt);
        BuildDeoptIfInt32Condition(valueI32, self->graph_->GetInt32Constant(0), Condition::LESS_THAN,
                                   kungfu::DeoptType::PRODUCTISNEGATIVEZERO);
        return BuildTaggedIntConstant(0);
    }

    ValueVertex *TryBuildI32DivByMinusOneReduction(ValueVertex *value, bool valueKnownInt)
    {
        std::optional<int32_t> constant = TryGetInt32Value(value);
        if (constant.has_value() && *constant == 0) {
            return nullptr;
        }

        ValueVertex *valueI32 = BuildI32Operand(value, valueKnownInt);
        ValueVertex *zeroI32 = self->graph_->GetInt32Constant(0);
        if (!constant.has_value()) {
            BuildDeoptIfInt32Condition(valueI32, zeroI32, Condition::EQUAL, kungfu::DeoptType::DIVZERO2);
        }
        ValueVertex *rawResult = BuildI32BinOpWithOverflow(BinaryOpKind::SUB, zeroI32, valueI32);
        return BuildTaggedI32Result(rawResult);
    }

    ValueVertex *TryBuildI32ModByOneReduction(ValueVertex *left, bool leftKnownInt, int32_t divisor)
    {
        ASSERT(divisor == 1 || divisor == -1);

        std::optional<int32_t> leftValue = TryGetInt32Value(left);
        if (leftValue.has_value()) {
            if (*leftValue < 0 || (divisor == -1 && *leftValue == std::numeric_limits<int32_t>::min())) {
                return nullptr;
            }
            return BuildTaggedIntConstant(0);
        }

        ValueVertex *leftI32 = BuildI32Operand(left, leftKnownInt);
        if (divisor == -1) {
            BuildDeoptIfInt32Condition(leftI32, self->graph_->GetInt32Constant(std::numeric_limits<int32_t>::min()),
                                       Condition::EQUAL, kungfu::DeoptType::INT32OVERFLOW1);
        }
        BuildDeoptIfInt32Condition(leftI32, self->graph_->GetInt32Constant(0), Condition::LESS_THAN,
                                   kungfu::DeoptType::REMAINDERISNEGATIVEZERO);
        return BuildTaggedIntConstant(0);
    }

    ValueVertex *TryBuildI32RightConstantReduction(BinaryOpKind kind, ValueVertex *left, bool leftKnownInt,
                                                   int32_t rightValue)
    {
        switch (kind) {
            case BinaryOpKind::ADD:
                if (rightValue == 1) {
                    // x + 1 -> ++x.
                    return BuildIntUnaryOp(CommonStubID::Inc, left, leftKnownInt);
                }
                if (rightValue == 0) {
                    // x + 0 -> x.
                    return TryReuseKnownIntOperand(left, leftKnownInt);
                }
                break;
            case BinaryOpKind::SUB:
                if (rightValue == 1) {
                    // x - 1 -> --x.
                    return BuildIntUnaryOp(CommonStubID::Dec, left, leftKnownInt);
                }
                if (rightValue == 0) {
                    // x - 0 -> x.
                    return TryReuseKnownIntOperand(left, leftKnownInt);
                }
                break;
            case BinaryOpKind::MUL:
                if (rightValue == 0) {
                    // x * 0 -> 0, guarding against -0.
                    return TryBuildI32MulByZeroReduction(left, leftKnownInt);
                }
                if (rightValue == 1) {
                    // x * 1 -> x.
                    return TryReuseKnownIntOperand(left, leftKnownInt);
                }
                break;
            case BinaryOpKind::DIV:
                if (rightValue == -1) {
                    // x / -1 -> -x, guarding zero and overflow.
                    return TryBuildI32DivByMinusOneReduction(left, leftKnownInt);
                }
                if (rightValue == 1) {
                    // x / 1 -> x.
                    return TryReuseKnownIntOperand(left, leftKnownInt);
                }
                if (rightValue != 0) {
                    // x / c -> checked constant-divisor path.
                    return BuildI32DivByConstWithCheckTagged(left, leftKnownInt, rightValue);
                }
                break;
            case BinaryOpKind::MOD:
                if (rightValue == 1 || rightValue == -1) {
                    // x % +/-1 -> 0, guarding -0 and overflow.
                    return TryBuildI32ModByOneReduction(left, leftKnownInt, rightValue);
                }
                break;
            default:
                break;
        }
        return nullptr;
    }

    ValueVertex *TryBuildI32LeftConstantReduction(BinaryOpKind kind, int32_t leftValue, ValueVertex *right,
                                                  bool rightKnownInt)
    {
        switch (kind) {
            case BinaryOpKind::ADD:
                if (leftValue == 1) {
                    // 1 + x -> ++x.
                    return BuildIntUnaryOp(CommonStubID::Inc, right, rightKnownInt);
                }
                if (leftValue == 0) {
                    // 0 + x -> x.
                    return TryReuseKnownIntOperand(right, rightKnownInt);
                }
                break;
            case BinaryOpKind::MUL:
                if (leftValue == 0) {
                    // 0 * x -> 0, guarding against -0.
                    return TryBuildI32MulByZeroReduction(right, rightKnownInt);
                }
                if (leftValue == 1) {
                    // 1 * x -> x.
                    return TryReuseKnownIntOperand(right, rightKnownInt);
                }
                break;
            default:
                break;
        }
        return nullptr;
    }

    ValueVertex *TryBuildI32BinaryReduction(BinaryOpKind kind, ValueVertex *left, ValueVertex *right,
                                            bool leftKnownInt, bool rightKnownInt)
    {
        if (std::optional<int32_t> rightValue = TryGetInt32Value(right)) {
            if (ValueVertex *reduced = TryBuildI32RightConstantReduction(kind, left, leftKnownInt, *rightValue)) {
                return reduced;
            }
        }

        if (std::optional<int32_t> leftValue = TryGetInt32Value(left)) {
            if (ValueVertex *reduced = TryBuildI32LeftConstantReduction(kind, *leftValue, right, rightKnownInt)) {
                return reduced;
            }
        }
        return nullptr;
    }

    ValueVertex *BuildI32BinOp(BinaryOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        ASSERT(SupportsI32CheckedBinOp(kind));

        bool leftKnownInt = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::INT);
        bool rightKnownInt = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::INT);
        return BuildI32CheckedBinOp(kind, left, right, leftKnownInt, rightKnownInt);
    }

    ValueVertex *BuildI32CheckedBinOp(BinaryOpKind kind, ValueVertex *left, ValueVertex *right,
                                       bool leftKnownInt, bool rightKnownInt)
    {
        ASSERT(SupportsI32CheckedBinOp(kind));

        bool leftIsConst = TryGetInt32Value(left).has_value();
        bool rightIsConst = TryGetInt32Value(right).has_value();
        if (leftIsConst || rightIsConst) {
            if (ValueVertex *reduced = TryBuildI32BinaryReduction(kind, left, right, leftKnownInt, rightKnownInt)) {
                return reduced;
            }
        }

        if (leftKnownInt && rightKnownInt) {
            return BuildI32WithOverflowTagged(kind, left, right);
        }

        ValueVertex *leftI32 = leftKnownInt ? BuildTaggedIntToI32(left) : BuildCheckedTaggedIntToI32(left);
        ValueVertex *rightI32 = rightKnownInt ? BuildTaggedIntToI32(right) : BuildCheckedTaggedIntToI32(right);
        ValueVertex *rawResult = BuildI32BinOpWithOverflow(kind, leftI32, rightI32);
        ASSERT(rawResult != nullptr);
        return BuildTaggedI32Result(rawResult);
    }

    ValueVertex *TryBuildProvenIntBinOp(BinaryOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        if (!SupportsI32CheckedBinOp(kind)) {
            return nullptr;
        }
        bool leftKnownInt = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::INT);
        bool rightKnownInt = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::INT);
        if (!leftKnownInt || !rightKnownInt) {
            return nullptr;
        }
        return BuildI32BinOp(kind, left, right);
    }

    ValueVertex *BuildCheckedNumberToF64(ValueVertex *value)
    {
        if (ValueVertex *alternative =
                compileInfoFacts_->TryGetAlternative(value, AlternativeNodes::Kind::HOLEY_FLOAT64)) {
            return alternative;
        }
        if (std::optional<int32_t> constant = TryGetInt32Value(value)) {
            return self->graph_->GetFloat64Constant(static_cast<double>(*constant));
        }
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::INT)) {
            ValueVertex *i32 = BuildTaggedIntToI32(value);
            ValueVertex *f64 = self->NewVertex<I32ToF64Vertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{i32});
            compileInfoFacts_->SetAlternative(value, AlternativeNodes::Kind::HOLEY_FLOAT64, f64);
            return f64;
        }
        std::vector<ValueVertex *> inputs {value};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        ValueVertex *f64 = self->NewVertex<CheckedNumberToF64Vertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
        f64->Cast<CheckedNumberToF64Vertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::NUMBER);
        compileInfoFacts_->SetAlternative(value, AlternativeNodes::Kind::HOLEY_FLOAT64, f64);
        return f64;
    }

    ValueVertex *BuildF64BinOpValue(BinaryOpKind kind, ValueVertex *leftF64, ValueVertex *rightF64)
    {
        auto inputs = {leftF64, rightF64};
        switch (kind) {
            case BinaryOpKind::ADD:
                return self->NewVertex<F64AddVertex>(compileInfoFacts_, currentBlock, inputs);
            case BinaryOpKind::SUB:
                return self->NewVertex<F64SubVertex>(compileInfoFacts_, currentBlock, inputs);
            case BinaryOpKind::MUL:
                return self->NewVertex<F64MulVertex>(compileInfoFacts_, currentBlock, inputs);
            case BinaryOpKind::DIV:
                return self->NewVertex<F64DivVertex>(compileInfoFacts_, currentBlock, inputs);
            default:
                return nullptr;
        }
    }

    bool IsPositiveZero(ValueVertex *value) const
    {
        auto *constant = value->TryCast<Float64ConstantVertex>();
        return constant != nullptr && constant->GetValue() == 0.0 && !std::signbit(constant->GetValue());
    }

    bool IsNegativeZero(ValueVertex *value) const
    {
        auto *constant = value->TryCast<Float64ConstantVertex>();
        return constant != nullptr && constant->GetValue() == 0.0 && std::signbit(constant->GetValue());
    }

    bool IsFloat64One(ValueVertex *value) const
    {
        auto *constant = value->TryCast<Float64ConstantVertex>();
        return constant != nullptr && constant->GetValue() == 1.0;
    }

    ValueVertex *BuildTaggedF64Value(ValueVertex *rawResult)
    {
        ValueVertex *taggedResult =
            self->NewVertex<F64ToTaggedDoubleVertex>(compileInfoFacts_, currentBlock, {rawResult});
        compileInfoFacts_->EnsureType(taggedResult, NodeInfo::NodeType::DOUBLE);
        compileInfoFacts_->SetAlternative(taggedResult, AlternativeNodes::Kind::HOLEY_FLOAT64, rawResult);
        return taggedResult;
    }

    ValueVertex *TryBuildF64Identity(BinaryOpKind kind, ValueVertex *leftF64, ValueVertex *rightF64)
    {
        switch (kind) {
            case BinaryOpKind::ADD:
                if (IsNegativeZero(rightF64)) {
                    return BuildTaggedF64Value(leftF64);
                }
                break;
            case BinaryOpKind::SUB:
                if (IsPositiveZero(rightF64)) {
                    return BuildTaggedF64Value(leftF64);
                }
                break;
            case BinaryOpKind::MUL:
            case BinaryOpKind::DIV:
                if (IsFloat64One(rightF64)) {
                    return BuildTaggedF64Value(leftF64);
                }
                break;
            default:
                break;
        }
        return nullptr;
    }

    ValueVertex *BuildF64TaggedBinOpValue(BinaryOpKind kind, ValueVertex *leftF64, ValueVertex *rightF64)
    {
        if (ValueVertex *identity = TryBuildF64Identity(kind, leftF64, rightF64)) {
            return identity;
        }
        ValueVertex *rawResult = BuildF64BinOpValue(kind, leftF64, rightF64);
        if (rawResult == nullptr) {
            return nullptr;
        }
        return BuildTaggedF64Value(rawResult);
    }

    ValueVertex *BuildF64NumberBinOp(BinaryOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        ASSERT(SupportsF64BinOp(kind));
        ValueVertex *leftF64 = BuildCheckedNumberToF64(left);
        ValueVertex *rightF64 = BuildCheckedNumberToF64(right);
        ValueVertex *taggedResult = BuildF64TaggedBinOpValue(kind, leftF64, rightF64);
        ASSERT(taggedResult != nullptr);
        return taggedResult;
    }

    ValueVertex *BuildCheckedNonNegativeI32ToTaggedInt(ValueVertex *rawResult)
    {
        std::vector<ValueVertex *> inputs {rawResult};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
        ValueVertex *tagged = self->NewVertex<CheckedNonNegativeI32ToTaggedIntVertex>(
            currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
        tagged->Cast<CheckedNonNegativeI32ToTaggedIntVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        compileInfoFacts_->EnsureType(tagged, NodeInfo::NodeType::INT);
        compileInfoFacts_->SetAlternative(tagged, AlternativeNodes::Kind::INT32, rawResult);
        return tagged;
    }

    ValueVertex *BuildGenericBitwiseBinOp(IntBitwiseKind kind, ValueVertex *left, ValueVertex *right)
    {
        switch (kind) {
            case IntBitwiseKind::BITWISE_AND:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::And);
            case IntBitwiseKind::BITWISE_OR:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Or);
            case IntBitwiseKind::BITWISE_XOR:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Xor);
            case IntBitwiseKind::SHIFT_LEFT:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Shl);
            case IntBitwiseKind::SHIFT_RIGHT_LOGICAL:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Shr);
            case IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Ashr);
        }
        UNREACHABLE();
    }

    ValueVertex *TryBuildI32BitwiseReduction(IntBitwiseKind kind, ValueVertex *left, ValueVertex *right)
    {
        std::optional<int32_t> leftValue = TryGetInt32Value(left);
        std::optional<int32_t> rightValue = TryGetInt32Value(right);
        auto knownInt = [this](ValueVertex *value) -> ValueVertex * {
            compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::INT);
            return value;
        };
        auto taggedIntConstant = [this](int32_t value) -> ValueVertex * {
            ValueVertex *constant = self->graph_->GetTaggedConstant(JSTaggedValue(value).GetRawData());
            compileInfoFacts_->EnsureType(constant, NodeInfo::NodeType::INT);
            return constant;
        };

        if (leftValue.has_value() && rightValue.has_value()) {
            int32_t lhs = *leftValue;
            uint32_t shift = static_cast<uint32_t>(*rightValue) & 31U;
            switch (kind) {
                case IntBitwiseKind::BITWISE_AND:
                    return taggedIntConstant(lhs & *rightValue);
                case IntBitwiseKind::BITWISE_OR:
                    return taggedIntConstant(lhs | *rightValue);
                case IntBitwiseKind::BITWISE_XOR:
                    return taggedIntConstant(lhs ^ *rightValue);
                case IntBitwiseKind::SHIFT_LEFT:
                    return taggedIntConstant(static_cast<int32_t>(static_cast<uint32_t>(lhs) << shift));
                case IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC:
                    return taggedIntConstant(lhs >> static_cast<int32_t>(shift));
                case IntBitwiseKind::SHIFT_RIGHT_LOGICAL: {
                    // >>> yields a uint32; fold only when it fits int32 (non-negative), otherwise
                    // leave it so the SHR tagging path (CheckedNonNegativeI32ToTaggedInt) deopts
                    // to double as JS requires (e.g. (-1) >>> 0 === 4294967295).
                    uint32_t ur = static_cast<uint32_t>(lhs) >> shift;
                    if (ur > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
                        break;
                    }
                    return taggedIntConstant(static_cast<int32_t>(ur));
                }
                default:
                    break;
            }
        }

        if (rightValue.has_value()) {
            switch (kind) {
                case IntBitwiseKind::BITWISE_AND:
                    if (*rightValue == -1) {
                        return knownInt(left);
                    }
                    if (*rightValue == 0) {
                        return taggedIntConstant(0);
                    }
                    break;
                case IntBitwiseKind::BITWISE_OR:
                    if (*rightValue == 0) {
                        return knownInt(left);
                    }
                    if (*rightValue == -1) {
                        return taggedIntConstant(-1);
                    }
                    break;
                case IntBitwiseKind::BITWISE_XOR:
                    if (*rightValue == 0) {
                        return knownInt(left);
                    }
                    break;
                case IntBitwiseKind::SHIFT_LEFT:
                case IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC:
                    if ((static_cast<uint32_t>(*rightValue) & 31U) == 0) {
                        return knownInt(left);
                    }
                    break;
                case IntBitwiseKind::SHIFT_RIGHT_LOGICAL:
                    break;
            }
        }

        if (leftValue.has_value()) {
            switch (kind) {
                case IntBitwiseKind::BITWISE_AND:
                    if (*leftValue == -1) {
                        return knownInt(right);
                    }
                    if (*leftValue == 0) {
                        return taggedIntConstant(0);
                    }
                    break;
                case IntBitwiseKind::BITWISE_OR:
                    if (*leftValue == 0) {
                        return knownInt(right);
                    }
                    if (*leftValue == -1) {
                        return taggedIntConstant(-1);
                    }
                    break;
                case IntBitwiseKind::BITWISE_XOR:
                    if (*leftValue == 0) {
                        return knownInt(right);
                    }
                    break;
                default:
                    break;
            }
        }

        return nullptr;
    }

    ValueVertex *BuildI32BitwiseTaggedValue(IntBitwiseKind kind, ValueVertex *leftI32, ValueVertex *rightI32)
    {
        ValueVertex *raw = self->NewVertex<I32BitwiseBinaryVertex>(
            compileInfoFacts_, currentBlock, {leftI32, rightI32}, kind);
        if (kind != IntBitwiseKind::SHIFT_RIGHT_LOGICAL) {
            return BuildTaggedI32Result(raw);
        }
        return BuildCheckedNonNegativeI32ToTaggedInt(raw);
    }

    ValueVertex *BuildI32BitwiseBinOp(IntBitwiseKind kind, ValueVertex *left, ValueVertex *right,
                                       bool leftKnownInt, bool rightKnownInt)
    {
        if (leftKnownInt && rightKnownInt) {
            if (ValueVertex *reduced = TryBuildI32BitwiseReduction(kind, left, right)) {
                return reduced;
            }
        }

        ValueVertex *leftI32 = leftKnownInt ? BuildTaggedIntToI32(left) : BuildCheckedTaggedIntToI32(left);
        ValueVertex *rightI32 = rightKnownInt ? BuildTaggedIntToI32(right) : BuildCheckedTaggedIntToI32(right);
        return BuildI32BitwiseTaggedValue(kind, leftI32, rightI32);
    }

    ValueVertex *BuildTruncatingNumberToInt32(ValueVertex *value)
    {
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::INT)) {
            return BuildTaggedIntToI32(value);
        }
        ValueVertex *valueF64 = BuildCheckedNumberToF64(value);
        return self->NewVertex<F64ToI32TruncVertex>(compileInfoFacts_, currentBlock, {valueF64});
    }

    ValueVertex *BuildBitwiseOperation(IntBitwiseKind kind)
    {
        ValueVertex *left = LoadRegister(currentBcInfo, 0);
        ValueVertex *right = frameState.GetAcc();
        bool leftKnownInt = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::INT);
        bool rightKnownInt = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::INT);
        if (leftKnownInt && rightKnownInt) {
            return BuildI32BitwiseBinOp(kind, left, right, leftKnownInt, rightKnownInt);
        }

        OperationFeedback feedback = self->pgoContext_.ReadOperationFeedback(*currentBcInfo);
        if (feedback.hint == ArkSteedOperationHint::INT) {
            LogOperationFeedback(feedback, "Bitwise", "I32BitwiseBinOp");
            return BuildI32BitwiseBinOp(kind, left, right, false, false);
        }
        if (feedback.hint == ArkSteedOperationHint::NUMBER) {
            LogOperationFeedback(feedback, "Bitwise", "TruncatingI32BitwiseBinOp");
            ValueVertex *leftI32 = BuildTruncatingNumberToInt32(left);
            ValueVertex *rightI32 = BuildTruncatingNumberToInt32(right);
            return BuildI32BitwiseTaggedValue(kind, leftI32, rightI32);
        }
        LogOperationFeedback(feedback, "Bitwise", "GenericBitwiseBinOp");
        return BuildGenericBitwiseBinOp(kind, left, right);
    }

    ValueVertex *BuildStringAdd(ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *result = CommonStubCall(
            {glue, left, right, self->graph_->GetInt32Constant(0), GlobalEnv()}, CommonStubID::StringAdd);
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::STRING);
        return result;
    }

    ValueVertex *TryBuildStringAdd(ValueVertex *left, ValueVertex *right, const OperationFeedback &feedback)
    {
        bool leftKnownString = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::STRING);
        bool rightKnownString = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::STRING);

        if (leftKnownString && rightKnownString) {
            if (IsEmptyStringConstant(left)) {
                return right;
            }
            if (IsEmptyStringConstant(right)) {
                return left;
            }
            return BuildStringAdd(left, right);
        }

        switch (feedback.hint) {
            case ArkSteedOperationHint::STRING: {
                LogOperationFeedback(feedback, "BinaryStringAdd", "StringAdd");
                ValueVertex *checkedLeft = leftKnownString ? left : BuildCheckedTaggedString(left);
                ValueVertex *checkedRight = rightKnownString ? right : BuildCheckedTaggedString(right);
                if (leftKnownString && IsEmptyStringConstant(left)) {
                    return checkedRight;
                }
                if (rightKnownString && IsEmptyStringConstant(right)) {
                    return checkedLeft;
                }
                return BuildStringAdd(checkedLeft, checkedRight);
            }

            case ArkSteedOperationHint::NUMBER_OR_STRING: {
                LogOperationFeedback(feedback, "BinaryStringAdd", "StringAdd");
                if (leftKnownString) {
                    if (IsEmptyStringConstant(left)) {
                        return BuildNumberToString(right);
                    }
                    return BuildStringAdd(left, BuildNumberToString(right));
                }
                if (rightKnownString) {
                    if (IsEmptyStringConstant(right)) {
                        return BuildNumberToString(left);
                    }
                    return BuildStringAdd(BuildNumberToString(left), right);
                }
                return nullptr;
            }

            default:
                return nullptr;
        }
    }

    ValueVertex *GetBooleanConstant(bool value)
    {
        JSTaggedValue tagged(value ? JSTaggedValue::VALUE_TRUE : JSTaggedValue::VALUE_FALSE);
        return TaggedConstantFromFoldedValue(tagged);
    }

    ValueVertex *TryBuildKnownIntToBoolean(ValueVertex *value, bool trueIfNonZero)
    {
        if (!compileInfoFacts_->CheckType(value, NodeInfo::NodeType::INT)) {
            return nullptr;
        }
        ValueVertex *valueI32 = BuildTaggedIntToI32(value);
        ValueVertex *zero = self->graph_->GetInt32Constant(0);
        Condition condition = trueIfNonZero ? Condition::NOT_EQUAL : Condition::EQUAL;
        ValueVertex *result = self->NewVertex<I32ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, {valueI32, zero}, condition);
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildGenericCompareOp(JSCondition kind, ValueVertex *left, ValueVertex *right)
    {
        CommonStubID stubId;
        switch (kind) {
            case JSCondition::EQUAL:
                stubId = CommonStubID::Equal;
                break;
            case JSCondition::NOT_EQUAL:
                stubId = CommonStubID::NotEqual;
                break;
            case JSCondition::LESS_THAN:
                stubId = CommonStubID::Less;
                break;
            case JSCondition::LESS_THAN_OR_EQUAL:
                stubId = CommonStubID::LessEq;
                break;
            case JSCondition::GREATER_THAN:
                stubId = CommonStubID::Greater;
                break;
            case JSCondition::GREATER_THAN_OR_EQUAL:
                stubId = CommonStubID::GreaterEq;
                break;
            case JSCondition::STRICT_EQUAL:
                stubId = CommonStubID::StrictEqual;
                break;
            case JSCondition::STRICT_NOT_EQUAL:
                stubId = CommonStubID::StrictNotEqual;
                break;
            default:
                UNREACHABLE();
        }
        ValueVertex *result = CommonStubCall({glue, left, right, GlobalEnv()}, stubId);
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildTaggedEqual(ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *result = self->NewVertex<TaggedEqualVertex>(compileInfoFacts_, currentBlock, {left, right});
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildTaggedNotEqual(ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *result = self->NewVertex<TaggedNotEqualVertex>(compileInfoFacts_, currentBlock, {left, right});
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *TryReduceCompareEqualAgainstConstant(JSCondition kind, ValueVertex *left, ValueVertex *right)
    {
        if (left == right && IsEqualityCompare(kind) &&
            compileInfoFacts_->CheckType(left, NodeInfo::NodeType::INT)) {
            return GetBooleanConstant(IsEqualCompare(kind));
        }
        if (!IsStrictEqualityCompare(kind)) {
            return nullptr;
        }

        bool equalResult = IsEqualCompare(kind);
        NodeInfo::NodeType leftType = compileInfoFacts_->GetKnownType(left);
        NodeInfo::NodeType rightType = compileInfoFacts_->GetKnownType(right);
        if (left == right && !NodeInfo::NodeTypeCanBe(leftType, NodeInfo::NodeType::NUMBER)) {
            return GetBooleanConstant(equalResult);
        }

        if (!StrictTypesCanBeEqual(leftType, rightType)) {
            return GetBooleanConstant(!equalResult);
        }

        if (IsReferenceComparableRootValue(left) || IsReferenceComparableRootValue(right)) {
            return equalResult ? BuildTaggedEqual(left, right) : BuildTaggedNotEqual(left, right);
        }

        if (IsReferenceComparableType(leftType) && IsReferenceComparableType(rightType)) {
            return equalResult ? BuildTaggedEqual(left, right) : BuildTaggedNotEqual(left, right);
        }

        return nullptr;
    }

    ValueVertex *BuildI32CompareTaggedValue(JSCondition kind, ValueVertex *left, ValueVertex *right)
    {
        std::optional<int32_t> leftValue = TryGetInt32Value(left);
        std::optional<int32_t> rightValue = TryGetInt32Value(right);
        if (leftValue.has_value() && rightValue.has_value()) {
            return GetBooleanConstant(EvaluateInt32Compare(kind, *leftValue, *rightValue));
        }

        ValueVertex *leftI32 = BuildTaggedIntToI32(left);
        ValueVertex *rightI32 = BuildTaggedIntToI32(right);
        ValueVertex *result = self->NewVertex<I32ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, {leftI32, rightI32}, Int32ConditionFromCompare(kind));
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildI32CompareOp(JSCondition kind, ValueVertex *left, ValueVertex *right,
                                   bool leftKnownInt, bool rightKnownInt)
    {
        if (leftKnownInt && rightKnownInt) {
            return BuildI32CompareTaggedValue(kind, left, right);
        }

        ValueVertex *leftI32 = leftKnownInt ? BuildTaggedIntToI32(left) : BuildCheckedTaggedIntToI32(left);
        ValueVertex *rightI32 = rightKnownInt ? BuildTaggedIntToI32(right) : BuildCheckedTaggedIntToI32(right);
        ValueVertex *result = self->NewVertex<I32ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, {leftI32, rightI32}, Int32ConditionFromCompare(kind));
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildF64CompareTaggedValue(JSCondition kind, ValueVertex *leftF64, ValueVertex *rightF64)
    {
        if (auto *leftConst = leftF64->TryCast<Float64ConstantVertex>()) {
            if (auto *rightConst = rightF64->TryCast<Float64ConstantVertex>()) {
                return GetBooleanConstant(EvaluateFloat64Compare(kind, leftConst->GetValue(), rightConst->GetValue()));
            }
        }

        ValueVertex *result = self->NewVertex<F64ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, {leftF64, rightF64}, Int32ConditionFromCompare(kind));
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildF64CompareOp(JSCondition kind, ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *leftF64 = BuildCheckedNumberToF64(left);
        ValueVertex *rightF64 = BuildCheckedNumberToF64(right);
        return BuildF64CompareTaggedValue(kind, leftF64, rightF64);
    }

    ValueVertex *BuildStringCompareOp(JSCondition kind, ValueVertex *left, ValueVertex *right)
    {
        if (kind == JSCondition::EQUAL || kind == JSCondition::STRICT_EQUAL) {
            ValueVertex *result = self->NewVertex<StringEqualVertex>(currentBlock, {glue, left, right, GlobalEnv()});
            compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
            return result;
        }
        if (kind == JSCondition::NOT_EQUAL || kind == JSCondition::STRICT_NOT_EQUAL) {
            // NOT_EQUAL = !EQUAL: build StringEqual then negate the boolean.
            ValueVertex *equal = BuildStringCompareOp(InvertCompare(kind), left, right);
            return BuildTaggedNotEqual(equal, GetBooleanConstant(true));
        }
        return BuildGenericCompareOp(kind, left, right);
    }

    ValueVertex *TryBuildStringCompareOp(JSCondition kind, ValueVertex *left, ValueVertex *right,
                                         const OperationFeedback &feedback)
    {
        bool leftKnownString = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::STRING);
        bool rightKnownString = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::STRING);
        if (leftKnownString && rightKnownString) {
            return BuildStringCompareOp(kind, left, right);
        }
        if (feedback.hint != ArkSteedOperationHint::STRING) {
            return nullptr;
        }
        LogOperationFeedback(feedback, "CompareString", "StringCompareOp");
        ValueVertex *checkedLeft = leftKnownString ? left : BuildCheckedTaggedString(left);
        ValueVertex *checkedRight = rightKnownString ? right : BuildCheckedTaggedString(right);
        return BuildStringCompareOp(kind, checkedLeft, checkedRight);
    }

    ValueVertex *TryFoldUint32ComparedToNonPositive(JSCondition kind, ValueVertex *left, ValueVertex *right)
    {
        if (left == nullptr || !left->Is<CheckedNonNegativeI32ToTaggedIntVertex>()) {
            return nullptr;
        }
        std::optional<int32_t> rightValue = TryGetInt32Value(right);
        if (!rightValue.has_value() || *rightValue > 0) {
            return nullptr;
        }
        switch (kind) {
            case JSCondition::GREATER_THAN_OR_EQUAL:
                return GetBooleanConstant(true);   // uint32 >= 0 >= right
            case JSCondition::LESS_THAN:
                return GetBooleanConstant(false);  // uint32 >= 0, cannot be < right (<=0)
            case JSCondition::GREATER_THAN:
                if (*rightValue < 0) {
                    return GetBooleanConstant(true);  // uint32 >= 0 > right
                }
                return nullptr;  // right == 0: uint32 > 0 not always (could be 0)
            default:
                return nullptr;  // LE / equality: not always-resolvable
        }
    }

    ValueVertex *BuildCompareOperation(JSCondition kind)
    {
        ValueVertex *left = LoadRegister(currentBcInfo, 0);
        ValueVertex *right = frameState.GetAcc();

        if (ValueVertex *result = TryReduceCompareEqualAgainstConstant(kind, left, right)) {
            return result;
        }

        if (ValueVertex *result = TryFoldUint32ComparedToNonPositive(kind, left, right)) {
            return result;
        }

        bool leftKnownInt = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::INT);
        bool rightKnownInt = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::INT);
        if (leftKnownInt && rightKnownInt) {
            return BuildI32CompareOp(kind, left, right, leftKnownInt, rightKnownInt);
        }

        OperationFeedback feedback = self->pgoContext_.ReadOperationFeedback(*currentBcInfo);
        if (feedback.hint == ArkSteedOperationHint::INT) {
            LogOperationFeedback(feedback, "Compare", "I32CompareOp");
            return BuildI32CompareOp(kind, left, right, false, false);
        }

        bool leftKnownNumber = compileInfoFacts_->CheckType(left, NodeInfo::NodeType::NUMBER);
        bool rightKnownNumber = compileInfoFacts_->CheckType(right, NodeInfo::NodeType::NUMBER);
        bool leftKnownNonIntNumber = leftKnownNumber && !leftKnownInt;
        bool rightKnownNonIntNumber = rightKnownNumber && !rightKnownInt;
        if (leftKnownNonIntNumber || rightKnownNonIntNumber ||
            feedback.hint == ArkSteedOperationHint::NUMBER) {
            LogOperationFeedback(feedback, "Compare", "F64CompareOp");
            return BuildF64CompareOp(kind, left, right);
        }

        if (leftKnownInt && rightKnownInt) {
            return BuildI32CompareOp(kind, left, right, leftKnownInt, rightKnownInt);
        }

        if (ValueVertex *stringCompare = TryBuildStringCompareOp(kind, left, right, feedback)) {
            return stringCompare;
        }

        if (IsEqualityCompare(kind) && left == right &&
            compileInfoFacts_->CheckType(left, NodeInfo::NodeType::STRING)) {
            return GetBooleanConstant(IsEqualCompare(kind));
        }

        return BuildGenericCompareOp(kind, left, right);
    }

    ValueVertex *BuildGenericBinOp(BinaryOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        switch (kind) {
            case BinaryOpKind::ADD:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Add);
            case BinaryOpKind::SUB:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Sub);
            case BinaryOpKind::MUL:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Mul);
            case BinaryOpKind::DIV:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Div);
            case BinaryOpKind::MOD:
                return CommonStubCall({glue, left, right, GlobalEnv()}, CommonStubID::Mod);
            case BinaryOpKind::EXP:
                return RuntimeCall({left, right}, RTSTUB_ID(Exp));
            default:
                break;
        }
        UNREACHABLE();
    }

    void LogOperationFeedback(const OperationFeedback &feedback, const char *opcodeName,
                              const char *chosenPath) const
    {
        LOG_COMPILER(INFO) << "ArkSteed operation feedback: op=" << opcodeName
                           << ", bcOffset=" << self->preproc_->GetBytecodeOffset(currentBcIndex)
                           << ", slotId=" << feedback.slotId
                           << ", rawTypeBits=" << feedback.rawTypeBits
                           << ", hint=" << static_cast<uint32_t>(feedback.hint)
                           << ", path=" << chosenPath;
    }

    ValueVertex *BuildNumericBinOp(BinaryOpKind kind, ValueVertex *left, ValueVertex *right,
                                    const OperationFeedback &feedback)
    {
        if (feedback.hint == ArkSteedOperationHint::INT && SupportsI32CheckedBinOp(kind)) {
            LogOperationFeedback(feedback, "BinaryNumeric", "I32BinOp");
            return BuildI32BinOp(kind, left, right);
        }
        if (feedback.hint == ArkSteedOperationHint::NUMBER && SupportsF64BinOp(kind)) {
            LogOperationFeedback(feedback, "BinaryNumeric", "F64NumberBinOp");
            return BuildF64NumberBinOp(kind, left, right);
        }
        LogOperationFeedback(feedback, "BinaryNumeric", "GenericBinOp");
        return BuildGenericBinOp(kind, left, right);
    }

    ValueVertex *BuildBinaryOperation(BinaryOpKind kind)
    {
        ValueVertex *left = LoadRegister(currentBcInfo, 0);
        ValueVertex *right = frameState.GetAcc();
        if (ValueVertex *constant = TryBuildConstantBinaryOperation(kind, left, right)) {
            JSTaggedValue value(constant->Cast<TaggedConstantVertex>()->GetValue());
            compileInfoFacts_->EnsureType(constant, NodeTypeFromJSTaggedValue(value));
            return constant;
        }
        OperationFeedback feedback = self->pgoContext_.ReadOperationFeedback(*currentBcInfo);
        bool observedNonInt32Result =
            SupportsF64BinOp(kind) && feedback.hint == ArkSteedOperationHint::NUMBER;
        if (!observedNonInt32Result) {
            if (ValueVertex *provenInt = TryBuildProvenIntBinOp(kind, left, right)) {
                return provenInt;
            }
        }
        if (kind == BinaryOpKind::ADD) {
            if (ValueVertex *stringAdd = TryBuildStringAdd(left, right, feedback)) {
                return stringAdd;
            }
        }
        return BuildNumericBinOp(kind, left, right, feedback);
    }

    ValueVertex *BuildGenericUnaryOp(CommonStubID stubId, ValueVertex *value)
    {
        return CommonStubCall({glue, value}, stubId);
    }

    ValueVertex *BuildIntUnaryOp(CommonStubID stubId, ValueVertex *value, bool valueKnownInt)
    {
        ValueVertex *valueI32 = valueKnownInt ? BuildTaggedIntToI32(value) : BuildCheckedTaggedIntToI32(value);
        std::vector<ValueVertex *> inputs {valueI32};

        switch (stubId) {
            case CommonStubID::Inc: {
                EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
                ValueVertex *rawResult = self->NewVertex<I32IncWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                rawResult->Cast<I32IncWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return BuildTaggedI32Result(rawResult);
            }
            case CommonStubID::Dec: {
                EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
                ValueVertex *rawResult = self->NewVertex<I32DecWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                rawResult->Cast<I32DecWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return BuildTaggedI32Result(rawResult);
            }
            case CommonStubID::Neg: {
                EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(currentBcIndex);
                ValueVertex *rawResult = self->NewVertex<I32NegWithOverflowVertex>(
                    currentBlock, inputs, self->chunk_, self->preproc_->GetBytecodeOffset(currentBcIndex));
                rawResult->Cast<I32NegWithOverflowVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
                return BuildTaggedI32Result(rawResult);
            }
            case CommonStubID::Not: {
                ValueVertex *rawResult =
                    self->NewVertex<I32BNotVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueI32});
                return BuildTaggedI32Result(rawResult);
            }
            default:
                return BuildGenericUnaryOp(stubId, value);
        }
    }

    ValueVertex *BuildTruncatingI32BNot(ValueVertex *value)
    {
        if (compileInfoFacts_->CheckType(value, NodeInfo::NodeType::INT)) {
            ValueVertex *valueI32 = BuildTaggedIntToI32(value);
            ValueVertex *rawResult = self->NewVertex<I32BNotVertex>(compileInfoFacts_, currentBlock, {valueI32});
            return BuildTaggedI32Result(rawResult);
        }
        ValueVertex *valueF64 = BuildCheckedNumberToF64(value);
        ValueVertex *truncI32 = self->NewVertex<F64ToI32TruncVertex>(compileInfoFacts_, currentBlock, {valueF64});
        ValueVertex *rawResult = self->NewVertex<I32BNotVertex>(compileInfoFacts_, currentBlock, {truncI32});
        return BuildTaggedI32Result(rawResult);
    }

    ValueVertex *BuildF64UnaryOp(CommonStubID stubId, ValueVertex *value)
    {
        ValueVertex *valueF64 = BuildCheckedNumberToF64(value);
        switch (stubId) {
            case CommonStubID::Neg: {
                ValueVertex *negF64 = self->NewVertex<F64NegVertex>(compileInfoFacts_, currentBlock, {valueF64});
                ValueVertex *result = self->NewVertex<F64ToTaggedDoubleVertex>(
                    compileInfoFacts_, currentBlock, {negF64});
                compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::NUMBER);
                return result;
            }
            case CommonStubID::Inc: {
                ValueVertex *oneF64 = self->graph_->GetFloat64Constant(1.0);
                ValueVertex *addF64 = self->NewVertex<F64AddVertex>(
                    compileInfoFacts_, currentBlock, {valueF64, oneF64});
                ValueVertex *result = self->NewVertex<F64ToTaggedDoubleVertex>(
                    compileInfoFacts_, currentBlock, {addF64});
                compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::NUMBER);
                return result;
            }
            case CommonStubID::Dec: {
                ValueVertex *oneF64 = self->graph_->GetFloat64Constant(1.0);
                ValueVertex *subF64 = self->NewVertex<F64SubVertex>(
                    compileInfoFacts_, currentBlock, {valueF64, oneF64});
                ValueVertex *result = self->NewVertex<F64ToTaggedDoubleVertex>(
                    compileInfoFacts_, currentBlock, {subF64});
                compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::NUMBER);
                return result;
            }
            case CommonStubID::Not:
                return BuildTruncatingI32BNot(value);
            default:
                return BuildGenericUnaryOp(stubId, value);
        }
    }

    ValueVertex *BuildUnaryOperation(CommonStubID stubId)
    {
        ValueVertex *value = frameState.GetAcc();
        if (ValueVertex *constant = TryBuildConstantUnaryOperation(stubId, value)) {
            return constant;
        }
        bool valueKnownInt = compileInfoFacts_->CheckType(value, NodeInfo::NodeType::INT);
        if (valueKnownInt) {
            return BuildIntUnaryOp(stubId, value, true);
        }

        OperationFeedback feedback = self->pgoContext_.ReadOperationFeedback(*currentBcInfo);
        if (feedback.hint == ArkSteedOperationHint::INT) {
            LogOperationFeedback(feedback, "Unary", "IntUnaryOp");
            return BuildIntUnaryOp(stubId, value, false);
        }
        if (feedback.hint == ArkSteedOperationHint::NUMBER ||
            feedback.hint == ArkSteedOperationHint::NUMBER_OR_STRING) {
            LogOperationFeedback(feedback, "Unary", "F64UnaryOp");
            return BuildF64UnaryOp(stubId, value);
        }
        LogOperationFeedback(feedback, "Unary", "GenericUnaryOp");
        return BuildGenericUnaryOp(stubId, value);
    }

    ValueVertex *TryBuildConstantUnaryOperation(CommonStubID stubId, ValueVertex *value)
    {
        if (std::optional<int32_t> intValue = TryGetInt32Value(value)) {
            return TryBuildInt32ConstantUnaryOperation(stubId, *intValue);
        }
        auto *float64Constant = value->TryCast<Float64ConstantVertex>();
        if (float64Constant == nullptr) {
            return nullptr;
        }
        return TryBuildFloat64ConstantUnaryOperation(stubId, float64Constant->GetValue());
    }

    ValueVertex *TryBuildInt32ConstantUnaryOperation(CommonStubID stubId, int32_t value)
    {
        switch (stubId) {
            case CommonStubID::Inc:
                if (value == std::numeric_limits<int32_t>::max()) {
                    return nullptr;
                }
                return TaggedConstantFromFoldedValue(JSTaggedValue(value + 1));
            case CommonStubID::Dec:
                if (value == std::numeric_limits<int32_t>::min()) {
                    return nullptr;
                }
                return TaggedConstantFromFoldedValue(JSTaggedValue(value - 1));
            case CommonStubID::Neg:
                if (value == 0 || value == std::numeric_limits<int32_t>::min()) {
                    return nullptr;
                }
                return TaggedConstantFromFoldedValue(JSTaggedValue(-value));
            case CommonStubID::Not:
                return TaggedConstantFromFoldedValue(JSTaggedValue(~value));
            default:
                return nullptr;
        }
    }

    ValueVertex *TryBuildFloat64ConstantUnaryOperation(CommonStubID stubId, double value)
    {
        switch (stubId) {
            case CommonStubID::Inc:
                return TaggedConstantFromFoldedValue(JSTaggedValue(value + 1.0));
            case CommonStubID::Dec:
                return TaggedConstantFromFoldedValue(JSTaggedValue(value - 1.0));
            case CommonStubID::Neg:
                return TaggedConstantFromFoldedValue(JSTaggedValue(-value));
            default:
                return nullptr;
        }
    }

    ValueVertex *TryBuildFloat64ConstantBinaryOperation(BinaryOpKind kind, double lhs, double rhs)
    {
        double result = 0.0;
        switch (kind) {
            case BinaryOpKind::ADD:
                result = lhs + rhs;
                break;
            case BinaryOpKind::SUB:
                result = lhs - rhs;
                break;
            case BinaryOpKind::MUL:
                result = lhs * rhs;
                break;
            case BinaryOpKind::DIV:
                if (rhs == 0.0) {
                    // x / 0 -> Infinity / -Infinity / NaN; the sign and NaN rules are best left
                    // to the runtime F64DivVertex rather than reproduced at compile time.
                    return nullptr;
                }
                result = lhs / rhs;
                break;
            default:
                return nullptr;
        }
        return TaggedConstantFromFoldedValue(JSTaggedValue(result));
    }

    ValueVertex *TryBuildConstantBinaryOperation(BinaryOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        std::optional<int32_t> leftValue = TryGetInt32Value(left);
        std::optional<int32_t> rightValue = TryGetInt32Value(right);
        if (leftValue.has_value() && rightValue.has_value()) {
            int32_t lhs = *leftValue;
            int32_t rhs = *rightValue;
            int64_t result = 0;
            switch (kind) {
                case BinaryOpKind::ADD:
                    result = static_cast<int64_t>(lhs) + static_cast<int64_t>(rhs);
                    break;
                case BinaryOpKind::SUB:
                    result = static_cast<int64_t>(lhs) - static_cast<int64_t>(rhs);
                    break;
                case BinaryOpKind::MUL:
                    if ((lhs == 0 || rhs == 0) && (lhs < 0 || rhs < 0)) {
                        return nullptr;
                    }
                    result = static_cast<int64_t>(lhs) * static_cast<int64_t>(rhs);
                    break;
                default:
                    // Int DIV/MOD/EXP are left unfolded.
                    return nullptr;
            }
            if (result < std::numeric_limits<int32_t>::min() || result > std::numeric_limits<int32_t>::max()) {
                return nullptr;
            }
            return self->graph_->GetTaggedConstant(JSTaggedValue(static_cast<int32_t>(result)).GetRawData());
        }

        // Float64 two-constant fold (ADD/SUB/MUL/DIV). Only fires for double constants, so int
        // DIV/MOD are unaffected.
        std::optional<double> leftF64 = TryGetFloat64Value(left);
        std::optional<double> rightF64 = TryGetFloat64Value(right);
        if (leftF64.has_value() && rightF64.has_value()) {
            return TryBuildFloat64ConstantBinaryOperation(kind, *leftF64, *rightF64);
        }
        return nullptr;
    }

    bool BuildCheckHClass(uint32_t bcIndex, ValueVertex *object, JSHClass *hclass,
                          bool installStableDependency = true, bool hasExternalStableDependency = false)
    {
        if (compileInfoFacts_->TryGetHClass(object) == hclass) {
            return true;
        }
        bool isSharedHClass = JSTaggedValue(hclass).IsInSharedHeap();
        bool hasStableDependency = false;
        bool shouldInstallStableDependency = installStableDependency && self->IsLazyDeoptEnabled() &&
            kungfu::StableHClassDependency::IsValid(hclass) && !isSharedHClass;
        if (shouldInstallStableDependency) {
            auto *dependencies = self->preproc_->GetEnv()->GetDependencies();
            if (dependencies == nullptr || !dependencies->DependOnStableHClass(hclass)) {
                return false;
            }
            hasStableDependency = true;
        }
        bool canAssumeStableHClass = isSharedHClass ||
            (self->IsLazyDeoptEnabled() && (hasStableDependency || hasExternalStableDependency));
        if (std::optional<JSTaggedValue> constant = TryGetConstantHeapObject(object)) {
            if (constant->IsHole() || !constant->IsHeapObject() ||
                constant->GetTaggedObject()->GetClass() != hclass) {
                return false;
            }
            compileInfoFacts_->RecordHClass(object, hclass, canAssumeStableHClass);
            return true;
        }

        std::vector<ValueVertex *> checkInputs {object};
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(bcIndex);
        self->NewVertex<DeoptIfHClassMismatchVertex>(
            currentBlock, checkInputs, self->chunk_, hclass, self->preproc_->GetBytecodeOffset(bcIndex))
            ->SetEagerDeoptFrameState(std::move(deoptFrameState));
        compileInfoFacts_->RecordHClass(object, hclass, canAssumeStableHClass);
        return true;
    }

    static bool ContainsSameHClasses(const std::vector<JSHClass *> &lhs, const std::vector<JSHClass *> &rhs)
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        return std::all_of(lhs.begin(), lhs.end(), [&rhs](JSHClass *hclass) {
            return std::find(rhs.begin(), rhs.end(), hclass) != rhs.end();
        });
    }

    static bool IsHClassSubset(const std::vector<JSHClass *> &subset, const std::vector<JSHClass *> &superset)
    {
        return std::all_of(subset.begin(), subset.end(), [&superset](JSHClass *hclass) {
            return std::find(superset.begin(), superset.end(), hclass) != superset.end();
        });
    }

    bool BuildCheckHClasses(uint32_t bcIndex, ValueVertex *object, const std::vector<JSHClass *> &hclasses,
                            bool mapsAreKnownFresh, bool canAssumeStableHClasses)
    {
        if (hclasses.empty()) {
            return false;
        }
        if (hclasses.size() == 1) {
            return BuildCheckHClass(bcIndex, object, hclasses.front(), false, canAssumeStableHClasses);
        }

        std::optional<std::vector<JSHClass *>> knownHClasses = compileInfoFacts_->TryGetPossibleHClasses(object);
        if (mapsAreKnownFresh && knownHClasses.has_value() && ContainsSameHClasses(knownHClasses.value(), hclasses)) {
            compileInfoFacts_->RecordPossibleHClasses(object, hclasses, canAssumeStableHClasses);
            return true;
        }
        if (knownHClasses.has_value() && !knownHClasses->empty() && IsHClassSubset(knownHClasses.value(), hclasses)) {
            return true;
        }

        if (std::any_of(hclasses.begin(), hclasses.end(), [](JSHClass *hclass) {
            return hclass == nullptr;
        })) {
            return false;
        }
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(bcIndex);
        auto *check = self->NewVertex<DeoptIfHClassNotInVertex>(
            currentBlock, {object}, self->chunk_, hclasses, self->preproc_->GetBytecodeOffset(bcIndex));
        check->Cast<DeoptIfHClassNotInVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        compileInfoFacts_->RecordPossibleHClasses(object, hclasses, canAssumeStableHClasses);
        return true;
    }

    bool BuildEagerCheckHClassesWithoutDependencies(
        uint32_t bcIndex, ValueVertex *object, const std::vector<JSHClass *> &hclasses)
    {
        if (hclasses.empty() || std::any_of(hclasses.begin(), hclasses.end(), [](JSHClass *hclass) {
            return hclass == nullptr;
        })) {
            return false;
        }

        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(bcIndex);
        if (hclasses.size() == 1) {
            auto *check = self->NewVertex<DeoptIfHClassMismatchVertex>(
                currentBlock, {object}, self->chunk_, hclasses.front(),
                self->preproc_->GetBytecodeOffset(bcIndex));
            check->Cast<DeoptIfHClassMismatchVertex>()->SetEagerDeoptFrameState(
                std::move(deoptFrameState));
        } else {
            auto *check = self->NewVertex<DeoptIfHClassNotInVertex>(
                currentBlock, {object}, self->chunk_, hclasses,
                self->preproc_->GetBytecodeOffset(bcIndex));
            check->Cast<DeoptIfHClassNotInVertex>()->SetEagerDeoptFrameState(
                std::move(deoptFrameState));
        }
        compileInfoFacts_->RecordPossibleHClasses(object, hclasses, false);
        return true;
    }

    ValueVertex *BuildLoadField(ValueVertex *object, PropertyLookupResult plr)
    {
        auto convertHoleToUndefined = [this, plr](ValueVertex *value) -> ValueVertex * {
            if (plr.IsNotHole() || !plr.IsLoadFromIterResult()) {
                return value;
            }
            return self->NewVertex<ConvertHoleToUndefinedVertex>(compileInfoFacts_, currentBlock, {value});
        };
        if (plr.IsInlinedProps()) {
            int32_t offset = static_cast<int32_t>(plr.GetOffset());
            ValueVertex *result =
                self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {object}, offset);
            return convertHoleToUndefined(result);
        }
        ValueVertex *properties = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {object}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET +
                                              plr.GetOffset() * JSTaggedValue::TaggedTypeSize());
        ValueVertex *result =
            self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {properties}, offset);
        return convertHoleToUndefined(result);
    }

    ValueVertex *BuildLoadFieldWithoutCse(ValueVertex *object, PropertyLookupResult plr)
    {
        if (plr.IsInlinedProps()) {
            int32_t offset = static_cast<int32_t>(plr.GetOffset());
            return self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {object}, offset);
        }
        ValueVertex *properties = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {object}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET +
                                              plr.GetOffset() * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {properties}, offset);
    }

    void BuildStoreTaggedField(ValueVertex *object, int32_t offset, ValueVertex *value)
    {
        ArkSteedWriteBarrierValueKind valueKind = ClassifyDirectWriteBarrierValueKind(value);
        if (valueKind == ArkSteedWriteBarrierValueKind::NonHeap) {
            self->NewVertex<StoreTaggedFieldVertex>(compileInfoFacts_, currentBlock, {object, value}, offset);
            return;
        }
        self->NewVertex<StoreTaggedFieldWithBarrierVertex>(
            compileInfoFacts_, currentBlock, {glue, object, value}, offset, valueKind);
    }

    void BuildStoreField(ValueVertex *object, ValueVertex *value, PropertyLookupResult plr)
    {
        if (plr.IsInlinedProps()) {
            BuildStoreTaggedField(object, static_cast<int32_t>(plr.GetOffset()), value);
            return;
        }
        ValueVertex *properties = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {object}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        int32_t offset = static_cast<int32_t>(
            TaggedArray::DATA_OFFSET + plr.GetOffset() * JSTaggedValue::TaggedTypeSize());
        BuildStoreTaggedField(properties, offset, value);
    }

    ValueVertex *BuildPropertyLoadSource(uint32_t bcIndex, ValueVertex *object, const NamedLoadAccessInfo &accessInfo)
    {
        if (accessInfo.holderDepth <= 0) {
            return object;
        }
        if (accessInfo.hasStableProtoChain) {
            // Proto chain is protected by lazy-deopt dependency. Emit direct prototype loads without eager guards.
            constexpr int32_t HCLASS_OFFSET = static_cast<int32_t>(TaggedObject::HCLASS_OFFSET);
            constexpr int32_t PROTOTYPE_OFFSET = static_cast<int32_t>(JSHClass::PROTOTYPE_OFFSET);
            for (uint32_t step = 0; step < accessInfo.holderDepth; step++) {
                auto *hclass = self->NewVertex<LoadTaggedFieldVertex>(
                    compileInfoFacts_, currentBlock, {object}, HCLASS_OFFSET);
                object = self->NewVertex<LoadTaggedFieldVertex>(
                    compileInfoFacts_, currentBlock, {hclass}, PROTOTYPE_OFFSET);
            }
            return object;
        } else {
            // Eager deopt check is required
            ChunkVector<ValueVertex *> checkInputs(self->chunk_);
            checkInputs.emplace_back(object);
            auto *loadHolder = self->NewVertex<LoadPrototypeHolderByHClassVertex>(
                compileInfoFacts_,
                currentBlock,
                checkInputs,
                self->chunk_,
                accessInfo.holderHClass,
                accessInfo.expectedPrototypeHClasses,
                accessInfo.holderDepth,
                self->preproc_->GetBytecodeOffset(bcIndex));
            loadHolder->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
            return loadHolder;
        }
    }

    bool TryResolveHeapRef(const ArkSteedHeapRef &ref, JSTaggedValue *value) const
    {
        auto *broker = self->pgoContext_.GetBroker();
        return broker != nullptr && broker->TryResolveRef(ref, value);
    }

    bool TryResolveHClassRef(const ArkSteedHClassRef &ref, JSHClass **hclass) const
    {
        if (hclass == nullptr) {
            return false;
        }
        JSTaggedValue value = JSTaggedValue::Undefined();
        if (!TryResolveHeapRef(ref, &value) || !value.IsJSHClass()) {
            return false;
        }
        *hclass = JSHClass::Cast(value.GetTaggedObject());
        return true;
    }

    ValueVertex *GetHeapConstant(const ArkSteedHeapRef &ref)
    {
        if (g_isEnableCMCGC) {
            return nullptr;
        }
        ArkSteedHeapBroker *broker = self->pgoContext_.GetBroker();
        if (broker == nullptr) {
            return nullptr;
        }
        ArkSteedHeapBroker::SerializingScope scope(broker, "GraphBuilder::GetHeapConstant");
        uint32_t handleIndex = JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX;
        JSTaggedValue value = JSTaggedValue::Undefined();
        if (!broker->TryRecordHeapConstant(ref, &handleIndex, &value)) {
            return nullptr;
        }
        uint16_t staticNodeType = static_cast<uint16_t>(NodeTypeFromJSTaggedValue(value));
        return self->graph_->GetHeapConstant(handleIndex, staticNodeType);
    }

    JSHClass *TryGetKnownHClass(ValueVertex *receiver) const
    {
        if (std::optional<JSTaggedValue> constant = TryGetConstantHeapObject(receiver)) {
            if (constant->IsHeapObject()) {
                return constant->GetTaggedObject()->GetClass();
            }
        }
        return compileInfoFacts_->TryGetHClass(receiver);
    }

    bool RequireKnownHClass(uint32_t bcIndex, const NamedStoreAccessInfo &access,
                            ValueVertex *receiver, JSHClass *receiverHClass)
    {
        return receiverHClass != nullptr &&
            BuildCheckHClass(bcIndex, receiver, receiverHClass, false,
                             access.dependencies.canAssumeStableHClass);
    }

    bool TryLowerNamedStoreShared(uint32_t bcIndex, const NamedStoreAccessInfo &access, ValueVertex *receiver,
                                  ValueVertex *value)
    {
        JSHClass *receiverHClass = nullptr;
        if (access.mode != AccessMode::NAMED_STORE || !access.isSharedStore || !access.holderIsReceiver ||
            !TryResolveHClassRef(access.expectedHClass, &receiverHClass)) {
            return false;
        }
        if (!RequireKnownHClass(bcIndex, access, receiver, receiverHClass)) {
            return false;
        }

        ValueVertex *storeTarget = receiver;
        if (access.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY) {
            storeTarget = self->NewVertex<LoadTaggedFieldVertex>(
                compileInfoFacts_, currentBlock, {receiver}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        } else if (access.fieldStorage != AccessFieldStorage::IN_OBJECT) {
            return false;
        }

        auto *prepareField = self->NewVertex<PrepareSharedStoreFieldVertex>(
            compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *> {value}, access.handlerInfo);
        UpdateCatchBlockData(prepareField);
        LoadLazyDeoptFrameStateForThrowableCall(bcIndex, prepareField);
        self->NewVertex<StoreSharedFieldWithBarrierVertex>(
            compileInfoFacts_, currentBlock, {glue, storeTarget, prepareField}, access.fieldOffset);
        return true;
    }

    bool TryLowerNamedStoreAccessor(uint32_t bcIndex, const NamedStoreAccessInfo &access, ValueVertex *receiver,
                                    ValueVertex *value)
    {
        if (access.mode != AccessMode::NAMED_STORE || access.kind != AccessKind::ACCESSOR) {
            return false;
        }

        ValueVertex *holder = receiver;
        if (!access.holderIsReceiver) {
            holder = BuildPrototypeHolder(bcIndex, access, receiver);
            if (holder == nullptr) {
                return false;
            }
        }

        ValueVertex *accessorHolder = holder;
        if (access.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY) {
            accessorHolder = self->NewVertex<LoadTaggedFieldVertex>(
                compileInfoFacts_, currentBlock, {holder}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        } else if (access.fieldStorage != AccessFieldStorage::IN_OBJECT) {
            return false;
        }
        ValueVertex *accessor = self->NewVertex<LoadTaggedFieldVertex>(
            compileInfoFacts_, currentBlock, {accessorHolder}, access.fieldOffset);

        CompileInfoFacts *entryFacts = compileInfoFacts_;
        BB *internalAccessorBlock = self->NewBlock();
        BB *loadSetterBlock = self->NewBlock();
        BB *undefinedSetterBlock = self->NewBlock();
        BB *callSetterBlock = self->NewBlock();
        BB *doneBlock = self->NewBlock();
        internalAccessorBlock->SetDeferred(true);
        undefinedSetterBlock->SetDeferred(true);

        self->FinishBlockWithBranch<BranchIfObjectTypeVertex>(
            currentBlock, {accessor}, internalAccessorBlock, loadSetterBlock, JSType::INTERNAL_ACCESSOR);

        currentBlock = internalAccessorBlock;
        compileInfoFacts_ = entryFacts->Clone();
        auto *internalCall = RuntimeCall({receiver, accessor, value}, RTSTUB_ID(CallInternalSetter));
        LoadLazyDeoptFrameStateForThrowableCall(bcIndex, internalCall);
        self->FinishBlockWithJump(currentBlock, doneBlock);

        currentBlock = loadSetterBlock;
        compileInfoFacts_ = entryFacts->Clone();
        ValueVertex *setter = self->NewVertex<LoadTaggedFieldVertex>(
            compileInfoFacts_, currentBlock, {accessor}, static_cast<int32_t>(AccessorData::SETTER_OFFSET));
        CompileInfoFacts *setterFacts = compileInfoFacts_;
        self->FinishBlockWithBranch<BranchIfReferenceEqualVertex>(
            currentBlock, {setter, self->undefinedValue_}, undefinedSetterBlock, callSetterBlock);

        currentBlock = undefinedSetterBlock;
        compileInfoFacts_ = setterFacts->Clone();
        auto *throwCall = RuntimeCall({}, RTSTUB_ID(ThrowSetterIsUndefinedException));
        LoadLazyDeoptFrameStateForThrowableCall(bcIndex, throwCall);
        self->FinishBlockWithJump(currentBlock, doneBlock);

        currentBlock = callSetterBlock;
        compileInfoFacts_ = setterFacts->Clone();
        CallVertex *call = BuildCallVertex(
            std::initializer_list<ValueVertex *> {setter, self->undefinedValue_, receiver, value}, 1);
        LoadLazyDeoptFrameStateForThrowableCall(bcIndex, call);
        self->FinishBlockWithJump(currentBlock, doneBlock);

        currentBlock = doneBlock;
        compileInfoFacts_ = entryFacts;
        compileInfoFacts_->OnSideEffect();
        return true;
    }

    bool BuildNamedStoreEagerGuards(uint32_t bcIndex, const NamedStoreAccessInfo &access, ValueVertex *receiver,
                                    bool checkNotPrototype = false)
    {
        bool needsStableProtoChain =
            access.hasProtoCell || !access.holderIsReceiver || access.kind == AccessKind::TRANSITION;
        bool needsProtoMarker = needsStableProtoChain &&
            !access.dependencies.canAssumeStableProtoChain;
        checkNotPrototype = checkNotPrototype && !access.dependencies.canAssumeNotPrototype;
        if (!needsProtoMarker && !checkNotPrototype) {
            return true;
        }
        ChunkVector<ValueVertex *> guardInputs(self->chunk_);
        guardInputs.emplace_back(receiver);
        auto *guard = self->NewVertex<DeoptIfPrototypeChangedVertex>(
            currentBlock, guardInputs, self->chunk_, needsProtoMarker, checkNotPrototype,
            self->preproc_->GetBytecodeOffset(bcIndex));
        guard->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
        return true;
    }

    ValueVertex *BuildPrototypeHolder(uint32_t bcIndex, const NamedStoreAccessInfo &access, ValueVertex *receiver)
    {
        JSHClass *holderHClass = nullptr;
        if (!access.HasHolderHClass() || !TryResolveHClassRef(access.holderHClass, &holderHClass)) {
            return nullptr;
        }
        ChunkVector<ValueVertex *> holderInputs(self->chunk_);
        holderInputs.emplace_back(receiver);
        auto *holder = self->NewVertex<FindPrototypeHolderVertex>(
            compileInfoFacts_, currentBlock, holderInputs, self->chunk_, holderHClass,
            self->preproc_->GetBytecodeOffset(bcIndex));
        holder->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
        return holder;
    }

    ValueVertex *BuildCheckedNamedStoreValue(AccessFieldRepresentation representation, ValueVertex *value)
    {
        switch (representation) {
            case AccessFieldRepresentation::TAGGED:
                return value;
            case AccessFieldRepresentation::INT32:
                return BuildCheckedTaggedIntToI32(value);
            case AccessFieldRepresentation::DOUBLE:
                return BuildCheckedNumberToF64(value);
            default:
                return nullptr;
        }
    }

    void BuildPreparedNamedStoreField(ValueVertex *storeTarget, int32_t offset, ValueVertex *value,
                                      AccessFieldRepresentation representation)
    {
        switch (representation) {
            case AccessFieldRepresentation::TAGGED:
                BuildStoreTaggedField(storeTarget, offset, value);
                return;
            case AccessFieldRepresentation::INT32:
                self->NewVertex<StoreInt32FieldVertex>(compileInfoFacts_, currentBlock,
                                                       {storeTarget, value}, offset);
                return;
            case AccessFieldRepresentation::DOUBLE:
                self->NewVertex<StoreDoubleFieldVertex>(compileInfoFacts_, currentBlock,
                                                        {storeTarget, value}, offset);
                return;
            default:
                UNREACHABLE();
        }
    }

    bool TryLowerNamedStoreTransition(uint32_t bcIndex, const NamedStoreAccessInfo &access,
                                      ValueVertex *receiver, ValueVertex *value)
    {
        JSHClass *receiverHClass = nullptr;
        JSHClass *transitionHClass = nullptr;
        bool supportedRepresentation = access.fieldRepresentation == AccessFieldRepresentation::TAGGED ||
            access.fieldRepresentation == AccessFieldRepresentation::INT32 ||
            access.fieldRepresentation == AccessFieldRepresentation::DOUBLE;
        if (access.mode != AccessMode::NAMED_STORE || access.kind != AccessKind::TRANSITION ||
            !access.holderIsReceiver || !supportedRepresentation ||
            !TryResolveHClassRef(access.expectedHClass, &receiverHClass) ||
            !TryResolveHClassRef(access.transitionHClass, &transitionHClass)) {
            return false;
        }
        if (access.fieldStorage != AccessFieldStorage::IN_OBJECT &&
            access.fieldStorage != AccessFieldStorage::PROPERTIES_ARRAY) {
            return false;
        }
        ValueVertex *transitionHClassValue = GetHeapConstant(access.transitionHClass);
        if (transitionHClassValue == nullptr) {
            return false;
        }
        if (receiverHClass->IsPrototype()) {
            return false;
        }
        if (!RequireKnownHClass(bcIndex, access, receiver, receiverHClass)) {
            return false;
        }

        if (!BuildNamedStoreEagerGuards(bcIndex, access, receiver, true)) {
            return false;
        }
        ValueVertex *preparedValue = BuildCheckedNamedStoreValue(access.fieldRepresentation, value);
        if (preparedValue == nullptr) {
            return false;
        }

        self->NewVertex<TransitionHClassWithBarrierVertex>(
            compileInfoFacts_, currentBlock, {glue, receiver, transitionHClassValue});

        if (access.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY) {
            auto *properties = self->NewVertex<EnsurePropertiesCapacityVertex>(
                compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *> {glue, receiver},
                static_cast<int32_t>(access.fieldIndex));
            UpdateCatchBlockData(properties);
            LoadLazyDeoptFrameStateForThrowableCall(bcIndex, properties);
            BuildPreparedNamedStoreField(properties, access.fieldOffset, preparedValue,
                                         access.fieldRepresentation);
        } else {
            BuildPreparedNamedStoreField(receiver, access.fieldOffset, preparedValue,
                                         access.fieldRepresentation);
        }
        compileInfoFacts_->RecordHClass(receiver, transitionHClass, false);
        return true;
    }

    bool TryLowerNamedStoreField(uint32_t bcIndex, const NamedStoreAccessInfo &access,
                                 ValueVertex *receiver, ValueVertex *value)
    {
        if (access.kind == AccessKind::TRANSITION) {
            return TryLowerNamedStoreTransition(bcIndex, access, receiver, value);
        }
        if (access.isSharedStore) {
            return TryLowerNamedStoreShared(bcIndex, access, receiver, value);
        }
        if (access.mode != AccessMode::NAMED_STORE) {
            return false;
        }

        JSHClass *receiverHClass = nullptr;
        if (!TryResolveHClassRef(access.expectedHClass, &receiverHClass) ||
            !RequireKnownHClass(bcIndex, access, receiver, receiverHClass)) {
            return false;
        }
        if (!BuildNamedStoreEagerGuards(bcIndex, access, receiver)) {
            return false;
        }
        if (access.kind == AccessKind::ACCESSOR) {
            return TryLowerNamedStoreAccessor(bcIndex, access, receiver, value);
        }
        if (!access.IsDataField()) {
            return false;
        }

        ValueVertex *storeTarget = receiver;
        if (access.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY) {
            storeTarget = self->NewVertex<LoadTaggedFieldVertex>(
                compileInfoFacts_, currentBlock, {receiver}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        } else if (access.fieldStorage != AccessFieldStorage::IN_OBJECT) {
            return false;
        }

        if (access.fieldRepresentation != AccessFieldRepresentation::TAGGED) {
            if (access.fieldRepresentation != AccessFieldRepresentation::INT32 &&
                access.fieldRepresentation != AccessFieldRepresentation::DOUBLE) {
                return false;
            }
            ChunkVector<ValueVertex *> storeInputs(self->chunk_);
            storeInputs.emplace_back(storeTarget);
            storeInputs.emplace_back(value);
            EagerDeoptimizableMixin *store = nullptr;
            if (access.fieldRepresentation == AccessFieldRepresentation::INT32) {
                store = self->NewVertex<StoreInt32FieldWithRepVertex>(
                    compileInfoFacts_, currentBlock, storeInputs, self->chunk_, access.fieldOffset,
                    self->preproc_->GetBytecodeOffset(bcIndex));
            } else {
                store = self->NewVertex<StoreDoubleFieldWithRepVertex>(
                    compileInfoFacts_, currentBlock, storeInputs, self->chunk_, access.fieldOffset,
                    self->preproc_->GetBytecodeOffset(bcIndex));
            }
            store->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
            return true;
        }

        BuildStoreTaggedField(storeTarget, access.fieldOffset, value);
        return true;
    }

    static bool IsLocalTaggedStoreField(const NamedStoreAccessInfo &access)
    {
        return access.mode == AccessMode::NAMED_STORE && !access.isSharedStore && !access.hasProtoCell &&
            access.IsDataField() && access.holderIsReceiver &&
            access.fieldRepresentation == AccessFieldRepresentation::TAGGED &&
            (access.fieldStorage == AccessFieldStorage::IN_OBJECT ||
             access.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY) &&
            access.expectedHClass.IsSafeForCompile();
    }

    static bool IsLocalPolyNamedStoreCase(const NamedStoreAccessInfo &access)
    {
        if (access.mode != AccessMode::NAMED_STORE || access.isSharedStore ||
            !access.expectedHClass.IsSafeForCompile()) {
            return false;
        }
        if (!access.holderIsReceiver && !access.hasProtoCell) {
            return false;
        }
        if (access.fieldStorage != AccessFieldStorage::IN_OBJECT &&
            access.fieldStorage != AccessFieldStorage::PROPERTIES_ARRAY) {
            return false;
        }
        if (access.kind == AccessKind::TRANSITION) {
            bool supportedRepresentation = access.fieldRepresentation == AccessFieldRepresentation::TAGGED ||
                access.fieldRepresentation == AccessFieldRepresentation::INT32 ||
                access.fieldRepresentation == AccessFieldRepresentation::DOUBLE;
            return access.holderIsReceiver && supportedRepresentation && access.transitionHClass.IsSafeForCompile();
        }
        if (access.kind == AccessKind::ACCESSOR) {
            return access.holderIsReceiver || (access.hasFieldHClass && access.fieldHClass.IsSafeForCompile());
        }
        if (!access.IsDataField()) {
            return false;
        }
        return access.fieldRepresentation == AccessFieldRepresentation::TAGGED ||
            access.fieldRepresentation == AccessFieldRepresentation::INT32 ||
            access.fieldRepresentation == AccessFieldRepresentation::DOUBLE;
    }

    static bool HasSameStoreFieldLocation(const NamedStoreAccessInfo &left, const NamedStoreAccessInfo &right)
    {
        return left.fieldStorage == right.fieldStorage && left.fieldOffset == right.fieldOffset;
    }

    bool BuildCheckHClassSet(uint32_t bcIndex, ValueVertex *receiver,
                             const std::vector<JSHClass *> &expectedHClasses,
                             bool canAssumeStableHClasses)
    {
        return BuildCheckHClasses(
            bcIndex, receiver, expectedHClasses, false, canAssumeStableHClasses);
    }

    void RecordPossibleHClasses(ValueVertex *receiver, const std::vector<JSHClass *> &expectedHClasses)
    {
        compileInfoFacts_->RecordPossibleHClasses(receiver, expectedHClasses, false);
    }

    bool TryLowerEquivalentNamedStoreFields(uint32_t bcIndex, const NamedStoreAccessSet &access,
                                            ValueVertex *receiver, ValueVertex *value)
    {
        if (access.caseCount < 2 || !IsLocalTaggedStoreField(access.cases[0])) {
            return false;
        }

        std::vector<JSHClass *> expectedHClasses;
        expectedHClasses.reserve(access.caseCount);
        JSHClass *firstHClass = nullptr;
        if (!TryResolveHClassRef(access.cases[0].expectedHClass, &firstHClass)) {
            return false;
        }
        expectedHClasses.push_back(firstHClass);
        for (uint32_t i = 1; i < access.caseCount; ++i) {
            if (!IsLocalTaggedStoreField(access.cases[i]) ||
                !HasSameStoreFieldLocation(access.cases[0], access.cases[i])) {
                return false;
            }
            JSHClass *expectedHClass = nullptr;
            if (!TryResolveHClassRef(access.cases[i].expectedHClass, &expectedHClass)) {
                return false;
            }
            if (std::find(expectedHClasses.begin(), expectedHClasses.end(), expectedHClass) !=
                expectedHClasses.end()) {
                return false;
            }
            expectedHClasses.push_back(expectedHClass);
        }

        bool canAssumeStableHClasses = std::all_of(
            access.cases.begin(), access.cases.begin() + access.caseCount,
            [](const NamedStoreAccessInfo &storeCase) {
                return storeCase.dependencies.canAssumeStableHClass;
            });
        if (!BuildCheckHClassSet(bcIndex, receiver, expectedHClasses, canAssumeStableHClasses)) {
            return false;
        }
        ValueVertex *storeTarget = receiver;
        if (access.cases[0].fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY) {
            storeTarget = self->NewVertex<LoadTaggedFieldVertex>(
                compileInfoFacts_, currentBlock, {receiver}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        }
        BuildStoreTaggedField(storeTarget, access.cases[0].fieldOffset, value);
        return true;
    }

    bool TryLowerPolyNamedStoreFields(uint32_t bcIndex, const NamedStoreAccessSet &access,
                                      ValueVertex *receiver, ValueVertex *value)
    {
        if (access.caseCount < 2) {
            return false;
        }

        std::vector<JSHClass *> expectedHClasses;
        std::vector<StoreTaggedFieldByHClassCase> storeCases;
        expectedHClasses.reserve(access.caseCount);
        storeCases.reserve(access.caseCount);
        for (uint32_t i = 0; i < access.caseCount; ++i) {
            const NamedStoreAccessInfo &storeCaseInfo = access.cases[i];
            if (!IsLocalTaggedStoreField(storeCaseInfo)) {
                return false;
            }

            JSHClass *expectedHClass = nullptr;
            if (!TryResolveHClassRef(storeCaseInfo.expectedHClass, &expectedHClass)) {
                return false;
            }
            if (std::find(expectedHClasses.begin(), expectedHClasses.end(), expectedHClass) !=
                expectedHClasses.end()) {
                return false;
            }
            expectedHClasses.push_back(expectedHClass);
            storeCases.push_back(StoreTaggedFieldByHClassCase {
                expectedHClass,
                storeCaseInfo.fieldOffset,
                storeCaseInfo.fieldStorage == AccessFieldStorage::PROPERTIES_ARRAY,
            });
        }

        ChunkVector<ValueVertex *> storeInputs(self->chunk_);
        storeInputs.emplace_back(glue);
        storeInputs.emplace_back(receiver);
        storeInputs.emplace_back(value);
        auto *store = self->NewVertex<StoreTaggedFieldByHClassVertex>(
            compileInfoFacts_, currentBlock, storeInputs, self->chunk_, storeCases,
            ClassifyDirectWriteBarrierValueKind(value), self->preproc_->GetBytecodeOffset(bcIndex));
        store->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
        RecordPossibleHClasses(receiver, expectedHClasses);
        return true;
    }

    bool TryLowerMixedPolyNamedStores(uint32_t bcIndex, const NamedStoreAccessSet &access,
                                      ValueVertex *receiver, ValueVertex *value)
    {
        if (access.caseCount < 2) {
            return false;
        }

        std::vector<JSHClass *> expectedHClasses;
        expectedHClasses.reserve(access.caseCount);
        for (uint32_t i = 0; i < access.caseCount; ++i) {
            const NamedStoreAccessInfo &storeCase = access.cases[i];
            if (!IsLocalPolyNamedStoreCase(storeCase)) {
                return false;
            }

            JSHClass *expectedHClass = nullptr;
            if (!TryResolveHClassRef(storeCase.expectedHClass, &expectedHClass) ||
                std::find(expectedHClasses.begin(), expectedHClasses.end(), expectedHClass) !=
                    expectedHClasses.end()) {
                return false;
            }
            expectedHClasses.push_back(expectedHClass);

            if (storeCase.kind == AccessKind::TRANSITION) {
                JSHClass *transitionHClass = nullptr;
                if (expectedHClass->IsPrototype() ||
                    !TryResolveHClassRef(storeCase.transitionHClass, &transitionHClass)) {
                    return false;
                }
            }
        }

        CompileInfoFacts *entryFacts = compileInfoFacts_;
        std::vector<BB *> checkBlocks;
        std::vector<BB *> caseBlocks;
        checkBlocks.reserve(access.caseCount);
        caseBlocks.reserve(access.caseCount);
        for (uint32_t i = 0; i < access.caseCount; ++i) {
            checkBlocks.push_back(self->NewBlock());
            caseBlocks.push_back(self->NewBlock());
        }
        BB *primitiveDeoptBlock = self->NewBlock();
        BB *hclassMissDeoptBlock = self->NewBlock();
        BB *doneBlock = self->NewBlock();
        primitiveDeoptBlock->SetDeferred(true);
        hclassMissDeoptBlock->SetDeferred(true);

        self->FinishBlockWithBranch<BranchIfTaggedHeapObjectVertex>(
            currentBlock, {receiver}, checkBlocks.front(), primitiveDeoptBlock);

        ValueVertex *actualHClass = nullptr;
        // TODO(ArkSteed): Replace raw HClass addresses with heap-constant table support.
        for (uint32_t i = 0; i < access.caseCount; ++i) {
            currentBlock = checkBlocks[i];
            compileInfoFacts_ = entryFacts;
            if (actualHClass == nullptr) {
                actualHClass = self->NewVertex<LoadHClassAddressVertex>(
                    compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *> {receiver});
            }
            ValueVertex *expectedHClass = self->graph_->GetInt64Constant(
                reinterpret_cast<uint64_t>(expectedHClasses[i]) & TaggedStateWord::ADDRESS_MASK);
            BB *nextBlock = i + 1 < access.caseCount ? checkBlocks[i + 1] : hclassMissDeoptBlock;
            self->FinishBlockWithBranch<BranchIfInt64CompareVertex>(
                currentBlock, {actualHClass, expectedHClass}, caseBlocks[i], nextBlock, Condition::EQUAL);

            currentBlock = caseBlocks[i];
            compileInfoFacts_ = entryFacts->Clone();
            compileInfoFacts_->RecordHClass(receiver, expectedHClasses[i], false);
            bool lowered = TryLowerNamedStoreField(bcIndex, access.cases[i], receiver, value);
            ASSERT(lowered);
            if (!lowered) {
                UNREACHABLE();
            }
            self->FinishBlockWithJump(currentBlock, doneBlock);
        }

        auto buildDeoptBlock = [&](BB *deoptBlock) {
            currentBlock = deoptBlock;
            compileInfoFacts_ = entryFacts->Clone();
            auto *deopt = self->FinishBlockWith<DeoptVertex>(
                currentBlock, {}, self->chunk_, kungfu::DeoptType::KEYMISSMATCH,
                self->preproc_->GetBytecodeOffset(bcIndex));
            deopt->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
        };
        buildDeoptBlock(primitiveDeoptBlock);
        buildDeoptBlock(hclassMissDeoptBlock);

        currentBlock = doneBlock;
        compileInfoFacts_ = entryFacts;
        compileInfoFacts_->OnSideEffect();
        return true;
    }

    bool TryLowerNamedStoreAccessSet(uint32_t bcIndex, const NamedStoreAccessSet &access,
                                     ValueVertex *receiver, ValueVertex *value)
    {
        if (access.caseCount == 0) {
            return false;
        }
        if (access.caseCount == 1) {
            JSHClass *expectedHClass = nullptr;
            if (!TryResolveHClassRef(access.cases[0].expectedHClass, &expectedHClass) ||
                !BuildCheckHClass(bcIndex, receiver, expectedHClass, false,
                                  access.cases[0].dependencies.canAssumeStableHClass)) {
                return false;
            }
            return TryLowerNamedStoreField(bcIndex, access.cases[0], receiver, value);
        }

        JSHClass *knownHClass = TryGetKnownHClass(receiver);
        if (knownHClass != nullptr) {
            const NamedStoreAccessInfo *matched = nullptr;
            for (uint32_t i = 0; i < access.caseCount; ++i) {
                JSHClass *caseHClass = nullptr;
                if (!TryResolveHClassRef(access.cases[i].expectedHClass, &caseHClass)) {
                    return false;
                }
                if (caseHClass != knownHClass) {
                    continue;
                }
                if (matched != nullptr) {
                    return false;
                }
                matched = &access.cases[i];
            }
            return matched != nullptr && TryLowerNamedStoreField(bcIndex, *matched, receiver, value);
        }

        return TryLowerEquivalentNamedStoreFields(bcIndex, access, receiver, value) ||
            TryLowerPolyNamedStoreFields(bcIndex, access, receiver, value) ||
            TryLowerMixedPolyNamedStores(bcIndex, access, receiver, value);
    }

    ValueVertex *TryBuildPropertyLoad(uint32_t bcIndex, ValueVertex *object, const LoadedPropertyKey &key,
                                      const NamedLoadAccessInfo &accessInfo)
    {
        ValueVertex *cached = compileInfoFacts_->LookupLoadedProperty(key);
        if (cached == nullptr) {
            cached = compileInfoFacts_->LookupLoadedConstantProperty(key);
        }
        if (cached != nullptr) {
            return cached;
        }

        ValueVertex *loadSource = object;
        if (accessInfo.holderDepth != 0) {
            EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(bcIndex);
            loadSource = self->NewVertex<LoadPrototypeHolderByHClassVertex>(
                compileInfoFacts_, currentBlock, {object}, self->chunk_, accessInfo.holderHClass,
                accessInfo.expectedPrototypeHClasses, accessInfo.holderDepth,
                self->preproc_->GetBytecodeOffset(bcIndex));
            loadSource->Cast<LoadPrototypeHolderByHClassVertex>()->SetEagerDeoptFrameState(std::move(deoptFrameState));
        }
        ValueVertex *result = BuildLoadField(loadSource, accessInfo.plr);
        if (accessInfo.isConst) {
            compileInfoFacts_->RecordLoadedConstantProperty(key, result);
        } else {
            compileInfoFacts_->RecordLoadedProperty(key, result);
        }
        return result;
    }

    ValueVertex *BuildPolymorphicPropertyLoad(uint32_t bcIndex, ValueVertex *object,
                                               const NamedLoadAccessInfo &accessInfo)
    {
        // Internal case blocks share CompileInfoFacts. Do not let a load from one sibling case
        // enter the property cache or available-expression table and leak into another case.
        ValueVertex *loadSource = BuildPropertyLoadSource(bcIndex, object, accessInfo);
        return BuildLoadFieldWithoutCse(loadSource, accessInfo.plr);
    }

    bool TryBuildPolymorphicNamedAccess(uint32_t bcIndex, ValueVertex *receiver,
                                        const std::vector<NamedLoadAccessInfo> &accessInfos)
    {
        ASSERT(accessInfos.size() > 1);
        std::vector<JSHClass *> allExpectedHClasses;
        for (const NamedLoadAccessInfo &accessInfo : accessInfos) {
            if (accessInfo.lookupStartObjectHClasses.empty()) {
                return false;
            }
            for (JSHClass *hclass : accessInfo.lookupStartObjectHClasses) {
                if (hclass == nullptr || hclass->IsString() ||
                    std::find(allExpectedHClasses.begin(), allExpectedHClasses.end(), hclass) !=
                        allExpectedHClasses.end()) {
                    return false;
                }
                allExpectedHClasses.push_back(hclass);
            }
        }
        if (!BuildEagerCheckHClassesWithoutDependencies(bcIndex, receiver, allExpectedHClasses)) {
            return false;
        }

        BB *doneBlock = self->NewBlock();
        std::vector<ValueVertex *> results;
        results.reserve(accessInfos.size());
        for (size_t i = 0; i + 1 < accessInfos.size(); ++i) {
            BB *caseBlock = self->NewBlock();
            BB *nextCaseBlock = self->NewBlock();
            self->FinishBlockWithBranch<BranchIfHClassInVertex>(
                currentBlock, {receiver}, caseBlock, nextCaseBlock,
                accessInfos[i].lookupStartObjectHClasses);

            currentBlock = caseBlock;
            results.push_back(BuildPolymorphicPropertyLoad(bcIndex, receiver, accessInfos[i]));
            self->FinishBlockWithJump(currentBlock, doneBlock);
            currentBlock = nextCaseBlock;
        }

        // The eager union check above guarantees that a receiver reaching here belongs to the
        // final access-info group, so the last case needs no additional HClass branch.
        results.push_back(BuildPolymorphicPropertyLoad(bcIndex, receiver, accessInfos.back()));
        self->FinishBlockWithJump(currentBlock, doneBlock);
        currentBlock = doneBlock;
        frameState.SetAcc(self->NewPhiVertexWith(currentBlock, results, self->AccIndex()));
        return true;
    }

    bool TryBuildNamedAccess(uint32_t bcIndex, ValueVertex *receiver, uint16_t constDataId,
                             const std::vector<NamedLoadAccessInfo> &accessInfos, bool mapsAreKnownFresh)
    {
        if (accessInfos.empty()) {
            return false;
        }
        if (accessInfos.size() > 1) {
            return TryBuildPolymorphicNamedAccess(bcIndex, receiver, accessInfos);
        }
        const NamedLoadAccessInfo &accessInfo = accessInfos.front();
        const std::vector<JSHClass *> &maps = accessInfo.lookupStartObjectHClasses;
        bool hasHClassOfString = std::any_of(maps.begin(), maps.end(), [](JSHClass *hclass) {
            return hclass != nullptr && hclass->IsString();
        });
        if (hasHClassOfString ||
            !BuildCheckHClasses(bcIndex, receiver, maps, mapsAreKnownFresh,
                                accessInfo.canAssumeStableHClasses)) {
            return false;
        }
        LoadedPropertyKey propertyKey = LoadedPropertyKey::ConstDataId(receiver, constDataId, accessInfo.plr);
        ValueVertex *result = TryBuildPropertyLoad(bcIndex, receiver, propertyKey, accessInfo);
        if (result == nullptr) {
            return false;
        }
        frameState.SetAcc(result);
        return true;
    }

    bool TryBuildLoadNamedProperty(
        const BytecodeInfo *bcInfo, uint32_t bcIndex, ValueVertex *receiver, uint16_t constDataId)
    {
        auto factory = self->pgoContext_.CreateAccessInfoFactory(*bcInfo);
        PropertyAccessSet accessSet;
        if (!factory.TryBuildNamedLoadAccessInfo(0, &accessSet)) {
            return false;
        }

        NamedLoadAccessInfosOpt accessInfos = TryGetLoadObjByNameAccessInfos(accessSet, constDataId);
        if (!accessInfos.has_value()) {
            return false;
        }
        if (!TryBuildNamedAccess(bcIndex, receiver, constDataId, accessInfos.value(), false)) {
            return false;
        }
        return true;
    }

    ValueVertex *LoadValueFeedbackKey(uint32_t slotId)
    {
        ValueVertex *function = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *profileCell = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {function}, static_cast<int32_t>(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
        ValueVertex *profile = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {profileCell}, static_cast<int32_t>(ProfileTypeInfoCell::VALUE_OFFSET));
        int32_t slotOffset = static_cast<int32_t>(
            ProfileTypeInfo::DATA_OFFSET + slotId * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<LoadTaggedFieldVertex>(currentBlock, {profile}, slotOffset);
    }

    void BuildCheckValueKey(uint32_t bcIndex, ValueVertex *key, uint32_t slotId)
    {
        ValueVertex *cachedKey = LoadValueFeedbackKey(slotId);
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(bcIndex);
        auto *check = self->NewVertex<DeoptIfTaggedConditionVertex>(
            currentBlock, {key, cachedKey}, self->chunk_, self->preproc_->GetBytecodeOffset(bcIndex),
            Condition::NOT_EQUAL, kungfu::DeoptType::KEYMISSMATCH);
        check->SetEagerDeoptFrameState(std::move(deoptFrameState));
    }

    bool TryBuildNamedAccessByValue(uint32_t bcIndex, ValueVertex *receiver, JSTaggedValue propertyKey,
                                    const std::vector<NamedLoadAccessInfo> &accessInfos)
    {
        if (accessInfos.empty()) {
            return false;
        }
        if (accessInfos.size() > 1) {
            return TryBuildPolymorphicNamedAccess(bcIndex, receiver, accessInfos);
        }

        const NamedLoadAccessInfo &accessInfo = accessInfos.front();
        const std::vector<JSHClass *> &hclasses = accessInfo.lookupStartObjectHClasses;
        bool hasStringHClass = std::any_of(hclasses.begin(), hclasses.end(), [](JSHClass *hclass) {
            return hclass != nullptr && hclass->IsString();
        });
        if (hasStringHClass ||
            !BuildCheckHClasses(bcIndex, receiver, hclasses, false, accessInfo.canAssumeStableHClasses)) {
            return false;
        }

        LoadedPropertyKey key = LoadedPropertyKey::TaggedValue(receiver, propertyKey, accessInfo.plr);
        ValueVertex *result = TryBuildPropertyLoad(bcIndex, receiver, key, accessInfo);
        if (result == nullptr) {
            return false;
        }
        frameState.SetAcc(result);
        return true;
    }

    void BuildCheckTaggedCondition(uint32_t bcIndex, ValueVertex *left, ValueVertex *right,
                                   Condition condition, kungfu::DeoptType deoptType)
    {
        EagerDeoptFrameState deoptFrameState = BuildCurrentEagerDeoptFrameState(bcIndex);
        auto *check = self->NewVertex<DeoptIfTaggedConditionVertex>(
            currentBlock, {left, right}, self->chunk_, self->preproc_->GetBytecodeOffset(bcIndex),
            condition, deoptType);
        check->SetEagerDeoptFrameState(std::move(deoptFrameState));
    }

    bool TryBuildNormalElementLoad(uint32_t bcIndex, ValueVertex *receiver, ValueVertex *key,
                                   const ElementLoadAccessInfo &accessInfo)
    {
        if (accessInfo.kind != ElementLoadKind::NORMAL ||
            HandlerBase::NeedSkipInPGODump(accessInfo.handlerInfo)) {
            return false;
        }

        JSHClass *receiverHClass = nullptr;
        if (!TryResolveHClassRef(accessInfo.expectedHClass, &receiverHClass) || receiverHClass == nullptr ||
            (receiverHClass->GetObjectType() != JSType::JS_OBJECT && !receiverHClass->IsJSArray()) ||
            receiverHClass->IsDictionaryElement() ||
            HandlerBase::IsJSArray(accessInfo.handlerInfo) != receiverHClass->IsJSArray()) {
            return false;
        }
        if (self->preproc_->GetEnv()->GetJSOptions().IsEnableMutantArray()) {
            ElementsKind kind = receiverHClass->GetElementsKind();
            if (Elements::IsIntOrHoleInt(kind) || Elements::IsNumberOrHoleNumber(kind)) {
                return false;
            }
        }
        if (!BuildCheckHClass(bcIndex, receiver, receiverHClass)) {
            return false;
        }

        ValueVertex *index = BuildCheckedTaggedIntToI32(key);
        ValueVertex *zero = self->graph_->GetInt32Constant(0);
        BuildDeoptIfInt32Condition(
            index, zero, Condition::LESS_THAN, kungfu::DeoptType::INDEXLESSZERO);

        ValueVertex *elements = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {receiver}, static_cast<int32_t>(JSObject::ELEMENTS_OFFSET));
        ValueVertex *capacity = self->NewVertex<LoadInt32FieldVertex>(
            currentBlock, {elements}, static_cast<int32_t>(TaggedArray::LENGTH_OFFSET));
        BuildDeoptIfInt32Condition(
            index, capacity, Condition::GREATER_THAN_OR_EQUAL, kungfu::DeoptType::RANGE_ERROR);

        ValueVertex *result = self->NewVertex<LoadTaggedElementVertex>(currentBlock, {elements, index});
        ValueVertex *hole = self->graph_->GetTaggedConstant(JSTaggedValue::VALUE_HOLE);
        BuildCheckTaggedCondition(
            bcIndex, result, hole, Condition::EQUAL, kungfu::DeoptType::BUILTINISHOLE1);
        frameState.SetAcc(result);
        return true;
    }

    bool TryBuildStringElementLoad(uint32_t bcIndex, ValueVertex *receiver, ValueVertex *key,
                                   const ElementLoadAccessInfo &accessInfo)
    {
        if (accessInfo.kind != ElementLoadKind::STRING ||
            HandlerBase::NeedSkipInPGODump(accessInfo.handlerInfo)) {
            return false;
        }

        JSHClass *receiverHClass = nullptr;
        if (!TryResolveHClassRef(accessInfo.expectedHClass, &receiverHClass) || receiverHClass == nullptr ||
            !receiverHClass->IsString() || receiverHClass->GetObjectType() == JSType::TREE_STRING ||
            !BuildCheckHClass(bcIndex, receiver, receiverHClass)) {
            return false;
        }

        ValueVertex *index = BuildCheckedTaggedIntToI32(key);
        ValueVertex *zero = self->graph_->GetInt32Constant(0);
        BuildDeoptIfInt32Condition(
            index, zero, Condition::LESS_THAN, kungfu::DeoptType::INDEXLESSZERO);

        ValueVertex *lengthAndFlags = self->NewVertex<LoadInt32FieldVertex>(
            currentBlock, {receiver}, static_cast<int32_t>(BaseString::LENGTH_AND_FLAGS_OFFSET));
        ValueVertex *lengthShift =
            self->graph_->GetInt32Constant(static_cast<int32_t>(BaseString::LengthBits::START_BIT));
        ValueVertex *length = self->NewVertex<I32BitwiseBinaryVertex>(
            currentBlock, {lengthAndFlags, lengthShift}, IntBitwiseKind::SHIFT_RIGHT_LOGICAL);
        BuildDeoptIfInt32Condition(
            index, length, Condition::GREATER_THAN_OR_EQUAL, kungfu::DeoptType::RANGE_ERROR);

        ValueVertex *charCode = self->NewVertex<StringLoadElementVertex>(
            compileInfoFacts_, currentBlock, {glue, receiver, index, GlobalEnv()});
        CommonStubCallToAccWithLazyDeopt(
            {glue, charCode, GlobalEnv()}, CommonStubID::CreateStringBySingleCharCode);
        return true;
    }

    bool TryBuildTypedArrayElementLoad(uint32_t bcIndex, ValueVertex *receiver, ValueVertex *key,
                                       const ElementLoadAccessInfo &accessInfo)
    {
        if (accessInfo.kind != ElementLoadKind::TYPED_ARRAY ||
            HandlerBase::NeedSkipInPGODump(accessInfo.handlerInfo)) {
            return false;
        }

        JSHClass *receiverHClass = nullptr;
        if (!TryResolveHClassRef(accessInfo.expectedHClass, &receiverHClass) || receiverHClass == nullptr ||
            !receiverHClass->IsTypedArray()) {
            return false;
        }
        JSType objectType = receiverHClass->GetObjectType();
        if (objectType <= JSType::JS_TYPED_ARRAY_FIRST || objectType > JSType::JS_FLOAT64_ARRAY ||
            HandlerBase::IsOnHeap(accessInfo.handlerInfo) != receiverHClass->IsOnHeapFromBitField() ||
            !BuildCheckHClass(bcIndex, receiver, receiverHClass)) {
            return false;
        }

        ValueVertex *index = BuildCheckedTaggedIntToI32(key);
        ValueVertex *zero = self->graph_->GetInt32Constant(0);
        BuildDeoptIfInt32Condition(
            index, zero, IntConditionKind::LESS_THAN, kungfu::DeoptType::INDEXLESSZERO);

        ValueVertex *length = self->NewVertex<LoadInt32FieldVertex>(
            currentBlock, {receiver}, static_cast<int32_t>(JSTypedArray::ARRAY_LENGTH_OFFSET));
        BuildDeoptIfInt32Condition(
            index, length, IntConditionKind::GREATER_THAN_OR_EQUAL, kungfu::DeoptType::RANGE_ERROR);

        CommonStubCallToAccWithLazyDeopt(
            {glue, receiver, index, GlobalEnv()}, CommonStubID::GetPropertyByIndex);
        return true;
    }

    bool TryBuildElementLoad(uint32_t bcIndex, ValueVertex *receiver, ValueVertex *key,
                             const ValueLoadAccessSet &access)
    {
        if (access.kind != ValueLoadAccessKind::ELEMENT || access.feedback.isPoly || access.elementCount != 1) {
            return false;
        }
        const ElementLoadAccessInfo &accessInfo = access.elements[0];
        switch (accessInfo.kind) {
            case ElementLoadKind::NORMAL:
                return TryBuildNormalElementLoad(bcIndex, receiver, key, accessInfo);
            case ElementLoadKind::STRING:
                return TryBuildStringElementLoad(bcIndex, receiver, key, accessInfo);
            case ElementLoadKind::TYPED_ARRAY:
                return TryBuildTypedArrayElementLoad(bcIndex, receiver, key, accessInfo);
            default:
                return false;
        }
    }

    bool TryBuildLoadPropertyByValue(
        const BytecodeInfo *bcInfo, uint32_t bcIndex, ValueVertex *receiver, ValueVertex *key)
    {
        auto factory = self->pgoContext_.CreateAccessInfoFactory(*bcInfo);
        ValueLoadAccessSet access;
        if (!factory.TryBuildValueLoadAccessInfo(&access)) {
            return false;
        }
        if (access.kind == ValueLoadAccessKind::ELEMENT) {
            return TryBuildElementLoad(bcIndex, receiver, key, access);
        }
        if (access.kind != ValueLoadAccessKind::NAMED) {
            return false;
        }

        JSTaggedValue propertyKey = JSTaggedValue::Undefined();
        if (!TryResolveHeapRef(access.key, &propertyKey) ||
            (!propertyKey.IsString() && !propertyKey.IsSymbol())) {
            return false;
        }
        NamedLoadAccessInfosOpt accessInfos = TryGetLoadObjByNameAccessInfos(access.named, access.key);
        if (!accessInfos.has_value()) {
            return false;
        }
        bool hasStringHClass = std::any_of(accessInfos->begin(), accessInfos->end(), [](const auto &accessInfo) {
            return std::any_of(accessInfo.lookupStartObjectHClasses.begin(),
                               accessInfo.lookupStartObjectHClasses.end(), [](JSHClass *hclass) {
                return hclass != nullptr && hclass->IsString();
            });
        });
        if (hasStringHClass) {
            return false;
        }

        BuildCheckValueKey(bcIndex, key, access.feedback.slotId);
        return TryBuildNamedAccessByValue(bcIndex, receiver, propertyKey, accessInfos.value());
    }

    bool TryBuildStoreNamedProperty(uint32_t bcIndex, ValueVertex *receiver, uint16_t constDataId,
                                    ValueVertex *value)
    {
        JSHClass *hclass = compileInfoFacts_->TryGetHClass(receiver);
        if (hclass == nullptr) {
            return false;
        }
        std::optional<ArkSteedNameRef> nameRef = TryGetNameRefFromConstDataId(constDataId);
        if (!nameRef.has_value()) {
            return false;
        }
        std::optional<PropertyLookupResult> maybePlr = TryLookupPropertyInPGOHClass(hclass, nameRef.value());
        if (!maybePlr.has_value()) {
            return false;
        }
        PropertyLookupResult plr = maybePlr.value();
        if (!plr.IsFound() || !plr.IsLocal() || !plr.IsWritable() || plr.IsAccessor()) {
            return false;
        }
        auto *dependencies = self->preproc_->GetEnv()->GetDependencies();
        if (self->IsLazyDeoptEnabled() && kungfu::StableHClassDependency::IsValid(hclass) &&
            (dependencies == nullptr || !dependencies->DependOnStableHClass(hclass))) {
            return false;
        }
        bool hasNotPrototypeDependency = self->IsLazyDeoptEnabled() && dependencies != nullptr &&
            dependencies->DependOnNotPrototype(hclass);
        if (!hasNotPrototypeDependency) {
            ChunkVector<ValueVertex *> guardInputs(self->chunk_);
            guardInputs.emplace_back(receiver);
            auto *guard = self->NewVertex<DeoptIfPrototypeChangedVertex>(
                currentBlock, guardInputs, self->chunk_, false, true,
                self->preproc_->GetBytecodeOffset(bcIndex));
            guard->SetEagerDeoptFrameState(BuildCurrentEagerDeoptFrameState(bcIndex));
        }
        BuildStoreField(receiver, value, plr);
        return true;
    }

    template <class VertexT>
    void UpdateCatchBlockData(VertexT *vertex)
    {
        ThrowableMixin *mixin = vertex;
        if (reinterpret_cast<uintptr_t>(lazyCatchBlock) == NO_CATCH_BLOCK_TAG) {
            return;  // (1) vertex has no catch block
        }
        if (blockInfo->catchBlockState == CatchBlockProfileState::NEVER_EXECUTED && self->IsLazyDeoptEnabled()) {
            if constexpr (std::is_base_of_v<LazyDeoptimizableMixin, VertexT>) {
                return;  // The catch block is represented by lazy-deopt frame state instead of compiled code.
            }
        }
        if (UNLIKELY(lazyCatchBlock == nullptr)) {
            lazyCatchBlock = self->ActivateCatchBlock(&lazyCatchBlockInputs, blockInfo->catchBlock->rpoIndex);
        }
        ASSERT(lazyCatchBlockInputs != nullptr);
        uint32_t catchPredIndex = lazyCatchBlockInputs->AddCatchPredecessor(frameState, self->undefinedValue_);
        lazyCatchBlockInputs->AddCompileInfoFacts(*compileInfoFacts_);
        mixin->LoadCatchBlock(lazyCatchBlock, catchPredIndex);
    }

    CallCommonStubVertex *CommonStubCall(
        std::initializer_list<ValueVertex *> inputs, CommonStubID id,
        SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        ValidateCommonStubCallArgs(inputs, id);
        auto *vertex = self->NewVertex<CallCommonStubVertex>(
            compileInfoFacts_, currentBlock, inputs, id, sideEffectKind);
        UpdateCatchBlockData(vertex);
        return vertex;
    }

    CallCommonStubVertex *CommonStubCallWithLazyDeopt(
        std::initializer_list<ValueVertex *> inputs, CommonStubID id,
        SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        auto *vertex = CommonStubCall(inputs, id, sideEffectKind);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, vertex);
        return vertex;
    }

    CallCommonStubVertex *CommonStubCallToAccWithLazyDeopt(
        std::initializer_list<ValueVertex *> inputs, CommonStubID id,
        SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        auto *vertex = CommonStubCall(inputs, id, sideEffectKind);
        frameState.SetAcc(vertex);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, vertex);
        return vertex;
    }

    CallCommonStubVertex *CommonStubCallWithIC(
        const BytecodeInfo *bcInfo, std::initializer_list<ValueVertex *> inputs, CommonStubID id)
    {
        ChunkVector<ValueVertex *> allArgs(self->chunk_);
        allArgs.reserve(inputs.size() + CallVertex::FIRST_ARG_INDEX);

        allArgs.push_back(glue);
        allArgs.insert(allArgs.end(), inputs.begin(), inputs.end());
        allArgs.push_back(LoadParam(CALL_TARGET_PARAM_INDEX));
        allArgs.push_back(self->graph_->GetInt32Constant(GetICSlotId<int>(bcInfo, 0)));

        ValidateCommonStubCallArgs({allArgs.data(), allArgs.size()}, id);

        auto *vertex = self->NewVertex<CallCommonStubVertex>(compileInfoFacts_, currentBlock, allArgs, id);
        UpdateCatchBlockData(vertex);
        return vertex;
    }

    CallCommonStubVertex *CommonStubCallWithICAndLazyDeopt(
        const BytecodeInfo *bcInfo, std::initializer_list<ValueVertex *> inputs, CommonStubID id)
    {
        auto *vertex = CommonStubCallWithIC(bcInfo, inputs, id);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, vertex);
        return vertex;
    }

    CallCommonStubVertex *CommonStubCallToAccWithICAndLazyDeopt(
        const BytecodeInfo *bcInfo, std::initializer_list<ValueVertex *> inputs, CommonStubID id)
    {
        auto *vertex = CommonStubCallWithIC(bcInfo, inputs, id);
        frameState.SetAcc(vertex);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, vertex);
        return vertex;
    }

    template <class InputRange = std::initializer_list<ValueVertex *>>
    CallRuntimeVertex *RuntimeCall(const InputRange &inputs, RuntimeStubID id,
                                   SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        auto *vertex = self->NewVertex<CallRuntimeVertex>(
            compileInfoFacts_, currentBlock, inputs, id, sideEffectKind);
        UpdateCatchBlockData(vertex);
        return vertex;
    }

    template <class InputRange = std::initializer_list<ValueVertex *>>
    CallRuntimeVertex *RuntimeCallToAccWithLazyDeopt(const InputRange &inputs, RuntimeStubID id,
                                                     SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        auto *vertex = RuntimeCall(inputs, id, sideEffectKind);
        frameState.SetAcc(vertex);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, vertex);
        return vertex;
    }

    template <class InputRange = std::initializer_list<ValueVertex *>>
    CallRuntimeVertex *RuntimeCallWithLazyDeopt(const InputRange &inputs, RuntimeStubID id,
                                                SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        auto *vertex = RuntimeCall(inputs, id, sideEffectKind);
        LoadLazyDeoptFrameStateForThrowableCall(currentBcIndex, vertex);
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
        return self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {array}, offset);
    }

    void SetValueToTaggedArray(ValueVertex *array, uint32_t index, ValueVertex *value)
    {
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET + index * JSTaggedValue::TaggedTypeSize());
        BuildStoreTaggedField(array, offset, value);
    }

    ValueVertex *SharedConstPool()
    {
        int32_t methodOffset = static_cast<int32_t>(JSFunctionBase::METHOD_OFFSET);
        int32_t constpoolOffset = static_cast<int32_t>(Method::CONSTANT_POOL_OFFSET);

        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        ValueVertex *method = self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {jsFunc}, methodOffset);
        return self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {method}, constpoolOffset);
    }

    ValueVertex *ModuleFromFunction()
    {
        int32_t moduleOffset = static_cast<int32_t>(JSFunction::ECMA_MODULE_OFFSET);
        ValueVertex *jsFunc = LoadParam(CALL_TARGET_PARAM_INDEX);
        return self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {jsFunc}, moduleOffset);
    }

    bool TryGetConstPoolIndex(ValueVertex *indexVertex, uint32_t *index) const
    {
        ASSERT(index != nullptr);
        if (auto *constant = indexVertex->TryCast<Int32ConstantVertex>()) {
            if (constant->GetValue() < 0) {
                return false;
            }
            *index = static_cast<uint32_t>(constant->GetValue());
            return true;
        }
        if (auto *constant = indexVertex->TryCast<TaggedConstantVertex>()) {
            JSTaggedValue value(constant->GetValue());
            if (!value.IsInt() || value.GetInt() < 0) {
                return false;
            }
            *index = static_cast<uint32_t>(value.GetInt());
            return true;
        }
        return false;
    }

    ValueVertex *StringFromConstPool(ValueVertex *stringId)
    {
        uint32_t index = 0;
        ArkSteedHeapBroker *broker = self->pgoContext_.GetBroker();
        if (TryGetConstPoolIndex(stringId, &index) && index <= std::numeric_limits<uint16_t>::max() &&
            broker != nullptr) {
            ArkSteedHeapBroker::SerializingScope scope(broker, "GraphBuilder::StringFromConstPool");
            ArkSteedNameRef stringRef;
            if (broker->TryGetNameFromConstantPool(static_cast<uint16_t>(index), &stringRef)) {
                if (ValueVertex *constant = GetHeapConstant(stringRef); constant != nullptr) {
                    return constant;
                }
            }
        }
        ValueVertex *constpool = SharedConstPool();
        return CommonStubCall({glue, constpool, stringId}, CommonStubID::GetStringFromConstPool);
    }

    ValueVertex *ObjectFromConstPool(ValueVertex *index)
    {
        ValueVertex *constpool = SharedConstPool();
        ValueVertex *module = ModuleFromFunction();
        return CommonStubCall({glue, constpool, index, module}, CommonStubID::GetObjectFromConstPool);
    }

    ValueVertex *MethodFromConstPool(ValueVertex *index)
    {
        uint32_t methodIndex = 0;
        ArkSteedHeapBroker *broker = self->pgoContext_.GetBroker();
        if (TryGetConstPoolIndex(index, &methodIndex) && methodIndex <= std::numeric_limits<uint16_t>::max() &&
            broker != nullptr) {
            ArkSteedHeapBroker::SerializingScope scope(broker, "GraphBuilder::MethodFromConstPool");
            ArkSteedObjectRef methodRef;
            if (broker->TryGetMethodFromConstantPool(static_cast<uint16_t>(methodIndex), &methodRef)) {
                if (ValueVertex *constant = GetHeapConstant(methodRef); constant != nullptr) {
                    return constant;
                }
            }
        }
        ValueVertex *constpool = SharedConstPool();
        return RuntimeCall({constpool, index}, RTSTUB_ID(GetMethodFromCache));
    }

    GraphBuilder *self;
    ValueVertex *glue;           // Equivalent to self->glue_. Cached for performance.
    ValueVertex *lazyGlobalEnv;  // Equivalent to self->lazyGlobalEnv_. Cached for performance.
    const BasicBlockInfo *blockInfo;
    BB *currentBlock;    // Equivalent to self->blocks_[blockInfo->rpoIndex]. Cached for performance.
    BB *lazyCatchBlock;  // Equivalent to self->blocks_[blockInfo->catchBlock->rpoIndex]. Cached for performance.
    CompileInfoFacts *compileInfoFacts_;  // Equivalent to self->compileInfoFacts_[blockInfo->rpoIndex].
    SharedBCFrameState frameState;
    CatchBlockInputData *lazyCatchBlockInputs;
    const BytecodeInfo *currentBcInfo {nullptr};
    uint32_t currentBcIndex {0};
};

BB *GraphBuilder::VisitBytecodesOfBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex)
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
        .compileInfoFacts_ = compileInfoFacts_[rpoIndex],
        .frameState = frameState,
        .lazyCatchBlockInputs = caughtByData,
    };
    for (uint32_t bcIndex = blockInfo->startBcIndex; bcIndex <= blockInfo->endBcIndex; ++bcIndex) {
        if (!visitor.Visit(preproc_->GetBytecode(bcIndex), bcIndex)) {
            break;
        }
    }

    if (visitor.currentBlock->GetControlVertex() == nullptr) {
        ASSERT(blockInfo->IsFallthrough());
        BB *target = ActivateNonCatchBlock(blockInfo->fallthroughBlock->rpoIndex);
        FinishBlockWithJump(visitor.currentBlock, target);
    }
    // Lowering may create internal subgraphs, so callers need the actual exit block rather than blocks_[rpoIndex].
    return visitor.currentBlock;
}
}  // namespace panda::ecmascript::arksteed
