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

#include "ecmascript/compiler/loop_analysis.h"
#include "ecmascript/compiler/loop_peeling.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::kungfu {
void LoopAnalysis::PrintLoop(LoopInfo* loopInfo)
{
    LOG_COMPILER(INFO) << "--------------------------------- LoopInfo Start ---------------------------------";
    LOG_COMPILER(INFO) << "Head: " << acc_.GetId(loopInfo->loopHead);
    LOG_COMPILER(INFO) << "Size: " << loopInfo->size;
    LOG_COMPILER(INFO) << "MaxDepth: " << loopInfo->maxDepth;
    LOG_COMPILER(INFO) << "Body: [";
    for (auto gate : loopInfo->loopBodys) {
        acc_.ShortPrint(gate);
    }
    LOG_COMPILER(INFO) << "]";
    LOG_COMPILER(INFO) << "Exit: [";
    for (auto gate : loopInfo->loopExits) {
        acc_.ShortPrint(gate);
    }
    LOG_COMPILER(INFO) << "]";
    LOG_COMPILER(INFO) << "--------------------------------- LoopInfo End ---------------------------------";
}

void LoopAnalysis::Run()
{
    ChunkVector<GateRef>& headerGates = bcBuilder_->GetLoopHeaderGates();
    for (auto it = headerGates.rbegin(); it != headerGates.rend(); ++it) {
        auto gate = *it;
        if (gate == Circuit::NullGate()) {
            continue;
        }
        auto loopInfo = chunk_->New<LoopInfo>(chunk_, gate);
        loopInfos_.emplace_back(loopInfo);
    }
}

void LoopAnalysis::CollectUseGate(ChunkUnorderedMap<GateRef, size_t>& gateToDepth,
    ChunkQueue<GateRef>& firstList, ChunkQueue<GateRef>& secondList,
    LoopInfo* loopInfo, GateRef cur)
{
    auto use = acc_.Uses(cur);
    ASSERT(use.begin() != use.end());
    bool isCurLoop = gateToDepth[cur] == 1; // 1: loopDepth
    for (auto it = use.begin(); it != use.end(); ++it) {
        auto nex = *it;
        if (isCurLoop && acc_.IsLoopExit(cur) && (!acc_.IsFixed(*it))) {
            continue;
        } else if (isCurLoop && acc_.IsLoopExitRelated(cur) && acc_.IsFixed(cur)) {
            continue;
        } else if (acc_.GetDependCount(nex) == 0 && acc_.GetStateCount(nex) == 0) {
            continue;
        }
        if (gateToDepth.count(nex)) {
            // loop back
            if (acc_.IsStateIn(it) || acc_.IsDependIn(it)) {
                ASSERT(gateToDepth[nex] == ComputeLoopDepth(cur, nex, gateToDepth[cur]));
            }
            continue;
        }
        if (acc_.IsStateIn(it) || acc_.IsDependIn(it)) {
            // only calculate loop depth for state & depend edges,
            // since there is no phi of each value and each loop head.
            gateToDepth[nex] = ComputeLoopDepth(cur, nex, gateToDepth[cur]);
            if (acc_.HasFrameState(nex)) {
                auto frameState = acc_.GetFrameState(nex);
                if (acc_.GetOpCode(frameState) == OpCode::FRAME_STATE) {
                    gateToDepth[frameState] = gateToDepth[nex];
                    gateToDepth[acc_.GetValueIn(frameState, 1)] = gateToDepth[nex];
                }
            }
            // state and depend edge should be visited first.
            firstList.push(nex);
            UpdateLoopInfo(loopInfo, nex, gateToDepth.at(nex));
        } else {
            secondList.push(nex);
        }
    }
}

void LoopAnalysis::CollectLoopBody(LoopInfo* loopInfo)
{
    ASSERT(acc_.IsLoopHead(loopInfo->loopHead));
    ChunkUnorderedMap<GateRef, size_t> gateToDepth(chunk_);
    ChunkQueue<GateRef> firstList(chunk_);  // for state and depend edges
    ChunkQueue<GateRef> secondList(chunk_); // for other edges
    gateToDepth[loopInfo->loopHead] = 1;
    firstList.push(loopInfo->loopHead);
    while ((!firstList.empty()) || (!secondList.empty())) {
        GateRef cur = Circuit::NullGate();
        if (!firstList.empty()) {
            cur = firstList.front();
            firstList.pop();
        } else {
            cur = secondList.front();
            secondList.pop();
        }
        ASSERT(gateToDepth.count(cur) > 0);
        CollectUseGate(gateToDepth, firstList, secondList, loopInfo, cur);
    }
}

void LoopAnalysis::UpdateLoopInfo(LoopInfo* loopInfo, GateRef gate, size_t dep)
{
    auto op = acc_.GetOpCode(gate);
    loopInfo->maxDepth = std::max(loopInfo->maxDepth, dep);
    loopInfo->size++;
    switch (op) {
        case OpCode::LOOP_BACK: {
            if (dep == 1) { // 1: depth of loop head
                loopInfo->loopBacks.emplace_back(gate);
                return;
            }
            break;
        }
        case OpCode::LOOP_EXIT: {
            if (dep == 1) { // 1: depth of loop head
                loopInfo->loopExits.emplace_back(gate);
                return;
            }
            break;
        }
        case OpCode::LOOP_EXIT_DEPEND:
        case OpCode::LOOP_EXIT_VALUE: {
            if (dep == 1) {
                return;
            }
            break;
        }
        default:
            break;
    }
    if (acc_.HasFrameState(gate)) {
        auto frameState = acc_.GetFrameState(gate);
        if (acc_.GetOpCode(frameState) == OpCode::FRAME_STATE) {
            loopInfo->size += 2;    // 2: framestate and framevalues
            loopInfo->loopBodys.emplace_back(frameState);
            loopInfo->loopBodys.emplace_back(acc_.GetValueIn(frameState, 1));
        }
    }
    loopInfo->loopBodys.emplace_back(gate);
}

// only receive state or depend edge (cur -> dep)
size_t LoopAnalysis::ComputeLoopDepth(GateRef cur, GateRef nex, size_t curDep)
{
    if (acc_.IsLoopExitRelated(cur)) {
        if ((!acc_.IsLoopExit(cur)) || (!acc_.IsFixed(nex))) {
            // exit from some loop
            ASSERT(curDep > 0);
            curDep--;
        }
    }
    auto nexOp = acc_.GetOpCode(nex);
    switch (nexOp) {
        case OpCode::LOOP_BEGIN: {
            if (acc_.GetState(nex) == cur) {
                // enter new loop by state edge
                curDep++;
            }
            break;
        }
        case OpCode::DEPEND_SELECTOR: {
            GateRef state = acc_.GetState(nex);
            if (acc_.IsLoopHead(state) && (cur == acc_.GetDep(nex))) {
                // enter new loop by depend edge
                curDep++;
            }
            break;
        }
        default: {
            break;
        }
    }
    return curDep;
}

void LoopAnalysis::LoopExitElimination()
{
    std::vector<GateRef> gateList;
    acc_.GetAllGates(gateList);
    ChunkQueue<GateRef> workList(chunk_);
    ChunkSet<GateRef> inList(chunk_);
    for (auto gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        switch (op) {
            case OpCode::LOOP_EXIT: {
                acc_.UpdateAllUses(gate, acc_.GetIn(gate, 0));
                acc_.DeleteGate(gate);
                break;
            }
            case OpCode::LOOP_EXIT_DEPEND:
            case OpCode::LOOP_EXIT_VALUE: {
                acc_.UpdateAllUses(gate, acc_.GetIn(gate, 1));
                acc_.DeleteGate(gate);
                break;
            }
            default:
                break;
        }
    }
}
}  // namespace panda::ecmascript::kungfu
