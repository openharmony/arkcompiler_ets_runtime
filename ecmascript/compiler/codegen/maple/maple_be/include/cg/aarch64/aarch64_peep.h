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

/*
 * add x0, x1, x2
 * mov x19, x0
 * ==>
 * add x19, x1, x2
 */
class RedundantMovAArch64 : public PeepPattern {
public:
    explicit RedundantMovAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
    ~RedundantMovAArch64() override = default;
    void Run(BB &bb, Insn &insn) override;
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
        kRedundantMovAArch64Opt,
        kRemoveMovingtoSameRegOpt,
        kEnhanceStrLdrAArch64Opt,
        kAddImmZeroToMov,
        kPeepholeOptsNum
    };
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H */
