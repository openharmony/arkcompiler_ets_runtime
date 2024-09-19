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

#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/compiler/typed_hcr_lowering.h"
#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/number_speculative_lowering.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_object.h"

namespace panda::ecmascript::kungfu {

void NumberSpeculativeLowering::Run()
{
    std::vector<GateRef> gateList;
    acc_.GetAllGates(gateList);
    for (auto gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        switch (op) {
            case OpCode::RANGE_GUARD: {
                rangeGuardGates_.push_back(gate);
                break;
            }
            default: {
                VisitGate(gate);
            }
        }
    }
    for (auto rangeGuard : rangeGuardGates_) {
        VisitRangeGuard(rangeGuard);
    }
}

void NumberSpeculativeLowering::VisitGate(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::TYPED_BINARY_OP: {
            VisitTypedBinaryOp(gate);
            break;
        }
        case OpCode::TYPED_UNARY_OP: {
            VisitTypedUnaryOp(gate);
            break;
        }
        case OpCode::TYPED_CONDITION_JUMP: {
            VisitTypedConditionJump(gate);
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
        case OpCode::LOAD_ELEMENT: {
            VisitLoadElement(gate);
            break;
        }
        case OpCode::INDEX_CHECK: {
            VisitIndexCheck(gate);
            break;
        }
        case OpCode::RANGE_CHECK_PREDICATE: {
            VisitRangeCheckPredicate(gate);
            break;
        }
        case OpCode::LOAD_ARRAY_LENGTH:
        case OpCode::LOAD_TYPED_ARRAY_LENGTH: {
            VisitLoadArrayLength(gate);
            break;
        }
        case OpCode::LOAD_STRING_LENGTH: {
            VisitLoadStringLength(gate);
            break;
        }
        case OpCode::LOAD_MAP_SIZE: {
            VisitLoadMapSize(gate);
            break;
        }
        case OpCode::LOAD_PROPERTY: {
            VisitLoadProperty(gate);
            break;
        }
        case OpCode::MONO_LOAD_PROPERTY_ON_PROTO: {
            VisitLoadPropertyOnProto(gate);
            break;
        }
        case OpCode::MATH_ROUND: {
            VisitRound(gate);
            break;
        }
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitTypedBinaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    if (acc_.HasStringType(gate)) {
        VisitStringBinaryOp(gate);
        return;
    }
    auto op = acc_.GetTypedBinaryOp(gate);
    switch (op) {
        case TypedBinOp::TYPED_STRICTEQ:
        case TypedBinOp::TYPED_STRICTNOTEQ: {
            VisitStrictEqualOrStrictNotEqual(gate);
            break;
        }
        case TypedBinOp::TYPED_EQ:
        case TypedBinOp::TYPED_NOTEQ: {
            VisitEqualOrNotEqual(gate);
            break;
        }
        default: {
            if (acc_.HasNumberType(gate)) {
                VisitNumberBinaryOp(gate);
            }
            break;
        }
    }
}

void NumberSpeculativeLowering::VisitEqualOrNotEqual(GateRef gate)
{
    if (acc_.HasNumberType(gate)) {
        VisitNumberBinaryOp(gate);
    } else {
        VisitUndefinedEqOrUndefinedNotEq(gate);
    }
}

void NumberSpeculativeLowering::VisitStrictEqualOrStrictNotEqual(GateRef gate)
{
    if (acc_.HasNumberType(gate)) {
        VisitNumberBinaryOp(gate);
    } else {
        VisitUndefinedStrictEqOrUndefinedStrictNotEq(gate);
    }
}

void NumberSpeculativeLowering::VisitNumberBinaryOp(GateRef gate)
{
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
        case TypedBinOp::TYPED_STRICTEQ: {
            VisitNumberCompare<TypedBinOp::TYPED_STRICTEQ>(gate);
            break;
        }
        case TypedBinOp::TYPED_STRICTNOTEQ: {
            VisitNumberCompare<TypedBinOp::TYPED_STRICTNOTEQ>(gate);
            break;
        }
        case TypedBinOp::TYPED_SHL: {
            VisitNumberShift<TypedBinOp::TYPED_SHL>(gate);
            break;
        }
        case TypedBinOp::TYPED_SHR: {
            VisitNumberShift<TypedBinOp::TYPED_SHR>(gate);
            break;
        }
        case TypedBinOp::TYPED_ASHR: {
            VisitNumberShift<TypedBinOp::TYPED_ASHR>(gate);
            break;
        }
        case TypedBinOp::TYPED_AND: {
            VisitNumberLogical<TypedBinOp::TYPED_AND>(gate);
            break;
        }
        case TypedBinOp::TYPED_OR: {
            VisitNumberLogical<TypedBinOp::TYPED_OR>(gate);
            break;
        }
        case TypedBinOp::TYPED_XOR: {
            VisitNumberLogical<TypedBinOp::TYPED_XOR>(gate);
            break;
        }
        case TypedBinOp::TYPED_DIV: {
            VisitNumberDiv(gate);
            break;
        }
        case TypedBinOp::TYPED_MOD: {
            VisitNumberMod<TypedBinOp::TYPED_MOD>(gate);
            break;
        }
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitTypedUnaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedUnOp Op = acc_.GetTypedUnAccessor(gate).GetTypedUnOp();
    switch (Op) {
        case TypedUnOp::TYPED_INC: {
            VisitNumberMonocular<TypedUnOp::TYPED_INC>(gate);
            return;
        }
        case TypedUnOp::TYPED_DEC: {
            VisitNumberMonocular<TypedUnOp::TYPED_DEC>(gate);
            return;
        }
        case TypedUnOp::TYPED_NEG: {
            VisitNumberMonocular<TypedUnOp::TYPED_NEG>(gate);
            return;
        }
        case TypedUnOp::TYPED_ISFALSE: {
            VisitIsTrueOrFalse(gate, false);
            return;
        }
        case TypedUnOp::TYPED_ISTRUE: {
            VisitIsTrueOrFalse(gate, true);
            return;
        }
        case TypedUnOp::TYPED_NOT: {
            VisitNumberNot(gate);
            return;
        }
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitTypedConditionJump(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ParamType type = acc_.GetTypedJumpAccessor(gate).GetParamType();
    if (type.IsBooleanType()) {
        VisitBooleanJump(gate);
    } else {
        UNREACHABLE();
    }
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitNumberCalculate(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    GateRef result = Circuit::NullGate();
    if (paramType.IsIntType()) {
        result = CalculateInts<Op>(left, right);    // int op int
        UpdateRange(result, GetRange(gate));
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
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    GateRef result = Circuit::NullGate();
    if (paramType.IsIntType()) {
        result = CompareInts<Op>(left, right);  // int op int
    } else {
        result = CompareDoubles<Op>(left, right);   // float op float
    }
    acc_.SetMachineType(gate, MachineType::I1);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitNumberShift(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateRef result = ShiftInts<Op>(left, right);  // int op int
    UpdateRange(result, GetRange(gate));
    acc_.SetMachineType(gate, MachineType::I32);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitNumberLogical(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateRef result = LogicalInts<Op>(left, right);  // int op int
    UpdateRange(result, GetRange(gate));
    acc_.SetMachineType(gate, MachineType::I32);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitNumberDiv(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    GateRef result = Circuit::NullGate();
    if (paramType.IsIntType()) {
        result = builder_.Int32DivWithCheck(left, right);
        acc_.SetMachineType(gate, MachineType::I32);
    } else {
        result = builder_.BinaryArithmetic(circuit_->Fdiv(),
            MachineType::F64, left, right, GateType::NJSValue());
        acc_.SetMachineType(gate, MachineType::F64);
    }
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitNumberMod(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    GateRef result = Circuit::NullGate();
    if (paramType.IsIntType()) {
        if (GetRange(right).MaybeZero()) {
            builder_.Int32CheckRightIsZero(right);
        }
        bool isNegativeZero = (GetRange(left) % GetRange(right)).MaybeZero() && GetRange(left).MaybeNegative();
        if (isNegativeZero) {
            builder_.RemainderIsNegativeZero(left, right);
        }
        result = CalculateInts<Op>(left, right);
        UpdateRange(result, GetRange(gate));
        acc_.SetMachineType(gate, MachineType::I32);
    } else {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatMod),
            Gate::InvalidGateRef, {left, right}, Circuit::NullGate());
        acc_.SetMachineType(gate, MachineType::F64);
    }
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<TypedUnOp Op>
void NumberSpeculativeLowering::VisitNumberMonocular(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    ParamType type = accessor.GetParamType();
    ASSERT(type.HasNumberType());
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = Circuit::NullGate();
    if (type.IsIntType()) {
        if (Op == TypedUnOp::TYPED_NEG) {
            builder_.ValueCheckNegOverflow(value);
        }
        result = MonocularInt<Op>(value);
        UpdateRange(result, GetRange(gate));
        acc_.SetMachineType(gate, MachineType::I32);
    } else {
        result = MonocularDouble<Op>(value);
        acc_.SetMachineType(gate, MachineType::F64);
    }
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitNumberNot(GateRef gate)
{
    ASSERT(TypedUnaryAccessor(acc_.TryGetValue(gate)).GetParamType().HasNumberType());
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.Int32Not(value);
    UpdateRange(result, GetRange(gate));
    acc_.SetMachineType(gate, MachineType::I32);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitIsTrueOrFalse(GateRef gate, bool flag)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = Circuit::NullGate();
    if (!flag) {
        result = builder_.BoolNot(value);
    } else {
        result = value;
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitBooleanJump(GateRef gate)
{
    TypedJumpAccessor jumpAcc = acc_.GetTypedJumpAccessor(gate);
    TypedJumpOp jumpOp = jumpAcc.GetTypedJumpOp();
    ASSERT((jumpOp == TypedJumpOp::TYPED_JEQZ) || (jumpOp == TypedJumpOp::TYPED_JNEZ));
    GateRef condition = acc_.GetValueIn(gate, 0);
    uint32_t trueWeight = jumpAcc.GetTrueWeight();
    uint32_t falseWeight = jumpAcc.GetFalseWeight();
    if (jumpOp == TypedJumpOp::TYPED_JEQZ) {
        std::swap(trueWeight, falseWeight);
        condition = builder_.BoolNot(condition);
    }
    GateRef ifBranch = builder_.Branch(acc_.GetState(gate), condition, trueWeight, falseWeight, "booleanJump");
    acc_.ReplaceGate(gate, ifBranch, acc_.GetDep(gate), Circuit::NullGate());
}

void NumberSpeculativeLowering::VisitUndefinedStrictEqOrUndefinedStrictNotEq(GateRef gate)
{
    ASSERT(acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_STRICTEQ ||
           acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_STRICTNOTEQ);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    ASSERT(acc_.IsUndefinedOrNullOrHole(left) || acc_.IsUndefinedOrNullOrHole(right));
    GateRef result = Circuit::NullGate();
    if (acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_STRICTEQ) {
        result = builder_.Equal(left, right);
    } else {
        result = builder_.NotEqual(left, right);
    }
    ASSERT(result != Circuit::NullGate());
    acc_.SetMachineType(gate, MachineType::I1);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitUndefinedEqOrUndefinedNotEq(GateRef gate)
{
    ASSERT(acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_EQ ||
           acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_NOTEQ);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    ASSERT(acc_.IsUndefinedOrNullOrHole(left) || acc_.IsUndefinedOrNullOrHole(right));
    GateRef valueGate =  acc_.IsUndefinedOrNullOrHole(left) ? right : left;
    GateRef result = Circuit::NullGate();
    if (acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_EQ) {
        result = builder_.TaggedIsUndefinedOrNullOrHole(valueGate);
    } else {
        result = builder_.TaggedIsNotUndefinedAndNullAndHole(valueGate);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitConstant(GateRef gate)
{
    TypeInfo output = GetOutputType(gate);
    switch (output) {
        case TypeInfo::INT32: {
            int value = acc_.GetInt32FromConstant(gate);
            GateRef constGate = GetConstInt32(value);
            acc_.UpdateAllUses(gate, constGate);
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
    TypeInfo output = GetOutputType(gate);
    switch (output) {
        case TypeInfo::INT1: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::I1);
            break;
        }
        case TypeInfo::INT32:
        case TypeInfo::UINT32: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::I32);
            break;
        }
        case TypeInfo::FLOAT64: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::F64);
            break;
        }
        case TypeInfo::CHAR: {
            acc_.SetGateType(gate, GateType::NJSValue());
            acc_.SetMachineType(gate, MachineType::I32);
            break;
        }
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitRangeCheckPredicate(GateRef gate)
{
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.SetMachineType(gate, MachineType::I32);
}

void NumberSpeculativeLowering::VisitIndexCheck(GateRef gate)
{
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.SetMachineType(gate, MachineType::I32);
}

void NumberSpeculativeLowering::VisitLoadArrayLength(GateRef gate)
{
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.SetMachineType(gate, MachineType::I32);
}

void NumberSpeculativeLowering::VisitLoadStringLength(GateRef gate)
{
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.SetMachineType(gate, MachineType::I32);
}

void NumberSpeculativeLowering::VisitLoadMapSize(GateRef gate)
{
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.SetMachineType(gate, MachineType::I32);
}

void NumberSpeculativeLowering::VisitLoadElement(GateRef gate)
{
    auto op = acc_.GetTypedLoadOp(gate);
    switch (op) {
        case TypedLoadOp::INT8ARRAY_LOAD_ELEMENT:
        case TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT:
        case TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT:
        case TypedLoadOp::INT16ARRAY_LOAD_ELEMENT:
        case TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT:
        case TypedLoadOp::INT32ARRAY_LOAD_ELEMENT:
        case TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT:
            acc_.SetMachineType(gate, MachineType::I32);
            acc_.SetGateType(gate, GateType::NJSValue());
            break;
        case TypedLoadOp::ARRAY_LOAD_HOLE_INT_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_HOLE_DOUBLE_ELEMENT:
            acc_.SetMachineType(gate, MachineType::I64);
            acc_.SetGateType(gate, GateType::NJSValue());
            break;
        case TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT:
        case TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT:
            acc_.SetMachineType(gate, MachineType::F64);
            acc_.SetGateType(gate, GateType::NJSValue());
            break;
        default:
            break;
    }
}

void NumberSpeculativeLowering::VisitLoadProperty(GateRef gate)
{
    TypeInfo output = GetOutputType(gate);
    if (output == TypeInfo::INT32 || output == TypeInfo::FLOAT64) {
        Environment env(gate, circuit_, &builder_);
        ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
        GateRef receiver = acc_.GetValueIn(gate, 0);
        GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
        PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
        ASSERT(plr.IsLocal() || plr.IsFunction());

        // Hole check?
        GateRef result = Circuit::NullGate();
        if (output == TypeInfo::FLOAT64) {
            if (plr.IsInlinedProps()) {
                result = builder_.LoadConstOffset(VariableType::FLOAT64(), receiver, plr.GetOffset());
            } else {
                auto properties =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
                result = builder_.GetValueFromTaggedArray(
                    VariableType::FLOAT64(), properties, builder_.Int32(plr.GetOffset()));
            }
            acc_.SetMachineType(gate, MachineType::F64);
        } else {
            if (plr.IsInlinedProps()) {
                result = builder_.LoadConstOffset(VariableType::INT32(), receiver, plr.GetOffset());
            } else {
                auto properties =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
                result = builder_.GetValueFromTaggedArray(
                    VariableType::INT32(), properties, builder_.Int32(plr.GetOffset()));
            }
            acc_.SetMachineType(gate, MachineType::I32);
        }
        acc_.SetGateType(gate, GateType::NJSValue());
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void NumberSpeculativeLowering::VisitRangeGuard(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef inputLength = acc_.GetValueIn(gate, 0);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), inputLength);
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::CalculateInts(GateRef left, GateRef right)
{
    GateRef res = Circuit::NullGate();
    RangeInfo leftRange = GetRange(left);
    RangeInfo rightRange = GetRange(right);
    switch (Op) {
        case TypedBinOp::TYPED_ADD: {
            if (!leftRange.MaybeAddOverflowOrUnderflow(rightRange)) {
                return builder_.Int32Add(left, right, GateType::NJSValue());
            }
            res = builder_.AddWithOverflow(left, right);
            break;
        }
        case TypedBinOp::TYPED_SUB: {
            if (!leftRange.MaybeSubOverflowOrUnderflow(rightRange)) {
                return builder_.Int32Sub(left, right, GateType::NJSValue());
            }
            res = builder_.SubWithOverflow(left, right);
            break;
        }
        case TypedBinOp::TYPED_MUL:
            if (!leftRange.MaybeMulOverflowOrUnderflow(rightRange)) {
                return builder_.Int32Mul(left, right);
            }
            res = builder_.MulWithOverflow(left, right);
            break;
        case TypedBinOp::TYPED_MOD: {
            return builder_.BinaryArithmetic(circuit_->Smod(),
                MachineType::I32, left, right, GateType::NJSValue());
            break;
        }
        default:
            break;
    }
    // DeoptCheckForOverFlow
    builder_.OverflowCheck(res);
    return builder_.ExtractValue(MachineType::I32, res, GetConstInt32(0));
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::CalculateDoubles(GateRef left, GateRef right)
{
    GateRef res = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_ADD:
            res = builder_.DoubleAdd(left, right, GateType::NJSValue());
            break;
        case TypedBinOp::TYPED_SUB:
            res = builder_.DoubleSub(left, right, GateType::NJSValue());
            break;
        case TypedBinOp::TYPED_MUL:
            res = builder_.DoubleMul(left, right, GateType::NJSValue());
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
        case TypedBinOp::TYPED_STRICTEQ:
            condition = builder_.Int32Equal(left, right);
            break;
        case TypedBinOp::TYPED_NOTEQ:
        case TypedBinOp::TYPED_STRICTNOTEQ:
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
        case TypedBinOp::TYPED_STRICTEQ: {
            condition = LogicAndBuilder(builder_.GetCurrentEnvironment())
                .And(builder_.BoolNot(builder_.DoubleIsNAN(left)))
                .And(builder_.BoolNot(builder_.DoubleIsNAN(right)))
                .And(builder_.DoubleEqual(left, right))
                .Done();
            break;
        }
        case TypedBinOp::TYPED_NOTEQ:
        case TypedBinOp::TYPED_STRICTNOTEQ: {
            condition = LogicOrBuilder(builder_.GetCurrentEnvironment())
                .Or(builder_.DoubleIsNAN(left))
                .Or(builder_.DoubleIsNAN(right))
                .Or(builder_.DoubleNotEqual(left, right))
                .Done();
            break;
        }
        default:
            break;
    }
    return condition;
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::ShiftInts(GateRef left, GateRef right)
{
    GateRef value = Circuit::NullGate();
    GateRef bitmask = GetConstInt32(0x1f); // 0x1f: bit mask of shift value
    GateRef shift = builder_.Int32And(right, bitmask, GateType::NJSValue());
    switch (Op) {
        case TypedBinOp::TYPED_SHL: {
            value = builder_.Int32LSL(left, shift, GateType::NJSValue());
            break;
        }
        case TypedBinOp::TYPED_SHR: {
            value = builder_.Int32LSR(left, shift, GateType::NJSValue());
            RangeInfo leftRange = GetRange(left);
            RangeInfo rightRange = GetRange(right);
            if (!leftRange.MaybeShrOverflow(rightRange)) {
                return value;
            }
            builder_.Int32UnsignedUpperBoundCheck(value, builder_.Int32(INT32_MAX));
            break;
        }
        case TypedBinOp::TYPED_ASHR: {
            value = builder_.Int32ASR(left, shift, GateType::NJSValue());
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return value;
}

template<TypedBinOp Op>
GateRef NumberSpeculativeLowering::LogicalInts(GateRef left, GateRef right)
{
    GateRef value = Circuit::NullGate();
    switch (Op) {
        case TypedBinOp::TYPED_AND: {
            value = builder_.Int32And(left, right, GateType::NJSValue());
            break;
        }
        case TypedBinOp::TYPED_OR: {
            value = builder_.Int32Or(left, right, GateType::NJSValue());
            break;
        }
        case TypedBinOp::TYPED_XOR: {
            value = builder_.Int32Xor(left, right, GateType::NJSValue());
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return value;
}

template<TypedUnOp Op>
GateRef NumberSpeculativeLowering::MonocularInt(GateRef value)
{
    GateRef res = Circuit::NullGate();
    switch (Op) {
        case TypedUnOp::TYPED_INC:
            res = CalculateInts<TypedBinOp::TYPED_ADD>(value, GetConstInt32(1));
            break;
        case TypedUnOp::TYPED_DEC:
            res = CalculateInts<TypedBinOp::TYPED_SUB>(value, GetConstInt32(1));
            break;
        case TypedUnOp::TYPED_NEG:
            res = builder_.Int32Sub(GetConstInt32(0), value, GateType::NJSValue());
            break;
        default:
            break;
    }
    return res;
}

template<TypedUnOp Op>
GateRef NumberSpeculativeLowering::MonocularDouble(GateRef value)
{
    GateRef res = Circuit::NullGate();
    switch (Op) {
        case TypedUnOp::TYPED_INC:
            res = CalculateDoubles<TypedBinOp::TYPED_ADD>(value, GetConstDouble(1));
            break;
        case TypedUnOp::TYPED_DEC:
            res = CalculateDoubles<TypedBinOp::TYPED_SUB>(value, GetConstDouble(1));
            break;
        case TypedUnOp::TYPED_NEG:
            res = CalculateDoubles<TypedBinOp::TYPED_MUL>(value, GetConstDouble(-1));
            break;
        default:
            break;
    }
    return res;
}

void NumberSpeculativeLowering::UpdateRange(GateRef gate, const RangeInfo& range)
{
    auto id = acc_.GetId(gate);
    if (id >= rangeInfos_.size()) {
        rangeInfos_.resize(id + 1, RangeInfo::ANY());
    }
    rangeInfos_[id] = range;
}

RangeInfo NumberSpeculativeLowering::GetRange(GateRef gate) const
{
    auto id = acc_.GetId(gate);
    if (id >= rangeInfos_.size()) {
        rangeInfos_.resize(id + 1, RangeInfo::ANY());
    }
    ASSERT(!rangeInfos_[id].IsNone());
    return rangeInfos_[id];
}

GateRef NumberSpeculativeLowering::GetConstInt32(int32_t v)
{
    auto val = builder_.Int32(v);
    UpdateRange(val, RangeInfo(v, v));
    return val;
}

GateRef NumberSpeculativeLowering::GetConstDouble(double v)
{
    auto val = builder_.Double(v);
    UpdateRange(val, RangeInfo(v, v));
    return val;
}

void NumberSpeculativeLowering::VisitStringBinaryOp(GateRef gate)
{
    TypedBinOp Op = acc_.GetTypedBinaryOp(gate);
    switch (Op) {
        case TypedBinOp::TYPED_EQ: {
            VisitStringCompare<TypedBinOp::TYPED_EQ>(gate);
            break;
        }
        case TypedBinOp::TYPED_ADD: {
            VisitStringAdd<TypedBinOp::TYPED_ADD>(gate);
            break;
        }
        default:
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitStringCompare(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);

    GateRef result;
    ASSERT(Op == TypedBinOp::TYPED_EQ);
    result = builder_.StringEqual(left, right);

    acc_.SetMachineType(gate, MachineType::I1);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template<TypedBinOp Op>
void NumberSpeculativeLowering::VisitStringAdd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);

    GateRef result;
    ASSERT(Op == TypedBinOp::TYPED_ADD);
    result = builder_.StringAdd(left, right);

    acc_.SetMachineType(gate, MachineType::I64);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void NumberSpeculativeLowering::VisitLoadPropertyOnProto(GateRef gate)
{
    TypeInfo output = GetOutputType(gate);
    if (output == TypeInfo::INT32 || output == TypeInfo::FLOAT64) {
        Environment env(gate, circuit_, &builder_);
        GateRef frameState = acc_.GetFrameState(gate);
        GateRef receiver = acc_.GetValueIn(gate, 0);
        GateRef propertyLookupResult = acc_.GetValueIn(gate, 1); // 1: propertyLookupResult
        GateRef hclassIndex = acc_.GetValueIn(gate, 2); // 2: hclassIndex
        GateRef unsharedConstPool = acc_.GetValueIn(gate, 3); // 3: constpool
        PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
        GateRef result = Circuit::NullGate();
        ASSERT(plr.IsLocal() || plr.IsFunction());

        auto receiverHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, TaggedObject::HCLASS_OFFSET);
        auto prototype = builder_.LoadConstOffset(VariableType::JS_ANY(), receiverHC, JSHClass::PROTOTYPE_OFFSET);

        auto holderHC = builder_.LoadHClassFromConstpool(unsharedConstPool, acc_.GetConstantValue(hclassIndex));
        DEFVALUE(current, (&builder_), VariableType::JS_ANY(), prototype);
        Label exit(&builder_);
        Label loopHead(&builder_);
        Label loadHolder(&builder_);
        Label lookUpProto(&builder_);
        builder_.Jump(&loopHead);

        builder_.LoopBegin(&loopHead);
        builder_.DeoptCheck(builder_.TaggedIsNotNull(*current), frameState, DeoptType::INCONSISTENTHCLASS7);
        auto curHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), *current, TaggedObject::HCLASS_OFFSET);
        BRANCH_CIR(builder_.Equal(curHC, holderHC), &loadHolder, &lookUpProto);

        builder_.Bind(&lookUpProto);
        current = builder_.LoadConstOffset(VariableType::JS_ANY(), curHC, JSHClass::PROTOTYPE_OFFSET);
        builder_.LoopEnd(&loopHead);

        builder_.Bind(&loadHolder);
        if (output == TypeInfo::FLOAT64) {
            if (plr.IsInlinedProps()) {
                result = builder_.LoadConstOffset(VariableType::FLOAT64(), *current, plr.GetOffset());
            } else {
                auto properties =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), *current, JSObject::PROPERTIES_OFFSET);
                result = builder_.GetValueFromTaggedArray(
                    VariableType::FLOAT64(), properties, builder_.Int32(plr.GetOffset()));
            }
            acc_.SetMachineType(gate, MachineType::F64);
        } else {
            if (plr.IsInlinedProps()) {
                result = builder_.LoadConstOffset(VariableType::INT32(), *current, plr.GetOffset());
            } else {
                auto properties =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), *current, JSObject::PROPERTIES_OFFSET);
                result = builder_.GetValueFromTaggedArray(
                    VariableType::INT32(), properties, builder_.Int32(plr.GetOffset()));
            }
            acc_.SetMachineType(gate, MachineType::I32);
        }
        builder_.Jump(&exit);
        builder_.Bind(&exit);
        acc_.SetGateType(gate, GateType::NJSValue());
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
    }
}

void NumberSpeculativeLowering::VisitRound(GateRef gate)
{
    TypeInfo output = GetOutputType(gate);
    GateRef in = acc_.GetValueIn(gate, 0);
    if (output == TypeInfo::INT32) {
        acc_.ReplaceGate(gate, in);
    }
}

}  // namespace panda::ecmascript
