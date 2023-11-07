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

#ifndef MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
#define MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
#include "me_ir.h"
#include "me_function.h"

#include <variant>

namespace maple {

class UseItem final {
public:
    explicit UseItem(MeStmt *useStmt) : useNode(useStmt) {}
    explicit UseItem(MePhiNode *phi) : useNode(phi) {}

    const BB *GetUseBB() const
    {
        return std::visit(
            [this](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, MeStmt *>) {
                    return Get<MeStmt>()->GetBB();
                } else {
                    return Get<MePhiNode>()->GetDefBB();
                }
            },
            useNode);
    }

    bool IsUseByPhi() const
    {
        return std::holds_alternative<MePhiNode *>(useNode);
    }

    bool IsUseByStmt() const
    {
        return std::holds_alternative<MeStmt *>(useNode);
    }

    const MeStmt *GetStmt() const
    {
        return Get<MeStmt>();
    }
    MeStmt *GetStmt()
    {
        return Get<MeStmt>();
    }

    const MePhiNode *GetPhi() const
    {
        return Get<MePhiNode>();
    }
    MePhiNode *GetPhi()
    {
        return Get<MePhiNode>();
    }

    bool operator==(const UseItem &other) const
    {
        return useNode == other.useNode;
    }
    bool operator!=(const UseItem &other) const
    {
        return !(*this == other);
    }

private:
    template <typename T>
    const T *Get() const
    {
        assert(std::holds_alternative<T *>(useNode) && "Invalid use type.");
        return std::get<T *>(useNode);
    }
    template <typename T>
    T *Get()
    {
        assert(std::holds_alternative<T *>(useNode) && "Invalid use type.");
        return std::get<T *>(useNode);
    }

    std::variant<MeStmt *, MePhiNode *> useNode;
};

enum MeExprUseInfoState {
    kUseInfoInvalid = 0,
    kUseInfoOfScalar = 1,
    kUseInfoOfAllExpr = 3,
};

using UseSitesType = MapleList<UseItem>;
using ExprUseInfoPair = std::pair<MeExpr *, UseSitesType *>;
class MeExprUseInfo final {
public:
    explicit MeExprUseInfo(MemPool *memPool) : allocator(memPool), useSites(nullptr), useInfoState(kUseInfoInvalid) {}

    ~MeExprUseInfo() {}
    void CollectUseInfoInFunc(IRMap *irMap, Dominance *domTree, MeExprUseInfoState state);
    UseSitesType *GetUseSitesOfExpr(const MeExpr *expr) const;
    const MapleVector<ExprUseInfoPair> &GetUseSites() const
    {
        return *useSites;
    }

    template <class T>
    void AddUseSiteOfExpr(MeExpr *expr, T *useSite);

    void CollectUseInfoInExpr(MeExpr *expr, MeStmt *stmt);
    void CollectUseInfoInStmt(MeStmt *stmt);
    void CollectUseInfoInBB(BB *bb);

    // return true if use sites of scalarA all replaced by scalarB

    MapleVector<ExprUseInfoPair> &GetUseSites()
    {
        return *useSites;
    }

    void SetUseSites(MapleVector<ExprUseInfoPair> *sites)
    {
        useSites = sites;
    }

    void SetState(MeExprUseInfoState state)
    {
        useInfoState = state;
    }

    void InvalidUseInfo()
    {
        useInfoState = kUseInfoInvalid;
    }

    bool IsInvalid() const
    {
        return useInfoState == kUseInfoInvalid;
    }

    bool UseInfoOfScalarIsValid() const
    {
        return useInfoState >= kUseInfoOfScalar;
    }

    bool UseInfoOfAllIsValid() const
    {
        return useInfoState == kUseInfoOfAllExpr;
    }

private:
    MapleAllocator allocator;
    MapleVector<ExprUseInfoPair> *useSites;  // index is exprId
    MeExprUseInfoState useInfoState;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEEXPRUSEINFO_H
