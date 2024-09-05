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

#ifndef MAPLEBE_INCLUDE_CG_CG_OPTION_H
#define MAPLEBE_INCLUDE_CG_CG_OPTION_H
#include <vector>
#include <sys/stat.h>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "types_def.h"

namespace maplebe {
using namespace maple;
struct Range {
    bool enable;
    uint64 begin;
    uint64 end;
};

typedef uint8 *(*MemoryManagerAllocateDataSectionCallback)(void *codeSpace, uint32 size, uint32 alignment,
                                                           const std::string &sectionName);

typedef void (*MemoryManagerSaveFunc2AddressInfoCallback)(void *codeSpace, std::string funcName, uint32_t address);

typedef void (*MemoryManagerSaveFunc2FPtoPrevSPDeltaCallback)(void *object, std::string funcName,
                                                              int32_t fp2PrevSpDelta);

typedef void (*MemoryManagerSaveFunc2CalleeOffsetInfoCallback)(void *object, std::string funcName,
                                                               std::vector<std::pair<uint16_t, int32_t>> calleeRegInfo);

typedef void (*MemoryManagerSavePC2DeoptInfoCallback)(void *object, uint64_t pc, std::vector<uint8_t> deoptInfo);

typedef void (*MemoryManagerSavePC2CallSiteInfoCallback)(void *object, uint64_t pc, std::vector<uint8_t> callSiteInfo);

class CGOptions {
public:
    enum OptionEnum : uint64 {
        kUndefined = 0ULL,
        kDoCg = 1ULL << 0,
        kDoLinearScanRegAlloc = 1ULL << 1,
        kVerboseAsm = 1ULL << 6,
        kGenInsertCall = 1ULL << 7,
        kGenYieldPoint = 1ULL << 9,
        kGenLocalRc = 1ULL << 10,
        kVerboseCG = 1ULL << 12,
        kDebugFriendly = 1ULL << 20,
        kWithLoc = 1ULL << 21,
        kWithDwarf = 1ULL << 22,
        kWithMpl = 1ULL << 23,
        kWithSrc = 1ULL << 24,
        kWithAsm = 1ULL << 25,
        kTailCallOpt = 1ULL << 36,
        /* undocumented */
        kDumpCFG = 1ULL << 61,
        kDumpCgir = 1ULL << 62,
        kSuppressFileInfo = 1ULL << 63,
    };

    using OptionFlag = uint64;

    enum GenerateEnum : uint64 {
        kGrootList = 1ULL << 2,
        kPrimorList = 1ULL << 3,
    };

    using GenerateFlag = uint64;

    enum OptimizeLevel : uint8 {
        kLevel0 = 0,
        kLevelLiteCG = 1,
        kLevel1 = 2,
        kLevel2 = 3,
    };
    struct EmitMemoryManager {
        void *codeSpace;
        MemoryManagerAllocateDataSectionCallback allocateDataSection;
        MemoryManagerSaveFunc2AddressInfoCallback funcAddressSaver;
        MemoryManagerSaveFunc2FPtoPrevSPDeltaCallback funcFpSPDeltaSaver;
        MemoryManagerSaveFunc2CalleeOffsetInfoCallback funcCalleeOffsetSaver;
        MemoryManagerSavePC2DeoptInfoCallback pc2DeoptInfoSaver;
        MemoryManagerSavePC2CallSiteInfoCallback pc2CallSiteInfoSaver;
    };
    /*
     * The default CG option values are:
     * Don't BE_QUITE; verbose,
     * DO CG and generate .s file as output,
     * Generate EH,
     * Use frame pointer,
     * Generate CFI directives,
     * DO peephole optimization,
     * Generate position-independent executable,
     * Don't insert debug comments in .s file,
     * Don't insert a call to the named (instrumentation)
     * function at each function entry.
     */
    static const OptionFlag kDefaultOptions = OptionFlag(
#if TARGAARCH64 || TARGARM32 || TARGRISCV64
        kDoCg
#else
        kDoCg
#endif
    );

    /*
     * The default metadata generation flags values are:
     * Generate .macros.def for C preprocessors.
     * Generate .groots.txt for GC.
     * Generate .primordials.txt for GC.
     * Generate yieldpoints for GC.
     * Do not generate separate GCTIB file.
     */
    static const GenerateFlag kDefaultGflags = GenerateFlag(0);

public:
    static CGOptions &GetInstance();
    virtual ~CGOptions() = default;
    bool SolveOptions();
    std::ostream& GetLogStream() const;
    void DumpOptions();
    std::vector<std::string> &GetSequence()
    {
        return phaseSequence;
    }

    const EmitMemoryManager &GetEmitMemoryManager() const
    {
        return emitMemoryManager;
    }

    void SetupEmitMemoryManager(void *codeSpace, MemoryManagerAllocateDataSectionCallback allocateDataSection,
                                MemoryManagerSaveFunc2AddressInfoCallback funcAddressSaver,
                                MemoryManagerSaveFunc2FPtoPrevSPDeltaCallback funcFPSPDeltaSaver,
                                MemoryManagerSaveFunc2CalleeOffsetInfoCallback funcCalleeOffsetSaver,
                                MemoryManagerSavePC2DeoptInfoCallback pc2DeoptInfoSaver,
                                MemoryManagerSavePC2CallSiteInfoCallback pc2CallSiteInfoSaver)
    {
        emitMemoryManager.codeSpace = codeSpace;
        emitMemoryManager.allocateDataSection = allocateDataSection;
        emitMemoryManager.funcAddressSaver = funcAddressSaver;
        emitMemoryManager.funcFpSPDeltaSaver = funcFPSPDeltaSaver;
        emitMemoryManager.funcCalleeOffsetSaver = funcCalleeOffsetSaver;
        emitMemoryManager.pc2DeoptInfoSaver = pc2DeoptInfoSaver;
        emitMemoryManager.pc2CallSiteInfoSaver = pc2CallSiteInfoSaver;
    }

    template <class T>
    void SetOrClear(T &dest, uint64 flag, bool truth) const
    {
        if (truth) {
            dest |= flag;
        } else {
            dest &= ~flag;
        }
    }

#ifdef ARK_LITECG_DEBUG
    void EnableO0();
    void EnableO1();
    void EnableO2();
#endif
    void EnableLiteCG();

    bool GenGrootList() const
    {
        return generateFlag & kGrootList;
    }

    bool GenPrimorList() const
    {
        return generateFlag & kPrimorList;
    }

    bool GenYieldPoint() const
    {
        return generateFlag & kGenYieldPoint;
    }

    bool GenLocalRC() const
    {
        return (generateFlag & kGenLocalRc) && !gcOnly;
    }

    bool DoEmitCode() const
    {
        return (options & kDoCg) != 0;
    }

    bool GenerateExceptionHandlingCode() const
    {
        return true;
    }

    bool DoLinearScanRegisterAllocation() const
    {
        return (options & kDoLinearScanRegAlloc) != 0;
    }

    bool GenerateVerboseAsm() const
    {
        return (options & kVerboseAsm) != 0;
    }

    bool GenerateVerboseCG() const
    {
        return (options & kVerboseCG) != 0;
    }

    bool GenerateDebugFriendlyCode() const
    {
        return true;
    }

    bool WithLoc() const
    {
        return (options & kWithLoc) != 0;
    }

    bool WithSrc() const
    {
        return (options & kWithSrc) != 0;
    }

    bool WithMpl() const
    {
        return (options & kWithMpl) != 0;
    }

    bool WithAsm() const
    {
        return (options & kWithAsm) != 0;
    }

    bool DoTailCall() const
    {
        return (options & kTailCallOpt) != 0;
    }

    bool SuppressFileInfo() const
    {
        return (options & kSuppressFileInfo) != 0;
    }

    bool DoDumpCFG() const
    {
        return (options & kDumpCFG) != 0;
    }

    void SetDefaultOptions(const MIRModule &mod);
    static bool DumpPhase(const std::string &phase);
    static bool FuncFilter(const std::string &name);
    void SplitPhases(const std::string &str, std::unordered_set<std::string> &set);
#ifdef ARK_LITECG_DEBUG
    void SetRange(const std::string &str, const std::string &cmd, Range &subRange);
    void SetTargetMachine(const std::string &str);
#endif

    int32 GetOptimizeLevel() const
    {
        return optimizeLevel;
    }

    bool IsRunCG() const
    {
        return runCGFlag;
    }

    void SetRunCGFlag(bool cgFlag)
    {
        runCGFlag = cgFlag;
    }

    bool IsAsmEmitterEnable() const
    {
        return asmEmitterEnable;
    }

    void SetAsmEmitterEnable(bool flag)
    {
        asmEmitterEnable = flag;
    }

    void SetParserOption(uint32 option)
    {
        parserOption |= option;
    }

    uint32 GetParserOption() const
    {
        return parserOption;
    }

    GenerateFlag &GetGenerateFlags()
    {
        return generateFlag;
    }

    const GenerateFlag &GetGenerateFlags() const
    {
        return generateFlag;
    }

    void SetGenerateFlags(GenerateFlag flag)
    {
        generateFlag |= flag;
    }

    void SetOption(OptionFlag opFlag)
    {
        options |= opFlag;
    }

    void ClearOption(OptionFlag opFlag)
    {
        options &= ~opFlag;
    }

    void SetEHExclusiveFile(const std::string &ehExclusive)
    {
        ehExclusiveFile = ehExclusive;
    }

    static std::unordered_set<std::string> &GetDumpPhases()
    {
        return dumpPhases;
    }

    static std::unordered_set<std::string> &GetSkipPhases()
    {
        return skipPhases;
    }

    static bool IsSkipPhase(const std::string &phaseName)
    {
        return !(skipPhases.find(phaseName) == skipPhases.end());
    }

    const std::vector<std::string> &GetEHExclusiveFunctionNameVec() const
    {
        return ehExclusiveFunctionName;
    }

    static const std::unordered_map<std::string, std::vector<std::string>> &GetCyclePatternMap()
    {
        return cyclePatternMap;
    }

    static bool IsSkipFromPhase(const std::string &phaseName)
    {
        return skipFrom.compare(phaseName) == 0;
    }

    static const std::string GetSkipFromPhase()
    {
        return skipFrom;
    }

    static void SetSkipFrom(const std::string &phaseName)
    {
        skipFrom = phaseName;
    }

    static bool IsSkipAfterPhase(const std::string &phaseName)
    {
        return skipAfter.compare(phaseName) == 0;
    }

    static const std::string GetSkipAfterPhase()
    {
        return skipAfter;
    }

    static void SetSkipAfter(const std::string &phaseName)
    {
        skipAfter = phaseName;
    }

    static const std::string &GetDumpFunc()
    {
        return dumpFunc;
    }

    static bool IsDumpFunc(const std::string &func)
    {
        return ((dumpFunc.compare("*") == 0) || (func.find(CGOptions::dumpFunc.c_str()) != std::string::npos));
    }

    static void SetDumpFunc(const std::string &func)
    {
        dumpFunc = func;
    }
    static size_t FindIndexInProfileData(char data)
    {
        return profileData.find(data);
    }

    static void SetProfileData(const std::string &path)
    {
        profileData = path;
    }

    static std::string &GetProfileData()
    {
        return profileData;
    }

    static const std::string GetProfileDataSubStr(size_t begin, size_t end)
    {
        return profileData.substr(begin, end);
    }

    static const std::string GetProfileDataSubStr(size_t position)
    {
        return profileData.substr(position);
    }

    static bool IsProfileDataEmpty()
    {
        return profileData.empty();
    }

    static const std::string &GetProfileFuncData()
    {
        return profileFuncData;
    }

    static bool IsProfileFuncDataEmpty()
    {
        return profileFuncData.empty();
    }

    static void SetProfileFuncData(const std::string &data)
    {
        profileFuncData = data;
    }

    static const std::string &GetProfileClassData()
    {
        return profileClassData;
    }

    static void SetProfileClassData(const std::string &data)
    {
        profileClassData = data;
    }

    static const std::string &GetDuplicateAsmFile()
    {
        return duplicateAsmFile;
    }

    static bool IsDuplicateAsmFileEmpty()
    {
        if (duplicateAsmFile.empty()) {
            return true;
        }
        struct stat buffer;
        if (stat(duplicateAsmFile.c_str(), &buffer) != 0) {
            return true;
        }
        return false;
    }

    static bool UseRange()
    {
        return range.enable;
    }

    static Range &GetRange()
    {
        return range;
    }

    static uint64 GetRangeBegin()
    {
        return range.begin;
    }

    static uint64 GetRangeEnd()
    {
        return range.end;
    }

    static Range &GetSpillRanges()
    {
        return spillRanges;
    }

    static uint64 GetSpillRangesBegin()
    {
        return spillRanges.begin;
    }

    static uint64 GetSpillRangesEnd()
    {
        return spillRanges.end;
    }

    static uint64 GetLSRABBOptSize()
    {
        return lsraBBOptSize;
    }

    static void SetLSRABBOptSize(uint64 size)
    {
        lsraBBOptSize = size;
    }

    static void SetLSRAInsnOptSize(uint64 size)
    {
        lsraInsnOptSize = size;
    }

    static uint64 GetOverlapNum()
    {
        return overlapNum;
    }

    static void SetOverlapNum(uint64 num)
    {
        overlapNum = num;
    }

    static bool OptimizeForSize()
    {
        return optForSize;
    }

    static bool IsEnableTimePhases()
    {
        return timePhases;
    }

    static void EnableTimePhases()
    {
        timePhases = true;
    }

    static void DisableTimePhases()
    {
        timePhases = false;
    }

    static void EnableInRange()
    {
        inRange = true;
    }

    static void DisableInRange()
    {
        inRange = false;
    }

    static bool IsInRange()
    {
        return inRange;
    }

    static void DisableEBO()
    {
        doEBO = false;
    }

    static void DisableCGSSA()
    {
        doCGSSA = false;
    }

    static void EnableCGSSA()
    {
        doCGSSA = true;
    }

    static void EnableSupportFuncSymbol()
    {
        supportFuncSymbol = true;
    }

    static void DisableSupportFuncSymbol()
    {
        supportFuncSymbol = false;
    }

    static bool addFuncSymbol()
    {
        return supportFuncSymbol;
    }

    static bool DoCGSSA()
    {
        return doCGSSA;
    }

    static void DisableLocalSchedule()
    {
        doLocalSchedule = false;
    }

    static void EnableLocalSchedule()
    {
        doLocalSchedule = true;
    }

    static bool DoLocalSchedule()
    {
        return doLocalSchedule;
    }

    static bool DoCGRegCoalecse()
    {
        return doCGRegCoalesce;
    }

    static void DisableIPARA()
    {
        doIPARA = false;
    }

    static bool DoIPARA()
    {
        return doIPARA;
    }

    static void EnableCFGO()
    {
        doCFGO = true;
    }

    static void DisableCFGO()
    {
        doCFGO = false;
    }

    static bool DoCFGO()
    {
        return doCFGO;
    }

    static void SetUseJitCodeSign(bool isJitCodeSign)
    {
        useJitCodeSign = isJitCodeSign;
    }

    static bool UseJitCodeSign()
    {
        return useJitCodeSign;
    }

    static void DisableICO()
    {
        doICO = false;
    }

    static void EnableBigEndianInCG()
    {
        cgBigEndian = true;
    }

    static void DisableBigEndianInCG()
    {
        cgBigEndian = false;
    }

    static bool IsBigEndian()
    {
        return cgBigEndian;
    }

    static void EnableArm64ilp32()
    {
        arm64ilp32 = true;
    }

    static void DisableArm64ilp32()
    {
        arm64ilp32 = false;
    }

    static bool IsArm64ilp32()
    {
        return arm64ilp32;
    }

    static bool IsTargetX86_64()
    {
        return targetArch == "x86_64";
    };

    static bool IsTargetAArch64()
    {
        return targetArch == "aarch64";
    };

    static bool DoPrePeephole()
    {
        return doPrePeephole;
    }

    static bool DoPeephole()
    {
        return doPeephole;
    }

    static bool DoPreSchedule()
    {
        return doPreSchedule;
    }

    static void DisableSchedule()
    {
        doSchedule = false;
    }

    static void EnableNoDupBB()
    {
        noDupBB = true;
    }

    static void DisableNoDupBB()
    {
        noDupBB = false;
    }

    static bool IsNoDupBB()
    {
        return noDupBB;
    }

    static void EnableNoCalleeCFI()
    {
        noCalleeCFI = true;
    }

    static void DisableNoCalleeCFI()
    {
        noCalleeCFI = false;
    }

    static bool IsNoCalleeCFI()
    {
        return noCalleeCFI;
    }

    static void EnableGeneralRegOnly()
    {
        generalRegOnly = true;
    }

    static void DisableGeneralRegOnly()
    {
        generalRegOnly = false;
    }

    static bool UseGeneralRegOnly()
    {
        return generalRegOnly;
    }

    static std::string &GetGlobalVarProFile()
    {
        return globalVarProfile;
    }

    static bool IsGlobalVarProFileEmpty()
    {
        return globalVarProfile.empty();
    }

    static bool IsEmitBlockMarker()
    {
        return emitBlockMarker;
    }

    static void EnableLazyBinding()
    {
        lazyBinding = true;
    }

    static void DisableLazyBinding()
    {
        lazyBinding = false;
    }

    static bool IsLazyBinding()
    {
        return lazyBinding;
    }

    static void EnableHotFix()
    {
        hotFix = true;
    }

    static void DisableHotFix()
    {
        hotFix = false;
    }

    static bool IsHotFix()
    {
        return hotFix;
    }

    static void EnableFramePointer()
    {
        useFramePointer = true;
    }

    static void DisableFramePointer()
    {
        useFramePointer = false;
    }

    static bool UseFramePointer()
    {
        return useFramePointer;
    }

    static void EnableGCOnly()
    {
        gcOnly = true;
    }

    static void DisableGCOnly()
    {
        gcOnly = false;
    }

    static bool IsGCOnly()
    {
        return gcOnly;
    }

    const OptionFlag &GetOptionFlag() const
    {
        return options;
    }

    void SetOptionFlag(const OptionFlag &flag)
    {
        options = flag;
    }

    static void SetFuncAlignPow(uint32 funcPow)
    {
        funcAlignPow = funcPow;
    }

    static uint32 GetFuncAlignPow()
    {
        return funcAlignPow;
    }

    static bool DoOptimizedFrameLayout()
    {
        return doOptimizedFrameLayout;
    }

    static bool DoCGIRVerify()
    {
        return doCgirVerify;
    }
private:
    std::vector<std::string> phaseSequence;
    EmitMemoryManager emitMemoryManager;

    bool runCGFlag = true;
    bool asmEmitterEnable = false;
    uint32 parserOption = 0;
    int32 optimizeLevel = 0;

    GenerateFlag generateFlag = 0;
    OptionFlag options = kUndefined;

    std::string ehExclusiveFile;
    /* we don't do exception handling in this list */
    std::vector<std::string> ehExclusiveFunctionName;

    static std::string targetArch;
    static std::unordered_set<std::string> dumpPhases;
    static std::unordered_set<std::string> skipPhases;
    static std::unordered_map<std::string, std::vector<std::string>> cyclePatternMap;
    static std::string skipFrom;
    static std::string skipAfter;
    static std::string dumpFunc;
    static std::string duplicateAsmFile;
    static bool optForSize;
    static bool enableHotColdSplit;
    static bool timePhases;
    static bool cgBigEndian;
    static bool doEBO;
    static bool doCGSSA;
    static bool doLocalSchedule;
    static bool doCGRegCoalesce;
    static bool doIPARA;
    static bool doCFGO;
    static bool doICO;
    static bool doStoreLoadOpt;
    static bool doGlobalOpt;
    static bool doPrePeephole;
    static bool doPeephole;
    static bool doSchedule;
    static bool doWriteRefFieldOpt;
    static bool doRegSavesOpt;
    static bool useSsaPreSave;
    static bool useSsuPreRestore;
    static bool checkArrayStore;
    static bool noDupBB;
    static bool noCalleeCFI;
    static std::string globalVarProfile;
    static bool nativeOpt;
    static bool lazyBinding;
    static bool arm64ilp32;
    static bool hotFix;
    static bool useJitCodeSign;
    static bool useFramePointer;
    static bool gcOnly;
    static bool doPreSchedule;
    static bool emitBlockMarker;
    static Range range;
    static bool inRange;
    static std::string profileData;
    static std::string profileFuncData;
    static std::string profileClassData;
    static Range spillRanges;
    static uint64 lsraBBOptSize;
    static uint64 lsraInsnOptSize;
    static uint64 overlapNum;
    static bool doPreLSRAOpt;
    static bool generalRegOnly;
    static std::string literalProfile;
    static bool noCommon;
    static uint32 funcAlignPow;
    static bool doOptimizedFrameLayout;
    static bool doCgirVerify;
    static bool supportFuncSymbol;
};
} /* namespace maplebe */

#define SET_FIND(SET, NAME) ((SET).find(NAME))
#define SET_END(SET) ((SET).end())
#define IS_STR_IN_SET(SET, NAME) (SET_FIND(SET, NAME) != SET_END(SET))

#define CG_DEBUG_FUNC(f)                                                                              \
    (!maplebe::CGOptions::GetDumpPhases().empty() && maplebe::CGOptions::IsDumpFunc((f).GetName()) && \
     maplebe::CGOptions::GetDumpPhases().find(PhaseName()) != maplebe::CGOptions::GetDumpPhases().end())
#ifndef TRACE_PHASE
#define TRACE_PHASE (IS_STR_IN_SET(maplebe::CGOptions::GetDumpPhases(), PhaseName()))
#endif

#endif /* MAPLEBE_INCLUDE_CG_CG_OPTION_H */
