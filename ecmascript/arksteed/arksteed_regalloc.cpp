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

#include "ecmascript/arksteed/arksteed_compiler.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

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
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << 'v' << vertex->GetId() << " ConstantGapMove: " << source.Description() << " -> "
                            << target.Description();
#endif
    } else {
        gapMove = Vertex::New<GapMoveVertex>(chunk, 0, AllocatedState::Cast(source), target);
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << 'v' << vertex->GetId() << " GapMove: " << source.Description() << " -> "
                            << target.Description();
#endif
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
    for (const auto &[index, constant] : graph_->GetRootConstants()) {
        constant->GetRegallocInfo()->SetConstantLocation(constant->GetId());  // to do
    }
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
    if (block->HasState()) {
        if (block->GetState()->IsExceptionHandler()) {
            ASSERT(false);  // to do: no catch now
            // Exceptions start with a blank state of register values.
            ClearRegisterValues();
        } else {
            RegisterMergeState &state = block->GetState()->RegisterState();
            InitializeRegisterValues(state);
#ifndef NDEBUG
            LOG_COMPILER(DEBUG) << "Register states after initialization:";
            DebugDumpRegisterValues(state, block->GetPredecessors().Size());
#endif
        }
    } else if (block->GetBlockType() == BB::EDGE_SPLIT) {
        RegisterMergeState *state = block->GetEdgeSplitBlockRegisterState();
        InitializeRegisterValues(*state);
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "Register states after initialization:";
        DebugDumpRegisterValues(*state, block->GetPredecessors().Size());
#endif
    }
}

void ArkSteedRegisterAllocator::ProcessBlockVertices(BB *block)
{
    for (NonControlVertex *vertex : block->GetVertices()) {
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "Visiting non-control vertex v" << vertex->GetId() << ": "
                            << OpcodeToString(vertex->GetOpcode());
#endif
        AllocateVertex(vertex);
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "Finished visiting non-control vertex v" << vertex->GetId() << ": "
                            << OpcodeToString(vertex->GetOpcode());
        DumpCurrentRegisters();
#endif
    }
}

void ArkSteedRegisterAllocator::AllocateBlock(BB *block)
{
    currentBlock_ = block;
    ASSERT(block->GetId() != INVALID_BLOCK_ID);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "========================================================================";
    LOG_COMPILER(DEBUG) << "RegAlloc: Starts BB #" << block->GetId()
                        << (block->GetBlockType() == BB::EDGE_SPLIT ? " [EDGE SPLIT]" : "");
#endif
    InitializeBlockState(block);

    // Activate phis.
    AllocatePhis(block);
#ifndef NDEBUG
    DumpCurrentRegisters("After allocating Phis");
#endif
    ASSERT(AllUsedRegistersLiveAt(block));
    VerifyRegisterState();

    ProcessBlockVertices(block);

    auto *controlVertex = block->GetControlVertex();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Visiting control vertex #" << controlVertex->GetId() << ": "
                        << OpcodeToString(controlVertex->GetOpcode());
#endif
    AllocateControlVertex(controlVertex, block);
    ApplyPatches(block);

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "RegAlloc: Finishes BB #" << block->GetId();
    LOG_COMPILER(DEBUG) << "========================================================================";
#endif
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
    VerifyInputs(vertex);

    // Spill registers if this is a call
    if (vertex->GetProperties().IsCall()) {
        SpillAndClearRegisters();
    }

    if (vertex->GetProperties().CanEagerDeopt()) {
        // to do: AllocateEagerDeopt
    }

    if (vertex->GetProperties().CanLazyDeopt()) {
        // to do: AllocateLazyDeopt
    }

    // Make sure to save snapshot after allocate eager deopt registers.
    if (vertex->GetProperties().NeedsRegisterSnapshot()) {
        // to do: SaveRegisterSnapshot
    }

    // Allocate vertex output.
    if (vertex->Is<ValueVertex>()) {
        AllocateVertexResult(static_cast<ValueVertex *>(vertex));
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

    if (target->HasState()) {
        // Not a fallthrough
        InitializeBranchTargetPhis(predecessorId, target);
        MergeRegisterValues(unconditional, target, predecessorId);
        if (target->HasPhi()) {
            for (PhiVertex *phi : target->GetPhis()) {
                UpdateUse(const_cast<ValueVertex *>(phi->GetPredecessor(predecessorId)),
                          phi->GetInputLocation(predecessorId));
            }
        }
    } else if (target->GetBlockType() == BB::EDGE_SPLIT) {
        // Edge split block
        InitializeEmptyBlockRegisterValues(currentVertex_->Cast<ControlVertex>(), target);
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
    ASSERT(vertex->Is<BranchIfTrueVertex>() || vertex->Is<ReturnVertex>());

    // Assign inputs
    AssignInputs(vertex);
    VerifyInputs(vertex);

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

    // Initialize branch target states for BranchIfTrue
    if (auto *branch = vertex->TryCast<BranchIfTrueVertex>()) {
        InitializeConditionalBranchTarget(vertex, branch->IfTrue());
        InitializeConditionalBranchTarget(vertex, branch->IfFalse());
    }
}

void ArkSteedRegisterAllocator::AllocateControlVertex(ControlVertex *vertex, BB *block)
{
    currentVertex_ = vertex;

    // to do: Add AbortVertex && DeoptVertex
    if (vertex->Is<ThrowVertex>()) {
        AllocateVertex(vertex);
    } else if (auto *unconditional = vertex->TryCast<UnconditionalControlVertex>()) {
        ProcessUnconditionalControl(unconditional, block);
    } else {
        ProcessConditionalOrReturn(vertex, block);
    }

    VerifyRegisterState();
}

void ArkSteedRegisterAllocator::MarkAsClobbered(ValueVertex * /*vertex*/, const AllocatedState & /*location*/)
{
    // Mark register as clobbered after use
}

void ArkSteedRegisterAllocator::AssignInputs(Vertex *vertex)
{
    // We allocate arbitrary register inputs after fixed inputs, since the fixed
    // inputs may clobber the arbitrarily chosen ones. Finally we assign the
    // location for the remaining inputs. Since inputs can alias a vertex, one of
    // the inputs could be assigned a register in AssignArbitraryRegisterInput
    // (and respectively its vertex location), therefore we wait until all
    // registers are allocated before assigning any location for these inputs.
    for (int i = 0; i < vertex->GetInputCount(); i++) {
        Input input(vertex, i);
        AssignFixedInput(input);
#ifndef NDEBUG
        if (input.GetOperand().IsAllocated()) {
            LOG_COMPILER(DEBUG) << "Input #" << i << " (which is v" << input.vertex()->GetId()
                                << ") is assigned to fixed location: " << input.GetOperand().Description();
        }
#endif
    }
    AssignFixedTemporaries(vertex);
    for (int i = 0; i < vertex->GetInputCount(); i++) {
        Input input(vertex, i);
        if (input.GetOperand().IsAllocated()) {
            continue;
        }
        AssignArbitraryRegisterInput(vertex, input);
#ifndef NDEBUG
        if (input.GetOperand().IsAllocated()) {
            LOG_COMPILER(DEBUG) << "Input #" << i << " (which is v" << input.vertex()->GetId()
                                << ") is assigned to arbitrary register: " << input.GetOperand().Description();
        }
#endif
    }
    AssignArbitraryTemporaries(vertex);
    for (int i = 0; i < vertex->GetInputCount(); i++) {
        Input input(vertex, i);
        if (input.GetOperand().IsAllocated()) {
            continue;
        }
        AssignAnyInput(input);
#ifndef NDEBUG
        if (input.GetOperand().IsAllocated()) {
            LOG_COMPILER(DEBUG) << "Input #" << i << " (which is v" << input.vertex()->GetId()
                                << ") is assigned to arbitrary location: " << input.GetOperand().Description();
        }
#endif
    }

    for (int i = 0; i < vertex->GetInputCount(); i++) {
        Input input(vertex, i);
#ifndef NDEBUG
        if (input.vertex()->GetRegallocInfo()->HasNoMoreUses()) {
            LOG_COMPILER(DEBUG) << "\tv" << input.vertex()->GetId() << " is no more live.";
        }
#endif
    }
#ifndef NDEBUG
    DumpCurrentRegisters("After allocating inputs");
#endif
}

void ArkSteedRegisterAllocator::AssignFixedInput(const Input &input)
{
    auto unallocated = UnallocatedState::Cast(input.GetOperand());
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
            ArkSteedDoubleRegister reg = ArkSteedDoubleRegister::FromCode(unallocated.GetFixedRegisterIndex());
            input.GetLocation()->SetAllocated(ForceAllocate(reg, vertex));
            break;
        }

        case UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT:
        case UnallocatedState::ExtendedPolicy::SAME_AS_INPUT:
        case UnallocatedState::ExtendedPolicy::NONE:
        case UnallocatedState::ExtendedPolicy::MUST_HAVE_SLOT:
            UNREACHABLE();
    }

    AllocatedState allocated = AllocatedState::Cast(input.GetOperand());
    if (location != allocated) {
        AddMoveBeforeCurrentVertex(vertex, location, allocated);
    }
    UpdateUse(vertex, input.GetLocation());
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

    ValueVertex *vertex = input.vertex();
    bool isClobbered = input.GetLocation()->IsClobbered();

    InstructionOperand location;  // Default type is INVALID
    isClobbered = false;         // Temporary
    if (isClobbered) {
    } else {
        ValueVertex *valueVertex = resultVertex->TryCast<ValueVertex>();
        InstructionOperand resultHint = InstructionOperand();  // to do: Temporary
        if (vertex->GetMachineRepresentation() == MachineRepresentation::Float64) {
            location = doubleRegisters_.TryChooseInputRegister(vertex, resultHint);
        } else {
            location = generalRegisters_.TryChooseInputRegister(vertex, resultHint);
        }
    }

    if (location.IsInvalid()) {
        // Otherwise, allocate a register for the vertex and load it in from there.
        InstructionOperand existingLocation = vertex->GetRegallocInfo()->GetAllocation();
        InstructionOperand hint;  // to do: Temporary
        AllocatedState allocation = AllocateRegister(vertex, hint);
        ASSERT(existingLocation != allocation);
        AddMoveBeforeCurrentVertex(vertex, existingLocation, allocation);

        location = allocation;
    }

    input.GetLocation()->SetAllocated(AllocatedState::Cast(location));

    UpdateUse(vertex, input.GetLocation());
    // Only need to mark the location as clobbered if the vertex wasn't already
    // killed by UpdateUse.
    if (isClobbered && !vertex->GetRegallocInfo()->HasNoMoreUses()) {
        ASSERT(false);  // to do: Temporary
        MarkAsClobbered(vertex, AllocatedState::Cast(location));
    }
}

void ArkSteedRegisterAllocator::AssignAnyInput(const Input &input)
{
    const InstructionOperand &operand = input.GetOperand();
    ASSERT(operand.IsUnallocated());

    ASSERT(UnallocatedState::Cast(operand).GetExtendedPolicy() ==
           UnallocatedState::ExtendedPolicy::REGISTER_OR_SLOT_OR_CONSTANT);

    ValueVertex *vertex = input.vertex();
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

void ArkSteedRegisterAllocator::AssignFixedTemporaries(Vertex *vertex)
{
    AssignFixedTemporaries(generalRegisters_, vertex);
    AssignFixedTemporaries(doubleRegisters_, vertex);
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::AssignFixedTemporaries(RegisterSnapshot<RegisterT> &registers, Vertex *vertex)
{
    auto *vertexInfo = vertex->GetRegallocInfo();

    // Get temporaries based on register type
    RegListBase<RegisterT> fixedTemporaries = vertexInfo->template GetTemporaries<RegisterT>();

    // Make sure that any initially set temporaries are definitely free.
    for (RegisterT reg : fixedTemporaries) {
        ASSERT(!registers.IsBlocked(reg));
        if (!registers.Free().Has(reg)) {
            DropRegisterValue(registers, reg);
            registers.AddToFree(reg);
        }
        registers.Block(reg);
    }

    // After allocating the specific/fixed temporary registers, we empty the vertex
    // set, so that it is used to allocate only the arbitrary/available temporary
    // register that is going to be inserted in the scratch scope.
    vertexInfo->template ClearTemporaries<RegisterT>();
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

    // to do: Add hint

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
            reserved.Set(ArkSteedDoubleRegister::FromCode(operand.GetFixedRegisterIndex()));
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
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "CALL OPERATION: Registers spilled and cleared.";
#endif
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
        // to do: use hint
        RegisterT targetReg = registers.UnblockedFree().First();
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
        return AllocateRegisterInternal(doubleRegisters_, vertex);
    } else {
        return AllocateRegisterInternal(generalRegisters_, vertex);
    }
}

AllocatedState ArkSteedRegisterAllocator::AllocateRegisterAtEnd(ValueVertex *vertex)
{
    return AllocateRegister(vertex);
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
AllocatedState ArkSteedRegisterAllocator::AllocateRegisterInternal(RegisterSnapshot<RegisterT> &registers,
                                                                   ValueVertex *vertex)
{
    if (registers.UnblockedFreeIsEmpty()) {
        RegListBase<RegisterT> emptyReserved;
        FreeUnblockedRegister(registers, emptyReserved);
    }
    // Allocate from unblocked free registers
    return registers.AllocateRegister(vertex, InstructionOperand());  // to do: Temporary hint
}

template AllocatedState ArkSteedRegisterAllocator::AllocateRegisterInternal(
    RegisterSnapshot<ArkSteedRegister> &registers, ValueVertex *vertex);
template AllocatedState ArkSteedRegisterAllocator::AllocateRegisterInternal(
    RegisterSnapshot<ArkSteedDoubleRegister> &registers, ValueVertex *vertex);

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
        // Input is in a register, use the same register
        if (inputLocation->IsDoubleRegister()) {
            ArkSteedDoubleRegister reg = inputLocation->GetAssignedDoubleRegister();
            return ForceAllocate(reg, vertex);
        } else {
            ArkSteedRegister reg = inputLocation->GetAssignedGeneralRegister();
            return ForceAllocate(reg, vertex);
        }
    } else {
        // Input is in memory, allocate a register
        return AllocateRegister(vertex);
    }
}

void ArkSteedRegisterAllocator::AllocateSpillSlot(ValueVertex *vertex)
{
    ASSERT(!vertex->GetRegallocInfo()->IsLoadable());
    auto *vertexInfo = vertex->GetRegallocInfo();

    MachineRepresentation rep = vertexInfo->GetRepresentation();
    bool isTagged = (rep == MachineRepresentation::Tagged);

    SpillLocations &slots = isTagged ? tagged_ : untagged_;
    uint32_t freeSlot = slots.top++;

    AllocatedState spillSlot(AllocatedState::STACK_SLOT, rep, freeSlot);
    vertexInfo->SetSpillSlot(spillSlot);
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "NEW SLOT: v" << vertex->GetId() << " assigned to " << spillSlot.Description();
#endif
    // to do: slot resuse
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
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << 'v' << vertex->GetId() << " spilled to fixed slot: " << location.Description();
#endif
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
            vertexInfo->SetResultAllocated(ForceAllocate(input, vertex));
            // Clear any hint that (probably) comes from this constraint.
            if (vertexInfo->HasHint()) {
                inputVertex->GetRegallocInfo()->ClearHint();
            }
            break;
        }

        case UnallocatedState::ExtendedPolicy::FIXED_FP_REGISTER: {
            ArkSteedDoubleRegister reg = ArkSteedDoubleRegister::FromCode(operand.GetFixedRegisterIndex());
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
    vertexInfo->SetNoSpill();

    auto &resultLocation = vertexInfo->GetResult();
    UnallocatedState &operand = static_cast<UnallocatedState &>(resultLocation.GetOperand());

    if (operand.GetBasicPolicy() == UnallocatedState::BasicPolicy::FIXED_SLOT) {
        AllocateFixedSlotResult(vertex);
        return;
    }

    AllocateByPolicy(vertex, operand);
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << 'v' << vertex->GetId()
                        << " allocated to: " << vertexInfo->GetResult().GetOperand().Description();
#endif

    // Immediately kill the register use if the vertex doesn't have a valid live-range.
    // to do: Remove once we can avoid allocating such registers.

    if (!vertexInfo->HasValidLiveRange() && resultLocation.IsAnyRegister()) {
        ASSERT(vertexInfo->HasRegisterResult());
        FreeRegistersUsedBy(vertex);
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << 'v' << vertex->GetId() << " is no more live.";
#endif
        ASSERT(!vertexInfo->HasRegisterResult());
        ASSERT(vertexInfo->HasNoMoreUses());
    }
}

void ArkSteedRegisterAllocator::TryAllocateToInput(PhiVertex *phi)
{
    // Try allocate phis to a register used by any of the inputs
    int inputCount = phi->GetInputCount();
    for (int i = 0; i < inputCount; i++) {
        Input input(phi, i);
        if (input.GetOperand().IsRegister()) {
            // We assume Phi vertices only point to tagged values, and so they use a general register
            ArkSteedRegister reg = input.GetLocation()->GetAssignedGeneralRegister();
            if (generalRegisters_.UnblockedFree().Has(reg)) {
                phi->GetRegallocInfo()->SetResultAllocated(ForceAllocate(reg, phi));
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

bool ArkSteedRegisterAllocator::IsCurrentVertexLastUse(ValueVertex * /*vertex*/)
{
    return true;  // Simplified for now
}

void ArkSteedRegisterAllocator::VerifyInputs(Vertex * /*vertex*/)  // to do:
{
#ifndef NDEBUG
#endif
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

void ArkSteedRegisterAllocator::InitializeBranchTargetPhis(int predecessorId, BB *target)
{
    ASSERT(target->GetBlockType() != BB::EDGE_SPLIT);

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
    ASSERT(target->GetBlockType() != BB::EDGE_SPLIT);
    ASSERT(target->HasState());

    RegisterMergeState &targetState = target->GetState()->RegisterState();
    ASSERT(!targetState.IsInitialized());

    bool hasChange = false;
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
        // to do: Remove the following debug output

        if (vertex != nullptr) {
            hasChange = true;
#ifndef NDEBUG
            LOG_COMPILER(DEBUG) << '\t' << (std::is_same_v<decltype(reg), ArkSteedDoubleRegister> ? "fr" : "r")
                                << static_cast<unsigned>(reg.Code()) << " <- v" << vertex->GetId();
#endif
        }
    };
    // to do: add loop optimize

#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Initializing register states -> Block #" << target->GetId();
    DumpCurrentRegisters("Before initializing");
#endif

    ForEachRegisterMergeState(targetState, init);
#ifndef NDEBUG
    if (!hasChange) {
        LOG_COMPILER(DEBUG) << "(Nothing to initialize)";
    }
#endif
    ASSERT(targetState.IsInitialized());
}

template <typename RegisterT>
void ArkSteedRegisterAllocator::CreateRegisterMerge(RegisterSnapshot<RegisterT> &registers, RegisterT reg,
                                                    RegisterState &state, ControlVertex *control, BB *target,
                                                    int predecessorId, int predecessorCount, ValueVertex *vertex,
                                                    ValueVertex *incoming, const AllocatedState &registerOperand)
{
    auto *chunk = graph_->GetChunk();
    size_t size = sizeof(RegisterMergeInfo) + predecessorCount * sizeof(AllocatedState);
    auto *newMerge = static_cast<RegisterMergeInfo *>(chunk->Allocate(size));
    newMerge->vertex = (vertex != nullptr) ? vertex : incoming;

    InstructionOperand infoSoFar;
    if (vertex == nullptr) {
        auto *incomingInfo = (incoming->GetRegallocInfo());
        infoSoFar = incomingInfo->GetSpillSlot();
    } else {
        infoSoFar = registerOperand;
    }

    for (int i = 0; i < predecessorCount; i++) {
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
                                                   int predecessorId, int predecessorCount)
{
    using RegType = decltype(reg);
    constexpr bool isDouble = std::is_same_v<RegType, ArkSteedDoubleRegister>;
    MachineRepresentation machRep = isDouble ? MachineRepresentation::Float64 : MachineRepresentation::Tagged;

    ValueVertex *vertex = nullptr;
    RegisterMergeInfo *mergeInfo = nullptr;
    state.LoadMergeState(&vertex, &mergeInfo);

    // Create register operand
    AllocatedState registerOperand(LocationState::LocationKind::REGISTER, machRep, reg.Code());

    ValueVertex *incoming = nullptr;
    ASSERT(registers.Blocked().IsEmpty());
    if (!registers.Free().Has(reg)) {
        incoming = registers.GetValue(reg);
        if (!IsLiveAtTarget(incoming, control, target)) {
            incoming = nullptr;
        }
    }

    if (incoming == vertex) {
        if (mergeInfo != nullptr) {
            mergeInfo->Operand(predecessorId) = registerOperand;
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
    if (target->GetBlockType() == BB::EDGE_SPLIT) {
        return InitializeEmptyBlockRegisterValues(control, target);
    }

    ASSERT(target->HasState());
    RegisterMergeState &targetState = target->GetState()->RegisterState();

    if (!targetState.IsInitialized()) {
        // This is the first block we're merging, initialize the values.
        return InitializeBranchTargetRegisterValues(control, target);
    }

    int predecessorCount = target->GetState()->PredecessorCount();

    auto merge = [&](auto &registers, auto reg, RegisterState &state) {
        MergeRegisterState(registers, reg, state, control, target, predecessorId, predecessorCount);
    };
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Merging register states -> Block #" << target->GetId() << " via predecessor #"
                        << predecessorId;
    DumpCurrentRegisters("Before merging");
#endif
    ForEachRegisterMergeState(targetState, merge);
}

void ArkSteedRegisterAllocator::InitializeConditionalBranchTarget(ControlVertex *controlVertex, BB *target)
{
    ASSERT(!target->HasPhi());

    if (target->HasState()) {
        // Not a fall-through branch, copy the state over.
        return InitializeBranchTargetRegisterValues(controlVertex, target);
    } else if (target->GetBlockType() == BB::EDGE_SPLIT) {
        return InitializeEmptyBlockRegisterValues(controlVertex, target);
    } else {
        // Fallthrough
        ASSERT(controlVertex->GetId() + 1 == target->GetFirstId());
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
    ASSERT(target->GetBlockType() == BB::EDGE_SPLIT);

    // Create a new merge point register state for the edge split block
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

    target->SetEdgeSplitBlockRegisterState(registerState);
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
    // to do: slot reuse
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

void ArkSteedRegisterAllocator::DebugDumpRegisterValues(RegisterMergeState &registerState, uint32_t predecessorCount)
{
    bool isEmpty = true;
    auto dumpFn = [&](auto &registers, auto reg, RegisterState &state) {
        using RegT = decltype(reg);
        constexpr std::string_view REG_NOTATION = std::is_same_v<RegT, ArkSteedDoubleRegister> ? "fr" : "r";

        ValueVertex *vertex = nullptr;
        RegisterMergeInfo *mergeInfo = nullptr;
        state.LoadMergeState(&vertex, &mergeInfo);

        std::ostringstream out;
        if (mergeInfo != nullptr) {
            isEmpty = false;
            out << REG_NOTATION << static_cast<unsigned>(reg.Code()) << ": [M] ";
            for (uint32_t i = 0; i < predecessorCount; i++) {
                InstructionOperand operand = mergeInfo->Operand(i);
                out << operand.Description() << ", ";
            }
            LOG_COMPILER(DEBUG) << out.str();
        } else if (vertex != nullptr) {
            isEmpty = false;
            out << REG_NOTATION << static_cast<unsigned>(reg.Code()) << ": [V] ";
            InstructionOperand operand = vertex->GetRegallocInfo()->GetResult().GetOperand();
            out << operand.Description();
            LOG_COMPILER(DEBUG) << out.str();
        }
    };
    ForEachRegisterMergeState(registerState, dumpFn);
    if (isEmpty) {
        LOG_COMPILER(DEBUG) << "(Empty register state)";
        return;
    }
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
}

void ArkSteedRegisterAllocator::TryAllocatePhisToInput(ChunkVector<PhiVertex *> &phis)
{
    for (auto &phi : phis) {
        if (!phi->GetRegallocInfo()->HasValidLiveRange()) {
            // Skip dead Phis
            LOG_COMPILER(DEBUG) << 'v' << phi->GetId() << " is dead Phi. Skipped.";
            continue;
        }
        TryAllocateToInput(phi);
#ifndef NDEBUG
        if (phi->GetRegallocInfo()->GetResult().GetOperand().IsAllocated()) {
            LOG_COMPILER(DEBUG) << "PHI v" << phi->GetId() << " allocated to one of input location.";
        }
#endif
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
                phiInfo->SetResultAllocated(AllocateRegister(phi));
            }
        } else {
            if (!generalRegisters_.UnblockedFreeIsEmpty()) {
                phiInfo->SetResultAllocated(AllocateRegister(phi));
            }
        }
#ifndef NDEBUG
        if (phi->GetRegallocInfo()->GetResult().GetOperand().IsAllocated()) {
            LOG_COMPILER(DEBUG) << "PHI v" << phi->GetId() << " allocated to register.";
        }
#endif
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
#ifndef NDEBUG
        if (phi->GetRegallocInfo()->GetResult().GetOperand().IsAllocated()) {
            LOG_COMPILER(DEBUG) << "PHI v" << phi->GetId() << " spilled.";
        }
#endif
    }
}

void ArkSteedRegisterAllocator::LogPhiAllocationResult(ChunkVector<PhiVertex *> &phis)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Phi allocation result:";
    for (const PhiVertex *phi : phis) {
        std::ostringstream out;
        InstructionOperand phiLoc = phi->GetRegallocInfo()->GetResult().GetOperand();
        out << "\tv" << phi->GetId() << '(' << phiLoc.Description() << ") = phi ";
        for (int i = 0, n = phi->GetInputCount(); i < n; i++) {
            const ValueVertex *input = phi->GetInput(i);
            InstructionOperand inputLoc = phi->GetRegallocInfo()->GetInputLocation(i)->GetOperand();
            if (i > 0) {
                out << ", ";
            }
            out << 'v' << input->GetId() << '(' << inputLoc.Description() << ')';
        }
        LOG_COMPILER(DEBUG) << out.str();
    }
#endif
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

    LogPhiAllocationResult(phis);

    generalRegisters_.ClearBlocked();
    doubleRegisters_.ClearBlocked();
}

void ArkSteedRegisterAllocator::DumpCurrentRegisters(std::string_view prompt)
{
    std::ostringstream out;
    if (!prompt.empty()) {
        out << '[' << prompt << "] ";
    }
    out << "Current GPR list: Free = " << generalRegisters_.Free().Dump()
        << ", Blocked = " << generalRegisters_.Blocked().Dump();
    LOG_COMPILER(DEBUG) << out.str();
    out.str("");
    out.clear();
    if (!prompt.empty()) {
        out << '[' << prompt << "] ";
    }
    out << "Current FPR list: Free = " << doubleRegisters_.Free().Dump()
        << ", Blocked = " << doubleRegisters_.Blocked().Dump();
    LOG_COMPILER(DEBUG) << out.str();
}

}  // namespace panda::ecmascript::arksteed
