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

#ifndef MAPLE_IR_INCLUDE_MIR_SCOPE_H
#define MAPLE_IR_INCLUDE_MIR_SCOPE_H
#include "mir_module.h"
#include "mir_type.h"
#include "src_position.h"

namespace maple {
// mapping src variable to mpl variables to display debug info
struct MIRAliasVars {
    GStrIdx mplStrIdx;  // maple varialbe name
    TyIdx tyIdx;
    bool isLocal;
    GStrIdx sigStrIdx;
};

class MIRScope {
public:
    explicit MIRScope(MIRModule *mod) : module(mod) {}
    MIRScope(MIRModule *mod, unsigned l) : module(mod), level(l) {}
    ~MIRScope() = default;

    bool NeedEmitAliasInfo() const
    {
        return aliasVarMap.size() != 0 || subScopes.size() != 0;
    }

    bool IsSubScope(const MIRScope *scp) const;
    bool HasJoinScope(const MIRScope *scp1, const MIRScope *scp2) const;
    bool HasSameRange(const MIRScope *s1, const MIRScope *s2) const;

    unsigned GetLevel() const
    {
        return level;
    }

    const SrcPosition &GetRangeLow() const
    {
        return range.first;
    }

    const SrcPosition &GetRangeHigh() const
    {
        return range.second;
    }

    void SetRange(SrcPosition low, SrcPosition high)
    {
        DEBUG_ASSERT(low.IsBfOrEq(high), "wrong order of low and high");
        range.first = low;
        range.second = high;
    }

    void SetAliasVarMap(GStrIdx idx, const MIRAliasVars &vars)
    {
        aliasVarMap[idx] = vars;
    }

    void AddAliasVarMap(GStrIdx idx, const MIRAliasVars &vars)
    {
        /* allow same idx, save last aliasVars */
        aliasVarMap[idx] = vars;
    }

    MapleMap<GStrIdx, MIRAliasVars> &GetAliasVarMap()
    {
        return aliasVarMap;
    }

    MapleVector<MIRScope *> &GetSubScopes()
    {
        return subScopes;
    }

    void IncLevel();
    bool AddScope(MIRScope *scope);
    void Dump(int32 indent) const;
    void Dump() const;

private:
    MIRModule *module;
    unsigned level = 0;
    std::pair<SrcPosition, SrcPosition> range;
    // source to maple variable alias
    MapleMap<GStrIdx, MIRAliasVars> aliasVarMap {module->GetMPAllocator().Adapter()};
    MapleVector<MIRScope *> subScopes {module->GetMPAllocator().Adapter()};
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_SCOPE_H
