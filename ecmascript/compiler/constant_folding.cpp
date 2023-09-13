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

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/lcr_gate_meta_data.h"
#include "ecmascript/compiler/mcr_gate_meta_data.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/constant_folding.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/share_opcodes.h"
#include "ecmascript/compiler/type.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::kungfu {

GateRef ConstantFolding::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::ADD:
            return VisitADD(gate);
        case OpCode::SUB:
            return VisitSUB(gate);
        case OpCode::MUL:
            return VisitMUL(gate);
        case OpCode::SMOD:
            return VisitSMOD(gate);
        case OpCode::UMOD:
            return VisitUMOD(gate);
        case OpCode::ZEXT:
            return VisitZEXT(gate);
        default:
            return Circuit::NullGate();
    }
}

GateRef ConstantFolding::VisitZEXT(GateRef gate)
{
    auto input = acc_.GetValueIn(gate, 0);
    if (acc_.GetMachineType(gate) == acc_.GetMachineType(input)) {
        AddFoldingCount();
        return input;
    }

    GateRef result = Circuit::NullGate();
    if (IsConstant(input) && IsInt32Type(input)) {
        int value = acc_.GetInt32FromConstant(input);
        auto machineType = acc_.GetMachineType(gate);
        switch (machineType) {
            case MachineType::I32:
                result = Int32Constant(value);
                break;
            case MachineType::ARCH:
            case MachineType::I64:
                result = Int64Constant(value);
                break;
            default:
                break;
        }
    }
    return result;
}

GateRef ConstantFolding::VisitSMOD(GateRef gate)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (IsConstant(left) && IsConstant(right)) {
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        if (rvalue != 0) {
            result = Int32Constant(lvalue % rvalue);
        }
    }
    return result;
}

GateRef ConstantFolding::VisitUMOD(GateRef gate)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (IsConstant(left) && IsConstant(right)) {
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        if (rvalue != 0) {
            result = Int32Constant(bit_cast<uint32_t>(lvalue) % bit_cast<uint32_t>(rvalue));
        }
    }
    return result;
}

GateRef ConstantFolding::VisitADD(GateRef gate)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (IsConstant(left) && IsConstant(right)) {
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        result = Int32Constant(lvalue + rvalue);
    }
    return result;
}

GateRef ConstantFolding::VisitSUB(GateRef gate)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (IsConstant(left) && IsConstant(right)) {
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        result = Int32Constant(lvalue - rvalue);
    }
    return result;
}

GateRef ConstantFolding::VisitMUL(GateRef gate)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (IsConstant(left) && IsConstant(right)) {
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        result = Int32Constant(lvalue * rvalue);
    }
    return result;
}

GateRef ConstantFolding::Int32Constant(int32_t val)
{
    AddFoldingCount();
    GateRef result = builder_.Int32(val);
    return result;
}

GateRef ConstantFolding::Int64Constant(size_t val)
{
    AddFoldingCount();
    GateRef result = builder_.Int64(val);
    return result;
}

bool ConstantFolding::IsInt32Type(GateRef gate) const
{
    return acc_.GetMachineType(gate) == MachineType::I32;
}
 
void ConstantFolding::Print() const
{
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After constant folding "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";

        LOG_COMPILER(INFO) << "Folding Count : " << GetFoldingCount();
    }
}
}   // namespace panda::ecmascript::kungfu