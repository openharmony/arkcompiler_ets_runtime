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

#include "mir_scope.h"
#include "mir_function.h"
#include "printing.h"

namespace maple {

// scp is a sub scope
// (low (scp.low, scp.high] high]
bool MIRScope::IsSubScope(const MIRScope *scp) const
{
    // special case for function level scope which might not have range specified
    if (level == 0) {
        return true;
    }
    auto &l = GetRangeLow();
    auto &l1 = scp->GetRangeLow();
    // allow included file
    if (l.FileNum() != l1.FileNum()) {
        return true;
    }
    auto &h = GetRangeHigh();
    auto &h1 = scp->GetRangeHigh();
    return l.IsBfOrEq(l1) && h1.IsBfOrEq(h);
}

// s1 and s2 has join
// (s1.low (s2.low s1.high] s2.high]
// (s2.low (s1.low s2.high] s1.high]
bool MIRScope::HasJoinScope(const MIRScope *scp1, const MIRScope *scp2) const
{
    auto &l1 = scp1->GetRangeLow();
    auto &h1 = scp1->GetRangeHigh();
    auto &l2 = scp2->GetRangeLow();
    auto &h2 = scp2->GetRangeHigh();
    return (l1.IsBfOrEq(l2) && l2.IsBfOrEq(h1)) || (l2.IsBfOrEq(l1) && l1.IsBfOrEq(h2));
}

// scope range of s1 and s2 may be completly same when macro calling macro expand
bool MIRScope::HasSameRange(const MIRScope *scp1, const MIRScope *scp2) const
{
    auto &l1 = scp1->GetRangeLow();
    auto &h1 = scp1->GetRangeHigh();
    auto &l2 = scp2->GetRangeLow();
    auto &h2 = scp2->GetRangeHigh();
    return l1.IsSrcPostionEq(l2) && h1.IsSrcPostionEq(h2);
}

void MIRScope::IncLevel()
{
    level++;
    for (auto *s : subScopes) {
        s->IncLevel();
    }
}

bool MIRScope::AddScope(MIRScope *scope)
{
    // check first if it is valid with parent scope and sibling sub scopes
    CHECK_FATAL(IsSubScope(scope), "<%s %s> is not a subscope of scope <%s %s>",
                scope->GetRangeLow().DumpLocWithColToString().c_str(),
                scope->GetRangeHigh().DumpLocWithColToString().c_str(), GetRangeLow().DumpLocWithColToString().c_str(),
                GetRangeHigh().DumpLocWithColToString().c_str());
    for (auto *s : subScopes) {
        if (!HasSameRange(s, scope) && HasJoinScope(s, scope)) {
            CHECK_FATAL(false, "<%s %s> has join range with another subscope <%s %s>",
                        scope->GetRangeLow().DumpLocWithColToString().c_str(),
                        scope->GetRangeHigh().DumpLocWithColToString().c_str(),
                        s->GetRangeLow().DumpLocWithColToString().c_str(),
                        s->GetRangeHigh().DumpLocWithColToString().c_str());
        }
    }
    if (this != module->CurFunction()->GetScope()) {
        // skip level incremental if this is function-scope, of level 0,
        // as scope is aready starting from 1
        scope->IncLevel();
    }
    subScopes.push_back(scope);
    return true;
}

#ifdef ARK_LITECG_DEBUG
void MIRScope::Dump(int32 indent) const
{
    int32 ind = static_cast<int32>(level != 0);
    if (level != 0) {
        SrcPosition low = range.first;
        SrcPosition high = range.second;
        PrintIndentation(indent);
        LogInfo::MapleLogger() << "SCOPE <(" << low.FileNum() << ", " << low.LineNum() << ", " << low.Column() << "), ("
                               << high.FileNum() << ", " << high.LineNum() << ", " << high.Column() << ")> {\n";
    }

    for (auto it : aliasVarMap) {
        PrintIndentation(indent + ind);
        LogInfo::MapleLogger() << "ALIAS %" << GlobalTables::GetStrTable().GetStringFromStrIdx(it.first)
                               << ((it.second.isLocal) ? " %" : " $")
                               << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.mplStrIdx) << " ";
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second.tyIdx)->Dump(0);
        if (it.second.sigStrIdx) {
            LogInfo::MapleLogger() << " \"" << GlobalTables::GetStrTable().GetStringFromStrIdx(it.second.sigStrIdx)
                                   << "\"";
        }
        LogInfo::MapleLogger() << '\n';
    }

    for (auto it : subScopes) {
        if (it->NeedEmitAliasInfo()) {
            it->Dump(indent + ind);
        }
    }

    if (level != 0) {
        PrintIndentation(indent);
        LogInfo::MapleLogger() << "}\n";
    }
}

void MIRScope::Dump() const
{
    Dump(0);
}
#endif
}  // namespace maple
