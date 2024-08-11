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
#include <fstream>
#include <cstring>
#include <cctype>
#include "driver_options.h"
#include "mpl_logging.h"
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
