/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/share_opcodes.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/elements.h"
#include "ecmascript/js_thread_hclass_entries.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
using ProfileType = pgo::ProfileType;
using GateRef = int32_t;
using PGOTypeRef = pgo::PGOTypeRef;
using PGODefineOpType = pgo::PGODefineOpType;
using PGOSampleType = pgo::PGOSampleType;
using PGORWOpType = pgo::PGORWOpType;
enum class TypedBinOp : uint8_t;
enum class TypedUnOp : uint8_t;
enum class TypedJumpOp : uint8_t;
enum class TypedLoadOp : uint8_t;
enum class TypedStoreOp : uint8_t;
enum class TypedCallTargetCheckOp : uint8_t;

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
        CALL_NEW,
    };
    GateMetaData() = default;
    GateMetaData(OpCode opcode, GateFlags flags,
        uint32_t statesIn, uint16_t dependsIn, uint32_t valuesIn)
        : opcode_(opcode), flags_(flags),
        statesIn_(statesIn), dependsIn_(dependsIn), valuesIn_(valuesIn) {}

    virtual bool equal(const GateMetaData &other) const
    {
        if (opcode_ == other.opcode_ && kind_ == other.kind_ && flags_ == other.flags_ &&
            statesIn_ == other.statesIn_ && dependsIn_ == other.dependsIn_ && valuesIn_ == other.valuesIn_) {
            return true;
        }
        return false;
    }

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
            GetKind() == Kind::TYPED_BINARY_OP || GetKind() == Kind::TYPED_CALLTARGETCHECK_OP ||
            GetKind() == Kind::CALL_NEW;
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
    bool IsIfOrSwitchRelated() const;
    bool IsLoopHead() const;
    bool IsNop() const;
    bool IsDead() const;
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
    static std::string Str(TypedBinOp op);
    static std::string Str(TypedUnOp op);
    static std::string Str(TypedJumpOp op);
    static std::string Str(TypedLoadOp op);
    static std::string Str(TypedStoreOp op);
    static std::string Str(TypedCallTargetCheckOp op);
    static std::string Str(ValueType type);
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

    bool equal(const GateMetaData &other) const override
    {
        if (!GateMetaData::equal(other)) {
            return false;
        }
        auto cast_other = static_cast<const BoolMetaData *>(&other);
        if (value_ == cast_other->value_) {
            return true;
        }
        return false;
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

    void SetBool(bool value)
    {
        value_ = value;
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

    bool equal(const GateMetaData &other) const override
    {
        if (!GateMetaData::equal(other)) {
            return false;
        }
        auto cast_other = static_cast<const OneParameterMetaData *>(&other);
        if (value_ == cast_other->value_) {
            return true;
        }
        return false;
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
    StringMetaData(Chunk* chunk, std::string_view str)
        : GateMetaData(OpCode::CONSTSTRING, GateFlags::NONE_FLAG, 0, 0, 0),
        stringData_(str.size() + 1, chunk)
    {
        auto srcLength = str.size();
        auto destlength = stringData_.size();
        auto dest = stringData_.data();
        auto src = str.data();
        if (destlength <= static_cast<size_t>(srcLength) || strcpy_s(dest, destlength, src) != EOK) {
            LOG_COMPILER(FATAL) << "StringMetaData strcpy_s failed";
        }
        SetKind(GateMetaData::Kind::MUTABLE_STRING);
    }
    bool equal(const GateMetaData &other) const override
    {
        if (!GateMetaData::equal(other)) {
            return false;
        }
        auto cast_other = static_cast<const StringMetaData *>(&other);
        if (stringData_.size() != cast_other->GetString().size()) {
            return false;
        }

        if (strncmp(stringData_.data(), cast_other->GetString().data(), stringData_.size()) != 0) {
            return false;
        }

        return true;
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
        return GateType(type_);
    }

    ParamType GetParamType() const
    {
        return ParamType(type_);
    }
private:
    uint32_t type_;
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

class TypeConvertAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit TypeConvertAccessor(uint64_t value) : bitField_(value) {}

    ParamType GetLeftType() const
    {
        return ParamType(LeftBits::Get(bitField_));
    }

    GateType GetRightType() const
    {
        return GateType(RightBits::Get(bitField_));
    }

    static uint64_t ToValue(ParamType leftType, GateType rightType)
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
        LOAD_LENGTH,
        CALL_BUILTIN_METHOD
    };

    static constexpr int BITS_SIZE = 8;
    static constexpr int ARRAY_LENGTH_BITS_SIZE = 32;
    explicit ArrayMetaDataAccessor(uint64_t value) : bitField_(value) {}
    explicit ArrayMetaDataAccessor(ElementsKind kind, Mode mode,
                                   uint32_t length = 0, RegionSpaceFlag flag = RegionSpaceFlag::IN_YOUNG_SPACE)
    {
        bitField_ = ElementsKindBits::Encode(kind) | ModeBits::Encode(mode) |
                    ArrayLengthBits::Encode(length) | RegionSpaceFlagBits::Encode(flag);
    }

    RegionSpaceFlag GetRegionSpaceFlag() const
    {
        return RegionSpaceFlagBits::Get(bitField_);
    }

    ElementsKind GetElementsKind() const
    {
        return ElementsKindBits::Get(bitField_);
    }

    Mode GetMode() const
    {
        return ModeBits::Get(bitField_);
    }

    void SetElementsKind(ElementsKind kind)
    {
        bitField_ = ElementsKindBits::Update(bitField_, kind);
    }

    void SetArrayLength(uint32_t length)
    {
        bitField_ = ArrayLengthBits::Update(bitField_, length);
    }

    void SetRegionSpaceFlag(RegionSpaceFlag flag)
    {
        bitField_ = RegionSpaceFlagBits::Update(bitField_, flag);
    }

    uint32_t GetArrayLength() const
    {
        return ArrayLengthBits::Get(bitField_);
    }

    bool IsLoadElement() const
    {
        return GetMode() == Mode::LOAD_ELEMENT;
    }

    bool IsStoreElement() const
    {
        return GetMode() == Mode::STORE_ELEMENT;
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using ElementsKindBits = panda::BitField<ElementsKind, 0, BITS_SIZE>;
    using ModeBits = ElementsKindBits::NextField<Mode, BITS_SIZE>;
    using ArrayLengthBits = ModeBits::NextField<uint32_t, ARRAY_LENGTH_BITS_SIZE>;
    using RegionSpaceFlagBits = ArrayLengthBits::NextField<RegionSpaceFlag, BITS_SIZE>;

    uint64_t bitField_;
};

class CreateArgumentsAccessor {
public:
    enum Mode : uint8_t {
        REST_ARGUMENTS,
        UNMAPPED_ARGUMENTS,
        INVALID,
    };

    static constexpr int BITS_SIZE = 8;
    explicit CreateArgumentsAccessor(uint64_t value) : bitField_(value) {}
    explicit CreateArgumentsAccessor(ElementsKind kind, Mode mode)
    {
        bitField_ = ElementsKindBits::Encode(kind) | ModeBits::Encode(mode);
    }
    Mode GetMode() const
    {
        return ModeBits::Get(bitField_);
    }
    uint64_t ToValue() const
    {
        return bitField_;
    }
private:
    using ElementsKindBits = panda::BitField<ElementsKind, 0, BITS_SIZE>;
    using ModeBits = ElementsKindBits::NextField<Mode, BITS_SIZE>;

    uint64_t bitField_;
};

class ObjectTypeAccessor {
public:
    static constexpr int IS_HEAP_OBJECT_BIT_SIZE = 1;

    explicit ObjectTypeAccessor(uint64_t value) : bitField_(value) {}
    explicit ObjectTypeAccessor(bool isHeapObject = false)
    {
        bitField_ = IsHeapObjectBit::Encode(isHeapObject);
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
    using IsHeapObjectBit = panda::BitField<bool, 0, IS_HEAP_OBJECT_BIT_SIZE>;

    uint64_t bitField_;
};

class BuiltinPrototypeHClassAccessor {
public:
    static constexpr int WORD_BITS_SIZE = 8;
    static constexpr int IS_PROTOTYPE_OF_PROTOTYPE_BITS_SIZE = 1;

    explicit BuiltinPrototypeHClassAccessor(uint64_t value): type_(value) {}
    // Only valid indices accepted
    explicit BuiltinPrototypeHClassAccessor(BuiltinTypeId type, ElementsKind kind,
                                            bool isPrototypeOfPrototype): type_(0)
    {
        type_ = BuiltinTypeIdBits::Encode(type) | ElementsKindBits::Encode(kind) |
                IsPrototypeOfPrototypeBits::Encode(isPrototypeOfPrototype);
        type_ = BuiltinTypeIdBits::Encode(type) | ElementsKindBits::Encode(kind);
        ASSERT(BuiltinHClassEntries::GetEntryIndex(type) < BuiltinHClassEntries::N_ENTRIES);
    }

    ElementsKind GetElementsKind() const
    {
        return ElementsKindBits::Get(type_);
    }

    BuiltinTypeId GetBuiltinTypeId() const
    {
        return BuiltinTypeIdBits::Get(type_);
    }

    bool IsPrototypeOfPrototype() const
    {
        return IsPrototypeOfPrototypeBits::Get(type_);
    }

    uint64_t ToValue() const
    {
        return type_;
    }

private:
    using BuiltinTypeIdBits = panda::BitField<BuiltinTypeId, 0, WORD_BITS_SIZE>;
    using ElementsKindBits = BuiltinTypeIdBits::NextField<ElementsKind, WORD_BITS_SIZE>;
    using IsPrototypeOfPrototypeBits = ElementsKindBits::NextField<bool, IS_PROTOTYPE_OF_PROTOTYPE_BITS_SIZE>;

    uint64_t type_;
};

class TypedArrayMetaDataAccessor {
public:
    enum Mode : uint8_t {
        ACCESS_ELEMENT = 0,
        LOAD_LENGTH,
    };

    static constexpr int TYPE_BITS_SIZE = 32;
    static constexpr int MODE_BITS_SIZE = 2;
    static constexpr int ON_HEAP_MODE_BITS_SIZE = 2;

    explicit TypedArrayMetaDataAccessor(uint64_t value) : bitField_(value) {}

    ParamType GetParamType() const
    {
        return ParamType(TypeBits::Get(bitField_));
    }

    OnHeapMode GetOnHeapMode() const
    {
        return OnHeapModeBits::Get(bitField_);
    }

    uint64_t UpdateOnHeapMode(OnHeapMode mode)
    {
        return OnHeapModeBits::Update(bitField_, mode);
    }

    bool IsAccessElement() const
    {
        return ModeBits::Get(bitField_) == Mode::ACCESS_ELEMENT;
    }

    uint64_t ToValue()
    {
        return bitField_;
    }

    static uint64_t ToValue(ParamType paramType, Mode mode, OnHeapMode onHeap)
    {
        return TypeBits::Encode(paramType.Value()) | ModeBits::Encode(mode) | OnHeapModeBits::Encode(onHeap);
    }

private:
    using TypeBits = panda::BitField<uint32_t, 0, TYPE_BITS_SIZE>;
    using ModeBits = TypeBits::NextField<Mode, MODE_BITS_SIZE>;
    using OnHeapModeBits = ModeBits::NextField<OnHeapMode, ON_HEAP_MODE_BITS_SIZE>;

    uint64_t bitField_;
};

class LoadElementAccessor {
public:
    static constexpr int TYPED_LOAD_OP_BITS_SIZE = 8;
    static constexpr int ON_HEAP_MODE_BITS_SIZE = 8;

    explicit LoadElementAccessor(uint64_t value): bitField_(value) {}
    explicit LoadElementAccessor(TypedLoadOp op, OnHeapMode onHeap)
    {
        bitField_ = TypedLoadOpBits::Encode(op) | OnHeapModeBits::Encode(onHeap);
    }

    TypedLoadOp GetTypedLoadOp() const
    {
        return TypedLoadOpBits::Get(bitField_);
    }

    OnHeapMode GetOnHeapMode() const
    {
        return OnHeapModeBits::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using TypedLoadOpBits = panda::BitField<TypedLoadOp, 0, TYPED_LOAD_OP_BITS_SIZE>;
    using OnHeapModeBits = TypedLoadOpBits::NextField<OnHeapMode, ON_HEAP_MODE_BITS_SIZE>;

    uint64_t bitField_;
};

class StoreElementAccessor {
public:
    static constexpr int TYPED_STORE_OP_BITS_SIZE = 8;
    static constexpr int ON_HEAP_MODE_BITS_SIZE = 8;

    explicit StoreElementAccessor(uint64_t value): bitField_(value) {}
    explicit StoreElementAccessor(TypedStoreOp op, OnHeapMode onHeap)
    {
        bitField_ = TypedStoreOpBits::Encode(op) | OnHeapModeBits::Encode(onHeap);
    }

    TypedStoreOp GetTypedStoreOp() const
    {
        return TypedStoreOpBits::Get(bitField_);
    }

    OnHeapMode GetOnHeapMode() const
    {
        return OnHeapModeBits::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using TypedStoreOpBits = panda::BitField<TypedStoreOp, 0, TYPED_STORE_OP_BITS_SIZE>;
    using OnHeapModeBits = TypedStoreOpBits::NextField<OnHeapMode, ON_HEAP_MODE_BITS_SIZE>;

    uint64_t bitField_;
};

class StringStatusAccessor {
public:
    explicit StringStatusAccessor(uint64_t value = 0) : type_(value) {}

    uint32_t GetStringStatus() const
    {
        return static_cast<uint32_t>(type_);
    }

    uint64_t ToValue() const
    {
        return type_;
    }

private:
    uint64_t type_ {0};
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_SHARE_GATE_META_DATA_H
