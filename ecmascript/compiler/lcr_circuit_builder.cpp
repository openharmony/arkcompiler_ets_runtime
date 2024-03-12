/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/lcr_circuit_builder.h"
#include "ecmascript/compiler/common_stubs.h"

namespace panda::ecmascript::kungfu {

GateRef CircuitBuilder::BinaryCmp(const GateMetaData* meta, GateRef left, GateRef right, const char* comment)
{
    return GetCircuit()->NewGate(meta, MachineType::I1, { left, right }, GateType::NJSValue(), comment);
}

MachineType CircuitBuilder::GetMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

GateRef CircuitBuilder::Sqrt(GateRef param)
{
    return GetCircuit()->NewGate(circuit_->Sqrt(), MachineType::F64, {param}, GateType::DoubleType());
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

GateRef CircuitBuilder::ReadSp()
{
    return circuit_->NewGate(circuit_->ReadSp(), MachineType::I64, GateType::NJSValue());
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

GateRef CircuitBuilder::BinaryArithmetic(const GateMetaData* meta, MachineType machineType,
                                         GateRef left, GateRef right, GateType gateType, const char* comment)
{
    auto circuit = GetCircuit();
    if (gateType == GateType::Empty()) {
        gateType = acc_.GetGateType(left);
    }
    return circuit->NewGate(meta, machineType, { left, right }, gateType, comment);
}

GateRef CircuitBuilder::Alloca(size_t size)
{
    return GetCircuit()->NewGate(circuit_->Alloca(size), MachineType::ARCH, GateType::NJSValue());
}

// memory
void CircuitBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value,
    MemoryOrder order)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    auto bit = LoadStoreAccessor::ToValue(order);
    GateRef result = GetCircuit()->NewGate(circuit_->Store(bit),
        MachineType::NOVALUE, { depend, glue, base, offset, value }, type.GetGateType());
    label->SetDepend(result);
}

void CircuitBuilder::StoreWithoutBarrier(VariableType type, GateRef addr, GateRef value, MemoryOrder order)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    auto bit = LoadStoreAccessor::ToValue(order);
    GateRef result = GetCircuit()->NewGate(circuit_->StoreWithoutBarrier(bit),
        MachineType::NOVALUE, { depend, addr, value }, type.GetGateType());
    label->SetDepend(result);
}

// memory
GateRef CircuitBuilder::Load(VariableType type, GateRef base, GateRef offset, MemoryOrder order)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef val = PtrAdd(base, offset);
    auto bits = LoadStoreAccessor::ToValue(order);
    GateRef result = GetCircuit()->NewGate(GetCircuit()->Load(bits), type.GetMachineType(),
                                           { depend, val }, type.GetGateType());
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::Load(VariableType type, GateRef base, GateRef offset, GateRef depend,
    MemoryOrder order)
{
    GateRef val = PtrAdd(base, offset);
    auto bits = LoadStoreAccessor::ToValue(order);
    GateRef result = GetCircuit()->NewGate(GetCircuit()->Load(bits), type.GetMachineType(),
                                           { depend, val }, type.GetGateType());
    return result;
}

GateRef CircuitBuilder::Load(VariableType type, GateRef addr, MemoryOrder order)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    auto bits = LoadStoreAccessor::ToValue(order);
    GateRef result = GetCircuit()->NewGate(GetCircuit()->Load(bits), type.GetMachineType(),
                                           { depend, addr }, type.GetGateType());
    label->SetDepend(result);
    return result;
}
}
