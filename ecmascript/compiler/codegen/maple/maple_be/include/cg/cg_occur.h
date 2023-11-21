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

#ifndef MAPLEBE_CG_INCLUDE_CGOCCUR_H
#define MAPLEBE_CG_INCLUDE_CGOCCUR_H
#include "cg_dominance.h"

// the data structures that represent occurrences and work candidates for PRE
namespace maplebe {
enum OccType {
    kOccUndef,
    kOccReal,
    kOccDef,
    kOccStore,
    kOccPhiocc,
    kOccPhiopnd,
    kOccExit,
    kOccUse,     // for use appearances when candidate is dassign
    kOccMembar,  // for representing occurrence of memory barriers (use CgRealOcc)
};

class CgOccur {
public:
    CgOccur(OccType ty, BB *bb, Insn *insn, Operand *opnd) : occTy(ty), cgBB(bb), insn(insn), opnd(opnd) {}

    CgOccur(OccType ty, int cId, BB &bb, CgOccur *df) : occTy(ty), classID(cId), cgBB(&bb), def(df) {}
    virtual ~CgOccur() = default;

    bool IsDominate(DomAnalysis &dom, CgOccur &occ);
    const BB *GetBB() const
    {
        return cgBB;
    }

    BB *GetBB()
    {
        return cgBB;
    }

    void SetBB(BB &bb)
    {
        cgBB = &bb;
    }

    OccType GetOccType() const
    {
        return occTy;
    }

    int GetClassID() const
    {
        return classID;
    }

    void SetClassID(int id)
    {
        classID = id;
    }

    const CgOccur *GetDef() const
    {
        return def;
    }

    CgOccur *GetDef()
    {
        return def;
    }

    void SetDef(CgOccur *define)
    {
        def = define;
    }

    const Insn *GetInsn() const
    {
        return insn;
    }

    Insn *GetInsn()
    {
        return insn;
    }

    const Operand *GetOperand() const
    {
        return opnd;
    }

    Operand *GetOperand()
    {
        return opnd;
    }

    bool Processed() const
    {
        return processed;
    }

    void SetProcessed(bool val)
    {
        processed = val;
    }

    virtual CgOccur *GetPrevVersionOccur()
    {
        CHECK_FATAL(false, "has no prev version occur");
    }

    virtual void SetPrevVersionOccur(CgOccur *)
    {
        CHECK_FATAL(false, "has no prev version occur");
    }

    virtual void Dump() const
    {
        if (occTy == kOccExit) {
            LogInfo::MapleLogger() << "ExitOcc at bb " << GetBB()->GetId() << std::endl;
        }
    };

private:
    OccType occTy = kOccUndef;  // kinds of occ
    int classID = 0;            // class id
    BB *cgBB = nullptr;         // the BB it occurs in
    Insn *insn = nullptr;
    Operand *opnd = nullptr;
    CgOccur *def = nullptr;
    bool processed = false;
};

class CgUseOcc : public CgOccur {
public:
    CgUseOcc(BB *bb, Insn *insn, Operand *opnd) : CgOccur(kOccUse, bb, insn, opnd), needReload(false) {}

    ~CgUseOcc() = default;

    bool Reload() const
    {
        return needReload;
    }

    void SetReload(bool val)
    {
        needReload = val;
    }

    CgOccur *GetPrevVersionOccur() override
    {
        return prevVersion;
    }

    void SetPrevVersionOccur(CgOccur *val) override
    {
        prevVersion = val;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "UseOcc " << GetClassID() << " at bb " << GetBB()->GetId() << ": "
                               << (needReload ? "need-reload, " : "not need-reload, ") << "\n";
    }

private:
    bool needReload = false;
    CgOccur *prevVersion = nullptr;
};

class CgStoreOcc : public CgOccur {
public:
    CgStoreOcc(BB *bb, Insn *insn, Operand *opnd) : CgOccur(kOccStore, bb, insn, opnd) {}
    ~CgStoreOcc() = default;

    bool Reload() const
    {
        return needReload;
    }

    void SetReload(bool val)
    {
        needReload = val;
    }

    CgOccur *GetPrevVersionOccur() override
    {
        return prevVersion;
    }

    void SetPrevVersionOccur(CgOccur *val) override
    {
        prevVersion = val;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "StoreOcc " << GetClassID() << " at bb " << GetBB()->GetId() << ": "
                               << (needReload ? "reload, " : "not reload, ") << "\n";
    }

private:
    bool needReload = false;
    CgOccur *prevVersion = nullptr;
};

class CgDefOcc : public CgOccur {
public:
    CgDefOcc(BB *bb, Insn *insn, Operand *opnd) : CgOccur(kOccDef, bb, insn, opnd) {}
    ~CgDefOcc() = default;

    bool Loaded() const
    {
        return needStore;
    }

    void SetLoaded(bool val)
    {
        needStore = val;
    }

    CgOccur *GetPrevVersionOccur() override
    {
        return prevVersion;
    }

    void SetPrevVersionOccur(CgOccur *val) override
    {
        prevVersion = val;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "DefOcc " << GetClassID() << " at bb " << GetBB()->GetId() << ": "
                               << (needStore ? "store" : "not store") << "\n";
    }

private:
    bool needStore = false;
    CgOccur *prevVersion = nullptr;
};

class CgPhiOpndOcc;
enum AvailState { kFullyAvailable, kPartialAvailable, kNotAvailable };
class CgPhiOcc : public CgOccur {
public:
    CgPhiOcc(BB &bb, Operand *opnd, MapleAllocator &alloc)
        : CgOccur(kOccPhiocc, 0, bb, nullptr), regOpnd(opnd), isDownSafe(!bb.IsCatch()), phiOpnds(alloc.Adapter())
    {
    }

    virtual ~CgPhiOcc() = default;

    bool IsDownSafe() const
    {
        return isDownSafe;
    }

    void SetIsDownSafe(bool downSafe)
    {
        isDownSafe = downSafe;
    }

    const MapleVector<CgPhiOpndOcc *> &GetPhiOpnds() const
    {
        return phiOpnds;
    }

    MapleVector<CgPhiOpndOcc *> &GetPhiOpnds()
    {
        return phiOpnds;
    }

    Operand *GetOpnd()
    {
        return regOpnd;
    }

    CgPhiOpndOcc *GetPhiOpnd(size_t idx)
    {
        DEBUG_ASSERT(idx < phiOpnds.size(), "out of range in CgPhiOcc::GetPhiOpnd");
        return phiOpnds.at(idx);
    }

    const CgPhiOpndOcc *GetPhiOpnd(size_t idx) const
    {
        DEBUG_ASSERT(idx < phiOpnds.size(), "out of range in CgPhiOcc::GetPhiOpnd");
        return phiOpnds.at(idx);
    }

    void AddPhiOpnd(CgPhiOpndOcc &opnd)
    {
        phiOpnds.push_back(&opnd);
    }

    CgOccur *GetPrevVersionOccur() override
    {
        return prevVersion;
    }

    void SetPrevVersionOccur(CgOccur *val) override
    {
        prevVersion = val;
    }

    bool IsFullyAvailable() const
    {
        return availState == kFullyAvailable;
    }

    bool IsPartialAvailable() const
    {
        return availState == kPartialAvailable;
    }

    bool IsNotAvailable() const
    {
        return availState == kNotAvailable;
    }

    void SetAvailability(AvailState val)
    {
        availState = val;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "PhiOcc " << GetClassID() << " at bb " << GetBB()->GetId() << ": "
                               << (isDownSafe ? "downsafe, " : "not downsafe, ")
                               << (availState == kNotAvailable
                                       ? "not avail"
                                       : (availState == kPartialAvailable ? "part avail" : "fully avail"))
                               << "\n";
    }

private:
    Operand *regOpnd;
    bool isDownSafe = true;  // default is true
    AvailState availState = kFullyAvailable;
    MapleVector<CgPhiOpndOcc *> phiOpnds;
    CgOccur *prevVersion = nullptr;
};

class CgPhiOpndOcc : public CgOccur {
public:
    CgPhiOpndOcc(BB *bb, Operand *opnd, CgPhiOcc *defPhi)
        : CgOccur(kOccPhiopnd, bb, nullptr, opnd), hasRealUse(false), phiOcc(defPhi)
    {
    }

    ~CgPhiOpndOcc() = default;

    bool HasRealUse() const
    {
        return hasRealUse;
    }

    void SetHasRealUse(bool realUse)
    {
        hasRealUse = realUse;
    }

    const CgPhiOcc *GetPhiOcc() const
    {
        return phiOcc;
    }

    CgPhiOcc *GetPhiOcc()
    {
        return phiOcc;
    }

    void SetPhiOcc(CgPhiOcc &occ)
    {
        phiOcc = &occ;
    }

    bool Reload() const
    {
        return reload;
    }
    void SetReload(bool val)
    {
        reload = val;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "PhiOpndOcc " << GetClassID() << " at bb " << GetBB()->GetId() << ": "
                               << (hasRealUse ? "hasRealUse, " : "not hasRealUse, ")
                               << (reload ? "reload" : "not reload") << std::endl;
    }

private:
    bool hasRealUse;
    bool reload = false;
    CgPhiOcc *phiOcc = nullptr;  // its lhs
};

// each singly linked list represents each bucket in workCandHashTable
class PreWorkCand {
public:
    PreWorkCand(MapleAllocator &alloc, Operand *curOpnd, PUIdx pIdx)
        : next(nullptr),
          allOccs(alloc.Adapter()),
          realOccs(alloc.Adapter()),
          phiOccs(alloc.Adapter()),
          theOperand(curOpnd),
          puIdx(pIdx),
          redo2HandleCritEdges(false)
    {
        DEBUG_ASSERT(pIdx != 0, "PreWorkCand: initial puIdx cannot be 0");
    }

    virtual ~PreWorkCand() = default;

    void AddRealOccAsLast(CgOccur &occ, PUIdx pIdx)
    {
        realOccs.push_back(&occ);  // add as last
        DEBUG_ASSERT(pIdx != 0, "puIdx of realocc cannot be 0");
        if (pIdx != puIdx) {
            puIdx = 0;
        }
    }

    const PreWorkCand *GetNext() const
    {
        return next;
    }

    PreWorkCand *GetNext()
    {
        return next;
    }

    void SetNext(PreWorkCand &workCand)
    {
        next = &workCand;
    }

    int32 GetIndex() const
    {
        return index;
    }

    void SetIndex(int idx)
    {
        index = idx;
    }

    const MapleVector<CgOccur *> &GetRealOccs() const
    {
        return realOccs;
    }

    MapleVector<CgOccur *> &GetRealOccs()
    {
        return realOccs;
    }

    const CgOccur *GetRealOcc(size_t idx) const
    {
        DEBUG_ASSERT(idx < realOccs.size(), "out of range in PreWorkCand::GetRealOccAt");
        return realOccs.at(idx);
    }

    CgOccur *GetRealOcc(size_t idx)
    {
        DEBUG_ASSERT(idx < realOccs.size(), "out of range in PreWorkCand::GetRealOccAt");
        return realOccs.at(idx);
    }

    const MapleVector<CgPhiOcc *> &PhiOccs() const
    {
        return phiOccs;
    }

    MapleVector<CgPhiOcc *> &PhiOccs()
    {
        return phiOccs;
    }

    const Operand *GetTheOperand() const
    {
        return theOperand;
    }

    Operand *GetTheOperand()
    {
        return theOperand;
    }

    void SetTheOperand(Operand &expr)
    {
        theOperand = &expr;
    }

    PUIdx GetPUIdx() const
    {
        return puIdx;
    }

    void SetPUIdx(PUIdx idx)
    {
        puIdx = idx;
    }

    bool Redo2HandleCritEdges() const
    {
        return redo2HandleCritEdges;
    }

    void SetRedo2HandleCritEdges(bool redo)
    {
        redo2HandleCritEdges = redo;
    }

private:
    PreWorkCand *next;
    int32 index = 0;
    MapleVector<CgOccur *> allOccs;
    MapleVector<CgOccur *> realOccs;  // maintained in order of dt_preorder
    MapleVector<CgPhiOcc *> phiOccs;
    Operand *theOperand;  // the expression of this workcand
    PUIdx puIdx;          // if 0, its occ span multiple PUs; initial value must
    // puIdx cannot be 0 if hasLocalOpnd is true
    bool redo2HandleCritEdges : 1;  // redo to make critical edges affect canbevail
};

class PreWorkCandHashTable {
public:
    static const uint32 workCandHashLength = 229;
    static uint32 ComputeWorkCandHashIndex(const Operand &opnd);
    static uint32 ComputeStmtWorkCandHashIndex(const Insn &insn);

    PreWorkCandHashTable() = default;
    ~PreWorkCandHashTable() = default;

    std::array<PreWorkCand *, workCandHashLength> &GetWorkcandHashTable()
    {
        return workCandHashTable;
    }

    PreWorkCand *GetWorkcandFromIndex(size_t idx)
    {
        return workCandHashTable[idx];
    }

    void SetWorkCandAt(size_t idx, PreWorkCand &workCand)
    {
        workCandHashTable[idx] = &workCand;
    }

private:
    std::array<PreWorkCand *, workCandHashLength> workCandHashTable;
};
}  // namespace maplebe
#endif  // MAPLEBE_CG_INCLUDE_CGOCCUR_H
