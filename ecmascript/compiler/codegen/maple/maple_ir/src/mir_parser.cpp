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

#include "mir_parser.h"
#include "mir_function.h"
#include "opcode_info.h"

namespace maple {

bool MIRParser::ParseSwitchCase(int64 &constVal, LabelIdx &lblIdx)
{
    // syntax <intconst0>: goto <label0>
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect intconst in switch but get ");
        return false;
    }
    constVal = lexer.GetTheIntVal();
    if (lexer.NextToken() != TK_colon) {
        Error("expect : in switch but get ");
        return false;
    }
    if (lexer.NextToken() != TK_goto) {
        Error("expect goto in switch case but get ");
        return false;
    }
    if (lexer.NextToken() != TK_label) {
        Error("expect label in switch but get ");
        return false;
    }
    lblIdx = mod.CurFunction()->GetOrCreateLableIdxFromName(lexer.GetName());
    lexer.NextToken();
    return true;
}

// used only when parsing mmpl
PUIdx MIRParser::EnterUndeclaredFunction(bool isMcount)
{
    std::string funcName;
    if (isMcount) {
        funcName = "_mcount";
    } else {
        funcName = lexer.GetName();
    }
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    funcSt->SetNameStrIdx(strIdx);
    (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*funcSt);
    funcSt->SetStorageClass(kScText);
    funcSt->SetSKind(kStFunc);
    auto *fn = mod.GetMemPool()->New<MIRFunction>(&mod, funcSt->GetStIdx());
    fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
    GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
    funcSt->SetFunction(fn);
    auto *funcType = mod.GetMemPool()->New<MIRFuncType>();
    fn->SetMIRFuncType(funcType);
    if (isMcount) {
        MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_void));
        fn->SetReturnTyIdx(retType->GetTypeIndex());
    }
    return fn->GetPuidx();
}

bool MIRParser::ParseStmtCallMcount(StmtNodePtr &stmt)
{
    // syntax: call <PU-name> (<opnd0>, ..., <opndn>)
    Opcode o = OP_call;
    PUIdx pIdx = EnterUndeclaredFunction(true);
    auto *callStmt = mod.CurFuncCodeMemPool()->New<CallNode>(mod, o);
    callStmt->SetPUIdx(pIdx);
    MapleVector<BaseNode *> opndsvec(mod.CurFuncCodeMemPoolAllocator()->Adapter());
    callStmt->SetNOpnd(opndsvec);
    callStmt->SetNumOpnds(opndsvec.size());
    stmt = callStmt;
    return true;
}

bool MIRParser::ParseCallReturns(CallReturnVector &retsvec)
{
    //             {
    //              dassign <var-name0> <field-id0>
    //              dassign <var-name1> <field-id1>
    //               . . .
    //              dassign <var-namen> <field-idn> }
    //              OR
    //             {
    //               regassign <type> <reg1>
    //               regassign <type> <reg2>
    //               regassign <type> <reg3>
    //             }
    if (lexer.NextToken() != TK_lbrace) {
        Error("expect { parsing call return values. ");
        return false;
    }
    TokenKind tk = lexer.NextToken();
    CallReturnPair retpair;
    while (tk != TK_rbrace) {
        if (lexer.GetTokenKind() != TK_dassign && lexer.GetTokenKind() != TK_regassign) {
            Error("expect dassign/regassign but get ");
            return false;
        }
        retsvec.push_back(retpair);
        tk = lexer.GetTokenKind();
    }
    return true;
}

bool MIRParser::ParseAssertInfo(std::string &funcName)
{
    if (lexer.NextToken() != TK_langle) {
        Error("expect < parsing safey assert check ");
        return false;
    }
    if (lexer.NextToken() != TK_fname) {
        Error("expect &funcname parsing parsing safey assert check ");
        return false;
    }
    funcName = lexer.GetName();
    if (lexer.NextToken() != TK_rangle) {
        Error("expect > parsing safey assert check ");
        return false;
    }
    return true;
}

bool MIRParser::ParseBinaryStmt(StmtNodePtr &stmt, Opcode op)
{
    auto *assStmt = mod.CurFuncCodeMemPool()->New<BinaryStmtNode>(op);
    lexer.NextToken();
    BaseNode *opnd0 = nullptr;
    BaseNode *opnd1 = nullptr;
    if (!ParseExprTwoOperand(opnd0, opnd1)) {
        return false;
    }
    assStmt->SetBOpnd(opnd0, 0);
    assStmt->SetBOpnd(opnd1, 1);
    stmt = assStmt;
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseCallAssertInfo(std::string &funcName, int *paramIndex, std::string &stmtFuncName)
{
    if (lexer.NextToken() != TK_langle) {
        Error("expect < parsing safey call check ");
        return false;
    }
    if (lexer.NextToken() != TK_fname) {
        Error("expect &funcname parsing parsing safey call check ");
        return false;
    }
    funcName = lexer.GetName();
    if (lexer.NextToken() != TK_coma) {
        Error("expect , parsing parsing safey call check ");
        return false;
    }
    if (lexer.NextToken() != TK_intconst) {
        Error("expect intconst parsing parsing safey call check ");
        return false;
    }
    *paramIndex = static_cast<int>(lexer.GetTheIntVal());
    if (lexer.NextToken() != TK_coma) {
        Error("expect , parsing parsing safey call check ");
        return false;
    }
    if (lexer.NextToken() != TK_fname) {
        Error("expect &stmtfuncname parsing parsing safey call check ");
        return false;
    }
    stmtFuncName = lexer.GetName();
    if (lexer.NextToken() != TK_rangle) {
        Error("expect > parsing parsing safey call check ");
        return false;
    }
    return true;
}

bool MIRParser::ParseNaryExpr(NaryStmtNode &stmtNode)
{
    if (lexer.NextToken() != TK_lparen) {
        Error("expect ( parsing NaryExpr ");
        return false;
    }
    (void)lexer.NextToken();  // skip TK_lparen
    while (lexer.GetTokenKind() != TK_rparen) {
        BaseNode *expr = nullptr;
        stmtNode.GetNopnd().push_back(expr);
        if (lexer.GetTokenKind() != TK_coma && lexer.GetTokenKind() != TK_rparen) {
            Error("expect , or ) parsing NaryStmt");
            return false;
        }
        if (lexer.GetTokenKind() == TK_coma) {
            lexer.NextToken();
        }
    }
    return true;
}

bool MIRParser::ParseLoc()
{
    if (lexer.NextToken() != TK_intconst) {
        Error("expect intconst in LOC but get ");
        return false;
    }
    lastFileNum = lexer.GetTheIntVal();
    if (lexer.NextToken() != TK_intconst) {
        Error("expect intconst in LOC but get ");
        return false;
    }
    lastLineNum = lexer.GetTheIntVal();
    if (firstLineNum == 0) {
        firstLineNum = lastLineNum;
    }
    if (lexer.NextToken() == TK_intconst) {  // optional column number
        lastColumnNum = static_cast<uint16>(lexer.GetTheIntVal());
        lexer.NextToken();
    }
    return true;
}

void MIRParser::ParseStmtBlockForSeenComment(BlockNodePtr blk, uint32 mplNum)
{
    if (Options::noComment) {
        lexer.seenComments.clear();
        return;
    }
    // collect accumulated comments into comment statement nodes
    if (!lexer.seenComments.empty()) {
        for (size_t i = 0; i < lexer.seenComments.size(); ++i) {
            auto *cmnt = mod.CurFuncCodeMemPool()->New<CommentNode>(mod);
            cmnt->SetComment(lexer.seenComments[i]);
            SetSrcPos(cmnt->GetSrcPos(), mplNum);
            blk->AddStatement(cmnt);
        }
        lexer.seenComments.clear();
    }
}

bool MIRParser::ParseStmtBlockForVar(TokenKind stmtTK)
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    MIRSymbol *st = fn->GetSymTab()->CreateSymbol(kScopeLocal);
    st->SetStorageClass(kScAuto);
    st->SetSKind(kStVar);
    SetSrcPos(st->GetSrcPosition(), lexer.GetLineNum());
    if (stmtTK == TK_tempvar) {
        st->SetIsTmp(true);
    }
    if (!fn->GetSymTab()->AddToStringSymbolMap(*st)) {
        Error("duplicate declare symbol parse function ");
        return false;
    }
    if (!ParseDeclareVarInitValue(*st)) {
        return false;
    }
    return true;
}

bool MIRParser::ParseStmtBlockForVar()
{
    return ParseStmtBlockForVar(TK_var);
}

bool MIRParser::ParseStmtBlockForTempVar()
{
    return ParseStmtBlockForVar(TK_tempvar);
}

bool MIRParser::ParseStmtBlockForReg()
{
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_preg) {
        Error("expect %%preg after reg");
        return false;
    }
    PregIdx pregIdx;
    if (!ParsePseudoReg(PTY_ref, pregIdx)) {
        return false;
    }
    MIRPreg *preg = mod.CurFunction()->GetPregTab()->PregFromPregIdx(pregIdx);
    TyIdx tyidx(0);
    if (!ParseType(tyidx)) {
        Error("ParseDeclareVar failed when parsing the type");
        return false;
    }
    DEBUG_ASSERT(tyidx > 0u, "parse declare var failed ");
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyidx);
    preg->SetMIRType(mirType);
    if (lexer.GetTokenKind() == TK_intconst) {
        int64 theIntVal = static_cast<int64>(lexer.GetTheIntVal());
        if (theIntVal != 0 && theIntVal != 1) {
            Error("parseDeclareReg failed");
            return false;
        }
        preg->SetNeedRC(theIntVal == 0 ? false : true);
    } else {
        Error("parseDeclareReg failed");
        return false;
    }
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseStmtBlockForType()
{
    paramParseLocalType = true;
    return true;
}

bool MIRParser::ParseStmtBlockForFrameSize()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after frameSize but get ");
        return false;
    }
    fn->SetFrameSize(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseStmtBlockForUpformalSize()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after upFormalSize but get ");
        return false;
    }
    fn->SetUpFormalSize(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseStmtBlockForModuleID()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after moduleid but get ");
        return false;
    }
    fn->SetModuleID(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseStmtBlockForFuncSize()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after funcSize but get ");
        return false;
    }
    fn->SetFuncSize(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseStmtBlockForFuncID()
{
    // funcid is for debugging purpose
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_intconst) {
        Error("expect integer after funcid but get ");
        return false;
    }
    fn->SetPuidxOrigin(lexer.GetTheIntVal());
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseStmtBlockForFormalWordsTypeTagged()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    uint8 *addr = ParseWordsInfo(fn->GetUpFormalSize());
    if (addr == nullptr) {
        Error("parser error for formalwordstypetagged");
        return false;
    }
    fn->SetFormalWordsTypeTagged(addr);
    return true;
}

bool MIRParser::ParseStmtBlockForLocalWordsTypeTagged()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    uint8 *addr = ParseWordsInfo(fn->GetFrameSize());
    if (addr == nullptr) {
        Error("parser error for localWordsTypeTagged");
        return false;
    }
    fn->SetLocalWordsTypeTagged(addr);
    return true;
}

bool MIRParser::ParseStmtBlockForFormalWordsRefCounted()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    uint8 *addr = ParseWordsInfo(fn->GetUpFormalSize());
    if (addr == nullptr) {
        Error("parser error for formalwordsrefcounted");
        return false;
    }
    fn->SetFormalWordsRefCounted(addr);
    return true;
}

bool MIRParser::ParseStmtBlockForLocalWordsRefCounted()
{
    MIRFunction *fn = paramCurrFuncForParseStmtBlock;
    uint8 *addr = ParseWordsInfo(fn->GetFrameSize());
    if (addr == nullptr) {
        Error("parser error for localwordsrefcounted");
        return false;
    }
    fn->SetLocalWordsRefCounted(addr);
    return true;
}

bool MIRParser::ParseStmtBlockForFuncInfo()
{
    lexer.NextToken();
    if (!ParseFuncInfo()) {
        return false;
    }
    return true;
}

bool MIRParser::ParseExprOneOperand(BaseNodePtr &expr)
{
    if (lexer.GetTokenKind() != TK_lparen) {
        Error("expect ( parsing operand parsing unary ");
        return false;
    }
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_rparen) {
        Error("expect ) parsing operand parsing unary ");
        return false;
    }
    return true;
}

bool MIRParser::ParseExprTwoOperand(BaseNodePtr &opnd0, BaseNodePtr &opnd1)
{
    if (lexer.GetTokenKind() != TK_lparen) {
        Error("expect ( parsing operand parsing unary ");
        return false;
    }
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_coma) {
        Error("expect , between two operands but get ");
        return false;
    }
    lexer.NextToken();
    if (lexer.GetTokenKind() != TK_rparen) {
        Error("expect ) parsing operand parsing unary ");
        return false;
    }
    return true;
}

bool MIRParser::ParseExprNaryOperand(MapleVector<BaseNode *> &opndVec)
{
    if (lexer.GetTokenKind() != TK_lparen) {
        Error("expect ( parsing operand parsing nary operands ");
        return false;
    }
    TokenKind tk = lexer.NextToken();
    while (tk != TK_rparen) {
        BaseNode *opnd = nullptr;
        opndVec.push_back(opnd);
        tk = lexer.GetTokenKind();
        if (tk == TK_coma) {
            tk = lexer.NextToken();
        }
    }
    return true;
}

bool MIRParser::ParseDeclaredSt(StIdx &stidx)
{
    TokenKind varTk = lexer.GetTokenKind();
    stidx.SetFullIdx(0);
    GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    if (varTk == TK_gname) {
        stidx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(stridx);
        if (stidx.FullIdx() == 0) {
            MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
            st->SetNameStrIdx(stridx);
            st->SetSKind(kStVar);
            (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*st);
            stidx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(stridx);
            return true;
        }
    } else if (varTk == TK_lname) {
        stidx = mod.CurFunction()->GetSymTab()->GetStIdxFromStrIdx(stridx);
        if (stidx.FullIdx() == 0) {
            Error("local symbol not declared ");
            return false;
        }
    } else {
        Error("expect global/local name but get ");
        return false;
    }
    return true;
}

void MIRParser::CreateFuncMIRSymbol(PUIdx &puidx, GStrIdx strIdx)
{
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    funcSt->SetNameStrIdx(strIdx);
    (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*funcSt);
    funcSt->SetStorageClass(kScText);
    funcSt->SetSKind(kStFunc);
    funcSt->SetNeedForwDecl();
    auto *fn = mod.GetMemPool()->New<MIRFunction>(&mod, funcSt->GetStIdx());
    puidx = static_cast<PUIdx>(GlobalTables::GetFunctionTable().GetFuncTable().size());
    fn->SetPuidx(puidx);
    GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
    funcSt->SetFunction(fn);
    if (options & kParseInlineFuncBody) {
        funcSt->SetIsTmpUnused(true);
    }
}

bool MIRParser::ParseDeclaredFunc(PUIdx &puidx)
{
    GStrIdx stridx = GlobalTables::GetStrTable().GetStrIdxFromName(lexer.GetName());
    if (stridx == 0u) {
        stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(lexer.GetName());
    }
    StIdx stidx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(stridx);
    if (stidx.FullIdx() == 0) {
        CreateFuncMIRSymbol(puidx, stridx);
        return true;
    }
    MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStidx(stidx.Idx());
    DEBUG_ASSERT(st != nullptr, "null ptr check");
    if (st->GetSKind() != kStFunc) {
        Error("function name not declared as function");
        return false;
    }
    MIRFunction *func = st->GetFunction();
    puidx = func->GetPuidx();
    st->SetAppearsInCode(true);
    return true;
}

bool MIRParser::ParseExprIreadIaddrof(IreadNode &expr)
{
    // syntax : iread/iaddrof <prim-type> <type> <field-id> (<addr-expr>)
    if (!IsPrimitiveType(lexer.NextToken())) {
        Error("expect primitive type but get ");
        return false;
    }
    TyIdx tyidx(0);
    if (!ParsePrimType(tyidx)) {
        return false;
    }
    expr.SetPrimType(GlobalTables::GetTypeTable().GetPrimTypeFromTyIdx(tyidx));
    tyidx = TyIdx(0);
    expr.SetTyIdx(tyidx);
    if (lexer.GetTokenKind() == TK_intconst) {
        expr.SetFieldID(lexer.theIntVal);
        lexer.NextToken();
    }
    BaseNode *opnd0 = nullptr;
    if (!ParseExprOneOperand(opnd0)) {
        return false;
    }
    expr.SetOpnd(opnd0, 0);
    lexer.NextToken();
    return true;
}

bool MIRParser::ParseIntrinsicId(IntrinsicopNode &intrnOpNode)
{
    MIRIntrinsicID intrinId = GetIntrinsicID(lexer.GetTokenKind());
    if (intrinId <= INTRN_UNDEFINED || intrinId >= INTRN_LAST) {
        Error("wrong intrinsic id ");
        return false;
    }
    intrnOpNode.SetIntrinsic(intrinId);
    return true;
}

bool MIRParser::ParseScalarValue(MIRConstPtr &stype, MIRType &type)
{
    PrimType ptp = type.GetPrimType();
    if (IsPrimitiveInteger(ptp) || IsPrimitiveDynType(ptp) || ptp == PTY_gen) {
        if (lexer.GetTokenKind() != TK_intconst) {
            Error("constant value incompatible with integer type at ");
            return false;
        }
        stype = GlobalTables::GetIntConstTable().GetOrCreateIntConst(lexer.GetTheIntVal(), type);
    } else if (ptp == PTY_f32) {
        if (lexer.GetTokenKind() != TK_floatconst) {
            Error("constant value incompatible with single-precision float type at ");
            return false;
        }
        MIRFloatConst *fConst = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(lexer.GetTheFloatVal());
        stype = fConst;
    } else if (ptp == PTY_f64) {
        if (lexer.GetTokenKind() != TK_doubleconst && lexer.GetTokenKind() != TK_intconst) {
            Error("constant value incompatible with double-precision float type at ");
            return false;
        }
        MIRDoubleConst *dconst = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(lexer.GetTheDoubleVal());
        stype = dconst;
    } else {
        return false;
    }
    return true;
}

void MIRParser::SetSrcPos(SrcPosition &srcPosition, uint32 mplNum)
{
    srcPosition.SetFileNum(lastFileNum);
    srcPosition.SetLineNum(lastLineNum);
    srcPosition.SetColumn(lastColumnNum);
    srcPosition.SetMplLineNum(mplNum);
}
}  // namespace maple
