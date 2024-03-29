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
#include "ext_constantfold.h"
#include "me_option.h"

#define DO_LT_0_CHECK 1

namespace maple {

static constexpr uint64 RoundUpConst(uint64 offset, uint32 align)
{
    return (-align) & (offset + align - 1);
}

static inline uint64 RoundUp(uint64 offset, uint32 align)
{
    if (align == 0) {
        return offset;
    }
    return RoundUpConst(offset, align);
}

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
        if (opnd0->GetOpCode() != OP_intrinsicop ||
            static_cast<IntrinsicopNode *>(opnd0)->GetIntrinsic() != INTRN_C___builtin_expect) {
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

//     while <cond> <body>
// is lowered to:
//     brfalse <cond> <endlabel>
//   label <bodylabel>
//     <body>
//     brtrue <cond> <bodylabel>
//   label <endlabel>
BlockNode *MIRLower::LowerWhileStmt(WhileStmtNode &whileStmt)
{
    DEBUG_ASSERT(whileStmt.GetBody() != nullptr, "nullptr check");
    whileStmt.SetBody(LowerBlock(*whileStmt.GetBody()));
    auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    auto *brFalseStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
    brFalseStmt->SetOpnd(whileStmt.Opnd(0), 0);
    brFalseStmt->SetSrcPos(whileStmt.GetSrcPos());
    LabelIdx lalbeIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lalbeIdx);
    brFalseStmt->SetOffset(lalbeIdx);
    blk->AddStatement(brFalseStmt);
    blk->AppendStatementsFromBlock(*whileStmt.GetBody());
    if (MeOption::optForSize) {
        // still keep while-do format to avoid coping too much condition-related stmt
        LabelIdx whileLalbeIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
        mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(whileLalbeIdx);
        auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
        lableStmt->SetLabelIdx(whileLalbeIdx);
        blk->InsertBefore(brFalseStmt, lableStmt);
        auto *whilegotonode = mirModule.CurFuncCodeMemPool()->New<GotoNode>(OP_goto, whileLalbeIdx);
        if (GetFuncProfData() && blk->GetLast()) {
            GetFuncProfData()->CopyStmtFreq(whilegotonode->GetStmtID(), blk->GetLast()->GetStmtID());
        }
        blk->AddStatement(whilegotonode);
    } else {
        LabelIdx bodyLableIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
        mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(bodyLableIdx);
        auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
        lableStmt->SetLabelIdx(bodyLableIdx);
        blk->InsertAfter(brFalseStmt, lableStmt);
        // update frequency
        if (GetFuncProfData()) {
            GetFuncProfData()->CopyStmtFreq(lableStmt->GetStmtID(), whileStmt.GetStmtID());
            GetFuncProfData()->CopyStmtFreq(brFalseStmt->GetStmtID(), whileStmt.GetStmtID());
        }
        auto *brTrueStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
        brTrueStmt->SetOpnd(whileStmt.Opnd(0)->CloneTree(mirModule.GetCurFuncCodeMPAllocator()), 0);
        brTrueStmt->SetOffset(bodyLableIdx);
        if (GetFuncProfData() && blk->GetLast()) {
            GetFuncProfData()->CopyStmtFreq(brTrueStmt->GetStmtID(), whileStmt.GetBody()->GetStmtID());
        }
        blk->AddStatement(brTrueStmt);
    }
    auto *lableStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    lableStmt->SetLabelIdx(lalbeIdx);
    blk->AddStatement(lableStmt);
    if (GetFuncProfData()) {
        int64_t freq = GetFuncProfData()->GetStmtFreq(whileStmt.GetStmtID()) -
                       GetFuncProfData()->GetStmtFreq(blk->GetLast()->GetStmtID());
        DEBUG_ASSERT(freq >= 0, "sanity check");
        GetFuncProfData()->SetStmtFreq(lableStmt->GetStmtID(), freq);
    }
    return blk;
}

//    doloop <do-var>(<start-expr>,<cont-expr>,<incr-amt>) {<body-stmts>}
// is lowered to:
//     dassign <do-var> (<start-expr>)
//     brfalse <cond-expr> <endlabel>
//   label <bodylabel>
//     <body-stmts>
//     dassign <do-var> (<incr-amt>)
//     brtrue <cond-expr>  <bodylabel>
//   label <endlabel>
BlockNode *MIRLower::LowerDoloopStmt(DoloopNode &doloop)
{
    DEBUG_ASSERT(doloop.GetDoBody() != nullptr, "nullptr check");
    doloop.SetDoBody(LowerBlock(*doloop.GetDoBody()));
    int64_t doloopnodeFreq = 0, bodynodeFreq = 0;
    if (GetFuncProfData()) {
        doloopnodeFreq = GetFuncProfData()->GetStmtFreq(doloop.GetStmtID());
        bodynodeFreq = GetFuncProfData()->GetStmtFreq(doloop.GetDoBody()->GetStmtID());
    }
    auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    if (doloop.IsPreg()) {
        PregIdx regIdx = static_cast<PregIdx>(doloop.GetDoVarStIdx().FullIdx());
        MIRPreg *mirPreg = mirModule.CurFunction()->GetPregTab()->PregFromPregIdx(regIdx);
        PrimType primType = mirPreg->GetPrimType();
        DEBUG_ASSERT(primType != kPtyInvalid, "runtime check error");
        auto *startRegassign = mirModule.CurFuncCodeMemPool()->New<RegassignNode>();
        startRegassign->SetRegIdx(regIdx);
        startRegassign->SetPrimType(primType);
        startRegassign->SetOpnd(doloop.GetStartExpr(), 0);
        startRegassign->SetSrcPos(doloop.GetSrcPos());
        blk->AddStatement(startRegassign);
    } else {
        auto *startDassign = mirModule.CurFuncCodeMemPool()->New<DassignNode>();
        startDassign->SetStIdx(doloop.GetDoVarStIdx());
        startDassign->SetRHS(doloop.GetStartExpr());
        startDassign->SetSrcPos(doloop.GetSrcPos());
        blk->AddStatement(startDassign);
    }
    if (GetFuncProfData()) {
        GetFuncProfData()->SetStmtFreq(blk->GetLast()->GetStmtID(), doloopnodeFreq - bodynodeFreq);
    }
    auto *brFalseStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brfalse);
    brFalseStmt->SetOpnd(doloop.GetCondExpr(), 0);
    LabelIdx lIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lIdx);
    brFalseStmt->SetOffset(lIdx);
    blk->AddStatement(brFalseStmt);
    // udpate stmtFreq
    if (GetFuncProfData()) {
        GetFuncProfData()->SetStmtFreq(brFalseStmt->GetStmtID(), (doloopnodeFreq - bodynodeFreq));
    }
    LabelIdx bodyLabelIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(bodyLabelIdx);
    auto *labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labelStmt->SetLabelIdx(bodyLabelIdx);
    blk->AddStatement(labelStmt);
    // udpate stmtFreq
    if (GetFuncProfData()) {
        GetFuncProfData()->SetStmtFreq(labelStmt->GetStmtID(), bodynodeFreq);
    }
    blk->AppendStatementsFromBlock(*doloop.GetDoBody());
    if (doloop.IsPreg()) {
        PregIdx regIdx = (PregIdx)doloop.GetDoVarStIdx().FullIdx();
        MIRPreg *mirPreg = mirModule.CurFunction()->GetPregTab()->PregFromPregIdx(regIdx);
        PrimType doVarPType = mirPreg->GetPrimType();
        DEBUG_ASSERT(doVarPType != kPtyInvalid, "runtime check error");
        auto *readDoVar = mirModule.CurFuncCodeMemPool()->New<RegreadNode>();
        readDoVar->SetRegIdx(regIdx);
        readDoVar->SetPrimType(doVarPType);
        auto *add =
            mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add, doVarPType, doloop.GetIncrExpr(), readDoVar);
        auto *endRegassign = mirModule.CurFuncCodeMemPool()->New<RegassignNode>();
        endRegassign->SetRegIdx(regIdx);
        endRegassign->SetPrimType(doVarPType);
        endRegassign->SetOpnd(add, 0);
        blk->AddStatement(endRegassign);
    } else {
        const MIRSymbol *doVarSym = mirModule.CurFunction()->GetLocalOrGlobalSymbol(doloop.GetDoVarStIdx());
        PrimType doVarPType = doVarSym->GetType()->GetPrimType();
        auto *readDovar =
            mirModule.CurFuncCodeMemPool()->New<DreadNode>(OP_dread, doVarPType, doloop.GetDoVarStIdx(), 0);
        auto *add =
            mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add, doVarPType, readDovar, doloop.GetIncrExpr());
        auto *endDassign = mirModule.CurFuncCodeMemPool()->New<DassignNode>();
        endDassign->SetStIdx(doloop.GetDoVarStIdx());
        endDassign->SetRHS(add);
        blk->AddStatement(endDassign);
    }
    auto *brTrueStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
    brTrueStmt->SetOpnd(doloop.GetCondExpr()->CloneTree(mirModule.GetCurFuncCodeMPAllocator()), 0);
    brTrueStmt->SetOffset(bodyLabelIdx);
    blk->AddStatement(brTrueStmt);
    // udpate stmtFreq
    if (GetFuncProfData()) {
        GetFuncProfData()->SetStmtFreq(brTrueStmt->GetStmtID(), bodynodeFreq);
    }
    labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labelStmt->SetLabelIdx(lIdx);
    blk->AddStatement(labelStmt);
    // udpate stmtFreq
    if (GetFuncProfData()) {
        GetFuncProfData()->SetStmtFreq(labelStmt->GetStmtID(), (doloopnodeFreq - bodynodeFreq));
    }
    return blk;
}

//     dowhile <body> <cond>
// is lowered to:
//   label <bodylabel>
//     <body>
//     brtrue <cond> <bodylabel>
BlockNode *MIRLower::LowerDowhileStmt(WhileStmtNode &doWhileStmt)
{
    DEBUG_ASSERT(doWhileStmt.GetBody() != nullptr, "nullptr check");
    doWhileStmt.SetBody(LowerBlock(*doWhileStmt.GetBody()));
    auto *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    LabelIdx lIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lIdx);
    auto *labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
    labelStmt->SetLabelIdx(lIdx);
    blk->AddStatement(labelStmt);
    blk->AppendStatementsFromBlock(*doWhileStmt.GetBody());
    auto *brTrueStmt = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(OP_brtrue);
    brTrueStmt->SetOpnd(doWhileStmt.Opnd(0), 0);
    brTrueStmt->SetOffset(lIdx);
    blk->AddStatement(brTrueStmt);
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
            case OP_while:
                newBlock->AppendStatementsFromBlock(*LowerWhileStmt(static_cast<WhileStmtNode &>(*stmt)));
                break;
            case OP_dowhile:
                newBlock->AppendStatementsFromBlock(*LowerDowhileStmt(static_cast<WhileStmtNode &>(*stmt)));
                break;
            case OP_doloop:
                newBlock->AppendStatementsFromBlock(*LowerDoloopStmt(static_cast<DoloopNode &>(*stmt)));
                break;
            case OP_icallassigned:
            case OP_icall: {
                if (mirModule.IsCModule()) {
                    // convert to icallproto/icallprotoassigned
                    IcallNode *ic = static_cast<IcallNode *>(stmt);
                    ic->SetOpCode(stmt->GetOpCode() == OP_icall ? OP_icallproto : OP_icallprotoassigned);
                    MIRFuncType *funcType = FuncTypeFromFuncPtrExpr(stmt->Opnd(0));
                    CHECK_FATAL(funcType != nullptr, "MIRLower::LowerBlock: cannot find prototype for icall");
                    ic->SetRetTyIdx(funcType->GetTypeIndex());
                    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx());
                    if (retType->GetPrimType() == PTY_agg && retType->GetSize() > k16BitSize) {
                        funcType->funcAttrs.SetAttr(FUNCATTR_firstarg_return);
                    }
                }
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
    if (x->GetOpCode() == OP_cand || x->GetOpCode() == OP_cior) {
        MIRBuilder *builder = mirModule.GetMIRBuilder();
        BinaryNode *bnode = static_cast<BinaryNode *>(x);
        bnode->SetOpnd(LowerEmbeddedCandCior(bnode->Opnd(0), curstmt, blk), 0);
        PregIdx pregIdx = mirFunc->GetPregTab()->CreatePreg(x->GetPrimType());
        RegassignNode *regass = builder->CreateStmtRegassign(x->GetPrimType(), pregIdx, bnode->Opnd(0));
        blk->InsertBefore(curstmt, regass);
        LabelIdx labIdx = mirFunc->GetLabelTab()->CreateLabel();
        mirFunc->GetLabelTab()->AddToStringLabelMap(labIdx);
        BaseNode *cond = builder->CreateExprRegread(x->GetPrimType(), pregIdx);
        CondGotoNode *cgoto =
            mirFunc->GetCodeMempool()->New<CondGotoNode>(x->GetOpCode() == OP_cior ? OP_brtrue : OP_brfalse);
        cgoto->SetOpnd(cond, 0);
        cgoto->SetOffset(labIdx);
        blk->InsertBefore(curstmt, cgoto);

        bnode->SetOpnd(LowerEmbeddedCandCior(bnode->Opnd(1), curstmt, blk), 1);
        regass = builder->CreateStmtRegassign(x->GetPrimType(), pregIdx, bnode->Opnd(1));
        blk->InsertBefore(curstmt, regass);
        LabelNode *lbl = mirFunc->GetCodeMempool()->New<LabelNode>();
        lbl->SetLabelIdx(labIdx);
        blk->InsertBefore(curstmt, lbl);
        return builder->CreateExprRegread(x->GetPrimType(), pregIdx);
    } else {
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
        if (stmt->IsCondBr() && (stmt->Opnd(0)->GetOpCode() == OP_cand || stmt->Opnd(0)->GetOpCode() == OP_cior)) {
            CondGotoNode *condGoto = static_cast<CondGotoNode *>(stmt);
            BinaryNode *cond = static_cast<BinaryNode *>(condGoto->Opnd(0));
            if ((stmt->GetOpCode() == OP_brfalse && cond->GetOpCode() == OP_cand) ||
                (stmt->GetOpCode() == OP_brtrue && cond->GetOpCode() == OP_cior)) {
                // short-circuit target label is same as original condGoto stmt
                condGoto->SetOpnd(cond->GetBOpnd(0), 0);
                auto *newCondGoto = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(Opcode(stmt->GetOpCode()));
                newCondGoto->SetOpnd(cond->GetBOpnd(1), 0);
                newCondGoto->SetOffset(condGoto->GetOffset());
                block.InsertAfter(condGoto, newCondGoto);
                nextStmt = stmt;  // so it will be re-processed if another cand/cior
            } else {              // short-circuit target is next statement
                LabelIdx lIdx;
                LabelNode *labelStmt = nullptr;
                if (nextStmt->GetOpCode() == OP_label) {
                    labelStmt = static_cast<LabelNode *>(nextStmt);
                    lIdx = labelStmt->GetLabelIdx();
                } else {
                    lIdx = mirModule.CurFunction()->GetLabelTab()->CreateLabel();
                    mirModule.CurFunction()->GetLabelTab()->AddToStringLabelMap(lIdx);
                    labelStmt = mirModule.CurFuncCodeMemPool()->New<LabelNode>();
                    labelStmt->SetLabelIdx(lIdx);
                    block.InsertAfter(condGoto, labelStmt);
                }
                auto *newCondGoto = mirModule.CurFuncCodeMemPool()->New<CondGotoNode>(
                    stmt->GetOpCode() == OP_brfalse ? OP_brtrue : OP_brfalse);
                newCondGoto->SetOpnd(cond->GetBOpnd(0), 0);
                newCondGoto->SetOffset(lIdx);
                block.InsertBefore(condGoto, newCondGoto);
                condGoto->SetOpnd(cond->GetBOpnd(1), 0);
                nextStmt = newCondGoto;  // so it will be re-processed if another cand/cior
            }
        } else {  // call LowerEmbeddedCandCior() for all the expression operands
            for (size_t i = 0; i < stmt->GetNumOpnds(); i++) {
                stmt->SetOpnd(LowerEmbeddedCandCior(stmt->Opnd(i), stmt, &block), i);
            }
        }
    } while (nextStmt != nullptr);
}

void MIRLower::LowerFunc(MIRFunction &func)
{
    if (GetOptLevel() > 0) {
        ExtConstantFold ecf(func.GetModule());
        (void)ecf.ExtSimplify(func.GetBody());
        ;
    }

    mirModule.SetCurFunction(&func);
    if (IsLowerExpandArray()) {
        ExpandArrayMrt(func);
    }
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

BaseNode *MIRLower::LowerFarray(ArrayNode *array)
{
    auto *farrayType = static_cast<MIRFarrayType *>(array->GetArrayType(GlobalTables::GetTypeTable()));
    size_t eSize = GlobalTables::GetTypeTable().GetTypeFromTyIdx(farrayType->GetElemTyIdx())->GetSize();
    MIRType &arrayType = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType()));
    /* how about multi-dimension array? */
    if (array->GetIndex(0)->GetOpCode() == OP_constval) {
        const ConstvalNode *constvalNode = static_cast<const ConstvalNode *>(array->GetIndex(0));
        if (constvalNode->GetConstVal()->GetKind() == kConstInt) {
            const MIRIntConst *pIntConst = static_cast<const MIRIntConst *>(constvalNode->GetConstVal());
            CHECK_FATAL(mirModule.IsJavaModule() || !pIntConst->IsNegative(), "Array index should >= 0.");
            int64 eleOffset = pIntConst->GetExtValue() * static_cast<int64>(eSize);

            BaseNode *baseNode = array->GetBase();
            if (eleOffset == 0) {
                return baseNode;
            }

            MIRIntConst *eleConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(eleOffset, arrayType);
            BaseNode *offsetNode = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(eleConst);
            offsetNode->SetPrimType(array->GetPrimType());

            BaseNode *rAdd = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
            rAdd->SetPrimType(array->GetPrimType());
            rAdd->SetOpnd(baseNode, 0);
            rAdd->SetOpnd(offsetNode, 1);
            return rAdd;
        }
    }

    BaseNode *rMul = nullptr;

    BaseNode *baseNode = array->GetBase();

    BaseNode *rAdd = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
    rAdd->SetPrimType(array->GetPrimType());
    rAdd->SetOpnd(baseNode, 0);
    rAdd->SetOpnd(rMul, 1);
    auto *newAdd = ConstantFold(mirModule).Fold(rAdd);
    rAdd = (newAdd != nullptr ? newAdd : rAdd);
    return rAdd;
}

BaseNode *MIRLower::LowerCArray(ArrayNode *array)
{
    MIRType *aType = array->GetArrayType(GlobalTables::GetTypeTable());
    if (aType->GetKind() == kTypeJArray) {
        return array;
    }
    if (aType->GetKind() == kTypeFArray) {
        return LowerFarray(array);
    }

    MIRArrayType *arrayType = static_cast<MIRArrayType *>(aType);
    /* There are two cases where dimension > 1.
     * 1) arrayType->dim > 1.  Process the current arrayType. (nestedArray = false)
     * 2) arrayType->dim == 1, but arraytype->eTyIdx is another array. (nestedArray = true)
     * Assume at this time 1) and 2) cannot mix.
     * Along with the array dimension, there is the array indexing.
     * It is allowed to index arrays less than the dimension.
     * This is dictated by the number of indexes.
     */
    bool nestedArray = false;
    uint64 dim = arrayType->GetDim();
    MIRType *innerType = nullptr;
    MIRArrayType *innerArrayType = nullptr;
    uint64 elemSize = 0;
    if (dim == 1) {
        innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
        if (innerType->GetKind() == kTypeArray) {
            nestedArray = true;
            do {
                innerArrayType = static_cast<MIRArrayType *>(innerType);
                elemSize = RoundUp(innerArrayType->GetElemType()->GetSize(), arrayType->GetElemType()->GetAlign());
                dim++;
                innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(innerArrayType->GetElemTyIdx());
            } while (innerType->GetKind() == kTypeArray);
        }
    }

    size_t numIndex = array->NumOpnds() - 1;
    MIRArrayType *curArrayType = arrayType;
    BaseNode *resNode = array->GetIndex(0);
    if (dim > 1) {
        BaseNode *prevNode = nullptr;
        for (size_t i = 0; (i < dim) && (i < numIndex); ++i) {
            uint32 mpyDim = 1;
            if (nestedArray) {
                CHECK_FATAL(arrayType->GetSizeArrayItem(0) > 0, "Zero size array dimension");
                innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(curArrayType->GetElemTyIdx());
                curArrayType = static_cast<MIRArrayType *>(innerType);
                while (innerType->GetKind() == kTypeArray) {
                    innerArrayType = static_cast<MIRArrayType *>(innerType);
                    mpyDim *= innerArrayType->GetSizeArrayItem(0);
                    innerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(innerArrayType->GetElemTyIdx());
                }
            } else {
                CHECK_FATAL(arrayType->GetSizeArrayItem(static_cast<uint32>(i)) > 0, "Zero size array dimension");
                for (size_t j = i + 1; j < dim; ++j) {
                    mpyDim *= arrayType->GetSizeArrayItem(static_cast<uint32>(j));
                }
            }

            BaseNode *index = static_cast<ConstvalNode *>(array->GetIndex(i));
            bool isConst = false;
            uint64 indexVal = 0;
            if (index->op == OP_constval) {
                ConstvalNode *constNode = static_cast<ConstvalNode *>(index);
                indexVal = static_cast<uint64>((static_cast<MIRIntConst *>(constNode->GetConstVal()))->GetExtValue());
                isConst = true;
                MIRIntConst *newConstNode = mirModule.GetMemPool()->New<MIRIntConst>(
                    indexVal * mpyDim, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
                BaseNode *newValNode = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(newConstNode);
                newValNode->SetPrimType(array->GetPrimType());
                if (i == 0) {
                    prevNode = newValNode;
                    continue;
                } else {
                    resNode = newValNode;
                }
            }
            if (i > 0 && isConst == false) {
                resNode = array->GetIndex(i);
            }

            BaseNode *mpyNode;
            if (isConst) {
                MIRIntConst *mulConst = mirModule.GetMemPool()->New<MIRIntConst>(
                    static_cast<int64>(mpyDim) * indexVal,
                    *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
                BaseNode *mulSize = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(mulConst);
                mulSize->SetPrimType(array->GetPrimType());
                mpyNode = mulSize;
            } else if (mpyDim == 1 && prevNode) {
                mpyNode = prevNode;
                prevNode = resNode;
            } else {
                mpyNode = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_mul);
                mpyNode->SetPrimType(array->GetPrimType());
                MIRIntConst *mulConst = mirModule.GetMemPool()->New<MIRIntConst>(
                    mpyDim, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
                BaseNode *mulSize = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(mulConst);
                mulSize->SetPrimType(array->GetPrimType());
                mpyNode->SetOpnd(mulSize, 1);
                PrimType signedInt4AddressCompute = GetSignedPrimType(array->GetPrimType());
                if (!IsPrimitiveInteger(resNode->GetPrimType())) {
                    resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, signedInt4AddressCompute,
                                                                               resNode->GetPrimType(), resNode);
                } else if (GetPrimTypeSize(resNode->GetPrimType()) != GetPrimTypeSize(array->GetPrimType())) {
                    resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(
                        OP_cvt, array->GetPrimType(), GetRegPrimType(resNode->GetPrimType()), resNode);
                }
                mpyNode->SetOpnd(resNode, 0);
            }
            if (i == 0) {
                prevNode = mpyNode;
                continue;
            }
            BaseNode *newResNode = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
            newResNode->SetPrimType(array->GetPrimType());
            newResNode->SetOpnd(mpyNode, 0);
            if (NeedCvtOrRetype(prevNode->GetPrimType(), array->GetPrimType())) {
                prevNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(
                    OP_cvt, array->GetPrimType(), GetRegPrimType(prevNode->GetPrimType()), prevNode);
            }
            newResNode->SetOpnd(prevNode, 1);
            prevNode = newResNode;
        }
        resNode = prevNode;
    }

    BaseNode *rMul = nullptr;
    // esize is the size of the array element (eg. int = 4 long = 8)
    uint64 esize;
    if (nestedArray) {
        esize = elemSize;
    } else {
        esize = arrayType->GetElemType()->GetSize();
    }
    Opcode opadd = OP_add;
    MIRIntConst *econst = mirModule.GetMemPool()->New<MIRIntConst>(
        esize, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(array->GetPrimType())));
    BaseNode *eSize = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(econst);
    eSize->SetPrimType(array->GetPrimType());
    rMul = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_mul);
    PrimType signedInt4AddressCompute = GetSignedPrimType(array->GetPrimType());
    if (!IsPrimitiveInteger(resNode->GetPrimType())) {
        resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, signedInt4AddressCompute,
                                                                   resNode->GetPrimType(), resNode);
    } else if (GetPrimTypeSize(resNode->GetPrimType()) != GetPrimTypeSize(array->GetPrimType())) {
        resNode = mirModule.CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, array->GetPrimType(),
                                                                   GetRegPrimType(resNode->GetPrimType()), resNode);
    }
    rMul->SetPrimType(resNode->GetPrimType());
    rMul->SetOpnd(resNode, 0);
    rMul->SetOpnd(eSize, 1);
    BaseNode *baseNode = array->GetBase();
    BaseNode *rAdd = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(opadd);
    rAdd->SetPrimType(array->GetPrimType());
    rAdd->SetOpnd(baseNode, 0);
    rAdd->SetOpnd(rMul, 1);
    auto *newAdd = ConstantFold(mirModule).Fold(rAdd);
    rAdd = (newAdd != nullptr ? newAdd : rAdd);
    return rAdd;
}

IfStmtNode *MIRLower::ExpandArrayMrtIfBlock(IfStmtNode &node)
{
    if (node.GetThenPart() != nullptr) {
        node.SetThenPart(ExpandArrayMrtBlock(*node.GetThenPart()));
    }
    if (node.GetElsePart() != nullptr) {
        node.SetElsePart(ExpandArrayMrtBlock(*node.GetElsePart()));
    }
    return &node;
}

WhileStmtNode *MIRLower::ExpandArrayMrtWhileBlock(WhileStmtNode &node)
{
    if (node.GetBody() != nullptr) {
        node.SetBody(ExpandArrayMrtBlock(*node.GetBody()));
    }
    return &node;
}

DoloopNode *MIRLower::ExpandArrayMrtDoloopBlock(DoloopNode &node)
{
    if (node.GetDoBody() != nullptr) {
        node.SetDoBody(ExpandArrayMrtBlock(*node.GetDoBody()));
    }
    return &node;
}

ForeachelemNode *MIRLower::ExpandArrayMrtForeachelemBlock(ForeachelemNode &node)
{
    if (node.GetLoopBody() != nullptr) {
        node.SetLoopBody(ExpandArrayMrtBlock(*node.GetLoopBody()));
    }
    return &node;
}

void MIRLower::AddArrayMrtMpl(BaseNode &exp, BlockNode &newBlock)
{
    MIRModule &mod = mirModule;
    MIRBuilder *builder = mod.GetMIRBuilder();
    for (size_t i = 0; i < exp.NumOpnds(); ++i) {
        DEBUG_ASSERT(exp.Opnd(i) != nullptr, "nullptr check");
        AddArrayMrtMpl(*exp.Opnd(i), newBlock);
    }
    if (exp.GetOpCode() == OP_array) {
        auto &arrayNode = static_cast<ArrayNode &>(exp);
        if (arrayNode.GetBoundsCheck()) {
            BaseNode *arrAddr = arrayNode.Opnd(0);
            BaseNode *index = arrayNode.Opnd(1);
            DEBUG_ASSERT(index != nullptr, "null ptr check");
            MIRType *indexType = GlobalTables::GetTypeTable().GetPrimType(index->GetPrimType());
            UnaryStmtNode *nullCheck = builder->CreateStmtUnary(OP_assertnonnull, arrAddr);
            newBlock.AddStatement(nullCheck);
#if DO_LT_0_CHECK
            ConstvalNode *indexZero = builder->GetConstUInt32(0);
            CompareNode *lessZero =
                builder->CreateExprCompare(OP_lt, *GlobalTables::GetTypeTable().GetUInt1(),
                                           *GlobalTables::GetTypeTable().GetUInt32(), index, indexZero);
#endif
            MIRType *infoLenType = GlobalTables::GetTypeTable().GetInt32();
            MapleVector<BaseNode *> arguments(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
            arguments.push_back(arrAddr);
            BaseNode *arrLen =
                builder->CreateExprIntrinsicop(INTRN_JAVA_ARRAY_LENGTH, OP_intrinsicop, *infoLenType, arguments);
            BaseNode *cpmIndex = index;
            if (arrLen->GetPrimType() != index->GetPrimType()) {
                cpmIndex = builder->CreateExprTypeCvt(OP_cvt, *infoLenType, *indexType, index);
            }
            CompareNode *largeLen =
                builder->CreateExprCompare(OP_ge, *GlobalTables::GetTypeTable().GetUInt1(),
                                           *GlobalTables::GetTypeTable().GetUInt32(), cpmIndex, arrLen);
            // maybe should use cior
#if DO_LT_0_CHECK
            BinaryNode *indexCon =
                builder->CreateExprBinary(OP_lior, *GlobalTables::GetTypeTable().GetUInt1(), lessZero, largeLen);
#endif
            MapleVector<BaseNode *> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
#if DO_LT_0_CHECK
            args.push_back(indexCon);
            IntrinsiccallNode *boundaryTrinsicCall = builder->CreateStmtIntrinsicCall(INTRN_MPL_BOUNDARY_CHECK, args);
#else
            args.push_back(largeLen);
            IntrinsiccallNode *boundaryTrinsicCall = builder->CreateStmtIntrinsicCall(INTRN_MPL_BOUNDARY_CHECK, args);
#endif
            newBlock.AddStatement(boundaryTrinsicCall);
        }
    }
}

BlockNode *MIRLower::ExpandArrayMrtBlock(BlockNode &block)
{
    auto *newBlock = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    if (block.GetFirst() == nullptr) {
        return newBlock;
    }
    StmtNode *nextStmt = block.GetFirst();
    do {
        StmtNode *stmt = nextStmt;
        DEBUG_ASSERT(stmt != nullptr, "nullptr check");
        nextStmt = stmt->GetNext();
        switch (stmt->GetOpCode()) {
            case OP_if:
                newBlock->AddStatement(ExpandArrayMrtIfBlock(static_cast<IfStmtNode &>(*stmt)));
                break;
            case OP_while:
                newBlock->AddStatement(ExpandArrayMrtWhileBlock(static_cast<WhileStmtNode &>(*stmt)));
                break;
            case OP_dowhile:
                newBlock->AddStatement(ExpandArrayMrtWhileBlock(static_cast<WhileStmtNode &>(*stmt)));
                break;
            case OP_doloop:
                newBlock->AddStatement(ExpandArrayMrtDoloopBlock(static_cast<DoloopNode &>(*stmt)));
                break;
            case OP_foreachelem:
                newBlock->AddStatement(ExpandArrayMrtForeachelemBlock(static_cast<ForeachelemNode &>(*stmt)));
                break;
            case OP_block:
                newBlock->AddStatement(ExpandArrayMrtBlock(static_cast<BlockNode &>(*stmt)));
                break;
            default:
                AddArrayMrtMpl(*stmt, *newBlock);
                newBlock->AddStatement(stmt);
                break;
        }
    } while (nextStmt != nullptr);
    return newBlock;
}

void MIRLower::ExpandArrayMrt(MIRFunction &func)
{
    if (ShouldOptArrayMrt(func)) {
        BlockNode *origBody = func.GetBody();
        DEBUG_ASSERT(origBody != nullptr, "nullptr check");
        BlockNode *newBody = ExpandArrayMrtBlock(*origBody);
        func.SetBody(newBody);
    }
}

MIRFuncType *MIRLower::FuncTypeFromFuncPtrExpr(BaseNode *x)
{
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
                if (preg->fieldID != 0) {
                    MIRStructType *structty = static_cast<MIRStructType *>(mirType);
                    FieldPair thepair = structty->TraverseToField(preg->fieldID);
                    mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
                }

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
            if (dread->GetFieldID() != 0) {
                MIRStructType *structty = static_cast<MIRStructType *>(mirType);
                FieldPair thepair = structty->TraverseToField(dread->GetFieldID());
                mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(thepair.second.first);
            }
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
        case OP_addroffunc: {
            AddroffuncNode *addrofFunc = static_cast<AddroffuncNode *>(x);
            PUIdx puIdx = addrofFunc->GetPUIdx();
            MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
            res = f->GetMIRFuncType();
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
        case OP_select: {
            res = FuncTypeFromFuncPtrExpr(x->Opnd(kNodeSecondOpnd));
            if (res == nullptr) {
                res = FuncTypeFromFuncPtrExpr(x->Opnd(kNodeThirdOpnd));
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
