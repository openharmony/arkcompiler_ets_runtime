/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/jspandafile/constpool_value.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/platform/elf.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#define DEFVAlUE(varname, cirBuilder, type, val) \
        Variable varname(cirBuilder, type, cirBuilder->NextVariableId(), val)

class Environment;
class Label;
class Variable;

#define BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(V)                            \
    V(Int16Add, Add, MachineType::I16)                                    \
    V(Int32Add, Add, MachineType::I32)                                    \
    V(Int64Add, Add, MachineType::I64)                                    \
    V(DoubleAdd, Add, MachineType::F64)                                   \
    V(PtrAdd, Add, MachineType::ARCH)                                     \
    V(Int16Sub, Sub, MachineType::I16)                                    \
    V(Int32Sub, Sub, MachineType::I32)                                    \
    V(Int64Sub, Sub, MachineType::I64)                                    \
    V(DoubleSub, Sub, MachineType::F64)                                   \
    V(PtrSub, Sub, MachineType::ARCH)                                     \
    V(Int32Mul, Mul, MachineType::I32)                                    \
    V(Int64Mul, Mul, MachineType::I64)                                    \
    V(DoubleMul, Mul, MachineType::F64)                                   \
    V(PtrMul, Mul, MachineType::ARCH)                                     \
    V(Int32Div, Sdiv, MachineType::I32)                                   \
    V(Int64Div, Sdiv, MachineType::I64)                                   \
    V(DoubleDiv, Fdiv, MachineType::F64)                                  \
    V(Int32Mod, Smod, MachineType::I32)                                   \
    V(DoubleMod, Smod, MachineType::F64)                                  \
    V(BoolAnd, And, MachineType::I1)                                      \
    V(Int8And, And, MachineType::I8)                                      \
    V(Int32And, And, MachineType::I32)                                    \
    V(Int64And, And, MachineType::I64)                                    \
    V(BoolOr, Or, MachineType::I1)                                        \
    V(Int32Or, Or, MachineType::I32)                                      \
    V(Int64Or, Or, MachineType::I64)                                      \
    V(Int32Xor, Xor, MachineType::I32)                                    \
    V(Int64Xor, Xor, MachineType::I64)                                    \
    V(Int16LSL, Lsl, MachineType::I16)                                    \
    V(Int32LSL, Lsl, MachineType::I32)                                    \
    V(Int64LSL, Lsl, MachineType::I64)                                    \
    V(Int8LSR, Lsr, MachineType::I8)                                      \
    V(Int32LSR, Lsr, MachineType::I32)                                    \
    V(Int64LSR, Lsr, MachineType::I64)                                    \
    V(Int32ASR, Asr, MachineType::I32)

#define UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(V)                  \
    V(BoolNot, Rev, MachineType::I1)                                   \
    V(Int32Not, Rev, MachineType::I32)                                 \
    V(Int64Not, Rev, MachineType::I64)                                 \
    V(CastDoubleToInt64, Bitcast, MachineType::I64)                    \
    V(CastInt64ToFloat64, Bitcast, MachineType::F64)                   \
    V(SExtInt32ToInt64, Sext, MachineType::I64)                        \
    V(SExtInt1ToInt64, Sext, MachineType::I64)                         \
    V(SExtInt1ToInt32, Sext, MachineType::I32)                         \
    V(ZExtInt8ToInt16, Zext, MachineType::I16)                         \
    V(ZExtInt32ToInt64, Zext, MachineType::I64)                        \
    V(ZExtInt1ToInt64, Zext, MachineType::I64)                         \
    V(ZExtInt1ToInt32, Zext, MachineType::I32)                         \
    V(ZExtInt8ToInt32, Zext, MachineType::I32)                         \
    V(ZExtInt8ToInt64, Zext, MachineType::I64)                         \
    V(ZExtInt8ToPtr, Zext, MachineType::ARCH)                          \
    V(ZExtInt16ToPtr, Zext, MachineType::ARCH)                         \
    V(ZExtInt32ToPtr, Zext, MachineType::ARCH)                         \
    V(SExtInt32ToPtr, Sext, MachineType::ARCH)                         \
    V(ZExtInt16ToInt32, Zext, MachineType::I32)                        \
    V(ZExtInt16ToInt64, Zext, MachineType::I64)                        \
    V(TruncInt16ToInt8, Trunc, MachineType::I8)                        \
    V(TruncInt64ToInt32, Trunc, MachineType::I32)                      \
    V(TruncPtrToInt32, Trunc, MachineType::I32)                        \
    V(TruncInt64ToInt1, Trunc, MachineType::I1)                        \
    V(TruncInt64ToInt16, Trunc, MachineType::I16)                      \
    V(TruncInt32ToInt1, Trunc, MachineType::I1)                        \
    V(TruncInt32ToInt16, Trunc, MachineType::I16)                      \
    V(TruncFloatToInt64, TruncFloatToInt64, MachineType::I64)          \
    V(ExtFloat32ToDouble, Fext, MachineType::F64)                      \
    V(TruncDoubleToFloat32, Ftrunc, MachineType::F32)                  \
    V(ChangeTaggedPointerToInt64, TaggedToInt64, MachineType::I64)     \
    V(ChangeInt32ToFloat64, SignedIntToFloat, MachineType::F64)        \
    V(ChangeInt32ToFloat32, SignedIntToFloat, MachineType::F32)        \
    V(ChangeUInt32ToFloat64, UnsignedIntToFloat, MachineType::F64)     \
    V(ChangeFloat64ToInt32, FloatToSignedInt, MachineType::I32)        \
    V(SExtInt16ToInt64, Sext, MachineType::I64)        \
    V(SExtInt16ToInt32, Sext, MachineType::I32)        \
    V(SExtInt8ToInt64, Sext, MachineType::I64)        \

#define BINARY_CMP_METHOD_LIST_WITHOUT_BITWIDTH(V)                                              \
    V(DoubleLessThan, Fcmp, static_cast<BitField>(FCmpCondition::OLT))                  \
    V(DoubleLessThanOrEqual, Fcmp, static_cast<BitField>(FCmpCondition::OLE))           \
    V(DoubleGreaterThan, Fcmp, static_cast<BitField>(FCmpCondition::OGT))               \
    V(DoubleGreaterThanOrEqual, Fcmp, static_cast<BitField>(FCmpCondition::OGE))        \
    V(Int32LessThan, Icmp, static_cast<BitField>(ICmpCondition::SLT))                   \
    V(Int32LessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SLE))            \
    V(Int32GreaterThan, Icmp, static_cast<BitField>(ICmpCondition::SGT))                \
    V(Int32GreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SGE))         \
    V(Int32UnsignedLessThan, Icmp, static_cast<BitField>(ICmpCondition::ULT))           \
    V(Int32UnsignedGreaterThan, Icmp, static_cast<BitField>(ICmpCondition::UGT))        \
    V(Int32UnsignedGreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::UGE)) \
    V(Int64LessThan, Icmp, static_cast<BitField>(ICmpCondition::SLT))                   \
    V(Int64LessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SLE))            \
    V(Int64GreaterThan, Icmp, static_cast<BitField>(ICmpCondition::SGT))                \
    V(Int64GreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SGE))         \
    V(Int64UnsignedLessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::ULE))

class CompilationConfig {
public:
    explicit CompilationConfig(const std::string &triple, bool enablePGOProfiler = false, bool isTraceBC = false,
                               bool profiling = false)
        : triple_(GetTripleFromString(triple)), isTraceBc_(isTraceBC), enablePGOProfiler_(enablePGOProfiler),
          profiling_(profiling)
    {
    }
    ~CompilationConfig() = default;

    inline bool Is32Bit() const
    {
        return triple_ == Triple::TRIPLE_ARM32;
    }

    inline bool IsAArch64() const
    {
        return triple_ == Triple::TRIPLE_AARCH64;
    }

    inline bool IsAmd64() const
    {
        return triple_ == Triple::TRIPLE_AMD64;
    }

    inline bool Is64Bit() const
    {
        return IsAArch64() || IsAmd64();
    }

    Triple GetTriple() const
    {
        return triple_;
    }

    bool IsTraceBC() const
    {
        return isTraceBc_;
    }

    bool IsEnablePGOProfiler() const
    {
        return enablePGOProfiler_;
    }

    bool IsProfiling() const
    {
        return profiling_;
    }

private:
    inline Triple GetTripleFromString(const std::string &triple)
    {
        if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_AMD64;
        }

        if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_AARCH64;
        }

        if (triple.compare("arm-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_ARM32;
        }
        UNREACHABLE();
    }
    Triple triple_;
    bool isTraceBc_;
    bool enablePGOProfiler_;
    bool profiling_;
};

class CircuitBuilder {
public:
    explicit CircuitBuilder(Circuit *circuit) : circuit_(circuit), acc_(circuit) {}
    explicit CircuitBuilder(Circuit *circuit, CompilationConfig *cmpCfg)
        : circuit_(circuit), acc_(circuit), cmpCfg_(cmpCfg)
    {
    }
    ~CircuitBuilder() = default;
    NO_MOVE_SEMANTIC(CircuitBuilder);
    NO_COPY_SEMANTIC(CircuitBuilder);
    // low level interface
    template<TypedUnOp Op>
    inline GateRef Int32OverflowCheck(GateRef gate);
    GateRef ArrayCheck(GateRef gate);
    GateRef StableArrayCheck(GateRef gate);
    GateRef TypedArrayCheck(GateType type, GateRef gate);
    GateRef IndexCheck(GateType type, GateRef gate, GateRef index);
    GateRef ObjectTypeCheck(GateType type, GateRef gate, GateRef hclassOffset);
    GateRef PrimitiveTypeCheck(GateType type, GateRef gate);
    GateRef CallTargetCheck(GateRef function, GateRef id, GateRef param);
    GateRef DeoptCheck(GateRef condition, GateRef frameState, DeoptType type = DeoptType::NOTCHECK);
    GateRef TypedBinaryOperator(MachineType type, TypedBinOp binOp, GateType typeLeft, GateType typeRight,
                                std::vector<GateRef> inList, GateType gateType);
    GateRef TypedCallOperator(MachineType type, const std::initializer_list<GateRef>& args);
    inline GateRef TypedCallBuiltin(GateRef x, BuiltinsStubCSigns::ID id);
    GateRef TypeConvert(MachineType type, GateType typeFrom, GateType typeTo, const std::vector<GateRef>& inList);
    GateRef TypedUnaryOperator(MachineType type, TypedUnOp unaryOp, GateType typeVal,
                               const std::vector<GateRef>& inList, GateType gateType);
    GateRef TypedNewAllocateThis(GateRef ctor, GateRef hclassIndex, GateRef frameState);
    GateRef TypedSuperAllocateThis(GateRef superCtor, GateRef newTarget, GateRef frameState);
    GateRef GetSuperConstructor(GateRef ctor);
    GateRef Arguments(size_t index);
    GateRef Merge(const std::vector<GateRef> &inList);
    GateRef Selector(OpCode opcode, MachineType machineType, GateRef control, const std::vector<GateRef> &values,
        int valueCounts, VariableType type = VariableType::VOID());
    GateRef Selector(OpCode opcode, GateRef control, const std::vector<GateRef> &values,
        int valueCounts, VariableType type = VariableType::VOID());
    GateRef Int8(int8_t val);
    GateRef Int16(int16_t val);
    GateRef Int32(int32_t value);
    GateRef Int64(int64_t value);
    GateRef IntPtr(int64_t val);
    GateRef Boolean(bool value);
    GateRef Double(double value);
    GateRef UndefineConstant();
    GateRef HoleConstant();
    GateRef NullConstant();
    GateRef ExceptionConstant();
    GateRef RelocatableData(uint64_t val);
    GateRef Alloca(size_t size);
    GateRef Branch(GateRef state, GateRef condition);
    GateRef SwitchBranch(GateRef state, GateRef index, int caseCounts);
    GateRef Return(GateRef state, GateRef depend, GateRef value);
    GateRef ReturnVoid(GateRef state, GateRef depend);
    GateRef Goto(GateRef state);
    GateRef LoopBegin(GateRef state);
    GateRef LoopEnd(GateRef state);
    GateRef IfTrue(GateRef ifBranch);
    GateRef IfFalse(GateRef ifBranch);
    GateRef SwitchCase(GateRef switchBranch, int64_t value);
    GateRef DefaultCase(GateRef switchBranch);
    GateRef DependRelay(GateRef state, GateRef depend);
    GateRef DependAnd(std::initializer_list<GateRef> args);
    GateRef BinaryArithmetic(const GateMetaData* meta, MachineType machineType,
        GateRef left, GateRef right);
    GateRef BinaryCmp(const GateMetaData* meta, GateRef left, GateRef right);
    static MachineType GetMachineTypeFromVariableType(VariableType type);
    GateRef GetCallBuiltinId(GateRef method);
    Circuit *GetCircuit() const
    {
        return circuit_;
    }
    // constant
    inline GateRef True();
    inline GateRef False();
    inline GateRef Undefined();

    // call operation
    GateRef CallBCHandler(GateRef glue, GateRef target, const std::vector<GateRef> &args);
    GateRef CallBCDebugger(GateRef glue, GateRef target, const std::vector<GateRef> &args);
    GateRef CallBuiltin(GateRef glue, GateRef target, const std::vector<GateRef> &args);
    GateRef CallBuiltinWithArgv(GateRef glue, GateRef target, const std::vector<GateRef> &args);
    GateRef CallRuntimeVarargs(GateRef glue, int index, GateRef argc, GateRef argv);
    GateRef CallRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args);
    GateRef CallNGCRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args);
    GateRef CallStub(GateRef glue, int index, const std::vector<GateRef> &args);
    GateRef CallBuiltinRuntime(GateRef glue, GateRef depend, const std::vector<GateRef> &args, bool isNew = false);
    GateRef Call(const CallSignature* cs, GateRef glue, GateRef target, GateRef depend,
                 const std::vector<GateRef> &args);

    // memory
    inline GateRef Load(VariableType type, GateRef base, GateRef offset);
    void Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value);

#define ARITHMETIC_BINARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                 \
    inline GateRef NAME(GateRef x, GateRef y)                                             \
    {                                                                                     \
        return BinaryArithmetic(circuit_->OPCODEID(), MACHINETYPEID, x, y);               \
    }

    BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(ARITHMETIC_BINARY_OP_WITH_BITWIDTH)
#undef ARITHMETIC_BINARY_OP_WITH_BITWIDTH

#define ARITHMETIC_UNARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                            \
    inline GateRef NAME(GateRef x)                                                                  \
    {                                                                                               \
        return circuit_->NewGate(circuit_->OPCODEID(), MACHINETYPEID, { x }, GateType::NJSValue()); \
    }

    UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(ARITHMETIC_UNARY_OP_WITH_BITWIDTH)
#undef ARITHMETIC_UNARY_OP_WITH_BITWIDTH

#define CMP_BINARY_OP_WITHOUT_BITWIDTH(NAME, OPCODEID, CONDITION)                         \
    inline GateRef NAME(GateRef x, GateRef y)                                             \
    {                                                                                     \
        return BinaryCmp(circuit_->OPCODEID(static_cast<uint64_t>(CONDITION)), x, y);                            \
    }

    BINARY_CMP_METHOD_LIST_WITHOUT_BITWIDTH(CMP_BINARY_OP_WITHOUT_BITWIDTH)
#undef CMP_BINARY_OP_WITHOUT_BITWIDTH

    inline GateRef Equal(GateRef x, GateRef y);
    inline GateRef NotEqual(GateRef x, GateRef y);

    // js world
    // cast operation
    inline GateRef GetInt64OfTInt(GateRef x);
    inline GateRef GetInt32OfTInt(GateRef x);
    inline GateRef TaggedCastToIntPtr(GateRef x);
    inline GateRef GetDoubleOfTDouble(GateRef x);
    inline GateRef GetDoubleOfTNumber(GateRef x);
    inline GateRef Int32ToTaggedPtr(GateRef x);
    inline GateRef Int64ToTaggedPtr(GateRef x);
    inline GateRef Int32ToTaggedInt(GateRef x);
    // bit operation
    inline GateRef IsSpecial(GateRef x, JSTaggedType type);
    inline GateRef TaggedIsInt(GateRef x);
    inline GateRef TaggedIsDouble(GateRef x);
    inline GateRef TaggedIsObject(GateRef x);
    inline GateRef TaggedIsNumber(GateRef x);
    inline GateRef TaggedIsNumeric(GateRef x);
    inline GateRef TaggedIsNotHole(GateRef x);
    inline GateRef TaggedIsHole(GateRef x);
    inline GateRef TaggedIsUndefined(GateRef x);
    inline GateRef TaggedIsException(GateRef x);
    inline GateRef TaggedIsSpecial(GateRef x);
    inline GateRef TaggedIsHeapObject(GateRef x);
    inline GateRef TaggedIsAsyncGeneratorObject(GateRef x);
    inline GateRef TaggedIsGeneratorObject(GateRef x);
    inline GateRef TaggedIsPropertyBox(GateRef x);
    inline GateRef TaggedIsWeak(GateRef x);
    inline GateRef TaggedIsPrototypeHandler(GateRef x);
    inline GateRef TaggedIsTransitionHandler(GateRef x);
    inline GateRef TaggedIsStoreTSHandler(GateRef x);
    inline GateRef TaggedIsTransWithProtoHandler(GateRef x);
    inline GateRef TaggedIsUndefinedOrNull(GateRef x);
    inline GateRef TaggedIsTrue(GateRef x);
    inline GateRef TaggedIsFalse(GateRef x);
    inline GateRef TaggedIsNull(GateRef x);
    inline GateRef TaggedIsBoolean(GateRef x);
    inline GateRef IsAOTLiteralInfo(GateRef x);
    inline GateRef TaggedGetInt(GateRef x);
    inline GateRef ToTaggedInt(GateRef x);
    inline GateRef ToTaggedIntPtr(GateRef x);
    inline GateRef DoubleToTaggedDoublePtr(GateRef x);
    inline GateRef BooleanToTaggedBooleanPtr(GateRef x);
    inline GateRef Float32ToTaggedDoublePtr(GateRef x);
    inline GateRef TaggedDoublePtrToFloat32(GateRef x);
    inline GateRef TaggedIntPtrToFloat32(GateRef x);
    inline GateRef DoubleToTaggedDouble(GateRef x);
    inline GateRef DoubleToTagged(GateRef x);
    inline GateRef DoubleIsNAN(GateRef x);
    inline GateRef TaggedTrue();
    inline GateRef TaggedFalse();
    // Pointer/Arithmetic/Logic Operations
    inline GateRef IntPtrDiv(GateRef x, GateRef y);
    inline GateRef IntPtrOr(GateRef x, GateRef y);
    inline GateRef IntPtrLSL(GateRef x, GateRef y);
    inline GateRef IntPtrLSR(GateRef x, GateRef y);
    inline GateRef Int64NotEqual(GateRef x, GateRef y);
    inline GateRef Int32NotEqual(GateRef x, GateRef y);
    inline GateRef Int64Equal(GateRef x, GateRef y);
    inline GateRef DoubleEqual(GateRef x, GateRef y);
    inline GateRef DoubleNotEqual(GateRef x, GateRef y);
    inline GateRef Int8Equal(GateRef x, GateRef y);
    inline GateRef Int32Equal(GateRef x, GateRef y);
    inline GateRef IntPtrGreaterThan(GateRef x, GateRef y);
    template<OpCode Op, MachineType Type>
    inline GateRef BinaryOp(GateRef x, GateRef y);
    inline GateRef GetValueFromTaggedArray(GateRef array, GateRef index);
    inline void SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val);
    GateRef TaggedIsString(GateRef obj);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    inline GateRef GetGlobalConstantString(ConstantIndex index);

    GateRef IsJSHClass(GateRef obj);
    GateRef HasPendingException(GateRef glue);

    // middle ir: operations with any type
    template<TypedBinOp Op>
    inline GateRef TypedBinaryOp(GateRef x, GateRef y, GateType xType, GateType yType, GateType gateType);
    template<TypedUnOp Op>
    inline GateRef TypedUnaryOp(GateRef x, GateType xType, GateType gateType);

    // middle ir: Number operations
    template<TypedBinOp Op>
    inline GateRef NumberBinaryOp(GateRef x, GateRef y);
    inline GateRef PrimitiveToNumber(GateRef x, VariableType type);

    // middle ir: object operations
    GateRef ToLength(GateRef receiver);
    template<TypedLoadOp Op>
    GateRef LoadElement(GateRef receiver, GateRef index);
    template<TypedStoreOp Op>
    GateRef StoreElement(GateRef receiver, GateRef index, GateRef value);
    GateRef LoadProperty(GateRef receiver, GateRef offset);
    GateRef StoreProperty(GateRef receiver, GateRef offset, GateRef value);
    GateRef LoadArrayLength(GateRef array);
    GateRef HeapAlloc(GateRef initialHClass, GateType type, RegionSpaceFlag flag);
    GateRef Construct(std::vector<GateRef> args);

    // Object Operations
    inline GateRef LoadHClass(GateRef object);
    inline GateRef IsDictionaryMode(GateRef object);
    inline void StoreHClass(GateRef glue, GateRef object, GateRef hClass);
    inline GateRef IsJsType(GateRef object, JSType type);
    inline GateRef GetObjectType(GateRef hClass);
    inline GateRef IsDictionaryModeByHClass(GateRef hClass);
    inline GateRef DoubleIsINF(GateRef x);
    inline GateRef IsDictionaryElement(GateRef hClass);
    inline GateRef IsClassConstructor(GateRef object);
    inline GateRef IsClassPrototype(GateRef object);
    inline GateRef IsExtensible(GateRef object);
    inline GateRef TaggedObjectIsEcmaObject(GateRef obj);
    inline GateRef IsJSObject(GateRef obj);
    inline GateRef TaggedObjectBothAreString(GateRef x, GateRef y);
    inline GateRef IsCallable(GateRef obj);
    inline GateRef IsCallableFromBitField(GateRef bitfield);
    inline GateRef LogicAnd(GateRef x, GateRef y);
    inline GateRef LogicOr(GateRef x, GateRef y);
    inline GateRef BothAreString(GateRef x, GateRef y);
    inline GateRef GetObjectSizeFromHClass(GateRef hClass);
    GateRef GetGlobalObject(GateRef glue);
    GateRef GetMethodFromFunction(GateRef function);
    GateRef GetModuleFromFunction(GateRef function);
    GateRef GetHomeObjectFromFunction(GateRef function);
    GateRef FunctionIsResolved(GateRef function);
    GateRef GetLengthFromString(GateRef value);
    GateRef GetHashcodeFromString(GateRef glue, GateRef value);
    GateRef IsUtf16String(GateRef string);
    GateRef TaggedIsBigInt(GateRef obj);
    void SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value);
    GateRef GetFunctionLexicalEnv(GateRef function);
    void SetModuleToFunction(GateRef glue, GateRef function, GateRef value);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
        GateRef value, GateRef attrOffset, VariableType type);
    void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value);
    GateRef GetConstPool(GateRef jsFunc);
    GateRef GetObjectFromConstPool(GateRef glue, GateRef jsFunc, GateRef index, ConstPoolType type);
    GateRef GetObjectFromConstPool(GateRef glue, GateRef constPool, GateRef module, GateRef index,
                                   ConstPoolType type);
    void SetEnvironment(Environment *env)
    {
        env_ = env;
    }
    Environment *GetCurrentEnvironment() const
    {
        return env_;
    }
    void SetCompilationConfig(CompilationConfig *cmpCfg)
    {
        cmpCfg_ = cmpCfg;
    }
    CompilationConfig *GetCompilationConfig() const
    {
        return cmpCfg_;
    }
    // label related
    void NewEnvironment(GateRef hir);
    void DeleteCurrentEnvironment();
    inline int NextVariableId();
    inline void HandleException(GateRef result, Label *success, Label *exception, Label *exit);
    inline void HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal);
    inline void SubCfgEntry(Label *entry);
    inline void SubCfgExit();
    inline GateRef Return(GateRef value);
    inline GateRef Return();
    inline void Bind(Label *label);
    inline void Bind(Label *label, bool justSlowPath);
    void Jump(Label *label);
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    inline Label *GetCurrentLabel() const;
    inline GateRef GetState() const;
    inline GateRef GetDepend() const;
    inline void SetDepend(GateRef depend);
    inline void SetState(GateRef state);

    GateRef GetGlobalEnvValue(VariableType type, GateRef env, size_t index);
    GateRef GetGlobalConstantValue(VariableType type, GateRef glue, ConstantIndex index);
    GateRef IsBase(GateRef ctor);

private:
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    Environment *env_ {nullptr};
    CompilationConfig *cmpCfg_ {nullptr};
};

class Label {
public:
    explicit Label() = default;
    explicit Label(Environment *env);
    explicit Label(CircuitBuilder *cirBuilder);
    ~Label() = default;
    Label(Label const &label) = default;
    Label &operator=(Label const &label) = default;
    Label(Label &&label) = default;
    Label &operator=(Label &&label) = default;
    inline void Seal();
    inline void WriteVariable(Variable *var, GateRef value)
    {
        impl_->WriteVariable(var, value);
    }
    inline GateRef ReadVariable(Variable *var)
    {
        return impl_->ReadVariable(var);
    }
    inline void Bind();
    inline void MergeAllControl();
    inline void MergeAllDepend();
    inline void AppendPredecessor(const Label *predecessor);
    inline std::vector<Label> GetPredecessors() const;
    inline void SetControl(GateRef control);
    inline void SetPreControl(GateRef control);
    inline void MergeControl(GateRef control);
    inline GateRef GetControl() const;
    inline GateRef GetDepend() const;
    inline void SetDepend(GateRef depend);

private:
    class LabelImpl {
    public:
        LabelImpl(Environment *env, GateRef control)
            : env_(env), control_(control), predeControl_(-1), isSealed_(false)
        {
        }
        ~LabelImpl() = default;
        NO_MOVE_SEMANTIC(LabelImpl);
        NO_COPY_SEMANTIC(LabelImpl);
        void Seal();
        void WriteVariable(Variable *var, GateRef value);
        GateRef ReadVariable(Variable *var);
        void Bind();
        void MergeAllControl();
        void MergeAllDepend();
        void AppendPredecessor(LabelImpl *predecessor);
        std::vector<LabelImpl *> GetPredecessors() const
        {
            return predecessors_;
        }
        void SetControl(GateRef control)
        {
            control_ = control;
        }
        void SetPreControl(GateRef control)
        {
            predeControl_ = control;
        }
        void MergeControl(GateRef control)
        {
            if (predeControl_ == -1) {
                predeControl_ = control;
                control_ = predeControl_;
            } else {
                otherPredeControls_.push_back(control);
            }
        }
        GateRef GetControl() const
        {
            return control_;
        }
        void SetDepend(GateRef depend)
        {
            depend_ = depend;
        }
        GateRef GetDepend() const
        {
            return depend_;
        }

    private:
        bool IsNeedSeal() const;
        bool IsSealed() const
        {
            return isSealed_;
        }
        bool IsLoopHead() const;
        bool IsControlCase() const;
        GateRef ReadVariableRecursive(Variable *var);
        Environment *env_;
        GateRef control_;
        GateRef predeControl_ {-1};
        GateRef dependRelay_ {-1};
        GateRef depend_ {-1};
        GateRef loopDepend_ {-1};
        std::vector<GateRef> otherPredeControls_;
        bool isSealed_ {false};
        std::map<Variable *, GateRef> valueMap_;
        std::vector<GateRef> phi;
        std::vector<LabelImpl *> predecessors_;
        std::map<Variable *, GateRef> incompletePhis_;
    };

    explicit Label(LabelImpl *impl) : impl_(impl) {}
    friend class Environment;
    LabelImpl *GetRawLabel() const
    {
        return impl_;
    }
    LabelImpl *impl_ {nullptr};
};

class Environment {
public:
    using LabelImpl = Label::LabelImpl;
    Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder);
    Environment(GateRef stateEntry, GateRef dependEntry, std::vector<GateRef>& inlist,
                Circuit *circuit, CircuitBuilder *builder);
    Environment(size_t arguments, CircuitBuilder *builder);
    ~Environment();
    Label *GetCurrentLabel() const
    {
        return currentLabel_;
    }
    void SetCurrentLabel(Label *label)
    {
        currentLabel_ = label;
    }
    CircuitBuilder *GetBuilder() const
    {
        return circuitBuilder_;
    }
    Circuit *GetCircuit() const
    {
        return circuit_;
    }
    int NextVariableId()
    {
        return nextVariableId_++;
    }
    void SetCompilationConfig(const CompilationConfig *cfg)
    {
        ccfg_ = cfg;
    }
    const CompilationConfig *GetCompilationConfig() const
    {
        return ccfg_;
    }
    inline bool Is32Bit() const
    {
        return ccfg_->Is32Bit();
    }
    inline bool IsAArch64() const
    {
        return ccfg_->IsAArch64();
    }
    inline bool IsAmd64() const
    {
        return ccfg_->IsAmd64();
    }
    inline bool IsArch64Bit() const
    {
        return ccfg_->IsAmd64() ||  ccfg_->IsAArch64();
    }
    inline bool IsAsmInterp() const
    {
        return circuit_->GetFrameType() == FrameType::ASM_INTERPRETER_FRAME;
    }
    inline bool IsArch32Bit() const
    {
        return ccfg_->Is32Bit();
    }
    inline GateRef GetArgument(size_t index) const
    {
        return arguments_.at(index);
    }
    inline bool IsEnablePGOProfiler() const
    {
        return ccfg_->IsEnablePGOProfiler();
    }

    inline Label GetLabelFromSelector(GateRef sel);
    inline void AddSelectorToLabel(GateRef sel, Label label);
    inline LabelImpl *NewLabel(Environment *env, GateRef control = -1);
    inline void SubCfgEntry(Label *entry);
    inline void SubCfgExit();
    inline GateRef GetInput(size_t index) const;

private:
    Label *currentLabel_ {nullptr};
    Circuit *circuit_ {nullptr};
    CircuitBuilder *circuitBuilder_ {nullptr};
    std::unordered_map<GateRef, LabelImpl *> phiToLabels_;
    std::vector<GateRef> inputList_;
    Label entry_;
    std::vector<LabelImpl *> rawLabels_;
    std::stack<Label *> stack_;
    int nextVariableId_ {0};
    std::vector<GateRef> arguments_;
    const CompilationConfig *ccfg_ {nullptr};
};

class Variable {
public:
    Variable(Environment *env, VariableType type, uint32_t id, GateRef value) : id_(id), type_(type), env_(env)
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    Variable(CircuitBuilder *cirbuilder, VariableType type, uint32_t id, GateRef value)
        : id_(id), type_(type), env_(cirbuilder->GetCurrentEnvironment())
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    ~Variable() = default;
    NO_MOVE_SEMANTIC(Variable);
    NO_COPY_SEMANTIC(Variable);
    void Bind(GateRef value)
    {
        currentValue_ = value;
    }
    GateRef Value() const
    {
        return currentValue_;
    }
    VariableType Type() const
    {
        return type_;
    }
    bool IsBound() const
    {
        return currentValue_ != 0;
    }
    Variable &operator=(const GateRef value)
    {
        env_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
        return *this;
    }
    GateRef operator*()
    {
        return env_->GetCurrentLabel()->ReadVariable(this);
    }
    GateRef ReadVariable()
    {
        return env_->GetCurrentLabel()->ReadVariable(this);
    }
    void WriteVariable(GateRef value)
    {
        env_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
    }
    GateRef AddPhiOperand(GateRef val);
    GateRef AddOperandToSelector(GateRef val, size_t idx, GateRef in);
    GateRef TryRemoveTrivialPhi(GateRef phi);
    uint32_t GetId() const
    {
        return id_;
    }

private:
    Circuit* GetCircuit() const
    {
        return env_->GetCircuit();
    }

    uint32_t id_;
    VariableType type_;
    GateRef currentValue_ {0};
    Environment *env_;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
