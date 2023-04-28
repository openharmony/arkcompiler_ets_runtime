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

#ifndef ECMASCRIPT_COMPILER_LOOP_PEELING_H
#define ECMASCRIPT_COMPILER_LOOP_PEELING_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/loop_analysis.h"

namespace panda::ecmascript::kungfu {

class LoopPeeling {
public:
    LoopPeeling(BytecodeCircuitBuilder* bcBuilder, Circuit *circuit, Chunk* chunk, LoopInfo* loopInfo)
        : bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit),
          chunk_(chunk), loopInfo_(loopInfo), copies_(chunk_) {}
    ~LoopPeeling() = default;
    void Peel();

private:
    void SetCopy(GateRef gate);
    GateRef GetCopy(GateRef gate) const;
    GateRef TryGetCopy(GateRef gate) const;
    BytecodeCircuitBuilder* bcBuilder_{nullptr};
    Circuit* circuit_;
    GateAccessor acc_;
    Chunk* chunk_{nullptr};
    LoopInfo* loopInfo_{nullptr};
    ChunkMap<GateRef, GateRef> copies_;
};

}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LOOP_PEELING_H