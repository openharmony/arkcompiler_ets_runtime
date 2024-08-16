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
    graphLinearizer_.SetScheduleJSOpcode();
    graphLinearizer_.LinearizeGraph();
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
#ifndef NDEBUG
    graphLinearizer_.Reset();
    graphLinearizer_.EnableScheduleUpperBound();
    graphLinearizer_.SetScheduleJSOpcode();
    graphLinearizer_.LinearizeGraph();
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " verify split linearizer "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        graphLinearizer_.PrintGraph("Build Basic Block");
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
#endif
}

struct RegionEdge {
    GateRef stateOut {Circuit::NullGate()};
    GateRef dependOut {Circuit::NullGate()};
    GateRef frameStateOut {Circuit::NullGate()};
};

struct PendingGateRegionEdge {
    explicit PendingGateRegionEdge(GateRegion* from, GateRegion* to,
        GateRef dependStart, size_t index) : from(from), to(to),
        dependStart(dependStart), index(index) {}

    GateRegion* from;
    GateRegion* to;
    GateRef dependStart;
    size_t index;
};

class RegionStateDependMap {
public:
    explicit RegionStateDependMap(Chunk* chunk) : regionMap_(chunk) {}
    RegionEdge& GetEdge(GateRegion* from, GateRegion* to)
    {
        return regionMap_[std::make_pair(from->GetId(), to->GetId())];
    }
private:
    ChunkMap<std::pair<size_t, size_t>, RegionEdge> regionMap_;
};

class StateDependBuilder {
public:
    explicit StateDependBuilder(StateSplitLinearizer* linearizer, Chunk* chunk)
        : linearizer_(linearizer), pendingList_(chunk),
        acc_(linearizer->circuit_), map_(chunk), pendingEdges_(chunk) {}

    void Run(ChunkVector<GateRegion*>& regionList)
    {
        maxGateId_ = acc_.GetCircuit()->GetMaxGateId();
        replacement_.SetDepend(acc_.GetDependRoot());
        dependStart_ = replacement_.Depend();
        auto entry = regionList.front();
        acc_.GetCircuit()->AdvanceTime();
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
        ConnectPendingRegionEdges();
        DeleteUnusedGates();
    }

    void DeleteUnusedGates()
    {
        std::vector<GateRef> gateList;
        auto circuit = acc_.GetCircuit();
        circuit->GetAllGates(gateList);

        for (const auto &gate : gateList) {
            if (acc_.GetMark(gate) == MarkCode::NO_MARK &&
                !acc_.IsProlog(gate) && !acc_.IsRoot(gate) &&
                acc_.GetId(gate) <= maxGateId_) {
                acc_.DeleteGate(gate);
            }
        }
    }

    void TryFindDependStart(GateRegion* curRegion)
    {
        // 0: is state
        for (; currentIndex_ > 0; currentIndex_--) {
            auto curGate = curRegion->gateList_[currentIndex_];
            auto op = acc_.GetOpCode(curGate);
            if (op == OpCode::DEPEND_SELECTOR || op == OpCode::DEPEND_RELAY) {
                replacement_.SetDepend(curGate);
                dependStart_ = curGate;
                acc_.SetMark(curGate, MarkCode::VISITED);
            } else {
                VisitGate(curGate);
            }
            if (dependStart_ != Circuit::NullGate()) {
                currentIndex_--;
                break;
            }
        }
    }

    void VisitRegion(GateRegion* curRegion)
    {
        ASSERT(curRegion->gateList_.size() > 0);
        replacement_.SetState(curRegion->GetState());
        currentIndex_ = static_cast<int32_t>(curRegion->gateList_.size() - 1); // 1: -1 for size
        TryLoadDependStart(curRegion);
        if (dependStart_ == Circuit::NullGate()) {
            TryFindDependStart(curRegion);
        }
        ConnectStateDepend(curRegion);
        for (; currentIndex_ > 0; currentIndex_--) {
            auto curGate = curRegion->gateList_[currentIndex_];
            VisitGate(curGate);
        }
        StoreStateDepend(curRegion);
    }

    void ProcessStateDepend(GateRef currentGate)
    {
        auto currentState = replacement_.State();
        auto currentDepend = replacement_.Depend();
        if (acc_.GetStateCount(currentGate) > 0) {
            ASSERT(acc_.GetStateCount(currentGate) == 1);
            auto stateInput = acc_.GetState(currentGate);
            if (currentState != stateInput) {
                acc_.ReplaceStateIn(currentGate, currentState);
            }
            if (!acc_.IsVirtualState(currentGate) && !acc_.IsFixed(currentGate)) {
                replacement_.SetState(currentGate);
            }
        }
        if (acc_.GetDependCount(currentGate) > 0) {
            ASSERT(acc_.GetDependCount(currentGate) == 1);
            auto dependInput = acc_.GetDep(currentGate);
            if (currentDepend != dependInput) {
                acc_.ReplaceDependIn(currentGate, currentDepend);
            }
            replacement_.SetDepend(currentGate);
        }
    }

    void VisitGate(GateRef gate)
    {
        acc_.SetMark(gate, MarkCode::VISITED);
        auto& lowering = linearizer_->lcrLowering_;
        auto op = acc_.GetOpCode(gate);
        switch (op) {
            case OpCode::FRAME_STATE:
                frameState_ = gate;
                break;
            case OpCode::STATE_SPLIT:
            case OpCode::DEPEND_RELAY:
                acc_.DeleteGate(gate);
                return;
            case OpCode::CONVERT:
                ASSERT(replacement_.State() != Circuit::NullGate());
                replacement_ = lowering.LowerConvert(replacement_, gate);
                break;
            default:
                break;
        }
        ProcessStateDepend(gate);
    }

    void TryInsertRelay(GateRegion* curRegion)
    {
        auto currentState = curRegion->GetState();
        auto curGate = curRegion->gateList_[currentIndex_];
        if (acc_.GetOpCode(curGate) != OpCode::DEPEND_RELAY ||
            acc_.GetState(curGate) != currentState) {
            auto circuit = acc_.GetCircuit();
            auto dependRelay = circuit->NewGate(
                circuit->DependRelay(), { currentState, Circuit::NullGate() });
            replacement_.SetDepend(dependRelay);
            ASSERT(dependStart_ == Circuit::NullGate());
            acc_.SetMark(dependRelay, MarkCode::VISITED);
            dependStart_ = dependRelay;
        }
    }

    void TryLoadDependStart(GateRegion* curRegion)
    {
        auto currentState = curRegion->GetState();
        if (dependStart_ == Circuit::NullGate()) {
            if (curRegion->preds_.size() == 1) {
                auto &edge = map_.GetEdge(curRegion->preds_[0], curRegion);
                ASSERT(edge.dependOut != Circuit::NullGate());
                replacement_.SetDepend(edge.dependOut);
                frameState_ = edge.frameStateOut;
            }

            auto opcode = acc_.GetOpCode(currentState);
            switch (opcode) {
                case OpCode::IF_EXCEPTION:
                    dependStart_ = currentState;
                    replacement_.SetDepend(dependStart_);
                    break;
                case OpCode::IF_TRUE:
                case OpCode::IF_FALSE:
                    TryInsertRelay(curRegion);
                    break;
                default:
                    break;
            }
        }
    }

    void ConnectStateDepend(GateRegion* curRegion)
    {
        auto currentState = curRegion->GetState();
        ASSERT(dependStart_ == Circuit::NullGate() ||
               curRegion->preds_.size() == acc_.GetDependCount(dependStart_));
        ASSERT(curRegion->preds_.size() == acc_.GetStateCount(currentState));
        for (size_t i = 0; i < curRegion->preds_.size(); i++) {
            auto pred = curRegion->preds_[i];
            auto &edge = map_.GetEdge(pred, curRegion);
            auto stateInput = acc_.GetState(currentState, i);
            if (edge.stateOut == Circuit::NullGate()) {
                ASSERT(edge.dependOut == Circuit::NullGate());
                pendingEdges_.emplace_back(PendingGateRegionEdge(pred, curRegion, dependStart_, i));
            } else {
                ConnectEdge(edge, currentState, stateInput, i);
            }
        }
    }

    void ConnectEdge(RegionEdge& edge, GateRef currentState, GateRef stateInput, size_t i)
    {
        if (edge.stateOut != stateInput) {
            acc_.ReplaceStateIn(currentState, edge.stateOut, i);
        }
        if (frameState_ == Circuit::NullGate()) {
            frameState_ = edge.frameStateOut;
        }
        if (dependStart_ != Circuit::NullGate()) {
            auto dependInput = acc_.GetDep(dependStart_, i);
            if (edge.dependOut != dependInput) {
                acc_.ReplaceOrNewDependIn(dependStart_, edge.dependOut, i);
            }
        }
    }

    void ConnectPendingRegionEdges()
    {
        for (auto regionEdge : pendingEdges_) {
            auto currentState = regionEdge.to->GetState();
            auto stateInput = acc_.GetState(currentState, regionEdge.index);
            auto &edge = map_.GetEdge(regionEdge.from, regionEdge.to);
            if (edge.stateOut != stateInput) {
                acc_.ReplaceStateIn(currentState, edge.stateOut, regionEdge.index);
            }
            auto dependInput = acc_.GetDep(regionEdge.dependStart, regionEdge.index);
            if (edge.dependOut != dependInput) {
                acc_.ReplaceDependIn(regionEdge.dependStart, edge.dependOut, regionEdge.index);
            }
        }
    }

    void StoreStateDepend(GateRegion* curRegion)
    {
        for (auto succ : curRegion->succs_) {
            auto &edge = map_.GetEdge(curRegion, succ);
            if (edge.stateOut == Circuit::NullGate()) {
                edge.stateOut = replacement_.State();
            }
            if (edge.dependOut == Circuit::NullGate()) {
                edge.dependOut = replacement_.Depend();
            }
            if (edge.frameStateOut == Circuit::NullGate()) {
                edge.frameStateOut = frameState_;
            }
        }
        replacement_.Reset();
        frameState_ = Circuit::NullGate();
        dependStart_ = Circuit::NullGate();
        currentIndex_ = -1;
    }

private:
    GateRef dependStart_ {Circuit::NullGate()};
    GateRef frameState_ {Circuit::NullGate()};
    StateDepend replacement_;
    int32_t currentIndex_ {-1};
    StateSplitLinearizer* linearizer_ {nullptr};
    ChunkDeque<GateRegion*> pendingList_;
    GateAccessor acc_;
    RegionStateDependMap map_;
    ChunkVector<PendingGateRegionEdge> pendingEdges_;
    size_t maxGateId_ {0};
};

void StateSplitLinearizer::LinearizeStateSplit()
{
    StateDependBuilder builder(this, graphLinearizer_.chunk_);
    builder.Run(graphLinearizer_.regionList_);
}

}  // namespace panda::ecmascript::kungfu
