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
        endStateList_(linearizer->chunk_),
        acc_(linearizer->circuit_), scheduleLIR_(linearizer->IsSchedueLIR()) {}

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
                auto fromRegion = linearizer_->FindPredRegion(input);
                fromRegion->AddSucc(toRegion);
            }
        }
        for (auto fixedGate : endStateList_) {
            auto state = acc_.GetState(fixedGate);
            auto region = linearizer_->FindPredRegion(state);
            linearizer_->AddFixedGateToRegion(fixedGate, region);
            linearizer_->BindGate(fixedGate, region);
        }
    }

    void VisitStateGates()
    {
        ASSERT(pendingList_.empty());
        linearizer_->circuit_->AdvanceTime();
        auto startGate = acc_.GetStateRoot();
        acc_.SetMark(startGate, MarkCode::VISITED);
        pendingList_.emplace_back(startGate);
        bool litecg = linearizer_->enableLiteCG();
        while (!pendingList_.empty()) {
            auto curGate = pendingList_.back();
            pendingList_.pop_back();
            VisitStateGate(curGate);
            if (acc_.GetOpCode(curGate) != OpCode::LOOP_BACK) {
                auto uses = acc_.Uses(curGate);
                // The LiteCG backend is unable to perform branch prediction scheduling, thus requiring advance
                // preparation
                if (litecg && acc_.GetOpCode(curGate) == OpCode::IF_BRANCH && acc_.HasBranchWeight(curGate)) {
                    std::vector<GateRef> outs;
                    acc_.GetOutStates(curGate, outs);
                    GateRef bTrue = (acc_.GetOpCode(outs[0]) == OpCode::IF_TRUE) ? outs[0] : outs[1];
                    GateRef bFalse = (acc_.GetOpCode(outs[0]) == OpCode::IF_FALSE) ? outs[0] : outs[1];
                    if (acc_.GetTrueWeight(curGate) >= acc_.GetFalseWeight(curGate)) {
                        std::swap(bTrue, bFalse);
                    }
                    if (acc_.GetMark(bTrue) == MarkCode::NO_MARK) {
                        acc_.SetMark(bTrue, MarkCode::VISITED);
                        pendingList_.emplace_back(bTrue);
                    }
                    if (acc_.GetMark(bFalse) == MarkCode::NO_MARK) {
                        acc_.SetMark(bFalse, MarkCode::VISITED);
                        pendingList_.emplace_back(bFalse);
                    }
                    continue;
                }
                for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                    if (acc_.IsStateIn(useIt) && acc_.IsState(*useIt) && acc_.GetMark(*useIt) == MarkCode::NO_MARK) {
                        acc_.SetMark(*useIt, MarkCode::VISITED);
                        pendingList_.emplace_back(*useIt);
                    }
                }
            }
        }
    }

    void VisitStateGate(GateRef gate)
    {
        if (scheduleLIR_) {
            linearizer_->CreateGateRegion(gate);
        } else {
            auto op = acc_.GetOpCode(gate);
            switch (op) {
                case OpCode::LOOP_BEGIN:
                case OpCode::MERGE:
                case OpCode::IF_TRUE:
                case OpCode::IF_FALSE:
                case OpCode::SWITCH_CASE:
                case OpCode::STATE_ENTRY:
                case OpCode::IF_EXCEPTION:
                case OpCode::IF_SUCCESS: {
                    linearizer_->CreateGateRegion(gate);
                    if (linearizer_->onlyBB_) {
                        currentRegion_ = linearizer_->GateToRegion(gate);
                    }
                    break;
                }
                case OpCode::LOOP_BACK:
                case OpCode::IF_BRANCH:
                case OpCode::SWITCH_BRANCH:
                case OpCode::RETURN:
                case OpCode::RETURN_VOID:
                case OpCode::TYPED_CONDITION_JUMP:
                    endStateList_.emplace_back(gate);
                    break;
                case OpCode::JS_BYTECODE:
                    if (IsStateSplit(gate)) {
                        endStateList_.emplace_back(gate);
                    }
                    break;
                default: {
                    if (linearizer_->onlyBB_) {
                        auto &info = linearizer_->GetGateInfo(gate);
                        info.region = currentRegion_;
                        linearizer_->BindGate(gate, currentRegion_);
                    }
                    break;
                }
            }
        }
    }

    bool IsStateSplit(GateRef gate)
    {
        size_t stateOut = 0;
        auto uses = acc_.Uses(gate);
        for (auto it = uses.begin(); it != uses.end(); it++) {
            if (acc_.IsStateIn(it)) {
                auto op = acc_.GetOpCode(*it);
                switch (op) {
                    case OpCode::IF_TRUE:
                    case OpCode::IF_FALSE:
                    case OpCode::IF_EXCEPTION:
                    case OpCode::IF_SUCCESS:
                        stateOut++;
                        break;
                    default:
                        break;
                }
            }
        }
        return stateOut > 1; // 1: depend out
    }

private:
    GraphLinearizer* linearizer_;
    ChunkDeque<GateRef> pendingList_;
    ChunkVector<GateRef> endStateList_;
    GateAccessor acc_;
    GateRegion* currentRegion_ {nullptr};
    bool scheduleLIR_;
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
        linearizer_->circuit_->AdvanceTime();
        entry->SetVisited(linearizer_->acc_);
        ASSERT(pendingList_.empty());
        pendingList_.emplace_back(entry);
        while (!pendingList_.empty()) {
            auto curRegion = pendingList_.back();
            pendingList_.pop_back();
            dfsList_.emplace_back(curRegion->id_);
            dfsTimestamp_[curRegion->id_] = timestamp++;
            for (auto succ : curRegion->succs_) {
                if (!succ->IsVisited(linearizer_->acc_)) {
                    succ->SetVisited(linearizer_->acc_);
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
        std::stack<size_t, std::vector<size_t>> allIdxs;
        allIdxs.emplace(idx);
        size_t pIdx = parentIdx_[idx];
        while (pIdx != allIdxs.top()) {
            allIdxs.emplace(pIdx);
            pIdx = parentIdx_[pIdx];
        }
        size_t unionFindSetRoot = pIdx;
        while (!allIdxs.empty()) {
            if (semiDom_[minIdx_[allIdxs.top()]] > semiDom_[minIdx_[pIdx]]) {
                minIdx_[allIdxs.top()] = minIdx_[pIdx];
            }
            parentIdx_[allIdxs.top()] = unionFindSetRoot;
            pIdx = allIdxs.top();
            allIdxs.pop();
        }
        return unionFindSetRoot;
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

        ASSERT(dfsList_.size() > 0);
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
            immDomRegion->depth_ = static_cast<int32_t>(immDom_[i]);
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

class LoopInfoBuilder {
public:
    explicit LoopInfoBuilder(GraphLinearizer *linearizer, Chunk* chunk)
        : linearizer_(linearizer), pendingList_(chunk),
        loopbacks_(chunk), chunk_(chunk),
        dfsStack_(chunk), acc_(linearizer->circuit_) {}

    void Run()
    {
        ComputeLoopNumber();
        ComputeLoopInfo();
        ComputeLoopTree();
        if (linearizer_->IsLogEnabled()) {
            for (size_t i = 0; i < numLoops_; i++) {
                auto& loopInfo = linearizer_->loops_[i];
                PrintLoop(loopInfo);
            }
        }
    }

    void PrintLoop(GraphLinearizer::LoopInfo& loopInfo)
    {
        auto size = linearizer_->regionList_.size();
        LOG_COMPILER(INFO) << "--------------------------------- LoopInfo Start ---------------------------------";
        LOG_COMPILER(INFO) << "Head: " << acc_.GetId(loopInfo.loopHead->state_);
        LOG_COMPILER(INFO) << "loopNumber: " << loopInfo.loopHead->loopNumber_;
        LOG_COMPILER(INFO) << "Body: [";
        for (size_t i = 0; i < size; i++) {
            if (loopInfo.loopBodys->TestBit(i)) {
                auto current = linearizer_->regionList_.at(i)->state_;
                LOG_COMPILER(INFO) << acc_.GetId(current) << ", ";
            }
        }
        LOG_COMPILER(INFO) << "]";
        LOG_COMPILER(INFO) << "Exit: [";
        if (loopInfo.loopExits != nullptr) {
            for (auto region : *loopInfo.loopExits) {
                auto current = region->state_;
                LOG_COMPILER(INFO) << acc_.GetId(current) << ", ";
            }
        }
        LOG_COMPILER(INFO) << "]";
        LOG_COMPILER(INFO) << "--------------------------------- LoopInfo End ---------------------------------";
    }

    void ComputeLoopInfo()
    {
        linearizer_->loops_.clear();
        auto size = linearizer_->regionList_.size();
        linearizer_->loops_.resize(numLoops_, GraphLinearizer::LoopInfo());

        for (auto curState : loopbacks_) {
            GateRegion* curRegion = curState.region;
            GateRegion* loopHead = curRegion->succs_[curState.index];
            auto loopNumber = loopHead->GetLoopNumber();
            auto& loopInfo = linearizer_->loops_[loopNumber];

            if (loopInfo.loopHead == nullptr) {
                loopInfo.loopHead = loopHead;
                loopInfo.loopBodys = chunk_->New<BitSet>(chunk_, size);
                loopInfo.loopIndex = static_cast<size_t>(loopNumber);
                loopInfo.loopHead->loopIndex_ = static_cast<int32_t>(loopInfo.loopIndex);
            }
            if (curRegion != loopHead) {
                loopInfo.loopBodys->SetBit(curRegion->GetId());
                pendingList_.emplace_back(curRegion);
            }
            PropagateLoopBody(loopInfo);
        }
    }

    void PropagateLoopBody(GraphLinearizer::LoopInfo& loopInfo)
    {
        while (!pendingList_.empty()) {
            auto curRegion = pendingList_.back();
            pendingList_.pop_back();
            for (auto pred : curRegion->preds_) {
                if (pred != loopInfo.loopHead) {
                    if (!loopInfo.loopBodys->TestBit(pred->GetId())) {
                        loopInfo.loopBodys->SetBit(pred->GetId());
                        pendingList_.emplace_back(pred);
                    }
                }
            }
        }
    }

    void ComputeLoopNumber()
    {
        auto size = linearizer_->regionList_.size();
        dfsStack_.resize(size, DFSState(nullptr, 0));
        linearizer_->circuit_->AdvanceTime();

        auto entry = linearizer_->regionList_.front();
        auto currentDepth = Push(entry, 0);
        while (currentDepth > 0) {
            auto& curState = dfsStack_[currentDepth - 1]; // -1: for current
            auto curRegion = curState.region;
            auto index = curState.index;

            if (index == curRegion->succs_.size()) {
                currentDepth--;
                curRegion->SetFinished(acc_);
            } else {
                GateRegion* succ = curRegion->succs_[index];
                curState.index = ++index;
                if (succ->IsFinished(acc_)) {
                    continue;
                }
                if (succ->IsUnvisited(acc_)) {
                    currentDepth = Push(succ, currentDepth);
                } else {
                    ASSERT(succ->IsVisited(acc_) && index > 0);
                    loopbacks_.emplace_back(DFSState(curRegion, index - 1)); // -1: for prev
                    if (!succ->HasLoopNumber()) {
                        succ->SetLoopNumber(numLoops_++);
                    }
                }
            }
        }
    }

    void ComputeLoopTree()
    {
        linearizer_->circuit_->AdvanceTime();
        auto entry = linearizer_->regionList_.front();
        GraphLinearizer::LoopInfo *loopInfo = nullptr;
        auto currentDepth = Push(entry, 0);
        while (currentDepth > 0) {
            auto &curState = dfsStack_[currentDepth - 1]; // -1: for current
            auto curRegion = curState.region;
            auto index = curState.index;
            GateRegion* succ = nullptr;
            if (index >= curRegion->succs_.size()) {
                if (curRegion->HasLoopNumber()) {
                    if (curRegion->IsVisited(acc_)) {
                        ASSERT(loopInfo != nullptr && loopInfo->loopHead == curRegion);
                        loopInfo = loopInfo->outer;
                        curRegion->SetFinished(acc_);
                    }
                    auto loopExitIndex = index - curRegion->succs_.size();
                    auto& currentInfo = linearizer_->loops_[curRegion->GetLoopNumber()];
                    if (currentInfo.loopExits != nullptr && loopExitIndex < currentInfo.loopExits->size()) {
                        succ = currentInfo.loopExits->at(loopExitIndex);
                        curState.index++;
                    }
                }
            } else {
                succ = curRegion->succs_[curState.index++]; // goto next
            }
            if (succ != nullptr) {
                if (!succ->IsUnvisited(acc_)) {
                    continue;
                }
                if (loopInfo != nullptr && !loopInfo->loopBodys->TestBit(succ->GetId())) {
                    AddLoopExit(succ, loopInfo);
                } else if (succ->IsUnvisited(acc_)) {
                    currentDepth = Push(succ, currentDepth);
                    loopInfo = EnterInnerLoop(succ, loopInfo);
                }
            } else {
                if (!curRegion->HasLoopNumber()) {
                    curRegion->SetFinished(acc_);
                }
                currentDepth--;
            }
        }
    }

    void AddLoopExit(GateRegion* succ, GraphLinearizer::LoopInfo *loopInfo)
    {
        if (loopInfo->loopExits == nullptr) {
            loopInfo->loopExits = chunk_->New<ChunkVector<GateRegion*>>(chunk_);
        }
        loopInfo->loopExits->emplace_back(succ);
    }

    GraphLinearizer::LoopInfo *EnterInnerLoop(GateRegion* succ, GraphLinearizer::LoopInfo *loopInfo)
    {
        // enter inner loop
        if (succ->HasLoopNumber()) {
            ASSERT_PRINT(succ->loopIndex_ != GateRegion::INVALID_INDEX, "GateRegion's index should be assigned");
            auto& innerLoop = linearizer_->loops_[succ->GetLoopNumber()];
            innerLoop.outer = loopInfo;
            if (loopInfo != nullptr) {
                innerLoop.loopDepth = loopInfo->loopDepth + 1;
            } else {
                innerLoop.loopDepth = 1;
            }
            succ->loopDepth_ = static_cast<int32_t>(innerLoop.loopDepth);
            loopInfo = &innerLoop;
        } else if (loopInfo != nullptr) {
            succ->loopIndex_ = static_cast<int32_t>(loopInfo->loopIndex);
            succ->loopDepth_ = static_cast<int32_t>(loopInfo->loopDepth);
        }
        return loopInfo;
    }

    size_t Push(GateRegion *region, size_t depth)
    {
        if (region->IsUnvisited(acc_)) {
            dfsStack_[depth].region = region;
            dfsStack_[depth].index = 0;
            region->SetVisited(acc_);
            return depth + 1;
        }
        return depth;
    }

private:
    struct DFSState {
        DFSState(GateRegion *region, size_t index)
            : region(region), index(index) {}

        GateRegion *region;
        size_t index;
    };
    GraphLinearizer* linearizer_ {nullptr};
    ChunkDeque<GateRegion*> pendingList_;
    ChunkVector<DFSState> loopbacks_;
    Chunk* chunk_ {nullptr};
    ChunkVector<DFSState> dfsStack_;
    GateAccessor acc_;
    size_t numLoops_{0};
};

class GateScheduler {
public:
    explicit GateScheduler(GraphLinearizer *linearizer)
        : linearizer_(linearizer), fixedGateList_(linearizer->chunk_),
        pendingList_(linearizer->chunk_), acc_(linearizer->circuit_),
        scheduleUpperBound_(linearizer_->scheduleUpperBound_) {}

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
                if (acc_.IsStateIn(it) && acc_.IsSelector(succGate)) {
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
                    ComputeLowerBoundAndScheduleGate(curGate);
                }
            }
        }
    }

    void VisitPreparedGate(Edge edge)
    {
        auto curGate = edge.GetGate();
        auto prevGate = acc_.GetIn(curGate, edge.GetIndex());
        if (acc_.IsProlog(prevGate) || acc_.IsRoot(prevGate)) {
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
        if (prevInfo.IsSchedulable() && curInfo.IsSchedulable()) {
            prevInfo.schedulableUseCount++;
        }
    }

    void ComputeLowerBoundAndScheduleGate(GateRef curGate)
    {
        auto& curInfo = linearizer_->GetGateInfo(curGate);
        if (!curInfo.IsSchedulable() ||
            (curInfo.schedulableUseCount != 0) || (curInfo.region != nullptr)) {
            return;
        }
        auto region = GetCommonDominatorOfAllUses(curGate);
        if (scheduleUpperBound_) {
            ASSERT(curInfo.upperBound != nullptr);
            ASSERT(linearizer_->GetCommonDominator(region, curInfo.upperBound) == curInfo.upperBound);
            auto uppermost = curInfo.upperBound->depth_;
            auto upperRegion = GetUpperBoundRegion(region);
            while (upperRegion != nullptr && upperRegion->depth_ >= uppermost) {
                region = upperRegion;
                upperRegion = GetUpperBoundRegion(region);
            }
        }
        if (acc_.GetOpCode(curGate) == OpCode::FINISH_ALLOCATE) {
            ScheduleAllocRegion(curGate, region);
        } else {
            ScheduleGate(curGate, region);
        }
    }

    GateRegion* GetUpperBoundRegion(GateRegion* region)
    {
        if (region->IsLoopHead()) {
            return region->iDominator_;
        }
        auto loopInfo = linearizer_->GetLoopInfo(region);
        if (loopInfo != nullptr) {
            if (!CheckRegionDomLoopExist(region, loopInfo)) {
                return nullptr;
            }
            return loopInfo->loopHead->iDominator_;
        }
        return nullptr;
    }

    void ScheduleAllocRegion(GateRef gate, GateRegion* region)
    {
        ASSERT(acc_.GetOpCode(gate) == OpCode::FINISH_ALLOCATE);
        // 1. schedule FINISH_ALLOCATE first
        ScheduleGate(gate, region);
        GateRef curGate = acc_.GetDep(gate);
        [[maybe_unused]]GateRef output = acc_.GetValueIn(gate, 0);
        // 2. schedule all gates from end to start
        while (acc_.GetOpCode(curGate) != OpCode::START_ALLOCATE) {
            [[maybe_unused]] auto& curInfo = linearizer_->GetGateInfo(curGate);
            ASSERT(curInfo.IsSchedulable());
            ASSERT(curInfo.schedulableUseCount == 0);
            ASSERT(curInfo.region == nullptr);
            ASSERT(acc_.GetStateCount(curGate) == 0);
            ASSERT(acc_.GetDependCount(curGate) == 1);
            ASSERT(acc_.GetValueUsesCount(curGate) == 0 || curGate == output);

            ScheduleGate(curGate, region);
            curGate = acc_.GetDep(curGate);
        }
        // 3. schedule START_ALLOCATE
        ASSERT(linearizer_->GetGateInfo(curGate).schedulableUseCount == 0);
        ScheduleGate(curGate, region);
    }

    bool CheckRegionDomLoopExist(GateRegion* region, GraphLinearizer::LoopInfo* loopInfo)
    {
        if (loopInfo->loopExits == nullptr) {
            return true;
        }
        for (auto exitRegion : *loopInfo->loopExits) {
            if (linearizer_->GetCommonDominator(region, exitRegion) != region) {
                return false;
            }
        }
        return true;
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
        linearizer_->BindGate(gate, region);
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
                ASSERT(acc_.IsCFGMerge(state) && useIt.GetIndex() > 0);
                useGate = acc_.GetIn(state, useIt.GetIndex() - 1); // -1: for state
                useRegion = linearizer_->FindPredRegion(useGate);
            } else if (acc_.IsCFGMerge(useGate)) {
                useGate = acc_.GetIn(useGate, useIt.GetIndex());
                useRegion = linearizer_->FindPredRegion(useGate);
            }

            if (region == nullptr) {
                region = useRegion;
            } else {
                ASSERT(useRegion != nullptr);
                region = linearizer_->GetCommonDominator(region, useRegion);
            }
        }
        return region;
    }

    bool IsInSameDominatorChain(GateRegion* left, GateRegion* right) const
    {
        auto dom = linearizer_->GetCommonDominator(left, right);
        return left == dom || right == dom;
    }

    void ScheduleFixedGate()
    {
        for (auto gate : fixedGateList_) {
            GateRegion* region = linearizer_->GateToRegion(gate);
            linearizer_->BindGate(gate, region);
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
            auto& gateInfo = linearizer_->GetGateInfo(gate);
            if (gateInfo.IsSchedulable()) {
                ASSERT(linearizer_->IsScheduled(gate));
            }
            ASSERT(gateInfo.schedulableUseCount == 0);
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
    if (IsEnableLoopInvariantCodeMotion() && loopNumber_ > 0) {
        scheduleUpperBound_ = true;
        LoopInfoBuilder loopInfoBuilder(this, chunk_);
        loopInfoBuilder.Run();
    }
    if (!onlyBB_) {
        GateScheduler scheduler(this);
        scheduler.Prepare();
        scheduler.ScheduleUpperBound();
        scheduler.ScheduleFloatingGate();
        scheduler.ScheduleFixedGate();
    }
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
    size_t liveNum = OptimizeCFG();

    ASSERT(result.size() == 0);
    result.resize(liveNum);
    auto uses = acc_.Uses(acc_.GetArgRoot());
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        regionList_.front()->gateList_.emplace_back(*useIt);
    }

    size_t i = 0;
    for (size_t id = 0; id < regionList_.size(); id++) {
        GateRegion* r = regionList_[id];
        if (r->IsDead()) {
            continue;
        }
        auto& gates = r->GetGates();
        auto& bb = result[i];
        bb.insert(bb.end(), gates.begin(), gates.end());
        i++;
    }
}

bool GateRegion::IsSimple(GateAccessor *acc) const
{
    for (auto g : gateList_) {
        bool isSimple = acc->IsSimpleState(g);
        bool complexOut = HasComplexOuts();
        if (!isSimple || complexOut) {
            return false;
        }
    }
    return true;
}

size_t GraphLinearizer::OptimizeControls(GateRegion *region)
{
    size_t deads = 0;
    GateRegion* target = region;
    do {
        GateRegion* succ = target->GetSimpleSuccRegion();
        if (succ == nullptr) {
            break;
        }
        MoveAndClear(target, succ);
        target = succ;
        deads++;
    } while (target->IsSimple(&acc_));
    return deads;
}

void GraphLinearizer::MoveAndClear(GateRegion* from, GateRegion* to)
{
    ASSERT(from != to);
    ASSERT(to->GetPreds().size() == 1);
    for (GateRef g: from->GetGates()) {
        ASSERT(acc_.IsSimpleState(g));
        OpCode op = acc_.GetOpCode(g);
        switch (op) {
            case OpCode::IF_TRUE:
            case OpCode::IF_FALSE:
            case OpCode::SWITCH_CASE:
            case OpCode::DEFAULT_CASE:
            case OpCode::LOOP_BACK:
            case OpCode::ORDINARY_BLOCK:
            case OpCode::MERGE:
            case OpCode::VALUE_SELECTOR:
                to->AddGate(g);
                break;
            default:
                break;
        }
    }
    for (auto p : from->GetPreds()) {
        p->ReplaceSucc(from, to);
    }
    to->RemovePred(from);
    from->SetDead();
#ifndef NDEBUG
    from->Clear();
#endif
}

size_t GraphLinearizer::OptimizeCFG()
{
    size_t liveNum = regionList_.size();
    for (size_t i = 0; i < regionList_.size(); i++) {
        GateRegion* src = regionList_[i];
        if (!src->IsDead() && src->IsSimple(&acc_)) {
            size_t dead = OptimizeControls(src);
            ASSERT(liveNum >= dead);
            liveNum -= dead;
        }
    }
    return liveNum;
}

GateRegion* GraphLinearizer::FindPredRegion(GateRef input)
{
    GateRegion* predRegion = GateToRegion(input);
    while (predRegion == nullptr) {
        ASSERT(acc_.GetStateCount(input) == 1); // 1: fall through block
        input = acc_.GetState(input);
        predRegion = GateToRegion(input);
    }
    ASSERT(predRegion != nullptr);
    return predRegion;
}

GateRegion* GraphLinearizer::GetCommonDominator(GateRegion* left, GateRegion* right) const
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

void GraphLinearizer::PrintGraph(const char* title)
{
    LOG_COMPILER(INFO) << "======================== " << title << " ========================";
    int bbIdx = 0;
    for (size_t i = 0; i < regionList_.size(); i++) {
        auto bb = regionList_[i];
        if (bb->IsDead()) {
            continue;
        }
        auto front = bb->gateList_.front();
        auto opcode = acc_.GetOpCode(front);
        size_t loopHeadId = 0;
        auto loopInfo = GetLoopInfo(bb);
        if (loopInfo != nullptr) {
            loopHeadId = loopInfo->loopHead->id_;
        }
        LOG_COMPILER(INFO) << "B" << bb->id_ << "_LB" << bbIdx << ": " << "depth: [" << bb->depth_ << "] "
                           << opcode << "(" << acc_.GetId(front) << ") "
                           << "IDom B" << bb->iDominator_->id_ << " loop Header: " << loopHeadId;
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
        bbIdx++;
    }
}
}  // namespace panda::ecmascript::kungfu
