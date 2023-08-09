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

#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/number_speculative_lowering.h"
#include "ecmascript/compiler/number_speculative_runner.h"
#include "ecmascript/compiler/range_analysis.h"

namespace panda::ecmascript::kungfu {
void NumberSpeculativeRunner::Run()
{
    RangeGuard rangeGuard(circuit_, chunk_);
    rangeGuard.Run();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After range guard "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }

    auto maxId = circuit_->GetMaxGateId();
    typeInfos_.resize(maxId + 1, TypeInfo::NONE);
    NumberSpeculativeRetype retype(circuit_, chunk_, typeInfos_);
    retype.Run();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After number speculative retype "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }

    maxId = circuit_->GetMaxGateId();
    rangeInfos_.resize(maxId + 1, RangeInfo::NONE());
    RangeAnalysis rangeAnalysis(circuit_, chunk_, typeInfos_, rangeInfos_);
    rangeAnalysis.Run();
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After range analysis "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        rangeAnalysis.PrintRangeInfo();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }

    NumberSpeculativeLowering lowering(circuit_, chunk_, typeInfos_, rangeInfos_);
    lowering.Run();
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After number speculative runner "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}
}  // panda::ecmascript::kungfu
