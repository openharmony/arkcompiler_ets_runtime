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

#ifndef MAPLE_BE_INCLUDE_CG_OPTIONS_H
#define MAPLE_BE_INCLUDE_CG_OPTIONS_H

#include "cl_option.h"
#include "cl_parser.h"

#include <stdint.h>
#include <string>

namespace opts::cg {

extern maplecl::Option<bool> pie;
extern maplecl::Option<bool> fpic;
extern maplecl::Option<bool> verboseAsm;
extern maplecl::Option<bool> verboseCg;
extern maplecl::Option<bool> maplelinker;
extern maplecl::Option<bool> quiet;
extern maplecl::Option<bool> cg;
extern maplecl::Option<bool> replaceAsm;
extern maplecl::Option<bool> generalRegOnly;
extern maplecl::Option<bool> lazyBinding;
extern maplecl::Option<bool> hotFix;
extern maplecl::Option<bool> ebo;
extern maplecl::Option<bool> cfgo;
extern maplecl::Option<bool> ico;
extern maplecl::Option<bool> storeloadopt;
extern maplecl::Option<bool> globalopt;
extern maplecl::Option<bool> hotcoldsplit;
extern maplecl::Option<bool> prelsra;
extern maplecl::Option<bool> calleeregsPlacement;
extern maplecl::Option<bool> ssapreSave;
extern maplecl::Option<bool> ssupreRestore;
extern maplecl::Option<bool> prepeep;
extern maplecl::Option<bool> peep;
extern maplecl::Option<bool> preschedule;
extern maplecl::Option<bool> schedule;
extern maplecl::Option<bool> retMerge;
extern maplecl::Option<bool> vregRename;
extern maplecl::Option<bool> fullcolor;
extern maplecl::Option<bool> writefieldopt;
extern maplecl::Option<bool> dumpOlog;
extern maplecl::Option<bool> nativeopt;
extern maplecl::Option<bool> objmap;
extern maplecl::Option<bool> yieldpoint;
extern maplecl::Option<bool> proepilogue;
extern maplecl::Option<bool> localRc;
extern maplecl::Option<std::string> insertCall;
extern maplecl::Option<bool> addDebugTrace;
extern maplecl::Option<bool> addFuncProfile;
extern maplecl::Option<std::string> classListFile;
extern maplecl::Option<bool> genCMacroDef;
extern maplecl::Option<bool> genGctibFile;
extern maplecl::Option<bool> stackProtectorStrong;
extern maplecl::Option<bool> stackProtectorAll;
extern maplecl::Option<bool> debug;
extern maplecl::Option<bool> gdwarf;
extern maplecl::Option<bool> gsrc;
extern maplecl::Option<bool> gmixedsrc;
extern maplecl::Option<bool> gmixedasm;
extern maplecl::Option<bool> profile;
extern maplecl::Option<bool> withRaLinearScan;
extern maplecl::Option<bool> withRaGraphColor;
extern maplecl::Option<bool> patchLongBranch;
extern maplecl::Option<bool> constFold;
extern maplecl::Option<std::string> ehExclusiveList;
extern maplecl::Option<bool> o0;
extern maplecl::Option<bool> o1;
extern maplecl::Option<bool> o2;
extern maplecl::Option<bool> os;
extern maplecl::Option<bool> olitecg;
extern maplecl::Option<uint64_t> lsraBb;
extern maplecl::Option<uint64_t> lsraInsn;
extern maplecl::Option<uint64_t> lsraOverlap;
extern maplecl::Option<uint8_t> remat;
extern maplecl::Option<bool> suppressFileinfo;
extern maplecl::Option<bool> dumpCfg;
extern maplecl::Option<std::string> target;
extern maplecl::Option<std::string> dumpPhases;
extern maplecl::Option<std::string> skipPhases;
extern maplecl::Option<std::string> skipFrom;
extern maplecl::Option<std::string> skipAfter;
extern maplecl::Option<std::string> dumpFunc;
extern maplecl::Option<bool> timePhases;
extern maplecl::Option<bool> useBarriersForVolatile;
extern maplecl::Option<std::string> range;
extern maplecl::Option<uint8_t> fastAlloc;
extern maplecl::Option<std::string> spillRange;
extern maplecl::Option<bool> dupBb;
extern maplecl::Option<bool> calleeCfi;
extern maplecl::Option<bool> printFunc;
extern maplecl::Option<std::string> cyclePatternList;
extern maplecl::Option<std::string> duplicateAsmList;
extern maplecl::Option<std::string> duplicateAsmList2;
extern maplecl::Option<std::string> blockMarker;
extern maplecl::Option<bool> soeCheck;
extern maplecl::Option<bool> checkArraystore;
extern maplecl::Option<bool> debugSchedule;
extern maplecl::Option<bool> bruteforceSchedule;
extern maplecl::Option<bool> simulateSchedule;
extern maplecl::Option<bool> crossLoc;
extern maplecl::Option<std::string> floatAbi;
extern maplecl::Option<std::string> archType;
extern maplecl::Option<std::string> filetype;
extern maplecl::Option<bool> longCalls;
extern maplecl::Option<bool> functionSections;
extern maplecl::Option<bool> omitFramePointer;
extern maplecl::Option<bool> fastMath;
extern maplecl::Option<bool> tailcall;
extern maplecl::Option<bool> alignAnalysis;
extern maplecl::Option<bool> cgSsa;
extern maplecl::Option<bool> common;
extern maplecl::Option<bool> condbrAlign;
extern maplecl::Option<uint32_t> alignMinBbSize;
extern maplecl::Option<uint32_t> alignMaxBbSize;
extern maplecl::Option<uint32_t> loopAlignPow;
extern maplecl::Option<uint32_t> jumpAlignPow;
extern maplecl::Option<uint32_t> funcAlignPow;
extern maplecl::Option<bool> optimizedFrameLayout;

}  // namespace opts::cg

#endif /* MAPLE_BE_INCLUDE_CG_OPTIONS_H */
