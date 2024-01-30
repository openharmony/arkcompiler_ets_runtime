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
#include "aarch64_global_schedule.h"
#include "aarch64_cg.h"

namespace maplebe {
/*
 * To verify the correctness of the dependency graph,
 * by only scheduling the instructions of nodes in the region based on the inter-block data dependency information.
 */
void AArch64GlobalSchedule::VerifyingSchedule(CDGRegion &region)
{
    for (auto cdgNode : region.GetRegionNodes()) {
        MemPool *cdgNodeMp = memPoolCtrler.NewMemPool("global-scheduler cdgNode memPool", true);

        InitInCDGNode(region, *cdgNode, *cdgNodeMp);
        uint32 scheduledNodeNum = 0;

        /* Schedule independent instructions sequentially */
        MapleVector<DepNode *> candidates = commonSchedInfo->GetCandidates();
        MapleVector<DepNode *> schedResults = commonSchedInfo->GetSchedResults();
        auto depIter = candidates.begin();
        while (!candidates.empty()) {
            DepNode *depNode = *depIter;
            // the depNode can be scheduled
            if (depNode->GetValidPredsSize() == 0) {
                Insn *insn = depNode->GetInsn();
                depNode->SetState(kScheduled);
                schedResults.emplace_back(depNode);
                for (auto succLink : depNode->GetSuccs()) {
                    DepNode &succNode = succLink->GetTo();
                    succNode.DecreaseValidPredsSize();
                }
                depIter = commonSchedInfo->EraseIterFromCandidates(depIter);
                if (insn->GetBB()->GetId() == cdgNode->GetBB()->GetId()) {
                    scheduledNodeNum++;
                }
            } else {
                ++depIter;
            }
            if (depIter == candidates.end()) {
                depIter = candidates.begin();
            }
            // When all instructions in the cdgNode are scheduled, the scheduling ends
            if (scheduledNodeNum == cdgNode->GetInsnNum()) {
                break;
            }
        }

        /* Reorder the instructions of BB based on the scheduling result */
        FinishScheduling(*cdgNode);
        ClearCDGNodeInfo(region, *cdgNode, cdgNodeMp);
    }
}

void AArch64GlobalSchedule::FinishScheduling(CDGNode &cdgNode)
{
    BB *curBB = cdgNode.GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    curBB->ClearInsns();

    MapleVector<DepNode *> schedResults = commonSchedInfo->GetSchedResults();
    for (auto depNode : schedResults) {
        CHECK_FATAL(depNode->GetInsn() != nullptr, "get insn from depNode failed");
        if (!depNode->GetClinitInsns().empty()) {
            for (auto clinitInsn : depNode->GetClinitInsns()) {
                curBB->AppendInsn(*clinitInsn);
            }
        }

        BB *bb = depNode->GetInsn()->GetBB();
        if (bb->GetId() != curBB->GetId()) {
            CDGNode *node = bb->GetCDGNode();
            CHECK_FATAL(node != nullptr, "get cdgNode from bb failed");
            node->RemoveDepNodeFromDataNodes(*depNode);
            if (curBB->MayFoldInCfg(*bb)) {
                // move dbg line with insn if two bb is close in cfg
                Insn *prev = depNode->GetInsn()->GetPrev();
                if (prev != nullptr && prev->IsDbgLine()) {
                    bb->RemoveInsn(*prev);
                    curBB->AppendOtherBBInsn(*prev);
                }
            }
            // Remove the instruction & depNode from the candidate BB
            bb->RemoveInsn(*depNode->GetInsn());
            // Append the instruction of candidateBB
            curBB->AppendOtherBBInsn(*depNode->GetInsn());
        } else {
            // Append debug & comment infos of curBB
            for (auto commentInsn : depNode->GetComments()) {
                if (commentInsn->GetPrev() != nullptr && commentInsn->GetPrev()->IsDbgInsn()) {
                    curBB->AppendInsn(*commentInsn->GetPrev());
                }
                curBB->AppendInsn(*commentInsn);
            }
            if (depNode->GetInsn()->GetPrev() != nullptr && depNode->GetInsn()->GetPrev()->IsDbgInsn()) {
                curBB->AppendInsn(*depNode->GetInsn()->GetPrev());
            }
            // Append the instruction of curBB
            curBB->AppendInsn(*depNode->GetInsn());
        }
    }
    for (auto lastComment : cdgNode.GetLastComments()) {
        curBB->AppendInsn(*lastComment);
    }
    cdgNode.ClearLastComments();
    ASSERT(curBB->NumInsn() >= static_cast<int32>(cdgNode.GetInsnNum()),
           "The number of instructions after global-scheduling is unexpected");
}
} /* namespace maplebe */