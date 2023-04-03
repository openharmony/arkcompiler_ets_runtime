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

#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class BytecodeCircuitBuilder;

class LoopAnalysis {
public:
    struct LoopInfo {
        LoopInfo(Chunk* chunk, size_t headId)
            : loopHead(headId), loopBody(chunk), loopExits(chunk) {}
        size_t loopHead;
        ChunkSet<size_t> loopBody;
        ChunkVector<size_t> loopExits;
    };

    LoopAnalysis(BytecodeCircuitBuilder *builder, Chunk* chunk)
        : builder_(builder), chunk_(chunk),
          visitState_(chunk), workList_(chunk),
          loopInfoVector_(chunk), dfsList_(chunk) {}
    ~LoopAnalysis() = default;
    void Run();

    ChunkVector<size_t>& DfsList()
    {
        return dfsList_;
    }

private:
    void CollectLoopBack();
    void CollectLoopBody(LoopInfo* loopInfo);
    void CollectLoopExits(LoopInfo* loopInfo);
    void CountLoopBackEdge(size_t fromId, size_t toId);

    BytecodeCircuitBuilder *builder_{nullptr};
    Chunk* chunk_{nullptr};
    ChunkVector<VisitState> visitState_;
    ChunkVector<size_t> workList_;
    ChunkVector<LoopInfo*> loopInfoVector_;
    ChunkVector<size_t> dfsList_;
    size_t loopCount_{0};
};

}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LOOP_ANALYSIS_H