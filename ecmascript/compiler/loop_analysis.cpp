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
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/loop_analysis.h"

namespace panda::ecmascript::kungfu {
void LoopAnalysis::Run()
{
    CollectLoopBack();
    if (loopCount_ == 1 && !builder_->HasTryCatch()) { // 1: sampe loop
        for (auto &it : loopInfoVector_) {
            CollectLoopBody(it);
            CollectLoopExits(it);
        }
    }
}

void LoopAnalysis::CollectLoopExits(LoopInfo* loopInfo)
{
    for (const auto &it : loopInfo->loopBody) {
        auto &bb = builder_->GetBasicBlockById(it);
        for (const auto &succBlock: bb.succs) {
            size_t succId = succBlock->id;
            if (!loopInfo->loopBody.count(succId)) {
                auto &succBB = builder_->GetBasicBlockById(succId);
                succBB.isLoopExit = true;
                succBB.loopHead = loopInfo->loopHead;
                loopInfo->loopExits.emplace_back(succId);
            }
        }
        bb.isLoopBody = true;
        bb.loopHead = loopInfo->loopHead;
    }
    if (builder_->IsLogEnabled()) {
        LOG_COMPILER(INFO) << "loopHead: " << loopInfo->loopHead;
        std::string log("LoopBody: ");
        for (const auto &it : loopInfo->loopBody) {
            log += std::to_string(it) + " , ";
        }
        LOG_COMPILER(INFO) << log;
        std::string log1("LoopExits: ");
        for (const auto &it : loopInfo->loopExits) {
            log1 += std::to_string(it) + " , ";
        }
        LOG_COMPILER(INFO) << log1;
    }
}

void LoopAnalysis::CollectLoopBody(LoopInfo* loopInfo)
{
    auto size = builder_->GetBasicBlockCount();
    // clear state
    visitState_.insert(visitState_.begin(), size, VisitState::UNVISITED);
    workList_.emplace_back(loopInfo->loopHead);
    loopInfo->loopBody.insert(loopInfo->loopHead);
    while (!workList_.empty()) {
        size_t bbId = workList_.back();
        auto &bb = builder_->GetBasicBlockById(bbId);
        visitState_[bbId] = VisitState::PENDING;
        bool allVisited = true;

        for (const auto &succBlock: bb.succs) {
            size_t succId = succBlock->id;
            if (visitState_[succId] == VisitState::UNVISITED) {
                // dfs
                workList_.emplace_back(succId);
                allVisited = false;
                break;
            // new path to loop body
            } else if (loopInfo->loopBody.count(succId) && !loopInfo->loopBody.count(bbId)) {
                ASSERT(visitState_[succId] != VisitState::UNVISITED);
                for (const auto &it : workList_) {
                    loopInfo->loopBody.insert(it);
                }
            }
        }
        if (allVisited) {
            workList_.pop_back();
            visitState_[bbId] = VisitState::VISITED;
        }
    }
}

void LoopAnalysis::CountLoopBackEdge(size_t fromId, size_t toId)
{
    auto &toBlock = builder_->GetBasicBlockById(toId);
    if (toBlock.numOfLoopBacks == 0) {
        loopCount_++;
        auto loopInfo = chunk_->New<LoopInfo>(chunk_, toId);
        loopInfoVector_.emplace_back(loopInfo);
    }
    toBlock.loopbackBlocks.insert(fromId);
    toBlock.numOfLoopBacks++;
}

void LoopAnalysis::CollectLoopBack()
{
    auto size = builder_->GetBasicBlockCount();
    visitState_.resize(size, VisitState::UNVISITED);

    size_t entryId = 0; // entry id
    workList_.emplace_back(entryId);
    dfsList_.emplace_back(entryId);
    while (!workList_.empty()) {
        size_t bbId = workList_.back();
        auto &bb = builder_->GetBasicBlockById(bbId);
        visitState_[bbId] = VisitState::PENDING;
        bool allVisited = true;

        for (const auto &succBlock: bb.succs) {
            size_t succId = succBlock->id;
            if (visitState_[succId] == VisitState::UNVISITED) {
                // dfs
                workList_.emplace_back(succId);
                allVisited = false;
                dfsList_.emplace_back(succId);
                break;
            } else if (visitState_[succId] == VisitState::PENDING) {
                // back edge
                CountLoopBackEdge(bbId, succId);
            }
        }

        for (const auto &succBlock: bb.catchs) {
            size_t succId = succBlock->id;
            if (visitState_[succId] == VisitState::UNVISITED) {
                // dfs
                workList_.emplace_back(succId);
                dfsList_.emplace_back(succId);
                allVisited = false;
                break;
            } else if (visitState_[succId] == VisitState::PENDING) {
                // back edge
                CountLoopBackEdge(bbId, succId);
            }
        }
        if (allVisited) {
            workList_.pop_back();
            visitState_[bbId] = VisitState::VISITED;
        }
    }
}
}  // namespace panda::ecmascript::kungfu