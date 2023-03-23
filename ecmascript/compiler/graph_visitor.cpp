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
#include "ecmascript/compiler/graph_visitor.h"

namespace panda::ecmascript::kungfu {

void GraphVisitor::ReplaceGate(GateRef gate, GateRef replacement)
{
    GateRef depend = Circuit::NullGate();
    if (acc_.GetDependCount(gate) > 0) {
        ASSERT(acc_.GetDependCount(gate) == 1); // 1: one dep
        depend = acc_.GetDep(gate);
    }
    GateRef state = Circuit::NullGate();
    if (acc_.GetStateCount(gate) > 0) {
        ASSERT(acc_.GetStateCount(gate) == 1);  // 1: one state
        state = acc_.GetState(gate);
    }
    return ReplaceGate(gate, StateDepend {state, depend}, replacement);
}

void GraphVisitor::ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement)
{
    ASSERT(gate != replacement);
    auto state = stateDepend.State();
    auto depend = stateDepend.Depend();
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetMark(*it) == MarkCode::FINISHED) {
            PushChangedGate(*it);
        }
        if (acc_.IsStateIn(it)) {
            ASSERT(state != Circuit::NullGate());
            it = acc_.ReplaceIn(it, state);
        } else if (acc_.IsDependIn(it)) {
            ASSERT(depend != Circuit::NullGate());
            it = acc_.ReplaceIn(it, depend);
        } else {
            it = acc_.ReplaceIn(it, replacement);
        }
    }
    acc_.DeleteGate(gate);
}

void GraphVisitor::VisitGraph()
{
    circuit_->AdvanceTime();
    GateRef returnList = acc_.GetReturnRoot();
    auto uses = acc_.Uses(returnList);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        PushChangedGate(*useIt);
    }

    while (true) {
        if (!workList_.empty()) {
            Edge& current = workList_.back();
            VisitTopGate(current);
        } else if (!changedList_.empty()) {
            GateRef gate = changedList_.back();
            changedList_.pop_back();
            if (acc_.GetMark(gate) == MarkCode::PREVISIT) {
                PushGate(gate, 0);
            }
        } else {
            break;
        }
    }
}

void GraphVisitor::ReVisitGate(GateRef gate)
{
    if (acc_.GetMark(gate) == MarkCode::FINISHED) {
        PushChangedGate(gate);
    }
}


// Reverse post-order
void GraphVisitor::VisitTopGate(Edge& current)
{
    GateRef gate = current.GetGate();
    // gate is delete or dead
    if (acc_.GetMetaData(gate)->IsNop()) {
        PopGate(gate);
        return;
    }
    auto numIns = acc_.GetNumIns(gate);
    for (size_t i = current.GetIndex(); i < numIns; i++) {
        GateRef input = acc_.GetIn(gate, i);
        ASSERT(input != gate);
        // find not visited gate, push stack
        if (acc_.GetMark(input) < MarkCode::VISITED) {
            PushGate(input, 0);
            // next index
            current.SetIndex(i + 1);
            return;
        }
    }
    // all input are visited
    GateRef replacement = VisitGate(gate);
    PopGate(gate);
    if (replacement == Circuit::NullGate()) {
        return;
    }
    if (replacement != gate) {
        ReplaceGate(gate, replacement);
    } else {
        // revisit not on stack gate.
        auto uses = acc_.Uses(gate);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            if (acc_.GetMark(*it) == MarkCode::FINISHED) {
                PushChangedGate(*it);
            }
        }
    }
}

void GraphVisitor::PrintStack()
{
    std::string log;
    for (size_t i = 0; i < workList_.size(); i++) {
        Edge current = workList_[i];
        GateRef gate = current.GetGate();
        log += std::to_string(acc_.GetId(gate)) + ", ";
    }
    LOG_COMPILER(INFO) << std::dec << log;
}

}  // namespace panda::ecmascript::kungfu