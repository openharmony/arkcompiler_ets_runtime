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

#ifndef MAPLE_ME_INCLUDE_ME_OPTION_H
#define MAPLE_ME_INCLUDE_ME_OPTION_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mpl_options.h"
#include "types_def.h"

namespace maple {
class MeOption {
public:
    static MeOption &GetInstance();

    ~MeOption() = default;

    static bool IsSkipPhase(const std::string &phaseName)
    {
        return !(skipPhases.find(phaseName) == skipPhases.end());
    }

    static std::unordered_set<std::string> dumpPhases;
    static std::unordered_set<std::string> skipPhases;
    static std::string dumpFunc;
    static bool quiet;
    static bool noDot;
    static bool stmtNum;
    static bool rcLowering;
    static bool noRC;
    static bool optForSize;
    static uint8 optLevel;
    static bool enableLFO;
    static bool unifyRets;
    // safety check option begin
    static SafetyCheckMode npeCheckMode;
    static SafetyCheckMode boundaryCheckMode;
};
#ifndef DEBUGFUNC_NEWPM
#define DEBUGFUNC_NEWPM(f)                                                                                    \
    (!MeOption::dumpPhases.empty() && MeOption::dumpPhases.find(PhaseName()) != MeOption::dumpPhases.end() && \
     (MeOption::dumpFunc.compare("*") == 0 || (f).GetName() == MeOption::dumpFunc))
#endif
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_OPTION_H
