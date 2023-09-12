/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_SHARE_GATE_META_DATA_H
#define ECMASCRIPT_COMPILER_SHARE_GATE_META_DATA_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "ecmascript/elements.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "libpandabase/macros.h"

#include "ecmascript/compiler/share_opcodes.h"

namespace panda::ecmascript::kungfu {
using GateRef = int32_t;
using PGOSampleType = pgo::PGOSampleType;
using PGORWOpType = pgo::PGORWOpType;

#define GATE_META_DATA_DEOPT_REASON(V)        \
    V(NotInt, NOTINT)                         \
    V(NotDouble, NOTDOUBLE)                   \
    V(NotNumber, NOTNUMBER)                   \
    V(NotBool, NOTBOOL)                       \
    V(NotHeapObject, NOTHEAPOBJECT)           \
    V(NotStableArray, NOTSARRAY)              \
    V(NotArray, NOTARRAY)                     \
    V(NotOnHeap, NOTONHEAP)                   \
    V(InconsistentHClass, INCONSISTENTHCLASS) \
    V(NotNewObj, NOTNEWOBJ)                   \
    V(NotLegalIndex, NOTLEGALIDX)             \
    V(NotIncOverflow, NOTINCOV)               \
    V(NotDecOverflow, NOTDECOV)               \
    V(NotNegativeOverflow, NOTNEGOV)          \
    V(NotCallTarget, NOTCALLTGT)              \
    V(NotJSCallTarget, NOTJSCALLTGT)          \
    V(CowArray, COWARRAY)                     \
    V(DivideZero, DIVZERO)                    \
    V(InlineFail, INLINEFAIL)                 \
    V(NotJSFastCallTarget, NOTJSFASTCALLTGT)  \
    V(LexVarIsHole, LEXVARISHOLE)             \
    V(ModZero, MODZERO)                       \
    V(Int32Overflow, INT32OVERFLOW)           \
    V(NotString, NOTSTRING)

enum class DeoptType : uint8_t {
    NOTCHECK = 0,
#define DECLARE_DEOPT_TYPE(NAME, TYPE) TYPE,
    GATE_META_DATA_DEOPT_REASON(DECLARE_DEOPT_TYPE)
#undef DECLARE_DEOPT_TYPE
};

enum GateFlags : uint8_t {
    NONE_FLAG = 0,
    NO_WRITE = 1 << 0,
    HAS_ROOT = 1 << 1,
    HAS_FRAME_STATE = 1 << 2,
    CONTROL = NO_WRITE,
    CONTROL_ROOT = NO_WRITE | HAS_ROOT,
    CHECKABLE = NO_WRITE | HAS_FRAME_STATE,
    ROOT = NO_WRITE | HAS_ROOT,
    FIXED = NO_WRITE,
};

class GateMetaData : public ChunkObject {
public:
    enum class Kind : uint8_t {
        IMMUTABLE = 0,
        MUTABLE_WITH_SIZE,
        IMMUTABLE_ONE_PARAMETER,
        MUTABLE_ONE_PARAMETER,
        IMMUTABLE_BOOL,
        MUTABLE_STRING,
        JSBYTECODE,
        TYPED_BINARY_OP,
        TYPED_CALLTARGETCHECK_OP,
        TYPED_CALL,
    };
    GateMetaData() = default;
    GateMetaData(OpCode opcode, GateFlags flags,
        uint32_t statesIn, uint16_t dependsIn, uint32_t valuesIn)
        : opcode_(opcode), flags_(flags),
        statesIn_(statesIn), dependsIn_(dependsIn), valuesIn_(valuesIn) {}

    size_t GetStateCount() const
    {
        return statesIn_;
    }

    size_t GetDependCount() const
    {
        return dependsIn_;
    }

    size_t GetInValueCount() const
    {
        return valuesIn_;
    }

    size_t GetRootCount() const
    {
        return HasRoot() ? 1 : 0;
    }

    size_t GetInFrameStateCount() const
    {
        return HasFrameState() ? 1 : 0;
    }

    size_t GetNumIns() const
    {
        return GetStateCount() + GetDependCount() + GetInValueCount()
            + GetInFrameStateCount() + GetRootCount();
    }

    size_t GetInValueStarts() const
    {
        return GetStateCount() + GetDependCount();
    }

    size_t GetInFrameStateStarts() const
    {
        return GetInValueStarts() + GetInValueCount();
    }

    OpCode GetOpCode() const
    {
        return opcode_;
    }

    Kind GetKind() const
    {
        return kind_;
    }

    void AssertKind([[maybe_unused]] Kind kind) const
    {
        ASSERT(GetKind() == kind);
    }

    bool IsOneParameterKind() const
    {
        return GetKind() == Kind::IMMUTABLE_ONE_PARAMETER || GetKind() == Kind::MUTABLE_ONE_PARAMETER ||
            GetKind() == Kind::TYPED_BINARY_OP || GetKind() == Kind::TYPED_CALLTARGETCHECK_OP;
    }

    bool IsStringType() const
    {
        return GetKind() == Kind::MUTABLE_STRING;
    }

    bool IsRoot() const;
    bool IsProlog() const;
    bool IsFixed() const;
    bool IsSchedulable() const;
    bool IsState() const;  // note: IsState(STATE_ENTRY) == false
    bool IsGeneralState() const;
    bool IsTerminalState() const;
    bool IsVirtualState() const;
    bool IsCFGMerge() const;
    bool IsControlCase() const;
    bool IsLoopHead() const;
    bool IsNop() const;
    bool IsConstant() const;
    bool IsDependSelector() const;
    bool IsTypedOperator() const;
    bool IsCheckWithOneIn() const;
    bool IsCheckWithTwoIns() const;
    bool HasFrameState() const
    {
        return HasFlag(GateFlags::HAS_FRAME_STATE);
    }

    bool IsNotWrite() const
    {
        return HasFlag(GateFlags::NO_WRITE);
    }

    ~GateMetaData() = default;

    static std::string Str(OpCode opcode);
    std::string Str() const
    {
        return Str(opcode_);
    }
protected:
    void SetKind(Kind kind)
    {
        kind_ = kind;
    }

    void SetFlags(GateFlags flags)
    {
        flags_ = flags;
    }

    void DecreaseIn(size_t idx)
    {
        ASSERT(GetKind() == Kind::MUTABLE_WITH_SIZE);
        if (idx < statesIn_) {
            statesIn_--;
        } else if (idx < statesIn_ + dependsIn_) {
            dependsIn_--;
        } else {
            valuesIn_--;
        }
    }

    bool HasRoot() const
    {
        return HasFlag(GateFlags::HAS_ROOT);
    }

    bool HasFlag(GateFlags flag) const
    {
        return (GetFlags() & flag) == flag;
    }

    GateFlags GetFlags() const
    {
        return flags_;
    }

private:
    friend class Gate;
    friend class Circuit;
    friend class GateMetaBuilder;

    OpCode opcode_ { OpCode::NOP };
    Kind kind_ { Kind::IMMUTABLE };
    GateFlags flags_ { GateFlags::NONE_FLAG };
    uint32_t statesIn_ { 0 };
    uint32_t dependsIn_ { 0 };
    uint32_t valuesIn_ { 0 };
};

inline std::ostream& operator<<(std::ostream& os, OpCode opcode)
{
    return os << GateMetaData::Str(opcode);
}

class BoolMetaData : public GateMetaData {
public:
    BoolMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, bool value)
        : GateMetaData(opcode, flags, statesIn, dependsIn, valuesIn), value_(value)
    {
        SetKind(GateMetaData::Kind::IMMUTABLE_BOOL);
    }

    static const BoolMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::IMMUTABLE_BOOL);
        return static_cast<const BoolMetaData*>(meta);
    }

    bool GetBool() const
    {
        return value_;
    }

private:
    bool value_ { false };
};

class OneParameterMetaData : public GateMetaData {
public:
    OneParameterMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value)
        : GateMetaData(opcode, flags, statesIn, dependsIn, valuesIn), value_(value)
    {
        SetKind(GateMetaData::Kind::IMMUTABLE_ONE_PARAMETER);
    }

    static const OneParameterMetaData* Cast(const GateMetaData* meta)
    {
        ASSERT(meta->IsOneParameterKind());
        return static_cast<const OneParameterMetaData*>(meta);
    }

    uint64_t GetValue() const
    {
        return value_;
    }

    void SetValue(uint64_t value)
    {
        value_ = value;
    }

private:
    uint64_t value_ { 0 };
};

class StringMetaData : public GateMetaData {
public:
    StringMetaData(Chunk* chunk, const std::string &str)
        : GateMetaData(OpCode::CONSTSTRING, GateFlags::NONE_FLAG, 0, 0, 0),
        stringData_(str.size() + 1, chunk)
    {
        auto srcLength = str.size();
        auto destlength = stringData_.size();
        auto dest = stringData_.data();
        auto src = str.c_str();
        if (destlength <= static_cast<size_t>(srcLength) || strcpy_s(dest, destlength, src) != EOK) {
            LOG_COMPILER(FATAL) << "StringMetaData strcpy_s failed";
        }
        SetKind(GateMetaData::Kind::MUTABLE_STRING);
    }

    const ChunkVector<char> &GetString() const
    {
        return stringData_;
    }

private:
    ChunkVector<char> stringData_;
};

class GateTypeAccessor {
public:
    explicit GateTypeAccessor(uint64_t value)
        : type_(static_cast<uint32_t>(value)) {}

    GateType GetGateType() const
    {
        return type_;
    }

    static uint64_t ToValue(GateType type)
    {
        return static_cast<uint64_t>(type.Value());
    }
private:
    GateType type_;
};

class ValuePairTypeAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 8;
    explicit ValuePairTypeAccessor(uint64_t value) : bitField_(value) {}

    ValueType GetSrcType() const
    {
        return static_cast<ValueType>(LeftBits::Get(bitField_));
    }

    ValueType GetDstType() const
    {
        return static_cast<ValueType>(RightBits::Get(bitField_));
    }

    bool IsConvertSupport() const
    {
        return ConvertSupportBits::Get(bitField_) == ConvertSupport::ENABLE;
    }

    static uint64_t ToValue(ValueType srcType, ValueType dstType, ConvertSupport support = ConvertSupport::ENABLE)
    {
        uint8_t srcVlaue = static_cast<uint8_t>(srcType);
        uint8_t dstVlaue = static_cast<uint8_t>(dstType);
        return LeftBits::Encode(srcVlaue) | RightBits::Encode(dstVlaue) | ConvertSupportBits::Encode(support);
    }

private:
    using LeftBits = panda::BitField<uint8_t, 0, OPRAND_TYPE_BITS>;
    using RightBits = LeftBits::NextField<uint8_t, OPRAND_TYPE_BITS>;
    using ConvertSupportBits = RightBits::NextField<ConvertSupport, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class GatePairTypeAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit GatePairTypeAccessor(uint64_t value) : bitField_(value) {}

    GateType GetLeftType() const
    {
        return GateType(LeftBits::Get(bitField_));
    }

    GateType GetRightType() const
    {
        return GateType(RightBits::Get(bitField_));
    }

    static uint64_t ToValue(GateType leftType, GateType rightType)
    {
        return LeftBits::Encode(leftType.Value()) | RightBits::Encode(rightType.Value());
    }

private:
    using LeftBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using RightBits = LeftBits::NextField<uint32_t, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class UInt32PairAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit UInt32PairAccessor(uint64_t value) : bitField_(value) {}
    explicit UInt32PairAccessor(uint32_t first, uint32_t second)
    {
        bitField_ = FirstBits::Encode(first) | SecondBits::Encode(second);
    }

    uint32_t GetFirstValue() const
    {
        return FirstBits::Get(bitField_);
    }

    uint32_t GetSecondValue() const
    {
        return SecondBits::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using FirstBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using SecondBits = FirstBits::NextField<uint32_t, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class ArrayMetaDataAccessor {
public:
    enum Mode : uint8_t {
        CREATE = 0,
        LOAD_ELEMENT,
        STORE_ELEMENT,
        LOAD_LENGTH
    };

    static constexpr int BITS_SIZE = 8;
    static constexpr int ARRAY_LENGTH_BITS_SIZE = 32;
    explicit ArrayMetaDataAccessor(uint64_t value) : bitField_(value) {}
    explicit ArrayMetaDataAccessor(ElementsKind kind, Mode mode, uint32_t length = 0)
    {
        bitField_ = ElementsKindBits::Encode(kind) | ModeBits::Encode(mode) | ArrayLengthBits::Encode(length);
    }

    ElementsKind GetElementsKind() const
    {
        return ElementsKindBits::Get(bitField_);
    }

    void SetArrayLength(uint32_t length)
    {
        bitField_ = ArrayLengthBits::Update(bitField_, length);
    }

    uint32_t GetArrayLength() const
    {
        return ArrayLengthBits::Get(bitField_);
    }

    bool IsLoadElement() const
    {
        return GetMode() == Mode::LOAD_ELEMENT;
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    Mode GetMode() const
    {
        return ModeBits::Get(bitField_);
    }

    using ElementsKindBits = panda::BitField<ElementsKind, 0, BITS_SIZE>;
    using ModeBits = ElementsKindBits::NextField<Mode, BITS_SIZE>;
    using ArrayLengthBits = ModeBits::NextField<uint32_t, ARRAY_LENGTH_BITS_SIZE>;

    uint64_t bitField_;
};

class ObjectTypeAccessor {
public:
    static constexpr int TYPE_BITS_SIZE = 32;
    static constexpr int IS_HEAP_OBJECT_BIT_SIZE = 1;

    explicit ObjectTypeAccessor(uint64_t value) : bitField_(value) {}
    explicit ObjectTypeAccessor(GateType type, bool isHeapObject = false)
    {
        bitField_ = TypeBits::Encode(type.Value()) | IsHeapObjectBit::Encode(isHeapObject);
    }

    GateType GetType() const
    {
        return GateType(TypeBits::Get(bitField_));
    }

    bool IsHeapObject() const
    {
        return IsHeapObjectBit::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using TypeBits = panda::BitField<uint32_t, 0, TYPE_BITS_SIZE>;
    using IsHeapObjectBit = TypeBits::NextField<bool, IS_HEAP_OBJECT_BIT_SIZE>;

    uint64_t bitField_;
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_SHARE_GATE_META_DATA_H
