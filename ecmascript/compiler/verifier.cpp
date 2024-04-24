/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/verifier.h"

#include <cmath>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::kungfu {
bool Verifier::RunDataIntegrityCheck(const Circuit *circuit)
{
    std::unordered_set<GateRef> gatesSet;
    std::vector<GateRef> gatesList;
    GateRef prevGate = sizeof(Out);
    gatesList.push_back(prevGate);
    gatesSet.insert(prevGate);
    size_t out = Gate::GetGateSize(0);
    while (true) {
        GateRef gate = circuit->GetGateRef(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<const Out *>(circuit->LoadGatePtrConst(GateRef(out)))->GetGateConst());
        if (gate < prevGate + static_cast<int64_t>(sizeof(Gate)) ||
            gate >= static_cast<int64_t>(circuit->GetCircuitDataSize())) {
            LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (bad next gate)";
            LOG_COMPILER(ERROR) << "at: " << std::dec << gate;
            return false;
        }
        gatesList.push_back(gate);
        gatesSet.insert(gate);
        prevGate = gate;
        out += Gate::GetGateSize(
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<const Out *>(circuit->LoadGatePtrConst(GateRef(out)))->GetIndex() + 1);
        if (out == circuit->GetCircuitDataSize()) {
            break;
        }
        if (out > circuit->GetCircuitDataSize() || out < 0) {
            LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (out of bound access)";
            LOG_COMPILER(ERROR) << "at: " << std::dec << out;
            return false;
        }
    }
    for (const auto &gate : gatesList) {
        for (size_t idx = 0; idx < circuit->LoadGatePtrConst(gate)->GetNumIns(); idx++) {
            const In *curIn = circuit->LoadGatePtrConst(gate)->GetInConst(idx);
            if (!(circuit->GetSpaceDataStartPtrConst() < curIn && curIn < circuit->GetSpaceDataEndPtrConst())) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (corrupted in list)";
                LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                return false;
            }
            if (gatesSet.count(circuit->GetGateRef(curIn->GetGateConst())) == 0) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (invalid in address)";
                LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                return false;
            }
        }
        {
            const Gate *curGate = circuit->LoadGatePtrConst(gate);
            if (!curGate->IsFirstOutNull()) {
                const Out *curOut = curGate->GetFirstOutConst();
                if (!(circuit->GetSpaceDataStartPtrConst() < curOut && curOut < circuit->GetSpaceDataEndPtrConst())) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (corrupted out list)";
                    LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                    return false;
                }
                if (gatesSet.count(circuit->GetGateRef(curOut->GetGateConst())) == 0) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (invalid out address)";
                    LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                    return false;
                }
                while (!curOut->IsNextOutNull()) {
                    curOut = curOut->GetNextOutConst();
                    if (!(circuit->GetSpaceDataStartPtrConst() < curOut &&
                        curOut < circuit->GetSpaceDataEndPtrConst())) {
                        LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (corrupted out list)";
                        LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                        return false;
                    }
                    if (gatesSet.count(circuit->GetGateRef(curOut->GetGateConst())) == 0) {
                        LOG_COMPILER(ERROR) << "[Verifier][Error] Circuit data is corrupted (invalid out address)";
                        LOG_COMPILER(ERROR) << "id: " << std::dec << circuit->GetId(gate);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Verifier::RunStateGatesCheck(const Circuit *circuit, const std::vector<GateRef> &bbGatesList,
                                  const std::string& methodName)
{
    for (const auto &bbGate : bbGatesList) {
        circuit->Verify(bbGate, methodName);
    }
    return true;
}

bool Verifier::RunCFGSoundnessCheck(const Circuit *circuit, const std::vector<GateRef> &bbGatesList,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx)
{
    for (const auto &bbGate : bbGatesList) {
        GateAccessor gateAcc(const_cast<Circuit *>(circuit));
        for (const auto &predGate : gateAcc.ConstIns(bbGate)) {
            if (gateAcc.IsState(predGate) || circuit->GetOpCode(predGate) == OpCode::STATE_ENTRY) {
                if (bbGatesAddrToIdx.count(predGate) == 0) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] CFG is not sound";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(predGate) << ") is pred of "
                              << "(id=" << circuit->GetId(bbGate) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(bbGate) << ") is reachable from entry";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(predGate) << ") is unreachable from entry";
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verifier::RunCFGIsDAGCheck(const Circuit *circuit)
{
    circuit->AdvanceTime();
    struct DFSState {
        GateRef cur;
        GateAccessor::ConstUseWrapper uses;
        GateAccessor::ConstUseIterator use;
    };
    std::stack<DFSState> dfsStack;
    GateAccessor gateAcc(const_cast<Circuit *>(circuit));
    auto root = gateAcc.GetStateRoot();
    gateAcc.SetVisited(root);
    auto rootUses = gateAcc.ConstUses(root);
    dfsStack.push({root, rootUses, rootUses.begin()});
    while (!dfsStack.empty()) {
        auto &curState = dfsStack.top();
        auto &cur = curState.cur;
        auto &uses = curState.uses;
        auto &use = curState.use;
        if (use == uses.end()) {
            gateAcc.SetFinished(cur);
            dfsStack.pop();
            continue;
        }
        if (gateAcc.IsState(*use) && use.GetIndex() < gateAcc.GetStateCount(*use)) {
            if (gateAcc.IsVisited(*use)) {
                LOG_COMPILER(ERROR) <<
                    "[Verifier][Error] CFG without loop back edges is not a directed acyclic graph";
                LOG_COMPILER(ERROR) << "Proof:";
                LOG_COMPILER(ERROR) << "(id=" << gateAcc.GetId(*use) << ") is succ of "
                          << "(id=" << gateAcc.GetId(cur) << ")";
                LOG_COMPILER(ERROR) << "(id=" << gateAcc.GetId(cur) << ") is reachable from "
                          << "(id=" << gateAcc.GetId(*use) << ") without loop back edges";
                return false;
            }
            if (gateAcc.IsFinished(*use) || gateAcc.IsLoopBack(*use)) {
                ++use;
                continue;
            }
            gateAcc.SetVisited(*use);
            auto newUses = gateAcc.ConstUses(*use);
            dfsStack.push({*use, newUses, newUses.begin()});
        }
        ++use;
    }
    return true;
}

bool Verifier::RunCFGReducibilityCheck(const Circuit *circuit, const std::vector<GateRef> &bbGatesList,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor)
{
    for (const auto &curGate : bbGatesList) {
        if (circuit->GetOpCode(curGate) == OpCode::LOOP_BACK) {
            GateAccessor gateAcc(const_cast<Circuit *>(circuit));
            auto uses = gateAcc.ConstUses(curGate);
            for (auto use = uses.begin(); use != uses.end(); use++) {
                if (use.GetIndex() >= circuit->LoadGatePtrConst(*use)->GetStateCount()) {
                    continue;
                }
                ASSERT(gateAcc.IsState(*use));
                bool isDom = isAncestor(bbGatesAddrToIdx.at(*use), bbGatesAddrToIdx.at(curGate));
                if (!isDom) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] CFG is not reducible";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(*use) << ") is loop back succ of "
                              << "(id=" << circuit->GetId(curGate) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(*use) << ") does not dominate "
                              << "(id=" << circuit->GetId(curGate) << ")";
                    return false;
                }
            }
        }
    }
    return true;
}

bool Verifier::RunFixedGatesCheck(const Circuit *circuit, const std::vector<GateRef> &fixedGatesList)
{
    for (const auto &fixedGate : fixedGatesList) {
        circuit->Verify(fixedGate);
    }
    return true;
}

bool Verifier::RunFixedGatesRelationsCheck(const Circuit *circuit, const std::vector<GateRef> &fixedGatesList,
                                           const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                           const std::function<bool(size_t, size_t)> &isAncestor)
{
    ConstGateAccessor ac(circuit);
    for (const auto &fixedGate : fixedGatesList) {
        size_t cnt = 0;
        auto ins = ac.Ins(fixedGate);
        for (auto i = ins.begin(); i != ins.end(); i++) {
            GateRef predGate = *i;
            if (ac.IsFixed(predGate) &&
                // 2: Judge equality
                (circuit->GetOpCode(circuit->GetIn(fixedGate, 0)) == OpCode::LOOP_BEGIN && cnt == 2)) {
                ASSERT(cnt > 0);
                auto a = bbGatesAddrToIdx.at(circuit->GetIn(predGate, 0));
                auto b = bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(fixedGate, 0),
                    static_cast<size_t>(cnt - 1)));
                if (!isAncestor(a, b)) {
                    LOG_COMPILER(ERROR) << "[Verifier][Error] Fixed gates relationship is not consistent";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "Fixed gate (id="
                                        << circuit->GetId(predGate)
                                        << ") is pred of fixed gate (id="
                                        << circuit->GetId(fixedGate) << ")";
                    LOG_COMPILER(ERROR) << "BB_" << bbGatesAddrToIdx.at(circuit->GetIn(predGate, 0))
                                        << " does not dominate BB_"
                                        << bbGatesAddrToIdx.at(circuit->GetIn(circuit->GetIn(fixedGate, 0),
                                            static_cast<size_t>(cnt - 1)));
                    return false;
                }
            }
            cnt++;
        }
    }
    return true;
}

bool Verifier::RunFlowCyclesFind(const Circuit *circuit, std::vector<GateRef> *schedulableGatesListPtr,
    const std::vector<GateRef> &bbGatesList, const std::vector<GateRef> &fixedGatesList)
{
    circuit->AdvanceTime();
    ConstGateAccessor ac(circuit);
    std::vector<GateRef> startGateList;
    for (const auto &gate : bbGatesList) {
        auto ins = ac.Ins(gate);
        for (auto i = ins.begin(); i != ins.end(); i++) {
            GateRef predGate = *i;
            if (ac.IsSchedulable(predGate)) {
                if (circuit->GetMark(predGate) == MarkCode::NO_MARK) {
                    startGateList.push_back(predGate);
                    circuit->SetMark(predGate, MarkCode::VISITED);
                }
            }
        }
    }
    for (const auto &gate : fixedGatesList) {
        auto ins = ac.Ins(gate);
        for (auto i = ins.begin(); i != ins.end(); i++) {
            GateRef predGate = *i;
            if (ac.IsSchedulable(predGate)) {
                if (circuit->GetMark(predGate) == MarkCode::NO_MARK) {
                    startGateList.push_back(predGate);
                    circuit->SetMark(predGate, MarkCode::VISITED);
                }
            }
        }
    }
    circuit->AdvanceTime();
    GateAccessor gateAcc(const_cast<Circuit *>(circuit));
    std::vector<GateRef> cycleGatesList;
    GateRef meet = -1;
    struct DFSState {
        GateRef cur;
        size_t numIns;
        size_t idx;
    };
    for (const auto &startGate : startGateList) {
        if (!gateAcc.IsNotMarked(startGate)) {
            continue;
        }
        std::stack<DFSState> dfsStack;
        size_t startNumIns = gateAcc.GetNumIns(startGate);
        dfsStack.push({startGate, startNumIns, 0});
        gateAcc.SetVisited(startGate);
        schedulableGatesListPtr->push_back(startGate);
        while (!dfsStack.empty()) {
            auto &curState = dfsStack.top();
            auto &cur = curState.cur;
            auto &numIns = curState.numIns;
            auto &idx = curState.idx;
            if (idx == numIns) {
                gateAcc.SetFinished(cur);
                dfsStack.pop();
                continue;
            }
            const auto prev = gateAcc.GetIn(cur, idx);
            if (gateAcc.IsSchedulable(prev)) {
                if (gateAcc.IsVisited(prev)) {
                    LOG_COMPILER(ERROR) <<
                        "[Verifier][Error] Found a data or depend flow cycle without passing selectors";
                    LOG_COMPILER(ERROR) << "Proof:";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(prev) << ") is prev of "
                            << "(id=" << circuit->GetId(cur) << ")";
                    LOG_COMPILER(ERROR) << "(id=" << circuit->GetId(prev) << ") is reachable from "
                            << "(id=" << circuit->GetId(cur) << ") without passing selectors";
                    meet = prev;
                    break;
                }
                if (!gateAcc.IsFinished(prev)) {
                    size_t newNumIns = gateAcc.GetNumIns(prev);
                    dfsStack.push({prev, newNumIns, 0});
                    gateAcc.SetVisited(prev);
                    schedulableGatesListPtr->push_back(prev);
                }
            }
            idx++;
        }
        if (meet != -1) {
            while (dfsStack.top().cur != meet) {
                cycleGatesList.push_back(dfsStack.top().cur);
                dfsStack.pop();
            }
            cycleGatesList.push_back(meet);
            LOG_COMPILER(ERROR) << "Path:";
            for (const auto &cycleGate : cycleGatesList) {
                gateAcc.Print(cycleGate);
            }
            return false;
        }
    }
    return true;
}

bool Verifier::RunSchedulableGatesCheck(const Circuit *circuit, const std::vector<GateRef> &schedulableGatesList)
{
    for (const auto &schedulableGate : schedulableGatesList) {
        circuit->Verify(schedulableGate);
    }
    return true;
}

bool Verifier::RunPrologGatesCheck(const Circuit *circuit, const std::vector<GateRef> &schedulableGatesList)
{
    ConstGateAccessor ac(circuit);
    for (const auto &schedulableGate : schedulableGatesList) {
        auto ins = ac.Ins(schedulableGate);
        for (auto i = ins.begin(); i != ins.end(); i++) {
            GateRef r = *i;
            if (ac.IsProlog(r)) {
                circuit->Verify(r);
            }
        }
    }
    return true;
}

bool Verifier::RunSchedulingBoundsCheck(const Circuit *circuit,
                                        const std::vector<GateRef> &schedulableGatesList,
                                        const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                        const std::function<bool(size_t, size_t)> &isAncestor,
                                        const std::function<size_t(size_t, size_t)> &lowestCommonAncestor)
{
    // check existence of scheduling upper bound
    std::unordered_map<GateRef, size_t> upperBound;
    {
        if (!Scheduler::CalculateSchedulingUpperBound(circuit, bbGatesAddrToIdx, isAncestor,
                                                      schedulableGatesList, upperBound)) {
            return false;
        }
    }
    // check existence of scheduling lower bound
    std::unordered_map<GateRef, size_t> lowerBound;
    {
        Scheduler::CalculateSchedulingLowerBound(circuit, bbGatesAddrToIdx, lowestCommonAncestor, lowerBound);
    }
    // check consistency of lower bound and upper bound
    {
        ASSERT(upperBound.size() == lowerBound.size());
        for (const auto &item : lowerBound) {
            if (!isAncestor(upperBound.at(item.first), lowerBound.at(item.first))) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Bounds of gate (id="
                                    << circuit->GetId(item.first) << ") is not consistent";
                LOG_COMPILER(ERROR) << "Proof:";
                LOG_COMPILER(ERROR) << "Upper bound is BB_" << upperBound.at(item.first);
                LOG_COMPILER(ERROR) << "Lower bound is BB_" << lowerBound.at(item.first);
                return false;
            }
        }
    }
    return true;
}

void Verifier::FindFixedGates(const Circuit *circuit, const std::vector<GateRef> &bbGatesList,
                              std::vector<GateRef> &fixedGatesList)
{
    ConstGateAccessor ac(circuit);
    for (const auto &bbGate : bbGatesList) {
        for (const auto &succGate : circuit->GetOutVector(bbGate)) {
            if (ac.IsFixed(succGate)) {
                fixedGatesList.push_back(succGate);
            }
        }
    }
}

bool Verifier::RunFlowCyclesFind(const Circuit* circuit)
{
    GateAccessor acc(const_cast<Circuit *>(circuit));
    circuit->AdvanceTime();
    struct DFSStack {
        GateRef gate;
        GateAccessor::UseIterator it;
        GateAccessor::UseIterator end;
    };
    std::stack<DFSStack> st;
    auto root = acc.GetCircuitRoot();
    auto rootUse = acc.Uses(root);
    st.push({root, rootUse.begin(), rootUse.end()});
    acc.SetVisited(root);
    while (!st.empty()) {
        auto& cur = st.top();
        if (cur.it == cur.end) {
            st.pop();
            acc.SetFinished(cur.gate);
            continue;
        }
        auto succ = *cur.it;
        if (acc.IsLoopBackUse(cur.gate, cur.it)) {
            cur.it++;
            continue;
        }
        if (acc.IsNotMarked(succ)) {
            auto succUse = acc.Uses(succ);
            st.push({succ, succUse.begin(), succUse.end()});
            acc.SetVisited(succ);
        } else if (acc.IsVisited(succ)) {
            LOG_COMPILER(ERROR) << "====================== Cycle Found ======================";
            std::string log = "";
            while (st.top().gate != succ) {
                log += std::to_string(acc.GetId(st.top().gate)) + " < ";
                st.pop();
            }
            log += std::to_string(acc.GetId(st.top().gate));
            LOG_COMPILER(ERROR) << log;
            return true;
        }
        cur.it++;
    }
    return false;
}

bool Verifier::Run(const Circuit *circuit, const std::string& methodName, bool enableLog)
{
    if (!RunDataIntegrityCheck(circuit)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] Circuit data integrity verifier failed, " << methodName;
        }
        return false;
    }
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    Scheduler::CalculateDominatorTree(circuit, bbGatesList, bbGatesAddrToIdx, immDom);
    if (!RunStateGatesCheck(circuit, bbGatesList, methodName)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunStateGatesCheck failed";
        }
        return false;
    }
    if (!RunCFGSoundnessCheck(circuit, bbGatesList, bbGatesAddrToIdx)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunCFGSoundnessCheck failed";
        }
        return false;
    }
    if (!RunCFGIsDAGCheck(circuit)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunCFGIsDAGCheck failed";
        }
        return false;
    }
    std::vector<std::vector<size_t>> sonList(bbGatesList.size());
    for (size_t idx = 1; idx < immDom.size(); idx++) {
        sonList[immDom[idx]].push_back(idx);
    }
    const size_t sizeLog = std::ceil(std::log2(static_cast<double>(bbGatesList.size())) + 1);
    std::vector<size_t> timeIn(bbGatesList.size());
    std::vector<size_t> timeOut(bbGatesList.size());
    std::vector<std::vector<size_t>> jumpUp;
    jumpUp.assign(bbGatesList.size(), std::vector<size_t>(sizeLog + 1));
    {
        size_t timestamp = 0;
        struct DFSState {
            size_t cur;
            std::vector<size_t> &succList;
            size_t idx;
        };
        std::stack<DFSState> dfsStack;
        size_t root = 0;
        dfsStack.push({root, sonList[root], 0});
        timeIn[root] = timestamp++;
        jumpUp[root][0] = root;
        for (size_t stepSize = 1; stepSize <= sizeLog; stepSize++) {
            auto jumpUpHalf = jumpUp[root][stepSize - 1];
            jumpUp[root][stepSize] = jumpUp[jumpUpHalf][stepSize - 1];
        }
        while (!dfsStack.empty()) {
            auto &curState = dfsStack.top();
            auto &cur = curState.cur;
            auto &succList = curState.succList;
            auto &idx = curState.idx;
            if (idx == succList.size()) {
                timeOut[cur] = timestamp++;
                dfsStack.pop();
                continue;
            }
            const auto &succ = succList[idx];
            dfsStack.push({succ, sonList[succ], 0});
            timeIn[succ] = timestamp++;
            jumpUp[succ][0] = cur;
            for (size_t stepSize = 1; stepSize <= sizeLog; stepSize++) {
                auto jumpUpHalf = jumpUp[succ][stepSize - 1];
                jumpUp[succ][stepSize] = jumpUp[jumpUpHalf][stepSize - 1];
            }
            idx++;
        }
    }
    auto isAncestor = [timeIn, timeOut](size_t nodeA, size_t nodeB) -> bool {
        return timeIn[nodeA] <= timeIn[nodeB] && timeOut[nodeA] >= timeOut[nodeB];
    };
    auto lowestCommonAncestor = [&](size_t nodeA, size_t nodeB) -> size_t {
        if (isAncestor(nodeA, nodeB)) {
            return nodeA;
        }
        if (isAncestor(nodeB, nodeA)) {
            return nodeB;
        }
        for (size_t stepSize = sizeLog + 1; stepSize > 0; stepSize--) {
            if (!isAncestor(jumpUp[nodeA][stepSize - 1], nodeB)) {
                nodeA = jumpUp[nodeA][stepSize - 1];
            }
        }
        return jumpUp[nodeA][0];
    };
    if (!RunCFGReducibilityCheck(circuit, bbGatesList, bbGatesAddrToIdx, isAncestor)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunCFGReducibilityCheck failed";
        }
        return false;
    }
    std::vector<GateRef> fixedGatesList;
    FindFixedGates(circuit, bbGatesList, fixedGatesList);
    if (!RunFixedGatesCheck(circuit, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunFixedGatesCheck failed";
        }
        return false;
    }
    if (!RunFixedGatesRelationsCheck(circuit, fixedGatesList, bbGatesAddrToIdx, isAncestor)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunFixedGatesRelationsCheck failed";
        }
        return false;
    }
    std::vector<GateRef> schedulableGatesList;
    if (!RunFlowCyclesFind(circuit, &schedulableGatesList, bbGatesList, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunFlowCyclesFind failed";
        }
        return false;
    }
    if (!RunSchedulableGatesCheck(circuit, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunSchedulableGatesCheck failed";
        }
        return false;
    }
    if (!RunPrologGatesCheck(circuit, fixedGatesList)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunPrologGatesCheck failed";
        }
        return false;
    }
    if (!RunSchedulingBoundsCheck(circuit, schedulableGatesList, bbGatesAddrToIdx, isAncestor, lowestCommonAncestor)) {
        if (enableLog) {
            LOG_COMPILER(ERROR) << "[Verifier][Fail] RunSchedulingBoundsCheck failed";
        }
        return false;
    }

    if (enableLog) {
        LOG_COMPILER(INFO) << "[Verifier][Pass] Verifier success";
    }

    return true;
}
}  // namespace panda::ecmascript::kungfu
