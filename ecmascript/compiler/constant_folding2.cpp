/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/constant_folding2.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/constant.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "macros.h"
#include "utils/bit_utils.h"
#include <cmath>
#include <cstdint>

namespace panda::ecmascript::kungfu {

GateRef ConstantFolding::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    auto found = opHandlers_.find(op);
    if (found != opHandlers_.end()) {
        return (this->*(found->second))(gate);
    } else {
        return Circuit::NullGate();
    }
}

void ConstantFolding::Initialize()
{
    InitializeOpHandlers();
}

void ConstantFolding::InitializeOpHandlers()
{
    opHandlers_ = {
        { OpCode::ADD, &ConstantFolding::VisitADD },
        { OpCode::SUB, &ConstantFolding::VisitSUB },
        { OpCode::MUL, &ConstantFolding::VisitMUL },
        { OpCode::SDIV, &ConstantFolding::VisitSDIV },
        { OpCode::SMOD, &ConstantFolding::VisitSMOD },
        { OpCode::UMOD, &ConstantFolding::VisitUMOD },
        { OpCode::FDIV, &ConstantFolding::VisitFDIV },
        { OpCode::AND, &ConstantFolding::VisitAND },
        { OpCode::XOR, &ConstantFolding::VisitXOR },
        { OpCode::OR, &ConstantFolding::VisitOR },
        { OpCode::MIN, &ConstantFolding::VisitMIN },
        { OpCode::MAX, &ConstantFolding::VisitMAX },
        { OpCode::ICMP, &ConstantFolding::VisitICMP },
        { OpCode::ZEXT, &ConstantFolding::VisitZEXT },
        { OpCode::REV, &ConstantFolding::VisitREV },
        { OpCode::ABS, &ConstantFolding::VisitABS },
        { OpCode::SQRT, &ConstantFolding::VisitSQRT },
        { OpCode::CEIL, &ConstantFolding::VisitCEIL },
        { OpCode::FLOOR, &ConstantFolding::VisitFLOOR }
    };
}

GateRef ConstantFolding::BinaryCalculate(GateRef gate, std::function<Constant(Constant, Constant)> op)
{
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    if (acc_.IsConstant(left) && acc_.IsConstant(right)) {
        Constant lvalue = GetConstant(acc_, left);
        Constant rvalue = GetConstant(acc_, right);
        if (!lvalue.IsValid() || !rvalue.IsValid()) {
            return Circuit::NullGate();
        }
        if (acc_.GetMachineType(left) == acc_.GetMachineType(right)) {
            auto res = op(lvalue, rvalue);
            if (!res.IsValid()) {
                return Circuit::NullGate();
            }
            return BuildConstant(gate, res);
        }
    }
    return Circuit::NullGate();
}

GateRef ConstantFolding::UnaryCalculate(GateRef gate, std::function<Constant(Constant)> op)
{
    auto valueIn = acc_.GetValueIn(gate, 0);
    if (acc_.IsConstant(valueIn)) {
        Constant val = GetConstant(acc_, valueIn);
        if (!val.IsValid()) {
            return Circuit::NullGate();
        }
        auto res = op(val);
        if (!res.IsValid()) {
            return Circuit::NullGate();
        }
        if (IsF64Type(gate) && std::isnan(val.GetDouble())) {
            return Circuit::NullGate();
        }
        return BuildConstant(gate, res);
    }
    return Circuit::NullGate();
}

GateRef ConstantFolding::VisitICMP(GateRef gate)
{
    return BinaryCalculate(gate, [this, gate](Constant lvalue, Constant rvalue) {
        switch (acc_.GetICmpCondition(gate)) {
            case ICmpCondition::NE:
                return lvalue != rvalue;
            case ICmpCondition::EQ:
                return lvalue == rvalue;
            case ICmpCondition::SGE:
                return rvalue <= lvalue;
            case ICmpCondition::SGT:
                return rvalue < lvalue;
            case ICmpCondition::SLE:
                return lvalue <= rvalue;
            case ICmpCondition::SLT:
                return lvalue < rvalue;
            case ICmpCondition::UGE:
                return rvalue.ULE(lvalue);
            case ICmpCondition::UGT:
                return rvalue.ULT(lvalue);
            case ICmpCondition::ULE:
                return lvalue.ULE(rvalue);
            case ICmpCondition::ULT:
                return lvalue.ULT(rvalue);
            default:
                return Constant();
        }
    });
}

GateRef ConstantFolding::VisitADD(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue + rvalue;
    });
}

GateRef ConstantFolding::VisitSUB(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue - rvalue;
    });
}

GateRef ConstantFolding::VisitMUL(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue * rvalue;
    });
}

GateRef ConstantFolding::VisitFDIV(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue.CheckDiv(rvalue);
    });
}

GateRef ConstantFolding::VisitSDIV(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue.CheckDiv(rvalue);
    });
}

GateRef ConstantFolding::VisitAND(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue & rvalue;
    });
}

GateRef ConstantFolding::VisitOR(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue | rvalue;
    });
}

GateRef ConstantFolding::VisitXOR(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        return lvalue ^ rvalue;
    });
}

GateRef ConstantFolding::VisitMAX(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        auto lval = lvalue.GetDouble();
        auto rval = rvalue.GetDouble();
        return lval >= rval ? lvalue : rvalue;
    });
}

GateRef ConstantFolding::VisitMIN(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        auto lval = lvalue.GetDouble();
        auto rval = rvalue.GetDouble();
        return lval >= rval ? rvalue : lvalue;
    });
}

GateRef ConstantFolding::VisitABS(GateRef gate)
{
    return UnaryCalculate(gate, [this](Constant value) {
        if (value.IsInt()) {
            return Constant(std::abs(value.GetInt()));
        }
        return Constant(std::abs(value.GetDouble()));
    });
}

GateRef ConstantFolding::VisitSQRT(GateRef gate)
{
    return UnaryCalculate(gate, [this](Constant value) {
        ASSERT(!value.IsInt());
        return Constant(std::sqrt(value.GetDouble()));
    });
}

GateRef ConstantFolding::VisitCEIL(GateRef gate)
{
    return UnaryCalculate(gate, [this](Constant value) {
        ASSERT(!value.IsInt());
        return Constant(std::ceil(value.GetDouble()));
    });
}

GateRef ConstantFolding::VisitFLOOR(GateRef gate)
{
    return UnaryCalculate(gate, [this](Constant value) {
        ASSERT(!value.IsInt());
        return Constant(std::floor(value.GetDouble()));
    });
}

GateRef ConstantFolding::VisitSMOD(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        if (lvalue.IsInt() && rvalue.IsInt() && rvalue.GetInt() != 0) {
            return lvalue.CheckMod(rvalue);
        }
        return Constant();
    });
}

GateRef ConstantFolding::VisitUMOD(GateRef gate)
{
    return BinaryCalculate(gate, [this](Constant lvalue, Constant rvalue) {
        auto rval = rvalue.GetInt();
        if (lvalue.IsInt() && rval != 0) {
            if (lvalue.IsInt32()) {
                return Constant(bit_cast<uint32_t>(lvalue.GetInt32()) % rval);
            } else if (lvalue.IsInt64()) {
                return Constant(int64_t(bit_cast<uint64_t>(lvalue.GetInt64()) % rval));
            }
        }
        return Constant();
    });
}

GateRef ConstantFolding::VisitREV(GateRef gate)
{
    return UnaryCalculate(gate, [this](Constant value) {
        return ~value;
    });
}

GateRef ConstantFolding::VisitZEXT(GateRef gate)
{
    return UnaryCalculate(gate, [this](Constant value) {
        if (value.IsBool()) {
            uint64_t raw = value.GetBool();
            return Constant(bit_cast<int64_t>(raw));
        } else if (value.IsInt32()) {
            uint64_t raw = bit_cast<uint32_t>(value.GetInt32());
            return Constant(bit_cast<int64_t>(raw));
        } else if (value.IsInt64()) {
            return value;
        }
        return Constant();
    });
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
