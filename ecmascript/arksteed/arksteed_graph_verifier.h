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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_GRAPH_VERIFIER_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_GRAPH_VERIFIER_H

#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_processor.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

class ArkSteedGraphVerifier {
public:
    explicit ArkSteedGraphVerifier(Chunk *chunk) : seen_(chunk) {}

    void PreProcessGraph(Graph *graph)
    {
        seen_.resize(graph->MaxBlockId());
    }

    void PostProcessGraph(Graph *graph) {}

    void PostProcessBlock(BB *block) {}

    void PreProcessBlock(BB *block)
    {
        // Check Ids are unique.
        ASSERT(seen_[block->GetId()] == 0);
        seen_[block->GetId()] = 1;
    }

    void PostPhiProcessing() {}

    // ProcessVertex is called via GraphProcessor's opcode-based dispatch.
    // The VertexT template parameter is deduced as the concrete vertex type
    // (e.g., JumpVertex, CallVertex, PhiVertex), not the base ControlVertex
    // or NonControlVertex types. This ensures VerifyInputs() is resolved
    // correctly for each specific vertex type.
    //
    // VerifyInputs() implementation coverage:
    //
    // 1. FixedInputVertexMixin - Base template providing VerifyInputs() for all
    //    fixed-input vertices. Validates input count matches compile-time
    //    FIXED_INPUT_COUNT and checks input types if INPUT_TYPES is defined.
    //    All fixed-input vertices inherit this implementation via CRTP.
    //
    // 2. Variable-input vertices with custom VerifyInputs() implementations:
    //    - CallVertex: Validates GetInputCount() >= 1 and input count matches
    //      arg_count_ + 1 (function + arguments)
    //    - CallRuntimeVertex: Validates GetInputCount() >= 1 (minimum glue
    //      parameter required for runtime stubs)
    //    - CallCommonStubVertex: Validates GetInputCount() >= 1 (minimum glue
    //      parameter required for common stubs)
    //    - DeoptVertex: Validates GetInputCount() >= 3 (frame state requires
    //      function, context, and accumulator at minimum)
    //    - ReturnVertex: Validates GetInputCount() is 0 or 1 (RETURNUNDEFINED
    //      vs RETURN with value)
    //    - ThrowVertex: Validates GetInputCount() matches hasInput_ flag
    //      (0 for rethrow, 1 for throw with exception)
    //    - PhiVertex: Validates GetInputCount() > 0 and all inputs have
    //      consistent ValueRepresentation (type consistency at SSA merge)
    //
    // All vertices are covered because ALL_VERTEX_LIST includes every opcode,
    // ensuring the switch-case in GraphProcessor dispatches to all types.
    // Fixed-input vertices use the inherited implementation from FixedInputVertexMixin,
    // semantic requirements.
    template <typename VertexT>
    void ProcessVertex(VertexT *vertex, [[maybe_unused]] const ArkSteedState &state)
    {
        // Verify all inputs have valid opcodes
        for (int i = 0; i < vertex->GetInputCount(); i++) {
            ValueVertex *input = vertex->GetInput(i);
            ASSERT(input != nullptr);
            VertexOpcode opcode = input->GetOpcode();
            ASSERT(opcode >= FIRST_OPCODE);
            ASSERT(opcode <= LAST_OPCODE);
        }
        vertex->VerifyInputs();
    }

private:
    ChunkVector<uint8_t> seen_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_GRAPH_VERIFIER_H
