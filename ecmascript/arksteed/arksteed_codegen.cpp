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
#include <limits>
#include <optional>
#include <sstream>

#include "ecmascript/arksteed/arksteed_assembler-inl.h"  // IWYU pragma: keep
#include "ecmascript/arksteed/arksteed_register_merge_state.h"
#include "ecmascript/arksteed/arksteed_safepoint_table.h"
#include "ecmascript/arksteed/arksteed_write_barrier.h"
#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_tagged_value_wrapper.h"

namespace panda::ecmascript::arksteed {
#define __ assembler_->

class GapMoveResolver {
    static constexpr uint8_t UNVISITED = 0;
    static constexpr uint8_t VISITING = 1;
    static constexpr uint8_t TEMP_REQUIRED = 2;
    static constexpr uint8_t DONE = 3;

public:
    using AssignmentPair = std::pair<AllocatedState, AllocatedState>;

    explicit GapMoveResolver(Chunk *chunk, ArkSteedRegister scratchGPR, ArkSteedDoubleRegister scratchFPR)
        : scratchGPR_(scratchGPR),
          scratchFPR_(scratchFPR),
          assignments_(chunk),
          reorderedAssignments_(chunk),
          sortedList_(chunk),
          sourceOperands_(chunk),
          hasSourceOperands_(chunk),
          edges_(chunk),
          adjLists_(chunk)
    {}

    void Add(AllocatedState dest, AllocatedState src)
    {
        if (SameStorage(dest, src)) {
            return;
        }
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
    struct AssignmentEdge {
        AllocatedState dest;
        AllocatedState src;
        uint32_t destIndex = 0;
        uint32_t srcIndex = 0;
    };

    static uint64_t StorageKey(AllocatedState operand)
    {
        ASSERT(operand.IsAllocated());
        constexpr uint64_t LOCATION_CLASS_SHIFT = 32;
        enum LocationClass : uint64_t {
            GP_REGISTER = 0,
            FP_REGISTER = 1,
            TAGGED_STACK_SLOT = 2,
            UNTAGGED_STACK_SLOT = 3,
        };

        uint64_t locationClass = UNTAGGED_STACK_SLOT;
        if (operand.IsRegister()) {
            locationClass = GP_REGISTER;
        } else if (operand.IsDoubleRegister()) {
            locationClass = FP_REGISTER;
        } else if (operand.GetRepresentation() == MachineRepresentation::Tagged) {
            ASSERT(operand.IsAnyStackSlot());
            locationClass = TAGGED_STACK_SLOT;
        } else {
            ASSERT(operand.IsAnyStackSlot());
        }
        return (locationClass << LOCATION_CLASS_SHIFT) | static_cast<uint32_t>(operand.GetIndex());
    }

    static bool SameStorage(AllocatedState lhs, AllocatedState rhs)
    {
        return StorageKey(lhs) == StorageKey(rhs);
    }

    static MachineRepresentation MergeStorageRepresentation(MachineRepresentation lhs, MachineRepresentation rhs)
    {
        if (lhs == rhs) {
            return lhs;
        }
        if (lhs == MachineRepresentation::Float64 || rhs == MachineRepresentation::Float64) {
            ASSERT(lhs == MachineRepresentation::Float64 && rhs == MachineRepresentation::Float64);
            return MachineRepresentation::Float64;
        }
        if (lhs == MachineRepresentation::Tagged || rhs == MachineRepresentation::Tagged) {
            return MachineRepresentation::Tagged;
        }
        if (lhs == MachineRepresentation::Word64 || rhs == MachineRepresentation::Word64) {
            return MachineRepresentation::Word64;
        }
        return MachineRepresentation::Word32;
    }

    static AllocatedState MergeSourceOperand(AllocatedState oldSource, AllocatedState newSource)
    {
        MachineRepresentation rep =
            MergeStorageRepresentation(oldSource.GetRepresentation(), newSource.GetRepresentation());
        return AllocatedState(oldSource.GetLocationKind(), rep, oldSource.GetIndex());
    }

    uint32_t SortedIndexOf(AllocatedState operand) const
    {
        uint64_t key = StorageKey(operand);
        auto compFn = [](AllocatedState x, uint64_t y) { return StorageKey(x) < y; };
        auto it = std::lower_bound(sortedList_.begin(), sortedList_.end(), key, compFn);
        ASSERT(it != sortedList_.end() && StorageKey(*it) == key);
        return static_cast<uint32_t>(it - sortedList_.begin());
    }

    void BuildSortedOperandList()
    {
        sortedList_.reserve(assignments_.size() * 2);  // 2: reserve space for both src and dest
        for (auto [dest, src] : assignments_) {
            sortedList_.push_back(dest);
            sortedList_.push_back(src);
        }
        std::sort(sortedList_.begin(), sortedList_.end(),
                  [](AllocatedState x, AllocatedState y) { return StorageKey(x) < StorageKey(y); });
        sortedList_.erase(std::unique(sortedList_.begin(), sortedList_.end(), SameStorage), sortedList_.end());
    }

    void BuildAdjacencyLists()
    {
        uint32_t n = static_cast<uint32_t>(sortedList_.size());
        sourceOperands_.resize(n);
        hasSourceOperands_.resize(n);
        edges_.reserve(assignments_.size());
        adjLists_.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            adjLists_.emplace_back(GetChunk());
        }
        for (auto [dest, src] : assignments_) {
            uint32_t destIndex = SortedIndexOf(dest);
            uint32_t srcIndex = SortedIndexOf(src);
            if (hasSourceOperands_[srcIndex] != 0) {
                sourceOperands_[srcIndex] = MergeSourceOperand(sourceOperands_[srcIndex], src);
            } else {
                sourceOperands_[srcIndex] = src;
                hasSourceOperands_[srcIndex] = 1;
            }
            edges_.push_back({dest, src, destIndex, srcIndex});
            adjLists_[srcIndex].push_back(static_cast<uint32_t>(edges_.size() - 1));
        }
    }

    void DFS(uint32_t index, ChunkVector<uint8_t> &states)
    {
        states[index] = VISITING;
        for (uint32_t edgeIndex : adjLists_[index]) {
            uint32_t destIndex = edges_[edgeIndex].destIndex;
            if (states[destIndex] == VISITING) {
                AllocatedState tempDest = ScratchOperandLike(ValueInStorage(destIndex));
                reorderedAssignments_.emplace_back(tempDest, ValueInStorage(destIndex));
                states[destIndex] = TEMP_REQUIRED;
            } else if (states[destIndex] == UNVISITED) {
                DFS(destIndex, states);
            } else {
                ASSERT(states[destIndex] == TEMP_REQUIRED || states[destIndex] == DONE);
            }
        }
        if (states[index] == TEMP_REQUIRED) {
            for (uint32_t edgeIndex : adjLists_[index]) {
                AllocatedState tempSrc = ScratchOperandLike(edges_[edgeIndex].src);
                reorderedAssignments_.emplace_back(edges_[edgeIndex].dest, tempSrc);
            }
            states[index] = DONE;
        } else {
            ASSERT(states[index] == VISITING);
            for (uint32_t edgeIndex : adjLists_[index]) {
                const AssignmentEdge &edge = edges_[edgeIndex];
                ASSERT(edge.dest.GetRepresentation() == edge.src.GetRepresentation());
                reorderedAssignments_.emplace_back(edge.dest, edge.src);
            }
            states[index] = DONE;
        }
    }

    AllocatedState ValueInStorage(uint32_t index) const
    {
        if (hasSourceOperands_[index] != 0) {
            return sourceOperands_[index];
        }
        return sortedList_[index];
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
    ChunkVector<AllocatedState> sourceOperands_;
    ChunkVector<uint8_t> hasSourceOperands_;
    ChunkVector<AssignmentEdge> edges_;
    ChunkVector<ChunkVector<uint32_t>> adjLists_;
};

namespace {

constexpr int CALL_ARG0 = 0;
constexpr int CALL_ARG1 = CALL_ARG0 + 1;
constexpr int CALL_ARG2 = CALL_ARG1 + 1;

ArkSteedRegister GetInputRegister(const Vertex *vertex, int index)
{
    const InputLocation *loc = vertex->GetInputLocation(index);
    ASSERT(loc->IsRegister());
    return loc->GetAssignedGeneralRegister();
}

ArkSteedDoubleRegister GetInputDoubleRegister(const Vertex *vertex, int index)
{
    const InputLocation *loc = vertex->GetInputLocation(index);
    ASSERT(loc->IsDoubleRegister());
    return loc->GetAssignedDoubleRegister();
}

ArkSteedRegister GetResultRegister(const ValueVertex *vertex)
{
    const ValueLocation &loc = vertex->Result();
    ASSERT(loc.IsRegister());
    return loc.GetAssignedGeneralRegister();
}

ArkSteedDoubleRegister GetResultDoubleRegister(const ValueVertex *vertex)
{
    const ValueLocation &loc = vertex->Result();
    ASSERT(loc.IsDoubleRegister());
    return loc.GetAssignedDoubleRegister();
}

std::optional<int32_t> TryGetInt32ConstantInput(const Vertex *vertex, int inputIndex)
{
    if (auto *constant = vertex->GetInput(inputIndex)->TryCast<Int32ConstantVertex>()) {
        return constant->GetValue();
    }
    return std::nullopt;
}

bool TryGetIntPtrConstant(const ValueVertex *vertex, intptr_t *value)
{
    if (vertex == nullptr || !vertex->Is<IntPtrConstantVertex>()) {
        return false;
    }
    *value = vertex->Cast<IntPtrConstantVertex>()->GetValue();
    return true;
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

kungfu::ARKDeopt MakeConstantDeopt(int32_t id, int64_t value)
{
    kungfu::ARKDeopt deopt;
    deopt.id = static_cast<kungfu::LLVMStackMapType::VRegId>(id);
    if (value > INT32_MAX || value < INT32_MIN) {
        deopt.kind = kungfu::LocationTy::Kind::CONSTANTNDEX;
        deopt.value = static_cast<kungfu::LLVMStackMapType::LargeInt>(value);
    } else {
        deopt.kind = kungfu::LocationTy::Kind::CONSTANT;
        deopt.value = static_cast<kungfu::LLVMStackMapType::IntType>(value);
    }
    return deopt;
}

int64_t GetConstantForDeopt(const ValueVertex *value, int32_t vregId)
{
    switch (value->GetOpcode()) {
        case VertexOpcode::TaggedConstant:
            return static_cast<int64_t>(value->Cast<TaggedConstantVertex>()->GetValue());
        case VertexOpcode::Int32Constant: {
            if (vregId == static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX) ||
                vregId == static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH)) {
                return value->Cast<Int32ConstantVertex>()->GetValue();
            }
            return static_cast<int64_t>(JSTaggedValue(value->Cast<Int32ConstantVertex>()->GetValue()).GetRawData());
        }
        case VertexOpcode::Int64Constant: {
            if (vregId == static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX) ||
                vregId == static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH)) {
                return static_cast<int64_t>(value->Cast<Int64ConstantVertex>()->GetValue());
            }
            int64_t rawValue = value->Cast<Int64ConstantVertex>()->GetValue();
            return static_cast<int64_t>(JSTaggedValue(static_cast<int>(rawValue)).GetRawData());
        }
        default:
            UNREACHABLE();
    }
}

void AppendDeoptInput(std::vector<kungfu::ARKDeopt> *deopts, const Vertex *vertex, int inputIndex,
                      int32_t vregId, ArkSteedAssembler *assembler_)
{
    const InputLocation *loc = vertex->GetInputLocation(inputIndex);
    const InstructionOperand &operand = loc->GetOperand();
    if (operand.IsConstant()) {
        deopts->emplace_back(MakeConstantDeopt(vregId, GetConstantForDeopt(vertex->GetInput(inputIndex), vregId)));
        return;
    }

    ASSERT(operand.IsAnyStackSlot());
    auto stackSlot = AllocatedState::Cast(operand);
    kungfu::ARKDeopt deoptValue;
    deoptValue.id = static_cast<kungfu::LLVMStackMapType::VRegId>(vregId);
    deoptValue.kind = kungfu::LocationTy::Kind::INDIRECT;
    int32_t offset =
        __ GetFramePointerOffsetForStackSlot(stackSlot.GetIndex(), stackSlot.GetRepresentation());
    deoptValue.value = std::make_pair(static_cast<kungfu::LLVMStackMapType::DwarfRegType>(GCStackMapRegisters::FP),
                                      static_cast<kungfu::LLVMStackMapType::OffsetType>(offset));
    deopts->emplace_back(deoptValue);
}

template <class NodeT>
void EmitUseSlotDeopt(ArkSteedAssembler *assembler_, ArkSteedSafepointTableBuilder *safepointBuilder,
                      const NodeT *vertex, kungfu::DeoptType type)
{
    ASSERT(safepointBuilder != nullptr);
    std::vector<kungfu::ARKDeopt> deopts;
    deopts.emplace_back(MakeConstantDeopt(static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH), 0));
    for (uint32_t index = 0; index < vertex->DeoptInputCount(); ++index) {
        AppendDeoptInput(&deopts, vertex, vertex->DeoptInputIndex(index),
                         vertex->GetDeoptVReg(index), assembler_);
    }
    __ CallDeoptHandler(type);
    safepointBuilder->DefineDeoptSafepoint(__ GetPcOffset(), std::move(deopts));
}

Condition ConditionFromIntCondition(IntConditionKind condition)
{
    switch (condition) {
        case IntConditionKind::EQUAL:
            return Condition::COND_EQUAL;
        case IntConditionKind::NOT_EQUAL:
            return Condition::COND_NOT_EQUAL;
        case IntConditionKind::LESS_THAN:
            return Condition::COND_LESS_THAN;
        case IntConditionKind::LESS_THAN_OR_EQUAL:
            return Condition::COND_LESS_THAN_OR_EQUAL;
        case IntConditionKind::GREATER_THAN:
            return Condition::COND_GREATER_THAN;
        case IntConditionKind::GREATER_THAN_OR_EQUAL:
            return Condition::COND_GREATER_THAN_OR_EQUAL;
        default:
            UNREACHABLE();
    }
}

void EmitTaggedBooleanFromCondition(ArkSteedAssembler *assembler_, ArkSteedRegister dst, Condition trueCondition)
{
    Label trueLabel;
    Label done;
    __ JumpIf(trueCondition, &trueLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::False().GetRawData());
    __ Jump(&done);
    __ Bind(&trueLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::True().GetRawData());
    __ Bind(&done);
}

Condition OrderedFloat64ConditionFromInt32Condition(IntConditionKind condition)
{
    switch (condition) {
        case IntConditionKind::EQUAL:
            return Condition::COND_EQUAL;
        case IntConditionKind::NOT_EQUAL:
            return Condition::COND_NOT_EQUAL;
        case IntConditionKind::LESS_THAN:
            return Condition::COND_BELOW;
        case IntConditionKind::LESS_THAN_OR_EQUAL:
            return Condition::COND_BELOW_OR_EQUAL;
        case IntConditionKind::GREATER_THAN:
            return Condition::COND_ABOVE;
        case IntConditionKind::GREATER_THAN_OR_EQUAL:
            return Condition::COND_ABOVE_OR_EQUAL;
        default:
            UNREACHABLE();
    }
}

void EmitTaggedBooleanFromFloat64Compare(ArkSteedAssembler *assembler_, ArkSteedRegister dst,
                                         IntConditionKind condition)
{
    Label trueLabel;
    Label falseLabel;
    Label done;
    if (condition == IntConditionKind::NOT_EQUAL) {
        __ JumpIf(Condition::COND_PARITY, &trueLabel);
        __ JumpIf(OrderedFloat64ConditionFromInt32Condition(condition), &trueLabel);
        __ Jump(&falseLabel);
    } else {
        __ JumpIf(Condition::COND_PARITY, &falseLabel);
        __ JumpIf(OrderedFloat64ConditionFromInt32Condition(condition), &trueLabel);
        __ Jump(&falseLabel);
    }
    __ Bind(&trueLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::True().GetRawData());
    __ Jump(&done);
    __ Bind(&falseLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::False().GetRawData());
    __ Bind(&done);
}

void BranchOnFloat64Compare(ArkSteedAssembler *assembler_, IntConditionKind condition, Label *ifTrue, Label *ifFalse)
{
    if (condition == IntConditionKind::NOT_EQUAL) {
        __ JumpIf(Condition::COND_PARITY, ifTrue);
        __ JumpIf(OrderedFloat64ConditionFromInt32Condition(condition), ifTrue);
        __ Jump(ifFalse);
        return;
    }
    __ JumpIf(Condition::COND_PARITY, ifFalse);
    __ JumpIf(OrderedFloat64ConditionFromInt32Condition(condition), ifTrue);
    __ Jump(ifFalse);
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
        case VertexOpcode::Int32Constant:
            constVertex->Cast<Int32ConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::Int64Constant:
            constVertex->Cast<Int64ConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        case VertexOpcode::TaggedConstant:
            constVertex->Cast<TaggedConstantVertex>()->DoLoadToRegister(assembler_, reg);
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedCodeGenerator::LoadConstantToDoubleRegister(const ValueVertex *constVertex, ArkSteedDoubleRegister reg)
{
    ASSERT(constVertex != nullptr);
    switch (constVertex->GetOpcode()) {
        case VertexOpcode::Float64Constant:
            constVertex->Cast<Float64ConstantVertex>()->DoLoadToRegister(assembler_, reg);
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
        __ MoveRepr(MachineRepresentation::Tagged, destMem, src);
    } else if (operand.IsAnyStackSlot()) {
        ArkSteedAssembler::MemoryOperand srcMem = __ ToMemOperand(operand);
        __ MoveRepr(MachineRepresentation::Tagged, destMem, srcMem);
    } else if (operand.IsConstant()) {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        LoadConstantToRegister(callVertex->Arg(paramIdx).vertex(), scratch);
        __ MoveRepr(MachineRepresentation::Tagged, destMem, scratch);
    } else {
        UNREACHABLE();
    }
}

int ArkSteedCodeGenerator::PrepareCommonStubStackArguments(const Vertex *callVertex, int argCount)
{
    const int stackArgCount = std::max(0, argCount - ArkSteedAssembler::NUM_ARG_REGISTERS);
    // Keep the stack 16-byte aligned at each call site by reserving an even number of slots.
    const int reservedSlotCount = (stackArgCount + 1) & ~1;  // ~1: round down to even number (2-slot alignment)
    __ ReserveCallArgSlots(reservedSlotCount);
    for (int paramIdx = ArkSteedAssembler::NUM_ARG_REGISTERS; paramIdx < argCount; paramIdx++) {
        int stackSlotIdx = paramIdx - ArkSteedAssembler::NUM_ARG_REGISTERS;
        ArkSteedAssembler::MemoryOperand destMem = __ GetCallArgSlot(stackSlotIdx);
        StoreStubStackArgument(callVertex, paramIdx, destMem);
    }
    if (reservedSlotCount > stackArgCount) {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(JSTaggedValue::VALUE_UNDEFINED));
        __ MoveRepr(MachineRepresentation::Tagged, __ GetCallArgSlot(stackArgCount), scratch);
    }
    return reservedSlotCount;
}

int ArkSteedCodeGenerator::PrepareRuntimeStubStackArguments(const Vertex *callVertex, int argCount, int runtimeId)
{
    const int stackArgCount = argCount + CALL_ARG2;
    const int reservedSlotCount = (stackArgCount + 1) & ~1;  // ~1: round down to even number (2-slot alignment)
    __ ReserveCallArgSlots(reservedSlotCount);

    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(runtimeId));
        __ MoveRepr(MachineRepresentation::Word64,
                             __ GetCallArgSlot(CALL_ARG0), scratch);
        __ Move(scratch, static_cast<int64_t>(argCount));
        __ MoveRepr(MachineRepresentation::Word64,
                             __ GetCallArgSlot(CALL_ARG1), scratch);
    }

    for (int paramIdx = 0; paramIdx < argCount; paramIdx++) {
        ArkSteedAssembler::MemoryOperand destMem =
            __ GetCallArgSlot(paramIdx + CALL_ARG2);
        StoreStubStackArgument(callVertex, paramIdx, destMem);
    }
    return reservedSlotCount;
}

void ArkSteedCodeGenerator::LoadSteedExpectedArgc(ArkSteedRegister target, ArkSteedRegister expectedArgc)
{
    __ LoadField(expectedArgc, target, JSFunction::METHOD_OFFSET);
    __ LoadField(expectedArgc, expectedArgc, Method::CALL_FIELD_OFFSET);
    __ Lsr(expectedArgc, Method::NumArgsBits::START_BIT);
    __ And(expectedArgc, static_cast<int64_t>(
        Method::NumArgsBits::Mask() >> Method::NumArgsBits::START_BIT));
}

void ArkSteedCodeGenerator::ComputeSteedCallSlotCount(CallVertex *call, ArkSteedRegister slotCount)
{
    uint32_t userArgc = call->GetActualArgc();

    Label countDone;
    __ Compare(slotCount, static_cast<int32_t>(userArgc));
    __ JumpIf(Condition::COND_GREATER_THAN, &countDone);
    __ Move(slotCount, static_cast<int32_t>(userArgc));

    __ Bind(&countDone);
    __ Add(slotCount, NUM_MANDATORY_JSFUNC_ARGS + 1);
    __ Add(slotCount, 1);
    __ And(slotCount, ~1ULL);
}

void ArkSteedCodeGenerator::PrepareArkSteedCall(CallVertex *call, ArkSteedRegister target)
{
    const uint32_t userArgc = call->GetActualArgc();
    const uint32_t totalArgc = userArgc + NUM_MANDATORY_JSFUNC_ARGS;

    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        LoadSteedExpectedArgc(target, scratch);
        ComputeSteedCallSlotCount(call, scratch);
        __ ReserveCallArgSlots(scratch);
        __ Sub(scratch, static_cast<int32_t>(NUM_MANDATORY_JSFUNC_ARGS + 1 + userArgc));
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG0),
                             scratch);
    }

    StoreStubStackArgument(call, CallVertex::TARGET_INDEX,
                           __ GetCallArgSlot(CallVertex::TARGET_INDEX + CALL_ARG1));
    StoreStubStackArgument(call, CallVertex::NEW_TARGET_INDEX,
                           __ GetCallArgSlot(CallVertex::NEW_TARGET_INDEX + CALL_ARG1));
    StoreStubStackArgument(call, CallVertex::THIS_INDEX,
                           __ GetCallArgSlot(CallVertex::THIS_INDEX + CALL_ARG1));
    for (uint32_t i = 0; i < userArgc; i++) {
        StoreStubStackArgument(call, CallVertex::FIRST_ARG_INDEX + i,
                               __ GetCallArgSlot(CallVertex::FIRST_ARG_INDEX +
                                                          CALL_ARG1 + i));
    }

    __ MoveRepr(MachineRepresentation::Word64, target, __ GetCallArgSlot(CALL_ARG0));
    __ PushUndefinedForSteedCall(target, userArgc);
    __ Move(target, static_cast<uint64_t>(totalArgc));
    __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG0), target);
    __ MoveRepr(MachineRepresentation::Tagged, target,
                         __ GetCallArgSlot(CallVertex::TARGET_INDEX + CALL_ARG1));
}

void ArkSteedCodeGenerator::FreeArkSteedCallFrame(CallVertex *call)
{
    (void)call;
    __ RestoreStackPointerToFrameBottom(graph_);
}

void ArkSteedCodeGenerator::EmitCallArkSteed(CallVertex *call, ArkSteedRegister target, Label *exit)
{
    PrepareArkSteedCall(call, target);
    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister codeEntry = scope.AcquireScratch();
        __ PrepareSteedCalleeContext(target, codeEntry);
        __ Call(codeEntry);
    }
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    FreeArkSteedCallFrame(call);
    __ Jump(exit);
}

void ArkSteedCodeGenerator::EmitCallGeneric(CallVertex *call)
{
    int stackArgCount = PrepareTrampolineArguments(call);
    __ CallTrampoline(RTSTUB_ID(JSCall));
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ FreeCallArgSlots(stackArgCount);
}

int ArkSteedCodeGenerator::PrepareTrampolineArguments(CallVertex *call)
{
    uint32_t userArgc = call->GetActualArgc();
    uint32_t totalArgc = userArgc + NUM_MANDATORY_JSFUNC_ARGS;
    // stack layout: totalArgc, actualArgV, target, newTarget, this, userArgs
    uint32_t stackArgCount = totalArgc + CALL_ARG2;
    uint32_t reservedSlotCount = (stackArgCount + 1) & ~1U;
    __ ReserveCallArgSlots(static_cast<int32_t>(reservedSlotCount));

    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(totalArgc));
        __ MoveRepr(MachineRepresentation::Word64,
                             __ GetCallArgSlot(CALL_ARG0), scratch);
        __ Move(scratch, 0);
        __ MoveRepr(MachineRepresentation::Word64,
                             __ GetCallArgSlot(CALL_ARG1), scratch);
    }

    StoreStubStackArgument(call, CallVertex::TARGET_INDEX,
                           __ GetCallArgSlot(CallVertex::TARGET_INDEX + CALL_ARG2));
    StoreStubStackArgument(call, CallVertex::NEW_TARGET_INDEX,
                           __ GetCallArgSlot(CallVertex::NEW_TARGET_INDEX +
                                                      CALL_ARG2));
    StoreStubStackArgument(call, CallVertex::THIS_INDEX,
                           __ GetCallArgSlot(CallVertex::THIS_INDEX + CALL_ARG2));
    for (uint32_t i = 0; i < userArgc; i++) {
        StoreStubStackArgument(call, CallVertex::FIRST_ARG_INDEX + i,
                               __ GetCallArgSlot(CallVertex::FIRST_ARG_INDEX +
                                                          CALL_ARG2 + i));
    }
    return static_cast<int>(reservedSlotCount);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CallRuntimeVertex>(CallRuntimeVertex *callRuntime)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << callRuntime->GetId() << ": CallRuntimeVertex";
#endif

    int stackArgCount = PrepareRuntimeStubStackArguments(callRuntime,
                                                         callRuntime->GetArgCount(),
                                                         static_cast<int>(callRuntime->GetRuntimeStubID()));
    __ CallRuntime(callRuntime->GetRuntimeStubID());
    __ FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CallVertex>(CallVertex *call)
{
    Label callGeneric;
    Label exit;
    ArkSteedRegister target = GetInputRegister(call, CallVertex::TARGET_INDEX);
    __ JumpIfNotTaggedHeapObject(target, &callGeneric);
    __ JumpIfNotJSFunction(target, &callGeneric);
    __ JumpIfClassConstructor(target, &callGeneric);
    __ JumpIfFunctionNotCompiled(target, &callGeneric);
    EmitCallArkSteed(call, target, &exit);
    __ Bind(&callGeneric);
    EmitCallGeneric(call);
    __ Bind(&exit);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfHClassMismatchVertex>(DeoptIfHClassMismatchVertex *checkHClass)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << checkHClass->GetId() << ": DeoptIfHClassMismatchVertex";
#endif
    constexpr int RECEIVER_INDEX = static_cast<int>(DeoptIfHClassMismatchVertex::RECEIVER_INDEX);
    ASSERT(safepointBuilder_ != nullptr);
    auto temporaries = checkHClass->GetRegallocInfo()->GetGeneralTemporaries();
    ArkSteedRegister actualHClass = temporaries.First();
    temporaries.PopFirst();
    ArkSteedRegister expectedHClass = temporaries.First();
    ArkSteedRegister receiver = GetInputRegister(checkHClass, RECEIVER_INDEX);
    Label deopt;
    Label pass;
    __ Move(actualHClass, receiver);
    __ Move(expectedHClass, static_cast<uint64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ And(actualHClass, expectedHClass);
    __ Compare(actualHClass, 0);
    __ JumpIf(Condition::COND_NOT_EQUAL, &deopt);

    __ LoadField(actualHClass, receiver, TaggedObject::HCLASS_OFFSET);
    __ Move(expectedHClass, TaggedStateWord::ADDRESS_MASK);
    __ And(actualHClass, expectedHClass);
    __ Move(expectedHClass, reinterpret_cast<uint64_t>(checkHClass->GetExpectedHClass()) &
                                    TaggedStateWord::ADDRESS_MASK);
    __ Compare(actualHClass, expectedHClass);
    __ JumpIf(Condition::COND_EQUAL, &pass);

    __ Bind(&deopt);
    std::vector<kungfu::ARKDeopt> deopts;
    deopts.emplace_back(MakeConstantDeopt(static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH), 0));
    for (int index = 0; index < static_cast<int>(checkHClass->GetDeoptVRegs().size()); index++) {
        AppendDeoptInput(&deopts, checkHClass, RECEIVER_INDEX + 1 + index,
                         checkHClass->GetDeoptVReg(static_cast<VRegIDType>(index)), assembler_);
    }
    __ CallDeoptHandler(kungfu::DeoptType::KEYMISSMATCH);
    safepointBuilder_->DefineDeoptSafepoint(__ GetPcOffset(), std::move(deopts));

    __ Bind(&pass);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfInt32ConditionVertex>(DeoptIfInt32ConditionVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": DeoptIfInt32ConditionVertex";
#endif
    auto left = GetInputRegister(check, DeoptIfInt32ConditionVertex::LEFT_INDEX);
    auto right = GetInputRegister(check, DeoptIfInt32ConditionVertex::RIGHT_INDEX);
    Label deopt;
    Label done;
    __ CompareInt32(left, right);
    __ JumpIf(ConditionFromIntCondition(check->GetCondition()), &deopt);
    __ Jump(&done);
    __ Bind(&deopt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, check, check->GetDeoptType());
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfNotNumberVertex>(DeoptIfNotNumberVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": DeoptIfNotNumberVertex";
#endif
    auto value = GetInputRegister(check, DeoptIfNotNumberVertex::VALUE_INDEX);
    auto temporaries = check->GetRegallocInfo()->GetGeneralTemporaries();
    ArkSteedRegister bits = temporaries.First();
    temporaries.PopFirst();
    ArkSteedRegister scratch = temporaries.First();
    Label deopt;
    Label done;

    __ Move(bits, value);
    __ Move(scratch, JSTaggedValue::TAG_MARK);
    __ Word64And(bits, scratch);
    __ Compare(bits, scratch);
    __ JumpIf(Condition::COND_EQUAL, &done);

    __ Move(scratch, static_cast<uint64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ Compare(value, scratch);
    __ JumpIf(Condition::COND_BELOW, &deopt);
    __ Move(scratch, static_cast<uint64_t>(JSTaggedValue::TAG_INT));
    __ Compare(value, scratch);
    __ JumpIf(Condition::COND_BELOW, &done);
    __ Jump(&deopt);

    __ Bind(&deopt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, check, kungfu::DeoptType::NOTNUMBER1);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptVertex>(DeoptVertex *deopt)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << deopt->GetId() << ": DeoptVertex";
#endif
    ASSERT(safepointBuilder_ != nullptr);
    std::vector<kungfu::ARKDeopt> deopts;
    deopts.emplace_back(MakeConstantDeopt(static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH), 0));
    for (uint32_t index = 0, n = deopt->GetInputCount(); index < n; index++) {
        AppendDeoptInput(&deopts, deopt, index, deopt->GetDeoptVReg(static_cast<VRegIDType>(index)), assembler_);
    }
    __ CallDeoptHandler(deopt->GetDeoptType());
    safepointBuilder_->DefineDeoptSafepoint(__ GetPcOffset(), std::move(deopts));
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
    __ LoadActualArgc(dst);
}

// ========================================= Slow Value Opcode =========================================

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadTaggedFromAddressVertex>(LoadTaggedFromAddressVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadTaggedFromAddressVertex";
#endif
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadTaggedFromAddressVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadI32FromAddressVertex>(LoadI32FromAddressVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadI32FromAddressVertex";
#endif
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadI32FromAddressVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadI64FromAddressVertex>(LoadI64FromAddressVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadI64FromAddressVertex";
#endif
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadI64FromAddressVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadF64FromAddressVertex>(LoadF64FromAddressVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadF64FromAddressVertex";
#endif
    auto dst = GetResultDoubleRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadF64FromAddressVertex::OBJECT_INDEX);
    __ LoadFloat64(dst, ArkSteedAssembler::MemoryOperand(obj, loadField->GetOffset()));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadExceptionVertex>(LoadExceptionVertex *loadException)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadException->GetId() << ": LoadExceptionVertex";
#endif
    auto dst = GetResultRegister(loadException);
    auto glue = GetInputRegister(loadException, LoadExceptionVertex::GLUE_INDEX);
    __ LoadAndClearPendingException(dst, glue);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadTaggedFieldVertex>(LoadTaggedFieldVertex *loadField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadField->GetId() << ": LoadTaggedFieldVertex";
#endif
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadTaggedFieldVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedToAddressVertex>(StoreTaggedToAddressVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreTaggedToAddressVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreTaggedToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedToAddressVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreI32ToAddressVertex>(StoreI32ToAddressVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreI32ToAddressVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreI32ToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreI32ToAddressVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreI64ToAddressVertex>(StoreI64ToAddressVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreI64ToAddressVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreI64ToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreI64ToAddressVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreF64ToAddressVertex>(StoreF64ToAddressVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreF64ToAddressVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreF64ToAddressVertex::OBJECT_INDEX);
    auto value = GetInputDoubleRegister(storeField, StoreF64ToAddressVertex::VALUE_INDEX);
    __ StoreFloat64(ArkSteedAssembler::MemoryOperand(obj, storeField->GetOffset()), value);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedFieldVertex>(StoreTaggedFieldVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreTaggedFieldVertex";
#endif
    auto obj = GetInputRegister(storeField, StoreTaggedFieldVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedFieldVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedFieldWithBarrierVertex>(
    StoreTaggedFieldWithBarrierVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId()
                        << ": StoreTaggedFieldWithBarrierVertex";
#endif
    auto glue = GetInputRegister(storeField, StoreTaggedFieldWithBarrierVertex::GLUE_INDEX);
    auto object = GetInputRegister(storeField, StoreTaggedFieldWithBarrierVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedFieldWithBarrierVertex::VALUE_INDEX);
    auto offsetScratch = ArkSteedAssembler::GetParameterRegister(2);
    auto temporaryRegisters = storeField->GetRegallocInfo()->GetGeneralTemporaries();
    ArkSteedRegister primaryScratch = temporaryRegisters.PopFirst();
    ArkSteedRegister secondaryScratch = temporaryRegisters.PopFirst();
    ASSERT(primaryScratch != offsetScratch);
    ASSERT(secondaryScratch != offsetScratch);
    ASSERT(primaryScratch != secondaryScratch);
    ArkSteedWriteBarrierEmitter(assembler_)
        .StoreTaggedField(glue, object, value, storeField->GetOffset(), ArkSteedWriteBarrierKind::GENERIC_BARRIER,
                          primaryScratch, offsetScratch, secondaryScratch, storeField->GetValueKind());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreSharedFieldWithBarrierVertex>(
    StoreSharedFieldWithBarrierVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId()
                        << ": StoreSharedFieldWithBarrierVertex";
#endif
    auto glue = GetInputRegister(storeField, StoreSharedFieldWithBarrierVertex::GLUE_INDEX);
    auto object = GetInputRegister(storeField, StoreSharedFieldWithBarrierVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreSharedFieldWithBarrierVertex::VALUE_INDEX);
    auto offsetScratch = ArkSteedAssembler::GetParameterRegister(2);
    auto temporaryRegisters = storeField->GetRegallocInfo()->GetGeneralTemporaries();
    ArkSteedRegister primaryScratch = temporaryRegisters.PopFirst();
    ArkSteedRegister secondaryScratch = temporaryRegisters.PopFirst();
    ASSERT(primaryScratch != offsetScratch);
    ASSERT(secondaryScratch != offsetScratch);
    ASSERT(primaryScratch != secondaryScratch);
    ArkSteedWriteBarrierEmitter(assembler_)
        .StoreTaggedField(glue, object, value, storeField->GetOffset(), ArkSteedWriteBarrierKind::SHARED_BARRIER,
                          primaryScratch, offsetScratch, secondaryScratch, storeField->GetValueKind());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreEnvSlotVertex>(StoreEnvSlotVertex *storeEnvSlot)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeEnvSlot->GetId() << ": StoreEnvSlotVertex";
#endif
    auto env = GetInputRegister(storeEnvSlot, StoreEnvSlotVertex::ENV_INDEX);
    auto value = GetInputRegister(storeEnvSlot, StoreEnvSlotVertex::VALUE_INDEX);
    __ StoreField(value, env, storeEnvSlot->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<SetValueWithBarrierVertex>(
    SetValueWithBarrierVertex *setValueWithBarrier)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << setValueWithBarrier->GetId() << ": SetValueWithBarrierVertex";
#endif
    auto value = GetInputRegister(setValueWithBarrier, SetValueWithBarrierVertex::VALUE_INDEX);
    Label done;
    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ And(scratch, value);
        __ Compare(scratch, 0);
        __ JumpIf(Condition::COND_NOT_ZERO, &done);
    }
    __ Move(ArkSteedAssembler::GetParameterRegister(2), static_cast<int64_t>(setValueWithBarrier->GetOffset()));
    __ CallCommonStub(kungfu::CommonStubCSigns::SetValueWithBarrier);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ Bind(&done);
}

// ========================================= Non-Value Opcode =========================================

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<GapMoveVertex>(GapMoveVertex *gapMove)
{
    const AllocatedState &source = gapMove->GetSource();
    const AllocatedState &target = gapMove->GetTarget();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting GapMoveVertex: " << target.Description() << " <- "
                        << source.Description();
#endif
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratchGPR = scope.AcquireScratch();
    ExecuteGapMove(target, source, &scratchGPR);
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
void ArkSteedCodeGenerator::VisitNonControlVertex<I32ToTaggedIntVertex>(I32ToTaggedIntVertex *toTaggedInt)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << toTaggedInt->GetId() << ": I32ToTaggedIntVertex";
#endif
    auto dst = GetResultRegister(toTaggedInt);
    auto src = GetInputRegister(toTaggedInt, I32ToTaggedIntVertex::INPUT_INDEX);
    ArkSteedRegister scratch = toTaggedInt->GetRegallocInfo()->GetGeneralTemporaries().First();
    __ SignExtendInt32ToInt64(dst, src);
    __ Move(scratch, static_cast<int64_t>(JSTaggedValue::TAG_INT));
    __ Or(dst, scratch);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<RawI64ToTaggedVertex>(RawI64ToTaggedVertex *toTagged)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << toTagged->GetId() << ": RawI64ToTaggedVertex";
#endif
    auto dst = GetResultRegister(toTagged);
    auto src = GetInputRegister(toTagged, RawI64ToTaggedVertex::INPUT_INDEX);
    if (dst != src) {
        __ Move(dst, src);
    }
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedToRawI64Vertex>(TaggedToRawI64Vertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": TaggedToRawI64Vertex";
#endif
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, TaggedToRawI64Vertex::INPUT_INDEX);
    if (dst != src) {
        __ Move(dst, src);
    }
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I64BitwiseBinaryVertex>(I64BitwiseBinaryVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I64BitwiseBinaryVertex";
#endif
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, I64BitwiseBinaryVertex::LEFT_INDEX);
    if (dst != left) {
        __ Move(dst, left);
    }
    auto right = GetInputRegister(op, I64BitwiseBinaryVertex::RIGHT_INDEX);
    switch (op->GetKind()) {
        case IntBitwiseKind::BITWISE_AND:
            __ And(dst, right);
            break;
        case IntBitwiseKind::BITWISE_OR:
            __ Or(dst, right);
            break;
        default:
            UNREACHABLE();
    }
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedIntToI32Vertex>(TaggedIntToI32Vertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": TaggedIntToI32Vertex";
#endif
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, TaggedIntToI32Vertex::INPUT_INDEX);
    __ SignExtendInt32ToInt64(dst, src);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedTaggedIntToI32Vertex>(CheckedTaggedIntToI32Vertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": CheckedTaggedIntToI32Vertex";
#endif
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, CheckedTaggedIntToI32Vertex::INPUT_INDEX);
    ArkSteedRegister scratch = convert->GetRegallocInfo()->GetGeneralTemporaries().First();
    Label deopt;
    Label done;

    __ Move(scratch, JSTaggedValue::TAG_MARK);
    __ Move(dst, src);
    __ Word64And(dst, scratch);
    __ Compare(dst, scratch);
    __ JumpIf(Condition::COND_NOT_EQUAL, &deopt);
    __ SignExtendInt32ToInt64(dst, src);
    __ Jump(&done);

    __ Bind(&deopt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, convert, kungfu::DeoptType::NOTINT1);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedTaggedStringVertex>(CheckedTaggedStringVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": CheckedTaggedStringVertex";
#endif
    auto value = GetInputRegister(check, CheckedTaggedStringVertex::INPUT_INDEX);
    ASSERT(GetResultRegister(check) == value);
    ArkSteedRegister scratch = check->GetRegallocInfo()->GetGeneralTemporaries().First();
    Label deopt;
    Label done;

    __ JumpIfNotTaggedHeapObject(value, &deopt);
    __ LoadField(scratch, value, TaggedObject::HCLASS_OFFSET);
    __ And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    __ LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    __ And(scratch, (1U << JSHClass::TYPE_BITFIELD_NUM) - 1);
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_FIRST));
    __ JumpIf(Condition::COND_LESS_THAN, &deopt);
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_LAST));
    __ JumpIf(Condition::COND_GREATER_THAN, &deopt);
    __ Jump(&done);

    __ Bind(&deopt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, check, kungfu::DeoptType::NOTSTRING1);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTaggedStringVertex>(BranchIfTaggedStringVertex *jumpIf)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId()
                        << ": BranchIfTaggedStringVertex to BB #" << jumpIf->IfTrue()->GetId()
                        << " if true; to BB #" << jumpIf->IfFalse()->GetId() << " if false.";
#endif
    auto value = GetInputRegister(jumpIf, BranchIfTaggedStringVertex::VALUE_INDEX);
    ArkSteedRegister scratch = jumpIf->GetRegallocInfo()->GetGeneralTemporaries().First();
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();

    __ JumpIfNotTaggedHeapObject(value, ifFalse->GetLabel());
    __ LoadField(scratch, value, TaggedObject::HCLASS_OFFSET);
    __ And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    __ LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    __ And(scratch, (1U << JSHClass::TYPE_BITFIELD_NUM) - 1);
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_FIRST));
    __ JumpIf(Condition::COND_LESS_THAN, ifFalse->GetLabel());
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_LAST));
    __ JumpIf(Condition::COND_GREATER_THAN, ifFalse->GetLabel());
    __ Jump(ifTrue->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32ConditionCheckVertex>(I32ConditionCheckVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": I32ConditionCheckVertex";
#endif
    auto dst = GetResultRegister(check);
    auto left = GetInputRegister(check, I32ConditionCheckVertex::LEFT_INDEX);
    auto right = GetInputRegister(check, I32ConditionCheckVertex::RIGHT_INDEX);
    __ CompareInt32(left, right);
    EmitTaggedBooleanFromCondition(assembler_, dst, ConditionFromIntCondition(check->GetCondition()));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ConditionCheckVertex>(F64ConditionCheckVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": F64ConditionCheckVertex";
#endif
    auto dst = GetResultRegister(check);
    auto left = GetInputDoubleRegister(check, F64ConditionCheckVertex::LEFT_INDEX);
    auto right = GetInputDoubleRegister(check, F64ConditionCheckVertex::RIGHT_INDEX);
    __ CompareFloat64(left, right);
    EmitTaggedBooleanFromFloat64Compare(assembler_, dst, check->GetCondition());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedEqualVertex>(TaggedEqualVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": TaggedEqualVertex";
#endif
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, TaggedEqualVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, TaggedEqualVertex::RIGHT_INDEX);
    __ Compare(left, right);
    EmitTaggedBooleanFromCondition(assembler_, dst, Condition::COND_EQUAL);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedNotEqualVertex>(TaggedNotEqualVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": TaggedNotEqualVertex";
#endif
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, TaggedNotEqualVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, TaggedNotEqualVertex::RIGHT_INDEX);
    __ Compare(left, right);
    EmitTaggedBooleanFromCondition(assembler_, dst, Condition::COND_NOT_EQUAL);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StringEqualVertex>(StringEqualVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": StringEqualVertex";
#endif
    auto dst = GetResultRegister(op);
    int stackArgCount = PrepareCommonStubStackArguments(op, op->GetInputCount());
    __ CallCommonStub(kungfu::CommonStubCSigns::FastStringEqual);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ FreeCallArgSlots(stackArgCount);
    __ And(dst, 1);

    Label falseLabel;
    Label done;
    __ Compare(dst, 0);
    __ JumpIf(Condition::COND_EQUAL, &falseLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::True().GetRawData());
    __ Jump(&done);
    __ Bind(&falseLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::False().GetRawData());
    __ Bind(&done);
}

#define DEFINE_I32_WITH_OVERFLOW_CODEGEN(Name, Op)                                                           \
    template <>                                                                                               \
    void ArkSteedCodeGenerator::VisitNonControlVertex<I32##Name##WithOverflowVertex>(                         \
        I32##Name##WithOverflowVertex *op)                                                                    \
    {                                                                                                         \
        auto dst = GetResultRegister(op);                                                                       \
        auto right = GetInputRegister(op, I32##Name##WithOverflowVertex::RIGHT_INDEX);                        \
        ASSERT(dst == GetInputRegister(op, I32##Name##WithOverflowVertex::LEFT_INDEX));                       \
        Label deopt;                                                                                            \
        Label done;                                                                                             \
        __ Op(dst, right);                                                                             \
        __ JumpIf(Condition::COND_OVERFLOW, &deopt);                                                   \
        __ Jump(&done);                                                                                \
        __ Bind(&deopt);                                                                               \
        EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::INT32OVERFLOW1);                   \
        __ Bind(&done);                                                                                \
    }

DEFINE_I32_WITH_OVERFLOW_CODEGEN(Add, Int32Add)
DEFINE_I32_WITH_OVERFLOW_CODEGEN(Sub, Int32Sub)
#undef DEFINE_I32_WITH_OVERFLOW_CODEGEN

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32MulWithOverflowVertex>(I32MulWithOverflowVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I32MulWithOverflowVertex";
#endif
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, I32MulWithOverflowVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, I32MulWithOverflowVertex::RIGHT_INDEX);
    ASSERT(dst == left);

    Label overflow;
    Label negativeZero;
    Label success;
    Label done;
    ArkSteedRegister savedLeft = op->GetRegallocInfo()->GetGeneralTemporaries().First();
    __ Move(savedLeft, left);
#if defined(PANDA_TARGET_ARM64)
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister product = scope.AcquireScratch();
    ArkSteedRegister truncatedProduct = scope.AcquireScratch();
    __ Int32MulWide(product, dst, right);
    __ SignExtendInt32ToInt64(truncatedProduct, product);
    __ Compare(product, truncatedProduct);
    __ JumpIf(Condition::COND_NOT_EQUAL, &overflow);
    __ SignExtendInt32ToInt64(dst, product);
#else
    __ Int32Mul(dst, right);
    __ JumpIf(Condition::COND_OVERFLOW, &overflow);
#endif
    __ CompareInt32(dst, 0);
    __ JumpIf(Condition::COND_NOT_EQUAL, &success);
    __ Int32Or(savedLeft, right);
    __ CompareInt32(savedLeft, 0);
    __ JumpIf(Condition::COND_LESS_THAN, &negativeZero);
    __ Bind(&success);
    __ Jump(&done);
    __ Bind(&overflow);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::INT32OVERFLOW1);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::PRODUCTISNEGATIVEZERO);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32DivWithOverflowVertex>(I32DivWithOverflowVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I32DivWithOverflowVertex";
#endif
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, I32DivWithOverflowVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, I32DivWithOverflowVertex::RIGHT_INDEX);
    Label divideZero;
    Label overflow;
    Label notInt;
    Label negativeZero;
    Label divisorReady;
    Label done;

    __ CompareInt32(right, 0);
    __ JumpIf(Condition::COND_EQUAL, &divideZero);
    __ CompareInt32(left, std::numeric_limits<int32_t>::min());
    __ JumpIf(Condition::COND_NOT_EQUAL, &divisorReady);
    __ CompareInt32(right, -1);
    __ JumpIf(Condition::COND_EQUAL, &overflow);
    __ Bind(&divisorReady);
#if defined(PANDA_TARGET_AMD64)
    __ Int32DivAndRemainder(dst, x64::rdx, left, right);
    ArkSteedRegister remainder = x64::rdx;
#else
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister remainder = scope.AcquireScratch();
    __ Int32DivAndRemainder(dst, remainder, left, right);
#endif
    __ CompareInt32(remainder, 0);
    __ JumpIf(Condition::COND_NOT_EQUAL, &notInt);
    __ CompareInt32(dst, 0);
    __ JumpIf(Condition::COND_NOT_EQUAL, &done);
    __ CompareInt32(right, 0);
    __ JumpIf(Condition::COND_LESS_THAN, &negativeZero);
    __ Jump(&done);

    __ Bind(&divideZero);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::DIVZERO1);
    __ Jump(&done);
    __ Bind(&overflow);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::INT32OVERFLOW1);
    __ Jump(&done);
    __ Bind(&notInt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::NOTINT5);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::DIVZERO2);

    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32DivByConstWithCheckVertex>(I32DivByConstWithCheckVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I32DivByConstWithCheckVertex";
#endif
    auto dst = GetResultRegister(op);
    auto dividend = GetInputRegister(op, I32DivByConstWithCheckVertex::INPUT_INDEX);
#if defined(PANDA_TARGET_AMD64)
    ASSERT(dst == x64::rax);
    ASSERT(dividend == x64::rax);

    ArkSteedRegister work = x64::rdx;
    ArkSteedRegister original = x64::rcx;
    ArkSteedRegister divisor = x64::r8;
#else
    auto temporaries = op->GetRegallocInfo()->GetGeneralTemporaries();
    ArkSteedRegister work = temporaries.First();
    temporaries.PopFirst();
    ArkSteedRegister original = temporaries.First();
#endif
    Label negativeZero;
    Label notInt;
    Label done;

    __ Move(original, dividend);
    if (op->GetDivisor() < 0) {
        __ CompareInt32(original, 0);
        __ JumpIf(Condition::COND_EQUAL, &negativeZero);
    }

    __ Move(work, op->GetMagic());
    __ Int32MulHigh(work, dividend, work);
    if (op->GetDivisor() > 0 && op->GetMagic() < 0) {
        __ Int32Add(work, original);
    } else if (op->GetDivisor() < 0 && op->GetMagic() > 0) {
        __ Int32Sub(work, original);
    }
    if (op->GetShift() != 0) {
        __ Int32ShiftRightArithmetic(work, op->GetShift());
    }

    __ Move(dst, work);
    __ Move(work, dst);
    __ Int32ShiftRightLogical(work, 31);
    __ Int32Add(dst, work);

#if defined(PANDA_TARGET_AMD64)
    __ Move(work, dst);
    __ Move(divisor, op->GetDivisor());
    __ Int32Mul(work, divisor);
#else
    __ Move(work, op->GetDivisor());
    __ Int32Mul(work, dst);
#endif
    __ CompareInt32(work, original);
    __ JumpIf(Condition::COND_NOT_EQUAL, &notInt);
    __ Jump(&done);

    __ Bind(&negativeZero);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::DIVZERO2);
    __ Jump(&done);
    __ Bind(&notInt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, op, kungfu::DeoptType::NOTINT5);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32DivVertex>(I32DivVertex *div)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << div->GetId() << ": I32DivVertex";
#endif
    __ Int32Div(GetResultRegister(div), GetInputRegister(div, I32DivVertex::LEFT_INDEX),
                         GetInputRegister(div, I32DivVertex::RIGHT_INDEX));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedI32ModVertex>(CheckedI32ModVertex *mod)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << mod->GetId() << ": CheckedI32ModVertex";
#endif
    auto dst = GetResultRegister(mod);
    auto left = GetInputRegister(mod, CheckedI32ModVertex::LEFT_INDEX);
    auto right = GetInputRegister(mod, CheckedI32ModVertex::RIGHT_INDEX);
    Label divideZero;
    Label overflow;
    Label negativeZero;
    Label divisorReady;
    Label leftNeg;
    Label done;

    // divisor == 0 -> NaN (a double), deopt.
    __ CompareInt32(right, 0);
    __ JumpIf(Condition::COND_EQUAL, &divideZero);
    // INT_MIN % -1 traps idiv (#DE); deopt to the interpreter (mathematically 0).
    __ CompareInt32(left, std::numeric_limits<int32_t>::min());
    __ JumpIf(Condition::COND_NOT_EQUAL, &divisorReady);
    __ CompareInt32(right, -1);
    __ JumpIf(Condition::COND_EQUAL, &overflow);
    __ Bind(&divisorReady);
    // Read the dividend sign before PositiveInt32Mod, which clobbers the left input
    // register. remainder == 0 with a negative dividend is JS -0.0 (not Int32) -> deopt.
    __ CompareInt32(left, 0);
    __ JumpIf(Condition::COND_LESS_THAN, &leftNeg);
    __ PositiveInt32Mod(dst, left, right);
    __ Jump(&done);

    __ Bind(&leftNeg);
    __ PositiveInt32Mod(dst, left, right);
    __ CompareInt32(dst, 0);
    __ JumpIf(Condition::COND_EQUAL, &negativeZero);
    __ Jump(&done);

    __ Bind(&divideZero);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, mod, kungfu::DeoptType::MODZERO1);
    __ Jump(&done);
    __ Bind(&overflow);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, mod, kungfu::DeoptType::INT32OVERFLOW1);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, mod, kungfu::DeoptType::REMAINDERISNEGATIVEZERO);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedNonNegativeI32ToTaggedIntVertex>(
    CheckedNonNegativeI32ToTaggedIntVertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": CheckedNonNegativeI32ToTaggedIntVertex";
#endif
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, CheckedNonNegativeI32ToTaggedIntVertex::INPUT_INDEX);
    ArkSteedRegister scratch = convert->GetRegallocInfo()->GetGeneralTemporaries().First();
    Label deopt;
    Label done;

    __ CompareInt32(src, 0);
    __ JumpIf(Condition::COND_LESS_THAN, &deopt);
    __ SignExtendInt32ToInt64(dst, src);
    __ Move(scratch, static_cast<int64_t>(JSTaggedValue::TAG_INT));
    __ Or(dst, scratch);
    __ Jump(&done);

    __ Bind(&deopt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, convert, kungfu::DeoptType::NOTINT5);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32BitwiseBinaryVertex>(I32BitwiseBinaryVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I32BitwiseBinaryVertex";
#endif
    auto dst = GetResultRegister(op);
    ASSERT(dst == GetInputRegister(op, I32BitwiseBinaryVertex::LEFT_INDEX));

    if (std::optional<int32_t> rightConstant = TryGetInt32ConstantInput(op, I32BitwiseBinaryVertex::RIGHT_INDEX)) {
        uint32_t shift = static_cast<uint32_t>(*rightConstant) & 31U;
        switch (op->GetKind()) {
            case IntBitwiseKind::BITWISE_AND:
                __ Int32And(dst, *rightConstant);
                break;
            case IntBitwiseKind::BITWISE_OR:
                __ Int32Or(dst, *rightConstant);
                break;
            case IntBitwiseKind::BITWISE_XOR:
                __ Int32Xor(dst, *rightConstant);
                break;
            case IntBitwiseKind::SHIFT_LEFT:
                if (shift != 0) {
                    __ Int32ShiftLeft(dst, shift);
                }
                break;
            case IntBitwiseKind::SHIFT_RIGHT_LOGICAL:
                if (shift != 0) {
                    __ Int32ShiftRightLogical(dst, shift);
                }
                break;
            case IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC:
                if (shift != 0) {
                    __ Int32ShiftRightArithmetic(dst, shift);
                }
                break;
            default:
                UNREACHABLE();
        }
        __ SignExtendInt32ToInt64(dst, dst);
        return;
    }

    auto right = GetInputRegister(op, I32BitwiseBinaryVertex::RIGHT_INDEX);
    switch (op->GetKind()) {
        case IntBitwiseKind::BITWISE_AND:
            __ Int32And(dst, right);
            break;
        case IntBitwiseKind::BITWISE_OR:
            __ Int32Or(dst, right);
            break;
        case IntBitwiseKind::BITWISE_XOR:
            __ Int32Xor(dst, right);
            break;
        case IntBitwiseKind::SHIFT_LEFT:
#if defined(PANDA_TARGET_AMD64)
            ASSERT(right == x64::rcx);
#endif
            __ Int32ShiftLeftByRegister(dst, right);
            break;
        case IntBitwiseKind::SHIFT_RIGHT_LOGICAL:
#if defined(PANDA_TARGET_AMD64)
            ASSERT(right == x64::rcx);
#endif
            __ Int32ShiftRightLogicalByRegister(dst, right);
            break;
        case IntBitwiseKind::SHIFT_RIGHT_ARITHMETIC:
#if defined(PANDA_TARGET_AMD64)
            ASSERT(right == x64::rcx);
#endif
            __ Int32ShiftRightArithmeticByRegister(dst, right);
            break;
        default:
            UNREACHABLE();
    }
    __ SignExtendInt32ToInt64(dst, dst);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32ToF64Vertex>(I32ToF64Vertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": I32ToF64Vertex";
#endif
    __ Int32ToFloat64(GetResultDoubleRegister(convert),
                               GetInputRegister(convert, I32ToF64Vertex::INPUT_INDEX));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedNumberToF64Vertex>(CheckedNumberToF64Vertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": CheckedNumberToF64Vertex";
#endif
    auto dst = GetResultDoubleRegister(convert);
    auto input = GetInputRegister(convert, CheckedNumberToF64Vertex::INPUT_INDEX);
    auto temporaries = convert->GetRegallocInfo()->GetGeneralTemporaries();
    ArkSteedRegister bits = temporaries.First();
    temporaries.PopFirst();
    ArkSteedRegister scratch = temporaries.First();
    Label intCase;
    Label deopt;
    Label done;

    __ Move(bits, input);
    __ Move(scratch, JSTaggedValue::TAG_MARK);
    __ Word64And(bits, scratch);
    __ Compare(bits, scratch);
    __ JumpIf(Condition::COND_EQUAL, &intCase);
    __ Compare(bits, JSTaggedValue::TAG_OBJECT);
    __ JumpIf(Condition::COND_EQUAL, &deopt);

    __ Move(bits, input);
    __ Move(scratch, static_cast<uint64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ Sub(bits, scratch);
    __ Move(dst, bits);
    __ Jump(&done);

    __ Bind(&intCase);
    __ SignExtendInt32ToInt64(bits, input);
    __ Int32ToFloat64(dst, bits);
    __ Jump(&done);

    __ Bind(&deopt);
    EmitUseSlotDeopt(assembler_, safepointBuilder_, convert, kungfu::DeoptType::NOTNUMBER1);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ToI32TruncVertex>(F64ToI32TruncVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": F64ToI32TruncVertex";
#endif
    __ TruncateFloat64ToInt32(GetResultRegister(op),
                                       GetInputDoubleRegister(op, F64ToI32TruncVertex::INPUT_INDEX));
}

#define DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Name, AsmOp, DeoptType, NeedZeroCheck)                \
    template <>                                                                                       \
    void ArkSteedCodeGenerator::VisitNonControlVertex<I32##Name##WithOverflowVertex>(                 \
        I32##Name##WithOverflowVertex *op)                                                            \
    {                                                                                                 \
        auto dst = GetResultRegister(op);                                                             \
        ASSERT(dst == GetInputRegister(op, I32##Name##WithOverflowVertex::VALUE_INDEX));              \
        Label deopt;                                                                                  \
        Label done;                                                                                   \
        if constexpr (NeedZeroCheck) {                                                                \
            __ CompareInt32(dst, 0);                                                         \
            __ JumpIf(Condition::COND_EQUAL, &deopt);                                        \
        }                                                                                             \
        __ AsmOp(dst);                                                                       \
        __ JumpIf(Condition::COND_OVERFLOW, &deopt);                                         \
        __ Jump(&done);                                                                      \
        __ Bind(&deopt);                                                                     \
        EmitUseSlotDeopt(assembler_, safepointBuilder_, op, DeoptType);                               \
        __ Bind(&done);                                                                      \
    }

DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Neg, Int32Neg, kungfu::DeoptType::NOTNEGOV1, true)
DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Inc, Int32Inc, kungfu::DeoptType::INT32OVERFLOW1, false)
DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Dec, Int32Dec, kungfu::DeoptType::INT32OVERFLOW1, false)
#undef DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32BNotVertex>(I32BNotVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I32BNotVertex";
#endif
    auto dst = GetResultRegister(op);
    ASSERT(dst == GetInputRegister(op, I32BNotVertex::VALUE_INDEX));
    __ Int32BNot(dst);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64NegVertex>(F64NegVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": F64NegVertex";
#endif
    auto dst = GetResultDoubleRegister(op);
    auto value = GetInputDoubleRegister(op, F64NegVertex::VALUE_INDEX);
    __ Float64Neg(dst, value);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ToTaggedDoubleVertex>(F64ToTaggedDoubleVertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": F64ToTaggedDoubleVertex";
#endif
    auto dst = GetResultRegister(convert);
    auto input = GetInputDoubleRegister(convert, F64ToTaggedDoubleVertex::INPUT_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.AcquireScratch();
    Label pureDouble;
    Label done;

    __ Move(dst, input);
    __ Move(scratch, JSTaggedValue::TAG_INT - JSTaggedValue::DOUBLE_ENCODE_OFFSET);
    __ Compare(dst, scratch);
    __ JumpIf(Condition::COND_BELOW, &pureDouble);
    __ LoadTaggedValue(dst, JSTaggedValue(base::NAN_VALUE).GetRawData());
    __ Jump(&done);
    __ Bind(&pureDouble);
    __ Move(scratch, static_cast<uint64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ Add(dst, scratch);
    __ Bind(&done);
}

#define DEFINE_F64_BINOP_CODEGEN(Name, Op)                                                        \
    template <>                                                                                    \
    void ArkSteedCodeGenerator::VisitNonControlVertex<F64##Name##Vertex>(F64##Name##Vertex *op)   \
    {                                                                                              \
        auto dst = GetResultDoubleRegister(op);                                                    \
        auto left = GetInputDoubleRegister(op, F64##Name##Vertex::LEFT_INDEX);                     \
        auto right = GetInputDoubleRegister(op, F64##Name##Vertex::RIGHT_INDEX);                   \
        ASSERT(dst == left);                                                                       \
        __ Op(dst, right);                                                                \
    }

DEFINE_F64_BINOP_CODEGEN(Add, Float64Add)
DEFINE_F64_BINOP_CODEGEN(Sub, Float64Sub)
DEFINE_F64_BINOP_CODEGEN(Mul, Float64Mul)
DEFINE_F64_BINOP_CODEGEN(Div, Float64Div)
#undef DEFINE_F64_BINOP_CODEGEN

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CallCommonStubVertex>(CallCommonStubVertex *callCommonStub)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << callCommonStub->GetId() << ": CallCommonStubVertex";
#endif
    int stackArgCount = PrepareCommonStubStackArguments(callCommonStub, callCommonStub->GetArgCount());
    __ CallCommonStub(callCommonStub->GetCommonStubID());
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ FreeCallArgSlots(stackArgCount);
}

bool ArkSteedCodeGenerator::AllPredecessorsDeferred(BB *block) const
{
    bool allDeferred = true;
    block->ForEachPredecessor([&allDeferred](BB *predecessor) {
        if (!predecessor->IsDeferred()) {
            allDeferred = false;
        }
    });
    return allDeferred;
}

bool ArkSteedCodeGenerator::AllSuccessorsDeferred(BB *block)
{
    bool allDeferred = true;
    block->ForEachSuccessor([&allDeferred](BB *successor) {
        ASSERT(successor != nullptr);
        if (!successor->IsDeferred()) {
            allDeferred = false;
        }
    });
    return allDeferred;
}

int ArkSteedCodeGenerator::ComputeDeferredBlocks()
{
    int deferredCount = 0;
    ChunkVector<BB *> workQueue(graph_->GetChunk());
    // (fixme): Seed deferred blocks when cold throw/deopt paths are modelled.
    for (BB *block : *graph_) {
        if (block->IsDeferred()) {
            ++deferredCount;
            workQueue.push_back(block);
        }
    }

    while (!workQueue.empty()) {
        BB *block = workQueue.back();
        workQueue.pop_back();
        ASSERT(block->IsDeferred());

        block->ForEachSuccessor([this, &workQueue, &deferredCount](BB *successor) {
            if (!successor->IsDeferred() && AllPredecessorsDeferred(successor)) {
                successor->SetDeferred(true);
                ++deferredCount;
                workQueue.push_back(successor);
            }
        });

        block->ForEachPredecessor([this, &workQueue, &deferredCount](BB *predecessor) {
            if (!predecessor->IsDeferred() && AllSuccessorsDeferred(predecessor)) {
                predecessor->SetDeferred(true);
                ++deferredCount;
                workQueue.push_back(predecessor);
            }
        });
    }
    return deferredCount;
}

void ArkSteedCodeGenerator::ReorderDeferredBlocks(int deferredCount)
{
    if (deferredCount == 0) {
        return;
    }
    // If we deferred the first block, un-defer it. This can happen because we
    // defer a block if all its successors are deferred (i.e., lead to an
    // unconditional deopt). E.g., if we only executed exception throwing code
    // paths, the non-exception code paths might be untaken, and thus contain
    // unconditional deopts, so we end up deferring all non-exception code
    // paths, including the first block.
    if ((*graph_)[0]->IsDeferred()) {
        (*graph_)[0]->SetDeferred(false);
        --deferredCount;
    }
    std::stable_partition(graph_->begin(), graph_->end(), [](BB *block) { return !block->IsDeferred(); });
}

bool ArkSteedCodeGenerator::IsNextBlockInLayout(BB *target) const
{
    ASSERT(target != nullptr);
    return target == currentLayoutNextBlock_;
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<JumpVertex>(JumpVertex *jump)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jump->GetId() << ": JumpVertex to BB #" << jump->Target()->GetId();
#endif
    bool isFallthrough = IsNextBlockInLayout(jump->Target());
    if (!isFallthrough) {
        __ Jump(jump->Target()->GetLabel());
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
    __ Jump(jumpLoop->Target()->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTrueVertex>(BranchIfTrueVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfTrueVertex to BB #" << ifTrue->GetId()
                        << (trueBranchIsFallthrough ? " (fallthrough)" : "") << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto cond = GetInputRegister(jumpIf, 0);
    // to do: optimize
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.AcquireScratch();
    __ LoadTaggedValue(scratch, JSTaggedValue::True().GetRawData());
    __ Compare(cond, scratch);

    __ Branch(Condition::COND_ZERO,
                       ifTrue->GetLabel(),
                       trueBranchIsFallthrough,
                       ifFalse->GetLabel(),
                       falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfInt32CompareVertex>(BranchIfInt32CompareVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfInt32CompareVertex to BB #"
                        << ifTrue->GetId() << (trueBranchIsFallthrough ? " (fallthrough)" : "")
                        << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto left = GetInputRegister(jumpIf, BranchIfInt32CompareVertex::LEFT_INDEX);
    auto right = GetInputRegister(jumpIf, BranchIfInt32CompareVertex::RIGHT_INDEX);
    __ CompareInt32(left, right);
    __ Branch(ConditionFromIntCondition(jumpIf->GetCondition()),
                       ifTrue->GetLabel(),
                       trueBranchIsFallthrough,
                       ifFalse->GetLabel(),
                       falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfInt64CompareVertex>(BranchIfInt64CompareVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfInt64CompareVertex to BB #"
                        << ifTrue->GetId() << (trueBranchIsFallthrough ? " (fallthrough)" : "")
                        << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto left = GetInputRegister(jumpIf, BranchIfInt64CompareVertex::LEFT_INDEX);
    auto right = GetInputRegister(jumpIf, BranchIfInt64CompareVertex::RIGHT_INDEX);
    __ Compare(left, right);
    __ Branch(ConditionFromIntCondition(jumpIf->GetCondition()),
                       ifTrue->GetLabel(),
                       trueBranchIsFallthrough,
                       ifFalse->GetLabel(),
                       falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfFloat64CompareVertex>(BranchIfFloat64CompareVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();

#ifndef NDEBUG
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfFloat64CompareVertex to BB #"
                        << ifTrue->GetId() << (trueBranchIsFallthrough ? " (fallthrough)" : "")
                        << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto left = GetInputDoubleRegister(jumpIf, BranchIfFloat64CompareVertex::LEFT_INDEX);
    auto right = GetInputDoubleRegister(jumpIf, BranchIfFloat64CompareVertex::RIGHT_INDEX);
    __ CompareFloat64(left, right);
    BranchOnFloat64Compare(assembler_, jumpIf->GetCondition(), ifTrue->GetLabel(), ifFalse->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTaggedHeapObjectVertex>(
    BranchIfTaggedHeapObjectVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfTaggedHeapObjectVertex to BB #"
                        << ifTrue->GetId() << (trueBranchIsFallthrough ? " (fallthrough)" : "")
                        << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto value = GetInputRegister(jumpIf, BranchIfTaggedHeapObjectVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.AcquireScratch();
    assembler_->Move(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    assembler_->And(scratch, value);
    assembler_->Compare(scratch, 0);
    assembler_->Branch(Condition::COND_EQUAL,
                       ifTrue->GetLabel(),
                       trueBranchIsFallthrough,
                       ifFalse->GetLabel(),
                       falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfReferenceEqualVertex>(BranchIfReferenceEqualVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId() << ": BranchIfReferenceEqualVertex to BB #"
                        << ifTrue->GetId() << (trueBranchIsFallthrough ? " (fallthrough)" : "")
                        << " if true; to BB #" << ifFalse->GetId()
                        << (falseBranchIsFallthrough ? " (fallthrough)" : "") << " if false.";
#endif

    auto left = GetInputRegister(jumpIf, BranchIfReferenceEqualVertex::LEFT_INDEX);
    auto right = GetInputRegister(jumpIf, BranchIfReferenceEqualVertex::RIGHT_INDEX);
    __ Compare(left, right);
    __ Branch(Condition::COND_EQUAL,
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
    __ Epilogue();
    __ Return();
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<ThrowVertex>(ThrowVertex *throws)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << throws->GetId() << ": ThrowVertex";
#endif
    int stackArgCount =
        PrepareRuntimeStubStackArguments(throws, throws->GetArgCount(), static_cast<int>(throws->GetRuntimeStubID()));
    __ CallRuntime(throws->GetRuntimeStubID());
    __ FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());

    BB *catchBlock = throws->CaughtBy();
    if (catchBlock != nullptr) {
        uint32_t catchPredId = throws->GetCatchPredecessorIndex();
        DeconstructPhisInSuccessor(catchBlock, catchPredId);
        __ Jump(catchBlock->GetLabel());
    } else {
        __ ReturnWithPendingException();
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
    int deferredCount = ComputeDeferredBlocks();
    ReorderDeferredBlocks(deferredCount);

    // Compute block colors only when code comments are enabled
    // This minimizes JIT compilation overhead when comments are disabled
    if (__ IsCommentEnabled()) {
        ComputeBlockColors();
    }

    __ Prologue(graph_);

    for (uint32_t i = 0, numBlocks = graph_->NumBlocks(); i < numBlocks; ++i) {
        BB *curBlock = (*graph_)[i];
        currentLayoutNextBlock_ = (i + 1 < numBlocks) ? (*graph_)[i + 1] : nullptr;
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "CodeGen: Starts BB #" << curBlock->GetId();
#endif

        RecordBlockComment(curBlock);
        __ Bind(curBlock->GetLabel());

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
            auto spillSlot = AllocatedState::Cast(vertexInfo->GetSpillSlot());
            if (source.IsRegister()) {
                __ MoveRepr(source.GetRepresentation(),
                                     __ GetStackSlot(spillSlot),
                                     source.GetRegister());
            } else if (source.IsDoubleRegister()) {
                __ StoreFloat64(__ GetStackSlot(spillSlot),
                                         source.GetDoubleRegister());
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
    TemporaryRegisterScope temporaryScope(assembler_);
    temporaryScope.Include(vertex->GetRegallocInfo()->GetGeneralTemporaries());
    temporaryScope.IncludeDouble(vertex->GetRegallocInfo()->GetDoubleTemporaries());

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
            __ BranchIfNoPendingException(&noException);
            DeconstructPhisInSuccessor(catchBlock, CatchPredecessorIndexOf(vertex));
            __ Jump(catchBlock->GetLabel());
            __ Bind(&noException);
        } else {
            __ ReturnIfPendingException();
        }
    }
}

void ArkSteedCodeGenerator::ProcessControlVertex(ControlVertex *vertex)
{
    RecordVertexComment(vertex);
    TemporaryRegisterScope temporaryScope(assembler_);
    temporaryScope.Include(vertex->GetRegallocInfo()->GetGeneralTemporaries());
    temporaryScope.IncludeDouble(vertex->GetRegallocInfo()->GetDoubleTemporaries());

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
        __ ReturnIfPendingException();
    }
}

void ArkSteedCodeGenerator::DeconstructPhisInSuccessor(BB *successor, uint32_t predecessorId)
{
    if (!successor->HasPhi() && !successor->HasRegisterMergeState()) {
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
        ExecuteConstantPhiMove(dest, constVertex, &scratchGPR, &scratchFPR);
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
    if (!successor->HasRegisterMergeState()) {
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

        ASSERT(vertex != nullptr);
        MachineRepresentation repr = vertex->GetMachineRepresentation();
        ASSERT(!IsFloatingPoint(repr));

        InstructionOperand src = mergeInfo->Operand(predecessorId);
        AllocatedState dest(LocationState::LocationKind::REGISTER, repr, reg.Code());
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
        if (src.IsConstant()) {
            constantMoves->emplace_back(dest, vertex);
        } else if (src != dest) {
            resolver->Add(dest, AllocatedState::Cast(src));
        }
    });
}

void ArkSteedCodeGenerator::ExecuteConstantPhiMove(const AllocatedState &dest, ValueVertex *constVertex,
                                                   const ArkSteedRegister *scratchGPR,
                                                   const ArkSteedDoubleRegister *scratchFPR)
{
    ASSERT(constVertex != nullptr);

    ArkSteedRegister localGPR = ArkSteedRegister::Invalid();
    ArkSteedDoubleRegister localFPR = ArkSteedDoubleRegister::Invalid();
    TemporaryRegisterScope scope(assembler_);
    if (scratchGPR == nullptr) {
        localGPR = scope.AcquireScratch();
        scratchGPR = &localGPR;
    }
    if (scratchFPR == nullptr) {
        localFPR = scope.AcquireDoubleScratch();
        scratchFPR = &localFPR;
    }

    auto loadConstant = [&](ArkSteedRegister reg) {
        if (dest.GetRepresentation() == MachineRepresentation::Tagged) {
            __ Move(reg, GetConstantForDeopt(constVertex, 0));
            return;
        }
        LoadConstantToRegister(constVertex, reg);
    };

    if (dest.IsRegister()) {
        loadConstant(dest.GetRegister());
        return;
    }
    if (dest.IsDoubleRegister()) {
        Float64ConstantVertex *f64const = constVertex->Cast<Float64ConstantVertex>();
        __ Move(dest.GetDoubleRegister(), f64const->GetValue(), *scratchGPR);
        return;
    }
    ASSERT(dest.IsAnyStackSlot());

    if (dest.GetRepresentation() == MachineRepresentation::Float64) {
        LoadConstantToDoubleRegister(constVertex, *scratchFPR);
        __ Move(*scratchGPR, *scratchFPR);
        __ MoveRepr(MachineRepresentation::Word64, __ ToMemOperand(dest), *scratchGPR);
        return;
    }

    loadConstant(*scratchGPR);
    __ MoveRepr(dest.GetRepresentation(), __ ToMemOperand(dest), *scratchGPR);
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
            __ MoveRepr(repr, destOp.GetRegister(), srcOp.GetRegister());
        } else if (destOp.IsAnyStackSlot()) {
            __ MoveRepr(repr, __ ToMemOperand(destOp), srcOp.GetRegister());
        } else {
            UNREACHABLE();
        }
    } else if (srcOp.IsDoubleRegister()) {
        if (destOp.IsDoubleRegister()) {
            __ Move(destOp.GetDoubleRegister(), srcOp.GetDoubleRegister());
        } else if (destOp.IsAnyStackSlot()) {
            ASSERT(scratchGPR != nullptr);
            __ Move(*scratchGPR, srcOp.GetDoubleRegister());
            __ MoveRepr(MachineRepresentation::Word64, __ ToMemOperand(destOp), *scratchGPR);
        } else {
            UNREACHABLE();
        }
    } else if (srcOp.IsAnyStackSlot()) {
        ArkSteedAssembler::MemoryOperand srcMem = __ ToMemOperand(srcOp);
        if (destOp.IsRegister()) {
            __ MoveRepr(repr, destOp.GetRegister(), srcMem);
        } else if (destOp.IsDoubleRegister()) {
            __ LoadFloat64(destOp.GetDoubleRegister(), srcMem);
        } else {
            ASSERT(destOp.IsAnyStackSlot());
            if (repr == MachineRepresentation::Float64) {
                ASSERT(scratchGPR != nullptr);
                __ MoveRepr(MachineRepresentation::Word64, *scratchGPR, srcMem);
                __ MoveRepr(MachineRepresentation::Word64, __ ToMemOperand(destOp), *scratchGPR);
            } else if (scratchGPR != nullptr) {
                __ MoveRepr(repr, *scratchGPR, srcMem);
                __ MoveRepr(repr, __ ToMemOperand(destOp), *scratchGPR);
            } else {
                __ MoveRepr(repr, __ ToMemOperand(destOp), srcMem);
            }
        }
    }
}

// -------------------------------------------------------------------------
// Comment recording helpers
// -------------------------------------------------------------------------

void ArkSteedCodeGenerator::RecordComment(const char *msg)
{
    if (!__ IsCommentEnabled()) {
        return;
    }
    __ RecordComment(msg);
}

void ArkSteedCodeGenerator::RecordBlockComment(BB *block)
{
    if (!__ IsCommentEnabled()) {
        return;
    }
    // Set current block color for subsequent IR lines
    SetCurrentBlockColor(block->GetId());
    std::ostringstream ss;
    ss << GetCurrentBlockColor() << "-- Block b" << block->GetId();
    if (block->IsDeferred()) {
        ss << " (deferred)";
    }

    // Print predecessors for merge blocks with their colors
    if (block->HasRegisterMergeState()) {
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
    if (!__ IsCommentEnabled()) {
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
    } else if (auto *branch = control->TryCast<BranchControlVertex>(); branch != nullptr) {
        *ss << " --> [" << GetBlockColor(branch->IfTrue()->GetId()) << "b" << branch->IfTrue()->GetId();
        if (IsNextBlockInLayout(branch->IfTrue())) {
            *ss << " (fallthrough)";
        }
        *ss << GetCurrentBlockColor() << " if true, " << GetBlockColor(branch->IfFalse()->GetId()) << "b"
            << branch->IfFalse()->GetId();
        if (IsNextBlockInLayout(branch->IfFalse())) {
            *ss << " (fallthrough)";
        }
        *ss << GetCurrentBlockColor() << " if false]";
    }
}

void ArkSteedCodeGenerator::RecordGapMoveComment(const InstructionOperand &src, const InstructionOperand &dest,
                                                 PhiVertex *phi)
{
    if (!__ IsCommentEnabled()) {
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
    if (__ IsCommentEnabled()) {
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
    uint32_t numBlocks = graph_->NumBlocks();
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
    uint32_t numBlocks = graph_->NumBlocks();
    for (uint32_t i = 0; i < numBlocks; ++i) {
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
        } else if (auto *branch = control->TryCast<BranchControlVertex>(); branch != nullptr) {
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
    uint32_t numBlocks = graph_->NumBlocks();
    for (uint32_t i = 0; i < numBlocks; ++i) {
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

#undef __
}  // namespace panda::ecmascript::arksteed
