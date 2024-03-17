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

// Build intra/inter block data dependence graph.
// 1: Build data dependence nodes
// 2: Build edges between dependence nodes. Edges are:
//   2.1) True dependence
//   2.2) Anti dependence
//   2.3) Output dependence
//   2.4) Barrier dependence
#ifndef MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H
#define MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H

#include "data_dep_base.h"
#include "mempool.h"
#include "cg_cdg.h"

namespace maplebe {
constexpr uint32 kMaxDumpRegionNodeNum = 6;

// Create data dependence of region, include:
// inter-data-dependence-analysis (cross-bb) and
// intra-data-dependence-analysis (in-bb)
class DataDepAnalysis {
public:
    DataDepAnalysis(CGFunc &f, MemPool &memPool, DataDepBase &dataDepBase)
        : cgFunc(f), interMp(memPool), interAlloc(&memPool), ddb(dataDepBase)
    {
    }
    virtual ~DataDepAnalysis() = default;

    void Run(CDGRegion &region);
    void GenerateDataDepGraphDotOfRegion(CDGRegion &region);

protected:
    void InitInfoInRegion(MemPool &regionMp, MapleAllocator &regionAlloc, CDGRegion &region);
    void InitInfoInCDGNode(MemPool &regionMp, MapleAllocator &regionAlloc, BB &bb, CDGNode &cdgNode);
    void ClearInfoInRegion(MemPool *regionMp, MapleAllocator *regionAlloc, CDGRegion &region) const;
    void BuildDepsForPrevSeparator(CDGNode &cdgNode, DepNode &depNode, CDGRegion &curRegion);
    void BuildSpecialInsnDependency(Insn &insn, CDGNode &cdgNode, CDGRegion &region, MapleAllocator &alloc);
    void UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, CDGNode &cdgNode);
    void UpdateReadyNodesInfo(CDGNode &cdgNode, const CDGNode &root) const;

private:
    CGFunc &cgFunc;
    MemPool &interMp;
    MapleAllocator interAlloc;
    DataDepBase &ddb;
};
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_DATA_DEP_ANALYSIS_H
