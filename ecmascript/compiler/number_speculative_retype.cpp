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
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"
#include <cstdint>

namespace panda::ecmascript::kungfu {
GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, GateType gateType)
{
    TypeInfo type = GetOutputTypeInfo(gate);
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
    SetOutputTypeInfo(gate, type);
    return old == type ? Circuit::NullGate() : gate;
}

GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, PGOSampleType pgoType)
{
    TypeInfo type = GetOutputTypeInfo(gate);
    TypeInfo old = type;
    if (pgoType.IsInt()) {
        type = TypeInfo::INT32;
    } else {
        type = TypeInfo::FLOAT64;
    }
    SetOutputTypeInfo(gate, type);
    return old == type ? Circuit::NullGate() : gate;
}

GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, Representation rep)
{
    TypeInfo type = GetOutputTypeInfo(gate);
    TypeInfo old = type;
    if (rep == Representation::INT) {
        type = TypeInfo::INT32;
    } else if (rep == Representation::DOUBLE) {
        type = TypeInfo::FLOAT64;
    } else {
        type = TypeInfo::TAGGED;
    }
    SetOutputTypeInfo(gate, type);
    return old == type ? Circuit::NullGate() : gate;
}

GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, TypeInfo type)
{
    TypeInfo old = GetOutputTypeInfo(gate);
    SetOutputTypeInfo(gate, type);
    return old == type ? Circuit::NullGate() : gate;
}
void NumberSpeculativeRetype::setState(NumberSpeculativeRetype::State state)
{
    state_ = state;
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
        case OpCode::RANGE_CHECK_PREDICATE:
            return VisitRangeCheckPredicate(gate);
        case OpCode::INDEX_CHECK:
            return VisitIndexCheck(gate);
        case OpCode::LOAD_ARRAY_LENGTH:
        case OpCode::LOAD_TYPED_ARRAY_LENGTH:
            return VisitLoadArrayLength(gate);
        case OpCode::LOAD_STRING_LENGTH:
            return VisitLoadStringLength(gate);
        case OpCode::LOAD_ELEMENT:
            return VisitLoadElement(gate);
        case OpCode::STORE_ELEMENT:
            return VisitStoreElement(gate);
        case OpCode::STORE_PROPERTY:
            return VisitStoreProperty(gate);
        case OpCode::LOAD_PROPERTY:
            return VisitLoadProperty(gate);
        case OpCode::MONO_LOAD_PROPERTY_ON_PROTO:
            return VisitMonoLoadPropertyOnProto(gate);
        case OpCode::MONO_CALL_GETTER_ON_PROTO:
            return VisitMonoCallGetterOnProto(gate);
        case OpCode::MONO_STORE_PROPERTY:
        case OpCode::MONO_STORE_PROPERTY_LOOK_UP_PROTO:
            return VisitMonoStoreProperty(gate);
        case OpCode::VALUE_SELECTOR:
            return VisitPhi(gate);
        case OpCode::CONSTANT:
            return VisitConstant(gate);
        case OpCode::TYPED_CALL_BUILTIN:
            return VisitCallBuiltins(gate);
        case OpCode::TYPE_CONVERT:
            return VisitTypeConvert(gate);
        case OpCode::FRAME_STATE:
            return VisitFrameState(gate);
        case OpCode::CALL_GETTER:
        case OpCode::CALL_SETTER:
        case OpCode::CONSTRUCT:
        case OpCode::TYPEDCALL:
        case OpCode::TYPEDFASTCALL:
        case OpCode::OBJECT_TYPE_CHECK:
            return VisitWithConstantValue(gate, PROPERTY_LOOKUP_RESULT_INDEX);
        case OpCode::LOOP_EXIT_VALUE:
        case OpCode::RANGE_GUARD:
            return VisitIntermediateValue(gate);
        case OpCode::NUMBER_TO_STRING:
            return VisitNumberToString(gate);
        case OpCode::JS_BYTECODE:
        case OpCode::PRIMITIVE_TYPE_CHECK:
        case OpCode::STABLE_ARRAY_CHECK:
        case OpCode::TYPED_ARRAY_CHECK:
        case OpCode::TYPED_CALLTARGETCHECK_OP:
        case OpCode::TYPED_CALL_CHECK:
        case OpCode::HEAP_ALLOC:
        case OpCode::TYPED_NEW_ALLOCATE_THIS:
        case OpCode::TYPED_SUPER_ALLOCATE_THIS:
        case OpCode::GET_SUPER_CONSTRUCTOR:
        case OpCode::ARG:
        case OpCode::RETURN:
        case OpCode::FRAME_ARGS:
        case OpCode::SAVE_REGISTER:
        case OpCode::RESTORE_REGISTER:
        case OpCode::LOAD_CONST_OFFSET:
        case OpCode::STORE_CONST_OFFSET:
        case OpCode::LEX_VAR_IS_HOLE_CHECK:
        case OpCode::TYPE_OF_CHECK:
        case OpCode::ARRAY_CONSTRUCTOR:
        case OpCode::OBJECT_CONSTRUCTOR:
        case OpCode::LD_LOCAL_MODULE_VAR:
        case OpCode::STORE_MODULE_VAR:
        case OpCode::STRING_FROM_SINGLE_CHAR_CODE:
        case OpCode::ORDINARY_HAS_INSTANCE:
        case OpCode::ECMA_STRING_CHECK:
        case OpCode::CREATE_ARGUMENTS:
        case OpCode::TAGGED_TO_INT64:
        case OpCode::MATH_COS:
        case OpCode::MATH_SIN:
            return VisitOthers(gate);
        default:
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::VisitTypedBinaryOp(GateRef gate)
{
    if (acc_.HasStringType(gate)) {
        return VisitStringBinaryOp(gate);
    }

    if (acc_.GetTypedBinaryOp(gate) != TypedBinOp::TYPED_STRICTEQ &&
        acc_.GetTypedBinaryOp(gate) != TypedBinOp::TYPED_STRICTNOTEQ &&
        acc_.GetTypedBinaryOp(gate) != TypedBinOp::TYPED_EQ &&
        acc_.GetTypedBinaryOp(gate) != TypedBinOp::TYPED_NOTEQ) {
        if (acc_.HasPrimitiveNumberType(gate)) {
            return VisitNumberBinaryOp(gate);
        }
    }

    return VisitEqualCompareOrNotEqualCompare(gate);
}

GateRef NumberSpeculativeRetype::VisitEqualCompareOrNotEqualCompare(GateRef gate)
{
    if (acc_.HasNumberType(gate)) {
        return VisitNumberBinaryOp(gate);
    } else {
        return VisitUndefinedEqualCompareOrUndefinedNotEqualCompare(gate);
    }
}

GateRef NumberSpeculativeRetype::VisitUndefinedEqualCompareOrUndefinedNotEqualCompare(GateRef gate)
{
    ASSERT(acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_STRICTEQ ||
           acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_STRICTNOTEQ ||
           acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_EQ ||
           acc_.GetTypedBinaryOp(gate) == TypedBinOp::TYPED_NOTEQ);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    ASSERT((acc_.IsUndefinedOrNullOrHole(left)) || (acc_.IsUndefinedOrNullOrHole(right)));
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BooleanType());
    }
    if (IsConvert()) {
        acc_.ReplaceValueIn(gate, ConvertToTagged(left), 0);
        acc_.ReplaceValueIn(gate, ConvertToTagged(right), 1);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitConstant(GateRef gate)
{
    if (IsRetype()) {
        if (acc_.GetGateType(gate).IsNJSValueType()) {
            auto machineType = acc_.GetMachineType(gate);
            switch (machineType) {
                case MachineType::I1:
                    SetOutputTypeInfo(gate, TypeInfo::INT1);
                    break;
                case MachineType::I32:
                    SetOutputTypeInfo(gate, TypeInfo::INT32);
                    break;
                case MachineType::F64:
                    SetOutputTypeInfo(gate, TypeInfo::FLOAT64);
                    break;
                default:
                    SetOutputTypeInfo(gate, TypeInfo::NONE);
                    break;
            }
        } else {
            return SetOutputType(gate, GateType::AnyType());
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitIntermediateValue(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    TypeInfo valueInfo = GetOutputTypeInfo(value);
    if (IsRetype()) {
        TypeInfo oldType = GetOutputTypeInfo(gate);
        SetOutputTypeInfo(gate, valueInfo);
        return oldType == valueInfo ? Circuit::NullGate() : gate;
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberToString(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::StringType());
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

GateRef NumberSpeculativeRetype::VisitStringBinaryOp(GateRef gate)
{
    TypedBinOp op = acc_.GetTypedBinaryOp(gate);
    switch (op) {
        case TypedBinOp::TYPED_EQ:
            return VisitStringCompare(gate);
        case TypedBinOp::TYPED_ADD:
            return VisitStringAdd(gate);
        default:
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::VisitStringCompare(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BooleanType());
    }

    Environment env(gate, circuit_, &builder_);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    TypeInfo leftInfo = GetOutputTypeInfo(left);
    TypeInfo rightInfo = GetOutputTypeInfo(right);
    if (leftInfo == TypeInfo::CHAR && rightInfo == TypeInfo::CHAR) {
        return Circuit::NullGate();
    }
    if (leftInfo == TypeInfo::CHAR) {
        GateRef rep = builder_.ConvertCharToEcmaString(left);
        acc_.ReplaceValueIn(gate, rep, 0);
    }
    if (rightInfo == TypeInfo::CHAR) {
        GateRef rep =  builder_.ConvertCharToEcmaString(right);
        acc_.ReplaceValueIn(gate, rep, 1);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitStringAdd(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::StringType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            TypeInfo inputInfo = GetOutputTypeInfo(input);
            if (inputInfo == TypeInfo::CHAR) {
                GateRef glue = acc_.GetGlueFromArgList();
                input = builder_.CallStub(glue, gate, CommonStubCSigns::CreateStringBySingleCharCode, { glue, input });
            }
            acc_.ReplaceValueIn(gate, input, i);
        }
    }
    return Circuit::NullGate();
}

TypeInfo NumberSpeculativeRetype::GetOuputForPhi(GateRef gate, bool ignoreConstant)
{
    size_t valueNum = acc_.GetNumValueIn(gate);
    bool hasConstantInput = false;
    TypeInfo tempType = TypeInfo::NONE;
    for (size_t i = 0; i < valueNum; ++i) {
        GateRef input = acc_.GetValueIn(gate, i);
        TypeInfo inputInfo = GetOutputTypeInfo(input);
        if (inputInfo == TypeInfo::NONE) {
            continue;
        }
        if (ignoreConstant && acc_.IsConstantNumber(input)) {
            hasConstantInput = true;
            continue;
        }
        // use less general input as phi output
        if (tempType == TypeInfo::NONE) {
            tempType = inputInfo;
        } else if (tempType != inputInfo) {
            tempType = TypeInfo::TAGGED;
            break;
        }
    }

    if (hasConstantInput && ignoreConstant && tempType == TypeInfo::NONE) {
        return GetOuputForPhi(gate, false);
    }
    return tempType;
}

GateRef NumberSpeculativeRetype::VisitPhi(GateRef gate)
{
    if (IsRetype()) {
        auto tempType = GetOuputForPhi(gate, true);
        TypeInfo typeInfo = GetOutputTypeInfo(gate);
        if (typeInfo != tempType) {
            SetOutputTypeInfo(gate, tempType);
            return gate;
        }
        return Circuit::NullGate();
    }
    ASSERT(IsConvert());
    size_t valueNum = acc_.GetNumValueIn(gate);
    auto merge = acc_.GetState(gate);
    auto dependSelector = acc_.GetDependSelectorFromMerge(merge);
    TypeInfo output = GetOutputTypeInfo(gate);
    for (size_t i = 0; i < valueNum; ++i) {
        GateRef input = acc_.GetValueIn(gate, i);
        if (output == TypeInfo::TAGGED || output == TypeInfo::NONE) {
            input = ConvertToTagged(input);
        } else {
            auto state = acc_.GetState(merge, i);
            auto depend = acc_.GetDep(dependSelector, i);
            Environment env(state, depend, {}, circuit_, &builder_);
            input = ConvertTaggedToNJSValue(input, output);
            acc_.ReplaceStateIn(merge, builder_.GetState(), i);
            acc_.ReplaceDependIn(dependSelector, builder_.GetDepend(), i);
        }
        acc_.ReplaceValueIn(gate, input, i);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::ConvertTaggedToNJSValue(GateRef gate, TypeInfo output)
{
    TypeInfo curOutput = GetOutputTypeInfo(gate);
    if (curOutput != TypeInfo::TAGGED) {
        return gate;
    }
    switch (output) {
        case TypeInfo::INT1:
            return CheckAndConvertToBool(gate, GateType::BooleanType());
        case TypeInfo::INT32:
            return CheckAndConvertToInt32(gate, GateType::NumberType());
        case TypeInfo::FLOAT64:
            return CheckAndConvertToFloat64(gate, GateType::NumberType());
        default:
            LOG_COMPILER(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::VisitNumberBinaryOp(GateRef gate)
{
    TypedBinOp op = acc_.GetTypedBinaryOp(gate);
    switch (op) {
        case TypedBinOp::TYPED_ADD:
        case TypedBinOp::TYPED_SUB:
        case TypedBinOp::TYPED_MUL:
        case TypedBinOp::TYPED_DIV: {
            return VisitNumberCalculate(gate);
        }
        case TypedBinOp::TYPED_LESS:
        case TypedBinOp::TYPED_LESSEQ:
        case TypedBinOp::TYPED_GREATER:
        case TypedBinOp::TYPED_GREATEREQ:
        case TypedBinOp::TYPED_EQ:
        case TypedBinOp::TYPED_NOTEQ:
        case TypedBinOp::TYPED_STRICTEQ:
        case TypedBinOp::TYPED_STRICTNOTEQ: {
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
        case TypedBinOp::TYPED_MOD: {
            return VisitNumberMod(gate);
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
        case TypedUnOp::TYPED_ISFALSE:
        case TypedUnOp::TYPED_ISTRUE:
            return VisitIsTrueOrFalse(gate);
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
        const PGOSampleType *sampleType = acc_.GetTypedBinaryType(gate).GetPGOSampleType();
        if (sampleType->IsNumber()) {
            return SetOutputType(gate, *sampleType);
        } else {
            GateType gateType = acc_.GetGateType(gate);
            GateType resType = gateType.IsIntType() ? GateType::IntType() : GateType::DoubleType();
            return SetOutputType(gate, resType);
        }
    } else if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        ConvertForBinaryOp(gate);
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
        const PGOSampleType *sampleType = acc_.GetTypedBinaryType(gate).GetPGOSampleType();
        if (sampleType->IsNumber()) {
            if (sampleType->IsInt()) {
                leftType = GateType::IntType();
                rightType = GateType::IntType();
            } else {
                leftType = GateType::NumberType();
                rightType = GateType::NumberType();
            }
        }
        ConvertForShiftAndLogicalOperator(gate, leftType, rightType);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberMonocular(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType type = accessor.GetTypeValue();
    ASSERT(type.IsPrimitiveNumberType());
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

GateRef NumberSpeculativeRetype::VisitIsTrueOrFalse(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType valueType = accessor.GetTypeValue();
    ASSERT(valueType.IsPrimitiveNumberType());
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BooleanType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        auto input = CheckAndConvertToBool(value, valueType);
        ResizeAndSetTypeInfo(input, TypeInfo::INT1);
        acc_.ReplaceValueIn(gate, input, 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberNot(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType valueType = accessor.GetTypeValue();
    ASSERT(valueType.IsPrimitiveNumberType());
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate,
            CheckAndConvertToInt32(value, valueType, ConvertSupport::ENABLE, OpType::SHIFT_AND_LOGICAL), 0);
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
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        GateRef input = Circuit::NullGate();
        if (GetOutputTypeInfo(value) == TypeInfo::TAGGED) {
            // use TaggedIsTrue
            input = builder_.ConvertTaggedBooleanToBool(value);
        } else {
            input = CheckAndConvertToBool(value, GateType::BooleanType());
        }
        ResizeAndSetTypeInfo(input, TypeInfo::INT1);
        acc_.ReplaceValueIn(gate, input, 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
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
        }
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitFrameState(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
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

GateRef NumberSpeculativeRetype::VisitWithConstantValue(GateRef gate, size_t ignoreIndex)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            if (i == ignoreIndex) {
                continue;
            }
            GateRef input = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::CheckAndConvertToBool(GateRef gate, GateType gateType)
{
    TypeInfo output = GetOutputTypeInfo(gate);
    switch (output) {
        case TypeInfo::INT1:
            return gate;
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToBool(gate);
        case TypeInfo::UINT32:
            return builder_.ConvertUInt32ToBool(gate);
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToBool(gate);
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            if (gateType.IsBooleanType()) {
                return builder_.CheckTaggedBooleanAndConvertToBool(gate);
            } else if (gateType.IsUndefinedType()) {
                return builder_.CheckUndefinedAndConvertToBool(gate);
            } else if (gateType.IsNullType()) {
                return builder_.CheckNullAndConvertToBool(gate);
            } else {
                ASSERT(gateType.IsNumberType());
                return builder_.CheckTaggedNumberAndConvertToBool(gate);
            }
        }
        default: {
            UNREACHABLE();
            return Circuit::NullGate();
        }
    }
}

void NumberSpeculativeRetype::ConvertForBinaryOp(GateRef gate)
{
    const PGOSampleType *sampleType = acc_.GetTypedBinaryType(gate).GetPGOSampleType();
    if (sampleType->IsNumber()) {
        if (sampleType->IsInt()) {
            GateType leftType = GateType::IntType();
            GateType rightType = GateType::IntType();
            ConvertForIntOperator(gate, leftType, rightType);
        } else {
            GateType leftType = GateType::NumberType();
            GateType rightType = GateType::NumberType();
            if (sampleType->IsIntOverFlow()) {
                leftType = GateType::IntType();
                rightType = GateType::IntType();
            } else if (sampleType->IsDouble()) {
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
    const PGOSampleType *sampleType = acc_.GetTypedBinaryType(gate).GetPGOSampleType();
    if (sampleType->IsNumber()) {
        if (sampleType->IsInt()) {
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

    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
}

void NumberSpeculativeRetype::ConvertForShiftAndLogicalOperator(GateRef gate, GateType leftType, GateType rightType)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateRef cLeft = CheckAndConvertToInt32(left, leftType, ConvertSupport::ENABLE, OpType::SHIFT_AND_LOGICAL);
    GateRef cRight = CheckAndConvertToInt32(right, rightType, ConvertSupport::ENABLE, OpType::SHIFT_AND_LOGICAL);

    acc_.ReplaceValueIn(gate, cLeft, 0);
    acc_.ReplaceValueIn(gate, cRight, 1);

    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
}

void NumberSpeculativeRetype::ConvertForDoubleOperator(GateRef gate, GateType leftType, GateType rightType)
{
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);

    acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(left, leftType), 0);
    acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(right, rightType), 1);

    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
}

GateRef NumberSpeculativeRetype::TryConvertConstant(GateRef gate, bool needInt32)
{
    if (acc_.GetOpCode(gate) != OpCode::CONSTANT) {
        return Circuit::NullGate();
    }

    if (acc_.GetGateType(gate).IsNJSValueType()) {
        MachineType mType = acc_.GetMachineType(gate);
        if (mType == MachineType::I32) {
            int32_t rawValue = acc_.GetInt32FromConstant(gate);
            double value = static_cast<double>(rawValue);
            return needInt32 ? builder_.Int32(rawValue) : builder_.Double(value);
        } else if (mType == MachineType::F64 && !needInt32) {
            double rawValue = acc_.GetFloat64FromConstant(gate);
            return builder_.Double(rawValue);
        } else {
            return Circuit::NullGate();
        }
    }

    JSTaggedValue value(acc_.GetConstantValue(gate));
    if (value.IsInt()) {
        int32_t rawValue = value.GetInt();
        double doubleValue = static_cast<double>(rawValue);
        return needInt32 ? builder_.Int32(rawValue) : builder_.Double(doubleValue);
    } else if (value.IsDouble() && !needInt32) {
        double rawValue = value.GetDouble();
        return builder_.Double(rawValue);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::CheckAndConvertToInt32(GateRef gate, GateType gateType, ConvertSupport support,
                                                        OpType type)
{
    auto result = TryConvertConstant(gate, true);
    if (result != Circuit::NullGate()) {
        acc_.DeleteGateIfNoUse(gate);
        ResizeAndSetTypeInfo(result, TypeInfo::INT32);
        return result;
    }
    TypeInfo output = GetOutputTypeInfo(gate);
    switch (output) {
        case TypeInfo::INT1:
            result = builder_.ConvertBoolToInt32(gate, support);
            break;
        case TypeInfo::CHAR:
        case TypeInfo::INT32:
            return gate;
        case TypeInfo::UINT32: {
            if (type != OpType::SHIFT_AND_LOGICAL) {
                result = builder_.CheckUInt32AndConvertToInt32(gate);
            } else {
                result = gate;
            }
            break;
        }
        case TypeInfo::FLOAT64:
            result = builder_.ConvertFloat64ToInt32(gate);
            break;
        case TypeInfo::HOLE_INT:
            result = builder_.CheckHoleIntAndConvertToInt32(gate);
            break;
        case TypeInfo::HOLE_DOUBLE:
            result = builder_.CheckHoleDoubleAndConvertToInt32(gate);
            break;
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            if (gateType.IsIntType()) {
                result = builder_.CheckTaggedIntAndConvertToInt32(gate);
            } else if (gateType.IsDoubleType()) {
                result = builder_.CheckTaggedDoubleAndConvertToInt32(gate);
            } else if (gateType.IsNullType()) {
                result = builder_.CheckNullAndConvertToInt32(gate);
            } else if (gateType.IsBooleanType()) {
                result = builder_.CheckTaggedBooleanAndConvertToInt32(gate);
            } else if (gateType.IsUndefinedType()) {
                if (type == OpType::SHIFT_AND_LOGICAL) {
                    result = builder_.CheckUndefinedAndConvertToInt32(gate);
                } else {
                    LOG_ECMA(FATAL) << "undefined cannot convert to int type";
                }
            } else {
                ASSERT(gateType.IsNumberType());
                result = builder_.CheckTaggedNumberAndConvertToInt32(gate);
            }
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            return Circuit::NullGate();
        }
    }
    ResizeAndSetTypeInfo(result, TypeInfo::INT32);
    return result;
}

GateRef NumberSpeculativeRetype::CheckAndConvertToFloat64(GateRef gate, GateType gateType, ConvertSupport support)
{
    auto result = TryConvertConstant(gate, false);
    if (result != Circuit::NullGate()) {
        acc_.DeleteGateIfNoUse(gate);
        ResizeAndSetTypeInfo(result, TypeInfo::FLOAT64);
        return result;
    }
    TypeInfo output = GetOutputTypeInfo(gate);
    switch (output) {
        case TypeInfo::INT1:
            result = builder_.ConvertBoolToFloat64(gate, support);
            break;
        case TypeInfo::INT32:
            result = builder_.ConvertInt32ToFloat64(gate);
            break;
        case TypeInfo::UINT32:
            result = builder_.ConvertUInt32ToFloat64(gate);
            break;
        case TypeInfo::FLOAT64:
            return gate;
        case TypeInfo::HOLE_INT:
            result = builder_.CheckHoleIntAndConvertToFloat64(gate);
            break;
        case TypeInfo::HOLE_DOUBLE:
            result = builder_.CheckHoleDoubleAndConvertToFloat64(gate);
            break;
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            if (gateType.IsIntType()) {
                result = builder_.CheckTaggedIntAndConvertToFloat64(gate);
            } else if (gateType.IsDoubleType()) {
                result = builder_.CheckTaggedDoubleAndConvertToFloat64(gate);
            } else if (gateType.IsNullType()) {
                result = builder_.CheckNullAndConvertToFloat64(gate);
            } else if (gateType.IsBooleanType()) {
                result = builder_.CheckTaggedBooleanAndConvertToFloat64(gate);
            } else if (gateType.IsUndefinedType()) {
                result = builder_.CheckUndefinedAndConvertToFloat64(gate);
            } else {
                ASSERT(gateType.IsNumberType());
                result = builder_.CheckTaggedNumberAndConvertToFloat64(gate);
            }
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            return Circuit::NullGate();
        }
    }
    ResizeAndSetTypeInfo(result, TypeInfo::FLOAT64);
    return result;
}

GateRef NumberSpeculativeRetype::CheckAndConvertToTagged(GateRef gate, GateType gateType)
{
    TypeInfo output = GetOutputTypeInfo(gate);
    switch (output) {
        case TypeInfo::INT1:
            return builder_.ConvertBoolToTaggedBoolean(gate);
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToTaggedInt(gate);
        case TypeInfo::UINT32:
            return builder_.ConvertUInt32ToTaggedNumber(gate);
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToTaggedDouble(gate);
        case TypeInfo::HOLE_INT:
            return builder_.CheckHoleIntAndConvertToTaggedInt(gate);
        case TypeInfo::HOLE_DOUBLE:
            return builder_.CheckHoleDoubleAndConvertToTaggedDouble(gate);
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            ASSERT(gateType.IsNumberType() || gateType.IsBooleanType());
            builder_.TryPrimitiveTypeCheck(gateType, gate);
            return gate;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::ConvertToTagged(GateRef gate)
{
    TypeInfo output = GetOutputTypeInfo(gate);
    switch (output) {
        case TypeInfo::INT1:
            return builder_.ConvertBoolToTaggedBoolean(gate);
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToTaggedInt(gate);
        case TypeInfo::UINT32:
            return builder_.ConvertUInt32ToTaggedNumber(gate);
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToTaggedDouble(gate);
        case TypeInfo::CHAR:
            return builder_.ConvertCharToEcmaString(gate);
        case TypeInfo::HOLE_INT:
            return builder_.ConvertSpecialHoleIntToTagged(gate);
        case TypeInfo::HOLE_DOUBLE:
            return builder_.ConvertSpecialHoleDoubleToTagged(gate);
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            return gate;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            return Circuit::NullGate();
    }
}

GateRef NumberSpeculativeRetype::VisitRangeCheckPredicate(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value0 = acc_.GetValueIn(gate, 0);
        GateRef value1 = acc_.GetValueIn(gate, 1);
        GateType value0Type = acc_.GetGateType(value0);
        GateType value1Type = acc_.GetGateType(value1);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value0, value0Type), 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value1, value1Type), 1);

        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitIndexCheck(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef receiver = acc_.GetValueIn(gate, 0);
        GateRef index = acc_.GetValueIn(gate, 1);
        GateType receiverType = acc_.GetGateType(receiver);
        GateType indexType = acc_.GetGateType(index);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(receiver, receiverType), 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1);

        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadArrayLength(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadStringLength(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadElement(GateRef gate)
{
    if (IsRetype()) {
        auto op = acc_.GetTypedLoadOp(gate);
        switch (op) {
            case TypedLoadOp::INT8ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT:
            case TypedLoadOp::INT16ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::INT32ARRAY_LOAD_ELEMENT:
                return SetOutputType(gate, GateType::IntType());
            case TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT:
                return SetOutputType(gate, TypeInfo::UINT32);
            case TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT:
            case TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT:
                return SetOutputType(gate, GateType::DoubleType());
            case TypedLoadOp::STRING_LOAD_ELEMENT:
                return SetOutputType(gate, TypeInfo::CHAR);
            case TypedLoadOp::ARRAY_LOAD_HOLE_INT_ELEMENT:
                return SetOutputType(gate, TypeInfo::HOLE_INT);
            case TypedLoadOp::ARRAY_LOAD_HOLE_DOUBLE_ELEMENT:
                return SetOutputType(gate, TypeInfo::HOLE_DOUBLE);
            default:
                return SetOutputType(gate, GateType::AnyType());
        }
    }

    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef index = acc_.GetValueIn(gate, 1);
        GateType indexType = acc_.GetGateType(index);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1);
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
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1);
        auto op = acc_.GetTypedStoreOp(gate);
        switch (op) {
            case TypedStoreOp::INT8ARRAY_STORE_ELEMENT:
            case TypedStoreOp::UINT8ARRAY_STORE_ELEMENT:
            case TypedStoreOp::UINT8CLAMPEDARRAY_STORE_ELEMENT:
            case TypedStoreOp::INT16ARRAY_STORE_ELEMENT:
            case TypedStoreOp::UINT16ARRAY_STORE_ELEMENT:
            case TypedStoreOp::INT32ARRAY_STORE_ELEMENT:
            case TypedStoreOp::UINT32ARRAY_STORE_ELEMENT:
            case TypedStoreOp::ARRAY_STORE_INT_ELEMENT:
                acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value, GateType::NumberType()), 2);   // 2: value idx
                break;
            case TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT:
            case TypedStoreOp::FLOAT64ARRAY_STORE_ELEMENT:
            case TypedStoreOp::ARRAY_STORE_DOUBLE_ELEMENT:
                acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(value, GateType::NumberType()), 2);  // 2: value idx
                break;
            default:
                acc_.ReplaceValueIn(gate, ConvertToTagged(value), 2);   // 2: value idx
                break;
        }
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
    ASSERT(IsConvert());
    GateRef value = acc_.GetValueIn(gate, 2); // 2: value

    Environment env(gate, circuit_, &builder_);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    if (plr.GetRepresentation() == Representation::DOUBLE) {
        acc_.SetMetaData(gate, circuit_->StorePropertyNoBarrier());
        acc_.ReplaceValueIn(
            gate, CheckAndConvertToFloat64(value, GateType::NumberType(), ConvertSupport::DISABLE), 2); // 2: value
    } else if (plr.GetRepresentation() == Representation::INT) {
        acc_.SetMetaData(gate, circuit_->StorePropertyNoBarrier());
        acc_.ReplaceValueIn(
            gate, CheckAndConvertToInt32(value, GateType::IntType(), ConvertSupport::DISABLE), 2); // 2: value
    } else {
        TypeInfo valueType = GetOutputTypeInfo(value);
        if (valueType == TypeInfo::INT1 || valueType == TypeInfo::INT32 || valueType == TypeInfo::FLOAT64) {
            acc_.SetMetaData(gate, circuit_->StorePropertyNoBarrier());
        }
        acc_.ReplaceValueIn(gate, ConvertToTagged(value), 2); // 2: value
    }

    GateRef receiver = acc_.GetValueIn(gate, 0); // receiver
    acc_.ReplaceValueIn(gate, ConvertToTagged(receiver), 0);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadProperty(GateRef gate)
{
    if (IsRetype()) {
        GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
        PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
        return SetOutputType(gate, plr.GetRepresentation());
    }

    ASSERT(IsConvert());

    return VisitWithConstantValue(gate, PROPERTY_LOOKUP_RESULT_INDEX); // ignoreIndex
}

GateRef NumberSpeculativeRetype::VisitTypeConvert(GateRef gate)
{
    GateRef input = acc_.GetValueIn(gate, 0);
    TypeInfo inputInfo = GetOutputTypeInfo(input);
    if (IsRetype()) {
        if (inputInfo == TypeInfo::TAGGED) {
            if (acc_.IsConstantNumber(input)) {
                return Circuit::NullGate();
            }
            GateType gateType = acc_.GetGateType(gate);
            ASSERT(gateType.IsNumberType());
            GateType resType = gateType.IsIntType() ? GateType::IntType() : GateType::DoubleType();
            return SetOutputType(gate, resType);
        }
        TypeInfo oldType = GetOutputTypeInfo(gate);
        SetOutputTypeInfo(gate, inputInfo);
        return oldType == inputInfo ? Circuit::NullGate() : gate;
    }
    ASSERT(IsConvert());
    ASSERT(inputInfo != TypeInfo::INT1 && inputInfo != TypeInfo::NONE);
    Environment env(gate, circuit_, &builder_);
    if (inputInfo == TypeInfo::TAGGED && !acc_.IsConstantNumber(input)) {
        GateType gateType = acc_.GetGateType(gate);
        ASSERT(gateType.IsNumberType());
        if (gateType.IsIntType()) {
            input = CheckAndConvertToInt32(input, GateType::IntType());
            ResizeAndSetTypeInfo(input, TypeInfo::INT32);
        } else {
            input = CheckAndConvertToFloat64(input, acc_.GetGateType(input));
            ResizeAndSetTypeInfo(input, TypeInfo::FLOAT64);
        }
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), input);
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberMod(GateRef gate)
{
    if (IsRetype()) {
        const PGOSampleType *sampleType = acc_.GetTypedBinaryType(gate).GetPGOSampleType();
        if (sampleType->IsNumber()) {
            return SetOutputType(gate, *sampleType);
        } else {
            GateType gateType = acc_.GetGateType(gate);
            GateType resType = gateType.IsIntType() ? GateType::IntType() : GateType::DoubleType();
            return SetOutputType(gate, resType);
        }
    } else if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        ConvertForBinaryOp(gate);
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitMonoLoadPropertyOnProto(GateRef gate)
{
    if (IsRetype()) {
        GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
        PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
        return SetOutputType(gate, plr.GetRepresentation());
    }

    ASSERT(IsConvert());
    size_t valueNum = acc_.GetNumValueIn(gate);
    for (size_t i = 0; i < valueNum; ++i) {
        if (i == PROPERTY_LOOKUP_RESULT_INDEX || i == HCLASS_INDEX) {
            continue;
        }
        GateRef input = acc_.GetValueIn(gate, i);
        acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
    }

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitMonoCallGetterOnProto(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            if (i == PROPERTY_LOOKUP_RESULT_INDEX || i == HCLASS_INDEX) {
                continue;
            }
            GateRef input = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitMonoStoreProperty(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    ASSERT(IsConvert());

    GateRef value = acc_.GetValueIn(gate, 4); // 4: value

    Environment env(gate, circuit_, &builder_);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    if (plr.IsAccessor()) {
        size_t valueNum = acc_.GetNumValueIn(gate);
        for (size_t i = 0; i < valueNum; ++i) {
            if (i == PROPERTY_LOOKUP_RESULT_INDEX || i == HCLASS_INDEX) {
                continue;
            }
            GateRef input = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
        }
        return Circuit::NullGate();
    }

    if (plr.GetRepresentation() == Representation::DOUBLE) {
        acc_.SetStoreNoBarrier(gate, true);
        acc_.ReplaceValueIn(
            gate, CheckAndConvertToFloat64(value, GateType::NumberType(), ConvertSupport::DISABLE), 4); // 4: value
    } else if (plr.GetRepresentation() == Representation::INT) {
        acc_.SetStoreNoBarrier(gate, true);
        acc_.ReplaceValueIn(
            gate, CheckAndConvertToInt32(value, GateType::IntType(), ConvertSupport::DISABLE), 4); // 4: value
    } else {
        TypeInfo valueType = GetOutputTypeInfo(value);
        if (valueType == TypeInfo::INT1 || valueType == TypeInfo::INT32 || valueType == TypeInfo::FLOAT64) {
            acc_.SetStoreNoBarrier(gate, true);
        }
        acc_.ReplaceValueIn(gate, ConvertToTagged(value), 4); // 4: value
    }

    GateRef receiver = acc_.GetValueIn(gate, 0); // receiver
    acc_.ReplaceValueIn(gate, ConvertToTagged(receiver), 0);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetypeManager::VisitGate(GateRef gate)
{
    retype_->setState(state_);
    return retype_->VisitGate(gate);
}

}  // namespace panda::ecmascript::kungfu
