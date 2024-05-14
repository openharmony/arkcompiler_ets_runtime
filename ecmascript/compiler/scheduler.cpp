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

#include "ecmascript/compiler/scheduler.h"
#include <cmath>
#include <stack>
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/verifier.h"

namespace panda::ecmascript::kungfu {
size_t UnionFind(std::vector<size_t> &semiDom, std::vector<size_t> &parent, std::vector<size_t> &minIdx, size_t idx)
{
    std::stack<size_t> allIdxs;
    allIdxs.emplace(idx);
    size_t pIdx = parent[idx];
    while (pIdx != allIdxs.top()) {
        allIdxs.emplace(pIdx);
        pIdx = parent[pIdx];
    }
    size_t unionFindSetRoot = pIdx;
    while (!allIdxs.empty()) {
        if (semiDom[minIdx[allIdxs.top()]] > semiDom[minIdx[pIdx]]) {
            minIdx[allIdxs.top()] = minIdx[pIdx];
        }
        parent[allIdxs.top()] = unionFindSetRoot;
        pIdx = allIdxs.top();
        allIdxs.pop();
    }
    return unionFindSetRoot;
}

void Scheduler::CalculateDominatorTree(const Circuit *circuit,
                                       std::vector<GateRef>& bbGatesList,
                                       std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                       std::vector<size_t> &immDom)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::unordered_map<GateRef, size_t> dfsTimestamp;
    std::unordered_map<GateRef, size_t> dfsFatherIdx;
    circuit->AdvanceTime();
    {
        size_t timestamp = 0;
        std::deque<GateRef> pendingList;
        auto startGate = acc.GetStateRoot();
        acc.SetMark(startGate, MarkCode::VISITED);
        pendingList.push_back(startGate);
        while (!pendingList.empty()) {
            auto curGate = pendingList.back();
            dfsTimestamp[curGate] = timestamp++;
            pendingList.pop_back();
            bbGatesList.push_back(curGate);
            if (acc.GetOpCode(curGate) != OpCode::LOOP_BACK) {
                auto uses = acc.Uses(curGate);
                for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                    if (useIt.GetIndex() < acc.GetStateCount(*useIt) &&
                        acc.IsState(*useIt) && acc.GetMark(*useIt) == MarkCode::NO_MARK) {
                        acc.SetMark(*useIt, MarkCode::VISITED);
                        pendingList.push_back(*useIt);
                        dfsFatherIdx[*useIt] = dfsTimestamp[curGate];
                    }
                }
            }
        }
        for (size_t idx = 0; idx < bbGatesList.size(); idx++) {
            bbGatesAddrToIdx[bbGatesList[idx]] = idx;
        }
    }
    immDom.resize(bbGatesList.size());
    std::vector<size_t> semiDom(bbGatesList.size());
    std::vector<std::vector<size_t> > semiDomTree(bbGatesList.size());
    {
        std::vector<size_t> parent(bbGatesList.size());
        std::iota(parent.begin(), parent.end(), 0);
        std::vector<size_t> minIdx(bbGatesList.size());
        auto merge = [&] (size_t fatherIdx, size_t sonIdx) -> void {
            size_t parentFatherIdx = UnionFind(semiDom, parent, minIdx, fatherIdx);
            size_t parentSonIdx = UnionFind(semiDom, parent, minIdx, sonIdx);
            parent[parentSonIdx] = parentFatherIdx;
        };
        std::iota(semiDom.begin(), semiDom.end(), 0);
        semiDom[0] = semiDom.size();
        ASSERT(bbGatesList.size() > 0);
        for (size_t idx = bbGatesList.size() - 1; idx >= 1; idx--) {
            std::vector<GateRef> preGates;
            acc.GetInStates(bbGatesList[idx], preGates);
            for (const auto &predGate : preGates) {
                if (bbGatesAddrToIdx.count(predGate) > 0) {
                    size_t preGateIdx = bbGatesAddrToIdx[predGate];
                    if (preGateIdx < idx) {
                        semiDom[idx] = std::min(semiDom[idx], preGateIdx);
                    } else {
                        UnionFind(semiDom, parent, minIdx, preGateIdx);
                        semiDom[idx] = std::min(semiDom[idx], semiDom[minIdx[preGateIdx]]);
                    }
                }
            }
            for (const auto &succDomIdx : semiDomTree[idx]) {
                UnionFind(semiDom, parent, minIdx, succDomIdx);
                if (idx == semiDom[minIdx[succDomIdx]]) {
                    immDom[succDomIdx] = idx;
                } else {
                    immDom[succDomIdx] = minIdx[succDomIdx];
                }
            }
            minIdx[idx] = idx;
            merge(dfsFatherIdx[bbGatesList[idx]], idx);
            semiDomTree[semiDom[idx]].push_back(idx);
        }
        for (size_t idx = 1; idx < bbGatesList.size(); idx++) {
            if (immDom[idx] != semiDom[idx]) {
                immDom[idx] = immDom[immDom[idx]];
            }
        }
        semiDom[0] = 0;
    }
}

void Scheduler::Run(const Circuit *circuit, ControlFlowGraph &result,
                    [[maybe_unused]] const std::string& methodName, [[maybe_unused]] bool enableLog)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    Scheduler::CalculateDominatorTree(circuit, bbGatesList, bbGatesAddrToIdx, immDom);
    ASSERT(result.size() == 0);
    result.resize(bbGatesList.size());
    for (size_t idx = 0; idx < bbGatesList.size(); idx++) {
        result[idx].push_back(bbGatesList[idx]);
    }
    // assuming CFG is always reducible
    std::vector<std::vector<size_t>> sonList(result.size());
    for (size_t idx = 1; idx < immDom.size(); idx++) {
        sonList[immDom[idx]].push_back(idx);
    }
    const size_t sizeLog = std::ceil(std::log2(static_cast<double>(result.size())) + 1);
    std::vector<size_t> timeIn(result.size());
    std::vector<size_t> timeOut(result.size());
    std::vector<std::vector<size_t>> jumpUp;
    jumpUp.assign(result.size(), std::vector<size_t>(sizeLog + 1));
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
    auto isAncestor = [&](size_t nodeA, size_t nodeB) -> bool {
        return (timeIn[nodeA] <= timeIn[nodeB]) && (timeOut[nodeA] >= timeOut[nodeB]);
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
    {
        std::vector<GateRef> order;
        std::unordered_map<GateRef, size_t> lowerBound;
        Scheduler::CalculateSchedulingLowerBound(circuit, bbGatesAddrToIdx, lowestCommonAncestor, lowerBound, &order);
        for (const auto &schedulableGate : order) {
            result[lowerBound.at(schedulableGate)].push_back(schedulableGate);
        }
        std::vector<GateRef> argList;
        acc.GetOuts(acc.GetArgRoot(), argList);
        std::sort(argList.begin(), argList.end(), [&](const GateRef &lhs, const GateRef &rhs) -> bool {
            return acc.TryGetValue(lhs) > acc.TryGetValue(rhs);
        });
        for (const auto &arg : argList) {
            result.front().push_back(arg);
        }
        for (const auto &bbGate : bbGatesList) {
            auto uses = acc.Uses(bbGate);
            for (auto i = uses.begin(); i != uses.end(); i++) {
                GateRef succGate = *i;
                if (acc.IsFixed(succGate)) {
                    result[bbGatesAddrToIdx.at(acc.GetIn(succGate, 0))].push_back(succGate);
                }
            }
        }
    }
    if (enableLog) {
        Print(&result, circuit);
    }
}

bool Scheduler::CalculateSchedulingUpperBound(const Circuit *circuit,
                                              const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                              const std::function<bool(size_t, size_t)> &isAncestor,
                                              const std::vector<GateRef> &schedulableGatesList,
                                              std::unordered_map<GateRef, size_t> &upperBound)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    struct DFSState {
        GateRef curGate = Circuit::NullGate();
        std::vector<GateRef> predGates;
        size_t idx = 0;
        size_t curUpperBound = 0;
    };
    DFSState emptyState = {Circuit::NullGate(), std::vector<GateRef>(0), 0, 0};
    bool getReturn = false;
    std::optional<size_t> returnValue = 0;
    std::stack<DFSState> dfsStack;
    auto CheckUnschedulable = [&](GateRef gate) -> void {
        if (upperBound.count(gate) > 0) {
            returnValue = upperBound[gate];
            getReturn = true;
        } else if (acc.IsProlog(gate) || acc.IsRoot(gate) || acc.IsVirtualState(gate)) {
            returnValue = 0;
            getReturn = true;
        } else if (acc.IsFixed(gate)) {
            returnValue = bbGatesAddrToIdx.at(acc.GetIn(gate, 0));
            getReturn = true;
        } else if (acc.IsState(gate)) {
            returnValue = bbGatesAddrToIdx.at(gate);
            getReturn = true;
        }
        // then gate is schedulable
    };
    for (const auto &schedulableGate : schedulableGatesList) {
        if (upperBound.count(schedulableGate) != 0) {
            continue;
        }
        getReturn = false;
        CheckUnschedulable(schedulableGate);
        if (getReturn) {
            continue;
        }
        dfsStack.push(emptyState);
        auto &rootState = dfsStack.top();
        auto &rootPredGates = rootState.predGates;
        rootState.curGate = schedulableGate;
        acc.GetIns(schedulableGate, rootPredGates);
        while (!dfsStack.empty()) {
            auto &curState = dfsStack.top();
            auto &curGate = curState.curGate;
            const auto &predGates = curState.predGates;
            auto &idx = curState.idx;
            auto &curUpperBound = curState.curUpperBound;
            if (idx == predGates.size()) {
                upperBound[curGate] = curUpperBound;
                returnValue = curUpperBound;
                dfsStack.pop();
                getReturn = true;
                continue;
            }
            if (getReturn) {
                if (!returnValue.has_value()) {
                    break;
                }
                auto predUpperBound = returnValue.value();
                if (!isAncestor(curUpperBound, predUpperBound) && !isAncestor(predUpperBound, curUpperBound)) {
                    PrintUpperBoundError(circuit, curGate, predUpperBound, curUpperBound);
                    returnValue = std::nullopt;
                    break;
                }
                if (isAncestor(curUpperBound, predUpperBound)) {
                    curUpperBound = predUpperBound;
                }
                getReturn = false;
                idx++;
            } else {
                const auto &predGate = predGates[idx];
                CheckUnschedulable(predGate);
                if (getReturn) {
                    continue;
                }
                dfsStack.push(emptyState);
                auto &newState = dfsStack.top();
                auto &newPredGates = newState.predGates;
                newState.curGate = predGate;
                acc.GetIns(predGate, newPredGates);
            }
        }
        if (!returnValue.has_value()) {
            return false;
        }
    }
    return true;
}

void Scheduler::PrintUpperBoundError(const Circuit *circuit, GateRef curGate,
                                     GateRef predUpperBound, GateRef curUpperBound)
{
    GateAccessor ac(const_cast<Circuit*>(circuit));
    LOG_COMPILER(ERROR) << "[Verifier][Error] Scheduling upper bound of gate (id="
                        << ac.GetId(curGate)
                        << ") does not exist, current-upper-bound = "
                        << curUpperBound << ", pred-upper-bound = "
                        << predUpperBound << ", there is no dominator relationship between them.";
}

void Scheduler::CalculateFixedGatesList(const Circuit *circuit,
                                        const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                        std::vector<GateRef> &bbAndFixedGatesList)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    for (const auto &item : bbGatesAddrToIdx) {
        bbAndFixedGatesList.push_back(item.first);
        auto uses = acc.Uses(item.first);
        for (auto i = uses.begin(); i != uses.end(); i++) {
            GateRef succGate = *i;
            if (acc.IsFixed(succGate)) {
                bbAndFixedGatesList.push_back(succGate);
            }
        }
    }
}

void Scheduler::CalculateSchedulingLowerBound(const Circuit *circuit,
                                              const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                              const std::function<size_t(size_t, size_t)> &lowestCommonAncestor,
                                              std::unordered_map<GateRef, size_t> &lowerBound,
                                              std::vector<GateRef> *order)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::vector<GateRef> bbAndFixedGatesList;
    CalculateFixedGatesList(circuit, bbGatesAddrToIdx, bbAndFixedGatesList);
    std::unordered_map<GateRef, size_t> useCount;
    struct DFSVisitState {
        std::vector<GateRef> prevGates;
        size_t idx = 0;
    };
    DFSVisitState emptyVisitState = {std::vector<GateRef>(0), 0};
    std::stack<DFSVisitState> dfsVisitStack;
    for (const auto &gate : bbAndFixedGatesList) {
        dfsVisitStack.push(emptyVisitState);
        auto &rootState = dfsVisitStack.top();
        auto &rootPrevGates = rootState.prevGates;
        acc.GetIns(gate, rootPrevGates);
        while (!dfsVisitStack.empty()) {
            auto &curState = dfsVisitStack.top();
            auto &prevGates = curState.prevGates;
            auto &idx = curState.idx;
            if (idx == prevGates.size()) {
                dfsVisitStack.pop();
                continue;
            }
            const auto &prevGate = prevGates[idx];
            if (!acc.IsSchedulable(prevGate)) {
                ++idx;
                continue;
            }
            useCount[prevGate]++;
            if (useCount[prevGate] == 1) {
                dfsVisitStack.push(emptyVisitState);
                auto &newState = dfsVisitStack.top();
                auto &newPrevGates = newState.prevGates;
                acc.GetIns(prevGate, newPrevGates);
            }
            ++idx;
        }
    }
    struct DFSFinishState {
        GateRef curGate = Circuit::NullGate();
        std::vector<GateRef> prevGates;
        size_t idx = 0;
    };
    DFSFinishState emptyFinishState = {Circuit::NullGate(), std::vector<GateRef>(0), 0};
    std::stack<DFSFinishState> dfsFinishStack;
    for (const auto &gate : bbAndFixedGatesList) {
        dfsFinishStack.push(emptyFinishState);
        auto &rootState = dfsFinishStack.top();
        auto &rootPrevGates = rootState.prevGates;
        rootState.curGate = gate;
        acc.GetIns(gate, rootPrevGates);
        while (!dfsFinishStack.empty()) {
            auto &curState = dfsFinishStack.top();
            auto &curGate = curState.curGate;
            auto &prevGates = curState.prevGates;
            auto &idx = curState.idx;
            if (idx == prevGates.size()) {
                dfsFinishStack.pop();
                continue;
            }
            const auto &prevGate = prevGates[idx];
            if (!acc.IsSchedulable(prevGate)) {
                ++idx;
                continue;
            }
            useCount[prevGate]--;
            size_t curLowerBound;
            if (acc.IsState(curGate)) {  // cur_opcode would not be STATE_ENTRY
                curLowerBound = bbGatesAddrToIdx.at(curGate);
            } else if (acc.IsSelector(curGate)) {
                ASSERT(idx > 0);
                curLowerBound = bbGatesAddrToIdx.at(acc.GetIn(acc.GetIn(curGate, 0), idx - 1));
            } else if (acc.IsFixed(curGate)) {
                ASSERT(idx > 0);
                curLowerBound = bbGatesAddrToIdx.at(acc.GetIn(curGate, 0));
            } else {
                curLowerBound = lowerBound.at(curGate);
            }
            if (lowerBound.count(prevGate) == 0) {
                lowerBound[prevGate] = curLowerBound;
            } else {
                lowerBound[prevGate] = lowestCommonAncestor(lowerBound[prevGate], curLowerBound);
            }
            if (useCount[prevGate] == 0) {
                if (order != nullptr) {
                    order->push_back(prevGate);
                }
                dfsFinishStack.push(emptyFinishState);
                auto &newState = dfsFinishStack.top();
                auto &newPrevGates = newState.prevGates;
                newState.curGate = prevGate;
                acc.GetIns(prevGate, newPrevGates);
            }
            ++idx;
        }
    }
}

void Scheduler::Print(const std::vector<std::vector<GateRef>> *cfg, const Circuit *circuit)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    Scheduler::CalculateDominatorTree(circuit, bbGatesList, bbGatesAddrToIdx, immDom);
    LOG_COMPILER(INFO) << "==================================== Scheduling ==================================";
    for (size_t bbIdx = 0; bbIdx < cfg->size(); bbIdx++) {
        auto opcode = acc.GetOpCode((*cfg)[bbIdx].front());
        LOG_COMPILER(INFO) << "B" << bbIdx << "_" << opcode << ":"
                           << "  immDom=" << immDom[bbIdx];
        LOG_COMPILER(INFO) << "  pred=[";
        bool isFirst = true;
        GateRef head = cfg->at(bbIdx).front();
        auto ins = acc.Ins(head);
        for (auto i = ins.begin(); i != ins.end(); i++) {
            GateRef predState = *i;
            if (acc.IsState(predState) ||
                acc.GetOpCode(predState) == OpCode::STATE_ENTRY) {
                LOG_COMPILER(INFO) << (isFirst ? "" : " ") << bbGatesAddrToIdx.at(predState);
                isFirst = false;
            }
        }
        LOG_COMPILER(INFO) << "]  succ=[";
        isFirst = true;
        GateRef h = cfg->at(bbIdx).front();
        auto uses = acc.Uses(h);
        for (auto i = uses.begin(); i != uses.end(); i++) {
            GateRef succState = *i;
            if (acc.IsState(succState) ||
                acc.GetOpCode(succState) == OpCode::STATE_ENTRY) {
                LOG_COMPILER(INFO) << (isFirst ? "" : " ") << bbGatesAddrToIdx.at(succState);
                isFirst = false;
            }
        }
        LOG_COMPILER(INFO) << "]";
        for (size_t instIdx = (*cfg)[bbIdx].size(); instIdx > 0; instIdx--) {
            acc.Print((*cfg)[bbIdx][instIdx - 1]);
        }
    }
    LOG_COMPILER(INFO) << "==================================== Scheduling ==================================";
}
}  // namespace panda::ecmascript::kungfu
