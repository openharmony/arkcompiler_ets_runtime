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

#ifndef MAPLEBE_INCLUDE_CG_PEEP_H
#define MAPLEBE_INCLUDE_CG_PEEP_H

#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
enum ReturnType : uint8 { kResUseFirst, kResDefFirst, kResNotFind };

class PeepOptimizeManager {
public:
    /* normal constructor */
    PeepOptimizeManager(CGFunc &f, BB &bb, Insn &insn) : cgFunc(&f), currBB(&bb), currInsn(&insn) {}

    ~PeepOptimizeManager() = default;
    template <typename OptimizePattern>
    void Optimize(bool patternEnable = false)
    {
        if (!patternEnable) {
            return;
        }
        OptimizePattern optPattern(*cgFunc, *currBB, *currInsn);
        optPattern.Run(*currBB, *currInsn);
        optSuccess = optPattern.GetPatternRes() || optSuccess;
        if (optSuccess && optPattern.GetCurrInsn() != nullptr) {
            currInsn = optPattern.GetCurrInsn();
        }
    }
    template <typename OptimizePattern>
    void NormalPatternOpt(bool patternEnable = false)
    {
        if (!patternEnable) {
            return;
        }
        OptimizePattern optPattern(*cgFunc, *currBB, *currInsn);
        optPattern.Run(*currBB, *currInsn);
        optSuccess = optPattern.GetPatternRes() || optSuccess;
        if (optSuccess && optPattern.GetCurrInsn() != nullptr) {
            currInsn = optPattern.GetCurrInsn();
        }
    }
    void SetOptSuccess(bool optRes)
    {
        optSuccess = optRes;
    }
    bool OptSuccess() const
    {
        return optSuccess;
    }

private:
    CGFunc *cgFunc;
    BB *currBB;
    Insn *currInsn;
    /*
     * The flag indicates whether the optimization pattern is successful,
     * this prevents the next optimization pattern that processs the same mop from failing to get the validInsn,
     * which was changed by previous pattern.
     *
     * Set the flag to true when the pattern optimize successfully.
     */
    bool optSuccess = false;
};

class CGPeepHole {
public:
    /* normal constructor */
    CGPeepHole(CGFunc &f, MemPool *memPool) : cgFunc(&f), peepMemPool(memPool) {}
    virtual ~CGPeepHole() { }

    virtual void Run() = 0;
    virtual void DoNormalOptimize(BB &bb, Insn &insn) = 0;

protected:
    CGFunc *cgFunc;
    MemPool *peepMemPool;
    PeepOptimizeManager *manager = nullptr;
};

class PeepPattern {
public:
    explicit PeepPattern(CGFunc &oneCGFunc) : cgFunc(oneCGFunc) {}
    virtual ~PeepPattern() = default;
    virtual void Run(BB &bb, Insn &insn) = 0;
    /* optimization support function */
    bool IfOperandIsLiveAfterInsn(const RegOperand &regOpnd, Insn &insn);
    bool FindRegLiveOut(const RegOperand &regOpnd, const BB &bb);
    bool CheckOpndLiveinSuccs(const RegOperand &regOpnd, const BB &bb) const;
    bool CheckRegLiveinReturnBB(const RegOperand &regOpnd, const BB &bb) const;
    ReturnType IsOpndLiveinBB(const RegOperand &regOpnd, const BB &bb) const;
    int LogValueAtBase2(int64 val) const;
    bool IsMemOperandOptPattern(const Insn &insn, Insn &nextInsn);

protected:
    CGFunc &cgFunc;
};

class CGPeepPattern {
public:
    /* normal constructor */
    CGPeepPattern(CGFunc &f, BB &bb, Insn &insn) : cgFunc(&f), currBB(&bb), currInsn(&insn) {}
    virtual ~CGPeepPattern() = default;

    std::string PhaseName() const
    {
        return "cgpeephole";
    }

    virtual std::string GetPatternName() = 0;
    /* optimization support function */
    bool IfOperandIsLiveAfterInsn(const RegOperand &regOpnd, Insn &insn);
    bool FindRegLiveOut(const RegOperand &regOpnd, const BB &bb);
    bool CheckOpndLiveinSuccs(const RegOperand &regOpnd, const BB &bb) const;
    bool CheckRegLiveinReturnBB(const RegOperand &regOpnd, const BB &bb) const;
    ReturnType IsOpndLiveinBB(const RegOperand &regOpnd, const BB &bb) const;
    bool GetPatternRes() const
    {
        return optSuccess;
    }
    Insn *GetCurrInsn()
    {
        return currInsn;
    }
    void SetCurrInsn(Insn *updateInsn)
    {
        currInsn = updateInsn;
    }
    virtual void Run(BB &bb, Insn &insn) = 0;
    virtual bool CheckCondition(Insn &insn) = 0;

protected:
    CGFunc *cgFunc;
    BB *currBB;
    Insn *currInsn;
    // !!! If the pattern is optimized, set the $optSuccess to true and check before the subsequent patterns
    // of the same mop, otherwise, the subsequent patterns of the same mop will get the old wrong instruction.
    bool optSuccess = false;
};

class PeepHoleOptimizer {
public:
    explicit PeepHoleOptimizer(CGFunc *cf) : cgFunc(cf)
    {
        cg = cgFunc->GetCG();
    }
    ~PeepHoleOptimizer() = default;
    void Peephole0();
    void PeepholeOpt();

private:
    CGFunc *cgFunc;
    CG *cg = nullptr;
}; /* class PeepHoleOptimizer */

class PeepPatternMatch {
public:
    PeepPatternMatch(CGFunc &oneCGFunc, MemPool *memPool)
        : optOwnMemPool(memPool), peepAllocator(memPool), optimizations(peepAllocator.Adapter()), cgFunc(oneCGFunc)
    {
    }
    virtual ~PeepPatternMatch() = default;
    virtual void Run(BB &bb, Insn &insn) = 0;
    virtual void InitOpts() = 0;

protected:
    MemPool *optOwnMemPool;
    MapleAllocator peepAllocator;
    MapleVector<PeepPattern *> optimizations;
    CGFunc &cgFunc;
};

class PeepOptimizer {
public:
    PeepOptimizer(CGFunc &oneCGFunc, MemPool *memPool) : cgFunc(oneCGFunc), peepOptMemPool(memPool)
    {
        index = 0;
    }
    ~PeepOptimizer() = default;
    template <typename T>
    void Run();
    static int32 index;

private:
    CGFunc &cgFunc;
    MemPool *peepOptMemPool;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPostPeepHole, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPeepHole0, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPeepHole1, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_PEEP_H */
