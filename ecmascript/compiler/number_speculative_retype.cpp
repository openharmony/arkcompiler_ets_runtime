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

#include "ecmascript/compiler/number_speculative_retype.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"

namespace panda::ecmascript::kungfu {
GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, GateType gateType)
{
    TypeInfo& type = typeInfos_[acc_.GetId(gate)];
    TypeInfo old = type;
    if (gateType.IsIntType()) {
        type = TypeInfo::INT32;
    } else if (gateType.IsDoubleType()) {
        type = TypeInfo::FLOAT64;
    } else if (gateType.IsBooleanType()) {
        type = TypeInfo::INT1;
    } else {
        type = TypeInfo::TAGGED;
    }
    return old == type ? Circuit::NullGate() : gate;
}

GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, PGOSampleType pgoType)
{
    TypeInfo& type = typeInfos_[acc_.GetId(gate)];
    TypeInfo old = type;
    if (pgoType.IsInt()) {
        type = TypeInfo::INT32;
    } else {
        type = TypeInfo::FLOAT64;
    }
    return old == type ? Circuit::NullGate() : gate;
}

GateRef NumberSpeculativeRetype::VisitGate(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::TYPED_BINARY_OP:
            return VisitTypedBinaryOp(gate);
        case OpCode::TYPED_UNARY_OP:
            return VisitTypedUnaryOp(gate);
        case OpCode::TYPED_CONDITION_JUMP:
            return VisitTypedConditionJump(gate);
        case OpCode::INT32_OVERFLOW_CHECK:
            return VisitOverflowCheck(gate);
        case OpCode::INDEX_CHECK:
            return VisitIndexCheck(gate);
        case OpCode::LOAD_ELEMENT:
            return VisitLoadElement(gate);
        case OpCode::STORE_ELEMENT:
            return VisitStoreElement(gate);
        case OpCode::VALUE_SELECTOR:
            return VisitPhi(gate);
        case OpCode::CONSTANT:
            return VisitConstant(gate);
        case OpCode::TYPED_CALL_BUILTIN:
            return VisitCallBuiltins(gate);
        case OpCode::STORE_PROPERTY:
            return VisitStoreProperty(gate);
        default:
            return VisitOthers(gate);
    }
}

GateRef NumberSpeculativeRetype::VisitTypedBinaryOp(GateRef gate)
{
    if (acc_.HasNumberType(gate)) {
        return VisitNumberBinaryOp(gate);
    } else {
        [[maybe_unused]] GateRef left = acc_.GetValueIn(gate, 0);
        [[maybe_unused]] GateRef right = acc_.GetValueIn(gate, 1);
        ASSERT((acc_.IsConstantUndefined(left)) || (acc_.IsConstantUndefined(right)));
        ASSERT(acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_STRICTEQ);
        return VisitUndefinedStrictEq(gate);
    }
}

GateRef NumberSpeculativeRetype::VisitUndefinedStrictEq(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BooleanType());
    }
    if (IsConvert()) {
        GateRef left = acc_.GetValueIn(gate, 0);
        GateRef right = acc_.GetValueIn(gate, 1);
        acc_.ReplaceValueIn(gate, ConvertToTagged(left), 0);
        acc_.ReplaceValueIn(gate, ConvertToTagged(right), 1);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitConstant(GateRef gate)
{
    if (IsRetype()) {
        GateType gateType = acc_.GetGateType(gate);
        if (acc_.IsConstantNumber(gate)) {
            return SetOutputType(gate, gateType);
        } else {
            return SetOutputType(gate, GateType::AnyType());
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitPhi(GateRef gate)
{
    size_t valueNum = acc_.GetNumValueIn(gate);
    if (IsRetype()) {
        TypeInfo tempType = TypeInfo::NONE;
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            TypeInfo inputInfo = typeInfos_[acc_.GetId(input)];
            if (tempType == TypeInfo::NONE) {
                tempType = inputInfo;
            } else if ((tempType != inputInfo) && (inputInfo != TypeInfo::NONE)) {
                tempType = TypeInfo::TAGGED;
                break;
            }
        }
        TypeInfo& typeInfo = typeInfos_[acc_.GetId(gate)];
        if (typeInfo != tempType) {
            typeInfo = tempType;
            return gate;
        }
    }

    if (IsConvert()) {
        TypeInfo output = typeInfos_[acc_.GetId(gate)];
        if (output == TypeInfo::TAGGED) {
            return VisitOthers(gate);
        }
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberBinaryOp(GateRef gate)
{
    TypedBinOp op = acc_.GetTypedBinaryOp(gate);
    switch (op) {
        case TypedBinOp::TYPED_ADD:
        case TypedBinOp::TYPED_SUB:
        case TypedBinOp::TYPED_MUL: {
            return VisitNumberCalculate(gate);
        }
        case TypedBinOp::TYPED_LESS:
        case TypedBinOp::TYPED_LESSEQ:
        case TypedBinOp::TYPED_GREATER:
        case TypedBinOp::TYPED_GREATEREQ:
        case TypedBinOp::TYPED_EQ:
        case TypedBinOp::TYPED_NOTEQ: {
            return VisitNumberCompare(gate);
        }
        case TypedBinOp::TYPED_SHL:
        case TypedBinOp::TYPED_SHR:
        case TypedBinOp::TYPED_ASHR:
        case TypedBinOp::TYPED_AND:
        case TypedBinOp::TYPED_OR:
        case TypedBinOp::TYPED_XOR: {
            return VisitNumberShiftAndLogical(gate);
        }
        case TypedBinOp::TYPED_DIV: {
            return VisitNumberDiv(gate);
        }
        default:
            return VisitNumberRelated(gate);
    }
}

GateRef NumberSpeculativeRetype::VisitTypedUnaryOp(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedUnOp Op = acc_.GetTypedUnAccessor(gate).GetTypedUnOp();
    switch (Op) {
        case TypedUnOp::TYPED_INC:
        case TypedUnOp::TYPED_DEC:
        case TypedUnOp::TYPED_NEG:
            return VisitNumberMonocular(gate);
        case TypedUnOp::TYPED_NOT:
            return VisitNumberNot(gate);
        default:
            return VisitNumberRelated(gate);
    }
}

GateRef NumberSpeculativeRetype::VisitTypedConditionJump(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    auto type = accessor.GetTypeValue();
    if (type.IsBooleanType()) {
        return VisitBooleanJump(gate);
    } else {
        UNREACHABLE();
        return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::VisitNumberCalculate(GateRef gate)
{
    if (IsRetype()) {
        PGOSampleType sampleType = acc_.GetTypedBinaryType(gate);
        if (sampleType.IsNumber()) {
            return SetOutputType(gate, sampleType);
        } else {
            GateType gateType = acc_.GetGateType(gate);
            GateType resType = gateType.IsIntType() ? GateType::IntType() : GateType::DoubleType();
            return SetOutputType(gate, resType);
        }
    } else if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        ConvertForBinaryOp(gate);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberCompare(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BooleanType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        ConvertForCompareOp(gate);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberShiftAndLogical(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateType leftType = acc_.GetLeftType(gate);
        GateType rightType = acc_.GetRightType(gate);
        ConvertForIntOperator(gate, leftType, rightType);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberMonocular(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType type = accessor.GetTypeValue();
    ASSERT(type.IsNumberType());
    if (type.IsIntType()) {
        return VisitIntMonocular(gate);
    } else {
        return VisitDoubleMonocular(gate);
    }
}

GateRef NumberSpeculativeRetype::VisitIntMonocular(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }

    if (IsConvert()) {
        GateRef value = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value, GateType::IntType()), 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitDoubleMonocular(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::DoubleType());
    }

    if (IsConvert()) {
        TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
        GateRef value = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(value, accessor.GetTypeValue()), 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberNot(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType valueType = accessor.GetTypeValue();
    ASSERT(valueType.IsNumberType());
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value, valueType), 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitBooleanJump(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        GateRef value = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToBool(value, GateType::BooleanType()), 0);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberRelated(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::NumberType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            GateType inputType = acc_.GetGateType(input);
            if (inputType.IsNumberType() || inputType.IsBooleanType()) {
                acc_.ReplaceValueIn(gate, CheckAndConvertToTagged(input, inputType), i);
            }
        }
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitCallBuiltins(GateRef gate)
{
    auto valuesIn = acc_.GetNumValueIn(gate);
    auto idGate = acc_.GetValueIn(gate, valuesIn - 1);
    auto id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(idGate));
    if (id != BUILTINS_STUB_ID(SQRT)) {
        return VisitOthers(gate);
    }

    if (IsRetype()) {
        // Sqrt output is double
        return SetOutputType(gate, GateType::DoubleType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        acc_.ReplaceValueIn(gate, ConvertToTagged(idGate), valuesIn - 1);
        for (size_t i = 0; i < valuesIn - 1; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(input, GateType::NumberType()), i);
            acc_.ReplaceStateIn(gate, builder_.GetState());
            acc_.ReplaceDependIn(gate, builder_.GetDepend());
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitOthers(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::CheckAndConvertToBool(GateRef gate, [[maybe_unused]]GateType gateType)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT1:
            return gate;
        case TypeInfo::TAGGED: {
            ASSERT(gateType.IsBooleanType());
            return builder_.ConvertTaggedBooleanToBool(gate);
        }
        default: {
            UNREACHABLE();
            return Circuit::NullGate();
        }
    }
}

void NumberSpeculativeRetype::ConvertForBinaryOp(GateRef gate)
{
    PGOSampleType sampleType = acc_.GetTypedBinaryType(gate);
    if (sampleType.IsNumber()) {
        if (sampleType.IsInt()) {
            GateType leftType = GateType::IntType();
            GateType rightType = GateType::IntType();
            ConvertForIntOperator(gate, leftType, rightType);
        } else {
            GateType leftType = GateType::NumberType();
            GateType rightType = GateType::NumberType();
            if (sampleType.IsIntOverFlow()) {
                leftType = GateType::IntType();
                rightType = GateType::IntType();
            } else if (sampleType.IsDouble()) {
                leftType = GateType::DoubleType();
                rightType = GateType::DoubleType();
            }
            ConvertForDoubleOperator(gate, leftType, rightType);
        }
    } else {
        GateType gateType = acc_.GetGateType(gate);
        GateType leftType = acc_.GetLeftType(gate);
        GateType rightType = acc_.GetRightType(gate);
        if (gateType.IsIntType()) {
            ConvertForIntOperator(gate, leftType, rightType);
        } else {
            ConvertForDoubleOperator(gate, leftType, rightType);
        }
    }
}

void NumberSpeculativeRetype::ConvertForCompareOp(GateRef gate)
{
    PGOSampleType sampleType = acc_.GetTypedBinaryType(gate);
    if (sampleType.IsNumber()) {
        if (sampleType.IsInt()) {
            GateType leftType = GateType::IntType();
            GateType rightType = GateType::IntType();
            ConvertForIntOperator(gate, leftType, rightType);
        } else {
            GateType leftType = GateType::NumberType();
            GateType rightType = GateType::NumberType();
            ConvertForDoubleOperator(gate, leftType, rightType);
        }
    } else {
        GateType leftType = acc_.GetLeftType(gate);
        GateType rightType = acc_.GetRightType(gate);
        if (leftType.IsIntType() && rightType.IsIntType()) {
            ConvertForIntOperator(gate, leftType, rightType);
        } else {
            ConvertForDoubleOperator(gate, leftType, rightType);
        }
    }
}

void NumberSpeculativeRetype::ConvertForIntOperator(GateRef gate, GateType leftType, GateType rightType)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);

    acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(left, leftType), 0);
    acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(right, rightType), 1);
}

void NumberSpeculativeRetype::ConvertForDoubleOperator(GateRef gate, GateType leftType, GateType rightType)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);

    acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(left, leftType), 0);
    acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(right, rightType), 1);
}

GateRef NumberSpeculativeRetype::CheckAndConvertToInt32(GateRef gate, GateType gateType)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT32:
            return gate;
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToInt32(gate);
        case TypeInfo::TAGGED: {
            ASSERT(gateType.IsNumberType());
            if (gateType.IsIntType()) {
                return builder_.CheckTaggedIntAndConvertToInt32(gate);
            } else if (gateType.IsDoubleType()) {
                return builder_.CheckTaggedDoubleAndConvertToInt32(gate);
            } else {
                return builder_.CheckTaggedNumberAndConvertToInt32(gate);
            }
        }
        default: {
            UNREACHABLE();
            return Circuit::NullGate();
        }
    }
}

GateRef NumberSpeculativeRetype::CheckAndConvertToFloat64(GateRef gate, GateType gateType)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToFloat64(gate);
        case TypeInfo::FLOAT64:
            return gate;
        case TypeInfo::TAGGED: {
            ASSERT(gateType.IsNumberType());
            if (gateType.IsIntType()) {
                return builder_.CheckTaggedIntAndConvertToFloat64(gate);
            } else if (gateType.IsDoubleType()) {
                return builder_.CheckTaggedDoubleAndConvertToFloat64(gate);
            } else {
                return builder_.CheckTaggedNumberAndConvertToFloat64(gate);
            }
        }
        default: {
            UNREACHABLE();
            return Circuit::NullGate();
        }
    }
}

GateRef NumberSpeculativeRetype::CheckAndConvertToTagged(GateRef gate, GateType gateType)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT1:
            return builder_.ConvertBoolToTaggedBoolean(gate);
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToTaggedInt(gate);
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToTaggedDouble(gate);
        case TypeInfo::TAGGED: {
            ASSERT(gateType.IsNumberType() || gateType.IsBooleanType());
            builder_.TryPrimitiveTypeCheck(gateType, gate);
            return gate;
        }
        default:
            UNREACHABLE();
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::ConvertToTagged(GateRef gate)
{
    TypeInfo output = typeInfos_[acc_.GetId(gate)];
    switch (output) {
        case TypeInfo::INT1:
            return builder_.ConvertBoolToTaggedBoolean(gate);
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToTaggedInt(gate);
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToTaggedDouble(gate);
        case TypeInfo::TAGGED: {
            return gate;
        }
        default:
            UNREACHABLE();
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::VisitOverflowCheck(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        ASSERT(acc_.GetGateType(value).IsIntType());
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value, GateType::IntType()), 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitIndexCheck(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef receiver = acc_.GetValueIn(gate, 0);
        GateRef index = acc_.GetValueIn(gate, 1);
        GateType receiverType = acc_.GetGateType(receiver);
        GateType indexType = acc_.GetGateType(index);
        if (receiverType.IsNumberType()) {
            // IndexCheck receive length at first value input.
            acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(receiver, receiverType), 0);
        }
        if (indexType.IsNumberType()) {
            acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1);
        }
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadElement(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef index = acc_.GetValueIn(gate, 1);
        GateType indexType = acc_.GetGateType(index);
        if (indexType.IsNumberType()) {
            acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1);
        }
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitStoreElement(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef index = acc_.GetValueIn(gate, 1);
        GateType indexType = acc_.GetGateType(index);
        GateRef value = acc_.GetValueIn(gate, 2);
        if (indexType.IsNumberType()) {
            acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1);
        }
        acc_.ReplaceValueIn(gate, ConvertToTagged(value), 2);   // 2: index of value to be stored.
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitStoreProperty(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        GateRef value = acc_.GetValueIn(gate, 2);
        TypeInfo valueType = typeInfos_[acc_.GetId(value)];
        switch (valueType) {
            case TypeInfo::INT1:
            case TypeInfo::INT32:
            case TypeInfo::FLOAT64:
                builder_.StorePropertyNoBarrier(gate);
                break;
            default:
                break;
        }
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberDiv(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::DoubleType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        ConvertForDoubleOperator(gate, acc_.GetLeftType(gate), acc_.GetRightType(gate));
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

void NumberSpeculativeRetype::Run()
{
    // visit gate in RPO, propagate use infos and
    // reset the machine type of number operator gate and related phi,
    // if some tagged phi is used as native value, change it to native phi.
    state_ = State::Retype;
    VisitGraph();
    state_ = State::Convert;
    VisitGraph();
}

}  // namespace panda::ecmascript::kungfu
