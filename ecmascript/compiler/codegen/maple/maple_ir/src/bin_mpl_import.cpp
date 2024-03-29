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

void BinaryMplImport::ImportConstBase(MIRConstKind &kind, MIRTypePtr &type)
{
    kind = static_cast<MIRConstKind>(ReadNum());
    TyIdx tyidx = mod.IsJavaModule() ? ImportType() : ImportTypeNonJava();
    type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyidx);
}

MIRConst *BinaryMplImport::ImportConst(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return nullptr;
    }

    MIRConstKind kind;
    MIRType *type = nullptr;
    MemPool *memPool = mod.GetMemPool();

    ImportConstBase(kind, type);
    switch (tag) {
        case kBinKindConstInt:
            return GlobalTables::GetIntConstTable().GetOrCreateIntConst(ReadNum(), *type);
        case kBinKindConstAddrof: {
            MIRSymbol *sym = InSymbol(func);
            CHECK_FATAL(sym != nullptr, "null ptr check");
            FieldID fi = ReadNum();
            int32 ofst = static_cast<int32>(ReadNum());
            // do not use "type"; instead, get exprTy from sym
            TyIdx ptyIdx = sym->GetTyIdx();
            MIRPtrType ptrType(ptyIdx, (mod.IsJavaModule() ? PTY_ref : GetExactPtrPrimType()));
            ptyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&ptrType);
            MIRType *exprTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx);
            return memPool->New<MIRAddrofConst>(sym->GetStIdx(), fi, *exprTy, ofst);
        }
        case kBinKindConstAddrofLocal: {
            MIRSymbol *sym = ImportLocalSymbol(func);
            FieldID fi = static_cast<FieldID>(ReadNum());
            int32 ofst = static_cast<int32>(ReadNum());
            return memPool->New<MIRAddrofConst>(sym->GetStIdx(), fi, *type, ofst);
        }
        case kBinKindConstAddrofFunc: {
            PUIdx puIdx = ImportFunction();
            MIRFunction *f = GlobalTables::GetFunctionTable().GetFuncTable()[puIdx];
            f->GetFuncSymbol()->SetAppearsInCode(true);
            mod.SetCurFunction(func);
            return memPool->New<MIRAddroffuncConst>(puIdx, *type);
        }
        case kBinKindConstAddrofLabel: {
            LabelIdx lidx = ImportLabel(func);
            PUIdx puIdx = func->GetPuidx();
            MIRLblConst *lblConst = memPool->New<MIRLblConst>(lidx, puIdx, *type);
            (void)func->GetLabelTab()->addrTakenLabels.insert(lidx);
            return lblConst;
        }
        case kBinKindConstStr: {
            UStrIdx ustr = ImportUsrStr();
            return memPool->New<MIRStrConst>(ustr, *type);
        }
        case kBinKindConstStr16: {
            Conststr16Node *cs;
            cs = memPool->New<Conststr16Node>();
            cs->SetPrimType(type->GetPrimType());
            int64 len = ReadNum();
            std::ostringstream ostr;
            for (int64 i = 0; i < len; ++i) {
                ostr << Read();
            }
            std::u16string str16;
            (void)namemangler::UTF8ToUTF16(str16, ostr.str());
            cs->SetStrIdx(GlobalTables::GetU16StrTable().GetOrCreateStrIdxFromName(str16));
            return memPool->New<MIRStr16Const>(cs->GetStrIdx(), *type);
        }
        case kBinKindConstFloat: {
            union {
                float fvalue;
                int32 ivalue;
            } value;

            value.ivalue = ReadNum();
            return GlobalTables::GetFpConstTable().GetOrCreateFloatConst(value.fvalue);
        }
        case kBinKindConstDouble: {
            union {
                double dvalue;
                int64 ivalue;
            } value;

            value.ivalue = ReadNum();
            return GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(value.dvalue);
        }
        case kBinKindConstAgg: {
            MIRAggConst *aggConst = mod.GetMemPool()->New<MIRAggConst>(mod, *type);
            int64 size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                auto fieldId = static_cast<uint32>(ReadNum());
                auto fieldConst = ImportConst(func);
                aggConst->AddItem(fieldConst, fieldId);
            }
            return aggConst;
        }
        case kBinKindConstSt: {
            MIRStConst *stConst = mod.GetMemPool()->New<MIRStConst>(mod, *type);
            int64 size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                stConst->PushbackSymbolToSt(InSymbol(func));
            }
            size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                stConst->PushbackOffsetToSt(ReadNum());
            }
            return stConst;
        }
        default:
            CHECK_FATAL(false, "Unhandled const type");
    }
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
    if (mod.IsJavaModule()) {
        p->SetTyIdx(ImportType());
        p->SetTyIdxEx(ImportType());
    } else {
        p->SetTyIdx(ImportTypeNonJava());
        p->SetTyIdxEx(ImportTypeNonJava());
    }
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
    fp.second.first = mod.IsJavaModule() ? ImportType() : ImportTypeNonJava();
    fp.second.second.SetAttrFlag(ReadNum());
    fp.second.second.SetAlignValue(ReadNum());
    FieldAttrs fa = fp.second.second;
    if (fa.GetAttr(FLDATTR_static) && fa.GetAttr(FLDATTR_final) &&
        (fa.GetAttr(FLDATTR_public) || fa.GetAttr(FLDATTR_protected))) {
        int64 tag = ReadNum();
        if (tag == kBinInitConst) {
            GlobalTables::GetConstPool().InsertConstPool(fp.first, ImportConst(nullptr));
        }
    }
}

void BinaryMplImport::ImportMethodPair(MethodPair &memPool)
{
    std::string funcName;
    ReadAsciiStr(funcName);
    TyIdx funcTyIdx = ImportType();
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
        TyIdx idx = ImportType();
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
    TyIdx tempType = ImportType();
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
        TyIdx t0 = ImportType();
        TyIdx t1 = ImportType();
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

inline static bool IsIncomplete(const MIRType &type)
{
    return (type.GetKind() == kTypeInterfaceIncomplete || type.GetKind() == kTypeClassIncomplete ||
            type.GetKind() == kTypeStructIncomplete);
}

TyIdx BinaryMplImport::ImportType(bool forPointedType)
{
    int64 tag = ReadNum();
    static MIRType *typeNeedsComplete = nullptr;
    static int ptrLev = 0;
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
        case kBinKindTypePointer: {
            MIRPtrType type(primType, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            size_t idx = typTab.size();
            typTab.push_back(TyIdx(0));
            type.SetTypeAttrs(ImportTypeAttrs());
            ++ptrLev;
            type.SetPointedTyIdx(ImportType(true));
            --ptrLev;
            MIRType *origType = &InsertInTypeTables(type);
            typTab[idx] = origType->GetTypeIndex();
            if (typeNeedsComplete != nullptr && ptrLev == 0) {
                TyIdx tyIdxNeedsComplete = typeNeedsComplete->GetTypeIndex();
                typeNeedsComplete = nullptr;
                CompleteAggInfo(tyIdxNeedsComplete);
            }
            return origType->GetTypeIndex();
        }
        case kBinKindTypeFArray: {
            MIRFarrayType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            size_t idx = typTab.size();
            typTab.push_back(TyIdx(0));
            type.SetElemtTyIdx(ImportType(forPointedType));
            MIRType *origType = &InsertInTypeTables(type);
            typTab[idx] = origType->GetTypeIndex();
            return origType->GetTypeIndex();
        }
        case kBinKindTypeJarray: {
            MIRJarrayType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            size_t idx = typTab.size();
            typTab.push_back(TyIdx(0));
            type.SetElemtTyIdx(ImportType(forPointedType));
            MIRType *origType = &InsertInTypeTables(type);
            typTab[idx] = origType->GetTypeIndex();
            return origType->GetTypeIndex();
        }
        case kBinKindTypeArray: {
            MIRArrayType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetDim(ReadNum());
            CHECK_FATAL(type.GetDim() < kMaxArrayDim, "array index out of range");
            for (uint16 i = 0; i < type.GetDim(); ++i) {
                type.SetSizeArrayItem(i, ReadNum());
            }
            size_t idx = typTab.size();
            typTab.push_back(TyIdx(0));
            type.SetElemTyIdx(ImportType(forPointedType));
            type.SetTypeAttrs(ImportTypeAttrs());
            MIRType *origType = &InsertInTypeTables(type);
            typTab[idx] = origType->GetTypeIndex();
            return origType->GetTypeIndex();
        }
        case kBinKindTypeFunction: {
            MIRFuncType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            size_t idx = typTab.size();
            typTab.push_back(TyIdx(0));
            type.SetRetTyIdx(ImportType());
            type.funcAttrs.SetAttrFlag(ReadNum());
            int64 size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                type.GetParamTypeList().push_back(ImportType());
            }
            size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                type.GetParamAttrsList().push_back(ImportTypeAttrs());
            }
            MIRType *origType = &InsertInTypeTables(type);
            typTab[idx] = origType->GetTypeIndex();
            return origType->GetTypeIndex();
        }
        case kBinKindTypeParam: {
            MIRTypeParam type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            MIRType *origType = &InsertInTypeTables(type);
            typTab.push_back(origType->GetTypeIndex());
            return origType->GetTypeIndex();
        }
        case kBinKindTypeInstantVector: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRInstantVectorType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            auto *origType = static_cast<MIRInstantVectorType *>(&InsertInTypeTables(type));
            typTab.push_back(origType->GetTypeIndex());
            ImportTypePairs(origType->GetInstantVec());
            return origType->GetTypeIndex();
        }
        case kBinKindTypeGenericInstant: {
            MIRGenericInstantType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            auto *origType = static_cast<MIRGenericInstantType *>(&InsertInTypeTables(type));
            typTab.push_back(origType->GetTypeIndex());
            ImportTypePairs(origType->GetInstantVec());
            origType->SetGenericTyIdx(ImportType());
            return origType->GetTypeIndex();
        }
        case kBinKindTypeBitField: {
            uint8 fieldSize = static_cast<uint8>(ReadNum());
            MIRBitFieldType type(fieldSize, primType, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            MIRType *origType = &InsertInTypeTables(type);
            typTab.push_back(origType->GetTypeIndex());
            return origType->GetTypeIndex();
        }
        case kBinKindTypeStruct: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRStructType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetTypeAttrs(ImportTypeAttrs());
            MIRStructType &origType = static_cast<MIRStructType &>(InsertInTypeTables(type));
            typTab.push_back(origType.GetTypeIndex());
            if (kind != kTypeStructIncomplete) {
                if (forPointedType) {
                    typeNeedsComplete = &origType;
                } else {
                    ImportStructTypeData(origType);
                }
            }
            return origType.GetTypeIndex();
        }
        case kBinKindTypeClass: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRClassType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            auto &origType = static_cast<MIRClassType &>(InsertInTypeTables(type));
            typTab.push_back(origType.GetTypeIndex());
            if (kind != kTypeClassIncomplete) {
                if (forPointedType) {
                    typeNeedsComplete = &origType;
                } else {
                    ImportStructTypeData(origType);
                    ImportClassTypeData(origType);
                }
            }
            return origType.GetTypeIndex();
        }
        case kBinKindTypeInterface: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRInterfaceType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            auto &origType = static_cast<MIRInterfaceType &>(InsertInTypeTables(type));
            typTab.push_back(origType.GetTypeIndex());
            if (kind != kTypeInterfaceIncomplete) {
                if (forPointedType) {
                    typeNeedsComplete = &origType;
                } else {
                    ImportStructTypeData(origType);
                    ImportInterfaceTypeData(origType);
                }
            }
            return origType.GetTypeIndex();
        }
        default:
            CHECK_FATAL(false, "Unexpected binary kind");
    }
}

TyIdx BinaryMplImport::ImportTypeNonJava()
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return TyIdx(0);
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < typTab.size(), "index out of bounds");
        return typTab[static_cast<uint64>(-tag)];
    }
    PrimType primType = static_cast<PrimType>(0);
    GStrIdx strIdx(0);
    bool nameIsLocal = false;
    ImportTypeBase(primType, strIdx, nameIsLocal);
    TyIdx tyIdxUsed(GlobalTables::GetTypeTable().GetTypeTableSize());
    if (tag != kBinKindTypeScalar) {
        GlobalTables::GetTypeTable().PushNull();
        typTab.push_back(tyIdxUsed);
    }

    switch (tag) {
        case kBinKindTypeScalar:
            return TyIdx(primType);
        case kBinKindTypePointer: {
            MIRPtrType type(primType, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetTypeAttrs(ImportTypeAttrs());
            type.SetPointedTyIdx(ImportTypeNonJava());
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeFArray: {
            MIRFarrayType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetElemtTyIdx(ImportTypeNonJava());
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeJarray: {
            MIRJarrayType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetElemtTyIdx(ImportTypeNonJava());
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeArray: {
            MIRArrayType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetDim(ReadNum());
            CHECK_FATAL(type.GetDim() < kMaxArrayDim, "array index out of range");
            for (uint16 i = 0; i < type.GetDim(); ++i) {
                type.SetSizeArrayItem(i, ReadNum());
            }
            type.SetElemTyIdx(ImportTypeNonJava());
            type.SetTypeAttrs(ImportTypeAttrs());
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeFunction: {
            MIRFuncType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetRetTyIdx(ImportTypeNonJava());
            type.funcAttrs.SetAttrFlag(ReadNum());
            int64 size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                type.GetParamTypeList().push_back(ImportTypeNonJava());
            }
            size = ReadNum();
            for (int64 i = 0; i < size; ++i) {
                type.GetParamAttrsList().push_back(ImportTypeAttrs());
            }
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeParam: {
            MIRTypeParam type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeInstantVector: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRInstantVectorType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            ImportTypePairs(type.GetInstantVec());
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeGenericInstant: {
            MIRGenericInstantType type(strIdx);
            type.SetNameIsLocal(nameIsLocal);
            ImportTypePairs(type.GetInstantVec());
            type.SetGenericTyIdx(ImportTypeNonJava());
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeBitField: {
            uint8 fieldSize = static_cast<uint8>(ReadNum());
            MIRBitFieldType type(fieldSize, primType, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, false);
            return tyIdxUsed;
        }
        case kBinKindTypeStruct: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRStructType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            type.SetTypeAttrs(ImportTypeAttrs());
            if (kind != kTypeStructIncomplete) {
                ImportStructTypeData(type);
            }
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, false, IsIncomplete(type));
            return tyIdxUsed;
        }
        case kBinKindTypeClass: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRClassType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            if (kind != kTypeClassIncomplete) {
                ImportStructTypeData(type);
                ImportClassTypeData(type);
            }
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, true, IsIncomplete(type));
            return tyIdxUsed;
        }
        case kBinKindTypeInterface: {
            auto kind = static_cast<MIRTypeKind>(ReadNum());
            MIRInterfaceType type(kind, strIdx);
            type.SetNameIsLocal(nameIsLocal);
            if (kind != kTypeInterfaceIncomplete) {
                ImportStructTypeData(type);
                ImportInterfaceTypeData(type);
            }
            GlobalTables::GetTypeTable().CreateMirTypeNodeAt(type, tyIdxUsed, &mod, true, IsIncomplete(type));
            return tyIdxUsed;
        }
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

inline static bool IsObject(const MIRType &type)
{
    return (type.GetKind() == kTypeClass || type.GetKind() == kTypeClassIncomplete ||
            type.GetKind() == kTypeInterface || type.GetKind() == kTypeInterfaceIncomplete);
}

MIRType &BinaryMplImport::InsertInTypeTables(MIRType &type)
{
    MIRType *resultTypePtr = &type;
    TyIdx prevTyIdx = mod.GetTypeNameTab()->GetTyIdxFromGStrIdx(type.GetNameStrIdx());
    if (prevTyIdx != 0u && !type.IsNameIsLocal()) {
        MIRType *prevType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(prevTyIdx);
        if (!prevType->IsMIRTypeByName() &&
            ((IsIncomplete(*prevType) && IsIncomplete(type)) || (!IsIncomplete(*prevType) && !IsIncomplete(type)) ||
             (!IsIncomplete(*prevType) && IsIncomplete(type)))) {
            resultTypePtr = prevType->CopyMIRTypeNode();
            if (resultTypePtr->GetKind() == kTypeStruct || resultTypePtr->GetKind() == kTypeUnion ||
                resultTypePtr->GetKind() == kTypeStructIncomplete) {
                tmpStruct.push_back(static_cast<MIRStructType *>(resultTypePtr));
            } else if (resultTypePtr->GetKind() == kTypeClass || resultTypePtr->GetKind() == kTypeClassIncomplete) {
                tmpClass.push_back(static_cast<MIRClassType *>(resultTypePtr));
            } else if (resultTypePtr->GetKind() == kTypeInterface ||
                       resultTypePtr->GetKind() == kTypeInterfaceIncomplete) {
                tmpInterface.push_back(static_cast<MIRInterfaceType *>(resultTypePtr));
            }
        } else {
            // New definition wins
            type.SetTypeIndex(prevTyIdx);
            CHECK_FATAL(GlobalTables::GetTypeTable().GetTypeTable().empty() == false, "container check");
            GlobalTables::GetTypeTable().SetTypeWithTyIdx(prevTyIdx, *type.CopyMIRTypeNode());
            resultTypePtr = GlobalTables::GetTypeTable().GetTypeFromTyIdx(prevTyIdx);
            if (!IsIncomplete(*resultTypePtr)) {
                GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(resultTypePtr->GetNameStrIdx(),
                                                                   resultTypePtr->GetTypeIndex());
            }
        }
    } else {
        // New type, no previous definition or anonymous type
        TyIdx tyIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&type);
        resultTypePtr = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        if (tyIdx + 1 == GlobalTables::GetTypeTable().GetTypeTable().size() && !resultTypePtr->IsNameIsLocal()) {
            GStrIdx stridx = resultTypePtr->GetNameStrIdx();
            if (stridx != 0u) {
                mod.GetTypeNameTab()->SetGStrIdxToTyIdx(stridx, tyIdx);
                mod.PushbackTypeDefOrder(stridx);
                if (IsObject(*resultTypePtr)) {
                    mod.AddClass(tyIdx);
                    if (!IsIncomplete(*resultTypePtr)) {
                        GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(stridx, tyIdx);
                    }
                }
            }
        }
    }
    return *resultTypePtr;
}

void BinaryMplImport::SetupEHRootType()
{
    // setup eh root type with most recent Ljava_2Flang_2FObject_3B
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(namemangler::kJavaLangObjectStr);
    if (gStrIdx == 0u) {
        return;
    }

    TyIdx tyIdx = GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(gStrIdx);
    if (tyIdx != 0u) {
        mod.SetThrowableTyIdx(tyIdx);
    }
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

MIRSymbol *BinaryMplImport::InSymbol(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return nullptr;
    } else if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < symTab.size(), "index out of bounds");
        return symTab.at(-tag);
    } else {
        CHECK_FATAL(tag == kBinSymbol, "expecting kBinSymbol");
        int64 scope = ReadNum();
        GStrIdx stridx = ImportStr();
        UStrIdx secAttr = ImportUsrStr();
        UStrIdx asmAttr = ImportUsrStr();
        auto skind = static_cast<MIRSymKind>(ReadNum());
        auto sclass = static_cast<MIRStorageClass>(ReadNum());
        TyIdx tyTmp(0);
        MIRSymbol *sym = GetOrCreateSymbol(tyTmp, stridx, skind, sclass, func, scope);
        if (secAttr != 0u) {
            sym->sectionAttr = secAttr;
        }
        if (asmAttr != 0u) {
            sym->SetAsmAttr(asmAttr);
        }
        symTab.push_back(sym);
        sym->SetAttrs(ImportTypeAttrs());
        sym->SetIsTmp(ReadNum() != 0);
        sym->SetIsImported(imported);
        uint32 thepregno = 0;
        if (skind == kStPreg) {
            CHECK_FATAL(scope == kScopeLocal && func != nullptr, "Expecting kScopeLocal");
            thepregno = static_cast<uint32>(ReadNum());
        } else if (skind == kStConst || skind == kStVar) {
            sym->SetKonst(ImportConst(func));
        } else if (skind == kStFunc) {
            PUIdx puidx = ImportFunction();
            mod.SetCurFunction(func);
            if (puidx != 0) {
                sym->SetFunction(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puidx));
            }
        }
        if (skind == kStVar || skind == kStFunc) {
            ImportSrcPos(sym->GetSrcPosition());
        }
        TyIdx tyIdx = mod.IsJavaModule() ? ImportType() : ImportTypeNonJava();
        sym->SetTyIdx(tyIdx);
        if (skind == kStPreg) {
            MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
            PregIdx pregidx = func->GetPregTab()->EnterPregNo(thepregno, mirType->GetPrimType(), mirType);
            MIRPregTable *pregTab = func->GetPregTab();
            MIRPreg *preg = pregTab->PregFromPregIdx(pregidx);
            preg->SetPrimType(mirType->GetPrimType());
            sym->SetPreg(preg);
        }
        return sym;
    }
}

PUIdx BinaryMplImport::ImportFunction()
{
    int64 tag = ReadNum();
    if (tag == 0) {
        mod.SetCurFunction(nullptr);
        return 0;
    } else if (tag < 0) {
        CHECK_FATAL(static_cast<uint64>(-tag) <= funcTab.size(), "index out of bounds");
        if (static_cast<uint64>(-tag) == funcTab.size()) {  // function was exported before its symbol
            return static_cast<PUIdx>(0);
        }
        PUIdx puIdx = funcTab[static_cast<uint64>(-tag)]->GetPuidx();
        mod.SetCurFunction(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx));
        return puIdx;
    }
    CHECK_FATAL(tag == kBinFunction, "expecting kBinFunction");
    MIRSymbol *funcSt = InSymbol(nullptr);
    CHECK_FATAL(funcSt != nullptr, "null ptr check");
    MIRFunction *func = nullptr;
    if (funcSt->GetFunction() == nullptr) {
        maple::MIRBuilder builder(&mod);
        func = builder.CreateFunction(funcSt->GetStIdx());
        funcTab.push_back(func);
    } else {
        func = funcSt->GetFunction();
        funcTab.push_back(func);
    }
    funcSt->SetFunction(func);
    methodSymbols.push_back(funcSt);
    if (mod.IsJavaModule()) {
        func->SetBaseClassFuncNames(funcSt->GetNameStrIdx());
    }
    TyIdx funcTyIdx = mod.IsJavaModule() ? ImportType() : ImportTypeNonJava();
    func->SetMIRFuncType(static_cast<MIRFuncType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcTyIdx)));

    func->SetStIdx(funcSt->GetStIdx());
    if (!inCG) {
        func->SetFuncAttrs(ReadNum());  // merge side effect
    } else {
        if (!func->IsDirty()) {
            func->SetDirty(true);
            func->SetFuncAttrs(ReadNum());  // merge side effect
        } else {
            FuncAttrs tmp;
            tmp.SetAttrFlag(ReadNum());
            if (func->IsNoDefArgEffect() != tmp.GetAttr(FUNCATTR_nodefargeffect)) {
                tmp.SetAttr(FUNCATTR_nodefargeffect, true);
            }
            if (func->IsNoDefEffect() != tmp.GetAttr(FUNCATTR_nodefeffect)) {
                tmp.SetAttr(FUNCATTR_nodefeffect, true);
            }
            if (func->IsNoRetGlobal() != tmp.GetAttr(FUNCATTR_noretglobal)) {
                tmp.SetAttr(FUNCATTR_noretglobal, true);
            }
            if (func->IsNoThrowException() != tmp.GetAttr(FUNCATTR_nothrow_exception)) {
                tmp.SetAttr(FUNCATTR_nothrow_exception, true);
            }
            if (func->IsIpaSeen() != tmp.GetAttr(FUNCATTR_ipaseen)) {
                tmp.SetAttr(FUNCATTR_ipaseen);
            }
            if (func->IsPure() != tmp.GetAttr(FUNCATTR_pure)) {
                tmp.SetAttr(FUNCATTR_pure, true);
            }
            if (func->IsNoRetArg() != tmp.GetAttr(FUNCATTR_noretarg)) {
                tmp.SetAttr(FUNCATTR_noretarg, true);
            }
            if (func->IsNoPrivateDefEffect() != tmp.GetAttr(FUNCATTR_noprivate_defeffect)) {
                tmp.SetAttr(FUNCATTR_noprivate_defeffect, true);
            }
            func->SetFuncAttrs(tmp);
        }
    }

    auto &attributes = func->GetFuncAttrs();
    if (attributes.GetAttr(FUNCATTR_constructor_priority)) {
        attributes.SetConstructorPriority(static_cast<int>(ReadNum()));
    }
    if (attributes.GetAttr(FUNCATTR_destructor_priority)) {
        attributes.SetDestructorPriority(static_cast<int>(ReadNum()));
    }

    func->SetFlag(ReadNum());
    if (mod.IsJavaModule()) {
        (void)ImportType();  // not set the field to mimic parser
    } else {
        (void)ImportTypeNonJava();  // not set the field to mimic parser
    }
    size_t size = static_cast<size_t>(ReadNum());
    if (func->GetFormalDefVec().size() == 0) {
        for (size_t i = 0; i < size; i++) {
            GStrIdx strIdx = ImportStr();
            TyIdx tyIdx = mod.IsJavaModule() ? ImportType() : ImportTypeNonJava();
            FormalDef formalDef(strIdx, nullptr, tyIdx, TypeAttrs());
            formalDef.formalAttrs.SetAttrFlag(static_cast<uint64>(ReadNum()));
            func->GetFormalDefVec().push_back(formalDef);
        }
    } else {
        CHECK_FATAL(func->GetFormalDefVec().size() >= size, "ImportFunction: inconsistent number of formals");
        for (size_t i = 0; i < size; i++) {
            func->GetFormalDefVec()[i].formalStrIdx = ImportStr();
            func->GetFormalDefVec()[i].formalTyIdx = mod.IsJavaModule() ? ImportType() : ImportTypeNonJava();
            func->GetFormalDefVec()[i].formalAttrs.SetAttrFlag(static_cast<uint64>(ReadNum()));
        }
    }

    mod.SetCurFunction(func);
    return func->GetPuidx();
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

void BinaryMplImport::ReadHeaderField()
{
    SkipTotalSize();
    mod.SetFlavor(static_cast<MIRFlavor>(ReadNum()));
    mod.SetSrcLang(static_cast<MIRSrcLang>(ReadNum()));
    mod.SetID(static_cast<uint16>(ReadNum()));
    if (mod.GetFlavor() == kFlavorLmbc) {
        mod.SetGlobalMemSize(static_cast<uint32>(ReadNum()));
        mod.SetWithDbgInfo(static_cast<uint32>(ReadNum()));
    }
    mod.SetNumFuncs(static_cast<uint32>(ReadNum()));
    std::string inStr;
    ReadAsciiStr(inStr);
    mod.SetEntryFuncName(inStr);
    ImportInfoVector(mod.GetFileInfo(), mod.GetFileInfoIsString());

    int32 size = static_cast<int32>(ReadNum());
    MIRInfoPair infopair;
    for (int32 i = 0; i < size; i++) {
        infopair.first = ImportStr();
        infopair.second = static_cast<uint32>(ReadNum());
        mod.PushbackFileInfo(infopair);
    }

    size = static_cast<int32>(ReadNum());
    for (int32 i = 0; i < size; i++) {
        GStrIdx gStrIdx = ImportStr();
        mod.GetImportFiles().push_back(gStrIdx);
        std::string importfilename = GlobalTables::GetStrTable().GetStringFromStrIdx(gStrIdx);
        // record the imported file for later reading summary info, if exists
        mod.PushbackImportedMplt(importfilename);
        BinaryMplt *binMplt = new BinaryMplt(mod);
        binMplt->GetBinImport().imported = true;

        INFO(kLncInfo, "importing %s", importfilename.c_str());
        if (!binMplt->GetBinImport().Import(importfilename, false)) {  // not a binary mplt
            FATAL(kLncFatal, "cannot open binary MPLT file: %s\n", importfilename.c_str());
        } else {
            INFO(kLncInfo, "finished import of %s", importfilename.c_str());
        }
        if (i == 0) {
            binMplt->SetImportFileName(importfilename);
            mod.SetBinMplt(binMplt);
        } else {
            delete binMplt;
        }
    }

    size = static_cast<int32>(ReadNum());
    for (int32 i = 0; i < size; i++) {
        std::string str;
        ReadAsciiStr(str);
        mod.GetAsmDecls().emplace_back(MapleString(str, mod.GetMemPool()));
    }

    int32 tag = static_cast<int32>(ReadNum());
    CHECK_FATAL(tag == ~kBinHeaderStart, "pattern mismatch in Read Import");
    return;
}

void BinaryMplImport::ReadTypeField()
{
    SkipTotalSize();

    int32 size = ReadInt();
    if (mod.IsJavaModule()) {
        for (int64 i = 0; i < size; ++i) {
            ImportType();
        }
    } else {
        for (int64 i = 0; i < size; ++i) {
            (void)ImportTypeNonJava();
        }
    }
    int64 tag = 0;
    tag = ReadNum();
    CHECK_FATAL(tag == ~kBinTypeStart, "pattern mismatch in Read TYPE");
}

CallInfo *BinaryMplImport::ImportCallInfo()
{
    int64 tag = ReadNum();
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < callInfoTab.size(), "index out of bounds");
        return callInfoTab.at(-tag);
    }
    CHECK_FATAL(tag == kBinCallinfo, "expecting kBinCallinfo");
    CallType ctype = static_cast<CallType>(ReadNum());  // call type
    uint32 loopDepth = static_cast<uint32>(ReadInt());
    uint32 id = static_cast<uint32>(ReadInt());
    bool argLocal = Read() == 1;
    MIRSymbol *funcSym = InSymbol(nullptr);
    CHECK_FATAL(funcSym != nullptr, "func_sym is null in BinaryMplImport::InCallInfo");
    CallInfo *ret = mod.GetMemPool()->New<CallInfo>(ctype, *funcSym->GetFunction(), static_cast<StmtNode *>(nullptr),
                                                    loopDepth, id, argLocal);
    callInfoTab.push_back(ret);
    return ret;
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

void BinaryMplImport::ReadCgField()
{
    SkipTotalSize();

    int32 size = ReadInt();
    int64 tag = 0;

    for (int i = 0; i < size; ++i) {
        tag = ReadNum();
        CHECK_FATAL(tag == kStartMethod, " should be start point of method");
        MIRSymbol *tmpInSymbol = InSymbol(nullptr);
        CHECK_FATAL(tmpInSymbol != nullptr, "null ptr check");
        PUIdx methodPuidx = tmpInSymbol->GetFunction()->GetPuidx();
        CHECK_FATAL(methodPuidx, "should not be 0");
        if (mod.GetMethod2TargetMap().find(methodPuidx) == mod.GetMethod2TargetMap().end()) {
            std::vector<CallInfo *> targetSetTmp;
            mod.AddMemToMethod2TargetMap(methodPuidx, targetSetTmp);
        }
        int32 targSize = ReadInt();
        std::vector<CallInfo *> targetSet;
        callInfoTab.clear();
        callInfoTab.push_back(nullptr);
        for (int32 j = 0; j < targSize; ++j) {
            CallInfo *callInfo = ImportCallInfo();
            targetSet.push_back(callInfo);
        }
        MergeDuplicated(methodPuidx, mod.GetMemFromMethod2TargetMap(methodPuidx), targetSet);
        tag = ReadNum();
        CHECK_FATAL(tag == ~kStartMethod, " should be start point of method");
    }
    tag = ReadNum();
    CHECK_FATAL(tag == ~kBinCgStart, "pattern mismatch in Read CG");
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
            tyIdx = ImportType();
        }
        const std::string &funcStr = GlobalTables::GetStrTable().GetStringFromStrIdx(funcName);
        if (funcStr == "Ljava_2Flang_2FObject_3B_7Cwait_7C_28_29V") {
            specialEffect = 0;
        }
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

void BinaryMplImport::ReadSymField()
{
    SkipTotalSize();
    int32 size = ReadInt();
    for (int64 i = 0; i < size; i++) {
        (void)InSymbol(nullptr);
    }
    int64 tag = ReadNum();
    CHECK_FATAL(tag == ~kBinSymStart, "pattern mismatch in Read SYM");
    return;
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

bool BinaryMplImport::Import(const std::string &fname, bool readSymbols, bool readSe)
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
    if (readSe) {
        while (fieldID != kBinFinish) {
            if (fieldID == kBinSeStart) {
#ifdef MPLT_DEBUG
                LogInfo::MapleLogger() << "read SE of : " << fname << '\n';
#endif
                BinaryMplImport tmp(mod);
                tmp.Reset();
                tmp.buf = buf;
                tmp.bufI = bufI;
                tmp.importFileName = fname;
                tmp.ReadSeField();
                Jump2NextField();
            } else if (fieldID == kBinEaStart) {
                BinaryMplImport tmp(mod);
                tmp.Reset();
                tmp.buf = buf;
                tmp.bufI = bufI;
                tmp.importFileName = fname;
                tmp.ReadEaField();
                Jump2NextField();
            } else {
                Jump2NextField();
            }
            fieldID = ReadNum();
        }
        return true;
    }
    while (fieldID != kBinFinish) {
        switch (fieldID) {
            case kBinContentStart: {
                ReadContentField();
                break;
            }
            case kBinStrStart: {
                ReadStrField();
                break;
            }
            case kBinHeaderStart: {
                ReadHeaderField();
                break;
            }
            case kBinTypeStart: {
                ReadTypeField();
                break;
            }
            case kBinSymStart: {
                if (readSymbols) {
                    ReadSymField();
                } else {
                    Jump2NextField();
                }
                break;
            }
            case kBinSymTabStart: {
                ReadSymTabField();
                break;
            }
            case kBinCgStart: {
                if (readSymbols) {
#ifdef MPLT_DEBUG
                    LogInfo::MapleLogger() << "read CG of : " << fname << '\n';
#endif
                    BinaryMplImport tmp(mod);
                    tmp.Reset();
                    tmp.inIPA = true;
                    tmp.buf = buf;
                    tmp.bufI = bufI;
                    tmp.importFileName = fname;
                    tmp.ReadCgField();
                    tmp.UpdateMethodSymbols();
                    Jump2NextField();
                } else {
                    Jump2NextField();
                }
                break;
            }
            case kBinSeStart: {
                Jump2NextField();
                break;
            }
            case kBinEaStart: {
                if (readSymbols) {
#ifdef MPLT_DEBUG
                    LogInfo::MapleLogger() << "read EA of : " << fname << '\n';
#endif
                    BinaryMplImport tmp(mod);
                    tmp.Reset();
                    tmp.buf = buf;
                    tmp.bufI = bufI;
                    tmp.importFileName = fname;
                    tmp.ReadEaField();
                    Jump2NextField();
                } else {
                    Jump2NextField();
                }
                break;
            }
            case kBinFunctionBodyStart: {
                ReadFunctionBodyField();
                break;
            }
            default:
                CHECK_FATAL(false, "should not run here");
        }
        fieldID = ReadNum();
    }
    UpdateMethodSymbols();
    SetupEHRootType();
    return true;
}
}  // namespace maple
