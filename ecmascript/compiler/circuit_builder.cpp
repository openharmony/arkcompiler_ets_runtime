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

#include "ecmascript/compiler/circuit_builder.h"

#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
GateRef CircuitBuilder::Merge(GateRef *inList, size_t controlCount)
{
    return circuit_->NewGate(OpCode(OpCode::MERGE), controlCount, controlCount, inList, GateType::Empty());
}

GateRef CircuitBuilder::Selector(OpCode opcode, MachineType machineType, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    return circuit_->NewGate(opcode, machineType, valueCounts, inList, type.GetGateType());
}

GateRef CircuitBuilder::Selector(OpCode opcode, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    return circuit_->NewGate(opcode, valueCounts, inList, type.GetGateType());
}

GateRef CircuitBuilder::UndefineConstant()
{
    auto type = GateType::TaggedValue();
    return circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition)
{
    return circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, { state, condition }, GateType::Empty());
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_BRANCH), caseCounts, { state, index }, GateType::Empty());
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN), 0, { state, depend, value, returnList }, GateType::Empty());
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN_VOID), 0, { state, depend, returnList }, GateType::Empty());
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0, { state }, GateType::Empty());
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(OpCode(OpCode::LOOP_BEGIN), 0, { state, nullGate }, GateType::Empty());
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::LOOP_BACK), 0, { state }, GateType::Empty());
}

GateRef CircuitBuilder::IfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, { ifBranch }, GateType::Empty());
}

GateRef CircuitBuilder::IfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, { ifBranch }, GateType::Empty());
}

GateRef CircuitBuilder::SwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_CASE), value, { switchBranch }, GateType::Empty());
}

GateRef CircuitBuilder::DefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(OpCode(OpCode::DEFAULT_CASE), 0, { switchBranch }, GateType::Empty());
}

GateRef CircuitBuilder::DependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::DEPEND_RELAY), 0, { state, depend }, GateType::Empty());
}

GateRef CircuitBuilder::DependAnd(std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    return circuit_->NewGate(OpCode(OpCode::DEPEND_AND), args.size(), inputs, GateType::Empty());
}

GateRef CircuitBuilder::Arguments(size_t index)
{
    auto argListOfCircuit = Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::ARG), MachineType::I64, index, {argListOfCircuit},
                                 GateType::NJSValue());
}

GateRef CircuitBuilder::TypeCheck(GateType type, GateRef gate)
{
    return GetCircuit()->NewGate(OpCode(OpCode::TYPE_CHECK), static_cast<uint64_t>(type.Value()),
                                 {gate}, GateType::NJSValue());
}

GateRef CircuitBuilder::TypedBinaryOperator(MachineType type, TypedBinOp binOp, GateType typeLeft, GateType typeRight,
                                            std::vector<GateRef> inList, GateType gateType)
{
    // get BinaryOpCode from a constant gate
    auto bin = Int8(static_cast<int8_t>(binOp));
    inList.emplace_back(bin);
    // merge two expected types of valueIns
    uint64_t operandTypes = (static_cast<uint64_t>(typeLeft.Value()) << OPRAND_TYPE_BITS) |
                          static_cast<uint64_t>(typeRight.Value());
    return GetCircuit()->NewGate(OpCode(OpCode::TYPED_BINARY_OP), type, operandTypes, inList, gateType);
}

GateRef CircuitBuilder::TypeConvert(MachineType type, GateType typeFrom, GateType typeTo,
                                    const std::vector<GateRef>& inList)
{
    // merge types of valueIns before and after convertion
    uint64_t operandTypes = (static_cast<uint64_t>(typeFrom.Value()) << OPRAND_TYPE_BITS) |
                          static_cast<uint64_t>(typeTo.Value());
    return GetCircuit()->NewGate(OpCode(OpCode::TYPE_CONVERT), type, operandTypes, inList, GateType::AnyType());
}

GateRef CircuitBuilder::TypedUnaryOperator(MachineType type, TypedUnOp unaryOp, GateType typeVal,
                                           const std::vector<GateRef>& inList, GateType gateType)
{
    auto unaryOpIdx = static_cast<uint64_t>(unaryOp);
    uint64_t bitfield = (static_cast<uint64_t>(typeVal.Value()) << OPRAND_TYPE_BITS) | unaryOpIdx;
    return GetCircuit()->NewGate(OpCode(OpCode::TYPED_UNARY_OP), type, bitfield, inList, gateType);
}

GateRef CircuitBuilder::Int8(int8_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I8, val, GateType::NJSValue());
}

GateRef CircuitBuilder::Int16(int16_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I16, val, GateType::NJSValue());
}

GateRef CircuitBuilder::Int32(int32_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I32, static_cast<BitField>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::Int64(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, val, GateType::NJSValue());
}

GateRef CircuitBuilder::IntPtr(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::ARCH, val, GateType::NJSValue());
}

GateRef CircuitBuilder::RelocatableData(uint64_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::RELOCATABLE_DATA), val, {constantList}, GateType::TaggedValue());
}

GateRef CircuitBuilder::Boolean(bool val)
{
    return GetCircuit()->GetConstantGate(MachineType::I1, val ? 1 : 0, GateType::NJSValue());
}

GateRef CircuitBuilder::Double(double val)
{
    return GetCircuit()->GetConstantGate(MachineType::F64, bit_cast<int64_t>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::HoleConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_HOLE, type);
}

GateRef CircuitBuilder::NullConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_NULL, type);
}

GateRef CircuitBuilder::ExceptionConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_EXCEPTION, type);
}

MachineType CircuitBuilder::GetMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

GateRef CircuitBuilder::BinaryArithmetic(OpCode opcode, MachineType machineType, GateRef left, GateRef right)
{
    auto circuit = GetCircuit();
    GateType type = acc_.GetGateType(left);
    return circuit->NewGate(opcode, machineType, 0, { left, right }, type);
}

GateRef CircuitBuilder::TaggedNumber(OpCode opcode, GateRef value)
{
    return GetCircuit()->NewGate(opcode, 0, { value }, GateType::TaggedValue());
}

GateRef CircuitBuilder::UnaryArithmetic(OpCode opcode, MachineType machineType, GateRef value)
{
    return GetCircuit()->NewGate(opcode, machineType, 0, { value }, GateType::NJSValue());
}

GateRef CircuitBuilder::UnaryArithmetic(OpCode opcode, GateRef value)
{
    return GetCircuit()->NewGate(opcode, 0, { value }, GateType::NJSValue());
}

GateRef CircuitBuilder::BinaryLogic(OpCode opcode, GateRef left, GateRef right)
{
    return GetCircuit()->NewGate(opcode, 0, { left, right }, GateType::NJSValue());
}

GateRef CircuitBuilder::BinaryCmp(OpCode opcode, GateRef left, GateRef right, BitField condition)
{
    return GetCircuit()->NewGate(opcode, condition, { left, right }, GateType::NJSValue());
}

GateRef CircuitBuilder::CallBCHandler(GateRef glue, GateRef target, const std::vector<GateRef> &args)
{
    const CallSignature *cs = BytecodeStubCSigns::BCHandler();
    ASSERT(cs->IsBCStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallBuiltin(GateRef glue, GateRef target, const std::vector<GateRef> &args)
{
    const CallSignature *cs = BuiltinsStubCSigns::BuiltinsHandler();
    ASSERT(cs->IsBuiltinsStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallBCDebugger(GateRef glue, GateRef target, const std::vector<GateRef> &args)
{
    const CallSignature *cs = BytecodeStubCSigns::BCDebuggerHandler();
    ASSERT(cs->IsBCDebuggerStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args)
{
    GateRef target = IntPtr(index);
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    ASSERT(cs->IsRuntimeStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntimeVarargs(GateRef glue, int index, GateRef argc, GateRef argv)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntimeWithArgv));
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    ASSERT(cs->IsRuntimeVAStub());
    GateRef result = Call(cs, glue, target, depend, {argc, argv});
    label->SetDepend(result);
    return result;
}

// call operation
GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    ASSERT(cs->IsRuntimeNGCStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallStub(GateRef glue, int index, const std::vector<GateRef> &args)
{
    const CallSignature *cs = CommonStubCSigns::Get(index);
    ASSERT(cs->IsCommonStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::Call(const CallSignature* cs, GateRef glue, GateRef target, GateRef depend,
                             const std::vector<GateRef> &args)
{
    std::vector<GateRef> inputs { depend, target, glue };
    inputs.insert(inputs.end(), args.begin(), args.end());
    OpCode op(OpCode::NOP);
    if (cs->IsCommonStub()) {
        op = OpCode(OpCode::CALL);
    } else if (cs->IsRuntimeVAStub()) {
        op = OpCode(OpCode::RUNTIME_CALL_WITH_ARGV);
    } else if (cs->IsRuntimeStub()) {
        op = OpCode(OpCode::RUNTIME_CALL);
    } else if (cs->IsBCDebuggerStub()) {
        op = OpCode(OpCode::DEBUGGER_BYTECODE_CALL);
    } else if (cs->IsBCHandlerStub()) {
        op = OpCode(OpCode::BYTECODE_CALL);
    } else if (cs->IsBuiltinsStub()) {
        op = OpCode(OpCode::BUILTINS_CALL);
    } else if (cs->IsRuntimeNGCStub()) {
        op = OpCode(OpCode::NOGC_RUNTIME_CALL);
    } else {
        UNREACHABLE();
    }
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    GateRef result = GetCircuit()->NewGate(op, machineType, args.size() + 2, inputs, type);
    return result;
}

// memory
void CircuitBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef ptr = PtrAdd(base, offset);
    GateRef result = GetCircuit()->NewGate(OpCode(OpCode::STORE), 0, { depend, value, ptr }, type.GetGateType());
    label->SetDepend(result);
    if (type == VariableType::JS_POINTER() || type == VariableType::JS_ANY()) {
        CallStub(glue, CommonStubCSigns::SetValueWithBarrier, { glue, base, offset, value });
    }
}

GateRef CircuitBuilder::Alloca(int size)
{
    auto allocaList = Circuit::GetCircuitRoot(OpCode(OpCode::ALLOCA_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::ALLOCA), size, { allocaList }, GateType::NJSValue());
}

GateRef CircuitBuilder::ToLength(GateRef receiver)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(OpCode(OpCode::TO_LENGTH), MachineType::I64,
                                     { currentControl, currentDepend, receiver }, GateType::NumberType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::HeapAlloc(GateRef size, GateType type, RegionSpaceFlag flag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(OpCode(OpCode::HEAP_ALLOC), flag,
                                     { currentControl, currentDepend, size }, type);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadElement(GateRef receiver, GateRef index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(OpCode(OpCode::LOAD_ELEMENT), MachineType::I64,
                                     { currentControl, currentDepend, receiver, index }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StoreElement(GateRef receiver, GateRef index, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(OpCode(OpCode::STORE_ELEMENT), MachineType::I64,
                                     { currentControl, currentDepend, receiver, index, value }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadProperty(GateRef receiver, GateRef key)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(OpCode(OpCode::LOAD_PROPERTY), MachineType::I64,
                                     { currentControl, currentDepend, receiver, key }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StoreProperty(GateRef receiver, GateRef key, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(OpCode(OpCode::STORE_PROPERTY), MachineType::I64,
                                     { currentControl, currentDepend, receiver, key, value }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TaggedIsString(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Equal(GetObjectType(LoadHClass(obj)),
            Int32(static_cast<int32_t>(JSType::STRING)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsStringOrSymbol(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Equal(objType, Int32(static_cast<int32_t>(JSType::STRING)));
        Label isString(env_);
        Label notString(env_);
        Branch(*result, &exit, &notString);
        Bind(&notString);
        {
            result = Equal(objType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::IsUtf16String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_UNCOMPRESSED));
}

GateRef CircuitBuilder::GetGlobalObject(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetGlobalObjOffset(cmpCfg_->Is32Bit()));
    return Load(VariableType::JS_ANY(), glue, offset);
}

GateRef CircuitBuilder::GetMethodFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunctionBase::METHOD_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::GetModuleFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::GetHomeObjectFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::GetLengthFromString(GateRef value)
{
    GateRef len = Load(VariableType::INT32(), value, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32LSR(len, Int32(2));  // 2 : 2 means len must be right shift 2 bits
}

GateRef CircuitBuilder::GetHashcodeFromString(GateRef glue, GateRef value)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label noRawHashcode(env_);
    Label exit(env_);
    DEFVAlUE(hashcode, env_, VariableType::INT32(), Int32(0));
    hashcode = Load(VariableType::INT32(), value, IntPtr(EcmaString::HASHCODE_OFFSET));
    Branch(Int32Equal(*hashcode, Int32(0)), &noRawHashcode, &exit);
    Bind(&noRawHashcode);
    {
        hashcode = GetInt32OfTInt(CallRuntime(glue, RTSTUB_ID(ComputeHashcode), Gate::InvalidGateRef, { value }));
        Store(VariableType::INT32(), glue, value, IntPtr(EcmaString::HASHCODE_OFFSET), *hashcode);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *hashcode;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsBigInt(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(obj)),
                            Int32(static_cast<int32_t>(JSType::BIGINT)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

void CircuitBuilder::SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

GateRef CircuitBuilder::GetLexicalEnv(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

void CircuitBuilder::SetModuleToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value);
}

void CircuitBuilder::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset, VariableType type)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(Int32Add(inlinedPropsStart, attrOffset),
        Int32(JSTaggedValue::TaggedTypeSize()));
    Store(type, glue, obj, ZExtInt32ToPtr(propOffset), value);
}

void CircuitBuilder::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

Environment::Environment(size_t arguments, CircuitBuilder *builder)
    : circuit_(builder->GetCircuit()), circuitBuilder_(builder), arguments_(arguments)
{
    circuitBuilder_->SetEnvironment(this);
    for (size_t i = 0; i < arguments; i++) {
        arguments_[i] = circuitBuilder_->Arguments(i);
    }
    entry_ = Label(NewLabel(this, Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY))));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto depend_entry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    currentLabel_->SetDepend(depend_entry);
}

Environment::Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder)
    : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    GateAccessor acc(circuit);
    entry_ = Label(NewLabel(this, acc.GetIn(hir, 0)));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto dependEntry = acc.GetDep(hir);
    currentLabel_->SetDepend(dependEntry);
    for (size_t i = 2; i < acc.GetNumIns(hir); i++) {
        inputList_.emplace_back(acc.GetIn(hir, i));
    }
}

Environment::Environment(GateRef stateEntry, GateRef dependEntry, std::vector<GateRef>& inlist,
    Circuit *circuit, CircuitBuilder *builder) : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    entry_ = Label(NewLabel(this, stateEntry));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    currentLabel_->SetDepend(dependEntry);
    for (auto in : inlist) {
        inputList_.emplace_back(in);
    }
}

Environment::~Environment()
{
    circuitBuilder_->SetEnvironment(nullptr);
    for (auto label : rawLabels_) {
        delete label;
    }
}

void CircuitBuilder::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = IfTrue(ifBranch);
    trueLabel->AppendPredecessor(GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = IfFalse(ifBranch);
    falseLabel->AppendPredecessor(GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = SwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    env_->SetCurrentLabel(loopHead);
}

void CircuitBuilder::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    env_->SetCurrentLabel(nullptr);
}

Label::Label(Environment *env)
{
    impl_ = env->NewLabel(env);
}

Label::Label(CircuitBuilder *cirBuilder)
{
    auto env = cirBuilder->GetCurrentEnvironment();
    impl_ = env->NewLabel(env);
}

void Label::LabelImpl::Seal()
{
    for (auto &[variable, gate] : incompletePhis_) {
        variable->AddPhiOperand(gate);
    }
    isSealed_ = true;
}

void Label::LabelImpl::WriteVariable(Variable *var, GateRef value)
{
    valueMap_[var] = value;
}

GateRef Label::LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        GateAccessor acc(env_->GetCircuit());
        if (!acc.GetOpCode(result).IsNop()) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef Label::LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    MachineType MachineType = CircuitBuilder::GetMachineTypeFromVariableType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;
        if (MachineType == MachineType::NOVALUE) {
            val = env_->GetBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
                predeControl_, {}, valueCounts, var->Type());
        } else {
            val = env_->GetBuilder()->Selector(OpCode(OpCode::VALUE_SELECTOR),
                MachineType, predeControl_, {}, valueCounts, var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (MachineType == MachineType::NOVALUE) {
            val = env_->GetBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
                predeControl_, {}, this->predecessors_.size(), var->Type());
        } else {
            val = env_->GetBuilder()->Selector(OpCode(OpCode::VALUE_SELECTOR), MachineType,
                predeControl_, {}, this->predecessors_.size(), var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        WriteVariable(var, val);
        val = var->AddPhiOperand(val);
    }
    WriteVariable(var, val);
    return val;
}

void Label::LabelImpl::Bind()
{
    ASSERT(!predecessors_.empty());
    if (IsLoopHead()) {
        // 2 means input number of depend selector gate
        loopDepend_ = env_->GetBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR), predeControl_, {}, 2);
        GateAccessor(env_->GetCircuit()).NewIn(loopDepend_, 1, predecessors_[0]->GetDepend());
        depend_ = loopDepend_;
    }
    if (IsNeedSeal()) {
        Seal();
        MergeAllControl();
        MergeAllDepend();
    }
}

void Label::LabelImpl::MergeAllControl()
{
    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        return;
    }

    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        ASSERT(otherPredeControls_.size() == 1);
        GateAccessor(env_->GetCircuit()).NewIn(predeControl_, 1, otherPredeControls_[0]);
        return;
    }

    // merge all control of predecessors_
    std::vector<GateRef> inGates(predecessors_.size());
    size_t i = 0;
    ASSERT(predeControl_ != -1);
    ASSERT((otherPredeControls_.size() + 1) == predecessors_.size());
    inGates[i++] = predeControl_;
    for (auto in : otherPredeControls_) {
        inGates[i++] = in;
    }

    GateRef merge = env_->GetBuilder()->Merge(inGates.data(), inGates.size());
    predeControl_ = merge;
    control_ = merge;
}

void Label::LabelImpl::MergeAllDepend()
{
    if (IsControlCase()) {
        // Add depend_relay to current label
        auto denpendEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        dependRelay_ = env_->GetBuilder()->DependRelay(predeControl_, denpendEntry);
    }

    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (dependRelay_ != -1) {
            depend_ = env_->GetBuilder()->DependAnd({depend_, dependRelay_});
        }
        return;
    }
    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        // Add loop depend to in of depend_seclector
        ASSERT(loopDepend_ != -1);
        // 2 mean 3rd input gate for loopDepend_(depend_selector)
        GateAccessor(env_->GetCircuit()).NewIn(loopDepend_, 2, predecessors_[1]->GetDepend());
        return;
    }

    //  Merge all depends to depend_seclector
    std::vector<GateRef> dependsList;
    for (auto prede : this->GetPredecessors()) {
        dependsList.push_back(prede->GetDepend());
    }
    depend_ = env_->GetBuilder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
        predeControl_, dependsList, dependsList.size());
}

void Label::LabelImpl::AppendPredecessor(Label::LabelImpl *predecessor)
{
    if (predecessor != nullptr) {
        predecessors_.push_back(predecessor);
    }
}

bool Label::LabelImpl::IsNeedSeal() const
{
    auto stateCount = GateAccessor(env_->GetCircuit()).GetStateCount(predeControl_);
    return predecessors_.size() >= stateCount;
}

bool Label::LabelImpl::IsLoopHead() const
{
    return GateAccessor(env_->GetCircuit()).IsLoopHead(predeControl_);
}

bool Label::LabelImpl::IsControlCase() const
{
    return GateAccessor(env_->GetCircuit()).IsControlCase(predeControl_);
}

GateRef Variable::AddPhiOperand(GateRef val)
{
    ASSERT(GateAccessor(env_->GetCircuit()).IsSelector(val));
    Label label = env_->GetLabelFromSelector(val);
    size_t idx = 0;
    for (auto pred : label.GetPredecessors()) {
        auto preVal = pred.ReadVariable(this);
        ASSERT(!GateAccessor(env_->GetCircuit()).GetOpCode(preVal).IsNop());
        idx++;
        val = AddOperandToSelector(val, idx, preVal);
    }
    return TryRemoveTrivialPhi(val);
}

GateRef Variable::AddOperandToSelector(GateRef val, size_t idx, GateRef in)
{
    GateAccessor(env_->GetCircuit()).NewIn(val, idx, in);
    return val;
}

GateRef Variable::TryRemoveTrivialPhi(GateRef phi)
{
    GateAccessor acc(GetCircuit());
    GateRef same = Gate::InvalidGateRef;
    const size_t inNum = acc.GetNumIns(phi);
    for (size_t i = 1; i < inNum; ++i) {
        GateRef phiIn = acc.GetIn(phi, i);
        if (phiIn == same || phiIn == phi) {
            continue;  // unique value or self-reference
        }
        if (same != Gate::InvalidGateRef) {
            return phi;  // the phi merges at least two valusses: not trivial
        }
        same = phiIn;
    }
    if (same == Gate::InvalidGateRef) {
        // the phi is unreachable or in the start block
        GateType type = acc.GetGateType(phi);
        same = env_->GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, type);
    }
    // remove the trivial phi
    // get all users of phi except self
    std::vector<GateRef> outs;
    auto uses = acc.Uses(phi);
    for (auto use = uses.begin(); use != uses.end();) {
        GateRef u = *use;
        if (u != phi) {
            outs.push_back(u);
            use = acc.ReplaceIn(use, same);
        } else {
            use++;
        }
    }
    acc.DeleteGate(phi);

    // try to recursiveby remove all phi users, which might have vecome trivial
    for (auto out : outs) {
        if (acc.IsSelector(out)) {
            auto result = TryRemoveTrivialPhi(out);
            if (same == out) {
                same = result;
            }
        }
    }
    return same;
}
}  // namespace panda::ecmascript::kungfu
