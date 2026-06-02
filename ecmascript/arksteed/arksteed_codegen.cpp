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

#include "ecmascript/arksteed/arksteed_codegen.h"

#include <algorithm>
#include <sstream>

#include "ecmascript/arksteed/arksteed_assembler-inl.h"  // IWYU pragma: keep
#include "ecmascript/arksteed/arksteed_safepoint_table.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {

class GapMoveResolver {
    static constexpr uint8_t UNVISITED = 0;
    static constexpr uint8_t VISITED = 1;
    static constexpr uint8_t TEMP_REQUIRED = 2;

public:
    using AssignmentPair = std::pair<AllocatedState, AllocatedState>;

    explicit GapMoveResolver(Chunk *chunk, ArkSteedRegister scratchGPR, ArkSteedDoubleRegister scratchFPR)
        : scratchGPR_(scratchGPR),
          scratchFPR_(scratchFPR),
          assignments_(chunk),
          reorderedAssignments_(chunk),
          sortedList_(chunk),
          adjLists_(chunk)
    {}

    void Add(AllocatedState dest, AllocatedState src)
    {
        assignments_.emplace_back(dest, src);
    }

    void Resolve()
    {
        if (assignments_.empty()) {
            return;
        }
        BuildSortedOperandList();
        BuildAdjacencyLists();
        reorderedAssignments_.reserve(assignments_.size() * 2);  // 2: reserve space for both src and dest
        size_t n = sortedList_.size();
        ChunkVector<uint8_t> states(n, GetChunk());
        for (size_t i = 0; i < n; i++) {
            if (states[i] == UNVISITED) {
                DFS(static_cast<uint32_t>(i), states);
            }
        }
    }

    Span<const AssignmentPair> ReorderedAssignments() const
    {
        return {reorderedAssignments_.data(), reorderedAssignments_.size()};
    }

private:
    uint32_t SortedIndexOf(AllocatedState operand) const
    {
        auto compFn = [](AllocatedState x, AllocatedState y) { return x.GetUnderlyingData() < y.GetUnderlyingData(); };
        auto it = std::lower_bound(sortedList_.begin(), sortedList_.end(), operand, compFn);
        ASSERT(it != sortedList_.end() && *it == operand);
        return static_cast<uint32_t>(it - sortedList_.begin());
    }

    void BuildSortedOperandList()
    {
        sortedList_.reserve(assignments_.size() * 2);  // 2: reserve space for both src and dest
        for (auto [dest, src] : assignments_) {
            sortedList_.push_back(dest);
            sortedList_.push_back(src);
        }
        std::sort(sortedList_.begin(), sortedList_.end(), [](AllocatedState x, AllocatedState y) {
            return x.GetUnderlyingData() < y.GetUnderlyingData();
        });
        sortedList_.erase(std::unique(sortedList_.begin(), sortedList_.end()), sortedList_.end());
    }

    void BuildAdjacencyLists()
    {
        uint32_t n = static_cast<uint32_t>(sortedList_.size());
        adjLists_.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            adjLists_.emplace_back(GetChunk());
        }
        for (auto [dest, src] : assignments_) {
            uint32_t destIndex = SortedIndexOf(dest);
            uint32_t srcIndex = SortedIndexOf(src);
            adjLists_[srcIndex].push_back(destIndex);
        }
    }

    void DFS(uint32_t index, ChunkVector<uint8_t> &states)
    {
        states[index] = VISITED;
        for (uint32_t destIndex : adjLists_[index]) {
            if (states[destIndex] == VISITED) {
                AllocatedState tempDest = ScratchOperandLike(sortedList_[destIndex]);
                reorderedAssignments_.emplace_back(tempDest, sortedList_[destIndex]);
                states[destIndex] = TEMP_REQUIRED;
            } else {
                ASSERT(states[destIndex] == UNVISITED);
                DFS(destIndex, states);
            }
        }
        if (states[index] == TEMP_REQUIRED) {
            AllocatedState tempSrc = ScratchOperandLike(sortedList_[index]);
            for (uint32_t destIndex : adjLists_[index]) {
                reorderedAssignments_.emplace_back(sortedList_[destIndex], tempSrc);
            }
        } else {
            ASSERT(states[index] == VISITED);
            for (uint32_t destIndex : adjLists_[index]) {
                AllocatedState dest = sortedList_[destIndex];
                AllocatedState src = sortedList_[index];
                ASSERT(dest.GetRepresentation() == src.GetRepresentation());
                reorderedAssignments_.emplace_back(dest, src);
            }
        }
    }

    AllocatedState ScratchOperandLike(AllocatedState input)
    {
        MachineRepresentation repr = input.GetRepresentation();
        switch (repr) {
            case MachineRepresentation::Bit:
            case MachineRepresentation::Word8:
            case MachineRepresentation::Word16:
            case MachineRepresentation::Word32:
            case MachineRepresentation::Word64:
            case MachineRepresentation::Tagged:
                return AllocatedState(AllocatedState::REGISTER, repr, scratchGPR_.Code());
            case MachineRepresentation::Float64:
                return AllocatedState(AllocatedState::REGISTER, repr, scratchFPR_.Code());
            default:
                UNREACHABLE();
        }
    }

    Chunk *GetChunk() const
    {
        return assignments_.get_allocator().chunk();
    }

    ArkSteedRegister scratchGPR_;
    ArkSteedDoubleRegister scratchFPR_;
    ChunkVector<AssignmentPair> assignments_;
    ChunkVector<AssignmentPair> reorderedAssignments_;
    ChunkVector<AllocatedState> sortedList_;
    ChunkVector<ChunkVector<uint32_t>> adjLists_;
};

namespace {

ArkSteedRegister GetInputRegister(const Vertex *vertex, int index)
{
    const InputLocation *loc = vertex->GetInputLocation(index);
    ASSERT(loc->IsRegister());
    return loc->GetAssignedGeneralRegister();
}

ArkSteedRegister GetResultRegister(const ValueVertex *vertex)
{
    const ValueLocation &loc = vertex->Result();
    ASSERT(loc.IsRegister());
    return loc.GetAssignedGeneralRegister();
}

template <typename T>
struct GetRegister;
template <>
struct GetRegister<ArkSteedRegister> {
    static ArkSteedRegister Get(AllocatedState target)
    {
        return target.GetRegister();
    }
};
template <>
struct GetRegister<ArkSteedDoubleRegister> {
    static ArkSteedDoubleRegister Get(AllocatedState target)
    {
        return target.GetDoubleRegister();
    }
};
bool IsGapMoveVertex(Vertex *vertex)
{
    return vertex->Is<GapMoveVertex>() || vertex->Is<ConstantGapMoveVertex>();
}
}  // namespace

template <class VertexT>
void ArkSteedCodeGenerator::VisitNonControlVertex(VertexT *vertex)
{
    std::ostringstream out;
    vertex->Dump(out);
    LOG_COMPILER(FATAL) << "Unimplemented assembly generation for vertex: " << out.str();
    UNREACHABLE();
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<PhiVertex>(PhiVertex *vertex)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << vertex->GetId() << ": PhiVertex [no-op]";
#endif
}

// ========================================= Common Value Opcode =========================================

void ArkSteedCodeGenerator::LoadConstantToRegister(const ValueVertex *constVertex, ArkSteedRegister reg)
{
    ASSERT(constVertex != nullptr);
    switch (constVertex->GetOpcode()) {
        case VertexOpcode::Constant:
            constVertex->Cast<ConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::Int32Constant:
            constVertex->Cast<Int32ConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::IntPtrConstant:
            constVertex->Cast<IntPtrConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::TaggedConstant:
            constVertex->Cast<TaggedConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::RootConstant:
            constVertex->Cast<RootConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::BooleanConstant:
            constVertex->Cast<BooleanConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedCodeGenerator::StoreStubStackArgument(const Vertex *callVertex, int paramIdx,
                                                   ArkSteedAssembler::MemoryOperand destMem)
{
    const InputLocation *loc = callVertex->GetInputLocation(paramIdx);
    const InstructionOperand &operand = loc->GetOperand();
    if (operand.IsAnyRegister()) {
        ArkSteedRegister src = AllocatedState::Cast(operand).GetRegister();
        assembler_->MoveRepr(MachineRepresentation::Tagged, destMem, src);
    } else if (operand.IsAnyStackSlot()) {
        ArkSteedAssembler::MemoryOperand srcMem = assembler_->ToMemOperand(operand);
        assembler_->MoveRepr(MachineRepresentation::Tagged, destMem, srcMem);
    } else if (operand.IsConstant()) {
        ScratchRegisterScope scope;
        ArkSteedRegister scratch = scope.AcquireScratch();
        LoadConstantToRegister(callVertex->Arg(paramIdx).vertex(), scratch);
        assembler_->MoveRepr(MachineRepresentation::Tagged, destMem, scratch);
    } else {
        UNREACHABLE();
    }
}

int ArkSteedCodeGenerator::PrepareCommonStubStackArguments(const Vertex *callVertex, int argCount)
{
    const int stackArgCount = std::max(0, argCount - ArkSteedAssembler::NUM_ARG_REGISTERS);
    // Keep the stack 16-byte aligned at each call site by reserving an even number of slots.
    const int reservedSlotCount = (stackArgCount + 1) & ~1;  // ~1: round down to even number (2-slot alignment)
    assembler_->ReserveCallArgSlots(reservedSlotCount);
    for (int paramIdx = ArkSteedAssembler::NUM_ARG_REGISTERS; paramIdx < argCount; paramIdx++) {
        int stackSlotIdx = paramIdx - ArkSteedAssembler::NUM_ARG_REGISTERS;
        ArkSteedAssembler::MemoryOperand destMem = assembler_->GetCallArgSlot(stackSlotIdx);
        StoreStubStackArgument(callVertex, paramIdx, destMem);
    }
    if (reservedSlotCount > stackArgCount) {
        ScratchRegisterScope scope;
        ArkSteedRegister scratch = scope.AcquireScratch();
        assembler_->Move(scratch, static_cast<int64_t>(JSTaggedValue::VALUE_UNDEFINED));
        assembler_->MoveRepr(MachineRepresentation::Tagged, assembler_->GetCallArgSlot(stackArgCount), scratch);
    }
    return reservedSlotCount;
}

int ArkSteedCodeGenerator::PrepareRuntimeStubStackArguments(const Vertex *callVertex, int argCount, int runtimeId)
{
    const int stackArgCount = argCount + 2;  // 2: extra slots for runtimeId and argc
    const int reservedSlotCount = (stackArgCount + 1) & ~1;  // ~1: round down to even number (2-slot alignment)
    assembler_->ReserveCallArgSlots(reservedSlotCount);

    {
        ScratchRegisterScope scope;
        ArkSteedRegister scratch = scope.AcquireScratch();
        assembler_->Move(scratch, static_cast<int64_t>(runtimeId));
        assembler_->MoveRepr(MachineRepresentation::Word64, assembler_->GetCallArgSlot(0), scratch);
        assembler_->Move(scratch, static_cast<int64_t>(argCount));
        assembler_->MoveRepr(MachineRepresentation::Word64, assembler_->GetCallArgSlot(1), scratch);
    }

    for (int paramIdx = 0; paramIdx < argCount; paramIdx++) {
        // 2: skip runtimeId and argc slots
        ArkSteedAssembler::MemoryOperand destMem = assembler_->GetCallArgSlot(paramIdx + 2);
        StoreStubStackArgument(callVertex, paramIdx, destMem);
    }
    return reservedSlotCount;
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CallRuntimeVertex>(CallRuntimeVertex *callRuntime)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << callRuntime->GetId() << ": CallRuntimeVertex";
#endif

    int stackArgCount = PrepareRuntimeStubStackArguments(callRuntime,
                                                         callRuntime->GetArgCount(),
                                                         static_cast<int>(callRuntime->GetRuntimeId()));
    assembler_->CallRuntime(callRuntime->GetRuntimeId());
    assembler_->FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptVertex>(DeoptVertex *deopt)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << deopt->GetId() << ": DeoptVertex [UNIMPLEMENTED]";
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<InitialValueVertex>(InitialValueVertex *initialValue)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << initialValue->GetId() << ": InitialValueVertex [no-op]";
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ActualArgcVertex>(ActualArgcVertex *actualArgc)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << actualArgc->GetId() << ": ActualArgcVertex";
#endif
    auto dst = GetResultRegister(actualArgc);
    assembler_->LoadActualArgc(dst);
}

// ========================================= Slow Value Opcode =========================================

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadFromAddressVertex>(LoadFromAddressVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadFromAddressVertex";
#endif
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadFromAddressVertex::OBJECT_INDEX);
    assembler_->LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadExceptionVertex>(LoadExceptionVertex *loadException)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadException->GetId() << ": LoadExceptionVertex";
#endif
    auto dst = GetResultRegister(loadException);
    auto glue = GetInputRegister(loadException, LoadExceptionVertex::GLUE_INDEX);
    assembler_->LoadAndClearPendingException(dst, glue);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadTaggedFieldVertex>(LoadTaggedFieldVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadTaggedFieldVertex";
#endif
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadTaggedFieldVertex::OBJECT_INDEX);
    assembler_->LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreToAddressVertex>(StoreToAddressVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreToAddressVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreToAddressVertex::VALUE_INDEX);
    assembler_->StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedFieldVertex>(StoreTaggedFieldVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreTaggedFieldVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreTaggedFieldVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedFieldVertex::VALUE_INDEX);
    assembler_->StoreField(value, obj, storeField->GetOffset());
}

// ========================================= Non-Value Opcode =========================================

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ThrowIfSuperNotCorrectCallVertex>(
    ThrowIfSuperNotCorrectCallVertex *throwIfSuper)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << throwIfSuper->GetId() << ": ThrowIfSuperNotCorrectCallVertex";
#endif
    int stackArgCount = PrepareRuntimeStubStackArguments(throwIfSuper,
                                                         throwIfSuper->GetArgCount(),
                                                         static_cast<int>(throwIfSuper->GetRuntimeId()));
    assembler_->CallRuntime(throwIfSuper->GetRuntimeId());
    assembler_->FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ThrowIfNotObjectVertex>(ThrowIfNotObjectVertex *throwIfNotObj)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << throwIfNotObj->GetId() << ": ThrowIfNotObjectVertex";
#endif
    int stackArgCount = PrepareRuntimeStubStackArguments(throwIfNotObj,
                                                         throwIfNotObj->GetArgCount(),
                                                         static_cast<int>(throwIfNotObj->GetRuntimeId()));
    assembler_->CallRuntime(throwIfNotObj->GetRuntimeId());
    assembler_->FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ThrowUndefinedIfHoleVertex>(ThrowUndefinedIfHoleVertex *throwIfHole)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << throwIfHole->GetId() << ": ThrowUndefinedIfHoleVertex";
#endif
    int stackArgCount = PrepareRuntimeStubStackArguments(throwIfHole,
                                                         throwIfHole->GetArgCount(),
                                                         static_cast<int>(throwIfHole->GetRuntimeId()));
    assembler_->CallRuntime(throwIfHole->GetRuntimeId());
    assembler_->FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ThrowUndefinedIfHoleWithNameVertex>(
    ThrowUndefinedIfHoleWithNameVertex *throwIfHoleWithName)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << throwIfHoleWithName->GetId()
                        << ": ThrowUndefinedIfHoleWithNameVertex";
#endif
    int stackArgCount = PrepareRuntimeStubStackArguments(throwIfHoleWithName,
                                                         throwIfHoleWithName->GetArgCount(),
                                                         static_cast<int>(throwIfHoleWithName->GetRuntimeId()));
    assembler_->CallRuntime(throwIfHoleWithName->GetRuntimeId());
    assembler_->FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<GapMoveVertex>(GapMoveVertex *gapMove)
{
    const AllocatedState &source = gapMove->GetSource();
    const AllocatedState &target = gapMove->GetTarget();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting GapMoveVertex: " << target.Description() << " <- "
                        << source.Description();
#endif
    ExecuteGapMove(target, source);
}

template <typename VertexType>
inline void LogConstGapMove(const VertexType *vertex, const AllocatedState &target)
{
#ifndef NDEBUG
    std::ostringstream out;
    out << "CodeGen: Visiting ConstantGapMoveVertex: " << target.Description() << " <- v" << vertex->GetId()
        << " which is ";
    vertex->Dump(out);
    LOG_COMPILER(DEBUG) << out.str();
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ConstantGapMoveVertex>(ConstantGapMoveVertex *constGapMove)
{
    auto target = constGapMove->GetTarget();
    auto vertex = constGapMove->GetVertex();

    switch (vertex->GetOpcode()) {
        // clang-format off
#define CONST_GAP_CASE(Name)                                                                                     \
        case VertexOpcode::Name: {                                                                               \
            auto *v = vertex->Cast<Name##Vertex>();                                                              \
            LogConstGapMove(v, target);                                                                          \
            v->DoLoadToRegister(assembler_, GetRegister<Name##Vertex::OutputRegister>::Get(target));             \
            break;                                                                                               \
        }
        CONSTANT_VALUE_VERTEX_LIST(CONST_GAP_CASE)
#undef CONST_GAP_CASE
        // clang-format on
        default:
            UNREACHABLE();
    }
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ToTaggedIntVertex>(ToTaggedIntVertex *toTaggedInt)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << toTaggedInt->GetId() << ": ToTaggedIntVertex";
#endif
    auto dst = GetResultRegister(toTaggedInt);
    auto src = GetInputRegister(toTaggedInt, ToTaggedIntVertex::INPUT_INDEX);
    assembler_->Move(dst, src);
    assembler_->Or(dst, JSTaggedValue::TAG_INT);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CallCommonStubVertex>(CallCommonStubVertex *callCommonStub)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << callCommonStub->GetId() << ": CallCommonStubVertex";
#endif
    int stackArgCount = PrepareCommonStubStackArguments(callCommonStub, callCommonStub->GetArgCount());
    assembler_->CallCommonStub(callCommonStub->GetStubId());
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());
    assembler_->FreeCallArgSlots(stackArgCount);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<JumpVertex>(JumpVertex *jump)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jump->GetId() << ": JumpVertex to BB #" << jump->Target()->GetId();
#endif
    uint32_t curBlockIndex = jump->GetOwner()->GetId();
    bool isFallthrough = (jump->Target()->GetId() == curBlockIndex + 1);
    if (!isFallthrough) {
        assembler_->Jump(jump->Target()->GetLabel());
    } else {
        LOG_COMPILER(DEBUG) << "Fallthrough: No jump instruction generated";
    }
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<JumpLoopVertex>(JumpLoopVertex *jumpLoop)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpLoop->GetId() << ": JumpLoopVertex to BB #"
                        << jumpLoop->Target()->GetId();
#endif
    assembler_->Jump(jumpLoop->Target()->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTrueVertex>(BranchIfTrueVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    uint32_t curBlockIndex = jumpIf->GetOwner()->GetId();
    bool trueBranchIsFallthrough = (ifTrue->GetId() == curBlockIndex + 1);
    bool falseBranchIsFallthrough = (ifFalse->GetId() == curBlockIndex + 1);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfTrueVertex to BB #" << ifTrue->GetId()
                        << (trueBranchIsFallthrough ? " (fallthrough)" : "") << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto cond = GetInputRegister(jumpIf, 0);
    // to do: optimize
    ScratchRegisterScope scope;
    ArkSteedRegister scratch = scope.AcquireScratch();
    assembler_->LoadTaggedValue(scratch, JSTaggedValue::True().GetRawData());
    assembler_->Compare(cond, scratch);

    assembler_->Branch(Condition::COND_ZERO,
                       ifTrue->GetLabel(),
                       trueBranchIsFallthrough,
                       ifFalse->GetLabel(),
                       falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<ReturnVertex>(ReturnVertex *returns)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << returns->GetId() << ": ReturnVertex";
#endif
    assembler_->Epilogue();
    assembler_->Return();
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<ThrowVertex>(ThrowVertex *throws)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << throws->GetId() << ": ThrowVertex";
#endif
    int stackArgCount =
        PrepareRuntimeStubStackArguments(throws, throws->GetArgCount(), static_cast<int>(throws->GetRuntimeId()));
    assembler_->CallRuntime(throws->GetRuntimeId());
    assembler_->FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(assembler_->GetPcOffset());

    BB *catchBlock = throws->CaughtBy();
    if (catchBlock != nullptr) {
        uint32_t catchPredId = throws->GetCatchPredecessorIndex();
        DeconstructPhisInSuccessor(catchBlock, static_cast<int>(catchPredId));
        assembler_->Jump(catchBlock->GetLabel());
    } else {
        assembler_->ReturnWithPendingException();
    }
}

int32_t ArkSteedCodeGenerator::ComputeFrameSize(Graph *graph)
{
    uint32_t tagged = graph->GetTaggedStackSlots();
    uint32_t untagged = graph->GetUntaggedStackSlots();
    return static_cast<int32_t>((tagged + untagged) * sizeof(uint64_t));
}

void ArkSteedCodeGenerator::Generate()
{
    // Compute block colors only when code comments are enabled
    // This minimizes JIT compilation overhead when comments are disabled
    if (assembler_->IsCommentEnabled()) {
        ComputeBlockColors();
    }

    assembler_->Prologue(graph_);

    for (int i = 0, numBlocks = graph_->NumBlocks(); i < numBlocks; ++i) {
        BB *curBlock = (*graph_)[i];
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "CodeGen: Starts BB #" << curBlock->GetId();
#endif

        RecordBlockComment(curBlock);
        assembler_->Bind(curBlock->GetLabel());

        if (curBlock->HasPhi()) {
            for (PhiVertex *phi : curBlock->GetPhis()) {
                ProcessNonControlVertex(phi);
            }
        }
        for (NonControlVertex *vertex : curBlock->GetVertices()) {
            ProcessNonControlVertex(vertex);
        }
        ControlVertex *controlVertex = curBlock->GetControlVertex();
        // Precondition: All critical edges have been split before.
        if (JumpVertex *jump = controlVertex->TryCast<JumpVertex>(); jump != nullptr) {
            DeconstructPhisInSuccessor(jump->Target(), jump->GetPredecessorId());
        } else if (JumpLoopVertex *jumpLoop = controlVertex->TryCast<JumpLoopVertex>(); jumpLoop != nullptr) {
            DeconstructPhisInSuccessor(jumpLoop->Target(), jumpLoop->GetPredecessorId());
        }

        ProcessControlVertex(controlVertex);
    }
}

void ArkSteedCodeGenerator::ProcessValueVertex(ValueVertex *valueVertex)
{
    RegallocValueVertexInfo *vertexInfo = valueVertex->GetRegallocInfo();
    if (vertexInfo->HasValidLiveRange() && vertexInfo->IsSpilled()) {
        const AllocatedState &source = AllocatedState::Cast(vertexInfo->GetResult().GetOperand());
        // We shouldn't spill vertices which already output to the stack.
        if (!source.IsAnyStackSlot()) {
            RecordSpillComment();
            if (source.IsRegister()) {
                auto spillSlot = AllocatedState::Cast(vertexInfo->GetSpillSlot());
                assembler_->MoveRepr(source.GetRepresentation(),
                                     assembler_->GetStackSlot(spillSlot),
                                     source.GetRegister());
            } else {
                UNREACHABLE();
            }
        } else {
            // Otherwise, the result source stack slot should be equal to the spill slot.
            auto spillSlot = AllocatedState::Cast(vertexInfo->GetSpillSlot());
            ASSERT(source.GetIndex() == spillSlot.GetIndex());
        }
    }
}

void ArkSteedCodeGenerator::ProcessNonControlVertex(NonControlVertex *vertex)
{
    RecordVertexComment(vertex);
    switch (vertex->GetOpcode()) {
#define PROCESS_VERTEX_CASE(Type)                                \
        case VertexOpcode::Type:                                 \
            VisitNonControlVertex(vertex->Cast<Type##Vertex>()); \
            break;
        NON_CONTROL_VERTEX_LIST(PROCESS_VERTEX_CASE)
#undef PROCESS_VERTEX_CASE
        default:
            UNREACHABLE();
            break;
    }

    // Handle spilling for ValueVertices with valid live range that need to be spilled.
    if (vertex->Is<ValueVertex>()) {
        ValueVertex *valueVertex = vertex->Cast<ValueVertex>();
        ProcessValueVertex(valueVertex);
    }

    if (vertex->GetProperties().CanThrow() || vertex->GetProperties().IsAnyCall()) {
        if (BB *catchBlock = CatchBlockOf(vertex)) {
            Label noException;
            assembler_->BranchIfNoPendingException(&noException);
            DeconstructPhisInSuccessor(catchBlock, static_cast<int>(CatchPredecessorIndexOf(vertex)));
            assembler_->Jump(catchBlock->GetLabel());
            assembler_->Bind(&noException);
        } else {
            assembler_->ReturnIfPendingException();
        }
    }
}

void ArkSteedCodeGenerator::ProcessControlVertex(ControlVertex *vertex)
{
    RecordVertexComment(vertex);
    switch (vertex->GetOpcode()) {
#define PROCESS_VERTEX_CASE(Type)                             \
        case VertexOpcode::Type:                              \
            VisitControlVertex(vertex->Cast<Type##Vertex>()); \
            break;
        CONTROL_VERTEX_LIST(PROCESS_VERTEX_CASE)
#undef PROCESS_VERTEX_CASE
        default:
            UNREACHABLE();
            break;
    }
    if (!vertex->Is<ThrowVertex>()) {
        assembler_->ReturnIfPendingException();
    }
}

void ArkSteedCodeGenerator::DeconstructPhisInSuccessor(BB *successor, int predecessorId)
{
    if (!successor->HasPhi() && !successor->HasRegisterMerge()) {
        return;
    }
    // Gap moves are part of the current block's code (preparing for jump to successor)
    // so we use the current block color, not the successor's color
    std::ostringstream gapMovesSs;
    gapMovesSs << GetCurrentBlockColor() << "--   Gap moves:" << COLOR_RESET;
    RecordComment(gapMovesSs.str().c_str());
    ScratchRegisterScope scope;
    ArkSteedRegister scratchGPR = scope.AcquireScratch();
    ArkSteedDoubleRegister scratchFPR = scope.AcquireDoubleScratch();

    GapMoveResolver resolver(graph_->GetChunk(), scratchGPR, scratchFPR);
    ChunkVector<std::pair<AllocatedState, ValueVertex *>> constantMoves(graph_->GetChunk());
    ArkSteedRegList registersSetByPhis;
    ArkDoubleRegList doubleRegistersSetByPhis;
    CollectPhiMoves(&resolver, successor, predecessorId, &registersSetByPhis, &doubleRegistersSetByPhis,
                    &constantMoves);
    CollectRegisterStateMoves(&resolver,
                              successor,
                              predecessorId,
                              registersSetByPhis,
                              doubleRegistersSetByPhis,
                              &constantMoves);

    resolver.Resolve();
    for (auto [dest, src] : resolver.ReorderedAssignments()) {
        RecordGapMoveComment(src, dest, nullptr);
        ExecuteGapMove(dest, src, &scratchGPR);
    }
    for (auto [dest, constVertex] : constantMoves) {
        ExecuteConstantPhiMove(dest, constVertex, &scratchGPR);
    }
}

void ArkSteedCodeGenerator::CollectPhiMoves(GapMoveResolver *resolver, BB *successor, int predecessorId,
                                            ArkSteedRegList *registersSetByPhis,
                                            ArkDoubleRegList *doubleRegistersSetByPhis,
                                            ChunkVector<std::pair<AllocatedState, ValueVertex *>> *constantMoves)
{
    if (!successor->HasPhi()) {
        return;
    }
    for (PhiVertex *phi : successor->GetPhis()) {
        // dest <- src: In-degree of dest = 1 at most
        InstructionOperand dest = phi->Result().GetOperand();
        InstructionOperand src = phi->GetInputLocation(predecessorId)->GetOperand();
        if (!dest.IsAllocated()) {
            // to do: Possibly a bug. To be investigated.
            LOG_COMPILER(WARN) << "Skips unallocated Phi v" << phi->GetId();
            continue;
        }
        if (src.IsConstant()) {
            constantMoves->emplace_back(AllocatedState::Cast(dest), phi->GetInput(predecessorId));
        } else {
            resolver->Add(AllocatedState::Cast(dest), AllocatedState::Cast(src));
        }

        auto target = AllocatedState::Cast(dest);
        if (target.IsRegister()) {
            registersSetByPhis->Set(target.GetRegister());
        } else if (target.IsDoubleRegister()) {
            doubleRegistersSetByPhis->Set(target.GetDoubleRegister());
        }
    }
}

void ArkSteedCodeGenerator::CollectRegisterStateMoves(GapMoveResolver *resolver, BB *successor, int predecessorId,
    const ArkSteedRegList &registersSetByPhis, const ArkDoubleRegList &doubleRegistersSetByPhis,
    ChunkVector<std::pair<AllocatedState, ValueVertex *>> *constantMoves)
{
    if (!successor->HasRegisterMerge()) {
        return;
    }
    RegisterMergeState &registerState = *successor->GetRegisterMergeState();
    // Exception handler blocks may have an uninitialized merge state since
    // they receive values from the frame/glue, not through registers.
    if (!registerState.IsInitialized()) {
        return;
    }
    registerState.ForEachGeneralRegister([&](ArkSteedRegister reg, RegisterState &state) {
        if (registersSetByPhis.Has(reg)) {
            return;
        }

        ValueVertex *vertex = nullptr;
        RegisterMergeInfo *mergeInfo = nullptr;
        if (!state.LoadMergeState(&vertex, &mergeInfo) || mergeInfo == nullptr) {
            return;
        }

        InstructionOperand src = mergeInfo->Operand(predecessorId);
        AllocatedState dest(LocationState::LocationKind::REGISTER, MachineRepresentation::Tagged, reg.Code());
        if (src.IsConstant()) {
            constantMoves->emplace_back(dest, vertex);
        } else if (src != dest) {
            resolver->Add(dest, AllocatedState::Cast(src));
        }
    });

    registerState.ForEachDoubleRegister([&](ArkSteedDoubleRegister reg, RegisterState &state) {
        if (doubleRegistersSetByPhis.Has(reg)) {
            return;
        }

        ValueVertex *vertex = nullptr;
        RegisterMergeInfo *mergeInfo = nullptr;
        if (!state.LoadMergeState(&vertex, &mergeInfo) || mergeInfo == nullptr) {
            return;
        }

        InstructionOperand src = mergeInfo->Operand(predecessorId);
        AllocatedState dest(LocationState::LocationKind::REGISTER, MachineRepresentation::Float64, reg.Code());
        ASSERT(!src.IsConstant());
        if (src != dest) {
            resolver->Add(dest, AllocatedState::Cast(src));
        }
    });
}

void ArkSteedCodeGenerator::ExecuteConstantPhiMove(const AllocatedState &dest, ValueVertex *constVertex,
                                                   const ArkSteedRegister *scratchGPR)
{
    ASSERT(constVertex != nullptr);
    if (dest.IsRegister()) {
        LoadConstantToRegister(constVertex, dest.GetRegister());
        return;
    }
    ASSERT(scratchGPR != nullptr);
    LoadConstantToRegister(constVertex, *scratchGPR);
    assembler_->MoveRepr(dest.GetRepresentation(), assembler_->ToMemOperand(dest), *scratchGPR);
}

void ArkSteedCodeGenerator::ExecuteGapMove(const InstructionOperand &dest, const InstructionOperand &src,
                                           const ArkSteedRegister *scratchGPR)
{
    ASSERT(dest.IsAllocated() && src.IsAllocated());
    const AllocatedState &destOp = AllocatedState::Cast(dest);
    const AllocatedState &srcOp = AllocatedState::Cast(src);

    // to do: Figure out why we have moves between different representations and whether we can avoid them.
    ASSERT((destOp.GetRepresentation() == srcOp.GetRepresentation()) ||
           ((destOp.GetRepresentation() == MachineRepresentation::Tagged ||
             destOp.GetRepresentation() == MachineRepresentation::Word64) &&
            (srcOp.GetRepresentation() == MachineRepresentation::Tagged ||
             srcOp.GetRepresentation() == MachineRepresentation::Word64)));

    MachineRepresentation repr = destOp.GetRepresentation();

    if (srcOp.IsRegister()) {
        if (destOp.IsRegister()) {
            assembler_->MoveRepr(repr, destOp.GetRegister(), srcOp.GetRegister());
        } else if (destOp.IsAnyStackSlot()) {
            assembler_->MoveRepr(repr, assembler_->ToMemOperand(destOp), srcOp.GetRegister());
        } else {
            UNREACHABLE();
        }
    } else if (srcOp.IsDoubleRegister()) {
        UNREACHABLE();
        // to do: Double Register
    } else if (srcOp.IsAnyStackSlot()) {
        ArkSteedAssembler::MemoryOperand srcMem = assembler_->ToMemOperand(srcOp);
        if (destOp.IsRegister()) {
            assembler_->MoveRepr(repr, destOp.GetRegister(), srcMem);
        } else if (destOp.IsDoubleRegister()) {
            UNREACHABLE();
            // to do: Double Register
        } else {
            ASSERT(destOp.IsAnyStackSlot());
            if (scratchGPR != nullptr) {
                assembler_->MoveRepr(repr, *scratchGPR, srcMem);
                assembler_->MoveRepr(repr, assembler_->ToMemOperand(destOp), *scratchGPR);
            } else {
                assembler_->MoveRepr(repr, assembler_->ToMemOperand(destOp), srcMem);
            }
        }
    }
}

// -------------------------------------------------------------------------
// Comment recording helpers
// -------------------------------------------------------------------------

void ArkSteedCodeGenerator::RecordComment(const char *msg)
{
    if (!assembler_->IsCommentEnabled()) {
        return;
    }
    assembler_->RecordComment(msg);
}

void ArkSteedCodeGenerator::RecordBlockComment(BB *block)
{
    if (!assembler_->IsCommentEnabled()) {
        return;
    }
    // Set current block color for subsequent IR lines
    SetCurrentBlockColor(block->GetId());
    std::ostringstream ss;
    ss << GetCurrentBlockColor() << "-- Block b" << block->GetId();

    // Print predecessors for merge blocks with their colors
    if (block->HasRegisterMerge()) {
        const auto &predecessors = block->GetPredecessors();
        if (!predecessors.empty()) {
            ss << " <-- [";
            for (size_t i = 0; i < predecessors.size(); ++i) {
                // Use predecessor's color for the block reference
                ss << GetBlockColor(predecessors[i]->GetId()) << "b" << predecessors[i]->GetId()
                   << GetCurrentBlockColor() << ", ";
            }
            ss << "]";
        }
    }
    ss << COLOR_RESET;

    RecordComment(ss.str().c_str());
}

void ArkSteedCodeGenerator::RecordVertexComment(Vertex *vertex)
{
    if (!assembler_->IsCommentEnabled()) {
        return;
    }
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    std::ostringstream ss;
    ss << GetCurrentBlockColor() << "--   ";
    if (labeller != nullptr) {
        std::string label = labeller->GetVertexLabel(vertex);
        // All vertices should be registered before code generation
        ASSERT(label != "<unregistered>");
        ss << label;
    } else {
        ss << "v" << vertex->GetId();
    }
    ss << ": " << OpcodeToString(vertex->GetOpcode());

    AppendVertexInputInfo(&ss, vertex);
    AppendVertexSuccessorInfo(&ss, vertex);
    ss << COLOR_RESET;

    RecordComment(ss.str().c_str());
}

void ArkSteedCodeGenerator::AppendVertexInputInfo(std::ostringstream *ss, Vertex *vertex)
{
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    int inputCount = vertex->GetInputCount();
    if (inputCount > 0) {
        *ss << " [";
        for (int i = 0; i < inputCount; i++) {
            if (i > 0) {
                *ss << ", ";
            }
            ValueVertex *input = vertex->GetInput(i);
            if (labeller != nullptr) {
                std::string inputLabel = labeller->GetVertexLabel(input);
                // All input vertices should also be registered
                ASSERT(inputLabel != "<unregistered>");
                *ss << inputLabel;
            } else {
                *ss << "v" << input->GetId();
            }
        }
        *ss << "]";
    }
}

void ArkSteedCodeGenerator::AppendVertexSuccessorInfo(std::ostringstream *ss, Vertex *vertex)
{
    if (!vertex->Is<ControlVertex>()) {
        return;
    }
    ControlVertex *control = vertex->Cast<ControlVertex>();
    if (auto *jump = control->TryCast<JumpVertex>(); jump != nullptr) {
        *ss << " --> [" << GetBlockColor(jump->Target()->GetId()) << "b" << jump->Target()->GetId()
            << GetCurrentBlockColor() << "]";
    } else if (auto *jumpLoop = control->TryCast<JumpLoopVertex>(); jumpLoop != nullptr) {
        *ss << " --> [" << GetBlockColor(jumpLoop->Target()->GetId()) << "b" << jumpLoop->Target()->GetId()
            << GetCurrentBlockColor() << "] (loop back)";
    } else if (auto *branch = control->TryCast<BranchIfTrueVertex>(); branch != nullptr) {
        uint32_t curBlockIndex = branch->GetOwner()->GetId();
        *ss << " --> [" << GetBlockColor(branch->IfTrue()->GetId()) << "b" << branch->IfTrue()->GetId();
        if (branch->IfTrue()->GetId() == curBlockIndex + 1) {
            *ss << " (fallthrough)";
        }
        *ss << GetCurrentBlockColor() << " if true, " << GetBlockColor(branch->IfFalse()->GetId()) << "b"
            << branch->IfFalse()->GetId();
        if (branch->IfFalse()->GetId() == curBlockIndex + 1) {
            *ss << " (fallthrough)";
        }
        *ss << GetCurrentBlockColor() << " if false]";
    }
}

void ArkSteedCodeGenerator::RecordGapMoveComment(const InstructionOperand &src, const InstructionOperand &dest,
                                                 PhiVertex *phi)
{
    if (!assembler_->IsCommentEnabled()) {
        return;
    }
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    std::ostringstream ss;
    ss << GetCurrentBlockColor() << "--   * " << src.Description() << " -> " << dest.Description();
    if (labeller != nullptr && phi != nullptr) {
        std::string label = labeller->GetVertexLabel(phi);
        // Phi vertices should be registered during graph building
        ASSERT(label != "<unregistered>");
        ss << " (" << label << ")";
    }
    ss << COLOR_RESET;
    RecordComment(ss.str().c_str());
}

void ArkSteedCodeGenerator::RecordSpillComment()
{
    if (assembler_->IsCommentEnabled()) {
        std::ostringstream ss;
        ss << GetCurrentBlockColor() << "--   Spill:" << COLOR_RESET;
        RecordComment(ss.str().c_str());
    }
}

// Get color index for a block - uses graph coloring if computed, otherwise falls back to id % NUM_BLOCK_COLORS
int ArkSteedCodeGenerator::GetBlockColorIndex(int blockId) const
{
    if (blockColorsComputed_ && blockId >= 0 && static_cast<size_t>(blockId) < blockColorAssignment_.size()) {
        return blockColorAssignment_[blockId];
    }
    // Fallback to simple modulo when graph coloring is not computed
    return blockId % NUM_BLOCK_COLORS;
}

// Compute graph coloring to ensure adjacent blocks have different colors
// Adjacent means: predecessor/successor relationship OR shared predecessor (branch targets)
// Only called when code comments are enabled to minimize JIT compilation overhead
void ArkSteedCodeGenerator::ComputeBlockColors()
{
    int numBlocks = graph_->NumBlocks();
    if (numBlocks <= 0) {
        return;
    }

    blockColorAssignment_.resize(numBlocks, -1);
    std::vector<std::vector<int>> adjacentBlocks;
    adjacentBlocks.resize(numBlocks);
    BuildBlockAdjacencyList(&adjacentBlocks);
    AssignBlockColors(adjacentBlocks);
    blockColorsComputed_ = true;
}

void ArkSteedCodeGenerator::BuildBlockAdjacencyList(std::vector<std::vector<int>> *adjacentBlocks)
{
    int numBlocks = graph_->NumBlocks();
    for (int i = 0; i < numBlocks; ++i) {
        BB *block = (*graph_)[i];
        int blockId = block->GetId();

        ControlVertex *control = block->GetControlVertex();
        if (control == nullptr) {
            continue;
        }

        ChunkVector<int> successorIds(graph_->GetChunk());
        if (auto *jump = control->TryCast<JumpVertex>(); jump != nullptr) {
            if (jump->Target() != nullptr) {
                successorIds.push_back(jump->Target()->GetId());
            }
        } else if (auto *jumpLoop = control->TryCast<JumpLoopVertex>(); jumpLoop != nullptr) {
            if (jumpLoop->Target() != nullptr) {
                successorIds.push_back(jumpLoop->Target()->GetId());
            }
        } else if (auto *branch = control->TryCast<BranchIfTrueVertex>(); branch != nullptr) {
            if (branch->IfTrue() != nullptr) {
                successorIds.push_back(branch->IfTrue()->GetId());
            }
            if (branch->IfFalse() != nullptr) {
                successorIds.push_back(branch->IfFalse()->GetId());
            }
        }

        for (int succId : successorIds) {
            if (succId != blockId) {
                (*adjacentBlocks)[blockId].push_back(succId);
                (*adjacentBlocks)[succId].push_back(blockId);
            }
        }

        for (size_t j = 0; j < successorIds.size(); ++j) {
            for (size_t k = j + 1; k < successorIds.size(); ++k) {
                int succ1 = successorIds[j];
                int succ2 = successorIds[k];
                if (succ1 != succ2) {
                    (*adjacentBlocks)[succ1].push_back(succ2);
                    (*adjacentBlocks)[succ2].push_back(succ1);
                }
            }
        }
    }
}

void ArkSteedCodeGenerator::AssignBlockColors(const std::vector<std::vector<int>> &adjacentBlocks)
{
    int numBlocks = graph_->NumBlocks();
    for (int i = 0; i < numBlocks; ++i) {
        BB *block = (*graph_)[i];
        int blockId = block->GetId();

        bool usedColors[NUM_BLOCK_COLORS] = {false};
        for (int adjId : adjacentBlocks[blockId]) {
            if (adjId >= 0 && static_cast<size_t>(adjId) < blockColorAssignment_.size() && adjId < blockId &&
                blockColorAssignment_[adjId] >= 0) {
                usedColors[blockColorAssignment_[adjId]] = true;
            }
        }

        int color = 0;
        while (color < NUM_BLOCK_COLORS && usedColors[color]) {
            ++color;
        }
        if (color >= NUM_BLOCK_COLORS) {
            color = color % NUM_BLOCK_COLORS;
        }

        blockColorAssignment_[blockId] = color;
    }
}

}  // namespace panda::ecmascript::arksteed
