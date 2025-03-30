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
#include "mir_builder.h"
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

PrimType GetLoweredPtrType()
{
#if TARGX86_64 || TARGAARCH64
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        return PTY_a64;
    } else if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
        auto pty = PTY_a64;
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
    return PTY_a64;
}

// answer in bytes; 0 if unknown
uint32 GetPrimTypeSize(PrimType primType)
{
    switch (primType) {
        case PTY_void:
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
        case PTY_f32:
        case PTY_i32:
        case PTY_u32:
            return k4BitSize;
        case PTY_a64:
        case PTY_f64:
        case PTY_i64:
        case PTY_u64:
            return k8BitSize;
        default:
            return k0BitSize;
    }
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

#ifdef ARK_LITECG_DEBUG
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
#endif

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

#ifdef ARK_LITECG_DEBUG
bool MIRPtrType::PointsToConstString() const
{
    return false;
}

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
#endif

#ifdef ARK_LITECG_DEBUG
void MIRTypeByName::Dump(int indent, bool dontUseName) const
{
    const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(nameStrIdx);
    LogInfo::MapleLogger() << (nameIsLocal ? "<%" : "<$") << name << ">";
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

#ifdef ARK_LITECG_DEBUG
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
#endif

bool MIRFuncType::EqualTo(const MIRType &type) const
{
    if (type.GetKind() != typeKind) {
        return false;
    }
    const auto &pType = static_cast<const MIRFuncType &>(type);
    return (pType.retTyIdx == retTyIdx && pType.paramTypeList == paramTypeList && pType.funcAttrs == funcAttrs &&
            pType.paramAttrsList == paramAttrsList && pType.retAttrs == retAttrs);
}

bool MIRPtrType::IsPointedTypeVolatile(int fieldID) const
{
    if (typeAttrs.GetAttr(ATTR_volatile)) {
        return true;
    }
    MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetPointedTyIdx());
    return pointedTy->IsVolatile(fieldID);
}

size_t MIRPtrType::GetSize() const
{
    return GetPointerSize();
}
uint32 MIRPtrType::GetAlign() const
{
    return GetPointerSize();
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
}  // namespace maple
#endif  // MIR_FEATURE_FULL
