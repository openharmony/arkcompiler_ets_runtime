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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CG_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CG_H

#include "cg.h"
#include "aarch64_cgfunc.h"
#include "aarch64_live.h"
#include "aarch64_args.h"
#include "aarch64_cfgo.h"
#include "aarch64_peep.h"
#include "aarch64_proepilog.h"

namespace maplebe {
constexpr int64 kShortBRDistance = (8 * 1024);
constexpr int64 kNegativeImmLowerLimit = -4096;
constexpr int32 kIntRegTypeNum = 5;
constexpr uint32 kAlignPseudoSize = 3;
constexpr uint32 kInsnSize = 4;
constexpr uint32 kAlignMovedFlag = 31;

/* sub Target info & implement */
class AArch64CG : public CG {
public:
    AArch64CG(MIRModule &mod, const CGOptions &opts, const std::vector<std::string> &nameVec,
              const std::unordered_map<std::string, std::vector<std::string>> &patternMap)
        : CG(mod, opts),
          ehExclusiveNameVec(nameVec),
          cyclePatternMap(patternMap),
          keyPatternMap(allocator.Adapter()),
          symbolPatternMap(allocator.Adapter())
    {
    }

    ~AArch64CG() override = default;

    CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool, StackMemPool &stackMp,
                         MapleAllocator &mallocator, uint32 funcId) override
    {
        return memPool.New<AArch64CGFunc>(mod, *this, mirFunc, bec, memPool, stackMp, mallocator, funcId);
    }

    void EnrollTargetPhases(MaplePhaseManager *pm) const override;

    const std::unordered_map<std::string, std::vector<std::string>> &GetCyclePatternMap() const
    {
        return cyclePatternMap;
    }

    bool IsExclusiveFunc(MIRFunction &) override;

    void EmitGCTIBLabel(GCTIBKey *key, const std::string &gcTIBName, std::vector<uint64> &bitmapWords, uint32 rcHeader);

    void CreateRefSymForGlobalPtn(GCTIBPattern &ptn) const;

    PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) override;

#ifdef ARK_LITECG_DEBUG
    std::string FindGCTIBPatternName(const std::string &name) const override;
#endif

    LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64LiveAnalysis>(f, mp);
    }
    GenProEpilog *CreateGenProEpilog(CGFunc &f, MemPool &mp, MemPool *tempMemPool = nullptr) const override
    {
        return mp.New<AArch64GenProEpilog>(f, *tempMemPool);
    }
    CGPeepHole *CreateCGPeepHole(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64CGPeepHole>(f, &mp);
    }
    MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64MoveRegArgs>(f);
    }
    CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f, LoopAnalysis &loop) const override
    {
        return mp.New<AArch64CFGOptimizer>(f, mp, loop);
    }

    /* Return the copy operand id of reg1 if it is an insn who just do copy from reg1 to reg2.
     * i. mov reg2, reg1
     * ii. add/sub reg2, reg1, 0/zero register
     * iii. mul reg2, reg1, 1
     */
    bool IsEffectiveCopy(Insn &insn) const final;
    bool IsTargetInsn(MOperator mOp) const final;
    bool IsClinitInsn(MOperator mOp) const final;
    bool IsPseudoInsn(MOperator mOp) const final;
    void DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const final;
    const InsnDesc &GetTargetMd(MOperator mOp) const final
    {
        return kMd[mOp];
    }

    static const InsnDesc kMd[kMopLast];
    enum : uint8 { kR8List, kR16List, kR32List, kR64List, kV64List };
    static std::array<std::array<const std::string, kAllRegNum>, kIntRegTypeNum> intRegNames;
    static std::array<const std::string, kAllRegNum> vectorRegNames;

private:
    const std::vector<std::string> &ehExclusiveNameVec;
    const std::unordered_map<std::string, std::vector<std::string>> &cyclePatternMap;
    MapleUnorderedMap<GCTIBKey *, GCTIBPattern *, Hasher, EqualFn> keyPatternMap;
    MapleUnorderedMap<std::string, GCTIBPattern *> symbolPatternMap;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CG_H */
