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

#include "lmir_builder.h"
#include "mir_builder.h"

namespace maple {
namespace litecg {

// not exposed any longer
inline Type *GetPrimitiveType(PrimType type)
{
    return GlobalTables::GetTypeTable().GetPrimType(type);
}

Module *CreateModuleWithName(const std::string &name)
{
    return new Module(name);
}

void ReleaseModule(Module *module)
{
    if (module != nullptr) {
        delete module;
        module = nullptr;
    }
    // clean current globals
    GlobalTables::Reset();
    memPoolCtrler.FreeFixedSizeMemBlockMemory();
}

bool Expr::IsDread() const
{
    return node->GetOpCode() == OP_dread;
}

bool Expr::IsRegread() const
{
    return node->GetOpCode() == OP_regread;
}

bool Expr::IsConstValue() const
{
    return node->GetOpCode() == OP_constval;
}

LMIRBuilder::LMIRBuilder(Module &module_) : mirBuilder(*module_.GetMIRBuilder()), module(module_)
{
    i8Type = GetPrimitiveType(PTY_i8);
    i16Type = GetPrimitiveType(PTY_i16);
    i32Type = GetPrimitiveType(PTY_i32);
    i64Type = GetPrimitiveType(PTY_i64);
    i128Type = GetPrimitiveType(PTY_i128);
    u1Type = GetPrimitiveType(PTY_u1);
    u8Type = GetPrimitiveType(PTY_u8);
    u16Type = GetPrimitiveType(PTY_u16);
    u32Type = GetPrimitiveType(PTY_u32);
    u64Type = GetPrimitiveType(PTY_u64);
    u128Type = GetPrimitiveType(PTY_u128);
    voidType = GetPrimitiveType(PTY_void);
    f32Type = GetPrimitiveType(PTY_f32);
    f64Type = GetPrimitiveType(PTY_f64);

    // builtin types: commonly used derived types
    strType = CreatePtrType(u8Type);  // u8PtrType
    i64PtrType = CreatePtrType(i64Type);
    i64RefType = CreateRefType(i64Type);
    i64RefRefType = CreateRefType(i64RefType);
}

void LMIRBuilder::DumpIRToFile(const std::string fileName)
{
    module.DumpToFile(fileName);
}

LiteCGTypeKind LMIRBuilder::LiteCGGetTypeKind(Type *type) const
{
    switch (type->GetKind()) {
        case MIRTypeKind::kTypeInvalid:
            return kLiteCGTypeInvalid;
        case MIRTypeKind::kTypeUnknown:
            return kLiteCGTypeUnknown;
        case MIRTypeKind::kTypeScalar:
            return kLiteCGTypeScalar;
        case MIRTypeKind::kTypeBitField:
            return kLiteCGTypeBitField;
        case MIRTypeKind::kTypeArray:
            return kLiteCGTypeArray;
        case MIRTypeKind::kTypeFArray:
            return kLiteCGTypeFArray;
        case MIRTypeKind::kTypeJArray:
            return kLiteCGTypeJArray;
        case MIRTypeKind::kTypeStruct:
            return kLiteCGTypeStruct;
        case MIRTypeKind::kTypeUnion:
            return kLiteCGTypeUnion;
        case MIRTypeKind::kTypeClass:
            return kLiteCGTypeClass;
        case MIRTypeKind::kTypeInterface:
            return kLiteCGTypeInterface;
        case MIRTypeKind::kTypeStructIncomplete:
            return kLiteCGTypeStructIncomplete;
        case MIRTypeKind::kTypeClassIncomplete:
            return kLiteCGTypeClassIncomplete;
        case MIRTypeKind::kTypeConstString:
            return kLiteCGTypeConstString;
        case MIRTypeKind::kTypeInterfaceIncomplete:
            return kLiteCGTypeInterfaceIncomplete;
        case MIRTypeKind::kTypePointer:
            return kLiteCGTypePointer;
        case MIRTypeKind::kTypeFunction:
            return kLiteCGTypeFunction;
        case MIRTypeKind::kTypeVoid:
            return kLiteCGTypeVoid;
        case MIRTypeKind::kTypeByName:
            return kLiteCGTypeByName;
        case MIRTypeKind::kTypeParam:
            return kLiteCGTypeParam;
        case MIRTypeKind::kTypeInstantVector:
            return kLiteCGTypeInstantVector;
        case MIRTypeKind::kTypeGenericInstant:
            return kLiteCGTypeGenericInstant;
        default:
            return kLiteCGTypeUnknown;
    }
}

void LMIRBuilder::SetCallStmtDeoptBundleInfo(Stmt &callNode,
                                             const std::unordered_map<int, LiteCGValue> &deoptBundleInfo)
{
    MapleUnorderedMap<int, MapleValue> deoptInfos(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (const auto itr : deoptBundleInfo) {
        auto value = itr.second;
        if (value.kind == kPregKind) {
            deoptInfos.insert(std::pair<int32_t, MapleValue>(itr.first, MapleValue(std::get<PregIdx>(value.data))));
        } else if (itr.second.kind == kSymbolKind) {
            CHECK_FATAL(false, "symbol is not supported currently");
            deoptInfos.insert(std::pair<int32_t, MapleValue>(itr.first, MapleValue(std::get<MIRSymbol*>(value.data))));
        } else {
            deoptInfos.insert(std::pair<int32_t, MapleValue>(itr.first, MapleValue(std::get<MIRConst*>(value.data))));
        }
    }
    if (callNode.GetOpCode() == OP_call) {
        static_cast<CallNode &>(callNode).SetDeoptBundleInfo(deoptInfos);
    } else {
        static_cast<IcallNode &>(callNode).SetDeoptBundleInfo(deoptInfos);
    }
}

Type *LMIRBuilder::CreatePtrType(Type *mirType)
{
    auto type = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType, PTY_ptr);
    return type;
}

Type *LMIRBuilder::CreateRefType(Type *mirType)
{
    auto type = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirType, PTY_ref);
    return type;
}

bool LMIRBuilder::IsHeapPointerType(Type *mirType) const
{
    return mirType->GetPrimType() == PTY_ref;
}

ArrayType *LMIRBuilder::CreateArrayType(Type *elemType, std::vector<uint32_t> &dimSize)
{
    auto type = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, dimSize.size(),
        reinterpret_cast<uint64*>(dimSize.data()));
    return static_cast<ArrayType *>(type);
}

Type *LMIRBuilder::CreateStructTypeInternal(const String &name,
                                            std::vector<std::pair<std::string_view, Type *>> &fields_)
{
    FieldVector parentFields;  // parentFields not used.
    // not sure about the cost
    FieldVector fields;
    for (auto field : fields_) {
        auto strIdx = mirBuilder.GetOrCreateStringIndex(field.first.data());
        fields.push_back(FieldPair(strIdx, TyIdxFieldAttrPair(field.second->GetTypeIndex(), FieldAttrs())));
    }
    auto type = GlobalTables::GetTypeTable().GetOrCreateStructType(name, fields, parentFields, module);
    return type;
}

Type *LMIRBuilder::GetStructType(const String &name)
{
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(name);
    TyIdx typeIdx = GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(strIdx);
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx);
    return type;
}

StructConst &LMIRBuilder::CreateStructConstInternal(StructType *type)
{
    return *module.GetMemPool()->New<StructConst>(module, *type);
}

ArrayConst &LMIRBuilder::CreateArrayConstInternal(ArrayType *type)
{
    return *module.GetMemPool()->New<ArrayConst>(module, *type);
}

FieldOffset LMIRBuilder::GetFieldOffset(StructType *structType, FieldId fieldId)
{
    // we should avoid access CG internals here
    // return Globals::GetInstance()->GetBECommon()->GetFieldOffset(*structType, fieldId);
    return std::pair<int32_t, int32_t>(0, 0);
}

Type *LMIRBuilder::CreateFuncType(std::vector<Type *> params_, Type *retType, bool isVarg)
{
    std::vector<TyIdx> params;
    std::vector<TypeAttrs> attrs;  // not used so far

    for (const auto param : params_) {
        params.push_back(param->GetTypeIndex());
        attrs.push_back(TypeAttrs());
    }

    auto type = GlobalTables::GetTypeTable().GetOrCreateFunctionType(retType->GetTypeIndex(), params, attrs, isVarg);
    return type;
}

Type *LMIRBuilder::LiteCGGetPointedType(Type *type)
{
    if (type == nullptr || !type->IsMIRPtrType()) {
        return nullptr;
    }
    return static_cast<MIRPtrType *>(type)->GetPointedFuncType();
}

std::vector<Type *> LMIRBuilder::LiteCGGetFuncParamTypes(Type *type)
{
    std::vector<TyIdx> &paramTypeList = static_cast<MIRFuncType *>(type)->GetParamTypeList();
    std::vector<Type *> paramTypes;
    for (const auto paramType : paramTypeList) {
        paramTypes.push_back(GlobalTables::GetTypeTable().GetTypeFromTyIdx(paramType));
    }
    return paramTypes;
}

Type *LMIRBuilder::LiteCGGetFuncReturnType(Type *type)
{
    if (type == nullptr || !type->IsMIRFuncType()) {
        return nullptr;
    }
    TyIdx retTypeIndex = static_cast<MIRFuncType *>(type)->GetRetTyIdx();
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTypeIndex);
}

// not sure it's FUNCATTR_local or FUNCATTR_static
static const FuncAttrKind FuncAttrMapTable[] = {
    // FUNC_global,   FUNC_weak,     FUNC_internal
    FUNCATTR_extern, FUNCATTR_weak, FUNCATTR_local};

static const FuncAttrKind FuncConvAttrMapTable[] = {
    // CCall,         Web_Kit_JS_Call,     GHC_Call
    FUNCATTR_ccall, FUNCATTR_webkitjscall, FUNCATTR_ghcall};

static const StmtAttrKind StmtConvAttrMapTable[] = {STMTATTR_ccall, STMTATTR_webkitjscall, STMTATTR_ghcall};

Function &LMIRBuilder::CreateFunctionInternal(const String &name, Type *retType, Params &params_, bool isVargs,
                                              bool needBody, FuncAttr attr, ConvAttr convAttr)
{
    ArgVector params(module.GetMPAllocator().Adapter());
    for (auto param : params_) {
        params.push_back(param);
    }
    auto &function = *mirBuilder.CreateFunction(name, *retType, params, isVargs, needBody);
    // check for attr
    function.SetAttr(FuncAttrMapTable[attr]);
    function.SetAttr(FuncConvAttrMapTable[convAttr]);
    // It defines a function, add to module
    if (needBody) {
        module.AddFunction(&function);
    }
    return function;
}

Function *LMIRBuilder::GetFunc(const String &name)
{
    return mirBuilder.GetFunctionFromName(name);
}

void LMIRBuilder::SetCurFunc(Function &function)
{
    module.SetCurFunction(&function);
}

Function &LMIRBuilder::GetCurFunction() const
{
    return *module.CurFunction();
}

void LMIRBuilder::RenameFormal2Preg(Function &func)
{
    if (!func.GetPregTab()) {
        // func no body, skip rename
        return;
    }
    auto &formalVec = func.GetFormalDefVec();
    for (uint32 i = 0; i < formalVec.size(); i++) {
        MIRSymbol *oldSym = formalVec[i].formalSym;
        if (!oldSym || !IsPrimitiveScalar(oldSym->GetType()->GetPrimType())) {
            continue;
        }
        PregIdx regid = CreatePreg(oldSym->GetType());
        MIRSymbol *newSym = mirBuilder.CreatePregFormalSymbol(oldSym->GetTyIdx(), regid, func);
        formalVec[i].formalSym = newSym;
    }
}

void LMIRBuilder::SetFuncFrameResverdSlot(int slot)
{
    CHECK_NULL_FATAL(module.CurFunction());
    module.CurFunction()->GetFuncAttrs().SetFrameResverdSlot(slot);
}

void LMIRBuilder::SetFuncFramePointer(const String &val)
{
    DEBUG_ASSERT(module.CurFunction() != nullptr, "module.CurFunction() should not be nullptr");
    module.CurFunction()->GetFuncAttrs().SetFramePointer(val);
}

void LMIRBuilder::SetCurrentDebugComment(const std::string& comment)
{
    mirBuilder.SetCurrentDebugComment(comment);
}

void LMIRBuilder::ClearCurrentDebugComment()
{
    mirBuilder.ClearCurrentDebugComment();
}

MIRPreg *LMIRBuilder::LiteCGGetPreg(Function &func, int32_t pRegNo)
{
    return func.GetPregItem(pRegNo);
}

Expr LMIRBuilder::LiteCGGetPregFP(Function &func)
{
    return Regread(kSregFp);
}

Expr LMIRBuilder::LiteCGGetPregSP()
{
    return Regread(kSregSp);
}

// not sure it's FUNCATTR_local or FUNCATTR_static
static const AttrKind VarAttrMapTable[] = {
    // VAR_external, VAR_weak, VAR_internal, VAR_global, VAR_readonly
    ATTR_extern, ATTR_weak, ATTR_local, ATTR_extern, ATTR_readonly};

Var &LMIRBuilder::CreateGlobalVar(Type *type, const String &name, GlobalVarAttr attr)
{
    Var *var = mirBuilder.GetOrCreateSymbol(type->GetTypeIndex(), name, kStVar, kScGlobal, nullptr, kScopeGlobal,
                                            false);  // sameType?
    var->SetAttr(VarAttrMapTable[attr]);
    return *var;
}

Var &LMIRBuilder::CreateGlobalVar(Type *type, const String &name, Const &init, GlobalVarAttr attr)
{
    Var &var = CreateGlobalVar(type, name, attr);
    var.SetKonst(&init);
    return var;
}

Var *LMIRBuilder::GetGlobalVar(const String &name)
{
    return mirBuilder.GetGlobalDecl(name);
}

Var &LMIRBuilder::CreateLocalVar(Type *type, const String &name)
{
    return *mirBuilder.GetOrCreateLocalDecl(name, *type);
}

Var *LMIRBuilder::GetLocalVar(const String &name)
{
    return mirBuilder.GetLocalDecl(name);
}

Var *LMIRBuilder::GetLocalVarFromExpr(Expr inExpr)
{
    auto *node = inExpr.GetNode();
    if ((node == nullptr) || (node->GetOpCode() != OP_dread)) {
        return nullptr;
    }
    return GetCurFunction().GetSymbolTabItem(static_cast<DreadNode *>(node)->GetStIdx().Idx(), true);
}

void LMIRBuilder::SetFunctionDerived2BaseRef(PregIdx derived, PregIdx base)
{
    return GetCurFunction().SetDerived2BaseRef(derived, base);
}

PregIdx LMIRBuilder::GetPregIdxFromExpr(const Expr &expr)
{
    auto *node = expr.GetNode();
    if ((node == nullptr) || (node->GetOpCode() != OP_regread)) {
        return 0;
    }
    return static_cast<RegreadNode *>(node)->GetRegIdx();
}

Var &LMIRBuilder::GetParam(Function &function, size_t index) const
{
    return *function.GetFormal(index);
}

Expr LMIRBuilder::GenExprFromVar(Var &var)
{
    if (var.IsPreg()) {
        return Regread(var.GetPreg()->GetPregNo());
    }
    return Dread(var);
}

Const &LMIRBuilder::CreateIntConst(Type *type, int64_t val)
{
    return *GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *type);
}

Const &LMIRBuilder::CreateFloatConst(float val)
{
    return *GlobalTables::GetFpConstTable().GetOrCreateFloatConst(val);
}

Const &LMIRBuilder::CreateDoubleConst(double val)
{
    return *GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(val);
}

Const &LMIRBuilder::CreateStrConst(const String &constStr)
{
    // fix the type for string const
    return *module.GetMemPool()->New<MIRStrConst>(constStr, *strType);
}

Const *LMIRBuilder::GetConstFromExpr(const Expr &expr)
{
    auto *node = expr.GetNode();
    if ((node == nullptr) || (node->GetOpCode() != OP_constval)) {
        return nullptr;
    }
    return static_cast<ConstvalNode *>(node)->GetConstVal();
}

BB &LMIRBuilder::CreateBB(bool needLabel)
{
    // not sure block-node is a correct representation
    // create block statement in current function
    BB &bb = *module.CurFuncCodeMemPool()->New<BlockNode>();
    if (needLabel) {
        // generate implement label statement as the first statement
        DEBUG_ASSERT(module.CurFunction() != nullptr, "module.CurFunction() should not be nullptr");
        LabelIdx labelIdx = module.CurFunction()->GetLabelTab()->CreateLabel();
        CHECK_NULL_FATAL(module.CurFunction());
        (void)module.CurFunction()->GetLabelTab()->AddToStringLabelMap(labelIdx);
        auto *labelStmt = module.CurFuncCodeMemPool()->New<LabelNode>();
        labelStmt->SetLabelIdx(labelIdx);
        bb.AddStatement(labelStmt);
    }
    return bb;
}

void LMIRBuilder::AppendStmt(BB &bb, Stmt &stmt)
{
    bb.AddStatement(&stmt);
}

void LMIRBuilder::AppendStmtBeforeBranch(BB &bb, Stmt &stmt)
{
    bool inserted = false;
    auto &nodes = bb.GetStmtNodes();
    for (auto it = nodes.crbegin(); it != nodes.crend(); it++) {
        auto &node = *it;
        if (!node.IsCondBr() && (node.GetOpCode() != OP_goto)) {
            bb.InsertAfter(&node, &stmt);
            inserted = true;
            break;
        }
    }
    CHECK_FATAL(inserted, "PreBB must have a non jump stmt to insert PhiVarAssagin Stmt.");
}

bool LMIRBuilder::IsEmptyBB(BB &bb)
{
    return bb.IsEmpty() || (bb.GetFirst() == bb.GetLast() && bb.GetFirst()->GetOpCode() == OP_label);
}

void LMIRBuilder::SetStmtCallConv(Stmt &stmt, ConvAttr convAttr)
{
    stmt.SetAttr(StmtConvAttrMapTable[convAttr]);
}

void LMIRBuilder::AppendBB(BB &bb)
{
    DEBUG_ASSERT(module.CurFunction() != nullptr, "module.CurFunction() should not be nullptr");
    module.CurFunction()->GetBody()->AddStatement(&bb);
}

void LMIRBuilder::AppendToLast(BB &bb)
{
    DEBUG_ASSERT(module.CurFunction() != nullptr, "module.CurFunction() should not be nullptr");
    module.CurFunction()->GetLastPosBody()->AddStatement(&bb);
}

BB &LMIRBuilder::GetLastAppendedBB()
{
    CHECK_NULL_FATAL(module.CurFunction());
    BB *pb = dynamic_cast<BB *>(module.CurFunction()->GetLastPosBody()->GetLast());
    return *pb;
}

BB &LMIRBuilder::GetLastPosBB()
{
    CHECK_NULL_FATAL(module.CurFunction());
    return *module.CurFunction()->GetLastPosBody();
}

LabelIdx GetBBLabelIdx(BB &bb)
{
    LabelNode *labelNode = dynamic_cast<LabelNode *>(bb.GetFirst());
    DEBUG_ASSERT(labelNode != nullptr, "BB should have a label statment");

    return labelNode->GetLabelIdx();
}

Stmt &LMIRBuilder::Goto(BB &dest)
{
    return *mirBuilder.CreateStmtGoto(OP_goto, GetBBLabelIdx(dest));
}

Stmt &LMIRBuilder::CondGoto(Var &cond, BB &target, bool inverseCond)
{
    auto opcode = inverseCond ? OP_brtrue : OP_brfalse;
    return *mirBuilder.CreateStmtCondGoto(Dread(cond).GetNode(), opcode, GetBBLabelIdx(target));
}

Stmt &LMIRBuilder::CondGoto(Expr cond, BB &target, bool inverseCond)
{
    auto opcode = inverseCond ? OP_brtrue : OP_brfalse;
    return *mirBuilder.CreateStmtCondGoto(cond.GetNode(), opcode, GetBBLabelIdx(target));
}

// not ready yet
Stmt &LMIRBuilder::CreateSwitchInternal(Type *type, Expr cond, BB &defaultBB,
                                        std::vector<std::pair<int64_t, BB *>> &cases)
{
    CaseVector switchTable(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (auto caseBranch : cases) {
        switchTable.push_back({caseBranch.first, GetBBLabelIdx(*caseBranch.second)});
    }
    return *mirBuilder.CreateStmtSwitch(cond.GetNode(), GetBBLabelIdx(defaultBB), switchTable);
}

Stmt &LMIRBuilder::Call(Function &func, Args &args_, Var *result)
{
    MapleVector<BaseNode *> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (const auto &arg : args_) {
        args.emplace_back(arg.GetNode());
    }

    if (result == nullptr) {
        return *mirBuilder.CreateStmtCall(func.GetPuidx(), args);
    } else {
        return *mirBuilder.CreateStmtCallAssigned(func.GetPuidx(), args, result);
    }
}

Stmt &LMIRBuilder::Call(Function &func, Args &args_, PregIdx pregIdx)
{
    MapleVector<BaseNode *> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (const auto &arg : args_) {
        args.emplace_back(arg.GetNode());
    }
    return *mirBuilder.CreateStmtCallRegassigned(func.GetPuidx(), args, pregIdx, OP_callassigned);
}

Stmt &LMIRBuilder::ICall(Expr funcAddr, Args &args_, Var *result)
{
    MapleVector<BaseNode *> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(funcAddr.GetNode());
    for (const auto &arg : args_) {
        args.emplace_back(arg.GetNode());
    }

    if (result == nullptr) {
        return *mirBuilder.CreateStmtIcall(args);
    } else {
        return *mirBuilder.CreateStmtIcallAssigned(args, *result);
    }
}

Stmt &LMIRBuilder::ICall(Expr funcAddr, Args &args_, PregIdx pregIdx)
{
    MapleVector<BaseNode *> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(funcAddr.GetNode());
    for (const auto &arg : args_) {
        args.emplace_back(arg.GetNode());
    }
    return *mirBuilder.CreateStmtIcallAssigned(args, pregIdx);
}

Stmt &LMIRBuilder::IntrinsicCall(IntrinsicId func_, Args &args_, Var *result)
{
    MapleVector<BaseNode *> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (const auto &arg : args_) {
        args.emplace_back(arg.GetNode());
    }

    // need to fix the type for IntrinsicId
    auto func = static_cast<MIRIntrinsicID>(func_);
    if (result == nullptr) {
        return *mirBuilder.CreateStmtIntrinsicCall(func, args);
    } else {
        return *mirBuilder.CreateStmtIntrinsicCallAssigned(func, args, result);
    }
}

Stmt &LMIRBuilder::Return(Expr returnVal)
{
    return *mirBuilder.CreateStmtReturn(returnVal.GetNode());
}

Stmt &LMIRBuilder::Comment(std::string comment)
{
    return *mirBuilder.CreateStmtComment(comment);
}

Stmt &LMIRBuilder::Dassign(Expr src, Var &var, FieldId fieldId)
{
    return *mirBuilder.CreateStmtDassign(var, fieldId, src.GetNode());
}

Stmt &LMIRBuilder::Iassign(Expr src, Expr addr, Type *baseType, FieldId fieldId)
{
    return *mirBuilder.CreateStmtIassign(*baseType, fieldId, addr.GetNode(), src.GetNode());
}

Expr LMIRBuilder::Dread(Var &var)
{
    return Expr(mirBuilder.CreateExprDread(var), var.GetType());
}

Expr LMIRBuilder::IntrinsicOp(IntrinsicId id, Type *type, Args &args_)
{
    auto func = static_cast<MIRIntrinsicID>(id);
    MapleVector<BaseNode *> args(mirBuilder.GetCurrentFuncCodeMpAllocator()->Adapter());
    for (const auto &arg : args_) {
        args.emplace_back(arg.GetNode());
    }
    return Expr(mirBuilder.CreateExprIntrinsicop(func, OP_intrinsicop, *type, args), type);
}

Expr LMIRBuilder::DreadWithField(Var &var, FieldId id)
{
    auto *type = var.GetType();
    CHECK_FATAL(type->IsStructType(), "DreadWithField: must be a struct type!");
    auto *fldType = static_cast<MIRStructType *>(type)->GetFieldType(id);
    return Expr(mirBuilder.CreateExprDread(*fldType, id, var), fldType);
}

Expr LMIRBuilder::Iread(Type *type, Expr addr, Type *baseType, FieldId fieldId)
{
    return Expr(mirBuilder.CreateExprIread(*type, *baseType, fieldId, addr.GetNode()), type);
}

PregIdx LMIRBuilder::CreatePreg(Type *mtype)
{
    if ((mtype->GetPrimType() != PTY_ptr) && (mtype->GetPrimType() != PTY_ref)) {
        // primitive type
        return GetCurFunction().GetPregTab()->CreatePreg(mtype->GetPrimType());
    }
    return GetCurFunction().GetPregTab()->CreatePreg(mtype->GetPrimType(), mtype);
}

Stmt &LMIRBuilder::Regassign(Expr src, PregIdx pregIdx)
{
    return *(mirBuilder.CreateStmtRegassign(src.GetType()->GetPrimType(), pregIdx, src.GetNode()));
}

Expr LMIRBuilder::Regread(PregIdx pregIdx)
{
    MIRPreg *preg = GetCurFunction().GetPregTab()->PregFromPregIdx(pregIdx);
    if (pregIdx < 0) {
        // special register
        return Expr(mirBuilder.CreateExprRegread(PTY_i64, pregIdx), i64Type);
    }
    if (preg->GetMIRType() != nullptr) {
        return Expr(mirBuilder.CreateExprRegread(preg->GetPrimType(), pregIdx), preg->GetMIRType());
    }
    // the type of value in reg is primitive type
    Type *type = GetPrimitiveType(preg->GetPrimType());
    return Expr(mirBuilder.CreateExprRegread(preg->GetPrimType(), pregIdx), type);
}

Expr LMIRBuilder::Addrof(Var &var)
{
    return Expr(mirBuilder.CreateAddrof(var), var.GetType());
}

Expr LMIRBuilder::ConstVal(Const &constVal)
{
    return Expr(mirBuilder.CreateConstval(&constVal), &constVal.GetType());
}

Expr LMIRBuilder::Lnot(Type *type, Expr src)
{
    return Expr(mirBuilder.CreateExprUnary(OP_lnot, *type, src.GetNode()), type);
}

Expr LMIRBuilder::Bnot(Type *type, Expr src)
{
    return Expr(mirBuilder.CreateExprUnary(OP_bnot, *type, src.GetNode()), type);
}

Expr LMIRBuilder::Sqrt(Type *type, Expr src)
{
    return Expr(mirBuilder.CreateExprUnary(OP_sqrt, *type, src.GetNode()), type);
}

Expr LMIRBuilder::Abs(Type *type, Expr src)
{
    return Expr(mirBuilder.CreateExprUnary(OP_abs, *type, src.GetNode()), type);
}

inline Expr CreateBinOpInternal(MIRBuilder &mirBuilder, Opcode op, Type *type, Expr src1, Expr src2)
{
    // we don't check for type mismatch and insert type-conversion here
    return Expr(mirBuilder.CreateExprBinary(op, *type, src1.GetNode(), src2.GetNode()), type);
}

Expr LMIRBuilder::Add(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_add, type, src1, src2);
}

Expr LMIRBuilder::Sub(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_sub, type, src1, src2);
}

Expr LMIRBuilder::Mul(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_mul, type, src1, src2);
}

Expr LMIRBuilder::UDiv(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_div, type, src1, src2);
}

Expr LMIRBuilder::SDiv(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_div, type, src1, src2);
}

Expr LMIRBuilder::URem(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_rem, type, src1, src2);
}

Expr LMIRBuilder::SRem(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_rem, type, src1, src2);
}

Expr LMIRBuilder::Shl(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_shl, type, src1, src2);
}

Expr LMIRBuilder::LShr(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_lshr, type, src1, src2);
}

Expr LMIRBuilder::AShr(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_ashr, type, src1, src2);
}

Expr LMIRBuilder::And(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_band, type, src1, src2);
}

Expr LMIRBuilder::Or(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_bior, type, src1, src2);
}

Expr LMIRBuilder::Xor(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_bxor, type, src1, src2);
}

Expr LMIRBuilder::ICmpEQ(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_eq, type, src1, src2);
}

Expr LMIRBuilder::ICmpNE(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_ne, type, src1, src2);
}

Expr LMIRBuilder::ICmpULT(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_lt, type, src1, src2);
}

Expr LMIRBuilder::ICmpULE(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_le, type, src1, src2);
}

Expr LMIRBuilder::ICmpUGT(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_gt, type, src1, src2);
}

Expr LMIRBuilder::ICmpUGE(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_ge, type, src1, src2);
}

Expr LMIRBuilder::ICmpSLT(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_lt, type, src1, src2);
}

Expr LMIRBuilder::ICmpSLE(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_le, type, src1, src2);
}

Expr LMIRBuilder::ICmpSGT(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_gt, type, src1, src2);
}

Expr LMIRBuilder::ICmpSGE(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_ge, type, src1, src2);
}

Expr LMIRBuilder::Min(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_min, type, src1, src2);
}

Expr LMIRBuilder::Max(Type *type, Expr src1, Expr src2)
{
    return CreateBinOpInternal(mirBuilder, OP_max, type, src1, src2);
}

inline Expr CreateExprCompare(MIRBuilder &mirBuilder, Opcode op, Type *type, Expr src1, Expr src2)
{
    // we don't check for type mismatch and insert type-conversion here
    return Expr(mirBuilder.CreateExprCompare(op, *type, *src1.GetType(), src1.GetNode(), src2.GetNode()), type);
}

Expr LMIRBuilder::ICmp(Type *type, Expr src1, Expr src2, IntCmpCondition cond)
{
    Opcode opCode = OP_eq;
    bool isSigned = true;
    switch (cond) {
        case kEQ:
            opCode = OP_eq;
            break;
        case kNE:
            opCode = OP_ne;
            break;
        case kULT:
            isSigned = false;
            opCode = OP_lt;
            break;
        case kULE:
            isSigned = false;
            opCode = OP_le;
            break;
        case kUGT:
            isSigned = false;
            opCode = OP_gt;
            break;
        case kUGE:
            isSigned = false;
            opCode = OP_ge;
            break;
        case kSLT:
            opCode = OP_lt;
            break;
        case kSLE:
            opCode = OP_le;
            break;
        case kSGT:
            opCode = OP_gt;
            break;
        case kSGE:
            opCode = OP_ge;
            break;
    }
    PrimType originType = src1.GetNode()->GetPrimType();
    PrimType newType = originType;
    switch (originType) {
        case PTY_i8:
        case PTY_u8:
            newType = isSigned ? PTY_i8 : PTY_u8;
            break;
        case PTY_i16:
        case PTY_u16:
            newType = isSigned ? PTY_i16 : PTY_u16;
            break;
        case PTY_i32:
        case PTY_u32:
            newType = isSigned ? PTY_i32 : PTY_u32;
            break;
        case PTY_i64:
        case PTY_u64:
            newType = isSigned ? PTY_i64 : PTY_u64;
            break;
        default:
            break;
    }
    Expr cmpExpr = CreateExprCompare(mirBuilder, opCode, type, src1, src2);
    static_cast<CompareNode *>(cmpExpr.GetNode())->SetOpndType(newType);
    return cmpExpr;
}

Expr LMIRBuilder::FCmp(Type *type, Expr src1, Expr src2, FloatCmpCondition cond)
{
    Opcode opCode = OP_eq;
    switch (cond) {
        case kOLT:
            opCode = OP_lt;
            break;
        case kOLE:
            opCode = OP_le;
            break;
        case kOGT:
            opCode = OP_gt;
            break;
        case kOGE:
            opCode = OP_ge;
            break;
        case kOEQ:
            opCode = OP_eq;
            break;
        case kONE:
            opCode = OP_ne;
            break;
    }
    return CreateExprCompare(mirBuilder, opCode, type, src1, src2);
}

Expr LMIRBuilder::Select(Type *type, Expr cond, Expr ifTrue, Expr ifFalse)
{
    return Expr(mirBuilder.CreateExprTernary(OP_select, *type, cond.GetNode(), ifTrue.GetNode(), ifFalse.GetNode()),
                type);
}

Expr LMIRBuilder::Trunc(Type *fromType, Type *toType, Expr opnd)
{
    if (fromType->GetPrimType() == toType->GetPrimType() &&
        (fromType->GetPrimType() == PTY_f64 || fromType->GetPrimType() == PTY_f32)) {
        return Expr(mirBuilder.CreateExprTypeCvt(OP_trunc, *toType, *fromType, opnd.GetNode()), toType);
    }
    return Expr(mirBuilder.CreateExprTypeCvt(OP_cvt, toType->GetPrimType(), fromType->GetPrimType(), *opnd.GetNode()),
                toType);
}

Expr LMIRBuilder::ZExt(Type *fromType, Type *toType, Expr opnd)
{
    return Expr(mirBuilder.CreateExprExtractbits(OP_zext, toType->GetPrimType(), 0,
                                                 GetPrimTypeActualBitSize(fromType->GetPrimType()), opnd.GetNode()),
                toType);
}

Expr LMIRBuilder::Cvt(Type *fromType, Type *toType, Expr opnd)
{
    if (fromType->GetPrimType() != toType->GetPrimType()) {
        return Expr(mirBuilder.CreateExprTypeCvt(OP_cvt, *toType, *fromType, opnd.GetNode()), toType);
    }
    return Expr(opnd.GetNode(), toType);
}

Expr LMIRBuilder::SExt(Type *fromType, Type *toType, Expr opnd)
{
    return Expr(mirBuilder.CreateExprExtractbits(OP_sext, toType->GetPrimType(), 0,
                                                 GetPrimTypeActualBitSize(fromType->GetPrimType()), opnd.GetNode()),
                toType);
}

Expr LMIRBuilder::Floor(Type *fromType, Type *toType, Expr opnd)
{
    return Expr(mirBuilder.CreateExprTypeCvt(OP_floor, *toType, *fromType, opnd.GetNode()), toType);
}

Expr LMIRBuilder::Ceil(Type *fromType, Type *toType, Expr opnd)
{
    return Expr(mirBuilder.CreateExprTypeCvt(OP_ceil, *toType, *fromType, opnd.GetNode()), toType);
}

Expr LMIRBuilder::BitCast(Type *fromType, Type *toType, Expr opnd)
{
    return Expr(mirBuilder.CreateExprRetype(*toType, *fromType, opnd.GetNode()), toType);
}
}  // namespace litecg
}  // namespace maple
