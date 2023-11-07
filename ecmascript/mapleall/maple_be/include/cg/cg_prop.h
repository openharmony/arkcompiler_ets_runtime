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

#ifndef MAPLEBE_INCLUDE_CG_PROP_H
#define MAPLEBE_INCLUDE_CG_PROP_H

#include "cgfunc.h"
#include "cg_ssa.h"
#include "cg_dce.h"
#include "cg.h"
#include "reg_coalesce.h"

namespace maplebe {
class CGProp {
public:
    CGProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, LiveIntervalAnalysis &ll)
        : memPool(&mp), cgFunc(&f), propAlloc(&mp), ssaInfo(&sInfo), regll(&ll)
    {
        cgDce = f.GetCG()->CreateCGDce(mp, f, sInfo);
    }
    virtual ~CGProp() = default;

    void DoCopyProp();
    void DoTargetProp();

protected:
    MemPool *memPool;
    CGFunc *cgFunc;
    MapleAllocator propAlloc;
    CGSSAInfo *GetSSAInfo()
    {
        return ssaInfo;
    }
    CGDce *GetDce()
    {
        return cgDce;
    }
    LiveIntervalAnalysis *GetRegll()
    {
        return regll;
    }

private:
    virtual void CopyProp() = 0;
    virtual void TargetProp(Insn &insn) = 0;
    virtual void PropPatternOpt() = 0;
    CGSSAInfo *ssaInfo;
    CGDce *cgDce = nullptr;
    LiveIntervalAnalysis *regll;
};

class PropOptimizeManager {
public:
    ~PropOptimizeManager() = default;
    template <typename PropOptimizePattern>
    void Optimize(CGFunc &cgFunc, CGSSAInfo *cgssaInfo, LiveIntervalAnalysis *ll) const
    {
        PropOptimizePattern optPattern(cgFunc, cgssaInfo, ll);
        optPattern.Run();
    }
    template <typename PropOptimizePattern>
    void Optimize(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) const
    {
        PropOptimizePattern optPattern(cgFunc, cgssaInfo);
        optPattern.Run();
    }
};

class PropOptimizePattern {
public:
    PropOptimizePattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo, LiveIntervalAnalysis *ll)
        : cgFunc(cgFunc), optSsaInfo(cgssaInfo), regll(ll)
    {
    }

    PropOptimizePattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : cgFunc(cgFunc), optSsaInfo(cgssaInfo) {}
    virtual ~PropOptimizePattern() = default;
    virtual bool CheckCondition(Insn &insn) = 0;
    virtual void Optimize(Insn &insn) = 0;
    virtual void Run() = 0;

protected:
    std::string PhaseName() const
    {
        return "propopt";
    }
    virtual void Init() = 0;
    Insn *FindDefInsn(const VRegVersion *useVersion);

    CGFunc &cgFunc;
    CGSSAInfo *optSsaInfo = nullptr;
    LiveIntervalAnalysis *regll = nullptr;
};

class ReplaceRegOpndVisitor : public OperandVisitorBase,
                              public OperandVisitors<RegOperand, ListOperand, MemOperand>,
                              public OperandVisitor<PhiOperand> {
public:
    ReplaceRegOpndVisitor(CGFunc &f, Insn &cInsn, uint32 cIdx, RegOperand &oldR, RegOperand &newR)
        : cgFunc(&f), insn(&cInsn), idx(cIdx), oldReg(&oldR), newReg(&newR)
    {
    }
    virtual ~ReplaceRegOpndVisitor() = default;

protected:
    CGFunc *cgFunc;
    Insn *insn;
    uint32 idx;
    RegOperand *oldReg;
    RegOperand *newReg;
};

MAPLE_FUNC_PHASE_DECLARE(CgCopyProp, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE(CgTargetProp, maplebe::CGFunc)
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_PROP_H */
