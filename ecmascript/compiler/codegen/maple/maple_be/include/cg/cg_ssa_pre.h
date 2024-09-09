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

#ifndef MAPLEBE_CG_INCLUDE_CG_SSU_PRE_H
#define MAPLEBE_CG_INCLUDE_CG_SSU_PRE_H
#include <vector>
#include "mempool.h"
#include "mempool_allocator.h"
#include "cg_dominance.h"
#include "loop.h"

// Use SSAPRE to determine where to insert saves for callee-saved registers.
// The external interface is DoSavePlacementOpt(). Class SsaPreWorkCand is used
// as input/output interface.

namespace maplebe {

using BBId = uint32;

// This must have been constructed by the caller of DoSavePlacementOpt() and
// passed to it as parameter.  The caller of DoSavePlacementOpt() describes
// the problem via occBBs.  DoSavePlacementOpt()'s outputs are returned to the
// caller by setting saveAtEntryBBs.
class SsaPreWorkCand {
public:
    explicit SsaPreWorkCand(MapleAllocator *alloc) : occBBs(alloc->Adapter()), saveAtEntryBBs(alloc->Adapter()) {}
    // inputs
    MapleSet<BBId> occBBs;  // Id's of BBs with appearances of the callee-saved reg
    // outputs
    MapleSet<BBId> saveAtEntryBBs;  // Id's of BBs to insert saves of the register at BB entry
    bool saveAtProlog = false;      // if true, no shrinkwrapping can be done and
                                    // the other outputs can be ignored
};

extern void DoSavePlacementOpt(CGFunc *f, DomAnalysis *dom, LoopAnalysis *loop, SsaPreWorkCand *workCand);

enum AOccType {
    kAOccUndef,
    kAOccReal,
    kAOccPhi,
    kAOccPhiOpnd,
    kAOccExit,
};

class Occ {
public:
    Occ(AOccType ty, BB *bb) : occTy(ty), cgbb(bb) {}
    virtual ~Occ() = default;

    virtual void Dump() const = 0;
    bool IsDominate(DomAnalysis *dom, const Occ *occ) const
    {
        return dom->Dominate(*cgbb, *occ->cgbb);
    }

    AOccType occTy;
    uint32 classId = 0;
    BB *cgbb;            // the BB it occurs in
    Occ *def = nullptr;  // points to its single def
};

class RealOcc : public Occ {
public:
    explicit RealOcc(BB *bb) : Occ(kAOccReal, bb) {}
    virtual ~RealOcc() = default;

    void Dump() const override
    {
        LogInfo::MapleLogger() << "RealOcc at bb" << cgbb->GetId();
        LogInfo::MapleLogger() << " classId" << classId;
    }

    bool redundant = true;
};

class PhiOcc;

class PhiOpndOcc : public Occ {
public:
    explicit PhiOpndOcc(BB *bb) : Occ(kAOccPhiOpnd, bb) {}
    virtual ~PhiOpndOcc() = default;

    void Dump() const override
    {
        LogInfo::MapleLogger() << "PhiOpndOcc at bb" << cgbb->GetId() << " classId" << classId;
    }

    PhiOcc *defPhiOcc = nullptr;  // its lhs definition
    bool hasRealUse = false;
    bool insertHere = false;
};

class PhiOcc : public Occ {
public:
    PhiOcc(BB *bb, MapleAllocator &alloc) : Occ(kAOccPhi, bb), phiOpnds(alloc.Adapter()) {}
    virtual ~PhiOcc() = default;

    bool WillBeAvail() const
    {
        return isCanBeAvail && !isLater;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "PhiOcc at bb" << cgbb->GetId() << " classId" << classId << " Phi[";
        for (size_t i = 0; i < phiOpnds.size(); i++) {
            phiOpnds[i]->Dump();
            if (i != phiOpnds.size() - 1) {
                LogInfo::MapleLogger() << ", ";
            }
        }
        LogInfo::MapleLogger() << "]";
    }

    bool isDownsafe = true;
    bool speculativeDownsafe = false;  // true if set to downsafe via speculation
    bool isCanBeAvail = true;
    bool isLater = true;
    MapleVector<PhiOpndOcc *> phiOpnds;
};

class ExitOcc : public Occ {
public:
    explicit ExitOcc(BB *bb) : Occ(kAOccExit, bb) {}
    virtual ~ExitOcc() = default;

    void Dump() const override
    {
        LogInfo::MapleLogger() << "ExitOcc at bb" << cgbb->GetId();
    }
};

class SSAPre {
public:
    SSAPre(CGFunc *cgfunc, DomAnalysis *dm, LoopAnalysis *loop, MemPool *memPool, SsaPreWorkCand *wkcand,
           bool aeap, bool enDebug)
        : cgFunc(cgfunc),
          dom(dm),
          loop(loop),
          preMp(memPool),
          preAllocator(memPool),
          workCand(wkcand),
          fullyAntBBs(cgfunc->GetAllBBs().size(), true, preAllocator.Adapter()),
          phiDfns(std::less<uint32>(), preAllocator.Adapter()),
          classCount(0),
          realOccs(preAllocator.Adapter()),
          allOccs(preAllocator.Adapter()),
          phiOccs(preAllocator.Adapter()),
          exitOccs(preAllocator.Adapter()),
          asEarlyAsPossible(aeap),
          enabledDebug(enDebug)
    {
    }
    ~SSAPre() = default;

    void ApplySSAPre();

private:
    // step 6 methods
    void CodeMotion();
    // step 5 methods
    void Finalize();
    // step 4 methods
    void ResetCanBeAvail(PhiOcc *phi) const;
    void ComputeCanBeAvail() const;
    void ResetLater(PhiOcc *phi) const;
    void ComputeLater() const;
    // step 3 methods
    void ResetDownsafe(const PhiOpndOcc *phiOpnd) const;
    void ComputeDownsafe() const;
    // step 2 methods
    void Rename();
    // step 1 methods
    void GetIterDomFrontier(const BB *bb, MapleSet<uint32> *dfset) const
    {
        for (BBId bbid : dom->GetIdomFrontier(bb->GetId())) {
            (void)dfset->insert(dom->GetDtDfnItem(bbid));
        }
    }
    void FormPhis();
    void CreateSortedOccs();
    // step 0 methods
    void PropagateNotAnt(BB *bb, std::set<BB *, BBIdCmp> *visitedBBs);
    void FormRealsNExits();

    CGFunc *cgFunc;
    DomAnalysis *dom;
    LoopAnalysis *loop;
    MemPool *preMp;
    MapleAllocator preAllocator;
    SsaPreWorkCand *workCand;
    // step 0
    MapleVector<bool> fullyAntBBs;  // index is BBid; true if occ is fully anticipated at BB entry
    // step 1 phi insertion data structures:
    MapleSet<uint32> phiDfns;  // set by FormPhis(); set of BBs in terms of their
                               // dfn's; index into dominance->dt_preorder to get
                               // their bbid's
    // step 2 renaming
    uint32 classCount;  // for assigning new class id
    // the following 4 lists are all maintained in order of dt_preorder
    MapleVector<Occ *> realOccs;
    MapleVector<Occ *> allOccs;
    MapleVector<PhiOcc *> phiOccs;
    MapleVector<ExitOcc *> exitOccs;
    bool asEarlyAsPossible;
    bool enabledDebug;
};

};      // namespace maplebe
#endif  // MAPLEBE_CG_INCLUDE_CG_SSA_PRE_H
