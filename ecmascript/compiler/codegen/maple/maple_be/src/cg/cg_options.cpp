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

#include "driver_options.h"

#include <stdint.h>
#include <string>

namespace opts::cg {

maplecl::Option<bool> verboseAsm({"--verbose-asm"},
                                 "  --verbose-asm               \tAdd comments to asm output\n"
                                 "  --no-verbose-asm\n",
                                 {cgCategory}, maplecl::DisableWith("--no-verbose-asm"));

maplecl::Option<bool> verboseCg({"--verbose-cg"},
                                "  --verbose-cg               \tAdd comments to cg output\n"
                                "  --no-verbose-cg\n",
                                {cgCategory}, maplecl::DisableWith("--no-verbose-cg"));

maplecl::Option<bool> cg({"--cg"},
                         "  --cg                        \tGenerate the output .s file\n"
                         "  --no-cg\n",
                         {cgCategory}, maplecl::DisableWith("--no-cg"));

maplecl::Option<bool> supportFuncSymbol({"--enable-func-symbol"},
                                "  --enable-func-symbol               \tenable function symbols in symbol table\n"
                                "  --disable-func-symbol\n",
                                {cgCategory}, maplecl::DisableWith("--disable-func-symbol"));

maplecl::Option<bool> generalRegOnly(
    {"--general-reg-only"},
    " --general-reg-only           \tdisable floating-point or Advanced SIMD registers\n"
    " --no-general-reg-only\n",
    {cgCategory}, maplecl::DisableWith("--no-general-reg-only"));

maplecl::Option<bool> lazyBinding({"--lazy-binding"},
                                  "  --lazy-binding              \tBind class symbols lazily[default off]\n",
                                  {cgCategory}, maplecl::DisableWith("--no-lazy-binding"));

maplecl::Option<bool> hotFix({"--hot-fix"},
                             "  --hot-fix                   \tOpen for App hot fix[default off]\n"
                             "  --no-hot-fix\n",
                             {cgCategory}, maplecl::DisableWith("--no-hot-fix"));

maplecl::Option<bool> yieldpoint({"--yieldpoint"},
                                 "  --yieldpoint                \tGenerate yieldpoints [default]\n"
                                 "  --no-yieldpoint\n",
                                 {cgCategory}, maplecl::DisableWith("--no-yieldpoint"));

maplecl::Option<bool> localRc({"--local-rc"},
                              "  --local-rc                  \tHandle Local Stack RC [default]\n"
                              "  --no-local-rc\n",
                              {cgCategory}, maplecl::DisableWith("--no-local-rc"));

maplecl::Option<bool> debug({"-g", "--g"}, "  -g                          \tGenerate debug information\n",
                            {cgCategory});

maplecl::Option<bool> gdwarf({"--gdwarf"}, "  --gdwarf                    \tGenerate dwarf infomation\n", {cgCategory});

maplecl::Option<bool> gsrc(
    {"--gsrc"}, "  --gsrc                      \tUse original source file instead of mpl file for debugging\n",
    {cgCategory});

maplecl::Option<bool> gmixedsrc({"--gmixedsrc"},
                                "  --gmixedsrc"
                                "                 \tUse both original source file and mpl file for debugging\n",
                                {cgCategory});

maplecl::Option<bool> gmixedasm({"--gmixedasm"},
                                "  --gmixedasm"
                                "                 \tComment out both original source file and mpl file for debugging\n",
                                {cgCategory});

maplecl::Option<bool> withRaLinearScan({"--with-ra-linear-scan"},
                                       "  --with-ra-linear-scan       \tDo linear-scan register allocation\n",
                                       {cgCategory});

maplecl::Option<uint64_t> lsraBb({"--lsra-bb"},
                                 "  --lsra-bb=NUM"
                                 "               \tSwitch to spill mode if number of bb in function exceeds NUM\n",
                                 {cgCategory});

maplecl::Option<uint64_t> lsraInsn(
    {"--lsra-insn"},
    "  --lsra-insn=NUM"
    "             \tSwitch to spill mode if number of instructons in function exceeds NUM\n",
    {cgCategory});

maplecl::Option<uint64_t> lsraOverlap({"--lsra-overlap"},
                                      "  --lsra-overlap=NUM          \toverlap NUM to decide pre spill in lsra\n",
                                      {cgCategory});

maplecl::Option<bool> suppressFileinfo({"--suppress-fileinfo"},
                                       "  --suppress-fileinfo         \tFor generating gold files in unit testing\n",
                                       {cgCategory});

maplecl::Option<bool> dumpCfg({"--dump-cfg"}, "  --dump-cfg\n", {cgCategory});

maplecl::Option<std::string> target({"--target"}, "  --target=TARGETMACHINE \t generate code for TARGETMACHINE\n",
                                    {cgCategory}, maplecl::optionalValue);

maplecl::Option<std::string> dumpPhases({"--dump-phases"},
                                        "  --dump-phases=PHASENAME,..."
                                        " \tEnable debug trace for specified phases in the comma separated list\n",
                                        {cgCategory});

maplecl::Option<std::string> skipPhases({"--skip-phases"},
                                        "  --skip-phases=PHASENAME,..."
                                        " \tSkip the phases specified in the comma separated list\n",
                                        {cgCategory});

maplecl::Option<std::string> skipFrom({"--skip-from"},
                                      "  --skip-from=PHASENAME       \tSkip the rest phases from PHASENAME(included)\n",
                                      {cgCategory});

maplecl::Option<std::string> skipAfter(
    {"--skip-after"}, "  --skip-after=PHASENAME      \tSkip the rest phases after PHASENAME(excluded)\n", {cgCategory});

maplecl::Option<std::string> dumpFunc(
    {"--dump-func"},
    "  --dump-func=FUNCNAME"
    "        \tDump/trace only for functions whose names contain FUNCNAME as substring\n"
    "                              \t(can only specify once)\n",
    {cgCategory});

maplecl::Option<bool> timePhases(
    {"--time-phases"},
    "  --time-phases               \tCollect compilation time stats for each phase\n"
    "  --no-time-phases            \tDon't Collect compilation time stats for each phase\n",
    {cgCategory}, maplecl::DisableWith("--no-time-phases"));

maplecl::Option<std::string> range(
    {"--range"}, "  --range=NUM0,NUM1           \tOptimize only functions in the range [NUM0, NUM1]\n", {cgCategory});

maplecl::Option<std::string> spillRange(
    {"--spill_range"}, "  --spill_range=NUM0,NUM1     \tO2 RA spill registers in the range [NUM0, NUM1]\n",
    {cgCategory});

maplecl::Option<bool> dupBb({"--dup-bb"},
                            "  --dup-bb                 \tAllow cfg optimizer to duplicate bb\n"
                            "  --no-dup-bb              \tDon't allow cfg optimizer to duplicate bb\n",
                            {cgCategory}, maplecl::DisableWith("--no-dup-bb"));

maplecl::Option<bool> calleeCfi({"--callee-cfi"},
                                "  --callee-cfi                \tcallee cfi message will be generated\n"
                                "  --no-callee-cfi             \tcallee cfi message will not be generated\n",
                                {cgCategory}, maplecl::DisableWith("--no-callee-cfi"));

maplecl::Option<std::string> blockMarker({"--block-marker"},
                                         "  --block-marker"
                                         "              \tEmit block marker symbols in emitted assembly files\n",
                                         {cgCategory});

maplecl::Option<bool> crossLoc({"--cross-loc"},
                               "  --cross-loc                 \tcross loc insn schedule\n"
                               "  --no-cross-loc\n",
                               {cgCategory}, maplecl::DisableWith("--no-cross-loc"));

maplecl::Option<bool> omitFramePointer({"--omit-frame-pointer", "-fomit-frame-pointer"},
                                       " --omit-frame-pointer          \t do not use frame pointer \n"
                                       " --no-omit-frame-pointer\n",
                                       {cgCategory, driverCategory},
                                       maplecl::DisableEvery({"--no-omit-frame-pointer", "-fno-omit-frame-pointer"}));

maplecl::Option<bool> tailcall({"--tailcall"},
                               "  --tailcall                   \tDo tail call optimization\n"
                               "  --no-tailcall\n",
                               {cgCategory}, maplecl::DisableWith("--no-tailcall"));

maplecl::Option<bool> cgSsa({"--cg-ssa"},
                            "  --cg-ssa                     \tPerform cg ssa\n"
                            "  --no-cg-ssa\n",
                            {cgCategory}, maplecl::DisableWith("--no-cg-ssa"));

maplecl::Option<uint32_t> funcAlignPow(
    {"--func-align-pow"}, " --func-align-pow=NUM           \tO2 func bb align pow (NUM == 0, no func-align)\n",
    {cgCategory});
}  // namespace opts::cg