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

#include "mir_builder.h"
#include "mir_symbol_builder.h"

namespace maple {
// create a function named str
MIRFunction *MIRBuilder::GetOrCreateFunction(const std::string &str, TyIdx retTyIdx)
{
    GStrIdx strIdx = GetStringIndex(str);
    MIRSymbol *funcSt = nullptr;
    if (strIdx != 0u) {
        funcSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
        if (funcSt == nullptr) {
            funcSt = CreateSymbol(TyIdx(0), strIdx, kStFunc, kScText, nullptr, kScopeGlobal);
        } else {
            DEBUG_ASSERT(funcSt->GetSKind() == kStFunc, "runtime check error");
            return funcSt->GetFunction();
        }
    } else {
        strIdx = GetOrCreateStringIndex(str);
        funcSt = CreateSymbol(TyIdx(0), strIdx, kStFunc, kScText, nullptr, kScopeGlobal);
    }
    auto *fn = mirModule->GetMemPool()->New<MIRFunction>(mirModule, funcSt->GetStIdx());
    fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
    MIRFuncType funcType;
    funcType.SetRetTyIdx(retTyIdx);
    auto funcTyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&funcType);
    auto *funcTypeInTypeTable = static_cast<MIRFuncType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcTyIdx));
    fn->SetMIRFuncType(funcTypeInTypeTable);
    fn->SetReturnTyIdx(retTyIdx);
    GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
    funcSt->SetFunction(fn);
    funcSt->SetTyIdx(funcTyIdx);
    return fn;
}

MIRFunction *MIRBuilder::GetFunctionFromSymbol(const MIRSymbol &funcSymbol)
{
    DEBUG_ASSERT(funcSymbol.GetSKind() == kStFunc, "Symbol %s is not a function symbol", funcSymbol.GetName().c_str());
    return funcSymbol.GetFunction();
}

MIRFunction *MIRBuilder::GetFunctionFromName(const std::string &str)
{
    auto *funcSymbol =
        GlobalTables::GetGsymTable().GetSymbolFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(str));
    return funcSymbol != nullptr ? GetFunctionFromSymbol(*funcSymbol) : nullptr;
}

MIRFunction *MIRBuilder::CreateFunction(const std::string &name, const MIRType &returnType, const ArgVector &arguments,
                                        bool isVarg, bool createBody) const
{
    MIRSymbol *funcSymbol = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    GStrIdx strIdx = GetOrCreateStringIndex(name);
    funcSymbol->SetNameStrIdx(strIdx);
    if (!GlobalTables::GetGsymTable().AddToStringSymbolMap(*funcSymbol)) {
        return nullptr;
    }
    funcSymbol->SetStorageClass(kScText);
    funcSymbol->SetSKind(kStFunc);
    auto *fn = mirModule->GetMemPool()->New<MIRFunction>(mirModule, funcSymbol->GetStIdx());
    fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
    GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
    std::vector<TyIdx> funcVecType;
    std::vector<TypeAttrs> funcVecAttrs;
    for (size_t i = 0; i < arguments.size(); ++i) {
        MIRType *ty = arguments[i].second;
        FormalDef formalDef(GetOrCreateStringIndex(arguments[i].first.c_str()), nullptr, ty->GetTypeIndex(),
                            TypeAttrs());
        fn->GetFormalDefVec().push_back(formalDef);
        funcVecType.push_back(ty->GetTypeIndex());
        funcVecAttrs.push_back(TypeAttrs());
        if (fn->GetSymTab() != nullptr && formalDef.formalSym != nullptr) {
            (void)fn->GetSymTab()->AddToStringSymbolMap(*formalDef.formalSym);
        }
    }
    funcSymbol->SetTyIdx(GlobalTables::GetTypeTable()
                             .GetOrCreateFunctionType(returnType.GetTypeIndex(), funcVecType, funcVecAttrs, isVarg)
                             ->GetTypeIndex());
    auto *funcType = static_cast<MIRFuncType *>(funcSymbol->GetType());
    fn->SetMIRFuncType(funcType);
    funcSymbol->SetFunction(fn);
    if (createBody) {
        fn->NewBody();
    }
    return fn;
}

MIRSymbol *MIRBuilder::GetOrCreateLocalDecl(const std::string &str, TyIdx tyIdx, MIRSymbolTable &symbolTable,
                                            bool &created) const
{
    GStrIdx strIdx = GetStringIndex(str);
    if (strIdx != 0u) {
        StIdx stIdx = symbolTable.GetStIdxFromStrIdx(strIdx);
        if (stIdx.Idx() != 0) {
            created = false;
            return symbolTable.GetSymbolFromStIdx(stIdx.Idx());
        }
    }
    created = true;
    strIdx = GetOrCreateStringIndex(str);
    MIRSymbol *st = symbolTable.CreateSymbol(kScopeLocal);
    DEBUG_ASSERT(st != nullptr, "null ptr check");
    st->SetNameStrIdx(strIdx);
    st->SetTyIdx(tyIdx);
    (void)symbolTable.AddToStringSymbolMap(*st);
    return st;
}

MIRSymbol *MIRBuilder::GetOrCreateDeclInFunc(const std::string &str, const MIRType &type, MIRFunction &func)
{
    MIRSymbolTable *symbolTable = func.GetSymTab();
    DEBUG_ASSERT(symbolTable != nullptr, "symbol_table is null");
    bool isCreated = false;
    MIRSymbol *st = GetOrCreateLocalDecl(str, type.GetTypeIndex(), *symbolTable, isCreated);
    DEBUG_ASSERT(st != nullptr, "st is null");
    if (isCreated) {
        st->SetStorageClass(kScAuto);
        st->SetSKind(kStVar);
    }
    return st;
}

MIRSymbol *MIRBuilder::GetOrCreateLocalDecl(const std::string &str, const MIRType &type)
{
    MIRFunction *currentFunc = GetCurrentFunction();
    CHECK_FATAL(currentFunc != nullptr, "null ptr check");
    return GetOrCreateDeclInFunc(str, type, *currentFunc);
}

MIRSymbol *MIRBuilder::GetLocalDecl(const std::string &str)
{
    MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
    return MIRSymbolBuilder::Instance().GetLocalDecl(*currentFunctionInner->GetSymTab(), GetStringIndex(str));
}

// when func is null, create global symbol, otherwise create local symbol
MIRSymbol *MIRBuilder::CreateSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                                    MIRFunction *func, uint8 scpID) const
{
    return CreateSymbol(tyIdx, GetOrCreateStringIndex(name), mClass, sClass, func, scpID);
}

// when func is null, create global symbol, otherwise create local symbol
MIRSymbol *MIRBuilder::CreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                    MIRFunction *func, uint8 scpID) const
{
    return MIRSymbolBuilder::Instance().CreateSymbol(tyIdx, strIdx, mClass, sClass, func, scpID);
}

MIRSymbol *MIRBuilder::CreatePregFormalSymbol(TyIdx tyIdx, PregIdx pRegIdx, MIRFunction &func) const
{
    return MIRSymbolBuilder::Instance().CreatePregFormalSymbol(tyIdx, pRegIdx, func);
}

ConstvalNode *MIRBuilder::CreateConstval(MIRConst *mirConst)
{
    return NewNode<ConstvalNode>(mirConst->GetType().GetPrimType(), mirConst);
}

ConstvalNode *MIRBuilder::CreateIntConst(uint64 val, PrimType pty)
{
    auto *mirConst =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *GlobalTables::GetTypeTable().GetPrimType(pty));
    return NewNode<ConstvalNode>(pty, mirConst);
}

AddrofNode *MIRBuilder::CreateExprAddrof(FieldID fieldID, const MIRSymbol &symbol, MemPool *memPool)
{
    return CreateExprAddrof(fieldID, symbol.GetStIdx(), memPool);
}

AddrofNode *MIRBuilder::CreateExprAddrof(FieldID fieldID, StIdx symbolStIdx, MemPool *memPool)
{
    if (memPool == nullptr) {
        memPool = GetCurrentFuncCodeMp();
    }
    auto node = memPool->New<AddrofNode>(OP_addrof, PTY_ptr, symbolStIdx, fieldID);
    node->SetDebugComment(currComment);
    return node;
}

AddrofNode *MIRBuilder::CreateExprDread(const MIRType &type, FieldID fieldID, const MIRSymbol &symbol)
{
    return CreateExprDread(type.GetPrimType(), fieldID, symbol);
}

AddrofNode *MIRBuilder::CreateExprDread(PrimType ptyp, FieldID fieldID, const MIRSymbol &symbol)
{
    auto *node = NewNode<AddrofNode>(OP_dread, kPtyInvalid, symbol.GetStIdx(), fieldID);
    node->SetPrimType(GetRegPrimType(ptyp));
    return node;
}

RegreadNode *MIRBuilder::CreateExprRegread(PrimType pty, PregIdx regIdx)
{
    return NewNode<RegreadNode>(pty, regIdx);
}


AddrofNode *MIRBuilder::CreateExprDread(MIRSymbol &symbol)
{
    return CreateExprDread(*symbol.GetType(), 0, symbol);
}

IreadNode *MIRBuilder::CreateExprIread(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID,
                                       BaseNode *addr)
{
    TyIdx returnTypeIdx = returnType.GetTypeIndex();
    CHECK(returnTypeIdx < GlobalTables::GetTypeTable().GetTypeTable().size(),
          "index out of range in MIRBuilder::CreateExprIread");
    PrimType type = GetRegPrimType(returnType.GetPrimType());
    return NewNode<IreadNode>(OP_iread, type, ptrType.GetTypeIndex(), fieldID, addr);
}

UnaryNode *MIRBuilder::CreateExprUnary(Opcode opcode, const MIRType &type, BaseNode *opnd)
{
    return NewNode<UnaryNode>(opcode, type.GetPrimType(), opnd);
}

TypeCvtNode *MIRBuilder::CreateExprTypeCvt(Opcode o, PrimType toPrimType, PrimType fromPrimType, BaseNode &opnd)
{
    return NewNode<TypeCvtNode>(o, toPrimType, fromPrimType, &opnd);
}

TypeCvtNode *MIRBuilder::CreateExprTypeCvt(Opcode o, const MIRType &type, const MIRType &fromType, BaseNode *opnd)
{
    return CreateExprTypeCvt(o, type.GetPrimType(), fromType.GetPrimType(), *opnd);
}

ExtractbitsNode *MIRBuilder::CreateExprExtractbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *opnd)
{
    return NewNode<ExtractbitsNode>(o, type, bOffset, bSize, opnd);
}

RetypeNode *MIRBuilder::CreateExprRetype(const MIRType &type, const MIRType &fromType, BaseNode *opnd)
{
    return CreateExprRetype(type, fromType.GetPrimType(), opnd);
}

RetypeNode *MIRBuilder::CreateExprRetype(const MIRType &type, PrimType fromType, BaseNode *opnd)
{
    return NewNode<RetypeNode>(type.GetPrimType(), fromType, type.GetTypeIndex(), opnd);
}

BinaryNode *MIRBuilder::CreateExprBinary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1)
{
    return NewNode<BinaryNode>(opcode, type.GetPrimType(), opnd0, opnd1);
}

CompareNode *MIRBuilder::CreateExprCompare(Opcode opcode, const MIRType &type, const MIRType &opndType, BaseNode *opnd0,
                                           BaseNode *opnd1)
{
    return NewNode<CompareNode>(opcode, type.GetPrimType(), opndType.GetPrimType(), opnd0, opnd1);
}

IntrinsicopNode *MIRBuilder::CreateExprIntrinsicop(MIRIntrinsicID id, Opcode op, PrimType primType, TyIdx tyIdx,
                                                   const MapleVector<BaseNode *> &ops)
{
    auto *expr = NewNode<IntrinsicopNode>(*GetCurrentFuncCodeMpAllocator(), op, primType);
    expr->SetIntrinsic(id);
    expr->SetNOpnd(ops);
    expr->SetNumOpnds(ops.size());
    return expr;
}

IntrinsicopNode *MIRBuilder::CreateExprIntrinsicop(MIRIntrinsicID idx, Opcode opCode, const MIRType &type,
                                                   const MapleVector<BaseNode *> &ops)
{
    return CreateExprIntrinsicop(idx, opCode, type.GetPrimType(), type.GetTypeIndex(), ops);
}

DassignNode *MIRBuilder::CreateStmtDassign(const MIRSymbol &symbol, FieldID fieldID, BaseNode *src)
{
    return NewNode<DassignNode>(src, symbol.GetStIdx(), fieldID);
}

RegassignNode *MIRBuilder::CreateStmtRegassign(PrimType pty, PregIdx regIdx, BaseNode *src)
{
    return NewNode<RegassignNode>(pty, regIdx, src);
}

DassignNode *MIRBuilder::CreateStmtDassign(StIdx sIdx, FieldID fieldID, BaseNode *src)
{
    return NewNode<DassignNode>(src, sIdx, fieldID);
}

IassignNode *MIRBuilder::CreateStmtIassign(const MIRType &type, FieldID fieldID, BaseNode *addr, BaseNode *src)
{
    return NewNode<IassignNode>(type.GetTypeIndex(), fieldID, addr, src);
}

CallNode *MIRBuilder::CreateStmtCall(PUIdx puIdx, const MapleVector<BaseNode *> &args, Opcode opCode)
{
    auto *stmt = NewNode<CallNode>(*GetCurrentFuncCodeMpAllocator(), opCode, puIdx, TyIdx());
    stmt->SetNOpnd(args);
    stmt->SetNumOpnds(args.size());
    return stmt;
}

IcallNode *MIRBuilder::CreateStmtTailIcall(const MapleVector<BaseNode *> &args)
{
    auto *stmt = NewNode<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_tailicall);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcall(const MapleVector<BaseNode *> &args)
{
    auto *stmt = NewNode<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icall);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcallproto(const MapleVector<BaseNode *> &args, const TyIdx &prototypeIdx)
{
    auto *stmt = NewNode<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallproto);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    stmt->SetRetTyIdx(prototypeIdx);
    return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcallAssigned(const MapleVector<BaseNode *> &args, const MIRSymbol &ret)
{
    auto *stmt = NewNode<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallassigned);
    CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
    CHECK_FATAL((ret.GetStorageClass() == kScAuto || ret.GetStorageClass() == kScFormal ||
                 ret.GetStorageClass() == kScExtern || ret.GetStorageClass() == kScGlobal),
                "unknown classtype! check it!");
    nrets.emplace_back(CallReturnPair(ret.GetStIdx(), RegFieldPair(0, 0)));
    stmt->SetNumOpnds(args.size());
    stmt->GetNopnd().resize(stmt->GetNumOpnds());
    stmt->SetReturnVec(nrets);
    for (size_t i = 0; i < stmt->GetNopndSize(); ++i) {
        stmt->SetNOpndAt(i, args.at(i));
    }
    stmt->SetRetTyIdx(ret.GetTyIdx());
    return stmt;
}

IcallNode *MIRBuilder::CreateStmtIcallAssigned(const MapleVector<BaseNode *> &args, PregIdx pregIdx)
{
    auto *stmt = NewNode<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallassigned);
    CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
    nrets.emplace_back(StIdx(), RegFieldPair(0, pregIdx));
    stmt->SetNumOpnds(args.size());
    stmt->GetNopnd().resize(stmt->GetNumOpnds());
    stmt->SetReturnVec(nrets);
    for (size_t i = 0; i < stmt->GetNopndSize(); ++i) {
        stmt->SetNOpndAt(i, args.at(i));
    }
    auto *preg = GetCurrentFunction()->GetPregTab()->PregFromPregIdx(pregIdx);
    DEBUG_ASSERT(preg, "preg should be created before used");
    if (preg->GetMIRType() == nullptr) {
        stmt->SetRetTyIdx(TyIdx(preg->GetPrimType()));
    } else {
        stmt->SetRetTyIdx(preg->GetMIRType()->GetTypeIndex());
    }
    return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments,
                                                       TyIdx tyIdx)
{
    auto *stmt = NewNode<IntrinsiccallNode>(
        *GetCurrentFuncCodeMpAllocator(), tyIdx == 0u ? OP_intrinsiccall : OP_intrinsiccallwithtype, idx);
    stmt->SetTyIdx(tyIdx);
    stmt->SetOpnds(arguments);
    return stmt;
}

CallNode *MIRBuilder::CreateStmtCallRegassigned(PUIdx puIdx, const MapleVector<BaseNode *> &args, PregIdx pRegIdx,
                                                Opcode opcode)
{
    auto *stmt = NewNode<CallNode>(*GetCurrentFuncCodeMpAllocator(), opcode, puIdx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    if (pRegIdx > 0) {
        stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, pRegIdx)));
    }
    return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode *> &args,
                                                               PregIdx retPregIdx1, PregIdx retPregIdx2)
{
    auto *stmt =
        NewNode<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(), OP_intrinsiccallassigned, idx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    if (retPregIdx1 > 0) {
        stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, retPregIdx1)));
    }
    if (retPregIdx2 > 0) {
        stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, retPregIdx2)));
    }
    return stmt;
}

NaryStmtNode *MIRBuilder::CreateStmtReturn(BaseNode *rVal)
{
    auto *stmt = NewNode<NaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), OP_return);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->PushOpnd(rVal);
    return stmt;
}

UnaryStmtNode *MIRBuilder::CreateStmtUnary(Opcode op, BaseNode *rVal)
{
    return NewNode<UnaryStmtNode>(op, kPtyInvalid, rVal);
}

IfStmtNode *MIRBuilder::CreateStmtIf(BaseNode *cond)
{
    auto *ifStmt = NewNode<IfStmtNode>();
    ifStmt->SetOpnd(cond, 0);
    BlockNode *thenBlock = NewNode<BlockNode>();
    ifStmt->SetThenPart(thenBlock);
    return ifStmt;
}

SwitchNode *MIRBuilder::CreateStmtSwitch(BaseNode *opnd, LabelIdx defaultLabel, const CaseVector &switchTable)
{
    auto *switchNode = NewNode<SwitchNode>(*GetCurrentFuncCodeMpAllocator(), defaultLabel, opnd);
    switchNode->SetSwitchTable(switchTable);
    return switchNode;
}

GotoNode *MIRBuilder::CreateStmtGoto(Opcode o, LabelIdx labIdx)
{
    return NewNode<GotoNode>(o, labIdx);
}

LabelNode *MIRBuilder::CreateStmtLabel(LabelIdx labIdx)
{
    return NewNode<LabelNode>(labIdx);
}

StmtNode *MIRBuilder::CreateStmtComment(const std::string &cmnt)
{
    return NewNode<CommentNode>(*GetCurrentFuncCodeMpAllocator(), cmnt);
}

CondGotoNode *MIRBuilder::CreateStmtCondGoto(BaseNode *cond, Opcode op, LabelIdx labIdx)
{
    return NewNode<CondGotoNode>(op, labIdx, cond);
}

MemPool *MIRBuilder::GetCurrentFuncCodeMp()
{
    if (MIRFunction *curFunction = GetCurrentFunction()) {
        return curFunction->GetCodeMemPool();
    }
    return mirModule->GetMemPool();
}

MapleAllocator *MIRBuilder::GetCurrentFuncCodeMpAllocator()
{
    if (MIRFunction *curFunction = GetCurrentFunction()) {
        return &curFunction->GetCodeMPAllocator();
    }
    return &mirModule->GetMPAllocator();
}

MIRBuilderExt::MIRBuilderExt(MIRModule *module) : MIRBuilder(module) {}
}  // namespace maple
