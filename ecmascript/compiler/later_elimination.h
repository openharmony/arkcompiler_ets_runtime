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

#ifndef ECMASCRIPT_COMPILER_LATER_ELIMINATION_H
#define ECMASCRIPT_COMPILER_LATER_ELIMINATION_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/base/depend_chain_helper.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class DependChains;
class LaterElimination : public PassVisitor {
public:
    LaterElimination(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor), dependChains_(chunk) {}

    ~LaterElimination() = default;

    void Initialize() override;
    GateRef VisitGate(GateRef gate) override;
    bool CheckReplacement(GateRef lhs, GateRef rhs);
private:

    DependChains* GetDependChain(GateRef dependIn)
    {
        size_t idx = acc_.GetId(dependIn);
        ASSERT(idx <= circuit_->GetMaxGateId());
        return dependChains_[idx];
    }

    GateRef LookupNode(DependChains* dependChain, GateRef gate)
    {
        for (auto iter = dependChain->begin(); iter != dependChain->end(); ++iter) {
            GateRef curGate = iter.GetCurrentGate();
            if (CheckReplacement(curGate, gate)) {
                return curGate;
            }
        }
        return Circuit::NullGate();
    }

    GateRef VisitDependEntry(GateRef gate);
    GateRef UpdateDependChain(GateRef gate, DependChains* dependInfo);
    GateRef TryEliminateGate(GateRef gate);
    GateRef TryEliminateOther(GateRef gate);
    GateRef TryEliminateDependSelector(GateRef gate);

    ChunkVector<DependChains*> dependChains_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LATER_ELIMINATION_H