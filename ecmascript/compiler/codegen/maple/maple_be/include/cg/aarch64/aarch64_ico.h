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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H
#include "ico.h"
#include "aarch64_isa.h"
#include "optimize_common.h"
#include "live.h"

namespace maplebe {
class AArch64IfConversionOptimizer : public IfConversionOptimizer {
public:
    AArch64IfConversionOptimizer(CGFunc &func, MemPool &memPool) : IfConversionOptimizer(func, memPool) {}

    ~AArch64IfConversionOptimizer() override = default;
    void InitOptimizePatterns() override;
};

class AArch64ICOPattern : public ICOPattern {
public:
    explicit AArch64ICOPattern(CGFunc &func) : ICOPattern(func) {}
    ~AArch64ICOPattern() override = default;

protected:
    ConditionCode Encode(MOperator mOp, bool inverse) const;
    Insn *BuildCmpInsn(const Insn &condBr) const;
    Insn *BuildCcmpInsn(ConditionCode ccCode, const Insn *cmpInsn) const;
    Insn *BuildCondSet(const Insn &branch, RegOperand &reg, bool inverse) const;
    Insn *BuildCondSel(const Insn &branch, MOperator mOp, RegOperand &dst, RegOperand &src1, RegOperand &src2) const;
    bool IsSetInsn(const Insn &insn, Operand *&dest, std::vector<Operand *> &src) const;
    static uint32 GetNZCV(ConditionCode ccCode, bool inverse);
    bool CheckMop(MOperator mOperator) const;
};

/* If-Then-Else pattern */
class AArch64ICOIfThenElsePattern : public AArch64ICOPattern {
public:
    explicit AArch64ICOIfThenElsePattern(CGFunc &func) : AArch64ICOPattern(func) {}
    ~AArch64ICOIfThenElsePattern() override = default;
    bool Optimize(BB &curBB) override
    {
        return true;
    }

protected:
    bool BuildCondMovInsn(BB &cmpBB, const BB &bb, const std::map<Operand *, std::vector<Operand *>> &ifDestSrcMap,
                          const std::map<Operand *, std::vector<Operand *>> &elseDestSrcMap, bool elseBBIsProcessed,
                          std::vector<Insn *> &generateInsn);
    void GenerateInsnForImm(const Insn &branchInsn, Operand &ifDest, Operand &elseDest, RegOperand &destReg,
                            std::vector<Insn *> &generateInsn);
    Operand *GetDestReg(const std::map<Operand *, std::vector<Operand *>> &destSrcMap, const RegOperand &destReg) const;
    void GenerateInsnForReg(const Insn &branchInsn, Operand &ifDest, Operand &elseDest, RegOperand &destReg,
                            std::vector<Insn *> &generateInsn);
    RegOperand *GenerateRegAndTempInsn(Operand &dest, const RegOperand &destReg,
                                       std::vector<Insn *> &generateInsn) const;
    bool CheckHasSameDest(std::vector<Insn *> &lInsn, std::vector<Insn *> &rInsn) const;
    bool CheckCondMoveBB(BB *bb, std::map<Operand *, std::vector<Operand *>> &destSrcMap,
                         std::vector<Operand *> &destRegs, std::vector<Insn *> &setInsn, Operand *flagReg,
                         Insn *cmpInsn) const;
};

/* If-Then MorePreds pattern
 *
 * .L.891__92:                                             .L.891__92:
 * cmp     x4, w0, UXTW                                    cmp     x4, w0, UXTW
 * bls     .L.891__41                                      csel    x0, x2, x0, LS
 * .L.891__42:                                             bls     .L.891__94
 * sub     x0, x4, w0, UXTW           =====>               .L.891__42:
 * cmp     x0, x2                                          sub     x0, x4, w0, UXTW
 * bls     .L.891__41                                      cmp     x0, x2
 * ......                                                  csel    x0, x2, x0, LS
 * .L.891__41:                                             bls     .L.891__94
 * mov     x0, x2
 * b       .L.891__94
 * */
class AArch64ICOMorePredsPattern : public AArch64ICOPattern {
public:
    explicit AArch64ICOMorePredsPattern(CGFunc &func) : AArch64ICOPattern(func) {}
    ~AArch64ICOMorePredsPattern() override = default;
    bool Optimize(BB &curBB) override
    {
        return true;
    }

protected:
    bool CheckGotoBB(BB &gotoBB, std::vector<Insn *> &movInsn) const;
    bool MovToCsel(std::vector<Insn *> &movInsn, std::vector<Insn *> &cselInsn, const Insn &branchInsn) const;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H */
