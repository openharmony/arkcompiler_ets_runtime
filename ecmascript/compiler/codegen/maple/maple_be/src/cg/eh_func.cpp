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

#include "eh_func.h"
#include "cgfunc.h"
#include "cg.h"
#include "mir_builder.h"
#include "switch_lowerer.h"

namespace maplebe {
using namespace maple;

void EHTry::DumpEHTry(const MIRModule &mirModule)
{
    if (tryNode != nullptr) {
        tryNode->Dump();
    }

    if (endTryNode != nullptr) {
        endTryNode->Dump();
    }

    for (const auto *currCatch : catchVec) {
        if (currCatch == nullptr) {
            continue;
        }
        currCatch->Dump();
    }
}

void EHThrow::ConvertThrowToRuntime(CGFunc &cgFunc, BaseNode &arg)
{
    MIRFunction &mirFunc = cgFunc.GetFunction();
    MIRModule *mirModule = mirFunc.GetModule();
    MIRFunction *calleeFunc =
        mirModule->GetMIRBuilder()->GetOrCreateFunction("MCC_ThrowException", static_cast<TyIdx>(PTY_void));
    cgFunc.GetBecommon().UpdateTypeTable(*calleeFunc->GetMIRFuncType());
    calleeFunc->SetNoReturn();
    MapleVector<BaseNode *> args(mirModule->GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.emplace_back(&arg);
    CallNode *callAssign = mirModule->GetMIRBuilder()->CreateStmtCall(calleeFunc->GetPuidx(), args);
    mirFunc.GetBody()->ReplaceStmt1WithStmt2(rethrow, callAssign);
}

void EHThrow::ConvertThrowToRethrow(CGFunc &cgFunc)
{
    MIRFunction &mirFunc = cgFunc.GetFunction();
    MIRModule *mirModule = mirFunc.GetModule();
    MIRBuilder *mirBuilder = mirModule->GetMIRBuilder();
    MIRFunction *unFunc = mirBuilder->GetOrCreateFunction("MCC_RethrowException", static_cast<TyIdx>(PTY_void));
    cgFunc.GetBecommon().UpdateTypeTable(*unFunc->GetMIRFuncType());
    unFunc->SetNoReturn();
    MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.emplace_back(rethrow->Opnd(0));
    CallNode *callNode = mirBuilder->CreateStmtCall(unFunc->GetPuidx(), args);
    mirFunc.GetBody()->ReplaceStmt1WithStmt2(rethrow, callNode);
}

EHFunc::EHFunc(CGFunc &func)
    : cgFunc(&func),
      tryVec(func.GetFuncScopeAllocator()->Adapter()),
      ehTyTable(func.GetFuncScopeAllocator()->Adapter()),
      ty2IndexTable(std::less<TyIdx>(), func.GetFuncScopeAllocator()->Adapter()),
      rethrowVec(func.GetFuncScopeAllocator()->Adapter())
{
}

EHFunc *CGFunc::BuildEHFunc()
{
    EHFunc *newEHFunc = GetMemoryPool()->New<EHFunc>(*this);
    SetEHFunc(*newEHFunc);
    std::vector<std::pair<LabelIdx, CatchNode *>> catchVec;
    newEHFunc->MergeCatchToTry(catchVec);
    newEHFunc->BuildEHTypeTable(catchVec);
    newEHFunc->InsertEHSwitchTable();
    newEHFunc->GenerateCleanupLabel();

    GetBecommon().BeGetOrCreatePointerType(*GlobalTables::GetTypeTable().GetVoid());
    if (newEHFunc->NeedFullLSDA()) {
        newEHFunc->CreateLSDA();
    }
    if (GetCG()->GetCGOptions().GenerateExceptionHandlingCode()) {
        newEHFunc->CreateTypeInfoSt();
    }

    return newEHFunc;
}

bool EHFunc::NeedFullLSDA() const
{
    return false;
}

bool EHFunc::NeedFastLSDA() const
{
    return false;
}

bool EHFunc::HasTry() const
{
    return !tryVec.empty();
}

void EHFunc::CreateTypeInfoSt()
{
    MIRFunction &mirFunc = cgFunc->GetFunction();
    bool ctorDefined = false;
    if (mirFunc.GetAttr(FUNCATTR_constructor) && !mirFunc.GetAttr(FUNCATTR_static) && (mirFunc.GetBody() != nullptr)) {
        ctorDefined = true;
    }

    if (!ctorDefined) {
        return;
    }

    const auto *classType = static_cast<const MIRClassType *>(mirFunc.GetClassType());
    if (cgFunc->GetMirModule().IsCModule() && classType == nullptr) {
        return;
    }
    DEBUG_ASSERT(classType != nullptr, "");
    if (classType->GetMethods().empty() && (classType->GetFieldsSize() == 0)) {
        return;
    }
}

/*
 * merge catch to try
 */
void EHFunc::MergeCatchToTry(const std::vector<std::pair<LabelIdx, CatchNode *>> &catchVec)
{
    size_t tryOffsetCount;
    for (auto *ehTry : tryVec) {
        tryOffsetCount = ehTry->GetTryNode()->GetOffsetsCount();
        for (size_t i = 0; i < tryOffsetCount; i++) {
            auto o = ehTry->GetTryNode()->GetOffset(i);
            for (const auto &catchVecPair : catchVec) {
                LabelIdx lbIdx = catchVecPair.first;
                if (lbIdx == o) {
                    ehTry->PushBackCatchVec(*catchVecPair.second);
                    break;
                }
            }
        }
        CHECK_FATAL(ehTry->GetCatchVecSize() == tryOffsetCount,
                    "EHTry instance offset does not equal catch node amount.");
    }
}

/* catchvec is going to be released by the caller */
void EHFunc::BuildEHTypeTable(const std::vector<std::pair<LabelIdx, CatchNode *>> &catchVec)
{
    if (!catchVec.empty()) {
        /* the first one assume to be <*void> */
        TyIdx voidTyIdx(PTY_void);
        ehTyTable.emplace_back(voidTyIdx);
        ty2IndexTable[voidTyIdx] = 0;
        /* create void pointer and update becommon's size table */
        cgFunc->GetBecommon().UpdateTypeTable(*GlobalTables::GetTypeTable().GetVoidPtr());
    }

    /* create the type table for this function, just iterate each catch */
    CatchNode *jCatchNode = nullptr;
    size_t catchNodeSize;
    for (const auto &catchVecPair : catchVec) {
        jCatchNode = catchVecPair.second;
        catchNodeSize = jCatchNode->Size();
        for (size_t i = 0; i < catchNodeSize; i++) {
            MIRType *mirTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(jCatchNode->GetExceptionTyIdxVecElement(i));
            DEBUG_ASSERT(mirTy->GetKind() == kTypePointer, "mirTy is not pointer type");
            TyIdx ehTyIdx = static_cast<MIRPtrType *>(mirTy)->GetPointedTyIdx();
            if (ty2IndexTable.find(ehTyIdx) != ty2IndexTable.end()) {
                continue;
            }

            ty2IndexTable[ehTyIdx] = ehTyTable.size();
            ehTyTable.emplace_back(ehTyIdx);
        }
    }
}

void EHFunc::DumpEHFunc() const
{
    MIRModule &mirModule = *cgFunc->GetFunction().GetModule();
    for (uint32 i = 0; i < this->tryVec.size(); i++) {
        LogInfo::MapleLogger() << "\n========== start " << i << " th eh:\n";
        EHTry *ehTry = tryVec[i];
        ehTry->DumpEHTry(mirModule);
        LogInfo::MapleLogger() << "========== end " << i << " th eh =========\n";
    }

    LogInfo::MapleLogger() << "\n========== start LSDA type table ========\n";
    for (uint32 i = 0; i < this->ehTyTable.size(); i++) {
        LogInfo::MapleLogger() << i << " vector to ";
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(ehTyTable[i])->Dump(0);
        LogInfo::MapleLogger() << "\n";
    }
    LogInfo::MapleLogger() << "========== end LSDA type table ========\n";

    LogInfo::MapleLogger() << "\n========== start type-index map ========\n";
    for (const auto &ty2indexTablePair : ty2IndexTable) {
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(ty2indexTablePair.first)->Dump(0);
        LogInfo::MapleLogger() << " map to ";
        LogInfo::MapleLogger() << ty2indexTablePair.second << "\n";
    }
    LogInfo::MapleLogger() << "========== end type-index map ========\n";
}

/*
 * cleanup_label is an LabelNode, and placed just before endLabel.
 * cleanup_label is the first statement of cleanupbb.
 * the layout of clean up code is:
 * //return bb
 *   ...
 * //cleanup bb = lastbb->prev; cleanupbb->PrependBB(retbb)
 *   cleanup_label:
 *     ...
 * //lastbb
 *   endLabel:
 *     .cfi_endproc
 *   .Label.xx.end:
 *     .size
 */
void EHFunc::GenerateCleanupLabel()
{
    MIRModule *mirModule = cgFunc->GetFunction().GetModule();
    cgFunc->SetCleanupLabel(*mirModule->GetMIRBuilder()->CreateStmtLabel(CreateLabel(".LCLEANUP")));
    BlockNode *blockNode = cgFunc->GetFunction().GetBody();
    blockNode->InsertBefore(cgFunc->GetEndLabel(), cgFunc->GetCleanupLabel());
}

void EHFunc::InsertDefaultLabelAndAbortFunc(BlockNode &blkNode, SwitchNode &switchNode, const StmtNode &beforeEndLabel)
{
    MIRModule &mirModule = *cgFunc->GetFunction().GetModule();
    LabelIdx dfLabIdx = cgFunc->GetFunction().GetLabelTab()->CreateLabel();
    cgFunc->GetFunction().GetLabelTab()->AddToStringLabelMap(dfLabIdx);
    StmtNode *dfLabStmt = mirModule.GetMIRBuilder()->CreateStmtLabel(dfLabIdx);
    blkNode.InsertAfter(&beforeEndLabel, dfLabStmt);
    MIRFunction *calleeFunc = mirModule.GetMIRBuilder()->GetOrCreateFunction("abort", static_cast<TyIdx>(PTY_void));
    cgFunc->GetBecommon().UpdateTypeTable(*calleeFunc->GetMIRFuncType());
    MapleVector<BaseNode *> args(mirModule.GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
    CallNode *callExit = mirModule.GetMIRBuilder()->CreateStmtCall(calleeFunc->GetPuidx(), args);
    blkNode.InsertAfter(dfLabStmt, callExit);
    switchNode.SetDefaultLabel(dfLabIdx);
}

void EHFunc::FillSwitchTable(SwitchNode &switchNode, const EHTry &ehTry)
{
    CatchNode *catchNode = nullptr;
    MIRType *exceptionType = nullptr;
    MIRPtrType *ptType = nullptr;
    size_t catchVecSize = ehTry.GetCatchVecSize();
    /* update switch node's cases */
    for (size_t i = 0; i < catchVecSize; i++) {
        catchNode = ehTry.GetCatchNodeAt(i);
        for (size_t j = 0; j < catchNode->Size(); j++) {
            exceptionType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(catchNode->GetExceptionTyIdxVecElement(j));
            ptType = static_cast<MIRPtrType *>(exceptionType);
            MapleMap<TyIdx, uint32>::iterator ty2IdxIt = ty2IndexTable.find(ptType->GetPointedTyIdx());
            DEBUG_ASSERT(ty2IdxIt != ty2IndexTable.end(), "find tyIdx failed!");
            uint32 tableIdx = ty2IdxIt->second;
            LabelNode *catchLabelNode = static_cast<LabelNode *>(catchNode->GetPrev());
            CasePair p(tableIdx, catchLabelNode->GetLabelIdx());
            bool inserted = false;
            for (auto x : switchNode.GetSwitchTable()) {
                if (x == p) {
                    inserted = true;
                    break;
                }
            }
            if (!inserted) {
                switchNode.InsertCasePair(p);
            }
        }
    }
}

/* this is also the landing pad code. */
void EHFunc::InsertEHSwitchTable()
{
    MIRModule &mirModule = *cgFunc->GetFunction().GetModule();
    BlockNode *blockNode = cgFunc->GetFunction().GetBody();
    CHECK_FATAL(blockNode != nullptr, "get function body failed in EHThrow::InsertEHSwitchTable");
    StmtNode *endLabelPrevNode = nullptr;
    SwitchNode *switchNode = nullptr;
    for (auto *ehTry : tryVec) {
        endLabelPrevNode = cgFunc->GetEndLabel()->GetPrev();
        /*
         * get the next statement of the trynode. when no throw happend in try block, jump to the statement directly
         * create a switch statement and insert after tryend;
         */
        switchNode = mirModule.CurFuncCodeMemPool()->New<SwitchNode>(mirModule);
        /* create a new label as default, and if program excute here, error it */
        InsertDefaultLabelAndAbortFunc(*blockNode, *switchNode, *endLabelPrevNode);
        /* create s special symbol that use the second return of __builtin_eh_return() */
        MIRSymbol *mirSymbol = mirModule.GetMIRBuilder()->CreateSymbol(TyIdx(PTY_i32), "__eh_index__", kStVar, kScAuto,
                                                                       &cgFunc->GetFunction(), kScopeLocal);
        switchNode->SetSwitchOpnd(mirModule.GetMIRBuilder()->CreateExprDread(*mirSymbol));
        FillSwitchTable(*switchNode, *ehTry);
        SwitchLowerer switchLower(mirModule, *switchNode, *cgFunc->GetFuncScopeAllocator());
        blockNode->InsertBlockAfter(*switchLower.LowerSwitch(), endLabelPrevNode);
        ehTry->SetFallthruGoto(endLabelPrevNode->GetNext());
    }
    if (!CGOptions::IsQuiet()) {
        cgFunc->GetFunction().Dump();
    }
}

LabelIdx EHFunc::CreateLabel(const std::string &cstr)
{
    MIRSymbol *mirSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc->GetFunction().GetStIdx().Idx());
    CHECK_FATAL(mirSymbol != nullptr, "get function symbol failed in EHFunc::CreateLabel");
    std::string funcName = mirSymbol->GetName();
    std::string labStr = funcName.append(cstr).append(std::to_string(labelIdx++));
    return cgFunc->GetFunction().GetOrCreateLableIdxFromName(labStr);
}

void EHFunc::CreateLSDAHeader()
{
    constexpr uint8 startEncoding = 0xff;
    constexpr uint8 typeEncoding = 0x9b;
    constexpr uint8 callSiteEncoding = 0x1;
    MIRBuilder *mirBuilder = cgFunc->GetFunction().GetModule()->GetMIRBuilder();

    LSDAHeader *lsdaHeaders = cgFunc->GetMemoryPool()->New<LSDAHeader>();
    LabelIdx lsdaHdLblIdx = CreateLabel("LSDAHD"); /* LSDA head */
    LabelNode *lsdaHdLblNode = mirBuilder->CreateStmtLabel(lsdaHdLblIdx);
    lsdaHeaders->SetLSDALabel(*lsdaHdLblNode);

    LabelIdx lsdaTTStartIdx = CreateLabel("LSDAALLS"); /* LSDA all start; */
    LabelNode *lsdaTTLblNode = mirBuilder->CreateStmtLabel(lsdaTTStartIdx);
    LabelIdx lsdaTTEndIdx = CreateLabel("LSDAALLE"); /* LSDA all end; */
    LabelNode *lsdaCSTELblNode = mirBuilder->CreateStmtLabel(lsdaTTEndIdx);
    lsdaHeaders->SetTTypeOffset(lsdaTTLblNode, lsdaCSTELblNode);

    lsdaHeaders->SetLPStartEncoding(startEncoding);
    lsdaHeaders->SetTTypeEncoding(typeEncoding);
    lsdaHeaders->SetCallSiteEncoding(callSiteEncoding);
    lsdaHeader = lsdaHeaders;
}

void EHFunc::FillLSDACallSiteTable()
{
    constexpr uint8 callSiteFirstAction = 0x1;
    MIRBuilder *mirBuilder = cgFunc->GetFunction().GetModule()->GetMIRBuilder();
    BlockNode *bodyNode = cgFunc->GetFunction().GetBody();

    lsdaCallSiteTable = cgFunc->GetMemoryPool()->New<LSDACallSiteTable>(*cgFunc->GetFuncScopeAllocator());
    LabelIdx lsdaCSTStartIdx = CreateLabel("LSDACSTS"); /* LSDA callsite table start; */
    LabelNode *lsdaCSTStartLabel = mirBuilder->CreateStmtLabel(lsdaCSTStartIdx);
    LabelIdx lsdaCSTEndIdx = CreateLabel("LSDACSTE"); /* LSDA callsite table end; */
    LabelNode *lsdaCSTEndLabel = mirBuilder->CreateStmtLabel(lsdaCSTEndIdx);
    lsdaCallSiteTable->SetCSTable(lsdaCSTStartLabel, lsdaCSTEndLabel);

    /* create LDSACallSite for each EHTry instance */
    for (auto *ehTry : tryVec) {
        DEBUG_ASSERT(ehTry != nullptr, "null ptr check");
        /* replace try with a label which is the callsite_start */
        LabelIdx csStartLblIdx = CreateLabel("LSDACS");
        LabelNode *csLblNode = mirBuilder->CreateStmtLabel(csStartLblIdx);
        LabelIdx csEndLblIdx = CreateLabel("LSDACE");
        LabelNode *ceLblNode = mirBuilder->CreateStmtLabel(csEndLblIdx);
        TryNode *tryNode = ehTry->GetTryNode();
        bodyNode->ReplaceStmt1WithStmt2(tryNode, csLblNode);
        StmtNode *endTryNode = ehTry->GetEndtryNode();
        bodyNode->ReplaceStmt1WithStmt2(endTryNode, ceLblNode);

        LabelNode *ladpadEndLabel = nullptr;
        if (ehTry->GetFallthruGoto()) {
            ladpadEndLabel = mirBuilder->CreateStmtLabel(CreateLabel("LSDALPE"));
            bodyNode->InsertBefore(ehTry->GetFallthruGoto(), ladpadEndLabel);
        } else {
            ladpadEndLabel = ceLblNode;
        }
        /* When there is only one catch, the exception table is optimized. */
        if (ehTry->GetCatchVecSize() == 1) {
            ladpadEndLabel = static_cast<LabelNode *>(ehTry->GetCatchNodeAt(0)->GetPrev());
        }

        LSDACallSite *lsdaCallSite = cgFunc->GetMemoryPool()->New<LSDACallSite>();
        LabelPair csStart(cgFunc->GetStartLabel(), csLblNode);
        LabelPair csLength(csLblNode, ceLblNode);
        LabelPair csLandingPad(cgFunc->GetStartLabel(), ladpadEndLabel);
        lsdaCallSite->Init(csStart, csLength, csLandingPad, callSiteFirstAction);
        ehTry->SetLSDACallSite(*lsdaCallSite);
        lsdaCallSiteTable->PushBack(*lsdaCallSite);
    }
}

void EHFunc::CreateLSDA()
{
    constexpr uint8 callSiteCleanUpAction = 0x0;
    /* create header */
    CreateLSDAHeader();
    /* create and fill callsite table */
    FillLSDACallSiteTable();

    for (auto *rethrow : rethrowVec) {
        DEBUG_ASSERT(rethrow != nullptr, "null ptr check");
        if (rethrow->HasLSDA()) {
            LSDACallSite *lsdaCallSite = cgFunc->GetMemoryPool()->New<LSDACallSite>();
            LabelPair csStart(cgFunc->GetStartLabel(), rethrow->GetStartLabel());
            LabelPair csLength(rethrow->GetStartLabel(), rethrow->GetEndLabel());
            LabelPair csLandingPad(nullptr, nullptr);
            lsdaCallSite->Init(csStart, csLength, csLandingPad, callSiteCleanUpAction);
            lsdaCallSiteTable->PushBack(*lsdaCallSite);
        }
    }

    /* LSDAAction table */
    CreateLSDAAction();
}

void EHFunc::CreateLSDAAction()
{
    constexpr uint8 actionTableNextEncoding = 0x7d;
    /* iterate each try and its corresponding catch */
    LSDAActionTable *actionTable = cgFunc->GetMemoryPool()->New<LSDAActionTable>(*cgFunc->GetFuncScopeAllocator());
    lsdaActionTable = actionTable;

    for (auto *ehTry : tryVec) {
        LSDAAction *lastAction = nullptr;
        for (int32 j = static_cast<int32>(ehTry->GetCatchVecSize()) - 1; j >= 0; --j) {
            CatchNode *catchNode = ehTry->GetCatchNodeAt(j);
            DEBUG_ASSERT(catchNode != nullptr, "null ptr check");
            for (uint32 idx = 0; idx < catchNode->Size(); ++idx) {
                MIRPtrType *ptType = static_cast<MIRPtrType *>(
                    GlobalTables::GetTypeTable().GetTypeFromTyIdx(catchNode->GetExceptionTyIdxVecElement(idx)));
                uint32 tyIndex = ty2IndexTable[ptType->GetPointedTyIdx()]; /* get the index of ptType of ehTyTable; */
                DEBUG_ASSERT(tyIndex != 0, "exception type index not allow equal zero");
                LSDAAction *lsdaAction = cgFunc->GetMemoryPool()->New<LSDAAction>(
                    tyIndex, lastAction == nullptr ? 0 : actionTableNextEncoding);
                lastAction = lsdaAction;
                actionTable->PushBack(*lsdaAction);
            }
        }
        
        CHECK_FATAL(actionTable->Size(), "must not be zero");
        /* record actionTable group offset, per LSDAAction object in actionTable occupy 2 bytes */
        ehTry->SetCSAction((actionTable->Size() - 1) * 2 + 1);
    }
}

bool CgBuildEHFunc::PhaseRun(maplebe::CGFunc &f)
{
    f.BuildEHFunc();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgBuildEHFunc, buildehfunc)
} /* namespace maplebe */
