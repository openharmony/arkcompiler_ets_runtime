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

#include "ecmascript/compiler/constant_folding.h"

namespace panda::ecmascript::kungfu {

GateRef ConstantFolding::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        BINARY_OP_LIST(GEN_SWITCH_CASE)
        UNARY_OP_LIST(GEN_SWITCH_CASE)
        default:
            return Circuit::NullGate();
    }
}

GateRef ConstantFolding::BinaryInt32Calculate(GateRef gate, std::function<GateRef(int, int)> op)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (acc_.IsConstant(left) && acc_.IsConstant(right)) {
        AddFoldingCount();
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        result = op(lvalue, rvalue);
    }
    return result;
}

GateRef ConstantFolding::UnaryInt32Calculate(GateRef gate, std::function<GateRef(int)> op)
{
    auto valueIn = acc_.GetValueIn(gate, 0);
    if (!IsInt32Type(gate) || !IsInt32Type(valueIn)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (acc_.IsConstant(valueIn)) {
        AddFoldingCount();
        int value = acc_.GetInt32FromConstant(valueIn);
        result = op(value);
    }
    return result;
}

GateRef ConstantFolding::BinaryF64Calculate(GateRef gate, std::function<GateRef(double, double)> op)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsF64Type(gate) || !IsF64Type(left) || !IsF64Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (acc_.IsConstant(left) && acc_.IsConstant(right)) {
        AddFoldingCount();
        double lvalue = acc_.GetFloat64FromConstant(left);
        double rvalue = acc_.GetFloat64FromConstant(right);
        if (!std::isnan(lvalue) && !std::isnan(rvalue)) {
            result = op(lvalue, rvalue);
        }
    }
    return result;
}

GateRef ConstantFolding::UnaryF64Calculate(GateRef gate, std::function<GateRef(double)> op)
{
    auto valueIn = acc_.GetValueIn(gate, 0);
    if (!IsF64Type(gate) || !IsF64Type(valueIn)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (acc_.IsConstant(valueIn)) {
        AddFoldingCount();
        double value = acc_.GetFloat64FromConstant(valueIn);
        if (!std::isnan(value)) {
            result = op(value);
        }
    }
    return result;
}

GateRef ConstantFolding::VisitADD(GateRef gate)
{
    return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) {
        if (lvalue > 0 && rvalue > INT_MAX - lvalue) {
            return Circuit::NullGate();
        }
        if (lvalue < 0 && rvalue < INT_MIN - lvalue) {
            return Circuit::NullGate();
        }
        return Int32Constant(lvalue + rvalue);
    });
}

GateRef ConstantFolding::VisitSUB(GateRef gate)
{
    return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) {
        if (rvalue > 0 && lvalue < INT_MIN + rvalue) {
            return Circuit::NullGate();
        }
        if (rvalue < 0 && lvalue > INT_MAX + rvalue) {
            return Circuit::NullGate();
        }
        return Int32Constant(lvalue - rvalue);
    });
}

GateRef ConstantFolding::VisitMUL(GateRef gate)
{
    return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) {
        if (lvalue > 0 && rvalue > 0 && lvalue > INT_MAX / rvalue) {
            return Circuit::NullGate();
        }
        if (lvalue < 0 && rvalue < 0 && lvalue < INT_MAX / rvalue) {
            return Circuit::NullGate();
        }
        if (lvalue > 0 && rvalue < 0 && rvalue < INT_MIN / lvalue) {
            return Circuit::NullGate();
        }
        if (lvalue < 0 && rvalue > 0 && lvalue < INT_MIN / rvalue) {
            return Circuit::NullGate();
        }
        return Int32Constant(lvalue * rvalue);
    });
}

GateRef ConstantFolding::VisitAND(GateRef gate)
{
    return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) { return Int32Constant(lvalue & rvalue); });
}

GateRef ConstantFolding::VisitOR(GateRef gate)
{
    return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) { return Int32Constant(lvalue | rvalue); });
}

GateRef ConstantFolding::VisitXOR(GateRef gate)
{
    return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) { return Int32Constant(lvalue ^ rvalue); });
}

GateRef ConstantFolding::VisitMAX(GateRef gate)
{
    if (IsInt32Type(gate)) {
        return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) {
            return Int32Constant(std::max(lvalue, rvalue));
        });
    }
    return BinaryF64Calculate(gate, [this](double lvalue, double rvalue) {
        return DoubleConstant(std::max(lvalue, rvalue));
    });
}

GateRef ConstantFolding::VisitMIN(GateRef gate)
{
    if (IsInt32Type(gate)) {
        return BinaryInt32Calculate(gate, [this](int lvalue, int rvalue) {
            return Int32Constant(std::min(lvalue, rvalue));
        });
    }
    return BinaryF64Calculate(gate, [this](double lvalue, double rvalue) {
        return DoubleConstant(std::min(lvalue, rvalue));
    });
}

GateRef ConstantFolding::VisitABS(GateRef gate)
{
    return UnaryInt32Calculate(gate, [this](int value) { return Int32Constant(std::abs(value)); });
}

GateRef ConstantFolding::VisitSQRT(GateRef gate)
{
    return UnaryF64Calculate(gate, [this](double value) { return DoubleConstant(std::sqrt(value)); });
}

GateRef ConstantFolding::VisitCEIL(GateRef gate)
{
    return UnaryF64Calculate(gate, [this](double value) { return DoubleConstant(std::ceil(value)); });
}

GateRef ConstantFolding::VisitFLOOR(GateRef gate)
{
    return UnaryF64Calculate(gate, [this](double value) { return DoubleConstant(std::floor(value)); });
}

GateRef ConstantFolding::VisitSMOD(GateRef gate)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (!IsInt32Type(gate) || !IsInt32Type(left) || !IsInt32Type(right)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    if (acc_.IsConstant(left) && acc_.IsConstant(right)) {
        AddFoldingCount();
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
    if (acc_.IsConstant(left) && acc_.IsConstant(right)) {
        AddFoldingCount();
        int lvalue = acc_.GetInt32FromConstant(left);
        int rvalue = acc_.GetInt32FromConstant(right);
        if (rvalue != 0) {
            result = Int32Constant(bit_cast<uint32_t>(lvalue) % rvalue);
        }
    }
    return result;
}

GateRef ConstantFolding::VisitREV(GateRef gate)
{
    auto input = acc_.GetValueIn(gate);
    if (!IsBoolType(gate) || !acc_.IsConstant(input) || !IsBoolType(input)) {
        return Circuit::NullGate();
    }

    GateRef result = Circuit::NullGate();
    uint64_t rawValue = acc_.GetConstantValue(input);
    if (acc_.GetGateType(gate).IsNJSValueType()) {
        AddFoldingCount();
        result = BoolConstant(rawValue == 0);
    }
    return result;
}

GateRef ConstantFolding::VisitZEXT(GateRef gate)
{
    auto input = acc_.GetValueIn(gate, 0);
    if (acc_.GetMachineType(gate) == acc_.GetMachineType(input)) {
        AddFoldingCount();
        return input;
    }

    GateRef result = Circuit::NullGate();
    if (acc_.IsConstant(input) && IsInt32Type(input)) {
        AddFoldingCount();
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