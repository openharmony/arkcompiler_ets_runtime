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

#include "ecmascript/compiler/base/bit_set.h"
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

    void SetVisited(GateAccessor& acc)
    {
        acc.SetMark(state_, MarkCode::VISITED);
    }

    void SetFinished(GateAccessor& acc)
    {
        acc.SetMark(state_, MarkCode::FINISHED);
    }

    bool IsUnvisited(GateAccessor& acc) const
    {
        return acc.GetMark(state_) == MarkCode::NO_MARK;
    }

    bool IsVisited(GateAccessor& acc) const
    {
        return acc.GetMark(state_) == MarkCode::VISITED;
    }

    bool IsFinished(GateAccessor& acc) const
    {
        return acc.GetMark(state_) == MarkCode::FINISHED;
    }

    bool IsLoopHead() const
    {
        return stateKind_ == StateKind::LOOP_HEAD;
    }

    GateRef GetState() const
    {
        return state_;
    }

    void SetDead()
    {
        stateKind_ = StateKind::DEAD;
    }

    bool IsDead() const
    {
        return stateKind_ == StateKind::DEAD;
    }

    bool IsSimple(GateAccessor *acc) const;

    bool HasComplexOuts() const
    {
        return succs_.size() > 1;
    }

    GateRegion* GetSimpleSuccRegion() const
    {
        if (succs_.size() == 1) {
            GateRegion* dst = succs_[0];
            if (dst->GetPreds().size() == 1) {
                return dst;
            }
        }
        return nullptr;
    }

    void ReplaceSucc(GateRegion* oldSucc, GateRegion* newSucc)
    {
        for (size_t i = 0; i < succs_.size(); i++) {
            if (succs_[i] == oldSucc) {
                succs_[i] = newSucc;
            }
        }
        newSucc->AddPred(this);
    }

    bool RemovePred(GateRegion* removedRegion)
    {
        for (auto it = preds_.begin(); it != preds_.end(); it++) {
            if (*it == removedRegion) {
                preds_.erase(it);
                return true;
            }
        }
        return false;
    }

    void AddPred(GateRegion* r)
    {
        for (auto p : preds_) {
            if (p == r) {
                return;
            }
        }
        preds_.emplace_back(r);
    }

    void AddGates(ChunkVector<GateRef>& gates)
    {
        gateList_.insert(gateList_.end(), gates.begin(), gates.end());
    }

    ChunkVector<GateRef>& GetGates()
    {
        return gateList_;
    }

    ChunkVector<GateRegion*>& GetPreds()
    {
        return preds_;
    }

    size_t GetId() const
    {
        return id_;
    }

    void Clear()
    {
        id_ = 0;
        depth_ = INVALID_DEPTH;
        iDominator_ = nullptr;
        gateList_.clear();
        preds_.clear();
        succs_.clear();
        dominatedRegions_.clear();
    }

    void SetLoopNumber(int32_t loopNumber)
    {
        loopNumber_ = static_cast<int32_t>(loopNumber);
    }

    int32_t GetLoopNumber() const
    {
        return static_cast<int32_t>(loopNumber_);
    }

    bool HasLoopNumber() const
    {
        return loopNumber_ >= 0;
    }

    GateRegion* GetDominator() const
    {
        return iDominator_;
    }

    ChunkVector<GateRegion*>& GetDominatedRegions()
    {
        return dominatedRegions_;
    }

    int32_t GetDepth() const
    {
        return depth_;
    }

    void SetLoopIndex(int32_t loopIndex)
    {
        loopIndex_ = loopIndex;
    }

    int32_t GetLoopIndex() const
    {
        return loopIndex_;
    }

    void SetLoopDepth(int32_t loopDepth)
    {
        loopDepth_ = loopDepth;
    }

    int32_t GetLoopDepth() const
    {
        return loopDepth_;
    }

private:
    enum StateKind {
        BRANCH,
        MERGE,
        LOOP_HEAD,
        OTHER,
        DEAD
    };
    static constexpr int32_t INVALID_DEPTH = -1;
    static constexpr int32_t INVALID_INDEX = -1;
    size_t id_ {0};
    int32_t depth_ {INVALID_DEPTH};
    GateRegion* iDominator_ {nullptr};
    ChunkVector<GateRef> gateList_;
    ChunkVector<GateRegion*> preds_;
    ChunkVector<GateRegion*> succs_;
    ChunkVector<GateRegion*> dominatedRegions_;
    GateRef state_ {Circuit::NullGate()};
    StateKind stateKind_ {StateKind::OTHER};
    int32_t loopNumber_ {INVALID_INDEX};
    int32_t loopIndex_ {INVALID_INDEX};
    int32_t loopDepth_ {INVALID_DEPTH};
    friend class ArrayBoundsCheckElimination;
    friend class CFGBuilder;
    friend class GateScheduler;
    friend class ImmediateDominatorsGenerator;
    friend class LoopInfoBuilder;
    friend class GraphLinearizer;
    friend class StateDependBuilder;
};

class GraphLinearizer {
public:
    using ControlFlowGraph = std::vector<std::vector<GateRef>>;
    struct LoopInfo {
        GateRegion* loopHead {nullptr};
        BitSet* loopBodys {nullptr};
        ChunkVector<GateRegion*>* loopExits {nullptr};
        LoopInfo* outer {nullptr};
        size_t loopIndex {0};
        size_t loopDepth {0};
    };

    GraphLinearizer(Circuit *circuit, bool enableLog, const std::string &name, Chunk *chunk, bool onlyBB = false,
                    bool loopInvariantCodeMotion = false, bool enableLiteCG = false)
        : enableLog_(enableLog), methodName_(name), chunk_(chunk), circuit_(circuit), acc_(circuit),
          gateIdToGateInfo_(chunk), regionList_(chunk), regionRootList_(chunk), loops_(chunk), onlyBB_(onlyBB),
          loopInvariantCodeMotion_(loopInvariantCodeMotion), enableLiteCG_(enableLiteCG)
    {
    }

    void Run(ControlFlowGraph &result);

    GateRef GetStateOfSchedulableGate(GateRef gate) const
    {
        return GateToRegion(gate)->GetState();
    }

    bool enableLiteCG() const
    {
        return enableLiteCG_;
    }

private:
    enum class ScheduleModel { LIR, JS_OPCODE };
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
            return state_ == ScheduleState::FIXED || IsSelector();
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

    bool IsEnableLoopInvariantCodeMotion() const
    {
        return loopInvariantCodeMotion_;
    }

    void LinearizeGraph();
    void LinearizeRegions(ControlFlowGraph &result);
    void CreateGateRegion(GateRef gate);
    GateRegion* FindPredRegion(GateRef input);
    GateRegion* GetCommonDominator(GateRegion* left, GateRegion* right) const;

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
        region->state_ = gate;
        region->AddGate(gate);
        info.state_ = ScheduleState::FIXED;
        regionRootList_.emplace_back(gate);
    }

    void BindGate(GateRef gate, GateRegion* region)
    {
        if (UNLIKELY(region == nullptr)) {
            return;
        }
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

    void Reset()
    {
        gateIdToGateInfo_.clear();
        regionList_.clear();
        regionRootList_.clear();
        scheduleUpperBound_ = false;
        model_ = ScheduleModel::LIR;
        loopNumber_ = 0;
    }

    void EnableScheduleUpperBound()
    {
        scheduleUpperBound_ = true;
    }

    void SetScheduleJSOpcode()
    {
        model_ = ScheduleModel::JS_OPCODE;
    }

    bool IsSchedueLIR() const
    {
        return model_ == ScheduleModel::LIR;
    }

    LoopInfo* GetLoopInfo(GateRegion *region)
    {
        auto loopIndex = region->GetLoopIndex();
        if (loopIndex >= 0) {
            ASSERT(static_cast<size_t>(loopIndex) <= loops_.size());
            return &loops_[loopIndex];
        }
        return nullptr;
    }

    size_t OptimizeCFG();
    size_t OptimizeControls(GateRegion *region);
    void MoveAndClear(GateRegion *from, GateRegion *to);
    void PrintGraph(const char* title);

    bool enableLog_ {false};
    bool scheduleUpperBound_ {false};
    ScheduleModel model_ {ScheduleModel::LIR};
    std::string methodName_;
    Chunk* chunk_ {nullptr};
    Circuit* circuit_ {nullptr};
    size_t loopNumber_ {0};

    GateAccessor acc_;
    ChunkVector<GateInfo> gateIdToGateInfo_;
    ChunkVector<GateRegion*> regionList_;
    ChunkVector<GateRef> regionRootList_;
    ChunkVector<LoopInfo> loops_;

    bool onlyBB_ {false}; // dont schedule
    bool loopInvariantCodeMotion_ {false};
    bool enableLiteCG_ {false};
    friend class ArrayBoundsCheckElimination;
    friend class StringBuilderOptimizer;
    friend class CFGBuilder;
    friend class GateScheduler;
    friend class ImmediateDominatorsGenerator;
    friend class LoopInfoBuilder;
    friend class StateSplitLinearizer;
    friend class InductionVariableAnalysis;
};
};  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GRAPH_LINEARIZER_H
