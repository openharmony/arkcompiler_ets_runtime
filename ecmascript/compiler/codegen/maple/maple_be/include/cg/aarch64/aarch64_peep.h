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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H

#include <vector>
#include "peep.h"
#include "aarch64_cg.h"
#include "optimize_common.h"
#include "mir_builder.h"

namespace maplebe {
class AArch64CGPeepHole : CGPeepHole {
public:
    /* normal constructor */
    AArch64CGPeepHole(CGFunc &f, MemPool *memPool) : CGPeepHole(f, memPool) {};
    /* constructor for ssa */
    AArch64CGPeepHole(CGFunc &f, MemPool *memPool, CGSSAInfo *cgssaInfo) : CGPeepHole(f, memPool, cgssaInfo) {};
    ~AArch64CGPeepHole() = default;

    void Run() override;
    bool DoSSAOptimize(BB &bb, Insn &insn) override;
    void DoNormalOptimize(BB &bb, Insn &insn) override;
};

/*
 * i.   cmp     x0, x1
 *      cset    w0, EQ     ===>   cmp x0, x1
 *      cmp     w0, #0            cset w0, EQ
 *      cset    w0, NE
 *
 * ii.  cmp     x0, x1
 *      cset    w0, EQ     ===>   cmp x0, x1
 *      cmp     w0, #0            cset w0, NE
 *      cset    w0, EQ
 */
class ContinuousCmpCsetPattern : public CGPeepPattern {
public:
    ContinuousCmpCsetPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~ContinuousCmpCsetPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ContinuousCmpCsetPattern";
    }

private:
    bool CheckCondCode(const CondOperand &condOpnd) const;
    Insn *prevCmpInsn = nullptr;
    Insn *prevCsetInsn1 = nullptr;
    Insn *prevCmpInsn1 = nullptr;
    bool reverse = false;
};

/*
 * Example 1)
 *  mov w5, #1
 *   ...
 *  mov w0, #0
 *  csel w5, w5, w0, NE    ===> cset w5, NE
 *
 * Example 2)
 *  mov w5, #0
 *   ...
 *  mov w0, #1
 *  csel w5, w5, w0, NE    ===> cset w5,EQ
 *
 * conditions:
 * 1. mov_imm1 value is 0(1) && mov_imm value is 1(0)
 */
class CselToCsetPattern : public CGPeepPattern {
public:
    CselToCsetPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~CselToCsetPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CselToCsetPattern";
    }

private:
    bool IsOpndDefByZero(const Insn &insn) const;
    bool IsOpndDefByOne(const Insn &insn) const;
    Insn *prevMovInsn1 = nullptr;
    Insn *prevMovInsn2 = nullptr;
};

/*
 * combine cset & cbz/cbnz ---> beq/bne
 * Example 1)
 *  cset    w0, EQ            or       cset    w0, NE
 *  cbnz    w0, .label                 cbnz    w0, .label
 *  ===> beq .label                    ===> bne .label
 *
 * Case: same conditon_code
 *
 * Example 2)
 *  cset    w0, EQ            or       cset    w0, NE
 *  cbz     w0, .label                 cbz    w0, .label
 *  ===> bne .label                    ===> beq .label
 *
 * Case: reversed condition_code
 */
class CsetCbzToBeqPattern : public CGPeepPattern {
public:
    CsetCbzToBeqPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~CsetCbzToBeqPattern() override = default;
    std::string GetPatternName() override
    {
        return "CsetCbzToBeqPattern";
    }
    bool CheckCondition(Insn &insn) override;
    void Run(BB &bb, Insn &insn) override;

private:
    MOperator SelectNewMop(ConditionCode condCode, bool inverse) const;
    Insn *prevInsn = nullptr;
};

/*
 * combine neg & cmp --> cmn
 * Example 1)
 *  neg x0, x6
 *  cmp x2, x0                --->    (currInsn)
 *  ===> cmn x2, x6
 *
 * Example 2)
 *  neg x0, x6, LSL #5
 *  cmp x2, x0                --->    (currInsn)
 *  ===> cmn x2, x6, LSL #5
 *
 * Conditions:
 * 1. neg_amount_val is valid in cmn amount range
 */
class NegCmpToCmnPattern : public CGPeepPattern {
public:
    NegCmpToCmnPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~NegCmpToCmnPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "NegCmpToCmnPattern";
    }

private:
    Insn *prevInsn = nullptr;
};

/*
 * combine {sxtw / uxtw} & lsl ---> {sbfiz / ubfiz}
 * sxtw  x1, w0
 * lsl   x2, x1, #3    ===>   sbfiz x2, x0, #3, #32
 *
 * uxtw  x1, w0
 * lsl   x2, x1, #3    ===>   ubfiz x2, x0, #3, #32
 */
class ExtLslToBitFieldInsertPattern : public CGPeepPattern {
public:
    ExtLslToBitFieldInsertPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~ExtLslToBitFieldInsertPattern() override = default;
    std::string GetPatternName() override
    {
        return "ExtLslToBitFieldInsertPattern";
    }
    bool CheckCondition(Insn &insn) override;
    void Run(BB &bb, Insn &insn) override;

private:
    Insn *prevInsn = nullptr;
};

/*
 * Optimize the following patterns:
 * Example 1)
 *  and  w0, w6, #1  ====> tbz  w6, #0, .label
 *  cmp  w0, #1
 *  bne  .label
 *
 *  and  w0, w6, #16  ====> tbz  w6, #4, .label
 *  cmp  w0, #16
 *  bne  .label
 *
 *  and  w0, w6, #32  ====> tbnz  w6, #5, .label
 *  cmp  w0, #32
 *  beq  .label
 *
 * Conditions:
 * 1. cmp_imm value == and_imm value
 * 2. (and_imm value is (1 << n)) && (cmp_imm value is (1 << n))
 *
 * Example 2)
 *  and  x0, x6, #32  ====> tbz  x6, #5, .label
 *  cmp  x0, #0
 *  beq  .label
 *
 *  and  x0, x6, #32  ====> tbnz  x6, #5, .label
 *  cmp  x0, #0
 *  bne  .labelSimplifyMulArithmeticPattern
 *
 * Conditions:
 * 1. (cmp_imm value is 0) || (cmp_imm == and_imm)
 * 2. and_imm value is (1 << n)
 */
class AndCmpBranchesToTbzPattern : public CGPeepPattern {
public:
    AndCmpBranchesToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~AndCmpBranchesToTbzPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "AndCmpBranchesToTbzPattern";
    }

private:
    bool CheckAndSelectPattern(const Insn &currInsn);
    Insn *prevAndInsn = nullptr;
    Insn *prevCmpInsn = nullptr;
    MOperator newMop = MOP_undef;
    int64 tbzImmVal = -1;
};

/*
 * optimize the following patterns:
 * Example 1)
 * cmp w1, wzr
 * bge .label        ====> tbz w1, #31, .label
 *
 * cmp wzr, w1
 * ble .label        ====> tbz w1, #31, .label
 *
 * cmp w1,wzr
 * blt .label        ====> tbnz w1, #31, .label
 *
 * cmp wzr, w1
 * bgt .label        ====> tbnz w1, #31, .label
 *
 *
 * Example 2)
 * cmp w1, #0
 * bge .label        ====> tbz w1, #31, .label
 *
 * cmp w1, #0
 * blt .label        ====> tbnz w1, #31, .label
 */
class ZeroCmpBranchesToTbzPattern : public CGPeepPattern {
public:
    ZeroCmpBranchesToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~ZeroCmpBranchesToTbzPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ZeroCmpBranchesToTbzPattern";
    }

private:
    bool CheckAndSelectPattern(const Insn &currInsn);
    Insn *prevInsn = nullptr;
    MOperator newMop = MOP_undef;
    RegOperand *regOpnd = nullptr;
};

/*
 * mvn  w3, w3          ====> bic  w3, w5, w3
 * and  w3, w5, w3
 * ====>
 * mvn  x3, x3          ====> bic  x3, x5, x3
 * and  x3, x5, x3
 */
class MvnAndToBicPattern : public CGPeepPattern {
public:
    MvnAndToBicPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~MvnAndToBicPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "MvnAndToBicPattern";
    }

private:
    Insn *prevInsn1 = nullptr;
    Insn *prevInsn2 = nullptr;
    bool op1IsMvnDef = false;
    bool op2IsMvnDef = false;
};

/*
 * and r0, r1, #4                  (the imm is n power of 2)
 * ...
 * cbz r0, .Label
 * ===>  tbz r1, #2, .Label
 *
 * and r0, r1, #4                  (the imm is n power of 2)
 * ...
 * cbnz r0, .Label
 * ===>  tbnz r1, #2, .Label
 */
class AndCbzToTbzPattern : public CGPeepPattern {
public:
    AndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    AndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~AndCbzToTbzPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "AndCbzToTbzPattern";
    }

private:
    Insn *prevInsn = nullptr;
};

class CombineSameArithmeticPattern : public CGPeepPattern {
public:
    CombineSameArithmeticPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~CombineSameArithmeticPattern() override
    {
        prevInsn = nullptr;
        newImmOpnd = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CombineSameArithmeticPattern";
    }

private:
    std::vector<MOperator> validMops = {MOP_wlsrrri5, MOP_xlsrrri6,  MOP_wasrrri5,  MOP_xasrrri6,  MOP_wlslrri5,
                                        MOP_xlslrri6, MOP_waddrri12, MOP_xaddrri12, MOP_wsubrri12, MOP_xsubrri12};
    Insn *prevInsn = nullptr;
    ImmOperand *newImmOpnd = nullptr;
};

/*
 * Specific Extension Elimination, includes sxt[b|h|w] & uxt[b|h|w]. There are  scenes:
 * 1. PrevInsn is mov
 * Example 1)
 *  mov    w0, #imm                    or    mov    w0, #imm
 *  sxt{}  w0, w0                            uxt{}  w0, w0
 *  ===> mov w0, #imm                        ===> mov w0, #imm
 *       mov w0, w0                               mov w0, w0
 *
 * Example 2)
 *  mov    w0, R0
 *  uxt{}  w0, w0
 *  ===> mov w0, R0
 *       mov w0, w0
 *
 * Conditions:
 * 1) #imm is not out of range depend on extention valid bits.
 * 2) [mov w0, R0] is return value of call and return size is not of range
 * 3) mov.destOpnd.size = ext.destOpnd.size
 *
 *
 * 2. PrevInsn is ldr[b|h|sb|sh]
 * Example 1)
 *  ldrb x1, []
 *  and  x1, x1, #imm
 *  ===> ldrb x1, []
 *       mov  x1, x1
 *
 * Example 2)
 *  ldrb x1, []           or   ldrb x1, []          or   ldrsb x1, []          or   ldrsb x1, []          or
 *  sxtb x1, x1                uxtb x1, x1               sxtb  x1, x1               uxtb  x1, x1
 *  ===> ldrsb x1, []          ===> ldrb x1, []          ===> ldrsb x1, []          ===> ldrb x1, []
 *       mov   x1, x1               mov  x1, x1               mov   x1, x1               mov  x1, x1
 *
 *  ldrh x1, []           or   ldrh x1, []          or   ldrsh x1, []          or   ldrsh x1, []          or
 *  sxth x1, x1                uxth x1, x1               sxth  x1, x1               uxth  x1, x1
 *  ===> ldrsh x1, []          ===> ldrh x1, []          ===> ldrsh x1, []          ===> ldrb x1, []
 *       mov   x1, x1               mov  x1, x1               mov   x1, x1               mov  x1, x1
 *
 *  ldrsw x1, []          or   ldrsw x1, []
 *  sxtw  x1, x1               uxtw x1, x1
 *  ===> ldrsw x1, []          ===> no change
 *       mov   x1, x1
 *
 * Example 3)
 *  ldrb x1, []           or   ldrb x1, []          or   ldrsb x1, []          or   ldrsb x1, []          or
 *  sxth x1, x1                uxth x1, x1               sxth  x1, x1               uxth  x1, x1
 *  ===> ldrb x1, []           ===> ldrb x1, []          ===> ldrsb x1, []          ===> no change
 *       mov  x1, x1                mov  x1, x1               mov   x1, x1
 *
 *  ldrb x1, []           or   ldrh x1, []          or   ldrsb x1, []          or   ldrsh x1, []          or
 *  sxtw x1, x1                sxtw x1, x1               sxtw  x1, x1               sxtw  x1, x1
 *  ===> ldrb x1, []           ===> ldrh x1, []          ===> ldrsb x1, []          ===> ldrsh x1, []
 *       mov  x1, x1                mov  x1, x1               mov   x1, x1               mov   x1, x1
 *
 *  ldr  x1, []
 *  sxtw x1, x1
 *  ===> ldrsw x1, []
 *       mov   x1, x1
 *
 * Cases:
 * 1) extension size == load size -> change the load type or eliminate the extension
 * 2) extension size >  load size -> possibly eliminating the extension
 *
 *
 * 3. PrevInsn is same sxt / uxt
 * Example 1)
 *  sxth x1, x2
 *  sxth x3, x1
 *  ===> sxth x1, x2
 *       mov  x3, x1
 *
 * Example 2)
 *  sxtb x1, x2          or    uxtb  w0, w0
 *  sxth x3, x1                uxth  w0, w0
 *  ===> sxtb x1, x2           ===> uxtb  w0, w0
 *       mov  x3, x1                mov   x0, x0
 *
 * Conditions:
 * 1) ext1.destOpnd.size == ext2.destOpnd.size
 * 2) ext1.destOpnd.regNo == ext2.destOpnd.regNo
 *    === prop ext1.destOpnd to ext2.srcOpnd, transfer ext2 to mov
 *
 * Cases:
 * 1) ext1 type == ext2 type ((sxth32 & sxth32) || (sxth64 & sxth64) || ...)
 * 2) ext1 type  < ext2 type ((sxtb32 & sxth32) || (sxtb64 & sxth64) || (sxtb64 & sxtw64) ||
 *                            (sxth64 & sxtw64) || (uxtb32 & uxth32))
 */
class ElimSpecificExtensionPattern : public CGPeepPattern {
public:
    ElimSpecificExtensionPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~ElimSpecificExtensionPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ElimSpecificExtensionPattern";
    }

protected:
    enum SpecificExtType : uint8 { EXTUNDEF = 0, SXTB, SXTH, SXTW, UXTB, UXTH, UXTW, SpecificExtTypeSize };
    enum OptSceneType : uint8 { kSceneUndef = 0, kSceneMov, kSceneLoad, kSceneSameExt };
    static constexpr uint8 kPrevLoadPatternNum = 6;
    static constexpr uint8 kPrevLoadMappingNum = 2;
    static constexpr uint8 kValueTypeNum = 2;
    static constexpr uint64 kInvalidValue = 0;
    static constexpr uint8 kSameExtPatternNum = 4;
    static constexpr uint8 kSameExtMappingNum = 2;
    uint64 extValueRangeTable[SpecificExtTypeSize][kValueTypeNum] = {
        /* {minValue, maxValue} */
        {kInvalidValue, kInvalidValue},      /* UNDEF */
        {0xFFFFFFFFFFFFFF80, 0x7F},          /* SXTB */
        {0xFFFFFFFFFFFF8000, 0x7FFF},        /* SXTH */
        {0xFFFFFFFF80000000, kInvalidValue}, /* SXTW */
        {0xFFFFFFFFFFFFFF00, kInvalidValue}, /* UXTB */
        {0xFFFFFFFFFFFF0000, kInvalidValue}, /* UXTH */
        {kInvalidValue, kInvalidValue}       /* UXTW */
    };
    MOperator loadMappingTable[SpecificExtTypeSize][kPrevLoadPatternNum][kPrevLoadMappingNum] = {
        /* {prevOrigMop, prevNewMop} */
        {{MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}}, /* UNDEF */
        {{MOP_wldrb, MOP_wldrsb},
         {MOP_wldrsb, MOP_wldrsb},
         {MOP_wldr, MOP_wldrsb},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}}, /* SXTB */
        {{MOP_wldrh, MOP_wldrsh},
         {MOP_wldrb, MOP_wldrb},
         {MOP_wldrsb, MOP_wldrsb},
         {MOP_wldrsh, MOP_wldrsh},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}}, /* SXTH */
        {{MOP_wldrh, MOP_wldrh},
         {MOP_wldrsh, MOP_wldrsh},
         {MOP_wldrb, MOP_wldrb},
         {MOP_wldrsb, MOP_wldrsb},
         {MOP_wldr, MOP_xldrsw},
         {MOP_xldrsw, MOP_xldrsw}}, /* SXTW */
        {{MOP_wldrb, MOP_wldrb},
         {MOP_wldrsb, MOP_wldrb},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}}, /* UXTB */
        {{MOP_wldrh, MOP_wldrh},
         {MOP_wldrb, MOP_wldrb},
         {MOP_wldr, MOP_wldrh},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}}, /* UXTH */
        {{MOP_wldr, MOP_wldr},
         {MOP_wldrh, MOP_wldrh},
         {MOP_wldrb, MOP_wldrb},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}} /* UXTW */
    };
    MOperator sameExtMappingTable[SpecificExtTypeSize][kSameExtPatternNum][kSameExtMappingNum] = {
        /* {prevMop, currMop} */
        {{MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}}, /* UNDEF */
        {{MOP_xsxtb32, MOP_xsxtb32},
         {MOP_xsxtb64, MOP_xsxtb64},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}}, /* SXTB */
        {{MOP_xsxtb32, MOP_xsxth32},
         {MOP_xsxtb64, MOP_xsxth64},
         {MOP_xsxth32, MOP_xsxth32},
         {MOP_xsxth64, MOP_xsxth64}}, /* SXTH */
        {{MOP_xsxtb64, MOP_xsxtw64},
         {MOP_xsxth64, MOP_xsxtw64},
         {MOP_xsxtw64, MOP_xsxtw64},
         {MOP_undef, MOP_undef}},                                                                             /* SXTW */
        {{MOP_xuxtb32, MOP_xuxtb32}, {MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}}, /* UXTB */
        {{MOP_xuxtb32, MOP_xuxth32},
         {MOP_xuxth32, MOP_xuxth32},
         {MOP_undef, MOP_undef},
         {MOP_undef, MOP_undef}},                                                                            /* UXTH */
        {{MOP_xuxtw64, MOP_xuxtw64}, {MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}, {MOP_undef, MOP_undef}} /* UXTW */
    };

private:
    void SetSpecificExtType(const Insn &currInsn);
    void SetOptSceneType();
    bool IsValidLoadExtPattern(Insn &currInsn, MOperator oldMop, MOperator newMop) const;
    MOperator SelectNewLoadMopByBitSize(MOperator lowBitMop) const;
    void ElimExtensionAfterLoad(Insn &currInsn);
    void ElimExtensionAfterMov(Insn &currInsn);
    void ElimExtensionAfterSameExt(Insn &currInsn);
    void ReplaceExtWithMov(Insn &currInsn);
    Insn *prevInsn = nullptr;
    SpecificExtType extTypeIdx = EXTUNDEF;
    OptSceneType sceneType = kSceneUndef;
    bool is64Bits = false;
};

/*
 * We optimize the following pattern in this function:
 * if w0's valid bits is one
 * uxtb w0, w0
 * eor w0, w0, #1
 * cbz w0, .label
 * =>
 * tbnz w0, .label
 * if there exists uxtb w0, w0 and w0's valid bits is
 * less than 8, eliminate it.
 */
class OneHoleBranchPattern : public CGPeepPattern {
public:
    explicit OneHoleBranchPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~OneHoleBranchPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "OneHoleBranchPattern";
    }

private:
    void FindNewMop(const BB &bb, const Insn &insn);
    bool CheckPrePrevInsn();
    Insn *prevInsn = nullptr;
    Insn *prePrevInsn = nullptr;
    MOperator newOp = MOP_undef;
};

/*
 * Combine logical shift and orr to [extr wd, wn, wm, #lsb  /  extr xd, xn, xm, #lsb]
 * Example 1)
 *  lsr w5, w6, #16
 *  lsl w4, w7, #16
 *  orr w5, w5, w4                  --->        (currInsn)
 *  ===> extr w5, w6, w7, #16
 *
 * Example 2)
 *  lsr w5, w6, #16
 *  orr w5, w5, w4, LSL #16         --->        (currInsn)
 *  ===> extr w5, w6, w4, #16
 *
 * Example 3)
 *  lsl w4, w7, #16
 *  orr w5, w4, w5, LSR #16         --->        (currInsn)
 *  ===> extr w5, w5, w7, #16
 *
 * Conditions:
 *  1. (def[wn] is lsl) & (def[wm] is lsr)
 *  2. lsl_imm + lsr_imm == curr type size (32 or 64)
 *  3. is64bits ? (extr_imm in range [0, 63]) : (extr_imm in range [0, 31])
 *  4. extr_imm = lsr_imm
 */
class LogicShiftAndOrrToExtrPattern : public CGPeepPattern {
public:
    LogicShiftAndOrrToExtrPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~LogicShiftAndOrrToExtrPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "LogicShiftAndOrrToExtrPattern";
    }

private:
    Insn *prevLsrInsn = nullptr;
    Insn *prevLslInsn = nullptr;
    int64 shiftValue = 0;
    bool is64Bits = false;
};

/*
 * Simplify Mul and Basic Arithmetic. There are three scenes:
 * 1. currInsn is add:
 * Example 1)
 *  mul   x1, x1, x2           or     mul   x0, x1, x2
 *  add   x0, x0, x1                  add   x0, x0, x1
 *  ===> madd x0, x1, x2, x0          ===> madd x0, x1, x2, x1
 *
 * Example 2)
 *  fmul  d1, d1, d2           or     fmul  d0, d1, d2
 *  fadd  d0, d0, d1                  fadd  d0, d0, d1
 *  ===> fmadd d0, d1, d2, d0         ===> fmadd d0, d1, d2, d1
 *
 * cases: addInsn second opnd || addInsn third opnd
 *
 *
 * 2. currInsn is sub:
 * Example 1)                         Example 2)
 *  mul   x1, x1, x2                   fmul  d1, d1, d2
 *  sub   x0, x0, x1                   fsub  d0, d0, d1
 *  ===> msub x0, x1, x2, x0           ===> fmsub d0, d1, d2, d0
 *
 * cases: subInsn third opnd
 *
 * 3. currInsn is neg:
 * Example 1)                         Example 2)
 *  mul   x1, x1, x2                   fmul     d1, d1, d2
 *  neg   x0, x1                       fneg     d0, d1
 *  ===> mneg x0, x1, x2               ===> fnmul d0, d1, d2
 *
 * cases: negInsn second opnd
 */
class SimplifyMulArithmeticPattern : public CGPeepPattern {
public:
    SimplifyMulArithmeticPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~SimplifyMulArithmeticPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "SimplifyMulArithmeticPattern";
    }

protected:
    enum ArithmeticType : uint8 { kUndef = 0, kAdd, kFAdd, kSub, kFSub, kNeg, kFNeg, kArithmeticTypeSize };
    static constexpr uint8 newMopNum = 2;
    MOperator curMop2NewMopTable[kArithmeticTypeSize][newMopNum] = {
        /* {32bit_mop, 64bit_mop} */
        {MOP_undef, MOP_undef},         /* kUndef  */
        {MOP_wmaddrrrr, MOP_xmaddrrrr}, /* kAdd    */
        {MOP_smadd, MOP_dmadd},         /* kFAdd   */
        {MOP_wmsubrrrr, MOP_xmsubrrrr}, /* kSub    */
        {MOP_smsub, MOP_dmsub},         /* kFSub   */
        {MOP_wmnegrrr, MOP_xmnegrrr},   /* kNeg    */
        {MOP_snmul, MOP_dnmul}          /* kFNeg   */
    };

private:
    void SetArithType(const Insn &currInsn);
    void DoOptimize(BB &currBB, Insn &currInsn);
    ArithmeticType arithType = kUndef;
    int32 validOpndIdx = -1;
    Insn *prevInsn = nullptr;
    bool isFloat = false;
};

/*
 * Example 1)
 *  lsr w0, w1, #6
 *  and w0, w0, #1                 --->        (currInsn)
 *  ===> ubfx w0, w1, #6, #1
 *
 * Conditions:
 * 1. and_imm value is (1 << n -1)
 * 2. is64bits ? (ubfx_imm_lsb in range [0, 63]) : (ubfx_imm_lsb in range [0, 31])
 * 3. is64bits ? ((ubfx_imm_lsb + ubfx_imm_width) in range [1, 32]) : ((ubfx_imm_lsb + ubfx_imm_width) in range [1, 64])
 */
class LsrAndToUbfxPattern : public CGPeepPattern {
public:
    LsrAndToUbfxPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~LsrAndToUbfxPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "LsrAndToUbfxPattern";
    }

private:
    Insn *prevInsn = nullptr;
};

/*
 * Optimize the following patterns:
 *  orr  w21, w0, #0  ====> mov  w21, w0
 *  orr  w21, #0, w0  ====> mov  w21, w0
 */
class OrrToMovPattern : public CGPeepPattern {
public:
    explicit OrrToMovPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~OrrToMovPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "OrrToMovPattern";
    }

private:
    MOperator newMop = MOP_undef;
    RegOperand *reg2 = nullptr;
};

/*
 * Optimize the following patterns:
 * ubfx  x201, x202, #0, #32
 * ====>
 * uxtw x201, w202
 */
class UbfxToUxtwPattern : public CGPeepPattern {
public:
    UbfxToUxtwPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~UbfxToUxtwPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "UbfxToUxtwPattern";
    }
};

/*
 * Optimize the following patterns:
 * ubfx  w0, w0, #2, #1
 * cbz   w0, .L.3434__292    ====>    tbz w0, #2, .L.3434__292
 * -------------------------------
 * ubfx  w0, w0, #2, #1
 * cnbz   w0, .L.3434__292    ====>    tbnz w0, #2, .L.3434__292
 * -------------------------------
 * ubfx  x0, x0, #2, #1
 * cbz   x0, .L.3434__292    ====>    tbz x0, #2, .L.3434__292
 * -------------------------------
 * ubfx  x0, x0, #2, #1
 * cnbz  x0, .L.3434__292    ====>    tbnz x0, #2, .L.3434__292
 */
class UbfxAndCbzToTbzPattern : public CGPeepPattern {
public:
    UbfxAndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
        : CGPeepPattern(cgFunc, currBB, currInsn, info)
    {
    }
    ~UbfxAndCbzToTbzPattern() override
    {
        useInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "UbfxAndCbzToTbzPattern";
    }

private:
    Insn *useInsn = nullptr;
    MOperator newMop = MOP_undef;
};

/*
 * Looking for identical mem insn to eliminate.
 * If two back-to-back is:
 * 1. str + str
 * 2. str + ldr
 * And the [MEM] is pattern of [base + offset]
 * 1. The [MEM] operand is exactly same then first
 *    str can be eliminate.
 * 2. The [MEM] operand is exactly same and src opnd
 *    of str is same as the dest opnd of ldr then
 *    ldr can be eliminate
 */
class RemoveIdenticalLoadAndStorePattern : public CGPeepPattern {
public:
    RemoveIdenticalLoadAndStorePattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
        : CGPeepPattern(cgFunc, currBB, currInsn)
    {
    }
    ~RemoveIdenticalLoadAndStorePattern() override
    {
        nextInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "RemoveIdenticalLoadAndStorePattern";
    }

private:
    bool IsMemOperandsIdentical(const Insn &insn1, const Insn &insn2) const;
    Insn *nextInsn = nullptr;
};

/* ======== CGPeepPattern End ======== */
/*
 * Looking for identical mem insn to eliminate.
 * If two back-to-back is:
 * 1. str + str
 * 2. str + ldr
 * And the [MEM] is pattern of [base + offset]
 * 1. The [MEM] operand is exactly same then first
 *    str can be eliminate.
 * 2. The [MEM] operand is exactly same and src opnd
 *    of str is same as the dest opnd of ldr then
 *    ldr can be eliminate
 */
class RemoveIdenticalLoadAndStoreAArch64 : public PeepPattern {
public:
    explicit RemoveIdenticalLoadAndStoreAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~RemoveIdenticalLoadAndStoreAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    bool IsMemOperandsIdentical(const Insn &insn1, const Insn &insn2) const;
};

/* Remove redundant mov which src and dest opnd is exactly same */
class RemoveMovingtoSameRegPattern : public CGPeepPattern {
public:
    RemoveMovingtoSameRegPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn)
    {
    }
    ~RemoveMovingtoSameRegPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "RemoveMovingtoSameRegPattern";
    }
};

/* Remove redundant mov which src and dest opnd is exactly same */
class RemoveMovingtoSameRegAArch64 : public PeepPattern {
public:
    explicit RemoveMovingtoSameRegAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~RemoveMovingtoSameRegAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Combining 2 STRs into 1 stp or 2 LDRs into 1 ldp, when they are
 * back to back and the [MEM] they access is conjointed.
 */
class CombineContiLoadAndStorePattern : public CGPeepPattern {
public:
    CombineContiLoadAndStorePattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
        : CGPeepPattern(cgFunc, currBB, currInsn)
    {
        doAggressiveCombine = cgFunc.GetMirModule().IsCModule();
    }
    ~CombineContiLoadAndStorePattern() override
    {
        memOpnd = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CombineContiLoadAndStorePattern";
    }

private:
    std::vector<Insn *> FindPrevStrLdr(Insn &insn, regno_t destRegNO, regno_t memBaseRegNO, int64 baseOfst);
    /*
     * avoid the following situation:
     * str x2, [x19, #8]
     * mov x0, x19
     * bl foo (change memory)
     * str x21, [x19, #16]
     */
    bool IsRegNotSameMemUseInInsn(const Insn &insn, regno_t regNO, bool isStore, int64 baseOfst) const;
    void RemoveInsnAndKeepComment(BB &bb, Insn &insn, Insn &prevInsn) const;
    MOperator GetMopHigherByte(MOperator mop) const;
    bool SplitOfstWithAddToCombine(const Insn &curInsn, Insn &combineInsn, const MemOperand &memOperand) const;
    Insn *FindValidSplitAddInsn(Insn &curInsn, RegOperand &baseOpnd) const;
    bool PlaceSplitAddInsn(const Insn &curInsn, Insn &combineInsn, const MemOperand &memOpnd, RegOperand &baseOpnd,
                           uint32 bitLen) const;
    bool doAggressiveCombine = false;
    MemOperand *memOpnd = nullptr;
};

/*
 * mov  x0, x1
 * ldr	x0, [x0]        --> ldr x0, [x1]
 *
 * add	x0, x0, #64
 * ldr	x0, [x0]        --> ldr x0, [x0, #64]
 * 
 * add	x0, x7, x0
 * ldr	x11, [x0]       --> ldr x11, [x0, x7]
 * 
 * lsl	x1, x4, #3
 * add	x0, x0, x1
 * ldr	d1, [x0]        --> ldr d1, [x0, x4, lsl #3]
 */
class EnhanceStrLdrAArch64 : public PeepPattern {
public:
    explicit EnhanceStrLdrAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~EnhanceStrLdrAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    ImmOperand *GetInsnAddOrSubNewOffset(Insn &insn, ImmOperand &offset);
    void OptimizeAddrBOI(Insn &insn, MemOperand &memOpnd, Insn &prevInsn);
    void OptimizeAddrBOrX(Insn &insn, MemOperand &memOpnd, Insn &prevInsn);
    void OptimizeAddrBOrXShiftExtend(Insn &insn, MemOperand &memOpnd, Insn &shiftExtendInsn);
    void OptimizeWithAddrrrs(Insn &insn, MemOperand &memOpnd, Insn &addInsn);
    bool CheckOperandIsDeadFromInsn(const RegOperand &regOpnd, Insn &insn);
};

/* Eliminate the sxt[b|h|w] w0, w0;, when w0 is satisify following:
 * i)  mov w0, #imm (#imm is not out of range)
 * ii) ldrs[b|h] w0, [MEM]
 */
class EliminateSpecifcSXTAArch64 : public PeepPattern {
public:
    explicit EliminateSpecifcSXTAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~EliminateSpecifcSXTAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/* Eliminate the uxt[b|h|w] w0, w0;when w0 is satisify following:
 * i)  mov w0, #imm (#imm is not out of range)
 * ii) mov w0, R0(Is return value of call and return size is not of range)
 * iii)w0 is defined and used by special load insn and uxt[] pattern
 */
class EliminateSpecifcUXTAArch64 : public PeepPattern {
public:
    explicit EliminateSpecifcUXTAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~EliminateSpecifcUXTAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/* fmov ireg1 <- freg1   previous insn
 * fmov ireg2 <- freg1   current insn
 * use  ireg2            may or may not be present
 * =>
 * fmov ireg1 <- freg1   previous insn
 * mov  ireg2 <- ireg1   current insn
 * use  ireg1            may or may not be present
 */
class FmovRegPattern : public CGPeepPattern {
public:
    FmovRegPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~FmovRegPattern() override
    {
        prevInsn = nullptr;
        nextInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "FmovRegPattern";
    }

private:
    Insn *prevInsn = nullptr;
    Insn *nextInsn = nullptr;
};

/* sbfx ireg1, ireg2, 0, 32
 * use  ireg1.32
 * =>
 * sbfx ireg1, ireg2, 0, 32
 * use  ireg2.32
 */
class SbfxOptPattern : public CGPeepPattern {
public:
    SbfxOptPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~SbfxOptPattern() override
    {
        nextInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "SbfxOptPattern";
    }

private:
    Insn *nextInsn = nullptr;
    bool toRemove = false;
    std::vector<uint32> cands;
};

/* cbnz x0, labelA
 * mov x0, 0
 * b  return-bb
 * labelA:
 * =>
 * cbz x0, return-bb
 * labelA:
 */
class CbnzToCbzPattern : public CGPeepPattern {
public:
    CbnzToCbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~CbnzToCbzPattern() override
    {
        nextBB = nullptr;
        movInsn = nullptr;
        brInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CbnzToCbzPattern";
    }

private:
    BB *nextBB = nullptr;
    Insn *movInsn = nullptr;
    Insn *brInsn = nullptr;
};

/* i.   cset    w0, EQ
 *      cbnz    w0, .label    ===> beq .label
 *
 * ii.  cset    w0, EQ
 *      cbz    w0, .label     ===> bne .label
 *
 * iii. cset    w0, NE
 *      cbnz    w0, .label    ===> bne .label
 *
 * iiii.cset    w0, NE
 *      cbz    w0, .label     ===> beq .label
 * ... ...
 */
class CsetCbzToBeqOptAArch64 : public PeepPattern {
public:
    explicit CsetCbzToBeqOptAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~CsetCbzToBeqOptAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
    MOperator SelectMOperator(ConditionCode condCode, bool inverse) const;
};

/* When exist load after load or load after store, and [MEM] is
 * totally same. Then optimize them.
 */
class ContiLDRorSTRToSameMEMPattern : public CGPeepPattern {
public:
    ContiLDRorSTRToSameMEMPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn)
    {
    }
    ~ContiLDRorSTRToSameMEMPattern() override
    {
        prevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ContiLDRorSTRToSameMEMPattern";
    }

private:
    Insn *prevInsn = nullptr;
    bool loadAfterStore = false;
    bool loadAfterLoad = false;
};

/*
 *  Remove following patterns:
 *  mov     x1, x0
 *  bl      MCC_IncDecRef_NaiveRCFast
 */
class RemoveIncDecRefPattern : public CGPeepPattern {
public:
    RemoveIncDecRefPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~RemoveIncDecRefPattern() override
    {
        prevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "RemoveIncDecRefPattern";
    }

private:
    Insn *prevInsn = nullptr;
};

/*
 * When GCONLY is enabled, the read barriers can be inlined.
 * we optimize it with the following pattern:
 * #if USE_32BIT_REF
 *   bl MCC_LoadRefField             ->  ldr  w0, [x1]
 *   bl MCC_LoadVolatileField        ->  ldar w0, [x1]
 *   bl MCC_LoadRefStatic            ->  ldr  w0, [x0]
 *   bl MCC_LoadVolatileStaticField  ->  ldar w0, [x0]
 *   bl MCC_Dummy                    ->  omitted
 * #else
 *   bl MCC_LoadRefField             ->  ldr  x0, [x1]
 *   bl MCC_LoadVolatileField        ->  ldar x0, [x1]
 *   bl MCC_LoadRefStatic            ->  ldr  x0, [x0]
 *   bl MCC_LoadVolatileStaticField  ->  ldar x0, [x0]
 *   bl MCC_Dummy                    ->  omitted
 * #endif
 *
 * if we encounter a tail call optimized read barrier call,
 * such as:
 *   b MCC_LoadRefField
 * a return instruction will be added just after the load:
 *   ldr w0, [x1]
 *   ret
 */
class InlineReadBarriersPattern : public CGPeepPattern {
public:
    InlineReadBarriersPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~InlineReadBarriersPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "InlineReadBarriersPattern";
    }
};

/*
 *    mov     w1, #34464
 *    movk    w1, #1,  LSL #16
 *    sdiv    w2, w0, w1
 *  ========>
 *    mov     w1, #34464         // may deleted if w1 not live anymore.
 *    movk    w1, #1,  LSL #16   // may deleted if w1 not live anymore.
 *    mov     w16, #0x588f
 *    movk    w16, #0x4f8b, LSL #16
 *    smull   x16, w0, w16
 *    asr     x16, x16, #32
 *    add     x16, x16, w0, SXTW
 *    asr     x16, x16, #17
 *    add     x2, x16, x0, LSR #31
 */
class ReplaceDivToMultiPattern : public CGPeepPattern {
public:
    ReplaceDivToMultiPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~ReplaceDivToMultiPattern() override
    {
        prevInsn = nullptr;
        prePrevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ReplaceDivToMultiPattern";
    }

private:
    Insn *prevInsn = nullptr;
    Insn *prePrevInsn = nullptr;
};

/*
 * Optimize the following patterns:
 *  and  w0, w0, #imm  ====> tst  w0, #imm
 *  cmp  w0, #0              beq/bne  .label
 *  beq/bne  .label
 *
 *  and  x0, x0, #imm  ====> tst  x0, #imm
 *  cmp  x0, #0              beq/bne  .label
 *  beq/bne  .label
 */
class AndCmpBranchesToTstAArch64 : public PeepPattern {
public:
    explicit AndCmpBranchesToTstAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~AndCmpBranchesToTstAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Optimize the following patterns:
 *  and  w0, w0, #imm  ====> tst  w0, #imm
 *  cbz/cbnz  .label         beq/bne  .label
 */
class AndCbzBranchesToTstAArch64 : public PeepPattern {
public:
    explicit AndCbzBranchesToTstAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~AndCbzBranchesToTstAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Optimize the following patterns:
 *  and  w0, w0, #1  ====> and  w0, w0, #1
 *  cmp  w0, #1
 *  cset w0, EQ
 *
 *  and  w0, w0, #1  ====> and  w0, w0, #1
 *  cmp  w0, #0
 *  cset w0, NE
 *  ---------------------------------------------------
 *  and  w0, w0, #imm  ====> ubfx  w0, w0, pos, size
 *  cmp  w0, #imm
 *  cset w0, EQ
 *
 *  and  w0, w0, #imm  ====> ubfx  w0, w0, pos, size
 *  cmp  w0, #0
 *  cset w0, NE
 *  conditions:
 *  imm is pos power of 2
 *
 *  ---------------------------------------------------
 *  and  w0, w0, #1  ====> and  wn, w0, #1
 *  cmp  w0, #1
 *  cset wn, EQ        # wn != w0 && w0 is not live after cset
 *
 *  and  w0, w0, #1  ====> and  wn, w0, #1
 *  cmp  w0, #0
 *  cset wn, NE        # wn != w0 && w0 is not live after cset
 *  ---------------------------------------------------
 *  and  w0, w0, #imm  ====> ubfx  wn, w0, pos, size
 *  cmp  w0, #imm
 *  cset wn, EQ        # wn != w0 && w0 is not live after cset
 *
 *  and  w0, w0, #imm  ====> ubfx  wn, w0, pos, size
 *  cmp  w0, #0
 *  cset wn, NE        # wn != w0 && w0 is not live after cset
 *  conditions:
 *  imm is pos power of 2 and w0 is not live after cset
 */
class AndCmpBranchesToCsetAArch64 : public PeepPattern {
public:
    explicit AndCmpBranchesToCsetAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~AndCmpBranchesToCsetAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    Insn *FindPreviousCmp(Insn &insn) const;
};
/*
 * We optimize the following pattern in this function:
 * cmp w[0-9]*, wzr  ====> tbz w[0-9]*, #31, .label
 * bge .label
 *
 * cmp wzr, w[0-9]*  ====> tbz w[0-9]*, #31, .label
 * ble .label
 *
 * cmp w[0-9]*,wzr   ====> tbnz w[0-9]*, #31, .label
 * blt .label
 *
 * cmp wzr, w[0-9]*  ====> tbnz w[0-9]*, #31, .label
 * bgt .label
 *
 * cmp w[0-9]*, #0   ====> tbz w[0-9]*, #31, .label
 * bge .label
 *
 * cmp w[0-9]*, #0   ====> tbnz w[0-9]*, #31, .label
 * blt .label
 */
class ZeroCmpBranchesAArch64 : public PeepPattern {
public:
    explicit ZeroCmpBranchesAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ZeroCmpBranchesAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Look for duplicate or overlapping zero or sign extensions.
 * Examples:
 *   sxth x1, x2   ====> sxth x1, x2
 *   sxth x3, x1         mov  x3, x1
 *
 *   sxtb x1, x2   ====> sxtb x1, x2
 *   sxth x3, x1         mov  x3, x1
 */
class ElimDuplicateExtensionAArch64 : public PeepPattern {
public:
    explicit ElimDuplicateExtensionAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ElimDuplicateExtensionAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
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
class CmpCsetAArch64 : public PeepPattern {
public:
    explicit CmpCsetAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~CmpCsetAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    bool CheckOpndDefPoints(Insn &checkInsn, int opndIdx);
    const Insn *DefInsnOfOperandInBB(const Insn &startInsn, const Insn &checkInsn, int opndIdx) const;
    bool OpndDefByOneValidBit(const Insn &defInsn) const;
    bool FlagUsedLaterInCurBB(const BB &bb, Insn &startInsn) const;
};

/*
 *  add     x0, x1, x0
 *  ldr     x2, [x0]
 *  ==>
 *  ldr     x2, [x1, x0]
 */
class ComplexMemOperandAddAArch64 : public PeepPattern {
public:
    explicit ComplexMemOperandAddAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ComplexMemOperandAddAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    bool IsExpandBaseOpnd(const Insn &insn, const Insn &prevInsn) const;
};

/*
 * cbnz w0, @label
 * ....
 * mov  w0, #0 (elseBB)        -->this instruction can be deleted
 *
 * cbz  w0, @label
 * ....
 * mov  w0, #0 (ifBB)          -->this instruction can be deleted
 *
 * condition:
 *  1.there is not predefine points of w0 in elseBB(ifBB)
 *  2.the first opearnd of cbnz insn is same as the first Operand of mov insn
 *  3.w0 is defined by move 0
 *  4.all preds of elseBB(ifBB) end with cbnz or cbz
 *
 *  NOTE: if there are multiple preds and there is not define point of w0 in one pred,
 *        (mov w0, 0) can't be deleted, avoiding use before def.
 */
class DeleteMovAfterCbzOrCbnzAArch64 : public PeepPattern {
public:
    explicit DeleteMovAfterCbzOrCbnzAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc)
    {
        cgcfg = cgFunc.GetTheCFG();
        cgcfg->InitInsnVisitor(cgFunc);
    }
    ~DeleteMovAfterCbzOrCbnzAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    bool PredBBCheck(BB &bb, bool checkCbz, const Operand &opnd) const;
    bool OpndDefByMovZero(const Insn &insn) const;
    bool NoPreDefine(Insn &testInsn) const;
    void ProcessBBHandle(BB *processBB, const BB &bb, const Insn &insn) const;
    CGCFG *cgcfg;
};

/*
 * We optimize the following pattern in this function:
 * if w0's valid bits is one
 * uxtb w0, w0
 * eor w0, w0, #1
 * cbz w0, .label
 * =>
 * tbnz w0, .label
 * &&
 * if there exists uxtb w0, w0 and w0's valid bits is
 * less than 8, eliminate it.
 */
class OneHoleBranchesPreAArch64 : public PeepPattern {
public:
    explicit OneHoleBranchesPreAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~OneHoleBranchesPreAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    MOperator FindNewMop(const BB &bb, const Insn &insn) const;
};

/*
 * We optimize the following pattern in this function:
 * movz x0, #11544, LSL #0
 * movk x0, #21572, LSL #16
 * movk x0, #8699, LSL #32
 * movk x0, #16393, LSL #48
 * =>
 * ldr x0, label_of_constant_1
 */
class LoadFloatPointPattern : public CGPeepPattern {
public:
    LoadFloatPointPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~LoadFloatPointPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "LoadFloatPointPattern";
    }

private:
    bool FindLoadFloatPoint(Insn &insn);
    bool IsPatternMatch();
    std::vector<Insn *> optInsn;
};

/*
 * Optimize the following patterns:
 *  orr  w21, w0, #0  ====> mov  w21, w0
 *  orr  w21, #0, w0  ====> mov  w21, w0
 */
class ReplaceOrrToMovAArch64 : public PeepPattern {
public:
    explicit ReplaceOrrToMovAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ReplaceOrrToMovAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Optimize the following patterns:
 *  ldr  w0, [x21,#68]        ldr  w0, [x21,#68]
 *  mov  w1, #-1              mov  w1, #-1
 *  cmp  w0, w1     ====>     cmn  w0, #-1
 */
class ReplaceCmpToCmnAArch64 : public PeepPattern {
public:
    explicit ReplaceCmpToCmnAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ReplaceCmpToCmnAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Remove following patterns:
 *   mov x0, XX
 *   mov x1, XX
 *    bl  MCC_IncDecRef_NaiveRCFast
 */
class RemoveIncRefPattern : public CGPeepPattern {
public:
    RemoveIncRefPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~RemoveIncRefPattern() override
    {
        insnMov2 = nullptr;
        insnMov1 = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "RemoveIncRefPattern";
    }

private:
    Insn *insnMov2 = nullptr;
    Insn *insnMov1 = nullptr;
};

/*
 * opt long int compare with 0
 *  *cmp x0, #0
 *  csinv w0, wzr, wzr, GE
 *  csinc w0, w0, wzr, LE
 *  cmp w0, #0
 *  =>
 *  cmp x0, #0
 */
class LongIntCompareWithZPattern : public CGPeepPattern {
public:
    LongIntCompareWithZPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~LongIntCompareWithZPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "LongIntCompareWithZPattern";
    }

private:
    bool FindLondIntCmpWithZ(Insn &insn);
    bool IsPatternMatch();
    std::vector<Insn *> optInsn;
};

/*
 *  add     x0, x1, #:lo12:Ljava_2Futil_2FLocale_241_3B_7C_24SwitchMap_24java_24util_24Locale_24Category
 *  ldr     x2, [x0]
 *  ==>
 *  ldr     x2, [x1, #:lo12:Ljava_2Futil_2FLocale_241_3B_7C_24SwitchMap_24java_24util_24Locale_24Category]
 */
class ComplexMemOperandAArch64 : public PeepPattern {
public:
    explicit ComplexMemOperandAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ComplexMemOperandAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 *  add     x0, x1, x0
 *  ldr     x2, [x0]
 *  ==>
 *  ldr     x2, [x1, x0]
 */
class ComplexMemOperandPreAddAArch64 : public PeepPattern {
public:
    explicit ComplexMemOperandPreAddAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ComplexMemOperandPreAddAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * add     x0, x0, x1, LSL #2
 * ldr     x2, [x0]
 * ==>
 * ldr     x2, [x0,x1,LSL #2]
 */
class ComplexMemOperandLSLAArch64 : public PeepPattern {
public:
    explicit ComplexMemOperandLSLAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ComplexMemOperandLSLAArch64() override = default;
    bool CheckShiftValid(const Insn &insn, const BitShiftOperand &lsl) const;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * ldr     x0, label_of_constant_1
 * fmov    d4, x0
 * ==>
 * ldr     d4, label_of_constant_1
 */
class ComplexMemOperandLabelAArch64 : public PeepPattern {
public:
    explicit ComplexMemOperandLabelAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ComplexMemOperandLabelAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * mov R0, vreg1 / R0         mov R0, vreg1
 * add vreg2, vreg1, #imm1    add vreg2, vreg1, #imm1
 * mov R1, vreg2              mov R1, vreg2
 * mov R2, vreg3              mov R2, vreg3
 * ...                        ...
 * mov R0, vreg1
 * add vreg4, vreg1, #imm2 -> str vreg5, [vreg1, #imm2]
 * mov R1, vreg4
 * mov R2, vreg5
 */
class WriteFieldCallPattern : public CGPeepPattern {
public:
    struct WriteRefFieldParam {
        Operand *objOpnd = nullptr;
        RegOperand *fieldBaseOpnd = nullptr;
        int64 fieldOffset = 0;
        Operand *fieldValue = nullptr;
    };
    WriteFieldCallPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~WriteFieldCallPattern() override
    {
        prevCallInsn = nullptr;
        nextInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "WriteFieldCallPattern";
    }

private:
    bool hasWriteFieldCall = false;
    Insn *prevCallInsn = nullptr;
    Insn *nextInsn = nullptr;
    WriteRefFieldParam firstCallParam;
    WriteRefFieldParam currentCallParam;
    std::vector<Insn *> paramDefInsns;
    bool WriteFieldCallOptPatternMatch(const Insn &writeFieldCallInsn, WriteRefFieldParam &param);
    bool IsWriteRefFieldCallInsn(const Insn &insn) const;
};

/*
 * Remove following patterns:
 *     mov     x0, xzr/#0
 *     bl      MCC_DecRef_NaiveRCFast
 */
class RemoveDecRefPattern : public CGPeepPattern {
public:
    RemoveDecRefPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~RemoveDecRefPattern() override
    {
        prevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "RemoveDecRefPattern";
    }

private:
    Insn *prevInsn = nullptr;
};

/*
 * We optimize the following pattern in this function:
 * and x1, x1, #imm (is n power of 2)
 * cbz/cbnz x1, .label
 * =>
 * and x1, x1, #imm (is n power of 2)
 * tbnz/tbz x1, #n, .label
 */
class OneHoleBranchesAArch64 : public PeepPattern {
public:
    explicit OneHoleBranchesAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~OneHoleBranchesAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Replace following pattern:
 * mov x1, xzr
 * bl MCC_IncDecRef_NaiveRCFast
 * =>
 * bl MCC_IncRef_NaiveRCFast
 */
class ReplaceIncDecWithIncPattern : public CGPeepPattern {
public:
    ReplaceIncDecWithIncPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~ReplaceIncDecWithIncPattern() override
    {
        prevInsn = nullptr;
        target = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ReplaceIncDecWithIncPattern";
    }

private:
    Insn *prevInsn = nullptr;
    FuncNameOperand *target = nullptr;
};

/*
 * Optimize the following patterns:
 *  and  w0, w6, #1  ====> tbz  w6, 0, .label
 *  cmp  w0, #1
 *  bne  .label
 *
 *  and  w0, w6, #16  ====> tbz  w6, 4, .label
 *  cmp  w0, #16
 *  bne  .label
 *
 *  and  w0, w6, #32  ====> tbnz  w6, 5, .label
 *  cmp  w0, #32
 *  beq  .label
 *
 *  and  x0, x6, #32  ====> tbz  x6, 5, .label
 *  cmp  x0, #0
 *  beq  .label
 *
 *  and  x0, x6, #32  ====> tbnz  x6, 5, .label
 *  cmp  x0, #0
 *  bne  .label
 */
class AndCmpBranchesToTbzAArch64 : public PeepPattern {
public:
    explicit AndCmpBranchesToTbzAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~AndCmpBranchesToTbzAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Optimize the following patterns:
 * sxth  r4, r4         ====> strh r4, [r0, r3]
 * strh  r4, [r0, r3]
 *
 * sxtb  r4, r4         ====> strb r4, [r0, r3]
 * strb  r4, [r0, r3]
 */
class RemoveSxtBeforeStrAArch64 : public PeepPattern {
public:
    explicit RemoveSxtBeforeStrAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~RemoveSxtBeforeStrAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
};

/*
 * Optimize the following patterns:
 * mov x1, #1
 * csel  x22, xzr, x1, LS   ====> cset x22, HI
 *
 * mov x1, #1
 * csel  x22, x1, xzr, LS   ====> cset x22, LS
 */
class CselZeroOneToCsetOpt : public PeepPattern {
public:
    explicit CselZeroOneToCsetOpt(CGFunc &cgFunc) : PeepPattern(cgFunc), cgFunc(&cgFunc) {}
    ~CselZeroOneToCsetOpt() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    Insn *trueMovInsn = nullptr;
    Insn *falseMovInsn = nullptr;
    Insn *FindFixedValue(Operand &opnd, BB &bb, Operand *&tempOp, const Insn &insn) const;

protected:
    CGFunc *cgFunc;
};

/*
 * Optimize the following patterns:
 * and w0, w0, #0x1
 * cmp w0, #0x0
 * cset w0, eq
 * eor w0, w0, #0x1
 * cbz w0, label
 * ====>
 * tbz w0, 0, label
 */
class AndCmpCsetEorCbzOpt : public PeepPattern {
public:
    explicit AndCmpCsetEorCbzOpt(CGFunc &cgFunc) : PeepPattern(cgFunc), cgFunc(&cgFunc) {}
    ~AndCmpCsetEorCbzOpt() override = default;
    void Run(BB &bb, Insn &insn) override;
private:
    CGFunc *cgFunc;
};

/*
 * Optimize the following patterns:
 * add x0, x0, x1
 * ldr w0, [x0]
 * ====>
 * ldr w0, [x0, x1]
 */
class AddLdrOpt : public PeepPattern {
public:
    explicit AddLdrOpt(CGFunc &cgFunc) : PeepPattern(cgFunc), cgFunc(&cgFunc) {}
    ~AddLdrOpt() override = default;
    void Run(BB &bb, Insn &insn) override;
private:
    CGFunc *cgFunc;
};

/*
 * Optimize the following patterns:
 * cset x0, eq
 * eor x0, x0, 0x1
 * ====>
 * cset x0, ne
 */
class CsetEorOpt : public PeepPattern {
public:
    explicit CsetEorOpt(CGFunc &cgFunc) : PeepPattern(cgFunc), cgFunc(&cgFunc) {}
    ~CsetEorOpt() override = default;
    void Run(BB &bb, Insn &insn) override;
private:
    CGFunc *cgFunc;
};

/*
 * Optimize the following patterns:
 * mov x1, #0x5
 * cmp x0, x1
 * ====>
 * cmp x0, #0x5
 */
class MoveCmpOpt : public PeepPattern {
public:
    explicit MoveCmpOpt(CGFunc &cgFunc) : PeepPattern(cgFunc), cgFunc(&cgFunc) {}
    ~MoveCmpOpt() override = default;
    void Run(BB &bb, Insn &insn) override;
private:
    CGFunc *cgFunc;
};

/*
 * Replace following pattern:
 * sxtw  x1, w0
 * lsl   x2, x1, #3  ====>  sbfiz x2, x0, #3, #32
 *
 * uxtw  x1, w0
 * lsl   x2, x1, #3  ====>  ubfiz x2, x0, #3, #32
 */
class ComplexExtendWordLslAArch64 : public PeepPattern {
public:
    explicit ComplexExtendWordLslAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~ComplexExtendWordLslAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    bool IsExtendWordLslPattern(const Insn &insn) const;
};

class AArch64PeepHole : public PeepPatternMatch {
public:
    AArch64PeepHole(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
    ~AArch64PeepHole() override = default;
    void InitOpts() override;
    void Run(BB &bb, Insn &insn) override;

private:
    enum PeepholeOpts : int32 {
        kRemoveIdenticalLoadAndStoreOpt = 0,
        kRemoveMovingtoSameRegOpt,
        kCombineContiLoadAndStoreOpt,
        kEliminateSpecifcSXTOpt,
        kEliminateSpecifcUXTOpt,
        kFmovRegOpt,
        kCbnzToCbzOpt,
        kCsetCbzToBeqOpt,
        kContiLDRorSTRToSameMEMOpt,
        kRemoveIncDecRefOpt,
        kInlineReadBarriersOpt,
        kReplaceDivToMultiOpt,
        kAndCmpBranchesToCsetOpt,
        kAndCmpBranchesToTstOpt,
        kAndCbzBranchesToTstOpt,
        kZeroCmpBranchesOpt,
        kCselZeroOneToCsetOpt,
        kAndCmpCsetEorCbzOpt,
        kAddLdrOpt,
        kCsetEorOpt,
        kMoveCmpOpt,
        kPeepholeOptsNum
    };
};

class AArch64PeepHole0 : public PeepPatternMatch {
public:
    AArch64PeepHole0(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
    ~AArch64PeepHole0() override = default;
    void InitOpts() override;
    void Run(BB &bb, Insn &insn) override;

private:
    enum PeepholeOpts : int32 {
        kRemoveIdenticalLoadAndStoreOpt = 0,
        kCmpCsetOpt,
        kComplexMemOperandOptAdd,
        kDeleteMovAfterCbzOrCbnzOpt,
        kRemoveSxtBeforeStrOpt,
        kRemoveMovingtoSameRegOpt,
        kEnhanceStrLdrAArch64Opt,
        kPeepholeOptsNum
    };
};

class AArch64PrePeepHole : public PeepPatternMatch {
public:
    AArch64PrePeepHole(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
    ~AArch64PrePeepHole() override = default;
    void InitOpts() override;
    void Run(BB &bb, Insn &insn) override;

private:
    enum PeepholeOpts : int32 {
        kOneHoleBranchesPreOpt = 0,
        kLoadFloatPointOpt,
        kReplaceOrrToMovOpt,
        kReplaceCmpToCmnOpt,
        kRemoveIncRefOpt,
        kLongIntCompareWithZOpt,
        kComplexMemOperandOpt,
        kComplexMemOperandPreOptAdd,
        kComplexMemOperandOptLSL,
        kComplexMemOperandOptLabel,
        kWriteFieldCallOpt,
        kDuplicateExtensionOpt,
        kEnhanceStrLdrAArch64Opt,
        kUbfxToUxtw,
        kPeepholeOptsNum
    };
};

class AArch64PrePeepHole1 : public PeepPatternMatch {
public:
    AArch64PrePeepHole1(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
    ~AArch64PrePeepHole1() override = default;
    void InitOpts() override;
    void Run(BB &bb, Insn &insn) override;

private:
    enum PeepholeOpts : int32 {
        kRemoveDecRefOpt = 0,
        kComputationTreeOpt,
        kOneHoleBranchesOpt,
        kReplaceIncDecWithIncOpt,
        kAndCmpBranchesToTbzOpt,
        kComplexExtendWordLslOpt,
        kPeepholeOptsNum
    };
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H */
