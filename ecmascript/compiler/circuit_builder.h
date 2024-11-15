/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/lcr_gate_meta_data.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/constpool_value.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#define DEFVALUE(varname, cirBuilder, type, val) \
        Variable varname(cirBuilder, type, cirBuilder->NextVariableId(), val)

class BuiltinsTypedArrayStubBuilder;
class BuiltinsStringStubBuilder;
class CompilationConfig;
class Environment;
class Label;
class NTypeBytecodeLowering;
class SlowPathLowering;
class StubBuilder;
class TypedBytecodeLowering;
class PGOHCRLowering;
class NTypeHCRLowering;
class TSHCRLowering;
class Variable;
class NativeInlineLowering;
class TypedHCRLowering;
class TypedNativeInlineLowering;
class StringBuilderOptimizer;
class PostSchedule;
class TSHCROptPass;

#define BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(V)                    \
    V(Int16Add, Add, MachineType::I16)                                    \
    V(Int32Add, Add, MachineType::I32)                                    \
    V(Int64Add, Add, MachineType::I64)                                    \
    V(DoubleAdd, Add, MachineType::F64)                                   \
    V(PtrAdd, Add, MachineType::ARCH)                                     \
    V(DoubleExp, Exp, MachineType::F64)                                   \
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
    V(BitAnd, And, MachineType::I1)                                       \
    V(Int8And, And, MachineType::I8)                                      \
    V(Int8Xor, Xor, MachineType::I8)                                      \
    V(Int16And, And, MachineType::I16)                                    \
    V(Int32And, And, MachineType::I32)                                    \
    V(Int64And, And, MachineType::I64)                                    \
    V(BitOr, Or, MachineType::I1)                                         \
    V(Int16Or, Or, MachineType::I16)                                      \
    V(Int32Or, Or, MachineType::I32)                                      \
    V(Int64Or, Or, MachineType::I64)                                      \
    V(Int32Xor, Xor, MachineType::I32)                                    \
    V(Int64Xor, Xor, MachineType::I64)                                    \
    V(Int16LSL, Lsl, MachineType::I16)                                    \
    V(Int32LSL, Lsl, MachineType::I32)                                    \
    V(Int64LSL, Lsl, MachineType::I64)                                    \
    V(Int8LSR, Lsr, MachineType::I8)                                      \
    V(Int16LSR, Lsr, MachineType::I16)                                      \
    V(Int32LSR, Lsr, MachineType::I32)                                    \
    V(Int64LSR, Lsr, MachineType::I64)                                    \
    V(Int32ASR, Asr, MachineType::I32)                                    \
    V(Int64ASR, Asr, MachineType::I64)                                    \
    V(Int32Min, Min, MachineType::I32)                                    \
    V(DoubleMin, Min, MachineType::F64)                                   \
    V(Int32Max, Max, MachineType::I32)                                    \
    V(DoubleMax, Max, MachineType::F64)

#define UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(V)                  \
    V(BoolNot, Rev, MachineType::I1)                                   \
    V(Int32Not, Rev, MachineType::I32)                                 \
    V(Int64Not, Rev, MachineType::I64)                                 \
    V(CastDoubleToInt64, Bitcast, MachineType::I64)                    \
    V(CastInt64ToFloat64, Bitcast, MachineType::F64)                   \
    V(CastInt32ToFloat32, Bitcast, MachineType::F32)                   \
    V(CastFloat32ToInt32, Bitcast, MachineType::I32)                   \
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
    V(DoubleTrunc, DoubleTrunc, MachineType::F64)                      \
    V(TruncInt16ToInt8, Trunc, MachineType::I8)                        \
    V(TruncInt64ToInt32, Trunc, MachineType::I32)                      \
    V(TruncPtrToInt32, Trunc, MachineType::I32)                        \
    V(TruncInt64ToInt1, Trunc, MachineType::I1)                        \
    V(TruncInt64ToInt16, Trunc, MachineType::I16)                      \
    V(TruncInt32ToInt1, Trunc, MachineType::I1)                        \
    V(TruncInt32ToInt8, Trunc, MachineType::I8)                        \
    V(TruncInt32ToInt16, Trunc, MachineType::I16)                      \
    V(TruncFloatToInt64, TruncFloatToInt64, MachineType::I64)          \
    V(TruncFloatToInt32, TruncFloatToInt32, MachineType::I32)          \
    V(ExtFloat32ToDouble, Fext, MachineType::F64)                      \
    V(TruncDoubleToFloat32, Ftrunc, MachineType::F32)                  \
    V(ChangeInt32ToFloat64, SignedIntToFloat, MachineType::F64)        \
    V(ChangeInt32ToFloat32, SignedIntToFloat, MachineType::F32)        \
    V(ChangeUInt32ToFloat64, UnsignedIntToFloat, MachineType::F64)     \
    V(ChangeFloat64ToInt32, FloatToSignedInt, MachineType::I32)        \
    V(ChangeTaggedPointerToInt64, TaggedToInt64, MachineType::I64)     \
    V(SExtInt16ToInt64, Sext, MachineType::I64)                        \
    V(SExtInt16ToInt32, Sext, MachineType::I32)                        \
    V(SExtInt8ToInt32, Sext, MachineType::I32)                         \
    V(SExtInt8ToInt64, Sext, MachineType::I64)                         \
    V(Abs, Abs, MachineType::I32)                                      \
    V(FAbs, Abs, MachineType::F64)                                     \
    V(CountLeadingZeroes32, Clz32, MachineType::I32)                   \
    V(DoubleCeil, Ceil, MachineType::F64)                              \
    V(DoubleFloor, Floor, MachineType::F64)

#define BINARY_CMP_METHOD_LIST_WITHOUT_BITWIDTH(V)                                      \
    V(DoubleLessThan, Fcmp, static_cast<BitField>(FCmpCondition::OLT))                  \
    V(DoubleLessThanOrEqual, Fcmp, static_cast<BitField>(FCmpCondition::OLE))           \
    V(DoubleGreaterThan, Fcmp, static_cast<BitField>(FCmpCondition::OGT))               \
    V(DoubleGreaterThanOrEqual, Fcmp, static_cast<BitField>(FCmpCondition::OGE))        \
    V(DoubleEqual, Fcmp, static_cast<BitField>(FCmpCondition::OEQ))                     \
    V(DoubleNotEqual, Fcmp, static_cast<BitField>(FCmpCondition::ONE))                  \
    V(Int8GreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SGE))          \
    V(Int32LessThan, Icmp, static_cast<BitField>(ICmpCondition::SLT))                   \
    V(Int32LessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SLE))            \
    V(Int32GreaterThan, Icmp, static_cast<BitField>(ICmpCondition::SGT))                \
    V(Int32GreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SGE))         \
    V(Int32UnsignedLessThan, Icmp, static_cast<BitField>(ICmpCondition::ULT))           \
    V(Int32UnsignedLessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::ULE))    \
    V(Int32UnsignedGreaterThan, Icmp, static_cast<BitField>(ICmpCondition::UGT))        \
    V(Int32UnsignedGreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::UGE)) \
    V(Int64LessThan, Icmp, static_cast<BitField>(ICmpCondition::SLT))                   \
    V(Int64LessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SLE))            \
    V(Int64GreaterThan, Icmp, static_cast<BitField>(ICmpCondition::SGT))                \
    V(Int64GreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::SGE))         \
    V(Int64UnsignedLessThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::ULE))    \
    V(Int64UnsignedGreaterThan, Icmp, static_cast<BitField>(ICmpCondition::UGT))        \
    V(Int64UnsignedGreaterThanOrEqual, Icmp, static_cast<BitField>(ICmpCondition::UGE))

class CircuitBuilder {
public:
    explicit CircuitBuilder(Circuit *circuit) : circuit_(circuit), acc_(circuit) {}
    CircuitBuilder(Circuit *circuit, CompilationConfig *cmpCfg)
        : circuit_(circuit), acc_(circuit), cmpCfg_(cmpCfg)
    {
    }
    ~CircuitBuilder() = default;
    NO_MOVE_SEMANTIC(CircuitBuilder);
    NO_COPY_SEMANTIC(CircuitBuilder);

    enum JudgeMethodType {
        HAS_AOT,
        HAS_AOT_FASTCALL,
        HAS_AOT_NOTFASTCALL,
    };

    // **************************** Share IR *****************************
    GateRef Arguments(size_t index);
    GateRef DefaultCase(GateRef switchBranch);
    GateRef DependRelay(GateRef state, GateRef depend);
    void DeoptCheck(GateRef condition, GateRef frameState, DeoptType type);
    GateRef GetElementsArray(GateRef object);
    GateRef GetLengthOfTaggedArray(GateRef array);
    GateRef GetLengthOfJSTypedArray(GateRef array);
    GateRef GetDataOfTaggedArray(GateRef array);
    GateRef GetLengthOfJSArray(GateRef array);
    GateRef IsTypedArray(GateRef array);
    GateRef GetSuperConstructor(GateRef ctor);
    GateRef Merge(const std::vector<GateRef> &inList);
    GateRef Selector(OpCode opcode, MachineType machineType, GateRef control, const std::vector<GateRef> &values,
        int valueCounts, VariableType type = VariableType::VOID());
    GateRef Selector(OpCode opcode, GateRef control, const std::vector<GateRef> &values,
        int valueCounts, VariableType type = VariableType::VOID());
    GateRef Nop();
    GateRef Return(GateRef state, GateRef depend, GateRef value);
    GateRef ReturnVoid(GateRef state, GateRef depend);
    GateRef Goto(GateRef state);
    GateRef LoopBegin(GateRef state);
    GateRef LoopEnd(GateRef state);
    GateRef LoopExit(GateRef state);
    GateRef LoopExitDepend(GateRef state, GateRef depend);
    GateRef LoopExitValue(GateRef state, GateRef value);
    GateRef IfTrue(GateRef ifBranch);
    GateRef IfFalse(GateRef ifBranch);
    GateRef IsJsCOWArray(GateRef obj);
    GateRef IsCOWArray(GateRef objectType);
    GateRef IsTaggedArray(GateRef object);
    GateRef CheckJSType(GateRef object, JSType jsType);
    GateRef IsMutantTaggedArray(GateRef objectType);
    GateRef SwitchCase(GateRef switchBranch, int64_t value);
    GateRef Int8(int8_t val);
    GateRef Int16(int16_t val);
    GateRef Int32(int32_t value);
    GateRef Int64(int64_t value);
    GateRef IntPtr(int64_t val);
    GateRef StringPtr(std::string_view str);
    GateRef StringPtr(const std::string &str);
    GateRef Boolean(bool value);
    GateRef Double(double value);
    GateRef UndefineConstant();
    GateRef HoleConstant();
    GateRef SpecialHoleConstant();
    GateRef NullPtrConstant();
    GateRef NullConstant();
    GateRef TaggedValueConstant(JSTaggedValue taggedValue);
    GateRef ExceptionConstant();
    void ClearConstantCache(GateRef gate);
    GateRef NanValue();
    GateRef RelocatableData(uint64_t val);

#define BRANCH_CIR(condition, trueLabel, falseLabel)                                \
    {                                                                               \
        std::ostringstream os;                                                      \
        os << __func__ << ": " << #trueLabel << "- " << #falseLabel;                \
        builder_.Branch(condition, trueLabel, falseLabel, 1, 1, os.str().c_str());  \
    }

#define BRANCH_CIR2(condition, trueLabel, falseLabel)                      \
    {                                                                      \
        std::ostringstream os;                                             \
        os << __func__ << ": " << #trueLabel << "- " << #falseLabel;       \
        Branch(condition, trueLabel, falseLabel, 1, 1, os.str().c_str());  \
    }

    GateRef Branch(GateRef state, GateRef condition, uint32_t leftWeight = 1, uint32_t rightWeight = 1,
                   const char* comment = nullptr);  // 1: default branch weight
    GateRef SwitchBranch(GateRef state, GateRef index, int caseCounts);
    void AppendFrameArgs(std::vector<GateRef> &args, GateRef hirGate);
    inline GateRef True();
    inline GateRef False();
    inline GateRef Undefined();
    inline GateRef Hole();

    GateRef ElementsKindIsIntOrHoleInt(GateRef kind);
    GateRef ElementsKindIsNumOrHoleNum(GateRef kind);
    GateRef ElementsKindIsHeapKind(GateRef kind);
    GateRef ElementsKindHasHole(GateRef kind);

    GateRef LoadBuiltinObject(size_t offset);

    // Get
    GateRef GetConstPoolFromFunction(GateRef jsFunc);
    GateRef GetUnsharedConstpoolFromGlue(GateRef glue, GateRef constpool);
    GateRef GetUnsharedConstpoolIndex(GateRef constpool);
    GateRef GetUnsharedConstpool(GateRef arrayAddr, GateRef index);
    GateRef GetCodeAddr(GateRef jsFunc);
    GateRef GetBaselineCodeAddr(GateRef baselineCode);
    GateRef GetObjectByIndexFromConstPool(GateRef glue, GateRef hirGate, GateRef frameState, GateRef index,
                                   ConstPoolType type);
    GateRef GetObjectFromConstPool(GateRef glue, GateRef hirGate, GateRef shardConstPool, GateRef unsharedConstPool,
                                   GateRef module, GateRef index, ConstPoolType type);
    GateRef GetFunctionLexicalEnv(GateRef function);
    GateRef GetGlobalEnv();
    GateRef GetGlobalEnvObj(GateRef env, size_t index);
    GateRef GetGlobalEnvObjHClass(GateRef env, size_t index);
    GateRef GetGlobalConstantValue(ConstantIndex index);
    GateRef GetGlobalEnvValue(VariableType type, GateRef env, size_t index);
    GateRef GetGlobalObject(GateRef glue);
    GateRef GetMethodFromFunction(GateRef function);
    GateRef GetModuleFromFunction(GateRef function);
    GateRef GetSendableEnvFromModule(GateRef module);
    GateRef GetHomeObjectFromFunction(GateRef function);
    GateRef GetHClassGateFromIndex(GateRef gate, int32_t index);
    GateRef GetProfileTypeInfo(GateRef function);
    inline GateRef GetExpectedNumOfArgs(GateRef method);
    inline GateRef GetGlobalConstantOffset(ConstantIndex index); // shareir
    GateRef GetEmptyArray(GateRef glue);
    GateRef GetPrototypeFromHClass(GateRef hClass);
    GateRef GetEnumCacheFromHClass(GateRef hClass);
    GateRef GetProtoChangeMarkerFromHClass(GateRef hClass);
    GateRef GetLengthFromForInIterator(GateRef iter);
    GateRef GetIndexFromForInIterator(GateRef iter);
    GateRef GetKeysFromForInIterator(GateRef iter);
    GateRef GetObjectFromForInIterator(GateRef iter);
    GateRef GetCachedHclassFromForInIterator(GateRef iter);
    void SetLengthOfForInIterator(GateRef glue, GateRef iter, GateRef length);
    void SetIndexOfForInIterator(GateRef glue, GateRef iter, GateRef index);
    void SetKeysOfForInIterator(GateRef glue, GateRef iter, GateRef keys);
    void SetObjectOfForInIterator(GateRef glue, GateRef iter, GateRef object);
    void SetCachedHclassOfForInIterator(GateRef glue, GateRef iter, GateRef hclass);
    void IncreaseInteratorIndex(GateRef glue, GateRef iter, GateRef index);
    void SetNextIndexOfArrayIterator(GateRef glue, GateRef iter, GateRef nextIndex);
    void SetIteratedArrayOfArrayIterator(GateRef glue, GateRef iter, GateRef iteratedArray);
    void SetBitFieldOfArrayIterator(GateRef glue, GateRef iter, GateRef kind);
    GateRef GetHasChanged(GateRef object);
    GateRef GetAccessorHasChanged(GateRef object);
    GateRef HasDeleteProperty(GateRef hClass);
    GateRef IsOnHeap(GateRef hClass);
    GateRef IsEcmaObject(GateRef obj);

    // Set
    void SetLengthToFunction(GateRef glue, GateRef function, GateRef value);
    void SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value);
    void SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value);
    void SetModuleToFunction(GateRef glue, GateRef function, GateRef value);
    void SetSendableEnvToModule(GateRef glue, GateRef module, GateRef value);
    void SetRawProfileTypeInfoToFunction(GateRef glue, GateRef function, GateRef value);
    void UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                             GateRef profileTypeInfo, GateRef slotId);
    void UpdateProfileTypeInfoCellType(GateRef glue, GateRef profileTypeInfoCell);

    GateRef FunctionIsResolved(GateRef function);
    GateRef HasPendingException(GateRef glue); // shareir
    GateRef IsUtf8String(GateRef string);
    GateRef IsUtf16String(GateRef string);
    GateRef IsInternString(GateRef string);
    GateRef LoadObjectFromConstPool(GateRef constPool, GateRef index);
    GateRef IsAccessorInternal(GateRef accessor);

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
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel,
                uint32_t trueWeight = 1, uint32_t falseWeight = 1, const char* comment = nullptr);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    void LoopExit(const std::vector<Variable*> &vars, size_t diff = 1);
    inline Label *GetCurrentLabel() const;
    inline GateRef GetState() const;
    inline GateRef GetDepend() const;
    inline StateDepend GetStateDepend() const;

    Circuit *GetCircuit() const
    {
        return circuit_;
    }

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

    // **************************** High IR ******************************
    GateRef CreateArray(ElementsKind kind, uint32_t arraySize, GateRef elementsLength, RegionSpaceFlag flag);
    GateRef CreateArrayWithBuffer(ElementsKind kind, ArrayMetaDataAccessor::Mode mode, GateRef cpId,
                                  GateRef constPoolIndex, RegionSpaceFlag flag);
    GateRef CreateArguments(ElementsKind kind, CreateArgumentsAccessor::Mode mode, GateRef restIdx);
    GateRef Construct(GateRef hirGate, std::vector<GateRef> args);
    GateRef CallNew(GateRef hirGate, std::vector<GateRef> args, bool needPushArgv = false);
    GateRef CallInternal(GateRef hirGate, std::vector<GateRef> args, uint64_t pcOffset);
    GateRef TypedCallNative(GateRef hirGate, GateRef thisObj, GateRef funcId);
    GateRef IsBase(GateRef ctor);
    GateRef IsDerived(GateRef ctor);
    GateRef ToLength(GateRef receiver);
    GateRef StoreModuleVar(GateRef jsFunc, GateRef index, GateRef value);
    GateRef LdLocalModuleVar(GateRef jsFunc, GateRef index);
    GateRef BuiltinConstructor(BuiltinsStubCSigns::ID id, GateRef gate);

    inline GateRef GetMethodId(GateRef func);
    inline GateRef GetBuiltinsId(GateRef func);
    inline GateRef IsAOTLiteralInfo(GateRef x);
    GateRef GetKeyFromLexivalEnv(GateRef lexicalEnv, GateRef levelIndex, GateRef slotIndex);
    GateRef GetParentEnv(GateRef object);
    GateRef GetSendableParentEnv(GateRef object);
    GateRef GetPropertiesFromLexicalEnv(GateRef object, GateRef index);
    GateRef GetPropertiesFromSendableEnv(GateRef object, GateRef index);
    GateRef Float32ArrayConstructor(GateRef hirGate, std::vector<GateRef> args);

    // call operation
    GateRef CallPrivateGetter(GateRef hirGate, GateRef receiver, GateRef accessor, const char* comment = nullptr);
    GateRef CallPrivateSetter(
        GateRef hirGate, GateRef receiver, GateRef accessor, GateRef value, const char* comment = nullptr);
    GateRef CallGetter(GateRef hirGate, GateRef receiver, GateRef holder,
        GateRef propertyLookupResult, const char* comment = nullptr);
    GateRef CallSetter(GateRef hirGate, GateRef receiver, GateRef holder, GateRef propertyLookupResult,
                       GateRef value, const char* comment = nullptr);
    GateRef CallBCHandler(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                          const char* comment = nullptr);
    GateRef CallBCDebugger(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                           const char* comment = nullptr);
    GateRef CallBuiltin(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                        const char* comment = nullptr);
    GateRef CallBuiltinWithArgv(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                const char* comment = nullptr);
    GateRef CallRuntimeVarargs(GateRef glue, int index, GateRef argc, GateRef argv, const char* comment = nullptr);
    GateRef CallRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args, GateRef hirGate,
                        const char* comment = nullptr);
    GateRef CallNGCRuntime(GateRef glue, GateRef gate, int index, const std::vector<GateRef> &args, bool useLabel);

    GateRef CallNGCRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args,
                           GateRef hirGate, const char* comment = nullptr);
    GateRef FastCallOptimized(GateRef glue, GateRef code, GateRef depend, const std::vector<GateRef> &args,
                              GateRef hirGate);
    GateRef CallOptimized(GateRef glue, GateRef code, GateRef depend, const std::vector<GateRef> &args,
                          GateRef hirGate);
    GateRef CallStub(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                     const char* comment = nullptr);
    GateRef CallBuiltinRuntime(GateRef glue, GateRef depend, const std::vector<GateRef> &args, bool isNew = false);
    GateRef CallBuiltinRuntimeWithNewTarget(GateRef glue, GateRef depend, const std::vector<GateRef> &args);
    GateRef Call(const CallSignature* cs, GateRef glue, GateRef target, GateRef depend,
                 const std::vector<GateRef> &args, GateRef hirGate, const char* comment = nullptr);
    GateRef NoLabelCallRuntime(GateRef glue, GateRef depend, size_t index, std::vector<GateRef> &args, GateRef hirGate);

    void StartCallTimer(GateRef glue, GateRef gate, const std::vector<GateRef> &args, bool useLabel);
    void EndCallTimer(GateRef glue, GateRef gate, const std::vector<GateRef> &args, bool useLabel);
    GateRef GetCallBuiltinId(GateRef method);

    // FastCall
    inline GateRef CanFastCall(GateRef jsFunc);

    // Js world
    inline GateRef GetObjectType(GateRef hClass);
    inline GateRef HasConstructor(GateRef object);
    inline GateRef IsSpecial(GateRef x, JSTaggedType type);
    inline GateRef IsJSFunction(GateRef obj);
    inline GateRef IsDictionaryMode(GateRef object);
    inline GateRef IsJsType(GateRef object, JSType type);
    inline GateRef IsStableElements(GateRef hClass);
    inline GateRef IsStableArguments(GateRef hClass);
    inline GateRef IsStableArray(GateRef hClass);
    inline GateRef IsDictionaryElement(GateRef hClass);
    inline GateRef IsJSArrayPrototypeModified(GateRef hClass);
    inline GateRef IsClassConstructor(GateRef object);
    inline GateRef IsClassConstructorWithBitField(GateRef bitfield);
    inline GateRef IsConstructor(GateRef object);
    inline GateRef IsClassPrototype(GateRef object);
    inline GateRef IsClassPrototypeWithBitField(GateRef object);
    inline GateRef IsExtensible(GateRef object);
    inline GateRef IsJSObject(GateRef obj);
    inline GateRef IsCallable(GateRef obj);
    inline GateRef IsCallableFromBitField(GateRef bitfield);
    inline GateRef IsPrototypeHClass(GateRef hclass);
    inline GateRef IsJsProxy(GateRef obj);
    GateRef IsJSHClass(GateRef obj);
    inline void StoreHClass(GateRef glue, GateRef object, GateRef hClass);
    inline void StoreHClassWithoutBarrier(GateRef glue, GateRef object, GateRef hClass);
    GateRef IsStabelArray(GateRef glue, GateRef obj);
    inline void StorePrototype(GateRef glue, GateRef hclass, GateRef prototype);
    void SetExtensibleToBitfield(GateRef glue, GateRef obj, bool isExtensible);

    // WeakRef
    inline GateRef CreateWeakRef(GateRef x);
    inline GateRef LoadObjectFromWeakRef(GateRef x);

    // hClass
    inline GateRef GetElementsKindByHClass(GateRef hClass);
    inline GateRef GetObjectSizeFromHClass(GateRef hClass);
    inline GateRef HasConstructorByHClass(GateRef hClass);
    inline GateRef IsDictionaryModeByHClass(GateRef hClass);
    inline GateRef LoadHClass(GateRef object);
    inline GateRef LoadHClassByConstOffset(GateRef object);
    inline GateRef LoadPrototype(GateRef hclass);
    inline GateRef LoadPrototypeHClass(GateRef object);
    inline GateRef LoadPrototypeOfPrototypeHClass(GateRef object);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
                                 GateRef value, GateRef attrOffset, VariableType type);

    // Others
    GateRef OrdinaryHasInstance(GateRef obj, GateRef target);
    GateRef MigrateArrayWithKind(GateRef receiver, GateRef oldElementsKind, GateRef newElementsKind);

    // **************************** Middle IR ****************************
    GateRef EcmaObjectCheck(GateRef gate);
    GateRef HeapObjectCheck(GateRef gate, GateRef frameState);
    GateRef MathHClassConsistencyCheck(GateRef receiver);
    GateRef HeapObjectIsEcmaObjectCheck(GateRef gate, GateRef frameState);
    GateRef ProtoChangeMarkerCheck(GateRef gate, GateRef frameState = Gate::InvalidGateRef);
    GateRef StableArrayCheck(GateRef gate, ElementsKind kind, ArrayMetaDataAccessor::Mode mode);
    GateRef ElementsKindCheck(GateRef receiver, ElementsKind kind, ArrayMetaDataAccessor::Mode mode);
    GateRef COWArrayCheck(GateRef gate);
    GateRef EcmaStringCheck(GateRef gate);
    GateRef InternStringCheck(GateRef gate);
    GateRef EcmaMapCheck(GateRef gate);
    GateRef FlattenTreeStringCheck(GateRef gate);
    GateRef HClassStableArrayCheck(GateRef gate, GateRef frameState, ArrayMetaDataAccessor accessor);
    GateRef ArrayGuardianCheck(GateRef frameState);
    GateRef TypedArrayCheck(GateRef gate, ParamType paramType, TypedArrayMetaDataAccessor::Mode mode,
                            OnHeapMode onHeap = OnHeapMode::NONE);
    GateRef LoadTypedArrayLength(GateRef gate, ParamType paramType, OnHeapMode onHeap = OnHeapMode::NONE);
    GateRef RangeGuard(GateRef gate, uint32_t left, uint32_t right);
    GateRef BuiltinPrototypeHClassCheck(GateRef gate, BuiltinTypeId type, ElementsKind kind,
                                        bool isPrototypeOfPrototype);
    GateRef OrdinaryHasInstanceCheck(GateRef target, GateRef jsFunc, std::vector<GateRef> &expectedHCIndexes);
    GateRef IndexCheck(GateRef gate, GateRef index);
    GateRef ObjectTypeCheck(bool isHeapObject, GateRef gate, GateRef hclassIndex,
                            GateRef frameState = Gate::InvalidGateRef);
    GateRef TryPrimitiveTypeCheck(GateType type, GateRef gate);
    GateRef CallTargetCheck(GateRef gate, GateRef function, GateRef id, const char* comment = nullptr);
    GateRef CallTargetCheck(GateRef gate, GateRef function, GateRef id, std::vector<GateRef> params,
                            const char* comment = nullptr);
    template<TypedCallTargetCheckOp Op>
    GateRef JSCallTargetTypeCheck(GateRef func, GateRef methodIndex, GateRef gate);
    template<TypedCallTargetCheckOp Op>
    inline GateRef JSNoGCCallThisTargetTypeCheck(GateRef func, GateRef methodId, GateRef gate);
    GateRef TypeOfCheck(GateRef gate, ParamType paramType);
    GateRef TypedTypeOf(ParamType paramType);
    GateRef TypedCallOperator(GateRef hirGate, MachineType type, const std::vector<GateRef>& inList, bool isSideEffect);
    inline GateRef TypedCallBuiltin(GateRef hirGate, const std::vector<GateRef> &args,
                                    BuiltinsStubCSigns::ID id, bool isSideEffect);
    GateRef TypeConvert(MachineType type, ParamType typeFrom, GateType typeTo, const std::vector<GateRef>& inList);
    GateRef Int32CheckRightIsZero(GateRef right);
    GateRef RemainderIsNegativeZero(GateRef left, GateRef right);
    GateRef Float64CheckRightIsZero(GateRef right);
    GateRef ValueCheckNegOverflow(GateRef value);
    GateRef OverflowCheck(GateRef value);
    GateRef LexVarIsHoleCheck(GateRef value);
    GateRef IsUndefinedOrHoleCheck(GateRef value);
    GateRef IsNotUndefinedOrHoleCheck(GateRef value);
    GateRef IsDataViewCheck(GateRef obj);
    GateRef IsCallableCheck(GateRef func);
    GateRef Int32UnsignedUpperBoundCheck(GateRef value, GateRef upperBound);
    GateRef Int32DivWithCheck(GateRef left, GateRef right);
    GateType GetGateTypeOfValueType(ValueType type);
    GateRef InsertStableArrayCheck(GateRef array, ArrayMetaDataAccessor accessor);
    GateRef InsertLoadArrayLength(GateRef array, GateRef length, bool isTypedArray);
    GateRef InsertTypedArrayCheck(GateRef array, TypedArrayMetaDataAccessor accessor);
    GateRef ArrayConstructorCheck(GateRef gate);
    GateRef Float32ArrayConstructorCheck(GateRef gate);
    GateRef ObjectConstructorCheck(GateRef gate);
    GateRef BooleanConstructorCheck(GateRef gate);
    GateRef InsertTypedBinaryop(GateRef left, GateRef right, TypedBinOp op);
    GateRef InsertRangeCheckPredicate(GateRef left, TypedBinOp cond, GateRef right);
    GateRef TypedConditionJump(MachineType type, TypedJumpOp jumpOp, uint32_t weight, ParamType paramType,
                               const std::vector<GateRef>& inList);
    GateRef TypedNewAllocateThis(GateRef ctor, GateRef hclass, GateRef size, GateRef frameState);
    GateRef TypedSuperAllocateThis(GateRef superCtor, GateRef newTarget, GateRef frameState);
    template<TypedBinOp Op>
    inline GateRef TypedBinaryOp(GateRef x, GateRef y, ParamType paramType);
    template<TypedUnOp Op>
    inline GateRef TypedUnaryOp(GateRef x, ParamType paramType);
    template<TypedJumpOp Op>
    inline GateRef TypedConditionJump(GateRef x, ParamType paramType, uint32_t weight);
    GateRef Convert(GateRef gate, ValueType src, ValueType dst);
    GateRef ConvertBoolToTaggedBoolean(GateRef gate);
    GateRef ConvertTaggedBooleanToBool(GateRef gate);
    GateRef ConvertInt32ToBool(GateRef gate);
    GateRef ConvertFloat64ToBool(GateRef gate);
    GateRef ConvertInt32ToTaggedInt(GateRef gate);
    GateRef ConvertFloat64ToTaggedDouble(GateRef gate);
    GateRef ConvertFloat64ToInt32(GateRef gate);
    GateRef ConvertInt32ToFloat64(GateRef gate);
    GateRef ConvertBoolToInt32(GateRef gate, ConvertSupport support);
    GateRef ConvertBoolToFloat64(GateRef gate, ConvertSupport support);
    GateRef ConvertUInt32ToBool(GateRef gate);
    GateRef ConvertUInt32ToTaggedNumber(GateRef gate);
    GateRef ConvertUInt32ToFloat64(GateRef gate);
    GateRef ConvertCharToEcmaString(GateRef gate);
    GateRef ConvertCharToInt32(GateRef gate);
    GateRef ConvertCharToDouble(GateRef gate);
    GateRef CheckAndConvert(
        GateRef gate, ValueType src, ValueType dst, ConvertSupport support = ConvertSupport::ENABLE);
    GateRef ConvertHoleAsUndefined(GateRef receiver);
    GateRef ConvertSpecialHoleIntToTagged(GateRef gate);
    GateRef ConvertSpecialHoleDoubleToTagged(GateRef gate);
    GateRef CheckSpecialHoleAndConvertToTaggedInt(GateRef receiver);
    GateRef CheckSpecialHoleAndConvertToTaggedDouble(GateRef receiver);
    GateRef CheckUInt32AndConvertToInt32(GateRef gate);
    GateRef CheckTaggedIntAndConvertToInt32(GateRef gate);
    GateRef CheckTaggedDoubleAndConvertToInt32(GateRef gate);
    GateRef CheckTaggedNumberAndConvertToInt32(GateRef gate);
    GateRef CheckTaggedIntAndConvertToFloat64(GateRef gate);
    GateRef CheckTaggedDoubleAndConvertToFloat64(GateRef gate);
    GateRef CheckTaggedNumberAndConvertToFloat64(GateRef gate);
    GateRef CheckTaggedNumberAndConvertToBool(GateRef gate);
    GateRef CheckTaggedBooleanAndConvertToBool(GateRef gate);
    GateRef CheckNullAndConvertToInt32(GateRef gate);
    GateRef CheckTaggedBooleanAndConvertToInt32(GateRef gate);
    GateRef CheckHoleIntAndConvertToTaggedInt(GateRef gate);
    GateRef CheckHoleDoubleAndConvertToTaggedDouble(GateRef gate);
    GateRef CheckNullAndConvertToFloat64(GateRef gate);
    GateRef CheckTaggedBooleanAndConvertToFloat64(GateRef gate);
    GateRef CheckUndefinedAndConvertToFloat64(GateRef gate);
    GateRef CheckUndefinedAndConvertToBool(GateRef gate);
    GateRef CheckNullAndConvertToBool(GateRef gate);
    GateRef CheckUndefinedAndConvertToInt32(GateRef gate);
    GateRef CheckHoleIntAndConvertToInt32(GateRef gate);
    GateRef CheckHoleDoubleAndConvertToInt32(GateRef gate);
    GateRef CheckHoleIntAndConvertToFloat64(GateRef gate);
    GateRef CheckHoleDoubleAndConvertToFloat64(GateRef gate);
    GateRef CheckFloat64ConvertToInt32Legally(GateRef gate);
    GateRef StartAllocate();
    GateRef FinishAllocate(GateRef value);

    inline GateRef PrimitiveToNumber(GateRef x, ParamType paramType);
    inline GateRef GetValueFromTaggedArray(GateRef array, GateRef index);
    inline GateRef GetValueFromTaggedArray(VariableType valType, GateRef array, GateRef index);
    inline GateRef GetValueFromJSArrayWithElementsKind(VariableType type, GateRef array, GateRef index);
    template<TypedLoadOp Op>
    GateRef LoadElement(GateRef receiver, GateRef index, OnHeapMode onHeap = OnHeapMode::NONE);
    GateRef LoadProperty(GateRef receiver, GateRef propertyLookupResult, bool isFunction);
    GateRef LoadArrayLength(GateRef gate, ElementsKind kind, ArrayMetaDataAccessor::Mode mode);
    inline GateRef LoadFromTaggedArray(GateRef array, size_t index);
    GateRef LoadStringLength(GateRef string);
    GateRef LoadMapSize(GateRef string);
    GateRef LoadConstOffset(VariableType type, GateRef receiver, size_t offset,
                            MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef LoadHClassFromConstpool(GateRef constpool, size_t index);
    GateRef TypedCall(GateRef hirGate, std::vector<GateRef> args, bool isNoGC);
    GateRef TypedFastCall(GateRef hirGate, std::vector<GateRef> args, bool isNoGC);
    inline void SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val);
    GateRef StoreConstOffset(VariableType type, GateRef receiver, size_t offset, GateRef value,
                             MemoryAttribute mAttr = MemoryAttribute::Default());
    inline GateRef StoreToTaggedArray(GateRef array, size_t index, GateRef value);
    GateRef StringEqual(GateRef x, GateRef y);
    GateRef StringAdd(GateRef x, GateRef y, uint32_t stringStatus = 0);
    template<TypedStoreOp Op>
    GateRef StoreElement(GateRef receiver, GateRef index, GateRef value, OnHeapMode onHeap = OnHeapMode::NONE);
    GateRef StoreMemory(MemoryType Op, VariableType type, GateRef receiver, GateRef index, GateRef value);
    GateRef StoreProperty(GateRef receiver, GateRef propertyLookupResult, GateRef value, uint32_t receiverHClassIndex);
    inline GateRef JudgeAotAndFastCall(GateRef jsFunc, JudgeMethodType type);
    GateRef ComputeTaggedArraySize(GateRef length);
    GateRef HeapAlloc(GateRef glue, GateRef size, GateType type, RegionSpaceFlag flag);
    GateRef TaggedIsHeapObjectOp(GateRef value);
    GateRef IsSpecificObjectType(GateRef obj, JSType type);
    GateRef IsMarkerCellValid(GateRef cell);
    GateRef IsMarkerCellValidOp(GateRef cell);
    GateRef MonoLoadPropertyOnProto(GateRef receiver, GateRef plrGate, GateRef jsFunc, size_t hclassIndex);
    GateRef MonoCallGetterOnProto(GateRef gate, GateRef receiver, GateRef plrGate, GateRef jsFunc, size_t hclassIndex);
    GateRef MonoStorePropertyLookUpProto(GateRef receiver, GateRef plrGate, GateRef jsFunc, size_t hclassIndex,
                                         GateRef value);
    GateRef MonoStoreProperty(GateRef receiver, GateRef plrGate, GateRef jsFunc, size_t hclassIndex,
                              GateRef value, GateRef keyIndex, GateRef isProto, GateRef frameState);
    GateRef TypedCreateObjWithBuffer(std::vector<GateRef> &valueIn);
    template<TypedLoadOp Op>
    GateRef ConvertJSArrayHoleAsUndefined(GateRef receiver);
    GateRef BuildBigIntAsIntN(const GateMetaData* op, std::vector<GateRef> &&args);
    GateRef NewJSPrimitiveRef(GateRef glue, size_t index, GateRef obj);
    GateRef ToObject(GateRef glue, GateRef obj);
    GateRef GetPrototype(GateRef glue, GateRef object);

    GateRef GetGlobalConstantValue(VariableType type, GateRef glue, ConstantIndex index);
    GateRef TransProtoWithoutLayout(GateRef glue, GateRef hClass, GateRef proto);
    GateRef OrdinaryNewJSObjectCreate(GateRef glue, GateRef proto);

    // bit operation
    inline GateRef TaggedIsInt(GateRef x);
    inline GateRef TaggedIsDouble(GateRef x);
    inline GateRef TaggedIsObject(GateRef x);
    inline GateRef TaggedIsNumber(GateRef x);
    inline GateRef TaggedIsNumeric(GateRef x);
    inline GateRef TaggedIsNotHole(GateRef x);
    inline GateRef TaggedIsHole(GateRef x);
    inline GateRef TaggedIsNullPtr(GateRef x);
    inline GateRef TaggedIsUndefined(GateRef x);
    inline GateRef TaggedIsException(GateRef x);
    inline GateRef TaggedIsSpecial(GateRef x);
    inline GateRef TaggedIsHeapObject(GateRef x);
    inline GateRef TaggedIsAsyncGeneratorObject(GateRef x);
    inline GateRef TaggedIsJSGlobalObject(GateRef x);
    inline GateRef TaggedIsGeneratorObject(GateRef x);
    inline GateRef TaggedIsJSArray(GateRef x);
    inline GateRef TaggedIsPropertyBox(GateRef x);
    inline GateRef TaggedIsWeak(GateRef x);
    inline GateRef TaggedIsPrototypeHandler(GateRef x);
    inline GateRef TaggedIsTransitionHandler(GateRef x);
    inline GateRef TaggedIsStoreAOTHandler(GateRef x);
    inline GateRef TaggedIsTransWithProtoHandler(GateRef x);
    inline GateRef TaggedIsUndefinedOrNull(GateRef x);
    inline GateRef TaggedIsUndefinedOrNullOrHole(GateRef x);
    inline GateRef TaggedIsNotUndefinedAndNullAndHole(GateRef x);
    inline GateRef TaggedIsNotUndefinedAndNull(GateRef x);
    inline GateRef TaggedIsUndefinedOrHole(GateRef x);
    inline GateRef TaggedIsTrue(GateRef x);
    inline GateRef TaggedIsFalse(GateRef x);
    inline GateRef TaggedIsNull(GateRef x);
    inline GateRef TaggedIsNotNull(GateRef x);
    inline GateRef TaggedIsBoolean(GateRef x);
    inline GateRef TaggedIsBigInt(GateRef obj);
    inline GateRef TaggedIsString(GateRef obj);
    inline GateRef TaggedIsStringIterator(GateRef obj);
    inline GateRef TaggedIsSharedObj(GateRef obj);
    inline GateRef TaggedIsStringOrSymbol(GateRef obj);
    inline GateRef TaggedIsSymbol(GateRef obj);
    inline GateRef TaggedIsProtoChangeMarker(GateRef obj);
    inline GateRef TaggedObjectIsJSMap(GateRef obj);
    inline GateRef TaggedObjectIsJSSet(GateRef obj);
    inline GateRef TaggedObjectIsJSDate(GateRef obj);
    inline GateRef TaggedObjectIsTypedArray(GateRef obj);
    inline GateRef TaggedObjectIsJSArray(GateRef obj);
    inline GateRef TaggedIsBoundFunction(GateRef obj);
    inline GateRef TaggedGetInt(GateRef x);
    inline GateRef TaggedObjectIsString(GateRef obj);
    inline GateRef TaggedObjectIsShared(GateRef obj);
    inline GateRef TaggedObjectIsEcmaObject(GateRef obj);
    inline GateRef TaggedObjectIsByteArray(GateRef obj);
    inline GateRef TaggedObjectIsMap(GateRef obj);
    inline GateRef TaggedObjectIsDataView(GateRef obj);
    inline GateRef IsSpecialHole(GateRef x);
    inline GateRef IsNotSpecialHole(GateRef x);
    inline GateRef TaggedTrue();
    inline GateRef TaggedFalse();

    // String
    inline GateRef BothAreString(GateRef x, GateRef y);
    inline GateRef IsTreeString(GateRef obj);
    inline GateRef IsSlicedString(GateRef obj);
    inline GateRef IsSpecialSlicedString(GateRef obj);
    inline GateRef IsLineString(GateRef obj);
    inline GateRef IsConstantString(GateRef obj);
    inline GateRef TreeStringIsFlat(GateRef string);
    inline GateRef GetFirstFromTreeString(GateRef string);
    inline GateRef GetSecondFromTreeString(GateRef string);
    inline GateRef ComputeSizeUtf8(GateRef length);
    inline GateRef ComputeSizeUtf16(GateRef length);
    inline GateRef AlignUp(GateRef x, GateRef alignment);
    GateRef NumberToString(GateRef number);
    GateRef TaggedPointerToInt64(GateRef x);
    GateRef GetLengthFromString(GateRef value);
    GateRef Rotl(GateRef word, uint32_t shift);
    GateRef CalcHashcodeForInt(GateRef value);
    GateRef GetHashcodeFromString(GateRef glue, GateRef value, GateRef hir = Circuit::NullGate());
    GateRef TryGetHashcodeFromString(GateRef string);
    GateRef IsIntegerString(GateRef string);
    GateRef IsLiteralString(GateRef string);
    GateRef CanBeConcat(GateRef leftString, GateRef rightString, GateRef isValidOpt);
    GateRef CanBackStore(GateRef rightString, GateRef isValidOpt);
    GateRef GetRawHashFromString(GateRef value);
    GateRef GetStringDataFromLineOrConstantString(GateRef str);
    void CopyUtf8AsUtf16(GateRef glue, GateRef dst, GateRef src, GateRef sourceLength);
    void CopyChars(GateRef glue, GateRef dst, GateRef source, GateRef sourceLength,
        GateRef charSize, VariableType type);
    void SetRawHashcode(GateRef glue, GateRef str, GateRef rawHashcode, GateRef isInteger);
    GateRef StringFromSingleCharCode(GateRef gate);
    GateRef StringCharCodeAt(GateRef thisValue, GateRef posTag);
    GateRef StringSubstring(GateRef thisValue, GateRef startTag, GateRef endTag);
    GateRef StringSubStr(GateRef thisValue, GateRef intStart, GateRef lengthTag);
    GateRef StringSlice(GateRef thisValue, GateRef startTag, GateRef endTag);
    GateRef NumberIsNaN(GateRef gate);
    GateRef NumberParseFloat(GateRef gate, GateRef frameState);
    GateRef NumberParseInt(GateRef gate, GateRef radix);
    GateRef NumberIsFinite(GateRef gate);
    GateRef NumberIsInteger(GateRef gate);
    GateRef NumberIsSafeInteger(GateRef gate);
    GateRef ArrayBufferIsView(GateRef gate);
    GateRef DataViewGet(
        GateRef thisobj, GateRef index, GateRef dataViewCallID, GateRef isLittleEndian, GateRef frameState);
    GateRef DataViewSet(GateRef thisobj,
                        GateRef index,
                        GateRef value,
                        GateRef dataViewCallID,
                        GateRef isLittleEndian,
                        GateRef frameState);
    GateRef ArrayIncludesIndexOf(
        GateRef thisArray, GateRef fromIndex, GateRef targetElement, GateRef CallID, GateRef ArrayKind);
    GateRef ArrayIteratorBuiltin(GateRef thisArray, GateRef callID);
    GateRef ArrayForEach(GateRef thisValue, GateRef callBackFn, GateRef usingThis, uint32_t pcOffset);
    GateRef ArraySort(GateRef thisValue, GateRef callBackFn);
    GateRef ArrayFilter(
        GateRef thisValue, GateRef callBackFn, GateRef usingThis, GateRef frameState, uint32_t pcOffset);
    GateRef ArrayMap(GateRef thisValue, GateRef callBackFn, GateRef usingThis, GateRef frameState, uint32_t pcOffset);
    GateRef ArraySome(GateRef thisValue, GateRef callBackFn, GateRef usingThis, uint32_t pcOffset);
    GateRef ArrayFindOrFindIndex(
        GateRef thisValue, GateRef callBackFn, GateRef usingThis, GateRef callIDRef, uint32_t pcOffset);
    GateRef ArrayEvery(GateRef thisValue, GateRef callBackFn, GateRef usingThis, uint32_t pcOffset);
    GateRef ArrayPop(GateRef thisValue, GateRef frameState);
    GateRef ArraySlice(GateRef thisValue, GateRef startIndex, GateRef endIndex, GateRef frameState);
    GateRef ToNumber(GateRef gate, GateRef value, GateRef glue);
    GateRef StringToNumber(GateRef gate, GateRef value, GateRef radix, GateRef glue);
    GateRef IsASCIICharacter(GateRef gate);

    // for in
    GateRef GetEnumCacheKind(GateRef glue, GateRef enumCache);
    GateRef IsEnumCacheValid(GateRef receiver, GateRef cachedHclass, GateRef kind);
    GateRef NeedCheckProperty(GateRef receiver);

    // ElementsKind Array
    GateRef MigrateFromRawValueToHeapValues(GateRef object, GateRef needCOW, GateRef isIntKind);
    GateRef MigrateFromHeapValueToRawValue(GateRef object, GateRef needCOW, GateRef isIntKind);
    GateRef MigrateFromHoleIntToHoleNumber(GateRef object);
    GateRef MigrateFromHoleNumberToHoleInt(GateRef object);

    // **************************** Low IR *******************************
    inline GateRef Equal(GateRef x, GateRef y, const char* comment = nullptr);
    inline GateRef NotEqual(GateRef x, GateRef y, const char* comment = nullptr);
    inline GateRef IntPtrDiv(GateRef x, GateRef y);
    inline GateRef IntPtrOr(GateRef x, GateRef y);
    inline GateRef IntPtrLSL(GateRef x, GateRef y);
    inline GateRef IntPtrLSR(GateRef x, GateRef y);
    inline GateRef Int64NotEqual(GateRef x, GateRef y);
    inline GateRef Int32NotEqual(GateRef x, GateRef y);
    inline GateRef Int64Equal(GateRef x, GateRef y);
    inline GateRef Int8Equal(GateRef x, GateRef y);
    inline GateRef Int32Equal(GateRef x, GateRef y);
    inline GateRef IntPtrGreaterThan(GateRef x, GateRef y);
    inline GateRef IntPtrAnd(GateRef x, GateRef y);
    inline GateRef IntPtrNot(GateRef x);
    inline GateRef IntPtrEqual(GateRef x, GateRef y);
    GateRef DoubleTrunc(GateRef gate, GateRef value, const char* comment = nullptr);
    GateRef AddWithOverflow(GateRef left, GateRef right);
    GateRef SubWithOverflow(GateRef left, GateRef right);
    GateRef MulWithOverflow(GateRef left, GateRef right);
    GateRef ExtractValue(MachineType mt, GateRef pointer, GateRef index);
    GateRef Sqrt(GateRef param);
    MachineType GetMachineTypeOfValueType(ValueType type);
    GateRef Alloca(size_t size);
    GateRef ReadSp();
    GateRef BinaryArithmetic(const GateMetaData* meta, MachineType machineType, GateRef left,
                             GateRef right, GateType gateType = GateType::Empty(), const char* comment = nullptr);
    GateRef BinaryCmp(const GateMetaData* meta, GateRef left, GateRef right, const char* comment = nullptr);
    GateRef Load(VariableType type, GateRef base, GateRef offset, MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef Load(VariableType type, GateRef base, GateRef offset, GateRef depend,
                 MemoryAttribute mAttr = MemoryAttribute::Default());
    GateRef Load(VariableType type, GateRef addr, MemoryAttribute mAttr = MemoryAttribute::Default());
    void Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value,
               MemoryAttribute mAttr = MemoryAttribute::Default());
    void StoreWithoutBarrier(VariableType type, GateRef addr, GateRef value,
                             MemoryAttribute mAttr = MemoryAttribute::Default());

    // cast operation
    inline GateRef Int16ToBigEndianInt16(GateRef x);
    inline GateRef Int32ToBigEndianInt32(GateRef x);
    inline GateRef Int64ToBigEndianInt64(GateRef x);
    inline GateRef GetInt64OfTInt(GateRef x);
    inline GateRef GetInt32OfTInt(GateRef x);
    inline GateRef GetInt32OfTNumber(GateRef x);
    inline GateRef TaggedCastToIntPtr(GateRef x);
    inline GateRef GetDoubleOfTInt(GateRef x);
    inline GateRef GetDoubleOfTDouble(GateRef x);
    inline GateRef GetBooleanOfTBoolean(GateRef x);
    GateRef GetDoubleOfTNumber(GateRef x);
    GateRef DoubleToInt(GateRef x, Label *exit);
    GateRef DoubleToInt(GateRef glue, GateRef x, size_t typeBits);
    GateRef DoubleCheckINFInRangeInt32(GateRef x);
    GateRef DoubleInRangeInt32(GateRef x);
    inline GateRef Int32ToTaggedPtr(GateRef x);
    inline GateRef Int64ToTaggedPtr(GateRef x);
    inline GateRef Int32ToTaggedInt(GateRef x);
    inline GateRef ToTaggedInt(GateRef x);
    inline GateRef ToTaggedIntPtr(GateRef x);
    inline GateRef DoubleToTaggedDoublePtr(GateRef x);
    inline GateRef DoubleIsImpureNaN(GateRef x);
    inline GateRef BooleanToTaggedBooleanPtr(GateRef x);
    inline GateRef BooleanToInt32(GateRef x);
    inline GateRef BooleanToFloat64(GateRef x);
    inline GateRef Float32ToTaggedDoublePtr(GateRef x);
    inline GateRef TaggedDoublePtrToFloat32(GateRef x);
    inline GateRef TaggedIntPtrToFloat32(GateRef x);
    inline GateRef DoubleToTaggedDouble(GateRef x);
    inline GateRef DoubleToTagged(GateRef x);
    inline GateRef DoubleIsNAN(GateRef x);
    inline GateRef DoubleIsINF(GateRef x);
    inline GateRef DoubleIsNanOrInf(GateRef x);
    static MachineType GetMachineTypeFromVariableType(VariableType type);
    GateRef FastToBoolean(GateRef value);

    // Opcode with control and depend inputs from label and value inputs args
    GateRef BuildControlDependOp(const GateMetaData* op, std::vector<GateRef> args,
                                 std::vector<GateRef> frameStates = {});
    template<OpCode Op, MachineType Type>
    inline GateRef BinaryOp(GateRef x, GateRef y);
    template<OpCode Op, MachineType Type>
    inline GateRef BinaryOpWithOverflow(GateRef x, GateRef y);

    GateRef BuildTypedArrayIterator(GateRef gate, const GateMetaData* op);
    GateRef IsStableArrayLengthWriteable(GateRef array);

#define ARITHMETIC_BINARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                                        \
    inline GateRef NAME(GateRef x, GateRef y, GateType type = GateType::Empty(), const char* comment = nullptr)  \
    {                                                                                                            \
        return BinaryArithmetic(circuit_->OPCODEID(), MACHINETYPEID, x, y, type, comment);                       \
    }

    BINARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(ARITHMETIC_BINARY_OP_WITH_BITWIDTH)
#undef ARITHMETIC_BINARY_OP_WITH_BITWIDTH

#define ARITHMETIC_UNARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                                     \
    inline GateRef NAME(GateRef x, const char* comment = nullptr)                                            \
    {                                                                                                        \
        return circuit_->NewGate(circuit_->OPCODEID(), MACHINETYPEID, { x }, GateType::NJSValue(), comment); \
    }

    UNARY_ARITHMETIC_METHOD_LIST_WITH_BITWIDTH(ARITHMETIC_UNARY_OP_WITH_BITWIDTH)
#undef ARITHMETIC_UNARY_OP_WITH_BITWIDTH

#define CMP_BINARY_OP_WITHOUT_BITWIDTH(NAME, OPCODEID, CONDITION)                                \
    inline GateRef NAME(GateRef x, GateRef y, const char* comment = nullptr)                     \
    {                                                                                            \
        return BinaryCmp(circuit_->OPCODEID(static_cast<uint64_t>(CONDITION)), x, y, comment);   \
    }

    BINARY_CMP_METHOD_LIST_WITHOUT_BITWIDTH(CMP_BINARY_OP_WITHOUT_BITWIDTH)
#undef CMP_BINARY_OP_WITHOUT_BITWIDTH

private:

    std::vector<GateRef> ConcatParams(const std::vector<std::vector<GateRef>>& params);
    static constexpr uint32_t GATE_TWO_VALUESIN = 2;

    inline void SetDepend(GateRef depend);
    inline void SetState(GateRef state);

#define ARITHMETIC_UNARY_OP_WITH_BITWIDTH(NAME, OPCODEID, MACHINETYPEID)                                     \
    inline GateRef NAME(GateRef x, const char* comment = nullptr)                                            \
    {                                                                                                        \
        return circuit_->NewGate(circuit_->OPCODEID(), MACHINETYPEID, { x }, GateType::NJSValue(), comment); \
    }

#undef ARITHMETIC_UNARY_OP_WITH_BITWIDTH

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    Environment *env_ {nullptr};
    CompilationConfig *cmpCfg_ {nullptr};
    friend StubBuilder;
    friend BuiltinsStringStubBuilder;
    friend BuiltinsTypedArrayStubBuilder;
    friend TypedBytecodeLowering;
    friend NTypeBytecodeLowering;
    friend PGOHCRLowering;
    friend TSHCRLowering;
    friend NTypeHCRLowering;
    friend SlowPathLowering;
    friend NativeInlineLowering;
    friend TypedHCRLowering;
    friend TypedNativeInlineLowering;
    friend PostSchedule;
    friend TSHCROptPass;
};

class LogicAndBuilder {
public:
    inline LogicAndBuilder(Environment *env);
    inline ~LogicAndBuilder();
    NO_MOVE_SEMANTIC(LogicAndBuilder);
    NO_COPY_SEMANTIC(LogicAndBuilder);

    inline LogicAndBuilder &And(GateRef check);

    inline GateRef Done();
private:
    Environment *env_ {nullptr};
    CircuitBuilder *builder_ {nullptr};
    Label *subentry_ {nullptr};
    Variable *result_ {nullptr};
    Label *exit_ {nullptr};
    std::vector<Label*> labels_;
};

class LogicOrBuilder {
public:
    inline LogicOrBuilder(Environment *env);
    inline ~LogicOrBuilder();
    NO_MOVE_SEMANTIC(LogicOrBuilder);
    NO_COPY_SEMANTIC(LogicOrBuilder);

    inline LogicOrBuilder &Or(GateRef check);

    inline GateRef Done();
private:
    Environment *env_ {nullptr};
    CircuitBuilder *builder_ {nullptr};
    Label *subentry_ {nullptr};
    Variable *result_ {nullptr};
    Label *exit_ {nullptr};
    std::vector<Label*> labels_;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
