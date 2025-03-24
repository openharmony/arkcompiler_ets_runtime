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

#include "lower.h"
#include "switch_lowerer.h"

namespace maplebe {

using namespace maple;

#define TARGARM32 0

// input node must be cvt, retype, zext or sext
BaseNode *CGLowerer::LowerCastExpr(BaseNode &expr)
{
    return &expr;
}

#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
BlockNode *CGLowerer::LowerReturnStructUsingFakeParm(NaryStmtNode &retNode)
{
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    for (size_t i = 0; i < retNode.GetNopndSize(); ++i) {
        retNode.SetOpnd(LowerExpr(retNode, *retNode.GetNopndAt(i), *blk), i);
    }
    BaseNode *opnd0 = retNode.Opnd(0);
    /* It is possible function never returns and have a dummy return const instead of a struct. */
    maple::LogInfo::MapleLogger(kLlWarn) << "return struct should have a kid" << std::endl;

    MIRFunction *curFunc = GetCurrentFunc();
    MIRSymbol *retSt = curFunc->GetFormal(0);
    MIRPtrType *retTy = static_cast<MIRPtrType *>(retSt->GetType());
    IassignNode *iassign = mirModule.CurFuncCodeMemPool()->New<IassignNode>();
    iassign->SetTyIdx(retTy->GetTypeIndex());
    DEBUG_ASSERT(opnd0 != nullptr, "opnd0 should not be nullptr");
    iassign->SetFieldID(0);
    iassign->SetRHS(opnd0);
    if (retSt->IsPreg()) {
        RegreadNode *regNode = mirModule.GetMIRBuilder()->CreateExprRegread(
            GetLoweredPtrType(), curFunc->GetPregTab()->GetPregIdxFromPregno(retSt->GetPreg()->GetPregNo()));
        iassign->SetOpnd(regNode, 0);
    } else {
        AddrofNode *dreadNode = mirModule.CurFuncCodeMemPool()->New<AddrofNode>(OP_dread);
        dreadNode->SetPrimType(GetLoweredPtrType());
        dreadNode->SetStIdx(retSt->GetStIdx());
        iassign->SetOpnd(dreadNode, 0);
    }
    blk->AddStatement(iassign);
    retNode.GetNopnd().clear();
    retNode.SetNumOpnds(0);
    blk->AddStatement(&retNode);
    return blk;
}

#endif /* TARGARM32 || TARGAARCH64 || TARGX86_64 */

BlockNode *CGLowerer::LowerReturn(NaryStmtNode &retNode)
{
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    if (retNode.NumOpnds() != 0) {
        BaseNode *expr = retNode.Opnd(0);
        Opcode opr = expr->GetOpCode();
        if (opr == OP_dread) {
            AddrofNode *retExpr = static_cast<AddrofNode *>(expr);
            MIRFunction *mirFunc = mirModule.CurFunction();
            MIRSymbol *sym = mirFunc->GetLocalOrGlobalSymbol(retExpr->GetStIdx());
            if (sym->GetAttr(ATTR_localrefvar)) {
                mirFunc->InsertMIRSymbol(sym);
            }
        }
    }
    for (size_t i = 0; i < retNode.GetNopndSize(); ++i) {
        retNode.SetOpnd(LowerExpr(retNode, *retNode.GetNopndAt(i), *blk), i);
    }
    blk->AddStatement(&retNode);
    return blk;
}

void CGLowerer::LowerIassign(IassignNode &iassign, BlockNode &newBlk)
{
    StmtNode *newStmt = nullptr;
    CHECK_FATAL(iassign.GetFieldID() == 0, "fieldID must be 0");
    LowerStmt(iassign, newBlk);
    newStmt = &iassign;
    newBlk.AddStatement(newStmt);
}

BaseNode *CGLowerer::NeedRetypeWhenLowerCallAssigned(PrimType pType)
{
    BaseNode *retNode = mirModule.GetMIRBuilder()->CreateExprRegread(pType, -kSregRetval0);
    if (IsPrimitiveInteger(pType) && GetPrimTypeBitSize(pType) <= k32BitSize) {
        auto newPty = IsPrimitiveUnsigned(pType) ? PTY_u64 : PTY_i64;
        retNode = mirModule.GetMIRBuilder()->CreateExprTypeCvt(OP_cvt, newPty, pType, *retNode);
    }
    return retNode;
}

DassignNode *CGLowerer::SaveReturnValueInLocal(StIdx stIdx, uint16 fieldID)
{
    MIRSymbol *var;
    if (stIdx.IsGlobal()) {
        var = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    } else {
        DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
        var = GetCurrentFunc()->GetSymbolTabItem(stIdx.Idx());
    }
    CHECK_FATAL(var != nullptr, "var should not be nullptr");
    PrimType pType;
    if (var->GetAttr(ATTR_oneelem_simd)) {
        pType = PTY_f64;
    } else {
        pType = GlobalTables::GetTypeTable().GetTypeTable().at(var->GetTyIdx())->GetPrimType();
    }
    auto *regRead = NeedRetypeWhenLowerCallAssigned(pType);
    return mirModule.GetMIRBuilder()->CreateStmtDassign(*var, fieldID, regRead);
}

/* to lower call (including icall) and intrinsicall statements */
void CGLowerer::LowerCallStmt(StmtNode &stmt, StmtNode *&nextStmt, BlockNode &newBlk, MIRType *retty, bool uselvar,
                              bool isIntrinAssign)
{
    StmtNode *newStmt = nullptr;
    if (stmt.GetOpCode() == OP_intrinsiccall) {
        auto &intrnNode = static_cast<IntrinsiccallNode &>(stmt);
        if (intrnNode.GetIntrinsic() == maple::INTRN_JS_PURE_CALL) {
            newStmt = &stmt;
        } else {
            newStmt = LowerIntrinsiccall(intrnNode, newBlk);
        }
    } else {
        /* We note the function has a user-defined (i.e., not an intrinsic) call. */
        DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
        GetCurrentFunc()->SetHasCall();
        newStmt = &stmt;
    }

    if (newStmt == nullptr) {
        return;
    }

    if (newStmt->GetOpCode() == OP_call || newStmt->GetOpCode() == OP_icall || newStmt->GetOpCode() == OP_icallproto) {
        auto &callNode = static_cast<NaryStmtNode&>(*newStmt);
        for (size_t i = 0; i < callNode.GetNopndSize(); ++i) {
            BaseNode *newOpnd = LowerExpr(callNode, *callNode.GetNopndAt(i), newBlk);
            callNode.SetOpnd(newOpnd, i);
        }
        newStmt = &callNode;
    }
    newStmt->SetSrcPos(stmt.GetSrcPos());
    newBlk.AddStatement(newStmt);
}

StmtNode *CGLowerer::GenCallNode(const StmtNode &stmt, PUIdx &funcCalled, CallNode &origCall)
{
    CallNode *newCall = nullptr;
    if (stmt.GetOpCode() == OP_callassigned) {
        newCall = mirModule.GetMIRBuilder()->CreateStmtCall(origCall.GetPUIdx(), origCall.GetNopnd());
    }
    CHECK_FATAL(newCall != nullptr, "nullptr is not expected");
    newCall->SetDeoptBundleInfo(origCall.GetDeoptBundleInfo());
    newCall->SetSrcPos(stmt.GetSrcPos());
    funcCalled = origCall.GetPUIdx();
    CHECK_FATAL((newCall->GetOpCode() == OP_call),
                "virtual call or super class call are not expected");
    newCall->SetStmtAttrs(stmt.GetStmtAttrs());
    return newCall;
}

StmtNode *CGLowerer::GenIntrinsiccallNode(const StmtNode &stmt, PUIdx &funcCalled, bool &handledAtLowerLevel,
                                          IntrinsiccallNode &origCall)
{
    StmtNode *newCall = nullptr;
    handledAtLowerLevel = IsIntrinsicCallHandledAtLowerLevel(origCall.GetIntrinsic());
    if (handledAtLowerLevel) {
        /* If the lower level can handle the intrinsic, just let it pass through. */
        newCall = &origCall;
    } else {
        PUIdx bFunc = GetBuiltinToUse(origCall.GetIntrinsic());
        if (bFunc != kFuncNotFound) {
            newCall = mirModule.GetMIRBuilder()->CreateStmtCall(bFunc, origCall.GetNopnd());
            CHECK_FATAL(newCall->GetOpCode() == OP_call, "intrinsicnode except intrinsiccall is not expected");
        } else {
            if (stmt.GetOpCode() == OP_intrinsiccallassigned) {
                newCall =
                    mirModule.GetMIRBuilder()->CreateStmtIntrinsicCall(origCall.GetIntrinsic(), origCall.GetNopnd());
                CHECK_FATAL(newCall->GetOpCode() == OP_intrinsiccall,
                            "intrinsicnode except intrinsiccall is not expected");
            } else {
                newCall = mirModule.GetMIRBuilder()->CreateStmtIntrinsicCall(origCall.GetIntrinsic(),
                                                                             origCall.GetNopnd(), origCall.GetTyIdx());
                CHECK_FATAL(newCall->GetOpCode() == OP_intrinsiccallwithtype,
                            "intrinsicnode except OP_intrinsiccallwithtype is not expected");
            }
        }
        newCall->SetSrcPos(stmt.GetSrcPos());
        funcCalled = bFunc;
    }
    return newCall;
}

StmtNode *CGLowerer::GenIcallNode(PUIdx &funcCalled, IcallNode &origCall)
{
    IcallNode *newCall = nullptr;
    if (origCall.GetOpCode() == OP_icallassigned) {
        newCall = mirModule.GetMIRBuilder()->CreateStmtIcall(origCall.GetNopnd());
    } else {
        newCall = mirModule.GetMIRBuilder()->CreateStmtIcallproto(origCall.GetNopnd(), origCall.GetRetTyIdx());
        newCall->SetRetTyIdx(static_cast<IcallNode &>(origCall).GetRetTyIdx());
    }
    newCall->SetDeoptBundleInfo(origCall.GetDeoptBundleInfo());
    newCall->SetStmtAttrs(origCall.GetStmtAttrs());
    newCall->SetSrcPos(origCall.GetSrcPos());
    CHECK_FATAL(newCall != nullptr, "nullptr is not expected");
    funcCalled = kFuncNotFound;
    return newCall;
}

BlockNode *CGLowerer::GenBlockNode(StmtNode &newCall, const CallReturnVector &p2nRets, const Opcode &opcode,
                                   const PUIdx &funcCalled, bool handledAtLowerLevel, bool uselvar)
{
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    blk->AddStatement(&newCall);
    if (!handledAtLowerLevel) {
        CHECK_FATAL(p2nRets.size() <= 1, "make sure p2nRets size <= 1");
        /* Create DassignStmt to save kSregRetval0. */
        StmtNode *dStmt = nullptr;
        MIRType *retType = nullptr;
        if (p2nRets.size() == 1) {
            MIRSymbol *sym = nullptr;
            StIdx stIdx = p2nRets[0].first;
            if (stIdx.IsGlobal()) {
                sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
            } else {
                DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
                sym = GetCurrentFunc()->GetSymbolTabItem(stIdx.Idx());
            }
            bool sizeIs0 = false;
            if (sym != nullptr) {
                retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
                if (beCommon.GetTypeSize(retType->GetTypeIndex().GetIdx()) == 0) {
                    sizeIs0 = true;
                }
            }
            if (!sizeIs0) {
                RegFieldPair regFieldPair = p2nRets[0].second;
                if (!regFieldPair.IsReg()) {
                    uint16 fieldID = static_cast<uint16>(regFieldPair.GetFieldID());
                    DassignNode *dn = SaveReturnValueInLocal(stIdx, fieldID);
                    CHECK_FATAL(dn->GetFieldID() == 0, "make sure dn's fieldID return 0");
                    LowerDassign(*dn, *blk);
                    CHECK_FATAL(&newCall == blk->GetLast() || newCall.GetNext() == blk->GetLast(), "");
                    dStmt = (&newCall == blk->GetLast()) ? nullptr : blk->GetLast();
                    CHECK_FATAL(newCall.GetNext() == dStmt, "make sure newCall's next equal dStmt");
                } else {
                    PregIdx pregIdx = static_cast<PregIdx>(regFieldPair.GetPregIdx());
                    DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
                    MIRPreg *mirPreg = GetCurrentFunc()->GetPregTab()->PregFromPregIdx(pregIdx);
                    PrimType pType = mirPreg->GetPrimType();
                    RegreadNode *regNode = mirModule.GetMIRBuilder()->CreateExprRegread(pType, -kSregRetval0);
                    RegassignNode *regAssign;

                    regAssign = mirModule.GetMIRBuilder()->CreateStmtRegassign(
                        mirPreg->GetPrimType(), regFieldPair.GetPregIdx(), regNode);
                    blk->AddStatement(regAssign);
                    dStmt = regAssign;
                }
            }
        }
        blk->ResetBlock();
        /* if VerboseCG, insert a comment */
        if (ShouldAddAdditionalComment()) {
            CommentNode *cmnt = mirModule.CurFuncCodeMemPool()->New<CommentNode>(mirModule);
            cmnt->SetComment(kOpcodeInfo.GetName(opcode).c_str());
            if (funcCalled == kFuncNotFound) {
                cmnt->Append(" : unknown");
            } else {
                cmnt->Append(" : ");
                cmnt->Append(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(funcCalled)->GetName());
            }
            blk->AddStatement(cmnt);
        }
        CHECK_FATAL(dStmt == nullptr || dStmt->GetNext() == nullptr, "make sure dStmt or dStmt's next is nullptr");
        LowerCallStmt(newCall, dStmt, *blk, retType, uselvar, opcode == OP_intrinsiccallassigned);
        if (!uselvar && dStmt != nullptr) {
            dStmt->SetSrcPos(newCall.GetSrcPos());
            blk->AddStatement(dStmt);
        }
    }
    return blk;
}

BlockNode *CGLowerer::LowerIntrinsiccallAassignedToAssignStmt(IntrinsiccallNode &intrinsicCall)
{
    auto *builder = mirModule.GetMIRBuilder();
    auto *block = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    auto intrinsicID = intrinsicCall.GetIntrinsic();
    auto &opndVector = intrinsicCall.GetNopnd();
    auto returnPair = intrinsicCall.GetReturnVec().begin();
    auto regFieldPair = returnPair->second;
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "CurFunction should not be nullptr");
    if (regFieldPair.IsReg()) {
        auto regIdx = regFieldPair.GetPregIdx();
        auto primType = mirModule.CurFunction()->GetPregItem(static_cast<PregIdx>(regIdx))->GetPrimType();
        auto intrinsicOp = builder->CreateExprIntrinsicop(intrinsicID, OP_intrinsicop, primType, TyIdx(0), opndVector);
        auto regAssign = builder->CreateStmtRegassign(primType, regIdx, intrinsicOp);
        block->AddStatement(regAssign);
    } else {
        auto fieldID = regFieldPair.GetFieldID();
        auto stIdx = returnPair->first;
        DEBUG_ASSERT(mirModule.CurFunction()->GetLocalOrGlobalSymbol(stIdx) != nullptr, "nullptr check");
        auto *type = mirModule.CurFunction()->GetLocalOrGlobalSymbol(stIdx)->GetType();
        auto intrinsicOp = builder->CreateExprIntrinsicop(intrinsicID, OP_intrinsicop, *type, opndVector);
        auto dAssign = builder->CreateStmtDassign(stIdx, fieldID, intrinsicOp);
        block->AddStatement(dAssign);
    }
    return LowerBlock(*block);
}

BlockNode *CGLowerer::LowerCallAssignedStmt(StmtNode &stmt, bool uselvar)
{
    StmtNode *newCall = nullptr;
    CallReturnVector *p2nRets = nullptr;
    PUIdx funcCalled = kFuncNotFound;
    bool handledAtLowerLevel = false;
    switch (stmt.GetOpCode()) {
        case OP_callassigned: {
            auto &origCall = static_cast<CallNode &>(stmt);
            newCall = GenCallNode(stmt, funcCalled, origCall);
            p2nRets = &origCall.GetReturnVec();
            static_cast<CallNode *>(newCall)->SetReturnVec(*p2nRets);
            MIRFunction *curFunc = mirModule.CurFunction();
            curFunc->SetLastFreqMap(newCall->GetStmtID(),
                                    static_cast<uint32>(curFunc->GetFreqFromLastStmt(stmt.GetStmtID())));
            break;
        }
        case OP_intrinsiccallassigned: {
            BlockNode *blockNode = LowerIntrinsiccallToIntrinsicop(stmt);
            if (blockNode) {
                return blockNode;
            }
            IntrinsiccallNode &intrincall = static_cast<IntrinsiccallNode &>(stmt);
            newCall = GenIntrinsiccallNode(stmt, funcCalled, handledAtLowerLevel, intrincall);
            p2nRets = &intrincall.GetReturnVec();
            static_cast<IntrinsiccallNode *>(newCall)->SetReturnVec(*p2nRets);
            break;
        }
        case OP_icallprotoassigned:
        case OP_icallassigned: {
            auto &origCall = static_cast<IcallNode &>(stmt);
            newCall = GenIcallNode(funcCalled, origCall);
            p2nRets = &origCall.GetReturnVec();
            static_cast<IcallNode *>(newCall)->SetReturnVec(*p2nRets);
            break;
        }
        default:
            CHECK_FATAL(false, "NIY");
            return nullptr;
    }

    /* transfer srcPosition location info */
    newCall->SetSrcPos(stmt.GetSrcPos());
    return GenBlockNode(*newCall, *p2nRets, stmt.GetOpCode(), funcCalled, handledAtLowerLevel, uselvar);
}

BlockNode *CGLowerer::LowerIntrinsiccallToIntrinsicop(StmtNode &stmt)
{
    return nullptr;
}

void CGLowerer::LowerStmt(StmtNode &stmt, BlockNode &newBlk)
{
    for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        DEBUG_ASSERT(stmt.Opnd(i) != nullptr, "null ptr check");
        stmt.SetOpnd(LowerExpr(stmt, *stmt.Opnd(i), newBlk), i);
    }
}

void CGLowerer::LowerSwitchOpnd(StmtNode &stmt, BlockNode &newBlk)
{
    BaseNode *opnd = LowerExpr(stmt, *stmt.Opnd(0), newBlk);
    if (CGOptions::GetInstance().GetOptimizeLevel() >= CGOptions::kLevel2 && opnd->GetOpCode() != OP_regread) {
        PrimType ptyp = stmt.Opnd(0)->GetPrimType();
        DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
        PregIdx pIdx = GetCurrentFunc()->GetPregTab()->CreatePreg(ptyp);
        RegassignNode *regAss = mirBuilder->CreateStmtRegassign(ptyp, pIdx, opnd);
        newBlk.AddStatement(regAss);
        GetCurrentFunc()->SetLastFreqMap(regAss->GetStmtID(),
                                         static_cast<uint32>(GetCurrentFunc()->GetFreqFromLastStmt(stmt.GetStmtID())));
        stmt.SetOpnd(mirBuilder->CreateExprRegread(ptyp, pIdx), 0);
    } else {
        stmt.SetOpnd(LowerExpr(stmt, *stmt.Opnd(0), newBlk), 0);
    }
}

BlockNode *CGLowerer::LowerBlock(BlockNode &block)
{
    BlockNode *newBlk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    BlockNode *tmpBlockNode = nullptr;
    std::vector<StmtNode *> abortNode;
    if (block.GetFirst() == nullptr) {
        return newBlk;
    }

    StmtNode *nextStmt = block.GetFirst();
    do {
        StmtNode *stmt = nextStmt;
        nextStmt = stmt->GetNext();
        stmt->SetNext(nullptr);
        currentBlock = newBlk;

        switch (stmt->GetOpCode()) {
            case OP_switch: {
                LowerSwitchOpnd(*stmt, *newBlk);
                auto switchMp = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "switchlowere");
                MapleAllocator switchAllocator(switchMp.get());
                SwitchLowerer switchLowerer(mirModule, static_cast<SwitchNode &>(*stmt), switchAllocator);
                BlockNode *blk = switchLowerer.LowerSwitch();
                if (blk->GetFirst() != nullptr) {
                    newBlk->AppendStatementsFromBlock(*blk);
                }
                needBranchCleanup = true;
                break;
            }
            case OP_block:
                tmpBlockNode = LowerBlock(static_cast<BlockNode &>(*stmt));
                CHECK_FATAL(tmpBlockNode != nullptr, "nullptr is not expected");
                newBlk->AppendStatementsFromBlock(*tmpBlockNode);
                break;
            case OP_dassign: {
                LowerDassign(static_cast<DassignNode &>(*stmt), *newBlk);
                break;
            }
            case OP_regassign: {
                LowerRegassign(static_cast<RegassignNode &>(*stmt), *newBlk);
                break;
            }
            case OP_iassign: {
                LowerIassign(static_cast<IassignNode &>(*stmt), *newBlk);
                break;
            }
            case OP_callassigned:
            case OP_icallassigned:
            case OP_icallprotoassigned: {
                // pass the addr of lvar if this is a struct call assignment
                bool lvar = false;
                newBlk->AppendStatementsFromBlock(*LowerCallAssignedStmt(*stmt, lvar));
                break;
            }
            case OP_intrinsiccallassigned:
                newBlk->AppendStatementsFromBlock(*LowerCallAssignedStmt(*stmt));
                break;
            case OP_intrinsiccall:
            case OP_call:
            case OP_icall:
            case OP_icallproto:
#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
                // nextStmt could be changed by the call to LowerStructReturn
                LowerCallStmt(*stmt, nextStmt, *newBlk);
#else
                LowerStmt(*stmt, *newBlk);
#endif
                break;
            case OP_return: {
#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
                DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
                if (GetCurrentFunc()->IsFirstArgReturn() && stmt->NumOpnds() > 0) {
                    newBlk->AppendStatementsFromBlock(
                        *LowerReturnStructUsingFakeParm(static_cast<NaryStmtNode &>(*stmt)));
                } else {
#endif
                    NaryStmtNode *retNode = static_cast<NaryStmtNode *>(stmt);
                    if (retNode->GetNopndSize() == 0) {
                        newBlk->AddStatement(stmt);
                    } else {
                        tmpBlockNode = LowerReturn(*retNode);
                        CHECK_FATAL(tmpBlockNode != nullptr, "nullptr is not expected");
                        newBlk->AppendStatementsFromBlock(*tmpBlockNode);
                    }
#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
                }
#endif
                break;
            }
            case OP_comment:
                newBlk->AddStatement(stmt);
                break;
            default:
                LowerStmt(*stmt, *newBlk);
                newBlk->AddStatement(stmt);
                break;
        }
        CHECK_FATAL(beCommon.GetSizeOfTypeSizeTable() == GlobalTables::GetTypeTable().GetTypeTableSize(), "Error!");
    } while (nextStmt != nullptr);
    for (auto node : abortNode) {
        newBlk->AddStatement(node);
    }
    return newBlk;
}

void CGLowerer::LowerTypePtr(BaseNode &node) const
{
    if ((node.GetPrimType() == PTY_ptr) || (node.GetPrimType() == PTY_ref)) {
        node.SetPrimType(GetLoweredPtrType());
    }

    if (kOpcodeInfo.IsTypeCvt(node.GetOpCode())) {
        auto &cvt = static_cast<TypeCvtNode &>(node);
        if ((cvt.FromType() == PTY_ptr) || (cvt.FromType() == PTY_ref)) {
            cvt.SetFromType(GetLoweredPtrType());
        }
    } else if (kOpcodeInfo.IsCompare(node.GetOpCode())) {
        auto &cmp = static_cast<CompareNode &>(node);
        if ((cmp.GetOpndType() == PTY_ptr) || (cmp.GetOpndType() == PTY_ref)) {
            cmp.SetOpndType(GetLoweredPtrType());
        }
    }
}

void CGLowerer::LowerEntry(MIRFunction &func)
{
    if (func.IsFirstArgReturn() && func.GetReturnType()->GetPrimType() != PTY_void) {
        MIRSymbol *retSt = func.GetSymTab()->CreateSymbol(kScopeLocal);
        retSt->SetStorageClass(kScFormal);
        retSt->SetSKind(kStVar);
        std::string retName(".return.");
        MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
        DEBUG_ASSERT(funcSt != nullptr, "null ptr check");
        retName += funcSt->GetName();
        retSt->SetNameStrIdx(retName);
        MIRType *pointType = beCommon.BeGetOrCreatePointerType(*func.GetReturnType());

        retSt->SetTyIdx(pointType->GetTypeIndex());
        std::vector<MIRSymbol *> formals;
        formals.emplace_back(retSt);
        for (uint32 i = 0; i < func.GetFormalCount(); ++i) {
            auto formal = func.GetFormal(i);
            formals.emplace_back(formal);
        }
        func.SetFirstArgReturn();

        beCommon.AddElementToFuncReturnType(func, func.GetReturnTyIdx());

        func.UpdateFuncTypeAndFormalsAndReturnType(formals, TyIdx(PTY_void), true);
        auto *funcType = func.GetMIRFuncType();
        DEBUG_ASSERT(funcType != nullptr, "null ptr check");
        funcType->SetFirstArgReturn();
        beCommon.AddTypeSizeAndAlign(funcType->GetTypeIndex(), GetPrimTypeSize(funcType->GetPrimType()));
    }
}

void CGLowerer::CleanupBranches(MIRFunction &func) const
{
    BlockNode *block = func.GetBody();
    StmtNode *prev = nullptr;
    StmtNode *next = nullptr;
    for (StmtNode *curr = block->GetFirst(); curr != nullptr; curr = next) {
        next = curr->GetNext();
        if (next != nullptr) {
            CHECK_FATAL(curr == next->GetPrev(), "unexpected node");
        }
        if ((next != nullptr) && (prev != nullptr) && (curr->GetOpCode() == OP_goto)) {
            /*
             * Skip until find a label.
             * Note that the CURRent 'goto' statement may be the last statement
             * when discounting comment statements.
             * Make sure we don't lose any comments.
             */
            StmtNode *cmtB = nullptr;
            StmtNode *cmtE = nullptr;
            next = curr->GetNext();

            while ((next != nullptr) && (next->GetOpCode() != OP_label)) {
                if (next->GetOpCode() == OP_comment) {
                    if (cmtB == nullptr) {
                        cmtB = next;
                        cmtE = next;
                    } else {
                        CHECK_FATAL(cmtE != nullptr, "cmt_e is null in CGLowerer::CleanupBranches");
                        cmtE->SetNext(next);
                        next->SetPrev(cmtE);
                        cmtE = next;
                    }
                }
                next = next->GetNext();
            }

            curr->SetNext(next);

            if (next != nullptr) {
                next->SetPrev(curr);
            }

            StmtNode *insertAfter = nullptr;

            if ((next != nullptr) &&
                ((static_cast<GotoNode *>(curr))->GetOffset() == (static_cast<LabelNode *>(next))->GetLabelIdx())) {
                insertAfter = prev;
                prev->SetNext(next); /* skip goto statement (which is pointed by curr) */
                next->SetPrev(prev);
                curr = next;            /* make curr point to the label statement */
                next = next->GetNext(); /* advance next to the next statement of the label statement */
            } else {
                insertAfter = curr;
            }

            /* insert comments before 'curr' */
            if (cmtB != nullptr) {
                CHECK_FATAL(cmtE != nullptr, "nullptr is not expected");
                StmtNode *iaNext = insertAfter->GetNext();
                if (iaNext != nullptr) {
                    iaNext->SetPrev(cmtE);
                }
                cmtE->SetNext(iaNext);

                insertAfter->SetNext(cmtB);
                cmtB->SetPrev(insertAfter);

                if (insertAfter == curr) {
                    curr = cmtE;
                }
            }
            if (next == nullptr) {
                func.GetBody()->SetLast(curr);
            }
        }
        prev = curr;
    }
    CHECK_FATAL(func.GetBody()->GetLast() == prev, "make sure the return value of GetLast equal prev");
}

inline bool IsAccessingTheSameMemoryLocation(const DassignNode &dassign, const RegreadNode &rRead,
                                             const CGLowerer &cgLowerer)
{
    StIdx stIdx = cgLowerer.GetSymbolReferredToByPseudoRegister(rRead.GetRegIdx());
    return ((dassign.GetStIdx() == stIdx) && (dassign.GetFieldID() == 0));
}

inline bool IsAccessingTheSameMemoryLocation(const DassignNode &dassign, const DreadNode &dread)
{
    return ((dassign.GetStIdx() == dread.GetStIdx()) && (dassign.GetFieldID() == dread.GetFieldID()));
}

inline bool IsDassignNOP(const DassignNode &dassign)
{
    if (dassign.GetRHS()->GetOpCode() == OP_dread) {
        return IsAccessingTheSameMemoryLocation(dassign, static_cast<DreadNode &>(*dassign.GetRHS()));
    }
    return false;
}

inline bool IsConstvalZero(const BaseNode &n)
{
    return ((n.GetOpCode() == OP_constval) && static_cast<const ConstvalNode &>(n).GetConstVal()->IsZero());
}

#define NEXT_ID(x) ((x) + 1)
#define INTRN_FIRST_SYNC_ENTER NEXT_ID(INTRN_LAST)
#define INTRN_SECOND_SYNC_ENTER NEXT_ID(INTRN_FIRST_SYNC_ENTER)
#define INTRN_THIRD_SYNC_ENTER NEXT_ID(INTRN_SECOND_SYNC_ENTER)
#define INTRN_FOURTH_SYNC_ENTER NEXT_ID(INTRN_THIRD_SYNC_ENTER)
#define INTRN_YNC_EXIT NEXT_ID(INTRN_FOURTH_SYNC_ENTER)

std::vector<std::pair<CGLowerer::BuiltinFunctionID, PUIdx>> CGLowerer::builtinFuncIDs;

LabelIdx CGLowerer::GetLabelIdx(MIRFunction &curFunc) const
{
    std::string suffix = std::to_string(curFunc.GetLabelTab()->GetLabelTableSize());
    GStrIdx labelStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("__label_BC_" + suffix);
    LabelIdx labIdx = curFunc.GetLabelTab()->AddLabel(labelStrIdx);
    return labIdx;
}

BaseNode *CGLowerer::LowerExpr(BaseNode &parent, BaseNode &expr, BlockNode &blkNode)
{
    bool isCvtU1Expr = (expr.GetOpCode() == OP_cvt && expr.GetPrimType() == PTY_u1 &&
                        static_cast<TypeCvtNode &>(expr).FromType() != PTY_u1);
    if (expr.GetPrimType() == PTY_u1) {
        expr.SetPrimType(PTY_u8);
    }

    {
        for (size_t i = 0; i < expr.NumOpnds(); ++i) {
            expr.SetOpnd(LowerExpr(expr, *expr.Opnd(i), blkNode), i);
        }
    }
    // Convert `cvt u1 xx <expr>` to `ne u8 xx (<expr>, constval xx 0)`
    // No need to convert `cvt u1 u1 <expr>`
    if (isCvtU1Expr) {
        auto &cvtExpr = static_cast<TypeCvtNode &>(expr);
        PrimType fromType = cvtExpr.FromType();
        auto *fromMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fromType));
        // We use u8 instead of u1 because codegen can't recognize u1
        auto *toMIRType = GlobalTables::GetTypeTable().GetUInt8();
        auto *zero = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *fromMIRType);
        auto *converted = mirBuilder->CreateExprCompare(OP_ne, *toMIRType, *fromMIRType, cvtExpr.Opnd(0),
                                                        mirBuilder->CreateConstval(zero));
        return converted;
    }
    switch (expr.GetOpCode()) {
        case OP_dread:
            return LowerDread(static_cast<DreadNode &>(expr), blkNode);

        case OP_addrof:
            return LowerAddrof(static_cast<AddrofNode &>(expr));

        case OP_iread:
            return LowerIread(static_cast<IreadNode &>(expr));

        case OP_cvt:
        case OP_retype:
        case OP_zext:
        case OP_sext:
            return LowerCastExpr(expr);
        default:
            return &expr;
    }
}

BaseNode *CGLowerer::LowerDread(DreadNode &dread, const BlockNode &block)
{
    /* use PTY_u8 for boolean type in dread/iread */
    if (dread.GetPrimType() == PTY_u1) {
        dread.SetPrimType(PTY_u8);
    }
    CHECK_FATAL(dread.GetFieldID() == 0, "fieldID must be 0");
    return LowerDreadToThreadLocal(dread, block);
}

void CGLowerer::LowerRegassign(RegassignNode &regNode, BlockNode &newBlk)
{
    BaseNode *rhsOpnd = regNode.Opnd(0);
    regNode.SetOpnd(LowerExpr(regNode, *rhsOpnd, newBlk), 0);
    newBlk.AddStatement(&regNode);
}

BaseNode *CGLowerer::ExtractSymbolAddress(const StIdx &stIdx)
{
    auto builder = mirModule.GetMIRBuilder();
    return builder->CreateExprAddrof(0, stIdx);
}

BaseNode *CGLowerer::LowerDreadToThreadLocal(BaseNode &expr, const BlockNode &block)
{
    auto *result = &expr;
    if (expr.GetOpCode() != maple::OP_dread) {
        return result;
    }
    uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    auto dread = static_cast<DreadNode &>(expr);
    StIdx stIdx = dread.GetStIdx();
    if (!stIdx.IsGlobal()) {
        return result;
    }
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    CHECK_FATAL(symbol != nullptr, "symbol should not be nullptr");

    if (symbol->IsThreadLocal()) {
        //  iread <* u32> 0 (regread u64 %addr)
        auto addr = ExtractSymbolAddress(stIdx);
        auto ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*symbol->GetType());
        auto iread = mirModule.GetMIRBuilder()->CreateExprIread(*symbol->GetType(), *ptrType, dread.GetFieldID(), addr);
        result = iread;
    }
    uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    if (newTypeTableSize != oldTypeTableSize) {
        beCommon.AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
    return result;
}

StmtNode *CGLowerer::LowerDassignToThreadLocal(StmtNode &stmt, const BlockNode &block)
{
    StmtNode *result = &stmt;
    if (stmt.GetOpCode() != maple::OP_dassign) {
        return result;
    }
    uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    auto dAssign = static_cast<DassignNode &>(stmt);
    StIdx stIdx = dAssign.GetStIdx();
    if (!stIdx.IsGlobal()) {
        return result;
    }
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    DEBUG_ASSERT(symbol != nullptr, "symbol should not be nullptr");
    if (symbol->IsThreadLocal()) {
        //  iassign <* u32> 0 (regread u64 %addr, dread u32 $x)
        auto addr = ExtractSymbolAddress(stIdx);
        auto ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*symbol->GetType());
        auto iassign =
            mirModule.GetMIRBuilder()->CreateStmtIassign(*ptrType, dAssign.GetFieldID(), addr, dAssign.GetRHS());
        result = iassign;
    }
    uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    if (newTypeTableSize != oldTypeTableSize) {
        beCommon.AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
    return result;
}

void CGLowerer::LowerDassign(DassignNode &dsNode, BlockNode &newBlk)
{
    StmtNode *newStmt = nullptr;
    BaseNode *rhs = nullptr;
    Opcode op = dsNode.GetRHS()->GetOpCode();
    CHECK_FATAL(dsNode.GetFieldID() == 0, "fieldID must be 0");
    if (op == OP_intrinsicop) {
        IntrinsicopNode *intrinNode = static_cast<IntrinsicopNode *>(dsNode.GetRHS());
        MIRType *retType = IntrinDesc::intrinTable[intrinNode->GetIntrinsic()].GetReturnType();
        CHECK_FATAL(retType != nullptr, "retType should not be nullptr");
        rhs = LowerExpr(dsNode, *intrinNode, newBlk);
        dsNode.SetRHS(rhs);
        CHECK_FATAL(dsNode.GetRHS() != nullptr, "dsNode->rhs is null in CGLowerer::LowerDassign");
        if (!IsDassignNOP(dsNode)) {
            newStmt = &dsNode;
        }
    } else {
        rhs = LowerExpr(dsNode, *dsNode.GetRHS(), newBlk);
        dsNode.SetRHS(rhs);
        newStmt = &dsNode;
    }

    if (newStmt != nullptr) {
        newBlk.AddStatement(LowerDassignToThreadLocal(*newStmt, newBlk));
    }
}

StmtNode *CGLowerer::LowerDefaultIntrinsicCall(IntrinsiccallNode &intrincall, MIRSymbol &st, MIRFunction &fn)
{
    MIRIntrinsicID intrnID = intrincall.GetIntrinsic();
    IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrnID];
    std::vector<TyIdx> funcTyVec;
    std::vector<TypeAttrs> fnTaVec;
    MapleVector<BaseNode *> &nOpnds = intrincall.GetNopnd();
    MIRType *retTy = intrinDesc->GetReturnType();
    CHECK_FATAL(retTy != nullptr, "retTy should not be nullptr");
    for (uint32 i = 0; i < nOpnds.size(); ++i) {
        MIRType *argTy = intrinDesc->GetArgType(i);
        CHECK_FATAL(argTy != nullptr, "argTy should not be nullptr");
        funcTyVec.emplace_back(argTy->GetTypeIndex());
        fnTaVec.emplace_back(TypeAttrs());
    }
    MIRType *funcType = beCommon.BeGetOrCreateFunctionType(retTy->GetTypeIndex(), funcTyVec, fnTaVec);
    st.SetTyIdx(funcType->GetTypeIndex());
    fn.SetMIRFuncType(static_cast<MIRFuncType *>(funcType));
    fn.SetReturnTyIdx(retTy->GetTypeIndex());
    return static_cast<CallNode *>(mirBuilder->CreateStmtCall(fn.GetPuidx(), nOpnds));
}

StmtNode *CGLowerer::LowerIntrinsiccall(IntrinsiccallNode &intrincall, BlockNode &newBlk)
{
    MIRIntrinsicID intrnID = intrincall.GetIntrinsic();
    for (size_t i = 0; i < intrincall.GetNumOpnds(); ++i) {
        intrincall.SetOpnd(LowerExpr(intrincall, *intrincall.Opnd(i), newBlk), i);
    }
    IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrnID];
    /* default lowers intrinsic call to real function call. */
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    CHECK_FATAL(intrinDesc->name != nullptr, "intrinsic's name should not be nullptr");
    const std::string name = intrinDesc->name;
    st->SetNameStrIdx(name);
    st->SetStorageClass(kScText);
    st->SetSKind(kStFunc);
    MIRFunction *fn = mirBuilder->GetOrCreateFunction(intrinDesc->name, TyIdx(0));
    beCommon.UpdateTypeTable(*fn->GetMIRFuncType());
    fn->AllocSymTab();
    st->SetFunction(fn);
    st->SetAppearsInCode(true);
    return LowerDefaultIntrinsicCall(intrincall, *st, *fn);
}

PUIdx CGLowerer::GetBuiltinToUse(BuiltinFunctionID id) const
{
    /*
     * use std::vector & linear search as the number of entries is small.
     * we may revisit it if the number of entries gets larger.
     */
    for (const auto &funcID : builtinFuncIDs) {
        if (funcID.first == id) {
            return funcID.second;
        }
    }
    return kFuncNotFound;
}

bool CGLowerer::IsIntrinsicCallHandledAtLowerLevel(MIRIntrinsicID intrinsic) const
{
    switch (intrinsic) {
        // js
        case INTRN_ADD_WITH_OVERFLOW:
        case INTRN_SUB_WITH_OVERFLOW:
        case INTRN_MUL_WITH_OVERFLOW:
        case INTRN_JS_PURE_CALL:
        case INTRN_HEAP_CONSTANT:
        case INTRN_GET_HEAP_CONSTANT_TABLE:
            return true;
        default: {
            return false;
        }
    }
}

void CGLowerer::LowerFunc(MIRFunction &func)
{
    labelIdx = 0;
    SetCurrentFunc(&func);
    LowerEntry(func);
    BlockNode *origBody = func.GetBody();
    CHECK_FATAL(origBody != nullptr, "origBody should not be nullptr");

    BlockNode *newBody = LowerBlock(*origBody);
    func.SetBody(newBody);
    if (needBranchCleanup) {
        CleanupBranches(func);
    }

    uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    if (newTypeTableSize != oldTypeTableSize) {
        beCommon.AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
}
} /* namespace maplebe */
