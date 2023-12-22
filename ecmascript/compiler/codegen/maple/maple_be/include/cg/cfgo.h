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

#ifndef MAPLEBE_INCLUDE_CG_CFGO_H
#define MAPLEBE_INCLUDE_CG_CFGO_H
#include "cg_cfg.h"
#include "optimize_common.h"

namespace maplebe {

enum CfgoPhase : maple::uint8 {
    kCfgoDefault,
    kCfgoPreRegAlloc,
    kCfgoPostRegAlloc,
    kPostCfgo,
};

class ChainingPattern : public OptimizationPattern {
public:
    explicit ChainingPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "BB Chaining";
        dotColor = kCfgoChaining;
    }
    ~ChainingPattern() override = default;

    bool Optimize(BB &curBB) override;

protected:
    bool NoInsnBetween(const BB &from, const BB &to) const;
    bool DoSameThing(const BB &bb1, const Insn &last1, const BB &bb2, const Insn &last2) const;
    bool MergeFallthuBB(BB &curBB);
    bool MergeGotoBB(BB &curBB, BB &sucBB);
    bool MoveSuccBBAsCurBBNext(BB &curBB, BB &sucBB);
    bool RemoveGotoInsn(BB &curBB, BB &sucBB);
    bool ClearCurBBAndResetTargetBB(BB &curBB, BB &sucBB);
};

class SequentialJumpPattern : public OptimizationPattern {
public:
    explicit SequentialJumpPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Sequential Jump";
        dotColor = kCfgoSj;
    }

    ~SequentialJumpPattern() override = default;
    bool Optimize(BB &curBB) override;

protected:
    void SkipSucBB(BB &curBB, BB &sucBB) const;
    void UpdateSwitchSucc(BB &curBB, BB &sucBB) const;
    // If the sucBB has one invalid predBB, the sucBB can not be skipped
    bool HasInvalidPred(BB &sucBB) const;
};

class FlipBRPattern : public OptimizationPattern {
public:
    explicit FlipBRPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Condition Flip";
        dotColor = kCfgoFlipCond;
    }

    ~FlipBRPattern() override = default;
    bool Optimize(BB &curBB) override;

    CfgoPhase GetPhase() const
    {
        return phase;
    }
    void SetPhase(CfgoPhase val)
    {
        phase = val;
    }
    CfgoPhase phase = kCfgoDefault;

protected:
    void RelocateThrowBB(BB &curBB);

private:
    virtual uint32 GetJumpTargetIdx(const Insn &insn) = 0;
    virtual MOperator FlipConditionOp(MOperator flippedOp) = 0;
};

// This class represents the scenario that the BB is unreachable.
class UnreachBBPattern : public OptimizationPattern {
public:
    explicit UnreachBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Unreachable BB";
        dotColor = kCfgoUnreach;
    }

    ~UnreachBBPattern() override = default;

    void InitPattern() override
    {
        cgFunc->GetTheCFG()->FindAndMarkUnreachable(*cgFunc);
    }

    bool Optimize(BB &curBB) override;
};

// This class represents the scenario that a common jump BB can be duplicated
// to one of its another predecessor.
class DuplicateBBPattern : public OptimizationPattern {
public:
    explicit DuplicateBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Duplicate BB";
        dotColor = kCfgoDup;
    }

    ~DuplicateBBPattern() override = default;
    bool Optimize(BB &curBB) override;

private:
    static constexpr int kThreshold = 10;
};

// This class represents the scenario that a BB contains nothing.
class EmptyBBPattern : public OptimizationPattern {
public:
    explicit EmptyBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Empty BB";
        dotColor = kCfgoEmpty;
    }

    ~EmptyBBPattern() override = default;
    bool Optimize(BB &curBB) override;
};

/* This class represents that two pred of a BB can cross-jump
 * BB1: insn1                  BB1: insn1
 *      insn2                       b BB4 (branch newBB)
 *      insn3                  BB2: insn4 (fallthru, no need branch)
 *      b BB3                  BB4: insn2
 * BB2: insn4        ==>            insn3
 *      insn2                       b BB3
 *      insn3
 *      b BB3
 * BB1 & BB2 is BB3's pred, and has same insns(insn2, insn3).
 * In BB2, we can split it into two BBs and set the newBB to be the fallthru of BB2.
 * In BB1, same insns need to be deleted, and a jump insn pointing to the new BB is
 * generated at the end of the BB.
 */
class CrossJumpBBPattern : public OptimizationPattern {
public:
    explicit CrossJumpBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Cross Jump BB";
        dotColor = kCfgoEmpty;
    }
    ~CrossJumpBBPattern() override = default;

    bool Optimize(BB &curBB) override;

private:
    enum MergeDirection {
        kDirectionNone,
        kDirectionForward,
        kDirectionBackward,
        kDirectionBoth,
    };
    bool OptimizeOnce(const BB &curBB);
    bool TryCrossJumpBB(BB &bb1, BB &bb2, MergeDirection dir = kDirectionBoth);
    bool CheckBBSuccMatch(BB &bb1, BB &bb2);
    uint32 CheckBBInsnsMatch(BB &bb1, BB &bb2, Insn *&f1, Insn *&f2);
    void MergeMemInfo(BB &bb1, Insn &newpos1, BB &redirectBB);
    void GetMergeDirection(BB &bb1, BB &bb2, const Insn &f1, const Insn &f2, MergeDirection &dir);

    BB &SplitBB(BB &srcBB, Insn &lastInsn);

    virtual uint32 GetJumpTargetIdx(const Insn &insn) const = 0;
    virtual MOperator FlipConditionOp(MOperator flippedOp) const = 0;
    virtual MOperator GetUnCondBranchMOP() const = 0;

    static constexpr uint32 kMaxCrossJumpPreds = 50;  // limit for compiler time
    static constexpr uint32 kMinCrossJumpPreds = 2;   // Noting to do if there is not at least two incoming edges
};

class CFGOptimizer : public Optimizer {
public:
    CFGOptimizer(CGFunc &func, MemPool &memPool) : Optimizer(func, memPool)
    {
        name = "CFGO";
    }

    ~CFGOptimizer() override = default;
    CfgoPhase GetPhase() const
    {
        return phase;
    }
    void SetPhase(CfgoPhase val)
    {
        phase = val;
    }

protected:
    CfgoPhase phase = kCfgoDefault;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPreCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPostCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_CFGO_H