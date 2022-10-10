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

void GuardEliminating::Run()
{
    // eliminate duplicate typecheck
    GlobalValueNumbering(circuit_).Run();

    // calculate dominator tree
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit_);
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
                conditionSet.erase(condition);
            }
            dfsStack.pop();
            continue;
        }
        auto succbb = domTree[curbb][idx];
        auto succGate = bbGatesList[succbb];
        if (HasGuard(succGate)) {
            auto guard = acc_.GetDep(succGate);
            auto condition = acc_.GetValueIn(guard, 0);
            if (conditionSet.count(condition) > 0) {
                acc_.DeleteGuardAndFrameState(succGate);
            } else {
                conditionSet.insert(condition);
            }
        }
        DFSState newState = { succbb, 0 };
        dfsStack.push(newState);
        idx++;
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After guard eliminating "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGates(*bcBuilder_);
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}
}  // namespace panda::ecmascript::kungfu