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

#include "bin_mpl_import.h"
#include <sstream>
#include <vector>
#include <unordered_set>
#include <limits>
#include "bin_mplt.h"
#include "mir_function.h"
#include "namemangler.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"

namespace maple {
uint8 BinaryMplImport::Read()
{
    CHECK_FATAL(bufI < buf.size(), "Index out of bound in BinaryMplImport::Read()");
    return buf[bufI++];
}

// Little endian
int32 BinaryMplImport::ReadInt()
{
    uint32 x0 = static_cast<uint32>(Read());
    uint32 x1 = static_cast<uint32>(Read());
    uint32 x2 = static_cast<uint32>(Read());
    uint32 x3 = static_cast<uint32>(Read());
    return (((((x3 << 8u) + x2) << 8u) + x1) << 8u) + x0;
}

int64 BinaryMplImport::ReadInt64()
{
    // casts to avoid sign extension
    uint32 x0 = static_cast<uint32>(ReadInt());
    uint64 x1 = static_cast<uint32>(ReadInt());
    return static_cast<int64>((x1 << 32) + x0); // x1 left shift 32 bit to join with x0
}

// LEB128
int64 BinaryMplImport::ReadNum()
{
    uint64 n = 0;
    int64 y = 0;
    uint64 b = static_cast<uint64>(Read());
    while (b >= 0x80) {
        y += ((b - 0x80) << n);
        n += k7BitSize;
        b = static_cast<uint64>(Read());
    }
    b = (b & 0x3F) - (b & 0x40);
    return y + (b << n);
}

void BinaryMplImport::ReadAsciiStr(std::string &str)
{
    int64 n = ReadNum();
    for (int64 i = 0; i < n; i++) {
        uint8 ch = Read();
        str.push_back(static_cast<char>(ch));
    }
}

void BinaryMplImport::ReadFileAt(const std::string &name, int32 offset)
{
    FILE *f = fopen(name.c_str(), "rb");
    CHECK_FATAL(f != nullptr, "Error while reading the binary file: %s", name.c_str());

    int seekRet = fseek(f, 0, SEEK_END);
    CHECK_FATAL(seekRet == 0, "call fseek failed");

    long size = ftell(f);
    size -= offset;

    CHECK_FATAL(size >= 0, "should not be negative");

    seekRet = fseek(f, offset, SEEK_SET);
    CHECK_FATAL(seekRet == 0, "call fseek failed");
    buf.resize(size);

    size_t result = fread(&buf[0], sizeof(uint8), static_cast<size_t>(size), f);
    fclose(f);
    CHECK_FATAL(result == static_cast<size_t>(size), "Error while reading the binary file: %s", name.c_str());
}

GStrIdx BinaryMplImport::ImportStr()
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return GStrIdx(0);
    }
    if (tag < 0) {
        CHECK_FATAL(-tag < static_cast<int64>(gStrTab.size()), "index out of range in BinaryMplt::ImportStr");
        return gStrTab[-tag];
    }
    CHECK_FATAL(tag == kBinString, "expecting kBinString");
    std::string str;
    ReadAsciiStr(str);
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
    gStrTab.push_back(strIdx);
    return strIdx;
}

UStrIdx BinaryMplImport::ImportUsrStr()
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return UStrIdx(0);
    }
    if (tag < 0) {
        CHECK_FATAL(-tag < static_cast<int64>(uStrTab.size()), "index out of range in BinaryMplt::InUsrStr");
        return uStrTab[-tag];
    }
    CHECK_FATAL(tag == kBinUsrString, "expecting kBinUsrString");
    std::string str;
    ReadAsciiStr(str);
    UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
    uStrTab.push_back(strIdx);
    return strIdx;
}

MIRPragmaElement *BinaryMplImport::ImportPragmaElement()
{
    MIRPragmaElement *element = mod.GetPragmaMemPool()->New<MIRPragmaElement>(mod);
    element->SetNameStrIdx(ImportStr());
    element->SetTypeStrIdx(ImportStr());
    element->SetType(static_cast<PragmaValueType>(ReadNum()));
    if (element->GetType() == kValueString || element->GetType() == kValueType || element->GetType() == kValueField ||
        element->GetType() == kValueMethod || element->GetType() == kValueEnum) {
        element->SetI32Val(static_cast<int32>(ImportStr()));
    } else {
        element->SetU64Val(static_cast<uint64>(ReadInt64()));
    }
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        element->SubElemVecPushBack(ImportPragmaElement());
    }
    return element;
}

MIRPragma *BinaryMplImport::ImportPragma()
{
    MIRPragma *p = mod.GetPragmaMemPool()->New<MIRPragma>(mod);
    p->SetKind(static_cast<PragmaKind>(ReadNum()));
    p->SetVisibility(ReadNum());
    p->SetStrIdx(ImportStr());
    p->SetParamNum(ReadNum());
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        p->PushElementVector(ImportPragmaElement());
    }
    return p;
}

void BinaryMplImport::ImportFieldPair(FieldPair &fp)
{
    fp.first = ImportStr();
    fp.second.second.SetAttrFlag(ReadNum());
    fp.second.second.SetAlignValue(ReadNum());
}

void BinaryMplImport::ImportMethodPair(MethodPair &memPool)
{
    std::string funcName;
    ReadAsciiStr(funcName);
    TyIdx funcTyIdx = ImportJType();
    int64 x = ReadNum();
    CHECK_FATAL(x >= 0, "ReadNum error, x: %d", x);
    auto attrFlag = static_cast<uint64>(x);

    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
    MIRSymbol *prevFuncSt = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
    MIRSymbol *funcSt = nullptr;
    MIRFunction *fn = nullptr;

    if (prevFuncSt != nullptr && (prevFuncSt->GetStorageClass() == kScText && prevFuncSt->GetSKind() == kStFunc)) {
        funcSt = prevFuncSt;
        fn = funcSt->GetFunction();
    } else {
        funcSt = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
        funcSt->SetNameStrIdx(strIdx);
        GlobalTables::GetGsymTable().AddToStringSymbolMap(*funcSt);
        funcSt->SetStorageClass(kScText);
        funcSt->SetSKind(kStFunc);
        funcSt->SetTyIdx(funcTyIdx);
        funcSt->SetIsImported(imported);
        funcSt->SetIsImportedDecl(imported);
        methodSymbols.push_back(funcSt);

        fn = mod.GetMemPool()->New<MIRFunction>(&mod, funcSt->GetStIdx());
        fn->SetPuidx(GlobalTables::GetFunctionTable().GetFuncTable().size());
        GlobalTables::GetFunctionTable().GetFuncTable().push_back(fn);
        funcSt->SetFunction(fn);
        auto *funcType = static_cast<MIRFuncType *>(funcSt->GetType());
        fn->SetMIRFuncType(funcType);
        fn->SetFileIndex(0);
        fn->SetBaseClassFuncNames(funcSt->GetNameStrIdx());
        fn->SetFuncAttrs(attrFlag);
    }
    memPool.first.SetFullIdx(funcSt->GetStIdx().FullIdx());
    memPool.second.first.reset(funcTyIdx);
    memPool.second.second.SetAttrFlag(attrFlag);
}

void BinaryMplImport::UpdateMethodSymbols()
{
    for (auto sym : methodSymbols) {
        MIRFunction *fn = sym->GetFunction();
        CHECK_FATAL(fn != nullptr, "fn is null");
        auto *funcType = static_cast<MIRFuncType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx()));
        fn->SetMIRFuncType(funcType);
        fn->SetReturnStruct(*GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx()));
        if (fn->GetFormalDefVec().size() != 0) {
            continue;  // already updated in ImportFunction()
        }
        for (size_t i = 0; i < funcType->GetParamTypeList().size(); ++i) {
            FormalDef formalDef(nullptr, funcType->GetParamTypeList()[i], funcType->GetParamAttrsList()[i]);
            fn->GetFormalDefVec().push_back(formalDef);
        }
    }
}

void BinaryMplImport::ImportFieldsOfStructType(FieldVector &fields, uint32 methodSize)
{
    int64 size = ReadNum();
    int64 initSize = static_cast<int64>(fields.size() + methodSize);
    for (int64 i = 0; i < size; ++i) {
        FieldPair fp;
        ImportFieldPair(fp);
        if (initSize == 0) {
            fields.push_back(fp);
        }
    }
}

void BinaryMplImport::ImportMethodsOfStructType(MethodVector &methods)
{
    int64 size = ReadNum();
    bool isEmpty = methods.empty();
    for (int64 i = 0; i < size; ++i) {
        MethodPair memPool;
        ImportMethodPair(memPool);
        if (isEmpty) {
            methods.push_back(memPool);
        }
    }
}

void BinaryMplImport::ImportStructTypeData(MIRStructType &type)
{
    uint32 methodSize = type.GetMethods().size();
    ImportFieldsOfStructType(type.GetFields(), methodSize);
    ImportFieldsOfStructType(type.GetStaticFields(), methodSize);
    ImportFieldsOfStructType(type.GetParentFields(), methodSize);
    ImportMethodsOfStructType(type.GetMethods());
    type.SetIsImported(imported);
}

void BinaryMplImport::ImportInterfacesOfClassType(std::vector<TyIdx> &interfaces)
{
    int64 size = ReadNum();
    bool isEmpty = interfaces.empty();
    for (int64 i = 0; i < size; ++i) {
        TyIdx idx = ImportJType();
        if (isEmpty) {
            interfaces.push_back(idx);
        }
    }
}

void BinaryMplImport::ImportInfoIsStringOfStructType(MIRStructType &type)
{
    int64 size = ReadNum();
    bool isEmpty = type.GetInfoIsString().empty();

    for (int64 i = 0; i < size; ++i) {
        auto isString = static_cast<bool>(ReadNum());

        if (isEmpty) {
            type.PushbackIsString(isString);
        }
    }
}

void BinaryMplImport::ImportInfoOfStructType(MIRStructType &type)
{
    uint64 size = static_cast<uint64>(ReadNum());
    bool isEmpty = type.GetInfo().empty();
    for (size_t i = 0; i < size; ++i) {
        GStrIdx idx = ImportStr();
        int64 x = (type.GetInfoIsStringElemt(i)) ? static_cast<int64>(ImportStr()) : ReadNum();
        CHECK_FATAL(x >= 0, "ReadNum nagative, x: %d", x);
        CHECK_FATAL(x <= std::numeric_limits<uint32_t>::max(), "ReadNum too large, x: %d", x);
        if (isEmpty) {
            type.PushbackMIRInfo(MIRInfoPair(idx, static_cast<uint32>(x)));
        }
    }
}

void BinaryMplImport::ImportPragmaOfStructType(MIRStructType &type)
{
    int64 size = ReadNum();
    bool isEmpty = type.GetPragmaVec().empty();
    for (int64 i = 0; i < size; ++i) {
        MIRPragma *pragma = ImportPragma();
        if (isEmpty) {
            type.PushbackPragma(pragma);
        }
    }
}

void BinaryMplImport::SetClassTyidxOfMethods(MIRStructType &type)
{
    if (type.GetTypeIndex() != 0u) {
        // set up classTyIdx for methods
        for (size_t i = 0; i < type.GetMethods().size(); ++i) {
            StIdx stidx = type.GetMethodsElement(i).first;
            MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStidx(stidx.Idx());
            CHECK_FATAL(st != nullptr, "st is null");
            CHECK_FATAL(st->GetSKind() == kStFunc, "unexpected st->sKind");
            st->GetFunction()->SetClassTyIdx(type.GetTypeIndex());
        }
    }
}

void BinaryMplImport::ImportClassTypeData(MIRClassType &type)
{
    TyIdx tempType = ImportJType();
    // Keep the parent_tyidx we first met.
    if (type.GetParentTyIdx() == 0u) {
        type.SetParentTyIdx(tempType);
    }
    ImportInterfacesOfClassType(type.GetInterfaceImplemented());
    ImportInfoIsStringOfStructType(type);
    if (!inIPA) {
        ImportInfoOfStructType(type);
        ImportPragmaOfStructType(type);
    }
    SetClassTyidxOfMethods(type);
}

void BinaryMplImport::ImportInterfaceTypeData(MIRInterfaceType &type)
{
    ImportInterfacesOfClassType(type.GetParentsTyIdx());
    ImportInfoIsStringOfStructType(type);
    if (!inIPA) {
        ImportInfoOfStructType(type);
        ImportPragmaOfStructType(type);
    }
    SetClassTyidxOfMethods(type);
}

void BinaryMplImport::Reset()
{
    buf.clear();
    bufI = 0;
    gStrTab.clear();
    uStrTab.clear();
    typTab.clear();
    funcTab.clear();
    symTab.clear();
    methodSymbols.clear();
    definedLabels.clear();
    gStrTab.push_back(GStrIdx(0));  // Dummy
    uStrTab.push_back(UStrIdx(0));  // Dummy
    symTab.push_back(nullptr);      // Dummy
    funcTab.push_back(nullptr);     // Dummy
    eaCgTab.push_back(nullptr);
    for (int32 pti = static_cast<int32>(PTY_begin); pti < static_cast<int32>(PTY_end); ++pti) {
        typTab.push_back(TyIdx(pti));
    }
}

TypeAttrs BinaryMplImport::ImportTypeAttrs()
{
    TypeAttrs ta;
    ta.SetAttrFlag(static_cast<uint64>(ReadNum()));
    ta.SetAlignValue(static_cast<uint8>(ReadNum()));
    ta.SetPack(static_cast<uint32>(ReadNum()));
    return ta;
}

void BinaryMplImport::ImportTypePairs(std::vector<TypePair> &insVecType)
{
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        TyIdx t0 = ImportJType();
        TyIdx t1 = ImportJType();
        TypePair tp(t0, t1);
        insVecType.push_back(tp);
    }
}

void BinaryMplImport::CompleteAggInfo(TyIdx tyIdx)
{
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    CHECK_FATAL(type != nullptr, "MIRType is null");
    if (type->GetKind() == kTypeInterface) {
        auto *interfaceType = static_cast<MIRInterfaceType *>(type);
        ImportStructTypeData(*interfaceType);
        ImportInterfaceTypeData(*interfaceType);
    } else if (type->GetKind() == kTypeClass) {
        auto *classType = static_cast<MIRClassType *>(type);
        ImportStructTypeData(*classType);
        ImportClassTypeData(*classType);
    } else if (type->GetKind() == kTypeStruct || type->GetKind() == kTypeUnion) {
        auto *structType = static_cast<MIRStructType *>(type);
        ImportStructTypeData(*structType);
    } else {
        ERR(kLncErr, "in BinaryMplImport::CompleteAggInfo, MIRType error");
    }
}

TyIdx BinaryMplImport::ImportJType(bool forPointedType)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return TyIdx(0);
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < typTab.size(), "index out of bounds");
        return typTab.at(static_cast<uint64>(-tag));
    }
    PrimType primType = static_cast<PrimType>(0);
    GStrIdx strIdx(0);
    bool nameIsLocal = false;
    ImportTypeBase(primType, strIdx, nameIsLocal);

    switch (tag) {
        case kBinKindTypeScalar:
            return TyIdx(primType);
        default:
            CHECK_FATAL(false, "Unexpected binary kind");
    }
}

void BinaryMplImport::ImportTypeBase(PrimType &primType, GStrIdx &strIdx, bool &nameIsLocal)
{
    primType = static_cast<PrimType>(ReadNum());
    strIdx = ImportStr();
    nameIsLocal = ReadNum();
}

MIRSymbol *BinaryMplImport::GetOrCreateSymbol(TyIdx tyIdx, GStrIdx strIdx, MIRSymKind mclass, MIRStorageClass sclass,
                                              MIRFunction *func, uint8 scpID)
{
    MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx);
    if (st != nullptr && st->GetStorageClass() == sclass && st->GetSKind() == mclass && scpID == kScopeGlobal) {
        return st;
    }
    return mirBuilder.CreateSymbol(tyIdx, strIdx, mclass, sclass, func, scpID);
}

inline void BinaryMplImport::SkipTotalSize()
{
    ReadInt();
}

void BinaryMplImport::ReadStrField()
{
    SkipTotalSize();

    int32 size = ReadInt();
    for (int64 i = 0; i < size; ++i) {
        GStrIdx stridx = ImportStr();
        GlobalTables::GetConstPool().PutLiteralNameAsImported(stridx);
    }
    int64 tag = 0;
    tag = ReadNum();
    CHECK_FATAL(tag == ~kBinStrStart, "pattern mismatch in Read STR");
}

void BinaryMplImport::MergeDuplicated(PUIdx methodPuidx, std::vector<CallInfo *> &targetSet,
                                      std::vector<CallInfo *> &newSet)
{
    if (targetSet.empty()) {
        (void)targetSet.insert(targetSet.begin(), newSet.begin(), newSet.end());
        std::unordered_set<uint32> tmp;
        mod.AddValueToMethod2TargetHash(methodPuidx, tmp);
        for (size_t i = 0; i < newSet.size(); ++i) {
            mod.InsertTargetHash(methodPuidx, newSet[i]->GetID());
        }
    } else {
        for (size_t i = 0; i < newSet.size(); ++i) {
            CallInfo *newItem = newSet[i];
            if (!mod.HasTargetHash(methodPuidx, newItem->GetID())) {
                targetSet.push_back(newItem);
                mod.InsertTargetHash(methodPuidx, newItem->GetID());
            }
        }
    }
}

void BinaryMplImport::ReadEaField()
{
    ReadInt();
    int size = ReadInt();
    for (int i = 0; i < size; ++i) {
        GStrIdx funcName = ImportStr();
        int nodesSize = ReadInt();
        EAConnectionGraph *newEaCg =
            mod.GetMemPool()->New<EAConnectionGraph>(&mod, &mod.GetMPAllocator(), funcName, true);
        newEaCg->ResizeNodes(nodesSize, nullptr);
        InEaCgNode(*newEaCg);
        int eaSize = ReadInt();
        for (int j = 0; j < eaSize; ++j) {
            EACGBaseNode *node = &InEaCgNode(*newEaCg);
            newEaCg->funcArgNodes.push_back(node);
        }
        mod.SetEAConnectionGraph(funcName, newEaCg);
    }
    CHECK_FATAL(ReadNum() == ~kBinEaStart, "pattern mismatch in Read EA");
}

void BinaryMplImport::ReadSeField()
{
    SkipTotalSize();

    int32 size = ReadInt();
#ifdef MPLT_DEBUG
    LogInfo::MapleLogger() << "SE SIZE : " << size << '\n';
#endif
    for (int32 i = 0; i < size; ++i) {
        GStrIdx funcName = ImportStr();
        uint8 specialEffect = Read();
        TyIdx tyIdx = kInitTyIdx;
        if ((specialEffect & kPureFunc) == kPureFunc) {
            tyIdx = ImportJType();
        }
        const std::string &funcStr = GlobalTables::GetStrTable().GetStringFromStrIdx(funcName);
        auto *funcSymbol =
            GlobalTables::GetGsymTable().GetSymbolFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(funcStr));
        MIRFunction *func = funcSymbol != nullptr ? mirBuilder.GetFunctionFromSymbol(*funcSymbol) : nullptr;
        if (func != nullptr) {
            func->SetAttrsFromSe(specialEffect);
        } else if ((specialEffect & kPureFunc) == kPureFunc) {
            func = mirBuilder.GetOrCreateFunction(funcStr, tyIdx);
            func->SetAttrsFromSe(specialEffect);
        }
    }
    int64 tag = ReadNum();
    CHECK_FATAL(tag == ~kBinSeStart, "pattern mismatch in Read TYPE");
}

void BinaryMplImport::InEaCgBaseNode(EACGBaseNode &base, EAConnectionGraph &newEaCg, bool firstPart)
{
    if (firstPart) {
        base.SetEAStatus(static_cast<EAStatus>(ReadNum()));
        base.SetID(ReadInt());
    } else {
        // start to in points to
        int size = ReadInt();
        for (int i = 0; i < size; ++i) {
            EACGBaseNode *point2Node = &InEaCgNode(newEaCg);
            CHECK_FATAL(point2Node->IsObjectNode(), "must be");
            (void)base.pointsTo.insert(static_cast<EACGObjectNode *>(point2Node));
        }
        // start to in in
        size = ReadInt();
        for (int i = 0; i < size; ++i) {
            EACGBaseNode *point2Node = &InEaCgNode(newEaCg);
            base.InsertInSet(point2Node);
        }
        // start to in out
        size = ReadInt();
        for (int i = 0; i < size; ++i) {
            EACGBaseNode *point2Node = &InEaCgNode(newEaCg);
            base.InsertOutSet(point2Node);
        }
    }
}

void BinaryMplImport::InEaCgActNode(EACGActualNode &actual)
{
    actual.isPhantom = Read() == 1;
    actual.isReturn = Read() == 1;
    actual.argIdx = Read();
    actual.callSiteInfo = static_cast<uint32>(ReadInt());
}

void BinaryMplImport::InEaCgFieldNode(EACGFieldNode &field, EAConnectionGraph &newEaCg)
{
    field.SetFieldID(ReadInt());
    int size = ReadInt();
    for (int i = 0; i < size; ++i) {
        EACGBaseNode *node = &InEaCgNode(newEaCg);
        CHECK_FATAL(node->IsObjectNode(), "must be");
        (void)field.belongsTo.insert(static_cast<EACGObjectNode *>(node));
    }
    field.isPhantom = Read() == 1;
}

void BinaryMplImport::InEaCgObjNode(EACGObjectNode &obj, EAConnectionGraph &newEaCg)
{
    Read();
    obj.isPhantom = true;
    int size = ReadInt();
    for (int i = 0; i < size; ++i) {
        EACGBaseNode *node = &InEaCgNode(newEaCg);
        CHECK_FATAL(node->IsFieldNode(), "must be");
        auto *field = static_cast<EACGFieldNode *>(node);
        obj.fieldNodes[static_cast<EACGFieldNode *>(field)->GetFieldID()] = field;
    }
    // start to in point by
    size = ReadInt();
    for (int i = 0; i < size; ++i) {
        EACGBaseNode *point2Node = &InEaCgNode(newEaCg);
        (void)obj.pointsBy.insert(point2Node);
    }
}

void BinaryMplImport::InEaCgRefNode(EACGRefNode &ref)
{
    ref.isStaticField = Read() == 1 ? true : false;
}

EACGBaseNode &BinaryMplImport::InEaCgNode(EAConnectionGraph &newEaCg)
{
    int64 tag = ReadNum();
    if (tag < 0) {
        CHECK_FATAL(static_cast<uint64>(-tag) < eaCgTab.size(), "index out of bounds");
        return *eaCgTab[-tag];
    }
    CHECK_FATAL(tag == kBinEaCgNode, "must be");
    NodeKind kind = static_cast<NodeKind>(ReadNum());
    EACGBaseNode *node = nullptr;
    switch (kind) {
        case kObejectNode:
            node = new EACGObjectNode(&mod, &mod.GetMPAllocator(), &newEaCg);
            break;
        case kReferenceNode:
            node = new EACGRefNode(&mod, &mod.GetMPAllocator(), &newEaCg);
            break;
        case kFieldNode:
            node = new EACGFieldNode(&mod, &mod.GetMPAllocator(), &newEaCg);
            break;
        case kActualNode:
            node = new EACGActualNode(&mod, &mod.GetMPAllocator(), &newEaCg);
            break;
        default:
            CHECK_FATAL(false, "impossible");
    }
    node->SetEACG(&newEaCg);
    eaCgTab.push_back(node);
    InEaCgBaseNode(*node, newEaCg, true);
    CHECK_FATAL(node->id > 0, "must not be zero");
    newEaCg.SetNodeAt(node->id - 1, node);
    if (node->IsActualNode()) {
        CHECK_FATAL(ReadNum() == kBinEaCgActNode, "must be");
        InEaCgActNode(static_cast<EACGActualNode &>(*node));
    } else if (node->IsFieldNode()) {
        CHECK_FATAL(ReadNum() == kBinEaCgFieldNode, "must be");
        InEaCgFieldNode(static_cast<EACGFieldNode &>(*node), newEaCg);
    } else if (node->IsObjectNode()) {
        CHECK_FATAL(ReadNum() == kBinEaCgObjNode, "must be");
        InEaCgObjNode(static_cast<EACGObjectNode &>(*node), newEaCg);
    } else if (node->IsReferenceNode()) {
        CHECK_FATAL(ReadNum() == kBinEaCgRefNode, "must be");
        InEaCgRefNode(static_cast<EACGRefNode &>(*node));
    }
    InEaCgBaseNode(*node, newEaCg, false);
    CHECK_FATAL(ReadNum() == ~kBinEaCgNode, "must be");
    return *node;
}

EAConnectionGraph *BinaryMplImport::ReadEaCgField()
{
    if (ReadNum() == ~kBinEaCgStart) {
        return nullptr;
    }
    ReadInt();
    GStrIdx funcStr = ImportStr();
    int nodesSize = ReadInt();
    EAConnectionGraph *newEaCg = mod.GetMemPool()->New<EAConnectionGraph>(&mod, &mod.GetMPAllocator(), funcStr, true);
    newEaCg->ResizeNodes(nodesSize, nullptr);
    InEaCgNode(*newEaCg);
    CHECK_FATAL(newEaCg->GetNode(kFirstOpnd)->IsObjectNode(), "must be");
    CHECK_FATAL(newEaCg->GetNode(kSecondOpnd)->IsReferenceNode(), "must be");
    CHECK_FATAL(newEaCg->GetNode(kThirdOpnd)->IsFieldNode(), "must be");
    newEaCg->globalField = static_cast<EACGFieldNode *>(newEaCg->GetNode(kThirdOpnd));
    newEaCg->globalObj = static_cast<EACGObjectNode *>(newEaCg->GetNode(kFirstOpnd));
    newEaCg->globalRef = static_cast<EACGRefNode *>(newEaCg->GetNode(kSecondOpnd));
    CHECK_FATAL(newEaCg->globalField && newEaCg->globalObj && newEaCg->globalRef, "must be");
    int32 nodeSize = ReadInt();
    for (int j = 0; j < nodeSize; ++j) {
        EACGBaseNode *node = &InEaCgNode(*newEaCg);
        newEaCg->funcArgNodes.push_back(node);
    }

    int32 callSitesize = ReadInt();
    for (int i = 0; i < callSitesize; ++i) {
        uint32 id = static_cast<uint32>(ReadInt());
        newEaCg->callSite2Nodes[id] =
            mod.GetMemPool()->New<MapleVector<EACGBaseNode *>>(mod.GetMPAllocator().Adapter());
        int32 calleeArgSize = ReadInt();
        for (int j = 0; j < calleeArgSize; ++j) {
            EACGBaseNode *node = &InEaCgNode(*newEaCg);
            newEaCg->callSite2Nodes[id]->push_back(node);
        }
    }

#ifdef DEBUG
    for (EACGBaseNode *node : newEaCg->GetNodes()) {
        if (node == nullptr) {
            continue;
        }
        node->CheckAllConnectionInNodes();
    }
#endif
    CHECK_FATAL(ReadNum() == ~kBinEaCgStart, "pattern mismatch in Read EACG");
    return newEaCg;
}

void BinaryMplImport::ReadSymTabField()
{
    SkipTotalSize();
    int32 size = ReadInt();
    for (int64 i = 0; i < size; i++) {
        std::string str;
        ReadAsciiStr(str);
    }
    int64 tag = ReadNum();
    CHECK_FATAL(tag == ~kBinSymTabStart, "pattern mismatch in Read TYPE");
    return;
}

void BinaryMplImport::ReadContentField()
{
    SkipTotalSize();

    int32 size = ReadInt();
    int64 item;
    int32 offset;
    for (int32 i = 0; i < size; ++i) {
        item = ReadNum();
        offset = ReadInt();
        content[item] = offset;
    }
    CHECK_FATAL(ReadNum() == ~kBinContentStart, "pattern mismatch in Read CONTENT");
}

void BinaryMplImport::Jump2NextField()
{
    uint32 totalSize = static_cast<uint32>(ReadInt());
    bufI += (totalSize - sizeof(uint32));
    ReadNum();  // skip end tag for this field
}

bool BinaryMplImport::ImportForSrcLang(const std::string &fname, MIRSrcLang &srcLang)
{
    Reset();
    ReadFileAt(fname, 0);
    int32 magic = ReadInt();
    if (kMpltMagicNumber != magic && (kMpltMagicNumber + 0x10) != magic) {
        buf.clear();
        return false;
    }
    importingFromMplt = kMpltMagicNumber == magic;
    int64 fieldID = ReadNum();
    while (fieldID != kBinFinish) {
        switch (fieldID) {
            case kBinHeaderStart: {
                SkipTotalSize();
                (void)ReadNum();  // skip flavor
                srcLang = static_cast<MIRSrcLang>(ReadNum());
                return true;
            }
            default: {
                Jump2NextField();
                break;
            }
        }
        fieldID = ReadNum();
    }
    return false;
}
}  // namespace maple
