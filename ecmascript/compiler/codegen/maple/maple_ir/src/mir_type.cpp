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

#include "mir_type.h"
#include "mir_symbol.h"
#include "printing.h"
#include "namemangler.h"
#include "global_tables.h"
#include "mir_builder.h"
#include "cfg_primitive_types.h"
#include "string_utils.h"
#include "triple.h"
#if MIR_FEATURE_FULL

namespace maple {
#define LOAD_PRIMARY_TYPE_PROPERTY
#include "prim_types.def"

#define LOAD_ALGO_PRIMARY_TYPE
const PrimitiveTypeProperty &GetPrimitiveTypeProperty(PrimType pType)
{
    switch (pType) {
        case PTY_begin:
            return PTProperty_begin;
#define PRIMTYPE(P) \
    case PTY_##P:   \
        return PTProperty_##P;
#include "prim_types.def"
#undef PRIMTYPE
        case PTY_end:
        default:
            return PTProperty_end;
    }
}

PrimType GetRegPrimType(PrimType primType)
{
    switch (primType) {
        case PTY_i8:
        case PTY_i16:
            return PTY_i32;
        case PTY_u1:
        case PTY_u8:
        case PTY_u16:
            return PTY_u32;
        default:
            return primType;
    }
}

PrimType GetIntegerPrimTypeBySizeAndSign(size_t sizeBit, bool isSign)
{
    switch (sizeBit) {
        case k1BitSize: {
            if (isSign) {
                return PTY_begin;  // There is no 'i1' type
            }
            return PTY_u1;
        }
        case k8BitSize: {
            return isSign ? PTY_i8 : PTY_u8;
        }
        case k16BitSize: {
            return isSign ? PTY_i16 : PTY_u16;
        }
        case k32BitSize: {
            return isSign ? PTY_i32 : PTY_u32;
        }
        case k64BitSize: {
            return isSign ? PTY_i64 : PTY_u64;
        }
        default: {
            return PTY_begin;  // Invalid integer type
        }
    }
}

bool IsNoCvtNeeded(PrimType toType, PrimType fromType)
{
    if (toType == fromType) {
        return true;
    }
    switch (toType) {
        case PTY_i32:
            return fromType == PTY_i16 || fromType == PTY_i8;
        case PTY_u32:
            return fromType == PTY_u16 || fromType == PTY_u8;
        case PTY_u1:
        case PTY_u8:
        case PTY_u16:
            return fromType == PTY_u32;
        case PTY_i8:
        case PTY_i16:
            return fromType == PTY_i32;
        case PTY_i64:
        case PTY_u64:
        case PTY_a64:
        case PTY_ptr:
            return fromType == PTY_ptr || fromType == PTY_u64 || fromType == PTY_a64 || fromType == PTY_i64;
        default:
            return false;
    }
}

bool NeedCvtOrRetype(PrimType origin, PrimType compared)
{
    return GetPrimTypeSize(origin) != GetPrimTypeSize(compared) || IsSignedInteger(origin) != IsSignedInteger(compared);
}

uint8 GetPointerSize()
{
#if TARGX86 || TARGARM32 || TARGVM
    return k4ByteSize;
#elif TARGX86_64 || TARGAARCH64
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        return k8ByteSize;
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        uint8 size = (Triple::GetTriple().GetEnvironment() == Triple::GNUILP32) ? k4ByteSize : k8ByteSize;
        return size;
    } else {
        CHECK_FATAL(false, "Unsupported target");
    }
#else
#error "Unsupported target"
#endif
}

uint8 GetP2Size()
{
#if TARGX86 || TARGARM32 || TARGVM
    return k2ByteSize;
#elif TARGX86_64 || TARGAARCH64
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        return k3ByteSize;
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        uint8 size = (Triple::GetTriple().GetEnvironment() == Triple::GNUILP32) ? k2ByteSize : k3ByteSize;
        return size;
    } else {
        CHECK_FATAL(false, "Unsupported target");
    }
#else
#error "Unsupported target"
#endif
}

PrimType GetLoweredPtrType()
{
#if TARGX86 || TARGARM32 || TARGVM
    return PTY_a32;
#elif TARGX86_64 || TARGAARCH64
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        return PTY_a64;
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        auto pty = (Triple::GetTriple().GetEnvironment() == Triple::GNUILP32) ? PTY_a32 : PTY_a64;
        return pty;
    } else {
        CHECK_FATAL(false, "Unsupported target");
    }
#else
#error "Unsupported target"
#endif
}

PrimType GetExactPtrPrimType()
{
    return (GetPointerSize() == k8ByteSize) ? PTY_a64 : PTY_a32;
}

// answer in bytes; 0 if unknown
uint32 GetPrimTypeSize(PrimType primType)
{
    switch (primType) {
        case PTY_void:
        case PTY_agg:
            return k0BitSize;
        case PTY_ptr:
        case PTY_ref:
            return GetPointerSize();
        case PTY_u1:
        case PTY_i8:
        case PTY_u8:
            return k1BitSize;
        case PTY_i16:
        case PTY_u16:
            return k2BitSize;
        case PTY_a32:
        case PTY_f32:
        case PTY_i32:
        case PTY_u32:
            return k4BitSize;
        case PTY_a64:
        case PTY_f64:
        case PTY_i64:
        case PTY_u64:
            return k8BitSize;
        case PTY_f128:
            return k16BitSize;
        default:
            return k0BitSize;
    }
}

// return the signed version that has the same size
PrimType GetSignedPrimType(PrimType pty)
{
    switch (pty) {
        case PTY_ptr:
        case PTY_ref:
        case PTY_a64:
        case PTY_u64:
            return PTY_i64;
        case PTY_u8:
            return PTY_i8;
        case PTY_u16:
            return PTY_i16;
        case PTY_a32:
        case PTY_u32:
            return PTY_i32;
        default:;
    }
    return pty;
}

// return the unsigned version that has the same size
PrimType GetUnsignedPrimType(PrimType pty)
{
    switch (pty) {
        case PTY_i64:
            return PTY_u64;
        case PTY_i8:
            return PTY_u8;
        case PTY_i16:
            return PTY_u16;
        case PTY_i32:
            return PTY_u32;
        default:;
    }
    return pty;
}

const char *GetPrimTypeName(PrimType primType)
{
#define LOAD_ALGO_PRIMARY_TYPE
    switch (primType) {
        case kPtyInvalid:
            return "kPtyInvalid";
#define PRIMTYPE(P) \
    case PTY_##P:   \
        return #P;
#include "prim_types.def"
#undef PRIMTYPE
        case kPtyDerived:
            return "derived";  // just for test: no primitive type for derived
        default:
            return "kPtyInvalid";
    }
    // SIMD types to be defined
}

void StmtAttrs::DumpAttributes() const
{
#define STMT_ATTR
#define STRING(s) #s
#define ATTR(AT)                \
    if (GetAttr(STMTATTR_##AT)) \
        LogInfo::MapleLogger() << STRING(AT);
#include "all_attributes.def"
#undef ATTR
#undef STMT_ATTR
}

void TypeAttrs::DumpAttributes() const
{
// dump attr without content
#define TYPE_ATTR
#define NOCONTENT_ATTR
#define STRING(s) #s
#define ATTR(AT)            \
    if (GetAttr(ATTR_##AT)) \
        LogInfo::MapleLogger() << " " << STRING(AT);
#include "all_attributes.def"
#undef ATTR
#undef NOCONTENT_ATTR
#undef TYPE_ATTR
    // dump attr with content
    if (attrAlign) {
        LogInfo::MapleLogger() << " align(" << GetAlign() << ")";
    }
    if (GetAttr(ATTR_pack) && GetPack() != 0) {
        LogInfo::MapleLogger() << " pack(" << GetPack() << ")";
    }
}

void FieldAttrs::DumpAttributes() const
{
// dump attr without content
#define FIELD_ATTR
#define NOCONTENT_ATTR
#define STRING(s) #s
#define ATTR(AT)               \
    if (GetAttr(FLDATTR_##AT)) \
        LogInfo::MapleLogger() << " " << STRING(AT);
#include "all_attributes.def"
#undef ATTR
#undef NOCONTENT_ATTR
#undef FIELD_ATTR
    // dump attr with content
    if (attrAlign) {
        LogInfo::MapleLogger() << " align(" << GetAlign() << ")";
    }
    if (IsPacked()) {
        LogInfo::MapleLogger() << " pack(1)";
    }
}

const std::string &MIRType::GetName() const
{
    return GlobalTables::GetStrTable().GetStringFromStrIdx(nameStrIdx);
}

bool MIRType::ValidateClassOrInterface(const std::string &className, bool noWarning) const
{
    if (primType == maple::PTY_agg && (typeKind == maple::kTypeClass || typeKind == maple::kTypeInterface) &&
        nameStrIdx != 0u) {
        return true;
    }
    if (!noWarning) {
        size_t len = className.size();
        constexpr int minClassNameLen = 4;
        constexpr char suffix[] = "_3B";
        size_t suffixLen = std::strlen(suffix);
        if (len > minClassNameLen && strncmp(className.c_str() + len - suffixLen, suffix, suffixLen) == 0) {
            LogInfo::MapleLogger(kLlErr) << "error: missing proper mplt file for " << className << '\n';
        } else {
            LogInfo::MapleLogger(kLlErr) << "internal error: type is not j class or interface " << className << '\n';
        }
    }
    return false;
}

bool MIRType::PointsToConstString() const
{
    return (typeKind == kTypePointer) ? static_cast<const MIRPtrType *>(this)->PointsToConstString() : false;
}

std::string MIRType::GetMplTypeName() const
{
    if (typeKind == kTypeScalar) {
        return GetPrimTypeName(primType);
    }
    return "";
}

std::string MIRType::GetCompactMplTypeName() const
{
    return "";
}

#ifdef ARK_LITECG_DEBUG
void MIRType::Dump(int indent, bool dontUseName) const
{
    LogInfo::MapleLogger() << GetPrimTypeName(primType);
}

void MIRType::DumpAsCxx(int indent) const
{
    switch (primType) {
        case PTY_void:
            LogInfo::MapleLogger() << "void";
            break;
        case PTY_i8:
            LogInfo::MapleLogger() << "int8 ";
            break;
        case PTY_i16:
            LogInfo::MapleLogger() << "int16";
            break;
        case PTY_i32:
            LogInfo::MapleLogger() << "int32";
            break;
        case PTY_i64:
            LogInfo::MapleLogger() << "int64";
            break;
        case PTY_u8:
            LogInfo::MapleLogger() << "uint8";
            break;
        case PTY_u16:
            LogInfo::MapleLogger() << "uint16";
            break;
        case PTY_u32:
            LogInfo::MapleLogger() << "uint32";
            break;
        case PTY_u64:
            LogInfo::MapleLogger() << "uint64";
            break;
        case PTY_u1:
            LogInfo::MapleLogger() << "bool  ";
            break;
        case PTY_ptr:
            LogInfo::MapleLogger() << "void* ";
            break;
        case PTY_ref:
            LogInfo::MapleLogger() << "void* ";
            break;
        case PTY_a32:
            LogInfo::MapleLogger() << "int32";
            break;
        case PTY_a64:
            LogInfo::MapleLogger() << "void* ";
            break;
        case PTY_f32:
            LogInfo::MapleLogger() << "float ";
            break;
        case PTY_f64:
            LogInfo::MapleLogger() << "double";
            break;
        default:
            DEBUG_ASSERT(false, "not yet implemented");
    }
}
#endif

bool MIRType::IsOfSameType(MIRType &type)
{
    if (typeKind != type.typeKind) {
        return false;
    }

    if (typeKind == kTypePointer) {
        const auto &ptrType = static_cast<const MIRPtrType &>(*this);
        const auto &ptrTypeIt = static_cast<const MIRPtrType &>(type);
        if (ptrType.GetPointedTyIdx() == ptrTypeIt.GetPointedTyIdx()) {
            return true;
        } else {
            MIRType &mirTypeIt = *GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTypeIt.GetPointedTyIdx());
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrType.GetPointedTyIdx())->IsOfSameType(mirTypeIt);
        }
    } else if (typeKind == kTypeJArray) {
        auto &arrType1 = static_cast<MIRJarrayType &>(*this);
        auto &arrType2 = static_cast<MIRJarrayType &>(type);
        if (arrType1.GetDim() != arrType2.GetDim()) {
            return false;
        }
        return arrType1.GetElemType()->IsOfSameType(*arrType2.GetElemType());
    } else {
        return tyIdx == type.tyIdx;
    }
}

#ifdef ARK_LITECG_DEBUG
inline void DumpTypeName(GStrIdx strIdx, bool isLocal)
{
    LogInfo::MapleLogger() << ((isLocal) ? "%" : "$") << GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx);
}

static bool CheckAndDumpTypeName(GStrIdx strIdx, bool isLocal)
{
    if (strIdx == 0u) {
        return false;
    }
    LogInfo::MapleLogger() << "<";
    DumpTypeName(strIdx, isLocal);
    LogInfo::MapleLogger() << ">";
    return true;
}

void MIRFuncType::Dump(int indent, bool dontUseName) const
{
    if (!dontUseName && CheckAndDumpTypeName(nameStrIdx, nameIsLocal)) {
        return;
    }
    LogInfo::MapleLogger() << "<func";
    funcAttrs.DumpAttributes();
    LogInfo::MapleLogger() << " (";
    size_t size = paramTypeList.size();
    for (size_t i = 0; i < size; ++i) {
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(paramTypeList[i])->Dump(indent + 1);
        if (i < paramAttrsList.size()) {
            paramAttrsList[i].DumpAttributes();
        }
        if (size - 1 != i) {
            LogInfo::MapleLogger() << ",";
        }
    }
    if (IsVarargs()) {
        LogInfo::MapleLogger() << ", ...";
    }
    LogInfo::MapleLogger() << ") ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTyIdx)->Dump(indent + 1);
    retAttrs.DumpAttributes();
    LogInfo::MapleLogger() << ">";
}
#endif

static constexpr uint64 RoundUpConst(uint64 offset, uint32 align)
{
    uint64 tempFirst = static_cast<uint64>(-align);
    CHECK_FATAL((offset <= UINT64_MAX - align), "must not be zero");
    DEBUG_ASSERT(offset + align > 0, "offset and align should not be zero");
    uint64 tempSecond = static_cast<uint64>(offset + align - 1);
    return tempFirst & tempSecond;
}

static inline uint64 RoundUp(uint64 offset, uint32 align)
{
    if (align == 0) {
        return offset;
    }
    return RoundUpConst(offset, align);
}

size_t MIRArrayType::GetSize() const
{
    if (size != kInvalidSize) {
        return size;
    }
    size_t elemsize = GetElemType()->GetSize();
    if (elemsize == 0) {
        return 0;
    }
    elemsize = RoundUp(elemsize, typeAttrs.GetAlign());
    size_t numelems = sizeArray[0];
    for (uint16 i = 1; i < dim; i++) {
        numelems *= sizeArray[i];
    }
    size = elemsize * numelems;
    return size;
}

uint32 MIRArrayType::GetAlign() const
{
    if (GetSize() == 0) {
        return typeAttrs.GetAlign();
    }
    return std::max(GetElemType()->GetAlign(), typeAttrs.GetAlign());
}

#ifdef ARK_LITECG_DEBUG
void MIRArrayType::Dump(int indent, bool dontUseName) const
{
    if (!dontUseName && CheckAndDumpTypeName(nameStrIdx, nameIsLocal)) {
        return;
    }
    LogInfo::MapleLogger() << "<";
    for (uint16 i = 0; i < dim; ++i) {
        LogInfo::MapleLogger() << "[" << GetSizeArrayItem(i) << "]";
    }
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(eTyIdx)->Dump(indent + 1);
    GetTypeAttrs().DumpAttributes();
    LogInfo::MapleLogger() << ">";
}
#endif

std::string MIRArrayType::GetCompactMplTypeName() const
{
    std::stringstream ss;
    ss << "A";
    MIRType *elemType = GetElemType();
    ss << elemType->GetCompactMplTypeName();
    return ss.str();
}

#ifdef ARK_LITECG_DEBUG
void MIRFarrayType::Dump(int indent, bool dontUseName) const
{
    if (!dontUseName && CheckAndDumpTypeName(nameStrIdx, nameIsLocal)) {
        return;
    }
    LogInfo::MapleLogger() << "<[] ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(elemTyIdx)->Dump(indent + 1);
    LogInfo::MapleLogger() << ">";
}
#endif

std::string MIRFarrayType::GetCompactMplTypeName() const
{
    std::stringstream ss;
    ss << "A";
    MIRType *elemType = GetElemType();
    ss << elemType->GetCompactMplTypeName();
    return ss.str();
}

void MIRJarrayType::DetermineName()
{
    if (jNameStrIdx != 0u) {
        return;  // already determined
    }
    dim = 1;
    std::string baseName;
    std::string name;
    for (int i = dim; i > 0; --i) {
        name += JARRAY_PREFIX_STR;
    }
    name += baseName;
    jNameStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
}

MIRType *MIRPtrType::GetPointedType() const
{
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointedTyIdx);
}

bool MIRType::IsVolatile(int fieldID) const
{
    CHECK_FATAL(fieldID == 0, "fieldID must be 0");
    return HasVolatileField();
}

bool MIRPtrType::HasTypeParam() const
{
    return GetPointedType()->HasTypeParam();
}

bool MIRPtrType::PointsToConstString() const
{
    return false;
}

#ifdef ARK_LITECG_DEBUG
void MIRPtrType::Dump(int indent, bool dontUseName) const
{
    if (!dontUseName && CheckAndDumpTypeName(nameStrIdx, nameIsLocal)) {
        return;
    }
    MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointedTyIdx);
    if (pointedType->GetKind() == kTypeFunction) {  // no * for function pointer
        pointedType->Dump(indent);
    } else {
        LogInfo::MapleLogger() << "<* ";
        pointedType->Dump(indent + 1);
        typeAttrs.DumpAttributes();
        LogInfo::MapleLogger() << ">";
    }
}

void MIRBitFieldType::Dump(int indent, bool dontUseName) const
{
    LogInfo::MapleLogger() << ":" << static_cast<int>(fieldSize) << " " << GetPrimTypeName(primType);
}
#endif

#ifdef ARK_LITECG_DEBUG
void MIRTypeByName::Dump(int indent, bool dontUseName) const
{
    const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(nameStrIdx);
    LogInfo::MapleLogger() << (nameIsLocal ? "<%" : "<$") << name << ">";
}

void MIRInstantVectorType::Dump(int indent, bool dontUseName) const
{
    LogInfo::MapleLogger() << "{";
    for (size_t i = 0; i < instantVec.size(); ++i) {
        TypePair typePair = instantVec[i];
        if (i != 0) {
            LogInfo::MapleLogger() << ", ";
        }
        MIRType *typeParmType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typePair.first);
        const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(typeParmType->GetNameStrIdx());
        LogInfo::MapleLogger() << "!" << name << "=";
        MIRType *realty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typePair.second);
        realty->Dump(0);
    }
    LogInfo::MapleLogger() << "}";
}

void MIRGenericInstantType::Dump(int indent, bool dontUseName) const
{
    LogInfo::MapleLogger() << "<";
    const MIRType *genericType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(genericTyIdx);
    DumpTypeName(genericType->GetNameStrIdx(), genericType->IsNameIsLocal());
    MIRInstantVectorType::Dump(indent, dontUseName);
    LogInfo::MapleLogger() << ">";
}
#endif

bool MIRType::EqualTo(const MIRType &mirType) const
{
    return typeKind == mirType.typeKind && primType == mirType.primType;
}

bool MIRPtrType::EqualTo(const MIRType &type) const
{
    if (typeKind != type.GetKind() || GetPrimType() != type.GetPrimType()) {
        return false;
    }
    const auto &pType = static_cast<const MIRPtrType &>(type);
    return pointedTyIdx == pType.GetPointedTyIdx() && typeAttrs == pType.GetTypeAttrs();
}

bool MIRArrayType::EqualTo(const MIRType &type) const
{
    if (type.GetKind() != typeKind) {
        return false;
    }
    const auto &pType = static_cast<const MIRArrayType &>(type);
    if (dim != pType.GetDim() || eTyIdx != pType.GetElemTyIdx() || typeAttrs != pType.GetTypeAttrs()) {
        return false;
    }
    for (size_t i = 0; i < dim; ++i) {
        if (GetSizeArrayItem(i) != pType.GetSizeArrayItem(i)) {
            return false;
        }
    }
    return true;
}

MIRType *MIRArrayType::GetElemType() const
{
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(eTyIdx);
}

std::string MIRArrayType::GetMplTypeName() const
{
    std::stringstream ss;
    ss << "<[] ";
    MIRType *elemType = GetElemType();
    ss << elemType->GetMplTypeName();
    ss << ">";
    return ss.str();
}

bool MIRArrayType::HasFields() const
{
    MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(eTyIdx);
    return elemType->HasFields();
}

uint32 MIRArrayType::NumberOfFieldIDs() const
{
    if (fieldsNum != kInvalidFieldNum) {
        return fieldsNum;
    }
    MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(eTyIdx);
    fieldsNum = elemType->NumberOfFieldIDs();
    return fieldsNum;
}

int64 MIRArrayType::GetBitOffsetFromArrayAddress(std::vector<int64> &indexArray)
{
    // for cases that num of dimension-operand is less than array's dimension
    // e.g. array 1 ptr <* <[d1][d2][d3] type>> (array_base_addr, dim1_opnd)
    // array's dimension is 3, and dimension-opnd's num is 1;
    if (indexArray.size() < dim) {
        indexArray.insert(indexArray.end(), dim - indexArray.size(), 0);
    }
    if (GetElemType()->GetKind() == kTypeArray) {
        // for cases that array's dimension is less than num of dimension-opnd
        // e.g array 1 ptr <* <[d1] <[d2] type>>> (array_base_addr, dim1_opnd, dim2_opnd)
        // array's dimension is 1 (its element type is <[d2] type>), and dimension-opnd-num is 2
        CHECK_FATAL(indexArray.size() >= dim, "dimension mismatch!");
    } else {
        CHECK_FATAL(indexArray.size() == dim, "dimension mismatch!");
    }
    int64 sum = 0;  // element numbers before the specified element
    uint32 numberOfElemInLowerDim = 1;
    for (uint32 id = 1; id <= dim; ++id) {
        sum += indexArray[dim - id] * static_cast<int64>(numberOfElemInLowerDim);
        numberOfElemInLowerDim *= sizeArray[dim - id];
    }
    size_t elemsize = GetElemType()->GetSize();
    if (elemsize == 0 || sum == 0) {
        return 0;
    }
    elemsize = RoundUp(elemsize, typeAttrs.GetAlign());
    constexpr int64 bitsPerByte = 8;
    int64 offset = static_cast<int64>(static_cast<uint64>(sum) * elemsize * static_cast<uint64>(bitsPerByte));
    if (GetElemType()->GetKind() == kTypeArray && indexArray.size() > dim) {
        std::vector<int64> subIndexArray(indexArray.begin() + dim, indexArray.end());
        offset += static_cast<MIRArrayType *>(GetElemType())->GetBitOffsetFromArrayAddress(subIndexArray);
    }
    return offset;
}

size_t MIRArrayType::ElemNumber() const
{
    size_t elemNum = 1;
    for (uint16 id = 0; id < dim; ++id) {
        elemNum *= sizeArray[id];
    }

    auto elemType = GetElemType();
    if (elemType->GetKind() == kTypeArray) {
        elemNum *= static_cast<MIRArrayType *>(elemType)->ElemNumber();
    }
    return elemNum;
}

MIRType *MIRFarrayType::GetElemType() const
{
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(elemTyIdx);
}

bool MIRFarrayType::EqualTo(const MIRType &type) const
{
    if (type.GetKind() != typeKind) {
        return false;
    }
    const auto &pType = static_cast<const MIRFarrayType &>(type);
    return elemTyIdx == pType.GetElemTyIdx();
}

std::string MIRFarrayType::GetMplTypeName() const
{
    std::stringstream ss;
    ss << "<[] ";
    MIRType *elemType = GetElemType();
    ss << elemType->GetMplTypeName();
    ss << ">";
    return ss.str();
}

bool MIRFarrayType::HasFields() const
{
    MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(elemTyIdx);
    return elemType->HasFields();
}

uint32 MIRFarrayType::NumberOfFieldIDs() const
{
    if (fieldsNum != kInvalidFieldNum) {
        return fieldsNum;
    }
    MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(elemTyIdx);
    fieldsNum = elemType->NumberOfFieldIDs();
    return fieldsNum;
}

int64 MIRFarrayType::GetBitOffsetFromArrayAddress(int64 arrayIndex)
{
    size_t elemsize = GetElemType()->GetSize();
    if (elemsize == 0 || arrayIndex == 0) {
        return 0;
    }
    uint32 elemAlign = GetElemType()->GetAlign();
    elemsize = RoundUp(elemsize, elemAlign);
    constexpr int64 bitsPerByte = 8;
    return arrayIndex * static_cast<int64>(elemsize) * bitsPerByte;
}

bool MIRFuncType::EqualTo(const MIRType &type) const
{
    if (type.GetKind() != typeKind) {
        return false;
    }
    const auto &pType = static_cast<const MIRFuncType &>(type);
    return (pType.retTyIdx == retTyIdx && pType.paramTypeList == paramTypeList && pType.funcAttrs == funcAttrs &&
            pType.paramAttrsList == paramAttrsList && pType.retAttrs == retAttrs);
}

static bool TypeCompatible(TyIdx typeIdx1, TyIdx typeIdx2)
{
    if (typeIdx1 == typeIdx2) {
        return true;
    }
    MIRType *type1 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx1);
    MIRType *type2 = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx2);
    if (type1 == nullptr || type2 == nullptr) {
        // this means we saw the use of this symbol before def.
        return false;
    }
    while (type1->IsMIRPtrType() || type1->IsMIRArrayType()) {
        if (type1->IsMIRPtrType()) {
            if (!type2->IsMIRPtrType()) {
                return false;
            }
            type1 = static_cast<MIRPtrType *>(type1)->GetPointedType();
            type2 = static_cast<MIRPtrType *>(type2)->GetPointedType();
        } else {
            if (!type2->IsMIRArrayType()) {
                return false;
            }
            type1 = static_cast<MIRArrayType *>(type1)->GetElemType();
            type2 = static_cast<MIRArrayType *>(type2)->GetElemType();
        }
    }
    if (type1 == type2) {
        return true;
    }
    if (type1->GetPrimType() == PTY_void || type2->GetPrimType() == PTY_void) {
        return true;
    }
    if (type1->IsIncomplete() || type2->IsIncomplete()) {
        return true;
    }
    return false;
}

bool MIRFuncType::CompatibleWith(const MIRType &type) const
{
    if (type.GetKind() != typeKind) {
        return false;
    }
    const auto &pType = static_cast<const MIRFuncType &>(type);
    if (!TypeCompatible(pType.retTyIdx, retTyIdx)) {
        return false;
    }
    if (pType.paramTypeList.size() != paramTypeList.size()) {
        return false;
    }
    for (size_t i = 0; i < paramTypeList.size(); i++) {
        if (!TypeCompatible(pType.GetNthParamType(i), GetNthParamType(i))) {
            return false;
        }
    }
    return true;
}

bool MIRBitFieldType::EqualTo(const MIRType &type) const
{
    if (type.GetKind() != typeKind || type.GetPrimType() != primType) {
        return false;
    }
    const auto &pType = static_cast<const MIRBitFieldType &>(type);
    return pType.fieldSize == fieldSize;
}

bool MIRTypeByName::EqualTo(const MIRType &type) const
{
    return type.GetKind() == typeKind && type.GetNameStrIdx() == nameStrIdx && type.IsNameIsLocal() == nameIsLocal;
}

bool MIRInstantVectorType::EqualTo(const MIRType &type) const
{
    if (type.GetKind() != typeKind) {
        return false;
    }
    const auto &pty = static_cast<const MIRInstantVectorType &>(type);
    return (instantVec == pty.GetInstantVec());
}

bool MIRGenericInstantType::EqualTo(const MIRType &type) const
{
    if (!MIRInstantVectorType::EqualTo(type)) {
        return false;
    }
    const auto &pType = static_cast<const MIRGenericInstantType &>(type);
    return genericTyIdx == pType.GetGenericTyIdx();
}

bool MIRPtrType::IsPointedTypeVolatile(int fieldID) const
{
    if (typeAttrs.GetAttr(ATTR_volatile)) {
        return true;
    }
    MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetPointedTyIdx());
    return pointedTy->IsVolatile(fieldID);
}

bool MIRPtrType::IsUnsafeType() const
{
    if (GetTypeAttrs().GetAttr(ATTR_may_alias)) {
        return true;
    }
    // Check for <* void>/<* i8/u8>
    MIRType *pointedType = GetPointedType();
    while (pointedType->IsMIRPtrType()) {
        pointedType = static_cast<MIRPtrType *>(pointedType)->GetPointedType();
    }
    return (pointedType->GetPrimType() == PTY_void || pointedType->GetSize() == 1);
}

bool MIRPtrType::IsVoidPointer() const
{
    return GlobalTables::GetTypeTable().GetVoidPtr() == this;
}

size_t MIRPtrType::GetSize() const
{
    return GetPointerSize();
}
uint32 MIRPtrType::GetAlign() const
{
    return GetPointerSize();
}

std::string MIRPtrType::GetMplTypeName() const
{
    std::stringstream ss;
    ss << "<* ";
    MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointedTyIdx);
    CHECK_FATAL(pointedType != nullptr, "invalid ptr type");
    ss << pointedType->GetMplTypeName();
    ss << ">";
    return ss.str();
}

std::string MIRPtrType::GetCompactMplTypeName() const
{
    MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointedTyIdx);
    CHECK_FATAL(pointedType != nullptr, "invalid ptr type");
    return pointedType->GetCompactMplTypeName();
}

MIRFuncType *MIRPtrType::GetPointedFuncType() const
{
    MIRType *pointedType = GetPointedType();
    if (pointedType->GetKind() == kTypeFunction) {
        return static_cast<MIRFuncType *>(pointedType);
    }
    if (pointedType->GetKind() == kTypePointer) {
        MIRPtrType *pointedPtrType = static_cast<MIRPtrType *>(pointedType);
        if (pointedPtrType->GetPointedType()->GetKind() == kTypeFunction) {
            return static_cast<MIRFuncType *>(pointedPtrType->GetPointedType());
        }
    }
    return nullptr;
}

TypeAttrs FieldAttrs::ConvertToTypeAttrs()
{
    TypeAttrs attr;
    constexpr uint32 maxAttrNum = 64;
    for (uint32 i = 0; i < maxAttrNum; ++i) {
        if ((attrFlag & (1ULL << i)) == 0) {
            continue;
        }
        auto attrKind = static_cast<FieldAttrKind>(i);
        switch (attrKind) {
#define FIELD_ATTR
#define ATTR(STR)                 \
    case FLDATTR_##STR:           \
        attr.SetAttr(ATTR_##STR); \
        break;
#include "all_attributes.def"
#undef ATTR
#undef FIELD_ATTR
            default:
                DEBUG_ASSERT(false, "unknown TypeAttrs");
                break;
        }
    }
    return attr;
}

MIRType *GetElemType(const MIRType &arrayType)
{
    if (arrayType.GetKind() == kTypeArray) {
        return static_cast<const MIRArrayType &>(arrayType).GetElemType();
    } else if (arrayType.GetKind() == kTypeFArray) {
        return static_cast<const MIRFarrayType &>(arrayType).GetElemType();
    } else if (arrayType.GetKind() == kTypeJArray) {
        return static_cast<const MIRJarrayType &>(arrayType).GetElemType();
    }
    return nullptr;
}

#ifdef TARGAARCH64
static constexpr size_t kMaxHfaOrHvaElemNumber = 4;

bool CheckHomogeneousAggregatesBaseType(PrimType type)
{
    if (type == PTY_f32 || type == PTY_f64 || type == PTY_f128) {
        return true;
    }
    return false;
}

bool IsSameHomogeneousAggregatesBaseType(PrimType type, PrimType nextType)
{
    if ((type == PTY_f32 || type == PTY_f64 || type == PTY_f128) && type == nextType) {
        return true;
    }
    return false;
}

bool IsArrayHomogeneousAggregates(const MIRArrayType &ty, PrimType &primType, size_t &elemNum)
{
    primType = PTY_begin;
    elemNum = 0;
    MIRType *elemMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ty.GetElemTyIdx());
    if (!IsHomogeneousAggregates(*elemMirType, primType, elemNum, false)) {
        return false;
    }
    elemNum *= ty.ElemNumber();
    if (elemNum > kMaxHfaOrHvaElemNumber) {
        return false;
    }
    uint32 needSize = static_cast<uint32>(GetPrimTypeSize(primType) * elemNum);
    return (ty.GetSize() == needSize);
}

bool IsHomogeneousAggregates(const MIRType &ty, PrimType &primType, size_t &elemNum, bool firstDepth)
{
    if (firstDepth) {
        return false;
    }
    primType = PTY_begin;
    elemNum = 0;
    if (ty.GetKind() == kTypeArray) {
        auto &arrType = static_cast<const MIRArrayType &>(ty);
        return IsArrayHomogeneousAggregates(arrType, primType, elemNum);
    } else {
        primType = ty.GetPrimType();
        elemNum = 1;
        if (!CheckHomogeneousAggregatesBaseType(primType)) {
            return false;
        }
    }
    return (elemNum != 0);
}

bool IsParamStructCopyToMemory(const MIRType &ty)
{
    if (ty.GetPrimType() == PTY_agg) {
        PrimType primType = PTY_begin;
        size_t elemNum = 0;
        return !IsHomogeneousAggregates(ty, primType, elemNum) && ty.GetSize() > k16BitSize;
    }
    return false;
}
#else
bool IsParamStructCopyToMemory(const MIRType &ty)
{
    return ty.GetPrimType() == PTY_agg && ty.GetSize() > k16BitSize;
}
#endif  // TARGAARCH64
}  // namespace maple
#endif  // MIR_FEATURE_FULL
