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

#ifndef MAPLEBE_CG_INCLUDE_CGSSUPRE_H
#define MAPLEBE_CG_INCLUDE_CGSSUPRE_H
#include <vector>
#include "mempool.h"
#include "mempool_allocator.h"
#include "cg_dominance.h"
#include "cg_ssa_pre.h"

// Use SSUPRE to determine where to insert restores for callee-saved registers.
// The external interface is DoRestorePlacementOpt(). Class SPreWorkCand is used
// as input/output interface.

namespace maplebe {

// This must have been constructed by the caller of DoRestorePlacementOpt() and
// passed to it as parameter.  The caller of DoRestorePlacementOpt() describes
// the problem via occBBs and saveBBs.  DoRestorePlacementOpt()'s outputs are
// returned to the caller by setting restoreAtEntryBBs and restoreAtExitBBs.
class SPreWorkCand {
public:
    explicit SPreWorkCand(MapleAllocator *alloc)
        : occBBs(alloc->Adapter()),
          saveBBs(alloc->Adapter()),
          restoreAtEntryBBs(alloc->Adapter()),
          restoreAtExitBBs(alloc->Adapter())
    {
    }
    // inputs
    MapleSet<BBId> occBBs;   // Id's of BBs with appearances of the callee-saved reg
    MapleSet<BBId> saveBBs;  // Id's of BBs with saves of the callee-saved reg
    // outputs
    MapleSet<BBId> restoreAtEntryBBs;  // Id's of BBs to insert restores of the register at BB entry
    MapleSet<BBId> restoreAtExitBBs;   // Id's of BBs to insert restores of the register at BB exit
    bool restoreAtEpilog = false;      // if true, no shrinkwrapping can be done and
                                       // the other outputs can be ignored
};

extern void DoRestorePlacementOpt(CGFunc *f, PostDomAnalysis *pdom, SPreWorkCand *workCand);

enum SOccType {
    kSOccUndef,
    kSOccReal,
    kSOccLambda,
    kSOccLambdaRes,
    kSOccEntry,
    kSOccKill,
};

class SOcc {
public:
    SOcc(SOccType ty, BB *bb) : occTy(ty), cgbb(bb) {}
    virtual ~SOcc() = default;

    virtual void Dump() const = 0;
    bool IsPostDominate(PostDomAnalysis *pdom, const SOcc *occ) const
    {
        return pdom->PostDominate(*cgbb, *occ->cgbb);
    }

    SOccType occTy;
    uint32 classId = 0;
    BB *cgbb;             // the BB it occurs in
    SOcc *use = nullptr;  // points to its single use
};

class SRealOcc : public SOcc {
public:
    explicit SRealOcc(BB *bb) : SOcc(kSOccReal, bb) {}
    virtual ~SRealOcc() = default;

    void Dump() const override
    {
        LogInfo::MapleLogger() << "RealOcc at bb" << cgbb->GetId();
        LogInfo::MapleLogger() << " classId" << classId;
    }

    bool redundant = true;
};

class SLambdaOcc;

class SLambdaResOcc : public SOcc {
public:
    explicit SLambdaResOcc(BB *bb) : SOcc(kSOccLambdaRes, bb) {}
    virtual ~SLambdaResOcc() = default;

    void Dump() const override
    {
        LogInfo::MapleLogger() << "LambdaResOcc at bb" << cgbb->GetId() << " classId" << classId;
    }

    SLambdaOcc *useLambdaOcc = nullptr;  // its rhs use
    bool hasRealUse = false;
    bool insertHere = false;
};

class SLambdaOcc : public SOcc {
public:
    SLambdaOcc(BB *bb, MapleAllocator &alloc) : SOcc(kSOccLambda, bb), lambdaRes(alloc.Adapter()) {}
    virtual ~SLambdaOcc() = default;

    bool WillBeAnt() const
    {
        return isCanBeAnt && !isEarlier;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "LambdaOcc at bb" << cgbb->GetId() << " classId" << classId << " Lambda[";
        for (size_t i = 0; i < lambdaRes.size(); i++) {
            lambdaRes[i]->Dump();
            DEBUG_ASSERT(lambdaRes.size() >= 1, "lambdaRes.size() -1 should be uint");
            if (i != lambdaRes.size() - 1) {
                LogInfo::MapleLogger() << ", ";
            }
        }
        LogInfo::MapleLogger() << "]";
    }

    bool isUpsafe = true;
    bool isCanBeAnt = true;
    bool isEarlier = true;
    MapleVector<SLambdaResOcc *> lambdaRes;
};

class SEntryOcc : public SOcc {
public:
    explicit SEntryOcc(BB *bb) : SOcc(kSOccEntry, bb) {}
    virtual ~SEntryOcc() = default;

    void Dump() const
    {
        LogInfo::MapleLogger() << "EntryOcc at bb" << cgbb->GetId();
    }
};

class SKillOcc : public SOcc {
public:
    explicit SKillOcc(BB *bb) : SOcc(kSOccKill, bb) {}
    virtual ~SKillOcc() = default;

    void Dump() const override
    {
        LogInfo::MapleLogger() << "KillOcc at bb" << cgbb->GetId();
    }
};

class SSUPre {
public:
    SSUPre(CGFunc *cgfunc, PostDomAnalysis *pd, MemPool *memPool, SPreWorkCand *wkcand, bool alap, bool enDebug)
        : cgFunc(cgfunc),
          pdom(pd),
          spreMp(memPool),
          spreAllocator(memPool),
          workCand(wkcand),
          fullyAvailBBs(cgfunc->GetAllBBs().size(), true, spreAllocator.Adapter()),
          lambdaDfns(std::less<uint32>(), spreAllocator.Adapter()),
          classCount(0),
          realOccs(spreAllocator.Adapter()),
          allOccs(spreAllocator.Adapter()),
          lambdaOccs(spreAllocator.Adapter()),
          entryOccs(spreAllocator.Adapter()),
          asLateAsPossible(alap),
          enabledDebug(enDebug)
    {
        CreateEntryOcc(cgfunc->GetFirstBB());
    }
    ~SSUPre() = default;

    void ApplySSUPre();

private:
    // step 6 methods
    void CodeMotion();
    // step 5 methods
    void Finalize();
    // step 4 methods
    void ResetCanBeAnt(SLambdaOcc *lambda) const;
    void ComputeCanBeAnt() const;
    void ResetEarlier(SLambdaOcc *lambda) const;
    void ComputeEarlier() const;
    // step 3 methods
    void ResetUpsafe(const SLambdaResOcc *lambdaRes) const;
    void ComputeUpsafe() const;
    // step 2 methods
    void Rename();
    // step 1 methods
    void GetIterPdomFrontier(const BB *bb, MapleSet<uint32> *pdfset) const
    {
        for (BBId bbid : pdom->GetIpdomFrontier(bb->GetId())) {
            (void)pdfset->insert(pdom->GetPdtDfnItem(bbid));
        }
    }
    void FormLambdas();
    void CreateSortedOccs();
    // step 0 methods
    void CreateEntryOcc(BB *bb)
    {
        SEntryOcc *entryOcc = spreMp->New<SEntryOcc>(bb);
        entryOccs.push_back(entryOcc);
    }
    void PropagateNotAvail(BB *bb, std::set<BB *, BBIdCmp> *visitedBBs);
    void FormReals();

    CGFunc *cgFunc;
    PostDomAnalysis *pdom;
    MemPool *spreMp;
    MapleAllocator spreAllocator;
    SPreWorkCand *workCand;
    // step 0
    MapleVector<bool> fullyAvailBBs;  // index is BBid; true if occ is fully available at BB exit
    // step 1 lambda insertion data structures:
    MapleSet<uint32> lambdaDfns;  // set by FormLambdas(); set of BBs in terms of
                                  // their dfn's; index into
                                  // dominance->pdt_preorder to get their bbid's
    // step 2 renaming
    uint32 classCount;  // for assigning new class id
    // the following 4 lists are all maintained in order of pdt_preorder
    MapleVector<SOcc *> realOccs;  // both real and kill occurrences
    MapleVector<SOcc *> allOccs;
    MapleVector<SLambdaOcc *> lambdaOccs;
    MapleVector<SEntryOcc *> entryOccs;
    bool asLateAsPossible;
    bool enabledDebug;
};

};      // namespace maplebe
#endif  // MAPLEBE_CG_INCLUDE_CGSSUPRE_H
