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

#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/number_speculative_lowering.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_native_pointer.h"

namespace panda::ecmascript::kungfu {

void NumberSpeculativeLowering::Run()
{
    std::vector<GateRef> gateList;
    acc_.GetAllGates(gateList);
    for (auto gate : gateList) {
        VisitGate(gate);
    }
}

void NumberSpeculativeLowering::VisitGate(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::TYPED_BINARY_OP: {
            ASSERT(acc_.GetLeftType(gate).IsNumberType() && acc_.GetRightType(gate).IsNumberType());
            VisitNumberBinaryOp(gate);
            break;
        }
        case OpCode::VALUE_SELECTOR: {
            VisitPhi(gate);
            break;
        }
        case OpCode::CONSTANT: {
            VisitConstant(gate);
            break;
        }
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitNumberBinaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedBinOp Op = acc_.GetTypedBinaryOp(gate);
    switch (Op) {
        case TypedBinOp::TYPED_ADD: {
            VisitNumberCalculate<TypedBinOp::TYPED_ADD>(gate);
            break;
        }
        case TypedBinOp::TYPED_SUB: {
            VisitNumberCalculate<TypedBinOp::TYPED_SUB>(gate);
            break;
        }
        case TypedBinOp::TYPED_MUL: {
            VisitNumberCalculate<TypedBinOp::TYPED_MUL>(gate);
            break;
        }
        case TypedBinOp::TYPED_LESS: {
            VisitNumberCompare<TypedBinOp::TYPED_LESS>(gate);
            break;
        }
        case TypedBinOp::TYPED_LESSEQ: {
            VisitNumberCompare<TypedBinOp::TYPED_LESSEQ>(gate);
            break;
        }
        case TypedBinOp::TYPED_GREATER: {
            VisitNumberCompare<TypedBinOp::TYPED_GREATER>(gate);
            break;
        }
        case TypedBinOp::TYPED_GREATEREQ: {
            VisitNumberCompare<TypedBinOp::TYPED_GREATEREQ>(gate);
            break;
        }
        case TypedBinOp::TYPED_EQ: {
            VisitNumberCompare<TypedBinOp::TYPED_EQ>(gate);
            break;
        }
        case TypedBinOp::TYPED_NOTEQ: {
            VisitNumberCompare<TypedBinOp::TYPED_NOTEQ>(gate);
            break;
        }
        default:
            break;
    }
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitNumberCalculate(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType gateType = acc_.GetGateType(gate);
    GateRef result = Circuit::NullGate();
    if (gateType.IsIntType()) {
        result = CalculateInts<Op>(left, right);    // int op int
        acc_.SetMachineType(gate, MachineType::I32);
    } else {
        result = CalculateDoubles<Op>(left, right); // float op float
        acc_.SetMachineType(gate, MachineType::F64);
    }
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitNumberCompare(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateType leftType = acc_.GetLeftType(gate);
    GateType rightType = acc_.GetRightType(gate);
    GateRef result = Circuit::NullGate();
    if (leftType.IsIntType() && rightType.IsIntType()) {
        result = CompareInts<Op>(left, right);  // int op int
    } else {
        result = CompareDoubles<Op>(left, right);   // float op float
    }
    acc_.SetMachineType(gate, MachineType::I1);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitConstant(GateRef gate)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT32: {
            int value = acc_.GetInt32FromConstant(gate);
            acc_.UpdateAllUses(gate, builder_.Int32(value));
            break;
        }
        case TypeInfo::FLOAT64: {
            double value = acc_.GetFloat64FromConstant(gate);
            acc_.UpdateAllUses(gate, builder_.Double(value));
            break;
        }
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitPhi(GateRef gate)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT1: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::I1);
            break;
        }
        case TypeInfo::INT32: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::I32);
            break;
        }
        case TypeInfo::FLOAT64: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::F64);
            break;
        }
        default:
            break;
    }
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::CalculateInts(GateRef left, GateRef right)
{
    GateRef res = Circuit::NullGate();
    left = builder_.SExtInt32ToInt64(left);
    right = builder_.SExtInt32ToInt64(right);
    switch (Op) {
        case TypedBinOp::TYPED_ADD:
            res = builder_.Int64Add(left, right);
            break;
        case TypedBinOp::TYPED_SUB:
            res = builder_.Int64Sub(left, right);
            break;
        case TypedBinOp::TYPED_MUL:
            res = builder_.Int64Mul(left, right);
            break;
        default:
            break;
    }
    // DeoptCheckForOverFlow
    GateRef max = builder_.Int64(INT32_MAX);
    GateRef min = builder_.Int64(INT32_MIN);
    GateRef notOverflow = builder_.Int64LessThanOrEqual(res, max);
    GateRef notUnderflow = builder_.Int64GreaterThanOrEqual(res, min);
    GateRef condition = builder_.BoolAnd(notOverflow, notUnderflow);
    builder_.DeoptCheck(condition, acc_.GetFrameState(builder_.GetDepend()), DeoptType::NOTINT);
    GateRef result = builder_.TruncInt64ToInt32(res);
    return result;
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::CalculateDoubles(GateRef left, GateRef right)
{
    GateRef res = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_ADD:
            res = builder_.DoubleAdd(left, right);
            break;
        case TypedBinOp::TYPED_SUB:
            res = builder_.DoubleSub(left, right);
            break;
        case TypedBinOp::TYPED_MUL:
            res = builder_.DoubleMul(left, right);
            break;
        default:
            break;
    }
    return res;
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::CompareInts(GateRef left, GateRef right)
{
    GateRef condition = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_LESS:
            condition = builder_.Int32LessThan(left, right);
            break;
        case TypedBinOp::TYPED_LESSEQ:
            condition = builder_.Int32LessThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_GREATER:
            condition = builder_.Int32GreaterThan(left, right);
            break;
        case TypedBinOp::TYPED_GREATEREQ:
            condition = builder_.Int32GreaterThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_EQ:
            condition = builder_.Int32Equal(left, right);
            break;
        case TypedBinOp::TYPED_NOTEQ:
            condition = builder_.Int32NotEqual(left, right);
            break;
        default:
            break;
    }
    return condition;
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::CompareDoubles(GateRef left, GateRef right)
{
    GateRef condition = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_LESS:
            condition = builder_.DoubleLessThan(left, right);
            break;
        case TypedBinOp::TYPED_LESSEQ:
            condition = builder_.DoubleLessThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_GREATER:
            condition = builder_.DoubleGreaterThan(left, right);
            break;
        case TypedBinOp::TYPED_GREATEREQ:
            condition = builder_.DoubleGreaterThanOrEqual(left, right);
            break;
        case TypedBinOp::TYPED_EQ:
            condition = builder_.DoubleEqual(left, right);
            break;
        case TypedBinOp::TYPED_NOTEQ:
            condition = builder_.DoubleNotEqual(left, right);
            break;
        default:
            break;
    }
    return condition;
}
}  // namespace panda::ecmascript
