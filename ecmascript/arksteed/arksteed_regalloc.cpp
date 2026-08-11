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

#include "ecmascript/arksteed/arksteed_regalloc.h"

#include <algorithm>
#include <type_traits>

#include "ecmascript/arksteed/arksteed_compiler.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

namespace {
void VerifyLazyDeoptInputLocations(const Vertex *vertex)
{
    if (!vertex->GetProperties().CanLazyDeopt()) {
        return;
    }
    const LazyDeoptimizableMixin *deopt = LazyDeoptMixinOf(vertex);
    ASSERT(deopt != nullptr);
    for (uint32_t index = 0, valueCount = deopt->GetDeoptFrameValueCount(); index < valueCount; ++index) {
        const InputLocation *location = deopt->GetDeoptSourceLocation(index);
        ASSERT(location->IsStackSlot() || location->IsConstant());
    }
}
}  // namespace

namespace {

template <typename VertexT, typename Function>
void ForEachEagerDeoptFrameValue(VertexT *vertex, Function &&function)
{
    if constexpr (std::is_base_of_v<EagerDeoptimizableMixin, VertexT> && VertexT::PROPERTIES.CanEagerDeopt()) {
        for (uint32_t index = 0; index < vertex->GetDeoptFrameValueCount(); ++index) {
            function(vertex->GetDeoptFrameValue(index), vertex->GetDeoptSourceLocation(index));
        }
    } else {
        UNREACHABLE();
    }
}

bool SameAsInput(ValueVertex *vertex, const Input &input)
{
    const ValueLocation &result = vertex->GetRegallocInfo()->GetResult();
    if (!result.IsUnallocated()) {
        return false;
    }
    UnallocatedState operand = UnallocatedState::Cast(result.GetOperand());
    return operand.HasSameAsInputPolicy() && input == Input(vertex, operand.GetInputIndex());
}

const InstructionOperand &InputHint(Vertex *resultVertex, const Input &input)
{
    ValueVertex *valueVertex = resultVertex->TryCast<ValueVertex>();
    if (valueVertex != nullptr && SameAsInput(valueVertex, input)) {
        return valueVertex->GetRegallocInfo()->GetHint();
    }
    return input.vertex()->GetRegallocInfo()->GetHint();
}

template <typename RegisterT>
RegisterT GetAllocatedRegister(const InstructionOperand &operand)
{
    if (!operand.IsAnyRegister()) {
        return RegisterT::Invalid();
    }

    AllocatedState allocated = AllocatedState::Cast(operand);
    if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
        if (operand.IsRegister()) {
            return allocated.GetRegister();
        }
    } else {
        static_assert(std::is_same_v<RegisterT, ArkSteedDoubleRegister>);
        if (operand.IsDoubleRegister()) {
            return allocated.GetDoubleRegister();
        }
    }
    return RegisterT::Invalid();
}

}  // namespace

// =============================================================================
// ArkSteedRegisterAllocator implementation
// =============================================================================

ArkSteedRegisterAllocator::ArkSteedRegisterAllocator(Graph *graph) : graph_(graph), patches_(graph->GetChunk())
{
    AllocateRegisters();

    uint32_t taggedStackSlots = tagged_.top;
    uint32_t untaggedStackSlots = untagged_.top;

    taggedStackSlots = taggedStackSlots + !(taggedStackSlots & 1);
    untaggedStackSlots = untaggedStackSlots + (untaggedStackSlots & 1);

    graph_->SetTaggedStackSlots(taggedStackSlots);
    graph_->SetUntaggedStackSlots(untaggedStackSlots);
}

ArkSteedRegisterAllocator::~ArkSteedRegisterAllocator() = default;

void ArkSteedRegisterAllocator::AddMoveBeforeCurrentVertex(ValueVertex *vertex, const InstructionOperand &source,
                                                           const AllocatedState &target)
{
    // Create a GapMove vertex for the register-to-register move
    Chunk *chunk = graph_->GetChunk();
    NonControlVertex *gapMove = nullptr;
    if (source.IsConstant()) {
        gapMove = Vertex::New<ConstantGapMoveVertex>(chunk, 0, vertex, target);
    } else {
        gapMove = Vertex::New<GapMoveVertex>(chunk, 0, AllocatedState::Cast(source), target);
    }

    // Set up the regalloc info for the gap move
    gapMove->SetRegallocInfo(
        chunk->New<RegallocValueVertexInfo>(chunk, gapMove->GetInputCount(), vertex->GetMachineRepresentation()));

    // Register the gap move vertex with the graph labeller if available
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    if (labeller != nullptr) {
        labeller->RegisterVertex(gapMove);
    }

    // Record the patch: insert gapMove before currentVertex_'s id
    VertexId beforeId = currentVertex_->GetId();
    patches_.push_back(GapMovePatch(beforeId, gapMove));
}

void ArkSteedRegisterAllocator::ApplyPatches(BB *block)
{
    auto &vertices = block->GetVertices();

    if (patches_.empty()) {
        return;
    }

#ifndef NDEBUG
    for (size_t i = 1; i < patches_.size(); ++i) {
        ASSERT(patches_[i - 1].beforeVertexId <= patches_[i].beforeVertexId);
    }
    for (size_t i = 1; i < vertices.size(); ++i) {
        ASSERT(vertices[i - 1]->GetId() <= vertices[i]->GetId());
    }
#endif

    std::vector<NonControlVertex *> newVertices;
    newVertices.reserve(vertices.size() + patches_.size());
    size_t patchIndex = 0;

    for (size_t i = 0; i < vertices.size(); ++i) {
        VertexId currentVertexId = vertices[i]->GetId();

        while (patchIndex < patches_.size() && patches_[patchIndex].beforeVertexId == currentVertexId) {
            newVertices.push_back(patches_[patchIndex].gapMove);
            patchIndex++;
        }

        newVertices.push_back(vertices[i]);
    }

    // Append remaining gap moves at the end
    while (patchIndex < patches_.size()) {
        ASSERT(patches_[patchIndex].beforeVertexId == block->GetControlVertex()->GetId());
        newVertices.push_back(patches_[patchIndex].gapMove);
        patchIndex++;
    }

    // Replace vertices with new array
    block->ReplaceVertices(newVertices);
    patches_.clear();
}

void ArkSteedRegisterAllocator::SetupConstantLocations()
{
    for (const auto &[value, constant] : graph_->GetInt32Constants()) {
        constant->GetRegallocInfo()->SetConstantLocation(constant->GetId());
    }
    for (const auto &[value, constant] : graph_->GetIntPtrConstants()) {
        constant->GetRegallocInfo()->SetConstantLocation(constant->GetId());
    }
    for (const auto &[value, constant] : graph_->GetFloat64Constants()) {
        constant->GetRegallocInfo()->SetConstantLocation(constant->GetId());
    }
    for (const auto &[value, constant] : graph_->GetTaggedConstants()) {
        constant->GetRegallocInfo()->SetConstantLocation(constant->GetId());
    }
}

void ArkSteedRegisterAllocator::InitializeBlockState(BB *block)
{
    if (block->HasRegisterMergeState()) {
        if (block->IsExceptionHandler()) {
            ClearRegisterValues();
        } else {
            RegisterMergeState &state = *block->GetRegisterMergeState();
            InitializeRegisterValues(state);
        }
    }
}

void ArkSteedRegisterAllocator::AllocateBlock(BB *block)
{
    currentBlock_ = block;
    ASSERT(block->GetId() != INVALID_BLOCK_ID);

    InitializeBlockState(block);

    // Activate phis.
    AllocatePhis(block);
    ASSERT(AllUsedRegistersLiveAt(block));
    VerifyRegisterState();

    for (NonControlVertex *vertex : block->GetVertices()) {
        BB *catchBlock = CatchBlockOf(vertex);
        if (catchBlock != nullptr && catchBlock->HasPhi()) {
            SpillCatchPhiInputsOfIndex(catchBlock, CatchPredecessorIndexOf(vertex));
        }
        AllocateVertex(vertex);
    }

    auto *controlVertex = block->GetControlVertex();
    AllocateControlVertex(controlVertex, block);
    ApplyPatches(block);
}

void ArkSteedRegisterAllocator::AllocateRegisters()
{
    SetupConstantLocations();

    for (auto blockIt = graph_->begin(); blockIt != graph_->end(); ++blockIt) {
        AllocateBlock(*blockIt);
    }

    // Clean up remaining register allocations at the end
    ClearRegisters();
}

void ArkSteedRegisterAllocator::AllocateVertex(Vertex *vertex)
{
    // We shouldn't be visiting any gap moves during allocation, we should only
    // have inserted gap moves in past visits.
    ASSERT(!vertex->Is<GapMoveVertex>());
    ASSERT(!vertex->Is<ConstantGapMoveVertex>());

    currentVertex_ = vertex;
    AssignInputs(vertex);

    // Spill registers if this is a call
    if (vertex->GetProperties().IsCall()) {
        SpillAndClearRegisters();
    } else if (vertex->GetProperties().IsASMBarrierCall()) {
        SpillAndClearASMBarrierClobbers();
    }
    // Save after inputs and temporaries have their physical locations.
    if (vertex->GetProperties().NeedsRegisterSnapshot()) {
        SaveDeferredRegisterSnapshot(vertex);
    }

    // Allocate vertex output.
    if (vertex->Is<ValueVertex>()) {
        AllocateVertexResult(static_cast<ValueVertex *>(vertex));
    }

    if (vertex->GetProperties().CanEagerDeopt()) {
        AssignEagerDeoptFrameSourceLocations(vertex);
    }

    if (vertex->GetProperties().CanLazyDeopt()) {
        VerifyLazyDeoptInputLocations(vertex);
    }

    if (vertex->Is<ValueVertex>()) {
        auto *vertexInfo = static_cast<ValueVertex *>(vertex)->GetRegallocInfo();
        if (!vertexInfo->IsDoubleRegister()) {
            ASSERT(vertexInfo->GetRegisterResult().IsEmpty() ||
                   !vertexInfo->GetGeneralTemporaries().Has(vertexInfo->GetRegisterResult().First()));
        } else {
            ASSERT(vertexInfo->GetDoubleRegisterResult().IsEmpty() ||
                   !vertexInfo->GetDoubleTemporaries().Has(vertexInfo->GetDoubleRegisterResult().First()));
        }
        ASSERT((generalRegisters_.Free() | vertexInfo->GetGeneralTemporaries()) == generalRegisters_.Free());
        ASSERT((doubleRegisters_.Free() | vertexInfo->GetDoubleTemporaries()) == doubleRegisters_.Free());
    }

    // Clear blocked registers
    generalRegisters_.ClearBlocked();
    doubleRegisters_.ClearBlocked();

    VerifyRegisterState();
}

void ArkSteedRegisterAllocator::ProcessUnconditionalControl(UnconditionalControlVertex *unconditional, BB *block)
{
    ASSERT(currentVertex_->GetTemporariesNeeded() == 0);
    ASSERT(currentVertex_->GetDoubleTemporariesNeeded() == 0);
    ASSERT(currentVertex_->GetInputCount() == 0);
    ASSERT(!currentVertex_->GetProperties().CanEagerDeopt());
    ASSERT(!currentVertex_->GetProperties().CanLazyDeopt());
    ASSERT(!currentVertex_->GetProperties().NeedsRegisterSnapshot());
    ASSERT(!currentVertex_->GetProperties().IsCall());

    auto predecessorId = block->GetPredecessorId();
    auto *target = unconditional->Target();

    if (target->HasRegisterMergeState()) {
        // Not a fallthrough
        InitializeBranchTargetPhis(predecessorId, target);
        MergeRegisterValues(unconditional, target, predecessorId);
        if (target->HasPhi()) {
            for (PhiVertex *phi : target->GetPhis()) {
                UpdateUse(phi->GetPredecessor(predecessorId), phi->GetInputLocation(predecessorId));
            }
        }
    } else {
        // Fallthrough
        ASSERT(currentVertex_->GetId() + 1 == target->GetFirstId());
        ASSERT(AllUsedRegistersLiveAt(target));
    }

    // Handle JumpLoop - extend lifetimes of vertices used in loop but not defined in loop
    if (auto *jumpLoop = currentVertex_->TryCast<JumpLoopVertex>()) {
        for (auto &inputPair : jumpLoop->GetUsedVertices()) {
            ValueVertex *inputVertex = inputPair.first;
            InputLocation *inputLocation = &inputPair.second;
            if (!inputVertex->GetRegallocInfo()->HasRegisterResult() && !inputVertex->GetRegallocInfo()->IsLoadable()) {
                // If the value isn't loadable by the end of a loop (this can happen
                // e.g. when a deferred throw doesn't spill it, and an exception
                // handler drops the value)
                ASSERT(false);  // to do: Temporary
                Spill(inputVertex);
            }
            UpdateUse(inputVertex, inputLocation);
        }
    }
}

void ArkSteedRegisterAllocator::ProcessConditionalOrReturn(ControlVertex *vertex, BB * /*block*/)
{
    // ConditionalControlVertex or Return
    ASSERT(vertex->Is<BranchControlVertex>() || vertex->Is<ReturnVertex>());

    // Assign inputs
    AssignInputs(vertex);

    ASSERT(!vertex->GetProperties().CanEagerDeopt());
    ASSERT(!vertex->GetProperties().CanLazyDeopt());

    // Spill registers if this is a call
    if (vertex->GetProperties().IsCall()) {
        SpillAndClearRegisters();
    }

    ASSERT(!vertex->GetProperties().NeedsRegisterSnapshot());

    // Verify register allocation state
    ASSERT((generalRegisters_.Free() | vertex->GetRegallocInfo()->GetGeneralTemporaries()) ==
           generalRegisters_.Free());
    ASSERT((doubleRegisters_.Free() | vertex->GetRegallocInfo()->GetDoubleTemporaries()) == doubleRegisters_.Free());

    // Clear blocked registers
    generalRegisters_.ClearBlocked();
    doubleRegisters_.ClearBlocked();
    VerifyRegisterState();

    // Initialize branch target states for conditional branches.
    if (auto *branch = vertex->TryCast<BranchControlVertex>()) {
        InitializeConditionalBranchTarget(vertex, branch->IfTrue());
        InitializeConditionalBranchTarget(vertex, branch->IfFalse());
    }
}

void ArkSteedRegisterAllocator::AllocateControlVertex(ControlVertex *vertex, BB *block)
{
    currentVertex_ = vertex;

    if (vertex->Is<ThrowVertex>()) {
        BB *catchBlock = CatchBlockOf(vertex);
        if (catchBlock != nullptr && catchBlock->HasPhi()) {
            SpillCatchPhiInputsOfIndex(catchBlock, CatchPredecessorIndexOf(vertex));
        }
        AllocateVertex(vertex);
    } else if (vertex->Is<DeoptVertex>()) {
        AllocateVertex(vertex);
    } else if (auto *unconditional = vertex->TryCast<UnconditionalControlVertex>()) {
        ProcessUnconditionalControl(unconditional, block);
    } else {
        ProcessConditionalOrReturn(vertex, block);
    }

    VerifyRegisterState();
}

void ArkSteedRegisterAllocator::MarkAsClobbered(ValueVertex *vertex, const AllocatedState &location)
{
    ASSERT(vertex != nullptr);
    ASSERT(location.IsAnyRegister());

    auto *vertexInfo = vertex->GetRegallocInfo();
    if (location.IsDoubleRegister()) {
        ArkSteedDoubleRegister reg = location.GetDoubleRegister();
        if (doubleRegisters_.GetValueMaybeFree(reg) != vertex) {
            return;
        }
        if (!vertexInfo->HasNoMoreUses() && !vertexInfo->IsLoadable() && vertexInfo->GetRegisterCount() == 1) {
            Spill(vertex);
        }
        vertexInfo->RemoveRegister(reg);
        doubleRegisters_.DropValueAt(reg);
        if (!doubleRegisters_.Free().Has(reg)) {
            doubleRegisters_.AddToFree(reg);
        }
        return;
    }

    ArkSteedRegister reg = location.GetRegister();
    if (generalRegisters_.GetValueMaybeFree(reg) != vertex) {
        return;
    }
    if (!vertexInfo->HasNoMoreUses() && !vertexInfo->IsLoadable() && vertexInfo->GetRegisterCount() == 1) {
        Spill(vertex);
    }
    vertexInfo->RemoveRegister(reg);
    generalRegisters_.DropValueAt(reg);
    if (!generalRegisters_.Free().Has(reg)) {
        generalRegisters_.AddToFree(reg);
    }
}

void ArkSteedRegisterAllocator::AssignInputs(Vertex *vertex)
{
    // We allocate arbitrary register inputs after fixed inputs, since the fixed
    // inputs may clobber the arbitrarily chosen ones. Finally we assign the
    // location for the remaining inputs. Since inputs can alias a vertex, one of
    // the inputs could be assigned a register in AssignArbitraryRegisterInput
    // (and respectively its vertex location), therefore we wait until all
    // registers are allocated before assigning any location for these inputs.
    for (uint32_t i = 0, n = vertex->GetInputCount(); i < n; i++) {
        Input input(vertex, i);
        AssignFixedInput(input);
    }
    AssignFixedTemporaries(vertex);
    for (uint32_t i = 0, n = vertex->GetInputCount(); i < n; i++) {
        Input input(vertex, i);
        if (!input.GetOperand().IsUnallocated()) {
            continue;
        }
        AssignArbitraryRegisterInput(vertex, input);
    }
    AssignArbitraryTemporaries(vertex);
    for (uint32_t i = 0, n = vertex->GetInputCount(); i < n; i++) {
        Input input(vertex, i);
        if (!input.GetOperand().IsUnallocated()) {
            continue;
        }
        AssignAnyInput(input);
    }
    AssignDeoptInputs(vertex);
}

void ArkSteedRegisterAllocator::AssignDeoptInput(ValueVertex *vertex, InputLocation *location)
{
    const InstructionOperand &operand = location->GetOperand();
    ASSERT(operand.IsUnallocated());
    UnallocatedState unallocated = UnallocatedState::Cast(operand);
    ASSERT(unallocated.GetExtendedPolicy() == UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT);

    ASSERT(vertex != currentVertex_);

    const InstructionOperand &currentLocation = vertex->GetRegallocInfo()->GetAllocation();
    if (currentLocation.IsConstant()) {
        location->GetOperand() = currentLocation;
        return;
    }
    if (currentLocation.IsAnyStackSlot()) {
        location->GetOperand() = currentLocation;
        UpdateUse(vertex, location);
        return;
    }

    ASSERT(currentLocation.IsRegister());
    auto *vertexInfo = vertex->GetRegallocInfo();
    if (vertexInfo->IsLoadable()) {
        location->GetOperand() = vertexInfo->GetSpillSlot();
        UpdateUse(vertex, location);
        return;
    }
    if (!vertexInfo->IsSpilled()) {
        AllocateSpillSlot(vertex);
    }
    AllocatedState spillSlot = AllocatedState::Cast(vertexInfo->GetSpillSlot());
    location->SetAllocated(spillSlot);
    AddMoveBeforeCurrentVertex(vertex, currentLocation, spillSlot);
    UpdateUse(vertex, location);
    vertexInfo->ClearHint();
}

template <class VertexT, class Callback>
void AssignDeoptInputsFor(VertexT *vertex, Callback assign)
{
    if constexpr (std::is_base_of_v<LazyDeoptimizableMixin, VertexT>) {
        if (vertex->HasLazyDeoptFrameState()) {
            for (uint32_t index = 0, valueCount = vertex->GetDeoptFrameValueCount(); index < valueCount; ++index) {
                assign(vertex->GetDeoptFrameValue(index), vertex->GetDeoptSourceLocation(index));
            }
        }
    }
}

void ArkSteedRegisterAllocator::AssignDeoptInputs(Vertex *vertex)
{
    auto assign = [this](ValueVertex *source, InputLocation *location) {
        AssignDeoptInput(source, location);
    };
    switch (vertex->GetOpcode()) {
#define ASSIGN_DEOPT_INPUTS(type)                                      \
        case VertexOpcode::type:                                       \
            AssignDeoptInputsFor(vertex->Cast<type##Vertex>(), assign); \
            break;
        ALL_VERTEX_LIST(ASSIGN_DEOPT_INPUTS)
#undef ASSIGN_DEOPT_INPUTS
        default:
            UNREACHABLE();
    }
}


void ArkSteedRegisterAllocator::AssignFixedInput(const Input &input)
{
    auto unallocated = UnallocatedState::Cast(input.GetOperand());
    bool clobbersInput = unallocated.IsUsedAtStart();
    ValueVertex *vertex = input.vertex();
    const InstructionOperand &location = vertex->GetRegallocInfo()->GetAllocation();

    switch (unallocated.GetExtendedPolicy()) {
        case UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER:
            // Allocated in AssignArbitraryRegisterInput.
            return;

        case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT:
            // Allocated in AssignAnyInput.
            return;

        case UnallocatedState::ExtendedPolicy::FIXED_REGISTER: {
            ArkSteedRegister reg = ArkSteedRegister::FromCode(unallocated.GetFixedRegisterIndex());
            input.GetLocation()->SetAllocated(ForceAllocate(reg, vertex));
            break;
        }

        case UnallocatedState::ExtendedPolicy::FIXED_FP_REGISTER: {
            ArkSteedDoubleRegister reg = RegListRegisterTraits<ArkSteedDoubleRegister>::FromCode(unallocated.GetFixedRegisterIndex());
            input.GetLocation()->SetAllocated(ForceAllocate(reg, vertex));
            break;
        }

        case UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT: {
            auto *vertexInfo = vertex->GetRegallocInfo();
            if (location.IsConstant()) {
                input.GetLocation()->GetOperand() = location;
                return;
            }
            if (location.IsAnyStackSlot()) {
                input.GetLocation()->GetOperand() = location;
                UpdateUse(vertex, input.GetLocation());
                return;
            }

            ASSERT(location.IsRegister());
            ASSERT(vertexInfo->HasValidLiveRange());
            if (vertexInfo->IsLoadable()) {
                input.GetLocation()->GetOperand() = vertexInfo->GetSpillSlot();
                UpdateUse(vertex, input.GetLocation());
                return;
            }
            if (!vertexInfo->IsSpilled()) {
                AllocateSpillSlot(vertex);
            }
            AllocatedState spillSlot = AllocatedState::Cast(vertexInfo->GetSpillSlot());
            input.GetLocation()->SetAllocated(spillSlot);
            UpdateUse(vertex, input.GetLocation());
            vertexInfo->ClearHint();
            return;
        }

        case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT:
        case UnallocatedState::ExtendedPolicy::SAME_AS_INPUT:
        case UnallocatedState::ExtendedPolicy::NONE:
            UNREACHABLE();
    }

    AllocatedState allocated = AllocatedState::Cast(input.GetOperand());
    if (location != allocated) {
        AddMoveBeforeCurrentVertex(vertex, location, allocated);
    }
    UpdateUse(vertex, input.GetLocation());
    if (clobbersInput) {
        MarkAsClobbered(vertex, allocated);
    }
    vertex->GetRegallocInfo()->ClearHint();
}

void ArkSteedRegisterAllocator::AssignArbitraryRegisterInput(Vertex *resultVertex, const Input &input)
{
    // Already assigned in AssignFixedInput
    const InstructionOperand &operand = input.GetOperand();
    ASSERT(operand.IsUnallocated());

    auto unallocated = UnallocatedState::Cast(operand);
    if (unallocated.GetExtendedPolicy() == UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT) {
        // Allocated in AssignAnyInput.
        return;
    }

    ASSERT(unallocated.GetExtendedPolicy() == UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER);
    bool clobbersInput = unallocated.IsUsedAtStart();

    ValueVertex *vertex = input.vertex();
    InstructionOperand resultHint;
    ValueVertex *valueVertex = resultVertex->TryCast<ValueVertex>();
    if (valueVertex != nullptr && SameAsInput(valueVertex, input)) {
        resultHint = valueVertex->GetRegallocInfo()->GetHint();
    }

    InstructionOperand location;  // Default type is INVALID
    if (clobbersInput) {
        // For clobbered inputs, pick a register that is not blocked by another
        // live input, so that we do not clobber a value that is still needed.
        if (vertex->GetMachineRepresentation() == MachineRepresentation::Float64) {
            location = doubleRegisters_.TryChooseUnblockedInputRegister(vertex);
        } else {
            location = generalRegisters_.TryChooseUnblockedInputRegister(vertex);
        }
    } else {
        // Only use the hint if it helps with the result's allocation due to
        // same-as-input policy. Otherwise this doesn't affect regalloc.
        InstructionOperand resultHint = InstructionOperand();
        ValueVertex *valueVertex = resultVertex->TryCast<ValueVertex>();
        if (valueVertex != nullptr && SameAsInput(valueVertex, input)) {
            resultHint = valueVertex->GetRegallocInfo()->GetHint();
        }
        if (vertex->GetMachineRepresentation() == MachineRepresentation::Float64) {
            location = doubleRegisters_.TryChooseInputRegister(vertex, resultHint);
        } else {
            location = generalRegisters_.TryChooseInputRegister(vertex, resultHint);
        }
    }

    if (location.IsInvalid()) {
        // Otherwise, allocate a register for the vertex and load it in from there.
        InstructionOperand existingLocation = vertex->GetRegallocInfo()->GetAllocation();
        const InstructionOperand &hint = InputHint(resultVertex, input);
        AllocatedState allocation = AllocateRegister(vertex, hint);
        ASSERT(existingLocation != allocation);
        AddMoveBeforeCurrentVertex(vertex, existingLocation, allocation);

        location = allocation;
    }

    input.GetLocation()->SetAllocated(AllocatedState::Cast(location));

    UpdateUse(vertex, input.GetLocation());
    if (clobbersInput) {
        MarkAsClobbered(vertex, AllocatedState::Cast(location));
    }
}

void ArkSteedRegisterAllocator::AssignAnyInput(const Input &input)
{
    const InstructionOperand &operand = input.GetOperand();
    ASSERT(operand.IsUnallocated());

    auto policy = UnallocatedState::Cast(operand).GetExtendedPolicy();
    ValueVertex *vertex = input.vertex();
    if (policy == UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT) {
        InstructionOperand location = vertex->GetRegallocInfo()->GetAllocation();
        if (!location.IsAnyStackSlot() && !location.IsConstant()) {
            Spill(vertex);
            location = vertex->GetRegallocInfo()->GetSpillSlot();
        }
        ASSERT(location.IsAnyStackSlot() || location.IsConstant());
        input.GetLocation()->InjectLocation(location);
        UpdateUse(vertex, input.GetLocation());
        return;
    }

    ASSERT(policy == UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT ||
           policy == UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT);
    InstructionOperand location = vertex->GetRegallocInfo()->GetAllocation();

    input.GetLocation()->InjectLocation(location);
    if (location.IsAnyRegister()) {
        AllocatedState allocation = AllocatedState::Cast(location);
        if (allocation.IsDoubleRegister()) {
            doubleRegisters_.Block(allocation.GetDoubleRegister());
        } else {
            generalRegisters_.Block(allocation.GetRegister());
        }
    }
    UpdateUse(vertex, input.GetLocation());
}

void ArkSteedRegisterAllocator::AssignDeoptFrameSourceLocation(
    ValueVertex *value, InputLocation *sourceLocation)
{
    auto *vertexInfo = value->GetRegallocInfo();
    if (!vertexInfo->HasRegisterResult() && !vertexInfo->IsLoadable()) {
        Spill(value);
    }
    sourceLocation->InjectLocation(vertexInfo->GetAllocation());
    UpdateUse(value, sourceLocation);
}

void ArkSteedRegisterAllocator::AssignEagerDeoptFrameSourceLocations(Vertex *vertex)
{
    ASSERT(vertex->GetProperties().CanEagerDeopt());
    auto assignLocation = [this](ValueVertex *value, InputLocation *sourceLocation) {
        AssignDeoptFrameSourceLocation(value, sourceLocation);
    };
    switch (vertex->GetOpcode()) {
#define ASSIGN_DEOPT_FRAME_SOURCE_LOCATIONS_CASE(Name)                       \
        case VertexOpcode::Name: {                                           \
            ForEachEagerDeoptFrameValue(                                     \
                vertex->Cast<Name##Vertex>(), assignLocation);               \
            return;                                                          \
        }
        ALL_VERTEX_LIST(ASSIGN_DEOPT_FRAME_SOURCE_LOCATIONS_CASE)
#undef ASSIGN_DEOPT_FRAME_SOURCE_LOCATIONS_CASE
        case VertexOpcode::INVALID:
            break;
    }
    UNREACHABLE();
}

void ArkSteedRegisterAllocator::AssignFixedTemporaries(Vertex *vertex)
{
    AssignFixedTemporaries(generalRegisters_, vertex);
    AssignFixedTemporaries(doubleRegisters_, vertex);
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::AssignFixedTemporaries(RegisterSnapshot<RegisterT> &registers, Vertex *vertex)
{
    auto *vertexInfo = vertex->GetRegallocInfo();

    RegListBase<RegisterT> specificTemporaries =
        vertexInfo->template GetRequiredSpecificTemporaries<RegisterT>();

    // Make sure that any initially set temporaries are definitely free.
    for (RegisterT reg : specificTemporaries) {
        ASSERT(!registers.IsBlocked(reg));
        if (!registers.Free().Has(reg)) {
            DropRegisterValue(registers, reg);
            registers.AddToFree(reg);
        }
        registers.Block(reg);
    }
}

void ArkSteedRegisterAllocator::AssignArbitraryTemporaries(Vertex *vertex)
{
    AssignArbitraryTemporaries(generalRegisters_, vertex);
    AssignArbitraryTemporaries(doubleRegisters_, vertex);
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::AssignArbitraryTemporaries(RegisterSnapshot<RegisterT> &registers, Vertex *vertex)
{
    auto *vertexInfo = vertex->GetRegallocInfo();

    // Get number of temporaries needed based on register type
    int numTemporariesNeeded = vertex->template GetNumTemporariesNeeded<RegisterT>();
    if (numTemporariesNeeded == 0) {
        return;
    }

    RegListBase<RegisterT> temporaries = vertexInfo->template GetTemporaries<RegisterT>();
    ASSERT(temporaries.IsEmpty());

    int remainingTemporariesNeeded = numTemporariesNeeded;

    // If the vertex is a ValueVertex with a fixed result register, we should not
    // assign a temporary to the result register, nor its hint.
    RegListBase<RegisterT> reserved = GetReservedRegisters<RegisterT>(vertex);

    // First, try to allocate from unblocked free registers (excluding reserved)
    for (RegisterT reg : (registers.UnblockedFree() - reserved)) {
        registers.Block(reg);
        ASSERT(!temporaries.Has(reg));
        temporaries.Set(reg);
        if (--remainingTemporariesNeeded == 0) {
            break;
        }
    }

    // Free extra registers if necessary.
    for (int i = 0; i < remainingTemporariesNeeded; ++i) {
        RegisterT reg = FreeUnblockedRegister<RegisterT>(registers, reserved);
        registers.Block(reg);
        temporaries.Set(reg);
    }

    vertexInfo->template GetTemporaries<RegisterT>() = temporaries;
}

template <typename RegisterT>
RegListBase<RegisterT> ArkSteedRegisterAllocator::GetReservedRegisters(Vertex *vertex)
{
    auto *valueVertex = vertex->TryCast<ValueVertex>();
    if (valueVertex == nullptr) {
        return RegListBase<RegisterT>();
    }

    auto *vertexInfo = valueVertex->GetRegallocInfo();
    const ValueLocation &result = vertexInfo->GetResult();
    const InstructionOperand &hint = vertexInfo->GetHint();

    RegListBase<RegisterT> reserved;

    RegisterT hintReg = GetRegisterHint<RegisterT>(hint);
    if (hintReg.IsValid()) {
        reserved.Set(hintReg);
    }

    ASSERT(result.IsUnallocated());
    const UnallocatedState &operand = UnallocatedState::Cast(result.GetOperand());

    // If fixed slot, just return the hint
    if (operand.IsFixedSlotPolicy()) {
        return reserved;
    }

    // Check for fixed register
    if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
        if (operand.HasFixedRegisterPolicy()) {
            reserved.Set(ArkSteedRegister::FromCode(operand.GetFixedRegisterIndex()));
        }
    } else {
        static_assert(std::is_same_v<RegisterT, ArkSteedDoubleRegister>);
        if (operand.HasFixedFPRegisterPolicy()) {
            reserved.Set(RegListRegisterTraits<ArkSteedDoubleRegister>::FromCode(operand.GetFixedRegisterIndex()));
        }
    }

    return reserved;
}

template <typename RegisterT>
RegisterT ArkSteedRegisterAllocator::PickRegisterToFree(RegListBase<RegisterT> reserved)
{
    RegisterSnapshot<RegisterT> &registers = GetRegisterSnapshot<RegisterT>();
    int furthestUse = 0;
    RegisterT best = RegisterT::Invalid();

    for (RegisterT reg : (registers.Used() - reserved)) {
        ValueVertex *value = registers.GetValue(reg);
        auto *vertexInfo = value->GetRegallocInfo();

        // The cheapest register to clear is a register containing a value that's
        // contained in another register as well. Since we found the register while
        // looping over unblocked registers, we can simply use this register.
        if (vertexInfo->GetRegisterCount() > 1) {
            best = reg;
            break;
        }

        if (vertexInfo->HasNoMoreUses()) {
            return reg;
        }

        int use = static_cast<int>(vertexInfo->CurrentNextUse());
        if (use > furthestUse) {
            furthestUse = use;
            best = reg;
        }
    }

    return best;
}

void ArkSteedRegisterAllocator::Spill(ValueVertex *vertex)
{
    if (vertex->GetRegallocInfo()->IsLoadable()) {
        return;
    }
    AllocateSpillSlot(vertex);
}

void ArkSteedRegisterAllocator::SpillRegisters()
{
    auto spill = [&](auto reg, ValueVertex *vertex) { Spill(vertex); };
    generalRegisters_.ForEachUsedRegister(spill);
    doubleRegisters_.ForEachUsedRegister(spill);
}

void ArkSteedRegisterAllocator::SpillAndClearRegisters()
{
    SpillAndClearRegisters(generalRegisters_);
    SpillAndClearRegisters(doubleRegisters_);
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::SpillAndClearRegisters(RegisterSnapshot<RegisterT> &registers,
                                                       RegListBase<RegisterT> clobbered)
{
    RegListBase<RegisterT> usedClobbered = registers.Used() & clobbered;
    while (usedClobbered != registers.Empty()) {
        RegisterT reg = usedClobbered.First();
        ValueVertex *vertex = registers.GetValue(reg);
        Spill(vertex);
        registers.FreeRegistersUsedBy(vertex);
        ASSERT(!registers.Used().Has(reg));
        usedClobbered = registers.Used() & clobbered;
    }
}

void ArkSteedRegisterAllocator::SpillAndClearASMBarrierClobbers()
{
#if defined(PANDA_TARGET_AMD64)
    SpillAndClearRegisters(generalRegisters_, ArkSteedRegList{x64::r11});
    SpillAndClearRegisters(doubleRegisters_, GetAllocatableDoubleRegisters());
#elif defined(PANDA_TARGET_ARM64)
    SpillAndClearRegisters(generalRegisters_, ArkSteedRegList{aarch64::x15});
    SpillAndClearRegisters(doubleRegisters_,
        ArkDoubleRegList{aarch64::d0,  aarch64::d1,  aarch64::d2,  aarch64::d3,
                         aarch64::d4,  aarch64::d5,  aarch64::d6,  aarch64::d7,
                         aarch64::d16, aarch64::d17, aarch64::d18, aarch64::d19,
                         aarch64::d20, aarch64::d21, aarch64::d22, aarch64::d23,
                         aarch64::d24, aarch64::d25, aarch64::d26, aarch64::d27,
                         aarch64::d28, aarch64::d29});
#endif
}

void ArkSteedRegisterAllocator::SaveDeferredRegisterSnapshot(Vertex *vertex)
{
    DeferredRegisterSnapshot snapshot;
    snapshot.liveRegisters = generalRegisters_.Used();
    snapshot.liveDoubleRegisters = doubleRegisters_.Used();
    vertex->GetRegallocInfo()->SetDeferredRegisterSnapshot(snapshot);
}

void ArkSteedRegisterAllocator::SpillCatchPhiInputsOfIndex(BB *catchBlock, uint32_t index)
{
    ASSERT(catchBlock != nullptr);
    for (PhiVertex *phi : catchBlock->GetPhis()) {
        if (!phi->GetRegallocInfo()->HasValidLiveRange()) {
            continue;
        }
        ValueVertex *inputVertex = phi->GetInput(index);
        // Ensure the input value is on the stack so it persists.
        Spill(inputVertex);
        InputLocation *inputLocation = phi->GetInputLocation(index);
        auto *info = inputVertex->GetRegallocInfo();
        // Prefer the spill slot so the location remains valid even after
        // AssignInputs moves/frees the register.
        if (info->IsSpilled()) {
            inputLocation->InjectLocation(info->GetSpillSlot());
        } else {
            inputLocation->InjectLocation(info->GetAllocation());
        }
    }
}

void ArkSteedRegisterAllocator::ClearRegisterValues()
{
    auto clearRegisterState = [&](auto &registers) {
        while (!registers.Used().IsEmpty()) {
            auto reg = registers.Used().First();
            ValueVertex *vertex = registers.GetValue(reg);
            registers.FreeRegistersUsedBy(vertex);
            ASSERT(!registers.Used().Has(reg));
        }
    };

    clearRegisterState(generalRegisters_);
    clearRegisterState(doubleRegisters_);

    // All registers should be free by now.
    ASSERT(generalRegisters_.UnblockedFree() == GetAllocatableList<ArkSteedRegister>());
    ASSERT(doubleRegisters_.UnblockedFree() == GetAllocatableList<ArkSteedDoubleRegister>());
}

// =============================================================================
// Template DropRegisterValue implementation
// =============================================================================
template <typename RegisterT>
void ArkSteedRegisterAllocator::DropRegisterValue(RegisterSnapshot<RegisterT> &registers, RegisterT reg,
                                                  bool forceSpill)
{
    // The register should not already be free
    ASSERT(!registers.Free().Has(reg));

    ValueVertex *vertex = registers.GetValue(reg);
    ASSERT(vertex != nullptr);

    auto *vertexInfo = vertex->GetRegallocInfo();
    MachineRepresentation machRepr = vertex->GetMachineRepresentation();

    // Remove the register from the vertex's list
    vertexInfo->RemoveRegister(reg);

    // Return if the removed value already has another register or is loadable from memory
    if (vertexInfo->HasRegisterResult() || vertexInfo->IsLoadable()) {
        return;
    }

    // Try to move the value to another register
    if (!registers.UnblockedFreeIsEmpty() && !forceSpill) {
        RegisterT targetReg = registers.UnblockedFree().First();
        RegisterT hintReg = GetRegisterHint<RegisterT>(vertexInfo->GetHint());
        if (hintReg.IsValid() && registers.UnblockedFree().Has(hintReg)) {
            targetReg = hintReg;
        }
        registers.RemoveFromFree(targetReg);
        registers.SetValueWithoutBlocking(targetReg, vertex);

        // Emit a gap move
        AllocatedState source(AllocatedState::LocationKind::REGISTER, machRepr, reg.Code());
        AllocatedState target(AllocatedState::LocationKind::REGISTER, machRepr, targetReg.Code());
        AddMoveBeforeCurrentVertex(vertex, source, target);
        return;
    }

    // If all else fails, spill the value
    Spill(vertex);
}

template void ArkSteedRegisterAllocator::DropRegisterValue(RegisterSnapshot<ArkSteedRegister> &registers,
                                                           ArkSteedRegister reg, bool forceSpill);
template void ArkSteedRegisterAllocator::DropRegisterValue(RegisterSnapshot<ArkSteedDoubleRegister> &registers,
                                                           ArkSteedDoubleRegister reg, bool forceSpill);

// DropRegisterValue forwarding layer
void ArkSteedRegisterAllocator::DropRegisterValue(ArkSteedRegister reg, bool forceSpill)
{
    DropRegisterValue<ArkSteedRegister>(generalRegisters_, reg, forceSpill);
}

void ArkSteedRegisterAllocator::DropRegisterValue(ArkSteedDoubleRegister reg, bool forceSpill)
{
    DropRegisterValue<ArkSteedDoubleRegister>(doubleRegisters_, reg, forceSpill);
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::DropRegisterValueAtEnd(RegisterSnapshot<RegisterT> &registers, RegisterT reg,
                                                       bool forceSpill)
{
    registers.Unblock(reg);
    if (!registers.Free().Has(reg)) {
        ValueVertex *vertex = registers.GetValue(reg);
        // If the register is not live after the current vertex, just remove its value
        if (IsCurrentVertexLastUse(vertex)) {
            vertex->GetRegallocInfo()->RemoveRegister(reg);
        } else {
            DropRegisterValue<RegisterT>(registers, reg, forceSpill);
        }
        registers.AddToFree(reg);
    }
}

void ArkSteedRegisterAllocator::DropRegisterValueAtEnd(ArkSteedRegister reg, bool forceSpill)
{
    DropRegisterValueAtEnd(generalRegisters_, reg, forceSpill);
}

void ArkSteedRegisterAllocator::DropRegisterValueAtEnd(ArkSteedDoubleRegister reg, bool forceSpill)
{
    DropRegisterValueAtEnd(doubleRegisters_, reg, forceSpill);
}

AllocatedState ArkSteedRegisterAllocator::AllocateRegister(ValueVertex *vertex)
{
    return AllocateRegister(vertex, InstructionOperand());
}

AllocatedState ArkSteedRegisterAllocator::AllocateRegister(ValueVertex *vertex, const InstructionOperand &hint)
{
    auto *vertexInfo = vertex->GetRegallocInfo();
    if (vertexInfo->IsDoubleRegister()) {
        return AllocateRegisterInternal(doubleRegisters_, vertex, hint);
    } else {
        return AllocateRegisterInternal(generalRegisters_, vertex, hint);
    }
}

AllocatedState ArkSteedRegisterAllocator::AllocateRegisterAtEnd(ValueVertex *vertex)
{
    auto *vertexInfo = vertex->GetRegallocInfo();
    const InstructionOperand &hint = vertexInfo->GetHint();
    if (vertexInfo->IsDoubleRegister()) {
        EnsureFreeRegisterAtEnd(doubleRegisters_, hint);
        return doubleRegisters_.AllocateRegister(vertex, hint);
    }
    EnsureFreeRegisterAtEnd(generalRegisters_, hint);
    return generalRegisters_.AllocateRegister(vertex, hint);
}

template <typename RegisterT>
RegisterT ArkSteedRegisterAllocator::FindReusableBlockedInputRegister(RegisterSnapshot<RegisterT> &registers,
                                                                      RegisterT hintReg)
{
    RegisterT fallback = RegisterT::Invalid();
    for (uint32_t i = 0; i < currentVertex_->GetInputCount(); i++) {
        RegisterT reg = GetAllocatedRegister<RegisterT>(currentVertex_->GetInputLocation(i)->GetOperand());
        if (!reg.IsValid()) {
            continue;
        }
        if (!registers.Free().Has(reg) || !registers.IsBlocked(reg)) {
            continue;
        }
        if (reg == hintReg) {
            return reg;
        }
        if (!fallback.IsValid()) {
            fallback = reg;
        }
    }
    return fallback;
}

template <typename RegisterT>
RegisterT ArkSteedRegisterAllocator::FindLastUseBlockedRegister(RegisterSnapshot<RegisterT> &registers,
                                                                RegisterT hintReg)
{
    RegisterT fallback = RegisterT::Invalid();
    for (RegisterT reg : (registers.Blocked() - registers.Free())) {
        ValueVertex *value = registers.GetValue(reg);
        if (value == nullptr || !IsCurrentVertexLastUse(value)) {
            continue;
        }
        if (reg == hintReg) {
            return reg;
        }
        if (!fallback.IsValid()) {
            fallback = reg;
        }
    }
    return fallback;
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::EnsureFreeRegisterAtEnd(RegisterSnapshot<RegisterT> &registers,
                                                        const InstructionOperand &hint)
{
    if (!registers.UnblockedFreeIsEmpty()) {
        return;
    }

    RegisterT hintReg = GetRegisterHint<RegisterT>(hint);

    // Last-use inputs are freed during UpdateUse but remain blocked until the current vertex finishes.
    // They are safe result registers, unlike arbitrary free-and-blocked temporaries.
    RegisterT reg = FindReusableBlockedInputRegister(registers, hintReg);
    if (reg.IsValid()) {
        registers.Unblock(reg);
        return;
    }

    reg = FindLastUseBlockedRegister(registers, hintReg);
    if (reg.IsValid()) {
        DropRegisterValueAtEnd(registers, reg);
        return;
    }

    reg = hintReg;
    if (!reg.IsValid() || registers.Free().Has(reg)) {
        reg = PickRegisterToFree<RegisterT>(RegListBase<RegisterT>());
    }
    ASSERT(reg.IsValid());
    DropRegisterValueAtEnd(registers, reg);
}

template <typename RegisterT>
AllocatedState ArkSteedRegisterAllocator::ForceAllocate(RegisterSnapshot<RegisterT> &registers, RegisterT reg,
                                                        ValueVertex *vertex)
{
    ASSERT(!registers.IsBlocked(reg));

    if (registers.Free().Has(reg)) {
        // If it's already free, remove it from the free list
        registers.RemoveFromFree(reg);
    } else if (registers.GetValue(reg) == vertex) {
        // If the register already contains this vertex, just block it
        registers.Block(reg);
        return AllocatedState(AllocatedState::LocationKind::REGISTER, vertex->GetMachineRepresentation(), reg.Code());
    } else {
        // Register is in use by a different vertex, drop its value
        ASSERT(!registers.IsBlocked(reg));
        DropRegisterValue(registers, reg);
    }

    // After DropRegisterValue, the register is in free list but may be blocked
    // Unblock it first, then set value (SetValue will block again)
    registers.Unblock(reg);
    registers.SetValue(reg, vertex);

    return AllocatedState(AllocatedState::LocationKind::REGISTER, vertex->GetMachineRepresentation(), reg.Code());
}

template AllocatedState ArkSteedRegisterAllocator::ForceAllocate(RegisterSnapshot<ArkSteedRegister> &registers,
                                                                 ArkSteedRegister reg, ValueVertex *vertex);
template AllocatedState ArkSteedRegisterAllocator::ForceAllocate(RegisterSnapshot<ArkSteedDoubleRegister> &registers,
                                                                 ArkSteedDoubleRegister reg, ValueVertex *vertex);

template <typename RegisterT>
void ArkSteedRegisterAllocator::SetLoopPhiRegisterHint(PhiVertex *phi, RegisterT reg)
{
    UnallocatedState::ExtendedPolicy policy;
    if constexpr (std::is_same_v<RegisterT, ArkSteedRegister>) {
        policy = UnallocatedState::ExtendedPolicy::FIXED_REGISTER;
    } else {
        static_assert(std::is_same_v<RegisterT, ArkSteedDoubleRegister>);
        policy = UnallocatedState::ExtendedPolicy::FIXED_FP_REGISTER;
    }

    UnallocatedState hint(policy, reg.Code(), NO_VREG);
    for (int i = 0, n = phi->GetInputCount(); i < n; i++) {
        ValueVertex *input = phi->GetInput(i);
        if (input->GetId() > phi->GetId()) {
            input->SetHint(hint);
        }
    }
}

template <typename RegisterT>
AllocatedState ArkSteedRegisterAllocator::AllocateRegisterInternal(RegisterSnapshot<RegisterT> &registers,
                                                                   ValueVertex *vertex,
                                                                   const InstructionOperand &hint)
{
    if (registers.UnblockedFreeIsEmpty()) {
        RegListBase<RegisterT> emptyReserved;
        FreeUnblockedRegister(registers, emptyReserved);
    }
    // Allocate from unblocked free registers
    return registers.AllocateRegister(vertex, hint);
}

template AllocatedState ArkSteedRegisterAllocator::AllocateRegisterInternal(
    RegisterSnapshot<ArkSteedRegister> &registers, ValueVertex *vertex, const InstructionOperand &hint);
template AllocatedState ArkSteedRegisterAllocator::AllocateRegisterInternal(
    RegisterSnapshot<ArkSteedDoubleRegister> &registers, ValueVertex *vertex, const InstructionOperand &hint);

AllocatedState ArkSteedRegisterAllocator::ForceAllocate(ArkSteedRegister reg, ValueVertex *vertex)
{
    return ForceAllocate<ArkSteedRegister>(generalRegisters_, reg, vertex);
}

AllocatedState ArkSteedRegisterAllocator::ForceAllocate(ArkSteedDoubleRegister reg, ValueVertex *vertex)
{
    return ForceAllocate<ArkSteedDoubleRegister>(doubleRegisters_, reg, vertex);
}

AllocatedState ArkSteedRegisterAllocator::ForceAllocate(const Input &input, ValueVertex *vertex)
{
    auto *inputLocation = input.GetLocation();
    if (inputLocation->IsAnyRegister()) {
        if (inputLocation->IsDoubleRegister()) {
            ArkSteedDoubleRegister reg = inputLocation->GetAssignedDoubleRegister();
            DropRegisterValueAtEnd(reg);
            return ForceAllocate(reg, vertex);
        }

        ArkSteedRegister reg = inputLocation->GetAssignedGeneralRegister();
        DropRegisterValueAtEnd(reg);
        return ForceAllocate(reg, vertex);
    }

    // SAME_AS_INPUT inputs are expected to have been materialized by AssignInputs.
    UNREACHABLE();
}

void ArkSteedRegisterAllocator::AllocateSpillSlot(ValueVertex *vertex)
{
    ASSERT(!vertex->GetRegallocInfo()->IsLoadable());
    auto *vertexInfo = vertex->GetRegallocInfo();

    MachineRepresentation rep = vertexInfo->GetRepresentation();
    bool isTagged = (rep == MachineRepresentation::Tagged);
    bool doubleSlot = (rep == MachineRepresentation::Float64);

    SpillLocations &slots = isTagged ? tagged_ : untagged_;
    uint32_t freeSlot = slots.top;
    bool reuseSlot = false;
    if (graph_->GetReuseStackSlots() && vertexInfo->HasValidLiveRange() && !slots.freeSlots.empty()) {
        VertexId start = vertexInfo->GetLiveRange().start;
#ifndef NDEBUG
        for (size_t i = 1; i < slots.freeSlots.size(); ++i) {
            ASSERT(slots.freeSlots[i - 1].freedAtPosition <= slots.freeSlots[i].freedAtPosition);
        }
#endif
        // freeSlots is sorted by freedAtPosition ascending. Find the first slot
        // freed at or after start; all earlier slots are reusable.
        auto it = std::upper_bound(slots.freeSlots.begin(), slots.freeSlots.end(), start,
                                   [](VertexId s, const SpillInfo &slotInfo) {
                                       return slotInfo.freedAtPosition >= s;
                                   });
        // Step backwards through reusable slots and pick the newest one that
        // also matches the slot width (double vs normal).
        while (it != slots.freeSlots.begin()) {
            --it;
            if (it->doubleSlot == doubleSlot) {
                ASSERT(it->freedAtPosition < start);
                freeSlot = it->slotIndex;
                slots.freeSlots.erase(it);
                reuseSlot = true;
                break;
            }
        }
    }
    if (!reuseSlot) {
        freeSlot = slots.top++;
    }

    AllocatedState spillSlot(AllocatedState::STACK_SLOT, rep, freeSlot);
    vertexInfo->SetSpillSlot(spillSlot);
}

void ArkSteedRegisterAllocator::AllocateFixedSlotResult(ValueVertex *vertex)
{
    auto *vertexInfo = vertex->GetRegallocInfo();
    auto &resultLocation = vertexInfo->GetResult();
    UnallocatedState &operand = static_cast<UnallocatedState &>(resultLocation.GetOperand());
    ASSERT(vertex->Is<InitialValueVertex>());
    // Set the stack slot to exactly where the value is.
    int32_t slotIndex = operand.GetFixedSlotIndex();
    AllocatedState location(AllocatedState::LocationKind::STACK_SLOT, vertex->GetMachineRepresentation(), slotIndex);
    vertexInfo->SetResultAllocated(location);
    vertexInfo->Spill(location);
}

void ArkSteedRegisterAllocator::AllocateByPolicy(ValueVertex *vertex, UnallocatedState &operand)
{
    auto *vertexInfo = vertex->GetRegallocInfo();
    switch (operand.GetExtendedPolicy()) {
        case UnallocatedState::ExtendedPolicy::FIXED_REGISTER: {
            ArkSteedRegister reg = ArkSteedRegister::FromCode(operand.GetFixedRegisterIndex());
            DropRegisterValueAtEnd(reg, true);
            vertexInfo->SetResultAllocated(ForceAllocate(reg, vertex));
            break;
        }

        case UnallocatedState::ExtendedPolicy::MUST_HAVE_REGISTER: {
            vertexInfo->SetResultAllocated(AllocateRegisterAtEnd(vertex));
            break;
        }

        case UnallocatedState::ExtendedPolicy::SAME_AS_INPUT: {
            uint32_t inputIndex = operand.GetInputIndex();
            ValueVertex *inputVertex = vertex->GetInput(inputIndex);
            Input input(vertex, inputIndex);
            AllocatedState allocation = ForceAllocate(input, vertex);
            vertexInfo->SetResultAllocated(allocation);
            // Clear any hint that (probably) comes from this constraint.
            if (vertexInfo->HasHint()) {
                inputVertex->GetRegallocInfo()->ClearHint();
            }
            break;
        }

        case UnallocatedState::ExtendedPolicy::FIXED_FP_REGISTER: {
            ArkSteedDoubleRegister reg = RegListRegisterTraits<ArkSteedDoubleRegister>::FromCode(operand.GetFixedRegisterIndex());
            DropRegisterValueAtEnd(reg, true);
            vertexInfo->SetResultAllocated(ForceAllocate(reg, vertex));
            break;
        }

        case UnallocatedState::ExtendedPolicy::NONE:
            // Constant vertex - nothing to allocate
            break;

        case UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT:
        case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT:
        case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT:
            UNREACHABLE();
            break;
    }
}

void ArkSteedRegisterAllocator::AllocateVertexResult(ValueVertex *vertex)
{
    ASSERT(!vertex->Is<PhiVertex>());

    auto *vertexInfo = vertex->GetRegallocInfo();

    auto &resultLocation = vertexInfo->GetResult();
    UnallocatedState &operand = static_cast<UnallocatedState &>(resultLocation.GetOperand());

    if (operand.GetBasicPolicy() == UnallocatedState::BasicPolicy::FIXED_SLOT) {
        vertexInfo->SetNoSpill();
        AllocateFixedSlotResult(vertex);
        return;
    }
    if (operand.GetExtendedPolicy() != UnallocatedState::ExtendedPolicy::NONE) {
        vertexInfo->SetNoSpill();
    }

    AllocateByPolicy(vertex, operand);

    // Immediately kill the register use if the vertex doesn't have a valid live-range.
    // to do: Remove once we can avoid allocating such registers.

    if (!vertexInfo->HasValidLiveRange() && resultLocation.IsAnyRegister()) {
        ASSERT(vertexInfo->HasRegisterResult());
        FreeRegistersUsedBy(vertex);
        ASSERT(!vertexInfo->HasRegisterResult());
        ASSERT(vertexInfo->HasNoMoreUses());
    }
}

void ArkSteedRegisterAllocator::TryAllocateToInput(PhiVertex *phi)
{
    // Try allocate phis to a register used by any of the inputs
    uint32_t inputCount = phi->GetInputCount();
    bool isDoublePhi = phi->GetRegallocInfo()->IsDoubleRegister();
    // Hint-based preference is currently only implemented for general registers.
    if (!isDoublePhi) {
        ArkSteedRegister hintReg = GetRegisterHint<ArkSteedRegister>(phi->GetRegallocInfo()->GetHint());
        // Prefer the hinted register if one of the incoming values already uses it.
        for (uint32_t i = 0; i < inputCount; i++) {
            Input input(phi, i);
            if (!input.GetOperand().IsRegister()) {
                continue;
            }
            if (input.GetLocation()->GetAssignedGeneralRegister() == hintReg &&
                generalRegisters_.UnblockedFree().Has(hintReg)) {
                phi->GetRegallocInfo()->SetResultAllocated(ForceAllocate(hintReg, phi));
                SetLoopPhiRegisterHint(phi, hintReg);
                return;
            }
        }
    }

    // Otherwise, fall back to the first reusable incoming register.
    for (uint32_t i = 0; i < inputCount; i++) {
        Input input(phi, i);
        if (isDoublePhi && input.GetOperand().IsDoubleRegister()) {
            ArkSteedDoubleRegister reg = input.GetLocation()->GetAssignedDoubleRegister();
            if (doubleRegisters_.UnblockedFree().Has(reg)) {
                phi->GetRegallocInfo()->SetResultAllocated(ForceAllocate(reg, phi));
                return;
            }
        } else if (!isDoublePhi && input.GetOperand().IsRegister()) {
            ArkSteedRegister reg = input.GetLocation()->GetAssignedGeneralRegister();
            if (generalRegisters_.UnblockedFree().Has(reg)) {
                phi->GetRegallocInfo()->SetResultAllocated(ForceAllocate(reg, phi));
                SetLoopPhiRegisterHint(phi, reg);
                return;
            }
        }
    }
}

void ArkSteedRegisterAllocator::FreeRegistersUsedBy(ValueVertex *vertex)
{
    auto *vertexInfo = vertex->GetRegallocInfo();
    if (vertexInfo->IsDoubleRegister()) {
        doubleRegisters_.FreeRegistersUsedBy(vertex);
    } else {
        generalRegisters_.FreeRegistersUsedBy(vertex);
    }
}

bool ArkSteedRegisterAllocator::IsCurrentVertexLastUse(ValueVertex *vertex)
{
    ASSERT(vertex != nullptr);
    // Uses the fixed live-range end, not HasNoMoreUses(): reuse callers run
    // before UpdateUse() advances the dynamic next-use for the current input.
    return vertex->GetRegallocInfo()->GetEndId() == currentVertex_->GetId();
}

void ArkSteedRegisterAllocator::VerifyRegisterState()  // to do:
{
#ifndef NDEBUG
    // We shouldn't have any blocked registers by now.
    ASSERT(generalRegisters_.Blocked().IsEmpty());
    ASSERT(doubleRegisters_.Blocked().IsEmpty());

    // Verify each used register has a valid value
    for (ArkSteedRegister reg : generalRegisters_.Used()) {
        ValueVertex *vertex = generalRegisters_.GetValue(reg);
        ASSERT(vertex->GetRegallocInfo()->IsInRegister(reg));
    }
    for (ArkSteedDoubleRegister reg : doubleRegisters_.Used()) {
        ValueVertex *vertex = doubleRegisters_.GetValue(reg);
        ASSERT(vertex->GetRegallocInfo()->IsInRegister(reg));
    }
#endif
}

bool ArkSteedRegisterAllocator::AllUsedRegistersLiveAt(BB *block)
{
    uint32_t blockFirstId = block->GetFirstId();

    auto forAllRegisters = [&](const auto &registers) {
        for (auto reg : registers.Used()) {
            ValueVertex *vertex = registers.GetValue(reg);
            auto *vertexInfo = vertex->GetRegallocInfo();
            if (vertexInfo->GetEndId() < blockFirstId) {
                return false;
            }
        }
        return true;
    };
    return forAllRegisters(generalRegisters_) && forAllRegisters(doubleRegisters_);
}

void ArkSteedRegisterAllocator::HoistLoopReloads(BB *target)
{
    BB::RegallocLoopInfo *loopInfo = target->GetRegallocLoopInfo();
    if (loopInfo == nullptr) {
        return;
    }

    for (ValueVertex *vertex : loopInfo->reloadHints) {
        RegallocValueVertexInfo *vertexInfo = vertex->GetRegallocInfo();
        ASSERT(generalRegisters_.Blocked().IsEmpty());
        if (generalRegisters_.Free().IsEmpty()) {
            break;
        }
        if (vertexInfo->IsDoubleRegister()) {
            // to do: Support double register reload hints.
            continue;
        }
        if (vertexInfo->HasRegisterResult()) {
            continue;
        }
        if (!vertexInfo->IsLoadable()) {
            continue;
        }

        ArkSteedRegister targetReg = GetRegisterHint<ArkSteedRegister>(vertexInfo->GetHint());
        if (!targetReg.IsValid() || !generalRegisters_.Free().Has(targetReg)) {
            targetReg = generalRegisters_.Free().First();
        }
        AllocatedState targetOperand(AllocatedState::LocationKind::REGISTER, vertex->GetMachineRepresentation(),
                                     targetReg.Code());
        generalRegisters_.RemoveFromFree(targetReg);
        generalRegisters_.SetValueWithoutBlocking(targetReg, vertex);
        AddMoveBeforeCurrentVertex(vertex, vertexInfo->GetSpillSlot(), targetOperand);
    }
}

void ArkSteedRegisterAllocator::HoistLoopSpills(BB *target)
{
    BB::RegallocLoopInfo *loopInfo = target->GetRegallocLoopInfo();
    if (loopInfo == nullptr) {
        return;
    }

    static constexpr bool FORCE_SPILL = true;
    for (ValueVertex *vertex : loopInfo->spillHints) {
        RegallocValueVertexInfo *vertexInfo = vertex->GetRegallocInfo();
        if (vertexInfo->IsDoubleRegister()) {
            continue;
        }
        if (!vertexInfo->HasRegisterResult()) {
            continue;
        }
        ArkSteedRegList registers = vertexInfo->GetRegisterResult();
        for (ArkSteedRegister reg : registers) {
            DropRegisterValueAtEnd(reg, FORCE_SPILL);
        }
    }
}

void ArkSteedRegisterAllocator::InitializeBranchTargetPhis(int predecessorId, BB *target)
{
    if (!target->HasPhi()) {
        return;
    }

    // Initialize phi input locations from the predecessor's allocation
    for (PhiVertex *phi : target->GetPhis()) {
        if (!phi->GetRegallocInfo()->HasValidLiveRange()) {
            // Dead phi - skip or handle if needed
            continue;
        }
        ValueVertex *inputVertex = phi->GetInput(predecessorId);
        InputLocation *inputLocation = phi->GetInputLocation(predecessorId);
        inputLocation->InjectLocation(inputVertex->GetRegallocInfo()->GetAllocation());
    }
}

void ArkSteedRegisterAllocator::InitializeBranchTargetRegisterValues(ControlVertex *control, BB *target)
{
    ASSERT(target->HasRegisterMergeState());

    RegisterMergeState &targetState = *target->GetRegisterMergeState();
    ASSERT(!targetState.IsInitialized());

    auto init = [&](auto &registers, auto reg, RegisterState &state) {
        ValueVertex *vertex = nullptr;
        ASSERT(registers.Blocked().IsEmpty());
        if (!registers.Free().Has(reg)) {
            vertex = registers.GetValue(reg);
            if (!IsLiveAtTarget(vertex, control, target)) {
                vertex = nullptr;
            }
        }
        state.SetValue(vertex);
        if (vertex != nullptr) {
            if (target->PredecessorCount() > 1 && !vertex->GetRegallocInfo()->IsLoadable()) {
                AllocatedState source(
                    LocationState::LocationKind::REGISTER, vertex->GetMachineRepresentation(), reg.Code());
                AllocateSpillSlot(vertex);
                AddMoveBeforeCurrentVertex(
                    vertex, source, AllocatedState::Cast(vertex->GetRegallocInfo()->GetSpillSlot()));
            }
        }
    };
    HoistLoopReloads(target);
    HoistLoopSpills(target);

    ForEachRegisterMergeState(targetState, init);
    ASSERT(targetState.IsInitialized());
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::CreateRegisterMerge(RegisterSnapshot<RegisterT> &registers, RegisterT reg,
                                                    RegisterState &state, ControlVertex *control, BB *target,
                                                    uint32_t predecessorId, uint32_t predecessorCount,
                                                    ValueVertex *vertex, ValueVertex *incoming,
                                                    const AllocatedState &registerOperand)
{
    auto *chunk = graph_->GetChunk();
    size_t size = sizeof(RegisterMergeInfo) + predecessorCount * sizeof(AllocatedState);
    auto *newMerge = static_cast<RegisterMergeInfo *>(chunk->Allocate(size));
    newMerge->vertex = (vertex != nullptr) ? vertex : incoming;

    InstructionOperand infoSoFar;
    if (vertex == nullptr) {
        auto *incomingInfo = incoming->GetRegallocInfo();
        if (!incomingInfo->IsLoadable()) {
            AllocatedState source(
                LocationState::LocationKind::REGISTER, incoming->GetMachineRepresentation(), reg.Code());
            AllocateSpillSlot(incoming);
            AddMoveBeforeCurrentVertex(
                incoming, source, AllocatedState::Cast(incomingInfo->GetSpillSlot()));
        }
        infoSoFar = incomingInfo->GetSpillSlot();
    } else {
        infoSoFar = registerOperand;
    }

    for (uint32_t i = 0; i < predecessorCount; i++) {
        newMerge->Operand(i) = infoSoFar;
    }

    if (vertex == nullptr) {
        newMerge->Operand(predecessorId) = registerOperand;
    } else {
        newMerge->Operand(predecessorId) = vertex->GetRegallocInfo()->GetAllocation();
    }

    state.SetMerge(newMerge);
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::MergeRegisterState(RegisterSnapshot<RegisterT> &registers, RegisterT reg,
                                                   RegisterState &state, ControlVertex *control, BB *target,
                                                   uint32_t predecessorId, uint32_t predecessorCount)
{
    ValueVertex *vertex = nullptr;
    RegisterMergeInfo *mergeInfo = nullptr;
    state.LoadMergeState(&vertex, &mergeInfo);

    ValueVertex *incoming = nullptr;
    ASSERT(registers.Blocked().IsEmpty());
    if (!registers.Free().Has(reg)) {
        incoming = registers.GetValue(reg);
        if (!IsLiveAtTarget(incoming, control, target)) {
            incoming = nullptr;
        }
    }

    using RegType = decltype(reg);
    constexpr bool isDouble = std::is_same_v<RegType, ArkSteedDoubleRegister>;
    auto makeRegisterOperand = [&](ValueVertex *value) {
        ASSERT(value != nullptr);
        MachineRepresentation machRep = isDouble ? MachineRepresentation::Float64 : value->GetMachineRepresentation();
        ASSERT(isDouble || !IsFloatingPoint(machRep));
        return AllocatedState(LocationState::LocationKind::REGISTER, machRep, reg.Code());
    };

    if (incoming == vertex) {
        if (mergeInfo != nullptr) {
            mergeInfo->Operand(predecessorId) = makeRegisterOperand(vertex);
        }
        return;
    }

    if (vertex == nullptr) {
        if (control->Is<JumpLoopVertex>()) {
            return;
        }
    } else {
        ASSERT(!(!vertex->GetRegallocInfo()->IsLoadable() &&
                 !vertex->GetRegallocInfo()->HasRegisterResult()));  // to do: Temporary
    }

    if (mergeInfo != nullptr) {
        ASSERT(vertex != nullptr);
        mergeInfo->Operand(predecessorId) = vertex->GetRegallocInfo()->GetAllocation();
        return;
    }

    ValueVertex *mergeVertex = (vertex != nullptr) ? vertex : incoming;
    AllocatedState registerOperand = makeRegisterOperand(mergeVertex);
    CreateRegisterMerge(registers,
                        reg,
                        state,
                        control,
                        target,
                        predecessorId,
                        predecessorCount,
                        vertex,
                        incoming,
                        registerOperand);
}

void ArkSteedRegisterAllocator::MergeRegisterValues(ControlVertex *control, BB *target, int predecessorId)
{
    ASSERT(target->HasRegisterMergeState());
    RegisterMergeState &targetState = *target->GetRegisterMergeState();

    if (!targetState.IsInitialized()) {
        // This is the first block we're merging, initialize the values.
        return InitializeBranchTargetRegisterValues(control, target);
    }

    uint32_t predecessorCount = target->PredecessorCount();

    auto merge = [&](auto &registers, auto reg, RegisterState &state) {
        MergeRegisterState(registers, reg, state, control, target, predecessorId, predecessorCount);
    };
    ForEachRegisterMergeState(targetState, merge);
}

void ArkSteedRegisterAllocator::InitializeConditionalBranchTarget(ControlVertex *controlVertex, BB *target)
{
    ASSERT(!target->HasPhi());

    if (target->HasRegisterMergeState()) {
        // Not a fall-through branch, copy the state over.
        return InitializeBranchTargetRegisterValues(controlVertex, target);
    } else {
        // Fallthrough
        ASSERT(AllUsedRegistersLiveAt(target));
    }
}

bool ArkSteedRegisterAllocator::IsLiveAtTarget(ValueVertex *vertex, ControlVertex *source, BB *target)
{
    ASSERT(vertex != nullptr);
    auto *vertexInfo = vertex->GetRegallocInfo();
    ASSERT(!vertexInfo->HasNoMoreUses());

    // If we're looping, a value can only be live if it was live before the loop.
    if (target->GetControlVertex()->GetId() <= source->GetId()) {
        // Gap moves may already be inserted in the target, so skip over those.
        return vertex->GetId() < target->GetFirstNonGapMoveId();
    }

    // Check if the vertex's live range extends to the target
    return vertexInfo->GetEndId() >= target->GetFirstId();
}

void ArkSteedRegisterAllocator::InitializeEmptyBlockRegisterValues(ControlVertex *source, BB *target)
{
    // Create a new merge point register state for the block
    auto *registerState = graph_->GetChunk()->New<RegisterMergeState>();

    ASSERT(!registerState->IsInitialized());

    auto init = [&](auto &registers, auto reg, RegisterState &state) {
        ValueVertex *vertex = nullptr;
        ASSERT(registers.Blocked().IsEmpty());
        if (!registers.Free().Has(reg)) {
            vertex = registers.GetValue(reg);
            if (!IsLiveAtTarget(vertex, source, target)) {
                vertex = nullptr;
            }
        }
        state.SetValue(vertex);
    };

    ForEachRegisterMergeState(*registerState, init);

    target->SetRegisterMergeState(registerState);
}

void ArkSteedRegisterAllocator::UpdateUse(ValueVertex *vertex, InputLocation *inputLocation)
{
    ASSERT(vertex != nullptr);
    ASSERT(inputLocation != nullptr);

    auto *vertexInfo = vertex->GetRegallocInfo();
    ASSERT(!vertexInfo->HasNoMoreUses());

    // Update the next use
    vertexInfo->AdvanceNextUse(inputLocation->GetNextUseId());

    if (!vertexInfo->HasNoMoreUses()) {
        return;
    }

    // If a value is dead, make sure it's cleared
    FreeRegistersUsedBy(vertex);

    if (vertexInfo->IsSpilled()) {
        // Value is dead: return its spill slot to the pool for later reuse.
        if (graph_->GetReuseStackSlots()) {
            AllocatedState spillSlot = AllocatedState::Cast(vertexInfo->GetSpillSlot());
            if (spillSlot.GetIndex() >= 0) {
                bool isTagged = (spillSlot.GetRepresentation() == MachineRepresentation::Tagged);
                bool doubleSlot = (spillSlot.GetRepresentation() == MachineRepresentation::Float64);
                SpillLocations &slots = isTagged ? tagged_ : untagged_;
                slots.freeSlots.emplace_back(static_cast<uint32_t>(spillSlot.GetIndex()), vertexInfo->GetEndId(),
                                             doubleSlot);
            }
        }
    }
}

template <typename Function>
void ArkSteedRegisterAllocator::ForEachRegisterMergeState(RegisterMergeState &mergeState, Function &&f)
{
    mergeState.ForEachGeneralRegister(
        [&](ArkSteedRegister reg, RegisterState &state) { f(generalRegisters_, reg, state); });
    mergeState.ForEachDoubleRegister(
        [&](ArkSteedDoubleRegister reg, RegisterState &state) { f(doubleRegisters_, reg, state); });
}

void ArkSteedRegisterAllocator::InitializeRegisterValues(RegisterMergeState &registerState)
{
    // First clear the register state.
    ClearRegisterValues();

    // Then fill it in with target information.
    auto fill = [&](auto &registers, auto reg, RegisterState &state) {
        ValueVertex *vertex = nullptr;
        RegisterMergeInfo *mergeInfo = nullptr;
        state.LoadMergeState(&vertex, &mergeInfo);
        if (vertex != nullptr) {
            registers.RemoveFromFree(reg);
            registers.SetValue(reg, vertex);
        } else {
            ASSERT(!state.IsMerge());
        }
    };
    ForEachRegisterMergeState(registerState, fill);

    // SetValue will have blocked registers, unblock them.
    generalRegisters_.ClearBlocked();
    doubleRegisters_.ClearBlocked();
}

void ArkSteedRegisterAllocator::ClearRegisters()
{
    ClearRegisters(generalRegisters_);
    ClearRegisters(doubleRegisters_);
}

void ArkSteedRegisterAllocator::ClearRegisters(RegisterSnapshot<ArkSteedRegister> &registers)
{
    ClearRegisters<ArkSteedRegister, false>(registers);
}

void ArkSteedRegisterAllocator::ClearRegisters(RegisterSnapshot<ArkSteedDoubleRegister> &registers)
{
    ClearRegisters<ArkSteedDoubleRegister, false>(registers);
}

template <typename RegisterT, bool spill>
void ArkSteedRegisterAllocator::ClearRegisters(RegisterSnapshot<RegisterT> &registers)
{
    while (registers.Used() != registers.Empty()) {
        RegisterT reg = registers.Used().First();
        ValueVertex *vertex = registers.GetValue(reg);
        if (spill) {
            Spill(vertex);
        }
        registers.FreeRegistersUsedBy(vertex);
        ASSERT(!registers.Used().Has(reg));
    }
    registers.ClearBlocked();
}

void ArkSteedRegisterAllocator::TryAllocatePhisToInput(ChunkVector<PhiVertex *> &phis)
{
    for (auto &phi : phis) {
        if (!phi->GetRegallocInfo()->HasValidLiveRange()) {
            // Skip dead Phis
            continue;
        }
        TryAllocateToInput(phi);
    }
}

void ArkSteedRegisterAllocator::TryAllocatePhisToRegister(ChunkVector<PhiVertex *> &phis)
{
    for (auto &phi : phis) {
        auto *phiInfo = (phi->GetRegallocInfo());
        if (!phi->GetRegallocInfo()->HasValidLiveRange()) {
            continue;
        }
        if (phiInfo->GetResult().IsAllocated()) {
            continue;
        }
        if (phiInfo->IsDoubleRegister()) {
            if (!doubleRegisters_.UnblockedFreeIsEmpty()) {
                AllocatedState allocation = AllocateRegister(phi, phiInfo->GetHint());
                phiInfo->SetResultAllocated(allocation);
                SetLoopPhiRegisterHint(phi, allocation.GetDoubleRegister());
            }
        } else {
            if (!generalRegisters_.UnblockedFreeIsEmpty()) {
                AllocatedState allocation = AllocateRegister(phi, phiInfo->GetHint());
                phiInfo->SetResultAllocated(allocation);
                SetLoopPhiRegisterHint(phi, allocation.GetRegister());
            }
        }
    }
}

void ArkSteedRegisterAllocator::SpillRemainingPhis(ChunkVector<PhiVertex *> &phis)
{
    for (auto &phi : phis) {
        auto *phiInfo = (phi->GetRegallocInfo());
        if (!phi->GetRegallocInfo()->HasValidLiveRange()) {
            continue;
        }
        if (phiInfo->GetResult().IsAllocated()) {
            continue;
        }
        AllocateSpillSlot(phi);
        phiInfo->SetResultAllocated(AllocatedState::Cast(phiInfo->GetSpillSlot()));
    }
}

void ArkSteedRegisterAllocator::AllocatePhis(BB *block)
{
    if (!block->HasPhi()) {
        return;
    }

    ChunkVector<PhiVertex *> &phis = block->GetPhis();

    // Firstly, make the phi live, and try to assign it to an input location.
    TryAllocatePhisToInput(phis);

    // Secondly try to assign the phi to a free register.
    TryAllocatePhisToRegister(phis);

    // Finally just use a stack slot.
    SpillRemainingPhis(phis);

    generalRegisters_.ClearBlocked();
    doubleRegisters_.ClearBlocked();
}

}  // namespace panda::ecmascript::arksteed
