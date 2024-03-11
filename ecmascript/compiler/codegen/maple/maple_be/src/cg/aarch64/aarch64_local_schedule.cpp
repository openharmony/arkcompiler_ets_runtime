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

#include "aarch64_local_schedule.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64LocalSchedule::FinishScheduling(CDGNode &cdgNode)
{
    BB *curBB = cdgNode.GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    curBB->ClearInsns();

    const Insn *prevLocInsn = (curBB->GetPrev() != nullptr ? curBB->GetPrev()->GetLastLoc() : nullptr);
    MapleVector<DepNode *> schedResults = commonSchedInfo->GetSchedResults();
    for (auto depNode : schedResults) {
        Insn *curInsn = depNode->GetInsn();
        CHECK_FATAL(curInsn != nullptr, "get insn from depNode failed");

        // Append comments
        for (auto comment : depNode->GetComments()) {
            if (comment->GetPrev() != nullptr && comment->GetPrev()->IsDbgInsn()) {
                curBB->AppendInsn(*comment->GetPrev());
            }
            curBB->AppendInsn(*comment);
        }

        // Append clinit insns
        if (!depNode->GetClinitInsns().empty()) {
            for (auto clinitInsn : depNode->GetClinitInsns()) {
                curBB->AppendInsn(*clinitInsn);
            }
        } else {
            // Append debug insns
            if (curInsn->GetPrev() != nullptr && curInsn->GetPrev()->IsDbgInsn()) {
                curBB->AppendInsn(*curInsn->GetPrev());
            }
            // Append insn
            curBB->AppendInsn(*curInsn);
        }
    }

    curBB->SetLastLoc(prevLocInsn);
    for (auto lastComment : cdgNode.GetLastComments()) {
        curBB->AppendInsn(*lastComment);
    }
    cdgNode.ClearLastComments();
    DEBUG_ASSERT(curBB->NumInsn() >= static_cast<int32>(cdgNode.GetInsnNum()),
                 "The number of instructions after local-scheduling is unexpected");

    commonSchedInfo = nullptr;
}
} /* namespace maplebe */
