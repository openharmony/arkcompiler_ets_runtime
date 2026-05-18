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

#include "ecmascript/arksteed/arksteed_framestate.h"

#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

void InterpreterFrameState::CopyFrom(const MergePointFrameState &mergeState)
{
    ASSERT(NumLocalVRegs() == mergeState.NumLocalVRegs());
    ASSERT(NumParamVRegs() == mergeState.NumParamVRegs());
    mergeState.FrameState().ForEach([this](const ValueVertex *vertex, VirtualRegister reg) {
        Set(reg, const_cast<ValueVertex *>(vertex));
    });
}

MergePointFrameState::MergePointFrameState(uint32_t bcIndex, uint32_t predecessorCount, BasicBlockType type,
                                           const LivenessBitSet *liveness, Chunk *chunk)
    : bcIndex_(bcIndex),
      predecessorsSoFar_(0),
      bitfield_(BasicBlockTypeBits::Encode(type)),
      predecessors_(predecessorCount, nullptr, chunk),
      phis_(chunk),
      frameState_(liveness, chunk),
      chunk_(chunk)
{}

// static
MergePointFrameState *MergePointFrameState::New(uint32_t bcIndex, uint32_t predecessorCount,
                                                const LivenessBitSet *liveness, Chunk *chunk)
{
    ASSERT(liveness != nullptr);
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "MergePointFrameState::New("
                        << "bcIndex = " << bcIndex << ", numLocal = " << liveness->NumLocalVRegs()
                        << ", numParams = " << liveness->NumParamVRegs() << ", predecessorCount = " << predecessorCount
                        << ")";
#endif
    MergePointFrameState *mergeState =
        chunk->New<MergePointFrameState>(bcIndex, predecessorCount, BasicBlockType::DEFAULT, liveness, chunk);
    return mergeState;
}

// static
MergePointFrameState *MergePointFrameState::NewForLoop(uint32_t bcIndex, uint32_t predecessorCount,
                                                       const LivenessBitSet *liveness, const LoopInfo *loopInfo,
                                                       Chunk *chunk)
{
    ASSERT(liveness != nullptr);
    ASSERT(loopInfo != nullptr);
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "MergePointFrameState::NewForLoop("
                        << "bcIndex = " << bcIndex << ", numLocal = " << liveness->NumLocalVRegs()
                        << ", numParams = " << liveness->NumParamVRegs() << ", predecessorCount = " << predecessorCount
                        << ")";
#endif
    MergePointFrameState *state =
        chunk->New<MergePointFrameState>(bcIndex, predecessorCount, BasicBlockType::LOOP_HEADER, liveness, chunk);

    state->loopInfo_ = loopInfo;
    state->loopEffects_ = nullptr;

    state->FrameState().ForEach([loopInfo, state](ValueVertex **value, VirtualRegister reg) {
        *value = loopInfo->IsDefinedInThisLoop(reg) ? state->NewLoopPhi(reg) : nullptr;
    });
    return state;
}

// static
MergePointFrameState *MergePointFrameState::NewForCatchBlock(uint32_t bcIndex, const LivenessBitSet *liveness,
                                                             Chunk *chunk)
{
    ASSERT(liveness != nullptr);
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "MergePointFrameState::NewForCatchBlock("
                        << "bcIndex = " << bcIndex << ", numLocal = " << liveness->NumLocalVRegs()
                        << ", numParams = " << liveness->NumParamVRegs() << ")";
#endif
    BasicBlockType type = BasicBlockType::EXCEPTION_HANDLER_START;
    // 0 : No predecessor initially for catch blocks. Predecessors are added dynamically later
    MergePointFrameState *state = chunk->New<MergePointFrameState>(bcIndex, 0, type, liveness, chunk);

    // If accumulator is live, create an exception phi for it
    if (state->FrameState().AccIsLive()) {
        auto *phi = state->NewExceptionPhi(state->VRegOfAcc());
        state->FrameState().SetAcc(phi);
    }
    return state;
}

void MergePointFrameState::MergeFrom(InterpreterFrameState &srcState, BB *srcPredecessor)
{
    ASSERT(srcPredecessor != nullptr);
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "MergePointFrameState::MergeFrom(): "
                        << "dest bytecodeIndex = " << bcIndex_ << ", perdecessorsSoFar_ = " << predecessorsSoFar_;
#endif
    if (IsExceptionHandler()) {
        // For exception handlers: predecessors are added dynamically
        predecessors_.push_back(srcPredecessor);
    } else {
        // For others: the number of predecessors is known initally.
        predecessors_[predecessorsSoFar_] = srcPredecessor;
    }
    MergePhisFrom(srcState);
    predecessorsSoFar_++;
}

void MergePointFrameState::ReducePredecessorCount(uint32_t num)
{
    ASSERT(num <= PredecessorCount());
    ASSERT(predecessorsSoFar_ <= PredecessorCount() - num);

    if (num == 0) {
        return;
    }

    predecessors_.resize(predecessors_.size() - num);
    for (PhiVertex *phi : phis_) {
        phi->ReduceInputCount(num);
    }
}

// destValue is one of this->FrameState()
ValueVertex *MergePointFrameState::MergeValue(VirtualRegister reg, ValueVertex *destValue, ValueVertex *srcValue)
{
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();

    // Input from the first predecessor
    if (destValue == nullptr) {
#ifndef NDEBUG
        if (labeller != nullptr) {
            LOG_COMPILER(DEBUG) << "\tMergeValue() for virtual register " << VRegDisplayString(reg)
                                << ": Gets the first input vertex " << labeller->GetVertexInputLabel(srcValue);
        }
#endif
        return srcValue;
    }

    // Check if destValue is already a phi belonging to this merge point
    PhiVertex *phi = destValue->TryCast<PhiVertex>();
    if (phi != nullptr && phi->GetMergePointState() == this) {
        return MergeExistingPhiInput(reg, phi, srcValue, labeller);
    }

    // If values are the same, nothing to merge
    if (destValue == srcValue) {
#ifndef NDEBUG
        if (labeller != nullptr) {
            LOG_COMPILER(DEBUG) << "\tMergeValue() for virtual register " << VRegDisplayString(reg)
                                << ": Meets the same input vertex " << labeller->GetVertexInputLabel(srcValue)
                                << ". No need to merge.";
        }
#endif
        return destValue;
    }

    return CreateMergePhi(reg, destValue, srcValue, labeller);
}

ValueVertex *MergePointFrameState::MergeExistingPhiInput(VirtualRegister reg, PhiVertex *phi, ValueVertex *srcValue,
                                                         ArkSteedGraphLabeller *labeller)
{
    ASSERT(phi->GetOwner() == reg);
    // Add input to existing phi
    if (IsExceptionHandler()) {
        LOG_COMPILER(WARN) << "to do: For exception handler: dynamic-input vertex is not implemented. " << "(bytecode #"
                           << bcIndex_ << ", vreg #" << reg.GetId() << ')';
    } else {
#ifndef NDEBUG
        if (labeller != nullptr) {
            LOG_COMPILER(DEBUG) << "\tMergeValue() for virtual register " << VRegDisplayString(reg)
                                << ": Write input vertex " << labeller->GetVertexInputLabel(srcValue) << " as input #"
                                << predecessorsSoFar_ << " of PHI vertex " << labeller->GetVertexInputLabel(phi);
        }
#endif
        phi->SetInput(predecessorsSoFar_, srcValue);
    }
    return phi;
}

PhiVertex *MergePointFrameState::CreateMergePhi(VirtualRegister reg, ValueVertex *destValue, ValueVertex *srcValue,
                                                ArkSteedGraphLabeller *labeller)
{
    PhiVertex *phi = Vertex::New<PhiVertex>(chunk_, PredecessorCount(), this, reg);
    // Initialize all inputs to the current value for predecessors seen so far
    for (uint32_t i = 0; i < predecessorsSoFar_; i++) {
        phi->SetInput(i, destValue);
    }
    // Set the new input for the current predecessor
    phi->SetInput(predecessorsSoFar_, srcValue);
    phis_.push_back(phi);

    // Register the phi with the graph labeller if one exists
    if (labeller != nullptr) {
        labeller->RegisterVertex(phi);
    }
#ifndef NDEBUG
    if (labeller != nullptr) {
        LOG_COMPILER(DEBUG) << "\tMergeValue() for virtual register " << VRegDisplayString(reg)
                            << ": Creates PHI input vertex " << labeller->GetVertexInputLabel(phi)
                            << " for previous input(s) " << labeller->GetVertexInputLabel(destValue)
                            << " and new input " << labeller->GetVertexInputLabel(srcValue);
    }
#endif

    return phi;
}

void MergePointFrameState::MergePhisFrom(InterpreterFrameState &srcState)
{
    ASSERT(srcState.NumLocalVRegs() == this->NumLocalVRegs() && srcState.NumParamVRegs() == this->NumParamVRegs());
    // Merge each value from the source state into the current state
    FrameState().ForEach(
        [&](ValueVertex **value, VirtualRegister reg) { *value = MergeValue(reg, *value, srcState.Get(reg)); });
}

ValueVertex *MergePointFrameState::NewLoopPhi(VirtualRegister reg)
{
    PhiVertex *result = Vertex::New<PhiVertex>(chunk_, PredecessorCount(), this, reg);
    // Initialize all inputs to nullptr
    for (uint32_t i = 0, n = PredecessorCount(); i < n; i++) {
        result->SetInput(i, nullptr);
    }
    phis_.push_back(result);

    // Register the phi with the graph labeller if one exists
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    if (labeller != nullptr) {
        labeller->RegisterVertex(result);
    }

    return result;
}

ValueVertex *MergePointFrameState::NewExceptionPhi(VirtualRegister reg)
{
    ASSERT(PredecessorCount() == 0 && "Catch block has no predecessors initially");
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "\tEXCEPTION PHI created for virtual register " << VRegDisplayString(reg);
#endif
    // Inputs of catch-Phi are added later during MergeThrow().
    PhiVertex *result = Vertex::New<PhiVertex>(chunk_, 0, this, reg);  // 0 : No input initially.
    phis_.push_back(result);

    // Register the phi with the graph labeller if one exists
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    if (labeller != nullptr) {
        labeller->RegisterVertex(result);
    }

    return result;
}

}  // namespace panda::ecmascript::arksteed
