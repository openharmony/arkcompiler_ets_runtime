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

#include "ecmascript/compiler/check_elimination.h"

namespace panda::ecmascript::kungfu {
void CheckElimination::Run()
{
    RemovePassedCheck();
    RemoveTypeTrustedCheck();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After check eliminating "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

bool CheckElimination::IsPrimitiveTypeCheck(GateRef gate) const {
    auto op = acc_.GetOpCode(gate);
    return op == OpCode::PRIMITIVE_TYPE_CHECK;
}

bool CheckElimination::IsTrustedType(GateRef gate) const
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

void CheckElimination::TrustedTypePropagate(std::queue<GateRef>& workList, const std::vector<GateRef>& checkList)
{
    std::unordered_map<GateRef, size_t> trustedInCount;
    while (!workList.empty()) {
        auto gate = workList.front();
        workList.pop();
        auto uses = acc_.Uses(gate);
        for (auto i = uses.begin(); i != uses.end(); i++) {
            GateRef phi = *i;
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
        ASSERT(acc_.GetOpCode(check) == OpCode::PRIMITIVE_TYPE_CHECK);
        auto value = acc_.GetValueIn(check, 0);
        ASSERT(acc_.GetGateType(value) == acc_.GetParamGateType(check));
        if (IsTrustedType(value)) {
            RemoveCheck(check);
            continue;
        }
        if ((trustedInCount.count(value) != 0) &&
            (trustedInCount.at(value) == acc_.GetNumValueIn(value))) {
            RemoveCheck(check);
            continue;
        }
        // remove check
    }
}

void CheckElimination::RemoveCheck(GateRef gate) {
    auto state = acc_.GetState(gate);
    auto depend = acc_.GetDep(gate);
    auto uses = acc_.Uses(gate);
    for (auto i = uses.begin(); i != uses.end();) {
        if (acc_.IsStateIn(i)) {
            i = acc_.ReplaceIn(i, state);
        } else if (acc_.IsDependIn(i)) {
            i = acc_.ReplaceIn(i, depend);
        } else if (acc_.IsValueIn(i)) {
            i = acc_.ReplaceIn(i, depend);
        }
    }
    acc_.DeleteGate(gate);
}

void CheckElimination::RemovePassedCheck()
{
    std::queue<GateRef> workList;
    std::vector<GateRef> passedCheck;
    std::vector<GateRef> outs;
    workList.push(acc_.GetStateRoot());
    std::unordered_set<GateRef> mergeVisit;
    std::set<std::tuple<OpCode, BitField, GateRef>> CheckSetForOne;
    std::set<std::tuple<OpCode, BitField, GateRef, GateRef>> CheckSetForTwo;
    while (!workList.empty()) {
        auto gate = workList.front();
        workList.pop();
        if (acc_.GetOpCode(gate) == OpCode::MERGE) {
            if (!mergeVisit.count(gate)) {
                mergeVisit.insert(gate);
            } else {
                continue;
            }
        }
        CheckSetForOne.clear();
        CheckSetForTwo.clear();
        while (true) {
            if (!acc_.IsNotWrite(gate)) {
                CheckSetForOne.clear();
                CheckSetForTwo.clear();
            }
            if (acc_.IsCheckWithOneIn(gate)) {
                auto op = OpCode(acc_.GetOpCode(gate));
                auto bit = acc_.TryGetValue(gate);
                auto v0 = acc_.GetValueIn(gate, 0);
                auto tp = std::make_tuple(op, bit, v0);
                if (CheckSetForOne.count(tp)) {
                    passedCheck.emplace_back(gate);
                } else {
                    CheckSetForOne.insert(tp);
                }
            }
            if (acc_.IsCheckWithTwoIns(gate)) {
                auto op = OpCode(acc_.GetOpCode(gate));
                auto bit = acc_.TryGetValue(gate);
                auto v0 = acc_.GetValueIn(gate, 0);
                auto v1 = acc_.GetValueIn(gate, 1);
                auto tp = std::make_tuple(op, bit, v0, v1);
                if (CheckSetForTwo.count(tp)) {
                    passedCheck.emplace_back(gate);
                } else {
                    CheckSetForTwo.insert(tp);
                }
            }

            outs.clear();
            acc_.GetStateUses(gate, outs);

            if (outs.size() == 1) {
                gate = outs[0];
                auto op = acc_.GetOpCode(gate);
                if (op == OpCode::LOOP_BACK || op == OpCode::LOOP_BEGIN || op == OpCode::MERGE) {
                    break;
                }
                continue;
            }
            if ((outs.size() == 2) && (acc_.GetOpCode(gate) == OpCode::JS_BYTECODE)) {
                if(acc_.GetOpCode(outs[0]) == OpCode::IF_SUCCESS) {
                    gate = outs[0];
                    continue;
                }
                if(acc_.GetOpCode(outs[1]) == OpCode::IF_SUCCESS) {
                    gate = outs[1];
                    continue;
                }
            }
            break;
        }
        for (auto out : outs) {
            if (acc_.GetOpCode(out) != OpCode::LOOP_BACK) {
                workList.push(out);
            }
        }
    }
    for (auto check : passedCheck) {
        RemoveCheck(check);
    }
}

void CheckElimination::RemoveTypeTrustedCheck() {
    // eliminate type check for type trusted gate (for primitive type check)
    std::vector<GateRef> allGates;
    acc_.GetAllGates(allGates);
    std::queue<GateRef> workList;
    std::vector<GateRef> checkList;
    for (auto gate : allGates) {
        if (IsTrustedType(gate)) {
            workList.push(gate);
        }
        if (IsPrimitiveTypeCheck(gate)) {
            checkList.emplace_back(gate);
        }
    }
    TrustedTypePropagate(workList, checkList);
}
}  // namespace panda::ecmascript::kungfu