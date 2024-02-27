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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_GLOBAL_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_GLOBAL_H

#include "global.h"
#include "aarch64_operand.h"

namespace maplebe {
using namespace maple;

class AArch64GlobalOpt : public GlobalOpt {
public:
    explicit AArch64GlobalOpt(CGFunc &func, LoopAnalysis &loop) : GlobalOpt(func, loop) {}
    ~AArch64GlobalOpt() override = default;
    void Run() override;
};

class OptimizeManager {
public:
    explicit OptimizeManager(CGFunc &cgFunc, LoopAnalysis &loop) : cgFunc(cgFunc), loopInfo(loop) {}
    ~OptimizeManager() = default;
    template <typename OptimizePattern>
    void Optimize()
    {
        OptimizePattern optPattern(cgFunc, loopInfo);
        optPattern.Run();
    }

private:
    CGFunc &cgFunc;
    LoopAnalysis &loopInfo;
};

class OptimizePattern {
public:
    explicit OptimizePattern(CGFunc &cgFunc, LoopAnalysis &loop) : cgFunc(cgFunc), loopInfo(loop) {}
    virtual ~OptimizePattern() = default;
    virtual bool CheckCondition(Insn &insn) = 0;
    virtual void Optimize(Insn &insn) = 0;
    virtual void Run() = 0;
    bool OpndDefByOne(Insn &insn, int32 useIdx) const;
    bool OpndDefByZero(Insn &insn, int32 useIdx) const;
    bool OpndDefByOneOrZero(Insn &insn, int32 useIdx) const;
    void ReplaceAllUsedOpndWithNewOpnd(const InsnSet &useInsnSet, uint32 regNO, Operand &newOpnd,
                                       bool updateInfo) const;

    static bool InsnDefOne(const Insn &insn);
    static bool InsnDefZero(const Insn &insn);
    static bool InsnDefOneOrZero(const Insn &insn);

    std::string PhaseName() const
    {
        return "globalopt";
    }

protected:
    virtual void Init() = 0;
    CGFunc &cgFunc;
    LoopAnalysis &loopInfo;
};

/*
 * Do Forward prop when insn is mov
 * mov xx, x1
 * ... // BBs and x1 is live
 * mOp yy, xx
 *
 * =>
 * mov x1, x1
 * ... // BBs and x1 is live
 * mOp yy, x1
 */
class ForwardPropPattern : public OptimizePattern {
public:
    explicit ForwardPropPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~ForwardPropPattern() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    InsnSet firstRegUseInsnSet;
    void RemoveMopUxtwToMov(Insn &insn);
    std::set<BB *, BBIdCmp> modifiedBB;
};

/*
 * Do back propagate of vreg/preg when encount following insn:
 *
 * mov vreg/preg1, vreg2
 *
 * back propagate reg1 to all vreg2's use points and def points, when all of them is in same bb
 */
class BackPropPattern : public OptimizePattern {
public:
    explicit BackPropPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~BackPropPattern() override
    {
        firstRegOpnd = nullptr;
        secondRegOpnd = nullptr;
        defInsnForSecondOpnd = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    bool CheckAndGetOpnd(const Insn &insn);
    bool DestOpndHasUseInsns(Insn &insn);
    bool CheckSrcOpndDefAndUseInsns(Insn &insn);
    bool CheckSrcOpndDefAndUseInsnsGlobal(Insn &insn);
    bool CheckPredefineInsn(Insn &insn);
    bool CheckRedefineInsn(Insn &insn);
    bool CheckReplacedUseInsn(Insn &insn);
    RegOperand *firstRegOpnd = nullptr;
    RegOperand *secondRegOpnd = nullptr;
    uint32 firstRegNO = 0;
    uint32 secondRegNO = 0;
    InsnSet srcOpndUseInsnSet;
    Insn *defInsnForSecondOpnd = nullptr;
    bool globalProp = false;
};

/*
 *  when w0 has only one valid bit, these tranformation will be done
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
class CmpCsetPattern : public OptimizePattern {
public:
    explicit CmpCsetPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~CmpCsetPattern() override
    {
        nextInsn = nullptr;
        cmpFirstOpnd = nullptr;
        cmpSecondOpnd = nullptr;
        csetFirstOpnd = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    Insn *nextInsn = nullptr;
    int64 cmpConstVal = 0;
    Operand *cmpFirstOpnd = nullptr;
    Operand *cmpSecondOpnd = nullptr;
    Operand *csetFirstOpnd = nullptr;
};

/*
 * mov w5, #1
 *  ...                   --> cset w5, NE
 * mov w0, #0
 * csel w5, w5, w0, NE
 *
 * mov w5, #0
 *  ...                   --> cset w5,EQ
 * mov w0, #1
 * csel w5, w5, w0, NE
 *
 * condition:
 *    1.all define points of w5 are defined by:   mov w5, #1(#0)
 *    2.all define points of w0 are defined by:   mov w0, #0(#1)
 *    3.w0 will not be used after: csel w5, w5, w0, NE(EQ)
 */
class CselPattern : public OptimizePattern {
public:
    explicit CselPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~CselPattern() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final {}
};

/*
 * uxtb  w0, w0    -->   null
 * uxth  w0, w0    -->   null
 *
 * condition:
 * 1. validbits(w0)<=8,16,32
 * 2. the first operand is same as the second operand
 *
 * uxtb  w0, w1    -->   null
 * uxth  w0, w1    -->   null
 *
 * condition:
 * 1. validbits(w1)<=8,16,32
 * 2. the use points of w0 has only one define point, that is uxt w0, w1
 */
class RedundantUxtPattern : public OptimizePattern {
public:
    explicit RedundantUxtPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~RedundantUxtPattern() override
    {
        secondOpnd = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    uint32 GetMaximumValidBit(Insn &insn, uint8 udIdx, InsnSet &insnChecked) const;
    static uint32 GetInsnValidBit(const Insn &insn);
    InsnSet useInsnSet;
    uint32 firstRegNO = 0;
    Operand *secondOpnd = nullptr;
};

/*
 *  bl  MCC_NewObj_flexible_cname                              bl  MCC_NewObj_flexible_cname
 *  mov x21, x0   //  [R203]
 *  str x0, [x29,#16]   // local var: Reg0_R6340 [R203]  -->   str x0, [x29,#16]   // local var: Reg0_R6340 [R203]
 *  ... (has call)                                             ... (has call)
 *  mov x2, x21  // use of x21                                 ldr x2, [x29, #16]
 *  bl ***                                                     bl ***
 */
class LocalVarSaveInsnPattern : public OptimizePattern {
public:
    explicit LocalVarSaveInsnPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~LocalVarSaveInsnPattern() override
    {
        firstInsnSrcOpnd = nullptr;
        firstInsnDestOpnd = nullptr;
        secondInsnSrcOpnd = nullptr;
        secondInsnDestOpnd = nullptr;
        useInsn = nullptr;
        secondInsn = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    bool CheckFirstInsn(const Insn &firstInsn);
    bool CheckSecondInsn();
    bool CheckAndGetUseInsn(Insn &firstInsn);
    bool CheckLiveRange(const Insn &firstInsn);
    Operand *firstInsnSrcOpnd = nullptr;
    Operand *firstInsnDestOpnd = nullptr;
    Operand *secondInsnSrcOpnd = nullptr;
    Operand *secondInsnDestOpnd = nullptr;
    Insn *useInsn = nullptr;
    Insn *secondInsn = nullptr;
};

class ExtendShiftOptPattern : public OptimizePattern {
public:
    explicit ExtendShiftOptPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~ExtendShiftOptPattern() override
    {
        defInsn = nullptr;
        newInsn = nullptr;
    }
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
    bool CheckDefUseInfo(Insn &use, uint32 size);
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
    bool optSuccess;
    bool removeDefInsn;
    ExMOpType exMOpType;
    LsMOpType lsMOpType;
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
class ExtenToMovPattern : public OptimizePattern {
public:
    explicit ExtenToMovPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~ExtenToMovPattern() override = default;
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    bool CheckHideUxtw(const Insn &insn, regno_t regno) const;
    bool CheckUxtw(Insn &insn);
    bool BitNotAffected(Insn &insn, uint32 validNum); /* check whether significant bits are affected */
    bool CheckSrcReg(Insn &insn, regno_t srcRegNo, uint32 validNum);

    MOperator replaceMop = MOP_undef;
};

class SameDefPattern : public OptimizePattern {
public:
    explicit SameDefPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~SameDefPattern() override
    {
        currInsn = nullptr;
        sameInsn = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    bool IsSameDef();
    bool SrcRegIsRedefined(regno_t regNo);
    bool IsSameOperand(Operand &opnd0, Operand &opnd1);

    Insn *currInsn = nullptr;
    Insn *sameInsn = nullptr;
};

/*
 * and r0, r0, #4        (the imm is n power of 2)
 * ...                   (r0 is not used)
 * cbz r0, .Label
 * ===>  tbz r0, #2, .Label
 *
 * and r0, r0, #4        (the imm is n power of 2)
 * ...                   (r0 is not used)
 * cbnz r0, .Label
 * ===>  tbnz r0, #2, .Label
 */
class AndCbzPattern : public OptimizePattern {
public:
    explicit AndCbzPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~AndCbzPattern() override
    {
        prevInsn = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    int64 CalculateLogValue(int64 val) const;
    bool IsAdjacentArea(Insn &prev, Insn &curr) const;
    Insn *prevInsn = nullptr;
};

/*
 * [arithmetic operation]
 * add/sub/ R202, R201, #1            add/sub/ R202, R201, #1
 * ...                           ...
 * add/sub/ R203, R201, #1    --->    mov R203, R202
 *
 * [copy operation]
 * mov R201, #1                  mov R201, #1
 * ...                           ...
 * mov R202, #1          --->    mov R202, R201
 *
 * The pattern finds the insn with the same rvalue as the current insn,
 * then prop its lvalue, and replaces the current insn with movrr insn.
 * The mov can be prop in forwardprop or backprop.
 *
 * conditions:
 * 1. in same BB
 * 2. rvalue is not defined between two insns
 * 3. lvalue is not defined between two insns
 */
class SameRHSPropPattern : public OptimizePattern {
public:
    explicit SameRHSPropPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
    ~SameRHSPropPattern() override
    {
        prevInsn = nullptr;
    }
    bool CheckCondition(Insn &insn) final;
    void Optimize(Insn &insn) final;
    void Run() final;

protected:
    void Init() final;

private:
    bool IsSameOperand(Operand *opnd1, Operand *opnd2) const;
    bool FindSameRHSInsnInBB(Insn &insn);
    Insn *prevInsn = nullptr;
    std::vector<MOperator> candidates;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_GLOBAL_H */
