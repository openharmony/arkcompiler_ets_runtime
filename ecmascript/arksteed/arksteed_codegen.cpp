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
#include "ecmascript/arksteed/arksteed_deopt_helper.h"
#include "ecmascript/arksteed/arksteed_dump_helper.h"
#include "ecmascript/arksteed/arksteed_register_merge_state.h"
#include "ecmascript/arksteed/arksteed_safepoint_table.h"
#include "ecmascript/arksteed/arksteed_write_barrier.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/byte_array.h"
#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::arksteed {
#define __ assembler_->

class GapMoveResolver {
public:
    using AssignmentPair = std::pair<AllocatedState, AllocatedState>;

    explicit GapMoveResolver(Chunk *chunk, ArkSteedRegister scratchGPR, ArkSteedDoubleRegister scratchFPR)
        : scratchGPR_(scratchGPR),
          scratchFPR_(scratchFPR),
          assignments_(chunk),
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

    template <typename EmitMoveFn, typename SaveScratchFn, typename RestoreScratchFn>
    void Resolve(EmitMoveFn &&emitMove, SaveScratchFn &&saveScratch, RestoreScratchFn &&restoreScratch)
    {
        if (assignments_.empty()) {
            return;
        }
        BuildSortedOperandList();
        BuildAdjacencyLists();

        for (uint32_t i = 0; i < sortedList_.size(); i++) {
            if (!ValueInStorage(i).IsAnyRegister()) {
                continue;
            }
            StartEmitMoveChain(i, emitMove, saveScratch, restoreScratch);
        }

        while (auto stackSourceIndex = FirstPendingStackSource()) {
            StartEmitMoveChain(*stackSourceIndex, emitMove, saveScratch, restoreScratch);
        }
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

    AllocatedState ValueInStorage(uint32_t index) const
    {
        if (hasSourceOperands_[index] != 0) {
            return sourceOperands_[index];
        }
        return sortedList_[index];
    }

    ChunkVector<uint32_t> PopTargets(uint32_t sourceIndex)
    {
        ChunkVector<uint32_t> targets(GetChunk());
        targets.swap(adjLists_[sourceIndex]);
        return targets;
    }

    std::optional<uint32_t> FirstPendingStackSource() const
    {
        for (uint32_t i = 0; i < sortedList_.size(); i++) {
            if (!ValueInStorage(i).IsAnyStackSlot()) {
                continue;
            }
            if (!adjLists_[i].empty()) {
                return i;
            }
        }
        return std::nullopt;
    }

    template <typename EmitMoveFn, typename SaveScratchFn, typename RestoreScratchFn>
    void StartEmitMoveChain(uint32_t sourceIndex, EmitMoveFn &emitMove, SaveScratchFn &saveScratch,
                            RestoreScratchFn &restoreScratch)
    {
        ASSERT(!scratchHasCycleStart_);
        cycleScratch_.reset();

        ChunkVector<uint32_t> targets = PopTargets(sourceIndex);
        if (targets.empty()) {
            return;
        }

        bool hasCycle = RecursivelyEmitMoveChainTargets(sourceIndex, targets, emitMove, saveScratch, restoreScratch);
        if (hasCycle) {
            ASSERT(cycleScratch_.has_value());
            if (!scratchHasCycleStart_) {
                restoreScratch(*cycleScratch_);
                scratchHasCycleStart_ = true;
            }
            EmitMovesFromSource(*cycleScratch_, targets, emitMove, saveScratch);
            scratchHasCycleStart_ = false;
        } else {
            EmitMovesFromSource(ValueInStorage(sourceIndex), targets, emitMove, saveScratch);
        }
        cycleScratch_.reset();
    }

    template <typename EmitMoveFn, typename SaveScratchFn, typename RestoreScratchFn>
    bool ContinueEmitMoveChain(uint32_t chainStartIndex, uint32_t sourceIndex, EmitMoveFn &emitMove,
                               SaveScratchFn &saveScratch, RestoreScratchFn &restoreScratch)
    {
        if (chainStartIndex == sourceIndex) {
            ASSERT(!cycleScratch_.has_value());
            cycleScratch_ = ScratchOperandLike(ValueInStorage(chainStartIndex));
            emitMove(*cycleScratch_, ValueInStorage(chainStartIndex));
            scratchHasCycleStart_ = true;
            return true;
        }

        ChunkVector<uint32_t> targets = PopTargets(sourceIndex);
        if (targets.empty()) {
            return false;
        }

        bool hasCycle = RecursivelyEmitMoveChainTargets(chainStartIndex, targets, emitMove, saveScratch, restoreScratch);
        EmitMovesFromSource(ValueInStorage(sourceIndex), targets, emitMove, saveScratch);
        return hasCycle;
    }

    template <typename EmitMoveFn, typename SaveScratchFn, typename RestoreScratchFn>
    bool RecursivelyEmitMoveChainTargets(uint32_t chainStartIndex, ChunkVector<uint32_t> &targets, EmitMoveFn &emitMove,
                                         SaveScratchFn &saveScratch, RestoreScratchFn &restoreScratch)
    {
        bool hasCycle = false;
        for (uint32_t edgeIndex : targets) {
            hasCycle |= ContinueEmitMoveChain(chainStartIndex, edges_[edgeIndex].destIndex, emitMove, saveScratch,
                                              restoreScratch);
        }
        return hasCycle;
    }

    template <typename EmitMoveFn, typename SaveScratchFn>
    void EmitMovesFromSource(AllocatedState source, ChunkVector<uint32_t> &targets, EmitMoveFn &emitMove,
                             SaveScratchFn &saveScratch)
    {
        if (source.IsAnyRegister()) {
            for (uint32_t edgeIndex : targets) {
                emitMove(edges_[edgeIndex].dest, source);
            }
            return;
        }

        ASSERT(source.IsAnyStackSlot());

        std::optional<size_t> registerTargetIndex;
        for (size_t i = 0; i < targets.size(); i++) {
            if (edges_[targets[i]].dest.IsAnyRegister()) {
                registerTargetIndex = i;
                break;
            }
        }

        AllocatedState cachedSource;
        if (registerTargetIndex.has_value()) {
            cachedSource = edges_[targets[*registerTargetIndex]].dest;
            emitMove(cachedSource, source);
        } else {
            cachedSource = ScratchOperandLike(source);
            if (scratchHasCycleStart_ && cycleScratch_.has_value() && SameStorage(*cycleScratch_, cachedSource)) {
                saveScratch(cachedSource);
                scratchHasCycleStart_ = false;
            }
            emitMove(cachedSource, source);
        }

        for (size_t i = 0; i < targets.size(); i++) {
            if (registerTargetIndex.has_value() && i == *registerTargetIndex) {
                continue;
            }
            emitMove(edges_[targets[i]].dest, cachedSource);
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
    ChunkVector<AllocatedState> sortedList_;
    ChunkVector<AllocatedState> sourceOperands_;
    ChunkVector<uint8_t> hasSourceOperands_;
    ChunkVector<AssignmentEdge> edges_;
    ChunkVector<ChunkVector<uint32_t>> adjLists_;
    std::optional<AllocatedState> cycleScratch_;
    bool scratchHasCycleStart_ = false;
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

void LoadObjectType(ArkSteedAssembler *assembler, ArkSteedRegister dst, ArkSteedRegister value)
{
    assembler->LoadField(dst, value, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
    assembler->And(dst, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    assembler->LoadField(dst, dst, static_cast<int32_t>(JSHClass::BIT_FIELD_OFFSET));
    assembler->And(dst, static_cast<int64_t>(0xFF));
}

void LoadHClassBitField(ArkSteedAssembler *assembler, ArkSteedRegister dst, ArkSteedRegister value)
{
    assembler->LoadField(dst, value, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
    assembler->And(dst, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    assembler->LoadField(dst, dst, static_cast<int32_t>(JSHClass::BIT_FIELD_OFFSET));
}

uint32_t TypedArrayElementShift(JSType type)
{
    switch (type) {
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return 0;
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
            return 1;
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
            return 2;
        case JSType::JS_FLOAT64_ARRAY:
            return 3;
        default:
            UNREACHABLE();
    }
}

void BuildTypedArrayDataPointer(ArkSteedAssembler *assembler, ArkSteedRegister receiver, OnHeapMode onHeapMode,
                                ArkSteedRegister base, ArkSteedRegister backingOffset)
{
    assembler->LoadField(base, receiver, static_cast<int32_t>(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET));
    if (OnHeap::IsOnHeap(onHeapMode)) {
        assembler->Add(base, static_cast<int32_t>(ByteArray::DATA_OFFSET));
    } else if (OnHeap::IsNotOnHeap(onHeapMode)) {
        ASSERT(backingOffset != base);
        assembler->LoadField(base, base, static_cast<int32_t>(JSArrayBuffer::DATA_OFFSET));
        assembler->LoadField(base, base, static_cast<int32_t>(JSNativePointer::POINTER_OFFSET));
        assembler->LoadInt32Field(backingOffset, receiver, static_cast<int32_t>(JSTypedArray::BYTE_OFFSET_OFFSET));
        assembler->Add(base, backingOffset);
    } else {
        ASSERT(backingOffset != base);
        Label offHeap;
        Label addressDone;
        LoadHClassBitField(assembler, backingOffset, receiver);
        assembler->And(backingOffset, static_cast<int64_t>(1U << JSHClass::IsOnHeap::START_BIT));
        assembler->Compare(backingOffset, 0);
        assembler->JumpIf(Condition::EQUAL, &offHeap);
        assembler->Add(base, static_cast<int32_t>(ByteArray::DATA_OFFSET));
        assembler->Jump(&addressDone);

        assembler->Bind(&offHeap);
        assembler->LoadField(base, base, static_cast<int32_t>(JSArrayBuffer::DATA_OFFSET));
        assembler->LoadField(base, base, static_cast<int32_t>(JSNativePointer::POINTER_OFFSET));
        assembler->LoadInt32Field(backingOffset, receiver, static_cast<int32_t>(JSTypedArray::BYTE_OFFSET_OFFSET));
        assembler->Add(base, backingOffset);
        assembler->Bind(&addressDone);
    }
}

void BuildTypedArrayElementAddress(ArkSteedAssembler *assembler, ArkSteedRegister receiver, ArkSteedRegister index,
                                   JSType type, OnHeapMode onHeapMode, ArkSteedRegister base, ArkSteedRegister byteOffset,
                                   ArkSteedRegister backingOffset)
{
    ASSERT(base != byteOffset);
    assembler->Move(byteOffset, index);
    uint32_t shift = TypedArrayElementShift(type);
    if (shift != 0) {
        assembler->ShiftLeft(byteOffset, shift);
    }
    BuildTypedArrayDataPointer(assembler, receiver, onHeapMode, base, backingOffset);
    assembler->Add(base, byteOffset);
}

void CanonicalizeNaN(ArkSteedAssembler *assembler, ArkSteedDoubleRegister value, ArkSteedDoubleRegister normalized,
                     ArkSteedRegister scratch)
{
    assembler->Move(normalized, value);
    assembler->CompareFloat64(normalized, normalized);
    Label done;
    assembler->JumpIf(Condition::NOT_PARITY, &done);
    assembler->Move(normalized, base::NAN_VALUE, scratch);
    assembler->Bind(&done);
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

#ifndef NDEBUG
bool EagerDeoptUsesRegister(const EagerDeoptimizableMixin *vertex, ArkSteedRegister reg)
{
    for (uint32_t index = 0; index < vertex->GetDeoptFrameValueCount(); ++index) {
        const InstructionOperand &operand = vertex->GetDeoptSourceLocation(index)->GetOperand();
        ASSERT(operand.IsConstant() || operand.IsAllocated());
        if (operand.IsRegister() && AllocatedState::Cast(operand).GetRegister() == reg) {
            return true;
        }
    }
    return false;
}
#endif

ArkSteedDeoptValue MakeConstantDeopt(int32_t id, int64_t value)
{
    return {
        static_cast<kungfu::LLVMStackMapType::VRegId>(id),
        ArkSteedDeoptValueKind::CONSTANT,
        value,
    };
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
        case VertexOpcode::Float64Constant: {
            ASSERT(vregId != static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX));
            ASSERT(vregId != static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH));
            uint64_t raw = static_cast<uint64_t>(
                GetFloat64RawBits(value->Cast<Float64ConstantVertex>()->GetValue()));
            if (raw >= static_cast<uint64_t>(JSTaggedValue::TAG_INT - JSTaggedValue::DOUBLE_ENCODE_OFFSET)) {
                return static_cast<int64_t>(JSTaggedValue(base::NAN_VALUE).GetRawData());
            }
            return static_cast<int64_t>(raw + JSTaggedValue::DOUBLE_ENCODE_OFFSET);
        }
        default:
            UNREACHABLE();
    }
}

void AppendDeoptSource(std::vector<ArkSteedDeoptValue> *deopts,
                       const LazyDeoptimizableMixin::LazyDeoptFrameValue &frameValue,
                       ArkSteedAssembler *assembler, DeoptLiteralTableBuilder *literalTableBuilder)
{
    const InstructionOperand &operand = frameValue.sourceLocation.GetOperand();
    if (operand.IsConstant()) {
        if (auto *heapConstant = frameValue.value->TryCast<HeapConstantVertex>()) {
            ASSERT(literalTableBuilder != nullptr);
            deopts->push_back({
                static_cast<kungfu::LLVMStackMapType::VRegId>(frameValue.vreg),
                ArkSteedDeoptValueKind::HEAP_LITERAL,
                literalTableBuilder->GetOrAdd(heapConstant->GetHandleIndex()),
            });
        } else {
            deopts->push_back(MakeConstantDeopt(
                frameValue.vreg, GetConstantForDeopt(frameValue.value, frameValue.vreg)));
        }
        return;
    }

    ASSERT(operand.IsAnyStackSlot());
    auto stackSlot = AllocatedState::Cast(operand);
    int32_t offset = assembler->GetFramePointerOffsetForStackSlot(stackSlot.GetIndex(), stackSlot.GetRepresentation());
    deopts->push_back({
        static_cast<kungfu::LLVMStackMapType::VRegId>(frameValue.vreg),
        ArkSteedDeoptValueKind::STACK_SLOT,
        EncodeLazyDeoptOffset(offset, frameValue.valueKind),
        static_cast<kungfu::LLVMStackMapType::DwarfRegType>(GCStackMapRegisters::FP),
    });
}

void AppendDeoptSources(std::vector<ArkSteedDeoptValue> *deopts,
                        const LazyDeoptimizableMixin *lazy,
                        ArkSteedAssembler *assembler, DeoptLiteralTableBuilder *literalTableBuilder)
{
    if (lazy == nullptr || !lazy->HasLazyDeoptFrameState()) {
        return;
    }
    for (const auto &frameValue : lazy->GetLazyDeoptFrameState()) {
        AppendDeoptSource(deopts, frameValue, assembler, literalTableBuilder);
    }
}

template <class NodeT>
bool HasLazyDeoptSafepointFor(const NodeT *vertex)
{
    if constexpr (std::is_base_of_v<LazyDeoptimizableMixin, NodeT>) {
        return static_cast<const LazyDeoptimizableMixin *>(vertex)->HasLazyDeoptFrameState();
    }
    return false;
}

bool HasLazyDeoptSafepoint(const Vertex *vertex)
{
    switch (vertex->GetOpcode()) {
#define CHECK_LAZY_DEOPT_INPUTS(type)                   \
        case VertexOpcode::type:                        \
            return HasLazyDeoptSafepointFor(vertex->Cast<type##Vertex>());
        ALL_VERTEX_LIST(CHECK_LAZY_DEOPT_INPUTS)
#undef CHECK_LAZY_DEOPT_INPUTS
        default:
            UNREACHABLE();
    }
}

template <class NodeT>
void EmitLazyDeoptSafepoint(ArkSteedAssembler *assembler,
                            ArkSteedSafepointTableBuilder *safepointBuilder,
                            DeoptLiteralTableBuilder *literalTableBuilder, const NodeT *vertex)
{
    static_assert(std::is_base_of_v<LazyDeoptimizableMixin, NodeT>);
    ASSERT(safepointBuilder != nullptr);
    const auto *lazy = static_cast<const LazyDeoptimizableMixin *>(vertex);
    ASSERT(lazy->HasLazyDeoptFrameState());

    std::vector<ArkSteedDeoptValue> deopts;
    deopts.emplace_back(MakeConstantDeopt(static_cast<int32_t>(SpecVregIndex::INLINE_DEPTH), 0));
    AppendDeoptSources(&deopts, lazy, assembler, literalTableBuilder);

    ExceptionHandlerKind exceptionHandlerKind = ExceptionHandlerKind::NONE;
    if constexpr (std::is_base_of_v<ThrowableMixin, NodeT>) {
        if (!vertex->HasCatchBlock() && vertex->HasExceptionLazyDeopt()) {
            exceptionHandlerKind = ExceptionHandlerKind::LAZY_DEOPT;
        }
    }
    safepointBuilder->DefineDeoptSafepoint(assembler->GetPcOffset(), std::move(deopts), exceptionHandlerKind);
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

Condition OrderedFloat64Condition(Condition condition)
{
    switch (condition) {
        case Condition::EQUAL:
            return Condition::EQUAL;
        case Condition::NOT_EQUAL:
            return Condition::NOT_EQUAL;
        case Condition::LESS_THAN:
            return Condition::BELOW;
        case Condition::LESS_THAN_OR_EQUAL:
            return Condition::BELOW_OR_EQUAL;
        case Condition::GREATER_THAN:
            return Condition::ABOVE;
        case Condition::GREATER_THAN_OR_EQUAL:
            return Condition::ABOVE_OR_EQUAL;
        default:
            UNREACHABLE();
    }
}

void EmitTaggedBooleanFromFloat64Compare(ArkSteedAssembler *assembler_, ArkSteedRegister dst,
                                         Condition condition)
{
    Label trueLabel;
    Label falseLabel;
    Label done;
    if (condition == Condition::NOT_EQUAL) {
        __ JumpIf(Condition::PARITY, &trueLabel);
        __ JumpIf(OrderedFloat64Condition(condition), &trueLabel);
        __ Jump(&falseLabel);
    } else {
        __ JumpIf(Condition::PARITY, &falseLabel);
        __ JumpIf(OrderedFloat64Condition(condition), &trueLabel);
        __ Jump(&falseLabel);
    }
    __ Bind(&trueLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::True().GetRawData());
    __ Jump(&done);
    __ Bind(&falseLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::False().GetRawData());
    __ Bind(&done);
}

void BranchOnFloat64Compare(ArkSteedAssembler *assembler_, Condition condition, Label *ifTrue, Label *ifFalse)
{
    if (condition == Condition::NOT_EQUAL) {
        __ JumpIf(Condition::PARITY, ifTrue);
        __ JumpIf(OrderedFloat64Condition(condition), ifTrue);
        __ Jump(ifFalse);
        return;
    }
    __ JumpIf(Condition::PARITY, ifFalse);
    __ JumpIf(OrderedFloat64Condition(condition), ifTrue);
    __ Jump(ifFalse);
}
}  // namespace

Label *ArkSteedCodeGenerator::RecordEagerDeoptTarget(
    const EagerDeoptimizableMixin *vertex, kungfu::DeoptType type)
{
    ASSERT(safepointBuilder_ != nullptr);
    ASSERT(translationBuilder_ != nullptr);

    uint32_t bytecodeOffset = vertex->GetBytecodeOffset();
    auto translationInputs = BuildDeoptTranslationInputs(assembler_, vertex, deoptLiteralTableBuilder_);
    DeoptId deoptId = translationBuilder_->AddTranslation(bytecodeOffset, type, std::move(translationInputs));

    if (deoptId.value >= eagerDeoptTargetsById_.size()) {
        eagerDeoptTargetsById_.resize(static_cast<size_t>(deoptId.value) + 1U, nullptr);
    }
    EagerDeoptTarget *target = eagerDeoptTargetsById_[deoptId.value];
    if (target == nullptr) {
        target = graph_->GetChunk()->New<EagerDeoptTarget>(deoptId);
        eagerDeoptTargetsById_[deoptId.value] = target;
    } else {
        ASSERT(target->deoptId == deoptId);
    }
    return &target->label;
}

void ArkSteedCodeGenerator::BranchToEagerDeoptTarget(
    Condition condition, const EagerDeoptimizableMixin *vertex, kungfu::DeoptType type)
{
    __ JumpIf(condition, RecordEagerDeoptTarget(vertex, type));
}

void ArkSteedCodeGenerator::EmitEagerDeoptExit(
    const EagerDeoptimizableMixin *vertex, kungfu::DeoptType type)
{
    __ Jump(RecordEagerDeoptTarget(vertex, type));
}

void ArkSteedCodeGenerator::EmitEagerDeoptStackOverflow()
{
    constexpr auto stubId = kungfu::RuntimeStubCSigns::ID_ThrowStackOverflowException;
    constexpr int stackArgCount = CALL_ARG2;
    __ ReserveCallArgSlots(stackArgCount);
    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(stubId));
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG0), scratch);
        __ Move(scratch, 0);
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG1), scratch);
    }
    __ CallRuntime(stubId);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ FreeCallArgSlots(stackArgCount);
    EmitReturnWithPendingException();
}

void ArkSteedCodeGenerator::EmitQueuedEagerDeoptExits()
{
    if (eagerDeoptTargetsById_.empty()) {
        return;
    }

    Label deoptVeneer;

    // Every fixed exit calls this one function-local veneer. Until the global
    // entry owns the snapshot, the veneer may touch only the two
    // architecture-reserved eager-deopt registers.
    __ Bind(&deoptVeneer);
    __ CallArkSteedDeoptimizationEntry();
    __ NormalizeEagerDeoptOverflowLink();
    EmitEagerDeoptStackOverflow();

    constexpr uint32_t exitSize = ARKSTEED_EAGER_DEOPT_EXIT_SIZE;
    CHECK(eagerDeoptTargetsById_.size() <=
          static_cast<size_t>(std::numeric_limits<uint32_t>::max() / exitSize));
    uint32_t exitClusterSize =
        static_cast<uint32_t>(eagerDeoptTargetsById_.size() * exitSize);
#if defined(PANDA_TARGET_ARM64)
    // No pool may be emitted inside the fixed-size exit cluster.
    __ CheckCodePools(false, exitClusterSize);
#endif

    uint32_t exitStartOffset = __ GetPcOffset();
    for (size_t index = 0; index < eagerDeoptTargetsById_.size(); ++index) {
        EagerDeoptTarget *target = eagerDeoptTargetsById_[index];
        ASSERT(target->deoptId.value == static_cast<uint32_t>(index));
        __ Bind(&target->label);
        CHECK(target->label.GetPos() ==
              exitStartOffset + static_cast<uint32_t>(index) * exitSize);
        uint32_t before = __ GetPcOffset();
        __ Call(&deoptVeneer);
        CHECK(__ GetPcOffset() - before == exitSize);
    }
    CHECK(__ GetPcOffset() - exitStartOffset == exitClusterSize);
}

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
        case VertexOpcode::HeapConstant:
            constVertex->Cast<HeapConstantVertex>()->DoLoadToRegister(assembler_, reg);
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
    __ JumpIf(Condition::GREATER_THAN, &countDone);
    __ Move(slotCount, static_cast<int32_t>(userArgc));

    __ Bind(&countDone);
    __ Add(slotCount, static_cast<int32_t>(NUM_MANDATORY_JSFUNC_ARGS + 1));
    __ Add(slotCount, static_cast<int32_t>(1));
    __ And(slotCount, static_cast<int32_t>(~1));
}

void ArkSteedCodeGenerator::PrepareArkSteedCall(CallVertex *call, ArkSteedRegister target, ArkSteedRegister scratch)
{
    const uint32_t userArgc = call->GetActualArgc();
    const uint32_t totalArgc = userArgc + NUM_MANDATORY_JSFUNC_ARGS;

    LoadSteedExpectedArgc(target, scratch);
    ComputeSteedCallSlotCount(call, scratch);
    __ ReserveCallArgSlots(scratch);
    __ Sub(scratch, static_cast<int32_t>(NUM_MANDATORY_JSFUNC_ARGS + 1 + userArgc));
    __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG0),
                         scratch);

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

void ArkSteedCodeGenerator::EmitCallArkSteed(CallVertex *call, ArkSteedRegister target, ArkSteedRegister scratch,
                                            Label *exit)
{
    PrepareArkSteedCall(call, target, scratch);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister codeEntry = scope.AcquireScratch();
    __ PrepareSteedCalleeContext(target, codeEntry);
    __ Call(codeEntry);
    if (HasLazyDeoptSafepoint(call)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, call);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    FreeArkSteedCallFrame(call);
    __ Jump(exit);
}

void ArkSteedCodeGenerator::EmitCallGeneric(CallVertex *call, ArkSteedRegister scratch)
{
    int stackArgCount = PrepareTrampolineArguments(call, scratch);
    __ CallTrampoline(RTSTUB_ID(JSCall));
    if (HasLazyDeoptSafepoint(call)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, call);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    __ FreeCallArgSlots(stackArgCount);
}

void ArkSteedCodeGenerator::EmitReturnWithPendingException()
{
    constexpr auto stubId = kungfu::RuntimeStubCSigns::ID_UpFrame;
    constexpr int stackArgCount = 4;
    constexpr int argCount = 1;
    constexpr int prepareExceptionLazyDeopt = 1;
    __ ReserveCallArgSlots(stackArgCount);
    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(stubId));
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG0), scratch);
        __ Move(scratch, argCount);
        __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG1), scratch);
        __ Move(scratch, JSTaggedValue(prepareExceptionLazyDeopt).GetRawData());
        __ MoveRepr(MachineRepresentation::Tagged, __ GetCallArgSlot(CALL_ARG2), scratch);
    }
    __ CallRuntime(stubId);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ FreeCallArgSlots(stackArgCount);
    __ ReturnWithPendingException();
}

void ArkSteedCodeGenerator::EmitReturnIfPendingException()
{
    Label noPendingException;
    __ BranchIfNoPendingException(&noPendingException);
    EmitReturnWithPendingException();
    __ Bind(&noPendingException);
}

int ArkSteedCodeGenerator::PrepareTrampolineArguments(CallVertex *call, ArkSteedRegister scratch)
{
    uint32_t userArgc = call->GetActualArgc();
    uint32_t totalArgc = userArgc + NUM_MANDATORY_JSFUNC_ARGS;
    // stack layout: totalArgc, actualArgV, target, newTarget, this, userArgs
    uint32_t stackArgCount = totalArgc + CALL_ARG2;
    uint32_t reservedSlotCount = (stackArgCount + 1) & ~1U;
    __ ReserveCallArgSlots(static_cast<int32_t>(reservedSlotCount));

    __ Move(scratch, static_cast<int64_t>(totalArgc));
    __ MoveRepr(MachineRepresentation::Word64,
                         __ GetCallArgSlot(CALL_ARG0), scratch);
    __ Move(scratch, 0);
    __ MoveRepr(MachineRepresentation::Word64,
                         __ GetCallArgSlot(CALL_ARG1), scratch);

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

    int stackArgCount = PrepareRuntimeStubStackArguments(
        callRuntime, callRuntime->GetArgCount(), static_cast<int>(callRuntime->GetRuntimeStubID()));
    __ CallRuntime(callRuntime->GetRuntimeStubID());
    if (HasLazyDeoptSafepoint(callRuntime)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, callRuntime);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    __ FreeCallArgSlots(stackArgCount);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CallVertex>(CallVertex *call)
{
    Label callGeneric;
    Label exit;
    ArkSteedRegister target = GetInputRegister(call, CallVertex::TARGET_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    __ JumpIfNotTaggedHeapObject(target, &callGeneric);
    __ JumpIfNotJSFunction(target, &callGeneric);
    __ JumpIfClassConstructor(target, &callGeneric);
    __ JumpIfFunctionNotCompiled(target, &callGeneric);
    EmitCallArkSteed(call, target, scratch, &exit);
    __ Bind(&callGeneric);
    EmitCallGeneric(call, scratch);
    __ Bind(&exit);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfHClassMismatchVertex>(DeoptIfHClassMismatchVertex *checkHClass)
{
    constexpr int RECEIVER_INDEX = static_cast<int>(DeoptIfHClassMismatchVertex::RECEIVER_INDEX);
    ASSERT(safepointBuilder_ != nullptr);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister actualHClass = scope.Acquire();
    ArkSteedRegister expectedHClass = scope.Acquire();
    ArkSteedRegister receiver = GetInputRegister(checkHClass, RECEIVER_INDEX);
#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(checkHClass, kungfu::DeoptType::KEYMISSMATCH);
#else
    Label deoptLabel;
    Label pass;
    Label *deopt = &deoptLabel;
#endif
    __ Move(actualHClass, receiver);
    __ Move(expectedHClass, static_cast<uint64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ And(actualHClass, expectedHClass);
    __ Compare(actualHClass, 0);
    __ JumpIf(Condition::NOT_EQUAL, deopt);

    __ LoadField(actualHClass, receiver, TaggedObject::HCLASS_OFFSET);
    __ Move(expectedHClass, TaggedStateWord::ADDRESS_MASK);
    __ And(actualHClass, expectedHClass);
    __ MoveEmbeddedTagged(expectedHClass, checkHClass->GetExpectedHClassHandleIndex());
    __ Compare(actualHClass, expectedHClass);
#if defined(PANDA_TARGET_AMD64)
    __ JumpIf(Condition::NOT_EQUAL, deopt);
#else
    __ JumpIf(Condition::EQUAL, &pass);
    __ Bind(deopt);
    EmitEagerDeoptExit(checkHClass, kungfu::DeoptType::KEYMISSMATCH);
    __ Bind(&pass);
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfHClassNotInVertex>(DeoptIfHClassNotInVertex *checkHClass)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << checkHClass->GetId() << ": DeoptIfHClassNotInVertex";
#endif
    constexpr int RECEIVER_INDEX = static_cast<int>(DeoptIfHClassNotInVertex::RECEIVER_INDEX);
    ASSERT(safepointBuilder_ != nullptr);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister actualHClass = scope.Acquire();
    ArkSteedRegister expectedHClass = scope.Acquire();
    ArkSteedRegister receiver = GetInputRegister(checkHClass, RECEIVER_INDEX);
    Label deopt;
    Label pass;
    __ Move(actualHClass, receiver);
    __ Move(expectedHClass, static_cast<uint64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ And(actualHClass, expectedHClass);
    __ Compare(actualHClass, 0);
    __ JumpIf(Condition::NOT_EQUAL, &deopt);

    __ LoadField(actualHClass, receiver, TaggedObject::HCLASS_OFFSET);
    __ Move(expectedHClass, TaggedStateWord::ADDRESS_MASK);
    __ And(actualHClass, expectedHClass);
    for (uint32_t i = 0; i < checkHClass->GetExpectedHClassCount(); ++i) {
        __ MoveEmbeddedTagged(expectedHClass, checkHClass->GetExpectedHClassHandleIndex(i));
        __ Compare(actualHClass, expectedHClass);
        __ JumpIf(Condition::EQUAL, &pass);
    }

    __ Bind(&deopt);
    EmitEagerDeoptExit(checkHClass, kungfu::DeoptType::KEYMISSMATCH);

    __ Bind(&pass);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfPrototypeChangedVertex>(
    DeoptIfPrototypeChangedVertex *checkPrototype)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << checkPrototype->GetId()
                        << ": DeoptIfPrototypeChangedVertex";
#endif
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister current = scope.Acquire();
    ArkSteedRegister scratch = scope.Acquire();
    ArkSteedRegister receiver = GetInputRegister(checkPrototype, DeoptIfPrototypeChangedVertex::RECEIVER_INDEX);
    Label protoMarkerDeopt;
    Label prototypeHClassDeopt;
    Label pass;

    auto loadHClassAddress = [this, scratch](ArkSteedRegister dst, ArkSteedRegister object) {
        __ LoadField(dst, object, TaggedObject::HCLASS_OFFSET);
        __ Move(scratch, TaggedStateWord::ADDRESS_MASK);
        __ And(dst, scratch);
    };

    if (checkPrototype->ShouldCheckNotPrototype()) {
        loadHClassAddress(current, receiver);
        __ LoadInt32Field(current, current, static_cast<int32_t>(JSHClass::BIT_FIELD_OFFSET));
        constexpr int32_t isPrototypeMask = 1U << JSHClass::IsPrototypeBit::START_BIT;
        __ And(current, isPrototypeMask);
        __ Compare(current, 0);
        __ JumpIf(Condition::NOT_EQUAL, &prototypeHClassDeopt);
    }

    if (checkPrototype->ShouldCheckProtoChangeMarker()) {
        loadHClassAddress(current, receiver);
        __ LoadField(current, current, static_cast<int32_t>(JSHClass::PROTOTYPE_OFFSET));
        __ Move(scratch, current);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &protoMarkerDeopt);

        loadHClassAddress(current, current);
        __ LoadField(current, current, static_cast<int32_t>(JSHClass::PROTO_CHANGE_MARKER_OFFSET));
        __ Move(scratch, current);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &protoMarkerDeopt);

        __ LoadInt32Field(current, current, static_cast<int32_t>(ProtoChangeMarker::BIT_FIELD_OFFSET));
        constexpr int32_t invalidatingChangeMask =
            (1U << (ProtoChangeMarker::HAS_CHANGED_BITS - 1U)) |
            (1U << ProtoChangeMarker::AccessorHasChangedBits::START_BIT);
        __ And(current, invalidatingChangeMask);
        __ Compare(current, 0);
        __ JumpIf(Condition::NOT_EQUAL, &protoMarkerDeopt);
    }
    __ Jump(&pass);

    if (checkPrototype->ShouldCheckProtoChangeMarker()) {
        __ Bind(&protoMarkerDeopt);
        EmitEagerDeoptExit(checkPrototype, kungfu::DeoptType::PROTOTYPECHANGED2);
    }
    if (checkPrototype->ShouldCheckNotPrototype()) {
        __ Bind(&prototypeHClassDeopt);
        EmitEagerDeoptExit(checkPrototype, kungfu::DeoptType::PROTOTYPECHANGED3);
    }
    __ Bind(&pass);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfTaggedConditionVertex>(
    DeoptIfTaggedConditionVertex *check)
{
    auto left = GetInputRegister(check, DeoptIfTaggedConditionVertex::LEFT_INDEX);
    auto right = GetInputRegister(check, DeoptIfTaggedConditionVertex::RIGHT_INDEX);
    __ Compare(left, right);
    BranchToEagerDeoptTarget(check->GetCondition(), check, check->GetDeoptType());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfInt32ConditionVertex>(DeoptIfInt32ConditionVertex *check)
{
    auto left = GetInputRegister(check, DeoptIfInt32ConditionVertex::LEFT_INDEX);
    auto right = GetInputRegister(check, DeoptIfInt32ConditionVertex::RIGHT_INDEX);
    __ CompareInt32(left, right);
    BranchToEagerDeoptTarget(check->GetCondition(), check, check->GetDeoptType());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfFloat64ConditionVertex>(
    DeoptIfFloat64ConditionVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": DeoptIfFloat64ConditionVertex";
#endif
    auto left = GetInputDoubleRegister(check, DeoptIfFloat64ConditionVertex::LEFT_INDEX);
    auto right = GetInputDoubleRegister(check, DeoptIfFloat64ConditionVertex::RIGHT_INDEX);
#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(check, check->GetDeoptType());
#else
    Label deoptLabel;
    Label *deopt = &deoptLabel;
#endif
    Label done;
    __ CompareFloat64(left, right);
    BranchOnFloat64Compare(assembler_, check->GetCondition(), deopt, &done);
#if !defined(PANDA_TARGET_AMD64)
    __ Bind(deopt);
    EmitEagerDeoptExit(check, check->GetDeoptType());
#endif
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfNotNumberVertex>(DeoptIfNotNumberVertex *check)
{
    auto value = GetInputRegister(check, DeoptIfNotNumberVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister bits = scope.Acquire();
    Label done;
    __ Move(bits, value);
    __ And(bits, static_cast<int64_t>(JSTaggedValue::TAG_MARK));
    __ Compare(bits, static_cast<int64_t>(JSTaggedValue::TAG_MARK));
    __ JumpIf(Condition::EQUAL, &done);

#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(check, kungfu::DeoptType::NOTNUMBER1);
#else
    Label deoptLabel;
    Label *deopt = &deoptLabel;
#endif
    __ Compare(value, static_cast<int64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ JumpIf(Condition::BELOW, deopt);
    __ Compare(value, static_cast<int64_t>(JSTaggedValue::TAG_INT));
    __ JumpIf(Condition::BELOW, &done);
    __ JumpIf(Condition::ABOVE_OR_EQUAL, deopt);
#if !defined(PANDA_TARGET_AMD64)
    __ Bind(deopt);
    EmitEagerDeoptExit(check, kungfu::DeoptType::NOTNUMBER1);
#endif
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfNotHeapObjectVertex>(DeoptIfNotHeapObjectVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": DeoptIfNotHeapObjectVertex";
#endif
    auto value = GetInputRegister(check, DeoptIfNotHeapObjectVertex::VALUE_INDEX);
#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(check, kungfu::DeoptType::NOTHEAPOBJECT1);
#else
    Label deoptLabel;
    Label *deopt = &deoptLabel;
#endif
    Label done;
    __ JumpIfNotTaggedHeapObject(value, deopt);
    __ Jump(&done);
#if !defined(PANDA_TARGET_AMD64)
    __ Bind(deopt);
    EmitEagerDeoptExit(check, kungfu::DeoptType::NOTHEAPOBJECT1);
#endif
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfArrayBufferDetachedVertex>(
    DeoptIfArrayBufferDetachedVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId()
                        << ": DeoptIfArrayBufferDetachedVertex";
#endif
    auto receiver = GetInputRegister(check, DeoptIfArrayBufferDetachedVertex::RECEIVER_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    Label done;

    if (!OnHeap::IsNotOnHeap(check->GetOnHeapMode())) {
        LoadHClassBitField(assembler_, scratch, receiver);
        __ And(scratch, static_cast<int64_t>(1U << JSHClass::IsOnHeap::START_BIT));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &done);
    }

    __ LoadField(scratch, receiver, static_cast<int32_t>(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET));
    __ LoadField(scratch, scratch, static_cast<int32_t>(JSArrayBuffer::DATA_OFFSET));
    __ Compare(scratch, static_cast<int64_t>(JSTaggedValue::VALUE_NULL));
    BranchToEagerDeoptTarget(Condition::EQUAL, check, kungfu::DeoptType::ARRAYBUFFERISDETACHED);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfCOWElementsVertex>(DeoptIfCOWElementsVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": DeoptIfCOWElementsVertex";
#endif
    auto elements = GetInputRegister(check, DeoptIfCOWElementsVertex::ELEMENTS_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister objectType = scope.Acquire();
#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(check, kungfu::DeoptType::INCONSISTENTELEMENTSKIND);
#else
    Label deoptLabel;
    Label *deopt = &deoptLabel;
#endif
    Label done;
    LoadObjectType(assembler_, objectType, elements);
    __ Compare(objectType, static_cast<int32_t>(JSType::COW_TAGGED_ARRAY));
    __ JumpIf(Condition::EQUAL, deopt);
    __ Compare(objectType, static_cast<int32_t>(JSType::COW_MUTANT_TAGGED_ARRAY));
    __ JumpIf(Condition::NOT_EQUAL, &done);
#if !defined(PANDA_TARGET_AMD64)
    __ Bind(deopt);
    EmitEagerDeoptExit(check, kungfu::DeoptType::INCONSISTENTELEMENTSKIND);
#endif
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DeoptIfElementsUnstableVertex>(
    DeoptIfElementsUnstableVertex *check)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << check->GetId() << ": DeoptIfElementsUnstableVertex";
#endif
    auto receiver = GetInputRegister(check, DeoptIfElementsUnstableVertex::RECEIVER_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister bitField = scope.Acquire();
#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(check, kungfu::DeoptType::NOTSARRAY2);
#else
    Label deoptLabel;
    Label *deopt = &deoptLabel;
#endif
    Label done;
    LoadHClassBitField(assembler_, bitField, receiver);
    __ And(bitField, static_cast<int64_t>(1U << JSHClass::IsStableElementsBit::START_BIT));
    __ Compare(bitField, 0);
    __ JumpIf(Condition::EQUAL, deopt);
    __ Jump(&done);
#if !defined(PANDA_TARGET_AMD64)
    __ Bind(deopt);
    EmitEagerDeoptExit(check, kungfu::DeoptType::NOTSARRAY2);
#endif
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<DeoptVertex>(DeoptVertex *deopt)
{
    EmitEagerDeoptExit(deopt, deopt->GetDeoptType());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<InitialValueVertex>(InitialValueVertex *initialValue)
{
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ActualArgcVertex>(ActualArgcVertex *actualArgc)
{
    auto dst = GetResultRegister(actualArgc);
    __ LoadActualArgc(dst);
}

// ========================================= Slow Value Opcode =========================================

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadTaggedFromAddressVertex>(LoadTaggedFromAddressVertex *loadField)
{
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadTaggedFromAddressVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadI32FromAddressVertex>(LoadI32FromAddressVertex *loadField)
{
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadI32FromAddressVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadI64FromAddressVertex>(LoadI64FromAddressVertex *loadField)
{
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadI64FromAddressVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadF64FromAddressVertex>(LoadF64FromAddressVertex *loadField)
{
    auto dst = GetResultDoubleRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadF64FromAddressVertex::OBJECT_INDEX);
    __ LoadFloat64(dst, ArkSteedAssembler::MemoryOperand(obj, loadField->GetOffset()));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadExceptionVertex>(LoadExceptionVertex *loadException)
{
    auto dst = GetResultRegister(loadException);
    auto glue = GetInputRegister(loadException, LoadExceptionVertex::GLUE_INDEX);
    __ LoadAndClearPendingException(dst, glue);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadTaggedFieldVertex>(LoadTaggedFieldVertex *loadField)
{
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadTaggedFieldVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadInt32FieldVertex>(LoadInt32FieldVertex *loadField)
{
    auto dst = GetResultRegister(loadField);
    auto obj = GetInputRegister(loadField, LoadInt32FieldVertex::OBJECT_INDEX);
    __ LoadInt32Field(dst, obj, loadField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadTaggedElementVertex>(LoadTaggedElementVertex *loadElement)
{
    auto dst = GetResultRegister(loadElement);
    auto elements = GetInputRegister(loadElement, LoadTaggedElementVertex::ELEMENTS_INDEX);
    auto index = GetInputRegister(loadElement, LoadTaggedElementVertex::INDEX_INDEX);
    __ LoadTaggedElement(dst, elements, index);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadSingleCharTableElementVertex>(
    LoadSingleCharTableElementVertex *loadElement)
{
    auto dst = GetResultRegister(loadElement);
    auto glue = GetInputRegister(loadElement, LoadSingleCharTableElementVertex::GLUE_INDEX);
    auto charCode = GetInputRegister(loadElement, LoadSingleCharTableElementVertex::CHAR_CODE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister singleCharTable = scope.Acquire();

    constexpr int32_t singleCharTableOffset = static_cast<int32_t>(ConstantIndex::SINGLE_CHAR_TABLE_INDEX) *
                                              static_cast<int32_t>(JSTaggedValue::TaggedTypeSize());
    __ LoadField(singleCharTable, glue, static_cast<int32_t>(JSThread::GlueData::GetGlobalConstOffset(false)));
    __ LoadField(singleCharTable, singleCharTable, singleCharTableOffset);
    __ LoadTaggedElement(dst, singleCharTable, charCode);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadPrototypeFromObjectVertex>(
    LoadPrototypeFromObjectVertex *loadPrototype)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadPrototype->GetId()
                        << ": LoadPrototypeFromObjectVertex";
#endif
    auto dst = GetResultRegister(loadPrototype);
    auto obj = GetInputRegister(loadPrototype, LoadPrototypeFromObjectVertex::OBJECT_INDEX);
    __ LoadField(dst, obj, TaggedObject::HCLASS_OFFSET);
    __ And(dst, static_cast<int64_t>(TaggedStateWord::ADDRESS_MASK));
    __ LoadField(dst, dst, JSHClass::PROTOTYPE_OFFSET);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadPrototypeHolderByHClassVertex>(
    LoadPrototypeHolderByHClassVertex *loadHolder)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadHolder->GetId()
                        << ": LoadPrototypeHolderByHClassVertex";
#endif
    constexpr int RECEIVER_INDEX = static_cast<int>(LoadPrototypeHolderByHClassVertex::RECEIVER_INDEX);
    ASSERT(safepointBuilder_ != nullptr);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister currentHClass = scope.Acquire();
    ArkSteedRegister expectedHClass = scope.Acquire();
    ArkSteedRegister receiver = GetInputRegister(loadHolder, RECEIVER_INDEX);
    ArkSteedRegister holder = GetResultRegister(loadHolder);
    Label protoChanged;
    Label deopt;
    Label pass;

    __ LoadField(currentHClass, receiver, TaggedObject::HCLASS_OFFSET);
    __ And(currentHClass, static_cast<int64_t>(TaggedStateWord::ADDRESS_MASK));
    __ LoadField(holder, currentHClass, JSHClass::PROTOTYPE_OFFSET);

    __ Move(expectedHClass, static_cast<int64_t>(JSTaggedValue::VALUE_NULL));
    __ Compare(holder, expectedHClass);
    __ JumpIf(Condition::EQUAL, &deopt);
    __ LoadField(currentHClass, holder, TaggedObject::HCLASS_OFFSET);
    __ And(currentHClass, static_cast<int64_t>(TaggedStateWord::ADDRESS_MASK));
    __ LoadField(expectedHClass, currentHClass, JSHClass::PROTO_CHANGE_MARKER_OFFSET);
    __ Move(currentHClass, static_cast<int64_t>(JSTaggedValue::VALUE_NULL));
    __ Compare(expectedHClass, currentHClass);
    __ JumpIf(Condition::EQUAL, &protoChanged);
    __ And(expectedHClass, static_cast<int64_t>(TaggedStateWord::ADDRESS_MASK));
    __ LoadField(currentHClass, expectedHClass, ProtoChangeMarker::BIT_FIELD_OFFSET);
    __ And(currentHClass, static_cast<int64_t>((1LLU << (ProtoChangeMarker::HAS_CHANGED_BITS - 1))));
    __ Compare(currentHClass, 0);
    __ JumpIf(Condition::NOT_EQUAL, &protoChanged);

    for (uint32_t i = 0; i < loadHolder->GetHolderDepth(); ++i) {
        __ Move(expectedHClass, static_cast<int64_t>(JSTaggedValue::VALUE_NULL));
        __ Compare(holder, expectedHClass);
        __ JumpIf(Condition::EQUAL, &deopt);
        __ LoadField(currentHClass, holder, TaggedObject::HCLASS_OFFSET);
        __ And(currentHClass, static_cast<int64_t>(TaggedStateWord::ADDRESS_MASK));
        __ MoveEmbeddedTagged(expectedHClass, loadHolder->GetExpectedHClassHandleIndex(i));
        __ Compare(currentHClass, expectedHClass);
        __ JumpIf(Condition::NOT_EQUAL, &protoChanged);
        if (i + 1 < loadHolder->GetHolderDepth()) {
            __ LoadField(holder, currentHClass, JSHClass::PROTOTYPE_OFFSET);
        }
    }
    __ Jump(&pass);

    __ Bind(&protoChanged);
    EmitEagerDeoptExit(loadHolder, kungfu::DeoptType::PROTOTYPECHANGED2);

    __ Bind(&deopt);
    EmitEagerDeoptExit(loadHolder, kungfu::DeoptType::INCONSISTENTHCLASS2);
    __ Bind(&pass);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<ConvertHoleToUndefinedVertex>(
    ConvertHoleToUndefinedVertex *convert)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << convert->GetId() << ": ConvertHoleToUndefinedVertex";
#endif
    auto dst = GetResultRegister(convert);
    auto value = GetInputRegister(convert, ConvertHoleToUndefinedVertex::VALUE_INDEX);
    Label done;
    __ Move(dst, value);
    __ Compare(dst, static_cast<int64_t>(JSTaggedValue::Hole().GetRawData()));
    __ JumpIf(Condition::NOT_EQUAL, &done);
    __ Move(dst, static_cast<int64_t>(JSTaggedValue::VALUE_UNDEFINED));
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LoadHClassAddressVertex>(LoadHClassAddressVertex *loadHClass)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << loadHClass->GetId() << ": LoadHClassAddressVertex";
#endif
    auto dst = GetResultRegister(loadHClass);
    auto object = GetInputRegister(loadHClass, LoadHClassAddressVertex::OBJECT_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister mask = scope.Acquire();
    ASSERT(dst != mask);
    __ LoadField(dst, object, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
    __ Move(mask, TaggedStateWord::ADDRESS_MASK);
    __ And(dst, mask);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<FindPrototypeHolderVertex>(
    FindPrototypeHolderVertex *findHolder)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << findHolder->GetId() << ": FindPrototypeHolderVertex";
#endif
    auto holder = GetResultRegister(findHolder);
    auto receiver = GetInputRegister(findHolder, FindPrototypeHolderVertex::RECEIVER_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister currentHClass = scope.Acquire();
    ArkSteedRegister scratch = scope.Acquire();
    ASSERT(holder != currentHClass && holder != scratch && currentHClass != scratch);

    auto loadHClassAddress = [this, scratch](ArkSteedRegister dst, ArkSteedRegister object) {
        __ LoadField(dst, object, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
        __ Move(scratch, TaggedStateWord::ADDRESS_MASK);
        __ And(dst, scratch);
    };

    Label loop;
    Label found;
    Label deopt;
    loadHClassAddress(currentHClass, receiver);
    __ LoadField(holder, currentHClass, static_cast<int32_t>(JSHClass::PROTOTYPE_OFFSET));
    __ Bind(&loop);
    __ Move(scratch, holder);
    __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ Compare(scratch, 0);
    __ JumpIf(Condition::NOT_EQUAL, &deopt);

    loadHClassAddress(currentHClass, holder);
    __ MoveEmbeddedTagged(scratch, findHolder->GetExpectedHClassHandleIndex());
    __ Compare(currentHClass, scratch);
    __ JumpIf(Condition::EQUAL, &found);
    __ LoadField(holder, currentHClass, static_cast<int32_t>(JSHClass::PROTOTYPE_OFFSET));
    __ Jump(&loop);

    __ Bind(&deopt);
    EmitEagerDeoptExit(findHolder, kungfu::DeoptType::INCONSISTENTHCLASS4);
    __ Bind(&found);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedToAddressVertex>(StoreTaggedToAddressVertex *storeField)
{
    auto obj = GetInputRegister(storeField, StoreTaggedToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedToAddressVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreI32ToAddressVertex>(StoreI32ToAddressVertex *storeField)
{
    auto obj = GetInputRegister(storeField, StoreI32ToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreI32ToAddressVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreI64ToAddressVertex>(StoreI64ToAddressVertex *storeField)
{
    auto obj = GetInputRegister(storeField, StoreI64ToAddressVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreI64ToAddressVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreF64ToAddressVertex>(StoreF64ToAddressVertex *storeField)
{
    auto obj = GetInputRegister(storeField, StoreF64ToAddressVertex::OBJECT_INDEX);
    auto value = GetInputDoubleRegister(storeField, StoreF64ToAddressVertex::VALUE_INDEX);
    __ StoreFloat64(ArkSteedAssembler::MemoryOperand(obj, storeField->GetOffset()), value);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedFieldVertex>(StoreTaggedFieldVertex *storeField)
{
    auto obj = GetInputRegister(storeField, StoreTaggedFieldVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedFieldVertex::VALUE_INDEX);
    __ StoreField(value, obj, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedFieldWithBarrierVertex>(
    StoreTaggedFieldWithBarrierVertex *storeField)
{
    auto glue = GetInputRegister(storeField, StoreTaggedFieldWithBarrierVertex::GLUE_INDEX);
    auto object = GetInputRegister(storeField, StoreTaggedFieldWithBarrierVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreTaggedFieldWithBarrierVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister objectRegionScratch = scope.Acquire();
    ArkSteedRegister valueRegionScratch = scope.Acquire();
    ASSERT(objectRegionScratch != valueRegionScratch);
    ArkSteedWriteBarrierEmitter(assembler_, graph_->GetChunk(), &deferredCode_,
                                storeField->GetRegallocInfo()->GetDeferredRegisterSnapshot())
        .StoreTaggedField(glue, object, value, storeField->GetOffset(), ArkSteedWriteBarrierKind::GENERIC_BARRIER,
                          objectRegionScratch, valueRegionScratch, storeField->GetValueKind());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedElementVertex>(StoreTaggedElementVertex *store)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << store->GetId() << ": StoreTaggedElementVertex";
#endif
    auto object = GetInputRegister(store, StoreTaggedElementVertex::OBJECT_INDEX);
    auto index = GetInputRegister(store, StoreTaggedElementVertex::INDEX_INDEX);
    auto value = GetInputRegister(store, StoreTaggedElementVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    __ StoreTaggedElement(object, index, value, scratch);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedElementWithBarrierVertex>(
    StoreTaggedElementWithBarrierVertex *store)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << store->GetId() << ": StoreTaggedElementWithBarrierVertex";
#endif
    auto glue = GetInputRegister(store, StoreTaggedElementWithBarrierVertex::GLUE_INDEX);
    auto object = GetInputRegister(store, StoreTaggedElementWithBarrierVertex::OBJECT_INDEX);
    auto index = GetInputRegister(store, StoreTaggedElementWithBarrierVertex::INDEX_INDEX);
    auto value = GetInputRegister(store, StoreTaggedElementWithBarrierVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister byteOffset = scope.AcquireSpecific(ArkSteedAssembler::GetParameterRegister(2));
    ArkSteedRegister tagScratch = scope.Acquire();
    ASSERT(tagScratch != byteOffset);
    __ Move(byteOffset, index);
    __ ShiftLeft(byteOffset, TAGGED_TYPE_SIZE_LOG);
    __ Add(byteOffset, static_cast<int32_t>(TaggedArray::DATA_OFFSET));
    ArkSteedWriteBarrierEmitter(assembler_, graph_->GetChunk(), &deferredCode_,
                                store->GetRegallocInfo()->GetDeferredRegisterSnapshot())
        .StoreTaggedElement(glue, object, byteOffset, value, tagScratch, store->GetValueKind());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreIntTypedArrayElementVertex>(
    StoreIntTypedArrayElementVertex *store)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << store->GetId() << ": StoreIntTypedArrayElementVertex";
#endif
    auto receiver = GetInputRegister(store, StoreIntTypedArrayElementVertex::RECEIVER_INDEX);
    auto index = GetInputRegister(store, StoreIntTypedArrayElementVertex::INDEX_INDEX);
    auto value = GetInputRegister(store, StoreIntTypedArrayElementVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister data = scope.Acquire();
    OnHeapMode onHeapMode = store->GetOnHeapMode();
    ArkSteedRegister backingOffset = OnHeap::IsOnHeap(onHeapMode) ? data : scope.Acquire();
    BuildTypedArrayDataPointer(assembler_, receiver, onHeapMode, data, backingOffset);
    __ StoreTypedArrayIntElement(value, data, index, store->GetType());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreFloatTypedArrayElementVertex>(
    StoreFloatTypedArrayElementVertex *store)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << store->GetId() << ": StoreFloatTypedArrayElementVertex";
#endif
    auto receiver = GetInputRegister(store, StoreFloatTypedArrayElementVertex::RECEIVER_INDEX);
    auto index = GetInputRegister(store, StoreFloatTypedArrayElementVertex::INDEX_INDEX);
    auto value = GetInputDoubleRegister(store, StoreFloatTypedArrayElementVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister base = scope.Acquire();
    ArkSteedRegister byteOffset = scope.Acquire();
    OnHeapMode onHeapMode = store->GetOnHeapMode();
    ArkSteedRegister backingOffset = OnHeap::IsOnHeap(onHeapMode) ? base : scope.Acquire();
    BuildTypedArrayElementAddress(assembler_, receiver, index, store->GetType(), onHeapMode, base, byteOffset,
                                  backingOffset);
    ArkSteedDoubleRegister normalized = scope.AcquireDouble();
    CanonicalizeNaN(assembler_, value, normalized, byteOffset);
    if (store->GetType() == JSType::JS_FLOAT32_ARRAY) {
        __ StoreFloat32Field(normalized, normalized, base, 0);
        return;
    }
    ASSERT(store->GetType() == JSType::JS_FLOAT64_ARRAY);
    __ StoreFloat64Field(normalized, base, 0);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreSharedFieldWithBarrierVertex>(
    StoreSharedFieldWithBarrierVertex *storeField)
{
    auto glue = GetInputRegister(storeField, StoreSharedFieldWithBarrierVertex::GLUE_INDEX);
    auto object = GetInputRegister(storeField, StoreSharedFieldWithBarrierVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeField, StoreSharedFieldWithBarrierVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister objectRegionScratch = scope.Acquire();
    ArkSteedRegister valueRegionScratch = scope.Acquire();
    ASSERT(objectRegionScratch != valueRegionScratch);
    ArkSteedWriteBarrierEmitter(assembler_, graph_->GetChunk(), &deferredCode_,
                                storeField->GetRegallocInfo()->GetDeferredRegisterSnapshot())
        .StoreTaggedField(glue, object, value, storeField->GetOffset(), ArkSteedWriteBarrierKind::SHARED_BARRIER,
                          objectRegionScratch, valueRegionScratch, storeField->GetValueKind());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TransitionHClassWithBarrierVertex>(
    TransitionHClassWithBarrierVertex *transition)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << transition->GetId()
                        << ": TransitionHClassWithBarrierVertex";
#endif
    auto glue = GetInputRegister(transition, TransitionHClassWithBarrierVertex::GLUE_INDEX);
    auto object = GetInputRegister(transition, TransitionHClassWithBarrierVertex::OBJECT_INDEX);
    auto hclass = GetInputRegister(transition, TransitionHClassWithBarrierVertex::HCLASS_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister objectRegionScratch = scope.Acquire();
    ArkSteedRegister hclassRegionScratch = scope.Acquire();
    ASSERT(objectRegionScratch != hclassRegionScratch);
    ArkSteedWriteBarrierEmitter(assembler_, graph_->GetChunk(), &deferredCode_,
                                transition->GetRegallocInfo()->GetDeferredRegisterSnapshot())
        .TransitionHClass(glue, object, hclass, objectRegionScratch, hclassRegionScratch);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<PrepareSharedStoreFieldVertex>(
    PrepareSharedStoreFieldVertex *prepareField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << prepareField->GetId()
                        << ": PrepareSharedStoreFieldVertex";
#endif
    uint64_t handlerInfo = prepareField->GetHandlerInfo();
    ASSERT(HandlerBase::IsStoreShared(handlerInfo));
    ASSERT(!HandlerBase::IsAccessor(handlerInfo));
    uint32_t fieldType = static_cast<uint32_t>(HandlerBase::GetFieldType(handlerInfo));
    auto value = GetInputRegister(prepareField, PrepareSharedStoreFieldVertex::VALUE_INDEX);
    auto result = GetResultRegister(prepareField);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    Label done;
    Label publishTreeString;
    Label typeMismatch;

    __ Move(result, value);
    __ LoadTaggedValue(scratch, JSTaggedValue::Undefined().GetRawData());
    __ Compare(value, scratch);
    __ JumpIf(Condition::EQUAL, &done);

    if ((fieldType & static_cast<uint32_t>(SharedFieldType::NUMBER)) != 0) {
        Label checkNext;
        __ Move(scratch, value);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_MARK));
        __ Compare(scratch, static_cast<int32_t>(JSTaggedValue::TAG_OBJECT));
        __ JumpIf(Condition::EQUAL, &checkNext);
        __ Jump(&done);
        __ Bind(&checkNext);
    }
    if ((fieldType & static_cast<uint32_t>(SharedFieldType::BOOLEAN)) != 0) {
        Label checkNext;
        __ Move(scratch, value);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, static_cast<int32_t>(JSTaggedValue::TAG_BOOLEAN_MASK));
        __ JumpIf(Condition::NOT_EQUAL, &checkNext);
        __ Jump(&done);
        __ Bind(&checkNext);
    }
    if ((fieldType & static_cast<uint32_t>(SharedFieldType::NULL_TYPE)) != 0) {
        __ LoadTaggedValue(scratch, JSTaggedValue::Null().GetRawData());
        __ Compare(value, scratch);
        __ JumpIf(Condition::EQUAL, &done);
    }
    if ((fieldType & static_cast<uint32_t>(SharedFieldType::UNDEFINED)) != 0) {
        __ LoadTaggedValue(scratch, JSTaggedValue::Undefined().GetRawData());
        __ Compare(value, scratch);
        __ JumpIf(Condition::EQUAL, &done);
    }
    if ((fieldType & static_cast<uint32_t>(SharedFieldType::STRING)) != 0) {
        Label checkNext;
        __ LoadTaggedValue(scratch, JSTaggedValue::Null().GetRawData());
        __ Compare(value, scratch);
        __ JumpIf(Condition::EQUAL, &done);
        __ Move(scratch, value);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &checkNext);
        LoadObjectType(assembler_, scratch, value);
        __ Compare(scratch, static_cast<int32_t>(JSType::STRING_FIRST));
        __ JumpIf(Condition::LESS_THAN, &checkNext);
        __ Compare(scratch, static_cast<int32_t>(JSType::STRING_LAST));
        __ JumpIf(Condition::GREATER_THAN, &checkNext);
        __ Compare(scratch, static_cast<int32_t>(JSType::TREE_STRING));
        __ JumpIf(Condition::EQUAL, &publishTreeString);
        __ Jump(&done);
        __ Bind(&checkNext);
    }
    if ((fieldType & static_cast<uint32_t>(SharedFieldType::BIG_INT)) != 0) {
        Label checkNext;
        __ Move(scratch, value);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &checkNext);
        LoadObjectType(assembler_, scratch, value);
        __ Compare(scratch, static_cast<int32_t>(JSType::BIGINT));
        __ JumpIf(Condition::EQUAL, &done);
        __ Bind(&checkNext);
    }
    if ((fieldType & static_cast<uint32_t>(SharedFieldType::SENDABLE)) != 0) {
        Label checkNext;
        __ LoadTaggedValue(scratch, JSTaggedValue::Null().GetRawData());
        __ Compare(value, scratch);
        __ JumpIf(Condition::EQUAL, &done);
        __ Move(scratch, value);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &checkNext);
        LoadHClassBitField(assembler_, scratch, value);
        __ And(scratch, static_cast<int64_t>(1U << JSHClass::IsJSSharedBit::START_BIT));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &done);
        __ Bind(&checkNext);
    }
    if (fieldType == static_cast<uint32_t>(SharedFieldType::NONE) ||
        (fieldType & static_cast<uint32_t>(SharedFieldType::GENERIC)) != 0) {
        Label checkShared;
        __ Move(scratch, value);
        __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &done);
        __ Jump(&checkShared);
        __ Bind(&checkShared);
        LoadHClassBitField(assembler_, scratch, value);
        __ And(scratch, static_cast<int64_t>(1U << JSHClass::IsJSSharedBit::START_BIT));
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_EQUAL, &done);
    }

    __ Jump(&typeMismatch);

    __ Bind(&publishTreeString);
    int stackArgCount = PrepareRuntimeStubStackArguments(
        prepareField, 1, RTSTUB_ID(SlowSharedObjectStoreBarrier));
    __ CallRuntime(RTSTUB_ID(SlowSharedObjectStoreBarrier));
    if (HasLazyDeoptSafepoint(prepareField)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, prepareField);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    __ FreeCallArgSlots(stackArgCount);
    __ Jump(&done);

    __ Bind(&typeMismatch);
    constexpr int32_t runtimeArgCount = 1;
    constexpr int32_t runtimeStackArgCount = runtimeArgCount + CALL_ARG2;
    constexpr int32_t reservedSlotCount = (runtimeStackArgCount + 1) & ~1;
    __ ReserveCallArgSlots(reservedSlotCount);
    __ Move(scratch, static_cast<int64_t>(RTSTUB_ID(ThrowTypeError)));
    __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG0), scratch);
    __ Move(scratch, static_cast<int64_t>(runtimeArgCount));
    __ MoveRepr(MachineRepresentation::Word64, __ GetCallArgSlot(CALL_ARG1), scratch);
    __ LoadTaggedValue(
        scratch, JSTaggedValue(GET_MESSAGE_STRING_ID(SetTypeMismatchedSharedProperty)).GetRawData());
    __ MoveRepr(MachineRepresentation::Tagged, __ GetCallArgSlot(CALL_ARG2), scratch);
    __ CallRuntime(RTSTUB_ID(ThrowTypeError));
    if (HasLazyDeoptSafepoint(prepareField)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, prepareField);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    __ FreeCallArgSlots(reservedSlotCount);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreInt32FieldVertex>(StoreInt32FieldVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreInt32FieldVertex";
#endif
    auto storeTarget = GetInputRegister(storeField, StoreInt32FieldVertex::STORE_TARGET_INDEX);
    auto value = GetInputRegister(storeField, StoreInt32FieldVertex::VALUE_INDEX);
    __ StoreInt32Field(value, storeTarget, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreDoubleFieldVertex>(StoreDoubleFieldVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId() << ": StoreDoubleFieldVertex";
#endif
    auto storeTarget = GetInputRegister(storeField, StoreDoubleFieldVertex::STORE_TARGET_INDEX);
    auto value = GetInputDoubleRegister(storeField, StoreDoubleFieldVertex::VALUE_INDEX);
    __ StoreFloat64Field(value, storeTarget, storeField->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreInt32FieldWithRepVertex>(
    StoreInt32FieldWithRepVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId()
                        << ": StoreInt32FieldWithRepVertex";
#endif
    auto storeTarget = GetInputRegister(storeField, StoreInt32FieldWithRepVertex::STORE_TARGET_INDEX);
    auto value = GetInputRegister(storeField, StoreInt32FieldWithRepVertex::VALUE_INDEX);

    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister tag = scope.Acquire();
    ArkSteedRegister expectedTag = scope.Acquire();
    ASSERT(tag != expectedTag);
    __ Move(tag, value);
    __ Move(expectedTag, static_cast<uint64_t>(JSTaggedValue::TAG_MARK));
    __ And(tag, expectedTag);
    __ Move(expectedTag, static_cast<uint64_t>(JSTaggedValue::TAG_INT));
    __ Compare(tag, expectedTag);

    Label deopt;
    Label done;
    __ JumpIf(Condition::NOT_EQUAL, &deopt);
    __ StoreInt32Field(value, storeTarget, storeField->GetOffset());
    __ Jump(&done);

    __ Bind(&deopt);
    EmitEagerDeoptExit(storeField, kungfu::DeoptType::NOTINT1);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreDoubleFieldWithRepVertex>(
    StoreDoubleFieldWithRepVertex *storeField)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeField->GetId()
                        << ": StoreDoubleFieldWithRepVertex";
#endif
    auto storeTarget = GetInputRegister(storeField, StoreDoubleFieldWithRepVertex::STORE_TARGET_INDEX);
    auto value = GetInputRegister(storeField, StoreDoubleFieldWithRepVertex::VALUE_INDEX);

    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister tag = scope.Acquire();
    ArkSteedRegister expectedTag = scope.Acquire();
    ASSERT(tag != expectedTag);
    __ Move(tag, value);
    __ Move(expectedTag, static_cast<uint64_t>(JSTaggedValue::TAG_MARK));
    __ And(tag, expectedTag);

    Label deopt;
    Label intValue;
    Label done;
    __ Move(expectedTag, static_cast<uint64_t>(JSTaggedValue::TAG_INT));
    __ Compare(tag, expectedTag);
    __ JumpIf(Condition::EQUAL, &intValue);
    __ Move(expectedTag, static_cast<uint64_t>(JSTaggedValue::TAG_OBJECT));
    __ Compare(tag, expectedTag);
    __ JumpIf(Condition::EQUAL, &deopt);

    __ Move(tag, value);
    __ Move(expectedTag, static_cast<uint64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ Sub(tag, expectedTag);
    __ StoreField(tag, storeTarget, storeField->GetOffset());
    __ Jump(&done);

    __ Bind(&intValue);
    {
        ArkSteedDoubleRegister doubleValue = scope.AcquireDoubleScratch();
        __ ConvertInt32ToDouble(doubleValue, value);
        __ StoreFloat64Field(doubleValue, storeTarget, storeField->GetOffset());
    }
    __ Jump(&done);

    __ Bind(&deopt);
    EmitEagerDeoptExit(storeField, kungfu::DeoptType::NOTNUMBER1);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<EnsurePropertiesCapacityVertex>(
    EnsurePropertiesCapacityVertex *ensureCapacity)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << ensureCapacity->GetId()
                        << ": EnsurePropertiesCapacityVertex";
#endif
    auto object = GetInputRegister(ensureCapacity, EnsurePropertiesCapacityVertex::OBJECT_INDEX);
    auto result = GetResultRegister(ensureCapacity);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister properties = scope.Acquire();
    ArkSteedRegister length = scope.Acquire();
    ASSERT(properties != object);
    ASSERT(length != object);
    ASSERT(properties != length);
    Label grow;
    Label done;

    __ LoadField(properties, object, JSObject::PROPERTIES_OFFSET);
    __ LoadField(length, properties, TaggedArray::LENGTH_OFFSET);
    __ And(length, static_cast<int64_t>(0xFFFFFFFF));
    __ Compare(length, ensureCapacity->GetFieldIndex());
    __ JumpIf(Condition::LESS_THAN_OR_EQUAL, &grow);
    __ Move(result, properties);
    __ Jump(&done);

    __ Bind(&grow);
    int stackArgCount = PrepareCommonStubStackArguments(ensureCapacity, ensureCapacity->GetArgCount());
    __ CallCommonStub(kungfu::CommonStubCSigns::EnsurePropertiesCapacity);
    if (HasLazyDeoptSafepoint(ensureCapacity)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, ensureCapacity);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    __ FreeCallArgSlots(stackArgCount);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreTaggedFieldByHClassVertex>(
    StoreTaggedFieldByHClassVertex *storeByHClass)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << storeByHClass->GetId()
                        << ": StoreTaggedFieldByHClassVertex";
#endif
    auto object = GetInputRegister(storeByHClass, StoreTaggedFieldByHClassVertex::OBJECT_INDEX);
    auto value = GetInputRegister(storeByHClass, StoreTaggedFieldByHClassVertex::VALUE_INDEX);
    auto glue = GetInputRegister(storeByHClass, StoreTaggedFieldByHClassVertex::GLUE_INDEX);
    ASSERT(!storeByHClass->GetCases().empty());
    std::vector<Label> caseLabels(storeByHClass->GetCases().size());
    Label deopt;
    Label done;
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister actualHClass = scope.Acquire();
    ArkSteedRegister expectedHClass = scope.Acquire();
    ArkSteedRegister valueRegionScratch = scope.Acquire();
    ASSERT(actualHClass != expectedHClass);
    ASSERT(actualHClass != valueRegionScratch);
    ASSERT(expectedHClass != valueRegionScratch);

    __ Move(actualHClass, object);
    __ Move(expectedHClass, static_cast<uint64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ And(actualHClass, expectedHClass);
    __ Compare(actualHClass, 0);
    __ JumpIf(Condition::NOT_EQUAL, &deopt);

    __ LoadField(actualHClass, object, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
    __ Move(expectedHClass, TaggedStateWord::ADDRESS_MASK);
    __ And(actualHClass, expectedHClass);
    for (size_t i = 0; i < storeByHClass->GetCases().size(); ++i) {
        __ MoveEmbeddedTagged(expectedHClass, storeByHClass->GetCases()[i].expectedHClassHandleIndex);
        __ Compare(actualHClass, expectedHClass);
        __ JumpIf(Condition::EQUAL, &caseLabels[i]);
    }
    __ Jump(&deopt);

    __ Bind(&deopt);
    EmitEagerDeoptExit(storeByHClass, kungfu::DeoptType::KEYMISSMATCH);

    for (size_t i = 0; i < storeByHClass->GetCases().size(); ++i) {
        const auto &storeCase = storeByHClass->GetCases()[i];
        __ Bind(&caseLabels[i]);
        ArkSteedRegister storeTarget = object;
        if (storeCase.propertiesArray) {
            __ LoadField(actualHClass, object, JSObject::PROPERTIES_OFFSET);
            storeTarget = actualHClass;
        }
        ArkSteedWriteBarrierEmitter(assembler_, graph_->GetChunk(), &deferredCode_,
                                    storeByHClass->GetRegallocInfo()->GetDeferredRegisterSnapshot())
            .StoreTaggedField(glue, storeTarget, value, storeCase.fieldOffset,
                              ArkSteedWriteBarrierKind::GENERIC_BARRIER, expectedHClass,
                              valueRegionScratch, storeByHClass->GetValueKind());
        __ Jump(&done);
    }
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StoreEnvSlotVertex>(StoreEnvSlotVertex *storeEnvSlot)
{
    auto env = GetInputRegister(storeEnvSlot, StoreEnvSlotVertex::ENV_INDEX);
    auto value = GetInputRegister(storeEnvSlot, StoreEnvSlotVertex::VALUE_INDEX);
    __ StoreField(value, env, storeEnvSlot->GetOffset());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<SetValueWithBarrierVertex>(
    SetValueWithBarrierVertex *setValueWithBarrier)
{
    auto value = GetInputRegister(setValueWithBarrier, SetValueWithBarrierVertex::VALUE_INDEX);
    Label done;
    {
        TemporaryRegisterScope scope(assembler_);
        ArkSteedRegister scratch = scope.AcquireScratch();
        __ Move(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ And(scratch, value);
        __ Compare(scratch, 0);
        __ JumpIf(Condition::NOT_ZERO, &done);
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
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratchGPR = scope.AcquireScratch();
    ArkSteedDoubleRegister scratchFPR = scope.AcquireDoubleScratch();
    ExecuteGapMove(target, source, &scratchGPR, &scratchFPR);
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
            ExecuteConstantMove(target, v);                                                                      \
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
    auto dst = GetResultRegister(toTaggedInt);
    auto src = GetInputRegister(toTaggedInt, I32ToTaggedIntVertex::INPUT_INDEX);
    __ SignExtendInt32ToInt64(dst, src);
    __ Or(dst, static_cast<int64_t>(JSTaggedValue::TAG_INT));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<RawI64ToTaggedVertex>(RawI64ToTaggedVertex *toTagged)
{
    auto dst = GetResultRegister(toTagged);
    auto src = GetInputRegister(toTagged, RawI64ToTaggedVertex::INPUT_INDEX);
    if (dst != src) {
        __ Move(dst, src);
    }
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedToRawI64Vertex>(TaggedToRawI64Vertex *convert)
{
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, TaggedToRawI64Vertex::INPUT_INDEX);
    if (dst != src) {
        __ Move(dst, src);
    }
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I64BitwiseBinaryVertex>(I64BitwiseBinaryVertex *op)
{
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
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, TaggedIntToI32Vertex::INPUT_INDEX);
    __ SignExtendInt32ToInt64(dst, src);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedTaggedIntToI32Vertex>(CheckedTaggedIntToI32Vertex *convert)
{
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, CheckedTaggedIntToI32Vertex::INPUT_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    __ Move(scratch, src);
    __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_MARK));
    __ Compare(scratch, static_cast<int64_t>(JSTaggedValue::TAG_MARK));
    BranchToEagerDeoptTarget(Condition::NOT_EQUAL, convert, kungfu::DeoptType::NOTINT1);
    __ SignExtendInt32ToInt64(dst, src);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedTaggedStringVertex>(CheckedTaggedStringVertex *check)
{
    auto value = GetInputRegister(check, CheckedTaggedStringVertex::INPUT_INDEX);
    ASSERT(GetResultRegister(check) == value);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
#if defined(PANDA_TARGET_AMD64)
    Label *deopt = RecordEagerDeoptTarget(check, kungfu::DeoptType::NOTSTRING1);
    __ JumpIfNotTaggedHeapObject(value, deopt);
    __ LoadField(scratch, value, TaggedObject::HCLASS_OFFSET);
    __ And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    __ LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    __ And(scratch, static_cast<int32_t>((1U << JSHClass::TYPE_BITFIELD_NUM) - 1));
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_FIRST));
    __ JumpIf(Condition::LESS_THAN, deopt);
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_LAST));
    __ JumpIf(Condition::GREATER_THAN, deopt);
#else
    Label deopt;
    Label done;

    __ JumpIfNotTaggedHeapObject(value, &deopt);
    __ LoadField(scratch, value, TaggedObject::HCLASS_OFFSET);
    __ And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    __ LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    __ And(scratch, static_cast<int32_t>((1U << JSHClass::TYPE_BITFIELD_NUM) - 1));
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_FIRST));
    __ JumpIf(Condition::LESS_THAN, &deopt);
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_LAST));
    __ JumpIf(Condition::GREATER_THAN, &deopt);
    __ Jump(&done);

    __ Bind(&deopt);
    EmitEagerDeoptExit(check, kungfu::DeoptType::NOTSTRING1);
    __ Bind(&done);
#endif
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTaggedStringVertex>(BranchIfTaggedStringVertex *jumpIf)
{
    auto value = GetInputRegister(jumpIf, BranchIfTaggedStringVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();

    __ JumpIfNotTaggedHeapObject(value, ifFalse->GetLabel());
    __ LoadField(scratch, value, TaggedObject::HCLASS_OFFSET);
    __ And(scratch, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    __ LoadField(scratch, scratch, JSHClass::BIT_FIELD_OFFSET);
    __ And(scratch, static_cast<int32_t>((1U << JSHClass::TYPE_BITFIELD_NUM) - 1));
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_FIRST));
    __ JumpIf(Condition::LESS_THAN, ifFalse->GetLabel());
    __ Compare(scratch, static_cast<int32_t>(JSType::STRING_LAST));
    __ JumpIf(Condition::GREATER_THAN, ifFalse->GetLabel());
    __ Jump(ifTrue->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfHClassInVertex>(BranchIfHClassInVertex *jumpIf)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << jumpIf->GetId()
                        << ": BranchIfHClassInVertex to BB #" << jumpIf->IfTrue()->GetId()
                        << " if true; to BB #" << jumpIf->IfFalse()->GetId() << " if false.";
#endif
    auto value = GetInputRegister(jumpIf, BranchIfHClassInVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister actualHClass = jumpIf->InputIsHClassAddress() ? value : scope.Acquire();
    ArkSteedRegister expectedHClass = scope.Acquire();
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();

    if (!jumpIf->InputIsHClassAddress()) {
        __ Move(actualHClass, value);
        __ Move(expectedHClass, static_cast<uint64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
        __ And(actualHClass, expectedHClass);
        __ Compare(actualHClass, 0);
        __ JumpIf(Condition::NOT_EQUAL, ifFalse->GetLabel());
        __ LoadField(actualHClass, value, TaggedObject::HCLASS_OFFSET);
        __ Move(expectedHClass, TaggedStateWord::ADDRESS_MASK);
        __ And(actualHClass, expectedHClass);
    }
    for (uint32_t i = 0; i < jumpIf->GetExpectedHClassCount(); ++i) {
        __ MoveEmbeddedTagged(expectedHClass, jumpIf->GetExpectedHClassHandleIndex(i));
        __ Compare(actualHClass, expectedHClass);
        __ JumpIf(Condition::EQUAL, ifTrue->GetLabel());
    }
    __ Jump(ifFalse->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32ConditionCheckVertex>(I32ConditionCheckVertex *check)
{
    auto dst = GetResultRegister(check);
    auto left = GetInputRegister(check, I32ConditionCheckVertex::LEFT_INDEX);
    auto right = GetInputRegister(check, I32ConditionCheckVertex::RIGHT_INDEX);
    __ CompareInt32(left, right);
    EmitTaggedBooleanFromCondition(assembler_, dst, check->GetCondition());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ConditionCheckVertex>(F64ConditionCheckVertex *check)
{
    auto dst = GetResultRegister(check);
    auto left = GetInputDoubleRegister(check, F64ConditionCheckVertex::LEFT_INDEX);
    auto right = GetInputDoubleRegister(check, F64ConditionCheckVertex::RIGHT_INDEX);
    __ CompareFloat64(left, right);
    EmitTaggedBooleanFromFloat64Compare(assembler_, dst, check->GetCondition());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedEqualVertex>(TaggedEqualVertex *op)
{
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, TaggedEqualVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, TaggedEqualVertex::RIGHT_INDEX);
    __ Compare(left, right);
    EmitTaggedBooleanFromCondition(assembler_, dst, Condition::EQUAL);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TaggedNotEqualVertex>(TaggedNotEqualVertex *op)
{
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, TaggedNotEqualVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, TaggedNotEqualVertex::RIGHT_INDEX);
    __ Compare(left, right);
    EmitTaggedBooleanFromCondition(assembler_, dst, Condition::NOT_EQUAL);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<StringEqualVertex>(StringEqualVertex *op)
{
    auto dst = GetResultRegister(op);
    int stackArgCount = PrepareCommonStubStackArguments(op, op->GetInputCount());
    __ CallCommonStub(kungfu::CommonStubCSigns::FastStringEqual);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    __ FreeCallArgSlots(stackArgCount);
    __ And(dst, 1);

    Label falseLabel;
    Label done;
    __ Compare(dst, 0);
    __ JumpIf(Condition::EQUAL, &falseLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::True().GetRawData());
    __ Jump(&done);
    __ Bind(&falseLabel);
    __ LoadTaggedValue(dst, JSTaggedValue::False().GetRawData());
    __ Bind(&done);
}

#define DEFINE_I32_WITH_OVERFLOW_CODEGEN(Name, Op)                                                      \
    template <>                                                                                         \
    void ArkSteedCodeGenerator::VisitNonControlVertex<I32##Name##WithOverflowVertex>(                   \
        I32##Name##WithOverflowVertex *op)                                                              \
    {                                                                                                   \
        auto dst = GetResultRegister(op);                                                               \
        auto left = GetInputRegister(op, I32##Name##WithOverflowVertex::LEFT_INDEX);                   \
        auto right = GetInputRegister(op, I32##Name##WithOverflowVertex::RIGHT_INDEX);                  \
        __ Op(dst, left, right);                                                                        \
        ASSERT(!EagerDeoptUsesRegister(op, dst));                                                       \
        BranchToEagerDeoptTarget(Condition::OVERFLOW, op, kungfu::DeoptType::INT32OVERFLOW1);      \
    }

DEFINE_I32_WITH_OVERFLOW_CODEGEN(Add, Int32Add)
DEFINE_I32_WITH_OVERFLOW_CODEGEN(Sub, Int32Sub)
#undef DEFINE_I32_WITH_OVERFLOW_CODEGEN

#define DEFINE_I32_CODEGEN(Name, Op)                                                          \
    template <>                                                                               \
    void ArkSteedCodeGenerator::VisitNonControlVertex<I32##Name##Vertex>(I32##Name##Vertex *op) \
    {                                                                                         \
        auto dst = GetResultRegister(op);                                                     \
        auto left = GetInputRegister(op, I32##Name##Vertex::LEFT_INDEX);                      \
        auto right = GetInputRegister(op, I32##Name##Vertex::RIGHT_INDEX);                    \
        __ Op(dst, left, right);                                                              \
    }

DEFINE_I32_CODEGEN(Add, Int32Add)
DEFINE_I32_CODEGEN(Sub, Int32Sub)
#undef DEFINE_I32_CODEGEN

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32MulVertex>(I32MulVertex *op)
{
    auto dst = GetResultRegister(op);
    auto right = GetInputRegister(op, I32MulVertex::RIGHT_INDEX);
    ASSERT(dst == GetInputRegister(op, I32MulVertex::LEFT_INDEX));
    __ Int32Mul(dst, right);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32MulWithOverflowVertex>(I32MulWithOverflowVertex *op)
{
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, I32MulWithOverflowVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, I32MulWithOverflowVertex::RIGHT_INDEX);
#if defined(PANDA_TARGET_AMD64)
    ASSERT(dst == left);
#endif

    Label success;
#if defined(PANDA_TARGET_ARM64)
    Label overflow;
    Label negativeZero;
    Label done;
#endif
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister savedLeft = scope.Acquire();
    __ Move(savedLeft, left);
#if defined(PANDA_TARGET_ARM64)
    {
        TemporaryRegisterScope scratchScope(assembler_);
        ArkSteedRegister product = scratchScope.AcquireScratch();
        ArkSteedRegister truncatedProduct = scratchScope.AcquireScratch();
        __ Int32MulWide(product, left, right);
        __ SignExtendInt32ToInt64(truncatedProduct, product);
        __ Compare(product, truncatedProduct);
        __ JumpIf(Condition::NOT_EQUAL, &overflow);
        __ SignExtendInt32ToInt64(dst, product);
    }
    ASSERT(!EagerDeoptUsesRegister(op, dst));
#else
    __ Int32Mul(dst, right);
    ASSERT(!EagerDeoptUsesRegister(op, dst));
    BranchToEagerDeoptTarget(Condition::OVERFLOW, op, kungfu::DeoptType::INT32OVERFLOW1);
#endif
    __ CompareInt32(dst, 0);
    __ JumpIf(Condition::NOT_EQUAL, &success);
    __ Int32Or(savedLeft, right);
    __ CompareInt32(savedLeft, 0);
#if defined(PANDA_TARGET_ARM64)
    __ JumpIf(Condition::LESS_THAN, &negativeZero);
    __ Bind(&success);
    __ Jump(&done);
    __ Bind(&overflow);
    EmitEagerDeoptExit(op, kungfu::DeoptType::INT32OVERFLOW1);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitEagerDeoptExit(op, kungfu::DeoptType::PRODUCTISNEGATIVEZERO);
    __ Bind(&done);
#else
    BranchToEagerDeoptTarget(Condition::LESS_THAN, op, kungfu::DeoptType::PRODUCTISNEGATIVEZERO);
    __ Bind(&success);
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32DivWithOverflowVertex>(I32DivWithOverflowVertex *op)
{
    auto dst = GetResultRegister(op);
    auto left = GetInputRegister(op, I32DivWithOverflowVertex::LEFT_INDEX);
    auto right = GetInputRegister(op, I32DivWithOverflowVertex::RIGHT_INDEX);
    Label divisorReady;
    Label done;
#if defined(PANDA_TARGET_ARM64)
    Label divideZero;
    Label overflow;
    Label notInt;
    Label negativeZero;
#endif

    __ CompareInt32(right, 0);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::EQUAL, op, kungfu::DeoptType::DIVZERO1);
#else
    __ JumpIf(Condition::EQUAL, &divideZero);
#endif
    __ CompareInt32(left, std::numeric_limits<int32_t>::min());
    __ JumpIf(Condition::NOT_EQUAL, &divisorReady);
    __ CompareInt32(right, -1);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::EQUAL, op, kungfu::DeoptType::INT32OVERFLOW1);
#else
    __ JumpIf(Condition::EQUAL, &overflow);
#endif
    __ Bind(&divisorReady);
#if defined(PANDA_TARGET_AMD64)
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister quotient = scope.AcquireSpecific(x64::rax);
    ArkSteedRegister remainder = scope.AcquireSpecific(x64::rdx);
    ASSERT(dst == quotient);
    __ Int32DivAndRemainder(quotient, remainder, left, right);
#else
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister remainder = scope.AcquireScratch();
    __ Int32DivAndRemainder(dst, remainder, left, right);
#endif
    __ CompareInt32(remainder, 0);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::NOT_EQUAL, op, kungfu::DeoptType::NOTINT5);
#else
    __ JumpIf(Condition::NOT_EQUAL, &notInt);
#endif
    __ CompareInt32(dst, 0);
    __ JumpIf(Condition::NOT_EQUAL, &done);
    __ CompareInt32(right, 0);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::LESS_THAN, op, kungfu::DeoptType::DIVZERO2);
#else
    __ JumpIf(Condition::LESS_THAN, &negativeZero);
    __ Jump(&done);
    __ Bind(&divideZero);
    EmitEagerDeoptExit(op, kungfu::DeoptType::DIVZERO1);
    __ Jump(&done);
    __ Bind(&overflow);
    EmitEagerDeoptExit(op, kungfu::DeoptType::INT32OVERFLOW1);
    __ Jump(&done);
    __ Bind(&notInt);
    EmitEagerDeoptExit(op, kungfu::DeoptType::NOTINT5);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitEagerDeoptExit(op, kungfu::DeoptType::DIVZERO2);
#endif

    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32DivByConstWithCheckVertex>(I32DivByConstWithCheckVertex *op)
{
    auto dst = GetResultRegister(op);
    auto dividend = GetInputRegister(op, I32DivByConstWithCheckVertex::INPUT_INDEX);
#if defined(PANDA_TARGET_AMD64)
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister mulDividend = scope.AcquireSpecific(x64::rax);
    ArkSteedRegister work = scope.AcquireSpecific(x64::rdx);
    ArkSteedRegister original = scope.AcquireSpecific(x64::rcx);
    ArkSteedRegister divisor = scope.AcquireSpecific(x64::r8);
    ASSERT(dst == mulDividend);
#else
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister work = scope.Acquire();
    ArkSteedRegister original = scope.Acquire();
    ArkSteedRegister mulDividend = dividend;
    Label negativeZero;
    Label notInt;
    Label done;
#endif
    __ Move(original, dividend);
#if defined(PANDA_TARGET_AMD64)
    __ Move(mulDividend, dividend);
#endif
    if (op->GetDivisor() < 0) {
        __ CompareInt32(original, 0);
#if defined(PANDA_TARGET_AMD64)
        BranchToEagerDeoptTarget(Condition::EQUAL, op, kungfu::DeoptType::DIVZERO2);
#else
        __ JumpIf(Condition::EQUAL, &negativeZero);
#endif
    }

    __ Move(work, op->GetMagic());
    __ Int32MulHigh(work, mulDividend, work);
    if (op->GetDivisor() > 0 && op->GetMagic() < 0) {
        __ Int32Add(work, work, original);
    } else if (op->GetDivisor() < 0 && op->GetMagic() > 0) {
        __ Int32Sub(work, work, original);
    }
    if (op->GetShift() != 0) {
        __ Int32ShiftRightArithmetic(work, op->GetShift());
    }

    __ Move(dst, work);
    __ Move(work, dst);
    __ Int32ShiftRightLogical(work, 31);
    __ Int32Add(dst, dst, work);

#if defined(PANDA_TARGET_AMD64)
    __ Move(work, dst);
    __ Move(divisor, op->GetDivisor());
    __ Int32Mul(work, divisor);
#else
    __ Move(work, op->GetDivisor());
    __ Int32Mul(work, dst);
#endif
    __ CompareInt32(work, original);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::NOT_EQUAL, op, kungfu::DeoptType::NOTINT5);
#else
    __ JumpIf(Condition::NOT_EQUAL, &notInt);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitEagerDeoptExit(op, kungfu::DeoptType::DIVZERO2);
    __ Jump(&done);
    __ Bind(&notInt);
    EmitEagerDeoptExit(op, kungfu::DeoptType::NOTINT5);
    __ Bind(&done);
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32DivVertex>(I32DivVertex *div)
{
    auto dst = GetResultRegister(div);
    auto left = GetInputRegister(div, I32DivVertex::LEFT_INDEX);
    auto right = GetInputRegister(div, I32DivVertex::RIGHT_INDEX);
#if defined(PANDA_TARGET_AMD64)
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister quotient = scope.AcquireSpecific(x64::rax);
    ArkSteedRegister remainder = scope.AcquireSpecific(x64::rdx);
    ASSERT(dst == quotient);
    __ Int32DivAndRemainder(quotient, remainder, left, right);
#else
    __ Int32Div(dst, left, right);
#endif
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedI32ModVertex>(CheckedI32ModVertex *mod)
{
    auto dst = GetResultRegister(mod);
    auto left = GetInputRegister(mod, CheckedI32ModVertex::LEFT_INDEX);
    auto right = GetInputRegister(mod, CheckedI32ModVertex::RIGHT_INDEX);
#if defined(PANDA_TARGET_AMD64)
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister quotient = scope.AcquireSpecific(x64::rax);
    ArkSteedRegister remainder = scope.AcquireSpecific(x64::rdx);
#else
    Label divideZero;
    Label overflow;
    Label negativeZero;
#endif
    Label divisorReady;
    Label leftNeg;
    Label done;

    // divisor == 0 -> NaN (a double), deopt.
    __ CompareInt32(right, 0);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::EQUAL, mod, kungfu::DeoptType::MODZERO1);
#else
    __ JumpIf(Condition::EQUAL, &divideZero);
#endif
    // INT_MIN % -1 traps idiv (#DE); deopt to the interpreter (mathematically 0).
    __ CompareInt32(left, std::numeric_limits<int32_t>::min());
    __ JumpIf(Condition::NOT_EQUAL, &divisorReady);
    __ CompareInt32(right, -1);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(Condition::EQUAL, mod, kungfu::DeoptType::INT32OVERFLOW1);
#else
    __ JumpIf(Condition::EQUAL, &overflow);
#endif
    __ Bind(&divisorReady);
    // Read the dividend sign before PositiveInt32Mod, which clobbers the left input
    // register. remainder == 0 with a negative dividend is JS -0.0 (not Int32) -> deopt.
    __ CompareInt32(left, 0);
    __ JumpIf(Condition::LESS_THAN, &leftNeg);
#if defined(PANDA_TARGET_AMD64)
    __ Int32DivAndRemainder(quotient, remainder, left, right);
    __ Move(dst, remainder);
#else
    __ PositiveInt32Mod(dst, left, right);
#endif
    __ Jump(&done);

    __ Bind(&leftNeg);
#if defined(PANDA_TARGET_AMD64)
    __ Int32DivAndRemainder(quotient, remainder, left, right);
    __ Move(dst, remainder);
#else
    __ PositiveInt32Mod(dst, left, right);
#endif
    __ CompareInt32(dst, 0);
#if defined(PANDA_TARGET_AMD64)
    BranchToEagerDeoptTarget(
        Condition::EQUAL, mod, kungfu::DeoptType::REMAINDERISNEGATIVEZERO);
#else
    __ JumpIf(Condition::EQUAL, &negativeZero);
    __ Jump(&done);
    __ Bind(&divideZero);
    EmitEagerDeoptExit(mod, kungfu::DeoptType::MODZERO1);
    __ Jump(&done);
    __ Bind(&overflow);
    EmitEagerDeoptExit(mod, kungfu::DeoptType::INT32OVERFLOW1);
    __ Jump(&done);
    __ Bind(&negativeZero);
    EmitEagerDeoptExit(mod, kungfu::DeoptType::REMAINDERISNEGATIVEZERO);
#endif
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedNonNegativeI32ToTaggedIntVertex>(
    CheckedNonNegativeI32ToTaggedIntVertex *convert)
{
    auto dst = GetResultRegister(convert);
    auto src = GetInputRegister(convert, CheckedNonNegativeI32ToTaggedIntVertex::INPUT_INDEX);

    __ CompareInt32(src, 0);
    BranchToEagerDeoptTarget(Condition::LESS_THAN, convert, kungfu::DeoptType::NOTINT5);
    __ SignExtendInt32ToInt64(dst, src);
    __ Or(dst, static_cast<int64_t>(JSTaggedValue::TAG_INT));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32BitwiseBinaryVertex>(I32BitwiseBinaryVertex *op)
{
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
    __ Int32ToFloat64(GetResultDoubleRegister(convert),
                               GetInputRegister(convert, I32ToF64Vertex::INPUT_INDEX));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<CheckedNumberToF64Vertex>(CheckedNumberToF64Vertex *convert)
{
    auto dst = GetResultDoubleRegister(convert);
    auto input = GetInputRegister(convert, CheckedNumberToF64Vertex::INPUT_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister bits = scope.Acquire();
    ArkSteedRegister scratch = scope.Acquire();
    Label intCase;
    Label done;

    __ Move(bits, input);
    __ Move(scratch, JSTaggedValue::TAG_MARK);
    __ Word64And(bits, scratch);
    __ Compare(bits, scratch);
    __ JumpIf(Condition::EQUAL, &intCase);
    __ Compare(bits, static_cast<int64_t>(JSTaggedValue::TAG_OBJECT));
    BranchToEagerDeoptTarget(Condition::EQUAL, convert, kungfu::DeoptType::NOTNUMBER1);

    __ Move(bits, input);
    __ Move(scratch, static_cast<uint64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ Sub(bits, scratch);
    __ Move(dst, bits);
    __ Jump(&done);

    __ Bind(&intCase);
    __ SignExtendInt32ToInt64(bits, input);
    __ Int32ToFloat64(dst, bits);
    __ Jump(&done);

    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ToI32TruncVertex>(F64ToI32TruncVertex *op)
{
    __ TruncateFloat64ToInt32(GetResultRegister(op),
                                       GetInputDoubleRegister(op, F64ToI32TruncVertex::INPUT_INDEX));
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32ToUint8ClampedVertex>(I32ToUint8ClampedVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": I32ToUint8ClampedVertex";
#endif
    auto value = GetInputRegister(op, I32ToUint8ClampedVertex::INPUT_INDEX);
    auto result = GetResultRegister(op);
    ASSERT(value == result);
    Label min;
    Label done;
    __ CompareInt32(result, 0);
    __ JumpIf(Condition::LESS_THAN_OR_EQUAL, &min);
    __ CompareInt32(result, static_cast<int32_t>(std::numeric_limits<uint8_t>::max()));
    __ JumpIf(Condition::LESS_THAN_OR_EQUAL, &done);
    __ Move(result, static_cast<int32_t>(std::numeric_limits<uint8_t>::max()));
    __ Jump(&done);
    __ Bind(&min);
    __ Move(result, 0);
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ToUint8ClampedVertex>(F64ToUint8ClampedVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": F64ToUint8ClampedVertex";
#endif
    auto value = GetInputDoubleRegister(op, F64ToUint8ClampedVertex::INPUT_INDEX);
    auto result = GetResultRegister(op);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    ArkSteedRegister halfBits = scope.Acquire();
    ArkSteedDoubleRegister temporary = scope.AcquireDouble();

    Label positive;
    Label inRange;
    Label tie;
    Label roundUp;
    Label min;
    Label max;
    Label done;

    __ Move(temporary, 0.0, scratch);
    __ CompareFloat64(value, temporary);
    BranchOnFloat64Compare(assembler_, Condition::GREATER_THAN, &positive, &min);

    __ Bind(&positive);
    __ Move(temporary, static_cast<double>(std::numeric_limits<uint8_t>::max()), scratch);
    __ CompareFloat64(value, temporary);
    BranchOnFloat64Compare(assembler_, Condition::LESS_THAN, &inRange, &max);

    __ Bind(&inRange);
    __ TruncateFloat64ToInt32(result, value);
    __ Int32ToFloat64(temporary, result);
    // For values in (0, 255), floor(value) - value is in (-1, 0]. Its IEEE bits
    // order around -0.5 lets us distinguish below-half, tie and above-half.
    __ Float64Sub(temporary, value);
    __ Move(scratch, temporary);
    __ Move(halfBits, base::bit_cast<int64_t>(-0.5));
    __ Compare(scratch, halfBits);
    __ JumpIf(Condition::BELOW, &done);
    __ JumpIf(Condition::EQUAL, &tie);
    __ Jump(&roundUp);

    __ Bind(&tie);
    __ Move(scratch, result);
    __ Int32And(scratch, 1);
    __ CompareInt32(scratch, 0);
    __ JumpIf(Condition::EQUAL, &done);

    __ Bind(&roundUp);
    __ Move(scratch, 1);
    __ Int32Add(result, result, scratch);
    __ Jump(&done);

    __ Bind(&min);
    __ Move(result, 0);
    __ Jump(&done);

    __ Bind(&max);
    __ Move(result, static_cast<int32_t>(std::numeric_limits<uint8_t>::max()));
    __ Bind(&done);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<DoubleToInt32CallVertex>(DoubleToInt32CallVertex *op)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CodeGen: Visiting v" << op->GetId() << ": DoubleToInt32CallVertex";
#endif
    ASSERT(GetInputDoubleRegister(op, DoubleToInt32CallVertex::INPUT_INDEX).Code() == 0);
    __ Move(ArkSteedAssembler::GetParameterRegister(0), static_cast<int64_t>(base::INT32_BITS));
    __ CallNGCRuntime(RTSTUB_ID(DoubleToInt));
}

#if defined(PANDA_TARGET_AMD64)
#define DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Name, AsmOp, DeoptType, NeedZeroCheck)                \
    template <>                                                                                       \
    void ArkSteedCodeGenerator::VisitNonControlVertex<I32##Name##WithOverflowVertex>(                 \
        I32##Name##WithOverflowVertex *op)                                                            \
    {                                                                                                 \
        auto dst = GetResultRegister(op);                                                             \
        auto input = GetInputRegister(op, I32##Name##WithOverflowVertex::VALUE_INDEX);                \
        if constexpr (NeedZeroCheck) {                                                                \
            __ CompareInt32(input, 0);                                                                \
            BranchToEagerDeoptTarget(Condition::EQUAL, op, DeoptType);                           \
        }                                                                                             \
        __ AsmOp(dst, input);                                                                         \
        ASSERT(!EagerDeoptUsesRegister(op, dst));                                                     \
        BranchToEagerDeoptTarget(Condition::OVERFLOW, op, DeoptType);                            \
    }
#else
#define DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Name, AsmOp, DeoptType, NeedZeroCheck)                 \
    template <>                                                                                       \
    void ArkSteedCodeGenerator::VisitNonControlVertex<I32##Name##WithOverflowVertex>(                 \
        I32##Name##WithOverflowVertex *op)                                                            \
    {                                                                                                 \
        auto dst = GetResultRegister(op);                                                             \
        auto input = GetInputRegister(op, I32##Name##WithOverflowVertex::VALUE_INDEX);                \
        Label deopt;                                                                                  \
        Label done;                                                                                   \
        if constexpr (NeedZeroCheck) {                                                                \
            __ CompareInt32(input, 0);                                                                \
            __ JumpIf(Condition::EQUAL, &deopt);                                                 \
        }                                                                                             \
        __ AsmOp(dst, input);                                                                         \
        ASSERT(!EagerDeoptUsesRegister(op, dst));                                                     \
        __ JumpIf(Condition::OVERFLOW, &deopt);                                                  \
        __ Jump(&done);                                                                               \
        __ Bind(&deopt);                                                                              \
        EmitEagerDeoptExit(op, DeoptType);                                                            \
        __ Bind(&done);                                                                               \
    }
#endif

DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Neg, Int32Neg, kungfu::DeoptType::NOTNEGOV1, true)
DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Inc, Int32Inc, kungfu::DeoptType::INT32OVERFLOW1, false)
DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN(Dec, Int32Dec, kungfu::DeoptType::INT32OVERFLOW1, false)
#undef DEFINE_I32_UNARY_WITH_OVERFLOW_CODEGEN

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<I32BNotVertex>(I32BNotVertex *op)
{
    auto dst = GetResultRegister(op);
    ASSERT(dst == GetInputRegister(op, I32BNotVertex::VALUE_INDEX));
    __ Int32BNot(dst);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64NegVertex>(F64NegVertex *op)
{
    auto dst = GetResultDoubleRegister(op);
    auto value = GetInputDoubleRegister(op, F64NegVertex::VALUE_INDEX);
    __ Float64Neg(dst, value);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<F64ToTaggedDoubleVertex>(F64ToTaggedDoubleVertex *convert)
{
    auto dst = GetResultRegister(convert);
    auto input = GetInputDoubleRegister(convert, F64ToTaggedDoubleVertex::INPUT_INDEX);
    Label pureDouble;
    Label done;

    __ Move(dst, input);
    __ Compare(dst, static_cast<int64_t>(JSTaggedValue::TAG_INT - JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ JumpIf(Condition::BELOW, &pureDouble);
    __ LoadTaggedValue(dst, JSTaggedValue(base::NAN_VALUE).GetRawData());
    __ Jump(&done);
    __ Bind(&pureDouble);
    __ Add(dst, static_cast<int64_t>(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    __ Bind(&done);
}

#define DEFINE_F64_BINOP_CODEGEN(Name, Op)                                                        \
    template <>                                                                                    \
    void ArkSteedCodeGenerator::VisitNonControlVertex<F64##Name##Vertex>(F64##Name##Vertex *op)   \
    {                                                                                              \
        auto dst = GetResultDoubleRegister(op);                                                    \
        auto right = GetInputDoubleRegister(op, F64##Name##Vertex::RIGHT_INDEX);                   \
        ASSERT(dst == GetInputDoubleRegister(op, F64##Name##Vertex::LEFT_INDEX));                  \
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
    int stackArgCount = PrepareCommonStubStackArguments(callCommonStub, callCommonStub->GetArgCount());
    __ CallCommonStub(callCommonStub->GetCommonStubID());
    if (HasLazyDeoptSafepoint(callCommonStub)) {
        EmitLazyDeoptSafepoint(assembler_, safepointBuilder_, deoptLiteralTableBuilder_, callCommonStub);
    } else {
        safepointBuilder_->DefineSafepoint(__ GetPcOffset());
    }
    __ FreeCallArgSlots(stackArgCount);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<LineStringLoadElementVertex>(LineStringLoadElementVertex *load)
{
    auto dst = GetResultRegister(load);
    auto string = GetInputRegister(load, LineStringLoadElementVertex::STRING_INDEX);
    auto index = GetInputRegister(load, LineStringLoadElementVertex::ELEMENT_INDEX);
    auto lengthAndFlags = GetInputRegister(load, LineStringLoadElementVertex::LENGTH_AND_FLAGS_INDEX);
    __ LoadLineStringCharCode(dst, string, index, lengthAndFlags);
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TypedArrayIntLoadElementVertex>(TypedArrayIntLoadElementVertex *load)
{
    auto dst = GetResultRegister(load);
    auto receiver = GetInputRegister(load, TypedArrayIntLoadElementVertex::RECEIVER_INDEX);
    auto index = GetInputRegister(load, TypedArrayIntLoadElementVertex::ELEMENT_INDEX);
    auto storage = GetInputRegister(load, TypedArrayIntLoadElementVertex::STORAGE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    auto data = scope.Acquire();
    auto scratch = scope.Acquire();
    __ LoadTypedArrayDataPointer(data, receiver, storage, scratch, load->IsOnHeap());
    __ LoadTypedArrayIntElement(dst, data, index, load->GetElementType());
}

template <>
void ArkSteedCodeGenerator::VisitNonControlVertex<TypedArrayDoubleLoadElementVertex>(
    TypedArrayDoubleLoadElementVertex *load)
{
    auto dst = GetResultDoubleRegister(load);
    auto receiver = GetInputRegister(load, TypedArrayDoubleLoadElementVertex::RECEIVER_INDEX);
    auto index = GetInputRegister(load, TypedArrayDoubleLoadElementVertex::ELEMENT_INDEX);
    auto storage = GetInputRegister(load, TypedArrayDoubleLoadElementVertex::STORAGE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    auto data = scope.Acquire();
    auto scratch = scope.Acquire();
    __ LoadTypedArrayDataPointer(data, receiver, storage, scratch, load->IsOnHeap());
    __ LoadTypedArrayDoubleElement(dst, data, index, scratch, load->GetElementType());
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
    bool isFallthrough = IsNextBlockInLayout(jump->Target());
    if (!isFallthrough) {
        __ Jump(jump->Target()->GetLabel());
    }
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<JumpLoopVertex>(JumpLoopVertex *jumpLoop)
{
    __ Jump(jumpLoop->Target()->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTrueVertex>(BranchIfTrueVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

    auto cond = GetInputRegister(jumpIf, 0);
    // to do: optimize
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.AcquireScratch();
    __ LoadTaggedValue(scratch, JSTaggedValue::True().GetRawData());
    __ Compare(cond, scratch);

    __ Branch(Condition::ZERO,
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

    auto left = GetInputRegister(jumpIf, BranchIfInt32CompareVertex::LEFT_INDEX);
    auto right = GetInputRegister(jumpIf, BranchIfInt32CompareVertex::RIGHT_INDEX);
    __ CompareInt32(left, right);
    __ Branch(jumpIf->GetCondition(),
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

    auto left = GetInputRegister(jumpIf, BranchIfInt64CompareVertex::LEFT_INDEX);
    auto right = GetInputRegister(jumpIf, BranchIfInt64CompareVertex::RIGHT_INDEX);
    __ Compare(left, right);
    __ Branch(jumpIf->GetCondition(),
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

    auto left = GetInputDoubleRegister(jumpIf, BranchIfFloat64CompareVertex::LEFT_INDEX);
    auto right = GetInputDoubleRegister(jumpIf, BranchIfFloat64CompareVertex::RIGHT_INDEX);
    __ CompareFloat64(left, right);
    BranchOnFloat64Compare(assembler_, jumpIf->GetCondition(), ifTrue->GetLabel(), ifFalse->GetLabel());
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfTaggedHeapObjectVertex>(BranchIfTaggedHeapObjectVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

    auto value = GetInputRegister(jumpIf, BranchIfTaggedHeapObjectVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratch = scope.Acquire();
    __ Move(scratch, value);
    __ And(scratch, static_cast<int64_t>(JSTaggedValue::TAG_HEAPOBJECT_MASK));
    __ Compare(scratch, 0);
    __ Branch(Condition::EQUAL, ifTrue->GetLabel(), trueBranchIsFallthrough, ifFalse->GetLabel(),
              falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfReferenceEqualVertex>(BranchIfReferenceEqualVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

    auto left = GetInputRegister(jumpIf, BranchIfReferenceEqualVertex::LEFT_INDEX);
    auto right = GetInputRegister(jumpIf, BranchIfReferenceEqualVertex::RIGHT_INDEX);
    __ Compare(left, right);
    __ Branch(Condition::EQUAL,
                       ifTrue->GetLabel(),
                       trueBranchIsFallthrough,
                       ifFalse->GetLabel(),
                       falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<BranchIfObjectTypeVertex>(BranchIfObjectTypeVertex *jumpIf)
{
    BB *ifTrue = jumpIf->IfTrue();
    BB *ifFalse = jumpIf->IfFalse();
    bool trueBranchIsFallthrough = IsNextBlockInLayout(ifTrue);
    bool falseBranchIsFallthrough = IsNextBlockInLayout(ifFalse);

    ArkSteedRegister value = GetInputRegister(jumpIf, BranchIfObjectTypeVertex::VALUE_INDEX);
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister objectType = scope.Acquire();
    __ LoadField(objectType, value, static_cast<int32_t>(TaggedObject::HCLASS_OFFSET));
    __ And(objectType, static_cast<int64_t>(TaggedObject::GC_STATE_MASK));
    __ LoadField(objectType, objectType, static_cast<int32_t>(JSHClass::BIT_FIELD_OFFSET));
    static_assert(JSHClass::ObjectTypeBits::START_BIT == 0);
    __ And(objectType, static_cast<int32_t>((1U << JSHClass::ObjectTypeBits::SIZE) - 1));
    __ Compare(objectType, static_cast<int32_t>(jumpIf->GetExpectedType()));
    __ Branch(Condition::EQUAL, ifTrue->GetLabel(), trueBranchIsFallthrough,
              ifFalse->GetLabel(), falseBranchIsFallthrough);
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<ReturnVertex>(ReturnVertex *returns)
{
    __ Epilogue();
    __ Return();
}

template <>
void ArkSteedCodeGenerator::VisitControlVertex<ThrowVertex>(ThrowVertex *throws)
{
    int stackArgCount =
        PrepareRuntimeStubStackArguments(throws, throws->GetArgCount(), static_cast<int>(throws->GetRuntimeStubID()));
    __ CallRuntime(throws->GetRuntimeStubID());
    __ FreeCallArgSlots(stackArgCount);
    safepointBuilder_->DefineSafepoint(__ GetPcOffset());

    BB *catchBlock = CatchBlockOf(throws);
    if (catchBlock != nullptr) {
        uint32_t catchPredId = CatchPredecessorIndexOf(throws);
        DeconstructPhisInSuccessor(catchBlock, catchPredId);
        __ Jump(catchBlock->GetLabel());
    } else {
        EmitReturnWithPendingException();
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
#if defined(PANDA_TARGET_ARM64)
    __ CheckCodePools(true);
#endif

    for (uint32_t i = 0, numBlocks = graph_->NumBlocks(); i < numBlocks; ++i) {
        BB *curBlock = (*graph_)[i];
        currentLayoutNextBlock_ = (i + 1 < numBlocks) ? (*graph_)[i + 1] : nullptr;

        RecordBlockComment(curBlock);
        __ Bind(curBlock->GetLabel());

        if (curBlock->HasPhi()) {
            for (PhiVertex *phi : curBlock->GetPhis()) {
                ProcessNonControlVertex(phi);
#if defined(PANDA_TARGET_ARM64)
                __ CheckCodePools(true);
#endif
            }
        }
        for (NonControlVertex *vertex : curBlock->GetVertices()) {
            ProcessNonControlVertex(vertex);
#if defined(PANDA_TARGET_ARM64)
            __ CheckCodePools(true);
#endif
        }
        ControlVertex *controlVertex = curBlock->GetControlVertex();
        // Precondition: All critical edges have been split before.
        if (JumpVertex *jump = controlVertex->TryCast<JumpVertex>(); jump != nullptr) {
            DeconstructPhisInSuccessor(jump->Target(), jump->GetPredecessorId());
        } else if (JumpLoopVertex *jumpLoop = controlVertex->TryCast<JumpLoopVertex>(); jumpLoop != nullptr) {
            DeconstructPhisInSuccessor(jumpLoop->Target(), jumpLoop->GetPredecessorId());
        }

        ProcessControlVertex(controlVertex);
#if defined(PANDA_TARGET_ARM64)
        __ CheckCodePools(true);
#endif
    }
    EmitDeferredCode();
    // Eager-deopt exit decoding relies on the fixed-size exit cluster being the
    // final bytes of the function. Flush ARM64 embedded literals before it.
    __ FinalizeEmbeddedRefs();
    EmitQueuedEagerDeoptExits();
#if defined(PANDA_TARGET_ARM64)
    __ FinalizeVeneers();
#endif
}

void ArkSteedCodeGenerator::EmitDeferredCode()
{
    for (size_t i = 0; i < deferredCode_.size(); ++i) {
        ArkSteedDeferredCode *deferred = deferredCode_[i];
        __ RecordComment("Deferred block");
        __ Bind(deferred->GetEntryLabel());
        deferred->Generate(assembler_);
#if defined(PANDA_TARGET_ARM64)
        __ CheckCodePools(true);
#endif
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
    uint32_t pcBefore = __ GetPcOffset();
    TemporaryRegisterScope temporaryScope(assembler_);
    temporaryScope.Include(vertex->GetRegallocInfo()->GetGeneralTemporaries());
    temporaryScope.IncludeDouble(vertex->GetRegallocInfo()->GetDoubleTemporaries());
    temporaryScope.IncludeSpecific(vertex->GetRegallocInfo()->GetRequiredSpecificGPRs());
    temporaryScope.IncludeSpecificDouble(vertex->GetRegallocInfo()->GetRequiredSpecificFPRs());

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

    if (vertex->CanThrow() || vertex->IsCall()) {
        if (HasExceptionLazyDeopt(vertex)) {
            ASSERT(CatchBlockOf(vertex) == nullptr);
            if (__ GetPcOffset() > pcBefore) {
                RecordVertexComment(pcBefore, vertex);
            }
            return;
        }
        if (BB *catchBlock = CatchBlockOf(vertex)) {
            Label noException;
            __ BranchIfNoPendingException(&noException);
            DeconstructPhisInSuccessor(catchBlock, CatchPredecessorIndexOf(vertex));
            __ Jump(catchBlock->GetLabel());
            __ Bind(&noException);
        } else {
            EmitReturnIfPendingException();
        }
    }

    if (__ GetPcOffset() > pcBefore) {
        RecordVertexComment(pcBefore, vertex);
    }
}

void ArkSteedCodeGenerator::ProcessControlVertex(ControlVertex *vertex)
{
    uint32_t pcBefore = __ GetPcOffset();
    TemporaryRegisterScope temporaryScope(assembler_);
    temporaryScope.Include(vertex->GetRegallocInfo()->GetGeneralTemporaries());
    temporaryScope.IncludeDouble(vertex->GetRegallocInfo()->GetDoubleTemporaries());
    temporaryScope.IncludeSpecific(vertex->GetRegallocInfo()->GetRequiredSpecificGPRs());
    temporaryScope.IncludeSpecificDouble(vertex->GetRegallocInfo()->GetRequiredSpecificFPRs());

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

    if (__ GetPcOffset() > pcBefore) {
        RecordVertexComment(pcBefore, vertex);
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
    WITH_ANSI_COLOR_SCOPE(NthBrightColor(gapMovesSs, currentBlockColorIndex_, withColors_)) {
        gapMovesSs << "--   Gap moves:";
    }
    RecordComment(gapMovesSs.str().c_str());
    TemporaryRegisterScope scope(assembler_);
    ArkSteedRegister scratchGPR = scope.AcquireScratch();
    ArkSteedDoubleRegister scratchFPR = scope.AcquireDoubleScratch();

    GapMoveResolver generalResolver(graph_->GetChunk(), scratchGPR, scratchFPR);
    GapMoveResolver doubleResolver(graph_->GetChunk(), scratchGPR, scratchFPR);
    ChunkVector<std::pair<AllocatedState, ValueVertex *>> constantMoves(graph_->GetChunk());
    ArkSteedRegList registersSetByPhis;
    ArkDoubleRegList doubleRegistersSetByPhis;
    CollectPhiMoves(&generalResolver, &doubleResolver, successor, predecessorId,
                    &registersSetByPhis, &doubleRegistersSetByPhis,
                    &constantMoves);
    CollectRegisterStateMoves(&generalResolver, &doubleResolver, successor, predecessorId,
                              registersSetByPhis, doubleRegistersSetByPhis, &constantMoves);

    generalResolver.Resolve(
        [&](AllocatedState dest, AllocatedState src) {
            RecordGapMoveComment(src, dest, nullptr);
            ExecuteGapMove(dest, src, &scratchGPR, &scratchFPR);
        },
        [&](AllocatedState) {
            __ Push(scratchGPR);
        },
        [&](AllocatedState) {
            __ Pop(scratchGPR);
        });
    doubleResolver.Resolve(
        [&](AllocatedState dest, AllocatedState src) {
            RecordGapMoveComment(src, dest, nullptr);
            ExecuteGapMove(dest, src, &scratchGPR, &scratchFPR);
        },
        [&](AllocatedState) {
            __ Push(scratchFPR);
        },
        [&](AllocatedState) {
            __ Pop(scratchFPR);
        });
    for (auto [dest, constVertex] : constantMoves) {
        ExecuteConstantMove(dest, constVertex, &scratchGPR, &scratchFPR);
    }
}

void ArkSteedCodeGenerator::CollectPhiMoves(GapMoveResolver *generalResolver, GapMoveResolver *doubleResolver,
                                            BB *successor, int predecessorId,
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
            AllocatedState allocatedDest = AllocatedState::Cast(dest);
            if (allocatedDest.GetRepresentation() == MachineRepresentation::Float64) {
                doubleResolver->Add(allocatedDest, AllocatedState::Cast(src));
            } else {
                generalResolver->Add(allocatedDest, AllocatedState::Cast(src));
            }
        }

        auto target = AllocatedState::Cast(dest);
        if (target.IsRegister()) {
            registersSetByPhis->Set(target.GetRegister());
        } else if (target.IsDoubleRegister()) {
            doubleRegistersSetByPhis->Set(target.GetDoubleRegister());
        }
    }
}

void ArkSteedCodeGenerator::CollectRegisterStateMoves(GapMoveResolver *generalResolver, GapMoveResolver *doubleResolver,
    BB *successor, int predecessorId,
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
            generalResolver->Add(dest, AllocatedState::Cast(src));
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
            doubleResolver->Add(dest, AllocatedState::Cast(src));
        }
    });
}

void ArkSteedCodeGenerator::ExecuteConstantMove(const AllocatedState &dest, ValueVertex *constVertex,
                                                const ArkSteedRegister *scratchGPR,
                                                const ArkSteedDoubleRegister *scratchFPR)
{
    ASSERT(constVertex != nullptr);

    ArkSteedRegister localGPR = ArkSteedRegister::Invalid();
    ArkSteedDoubleRegister localFPR = ArkSteedDoubleRegister::Invalid();
    TemporaryRegisterScope scope(assembler_);

    auto getScratchGPR = [&]() {
        if (scratchGPR == nullptr) {
            localGPR = scope.AcquireScratch();
            scratchGPR = &localGPR;
        }
        return *scratchGPR;
    };
    auto getScratchFPR = [&]() {
        if (scratchFPR == nullptr) {
            localFPR = scope.AcquireDoubleScratch();
            scratchFPR = &localFPR;
        }
        return *scratchFPR;
    };

    auto loadConstant = [&](ArkSteedRegister reg) {
        if (constVertex->Is<HeapConstantVertex>()) {
            LoadConstantToRegister(constVertex, reg);
            return;
        }
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
        __ Move(dest.GetDoubleRegister(), f64const->GetValue(), getScratchGPR());
        return;
    }
    ASSERT(dest.IsAnyStackSlot());

    if (dest.GetRepresentation() == MachineRepresentation::Float64) {
        Float64ConstantVertex *f64const = constVertex->Cast<Float64ConstantVertex>();
        __ StoreFloat64Constant(__ ToMemOperand(dest), f64const->GetValue(), getScratchGPR(), getScratchFPR());
        return;
    }

    ArkSteedRegister scratch = getScratchGPR();
    loadConstant(scratch);
    __ MoveRepr(dest.GetRepresentation(), __ ToMemOperand(dest), scratch);
}

void ArkSteedCodeGenerator::ExecuteGapMove(const InstructionOperand &dest, const InstructionOperand &src,
                                           const ArkSteedRegister *scratchGPR,
                                           const ArkSteedDoubleRegister *scratchFPR)
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
            __ StoreFloat64(__ ToMemOperand(destOp), srcOp.GetDoubleRegister());
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
                ASSERT(scratchFPR != nullptr);
                __ LoadFloat64(*scratchFPR, srcMem);
                __ StoreFloat64(__ ToMemOperand(destOp), *scratchFPR);
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
    currentBlockColorIndex_ = GetBlockColorIndex(block->GetId());
    std::ostringstream ss;
    WITH_ANSI_COLOR_SCOPE(NthBrightColor(ss, currentBlockColorIndex_, withColors_)) {
        ss << "-- Block b" << block->GetId();
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
                    int colorIndex = GetBlockColorIndex(predecessors[i]->GetId());
                    WITH_ANSI_COLOR_SCOPE(NthBrightColor(ss, colorIndex, withColors_)) {
                        ss << "b" << predecessors[i]->GetId();
                    }
                    ss << ", ";
                }
                ss << "]";
            }
        }
    }
    RecordComment(ss.str().c_str());
}

// The vertex comment is anchored at pcBefore (just before the vertex's first
// instruction) but only recorded after emission, so vertices that produce no
// machine code (e.g. InitialValue, constants, elided fallthrough jumps) are
// filtered out instead of piling up empty comment lines.
void ArkSteedCodeGenerator::RecordVertexComment(uint32_t pcBefore, Vertex *vertex)
{
    if (!__ IsCommentEnabled()) {
        return;
    }
    std::ostringstream ss;
    WITH_ANSI_COLOR_SCOPE(NthBrightColor(ss, currentBlockColorIndex_, withColors_)) {
        ss << "--   ";
    }
    ss << vertex->Dump(withColors_);
    AppendVertexSuccessorInfo(&ss, vertex);
    __ RecordCommentAt(pcBefore, ss.str().c_str());
}

void ArkSteedCodeGenerator::AppendVertexSuccessorInfo(std::ostringstream *ss, Vertex *vertex)
{
    if (!vertex->Is<ControlVertex>()) {
        return;
    }
    ControlVertex *control = vertex->Cast<ControlVertex>();
    auto blockRef = [this, ss](BB *block) {
        int colorIndex = GetBlockColorIndex(block->GetId());
        WITH_ANSI_COLOR_SCOPE(NthBrightColor(*ss, colorIndex, withColors_)) {
            *ss << "b" << block->GetId();
        }
    };
    if (auto *jump = control->TryCast<JumpVertex>(); jump != nullptr) {
        *ss << " --> [";
        blockRef(jump->Target());
        *ss << "]";
    } else if (auto *jumpLoop = control->TryCast<JumpLoopVertex>(); jumpLoop != nullptr) {
        *ss << " --> [";
        blockRef(jumpLoop->Target());
        *ss << "] (loop back)";
    } else if (auto *branch = control->TryCast<BranchControlVertex>(); branch != nullptr) {
        *ss << " --> [";
        blockRef(branch->IfTrue());
        if (IsNextBlockInLayout(branch->IfTrue())) {
            *ss << " (fallthrough)";
        }
        *ss << " if true, ";
        blockRef(branch->IfFalse());
        if (IsNextBlockInLayout(branch->IfFalse())) {
            *ss << " (fallthrough)";
        }
        *ss << " if false]";
    }
}

void ArkSteedCodeGenerator::RecordGapMoveComment(const InstructionOperand &src, const InstructionOperand &dest,
                                                 PhiVertex *phi)
{
    if (!__ IsCommentEnabled()) {
        return;
    }
    std::ostringstream ss;
    WITH_ANSI_COLOR_SCOPE(NthBrightColor(ss, currentBlockColorIndex_, withColors_)) {
        ss << "--   * " << src.Description() << " -> " << dest.Description();
        if (phi != nullptr) {
            std::string label = FormatVertexLabel(phi);
            // Phi vertices should be labelled during graph building
            ASSERT(label != "<unregistered>");
            ss << " (" << label << ")";
        }
    }
    RecordComment(ss.str().c_str());
}

void ArkSteedCodeGenerator::RecordSpillComment()
{
    if (__ IsCommentEnabled()) {
        std::ostringstream ss;
        WITH_ANSI_COLOR_SCOPE(NthBrightColor(ss, currentBlockColorIndex_, withColors_)) {
            ss << "--   Spill:";
        }
        RecordComment(ss.str().c_str());
    }
}

// Get color index for a block - uses graph coloring if computed, otherwise falls back to the raw block id.
int ArkSteedCodeGenerator::GetBlockColorIndex(int blockId) const
{
    if (blockColorsComputed_ && blockId >= 0 && static_cast<size_t>(blockId) < blockColorAssignment_.size()) {
        return blockColorAssignment_[blockId];
    }
    // Fallback to simple modulo when graph coloring is not computed
    return blockId;
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

        bool usedColors[AnsiColorScope::NUM_BRIGHT_COLORS] = {false};
        for (int adjId : adjacentBlocks[blockId]) {
            if (adjId >= 0 && static_cast<size_t>(adjId) < blockColorAssignment_.size() && adjId < blockId &&
                blockColorAssignment_[adjId] >= 0) {
                usedColors[blockColorAssignment_[adjId]] = true;
            }
        }

        size_t color = 0;
        while (color < AnsiColorScope::NUM_BRIGHT_COLORS && usedColors[color]) {
            ++color;
        }

        blockColorAssignment_[blockId] = color % AnsiColorScope::NUM_BRIGHT_COLORS;
    }
}

#undef __
}  // namespace panda::ecmascript::arksteed
