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
#include "loop.h"

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

class FlipBRPattern : public OptimizationPattern {
public:
    explicit FlipBRPattern(CGFunc &func, LoopAnalysis &loop) : OptimizationPattern(func), loopInfo(loop)
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
    LoopAnalysis &loopInfo;

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

class CFGOptimizer : public Optimizer {
public:
    CFGOptimizer(CGFunc &func, MemPool &memPool, LoopAnalysis &loop) : Optimizer(func, memPool), loopInfo(loop)
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
    LoopAnalysis &loopInfo;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgCfgo, maplebe::CGFunc)
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_CFGO_H