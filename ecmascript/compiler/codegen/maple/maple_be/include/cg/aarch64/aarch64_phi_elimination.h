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

#ifndef MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
#define MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
#include "cg_phi_elimination.h"
namespace maplebe {
class AArch64PhiEliminate : public PhiEliminate {
public:
    AArch64PhiEliminate(CGFunc &f, CGSSAInfo &ssaAnalysisResult, MemPool &mp) : PhiEliminate(f, ssaAnalysisResult, mp)
    {
    }
    ~AArch64PhiEliminate() override = default;
    RegOperand &GetCGVirtualOpearnd(RegOperand &ssaOpnd, const Insn &curInsn /* for remat */);

private:
    void ReCreateRegOperand(Insn &insn) override;
    Insn &CreateMov(RegOperand &destOpnd, RegOperand &fromOpnd) override;
    void MaintainRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd, bool isCopy) override;
    RegOperand &CreateTempRegForCSSA(RegOperand &oriOpnd) override;
    void AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const override;
};

class A64OperandPhiElmVisitor : public OperandPhiElmVisitor {
public:
    A64OperandPhiElmVisitor(AArch64PhiEliminate *a64PhiElm, Insn &cInsn, uint32 idx)
        : a64PhiEliminator(a64PhiElm), insn(&cInsn), idx(idx) {};
    ~A64OperandPhiElmVisitor() override = default;
    void Visit(RegOperand *v) final;
    void Visit(ListOperand *v) final;
    void Visit(MemOperand *v) final;

private:
    AArch64PhiEliminate *a64PhiEliminator;
    Insn *insn;
    uint32 idx;
};
}  // namespace maplebe
#endif  // MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
