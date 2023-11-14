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

#include "me_ssa_update.h"

// Create or update HSSA representation for variables given by *updateCands;
// for each variable, the mapped bb set gives the bbs that have newly inserted
// dassign's to the variable.
// If some assignments have been deleted, the current implementation does not
// delete useless phi's, and these useless phi's may end up having identical
// phi operands.
namespace maple {
std::stack<ScalarMeExpr *> *VectorVersionStacks::GetRenameStack(OStIdx idx)
{
    return renameWithVectorStacks.at(idx).get();
}

std::stack<ScalarMeExpr *> *MapVersionStacks::GetRenameStack(OStIdx idx)
{
    auto it = renameWithMapStacks.find(idx);
    if (it == renameWithMapStacks.end()) {
        return nullptr;
    }
    return it->second.get();
}

void VectorVersionStacks::InitRenameStack(OStIdx idx)
{
    renameWithVectorStacks[idx] = std::make_unique<std::stack<ScalarMeExpr *>>();
}

void MapVersionStacks::InitRenameStack(OStIdx idx)
{
    renameWithMapStacks[idx] = std::make_unique<std::stack<ScalarMeExpr *>>();
}

void VectorVersionStacks::RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize)
{
    origStackSize.resize(renameWithVectorStacks.size());
    for (size_t i = 0; i < renameWithVectorStacks.size(); ++i) {
        if (renameWithVectorStacks.at(i) == nullptr) {
            continue;
        }
        origStackSize[i] = std::make_pair(renameWithVectorStacks.at(i)->size(), OStIdx(i));
    }
}

void MapVersionStacks::RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize)
{
    origStackSize.resize(renameWithMapStacks.size());
    uint32 stackId = 0;
    for (const auto &ost2stack : renameWithMapStacks) {
        origStackSize[stackId] = std::make_pair(ost2stack.second->size(), ost2stack.first);
        ++stackId;
    }
}

void VectorVersionStacks::RecoverStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize)
{
    for (size_t i = 1; i < renameWithVectorStacks.size(); ++i) {
        if (renameWithVectorStacks.at(i) == nullptr) {
            continue;
        }
        while (renameWithVectorStacks.at(i)->size() > origStackSize[i].first) {
            renameWithVectorStacks.at(i)->pop();
        }
    }
}

void MapVersionStacks::RecoverStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize)
{
    uint32 stackId = 0;
    for (const auto &ost2stack : renameWithMapStacks) {
        DEBUG_ASSERT(ost2stack.first == origStackSize[stackId].second,
                     "OStIdx must be equal, element of renameWithMapStacks should not be changed");
        while (ost2stack.second->size() > origStackSize[stackId].first) {
            ost2stack.second->pop();
        }
        ++stackId;
    }
}
}  // namespace maple
