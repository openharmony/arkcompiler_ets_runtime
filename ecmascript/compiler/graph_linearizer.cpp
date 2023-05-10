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

#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/compiler/scheduler.h"

namespace panda::ecmascript::kungfu {
void GraphLinearizer::Run(ControlFlowGraph &result)
{
    LinearizeGraph();
    LinearizeRegions(result);
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After graph linearizer "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        PrintGraph("Build Basic Block");
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

class CFGBuilder {
public:
    explicit CFGBuilder(GraphLinearizer *linearizer)
        : linearizer_(linearizer), pendingList_(linearizer->chunk_),
        acc_(linearizer->circuit_) {}

    void Run()
    {
        VisitStateGates();
        // connect region
        for (auto rootGate : linearizer_->regionRootList_) {
            auto toRegion = linearizer_->GateToRegion(rootGate);
            auto numStateIn = acc_.GetStateCount(rootGate);
            for (size_t i = 0; i < numStateIn; i++) {
                auto input = acc_.GetState(rootGate, i);
                ASSERT(acc_.IsState(input) || acc_.GetOpCode(input) == OpCode::STATE_ENTRY);
                auto fromRegion = linearizer_->GateToRegion(input);
                fromRegion->AddSucc(toRegion);
            }
        }
    }

    void VisitStateGates()
    {
        ASSERT(pendingList_.empty());
        linearizer_->circuit_->AdvanceTime();
        auto startGate = acc_.GetStateRoot();
        acc_.SetMark(startGate, MarkCode::VISITED);
        pendingList_.emplace_back(startGate);

        while (!pendingList_.empty()) {
            auto curGate = pendingList_.back();
            pendingList_.pop_back();
            linearizer_->CreateGateRegion(curGate);
            if (acc_.GetOpCode(curGate) != OpCode::LOOP_BACK) {
                auto uses = acc_.Uses(curGate);
                for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                    if (acc_.IsStateIn(useIt) && acc_.IsState(*useIt) && acc_.GetMark(*useIt) == MarkCode::NO_MARK) {
                        acc_.SetMark(*useIt, MarkCode::VISITED);
                        pendingList_.emplace_back(*useIt);
                    }
                }
            }
        }
    }

private:
    GraphLinearizer* linearizer_;
    ChunkDeque<GateRef> pendingList_;
    GateAccessor acc_;
};

class ImmediateDominatorsGenerator {
public:
    explicit ImmediateDominatorsGenerator(GraphLinearizer *linearizer, Chunk* chunk, size_t size)
        : linearizer_(linearizer), pendingList_(chunk),
        dfsList_(chunk), dfsTimestamp_(size, chunk), dfsFatherIdx_(size, chunk),
        bbDfsTimestampToIdx_(size, chunk), semiDom_(size, chunk), immDom_(size, chunk),
        minIdx_(size, chunk), parentIdx_(size, chunk), semiDomTree_(chunk) {}

    void Run()
    {
        BuildDfsFather();
        BuildDomTree();
        BuildImmediateDominator();
        BuildImmediateDominatorDepth();
    }

    void BuildDfsFather()
    {
        size_t timestamp = 0;
        auto entry = linearizer_->regionList_.front();
        entry->SetVisited();
        ASSERT(pendingList_.empty());
        pendingList_.emplace_back(entry);
        while (!pendingList_.empty()) {
            auto curRegion = pendingList_.back();
            pendingList_.pop_back();
            dfsList_.emplace_back(curRegion->id_);
            dfsTimestamp_[curRegion->id_] = timestamp++;
            for (auto succ : curRegion->succs_) {
                if (!succ->IsVisited()) {
                    succ->SetVisited();
                    pendingList_.emplace_back(succ);
                    dfsFatherIdx_[succ->id_] = dfsTimestamp_[curRegion->id_];
                }
            }
        }
        for (size_t idx = 0; idx < dfsList_.size(); idx++) {
            bbDfsTimestampToIdx_[dfsList_[idx]] = idx;
        }
        ASSERT(timestamp == linearizer_->regionList_.size());
    }

    size_t UnionFind(size_t idx)
    {
        size_t pIdx = parentIdx_[idx];
        if (pIdx == idx) {
            return idx;
        }
        size_t unionFindSetRoot = UnionFind(pIdx);
        if (semiDom_[minIdx_[idx]] > semiDom_[minIdx_[pIdx]]) {
            minIdx_[idx] = minIdx_[pIdx];
        }
        return parentIdx_[idx] = unionFindSetRoot;
    }

    void Merge(size_t fatherIdx, size_t sonIdx)
    {
        size_t parentFatherIdx = UnionFind(fatherIdx);
        size_t parentSonIdx = UnionFind(sonIdx);
        parentIdx_[parentSonIdx] = parentFatherIdx;
    }

    void BuildDomTree()
    {
        auto &regionList = linearizer_->regionList_;
        for (size_t i = 0; i < dfsList_.size(); i++) {
            ChunkDeque<size_t> sonList(linearizer_->chunk_);
            semiDomTree_.emplace_back(std::move(sonList));
        }
        std::iota(parentIdx_.begin(), parentIdx_.end(), 0);
        std::iota(semiDom_.begin(), semiDom_.end(), 0);
        semiDom_[0] = semiDom_.size();

        for (size_t idx = dfsList_.size() - 1; idx >= 1; idx--) {
            for (const auto &preRegion : regionList[dfsList_[idx]]->preds_) {
                size_t preRegionIdx = bbDfsTimestampToIdx_[preRegion->id_];
                if (bbDfsTimestampToIdx_[preRegion->id_] < idx) {
                    semiDom_[idx] = std::min(semiDom_[idx], preRegionIdx);
                } else {
                    UnionFind(preRegionIdx);
                    semiDom_[idx] = std::min(semiDom_[idx], semiDom_[minIdx_[preRegionIdx]]);
                }
            }
            for (const auto &succDomIdx : semiDomTree_[idx]) {
                UnionFind(succDomIdx);
                if (idx == semiDom_[minIdx_[succDomIdx]]) {
                    immDom_[succDomIdx] = idx;
                } else {
                    immDom_[succDomIdx] = minIdx_[succDomIdx];
                }
            }
            minIdx_[idx] = idx;
            Merge(dfsFatherIdx_[dfsList_[idx]], idx);
            semiDomTree_[semiDom_[idx]].emplace_back(idx);
        }
    }

    void BuildImmediateDominator()
    {
        for (size_t idx = 1; idx < dfsList_.size(); idx++) {
            if (immDom_[idx] != semiDom_[idx]) {
                immDom_[idx] = immDom_[immDom_[idx]];
            }
        }
        auto entry = linearizer_->regionList_.front();
        entry->iDominator_ = entry;
        entry->depth_ = 0;
        auto &regionList = linearizer_->regionList_;
        for (size_t i = 1; i < immDom_.size(); i++) {
            auto index = dfsList_[i];
            auto dominatedRegion = regionList[index];
            auto domIndex = dfsList_[immDom_[i]];
            auto immDomRegion = regionList[domIndex];
            immDomRegion->depth_ = immDom_[i];
            dominatedRegion->iDominator_ = immDomRegion;
            immDomRegion->dominatedRegions_.emplace_back(dominatedRegion);
        }
    }

    void BuildImmediateDominatorDepth()
    {
        auto entry = linearizer_->regionList_.front();
        entry->depth_ = 0;

        ASSERT(pendingList_.empty());
        pendingList_.emplace_back(entry);
        while (!pendingList_.empty()) {
            auto curRegion = pendingList_.back();
            pendingList_.pop_back();

            for (auto succ : curRegion->dominatedRegions_) {
                ASSERT(succ->iDominator_->depth_ != GateRegion::INVALID_DEPTH);
                succ->depth_ = succ->iDominator_->depth_ + 1;
                pendingList_.emplace_back(succ);
            }
        }
    }

private:
    GraphLinearizer* linearizer_;
    ChunkDeque<GateRegion*> pendingList_;
    ChunkVector<size_t> dfsList_;
    ChunkVector<size_t> dfsTimestamp_;
    ChunkVector<size_t> dfsFatherIdx_;
    ChunkVector<size_t> bbDfsTimestampToIdx_;
    ChunkVector<size_t> semiDom_;
    ChunkVector<size_t> immDom_;
    ChunkVector<size_t> minIdx_;
    ChunkVector<size_t> parentIdx_;
    ChunkVector<ChunkDeque<size_t>> semiDomTree_;
};

class GateScheduler {
public:
    explicit GateScheduler(GraphLinearizer *linearizer)
        : linearizer_(linearizer), fixedGateList_(linearizer->chunk_),
        pendingList_(linearizer->chunk_), acc_(linearizer->circuit_),
        scheduleUpperBound_(false) {}

    void InitializeFixedGate()
    {
        auto &regionRoots = linearizer_->regionRootList_;
        auto size = regionRoots.size();
        for (size_t i = 0; i < size; i++) {
            auto fixedGate = regionRoots[i];
            auto region = linearizer_->GateToRegion(fixedGate);
            // fixed Gate's output
            auto uses = acc_.Uses(fixedGate);
            for (auto it = uses.begin(); it != uses.end(); it++) {
                GateRef succGate = *it;
                if (acc_.IsStateIn(it) && acc_.IsFixed(succGate)) {
                    linearizer_->AddFixedGateToRegion(succGate, region);
                    fixedGateList_.emplace_back(succGate);
                }
            }
        }
    }

    void Prepare()
    {
        InitializeFixedGate();
        auto &regionRoots = linearizer_->regionRootList_;
        ASSERT(pendingList_.empty());
        for (const auto rootGate : regionRoots) {
            pendingList_.emplace_back(rootGate);
        }
        while (!pendingList_.empty()) {
            auto curGate = pendingList_.back();
            pendingList_.pop_back();
            auto numIns = acc_.GetNumIns(curGate);
            for (size_t i = 0; i < numIns; i++) {
                VisitPreparedGate(Edge(curGate, i));
            }
        }
    }

    void ScheduleUpperBound()
    {
        if (!scheduleUpperBound_) {
            return;
        }
        auto &regionRoots = linearizer_->regionRootList_;
        ASSERT(pendingList_.empty());
        for (const auto rootGate : regionRoots) {
            pendingList_.emplace_back(rootGate);
        }
        while (!pendingList_.empty()) {
            auto curGate = pendingList_.back();
            pendingList_.pop_back();
            auto uses = acc_.Uses(curGate);
            for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                VisitUpperBoundGate(useIt.GetEdge());
            }
        }
    }

    void VisitUpperBoundGate(Edge edge)
    {
        GateRef succGate = edge.GetGate();
        auto& succInfo = linearizer_->GetGateInfo(succGate);
        if (!succInfo.IsSchedulable()) {
            return;
        }
        ASSERT(succInfo.upperBound != nullptr);
        auto curGate = acc_.GetIn(succGate, edge.GetIndex());
        auto curUpperBound = linearizer_->GateToUpperBound(curGate);
        ASSERT(IsInSameDominatorChain(curUpperBound, succInfo.upperBound));
        if (curUpperBound->depth_ > succInfo.upperBound->depth_) {
            succInfo.upperBound = curUpperBound;
            pendingList_.emplace_back(succGate);
        }
    }

    void ScheduleFloatingGate()
    {
        auto &regionRoots = linearizer_->regionRootList_;
        for (const auto rootGate : regionRoots) {
            auto ins = acc_.Ins(rootGate);
            for (auto it = ins.begin(); it != ins.end(); it++) {
                pendingList_.emplace_back(*it);
                while (!pendingList_.empty()) {
                    auto curGate = pendingList_.back();
                    pendingList_.pop_back();
                    VisitScheduleGate(curGate);
                }
            }
        }
    }

    void VisitPreparedGate(Edge edge)
    {
        auto curGate = edge.GetGate();
        auto prevGate = acc_.GetIn(curGate, edge.GetIndex());
        if (!acc_.IsSchedulable(prevGate)) {
            return;
        }
        auto& prevInfo = linearizer_->GetGateInfo(prevGate);
        if (prevInfo.IsNone()) {
            if (scheduleUpperBound_) {
                prevInfo.upperBound = linearizer_->GetEntryRegion();
            }
            ASSERT(prevInfo.schedulableUseCount == 0);
            prevInfo.state_ = GraphLinearizer::ScheduleState::SCHEDELABLE;
            pendingList_.emplace_back(prevGate);
        }
        auto& curInfo = linearizer_->GetGateInfo(curGate);
        if (curInfo.IsSchedulable()) {
            prevInfo.schedulableUseCount++;
        }
    }

    void VisitScheduleGate(GateRef curGate)
    {
        auto& curInfo = linearizer_->GetGateInfo(curGate);
        if (!curInfo.IsSchedulable() ||
            (curInfo.schedulableUseCount != 0) || (curInfo.region != nullptr)) {
            return;
        }
        auto region = GetCommonDominatorOfAllUses(curGate);
        if (scheduleUpperBound_) {
            ASSERT(curInfo.upperBound != nullptr);
            ASSERT(GetCommonDominator(region, curInfo.upperBound) == curInfo.upperBound);
            region = curInfo.upperBound;
        }
        ScheduleGate(curGate, region);
    }

    void ScheduleGate(GateRef gate, GateRegion* region)
    {
        auto ins = acc_.Ins(gate);
        for (auto it = ins.begin(); it != ins.end(); it++) {
            auto inputGate = *it;
            auto& inputInfo = linearizer_->GetGateInfo(inputGate);
            if (!inputInfo.IsSchedulable()) {
                continue;
            }
            inputInfo.schedulableUseCount--;
            if (inputInfo.schedulableUseCount == 0) {
                pendingList_.emplace_back(inputGate);
            }
        }
        ASSERT(!linearizer_->IsScheduled(gate));
        linearizer_->ScheduleGate(gate, region);
    }

    GateRegion* GetCommonDominatorOfAllUses(GateRef curGate)
    {
        GateRegion* region = nullptr;
        auto uses = acc_.Uses(curGate);
        for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
            GateRef useGate = *useIt;
            auto& useInfo = linearizer_->GetGateInfo(useGate);
            if (useInfo.IsNone()) {
                continue;
            }
            GateRegion* useRegion = useInfo.region;
            if (useInfo.IsSelector()) {
                GateRef state = acc_.GetState(useGate);
                useGate = acc_.GetIn(state, useIt.GetIndex() - 1); // -1: for state
                useRegion = linearizer_->GateToRegion(useGate);
            }

            if (region == nullptr) {
                region = useRegion;
            } else {
                ASSERT(useRegion != nullptr);
                region = GetCommonDominator(region, useRegion);
            }
        }
        return region;
    }

    GateRegion* GetCommonDominator(GateRegion* left, GateRegion* right) const
    {
        while (left != right) {
            if (left->depth_ < right->depth_) {
                right = right->iDominator_;
            } else {
                left = left->iDominator_;
            }
        }
        return left;
    }

    bool IsInSameDominatorChain(GateRegion* left, GateRegion* right) const
    {
        auto dom = GetCommonDominator(left, right);
        return left == dom || right == dom;
    }

    void ScheduleFixedGate()
    {
        for (auto gate : fixedGateList_) {
            GateRegion* region = linearizer_->GateToRegion(gate);
            linearizer_->ScheduleGate(gate, region);
        }
#ifndef NDEBUG
        Verify();
#endif
    }

    void Verify()
    {
        std::vector<GateRef> gateList;
        linearizer_->circuit_->GetAllGates(gateList);
        for (const auto &gate : gateList) {
            auto& GateInfo = linearizer_->GetGateInfo(gate);
            if (GateInfo.IsSchedulable()) {
                ASSERT(linearizer_->IsScheduled(gate));
            }
        }
    }

private:
    GraphLinearizer* linearizer_ {nullptr};
    ChunkVector<GateRef> fixedGateList_;
    ChunkDeque<GateRef> pendingList_;
    GateAccessor acc_;
    bool scheduleUpperBound_{false};
};

void GraphLinearizer::LinearizeGraph()
{
    gateIdToGateInfo_.resize(circuit_->GetMaxGateId() + 1, GateInfo{nullptr}); // 1: max + 1 = size
    CFGBuilder builder(this);
    builder.Run();
    ImmediateDominatorsGenerator generator(this, chunk_, regionList_.size());
    generator.Run();
    GateScheduler scheduler(this);
    scheduler.Prepare();
    scheduler.ScheduleUpperBound();
    scheduler.ScheduleFloatingGate();
    scheduler.ScheduleFixedGate();
}

void GraphLinearizer::CreateGateRegion(GateRef gate)
{
    ASSERT(GateToRegion(gate) == nullptr);
    auto region = new (chunk_) GateRegion(chunk_);
    region->id_ = regionList_.size();
    regionList_.emplace_back(region);
    if (acc_.GetOpCode(gate) == OpCode::LOOP_BEGIN) {
        loopNumber_++;
        region->stateKind_ = GateRegion::StateKind::LOOP_HEAD;
    }
    AddRootGateToRegion(gate, region);
}

void GraphLinearizer::LinearizeRegions(ControlFlowGraph &result)
{
    ASSERT(result.size() == 0);
    result.resize(regionList_.size());
    auto uses = acc_.Uses(acc_.GetArgRoot());
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        regionList_.front()->gateList_.emplace_back(*useIt);
    }

    for (size_t i = 0; i < regionList_.size(); i++) {
        auto region = regionList_[i];
        auto &gateList = region->gateList_;
        result[i].insert(result[i].end(), gateList.begin(), gateList.end());
    }
}

void GraphLinearizer::PrintGraph(const char* title)
{
    LOG_COMPILER(INFO) << "======================== " << title << " ========================";
    for (size_t i = 0; i < regionList_.size(); i++) {
        auto bb = regionList_[i];
        auto front = bb->gateList_.front();
        auto opcode = acc_.GetOpCode(front);
        LOG_COMPILER(INFO) << "B" << bb->id_ << ": " << "depth: [" << bb->depth_ << "] "
                           << opcode << "(" << acc_.GetId(front) << ") "
                           << "IDom B" << bb->iDominator_->id_;
        std::string log("\tPreds: ");
        for (size_t k = 0; k < bb->preds_.size(); ++k) {
            log += std::to_string(bb->preds_[k]->id_) + ", ";
        }
        LOG_COMPILER(INFO) << log;
        std::string log1("\tSucces: ");
        for (size_t j = 0; j < bb->succs_.size(); j++) {
            log1 += std::to_string(bb->succs_[j]->id_) + ", ";
        }
        LOG_COMPILER(INFO) << log1;
        for (auto it = bb->gateList_.crbegin(); it != bb->gateList_.crend(); it++) {
            acc_.Print(*it);
        }
        LOG_COMPILER(INFO) << "";
    }
}
}  // namespace panda::ecmascript::kungfu
