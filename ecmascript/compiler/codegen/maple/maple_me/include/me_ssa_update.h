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

#ifndef MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
#define MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
#include "me_function.h"
#include "dominance.h"
#include "me_irmap.h"

namespace maple {
// When the number of osts is greater than kOstLimitSize, use RenameWithVectorStack object to update ssa, otherwise,
// use RenameWithMapStack object to update ssa
constexpr size_t kOstLimitSize = 5000;

class VersionStacks {
public:
    VersionStacks() = default;
    virtual ~VersionStacks() = default;

    virtual std::stack<ScalarMeExpr *> *GetRenameStack(OStIdx idx)
    {
        (void)idx;
        DEBUG_ASSERT(false, "can not be here");
        return nullptr;
    }

    virtual void InitRenameStack(OStIdx idx)
    {
        (void)idx;
        DEBUG_ASSERT(false, "can not be here");
    }

    virtual void RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize)
    {
        (void)origStackSize;
        DEBUG_ASSERT(false, "can not be here");
    }

    virtual void RecoverStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize)
    {
        (void)origStackSize;
        DEBUG_ASSERT(false, "can not be here");
    }
};

class VectorVersionStacks : public VersionStacks {
public:
    VectorVersionStacks() = default;
    virtual ~VectorVersionStacks() = default;

    std::stack<ScalarMeExpr *> *GetRenameStack(OStIdx idx) override;
    void InitRenameStack(OStIdx idx) override;
    void RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize) override;
    void RecoverStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize) override;

    void ResizeRenameStack(size_t size)
    {
        renameWithVectorStacks.resize(size);
    }

private:
    std::vector<std::unique_ptr<std::stack<ScalarMeExpr *>>> renameWithVectorStacks;
};

class MapVersionStacks : public VersionStacks {
public:
    MapVersionStacks() : renameWithMapStacks(std::less<OStIdx>()) {}
    virtual ~MapVersionStacks() = default;

    std::stack<ScalarMeExpr *> *GetRenameStack(OStIdx idx) override;
    void InitRenameStack(OStIdx idx) override;
    void RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize) override;
    void RecoverStackSize(std::vector<std::pair<uint32, OStIdx>> &origStackSize) override;

private:
    std::map<OStIdx, std::unique_ptr<std::stack<ScalarMeExpr *>>> renameWithMapStacks;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SSA_UPDATE_H
