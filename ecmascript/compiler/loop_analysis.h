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

#ifndef ECMASCRIPT_COMPILER_LOOP_ANALYSIS_H
#define ECMASCRIPT_COMPILER_LOOP_ANALYSIS_H

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
struct LoopInfo {
    LoopInfo(Chunk* chunk, GateRef head)
        : loopHead(head), loopBacks(chunk), loopExits(chunk), loopBodys(chunk) {}
    GateRef loopHead{Circuit::NullGate()};
    ChunkVector<GateRef> loopBacks;
    ChunkVector<GateRef> loopExits;
    ChunkVector<GateRef> loopBodys;
    size_t size{0};
    size_t maxDepth{0};
};
class LoopAnalysis {
public:
    LoopAnalysis(BytecodeCircuitBuilder* bcBuilder, Circuit *circuit, Chunk* chunk)
        : bcBuilder_(bcBuilder), acc_(circuit), chunk_(chunk), loopInfos_(chunk) {}
    ~LoopAnalysis() = default;
    void Run();
    void CollectLoopBody(LoopInfo* loopInfo);
    void LoopExitElimination();
    void PrintGraph();
    void PrintLoop(LoopInfo* loopInfo);
    const ChunkVector<LoopInfo*>& GetLoopTree() const
    {
        return loopInfos_;
    }

private:
    void UpdateLoopInfo(LoopInfo* loopInfo, GateRef gate, size_t dep);
    size_t ComputeLoopDepth(GateRef cur, GateRef nex, size_t curDep);
    void CollectUseGate(ChunkUnorderedMap<GateRef, size_t>& gateToDepth,
        ChunkQueue<GateRef>& firstList, ChunkQueue<GateRef>& secondList,
        LoopInfo* loopInfo, GateRef cur);

    BytecodeCircuitBuilder* bcBuilder_{nullptr};
    GateAccessor acc_;
    Chunk* chunk_{nullptr};
    ChunkVector<LoopInfo*> loopInfos_;
};

}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LOOP_ANALYSIS_H