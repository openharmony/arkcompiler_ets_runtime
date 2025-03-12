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

#include "ecmascript/compiler/circuit_arg_indices.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

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

GateRef NumberSpeculativeRetype::SetOutputType(GateRef gate, ParamType paramType)
{
    TypeInfo type = GetOutputTypeInfo(gate);
    TypeInfo old = type;
    if (paramType.IsIntType()) {
        type = TypeInfo::INT32;
    } else if (paramType.IsNumberType()) {
        type = TypeInfo::FLOAT64;
    } else if (paramType.IsDoubleType()) {
        type = TypeInfo::FLOAT64;
    } else if (paramType.IsIntOverflowType()) {
        type = TypeInfo::FLOAT64;
    } else if (paramType.IsBooleanType()) {
        type = TypeInfo::INT1;
    } else {
        type = TypeInfo::TAGGED;
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

TypeInfo NumberSpeculativeRetype::GetNumberTypeInfo(GateRef gate)
{
    TypeInfo typeInfo = GetOutputTypeInfo(gate);
    switch (typeInfo) {
        case TypeInfo::INT1:
        case TypeInfo::INT32:
        case TypeInfo::HOLE_INT:
            return TypeInfo::INT32;
        case TypeInfo::UINT32:
        case TypeInfo::FLOAT64:
        case TypeInfo::HOLE_DOUBLE:
            return TypeInfo::FLOAT64;
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            GateType gateType = acc_.GetGateType(gate);
            if (gateType.IsIntType() || gateType.IsBooleanType()) {
                return TypeInfo::INT32;
            } else if (gateType.IsDoubleType()) {
                return TypeInfo::FLOAT64;
            } else {
                return TypeInfo::TAGGED;
            }
        }
        case TypeInfo::CHAR:
            return TypeInfo::TAGGED;
    }
    UNREACHABLE();
}

static TypeInfo GetCommonTypeInfo(TypeInfo left, TypeInfo right)
{
    if (left == TypeInfo::TAGGED || right == TypeInfo::TAGGED) {
        return TypeInfo::TAGGED;
    }
    ASSERT(left == TypeInfo::INT32 || left == TypeInfo::FLOAT64);
    ASSERT(right == TypeInfo::INT32 || right == TypeInfo::FLOAT64);
    if (left == right) {
        return left;
    }
    return TypeInfo::FLOAT64;
}

void NumberSpeculativeRetype::setState(NumberSpeculativeRetype::State state)
{
    state_ = state;
}

void NumberSpeculativeRetype::ConvertMonoAccessorValueIn(GateRef gate)
{
    size_t valueNum = acc_.GetNumValueIn(gate);
    for (size_t i = 0; i < valueNum; ++i) {
        if (i == PROPERTY_LOOKUP_RESULT_INDEX || i == HCLASS_INDEX) {
            continue;
        }
        GateRef input = acc_.GetValueIn(gate, i);
        acc_.ReplaceValueIn(gate, ConvertToTagged(input), i);
    }
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
        case OpCode::LOAD_MAP_SIZE:
            return VisitLoadMapSize(gate);
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
        case OpCode::TYPE_CONVERT:
            return VisitTypeConvert(gate);
        case OpCode::FRAME_STATE:
            return VisitFrameState(gate);
        case OpCode::CALL_GETTER:
        case OpCode::CALL_SETTER:
        case OpCode::CONSTRUCT:
        case OpCode::CALL_NEW:
        case OpCode::TYPEDCALL:
        case OpCode::TYPEDFASTCALL:
        case OpCode::OBJECT_TYPE_CHECK:
        case OpCode::CALLINTERNAL:
            return VisitWithConstantValue(gate, PROPERTY_LOOKUP_RESULT_INDEX);
        case OpCode::LOOP_EXIT_VALUE:
        case OpCode::RANGE_GUARD:
            return VisitIntermediateValue(gate);
        case OpCode::NUMBER_TO_STRING:
            return VisitNumberToString(gate);
        case OpCode::ARRAY_FIND_OR_FINDINDEX:
            return VisitArrayFindOrFindIndex(gate);
        case OpCode::MATH_LOG:
        case OpCode::MATH_LOG2:
        case OpCode::MATH_LOG10:
        case OpCode::MATH_LOG1P:
        case OpCode::MATH_EXP:
        case OpCode::MATH_EXPM1:
        case OpCode::MATH_ACOS:
        case OpCode::MATH_ACOSH:
        case OpCode::MATH_ASIN:
        case OpCode::MATH_ASINH:
        case OpCode::MATH_ATAN:
        case OpCode::MATH_ATAN2:
        case OpCode::MATH_ATANH:
        case OpCode::MATH_COS:
        case OpCode::MATH_COSH:
        case OpCode::MATH_SIN:
        case OpCode::MATH_SINH:
        case OpCode::MATH_SQRT:
        case OpCode::MATH_TAN:
        case OpCode::MATH_TANH:
        case OpCode::MATH_POW:
        case OpCode::MATH_CBRT:
        case OpCode::MATH_FROUND:
        case OpCode::MATH_CEIL:
        case OpCode::MATH_FLOOR:
            return VisitMathDoubleParamsBuiltin(gate);
        case OpCode::MATH_CLZ32:
            return VisitClz32Builtin(gate);
        case OpCode::MATH_ABS:
        case OpCode::MATH_MIN:
        case OpCode::MATH_MAX:
        case OpCode::MATH_SIGN:
        case OpCode::MATH_ROUND:
            return VisitMathTaggedNumberParamsBuiltin(gate);
        case OpCode::MATH_TRUNC:
            return VisitMathTrunc(gate);
        case OpCode::GLOBAL_IS_FINITE:
        case OpCode::NUMBER_IS_FINITE:
            return VisitNumberOrGlobalBuiltin<false>(gate);
        case OpCode::GLOBAL_IS_NAN:
        case OpCode::NUMBER_IS_NAN:
            return VisitNumberOrGlobalBuiltin<true>(gate);
        case OpCode::NUMBER_IS_INTEGER:
        case OpCode::NUMBER_IS_SAFEINTEGER:
            return VisitNumberIsInteger(gate);
        case OpCode::NUMBER_PARSE_FLOAT:
            return VisitNumberParseFloat(gate);
        case OpCode::MATH_IMUL:
            return VisitMathImul(gate);
        case OpCode::DATA_VIEW_GET:
            return VisitDataViewGet(gate);
        case OpCode::DATA_VIEW_SET:
            return VisitDataViewSet(gate);
        case OpCode::DATE_GET_TIME:
            return VisitDateGetTime(gate);
        case OpCode::BIGINT_ASINTN:
        case OpCode::BIGINT_ASUINTN:
            return VisitBigIntAsIntN(gate);
        case OpCode::MAP_HAS:
        case OpCode::SET_HAS:
        case OpCode::MAP_DELETE:
        case OpCode::SET_DELETE:
            return VisitOthers(gate, GateType::BooleanType());
        case OpCode::DATE_NOW:
            return VisitDateNow(gate);
        case OpCode::BIGINT_CONSTRUCTOR:
            return VisitBigIntConstructor(gate);
        case OpCode::CREATE_ARRAY_WITH_BUFFER:
        case OpCode::TYPED_CREATE_OBJ_WITH_BUFFER:
        case OpCode::ARRAY_ITERATOR_BUILTIN:
        case OpCode::MAP_KEYS:
        case OpCode::MAP_VALUES:
        case OpCode::MAP_ENTRIES:
        case OpCode::SET_ENTRIES:
        case OpCode::SET_VALUES:
            return VisitOthersWithoutConvert(gate);
        case OpCode::STRING_SUB_STR:
        case OpCode::STRING_SUB_STRING:
        case OpCode::STRING_SLICE:
            return VisitString(gate);
        case OpCode::ARRAY_INCLUDES_INDEXOF:
            return VisitArrayIncludesIndexOf(gate);
        case OpCode::STRING_CHAR_CODE_AT:
            return VisitStringCharCodeAt(gate);
        case OpCode::NUMBER_PARSE_INT:
        case OpCode::JS_BYTECODE:
        case OpCode::RUNTIME_CALL:
        case OpCode::PRIMITIVE_TYPE_CHECK:
        case OpCode::STABLE_ARRAY_CHECK:
        case OpCode::TYPED_ARRAY_CHECK:
        case OpCode::TYPED_CALLTARGETCHECK_OP:
        case OpCode::TYPED_CALL_CHECK:
        case OpCode::HEAP_ALLOC:
        case OpCode::TYPED_SUPER_ALLOCATE_THIS:
        case OpCode::GET_SUPER_CONSTRUCTOR:
        case OpCode::ARG:
        case OpCode::INITVREG:
        case OpCode::RETURN:
        case OpCode::FRAME_ARGS:
        case OpCode::SAVE_REGISTER:
        case OpCode::RESTORE_REGISTER:
        case OpCode::LOAD_CONST_OFFSET:
        case OpCode::STORE_CONST_OFFSET:
        case OpCode::LEX_VAR_IS_HOLE_CHECK:
        case OpCode::TYPE_OF_CHECK:
        case OpCode::TYPE_OF:
        case OpCode::ARRAY_CONSTRUCTOR:
        case OpCode::FLOAT32_ARRAY_CONSTRUCTOR:
        case OpCode::OBJECT_CONSTRUCTOR:
        case OpCode::BOOLEAN_CONSTRUCTOR:
        case OpCode::LD_LOCAL_MODULE_VAR:
        case OpCode::STORE_MODULE_VAR:
        case OpCode::STRING_FROM_SINGLE_CHAR_CODE:
        case OpCode::ORDINARY_HAS_INSTANCE:
        case OpCode::ECMA_STRING_CHECK:
        case OpCode::INTERN_STRING_CHECK:
        case OpCode::CREATE_ARGUMENTS:
        case OpCode::TAGGED_TO_INT64:
        case OpCode::TYPED_CALL_BUILTIN:
        case OpCode::TYPED_CALL_BUILTIN_SIDE_EFFECT:
        case OpCode::CALL_PRIVATE_GETTER:
        case OpCode::CALL_PRIVATE_SETTER:
        case OpCode::MAP_GET:
        case OpCode::NEW_NUMBER:
        case OpCode::TYPED_ARRAY_ENTRIES:
        case OpCode::TYPED_ARRAY_KEYS:
        case OpCode::TYPED_ARRAY_VALUES:
        case OpCode::SET_ADD:
        case OpCode::LOAD_BUILTIN_OBJECT:
        case OpCode::CREATE_ARRAY:
        case OpCode::OBJECT_IS:
        case OpCode::OBJECT_GET_PROTOTYPE_OF:
        case OpCode::OBJECT_CREATE:
        case OpCode::OBJECT_IS_PROTOTYPE_OF:
        case OpCode::OBJECT_HAS_OWN_PROPERTY:
        case OpCode::REFLECT_GET_PROTOTYPE_OF:
        case OpCode::REFLECT_GET:
        case OpCode::REFLECT_HAS:
        case OpCode::REFLECT_CONSTRUCT:
        case OpCode::REFLECT_APPLY:
        case OpCode::FUNCTION_PROTOTYPE_APPLY:
        case OpCode::FUNCTION_PROTOTYPE_BIND:
        case OpCode::FUNCTION_PROTOTYPE_CALL:
        case OpCode::BUILTIN_PROTOTYPE_HCLASS_CHECK:
        case OpCode::FLATTEN_TREE_STRING_CHECK:
        case OpCode::ARRAY_POP:
        case OpCode::ARRAY_PUSH:
        case OpCode::ARRAY_SOME:
        case OpCode::ARRAY_EVERY:
        case OpCode::ARRAY_FOR_EACH:
        case OpCode::HEAP_OBJECT_CHECK:
        case OpCode::ARRAY_FILTER:
        case OpCode::ARRAY_MAP:
        case OpCode::ARRAY_SLICE:
        case OpCode::ARRAY_BUFFER_IS_VIEW:
        case OpCode::ARRAY_SORT:
        case OpCode::FINISH_ALLOCATE:
        case OpCode::IS_CALLABLE_CHECK:
        case OpCode::IS_UNDEFINED_OR_HOLE_CHECK:
        case OpCode::IS_NOT_UNDEFINED_OR_HOLE_CHECK:
        case OpCode::ECMA_OBJECT_CHECK:
        case OpCode::GET_EXCEPTION:
        case OpCode::MATH_HCLASS_CONSISTENCY_CHECK:
            return VisitOthers(gate);
        case OpCode::CALL:
        case OpCode::BYTECODE_CALL:
        case OpCode::DEBUGGER_BYTECODE_CALL:
        case OpCode::BUILTINS_CALL_WITH_ARGV:
        case OpCode::BUILTINS_CALL:
        case OpCode::RUNTIME_CALL_WITH_ARGV:
        case OpCode::BASELINE_CALL:
        case OpCode::ASM_CALL_BARRIER:
        case OpCode::CALL_OPTIMIZED:
        case OpCode::FAST_CALL_OPTIMIZED:
            UNREACHABLE();
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
        if (acc_.HasNumberType(gate)) {
            return VisitNumberBinaryOp(gate);
        }
    }

    return VisitEqualCompareOrNotEqualCompare(gate);
}

GateRef NumberSpeculativeRetype::VisitArrayFindOrFindIndex(GateRef gate)
{
    constexpr size_t BUILTINS_FUNC_ID_INDEX = 3;
    ASSERT(acc_.GetOpCode(gate) == OpCode::ARRAY_FIND_OR_FINDINDEX);
    ASSERT(acc_.GetNumValueIn(gate) > BUILTINS_FUNC_ID_INDEX);
    if (IsRetype()) {
        GateRef builtinFunc = acc_.GetValueIn(gate, BUILTINS_FUNC_ID_INDEX);
        auto builtinsID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(builtinFunc));
        if (builtinsID == BuiltinsStubCSigns::ID::ArrayFind) {
            return SetOutputType(gate, GateType::AnyType());
        } else {
            return SetOutputType(gate, GateType::IntType());
        }
    }

    return VisitWithConstantValue(gate, BUILTINS_FUNC_ID_INDEX); // ignoreIndex
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
        case TypedBinOp::TYPED_STRICTEQ:
        case TypedBinOp::TYPED_STRICTNOTEQ:
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
                input = builder_.ConvertCharToEcmaString(input);
            }
            acc_.ReplaceValueIn(gate, input, i);
        }
    }
    return Circuit::NullGate();
}

TypeInfo NumberSpeculativeRetype::GetOutputForPhi(GateRef gate, bool ignoreConstant)
{
    size_t valueNum = acc_.GetNumValueIn(gate);
    bool hasConstantInput = false;
    TypeInfo tempType = TypeInfo::NONE;
    for (size_t i = 0; i < valueNum; ++i) {
        GateRef input = acc_.GetValueIn(gate, i);
        TypeInfo inputInfo = GetOutputTypeInfo(input);
        if (ignoreConstant && acc_.IsConstantNumber(input)) {
            hasConstantInput = true;
            continue;
        }
        if (inputInfo == TypeInfo::NONE) {
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

    if (hasConstantInput && ignoreConstant && (tempType == TypeInfo::NONE || tempType == TypeInfo::CHAR)) {
        return GetOutputForPhi(gate, false);
    }
    return tempType;
}

GateRef NumberSpeculativeRetype::VisitPhi(GateRef gate)
{
    if (IsRetype()) {
        auto tempType = GetOutputForPhi(gate, true);
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
        case TypeInfo::UINT32:
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
        case TypedBinOp::TYPED_DIV:
        case TypedBinOp::TYPED_MOD: {
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
        default: {
            TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
            const ParamType paramType = accessor.GetParamType();
            return VisitNumberRelated(gate, paramType);
        }
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
        default: {
            TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
            ParamType paramType = accessor.GetParamType();
            return VisitNumberRelated(gate, paramType);
        }
    }
}

GateRef NumberSpeculativeRetype::VisitTypedConditionJump(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedJumpAccessor accessor(acc_.TryGetValue(gate));
    ParamType type = accessor.GetParamType();
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
        TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
        const ParamType paramType = accessor.GetParamType();
        ASSERT(paramType.HasNumberType());
        return SetOutputType(gate, paramType);
    } else if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        ConvertForNumberBinaryOp(gate);
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
        ConvertForNumberCompareOp(gate);
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
        ConvertForNumberShiftAndLogicalOperator(gate);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberMonocular(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    ParamType type = accessor.GetParamType();
    ASSERT(type.HasNumberType());
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
        ParamType paramType = accessor.GetParamType();
        GateRef value = acc_.GetValueIn(gate, 0);
        GateType valueType = GateType::NumberType();
        if (paramType.IsIntType()) {
            valueType = GateType::IntType();
        } else if (paramType.IsDoubleType()) {
            valueType = GateType::DoubleType();
        }
        acc_.ReplaceValueIn(gate, CheckAndConvertToFloat64(value, valueType), 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitIsTrueOrFalse(GateRef gate)
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType() || paramType.IsBooleanType());
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BooleanType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        GateType valueType = GateType::NumberType();
        // NOTICE-PGO: wx support undefined
        if (paramType.IsIntType()) {
            valueType = GateType::IntType();
        } else if (paramType.IsDoubleType()) {
            valueType = GateType::DoubleType();
        } else if (paramType.IsBooleanType()) {
            valueType = GateType::BooleanType();
        }
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
    ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        GateRef value = acc_.GetValueIn(gate, 0);
        GateType valueType = GateType::NumberType();
        if (paramType.IsIntType()) {
            valueType = GateType::IntType();
        } else if (paramType.IsDoubleType()) {
            valueType = GateType::DoubleType();
        }
        acc_.ReplaceValueIn(gate,
            CheckAndConvertToInt32(value, valueType, ConvertSupport::ENABLE, OpType::SHIFT_AND_LOGICAL, false), 0);
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

GateRef NumberSpeculativeRetype::VisitNumberRelated(GateRef gate, ParamType paramType)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::NumberType());
    }
    if (IsConvert()) {
        Environment env(gate, circuit_, &builder_);
        size_t valueNum = acc_.GetNumValueIn(gate);
        GateType inputType = GateType::AnyType();
        if (paramType.IsIntType() || paramType.IsIntOverflowType()) {
            inputType = GateType::IntType();
        } else if (paramType.IsDoubleType()) {
            inputType = GateType::DoubleType();
        } else if (paramType.IsNumberType()) {
            inputType = GateType::NumberType();
        } else if (paramType.IsBooleanType()) {
            inputType = GateType::BooleanType();
        }
        for (size_t i = 0; i < valueNum; ++i) {
            GateRef input = acc_.GetValueIn(gate, i);
            if (paramType.HasNumberType() || paramType.IsBooleanType()) {
                acc_.ReplaceValueIn(gate, CheckAndConvertToTagged(input, inputType, ConvertToNumber::BOOL_ONLY), i);
            }
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
    GateRef frameValue = acc_.GetFrameValue(gate);
    size_t numValueIn = acc_.GetNumValueIn(frameValue);
    for (size_t i = 0; i < numValueIn; i++) {
        GateRef val = acc_.GetValueIn(frameValue, i);
        TypeInfo output = GetOutputTypeInfo(val);
        // The convert of char type needs to be converted into ecmastring, and other types of variables are converted
        // in the ir builder stage. Because the typeinfo::char information will be lost after passing this pass.
        switch (output) {
            case TypeInfo::CHAR:
                acc_.ReplaceValueIn(frameValue, ConvertToTagged(val), i);
                break;
            case TypeInfo::NONE:
            case TypeInfo::INT1:
            case TypeInfo::INT32:
            case TypeInfo::UINT32:
            case TypeInfo::FLOAT64:
            case TypeInfo::TAGGED:
                break;
            default:
                // Other new types are intercepted here. Please confirm that the new types will not allocate heap
                // objects after ConvertToTagged occurs. This check is skipped after confirmation.
                LOG_ECMA(FATAL) << "Please check new kind of TypeInfo";
                UNREACHABLE();
                break;
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitOthers(GateRef gate, GateType outputType)
{
    if (IsRetype()) {
        return SetOutputType(gate, outputType);
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

GateRef NumberSpeculativeRetype::VisitOthersWithoutConvert(GateRef gate, GateType outputType)
{
    if (IsRetype()) {
        return SetOutputType(gate, outputType);
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
        case TypeInfo::CHAR:
            // CHAR refers to string of length 1, while only empty string converts to false in a boolean context.
            return builder_.Boolean(true);
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

void NumberSpeculativeRetype::ConvertForNumberBinaryOp(GateRef gate)
{
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    if (paramType.IsIntType()) {
        ConvertForIntOperator(gate, GateType::IntType(), GateType::IntType());
    } else if (paramType.IsIntOverflowType()) {
        ConvertForDoubleOperator(gate, GateType::IntType(), GateType::IntType());
    } else if (paramType.IsDoubleType()) {
        ConvertForDoubleOperator(gate, GateType::DoubleType(), GateType::DoubleType());
    } else if (paramType.IsNumberType()) {
        ConvertForDoubleOperator(gate, GateType::NumberType(), GateType::NumberType());
    }
}

void NumberSpeculativeRetype::ConvertForNumberCompareOp(GateRef gate)
{
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    ASSERT(!paramType.IsIntOverflowType());
    if (paramType.IsIntType()) {
        ConvertForIntOperator(gate, GateType::IntType(), GateType::IntType());
    } else {
        ConvertForDoubleOperator(gate, GateType::NumberType(), GateType::NumberType());
    }
}

void NumberSpeculativeRetype::ConvertForNumberShiftAndLogicalOperator(GateRef gate)
{
    TypedBinaryAccessor accessor(acc_.TryGetValue(gate));
    const ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.HasNumberType());
    if (paramType.IsIntType()) {
        ConvertForShiftAndLogicalOperator(gate, GateType::IntType(), GateType::IntType());
    } else {
        ConvertForShiftAndLogicalOperator(gate, GateType::NumberType(), GateType::NumberType());
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
    GateRef cLeft = CheckAndConvertToInt32(left, leftType, ConvertSupport::ENABLE, OpType::SHIFT_AND_LOGICAL, false);
    GateRef cRight = CheckAndConvertToInt32(right, rightType, ConvertSupport::ENABLE, OpType::SHIFT_AND_LOGICAL, false);

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
            return needInt32 ? builder_.Int32(rawValue) : builder_.Double(static_cast<double>(rawValue));
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
        return needInt32 ? builder_.Int32(rawValue) : builder_.Double(static_cast<double>(rawValue));
    } else if (value.IsDouble() && !needInt32) {
        double rawValue = value.GetDouble();
        return builder_.Double(rawValue);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::TryConvertConstantToInt32(GateRef gate)
{
    if (acc_.GetOpCode(gate) != OpCode::CONSTANT) {
        return Circuit::NullGate();
    }

    if (acc_.GetGateType(gate).IsNJSValueType()) {
        MachineType mType = acc_.GetMachineType(gate);
        if (mType == MachineType::I32) {
            int32_t rawValue = acc_.GetInt32FromConstant(gate);
            return builder_.Int32(rawValue);
        } else if (mType == MachineType::F64) {
            double rawValue = acc_.GetFloat64FromConstant(gate);
            int32_t int32Value = base::NumberHelper::DoubleInRangeInt32(rawValue);
            return builder_.Int32(int32Value);
        } else {
            return Circuit::NullGate();
        }
    }

    JSTaggedValue value(acc_.GetConstantValue(gate));
    if (value.IsInt()) {
        int32_t rawValue = value.GetInt();
        return builder_.Int32(rawValue);
    } else if (value.IsDouble()) {
        double rawValue = value.GetDouble();
        int32_t int32Value = base::NumberHelper::DoubleInRangeInt32(rawValue);
        return builder_.Int32(int32Value);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::CheckTaggedAndConvertToInt32(GateRef gate, GateType gateType, OpType type)
{
    GateRef result = Circuit::NullGate();
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
        ASSERT(gateType.IsNumberType() || gateType.IsAnyType());
        result = builder_.CheckTaggedNumberAndConvertToInt32(gate);
    }
    return result;
}

GateRef NumberSpeculativeRetype::CheckAndConvertToInt32(GateRef gate, GateType gateType, ConvertSupport support,
                                                        OpType type, bool needCheckFloat64)
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
        case TypeInfo::CHAR: {
            result = builder_.ConvertCharToInt32(gate);
            break;
        }
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
        case TypeInfo::FLOAT64: {
            if (needCheckFloat64) {
                result = builder_.CheckFloat64ConvertToInt32Legally(gate);
            } else {
                result = builder_.ConvertFloat64ToInt32(gate);
            }
            break;
        }
        case TypeInfo::HOLE_INT:
            result = builder_.CheckHoleIntAndConvertToInt32(gate);
            break;
        case TypeInfo::HOLE_DOUBLE:
            result = builder_.CheckHoleDoubleAndConvertToInt32(gate);
            break;
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            result = CheckTaggedAndConvertToInt32(gate, gateType, type);
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

GateRef NumberSpeculativeRetype::CheckBoundAndConvertToInt32(GateRef gate, ConvertSupport support, OpType type)
{
    auto result = TryConvertConstantToInt32(gate);
    GateType gateType = acc_.GetGateType(gate);
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
        case TypeInfo::CHAR: {
            result = builder_.ConvertCharToInt32(gate);
            break;
        }
        case TypeInfo::INT32:
            return gate;
        case TypeInfo::UINT32: {
            result = builder_.CheckUInt32AndConvertToInt32(gate);
            break;
        }
        case TypeInfo::FLOAT64: {
            result = builder_.DoubleCheckINFInRangeInt32(gate);
            break;
        }
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
                GateRef doubleValue = builder_.CheckTaggedDoubleAndConvertToFloat64(gate);
                result = builder_.DoubleInRangeInt32(doubleValue);
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
                GateRef doubleValue = builder_.CheckTaggedNumberAndConvertToFloat64(gate);
                result = builder_.DoubleInRangeInt32(doubleValue);
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

GateRef NumberSpeculativeRetype::CheckAndConvertToFloat64(GateRef gate, GateType gateType,
                                                          ConvertToNumber convert)
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
            result = builder_.ConvertBoolToFloat64(gate, ToConvertSupport(convert));
            break;
        case TypeInfo::CHAR: {
            result = builder_.ConvertCharToDouble(gate);
            break;
        }
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
                if (convert == ConvertToNumber::ALL) {
                    GateRef glue = acc_.GetGlueFromArgList();
                    GateRef number = builder_.ToNumber(glue, gate, glue);
                    ResizeAndSetTypeInfo(number, TypeInfo::TAGGED);
                    result = builder_.GetDoubleOfTNumber(number);
                } else {
                    result = builder_.CheckTaggedNumberAndConvertToFloat64(gate);
                }
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

GateRef NumberSpeculativeRetype::CheckAndConvertToTagged(GateRef gate, GateType gateType, ConvertToNumber convert)
{
    TypeInfo output = GetOutputTypeInfo(gate);
    switch (output) {
        case TypeInfo::INT1: {
            if (gateType.IsNumberType() && convert != ConvertToNumber::DISABLE) {
                return builder_.ConvertInt32ToTaggedInt(builder_.BooleanToInt32(gate));
            }
            return builder_.ConvertBoolToTaggedBoolean(gate);
        }
        case TypeInfo::INT32:
            return builder_.ConvertInt32ToTaggedInt(gate);
        case TypeInfo::UINT32:
            return builder_.ConvertUInt32ToTaggedNumber(gate);
        case TypeInfo::FLOAT64:
            return builder_.ConvertFloat64ToTaggedDouble(gate);
        case TypeInfo::CHAR:
            return builder_.ConvertCharToEcmaString(gate);
        case TypeInfo::HOLE_INT:
            return builder_.CheckHoleIntAndConvertToTaggedInt(gate);
        case TypeInfo::HOLE_DOUBLE:
            return builder_.CheckHoleDoubleAndConvertToTaggedDouble(gate);
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            if (convert == ConvertToNumber::ALL) {
                // Convert if not number
                ASSERT(gateType.IsNumberType());
                GateRef glue = acc_.GetGlueFromArgList();
                GateRef ret = builder_.ToNumber(glue, gate, glue);
                ResizeAndSetTypeInfo(ret, TypeInfo::TAGGED);
                return ret;
            }
            // Deoptimize if not number
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
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value0, value0Type, ConvertSupport::ENABLE, OpType::NORMAL,
            false), 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value1, value1Type, ConvertSupport::ENABLE, OpType::NORMAL,
            false), 1);

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
        if (indexType.IsAnyType()) {
            indexType = GateType::IntType();
        }
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(receiver, receiverType, ConvertSupport::ENABLE, OpType::NORMAL,
            false), 0);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType, ConvertSupport::ENABLE, OpType::NORMAL,
            false), 1);

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

    if (IsConvert()) {
        GateRef input = acc_.GetValueIn(gate, 0);
        TypeInfo typeInfo = GetOutputTypeInfo(input);
        if (typeInfo == TypeInfo::CHAR) {
            acc_.ReplaceValueIn(gate, ConvertToTagged(input), 0);
        }
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadMapSize(GateRef gate)
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
        GateRef index = acc_.GetValueIn(gate, 1); // 1: value idx
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
        GateRef index = acc_.GetValueIn(gate, 1); // 1: value idx
        GateType indexType = acc_.GetGateType(index);
        GateRef value = acc_.GetValueIn(gate, 2); // 2: value idx
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(index, indexType), 1); // 1: value idx
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
                acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(value, GateType::NumberType(), ConvertSupport::ENABLE,
                    OpType::NORMAL, false), 2);   // 2: value idx
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
            gate, CheckAndConvertToFloat64(value, GateType::NumberType(), ConvertToNumber::DISABLE), 2); // 2: value
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
    acc_.ReplaceValueIn(gate, ConvertToTagged(receiver), 0); // 0: value idx
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitLoadProperty(GateRef gate)
{
    if (IsRetype()) {
        GateRef propertyLookupResult = acc_.GetValueIn(gate, 1); // 1: value idx
        PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
        return SetOutputType(gate, plr.GetRepresentation());
    }

    ASSERT(IsConvert());

    return VisitWithConstantValue(gate, PROPERTY_LOOKUP_RESULT_INDEX); // ignoreIndex
}

GateRef NumberSpeculativeRetype::VisitTypeConvert(GateRef gate)
{
    GateRef input = acc_.GetValueIn(gate, 0); // 0: value idx
    TypeInfo inputInfo = GetOutputTypeInfo(input);
    TypeConvertAccessor accessor(acc_.TryGetValue(gate));
    ParamType paramType = accessor.GetLeftType();
    bool optForConstant = acc_.IsConstantNumber(input) || acc_.GetOpCode(input) == OpCode::TYPE_CONVERT;
    if (IsRetype()) {
        if (inputInfo == TypeInfo::CHAR || inputInfo == TypeInfo::INT1) {
            ASSERT(paramType.HasNumberType());
            return SetOutputType(gate, paramType);
        }
        if (inputInfo == TypeInfo::TAGGED) {
            if (optForConstant) {
                TypeInfo oldType = GetOutputTypeInfo(gate);
                acc_.SetGateType(gate, acc_.GetGateType(input));
                SetOutputTypeInfo(gate, inputInfo);
                return oldType == inputInfo ? Circuit::NullGate() : gate;
            }
            ASSERT(paramType.HasNumberType());
            return SetOutputType(gate, paramType);
        }
        TypeInfo oldType = GetOutputTypeInfo(gate);
        SetOutputTypeInfo(gate, inputInfo);
        return oldType == inputInfo ? Circuit::NullGate() : gate;
    }
    ASSERT(IsConvert());
    ASSERT(inputInfo != TypeInfo::NONE);
    Environment env(gate, circuit_, &builder_);
    if ((inputInfo == TypeInfo::TAGGED && !optForConstant) ||
        inputInfo == TypeInfo::CHAR || inputInfo == TypeInfo::INT1) {
        ASSERT(paramType.HasNumberType());
        if (paramType.IsIntType()) {
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

GateRef NumberSpeculativeRetype::VisitMathDoubleParamsBuiltin(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::DoubleType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    size_t valueNum = acc_.GetNumValueIn(gate);
    for (size_t i = 0; i < valueNum; ++i) {
        GateRef input = acc_.GetValueIn(gate, i);
        GateRef convertedInput = CheckAndConvertToFloat64(input, GateType::NumberType(), ConvertToNumber::BOOL_ONLY);
        acc_.ReplaceValueIn(gate, convertedInput, i);
    }
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

double NumberSpeculativeRetype::GetDoubleValueFromConst(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::CONSTANT);
    double rawValue;
    if (acc_.GetGateType(gate).IsNJSValueType()) {
        ASSERT(GetNumberInputTypeInfo(gate) == TypeInfo::FLOAT64);
        rawValue = acc_.GetFloat64FromConstant(gate);
    } else {
        JSTaggedValue value(acc_.GetConstantValue(gate));
        ASSERT(value.IsDouble());
        rawValue = value.GetDouble();
    }
    return rawValue;
}

template <bool IS_NAN>
GateRef NumberSpeculativeRetype::VisitNumberOrGlobalBuiltin(GateRef gate)
{
    if (IsRetype()) {
        ASSERT(acc_.GetOpCode(gate) == OpCode::GLOBAL_IS_FINITE ||
               acc_.GetOpCode(gate) == OpCode::NUMBER_IS_FINITE ||
               acc_.GetOpCode(gate) == OpCode::GLOBAL_IS_NAN ||
               acc_.GetOpCode(gate) == OpCode::NUMBER_IS_NAN);
        return SetOutputType(gate, GateType::BooleanType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef input = acc_.GetValueIn(gate, 0);

    // We change IsNan/IsFinite to constant if input is INT32 without check
    // So we skip tagged input with int profiled type
    auto type = GetNumberInputTypeInfo(input, true);
    if (type == TypeInfo::INT32) {
        GateRef result;
        if constexpr (IS_NAN) {
            result = builder_.Boolean(false);
        } else {
            result = builder_.Boolean(true);
        }
        ResizeAndSetTypeInfo(result, TypeInfo::INT1);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        return Circuit::NullGate();
    } else {
        ASSERT(type == TypeInfo::FLOAT64);
        input = CheckAndConvertToFloat64(input, GateType::NumberType());
    }

    acc_.ReplaceValueIn(gate, input, 0);
    ResizeAndSetTypeInfo(input, TypeInfo::FLOAT64);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberIsInteger(GateRef gate)
{
    if (IsRetype()) {
        ASSERT(acc_.GetOpCode(gate) == OpCode::NUMBER_IS_INTEGER ||
               acc_.GetOpCode(gate) == OpCode::NUMBER_IS_SAFEINTEGER);
        return SetOutputType(gate, GateType::BooleanType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef input = acc_.GetValueIn(gate, 0);
    auto type = GetNumberInputTypeInfo<false>(input);
    if (type == TypeInfo::INT32 || type == TypeInfo::INT1) {
        GateRef result = type == TypeInfo::INT32 ? builder_.True() : builder_.False();
        ResizeAndSetTypeInfo(result, TypeInfo::INT1);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
        return Circuit::NullGate();
    }
    ASSERT(type == TypeInfo::FLOAT64);
    input = ConvertToTagged(input);
    acc_.ReplaceValueIn(gate, input, 0);
    ResizeAndSetTypeInfo(input, TypeInfo::TAGGED);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberParseFloat(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::DoubleType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    GateRef input = acc_.GetValueIn(gate, 0);
    TypeInfo type = GetNumberTypeInfo(input);
    if (type == TypeInfo::INT32) {
        // replace parseFloat with cast
        input = CheckAndConvertToFloat64(input, GateType::NumberType(), ConvertToNumber::DISABLE);
        acc_.ReplaceGate(gate, builder_.GetStateDepend(), input);
    } else {
        acc_.ReplaceValueIn(gate, ConvertToTagged(input), 0);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitNumberParseInt(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    ASSERT(IsConvert());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitClz32Builtin(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    GateRef input = acc_.GetValueIn(gate, 0);

    TypeInfo type = GetNumberTypeInfo(input);
    if (type == TypeInfo::INT32) {
        input = CheckAndConvertToInt32(input, GateType::IntType());
        acc_.SetMetaData(gate, circuit_->MathClz32Int32());
    } else {  // All other types describe in ConvertToFloat64
        input = CheckAndConvertToFloat64(input, GateType::NumberType(), ConvertToNumber::BOOL_ONLY);
        acc_.SetMetaData(gate, circuit_->MathClz32Double());
    }
    acc_.ReplaceValueIn(gate, input, 0);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

const GateMetaData *NumberSpeculativeRetype::GetNewMeta(OpCode op, TypeInfo type)
{
    if (type == TypeInfo::INT32) {
        switch (op) {
            case OpCode::MATH_ABS:
                return circuit_->MathAbsInt32();
            case OpCode::MATH_MIN:
                return circuit_->MathMinInt32();
            case OpCode::MATH_MAX:
                return circuit_->MathMaxInt32();
            default:
                return nullptr;
        }
    } else if (type == TypeInfo::FLOAT64) {
        switch (op) {
            case OpCode::MATH_ABS:
                return circuit_->MathAbsDouble();
            case OpCode::MATH_MIN:
                return circuit_->MathMinDouble();
            case OpCode::MATH_MAX:
                return circuit_->MathMaxDouble();
            case OpCode::MATH_ROUND:
                return circuit_->MathRoundDouble();
            default:
                return nullptr;
        }
    } else if (type == TypeInfo::TAGGED) {
        if (op == OpCode::MATH_SIGN) {
            return circuit_->MathSignTagged();
        }
        return nullptr;
    } else {
        return nullptr;
    }
}

void NumberSpeculativeRetype::UpdateMeta(GateRef gate, TypeInfo newType, const GateMetaData *meta)
{
    if (meta == nullptr) {
        return;
    }
    acc_.SetMetaData(gate, meta);
    if (newType != TypeInfo::TAGGED) {
        ASSERT(newType == TypeInfo::INT32 || newType == TypeInfo::FLOAT64);
        acc_.SetGateType(gate, GateType::NJSValue());
        acc_.SetMachineType(gate, newType == TypeInfo::INT32 ? MachineType::I32 : MachineType::F64);
    } else {
        ASSERT(acc_.GetGateType(gate) != GateType::NJSValue() && acc_.GetMachineType(gate) == MachineType::I64);
    }
}

GateRef NumberSpeculativeRetype::VisitMathTaggedNumberParamsBuiltin(GateRef gate)
{
    size_t valueNum = acc_.GetNumValueIn(gate);
    ASSERT(valueNum <= 2U);
    OpCode op = acc_.GetOpCode(gate);
    if (IsRetype()) {
        TypeInfo type = GetNumberTypeInfo(acc_.GetValueIn(gate, 0));
        if (valueNum > 1U) {
            TypeInfo secondInputType = GetNumberTypeInfo(acc_.GetValueIn(gate, 1U));
            type = GetCommonTypeInfo(type, secondInputType);
        }
        if (type == TypeInfo::TAGGED && op == OpCode::MATH_ROUND) {
            type = TypeInfo::FLOAT64;
        }
        if (op == OpCode::MATH_SIGN) {
            if (type == TypeInfo::INT32) {
                return SetOutputType(gate, TypeInfo::INT32);
            }
            return SetOutputType(gate, TypeInfo::TAGGED);
        }
        return SetOutputType(gate, type);
    }
    ASSERT(IsConvert());
    TypeInfo type = GetOutputTypeInfo(gate); // load type computed in retype phase
    if (op == OpCode::MATH_ROUND) {
        type = GetNumberTypeInfo(acc_.GetValueIn(gate, 0));
    }
    const GateMetaData* meta = GetNewMeta(op, type);
    UpdateMeta(gate, type, meta);
    Environment env(gate, circuit_, &builder_);
    for (size_t i = 0; i < valueNum; ++i) {
        GateRef input = acc_.GetValueIn(gate, i);
        if (type == TypeInfo::INT32) {
            input = CheckAndConvertToInt32(input, GateType::IntType());
        } else if (type == TypeInfo::FLOAT64) {
            input = CheckAndConvertToFloat64(input, acc_.GetGateType(input), ConvertToNumber::BOOL_ONLY);
        } else {
            ASSERT(type == TypeInfo::TAGGED);
            input = CheckAndConvertToTagged(input, GateType::NumberType(), ConvertToNumber::BOOL_ONLY);
        }
        acc_.ReplaceValueIn(gate, input, i);
    }
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitMathTrunc(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::DoubleType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef input = acc_.GetValueIn(gate, 0);
    acc_.ReplaceValueIn(gate, CheckAndConvertToTagged(input, GateType::NumberType(), ConvertToNumber::BOOL_ONLY), 0);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitBigIntAsIntN(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3U);
    GateRef bits = acc_.GetValueIn(gate, 0);
    acc_.ReplaceValueIn(gate, ConvertToTagged(CheckAndConvertToFloat64(bits, GateType::NumberType(),
                                                                       ConvertToNumber::BOOL_ONLY)), 0);
    GateRef bigint = acc_.GetValueIn(gate, 1);
    acc_.ReplaceValueIn(gate, ConvertToTagged(bigint), 1);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

template<bool BOOL_AS_INT>
TypeInfo NumberSpeculativeRetype::GetNumberInputTypeInfo(GateRef gate, bool skipTagged)
{
    TypeInfo typeInfo = GetOutputTypeInfo(gate);
    switch (typeInfo) {
        case TypeInfo::INT1:
            return BOOL_AS_INT ? TypeInfo::INT32 : TypeInfo::INT1;
        case TypeInfo::INT32:
        case TypeInfo::HOLE_INT:
            return TypeInfo::INT32;
        case TypeInfo::NONE:
        case TypeInfo::TAGGED: {
            if (skipTagged) {
                return TypeInfo::FLOAT64;
            }
            GateType gateType = acc_.GetGateType(gate);
            if (gateType.IsIntType()) {
                return TypeInfo::INT32;
            }
            if (gateType.IsBooleanType()) {
                return BOOL_AS_INT ? TypeInfo::INT32 : TypeInfo::INT1;
            }
            return TypeInfo::FLOAT64;
        }
        default:
            return TypeInfo::FLOAT64;
    }
}

void NumberSpeculativeRetype::SetNewInputForMathImul(GateRef gate, int idx)
{
    GateRef input = acc_.GetValueIn(gate, idx);
    auto type = GetNumberInputTypeInfo(input);
    if (type == TypeInfo::INT32) {
        input = CheckAndConvertToInt32(input, GateType::IntType());
    } else {
        ASSERT(type == TypeInfo::FLOAT64);
        input = CheckAndConvertToFloat64(input, GateType::NumberType(), ConvertToNumber::BOOL_ONLY);
        input = builder_.TruncDoubleToInt(acc_.GetGlueFromArgList(), input, base::INT32_BITS);
    }
    ResizeAndSetTypeInfo(input, TypeInfo::INT32);
    acc_.ReplaceValueIn(gate, input, idx);
}

GateRef NumberSpeculativeRetype::VisitMathImul(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::IntType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2U);

    SetNewInputForMathImul(gate, 0);
    SetNewInputForMathImul(gate, 1);

    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.SetMachineType(gate, MachineType::I32);
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitDataViewGet(GateRef gate)
{
    GateRef builtinsID = acc_.GetValueIn(gate, 2);
    auto ID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(builtinsID));
    if (IsRetype()) {
        switch (ID) {
            case BuiltinsStubCSigns::ID::DataViewGetInt8:
                return SetOutputType(gate, GateType::IntType());
            case BuiltinsStubCSigns::ID::DataViewGetUint8:
                return SetOutputType(gate, GateType::IntType());
            case BuiltinsStubCSigns::ID::DataViewGetInt16:
                return SetOutputType(gate, GateType::IntType());
            case BuiltinsStubCSigns::ID::DataViewGetUint16:
                return SetOutputType(gate, GateType::IntType());
            case BuiltinsStubCSigns::ID::DataViewGetInt32:
                return SetOutputType(gate, GateType::IntType());
            case BuiltinsStubCSigns::ID::DataViewGetUint32:
                return SetOutputType(gate, TypeInfo::UINT32);
            case BuiltinsStubCSigns::ID::DataViewGetFloat32:
                return SetOutputType(gate, GateType::DoubleType());
            case BuiltinsStubCSigns::ID::DataViewGetFloat64:
                return SetOutputType(gate, GateType::DoubleType());
            default:
                UNREACHABLE();
        }
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    GateRef isLittleEndian = acc_.GetValueIn(gate, 3);  // 3: isLittleEndian
    GateRef inputIsLittleEndian = CheckAndConvertToBool(isLittleEndian, GateType::BooleanType());
    acc_.ReplaceValueIn(gate, inputIsLittleEndian, 3); // 3: replace input value to Bool
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitBigIntConstructor(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::BigIntType());
    }
    GateRef input = acc_.GetValueIn(gate, 0);
    TypeInfo typeInfo = GetOutputTypeInfo(input);
    const GateMetaData* meta = nullptr;
    switch (typeInfo) {
        case TypeInfo::INT32:
            meta = circuit_->BigIntConstructorInt32();
            break;
        case TypeInfo::UINT32:
            meta = circuit_->BigIntConstructorUint32();
            break;
        default:
            auto int32_cnst = TryConvertConstant(input, true);
            if (int32_cnst != Circuit::NullGate()) {
                acc_.ReplaceValueIn(gate, int32_cnst, 0);
                int32_t rawValue = acc_.GetInt32FromConstant(int32_cnst);
                if (rawValue < 0) {
                    meta = circuit_->BigIntConstructorInt32();
                } else {
                    meta = circuit_->BigIntConstructorUint32();
                }
            }
            break;
    }
    if (meta != nullptr) {
        // int or uint input
        acc_.SetMetaData(gate, meta);
    } else {
        // double, object or some other input
        acc_.ReplaceValueIn(gate, ConvertToTagged(input), 0);
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitDataViewSet(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::UndefinedType());
    }
    ASSERT(IsConvert());
    Environment env(gate, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef inputValue = CheckAndConvertToFloat64(value, acc_.GetGateType(value), ConvertToNumber::BOOL_ONLY);
    GateRef isLittleEndian = acc_.GetValueIn(gate, 4);  // 4: isLittleEndian
    GateRef inputIsLittleEndian = CheckAndConvertToBool(isLittleEndian, GateType::BooleanType());
    acc_.ReplaceValueIn(gate, inputValue, 2); // 2: replace input value to Double64
    acc_.ReplaceValueIn(gate, inputIsLittleEndian, 4); // 4: replace input value to Bool
    acc_.ReplaceStateIn(gate, builder_.GetState());
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitDateGetTime(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::TaggedValue());
    }
    ASSERT(IsConvert());
    // Nothing to do, because one input and it is object "this"
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitDateNow(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::DoubleType());
    }
    ASSERT(IsConvert());
    // Nothing to do, because don't have inputs
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitArrayIncludesIndexOf(GateRef gate)
{
    using Indices = CircuitArgIndices::ArrayIncludesIndexOf;
    Environment env(gate, circuit_, &builder_);
    GateRef callID = acc_.GetValueIn(gate, Indices::CALL_ID);
    BuiltinsStubCSigns::ID id = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(callID));
    if (IsRetype()) {
        if (id == BuiltinsStubCSigns::ID::ArrayIncludes) {
            return SetOutputType(gate, GateType::BooleanType());
        } else {
            return SetOutputType(gate, GateType::IntType());
        }
    }
    GateRef findElement = acc_.GetValueIn(gate, Indices::TARGET);
    acc_.ReplaceValueIn(gate, ConvertToTagged(findElement), Indices::TARGET);
    acc_.ReplaceDependIn(gate, builder_.GetDepend());
    acc_.ReplaceStateIn(gate, builder_.GetState());
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitStringCharCodeAt(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        GateRef thisValue = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate, ConvertToTagged(thisValue), 0);
        GateRef pos = acc_.GetValueIn(gate, 1);
        acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(pos, GateType::IntType()), 1);
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
    }
    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitString(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        GateRef thisValue = acc_.GetValueIn(gate, 0);
        acc_.ReplaceValueIn(gate, ConvertToTagged(thisValue), 0);
        auto argc = acc_.GetNumValueIn(gate);
        for (size_t i = 1; i < argc; i++) {
            GateRef arg = acc_.GetValueIn(gate, i);
            acc_.ReplaceValueIn(gate, CheckAndConvertToInt32(arg, GateType::IntType()), i);
        }
        acc_.ReplaceStateIn(gate, builder_.GetState());
        acc_.ReplaceDependIn(gate, builder_.GetDepend());
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
    ConvertMonoAccessorValueIn(gate);

    return Circuit::NullGate();
}

GateRef NumberSpeculativeRetype::VisitMonoCallGetterOnProto(GateRef gate)
{
    if (IsRetype()) {
        return SetOutputType(gate, GateType::AnyType());
    }
    if (IsConvert()) {
        ConvertMonoAccessorValueIn(gate);
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
            gate, CheckAndConvertToFloat64(value, GateType::NumberType(), ConvertToNumber::DISABLE), 4); // 4: value
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
