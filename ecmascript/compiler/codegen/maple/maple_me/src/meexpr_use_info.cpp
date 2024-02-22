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

#include "meexpr_use_info.h"
#include "irmap.h"
#include "dominance.h"

namespace maple {
template <class T>
void MeExprUseInfo::AddUseSiteOfExpr(MeExpr *expr, T *useSite)
{
    if (expr->GetExprID() == kInvalidExprID) {
        return;
    }

    auto uintExprID = static_cast<uint32>(expr->GetExprID());
    if (useSites->size() <= uintExprID) {
        constexpr uint32 bufferSize = 50;
        useSites->insert(useSites->end(), uintExprID + bufferSize, {nullptr, nullptr});
    }
    if ((*useSites)[uintExprID].second == nullptr) {
        auto *newList = allocator.New<MapleList<UseItem>>(allocator.Adapter());
        newList->emplace_front(UseItem(useSite));
        (*useSites)[uintExprID] = {expr, newList};
        return;
    }
    UseItem use(useSite);
    if ((*useSites)[uintExprID].second->front() != use) {
        (*useSites)[uintExprID].second->emplace_front(use);
    }
}

MapleList<UseItem> *MeExprUseInfo::GetUseSitesOfExpr(const MeExpr *expr) const
{
    if (IsInvalid()) {
        CHECK_FATAL(false, "Expr use info is invalid");
    } else if (!expr->IsScalar()) {
        CHECK_FATAL(useInfoState == kUseInfoOfAllExpr, "expr is not scalar, use info has not been collected");
    }

    if (expr->GetExprID() == kInvalidExprID) {
        return nullptr;
    }

    auto uintExprID = static_cast<uint32>(expr->GetExprID());
    if (useSites->size() <= uintExprID) {
        return nullptr;
    }
    return (*useSites)[uintExprID].second;
}

void MeExprUseInfo::CollectUseInfoInExpr(MeExpr *expr, MeStmt *stmt)
{
    if (expr == nullptr) {
        return;
    }
    if (expr->IsScalar()) {
        AddUseSiteOfExpr(static_cast<ScalarMeExpr *>(expr), stmt);
        return;
    }

    if (useInfoState == kUseInfoOfAllExpr) {
        AddUseSiteOfExpr(expr, stmt);
    }

    if (expr->GetMeOp() == kMeOpIvar) {
        for (auto *mu : static_cast<IvarMeExpr *>(expr)->GetMuList()) {
            AddUseSiteOfExpr(mu, stmt);
        }
    }

    for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
        auto opnd = expr->GetOpnd(opndId);
        CollectUseInfoInExpr(opnd, stmt);
    }
}

void MeExprUseInfo::CollectUseInfoInStmt(MeStmt *stmt)
{
    for (size_t opndId = 0; opndId < stmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = stmt->GetOpnd(opndId);
        CollectUseInfoInExpr(opnd, stmt);
    }

    auto *muList = stmt->GetMuList();
    if (muList != nullptr) {
        for (const auto &ost2mu : *muList) {
            AddUseSiteOfExpr(ost2mu.second, stmt);
        }
    }
}

void MeExprUseInfo::CollectUseInfoInBB(BB *bb)
{
    if (bb == nullptr) {
        return;
    }

    auto &phiList = bb->GetMePhiList();
    for (const auto &ost2phi : phiList) {
        auto *phi = ost2phi.second;
        if (!phi->GetIsLive()) {
            continue;
        }
        for (size_t id = 0; id < phi->GetOpnds().size(); ++id) {
            auto *opnd = phi->GetOpnd(id);
            AddUseSiteOfExpr(opnd, phi);
        }
    }

    for (auto &stmt : bb->GetMeStmts()) {
        CollectUseInfoInStmt(&stmt);
    }
}

void MeExprUseInfo::CollectUseInfoInFunc(IRMap *irMap, Dominance *domTree, MeExprUseInfoState state)
{
    if (!IsInvalid() && (state <= this->useInfoState)) {
        return;
    }
    if (useSites != nullptr) {
        for (auto &useSite : *useSites) {
            if (useSite.second != nullptr) {
                useSite.second->clear();
            }
        }
        useSites->clear();
    }

    allocator.SetMemPool(irMap->GetIRMapAlloc().GetMemPool());
    useSites = irMap->New<MapleVector<ExprUseInfoPair>>(allocator.Adapter());
    if (irMap->GetExprID() >= 0) {
        useSites->resize(static_cast<size_t>(irMap->GetExprID() + 1));
    }
    useInfoState = state;

    for (auto bb : domTree->GetReversePostOrder()) {
        CollectUseInfoInBB(irMap->GetBB(BBId(bb->GetID())));
    }
}
}  // namespace maple
