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
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
GateRef CircuitBuilder::Merge(const std::vector<GateRef> &inList)
{
    return circuit_->NewGate(circuit_->Merge(inList.size()), inList);
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
    ASSERT((opcode == OpCode::VALUE_SELECTOR) || (opcode == OpCode::DEPEND_SELECTOR));
    const GateMetaData* meta = (opcode == OpCode::DEPEND_SELECTOR) ?
        circuit_->DependSelector(valueCounts) : circuit_->ValueSelector(valueCounts);
    return circuit_->NewGate(meta, machineType, inList.size(), inList.data(), type.GetGateType());
}

GateRef CircuitBuilder::Selector(OpCode opcode, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    MachineType machineType = (opcode == OpCode::DEPEND_SELECTOR) ?
        MachineType::NOVALUE : MachineType::FLEX;
    return Selector(opcode, machineType, control, values, valueCounts, type);
}

GateRef CircuitBuilder::UndefineConstant()
{
    auto type = GateType::TaggedValue();
    return circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition, uint32_t trueWeight, uint32_t falseWeight)
{
    auto value = BranchAccessor::ToValue(trueWeight, falseWeight);
    return circuit_->NewGate(circuit_->IfBranch(value), { state, condition });
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(circuit_->SwitchBranch(caseCounts), { state, index });
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = circuit_->GetReturnRoot();
    return circuit_->NewGate(circuit_->Return(), { state, depend, value, returnList });
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = circuit_->GetReturnRoot();
    return circuit_->NewGate(circuit_->ReturnVoid(), { state, depend, returnList });
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(circuit_->OrdinaryBlock(), { state });
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(circuit_->LoopBegin(), { state, nullGate });
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(circuit_->LoopBack(), { state });
}

GateRef CircuitBuilder::IfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(circuit_->IfTrue(), { ifBranch });
}

GateRef CircuitBuilder::IfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(circuit_->IfFalse(), { ifBranch });
}

GateRef CircuitBuilder::SwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(circuit_->SwitchCase(value), { switchBranch });
}

GateRef CircuitBuilder::DefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(circuit_->DefaultCase(), { switchBranch });
}

GateRef CircuitBuilder::DependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(circuit_->DependRelay(), { state, depend });
}

GateRef CircuitBuilder::ReadSp()
{
    return circuit_->NewGate(circuit_->ReadSp(), MachineType::I64, GateType::NJSValue());
}

GateRef CircuitBuilder::Arguments(size_t index)
{
    auto argListOfCircuit = circuit_->GetArgRoot();
    return GetCircuit()->NewArg(MachineType::I64, index, GateType::NJSValue(), argListOfCircuit);
}

GateRef CircuitBuilder::ObjectTypeCheck(GateType type, bool isHeapObject, GateRef gate, GateRef hclassIndex)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    ObjectTypeAccessor accessor(type, isHeapObject);
    GateRef ret = GetCircuit()->NewGate(circuit_->ObjectTypeCheck(accessor.ToValue()), MachineType::I1,
        {currentControl, currentDepend, gate, hclassIndex, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ObjectTypeCompare(GateType type, bool isHeapObject, GateRef gate, GateRef hclassIndex)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    ObjectTypeAccessor accessor(type, isHeapObject);
    GateRef ret = GetCircuit()->NewGate(circuit_->ObjectTypeCompare(accessor.ToValue()), MachineType::I1,
        {currentControl, currentDepend, gate, hclassIndex, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::HeapObjectCheck(GateRef gate, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->HeapObjectCheck(),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StableArrayCheck(GateRef gate, ElementsKind kind, ArrayMetaDataAccessor::Mode mode)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    ArrayMetaDataAccessor accessor(kind, mode);
    GateRef ret = GetCircuit()->NewGate(circuit_->StableArrayCheck(accessor.ToValue()),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::COWArrayCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->COWArrayCheck(),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::EcmaStringCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->EcmaStringCheck(),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::FlattenStringCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->FlattenStringCheck(),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::HClassStableArrayCheck(GateRef gate, GateRef frameState, ArrayMetaDataAccessor accessor)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->HClassStableArrayCheck(accessor.ToValue()),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayGuardianCheck(GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayGuardianCheck(),
        MachineType::I1, {currentControl, currentDepend, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedArrayCheck(GateType type, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedArrayCheck(static_cast<size_t>(type.Value())),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadTypedArrayLength(GateType type, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->LoadTypedArrayLength(static_cast<size_t>(type.Value())),
        MachineType::I64, {currentControl, currentDepend, gate}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::RangeGuard(GateRef gate, uint32_t left, uint32_t right)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    UInt32PairAccessor accessor(left, right);
    GateRef ret = GetCircuit()->NewGate(circuit_->RangeGuard(accessor.ToValue()),
        MachineType::I64, {currentControl, currentDepend, gate}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IndexCheck(GateType type, GateRef gate, GateRef index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->IndexCheck(static_cast<size_t>(type.Value())),
        MachineType::I64, {currentControl, currentDepend, gate, index, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IsJsCOWArray(GateRef obj)
{
    // Elements of JSArray are shared and properties are not yet.
    GateRef elements = GetElementsArray(obj);
    GateRef objectType = GetObjectType(LoadHClass(elements));
    return IsCOWArray(objectType);
}

GateRef CircuitBuilder::IsCOWArray(GateRef objectType)
{
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_TAGGED_ARRAY)));
}

GateRef CircuitBuilder::GetElementsArray(GateRef object)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    return Load(VariableType::JS_POINTER(), object, elementsOffset);
}

MachineType CircuitBuilder::GetMachineTypeOfValueType(ValueType type)
{
    switch (type) {
        case ValueType::BOOL:
            return MachineType::I1;
        case ValueType::INT32:
            return MachineType::I32;
        case ValueType::FLOAT64:
            return MachineType::F64;
        case ValueType::TAGGED_BOOLEAN:
        case ValueType::TAGGED_INT:
        case ValueType::TAGGED_DOUBLE:
        case ValueType::TAGGED_NUMBER:
            return MachineType::I64;
        default:
            return MachineType::NOVALUE;
    }
}

GateType CircuitBuilder::GetGateTypeOfValueType(ValueType type)
{
    switch (type) {
        case ValueType::BOOL:
        case ValueType::INT32:
        case ValueType::FLOAT64:
            return GateType::NJSValue();
        case ValueType::TAGGED_BOOLEAN:
            return GateType::BooleanType();
        case ValueType::TAGGED_INT:
            return GateType::IntType();
        case ValueType::TAGGED_DOUBLE:
            return GateType::DoubleType();
        case ValueType::TAGGED_NUMBER:
            return GateType::NumberType();
        default:
            return GateType::Empty();
    }
}

GateRef CircuitBuilder::CheckAndConvert(GateRef gate, ValueType src, ValueType dst, ConvertSupport support)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto stateSplit = acc_.FindNearestStateSplit(currentDepend);
    auto frameState = acc_.GetFrameState(stateSplit);
    MachineType machineType = GetMachineTypeOfValueType(dst);
    GateType gateType = GetGateTypeOfValueType(dst);
    uint64_t value = ValuePairTypeAccessor::ToValue(src, dst, support);
    GateRef ret = GetCircuit()->NewGate(circuit_->CheckAndConvert(value),
        machineType, {currentControl, currentDepend, gate, frameState}, gateType);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Convert(GateRef gate, ValueType src, ValueType dst)
{
    MachineType machineType = GetMachineTypeOfValueType(dst);
    GateType gateType = GetGateTypeOfValueType(dst);
    uint64_t value = ValuePairTypeAccessor::ToValue(src, dst);
    GateRef ret = GetCircuit()->NewGate(circuit_->Convert(value), machineType, {gate}, gateType);
    return ret;
}

GateRef CircuitBuilder::ConvertBoolToInt32(GateRef gate, ConvertSupport support)
{
    return CheckAndConvert(gate, ValueType::BOOL, ValueType::INT32, support);
}

GateRef CircuitBuilder::ConvertBoolToFloat64(GateRef gate, ConvertSupport support)
{
    return CheckAndConvert(gate, ValueType::BOOL, ValueType::FLOAT64, support);
}

GateRef CircuitBuilder::ConvertInt32ToFloat64(GateRef gate)
{
    return Convert(gate, ValueType::INT32, ValueType::FLOAT64);
}

GateRef CircuitBuilder::ConvertUInt32ToFloat64(GateRef gate)
{
    return Convert(gate, ValueType::UINT32, ValueType::FLOAT64);
}

GateRef CircuitBuilder::ConvertFloat64ToInt32(GateRef gate)
{
    return Convert(gate, ValueType::FLOAT64, ValueType::INT32);
}

GateRef CircuitBuilder::ConvertBoolToTaggedBoolean(GateRef gate)
{
    return Convert(gate, ValueType::BOOL, ValueType::TAGGED_BOOLEAN);
}

GateRef CircuitBuilder::ConvertTaggedBooleanToBool(GateRef gate)
{
    return Convert(gate, ValueType::TAGGED_BOOLEAN, ValueType::BOOL);
}

GateRef CircuitBuilder::ConvertInt32ToTaggedInt(GateRef gate)
{
    return Convert(gate, ValueType::INT32, ValueType::TAGGED_INT);
}

GateRef CircuitBuilder::ConvertUInt32ToTaggedNumber(GateRef gate)
{
    return Convert(gate, ValueType::UINT32, ValueType::TAGGED_NUMBER);
}

GateRef CircuitBuilder::ConvertInt32ToBool(GateRef gate)
{
    return Convert(gate, ValueType::INT32, ValueType::BOOL);
}

GateRef CircuitBuilder::ConvertUInt32ToBool(GateRef gate)
{
    return Convert(gate, ValueType::UINT32, ValueType::BOOL);
}

GateRef CircuitBuilder::ConvertFloat64ToBool(GateRef gate)
{
    return Convert(gate, ValueType::FLOAT64, ValueType::BOOL);
}

GateRef CircuitBuilder::CheckTaggedBooleanAndConvertToBool(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_BOOLEAN, ValueType::BOOL);
}

GateRef CircuitBuilder::CheckTaggedNumberAndConvertToBool(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_NUMBER, ValueType::BOOL);
}

GateRef CircuitBuilder::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    return Convert(gate, ValueType::FLOAT64, ValueType::TAGGED_DOUBLE);
}

GateRef CircuitBuilder::CheckUInt32AndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::UINT32, ValueType::INT32);
}

GateRef CircuitBuilder::CheckTaggedIntAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_INT, ValueType::INT32);
}

GateRef CircuitBuilder::CheckTaggedDoubleAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_DOUBLE, ValueType::INT32);
}

GateRef CircuitBuilder::CheckTaggedNumberAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_NUMBER, ValueType::INT32);
}

GateRef CircuitBuilder::CheckTaggedIntAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_INT, ValueType::FLOAT64);
}

GateRef CircuitBuilder::CheckTaggedDoubleAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_DOUBLE, ValueType::FLOAT64);
}

GateRef CircuitBuilder::CheckTaggedNumberAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_NUMBER, ValueType::FLOAT64);
}

GateRef CircuitBuilder::TryPrimitiveTypeCheck(GateType type, GateRef gate)
{
    if (acc_.GetOpCode(gate) == OpCode::CONSTANT) {
        return Circuit::NullGate();
    }
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->PrimitiveTypeCheck(static_cast<size_t>(type.Value())),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::CallTargetCheck(GateRef gate, GateRef function, GateRef id, GateRef param, const char* comment)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.GetFrameState(gate);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedCallCheck(),
                                        MachineType::I1,
                                        { currentControl, currentDepend, function, id, param, frameState},
                                        GateType::NJSValue(),
                                        comment);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::JSCallTargetFromDefineFuncCheck(GateType type, GateRef func, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.GetFrameState(gate);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedCallTargetCheckOp(1, static_cast<size_t>(type.Value()),
        TypedCallTargetCheckOp::JSCALL_IMMEDIATE_AFTER_FUNC_DEF),
        MachineType::I1, {currentControl, currentDepend, func, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::DeoptCheck(GateRef condition, GateRef frameState, DeoptType type)
{
    std::string comment = Deoptimizier::DisplayItems(type);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ASSERT(acc_.GetOpCode(frameState) == OpCode::FRAME_STATE);
    GateRef ret = GetCircuit()->NewGate(circuit_->DeoptCheck(),
        MachineType::I1, { currentControl, currentDepend, condition,
        frameState, Int64(static_cast<int64_t>(type))}, GateType::NJSValue(), comment.c_str());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedNewAllocateThis(GateRef ctor, GateRef hclassIndex, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedNewAllocateThis(),
        MachineType::ANYVALUE, {currentControl, currentDepend, ctor, hclassIndex, frameState}, GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedSuperAllocateThis(GateRef superCtor, GateRef newTarget, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedSuperAllocateThis(), MachineType::ANYVALUE,
        {currentControl, currentDepend, superCtor, newTarget, frameState}, GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::GetSuperConstructor(GateRef ctor)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->GetSuperConstructor(), MachineType::ANYVALUE,
                                     { currentControl, currentDepend, ctor }, GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedCallOperator(GateRef hirGate, MachineType type, const std::vector<GateRef> &inList)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    auto numValueIn = inList.size() - 2; // 2: state & depend
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    return GetCircuit()->NewGate(circuit_->TypedCallBuiltin(numValueIn, pcOffset), type, inList.size(), inList.data(),
                                 GateType::AnyType());
}

GateRef CircuitBuilder::AddWithOverflow(GateRef left, GateRef right)
{
    return GetCircuit()->NewGate(circuit_->AddWithOverflow(), MachineType::I64, {left, right}, GateType::AnyType());
}

GateRef CircuitBuilder::SubWithOverflow(GateRef left, GateRef right)
{
    return GetCircuit()->NewGate(circuit_->SubWithOverflow(), MachineType::I64, {left, right}, GateType::AnyType());
}

GateRef CircuitBuilder::MulWithOverflow(GateRef left, GateRef right)
{
    return GetCircuit()->NewGate(circuit_->MulWithOverflow(), MachineType::I64, {left, right}, GateType::AnyType());
}

GateRef CircuitBuilder::ExtractValue(MachineType mt, GateRef pointer, GateRef index)
{
    ASSERT(acc_.GetOpCode(index) == OpCode::CONSTANT);
    ASSERT(acc_.GetMachineType(index) == MachineType::I32);
    return GetCircuit()->NewGate(circuit_->ExtractValue(), mt, {pointer, index}, GateType::NJSValue());
}

GateRef CircuitBuilder::Sqrt(GateRef param)
{
    return GetCircuit()->NewGate(circuit_->Sqrt(), MachineType::F64, {param}, GateType::DoubleType());
}

GateRef CircuitBuilder::Int32CheckRightIsZero(GateRef right)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->Int32CheckRightIsZero(),
    MachineType::I1, {currentControl, currentDepend, right, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Float64CheckRightIsZero(GateRef right)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->Float64CheckRightIsZero(),
    MachineType::I1, {currentControl, currentDepend, right, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LexVarIsHoleCheck(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->LexVarIsHoleCheck(),
    MachineType::I1, {currentControl, currentDepend, value, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ValueCheckNegOverflow(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->ValueCheckNegOverflow(),
    MachineType::I1, {currentControl, currentDepend, value, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::OverflowCheck(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->OverflowCheck(),
        MachineType::I1, {currentControl, currentDepend, value, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Int32UnsignedUpperBoundCheck(GateRef value, GateRef upperBound)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->Int32UnsignedUpperBoundCheck(),
        MachineType::I1, {currentControl, currentDepend, value, upperBound, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Int32DivWithCheck(GateRef left, GateRef right)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->Int32DivWithCheck(),
        MachineType::I32, {currentControl, currentDepend, left, right, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypeConvert(MachineType type, GateType typeFrom, GateType typeTo,
                                    const std::vector<GateRef>& inList)
{
    // merge types of valueIns before and after convertion
    uint64_t operandTypes = GatePairTypeAccessor::ToValue(typeFrom, typeTo);
    return GetCircuit()->NewGate(circuit_->TypedConvert(operandTypes),
        type, inList.size(), inList.data(), GateType::AnyType());
}

GateRef CircuitBuilder::TypedConditionJump(MachineType type, TypedJumpOp jumpOp, BranchKind branchKind,
    GateType typeVal, const std::vector<GateRef>& inList)
{
    uint64_t value = TypedJumpAccessor::ToValue(typeVal, jumpOp, branchKind);
    return GetCircuit()->NewGate(circuit_->TypedConditionJump(value),
        type, inList.size(), inList.data(), GateType::Empty());
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

GateRef CircuitBuilder::StringPtr(const std::string &str)
{
    return GetCircuit()->GetConstantStringGate(MachineType::ARCH, str, GateType::NJSValue());
}

GateRef CircuitBuilder::RelocatableData(uint64_t val)
{
    return GetCircuit()->NewGate(circuit_->RelocatableData(val),
        MachineType::ARCH, GateType::TaggedValue());
}

GateRef CircuitBuilder::Boolean(bool val)
{
    return GetCircuit()->GetConstantGate(MachineType::I1, val ? 1 : 0, GateType::NJSValue());
}

GateRef CircuitBuilder::Double(double val)
{
    return GetCircuit()->GetConstantGate(MachineType::F64, base::bit_cast<int64_t>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::HoleConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_HOLE, type);
}

GateRef CircuitBuilder::NullPtrConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, 0u, type);
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

GateRef CircuitBuilder::BinaryArithmetic(const GateMetaData* meta, MachineType machineType,
                                         GateRef left, GateRef right, GateType gateType)
{
    auto circuit = GetCircuit();
    if (gateType == GateType::Empty()) {
        gateType = acc_.GetGateType(left);
    }
    return circuit->NewGate(meta, machineType, { left, right }, gateType);
}

GateRef CircuitBuilder::BinaryCmp(const GateMetaData* meta, GateRef left, GateRef right)
{
    return GetCircuit()->NewGate(meta, MachineType::I1, { left, right }, GateType::NJSValue());
}

GateRef CircuitBuilder::CallBCHandler(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                      const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    const CallSignature *cs = BytecodeStubCSigns::BCHandler();
    ASSERT(cs->IsBCStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallBuiltin(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                    const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    const CallSignature *cs = BuiltinsStubCSigns::BuiltinsCSign();
    ASSERT(cs->IsBuiltinsStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallBuiltinWithArgv(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                            const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    const CallSignature *cs = BuiltinsStubCSigns::BuiltinsWithArgvCSign();
    ASSERT(cs->IsBuiltinsWithArgvStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallBCDebugger(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                       const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    const CallSignature *cs = BytecodeStubCSigns::BCDebuggerHandler();
    ASSERT(cs->IsBCDebuggerStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args,
                                    GateRef hirGate, const char* comment)
{
    GateRef target = IntPtr(index);
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    ASSERT(cs->IsRuntimeStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedJSFunctionFrame()) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, target, depend, args, filteredHirGate, comment);
    return result;
}

GateRef CircuitBuilder::CallRuntimeVarargs(GateRef glue, int index, GateRef argc, GateRef argv, const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntimeWithArgv));
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    ASSERT(cs->IsRuntimeVAStub());
    GateRef result = Call(cs, glue, target, depend, {argc, argv}, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args,
                                       GateRef hirGate, const char* comment)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    ASSERT(cs->IsRuntimeNGCStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedJSFunctionFrame() && RuntimeStubCSigns::IsAsmStub(index)) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, target, depend, args, filteredHirGate, comment);
    return result;
}

GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, GateRef gate, int index, const std::vector<GateRef> &args,
                                       bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = CallNGCRuntime(glue, index, Gate::InvalidGateRef, args, gate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(index);
        GateRef target = IntPtr(index);
        GateRef result = Call(cs, glue, target, GetDepend(), args, gate, name.c_str());
        return result;
    }
}

void CircuitBuilder::StartCallTimer(GateRef glue, GateRef gate, const std::vector<GateRef> &args, bool useLabel)
{
    (void)glue;
    (void)gate;
    (void)args;
    (void)useLabel;
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue, gate, RTSTUB_ID(StartCallTimer), args, useLabel);
#endif
}

void CircuitBuilder::EndCallTimer(GateRef glue, GateRef gate, const std::vector<GateRef> &args, bool useLabel)
{
    (void)glue;
    (void)gate;
    (void)args;
    (void)useLabel;
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue, gate, RTSTUB_ID(EndCallTimer), args, useLabel);
#endif
}

GateRef CircuitBuilder::FastCallOptimized(GateRef glue, GateRef code, GateRef depend, const std::vector<GateRef> &args,
                                          GateRef hirGate)
{
    const CallSignature *cs = RuntimeStubCSigns::GetOptimizedFastCallSign();
    ASSERT(cs->IsOptimizedFastCallStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedJSFunctionFrame()) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, code, depend, args, filteredHirGate);
    return result;
}

GateRef CircuitBuilder::CallOptimized(GateRef glue, GateRef code, GateRef depend, const std::vector<GateRef> &args,
                                      GateRef hirGate)
{
    const CallSignature *cs = RuntimeStubCSigns::GetOptimizedCallSign();
    ASSERT(cs->IsOptimizedStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedJSFunctionFrame()) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, code, depend, args, filteredHirGate);
    return result;
}

GateRef CircuitBuilder::CallStub(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                                 const char* comment)
{
    const CallSignature *cs = CommonStubCSigns::Get(index);
    ASSERT(cs->IsCommonStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result;
    if (GetCircuit()->IsOptimizedJSFunctionFrame()) {
        ASSERT(hirGate != Circuit::NullGate());
        result = Call(cs, glue, target, depend, args, hirGate, comment);
    } else {
        result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    }
    return result;
}

GateRef CircuitBuilder::CallBuiltinRuntime(GateRef glue, GateRef depend, const std::vector<GateRef> &args,
                                           bool isNew, const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    int index = 0;
    if (!isNew) {
        index = static_cast<int>(RTSTUB_ID(PushCallArgsAndDispatchNative));
    } else {
        index = static_cast<int>(RTSTUB_ID(PushCallNewAndDispatchNative));
    }

    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::Call(const CallSignature* cs, GateRef glue, GateRef target, GateRef depend,
                             const std::vector<GateRef> &args, GateRef hirGate, const char* comment)
{
    std::vector<GateRef> inputs { depend, target, glue };
    inputs.insert(inputs.end(), args.begin(), args.end());
    auto numValuesIn = args.size() + 2; // 2: target & glue
    if (GetCircuit()->IsOptimizedJSFunctionFrame() && hirGate != Circuit::NullGate()) {
        GateRef pcOffset = Int64(acc_.TryGetPcOffset(hirGate));
        inputs.emplace_back(pcOffset);
        numValuesIn += 1;
    }

    const GateMetaData* meta = nullptr;
    if (cs->IsCommonStub()) {
        meta = circuit_->Call(numValuesIn);
    } else if (cs->IsRuntimeVAStub()) {
        meta = circuit_->RuntimeCallWithArgv(numValuesIn);
    } else if (cs->IsRuntimeStub()) {
        meta = circuit_->RuntimeCall(numValuesIn);
    } else if (cs->IsBCDebuggerStub()) {
        meta = circuit_->DebuggerBytecodeCall(numValuesIn);
    } else if (cs->IsBCHandlerStub()) {
        meta = circuit_->BytecodeCall(numValuesIn);
    } else if (cs->IsBuiltinsStub()) {
        meta = circuit_->BuiltinsCall(numValuesIn);
    } else if (cs->IsBuiltinsWithArgvStub()) {
        meta = circuit_->BuiltinsCallWithArgv(numValuesIn);
    } else if (cs->IsRuntimeNGCStub()) {
        meta = circuit_->NoGcRuntimeCall(numValuesIn);
    } else if (cs->IsOptimizedStub()) {
        bool isNoGC = acc_.GetNoGCFlag(hirGate);
        meta = circuit_->CallOptimized(numValuesIn, isNoGC);
    } else if (cs->IsOptimizedFastCallStub()) {
        bool isNoGC = acc_.GetNoGCFlag(hirGate);
        meta = circuit_->FastCallOptimized(numValuesIn, isNoGC);
    } else {
        LOG_ECMA(FATAL) << "unknown call operator";
        UNREACHABLE();
    }
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    GateRef result = GetCircuit()->NewGate(meta, machineType, inputs.size(), inputs.data(), type, comment);
    auto label = GetCurrentLabel();
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::StoreMemory(MemoryType Op, VariableType type, GateRef receiver, GateRef index, GateRef value)
{
    auto opIdx = static_cast<uint64_t>(Op);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(GetCircuit()->StoreMemory(opIdx), type.GetMachineType(),
        {currentControl, currentDepend, receiver, index, value}, type.GetGateType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NoLabelCallRuntime(GateRef glue, GateRef depend, size_t index, std::vector<GateRef> &args,
                                           GateRef hirGate)
{
    const std::string name = RuntimeStubCSigns::GetRTName(RTSTUB_ID(CallRuntime));
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    GateRef target = IntPtr(index);
    std::vector<GateRef> inputs { depend, target, glue };
    inputs.insert(inputs.end(), args.begin(), args.end());
    auto numValuesIn = args.size() + 2; // 2: target & glue
    GateRef pcOffset = Int64(acc_.TryGetPcOffset(hirGate));
    inputs.emplace_back(pcOffset);
    numValuesIn += 1;

    const GateMetaData* meta = circuit_->RuntimeCall(numValuesIn);
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    GateRef result = circuit_->NewGate(meta, machineType, inputs.size(), inputs.data(), type, name.c_str());
    return result;
}

void CircuitBuilder::StoreWithNoBarrier(VariableType type, GateRef base, GateRef offset, GateRef value)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef ptr = PtrAdd(base, offset);
    GateRef result = GetCircuit()->NewGate(circuit_->Store(),
        MachineType::NOVALUE, { depend, value, ptr }, type.GetGateType());
    label->SetDepend(result);
}

// memory
void CircuitBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    StoreWithNoBarrier(type, base, offset, value);
    if (type == VariableType::JS_POINTER() || type == VariableType::JS_ANY()) {
        Label entry(env_);
        SubCfgEntry(&entry);
        Label exit(env_);
        Label isHeapObject(env_);
        Branch(TaggedIsHeapObject(value), &isHeapObject, &exit);
        Bind(&isHeapObject);
        {
            CallStub(glue, base, CommonStubCSigns::SetValueWithBarrier, { glue, base, offset, value });
            Jump(&exit);
        }
        Bind(&exit);
        SubCfgExit();
    }
}

GateRef CircuitBuilder::Alloca(size_t size)
{
    return GetCircuit()->NewGate(circuit_->Alloca(size), MachineType::ARCH, GateType::NJSValue());
}

GateRef CircuitBuilder::ToLength(GateRef receiver)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->ToLength(), MachineType::I64,
                                     { currentControl, currentDepend, receiver }, GateType::NumberType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadProperty(GateRef receiver, GateRef propertyLookupResult, bool isFunction)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->LoadProperty(isFunction), MachineType::I64,
                                     { currentControl, currentDepend, receiver, propertyLookupResult },
                                     GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StoreProperty(GateRef receiver, GateRef propertyLookupResult, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->StoreProperty(), MachineType::I64,
                                     { currentControl, currentDepend, receiver, propertyLookupResult, value },
                                     GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadArrayLength(GateRef array)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->LoadArrayLength(), MachineType::I64,
                                     { currentControl, currentDepend, array }, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadStringLength(GateRef string)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->LoadStringLength(), MachineType::I64,
                                     { currentControl, currentDepend, string }, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadConstOffset(VariableType type, GateRef receiver, size_t offset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();

    auto ret = GetCircuit()->NewGate(circuit_->LoadConstOffset(offset), type.GetMachineType(),
                                     { currentDepend, receiver }, type.GetGateType());
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StoreConstOffset(VariableType type,
    GateRef receiver, size_t offset, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();

    auto ret = GetCircuit()->NewGate(circuit_->StoreConstOffset(offset), type.GetMachineType(),
        { currentControl, currentDepend, receiver, value }, type.GetGateType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = GetConstPoolFromFunction(jsFunc);
    return GetValueFromTaggedArray(constPool, TruncInt64ToInt32(index));
}

GateRef CircuitBuilder::ConvertHoleAsUndefined(GateRef receiver)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();

    auto ret = GetCircuit()->NewGate(circuit_->ConvertHoleAsUndefined(),
        MachineType::I64, { currentControl, currentDepend, receiver }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Construct(GateRef hirGate, std::vector<GateRef> args)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    auto callGate = GetCircuit()->NewGate(circuit_->Construct(bitfield, pcOffset), MachineType::I64,
                                          args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::TypedCall(GateRef hirGate, std::vector<GateRef> args, bool isNoGC)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    auto callGate = GetCircuit()->NewGate(circuit_->TypedCall(bitfield, pcOffset, isNoGC), MachineType::I64,
                                          args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::TypedFastCall(GateRef hirGate, std::vector<GateRef> args, bool isNoGC)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    auto callGate = GetCircuit()->NewGate(circuit_->TypedFastCall(bitfield, pcOffset, isNoGC), MachineType::I64,
                                          args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallGetter(GateRef hirGate, GateRef receiver, GateRef propertyLookupResult,
                                   const char* comment)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto callGate = GetCircuit()->NewGate(circuit_->CallGetter(pcOffset),
                                          MachineType::I64,
                                          { currentControl, currentDepend, receiver, propertyLookupResult },
                                          GateType::AnyType(),
                                          comment);
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallSetter(GateRef hirGate, GateRef receiver, GateRef propertyLookupResult,
                                   GateRef value, const char* comment)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto callGate = GetCircuit()->NewGate(circuit_->CallSetter(pcOffset),
                                          MachineType::I64,
                                          { currentControl, currentDepend, receiver, propertyLookupResult, value },
                                          GateType::AnyType(),
                                          comment);
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::GetConstPool(GateRef jsFunc)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetConstPool(), MachineType::I64,
                                         { currentDepend, jsFunc },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalEnv()
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalEnv(), MachineType::I64,
                                         { currentDepend },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalEnvObj(GateRef env, size_t index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalEnvObj(index), MachineType::I64,
                                         { currentDepend, env },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalEnvObjHClass(GateRef env, size_t index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalEnvObjHClass(index), MachineType::I64,
                                         { currentDepend, env },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalConstantValue(ConstantIndex index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalConstantValue(static_cast<size_t>(index)),
                                         MachineType::I64, { currentDepend }, GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::HasPendingException(GateRef glue)
{
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env_->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    return TaggedIsNotHole(exception);
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
        result = TaggedObjectIsString(obj);
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
        result = TaggedObjectIsString(obj);
        Label isString(env_);
        Label notString(env_);
        Branch(*result, &exit, &notString);
        Bind(&notString);
        {
            GateRef objType = GetObjectType(LoadHClass(obj));
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

GateRef CircuitBuilder::GetConstPoolFromFunction(GateRef jsFunc)
{
    GateRef method = GetMethodFromFunction(jsFunc);
    return Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
}

GateRef CircuitBuilder::GetObjectFromConstPool(GateRef glue, GateRef hirGate, GateRef jsFunc, GateRef index,
                                               ConstPoolType type)
{
    GateRef constPool = GetConstPoolFromFunction(jsFunc);
    GateRef module = GetModuleFromFunction(jsFunc);
    return GetObjectFromConstPool(glue, hirGate, constPool, module, index, type);
}

GateRef CircuitBuilder::GetObjectFromConstPool(GateRef glue, GateRef hirGate, GateRef constPool, GateRef module,
                                               GateRef index, ConstPoolType type)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    Label cacheMiss(env_);
    Label cache(env_);

    auto cacheValue = GetValueFromTaggedArray(constPool, index);
    DEFVAlUE(result, env_, VariableType::JS_ANY(), cacheValue);
    Branch(BoolOr(TaggedIsHole(*result), TaggedIsNullPtr(*result)), &cacheMiss, &cache);
    Bind(&cacheMiss);
    {
        if (type == ConstPoolType::STRING) {
            result = CallRuntime(glue, RTSTUB_ID(GetStringFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index) }, hirGate);
        } else if (type == ConstPoolType::ARRAY_LITERAL) {
            result = CallRuntime(glue, RTSTUB_ID(GetArrayLiteralFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index), module }, hirGate);
        } else if (type == ConstPoolType::OBJECT_LITERAL) {
            result = CallRuntime(glue, RTSTUB_ID(GetObjectLiteralFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index), module }, hirGate);
        } else {
            result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index) }, hirGate);
        }
        Jump(&exit);
    }
    Bind(&cache);
    {
        if (type == ConstPoolType::METHOD) {
            Label isInt(env_);
            Branch(TaggedIsInt(*result), &isInt, &exit);
            Bind(&isInt);
            {
                result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                    { constPool, Int32ToTaggedInt(index) }, hirGate);
                Jump(&exit);
            }
        } else if (type == ConstPoolType::ARRAY_LITERAL) {
            Label isAOTLiteralInfo(env_);
            Branch(IsAOTLiteralInfo(*result), &isAOTLiteralInfo, &exit);
            Bind(&isAOTLiteralInfo);
            {
                result = CallRuntime(glue, RTSTUB_ID(GetArrayLiteralFromCache), Gate::InvalidGateRef,
                    { constPool, Int32ToTaggedInt(index), module }, hirGate);
                Jump(&exit);
            }
        } else if (type == ConstPoolType::OBJECT_LITERAL) {
            Label isAOTLiteralInfo(env_);
            Branch(IsAOTLiteralInfo(*result), &isAOTLiteralInfo, &exit);
            Bind(&isAOTLiteralInfo);
            {
                result = CallRuntime(glue, RTSTUB_ID(GetObjectLiteralFromCache), Gate::InvalidGateRef,
                    { constPool, Int32ToTaggedInt(index), module }, hirGate);
                Jump(&exit);
            }
        } else {
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::ComputeTaggedArraySize(GateRef length)
{
    return PtrAdd(IntPtr(TaggedArray::DATA_OFFSET),
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), length));
}

GateRef CircuitBuilder::CreateArray(ElementsKind kind, uint32_t arraySize)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ArrayMetaDataAccessor accessor(kind, ArrayMetaDataAccessor::Mode::CREATE, arraySize);
    GateRef newGate = GetCircuit()->NewGate(circuit_->CreateArray(accessor.ToValue()), MachineType::I64,
                                            { currentControl, currentDepend }, GateType::TaggedValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::CreateArrayWithBuffer(ElementsKind kind, ArrayMetaDataAccessor::Mode mode,
                                              GateRef constPoolIndex, GateRef elementIndex)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    ArrayMetaDataAccessor accessor(kind, mode);
    GateRef newGate = GetCircuit()->NewGate(circuit_->CreateArrayWithBuffer(accessor.ToValue()),
                                            MachineType::I64,
                                            { currentControl, currentDepend, constPoolIndex,
                                              elementIndex, frameState },
                                            GateType::NJSValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::StartAllocate()
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->StartAllocate(),  MachineType::I64,
                                            { currentDepend }, GateType::NJSValue());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::FinishAllocate()
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->FinishAllocate(),  MachineType::I64,
                                            { currentDepend }, GateType::NJSValue());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::HeapAlloc(GateRef size, GateType type, RegionSpaceFlag flag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->HeapAlloc(flag), MachineType::ANYVALUE,
                                     { currentControl, currentDepend, size }, type);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TryGetHashcodeFromString(GateRef string)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label noRawHashcode(env_);
    Label storeHash(env_);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::INT64(), Int64(-1));
    GateRef hashCode = ZExtInt32ToInt64(Load(VariableType::INT32(), string, IntPtr(EcmaString::HASHCODE_OFFSET)));
    Branch(Int64Equal(hashCode, Int64(0)), &noRawHashcode, &storeHash);
    Bind(&noRawHashcode);
    {
        GateRef length = GetLengthFromString(string);
        Label lengthNotZero(env_);
        Branch(Int32Equal(length, Int32(0)), &storeHash, &exit);
    }
    Bind(&storeHash);
    result = hashCode;
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetHashcodeFromString(GateRef glue, GateRef value)
{
    ASSERT(!GetCircuit()->IsOptimizedJSFunctionFrame());
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label noRawHashcode(env_);
    Label exit(env_);
    DEFVAlUE(hashcode, env_, VariableType::INT32(), Int32(0));
    hashcode = Load(VariableType::INT32(), value, IntPtr(EcmaString::HASHCODE_OFFSET));
    Branch(Int32Equal(*hashcode, Int32(0)), &noRawHashcode, &exit);
    Bind(&noRawHashcode);
    {
        hashcode = GetInt32OfTInt(
            CallRuntime(glue, RTSTUB_ID(ComputeHashcode), Gate::InvalidGateRef, { value }, Circuit::NullGate()));
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

GateRef CircuitBuilder::GetFunctionLexicalEnv(GateRef function)
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

GateRef CircuitBuilder::GetGlobalEnvValue(VariableType type, GateRef env, size_t index)
{
    auto valueIndex = IntPtr(GlobalEnv::HEADER_SIZE + JSTaggedValue::TaggedTypeSize() * index);
    return Load(type, env, valueIndex);
}

GateRef CircuitBuilder::GetCodeAddr(GateRef method)
{
    auto codeAddOffset = IntPtr(Method::CODE_ENTRY_OFFSET);
    return Load(VariableType::NATIVE_POINTER(), method, codeAddOffset);
}

GateRef CircuitBuilder::GetCallBuiltinId(GateRef method)
{
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef extraLiteralInfo = Load(VariableType::INT64(), method, extraLiteralInfoOffset);
    return Int64And(
        Int64LSR(extraLiteralInfo, Int64(MethodLiteral::BuiltinIdBits::START_BIT)),
        Int64((1LU << MethodLiteral::BuiltinIdBits::SIZE) - 1));
}

Environment::Environment(size_t arguments, CircuitBuilder *builder)
    : circuit_(builder->GetCircuit()), circuitBuilder_(builder), arguments_(arguments)
{
    circuitBuilder_->SetEnvironment(this);
    SetCompilationConfig(circuitBuilder_->GetCompilationConfig());
    for (size_t i = 0; i < arguments; i++) {
        arguments_[i] = circuitBuilder_->Arguments(i);
    }
    entry_ = Label(NewLabel(this, circuit_->GetStateRoot()));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto depend_entry = circuit_->GetDependRoot();
    currentLabel_->SetDepend(depend_entry);
}

Environment::Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder)
    : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    SetCompilationConfig(circuitBuilder_->GetCompilationConfig());
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

Environment::Environment(GateRef stateEntry, GateRef dependEntry,
    const std::initializer_list<GateRef>& args, Circuit *circuit, CircuitBuilder *builder)
    : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    SetCompilationConfig(circuitBuilder_->GetCompilationConfig());
    entry_ = Label(NewLabel(this, stateEntry));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    currentLabel_->SetDepend(dependEntry);
    for (auto in : args) {
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

void CircuitBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel,
    uint32_t trueWeight, uint32_t falseWeight)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = Branch(currentControl, condition, trueWeight, falseWeight);
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
        if (!acc.IsNop(result)) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef Label::LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    MachineType machineType = CircuitBuilder::GetMachineTypeFromVariableType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;
        if (machineType == MachineType::NOVALUE) {
            val = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR,
                predeControl_, {}, valueCounts, var->Type());
        } else {
            val = env_->GetBuilder()->Selector(OpCode::VALUE_SELECTOR,
                machineType, predeControl_, {}, valueCounts, var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (machineType == MachineType::NOVALUE) {
            val = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR,
                predeControl_, {}, this->predecessors_.size(), var->Type());
        } else {
            val = env_->GetBuilder()->Selector(OpCode::VALUE_SELECTOR, machineType,
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
        loopDepend_ = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR, predeControl_, {}, 2);
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

    GateRef merge = env_->GetBuilder()->Merge(inGates);
    predeControl_ = merge;
    control_ = merge;
}

void Label::LabelImpl::MergeAllDepend()
{
    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (IsControlCase()) {
            // Add depend_relay to current label
            depend_ = env_->GetBuilder()->DependRelay(predeControl_, depend_);
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
    depend_ = env_->GetBuilder()->Selector(OpCode::DEPEND_SELECTOR,
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
    ASSERT(GateAccessor(env_->GetCircuit()).IsValueSelector(val));
    Label label = env_->GetLabelFromSelector(val);
    size_t idx = 0;
    for (auto pred : label.GetPredecessors()) {
        auto preVal = pred.ReadVariable(this);
        ASSERT(!GateAccessor(env_->GetCircuit()).IsNop(preVal));
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
        if (acc.IsValueSelector(out)) {
            auto result = TryRemoveTrivialPhi(out);
            if (same == out) {
                same = result;
            }
        }
    }
    return same;
}

void CircuitBuilder::ClearConstantCache(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::CONSTANT);
    auto machineType = acc_.GetMachineType(gate);
    auto value = acc_.GetConstantValue(gate);
    auto gateType = acc_.GetGateType(gate);
    GetCircuit()->ClearConstantCache(machineType, value, gateType);
}
}  // namespace panda::ecmascript::kungfu
