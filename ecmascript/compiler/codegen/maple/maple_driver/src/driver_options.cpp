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
#include "cl_option.h"

namespace opts {

/* ##################### BOOL Options ############################################################### */

maplecl::Option<bool> verbose({"-verbose"}, "  -verbose                    \tPrint informations\n",
                              {driverCategory, jbc2mplCategory, hir2mplCategory, meCategory, mpl2mplCategory,
                               cgCategory});

maplecl::Option<bool> genLMBC({"--genlmbc"}, "  --genlmbc                \tGenerate .lmbc file\n",
                              {driverCategory, mpl2mplCategory});

maplecl::Option<bool> profileGen({"--profileGen"},
                                 "  --profileGen                \tGenerate profile data for static languages\n",
                                 {driverCategory, meCategory, mpl2mplCategory, cgCategory});

maplecl::Option<bool> profileUse({"--profileUse"},
                                 "  --profileUse                \tOptimize static languages with profile data\n",
                                 {driverCategory, mpl2mplCategory});

} /* namespace opts */
