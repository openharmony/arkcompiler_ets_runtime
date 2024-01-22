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

typedef void (*MemoryManagerSavePC2DeoptInfoCallback)(void *object, uint64_t pc, std::vector<uint64_t> deoptInfo);

typedef void (*MemoryManagerSavePC2CallSiteInfoCallback)(void *object, uint64_t pc, std::vector<uint64_t> callSiteInfo);

class CGOptions {
public:
    enum OptionEnum : uint64 {
        kUndefined = 0ULL,
        kDoCg = 1ULL << 0,
        kDoLinearScanRegAlloc = 1ULL << 1,
        kDoColorRegAlloc = 1ULL << 2,
        kConstFold = 1ULL << 3,
        kGenPic = 1ULL << 4,
        kGenPie = 1ULL << 5,
        kVerboseAsm = 1ULL << 6,
        kGenInsertCall = 1ULL << 7,
        kAddDebugTrace = 1ULL << 8,
        kGenYieldPoint = 1ULL << 9,
        kGenLocalRc = 1ULL << 10,
        kProEpilogueOpt = 1ULL << 11,
        kVerboseCG = 1ULL << 12,
        kDebugFriendly = 1ULL << 20,
        kWithLoc = 1ULL << 21,
        kWithDwarf = 1ULL << 22,
        kWithMpl = 1ULL << 23,
        kWithSrc = 1ULL << 24,
        kWithAsm = 1ULL << 25,
        kWithProfileCode = 1ULL << 30,
        kUseStackProtectorStrong = 1ULL << 31,
        kUseStackProtectorAll = 1ULL << 32,
        kSoeCheckInsert = 1ULL << 33,
        kAddFuncProfile = 1ULL << 34,
        kPatchLongBranch = 1ULL << 35,
        kTailCallOpt = 1ULL << 36,
        /* undocumented */
        kDumpCFG = 1ULL << 61,
        kDumpCgir = 1ULL << 62,
        kSuppressFileInfo = 1ULL << 63,
    };

    using OptionFlag = uint64;

    enum GenerateEnum : uint64 {
        kCMacroDef = 1ULL << 0,
        kGctib = 1ULL << 1,
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

    enum ABIType : uint8 { kABIHard, kABISoft, kABISoftFP };

    enum EmitFileType : uint8 {
        kAsm,
        kObj,
        kEmitNone,
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
        kDoCg | kGenPie | kDoColorRegAlloc
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
    bool SolveOptions(bool isDebug);
    void DecideMplcgRealLevel(bool isDebug);

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

    void ParseExclusiveFunc(const std::string &fileName);
    void ParseCyclePattern(const std::string &fileName);

    void EnableO0();
    void EnableO1();
    void EnableO2();
    void EnableLiteCG();

    bool GenDef() const
    {
        return generateFlag & kCMacroDef;
    }

    bool GenGctib() const
    {
        return generateFlag & kGctib;
    }

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

    bool DoConstFold() const
    {
        return options & kConstFold;
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
    bool DoColoringBasedRegisterAllocation() const
    {
        return (options & kDoColorRegAlloc) != 0;
    }

    bool GeneratePositionIndependentExecutable() const
    {
        return (options & kGenPie) != 0;
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

    bool DoPrologueEpilogue() const
    {
        return (options & kProEpilogueOpt) != 0;
    }

    bool IsStackProtectorStrong() const
    {
        return (options & kUseStackProtectorStrong) != 0;
    }

    bool IsStackProtectorAll() const
    {
        return (options & kUseStackProtectorAll) != 0;
    }

    bool WithLoc() const
    {
        return (options & kWithLoc) != 0;
    }

    bool WithDwarf() const
    {
        return (options & kWithDwarf) != 0;
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

    bool NeedInsertInstrumentationFunction() const
    {
        return (options & kGenInsertCall) != 0;
    }

    bool InstrumentWithDebugTraceCall() const
    {
        return (options & kAddDebugTrace) != 0;
    }

    bool InstrumentWithProfile() const
    {
        return (options & kAddFuncProfile) != 0;
    }

    bool DoPatchLongBranch() const
    {
        return (options & kPatchLongBranch) != 0;
    }

    bool DoTailCall() const
    {
        return (options & kTailCallOpt) != 0;
    }

    bool DoCheckSOE() const
    {
        return (options & kSoeCheckInsert) != 0;
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
    void SetRange(const std::string &str, const std::string &cmd, Range &subRange);
    void SetTargetMachine(const std::string &str);

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

    bool IsInsertCall() const
    {
        return insertCall;
    }

    void SetInsertCall(bool insertFlag)
    {
        insertCall = insertFlag;
    }

    bool IsGenerateObjectMap() const
    {
        return generateObjectMap;
    }

    void SetGenerateObjectMap(bool flag)
    {
        generateObjectMap = flag;
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

    const std::string &GetInstrumentationFunction() const
    {
        return instrumentationFunction;
    }

    void SetInstrumentationFunction(const std::string &function)
    {
        instrumentationFunction = function;
    }

    const std::string &GetClassListFile() const
    {
        return classListFile;
    }

    void SetClassListFile(const std::string &classList)
    {
        classListFile = classList;
    }

    void SetEHExclusiveFile(const std::string &ehExclusive)
    {
        ehExclusiveFile = ehExclusive;
    }

    void SetCyclePatternFile(const std::string &cyclePattern)
    {
        cyclePatternFile = cyclePattern;
    }

    static bool IsQuiet()
    {
        return quiet;
    }

    static void SetQuiet(bool flag)
    {
        quiet = flag;
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

    static void SetDuplicateAsmFile(const std::string &fileName)
    {
        duplicateAsmFile = fileName;
    }

    static bool UseRange()
    {
        return range.enable;
    }
    static const std::string &GetFastFuncsAsmFile()
    {
        return fastFuncsAsmFile;
    }

    static bool IsFastFuncsAsmFileEmpty()
    {
        return fastFuncsAsmFile.empty();
    }

    static void SetFastFuncsAsmFile(const std::string &fileName)
    {
        fastFuncsAsmFile = fileName;
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

    static uint8 GetRematLevel()
    {
        return rematLevel;
    }

    static bool OptimizeForSize()
    {
        return optForSize;
    }

    static void SetRematLevel(uint8 level)
    {
        rematLevel = level;
    }

    static uint8 GetFastAllocMode()
    {
        return fastAllocMode;
    }

    static void SetFastAllocMode(uint8 mode)
    {
        fastAllocMode = mode;
    }

    static void EnableBarriersForVolatile()
    {
        useBarriersForVolatile = true;
    }

    static void DisableBarriersForVolatile()
    {
        useBarriersForVolatile = false;
    }

    static bool UseBarriersForVolatile()
    {
        return useBarriersForVolatile;
    }
    static void EnableFastAlloc()
    {
        fastAlloc = true;
    }

    static bool IsFastAlloc()
    {
        return fastAlloc;
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

    static void EnableEBO()
    {
        doEBO = true;
    }

    static void DisableEBO()
    {
        doEBO = false;
    }

    static bool DoEBO()
    {
        return doEBO;
    }

    static void DisableCGSSA()
    {
        doCGSSA = false;
    }

    static void EnableCGSSA()
    {
        doCGSSA = true;
    }

    static bool DoCGSSA()
    {
        return doCGSSA;
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

    static void EnableRegSavesOpt()
    {
        doRegSavesOpt = true;
    }

    static void DisableRegSavesOpt()
    {
        doRegSavesOpt = false;
    }

    static bool DoRegSavesOpt()
    {
        return doRegSavesOpt;
    }

    static void EnableSsaPreSave()
    {
        useSsaPreSave = true;
    }

    static void DisableSsaPreSave()
    {
        useSsaPreSave = false;
    }

    static bool UseSsaPreSave()
    {
        return useSsaPreSave;
    }
    static void EnableSsuPreRestore()
    {
        useSsuPreRestore = true;
    }

    static void DisableSsuPreRestore()
    {
        useSsuPreRestore = false;
    }

    static bool UseSsuPreRestore()
    {
        return useSsuPreRestore;
    }

    static void EnableICO()
    {
        doICO = true;
    }

    static void DisableICO()
    {
        doICO = false;
    }

    static bool DoICO()
    {
        return doICO;
    }

    static void EnableStoreLoadOpt()
    {
        doStoreLoadOpt = true;
    }

    static void DisableStoreLoadOpt()
    {
        doStoreLoadOpt = false;
    }

    static bool DoStoreLoadOpt()
    {
        return doStoreLoadOpt;
    }

    static void EnableGlobalOpt()
    {
        doGlobalOpt = true;
    }

    static void DisableGlobalOpt()
    {
        doGlobalOpt = false;
    }

    static void EnableHotColdSplit()
    {
        enableHotColdSplit = true;
    }

    static void DisableHotColdSplit()
    {
        enableHotColdSplit = false;
    }

    static bool DoEnableHotColdSplit()
    {
        return enableHotColdSplit;
    }

    static bool DoGlobalOpt()
    {
        return doGlobalOpt;
    }

    static void EnableAlignAnalysis()
    {
        doAlignAnalysis = true;
    }

    static void DisableAlignAnalysis()
    {
        doAlignAnalysis = false;
    }

    static bool DoAlignAnalysis()
    {
        return doAlignAnalysis;
    }

    static void EnableCondBrAlign()
    {
        doCondBrAlign = true;
    }

    static void DisableCondBrAlign()
    {
        doCondBrAlign = false;
    }

    static bool DoCondBrAlign()
    {
        return doCondBrAlign;
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

    static void EnableVregRename()
    {
        doVregRename = true;
    }

    static void DisableVregRename()
    {
        doVregRename = false;
    }

    static bool DoVregRename()
    {
        return doVregRename;
    }

    static void EnableMultiPassColorRA()
    {
        doMultiPassColorRA = true;
    }

    static void DisableMultiPassColorRA()
    {
        doMultiPassColorRA = false;
    }

    static bool DoMultiPassColorRA()
    {
        return doMultiPassColorRA;
    }

    static void EnablePreLSRAOpt()
    {
        doPreLSRAOpt = true;
    }

    static void DisablePreLSRAOpt()
    {
        doPreLSRAOpt = false;
    }

    static bool DoPreLSRAOpt()
    {
        return doPreLSRAOpt;
    }

    static void EnablePrePeephole()
    {
        doPrePeephole = true;
    }

    static void DisablePrePeephole()
    {
        doPrePeephole = false;
    }

    static bool DoPrePeephole()
    {
        return doPrePeephole;
    }

    static void EnablePeephole()
    {
        doPeephole = true;
    }

    static void DisablePeephole()
    {
        doPeephole = false;
    }

    static bool DoPeephole()
    {
        return doPeephole;
    }

    static void EnableRetMerge()
    {
        doRetMerge = true;
    }

    static void DisableRetMerge()
    {
        doRetMerge = false;
    }

    static bool DoRetMerge()
    {
        return doRetMerge;
    }

    static void EnablePreSchedule()
    {
        doPreSchedule = true;
    }

    static void DisablePreSchedule()
    {
        doPreSchedule = false;
    }

    static bool DoPreSchedule()
    {
        return doPreSchedule;
    }

    static void EnableSchedule()
    {
        doSchedule = true;
    }

    static void DisableSchedule()
    {
        doSchedule = false;
    }

    static bool DoSchedule()
    {
        return doSchedule;
    }
    static void EnableWriteRefFieldOpt()
    {
        doWriteRefFieldOpt = true;
    }

    static void DisableWriteRefFieldOpt()
    {
        doWriteRefFieldOpt = false;
    }
    static bool DoWriteRefFieldOpt()
    {
        return doWriteRefFieldOpt;
    }

    static void EnableDumpOptimizeCommonLog()
    {
        dumpOptimizeCommonLog = true;
    }

    static void DisableDumpOptimizeCommonLog()
    {
        dumpOptimizeCommonLog = false;
    }

    static bool IsDumpOptimizeCommonLog()
    {
        return dumpOptimizeCommonLog;
    }

    static void EnableCheckArrayStore()
    {
        checkArrayStore = true;
    }

    static void DisableCheckArrayStore()
    {
        checkArrayStore = false;
    }

    static bool IsCheckArrayStore()
    {
        return checkArrayStore;
    }

    static void EnableExclusiveEH()
    {
        exclusiveEH = true;
    }

    static bool IsExclusiveEH()
    {
        return exclusiveEH;
    }

    static void EnablePIC()
    {
        doPIC = true;
    }

    static void DisablePIC()
    {
        doPIC = false;
    }

    static bool IsPIC()
    {
        return doPIC;
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

    static void EnableEmitCyclePattern()
    {
        emitCyclePattern = true;
    }

    static bool IsInsertYieldPoint()
    {
        return insertYieldPoint;
    }

    static void EnableMapleLinker()
    {
        mapleLinker = true;
    }

    static void DisableMapleLinker()
    {
        mapleLinker = false;
    }

    static bool IsMapleLinker()
    {
        return mapleLinker;
    }
    static void EnableReplaceASM()
    {
        replaceASM = true;
    }

    static void DisableReplaceASM()
    {
        replaceASM = false;
    }

    static bool IsReplaceASM()
    {
        return replaceASM;
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

    static void EnablePrintFunction()
    {
        printFunction = true;
    }

    static void DisablePrintFunction()
    {
        printFunction = false;
    }

    static bool IsPrintFunction()
    {
        return printFunction;
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

    static void EnableNativeOpt()
    {
        nativeOpt = true;
    }

    static void DisableNativeOpt()
    {
        nativeOpt = false;
    }

    static bool IsNativeOpt()
    {
        return nativeOpt;
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

    static void EnableDebugSched()
    {
        debugSched = true;
    }

    static void DisableDebugSched()
    {
        debugSched = false;
    }

    static bool IsDebugSched()
    {
        return debugSched;
    }

    static void EnableDruteForceSched()
    {
        bruteForceSched = true;
    }

    static void DisableDruteForceSched()
    {
        bruteForceSched = false;
    }

    static bool IsDruteForceSched()
    {
        return bruteForceSched;
    }

    static void EnableSimulateSched()
    {
        simulateSched = true;
    }

    static void DisableSimulateSched()
    {
        simulateSched = false;
    }

    static bool IsSimulateSched()
    {
        return simulateSched;
    }

    static void SetABIType(const std::string &type)
    {
        if (type == "hard") {
            abiType = kABIHard;
        } else if (type == "soft") {
            CHECK_FATAL(false, "float-abi=soft is not supported Currently.");
        } else if (type == "softfp") {
            abiType = kABISoftFP;
        } else {
            CHECK_FATAL(false, "unexpected abi-type, only hard, soft and softfp are supported");
        }
    }

    static ABIType GetABIType()
    {
        return abiType;
    }

    static void SetEmitFileType(const std::string &type)
    {
        if (type == "asm") {
            emitFileType = kAsm;
        } else if (type == "obj") {
            emitFileType = kObj;
        } else if (type == "null") {
            emitFileType = kEmitNone;
            CHECK_FATAL(false, "null is not supported Currently.");
        } else {
            CHECK_FATAL(false, "unexpected file-type, only asm, obj, and null are supported");
        }
    }

    static EmitFileType GetEmitFileType()
    {
        return emitFileType;
    }

    static void EnableLongCalls()
    {
        genLongCalls = true;
    }

    static void DisableLongCalls()
    {
        genLongCalls = false;
    }

    static bool IsLongCalls()
    {
        return genLongCalls;
    }

    static void EnableFunctionSections()
    {
        functionSections = true;
    }

    static void DisableFunctionSections()
    {
        functionSections = false;
    }

    static bool IsFunctionSections()
    {
        return functionSections;
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

    static void EnableFastMath()
    {
        fastMath = true;
    }

    static void DisableFastMath()
    {
        fastMath = false;
    }

    static bool IsFastMath()
    {
        return fastMath;
    }

    static void EnableCommon()
    {
        noCommon = false;
    }

    static void DisableCommon()
    {
        noCommon = true;
    }

    static bool IsNoCommon()
    {
        return noCommon;
    }

    static void SetAlignMinBBSize(uint32 minBBSize)
    {
        alignMinBBSize = minBBSize;
    }

    static uint32 GetAlignMinBBSize()
    {
        return alignMinBBSize;
    }

    static void SetAlignMaxBBSize(uint32 maxBBSize)
    {
        alignMaxBBSize = maxBBSize;
    }

    static uint32 GetAlignMaxBBSize()
    {
        return alignMaxBBSize;
    }

    static void SetLoopAlignPow(uint32 loopPow)
    {
        loopAlignPow = loopPow;
    }

    static uint32 GetLoopAlignPow()
    {
        return loopAlignPow;
    }

    static void SetJumpAlignPow(uint32 jumpPow)
    {
        jumpAlignPow = jumpPow;
    }

    static uint32 GetJumpAlignPow()
    {
        return jumpAlignPow;
    }

    static void SetFuncAlignPow(uint32 funcPow)
    {
        funcAlignPow = funcPow;
    }

    static uint32 GetFuncAlignPow()
    {
        return funcAlignPow;
    }

    static void EnableOptimizedFrameLayout()
    {
        doOptimizedFrameLayout = true;
    }

    static void DisableOptimizedFrameLayout()
    {
        doOptimizedFrameLayout = false;
    }

    static bool DoOptimizedFrameLayout()
    {
        return doOptimizedFrameLayout;
    }

private:
    std::vector<std::string> phaseSequence;
    EmitMemoryManager emitMemoryManager;

    bool insertCall = false;
    bool runCGFlag = true;
    bool generateObjectMap = true;
    uint32 parserOption = 0;
    int32 optimizeLevel = 0;

    GenerateFlag generateFlag = 0;
    OptionFlag options = kUndefined;
    std::string instrumentationFunction;

    std::string classListFile;
    std::string ehExclusiveFile;
    std::string cyclePatternFile;
    /* we don't do exception handling in this list */
    std::vector<std::string> ehExclusiveFunctionName;

    static bool quiet;
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
    static bool useBarriersForVolatile;
    static bool timePhases;
    static bool cgBigEndian;
    static bool doEBO;
    static bool doCGSSA;
    static bool doCGRegCoalesce;
    static bool doIPARA;
    static bool doCFGO;
    static bool doICO;
    static bool doStoreLoadOpt;
    static bool doGlobalOpt;
    static bool doVregRename;
    static bool doMultiPassColorRA;
    static bool doPrePeephole;
    static bool doPeephole;
    static bool doRetMerge;
    static bool doSchedule;
    static bool doAlignAnalysis;
    static bool doCondBrAlign;
    static bool doWriteRefFieldOpt;
    static bool doRegSavesOpt;
    static bool useSsaPreSave;
    static bool useSsuPreRestore;
    static bool dumpOptimizeCommonLog;
    static bool checkArrayStore;
    static bool exclusiveEH;
    static bool doPIC;
    static bool noDupBB;
    static bool noCalleeCFI;
    static bool emitCyclePattern;
    static bool insertYieldPoint;
    static bool mapleLinker;
    static bool printFunction;
    static std::string globalVarProfile;
    static bool nativeOpt;
    static bool lazyBinding;
    static bool arm64ilp32;
    static bool hotFix;
    /* if true dump scheduling information */
    static bool debugSched;
    /* if true do BruteForceSchedule */
    static bool bruteForceSched;
    /* if true do SimulateSched */
    static bool simulateSched;
    static ABIType abiType;
    static EmitFileType emitFileType;
    /* if true generate adrp/ldr/blr */
    static bool genLongCalls;
    static bool functionSections;
    static bool useFramePointer;
    static bool gcOnly;
    static bool doPreSchedule;
    static bool emitBlockMarker;
    static Range range;
    static bool inRange;
    static bool doPatchLongBranch;
    static std::string profileData;
    static std::string profileFuncData;
    static std::string profileClassData;
    static std::string fastFuncsAsmFile;
    static Range spillRanges;
    static uint64 lsraBBOptSize;
    static uint64 lsraInsnOptSize;
    static uint64 overlapNum;
    static uint8 rematLevel;
    static uint8 fastAllocMode;
    static bool fastAlloc;
    static bool doPreLSRAOpt;
    static bool replaceASM;
    static bool generalRegOnly;
    static std::string literalProfile;
    static bool fastMath;
    static bool noCommon;
    static uint32 alignMinBBSize;
    static uint32 alignMaxBBSize;
    static uint32 loopAlignPow;
    static uint32 jumpAlignPow;
    static uint32 funcAlignPow;
    static bool doOptimizedFrameLayout;
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
