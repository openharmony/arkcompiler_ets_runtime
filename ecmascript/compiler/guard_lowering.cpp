/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/guard_lowering.h"

namespace panda::ecmascript::kungfu {
void GuardLowering::LowerGuard(GateRef gate)
{
    GateRef checkGate = acc_.GetIn(gate, 1);
    acc_.ReplaceValueIn(gate, builder_.False(), 0);
    std::vector<GateRef> outs;
    acc_.GetOutVector(gate, outs); // get guard next gate
    ASSERT(outs.size() == 1);
    GateRef next = outs[0];
    GateRef prev = acc_.GetState(next);
    GateRef ifBranch = builder_.Branch(prev, checkGate);
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    acc_.ReplaceStateIn(next, ifTrue);
    GateRef dep = acc_.GetDep(gate);
    acc_.ReplaceDependIn(next, dep);
    GateRef relay = builder_.DependRelay(ifFalse, dep);
    acc_.ReplaceDependIn(gate, relay);
    builder_.Return(ifFalse, gate, gate);
}

void GuardLowering::Run()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::GUARD) {
            LowerGuard(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After guard lowering "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGates(*bcBuilder_);
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}
}  // namespace panda::ecmascript::kungfu