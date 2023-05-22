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

#include "ecmascript/compiler/state_split_linearizer.h"
#include "ecmascript/compiler/scheduler.h"

namespace panda::ecmascript::kungfu {
void StateSplitLinearizer::Run()
{
    graphLinearizer_.LinearizeGraph();
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " Before state split linearizer "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        graphLinearizer_.PrintGraph("Build Basic Block");
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
    LinearizeStateSplit();
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After state split linearizer "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

void StateSplitLinearizer::VisitGate(GateRef gate)
{
    if (acc_.GetOpCode(gate) == OpCode::CHECK_AND_CONVERT) {
        frameState_ = acc_.FindNearestFrameState(gate);
        lcrLowering_.LowerCheckAndConvert(gate, frameState_);
    }
}

void StateSplitLinearizer::VisitRegion(GateRegion* region)
{
    for (auto gate : region->gateList_) {
        VisitGate(gate);
    }
}

void StateSplitLinearizer::LinearizeStateSplit()
{
    auto entry = graphLinearizer_.regionList_.front();
    circuit_->AdvanceTime();
    entry->SetVisited(acc_);
    ASSERT(pendingList_.empty());
    pendingList_.emplace_back(entry);
    while (!pendingList_.empty()) {
        auto curRegion = pendingList_.back();
        pendingList_.pop_back();
        VisitRegion(curRegion);
        for (auto succ : curRegion->succs_) {
            if (!succ->IsVisited(acc_)) {
                succ->SetVisited(acc_);
                pendingList_.emplace_back(succ);
            }
        }
    }
}

}  // namespace panda::ecmascript::kungfu
