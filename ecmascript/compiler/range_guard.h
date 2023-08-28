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

#ifndef ECMASCRIPT_COMPILER_RANGE_GUARD_H
#define ECMASCRIPT_COMPILER_RANGE_GUARD_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/base/depend_chain_helper.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/compiler/number_gate_info.h"

namespace panda::ecmascript::kungfu {
class DependChains;
class RangeGuard : public PassVisitor {
public:
    RangeGuard(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit),
        builder_(circuit), dependChains_(chunk) {}

    ~RangeGuard() = default;

    void Initialize() override;
    GateRef VisitGate(GateRef gate) override;
    bool CheckInputSource(GateRef lhs, GateRef rhs);
    uint32_t CheckIndexCheckLengthInput(GateRef lhs, GateRef rhs);
    uint32_t CheckIndexCheckIndexInput(GateRef lhs, GateRef rhs);
private:

    DependChains* GetDependChain(GateRef dependIn)
    {
        size_t idx = acc_.GetId(dependIn);
        ASSERT(idx <= circuit_->GetMaxGateId());
        return dependChains_[idx];
    }

    GateRef VisitDependEntry(GateRef gate);
    GateRef UpdateDependChain(GateRef gate, DependChains* dependInfo);
    GateRef TryApplyRangeGuardForLength(DependChains* dependInfo, GateRef gate, GateRef input);
    GateRef TryApplyRangeGuardForIndex(DependChains* dependInfo, GateRef gate, GateRef input);
    GateRef TryApplyRangeGuardGate(GateRef gate);
    GateRef TraverseOthers(GateRef gate);
    GateRef TraverseDependSelector(GateRef gate);

    Circuit* circuit_;
    CircuitBuilder builder_;
    ChunkVector<DependChains*> dependChains_;

    friend class RangeInfo;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_RANGE_GUARD_H