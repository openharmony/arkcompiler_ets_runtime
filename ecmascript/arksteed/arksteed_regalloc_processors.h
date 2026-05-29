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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_PROCESSORS_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_PROCESSORS_H

#include "ecmascript/arksteed/arksteed_framestate.h"
#include "ecmascript/arksteed/arksteed_graph_processor.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_regalloc.h"
#include "ecmascript/arksteed/arksteed_regalloc_vertex_info.h"
#include "ecmascript/arksteed/arksteed_vertex.h"

namespace panda::ecmascript::arksteed {

// =============================================================================
// VertexInfoAllocateProcessor
// Allocates RegallocValueVertexInfo for each vertex in the graph
// This must run before any other register allocation processors
// =============================================================================

class VertexInfoAllocateProcessor {
public:
    void PreProcessGraph(Graph *graph)
    {
        chunk_ = graph->GetChunk();
    }

    void PreProcessBlock(BB *block) {}
    void PostProcessBlock(BB *block) {}
    void PostProcessGraph(Graph *graph) {}
    void PostPhiProcessing() {}

    void ProcessVertex(ValueVertex *vertex, const ArkSteedState &state)
    {
        vertex->SetRegallocInfo(
            chunk_->New<RegallocValueVertexInfo>(chunk_, vertex->GetInputCount(), vertex->GetMachineRepresentation()));
    }

    void ProcessVertex(Vertex *vertex, const ArkSteedState &state)
    {
        vertex->SetRegallocInfo(chunk_->New<RegallocVertexInfo>(chunk_, vertex->GetInputCount()));
    }

private:
    Chunk *chunk_ = nullptr;
};

// =============================================================================
// ValueLocationConstraintProcessor
// Calls SetValueLocationConstraints() on each vertex to define
// input/output location requirements for register allocation
// =============================================================================

class ValueLocationConstraintProcessor {
public:
    void PreProcessGraph(Graph *graph) {}
    void PreProcessBlock(BB *block) {}
    void PostProcessBlock(BB *block) {}
    void PostProcessGraph(Graph *graph) {}
    void PostPhiProcessing() {}

#define DEF_PROCESS_VERTEX(NAME)                                         \
    void ProcessVertex(NAME##Vertex *vertex, const ArkSteedState &state) \
    {                                                                    \
        vertex->SetValueLocationConstraints();                           \
        return;                                                          \
    }
    ALL_VERTEX_LIST(DEF_PROCESS_VERTEX)
#undef DEF_PROCESS_VERTEX
};

// =============================================================================
// MaxCallDepthProcessor
// Computes the maximum number of stack arguments passed to calls
// =============================================================================

class MaxCallDepthProcessor {
public:
    void PreProcessGraph(Graph *graph) {}
    void PreProcessBlock(BB *block) {}
    void PostProcessBlock(BB *block) {}
    void PostProcessGraph(Graph *graph)
    {
        graph->SetMaxCallStackArgs(maxCallStackArgs_);
    }
    void PostPhiProcessing() {}

    template <typename T>
    void ProcessVertex(T *vertex, const ArkSteedState &state)
    {
        constexpr bool isCall = T::PROPERTIES.IsCall();
        constexpr bool needsRegSnapshot = T::PROPERTIES.NeedsRegisterSnapshot();
        if constexpr (isCall || needsRegSnapshot) {
            int vertexStackArgs = static_cast<int>(vertex->GetInputCount());
            if constexpr (needsRegSnapshot) {
                // Pessimistically assume that we'll push all registers in deferred calls.
                vertexStackArgs += ALLOCATABLE_GENERAL_REGISTER_COUNT + ALLOCATABLE_DOUBLE_REGISTER_COUNT;
            }
            maxCallStackArgs_ = std::max(maxCallStackArgs_, vertexStackArgs);
        }
    }

private:
    int maxCallStackArgs_{0};
    static constexpr int ALLOCATABLE_GENERAL_REGISTER_COUNT = 32; // 32: number of allocatable general-purpose registers
    static constexpr int ALLOCATABLE_DOUBLE_REGISTER_COUNT = 32;  // 32: number of allocatable double (FP) registers
};

// =============================================================================
// LivenessProcessor
// Tracks live ranges and next-use information for register allocation
// Handles loop-external vertex lifetime extension
//
// ArkSteed uses Edge Splitting optimization:
// - For conditional branches (BranchIfTrue): target block cannot have Phi vertices, so no special handling is needed
// - For unconditional jumps (Jump/JumpLoop): target block can have Phi vertices
// =============================================================================

// to do: loop optimize && handle deoptimization
class LivenessProcessor {
public:
    void PreProcessGraph(Graph *graph) {}

    void PreProcessBlock(BB *block)
    {
        // Check if this block is a loop header
        if (block->IsLoopHeader()) {
            LoopUsedVertices loopVertex;
            loopVertex.header = block;
            loopUsedVertices_.push_back(loopVertex);
        }
    }

    void PostProcessBlock(BB *block) {}

    void PostProcessGraph(Graph *graph)
    {
        // Verify all loops have been processed
        ASSERT(loopUsedVertices_.empty());
    }

    void PostPhiProcessing() {}

    // Generic template for all vertex types
    template <typename T>
    void ProcessVertex(T *vertex, const ArkSteedState &state)
    {
        vertex->GetRegallocInfo()->SetId(nextVertexId_++);
        MarkInputUses(vertex, state);
    }

private:
    struct LoopUsedVertices {
        std::vector<ValueVertex *> usedVertices;  // ValueVertex* from outside the loop
        BB *header;
    };

    LoopUsedVertices *GetCurrentLoopUsedVertices()
    {
        if (loopUsedVertices_.empty()) {
            return nullptr;
        }
        return &loopUsedVertices_.back();
    }

    template <typename T>
    void MarkInputUses(T *vertex, const ArkSteedState &state)
    {
        LoopUsedVertices *loopUsedVertices = GetCurrentLoopUsedVertices();
        vertex->ForAllInputsInRegallocAssignmentOrder([&](const Input &input) {
            MarkUse(static_cast<ValueVertex *>(input.vertex()), vertex->GetId(), input.GetLocation(), loopUsedVertices);
        });
    }

    // Specialization for PhiVertex - skip here, will be handled by control vertices
    void MarkInputUses(PhiVertex *vertex, const ArkSteedState &state) {}

    // Specialization for JumpVertex - handle phi inputs for target block
    void MarkInputUses(JumpVertex *vertex, const ArkSteedState &state)
    {
        BB *target = vertex->Target();
        uint32_t use = vertex->GetId();
        int predecessorIdx = state.GetBlock()->GetPredecessorId();
        LoopUsedVertices *loopUsedVertices = GetCurrentLoopUsedVertices();

        if (!target->HasPhi()) {
            return;
        }

        const auto &phis = target->GetPhis();
        for (PhiVertex *phi : phis) {
            const ValueVertex *input = phi->GetPredecessor(predecessorIdx);
            InputLocation *location = phi->GetInputLocation(predecessorIdx);
            MarkUse(const_cast<ValueVertex *>(input), use, location, loopUsedVertices);
        }
    }

    // Specialization for JumpLoopVertex - handle phi inputs for loop header block and propagate loop-external vertices
    void MarkInputUses(JumpLoopVertex *vertex, const ArkSteedState &state)
    {
        BB *target = vertex->Target();
        uint32_t use = vertex->GetId();
        int predecessorIdx = state.GetBlock()->GetPredecessorId();

        ASSERT(!loopUsedVertices_.empty());
        LoopUsedVertices loopUsedVertices = std::move(loopUsedVertices_.back());
        loopUsedVertices_.pop_back();
        ASSERT(loopUsedVertices.header == target);

        LoopUsedVertices *outerLoopUsedVertices = GetCurrentLoopUsedVertices();

        // Handle phi inputs from loop header
        if (target->HasPhi()) {
            const auto &phis = target->GetPhis();
            for (PhiVertex *phi : phis) {
                const ValueVertex *input = phi->GetPredecessor(predecessorIdx);
                InputLocation *location = phi->GetInputLocation(predecessorIdx);
                MarkUse(const_cast<ValueVertex *>(input), use, location, outerLoopUsedVertices);
            }
        }

        // Propagate loop-external vertices to outer loop if exists
        // This extends their lifetime across the loop back edge
        if (!loopUsedVertices.usedVertices.empty()) {
            JumpLoopVertex::UsedVerticesType usedVertexInputs;
            usedVertexInputs.reserve(loopUsedVertices.usedVertices.size());
            for (size_t i = 0; i < loopUsedVertices.usedVertices.size(); i++) {
                usedVertexInputs.emplace_back(loopUsedVertices.usedVertices[i], InputLocation());
                MarkUse(loopUsedVertices.usedVertices[i], use, &usedVertexInputs[i].second, outerLoopUsedVertices);
            }
            vertex->SetUsedVertices(std::move(usedVertexInputs));
        }
    }

    void MarkUse(ValueVertex *vertex, uint32_t useId, InputLocation *input, LoopUsedVertices *loopUsedVertices)
    {
        ASSERT(vertex->GetRegallocInfo() != nullptr);

        // record use
        vertex->GetRegallocInfo()->RecordUse(useId, input);

        // If we are in a loop, check if the incoming vertex is from outside the loop,
        // and make sure to extend its lifetime to the loop end if yes.
        if (loopUsedVertices != nullptr) {
            if (vertex->GetId() < loopUsedVertices->header->GetFirstId()) {
                loopUsedVertices->usedVertices.push_back(vertex);
            }
        }
    }

    std::vector<LoopUsedVertices> loopUsedVertices_;
    uint32_t nextVertexId_{0};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_REGALLOC_PROCESSORS_H
