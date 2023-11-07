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

#ifndef MAPLEBE_INCLUDE_EH_EH_FUNC_H
#define MAPLEBE_INCLUDE_EH_EH_FUNC_H
#include "mir_parser.h"
#include "mir_function.h"
#include "lsda.h"
#include "cg_phase.h"
#include "maple_phase.h"

namespace maplebe {
class EHTry {
public:
    EHTry(MapleAllocator &alloc, TryNode &tryNode) : tryNode(&tryNode), catchVec(alloc.Adapter()) {}
    ~EHTry() = default;

    TryNode *GetTryNode() const
    {
        return tryNode;
    }

    void SetEndtryNode(StmtNode &endtryNode)
    {
        this->endTryNode = &endtryNode;
    }

    StmtNode *GetEndtryNode()
    {
        return endTryNode;
    }

    void SetFallthruGoto(StmtNode *fallthruGoto)
    {
        this->fallThroughGoto = fallthruGoto;
    }

    StmtNode *GetFallthruGoto()
    {
        return fallThroughGoto;
    }

    size_t GetCatchVecSize() const
    {
        return catchVec.size();
    }

    void PushBackCatchVec(CatchNode &catchNode)
    {
        catchVec.emplace_back(&catchNode);
    }

    CatchNode *GetCatchNodeAt(size_t pos) const
    {
        CHECK_FATAL(pos < GetCatchVecSize(), "pos is out of range.");
        return catchVec.at(pos);
    }

    void SetLSDACallSite(LSDACallSite &lsdaCallSite)
    {
        this->lsdaCallSite = &lsdaCallSite;
    }

    void SetCSAction(uint32 action) const
    {
        lsdaCallSite->csAction = action;
    }

    void DumpEHTry(const MIRModule &mirModule);

private:
    TryNode *tryNode;
    StmtNode *endTryNode = nullptr;
    StmtNode *fallThroughGoto = nullptr; /* no throw in the try block, the goto stmt to the fall through */
    MapleVector<CatchNode *> catchVec;
    LSDACallSite *lsdaCallSite = nullptr; /* one try has a callsite */
};

class EHThrow {
public:
    explicit EHThrow(UnaryStmtNode &rtNode) : rethrow(&rtNode) {}
    ~EHThrow() = default;

    bool IsUnderTry() const
    {
        return javaTry != nullptr;
    }

    bool HasLSDA() const
    {
        return startLabel != nullptr;
    }

    const UnaryStmtNode *GetRethrow() const
    {
        return rethrow;
    }

    void SetJavaTry(EHTry *javaTry)
    {
        this->javaTry = javaTry;
    }

    LabelNode *GetStartLabel()
    {
        return startLabel;
    }

    LabelNode *GetEndLabel()
    {
        return endLabel;
    }

    void Lower(CGFunc &cgFunc);
    void ConvertThrowToRethrow(CGFunc &cgFunc);
    void ConvertThrowToRuntime(CGFunc &cgFunc, BaseNode &arg);

private:
    UnaryStmtNode *rethrow;          /* must be a throw stmt */
    EHTry *javaTry = nullptr;        /* the try statement wrapping this throw */
    LabelNode *startLabel = nullptr; /* the label that "MCC_RethrowException" or "MCC_ThrowException" begin */
    LabelNode *endLabel = nullptr;   /* the label that "MCC_RethrowException" or "MCC_ThrowException" end */
};

class EHFunc {
public:
    static constexpr uint8 kTypeEncoding = 0x9b; /* same thing as LSDAHeader.kTypeEncoding */
    explicit EHFunc(CGFunc &func);
    ~EHFunc() = default;

    void CollectEHInformation(std::vector<std::pair<LabelIdx, CatchNode *>> &catchVec);
    void InsertEHSwitchTable();
    void CreateLSDA();
    bool NeedFullLSDA() const;
    bool NeedFastLSDA() const;
    void InsertCxaAfterEachCatch(const std::vector<std::pair<LabelIdx, CatchNode *>> &catchVec);
    void GenerateCleanupLabel();
    void MergeCatchToTry(const std::vector<std::pair<LabelIdx, CatchNode *>> &catchVec);
    void BuildEHTypeTable(const std::vector<std::pair<LabelIdx, CatchNode *>> &catchVec);
    void LowerThrow(); /* for non-personality function */
    void CreateTypeInfoSt();
    void DumpEHFunc() const;

    bool HasThrow() const
    {
        return !rethrowVec.empty();
    }

    void AddTry(EHTry &ehTry)
    {
        tryVec.emplace_back(&ehTry);
    }

    size_t GetEHTyTableSize() const
    {
        return ehTyTable.size();
    }

    TyIdx &GetEHTyTableMember(int32 index)
    {
        CHECK_FATAL(static_cast<size_t>(index) < ehTyTable.size(), "out of ehTyTable");
        return ehTyTable[index];
    }

    LSDAHeader *GetLSDAHeader()
    {
        return lsdaHeader;
    }

    LSDACallSiteTable *GetLSDACallSiteTable()
    {
        return lsdaCallSiteTable;
    }

    const LSDACallSiteTable *GetLSDACallSiteTable() const
    {
        return lsdaCallSiteTable;
    }

    const LSDAActionTable *GetLSDAActionTable() const
    {
        return lsdaActionTable;
    }

    void AddRethrow(EHThrow &rethrow)
    {
        rethrowVec.emplace_back(&rethrow);
    }

private:
    void CreateLSDAAction();
    void InsertDefaultLabelAndAbortFunc(BlockNode &blkNode, SwitchNode &switchNode, const StmtNode &beforeEndLabel);
    void FillSwitchTable(SwitchNode &switchNode, const EHTry &ehTry);
    void CreateLSDAHeader();
    void FillLSDACallSiteTable();
    LabelIdx CreateLabel(const std::string &cstr);
    bool HasTry() const;

    CGFunc *cgFunc;
    LabelIdx labelIdx = 0;
    MapleVector<EHTry *> tryVec;           /* try stmt node */
    MapleVector<TyIdx> ehTyTable;          /* the type that would emit in LSDA */
    MapleMap<TyIdx, uint32> ty2IndexTable; /* use the TyIdx to get the index of ehTyTable; */
    LSDAHeader *lsdaHeader = nullptr;
    LSDACallSiteTable *lsdaCallSiteTable = nullptr;
    LSDAActionTable *lsdaActionTable = nullptr;
    MapleVector<EHThrow *> rethrowVec; /* EHRethrow */
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgBuildEHFunc, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_EH_EH_FUNC_H */