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

#include <queue>
#include <stack>

#include "ecmascript/compiler/circuit_optimizer.h"
#include "ecmascript/compiler/guard_eliminating.h"
#include "ecmascript/compiler/scheduler.h"

namespace panda::ecmascript::kungfu {
bool GuardEliminating::HasGuard(GateRef gate) const
{
    if (acc_.IsTypedOperator(gate)) {
        auto guard = acc_.GetDep(gate);
        auto op = acc_.GetOpCode(guard);
        return op == OpCode::GUARD;
    }
    return false;
}

void GuardEliminating::ProcessTwoConditions(GateRef gate, std::set<GateRef> &conditionSet)
{
    auto guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    auto condition = acc_.GetValueIn(guard, 0);
    auto left = acc_.GetValueIn(condition, 0);
    auto right = acc_.GetValueIn(condition, 1);
    if (left == right) {
        acc_.ReplaceValueIn(guard, left, 0);
        ProcessOneCondition(gate, conditionSet);
        return;
    }
    if (conditionSet.count(left) > 0 && conditionSet.count(right) > 0) {
        acc_.DeleteGuardAndFrameState(gate);
    } else if (conditionSet.count(left) > 0) {
        acc_.ReplaceValueIn(guard, right, 0);
        conditionSet.insert(right);
    } else if (conditionSet.count(right) > 0) {
        acc_.ReplaceValueIn(guard, left, 0);
        conditionSet.insert(left);
    } else {
        conditionSet.insert(left);
        conditionSet.insert(right);
    }
}

void GuardEliminating::ProcessOneCondition(GateRef gate, std::set<GateRef> &conditionSet)
{
    auto guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    auto condition = acc_.GetValueIn(guard, 0);
    if (conditionSet.count(condition) > 0) {
        acc_.DeleteGuardAndFrameState(gate);
    } else {
        conditionSet.insert(condition);
    }
}

void GuardEliminating::RemoveConditionFromSet(GateRef condition, std::set<GateRef> &conditionSet)
{
    if (acc_.GetOpCode(condition) == OpCode::AND) {
        auto left = acc_.GetValueIn(condition, 0);
        auto right = acc_.GetValueIn(condition, 1);
        conditionSet.erase(left);
        conditionSet.erase(right);
    } else {
        conditionSet.erase(condition);
    }
}

bool GuardEliminating::IsTrustedType(GateRef gate) const
{
    if (acc_.IsConstant(gate)) {
        return true;
    }
    if (acc_.IsTypedOperator(gate)) {
        if (acc_.GetOpCode(gate) == OpCode::TYPED_BINARY_OP) {
            return !acc_.GetGateType(gate).IsIntType();
        } else {
            return true;
        }
    }
    return false;
}

void GuardEliminating::RemoveOneTrusted(GateRef gate)
{
    auto guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    auto condition = acc_.GetValueIn(guard);
    if (condition == builder_.Boolean(true)) {
        acc_.DeleteGuardAndFrameState(gate);
    }
}

void GuardEliminating::RemoveTwoTrusted(GateRef gate)
{
    auto guard = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(guard) == OpCode::GUARD);
    auto condition = acc_.GetValueIn(guard);
    auto left = acc_.GetValueIn(condition, 0);
    auto right = acc_.GetValueIn(condition, 1);
    if (left == builder_.Boolean(true) && right == builder_.Boolean(true)) {
        acc_.DeleteGuardAndFrameState(gate);
    } else if (left == builder_.Boolean(true)) {
        acc_.ReplaceValueIn(guard, right, 0);
    } else if (right == builder_.Boolean(true)) {
        acc_.ReplaceValueIn(guard, left, 0);
    }
}

void GuardEliminating::TrustedTypePropagate(std::queue<GateRef>& workList, const std::vector<GateRef>& checkList)
{
    std::unordered_map<GateRef, size_t> trustedInCount;
    while (!workList.empty()) {
        auto gate = workList.front();
        workList.pop();
        std::vector<GateRef> outs;
        acc_.GetOutVector(gate, outs);
        for (auto phi : outs) {
            if ((acc_.GetOpCode(phi) != OpCode::VALUE_SELECTOR) ||
                (acc_.GetGateType(phi) != acc_.GetGateType(gate))) {
                continue;
            }
            trustedInCount[phi]++;
            if (trustedInCount.at(phi) == acc_.GetNumValueIn(phi)) {
                workList.push(phi);
            }
        }
    }
    for (auto check : checkList) {
        ASSERT(acc_.GetOpCode(check) == OpCode::TYPE_CHECK);
        auto phi = acc_.GetValueIn(check, 0);
        if ((trustedInCount.count(phi) == 0) ||
            (trustedInCount.at(phi) != acc_.GetNumValueIn(phi)) ||
            (acc_.GetGateType(phi) != acc_.GetRightType(check))) {
            continue;
        }
        acc_.UpdateAllUses(check, builder_.Boolean(true));
    }
}

void GuardEliminating::RemoveTrustedTypeCheck(const std::vector<GateRef>& guardedList)
{
    for (auto gate : guardedList) {
        ASSERT(HasGuard(gate));
        auto guard = acc_.GetDep(gate);
        auto condition = acc_.GetValueIn(guard);
        if (acc_.GetOpCode(condition) == OpCode::AND) {
            RemoveTwoTrusted(gate);
        } else {
            RemoveOneTrusted(gate);
        }
    }
}

void GuardEliminating::Run()
{
    // eliminate duplicate typecheck
    GlobalValueNumbering(circuit_).Run();

    // calculate dominator tree
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    Scheduler::CalculateDominatorTree(circuit_, bbGatesList, bbGatesAddrToIdx, immDom);
    std::vector<std::vector<size_t>> domTree(immDom.size(), std::vector<size_t>(0));
    for (size_t idx = 1; idx < immDom.size(); ++idx) {
        domTree[immDom[idx]].emplace_back(idx);
    }

    // dfs the dominator tree to eliminate guard
    // which is domined by another guard with same condition
    std::set<GateRef> conditionSet;
    struct DFSState {
        size_t curbb;
        size_t idx;
    };
    std::stack<DFSState> dfsStack;
    auto startGate = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
    DFSState startState = { bbGatesAddrToIdx[startGate], 0 };
    dfsStack.push(startState);
    while (!dfsStack.empty()) {
        auto &curState = dfsStack.top();
        auto &curbb = curState.curbb;
        auto &idx = curState.idx;
        if (idx == domTree[curbb].size()) {
            auto curGate = bbGatesList[curbb];
            if (HasGuard(curGate)) {
                auto guard = acc_.GetDep(curGate);
                auto condition = acc_.GetValueIn(guard, 0);
                RemoveConditionFromSet(condition, conditionSet);
            }
            dfsStack.pop();
            continue;
        }
        auto succbb = domTree[curbb][idx];
        auto succGate = bbGatesList[succbb];
        if (HasGuard(succGate)) {
            auto guard = acc_.GetDep(succGate);
            auto condition = acc_.GetValueIn(guard, 0);
            if (acc_.GetOpCode(condition) == OpCode::AND) {
                ProcessTwoConditions(succGate, conditionSet);
            } else {
                ProcessOneCondition(succGate, conditionSet);
            }
        }
        DFSState newState = { succbb, 0 };
        dfsStack.push(newState);
        idx++;
    }

    std::vector<GateRef> allGates;
    acc_.GetAllGates(allGates);
    std::queue<GateRef> workList;
    std::vector<GateRef> checkList;
    std::vector<GateRef> guardedList;
    for (auto gate : allGates) {
        if (IsTrustedType(gate)) {
            workList.push(gate);
        }
        if (acc_.GetOpCode(gate) == OpCode::TYPE_CHECK) {
            checkList.emplace_back(gate);
        }
        if (HasGuard(gate)) {
            guardedList.emplace_back(gate);
        }
    }
    TrustedTypePropagate(workList, checkList);
    RemoveTrustedTypeCheck(guardedList);

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After guard eliminating "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}
}  // namespace panda::ecmascript::kungfu