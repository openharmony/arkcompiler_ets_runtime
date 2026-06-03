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

#include "ecmascript/compiler/branch_elimination.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/constant.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "macros.h"
#include <string>
#include <vector>

namespace panda::ecmascript::kungfu {

GateRef BranchElimination::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::IF_BRANCH:
            AddTotalBranchCount();
            return TryEliminateIfBranch(gate);
        default:
            return Circuit::NullGate();
    }
}

GateRef BranchElimination::TryEliminateIfBranch(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::IF_BRANCH);
    auto cond = acc_.GetValueIn(gate, 0);
    if (!acc_.IsConstant(cond)) {
        return Circuit::NullGate();
    }
    ASSERT(IsBoolType(cond));
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    auto cond_val = GetConstant(acc_, cond).GetBool();
    GateRef bTrue = (acc_.GetOpCode(outs[0]) == OpCode::IF_TRUE) ? outs[0] : outs[1];
    GateRef bFalse = (acc_.GetOpCode(outs[0]) == OpCode::IF_FALSE) ? outs[0] : outs[1];
    auto state = acc_.GetState(gate, 0);
    auto block = circuit_->NewGate(circuit_->OrdinaryBlock(), {state});
    auto GetDepend = [&](GateRef gate) {
        GateRef depend = Circuit::NullGate();
        if (acc_.GetDependCount(gate) > 0) {
            depend = acc_.GetDep(gate);
        }
        return depend;
    };

    if (cond_val) {
        GateRef depend = GetDepend(bTrue);
        ReplaceGate(bTrue, StateDepend{block, depend}, block);
    } else {
        GateRef depend = GetDepend(bFalse);
        ReplaceGate(bFalse, StateDepend{block, depend}, block);
    }
    AddEliminatingCount();
    return circuit_->DeadGate();
}

void BranchElimination::Print() const
{
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                        << "===================="
                        << " After branch elimination "
                        << "===================="
                        << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO)
            << "\033[34m"
            << "========================= End =========================="
            << "\033[0m";
        double rate = GetEliminatingCount() * 1.0 / GetTotalBranchCount();
        LOG_COMPILER(INFO) << "Branch Eliminating Rate: "<< std::to_string(rate)<<"("
                        << GetEliminatingCount()<< " "<< " / " << GetTotalBranchCount()<< ")";
    }
}
} // namespace panda::ecmascript::kungfu
