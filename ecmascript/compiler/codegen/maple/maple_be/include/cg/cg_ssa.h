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

#ifndef MAPLEBE_CG_INCLUDE_CG_SSA_H
#define MAPLEBE_CG_INCLUDE_CG_SSA_H

#include "cgfunc.h"
#include "cg_dominance.h"
#include "live.h"
#include "operand.h"
#include "visitor_common.h"

namespace maplebe {
class CGSSAInfo;
enum SSAOpndDefBy { kDefByNo, kDefByInsn, kDefByPhi };

/* precise def/use info in machine instrcution */
class DUInsnInfo {
public:
    DUInsnInfo(Insn *cInsn, uint32 cIdx, MapleAllocator &alloc) : insn(cInsn), DUInfo(alloc.Adapter())
    {
        IncreaseDU(cIdx);
    }
    void IncreaseDU(uint32 idx)
    {
        if (!DUInfo.count(idx)) {
            DUInfo[idx] = 0;
        }
        DUInfo[idx]++;
    }
    void DecreaseDU(uint32 idx)
    {
        DEBUG_ASSERT(DUInfo[idx] > 0, "no def/use any more");
        DUInfo[idx]--;
    }
    void ClearDU(uint32 idx)
    {
        DEBUG_ASSERT(DUInfo.count(idx), "no def/use find");
        DUInfo[idx] = 0;
    }
    bool HasNoDU()
    {
        for (auto it : DUInfo) {
            if (it.second != 0) {
                return false;
            }
        }
        return true;
    }
    Insn *GetInsn()
    {
        return insn;
    }
    MapleMap<uint32, uint32> &GetOperands()
    {
        return DUInfo;
    }

private:
    Insn *insn;
    /* operand idx --- count */
    MapleMap<uint32, uint32> DUInfo;
};

class VRegVersion {
public:
    VRegVersion(const MapleAllocator &alloc, RegOperand &vReg, uint32 vIdx, regno_t vregNO)
        : versionAlloc(alloc),
          ssaRegOpnd(&vReg),
          versionIdx(vIdx),
          originalRegNO(vregNO),
          useInsnInfos(versionAlloc.Adapter())
    {
    }
    void SetDefInsn(DUInsnInfo *duInfo, SSAOpndDefBy defTy)
    {
        defInsnInfo = duInfo;
        defType = defTy;
    }
    DUInsnInfo *GetDefInsnInfo() const
    {
        return defInsnInfo;
    }
    SSAOpndDefBy GetDefType() const
    {
        return defType;
    }
    RegOperand *GetSSAvRegOpnd(bool isDef = true)
    {
        if (!isDef) {
            return implicitCvtedRegOpnd;
        }
        return ssaRegOpnd;
    }
    uint32 GetVersionIdx() const
    {
        return versionIdx;
    }
    regno_t GetOriginalRegNO() const
    {
        return originalRegNO;
    }
    void AddUseInsn(CGSSAInfo &ssaInfo, Insn &useInsn, uint32 idx);
    /* elimate dead use */
    void CheckDeadUse(const Insn &useInsn);
    void RemoveUseInsn(const Insn &useInsn, uint32 idx);
    MapleUnorderedMap<uint32, DUInsnInfo *> &GetAllUseInsns()
    {
        return useInsnInfos;
    }
    void MarkDeleted()
    {
        deleted = true;
    }
    void MarkRecovery()
    {
        deleted = false;
    }
    bool IsDeleted() const
    {
        return deleted;
    }
    void SetImplicitCvt()
    {
        hasImplicitCvt = true;
    }
    bool HasImplicitCvt() const
    {
        return hasImplicitCvt;
    }

private:
    MapleAllocator versionAlloc;
    /* if this version has implicit conversion, it refers to def reg */
    RegOperand *ssaRegOpnd;
    RegOperand *implicitCvtedRegOpnd = nullptr;
    uint32 versionIdx;
    regno_t originalRegNO;
    DUInsnInfo *defInsnInfo = nullptr;
    SSAOpndDefBy defType = kDefByNo;
    /* insn ID ->  insn* & operand Idx */
    // --> vector?
    MapleUnorderedMap<uint32, DUInsnInfo *> useInsnInfos;
    bool deleted = false;
    /*
     * def reg (size:64)  or       def reg (size:32)  -->
     * all use reg (size:32)       all use reg (size:64)
     * do not support single use which has implicit conversion yet
     * support single use in DUInfo in future
     */
    bool hasImplicitCvt = false;
};

class CGSSAInfo {
public:
    CGSSAInfo(CGFunc &f, DomAnalysis &da, MemPool &mp, MemPool &tmp)
        : cgFunc(&f),
          memPool(&mp),
          tempMp(&tmp),
          ssaAlloc(&mp),
          domInfo(&da),
          renamedBBs(ssaAlloc.Adapter()),
          vRegDefCount(ssaAlloc.Adapter()),
          vRegStk(ssaAlloc.Adapter()),
          allSSAOperands(ssaAlloc.Adapter()),
          noDefVRegs(ssaAlloc.Adapter()),
          reversePostOrder(ssaAlloc.Adapter()),
          safePropInsns(ssaAlloc.Adapter())
    {
    }
    virtual ~CGSSAInfo() = default;
    void ConstructSSA();
    VRegVersion *FindSSAVersion(regno_t ssaRegNO); /* Get specific ssa info */
    /* replace insn & update ssaInfo */
    virtual void ReplaceInsn(Insn &oriInsn, Insn &newInsn) = 0;
    virtual void ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) = 0;
    virtual void CreateNewInsnSSAInfo(Insn &newInsn) = 0;
    PhiOperand &CreatePhiOperand();

    DUInsnInfo *CreateDUInsnInfo(Insn *cInsn, uint32 idx)
    {
        return memPool->New<DUInsnInfo>(cInsn, idx, ssaAlloc);
    }
    const MapleUnorderedMap<regno_t, VRegVersion *> &GetAllSSAOperands() const
    {
        return allSSAOperands;
    }
    bool IsNoDefVReg(regno_t vRegNO) const
    {
        return noDefVRegs.find(vRegNO) != noDefVRegs.end();
    }
    uint32 GetVersionNOOfOriginalVreg(regno_t vRegNO)
    {
        if (vRegDefCount.count(vRegNO)) {
            return vRegDefCount[vRegNO];
        }
        DEBUG_ASSERT(false, " original vreg is not existed");
        return 0;
    }
    MapleVector<uint32> &GetReversePostOrder()
    {
        return reversePostOrder;
    }
    void InsertSafePropInsn(uint32 insnId)
    {
        (void)safePropInsns.emplace_back(insnId);
    }
    MapleVector<uint32> &GetSafePropInsns()
    {
        return safePropInsns;
    }
    Insn *GetDefInsn(const RegOperand &useReg);
    void DumpFuncCGIRinSSAForm() const;
    virtual void DumpInsnInSSAForm(const Insn &insn) const = 0;
    static uint32 SSARegNObase;

protected:
    VRegVersion *CreateNewVersion(RegOperand &virtualOpnd, Insn &defInsn, uint32 idx, bool isDefByPhi = false);
    virtual RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) = 0;
    bool IncreaseSSAOperand(regno_t vRegNO, VRegVersion *vst);
    uint32 IncreaseVregCount(regno_t vRegNO);
    VRegVersion *GetVersion(const RegOperand &virtualOpnd);
    MapleUnorderedMap<regno_t, VRegVersion *> &GetPrivateAllSSAOperands()
    {
        return allSSAOperands;
    }
    void AddNoDefVReg(regno_t noDefVregNO)
    {
        DEBUG_ASSERT(!noDefVRegs.count(noDefVregNO), "duplicate no def Reg, please check");
        noDefVRegs.emplace(noDefVregNO);
    }
    void MarkInsnsInSSA(Insn &insn);
    CGFunc *cgFunc = nullptr;
    MemPool *memPool = nullptr;
    MemPool *tempMp = nullptr;
    MapleAllocator ssaAlloc;

private:
    void InsertPhiInsn();
    void RenameVariablesForBB(uint32 bbID);
    void RenameBB(BB &bb);
    void RenamePhi(BB &bb);
    virtual void RenameInsn(Insn &insn) = 0;
    /* build ssa on virtual register only */
    virtual RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn, uint32 idx) = 0;
    void RenameSuccPhiUse(const BB &bb);
    void PrunedPhiInsertion(const BB &bb, RegOperand &virtualOpnd);

    void AddRenamedBB(uint32 bbID)
    {
        DEBUG_ASSERT(!renamedBBs.count(bbID), "cgbb has been renamed already");
        renamedBBs.emplace(bbID);
    }
    bool IsBBRenamed(uint32 bbID) const
    {
        return renamedBBs.count(bbID);
    }
    void SetReversePostOrder();

    DomAnalysis *domInfo = nullptr;
    MapleSet<uint32> renamedBBs;
    /* original regNO - number of definitions (start from 0) */
    MapleMap<regno_t, uint32> vRegDefCount;
    /* original regNO - ssa version stk */
    MapleMap<regno_t, MapleStack<VRegVersion *>> vRegStk;
    /* ssa regNO - ssa virtual operand version */
    MapleUnorderedMap<regno_t, VRegVersion *> allSSAOperands;
    /* For virtual registers which do not have definition */
    MapleSet<regno_t> noDefVRegs;
    /* only save bb_id to reduce space */
    MapleVector<uint32> reversePostOrder;
    /* destSize < srcSize but can be propagated */
    MapleVector<uint32> safePropInsns;
    int32 insnCount = 0;
};

class SSAOperandVisitor : public OperandVisitorBase, public OperandVisitors<RegOperand, ListOperand, MemOperand> {
public:
    SSAOperandVisitor(Insn &cInsn, const OpndDesc &cDes, uint32 idx) : insn(&cInsn), opndDes(&cDes), idx(idx) {}
    SSAOperandVisitor() = default;
    virtual ~SSAOperandVisitor() = default;
    void SetInsnOpndInfo(Insn &cInsn, const OpndDesc &cDes, uint32 index)
    {
        insn = &cInsn;
        opndDes = &cDes;
        this->idx = index;
    }

protected:
    Insn *insn = nullptr;
    const OpndDesc *opndDes = nullptr;
    uint32 idx = 0;
};

class SSAOperandDumpVisitor : public OperandVisitorBase,
                              public OperandVisitors<RegOperand, ListOperand, MemOperand>,
                              public OperandVisitor<PhiOperand> {
public:
    explicit SSAOperandDumpVisitor(const MapleUnorderedMap<regno_t, VRegVersion *> &allssa) : allSSAOperands(allssa) {}
    virtual ~SSAOperandDumpVisitor() = default;
    void SetHasDumped()
    {
        hasDumped = true;
    }
    bool HasDumped() const
    {
        return hasDumped;
    }
    bool hasDumped = false;

protected:
    const MapleUnorderedMap<regno_t, VRegVersion *> &allSSAOperands;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgSSAConstruct, maplebe::CGFunc);
CGSSAInfo *GetResult()
{
    return ssaInfo;
}
CGSSAInfo *ssaInfo = nullptr;

private:
void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe

#endif  // MAPLEBE_CG_INCLUDE_CG_SSA_H
