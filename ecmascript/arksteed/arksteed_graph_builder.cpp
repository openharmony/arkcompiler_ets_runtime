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

#include "ecmascript/arksteed/arksteed_compile_info_facts.h"
#include "ecmascript/arksteed/arksteed_constant_folding.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_register_merge_state.h"
#include "ecmascript/arksteed/arksteed_side_effect_classifier.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/lazy_deopt_dependency.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/ic/ic_info.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/program_object.h"
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
            return value->IsInt32() || value->IsUint32() || value->IsInt64();
        case kungfu::MachineType::F32:
        case kungfu::MachineType::F64:
            return value->IsAnyFloat64();
    }
    return true;
}

void ValidateCommonStubCallArgs(Span<ValueVertex *const> inputs, kungfu::CommonStubCSigns::ID id)
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

bool TryAppendHClassFromWeak(JSTaggedValue maybeWeak, std::vector<JSHClass *> &maps)
{
    if (!maybeWeak.IsWeak()) {
        return false;
    }
    TaggedObject *referent = maybeWeak.GetWeakReferent();
    if (referent == nullptr || !JSTaggedValue(referent).IsJSHClass()) {
        return false;
    }
    JSHClass *hclass = JSHClass::Cast(referent);
    if (std::find(maps.begin(), maps.end(), hclass) == maps.end()) {
        maps.push_back(hclass);
    }
    return true;
}

class KnownHClassesMerger {
public:
    explicit KnownHClassesMerger(const std::vector<JSHClass *> &feedbackMaps)
    {
        for (JSHClass *hclass : feedbackMaps) {
            AppendIfMissing(intersectSet_, hclass);
        }
    }

    void IntersectWithCompileInfoFacts(const std::optional<std::vector<JSHClass *>> &knownMaps)
    {
        if (!knownMaps.has_value()) {
            return;
        }
        std::vector<JSHClass *> intersection;
        for (JSHClass *hclass : intersectSet_) {
            if (std::find(knownMaps->begin(), knownMaps->end(), hclass) != knownMaps->end()) {
                AppendIfMissing(intersection, hclass);
            }
        }
        intersectSet_ = std::move(intersection);
    }

    const std::vector<JSHClass *> &GetIntersection() const
    {
        return intersectSet_;
    }

private:
    static void AppendIfMissing(std::vector<JSHClass *> &maps, JSHClass *hclass)
    {
        if (hclass != nullptr && std::find(maps.begin(), maps.end(), hclass) == maps.end()) {
            maps.push_back(hclass);
        }
    }

    std::vector<JSHClass *> intersectSet_;
};

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

bool IsEqualityCompare(CompareOpKind kind)
{
    return kind == CompareOpKind::EQUAL || kind == CompareOpKind::NOT_EQUAL ||
           kind == CompareOpKind::STRICT_EQUAL || kind == CompareOpKind::STRICT_NOT_EQUAL;
}

bool IsStrictEqualityCompare(CompareOpKind kind)
{
    return kind == CompareOpKind::STRICT_EQUAL || kind == CompareOpKind::STRICT_NOT_EQUAL;
}

bool IsEqualCompare(CompareOpKind kind)
{
    return kind == CompareOpKind::EQUAL || kind == CompareOpKind::STRICT_EQUAL;
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

bool EvaluateInt32Compare(CompareOpKind kind, int32_t left, int32_t right)
{
    switch (kind) {
        case CompareOpKind::EQUAL:
        case CompareOpKind::STRICT_EQUAL:
            return left == right;
        case CompareOpKind::NOT_EQUAL:
        case CompareOpKind::STRICT_NOT_EQUAL:
            return left != right;
        case CompareOpKind::LESS_THAN:
            return left < right;
        case CompareOpKind::LESS_THAN_OR_EQUAL:
            return left <= right;
        case CompareOpKind::GREATER_THAN:
            return left > right;
        case CompareOpKind::GREATER_THAN_OR_EQUAL:
            return left >= right;
    }
    UNREACHABLE();
}

bool EvaluateFloat64Compare(CompareOpKind kind, double left, double right)
{
    bool unordered = std::isnan(left) || std::isnan(right);
    switch (kind) {
        case CompareOpKind::EQUAL:
        case CompareOpKind::STRICT_EQUAL:
            return !unordered && left == right;
        case CompareOpKind::NOT_EQUAL:
        case CompareOpKind::STRICT_NOT_EQUAL:
            return unordered || left != right;
        case CompareOpKind::LESS_THAN:
            return !unordered && left < right;
        case CompareOpKind::LESS_THAN_OR_EQUAL:
            return !unordered && left <= right;
        case CompareOpKind::GREATER_THAN:
            return !unordered && left > right;
        case CompareOpKind::GREATER_THAN_OR_EQUAL:
            return !unordered && left >= right;
    }
    UNREACHABLE();
}

IntConditionKind Int32ConditionFromCompare(CompareOpKind kind)
{
    switch (kind) {
        case CompareOpKind::EQUAL:
        case CompareOpKind::STRICT_EQUAL:
            return IntConditionKind::EQUAL;
        case CompareOpKind::NOT_EQUAL:
        case CompareOpKind::STRICT_NOT_EQUAL:
            return IntConditionKind::NOT_EQUAL;
        case CompareOpKind::LESS_THAN:
            return IntConditionKind::LESS_THAN;
        case CompareOpKind::LESS_THAN_OR_EQUAL:
            return IntConditionKind::LESS_THAN_OR_EQUAL;
        case CompareOpKind::GREATER_THAN:
            return IntConditionKind::GREATER_THAN;
        case CompareOpKind::GREATER_THAN_OR_EQUAL:
            return IntConditionKind::GREATER_THAN_OR_EQUAL;
    }
    UNREACHABLE();
}

CompareOpKind InvertCompare(CompareOpKind kind)
{
    switch (kind) {
        case CompareOpKind::EQUAL:
            return CompareOpKind::NOT_EQUAL;
        case CompareOpKind::NOT_EQUAL:
            return CompareOpKind::EQUAL;
        case CompareOpKind::STRICT_EQUAL:
            return CompareOpKind::STRICT_NOT_EQUAL;
        case CompareOpKind::STRICT_NOT_EQUAL:
            return CompareOpKind::STRICT_EQUAL;
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
        constexpr VertexOpcode opcode = Vertex::opcode_of<VertexT>();
        constexpr VertexProperties props = VertexT::PROPERTIES;
        return !CseIsExcludedAvailableExpressionOpcode(opcode) && props.CanParticipateInCSE() &&
               !props.CanRead() && !props.IsAnyCall() && !props.CanAllocate() && !props.CanThrow();
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
    const int32_t CALL_TARGET_FP_SLOT_INDEX = 3;
    for (uint32_t i = 0, n = numParams_; i < n; i++) {
        int32_t slotIndex = static_cast<int32_t>(i + CALL_TARGET_FP_SLOT_INDEX);
        auto *v = NewVertex<InitialValueVertex>(blocks_[0], {}, slotIndex);
        graph_->AddParameter(v);
        frameState.Set(VRegOfParam(numLocal_, i), v);
    }
    // -3 : Fixed header lexicalEnv is at slot -3 in word units.
    initialLexicalEnv_ = NewVertex<InitialValueVertex>(blocks_[0], {}, -3);
    frameState.SetLexicalEnv(initialLexicalEnv_);

    if (GetOptions()->GetCompilerArkSteedPrintMethodName()) {
        ValueVertex *jsFunc = frameState.Get(VRegOfParam(numLocal_, CALL_TARGET_PARAM_INDEX));
        NewVertex<CallRuntimeVertex>(compileInfoFacts_[0], blocks_[0], {jsFunc}, RTSTUB_ID(PrintMethodName));
    }

    FinishBlockWithJump(blocks_[0], ActivateNonCatchBlock(1));
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
    FinishBlockWithJumpLoop(blocks_[rpoIndex], blocks_[headerRpoIndex]);

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
    } else {
        VisitBytecodesOfBasicBlock(frameState, rpoIndex);
    }
    if (bcBlock->IsEndOfLoop()) {
        WriteBackFrameStateToLoopHeader(frameState, rpoIndex);
    }
}

void GraphBuilder::ProcessCatchBlockHead(SharedBCFrameState frameState, uint32_t rpoIndex)
{
    blocks_[rpoIndex]->SetIsExceptionHandler(true);
    InitCompileInfoFactsForCatchBlock(rpoIndex);
    InitFrameStateForCatchBlockHeader(frameState, rpoIndex);

    const BasicBlockInfo *bcBlock = preproc_->GetBasicBlockByRPO(rpoIndex);
    // Catch block header is always synthetic. Only an unconditional jump.
    ASSERT(bcBlock->IsJump());
    FinishBlockWithJump(blocks_[rpoIndex], ActivateNonCatchBlock(bcBlock->jumpBlock->rpoIndex));
}

void GraphBuilder::InitFrameState(SharedBCFrameState frameState, uint32_t rpoIndex)
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

void GraphBuilder::InitFrameStateForLoopHeader(SharedBCFrameState frameState, uint32_t rpoIndex)
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

void GraphBuilder::InitFrameStateForCatchBlockHeader(SharedBCFrameState frameState, uint32_t rpoIndex)
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
        if (blocks_[predRpoIndex] == nullptr) {
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

PhiVertex *GraphBuilder::NewPhiVertex(BB *owner, uint32_t numPredecessors, VRegIDType vreg)
{
    PhiVertex *phi = PhiVertex::New(chunk_, numPredecessors, VirtualRegister(vreg));
    phi->SetOwner(owner);
    owner->AddPhiVertex(phi);
    REGISTER_VERTEX_TO_LABELLER(phi);
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
    REGISTER_VERTEX_TO_LABELLER(vertex);

    constexpr VertexProperties props = VertexT::PROPERTIES;
    // At most one of: deopt_checkpoint, eager_deopt, lazy_deopt
    static_assert(props.IsDeoptCheckpoint() + props.CanEagerDeopt() + props.CanLazyDeopt() <= 1);

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
        uint32_t hash = CseHashExpression(Vertex::opcode_of<VertexT>(), expressionInputs, options);
        bool needsEpochCheck = VertexT::PROPERTIES.CanRead();
        ValueVertex *cached = compileInfoFacts->FindExpression(
            hash, Vertex::opcode_of<VertexT>(), expressionInputs, options, needsEpochCheck);
        if (cached != nullptr) {
            return cached->Cast<VertexT>();
        }

        VertexT *vertex = Vertex::New<VertexT>(chunk_, inputs, std::forward<Args>(args)...);
        vertex->SetOwner(owner);
        owner->AddVertex(vertex);
        REGISTER_VERTEX_TO_LABELLER(vertex);
        compileInfoFacts->AddExpression(hash, vertex, expressionInputs, options, needsEpochCheck);
        return vertex;
    }
    VertexT *vertex = NewVertex<VertexT>(owner, inputs, std::forward<Args>(args)...);
    if constexpr (VertexT::PROPERTIES.CanWrite()) {
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
    REGISTER_VERTEX_TO_LABELLER(vertex);

    constexpr VertexProperties props = VertexT::PROPERTIES;
    // Control vertices cannot have lazy deopt, throw, or write side effects
    // Note: ThrowVertex is a special case that can throw
    static_assert(!props.CanLazyDeopt() && !props.CanWrite());

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
        catchBlockInputs_[rpoIndex] = chunk_->New<CatchBlockInputData>(analysis_->GetLiveIn(rpoIndex), chunk_);
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
        std::optional<JSTaggedValue> handler;
        PropertyLookupResult plr;
        bool isConst {false};
    };

    struct NamedAccessFeedback {
        std::vector<JSHClass *> maps;
        std::vector<JSTaggedValue> handlers;
    };

    using NamedLoadAccessInfoOpt = std::optional<NamedLoadAccessInfo>;
    using NamedLoadAccessInfosOpt = std::optional<std::vector<NamedLoadAccessInfo>>;

    void Visit(const BytecodeInfo *bcInfo, uint32_t bcIndex)
    {
        currentBcInfo = bcInfo;
        currentBcIndex = bcIndex;
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
        frameState.SetAcc(RuntimeCall({numberBigInt}, RTSTUB_ID(LdBigInt)));
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
        frameState.SetAcc(BuildUnaryOperation(CommonStubID::Inc));
    }

    void LowerDec()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::DEC, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        frameState.SetAcc(BuildUnaryOperation(CommonStubID::Dec));
    }

    void LowerNeg()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::NEG, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        frameState.SetAcc(BuildUnaryOperation(CommonStubID::Neg));
    }

    void LowerNot()
    {
        ValueVertex *value = frameState.GetAcc();
        JSTaggedValue folded;
        if (TryFoldUnaryConstant(value, UnaryFoldOp::NOT, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        frameState.SetAcc(BuildUnaryOperation(CommonStubID::Not));
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
        ValueVertex *xIsTrue = CommonStubCall({glue, x}, CommonStubID::ToBooleanTrue);
        // Submit currentBlock to the graph.
        // Note: Blocks should be submitted to the graph by RPO order.
        self->FinishBlockWithBranch(currentBlock, xIsTrue, trueBranch, falseBranch);

        // Bind currentBlock to trueBranch
        currentBlock = trueBranch;
        // Add result1 to currentBlock (which is trueBranch)
        ValueVertex *result1 = CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubID::Add);
        // Submit currentBlock (which is trueBranch) to the graph.
        self->FinishBlockWithJump(currentBlock, doneBlock);

        // Bind currentBlock to falseBranch
        currentBlock = falseBranch;
        // Add result2 to currentBlock (which is falseBranch)
        ValueVertex *result2 = CommonStubCall({glue, x, y, GlobalEnv()}, CommonStubID::Add);
        // Submit currentBlock (which is falseBranch) to the graph.
        self->FinishBlockWithJump(currentBlock, doneBlock);

        // Bind currentBlock to doneBlock
        currentBlock = doneBlock;
        // Add Phi to currentBlock (which is doneBlock)
        frameState.SetAcc(self->NewPhiVertexWith(currentBlock, {result1, result2}, self->AccIndex()));

        // ... Continue processing the subsequent bytecodes with doneBlock
    }

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
        frameState.SetAcc(BuildBinaryOperation(BinaryOpKind::ADD));
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
        frameState.SetAcc(BuildBinaryOperation(BinaryOpKind::SUB));
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
        frameState.SetAcc(BuildBinaryOperation(BinaryOpKind::MUL));
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
        frameState.SetAcc(BuildBinaryOperation(BinaryOpKind::DIV));
    }

    void LowerMod2(const BytecodeInfo * /*bcInfo*/)
    {
        frameState.SetAcc(BuildBinaryOperation(BinaryOpKind::MOD));
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
        JSTaggedValue folded;
        if (TryFoldBinaryConstant(x, y, BinaryFoldOp::SHL, &folded)) {
            frameState.SetAcc(TaggedConstantFromFoldedValue(folded));
            return;
        }
        frameState.SetAcc(BuildBitwiseOperation(IntBitwiseKind::SHIFT_LEFT));
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
        frameState.SetAcc(BuildBitwiseOperation(IntBitwiseKind::SHIFT_RIGHT_LOGICAL));
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
        frameState.SetAcc(BuildBitwiseOperation(IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC));
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
        frameState.SetAcc(BuildBitwiseOperation(IntBitwiseKind::BITWISE_AND));
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
        frameState.SetAcc(BuildBitwiseOperation(IntBitwiseKind::BITWISE_OR));
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
        frameState.SetAcc(BuildBitwiseOperation(IntBitwiseKind::BITWISE_XOR));
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
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::EQUAL));
    }

    void LowerNotEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::NOT_EQ)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::NOT_EQUAL));
    }

    void LowerLess(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::LESS)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::LESS_THAN));
    }

    void LowerLessEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::LESS_EQ)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::LESS_THAN_OR_EQUAL));
    }

    void LowerGreater(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::GREATER)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::GREATER_THAN));
    }

    void LowerGreaterEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::GREATER_EQ)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::GREATER_THAN_OR_EQUAL));
    }

    void LowerStrictNotEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::STRICT_NOT_EQ)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::STRICT_NOT_EQUAL));
    }

    void LowerStrictEq(const BytecodeInfo *bcInfo)
    {
        if (TryFoldCompareAtBytecode(bcInfo, BinaryFoldOp::STRICT_EQ)) {
            return;
        }
        frameState.SetAcc(BuildCompareOperation(CompareOpKind::STRICT_EQUAL));
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
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(ToNumber)));
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
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(ToNumeric)));
    }

    void LowerToPropertyKey()
    {
        ValueVertex *value = frameState.GetAcc();
        frameState.SetAcc(RuntimeCall({value}, RTSTUB_ID(ToPropertyKey)));
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
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, id, GlobalEnv()}, CommonStubID::GetPropertyByName));
    }

    void LowerStObjByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 2);  // 2: receiver register index
        uint16_t constDataId = GetConstDataId(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        if (TryBuildStoreNamedProperty(bcInfo, receiver, constDataId, value)) {
            return;
        }
        ValueVertex *id = self->graph_->GetIntPtrConstant(static_cast<intptr_t>(constDataId));
        CommonStubCallWithIC(bcInfo, {receiver, id, value, GlobalEnv()}, CommonStubID::SetPropertyByName);
    }

    void LowerLdObjByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *key = frameState.GetAcc();
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, key, GlobalEnv()}, CommonStubID::GetPropertyByValue));
    }

    void LowerStObjByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *key = LoadRegister(bcInfo, 2);  // 2: key register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, key, value, GlobalEnv()}, CommonStubID::SetPropertyByValue);
    }

    void LowerLdObjByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 0));
        frameState.SetAcc(CommonStubCall({glue, receiver, index, GlobalEnv()}, CommonStubID::LdObjByIndex));
    }

    void LowerStObjByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, receiver, index, value, GlobalEnv()}, CommonStubID::StObjByIndex);
    }

    void LowerLdThisByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = frameState.GetAcc();
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, key, GlobalEnv()}, CommonStubID::GetPropertyByValue));
    }

    void LowerStThisByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *key = LoadRegister(bcInfo, 1);
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, key, value, GlobalEnv()}, CommonStubID::SetPropertyByValue);
    }

    void LowerLdThisByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(
            bcInfo, {receiver, id, GlobalEnv()}, CommonStubID::GetPropertyByName));
    }

    void LowerStThisByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadParam(THIS_OBJECT_PARAM_INDEX);
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {receiver, id, value, GlobalEnv()}, CommonStubID::SetPropertyByName);
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
        CommonStubCall({glue, receiver, key, value, GlobalEnv()}, CommonStubID::StOwnByValue);
    }

    void LowerStOwnByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *index = self->graph_->GetInt32Constant(GetImmediate<int>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, receiver, index, value, GlobalEnv()}, CommonStubID::StOwnByIndex);
    }

    void LowerStOwnByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubID::StOwnByName);
    }

    void LowerStOwnByValueWithNameSet(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 0);
        ValueVertex *propKey = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubID::StOwnByValueWithNameSet);
    }

    void LowerStOwnByNameWithNameSet(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *propKey = StringFromConstPool(stringId);
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *accValue = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, accValue, GlobalEnv()}, CommonStubID::StOwnByNameWithNameSet);
    }

    void LowerTryLdGlobalByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(bcInfo, {id, GlobalEnv()}, CommonStubID::TryLdGlobalByName));
    }

    void LowerTryStGlobalByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {id, value, GlobalEnv()}, CommonStubID::TryStGlobalByName);
    }

    void LowerLdGlobalVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        frameState.SetAcc(CommonStubCallWithIC(bcInfo, {id, GlobalEnv()}, CommonStubID::LdGlobalVar));
    }

    void LowerStGlobalVar(const BytecodeInfo *bcInfo)
    {
        ValueVertex *id = self->graph_->GetIntPtrConstant(GetConstDataId<intptr_t>(bcInfo, 1));
        ValueVertex *value = frameState.GetAcc();
        CommonStubCallWithIC(bcInfo, {id, value, GlobalEnv()}, CommonStubID::StGlobalVar);
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

        frameState.SetAcc(self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {GlobalEnv()}, offset));
    }

    void LowerLdSymbol()
    {
        constexpr int32_t offset = static_cast<int32_t>(
            GlobalEnv::HEADER_SIZE + GlobalEnv::SYMBOL_FUNCTION_INDEX * JSTaggedValue::TaggedTypeSize());

        frameState.SetAcc(self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {GlobalEnv()}, offset));
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
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, undefined}, CALL_ARG0);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallArg1(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, undefined, a0Value}, CALL_ARG1);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallArgs2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *a1Value = LoadRegister(bcInfo, 1);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, undefined, a0Value, a1Value}, CALL_ARG2);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallArgs3(const BytecodeInfo *bcInfo)
    {
        ValueVertex *a0Value = LoadRegister(bcInfo, 0);
        ValueVertex *a1Value = LoadRegister(bcInfo, 1);
        ValueVertex *a2Value = LoadRegister(bcInfo, 2);  // 2: third argument register index
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, undefined, a0Value, a1Value, a2Value}, CALL_ARG3);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallThis0(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, thisObj}, CALL_ARG0);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallThis1(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, thisObj, a0Value}, CALL_ARG1);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallThis2(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *a1Value = LoadRegister(bcInfo, 2);  // 2: second argument register index
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, thisObj, a0Value, a1Value}, CALL_ARG2);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
    }

    void LowerCallThis3(const BytecodeInfo *bcInfo)
    {
        ValueVertex *thisObj = LoadRegister(bcInfo, 0);
        ValueVertex *a0Value = LoadRegister(bcInfo, 1);
        ValueVertex *a1Value = LoadRegister(bcInfo, 2);  // 2: second argument register index
        ValueVertex *a2Value = LoadRegister(bcInfo, 3);  // 3: third argument register index
        ValueVertex *func = frameState.GetAcc();
        ValueVertex *undefined = self->undefinedValue_;

        CallVertex *call = self->NewVertex<CallVertex>(
            compileInfoFacts_, currentBlock, {func, undefined, thisObj, a0Value, a1Value, a2Value}, CALL_ARG3);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
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
        CallVertex *call = self->NewVertex<CallVertex>(compileInfoFacts_, currentBlock, args, inputSize);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
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
        CallVertex *call = self->NewVertex<CallVertex>(compileInfoFacts_, currentBlock, args, argc);
        UpdateCatchBlockData(call);
        frameState.SetAcc(call);
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

        ValueVertex *argsArray = CommonStubCall({glue, array, GlobalEnv()}, CommonStubID::GetCallSpreadArgs);
        frameState.SetAcc(RuntimeCall({func, newTarget, argsArray}, RTSTUB_ID(OptSuperCallSpread)));
    }

    void LowerSuperCallForwardAllArgs(const BytecodeInfo *bcInfo)
    {
        ValueVertex *func = LoadRegister(bcInfo, 0);
        ValueVertex *superFunc = CommonStubCall({glue, func}, CommonStubID::GetPrototype);
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
        frameState.SetAcc(CommonStubCall({glue, GlobalEnv()}, CommonStubID::CreateEmptyArray));
    }

    void LowerCreateObjectWithBuffer(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 0));
        ValueVertex *obj = ObjectFromConstPool(index);
        ValueVertex *lexEnv = LoadRegister(bcInfo, 1);
        frameState.SetAcc(CommonStubCall({glue, obj, lexEnv}, CommonStubID::CreateObjectHavingMethod));
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
            {glue, index, jsFunc, slotId, GlobalEnv()}, CommonStubID::CreateArrayWithBuffer));
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
            CommonStubID::Definefunc));
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
        CommonStubCall({glue, obj, prop, value, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerDefineFieldByName(const BytecodeInfo *bcInfo)
    {
        ValueVertex *stringId = self->graph_->GetInt32Constant(GetConstDataId<int>(bcInfo, 1));
        ValueVertex *prop = StringFromConstPool(stringId);
        ValueVertex *obj = LoadRegister(bcInfo, 2);  // 2: obj register index
        ValueVertex *value = frameState.GetAcc();
        CommonStubCall({glue, obj, prop, value, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerDefineFieldByValue(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *propKey = LoadRegister(bcInfo, 0);
        ValueVertex *acc = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, acc, GlobalEnv()}, CommonStubID::DefineField);
    }

    void LowerDefineFieldByIndex(const BytecodeInfo *bcInfo)
    {
        ValueVertex *receiver = LoadRegister(bcInfo, 1);
        ValueVertex *propKey = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *acc = frameState.GetAcc();
        CommonStubCall({glue, receiver, propKey, acc, GlobalEnv()}, CommonStubID::DefineField);
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
        frameState.SetAcc(CommonStubCall({glue, obj, GlobalEnv()}, CommonStubID::GetIterator));
    }

    void LowerGetPropIterator()
    {
        ValueVertex *object = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, object, GlobalEnv()}, CommonStubID::Getpropiterator));
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
        frameState.SetAcc(CommonStubCall({glue, obj}, CommonStubID::TypeOf));
    }

    void LowerGetUnmappedArgs()
    {
        ValueVertex *argv = self->graph_->GetIntPtrConstant(0);
        ValueVertex *numArgs = ActualArgc();
        ValueVertex *argvTaggedArray = self->undefinedValue_;

        frameState.SetAcc(CommonStubCall(
            {glue, argv, numArgs, argvTaggedArray, GlobalEnv()}, CommonStubID::GetUnmappedArgs));
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
            {glue, object, prop, GlobalEnv()}, CommonStubID::DeleteObjectProperty));
    }

    void LowerIsIn(const BytecodeInfo *bcInfo)
    {
        ValueVertex *prop = LoadRegister(bcInfo, 0);
        ValueVertex *obj = frameState.GetAcc();
        frameState.SetAcc(CommonStubCall({glue, prop, obj, GlobalEnv()}, CommonStubID::IsIn));
    }

    void LowerInstanceOf(const BytecodeInfo *bcInfo)
    {
        ValueVertex *object = LoadRegister(bcInfo, 1);
        ValueVertex *target = frameState.GetAcc();
        frameState.SetAcc(CommonStubCallWithIC(bcInfo, {object, target, GlobalEnv()}, CommonStubID::Instanceof));
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
        ValueVertex *exception = frameState.GetAcc();
        auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {exception}, RTSTUB_ID(Throw));
        UpdateCatchBlockData(vertex);
    }

    void LowerThrowConstAssignment(const BytecodeInfo *bcInfo)
    {
        ValueVertex *value = LoadRegister(bcInfo, 0);
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {value}, RTSTUB_ID(ThrowConstAssignment));
        UpdateCatchBlockData(vertex);
    }

    void LowerThrowNotExists()
    {
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {}, RTSTUB_ID(ThrowThrowNotExists));
        UpdateCatchBlockData(vertex);
    }

    void LowerThrowPatternNonCoercible()
    {
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {}, RTSTUB_ID(ThrowPatternNonCoercible));
        UpdateCatchBlockData(vertex);
    }

    void LowerThrowDeleteSuperProperty()
    {
        auto *vertex = self->FinishBlockWith<ThrowVertex>(
            currentBlock, {}, RTSTUB_ID(ThrowDeleteSuperProperty));
        UpdateCatchBlockData(vertex);
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
            checkLowerDoneBlock, checkLowerFailedBlock, IntConditionKind::GREATER_THAN_OR_EQUAL);

        currentBlock = checkLowerDoneBlock;
        ValueVertex *lastType = self->graph_->GetInt64Constant(static_cast<int64_t>(JSType::ECMA_OBJECT_LAST));
        self->FinishBlockWithBranch<BranchIfInt64CompareVertex>(
            currentBlock, {typeBits, lastType},
            checkUpperDoneBlock, checkUpperFailedBlock, IntConditionKind::LESS_THAN_OR_EQUAL);

        for (BB *exceptionBlock : {notHeapObjectBlock, checkLowerFailedBlock, checkUpperFailedBlock}) {
            currentBlock = exceptionBlock;
            currentBlock->SetDeferred(true);
            auto *vertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {}, RTSTUB_ID(ThrowIfNotObject));
            UpdateCatchBlockData(vertex);
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
        auto *throwVertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {obj}, RTSTUB_ID(ThrowUndefinedIfHole));
        UpdateCatchBlockData(throwVertex);

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
        auto *throwVertex = self->FinishBlockWith<ThrowVertex>(currentBlock, {str}, RTSTUB_ID(ThrowUndefinedIfHole));
        UpdateCatchBlockData(throwVertex);

        currentBlock = doneBlock;
    }

    void LowerThrowIfSuperNotCorrectCall(const BytecodeInfo *bcInfo)
    {
        ValueVertex *index = TaggedConstantFromInt32(GetImmediate<int>(bcInfo, 0));
        ValueVertex *thisValue = frameState.GetAcc();

        RuntimeCall({index, thisValue}, RTSTUB_ID(ThrowIfSuperNotCorrectCall));
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
        return self->NewPhiVertexWith(currentBlock, std::initializer_list<ValueVertex *> {trueResult, falseResult},
                                      resultVreg);
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
        return self->NewVertex<ActualArgcVertex>(compileInfoFacts_, currentBlock, {});
    }

    ValueVertex *TaggedActualArgc()
    {
        ValueVertex *argc = ActualArgc();
        return self->NewVertex<I32ToTaggedIntVertex>(compileInfoFacts_, currentBlock, {argc});
    }

    std::optional<JSTaggedValue> TryGetConstantHeapObject(ValueVertex *node) const
    {
        if (node == nullptr || !node->IsTagged()) {
            return std::nullopt;
        }

        JSTaggedValue value;
        if (auto *constant = node->TryCast<TaggedConstantVertex>()) {
            value = JSTaggedValue(constant->GetValue());
        } else {
            return std::nullopt;
        }
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

    std::optional<std::vector<JSHClass *>> TryGetPossibleHClasses(ValueVertex *node) const
    {
        if (std::optional<JSTaggedValue> constant = TryGetConstantHeapObject(node)) {
            if (constant->IsHole() || !constant->IsHeapObject()) {
                return std::nullopt;
            }
            return std::vector<JSHClass *> {constant->GetTaggedObject()->GetClass()};
        }
        return compileInfoFacts_->TryGetPossibleHClasses(node);
    }

    std::optional<NamedAccessFeedback> TryGetLoadObjByNameFeedback(kungfu::ICSlotIdType slotId) const
    {
        ALLOW_DEREF_HANDLE;
        JSHandle<ProfileTypeInfo> profileTypeInfo = self->preproc_->GetEnv()->GetProfileTypeInfo();
        if (profileTypeInfo.GetAddress() == 0 || !profileTypeInfo.GetTaggedValue().IsTaggedArray()) {
            return std::nullopt;
        }

        auto *profile = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedValue().GetTaggedObject());
        uint32_t index = static_cast<uint32_t>(slotId);
        if (index + 1 >= profile->GetIcSlotLength()) {
            return std::nullopt;
        }
        IcAccessor accessor(self->compilerThread_, profileTypeInfo, index, ICKind::NamedLoadIC);
        IcAccessor::ICState state = accessor.GetICState();
        if (state == IcAccessor::ICState::UNINIT) {
            return std::nullopt;
        }

        NamedAccessFeedback feedback;
        if (state == IcAccessor::ICState::MEGA || state == IcAccessor::ICState::IC_MEGA) {
            return feedback;
        }

        JSTaggedValue first = profile->GetIcSlot(self->compilerThread_, index);
        if (state == IcAccessor::ICState::MONO) {
            if (!TryAppendHClassFromWeak(first, feedback.maps)) {
                return std::nullopt;
            }
            feedback.handlers.push_back(profile->GetIcSlot(self->compilerThread_, index + 1));
            return feedback;
        }
        if (state != IcAccessor::ICState::POLY || !first.IsTaggedArray()) {
            return std::nullopt;
        }

        TaggedArray *mapsAndHandlers = TaggedArray::Cast(first.GetTaggedObject());
        constexpr uint32_t entrySize = 2;
        for (uint32_t i = 0; i + 1 < mapsAndHandlers->GetLength(); i += entrySize) {
            JSTaggedValue maybeWeak = mapsAndHandlers->Get(self->compilerThread_, i);
            if (maybeWeak.IsUndefined()) {
                continue;
            }
            if (!TryAppendHClassFromWeak(maybeWeak, feedback.maps)) {
                return std::nullopt;
            }
            feedback.handlers.push_back(mapsAndHandlers->Get(self->compilerThread_, i + 1));
        }
        return feedback.maps.empty() ? std::nullopt : std::optional<NamedAccessFeedback>(std::move(feedback));
    }

    std::optional<JSTaggedValue> TryFindFeedbackHandler(const NamedAccessFeedback &feedback, JSHClass *map) const
    {
        auto it = std::find(feedback.maps.begin(), feedback.maps.end(), map);
        if (it == feedback.maps.end()) {
            return std::nullopt;
        }
        size_t index = static_cast<size_t>(std::distance(feedback.maps.begin(), it));
        return index < feedback.handlers.size() ? std::optional<JSTaggedValue>(feedback.handlers[index])
                                                : std::nullopt;
    }

    static bool IsSupportedMonoNamedLoad(const NamedLoadAccessInfo &accessInfo)
    {
        PropertyLookupResult plr = accessInfo.plr;
        return accessInfo.receiverHClass != nullptr && !accessInfo.lookupStartObjectHClasses.empty() &&
               accessInfo.holderHClass == accessInfo.receiverHClass && plr.IsFound() && plr.IsLocal() &&
               plr.IsNotHole() && !plr.IsAccessor() && !plr.IsFunction() && !plr.IsLoadFromIterResult();
    }

    NamedLoadAccessInfoOpt TryGetPropertyAccessInfo(JSHClass *map, uint16_t constDataId,
                                                     std::optional<JSTaggedValue> handler) const
    {
        if (map == nullptr) {
            return std::nullopt;
        }
        std::optional<JSTaggedValue> name = TryGetNameFromConstDataId(constDataId);
        if (!name.has_value() || !map->GetLayout(self->compilerThread_).IsTaggedArray()) {
            return std::nullopt;
        }

        PropertyLookupResult plr =
            JSHClass::LookupPropertyInPGOHClass(self->compilerThread_, map, name.value());
        NamedLoadAccessInfo accessInfo {
            .receiverHClass = map,
            .holderHClass = map,
            .lookupStartObjectHClasses = {map},
            .handler = handler,
            .plr = plr,
            .isConst = plr.IsFound() && !plr.IsWritable() && kungfu::StableHClassDependency::IsValid(map),
        };
        return IsSupportedMonoNamedLoad(accessInfo) ? NamedLoadAccessInfoOpt(std::move(accessInfo)) : std::nullopt;
    }

    bool RecordNamedAccessInfoDependencies(const NamedLoadAccessInfo &accessInfo) const
    {
        if (!accessInfo.isConst) {
            return true;
        }
        auto *dependencies = self->preproc_->GetEnv()->GetDependencies();
        if (dependencies == nullptr) {
            return false;
        }
        for (JSHClass *hclass : accessInfo.lookupStartObjectHClasses) {
            if (hclass != nullptr && kungfu::StableHClassDependency::IsValid(hclass) &&
                !dependencies->DependOnStableHClass(hclass)) {
                return false;
            }
        }
        return true;
    }

    static bool HasSameLoadFieldAccess(const NamedLoadAccessInfo &lhs, const NamedLoadAccessInfo &rhs)
    {
        return lhs.plr.GetData() == rhs.plr.GetData() && lhs.isConst == rhs.isConst;
    }

    static void AppendHClassIfMissing(std::vector<JSHClass *> *hclasses, JSHClass *hclass)
    {
        if (hclasses != nullptr && hclass != nullptr &&
            std::find(hclasses->begin(), hclasses->end(), hclass) == hclasses->end()) {
            hclasses->push_back(hclass);
        }
    }

    NamedLoadAccessInfosOpt TryGetLoadObjByNameAccessInfos(
        const std::vector<JSHClass *> &maps, const NamedAccessFeedback &feedback, uint16_t constDataId) const
    {
        std::vector<NamedLoadAccessInfo> result;
        for (JSHClass *map : maps) {
            NamedLoadAccessInfoOpt accessInfo =
                TryGetPropertyAccessInfo(map, constDataId, TryFindFeedbackHandler(feedback, map));
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
        for (const NamedLoadAccessInfo &accessInfo : result) {
            if (!RecordNamedAccessInfoDependencies(accessInfo)) {
                return std::nullopt;
            }
        }
        return result;
    }

    void BuildCurrentFrameStateForDeopt(
        uint32_t bcIndex, std::vector<ValueVertex *> *inputs, ChunkVector<VRegIDType> *vregIds)
    {
        auto normalizeFrameValue = [this](ValueVertex *value) -> ValueVertex * {
            switch (value->GetValueRepresentation()) {
                case ValueRepresentation::TAGGED:
                    return value;
                case ValueRepresentation::INT32:
                    return self->NewVertex<I32ToTaggedIntVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{value});
                case ValueRepresentation::FLOAT64:
                case ValueRepresentation::HOLEY_FLOAT64:
                    return self->NewVertex<F64ToTaggedDoubleVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{value});
                case ValueRepresentation::UINT32:
                case ValueRepresentation::INT64:
                case ValueRepresentation::NONE:
                    break;
            }
            UNREACHABLE();
        };
        auto add = [&](int32_t id, ValueVertex *value) {
            vregIds->emplace_back(id);
            inputs->emplace_back(normalizeFrameValue(value == nullptr ? self->undefinedValue_ : value));
        };
        auto addRaw = [&](int32_t id, ValueVertex *value) {
            vregIds->emplace_back(id);
            inputs->emplace_back(value == nullptr ? self->undefinedValue_ : value);
        };

        add(static_cast<int32_t>(SpecVregIndex::FUNC_INDEX), LoadParam(CALL_TARGET_PARAM_INDEX));
        add(static_cast<int32_t>(SpecVregIndex::NEWTARGET_INDEX), LoadParam(NEW_TARGET_PARAM_INDEX));
        add(static_cast<int32_t>(SpecVregIndex::THIS_OBJECT_INDEX), LoadParam(THIS_OBJECT_PARAM_INDEX));
        add(static_cast<int32_t>(SpecVregIndex::ENV_INDEX), frameState.GetLexicalEnv());
        add(static_cast<int32_t>(SpecVregIndex::ACC_INDEX), frameState.GetAcc());
        add(static_cast<int32_t>(SpecVregIndex::ACTUAL_ARGC_INDEX), TaggedActualArgc());
        addRaw(static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX),
               self->graph_->GetInt32Constant(static_cast<int32_t>(self->preproc_->GetBytecodeOffset(bcIndex))));

        for (VRegIDType index = 0; index < self->numLocal_; index++) {
            add(static_cast<int32_t>(VRegOfLocal(index)), frameState.Get(VRegOfLocal(index)));
        }
        for (VRegIDType index = 0; index < self->numParams_; index++) {
            add(static_cast<int32_t>(VRegOfParam(self->numLocal_, index)), LoadParam(index));
        }
    }

    uint32_t AppendCurrentFrameStateForDeopt(std::vector<ValueVertex *> *inputs, ChunkVector<VRegIDType> *vregIds)
    {
        ASSERT(inputs != nullptr);
        uint32_t firstDeoptInputIndex = static_cast<uint32_t>(inputs->size());
        BuildCurrentFrameStateForDeopt(currentBcIndex, inputs, vregIds);
        return firstDeoptInputIndex;
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
        ValueVertex *i32 = self->NewVertex<TaggedIntToI32Vertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{value});
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
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        ValueVertex *i32 = self->NewVertex<CheckedTaggedIntToI32Vertex>(
            currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
            self->preproc_->GetBytecodeOffset(currentBcIndex));
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
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        ValueVertex *checked = self->NewVertex<CheckedTaggedStringVertex>(
            currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
            self->preproc_->GetBytecodeOffset(currentBcIndex));
        compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::STRING);
        compileInfoFacts_->EnsureType(checked, NodeInfo::NodeType::STRING);
        return checked;
    }

    void BuildDeoptIfNotNumber(ValueVertex *value)
    {
        std::vector<ValueVertex *> inputs {value};
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        self->NewVertex<DeoptIfNotNumberVertex>(currentBlock, inputs, std::move(deoptVRegs),
                                                self->preproc_->GetBytecodeOffset(currentBcIndex));
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
        ValueVertex *taggedResult =
            self->NewVertex<I32ToTaggedIntVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{rawResult});
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
        switch (kind) {
            case BinaryOpKind::ADD:
                return self->NewVertex<I32AddVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32});
            case BinaryOpKind::SUB:
                return self->NewVertex<I32SubVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32});
            case BinaryOpKind::MUL:
                return self->NewVertex<I32MulVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32});
            case BinaryOpKind::DIV:
                return self->NewVertex<I32DivVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32});
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
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        switch (kind) {
            case BinaryOpKind::ADD:
                return self->NewVertex<I32AddWithOverflowVertex>(
                    currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
            case BinaryOpKind::SUB:
                return self->NewVertex<I32SubWithOverflowVertex>(
                    currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
            case BinaryOpKind::MUL:
                return self->NewVertex<I32MulWithOverflowVertex>(
                    currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
            case BinaryOpKind::DIV:
                return self->NewVertex<I32DivWithOverflowVertex>(
                    currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
            case BinaryOpKind::MOD:
                return self->NewVertex<CheckedI32ModVertex>(
                    currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
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
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        ValueVertex *rawResult = self->NewVertex<I32DivByConstWithCheckVertex>(
            currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
            self->preproc_->GetBytecodeOffset(currentBcIndex), divisor, magic.magic, magic.shift);
        return BuildTaggedI32Result(rawResult);
    }

    void BuildDeoptIfInt32Condition(ValueVertex *leftI32, ValueVertex *rightI32, IntConditionKind condition,
                                    kungfu::DeoptType deoptType)
    {
        std::vector<ValueVertex *> inputs {leftI32, rightI32};
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        self->NewVertex<DeoptIfInt32ConditionVertex>(
            currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
            self->preproc_->GetBytecodeOffset(currentBcIndex), condition, deoptType);
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
        BuildDeoptIfInt32Condition(valueI32, self->graph_->GetInt32Constant(0), IntConditionKind::LESS_THAN,
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
            BuildDeoptIfInt32Condition(valueI32, zeroI32, IntConditionKind::EQUAL, kungfu::DeoptType::DIVZERO2);
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
                                       IntConditionKind::EQUAL, kungfu::DeoptType::INT32OVERFLOW1);
        }
        BuildDeoptIfInt32Condition(leftI32, self->graph_->GetInt32Constant(0), IntConditionKind::LESS_THAN,
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
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        ValueVertex *f64 = self->NewVertex<CheckedNumberToF64Vertex>(
            currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
            self->preproc_->GetBytecodeOffset(currentBcIndex));
        compileInfoFacts_->EnsureType(value, NodeInfo::NodeType::NUMBER);
        compileInfoFacts_->SetAlternative(value, AlternativeNodes::Kind::HOLEY_FLOAT64, f64);
        return f64;
    }

    ValueVertex *BuildF64BinOpValue(BinaryOpKind kind, ValueVertex *leftF64, ValueVertex *rightF64)
    {
        switch (kind) {
            case BinaryOpKind::ADD:
                return self->NewVertex<F64AddVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftF64, rightF64});
            case BinaryOpKind::SUB:
                return self->NewVertex<F64SubVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftF64, rightF64});
            case BinaryOpKind::MUL:
                return self->NewVertex<F64MulVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftF64, rightF64});
            case BinaryOpKind::DIV:
                return self->NewVertex<F64DivVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftF64, rightF64});
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
            self->NewVertex<F64ToTaggedDoubleVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{rawResult});
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
        ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
        uint32_t firstDeoptInputIndex = AppendCurrentFrameStateForDeopt(&inputs, &deoptVRegs);
        ValueVertex *tagged = self->NewVertex<CheckedNonNegativeI32ToTaggedIntVertex>(
            currentBlock, inputs, firstDeoptInputIndex, std::move(deoptVRegs),
            self->preproc_->GetBytecodeOffset(currentBcIndex));
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
        ValueVertex *raw =
            self->NewVertex<I32BitwiseBinaryVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32}, kind);
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
        return self->NewVertex<F64ToI32TruncVertex>(
            compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueF64});
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
        IntConditionKind condition =
            trueIfNonZero ? IntConditionKind::NOT_EQUAL : IntConditionKind::EQUAL;
        ValueVertex *result = self->NewVertex<I32ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueI32, zero}, condition);
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildGenericCompareOp(CompareOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        CommonStubID stubId;
        switch (kind) {
            case CompareOpKind::EQUAL:
                stubId = CommonStubID::Equal;
                break;
            case CompareOpKind::NOT_EQUAL:
                stubId = CommonStubID::NotEqual;
                break;
            case CompareOpKind::LESS_THAN:
                stubId = CommonStubID::Less;
                break;
            case CompareOpKind::LESS_THAN_OR_EQUAL:
                stubId = CommonStubID::LessEq;
                break;
            case CompareOpKind::GREATER_THAN:
                stubId = CommonStubID::Greater;
                break;
            case CompareOpKind::GREATER_THAN_OR_EQUAL:
                stubId = CommonStubID::GreaterEq;
                break;
            case CompareOpKind::STRICT_EQUAL:
                stubId = CommonStubID::StrictEqual;
                break;
            case CompareOpKind::STRICT_NOT_EQUAL:
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
        ValueVertex *result =
            self->NewVertex<TaggedEqualVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{left, right});
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildTaggedNotEqual(ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *result =
            self->NewVertex<TaggedNotEqualVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{left, right});
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *TryReduceCompareEqualAgainstConstant(CompareOpKind kind, ValueVertex *left, ValueVertex *right)
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

    ValueVertex *BuildI32CompareTaggedValue(CompareOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        std::optional<int32_t> leftValue = TryGetInt32Value(left);
        std::optional<int32_t> rightValue = TryGetInt32Value(right);
        if (leftValue.has_value() && rightValue.has_value()) {
            return GetBooleanConstant(EvaluateInt32Compare(kind, *leftValue, *rightValue));
        }

        ValueVertex *leftI32 = BuildTaggedIntToI32(left);
        ValueVertex *rightI32 = BuildTaggedIntToI32(right);
        ValueVertex *result = self->NewVertex<I32ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32},
            Int32ConditionFromCompare(kind));
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildI32CompareOp(CompareOpKind kind, ValueVertex *left, ValueVertex *right,
                                   bool leftKnownInt, bool rightKnownInt)
    {
        if (leftKnownInt && rightKnownInt) {
            return BuildI32CompareTaggedValue(kind, left, right);
        }

        ValueVertex *leftI32 = leftKnownInt ? BuildTaggedIntToI32(left) : BuildCheckedTaggedIntToI32(left);
        ValueVertex *rightI32 = rightKnownInt ? BuildTaggedIntToI32(right) : BuildCheckedTaggedIntToI32(right);
        ValueVertex *result = self->NewVertex<I32ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftI32, rightI32},
            Int32ConditionFromCompare(kind));
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildF64CompareTaggedValue(CompareOpKind kind, ValueVertex *leftF64, ValueVertex *rightF64)
    {
        if (auto *leftConst = leftF64->TryCast<Float64ConstantVertex>()) {
            if (auto *rightConst = rightF64->TryCast<Float64ConstantVertex>()) {
                return GetBooleanConstant(EvaluateFloat64Compare(kind, leftConst->GetValue(), rightConst->GetValue()));
            }
        }

        ValueVertex *result = self->NewVertex<F64ConditionCheckVertex>(
            compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{leftF64, rightF64},
            Int32ConditionFromCompare(kind));
        compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
        return result;
    }

    ValueVertex *BuildF64CompareOp(CompareOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        ValueVertex *leftF64 = BuildCheckedNumberToF64(left);
        ValueVertex *rightF64 = BuildCheckedNumberToF64(right);
        return BuildF64CompareTaggedValue(kind, leftF64, rightF64);
    }

    ValueVertex *BuildStringCompareOp(CompareOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        if (kind == CompareOpKind::EQUAL || kind == CompareOpKind::STRICT_EQUAL) {
            ValueVertex *result = self->NewVertex<StringEqualVertex>(
                currentBlock, std::initializer_list<ValueVertex *>{glue, left, right, GlobalEnv()});
            compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::BOOLEAN);
            return result;
        }
        if (kind == CompareOpKind::NOT_EQUAL || kind == CompareOpKind::STRICT_NOT_EQUAL) {
            // NOT_EQUAL = !EQUAL: build StringEqual then negate the boolean.
            ValueVertex *equal = BuildStringCompareOp(InvertCompare(kind), left, right);
            return BuildTaggedNotEqual(equal, GetBooleanConstant(true));
        }
        return BuildGenericCompareOp(kind, left, right);
    }

    ValueVertex *TryBuildStringCompareOp(CompareOpKind kind, ValueVertex *left, ValueVertex *right,
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

    ValueVertex *TryFoldUint32ComparedToNonPositive(CompareOpKind kind, ValueVertex *left, ValueVertex *right)
    {
        if (left == nullptr || !left->Is<CheckedNonNegativeI32ToTaggedIntVertex>()) {
            return nullptr;
        }
        std::optional<int32_t> rightValue = TryGetInt32Value(right);
        if (!rightValue.has_value() || *rightValue > 0) {
            return nullptr;
        }
        switch (kind) {
            case CompareOpKind::GREATER_THAN_OR_EQUAL:
                return GetBooleanConstant(true);   // uint32 >= 0 >= right
            case CompareOpKind::LESS_THAN:
                return GetBooleanConstant(false);  // uint32 >= 0, cannot be < right (<=0)
            case CompareOpKind::GREATER_THAN:
                if (*rightValue < 0) {
                    return GetBooleanConstant(true);  // uint32 >= 0 > right
                }
                return nullptr;  // right == 0: uint32 > 0 not always (could be 0)
            default:
                return nullptr;  // LE / equality: not always-resolvable
        }
    }

    ValueVertex *BuildCompareOperation(CompareOpKind kind)
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
        if (ValueVertex *provenInt = TryBuildProvenIntBinOp(kind, left, right)) {
            return provenInt;
        }
        OperationFeedback feedback = self->pgoContext_.ReadOperationFeedback(*currentBcInfo);
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
        auto buildUnaryInputs = [this, valueI32](ChunkVector<VRegIDType> *deoptVRegs) {
            std::vector<ValueVertex *> inputs {valueI32};
            AppendCurrentFrameStateForDeopt(&inputs, deoptVRegs);
            return inputs;
        };

        switch (stubId) {
            case CommonStubID::Inc: {
                ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
                std::vector<ValueVertex *> inputs = buildUnaryInputs(&deoptVRegs);
                ValueVertex *rawResult = self->NewVertex<I32IncWithOverflowVertex>(
                    currentBlock, inputs, I32IncWithOverflowVertex::FIRST_DEOPT_INDEX, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
                return BuildTaggedI32Result(rawResult);
            }
            case CommonStubID::Dec: {
                ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
                std::vector<ValueVertex *> inputs = buildUnaryInputs(&deoptVRegs);
                ValueVertex *rawResult = self->NewVertex<I32DecWithOverflowVertex>(
                    currentBlock, inputs, I32DecWithOverflowVertex::FIRST_DEOPT_INDEX, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
                return BuildTaggedI32Result(rawResult);
            }
            case CommonStubID::Neg: {
                ChunkVector<VRegIDType> deoptVRegs {self->chunk_};
                std::vector<ValueVertex *> inputs = buildUnaryInputs(&deoptVRegs);
                ValueVertex *rawResult = self->NewVertex<I32NegWithOverflowVertex>(
                    currentBlock, inputs, I32NegWithOverflowVertex::FIRST_DEOPT_INDEX, std::move(deoptVRegs),
                    self->preproc_->GetBytecodeOffset(currentBcIndex));
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
            ValueVertex *rawResult =
                self->NewVertex<I32BNotVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueI32});
            return BuildTaggedI32Result(rawResult);
        }
        ValueVertex *valueF64 = BuildCheckedNumberToF64(value);
        ValueVertex *truncI32 =
            self->NewVertex<F64ToI32TruncVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueF64});
        ValueVertex *rawResult =
            self->NewVertex<I32BNotVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{truncI32});
        return BuildTaggedI32Result(rawResult);
    }

    ValueVertex *BuildF64UnaryOp(CommonStubID stubId, ValueVertex *value)
    {
        ValueVertex *valueF64 = BuildCheckedNumberToF64(value);
        switch (stubId) {
            case CommonStubID::Neg: {
                ValueVertex *negF64 =
                    self->NewVertex<F64NegVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueF64});
                ValueVertex *result =
                    self->NewVertex<F64ToTaggedDoubleVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{negF64});
                compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::NUMBER);
                return result;
            }
            case CommonStubID::Inc: {
                ValueVertex *oneF64 = self->graph_->GetFloat64Constant(1.0);
                ValueVertex *addF64 =
                    self->NewVertex<F64AddVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueF64, oneF64});
                ValueVertex *result =
                    self->NewVertex<F64ToTaggedDoubleVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{addF64});
                compileInfoFacts_->EnsureType(result, NodeInfo::NodeType::NUMBER);
                return result;
            }
            case CommonStubID::Dec: {
                ValueVertex *oneF64 = self->graph_->GetFloat64Constant(1.0);
                ValueVertex *subF64 =
                    self->NewVertex<F64SubVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{valueF64, oneF64});
                ValueVertex *result =
                    self->NewVertex<F64ToTaggedDoubleVertex>(compileInfoFacts_, currentBlock, std::initializer_list<ValueVertex *>{subF64});
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

    bool BuildCheckHClass(uint32_t bcIndex, ValueVertex *object, JSHClass *hclass)
    {
        if (compileInfoFacts_->TryGetHClass(object) == hclass) {
            return true;
        }
        if (std::optional<JSTaggedValue> constant = TryGetConstantHeapObject(object)) {
            if (constant->IsHole() || !constant->IsHeapObject() ||
                constant->GetTaggedObject()->GetClass() != hclass) {
                return false;
            }
            compileInfoFacts_->RecordHClass(object, hclass, kungfu::StableHClassDependency::IsValid(hclass));
            return true;
        }

        if (kungfu::StableHClassDependency::IsValid(hclass)) {
            auto *dependencies = self->preproc_->GetEnv()->GetDependencies();
            if (dependencies == nullptr || !dependencies->DependOnStableHClass(hclass)) {
                return false;
            }
        }

        std::vector<ValueVertex *> checkInputs {object};
        ChunkVector<VRegIDType> deoptVRegs(self->chunk_);
        BuildCurrentFrameStateForDeopt(bcIndex, &checkInputs, &deoptVRegs);
        self->NewVertex<DeoptIfHClassMismatchVertex>(
            currentBlock, checkInputs, hclass, std::move(deoptVRegs), self->preproc_->GetBytecodeOffset(bcIndex));
        compileInfoFacts_->RecordHClass(object, hclass, kungfu::StableHClassDependency::IsValid(hclass));
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

    bool BuildCheckHClasses(
        uint32_t bcIndex, ValueVertex *object, const std::vector<JSHClass *> &hclasses, bool mapsAreKnownFresh)
    {
        if (hclasses.empty()) {
            return false;
        }
        if (hclasses.size() == 1) {
            return BuildCheckHClass(bcIndex, object, hclasses.front());
        }

        std::optional<std::vector<JSHClass *>> knownHClasses = compileInfoFacts_->TryGetPossibleHClasses(object);
        if (!mapsAreKnownFresh || !knownHClasses.has_value() ||
            !ContainsSameHClasses(knownHClasses.value(), hclasses)) {
            return false;
        }
        bool allStable = std::all_of(hclasses.begin(), hclasses.end(), [](JSHClass *hclass) {
            return hclass != nullptr && kungfu::StableHClassDependency::IsValid(hclass);
        });
        compileInfoFacts_->RecordPossibleHClasses(object, hclasses, allStable);
        return true;
    }

    ValueVertex *BuildLoadField(ValueVertex *object, PropertyLookupResult plr)
    {
        if (plr.IsInlinedProps()) {
            int32_t offset = static_cast<int32_t>(plr.GetOffset());
            return self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {object}, offset);
        }
        ValueVertex *properties = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {object}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        int32_t offset = static_cast<int32_t>(TaggedArray::DATA_OFFSET +
                                              plr.GetOffset() * JSTaggedValue::TaggedTypeSize());
        return self->NewVertex<LoadTaggedFieldVertex>(compileInfoFacts_, currentBlock, {properties}, offset);
    }

    void BuildStoreField(ValueVertex *object, ValueVertex *value, PropertyLookupResult plr)
    {
        if (plr.IsInlinedProps()) {
            self->NewVertex<StoreTaggedFieldVertex>(
                compileInfoFacts_, currentBlock, {object, value}, static_cast<int32_t>(plr.GetOffset()));
            return;
        }
        ValueVertex *properties = self->NewVertex<LoadTaggedFieldVertex>(
            currentBlock, {object}, static_cast<int32_t>(JSObject::PROPERTIES_OFFSET));
        int32_t offset = static_cast<int32_t>(
            TaggedArray::DATA_OFFSET + plr.GetOffset() * JSTaggedValue::TaggedTypeSize());
        self->NewVertex<StoreTaggedFieldVertex>(
            compileInfoFacts_, currentBlock, {properties, value}, offset);
    }

    ValueVertex *TryBuildPropertyLoad(ValueVertex *object, uint16_t constDataId,
                                      const NamedLoadAccessInfo &accessInfo)
    {
        LoadedPropertyKey key = LoadedPropertyKey::ConstDataId(object, constDataId, accessInfo.plr);
        ValueVertex *cached = compileInfoFacts_->LookupLoadedProperty(key);
        if (cached == nullptr) {
            cached = compileInfoFacts_->LookupLoadedConstantProperty(key);
        }
        if (cached != nullptr) {
            return cached;
        }

        ValueVertex *result = BuildLoadField(object, accessInfo.plr);
        if (accessInfo.isConst) {
            compileInfoFacts_->RecordLoadedConstantProperty(key, result);
        } else {
            compileInfoFacts_->RecordLoadedProperty(key, result);
        }
        return result;
    }

    bool TryBuildNamedAccess(uint32_t bcIndex, ValueVertex *receiver, uint16_t constDataId,
                             const std::vector<NamedLoadAccessInfo> &accessInfos, bool mapsAreKnownFresh)
    {
        if (accessInfos.empty()) {
            return false;
        }
        const NamedLoadAccessInfo &accessInfo = accessInfos.front();
        for (const NamedLoadAccessInfo &candidate : accessInfos) {
            if (!HasSameLoadFieldAccess(accessInfo, candidate)) {
                return false;
            }
        }
        const std::vector<JSHClass *> &maps = accessInfo.lookupStartObjectHClasses;
        bool hasHClassOfString = std::any_of(maps.begin(), maps.end(), [](JSHClass *hclass) {
            return hclass != nullptr && hclass->IsString();
        });
        if (hasHClassOfString || !BuildCheckHClasses(bcIndex, receiver, maps, mapsAreKnownFresh)) {
            return false;
        }
        frameState.SetAcc(TryBuildPropertyLoad(receiver, constDataId, accessInfo));
        return true;
    }

    bool TryBuildLoadNamedProperty(
        const BytecodeInfo *bcInfo, uint32_t bcIndex, ValueVertex *receiver, uint16_t constDataId)
    {
        if (std::optional<JSTaggedValue> constant = TryGetConstantHeapObject(receiver)) {
            if (constant->IsHole() || !constant->IsHeapObject()) {
                return false;
            }
            JSHClass *hclass = constant->GetTaggedObject()->GetClass();
            NamedLoadAccessInfoOpt accessInfo = TryGetPropertyAccessInfo(hclass, constDataId, std::nullopt);
            if (!accessInfo.has_value() || !RecordNamedAccessInfoDependencies(accessInfo.value())) {
                return false;
            }
            return TryBuildNamedAccess(bcIndex, receiver, constDataId, {accessInfo.value()}, false);
        }

        std::optional<NamedAccessFeedback> feedback =
            TryGetLoadObjByNameFeedback(GetICSlotId<kungfu::ICSlotIdType>(bcInfo, 0));
        if (!feedback.has_value()) {
            return false;
        }

        std::optional<std::vector<JSHClass *>> inferredMaps;
        bool mapsAreKnownFresh = false;
        if (feedback->maps.empty()) {
            inferredMaps = TryGetPossibleHClasses(receiver);
            mapsAreKnownFresh = inferredMaps.has_value();
        } else {
            KnownHClassesMerger merger(feedback->maps);
            merger.IntersectWithCompileInfoFacts(TryGetPossibleHClasses(receiver));
            inferredMaps = merger.GetIntersection();
        }
        if (!inferredMaps.has_value() || inferredMaps->empty()) {
            return false;
        }

        NamedLoadAccessInfosOpt accessInfos =
            TryGetLoadObjByNameAccessInfos(inferredMaps.value(), feedback.value(), constDataId);
        return accessInfos.has_value() &&
               TryBuildNamedAccess(bcIndex, receiver, constDataId, accessInfos.value(), mapsAreKnownFresh);
    }

    bool TryBuildStoreNamedProperty(const BytecodeInfo *bcInfo, ValueVertex *receiver,
                                    uint16_t constDataId, ValueVertex *value)
    {
        JSHClass *hclass = compileInfoFacts_->TryGetHClass(receiver);
        if (hclass == nullptr) {
            return false;
        }
        std::optional<JSTaggedValue> name = TryGetNameFromConstDataId(constDataId);
        if (!name.has_value()) {
            return false;
        }
        PropertyLookupResult plr =
            JSHClass::LookupPropertyInPGOHClass(self->compilerThread_, hclass, name.value());
        if (!plr.IsFound() || !plr.IsLocal() || !plr.IsWritable() || plr.IsAccessor()) {
            return false;
        }
        if (kungfu::StableHClassDependency::IsValid(hclass)) {
            auto *dependencies = self->preproc_->GetEnv()->GetDependencies();
            if (dependencies == nullptr || !dependencies->DependOnStableHClass(hclass)) {
                return false;
            }
        }
        BuildStoreField(receiver, value, plr);
        return true;
    }

    void UpdateCatchBlockData(ThrowableMixin *mixin)
    {
        if (reinterpret_cast<uintptr_t>(lazyCatchBlock) == NO_CATCH_BLOCK_TAG) {
            return;
        }
        if (UNLIKELY(lazyCatchBlock == nullptr)) {
            lazyCatchBlock = self->ActivateCatchBlock(&lazyCatchBlockInputs, blockInfo->catchBlock->rpoIndex);
        }
        ASSERT(lazyCatchBlockInputs != nullptr);
        uint32_t catchPredIndex = lazyCatchBlockInputs->AddCatchPredecessor(frameState, self->undefinedValue_);
        lazyCatchBlockInputs->AddCompileInfoFacts(*compileInfoFacts_);
        mixin->SetCaughtBy(lazyCatchBlock);
        mixin->SetCatchPredecessorIndex(catchPredIndex);
    }

    ValueVertex *CommonStubCall(std::initializer_list<ValueVertex *> inputs, CommonStubID id,
                                SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        ValidateCommonStubCallArgs({inputs.begin(), inputs.end()}, id);
        auto *vertex = self->NewVertex<CallCommonStubVertex>(
            compileInfoFacts_, currentBlock, inputs, id, sideEffectKind);
        UpdateCatchBlockData(vertex);
        return vertex;
    }

    ValueVertex *CommonStubCallWithIC(
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

    template <class InputRange = std::initializer_list<ValueVertex *>>
    ValueVertex *RuntimeCall(const InputRange &inputs, RuntimeStubID id,
                             SideEffectKind sideEffectKind = SideEffectKind::UNKNOWN_CALL)
    {
        auto *vertex = self->NewVertex<CallRuntimeVertex>(
            compileInfoFacts_, currentBlock, inputs, id, sideEffectKind);
        UpdateCatchBlockData(vertex);
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
        self->NewVertex<StoreTaggedFieldVertex>(compileInfoFacts_, currentBlock, {array, value}, offset);
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

    ValueVertex *StringFromConstPool(ValueVertex *stringId)
    {
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

void GraphBuilder::VisitBytecodesOfBasicBlock(SharedBCFrameState frameState, uint32_t rpoIndex)
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
        visitor.Visit(preproc_->GetBytecode(bcIndex), bcIndex);
    }

    if (visitor.currentBlock->GetControlVertex() == nullptr) {
        ASSERT(blockInfo->IsFallthrough());
        BB *target = ActivateNonCatchBlock(blockInfo->fallthroughBlock->rpoIndex);
        FinishBlockWithJump(visitor.currentBlock, target);
    }
}
}  // namespace panda::ecmascript::arksteed
