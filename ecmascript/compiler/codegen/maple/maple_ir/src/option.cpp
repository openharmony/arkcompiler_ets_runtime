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

#include "option.h"
#include <iostream>
#include <cstring>
#include <cctype>
#include "driver_options.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "mpl2mpl_options.h"
#include "triple.h"

namespace maple {

bool Options::dumpBefore = false;
bool Options::dumpAfter = false;
std::string Options::dumpPhase = "";
std::string Options::dumpFunc = "*";
std::string Options::skipPhase;
std::string Options::skipFrom;
std::string Options::skipAfter;
bool Options::quiet = false;
bool Options::regNativeFunc = false;
bool Options::nativeWrapper = true;  // Enabled by default
bool Options::inlineWithProfile = false;
bool Options::useInline = true;  // Enabled by default
bool Options::enableIPAClone = true;
bool Options::useCrossModuleInline = true;  // Enabled by default
std::string Options::noInlineFuncList = "";
std::string Options::importFileList = "";
uint32 Options::inlineSmallFunctionThreshold = 80;  // Only for srcLangC, value will be reset later for other srcLang
uint32 Options::inlineHotFunctionThreshold = 100;   // Only for srcLangC, value will be reset later for other srcLang
uint32 Options::inlineRecursiveFunctionThreshold = 15;
uint32 Options::inlineDepth = 8;
uint32 Options::inlineModuleGrowth = 10;
uint32 Options::inlineColdFunctionThreshold = 3;
uint32 Options::profileHotCount = 1000;
uint32 Options::profileColdCount = 10;
uint32 Options::numOfCloneVersions = 2;
uint32 Options::numOfImpExprLowBound = 2;
uint32 Options::numOfImpExprHighBound = 5;
uint32 Options::numOfCallSiteLowBound = 2;
uint32 Options::numOfCallSiteUpBound = 10;
uint32 Options::numOfConstpropValue = 2;
bool Options::profileHotCountSeted = false;
bool Options::profileColdCountSeted = false;
uint32 Options::profileHotRate = 500000;
uint32 Options::profileColdRate = 900000;
bool Options::regNativeDynamicOnly = false;
std::string Options::staticBindingList;
bool Options::usePreg = false;
bool Options::mapleLinker = false;
bool Options::dumpMuidFile = false;
bool Options::emitVtableImpl = false;
#if MIR_JAVA
bool Options::skipVirtualMethod = false;
#endif
bool Options::profileGen = false;
bool Options::profileUse = false;
bool Options::genLMBC = false;

// Ready to be deleted.
bool Options::noRC = false;
bool Options::analyzeCtor = true;
bool Options::strictNaiveRC = false;
bool Options::gcOnly = false;
bool Options::bigEndian = false;
bool Options::rcOpt1 = true;
bool Options::nativeOpt = true;
bool Options::optForSize = false;
bool Options::O2 = false;
bool Options::noDot = false;
bool Options::genIRProfile = false;
bool Options::profileTest = false;
std::string Options::criticalNativeFile = "maple/mrt/codetricks/profile.pv/criticalNative.list";
std::string Options::fastNativeFile = "maple/mrt/codetricks/profile.pv/fastNative.list";
bool Options::barrier = false;
std::string Options::nativeFuncPropertyFile = "maple/mrt/codetricks/native_binding/native_func_property.list";
bool Options::mapleLinkerTransformLocal = true;
bool Options::partialAot = false;
uint32 Options::decoupleInit = 0;
uint32 Options::buildApp = kNoDecouple;
std::string Options::sourceMuid = "";
bool Options::decoupleSuper = false;
bool Options::deferredVisit = false;
bool Options::deferredVisit2 = false;
bool Options::genVtabAndItabForDecouple = false;
bool Options::profileFunc = false;
uint32 Options::parserOpt = 0;
std::string Options::dumpDevirtualList = "";
std::string Options::readDevirtualList = "";
bool Options::usePreloadedClass = false;
std::string Options::profile = "";
std::string Options::appPackageName = "";
bool Options::profileStaticFields = false;
std::string Options::proFileData = "";
std::string Options::proFileFuncData = "";
std::string Options::proFileClassData = "";
std::string Options::classLoaderInvocationList = "";  // maple/mrt/codetricks/profile.pv/classloaderInvocation.list
bool Options::dumpClassLoaderInvocation = false;
unsigned int Options::warningLevel = 0;
bool Options::lazyBinding = false;
bool Options::hotFix = false;
bool Options::compactMeta = false;
bool Options::genPGOReport = false;
bool Options::verify = false;
uint32 Options::inlineCache = 0;
bool Options::checkArrayStore = false;
bool Options::noComment = false;
bool Options::rmNoUseFunc = true;  // default remove no-used static function
bool Options::sideEffect = true;
bool Options::dumpIPA = false;
bool Options::wpaa = false;  // whole program alias analysis

Options &Options::GetInstance()
{
    static Options instance;
    return instance;
}

void Options::DecideMpl2MplRealLevel() const
{
    if (opts::mpl2mpl::o2 || opts::mpl2mpl::os) {
        if (opts::mpl2mpl::os) {
            optForSize = true;
        }
        O2 = true;
        usePreg = true;
    }
}

bool Options::SolveOptions(bool isDebug) const
{
    DecideMpl2MplRealLevel();

    for (const auto &opt : mpl2mplCategory.GetEnabledOptions()) {
        std::string printOpt;
        if (isDebug) {
            for (const auto &val : opt->GetRawValues()) {
                printOpt += opt->GetName() + " " + val + " ";
            }
            LogInfo::MapleLogger() << "mpl2mpl options: " << printOpt << '\n';
        }
    }

    maplecl::CopyIfEnabled(dumpBefore, opts::mpl2mpl::dumpBefore);
    maplecl::CopyIfEnabled(dumpAfter, opts::mpl2mpl::dumpAfter);
    maplecl::CopyIfEnabled(dumpFunc, opts::mpl2mpl::dumpFunc);

    // quiet can be set by verbose option in driver
    maplecl::CopyIfEnabled(quiet, !opts::verbose, opts::verbose);
    maplecl::CopyIfEnabled(quiet, opts::mpl2mpl::quiet);

    maplecl::CopyIfEnabled(dumpPhase, opts::mpl2mpl::dumpPhase);
    maplecl::CopyIfEnabled(skipPhase, opts::mpl2mpl::skipPhase);
    maplecl::CopyIfEnabled(skipFrom, opts::mpl2mpl::skipFrom);
    maplecl::CopyIfEnabled(skipAfter, opts::mpl2mpl::skipAfter);
    maplecl::CopyIfEnabled(regNativeDynamicOnly, opts::mpl2mpl::regnativeDynamicOnly);
    maplecl::CopyIfEnabled(staticBindingList, opts::mpl2mpl::staticBindingList);
    maplecl::CopyIfEnabled(regNativeFunc, opts::mpl2mpl::regnativefunc);
    maplecl::CopyIfEnabled(nativeWrapper, opts::mpl2mpl::nativewrapper);
    maplecl::CopyIfEnabled(inlineWithProfile, opts::mpl2mpl::inlineWithProfile);
    maplecl::CopyIfEnabled(useInline, opts::mpl2mpl::inlineOpt);
    maplecl::CopyIfEnabled(enableIPAClone, opts::mpl2mpl::ipaClone);
    maplecl::CopyIfEnabled(noInlineFuncList, opts::mpl2mpl::noInlineFunc);
    maplecl::CopyIfEnabled(importFileList, opts::mpl2mpl::importFileList);
    maplecl::CopyIfEnabled(numOfCloneVersions, opts::mpl2mpl::numOfCloneVersions);
    maplecl::CopyIfEnabled(numOfImpExprLowBound, opts::mpl2mpl::numOfImpExprLowBound);
    maplecl::CopyIfEnabled(numOfImpExprHighBound, opts::mpl2mpl::numOfImpExprHighBound);
    maplecl::CopyIfEnabled(numOfCallSiteLowBound, opts::mpl2mpl::numOfCallSiteLowBound);
    maplecl::CopyIfEnabled(numOfCallSiteUpBound, opts::mpl2mpl::numOfCallSiteUpBound);
    maplecl::CopyIfEnabled(numOfConstpropValue, opts::mpl2mpl::numOfConstpropValue);
    maplecl::CopyIfEnabled(useCrossModuleInline, opts::mpl2mpl::crossModuleInline);
    maplecl::CopyIfEnabled(inlineSmallFunctionThreshold, opts::mpl2mpl::inlineSmallFunctionThreshold);
    maplecl::CopyIfEnabled(inlineHotFunctionThreshold, opts::mpl2mpl::inlineHotFunctionThreshold);
    maplecl::CopyIfEnabled(inlineRecursiveFunctionThreshold, opts::mpl2mpl::inlineRecursiveFunctionThreshold);
    maplecl::CopyIfEnabled(inlineDepth, opts::mpl2mpl::inlineDepth);
    maplecl::CopyIfEnabled(inlineModuleGrowth, opts::mpl2mpl::inlineModuleGrow);
    maplecl::CopyIfEnabled(inlineColdFunctionThreshold, opts::mpl2mpl::inlineColdFuncThresh);
    maplecl::CopyIfEnabled(profileHotCount, opts::mpl2mpl::profileHotCount);
    maplecl::CopyIfEnabled(profileColdCount, opts::mpl2mpl::profileColdCount);
    maplecl::CopyIfEnabled(profileHotRate, opts::mpl2mpl::profileHotRate);
    maplecl::CopyIfEnabled(profileColdRate, opts::mpl2mpl::profileColdRate);

    if (opts::mpl2mpl::maplelinker) {
        mapleLinker = opts::mpl2mpl::maplelinker;
        dumpMuidFile = mapleLinker;  // quiet is overwrited by maplelinker option
        if (isDebug) {
            LogInfo::MapleLogger() << "--sub options: dumpMuidFile " << dumpMuidFile << '\n';
        }
    }

    maplecl::CopyIfEnabled(dumpMuidFile, opts::mpl2mpl::dumpMuid);
    maplecl::CopyIfEnabled(emitVtableImpl, opts::mpl2mpl::emitVtableImpl);

#if MIR_JAVA
    maplecl::CopyIfEnabled(skipVirtualMethod, opts::mpl2mpl::skipvirtual);
#endif

    maplecl::CopyIfEnabled(noRC, !opts::mpl2mpl::userc, opts::mpl2mpl::userc);
    maplecl::CopyIfEnabled(strictNaiveRC, opts::mpl2mpl::strictNaiveRc);

    /* big endian can be set with several options: --target, -Be.
     * Triple takes to account all these options and allows to detect big endian with IsBigEndian() interface */
    if (Triple::GetTriple().IsBigEndian()) {
        bigEndian = true;
    }

    maplecl::CopyIfEnabled(rcOpt1, opts::mpl2mpl::rcOpt1);
    maplecl::CopyIfEnabled(nativeOpt, opts::mpl2mpl::nativeopt);
    maplecl::CopyIfEnabled(criticalNativeFile, opts::mpl2mpl::criticalNative);
    maplecl::CopyIfEnabled(fastNativeFile, opts::mpl2mpl::fastNative);
    maplecl::CopyIfEnabled(noDot, opts::mpl2mpl::nodot);
    maplecl::CopyIfEnabled(genIRProfile, opts::mpl2mpl::genIrProfile);
    maplecl::CopyIfEnabled(profileTest, opts::mpl2mpl::profileTest);
    maplecl::CopyIfEnabled(barrier, opts::mpl2mpl::barrier);
    maplecl::CopyIfEnabled(nativeFuncPropertyFile, opts::mpl2mpl::nativeFuncPropertyFile);

    maplecl::CopyIfEnabled(mapleLinkerTransformLocal, !opts::mpl2mpl::maplelinkerNolocal,
                           opts::mpl2mpl::maplelinkerNolocal);
    maplecl::CopyIfEnabled(deferredVisit, opts::mpl2mpl::deferredVisit);

    if (opts::mpl2mpl::deferredVisit.IsEnabledByUser()) {
        deferredVisit = true;
        deferredVisit2 = true;
        if (isDebug) {
            LogInfo::MapleLogger() << "--sub options: deferredVisit " << deferredVisit << '\n';
            LogInfo::MapleLogger() << "--sub options: deferredVisit2 " << deferredVisit2 << '\n';
        }
    }

    maplecl::CopyIfEnabled(decoupleSuper, opts::mpl2mpl::decoupleSuper);

    if (opts::mpl2mpl::buildApp.IsEnabledByUser()) {
        if (opts::mpl2mpl::buildApp != kMpl2MplLevelZero && opts::mpl2mpl::buildApp != kMpl2MplLevelOne &&
            opts::mpl2mpl::buildApp != kMpl2MplLevelTwo) {
            LogInfo::MapleLogger(kLlErr) << "expecting 0,1,2 or empty for --build-app\n";
            return false;
        }
        /* Default buildApp value is 1, so if an User does not select buildApp (with --buildApp=2), it will be 1 */
        buildApp = opts::mpl2mpl::buildApp;
    }

    maplecl::CopyIfEnabled(sourceMuid, opts::mpl2mpl::sourceMuid);

    maplecl::CopyIfEnabled(partialAot, opts::mpl2mpl::partialAot);

    if (opts::mpl2mpl::decoupleInit.IsEnabledByUser()) {
        if (opts::mpl2mpl::decoupleInit != kNoDecouple && opts::mpl2mpl::decoupleInit != kConservativeDecouple &&
            opts::mpl2mpl::decoupleInit != kAggressiveDecouple) {
            std::cerr << "expecting 0,1,2 or empty for --decouple-init\n";
            return false;
        }

        /* Default opts::mpl2mpl::decoupleInit value is Options::kConservativeDecouple==1
         * so if an User does not select buildApp (with --decouple-init=2), it will be 1.
         */
        decoupleInit = opts::mpl2mpl::decoupleInit;
        buildApp = Options::kConservativeDecouple;
    }

    maplecl::CopyIfEnabled(genVtabAndItabForDecouple, opts::mpl2mpl::genDecoupleVtab);
    maplecl::CopyIfEnabled(profileFunc, opts::mpl2mpl::profileFunc);
    maplecl::CopyIfEnabled(dumpDevirtualList, opts::mpl2mpl::dumpDevirtual);
    maplecl::CopyIfEnabled(readDevirtualList, opts::mpl2mpl::readDevirtual);
    maplecl::CopyIfEnabled(usePreloadedClass, opts::mpl2mpl::usewhiteclass);

    if (opts::profileGen.IsEnabledByUser()) {
        if (O2) {
            LogInfo::MapleLogger(kLlErr) << "profileGen requires no optimization\n";
            return false;
        } else {
            profileGen = true;
        }
    }

    maplecl::CopyIfEnabled(profileUse, opts::profileUse);
    maplecl::CopyIfEnabled(genLMBC, opts::genLMBC);
    maplecl::CopyIfEnabled(appPackageName, opts::mpl2mpl::appPackageName);
    maplecl::CopyIfEnabled(classLoaderInvocationList, opts::mpl2mpl::checkClInvocation);
    maplecl::CopyIfEnabled(dumpClassLoaderInvocation, opts::mpl2mpl::dumpClInvocation);
    maplecl::CopyIfEnabled(warningLevel, opts::mpl2mpl::warning);
    maplecl::CopyIfEnabled(lazyBinding, opts::mpl2mpl::lazyBinding);
    maplecl::CopyIfEnabled(hotFix, opts::mpl2mpl::hotFix);
    maplecl::CopyIfEnabled(compactMeta, opts::mpl2mpl::compactMeta);
    maplecl::CopyIfEnabled(genPGOReport, opts::mpl2mpl::genPGOReport);

    if (opts::mpl2mpl::inlineCache.IsEnabledByUser()) {
        if (opts::mpl2mpl::inlineCache != 0 && opts::mpl2mpl::inlineCache != 1 &&
            opts::mpl2mpl::inlineCache != 2 && opts::mpl2mpl::inlineCache != 3) { // expecting 0, 1, 2, 3
            LogInfo::MapleLogger(kLlErr) << "expecting 0,1,2,3 or empty for --inlineCache\n";
            return false;
        }

        inlineCache = opts::mpl2mpl::inlineCache;
    }

    maplecl::CopyIfEnabled(noComment, opts::mpl2mpl::noComment);
    maplecl::CopyIfEnabled(rmNoUseFunc, opts::mpl2mpl::rmnousefunc);
    maplecl::CopyIfEnabled(sideEffect, opts::mpl2mpl::sideeffect);
    maplecl::CopyIfEnabled(dumpIPA, opts::mpl2mpl::dumpIPA);
    maplecl::CopyIfEnabled(wpaa, opts::mpl2mpl::wpaa);

    return true;
}

bool Options::ParseOptions(int argc, char **argv, std::string &fileName) const
{
    (void)maplecl::CommandLine::GetCommandLine().Parse(argc, static_cast<char **>(argv), mpl2mplCategory);
    bool result = SolveOptions(false);
    if (!result) {
        return result;
    }

    auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;
    int inputFileCount = 0;
    for (auto &arg : badArgs) {
        if (FileUtils::IsFileExists(arg.first)) {
            inputFileCount++;
            fileName = arg.first;
        } else {
            LogInfo::MapleLogger(kLlErr) << "Unknown Option: " << arg.first;
            return false;
        }
    }

    if (inputFileCount != 1) {
        LogInfo::MapleLogger(kLlErr) << "expecting one .mpl file as last argument, found: ";
        for (const auto &optionArg : badArgs) {
            LogInfo::MapleLogger(kLlErr) << optionArg.first << " ";
        }
        LogInfo::MapleLogger(kLlErr) << "\n";
        return false;
    }

#ifdef DEBUG_OPTION
    LogInfo::MapleLogger() << "mpl file : " << fileName << "\t";
    DumpOptions();
#endif

    return true;
}

void Options::DumpOptions() const
{
    LogInfo::MapleLogger() << "phase sequence : \t";
    if (phaseSeq.empty()) {
        LogInfo::MapleLogger() << "default phase sequence\n";
    } else {
        for (size_t i = 0; i < phaseSeq.size(); ++i) {
            LogInfo::MapleLogger() << " " << phaseSeq[i];
        }
    }
    LogInfo::MapleLogger() << "\n";
}
};  // namespace maple
