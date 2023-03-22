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
#include "ecmascript/compiler/later_elimination.h"

namespace panda::ecmascript::kungfu {
void LaterElimination::Run()
{
    dependChains_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
    VisitGraph();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After late elimination "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

GateRef LaterElimination::VisitDependEntry(GateRef gate)
{
    auto empty = new (chunk_) DependChainNodes(chunk_);
    return UpdateDependChain(gate, empty);
}

GateRef LaterElimination::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::GET_CONSTPOOL:
            return TryEliminateGate(gate);
        case OpCode::DEPEND_SELECTOR:
            return TryEliminateDependSelector(gate);
        default:
            if (acc_.GetDependCount(gate) == 1) { // 1: depend in is 1
                return TryEliminateOther(gate);
            }
    }
    return Circuit::NullGate();
}

GateRef LaterElimination::TryEliminateOther(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) >= 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    return UpdateDependChain(gate, dependChain);
}

GateRef LaterElimination::TryEliminateGate(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    // lookup gate, replace
    auto preGate = dependChain->LookupNode(this, gate);
    if (preGate != Circuit::NullGate()) {
        return preGate;
    }
    // update gate, for others elimination
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

GateRef LaterElimination::TryEliminateDependSelector(GateRef gate)
{
    auto state = acc_.GetState(gate);
    if (acc_.IsLoopHead(state)) {
        // use loop head as depend chain
        return TryEliminateOther(gate);
    }

    auto dependCount = acc_.GetDependCount(gate);
    for (size_t i = 0; i < dependCount; ++i) {
        auto depend = acc_.GetDep(gate, i);
        auto dependChain = GetDependChain(depend);
        if (dependChain == nullptr) {
            return Circuit::NullGate();
        }
    }

    // all depend done.
    auto depend = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depend);
    DependChainNodes* copy = new (chunk_) DependChainNodes(chunk_);
    copy->CopyFrom(dependChain);
    for (size_t i = 1; i < dependCount; ++i) { // 1: second in
        auto dependIn = acc_.GetDep(gate, i);
        auto tempChain = GetDependChain(dependIn);
        copy->Merge(tempChain);
    }
    return UpdateDependChain(gate, copy);
}

GateRef LaterElimination::UpdateDependChain(GateRef gate, DependChainNodes* dependChain)
{
    ASSERT(dependChain != nullptr);
    auto oldDependChain = GetDependChain(gate);
    if (dependChain->Equals(oldDependChain)) {
        return Circuit::NullGate();
    }
    dependChains_[acc_.GetId(gate)] = dependChain;
    return gate;
}

bool LaterElimination::CheckReplacement(GateRef lhs, GateRef rhs)
{
    if (acc_.GetMetaData(lhs) != acc_.GetMetaData(rhs)) {
        if (acc_.GetOpCode(lhs) != acc_.GetOpCode(rhs)) {
            return false;
        }
    }
    size_t valueCount = acc_.GetNumValueIn(lhs);
    for (size_t i = 0; i < valueCount; i++) {
        if (acc_.GetValueIn(lhs, i) != acc_.GetValueIn(rhs, i)) {
            return false;
        }
    }
    return true;
}

void DependChainNodes::Merge(DependChainNodes* that)
{
    // find common sub list
    while (size_ > that->size_) {
        head_ = head_->next;
        size_--;
    }

    auto lhs = this->head_;
    auto rhs = that->head_;
    size_t rhsSize = that->size_;
    while (rhsSize > size_) {
        rhs = rhs->next;
        rhsSize--;
    }
    while (lhs != rhs) {
        ASSERT(lhs != nullptr);
        lhs = lhs->next;
        rhs = rhs->next;
        size_--;
    }
    head_ = lhs;
}

bool DependChainNodes::Equals(DependChainNodes* that)
{
    if (that == nullptr) {
        return false;
    }
    if (size_ != that->size_) {
        return false;
    }
    auto lhs = this->head_;
    auto rhs = that->head_;
    while (lhs != rhs) {
        if (lhs->gate != rhs->gate) {
            return false;
        }
        lhs = lhs->next;
        rhs = rhs->next;
    }
    return true;
}

GateRef DependChainNodes::LookupNode(LaterElimination* elimination, GateRef gate)
{
    for (Node* node = head_; node != nullptr; node = node->next) {
        if (elimination->CheckReplacement(node->gate, gate)) {
            return node->gate;
        }
    }
    return Circuit::NullGate();
}

DependChainNodes* DependChainNodes::UpdateNode(GateRef gate)
{
    // assign node->next to head
    Node* node = chunk_->New<Node>(gate, head_);
    DependChainNodes* that = new (chunk_) DependChainNodes(chunk_);
    // assign head to node
    that->head_ = node;
    that->size_ = size_ + 1;
    return that;
}
}  // namespace panda::ecmascript::kungfu