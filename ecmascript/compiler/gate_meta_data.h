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

#define BINARY_GATE_META_DATA_CACHE_LIST(V)          \
    V(Add, ADD, false, 0, 0, 2)                      \
    V(Sub, SUB, false, 0, 0, 2)                      \
    V(Mul, MUL, false, 0, 0, 2)                      \
    V(Exp, EXP, false, 0, 0, 2)                      \
    V(Sdiv, SDIV, false, 0, 0, 2)                    \
    V(Smod, SMOD, false, 0, 0, 2)                    \
    V(Udiv, UDIV, false, 0, 0, 2)                    \
    V(Umod, UMOD, false, 0, 0, 2)                    \
    V(Fdiv, FDIV, false, 0, 0, 2)                    \
    V(Fmod, FMOD, false, 0, 0, 2)                    \
    V(And, AND, false, 0, 0, 2)                      \
    V(Xor, XOR, false, 0, 0, 2)                      \
    V(Or, OR, false, 0, 0, 2)                        \
    V(Lsl, LSL, false, 0, 0, 2)                      \
    V(Lsr, LSR, false, 0, 0, 2)                      \
    V(Asr, ASR, false, 0, 0, 2)

#define UNARY_GATE_META_DATA_CACHE_LIST(V)                        \
    V(Zext, ZEXT, false, 0, 0, 1)                                 \
    V(Sext, SEXT, false, 0, 0, 1)                                 \
    V(Trunc, TRUNC, false, 0, 0, 1)                               \
    V(Fext, FEXT, false, 0, 0, 1)                                 \
    V(Ftrunc, FTRUNC, false, 0, 0, 1)                             \
    V(Rev, REV, false, 0, 0, 1)                                   \
    V(TruncFloatToInt64, TRUNC_FLOAT_TO_INT64, false, 0, 0, 1)    \
    V(TaggedToInt64, TAGGED_TO_INT64, false, 0, 0, 1)             \
    V(Int64ToTagged, INT64_TO_TAGGED, false, 0, 0, 1)             \
    V(SignedIntToFloat, SIGNED_INT_TO_FLOAT, false, 0, 0, 1)      \
    V(UnsignedIntToFloat, UNSIGNED_INT_TO_FLOAT, false, 0, 0, 1)  \
    V(FloatToSignedInt, FLOAT_TO_SIGNED_INT, false, 0, 0, 1)      \
    V(UnsignedFloatToInt, UNSIGNED_FLOAT_TO_INT, false, 0, 0, 1)  \
    V(Bitcast, BITCAST, false, 0, 0, 1)

#define IMMUTABLE_META_DATA_CACHE_LIST(V)                   \
    V(CircuitRoot, CIRCUIT_ROOT, false, 0, 0, 0)            \
    V(StateEntry, STATE_ENTRY, true, 0, 0, 0)               \
    V(DependEntry, DEPEND_ENTRY, true, 0, 0, 0)             \
    V(ReturnList, RETURN_LIST, true, 0, 0, 0)               \
    V(ArgList, ARG_LIST, true, 0, 0, 0)                     \
    V(Return, RETURN, true, 1, 1, 1)                        \
    V(ReturnVoid, RETURN_VOID, true, 1, 1, 0)               \
    V(Throw, THROW, false, 1, 1, 1)                         \
    V(OrdinaryBlock, ORDINARY_BLOCK, false, 1, 0, 0)        \
    V(IfBranch, IF_BRANCH, false, 1, 0, 1)                  \
    V(IfTrue, IF_TRUE, false, 1, 0, 0)                      \
    V(IfFalse, IF_FALSE, false, 1, 0, 0)                    \
    V(LoopBegin, LOOP_BEGIN, false, 2, 0, 0)                \
    V(LoopBack, LOOP_BACK, false, 1, 0, 0)                  \
    V(DependRelay, DEPEND_RELAY, false, 1, 1, 0)            \
    V(DependAnd, DEPEND_AND, false, 0, 2, 0)                \
    V(IfSuccess, IF_SUCCESS, false, 1, 0, 0)                \
    V(IfException, IF_EXCEPTION, false, 1, 0, 0)            \
    V(GetException, GET_EXCEPTION, false, 0, 1, 0)          \
    V(Guard, GUARD, false, 0, 1, 3)                         \
    V(Deopt, DEOPT, false, 0, 1, 2)                         \
    V(Load, LOAD, false, 0, 1, 1)                           \
    V(Store, STORE, false, 0, 1, 2)                         \
    V(TypedCallCheck, TYPED_CALL_CHECK, false, 1, 1, 2)     \
    V(ArrayCheck, ARRAY_CHECK, false, 1, 1, 1)              \
    V(StableArrayCheck, STABLE_ARRAY_CHECK, false, 1, 1, 1) \
    V(LoadProperty, LOAD_PROPERTY, false, 1, 1, 2)          \
    V(StoreProperty, STORE_PROPERTY, false, 1, 1, 3)        \
    V(ToLength, TO_LENGTH, false, 1, 1, 1)                  \
    V(GetEnv, GET_ENV, false, 0, 1, 0)                      \
    V(DefaultCase, DEFAULT_CASE, false, 1, 0, 0)            \
    V(LoadArrayLength, LOAD_ARRAY_LENGTH, false, 1, 1, 1)   \
    BINARY_GATE_META_DATA_CACHE_LIST(V)                     \
    UNARY_GATE_META_DATA_CACHE_LIST(V)

#define GATE_META_DATA_LIST_WITH_VALUE_IN(V)                              \
    V(ValueSelector, VALUE_SELECTOR, false, 1, 0, value)                  \
    V(TypedCall, TYPED_CALL, false, 1, 1, value)                          \
    V(Construct, CONSTRUCT, false, 1, 1, value)                           \
    V(FrameState, FRAME_STATE, false, 0, 0, value)                        \
    V(RuntimeCall, RUNTIME_CALL, false, 0, 1, value)                      \
    V(RuntimeCallWithArgv, RUNTIME_CALL_WITH_ARGV, false, 0, 1, value)    \
    V(NoGcRuntimeCall, NOGC_RUNTIME_CALL, false, 0, 1, value)             \
    V(Call, CALL, false, 0, 1, value)                                     \
    V(BytecodeCall, BYTECODE_CALL, false, 0, 1, value)                    \
    V(DebuggerBytecodeCall, DEBUGGER_BYTECODE_CALL, false, 0, 1, value)   \
    V(BuiltinsCallWithArgv, BUILTINS_CALL_WITH_ARGV, false, 0, 1, value)  \
    V(BuiltinsCall, BUILTINS_CALL, false, 0, 1, value)

#define GATE_META_DATA_LIST_WITH_SIZE(V)                                  \
    V(Merge, MERGE, false, value, 0, 0)                                   \
    V(DependSelector, DEPEND_SELECTOR, false, 1, value, 0)                \
    GATE_META_DATA_LIST_WITH_VALUE_IN(V)

#define GATE_META_DATA_LIST_WITH_GATE_TYPE(V)                   \
    V(PrimitiveTypeCheck, PRIMITIVE_TYPE_CHECK, false, 0, 0, 1) \
    V(ObjectTypeCheck, OBJECT_TYPE_CHECK, false, 1, 1, 2)       \
    V(TypedArrayCheck, TYPED_ARRAY_CHECK, false, 1, 1, 1)       \
    V(IndexCheck, INDEX_CHECK, false, 1, 1, 2)                  \
    V(TypedUnaryOp, TYPED_UNARY_OP, false, 1, 1, 1)             \
    V(TypedConvert, TYPE_CONVERT, false, 1, 1, 1)               \

#define GATE_META_DATA_LIST_WITH_VALUE(V)                 \
    V(Icmp, ICMP, false, 0, 0, 2)                         \
    V(Fcmp, FCMP, false, 0, 0, 2)                         \
    V(Alloca, ALLOCA, false, 0, 0, 0)                     \
    V(SwitchBranch, SWITCH_BRANCH, false, 1, 0, 1)        \
    V(SwitchCase, SWITCH_CASE, false, 1, 0, 0)            \
    V(HeapAlloc, HEAP_ALLOC, false, 1, 1, 1)              \
    V(LoadElement, LOAD_ELEMENT, false, 1, 1, 2)          \
    V(StoreElement, STORE_ELEMENT, false, 1, 1, 3)        \
    V(ConstData, CONST_DATA, false, 0, 0, 0)              \
    V(Constant, CONSTANT, false, 0, 0, 0)                 \
    V(RelocatableData, RELOCATABLE_DATA, false, 0, 0, 0)

#define GATE_META_DATA_LIST_WITH_ONE_PARAMETER(V)         \
    V(Arg, ARG, true, 0, 0, 0)                            \
    GATE_META_DATA_LIST_WITH_VALUE(V)                     \
    GATE_META_DATA_LIST_WITH_GATE_TYPE(V)

#define GATE_META_DATA_LIST_WITH_STRING(V)                \
    V(ConstString, CONSTSTRING, false, 0, 0, 0)

#define GATE_META_DATA_IN_SAVE_REGISTER(V)         \
    V(SaveRegister, SAVE_REGISTER, false, 0, 1, value)

#define GATE_META_DATA_IN_RESTORE_REGISTER(V)      \
    V(RestoreRegister, RESTORE_REGISTER, false, 0, 1, 0)

#define GATE_OPCODE_LIST(V)     \
    V(JS_BYTECODE)              \
    V(TYPED_BINARY_OP)

enum class OpCode : uint8_t {
    NOP = 0,
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) OP,
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_STRING(DECLARE_GATE_OPCODE)
    GATE_META_DATA_IN_SAVE_REGISTER(DECLARE_GATE_OPCODE)
    GATE_META_DATA_IN_RESTORE_REGISTER(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
#define DECLARE_GATE_OPCODE(NAME) NAME,
    GATE_OPCODE_LIST(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
};

class GateMetaData : public ChunkObject {
public:
    enum Kind {
        IMMUTABLE = 0,
        MUTABLE_WITH_SIZE,
        IMMUTABLE_ONE_PARAMETER,
        MUTABLE_ONE_PARAMETER,
        MUTABLE_STRING,
        JSBYTECODE,
        TYPED_BINARY_OP,
        SAVE_REGISTERS_OP,
        RESTORE_REGISTERS_OP,
    };
    GateMetaData() = default;
    explicit GateMetaData(OpCode opcode, bool hasRoot,
        uint32_t statesIn, uint16_t dependsIn, uint32_t valuesIn)
        : opcode_(opcode), bitField_(HasRootBit::Encode(hasRoot)),
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
        return HasRootBit::Get(bitField_) ? 1 : 0;
    }

    size_t GetNumIns() const
    {
        return GetStateCount() + GetDependCount() + GetInValueCount() + GetRootCount();
    }

    size_t GetInValueStarts() const
    {
        return GetStateCount() + GetDependCount();
    }

    OpCode GetOpCode() const
    {
        return opcode_;
    }

    Kind GetKind() const
    {
        return KindBits::Get(bitField_);
    }

    void AssertKind([[maybe_unused]] Kind kind) const
    {
        ASSERT(GetKind() == kind);
    }

    bool IsOneParameterKind() const
    {
        return GetKind() == IMMUTABLE_ONE_PARAMETER || GetKind() == MUTABLE_ONE_PARAMETER ||
            GetKind() == TYPED_BINARY_OP;
    }

    bool IsStringType() const
    {
        return GetKind() == MUTABLE_STRING;
    }

    bool IsSaveRegisterOp() const
    {
        return GetKind() == SAVE_REGISTERS_OP;
    }

    bool IsRestoreRegisterOp() const
    {
        return GetKind() == RESTORE_REGISTERS_OP;
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
    bool IsTypedOperator() const;
    ~GateMetaData() = default;

    static std::string Str(OpCode opcode);
    std::string Str() const
    {
        return Str(opcode_);
    }
protected:
    void SetKind(Kind kind)
    {
        KindBits::Set<uint8_t>(kind, &bitField_);
    }

    bool HasRoot() const
    {
        return HasRootBit::Get(bitField_);
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

private:
    friend class Gate;
    friend class Circuit;
    friend class GateMetaBuilder;

    using KindBits = panda::BitField<Kind, 0, 4>; // 4: type width
    using HasRootBit = KindBits::NextFlag;

    OpCode opcode_ { OpCode::NOP };
    uint8_t bitField_ { 0 };
    uint32_t statesIn_ { 0 };
    uint16_t dependsIn_ { 0 };
    uint32_t valuesIn_ { 0 };
};

inline std::ostream& operator<<(std::ostream& os, OpCode opcode)
{
    return os << GateMetaData::Str(opcode);
}

class JSBytecodeMegaData : public GateMetaData {
public:
    explicit JSBytecodeMegaData(size_t valuesIn, EcmaOpcode opcode, uint32_t bcIndex)
        : GateMetaData(OpCode::JS_BYTECODE, false, 1, 1, valuesIn),
        opcode_(opcode), bcIndex_(bcIndex)
    {
        SetKind(GateMetaData::JSBYTECODE);
    }

    static const JSBytecodeMegaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::JSBYTECODE);
        return static_cast<const JSBytecodeMegaData*>(meta);
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
    explicit OneParameterMetaData(OpCode opcode, bool hasRoot, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value)
        : GateMetaData(opcode, hasRoot, statesIn, dependsIn, valuesIn), value_(value)
    {
        SetKind(GateMetaData::IMMUTABLE_ONE_PARAMETER);
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

class SaveRegsMetaData : public GateMetaData {
public:
    explicit SaveRegsMetaData(OpCode opcode, bool hasRoot, uint32_t statesIn, uint16_t dependsIn, uint32_t valuesIn,
        uint64_t value) : GateMetaData(opcode, hasRoot, statesIn, dependsIn, valuesIn), numValue_(value)
    {
        SetKind(GateMetaData::SAVE_REGISTERS_OP);
    }

    static const SaveRegsMetaData* Cast(const GateMetaData* meta)
    {
        ASSERT(meta->IsSaveRegisterOp());
        return static_cast<const SaveRegsMetaData*>(meta);
    }

    uint64_t GetNumValue() const
    {
        return numValue_;
    }
private:
    uint64_t numValue_ { 0 };
};

class RestoreRegsMetaData : public GateMetaData {
public:
    explicit RestoreRegsMetaData() : GateMetaData(OpCode::RESTORE_REGISTER, false, 0, 1, 0)
    {
        SetKind(GateMetaData::RESTORE_REGISTERS_OP);
    }

    static const RestoreRegsMetaData* Cast(const GateMetaData* meta)
    {
        ASSERT(meta->IsRestoreRegisterOp());
        return static_cast<const RestoreRegsMetaData*>(meta);
    }

    const std::map<std::pair<GateRef, uint32_t>, uint32_t> &GetRestoreRegsInfo() const
    {
        return restoreRegsInfo_;
    }

    void SetRestoreRegsInfo(std::pair<GateRef, uint32_t> &needReplaceInfo, uint32_t regIdx)
    {
        restoreRegsInfo_[needReplaceInfo] = regIdx;
    }
private:
    std::map<std::pair<GateRef, uint32_t>, uint32_t> restoreRegsInfo_;
};


class TypedBinaryMegaData : public OneParameterMetaData {
public:
    explicit TypedBinaryMegaData(uint64_t value, TypedBinOp binOp)
        : OneParameterMetaData(OpCode::TYPED_BINARY_OP, false, 1, 1, 2, value), // 2: valuesIn
        binOp_(binOp)
    {
        SetKind(GateMetaData::TYPED_BINARY_OP);
    }

    static const TypedBinaryMegaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::TYPED_BINARY_OP);
        return static_cast<const TypedBinaryMegaData*>(meta);
    }

    TypedBinOp GetTypedBinaryOp() const
    {
        return binOp_;
    }
private:
    TypedBinOp binOp_;
};

class StringMetaData : public GateMetaData {
public:
    explicit StringMetaData(OpCode opcode, bool hasRoot, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, const std::string &str)
        : GateMetaData(opcode, hasRoot, statesIn, dependsIn, valuesIn), str_(str)
    {
        SetKind(GateMetaData::MUTABLE_STRING);
    }

    std::string GetString() const
    {
        return str_;
    }

private:
    std::string str_;
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