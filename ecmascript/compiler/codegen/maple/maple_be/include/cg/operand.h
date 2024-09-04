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

#ifndef MAPLEBE_INCLUDE_CG_OPERAND_H
#define MAPLEBE_INCLUDE_CG_OPERAND_H

#include "becommon.h"
#include "cg_option.h"
#include "aarch64/aarch64_imm_valid.h"
#include "visitor_common.h"

/* maple_ir */
#include "mir_symbol.h"
#include "prim_types.h" /* for PrimType */
#include "types_def.h"  /* need uint8 etc */

/* Mempool */
#include "memlayout.h"
#include "mempool_allocator.h" /* MapleList */

namespace maplebe {
class OpndDesc;
class Emitter;
class FuncEmitInfo;

bool IsMoveWidableImmediate(uint64 val, uint32 bitLen);
bool BetterUseMOVZ(uint64 val);

using MOperator = uint32;
enum RegType : maple::uint8 {
    kRegTyUndef,
    kRegTyInt,
    kRegTyFloat,
    kRegTyCc,
    kRegTyX87,
    kRegTyVary,
    kRegTyFpsc,
    kRegTyIndex,
    kRegTyLast,
};

class Operand {
public:
    enum OperandType : uint8 {
        kOpdRegister,
        kOpdImmediate,
        kOpdMem,
        kOpdCond, /*  for condition code */
        kOpdPhi,  /*  for phi operand */
        kOpdFPImmediate,
        kOpdStImmediate, /* use the symbol name as the offset */
        kOpdOffset,      /* for the offset operand in MemOperand */
        kOpdBBAddress,
        kOpdList,     /*  for list operand */
        kOpdShift,    /*  for imm shift operand */
        kOpdRegShift, /*  for reg shift operand */
        kOpdExtend,   /*  for extend operand */
        kOpdString,   /*  for comments */
        kOpdUndef
    };

    Operand(OperandType type, uint32 size) : opndKind(type), size(size) {}
    virtual ~Operand() = default;

    uint32 GetSize() const
    {
        return size;
    }

    void SetSize(uint32 sz)
    {
        size = sz;
    }

    bool IsReference() const
    {
        return isReference;
    }

    void SetIsReference(bool isRef)
    {
        isReference = isRef;
    }

    OperandType GetKind() const
    {
        return opndKind;
    }

    bool IsIntImmediate() const
    {
        return opndKind == kOpdImmediate || opndKind == kOpdOffset;
    }

    bool IsConstImmediate() const
    {
        return opndKind == kOpdImmediate || opndKind == kOpdOffset || opndKind == kOpdFPImmediate;
    }

    bool IsOfstImmediate() const
    {
        return opndKind == kOpdOffset;
    }

    bool IsStImmediate() const
    {
        return opndKind == kOpdStImmediate;
    }

    bool IsImmediate() const
    {
        return (kOpdFPImmediate <= opndKind && opndKind <= kOpdOffset) || opndKind == kOpdImmediate;
    }

    bool IsRegister() const
    {
        return opndKind == kOpdRegister;
    }

    bool IsList() const
    {
        return opndKind == kOpdList;
    }

    bool IsPhi() const
    {
        return opndKind == kOpdPhi;
    }

    bool IsMemoryAccessOperand() const
    {
        return opndKind == kOpdMem;
    }

    bool IsLabel() const
    {
        return opndKind == kOpdBBAddress;
    }

    virtual bool IsZeroRegister() const
    {
        return false;
    };

    bool IsConditionCode() const
    {
        return opndKind == kOpdCond;
    }

    bool IsOpdShift() const
    {
        return opndKind == kOpdShift;
    }

    bool IsRegShift() const
    {
        return opndKind == kOpdRegShift;
    }

    bool IsOpdExtend() const
    {
        return opndKind == kOpdExtend;
    }

    virtual bool IsLabelOpnd() const
    {
        return false;
    }

    virtual bool IsFuncNameOpnd() const
    {
        return false;
    }

    virtual bool IsCommentOpnd() const
    {
        return false;
    }

    // Custom deep copy
    virtual Operand *CloneTree(MapleAllocator &allocator) const = 0;
    virtual Operand *Clone(MemPool &memPool) const = 0;

    /*
     * A simple implementation here.
     * Each subclass can elaborate on demand.
     */
    virtual bool Equals(Operand &op) const
    {
        return BasicEquals(op) && (&op == this);
    }

    bool BasicEquals(const Operand &op) const
    {
        return opndKind == op.GetKind() && size == op.GetSize();
    }

    virtual void Dump() const = 0;

    virtual bool Less(const Operand &right) const = 0;

    virtual void Accept(OperandVisitorBase &v) = 0;

protected:
    OperandType opndKind; /* operand type */
    uint32 size;          /* size in bits */
    uint64 flag = 0;      /* operand property*/
    bool isReference = false;
};

/* RegOperand */
enum RegOperandState : uint32 { kRegOpndNone = 0, kRegOpndSetLow32 = 0x1, kRegOpndSetHigh32 = 0x2 };

template <typename VisitableTy>
class OperandVisitable : public Operand {
public:
    using Operand::Operand;
    void Accept(OperandVisitorBase &v) override
    {
        if (OperandVisitor<VisitableTy> *typeV = dynamic_cast<OperandVisitor<VisitableTy> *>(&v)) {
            typeV->Visit(static_cast<VisitableTy *>(this));
        } else {
            /* the type which has no implements */
        }
    }
};

class RegOperand : public OperandVisitable<RegOperand> {
public:
    RegOperand(regno_t regNum, uint32 size, RegType type, uint32 flg = 0)
        : OperandVisitable(kOpdRegister, size), regNO(regNum), regType(type), validBitsNum(size), regFlag(flg)
    {
    }

    ~RegOperand() override = default;
    using OperandVisitable<RegOperand>::OperandVisitable;

    RegOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<RegOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<RegOperand>(*this);
    }

    void SetValidBitsNum(uint32 validNum)
    {
        validBitsNum = validNum;
    }

    uint32 GetValidBitsNum() const
    {
        return validBitsNum;
    }

    bool IsOfIntClass() const
    {
        return regType == kRegTyInt;
    }

    bool IsOfFloatOrSIMDClass() const
    {
        return regType == kRegTyFloat;
    }

    bool IsOfCC() const
    {
        return regType == kRegTyCc;
    }

    bool IsOfVary() const
    {
        return regType == kRegTyVary;
    }

    RegType GetRegisterType() const
    {
        return regType;
    }

    void SetRegisterType(RegType newTy)
    {
        regType = newTy;
    }

    virtual bool IsBBLocalReg() const
    {
        return isBBLocal;
    }

    void SetRegNotBBLocal()
    {
        isBBLocal = false;
    }

    regno_t GetRegisterNumber() const
    {
        return regNO;
    }

    void SetRegisterNumber(regno_t regNum)
    {
        regNO = regNum;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "reg ";
        LogInfo::MapleLogger() << "size : " << GetSize();
        LogInfo::MapleLogger() << " NO_" << GetRegisterNumber();
        if (IsReference()) {
            LogInfo::MapleLogger() << " is_ref";
        }
    };

    bool Less(const Operand &right) const override
    {
        if (&right == this) {
            return false;
        }

        /* For different type. */
        if (opndKind != right.GetKind()) {
            return opndKind < right.GetKind();
        }

        auto *rightOpnd = static_cast<const RegOperand *>(&right);

        /* The same type. */
        return regNO < rightOpnd->regNO;
    }

    bool Less(const RegOperand &right) const
    {
        return regNO < right.regNO;
    }

    bool RegNumEqual(const RegOperand &right) const
    {
        return regNO == right.GetRegisterNumber();
    }

    int32 RegCompare(const RegOperand &right) const
    {
        return (regNO - right.GetRegisterNumber());
    }

    bool Equals(Operand &operand) const override
    {
        if (!operand.IsRegister()) {
            return false;
        }
        auto &op = static_cast<RegOperand &>(operand);
        if (&op == this) {
            return true;
        }
        return (BasicEquals(op) && regNO == op.GetRegisterNumber() && regType == op.GetRegisterType() &&
                IsBBLocalReg() == op.IsBBLocalReg());
    }

    static bool IsSameRegNO(const Operand &firstOpnd, const Operand &secondOpnd)
    {
        if (!firstOpnd.IsRegister() || !secondOpnd.IsRegister()) {
            return false;
        }
        auto &firstReg = static_cast<const RegOperand &>(firstOpnd);
        auto &secondReg = static_cast<const RegOperand &>(secondOpnd);
        return firstReg.RegNumEqual(secondReg);
    }

    static bool IsSameReg(const Operand &firstOpnd, const Operand &secondOpnd)
    {
        if (firstOpnd.GetSize() != secondOpnd.GetSize()) {
            return false;
        }
        return IsSameRegNO(firstOpnd, secondOpnd);
    }

    void SetOpndSSAForm()
    {
        isSSAForm = true;
    }

    void SetOpndOutOfSSAForm()
    {
        isSSAForm = false;
    }

    bool IsSSAForm() const
    {
        return isSSAForm;
    }

    void SetRefField(bool newIsRefField)
    {
        isRefField = newIsRefField;
    }

    bool IsPhysicalRegister() const
    {
        constexpr uint32 maxPhysicalRegisterNumber = 100;
        return GetRegisterNumber() > 0 && GetRegisterNumber() < maxPhysicalRegisterNumber && !IsOfCC();
    }

    bool IsVirtualRegister() const
    {
        return !IsPhysicalRegister();
    }

    bool IsBBLocalVReg() const
    {
        return IsVirtualRegister() && IsBBLocalReg();
    }

    void SetHigh8Bit()
    {
        isHigh8Bit = true;
    }

    bool IsHigh8Bit()
    {
        return isHigh8Bit;
    }

    void SetBaseRefOpnd(RegOperand &regOpnd)
    {
        baseRefOpnd = &regOpnd;
    }

    const RegOperand *GetBaseRefOpnd() const
    {
        return baseRefOpnd;
    }

    RegOperand *GetBaseRefOpnd()
    {
        return baseRefOpnd;
    }

    bool operator==(const RegOperand &o) const;

    bool operator<(const RegOperand &o) const;

protected:
    regno_t regNO;
    RegType regType;

    /*
     * used for EBO(-O1), it can recognize the registers whose use and def are in
     * different BB. It is true by default. Sometime it should be false such as
     * when handle intrinsiccall for target
     * aarch64(AArch64CGFunc::SelectIntrinsicCall).
     */
    bool isBBLocal = true;
    uint32 validBitsNum;
    /* use for SSA analysis */
    bool isSSAForm = false;
    bool isRefField = false;
    uint32 regFlag = 0;
    int16 vecLane = -1;        /* -1 for whole reg, 0 to 15 to specify each lane one at a time */
    uint16 vecLaneSize = 0;    /* Number of lanes */
    uint64 vecElementSize = 0; /* size of vector element in each lane */
    bool if64Vec = false;      /* operand returning 64x1's int value in FP/Simd register */
    bool isHigh8Bit = false;
    RegOperand *baseRefOpnd = nullptr;
}; /* class RegOperand */

enum VaryType : uint8 {
    kNotVary = 0,
    kUnAdjustVary,
    kAdjustVary,
};

class ImmOperand : public OperandVisitable<ImmOperand> {
public:
    ImmOperand(int64 val, uint32 size, bool isSigned, VaryType isVar = kNotVary, bool isFloat = false)
        : OperandVisitable(kOpdImmediate, size), value(val), isSigned(isSigned), isVary(isVar), isFmov(isFloat)
    {
    }
    ImmOperand(OperandType type, int64 val, uint32 size, bool isSigned, VaryType isVar = kNotVary, bool isFloat = false)
        : OperandVisitable(type, size), value(val), isSigned(isSigned), isVary(isVar), isFmov(isFloat)
    {
    }
    ImmOperand(const MIRSymbol &symbol, int64 val, int32 relocs, bool isSigned, VaryType isVar = kNotVary,
               bool isFloat = false)
        : OperandVisitable(kOpdStImmediate, 0),
          value(val),
          isSigned(isSigned),
          isVary(isVar),
          isFmov(isFloat),
          symbol(&symbol),
          relocs(relocs)
    {
    }
    ~ImmOperand() override = default;
    using OperandVisitable<ImmOperand>::OperandVisitable;

    ImmOperand *CloneTree(MapleAllocator &allocator) const override
    {
        // const MIRSymbol is not changed in cg, so we can do shallow copy
        return allocator.GetMemPool()->New<ImmOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<ImmOperand>(*this);
    }

    const MIRSymbol *GetSymbol() const
    {
        return symbol;
    }

    const std::string &GetName() const
    {
        return symbol->GetName();
    }

    int32 GetRelocs() const
    {
        return relocs;
    }

    bool IsInBitSize(uint8 size, uint8 nLowerZeroBits) const
    {
        return IsBitSizeImmediate(static_cast<uint64>(value), size, nLowerZeroBits);
    }

    bool IsBitmaskImmediate() const
    {
        DEBUG_ASSERT(!IsZero(), " 0 is reserved for bitmask immediate");
        DEBUG_ASSERT(!IsAllOnes(), " -1 is reserved for bitmask immediate");
        return maplebe::aarch64::IsBitmaskImmediate(static_cast<uint64>(value), static_cast<uint32>(size));
    }

    bool IsBitmaskImmediate(uint32 destSize) const
    {
        DEBUG_ASSERT(!IsZero(), " 0 is reserved for bitmask immediate");
        DEBUG_ASSERT(!IsAllOnes(), " -1 is reserved for bitmask immediate");
        return maplebe::aarch64::IsBitmaskImmediate(static_cast<uint64>(value), static_cast<uint32>(destSize));
    }

    bool IsSingleInstructionMovable() const
    {
        return (IsMoveWidableImmediate(static_cast<uint64>(value), static_cast<uint32>(size)) ||
                IsMoveWidableImmediate(~static_cast<uint64>(value), static_cast<uint32>(size)) || IsBitmaskImmediate());
    }

    bool IsSingleInstructionMovable(uint32 destSize) const
    {
        return (IsMoveWidableImmediate(static_cast<uint64>(value), static_cast<uint32>(destSize)) ||
                IsMoveWidableImmediate(~static_cast<uint64>(value), static_cast<uint32>(destSize)) ||
                IsBitmaskImmediate(destSize));
    }

    int64 GetValue() const
    {
        return value;
    }

    void SetValue(int64 val)
    {
        value = val;
    }

    void SetVary(VaryType flag)
    {
        isVary = flag;
    }

    bool IsZero() const
    {
        return value == 0;
    }

    VaryType GetVary() const
    {
        return isVary;
    }

    bool IsOne() const
    {
        return value == 1;
    }

    bool IsSignedValue() const
    {
        return isSigned;
    }

    void SetSigned()
    {
        isSigned = true;
    }

    void SetSigned(bool flag)
    {
        isSigned = flag;
    }

    bool IsInBitSizeRot(uint8 size) const
    {
        return IsInBitSizeRot(size, value);
    }

    static bool IsInBitSizeRot(uint8 size, int64 val)
    {
        /* to tell if the val is in a rotate window of size */
#if __GNU_C__ || __clang__
        if (val == 0) {
            return true;
        }
        int32 start = __builtin_ctzll(static_cast<uint64>(val));
        int32 end = static_cast<int32>(sizeof(val) * kBitsPerByte - __builtin_clzll(static_cast<uint64>(val)) - 1);
        return (size >= end - start + 1);
#else
        uint8 start = 0;
        uint8 end = 0;
        bool isFound = false;
        CHECK_FATAL(val > 0, "do not perform bit operator operations on signed integers");
        for (uint32 i = 0; i < k64BitSize; ++i) {
            /* check whether the ith bit of val is 1 or not */
            if (((static_cast<uint64>(val) >> i) & 0x1) == 0x1) {
                if (!isFound) {
                    start = i;
                    end = i;
                    isFound = true;
                } else {
                    end = i;
                }
            }
        }
        return !isFound || (size >= (end - start) + 1);
#endif
    }

    static bool IsInValueRange(int32 lowVal, int32 highVal, int32 val)
    {
        return val >= lowVal && val <= highVal;
    }

    bool IsNegative() const
    {
        return isSigned && value < 0;
    }

    void Add(int64 delta)
    {
        value += delta;
    }

    void Negate()
    {
        value = -value;
    }

    void BitwiseNegate()
    {
        value = ~(static_cast<uint64>(value)) & ((1ULL << size) - 1UL);
    }

    void DivideByPow2(int32 shift)
    {
        value = (static_cast<uint64>(value)) >> shift;
    }

    void ModuloByPow2(int32 shift)
    {
        value = (static_cast<uint64>(value)) & ((1ULL << shift) - 1UL);
    }

    bool IsAllOnes() const
    {
        return value == -1;
    }

    bool IsAllOnes32bit() const
    {
        return value == 0x0ffffffffLL;
    }

    bool operator<(const ImmOperand &iOpnd) const
    {
        return value < iOpnd.value || (value == iOpnd.value && isSigned < iOpnd.isSigned) ||
               (value == iOpnd.value && isSigned == iOpnd.isSigned && size < iOpnd.GetSize());
    }

    bool operator==(const ImmOperand &iOpnd) const
    {
        return (value == iOpnd.value && isSigned == iOpnd.isSigned && size == iOpnd.GetSize());
    }

    void Dump() const override;

    bool Less(const Operand &right) const override
    {
        if (&right == this) {
            return false;
        }

        /* For different type. */
        if (opndKind != right.GetKind()) {
            return opndKind < right.GetKind();
        }

        auto *rightOpnd = static_cast<const ImmOperand *>(&right);

        /* The same type. */
        if (isSigned != rightOpnd->isSigned) {
            return isSigned;
        }

        if (isVary != rightOpnd->isVary) {
            return isVary;
        }

        return value < rightOpnd->value;
    }

    bool Equals(Operand &operand) const override
    {
        if (!operand.IsImmediate()) {
            return false;
        }
        auto &op = static_cast<ImmOperand &>(operand);
        if (&op == this) {
            return true;
        }
        return (BasicEquals(op) && value == op.GetValue() && isSigned == op.IsSignedValue());
    }

    bool ValueEquals(const ImmOperand &op) const
    {
        if (&op == this) {
            return true;
        }
        return (value == op.GetValue() && isSigned == op.IsSignedValue());
    }
    bool IsFmov() const
    {
        return isFmov;
    }

protected:
    int64 value;
    bool isSigned;
    VaryType isVary;
    bool isFmov = false;
    const MIRSymbol *symbol; /* for Immediate in symbol form */
    int32 relocs;
};

class OfstOperand : public ImmOperand {
public:
    enum OfstType : uint8 {
        kSymbolOffset,
        kImmediateOffset,
        kSymbolImmediateOffset,
    };

    /* only for symbol offset */
    OfstOperand(const MIRSymbol &mirSymbol, uint32 size, int32 relocs)
        : ImmOperand(kOpdOffset, 0, size, true, kNotVary, false),
          offsetType(kSymbolOffset),
          symbol(&mirSymbol),
          relocs(relocs)
    {
    }
    /* only for Immediate offset */
    OfstOperand(int64 val, uint32 size, VaryType isVar = kNotVary)
        : ImmOperand(kOpdOffset, static_cast<int64>(val), size, true, isVar, false),
          offsetType(kImmediateOffset),
          symbol(nullptr),
          relocs(0)
    {
    }
    /* for symbol and Immediate offset */
    OfstOperand(const MIRSymbol &mirSymbol, int64 val, uint32 size, int32 relocs, VaryType isVar = kNotVary)
        : ImmOperand(kOpdOffset, val, size, true, isVar, false),
          offsetType(kSymbolImmediateOffset),
          symbol(&mirSymbol),
          relocs(relocs)
    {
    }

    ~OfstOperand() override
    {
        symbol = nullptr;
    }

    OfstOperand *CloneTree(MapleAllocator &allocator) const override
    {
        // const MIRSymbol is not changed in cg, so we can do shallow copy
        return allocator.GetMemPool()->New<OfstOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<OfstOperand>(*this);
    }

    bool IsSymOffset() const
    {
        return offsetType == kSymbolOffset;
    }
    bool IsImmOffset() const
    {
        return offsetType == kImmediateOffset;
    }
    bool IsSymAndImmOffset() const
    {
        return offsetType == kSymbolImmediateOffset;
    }

    const MIRSymbol *GetSymbol() const
    {
        return symbol;
    }

    const std::string &GetSymbolName() const
    {
        return symbol->GetName();
    }

    int64 GetOffsetValue() const
    {
        return GetValue();
    }

    void SetOffsetValue(int32 offVal)
    {
        SetValue(static_cast<int64>(offVal));
    }

    void AdjustOffset(int32 delta)
    {
        Add(static_cast<int64>(delta));
    }

    bool operator==(const OfstOperand &opnd) const
    {
        return (offsetType == opnd.offsetType && symbol == opnd.symbol && ImmOperand::operator==(opnd) &&
                relocs == opnd.relocs);
    }

    bool operator<(const OfstOperand &opnd) const
    {
        return (offsetType < opnd.offsetType || (offsetType == opnd.offsetType && symbol < opnd.symbol) ||
                (offsetType == opnd.offsetType && symbol == opnd.symbol && GetValue() < opnd.GetValue()));
    }

    void Dump() const override
    {
        if (IsImmOffset()) {
            LogInfo::MapleLogger() << "ofst:" << GetValue();
        } else {
            LogInfo::MapleLogger() << GetSymbolName();
            LogInfo::MapleLogger() << "+offset:" << GetValue();
        }
    }

private:
    OfstType offsetType;
    const MIRSymbol *symbol;
    int32 relocs;
};

/*
 * Table C1-6 A64 Load/Store addressing modes
 * |         Offset
 * Addressing Mode    | Immediate     | Register             | Extended Register
 *
 * Base register only | [base{,#0}]   | -                    | -
 * (no offset)        | B_OI_NONE     |                      |
 *                   imm=0
 *
 * Base plus offset   | [base{,#imm}] | [base,Xm{,LSL #imm}] | [base,Wm,(S|U)XTW
 * {#imm}] B_OI_NONE     | B_OR_X               | B_OR_X imm=0,1 (0,3)        |
 * imm=00,01,10,11 (0/2,s/u)
 *
 * Pre-indexed        | [base, #imm]! | -                    | -
 *
 * Post-indexed       | [base], #imm  | [base], Xm(a)        | -
 *
 * Literal            | label         | -                    | -
 * (PC-relative)
 *
 * a) The post-indexed by register offset mode can be used with the SIMD
 * Load/Store structure instructions described in Load/Store Vector on page
 * C3-154. Otherwise the post-indexed by register offset mode is not available.
 */
class MemOperand : public OperandVisitable<MemOperand> {
public:
    enum AArch64AddressingMode : uint8 {
        kAddrModeUndef,
        /* AddrMode_BO, base, offset. EA = [base] + offset; */
        kAddrModeBOi, /* INTACT: EA = [base]+immediate */
        /*
         * PRE: base += immediate, EA = [base]
         * POST: EA = [base], base += immediate
         */
        kAddrModeBOrX,    /* EA = [base]+Extend([offreg/idxreg]), OR=Wn/Xn */
        kAddrModeLiteral, /* AArch64 insruction LDR takes literal and */
        /*
         * "calculates an address from the PC value and an immediate offset,
         * loads a word from memory, and writes it to a register."
         */
        kAddrModeLo12Li,  // EA = [base] + #:lo12:Label+immediate. (Example: [x0,
                          // #:lo12:__Label300+456]
        kLiteral,         /* xxx_l mode: label */
        // X86 scale Type
        kScale,
    };
    /*
     * ARMv8-A A64 ISA Overview by Matteo Franchin @ ARM
     * (presented at 64-bit terminal platform on ARM. Sep. 2015) p.14
     * o Address to load from/store to is a 64-bit base register + an optional
     * offset LDR X0, [X1] ; Load from address held in X1 STR X0, [X1] ; Store to
     * address held in X1
     *
     * o Offset can be an immediate or a register
     *   LDR X0, [X1, #8]  ; Load from address [X1 + 8 bytes]
     *   LDR X0, [X1, #-8] ; Load with negative offset
     *   LDR X0, [X1, X2]  ; Load from address [X1 + X2]
     *
     * o A Wn register offset needs to be extended to 64 bits
     *  LDR X0, [X1, W2, SXTW] ; Sign-extend offset in W2
     *   LDR X0, [X1, W2, UXTW] ; Zero-extend offset in W2
     *
     * o Both Xn and Wn register offsets can include an optional left-shift
     *   LDR X0, [X1, W2, UXTW #2] ; Zero-extend offset in W2 & left-shift by 2
     *   LDR X0, [X1, X2, LSL #2]  ; Left-shift offset in X2 by 2
     *
     * p.15
     * Addressing Modes                       Analogous C Code
     *                                       int *intptr = ... // X1
     *                                       int out; // W0
     * o Simple: X1 is not changed
     *   LDR W0, [X1]                        out = *intptr;
     * o Offset: X1 is not changed
     *   LDR W0, [X1, #4]                    out = intptr[1];
     * o Pre-indexed: X1 changed before load
     *   LDR W0, [X1, #4]! =|ADD X1,X1,#4    out = *(++intptr);
     * |LDR W0,[X1]
     * o Post-indexed: X1 changed after load
     *   LDR W0, [X1], #4  =|LDR W0,[X1]     out = *(intptr++);
     * |ADD X1,X1,#4
     */
    enum ExtendInfo : uint8 {
        kShiftZero = 0x1,
        kShiftOne = 0x2,
        kShiftTwo = 0x4,
        kShiftThree = 0x8,
        kUnsignedExtend = 0x10,
        kSignExtend = 0x20
    };

    enum IndexingOption : uint8 {
        kIntact,    /* base register stays the same */
        kPreIndex,  /* base register gets changed before load */
        kPostIndex, /* base register gets changed after load */
    };

    MemOperand(uint32 size) : OperandVisitable(Operand::kOpdMem, size) {}
    MemOperand(uint32 size, const MIRSymbol &mirSymbol) : OperandVisitable(Operand::kOpdMem, size), symbol(&mirSymbol)
    {
    }

    MemOperand(uint32 size, RegOperand &baseOp, ImmOperand &ofstOp, AArch64AddressingMode mode = kAddrModeBOi)
        : OperandVisitable(Operand::kOpdMem, size),
          baseOpnd(&baseOp),
          offsetOpnd(&ofstOp),
          symbol(nullptr),
          addrMode(mode)
    {
        DEBUG_ASSERT((mode == kAddrModeBOi), "check mode!");
    }

    MemOperand(uint32 size, RegOperand *baseOp, RegOperand *indexOp, ImmOperand *ofstOp, const MIRSymbol *mirSymbol,
               ImmOperand *scaleOp = nullptr)
        : OperandVisitable(Operand::kOpdMem, size),
          baseOpnd(baseOp),
          indexOpnd(indexOp),
          offsetOpnd(ofstOp),
          scaleOpnd(scaleOp),
          symbol(mirSymbol)
    {
    }

    MemOperand(RegOperand *base, OfstOperand *offset, uint32 size, IndexingOption idxOpt = kIntact)
        : OperandVisitable(Operand::kOpdMem, size),
          baseOpnd(base),
          indexOpnd(nullptr),
          offsetOpnd(offset),
          symbol(nullptr),
          addrMode(kAddrModeBOi),
          extend(0),
          idxOpt(idxOpt),
          noExtend(false),
          isStackMem(false)
    {
    }

    MemOperand(AArch64AddressingMode mode, uint32 size, RegOperand &base, RegOperand *index, ImmOperand *offset,
               const MIRSymbol *sym)
        : OperandVisitable(Operand::kOpdMem, size),
          baseOpnd(&base),
          indexOpnd(index),
          offsetOpnd(offset),
          symbol(sym),
          addrMode(mode),
          extend(0),
          idxOpt(kIntact),
          noExtend(false),
          isStackMem(false)
    {
    }

    MemOperand(AArch64AddressingMode mode, uint32 size, RegOperand &base, RegOperand &index, ImmOperand *offset,
               const MIRSymbol &sym, bool noExtend)
        : OperandVisitable(Operand::kOpdMem, size),
          baseOpnd(&base),
          indexOpnd(&index),
          offsetOpnd(offset),
          symbol(&sym),
          addrMode(mode),
          extend(0),
          idxOpt(kIntact),
          noExtend(noExtend),
          isStackMem(false)
    {
    }

    MemOperand(AArch64AddressingMode mode, uint32 dSize, RegOperand &baseOpnd, RegOperand &indexOpnd, uint32 shift,
               bool isSigned = false)
        : OperandVisitable(Operand::kOpdMem, dSize),
          baseOpnd(&baseOpnd),
          indexOpnd(&indexOpnd),
          offsetOpnd(nullptr),
          symbol(nullptr),
          addrMode(mode),
          extend((isSigned ? kSignExtend : kUnsignedExtend) | (1U << shift)),
          idxOpt(kIntact),
          noExtend(false),
          isStackMem(false)
    {
    }

    MemOperand(AArch64AddressingMode mode, uint32 dSize, const MIRSymbol &sym)
        : OperandVisitable(Operand::kOpdMem, dSize),
          baseOpnd(nullptr),
          indexOpnd(nullptr),
          offsetOpnd(nullptr),
          symbol(&sym),
          addrMode(mode),
          extend(0),
          idxOpt(kIntact),
          noExtend(false),
          isStackMem(false)
    {
        DEBUG_ASSERT(mode == kAddrModeLiteral,
                     "This constructor version is supposed to be used with "
                     "AddrMode_Literal only");
    }

    /* Copy constructor */
    explicit MemOperand(const MemOperand &memOpnd)
        : OperandVisitable(Operand::kOpdMem, memOpnd.GetSize()),
          baseOpnd(memOpnd.baseOpnd),
          indexOpnd(memOpnd.indexOpnd),
          offsetOpnd(memOpnd.offsetOpnd),
          scaleOpnd(memOpnd.scaleOpnd),
          symbol(memOpnd.symbol),
          memoryOrder(memOpnd.memoryOrder),
          addrMode(memOpnd.addrMode),
          extend(memOpnd.extend),
          idxOpt(memOpnd.idxOpt),
          noExtend(memOpnd.noExtend),
          isStackMem(memOpnd.isStackMem),
          isStackArgMem(memOpnd.isStackArgMem),
          isVolatile(memOpnd.isVolatile)
    {
    }

    MemOperand &operator=(const MemOperand &memOpnd) = default;

    ~MemOperand() override = default;
    using OperandVisitable<MemOperand>::OperandVisitable;

    MemOperand *CloneTree(MapleAllocator &allocator) const override
    {
        auto *memOpnd = allocator.GetMemPool()->New<MemOperand>(*this);
        if (baseOpnd != nullptr) {
            memOpnd->SetBaseRegister(*baseOpnd->CloneTree(allocator));
        }
        if (indexOpnd != nullptr) {
            memOpnd->SetIndexRegister(*indexOpnd->CloneTree(allocator));
        }
        if (offsetOpnd != nullptr) {
            memOpnd->SetOffsetOperand(*offsetOpnd->CloneTree(allocator));
        }
        return memOpnd;
    }

    MemOperand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<MemOperand>(*this);
    }

    void Dump() const override {};

    RegOperand *GetBaseRegister() const
    {
        return baseOpnd;
    }

    void SetBaseRegister(RegOperand &regOpnd)
    {
        baseOpnd = &regOpnd;
    }

    RegOperand *GetIndexRegister() const
    {
        return indexOpnd;
    }

    void SetIndexRegister(RegOperand &regOpnd)
    {
        indexOpnd = &regOpnd;
    }

    ImmOperand *GetOffsetOperand() const
    {
        return offsetOpnd;
    }

    void SetOffsetOperand(ImmOperand &oftOpnd)
    {
        offsetOpnd = &oftOpnd;
    }

    const ImmOperand *GetScaleOperand() const
    {
        return scaleOpnd;
    }

    void SetScaleOperand(ImmOperand &scaOpnd)
    {
        scaleOpnd = &scaOpnd;
    }

    const MIRSymbol *GetSymbol() const
    {
        return symbol;
    }

    void SetAccessSize(uint8 size)
    {
        accessSize = size;
    }

    uint8 GetAccessSize() const
    {
        return accessSize;
    }

    AArch64AddressingMode GetAddrMode() const
    {
        return addrMode;
    }

    const std::string &GetSymbolName() const
    {
        return GetSymbol()->GetName();
    }

    bool IsStackMem() const
    {
        return isStackMem;
    }

    void SetStackMem(bool isStack)
    {
        isStackMem = isStack;
    }

    bool IsStackArgMem() const
    {
        return isStackArgMem;
    }

    void SetStackArgMem(bool isStackArg)
    {
        isStackArgMem = isStackArg;
    }

    Operand *GetOffset() const;

    OfstOperand *GetOffsetImmediate() const
    {
        return static_cast<OfstOperand *>(GetOffsetOperand());
    }

    /* Returns N where alignment == 2^N */
    static uint32 GetImmediateOffsetAlignment(uint32 dSize)
    {
        DEBUG_ASSERT(dSize >= k8BitSize, "error val:dSize");
        DEBUG_ASSERT(dSize <= k128BitSize, "error val:dSize");
        DEBUG_ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
        /* dSize==8: 0, dSize==16 : 1, dSize==32: 2, dSize==64: 3 */
        return __builtin_ctz(dSize) - kBaseOffsetAlignment;
    }

    static int32 GetMaxPIMM(uint32 dSize)
    {
        dSize = dSize > k64BitSize ? k64BitSize : dSize;
        DEBUG_ASSERT(dSize >= k8BitSize, "error val:dSize");
        DEBUG_ASSERT(dSize <= k128BitSize, "error val:dSize");
        DEBUG_ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
        uint32 alignment = GetImmediateOffsetAlignment(dSize);
        /* alignment is between kAlignmentOf8Bit and kAlignmentOf64Bit */
        DEBUG_ASSERT(alignment >= kOffsetAlignmentOf8Bit, "error val:alignment");
        DEBUG_ASSERT(alignment <= kOffsetAlignmentOf128Bit, "error val:alignment");
        return (kMaxPimm[alignment]);
    }

    static int32 GetMaxPairPIMM(uint32 dSize)
    {
        DEBUG_ASSERT(dSize >= k32BitSize, "error val:dSize");
        DEBUG_ASSERT(dSize <= k128BitSize, "error val:dSize");
        DEBUG_ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
        uint32 alignment = GetImmediateOffsetAlignment(dSize);
        /* alignment is between kAlignmentOf8Bit and kAlignmentOf64Bit */
        DEBUG_ASSERT(alignment >= kOffsetAlignmentOf32Bit, "error val:alignment");
        DEBUG_ASSERT(alignment <= kOffsetAlignmentOf128Bit, "error val:alignment");
        return (kMaxPairPimm[alignment - k2BitSize]);
    }

    bool IsOffsetMisaligned(uint32 dSize) const
    {
        DEBUG_ASSERT(dSize >= k8BitSize, "error val:dSize");
        DEBUG_ASSERT(dSize <= k128BitSize, "error val:dSize");
        DEBUG_ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
        if (dSize == k8BitSize) {
            return false;
        }
        OfstOperand *ofstOpnd = GetOffsetImmediate();
        if (!ofstOpnd) {
            return false;
        }
        int64 ofstVal = ofstOpnd->GetOffsetValue();
        if (addrMode == kAddrModeBOi) {
            if (ofstVal >= kMinSimm32 && ofstVal <= kMaxSimm32) {
                return false;
            }
            return ((static_cast<uint32>(ofstOpnd->GetOffsetValue()) &
                     static_cast<uint32>((1U << static_cast<uint32>(GetImmediateOffsetAlignment(dSize))) - 1)) != 0);
        } else if (addrMode == kAddrModeLo12Li) {
            uint32 alignByte = (dSize / k8BitSize);
            return ((ofstVal % static_cast<int64>(alignByte)) != k0BitSize);
        }
        return false;
    }

    static bool IsSIMMOffsetOutOfRange(int64 offset, bool is64bit, bool isLDSTPair)
    {
        if (!isLDSTPair) {
            return (offset < kMinSimm32 || offset > kMaxSimm32);
        }
        if (is64bit) {
            return (offset < kMinSimm64 || offset > kMaxSimm64Pair) || (static_cast<uint64>(offset) & k7BitSize);
        }
        return (offset < kMinSimm32 || offset > kMaxSimm32Pair) || (static_cast<uint64>(offset) & k3BitSize);
    }

    static bool IsPIMMOffsetOutOfRange(int32 offset, uint32 dSize)
    {
        DEBUG_ASSERT(dSize >= k8BitSize, "error val:dSize");
        DEBUG_ASSERT(dSize <= k128BitSize, "error val:dSize");
        DEBUG_ASSERT((dSize & (dSize - 1)) == 0, "error val:dSize");
        return (offset < 0 || offset > GetMaxPIMM(dSize));
    }

    bool operator<(const MemOperand &opnd) const
    {
        if (addrMode != opnd.addrMode) {
            return addrMode < opnd.addrMode;
        }
        if (GetBaseRegister() != opnd.GetBaseRegister()) {
            return GetBaseRegister() < opnd.GetBaseRegister();
        }
        if (GetIndexRegister() != opnd.GetIndexRegister()) {
            return GetIndexRegister() < opnd.GetIndexRegister();
        }
        if (GetOffsetOperand() != opnd.GetOffsetOperand()) {
            return GetOffsetOperand() < opnd.GetOffsetOperand();
        }
        if (GetSymbol() != opnd.GetSymbol()) {
            return GetSymbol() < opnd.GetSymbol();
        }
        if (GetSize() != opnd.GetSize()) {
            return GetSize() < opnd.GetSize();
        }
        if (extend != opnd.extend) {
            return extend < opnd.extend;
        }
        return false;
    }

    bool operator==(const MemOperand &opnd) const
    {
        return (GetSize() == opnd.GetSize()) && (addrMode == opnd.addrMode) && (extend == opnd.extend) &&
               (GetBaseRegister() == opnd.GetBaseRegister()) && (GetIndexRegister() == opnd.GetIndexRegister()) &&
               (GetSymbol() == opnd.GetSymbol()) && (GetOffsetOperand() == opnd.GetOffsetOperand()) &&
               (IsVolatile() == opnd.IsVolatile());
    }

    VaryType GetMemVaryType() const
    {
        Operand *ofstOpnd = GetOffsetOperand();
        if (ofstOpnd != nullptr) {
            auto *opnd = static_cast<OfstOperand *>(ofstOpnd);
            return opnd->GetVary();
        }
        return kNotVary;
    }

    void SetAddrMode(AArch64AddressingMode val)
    {
        addrMode = val;
    }

    bool IsExtendedRegisterMode() const
    {
        return addrMode == kAddrModeBOrX;
    }

    void UpdateExtend(ExtendInfo flag)
    {
        extend = flag | (1U << ShiftAmount());
    }

    bool SignedExtend() const
    {
        return IsExtendedRegisterMode() && ((extend & kSignExtend) != 0);
    }

    bool UnsignedExtend() const
    {
        return IsExtendedRegisterMode() && !SignedExtend();
    }

    uint32 ShiftAmount() const
    {
        uint32 scale = extend & 0xF;
        /* 8 is 1 << 3, 4 is 1 << 2, 2 is 1 << 1, 1 is 1 << 0; */
        return (scale == 8) ? 3 : ((scale == 4) ? 2 : ((scale == 2) ? 1 : 0));
    }

    bool ShouldEmitExtend() const
    {
        return !noExtend && ((extend & 0x3F) != 0);
    }

    IndexingOption GetIndexOpt() const
    {
        return idxOpt;
    }

    void SetIndexOpt(IndexingOption newidxOpt)
    {
        idxOpt = newidxOpt;
    }

    bool GetNoExtend() const
    {
        return noExtend;
    }

    void SetNoExtend(bool val)
    {
        noExtend = val;
    }

    uint32 GetExtend() const
    {
        return extend;
    }

    void SetExtend(uint32 val)
    {
        extend = val;
    }

    void SetVolatile(bool flag)
    {
        isVolatile = flag;
    }

    bool IsIntactIndexed() const
    {
        return idxOpt == kIntact;
    }

    bool IsPostIndexed() const
    {
        return idxOpt == kPostIndex;
    }

    bool IsPreIndexed() const
    {
        return idxOpt == kPreIndex;
    }

    bool IsVolatile() const
    {
        return isVolatile;
    }

    std::string GetExtendAsString() const
    {
        if (GetIndexRegister()->GetSize() == k64BitSize) {
            return std::string("LSL");
        }
        return ((extend & kSignExtend) != 0) ? std::string("SXTW") : std::string("UXTW");
    }

    /* Return true if given operand has the same base reg and offset with this. */
    bool Equals(Operand &op) const override;
    bool Equals(const MemOperand &op) const;
    bool Less(const Operand &right) const override;

private:
    RegOperand *baseOpnd = nullptr;   /* base register */
    RegOperand *indexOpnd = nullptr;  /* index register */
    ImmOperand *offsetOpnd = nullptr; /* offset immediate */
    ImmOperand *scaleOpnd = nullptr;
    const MIRSymbol *symbol; /* AddrMode_Literal */
    uint32 memoryOrder = 0;
    uint8 accessSize = 0; /* temp, must be set right before use everytime. */
    AArch64AddressingMode addrMode = kAddrModeBOi;
    uint32 extend = false;           /* used with offset register ; AddrMode_B_OR_X */
    IndexingOption idxOpt = kIntact; /* used with offset immediate ; AddrMode_B_OI */
    bool noExtend = false;
    bool isStackMem = false;
    bool isStackArgMem = false;
    bool isVolatile = false;  // based on mem info from ME
};

class LabelOperand : public OperandVisitable<LabelOperand> {
public:
    LabelOperand(const char *parent, LabelIdx labIdx, MemPool &mp)
        : OperandVisitable(kOpdBBAddress, 0), labelIndex(labIdx), parentFunc(parent, &mp), orderID(-1u)
    {
    }

    ~LabelOperand() override = default;
    using OperandVisitable<LabelOperand>::OperandVisitable;

    LabelOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<LabelOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<LabelOperand>(*this);
    }

    bool IsLabelOpnd() const override
    {
        return true;
    }

    LabelIdx GetLabelIndex() const
    {
        return labelIndex;
    }

    const MapleString &GetParentFunc() const
    {
        return parentFunc;
    }

    LabelIDOrder GetLabelOrder() const
    {
        return orderID;
    }

    void SetLabelOrder(LabelIDOrder idx)
    {
        orderID = idx;
    }

    void Dump() const override;

    bool Less(const Operand &right) const override
    {
        if (&right == this) {
            return false;
        }

        /* For different type. */
        if (opndKind != right.GetKind()) {
            return opndKind < right.GetKind();
        }

        auto *rightOpnd = static_cast<const LabelOperand *>(&right);

        int32 nRes = strcmp(parentFunc.c_str(), rightOpnd->parentFunc.c_str());
        if (nRes == 0) {
            return labelIndex < rightOpnd->labelIndex;
        } else {
            return nRes < 0;
        }
    }

    bool Equals(Operand &operand) const override
    {
        if (!operand.IsLabel()) {
            return false;
        }
        auto &op = static_cast<LabelOperand &>(operand);
        return ((&op == this) || (op.GetLabelIndex() == labelIndex));
    }

protected:
    LabelIdx labelIndex;
    const MapleString parentFunc;

private:
    /* this index records the order this label is defined during code emit. */
    LabelIDOrder orderID = -1u;
};

class ListOperand : public OperandVisitable<ListOperand> {
public:
    explicit ListOperand(MapleAllocator &allocator)
        : OperandVisitable(Operand::kOpdList, 0), opndList(allocator.Adapter())
    {
    }

    ~ListOperand() override = default;

    using OperandVisitable<ListOperand>::OperandVisitable;

    ListOperand *CloneTree(MapleAllocator &allocator) const override
    {
        auto *listOpnd = allocator.GetMemPool()->New<ListOperand>(allocator);
        for (auto regOpnd : opndList) {
            listOpnd->PushOpnd(*regOpnd->CloneTree(allocator));
        }
        return listOpnd;
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<ListOperand>(*this);
    }

    void PushOpnd(RegOperand &opnd)
    {
        opndList.push_back(&opnd);
    }

    MapleList<RegOperand *> &GetOperands()
    {
        return opndList;
    }

    const MapleList<RegOperand *> &GetOperands() const
    {
        return opndList;
    }

    void Dump() const override
    {
        for (auto it = opndList.begin(); it != opndList.end();) {
            (*it)->Dump();
            LogInfo::MapleLogger() << (++it == opndList.end() ? "" : " ,");
        }
    }

    bool Less(const Operand &right) const override
    {
        /* For different type. */
        if (opndKind != right.GetKind()) {
            return opndKind < right.GetKind();
        }

        DEBUG_ASSERT(false, "We don't need to compare list operand.");
        return false;
    }

    bool Equals(Operand &operand) const override
    {
        if (!operand.IsList()) {
            return false;
        }
        auto &op = static_cast<ListOperand &>(operand);
        return (&op == this);
    }

protected:
    MapleList<RegOperand *> opndList;
};

/* representing for global variables address */
class StImmOperand : public OperandVisitable<StImmOperand> {
public:
    StImmOperand(const MIRSymbol &symbol, int64 offset, int32 relocs)
        : OperandVisitable(kOpdStImmediate, 0), symbol(&symbol), offset(offset), relocs(relocs)
    {
    }

    ~StImmOperand() override = default;
    using OperandVisitable<StImmOperand>::OperandVisitable;

    StImmOperand *CloneTree(MapleAllocator &allocator) const override
    {
        // const MIRSymbol is not changed in cg, so we can do shallow copy
        return allocator.GetMemPool()->New<StImmOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<StImmOperand>(*this);
    }

    const MIRSymbol *GetSymbol() const
    {
        return symbol;
    }

    const std::string &GetName() const
    {
        return symbol->GetName();
    }

    int64 GetOffset() const
    {
        return offset;
    }

    void SetOffset(int64 newOffset)
    {
        offset = newOffset;
    }

    int32 GetRelocs() const
    {
        return relocs;
    }

    bool operator==(const StImmOperand &opnd) const
    {
        return (symbol == opnd.symbol && offset == opnd.offset && relocs == opnd.relocs);
    }

    bool operator<(const StImmOperand &opnd) const
    {
        return (symbol < opnd.symbol || (symbol == opnd.symbol && offset < opnd.offset) ||
                (symbol == opnd.symbol && offset == opnd.offset && relocs < opnd.relocs));
    }

    bool Less(const Operand &right) const override;

    void Dump() const override
    {
        CHECK_FATAL(false, "dont run here");
    }

private:
    const MIRSymbol *symbol;
    int64 offset;
    int32 relocs;
};

class ExtendShiftOperand : public OperandVisitable<ExtendShiftOperand> {
public:
    /* if and only if at least one register is WSP, ARM Recommends use of the LSL
     * operator name rathe than UXTW */
    enum ExtendOp : uint8 {
        kUndef,
        kUXTB,
        kUXTH,
        kUXTW, /* equal to lsl in 32bits */
        kUXTX, /* equal to lsl in 64bits */
        kSXTB,
        kSXTH,
        kSXTW,
        kSXTX,
    };

    ExtendShiftOperand(ExtendOp op, uint32 amt, int32 bitLen)
        : OperandVisitable(Operand::kOpdExtend, bitLen), extendOp(op), shiftAmount(amt)
    {
    }

    ~ExtendShiftOperand() override = default;
    using OperandVisitable<ExtendShiftOperand>::OperandVisitable;

    ExtendShiftOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<ExtendShiftOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<ExtendShiftOperand>(*this);
    }

    uint32 GetShiftAmount() const
    {
        return shiftAmount;
    }

    ExtendOp GetExtendOp() const
    {
        return extendOp;
    }

    uint32 GetValue() const
    {
        return shiftAmount;
    }

    bool Less(const Operand &right) const override;

    void Dump() const override
    {
        CHECK_FATAL(false, "dont run here");
    }

private:
    ExtendOp extendOp;
    uint32 shiftAmount;
};

class BitShiftOperand : public OperandVisitable<BitShiftOperand> {
public:
    enum ShiftOp : uint8 {
        kUndef,
        kLSL, /* logical shift left */
        kLSR, /* logical shift right */
        kASR, /* arithmetic shift right */
    };

    /* bitlength is equal to 5 or 6 */
    BitShiftOperand(ShiftOp op, uint32 amt, int32 bitLen)
        : OperandVisitable(Operand::kOpdShift, bitLen), shiftOp(op), shiftAmount(amt)
    {
    }

    ~BitShiftOperand() override = default;
    using OperandVisitable<BitShiftOperand>::OperandVisitable;

    BitShiftOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<BitShiftOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<BitShiftOperand>(*this);
    }

    bool Less(const Operand &right) const override
    {
        if (&right == this) {
            return false;
        }

        /* For different type. */
        if (GetKind() != right.GetKind()) {
            return GetKind() < right.GetKind();
        }

        const BitShiftOperand *rightOpnd = static_cast<const BitShiftOperand *>(&right);

        /* The same type. */
        if (shiftOp != rightOpnd->shiftOp) {
            return shiftOp < rightOpnd->shiftOp;
        }
        return shiftAmount < rightOpnd->shiftAmount;
    }

    uint32 GetShiftAmount() const
    {
        return shiftAmount;
    }

    ShiftOp GetShiftOp() const
    {
        return shiftOp;
    }

    uint32 GetValue() const
    {
        return GetShiftAmount();
    }

    void Dump() const override
    {
        CHECK_FATAL(false, "dont run here");
    }

private:
    ShiftOp shiftOp;
    uint32 shiftAmount;
};

class CommentOperand : public OperandVisitable<CommentOperand> {
public:
    CommentOperand(const char *str, MemPool &memPool) : OperandVisitable(Operand::kOpdString, 0), comment(str, &memPool)
    {
    }

    CommentOperand(const std::string &str, MemPool &memPool)
        : OperandVisitable(Operand::kOpdString, 0), comment(str, &memPool)
    {
    }

    ~CommentOperand() override = default;
    using OperandVisitable<CommentOperand>::OperandVisitable;

    const MapleString &GetComment() const
    {
        return comment;
    }

    CommentOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<CommentOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<CommentOperand>(*this);
    }

    bool IsCommentOpnd() const override
    {
        return true;
    }

    bool Less(const Operand &right) const override
    {
        /* For different type. */
        return GetKind() < right.GetKind();
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "# ";
        if (!comment.empty()) {
            LogInfo::MapleLogger() << comment;
        }
    }

private:
    const MapleString comment;
};

using StringOperand = CommentOperand;

class ListConstraintOperand : public OperandVisitable<ListConstraintOperand> {
public:
    explicit ListConstraintOperand(MapleAllocator &allocator)
        : OperandVisitable(Operand::kOpdString, 0), stringList(allocator.Adapter()) {};

    ~ListConstraintOperand() override = default;
    using OperandVisitable<ListConstraintOperand>::OperandVisitable;

    void Dump() const override
    {
        for (auto *str : stringList) {
            LogInfo::MapleLogger() << "(" << str->GetComment().c_str() << ")";
        }
    }

    ListConstraintOperand *CloneTree(MapleAllocator &allocator) const override
    {
        auto *constraintOpnd = allocator.GetMemPool()->New<ListConstraintOperand>(allocator);
        for (auto stringOpnd : stringList) {
            constraintOpnd->stringList.emplace_back(stringOpnd->CloneTree(allocator));
        }
        return constraintOpnd;
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<ListConstraintOperand>(*this);
    }

    bool Less(const Operand &right) const override
    {
        /* For different type. */
        if (opndKind != right.GetKind()) {
            return opndKind < right.GetKind();
        }

        DEBUG_ASSERT(false, "We don't need to compare list operand.");
        return false;
    }

    MapleVector<StringOperand *> stringList;
};

/* for cg ssa analysis */
class PhiOperand : public OperandVisitable<PhiOperand> {
public:
    explicit PhiOperand(MapleAllocator &allocator) : OperandVisitable(Operand::kOpdPhi, 0), phiList(allocator.Adapter())
    {
    }

    ~PhiOperand() override = default;
    using OperandVisitable<PhiOperand>::OperandVisitable;

    PhiOperand *CloneTree(MapleAllocator &allocator) const override
    {
        auto *phiOpnd = allocator.GetMemPool()->New<PhiOperand>(allocator);
        for (auto phiPair : phiList) {
            phiOpnd->InsertOpnd(phiPair.first, *phiPair.second->CloneTree(allocator));
        }
        return phiOpnd;
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.Clone<PhiOperand>(*this);
    }

    void Dump() const override
    {
        CHECK_FATAL(false, "NIY");
    }

    void InsertOpnd(uint32 bbId, RegOperand &phiParam)
    {
        DEBUG_ASSERT(!phiList.count(bbId), "cannot insert duplicate operand");
        (void)phiList.emplace(std::pair(bbId, &phiParam));
    }

    void UpdateOpnd(uint32 bbId, uint32 newId, RegOperand &phiParam)
    {
        (void)phiList.emplace(std::pair(newId, &phiParam));
        phiList.erase(bbId);
    }

    MapleMap<uint32, RegOperand *> &GetOperands()
    {
        return phiList;
    }

    uint32 GetLeastCommonValidBit() const;

    bool IsRedundancy() const;

    bool Less(const Operand &right) const override
    {
        /* For different type. */
        if (opndKind != right.GetKind()) {
            return opndKind < right.GetKind();
        }
        DEBUG_ASSERT(false, "We don't need to compare list operand.");
        return false;
    }

    bool Equals(Operand &operand) const override
    {
        if (!operand.IsPhi()) {
            return false;
        }
        auto &op = static_cast<PhiOperand &>(operand);
        return (&op == this);
    }

protected:
    MapleMap<uint32, RegOperand *> phiList; /* ssa-operand && BBId */
};

/* Use StImmOperand instead? */
class FuncNameOperand : public OperandVisitable<FuncNameOperand> {
public:
    explicit FuncNameOperand(const MIRSymbol &fsym) : OperandVisitable(kOpdBBAddress, 0), symbol(&fsym) {}

    ~FuncNameOperand() override
    {
        symbol = nullptr;
    }
    using OperandVisitable<FuncNameOperand>::OperandVisitable;

    const std::string &GetName() const
    {
        return symbol->GetName();
    }

    bool IsFuncNameOpnd() const override
    {
        return true;
    }

    const MIRSymbol *GetFunctionSymbol() const
    {
        return symbol;
    }

    void SetFunctionSymbol(const MIRSymbol &fsym)
    {
        symbol = &fsym;
    }

    FuncNameOperand *CloneTree(MapleAllocator &allocator) const override
    {
        // const MIRSymbol is not changed in cg, so we can do shallow copy
        return allocator.GetMemPool()->New<FuncNameOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.New<FuncNameOperand>(*this);
    }

    bool Less(const Operand &right) const override
    {
        if (&right == this) {
            return false;
        }
        /* For different type. */
        if (GetKind() != right.GetKind()) {
            return GetKind() < right.GetKind();
        }

        auto *rightOpnd = static_cast<const FuncNameOperand *>(&right);

        return static_cast<const void *>(symbol) < static_cast<const void *>(rightOpnd->symbol);
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << GetName();
    }

private:
    const MIRSymbol *symbol;
};

namespace operand {
/* bit 0-7 for common */
enum CommOpndDescProp : maple::uint64 { kIsDef = 1ULL, kIsUse = (1ULL << 1), kIsVector = (1ULL << 2) };

/* bit 8-15 for reg */
enum RegOpndDescProp : maple::uint64 {
    kInt = (1ULL << 8),
    kFloat = (1ULL << 9),
    kRegTyCc = (1ULL << 10),
    kRegTyVary = (1ULL << 11),
};

/* bit 16-23 for imm */
enum ImmOpndDescProp : maple::uint64 {};

/* bit 24-31 for mem */
enum MemOpndDescProp : maple::uint64 {
    kMemLow12 = (1ULL << 24),
    kLiteralLow12 = kMemLow12,
    kIsLoadLiteral = (1ULL << 25)
};
}  // namespace operand

class OpndDesc {
public:
    OpndDesc(Operand::OperandType t, maple::uint64 p, maple::uint32 s) : opndType(t), property(p), size(s) {}
    virtual ~OpndDesc() = default;

    Operand::OperandType GetOperandType() const
    {
        return opndType;
    }

    maple::uint32 GetSize() const
    {
        return size;
    }

    bool IsImm() const
    {
        return opndType == Operand::kOpdImmediate;
    }

    bool IsRegister() const
    {
        return opndType == Operand::kOpdRegister;
    }

    bool IsMem() const
    {
        return opndType == Operand::kOpdMem;
    }

    bool IsRegDef() const
    {
        return opndType == Operand::kOpdRegister && (property & operand::kIsDef);
    }

    bool IsRegUse() const
    {
        return opndType == Operand::kOpdRegister && (property & operand::kIsUse);
    }

    bool IsDef() const
    {
        return (property & operand::kIsDef) != 0;
    }

    bool IsUse() const
    {
        return (property & operand::kIsUse) != 0;
    }

    bool IsMemLow12() const
    {
        return IsMem() && (property & operand::kMemLow12);
    }

    bool IsLiteralLow12() const
    {
        return opndType == Operand::kOpdStImmediate && (property & operand::kLiteralLow12);
    }

    bool IsLoadLiteral() const
    {
        return (property & operand::kIsLoadLiteral) != 0;
    }

#define DEFINE_MOP(op, ...) static const OpndDesc op;
#include "operand.def"
#undef DEFINE_MOP

private:
    Operand::OperandType opndType;
    maple::uint64 property;
    maple::uint32 size;
};

class CondOperand : public OperandVisitable<CondOperand> {
public:
    explicit CondOperand(maplebe::ConditionCode cc) : OperandVisitable(Operand::kOpdCond, k4ByteSize), cc(cc) {}

    ~CondOperand() override = default;
    using OperandVisitable<CondOperand>::OperandVisitable;

    CondOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<CondOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        return memPool.New<CondOperand>(cc);
    }

    ConditionCode GetCode() const
    {
        return cc;
    }

    bool Less(const Operand &right) const override;

    void Dump() const override
    {
        CHECK_FATAL(false, "dont run here");
    }

    static const char *ccStrs[kCcLast];

private:
    ConditionCode cc;
};

class OpndDumpVisitor : public OperandVisitorBase,
                        public OperandVisitors<RegOperand, ImmOperand, MemOperand, LabelOperand, FuncNameOperand,
                                               ListOperand, StImmOperand, CondOperand, CommentOperand, BitShiftOperand,
                                               ExtendShiftOperand, PhiOperand> {
public:
    explicit OpndDumpVisitor(const OpndDesc &operandDesc) : opndDesc(&operandDesc) {}
    virtual ~OpndDumpVisitor()
    {
        opndDesc = nullptr;
    }

protected:
    virtual void DumpOpndPrefix()
    {
        LogInfo::MapleLogger() << " (opnd:";
    }
    virtual void DumpOpndSuffix()
    {
        LogInfo::MapleLogger() << " )";
    }
    void DumpSize(const Operand &opnd) const
    {
        LogInfo::MapleLogger() << " [size:" << opnd.GetSize() << "]";
    }
    void DumpReferenceInfo(const Operand &opnd) const
    {
        if (opnd.IsReference()) {
            LogInfo::MapleLogger() << "[is_ref]";
        }
    }
    const OpndDesc *GetOpndDesc() const
    {
        return opndDesc;
    }

private:
    const OpndDesc *opndDesc;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_OPERAND_H */
