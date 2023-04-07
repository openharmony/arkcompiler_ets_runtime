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

#ifndef ECMASCRIPT_COMPILER_GATE_META_DATA_H
#define ECMASCRIPT_COMPILER_GATE_META_DATA_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
using GateRef = int32_t;
enum MachineType : uint8_t { // Bit width
    NOVALUE = 0,
    ANYVALUE,
    ARCH,
    FLEX,
    I1,
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
};

enum class TypedBinOp : uint8_t {
    TYPED_ADD = 0,
    TYPED_SUB,
    TYPED_MUL,
    TYPED_DIV,
    TYPED_MOD,
    TYPED_LESS,
    TYPED_LESSEQ,
    TYPED_GREATER,
    TYPED_GREATEREQ,
    TYPED_EQ,
    TYPED_NOTEQ,
    TYPED_SHL,
    TYPED_SHR,
    TYPED_ASHR,
    TYPED_AND,
    TYPED_OR,
    TYPED_XOR,
    TYPED_EXP,
};

enum class TypedUnOp : uint8_t {
    TYPED_TONUMBER = 0,
    TYPED_NEG,
    TYPED_NOT,
    TYPED_INC,
    TYPED_DEC,
    TYPED_TOBOOL,
};

enum class DeoptType : uint8_t {
    NOTCHECK = 0,
    NOTINT,
    NOTDOUBLE,
    NOTNUMBER,
    NOTBOOL,
    NOTARRAY,
    NOTSARRAY,
    NOTF32ARRAY,
    WRONGHCLASS,
    NOTNEWOBJ,
    NOTARRAYIDX,
    NOTF32ARRAYIDX,
    NOTINCOV,
    NOTDECOV,
    NOTNEGOV,
    NOTCALLTGT,
};

enum class ICmpCondition : uint8_t {
    EQ = 1,
    UGT,
    UGE,
    ULT,
    ULE,
    NE,
    SGT,
    SGE,
    SLT,
    SLE,
};

enum class FCmpCondition : uint8_t {
    ALW_FALSE = 0,
    OEQ,
    OGT,
    OGE,
    OLT,
    OLE,
    ONE,
    ORD,
    UNO,
    UEQ,
    UGT,
    UGE,
    ULT,
    ULE,
    UNE,
    ALW_TRUE,
};

enum class TypedStoreOp : uint8_t {
    ARRAY_STORE_ELEMENT = 0,
    FLOAT32ARRAY_STORE_ELEMENT,
};

enum class TypedLoadOp : uint8_t {
    ARRAY_LOAD_ELEMENT = 0,
    FLOAT32ARRAY_LOAD_ELEMENT,
};

std::string MachineTypeToStr(MachineType machineType);

#define BINARY_GATE_META_DATA_CACHE_LIST(V)                         \
    V(Add, ADD, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Sub, SUB, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Mul, MUL, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Exp, EXP, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Sdiv, SDIV, GateFlags::NONE_FLAG, 0, 0, 2)                    \
    V(Smod, SMOD, GateFlags::NONE_FLAG, 0, 0, 2)                    \
    V(Udiv, UDIV, GateFlags::NONE_FLAG, 0, 0, 2)                    \
    V(Umod, UMOD, GateFlags::NONE_FLAG, 0, 0, 2)                    \
    V(Fdiv, FDIV, GateFlags::NONE_FLAG, 0, 0, 2)                    \
    V(Fmod, FMOD, GateFlags::NONE_FLAG, 0, 0, 2)                    \
    V(And, AND, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Xor, XOR, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Or, OR, GateFlags::NONE_FLAG, 0, 0, 2)                        \
    V(Lsl, LSL, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Lsr, LSR, GateFlags::NONE_FLAG, 0, 0, 2)                      \
    V(Asr, ASR, GateFlags::NONE_FLAG, 0, 0, 2)

#define UNARY_GATE_META_DATA_CACHE_LIST(V)                                       \
    V(Zext, ZEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Sext, SEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Trunc, TRUNC, GateFlags::NONE_FLAG, 0, 0, 1)                               \
    V(Fext, FEXT, GateFlags::NONE_FLAG, 0, 0, 1)                                 \
    V(Ftrunc, FTRUNC, GateFlags::NONE_FLAG, 0, 0, 1)                             \
    V(Rev, REV, GateFlags::NONE_FLAG, 0, 0, 1)                                   \
    V(TruncFloatToInt64, TRUNC_FLOAT_TO_INT64, GateFlags::NONE_FLAG, 0, 0, 1)    \
    V(TaggedToInt64, TAGGED_TO_INT64, GateFlags::NONE_FLAG, 0, 0, 1)             \
    V(Int64ToTagged, INT64_TO_TAGGED, GateFlags::NONE_FLAG, 0, 0, 1)             \
    V(SignedIntToFloat, SIGNED_INT_TO_FLOAT, GateFlags::NONE_FLAG, 0, 0, 1)      \
    V(UnsignedIntToFloat, UNSIGNED_INT_TO_FLOAT, GateFlags::NONE_FLAG, 0, 0, 1)  \
    V(FloatToSignedInt, FLOAT_TO_SIGNED_INT, GateFlags::NONE_FLAG, 0, 0, 1)      \
    V(UnsignedFloatToInt, UNSIGNED_FLOAT_TO_INT, GateFlags::NONE_FLAG, 0, 0, 1)  \
    V(Bitcast, BITCAST, GateFlags::NONE_FLAG, 0, 0, 1)

#define IMMUTABLE_META_DATA_CACHE_LIST(V)                                               \
    V(CircuitRoot, CIRCUIT_ROOT, GateFlags::NONE_FLAG, 0, 0, 0)                         \
    V(StateEntry, STATE_ENTRY, GateFlags::ROOT, 0, 0, 0)                                \
    V(DependEntry, DEPEND_ENTRY, GateFlags::ROOT, 0, 0, 0)                              \
    V(ReturnList, RETURN_LIST, GateFlags::ROOT, 0, 0, 0)                                \
    V(ArgList, ARG_LIST, GateFlags::ROOT, 0, 0, 0)                                      \
    V(Return, RETURN, GateFlags::HAS_ROOT, 1, 1, 1)                                     \
    V(ReturnVoid, RETURN_VOID, GateFlags::HAS_ROOT, 1, 1, 0)                            \
    V(Throw, THROW, GateFlags::CONTROL, 1, 1, 1)                                        \
    V(OrdinaryBlock, ORDINARY_BLOCK, GateFlags::CONTROL, 1, 0, 0)                       \
    V(IfBranch, IF_BRANCH, GateFlags::CONTROL, 1, 0, 1)                                 \
    V(IfTrue, IF_TRUE, GateFlags::CONTROL, 1, 0, 0)                                     \
    V(IfFalse, IF_FALSE, GateFlags::CONTROL, 1, 0, 0)                                   \
    V(LoopBegin, LOOP_BEGIN, GateFlags::CONTROL, 2, 0, 0)                               \
    V(LoopBack, LOOP_BACK, GateFlags::CONTROL, 1, 0, 0)                                 \
    V(DependRelay, DEPEND_RELAY, GateFlags::FIXED, 1, 1, 0)                             \
    V(DependAnd, DEPEND_AND, GateFlags::FIXED, 0, 2, 0)                                 \
    V(IfSuccess, IF_SUCCESS, GateFlags::CONTROL, 1, 0, 0)                               \
    V(IfException, IF_EXCEPTION, GateFlags::CONTROL, 1, 0, 0)                           \
    V(GetException, GET_EXCEPTION, GateFlags::NONE_FLAG, 0, 1, 0)                       \
    V(StateSplit, STATE_SPLIT, GateFlags::CHECKABLE, 0, 1, 0)                           \
    V(Deopt, DEOPT, GateFlags::NONE_FLAG, 0, 1, 3)                                      \
    V(Load, LOAD, GateFlags::NONE_FLAG, 0, 1, 1)                                        \
    V(Store, STORE, GateFlags::NONE_FLAG, 0, 1, 2)                                      \
    V(TypedCallCheck, TYPED_CALL_CHECK, GateFlags::CHECKABLE, 1, 1, 3)                  \
    V(ArrayCheck, ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)                           \
    V(StableArrayCheck, STABLE_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)              \
    V(DeoptCheck, DEOPT_CHECK, GateFlags::NONE_FLAG, 1, 1, 3)                           \
    V(LoadProperty, LOAD_PROPERTY, GateFlags::NO_WRITE, 1, 1, 2)                        \
    V(StoreProperty, STORE_PROPERTY, GateFlags::NONE_FLAG, 1, 1, 3)                     \
    V(ToLength, TO_LENGTH, GateFlags::NONE_FLAG, 1, 1, 1)                               \
    V(DefaultCase, DEFAULT_CASE, GateFlags::CONTROL, 1, 0, 0)                           \
    V(LoadArrayLength, LOAD_ARRAY_LENGTH, GateFlags::NO_WRITE, 1, 1, 1)                 \
    V(TypedNewAllocateThis, TYPED_NEW_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 2)     \
    V(TypedSuperAllocateThis, TYPED_SUPER_ALLOCATE_THIS, GateFlags::CHECKABLE, 1, 1, 2) \
    V(GetSuperConstructor, GET_SUPER_CONSTRUCTOR, GateFlags::NO_WRITE, 1, 1, 1)         \
    V(UpdateHotness, UPDATE_HOTNESS, GateFlags::NO_WRITE, 1, 1, 1)                      \
    BINARY_GATE_META_DATA_CACHE_LIST(V)                                                 \
    UNARY_GATE_META_DATA_CACHE_LIST(V)

#define GATE_META_DATA_LIST_WITH_VALUE_IN(V)                                             \
    V(ValueSelector, VALUE_SELECTOR, GateFlags::FIXED, 1, 0, value)                      \
    V(TypedCall, TYPED_CALL, GateFlags::NONE_FLAG, 1, 1, value)                          \
    V(Construct, CONSTRUCT, GateFlags::NONE_FLAG, 1, 1, value)                           \
    V(FrameState, FRAME_STATE, GateFlags::NONE_FLAG, 0, 0, value)                        \
    V(RuntimeCall, RUNTIME_CALL, GateFlags::NONE_FLAG, 0, 1, value)                      \
    V(RuntimeCallWithArgv, RUNTIME_CALL_WITH_ARGV, GateFlags::NONE_FLAG, 0, 1, value)    \
    V(NoGcRuntimeCall, NOGC_RUNTIME_CALL, GateFlags::NONE_FLAG, 0, 1, value)             \
    V(Call, CALL, GateFlags::NONE_FLAG, 0, 1, value)                                     \
    V(BytecodeCall, BYTECODE_CALL, GateFlags::NONE_FLAG, 0, 1, value)                    \
    V(DebuggerBytecodeCall, DEBUGGER_BYTECODE_CALL, GateFlags::NONE_FLAG, 0, 1, value)   \
    V(BuiltinsCallWithArgv, BUILTINS_CALL_WITH_ARGV, GateFlags::NONE_FLAG, 0, 1, value)  \
    V(BuiltinsCall, BUILTINS_CALL, GateFlags::NONE_FLAG, 0, 1, value)                    \
    V(SaveRegister, SAVE_REGISTER, GateFlags::NONE_FLAG, 0, 1, value)                    \

#define GATE_META_DATA_LIST_WITH_SIZE(V)                                            \
    V(Merge, MERGE, GateFlags::CONTROL, value, 0, 0)                                \
    V(DependSelector, DEPEND_SELECTOR, GateFlags::FIXED, 1, value, 0)               \
    GATE_META_DATA_LIST_WITH_VALUE_IN(V)

#define GATE_META_DATA_LIST_WITH_GATE_TYPE(V)                                  \
    V(PrimitiveTypeCheck, PRIMITIVE_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 1) \
    V(ObjectTypeCheck, OBJECT_TYPE_CHECK, GateFlags::CHECKABLE, 1, 1, 2)       \
    V(TypedArrayCheck, TYPED_ARRAY_CHECK, GateFlags::CHECKABLE, 1, 1, 1)       \
    V(IndexCheck, INDEX_CHECK, GateFlags::CHECKABLE, 1, 1, 2)                  \
    V(Int32OverflowCheck, INT32_OVERFLOW_CHECK, GateFlags::CHECKABLE, 1, 1, 1) \
    V(TypedUnaryOp, TYPED_UNARY_OP, GateFlags::NO_WRITE, 1, 1, 1)              \
    V(TypedConvert, TYPE_CONVERT, GateFlags::NO_WRITE, 1, 1, 1)                \

#define GATE_META_DATA_LIST_WITH_VALUE(V)                                \
    V(Icmp, ICMP, GateFlags::NONE_FLAG, 0, 0, 2)                         \
    V(Fcmp, FCMP, GateFlags::NONE_FLAG, 0, 0, 2)                         \
    V(Alloca, ALLOCA, GateFlags::NONE_FLAG, 0, 0, 0)                     \
    V(SwitchBranch, SWITCH_BRANCH, GateFlags::CONTROL, 1, 0, 1)          \
    V(SwitchCase, SWITCH_CASE, GateFlags::CONTROL, 1, 0, 0)              \
    V(HeapAlloc, HEAP_ALLOC, GateFlags::NONE_FLAG, 1, 1, 1)              \
    V(LoadElement, LOAD_ELEMENT, GateFlags::NO_WRITE, 1, 1, 2)           \
    V(StoreElement, STORE_ELEMENT, GateFlags::NONE_FLAG, 1, 1, 3)        \
    V(RestoreRegister, RESTORE_REGISTER, GateFlags::NONE_FLAG, 0, 1, 0)  \
    V(ConstData, CONST_DATA, GateFlags::NONE_FLAG, 0, 0, 0)              \
    V(Constant, CONSTANT, GateFlags::NONE_FLAG, 0, 0, 0)                 \
    V(RelocatableData, RELOCATABLE_DATA, GateFlags::NONE_FLAG, 0, 0, 0)

#define GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    V(Arg, ARG, GateFlags::HAS_ROOT, 0, 0, 0)             \
    GATE_META_DATA_LIST_WITH_VALUE(V)                     \
    GATE_META_DATA_LIST_WITH_GATE_TYPE(V)

#define GATE_OPCODE_LIST(V)     \
    V(JS_BYTECODE)              \
    V(TYPED_BINARY_OP)          \
    V(CONSTSTRING)

enum class OpCode : uint8_t {
    NOP = 0,
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) OP,
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
#define DECLARE_GATE_OPCODE(NAME) NAME,
    GATE_OPCODE_LIST(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
};

enum GateFlags : uint8_t {
    NONE_FLAG = 0,
    NO_WRITE = 1 << 0,
    HAS_ROOT = 1 << 1,
    HAS_FRAME_STATE = 1 << 2,
    CONTROL = NO_WRITE,
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
        MUTABLE_STRING,
        JSBYTECODE,
        TYPED_BINARY_OP,
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
            GetKind() == Kind::TYPED_BINARY_OP;
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

class JSBytecodeMetaData : public GateMetaData {
public:
    explicit JSBytecodeMetaData(size_t valuesIn, EcmaOpcode opcode, uint32_t bcIndex, GateFlags flags)
        : GateMetaData(OpCode::JS_BYTECODE, flags, 1, 1, valuesIn),
        opcode_(opcode), bcIndex_(bcIndex)
    {
        SetKind(GateMetaData::Kind::JSBYTECODE);
    }

    static const JSBytecodeMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::JSBYTECODE);
        return static_cast<const JSBytecodeMetaData*>(meta);
    }

    uint32_t GetBytecodeIndex() const
    {
        return bcIndex_;
    }

    EcmaOpcode GetByteCodeOpcode() const
    {
        return opcode_;
    }
private:
    EcmaOpcode opcode_;
    uint32_t bcIndex_;
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

private:
    uint64_t value_ { 0 };
};

class TypedBinaryMegaData : public OneParameterMetaData {
public:
    TypedBinaryMegaData(uint64_t value, TypedBinOp binOp)
        : OneParameterMetaData(OpCode::TYPED_BINARY_OP, GateFlags::NO_WRITE, 1, 1, 2, value), // 2: valuesIn
        binOp_(binOp)
    {
        SetKind(GateMetaData::Kind::TYPED_BINARY_OP);
    }

    static const TypedBinaryMegaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_BINARY_OP);
        return static_cast<const TypedBinaryMegaData*>(meta);
    }

    TypedBinOp GetTypedBinaryOp() const
    {
        return binOp_;
    }
private:
    TypedBinOp binOp_;
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

class TypedUnaryAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit TypedUnaryAccessor(uint64_t value) : bitField_(value) {}

    GateType GetTypeValue() const
    {
        return GateType(TypedValueBits::Get(bitField_));
    }

    TypedUnOp GetTypedUnOp() const
    {
        return TypedUnOpBits::Get(bitField_);
    }

    static uint64_t ToValue(GateType typeValue, TypedUnOp unaryOp)
    {
        return TypedValueBits::Encode(typeValue.Value())
            | TypedUnOpBits::Encode(unaryOp);
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedUnOpBits = TypedValueBits::NextField<TypedUnOp, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GATE_META_DATA_H
