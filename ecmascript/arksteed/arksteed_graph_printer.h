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

#ifndef ECMASCRIPT_ARKSTEED_GRAPH_PRINTER_H
#define ECMASCRIPT_ARKSTEED_GRAPH_PRINTER_H

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_processor.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

struct ArrowTarget {
    BB *target;
    bool isBackward;

    ArrowTarget() : target(nullptr), isBackward(false) {}
    ArrowTarget(BB *target, bool backward) : target(target), isBackward(backward) {}

    bool operator==(const BB *other) const
    {
        return target == other;
    }
    bool operator!=(const BB *other) const
    {
        return target != other;
    }
    operator bool() const
    {
        return target != nullptr;
    }
};

class GraphPrinter {
public:
    explicit GraphPrinter(Chunk *chunk, bool withColors = false);

    void PreProcessGraph(Graph *graph);
    void PostProcessGraph(Graph *graph);

    void PreProcessBlock(BB *block);
    void PostProcessBlock(BB *block);

    // For Vertex* (used by ControlVertex types converted to Vertex*)
    void ProcessVertex(ControlVertex *vertex, const ArkSteedState &state);
    void ProcessVertex(NonControlVertex *vertex, [[maybe_unused]] const ArkSteedState &state);

    void PostPhiProcessing() {}

private:
    // Fallthrough tracking for adjacent block jump arrow
    bool hasFallthrough_ = false;
    void PrintVertex(Vertex *vertex, ChunkSet<size_t> *arrowsStartingHere);
    std::string FormatControlVertexTargets(ControlVertex *vertex) const;
    std::string Indent() const;

    std::string FormatVertexAnnotations(Vertex *vertex) const;
    void PrintConstants(Graph *graph);
    bool HasConstantsToPrint(Graph *graph) const;
    void PrintInt32Constants(Graph *graph);
    void PrintIntPtrConstants(Graph *graph);
    void PrintFloat64Constants(Graph *graph);
    void PrintTaggedConstants(Graph *graph);
    std::string DecodeTaggedValue(uint64_t value) const;

    size_t AddTarget(BB *target, BB *currentBlock);
    bool AddTargetIfNotNext(BB *target, BB *nextBlock, BB *currentBlock, ChunkSet<size_t> *arrowsStarting);
    std::string PrintBlockArrows(BB *block);
    void PrintPredecessors(BB *block);
    std::string GetArrowColumn(ChunkSet<size_t> *arrowsStarting);

    Chunk *chunk_;
    bool withColors_;
    int totalVertices_;
    uint32_t totalBlocks_;
    BB *currentBlock_;
    int verticesInCurrentBlock_;

    ChunkVector<BB *> blockOrder_;
    ChunkMap<BB *, std::vector<BB *>> successorsMap_;
    ChunkSet<BB *> loopHeaders_;

    ChunkVector<ArrowTarget> targets_;
    ChunkSet<size_t> activeArrows_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_PRINTER_H
