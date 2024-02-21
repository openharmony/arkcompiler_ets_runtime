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
#ifndef MAPLE_PGO_INCLUDE_CFG_MST_H
#define MAPLE_PGO_INCLUDE_CFG_MST_H

#include "types_def.h"
#include "mempool_allocator.h"

namespace maple {
template <class Edge, class BB>
class CFGMST {
public:
    explicit CFGMST(MemPool &mp) : mp(&mp), alloc(&mp), allEdges(alloc.Adapter()), bbGroups(alloc.Adapter()) {}
    virtual ~CFGMST()
    {
        mp = nullptr;
    }
    void ComputeMST(BB *commonEntry, BB *commonExit);
    void BuildEdges(BB *commonEntry, BB *commonExit);
    void SortEdges();
    void AddEdge(BB *src, BB *dest, uint64 w, bool isCritical = false, bool isFake = false);
    bool IsCritialEdge(const BB *src, const BB *dest) const
    {
        return src->GetSuccs().size() > 1 && dest->GetPreds().size() > 1;
    }
    const MapleVector<Edge *> &GetAllEdges() const
    {
        return allEdges;
    }

    size_t GetAllEdgesSize() const
    {
        return allEdges.size();
    }

    uint32 GetAllBBs() const
    {
        return totalBB;
    }

    void GetInstrumentEdges(std::vector<Edge *> &instrumentEdges) const
    {
        for (const auto &e : allEdges) {
            if (!e->IsInMST()) {
                instrumentEdges.push_back(e);
            }
        }
    }
    void DumpEdgesInfo() const;

private:
    uint32 FindGroup(uint32 bbId);
    bool UnionGroups(uint32 srcId, uint32 destId);
    static constexpr int kNormalEdgeWeight = 2;
    static constexpr int kExitEdgeWeight = 3;
    static constexpr int kFakeExitEdgeWeight = 4;
    static constexpr int kCriticalEdgeWeight = 4;
    MemPool *mp;
    MapleAllocator alloc;
    MapleVector<Edge *> allEdges;
    MapleMap<uint32, uint32> bbGroups;  // bbId - gourpId
    uint32 totalBB = 0;
};
} /* namespace maple */
#endif  // MAPLE_PGO_INCLUDE_CFG_MST_H
