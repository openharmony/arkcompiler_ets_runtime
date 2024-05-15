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

#include "cfg_mst.h"
#include "cgbb.h"
#include "instrument.h"
namespace maple {
template <class Edge, class BB>
void CFGMST<Edge, BB>::BuildEdges(BB *commonEntry, BB *commonExit)
{
    for (auto *curbb = commonEntry; curbb != nullptr; curbb = curbb->GetNext()) {
        bbGroups[curbb->GetId()] = curbb->GetId();
        if (curbb == commonExit) {
            continue;
        }
        totalBB++;
        for (auto *succBB : curbb->GetSuccs()) {
            // exitBB incoming edge allocate high weight
            if (succBB->GetKind() == BB::BBKind::kBBReturn) {
                AddEdge(curbb, succBB, kExitEdgeWeight);
                continue;
            }
            if (IsCritialEdge(curbb, succBB)) {
                AddEdge(curbb, succBB, kCriticalEdgeWeight, true);
                continue;
            }
            AddEdge(curbb, succBB, kNormalEdgeWeight);
        }
    }
    ASSERT_NOT_NULL(commonExit);
    for (BB *bb : commonExit->GetPreds()) {
        AddEdge(bb, commonExit, kFakeExitEdgeWeight, false, true);
    }
    bbGroups[commonExit->GetId()] = commonExit->GetId();
    // insert fake edge to keep consistent
    AddEdge(commonExit, commonEntry, UINT64_MAX, false, true);
}

template <class Edge, class BB>
void CFGMST<Edge, BB>::ComputeMST(BB *commonEntry, BB *commonExit)
{
    BuildEdges(commonEntry, commonExit);
    SortEdges();
    /* only one edge means only one bb */
    if (allEdges.size() == 1) {
        LogInfo::MapleLogger() << "only one edge find " << std::endl;
        return;
    }
    // first,put all critial edge,with destBB is eh-handle
    // in mst,because current doesn't support split that edge
    for (auto &e : allEdges) {
        if (UnionGroups(e->GetSrcBB()->GetId(), e->GetDestBB()->GetId())) {
            e->SetInMST();
        }
    }
}

template <class Edge, class BB>
void CFGMST<Edge, BB>::AddEdge(BB *src, BB *dest, uint64 w, bool isCritical, bool isFake)
{
    if (src == nullptr || dest == nullptr) {
        return;
    }
    bool found = false;
    for (auto &edge : allEdges) {
        if (edge->GetSrcBB() == src && edge->GetDestBB() == dest) {
            uint64 weight = edge->GetWeight();
            weight++;
            edge->SetWeight(weight);
            found = true;
        }
    }
    if (!found) {
        (void)allEdges.emplace_back(mp->New<Edge>(src, dest, w, isCritical, isFake));
    }
}

template <class Edge, class BB>
void CFGMST<Edge, BB>::SortEdges()
{
    std::stable_sort(allEdges.begin(), allEdges.end(),
                     [](const Edge *edge1, const Edge *edge2) { return edge1->GetWeight() > edge2->GetWeight(); });
}

template <class Edge, class BB>
uint32 CFGMST<Edge, BB>::FindGroup(uint32 bbId)
{
    CHECK_FATAL(bbGroups.count(bbId) != 0, "unRegister bb");
    if (bbGroups[bbId] != bbId) {
        bbGroups[bbId] = FindGroup(bbGroups[bbId]);
    }
    return bbGroups[bbId];
}

template <class Edge, class BB>
bool CFGMST<Edge, BB>::UnionGroups(uint32 srcId, uint32 destId)
{
    uint32 srcGroupId = FindGroup(srcId);
    uint32 destGroupId = FindGroup(destId);
    if (srcGroupId == destGroupId) {
        return false;
    }
    bbGroups[srcGroupId] = destGroupId;
    return true;
}
template <class Edge, class BB>
void CFGMST<Edge, BB>::DumpEdgesInfo() const
{
    for (auto &edge : allEdges) {
        BB *src = edge->GetSrcBB();
        BB *dest = edge->GetDestBB();
        LogInfo::MapleLogger() << "BB" << src->GetId() << "->"
                               << "BB" << dest->GetId() << " weight " << edge->GetWeight();
        if (edge->IsInMST()) {
            LogInfo::MapleLogger() << " in Mst\n";
        } else {
            LogInfo::MapleLogger() << " not in  Mst\n";
        }
    }
}

template class CFGMST<maple::BBEdge<maplebe::BB>, maplebe::BB>;
template class CFGMST<maple::BBUseEdge<maplebe::BB>, maplebe::BB>;
}  // namespace maple
