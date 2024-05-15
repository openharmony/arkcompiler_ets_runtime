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

#include "bin_mpl_export.h"
#include <sstream>
#include <vector>
#include "mir_function.h"
#include "namemangler.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "bin_mplt.h"
#include "factory.h"

namespace {
using namespace maple;
/* Storage location of field  */
constexpr uint32 kFirstField = 0;
constexpr uint32 kSecondField = 1;
constexpr uint32 kThirdField = 2;
constexpr uint32 kFourthField = 3;
constexpr int32 kFourthFieldInt = 3;
constexpr uint32 kFifthField = 4;
constexpr int32 kSixthFieldInt = 5;

using OutputConstFactory = FunctionFactory<MIRConstKind, void, MIRConst &, BinaryMplExport &>;
using OutputTypeFactory = FunctionFactory<MIRTypeKind, void, MIRType &, BinaryMplExport &>;

void OutputConstInt(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstInt);
    mplExport.OutputConstBase(constVal);
    mplExport.WriteNum(static_cast<const MIRIntConst &>(constVal).GetExtValue());
}

void OutputConstAddrof(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    const MIRAddrofConst &addrof = static_cast<const MIRAddrofConst &>(constVal);
    if (addrof.GetSymbolIndex().IsGlobal()) {
        mplExport.WriteNum(kBinKindConstAddrof);
    } else {
        mplExport.WriteNum(kBinKindConstAddrofLocal);
    }
    mplExport.OutputConstBase(constVal);
    if (addrof.GetSymbolIndex().IsGlobal()) {
        mplExport.OutputSymbol(mplExport.GetMIRModule().CurFunction()->GetLocalOrGlobalSymbol(addrof.GetSymbolIndex()));
    } else {
        mplExport.OutputLocalSymbol(mplExport.curFunc->GetLocalOrGlobalSymbol(addrof.GetSymbolIndex()));
    }
    mplExport.WriteNum(addrof.GetFieldID());
    mplExport.WriteNum(addrof.GetOffset());
}

void OutputConstAddrofFunc(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstAddrofFunc);
    mplExport.OutputConstBase(constVal);
    const auto &newConst = static_cast<const MIRAddroffuncConst &>(constVal);
    mplExport.OutputFunction(newConst.GetValue());
}

void OutputConstLbl(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstAddrofLabel);
    mplExport.OutputConstBase(constVal);
    const MIRLblConst &lblConst = static_cast<const MIRLblConst &>(constVal);
    mplExport.OutputLabel(lblConst.GetValue());
}

void OutputConstStr(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstStr);
    mplExport.OutputConstBase(constVal);
    const auto &newConst = static_cast<const MIRStrConst &>(constVal);
    mplExport.OutputUsrStr(newConst.GetValue());
}

void OutputConstStr16(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstStr16);
    mplExport.OutputConstBase(constVal);
    const auto &mirStr16 = static_cast<const MIRStr16Const &>(constVal);
    std::u16string str16 = GlobalTables::GetU16StrTable().GetStringFromStrIdx(mirStr16.GetValue());
    std::string str;
    (void)namemangler::UTF16ToUTF8(str, str16);
    mplExport.WriteNum(str.length());
    for (char c : str) {
        mplExport.Write(static_cast<uint8>(c));
    }
}

void OutputConstFloat(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstFloat);
    mplExport.OutputConstBase(constVal);
    const auto &newConst = static_cast<const MIRFloatConst &>(constVal);
    mplExport.WriteNum(newConst.GetIntValue());
}

void OutputConstDouble(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstDouble);
    mplExport.OutputConstBase(constVal);
    const auto &newConst = static_cast<const MIRDoubleConst &>(constVal);
    mplExport.WriteNum(newConst.GetIntValue());
}

void OutputConstAgg(const MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstAgg);
    mplExport.OutputConstBase(constVal);
    const auto &aggConst = static_cast<const MIRAggConst &>(constVal);
    size_t size = aggConst.GetConstVec().size();
    mplExport.WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        mplExport.WriteNum(aggConst.GetFieldIdItem(i));
        mplExport.OutputConst(aggConst.GetConstVecItem(i));
    }
}

void OutputConstSt(MIRConst &constVal, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindConstSt);
    mplExport.OutputConstBase(constVal);
    auto &stConst = static_cast<MIRStConst &>(constVal);
    size_t size = stConst.GetStVec().size();
    mplExport.WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        mplExport.OutputSymbol(stConst.GetStVecItem(i));
    }
    size = stConst.GetStOffsetVec().size();
    mplExport.WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        mplExport.WriteNum(stConst.GetStOffsetVecItem(i));
    }
}

static bool InitOutputConstFactory()
{
    RegisterFactoryFunction<OutputConstFactory>(kConstInt, OutputConstInt);
    RegisterFactoryFunction<OutputConstFactory>(kConstAddrof, OutputConstAddrof);
    RegisterFactoryFunction<OutputConstFactory>(kConstAddrofFunc, OutputConstAddrofFunc);
    RegisterFactoryFunction<OutputConstFactory>(kConstLblConst, OutputConstLbl);
    RegisterFactoryFunction<OutputConstFactory>(kConstStrConst, OutputConstStr);
    RegisterFactoryFunction<OutputConstFactory>(kConstStr16Const, OutputConstStr16);
    RegisterFactoryFunction<OutputConstFactory>(kConstFloatConst, OutputConstFloat);
    RegisterFactoryFunction<OutputConstFactory>(kConstDoubleConst, OutputConstDouble);
    RegisterFactoryFunction<OutputConstFactory>(kConstAggConst, OutputConstAgg);
    RegisterFactoryFunction<OutputConstFactory>(kConstStConst, OutputConstSt);
    return true;
}

void OutputTypeScalar(const MIRType &ty, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindTypeScalar);
    mplExport.OutputTypeBase(ty);
}

void OutputTypePointer(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRPtrType &>(ty);
    mplExport.WriteNum(kBinKindTypePointer);
    mplExport.OutputTypeBase(type);
    mplExport.OutputTypeAttrs(type.GetTypeAttrs());
    mplExport.OutputType(type.GetPointedTyIdx());
}

void OutputTypeByName(const MIRType &ty, BinaryMplExport &mplExport)
{
    mplExport.WriteNum(kBinKindTypeByName);
    mplExport.OutputTypeBase(ty);
}

void OutputTypeFArray(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRFarrayType &>(ty);
    mplExport.WriteNum(kBinKindTypeFArray);
    mplExport.OutputTypeBase(type);
    mplExport.OutputType(type.GetElemTyIdx());
}

void OutputTypeJArray(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRJarrayType &>(ty);
    mplExport.WriteNum(kBinKindTypeJarray);
    mplExport.OutputTypeBase(type);
    mplExport.OutputType(type.GetElemTyIdx());
}

void OutputTypeArray(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRArrayType &>(ty);
    mplExport.WriteNum(kBinKindTypeArray);
    mplExport.OutputTypeBase(type);
    mplExport.WriteNum(type.GetDim());
    for (uint16 i = 0; i < type.GetDim(); ++i) {
        mplExport.WriteNum(type.GetSizeArrayItem(i));
    }
    mplExport.OutputType(type.GetElemTyIdx());
    mplExport.OutputTypeAttrs(type.GetTypeAttrs());
}

void OutputTypeFunction(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRFuncType &>(ty);
    mplExport.WriteNum(kBinKindTypeFunction);
    mplExport.OutputTypeBase(type);
    mplExport.OutputType(type.GetRetTyIdx());
    mplExport.WriteNum(type.funcAttrs.GetAttrFlag());
    size_t size = type.GetParamTypeList().size();
    mplExport.WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        mplExport.OutputType(type.GetNthParamType(i));
    }
    size = type.GetParamAttrsList().size();
    mplExport.WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        mplExport.OutputTypeAttrs(type.GetNthParamAttrs(i));
    }
}

void OutputTypeParam(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRTypeParam &>(ty);
    mplExport.WriteNum(kBinKindTypeParam);
    mplExport.OutputTypeBase(type);
}

void OutputTypeInstantVector(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRInstantVectorType &>(ty);
    mplExport.WriteNum(kBinKindTypeInstantVector);
    mplExport.OutputTypeBase(type);
    mplExport.WriteNum(ty.GetKind());
    mplExport.OutputTypePairs(type);
}

void OutputTypeGenericInstant(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRGenericInstantType &>(ty);
    mplExport.WriteNum(kBinKindTypeGenericInstant);
    mplExport.OutputTypeBase(type);
    mplExport.OutputTypePairs(type);
    mplExport.OutputType(type.GetGenericTyIdx());
}

void OutputTypeBitField(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRBitFieldType &>(ty);
    mplExport.WriteNum(kBinKindTypeBitField);
    mplExport.OutputTypeBase(type);
    mplExport.WriteNum(type.GetFieldSize());
}

// for Struct/StructIncomplete/Union
void OutputTypeStruct(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRStructType &>(ty);
    mplExport.WriteNum(kBinKindTypeStruct);
    mplExport.OutputTypeBase(type);
    MIRTypeKind kind = ty.GetKind();
    if (type.IsImported()) {
        CHECK_FATAL(ty.GetKind() != kTypeUnion, "Must be.");
        kind = kTypeStructIncomplete;
    }
    mplExport.WriteNum(kind);
    mplExport.OutputTypeAttrs(type.GetTypeAttrs());
    if (kind != kTypeStructIncomplete) {
        mplExport.OutputStructTypeData(type);
    }
}

void OutputTypeClass(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRClassType &>(ty);
    mplExport.WriteNum(kBinKindTypeClass);
    mplExport.OutputTypeBase(type);
    MIRTypeKind kind = ty.GetKind();
    if (type.IsImported()) {
        kind = kTypeClassIncomplete;
    }
    mplExport.WriteNum(kind);
    if (kind != kTypeClassIncomplete) {
        mplExport.OutputStructTypeData(type);
        mplExport.OutputClassTypeData(type);
    }
}

void OutputTypeInterface(const MIRType &ty, BinaryMplExport &mplExport)
{
    const auto &type = static_cast<const MIRInterfaceType &>(ty);
    mplExport.WriteNum(kBinKindTypeInterface);
    mplExport.OutputTypeBase(type);
    MIRTypeKind kind = ty.GetKind();
    if (type.IsImported()) {
        kind = kTypeInterfaceIncomplete;
    }
    mplExport.WriteNum(kind);
    if (kind != kTypeInterfaceIncomplete) {
        mplExport.OutputStructTypeData(type);
        mplExport.OutputInterfaceTypeData(type);
    }
}

void OutputTypeConstString(const MIRType &ty, BinaryMplExport &)
{
    DEBUG_ASSERT(false, "Type's kind not yet implemented: %d", ty.GetKind());
    (void)ty;
}

static bool InitOutputTypeFactory()
{
    RegisterFactoryFunction<OutputTypeFactory>(kTypeScalar, OutputTypeScalar);
    RegisterFactoryFunction<OutputTypeFactory>(kTypePointer, OutputTypePointer);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeByName, OutputTypeByName);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeFArray, OutputTypeFArray);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeJArray, OutputTypeJArray);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeArray, OutputTypeArray);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeFunction, OutputTypeFunction);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeParam, OutputTypeParam);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeInstantVector, OutputTypeInstantVector);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeGenericInstant, OutputTypeGenericInstant);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeBitField, OutputTypeBitField);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeStruct, OutputTypeStruct);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeStructIncomplete, OutputTypeStruct);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeUnion, OutputTypeStruct);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeClass, OutputTypeClass);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeClassIncomplete, OutputTypeClass);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeInterface, OutputTypeInterface);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeInterfaceIncomplete, OutputTypeInterface);
    RegisterFactoryFunction<OutputTypeFactory>(kTypeConstString, OutputTypeConstString);
    return true;
}
};  // namespace

namespace maple {
int BinaryMplExport::typeMarkOffset = 0;

BinaryMplExport::BinaryMplExport(MIRModule &md) : mod(md)
{
    bufI = 0;
    Init();
    (void)InitOutputConstFactory();
    (void)InitOutputTypeFactory();
    not2mplt = false;
}

uint8 BinaryMplExport::Read()
{
    CHECK_FATAL(bufI < buf.size(), "Index out of bound in BinaryMplImport::Read()");
    return buf[bufI++];
}

// Little endian
int32 BinaryMplExport::ReadInt()
{
    uint32 x0 = static_cast<uint32>(Read());
    uint32 x1 = static_cast<uint32>(Read());
    uint32 x2 = static_cast<uint32>(Read());
    uint32 x3 = static_cast<uint32>(Read());
    int32 x = static_cast<int32>((((((x3 << 8) + x2) << 8) + x1) << 8) + x0);
    return x;
}

void BinaryMplExport::Write(uint8 b)
{
    buf.push_back(b);
}

// Little endian
void BinaryMplExport::WriteInt(int32 x)
{
    Write(static_cast<uint8>(static_cast<uint32>(x) & 0xFF));
    Write(static_cast<uint8>((static_cast<uint32>(x) >> 8) & 0xFF));  // shift 8 to get 9th-16th bits
    Write(static_cast<uint8>((static_cast<uint32>(x) >> 16) & 0xFF)); // shift 16 to get 17th-24th bits
    Write(static_cast<uint8>((static_cast<uint32>(x) >> 24) & 0xFF)); // shift 24 to get high 8 bits
}

void BinaryMplExport::ExpandFourBuffSize()
{
    WriteInt(0);
}

void BinaryMplExport::Fixup(size_t i, int32 x)
{
    constexpr int fixupCount = 4;
    CHECK(i <= buf.size() - fixupCount, "Index out of bound in BinaryMplImport::Fixup()");
    buf[i + kFirstField] = static_cast<uint8>(static_cast<uint32>(x) & 0xFF);
    buf[i + kSecondField] = static_cast<uint8>((static_cast<uint32>(x) >> 8) & 0xFF);  // shift 8 to get 9th-16th bits
    buf[i + kThirdField] = static_cast<uint8>((static_cast<uint32>(x) >> 16) & 0xFF);  // shift 16 to get 17th-24th bits
    buf[i + kFourthField] = static_cast<uint8>((static_cast<uint32>(x) >> 24) & 0xFF); // shift 24 to get high 8 bits
}

void BinaryMplExport::WriteInt64(int64 x)
{
    WriteInt(static_cast<int32>(static_cast<uint64>(x) & 0xFFFFFFFF));
    WriteInt(static_cast<int32>((static_cast<uint64>(x) >> k32BitSize) & 0xFFFFFFFF));
}

// LEB128
void BinaryMplExport::WriteNum(int64 x)
{
    while (x < -0x40 || x >= 0x40) {
        Write(static_cast<uint8>((static_cast<uint64>(x) & 0x7F) + 0x80));
        x = x >> 7; // LEB128 is a compress algorithm, save 1 bit subsequent falg and 7 bits data in 1 byte.
                    // Do not cast int64 to uint64. If do so, small negtivate number like
                    // -3 will occupy 9 bits and we will not get the compressed benefit.
    }
    Write(static_cast<uint8>(static_cast<uint64>(x) & 0x7F));
}

void BinaryMplExport::WriteAsciiStr(const std::string &str)
{
    WriteNum(static_cast<int64>(str.size()));
    for (size_t i = 0; i < str.size(); ++i) {
        Write(static_cast<uint8>(str[i]));
    }
}

void BinaryMplExport::DumpBuf(const std::string &name)
{
    FILE *f = fopen(name.c_str(), "wb");
    if (f == nullptr) {
        LogInfo::MapleLogger(kLlErr) << "Error while creating the binary file: " << name << '\n';
        FATAL(kLncFatal, "Error while creating the binary file: %s\n", name.c_str());
    }
    size_t size = buf.size();
    size_t k = fwrite(&buf[0], sizeof(uint8), size, f);
    fclose(f);
    if (k != size) {
        LogInfo::MapleLogger(kLlErr) << "Error while writing the binary file: " << name << '\n';
    }
}

void BinaryMplExport::OutputConstBase(const MIRConst &constVal)
{
    WriteNum(constVal.GetKind());
    OutputType(constVal.GetType().GetTypeIndex());
}

void BinaryMplExport::OutputConst(MIRConst *constVal)
{
    if (constVal == nullptr) {
        WriteNum(0);
    } else {
        auto func = CreateProductFunction<OutputConstFactory>(constVal->GetKind());
        if (func != nullptr) {
            func(*constVal, *this);
        }
    }
}

void BinaryMplExport::OutputStr(const GStrIdx &gstr)
{
    if (gstr == 0u) {
        WriteNum(0);
        return;
    }

    auto it = gStrMark.find(gstr);
    if (it != gStrMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    size_t mark = gStrMark.size();
    gStrMark[gstr] = static_cast<int64>(mark);
    WriteNum(kBinString);
    DEBUG_ASSERT(GlobalTables::GetStrTable().StringTableSize() != 0, "Container check");
    WriteAsciiStr(GlobalTables::GetStrTable().GetStringFromStrIdx(gstr));
}

void BinaryMplExport::OutputUsrStr(UStrIdx ustr)
{
    if (ustr == 0u) {
        WriteNum(0);
        return;
    }

    auto it = uStrMark.find(ustr);
    if (it != uStrMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    size_t mark = uStrMark.size();
    uStrMark[ustr] = static_cast<int64>(mark);
    WriteNum(kBinUsrString);
    WriteAsciiStr(GlobalTables::GetUStrTable().GetStringFromStrIdx(ustr));
}

void BinaryMplExport::OutputPragmaElement(const MIRPragmaElement &e)
{
    OutputStr(e.GetNameStrIdx());
    OutputStr(e.GetTypeStrIdx());
    WriteNum(e.GetType());

    if (e.GetType() == kValueString || e.GetType() == kValueType || e.GetType() == kValueField ||
        e.GetType() == kValueMethod || e.GetType() == kValueEnum) {
        OutputStr(GStrIdx(e.GetI32Val()));
    } else {
        WriteInt64(e.GetU64Val());
    }
    size_t size = e.GetSubElemVec().size();
    WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        OutputPragmaElement(*(e.GetSubElement(i)));
    }
}

void BinaryMplExport::OutputPragma(const MIRPragma &p)
{
    WriteNum(p.GetKind());
    WriteNum(p.GetVisibility());
    OutputStr(p.GetStrIdx());
    OutputType(p.GetTyIdx());
    OutputType(p.GetTyIdxEx());
    WriteNum(p.GetParamNum());
    size_t size = p.GetElementVector().size();
    WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        OutputPragmaElement(*(p.GetNthElement(i)));
    }
}

void BinaryMplExport::OutputTypeBase(const MIRType &type)
{
    WriteNum(type.GetPrimType());
    OutputStr(type.GetNameStrIdx());
    WriteNum(type.IsNameIsLocal());
}

void BinaryMplExport::OutputFieldPair(const FieldPair &fp)
{
    OutputStr(fp.first);          // GStrIdx
    OutputType(fp.second.first);  // TyIdx
    FieldAttrs fa = fp.second.second;
    WriteNum(fa.GetAttrFlag());
    WriteNum(fa.GetAlignValue());
    if (fa.GetAttr(FLDATTR_static) && fa.GetAttr(FLDATTR_final) &&
        (fa.GetAttr(FLDATTR_public) || fa.GetAttr(FLDATTR_protected))) {
        const std::string &fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(fp.first);
        MIRSymbol *fieldVar = mod.GetMIRBuilder()->GetGlobalDecl(fieldName);
        if ((fieldVar != nullptr) && (fieldVar->GetKonst() != nullptr) &&
            (fieldVar->GetKonst()->GetKind() == kConstStr16Const)) {
            WriteNum(kBinInitConst);
            OutputConst(fieldVar->GetKonst());
        } else {
            WriteNum(0);
        }
    }
}

void BinaryMplExport::OutputMethodPair(const MethodPair &memPool)
{
    // use GStrIdx instead, StIdx will be created by ImportMethodPair
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(memPool.first.Idx());
    CHECK_FATAL(funcSt != nullptr, "Pointer funcSt is nullptr, can't get symbol! Check it!");
    WriteAsciiStr(GlobalTables::GetStrTable().GetStringFromStrIdx(funcSt->GetNameStrIdx()));
    OutputType(memPool.second.first);               // TyIdx
    WriteNum(memPool.second.second.GetAttrFlag());  // FuncAttrs
}

void BinaryMplExport::OutputFieldsOfStruct(const FieldVector &fields)
{
    WriteNum(fields.size());
    for (const FieldPair &fp : fields) {
        OutputFieldPair(fp);
    }
}

void BinaryMplExport::OutputMethodsOfStruct(const MethodVector &methods)
{
    WriteNum(methods.size());
    for (const MethodPair &memPool : methods) {
        OutputMethodPair(memPool);
    }
}

void BinaryMplExport::OutputStructTypeData(const MIRStructType &type)
{
    OutputFieldsOfStruct(type.GetFields());
    OutputFieldsOfStruct(type.GetStaticFields());
    OutputFieldsOfStruct(type.GetParentFields());
    OutputMethodsOfStruct(type.GetMethods());
}

void BinaryMplExport::OutputImplementedInterfaces(const std::vector<TyIdx> &interfaces)
{
    WriteNum(interfaces.size());
    for (const TyIdx &tyIdx : interfaces) {
        OutputType(tyIdx);
    }
}

void BinaryMplExport::OutputInfoIsString(const std::vector<bool> &infoIsString)
{
    WriteNum(infoIsString.size());
    for (bool isString : infoIsString) {
        WriteNum(static_cast<int64>(isString));
    }
}

void BinaryMplExport::OutputInfo(const std::vector<MIRInfoPair> &info, const std::vector<bool> &infoIsString)
{
    size_t size = info.size();
    WriteNum(size);
    for (size_t i = 0; i < size; ++i) {
        OutputStr(info[i].first);  // GStrIdx
        if (infoIsString[i]) {
            OutputStr(GStrIdx(info[i].second));
        } else {
            WriteNum(info[i].second);
        }
    }
}

void BinaryMplExport::OutputPragmaVec(const std::vector<MIRPragma *> &pragmaVec)
{
    WriteNum(pragmaVec.size());
    for (MIRPragma *pragma : pragmaVec) {
        OutputPragma(*pragma);
    }
}

void BinaryMplExport::OutputClassTypeData(const MIRClassType &type)
{
    OutputType(type.GetParentTyIdx());
    OutputImplementedInterfaces(type.GetInterfaceImplemented());
    OutputInfoIsString(type.GetInfoIsString());
    if (!inIPA) {
        OutputInfo(type.GetInfo(), type.GetInfoIsString());
        OutputPragmaVec(type.GetPragmaVec());
    }
}

void BinaryMplExport::OutputInterfaceTypeData(const MIRInterfaceType &type)
{
    OutputImplementedInterfaces(type.GetParentsTyIdx());
    OutputInfoIsString(type.GetInfoIsString());
    if (!inIPA) {
        OutputInfo(type.GetInfo(), type.GetInfoIsString());
        OutputPragmaVec(type.GetPragmaVec());
    }
}

void BinaryMplExport::Init()
{
    BinaryMplExport::typeMarkOffset = 0;
    gStrMark.clear();
    uStrMark.clear();
    symMark.clear();
    funcMark.clear();
    typMark.clear();
    gStrMark[GStrIdx(0)] = 0;
    uStrMark[UStrIdx(0)] = 0;
    symMark[nullptr] = 0;
    funcMark[nullptr] = 0;
    eaNodeMark[nullptr] = 0;
    curFunc = nullptr;
    for (uint32 pti = static_cast<uint32>(PTY_begin); pti < static_cast<uint32>(PTY_end); ++pti) {
        typMark[GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(pti))] = pti;
    }
}

void BinaryMplExport::OutputSymbol(MIRSymbol *sym)
{
    if (sym == nullptr) {
        WriteNum(0);
        return;
    }

    std::unordered_map<const MIRSymbol *, int64>::iterator it = symMark.find(sym);
    if (it != symMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinSymbol);
    WriteNum(sym->GetScopeIdx());
    OutputStr(sym->GetNameStrIdx());
    OutputUsrStr(sym->sectionAttr);
    OutputUsrStr(sym->GetAsmAttr());
    WriteNum(sym->GetSKind());
    WriteNum(sym->GetStorageClass());
    size_t mark = symMark.size();
    symMark[sym] = static_cast<int64>(mark);
    OutputTypeAttrs(sym->GetAttrs());
    WriteNum(sym->GetIsTmp() ? 1 : 0);
    if (sym->GetSKind() == kStPreg) {
        WriteNum(sym->GetPreg()->GetPregNo());
    } else if (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) {
        if (sym->GetKonst() != nullptr) {
            sym->GetKonst()->SetType(*sym->GetType());
        }
        OutputConst(sym->GetKonst());
    } else if (sym->GetSKind() == kStFunc) {
        OutputFunction(sym->GetFunction()->GetPuidx());
    } else {
        CHECK_FATAL(false, "should not used");
    }
    if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
        OutputSrcPos(sym->GetSrcPosition());
    }
    OutputType(sym->GetTyIdx());
}

void BinaryMplExport::OutputFunction(PUIdx puIdx)
{
    if (puIdx == 0) {
        WriteNum(0);
        mod.SetCurFunction(nullptr);
        return;
    }
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    CHECK_FATAL(func != nullptr, "Cannot get MIRFunction.");
    auto it = funcMark.find(func);
    if (it != funcMark.end()) {
        WriteNum(-it->second);
        mod.SetCurFunction(func);
        return;
    }
    size_t mark = funcMark.size();
    funcMark[func] = static_cast<int64>(mark);
    MIRFunction *savedFunc = mod.CurFunction();
    mod.SetCurFunction(func);

    WriteNum(kBinFunction);
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
    CHECK_FATAL(funcSt != nullptr, "Pointer funcSt is nullptr, cannot get symbol! Check it!");
    OutputSymbol(funcSt);
    OutputType(func->GetMIRFuncType()->GetTypeIndex());
    WriteNum(func->GetFuncAttrs().GetAttrFlag());

    auto &attributes = func->GetFuncAttrs();
    if (attributes.GetAttr(FUNCATTR_constructor_priority)) {
        WriteNum(attributes.GetConstructorPriority());
    }

    if (attributes.GetAttr(FUNCATTR_destructor_priority)) {
        WriteNum(attributes.GetDestructorPriority());
    }

    WriteNum(func->GetFlag());
    OutputType(func->GetClassTyIdx());
    // output formal parameter information
    WriteNum(static_cast<int64>(func->GetFormalDefVec().size()));
    for (FormalDef formalDef : func->GetFormalDefVec()) {
        OutputStr(formalDef.formalStrIdx);
        OutputType(formalDef.formalTyIdx);
        WriteNum(static_cast<int64>(formalDef.formalAttrs.GetAttrFlag()));
    }
    //  store Side Effect for each func
    if (func2SEMap) {
        uint32 isSee = func->IsIpaSeen() == true ? 1 : 0;
        uint32 isPure = func->IsPure() == true ? 1 : 0;
        uint32 noDefArg = func->IsNoDefArgEffect() == true ? 1 : 0;
        uint32 noDef = func->IsNoDefEffect() == true ? 1 : 0;
        uint32 noRetGlobal = func->IsNoRetGlobal() == true ? 1 : 0;
        uint32 noThr = func->IsNoThrowException() == true ? 1 : 0;
        uint32 noRetArg = func->IsNoRetArg() == true ? 1 : 0;
        uint32 noPriDef = func->IsNoPrivateDefEffect() == true ? 1 : 0;
        uint32 i = 0;
        uint8 se = noThr << i++;
        se |= noRetGlobal << i++;
        se |= noDef << i++;
        se |= noDefArg << i++;
        se |= isPure << i++;
        se |= isSee << i++;
        se |= noRetArg << i++;
        se |= noPriDef << i;
        if ((*func2SEMap).find(func->GetNameStrIdx()) == (*func2SEMap).end()) {
            (*func2SEMap)[func->GetNameStrIdx()] = se;
        } else if ((*func2SEMap)[func->GetNameStrIdx()] != se) {
            FATAL(kLncFatal, "It is a bug.");
        }
    }
    mod.SetCurFunction(savedFunc);
}

void BinaryMplExport::WriteStrField(uint64 contentIdx)
{
    Fixup(contentIdx, buf.size());
    WriteNum(kBinStrStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_STR_START
    size_t outStrSizeIdx = buf.size();
    ExpandFourBuffSize();  // size of OutputStr

    int32 size = 0;
    for (const auto &entity : GlobalTables::GetConstPool().GetConstU16StringPool()) {
        MIRSymbol *sym = entity.second;
        if (sym->IsLiteral()) {
            OutputStr(sym->GetNameStrIdx());
            ++size;
        }
    }
    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    Fixup(outStrSizeIdx, size);
    WriteNum(~kBinStrStart);
}

void BinaryMplExport::WriteHeaderField(uint64 contentIdx)
{
    Fixup(contentIdx, buf.size());
    WriteNum(kBinHeaderStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_IMPORT_START
    WriteNum(mod.GetFlavor());
    WriteNum(mod.GetSrcLang());
    WriteNum(mod.GetID());
    if (mod.GetFlavor() == kFlavorLmbc) {
        WriteNum(mod.GetGlobalMemSize());
        WriteNum(mod.IsWithDbgInfo());
    }
    WriteNum(mod.GetNumFuncs());
    WriteAsciiStr(mod.GetEntryFuncName());
    OutputInfoVector(mod.GetFileInfo(), mod.GetFileInfoIsString());

    if (mod.IsWithDbgInfo()) {
        WriteNum(static_cast<int64>(mod.GetSrcFileInfo().size()));
        for (uint32 i = 0; i < mod.GetSrcFileInfo().size(); i++) {
            OutputStr(mod.GetSrcFileInfo()[i].first);
            WriteNum(mod.GetSrcFileInfo()[i].second);
        }
    } else {
        Write(0);
    }

    WriteNum(static_cast<int64>(mod.GetImportFiles().size()));
    for (GStrIdx strIdx : mod.GetImportFiles()) {
        OutputStr(strIdx);
    }

    WriteNum(static_cast<int64>(mod.GetAsmDecls().size()));
    for (MapleString mapleStr : mod.GetAsmDecls()) {
        std::string str(mapleStr.c_str());
        WriteAsciiStr(str);
    }

    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    WriteNum(~kBinHeaderStart);
    return;
}

void BinaryMplExport::WriteTypeField(uint64 contentIdx, bool useClassList)
{
    Fixup(contentIdx, buf.size());
    WriteNum(kBinTypeStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_TYPE_START
    size_t outTypeSizeIdx = buf.size();
    ExpandFourBuffSize();  // size of OutputType
    int32 size = 0;
    if (useClassList) {
        for (uint32 tyIdx : mod.GetClassList()) {
            TyIdx curTyidx(tyIdx);
            MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(curTyidx);
            CHECK_FATAL(type != nullptr, "Pointer type is nullptr, cannot get type, check it!");
            if (type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
                auto *structType = static_cast<MIRStructType *>(type);
                // skip imported class/interface and incomplete types
                if (!structType->IsImported() && !structType->IsIncomplete()) {
                    OutputType(curTyidx);
                    ++size;
                }
            }
        }
    } else {
        uint32 idx = GlobalTables::GetTypeTable().lastDefaultTyIdx.GetIdx();
        for (idx = idx + 1; idx < GlobalTables::GetTypeTable().GetTypeTableSize(); idx++) {
            OutputType(TyIdx(idx));
            size++;
        }
    }
    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    Fixup(outTypeSizeIdx, size);
    WriteNum(~kBinTypeStart);
}

void BinaryMplExport::OutputCallInfo(CallInfo &callInfo)
{
    auto it = callInfoMark.find(callInfo.GetID());
    if (it != callInfoMark.end()) {
        WriteNum(-(it->second));
        return;
    }
    WriteNum(kBinCallinfo);
    size_t mark = callInfoMark.size();
    callInfoMark[callInfo.GetID()] = static_cast<int64>(mark);
    WriteNum(callInfo.GetCallType());  // call type
    WriteInt(callInfo.GetLoopDepth());
    WriteInt(callInfo.GetID());
    callInfo.AreAllArgsLocal() ? Write(1) : Write(0);  // All args are local variables or not.
    OutputSymbol(callInfo.GetFunc()->GetFuncSymbol());
}

void BinaryMplExport::WriteCgField(uint64 contentIdx, const CallGraph *cg)
{
    if (contentIdx != 0) {
        Fixup(contentIdx, buf.size());
    }
    WriteNum(kBinCgStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_CG_START
    size_t outcgSizeIdx = buf.size();
    ExpandFourBuffSize();  // size of OutCG
    int32 size = 0;
    if (cg != nullptr) {
        for (auto entry : cg->GetNodesMap()) {
            MIRSymbol *methodSym = entry.first->GetFuncSymbol();
            WriteNum(kStartMethod);
            OutputSymbol(methodSym);
            size_t targetTyIdx = buf.size();
            ExpandFourBuffSize();
            int32 targSize = 0;
            callInfoMark.clear();
            callInfoMark[0xffffffff] = 0;
            for (const auto &callSite : entry.second->GetCallee()) {
                OutputCallInfo(*(callSite.first));
                ++targSize;
            }
            Fixup(targetTyIdx, targSize);
            WriteNum(~kStartMethod);
            ++size;
        }
    }

    DEBUG_ASSERT((buf.size() - totalSizeIdx) <= 0xffffffff, "Integer overflow.");
    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    Fixup(outcgSizeIdx, size);
    WriteNum(~kBinCgStart);
}

void BinaryMplExport::WriteSeField()
{
    DEBUG_ASSERT(func2SEMap != nullptr, "Expecting a func2SE map");
    WriteNum(kBinSeStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_SYM_START
    size_t outseSizeIdx = buf.size();
    ExpandFourBuffSize();  // size of OutSym
    int32 size = 0;

    for (const auto &func2SE : *func2SEMap) {
        uint8 se = func2SE.second;
        if (static_cast<int32>(se)) {
            OutputStr(func2SE.first);
            Write(se);
            if ((se & kPureFunc) == kPureFunc) {
                const std::string &funcStr = GlobalTables::GetStrTable().GetStringFromStrIdx(func2SE.first);
                auto *funcSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
                    GlobalTables::GetStrTable().GetStrIdxFromName(funcStr));
                MIRFunction *func = (funcSymbol != nullptr)
                                        ? GetMIRModule().GetMIRBuilder()->GetFunctionFromSymbol(*funcSymbol)
                                        : nullptr;
                OutputType(func->GetReturnTyIdx());
            }
            ++size;
        }
    }
    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    Fixup(outseSizeIdx, size);
    WriteNum(~kBinSeStart);
}

void BinaryMplExport::OutEaCgBaseNode(const EACGBaseNode &node, bool firstPart)
{
    if (firstPart) {
        WriteNum(node.eaStatus);
        WriteInt(static_cast<int32>(node.id));
    } else {
        // in and out set in base node is not necessary to be outed
        // start to out point-to set
        size_t outP2SizeIdx = buf.size();
        WriteInt(0);
        uint32 size = 0;
        for (EACGBaseNode *outNode : node.GetPointsToSet()) {
            OutEaCgNode(*outNode);
            ++size;
        }
        Fixup(outP2SizeIdx, size);
        // start to out in set
        outP2SizeIdx = buf.size();
        WriteInt(0);
        size = 0;
        for (EACGBaseNode *outNode : node.GetInSet()) {
            OutEaCgNode(*outNode);
            ++size;
        }
        Fixup(outP2SizeIdx, size);
        // start to out out set
        outP2SizeIdx = buf.size();
        WriteInt(0);
        size = 0;
        for (EACGBaseNode *outNode : node.GetOutSet()) {
            OutEaCgNode(*outNode);
            ++size;
        }
        Fixup(outP2SizeIdx, size);
    }
}

void BinaryMplExport::OutEaCgObjNode(EACGObjectNode &obj)
{
    Write(uint8(obj.isPhantom));
    size_t outFieldSizeIdx = buf.size();
    WriteInt(0);
    uint32 size = 0;
    for (const auto &fieldNodePair : obj.fieldNodes) {
        EACGBaseNode *fieldNode = fieldNodePair.second;
        DEBUG_ASSERT(fieldNodePair.first == static_cast<EACGFieldNode *>(fieldNode)->GetFieldID(), "Must be.");
        OutEaCgNode(*fieldNode);
        ++size;
    }
    Fixup(outFieldSizeIdx, size);
    // start to out point by
    outFieldSizeIdx = buf.size();
    WriteInt(0);
    size = 0;
    for (EACGBaseNode *node : obj.pointsBy) {
        OutEaCgNode(*node);
        ++size;
    }
    Fixup(outFieldSizeIdx, size);
}

void BinaryMplExport::OutEaCgRefNode(const EACGRefNode &ref)
{
    Write(uint8(ref.isStaticField));
}

void BinaryMplExport::OutEaCgFieldNode(EACGFieldNode &field)
{
    WriteInt(field.GetFieldID());
    int32 size = 0;
    size_t outFieldSizeIdx = buf.size();
    WriteInt(0);
    for (EACGBaseNode *obj : field.belongsTo) {
        OutEaCgNode(*obj);
        ++size;
    }
    Fixup(outFieldSizeIdx, size);
    Write(uint8(field.isPhantom));
}

void BinaryMplExport::OutEaCgActNode(const EACGActualNode &act)
{
    Write(uint8(act.isPhantom));
    Write(uint8(act.isReturn));
    Write(act.argIdx);
    WriteInt(act.callSiteInfo);
}

void BinaryMplExport::OutEaCgNode(EACGBaseNode &node)
{
    auto it = eaNodeMark.find(&node);
    if (it != eaNodeMark.end()) {
        WriteNum(-it->second);
        return;
    }
    size_t mark = eaNodeMark.size();
    eaNodeMark[&node] = static_cast<int64>(mark);
    WriteNum(kBinEaCgNode);
    WriteNum(node.kind);
    OutEaCgBaseNode(node, true);
    if (node.IsActualNode()) {
        WriteNum(kBinEaCgActNode);
        OutEaCgActNode(static_cast<EACGActualNode &>(node));
    } else if (node.IsFieldNode()) {
        WriteNum(kBinEaCgFieldNode);
        OutEaCgFieldNode(static_cast<EACGFieldNode &>(node));
    } else if (node.IsObjectNode()) {
        WriteNum(kBinEaCgObjNode);
        OutEaCgObjNode(static_cast<EACGObjectNode &>(node));
    } else if (node.IsReferenceNode()) {
        WriteNum(kBinEaCgRefNode);
        OutEaCgRefNode(static_cast<EACGRefNode &>(node));
    } else {
        DEBUG_ASSERT(false, "Must be.");
    }
    OutEaCgBaseNode(node, false);
    WriteNum(~kBinEaCgNode);
}

void BinaryMplExport::WriteEaField(const CallGraph &cg)
{
    WriteNum(kBinEaStart);
    uint64 totalSizeIdx = buf.size();
    WriteInt(0);
    uint64 outeaSizeIdx = buf.size();
    WriteInt(0);
    int32 size = 0;
    for (auto cgNodePair : cg.GetNodesMap()) {
        MIRFunction *func = cgNodePair.first;
        if (func->GetEACG() == nullptr) {
            continue;
        }
        EAConnectionGraph *eacg = func->GetEACG();
        DEBUG_ASSERT(eacg != nullptr, "Must be.");
        OutputStr(eacg->GetFuncNameStrIdx());
        WriteInt(eacg->GetNodes().size());
        OutEaCgNode(*eacg->GetGlobalObject());
        uint64 outFunceaIdx = buf.size();
        WriteInt(0);
        size_t funceaSize = 0;
        for (EACGBaseNode *node : eacg->GetFuncArgNodes()) {
            OutEaCgNode(*node);
            ++funceaSize;
        }
        Fixup(outFunceaIdx, funceaSize);
        ++size;
    }
    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    Fixup(outeaSizeIdx, size);
    WriteNum(~kBinEaStart);
}

void BinaryMplExport::WriteEaCgField(EAConnectionGraph *eaCg)
{
    if (eaCg == nullptr) {
        WriteNum(~kBinEaCgStart);
        return;
    }
    WriteNum(kBinEaCgStart);
    size_t totalSizeIdx = buf.size();
    WriteInt(0);
    // out this function's arg list
    OutputStr(eaCg->GetFuncNameStrIdx());
    WriteInt(eaCg->GetNodes().size());
    OutEaCgNode(*eaCg->GetGlobalObject());
    size_t outNodeSizeIdx = buf.size();
    WriteInt(0);
    size_t argNodeSize = 0;
    for (EACGBaseNode *node : eaCg->GetFuncArgNodes()) {
        OutEaCgNode(*node);
        ++argNodeSize;
    }
    Fixup(outNodeSizeIdx, argNodeSize);
    // out this function's call site's arg list
    outNodeSizeIdx = buf.size();
    WriteInt(0);
    size_t callSiteSize = 0;
    for (auto nodePair : eaCg->GetCallSite2Nodes()) {
        uint32 id = nodePair.first;
        MapleVector<EACGBaseNode *> *calleeArgNode = nodePair.second;
        WriteInt(id);
        size_t outCalleeArgSizeIdx = buf.size();
        WriteInt(0);
        size_t calleeArgSize = 0;
        for (EACGBaseNode *node : *calleeArgNode) {
            OutEaCgNode(*node);
            ++calleeArgSize;
        }
        Fixup(outCalleeArgSizeIdx, calleeArgSize);
        ++callSiteSize;
    }
    Fixup(outNodeSizeIdx, callSiteSize);

    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    WriteNum(~kBinEaCgStart);
}

void BinaryMplExport::WriteSymField(uint64 contentIdx)
{
    Fixup(contentIdx, buf.size());
    WriteNum(kBinSymStart);
    uint64 totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_SYM_START
    uint64 outsymSizeIdx = buf.size();
    ExpandFourBuffSize();  // size of OutSym
    int32 size = 0;

    if (not2mplt) {
        for (auto sit = GetMIRModule().GetSymbolDefOrder().begin(); sit != GetMIRModule().GetSymbolDefOrder().end();
             ++sit) {
            MIRSymbol *s = GlobalTables::GetGsymTable().GetSymbolFromStidx(sit->Idx());
            DEBUG_ASSERT(s != nullptr, "null ptr check");
            // Verify: all wpofake variables should have been deleted from globaltable
            DEBUG_ASSERT(!(s->IsWpoFakeParm() || s->IsWpoFakeRet()) || s->IsDeleted(), "wpofake var not deleted");
            MIRStorageClass storageClass = s->GetStorageClass();
            MIRSymKind sKind = s->GetSKind();
	        if (s->IsDeleted() || storageClass == kScUnused || (s->GetIsImported() && !s->GetAppearsInCode()) ||
                (sKind == kStFunc && (storageClass == kScExtern || !s->GetAppearsInCode()))) {
                continue;
            }
            OutputSymbol(s);
            size++;
        }
    }
    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    Fixup(outsymSizeIdx, size);
    WriteNum(~kBinSymStart);
    return;
}

void BinaryMplExport::WriteContentField4mplt(int fieldNum, uint64 *fieldStartP)
{
    WriteNum(kBinContentStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_SYM_START

    WriteInt(fieldNum);  // size of Content item

    WriteNum(kBinStrStart);
    fieldStartP[kFirstField] = buf.size();
    ExpandFourBuffSize();

    WriteNum(kBinTypeStart);
    fieldStartP[kSecondField] = buf.size();
    ExpandFourBuffSize();

    WriteNum(kBinCgStart);
    fieldStartP[kThirdField] = buf.size();
    ExpandFourBuffSize();

    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    WriteNum(~kBinContentStart);
}

void BinaryMplExport::WriteContentField4nonmplt(int fieldNum, uint64 *fieldStartP)
{
    CHECK_FATAL(fieldStartP != nullptr, "fieldStartP is null.");
    WriteNum(kBinContentStart);
    size_t totalSizeIdx = buf.size();
    ExpandFourBuffSize();  // total size of this field to ~BIN_SYM_START

    WriteInt(fieldNum);  // size of Content item

    WriteNum(kBinHeaderStart);
    fieldStartP[kFirstField] = buf.size();
    ExpandFourBuffSize();

    WriteNum(kBinStrStart);
    fieldStartP[kSecondField] = buf.size();
    ExpandFourBuffSize();

    WriteNum(kBinTypeStart);
    fieldStartP[kThirdField] = buf.size();
    ExpandFourBuffSize();

    WriteNum(kBinSymStart);
    fieldStartP[kFourthField] = buf.size();
    ExpandFourBuffSize();

    WriteNum(kBinFunctionBodyStart);
    fieldStartP[kFifthField] = buf.size();
    ExpandFourBuffSize();

    Fixup(totalSizeIdx, buf.size() - totalSizeIdx);
    WriteNum(~kBinContentStart);
}

void BinaryMplExport::Export(const std::string &fname, std::unordered_set<std::string> *dumpFuncSet)
{
    uint64 fieldStartPoint[5];
    if (!not2mplt) {
        WriteInt(kMpltMagicNumber);
        WriteContentField4mplt(kFourthFieldInt, fieldStartPoint);
        WriteStrField(fieldStartPoint[kFirstField]);
        WriteTypeField(fieldStartPoint[kSecondField]);
        WriteCgField(fieldStartPoint[kThirdField], nullptr);
        importFileName = fname;
    } else {
        WriteInt(kMpltMagicNumber + 0x10);
        WriteContentField4nonmplt(kSixthFieldInt, fieldStartPoint);
        WriteHeaderField(fieldStartPoint[kFirstField]);
        WriteSymField(fieldStartPoint[kFourthField]);
        WriteFunctionBodyField(fieldStartPoint[kFifthField], dumpFuncSet);
    }
    WriteNum(kBinFinish);
    DumpBuf(fname);
}

void BinaryMplExport::AppendAt(const std::string &name, int32 offset)
{
    FILE *f = fopen(name.c_str(), "r+b");
    if (f == nullptr) {
        LogInfo::MapleLogger(kLlErr) << "Error while opening the binary file: " << name << '\n';
        FATAL(kLncFatal, "Error while creating the binary file: %s\n", name.c_str());
    }
    int seekRet = fseek(f, static_cast<long int>(offset), SEEK_SET);
    CHECK_FATAL(seekRet == 0, "Call fseek failed.");
    size_t size = buf.size();
    size_t k = fwrite(&buf[0], sizeof(uint8), size, f);
    fclose(f);
    if (k != size) {
        LogInfo::MapleLogger(kLlErr) << "Error while writing the binary file: " << name << '\n';
    }
}

void BinaryMplExport::OutputTypePairs(const MIRInstantVectorType &type)
{
    size_t size = type.GetInstantVec().size();
    WriteNum(size);
    for (const TypePair &typePair : type.GetInstantVec()) {
        OutputType(typePair.first);
        OutputType(typePair.second);
    }
}

void BinaryMplExport::OutputTypeAttrs(const TypeAttrs &ta)
{
    WriteNum(ta.GetAttrFlag());
    WriteNum(ta.GetAlignValue());
    WriteNum(ta.GetPack());
}

void BinaryMplExport::OutputType(TyIdx tyIdx)
{
    if (tyIdx == 0u) {
        WriteNum(0);
        return;
    }
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    CHECK_FATAL(ty != nullptr, "If gets nulltype, should have been returned!");
    auto it = typMark.find(ty);
    if (it != typMark.end()) {
        if (ty->GetKind() != kTypeFunction) {
            WriteNum(-(it->second));
            return;
        }
        ++BinaryMplExport::typeMarkOffset;
    } else {
        size_t mark = typMark.size() + static_cast<size_t>(BinaryMplExport::typeMarkOffset);
        typMark[ty] = static_cast<int64>(mark);
    }

    auto func = CreateProductFunction<OutputTypeFactory>(ty->GetKind());
    if (func != nullptr) {
        func(*ty, *this);
    } else {
        DEBUG_ASSERT(false, "Type's kind not yet implemented: %d", ty->GetKind());
    }
}

void UpdateMplt::UpdateCgField(BinaryMplt &binMplt, const CallGraph &cg)
{
    BinaryMplImport &binImport = binMplt.GetBinImport();
    BinaryMplExport &binExport = binMplt.GetBinExport();
    binImport.SetBufI(0);
    if (binImport.IsBufEmpty() || binImport.ReadInt() != kMpltMagicNumber) {
        INFO(kLncInfo, " This Module depends on nothing");
        return;
    }
    int64 cgStart = binImport.GetContent(kBinCgStart);
    DEBUG_ASSERT(cgStart != 0, "Should be updated in import processing.");
    binImport.SetBufI(cgStart);
    [[maybe_unused]] int64 checkReadNum = binImport.ReadNum();
    DEBUG_ASSERT(checkReadNum == kBinCgStart, "Should be cg start point.");
    int32 totalSize = binImport.ReadInt();
    constexpr int32 headLen = 4;
    binImport.SetBufI(binImport.GetBufI() + totalSize - headLen);
    checkReadNum = binImport.ReadNum();
    DEBUG_ASSERT(checkReadNum == ~kBinCgStart, "Should be end of cg.");
    binExport.Init();
    std::map<GStrIdx, uint8> tmp;
    binExport.func2SEMap = &tmp;
    binExport.inIPA = true;
    binExport.WriteCgField(0, &cg);
    binExport.Init();
    binExport.WriteSeField();
    binExport.eaNodeMark.clear();
    binExport.eaNodeMark[nullptr] = 0;
    binExport.gStrMark.clear();
    binExport.gStrMark[GStrIdx(0)] = 0;
    binExport.WriteEaField(cg);
    binExport.WriteNum(kBinFinish);
    std::string filename(binMplt.GetImportFileName());
    binExport.AppendAt(filename, cgStart);
}

}  // namespace maple
