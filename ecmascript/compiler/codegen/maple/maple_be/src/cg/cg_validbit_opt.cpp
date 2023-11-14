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

#include "cg_validbit_opt.h"
#include "mempool.h"
#include "aarch64_validbit_opt.h"

namespace maplebe {
Insn *ValidBitPattern::GetDefInsn(const RegOperand &useReg)
{
    if (!useReg.IsSSAForm()) {
        return nullptr;
    }
    regno_t useRegNO = useReg.GetRegisterNumber();
    VRegVersion *useVersion = ssaInfo->FindSSAVersion(useRegNO);
    DEBUG_ASSERT(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
    CHECK_FATAL(!useVersion->IsDeleted(), "deleted version");
    DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
    return defInfo == nullptr ? nullptr : defInfo->GetInsn();
}

InsnSet ValidBitPattern::GetAllUseInsn(const RegOperand &defReg)
{
    InsnSet allUseInsn;
    if ((ssaInfo != nullptr) && defReg.IsSSAForm()) {
        VRegVersion *defVersion = ssaInfo->FindSSAVersion(defReg.GetRegisterNumber());
        CHECK_FATAL(defVersion != nullptr, "useVRegVersion must not be null based on ssa");
        for (auto insnInfo : defVersion->GetAllUseInsns()) {
            Insn *currInsn = insnInfo.second->GetInsn();
            allUseInsn.emplace(currInsn);
        }
    }
    return allUseInsn;
}

void ValidBitPattern::DumpAfterPattern(std::vector<Insn *> &prevInsns, const Insn *replacedInsn, const Insn *newInsn)
{
    LogInfo::MapleLogger() << ">>>>>>> In " << GetPatternName() << " : <<<<<<<\n";
    if (!prevInsns.empty()) {
        if ((replacedInsn == nullptr) && (newInsn == nullptr)) {
            LogInfo::MapleLogger() << "======= RemoveInsns : {\n";
        } else {
            LogInfo::MapleLogger() << "======= PrevInsns : {\n";
        }
        for (auto *prevInsn : prevInsns) {
            if (prevInsn != nullptr) {
                LogInfo::MapleLogger() << "[primal form] ";
                prevInsn->Dump();
                if (ssaInfo != nullptr) {
                    LogInfo::MapleLogger() << "[ssa form] ";
                    ssaInfo->DumpInsnInSSAForm(*prevInsn);
                }
            }
        }
        LogInfo::MapleLogger() << "}\n";
    }
    if (replacedInsn != nullptr) {
        LogInfo::MapleLogger() << "======= OldInsn :\n";
        LogInfo::MapleLogger() << "[primal form] ";
        replacedInsn->Dump();
        if (ssaInfo != nullptr) {
            LogInfo::MapleLogger() << "[ssa form] ";
            ssaInfo->DumpInsnInSSAForm(*replacedInsn);
        }
    }
    if (newInsn != nullptr) {
        LogInfo::MapleLogger() << "======= NewInsn :\n";
        LogInfo::MapleLogger() << "[primal form] ";
        newInsn->Dump();
        if (ssaInfo != nullptr) {
            LogInfo::MapleLogger() << "[ssa form] ";
            ssaInfo->DumpInsnInSSAForm(*newInsn);
        }
    }
}

void ValidBitOpt::RectifyValidBitNum()
{
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            SetValidBits(*insn);
        }
    }
    bool iterate;
    /* Use inverse postorder to converge with minimal iterations */
    do {
        iterate = false;
        MapleVector<uint32> reversePostOrder = ssaInfo->GetReversePostOrder();
        for (uint32 bbId : reversePostOrder) {
            BB *bb = cgFunc->GetBBFromID(bbId);
            FOR_BB_INSNS(insn, bb) {
                if (!insn->IsPhi()) {
                    continue;
                }
                bool change = SetPhiValidBits(*insn);
                if (change) {
                    /* if vb changes once, iterate. */
                    iterate = true;
                }
            }
        }
    } while (iterate);
}

void ValidBitOpt::RecoverValidBitNum()
{
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction() && !insn->IsPhi()) {
                continue;
            }
            uint32 opndNum = insn->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                if (insn->OpndIsDef(i) && insn->GetOperand(i).IsRegister()) {
                    auto &dstOpnd = static_cast<RegOperand &>(insn->GetOperand(i));
                    dstOpnd.SetValidBitsNum(dstOpnd.GetSize());
                }
            }
        }
    }
}

void ValidBitOpt::Run()
{
    /*
     * Set validbit of regOpnd before optimization
     */
    RectifyValidBitNum();
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            DoOpt(*bb, *insn);
        }
    }
    /*
     * Recover validbit of regOpnd after optimization
     */
    RecoverValidBitNum();
}

bool CgValidBitOpt::PhaseRun(maplebe::CGFunc &f)
{
    CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
    CHECK_FATAL(ssaInfo != nullptr, "Get ssaInfo failed");
    auto *vbOpt = f.GetCG()->CreateValidBitOpt(*GetPhaseMemPool(), f, *ssaInfo);
    CHECK_FATAL(vbOpt != nullptr, "vbOpt instance create failed");
    vbOpt->Run();
    return true;
}

void CgValidBitOpt::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.AddRequired<CgSSAConstruct>();
    aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgValidBitOpt, cgvalidbitopt)
} /* namespace maplebe */
