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

#ifndef ECMASCRIPT_ARKSTEED_GRAPH_PROCESSOR_H
#define ECMASCRIPT_ARKSTEED_GRAPH_PROCESSOR_H

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/arksteed/arksteed_vertex.h"

namespace panda::ecmascript::arksteed {

// Forward declarations for dispatch
#define DEF_FORWARD_DECLARATION(type) class type##Vertex;
ALL_VERTEX_LIST(DEF_FORWARD_DECLARATION)
#undef DEF_FORWARD_DECLARATION

// Simple processing state for graph processors
class ArkSteedState {
public:
    explicit ArkSteedState(BB *block) : block_(block), nextBlock_(nullptr) {}

    BB *GetBlock() const
    {
        return block_;
    }

    BB *NextBlock() const
    {
        return nextBlock_;
    }
    void SetNextBlock(BB *next)
    {
        nextBlock_ = next;
    }

    bool IsFallThroughTo(BB *target) const
    {
        return target == nextBlock_;
    }

private:
    BB *block_;
    BB *nextBlock_;
};

// Base class for all graph processors
template <typename DerivedProcessor>
class GraphProcessor {
public:
    template <typename... Args>
    explicit GraphProcessor(Args &&...args) : derivedProcessor_(std::forward<Args>(args)...)
    {}
    ~GraphProcessor() = default;

    void Run(Graph *graph)
    {
        ASSERT(graph != nullptr);
        derivedProcessor_.PreProcessGraph(graph);

        // Process constant vertices before processing basic blocks
        ProcessConstants(graph);

        BB *nextBlock = nullptr;
        for (auto it = graph->begin(); it != graph->end(); ++it) {
            BB *block = *it;
            // Pre-compute next block for fallthrough detection
            auto nextIt = it;
            ++nextIt;
            nextBlock = (nextIt != graph->end()) ? *nextIt : nullptr;

            ArkSteedState state(block);
            state.SetNextBlock(nextBlock);

            derivedProcessor_.PreProcessBlock(block);

            if (block->HasPhi()) {
                for (PhiVertex *phi : block->GetPhis()) {
                    ProcessVertexByOpcode(phi, state);
                }
            }

            derivedProcessor_.PostPhiProcessing();

            // Process non-control vertices with opcode-based dispatch
            for (NonControlVertex *vertex : block->GetVertices()) {
                ProcessVertexByOpcode(vertex, state);
            }

            // Process control vertex if present with opcode-based dispatch
            if (ControlVertex *control = block->GetControlVertex()) {
                ProcessVertexByOpcode(control, state);
            }

            derivedProcessor_.PostProcessBlock(block);
        }

        derivedProcessor_.PostProcessGraph(graph);
    }

private:
    // Process all constant vertices before processing basic blocks
    void ProcessConstants(Graph *graph)
    {
        ArkSteedState state(nullptr);
        state.SetNextBlock(nullptr);

        // Process RootConstants
        for (const auto &[index, vertex] : graph->GetRootConstants()) {
            derivedProcessor_.ProcessVertex(vertex, state);
        }

        // Process Int32Constants
        for (const auto &[value, vertex] : graph->GetInt32Constants()) {
            derivedProcessor_.ProcessVertex(vertex, state);
        }

        // Process IntPtrConstants
        for (const auto &[value, vertex] : graph->GetIntPtrConstants()) {
            derivedProcessor_.ProcessVertex(vertex, state);
        }

        // Process Float64Constants
        for (const auto &[value, vertex] : graph->GetFloat64Constants()) {
            derivedProcessor_.ProcessVertex(vertex, state);
        }

        // Process TaggedConstants
        for (const auto &[value, vertex] : graph->GetTaggedConstants()) {
            derivedProcessor_.ProcessVertex(vertex, state);
        }
    }

    // Opcode-based dispatch to ensure template deduction uses concrete vertex types.
    // This allows VerifyInputs() to be resolved correctly for each vertex type,
    // rather than being resolved against the base ControlVertex/NonControlVertex types.
    void ProcessVertexByOpcode(Vertex *vertex, const ArkSteedState &state)
    {
        switch (vertex->GetOpcode()) {
            // clang-format off
#define PROCESS_VERTEX_CASE(type)                                                       \
            case VertexOpcode::type:                                                    \
                derivedProcessor_.ProcessVertex(vertex->Cast<type##Vertex>(), state);   \
                break;
            ALL_VERTEX_LIST(PROCESS_VERTEX_CASE)
#undef PROCESS_VERTEX_CASE
            // clang-format on
            default:
                UNREACHABLE();
                break;
        }
    }

private:
    DerivedProcessor derivedProcessor_;
};

// ============================================================================
// Example processors (for reference only - can be removed)
// ============================================================================

// Example: A simple debug printer processor
class DebugPrintProcessor {
public:
    void PreProcessGraph(Graph *graph)
    {
        std::cout << "=== Processing Graph ===" << std::endl;
    }

    void PreProcessBlock(BB *block)
    {
        std::cout << "Block " << block->GetId() << ":" << std::endl;
    }

    void ProcessVertex(ControlVertex *vertex, [[maybe_unused]] const ArkSteedState &state)
    {
        std::cout << "  Control Vertex (opcode: " << OpcodeToString(vertex->GetOpcode()) << ")" << std::endl;
    }

    void PostPhiProcessing() {}

    void ProcessVertex(NonControlVertex *vertex, [[maybe_unused]] const ArkSteedState &state)
    {
        std::cout << "  NonControl Vertex (opcode: " << OpcodeToString(vertex->GetOpcode()) << ")" << std::endl;
    }

    void PostProcessBlock(BB *block) {}

    void PostProcessGraph(Graph *graph)
    {
        std::cout << "=== Finished Processing Graph ===" << std::endl;
    }
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_PROCESSOR_H
