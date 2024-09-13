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

#ifndef MAPLE_IR_INCLUDE_MIR_TYPE_H
#define MAPLE_IR_INCLUDE_MIR_TYPE_H
#include <algorithm>
#include <array>
#include "prim_types.h"
#include "mir_pragma.h"
#include "mpl_logging.h"
#if MIR_FEATURE_FULL
#include "mempool.h"
#include "mempool_allocator.h"
#endif  // MIR_FEATURE_FULL

namespace maple {
constexpr uint32 kTypeHashLength = 12289;   // hash length for mirtype, ref: planetmath.org/goodhashtableprimes
const std::string kRenameKeyWord = "_MNO";  // A static symbol name will be renamed as oriname_MNOxxx.
const uint8 kAlignBase = 0;                 // alignment base
using MIRTypePtr = MIRType *;

constexpr size_t kMaxArrayDim = 20;
constexpr uint32 kInvalidFieldNum = UINT32_MAX;
constexpr size_t kInvalidSize = static_cast<size_t>(UINT64_MAX);
#if MIR_FEATURE_FULL
extern bool VerifyPrimType(PrimType primType1, PrimType primType2);  // verify if primType1 and primType2 match
extern PrimType GetExactPtrPrimType();                               // return either PTY_a64 or PTY_a32
extern uint32 GetPrimTypeSize(PrimType primType);                    // answer in bytes; 0 if unknown
extern uint32 GetPrimTypeP2Size(PrimType primType);                  // answer in bytes in power-of-two.
extern PrimType GetUnsignedPrimType(PrimType pty);                   // return unsigned version
#ifdef ARK_LITECG_DEBUG
extern const char *GetPrimTypeName(PrimType primType);
#endif
extern int64 MinValOfSignedInteger(PrimType primType);
extern PrimType GetVecElemPrimType(PrimType primType);
constexpr uint32 k0BitSize = 0;
constexpr uint32 k1BitSize = 1;
constexpr uint32 k2BitSize = 2;
constexpr uint32 k3BitSize = 3;
constexpr uint32 k4BitSize = 4;
constexpr uint32 k5BitSize = 5;
constexpr uint32 k7BitSize = 7;
constexpr uint32 k8BitSize = 8;
constexpr uint32 k9BitSize = 9;
constexpr uint32 k10BitSize = 10;
constexpr uint32 k16BitSize = 16;
constexpr uint32 k32BitSize = 32;
constexpr uint32 k64BitSize = 64;
constexpr uint32 k4BitShift = 2;  /* 4 is 1 << 2; */
constexpr uint32 k8BitShift = 3;  /* 8 is 1 << 3; */
constexpr uint32 k16BitShift = 4; /* 8 is 1 << 3; */
constexpr uint32 k2ByteSize = 2;
constexpr uint32 k3ByteSize = 3;
constexpr uint32 k4ByteSize = 4;
constexpr uint32 k7ByteSize = 7;
constexpr uint32 k8ByteSize = 8;

inline uint32 GetPrimTypeBitSize(PrimType primType)
{
    // 1 byte = 8 bits = 2^3 bits
    return GetPrimTypeSize(primType) << k8BitShift;
}

inline uint32 GetAlignedPrimTypeBitSize(PrimType primType)
{
    auto size = GetPrimTypeBitSize(primType);
    return size <= k32BitSize ? k32BitSize : k64BitSize;
}

inline uint32 GetPrimTypeActualBitSize(PrimType primType)
{
    // GetPrimTypeSize(PTY_u1) will return 1, so we take it as a special case
    if (primType == PTY_u1) {
        return 1;
    }
    // 1 byte = 8 bits = 2^3 bits
    return GetPrimTypeSize(primType) << k8BitShift;
}

#endif  // MIR_FEATURE_FULL
// return the same type with size increased to register size
PrimType GetRegPrimType(PrimType primType);
PrimType GetIntegerPrimTypeBySizeAndSign(size_t sizeBit, bool isSign);

inline bool IsAddress(PrimitiveType primitiveType)
{
    return primitiveType.IsAddress();
}

inline bool IsPossible64BitAddress(PrimType tp)
{
    return (tp == PTY_ptr || tp == PTY_ref || tp == PTY_u64 || tp == PTY_a64);
}

inline bool IsPossible32BitAddress(PrimType tp)
{
    return (tp == PTY_ptr || tp == PTY_ref || tp == PTY_u32);
}


inline bool IsPrimitivePureScalar(PrimitiveType primitiveType)
{
    return primitiveType.IsInteger() && !primitiveType.IsAddress();
}

inline bool IsPrimitiveUnsigned(PrimitiveType primitiveType)
{
    return primitiveType.IsUnsigned();
}

inline bool IsUnsignedInteger(PrimitiveType primitiveType)
{
    return IsPrimitiveUnsigned(primitiveType) && primitiveType.IsInteger();
}

inline bool IsSignedInteger(PrimitiveType primitiveType)
{
    return !IsPrimitiveUnsigned(primitiveType) && primitiveType.IsInteger();
}

inline bool IsPrimitiveInteger(PrimitiveType primitiveType)
{
    return primitiveType.IsInteger();
}

inline bool IsPrimitiveFloat(PrimitiveType primitiveType)
{
    return primitiveType.IsFloat();
}

inline bool IsPrimitiveScalar(PrimitiveType primitiveType)
{
    return primitiveType.IsInteger() || primitiveType.IsFloat() || primitiveType.IsSimple();
}

inline bool IsPrimitivePoint(PrimitiveType primitiveType)
{
    return primitiveType.IsPointer();
}

bool IsNoCvtNeeded(PrimType toType, PrimType fromType);

uint8 GetPointerSize();
PrimType GetLoweredPtrType();

inline bool IsRefOrPtrAssign(PrimType toType, PrimType fromType)
{
    return (toType == PTY_ref && fromType == PTY_ptr) || (toType == PTY_ptr && fromType == PTY_ref);
}

enum MIRTypeKind : std::uint8_t {
    kTypeScalar,
    kTypeArray,
    kTypePointer,
    kTypeFunction,
    kTypeVoid,
    kTypeByName,          // type definition not yet seen
};

enum AttrKind : unsigned {
#define TYPE_ATTR
#define ATTR(STR) ATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef TYPE_ATTR
};

class AttrBoundary {
public:
    AttrBoundary() = default;
    ~AttrBoundary() = default;

    bool operator==(const AttrBoundary &tA) const
    {
        return lenExprHash == tA.lenExprHash && lenParamIdx == tA.lenParamIdx && isBytedLen == tA.isBytedLen;
    }

    bool operator!=(const AttrBoundary &tA) const
    {
        return !(*this == tA);
    }

    bool operator<(const AttrBoundary &tA) const
    {
        return lenExprHash < tA.lenExprHash && lenParamIdx < tA.lenParamIdx &&
               static_cast<int>(isBytedLen) < static_cast<int>(tA.isBytedLen);
    }

    void SetLenExprHash(uint32 val)
    {
        lenExprHash = val;
    }

    uint32 GetLenExprHash() const
    {
        return lenExprHash;
    }

    void SetLenParamIdx(int8 idx)
    {
        lenParamIdx = idx;
    }

    int8 GetLenParamIdx() const
    {
        return lenParamIdx;
    }

    void SetIsBytedLen(bool flag)
    {
        isBytedLen = flag;
    }

    bool IsBytedLen() const
    {
        return isBytedLen;
    }

    void Clear()
    {
        lenExprHash = 0;
        lenParamIdx = -1;
        isBytedLen = false;
    }

private:
    bool isBytedLen = false;
    int8 lenParamIdx = -1;
    uint32 lenExprHash = 0;
};

class TypeAttrs {
public:
    TypeAttrs() = default;
    TypeAttrs(const TypeAttrs &ta) = default;
    TypeAttrs &operator=(const TypeAttrs &t) = default;
    ~TypeAttrs() = default;

    uint8 GetAlignValue() const
    {
        return attrAlign;
    }

    void SetAttrFlag(uint64 flag)
    {
        attrFlag = flag;
    }

    uint8 GetTypeAlignValue() const
    {
        return attrTypeAlign;
    }

    uint64 GetAttrFlag() const
    {
        return attrFlag;
    }

    void SetAttr(AttrKind x)
    {
        attrFlag |= (1ULL << static_cast<unsigned int>(x));
    }

    void ResetAttr(AttrKind x)
    {
        attrFlag &= ~(1ULL << static_cast<unsigned int>(x));
    }

    bool GetAttr(AttrKind x) const
    {
        return (attrFlag & (1ULL << static_cast<unsigned int>(x))) != 0;
    }

    void SetAlign(uint32 x)
    {
        DEBUG_ASSERT((~(x - 1) & x) == x, "SetAlign called with non-power-of-2");
        attrAlign = 0;
        while (x != 1) {
            x >>= 1;
            ++attrAlign;
        }
    }

    uint32 GetAlign() const
    {
        if (attrAlign == 0) {
            return 1;
        }
        uint32 res = 1;
        uint32 exp = attrAlign;
        do {
            --exp;
            res <<= 1;
        } while (exp != 0);
        return res;
    }

    void SetTypeAlign(uint32 x)
    {
        DEBUG_ASSERT((~(x - 1) & x) == x, "SetTypeAlign called with non-power-of-2");
        attrTypeAlign = 0;
        while (x != kAlignBase) {
            x >>= 1;
            ++attrTypeAlign;
        }
    }

    uint32 GetTypeAlign() const
    {
        if (attrTypeAlign == 1) {  // align(1)
            return 1;
        }
        constexpr uint32 square = 2;
        uint32 res = 1;
        uint32 exp = attrTypeAlign;
        while (exp > 1) {  // calculate align(x)
            --exp;
            res *= square;
        }
        return res;
    }

    bool operator==(const TypeAttrs &tA) const
    {
        return attrFlag == tA.attrFlag && attrAlign == tA.attrAlign && attrBoundary == tA.attrBoundary;
    }

    bool operator!=(const TypeAttrs &tA) const
    {
        return !(*this == tA);
    }

#ifdef ARK_LITECG_DEBUG
    void DumpAttributes() const;
#endif

    const AttrBoundary &GetAttrBoundary() const
    {
        return attrBoundary;
    }

    AttrBoundary &GetAttrBoundary()
    {
        return attrBoundary;
    }

    void AddAttrBoundary(const AttrBoundary &attr)
    {
        if (attr.GetLenExprHash() != 0) {
            attrBoundary.SetLenExprHash(attr.GetLenExprHash());
        }
        if (attr.GetLenParamIdx() != -1) {
            attrBoundary.SetLenParamIdx(attr.GetLenParamIdx());
        }
        if (attr.IsBytedLen()) {
            attrBoundary.SetIsBytedLen(attr.IsBytedLen());
        }
    }

    void SetPack(uint32 pack)
    {
        attrPack = pack;
    }

    uint32 GetPack() const
    {
        return attrPack;
    }

    bool IsPacked() const
    {
        return GetAttr(ATTR_pack);
    }

    bool HasPack() const
    {
        return GetAttr(ATTR_pack);
    }

    bool IsTypedef() const
    {
        return GetAttr(ATTR_typedef);
    }

    MIRType *GetOriginType() const
    {
        return originType;
    }

private:
    uint64 attrFlag = 0;
    uint8 attrAlign = 0;        // alignment in bytes is 2 to the power of attrAlign
    uint8 attrTypeAlign = 0;    // alignment in bytes is 2 to the power of attrTypeAlign
    uint32 attrPack = -1;       // -1 means inactive
    AttrBoundary attrBoundary;  // boundary attr for EnhanceC
    MIRType *originType = nullptr;
};

enum FieldAttrKind {
#define FIELD_ATTR
#define ATTR(STR) FLDATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef FIELD_ATTR
};

enum StmtAttrKind : unsigned {
#define STMT_ATTR
#define ATTR(STR) STMTATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef STMT_ATTR
};

class StmtAttrs {
public:
    StmtAttrs() = default;
    StmtAttrs(const StmtAttrs &ta) = default;
    StmtAttrs &operator=(const StmtAttrs &p) = default;
    ~StmtAttrs() = default;

    void SetAttr(StmtAttrKind x, bool flag = true)
    {
        if (flag) {
            attrFlag |= (1u << static_cast<unsigned int>(x));
        } else {
            attrFlag &= ~(1u << static_cast<unsigned int>(x));
        }
    }

    bool GetAttr(StmtAttrKind x) const
    {
        return (attrFlag & (1u << static_cast<unsigned int>(x))) != 0;
    }

    uint32 GetTargetAttrFlag(StmtAttrKind x) const
    {
        return attrFlag & (1u << static_cast<unsigned int>(x));
    }

    uint32 GetAttrFlag() const
    {
        return attrFlag;
    }

    void AppendAttr(uint32 flag)
    {
        attrFlag |= flag;
    }

    void Clear()
    {
        attrFlag = 0;
    }

#ifdef ARK_LITECG_DEBUG
    void DumpAttributes() const;
#endif

private:
    uint32 attrFlag = 0;
};

enum FuncAttrKind : unsigned {
#define FUNC_ATTR
#define ATTR(STR) FUNCATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef FUNC_ATTR
};

class FuncAttrs {
public:
    FuncAttrs() = default;
    FuncAttrs(const FuncAttrs &ta) = default;
    FuncAttrs &operator=(const FuncAttrs &p) = default;
    ~FuncAttrs() = default;

    void SetAttr(FuncAttrKind x, bool unSet = false)
    {
        if (!unSet) {
            attrFlag |= (1ULL << x);
        } else {
            attrFlag &= ~(1ULL << x);
        }
    }

    void SetAliasFuncName(const std::string &name)
    {
        aliasFuncName = name;
    }

    const std::string &GetAliasFuncName() const
    {
        return aliasFuncName;
    }

    void SetPrefixSectionName(const std::string &name)
    {
        prefixSectionName = name;
    }

    const std::string &GetPrefixSectionName() const
    {
        return prefixSectionName;
    }

    void SetAttrFlag(uint64 flag)
    {
        attrFlag = flag;
    }

    uint64 GetAttrFlag() const
    {
        return attrFlag;
    }

    bool GetAttr(FuncAttrKind x) const
    {
        return (attrFlag & (1ULL << x)) != 0;
    }

    bool operator==(const FuncAttrs &tA) const
    {
        return attrFlag == tA.attrFlag;
    }

    bool operator!=(const FuncAttrs &tA) const
    {
        return !(*this == tA);
    }

#ifdef ARK_LITECG_DEBUG
    void DumpAttributes() const;
#endif

    const AttrBoundary &GetAttrBoundary() const
    {
        return attrBoundary;
    }

    AttrBoundary &GetAttrBoundary()
    {
        return attrBoundary;
    }

    void SetConstructorPriority(int priority)
    {
        constructorPriority = priority;
    }

    int GetConstructorPriority() const
    {
        return constructorPriority;
    }

    void SetDestructorPriority(int priority)
    {
        destructorPriority = priority;
    }

    int GetDestructorPriority() const
    {
        return destructorPriority;
    }

    int GetFrameResverdSlot() const
    {
        return frameResverdSlot;
    }

    void SetFrameResverdSlot(int slot)
    {
        SetAttr(FUNCATTR_frame_pointer);
        frameResverdSlot = slot;
    }

    void SetFramePointer(std::string framePointer_)
    {
        SetAttr(FUNCATTR_frame_reserved_slots);
        framePointer = framePointer_;
    }

private:
    uint64 attrFlag = 0;
    std::string aliasFuncName;
    std::string prefixSectionName;
    std::string framePointer;
    AttrBoundary attrBoundary;     // ret boundary for EnhanceC
    int constructorPriority = -1;  // 0~65535, -1 means inactive
    int destructorPriority = -1;   // 0~65535, -1 means inactive
    int frameResverdSlot = 0;
};

#if MIR_FEATURE_FULL
constexpr size_t kShiftNumOfTypeKind = 8;
constexpr size_t kShiftNumOfNameStrIdx = 6;
constexpr int32 kOffsetUnknown = INT_MAX;
constexpr int32 kOffsetMax = (INT_MAX - 1);
constexpr int32 kOffsetMin = INT_MIN;
struct OffsetType {
    explicit OffsetType(int64 offset)
    {
        Set(offset);
    }

    OffsetType(const OffsetType &other) : val(other.val) {}

    ~OffsetType() = default;

    void Set(int64 offsetVal)
    {
        val = (offsetVal >= kOffsetMin && offsetVal <= kOffsetMax) ? static_cast<int32>(offsetVal) : kOffsetUnknown;
    }

    bool IsInvalid() const
    {
        return val == kOffsetUnknown;
    }

    OffsetType &operator=(const OffsetType &other)
    {
        val = other.val;
        return *this;
    }

    OffsetType operator+(int64 offset) const
    {
        if (this->IsInvalid() || OffsetType(offset).IsInvalid()) {
            return InvalidOffset();
        }
        return OffsetType(val + offset);
    }

    OffsetType operator+(OffsetType other) const
    {
        return other + val;
    }

    void operator+=(int64 offset)
    {
        if (this->IsInvalid() || OffsetType(offset).IsInvalid()) {
            val = kOffsetUnknown;
            return;
        }
        Set(offset + val);
    }

    void operator+=(OffsetType other)
    {
        this->operator+=(other.val);
    }

    OffsetType operator-() const
    {
        if (this->IsInvalid()) {
            return *this;
        }
        return OffsetType(-val);
    }

    bool operator<(OffsetType other) const
    {
        return val < other.val;
    }

    bool operator==(OffsetType other) const
    {
        return val == other.val;
    }

    bool operator!=(OffsetType other) const
    {
        return val != other.val;
    }

    static OffsetType InvalidOffset()
    {
        return OffsetType(kOffsetUnknown);
    }

    int32 val = kOffsetUnknown;
};
class MIRFuncType;

class MIRType {
public:
    MIRType(MIRTypeKind kind) : typeKind(kind) {}

    MIRType(MIRTypeKind kind, PrimType pType) : typeKind(kind), primType(pType) {}

    MIRType(MIRTypeKind kind, GStrIdx strIdx) : typeKind(kind), nameStrIdx(strIdx) {}

    MIRType(MIRTypeKind kind, PrimType pType, GStrIdx strIdx) : typeKind(kind), primType(pType), nameStrIdx(strIdx) {}

    virtual ~MIRType() = default;
#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int indent, bool dontUseName = false) const;
    virtual void DumpAsCxx(int indent) const;
#endif
    virtual bool EqualTo(const MIRType &mirType) const;

    virtual MIRType *CopyMIRTypeNode() const
    {
        return new MIRType(*this);
    }

    PrimType GetPrimType() const
    {
        return primType;
    }
    void SetPrimType(const PrimType pt)
    {
        primType = pt;
    }

    TyIdx GetTypeIndex() const
    {
        return tyIdx;
    }
    void SetTypeIndex(TyIdx idx)
    {
        tyIdx = idx;
    }

    MIRTypeKind GetKind() const
    {
        return typeKind;
    }
    void SetMIRTypeKind(MIRTypeKind kind)
    {
        typeKind = kind;
    }

    bool IsNameIsLocal() const
    {
        return nameIsLocal;
    }
    void SetNameIsLocal(bool flag)
    {
        nameIsLocal = flag;
    }

    GStrIdx GetNameStrIdx() const
    {
        return nameStrIdx;
    }
    void SetNameStrIdx(GStrIdx strIdx)
    {
        nameStrIdx = strIdx;
    }
    void SetNameStrIdxItem(uint32 idx)
    {
        nameStrIdx.reset(idx);
    }

    virtual size_t GetSize() const
    {
        return GetPrimTypeSize(primType);
    }

    virtual uint32 GetAlign() const
    {
        return GetPrimTypeSize(primType);
    }

    virtual uint32 GetUnadjustedAlign() const
    {
        return GetPrimTypeSize(primType);
    }

    virtual bool HasVolatileField() const
    {
        return false;
    }

    virtual bool HasTypeParam() const
    {
        return false;
    }

    bool IsVolatile(int fieldID) const;

    bool IsMIRPtrType() const
    {
        return typeKind == kTypePointer;
    }

    bool IsMIRArrayType() const
    {
        return typeKind == kTypeArray;
    }

    bool IsMIRFuncType() const
    {
        return typeKind == kTypeFunction;
    }

    bool IsScalarType() const
    {
        return typeKind == kTypeScalar;
    }

    bool IsMIRTypeByName() const
    {
        return typeKind == kTypeByName;
    }

    virtual size_t GetHashIndex() const
    {
        constexpr uint8 idxShift = 2;
        return ((static_cast<uint32>(primType) << idxShift) + (typeKind << kShiftNumOfTypeKind)) % kTypeHashLength;
    }

protected:
    MIRTypeKind typeKind;
    PrimType primType;
    bool nameIsLocal = false;  // needed when printing the type name
    TyIdx tyIdx {0};
    GStrIdx nameStrIdx {0};  // name in global string table
};

class MIRPtrType : public MIRType {
public:
    explicit MIRPtrType(TyIdx pTyIdx) : MIRType(kTypePointer, PTY_ptr), pointedTyIdx(pTyIdx) {}

    MIRPtrType(TyIdx pTyIdx, PrimType pty) : MIRType(kTypePointer, pty), pointedTyIdx(pTyIdx) {}

    MIRPtrType(PrimType primType, GStrIdx strIdx) : MIRType(kTypePointer, primType, strIdx), pointedTyIdx(0) {}

    ~MIRPtrType() override = default;

    MIRType *CopyMIRTypeNode() const override
    {
        return new MIRPtrType(*this);
    }

    MIRType *GetPointedType() const;

    TyIdx GetPointedTyIdx() const
    {
        return pointedTyIdx;
    }
    void SetPointedTyIdx(TyIdx idx)
    {
        pointedTyIdx = idx;
    }

    TypeAttrs &GetTypeAttrs()
    {
        return typeAttrs;
    }

    const TypeAttrs &GetTypeAttrs() const
    {
        return typeAttrs;
    }

    void SetTypeAttrs(const TypeAttrs &attrs)
    {
        typeAttrs = attrs;
    }

    bool EqualTo(const MIRType &type) const override;
    bool HasTypeParam() const override;
    bool IsPointedTypeVolatile(int fieldID) const;
#ifdef ARK_LITECG_DEBUG
    void Dump(int indent, bool dontUseName = false) const override;
#endif
    size_t GetSize() const override;
    uint32 GetAlign() const override;
    size_t GetHashIndex() const override
    {
        constexpr uint8 idxShift = 4;
        constexpr uint8 attrShift = 3;
        size_t hIdx = (static_cast<size_t>(pointedTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
        hIdx += (typeAttrs.GetAttrFlag() << attrShift) + typeAttrs.GetAlignValue();
        return hIdx % kTypeHashLength;
    }
    bool IsFunctionPtr() const
    {
        MIRType *pointedType = GetPointedType();
        if (pointedType->GetKind() == kTypeFunction) {
            return true;
        }
        if (pointedType->GetKind() == kTypePointer) {
            MIRPtrType *pointedPtrType = static_cast<MIRPtrType *>(pointedType);
            return pointedPtrType->GetPointedType()->GetKind() == kTypeFunction;
        }
        return false;
    }

    MIRFuncType *GetPointedFuncType() const;

#ifdef ARK_LITECG_DEBUG
    bool PointsToConstString() const;
#endif

private:
    TyIdx pointedTyIdx;
    TypeAttrs typeAttrs;
};

class MIRArrayType : public MIRType {
public:
    MIRArrayType() : MIRType(kTypeArray) {}
    explicit MIRArrayType(GStrIdx strIdx) : MIRType(kTypeArray, strIdx) {}

    MIRArrayType(TyIdx eTyIdx, const std::vector<uint64> &sizeArray)
        : MIRType(kTypeArray), eTyIdx(eTyIdx), dim(sizeArray.size())
    {
        for (size_t i = 0; i < kMaxArrayDim; ++i) {
            this->sizeArray[i] = (i < dim) ? sizeArray[i] : 0;
        }
    }

    MIRArrayType(const MIRArrayType &pat) = default;
    MIRArrayType &operator=(const MIRArrayType &p) = default;
    ~MIRArrayType() override = default;

    TyIdx GetElemTyIdx() const
    {
        return eTyIdx;
    }
    void SetElemTyIdx(TyIdx idx)
    {
        eTyIdx = idx;
    }

    uint64 GetSizeArrayItem(uint32 n) const
    {
        CHECK_FATAL((n >= 0 && n < kMaxArrayDim), "out of bound of array!");
        return sizeArray[n];
    }
    void SetSizeArrayItem(uint32 idx, uint64 value)
    {
        CHECK_FATAL((idx >= 0 && idx < kMaxArrayDim), "out of bound of array!");
        sizeArray[idx] = value;
    }

    bool IsIncompleteArray() const
    {
        return typeAttrs.GetAttr(ATTR_incomplete_array);
    }

    bool EqualTo(const MIRType &type) const override;

    uint16 GetDim() const
    {
        return dim;
    }
    void SetDim(uint16 newDim)
    {
        this->dim = newDim;
    }

    const TypeAttrs &GetTypeAttrs() const
    {
        return typeAttrs;
    }

    TypeAttrs &GetTypeAttrs()
    {
        return typeAttrs;
    }

    void SetTypeAttrs(const TypeAttrs &attrs)
    {
        typeAttrs = attrs;
    }

    MIRType *GetElemType() const;

    MIRType *CopyMIRTypeNode() const override
    {
        return new MIRArrayType(*this);
    }

    bool HasTypeParam() const override
    {
        return GetElemType()->HasTypeParam();
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(int indent, bool dontUseName) const override;
#endif
    size_t GetSize() const override;
    uint32 GetAlign() const override;

    size_t GetHashIndex() const override
    {
        constexpr uint8 idxShift = 2;
        size_t hIdx = (static_cast<size_t>(eTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
        for (size_t i = 0; i < dim; ++i) {
            CHECK_FATAL(i < kMaxArrayDim, "array index out of range");
            hIdx += (sizeArray[i] << i);
        }
        constexpr uint8 attrShift = 3;
        hIdx += (typeAttrs.GetAttrFlag() << attrShift) + typeAttrs.GetAlignValue();
        return hIdx % kTypeHashLength;
    }

#ifdef ARK_LITECG_DEBUG
    size_t ElemNumber() const;
#endif

private:
    TyIdx eTyIdx {0};
    uint16 dim = 0;
    std::array<uint64, kMaxArrayDim> sizeArray {{0}};
    TypeAttrs typeAttrs;
    mutable size_t size = kInvalidSize;
};

using TyidxFuncAttrPair = std::pair<TyIdx, FuncAttrs>;
using MethodPair = std::pair<StIdx, TyidxFuncAttrPair>;
using MethodVector = std::vector<MethodPair>;
using MethodPtrVector = std::vector<MethodPair *>;
using MIREncodedArray = std::vector<EncodedValue>;
class GenericDeclare;
class AnnotationType;
class GenericType;

class MIRFuncType : public MIRType {
public:
    MIRFuncType() : MIRType(kTypeFunction, PTY_ptr) {}

    explicit MIRFuncType(const GStrIdx &strIdx) : MIRType(kTypeFunction, PTY_ptr, strIdx) {}

    MIRFuncType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecTy, const std::vector<TypeAttrs> &vecAt,
                const TypeAttrs &retAttrsIn)
        : MIRType(kTypeFunction, PTY_ptr),
          retTyIdx(retTyIdx),
          paramTypeList(vecTy),
          paramAttrsList(vecAt),
          retAttrs(retAttrsIn)
    {
    }

    ~MIRFuncType() override = default;

    bool EqualTo(const MIRType &type) const override;
    MIRType *CopyMIRTypeNode() const override
    {
        return new MIRFuncType(*this);
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(int indent, bool dontUseName = false) const override;
#endif
    size_t GetSize() const override
    {
        return 0;
    }  // size unknown

    TyIdx GetRetTyIdx() const
    {
        return retTyIdx;
    }

    void SetRetTyIdx(TyIdx idx)
    {
        retTyIdx = idx;
    }

    const std::vector<TyIdx> &GetParamTypeList() const
    {
        return paramTypeList;
    }

    std::vector<TyIdx> &GetParamTypeList()
    {
        return paramTypeList;
    }

    TyIdx GetNthParamType(size_t i) const
    {
        DEBUG_ASSERT(i < paramTypeList.size(), "array index out of range");
        return paramTypeList[i];
    }

    void SetParamTypeList(const std::vector<TyIdx> &list)
    {
        paramTypeList.clear();
        (void)paramTypeList.insert(paramTypeList.begin(), list.begin(), list.end());
    }

    const std::vector<TypeAttrs> &GetParamAttrsList() const
    {
        return paramAttrsList;
    }

    std::vector<TypeAttrs> &GetParamAttrsList()
    {
        return paramAttrsList;
    }

    const TypeAttrs &GetNthParamAttrs(size_t i) const
    {
        DEBUG_ASSERT(i < paramAttrsList.size(), "array index out of range");
        return paramAttrsList[i];
    }

    TypeAttrs &GetNthParamAttrs(size_t i)
    {
        DEBUG_ASSERT(i < paramAttrsList.size(), "array index out of range");
        return paramAttrsList[i];
    }

    void SetParamAttrsList(const std::vector<TypeAttrs> &list)
    {
        paramAttrsList.clear();
        (void)paramAttrsList.insert(paramAttrsList.begin(), list.begin(), list.end());
    }

    void SetNthParamAttrs(size_t i, const TypeAttrs &attrs)
    {
        DEBUG_ASSERT(i < paramAttrsList.size(), "array index out of range");
        paramAttrsList[i] = attrs;
    }

    bool IsVarargs() const
    {
        return funcAttrs.GetAttr(FUNCATTR_varargs);
    }

    void SetVarArgs()
    {
        funcAttrs.SetAttr(FUNCATTR_varargs);
    }

    bool FirstArgReturn() const
    {
        return funcAttrs.GetAttr(FUNCATTR_firstarg_return);
    }

    void SetFirstArgReturn()
    {
        funcAttrs.SetAttr(FUNCATTR_firstarg_return);
    }

    const TypeAttrs &GetRetAttrs() const
    {
        return retAttrs;
    }

    TypeAttrs &GetRetAttrs()
    {
        return retAttrs;
    }

    void SetRetAttrs(const TypeAttrs &attrs)
    {
        retAttrs = attrs;
    }

    size_t GetHashIndex() const override
    {
        constexpr uint8 idxShift = 6;
        size_t hIdx = (static_cast<size_t>(retTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
        size_t size = paramTypeList.size();
        hIdx += (size ? (static_cast<size_t>(paramTypeList[0]) + size) : 0) << 4;  // shift bit is 4
        return hIdx % kTypeHashLength;
    }

public:
    FuncAttrs funcAttrs;

private:
    TyIdx retTyIdx {0};
    std::vector<TyIdx> paramTypeList;
    std::vector<TypeAttrs> paramAttrsList;
    TypeAttrs retAttrs;
};

class MIRTypeByName : public MIRType {
    // use nameStrIdx to store the name for both local and global
public:
    explicit MIRTypeByName(GStrIdx gStrIdx) : MIRType(kTypeByName, PTY_void)
    {
        nameStrIdx = gStrIdx;
    }

    ~MIRTypeByName() override = default;

    MIRType *CopyMIRTypeNode() const override
    {
        return new MIRTypeByName(*this);
    }

    bool EqualTo(const MIRType &type) const override;
#ifdef ARK_LITECG_DEBUG
    void Dump(int indent, bool dontUseName = false) const override;
#endif
    size_t GetSize() const override
    {
        return 0;
    }  // size unknown

    size_t GetHashIndex() const override
    {
        constexpr uint8 idxShift = 2;
        return ((static_cast<size_t>(nameStrIdx) << idxShift) + nameIsLocal + (typeKind << kShiftNumOfTypeKind)) %
               kTypeHashLength;
    }
};
// aarch64 specific
bool IsHomogeneousAggregates(const MIRType &ty, PrimType &primType, size_t &elemNum, bool firstDepth = true);
bool IsParamStructCopyToMemory(const MIRType &ty);
#endif  // MIR_FEATURE_FULL
}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_TYPE
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_TYPE_H
