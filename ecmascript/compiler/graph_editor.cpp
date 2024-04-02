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

#include <queue>
#include <stack>
#include "ecmascript/compiler/graph_editor.h"

namespace panda::ecmascript::kungfu {

void GraphEditor::RemoveDeadState(Circuit* circuit, GateRef gate)
{
    GraphEditor editor(circuit);
    editor.ReplaceGate(gate);
    editor.RemoveGate();
}

void GraphEditor::EliminateRedundantPhi(Circuit* circuit, bool enableLog, const std::string& methodName)
{
    GraphEditor editor(circuit);
    editor.EliminatePhi();
    if (enableLog) {
        LOG_COMPILER(INFO) << " ";
        LOG_COMPILER(INFO) << "\033[34m" << "================="
                           << " After Redundant Phi Elimination "
                           << "[" << methodName << "] "
                           << "=================" << "\033[0m";
        circuit->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

void GraphEditor::ReplaceGate(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        if (acc_.IsDependIn(useIt)) {
            GateRef depend = acc_.GetDep(gate);
            useIt = acc_.ReplaceIn(useIt, depend);
        } else {
            workList_.push_back(useIt.GetEdge());
            useIt = acc_.ReplaceIn(useIt, circuit_->DeadGate());
        }
    }
    acc_.DeleteGate(gate);
}

void GraphEditor::RemoveGate()
{
    while (!workList_.empty()) {
        Edge& edge = workList_.back();
        GateRef gate = edge.GetGate();
        workList_.pop_back();
        auto opcode = acc_.GetOpCode(gate);
        switch (opcode) {
            case OpCode::NOP:
            case OpCode::DEAD:
            case OpCode::VALUE_SELECTOR:
            case OpCode::DEPEND_SELECTOR:
                // dead op, just continue
                break;
            case OpCode::LOOP_BEGIN:
            case OpCode::MERGE:
                PropagateMerge(edge);
                break;
            default:
                PropagateGate(edge);
                break;
        }
    }
}

void GraphEditor::PropagateGate(const Edge& edge)
{
    GateRef gate = edge.GetGate();
    // isStateIn
    if (acc_.IsStateIn(gate, edge.GetIndex())) {
        ASSERT(acc_.GetStateCount(gate) == 1);
        ReplaceGate(gate);
        return;
    }

    // IsValueIn
    if (acc_.IsValueIn(gate, edge.GetIndex())) {
        // value gate
        ReplaceGate(gate);
    }
}

void GraphEditor::PropagateMerge(const Edge& edge)
{
    GateRef gate = edge.GetGate();
    auto numIns = acc_.GetNumIns(gate);
    if (numIns == 1) {
        ReplaceGate(gate);
    } else {
        auto uses = acc_.Uses(gate);
        for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
            GateRef use = *useIt;
            //  (Gate1)   (Dead)   (Gate2)       (Gate1)  (Gate2)
            //     |       |         |              |        |
            //     ___________________      ==>     __________
            //             |                             |
            //           (phi)                         (phi)
            if (acc_.GetOpCode(use) == OpCode::VALUE_SELECTOR ||
                acc_.GetOpCode(use) == OpCode::DEPEND_SELECTOR) {
                acc_.DecreaseIn(use, edge.GetIndex() + 1); // +1 skip state input
            }
        }
        acc_.DecreaseIn(gate, edge.GetIndex());
    }
}

bool GraphEditor::HasOsrDeoptUse(GateRef gate)
{
    std::vector<GateRef> valueOuts;
    acc_.GetValueUses(gate, valueOuts);
    for (auto out : valueOuts) {
        if (acc_.GetOpCode(out) != OpCode::FRAME_STATE) {
            continue;
        }
        std::vector<GateRef> outs;
        acc_.GetValueUses(out, outs);
        for (auto use : outs) {
            if (acc_.GetOpCode(use) != OpCode::DEOPT_CHECK) {
                continue;
            }
            GateRef deoptType = acc_.GetValueIn(use, 2);  // 2: deopt type
            uint64_t v = acc_.GetConstantValue(deoptType);
            if (v == static_cast<uint64_t>(DeoptType::OSRLOOPEXIT)) {
                return true;
            }
        }
    }
    return false;
}

void GraphEditor::EliminatePhi()
{
    circuit_->AdvanceTime();
    std::vector<GateRef> gateList;
    acc_.GetAllGates(gateList);
    std::vector<GateRef> phis;
    std::queue<GateRef> workList;
    // nomarked phis are unused phis
    // set previsit for phis in worklist
    // set visited for used phis
    // set finished for used gate which is self-use or has same inputs

    for (auto gate : gateList) {
        if (acc_.IsValueSelector(gate)) {
            phis.emplace_back(gate);
            continue;
        }
        if (acc_.IsFrameValues(gate) && !HasOsrDeoptUse(gate)) {
            continue;
        }
        // get used phi
        auto valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            if (acc_.IsValueSelector(input) && acc_.IsNotMarked(input)) {
                acc_.SetPrevisit(input);
                workList.push(input);
            }
        }
    }

    // visit used phi
    while (!workList.empty()) {
        auto cur = workList.front();
        workList.pop();
        acc_.SetVisited(cur);
        auto valueNum = acc_.GetNumValueIn(cur);
        bool sameIns = true;
        GateRef first = acc_.GetValueIn(cur, 0);
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(cur, i);
            if (input != first) {
                sameIns = false;
            }
            if (acc_.IsValueSelector(input) && acc_.IsNotMarked(input)) {
                workList.push(input);
                acc_.SetPrevisit(input);
            }
        }

        if (!sameIns) {
            continue;
        }

        acc_.SetFinished(cur);
        auto uses = acc_.Uses(cur);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            if (acc_.IsValueSelector(*it) && acc_.IsVisited(*it)) {
                // revisit user
                acc_.SetPrevisit(*it);
                workList.push(*it);
            }
        }
        acc_.UpdateAllUses(cur, first);
        acc_.DeleteGate(cur);
    }

    // delete unused phi
    for (auto phi : phis) {
        if (acc_.IsValueSelector(phi) && acc_.IsNotMarked(phi)) {
            auto optimizedGate = circuit_->GetConstantGate(MachineType::I64,
                JSTaggedValue::VALUE_OPTIMIZED_OUT, GateType::TaggedValue());
            acc_.UpdateAllUses(phi, optimizedGate);
            acc_.DeleteGate(phi);
        }
    }
}
}  // namespace panda::ecmascript::kungfu
