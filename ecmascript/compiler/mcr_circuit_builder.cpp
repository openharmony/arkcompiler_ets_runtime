/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/mcr_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/marker_cell.h"

namespace panda::ecmascript::kungfu {

GateRef CircuitBuilder::ObjectTypeCheck(bool isHeapObject, GateRef gate, const std::vector<int>& hclassIndexList,
                                        GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    if (frameState == Circuit::NullGate()) {
        frameState = acc_.FindNearestFrameState(currentDepend);
    }
    ObjectTypeAccessor accessor(isHeapObject);
    std::vector<GateRef> gateList;
    gateList.emplace_back(currentControl);
    gateList.emplace_back(currentDepend);
    gateList.emplace_back(gate);
    for (auto index : hclassIndexList) {
        gateList.emplace_back(Int32(index));
    }
    gateList.emplace_back(frameState);
    GateRef ret = GetCircuit()->NewGate(circuit_->ObjectTypeCheck(hclassIndexList.size() + 1, accessor.ToValue()),
        MachineType::I1, gateList, GateType::NJSValue()); // 1 : gate.
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ObjectTypeCheck(bool isHeapObject, GateRef gate, GateRef hclassIndex,
                                        GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    if (frameState == Circuit::NullGate()) {
        frameState = acc_.FindNearestFrameState(currentDepend);
    }
    ObjectTypeAccessor accessor(isHeapObject);
    GateRef ret = GetCircuit()->NewGate(circuit_->ObjectTypeCheck(2, accessor.ToValue()), // 2 : gate and hclass
        MachineType::I1, {currentControl, currentDepend, gate, hclassIndex, frameState}, GateType::NJSValue());
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
                                        MachineType::I1,
                                        {currentControl, currentDepend, gate, frameState},
                                        GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::EcmaObjectCheck(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->EcmaObjectCheck(),
        MachineType::I1, {currentControl, currentDepend, value, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::HeapObjectIsEcmaObjectCheck(GateRef gate, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->HeapObjectIsEcmaObjectCheck(),
                                        MachineType::I1,
                                        {currentControl, currentDepend, gate, frameState},
                                        GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ProtoChangeMarkerCheck(GateRef gate, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    if (frameState == Circuit::NullGate()) {
        frameState = acc_.FindNearestFrameState(currentDepend);
    }
    GateRef ret = GetCircuit()->NewGate(circuit_->ProtoChangeMarkerCheck(),
                                        MachineType::I1,
                                        {currentControl, currentDepend, gate, frameState},
                                        GateType::NJSValue());
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

GateRef CircuitBuilder::MathHClassConsistencyCheck(GateRef receiver)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef frameState = acc_.FindNearestFrameState(receiver);

    GateRef ret = GetCircuit()->NewGate(circuit_->MathHClassConsistencyCheck(),
        MachineType::I1, {currentControl, currentDepend, receiver, frameState},
        GateType::NJSValue());

    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ElementsKindCheck(GateRef receiver, ElementsKind kind, ArrayMetaDataAccessor::Mode mode)
{
    // If elements kind is hole, no ElementsKindCheck is required.
    if (Elements::IsHole(kind)) {
        return Circuit::NullGate();
    }
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    ArrayMetaDataAccessor accessor(kind, mode);
    GateRef ret = GetCircuit()->NewGate(circuit_->ElementsKindCheck(accessor.ToValue()),
        MachineType::I1, {currentControl, currentDepend, receiver, frameState}, GateType::NJSValue());
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

GateRef CircuitBuilder::InternStringCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->InternStringCheck(),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::EcmaMapCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->EcmaMapCheck(),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::FlattenTreeStringCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->FlattenTreeStringCheck(),
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

GateRef CircuitBuilder::TypedArrayCheck(GateRef gate, ParamType type, TypedArrayMetaDataAccessor::Mode mode,
                                        OnHeapMode onHeap)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    uint64_t value = TypedArrayMetaDataAccessor::ToValue(type, mode, onHeap);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedArrayCheck(value), MachineType::I1,
                                        {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadTypedArrayLength(GateRef gate, ParamType paramType, OnHeapMode onHeap)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t value = TypedArrayMetaDataAccessor::ToValue(paramType,
        TypedArrayMetaDataAccessor::Mode::LOAD_LENGTH, onHeap);
    GateRef ret = GetCircuit()->NewGate(circuit_->LoadTypedArrayLength(value), MachineType::I64,
                                        {currentControl, currentDepend, gate}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringEqual(GateRef x, GateRef y)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->StringEqual(), MachineType::I1,
                                     { currentControl, currentDepend, x, y }, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringAdd(GateRef x, GateRef y, uint32_t stringStatus)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    StringStatusAccessor accessor(stringStatus);
    auto ret = GetCircuit()->NewGate(circuit_->StringAdd(accessor.ToValue()), MachineType::I64,
                                     { currentControl, currentDepend, x, y }, GateType::AnyType());
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

GateRef CircuitBuilder::BuiltinPrototypeHClassCheck(GateRef gate, BuiltinTypeId type,
                                                    ElementsKind kind, bool isPrototypeOfPrototype)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    BuiltinPrototypeHClassAccessor accessor(type, kind, isPrototypeOfPrototype);
    GateRef ret = GetCircuit()->NewGate(circuit_->BuiltinPrototypeHClassCheck(accessor.ToValue()),
        MachineType::I1, {currentControl, currentDepend, gate, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IndexCheck(GateRef gate, GateRef index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->IndexCheck(),
        MachineType::I64, {currentControl, currentDepend, gate, index, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypeOfCheck(GateRef gate, ParamType paramType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypeOfCheck(static_cast<uint64_t>(paramType.Value())),
        MachineType::I64, {currentControl, currentDepend, gate, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedTypeOf(ParamType paramType)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->TypeOf(static_cast<uint64_t>(paramType.Value())),
        MachineType::I64, {currentControl, currentDepend}, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IsMarkerCellValid(GateRef cell)
{
    GateRef bitfield = Load(VariableType::INT32(), cell, IntPtr(MarkerCell::BIT_FIELD_OFFSET));
    return Int32Equal(
        Int32And(Int32LSR(bitfield, Int32(MarkerCell::IsDetectorInvalidBits::START_BIT)),
                 Int32((1LU << MarkerCell::IsDetectorInvalidBits::SIZE) - 1)),
        Int32(0));
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

GateRef CircuitBuilder::ConvertCharToEcmaString(GateRef gate)
{
    return Convert(gate, ValueType::CHAR, ValueType::ECMA_STRING);
}

GateRef CircuitBuilder::ConvertCharToInt32(GateRef gate)
{
    return Convert(gate, ValueType::CHAR, ValueType::INT32);
}

GateRef CircuitBuilder::ConvertCharToDouble(GateRef gate)
{
    return Convert(gate, ValueType::CHAR, ValueType::FLOAT64);
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

GateRef CircuitBuilder::CheckFloat64ConvertToInt32Legally(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::FLOAT64, ValueType::INT32);
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

GateRef CircuitBuilder::CheckHoleIntAndConvertToTaggedInt(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::HOLE_INT, ValueType::TAGGED_INT);
}

GateRef CircuitBuilder::CheckHoleDoubleAndConvertToTaggedDouble(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::HOLE_DOUBLE, ValueType::TAGGED_DOUBLE);
}

GateRef CircuitBuilder::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    return Convert(gate, ValueType::FLOAT64, ValueType::TAGGED_DOUBLE);
}

GateRef CircuitBuilder::ConvertSpecialHoleIntToTagged(GateRef gate)
{
    return Convert(gate, ValueType::HOLE_INT, ValueType::TAGGED_INT);
}

GateRef CircuitBuilder::ConvertSpecialHoleDoubleToTagged(GateRef gate)
{
    return Convert(gate, ValueType::HOLE_DOUBLE, ValueType::TAGGED_DOUBLE);
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

GateRef CircuitBuilder::CheckNullAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_NULL, ValueType::INT32);
}

GateRef CircuitBuilder::CheckTaggedBooleanAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_BOOLEAN, ValueType::INT32);
}

GateRef CircuitBuilder::CheckNullAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_NULL, ValueType::FLOAT64);
}

GateRef CircuitBuilder::CheckTaggedBooleanAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_BOOLEAN, ValueType::FLOAT64);
}

GateRef CircuitBuilder::CheckUndefinedAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::UNDEFINED, ValueType::FLOAT64);
}

GateRef CircuitBuilder::CheckUndefinedAndConvertToBool(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::UNDEFINED, ValueType::BOOL);
}

GateRef CircuitBuilder::CheckNullAndConvertToBool(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::TAGGED_NULL, ValueType::BOOL);
}

GateRef CircuitBuilder::CheckUndefinedAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::UNDEFINED, ValueType::INT32);
}

GateRef CircuitBuilder::CheckHoleIntAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::HOLE_INT, ValueType::INT32);
}

GateRef CircuitBuilder::CheckHoleDoubleAndConvertToInt32(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::HOLE_DOUBLE, ValueType::INT32);
}

GateRef CircuitBuilder::CheckHoleIntAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::HOLE_INT, ValueType::FLOAT64);
}

GateRef CircuitBuilder::CheckHoleDoubleAndConvertToFloat64(GateRef gate)
{
    return CheckAndConvert(gate, ValueType::HOLE_DOUBLE, ValueType::FLOAT64);
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

std::vector<GateRef> CircuitBuilder::ConcatParams(const std::vector<std::vector<GateRef>>& params)
{
    std::vector<GateRef> unionParams;
    for (auto param: params) {
        unionParams.insert(unionParams.end(), param.begin(), param.end());
    }
    return unionParams;
}

GateRef CircuitBuilder::CallTargetCheck(GateRef gate, GateRef function, GateRef id, const char* comment)
{
    return CallTargetCheck(gate, function, id, {}, comment);
}

GateRef CircuitBuilder::CallTargetCheck(GateRef gate, GateRef function, GateRef id, std::vector<GateRef> params,
                                        const char* comment)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef frameState;
    if (Bytecodes::IsCallOp(acc_.GetByteCodeOpcode(gate))) {
        frameState = acc_.GetFrameState(gate);
    } else {
        frameState = acc_.FindNearestFrameState(currentDepend);
    }
    auto params_vec = ConcatParams({{ currentControl, currentDepend, function, id }, params, {frameState}});
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedCallCheck(params.size() + 2),
                                        MachineType::I1,
                                        params_vec,
                                        GateType::NJSValue(),
                                        comment);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedCallOperator(GateRef hirGate, MachineType type, const std::vector<GateRef> &inList,
                                          bool isSideEffect)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    auto numValueIn = inList.size() - 3; // 3: state & depend & frame state
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    if (!isSideEffect) {
        return GetCircuit()->NewGate(circuit_->TypedCallBuiltin(numValueIn, pcOffset), type,
                                     inList.size(), inList.data(), GateType::AnyType());
    }
    return GetCircuit()->NewGate(circuit_->TypedCallBuiltinSideEffect(numValueIn, pcOffset), type,
                                 inList.size(), inList.data(), GateType::AnyType());
}

GateRef CircuitBuilder::TypedNewAllocateThis(GateRef ctor, GateRef hclass, GateRef size, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedNewAllocateThis(),
        MachineType::ANYVALUE, {currentControl, currentDepend, ctor, hclass,
        size, frameState}, GateType::TaggedValue());
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

GateRef CircuitBuilder::RemainderIsNegativeZero(GateRef left, GateRef right)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->RemainderIsNegativeZero(),
                                        MachineType::I1,
                                        {currentControl, currentDepend, left, right, frameState},
                                        GateType::NJSValue());
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

GateRef CircuitBuilder::IsUndefinedOrHoleCheck(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->IsUndefinedOrHoleCheck(),
        MachineType::I1, {currentControl, currentDepend, value, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IsNotUndefinedOrHoleCheck(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->IsNotUndefinedOrHoleCheck(),
        MachineType::I1, {currentControl, currentDepend, value, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IsCallableCheck(GateRef func, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    if (frameState == Circuit::NullGate()) {
        frameState = acc_.FindNearestFrameState(currentDepend);
    }
    GateRef ret = GetCircuit()->NewGate(circuit_->IsCallableCheck(),
                                        MachineType::I1,
                                        {currentControl, currentDepend, func, frameState},
                                        GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IsDataViewCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->IsDataViewCheck(),
                                        MachineType::I1,
                                        {currentControl, currentDepend, gate, frameState},
                                        GateType::NJSValue());
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

GateRef CircuitBuilder::TypedConditionJump(MachineType type, TypedJumpOp jumpOp, uint32_t weight,
    ParamType paramType, const std::vector<GateRef>& inList)
{
    uint64_t value = TypedJumpAccessor::ToValue(paramType, jumpOp, weight);
    return GetCircuit()->NewGate(circuit_->TypedConditionJump(value),
        type, inList.size(), inList.data(), GateType::Empty());
}

GateRef CircuitBuilder::TypeConvert(MachineType type, ParamType typeFrom, GateType typeTo,
                                    const std::vector<GateRef>& inList)
{
    // merge types of valueIns before and after convertion
    uint64_t operandTypes = TypeConvertAccessor::ToValue(typeFrom, typeTo);
    return GetCircuit()->NewGate(circuit_->TypedConvert(operandTypes),
        type, inList.size(), inList.data(), GateType::AnyType());
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

GateRef CircuitBuilder::StoreProperty(GateRef receiver, GateRef propertyLookupResult, GateRef value,
                                      uint32_t receiverHClassIndex)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->StoreProperty(receiverHClassIndex), MachineType::I64,
                                     { currentControl, currentDepend, receiver, propertyLookupResult, value },
                                     GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadArrayLength(GateRef gate, ElementsKind kind, ArrayMetaDataAccessor::Mode mode)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ArrayMetaDataAccessor accessor(kind, mode);
    auto ret = GetCircuit()->NewGate(circuit_->LoadArrayLength(accessor.ToValue()), MachineType::I64,
                                     { currentControl, currentDepend, gate }, GateType::IntType());
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

GateRef CircuitBuilder::LoadMapSize(GateRef string)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->LoadMapSize(), MachineType::I64,
                                     { currentControl, currentDepend, string }, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadConstOffset(VariableType type, GateRef receiver, size_t offset, MemoryAttribute mAttr)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto bits = LoadStoreConstOffsetAccessor::ToValue(offset, mAttr);
    auto ret = GetCircuit()->NewGate(circuit_->LoadConstOffset(bits), type.GetMachineType(),
                                     { currentDepend, receiver }, type.GetGateType());
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::LoadHClassFromConstpool(GateRef constpool, size_t index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->LoadHClassFromConstpool(index), MachineType::I64,
                                     { currentDepend, constpool }, GateType::AnyType());
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StoreConstOffset(VariableType type,
                                         GateRef receiver, size_t offset, GateRef value, MemoryAttribute mAttr)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    if (mAttr.GetBarrier() == MemoryAttribute::Barrier::UNKNOWN_BARRIER && acc_.IsConstant(value)) {
        mAttr.SetBarrier(MemoryAttribute::Barrier::NO_BARRIER);
    }
    auto bits = LoadStoreConstOffsetAccessor::ToValue(offset, mAttr);
    auto ret = GetCircuit()->NewGate(circuit_->StoreConstOffset(bits), type.GetMachineType(),
        { currentDepend, receiver, value }, type.GetGateType());
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TaggedIsHeapObjectOp(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->TaggedIsHeapObject(), MachineType::I1,
                                         { currentControl, currentDepend, value },
                                         GateType::NJSValue());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::IsSpecificObjectType(GateRef obj, JSType type)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->IsSpecificObjectType(static_cast<int32_t>(type)), MachineType::I1,
                                         { currentControl, currentDepend, obj },
                                         GateType::NJSValue());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::IsMarkerCellValidOp(GateRef cell)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->IsMarkerCellValid(), MachineType::I1,
                                         { currentControl, currentDepend, cell },
                                         GateType::NJSValue());
    currentLabel->SetDepend(newGate);
    return newGate;
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
    AppendFrameArgs(args, hirGate);
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
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->TypedFastCall(bitfield, pcOffset, isNoGC), MachineType::I64,
                                          args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
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

GateRef CircuitBuilder::FinishAllocate(GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->FinishAllocate(),  MachineType::I64,
                                            { currentDepend, value }, acc_.GetGateType(value));
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::HeapAlloc(GateRef glue, GateRef size, GateType type, RegionSpaceFlag flag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->HeapAlloc(flag), MachineType::I64,
                                     { currentDepend, glue, size }, type);
    currentLabel->SetDepend(ret);
    return ret;
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

GateRef CircuitBuilder::InsertTypedBinaryop(GateRef left, GateRef right, TypedBinOp op)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t value = TypedBinaryAccessor::ToValue(ParamType::NumberType(), op);
    auto ret = GetCircuit()->NewGate(circuit_->TypedBinaryOp(value),
                                     MachineType::I64,
                                     {currentControl, currentDepend, left, right},
                                     GateType::AnyType());
    acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::InsertRangeCheckPredicate(GateRef left, TypedBinOp cond, GateRef right)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    uint64_t value = TypedBinaryAccessor::ToValue(ParamType::IntType(), cond);
    auto ret = GetCircuit()->NewGate(circuit_->RangeCheckPredicate(value),
                                     MachineType::I32,
                                     {currentControl, currentDepend, left, right, frameState},
                                     GateType::IntType());
    acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::InsertStableArrayCheck(GateRef array, ArrayMetaDataAccessor accessor)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef frameState = acc_.FindNearestFrameState(currentDepend);
    auto ret = GetCircuit()->NewGate(circuit_->StableArrayCheck(accessor.ToValue()),
                                     MachineType::I1,
                                     {currentControl, currentDepend, array, frameState},
                                     GateType::NJSValue());
    acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::InsertTypedArrayCheck(GateRef array, TypedArrayMetaDataAccessor accessor)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef frameState = acc_.FindNearestFrameState(currentDepend);
    auto ret = GetCircuit()->NewGate(circuit_->TypedArrayCheck(accessor.ToValue()),
                                     MachineType::I1,
                                     {currentControl, currentDepend, array, frameState},
                                     GateType::NJSValue());
    acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::InsertLoadArrayLength(GateRef array, GateRef length, bool isTypedArray)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    if (isTypedArray) {
        TypedArrayMetaDataAccessor accessor = acc_.GetTypedArrayMetaDataAccessor(length);
        InsertTypedArrayCheck(array, accessor);
        currentControl = currentLabel->GetControl();
        currentDepend = currentLabel->GetDepend();
        auto ret = GetCircuit()->NewGate(circuit_->LoadTypedArrayLength(accessor.ToValue()),
                                         MachineType::I64,
                                         { currentControl, currentDepend, array },
                                         GateType::IntType());
        acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
        currentLabel->SetControl(ret);
        currentLabel->SetDepend(ret);
        return ret;
    } else {
        ArrayMetaDataAccessor accessor = acc_.GetArrayMetaDataAccessor(length);
        InsertStableArrayCheck(array, accessor);
        currentControl = currentLabel->GetControl();
        currentDepend = currentLabel->GetDepend();
        auto ret = GetCircuit()->NewGate(circuit_->LoadArrayLength(accessor.ToValue()),
                                         MachineType::I64,
                                         { currentControl, currentDepend, array },
                                         GateType::IntType());
        acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
        currentLabel->SetControl(ret);
        currentLabel->SetDepend(ret);
        return ret;
    }
    UNREACHABLE();
    return Circuit::NullGate();
}

GateRef CircuitBuilder::IsIntegerString(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef hash = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_HASHCODE_OFFSET));
    return Int32Equal(
        Int32And(hash, Int32(EcmaString::IS_INTEGER_MASK)),
        Int32(EcmaString::IS_INTEGER_MASK));
}

GateRef CircuitBuilder::GetRawHashFromString(GateRef value)
{
    GateRef hash = Load(VariableType::INT32(), value, IntPtr(EcmaString::MIX_HASHCODE_OFFSET));
    return Int32And(hash, Int32(~EcmaString::IS_INTEGER_MASK));
}

void CircuitBuilder::SetRawHashcode(GateRef glue, GateRef str, GateRef rawHashcode, GateRef isInteger)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label integer(env_);
    Label notInteger(env_);
    Label exit(env_);

    DEFVALUE(hash, env_, VariableType::INT32(), Int32(0));
    BRANCH(isInteger, &integer, &notInteger);
    Bind(&integer);
    {
        hash = Int32Or(rawHashcode, Int32(EcmaString::IS_INTEGER_MASK));
        Jump(&exit);
    }
    Bind(&notInteger);
    {
        hash = Int32And(rawHashcode, Int32(~EcmaString::IS_INTEGER_MASK));
        Jump(&exit);
    }
    Bind(&exit);
    Store(VariableType::INT32(), glue, str, IntPtr(EcmaString::MIX_HASHCODE_OFFSET), *hash);
    SubCfgExit();
    return;
}

GateRef CircuitBuilder::GetLengthFromString(GateRef value)
{
    GateRef len = Load(VariableType::INT32(), value, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32LSR(len, Int32(EcmaString::STRING_LENGTH_SHIFT_COUNT));
}

GateRef CircuitBuilder::Rotl(GateRef word, uint32_t shift)
{
    static constexpr uint32_t MAX_BITS = 32;
    return Int32Or(Int32LSL(word, Int32(shift)), Int32LSR(word, Int32(MAX_BITS - shift)));
}

GateRef CircuitBuilder::CalcHashcodeForInt(GateRef value)
{
    GateRef rawVal = ChangeTaggedPointerToInt64(value);
    GateRef low = TruncInt64ToInt32(rawVal);
    GateRef k1 = Int32Mul(low, Int32(MurmurHash32Const::C1));
    GateRef k2 = Rotl(k1, MurmurHash32Const::MAIN_FIRST_SHIFT);
    GateRef k3 = Int32Mul(k2, Int32(MurmurHash32Const::C2));
    GateRef hash1 = Int32Xor(Int32(DEFAULT_SEED), k3);
    GateRef hash2 = Rotl(hash1, MurmurHash32Const::MAIN_SECOND_SHIFT);
    GateRef hash3 = Int32Add(Int32Mul(hash2, Int32(MurmurHash32Const::MAIN_MULTIPLICATOR)),
        Int32(MurmurHash32Const::MAIN_CONSTANT));

    GateRef high = TruncInt64ToInt32(Int64LSR(rawVal, Int64(32U)));
    GateRef k4 = Int32Mul(high, Int32(MurmurHash32Const::C1));
    GateRef k5 = Rotl(k4, MurmurHash32Const::MAIN_FIRST_SHIFT);
    GateRef k6 = Int32Mul(k5, Int32(MurmurHash32Const::C2));
    GateRef hash4 = Int32Xor(hash3, k6);
    GateRef hash5 = Rotl(hash4, MurmurHash32Const::MAIN_SECOND_SHIFT);
    GateRef hash6 = Int32Add(Int32Mul(hash5, Int32(MurmurHash32Const::MAIN_MULTIPLICATOR)),
        Int32(MurmurHash32Const::MAIN_CONSTANT));

    GateRef hash7 = Int32Xor(hash6, Int32(8U));
    // Finalize
    GateRef hash8 = Int32Xor(hash7, Int32LSR(hash7, Int32(MurmurHash32Const::FINALIZE_FIRST_SHIFT)));
    GateRef hash9 = Int32Mul(hash8, Int32(MurmurHash32Const::FINALIZE_FIRST_MULTIPLICATOR));
    GateRef hash10 = Int32Xor(hash9, Int32LSR(hash9, Int32(MurmurHash32Const::FINALIZE_SECOND_SHIFT)));
    GateRef hash11 = Int32Mul(hash10, Int32(MurmurHash32Const::FINALIZE_SECOND_MULTIPLICATOR));
    GateRef hash12 = Int32Xor(hash11, Int32LSR(hash11, Int32(MurmurHash32Const::FINALIZE_THIRD_SHIFT)));
    return hash12;
}

GateRef CircuitBuilder::GetHashcodeFromString(GateRef glue, GateRef value, GateRef hir)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label noRawHashcode(env_);
    Label exit(env_);
    DEFVALUE(hashcode, env_, VariableType::INT32(), Int32(0));
    hashcode = Load(VariableType::INT32(), value, IntPtr(EcmaString::MIX_HASHCODE_OFFSET));
    BRANCH(Int32Equal(*hashcode, Int32(0)), &noRawHashcode, &exit);
    Bind(&noRawHashcode);
    {
        hashcode = GetInt32OfTInt(
            CallRuntime(glue, RTSTUB_ID(ComputeHashcode), Gate::InvalidGateRef, { value }, hir));
        Store(VariableType::INT32(), glue, value, IntPtr(EcmaString::MIX_HASHCODE_OFFSET), *hashcode);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *hashcode;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TryGetHashcodeFromString(GateRef string)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label noRawHashcode(env_);
    Label storeHash(env_);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::INT64(), Int64(-1));
    GateRef hashCode = ZExtInt32ToInt64(Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_HASHCODE_OFFSET)));
    BRANCH(Int64Equal(hashCode, Int64(0)), &noRawHashcode, &storeHash);
    Bind(&noRawHashcode);
    {
        GateRef length = GetLengthFromString(string);
        Label lengthNotZero(env_);
        BRANCH(Int32Equal(length, Int32(0)), &storeHash, &exit);
    }
    Bind(&storeHash);
    result = hashCode;
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

void CircuitBuilder::CopyChars(GateRef glue, GateRef dst, GateRef source,
    GateRef sourceLength, GateRef charSize, VariableType type)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    DEFVALUE(dstTmp, env_, VariableType::NATIVE_POINTER(), dst);
    DEFVALUE(sourceTmp, env_, VariableType::NATIVE_POINTER(), source);
    DEFVALUE(len, env_, VariableType::INT32(), sourceLength);
    Label loopHead(env_);
    Label loopEnd(env_);
    Label next(env_);
    Label exit(env_);
    Jump(&loopHead);

    LoopBegin(&loopHead);
    {
        BRANCH(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = Load(type, *sourceTmp, IntPtr(0));
            Store(type, glue, *dstTmp, IntPtr(0), i);
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, charSize);
    dstTmp = PtrAdd(*dstTmp, charSize);
    LoopEnd(&loopHead);

    Bind(&exit);
    SubCfgExit();
    return;
}

// source is utf8, dst is utf16
void CircuitBuilder::CopyUtf8AsUtf16(GateRef glue, GateRef dst, GateRef src,

    GateRef sourceLength)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    DEFVALUE(dstTmp, env_, VariableType::NATIVE_POINTER(), dst);
    DEFVALUE(sourceTmp, env_, VariableType::NATIVE_POINTER(), src);
    DEFVALUE(len, env_, VariableType::INT32(), sourceLength);
    Label loopHead(env_);
    Label loopEnd(env_);
    Label next(env_);
    Label exit(env_);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = Load(VariableType::INT8(), *sourceTmp, IntPtr(0));
            Store(VariableType::INT16(), glue, *dstTmp, IntPtr(0), ZExtInt8ToInt16(i));
            Jump(&loopEnd);
        }
    }

    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(uint8_t)));
    dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(uint16_t)));
    LoopEnd(&loopHead);

    Bind(&exit);
    SubCfgExit();
    return;
}

GateRef CircuitBuilder::TaggedPointerToInt64(GateRef x)
{
    return ChangeTaggedPointerToInt64(x);
}

GateRef CircuitBuilder::ComputeTaggedArraySize(GateRef length)
{
    return PtrAdd(IntPtr(TaggedArray::DATA_OFFSET),
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), length));
}

GateRef CircuitBuilder::IsEnumCacheValid(GateRef receiver, GateRef cachedHclass, GateRef kind)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::BOOL(), False());

    Label isSameHclass(env_);
    Label isSimpleEnumCache(env_);
    Label notSimpleEnumCache(env_);
    Label isEnumCache(env_);
    Label isProtoChainEnumCache(env_);
    Label prototypeIsHeapObj(env_);
    Label protoChainNotChanged(env_);

    GateRef hclass = LoadHClass(receiver);
    BRANCH(Int64Equal(hclass, cachedHclass), &isSameHclass, &exit);
    Bind(&isSameHclass);
    BRANCH(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::SIMPLE))),
           &isSimpleEnumCache, &notSimpleEnumCache);
    Bind(&isSimpleEnumCache);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&notSimpleEnumCache);
    BRANCH(Int32Equal(kind, Int32(static_cast<int32_t>(EnumCacheKind::NONE))),
           &exit, &isProtoChainEnumCache);
    Bind(&isProtoChainEnumCache);
    GateRef prototype = GetPrototypeFromHClass(hclass);
    BRANCH(TaggedIsHeapObject(prototype), &prototypeIsHeapObj, &protoChainNotChanged);
    Bind(&prototypeIsHeapObj);
    GateRef protoHClass = LoadHClass(prototype);
    GateRef enumCache = GetEnumCacheFromHClass(protoHClass);
    BRANCH(TaggedIsEnumCache(enumCache), &isEnumCache, &exit);
    Bind(&isEnumCache);
    GateRef protoEnumCacheAll = GetEnumCacheAllFromEnumCache(enumCache);
    BRANCH(TaggedIsNull(protoEnumCacheAll), &exit, &protoChainNotChanged);
    
    Bind(&protoChainNotChanged);
    {
        result = True();
        Jump(&exit);
    }
    
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::NeedCheckProperty(GateRef receiver)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);

    Label loopHead(env_);
    Label loopEnd(env_);
    Label afterLoop(env_);
    Label isJSObject(env_);
    Label hasNoDeleteProperty(env_);

    DEFVALUE(result, env_, VariableType::BOOL(), True());
    DEFVALUE(current, env_, VariableType::JS_ANY(), receiver);

    BRANCH(TaggedIsHeapObject(*current), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    {
        BRANCH(IsJSObject(*current), &isJSObject, &exit);
        Bind(&isJSObject);
        GateRef hclass = LoadHClass(*current);
        BRANCH(HasDeleteProperty(hclass), &exit, &hasNoDeleteProperty);
        Bind(&hasNoDeleteProperty);
        current = GetPrototypeFromHClass(hclass);
        BRANCH(TaggedIsHeapObject(*current), &loopEnd, &afterLoop);
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    {
        result = False();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::ArrayConstructorCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayConstructorCheck(),
        MachineType::I64, {currentControl, currentDepend, gate, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Float32ArrayConstructorCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->Float32ArrayConstructorCheck(),
        MachineType::I64, {currentControl, currentDepend, gate, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ObjectConstructorCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->ObjectConstructorCheck(),
        MachineType::I64, {currentControl, currentDepend, gate, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::BooleanConstructorCheck(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->BooleanConstructorCheck(),
        MachineType::I64, {currentControl, currentDepend, gate, frameState}, GateType::IntType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MonoLoadPropertyOnProto(GateRef receiver, GateRef plrGate, GateRef unsharedConstPool,
                                                size_t hclassIndex)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    auto ret = GetCircuit()->NewGate(circuit_->MonoLoadPropertyOnProto(), MachineType::I64,
                                     { currentControl, currentDepend, receiver, plrGate, Int32(hclassIndex),
                                       unsharedConstPool, frameState },
                                     GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MonoCallGetterOnProto(GateRef gate, GateRef receiver, GateRef plrGate,
                                              GateRef unsharedConstPool, size_t hclassIndex)
{
    uint64_t pcOffset = acc_.TryGetPcOffset(gate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    std::vector<GateRef> args = { currentControl, currentDepend, receiver, plrGate, Int32(hclassIndex),
                                  unsharedConstPool, frameState };
    auto callGate = GetCircuit()->NewGate(circuit_->MonoCallGetterOnProto(pcOffset),
                                          MachineType::I64,
                                          args.size(),
                                          args.data(),
                                          GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::MonoStorePropertyLookUpProto(GateRef receiver, GateRef plrGate, GateRef unsharedConstPool,
                                                     size_t hclassIndex, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    auto ret = GetCircuit()->NewGate(circuit_->MonoStorePropertyLookUpProto(false), MachineType::I64,
        { currentControl, currentDepend, receiver, plrGate, Int32(hclassIndex), unsharedConstPool, value, frameState},
        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MonoStoreProperty(GateRef receiver, GateRef plrGate, GateRef unsharedConstPool,
                                          size_t hclassIndex, GateRef value, GateRef keyIndex, GateRef isProto,
                                          GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->MonoStoreProperty(false), MachineType::I64,
                                     {currentControl, currentDepend, receiver, plrGate, Int32(hclassIndex),
                                      unsharedConstPool, value, keyIndex, isProto, frameState},
                                     GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::TypedCreateObjWithBuffer(std::vector<GateRef> &valueIn)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    std::vector<GateRef> vec { currentControl, currentDepend };
    vec.insert(vec.end(), valueIn.begin(), valueIn.end());
    vec.emplace_back(frameState);
    GateRef ret = GetCircuit()->NewGate(circuit_->TypedCreateObjWithBuffer(valueIn.size()),
        MachineType::I64, vec, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ToNumber(GateRef gate, GateRef value, GateRef glue)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    Label exit(env_);
    Label isNumber(env_);
    Label notNumber(env_);
    DEFVALUE(result, env_, VariableType::JS_ANY(), Hole());
    BRANCH(TaggedIsNumber(value), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = value;
        Jump(&exit);
    }
    Bind(&notNumber);
    {
        result = CallRuntime(glue, RTSTUB_ID(ToNumber), Gate::InvalidGateRef, { value }, gate);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::StringToNumber(GateRef gate, GateRef value, GateRef radix, GateRef glue)
{
    return CallNGCRuntime(glue, RTSTUB_ID(StringToNumber), Gate::InvalidGateRef, { value, radix }, gate);
}

GateRef CircuitBuilder::BuildControlDependOp(const GateMetaData* op, std::vector<GateRef> args,
                                             std::vector<GateRef> frameStates)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(op, MachineType::I64,
            ConcatParams({std::vector{ currentControl, currentDepend}, args, frameStates}), GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringFromSingleCharCode(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->StringFromSingleCharCode(), MachineType::I64,
            { currentControl, currentDepend, gate }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringCharCodeAt(GateRef thisValue, GateRef posTag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->StringCharCodeAt(), MachineType::I64,
            { currentControl, currentDepend, thisValue, posTag }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringSubstring(std::vector<GateRef>& args)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    std::vector<GateRef> inList {currentControl, currentDepend};
    inList.insert(inList.end(), args.begin(), args.end());
    GateRef ret = GetCircuit()->NewGate(
        circuit_->StringSubstring(args.size()), MachineType::I64, inList, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringSubStr(GateRef thisValue, GateRef intStart, GateRef lengthTag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->StringSubStr(), MachineType::I64,
            { currentControl, currentDepend, thisValue, intStart, lengthTag }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::StringSlice(std::vector<GateRef>& args)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    std::vector<GateRef> inList {currentControl, currentDepend};
    inList.insert(inList.end(), args.begin(), args.end());
    GateRef ret =
        GetCircuit()->NewGate(circuit_->StringSlice(args.size()), MachineType::I64, inList, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayBufferIsView(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(
        circuit_->ArrayBufferIsView(), MachineType::I64, {currentControl, currentDepend, gate}, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::DataViewGet(
    GateRef thisobj, GateRef index, GateRef dataViewCallID, GateRef isLittleEndian, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(
        circuit_->DataViewGet(),
        MachineType::I64,
        {currentControl, currentDepend, thisobj, index, dataViewCallID, isLittleEndian, frameState},
        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::DataViewSet(
    GateRef thisobj, GateRef index, GateRef value, GateRef dataViewCallID, GateRef isLittleEndian, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(
        circuit_->DataViewSet(),
        MachineType::I64,
        {currentControl, currentDepend, thisobj, index, value, dataViewCallID, isLittleEndian, frameState},
        GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayIncludesIndexOf(
    GateRef elements, GateRef target, GateRef fromIndex, GateRef len, GateRef callID, GateRef arrayKind)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(
        circuit_->ArrayIncludesIndexOf(),
        MachineType::I64,
        {currentControl, currentDepend, elements, target, fromIndex, len, callID, arrayKind},
        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayIteratorBuiltin(GateRef thisArray, GateRef callID)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->ArrayIteratorBuiltin(),
                                     MachineType::I64,
                                     {currentControl, currentDepend, thisArray, callID},
                                     GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayForEach(GateRef thisValue, GateRef callBackFn, GateRef usingThis, uint32_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayForEach(static_cast<uint64_t>(pcOffset)),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn, usingThis},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArraySort(GateRef thisValue, GateRef callBackFn)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArraySort(),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayFilter(
    GateRef thisValue, GateRef callBackFn, GateRef usingThis, GateRef frameState, uint32_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayFilter(static_cast<uint64_t>(pcOffset)),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn, usingThis, frameState},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayMap(
    GateRef thisValue, GateRef callBackFn, GateRef usingThis, GateRef frameState, uint32_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayMap(static_cast<uint64_t>(pcOffset)),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn, usingThis, frameState},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArraySome(GateRef thisValue, GateRef callBackFn, GateRef usingThis, uint32_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArraySome(static_cast<uint64_t>(pcOffset)),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn, usingThis},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayEvery(GateRef thisValue, GateRef callBackFn, GateRef usingThis, uint32_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayEvery(static_cast<uint64_t>(pcOffset)),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn, usingThis},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayPop(GateRef thisValue, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayPop(),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, frameState},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayPush(GateRef thisValue, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayPush(),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, value},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArraySlice(GateRef thisValue, GateRef startIndex, GateRef endIndex, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArraySlice(),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, startIndex, endIndex, frameState},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::ArrayFindOrFindIndex(
    GateRef thisValue, GateRef callBackFn, GateRef usingThis, GateRef callIDRef, uint32_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret = GetCircuit()->NewGate(circuit_->ArrayFindOrFindIndex(static_cast<uint64_t>(pcOffset)),
                                        MachineType::I64,
                                        {currentControl, currentDepend, thisValue, callBackFn, usingThis, callIDRef},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NumberIsFinite(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->NumberIsFinite(), MachineType::I64,
            { currentControl, currentDepend, gate }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NumberIsInteger(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->NumberIsInteger(), MachineType::I64,
            { currentControl, currentDepend, gate }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NumberIsNaN(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->NumberIsNaN(), MachineType::I64,
            { currentControl, currentDepend, gate }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NumberParseFloat(GateRef gate, GateRef frameState)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->NumberParseFloat(), MachineType::I64,
            { currentControl, currentDepend, gate, frameState }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NumberParseInt(GateRef gate, GateRef radix)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->NumberParseInt(), MachineType::I64,
            { currentControl, currentDepend, gate, radix }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::NumberIsSafeInteger(GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(circuit_->NumberIsSafeInteger(), MachineType::I64,
            { currentControl, currentDepend, gate }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::BuildBigIntAsIntN(const GateMetaData* op, std::vector<GateRef> &&args)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(op, MachineType::I64,
            ConcatParams({std::vector{currentControl, currentDepend}, args}), GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::BuildTypedArrayIterator(GateRef gate, const GateMetaData* op)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef ret =
        GetCircuit()->NewGate(op, MachineType::I64,
            { currentControl, currentDepend, gate }, GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::IsASCIICharacter(GateRef gate)
{
    return Int32UnsignedLessThan(Int32Sub(gate, Int32(1)), Int32(base::utf_helper::UTF8_1B_MAX));
}

GateRef CircuitBuilder::MigrateFromRawValueToHeapValues(GateRef object, GateRef needCOW, GateRef isIntKind)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->MigrateFromRawValueToHeapValues(),
                                     MachineType::I64,
                                     { currentControl, currentDepend, object, needCOW, isIntKind },
                                     GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MigrateFromHeapValueToRawValue(GateRef object, GateRef needCOW, GateRef isIntKind)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->MigrateFromHeapValueToRawValue(),
                                     MachineType::I64,
                                     { currentControl, currentDepend, object, needCOW, isIntKind },
                                     GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MigrateFromHoleIntToHoleNumber(GateRef object)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->MigrateFromHoleIntToHoleNumber(),
                                     MachineType::I64,
                                     { currentControl, currentDepend, object },
                                     GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MigrateFromHoleNumberToHoleInt(GateRef object)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->MigrateFromHoleNumberToHoleInt(),
                                     MachineType::I64,
                                     { currentControl, currentDepend, object },
                                     GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::CallTargetIsCompiledCheck(GateRef func, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.GetFrameState(gate);
    GateRef ret = GetCircuit()->NewGate(circuit_->CallTargetIsCompiledCheck(), MachineType::I1,
                                        {currentControl, currentDepend, func, frameState}, GateType::NJSValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}
}
