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
class ChainingPattern : public OptimizationPattern {
public:
    explicit ChainingPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "BB Chaining";
        dotColor = kCfgoChaining;
    }

    virtual ~ChainingPattern() = default;
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

    virtual ~SequentialJumpPattern() = default;
    bool Optimize(BB &curBB) override;

protected:
    void SkipSucBB(BB &curBB, BB &sucBB);
    void UpdateSwitchSucc(BB &curBB, BB &sucBB);
};

class FlipBRPattern : public OptimizationPattern {
public:
    explicit FlipBRPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Condition Flip";
        dotColor = kCfgoFlipCond;
    }

    virtual ~FlipBRPattern() = default;
    bool Optimize(BB &curBB) override;

protected:
    void RelocateThrowBB(BB &curBB);

private:
    virtual uint32 GetJumpTargetIdx(const Insn &insn) = 0;
    virtual MOperator FlipConditionOp(MOperator flippedOp) = 0;
};

/* This class represents the scenario that the BB is unreachable. */
class UnreachBBPattern : public OptimizationPattern {
public:
    explicit UnreachBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Unreachable BB";
        dotColor = kCfgoUnreach;
        func.GetTheCFG()->FindAndMarkUnreachable(*cgFunc);
    }

    virtual ~UnreachBBPattern() = default;
    bool Optimize(BB &curBB) override;
};

/*
 * This class represents the scenario that a common jump BB can be duplicated
 * to one of its another predecessor.
 */
class DuplicateBBPattern : public OptimizationPattern {
public:
    explicit DuplicateBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Duplicate BB";
        dotColor = kCfgoDup;
    }

    virtual ~DuplicateBBPattern() = default;
    bool Optimize(BB &curBB) override;

private:
    static constexpr int kThreshold = 10;
};

/*
 * This class represents the scenario that a BB contains nothing.
 */
class EmptyBBPattern : public OptimizationPattern {
public:
    explicit EmptyBBPattern(CGFunc &func) : OptimizationPattern(func)
    {
        patternName = "Empty BB";
        dotColor = kCfgoEmpty;
    }

    virtual ~EmptyBBPattern() = default;
    bool Optimize(BB &curBB) override;
};

class CFGOptimizer : public Optimizer {
public:
    CFGOptimizer(CGFunc &func, MemPool &memPool) : Optimizer(func, memPool)
    {
        name = "CFGO";
    }

    virtual ~CFGOptimizer() = default;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPostCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_CFGO_H */
