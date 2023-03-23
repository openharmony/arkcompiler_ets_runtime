/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_GRAPH_VISITOR_H
#define ECMASCRIPT_COMPILER_GRAPH_VISITOR_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {

class GraphVisitor {
public:
    GraphVisitor(Circuit *circuit, Chunk* chunk)
        : circuit_(circuit), acc_(circuit),
        chunk_(chunk), workList_(chunk), changedList_(chunk) {}

    virtual ~GraphVisitor() = default;

    void VisitGraph();
    void ReVisitGate(GateRef gate);

    virtual GateRef VisitGate(GateRef gate) = 0;
protected:
    void ReplaceGate(GateRef gate, GateRef replacement);
    void ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement);
    void VisitTopGate(Edge& current);

    void PushGate(GateRef gate, size_t index)
    {
        workList_.push_back(Edge{gate, index});
        acc_.SetMark(gate, MarkCode::VISITED);
    }

    void PushChangedGate(GateRef gate)
    {
        changedList_.push_back(gate);
        acc_.SetMark(gate, MarkCode::PREVISIT);
    }

    void PopGate(GateRef gate)
    {
        workList_.pop_back();
        acc_.SetMark(gate, MarkCode::FINISHED);
    }

    Chunk *GetChunk() const
    {
        return chunk_;
    }
    void PrintStack();

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    Chunk* chunk_ {nullptr};
    ChunkDeque<Edge> workList_;
    ChunkDeque<GateRef> changedList_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_GRAPH_VISITOR_H