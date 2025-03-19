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

#ifndef ECMASCRIPT_COMPILER_GRAPH_EDITOR_H
#define ECMASCRIPT_COMPILER_GRAPH_EDITOR_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {

class GraphEditor {
public:
    GraphEditor(Circuit *circuit)
        : circuit_(circuit), acc_(circuit),
        chunk_(circuit->chunk()), workList_(circuit->chunk()) {}

    ~GraphEditor() = default;

    static void RemoveDeadState(Circuit* circuit, GateRef gate);
    static void EliminateRedundantPhi(Circuit* circuit, bool enableLog, const std::string& methodName);
private:
    void ReplaceGate(GateRef gate);
    void RemoveGate();
    void PropagateGate(const Edge& edge);
    void PropagateMerge(const Edge& edge);
    void EliminatePhi();
    bool FrameValueUsedInCFGTailoring(GateRef gate);

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    Chunk* chunk_ {nullptr};
    ChunkVector<Edge> workList_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_GRAPH_EDITOR_H
