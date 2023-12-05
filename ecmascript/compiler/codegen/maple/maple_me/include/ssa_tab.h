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

#ifndef MAPLE_ME_INCLUDE_SSA_TAB_H
#define MAPLE_ME_INCLUDE_SSA_TAB_H
#include "mempool.h"
#include "mempool_allocator.h"
#include "ver_symbol.h"
#include "ssa_mir_nodes.h"

namespace maple {

class MeFunction;

class SSATab : public AnalysisResult {
    // represent the SSA table
public:
    SSATab(MemPool *memPool, MemPool *versMp, MIRModule *mod, MeFunction *f)
        : AnalysisResult(memPool),
          mirModule(*mod),
          func(f),
          versAlloc(versMp),
          versionStTable(*versMp),
          originalStTable(*memPool, *mod),
          stmtsSSAPart(versMp),
          defBBs4Ost(16, nullptr, versAlloc.Adapter()) // preallocation 16 element memory
    {
    }

    ~SSATab() = default;

    BaseNode *CreateSSAExpr(BaseNode *expr);
    bool HasDefBB(OStIdx oidx)
    {
        return oidx < defBBs4Ost.size() && defBBs4Ost[oidx] && !defBBs4Ost[oidx]->empty();
    }
    void AddDefBB4Ost(OStIdx oidx, BBId bbid)
    {
        if (oidx >= defBBs4Ost.size()) {
            defBBs4Ost.resize(oidx + k16BitSize, nullptr); // expand 16 element memory
        }
        if (defBBs4Ost[oidx] == nullptr) {
            defBBs4Ost[oidx] = versAlloc.GetMemPool()->New<MapleSet<BBId>>(versAlloc.Adapter());
        }
        (void)defBBs4Ost[oidx]->insert(bbid);
    }
    MapleSet<BBId> *GetDefBBs4Ost(OStIdx oidx)
    {
        return defBBs4Ost[oidx];
    }
    VersionSt *GetVerSt(size_t verIdx)
    {
        return versionStTable.GetVersionStVectorItem(verIdx);
    }

    MapleAllocator &GetVersAlloc()
    {
        return versAlloc;
    }

    // following are handles to methods in originalStTable
    OriginalSt *CreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld)
    {
        return originalStTable.CreateSymbolOriginalSt(mirSt, puIdx, fld);
    }

    OriginalSt *FindOrCreateSymbolOriginalSt(MIRSymbol &mirSt, PUIdx puIdx, FieldID fld)
    {
        return originalStTable.FindOrCreateSymbolOriginalSt(mirSt, puIdx, fld);
    }

    OriginalSt *FindOrCreateAddrofSymbolOriginalSt(OriginalSt *ost)
    {
        CHECK_FATAL(ost, "ost is nullptr!");
        auto *addrofOst = ost->GetPrevLevelOst();
        if (ost->GetPrevLevelOst() != nullptr) {
            return addrofOst;
        }
        addrofOst = originalStTable.FindOrCreateAddrofSymbolOriginalSt(ost);
        auto *zeroVersionSt = versionStTable.GetOrCreateZeroVersionSt(*addrofOst);
        originalStTable.AddNextLevelOstOfVst(zeroVersionSt, ost);
        ost->SetPointerVst(zeroVersionSt);
        return addrofOst;
    }

    const OriginalSt *GetOriginalStFromID(OStIdx id) const
    {
        return originalStTable.GetOriginalStFromID(id);
    }
    OriginalSt *GetOriginalStFromID(OStIdx id)
    {
        return originalStTable.GetOriginalStFromID(id);
    }

    const OriginalSt *GetSymbolOriginalStFromID(OStIdx id) const
    {
        const OriginalSt *ost = originalStTable.GetOriginalStFromID(id);
        DEBUG_ASSERT(ost->IsSymbolOst(), "GetSymbolOriginalStFromid: id has wrong ost type");
        return ost;
    }
    OriginalSt *GetSymbolOriginalStFromID(OStIdx id)
    {
        OriginalSt *ost = originalStTable.GetOriginalStFromID(id);
        DEBUG_ASSERT(ost->IsSymbolOst() || ost->GetIndirectLev() > 0,
                     "GetSymbolOriginalStFromid: id has wrong ost type");
        return ost;
    }

    MapleVector<OriginalSt *> *GetNextLevelOsts(const VersionSt &vst) const
    {
        return originalStTable.GetNextLevelOstsOfVst(vst.GetIndex());
    }

    MapleVector<OriginalSt *> *GetNextLevelOsts(size_t vstIdx) const
    {
        return originalStTable.GetNextLevelOstsOfVst(vstIdx);
    }

    PrimType GetPrimType(OStIdx idx) const
    {
        const MIRSymbol *symbol = GetMIRSymbolFromID(idx);
        return symbol->GetType()->GetPrimType();
    }

    const MIRSymbol *GetMIRSymbolFromID(OStIdx id) const
    {
        return originalStTable.GetMIRSymbolFromID(id);
    }
    MIRSymbol *GetMIRSymbolFromID(OStIdx id)
    {
        return originalStTable.GetMIRSymbolFromID(id);
    }

    VersionStTable &GetVersionStTable()
    {
        return versionStTable;
    }

    size_t GetVersionStTableSize() const
    {
        return versionStTable.GetVersionStVectorSize();
    }

    OriginalStTable &GetOriginalStTable()
    {
        return originalStTable;
    }

    const OriginalStTable &GetOriginalStTable() const
    {
        return originalStTable;
    }

    size_t GetOriginalStTableSize() const
    {
        return originalStTable.Size();
    }

    StmtsSSAPart &GetStmtsSSAPart()
    {
        return stmtsSSAPart;
    }
    const StmtsSSAPart &GetStmtsSSAPart() const
    {
        return stmtsSSAPart;
    }

    // should check HasSSAUse first
    const TypeOfMayUseList &GetStmtMayUseNodes(const StmtNode &stmt) const
    {
        return stmtsSSAPart.SSAPartOf(stmt)->GetMayUseNodes();
    }
    // should check IsCallAssigned first
    MapleVector<MustDefNode> &GetStmtMustDefNodes(const StmtNode &stmt)
    {
        return stmtsSSAPart.GetMustDefNodesOf(stmt);
    }

    bool IsWholeProgramScope() const
    {
        return wholeProgramScope;
    }

    void SetWholeProgramScope(bool val)
    {
        wholeProgramScope = val;
    }

    MIRModule &GetModule() const
    {
        return mirModule;
    }

    void SetEPreLocalRefVar(const OStIdx &ostIdx, bool epreLocalrefvarPara = true)
    {
        originalStTable.SetEPreLocalRefVar(ostIdx, epreLocalrefvarPara);
    }

    void SetZeroVersionIndex(const OStIdx &ostIdx, size_t zeroVersionIndexParam)
    {
        originalStTable.SetZeroVersionIndex(ostIdx, zeroVersionIndexParam);
    }

    size_t GetVersionsIndicesSize(const OStIdx &ostIdx) const
    {
        return originalStTable.GetVersionsIndicesSize(ostIdx);
    }

    void UpdateVarOstMap(const OStIdx &ostIdx, std::map<OStIdx, OriginalSt *> &varOstMap)
    {
        originalStTable.UpdateVarOstMap(ostIdx, varOstMap);
    }

    // MIRSymbol query
    const MIRSymbol &GetStmtMIRSymbol(const StmtNode &stmt) const
    {
        return *(GetStmtsSSAPart().GetAssignedVarOf(stmt)->GetOst()->GetMIRSymbol());
    }

    bool IsInitVersion(size_t vstIdx, const OStIdx &ostIdx) const
    {
        auto *ost = GetOriginalStFromID(ostIdx);
        DEBUG_ASSERT(ost != nullptr, "null pointer check");
        return ost->GetZeroVersionIndex() == vstIdx;
    }

    void CollectIterNextLevel(size_t vstIdx, std::set<OriginalSt *> &resultOsts);

private:
    MIRModule &mirModule;
    MeFunction *func;
    MapleAllocator versAlloc;
    VersionStTable versionStTable;  // this uses special versMp because it will be freed earlier
    OriginalStTable originalStTable;
    StmtsSSAPart stmtsSSAPart;                 // this uses special versMp because it will be freed earlier
    MapleVector<MapleSet<BBId> *> defBBs4Ost;  // gives the set of BBs that has def for each original symbol
    bool wholeProgramScope = false;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSA_TAB_H
