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

#include "cg_phi_elimination.h"
#include "cg.h"
#include "cgbb.h"

namespace maplebe {
void PhiEliminate::TranslateTSSAToCSSA()
{
    FOR_ALL_BB(bb, cgFunc) {
        eliminatedBB.emplace(bb->GetId());
        for (auto phiInsnIt : bb->GetPhiInsns()) {
            /* Method I create a temp move for phi-node */
            auto &destReg = static_cast<RegOperand &>(phiInsnIt.second->GetOperand(kInsnFirstOpnd));
            RegOperand &tempMovDest = cgFunc->GetOrCreateVirtualRegisterOperand(CreateTempRegForCSSA(destReg));
            auto &phiList = static_cast<PhiOperand &>(phiInsnIt.second->GetOperand(kInsnSecondOpnd));
            for (auto phiOpndIt : phiList.GetOperands()) {
                uint32 fBBId = phiOpndIt.first;
                DEBUG_ASSERT(fBBId != 0, "GetFromBBID = 0");
#if DEBUG
                bool find = false;
                for (auto predBB : bb->GetPreds()) {
                    if (predBB->GetId() == fBBId) {
                        find = true;
                    }
                }
                CHECK_FATAL(find, "dont exited pred for phi-node");
#endif
                PlaceMovInPredBB(fBBId, CreateMov(tempMovDest, *(phiOpndIt.second)));
            }
            if (!destReg.IsOfCC()) {
                Insn &movInsn = CreateMov(destReg, tempMovDest);
                bb->ReplaceInsn(*phiInsnIt.second, movInsn);
            } else {
                bb->RemoveInsn(*phiInsnIt.second);
            }
        }
    }

    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            CHECK_FATAL(eliminatedBB.count(bb->GetId()), "still have phi");
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            ReCreateRegOperand(*insn);
            bb->GetPhiInsns().clear();
        }
    }
    UpdateRematInfo();
    cgFunc->SetSSAvRegCount(0);
}

void PhiEliminate::UpdateRematInfo()
{
    if (CGOptions::GetRematLevel() > 0) {
        cgFunc->UpdateAllRegisterVregMapping(remateInfoAfterSSA);
    }
}

void PhiEliminate::PlaceMovInPredBB(uint32 predBBId, Insn &movInsn)
{
    BB *predBB = cgFunc->GetBBFromID(predBBId);
    DEBUG_ASSERT(movInsn.GetOperand(kInsnSecondOpnd).IsRegister(), "unexpect operand");
    if (predBB->GetKind() == BB::kBBFallthru) {
        predBB->AppendInsn(movInsn);
    } else {
        AppendMovAfterLastVregDef(*predBB, movInsn);
    }
}

regno_t PhiEliminate::GetAndIncreaseTempRegNO()
{
    while (GetSSAInfo()->GetAllSSAOperands().count(tempRegNO)) {
        tempRegNO++;
    }
    regno_t ori = tempRegNO;
    tempRegNO++;
    return ori;
}

RegOperand *PhiEliminate::MakeRoomForNoDefVreg(RegOperand &conflictReg)
{
    regno_t conflictVregNO = conflictReg.GetRegisterNumber();
    auto rVregIt = replaceVreg.find(conflictVregNO);
    if (rVregIt != replaceVreg.end()) {
        return rVregIt->second;
    } else {
        RegOperand *regForRecreate = &CreateTempRegForCSSA(conflictReg);
        (void)replaceVreg.emplace(std::pair<regno_t, RegOperand *>(conflictVregNO, regForRecreate));
        return regForRecreate;
    }
}

void PhiEliminate::RecordRematInfo(regno_t vRegNO, PregIdx pIdx)
{
    if (remateInfoAfterSSA.count(vRegNO)) {
        if (remateInfoAfterSSA[vRegNO] != pIdx) {
            remateInfoAfterSSA.erase(vRegNO);
        }
    } else {
        (void)remateInfoAfterSSA.emplace(std::pair<regno_t, PregIdx>(vRegNO, pIdx));
    }
}

bool CgPhiElimination::PhaseRun(maplebe::CGFunc &f)
{
    CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
    PhiEliminate *pe = f.GetCG()->CreatePhiElimintor(*GetPhaseMemPool(), f, *ssaInfo);
    pe->TranslateTSSAToCSSA();
    return false;
}
void CgPhiElimination::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgPhiElimination, cgphielimination)
}  // namespace maplebe
