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
#include "cg_options.h"
#include "triple.h"
#include "option.h"

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
Range CGOptions::spillRanges = Range();
uint64 CGOptions::lsraBBOptSize = 150000;
uint64 CGOptions::lsraInsnOptSize = 200000;
uint64 CGOptions::overlapNum = 28;
bool CGOptions::optForSize = false;
bool CGOptions::enableHotColdSplit = false;
uint32 CGOptions::funcAlignPow = 5;
bool CGOptions::doOptimizedFrameLayout = true;
bool CGOptions::supportFuncSymbol = false;
bool CGOptions::doEBO = false;
bool CGOptions::doCGSSA = false;
bool CGOptions::doLocalSchedule = false;
bool CGOptions::doCGRegCoalesce = false;
bool CGOptions::doIPARA = true;
bool CGOptions::doCFGO = false;
bool CGOptions::doICO = false;
bool CGOptions::doStoreLoadOpt = false;
bool CGOptions::doGlobalOpt = false;
bool CGOptions::doPrePeephole = false;
bool CGOptions::doPeephole = false;
bool CGOptions::doSchedule = false;
bool CGOptions::doWriteRefFieldOpt = false;
bool CGOptions::noDupBB = false;
bool CGOptions::noCalleeCFI = true;
bool CGOptions::lazyBinding = false;
bool CGOptions::hotFix = false;
bool CGOptions::useFramePointer = false;
bool CGOptions::gcOnly = false;
bool CGOptions::doPreSchedule = false;
bool CGOptions::emitBlockMarker = true;
bool CGOptions::inRange = false;
bool CGOptions::doPreLSRAOpt = false;
bool CGOptions::doRegSavesOpt = false;
bool CGOptions::useSsaPreSave = false;
bool CGOptions::useSsuPreRestore = false;
bool CGOptions::generalRegOnly = false;
bool CGOptions::cgBigEndian = false;
bool CGOptions::arm64ilp32 = false;
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


bool CGOptions::SolveOptions()
{
    if (opts::cg::supportFuncSymbol.IsEnabledByUser()) {
        opts::cg::supportFuncSymbol ? EnableSupportFuncSymbol() : DisableSupportFuncSymbol();
    }

    if (opts::cg::verboseAsm.IsEnabledByUser()) {
        opts::cg::verboseAsm ? SetOption(CGOptions::kVerboseAsm) : ClearOption(CGOptions::kVerboseAsm);
        SetAsmEmitterEnable(true);
    }

    if (opts::cg::verboseCg.IsEnabledByUser()) {
        opts::cg::verboseCg ? SetOption(CGOptions::kVerboseCG) : ClearOption(CGOptions::kVerboseCG);
    }

    if (opts::cg::spillRange.IsEnabledByUser()) {
#ifdef ARK_LITECG_DEBUG
        SetRange(opts::cg::spillRange, "--pill-range", GetSpillRanges());
#endif
    }

    if (opts::cg::range.IsEnabledByUser()) {
#ifdef ARK_LITECG_DEBUG
        SetRange(opts::cg::range, "--range", GetRange());
#endif
    }

    if (opts::cg::timePhases.IsEnabledByUser()) {
        opts::cg::timePhases ? EnableTimePhases() : DisableTimePhases();
    }

    if (opts::cg::dumpFunc.IsEnabledByUser()) {
        SetDumpFunc(opts::cg::dumpFunc);
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

    if (opts::cg::withRaLinearScan.IsEnabledByUser()) {
        SetOption(kDoLinearScanRegAlloc);
    }

    if (opts::cg::suppressFileinfo.IsEnabledByUser()) {
        SetOption(kSuppressFileInfo);
    }

    if (opts::cg::dumpCfg.IsEnabledByUser()) {
        SetOption(kDumpCFG);
    }

    if (opts::cg::yieldpoint.IsEnabledByUser()) {
        SetOrClear(GetGenerateFlags(), CGOptions::kGenYieldPoint, opts::cg::yieldpoint);
    }

    if (opts::cg::localRc.IsEnabledByUser()) {
        SetOrClear(GetGenerateFlags(), CGOptions::kGenLocalRc, opts::cg::localRc);
    }

    if (opts::cg::cg.IsEnabledByUser()) {
        SetRunCGFlag(opts::cg::cg);
        opts::cg::cg ? SetOption(CGOptions::kDoCg) : ClearOption(CGOptions::kDoCg);
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

    if (opts::cg::dupBb.IsEnabledByUser()) {
        opts::cg::dupBb ? DisableNoDupBB() : EnableNoDupBB();
    }

    if (opts::cg::calleeCfi.IsEnabledByUser()) {
        opts::cg::calleeCfi ? DisableNoCalleeCFI() : EnableNoCalleeCFI();
    }

    if (opts::cg::tailcall.IsEnabledByUser()) {
        opts::cg::tailcall ? SetOption(CGOptions::kTailCallOpt) : ClearOption(CGOptions::kTailCallOpt);
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

    if (opts::cg::dumpPhases.IsEnabledByUser()) {
#ifdef ARK_LITECG_DEBUG
        SplitPhases(opts::cg::dumpPhases, GetDumpPhases());
#endif
    }

    if (opts::cg::target.IsEnabledByUser()) {
#ifdef ARK_LITECG_DEBUG
        SetTargetMachine(opts::cg::target);
#endif
    }

    if (opts::cg::skipPhases.IsEnabledByUser()) {
#ifdef ARK_LITECG_DEBUG
        SplitPhases(opts::cg::skipPhases, GetSkipPhases());
#endif
    }

    if (opts::cg::skipFrom.IsEnabledByUser()) {
        SetSkipFrom(opts::cg::skipFrom);
    }

    if (opts::cg::skipAfter.IsEnabledByUser()) {
        SetSkipAfter(opts::cg::skipAfter);
    }

    if (opts::cg::omitFramePointer.IsEnabledByUser()) {
        opts::cg::omitFramePointer ? DisableFramePointer() : EnableFramePointer();
    }

    /* big endian can be set with several options: --target, -Be.
     * Triple takes to account all these options and allows to detect big endian with IsBigEndian() interface */
    Triple::GetTriple().IsBigEndian() ? EnableBigEndianInCG() : DisableBigEndianInCG();
    (maple::Triple::GetTriple().GetEnvironment() == Triple::GNUILP32) ? EnableArm64ilp32() : DisableArm64ilp32();

    if (opts::cg::cgSsa.IsEnabledByUser()) {
        opts::cg::cgSsa ? EnableCGSSA() : DisableCGSSA();
    }

    if (opts::cg::funcAlignPow.IsEnabledByUser()) {
        SetFuncAlignPow(opts::cg::funcAlignPow);
    }

    /* override some options when loc, dwarf is generated */
    if (WithLoc()) {
        DisableSchedule();
        SetOption(kWithSrc);
    }

    return true;
}

#ifdef ARK_LITECG_DEBUG
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

    ClearOption(kTailCallOpt);
}

void CGOptions::EnableO1()
{
    optimizeLevel = kLevel1;
    doPreLSRAOpt = true;
    SetOption(kTailCallOpt);
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
#if TARGARM32
    doPreLSRAOpt = false;
    doWriteRefFieldOpt = false;
    ClearOption(kTailCallOpt);
#else
    doPreLSRAOpt = true;
    doRegSavesOpt = false;
    useSsaPreSave = false;
    useSsuPreRestore = true;
    doWriteRefFieldOpt = true;
    SetOption(kTailCallOpt);
#endif
}
#endif

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
    supportFuncSymbol = true;

    ClearOption(kTailCallOpt);
    SetOption(kDoLinearScanRegAlloc);
}

#ifdef ARK_LITECG_DEBUG
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
#endif

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
