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
#include "aarch64_ssa.h"
#include "aarch64_phi_elimination.h"
#include "aarch64_prop.h"
#include "aarch64_dce.h"
#include "aarch64_live.h"
#include "aarch64_reaching.h"
#include "aarch64_args.h"
#include "aarch64_alignment.h"
#include "aarch64_validbit_opt.h"
#include "aarch64_reg_coalesce.h"
#include "aarch64_cfgo.h"

namespace maplebe {
constexpr int64 kShortBRDistance = (8 * 1024);
constexpr int64 kNegativeImmLowerLimit = -4096;
constexpr int32 kIntRegTypeNum = 5;
constexpr uint32 kAlignPseudoSize = 3;
constexpr uint32 kInsnSize = 4;
constexpr uint32 kAlignMovedFlag = 31;

/* Supporting classes for GCTIB merging */
class GCTIBKey {
public:
    GCTIBKey(MapleAllocator &allocator, uint32 rcHeader, std::vector<uint64> &patternWords)
        : header(rcHeader), bitMapWords(allocator.Adapter())
    {
        (void)bitMapWords.insert(bitMapWords.begin(), patternWords.begin(), patternWords.end());
    }

    ~GCTIBKey() = default;

    uint32 GetHeader() const
    {
        return header;
    }

    const MapleVector<uint64> &GetBitmapWords() const
    {
        return bitMapWords;
    }

private:
    uint32 header;
    MapleVector<uint64> bitMapWords;
};

class Hasher {
public:
    size_t operator()(const GCTIBKey *key) const
    {
        CHECK_NULL_FATAL(key);
        size_t hash = key->GetHeader();
        return hash;
    }
};

class EqualFn {
public:
    bool operator()(const GCTIBKey *firstKey, const GCTIBKey *secondKey) const
    {
        CHECK_NULL_FATAL(firstKey);
        CHECK_NULL_FATAL(secondKey);
        const MapleVector<uint64> &firstWords = firstKey->GetBitmapWords();
        const MapleVector<uint64> &secondWords = secondKey->GetBitmapWords();

        if ((firstKey->GetHeader() != secondKey->GetHeader()) || (firstWords.size() != secondWords.size())) {
            return false;
        }

        for (size_t i = 0; i < firstWords.size(); ++i) {
            if (firstWords[i] != secondWords[i]) {
                return false;
            }
        }
        return true;
    }
};

class GCTIBPattern {
public:
    GCTIBPattern(GCTIBKey &patternKey, MemPool &mp) : name(&mp)
    {
        key = &patternKey;
        id = GetId();
        name = GCTIB_PREFIX_STR + std::string("PTN_") + std::to_string(id);
    }

    ~GCTIBPattern() = default;

    int GetId() const
    {
        static int id = 0;
        return id++;
    }

    std::string GetName() const
    {
        DEBUG_ASSERT(!name.empty(), "null name check!");
        return std::string(name.c_str());
    }

    void SetName(const std::string &ptnName)
    {
        name = ptnName;
    }

private:
    int id;
    MapleString name;
    GCTIBKey *key;
};

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

    void GenerateObjectMaps(BECommon &beCommon) override;

    bool IsExclusiveFunc(MIRFunction &) override;

    void FindOrCreateRepresentiveSym(std::vector<uint64> &bitmapWords, uint32 rcHeader, const std::string &name);

    void CreateRefSymForGlobalPtn(GCTIBPattern &ptn) const;

    Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) override;

    PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) override;

    std::string FindGCTIBPatternName(const std::string &name) const override;

    LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64LiveAnalysis>(f, mp);
    }
    ReachingDefinition *CreateReachingDefinition(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64ReachingDefinition>(f, mp);
    }
    MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64MoveRegArgs>(f);
    }
    AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64AlignAnalysis>(f, mp);
    }
    CGSSAInfo *CreateCGSSAInfo(MemPool &mp, CGFunc &f, DomAnalysis &da, MemPool &tmp) const override
    {
        return mp.New<AArch64CGSSAInfo>(f, da, mp, tmp);
    }
    LiveIntervalAnalysis *CreateLLAnalysis(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64LiveIntervalAnalysis>(f, mp);
    };
    PhiEliminate *CreatePhiElimintor(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override
    {
        return mp.New<AArch64PhiEliminate>(f, ssaInfo, mp);
    }
    CGProp *CreateCGProp(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo, LiveIntervalAnalysis &ll) const override
    {
        return mp.New<AArch64Prop>(mp, f, ssaInfo, ll);
    }
    CGDce *CreateCGDce(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override
    {
        return mp.New<AArch64Dce>(mp, f, ssaInfo);
    }
    ValidBitOpt *CreateValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override
    {
        return mp.New<AArch64ValidBitOpt>(f, ssaInfo);
    }
    CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f) const override
    {
        return mp.New<AArch64CFGOptimizer>(f, mp);
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
