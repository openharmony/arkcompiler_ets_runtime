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
enum MachineType : uint8_t { // Bit width
    NOVALUE,
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

#define IMMUTABLE_META_DATA_CACHE_LIST(V)               \
    V(CircuitRoot, CIRCUIT_ROOT, false, 0, 0, 0)        \
    V(StateEntry, STATE_ENTRY, true, 0, 0, 0)           \
    V(DependEntry, DEPEND_ENTRY, true, 0, 0, 0)         \
    V(ReturnList, RETURN_LIST, true, 0, 0, 0)           \
    V(ArgList, ARG_LIST, true, 0, 0, 0)                 \
    V(Return, RETURN, true, 1, 1, 1)                    \
    V(ReturnVoid, RETURN_VOID, true, 1, 1, 0)           \
    V(Throw, THROW, false, 1, 1, 1)                     \
    V(OrdinaryBlock, ORDINARY_BLOCK, false, 1, 0, 0)    \
    V(IfBranch, IF_BRANCH, false, 1, 0, 1)              \
    V(IfTrue, IF_TRUE, false, 1, 0, 0)                  \
    V(IfFalse, IF_FALSE, false, 1, 0, 0)                \
    V(LoopBegin, LOOP_BEGIN, false, 2, 0, 0)            \
    V(LoopBack, LOOP_BACK, false, 1, 0, 0)              \
    V(DependRelay, DEPEND_RELAY, false, 1, 1, 0)        \
    V(DependAnd, DEPEND_AND, false, 0, 2, 0)            \
    V(IfSuccess, IF_SUCCESS, false, 1, 0, 0)            \
    V(IfException, IF_EXCEPTION, false, 1, 0, 0)        \
    V(GetException, GET_EXCEPTION, false, 0, 1, 0)      \
    V(Guard, GUARD, false, 0, 1, 3)                     \
    V(Deopt, DEOPT, false, 0, 1, 2)                     \
    V(Load, LOAD, false, 0, 1, 1)                       \
    V(Store, STORE, false, 0, 1, 2)                     \
    V(TypedCallCheck, TYPED_CALL_CHECK, false, 1, 1, 2) \
    V(LoadProperty, LOAD_PROPERTY, false, 1, 1, 2)      \
    V(StoreProperty, STORE_PROPERTY, false, 1, 1, 3)    \
    V(ToLength, TO_LENGTH, false, 1, 1, 1)              \
    V(GetEnv, GET_ENV, false, 0, 1, 0)                  \
    V(DefaultCase, DEFAULT_CASE, false, 1, 0, 0)        \
    BINARY_GATE_META_DATA_CACHE_LIST(V)                 \
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

#define GATE_META_DATA_LIST_WITH_ONE_VALUE(V)             \
    V(Icmp, ICMP, false, 0, 0, 2)                         \
    V(Fcmp, FCMP, false, 0, 0, 2)                         \
    V(TypeCheck, TYPE_CHECK, false, 0, 0, 1)              \
    V(TypedConvert, TYPE_CONVERT, false, 1, 1, 1)         \
    V(TypedUnaryOp, TYPED_UNARY_OP, false, 1, 1, 1)       \
    V(TypedBinaryOp, TYPED_BINARY_OP, false, 1, 1, 3)     \
    V(ObjectTypeCheck, OBJECT_TYPE_CHECK, false, 1, 1, 2) \
    V(Arg, ARG, true, 0, 0, 0)                            \
    V(Alloca, ALLOCA, false, 0, 0, 0)                     \
    V(SwitchBranch, SWITCH_BRANCH, false, 1, 0, 1)        \
    V(SwitchCase, SWITCH_CASE, false, 1, 0, 0)            \
    V(HeapAlloc, HEAP_ALLOC, false, 1, 1, 1)              \
    V(RestoreRegister, RESTORE_REGISTER, false, 0, 1, 0)  \
    V(SaveRegister, SAVE_REGISTER, false, 0, 1, 1)        \
    V(LoadElement, LOAD_ELEMENT, false, 1, 1, 2)          \
    V(StoreElement, STORE_ELEMENT, false, 1, 1, 3)        \
    V(ConstData, CONST_DATA, false, 0, 0, 0)              \
    V(Constant, CONSTANT, false, 0, 0, 0)                 \
    V(RelocatableData, RELOCATABLE_DATA, false, 0, 0, 0)

#define GATE_OPCODE_LIST(V)     \
    V(JS_BYTECODE)              \


enum class OpCode : uint8_t {
    NOP = 0,
#define DECLARE_GATE_OPCODE(NAME, OP, R, S, D, V) OP,
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_OPCODE)
    GATE_META_DATA_LIST_WITH_ONE_VALUE(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
#define DECLARE_GATE_OPCODE(NAME) NAME,
    GATE_OPCODE_LIST(DECLARE_GATE_OPCODE)
#undef DECLARE_GATE_OPCODE
};

struct Properties {
    MachineType returnValue;
    std::optional<std::pair<std::vector<OpCode>, bool>> statesIn;
    size_t dependsIn;
    std::optional<std::pair<std::vector<MachineType>, bool>> valuesIn;
    std::optional<OpCode> root;
};

class GateMetaData : public ChunkObject {
public:
    enum Type {
        IMMUTABLE = 0,
        MUTABLE_WITH_SIZE,
        IMMUTABLE_ONE_VALUE,
        MUTABLE_ONE_VALUE,
        JSBYTECODE
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

    Type GetType() const
    {
        return TypeBits::Get(bitField_);
    }

    void AssertType([[maybe_unused]] Type type) const
    {
        ASSERT(GetType() == type);
    }

    bool IsOneValueType() const
    {
        return GetType() == IMMUTABLE_ONE_VALUE || GetType() == MUTABLE_ONE_VALUE;
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
    const Properties& GetProperties() const;

    MachineType GetInMachineType(size_t idx) const;
    OpCode GetInStateCode(size_t idx) const;

protected:
    void SetType(Type type)
    {
        TypeBits::Set<uint8_t>(type, &bitField_);
    }

    bool HasRoot() const
    {
        return HasRootBit::Get(bitField_);
    }

    void DecreaseIn(size_t idx)
    {
        ASSERT(GetType() == Type::MUTABLE_WITH_SIZE);
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

    using TypeBits = panda::BitField<Type, 0, 4>; // 4: type width
    using HasRootBit = TypeBits::NextFlag;

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

class OneValueMetaData : public GateMetaData {
public:
    explicit OneValueMetaData(OpCode opcode, bool hasRoot, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value)
        : GateMetaData(opcode, hasRoot, statesIn, dependsIn, valuesIn), value_(value)
    {
        SetType(GateMetaData::IMMUTABLE_ONE_VALUE);
    }

    uint64_t GetValue() const
    {
        return value_;
    }

private:
    uint64_t value_ { 0 };
};

class JSBytecodeMegaData : public GateMetaData {
public:
    explicit JSBytecodeMegaData(size_t valuesIn, EcmaOpcode opcode, uint32_t bcIndex) :
        GateMetaData(OpCode::JS_BYTECODE, false, 1, 1, valuesIn),
        opcode_(opcode), bcIndex_(bcIndex)
    {
        SetType(GateMetaData::JSBYTECODE);
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

struct GateMetaDataChache;
class GateMetaBuilder {
public:
#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
    const GateMetaData* NAME();
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                        \
    const GateMetaData* NAME(size_t value);
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                        \
    const GateMetaData* NAME(uint64_t value);
    GATE_META_DATA_LIST_WITH_ONE_VALUE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

    GateMetaBuilder(Chunk* chunk);
    const GateMetaData* JSBytecode(size_t valuesIn, EcmaOpcode opcode, uint32_t bcIndex)
    {
        return new (chunk_) JSBytecodeMegaData(valuesIn, opcode, bcIndex);
    }

    const GateMetaData* Nop()
    {
        return &cachedNop_;
    }

    GateMetaData* NewGateMetaData(const GateMetaData* other)
    {
        auto meta = new (chunk_) GateMetaData(other->opcode_, other->HasRoot(),
            other->statesIn_, other->dependsIn_, other->valuesIn_);
        meta->SetType(GateMetaData::MUTABLE_WITH_SIZE);
        return meta;
    }

private:
    const GateMetaDataChache& cache_;
    const GateMetaData cachedNop_ { OpCode::NOP, false, 0, 0, 0 };
    Chunk* chunk_;
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GATE_META_DATA_H
