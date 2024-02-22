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

#ifndef MAPLEBE_INCLUDE_CG_ALIGNMENT_H
#define MAPLEBE_INCLUDE_CG_ALIGNMENT_H

#include "cg_phase.h"
#include "maple_phase.h"
#include "cgbb.h"
#include "loop.h"

namespace maplebe {
class AlignAnalysis {
public:
    AlignAnalysis(CGFunc &func, MemPool &memP, LoopAnalysis &loop)
        : cgFunc(&func),
          alignAllocator(&memP),
          loopInfo(loop),
          loopHeaderBBs(alignAllocator.Adapter()),
          jumpTargetBBs(alignAllocator.Adapter()),
          alignInfos(alignAllocator.Adapter()),
          sameTargetBranches(alignAllocator.Adapter())
    {
    }

    virtual ~AlignAnalysis() = default;

    void AnalysisAlignment();
    void Dump();
    virtual void FindLoopHeader() = 0;
    virtual void FindJumpTarget() = 0;
    virtual void ComputeLoopAlign() = 0;
    virtual void ComputeJumpAlign() = 0;
    virtual void ComputeCondBranchAlign() = 0;

    /* filter condition */
    virtual bool IsIncludeCall(BB &bb) = 0;
    virtual bool IsInSizeRange(BB &bb) = 0;
    virtual bool HasFallthruEdge(BB &bb) = 0;

    std::string PhaseName() const
    {
        return "alignanalysis";
    }
    const MapleUnorderedSet<BB *> &GetLoopHeaderBBs() const
    {
        return loopHeaderBBs;
    }
    const MapleUnorderedSet<BB *> &GetJumpTargetBBs() const
    {
        return jumpTargetBBs;
    }
    const MapleUnorderedMap<BB *, uint32> &GetAlignInfos() const
    {
        return alignInfos;
    }
    uint32 GetAlignPower(BB &bb)
    {
        return alignInfos[&bb];
    }

    void InsertLoopHeaderBBs(BB &bb)
    {
        loopHeaderBBs.insert(&bb);
    }
    void InsertJumpTargetBBs(BB &bb)
    {
        jumpTargetBBs.insert(&bb);
    }
    void InsertAlignInfos(BB &bb, uint32 power)
    {
        alignInfos[&bb] = power;
    }

protected:
    CGFunc *cgFunc;
    MapleAllocator alignAllocator;
    LoopAnalysis &loopInfo;
    MapleUnorderedSet<BB *> loopHeaderBBs;
    MapleUnorderedSet<BB *> jumpTargetBBs;
    MapleUnorderedMap<BB *, uint32> alignInfos;
    MapleUnorderedMap<LabelIdx, uint32> sameTargetBranches;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgAlignAnalysis, maplebe::CGFunc)
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ALIGNMENT_H */
