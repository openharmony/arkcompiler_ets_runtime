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
#include "aarch64_isa.h"
#include "optimize_common.h"
#include "mir_builder.h"

namespace maplebe {
class AArch64CGPeepHole : public CGPeepHole {
public:
    /* normal constructor */
    AArch64CGPeepHole(CGFunc &f, MemPool *memPool) : CGPeepHole(f, memPool) {};
    ~AArch64CGPeepHole() override = default;

    void Run() override;
    void DoNormalOptimize(BB &bb, Insn &insn) override;
};

// cmp w2, w3/imm
// csel w0, w1, w1, NE   ===> mov w0, w1
class CselToMovPattern : public CGPeepPattern {
public:
    CselToMovPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~CselToMovPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CselToMovPattern";
    }
};

/*
 * case:
 * ldr R261(32), [R197, #300]
 * ldr R262(32), [R208, #12]
 * cmp (CC) R261, R262
 * bne lable175.
 * ldr R264(32), [R197, #304]
 * ldr R265(32), [R208, #16]
 * cmp (CC) R264, R265
 * bne lable175.
 * ====>
 * ldr R261(64), [R197, #300]
 * ldr R262(64), [R208, #12]
 * cmp (CC) R261, R262
 * bne lable175.
 */
class LdrCmpPattern : public CGPeepPattern {
public:
    LdrCmpPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~LdrCmpPattern() override
    {
        prevLdr1 = nullptr;
        prevLdr2 = nullptr;
        ldr1 = nullptr;
        ldr2 = nullptr;
        prevCmp = nullptr;
        bne1 = nullptr;
        bne2 = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "LdrCmpPattern";
    }

private:
    bool IsLdr(const Insn *insn) const
    {
        if (insn == nullptr) {
            return false;
        }
        return insn->GetMachineOpcode() == MOP_wldr;
    }

    bool IsCmp(const Insn *insn) const
    {
        if (insn == nullptr) {
            return false;
        }
        return insn->GetMachineOpcode() == MOP_wcmprr;
    }

    bool IsBne(const Insn *insn) const
    {
        if (insn == nullptr) {
            return false;
        }
        return insn->GetMachineOpcode() == MOP_bne;
    }

    bool SetInsns();
    bool CheckInsns() const;
    bool MemOffet4Bit(const MemOperand &m1, const MemOperand &m2) const;
    Insn *prevLdr1 = nullptr;
    Insn *prevLdr2 = nullptr;
    Insn *ldr1 = nullptr;
    Insn *ldr2 = nullptr;
    Insn *prevCmp = nullptr;
    Insn *bne1 = nullptr;
    Insn *bne2 = nullptr;
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

/* Find up identical two mem insns in local bb to eliminate redundancy, as following:
 * 1. str[BOI] + str[BOI] :
 *    Remove first str insn when the [MEM] operand is exactly same, and the [srcOpnd] of two str don't need to be same,
 *    and there is no redefinition of [srcOpnd] between two strs.
 * 2. str[BOI] + ldr[BOI] :
 *    Remove ldr insn when the [MEM] operand is exactly same and the [srcOpnd] of str
 *    is same as the [destOpnd] of ldr, and there is no redefinition of [srcOpnd] and [destOpnd] between two insns.
 * 3. ldr[BOI] + ldr[BOI] :
 *    Remove second ldr insn
 */
class RemoveIdenticalLoadAndStorePattern : public CGPeepPattern {
public:
    RemoveIdenticalLoadAndStorePattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
        : CGPeepPattern(cgFunc, currBB, currInsn)
    {
    }
    ~RemoveIdenticalLoadAndStorePattern() override
    {
        prevIdenticalInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "RemoveIdenticalLoadAndStorePattern";
    }

private:
    bool IsIdenticalMemOpcode(const Insn &curInsn, const Insn &checkedInsn) const;
    Insn *FindPrevIdenticalMemInsn(const Insn &curInsn) const;
    bool HasImplictSizeUse(const Insn &curInsn) const;
    bool HasMemReferenceBetweenTwoInsns(const Insn &curInsn) const;
    Insn *prevIdenticalInsn = nullptr;
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
 * Combining {2 str into 1 stp || 2 ldr into 1 ldp || 2 strb into 1 strh || 2 strh into 1 str},
 * when they are back to back and the [MEM] they access is conjoined.
 */
class CombineContiLoadAndStorePattern : public CGPeepPattern {
public:
    CombineContiLoadAndStorePattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
        : CGPeepPattern(cgFunc, currBB, currInsn)
    {
        doAggressiveCombine = cgFunc.GetMirModule().IsCModule();
    }
    ~CombineContiLoadAndStorePattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CombineContiLoadAndStorePattern";
    }

private:
    std::vector<Insn *> FindPrevStrLdr(Insn &insn, regno_t destRegNO, regno_t memBaseRegNO, int64 baseOfst) const;
    /*
     * avoid the following situation:
     * str x2, [x19, #8]
     * mov x0, x19
     * bl foo (change memory)
     * str x21, [x19, #16]
     */
    bool IsRegNotSameMemUseInInsn(const Insn &checkInsn, const Insn &curInsn, regno_t curBaseRegNO, bool isCurStore,
                                  int64 curBaseOfst, int64 curMemRange) const;
    bool IsValidNormalLoadOrStorePattern(const Insn &insn, const Insn &prevInsn, const MemOperand &memOpnd,
                                         int64 curOfstVal, int64 prevOfstVal);
    bool IsValidStackArgLoadOrStorePattern(const Insn &curInsn, const Insn &prevInsn, const MemOperand &curMemOpnd,
                                           const MemOperand &prevMemOpnd, int64 curOfstVal, int64 prevOfstVal) const;
    Insn *GenerateMemPairInsn(MOperator newMop, RegOperand &curDestOpnd, RegOperand &prevDestOpnd,
                              MemOperand &combineMemOpnd, bool isCurDestFirst);
    bool FindUseX16AfterInsn(const Insn &curInsn) const;
    void RemoveInsnAndKeepComment(BB &bb, Insn &insn, Insn &prevInsn) const;

    bool doAggressiveCombine = false;
    bool isPairAfterCombine = true;
};

/*
 * add xt, xn, #imm               add  xt, xn, xm
 * ldr xd, [xt]                   ldr xd, [xt]
 * =====================>
 * ldr xd, [xn, #imm]             ldr xd, [xn, xm]
 *
 * load/store can do extend shift as well
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

class EliminateSpecifcSXTPattern : public CGPeepPattern {
public:
    EliminateSpecifcSXTPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~EliminateSpecifcSXTPattern() override
    {
        prevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "EliminateSpecifcSXTPattern";
    }

private:
    Insn *prevInsn = nullptr;
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

class EliminateSpecifcUXTPattern : public CGPeepPattern {
public:
    EliminateSpecifcUXTPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~EliminateSpecifcUXTPattern() override
    {
        prevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "EliminateSpecifcUXTPattern";
    }

private:
    Insn *prevInsn = nullptr;
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
    bool HasImplicitSizeUse(const Insn &insn) const;
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

class AndCbzBranchesToTstPattern : public CGPeepPattern {
public:
    AndCbzBranchesToTstPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~AndCbzBranchesToTstPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "AndCbzBranchesToTstPattern";
    }
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

/* we optimize the following scenarios in this pattern:
 * for example 1:
 * mov     w1, #9
 * cmp     w0, #1              =>               cmp     w0, #1
 * mov     w2, #8                               csel    w0, w0, wzr, EQ
 * csel    w0, w1, w2, EQ                       add     w0, w0, #8
 * for example 2:
 * mov     w1, #8
 * cmp     w0, #1              =>               cmp     w0, #1
 * mov     w2, #9                               cset    w0, NE
 * csel    w0, w1, w2, EQ                       add     w0, w0, #8
 * for example 3:
 * mov     w1, #3
 * cmp     w0, #4              =>               cmp     w0, #4
 * mov     w2, #7                               csel    w0, w0, wzr, EQ
 * csel    w0, w1, w2, NE                       add     w0, w0, #3
 * condition:
 *  1. The source operand of the two mov instructions are immediate operand;
 *  2. The difference value between two immediates is equal to the value being compared in the cmp insn;
 *  3. The reg w1 and w2 are not used in the instructions after csel;
 *  4. The condOpnd in csel insn must be CC_NE or CC_EQ;
 *  5. If the value in w1 is less than value in w2, condition in csel must be CC_NE, otherwise,
 *     the difference between them must be one;
 *  6. If the value in w1 is more than value in w2, condition in csel must be CC_EQ, otherwise,
 *     the difference between them must be one.
 */
class CombineMovInsnBeforeCSelPattern : public CGPeepPattern {
public:
    CombineMovInsnBeforeCSelPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
        : CGPeepPattern(cgFunc, currBB, currInsn)
    {
    }
    ~CombineMovInsnBeforeCSelPattern() override
    {
        insnMov2 = nullptr;
        insnMov1 = nullptr;
        cmpInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "CombineMovInsnBeforeCSelPattern";
    }

private:
    Insn *FindPrevMovInsn(const Insn &insn, regno_t regNo) const;
    Insn *FindPrevCmpInsn(const Insn &insn) const;
    Insn *insnMov2 = nullptr;
    Insn *insnMov1 = nullptr;
    Insn *cmpInsn = nullptr;
    bool needReverseCond = false;
    bool needCsetInsn = false;
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
 *   bl  MCC_IncDecRef_NaiveRCFast
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

// pattern1 :
// -----------------------------------------------------
// lsr(304)  R327 R324 8
// strb(462) R327 [R164, 10]    rev16 R327 R324
// strb(462) R324 [R164, 11] => strh  R327 [R164 10]
// pattern2 :
// ldrb(362) R369 R163 7
// ldrb(362) R371 R163 6           ldrh   R369 R163 6
// add(157)  R374 R369 R371 LSL => rev16  R374 R369

class LdrStrRevPattern : public CGPeepPattern {
public:
    LdrStrRevPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~LdrStrRevPattern() override
    {
        lsrInsn = nullptr;
        adjacentInsn = nullptr;
        curMemOpnd = nullptr;
        adjacentMemOpnd = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "LdrStrRevPattern";
    }

private:
    bool IsAdjacentMem(const MemOperand &memOperandLow, const MemOperand &memOperandHigh) const;
    Insn *lsrInsn = nullptr;
    Insn *adjacentInsn = nullptr;
    MemOperand *curMemOpnd = nullptr;
    MemOperand *adjacentMemOpnd = nullptr;
    bool isStrInsn = false;
};

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
 * Replace following patterns:
 *
 * add   w1, w0, w1
 * cmp   w1, #0       ====>  adds w1, w0, w1
 *       EQ
 *
 * add   x1, x0, x1
 * cmp   x1, #0       ====>  adds x1, x0, x1
 *.......EQ
 *
 * ....
 */
class AddCmpZeroPattern : public CGPeepPattern {
public:
    AddCmpZeroPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~AddCmpZeroPattern() override
    {
        prevInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "AddCmpZeroPattern";
    }

private:
    bool CheckAddCmpZeroCheckAdd(const Insn &insn) const;
    bool CheckAddCmpZeroContinue(const Insn &insn, const RegOperand &opnd) const;
    bool CheckAddCmpZeroCheckCond(const Insn &insn) const;
    Insn *prevInsn = nullptr;
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

class ComplexExtendWordLslPattern : public CGPeepPattern {
public:
    ComplexExtendWordLslPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
    ~ComplexExtendWordLslPattern() override
    {
        useInsn = nullptr;
    }
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "ComplexExtendWordLslPattern";
    }

private:
    Insn *useInsn = nullptr;
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
 * Optimize the following patterns:
 * add x0, x0, #0x0         add x0, x1, #0x0
 * ====>
 * ---                      mov x0, x1
 */
class AddImmZeroToMov : public PeepPattern {
public:
    explicit AddImmZeroToMov(CGFunc &cgFunc) : PeepPattern(cgFunc), cgFunc(&cgFunc) {}
    ~AddImmZeroToMov() override = default;
    void Run(BB &bb, Insn &insn) override;

private:
    CGFunc *cgFunc;
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
        kAddImmZeroToMov,
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
