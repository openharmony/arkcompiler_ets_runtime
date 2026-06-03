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

#include "ecmascript/compiler/branch_profile.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include <vector>

namespace panda::ecmascript::kungfu {

GateRef BranchProfile::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::DEPEND_RELAY:
            AddBranchProfiling(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void BranchProfile::AddBranchProfiling(GateRef gate)
{
    auto stateIn = acc_.GetState(gate);
    auto stateOp = acc_.GetOpCode(stateIn);
    if (stateOp != OpCode::IF_TRUE && stateOp != OpCode::IF_FALSE) {
        return;
    }
    auto glue = acc_.GetGlueFromArgList();
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    // target & glue & reserved frameState
    auto meta = circuit_->RuntimeCall(3);
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    auto target = circuit_->GetConstantGateWithoutCache(MachineType::ARCH,
        RTSTUB_ID(RuntimeBranchProfile), GateType::NJSValue());
    auto reserved = circuit_->GetConstantGate(MachineType::ARCH, 0, GateType::NJSValue());
    std::vector<GateRef> inList = {gate, target, glue, reserved};
    GateRef callRuntime = circuit_->NewGate(meta, machineType, inList.size(), inList.data(), type, "BranchProfile");
    auto uses = acc_.Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt ++) {
        if (acc_.IsDependIn(useIt) && callRuntime != *useIt) {
            acc_.ReplaceIn(useIt, callRuntime);
            break;
        }
    }
}

}