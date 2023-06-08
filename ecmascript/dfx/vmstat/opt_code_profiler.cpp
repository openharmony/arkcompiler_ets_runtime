/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include <iomanip>
#include "ecmascript/base/config.h"

namespace panda::ecmascript {
using EcmaOpcode = kungfu::EcmaOpcode;

void OptCodeProfiler::PrintAndReset()
{
#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
    std::vector<std::pair<EcmaOpcode, Value>> profVec;
    for (auto it = profMap_.begin(); it != profMap_.end(); it++) {
        profVec.emplace_back(std::make_pair(it->first, it->second));
        it->second.ResetStat();
    }
    std::sort(profVec.begin(), profVec.end(),
              [](std::pair<EcmaOpcode, Value> &x, std::pair<EcmaOpcode, Value> &y) -> bool {
                  return x.second.Count() > y.second.Count();
              });

    LOG_ECMA(INFO) << "Runtime Statistics of optimized code path:";
    static constexpr int nameRightAdjustment = 46;
    static constexpr int numberRightAdjustment = 15;
    static constexpr int hundred = 100;
    LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << "Bytecode"
                   << std::setw(numberRightAdjustment) << "Count"
                   << std::setw(numberRightAdjustment) << "TypedPathCount"
                   << std::setw(numberRightAdjustment) << "SlowPathCount"
                   << std::setw(numberRightAdjustment + 1) << "TypedPathRate";
    LOG_ECMA(INFO) << "============================================================"
                   << "=========================================================";

    uint64_t totalCount = 0;
    uint64_t totalTypedPathCount = 0;
    uint64_t totalSlowPathCount = 0;

    for (auto it = profVec.begin(); it != profVec.end(); it++) {
        Value val = it->second;
        if (val.Count() == 0) {
            break;
        }

        LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << kungfu::GetEcmaOpcodeStr(it->first)
                       << std::setw(numberRightAdjustment) << val.Count()
                       << std::setw(numberRightAdjustment) << val.TypedPathCount()
                       << std::setw(numberRightAdjustment) << val.SlowPathCount()
                       << std::setw(numberRightAdjustment) << val.TypedPathCount() * hundred / val.Count() << "%";

        totalCount += val.Count();
        totalTypedPathCount += val.TypedPathCount();
        totalSlowPathCount += val.SlowPathCount();
    }

    if (totalCount != 0) {
        LOG_ECMA(INFO) << "------------------------------------------------------------"
                       << "---------------------------------------------------------";
        LOG_ECMA(INFO) << std::right << std::setw(nameRightAdjustment) << "Total"
                       << std::setw(numberRightAdjustment) << totalCount
                       << std::setw(numberRightAdjustment) << totalTypedPathCount
                       << std::setw(numberRightAdjustment) << totalSlowPathCount
                       << std::setw(numberRightAdjustment) << totalTypedPathCount * hundred / totalCount << "%";
    }
#endif
}

OptCodeProfiler::~OptCodeProfiler()
{
    PrintAndReset();
}
} // namespace panda::ecmascript