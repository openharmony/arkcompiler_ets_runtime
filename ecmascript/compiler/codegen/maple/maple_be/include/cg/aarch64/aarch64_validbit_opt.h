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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_VALIDBIT_OPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_VALIDBIT_OPT_H

#include "cg_validbit_opt.h"
#include "operand.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
class AArch64ValidBitOpt : public ValidBitOpt {
public:
    AArch64ValidBitOpt(CGFunc &f, CGSSAInfo &info) : ValidBitOpt(f, info) {}
    ~AArch64ValidBitOpt() override = default;

    void DoOpt(BB &bb, Insn &insn) override;
    void SetValidBits(Insn &insn) override;
    bool SetPhiValidBits(Insn &insn) override;
};

/*
 * Example 1)
 * def w9                          def w9
 * ...                             ...
 * and w4, w9, #255       ===>     mov w4, w9
 *
 * Example 2)
 * and w6[16], w0[16], #FF00[16]              mov  w6, w0
 * asr w6,     w6[16], #8[4]        ===>      asr  w6, w6
 */
class AndValidBitPattern : public ValidBitPattern {
public:
    AndValidBitPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
    ~AndValidBitPattern() override
    {
        desReg = nullptr;
        srcReg = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "AndValidBitPattern";
    }

private:
    bool CheckImmValidBit(int64 andImm, uint32 andImmVB, int64 shiftImm) const;
    MOperator newMop = MOP_undef;
    RegOperand *desReg = nullptr;
    RegOperand *srcReg = nullptr;
};

/*
 * Example 1)
 * uxth  w1[16], w2[16]  /  uxtb  w1[8], w2[8]
 * ===>
 * mov   w1, w2
 *
 * Example 2)
 * ubfx  w1, w2[16], #0, #16  /  sbfx  w1, w2[16], #0, #16
 * ===>
 * mov   w1, w2
 */
class ExtValidBitPattern : public ValidBitPattern {
public:
    ExtValidBitPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
    ~ExtValidBitPattern() override
    {
        newDstOpnd = nullptr;
        newSrcOpnd = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ExtValidBitPattern";
    }

private:
    RegOperand *newDstOpnd = nullptr;
    RegOperand *newSrcOpnd = nullptr;
    MOperator newMop = MOP_undef;
};

/*
 *  cmp  w0, #0
 *  cset w1, NE --> mov w1, w0
 *
 *  cmp  w0, #0
 *  cset w1, EQ --> eor w1, w0, 1
 *
 *  cmp  w0, #1
 *  cset w1, NE --> eor w1, w0, 1
 *
 *  cmp  w0, #1
 *  cset w1, EQ --> mov w1, w0
 *
 *  cmp w0,  #0
 *  cset w0, NE -->null
 *
 *  cmp w0, #1
 *  cset w0, EQ -->null
 *
 *  condition:
 *    1. the first operand of cmp instruction must has only one valid bit
 *    2. the second operand of cmp instruction must be 0 or 1
 *    3. flag register of cmp isntruction must not be used later
 */
class CmpCsetVBPattern : public ValidBitPattern {
public:
    CmpCsetVBPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
    ~CmpCsetVBPattern() override
    {
        cmpInsn = nullptr;
    }
    void Run(BB &bb, Insn &csetInsn) override;
    bool CheckCondition(Insn &csetInsn) override;
    std::string GetPatternName() override
    {
        return "CmpCsetPattern";
    };

private:
    bool IsContinuousCmpCset(const Insn &curInsn);
    bool OpndDefByOneValidBit(const Insn &defInsn);
    Insn *cmpInsn = nullptr;
    int64 cmpConstVal = -1;
};

/*
 * cmp w0[16], #32768
 * bge label           ===>   tbnz w0, #15, label
 *
 * bge / blt
 */
class CmpBranchesPattern : public ValidBitPattern {
public:
    CmpBranchesPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
    ~CmpBranchesPattern() override
    {
        prevCmpInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CmpBranchesPattern";
    };

private:
    void SelectNewMop(MOperator mop);
    Insn *prevCmpInsn = nullptr;
    int64 newImmVal = -1;
    MOperator newMop = MOP_undef;
    bool is64Bit = false;
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_VALIDBIT_OPT_H */
