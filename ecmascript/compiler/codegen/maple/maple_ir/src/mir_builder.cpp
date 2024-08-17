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
// This is for compiler-generated metadata 1-level struct
void MIRBuilder::AddIntFieldConst(const MIRStructType &sType, MIRAggConst &newConst, uint32 fieldID, int64 constValue)
{
    DEBUG_ASSERT(fieldID > 0, "must not be zero");
    auto *fieldConst =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(constValue, *sType.GetElemType(fieldID - 1));
    newConst.AddItem(fieldConst, fieldID);
}

// This is for compiler-generated metadata 1-level struct
void MIRBuilder::AddAddrofFieldConst(const MIRStructType &structType, MIRAggConst &newConst, uint32 fieldID,
                                     const MIRSymbol &fieldSymbol)
{
    AddrofNode *fieldExpr = CreateExprAddrof(0, fieldSymbol, mirModule->GetMemPool());
    DEBUG_ASSERT(fieldID > 0, "must not be zero");
    auto *fieldConst = mirModule->GetMemPool()->New<MIRAddrofConst>(fieldExpr->GetStIdx(), fieldExpr->GetFieldID(),
                                                                    *structType.GetElemType(fieldID - 1));
    newConst.AddItem(fieldConst, fieldID);
}

// This is for compiler-generated metadata 1-level struct
void MIRBuilder::AddAddroffuncFieldConst(const MIRStructType &structType, MIRAggConst &newConst, uint32 fieldID,
                                         const MIRSymbol &funcSymbol)
{
    MIRConst *fieldConst = nullptr;
    MIRFunction *vMethod = funcSymbol.GetFunction();
    if (vMethod->IsAbstract()) {
        DEBUG_ASSERT(fieldID > 0, "must not be zero");
        fieldConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *structType.GetElemType(fieldID - 1));
    } else {
        AddroffuncNode *addrofFuncExpr =
            CreateExprAddroffunc(funcSymbol.GetFunction()->GetPuidx(), mirModule->GetMemPool());
        DEBUG_ASSERT(fieldID > 0, "must not be zero");
        fieldConst = mirModule->GetMemPool()->New<MIRAddroffuncConst>(addrofFuncExpr->GetPUIdx(),
                                                                      *structType.GetElemType(fieldID - 1));
    }
    newConst.AddItem(fieldConst, fieldID);
}

// fieldID is continuously being updated during traversal;
// when the field is found, its field id is returned via fieldID
bool MIRBuilder::TraverseToNamedField(MIRStructType &structType, GStrIdx nameIdx, uint32 &fieldID)
{
    TyIdx tid(0);
    return TraverseToNamedFieldWithTypeAndMatchStyle(structType, nameIdx, tid, fieldID, kMatchAnyField);
}

// traverse parent first but match self first.
void MIRBuilder::TraverseToNamedFieldWithType(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx,
                                              uint32 &fieldID, uint32 &idx)
{
    if (structType.IsIncomplete()) {
        (void)incompleteTypeRefedSet.insert(structType.GetTypeIndex());
    }
    // process parent
    if (structType.GetKind() == kTypeClass || structType.GetKind() == kTypeClassIncomplete) {
        auto &classType = static_cast<MIRClassType &>(structType);
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classType.GetParentTyIdx());
        auto *parentType = static_cast<MIRStructType *>(type);
        if (parentType != nullptr) {
            ++fieldID;
            TraverseToNamedFieldWithType(*parentType, nameIdx, typeIdx, fieldID, idx);
        }
    }
    for (uint32 fieldIdx = 0; fieldIdx < structType.GetFieldsSize(); ++fieldIdx) {
        ++fieldID;
        TyIdx fieldTyIdx = structType.GetFieldsElemt(fieldIdx).second.first;
        MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
        if (structType.GetFieldsElemt(fieldIdx).first == nameIdx) {
            if (typeIdx == 0u || fieldTyIdx == typeIdx) {
                idx = fieldID;
                continue;
            }
            // for pointer type, check their pointed type
            MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx);
            if (type->IsOfSameType(*fieldType)) {
                idx = fieldID;
            }
        }

        if (fieldType->IsStructType()) {
            auto *subStructType = static_cast<MIRStructType *>(fieldType);
            TraverseToNamedFieldWithType(*subStructType, nameIdx, typeIdx, fieldID, idx);
        }
    }
}

// fieldID is continuously being updated during traversal;
// when the field is found, its field id is returned via fieldID
//
// typeidx: TyIdx(0) means do not check types.
// matchstyle: 0: do not match but traverse to update fieldID
//             1: match top level field only
//             2: match any field
//             4: traverse parent first
//          0xc: do not match but traverse to update fieldID, traverse parent first, found in child
bool MIRBuilder::TraverseToNamedFieldWithTypeAndMatchStyle(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx,
                                                           uint32 &fieldID, unsigned int matchStyle)
{
    if (structType.IsIncomplete()) {
        (void)incompleteTypeRefedSet.insert(structType.GetTypeIndex());
    }
    if (matchStyle & kParentFirst) {
        // process parent
        if ((structType.GetKind() != kTypeClass) && (structType.GetKind() != kTypeClassIncomplete)) {
            return false;
        }

        auto &classType = static_cast<MIRClassType &>(structType);
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(classType.GetParentTyIdx());
        auto *parentType = static_cast<MIRStructType *>(type);
        if (parentType != nullptr) {
            ++fieldID;
            if (matchStyle == (kFoundInChild | kParentFirst | kUpdateFieldID)) {
                matchStyle = kParentFirst;
                uint32 idxBackup = nameIdx;
                nameIdx.reset();
                // do not match but traverse to update fieldID, traverse parent first
                TraverseToNamedFieldWithTypeAndMatchStyle(*parentType, nameIdx, typeIdx, fieldID, matchStyle);
                nameIdx.reset(idxBackup);
            } else if (TraverseToNamedFieldWithTypeAndMatchStyle(*parentType, nameIdx, typeIdx, fieldID, matchStyle)) {
                return true;
            }
        }
    }
    for (uint32 fieldIdx = 0; fieldIdx < structType.GetFieldsSize(); ++fieldIdx) {
        ++fieldID;
        TyIdx fieldTyIdx = structType.GetFieldsElemt(fieldIdx).second.first;
        MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
        DEBUG_ASSERT(fieldType != nullptr, "fieldType is null");
        if (matchStyle && structType.GetFieldsElemt(fieldIdx).first == nameIdx) {
            if (typeIdx == 0u || fieldTyIdx == typeIdx ||
                fieldType->IsOfSameType(*GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx))) {
                return true;
            }
        }
        unsigned int style = matchStyle & kMatchAnyField;
        if (fieldType->IsStructType()) {
            auto *subStructType = static_cast<MIRStructType *>(fieldType);
            if (TraverseToNamedFieldWithTypeAndMatchStyle(*subStructType, nameIdx, typeIdx, fieldID, style)) {
                return true;
            }
        }
    }
    return false;
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx,
                                                    unsigned int matchStyle)
{
    auto &structType = static_cast<MIRStructType &>(type);
    uint32 fieldID = 0;
    GStrIdx strIdx = GetStringIndex(name);
    if (TraverseToNamedFieldWithTypeAndMatchStyle(structType, strIdx, idx, fieldID, matchStyle)) {
        return fieldID;
    }
    return 0;
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx)
{
    return GetStructFieldIDFromNameAndType(type, name, idx, kMatchAnyField);
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndTypeParentFirst(MIRType &type, const std::string &name, TyIdx idx)
{
    return GetStructFieldIDFromNameAndType(type, name, idx, kParentFirst);
}

FieldID MIRBuilder::GetStructFieldIDFromNameAndTypeParentFirstFoundInChild(MIRType &type, const std::string &name,
                                                                           TyIdx idx)
{
    // do not match but traverse to update fieldID, traverse parent first, found in child
    return GetStructFieldIDFromNameAndType(type, name, idx, kFoundInChild | kParentFirst | kUpdateFieldID);
}

FieldID MIRBuilder::GetStructFieldIDFromFieldName(MIRType &type, const std::string &name)
{
    return GetStructFieldIDFromNameAndType(type, name, TyIdx(0), kMatchAnyField);
}

FieldID MIRBuilder::GetStructFieldIDFromFieldNameParentFirst(MIRType *type, const std::string &name)
{
    if (type == nullptr) {
        return 0;
    }
    return GetStructFieldIDFromNameAndType(*type, name, TyIdx(0), kParentFirst);
}

void MIRBuilder::SetStructFieldIDFromFieldName(MIRStructType &structType, const std::string &name, GStrIdx newStrIdx,
                                               const MIRType &newFieldType)
{
    uint32 fieldID = 0;
    GStrIdx strIdx = GetStringIndex(name);
    while (true) {
        if (structType.GetElemStrIdx(fieldID) == strIdx) {
            if (newStrIdx != 0u) {
                structType.SetElemStrIdx(fieldID, newStrIdx);
            }
            structType.SetElemtTyIdx(fieldID, newFieldType.GetTypeIndex());
            return;
        }
        ++fieldID;
    }
}

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

MIRFunction *MIRBuilder::GetFunctionFromStidx(StIdx stIdx)
{
    auto *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
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

MIRFunction *MIRBuilder::CreateFunction(StIdx stIdx, bool addToTable) const
{
    auto *fn = mirModule->GetMemPool()->New<MIRFunction>(mirModule, stIdx);
    fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
    if (addToTable) {
        GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
    }

    auto *funcType = mirModule->GetMemPool()->New<MIRFuncType>();
    fn->SetMIRFuncType(funcType);
    return fn;
}

MIRSymbol *MIRBuilder::GetOrCreateGlobalDecl(const std::string &str, TyIdx tyIdx, bool &created) const
{
    GStrIdx strIdx = GetStringIndex(str);
    if (strIdx != 0u) {
        StIdx stIdx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(strIdx);
        if (stIdx.Idx() != 0) {
            created = false;
            return GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
        }
    }
    created = true;
    strIdx = GetOrCreateStringIndex(str);
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    st->SetNameStrIdx(strIdx);
    st->SetTyIdx(tyIdx);
    (void)GlobalTables::GetGsymTable().AddToStringSymbolMap(*st);
    return st;
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

MIRSymbol *MIRBuilder::CreateLocalDecl(const std::string &str, const MIRType &type)
{
    MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
    return MIRSymbolBuilder::Instance().CreateLocalDecl(*currentFunctionInner->GetSymTab(), GetOrCreateStringIndex(str),
                                                        type);
}

MIRSymbol *MIRBuilder::GetGlobalDecl(const std::string &str)
{
    return MIRSymbolBuilder::Instance().GetGlobalDecl(GetStringIndex(str));
}

MIRSymbol *MIRBuilder::GetLocalDecl(const std::string &str)
{
    MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
    return MIRSymbolBuilder::Instance().GetLocalDecl(*currentFunctionInner->GetSymTab(), GetStringIndex(str));
}

// search the scope hierarchy
MIRSymbol *MIRBuilder::GetDecl(const std::string &str)
{
    GStrIdx strIdx = GetStringIndex(str);
    MIRSymbol *sym = nullptr;
    if (strIdx != 0u) {
        // try to find the decl in local scope first
        MIRFunction *currentFunctionInner = GetCurrentFunction();
        if (currentFunctionInner != nullptr) {
            sym = currentFunctionInner->GetSymTab()->GetSymbolFromStrIdx(strIdx);
        }
        if (sym == nullptr) {
            sym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
        }
    }
    return sym;
}

MIRSymbol *MIRBuilder::CreateGlobalDecl(const std::string &str, const MIRType &type, MIRStorageClass sc)
{
    return MIRSymbolBuilder::Instance().CreateGlobalDecl(GetOrCreateStringIndex(str), type, sc);
}

MIRSymbol *MIRBuilder::GetOrCreateGlobalDecl(const std::string &str, const MIRType &type)
{
    bool isCreated = false;
    MIRSymbol *st = GetOrCreateGlobalDecl(str, type.GetTypeIndex(), isCreated);
    DEBUG_ASSERT(st != nullptr, "null ptr check");
    if (isCreated) {
        st->SetStorageClass(kScGlobal);
        st->SetSKind(kStVar);
    } else {
        // Existing symbol may come from anther module. We need to register it
        // in the current module so that per-module mpl file is self-sustained.
        mirModule->AddSymbol(st);
    }
    MIRConst *cst = GlobalTables::GetConstPool().GetConstFromPool(st->GetNameStrIdx());
    if (cst != nullptr) {
        st->SetKonst(cst);
    }
    return st;
}

MIRSymbol *MIRBuilder::GetSymbolFromEnclosingScope(StIdx stIdx) const
{
    if (stIdx.FullIdx() == 0) {
        return nullptr;
    }
    if (stIdx.Islocal()) {
        MIRFunction *fun = GetCurrentFunctionNotNull();
        MIRSymbol *st = fun->GetSymTab()->GetSymbolFromStIdx(stIdx.Idx());
        if (st != nullptr) {
            return st;
        }
    }
    return GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
}

MIRSymbol *MIRBuilder::GetSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass, MIRStorageClass sClass,
                                 uint8 scpID, bool sameType = false) const
{
    return GetSymbol(tyIdx, GetOrCreateStringIndex(name), mClass, sClass, scpID, sameType);
}

// when sametype is true, it means match everything the of the symbol
MIRSymbol *MIRBuilder::GetSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass, uint8 scpID,
                                 bool sameType = false) const
{
    if (scpID != kScopeGlobal) {
        ERR(kLncErr, "not yet implemented");
        return nullptr;
    }
    return MIRSymbolBuilder::Instance().GetSymbol(tyIdx, strIdx, mClass, sClass, sameType);
}

MIRSymbol *MIRBuilder::GetOrCreateSymbol(TyIdx tyIdx, const std::string &name, MIRSymKind mClass,
                                         MIRStorageClass sClass, MIRFunction *func, uint8 scpID,
                                         bool sametype = false) const
{
    return GetOrCreateSymbol(tyIdx, GetOrCreateStringIndex(name), mClass, sClass, func, scpID, sametype);
}

MIRSymbol *MIRBuilder::GetOrCreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mClass, MIRStorageClass sClass,
                                         MIRFunction *func, uint8 scpID, bool sameType = false) const
{
    if (MIRSymbol *st = GetSymbol(tyIdx, strIdx, mClass, sClass, scpID, sameType)) {
        return st;
    }
    return CreateSymbol(tyIdx, strIdx, mClass, sClass, func, scpID);
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

MIRSymbol *MIRBuilder::CreateConstStringSymbol(const std::string &symbolName, const std::string &content)
{
    auto elemPrimType = PTY_u8;
    MIRType *type = GlobalTables::GetTypeTable().GetPrimType(elemPrimType);
    uint64 sizeIn = static_cast<uint64>(content.length());
    MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(
        *GlobalTables::GetTypeTable().GetPrimType(elemPrimType), 1, &sizeIn);

    if (GetLocalDecl(symbolName)) {
        return GetLocalDecl(symbolName);
    }
    MIRSymbol *arrayVar = GetOrCreateGlobalDecl(symbolName, *arrayTypeWithSize);
    arrayVar->SetAttr(ATTR_readonly);
    arrayVar->SetStorageClass(kScFstatic);
    MIRAggConst *val = mirModule->GetMemPool()->New<MIRAggConst>(*mirModule, *arrayTypeWithSize);
    for (uint32 i = 0; i < sizeIn; ++i) {
        MIRConst *cst = mirModule->GetMemPool()->New<MIRIntConst>(content[i], *type);
        val->PushBack(cst);
    }
    // This interface is only for string literal, 0 is added to the end of the string.
    MIRConst *cst0 = mirModule->GetMemPool()->New<MIRIntConst>(0, *type);
    val->PushBack(cst0);
    arrayVar->SetKonst(val);
    return arrayVar;
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

ConstvalNode *MIRBuilder::CreateFloatConst(float val)
{
    auto *mirConst =
        GetCurrentFuncDataMp()->New<MIRFloatConst>(val, *GlobalTables::GetTypeTable().GetPrimType(PTY_f32));
    return NewNode<ConstvalNode>(PTY_f32, mirConst);
}

ConstvalNode *MIRBuilder::CreateDoubleConst(double val)
{
    auto *mirConst =
        GetCurrentFuncDataMp()->New<MIRDoubleConst>(val, *GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
    return NewNode<ConstvalNode>(PTY_f64, mirConst);
}

ConstvalNode *MIRBuilder::CreateFloat128Const(const uint64 *val)
{
    auto *mirConst =
        GetCurrentFuncDataMp()->New<MIRFloat128Const>(*val, *GlobalTables::GetTypeTable().GetPrimType(PTY_f128));
    return NewNode<ConstvalNode>(PTY_f128, mirConst);
}

ConstvalNode *MIRBuilder::GetConstInt(MemPool &memPool, int val)
{
    auto *mirConst =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *GlobalTables::GetTypeTable().GetInt64());
    return memPool.New<ConstvalNode>(PTY_i32, mirConst);
}

ConstvalNode *MIRBuilder::CreateAddrofConst(BaseNode &node)
{
    DEBUG_ASSERT(node.GetOpCode() == OP_addrof, "illegal op for addrof const");
    MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();

    // determine the type of 'node' and create a pointer type, accordingly
    auto &aNode = static_cast<AddrofNode &>(node);
    const MIRSymbol *var = currentFunctionInner->GetLocalOrGlobalSymbol(aNode.GetStIdx());
    DEBUG_ASSERT(var != nullptr, "var should not be nullptr");
    TyIdx ptyIdx = var->GetTyIdx();
    MIRPtrType ptrType(ptyIdx);
    ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
    MIRType &exprType = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
    auto *temp = mirModule->GetMemPool()->New<MIRAddrofConst>(aNode.GetStIdx(), aNode.GetFieldID(), exprType);
    return NewNode<ConstvalNode>(PTY_ptr, temp);
}

ConstvalNode *MIRBuilder::CreateAddroffuncConst(const BaseNode &node)
{
    DEBUG_ASSERT(node.GetOpCode() == OP_addroffunc, "illegal op for addroffunc const");

    const auto &aNode = static_cast<const AddroffuncNode &>(node);
    MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(aNode.GetPUIdx());
    CHECK_NULL_FATAL(f->GetFuncSymbol());
    TyIdx ptyIdx = f->GetFuncSymbol()->GetTyIdx();
    MIRPtrType ptrType(ptyIdx);
    ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
    MIRType *exprType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
    auto *mirConst = mirModule->GetMemPool()->New<MIRAddroffuncConst>(aNode.GetPUIdx(), *exprType);
    return NewNode<ConstvalNode>(PTY_ptr, mirConst);
}

ConstvalNode *MIRBuilder::CreateStrConst(const BaseNode &node)
{
    DEBUG_ASSERT(node.GetOpCode() == OP_conststr, "illegal op for conststr const");
    UStrIdx strIdx = static_cast<const ConststrNode &>(node).GetStrIdx();
    CHECK_FATAL(PTY_u8 < GlobalTables::GetTypeTable().GetTypeTable().size(),
                "index is out of range in MIRBuilder::CreateStrConst");
    TyIdx tyIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8))->GetTypeIndex();
    MIRPtrType ptrType(tyIdx);
    tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
    MIRType *exprType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    auto *mirConst = mirModule->GetMemPool()->New<MIRStrConst>(strIdx, *exprType);
    return NewNode<ConstvalNode>(PTY_ptr, mirConst);
}

ConstvalNode *MIRBuilder::CreateStr16Const(const BaseNode &node)
{
    DEBUG_ASSERT(node.GetOpCode() == OP_conststr16, "illegal op for conststr16 const");
    U16StrIdx strIdx = static_cast<const Conststr16Node &>(node).GetStrIdx();
    CHECK_FATAL(PTY_u16 < GlobalTables::GetTypeTable().GetTypeTable().size(),
                "index out of range in MIRBuilder::CreateStr16Const");
    TyIdx ptyIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u16))->GetTypeIndex();
    MIRPtrType ptrType(ptyIdx);
    ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
    MIRType *exprType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
    auto *mirConst = mirModule->GetMemPool()->New<MIRStr16Const>(strIdx, *exprType);
    return NewNode<ConstvalNode>(PTY_ptr, mirConst);
}

SizeoftypeNode *MIRBuilder::CreateExprSizeoftype(const MIRType &type)
{
    return NewNode<SizeoftypeNode>(PTY_u32, type.GetTypeIndex());
}

FieldsDistNode *MIRBuilder::CreateExprFieldsDist(const MIRType &type, FieldID field1, FieldID field2)
{
    return NewNode<FieldsDistNode>(PTY_i32, type.GetTypeIndex(), field1, field2);
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

AddroffuncNode *MIRBuilder::CreateExprAddroffunc(PUIdx puIdx, MemPool *memPool)
{
    if (memPool == nullptr) {
        memPool = GetCurrentFuncCodeMp();
    }
    auto node = memPool->New<AddroffuncNode>(PTY_ptr, puIdx);
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

AddrofNode *MIRBuilder::CreateExprDread(MIRType &type, MIRSymbol &symbol)
{
    return CreateExprDread(type, 0, symbol);
}

AddrofNode *MIRBuilder::CreateExprDread(MIRSymbol &symbol, uint16 fieldID)
{
    if (fieldID == 0) {
        return CreateExprDread(symbol);
    }
    DEBUG_ASSERT(false, "NYI");
    return nullptr;
}

AddrofNode *MIRBuilder::CreateExprDread(MIRSymbol &symbol)
{
    return CreateExprDread(*symbol.GetType(), 0, symbol);
}

AddrofNode *MIRBuilder::CreateExprDread(PregIdx pregID, PrimType pty)
{
    auto *dread = NewNode<AddrofNode>(OP_dread, pty);
    dread->SetStFullIdx(pregID);
    return dread;
}

DreadoffNode *MIRBuilder::CreateExprDreadoff(Opcode op, PrimType pty, const MIRSymbol &symbol, int32 offset)
{
    DreadoffNode *node = NewNode<DreadoffNode>(op, pty);
    node->stIdx = symbol.GetStIdx();
    node->offset = offset;
    return node;
}

IreadNode *MIRBuilder::CreateExprIread(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID,
                                       BaseNode *addr)
{
    TyIdx returnTypeIdx = returnType.GetTypeIndex();
    CHECK(returnTypeIdx < GlobalTables::GetTypeTable().GetTypeTable().size(),
          "index out of range in MIRBuilder::CreateExprIread");
    DEBUG_ASSERT(fieldID != 0 || ptrType.GetPrimType() != PTY_agg,
                 "Error: Fieldid should not be 0 when trying to iread a field from type ");
    PrimType type = GetRegPrimType(returnType.GetPrimType());
    return NewNode<IreadNode>(OP_iread, type, ptrType.GetTypeIndex(), fieldID, addr);
}

IreadoffNode *MIRBuilder::CreateExprIreadoff(PrimType pty, int32 offset, BaseNode *opnd0)
{
    return NewNode<IreadoffNode>(pty, opnd0, offset);
}

IreadFPoffNode *MIRBuilder::CreateExprIreadFPoff(PrimType pty, int32 offset)
{
    return NewNode<IreadFPoffNode>(pty, offset);
}

IaddrofNode *MIRBuilder::CreateExprIaddrof(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID,
                                           BaseNode *addr)
{
    IaddrofNode *iAddrOfNode = CreateExprIread(returnType, ptrType, fieldID, addr);
    iAddrOfNode->SetOpCode(OP_iaddrof);
    return iAddrOfNode;
}

IaddrofNode *MIRBuilder::CreateExprIaddrof(PrimType returnTypePty, TyIdx ptrTypeIdx, FieldID fieldID, BaseNode *addr)
{
    return NewNode<IreadNode>(OP_iaddrof, returnTypePty, ptrTypeIdx, fieldID, addr);
}

UnaryNode *MIRBuilder::CreateExprUnary(Opcode opcode, const MIRType &type, BaseNode *opnd)
{
    return NewNode<UnaryNode>(opcode, type.GetPrimType(), opnd);
}

GCMallocNode *MIRBuilder::CreateExprGCMalloc(Opcode opcode, const MIRType &pType, const MIRType &type)
{
    return NewNode<GCMallocNode>(opcode, pType.GetPrimType(), type.GetTypeIndex());
}

JarrayMallocNode *MIRBuilder::CreateExprJarrayMalloc(Opcode opcode, const MIRType &pType, const MIRType &type,
                                                     BaseNode *opnd)
{
    return NewNode<JarrayMallocNode>(opcode, pType.GetPrimType(), type.GetTypeIndex(), opnd);
}

TypeCvtNode *MIRBuilder::CreateExprTypeCvt(Opcode o, PrimType toPrimType, PrimType fromPrimType, BaseNode &opnd)
{
    return NewNode<TypeCvtNode>(o, toPrimType, fromPrimType, &opnd);
}

TypeCvtNode *MIRBuilder::CreateExprTypeCvt(Opcode o, const MIRType &type, const MIRType &fromType, BaseNode *opnd)
{
    return CreateExprTypeCvt(o, type.GetPrimType(), fromType.GetPrimType(), *opnd);
}

ExtractbitsNode *MIRBuilder::CreateExprExtractbits(Opcode o, const MIRType &type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *opnd)
{
    return CreateExprExtractbits(o, type.GetPrimType(), bOffset, bSize, opnd);
}

ExtractbitsNode *MIRBuilder::CreateExprExtractbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *opnd)
{
    return NewNode<ExtractbitsNode>(o, type, bOffset, bSize, opnd);
}

DepositbitsNode *MIRBuilder::CreateExprDepositbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize,
                                                   BaseNode *leftOpnd, BaseNode *rightOpnd)
{
    return NewNode<DepositbitsNode>(o, type, bOffset, bSize, leftOpnd, rightOpnd);
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

TernaryNode *MIRBuilder::CreateExprTernary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1,
                                           BaseNode *opnd2)
{
    return NewNode<TernaryNode>(opcode, type.GetPrimType(), opnd0, opnd1, opnd2);
}

CompareNode *MIRBuilder::CreateExprCompare(Opcode opcode, const MIRType &type, const MIRType &opndType, BaseNode *opnd0,
                                           BaseNode *opnd1)
{
    return NewNode<CompareNode>(opcode, type.GetPrimType(), opndType.GetPrimType(), opnd0, opnd1);
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType)
{
    MIRType *addrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(arrayType);
    DEBUG_ASSERT(addrType != nullptr, "addrType is null");
    auto *arrayNode = NewNode<ArrayNode>(*GetCurrentFuncCodeMpAllocator(), addrType->GetPrimType(),
                                                             addrType->GetTypeIndex());
    arrayNode->SetNumOpnds(0);
    return arrayNode;
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType, BaseNode *op)
{
    ArrayNode *arrayNode = CreateExprArray(arrayType);
    arrayNode->GetNopnd().push_back(op);
    arrayNode->SetNumOpnds(1);
    return arrayNode;
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType, BaseNode *op1, BaseNode *op2)
{
    ArrayNode *arrayNode = CreateExprArray(arrayType, op1);
    arrayNode->GetNopnd().push_back(op2);
    arrayNode->SetNumOpnds(2);  // 2 operands
    return arrayNode;
}

ArrayNode *MIRBuilder::CreateExprArray(const MIRType &arrayType, std::vector<BaseNode *> ops)
{
    MIRType *addrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(arrayType);
    DEBUG_ASSERT(addrType != nullptr, "addrType is null");
    auto *arrayNode = NewNode<ArrayNode>(*GetCurrentFuncCodeMpAllocator(), addrType->GetPrimType(),
                                                             addrType->GetTypeIndex());
    arrayNode->GetNopnd().insert(arrayNode->GetNopnd().begin(), ops.begin(), ops.end());
    arrayNode->SetNumOpnds(static_cast<uint8>(ops.size()));
    return arrayNode;
}

IntrinsicopNode *MIRBuilder::CreateExprIntrinsicop(MIRIntrinsicID id, Opcode op, PrimType primType, TyIdx tyIdx,
                                                   const MapleVector<BaseNode *> &ops)
{
    auto *expr = NewNode<IntrinsicopNode>(*GetCurrentFuncCodeMpAllocator(), op, primType);
    expr->SetIntrinsic(id);
    expr->SetNOpnd(ops);
    expr->SetNumOpnds(ops.size());
    if (op == OP_intrinsicopwithtype) {
        expr->SetTyIdx(tyIdx);
    }
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

IassignoffNode *MIRBuilder::CreateStmtIassignoff(PrimType pty, int32 offset, BaseNode *addr, BaseNode *src)
{
    return NewNode<IassignoffNode>(pty, offset, addr, src);
}

IassignFPoffNode *MIRBuilder::CreateStmtIassignFPoff(Opcode op, PrimType pty, int32 offset, BaseNode *src)
{
    return NewNode<IassignFPoffNode>(op, pty, offset, src);
}

CallNode *MIRBuilder::CreateStmtCall(PUIdx puIdx, const MapleVector<BaseNode *> &args, Opcode opCode)
{
    auto *stmt = NewNode<CallNode>(*GetCurrentFuncCodeMpAllocator(), opCode, puIdx, TyIdx());
    stmt->SetNOpnd(args);
    stmt->SetNumOpnds(args.size());
    return stmt;
}

CallNode *MIRBuilder::CreateStmtCall(const std::string &callee, const MapleVector<BaseNode *> &args)
{
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callee);
    StIdx stIdx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(strIdx);
    MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    DEBUG_ASSERT(st != nullptr, "MIRSymbol st is null");
    MIRFunction *func = st->GetFunction();
    DEBUG_ASSERT(func != nullptr, "func st is null");
    return CreateStmtCall(func->GetPuidx(), args, OP_call);
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

IcallNode *MIRBuilder::CreateStmtIcallprotoAssigned(const MapleVector<BaseNode *> &args, const MIRSymbol &ret)
{
    auto *stmt = NewNode<IcallNode>(*GetCurrentFuncCodeMpAllocator(), OP_icallprotoassigned);
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

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments,
                                                       TyIdx tyIdx)
{
    auto *stmt = NewNode<IntrinsiccallNode>(
        *GetCurrentFuncCodeMpAllocator(), tyIdx == 0u ? OP_intrinsiccall : OP_intrinsiccallwithtype, idx);
    stmt->SetTyIdx(tyIdx);
    stmt->SetOpnds(arguments);
    return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtXintrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments)
{
    auto *stmt =
        NewNode<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(), OP_xintrinsiccall, idx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(arguments);
    return stmt;
}

CallNode *MIRBuilder::CreateStmtCallAssigned(PUIdx puIdx, const MIRSymbol *ret, Opcode op)
{
    auto *stmt = NewNode<CallNode>(*GetCurrentFuncCodeMpAllocator(), op, puIdx);
    if (ret) {
        DEBUG_ASSERT(ret->IsLocal(), "Not Excepted ret");
        stmt->GetReturnVec().push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
    }
    return stmt;
}

CallNode *MIRBuilder::CreateStmtCallAssigned(PUIdx puIdx, const MapleVector<BaseNode *> &args, const MIRSymbol *ret,
                                             Opcode opcode, TyIdx tyIdx)
{
    auto *stmt = NewNode<CallNode>(*GetCurrentFuncCodeMpAllocator(), opcode, puIdx, tyIdx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    if (ret != nullptr) {
        DEBUG_ASSERT(ret->IsLocal(), "Not Excepted ret");
        stmt->GetReturnVec().push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
    }
    return stmt;
}

CallNode *MIRBuilder::CreateStmtCallRegassigned(PUIdx puIdx, PregIdx pRegIdx, Opcode opcode, BaseNode *arg)
{
    auto *stmt = NewNode<CallNode>(*GetCurrentFuncCodeMpAllocator(), opcode, puIdx);
    stmt->GetNopnd().push_back(arg);
    stmt->SetNumOpnds(stmt->GetNopndSize());
    if (pRegIdx > 0) {
        stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, pRegIdx)));
    }
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
                                                               PregIdx retPregIdx)
{
    auto *stmt =
        NewNode<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(), OP_intrinsiccallassigned, idx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    if (retPregIdx > 0) {
        stmt->GetReturnVec().push_back(CallReturnPair(StIdx(), RegFieldPair(0, retPregIdx)));
    }
    return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode *> &args,
                                                               const MIRSymbol *ret, TyIdx tyIdx)
{
    auto *stmt = NewNode<IntrinsiccallNode>(
        *GetCurrentFuncCodeMpAllocator(), tyIdx == 0u ? OP_intrinsiccallassigned : OP_intrinsiccallwithtypeassigned,
        idx);
    stmt->SetTyIdx(tyIdx);
    stmt->SetOpnds(args);
    CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
    if (ret != nullptr) {
        DEBUG_ASSERT(ret->IsLocal(), "Not Excepted ret");
        nrets.push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
    }
    stmt->SetReturnVec(nrets);
    return stmt;
}

IntrinsiccallNode *MIRBuilder::CreateStmtXintrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode *> &args,
                                                                const MIRSymbol *ret)
{
    auto *stmt = NewNode<IntrinsiccallNode>(*GetCurrentFuncCodeMpAllocator(),
                                                                OP_xintrinsiccallassigned, idx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(args);
    CallReturnVector nrets(GetCurrentFuncCodeMpAllocator()->Adapter());
    if (ret != nullptr) {
        DEBUG_ASSERT(ret->IsLocal(), "Not Excepted ret");
        nrets.push_back(CallReturnPair(ret->GetStIdx(), RegFieldPair(0, 0)));
    }
    stmt->SetReturnVec(nrets);
    return stmt;
}

NaryStmtNode *MIRBuilder::CreateStmtReturn(BaseNode *rVal)
{
    auto *stmt = NewNode<NaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), OP_return);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->PushOpnd(rVal);
    return stmt;
}

NaryStmtNode *MIRBuilder::CreateStmtNary(Opcode op, const MapleVector<BaseNode *> &rVals)
{
    auto *stmt = NewNode<NaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(rVals);
    return stmt;
}

CallAssertBoundaryStmtNode *MIRBuilder::CreateStmtCallAssertBoundary(Opcode op, const MapleVector<BaseNode *> &rVals,
                                                                     GStrIdx funcNameIdx, size_t paramIndex,
                                                                     GStrIdx stmtFuncNameIdx)
{
    auto *stmt = NewNode<CallAssertBoundaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op,
                                                                         funcNameIdx, paramIndex, stmtFuncNameIdx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(rVals);
    return stmt;
}

NaryStmtNode *MIRBuilder::CreateStmtNary(Opcode op, BaseNode *rVal)
{
    auto *stmt = NewNode<NaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->PushOpnd(rVal);
    return stmt;
}

AssertNonnullStmtNode *MIRBuilder::CreateStmtAssertNonnull(Opcode op, BaseNode *rVal, GStrIdx funcNameIdx)
{
    auto *stmt = NewNode<AssertNonnullStmtNode>(op, funcNameIdx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetRHS(rVal);
    return stmt;
}

AssertBoundaryStmtNode *MIRBuilder::CreateStmtAssertBoundary(Opcode op, const MapleVector<BaseNode *> &rVals,
                                                             GStrIdx funcNameIdx)
{
    auto *stmt = NewNode<AssertBoundaryStmtNode>(*GetCurrentFuncCodeMpAllocator(), op, funcNameIdx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetOpnds(rVals);
    return stmt;
}

CallAssertNonnullStmtNode *MIRBuilder::CreateStmtCallAssertNonnull(Opcode op, BaseNode *rVal, GStrIdx callFuncNameIdx,
                                                                   size_t paramIndex, GStrIdx stmtFuncNameIdx)
{
    auto *stmt =
        NewNode<CallAssertNonnullStmtNode>(op, callFuncNameIdx, paramIndex, stmtFuncNameIdx);
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmt->SetRHS(rVal);
    return stmt;
}

UnaryStmtNode *MIRBuilder::CreateStmtUnary(Opcode op, BaseNode *rVal)
{
    return NewNode<UnaryStmtNode>(op, kPtyInvalid, rVal);
}

UnaryStmtNode *MIRBuilder::CreateStmtThrow(BaseNode *rVal)
{
    return CreateStmtUnary(OP_throw, rVal);
}

IfStmtNode *MIRBuilder::CreateStmtIf(BaseNode *cond)
{
    auto *ifStmt = NewNode<IfStmtNode>();
    ifStmt->SetOpnd(cond, 0);
    BlockNode *thenBlock = NewNode<BlockNode>();
    ifStmt->SetThenPart(thenBlock);
    return ifStmt;
}

IfStmtNode *MIRBuilder::CreateStmtIfThenElse(BaseNode *cond)
{
    auto *ifStmt = NewNode<IfStmtNode>();
    ifStmt->SetOpnd(cond, 0);
    auto *thenBlock = NewNode<BlockNode>();
    ifStmt->SetThenPart(thenBlock);
    auto *elseBlock = NewNode<BlockNode>();
    ifStmt->SetElsePart(elseBlock);
    ifStmt->SetNumOpnds(3);  // 3 operands
    return ifStmt;
}

DoloopNode *MIRBuilder::CreateStmtDoloop(StIdx doVarStIdx, bool isPReg, BaseNode *startExp, BaseNode *contExp,
                                         BaseNode *incrExp)
{
    return NewNode<DoloopNode>(doVarStIdx, isPReg, startExp, contExp, incrExp,
                                                   NewNode<BlockNode>());
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

JsTryNode *MIRBuilder::CreateStmtJsTry(Opcode, LabelIdx cLabIdx, LabelIdx fLabIdx)
{
    return NewNode<JsTryNode>(static_cast<uint16>(cLabIdx), static_cast<uint16>(fLabIdx));
}

TryNode *MIRBuilder::CreateStmtTry(const MapleVector<LabelIdx> &cLabIdxs)
{
    return NewNode<TryNode>(cLabIdxs);
}

CatchNode *MIRBuilder::CreateStmtCatch(const MapleVector<TyIdx> &tyIdxVec)
{
    return NewNode<CatchNode>(tyIdxVec);
}

LabelNode *MIRBuilder::CreateStmtLabel(LabelIdx labIdx)
{
    return NewNode<LabelNode>(labIdx);
}

StmtNode *MIRBuilder::CreateStmtComment(const std::string &cmnt)
{
    return NewNode<CommentNode>(*GetCurrentFuncCodeMpAllocator(), cmnt);
}

AddrofNode *MIRBuilder::CreateAddrof(const MIRSymbol &st, PrimType pty)
{
    return NewNode<AddrofNode>(OP_addrof, pty, st.GetStIdx(), 0);
}

AddrofNode *MIRBuilder::CreateDread(const MIRSymbol &st, PrimType pty)
{
    return NewNode<AddrofNode>(OP_dread, pty, st.GetStIdx(), 0);
}

CondGotoNode *MIRBuilder::CreateStmtCondGoto(BaseNode *cond, Opcode op, LabelIdx labIdx)
{
    return NewNode<CondGotoNode>(op, labIdx, cond);
}

LabelIdx MIRBuilder::GetOrCreateMIRLabel(const std::string &name)
{
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    MIRFunction *currentFunctionInner = GetCurrentFunctionNotNull();
    LabelIdx lableIdx = currentFunctionInner->GetLabelTab()->GetLabelIdxFromStrIdx(strIdx);
    if (lableIdx == 0) {
        lableIdx = currentFunctionInner->GetLabelTab()->CreateLabel();
        currentFunctionInner->GetLabelTab()->SetSymbolFromStIdx(lableIdx, strIdx);
        currentFunctionInner->GetLabelTab()->AddToStringLabelMap(lableIdx);
    }
    return lableIdx;
}

LabelIdx MIRBuilder::CreateLabIdx(MIRFunction &mirFunc)
{
    LabelIdx lableIdx = mirFunc.GetLabelTab()->CreateLabel();
    mirFunc.GetLabelTab()->AddToStringLabelMap(lableIdx);
    return lableIdx;
}

void MIRBuilder::AddStmtInCurrentFunctionBody(StmtNode &stmt)
{
    MIRFunction *fun = GetCurrentFunctionNotNull();
    stmt.GetSrcPos().CondSetLineNum(lineNum);
    fun->GetBody()->AddStatement(&stmt);
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

MemPool *MIRBuilder::GetCurrentFuncDataMp()
{
    if (MIRFunction *curFunction = GetCurrentFunction()) {
        return curFunction->GetDataMemPool();
    }
    return mirModule->GetMemPool();
}

MIRBuilderExt::MIRBuilderExt(MIRModule *module, pthread_mutex_t *mutex) : MIRBuilder(module), mutex(mutex) {}

MemPool *MIRBuilderExt::GetCurrentFuncCodeMp()
{
    DEBUG_ASSERT(curFunction, "curFunction is null");
    return curFunction->GetCodeMemPool();
}

MapleAllocator *MIRBuilderExt::GetCurrentFuncCodeMpAllocator()
{
    DEBUG_ASSERT(curFunction, "curFunction is null");
    return &curFunction->GetCodeMemPoolAllocator();
}

void MIRBuilderExt::GlobalLock()
{
    if (mutex) {
        DEBUG_ASSERT(pthread_mutex_lock(mutex) == 0, "lock failed");
    }
}

void MIRBuilderExt::GlobalUnlock()
{
    if (mutex) {
        DEBUG_ASSERT(pthread_mutex_unlock(mutex) == 0, "unlock failed");
    }
}
}  // namespace maple
