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

#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/ark_stackmap.h"

namespace panda::ecmascript::kungfu {
void TypeLowering::RunTypeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (acc_.IsTypedGate(gate)) {
            LowerType(gate);
        } else if (op == OpCode::DEOPT) {
            LowerDeoptimize(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m" << "================== after type lowering ==================" << "\033[0m";
        circuit_->PrintAllGates(*bcBuilder_);
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End =========================" << "\033[0m";
    }
}

void TypeLowering::LowerType(GateRef gate)
{
    auto op = OpCode::Op(acc_.GetOpCode(gate));
    switch (op) {
        case OpCode::TYPE_CHECK:
            LowerTypeCheck(gate);
            break;
        case OpCode::TYPED_BINARY_OP:
            LowerTypedBinaryOp(gate);
            break;
        case OpCode::TYPE_CONVERT:
            LowerTypeConvert(gate);
            break;
        case OpCode::TYPED_UNARY_OP:
            LowerTypedUnaryOp(gate);
            break;
        case OpCode::DEOPT:
            LowerDeoptimize(gate);
            break;
        default:
            break;
    }
}

void TypeLowering::LowerTypeConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateAccessor acc(circuit_);
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (rightType.IsNumberType()) {
        GateRef value = acc_.GetValueIn(gate, 0);
        if (leftType.IsPrimitiveType() && !leftType.IsStringType()) {
            LowerPrimitiveToNumber(gate, value, leftType);
        }
        return;
    }
}

void TypeLowering::LowerPrimitiveToNumber(GateRef dst, GateRef src, GateType srcType)
{
    Label exit(&builder_);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    if (srcType.IsBooleanType()) {
        Label isTrue(&builder_);
        Label isFalse(&builder_);
        builder_.Branch(builder_.TaggedIsTrue(src), &isTrue, &isFalse);
        builder_.Bind(&isTrue);
        result = IntToTaggedIntPtr(builder_.Int32(1));
        builder_.Jump(&exit);
        builder_.Bind(&isFalse);
        result = IntToTaggedIntPtr(builder_.Int32(0));
        builder_.Jump(&exit);
    } else if (srcType.IsUndefinedType()) {
        result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    } else if (srcType.IsBigIntType() || srcType.IsNumberType()) {
        result = src;
        builder_.Jump(&exit);
    } else if (srcType.IsNullType()) {
        result = IntToTaggedIntPtr(builder_.Int32(0));
        builder_.Jump(&exit);
    } else {
        UNREACHABLE();
    }
    builder_.Bind(&exit);
    ReplaceGateToSubCfg(dst, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerTypeCheck(GateRef gate)
{
    auto type = GateType(static_cast<uint32_t>(acc_.GetBitField(gate)));
    if (type.IsNumberType()) {
        LowerNumberCheck(gate);
        return;
    }
}

void TypeLowering::LowerNumberCheck(GateRef gate)
{
    auto value = acc_.GetValueIn(gate, 0);
    auto typeCheck = builder_.TaggedIsNumber(value);
    acc_.UpdateAllUses(gate, typeCheck);
    acc_.DeleteGate(gate);
}

void TypeLowering::LowerTypedBinaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto opGate = acc_.GetValueIn(gate, 2);
    auto op = static_cast<TypedBinOp>(acc_.GetBitField(opGate));
    switch (op) {
        case TypedBinOp::TYPED_ADD: {
            LowerTypedAdd(gate);
            break;
        }
        case TypedBinOp::TYPED_SUB: {
            LowerTypedSub(gate);
            break;
        }
        case TypedBinOp::TYPED_MUL: {
            LowerTypedMul(gate);
            break;
        }
        case TypedBinOp::TYPED_MOD: {
            LowerTypedMod(gate);
            break;
        }
        case TypedBinOp::TYPED_LESS: {
            LowerTypedLess(gate);
            break;
        }
        case TypedBinOp::TYPED_LESSEQ: {
            LowerTypedLessEq(gate);
            break;
        }
        case TypedBinOp::TYPED_GREATER: {
            LowerTypedGreater(gate);
            break;
        }
        case TypedBinOp::TYPED_GREATEREQ: {
            LowerTypedGreaterEq(gate);
            break;
        }
        case TypedBinOp::TYPED_DIV: {
            LowerTypedDiv(gate);
            break;
        }
        case TypedBinOp::TYPED_EQ: {
            LowerTypedEq(gate);
            break;
        }
        case TypedBinOp::TYPED_NOTEQ: {
            LowerTypedNotEq(gate);
            break;
        }
        default:
            break;
    }
}

void TypeLowering::LowerTypedUnaryOp(GateRef gate)
{
    auto bitfield = acc_.GetBitField(gate);
    auto temp = bitfield >>  CircuitBuilder::OPRAND_TYPE_BITS;
    auto op = static_cast<TypedUnaryOp>(bitfield ^ (temp << CircuitBuilder::OPRAND_TYPE_BITS));
    switch (op) {
        case TypedUnaryOp::TYPED_TONUMBER:
            break;
        case TypedUnaryOp::TYPED_NEG:
            break;
        case TypedUnaryOp::TYPED_NOT:
            break;
        case TypedUnaryOp::TYPED_INC:
            break;
        case TypedUnaryOp::TYPED_DEC:
            break;
        default:
            break;
    }
}

GateType TypeLowering::GetLeftType(GateRef gate)
{
    auto operandTypes = acc_.GetBitField(gate);
    auto temp = operandTypes >> CircuitBuilder::OPRAND_TYPE_BITS;
    return GateType(static_cast<uint32_t>(temp));
}

GateType TypeLowering::GetRightType(GateRef gate)
{
    auto operandTypes = acc_.GetBitField(gate);
    auto temp = operandTypes >> CircuitBuilder::OPRAND_TYPE_BITS;
    return GateType(static_cast<uint32_t>(operandTypes ^ (temp << CircuitBuilder::OPRAND_TYPE_BITS)));
}

void TypeLowering::LowerTypedAdd(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberAdd(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedSub(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberSub(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedMul(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberMul(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedMod(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberMod(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedLess(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberLess(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedLessEq(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberLessEq(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedGreater(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberGreater(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedGreaterEq(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberGreaterEq(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedDiv(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberDiv(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedEq(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberEq(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerTypedNotEq(GateRef gate)
{
    auto leftType = GetLeftType(gate);
    auto rightType = GetRightType(gate);
    if (leftType.IsNumberType() && rightType.IsNumberType()) {
        LowerNumberNotEq(gate);
    } else {
        UNREACHABLE();
    }
    return;
}

void TypeLowering::LowerNumberAdd(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CalculateNumbers<OpCode::ADD>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberSub(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CalculateNumbers<OpCode::SUB>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberMul(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CalculateNumbers<OpCode::MUL>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberMod(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = ModNumbers(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberLess(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESS>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberLessEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESSEQ>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberGreater(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESS>(right, left);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberGreaterEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_LESSEQ>(right, left);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberDiv(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = DivNumbers(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_EQ>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypeLowering::LowerNumberNotEq(GateRef gate)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    result = CompareNumbers<TypedBinOp::TYPED_NOTEQ>(left, right);
    ReplaceGateToSubCfg(gate, builder_.GetState(), builder_.GetDepend(), *result);
}



void TypeLowering::ReplaceGateToSubCfg(GateRef gate, GateRef state, GateRef depend, GateRef value)
{
    auto uses = acc_.Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        if (acc_.IsStateIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, state);
        } else if (acc_.IsDependIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, depend);
        } else if (acc_.IsValueIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, value);
        } else {
            UNREACHABLE();
        }
    }
    acc_.DeleteGate(gate);
}

void TypeLowering::ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch;
    if (!noThrow) {
        // exception value
        GateRef exceptionVal = builder_.ExceptionConstant();
        // compare with trampolines result
        GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);
        ifBranch = builder_.Branch(stateInGate, equal);
    } else {
        ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    }

    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            it = acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                it = acc_.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    acc_.DeleteGate(hirGate);
}

GateRef TypeLowering::LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args);
        return result;
    }
}

template<OpCode::Op Op>
GateRef TypeLowering::CalculateNumbers(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label exit(&builder_);
    Label doFloatOp(&builder_);
    Label doIntOp(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    Label leftIsInt(&builder_);
    Label leftIsDouble(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftIsDouble);
    builder_.Bind(&leftIsInt);
    {
        builder_.Branch(builder_.TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
        builder_.Bind(&leftIsIntRightIsDouble);
        {
            doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            doubleRight = builder_.GetDoubleOfTDouble(right);
            builder_.Jump(&doFloatOp);
        }
    }
    builder_.Bind(&leftIsDouble);
    {
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightIsDouble);
        builder_.Bind(&rightIsInt);
        {
            doubleLeft = builder_.GetDoubleOfTDouble(left);
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            builder_.Jump(&doFloatOp);
        }
        builder_.Bind(&rightIsDouble);
        {
            doubleLeft = builder_.GetDoubleOfTDouble(left);
            doubleRight = builder_.GetDoubleOfTDouble(right);
            builder_.Jump(&doFloatOp);
        }
    }
    builder_.Bind(&doIntOp);
    {
        Label overflow(&builder_);
        Label notOverflow(&builder_);
        Label notOverflowOrUnderflow(&builder_);
        // handle left is int and right is int
        GateRef res = BinaryOp<Op, MachineType::I64>(builder_.GetInt64OfTInt(left),
                                                     builder_.GetInt64OfTInt(right));
        GateRef max = builder_.Int64(INT32_MAX);
        GateRef min = builder_.Int64(INT32_MIN);
        builder_.Branch(builder_.Int64GreaterThan(res, max), &overflow, &notOverflow);
        builder_.Bind(&notOverflow);
        {
            builder_.Branch(builder_.Int64LessThan(res, min), &overflow, &notOverflowOrUnderflow);
        }
        builder_.Bind(&overflow);
        {
            doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            builder_.Jump(&doFloatOp);
        }
        builder_.Bind(&notOverflowOrUnderflow);
        {
            result = builder_.ToTaggedIntPtr(res);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&doFloatOp);
    {
        // Other situations
        auto res = BinaryOp<Op, MachineType::F64>(*doubleLeft, *doubleRight);
        result = DoubleToTaggedDoublePtr(res);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode::Op Op>
GateRef TypeLowering::FastAddOrSubOrMul(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label exit(&builder_);
    Label doFloatOp(&builder_);
    Label doIntOp(&builder_);
    Label leftIsNumber(&builder_);
    Label rightIsNumber(&builder_);
    Label leftIsIntRightIsDouble(&builder_);
    Label rightIsInt(&builder_);
    Label rightIsDouble(&builder_);
    builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
    builder_.Bind(&leftIsNumber);
    {
        builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
        builder_.Bind(&rightIsNumber);
        {
            Label leftIsInt(&builder_);
            Label leftIsDouble(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            builder_.Bind(&leftIsInt);
            {
                builder_.Branch(builder_.TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
                builder_.Bind(&leftIsIntRightIsDouble);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&doFloatOp);
                }
            }
            builder_.Bind(&leftIsDouble);
            {
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                builder_.Bind(&rightIsInt);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&doFloatOp);
                }
                builder_.Bind(&rightIsDouble);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&doFloatOp);
                }
            }
        }
    }
    builder_.Bind(&doIntOp);
    {
        Label overflow(&builder_);
        Label notOverflow(&builder_);
        // handle left is int and right is int
        GateRef res = BinaryOp<Op, MachineType::I64>(builder_.GetInt64OfTInt(left),
                                                     builder_.GetInt64OfTInt(right));
        GateRef max = builder_.Int64(INT32_MAX);
        GateRef min = builder_.Int64(INT32_MIN);
        Label greaterZero(&builder_);
        Label notGreaterZero(&builder_);
        builder_.Branch(builder_.Int32GreaterThan(builder_.GetInt32OfTInt(left), builder_.Int32(0)),
                        &greaterZero, &notGreaterZero);
        builder_.Bind(&greaterZero);
        {
            builder_.Branch(builder_.Int64GreaterThan(res, max), &overflow, &notOverflow);
        }
        builder_.Bind(&notGreaterZero);
        {
            Label lessZero(&builder_);
            builder_.Branch(builder_.Int32LessThan(builder_.GetInt32OfTInt(left), builder_.Int32(0)),
                            &lessZero, &notOverflow);
            builder_.Bind(&lessZero);
            builder_.Branch(builder_.Int64LessThan(res, min), &overflow, &notOverflow);
        }
        builder_.Bind(&overflow);
        {
            GateRef newDoubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            GateRef newDoubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            GateRef middleRet = BinaryOp<Op, MachineType::F64>(newDoubleLeft, newDoubleRight);
            result = DoubleToTaggedDoublePtr(middleRet);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notOverflow);
        {
            result = builder_.ToTaggedIntPtr(res);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&doFloatOp);
    {
        // Other situations
        auto res = BinaryOp<Op, MachineType::F64>(*doubleLeft, *doubleRight);
        result = DoubleToTaggedDoublePtr(res);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<TypedBinOp Op>
GateRef TypeLowering::CompareNumbers(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label leftIsInt(&builder_);
    Label leftOrRightNotInt(&builder_);
    Label leftOpRight(&builder_);
    Label leftNotOpRight(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            GateRef intLeft = builder_.GetInt32OfTInt(left);
            GateRef intRight = builder_.GetInt32OfTInt(right);
            builder_.Branch(CompareInt<Op>(intLeft, intRight), &leftOpRight, &leftNotOpRight);
        }
    }
    builder_.Bind(&leftOrRightNotInt);
    {
        // fast path
        DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
        DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
        Label leftIsInt1(&builder_);
        Label leftNotInt1(&builder_);
        Label exit1(&builder_);
        Label exit2(&builder_);
        Label rightIsInt1(&builder_);
        Label rightNotInt1(&builder_);
        builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
        builder_.Bind(&leftIsInt1);
        {
            doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            builder_.Jump(&exit1);
        }
        builder_.Bind(&leftNotInt1);
        {
            doubleLeft = builder_.GetDoubleOfTDouble(left);
            builder_.Jump(&exit1);
        }
        builder_.Bind(&exit1);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
        builder_.Bind(&rightIsInt1);
        {
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            builder_.Jump(&exit2);
        }
        builder_.Bind(&rightNotInt1);
        {
            doubleRight = builder_.GetDoubleOfTDouble(right);
            builder_.Jump(&exit2);
        }
        builder_.Bind(&exit2);
        builder_.Branch(CompareDouble<Op>(*doubleLeft, *doubleRight), &leftOpRight,
                        &leftNotOpRight);
    }
    builder_.Bind(&leftOpRight);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&leftNotOpRight);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<TypedBinOp Op>
GateRef TypeLowering::CompareInt(GateRef left, GateRef right)
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
GateRef TypeLowering::CompareDouble(GateRef left, GateRef right)
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

template<OpCode::Op Op, MachineType Type>
GateRef TypeLowering::BinaryOp(GateRef x, GateRef y)
{
    return builder_.BinaryArithmetic(OpCode(Op), Type, x, y);
}

GateRef TypeLowering::DoubleToTaggedDoublePtr(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef TypeLowering::ChangeInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef TypeLowering::Int32Mod(GateRef left, GateRef right)
{
    return BinaryOp<OpCode::SMOD, MachineType::I32>(left, right);
}

GateRef TypeLowering::DoubleMod(GateRef left, GateRef right)
{
    return BinaryOp<OpCode::FMOD, MachineType::F64>(left, right);
}

GateRef TypeLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

GateRef TypeLowering::DoubleIsINF(GateRef x)
{
    GateRef infinity = builder_.Double(base::POSITIVE_INFINITY);
    GateRef negativeInfinity = builder_.Double(-base::POSITIVE_INFINITY);
    GateRef isInfinity = builder_.Equal(x, infinity);
    GateRef isNegativeInfinity = builder_.Equal(x, negativeInfinity);
    return builder_.BoolOr(builder_.Equal(builder_.SExtInt1ToInt32(isInfinity), builder_.Int32(1)),
                           builder_.Equal(builder_.SExtInt1ToInt32(isNegativeInfinity), builder_.Int32(1)));
}

GateRef TypeLowering::GeneralMod(GateRef left, GateRef right, GateRef glue)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(intLeft, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(intRight, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label leftIsInt(&builder_);
    Label leftNotIntOrRightNotInt(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotIntOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftNotIntOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            intLeft = builder_.GetInt32OfTInt(left);
            intRight = builder_.GetInt32OfTInt(right);
            Label leftGreaterZero(&builder_);
            builder_.Branch(builder_.Int32GreaterThan(*intLeft, builder_.Int32(0)),
                            &leftGreaterZero, &leftNotIntOrRightNotInt);
            builder_.Bind(&leftGreaterZero);
            {
                Label rightGreaterZero(&builder_);
                builder_.Branch(builder_.Int32GreaterThan(*intRight, builder_.Int32(0)),
                                &rightGreaterZero, &leftNotIntOrRightNotInt);
                builder_.Bind(&rightGreaterZero);
                {
                    result = IntToTaggedIntPtr(Int32Mod(*intLeft, *intRight));
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&leftNotIntOrRightNotInt);
    {
        Label leftIsNumber(&builder_);
        Label leftNotNumberOrRightNotNumber(&builder_);
        Label leftIsNumberAndRightIsNumber(&builder_);
        Label leftIsDoubleAndRightIsDouble(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
        builder_.Bind(&leftIsNumber);
        {
            Label rightIsNumber(&builder_);
            builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
            builder_.Bind(&rightIsNumber);
            {
                Label leftIsInt1(&builder_);
                Label leftNotInt1(&builder_);
                builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                builder_.Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    builder_.Jump(&leftIsNumberAndRightIsNumber);
                }
                builder_.Bind(&leftNotInt1);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    builder_.Jump(&leftIsNumberAndRightIsNumber);
                }
            }
        }
        builder_.Bind(&leftNotNumberOrRightNotNumber);
        {
            builder_.Jump(&exit);
        }
        builder_.Bind(&leftIsNumberAndRightIsNumber);
        {
            Label rightIsInt1(&builder_);
            Label rightNotInt1(&builder_);
            builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
            builder_.Bind(&rightIsInt1);
            {
                doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                builder_.Jump(&leftIsDoubleAndRightIsDouble);
            }
            builder_.Bind(&rightNotInt1);
            {
                doubleRight = builder_.GetDoubleOfTDouble(right);
                builder_.Jump(&leftIsDoubleAndRightIsDouble);
            }
        }
        builder_.Bind(&leftIsDoubleAndRightIsDouble);
        {
            Label rightNotZero(&builder_);
            Label rightIsZeroOrNanOrLeftIsNanOrInf(&builder_);
            Label rightNotZeroAndNanAndLeftNotNanAndInf(&builder_);
            builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZeroOrNanOrLeftIsNanOrInf,
                            &rightNotZero);
            builder_.Bind(&rightNotZero);
            {
                Label rightNotNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleRight), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotNan);
                builder_.Bind(&rightNotNan);
                {
                    Label leftNotNan(&builder_);
                    builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf, &leftNotNan);
                    builder_.Bind(&leftNotNan);
                    builder_.Branch(DoubleIsINF(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf,
                                    &rightNotZeroAndNanAndLeftNotNanAndInf);
                }
            }
            builder_.Bind(&rightIsZeroOrNanOrLeftIsNanOrInf);
            {
                result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&rightNotZeroAndNanAndLeftNotNanAndInf);
            {
                Label leftNotZero(&builder_);
                Label leftIsZeroOrRightIsInf(&builder_);
                builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZeroOrRightIsInf,
                                &leftNotZero);
                builder_.Bind(&leftNotZero);
                {
                    Label rightNotInf(&builder_);
                    builder_.Branch(DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
                    builder_.Bind(&rightNotInf);
                    {
                        result = LowerCallRuntime(glue, RTSTUB_ID(FloatMod), {*doubleLeft, *doubleRight});
                        builder_.Jump(&exit);
                    }
                }
                builder_.Bind(&leftIsZeroOrRightIsInf);
                {
                    result = DoubleToTaggedDoublePtr(*doubleLeft);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::ModNumbers(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(intLeft, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(intRight, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label leftIsInt(&builder_);
    Label leftNotIntOrRightNotInt(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotIntOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftNotIntOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            intLeft = builder_.GetInt32OfTInt(left);
            intRight = builder_.GetInt32OfTInt(right);
            Label leftGreaterZero(&builder_);
            builder_.Branch(builder_.Int32GreaterThan(*intLeft, builder_.Int32(0)),
                            &leftGreaterZero, &leftNotIntOrRightNotInt);
            builder_.Bind(&leftGreaterZero);
            {
                Label rightGreaterZero(&builder_);
                builder_.Branch(builder_.Int32GreaterThan(*intRight, builder_.Int32(0)),
                                &rightGreaterZero, &leftNotIntOrRightNotInt);
                builder_.Bind(&rightGreaterZero);
                {
                    result = IntToTaggedIntPtr(Int32Mod(*intLeft, *intRight));
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&leftNotIntOrRightNotInt);
    {
        Label leftIsNumberAndRightIsNumber(&builder_);
        Label leftIsDoubleAndRightIsDouble(&builder_);
        Label leftIsInt1(&builder_);
        Label leftNotInt1(&builder_);
        builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
        builder_.Bind(&leftIsInt1);
        {
            doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
            builder_.Jump(&leftIsNumberAndRightIsNumber);
        }
        builder_.Bind(&leftNotInt1);
        {
            doubleLeft = builder_.GetDoubleOfTDouble(left);
            builder_.Jump(&leftIsNumberAndRightIsNumber);
        }
        builder_.Bind(&leftIsNumberAndRightIsNumber);
        {
            Label rightIsInt1(&builder_);
            Label rightNotInt1(&builder_);
            builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
            builder_.Bind(&rightIsInt1);
            {
                doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                builder_.Jump(&leftIsDoubleAndRightIsDouble);
            }
            builder_.Bind(&rightNotInt1);
            {
                doubleRight = builder_.GetDoubleOfTDouble(right);
                builder_.Jump(&leftIsDoubleAndRightIsDouble);
            }
        }
        builder_.Bind(&leftIsDoubleAndRightIsDouble);
        {
            Label rightNotZero(&builder_);
            Label rightIsZeroOrNanOrLeftIsNanOrInf(&builder_);
            Label rightNotZeroAndNanAndLeftNotNanAndInf(&builder_);
            builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZeroOrNanOrLeftIsNanOrInf,
                            &rightNotZero);
            builder_.Bind(&rightNotZero);
            {
                Label rightNotNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleRight), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotNan);
                builder_.Bind(&rightNotNan);
                {
                    Label leftNotNan(&builder_);
                    builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf, &leftNotNan);
                    builder_.Bind(&leftNotNan);
                    builder_.Branch(DoubleIsINF(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf,
                                    &rightNotZeroAndNanAndLeftNotNanAndInf);
                }
            }
            builder_.Bind(&rightIsZeroOrNanOrLeftIsNanOrInf);
            {
                result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&rightNotZeroAndNanAndLeftNotNanAndInf);
            {
                Label leftNotZero(&builder_);
                Label leftIsZeroOrRightIsInf(&builder_);
                builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZeroOrRightIsInf,
                                &leftNotZero);
                builder_.Bind(&leftNotZero);
                {
                    Label rightNotInf(&builder_);
                    builder_.Branch(DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
                    builder_.Bind(&rightNotInf);
                    {
                        ArgumentAccessor argAcc(circuit_);
                        auto glue = argAcc.GetCommonArgGate(CommonArgIdx::GLUE);
                        result = LowerCallRuntime(glue, RTSTUB_ID(FloatMod), {*doubleLeft, *doubleRight});
                        builder_.Jump(&exit);
                    }
                }
                builder_.Bind(&leftIsZeroOrRightIsInf);
                {
                    result = DoubleToTaggedDoublePtr(*doubleLeft);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::Less(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label leftIsInt(&builder_);
    Label leftOrRightNotInt(&builder_);
    Label leftLessRight(&builder_);
    Label leftGreaterEqRight(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            GateRef intLeft = builder_.GetInt32OfTInt(left);
            GateRef intRight = builder_.GetInt32OfTInt(right);
            builder_.Branch(builder_.Int32LessThan(intLeft, intRight), &leftLessRight, &leftGreaterEqRight);
        }
    }
    builder_.Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
        builder_.Bind(&leftIsNumber);
        {
            Label rightIsNumber(&builder_);
            builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
            builder_.Bind(&rightIsNumber);
            {
                // fast path
                DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                Label leftIsInt1(&builder_);
                Label leftNotInt1(&builder_);
                Label exit1(&builder_);
                Label exit2(&builder_);
                Label rightIsInt1(&builder_);
                Label rightNotInt1(&builder_);
                builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                builder_.Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&leftNotInt1);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&exit1);
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                builder_.Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&rightNotInt1);
                {
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&exit2);
                builder_.Branch(builder_.DoubleLessThan(*doubleLeft, *doubleRight), &leftLessRight,
                                &leftGreaterEqRight);
            }
        }
    }
    builder_.Bind(&leftLessRight);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&leftGreaterEqRight);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::LessEq(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label leftIsInt(&builder_);
    Label leftOrRightNotInt(&builder_);
    Label leftLessEqRight(&builder_);
    Label leftGreaterRight(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftOrRightNotInt);
    builder_.Bind(&leftIsInt);
    {
        Label rightIsInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftOrRightNotInt);
        builder_.Bind(&rightIsInt);
        {
            GateRef intLeft = builder_.GetInt32OfTInt(left);
            GateRef intRight = builder_.GetInt32OfTInt(right);
            builder_.Branch(builder_.Int32LessThanOrEqual(intLeft, intRight), &leftLessEqRight, &leftGreaterRight);
        }
    }
    builder_.Bind(&leftOrRightNotInt);
    {
        Label leftIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &exit);
        builder_.Bind(&leftIsNumber);
        {
            Label rightIsNumber(&builder_);
            builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
            builder_.Bind(&rightIsNumber);
            {
                // fast path
                DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
                Label leftIsInt1(&builder_);
                Label leftNotInt1(&builder_);
                Label exit1(&builder_);
                Label exit2(&builder_);
                Label rightIsInt1(&builder_);
                Label rightNotInt1(&builder_);
                builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                builder_.Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&leftNotInt1);
                {
                    doubleLeft = builder_.GetDoubleOfTDouble(left);
                    builder_.Jump(&exit1);
                }
                builder_.Bind(&exit1);
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
                builder_.Bind(&rightIsInt1);
                {
                    doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&rightNotInt1);
                {
                    doubleRight = builder_.GetDoubleOfTDouble(right);
                    builder_.Jump(&exit2);
                }
                builder_.Bind(&exit2);
                builder_.Branch(builder_.DoubleLessThanOrEqual(*doubleLeft, *doubleRight), &leftLessEqRight,
                                &leftGreaterRight);
            }
        }
    }
    builder_.Bind(&leftLessEqRight);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&leftGreaterRight);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::FastDiv(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    Label leftIsNumber(&builder_);
    Label leftNotNumberOrRightNotNumber(&builder_);
    Label leftIsNumberAndRightIsNumber(&builder_);
    Label leftIsDoubleAndRightIsDouble(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    builder_.Bind(&leftIsNumber);
    {
        Label rightIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        builder_.Bind(&rightIsNumber);
        {
            Label leftIsInt(&builder_);
            Label leftNotInt(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotInt);
            builder_.Bind(&leftIsInt);
            {
                doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                builder_.Jump(&leftIsNumberAndRightIsNumber);
            }
            builder_.Bind(&leftNotInt);
            {
                doubleLeft = builder_.GetDoubleOfTDouble(left);
                builder_.Jump(&leftIsNumberAndRightIsNumber);
            }
        }
    }
    builder_.Bind(&leftNotNumberOrRightNotNumber);
    builder_.Jump(&exit);
    builder_.Bind(&leftIsNumberAndRightIsNumber);
    {
        Label rightIsInt(&builder_);
        Label rightNotInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightNotInt);
        builder_.Bind(&rightIsInt);
        {
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            builder_.Jump(&leftIsDoubleAndRightIsDouble);
        }
        builder_.Bind(&rightNotInt);
        {
            doubleRight = builder_.GetDoubleOfTDouble(right);
            builder_.Jump(&leftIsDoubleAndRightIsDouble);
        }
    }
    builder_.Bind(&leftIsDoubleAndRightIsDouble);
    {
        Label rightIsZero(&builder_);
        Label rightNotZero(&builder_);
        builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZero, &rightNotZero);
        builder_.Bind(&rightIsZero);
        {
            Label leftIsZero(&builder_);
            Label leftNotZero(&builder_);
            Label leftIsZeroOrNan(&builder_);
            Label leftNotZeroAndNotNan(&builder_);
            builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZero, &leftNotZero);
            builder_.Bind(&leftIsZero);
            {
                builder_.Jump(&leftIsZeroOrNan);
            }
            builder_.Bind(&leftNotZero);
            {
                Label leftIsNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
                builder_.Bind(&leftIsNan);
                {
                    builder_.Jump(&leftIsZeroOrNan);
                }
            }
            builder_.Bind(&leftIsZeroOrNan);
            {
                result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&leftNotZeroAndNotNan);
            {
                GateRef intLeftTmp = builder_.CastDoubleToInt64(*doubleLeft);
                GateRef intRightTmp = builder_.CastDoubleToInt64(*doubleRight);
                GateRef flagBit = builder_.Int64And(builder_.Int64Xor(intLeftTmp, intRightTmp),
                                                    builder_.Int64(base::DOUBLE_SIGN_MASK));
                GateRef tmpResult =
                    builder_.Int64Xor(flagBit, builder_.CastDoubleToInt64(builder_.Double(base::POSITIVE_INFINITY)));
                result = DoubleToTaggedDoublePtr(builder_.CastInt64ToFloat64(tmpResult));
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&rightNotZero);
        {
            result = DoubleToTaggedDoublePtr(BinaryOp<OpCode::FDIV, MachineType::F64>(*doubleLeft, *doubleRight));
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::DivNumbers(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    Label leftIsNumberAndRightIsNumber(&builder_);
    Label leftIsDoubleAndRightIsDouble(&builder_);
    Label exit(&builder_);

    Label leftIsInt(&builder_);
    Label leftNotInt(&builder_);
    builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotInt);
    builder_.Bind(&leftIsInt);
    {
        doubleLeft = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
        builder_.Jump(&leftIsNumberAndRightIsNumber);
    }
    builder_.Bind(&leftNotInt);
    {
        doubleLeft = builder_.GetDoubleOfTDouble(left);
        builder_.Jump(&leftIsNumberAndRightIsNumber);
    }
    builder_.Bind(&leftIsNumberAndRightIsNumber);
    {
        Label rightIsInt(&builder_);
        Label rightNotInt(&builder_);
        builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightNotInt);
        builder_.Bind(&rightIsInt);
        {
            doubleRight = ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
            builder_.Jump(&leftIsDoubleAndRightIsDouble);
        }
        builder_.Bind(&rightNotInt);
        {
            doubleRight = builder_.GetDoubleOfTDouble(right);
            builder_.Jump(&leftIsDoubleAndRightIsDouble);
        }
    }
    builder_.Bind(&leftIsDoubleAndRightIsDouble);
    {
        Label rightIsZero(&builder_);
        Label rightNotZero(&builder_);
        builder_.Branch(builder_.Equal(*doubleRight, builder_.Double(0.0)), &rightIsZero, &rightNotZero);
        builder_.Bind(&rightIsZero);
        {
            Label leftIsZero(&builder_);
            Label leftNotZero(&builder_);
            Label leftIsZeroOrNan(&builder_);
            Label leftNotZeroAndNotNan(&builder_);
            builder_.Branch(builder_.Equal(*doubleLeft, builder_.Double(0.0)), &leftIsZero, &leftNotZero);
            builder_.Bind(&leftIsZero);
            {
                builder_.Jump(&leftIsZeroOrNan);
            }
            builder_.Bind(&leftNotZero);
            {
                Label leftIsNan(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
                builder_.Bind(&leftIsNan);
                {
                    builder_.Jump(&leftIsZeroOrNan);
                }
            }
            builder_.Bind(&leftIsZeroOrNan);
            {
                result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
                builder_.Jump(&exit);
            }
            builder_.Bind(&leftNotZeroAndNotNan);
            {
                GateRef intLeftTmp = builder_.CastDoubleToInt64(*doubleLeft);
                GateRef intRightTmp = builder_.CastDoubleToInt64(*doubleRight);
                GateRef flagBit = builder_.Int64And(builder_.Int64Xor(intLeftTmp, intRightTmp),
                                                    builder_.Int64(base::DOUBLE_SIGN_MASK));
                GateRef tmpResult =
                    builder_.Int64Xor(flagBit, builder_.CastDoubleToInt64(builder_.Double(base::POSITIVE_INFINITY)));
                result = DoubleToTaggedDoublePtr(builder_.CastInt64ToFloat64(tmpResult));
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&rightNotZero);
        {
            result = DoubleToTaggedDoublePtr(BinaryOp<OpCode::FDIV, MachineType::F64>(*doubleLeft, *doubleRight));
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef TypeLowering::FastEqual(GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label leftEqRight(&builder_);
    Label leftNotEqRight(&builder_);
    Label exit(&builder_);

    builder_.Branch(builder_.Equal(left, right), &leftEqRight, &leftNotEqRight);
    builder_.Bind(&leftEqRight);
    {
        Label leftIsDouble(&builder_);
        Label leftNotDoubleOrLeftNotNan(&builder_);
        builder_.Branch(builder_.TaggedIsDouble(left), &leftIsDouble, &leftNotDoubleOrLeftNotNan);
        builder_.Bind(&leftIsDouble);
        {
            GateRef doubleLeft = builder_.GetDoubleOfTDouble(left);
            Label leftIsNan(&builder_);
            builder_.Branch(builder_.DoubleIsNAN(doubleLeft), &leftIsNan, &leftNotDoubleOrLeftNotNan);
            builder_.Bind(&leftIsNan);
            {
                result = builder_.TaggedFalse();
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&leftNotDoubleOrLeftNotNan);
        {
            result = builder_.TaggedTrue();
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&leftNotEqRight);
    {
        Label leftIsNumber(&builder_);
        Label leftNotNumberOrLeftNotIntOrRightNotInt(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrLeftNotIntOrRightNotInt);
        builder_.Bind(&leftIsNumber);
        {
            Label leftIsInt(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
            {
                builder_.Bind(&leftIsInt);
                {
                    Label rightIsInt(&builder_);
                    builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
                    builder_.Bind(&rightIsInt);
                    {
                        result = builder_.TaggedFalse();
                        builder_.Jump(&exit);
                    }
                }
            }
        }
        builder_.Bind(&leftNotNumberOrLeftNotIntOrRightNotInt);
        {
            Label rightIsUndefinedOrNull(&builder_);
            Label leftOrRightNotUndefinedOrNull(&builder_);
            builder_.Branch(builder_.TaggedIsUndefinedOrNull(right), &rightIsUndefinedOrNull,
                            &leftOrRightNotUndefinedOrNull);
            builder_.Bind(&rightIsUndefinedOrNull);
            {
                Label leftIsHeapObject(&builder_);
                Label leftNotHeapObject(&builder_);
                builder_.Branch(builder_.TaggedIsHeapObject(left), &leftIsHeapObject, &leftNotHeapObject);
                builder_.Bind(&leftIsHeapObject);
                {
                    result = builder_.TaggedFalse();
                    builder_.Jump(&exit);
                }
                builder_.Bind(&leftNotHeapObject);
                {
                    Label leftIsUndefinedOrNull(&builder_);
                    builder_.Branch(builder_.TaggedIsUndefinedOrNull(left), &leftIsUndefinedOrNull,
                                    &leftOrRightNotUndefinedOrNull);
                    builder_.Bind(&leftIsUndefinedOrNull);
                    {
                        result = builder_.TaggedTrue();
                        builder_.Jump(&exit);
                    }
                }
            }
            builder_.Bind(&leftOrRightNotUndefinedOrNull);
            {
                Label leftIsBoolean(&builder_);
                Label leftNotBooleanOrRightNotSpecial(&builder_);
                builder_.Branch(builder_.TaggedIsBoolean(left), &leftIsBoolean, &leftNotBooleanOrRightNotSpecial);
                builder_.Bind(&leftIsBoolean);
                {
                    Label rightIsSpecial(&builder_);
                    builder_.Branch(builder_.TaggedIsSpecial(right), &rightIsSpecial, &leftNotBooleanOrRightNotSpecial);
                    builder_.Bind(&rightIsSpecial);
                    {
                        result = builder_.TaggedFalse();
                        builder_.Jump(&exit);
                    }
                }
                builder_.Bind(&leftNotBooleanOrRightNotSpecial);
                {
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void TypeLowering::LowerDeoptimize(GateRef gate)
{
    ArgumentAccessor argAcc(circuit_);
    auto glue = argAcc.GetCommonArgGate(CommonArgIdx::GLUE);
    ASSERT(acc_.GetStateCount(gate) == 1);
    GateRef stateIn = acc_.GetState(gate, 0);
    GateRef guard = acc_.GetDep(gate);
    GateRef frameState = acc_.GetValueIn(guard, 0);
    size_t numValueIn = acc_.GetBitField(frameState);
    size_t accIndex = numValueIn - 2; // 2: acc valueIn index
    size_t pcIndex = numValueIn - 1;
    std::vector<GateRef> vec;
    for (size_t i = 0; i < accIndex; i++) {
        GateRef vreg = acc_.GetValueIn(frameState, i);
        if (acc_.GetBitField(vreg) != JSTaggedValue::VALUE_HOLE) {
            vec.emplace_back(builder_.Int32(i));
            vec.emplace_back(vreg);
        }
    }
    GateRef acc = acc_.GetValueIn(frameState, accIndex);
    GateRef pc = acc_.GetValueIn(frameState, pcIndex);
    vec.emplace_back(builder_.Int32(static_cast<int>(SpecVregIndex::ACC_INDEX)));
    vec.emplace_back(acc);
    vec.emplace_back(builder_.Int32(static_cast<int>(SpecVregIndex::PC_INDEX)));
    vec.emplace_back(pc);
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(DeoptHandlerAsm));
    GateRef target = builder_.IntPtr(RTSTUB_ID(DeoptHandlerAsm));
    GateRef newGate = builder_.Call(cs, glue, target, guard, vec);
    builder_.Return(stateIn, acc_.GetDep(guard), newGate);
    acc_.DeleteGate(gate);
}
}  // namespace panda::ecmascript