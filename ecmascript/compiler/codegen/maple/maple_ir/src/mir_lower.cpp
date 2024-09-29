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

#include "mir_lower.h"
#include "constantfold.h"
#include "me_option.h"

#define DO_LT_0_CHECK 1

namespace maple {
// Remove intrinsicop __builtin_expect and record likely info to brStmt
// Target condExpr example:
//  ne u1 i64 (
//    intrinsicop i64 C___builtin_expect (
//     cvt i64 i32 (dread i32 %levVar_9354), cvt i64 i32 (constval i32 0)),
//    constval i64 0)
void LowerCondGotoStmtWithBuiltinExpect(CondGotoNode &brStmt)
{
    BaseNode *condExpr = brStmt.Opnd(0);
    // Poke ne for dread shortCircuit
    // Example:
    //  dassign %shortCircuit 0 (ne u1 i64 (
    //    intrinsicop i64 C___builtin_expect (
    //      cvt i64 i32 (dread i32 %levVar_32349),
    //      cvt i64 i32 (constval i32 0)),
    //    constval i64 0))
    //  dassign %shortCircuit 0 (ne u1 u32 (dread u32 %shortCircuit, constval u1 0))
    if (condExpr->GetOpCode() == OP_ne && condExpr->Opnd(0)->GetOpCode() == OP_dread &&
        condExpr->Opnd(1)->GetOpCode() == OP_constval) {
        auto *constVal = static_cast<ConstvalNode *>(condExpr->Opnd(1))->GetConstVal();
        if (constVal->GetKind() == kConstInt && static_cast<MIRIntConst *>(constVal)->GetValue() == 0) {
            condExpr = condExpr->Opnd(0);
        }
    }
    if (condExpr->GetOpCode() == OP_dread) {
        // Example:
        //    dassign %shortCircuit 0 (ne u1 i64 (
        //      intrinsicop i64 C___builtin_expect (
        //        cvt i64 i32 (dread i32 %levVar_9488),
        //        cvt i64 i32 (constval i32 1)),
        //      constval i64 0))
        //    brfalse @shortCircuit_label_13351 (dread u32 %shortCircuit)
        StIdx stIdx = static_cast<DreadNode *>(condExpr)->GetStIdx();
        FieldID fieldId = static_cast<DreadNode *>(condExpr)->GetFieldID();
        if (fieldId != 0) {
            return;
        }
        if (brStmt.GetPrev() == nullptr || brStmt.GetPrev()->GetOpCode() != OP_dassign) {
            return;  // prev stmt may be a label, we skip it too
        }
        auto *dassign = static_cast<DassignNode *>(brStmt.GetPrev());
        if (stIdx != dassign->GetStIdx() || dassign->GetFieldID() != 0) {
            return;
        }
        condExpr = dassign->GetRHS();
    }
    if (condExpr->GetOpCode() == OP_ne) {
        // opnd1 must be int const 0
        BaseNode *opnd1 = condExpr->Opnd(1);
        if (opnd1->GetOpCode() != OP_constval) {
            return;
        }
        auto *constVal = static_cast<ConstvalNode *>(opnd1)->GetConstVal();
        if (constVal->GetKind() != kConstInt || static_cast<MIRIntConst *>(constVal)->GetValue() != 0) {
            return;
        }
        // opnd0 must be intrinsicop C___builtin_expect
        BaseNode *opnd0 = condExpr->Opnd(0);
        if (opnd0->GetOpCode() != OP_intrinsicop) {
            return;
        }
        // We trust constant fold
        auto *expectedConstExpr = opnd0->Opnd(1);
        if (expectedConstExpr->GetOpCode() == OP_cvt) {
            expectedConstExpr = expectedConstExpr->Opnd(0);
        }
        if (expectedConstExpr->GetOpCode() != OP_constval) {
            return;
        }
        auto *expectedConstNode = static_cast<ConstvalNode *>(expectedConstExpr)->GetConstVal();
        CHECK_FATAL(expectedConstNode->GetKind() == kConstInt, "must be");
        auto expectedVal = static_cast<MIRIntConst *>(expectedConstNode)->GetValue();
        if (expectedVal != 0 && expectedVal != 1) {
            return;
        }
        bool likelyTrue = (expectedVal == 1);  // The condition is likely to be true
        bool likelyBranch = (brStmt.GetOpCode() == OP_brtrue ? likelyTrue : !likelyTrue);  // High probability jump
        if (likelyBranch) {
            brStmt.SetBranchProb(kProbLikely);
        } else {
            brStmt.SetBranchProb(kProbUnlikely);
        }
        // Remove __builtin_expect
        condExpr->SetOpnd(opnd0->Opnd(0), 0);
    }
}

void MIRLower::LowerBuiltinExpect(BlockNode &block)
{
    auto *stmt = block.GetFirst();
    auto *last = block.GetLast();
    while (stmt != last) {
        if (stmt->GetOpCode() == OP_brtrue || stmt->GetOpCode() == OP_brfalse) {
            LowerCondGotoStmtWithBuiltinExpect(*static_cast<CondGotoNode *>(stmt));
        }
        stmt = stmt->GetNext();
    }
}

LabelIdx MIRLower::CreateCondGotoStmt(Opcode op, BlockNode &blk, const IfStmtNode &ifStmt)
{
    auto *brStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(op);
    brStmt->SetOpnd(ifStmt.Opnd(), 0);
    brStmt->SetSrcPos(ifStmt.GetSrcPos());
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "mirModule.CurFunction() should not be nullptr");
    LabelIdx lableIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lableIdx);
    brStmt->SetOffset(lableIdx);
    blk.AddStatement(brStmt);
    if (GetFuncProfData()) {
        GetFuncProfData()->CopyStmtFreq(brStmt->GetStmtID(), ifStmt.GetStmtID());
    }
    bool thenEmpty = (ifStmt.GetThenPart() == nullptr) || (ifStmt.GetThenPart()->GetFirst() == nullptr);
    if (thenEmpty) {
        blk.AppendStatementsFromBlock(*ifStmt.GetElsePart());
    } else {
        blk.AppendStatementsFromBlock(*ifStmt.GetThenPart());
    }
    return lableIdx;
}

void MIRLower::CreateBrFalseStmt(BlockNode &blk, const IfStmtNode &ifStmt)
{
    LabelIdx labelIdx = CreateCondGotoStmt(OP_brfalse, blk, ifStmt);
    auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    lableStmt->SetLabelIdx(labelIdx);
    blk.AddStatement(lableStmt);
    // set stmtfreqs
    if (GetFuncProfData()) {
        DEBUG_ASSERT(GetFuncProfData()->GetStmtFreq(ifStmt.GetThenPart()->GetStmtID()) >= 0, "sanity check");
        int64_t freq = GetFuncProfData()->GetStmtFreq(ifStmt.GetStmtID()) -
                       GetFuncProfData()->GetStmtFreq(ifStmt.GetThenPart()->GetStmtID());
        GetFuncProfData()->SetStmtFreq(lableStmt->GetStmtID(), freq);
    }
}

void MIRLower::CreateBrTrueStmt(BlockNode &blk, const IfStmtNode &ifStmt)
{
    LabelIdx labelIdx = CreateCondGotoStmt(OP_brtrue, blk, ifStmt);
    auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    lableStmt->SetLabelIdx(labelIdx);
    blk.AddStatement(lableStmt);
    // set stmtfreqs
    if (GetFuncProfData()) {
        DEBUG_ASSERT(GetFuncProfData()->GetStmtFreq(ifStmt.GetElsePart()->GetStmtID()) >= 0, "sanity check");
        int64_t freq = GetFuncProfData()->GetStmtFreq(ifStmt.GetStmtID()) -
                       GetFuncProfData()->GetStmtFreq(ifStmt.GetElsePart()->GetStmtID());
        GetFuncProfData()->SetStmtFreq(lableStmt->GetStmtID(), freq);
    }
}

void MIRLower::CreateBrFalseAndGotoStmt(BlockNode &blk, const IfStmtNode &ifStmt)
{
    LabelIdx labelIdx = CreateCondGotoStmt(OP_brfalse, blk, ifStmt);
    bool fallThroughFromThen = !IfStmtNoFallThrough(ifStmt);
    LabelIdx gotoLableIdx = 0;
    if (fallThroughFromThen) {
        auto *gotoStmt = mirModule.CurFuncCodeMemPool()->New<GotoNode>(OP_goto);
        DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "mirModule.CurFunction() should not be nullptr");
        gotoLableIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
        mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(gotoLableIdx);
        gotoStmt->SetOffset(gotoLableIdx);
        blk.AddStatement(gotoStmt);
        // set stmtfreqs
        if (GetFuncProfData()) {
            GetFuncProfData()->CopyStmtFreq(gotoStmt->GetStmtID(), ifStmt.GetThenPart()->GetStmtID());
        }
    }
    auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    lableStmt->SetLabelIdx(labelIdx);
    blk.AddStatement(lableStmt);
    blk.AppendStatementsFromBlock(*ifStmt.GetElsePart());
    // set stmtfreqs
    if (GetFuncProfData()) {
        GetFuncProfData()->CopyStmtFreq(lableStmt->GetStmtID(), ifStmt.GetElsePart()->GetStmtID());
    }
    if (fallThroughFromThen) {
        lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
        lableStmt->SetLabelIdx(gotoLableIdx);
        blk.AddStatement(lableStmt);
        // set endlabel stmtfreqs
        if (GetFuncProfData()) {
            GetFuncProfData()->CopyStmtFreq(lableStmt->GetStmtID(), ifStmt.GetStmtID());
        }
    }
}

BlockNode *MIRLower::LowerIfStmt(IfStmtNode &ifStmt, bool recursive)
{
    bool thenEmpty = (ifStmt.GetThenPart() == nullptr) || (ifStmt.GetThenPart()->GetFirst() == nullptr);
    bool elseEmpty = (ifStmt.GetElsePart() == nullptr) || (ifStmt.GetElsePart()->GetFirst() == nullptr);
    if (recursive) {
        if (!thenEmpty) {
            ifStmt.SetThenPart(LowerBlock(*ifStmt.GetThenPart()));
        }
        if (!elseEmpty) {
            ifStmt.SetElsePart(LowerBlock(*ifStmt.GetElsePart()));
        }
    }
    auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    if (thenEmpty && elseEmpty) {
        // generate EVAL <cond> statement
        auto *evalStmt = mirModule.CurFuncCodeMemPool()->New<UnaryStmtNode>(OP_eval);
        evalStmt->SetOpnd(ifStmt.Opnd(), 0);
        evalStmt->SetSrcPos(ifStmt.GetSrcPos());
        blk->AddStatement(evalStmt);
        if (GetFuncProfData()) {
            GetFuncProfData()->CopyStmtFreq(evalStmt->GetStmtID(), ifStmt.GetStmtID());
        }
    } else if (elseEmpty) {
        // brfalse <cond> <endlabel>
        // <thenPart>
        // label <endlabel>
        CreateBrFalseStmt(*blk, ifStmt);
    } else if (thenEmpty) {
        // brtrue <cond> <endlabel>
        // <elsePart>
        // label <endlabel>
        CreateBrTrueStmt(*blk, ifStmt);
    } else {
        // brfalse <cond> <elselabel>
        // <thenPart>
        // goto <endlabel>
        // label <elselabel>
        // <elsePart>
        // label <endlabel>
        CreateBrFalseAndGotoStmt(*blk, ifStmt);
    }
    return blk;
}

static bool ConsecutiveCaseValsAndSameTarget(const CaseVector *switchTable)
{
    size_t caseNum = switchTable->size();
    int lastVal = static_cast<int>((*switchTable)[0].first);
    LabelIdx lblIdx = (*switchTable)[0].second;
    for (size_t id = 1; id < caseNum; id++) {
        lastVal++;
        if (lastVal != (*switchTable)[id].first) {
            return false;
        }
        if (lblIdx != (*switchTable)[id].second) {
            return false;
        }
    }
    return true;
}

// if there is only 1 case branch, replace with conditional branch(es) and
// return the optimized multiple statements; otherwise, return nullptr
BlockNode *MIRLower::LowerSwitchStmt(SwitchNode *switchNode)
{
    CaseVector *switchTable = &switchNode->GetSwitchTable();
    if (switchTable->empty()) {  // goto @defaultLabel
        BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
        LabelIdx defaultLabel = switchNode->GetDefaultLabel();
        MIRBuilder *builder = mirModule.GetMIRBuilder();
        GotoNode *gotoStmt = builder->CreateStmtGoto(OP_goto, defaultLabel);
        blk->AddStatement(gotoStmt);
        return blk;
    }
    if (!ConsecutiveCaseValsAndSameTarget(switchTable)) {
        return nullptr;
    }
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    LabelIdx caseGotoLabel = switchTable->front().second;
    LabelIdx defaultLabel = switchNode->GetDefaultLabel();
    int64 minCaseVal = switchTable->front().first;
    int64 maxCaseVal = switchTable->back().first;
    BaseNode *switchOpnd = switchNode->Opnd(0);
    MIRBuilder *builder = mirModule.GetMIRBuilder();
    ConstvalNode *minCaseNode = builder->CreateIntConst(minCaseVal, switchOpnd->GetPrimType());
    ConstvalNode *maxCaseNode = builder->CreateIntConst(maxCaseVal, switchOpnd->GetPrimType());
    if (minCaseVal == maxCaseVal) {
        // brtrue (x == minCaseVal) @case_goto_label
        // goto @default_label
        CompareNode *eqNode = builder->CreateExprCompare(
            OP_eq, *GlobalTables::GetTypeTable().GetInt32(),
            *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(switchOpnd->GetPrimType())), switchOpnd, minCaseNode);
        CondGotoNode *condGoto = builder->CreateStmtCondGoto(eqNode, OP_brtrue, caseGotoLabel);
        blk->AddStatement(condGoto);
        GotoNode *gotoStmt = builder->CreateStmtGoto(OP_goto, defaultLabel);
        blk->AddStatement(gotoStmt);
    } else {
        // brtrue (x < minCaseVal) @default_label
        // brtrue (x > maxCaseVal) @default_label
        // goto @case_goto_label
        CompareNode *ltNode = builder->CreateExprCompare(
            OP_lt, *GlobalTables::GetTypeTable().GetInt32(),
            *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(switchOpnd->GetPrimType())), switchOpnd, minCaseNode);
        CondGotoNode *condGoto = builder->CreateStmtCondGoto(ltNode, OP_brtrue, defaultLabel);
        blk->AddStatement(condGoto);
        CompareNode *gtNode = builder->CreateExprCompare(
            OP_gt, *GlobalTables::GetTypeTable().GetInt32(),
            *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(switchOpnd->GetPrimType())), switchOpnd, maxCaseNode);
        condGoto = builder->CreateStmtCondGoto(gtNode, OP_brtrue, defaultLabel);
        blk->AddStatement(condGoto);
        GotoNode *gotoStmt = builder->CreateStmtGoto(OP_goto, caseGotoLabel);
        blk->AddStatement(gotoStmt);
    }
    return blk;
}

BlockNode *MIRLower::LowerBlock(BlockNode &block)
{
    auto *newBlock = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    BlockNode *tmp = nullptr;
    if (block.GetFirst() == nullptr) {
        newBlock->SetStmtID(block.GetStmtID());  // keep original block stmtid
        return newBlock;
    }
    StmtNode *nextStmt = block.GetFirst();
    DEBUG_ASSERT(nextStmt != nullptr, "nullptr check");
    do {
        StmtNode *stmt = nextStmt;
        nextStmt = stmt->GetNext();
        switch (stmt->GetOpCode()) {
            case OP_if:
                tmp = LowerIfStmt(static_cast<IfStmtNode &>(*stmt), true);
                newBlock->AppendStatementsFromBlock(*tmp);
                break;
            case OP_switch:
                tmp = LowerSwitchStmt(static_cast<SwitchNode *>(stmt));
                if (tmp != nullptr) {
                    newBlock->AppendStatementsFromBlock(*tmp);
                } else {
                    newBlock->AddStatement(stmt);
                }
                break;
            case OP_icallassigned:
            case OP_icall: {
                newBlock->AddStatement(stmt);
                break;
            }
            case OP_block:
                tmp = LowerBlock(static_cast<BlockNode &>(*stmt));
                newBlock->AppendStatementsFromBlock(*tmp);
                break;
            default:
                newBlock->AddStatement(stmt);
                break;
        }
    } while (nextStmt != nullptr);
    newBlock->SetStmtID(block.GetStmtID());  // keep original block stmtid
    return newBlock;
}

// for lowering OP_cand and OP_cior embedded in the expression x which belongs
// to curstmt
BaseNode *MIRLower::LowerEmbeddedCandCior(BaseNode *x, StmtNode *curstmt, BlockNode *blk)
{
    DEBUG_ASSERT(x != nullptr, "nullptr check");
    {
        for (size_t i = 0; i < x->GetNumOpnds(); i++) {
            x->SetOpnd(LowerEmbeddedCandCior(x->Opnd(i), curstmt, blk), i);
        }
        return x;
    }
}

// for lowering all appearances of OP_cand and OP_cior associated with condional
// branches in the block
void MIRLower::LowerCandCior(BlockNode &block)
{
    if (block.GetFirst() == nullptr) {
        return;
    }
    StmtNode *nextStmt = block.GetFirst();
    do {
        StmtNode *stmt = nextStmt;
        nextStmt = stmt->GetNext();
        {  // call LowerEmbeddedCandCior() for all the expression operands
            for (size_t i = 0; i < stmt->GetNumOpnds(); i++) {
                stmt->SetOpnd(LowerEmbeddedCandCior(stmt->Opnd(i), stmt, &block), i);
            }
        }
    } while (nextStmt != nullptr);
}

void MIRLower::LowerFunc(MIRFunction &func)
{
    mirModule.SetCurFunction(&func);
    BlockNode *origBody = func.GetBody();
    DEBUG_ASSERT(origBody != nullptr, "nullptr check");
    BlockNode *newBody = LowerBlock(*origBody);
    DEBUG_ASSERT(newBody != nullptr, "nullptr check");
    LowerBuiltinExpect(*newBody);
    if (!InLFO()) {
        LowerCandCior(*newBody);
    }
    func.SetBody(newBody);
}

MIRFuncType *MIRLower::FuncTypeFromFuncPtrExpr(BaseNode *x)
{
    DEBUG_ASSERT(x != nullptr, "nullptr check");
    MIRFuncType *res = nullptr;
    MIRFunction *func = mirModule.CurFunction();
    switch (x->GetOpCode()) {
        case OP_regread: {
            RegreadNode *regread = static_cast<RegreadNode *>(x);
            MIRPreg *preg = func->GetPregTab()->PregFromPregIdx(regread->GetRegIdx());
            // see if it is promoted from a symbol
            if (preg->GetOp() == OP_dread) {
                const MIRSymbol *symbol = preg->rematInfo.sym;
                MIRType *mirType = symbol->GetType();

                if (mirType->GetKind() == kTypePointer) {
                    res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
                }
                if (res != nullptr) {
                    break;
                }
            }
            // check if a formal promoted to preg
            for (FormalDef &formalDef : func->GetFormalDefVec()) {
                if (!formalDef.formalSym->IsPreg()) {
                    continue;
                }
                if (formalDef.formalSym->GetPreg() == preg) {
                    MIRType *mirType = formalDef.formalSym->GetType();
                    if (mirType->GetKind() == kTypePointer) {
                        res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
                    }
                    break;
                }
            }
            break;
        }
        case OP_dread: {
            DreadNode *dread = static_cast<DreadNode *>(x);
            MIRSymbol *symbol = func->GetLocalOrGlobalSymbol(dread->GetStIdx());
            MIRType *mirType = symbol->GetType();
            if (mirType->GetKind() == kTypePointer) {
                res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
            }
            break;
        }
        case OP_iread: {
            IreadNode *iread = static_cast<IreadNode *>(x);
            MIRPtrType *ptrType = static_cast<MIRPtrType *>(iread->GetType());
            MIRType *mirType = ptrType->GetPointedType();
            if (mirType->GetKind() == kTypeFunction) {
                res = static_cast<MIRFuncType *>(mirType);
            } else if (mirType->GetKind() == kTypePointer) {
                res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
            }
            break;
        }
        case OP_retype: {
            MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<RetypeNode *>(x)->GetTyIdx());
            if (mirType->GetKind() == kTypePointer) {
                res = static_cast<MIRPtrType *>(mirType)->GetPointedFuncType();
            }
            if (res == nullptr) {
                res = FuncTypeFromFuncPtrExpr(x->Opnd(kNodeFirstOpnd));
            }
            break;
        }
        default:
            CHECK_FATAL(false, "LMBCLowerer::FuncTypeFromFuncPtrExpr: NYI");
    }
    return res;
}

const std::set<std::string> MIRLower::kSetArrayHotFunc = {};

bool MIRLower::ShouldOptArrayMrt(const MIRFunction &func)
{
    return (MIRLower::kSetArrayHotFunc.find(func.GetName()) != MIRLower::kSetArrayHotFunc.end());
}
}  // namespace maple
