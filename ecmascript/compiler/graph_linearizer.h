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

#ifndef ECMASCRIPT_COMPILER_GRAPH_LINEARIZER_H
#define ECMASCRIPT_COMPILER_GRAPH_LINEARIZER_H

#include <numeric>

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class GateRegion : public ChunkObject {
public:
    GateRegion(Chunk* chunk) : gateList_(chunk), preds_(chunk),
        succs_(chunk), dominatedRegions_(chunk) {}
    ~GateRegion() = default;

    void AddGate(GateRef gate)
    {
        gateList_.emplace_back(gate);
    }

    void AddSucc(GateRegion *to)
    {
        succs_.emplace_back(to);
        to->preds_.emplace_back(this);
    }

    void SetVisited()
    {
        state_ = VisitState::VISITED;
    }

    bool IsVisited() const
    {
        return state_ == VisitState::VISITED;
    }

    bool IsLoopHead() const
    {
        return stateKind_ == StateKind::LOOP_HEAD;
    }

private:
    enum StateKind {
        BRANCH,
        LOOP_HEAD,
        OTHER,
    };
    static constexpr int32_t INVALID_DEPTH = -1;
    size_t id_ {0};
    int32_t depth_ {INVALID_DEPTH};
    GateRegion* iDominator_ {nullptr};
    ChunkVector<GateRef> gateList_;
    ChunkVector<GateRegion*> preds_;
    ChunkVector<GateRegion*> succs_;
    ChunkVector<GateRegion*> dominatedRegions_;
    VisitState state_ {VisitState::UNVISITED};
    StateKind stateKind_ {StateKind::OTHER};
    friend class CFGBuilder;
    friend class GateScheduler;
    friend class ImmediateDominatorsGenerator;
    friend class GraphLinearizer;
};

class GraphLinearizer {
public:
    using ControlFlowGraph = std::vector<std::vector<GateRef>>;

    GraphLinearizer(Circuit *circuit, bool enableLog, const std::string& name, Chunk* chunk)
        : enableLog_(enableLog), methodName_(name), chunk_(chunk), circuit_(circuit),
        acc_(circuit), gateIdToGateInfo_(chunk),
        regionList_(chunk), regionRootList_(chunk) {}

    void Run(ControlFlowGraph &result);
private:
    enum class ScheduleState { NONE, FIXED, SELECTOR, SCHEDELABLE };
    struct GateInfo {
        GateInfo(GateRegion* region) : region(region) {}
        GateRegion* region {nullptr};
        GateRegion* upperBound {nullptr};
        size_t schedulableUseCount {0};
        ScheduleState state_ {ScheduleState::NONE};

        bool IsSchedulable() const
        {
            return state_ == ScheduleState::SCHEDELABLE;
        }

        bool IsFixed() const
        {
            return state_ == ScheduleState::FIXED;
        }

        bool IsSelector() const
        {
            return state_ == ScheduleState::SELECTOR;
        }

        bool IsNone() const
        {
            return state_ == ScheduleState::NONE;
        }
    };

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void LinearizeGraph();
    void LinearizeRegions(ControlFlowGraph &result);
    void CreateGateRegion(GateRef gate);

    GateInfo& GetGateInfo(GateRef gate)
    {
        auto id = acc_.GetId(gate);
        ASSERT(id < gateIdToGateInfo_.size());
        return gateIdToGateInfo_[id];
    }

    const GateInfo& GetGateInfo(GateRef gate) const
    {
        auto id = acc_.GetId(gate);
        ASSERT(id < gateIdToGateInfo_.size());
        return gateIdToGateInfo_[id];
    }

    GateRegion* GateToRegion(GateRef gate) const
    {
        const GateInfo& info = GetGateInfo(gate);
        return info.region;
    }

    GateRegion* GateToUpperBound(GateRef gate) const
    {
        const GateInfo& info = GetGateInfo(gate);
        return info.upperBound;
    }

    GateRegion* GetEntryRegion() const
    {
        ASSERT(!regionList_.empty());
        return regionList_[0];
    }

    void AddFixedGateToRegion(GateRef gate, GateRegion* region)
    {
        GateInfo& info = GetGateInfo(gate);
        ASSERT(info.upperBound == nullptr);
        info.upperBound = region;
        ASSERT(info.region == nullptr);
        info.region = region;
        if (acc_.GetOpCode(gate) == OpCode::VALUE_SELECTOR ||
            acc_.GetOpCode(gate) == OpCode::DEPEND_SELECTOR) {
            info.state_ = ScheduleState::SELECTOR;
        } else {
            info.state_ = ScheduleState::FIXED;
        }
        regionRootList_.emplace_back(gate);
    }

    void AddRootGateToRegion(GateRef gate, GateRegion* region)
    {
        GateInfo& info = GetGateInfo(gate);
        ASSERT(info.upperBound == nullptr);
        info.upperBound = region;
        ASSERT(info.region == nullptr);
        info.region = region;
        region->AddGate(gate);
        info.state_ = ScheduleState::FIXED;
        regionRootList_.emplace_back(gate);
    }

    void ScheduleGate(GateRef gate, GateRegion* region)
    {
        GateInfo& info = GetGateInfo(gate);
        info.region = region;
        region->AddGate(gate);
    }

    bool IsScheduled(GateRef gate) const
    {
        GateRegion* region = GateToRegion(gate);
        return region != nullptr;
    }

    bool HasLoop() const
    {
        return loopNumber_ != 0;
    }

    void PrintGraph(const char* title);

    bool enableLog_ {false};
    std::string methodName_;
    Chunk* chunk_ {nullptr};
    Circuit* circuit_ {nullptr};
    size_t loopNumber_ {0};

    GateAccessor acc_;
    ChunkVector<GateInfo> gateIdToGateInfo_;
    ChunkVector<GateRegion*> regionList_;
    ChunkDeque<GateRef> regionRootList_;

    friend class CFGBuilder;
    friend class GateScheduler;
    friend class ImmediateDominatorsGenerator;
};
};  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GRAPH_LINEARIZER_H
