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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_H

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_regalloc_types.h"
#include "ecmascript/arksteed/arksteed_reglist.h"
#include "ecmascript/arksteed/arksteed_vertex.h"

namespace panda::ecmascript::arksteed {

class Graph;
class ArkSteedGraphLabeller;
class BB;
class Vertex;
class ValueVertex;
class PhiVertex;
class ControlVertex;
class VertexInput;
class InputLocation;
class RegallocValueVertexInfo;
class InstructionOperand;

// =============================================================================
// RegisterSnapshot - Manages register state during code generation
// =============================================================================
//
// RegisterSnapshot manages the register frame state during register allocation,
// including:
//   1. Current register values - which registers hold valid values
//   2. State of each register - whether registers are available or blocked
//
// Register state encodes two orthogonal concepts:
//
//   1. Used/free registers: Which registers currently hold a valid value
//   2. Blocked/unblocked registers: Which registers can be modified during the current allocation
//
// The combination of these encodes different states:
//
//  Free + unblocked: Completely unused registers which can be used for anything
//  Used + unblocked: Live values that can be spilled if there is register pressure
//  Used + blocked:   Values that are in a register and are used as an input in the current allocation
//  Free + blocked:   Unused registers that are reserved as temporaries, or inputs that will get clobbered during the
//                    execution of the vertex being allocated
// =============================================================================
template <typename RegisterT>
class RegisterSnapshot {
public:
    static constexpr bool IS_GENERAL_REGISTER = std::is_same_v<ArkSteedRegister, RegisterT>;
    static constexpr bool IS_DOUBLE_REGISTER = std::is_same_v<ArkSteedDoubleRegister, RegisterT>;

    static_assert(IS_GENERAL_REGISTER || IS_DOUBLE_REGISTER,
                  "RegisterSnapshot should be used only for ArkSteedRegister and "
                  "ArkSteedDoubleRegister.");

    using RegTList = RegListBase<RegisterT>;

    RegTList Empty() const
    {
        return RegTList();
    }
    RegTList Free() const
    {
        return free_;
    }
    RegTList UnblockedFree() const
    {
        return free_ - blocked_;
    }
    RegTList Used() const
    {
        return GetAllocatableList<RegisterT>() ^ free_;
    }

    bool IsEmpty() const
    {
        return Used().IsEmpty();
    }

    RegisterT GetFirstUsed() const
    {
        return Used().First();
    }

    bool UnblockedFreeIsEmpty() const
    {
        return UnblockedFree().IsEmpty();
    }

    template <typename Function>
    void ForEachUsedRegister(Function &&f) const
    {
        for (RegisterT reg : Used()) {
            f(reg, GetValue(reg));
        }
    }

    void RemoveFromFree(RegisterT reg)
    {
        ASSERT(free_.Has(reg));
        free_.Clear(reg);
    }
    void AddToFree(RegisterT reg)
    {
        free_.Set(reg);
    }
    void AddToFree(RegTList list)
    {
        free_ |= list;
    }

    void FreeRegistersUsedBy(ValueVertex *vertex)
    {
        auto *vertexInfo = vertex->GetRegallocInfo();
        RegTList list = vertexInfo->ClearRegisters<RegisterT>();
        for (RegisterT reg : list) {
            values_[reg.Code()] = nullptr;
        }
        AddToFree(list);
    }

    void SetValue(RegisterT reg, ValueVertex *vertex)
    {
        ASSERT(!free_.Has(reg));
        ASSERT(!blocked_.Has(reg));
        values_[reg.Code()] = vertex;
        Block(reg);
        auto *vertexInfo = vertex->GetRegallocInfo();
        vertexInfo->AddRegister(reg);
    }

    void SetValueWithoutBlocking(RegisterT reg, ValueVertex *vertex)
    {
        ASSERT(!free_.Has(reg));
        ASSERT(!blocked_.Has(reg));
        values_[reg.Code()] = vertex;
        auto *vertexInfo = vertex->GetRegallocInfo();
        vertexInfo->AddRegister(reg);
    }

    ValueVertex *GetValue(RegisterT reg) const
    {
        return values_[reg.Code()];
    }

    RegTList Blocked() const
    {
        return blocked_;
    }
    void Block(RegisterT reg)
    {
        blocked_.Set(reg);
    }
    void Unblock(RegisterT reg)
    {
        blocked_.Clear(reg);
    }
    bool IsBlocked(RegisterT reg) const
    {
        return blocked_.Has(reg);
    }
    void ClearBlocked()
    {
        blocked_ = RegTList();
    }

    bool IsFree(RegisterT reg) const
    {
        return free_.Has(reg) && !blocked_.Has(reg);
    }

    ValueVertex *GetValueMaybeFree(RegisterT reg) const
    {
        return values_[reg.Code()];
    }

    void DropValueAt(RegisterT reg)
    {
        values_[reg.Code()] = nullptr;
    }

    AllocatedState OperandForVertexRegister(ValueVertex *vertex, RegisterT reg)
    {
        return AllocatedState(AllocatedState::LocationKind::REGISTER, vertex->GetMachineRepresentation(), reg.Code());
    }

    // Allocate a register for the given vertex
    AllocatedState AllocateRegister(ValueVertex *vertex, const InstructionOperand &hint)
    {
        // to do: use hint
        ASSERT(!UnblockedFreeIsEmpty());
        RegisterT reg = UnblockedFree().First();
        RemoveFromFree(reg);
        SetValue(reg, vertex);
        return OperandForVertexRegister(vertex, reg);
    }

    // to do: hint not use now
    InstructionOperand TryChooseInputRegister(ValueVertex *vertex, const InstructionOperand &hint)
    {
        auto *vertexInfo = vertex->GetRegallocInfo();
        RegTList resultRegisters;
        if constexpr (IS_GENERAL_REGISTER) {
            resultRegisters = vertexInfo->GetRegisterResult();
        } else {
            resultRegisters = vertexInfo->GetDoubleRegisterResult();
        }
        if (resultRegisters.IsEmpty()) {
            return InstructionOperand();  // Returns INVALID by default
        }
        // Prefer to return an existing blocked register
        RegTList blockedResult = resultRegisters & blocked_;
        if (!blockedResult.IsEmpty()) {
            RegisterT reg = blockedResult.First();
            return OperandForVertexRegister(vertex, reg);
        }
        // Otherwise use the first result register
        RegisterT reg = resultRegisters.First();
        Block(reg);
        return OperandForVertexRegister(vertex, reg);
    }

    // to do: Not use now
    // Try to use an unblocked register that already has the value

private:
    ValueVertex *values_[RegisterT::NUM_REGISTERS] = {nullptr};
    RegTList free_ = GetAllocatableList<RegisterT>();
    RegTList blocked_ = RegTList();
};

// =============================================================================
// ArkSteedRegisterAllocator - Straight-forward register allocator
// =============================================================================

class ArkSteedRegisterAllocator {
public:
    explicit ArkSteedRegisterAllocator(Graph *graph);
    ~ArkSteedRegisterAllocator();

    void AllocateRegisters();

    // Pick a register to free based on heuristics
    template <typename RegisterT>
    RegisterT PickRegisterToFree(RegListBase<RegisterT> reserved);

private:
    RegisterSnapshot<ArkSteedRegister> generalRegisters_;
    RegisterSnapshot<ArkSteedDoubleRegister> doubleRegisters_;

    // Template helper to get register frame state
    template <typename RegisterT>
    RegisterSnapshot<RegisterT> &GetRegisterSnapshot()
    {
        if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
            return generalRegisters_;
        } else {
            return doubleRegisters_;
        }
    }

    void AddMoveBeforeCurrentVertex(ValueVertex *vertex, const InstructionOperand &source,
                                    const AllocatedState &target);
    void ApplyPatches(BB *block);

    struct GapMovePatch {
        VertexId beforeVertexId;    // Insert the gap move before this vertex's id
        NonControlVertex *gapMove;  // The gap move to insert
        GapMovePatch(VertexId id, NonControlVertex *move) : beforeVertexId(id), gapMove(move) {}
    };

    struct SpillInfo {
        uint32_t slotIndex;
        VertexId freedAtPosition;
        bool doubleSlot;
        SpillInfo(uint32_t slotIndex, VertexId pos, bool isDouble)
            : slotIndex(slotIndex), freedAtPosition(pos), doubleSlot(isDouble)
        {}
    };

    struct SpillLocations {
        int top = 0;
        std::vector<SpillInfo> freeSlots;
    };

    SpillLocations untagged_;
    SpillLocations tagged_;

    void SetupConstantLocations();
    void InitializeBlockState(BB *block);
    void ProcessBlockVertices(BB *block);
    void AllocateBlock(BB *block);

    void AllocateVertex(Vertex *vertex);
    void AllocateControlVertex(ControlVertex *vertex, BB *block);
    void ProcessUnconditionalControl(UnconditionalControlVertex *unconditional, BB *block);
    void ProcessConditionalOrReturn(ControlVertex *vertex, BB *block);
    void AllocatePhis(BB *block);
    void TryAllocatePhisToInput(ChunkVector<PhiVertex *> &phis);
    void TryAllocatePhisToRegister(ChunkVector<PhiVertex *> &phis);
    void SpillRemainingPhis(ChunkVector<PhiVertex *> &phis);
    void LogPhiAllocationResult(ChunkVector<PhiVertex *> &phis);

    void MarkAsClobbered(ValueVertex *vertex, const AllocatedState &location);

    void AssignFixedInput(const Input &input);
    void AssignArbitraryRegisterInput(Vertex *resultVertex, const Input &input);
    void AssignAnyInput(const Input &input);
    void AssignInputs(Vertex *vertex);
    void VerifyInputs(Vertex *vertex);

    void AssignFixedTemporaries(Vertex *vertex);
    template <typename RegisterT>
    void AssignFixedTemporaries(RegisterSnapshot<RegisterT> &registers, Vertex *vertex);

    void AssignArbitraryTemporaries(Vertex *vertex);
    template <typename RegisterT>
    void AssignArbitraryTemporaries(RegisterSnapshot<RegisterT> &registers, Vertex *vertex);

    void Spill(ValueVertex *vertex);
    void SpillRegisters();
    void SpillAndClearRegisters();

    // SpillAndClearRegisters as inline template, calls ClearRegisters with spill=true
    template <typename RegisterT>
    void SpillAndClearRegisters(RegisterSnapshot<RegisterT> &registers)
    {
        ClearRegisters<RegisterT, true>(registers);
    }

    void ClearRegisters();

    // ClearRegisters overloads
    void ClearRegisters(RegisterSnapshot<ArkSteedRegister> &registers);
    void ClearRegisters(RegisterSnapshot<ArkSteedDoubleRegister> &registers);

    template <typename RegisterT, bool spill>
    void ClearRegisters(RegisterSnapshot<RegisterT> &registers);

    void AllocateSpillSlot(ValueVertex *vertex);
    void AllocateVertexResult(ValueVertex *vertex);
    void AllocateFixedSlotResult(ValueVertex *vertex);
    void AllocateByPolicy(ValueVertex *vertex, UnallocatedState &operand);

    // Template DropRegisterValue
    template <typename RegisterT>
    void DropRegisterValue(RegisterSnapshot<RegisterT> &registers, RegisterT reg, bool forceSpill = false);

    // Template DropRegisterValueAtEnd
    template <typename RegisterT>
    void DropRegisterValueAtEnd(RegisterSnapshot<RegisterT> &registers, RegisterT reg, bool forceSpill = false);

    // DropRegisterValue forwarding layer
    void DropRegisterValue(ArkSteedRegister reg, bool forceSpill = false);
    void DropRegisterValue(ArkSteedDoubleRegister reg, bool forceSpill = false);

    // DropRegisterValueAtEnd forwarding layer
    void DropRegisterValueAtEnd(ArkSteedRegister reg, bool forceSpill = false);
    void DropRegisterValueAtEnd(ArkSteedDoubleRegister reg, bool forceSpill = false);

    void FreeRegistersUsedBy(ValueVertex *vertex);

    template <typename RegisterT>
    RegisterT FreeUnblockedRegister(RegisterSnapshot<RegisterT> &registers, RegListBase<RegisterT> reserved)
    {
        // Pick a register to free
        RegisterT best = PickRegisterToFree(registers.Blocked() | reserved);
        ASSERT(best.IsValid());
        ASSERT(!registers.IsBlocked(best));
        // Drop the value in that register
        DropRegisterValue(registers, best);
        registers.AddToFree(best);
        return best;
    }

    // Get reserved registers for a vertex (e.g., fixed result register)
    template <typename RegisterT>
    RegListBase<RegisterT> GetReservedRegisters(Vertex *vertex);

    AllocatedState AllocateRegister(ValueVertex *vertex);
    AllocatedState AllocateRegister(ValueVertex *vertex, const InstructionOperand &hint);
    AllocatedState AllocateRegisterAtEnd(ValueVertex *vertex);

    template <typename RegisterT>
    AllocatedState ForceAllocate(RegisterSnapshot<RegisterT> &registers, RegisterT reg, ValueVertex *vertex);

    // Template helper to allocate register
    template <typename RegisterT>
    AllocatedState AllocateRegisterInternal(RegisterSnapshot<RegisterT> &registers, ValueVertex *vertex);

    AllocatedState ForceAllocate(ArkSteedRegister reg, ValueVertex *vertex);
    AllocatedState ForceAllocate(ArkSteedDoubleRegister reg, ValueVertex *vertex);
    AllocatedState ForceAllocate(const Input &input, ValueVertex *vertex);

    // Phi allocation helpers
    void TryAllocateToInput(PhiVertex *phi);

    void VerifyRegisterState();
    bool AllUsedRegistersLiveAt(BB *block);

    void InitializeBranchTargetPhis(int predecessorId, BB *target);
    void InitializeBranchTargetRegisterValues(ControlVertex *control, BB *target);
    void MergeRegisterValues(ControlVertex *control, BB *target, int predecessorId);
    template <typename RegisterT>
    void MergeRegisterState(RegisterSnapshot<RegisterT> &registers, RegisterT reg, RegisterState &state,
                            ControlVertex *control, BB *target, int predecessorId, int predecessorCount);
    template <typename RegisterT>
    void CreateRegisterMerge(RegisterSnapshot<RegisterT> &registers, RegisterT reg, RegisterState &state,
                             ControlVertex *control, BB *target, int predecessorId, int predecessorCount,
                             ValueVertex *vertex, ValueVertex *incoming, const AllocatedState &registerOperand);
    void InitializeConditionalBranchTarget(ControlVertex *controlVertex, BB *target);
    void InitializeEmptyBlockRegisterValues(ControlVertex *source, BB *target);
    void UpdateUse(ValueVertex *vertex, InputLocation *inputLocation);
    bool IsLiveAtTarget(ValueVertex *vertex, ControlVertex *source, BB *target);

    void ClearRegisterValues();
    void InitializeRegisterValues(RegisterMergeState &registerState);
    void DebugDumpRegisterValues(RegisterMergeState &registerState, uint32_t predecessorCount);

    template <typename Function>
    void ForEachRegisterMergeState(RegisterMergeState &mergeState, Function &&f);

    bool IsCurrentVertexLastUse(ValueVertex *vertex);

    // to do: Remove this function after debugging done
    void DumpCurrentRegisters(std::string_view prompt = "");

    Graph *graph_;
    BB *currentBlock_ = nullptr;
    Vertex *currentVertex_ = nullptr;
    ChunkVector<GapMovePatch> patches_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_H
