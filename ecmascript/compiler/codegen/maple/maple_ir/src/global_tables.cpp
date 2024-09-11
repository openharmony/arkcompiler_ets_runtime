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

#include "global_tables.h"
#include "mir_type.h"
#include "mir_symbol.h"

#if MIR_FEATURE_FULL
namespace maple {
MIRType *TypeTable::CreateMirType(uint32 primTypeIdx) const
{
    MIRTypeKind defaultKind = kTypeScalar;
    auto primType = static_cast<PrimType>(primTypeIdx);
    auto *mirType = new MIRType(defaultKind, primType);
    return mirType;
}

TypeTable::TypeTable()
{
    Init();
}

void TypeTable::Init()
{
    // enter the primitve types in type_table_
    typeTable.push_back(static_cast<MIRType *>(nullptr));
    DEBUG_ASSERT(typeTable.size() == static_cast<size_t>(PTY_void), "use PTY_void as the first index to type table");
    uint32 primTypeIdx;
    for (primTypeIdx = static_cast<uint32>(PTY_begin) + 1; primTypeIdx <= static_cast<uint32>(PTY_end); ++primTypeIdx) {
        MIRType *type = CreateMirType(primTypeIdx);
        type->SetTypeIndex(TyIdx {primTypeIdx});
        typeTable.push_back(type);
        PutToHashTable(type);
    }
    if (voidPtrType == nullptr) {
        voidPtrType = GetOrCreatePointerType(*GetVoid(), PTY_ptr);
    }
    lastDefaultTyIdx.SetIdx(primTypeIdx);
}

void TypeTable::Reset()
{
    ReleaseTypes();
    typeHashTable.clear();
    ptrTypeMap.clear();
    refTypeMap.clear();
    typeTable.clear();
    Init();
}

void TypeTable::ReleaseTypes()
{
    for (auto index = static_cast<uint32>(PTY_void); index < typeTable.size(); ++index) {
        delete typeTable[index];
        typeTable[index] = nullptr;
    }
    voidPtrType = nullptr;
}

TypeTable::~TypeTable()
{
    ReleaseTypes();
}

void TypeTable::PutToHashTable(MIRType *mirType)
{
    (void)typeHashTable.insert(mirType);
}

MIRType *TypeTable::CreateAndUpdateMirTypeNode(MIRType &pType)
{
    MIRType *nType = pType.CopyMIRTypeNode();
    nType->SetTypeIndex(TyIdx(typeTable.size()));
    typeTable.push_back(nType);

    if (pType.IsMIRPtrType()) {
        auto &pty = static_cast<MIRPtrType &>(pType);
        if (pty.GetTypeAttrs() == TypeAttrs()) {
            if (pty.GetPrimType() != PTY_ref) {
                ptrTypeMap[pty.GetPointedTyIdx()] = nType->GetTypeIndex();
            } else {
                refTypeMap[pty.GetPointedTyIdx()] = nType->GetTypeIndex();
            }
        } else {
            (void)typeHashTable.insert(nType);
        }
    } else {
        (void)typeHashTable.insert(nType);
    }
    return nType;
}

MIRType *TypeTable::GetOrCreateMIRTypeNode(MIRType &pType)
{
    if (pType.IsMIRPtrType()) {
        auto &type = static_cast<MIRPtrType &>(pType);
        if (type.GetTypeAttrs() == TypeAttrs()) {
            auto *pMap = (type.GetPrimType() != PTY_ref ? &ptrTypeMap : &refTypeMap);
            auto *otherPMap = (type.GetPrimType() == PTY_ref ? &ptrTypeMap : &refTypeMap);
            {
                std::shared_lock<std::shared_timed_mutex> lock(mtx);
                const auto it = pMap->find(type.GetPointedTyIdx());
                if (it != pMap->end()) {
                    return GetTypeFromTyIdx(it->second);
                }
            }
            std::unique_lock<std::shared_timed_mutex> lock(mtx);
            CHECK_FATAL(!(type.GetPointedTyIdx().GetIdx() >= kPtyDerived && type.GetPrimType() == PTY_ref &&
                          otherPMap->find(type.GetPointedTyIdx()) != otherPMap->end()),
                        "GetOrCreateMIRType: ref pointed-to type %d has previous ptr occurrence",
                        type.GetPointedTyIdx().GetIdx());
            return CreateAndUpdateMirTypeNode(pType);
        }
    }
    {
        std::shared_lock<std::shared_timed_mutex> lock(mtx);
        const auto it = typeHashTable.find(&pType);
        if (it != typeHashTable.end()) {
            return *it;
        }
    }
    std::unique_lock<std::shared_timed_mutex> lock(mtx);
    return CreateAndUpdateMirTypeNode(pType);
}

MIRType *TypeTable::voidPtrType = nullptr;
// get or create a type that pointing to pointedTyIdx
MIRType *TypeTable::GetOrCreatePointerType(const TyIdx &pointedTyIdx, PrimType primType, const TypeAttrs &attrs)
{
    MIRPtrType type(pointedTyIdx, primType);
    type.SetTypeAttrs(attrs);
    TyIdx tyIdx = GetOrCreateMIRType(&type);
    DEBUG_ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreatePointerType");
    return typeTable.at(tyIdx);
}

MIRType *TypeTable::GetOrCreatePointerType(const MIRType &pointTo, PrimType primType, const TypeAttrs &attrs)
{
    return GetOrCreatePointerType(pointTo.GetTypeIndex(), primType, attrs);
}

MIRType *TypeTable::GetOrCreateFunctionType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecType,
                                            const std::vector<TypeAttrs> &vecAttrs, bool isVarg,
                                            const TypeAttrs &retAttrs)
{
    MIRFuncType funcType(retTyIdx, vecType, vecAttrs, retAttrs);
    if (isVarg) {
        funcType.SetVarArgs();
    }
    TyIdx tyIdx = GetOrCreateMIRType(&funcType);
    DEBUG_ASSERT(tyIdx < typeTable.size(), "index out of range in TypeTable::GetOrCreateFunctionType");
    return typeTable.at(tyIdx);
}

void FPConstTable::PostInit()
{
    MIRType &typeFloat = *GlobalTables::GetTypeTable().GetPrimType(PTY_f32);
    nanFloatConst = new MIRFloatConst(NAN, typeFloat);
    infFloatConst = new MIRFloatConst(INFINITY, typeFloat);
    minusInfFloatConst = new MIRFloatConst(-INFINITY, typeFloat);
    minusZeroFloatConst = new MIRFloatConst(-0.0, typeFloat);
    MIRType &typeDouble = *GlobalTables::GetTypeTable().GetPrimType(PTY_f64);
    nanDoubleConst = new MIRDoubleConst(NAN, typeDouble);
    infDoubleConst = new MIRDoubleConst(INFINITY, typeDouble);
    minusInfDoubleConst = new MIRDoubleConst(-INFINITY, typeDouble);
    minusZeroDoubleConst = new MIRDoubleConst(-0.0, typeDouble);
}

MIRIntConst *IntConstTable::GetOrCreateIntConst(uint64 val, MIRType &type)
{
    return DoGetOrCreateIntConst(val, type);
}

MIRIntConst *IntConstTable::DoGetOrCreateIntConst(uint64 val, MIRType &type)
{
    IntConstKey key(val, type.GetTypeIndex());
    if (intConstTable.find(key) != intConstTable.end()) {
        return intConstTable[key];
    }
    intConstTable[key] = new MIRIntConst(val, type);
    return intConstTable[key];
}

IntConstTable::~IntConstTable()
{
    for (auto pair : intConstTable) {
        delete pair.second;
    }
}

MIRFloatConst *FPConstTable::GetOrCreateFloatConst(float floatVal)
{
    if (std::isnan(floatVal)) {
        return nanFloatConst;
    }
    if (std::isinf(floatVal)) {
        return (floatVal < 0) ? minusInfFloatConst : infFloatConst;
    }
    if (floatVal == 0.0 && std::signbit(floatVal)) {
        return minusZeroFloatConst;
    }
    return DoGetOrCreateFloatConst(floatVal);
}

MIRFloatConst *FPConstTable::DoGetOrCreateFloatConst(float floatVal)
{
    const auto it = floatConstTable.find(floatVal);
    if (it != floatConstTable.cend()) {
        return it->second;
    }
    // create a new one
    auto *floatConst = new MIRFloatConst(floatVal, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx {PTY_f32}));
    floatConstTable[floatVal] = floatConst;
    return floatConst;
}

MIRDoubleConst *FPConstTable::GetOrCreateDoubleConst(double doubleVal)
{
    if (std::isnan(doubleVal)) {
        return nanDoubleConst;
    }
    if (std::isinf(doubleVal)) {
        return (doubleVal < 0) ? minusInfDoubleConst : infDoubleConst;
    }
    if (doubleVal == 0.0 && std::signbit(doubleVal)) {
        return minusZeroDoubleConst;
    }
    return DoGetOrCreateDoubleConst(doubleVal);
}

MIRDoubleConst *FPConstTable::DoGetOrCreateDoubleConst(double doubleVal)
{
    const auto it = doubleConstTable.find(doubleVal);
    if (it != doubleConstTable.cend()) {
        return it->second;
    }
    // create a new one
    auto *doubleConst =
        new MIRDoubleConst(doubleVal, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f64)));
    doubleConstTable[doubleVal] = doubleConst;
    return doubleConst;
}

FPConstTable::~FPConstTable()
{
    delete nanFloatConst;
    delete infFloatConst;
    delete minusInfFloatConst;
    delete minusZeroFloatConst;
    delete nanDoubleConst;
    delete infDoubleConst;
    delete minusInfDoubleConst;
    delete minusZeroDoubleConst;
    for (const auto &floatConst : floatConstTable) {
        delete floatConst.second;
    }
    for (const auto &doubleConst : doubleConstTable) {
        delete doubleConst.second;
    }
}

GSymbolTable::GSymbolTable()
{
    Init();
}

void GSymbolTable::Init()
{
    symbolTable.push_back(static_cast<MIRSymbol *>(nullptr));
}

void GSymbolTable::Reset()
{
    ReleaseSymbols();
    symbolTable.clear();
    strIdxToStIdxMap.clear();
    Init();
}

void GSymbolTable::ReleaseSymbols()
{
    for (MIRSymbol *symbol : symbolTable) {
        delete symbol;
    }
}

GSymbolTable::~GSymbolTable()
{
    ReleaseSymbols();
}

MIRSymbol *GSymbolTable::CreateSymbol(uint8 scopeID)
{
    auto *st = new MIRSymbol(symbolTable.size(), scopeID);
    CHECK_FATAL(st != nullptr, "CreateSymbol failure");
    symbolTable.push_back(st);
    module->AddSymbol(st);
    return st;
}

bool GSymbolTable::AddToStringSymbolMap(const MIRSymbol &st)
{
    GStrIdx strIdx = st.GetNameStrIdx();
    if (strIdxToStIdxMap[strIdx].FullIdx() != 0) {
        return false;
    }
    strIdxToStIdxMap[strIdx] = st.GetStIdx();
    return true;
}

#ifdef ARK_LITECG_DEBUG
void GSymbolTable::Dump(bool isLocal, int32 indent) const
{
    for (size_t i = 1; i < symbolTable.size(); ++i) {
        const MIRSymbol *symbol = symbolTable[i];
        if (symbol != nullptr) {
            symbol->Dump(isLocal, indent);
        }
    }
}
#endif

GlobalTables GlobalTables::globalTables;
GlobalTables &GlobalTables::GetGlobalTables()
{
    return globalTables;
}
}  // namespace maple
#endif  // MIR_FEATURE_FULL
