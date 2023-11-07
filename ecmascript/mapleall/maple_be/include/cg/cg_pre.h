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

#ifndef MAPLEBE_CG_INCLUDE_CGPRE_H
#define MAPLEBE_CG_INCLUDE_CGPRE_H
#include "cg_occur.h"
#include "cg_dominance.h"
#include "cgfunc.h"

namespace maplebe {
enum PreKind { kExprPre, kStmtPre, kLoadPre, kAddrPre };

class CGPre {
public:
    CGPre(DomAnalysis &currDom, MemPool &memPool, MemPool &mp2, PreKind kind, uint32 limit)
        : dom(&currDom),
          ssaPreMemPool(&memPool),
          ssaPreAllocator(&memPool),
          perCandMemPool(&mp2),
          perCandAllocator(&mp2),
          workList(ssaPreAllocator.Adapter()),
          preKind(kind),
          allOccs(ssaPreAllocator.Adapter()),
          phiOccs(ssaPreAllocator.Adapter()),
          exitOccs(ssaPreAllocator.Adapter()),
          preLimit(limit),
          dfPhiDfns(std::less<uint32>(), ssaPreAllocator.Adapter()),
          varPhiDfns(std::less<uint32>(), ssaPreAllocator.Adapter()),
          temp2LocalRefVarMap(ssaPreAllocator.Adapter())
    {
        preWorkCandHashTable.GetWorkcandHashTable().fill(nullptr);
    }

    virtual ~CGPre() = default;

    const MapleVector<CgOccur *> &GetRealOccList() const
    {
        return workCand->GetRealOccs();
    }

    virtual BB *GetBB(uint32 id) const = 0;
    virtual PUIdx GetPUIdx() const = 0;
    virtual void SetCurFunction(PUIdx) const {}

    void GetIterDomFrontier(const BB *bb, MapleSet<uint32> *dfset) const
    {
        for (uint32 bbid : dom->GetIdomFrontier(bb->GetId())) {
            (void)dfset->insert(dom->GetDtDfnItem(bbid));
        }
    }

    PreWorkCand *GetWorkCand() const
    {
        return workCand;
    }
    // compute downsafety for each PHI
    static void ResetDS(CgPhiOcc *phiOcc);
    void ComputeDS();

protected:
    virtual void ComputeVarAndDfPhis() = 0;
    virtual void CreateSortedOccs();
    CgOccur *CreateRealOcc(Insn &insn, Operand &opnd, OccType occType);
    virtual void BuildWorkList() = 0;
    /* for stmt pre only */
    void CreateExitOcc(BB &bb)
    {
        CgOccur *exitOcc = ssaPreMemPool->New<CgOccur>(kOccExit, 0, bb, nullptr);
        exitOccs.push_back(exitOcc);
    }

    DomAnalysis *dom;
    MemPool *ssaPreMemPool;
    MapleAllocator ssaPreAllocator;
    MemPool *perCandMemPool;
    MapleAllocator perCandAllocator;
    MapleList<PreWorkCand *> workList;
    PreWorkCand *workCand = nullptr;  // the current PreWorkCand
    PreKind preKind;

    // PRE work candidates; incremented by 2 for each tree;
    // purpose is to avoid processing a node the third time
    // inside a tree (which is a DAG)
    // the following 3 lists are all maintained in order of dt_preorder
    MapleVector<CgOccur *> allOccs;   // cleared at start of each workcand
    MapleVector<CgPhiOcc *> phiOccs;  // cleared at start of each workcand
    MapleVector<CgOccur *> exitOccs;  // this is shared by all workcands
    uint32 preLimit;  // set by command-line option to limit the number of candidates optimized (for debugging purpose)
    // step 1 phi insertion data structures
    // following are set of BBs in terms of their dfn's; index into
    // dominance->pdt_preorder to get their bbid's
    MapleSet<uint32> dfPhiDfns;   // phis inserted due to dominance frontiers
    MapleSet<uint32> varPhiDfns;  // phis inserted due to the var operands
    // step 2 renaming data structures
    uint32 classCount = 0;  // count class created during renaming
    // is index into workCand->realOccs
    // step 6 codemotion data structures
    MapleMap<Operand *, Operand *> temp2LocalRefVarMap;
    int32 reBuiltOccIndex = -1;  // stores the size of worklist every time when try to add new worklist, update before
    // each code motion
    uint32 strIdxCount =
        0;  // ssapre will create a lot of temp variables if using var to store redundances, start from 0
    PreWorkCandHashTable preWorkCandHashTable;
};
}  // namespace maplebe
#endif  // MAPLEBE_CG_INCLUDE_CGPRE_H
