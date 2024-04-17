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

#include "loop.h"
#include "optimize_common.h"

// This phase analyses the CFG and identify the loops. The implementation is
// based on the idea that, given two basic block a and b, if b is a's pred and
// a dominates b, then there is a loop from a to b. Loop identification is done
// in a preorder traversal of the dominator tree. In this order, outer loop is
// always detected before its nested loop(s). The building of the LoopDesc data
// structure takes advantage of this ordering.
namespace maplebe {
void LoopDesc::Dump() const
{
    LogInfo::MapleLogger() << "LoopDesc:" << header.GetId() << ", nest depth:" << nestDepth;
    if (parentLoop != nullptr) {
        LogInfo::MapleLogger() << ", parent:" << parentLoop->GetHeader().GetId();
    }
    LogInfo::MapleLogger() << "\n\tbackedge:";
    for (auto bbId : backEdges) {
        LogInfo::MapleLogger() << bbId << " ";
    }
    LogInfo::MapleLogger() << "\n\tmembers:";
    for (auto bbId : loopBBs) {
        LogInfo::MapleLogger() << bbId << " ";
    }
    LogInfo::MapleLogger() << "\n\texitBB:";
    for (auto bbId : exitBBs) {
        LogInfo::MapleLogger() << bbId << " ";
    }
    if (!childLoops.empty()) {
        LogInfo::MapleLogger() << "\n\tchild loops:";
        for (auto *childLoop : childLoops) {
            LogInfo::MapleLogger() << childLoop->GetHeader().GetId() << " ";
        }
    }
    LogInfo::MapleLogger() << "\n\n";
}

LoopDesc *LoopAnalysis::GetOrCreateLoopDesc(BB &headBB)
{
    auto *loop = bbLoopParent[headBB.GetId()];
    if (loop == nullptr || loop->GetHeader().GetId() != headBB.GetId()) {
        // If the headBB is not in loop or loop's header is not the headBB, create a new loop.
        loop = alloc.New<LoopDesc>(alloc, headBB);
        loops.push_back(loop);
    }
    return loop;
}

void LoopAnalysis::SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc)
{
    if (bbLoopParent[bb.GetId()] != nullptr && bbLoopParent[bb.GetId()] != &loopDesc) {
        if (loopDesc.GetParentLoop() == nullptr) {
            loopDesc.SetParentLoop(*bbLoopParent[bb.GetId()]);
            ASSERT_NOT_NULL(loopDesc.GetParentLoop());
            loopDesc.GetParentLoop()->InsertChildLoops(loopDesc);
            loopDesc.SetNestDepth(loopDesc.GetParentLoop()->GetNestDepth() + 1);
        }
    }
    loopDesc.InsertLoopBBs(bb);
    bbLoopParent[bb.GetId()] = &loopDesc;
}

void LoopAnalysis::SetExitBBs(LoopDesc &loop) const
{
    // if loopBBs succs not in the loop, the succs is the loop's exitBB
    for (auto bbId : loop.GetLoopBBs()) {
        auto *bb = cgFunc.GetBBFromID(bbId);
        for (auto *succ : bb->GetSuccs()) {
            if (loop.Has(*succ)) {
                continue;
            }
            loop.InsertExitBBs(*succ);
        }
    }
}

void LoopAnalysis::GenerateLoop(BB *bb)
{
    for (auto *pred : bb->GetPreds()) {
        if (!dom.Dominate(*bb, *pred)) {
            continue;
        }
        auto *loop = GetOrCreateLoopDesc(*bb);
        loop->InsertBackEdges(*pred);
        std::list<BB*> bodyList;
        bodyList.push_back(pred);
        while (!bodyList.empty()) {
            auto *curBB = bodyList.front();
            bodyList.pop_front();
            // skip bb or if it has already been dealt with
            if (curBB == bb || loop->Has(*curBB)) {
                continue;
            }
            SetLoopParent4BB(*curBB, *loop);
            for (auto *curPred : curBB->GetPreds()) {
                bodyList.push_back(curPred);
            }
        }
        SetLoopParent4BB(*bb, *loop);
        SetExitBBs(*loop);
    }
}

void LoopAnalysis::ProcessBB(BB &entryBB)
{
    std::queue<BB *> allBBs;
    allBBs.emplace(&entryBB);
    while (!allBBs.empty()) {
        BB *bb = allBBs.front();
        allBBs.pop();
        if (bb == cgFunc.GetCommonExitBB()) {
            continue;
        }

        // generate loop based on the dom information
        GenerateLoop(bb);
        // process dom tree
        for (auto domChildBBId : dom.GetDomChildren(bb->GetId())) {
            allBBs.emplace(cgFunc.GetBBFromID(domChildBBId));
        }
    }
}

void LoopAnalysis::Analysis()
{
    std::vector<BB*> entryBBs;
    FOR_ALL_BB(bb, (&cgFunc))
    {
        if (bb->GetPreds().size() == 0) {
            entryBBs.push_back(bb);
        }
    }

    for (auto *bb : entryBBs) {
        ProcessBB(*bb);
    }
}

void CgLoopAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.AddRequired<CgDomAnalysis>();
    aDep.SetPreservedAll();
}

bool CgLoopAnalysis::PhaseRun(maplebe::CGFunc &f)
{
    auto *domInfo = GET_ANALYSIS(CgDomAnalysis, f);
    auto *memPool = GetPhaseMemPool();
    loop = memPool->New<LoopAnalysis>(f, *memPool, *domInfo);
    loop->Analysis();
    if (CG_DEBUG_FUNC(f)) {
        loop->Dump();
        // do dot gen after detection so the loop backedge can be properly colored using the loop info
        DotGenerator::GenerateDot("buildloop", f, f.GetMirModule(), f.GetName());
    }
    return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLoopAnalysis, loopanalysis)
} /* namespace maplebe */
