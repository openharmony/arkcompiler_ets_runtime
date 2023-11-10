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

#ifndef MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H
#define MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H

#include "cgfunc.h"
#include "cg_ssa.h"

namespace maplebe {
class PhiEliminate {
public:
    PhiEliminate(CGFunc &f, CGSSAInfo &ssaAnalysisResult, MemPool &mp)
        : cgFunc(&f),
          ssaInfo(&ssaAnalysisResult),
          phiEliAlloc(&mp),
          eliminatedBB(phiEliAlloc.Adapter()),
          replaceVreg(phiEliAlloc.Adapter()),
          remateInfoAfterSSA(phiEliAlloc.Adapter())
    {
        tempRegNO = static_cast<uint32_t>(GetSSAInfo()->GetAllSSAOperands().size()) + CGSSAInfo::SSARegNObase;
    }
    virtual ~PhiEliminate() = default;
    CGSSAInfo *GetSSAInfo()
    {
        return ssaInfo;
    }
    void TranslateTSSAToCSSA();
    /* move ssaRegOperand from ssaInfo to cgfunc */
    virtual void ReCreateRegOperand(Insn &insn) = 0;

protected:
    virtual Insn &CreateMov(RegOperand &destOpnd, RegOperand &fromOpnd) = 0;
    virtual void MaintainRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd, bool isCopy) = 0;
    virtual void AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const = 0;
    void UpdateRematInfo();
    regno_t GetAndIncreaseTempRegNO();
    RegOperand *MakeRoomForNoDefVreg(RegOperand &conflictReg);
    void RecordRematInfo(regno_t vRegNO, PregIdx pIdx);
    PregIdx FindRematInfo(regno_t vRegNO)
    {
        return remateInfoAfterSSA.count(vRegNO) ? remateInfoAfterSSA[vRegNO] : -1;
    }
    CGFunc *cgFunc;
    CGSSAInfo *ssaInfo;
    MapleAllocator phiEliAlloc;

private:
    void PlaceMovInPredBB(uint32 predBBId, Insn &movInsn);
    virtual RegOperand &CreateTempRegForCSSA(RegOperand &oriOpnd) = 0;
    MapleSet<uint32> eliminatedBB;
    /*
     * noDef Vregs occupy the vregno_t which is used for ssa re_creating
     * first : conflicting VReg with noDef VReg  second : new_Vreg opnd to replace occupied Vreg
     */
    MapleUnorderedMap<regno_t, RegOperand *> replaceVreg;
    regno_t tempRegNO = 0; /* use for create mov insn for phi */
    MapleMap<regno_t, PregIdx> remateInfoAfterSSA;
};

class OperandPhiElmVisitor : public OperandVisitorBase, public OperandVisitors<RegOperand, ListOperand, MemOperand> {
};

MAPLE_FUNC_PHASE_DECLARE(CgPhiElimination, maplebe::CGFunc)
}  // namespace maplebe

#endif  // MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H
