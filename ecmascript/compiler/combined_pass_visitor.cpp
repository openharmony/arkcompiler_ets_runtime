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

#include "ecmascript/compiler/combined_pass_visitor.h"

namespace panda::ecmascript::kungfu {

int32_t CombinedPassVisitor::GetGateOrder(GateRef gate)
{
    auto id = acc_.GetId(gate);
    if (id >= orderList_.size()) {
        Resize(id + 1, -1);
    }
    return orderList_[acc_.GetId(gate)];
}

void CombinedPassVisitor::SetGateOrder(GateRef gate, int32_t orderId)
{
    auto id = acc_.GetId(gate);
    if (id >= orderList_.size()) {
        Resize(id + 1, -1);
    }
    orderList_[acc_.GetId(gate)] = orderId;
}

void CombinedPassVisitor::Resize(int32_t size, int32_t num)
{
    orderList_.resize(size, num);
}

void CombinedPassVisitor::AddPass(PassVisitor* pass)
{
    passList_.emplace_back(pass);
}

void CombinedPassVisitor::LogicallyReplaceGate(GateRef gate, GateRef replacement)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetMark(*it) == MarkCode::FINISHED) {
            PushChangedGate(*it);
        }
        it = acc_.ReplaceIn(it, replacement);
    }
}
void CombinedPassVisitor::RelaxStateAndDepend(GateRef gate)
{
    ReplaceGate(gate, StateDepend {acc_.GetState(gate), acc_.GetDep(gate)}, gate);
}

void CombinedPassVisitor::ReplaceGate(GateRef gate, GateRef replacement)
{
    GateRef depend = Circuit::NullGate();
    if (acc_.GetDependCount(gate) > 0) {
        ASSERT(acc_.GetDependCount(gate) == 1 || acc_.GetOpCode(replacement) == OpCode::DEAD); // 1: one dep
        depend = acc_.GetDep(gate);
    }
    GateRef state = Circuit::NullGate();
    if (acc_.GetStateCount(gate) > 0) {
        ASSERT(acc_.GetStateCount(gate) == 1 || acc_.GetOpCode(replacement) == OpCode::DEAD);  // 1: one state
        state = acc_.GetState(gate);
    }
    ReplaceGate(gate, StateDepend {state, depend}, replacement);
    acc_.DeleteGate(gate);
}

void CombinedPassVisitor::ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement)
{
    auto state = stateDepend.State();
    auto depend = stateDepend.Depend();
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetMark(*it) == MarkCode::FINISHED) {
            PushChangedGate(*it);
        }
        if (acc_.GetOpCode(replacement) == OpCode::DEAD) {
            it = acc_.ReplaceIn(it, replacement);
        } else if (acc_.IsStateIn(it)) {
            ASSERT(state != Circuit::NullGate());
            it = acc_.ReplaceIn(it, state);
        } else if (acc_.IsDependIn(it)) {
            ASSERT(depend != Circuit::NullGate());
            it = acc_.ReplaceIn(it, depend);
        } else {
            it = acc_.ReplaceIn(it, replacement);
        }
    }
#ifndef NDEBUG
    acc_.GetCircuit()->AddComment(replacement,  "old V " + std::to_string(acc_.GetId(gate)));
#endif
}

void CombinedPassVisitor::VistDependSelectorForLoop(GateRef gate)
{
    auto use = acc_.Uses(gate);
    for (auto useIt = use.begin(); useIt != use.end(); useIt++) {
        if (acc_.GetOpCode(*useIt) == OpCode::DEPEND_SELECTOR) {
            PushChangedGate(*useIt);
        }
    }
}

void CombinedPassVisitor::VisitGraph()
{
    for (auto pass : passList_) {
        pass->Initialize();
    }
    circuit_->AdvanceTime();
    orderCount_ = 0;
    Resize(circuit_->GetMaxGateId() + 1, -1);
    GateRef returnList = acc_.GetReturnRoot();
    auto uses = acc_.Uses(returnList);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        PushChangedGate(*useIt);
    }
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (auto gate : gateList) {
        if (acc_.GetOpCode(gate) == OpCode::LOOP_BEGIN) {
            PushChangedGate(gate);
            // For empty loop and no RETURN opcode
            VistDependSelectorForLoop(gate);
        }
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
            for (auto pass : passList_) {
                pass->Finalize();
            }
            break;
        }
    }
}

void CombinedPassVisitor::ReVisitGate(GateRef gate)
{
    if (acc_.GetMark(gate) == MarkCode::FINISHED) {
        PushChangedGate(gate);
    }
}


GateRef CombinedPassVisitor::VisitGate(GateRef gate)
{
    [[maybe_unused]] auto scopedGate = circuit_->VisitGateBegin(gate);
    auto skip = passList_.end();
    for (auto i = passList_.begin(); i != passList_.end();) {
        if (i == skip) {
            i++;
            continue;
        }
        GateRef replacement = (*i)->VisitGate(gate);
        if (replacement == gate) {
            skip = i;
            i = passList_.begin();
            continue;
        } else if (replacement != Circuit::NullGate()) {
            return replacement;
        }
        i++;
    }
    if (skip == passList_.end()) {
        return Circuit::NullGate();
    }
    return gate;
}
// Reverse post-order
void CombinedPassVisitor::VisitTopGate(Edge& current)
{
    GateRef gate = current.GetGate();
    // gate is delete or dead
    if (acc_.IsNop(gate)) {
        PopGate(gate);
        return;
    }
    auto numIns = acc_.GetNumIns(gate);
    auto start = current.GetIndex();
    if (start >= numIns) {
        start = 0;
    }
    for (size_t i = start; i < numIns; i++) {
        GateRef input = acc_.GetIn(gate, i);
        if (input == gate) {
            continue;
        }
        // find not visited gate, push stack
        if (acc_.GetMark(input) < MarkCode::VISITED) {
            PushGate(input, 0);
            // next index
            current.SetIndex(i + 1);
            return;
        }
    }
    for (size_t i = 0; i < start; i++) {
        GateRef input = acc_.GetIn(gate, i);
        if (input == gate) {
            continue;
        }
        // find not visited gate, push stack
        if (acc_.GetMark(input) < MarkCode::VISITED) {
            PushGate(input, 0);
            // next index
            current.SetIndex(i + 1);
            return;
        }
    }
    // all input are visited
    if (GetGateOrder(gate) == -1) { // -1 : not visited before
        SetGateOrder(gate, ++orderCount_);
    }
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

void CombinedPassVisitor::PrintStack()
{
    std::string log;
    for (size_t i = 0; i < workList_.size(); i++) {
        Edge current = workList_[i];
        GateRef gate = current.GetGate();
        log += std::to_string(acc_.GetId(gate)) + ", ";
    }
    LOG_COMPILER(INFO) << std::dec << log;
}

void CombinedPassVisitor::PrintLog(const std::string& phaseName)
{
    if (enableLog_) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                        << "===================="
                        << " After " << phaseName << " "
                        << "[" << methodName_ << "]"
                        << "===================="
                        << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

} // namespace panda::ecmascript::kungfu
