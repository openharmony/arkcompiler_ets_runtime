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

#ifndef MAPLE_IR_INCLUDE_MPL2MPL_OPTION_H
#define MAPLE_IR_INCLUDE_MPL2MPL_OPTION_H

#include "cl_option.h"
#include "cl_parser.h"

#include <stdint.h>
#include <string>

namespace opts::mpl2mpl {

extern maplecl::Option<std::string> dumpPhase;
extern maplecl::Option<std::string> skipPhase;
extern maplecl::Option<std::string> skipFrom;
extern maplecl::Option<std::string> skipAfter;
extern maplecl::Option<std::string> dumpFunc;
extern maplecl::Option<bool> quiet;
extern maplecl::Option<bool> maplelinker;
extern maplecl::Option<bool> regnativefunc;
extern maplecl::Option<bool> inlineWithProfile;
extern maplecl::Option<bool> inlineOpt;
extern maplecl::Option<bool> ipaClone;
extern maplecl::Option<std::string> noInlineFunc;
extern maplecl::Option<std::string> importFileList;
extern maplecl::Option<bool> crossModuleInline;
extern maplecl::Option<uint32_t> inlineSmallFunctionThreshold;
extern maplecl::Option<uint32_t> inlineHotFunctionThreshold;
extern maplecl::Option<uint32_t> inlineRecursiveFunctionThreshold;
extern maplecl::Option<uint32_t> inlineDepth;
extern maplecl::Option<uint32_t> inlineModuleGrow;
extern maplecl::Option<uint32_t> inlineColdFuncThresh;
extern maplecl::Option<uint32_t> profileHotCount;
extern maplecl::Option<uint32_t> profileColdCount;
extern maplecl::Option<uint32_t> profileHotRate;
extern maplecl::Option<uint32_t> profileColdRate;
extern maplecl::Option<bool> nativewrapper;
extern maplecl::Option<bool> regnativeDynamicOnly;
extern maplecl::Option<std::string> staticBindingList;
extern maplecl::Option<bool> dumpBefore;
extern maplecl::Option<bool> dumpAfter;
extern maplecl::Option<bool> dumpMuid;
extern maplecl::Option<bool> emitVtableImpl;

#if MIR_JAVA
extern maplecl::Option<bool> skipvirtual;
#endif

extern maplecl::Option<bool> userc;
extern maplecl::Option<bool> strictNaiveRc;
extern maplecl::Option<bool> rcOpt1;
extern maplecl::Option<bool> nativeopt;
extern maplecl::Option<bool> o0;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> os;
extern maplecl::Option<std::string> criticalNative;
extern maplecl::Option<std::string> fastNative;
extern maplecl::Option<bool> nodot;
extern maplecl::Option<bool> genIrProfile;
extern maplecl::Option<bool> profileTest;
extern maplecl::Option<bool> barrier;
extern maplecl::Option<std::string> nativeFuncPropertyFile;
extern maplecl::Option<bool> maplelinkerNolocal;
extern maplecl::Option<uint32_t> buildApp;
extern maplecl::Option<bool> partialAot;
extern maplecl::Option<uint32_t> decoupleInit;
extern maplecl::Option<std::string> sourceMuid;
extern maplecl::Option<bool> deferredVisit;
extern maplecl::Option<bool> deferredVisit2;
extern maplecl::Option<bool> decoupleSuper;
extern maplecl::Option<bool> genDecoupleVtab;
extern maplecl::Option<bool> profileFunc;
extern maplecl::Option<std::string> dumpDevirtual;
extern maplecl::Option<std::string> readDevirtual;
extern maplecl::Option<bool> usewhiteclass;
extern maplecl::Option<std::string> appPackageName;
extern maplecl::Option<std::string> checkClInvocation;
extern maplecl::Option<bool> dumpClInvocation;
extern maplecl::Option<uint32_t> warning;
extern maplecl::Option<bool> lazyBinding;
extern maplecl::Option<bool> hotFix;
extern maplecl::Option<bool> compactMeta;
extern maplecl::Option<bool> genPGOReport;
extern maplecl::Option<uint32_t> inlineCache;
extern maplecl::Option<bool> noComment;
extern maplecl::Option<bool> rmnousefunc;
extern maplecl::Option<bool> sideeffect;
extern maplecl::Option<bool> dumpIPA;
extern maplecl::Option<bool> wpaa;
extern maplecl::Option<uint32_t> numOfCloneVersions;
extern maplecl::Option<uint32_t> numOfImpExprLowBound;
extern maplecl::Option<uint32_t> numOfImpExprHighBound;
extern maplecl::Option<uint32_t> numOfCallSiteLowBound;
extern maplecl::Option<uint32_t> numOfCallSiteUpBound;
extern maplecl::Option<uint32_t> numOfConstpropValue;

}  // namespace opts::mpl2mpl

#endif /* MAPLE_IR_INCLUDE_MPL2MPL_OPTION_H */
