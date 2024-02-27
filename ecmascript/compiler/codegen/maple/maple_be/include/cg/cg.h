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

#ifndef MAPLEBE_INCLUDE_CG_CG_H
#define MAPLEBE_INCLUDE_CG_CG_H

/* C++ headers. */
#include <cstddef>
#include <string>
/* MapleIR headers. */
#include "operand.h"
#include "insn.h"
#include "cgfunc.h"
#include "live.h"
#include "cg_option.h"
#include "opcode_info.h"
#include "global_tables.h"
#include "mir_function.h"
#include "mad.h"
#include "target_machine.h"
#include "proepilog.h"

namespace maplebe {
#define ADDTARGETPHASE(PhaseName, condition)  \
    if (!CGOptions::IsSkipPhase(PhaseName)) { \
        pm->AddPhase(PhaseName, condition);   \
    }
/* subtarget opt phase -- cyclic Dependency, use Forward declaring */
class CGSSAInfo;
class PhiEliminate;
class DomAnalysis;
class CGProp;
class CGDce;
class AlignAnalysis;
class MoveRegArgs;
class MPISel;
class Standardize;
class LiveIntervalAnalysis;
class ValidBitOpt;
class CG;
class LocalOpt;
class CFGOptimizer;
class LocalSchedule;
class ControlDepAnalysis;
class DataDepAnalysis;
class CGPeepHole;
class GenProEpilog;
class LoopAnalysis;

class Globals {
public:
    static Globals *GetInstance()
    {
        static Globals instance;
        return &instance;
    }

    ~Globals() = default;

    void SetBECommon(BECommon &bc)
    {
        beCommon = &bc;
    }

    BECommon *GetBECommon()
    {
        return beCommon;
    }

    const BECommon *GetBECommon() const
    {
        return beCommon;
    }

    void SetMAD(MAD &m)
    {
        mad = &m;
    }

    MAD *GetMAD()
    {
        return mad;
    }

    void ClearMAD()
    {
        mad = nullptr;
    }

    const MAD *GetMAD() const
    {
        return mad;
    }

    void SetOptimLevel(int32 opLevel)
    {
        optimLevel = opLevel;
    }

    int32 GetOptimLevel() const
    {
        return optimLevel;
    }

    void SetTarget(CG &target);
    const CG *GetTarget() const;

private:
    BECommon *beCommon = nullptr;
    MAD *mad = nullptr;
    int32 optimLevel = 0;
    CG *cg = nullptr;
    Globals() = default;
};

class GCTIBKey {
public:
    GCTIBKey(MapleAllocator &allocator, uint32 rcHeader, const std::vector<uint64> &patternWords)
        : header(rcHeader),
          bitMapWords(allocator.Adapter())
    {
        (void)bitMapWords.insert(bitMapWords.cbegin(), patternWords.cbegin(), patternWords.cend());
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
    GCTIBPattern(GCTIBKey &patternKey, MemPool &mp)
        : name(&mp)
    {
        key = &patternKey;
        id = GetId();
        name = GCTIB_PREFIX_STR + std::string("PTN_") + std::to_string(id);
    }

    ~GCTIBPattern() = default;

    int GetId() const
    {
        static int createNum = 0;
        return createNum++;
    }

    std::string GetName() const
    {
        return std::string(name.c_str());
    }

    void SetName(const std::string &ptnName)
    {
        name = ptnName;
    }

private:
    int id = 0;
    MapleString name;
    GCTIBKey *key = nullptr;
};

class CG {
public:
    using GenerateFlag = uint64;

public:
    CG(MIRModule &mod, const CGOptions &cgOptions)
        : memPool(memPoolCtrler.NewMemPool("maplecg mempool", false /* isLocalPool */)),
          allocator(memPool),
          mirModule(&mod),
          emitter(nullptr),
          labelOrderCnt(0),
          cgOption(cgOptions),
          instrumentationFunction(nullptr),
          fileGP(nullptr)
    {
        const std::string &internalNameLiteral = namemangler::GetInternalNameLiteral(namemangler::kJavaLangObjectStr);
        GStrIdx strIdxFromName = GlobalTables::GetStrTable().GetStrIdxFromName(internalNameLiteral);
        isLibcore = (GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdxFromName) != nullptr);
        DefineDebugTraceFunctions();
        isLmbc = (mirModule->GetFlavor() == MIRFlavor::kFlavorLmbc);
    }

    virtual ~CG();

    /* enroll all code generator phases for target machine */
    virtual void EnrollTargetPhases(MaplePhaseManager *pm) const = 0;

    void GenExtraTypeMetadata(const std::string &classListFileName, const std::string &outputBaseName);
    void GenPrimordialObjectList(const std::string &outputBaseName);
    const std::string ExtractFuncName(const std::string &str);

    virtual Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) = 0;
    virtual PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) = 0;

    virtual CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction &, BECommon &, MemPool &, StackMemPool &, MapleAllocator &,
                                 uint32) = 0;

    bool IsExclusiveEH() const
    {
        return CGOptions::IsExclusiveEH();
    }

    virtual bool IsExclusiveFunc(MIRFunction &mirFunc) = 0;

    /* NOTE: Consider making be_common a field of CG. */
    virtual void GenerateObjectMaps(BECommon &beCommon) = 0;

    /* Used for GCTIB pattern merging */
    virtual std::string FindGCTIBPatternName(const std::string &name) const = 0;

    bool GenerateVerboseAsm() const
    {
        return cgOption.GenerateVerboseAsm();
    }

    bool GenerateVerboseCG() const
    {
        return cgOption.GenerateVerboseCG();
    }

    bool DoPrologueEpilogue() const
    {
        return cgOption.DoPrologueEpilogue();
    }

    bool DoTailCall() const
    {
        return cgOption.DoTailCall();
    }

    bool DoCheckSOE() const
    {
        return cgOption.DoCheckSOE();
    }

    bool GenerateDebugFriendlyCode() const
    {
        return cgOption.GenerateDebugFriendlyCode();
    }

    int32 GetOptimizeLevel() const
    {
        return cgOption.GetOptimizeLevel();
    }

    bool UseFastUnwind() const
    {
        return true;
    }

    bool IsStackProtectorStrong() const
    {
        return cgOption.IsStackProtectorStrong();
    }

    bool IsStackProtectorAll() const
    {
        return cgOption.IsStackProtectorAll();
    }

    bool NeedInsertInstrumentationFunction() const
    {
        return cgOption.NeedInsertInstrumentationFunction();
    }

    void SetInstrumentationFunction(const std::string &name);
    const MIRSymbol *GetInstrumentationFunction() const
    {
        return instrumentationFunction;
    }

    bool InstrumentWithDebugTraceCall() const
    {
        return cgOption.InstrumentWithDebugTraceCall();
    }

    bool InstrumentWithProfile() const
    {
        return cgOption.InstrumentWithProfile();
    }

    bool DoPatchLongBranch() const
    {
        return cgOption.DoPatchLongBranch();
    }

    uint8 GetRematLevel() const
    {
        return CGOptions::GetRematLevel();
    }

    bool GenYieldPoint() const
    {
        return cgOption.GenYieldPoint();
    }

    bool GenLocalRC() const
    {
        return cgOption.GenLocalRC();
    }

    bool GenerateExceptionHandlingCode() const
    {
        return cgOption.GenerateExceptionHandlingCode();
    }

    bool DoConstFold() const
    {
        return cgOption.DoConstFold();
    }

    void AddStackGuardvar();
    void DefineDebugTraceFunctions();
    MIRModule *GetMIRModule()
    {
        return mirModule;
    }

    void SetEmitter(Emitter &emitter)
    {
        this->emitter = &emitter;
    }

    Emitter *GetEmitter() const
    {
        return emitter;
    }

    MIRModule *GetMIRModule() const
    {
        return mirModule;
    }

    void SetTargetMachine(TargetMachine &targetMachine)
    {
        this->targetMachine = &targetMachine;
    }
	
    TargetMachine *GetTargetMachine() const
    {
        return targetMachine;
    }
	
    void IncreaseLabelOrderCnt()
    {
        labelOrderCnt++;
    }

    LabelIDOrder GetLabelOrderCnt() const
    {
        return labelOrderCnt;
    }

    const CGOptions &GetCGOptions() const
    {
        return cgOption;
    }

    void UpdateCGOptions(const CGOptions &newOption)
    {
        cgOption.SetOptionFlag(newOption.GetOptionFlag());
    }

    bool IsLibcore() const
    {
        return isLibcore;
    }

    bool IsLmbc() const
    {
        return isLmbc;
    }

    MIRSymbol *GetDebugTraceEnterFunction()
    {
        return dbgTraceEnter;
    }

    const MIRSymbol *GetDebugTraceEnterFunction() const
    {
        return dbgTraceEnter;
    }

    MIRSymbol *GetProfileFunction()
    {
        return dbgFuncProfile;
    }

    const MIRSymbol *GetProfileFunction() const
    {
        return dbgFuncProfile;
    }

    const MIRSymbol *GetDebugTraceExitFunction() const
    {
        return dbgTraceExit;
    }

    /* Init SubTarget phase */
    virtual LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const
    {
        return nullptr;
    };
    virtual ReachingDefinition *CreateReachingDefinition(MemPool &mp, CGFunc &f) const
    {
        return nullptr;
    };
    virtual GenProEpilog *CreateGenProEpilog(CGFunc &func, MemPool &mp, MemPool *tempMemPool = nullptr) const = 0;
    virtual CGPeepHole *CreateCGPeepHole(MemPool &mp, CGFunc &f) const = 0;
    virtual MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const
    {
        return nullptr;
    };
    virtual AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f, LoopAnalysis &loop) const
    {
        return nullptr;
    };
    virtual MPISel *CreateMPIsel(MemPool &mp, MapleAllocator &allocator, CGFunc &f) const
    {
        return nullptr;
    }
    virtual Standardize *CreateStandardize(MemPool &mp, CGFunc &f) const
    {
        return nullptr;
    }
    virtual ValidBitOpt *CreateValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const
    {
        return nullptr;
    }

    /* Init SubTarget optimization */
    virtual CGSSAInfo *CreateCGSSAInfo(MemPool &mp, CGFunc &f, DomAnalysis &da, MemPool &tmp) const
    {
        return nullptr;
    };
    virtual LiveIntervalAnalysis *CreateLLAnalysis(MemPool &mp, CGFunc &f) const
    {
        return nullptr;
    };
    virtual PhiEliminate *CreatePhiElimintor(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const
    {
        return nullptr;
    };
    virtual CGProp *CreateCGProp(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo, LiveIntervalAnalysis &ll) const
    {
        return nullptr;
    };
    virtual CGDce *CreateCGDce(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const
    {
        return nullptr;
    };
    virtual LocalSchedule *CreateLocalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cda,
                                               DataDepAnalysis &dda) const
    {
        return nullptr;
    }
    virtual LocalOpt *CreateLocalOpt(MemPool &mp, CGFunc &f, ReachingDefinition &) const
    {
        return nullptr;
    };
    virtual CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f, LoopAnalysis &loop) const
    {
        return nullptr;
    }

    /* Object map generation helper */
    std::vector<int64> GetReferenceOffsets64(const BECommon &beCommon, MIRStructType &structType);

    void SetGP(MIRSymbol *sym)
    {
        fileGP = sym;
    }
    MIRSymbol *GetGP() const
    {
        return fileGP;
    }

    static bool IsInFuncWrapLabels(MIRFunction *func)
    {
        return funcWrapLabels.find(func) != funcWrapLabels.end();
    }

    static void SetFuncWrapLabels(MIRFunction *func, const std::pair<LabelIdx, LabelIdx> labels)
    {
        if (!IsInFuncWrapLabels(func)) {
            funcWrapLabels[func] = labels;
        }
    }

    static std::map<MIRFunction *, std::pair<LabelIdx, LabelIdx>> &GetFuncWrapLabels()
    {
        return funcWrapLabels;
    }
    static void SetCurCGFunc(CGFunc &cgFunc)
    {
        currentCGFunction = &cgFunc;
    }

    static const CGFunc *GetCurCGFunc()
    {
        return currentCGFunction;
    }

    static CGFunc *GetCurCGFuncNoConst()
    {
        return currentCGFunction;
    }

    virtual const InsnDesc &GetTargetMd(MOperator mOp) const = 0;
    virtual bool IsEffectiveCopy(Insn &insn) const = 0;
    virtual bool IsTargetInsn(MOperator mOp) const = 0;
    virtual bool IsClinitInsn(MOperator mOp) const = 0;
    virtual bool IsPseudoInsn(MOperator mOp) const = 0;
    virtual void DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const = 0;

protected:
    MemPool *memPool;
    MapleAllocator allocator;

private:
    MIRModule *mirModule;
    Emitter *emitter;
    TargetMachine *targetMachine = nullptr;
    LabelIDOrder labelOrderCnt;
    static CGFunc *currentCGFunction; /* current cg function being compiled */
    CGOptions cgOption;
    MIRSymbol *instrumentationFunction;
    MIRSymbol *dbgTraceEnter = nullptr;
    MIRSymbol *dbgTraceExit = nullptr;
    MIRSymbol *dbgFuncProfile = nullptr;
    MIRSymbol *fileGP; /* for lmbc, one local %GP per file */
    static std::map<MIRFunction *, std::pair<LabelIdx, LabelIdx>> funcWrapLabels;
    bool isLibcore;
    bool isLmbc;
}; /* class CG */
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_CG_H */
