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

#ifndef MAPLE_IR_INCLUDE_MIR_CONST_H
#define MAPLE_IR_INCLUDE_MIR_CONST_H
#include <math.h>
#include "mir_type.h"
#include "mpl_int_val.h"

namespace maple {
class MIRConst;  // circular dependency exists, no other choice
using MIRConstPtr = MIRConst *;
#if MIR_FEATURE_FULL
class MIRSymbol;  // circular dependency exists, no other choice
enum MIRConstKind {
    kConstInvalid,
    kConstInt,
    kConstAddrof,
    kConstAddrofFunc,
    kConstLblConst,
    kConstStrConst,
    kConstStr16Const,
    kConstFloatConst,
    kConstDoubleConst,
    kConstFloat128Const,
    kConstAggConst,
    kConstStConst
};

class MIRConst {
public:
    explicit MIRConst(MIRType &type, MIRConstKind constKind = kConstInvalid) : type(&type), kind(constKind) {}

    virtual ~MIRConst() = default;
#ifdef ARK_LITECG_DEBUG
    virtual void Dump(const MIRSymbolTable *localSymTab = nullptr) const
    {
        (void)localSymTab;
    }
#endif

    uint32 GetFieldId() const
    {
        return fieldID;
    }

    void SetFieldId(uint32 fieldIdx)
    {
        DoSetFieldId(fieldIdx);
    }

    virtual bool IsZero() const
    {
        return false;
    }

    virtual bool IsOne() const
    {
        return false;
    }

    virtual bool IsMagicNum() const
    {
        return false;
    }

    // NO OP
    virtual void Neg() {}

    virtual bool operator==(const MIRConst &rhs) const
    {
        return &rhs == this;
    }

    virtual MIRConst *Clone(MemPool &memPool) const = 0;

    MIRConstKind GetKind() const
    {
        return kind;
    }

    MIRType &GetType()
    {
        return *type;
    }

    const MIRType &GetType() const
    {
        return *type;
    }

    void SetType(MIRType &t)
    {
        type = &t;
    }

protected:
    uint32 fieldID = 0;

private:
    MIRType *type;
    MIRConstKind kind;
    virtual void DoSetFieldId(uint32 fieldIdx)
    {
        DEBUG_ASSERT(kind != kConstInt, "must be");
        fieldID = fieldIdx;
    }
};

class MIRIntConst : public MIRConst {
public:
    MIRIntConst(uint64 val, MIRType &type) : MIRConst(type, kConstInt), value(val, type.GetPrimType()) {}

    MIRIntConst(const IntVal &val, MIRType &type) : MIRConst(type, kConstInt), value(val)
    {
        [[maybe_unused]] PrimType pType = type.GetPrimType();
        DEBUG_ASSERT(
            IsPrimitiveInteger(pType) && GetPrimTypeActualBitSize(pType) <= value.GetBitWidth(),
            "Constant is tried to be constructed with non-integral type or bit-width is not appropriate for it");
    }

#ifdef ARK_LITECG_DEBUG
    /// @return number of used bits in the value
    uint8 GetActualBitWidth() const;
#endif

    void Trunc(uint8 width)
    {
        value.TruncInPlace(width);
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool IsNegative() const
    {
        return value.IsSigned() && value.GetSignBit();
    }

    bool IsPositive() const
    {
        return !IsNegative() && value != 0;
    }

    bool IsZero() const override
    {
        return value == 0;
    }

    bool IsOne() const override
    {
        return value == 1;
    }

    void Neg() override
    {
        value = -value;
    }

    const IntVal &GetValue() const
    {
        return value;
    }

    int64 GetExtValue(uint8 size = 0) const
    {
        return value.GetExtValue(size);
    }

    int64 GetSXTValue(uint8 size = 0) const
    {
        return value.GetSXTValue(size);
    }

    uint64 GetZXTValue(uint8 size = 0) const
    {
        return value.GetZXTValue(size);
    }

    void SetValue(int64 val) const
    {
        (void)val;
        CHECK_FATAL(false, "Can't Use This Interface in This Object");
    }

    bool operator==(const MIRConst &rhs) const override;

    MIRIntConst *Clone([[maybe_unused]] MemPool &memPool) const override
    {
        CHECK_FATAL(false, "Can't Use This Interface in This Object");
    }

private:
    IntVal value;

    void DoSetFieldId(uint32 fieldIdx) override
    {
        DEBUG_ASSERT(false, "Can't Use This Interface in This Object");
        (void)fieldIdx;
    }
};

class MIRAddrofConst : public MIRConst {
public:
    MIRAddrofConst(StIdx sy, FieldID fi, MIRType &ty) : MIRConst(ty, kConstAddrof), stIdx(sy), fldID(fi), offset(0) {}

    MIRAddrofConst(StIdx sy, FieldID fi, MIRType &ty, int32 ofst)
        : MIRConst(ty, kConstAddrof), stIdx(sy), fldID(fi), offset(ofst)
    {
    }

    ~MIRAddrofConst() = default;

    StIdx GetSymbolIndex() const
    {
        return stIdx;
    }

    void SetSymbolIndex(StIdx idx)
    {
        stIdx = idx;
    }

    FieldID GetFieldID() const
    {
        return fldID;
    }

    int32 GetOffset() const
    {
        return offset;
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool operator==(const MIRConst &rhs) const override;

    MIRAddrofConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRAddrofConst>(*this);
    }

private:
    StIdx stIdx;
    FieldID fldID;
    int32 offset;
};

class MIRAddroffuncConst : public MIRConst {
public:
    MIRAddroffuncConst(PUIdx idx, MIRType &ty) : MIRConst(ty, kConstAddrofFunc), puIdx(idx) {}

    ~MIRAddroffuncConst() = default;

    PUIdx GetValue() const
    {
        return puIdx;
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool operator==(const MIRConst &rhs) const override;

    MIRAddroffuncConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRAddroffuncConst>(*this);
    }

private:
    PUIdx puIdx;
};

class MIRLblConst : public MIRConst {
public:
    MIRLblConst(LabelIdx val, PUIdx pidx, MIRType &type) : MIRConst(type, kConstLblConst), value(val), puIdx(pidx) {}

    ~MIRLblConst() = default;
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool operator==(const MIRConst &rhs) const override;

    MIRLblConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRLblConst>(*this);
    }

    LabelIdx GetValue() const
    {
        return value;
    }

    PUIdx GetPUIdx() const
    {
        return puIdx;
    }

private:
    LabelIdx value;
    PUIdx puIdx;
};

class MIRStrConst : public MIRConst {
public:
    MIRStrConst(UStrIdx val, MIRType &type) : MIRConst(type, kConstStrConst), value(val) {}

    MIRStrConst(const std::string &str, MIRType &type);

    ~MIRStrConst() = default;
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool operator==(const MIRConst &rhs) const override;

    MIRStrConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRStrConst>(*this);
    }

    UStrIdx GetValue() const
    {
        return value;
    }

    static PrimType GetPrimType()
    {
        return kPrimType;
    }

private:
    UStrIdx value;
    static const PrimType kPrimType = PTY_ptr;
};

class MIRStr16Const : public MIRConst {
public:
    MIRStr16Const(const U16StrIdx &val, MIRType &type) : MIRConst(type, kConstStr16Const), value(val) {}

    MIRStr16Const(const std::u16string &str, MIRType &type);
    ~MIRStr16Const() = default;

    static PrimType GetPrimType()
    {
        return kPrimType;
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool operator==(const MIRConst &rhs) const override;

    MIRStr16Const *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRStr16Const>(*this);
    }

    U16StrIdx GetValue() const
    {
        return value;
    }

private:
    static const PrimType kPrimType = PTY_ptr;
    U16StrIdx value;
};

class MIRFloatConst : public MIRConst {
public:
    using value_type = float;
    MIRFloatConst(float val, MIRType &type) : MIRConst(type, kConstFloatConst)
    {
        value.floatValue = val;
    }

    ~MIRFloatConst() = default;

    void SetFloatValue(float fvalue)
    {
        value.floatValue = fvalue;
    }

    value_type GetFloatValue() const
    {
        return value.floatValue;
    }

    static PrimType GetPrimType()
    {
        return kPrimType;
    }

    int32 GetIntValue() const
    {
        return value.intValue;
    }

    value_type GetValue() const
    {
        return GetFloatValue();
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool IsZero() const override
    {
        return fabs(value.floatValue) <= 1e-6;
    }

    bool IsGeZero() const
    {
        return value.floatValue >= 0;
    }

    bool IsNeg() const
    {
        return ((static_cast<uint32>(value.intValue) & 0x80000000) == 0x80000000);
    }

    bool IsOne() const override
    {
        return fabs(value.floatValue - 1) <= 1e-6;
    };
    bool IsAllBitsOne() const
    {
        return fabs(value.floatValue + 1) <= 1e-6;
    };
    void Neg() override
    {
        value.floatValue = -value.floatValue;
    }

    bool operator==(const MIRConst &rhs) const override;

    MIRFloatConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRFloatConst>(*this);
    }

private:
    static const PrimType kPrimType = PTY_f32;
    union {
        value_type floatValue;
        int32 intValue;
    } value;
};

class MIRDoubleConst : public MIRConst {
public:
    using value_type = double;
    MIRDoubleConst(double val, MIRType &type) : MIRConst(type, kConstDoubleConst)
    {
        value.dValue = val;
    }

    ~MIRDoubleConst() = default;

    uint32 GetIntLow32() const
    {
        auto unsignVal = static_cast<uint64>(value.intValue);
        return static_cast<uint32>(unsignVal & 0xffffffff);
    }

    uint32 GetIntHigh32() const
    {
        auto unsignVal = static_cast<uint64>(value.intValue);
        return static_cast<uint32>((unsignVal & 0xffffffff00000000) >> k32BitSize);
    }

    int64 GetIntValue() const
    {
        return value.intValue;
    }

    value_type GetValue() const
    {
        return value.dValue;
    }

    static PrimType GetPrimType()
    {
        return kPrimType;
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool IsZero() const override
    {
        return fabs(value.dValue) <= 1e-15;
    }

    bool IsGeZero() const
    {
        return value.dValue >= 0;
    }

    bool IsNeg() const
    {
        return ((static_cast<uint64>(value.intValue) & 0x8000000000000000LL) == 0x8000000000000000LL);
    }

    bool IsOne() const override
    {
        return fabs(value.dValue - 1) <= 1e-15;
    };
    bool IsAllBitsOne() const
    {
        return fabs(value.dValue + 1) <= 1e-15;
    };
    void Neg() override
    {
        value.dValue = -value.dValue;
    }

    bool operator==(const MIRConst &rhs) const override;

    MIRDoubleConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRDoubleConst>(*this);
    }

private:
    static const PrimType kPrimType = PTY_f64;
    union {
        value_type dValue;
        int64 intValue;
    } value;
};


class MIRAggConst : public MIRConst {
public:
    MIRAggConst(MIRModule &mod, MIRType &type)
        : MIRConst(type, kConstAggConst),
          constVec(mod.GetMPAllocator().Adapter()),
          fieldIdVec(mod.GetMPAllocator().Adapter())
    {
    }

    ~MIRAggConst() = default;

    MIRConst *GetAggConstElement(unsigned int fieldId)
    {
        for (size_t i = 0; i < fieldIdVec.size(); ++i) {
            if (fieldId == fieldIdVec[i]) {
                return constVec[i];
            }
        }
        return nullptr;
    }

    void SetFieldIdOfElement(uint32 index, uint32 fieldId)
    {
        DEBUG_ASSERT(index < fieldIdVec.size(), "index out of range");
        fieldIdVec[index] = fieldId;
    }

    const MapleVector<MIRConst *> &GetConstVec() const
    {
        return constVec;
    }

    MapleVector<MIRConst *> &GetConstVec()
    {
        return constVec;
    }

    const MIRConstPtr &GetConstVecItem(size_t index) const
    {
        CHECK_FATAL(index < constVec.size(), "index out of range");
        return constVec[index];
    }

    MIRConstPtr &GetConstVecItem(size_t index)
    {
        CHECK_FATAL(index < constVec.size(), "index out of range");
        return constVec[index];
    }

    void SetConstVecItem(size_t index, MIRConst &st)
    {
        CHECK_FATAL(index < constVec.size(), "index out of range");
        constVec[index] = &st;
    }

    uint32 GetFieldIdItem(size_t index) const
    {
        DEBUG_ASSERT(index < fieldIdVec.size(), "index out of range");
        return fieldIdVec[index];
    }

    void SetItem(uint32 index, MIRConst *mirConst, uint32 fieldId)
    {
        CHECK_FATAL(index < constVec.size(), "index out of range");
        constVec[index] = mirConst;
        fieldIdVec[index] = fieldId;
    }

    void AddItem(MIRConst *mirConst, uint32 fieldId)
    {
        constVec.push_back(mirConst);
        fieldIdVec.push_back(fieldId);
    }

    void PushBack(MIRConst *elem)
    {
        AddItem(elem, 0);
    }
#ifdef ARK_LITECG_DEBUG
    void Dump(const MIRSymbolTable *localSymTab) const override;
#endif
    bool operator==(const MIRConst &rhs) const override;

    MIRAggConst *Clone(MemPool &memPool) const override
    {
        return memPool.New<MIRAggConst>(*this);
    }

private:
    MapleVector<MIRConst *> constVec;
    MapleVector<uint32> fieldIdVec;
};

// the const has one or more symbols
class MIRStConst : public MIRConst {
public:
    MIRStConst(MIRModule &mod, MIRType &type)
        : MIRConst(type, kConstStConst),
          stVec(mod.GetMPAllocator().Adapter()),
          stOffsetVec(mod.GetMPAllocator().Adapter())
    {
    }

    const MapleVector<MIRSymbol *> &GetStVec() const
    {
        return stVec;
    }
    void PushbackSymbolToSt(MIRSymbol *sym)
    {
        stVec.push_back(sym);
    }

    MIRSymbol *GetStVecItem(size_t index)
    {
        CHECK_FATAL(index < stVec.size(), "array index out of range");
        return stVec[index];
    }

    const MapleVector<uint32> &GetStOffsetVec() const
    {
        return stOffsetVec;
    }
    void PushbackOffsetToSt(uint32 offset)
    {
        stOffsetVec.push_back(offset);
    }

    uint32 GetStOffsetVecItem(size_t index) const
    {
        CHECK_FATAL(index < stOffsetVec.size(), "array index out of range");
        return stOffsetVec[index];
    }

    MIRStConst *Clone(MemPool &memPool) const override
    {
        auto *res = memPool.New<MIRStConst>(*this);
        return res;
    }

    ~MIRStConst() = default;

private:
    MapleVector<MIRSymbol *> stVec;   // symbols that in the st const
    MapleVector<uint32> stOffsetVec;  // symbols offset
};
#endif  // MIR_FEATURE_FULL

bool IsDivSafe(const MIRIntConst &dividend, const MIRIntConst &divisor, PrimType pType);

}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_CONST
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_CONST_H
