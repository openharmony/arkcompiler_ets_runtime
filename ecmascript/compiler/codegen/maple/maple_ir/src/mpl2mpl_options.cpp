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

namespace opts::mpl2mpl {

maplecl::Option<std::string> dumpPhase({"--dump-phase"},
                                       "  --dump-phase                \tEnable debug trace for"
                                       " specified phase (can only specify once)\n"
                                       "                              \t--dump-phase=PHASENAME\n",
                                       {mpl2mplCategory});

maplecl::Option<std::string> skipPhase(
    {"--skip-phase"},
    "  --skip-phase                \tSkip the phase when adding it to phase manager\n"
    "                              \t--skip-phase=PHASENAME\n",
    {mpl2mplCategory});

maplecl::Option<std::string> skipFrom({"--skip-from"},
                                      "  --skip-from                 \tSkip all remaining phases including PHASENAME\n"
                                      "                              \t--skip-from=PHASENAME\n",
                                      {mpl2mplCategory});

maplecl::Option<std::string> skipAfter({"--skip-after"},
                                       "  --skip-after                \tSkip all remaining phases after PHASENAME\n"
                                       "                              \t--skip-after=PHASENAME\n",
                                       {mpl2mplCategory});

maplecl::Option<std::string> dumpFunc({"--dump-func"},
                                      "  --dump-func                 \tDump/trace only for functions"
                                      " whose names contain FUNCNAME as substring\n"
                                      "                              \t(can only specify once)\n"
                                      "                              \t--dump-func=FUNCNAME\n",
                                      {mpl2mplCategory});

maplecl::Option<bool> quiet({"--quiet"},
                            "  --quiet                     \tDisable brief trace messages with phase/function names\n"
                            "  --no-quiet                  \tEnable brief trace messages with phase/function names\n",
                            {mpl2mplCategory}, maplecl::DisableWith("--no-quiet"));

maplecl::Option<bool> maplelinker({"--maplelinker"},
                                  "  --maplelinker               \tGenerate MUID symbol tables and references\n"
                                  "  --no-maplelinker            \tDon't Generate MUID symbol tables and references\n",
                                  {mpl2mplCategory}, maplecl::DisableWith("--no-maplelinker"));

maplecl::Option<bool> regnativefunc({"--regnativefunc"},
                                    "  --regnativefunc             \tGenerate native stub function"
                                    " to support JNI registration and calling\n"
                                    "  --no-regnativefunc          \tDisable regnativefunc\n",
                                    {mpl2mplCategory}, maplecl::DisableWith("--no-regnativefunc"));

maplecl::Option<bool> inlineWithProfile({"--inline-with-profile"},
                                        "  --inline-with-profile       \tEnable profile-based inlining\n"
                                        "  --no-inline-with-profile    \tDisable profile-based inlining\n",
                                        {mpl2mplCategory}, maplecl::DisableWith("--no-inline-with-profile"));

maplecl::Option<bool> inlineOpt({"--inline"},
                                "  --inline                    \tEnable function inlining\n"
                                "  --no-inline                 \tDisable function inlining\n",
                                {mpl2mplCategory}, maplecl::DisableWith("--no-inline"));

maplecl::Option<bool> ipaClone({"--ipa-clone"},
                               "  --ipa-clone                 \tEnable ipa constant_prop and clone\n"
                               "  --no-ipa-clone              \tDisable ipa constant_prop and clone\n",
                               {mpl2mplCategory}, maplecl::DisableWith("--no-ipa-clone"));

maplecl::Option<std::string> noInlineFunc({"--no-inlinefunclist"},
                                          "  --no-inlinefunclist=list    \tDo not inline function in this list\n",
                                          {mpl2mplCategory});

maplecl::Option<std::string> importFileList(
    {"--importfilelist"}, "  --importfilelist=list    \tImport there files to do cross module analysis\n",
    {mpl2mplCategory});

maplecl::Option<bool> crossModuleInline({"--cross-module-inline"},
                                        "  --cross-module-inline       \tEnable cross-module inlining\n"
                                        "  --no-cross-module-inline    \tDisable cross-module inlining\n",
                                        {mpl2mplCategory}, maplecl::DisableWith("--no-cross-module-inline"));

maplecl::Option<uint32_t> inlineSmallFunctionThreshold({"--inline-small-function-threshold"},
                                                       "  --inline-small-function-threshold=15"
                                                       "            \tThreshold for inlining small function\n",
                                                       {mpl2mplCategory});

maplecl::Option<uint32_t> inlineHotFunctionThreshold({"--inline-hot-function-threshold"},
                                                     "  --inline-hot-function-threshold=30"
                                                     "              \tThreshold for inlining hot function\n",
                                                     {mpl2mplCategory});

maplecl::Option<uint32_t> inlineRecursiveFunctionThreshold(
    {"--inline-recursive-function-threshold"},
    "  --inline-recursive-function-threshold=15"
    "              \tThreshold for inlining recursive function\n",
    {mpl2mplCategory});

maplecl::Option<uint32_t> inlineDepth({"--inline-depth"},
                                      "  --inline-depth=8              \tMax call graph depth for inlining\n",
                                      {mpl2mplCategory});

maplecl::Option<uint32_t> inlineModuleGrow({"--inline-module-growth"},
                                           "  --inline-module-growth=100000"
                                           "                   \tThreshold for maxmium code size growth rate (10%)\n",
                                           {mpl2mplCategory});

maplecl::Option<uint32_t> inlineColdFuncThresh({"--inline-cold-function-threshold"},
                                               "  --inline-cold-function-threshold=3"
                                               "              \tThreshold for inlining hot function\n",
                                               {mpl2mplCategory});

maplecl::Option<uint32_t> profileHotCount({"--profile-hot-count"},
                                          "  --profile-hot-count=1000"
                                          "    \tA count is regarded as hot if it exceeds this number\n",
                                          {mpl2mplCategory});

maplecl::Option<uint32_t> profileColdCount({"--profile-cold-count"},
                                           "  --profile-cold-count=10"
                                           "     \tA count is regarded as cold if it is below this number\n",
                                           {mpl2mplCategory});

maplecl::Option<uint32_t> profileHotRate({"--profile-hot-rate"},
                                         "  --profile-hot-rate=500000"
                                         "   \tA count is regarded as hot if it is in the largest 50%\n",
                                         {mpl2mplCategory});

maplecl::Option<uint32_t> profileColdRate({"--profile-cold-rate"},
                                          "  --profile-cold-rate=900000"
                                          "  \tA count is regarded as cold if it is in the smallest 10%\n",
                                          {mpl2mplCategory});

maplecl::Option<bool> nativewrapper({"--nativewrapper"},
                                    "  --nativewrapper             \tGenerate native wrappers [default]\n",
                                    {mpl2mplCategory}, maplecl::DisableWith("--no-nativewrapper"));

maplecl::Option<bool> regnativeDynamicOnly({"--regnative-dynamic-only"},
                                           "  --regnative-dynamic-only    \tOnly Generate dynamic register code,"
                                           " Report Fatal Msg if no implemented\n"
                                           "  --no-regnative-dynamic-only \tDisable regnative-dynamic-only\n",
                                           {mpl2mplCategory}, maplecl::DisableWith("--no-regnative-dynamic-only"));

maplecl::Option<std::string> staticBindingList({"--static-binding-list"},
                                               "  --static-bindig-list        \tOnly Generate static binding"
                                               " function in file configure list\n"
                                               "                              \t--static-bindig-list=file\n",
                                               {mpl2mplCategory});

maplecl::Option<bool> dumpBefore({"--dump-before"},
                                 "  --dump-before               \tDo extra IR dump before the specified phase\n"
                                 "  --no-dump-before            \tDon't extra IR dump before the specified phase\n",
                                 {mpl2mplCategory}, maplecl::DisableWith("--no-dump-before"));

maplecl::Option<bool> dumpAfter({"--dump-after"},
                                "  --dump-after                \tDo extra IR dump after the specified phase\n"
                                "  --no-dump-after             \tDon't extra IR dump after the specified phase\n",
                                {mpl2mplCategory}, maplecl::DisableWith("--no-dump-after"));

maplecl::Option<bool> dumpMuid({"--dump-muid"},
                               "  --dump-muid                 \tDump MUID def information into a .muid file\n"
                               "  --no-dump-muid              \tDon't dump MUID def information into a .muid file\n",
                               {mpl2mplCategory}, maplecl::DisableWith("--no-dump-muid"));

maplecl::Option<bool> emitVtableImpl({"--emitVtableImpl"},
                                     "  --emitVtableImpl            \tgenerate VtableImpl file\n"
                                     "  --no-emitVtableImpl         \tDon't generate VtableImpl file\n",
                                     {mpl2mplCategory}, maplecl::DisableWith("--no-emitVtableImpl"));

maplecl::Option<bool> userc({"--userc"},
                            "  --userc                     \tEnable reference counting [default]\n"
                            "  --no-userc                  \tDisable reference counting [default]\n",
                            {mpl2mplCategory}, maplecl::DisableWith("--no-userc"));

maplecl::Option<bool> strictNaiveRc({"--strict-naiverc"},
                                    "  --strict-naiverc            \tStrict Naive RC mode,"
                                    " assume no unsafe multi-thread read/write racing\n"
                                    "  --no-strict-naiverc         \tDisable strict-naiverc\n",
                                    {mpl2mplCategory}, maplecl::DisableWith("--no-strict-naiverc"));

maplecl::Option<bool> rcOpt1({"--rc-opt1"}, "  --rc-opt1                   \tEnable RC optimization1 [default]\n",
                             {mpl2mplCategory}, maplecl::DisableWith("--no-rc-opt1"));

maplecl::Option<bool> nativeopt({"--nativeopt"},
                                "  --nativeopt                 \tEnable native opt\n"
                                "  --no-nativeopt              \tDisable native opt\n",
                                {mpl2mplCategory}, maplecl::DisableWith("--no-nativeopt"));

maplecl::Option<bool> o0({"-O0", "--O0"}, "  -O0                         \tDo some optimization.\n", {mpl2mplCategory});

maplecl::Option<bool> o2({"-O2", "--O2"}, "  -O2                         \tDo some optimization.\n", {mpl2mplCategory});

maplecl::Option<bool> os({"-Os", "--Os"}, "  -Os                         \tOptimize for size, based on O2.\n",
                         {mpl2mplCategory});

maplecl::Option<std::string> criticalNative({"--CriticalNative"},
                                            "  --CriticalNative            \tFor CriticalNative optimization\n"
                                            "                              \t--CriticalNative=list_file\n",
                                            {mpl2mplCategory});

maplecl::Option<std::string> fastNative({"--FastNative"},
                                        "  --FastNative                \tFor FastNative optimization\n"
                                        "                              \t--FastNative=list_file\n",
                                        {mpl2mplCategory});

maplecl::Option<bool> nodot({"--nodot"},
                            "  --nodot                     \tDisable dot file generation from cfg\n"
                            "  --no-nodot                  \tEnable dot file generation from cfg\n",
                            {mpl2mplCategory}, maplecl::DisableWith("--no-nodot"));

maplecl::Option<bool> genIrProfile({"--ir-profile-gen"},
                                   "  --ir-profile-gen              \tGen IR level Profile\n"
                                   "  --no-ir-profile-gen           \tDisable Gen IR level Profile\n",
                                   {mpl2mplCategory}, maplecl::DisableWith("--no-ir-profile-gen"));

maplecl::Option<bool> profileTest({"--profile-test"},
                                  "  --profile-test              \tprofile test\n"
                                  "  --no-profile-test           \tDisable profile test\n",
                                  {mpl2mplCategory}, maplecl::DisableWith("--no-profile-test"));

maplecl::Option<bool> barrier({"--barrier"},
                              "  --barrier                   \tEnable barrier insertion instead of RC insertion\n"
                              "  --no-barrier                \tDisable barrier insertion instead of RC insertion\n",
                              {mpl2mplCategory}, maplecl::DisableWith("--no-barrier"));

maplecl::Option<std::string> nativeFuncPropertyFile({"--nativefunc-property-list"},
                                                    "  --nativefunc-property-list"
                                                    "  \tGenerate native binding function stub\n"
                                                    "                              \t--nativefunc-property-list=file\n",
                                                    {mpl2mplCategory});

maplecl::Option<bool> maplelinkerNolocal({"--maplelinker-nolocal"},
                                         "  --maplelinker-nolocal       \tDo not turn functions"
                                         " into local when maple linker is on\n"
                                         "  --no-maplelinker-nolocal\n",
                                         {mpl2mplCategory}, maplecl::DisableWith("--no-maplelinker-nolocal"));

maplecl::Option<uint32_t> buildApp({"--build-app"},
                                   "  --build-app[=0,1,2]         \tbuild the app bytecode"
                                   " 0:off, 1:method1, 2:method2, ignore:method1\n",
                                   {mpl2mplCategory}, maplecl::optionalValue, maplecl::Init(1));

maplecl::Option<bool> partialAot({"--partialAot"},
                                 "  --partialAot               \tenerate the detailed information for the partialAot\n",
                                 {mpl2mplCategory}, maplecl::optionalValue);

maplecl::Option<uint32_t> decoupleInit({"--decouple-init"},
                                       "  --decouple-init          \tdecouple the constructor method\n",
                                       {mpl2mplCategory}, maplecl::optionalValue, maplecl::Init(1));

maplecl::Option<std::string> sourceMuid({"--source-muid"},
                                        "  --source-muid="
                                        "            \tWrite the source file muid into the mpl file\n",
                                        {mpl2mplCategory}, maplecl::optionalValue);

maplecl::Option<bool> deferredVisit({"--deferred-visit"},
                                    "  --deferred-visit            \tGenerate deferred MCC call for undefined type\n"
                                    "  --no-deferred-visit         \tDont't generate"
                                    " deferred MCC call for undefined type\n",
                                    {mpl2mplCategory}, maplecl::DisableWith("--no-deferred-visit"));

maplecl::Option<bool> deferredVisit2({"--deferred-visit2"},
                                     "  --deferred-visit2"
                                     "           \tGenerate deferred MCC call(DAI2.0) for undefined type\n"
                                     "  --no-deferred-visit2"
                                     "        \tDon't generate deferred MCC call(DAI2.0) for undefined type\n",
                                     {mpl2mplCategory}, maplecl::DisableWith("--no-deferred-visit2"));

maplecl::Option<bool> decoupleSuper(
    {"--decouple-super"},
    "  --decouple-super            \tGenerate deferred MCC call for undefined type\n"
    "  --no-decouple-super         \tDon't generate deferred MCC call for undefined type\n",
    {mpl2mplCategory}, maplecl::DisableWith("--no-decouple-super"));

maplecl::Option<bool> genDecoupleVtab({"--gen-decouple-vtab"},
                                      "  --gen-decouple-vtab         \tGenerate the whole and complete vtab and itab\n"
                                      "  --no-gen-decouple-vtab"
                                      "      \tDon't generate the whole and complete vtab and itab\n",
                                      {mpl2mplCategory}, maplecl::DisableWith("--no-gen-decouple-vtab"));

maplecl::Option<bool> profileFunc({"--profile-func"},
                                  "  --profile-func              \tProfile function usage\n"
                                  "  --no-profile-func           \tDisable profile function usage\n",
                                  {mpl2mplCategory}, maplecl::DisableWith("--no-profile-func"));

maplecl::Option<std::string> dumpDevirtual({"--dump-devirtual-list"},
                                           "  --dump-devirtual-list"
                                           "       \tDump candidates of devirtualization into a specified file\n"
                                           "                              \t--dump-devirtual-list=\n",
                                           {mpl2mplCategory});

maplecl::Option<std::string> readDevirtual(
    {"--read-devirtual-list"},
    "  --read-devirtual-list       \tRead in candidates of devirtualization from\n"
    "                              \t a specified file and perform devirtualizatin\n"
    "                              \t--read-devirtual-list=\n",
    {mpl2mplCategory});

maplecl::Option<bool> usewhiteclass({"--usewhiteclass"},
                                    "  --usewhiteclass"
                                    "             \tEnable use preloaded class list to reducing clinit check\n"
                                    "  --no-usewhiteclass"
                                    "          \tDisable use preloaded class list to reducing clinit check\n",
                                    {mpl2mplCategory}, maplecl::DisableWith("--no-usewhiteclass"));

maplecl::Option<std::string> appPackageName({"--app-package-name"},
                                            "  --app-package-name          \tSet APP package name\n"
                                            "                              \t--app-package-name=package_name\n",
                                            {mpl2mplCategory}, maplecl::optionalValue);

maplecl::Option<std::string> checkClInvocation({"--check_cl_invocation"},
                                               "  --check_cl_invocation       \tFor classloader invocation checking\n"
                                               "                              \t--check_cl_invocation=list_file\n",
                                               {mpl2mplCategory});

maplecl::Option<bool> dumpClInvocation(
    {"--dump_cl_invocation"},
    "  --dump_cl_invocation        \tFor classloader invocation dumping.\n"
    "                              \tWork only if already set --check_cl_invocation\n"
    "  --no-dump_cl_invocation     \tDisable dump_cl_invocation\n",
    {mpl2mplCategory}, maplecl::DisableWith("--no-dump_cl_invocation"));

maplecl::Option<uint32_t> warning({"--warning"}, "  --warning=level             \t--warning=level\n",
                                  {mpl2mplCategory});

maplecl::Option<bool> lazyBinding({"--lazy-binding"},
                                  "  --lazy-binding              \tBind class symbols lazily[default off]\n"
                                  "  --no-lazy-binding           \tDon't bind class symbols lazily\n",
                                  {mpl2mplCategory}, maplecl::DisableWith("--no-lazy-binding"));

maplecl::Option<bool> hotFix({"--hot-fix"},
                             "  --hot-fix                   \tOpen for App hot fix[default off]\n"
                             "  --no-hot-fix                \tDon't open for App hot fix\n",
                             {mpl2mplCategory}, maplecl::DisableWith("--no-hot-fix"));

maplecl::Option<bool> compactMeta({"--compact-meta"},
                                  "  --compact-meta              \tEnable compact method and field meta\n"
                                  "  --no-compact-meta           \tDisable compact method and field meta\n",
                                  {mpl2mplCategory}, maplecl::DisableWith("--no-compact-meta"));

maplecl::Option<bool> genPGOReport({"--gen-pgo-report"},
                                   "  --gen-pgo-report            \tDisplay pgo report\n"
                                   "  --no-gen-pgo-report\n",
                                   {mpl2mplCategory}, maplecl::DisableWith("--no-gen-pgo-report"));

maplecl::Option<uint32_t> inlineCache({"--inlineCache"}, "  --inlineCache            \tbuild inlineCache 0,1,2,3\n",
                                      {mpl2mplCategory}, maplecl::optionalValue, maplecl::Init(0));

maplecl::Option<bool> noComment({"--no-comment"}, "  --no-comment             \tbuild inlineCache 0:off, 1:open\n",
                                {mpl2mplCategory});

maplecl::Option<bool> rmnousefunc({"--rmnousefunc"},
                                  "  --rmnousefunc            \tEnable remove no-used file-static function\n"
                                  "  --no-rmnousefunc         \tDisable remove no-used file-static function\n",
                                  {mpl2mplCategory}, maplecl::DisableWith("--no-rmnousefunc"));

maplecl::Option<bool> sideeffect({"--sideeffect"},
                                 "  --sideeffect      \tIPA: analysis sideeffect\n"
                                 "  --no-sideeffect          \n",
                                 {mpl2mplCategory}, maplecl::DisableWith("--no-sideeffect"));

maplecl::Option<bool> dumpIPA({"--dump-ipa"},
                              "  --dump-ipa      \tIPA: dump\n"
                              "  --no-dump-ipa          \n",
                              {mpl2mplCategory}, maplecl::DisableWith("--no-dump-ipa"));

maplecl::Option<bool> wpaa({"--wpaa"},
                           "  --wpaa      \tWhole Program Alias Analysis\n"
                           "  --no-wpaa          \n",
                           {mpl2mplCategory}, maplecl::DisableWith("--no-wpaa"));

maplecl::Option<uint32_t> numOfCloneVersions({"--num-of-clone-versions"},
                                             "  --num-of-clone-versions=3        \tnum of clone versions\n",
                                             {mpl2mplCategory});

maplecl::Option<uint32_t> numOfImpExprLowBound({"--num-of-ImpExpr-LowBound"},
                                               "  --num-of-ImpExpr-LowBound=3        \tnum of ImpExpr LowBound\n",
                                               {mpl2mplCategory});

maplecl::Option<uint32_t> numOfImpExprHighBound({"--num-of-ImpExpr-HighBound"},
                                                "  --num-of-ImpExpr-LowBound=3        \tnum of ImpExpr LowBound\n",
                                                {mpl2mplCategory});

maplecl::Option<uint32_t> numOfCallSiteLowBound({"--num-of-CallSite-LowBound"},
                                                "  --num-of-CallSite-LowBound=3        \tnum of CallSite LowBound\n",
                                                {mpl2mplCategory});

maplecl::Option<uint32_t> numOfCallSiteUpBound({"--num-of-CallSite-HighBound"},
                                               "  --num-of-CallSite-HighBound=3        \tnum of CallSite HighBound\n",
                                               {mpl2mplCategory});

maplecl::Option<uint32_t> numOfConstpropValue({"--num-of-ConstProp-value"},
                                              "  --num-of-CallSite-HighBound=3        \tnum of CallSite HighBound\n",
                                              {mpl2mplCategory});

}  // namespace opts::mpl2mpl
