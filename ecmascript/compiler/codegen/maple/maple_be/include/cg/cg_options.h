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

extern maplecl::Option<bool> verboseAsm;
extern maplecl::Option<bool> verboseCg;
extern maplecl::Option<bool> cg;
extern maplecl::Option<bool> generalRegOnly;
extern maplecl::Option<bool> lazyBinding;
extern maplecl::Option<bool> hotFix;
extern maplecl::Option<bool> yieldpoint;
extern maplecl::Option<bool> localRc;
extern maplecl::Option<bool> debug;
extern maplecl::Option<bool> gdwarf;
extern maplecl::Option<bool> gsrc;
extern maplecl::Option<bool> gmixedsrc;
extern maplecl::Option<bool> gmixedasm;
extern maplecl::Option<bool> withRaLinearScan;
extern maplecl::Option<uint64_t> lsraBb;
extern maplecl::Option<uint64_t> lsraInsn;
extern maplecl::Option<uint64_t> lsraOverlap;
extern maplecl::Option<bool> suppressFileinfo;
extern maplecl::Option<bool> dumpCfg;
extern maplecl::Option<std::string> target;
extern maplecl::Option<std::string> dumpPhases;
extern maplecl::Option<std::string> skipPhases;
extern maplecl::Option<std::string> skipFrom;
extern maplecl::Option<std::string> skipAfter;
extern maplecl::Option<std::string> dumpFunc;
extern maplecl::Option<bool> timePhases;
extern maplecl::Option<std::string> range;
extern maplecl::Option<std::string> spillRange;
extern maplecl::Option<bool> dupBb;
extern maplecl::Option<bool> calleeCfi;
extern maplecl::Option<std::string> blockMarker;
extern maplecl::Option<bool> crossLoc;
extern maplecl::Option<std::string> archType;
extern maplecl::Option<bool> omitFramePointer;
extern maplecl::Option<bool> tailcall;
extern maplecl::Option<bool> cgSsa;
extern maplecl::Option<uint32_t> funcAlignPow;
extern maplecl::Option<bool> supportFuncSymbol;

}  // namespace opts::cg

#endif /* MAPLE_BE_INCLUDE_CG_OPTIONS_H */
