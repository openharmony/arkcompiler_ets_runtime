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

#include "cg_dominance.h"
#include <set>
#include "cg_option.h"
#include "cgfunc.h"

/*
 * This phase build dominance
 */
namespace maplebe {
constexpr uint32 kBBVectorInitialSize = 2;
void DomAnalysis::PostOrderWalk(const BB &bb, int32 &pid, MapleVector<bool> &visitedMap)
{
    std::stack<const BB*> s;
    s.push(&bb);
    visitedMap[bb.GetId()] = true;
    while (!s.empty()) {
        auto node = s.top();
        auto nodeId = node->GetId();
        DEBUG_ASSERT(nodeId < visitedMap.size() && nodeId < postOrderIDVec.size(), "index out of range");
        bool tail = true;
        for (auto succ : node->GetSuccs()) {
            if (!visitedMap[succ->GetId()]) {
                tail = false;
                visitedMap[succ->GetId()] = true;
                s.push(succ);
                break;
            }
        }
        if (tail) {
            s.pop();
            postOrderIDVec[nodeId] = pid++;
        }
    }
}

void DomAnalysis::GenPostOrderID()
{
    DEBUG_ASSERT(!bbVec.empty(), "size to be allocated is 0");
    MapleVector<bool> visitedMap(bbVec.size() + 1, false, cgFunc.GetFuncScopeAllocator()->Adapter());
    int32 postOrderID = 0;
    PostOrderWalk(commonEntryBB, postOrderID, visitedMap);
    // initialize reversePostOrder
    int32 maxPostOrderID = postOrderID - 1;
    reversePostOrder.resize(static_cast<uint32>(maxPostOrderID + 1));
    for (size_t i = 0; i < postOrderIDVec.size(); ++i) {
        int32 postOrderNo = postOrderIDVec[i];
        if (postOrderNo == -1) {
            continue;
        }
        reversePostOrder[static_cast<uint32>(maxPostOrderID - postOrderNo)] = bbVec[i];
    }
}

BB *DomAnalysis::Intersect(BB &bb1, const BB &bb2)
{
    auto *ptrBB1 = &bb1;
    auto *ptrBB2 = &bb2;
    while (ptrBB1 != ptrBB2) {
        while (postOrderIDVec[ptrBB1->GetId()] < postOrderIDVec[ptrBB2->GetId()]) {
            ptrBB1 = GetDom(ptrBB1->GetId());
        }
        while (postOrderIDVec[ptrBB2->GetId()] < postOrderIDVec[ptrBB1->GetId()]) {
            ptrBB2 = GetDom(ptrBB2->GetId());
        }
    }
    return ptrBB1;
}

bool DominanceBase::CommonEntryBBIsPred(const BB &bb) const
{
    for (const BB *suc : commonEntryBB.GetSuccs()) {
        if (suc == &bb) {
            return true;
        }
    }
    return false;
}

// Figure 3 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
void DomAnalysis::ComputeDominance()
{
    SetDom(commonEntryBB.GetId(), &commonEntryBB);
    bool changed;
    do {
        changed = false;
        for (size_t i = 1; i < reversePostOrder.size(); ++i) {
            BB *bb = reversePostOrder[i];
            if (bb == nullptr) {
                continue;
            }
            BB *pre = nullptr;
            auto it = bb->GetPredsBegin();
            if (CommonEntryBBIsPred(*bb) || bb->GetPreds().empty()) {
                pre = &commonEntryBB;
            } else {
                pre = *it;
            }
            ++it;
            while ((GetDom(pre->GetId()) == nullptr || pre == bb) && it != bb->GetPredsEnd()) {
                pre = *it;
                ++it;
            }
            BB *newIDom = pre;
            for (; it != bb->GetPredsEnd(); ++it) {
                pre = *it;
                if (GetDom(pre->GetId()) != nullptr && pre != bb) {
                    newIDom = Intersect(*pre, *newIDom);
                }
            }
            if (GetDom(bb->GetId()) != newIDom) {
                SetDom(bb->GetId(), newIDom);
                changed = true;
            }
        }
    } while (changed);
}

// Figure 5 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
void DomAnalysis::ComputeDomFrontiers()
{
    for (const BB *bb : bbVec) {
        if (bb == nullptr || bb == &commonExitBB) {
            continue;
        }
        if (bb->GetPreds().size() < kBBVectorInitialSize) {
            continue;
        }
        for (BB *pre : bb->GetPreds()) {
            BB *runner = pre;
            while (runner != nullptr && runner != GetDom(bb->GetId()) && runner != &commonEntryBB) {
                if (!HasDomFrontier(runner->GetId(), bb->GetId())) {
                    domFrontier[runner->GetId()].push_back(bb->GetId());
                }
                runner = GetDom(runner->GetId());
            }
        }
    }
    // check entry bb's predBB, such as :
    // bb1 is commonEntryBB, bb2 is entryBB, bb2 is domFrontier of bb3 and bb7.
    //       1
    //       |
    //       2 <-
    //      /   |
    //     3    |
    //    / \   |
    //   4   7---
    //  / \  ^
    // |   | |
    // 5-->6--
    for (BB *succ : commonEntryBB.GetSuccs()) {
        if (succ->GetPreds().size() != 1) {  // Only deal with one pred bb.
            continue;
        }
        for (BB *pre : succ->GetPreds()) {
            BB *runner = pre;
            while (runner != GetDom(succ->GetId()) && runner != &commonEntryBB && runner != succ) {
                if (!HasDomFrontier(runner->GetId(), succ->GetId())) {
                    domFrontier[runner->GetId()].push_back(succ->GetId());
                }
                runner = GetDom(runner->GetId());
            }
        }
    }
}

void DomAnalysis::ComputeDomChildren()
{
    for (auto *bb : reversePostOrder) {
        if (bb == nullptr || GetDom(bb->GetId()) == nullptr) {
            continue;
        }
        BB *parent = GetDom(bb->GetId());
        if (parent == bb) {
            continue;
        }
        domChildren[parent->GetId()].push_back(bb->GetId());
    }
}

// bbidMarker indicates that the iterDomFrontier results for bbid < bbidMarker
// have been computed
void DomAnalysis::GetIterDomFrontier(const BB *bb, MapleUnorderedSet<uint32> *dfset, uint32 bbidMarker,
                                     std::vector<bool> &visitedMap)
{
    if (visitedMap[bb->GetId()]) {
        return;
    }
    visitedMap[bb->GetId()] = true;
    for (uint32 frontierbbid : domFrontier[bb->GetId()]) {
        (void)dfset->insert(frontierbbid);
        if (frontierbbid < bbidMarker) {  // union with its computed result
            dfset->insert(iterDomFrontier[frontierbbid].begin(), iterDomFrontier[frontierbbid].end());
        } else {  // recursive call
            BB *frontierbb = bbVec[frontierbbid];
            GetIterDomFrontier(frontierbb, dfset, bbidMarker, visitedMap);
        }
    }
}

void DomAnalysis::ComputeIterDomFrontiers()
{
    for (BB *bb : bbVec) {
        if (bb == nullptr || bb == &commonExitBB) {
            continue;
        }
        std::vector<bool> visitedMap(bbVec.size(), false);
        GetIterDomFrontier(bb, &iterDomFrontier[bb->GetId()], bb->GetId(), visitedMap);
    }
}

uint32 DomAnalysis::ComputeDtPreorder(const BB &bb, uint32 &num)
{
    // {BB, parent/self BB id}
    using Node = std::pair<const BB *, uint32>;
    std::stack<Node> allNodes;
    allNodes.emplace(Node{&bb, bb.GetId()});

    while (!allNodes.empty()) {
        DEBUG_ASSERT(num < dtPreOrder.size(), "index out of range in Dominance::ComputeDtPreorder");
        Node curNode = allNodes.top();
        allNodes.pop();
        auto curBBId = curNode.first->GetId();
        dtPreOrder[num] = curBBId;
        dtDfn[curBBId] = num;
        ++num;
        dtDfnOut[curNode.second] = num;
        if (domChildren[curBBId].empty()) {
            dtDfnOut[curBBId] = num;
            continue;
        }
        for (size_t idx = domChildren[curBBId].size(); idx > 0; --idx) {
            allNodes.emplace(Node{bbVec[domChildren[curBBId][idx - 1]], curBBId});
        }
    }
    return num;
}

// true if b1 dominates b2
bool DomAnalysis::Dominate(const BB &bb1, const BB &bb2)
{
    return dtDfn[bb1.GetId()] <= dtDfn[bb2.GetId()] && dtDfnOut[bb1.GetId()] >= dtDfnOut[bb2.GetId()];
}

void DomAnalysis::Compute()
{
    GenPostOrderID();
    ComputeDominance();
    ComputeDomFrontiers();
    ComputeDomChildren();
    ComputeIterDomFrontiers();
    uint32 num = 0;
    (void)ComputeDtPreorder(*cgFunc.GetFirstBB(), num);
    GetDtPreOrder().resize(num);
}

void DomAnalysis::Dump()
{
    for (BB *bb : reversePostOrder) {
        LogInfo::MapleLogger() << "postorder no " << postOrderIDVec[bb->GetId()];
        LogInfo::MapleLogger() << " is bb:" << bb->GetId();
        LogInfo::MapleLogger() << " im_dom is bb:" << GetDom(bb->GetId())->GetId();
        LogInfo::MapleLogger() << " domfrontier: [";
        for (uint32 id : domFrontier[bb->GetId()]) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "] domchildren: [";
        for (uint32 id : domChildren[bb->GetId()]) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n";
    }
    LogInfo::MapleLogger() << "\npreorder traversal of dominator tree:";
    for (uint32 id : dtPreOrder) {
        LogInfo::MapleLogger() << id << " ";
    }
    LogInfo::MapleLogger() << "\n\n";
}

bool CgDomAnalysis::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *domMemPool = GetPhaseMemPool();
    domAnalysis =
        domMemPool->New<DomAnalysis>(f, *domMemPool, *domMemPool, f.GetAllBBs(), *f.GetFirstBB(), *f.GetCommonExitBB());
    domAnalysis->Compute();
    if (CG_DEBUG_FUNC(f)) {
        domAnalysis->Dump();
    }
    return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgDomAnalysis, domanalysis)
} /* namespace maplebe */
