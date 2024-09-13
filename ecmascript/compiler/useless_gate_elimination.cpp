/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/useless_gate_elimination.h"

namespace panda::ecmascript::kungfu {

void UselessGateElimination::PushGate(GateRef gate)
{
    workList_.push_back(gate);
    acc_.SetMark(gate, MarkCode::VISITED);
}

void UselessGateElimination::InitList()
{
    circuit_->AdvanceTime();
    GateRef returnList = acc_.GetReturnRoot();
    auto uses = acc_.Uses(returnList);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        PushGate(*useIt);
    }
    circuit_->GetAllGates(gateList_);
    for (auto gate : gateList_) {
        if (acc_.GetOpCode(gate) == OpCode::LOOP_BEGIN) {
            PushGate(gate);
        } else if (acc_.IsProlog(gate) || acc_.IsRoot(gate)) {
            acc_.SetMark(gate, MarkCode::VISITED);
        }
    }
}

void UselessGateElimination::MarkGate()
{
    while (!workList_.empty()) {
        GateRef gate = workList_.back();
        workList_.pop_back();
        std::vector<GateRef> ins;
        acc_.GetIns(gate, ins);
        for (auto in : ins) {
            if (acc_.GetMark(in) != MarkCode::VISITED) {
                PushGate(in);
            }
        }
    }
}

void UselessGateElimination::ReplaceDead(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        it = acc_.ReplaceIn(it, circuit_->DeadGate());
    }
    acc_.DeleteGate(gate);
}

void UselessGateElimination::EliminateUnmarkedGate()
{
    for (auto gate : gateList_) {
        if (acc_.GetMark(gate) != MarkCode::VISITED) {
            ReplaceDead(gate);
        }
    }
}

std::string UselessGateElimination::GetMethodName()
{
    return methodName_;
}

void UselessGateElimination::Run()
{
    InitList();
    MarkGate();
    EliminateUnmarkedGate();
    if (enableLog_) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After Useless Gate Elimination "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

}