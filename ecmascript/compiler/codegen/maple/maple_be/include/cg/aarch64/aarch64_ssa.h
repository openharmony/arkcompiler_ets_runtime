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

#ifndef MAPLEBE_CG_INCLUDE_AARCH64_SSA_H
#define MAPLEBE_CG_INCLUDE_AARCH64_SSA_H

#include "cg_ssa.h"
#include "aarch64_insn.h"

namespace maplebe {
class AArch64CGSSAInfo : public CGSSAInfo {
public:
    AArch64CGSSAInfo(CGFunc &f, DomAnalysis &da, MemPool &mp, MemPool &tmp) : CGSSAInfo(f, da, mp, tmp) {}
    ~AArch64CGSSAInfo() override = default;
    void DumpInsnInSSAForm(const Insn &insn) const override;
    RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn, uint32 idx) override;
    MemOperand *CreateMemOperand(MemOperand &memOpnd, bool isOnSSA /* false = on cgfunc */);
    void ReplaceInsn(Insn &oriInsn, Insn &newInsn) override;
    void ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) override;
    void CreateNewInsnSSAInfo(Insn &newInsn) override;

private:
    void RenameInsn(Insn &insn) override;
    VRegVersion *RenamedOperandSpecialCase(RegOperand &vRegOpnd, Insn &curInsn, uint32 idx);
    RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) override;
    void CheckAsmDUbinding(Insn &insn, const VRegVersion *toBeReplaced, VRegVersion *newVersion);
};

class A64SSAOperandRenameVisitor : public SSAOperandVisitor {
public:
    A64SSAOperandRenameVisitor(AArch64CGSSAInfo &cssaInfo, Insn &cInsn, const OpndDesc &cProp, uint32 idx)
        : SSAOperandVisitor(cInsn, cProp, idx), ssaInfo(&cssaInfo)
    {
    }
    ~A64SSAOperandRenameVisitor() override = default;
    void Visit(RegOperand *v) final;
    void Visit(ListOperand *v) final;
    void Visit(MemOperand *a64MemOpnd) final;

private:
    AArch64CGSSAInfo *ssaInfo;
};

class A64OpndSSAUpdateVsitor : public SSAOperandVisitor, public OperandVisitor<PhiOperand> {
public:
    explicit A64OpndSSAUpdateVsitor(AArch64CGSSAInfo &cssaInfo) : ssaInfo(&cssaInfo) {}
    ~A64OpndSSAUpdateVsitor() override = default;
    void MarkIncrease()
    {
        isDecrease = false;
    };
    void MarkDecrease()
    {
        isDecrease = true;
    };
    bool HasDeleteDef() const
    {
        return !deletedDef.empty();
    }
    void Visit(RegOperand *regOpnd) final;
    void Visit(ListOperand *v) final;
    void Visit(MemOperand *a64MemOpnd) final;
    void Visit(PhiOperand *phiOpnd) final;

    bool IsPhi() const
    {
        return isPhi;
    }

    void SetPhi(bool flag)
    {
        isPhi = flag;
    }

private:
    void UpdateRegUse(uint32 ssaIdx);
    void UpdateRegDef(uint32 ssaIdx);
    AArch64CGSSAInfo *ssaInfo;
    bool isDecrease = false;
    std::set<regno_t> deletedDef;
    bool isPhi = false;
};

class A64SSAOperandDumpVisitor : public SSAOperandDumpVisitor {
public:
    explicit A64SSAOperandDumpVisitor(const MapleUnorderedMap<regno_t, VRegVersion *> &allssa)
        : SSAOperandDumpVisitor(allssa) {};
    ~A64SSAOperandDumpVisitor() override = default;
    void Visit(RegOperand *a64RegOpnd) final;
    void Visit(ListOperand *v) final;
    void Visit(MemOperand *a64MemOpnd) final;
    void Visit(PhiOperand *phi) final;
};
}  // namespace maplebe

#endif  // MAPLEBE_CG_INCLUDE_AARCH64_SSA_H
