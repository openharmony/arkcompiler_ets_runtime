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
            case OpCode::MERGE:
                PropagateMerge(edge);
                break;
            case OpCode::LOOP_BACK:
                PropagateLoopBack(edge);
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
        // 2: is loop begin
        ASSERT(acc_.GetStateCount(gate) == 1 ||
            acc_.GetStateCount(gate) == 2); // 2: LOOP_BEGIN
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

void GraphEditor::PropagateLoopBack(const Edge& edge)
{
    GateRef gate = edge.GetGate();

    auto uses = acc_.Uses(gate);
    auto firstUse = uses.begin();
    if (firstUse == uses.end()) {
        // LoopBegin => LoopBack
        ReplaceGate(gate);
    } else {
        // maybe Merge => LoopBack
        GateRef loopBegin = *firstUse;
        GateRef otherStateIn = acc_.GetState(loopBegin, 0);

        firstUse = acc_.Uses(loopBegin).begin();
        acc_.ReplaceIn(*firstUse, firstUse.GetIndex(), otherStateIn);
        ReplaceGate(gate);
        acc_.DeleteGate(loopBegin);
    }
}

}  // namespace panda::ecmascript::kungfu