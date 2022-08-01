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
#include <cmath>

#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/scheduler.h"

namespace panda::ecmascript::kungfu {
using DominatorTreeInfo = std::tuple<std::vector<GateRef>, std::unordered_map<GateRef, size_t>,
    std::vector<size_t>>;
DominatorTreeInfo Scheduler::CalculateDominatorTree(const Circuit *circuit)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::unordered_map<GateRef, size_t> dfsTimestamp;
    std::unordered_map<GateRef, size_t> dfsFatherIdx;
    circuit->AdvanceTime();
    {
        size_t timestamp = 0;
        std::deque<GateRef> pendingList;
        auto startGate = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
        acc.SetMark(startGate, MarkCode::VISITED);
        pendingList.push_back(startGate);
        while (!pendingList.empty()) {
            auto curGate = pendingList.back();
            dfsTimestamp[curGate] = timestamp++;
            pendingList.pop_back();
            bbGatesList.push_back(curGate);
            if (acc.GetOpCode(curGate) != OpCode::LOOP_BACK) {
                std::vector<GateRef> succGates;
                acc.GetOutVector(curGate, succGates);
                for (const auto &succGate : succGates) {
                    if (acc.GetOpCode(succGate).IsState() && acc.GetMark(succGate) == MarkCode::NO_MARK) {
                        acc.SetMark(succGate, MarkCode::VISITED);
                        pendingList.push_back(succGate);
                        dfsFatherIdx[succGate] = dfsTimestamp[curGate];
                    }
                }
            }
        }
        for (size_t idx = 0; idx < bbGatesList.size(); idx++) {
            bbGatesAddrToIdx[bbGatesList[idx]] = idx;
        }
    }
    std::vector<size_t> immDom(bbGatesList.size());
    std::vector<size_t> semiDom(bbGatesList.size());
    std::vector<std::vector<size_t> > semiDomTree(bbGatesList.size());
    {
        std::vector<size_t> parent(bbGatesList.size());
        std::iota(parent.begin(), parent.end(), 0);
        std::vector<size_t> minIdx(bbGatesList.size());
        std::function<size_t(size_t)> unionFind = [&] (size_t idx) -> size_t {
            if (parent[idx] == idx) return idx;
            size_t unionFindSetRoot = unionFind(parent[idx]);
            if (semiDom[minIdx[idx]] > semiDom[minIdx[parent[idx]]]) {
                minIdx[idx] = minIdx[parent[idx]];
            }
            return parent[idx] = unionFindSetRoot;
        };
        auto merge = [&] (size_t fatherIdx, size_t sonIdx) -> void {
            size_t parentFatherIdx = unionFind(fatherIdx);
            size_t parentSonIdx = unionFind(sonIdx);
            parent[parentSonIdx] = parentFatherIdx;
        };
        std::iota(semiDom.begin(), semiDom.end(), 0);
        semiDom[0] = semiDom.size();
        for (size_t idx = bbGatesList.size() - 1; idx >= 1; idx --) {
            std::vector<GateRef> preGates;
            acc.GetInVector(bbGatesList[idx], preGates);
            for (const auto &predGate : preGates) {
                if (bbGatesAddrToIdx.count(predGate) > 0) {
                    if (bbGatesAddrToIdx[predGate] < idx) {
                        semiDom[idx] = std::min(semiDom[idx], bbGatesAddrToIdx[predGate]);
                    } else {
                        unionFind(bbGatesAddrToIdx[predGate]);
                        semiDom[idx] = std::min(semiDom[idx], semiDom[minIdx[bbGatesAddrToIdx[predGate]]]);
                    }
                }
            }
            for (const auto &succDomIdx : semiDomTree[idx]) {
                unionFind(succDomIdx);
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
    return {bbGatesList, bbGatesAddrToIdx, immDom};
}

std::vector<std::vector<GateRef>> Scheduler::Run(const Circuit *circuit, [[maybe_unused]] bool enableLog)
{
#ifndef NDEBUG
    if (!Verifier::Run(circuit, enableLog)) {
        UNREACHABLE();
    }
#endif
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit);
    std::vector<std::vector<GateRef>> result(bbGatesList.size());
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
        std::function<void(size_t, size_t)> dfs = [&](size_t cur, size_t prev) {
            timeIn[cur] = timestamp;
            timestamp++;
            jumpUp[cur][0] = prev;
            for (size_t stepSize = 1; stepSize <= sizeLog; stepSize++) {
                jumpUp[cur][stepSize] = jumpUp[jumpUp[cur][stepSize - 1]][stepSize - 1];
            }
            for (const auto &succ : sonList[cur]) {
                dfs(succ, cur);
            }
            timeOut[cur] = timestamp;
            timestamp++;
        };
        size_t root = 0;
        dfs(root, root);
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
        auto lowerBound =
            Scheduler::CalculateSchedulingLowerBound(circuit, bbGatesAddrToIdx, lowestCommonAncestor, &order).value();
        for (const auto &schedulableGate : order) {
            result[lowerBound.at(schedulableGate)].push_back(schedulableGate);
        }
        std::vector<GateRef> argList;
        acc.GetOutVector(Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST)), argList);
        std::sort(argList.begin(), argList.end(), [&](const GateRef &lhs, const GateRef &rhs) -> bool {
            return acc.GetBitField(lhs) > acc.GetBitField(rhs);
        });
        for (const auto &arg : argList) {
            result.front().push_back(arg);
        }
        for (const auto &bbGate : bbGatesList) {
            std::vector<GateRef> succGates;
            acc.GetOutVector(bbGate, succGates);
            for (const auto &succGate : succGates) {
                if (acc.GetOpCode(succGate).IsFixed()) {
                    result[bbGatesAddrToIdx.at(acc.GetIn(succGate, 0))].push_back(succGate);
                }
            }
        }
    }
    return result;
}

std::optional<std::unordered_map<GateRef, size_t>> Scheduler::CalculateSchedulingUpperBound(const Circuit *circuit,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<bool(size_t, size_t)> &isAncestor, const std::vector<GateRef> &schedulableGatesList)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::unordered_map<GateRef, size_t> upperBound;
    std::function<std::optional<size_t>(GateRef)> dfs = [&](GateRef curGate) -> std::optional<size_t> {
        if (upperBound.count(curGate) > 0) {
            return upperBound[curGate];
        }
        if (acc.GetOpCode(curGate).IsProlog() || acc.GetOpCode(curGate).IsRoot()) {
            return 0;
        }
        if (acc.GetOpCode(curGate).IsFixed()) {
            return bbGatesAddrToIdx.at(acc.GetIn(curGate, 0));
        }
        if (acc.GetOpCode(curGate).IsState()) {
            return bbGatesAddrToIdx.at(curGate);
        }
        // then cur is schedulable
        size_t curUpperBound = 0;
        std::vector<GateRef> predGates;
        acc.GetInVector(curGate, predGates);
        for (const auto &predGate : predGates) {
            auto predResult = dfs(predGate);
            if (!predResult.has_value()) {
                return std::nullopt;
            }
            auto predUpperBound = predResult.value();
            if (!isAncestor(curUpperBound, predUpperBound) && !isAncestor(predUpperBound, curUpperBound)) {
                LOG_COMPILER(ERROR) << "[Verifier][Error] Scheduling upper bound of gate (id="
                                    << GateAccessor(const_cast<Circuit*>(circuit)).GetId(curGate) << ") does not exist";
                return std::nullopt;
            }
            if (isAncestor(curUpperBound, predUpperBound)) {
                curUpperBound = predUpperBound;
            }
        }
        return (upperBound[curGate] = curUpperBound);
    };
    for (const auto &schedulableGate : schedulableGatesList) {
        if (upperBound.count(schedulableGate) == 0) {
            if (!dfs(schedulableGate).has_value()) {
                return std::nullopt;
            }
        }
    }
    return upperBound;
}

std::optional<std::unordered_map<GateRef, size_t>> Scheduler::CalculateSchedulingLowerBound(const Circuit *circuit,
    const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
    const std::function<size_t(size_t, size_t)> &lowestCommonAncestor, std::vector<GateRef> *order)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::unordered_map<GateRef, size_t> lowerBound;
    std::unordered_map<GateRef, size_t> useCount;
    std::deque<GateRef> pendingList;
    std::vector<GateRef> bbAndFixedGatesList;
    for (const auto &item : bbGatesAddrToIdx) {
        bbAndFixedGatesList.push_back(item.first);
        std::vector<GateRef> succGates;
        acc.GetOutVector(item.first, succGates);
        for (const auto &succGate : succGates) {
            if (acc.GetOpCode(succGate).IsFixed()) {
                bbAndFixedGatesList.push_back(succGate);
            }
        }
    }
    std::function<void(GateRef)> dfsVisit = [&](GateRef curGate) {
        std::vector<GateRef> prevGates;
        acc.GetInVector(curGate, prevGates);
        for (const auto &prevGate : prevGates) {
            if (acc.GetOpCode(prevGate).IsSchedulable()) {
                useCount[prevGate]++;
                if (useCount[prevGate] == 1) {
                    dfsVisit(prevGate);
                }
            }
        }
    };
    for (const auto &gate : bbAndFixedGatesList) {
        dfsVisit(gate);
    }
    std::function<void(GateRef)> dfsFinish = [&](GateRef curGate) {
        size_t cnt = 0;
        std::vector<GateRef> prevGates;
        acc.GetInVector(curGate, prevGates);
        for (const auto &prevGate : prevGates) {
            if (acc.GetOpCode(prevGate).IsSchedulable()) {
                useCount[prevGate]--;
                size_t curLowerBound;
                if (acc.GetOpCode(curGate).IsState()) {  // cur_opcode would not be STATE_ENTRY
                    curLowerBound = bbGatesAddrToIdx.at(curGate);
                } else if (acc.GetOpCode(curGate).IsFixed()) {
                    ASSERT(cnt > 0);
                    curLowerBound = bbGatesAddrToIdx.at(acc.GetIn(acc.GetIn(curGate, 0), cnt - 1));
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
                    dfsFinish(prevGate);
                }
            }
            cnt++;
        }
    };
    for (const auto &gate : bbAndFixedGatesList) {
        dfsFinish(gate);
    }
    return lowerBound;
}

void Scheduler::Print(const std::vector<std::vector<GateRef>> *cfg, const Circuit *circuit)
{
    GateAccessor acc(const_cast<Circuit*>(circuit));
    std::vector<GateRef> bbGatesList;
    std::unordered_map<GateRef, size_t> bbGatesAddrToIdx;
    std::vector<size_t> immDom;
    std::tie(bbGatesList, bbGatesAddrToIdx, immDom) = Scheduler::CalculateDominatorTree(circuit);
    LOG_COMPILER(INFO) << "==========================================================================";
    for (size_t bbIdx = 0; bbIdx < cfg->size(); bbIdx++) {
        LOG_COMPILER(INFO) << "BB_" << bbIdx << "_" << acc.GetOpCode((*cfg)[bbIdx].front()).Str() << ":"
                           << "  immDom=" << immDom[bbIdx];
        LOG_COMPILER(INFO) << "  pred=[";
        bool isFirst = true;
        std::vector<GateRef> predStates;
        acc.GetInVector((*cfg)[bbIdx].front(), predStates);
        for (const auto &predState : predStates) {
            if (acc.GetOpCode(predState).IsState() || acc.GetOpCode(predState) == OpCode::STATE_ENTRY) {
                LOG_COMPILER(INFO) << (isFirst ? "" : " ") << bbGatesAddrToIdx.at(predState);
                isFirst = false;
            }
        }
        LOG_COMPILER(INFO) << "]  succ=[";
        isFirst = true;
        std::vector<GateRef> succStates;
        acc.GetOutVector((*cfg)[bbIdx].front(), succStates);
        for (const auto &succState : succStates) {
            if (acc.GetOpCode(succState).IsState() || acc.GetOpCode(succState) == OpCode::STATE_ENTRY) {
                LOG_COMPILER(INFO) << (isFirst ? "" : " ") << bbGatesAddrToIdx.at(succState);
                isFirst = false;
            }
        }
        LOG_COMPILER(INFO) << "]";
        for (size_t instIdx = (*cfg)[bbIdx].size(); instIdx > 0; instIdx--) {
            acc.Print((*cfg)[bbIdx][instIdx - 1]);
        }
    }
    LOG_COMPILER(INFO) << "==========================================================================";
}
}  // namespace panda::ecmascript::kungfu