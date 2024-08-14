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

#include "cg_option.h"
#include <fstream>
#include <unordered_map>
#include "cg_options.h"
#include "driver_options.h"
#include "mpl_logging.h"
#include "parser_opt.h"
#include "mir_parser.h"
#include "string_utils.h"
#include "triple.h"

namespace maplebe {
using namespace maple;

const std::string kMplcgVersion = "";

bool CGOptions::timePhases = false;
std::string CGOptions::targetArch = "";
std::unordered_set<std::string> CGOptions::dumpPhases = {};
std::unordered_set<std::string> CGOptions::skipPhases = {};
std::unordered_map<std::string, std::vector<std::string>> CGOptions::cyclePatternMap = {};
std::string CGOptions::skipFrom = "";
std::string CGOptions::skipAfter = "";
std::string CGOptions::dumpFunc = "*";
std::string CGOptions::globalVarProfile = "";
std::string CGOptions::profileData = "";
std::string CGOptions::profileFuncData = "";
std::string CGOptions::profileClassData = "";
#ifdef TARGARM32
std::string CGOptions::duplicateAsmFile = "";
#else
std::string CGOptions::duplicateAsmFile = "maple/mrt/codetricks/arch/arm64/duplicateFunc.s";
#endif
Range CGOptions::range = Range();
std::string CGOptions::fastFuncsAsmFile = "";
Range CGOptions::spillRanges = Range();
uint8 CGOptions::fastAllocMode = 0; /* 0: fast, 1: spill all */
bool CGOptions::fastAlloc = false;
uint64 CGOptions::lsraBBOptSize = 150000;
uint64 CGOptions::lsraInsnOptSize = 200000;
uint64 CGOptions::overlapNum = 28;
uint8 CGOptions::rematLevel = 2;
bool CGOptions::optForSize = false;
bool CGOptions::enableHotColdSplit = false;
uint32 CGOptions::alignMinBBSize = 16;
uint32 CGOptions::alignMaxBBSize = 96;
uint32 CGOptions::loopAlignPow = 4;
uint32 CGOptions::jumpAlignPow = 5;
uint32 CGOptions::funcAlignPow = 5;
bool CGOptions::doOptimizedFrameLayout = true;
bool CGOptions::supportFuncSymbol = false;
#if TARGAARCH64 || TARGRISCV64
bool CGOptions::useBarriersForVolatile = false;
#else
bool CGOptions::useBarriersForVolatile = true;
#endif
bool CGOptions::exclusiveEH = false;
bool CGOptions::doEBO = false;
bool CGOptions::doCGSSA = false;
bool CGOptions::doLocalSchedule = false;
bool CGOptions::doCGRegCoalesce = false;
bool CGOptions::doIPARA = true;
bool CGOptions::doCFGO = false;
bool CGOptions::doICO = false;
bool CGOptions::doStoreLoadOpt = false;
bool CGOptions::doGlobalOpt = false;
bool CGOptions::doVregRename = false;
bool CGOptions::doMultiPassColorRA = true;
bool CGOptions::doPrePeephole = false;
bool CGOptions::doPeephole = false;
bool CGOptions::doRetMerge = false;
bool CGOptions::doSchedule = false;
bool CGOptions::doWriteRefFieldOpt = false;
bool CGOptions::dumpOptimizeCommonLog = false;
bool CGOptions::checkArrayStore = false;
bool CGOptions::doPIC = false;
bool CGOptions::noDupBB = false;
bool CGOptions::noCalleeCFI = true;
bool CGOptions::emitCyclePattern = false;
bool CGOptions::insertYieldPoint = false;
bool CGOptions::mapleLinker = false;
bool CGOptions::printFunction = false;
bool CGOptions::nativeOpt = false;
bool CGOptions::lazyBinding = false;
bool CGOptions::hotFix = false;
bool CGOptions::debugSched = false;
bool CGOptions::bruteForceSched = false;
bool CGOptions::simulateSched = false;
CGOptions::ABIType CGOptions::abiType = kABIHard;
bool CGOptions::genLongCalls = false;
bool CGOptions::functionSections = false;
bool CGOptions::useFramePointer = false;
bool CGOptions::gcOnly = false;
bool CGOptions::quiet = true;
bool CGOptions::doPatchLongBranch = false;
bool CGOptions::doPreSchedule = false;
bool CGOptions::emitBlockMarker = true;
bool CGOptions::inRange = false;
bool CGOptions::doPreLSRAOpt = false;
bool CGOptions::doRegSavesOpt = false;
bool CGOptions::useSsaPreSave = false;
bool CGOptions::useSsuPreRestore = false;
bool CGOptions::replaceASM = false;
bool CGOptions::generalRegOnly = false;
bool CGOptions::fastMath = false;
bool CGOptions::doAlignAnalysis = false;
bool CGOptions::doCondBrAlign = false;
bool CGOptions::cgBigEndian = false;
bool CGOptions::arm64ilp32 = false;
bool CGOptions::noCommon = false;
bool CGOptions::doCgirVerify = false;
bool CGOptions::useJitCodeSign = false;

CGOptions &CGOptions::GetInstance()
{
    static CGOptions instance;
    return instance;
}

std::ostream& CGOptions::GetLogStream() const
{
    return LogInfo::MapleLogger();
}

void CGOptions::DecideMplcgRealLevel(bool isDebug)
{
    if (opts::cg::o0) {
        if (isDebug) {
            LogInfo::MapleLogger() << "Real Mplcg level: O0\n";
        }
        EnableO0();
    }

    if (opts::cg::o1) {
        if (isDebug) {
            LogInfo::MapleLogger() << "Real Mplcg level: O1\n";
        }
        EnableO1();
    }

    if (opts::cg::o2 || opts::cg::os) {
        if (opts::cg::os) {
            optForSize = true;
        }
        if (isDebug) {
            std::string oLog = (opts::cg::os == true) ? "Os" : "O2";
            LogInfo::MapleLogger() << "Real Mplcg level: " << oLog << "\n";
        }
        EnableO2();
    }
    if (opts::cg::olitecg) {
        if (isDebug) {
            LogInfo::MapleLogger() << "Real Mplcg level: LiteCG\n";
        }
        EnableLiteCG();
    }
}

bool CGOptions::SolveOptions(bool isDebug)
{
    DecideMplcgRealLevel(isDebug);

    for (const auto &opt : cgCategory.GetEnabledOptions()) {
        std::string printOpt;
        if (isDebug) {
            for (const auto &val : opt->GetRawValues()) {
                printOpt += opt->GetName() + " " + val + " ";
            }
            LogInfo::MapleLogger() << "cg options: " << printOpt << '\n';
        }
    }

    if (opts::cg::supportFuncSymbol.IsEnabledByUser()) {
        opts::cg::supportFuncSymbol ? EnableSupportFuncSymbol() : DisableSupportFuncSymbol();
    }

    if (opts::cg::quiet.IsEnabledByUser()) {
        SetQuiet(true);
    }

    if (opts::cg::pie.IsEnabledByUser()) {
        opts::cg::pie ? SetOption(CGOptions::kGenPie) : ClearOption(CGOptions::kGenPie);
    }

    if (opts::cg::fpic.IsEnabledByUser()) {
        if (opts::cg::fpic) {
            EnablePIC();
            SetOption(CGOptions::kGenPic);
        } else {
            DisablePIC();
            ClearOption(CGOptions::kGenPic);
        }
    }

    if (opts::cg::verboseAsm.IsEnabledByUser()) {
        opts::cg::verboseAsm ? SetOption(CGOptions::kVerboseAsm) : ClearOption(CGOptions::kVerboseAsm);
        SetAsmEmitterEnable(true);
    }

    if (opts::cg::verboseCg.IsEnabledByUser()) {
        opts::cg::verboseCg ? SetOption(CGOptions::kVerboseCG) : ClearOption(CGOptions::kVerboseCG);
    }

    if (opts::cg::maplelinker.IsEnabledByUser()) {
        opts::cg::maplelinker ? EnableMapleLinker() : DisableMapleLinker();
    }

    if (opts::cg::fastAlloc.IsEnabledByUser()) {
        EnableFastAlloc();
        SetFastAllocMode(opts::cg::fastAlloc);
    }

    if (opts::cg::useBarriersForVolatile.IsEnabledByUser()) {
        opts::cg::useBarriersForVolatile ? EnableBarriersForVolatile() : DisableBarriersForVolatile();
    }

    if (opts::cg::spillRange.IsEnabledByUser()) {
        SetRange(opts::cg::spillRange, "--pill-range", GetSpillRanges());
    }

    if (opts::cg::range.IsEnabledByUser()) {
        SetRange(opts::cg::range, "--range", GetRange());
    }

    if (opts::cg::timePhases.IsEnabledByUser()) {
        opts::cg::timePhases ? EnableTimePhases() : DisableTimePhases();
    }

    if (opts::cg::dumpFunc.IsEnabledByUser()) {
        SetDumpFunc(opts::cg::dumpFunc);
    }

    if (opts::cg::duplicateAsmList.IsEnabledByUser()) {
        SetDuplicateAsmFile(opts::cg::duplicateAsmList);
    }

    if (opts::cg::duplicateAsmList2.IsEnabledByUser()) {
        SetFastFuncsAsmFile(opts::cg::duplicateAsmList2);
    }

    if (opts::cg::stackProtectorStrong.IsEnabledByUser()) {
        SetOption(kUseStackProtectorStrong);
    }

    if (opts::cg::stackProtectorAll.IsEnabledByUser()) {
        SetOption(kUseStackProtectorAll);
    }

    if (opts::cg::debug.IsEnabledByUser()) {
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        ClearOption(kSuppressFileInfo);
    }

    if (opts::cg::gdwarf.IsEnabledByUser()) {
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithDwarf);
        SetParserOption(kWithDbgInfo);
        ClearOption(kSuppressFileInfo);
    }

    if (opts::cg::gsrc.IsEnabledByUser()) {
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithSrc);
        ClearOption(kWithMpl);
    }

    if (opts::cg::gmixedsrc.IsEnabledByUser()) {
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithSrc);
        SetOption(kWithMpl);
    }

    if (opts::cg::gmixedasm.IsEnabledByUser()) {
        SetOption(kDebugFriendly);
        SetOption(kWithLoc);
        SetOption(kWithSrc);
        SetOption(kWithMpl);
        SetOption(kWithAsm);
    }

    if (opts::cg::profile.IsEnabledByUser()) {
        SetOption(kWithProfileCode);
        SetParserOption(kWithProfileInfo);
    }

    if (opts::cg::withRaLinearScan.IsEnabledByUser()) {
        SetOption(kDoLinearScanRegAlloc);
        ClearOption(kDoColorRegAlloc);
    }

    if (opts::cg::withRaGraphColor.IsEnabledByUser()) {
        SetOption(kDoColorRegAlloc);
        ClearOption(kDoLinearScanRegAlloc);
    }

    if (opts::cg::printFunc.IsEnabledByUser()) {
        opts::cg::printFunc ? EnablePrintFunction() : DisablePrintFunction();
    }

    if (opts::cg::addDebugTrace.IsEnabledByUser()) {
        SetOption(kAddDebugTrace);
    }

    if (opts::cg::addFuncProfile.IsEnabledByUser()) {
        SetOption(kAddFuncProfile);
    }

    if (opts::cg::suppressFileinfo.IsEnabledByUser()) {
        SetOption(kSuppressFileInfo);
    }

    if (opts::cg::patchLongBranch.IsEnabledByUser()) {
        SetOption(kPatchLongBranch);
    }

    if (opts::cg::constFold.IsEnabledByUser()) {
        opts::cg::constFold ? SetOption(kConstFold) : ClearOption(kConstFold);
    }

    if (opts::cg::dumpCfg.IsEnabledByUser()) {
        SetOption(kDumpCFG);
    }

    if (opts::cg::classListFile.IsEnabledByUser()) {
        SetClassListFile(opts::cg::classListFile);
    }

    if (opts::cg::genCMacroDef.IsEnabledByUser()) {
        SetOrClear(GetGenerateFlags(), CGOptions::kCMacroDef, opts::cg::genCMacroDef);
    }

    if (opts::cg::genGctibFile.IsEnabledByUser()) {
        SetOrClear(GetGenerateFlags(), CGOptions::kGctib, opts::cg::genGctibFile);
    }

    if (opts::cg::yieldpoint.IsEnabledByUser()) {
        SetOrClear(GetGenerateFlags(), CGOptions::kGenYieldPoint, opts::cg::yieldpoint);
    }

    if (opts::cg::localRc.IsEnabledByUser()) {
        SetOrClear(GetGenerateFlags(), CGOptions::kGenLocalRc, opts::cg::localRc);
    }

    if (opts::cg::ehExclusiveList.IsEnabledByUser()) {
        SetEHExclusiveFile(opts::cg::ehExclusiveList);
        EnableExclusiveEH();
        ParseExclusiveFunc(opts::cg::ehExclusiveList);
    }

    if (opts::cg::cyclePatternList.IsEnabledByUser()) {
        SetCyclePatternFile(opts::cg::cyclePatternList);
        EnableEmitCyclePattern();
        ParseCyclePattern(opts::cg::cyclePatternList);
    }

    if (opts::cg::cg.IsEnabledByUser()) {
        SetRunCGFlag(opts::cg::cg);
        opts::cg::cg ? SetOption(CGOptions::kDoCg) : ClearOption(CGOptions::kDoCg);
    }

    if (opts::cg::objmap.IsEnabledByUser()) {
        SetGenerateObjectMap(opts::cg::objmap);
    }

    if (opts::cg::replaceAsm.IsEnabledByUser()) {
        opts::cg::replaceAsm ? EnableReplaceASM() : DisableReplaceASM();
    }

    if (opts::cg::generalRegOnly.IsEnabledByUser()) {
        opts::cg::generalRegOnly ? EnableGeneralRegOnly() : DisableGeneralRegOnly();
    }

    if (opts::cg::lazyBinding.IsEnabledByUser()) {
        opts::cg::lazyBinding ? EnableLazyBinding() : DisableLazyBinding();
    }

    if (opts::cg::hotFix.IsEnabledByUser()) {
        opts::cg::hotFix ? EnableHotFix() : DisableHotFix();
    }

    if (opts::cg::soeCheck.IsEnabledByUser()) {
        SetOption(CGOptions::kSoeCheckInsert);
    }

    if (opts::cg::checkArraystore.IsEnabledByUser()) {
        opts::cg::checkArraystore ? EnableCheckArrayStore() : DisableCheckArrayStore();
    }

    if (opts::cg::ebo.IsEnabledByUser()) {
        opts::cg::ebo ? EnableEBO() : DisableEBO();
    }

    if (opts::cg::cfgo.IsEnabledByUser()) {
        opts::cg::cfgo ? EnableCFGO() : DisableCFGO();
    }

    if (opts::cg::ico.IsEnabledByUser()) {
        opts::cg::ico ? EnableICO() : DisableICO();
    }

    if (opts::cg::storeloadopt.IsEnabledByUser()) {
        opts::cg::storeloadopt ? EnableStoreLoadOpt() : DisableStoreLoadOpt();
    }

    if (opts::cg::globalopt.IsEnabledByUser()) {
        opts::cg::globalopt ? EnableGlobalOpt() : DisableGlobalOpt();
    }

    if (opts::cg::hotcoldsplit.IsEnabledByUser()) {
        opts::cg::hotcoldsplit ? EnableHotColdSplit() : DisableHotColdSplit();
    }

    if (opts::cg::prelsra.IsEnabledByUser()) {
        opts::cg::prelsra ? EnablePreLSRAOpt() : DisablePreLSRAOpt();
    }

    if (opts::cg::prepeep.IsEnabledByUser()) {
        opts::cg::prepeep ? EnablePrePeephole() : DisablePrePeephole();
    }

    if (opts::cg::peep.IsEnabledByUser()) {
        opts::cg::peep ? EnablePeephole() : DisablePeephole();
    }

    if (opts::cg::retMerge.IsEnabledByUser()) {
        opts::cg::retMerge ? EnableRetMerge() : DisableRetMerge();
    }

    if (opts::cg::preschedule.IsEnabledByUser()) {
        opts::cg::preschedule ? EnablePreSchedule() : DisablePreSchedule();
    }

    if (opts::cg::schedule.IsEnabledByUser()) {
        opts::cg::schedule ? EnableSchedule() : DisableSchedule();
    }

    if (opts::cg::vregRename.IsEnabledByUser()) {
        opts::cg::vregRename ? EnableVregRename() : DisableVregRename();
    }

    if (opts::cg::fullcolor.IsEnabledByUser()) {
        opts::cg::fullcolor ? EnableMultiPassColorRA() : DisableMultiPassColorRA();
    }

    if (opts::cg::writefieldopt.IsEnabledByUser()) {
        opts::cg::writefieldopt ? EnableWriteRefFieldOpt() : DisableWriteRefFieldOpt();
    }

    if (opts::cg::dumpOlog.IsEnabledByUser()) {
        opts::cg::dumpOlog ? EnableDumpOptimizeCommonLog() : DisableDumpOptimizeCommonLog();
    }

    if (opts::cg::nativeopt.IsEnabledByUser()) {
        // Disabling Looks strage: should be checked by author of the code
        DisableNativeOpt();
    }

    if (opts::cg::dupBb.IsEnabledByUser()) {
        opts::cg::dupBb ? DisableNoDupBB() : EnableNoDupBB();
    }

    if (opts::cg::calleeCfi.IsEnabledByUser()) {
        opts::cg::calleeCfi ? DisableNoCalleeCFI() : EnableNoCalleeCFI();
    }

    if (opts::cg::proepilogue.IsEnabledByUser()) {
        opts::cg::proepilogue ? SetOption(CGOptions::kProEpilogueOpt) : ClearOption(CGOptions::kProEpilogueOpt);
    }

    if (opts::cg::tailcall.IsEnabledByUser()) {
        opts::cg::tailcall ? SetOption(CGOptions::kTailCallOpt) : ClearOption(CGOptions::kTailCallOpt);
    }

    if (opts::cg::calleeregsPlacement.IsEnabledByUser()) {
        opts::cg::calleeregsPlacement ? EnableRegSavesOpt() : DisableRegSavesOpt();
    }

    if (opts::cg::ssapreSave.IsEnabledByUser()) {
        opts::cg::ssapreSave ? EnableSsaPreSave() : DisableSsaPreSave();
    }

    if (opts::cg::ssupreRestore.IsEnabledByUser()) {
        opts::cg::ssupreRestore ? EnableSsuPreRestore() : DisableSsuPreRestore();
    }

    if (opts::cg::lsraBb.IsEnabledByUser()) {
        SetLSRABBOptSize(opts::cg::lsraBb);
    }

    if (opts::cg::lsraInsn.IsEnabledByUser()) {
        SetLSRAInsnOptSize(opts::cg::lsraInsn);
    }

    if (opts::cg::lsraOverlap.IsEnabledByUser()) {
        SetOverlapNum(opts::cg::lsraOverlap);
    }

    if (opts::cg::remat.IsEnabledByUser()) {
        SetRematLevel(opts::cg::remat);
    }

    if (opts::cg::dumpPhases.IsEnabledByUser()) {
        SplitPhases(opts::cg::dumpPhases, GetDumpPhases());
    }

    if (opts::cg::target.IsEnabledByUser()) {
        SetTargetMachine(opts::cg::target);
    }

    if (opts::cg::skipPhases.IsEnabledByUser()) {
        SplitPhases(opts::cg::skipPhases, GetSkipPhases());
    }

    if (opts::cg::skipFrom.IsEnabledByUser()) {
        SetSkipFrom(opts::cg::skipFrom);
    }

    if (opts::cg::skipAfter.IsEnabledByUser()) {
        SetSkipAfter(opts::cg::skipAfter);
    }

    if (opts::cg::debugSchedule.IsEnabledByUser()) {
        opts::cg::debugSchedule ? EnableDebugSched() : DisableDebugSched();
    }

    if (opts::cg::bruteforceSchedule.IsEnabledByUser()) {
        opts::cg::bruteforceSchedule ? EnableDruteForceSched() : DisableDruteForceSched();
    }

    if (opts::cg::simulateSchedule.IsEnabledByUser()) {
        opts::cg::simulateSchedule ? EnableSimulateSched() : DisableSimulateSched();
    }

    if (opts::cg::floatAbi.IsEnabledByUser()) {
        SetABIType(opts::cg::floatAbi);
    }

    if (opts::cg::longCalls.IsEnabledByUser()) {
        opts::cg::longCalls ? EnableLongCalls() : DisableLongCalls();
    }

    if (opts::cg::functionSections.IsEnabledByUser()) {
        opts::cg::functionSections ? EnableFunctionSections() : DisableFunctionSections();
    }

    if (opts::cg::omitFramePointer.IsEnabledByUser()) {
        opts::cg::omitFramePointer ? DisableFramePointer() : EnableFramePointer();
    }

    if (opts::cg::fastMath.IsEnabledByUser()) {
        opts::cg::fastMath ? EnableFastMath() : DisableFastMath();
    }

    if (opts::cg::alignAnalysis.IsEnabledByUser()) {
        opts::cg::alignAnalysis ? EnableAlignAnalysis() : DisableAlignAnalysis();
    }

    if (opts::cg::condbrAlign.IsEnabledByUser()) {
        opts::cg::condbrAlign ? EnableCondBrAlign() : DisableCondBrAlign();
    }

    /* big endian can be set with several options: --target, -Be.
     * Triple takes to account all these options and allows to detect big endian with IsBigEndian() interface */
    Triple::GetTriple().IsBigEndian() ? EnableBigEndianInCG() : DisableBigEndianInCG();
    (maple::Triple::GetTriple().GetEnvironment() == Triple::GNUILP32) ? EnableArm64ilp32() : DisableArm64ilp32();

    if (opts::cg::cgSsa.IsEnabledByUser()) {
        opts::cg::cgSsa ? EnableCGSSA() : DisableCGSSA();
    }

    if (opts::cg::common.IsEnabledByUser()) {
        opts::cg::common ? EnableCommon() : DisableCommon();
    }

    if (opts::cg::alignMinBbSize.IsEnabledByUser()) {
        SetAlignMinBBSize(opts::cg::alignMinBbSize);
    }

    if (opts::cg::alignMaxBbSize.IsEnabledByUser()) {
        SetAlignMaxBBSize(opts::cg::alignMaxBbSize);
    }

    if (opts::cg::loopAlignPow.IsEnabledByUser()) {
        SetLoopAlignPow(opts::cg::loopAlignPow);
    }

    if (opts::cg::jumpAlignPow.IsEnabledByUser()) {
        SetJumpAlignPow(opts::cg::jumpAlignPow);
    }

    if (opts::cg::funcAlignPow.IsEnabledByUser()) {
        SetFuncAlignPow(opts::cg::funcAlignPow);
    }

    if (opts::cg::optimizedFrameLayout.IsEnabledByUser()) {
        opts::cg::optimizedFrameLayout ? EnableOptimizedFrameLayout() : DisableOptimizedFrameLayout();
    }

    /* override some options when loc, dwarf is generated */
    if (WithLoc()) {
        DisableSchedule();
        SetOption(kWithSrc);
    }
    if (WithDwarf()) {
        DisableEBO();
        DisableCFGO();
        DisableICO();
        DisableSchedule();
        SetOption(kDebugFriendly);
        SetOption(kWithSrc);
        SetOption(kWithLoc);
        ClearOption(kSuppressFileInfo);
    }

    return true;
}

void CGOptions::ParseExclusiveFunc(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open()) {
        ERR(kLncErr, "%s open failed!", fileName.c_str());
        return;
    }
    std::string content;
    while (file >> content) {
        ehExclusiveFunctionName.push_back(content);
    }
}

void CGOptions::ParseCyclePattern(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open()) {
        ERR(kLncErr, "%s open failed!", fileName.c_str());
        return;
    }
    std::string content;
    std::string classStr("class: ");
    while (getline(file, content)) {
        if (content.compare(0, classStr.length(), classStr) == 0) {
            std::vector<std::string> classPatternContent;
            std::string patternContent;
            while (getline(file, patternContent)) {
                if (patternContent.length() == 0) {
                    break;
                }
                classPatternContent.push_back(patternContent);
            }
            std::string className = content.substr(classStr.length());
            CGOptions::cyclePatternMap[className] = std::move(classPatternContent);
        }
    }
}

void CGOptions::SetRange(const std::string &str, const std::string &cmd, Range &subRange)
{
    const std::string &tmpStr = str;
    size_t comma = tmpStr.find_first_of(",", 0);
    subRange.enable = true;

    if (comma != std::string::npos) {
        subRange.begin = std::stoul(tmpStr.substr(0, comma), nullptr);
        subRange.end = std::stoul(tmpStr.substr(comma + 1, std::string::npos - (comma + 1)), nullptr);
    }
    CHECK_FATAL(range.begin < range.end, "invalid values for %s=%lu,%lu", cmd.c_str(), subRange.begin, subRange.end);
}

/* Set default options according to different languages. */
void CGOptions::SetDefaultOptions(const maple::MIRModule &mod)
{
    insertYieldPoint = GenYieldPoint();
}

void CGOptions::EnableO0()
{
    optimizeLevel = kLevel0;
    doEBO = false;
    doCGSSA = false;
    doLocalSchedule = false;
    doCFGO = false;
    doICO = false;
    doPrePeephole = false;
    doPeephole = false;
    doStoreLoadOpt = false;
    doGlobalOpt = false;
    doPreLSRAOpt = false;
    doPreSchedule = false;
    doSchedule = false;
    doRegSavesOpt = false;
    useSsaPreSave = false;
    useSsuPreRestore = false;
    doWriteRefFieldOpt = false;
    doAlignAnalysis = false;
    doCondBrAlign = false;

    if (maple::Triple::GetTriple().GetEnvironment() == Triple::GNUILP32) {
        ClearOption(kUseStackProtectorStrong);
        ClearOption(kUseStackProtectorAll);
    } else {
        SetOption(kUseStackProtectorStrong);
        SetOption(kUseStackProtectorAll);
    }

    ClearOption(kConstFold);
    ClearOption(kProEpilogueOpt);
    ClearOption(kTailCallOpt);
}

void CGOptions::EnableO1()
{
    optimizeLevel = kLevel1;
    doPreLSRAOpt = true;
    SetOption(kConstFold);
    SetOption(kProEpilogueOpt);
    SetOption(kTailCallOpt);
    ClearOption(kUseStackProtectorStrong);
    ClearOption(kUseStackProtectorAll);
}

void CGOptions::EnableO2()
{
    optimizeLevel = kLevel2;
    doEBO = true;
    doCGSSA = true;
    doLocalSchedule = true;
    doCFGO = true;
    doICO = true;
    doPrePeephole = true;
    doPeephole = true;
    doStoreLoadOpt = true;
    doGlobalOpt = true;
    doPreSchedule = true;
    doSchedule = true;
    doAlignAnalysis = true;
    doCondBrAlign = true;
    SetOption(kConstFold);
    ClearOption(kUseStackProtectorStrong);
    ClearOption(kUseStackProtectorAll);
#if TARGARM32
    doPreLSRAOpt = false;
    doWriteRefFieldOpt = false;
    ClearOption(kProEpilogueOpt);
    ClearOption(kTailCallOpt);
#else
    doPreLSRAOpt = true;
    doRegSavesOpt = false;
    useSsaPreSave = false;
    useSsuPreRestore = true;
    doWriteRefFieldOpt = true;
    SetOption(kProEpilogueOpt);
    SetOption(kTailCallOpt);
#endif
}

void CGOptions::EnableLiteCG()
{
    optimizeLevel = kLevelLiteCG;
    doEBO = false;
    doCGSSA = false;
    doLocalSchedule = false;
    doCGRegCoalesce = false;
    doCFGO = true;
    doICO = false;
    doPrePeephole = false;
    doPeephole = true;
    doStoreLoadOpt = false;
    doGlobalOpt = false;
    doPreLSRAOpt = false;
    doPreSchedule = false;
    doSchedule = false;
    doRegSavesOpt = false;
    useSsaPreSave = false;
    useSsuPreRestore = false;
    doWriteRefFieldOpt = false;
    doAlignAnalysis = false;
    doCondBrAlign = false;
    supportFuncSymbol = true;

    ClearOption(kUseStackProtectorStrong);
    ClearOption(kUseStackProtectorAll);
    ClearOption(kConstFold);
    ClearOption(kProEpilogueOpt);
    ClearOption(kTailCallOpt);
    ClearOption(kDoColorRegAlloc);
    SetOption(kDoLinearScanRegAlloc);
}

void CGOptions::SetTargetMachine(const std::string &str)
{
    // this is a temporary plan, all ilp32 logic follow the same path with aarch64
    if (str == "aarch64" || str == "aarch64_be-linux-gnu_ilp32" || str == "aarch64_be-linux-gnu") {
        targetArch = "aarch64";
    } else if (str == "x86_64") {
        targetArch = "x86_64";
    } else {
        CHECK_FATAL_FALSE("unsupported target!!");
    }
}

void CGOptions::SplitPhases(const std::string &str, std::unordered_set<std::string> &set)
{
    const std::string &tmpStr {str};
    if ((tmpStr.compare("*") == 0) || (tmpStr.compare("cgir") == 0)) {
        (void)set.insert(tmpStr);
        return;
    }
    StringUtils::Split(tmpStr, set, ',');
}

bool CGOptions::DumpPhase(const std::string &phase)
{
    return (IS_STR_IN_SET(dumpPhases, "*") || IS_STR_IN_SET(dumpPhases, "cgir") || IS_STR_IN_SET(dumpPhases, phase));
}

/* match sub std::string of function name */
bool CGOptions::FuncFilter(const std::string &name)
{
    return dumpFunc == "*" || dumpFunc == name;
}
} /* namespace maplebe */
