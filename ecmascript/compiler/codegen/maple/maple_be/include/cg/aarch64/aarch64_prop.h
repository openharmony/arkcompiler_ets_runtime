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

#ifndef MAPLEBE_INCLUDE_AARCH64_PROP_H
#define MAPLEBE_INCLUDE_AARCH64_PROP_H

#include "cg_prop.h"
#include "aarch64_cgfunc.h"
#include "aarch64_strldr.h"
namespace maplebe {
class AArch64Prop : public CGProp {
public:
    AArch64Prop(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, LiveIntervalAnalysis &ll) : CGProp(mp, f, sInfo, ll) {}
    ~AArch64Prop() override = default;

    /* do not extend life range */
    static bool IsInLimitCopyRange(VRegVersion *toBeReplaced);

private:
    void CopyProp() override;
    /*
     * for aarch64
     * 1. extended register prop
     * 2. shift register prop
     * 3. add/ext/shf prop -> str/ldr
     * 4. const prop
     */
    void TargetProp(Insn &insn) override;
    void PropPatternOpt() override;
};

class A64StrLdrProp {
public:
    A64StrLdrProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, Insn &insn, CGDce &dce)
        : cgFunc(&f),
          ssaInfo(&sInfo),
          curInsn(&insn),
          a64StrLdrAlloc(&mp),
          replaceVersions(a64StrLdrAlloc.Adapter()),
          cgDce(&dce)
    {
    }
    void DoOpt();

private:
    MemOperand *StrLdrPropPreCheck(const Insn &insn, MemPropMode prevMod = kUndef);
    static MemPropMode SelectStrLdrPropMode(const MemOperand &currMemOpnd);
    bool ReplaceMemOpnd(const MemOperand &currMemOpnd, const Insn *defInsn);
    MemOperand *SelectReplaceMem(const Insn &defInsn, const MemOperand &currMemOpnd);
    RegOperand *GetReplaceReg(RegOperand &a64Reg);
    MemOperand *HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal, uint32 memSize) const;
    MemOperand *SelectReplaceExt(const Insn &defInsn, RegOperand &base, uint32 amount, bool isSigned, uint32 memSize);
    bool CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx) const;
    void DoMemReplace(const RegOperand &replacedReg, MemOperand &newMem, Insn &useInsn);
    uint32 GetMemOpndIdx(MemOperand *newMemOpnd, const Insn &insn) const;

    bool CheckSameReplace(const RegOperand &replacedReg, const MemOperand *memOpnd) const;

    CGFunc *cgFunc;
    CGSSAInfo *ssaInfo;
    Insn *curInsn;
    MapleAllocator a64StrLdrAlloc;
    MapleMap<regno_t, VRegVersion *> replaceVersions;
    MemPropMode memPropMode = kUndef;
    CGDce *cgDce = nullptr;
};

enum ArithmeticType { kAArch64Add, kAArch64Sub, kAArch64Orr, kAArch64Eor, kUndefArith };

class A64ConstProp {
public:
    A64ConstProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, Insn &insn)
        : constPropMp(&mp), cgFunc(&f), ssaInfo(&sInfo), curInsn(&insn)
    {
    }
    void DoOpt();
    /* false : default lsl #0 true: lsl #12 (only support 12 bit left shift in aarch64) */
    static MOperator GetRegImmMOP(MOperator regregMop, bool withLeftShift);
    static MOperator GetReversalMOP(MOperator arithMop);
    static MOperator GetFoldMopAndVal(int64 &newVal, int64 constVal, const Insn &arithInsn);

private:
    bool ConstProp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd);
    /* use xzr/wzr in aarch64 to shrink register live range */
    void ZeroRegProp(DUInsnInfo &useDUInfo, RegOperand &toReplaceReg);

    /* replace old Insn with new Insn, update ssa info automatically */
    void ReplaceInsnAndUpdateSSA(Insn &oriInsn, Insn &newInsn) const;
    ImmOperand *CanDoConstFold(const ImmOperand &value1, const ImmOperand &value2, ArithmeticType aT, bool is64Bit);

    /* optimization */
    bool MovConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd);
    bool ArithmeticConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd, ArithmeticType aT);
    bool ArithmeticConstFold(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd, ArithmeticType aT);
    bool ShiftConstReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd);
    bool BitInsertReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd);

    MemPool *constPropMp;
    CGFunc *cgFunc;
    CGSSAInfo *ssaInfo;
    Insn *curInsn;
};

class CopyRegProp : public PropOptimizePattern {
public:
    CopyRegProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo, LiveIntervalAnalysis *ll)
        : PropOptimizePattern(cgFunc, cgssaInfo, ll)
    {
    }
    ~CopyRegProp() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final
    {
        destVersion = nullptr;
        srcVersion = nullptr;
    }

private:
    bool IsValidCopyProp(const RegOperand &dstReg, const RegOperand &srcReg) const;
    void VaildateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn);
    VRegVersion *destVersion = nullptr;
    VRegVersion *srcVersion = nullptr;
};

class RedundantPhiProp : public PropOptimizePattern {
public:
    RedundantPhiProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
    ~RedundantPhiProp() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final
    {
        destVersion = nullptr;
        srcVersion = nullptr;
    }

private:
    VRegVersion *destVersion = nullptr;
    VRegVersion *srcVersion = nullptr;
};

class ValidBitNumberProp : public PropOptimizePattern {
public:
    ValidBitNumberProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
    ~ValidBitNumberProp() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final
    {
        destVersion = nullptr;
        srcVersion = nullptr;
    }

private:
    VRegVersion *destVersion = nullptr;
    VRegVersion *srcVersion = nullptr;
};

/*
 * frame pointer and stack pointer will not be varied in function body
 * treat them as const
 */
class FpSpConstProp : public PropOptimizePattern {
public:
    FpSpConstProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
    ~FpSpConstProp() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final
    {
        fpSpBase = nullptr;
        shiftOpnd = nullptr;
        aT = kUndefArith;
        replaced = nullptr;
    }

private:
    bool GetValidSSAInfo(Operand &opnd);
    void PropInMem(DUInsnInfo &useDUInfo, Insn &useInsn);
    void PropInArith(DUInsnInfo &useDUInfo, Insn &useInsn, ArithmeticType curAT);
    void PropInCopy(DUInsnInfo &useDUInfo, Insn &useInsn, MOperator oriMop);
    int64 ArithmeticFold(int64 valInUse, ArithmeticType useAT) const;

    RegOperand *fpSpBase = nullptr;
    ImmOperand *shiftOpnd = nullptr;
    ArithmeticType aT = kUndefArith;
    VRegVersion *replaced = nullptr;
};

/*
 * This pattern do:
 * 1)
 * uxtw vreg:Rm validBitNum:[64], vreg:Rn validBitNum:[32]
 * ------>
 * mov vreg:Rm validBitNum:[64], vreg:Rn validBitNum:[32]
 * 2)
 * ldrh  R201, [...]
 * and R202, R201, #65520
 * uxth  R203, R202
 * ------->
 * ldrh  R201, [...]
 * and R202, R201, #65520
 * mov  R203, R202
 */
class ExtendMovPattern : public PropOptimizePattern {
public:
    ExtendMovPattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
    ~ExtendMovPattern() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    bool BitNotAffected(const Insn &insn, uint32 validNum); /* check whether significant bits are affected */
    bool CheckSrcReg(regno_t srcRegNo, uint32 validNum);

    MOperator replaceMop = MOP_undef;
};

class ExtendShiftPattern : public PropOptimizePattern {
public:
    ExtendShiftPattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
    ~ExtendShiftPattern() override = default;
    bool IsSwapInsn(const Insn &insn) const;
    void SwapOpnd(Insn &insn);
    bool CheckAllOpndCondition(Insn &insn);
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;
    void DoExtendShiftOpt(Insn &insn);

    enum ExMOpType : uint8 {
        kExUndef,
        kExAdd, /* MOP_xaddrrr | MOP_xxwaddrrre | MOP_xaddrrrs */
        kEwAdd, /* MOP_waddrrr | MOP_wwwaddrrre | MOP_waddrrrs */
        kExSub, /* MOP_xsubrrr | MOP_xxwsubrrre | MOP_xsubrrrs */
        kEwSub, /* MOP_wsubrrr | MOP_wwwsubrrre | MOP_wsubrrrs */
        kExCmn, /* MOP_xcmnrr | MOP_xwcmnrre | MOP_xcmnrrs */
        kEwCmn, /* MOP_wcmnrr | MOP_wwcmnrre | MOP_wcmnrrs */
        kExCmp, /* MOP_xcmprr | MOP_xwcmprre | MOP_xcmprrs */
        kEwCmp, /* MOP_wcmprr | MOP_wwcmprre | MOP_wcmprrs */
    };

    enum LsMOpType : uint8 {
        kLsUndef,
        kLxAdd, /* MOP_xaddrrr | MOP_xaddrrrs */
        kLwAdd, /* MOP_waddrrr | MOP_waddrrrs */
        kLxSub, /* MOP_xsubrrr | MOP_xsubrrrs */
        kLwSub, /* MOP_wsubrrr | MOP_wsubrrrs */
        kLxCmn, /* MOP_xcmnrr | MOP_xcmnrrs */
        kLwCmn, /* MOP_wcmnrr | MOP_wcmnrrs */
        kLxCmp, /* MOP_xcmprr | MOP_xcmprrs */
        kLwCmp, /* MOP_wcmprr | MOP_wcmprrs */
        kLxEor, /* MOP_xeorrrr | MOP_xeorrrrs */
        kLwEor, /* MOP_weorrrr | MOP_weorrrrs */
        kLxNeg, /* MOP_xinegrr | MOP_xinegrrs */
        kLwNeg, /* MOP_winegrr | MOP_winegrrs */
        kLxIor, /* MOP_xiorrrr | MOP_xiorrrrs */
        kLwIor, /* MOP_wiorrrr | MOP_wiorrrrs */
    };

    enum SuffixType : uint8 {
        kNoSuffix, /* no suffix or do not perform the optimization. */
        kLSL,      /* logical shift left */
        kLSR,      /* logical shift right */
        kASR,      /* arithmetic shift right */
        kExten     /* ExtendOp */
    };

protected:
    void Init() final;

private:
    void SelectExtendOrShift(const Insn &def);
    SuffixType CheckOpType(const Operand &lastOpnd) const;
    void ReplaceUseInsn(Insn &use, const Insn &def, uint32 amount);
    void SetExMOpType(const Insn &use);
    void SetLsMOpType(const Insn &use);

    MOperator replaceOp;
    uint32 replaceIdx;
    ExtendShiftOperand::ExtendOp extendOp;
    BitShiftOperand::ShiftOp shiftOp;
    Insn *defInsn = nullptr;
    Insn *newInsn = nullptr;
    Insn *curInsn = nullptr;
    bool optSuccess;
    ExMOpType exMOpType;
    LsMOpType lsMOpType;
};

/*
 * optimization for call convention
 * example:
 *                              [BB26]                      [BB43]
 *                        sub R287, R101, R275        sub R279, R101, R275
 *                                            \      /
 *                                             \    /
 *                                             [BB27]
 *                                                <---- insert new phi: R403, (R275 <26>, R275 <43>)
 *                                       old phi: R297, (R287 <26>, R279 <43>)
 *                                               /          \
 *                                              /            \
 *                                           [BB28]           \
 *                                       sub R310, R101, R309  \
 *                                             |                \
 *                                             |                 \
 *            [BB17]                         [BB29]            [BB44]
 *        sub R314, R101, R275                 |              /
 *                                \            |             /
 *                                 \           |            /
 *                                  \          |           /
 *                                   \         |          /
 *                                           [BB18]
 *                                               <---- insert new phi: R404, (R275 <17>, R309 <29>, R403 <44>)
 *                                       old phi: R318, (R314 <17>, R310 <29>, R297 <44>)
 *                                       mov R1, R318    ====>    sub R1, R101, R404
 *                                          /                   \
 *                                         /                     \
 *                                        /                       \
 *                                     [BB19]                   [BB34]
 *                              sub R336, R101, R335           /
 *                                               \            /
 *                                                \          /
 *                                                 \        /
 *                                                   [BB20]
 *                                                      <---- insert new phi: R405, (R335 <19>, R404<34>)
 *                                                old phi: R340, (R336 <19>, R318 <34>)
 *                                                mov R1, R340      ====>     sub R1, R101, R405
 */
class A64PregCopyPattern : public PropOptimizePattern {
public:
    A64PregCopyPattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
    ~A64PregCopyPattern() override
    {
        firstPhiInsn = nullptr;
    }
    bool CheckCondition(Insn &insn) override;
    void Optimize(Insn &insn) override;
    void Run() override;

protected:
    void Init() override
    {
        validDefInsns.clear();
        firstPhiInsn = nullptr;
        differIdx = -1;
        differOrigNO = 0;
        isCrossPhi = false;
    }

private:
    bool CheckUselessDefInsn(const Insn *defInsn) const;
    bool CheckValidDefInsn(const Insn *defInsn);
    bool CheckMultiUsePoints(const Insn *defInsn) const;
    bool CheckPhiCaseCondition(Insn &curInsn, Insn &defInsn);
    bool DFSFindValidDefInsns(Insn *curDefInsn, RegOperand *lastPhiDef, std::unordered_map<uint32, bool> &visited);
    Insn &CreateNewPhiInsn(std::unordered_map<uint32, RegOperand *> &newPhiList, Insn *curInsn);
    RegOperand &DFSBuildPhiInsn(Insn *curInsn, std::unordered_map<uint32, RegOperand *> &visited);
    RegOperand *CheckAndGetExistPhiDef(Insn &phiInsn, std::vector<regno_t> &validDifferRegNOs) const;
    std::vector<Insn *> validDefInsns;
    Insn *firstPhiInsn = nullptr;
    int differIdx = -1;
    regno_t differOrigNO = 0;
    bool isCrossPhi = false;
};

class A64ReplaceRegOpndVisitor : public ReplaceRegOpndVisitor {
public:
    A64ReplaceRegOpndVisitor(CGFunc &f, Insn &cInsn, uint32 cIdx, RegOperand &oldRegister, RegOperand &newRegister)
        : ReplaceRegOpndVisitor(f, cInsn, cIdx, oldRegister, newRegister)
    {
    }
    ~A64ReplaceRegOpndVisitor() override = default;

private:
    void Visit(RegOperand *v) final;
    void Visit(ListOperand *v) final;
    void Visit(MemOperand *v) final;
    void Visit(PhiOperand *v) final;
};
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_AARCH64_PROP_H */
