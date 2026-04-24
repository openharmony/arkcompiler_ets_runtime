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

#ifndef ECMASCRIPT_ARKSTEED_CODEGEN_H
#define ECMASCRIPT_ARKSTEED_CODEGEN_H

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

class ArkSteedSafepointTableBuilder;
class GapMoveResolver;

class ArkSteedCodeGenerator {
public:
    ArkSteedCodeGenerator(ArkSteedAssembler *assembler, Graph *graph,
                          ArkSteedSafepointTableBuilder *safepointBuilder = nullptr)
        : assembler_(assembler),
          graph_(graph),
          safepointBuilder_(safepointBuilder),
          blockColorAssignment_(graph->GetChunk())
    {}

    void Generate();

private:
    static int32_t ComputeFrameSize(Graph *graph);

    void ProcessValueVertex(ValueVertex *valueVertex);
    void ProcessNonControlVertex(NonControlVertex *vertex);
    void ProcessControlVertex(ControlVertex *vertex);
    void DeconstructPhisInSuccessor(BB *successor, int predecessorId);
    void CollectPhiMoves(GapMoveResolver *resolver, BB *successor, int predecessorId,
                         ArkSteedRegList *registersSetByPhis, ArkDoubleRegList *doubleRegistersSetByPhis,
                         ChunkVector<std::pair<AllocatedState, ValueVertex *>> *constantMoves);
    void CollectRegisterStateMoves(GapMoveResolver *resolver, BB *successor, int predecessorId,
                                   const ArkSteedRegList &registersSetByPhis,
                                   const ArkDoubleRegList &doubleRegistersSetByPhis,
                                   ChunkVector<std::pair<AllocatedState, ValueVertex *>> *constantMoves);
    void LoadConstantToRegister(const ValueVertex *constVertex, ArkSteedRegister reg);
    void ExecuteConstantPhiMove(const AllocatedState &dest, ValueVertex *constVertex,
                                const ArkSteedRegister *scratchGPR = nullptr);
    void ExecuteGapMove(const InstructionOperand &dest, const InstructionOperand &src,
                        const ArkSteedRegister *scratchGPR = nullptr);
    void StoreStubStackArgument(const Vertex *callVertex, int paramIdx, ArkSteedAssembler::MemoryOperand destMem);

    int PrepareCommonStubStackArguments(const Vertex *callVertex, int argCount);
    int PrepareRuntimeStubStackArguments(const Vertex *callVertex, int argCount, int runtimeId);

    template <class VertexT>
    void VisitNonControlVertex(VertexT *vertex);

    template <class VertexT>
    void VisitControlVertex(VertexT *vertex);

    ArkSteedSafepointTableBuilder *GetSafepointBuilder() const
    {
        return safepointBuilder_;
    }

    // -------------------------------------------------------------------------
    // Comment recording helpers
    // -------------------------------------------------------------------------

    void RecordComment(const char *msg);
    void RecordBlockComment(BB *block);
    void RecordVertexComment(Vertex *vertex);
    void AppendVertexInputInfo(std::ostringstream *ss, Vertex *vertex);
    void AppendVertexSuccessorInfo(std::ostringstream *ss, Vertex *vertex);
    void RecordGapMoveComment(const InstructionOperand &src, const InstructionOperand &dest, PhiVertex *phi);
    void RecordSpillComment();

    // Block color management for IR visualization
    static constexpr const char *BLOCK_COLORS[] = {
        "\033[33m",  // Yellow
        "\033[36m",  // Cyan
        "\033[35m",  // Magenta
        "\033[32m",  // Green
        "\033[31m",  // Red
        "\033[34m",  // Blue
    };
    static constexpr int NUM_BLOCK_COLORS = 6;
    static constexpr const char *COLOR_RESET = "\033[0m";

    // Graph coloring for block colors - ensures adjacent blocks have different colors
    void ComputeBlockColors();
    void BuildBlockAdjacencyList(std::vector<std::vector<int>> *adjacentBlocks);
    void AssignBlockColors(const std::vector<std::vector<int>> &adjacentBlocks);
    int GetBlockColorIndex(int blockId) const;

    const char *GetBlockColor(int blockId) const
    {
        return BLOCK_COLORS[GetBlockColorIndex(blockId)];
    }

    void SetCurrentBlockColor(int blockId)
    {
        currentBlockColor_ = GetBlockColor(blockId);
    }

    const char *GetCurrentBlockColor() const
    {
        return currentBlockColor_;
    }

    ArkSteedAssembler *assembler_;
    Graph *graph_;
    ArkSteedSafepointTableBuilder *safepointBuilder_;
    const char *currentBlockColor_ = "";

    // Block color assignment for CFG coloring (only computed when comments enabled)
    ChunkVector<int> blockColorAssignment_;
    bool blockColorsComputed_ = false;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_CODEGEN_H
