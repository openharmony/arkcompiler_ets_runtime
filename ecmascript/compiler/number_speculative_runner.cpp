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

#include "ecmascript/compiler/array_bounds_check_elimination.h"
#include "ecmascript/compiler/number_speculative_runner.h"
#include "ecmascript/compiler/range_analysis.h"

namespace panda::ecmascript::kungfu {
void NumberSpeculativeRunner::Run()
{
    CombinedPassVisitor rangeGuardVisitor(circuit_, enableLog_, methodName_, chunk_);
    RangeGuard rangeGuard(circuit_, &rangeGuardVisitor, chunk_);
    rangeGuardVisitor.AddPass(&rangeGuard);
    rangeGuardVisitor.VisitGraph();

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

    if (enableArrayBoundsCheckElimination_) {
        ArrayBoundsCheckElimination arrayBoundsCheck(circuit_, enableLog_, methodName_, chunk_);
        arrayBoundsCheck.Run();
        if (IsLogEnabled()) {
            LOG_COMPILER(INFO) << "";
            LOG_COMPILER(INFO) << "\033[34m"
                            << "===================="
                            << " After array bounds check elimination "
                            << "[" << GetMethodName() << "]"
                            << "===================="
                            << "\033[0m";
            circuit_->PrintAllGatesWithBytecode();
            LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
        }
    }

    maxId = circuit_->GetMaxGateId();
    typeInfos_.resize(maxId + 1, TypeInfo::NONE);

    // visit gate in RPO, propagate use infos and
    // reset the machine type of number operator gate and related phi,
    // if some tagged phi is used as native value, change it to native phi.
    NumberSpeculativeRetype retype(circuit_, chunk_, typeInfos_);
    CombinedPassVisitor retypeVisitor(circuit_, enableLog_, methodName_, chunk_);
    NumberSpeculativeRetypeManager retypePhase(circuit_, &retypeVisitor, chunk_,
                                               &retype, NumberSpeculativeRetype::State::Retype);
    retypeVisitor.AddPass(&retypePhase);
    retypeVisitor.VisitGraph();
    CombinedPassVisitor convertVisitor(circuit_, enableLog_, methodName_, chunk_);
    NumberSpeculativeRetypeManager convertPhase(circuit_, &convertVisitor,
                                                chunk_, &retype, NumberSpeculativeRetype::State::Convert);
    convertVisitor.AddPass(&convertPhase);
    convertVisitor.VisitGraph();

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
    typeInfos_.resize(maxId + 1, TypeInfo::NONE);
    CombinedPassVisitor rangeAnalysisVisitor(circuit_, enableLog_, methodName_, chunk_);
    RangeAnalysis rangeAnalysis(circuit_, &rangeAnalysisVisitor, chunk_, typeInfos_, rangeInfos_);
    rangeAnalysisVisitor.AddPass(&rangeAnalysis);
    rangeAnalysisVisitor.VisitGraph();

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
