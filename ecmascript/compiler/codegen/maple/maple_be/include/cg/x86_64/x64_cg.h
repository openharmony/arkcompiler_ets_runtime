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

/* sub Target info & implement */
#ifndef MAPLEBE_INCLUDE_CG_X86_64_CG_H
#define MAPLEBE_INCLUDE_CG_X86_64_CG_H

#include "cg.h"
#include "x64_isa.h"
#include "x64_live.h"
#include "x64_reaching.h"
#include "x64_MPISel.h"
#include "x64_standardize.h"
#include "x64_args.h"
#include "x64_local_opt.h"
#include "x64_cfgo.h"
#include "x64_peep.h"
#include "x64_proepilog.h"

namespace maplebe {
class X64CG : public CG {
public:
    X64CG(MIRModule &mod, const CGOptions &opts, const std::vector<std::string> &nameVec,
          const std::unordered_map<std::string, std::vector<std::string>> &patternMap)
        : CG(mod, opts),
        ehExclusiveNameVec(nameVec),
        cyclePatternMap(patternMap),
        keyPatternMap(allocator.Adapter()),
        symbolPatternMap(allocator.Adapter()) {}

    static const InsnDesc kMd[x64::kMopLast];
    void EnrollTargetPhases(MaplePhaseManager *pm) const override;
    /* Init SubTarget phase */
    /* AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f) const override;*/

    LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<X64LiveAnalysis>(f, mp);
    }
    ReachingDefinition *CreateReachingDefinition(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<X64ReachingDefinition>(f, mp);
    }
    CGPeepHole *CreateCGPeepHole(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<X64CGPeepHole>(f, &mp);
    }
    virtual GenProEpilog *CreateGenProEpilog(CGFunc &f, MemPool &mp, MemPool *tempMemPool = nullptr) const override
    {
        return mp.New<X64GenProEpilog>(f);
    }
    LocalOpt *CreateLocalOpt(MemPool &mp, CGFunc &f, ReachingDefinition &rd) const override
    {
        return mp.New<X64LocalOpt>(mp, f, rd);
    }
    MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<X64MoveRegArgs>(f);
    }

    MPISel *CreateMPIsel(MemPool &mp, MapleAllocator &allocator, CGFunc &f) const override
    {
        return mp.New<X64MPIsel>(mp, allocator, f);
    }

    Standardize *CreateStandardize(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<X64Standardize>(f);
    }

    CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<X64CFGOptimizer>(f, mp);
    }

    /* Init SubTarget optimization */

    Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) override;

    PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) override;

    CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool, StackMemPool &stackMp,
                         MapleAllocator &mallocator, uint32 funcId) override;

    bool IsExclusiveFunc(MIRFunction &mirFunc) override;

    /* NOTE: Consider making be_common a field of CG. */
    void GenerateObjectMaps(BECommon &beCommon) override;
	
    void DoNothing()
    {
        (void)ehExclusiveNameVec;
        (void)cyclePatternMap;
        (void)keyPatternMap;
        (void)symbolPatternMap;
    }

    /* Used for GCTIB pattern merging */
    std::string FindGCTIBPatternName(const std::string &name) const override;
    enum : uint8 { kR8LowList, kR8HighList, kR16List, kR32List, kR64List, kR128List };
    bool IsEffectiveCopy(Insn &insn) const final;
    bool IsTargetInsn(MOperator mOp) const final;
    bool IsClinitInsn(MOperator mOp) const final;
    bool IsPseudoInsn(MOperator mOp) const final;
    void DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const final;
    const InsnDesc &GetTargetMd(MOperator mOp) const final
    {
        return kMd[mOp];
    }
private:
	const std::vector<std::string> &ehExclusiveNameVec;
    const std::unordered_map<std::string, std::vector<std::string>> &cyclePatternMap;
    MapleUnorderedMap<GCTIBKey*, GCTIBPattern*, Hasher, EqualFn> keyPatternMap;
    MapleUnorderedMap<std::string, GCTIBPattern*> symbolPatternMap;
};
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_X86_64_CG_H */
