/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/typed_native_inline_lowering.h"
#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_object_stub_builder.h"
#include "ecmascript/compiler/circuit_arg_indices.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/type_info_accessors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_primitive_ref.h"

namespace panda::ecmascript::kungfu {
GateRef TypedNativeInlineLowering::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::MATH_COS:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatCos));
            break;
        case OpCode::MATH_COSH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatCosh));
            break;
        case OpCode::MATH_SIN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatSin));
            break;
        case OpCode::MATH_LOG:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog));
            break;
        case OpCode::MATH_LOG2:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog2));
            break;
        case OpCode::MATH_LOG10:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog10));
            break;
        case OpCode::MATH_LOG1P:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatLog1p));
            break;
        case OpCode::MATH_EXP:
            LowerMathExp(gate);
            break;
        case OpCode::MATH_EXPM1:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatExpm1));
            break;
        case OpCode::MATH_SINH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatSinh));
            break;
        case OpCode::MATH_ASINH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAsinh));
            break;
        case OpCode::MATH_TAN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatTan));
            break;
        case OpCode::MATH_ATAN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAtan));
            break;
        case OpCode::MATH_TANH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatTanh));
            break;
        case OpCode::MATH_ACOS:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAcos));
            break;
        case OpCode::MATH_ASIN:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAsin));
            break;
        case OpCode::MATH_ATANH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAtanh));
            break;
        case OpCode::MATH_ACOSH:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatAcosh));
            break;
        case OpCode::MATH_ATAN2:
            LowerMathAtan2(gate);
            break;
        case OpCode::MATH_ABS:
            LowerAbs(gate);
            break;
        case OpCode::MATH_ABS_INT32:
            LowerIntAbs(gate);
            break;
        case OpCode::MATH_ABS_DOUBLE:
            LowerDoubleAbs(gate);
            break;
        case OpCode::MATH_TRUNC:
            LowerTrunc(gate);
            break;
        case OpCode::MATH_POW:
            LowerMathPow(gate);
            break;
        case OpCode::MATH_CBRT:
            LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatCbrt));
            break;
        case OpCode::MATH_SIGN:
            LowerMathSignInt(gate);
            break;
        case OpCode::MATH_SIGN_TAGGED:
            LowerMathSignTagged(gate);
            break;
        case OpCode::MATH_MIN:
            LowerMinMax<false>(gate);
            break;
        case OpCode::MATH_MAX:
            LowerMinMax<true>(gate);
            break;
        case OpCode::MATH_MIN_INT32:
            LowerIntMinMax<false>(gate);
            break;
        case OpCode::MATH_MAX_INT32:
            LowerIntMinMax<true>(gate);
            break;
        case OpCode::MATH_MIN_DOUBLE:
            LowerDoubleMinMax<false>(gate);
            break;
        case OpCode::MATH_MAX_DOUBLE:
            LowerDoubleMinMax<true>(gate);
            break;
        case OpCode::MATH_CLZ32_DOUBLE:
            LowerClz32Float64(gate);
            break;
        case OpCode::MATH_CLZ32_INT32:
            LowerClz32Int32(gate);
            break;
        case OpCode::MATH_SQRT:
            LowerMathSqrt(gate);
            break;
        case OpCode::MATH_ROUND:
            LowerTaggedRounding(gate);
            break;
        case OpCode::MATH_ROUND_DOUBLE:
        case OpCode::MATH_FROUND:
            LowerDoubleRounding(gate);
            break;
        case OpCode::MATH_CEIL:
            LowerMathCeilFloor<true>(gate);
            break;
        case OpCode::MATH_FLOOR:
            LowerMathCeilFloor<false>(gate);
            break;
        case OpCode::MATH_IMUL:
            LowerMathImul(gate);
            break;
        case OpCode::NEW_NUMBER:
            LowerNewNumber(gate);
            break;
        case OpCode::GLOBAL_IS_FINITE:
            LowerGlobalIsFinite(gate);
            break;
        case OpCode::GLOBAL_IS_NAN:
            LowerGlobalIsNan(gate);
            break;
        case OpCode::DATA_VIEW_GET:
            LowerDataViewProtoFunc(gate, DataViewProtoFunc::GET);
            break;
        case OpCode::DATA_VIEW_SET:
            LowerDataViewProtoFunc(gate, DataViewProtoFunc::SET);
            break;
        case OpCode::ARRAY_BUFFER_IS_VIEW:
            LowerArrayBufferIsView(gate);
            break;
        case OpCode::BIGINT_ASINTN:
            LowerBigIntAsIntN<false>(gate);
            break;
        case OpCode::BIGINT_ASUINTN:
            LowerBigIntAsIntN<true>(gate);
            break;
        case OpCode::NUMBER_IS_FINITE:
            LowerNumberIsFinite(gate);
            break;
        case OpCode::NUMBER_IS_INTEGER:
        case OpCode::NUMBER_IS_SAFEINTEGER:
            LowerNumberIsInteger(gate, op);
            break;
        case OpCode::NUMBER_IS_NAN:
            LowerNumberIsNaN(gate);
            break;
        case OpCode::NUMBER_PARSE_FLOAT:
            LowerNumberParseFloat(gate);
            break;
        case OpCode::NUMBER_PARSE_INT:
            LowerNumberParseInt(gate);
            break;
        case OpCode::MAP_GET:
            LowerToCommonStub(gate, CommonStubCSigns::JSMapGet);
            break;
        case OpCode::DATE_GET_TIME:
            LowerDateGetTime(gate);
            break;
        case OpCode::MAP_HAS:
            LowerToCommonStub(gate, CommonStubCSigns::JSMapHas);
            break;
        case OpCode::MAP_KEYS:
            LowerToCommonStub(gate, CommonStubCSigns::JSMapKeys);
            break;
        case OpCode::MAP_VALUES:
            LowerToCommonStub(gate, CommonStubCSigns::JSMapValues);
            break;
        case OpCode::MAP_ENTRIES:
            LowerToCommonStub(gate, CommonStubCSigns::CreateJSMapIterator);
            break;
        case OpCode::SET_HAS:
            LowerToCommonStub(gate, CommonStubCSigns::JSSetHas);
            break;
        case OpCode::SET_ADD:
            LowerToCommonStub(gate, CommonStubCSigns::JSSetAdd);
            break;
        case OpCode::DATE_NOW:
            LowerGeneralWithoutArgs(gate, RTSTUB_ID(CallDateNow));
            break;
        case OpCode::TYPED_ARRAY_ENTRIES:
            LowerTypedArrayIterator(gate, CommonStubCSigns::CreateJSTypedArrayEntries,
                                    IterationKind::KEY_AND_VALUE);
            break;
        case OpCode::TYPED_ARRAY_KEYS:
            LowerTypedArrayIterator(gate, CommonStubCSigns::CreateJSTypedArrayKeys,
                                    IterationKind::KEY);
            break;
        case OpCode::TYPED_ARRAY_VALUES:
            LowerTypedArrayIterator(gate, CommonStubCSigns::CreateJSTypedArrayValues,
                                    IterationKind::VALUE);
            break;
        case OpCode::MAP_DELETE:
            LowerToCommonStub(gate, CommonStubCSigns::JSMapDelete);
            break;
        case OpCode::SET_DELETE:
            LowerToCommonStub(gate, CommonStubCSigns::JSSetDelete);
            break;
        case OpCode::SET_VALUES:
            LowerToCommonStub(gate, CommonStubCSigns::CreateJSSetIterator);
            break;
        case OpCode::SET_ENTRIES:
            LowerToCommonStub(gate, CommonStubCSigns::JSSetEntries);
            break;
        case OpCode::BIGINT_CONSTRUCTOR:
            LowerBigIntConstructor(gate);
            break;
        case OpCode::BIGINT_CONSTRUCTOR_INT32:
            LowerBigIntConstructorInt32<true>(gate);
            break;
        case OpCode::BIGINT_CONSTRUCTOR_UINT32:
            LowerBigIntConstructorInt32<false>(gate);
            break;
        case OpCode::STRING_CHAR_CODE_AT:
            LowerStringCharCodeAt(gate);
            break;
        case OpCode::STRING_SUB_STRING:
            LowerStringSubstring(gate);
            break;
        case OpCode::STRING_SUB_STR:
            LowerStringSubStr(gate);
            break;
        case OpCode::STRING_SLICE:
            LowerStringSlice(gate);
            break;
        case OpCode::OBJECT_IS:
            LowerObjectIs(gate);
            break;
        case OpCode::OBJECT_GET_PROTOTYPE_OF:
            LowerObjectGetPrototypeOf(gate);
            break;
        case OpCode::OBJECT_CREATE:
            LowerObjectCreate(gate);
            break;
        case OpCode::OBJECT_IS_PROTOTYPE_OF:
            LowerObjectIsPrototypeOf(gate);
            break;
        case OpCode::OBJECT_HAS_OWN_PROPERTY:
            LowerObjectHasOwnProperty(gate);
            break;
        case OpCode::REFLECT_GET_PROTOTYPE_OF:
            LowerReflectGetPrototypeOf(gate);
            break;
        case OpCode::REFLECT_GET:
            LowerReflectGet(gate);
            break;
        case OpCode::REFLECT_HAS:
            LowerReflectHas(gate);
            break;
        case OpCode::REFLECT_CONSTRUCT:
            LowerReflectConstruct(gate);
            break;
        case OpCode::REFLECT_APPLY:
            LowerReflectApply(gate);
            break;
        case OpCode::FUNCTION_PROTOTYPE_APPLY:
            LowerFunctionPrototypeApply(gate);
            break;
        case OpCode::FUNCTION_PROTOTYPE_BIND:
            LowerFunctionPrototypeBind(gate);
            break;
        case OpCode::FUNCTION_PROTOTYPE_CALL:
            LowerFunctionPrototypeCall(gate);
            break;
        case OpCode::ARRAY_INCLUDES_INDEXOF:
            LowerArrayIncludesIndexOf(gate);
            break;
        case OpCode::ARRAY_ITERATOR_BUILTIN:
            LowerArrayIteratorBuiltin(gate);
            break;
        case OpCode::ARRAY_FOR_EACH:
            LowerArrayForEach(gate);
            break;
        case OpCode::ARRAY_FIND_OR_FINDINDEX:
            LowerArrayFindOrFindIndex(gate);
            break;
        case OpCode::ARRAY_FILTER:
            LowerArrayFilter(gate);
            break;
        case OpCode::ARRAY_MAP:
            LowerArrayMap(gate);
            break;
        case OpCode::ARRAY_SOME:
            LowerArraySome(gate);
            break;
        case OpCode::ARRAY_EVERY:
            LowerArrayEvery(gate);
            break;
        case OpCode::ARRAY_POP:
            LowerArrayPop(gate);
            break;
        case OpCode::ARRAY_PUSH:
            LowerArrayPush(gate);
            break;
        case OpCode::ARRAY_SLICE:
            LowerArraySlice(gate);
            break;
        case OpCode::ARRAY_SORT:
            LowerArraySort(gate);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

template <bool IS_CEIL>
void TypedNativeInlineLowering::LowerMathCeilFloor(GateRef gate)
{
    if (builder_.GetCompilationConfig()->IsAArch64()) {
        LowerMathCeilFloorWithIntrinsic<IS_CEIL>(gate);
    } else {
        LowerMathCeilFloorWithRuntimeCall<IS_CEIL>(gate);
    }
}

template <bool IS_CEIL>
void TypedNativeInlineLowering::LowerMathCeilFloorWithIntrinsic(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.NanValue());
    GateRef arg = acc_.GetValueIn(gate, 0);

    if constexpr (IS_CEIL) {
        result = builder_.DoubleCeil(arg);
    } else {
        result = builder_.DoubleFloor(arg);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

template <bool IS_CEIL>
void TypedNativeInlineLowering::LowerMathCeilFloorWithRuntimeCall(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.NanValue());
    GateRef arg = acc_.GetValueIn(gate, 0);
    GateRef glue = acc_.GetGlueFromArgList();

    if constexpr (IS_CEIL) {
        result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatCeil), Gate::InvalidGateRef, {arg}, gate);
    } else {
        result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatFloor), Gate::InvalidGateRef, {arg}, gate);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypedNativeInlineLowering::AllocateArrayIterator(GateRef glue,
                                                         GateRef self,
                                                         GateRef iteratorHClass,
                                                         IterationKind iterationKind)
{
    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef kind = builder_.Int32(static_cast<int32_t>(iterationKind));

    builder_.StartAllocate();

    GateRef iterator = builder_.HeapAlloc(glue, builder_.IntPtr(JSArrayIterator::SIZE),
        GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);

    builder_.StoreConstOffset(VariableType::JS_POINTER(), iterator, TaggedObject::HCLASS_OFFSET,
                              iteratorHClass, MemoryAttribute::NeedBarrierAndAtomic());
    builder_.StoreConstOffset(VariableType::INT64(), iterator, JSObject::HASH_OFFSET,
        builder_.Int64(JSTaggedValue(0).GetRawData()));
    builder_.StoreConstOffset(VariableType::INT64(), iterator, JSObject::PROPERTIES_OFFSET, emptyArray);
    builder_.StoreConstOffset(VariableType::INT64(), iterator, JSObject::ELEMENTS_OFFSET, emptyArray);

    builder_.StoreConstOffset(VariableType::JS_ANY(), iterator, JSArrayIterator::ITERATED_ARRAY_OFFSET, self);
    builder_.StoreConstOffset(VariableType::INT32(), iterator, JSArrayIterator::NEXT_INDEX_OFFSET, builder_.Int32(0));
    builder_.StoreConstOffset(VariableType::INT32(), iterator, JSArrayIterator::BIT_FIELD_OFFSET, kind);

    GateRef result = builder_.FinishAllocate(iterator);
    builder_.SubCfgExit();

    return result;
}

void TypedNativeInlineLowering::LowerTypedArrayIterator(GateRef gate, CommonStubCSigns::ID index,
                                                        IterationKind iterationKind)
{
    Environment env(gate, circuit_, &builder_);

    GateRef glue = acc_.GetGlueFromArgList();
    GateRef self = acc_.GetValueIn(gate, 0);

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());

    Label selfExistsLabel(&env);
    Label isHeapObjectLabel(&env);
    Label isTypedArrayLabel(&env);
    Label selfValidLabel(&env);
    Label selfInvalidLabel(&env);
    Label exit(&env);

    GateRef selfExists = builder_.TaggedIsNotUndefinedAndNullAndHole(self);
    BRANCH_CIR(selfExists, &selfExistsLabel, &selfInvalidLabel);
    builder_.Bind(&selfExistsLabel);

    GateRef isHeapObject = builder_.TaggedIsHeapObject(self);
    BRANCH_CIR(isHeapObject, &isHeapObjectLabel, &selfInvalidLabel);
    builder_.Bind(&isHeapObjectLabel);

    GateRef isTypedArray = builder_.IsTypedArray(self);
    BRANCH_CIR(isTypedArray, &isTypedArrayLabel, &selfInvalidLabel);
    builder_.Bind(&isTypedArrayLabel);

    GateRef hasNoConstructor = builder_.BoolNot(builder_.HasConstructor(self));
    BRANCH_CIR(hasNoConstructor, &selfValidLabel, &selfInvalidLabel);
    builder_.Bind(&selfValidLabel);
    {
        GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env.Is32Bit()));
        GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        GateRef prototype = builder_.GetGlobalEnvValue(VariableType::JS_POINTER(), glueGlobalEnv,
                                                       GlobalEnv::ARRAY_ITERATOR_PROTOTYPE_INDEX);

        GateRef iteratorHClass = builder_.GetGlobalConstantValue(ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX);
        GateRef offset = builder_.IntPtr(JSHClass::PROTOTYPE_OFFSET);
        builder_.Store(VariableType::JS_POINTER(), glue, iteratorHClass, offset, prototype);

        result = AllocateArrayIterator(glue, self, iteratorHClass, iterationKind);
        builder_.Jump(&exit);
    }

    builder_.Bind(&selfInvalidLabel);
    {
        result = builder_.CallStub(glue, gate, index, { glue, self });
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypedNativeInlineLowering::LowerGlobalDoubleIsFinite(GateRef value)
{
    // set the sign bit to 0 by shift left then right.
    auto temp = builder_.Int64LSL(builder_.CastDoubleToInt64(value), builder_.Int64(1));
    auto res = builder_.Int64LSR(temp, builder_.Int64(1));
    auto abs = builder_.CastInt64ToFloat64(res);
    auto result = builder_.DoubleLessThan(abs, builder_.Double(base::POSITIVE_INFINITY));
    return result;
}

GateRef TypedNativeInlineLowering::LowerGlobalTNumberIsFinite(GateRef value)
{
    ASSERT(!acc_.GetGateType(value).IsNJSValueType());
    DEFVALUE(result, (&builder_), VariableType::BOOL(), builder_.Boolean(true));

    Label isInt(&builder_);
    Label notInt(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(value), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        result = builder_.Boolean(true);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        result = LowerGlobalDoubleIsFinite(value);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    return *result;
}

void TypedNativeInlineLowering::LowerGlobalIsFinite(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef result = LowerGlobalDoubleIsFinite(value);

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

GateRef TypedNativeInlineLowering::LowerGlobalTNumberIsNan(GateRef value)
{
    ASSERT(!acc_.GetGateType(value).IsNJSValueType());
    DEFVALUE(result, (&builder_), VariableType::BOOL(), builder_.Boolean(false));

    Label isInt(&builder_);
    Label notInt(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(value), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        result = builder_.Boolean(false);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        result = builder_.DoubleIsNAN(value);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    return *result;
}

void TypedNativeInlineLowering::LowerGlobalIsNan(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef result = builder_.DoubleIsNAN(value);

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerMathPow(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef base = acc_.GetValueIn(gate, 0);
    GateRef exp = acc_.GetValueIn(gate, 1);
    Label exit(&builder_);
    Label notNan(&builder_);

    auto nanValue = builder_.NanValue();
    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), nanValue);

    const double doubleOne = 1.0;
    // Base is 1.0, exponent is inf => NaN
    // Exponent is not finit, if is NaN or is Inf
    GateRef resultIsNan = LogicOrBuilder(&env)
        .Or(builder_.DoubleIsNAN(exp))
        .Or(LogicAndBuilder(&env)
            .And(builder_.DoubleEqual(builder_.FAbs(base), builder_.Double(doubleOne)))
            .And(builder_.DoubleIsINF(exp))
            .Done())
        .Done();

    BRANCH_CIR(resultIsNan, &exit, &notNan);
    builder_.Bind(&notNan);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatPow), Gate::InvalidGateRef, {base, exp}, gate);
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerMathExp(GateRef gate)
{
#ifdef SUPPORT_LLVM_INTRINSICS_WITH_CALLS
    Environment env(gate, circuit_, &builder_);
    constexpr double one = 1.0;
    GateRef base = builder_.Double(std::exp(one));
    GateRef power = acc_.GetValueIn(gate, 0U);

    GateRef exp = builder_.DoubleExp(base, power);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), exp);
#else
    LowerGeneralUnaryMath(gate, RTSTUB_ID(FloatExp));
#endif
}

void TypedNativeInlineLowering::LowerMathImul(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef val1 = acc_.GetValueIn(gate, 0);
    GateRef val2 = acc_.GetValueIn(gate, 1);
    ASSERT(acc_.GetGateType(val1).IsNJSValueType() && acc_.GetGateType(val2).IsNJSValueType());

    GateRef result = builder_.Int32Mul(val1, val2);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedNativeInlineLowering::LowerGeneralUnaryMath(GateRef gate, RuntimeStubCSigns::ID stubId)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef result = builder_.CallNGCRuntime(glue, stubId, Gate::InvalidGateRef, {value}, gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedNativeInlineLowering::LowerMathAtan2(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef y = acc_.GetValueIn(gate, 0);
    GateRef x = acc_.GetValueIn(gate, 1);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef result = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatAtan2), Gate::InvalidGateRef, {y, x}, gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

//  Int abs : The internal representation of an integer is inverse code,
//  The absolute value of a negative number can be found by inverting it by adding one.
GateRef TypedNativeInlineLowering::BuildIntAbs(GateRef value)
{
    ASSERT(acc_.GetMachineType(value) == MachineType::I32);
    if (isLiteCG_) {
        auto temp = builder_.Int32ASR(value, builder_.Int32(JSTaggedValue::INT_SIGN_BIT_OFFSET));
        auto res = builder_.Int32Xor(value, temp);
        return builder_.Int32Sub(res, temp);
    }
    return builder_.Abs(value);
}

//  Float abs : A floating-point number is composed of mantissa and exponent.
//  The length of mantissa will affect the precision of the number, and its sign will determine the sign of the number.
//  The absolute value of a floating-point number can be found by setting mantissa sign bit to 0.
GateRef TypedNativeInlineLowering::BuildDoubleAbs(GateRef value)
{
    ASSERT(acc_.GetMachineType(value) == MachineType::F64);
    if (isLiteCG_) {
        // set the sign bit to 0 by shift left then right.
        auto temp = builder_.Int64LSL(builder_.CastDoubleToInt64(value), builder_.Int64(1));
        auto res = builder_.Int64LSR(temp, builder_.Int64(1));
        return builder_.CastInt64ToFloat64(res);
    }
    return builder_.FAbs(value);
}

GateRef TypedNativeInlineLowering::BuildTNumberAbs(GateRef param)
{
    ASSERT(!acc_.GetGateType(param).IsNJSValueType());
    Label entry(&builder_);
    builder_.SubCfgEntry(&entry);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label exit(&builder_);
    Label isInt(&builder_);
    Label notInt(&builder_);
    Label isIntMin(&builder_);
    Label isResultInt(&builder_);
    Label intExit(&builder_);
    BRANCH_CIR(builder_.TaggedIsInt(param), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        auto value = builder_.GetInt32OfTInt(param);
        BRANCH_CIR(builder_.Equal(value, builder_.Int32(INT32_MIN)), &isIntMin, &isResultInt);
        builder_.Bind(&isResultInt);
        {
            result = builder_.Int32ToTaggedPtr(BuildIntAbs(value));
            builder_.Jump(&intExit);
        }
        builder_.Bind(&isIntMin);
        {
            result = builder_.DoubleToTaggedDoublePtr(builder_.Double(-static_cast<double>(INT_MIN)));
            builder_.Jump(&intExit);
        }
        // Aot compiler fails without jump to intermediate label
        builder_.Bind(&intExit);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        auto value = builder_.GetDoubleOfTDouble(param);
        result = builder_.DoubleToTaggedDoublePtr(BuildDoubleAbs(value));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    GateRef res = *result;
    builder_.SubCfgExit();
    return res;
}

void TypedNativeInlineLowering::LowerAbs(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef res = BuildTNumberAbs(value);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), res);
}

GateRef TypedNativeInlineLowering::BuildRounding(GateRef gate, GateRef value, OpCode op)
{
    if (op == OpCode::MATH_ROUND || op == OpCode::MATH_ROUND_DOUBLE) {
        Label entry(&builder_);
        builder_.SubCfgEntry(&entry);
        const double diff = 0.5;
        GateRef diffValue = builder_.Double(diff);
        const double zero = 0.0;
        Label subOne(&builder_);
        Label exit(&builder_);
        Label retCeil(&builder_);
        Label nonZero(&builder_);
        DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.Double(zero));
        // 0 <= x < 0.5, return 0
        GateRef returnZero = builder_.BitAnd(builder_.DoubleLessThan(value, diffValue),
            builder_.DoubleGreaterThan(value, builder_.Double(zero)));
        BRANCH_CIR(returnZero, &exit, &nonZero);
        builder_.Bind(&nonZero);
        {
            GateRef rounded;
            if (builder_.GetCompilationConfig()->IsAArch64() && !isLiteCG_) {
                rounded = builder_.DoubleCeil(value);
            } else {
                GateRef glue = acc_.GetGlueFromArgList();
                rounded = builder_.CallNGCRuntime(glue, RTSTUB_ID(FloatCeil), Gate::InvalidGateRef, {value}, gate);
            }
            // if ceil(x) - x > 0.5, return ceil(x) - 1
            // else return ceil(x)
            BRANCH_CIR(builder_.DoubleGreaterThan(builder_.DoubleSub(rounded, value), diffValue),
                &subOne, &retCeil);
            builder_.Bind(&subOne);
            {
                result = builder_.DoubleSub(rounded, builder_.Double(1U));
                builder_.Jump(&exit);
            }
            builder_.Bind(&retCeil);
            result = rounded;
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
        GateRef res = *result;
        builder_.SubCfgExit();
        return res;
    } else if (op == OpCode::MATH_FROUND) {
        return builder_.ExtFloat32ToDouble(builder_.TruncDoubleToFloat32(value));
    } else {
        UNREACHABLE();
    }
}

void TypedNativeInlineLowering::LowerTaggedRounding(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.NanValue());

    GateRef in = acc_.GetValueIn(gate, 0);
    Label isInt(&builder_);
    Label isDouble(&builder_);
    Label exit(&builder_);

    builder_.Branch(builder_.TaggedIsInt(in), &isInt, &isDouble);
    builder_.Bind(&isInt);
    {
        result = builder_.GetDoubleOfTInt(in);
        builder_.Jump(&exit);
    }
    builder_.Bind(&isDouble);
    {
        GateRef value = builder_.GetDoubleOfTDouble(in);
        result = BuildRounding(gate, value, acc_.GetOpCode(gate));
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerDoubleRounding(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef res = BuildRounding(gate, value, acc_.GetOpCode(gate));
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), res);
}

void TypedNativeInlineLowering::LowerIntAbs(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    auto frameState = FindFrameState(gate);
    builder_.DeoptCheck(builder_.NotEqual(value, builder_.Int32(INT32_MIN)), frameState, DeoptType::NOTINT3);
    GateRef res = BuildIntAbs(value);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), res);
}

void TypedNativeInlineLowering::LowerDoubleAbs(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef res = BuildDoubleAbs(value);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), res);
}

// for min select in1 if int1 < int2, in2 otherwise
template<bool IS_MAX>
GateRef TypedNativeInlineLowering::BuildIntMinMax(GateRef int1, GateRef int2, GateRef in1, GateRef in2)
{
    Label entry(&builder_);
    builder_.SubCfgEntry(&entry);
    // int or tagged
    VariableType type {acc_.GetMachineType(in1), acc_.GetGateType(in1)};
    DEFVALUE(result, (&builder_), type, (IS_MAX ? in1 : in2));
    Label left(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.Int32LessThan(int1, int2), &left, &exit);
    builder_.Bind(&left);
    {
        result = IS_MAX ? in2 : in1;
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    GateRef res = *result;
    builder_.SubCfgExit();
    return res;
}

template<bool IS_MAX>
GateRef TypedNativeInlineLowering::BuildIntMinMax(GateRef in1, GateRef in2)
{
    ASSERT(acc_.GetMachineType(in1) == MachineType::I32);
    ASSERT(acc_.GetMachineType(in2) == MachineType::I32);
    if (isLiteCG_) {
        return BuildIntMinMax<IS_MAX>(in1, in2, in1, in2);
    }
    return IS_MAX ? builder_.Int32Max(in1, in2) : builder_.Int32Min(in1, in2);
}

/* for min select:
 * NaN if double1 or double2 is NaN
 * in1 if double1 and double2 are equal and in1 is negative zero
 * in1 if double1 < double2, in2 otherwise */
template<bool IS_MAX>
GateRef TypedNativeInlineLowering::BuildDoubleMinMax(GateRef double1, GateRef double2, GateRef in1, GateRef in2)
{
    Label entry(&builder_);
    builder_.SubCfgEntry(&entry);
    GateRef nanValue = builder_.NanValue();
    if (in1 != double1) { // case when in1 and in2 are tagged
        nanValue = builder_.DoubleToTaggedDoublePtr(nanValue);
    }
    // double or tagged
    VariableType type {acc_.GetMachineType(in1), acc_.GetGateType(in1)};
    DEFVALUE(result, (&builder_), type, nanValue);
    Label left(&builder_);
    Label rightOrZeroOrNan(&builder_);
    Label right(&builder_);
    Label exit(&builder_);
    Label equal(&builder_);
    Label equalOrNan(&builder_);
    builder_.Branch(builder_.DoubleLessThan(double1, double2), &left, &rightOrZeroOrNan);
    builder_.Bind(&rightOrZeroOrNan);
    {
        builder_.Branch(builder_.DoubleGreaterThan(double1, double2), &right, &equalOrNan);
        builder_.Bind(&equalOrNan);
        {
            builder_.Branch(builder_.DoubleEqual(double1, double2), &equal, &exit);
            builder_.Bind(&equal);
            {
                // Whether to return in1 or in2 matters only in case of 0, -0
                const double negZero = -0.0;
                GateRef negZeroValue = builder_.CastDoubleToInt64(builder_.Double(negZero));
                builder_.Branch(builder_.Equal(builder_.CastDoubleToInt64(double1), negZeroValue), &left, &right);
            }
        }
        builder_.Bind(&right);
        {
            result = IS_MAX ? in1 : in2;
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&left);
    {
        result = IS_MAX ? in2 : in1;
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    GateRef res = *result;
    builder_.SubCfgExit();
    return res;
}

template<bool IS_MAX>
GateRef TypedNativeInlineLowering::BuildDoubleMinMax(GateRef in1, GateRef in2)
{
    ASSERT(acc_.GetMachineType(in1) == MachineType::F64);
    ASSERT(acc_.GetMachineType(in2) == MachineType::F64);
    if (!isLiteCG_ && builder_.GetCompilationConfig()->IsAArch64()) {
        return IS_MAX ? builder_.DoubleMax(in1, in2) : builder_.DoubleMin(in1, in2);
    }
    return BuildDoubleMinMax<IS_MAX>(in1, in2, in1, in2);
}

template<bool IS_MAX>
void TypedNativeInlineLowering::LowerTNumberMinMax(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef in1 = acc_.GetValueIn(gate, 0);
    GateRef in2 = acc_.GetValueIn(gate, 1);
    GateRef nanValue = builder_.DoubleToTaggedDoublePtr(builder_.NanValue());
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), nanValue);
    DEFVALUE(double1, (&builder_), VariableType::FLOAT64(), builder_.Double(0));
    DEFVALUE(double2, (&builder_), VariableType::FLOAT64(), builder_.Double(0));

    Label isInt1(&builder_);
    Label isInt2(&builder_);
    Label isDouble1(&builder_);
    Label isDouble2(&builder_);
    Label doubleExit(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsInt(in1), &isInt1, &isDouble1);
    {
        builder_.Bind(&isInt1);
        GateRef int1 = builder_.GetInt32OfTInt(in1);
        builder_.Branch(builder_.TaggedIsInt(in2), &isInt2, &isDouble2);
        {
            builder_.Bind(&isInt2);
            GateRef int2 = builder_.GetInt32OfTInt(in2);
            result = BuildIntMinMax<IS_MAX>(int1, int2, in1, in2);
            builder_.Jump(&exit);
        }
        builder_.Bind(&isDouble2);
        double1 = builder_.ChangeInt32ToFloat64(int1);
        double2 = builder_.GetDoubleOfTDouble(in2);
        builder_.Jump(&doubleExit);
    }
    {
        builder_.Bind(&isDouble1);
        double1 = builder_.GetDoubleOfTDouble(in1);
        double2 = builder_.GetDoubleOfTNumber(in2);
        builder_.Jump(&doubleExit);
    }
    builder_.Bind(&doubleExit);
    result = BuildDoubleMinMax<IS_MAX>(*double1, *double2, in1, in2);
    builder_.Jump(&exit);

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

template<bool IS_MAX>
void TypedNativeInlineLowering::LowerMathMinMaxWithIntrinsic(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef in1 = acc_.GetValueIn(gate, 0);
    GateRef in2 = acc_.GetValueIn(gate, 1);
    GateRef nanValue = builder_.DoubleToTaggedDoublePtr(builder_.NanValue());
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), nanValue);

    Label intRes(&builder_);
    Label doubleRes(&builder_);
    Label exit(&builder_);

    BRANCH_CIR(LogicAndBuilder(&env).And(builder_.TaggedIsInt(in1)).And(builder_.TaggedIsInt(in2)).Done(),
        &intRes, &doubleRes);
    builder_.Bind(&intRes);
    {
        GateRef int1 = builder_.GetInt32OfTInt(in1);
        GateRef int2 = builder_.GetInt32OfTInt(in2);
        GateRef intRet = BuildIntMinMax<IS_MAX>(int1, int2);
        result = builder_.Int32ToTaggedPtr(intRet);
        builder_.Jump(&exit);
    }
    builder_.Bind(&doubleRes);
    {
        GateRef double1 = builder_.GetDoubleOfTNumber(in1);
        GateRef double2 = builder_.GetDoubleOfTNumber(in2);
        // LLVM supports lowering of `minimum/maximum` intrinsics on X86 only since version 17
        // see https://github.com/llvm/llvm-project/commit/a82d27a9a6853c96f857ba0f514a78cd03bc5c35
        if (builder_.GetCompilationConfig()->IsAArch64()) {
            GateRef doubleRet = IS_MAX ? builder_.DoubleMax(double1, double2) : builder_.DoubleMin(double1, double2);
            result = builder_.DoubleToTaggedDoublePtr(doubleRet);
        } else {
            result = BuildDoubleMinMax<IS_MAX>(double1, double2, in1, in2);
        }
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

template<bool IS_MAX>
void TypedNativeInlineLowering::LowerMinMax(GateRef gate)
{
    if (isLiteCG_) {
        LowerTNumberMinMax<IS_MAX>(gate);
    } else {
        LowerMathMinMaxWithIntrinsic<IS_MAX>(gate);
    }
}

template<bool IS_MAX>
void TypedNativeInlineLowering::LowerIntMinMax(GateRef gate)
{
    GateRef in1 = acc_.GetValueIn(gate, 0);
    GateRef in2 = acc_.GetValueIn(gate, 1);
    Environment env(gate, circuit_, &builder_);
    GateRef res = BuildIntMinMax<IS_MAX>(in1, in2);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), res);
}

template<bool IS_MAX>
void TypedNativeInlineLowering::LowerDoubleMinMax(GateRef gate)
{
    GateRef in1 = acc_.GetValueIn(gate, 0);
    GateRef in2 = acc_.GetValueIn(gate, 1);
    Environment env(gate, circuit_, &builder_);
    GateRef res = BuildDoubleMinMax<IS_MAX>(in1, in2);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), res);
}

GateRef TypedNativeInlineLowering::FindFrameState(GateRef gate)
{
    while (!acc_.HasFrameState(gate)) {
        ASSERT(acc_.GetDependCount(gate) > 0);
        gate = acc_.GetDep(gate);
    }
    return acc_.GetFrameState(gate);
}

void TypedNativeInlineLowering::LowerClz32Float64(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label exit(&builder_);
    Label isFinit(&builder_);

    GateRef param = acc_.GetValueIn(gate, 0);
    const int32_t defaultReturnValue = 32;
    DEFVALUE(result, (&builder_), VariableType::INT32(), builder_.Int32(defaultReturnValue));

    // NaN, Inf, -Inf after ToUint32 equal 0, so we in advance know result: Clz32(0) = 32
    builder_.Branch(builder_.DoubleIsNanOrInf(param), &exit, &isFinit);
    builder_.Bind(&isFinit);
    {
        auto truncedValue = builder_.TruncInt64ToInt32(builder_.TruncFloatToInt64(param));
        result = builder_.CountLeadingZeroes32(truncedValue);
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerClz32Int32(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef param = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.CountLeadingZeroes32(param);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

//  Int trunc(x) : return x
//  Float trunc(x) : return the integer part removing all fractional digits
void TypedNativeInlineLowering::LowerTrunc(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef param = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.NanValue());

    Label isInt(&builder_);
    Label notInt(&builder_);
    Label isDouble(&builder_);
    Label exit(&builder_);

    BRANCH_CIR(builder_.TaggedIsInt(param), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        result = builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(param));
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        BRANCH_CIR(builder_.TaggedIsDouble(param), &isDouble, &exit);
        builder_.Bind(&isDouble);
        {
            GateRef input = builder_.GetDoubleOfTDouble(param);
            result = builder_.DoubleTrunc(gate, input);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerMathSqrt(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    builder_.SetEnvironment(&env);
    GateRef param = acc_.GetValueIn(gate, 0);
    // 20.2.2.32
    // If value is NAN or negative, include -NaN and -Infinity but not -0.0, the result is NaN
    // Assembly instruction support NAN and negative
    auto ret = builder_.Sqrt(param);
    acc_.SetMachineType(ret, MachineType::F64);
    acc_.SetGateType(ret, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}


GateRef AllocateNewNumber(const CompilationEnv *compilationEnv, CircuitBuilder *builder, GateAccessor acc,
                          GateRef protoOrHclass, GateRef result)
{
    Jit::JitLockHolder lock(compilationEnv, "AllocateNewNumber");
    JSHandle<JSFunction> numberFunctionCT(compilationEnv->GetGlobalEnv()->GetNumberFunction());
    JSTaggedValue protoOrHClassCT = numberFunctionCT->GetProtoOrHClass();
    JSHClass *numberHClassCT = JSHClass::Cast(protoOrHClassCT.GetTaggedObject());
    size_t objectSize = numberHClassCT->GetObjectSize();

    GateRef size = builder->IntPtr(objectSize);

    GateRef emptyArray = builder->GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    builder->StartAllocate();
    GateRef object = builder->HeapAlloc(acc.GetGlueFromArgList(), size, GateType::TaggedValue(),
                                        RegionSpaceFlag::IN_YOUNG_SPACE);
    // Initialization:
    builder->StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::HCLASS_OFFSET, protoOrHclass,
                              MemoryAttribute::NeedBarrierAndAtomic());
    builder->StoreConstOffset(VariableType::INT64(), object, JSObject::HASH_OFFSET,
                              builder->Int64(JSTaggedValue(0).GetRawData()));
    builder->StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::PROPERTIES_OFFSET, emptyArray,
                              MemoryAttribute::NoBarrier());
    builder->StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::ELEMENTS_OFFSET, emptyArray,
                              MemoryAttribute::NoBarrier());
    builder->StoreConstOffset(VariableType::JS_ANY(), object, JSPrimitiveRef::VALUE_OFFSET, result);
    auto offset = JSPrimitiveRef::VALUE_OFFSET + JSTaggedValue::TaggedTypeSize();
    // Initialize inlined properties:
    for (; offset < objectSize; offset += JSTaggedValue::TaggedTypeSize()) {
        builder->StoreConstOffset(VariableType::INT64(), object, offset, builder->Undefined());
    }
    return builder->FinishAllocate(object);
}

void TypedNativeInlineLowering::LowerNewNumber(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef numberFunction = acc_.GetValueIn(gate, 0);
    GateRef param = acc_.GetValueIn(gate, 1);

    GateRef globalEnvNumberFunction = builder_.GetGlobalEnvValue(VariableType::JS_ANY(), builder_.GetGlobalEnv(),
                                                                 GlobalEnv::NUMBER_FUNCTION_INDEX);
    auto builtinIsNumber = builder_.Equal(numberFunction, globalEnvNumberFunction);
    builder_.DeoptCheck(builtinIsNumber, FindFrameState(gate), DeoptType::NEWBUILTINCTORFAIL1);

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(),
             builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(builder_.Int32(0))));
    Label exit(&env);
    Label isNumber(&env);
    Label notNumber(&env);
    BRANCH_CIR(builder_.TaggedIsNumber(param), &isNumber, &notNumber);
    builder_.Bind(&isNumber);
    {
        result = param;
        builder_.Jump(&exit);
    }
    builder_.Bind(&notNumber);
    {
        builder_.DeoptCheck(builder_.TaggedIsString(param), FindFrameState(gate), DeoptType::NOTSTRING1);
        auto isIntString = builder_.IsIntegerString(param);
        builder_.DeoptCheck(isIntString, FindFrameState(gate), DeoptType::NOTINT1);
        result = builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(builder_.GetRawHashFromString(param)));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);

    auto protoOrHclass = builder_.LoadConstOffset(VariableType::JS_POINTER(), numberFunction,
                                                  JSFunction::PROTO_OR_DYNCLASS_OFFSET);

    GateRef ret = AllocateNewNumber(compilationEnv_, &builder_, acc_, protoOrHclass, *result);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void TypedNativeInlineLowering::LowerArrayBufferIsView(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef arg = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedFalse());
    Label exit(&builder_);
    Label isDataViewOrTypedArray(&builder_);
    Label returnTaggedTrue(&builder_);
    BRANCH_CIR(builder_.IsEcmaObject(arg), &isDataViewOrTypedArray, &exit);
    builder_.Bind(&isDataViewOrTypedArray);
    {
        BRANCH_CIR(LogicOrBuilder(&env).Or(builder_.CheckJSType(arg, JSType::JS_DATA_VIEW))
            .Or(builder_.TaggedObjectIsTypedArray(arg)).Done(),
            &returnTaggedTrue, &exit);
    }
    builder_.Bind(&returnTaggedTrue);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

template <bool IS_UNSIGNED>
void TypedNativeInlineLowering::LowerBigIntAsIntN(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label hasException(&builder_);
    Label exit(&builder_);
    Label returnBigInt(&builder_);
    Label notZeroBigInt(&builder_);
    Label commonCase(&builder_);
#if BIGINT_CONSTRUCTOR_IMPLEMENTED // NOTE: add fastpath after BigInt constructor implementing
    Label zeroBits(&builder_);
    Label notZeroBits(&builder_);
#endif // BIGINT_CONSTRUCTOR_IMPLEMENTED

    GateRef glue = acc_.GetGlueFromArgList();
    GateRef bits = acc_.GetValueIn(gate, 0);
    GateRef bigint = acc_.GetValueIn(gate, 1);
    GateRef frameState = acc_.GetValueIn(gate, 2);
    GateRef bitness = builder_.GetDoubleOfTDouble(bits);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());

    // Deoptimization if bitness is negative or more than safe number
    GateRef safeNumber = builder_.Double(SAFE_NUMBER);
    GateRef positiveCheck = builder_.DoubleGreaterThanOrEqual(bitness, builder_.Double(0));
    GateRef safeCheck = builder_.DoubleLessThanOrEqual(bitness, safeNumber);
    builder_.DeoptCheck(positiveCheck, frameState, DeoptType::RANGE_ERROR);
    builder_.DeoptCheck(safeCheck, frameState, DeoptType::RANGE_ERROR);
    builder_.DeoptCheck(builder_.TaggedIsBigInt(bigint), frameState, DeoptType::NOT_BIG_INT);

    // Return bigint(0), if bits == 0
#if BIGINT_CONSTRUCTOR_IMPLEMENTED // NOTE: add fastpath after BigInt constructor implementing
    BRANCH_CIR(builder_.DoubleEqual(bitness, builder_.Double(0)), &zeroBits, &notZeroBits);
    builder_.Bind(&zeroBits);
    {
        result = builder_.BigIntConstructor(0);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notZeroBits);
#endif // BIGINT_CONSTRUCTOR_IMPLEMENTED

    // Return bigint, if bigint == 0
    GateRef isZeroBigInt = LogicAndBuilder(&env)
        .And(builder_.Int32Equal(builder_.Load(VariableType::INT32(), bigint, builder_.IntPtr(BigInt::LENGTH_OFFSET)),
                                 builder_.Int32(1)))
        .And(builder_.Int32Equal(builder_.Load(VariableType::INT32(), bigint, builder_.IntPtr(BigInt::DATA_OFFSET)),
                                 builder_.Int32(0)))
        .Done();
    BRANCH_CIR(isZeroBigInt, &returnBigInt, &notZeroBigInt);

    // Return bigint, if bits >= max_value
    builder_.Bind(&notZeroBigInt);
    GateRef maxLengthBits = builder_.Double(static_cast<double>(BigInt::kMaxLengthBits));
    BRANCH_CIR(builder_.DoubleGreaterThanOrEqual(bitness, maxLengthBits), &returnBigInt, &commonCase);
    builder_.Bind(&returnBigInt);
    {
        result = bigint;
        builder_.Jump(&exit);
    }

    // Common case
    builder_.Bind(&commonCase);
    if constexpr (IS_UNSIGNED) {
        result = builder_.CallRuntime(glue, RTSTUB_ID(CallBigIntAsUintN), Gate::InvalidGateRef,
                                      {bits, bigint}, gate);
    } else {
        result = builder_.CallRuntime(glue, RTSTUB_ID(CallBigIntAsIntN), Gate::InvalidGateRef,
                                      {bits, bigint}, gate);
    }
    BRANCH_CIR(builder_.HasPendingException(glue), &hasException, &exit);
    builder_.Bind(&hasException);
    {
        result = builder_.ExceptionConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerDataViewProtoFunc(GateRef gate, DataViewProtoFunc func)
{
    Environment env(gate, circuit_, &builder_);

    Label isNotDetachedBuffer(&builder_);
    Label isNotByteArray(&builder_);
    Label getPointFromByteArray(&builder_);
    Label getPointFromNotByteArray(&builder_);
    Label getValueFromBuffer(&builder_);
    Label bufferByteLengthIsZero(&builder_);
    Label bufferByteLengthIsNotZero(&builder_);
    GateRef thisobj = acc_.GetValueIn(gate, 0);
    GateRef requestIndex = acc_.GetValueIn(gate, 1); // 1: requestIndex
    GateRef builtinId = Circuit::NullGate();
    GateRef isLittleEndian = Circuit::NullGate();
    GateRef frameState = Circuit::NullGate();
    ASSERT(func == DataViewProtoFunc::GET || func == DataViewProtoFunc::SET);
    if (func == DataViewProtoFunc::GET) {
        builtinId = acc_.GetValueIn(gate, 2); // 2: builtinId
        isLittleEndian = acc_.GetValueIn(gate, 3); // 3: isLittleEndian
        frameState = acc_.GetValueIn(gate, 4); // 4: frameState
    } else if (func == DataViewProtoFunc::SET) {
        builtinId = acc_.GetValueIn(gate, 3); // 3: builtinId
        isLittleEndian = acc_.GetValueIn(gate, 4); // 4: isLittleEndian
        frameState = acc_.GetValueIn(gate, 5); // 5: frameState
    } else {
        UNREACHABLE();
    }

    GateRef resultfinal = Circuit::NullGate();

    DEFVALUE(dataPointer, (&builder_), VariableType::NATIVE_POINTER(), builder_.IntPtr(0));

    builder_.DeoptCheck(
        builder_.Int32GreaterThanOrEqual(requestIndex, builder_.Int32(0)), frameState, DeoptType::INDEXLESSZERO);
    GateRef viewedArrayBufferOffset = builder_.IntPtr(JSDataView::VIEW_ARRAY_BUFFER_OFFSET);
    GateRef buffer = builder_.Load(VariableType::JS_ANY(), thisobj, viewedArrayBufferOffset);
    BRANCH_CIR(builder_.CheckJSType(buffer, JSType::BYTE_ARRAY), &isNotDetachedBuffer, &isNotByteArray);
    builder_.Bind(&isNotByteArray);
    {
        GateRef dataOffset = builder_.IntPtr(JSArrayBuffer::DATA_OFFSET);
        GateRef dataSlot = builder_.Load(VariableType::JS_ANY(), buffer, dataOffset);
        builder_.DeoptCheck(builder_.TaggedIsNotNull(dataSlot), frameState, DeoptType::ARRAYBUFFERISDETACHED);
        builder_.Jump(&isNotDetachedBuffer);
    }
    builder_.Bind(&isNotDetachedBuffer);
    GateRef sizeOffset = builder_.IntPtr(JSDataView::BYTE_LENGTH_OFFSET);
    GateRef size = builder_.ZExtInt32ToInt64(builder_.Load(VariableType::INT32(), thisobj, sizeOffset));
    GateRef elementSize = BuiltinIdToSize(builtinId);
    GateRef totalSize = builder_.Int64Add(builder_.ZExtInt32ToInt64(requestIndex), elementSize);

    builder_.DeoptCheck(
        builder_.Int64UnsignedGreaterThanOrEqual(size, totalSize), frameState, DeoptType::TOTALSIZEOVERFLOW);
    GateRef byteOffset = builder_.IntPtr(JSDataView::BYTE_OFFSET_OFFSET);
    GateRef offset = builder_.Load(VariableType::INT32(), thisobj, byteOffset);
    GateRef bufferIndex = builder_.Int32Add(requestIndex, offset);
    BRANCH_CIR(builder_.CheckJSType(buffer, JSType::BYTE_ARRAY), &getPointFromByteArray, &getPointFromNotByteArray);
    builder_.Bind(&getPointFromByteArray);
    {
        dataPointer = builder_.Load(VariableType::NATIVE_POINTER(), buffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        builder_.Jump(&getValueFromBuffer);
    }
    builder_.Bind(&getPointFromNotByteArray);
    {
        GateRef arrayBufferByteLengthOffset = builder_.IntPtr(JSArrayBuffer::BYTE_LENGTH_OFFSET);
        GateRef arrayBufferByteLength = builder_.Load(VariableType::INT32(), buffer, arrayBufferByteLengthOffset);
        BRANCH_CIR(builder_.Int32Equal(arrayBufferByteLength, builder_.Int32(0)),
                   &bufferByteLengthIsZero,
                   &bufferByteLengthIsNotZero);
        builder_.Bind(&bufferByteLengthIsZero);
        {
            dataPointer = builder_.IntPtr(0);
            builder_.Jump(&getValueFromBuffer);
        }
        builder_.Bind(&bufferByteLengthIsNotZero);
        {
            GateRef bufferDataOffset = builder_.IntPtr(JSArrayBuffer::DATA_OFFSET);
            GateRef data = builder_.Load(VariableType::JS_ANY(), buffer, bufferDataOffset);
            GateRef externalPointerOffset = builder_.IntPtr(JSNativePointer::POINTER_OFFSET);
            GateRef externalPointer = builder_.Load(VariableType::NATIVE_POINTER(), data, externalPointerOffset);
            dataPointer = externalPointer;
            builder_.Jump(&getValueFromBuffer);
        }
    }
    builder_.Bind(&getValueFromBuffer);
    ASSERT(func == DataViewProtoFunc::GET || func == DataViewProtoFunc::SET);
    if (func == DataViewProtoFunc::GET) {
        resultfinal = GetValueFromBuffer(bufferIndex, *dataPointer, isLittleEndian, builtinId);
    } else if (func == DataViewProtoFunc::SET) {
        GateRef value = acc_.GetValueIn(gate, 2); // 2: value
        resultfinal =
            SetValueInBuffer(bufferIndex, value, *dataPointer, isLittleEndian, builtinId, acc_.GetGlueFromArgList());
    } else {
        UNREACHABLE();
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), resultfinal);
}

GateRef TypedNativeInlineLowering::BuiltinIdToSize(GateRef ID)
{
    auto builtinsID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(ID));
    switch (builtinsID) {
        case BuiltinsStubCSigns::ID::DataViewGetInt8:
        case BuiltinsStubCSigns::ID::DataViewGetUint8:
        case BuiltinsStubCSigns::ID::DataViewSetUint8:
        case BuiltinsStubCSigns::ID::DataViewSetInt8:
            return builder_.Int64(ElmentSize::BITS_8);
        case BuiltinsStubCSigns::ID::DataViewGetInt16:
        case BuiltinsStubCSigns::ID::DataViewGetUint16:
        case BuiltinsStubCSigns::ID::DataViewSetInt16:
        case BuiltinsStubCSigns::ID::DataViewSetUint16:
            return builder_.Int64(ElmentSize::BITS_16);
        case BuiltinsStubCSigns::ID::DataViewGetUint32:
        case BuiltinsStubCSigns::ID::DataViewGetInt32:
        case BuiltinsStubCSigns::ID::DataViewGetFloat32:
        case BuiltinsStubCSigns::ID::DataViewSetUint32:
        case BuiltinsStubCSigns::ID::DataViewSetInt32:
        case BuiltinsStubCSigns::ID::DataViewSetFloat32:
            return builder_.Int64(ElmentSize::BITS_32);
        case BuiltinsStubCSigns::ID::DataViewGetFloat64:
        case BuiltinsStubCSigns::ID::DataViewSetFloat64:
            return builder_.Int64(ElmentSize::BITS_64);
        default:
            UNREACHABLE();
    }
}
GateRef TypedNativeInlineLowering::GetValueFromBuffer(GateRef bufferIndex,
                                                      GateRef dataPointer,
                                                      GateRef isLittleEndian,
                                                      GateRef ID)
{
    Label entry(&builder_);
    builder_.SubCfgEntry(&entry);
    Label exit(&builder_);
    Label littleEndian(&builder_);
    Label bigEndian(&builder_);
    Label passResult(&builder_);
    GateRef finalResult = builder_.NullConstant();
    BuiltinsStubCSigns::ID builtinsID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(ID));
    switch (builtinsID) {
        case BuiltinsStubCSigns::ID::DataViewGetUint8: {
            GateRef uint8Res = builder_.Load(VariableType::INT8(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            finalResult = builder_.ZExtInt8ToInt32(uint8Res);
            builder_.Jump(&exit);
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetInt8: {
            GateRef int8res = builder_.Load(VariableType::INT8(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            finalResult = builder_.SExtInt8ToInt32(int8res);
            builder_.Jump(&exit);
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetUint16: {
            DEFVALUE(tempRes, (&builder_), VariableType::INT32(), builder_.Int32(0));
            GateRef uint16Res = builder_.Load(VariableType::INT16(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            BRANCH_CIR(builder_.BoolNot(isLittleEndian), &bigEndian, &littleEndian);
            builder_.Bind(&littleEndian);
            {
                tempRes = builder_.ZExtInt16ToInt32(uint16Res);
                builder_.Jump(&passResult);
            }
            builder_.Bind(&bigEndian);
            {
                GateRef bigEndianInt16 = builder_.Int16ToBigEndianInt16(uint16Res);
                tempRes = builder_.ZExtInt16ToInt32(bigEndianInt16);
                builder_.Jump(&passResult);
            }
            builder_.Bind(&passResult);
            {
                finalResult = *tempRes;
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetInt16: {
            DEFVALUE(tempRes, (&builder_), VariableType::INT32(), builder_.Int32(0));
            GateRef int16Res = builder_.Load(VariableType::INT16(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            BRANCH_CIR(builder_.BoolNot(isLittleEndian), &bigEndian, &littleEndian);
            builder_.Bind(&littleEndian);
            {
                tempRes = builder_.SExtInt16ToInt32(int16Res);
                builder_.Jump(&passResult);
            }
            builder_.Bind(&bigEndian);
            {
                tempRes = builder_.SExtInt16ToInt32(builder_.Int16ToBigEndianInt16(int16Res));
                builder_.Jump(&passResult);
            }
            builder_.Bind(&passResult);
            {
                finalResult = *tempRes;
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetUint32: {
            DEFVALUE(tempRes, (&builder_), VariableType::INT32(), builder_.Int32(0));
            GateRef uint32Res = builder_.Load(VariableType::INT32(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            BRANCH_CIR(builder_.BoolNot(isLittleEndian), &bigEndian, &littleEndian);
            builder_.Bind(&littleEndian);
            {
                tempRes = uint32Res;
                builder_.Jump(&passResult);
            }
            builder_.Bind(&bigEndian);
            {
                tempRes = builder_.Int32ToBigEndianInt32(uint32Res);
                builder_.Jump(&passResult);
            }
            builder_.Bind(&passResult);
            {
                finalResult = *tempRes;
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetInt32: {
            DEFVALUE(tempRes, (&builder_), VariableType::INT32(), builder_.Int32(0));
            GateRef int32Res = builder_.Load(VariableType::INT32(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            BRANCH_CIR(builder_.BoolNot(isLittleEndian), &bigEndian, &littleEndian);
            builder_.Bind(&littleEndian);
            {
                tempRes = int32Res;
                builder_.Jump(&passResult);
            }
            builder_.Bind(&bigEndian);
            {
                tempRes = builder_.Int32ToBigEndianInt32(int32Res);
                builder_.Jump(&passResult);
            }
            builder_.Bind(&passResult);
            {
                finalResult = *tempRes;
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetFloat32: {
            DEFVALUE(tempRes, (&builder_), VariableType::FLOAT64(), builder_.Double(base::NAN_VALUE));
            Label notNaN(&builder_);
            GateRef int32Res = builder_.Load(VariableType::INT32(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            BRANCH_CIR(builder_.BoolNot(isLittleEndian), &bigEndian, &littleEndian);
            builder_.Bind(&littleEndian);
            {
                GateRef float32Res = builder_.CastInt32ToFloat32(int32Res);
                tempRes = builder_.ExtFloat32ToDouble(float32Res);
                builder_.Jump(&passResult);
            }
            builder_.Bind(&bigEndian);
            {
                GateRef originFloat32Res = builder_.CastInt32ToFloat32(int32Res);
                GateRef originDoubleRes = builder_.ExtFloat32ToDouble(originFloat32Res);
                BRANCH_CIR(builder_.DoubleIsNAN(originDoubleRes), &passResult, &notNaN);
                builder_.Bind(&notNaN);
                {
                    GateRef bigEndianInt32Res = builder_.Int32ToBigEndianInt32(int32Res);
                    GateRef float32Res = builder_.CastInt32ToFloat32(bigEndianInt32Res);
                    tempRes = builder_.ExtFloat32ToDouble(float32Res);
                    builder_.Jump(&passResult);
                }
            }
            builder_.Bind(&passResult);
            {
                finalResult = *tempRes;
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewGetFloat64: {
            GateRef float64Res =
                builder_.Load(VariableType::FLOAT64(), dataPointer, builder_.ZExtInt32ToPtr(bufferIndex));
            DEFVALUE(tempRes, (&builder_), VariableType::FLOAT64(), builder_.NanValue());
            Label NaNandNotImpure(&builder_);
            Label ifLittleEndian(&builder_);
            GateRef resIsImpure = builder_.DoubleIsImpureNaN(float64Res);
            GateRef canPassResult = LogicAndBuilder(builder_.GetCurrentEnvironment())
                .And(builder_.BoolNot(resIsImpure)).And(builder_.DoubleIsNAN(float64Res)).Done();
            BRANCH_CIR(canPassResult, &passResult, &ifLittleEndian);
            builder_.Bind(&ifLittleEndian);
            {
                BRANCH_CIR(builder_.BoolNot(isLittleEndian), &bigEndian, &littleEndian);
                builder_.Bind(&bigEndian);
                {
                    GateRef int64Res = builder_.CastDoubleToInt64(float64Res);
                    GateRef bigEndianInt64Res = builder_.Int64ToBigEndianInt64(int64Res);
                    GateRef bigEndianDoubleRes = builder_.CastInt64ToFloat64(bigEndianInt64Res);
                    Label bigEndianResNotImpure(&builder_);
                    BRANCH_CIR(builder_.DoubleIsImpureNaN(bigEndianDoubleRes), &passResult, &bigEndianResNotImpure);
                    builder_.Bind(&bigEndianResNotImpure);
                    {
                        tempRes = bigEndianDoubleRes;
                        builder_.Jump(&passResult);
                    }
                }
                builder_.Bind(&littleEndian);
                {
                    Label resNotImpure(&builder_);
                    BRANCH_CIR(resIsImpure, &passResult, &resNotImpure);
                    builder_.Bind(&resNotImpure);
                    {
                        tempRes = float64Res;
                        builder_.Jump(&passResult);
                    }
                }
            }
            builder_.Bind(&passResult);
            {
                finalResult = *tempRes;
                builder_.Jump(&exit);
            }
            break;
        }
        default:
            UNREACHABLE();
    }
    builder_.Bind(&exit);
    builder_.SubCfgExit();
    return finalResult;
}

GateRef TypedNativeInlineLowering::SetValueInBuffer(
    GateRef bufferIndex, GateRef value, GateRef dataPointer, GateRef isLittleEndian, GateRef ID, GateRef glue)
{
    Label entry(&builder_);
    builder_.SubCfgEntry(&entry);
    Label exit(&builder_);

    Label littleEndian(&builder_);
    Label bigEndian(&builder_);
    Label passResult(&builder_);
    GateRef offset = builder_.ZExtInt32ToPtr(bufferIndex);
    Label startStore(&builder_);
    Label getInt64Value(&builder_);
    DEFVALUE(int64Value, (&builder_), VariableType::INT64(), builder_.Int64(0));
    BRANCH_CIR(builder_.DoubleIsNanOrInf(value), &startStore, &getInt64Value);
    builder_.Bind(&getInt64Value);
    {
        int64Value = builder_.TruncFloatToInt64(value);
        builder_.Jump(&startStore);
    }
    builder_.Bind(&startStore);
    BuiltinsStubCSigns::ID builtinsID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(ID));

    switch (builtinsID) {
        case BuiltinsStubCSigns::ID::DataViewSetUint8:
        case BuiltinsStubCSigns::ID::DataViewSetInt8: {
            GateRef int32Value = builder_.TruncInt64ToInt32(*int64Value);
            builder_.Store(VariableType::INT8(), glue, dataPointer, offset, builder_.TruncInt32ToInt8(int32Value));
            builder_.Jump(&exit);
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewSetUint16:
        case BuiltinsStubCSigns::ID::DataViewSetInt16: {
            BRANCH_CIR(isLittleEndian, &littleEndian, &bigEndian);
            builder_.Bind(&littleEndian);
            {
                builder_.Store(
                    VariableType::INT16(), glue, dataPointer, offset, builder_.TruncInt64ToInt16(*int64Value));
                builder_.Jump(&exit);
            }
            builder_.Bind(&bigEndian);
            {
                GateRef int16Value = builder_.TruncInt64ToInt16(*int64Value);
                GateRef bigEndianInt16 = builder_.Int16ToBigEndianInt16(int16Value);
                builder_.Store(VariableType::INT16(), glue, dataPointer, offset, bigEndianInt16);
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewSetFloat32: {
            Label isNaN(&builder_);
            Label notNaN(&builder_);
            GateRef float32Value = builder_.TruncDoubleToFloat32(value);
            BRANCH_CIR(builder_.DoubleIsNAN(value), &isNaN, &notNaN);
            builder_.Bind(&isNaN);
            {
                builder_.Store(VariableType::FLOAT32(), glue, dataPointer, offset, float32Value);
                builder_.Jump(&exit);
            }
            builder_.Bind(&notNaN);
            {
                BRANCH_CIR(isLittleEndian, &littleEndian, &bigEndian);
                builder_.Bind(&littleEndian);
                {
                    builder_.Store(VariableType::FLOAT32(), glue, dataPointer, offset, float32Value);
                    builder_.Jump(&exit);
                }
                builder_.Bind(&bigEndian);
                {
                    GateRef int32Value = builder_.CastFloat32ToInt32(float32Value);
                    GateRef bigEndianInt32Value = builder_.Int32ToBigEndianInt32(int32Value);
                    GateRef bigEndianFloat32Value = builder_.CastInt32ToFloat32(bigEndianInt32Value);
                    builder_.Store(VariableType::FLOAT32(), glue, dataPointer, offset, bigEndianFloat32Value);
                    builder_.Jump(&exit);
                }
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewSetInt32:
        case BuiltinsStubCSigns::ID::DataViewSetUint32: {
            BRANCH_CIR(isLittleEndian, &littleEndian, &bigEndian);
            builder_.Bind(&littleEndian);
            {
                builder_.Store(
                    VariableType::INT32(), glue, dataPointer, offset, builder_.TruncInt64ToInt32(*int64Value));
                builder_.Jump(&exit);
            }
            builder_.Bind(&bigEndian);
            {
                GateRef int32Value = builder_.TruncInt64ToInt32(*int64Value);
                GateRef bigEndianInt32 = builder_.Int32ToBigEndianInt32(int32Value);
                builder_.Store(VariableType::INT32(), glue, dataPointer, offset, bigEndianInt32);
                builder_.Jump(&exit);
            }
            break;
        }
        case BuiltinsStubCSigns::ID::DataViewSetFloat64: {
            Label isNaN(&builder_);
            Label notNaN(&builder_);
            BRANCH_CIR(builder_.DoubleIsNAN(value), &isNaN, &notNaN);
            {
                builder_.Bind(&isNaN);
                {
                    builder_.Store(VariableType::FLOAT64(), glue, dataPointer, offset, value);
                    builder_.Jump(&exit);
                }
                builder_.Bind(&notNaN);
                {
                    BRANCH_CIR(isLittleEndian, &littleEndian, &bigEndian);
                    builder_.Bind(&littleEndian);
                    {
                        builder_.Store(VariableType::FLOAT64(), glue, dataPointer, offset, value);
                        builder_.Jump(&exit);
                    }
                    builder_.Bind(&bigEndian);
                    {
                        GateRef int64bitsValue = builder_.CastDoubleToInt64(value);
                        GateRef bigEndianInt64Value = builder_.Int64ToBigEndianInt64(int64bitsValue);
                        GateRef float64Value = builder_.CastInt64ToFloat64(bigEndianInt64Value);
                        builder_.Store(VariableType::FLOAT64(), glue, dataPointer, offset, float64Value);
                        builder_.Jump(&exit);
                    }
                }
            }
            break;
        }
        default:
            UNREACHABLE();
    }
    builder_.Bind(&exit);
    builder_.SubCfgExit();
    return builder_.UndefineConstant();
}

static void BuildMathSignDouble(Variable *resVarPtr, CircuitBuilder *builder, GateRef param,
                                std::vector<Label> *labelsForFloatCase)
{
    auto &taggedRes = *resVarPtr;
    auto &labelsForFloatCaseRef = *labelsForFloatCase;
    auto labelsIdx = 0;
    Label *isNan = &labelsForFloatCaseRef.at(labelsIdx++);
    Label *notNan = &labelsForFloatCaseRef.at(labelsIdx++);
    Label *exitNan = &labelsForFloatCaseRef.at(labelsIdx++);

    auto value = builder->GetDoubleOfTDouble(param);
    builder->Branch(builder->DoubleIsNAN(value), isNan, notNan);
    builder->Bind(isNan);
    {
        taggedRes = builder->DoubleToTaggedDoublePtr(builder->NanValue());
        builder->Jump(exitNan);
    }
    builder->Bind(notNan);
    {
        Label *isZero = &labelsForFloatCaseRef.at(labelsIdx++);
        Label *notZero = &labelsForFloatCaseRef.at(labelsIdx++);
        Label *exitZero = &labelsForFloatCaseRef.at(labelsIdx++);

        builder->Branch(builder->DoubleEqual(value, builder->Double(0)), isZero, notZero);
        builder->Bind(isZero);
        {
            taggedRes = param;
            builder->Jump(exitZero);
        }
        builder->Bind(notZero);
        {
            Label *isNegative = &labelsForFloatCaseRef.at(labelsIdx++);
            Label *notNegative = &labelsForFloatCaseRef.at(labelsIdx++);
            Label *exitNegative = &labelsForFloatCaseRef.at(labelsIdx);

            builder->Branch(builder->DoubleLessThan(value, builder->Double(0)), isNegative, notNegative);
            builder->Bind(isNegative);
            {
                taggedRes = builder->Int32ToTaggedPtr(builder->Int32(-1));
                builder->Jump(exitNegative);
            }
            builder->Bind(notNegative);
            {
                taggedRes = builder->Int32ToTaggedPtr(builder->Int32(1));
                builder->Jump(exitNegative);
            }
            builder->Bind(exitNegative);
            builder->Jump(exitZero);
        }
        builder->Bind(exitZero);
        builder->Jump(exitNan);
    }
    builder->Bind(exitNan);
}

static GateRef BuildMathSignInt(CircuitBuilder *builder, GateRef input)
{
    auto nz = builder->BooleanToInt32(builder->Int32NotEqual(input, builder->Int32(0)));
    auto valueShifted = builder->Int32ASR(input, builder->Int32(JSTaggedValue::INT_SIGN_BIT_OFFSET));
    return builder->Int32Or(valueShifted, nz);
}

void TypedNativeInlineLowering::LowerMathSignInt(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef param = acc_.GetValueIn(gate, 0);

    auto res = BuildMathSignInt(&builder_, param);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), res);
}

void TypedNativeInlineLowering::LowerMathSignTagged(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label exit(&builder_);
    GateRef param = acc_.GetValueIn(gate, 0);
    DEFVALUE(taggedRes, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());

    std::vector<Label> labelsForFloatCase;
    constexpr auto FLOAT_CASE_LABELS_COUNT = 9;
    labelsForFloatCase.reserve(FLOAT_CASE_LABELS_COUNT);
    for (auto i = 0; i < FLOAT_CASE_LABELS_COUNT; i++) {
        labelsForFloatCase.emplace_back(&builder_);
    }

    Label isInt(&builder_);
    Label notInt(&builder_);
    builder_.Branch(builder_.TaggedIsInt(param), &isInt, &notInt);
    builder_.Bind(&isInt);
    {
        auto value = builder_.GetInt32OfTInt(param);

        auto res = BuildMathSignInt(&builder_, value);

        taggedRes = builder_.Int32ToTaggedPtr(res);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notInt);
    {
        BuildMathSignDouble(&taggedRes, &builder_, param, &labelsForFloatCase);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *taggedRes);
}

GateRef TypedNativeInlineLowering::BuildDoubleIsFinite(GateRef value)
{
    GateRef diff = builder_.DoubleSub(value, value);
    return builder_.DoubleEqual(diff, diff);
}

void TypedNativeInlineLowering::LowerNumberIsFinite(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef result = BuildDoubleIsFinite(value);

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

GateRef TypedNativeInlineLowering::BuildTaggedIsInteger(GateRef gate, GateRef value, bool safe)
{
    Label exit(&builder_);
    Label entry(&builder_);
    builder_.SubCfgEntry(&entry);
    DEFVALUE(result, (&builder_), VariableType::BOOL(), builder_.False());
    Label isInt(&builder_);
    Label notIsInt(&builder_);
    BRANCH_CIR(builder_.TaggedIsInt(value), &isInt, &notIsInt);
    builder_.Bind(&isInt);
    {
        result = builder_.True();
        builder_.Jump(&exit);
    }
    builder_.Bind(&notIsInt);

    Label isFinite(&builder_);
    GateRef doubleVal = builder_.GetDoubleOfTDouble(value);
    BRANCH_CIR(BuildDoubleIsFinite(doubleVal), &isFinite, &exit);
    builder_.Bind(&isFinite);
    {
        GateRef doubleTrunc = builder_.DoubleTrunc(gate, doubleVal);
        result = builder_.Equal(doubleVal, doubleTrunc);
        if (safe) {
            GateRef resultVal = *result;
            result = LogicAndBuilder(builder_.GetCurrentEnvironment())
                .And(resultVal)
                .And(builder_.DoubleLessThanOrEqual(builder_.FAbs(doubleTrunc),
                                                    builder_.Double(base::MAX_SAFE_INTEGER)))
                .Done();
        }
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    GateRef res = *result;
    builder_.SubCfgExit();
    return res;
}

void TypedNativeInlineLowering::LowerNumberIsInteger(GateRef gate, OpCode op)
{
    ASSERT(op == OpCode::NUMBER_IS_INTEGER || op == OpCode::NUMBER_IS_SAFEINTEGER);
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef result = BuildTaggedIsInteger(gate, value, op == OpCode::NUMBER_IS_SAFEINTEGER);

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerNumberIsNaN(GateRef gate)
{
    GateRef value = acc_.GetValueIn(gate, 0);
    Environment env(gate, circuit_, &builder_);
    GateRef result = builder_.DoubleIsNAN(value);

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerNumberParseInt(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    Label exit(&builder_);
    Label slowPath(&builder_);
    Label msgIsString(&builder_);
    Label radixIsSpecial(&builder_);
    Label radixIsSpecialInt(&builder_);

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    DEFVALUE(radix, (&builder_), VariableType::INT32(), builder_.Int32(0));
    GateRef msg = acc_.GetValueIn(gate, 0);
    GateRef arg2 = acc_.GetValueIn(gate, 1);

    builder_.Branch(builder_.TaggedIsString(msg), &msgIsString, &slowPath);

    builder_.Bind(&msgIsString);
    {
        builder_.Branch(builder_.TaggedIsUndefined(arg2), &radixIsSpecialInt, &radixIsSpecial);
        builder_.Bind(&radixIsSpecial);
        {
            Label radixIsInt(&builder_);
            builder_.Branch(builder_.TaggedIsInt(arg2), &radixIsInt, &slowPath);
            builder_.Bind(&radixIsInt);
            {
                radix = builder_.GetInt32OfTInt(arg2);
                builder_.Jump(&radixIsSpecialInt);
            }
        }
        builder_.Bind(&radixIsSpecialInt);
        {
            GateRef glue = acc_.GetGlueFromArgList();
            result = builder_.CallNGCRuntime(glue, RTSTUB_ID(StringToNumber), Gate::InvalidGateRef,
                { msg, *radix }, gate);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&slowPath);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        // this may return exception
        result = builder_.CallRuntime(glue, RTSTUB_ID(ParseInt), Gate::InvalidGateRef, {msg, arg2}, gate);
        builder_.Jump(&exit);
    }

    builder_.Bind(&exit);
    ReplaceGateWithPendingException(
        gate, acc_.GetGlueFromArgList(), builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerNumberParseFloat(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    DEFVALUE(result, (&builder_), VariableType::FLOAT64(), builder_.HoleConstant());
    Label slowPath(&builder_);
    Label exit(&builder_);

    Label definedMsg(&builder_);
    Label undefinedMsg(&builder_);
    GateRef msg = acc_.GetValueIn(gate, 0);
    builder_.Branch(builder_.BoolNot(builder_.TaggedIsUndefined(msg)), &definedMsg, &undefinedMsg);
    builder_.Bind(&definedMsg);
    {
        auto frameState = acc_.GetFrameState(gate);
        builder_.DeoptCheck(builder_.TaggedIsString(msg), frameState, DeoptType::NOTSTRING1);
        Label isIntegerStr(&builder_);
        Label notIntegerStr(&builder_);
        Label exitIntegerStr(&builder_);
        builder_.Branch(builder_.IsIntegerString(msg), &isIntegerStr, &notIntegerStr);
        builder_.Bind(&isIntegerStr);
        {
            result = builder_.ChangeInt32ToFloat64(builder_.GetRawHashFromString(msg));
            builder_.Jump(&exitIntegerStr);
        }
        builder_.Bind(&notIntegerStr);
        {
            GateRef glue = acc_.GetGlueFromArgList();
            auto taggedDouble = builder_.CallNGCRuntime(glue, RTSTUB_ID(NumberHelperStringToDouble),
                                                        Gate::InvalidGateRef, { msg }, gate);
            result = builder_.GetDoubleOfTDouble(taggedDouble);
            builder_.Jump(&exitIntegerStr);
        }
        builder_.Bind(&exitIntegerStr);
        builder_.Jump(&exit);
    }
    builder_.Bind(&undefinedMsg);
    {
        result = builder_.Double(base::NAN_VALUE);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerToCommonStub(GateRef gate, CommonStubCSigns::ID id)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    std::vector<GateRef> args {glue};
    size_t numIn = acc_.GetNumValueIn(gate);
    for (size_t idx = 0; idx < numIn; idx++) {
        args.emplace_back(acc_.GetValueIn(gate, idx));
    }
    GateRef ret = builder_.CallStub(glue, gate, id, args);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), ret);
}

void TypedNativeInlineLowering::LowerDateGetTime(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef dateValueOffset = builder_.IntPtr(JSDate::TIME_VALUE_OFFSET);
    GateRef result = builder_.Load(VariableType::JS_ANY(), obj, dateValueOffset);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedNativeInlineLowering::LowerGeneralWithoutArgs(GateRef gate, RuntimeStubCSigns::ID stubId)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef result = builder_.CallNGCRuntime(glue, stubId, Gate::InvalidGateRef, {glue}, gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedNativeInlineLowering::LowerBigIntConstructor(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label hasException(&builder_);
    Label exit(&builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef glue = acc_.GetGlueFromArgList();
    auto result = builder_.CallRuntime(glue, RTSTUB_ID(BigIntConstructor), Gate::InvalidGateRef, {value}, gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

template <bool IS_SIGNED>
void TypedNativeInlineLowering::LowerBigIntConstructorInt32(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    size_t length = 1;
    size_t size = AlignUp(BigInt::ComputeSize(length), static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    GateRef sizeGate = builder_.IntPtr(size);
    GateRef hclass = builder_.GetGlobalConstantValue(ConstantIndex::BIGINT_CLASS_INDEX);
    GateRef sign = builder_.Int32(0);
    if constexpr (IS_SIGNED) {
        sign = builder_.Int32LSR(value, builder_.Int32(JSTaggedValue::INT_SIGN_BIT_OFFSET));
        auto temp = builder_.Int32ASR(value, builder_.Int32(JSTaggedValue::INT_SIGN_BIT_OFFSET));
        value = builder_.Int32Sub(builder_.Int32Xor(value, temp), temp);
    }

    // looks like code from StartAllocate to FinishAllocate must be linear
    builder_.StartAllocate();
    GateRef object = builder_.HeapAlloc(acc_.GetGlueFromArgList(), sizeGate, GateType::TaggedValue(),
                                        RegionSpaceFlag::IN_SHARED_NON_MOVABLE);
    // initialization
    builder_.StoreConstOffset(VariableType::JS_POINTER(), object, JSObject::HCLASS_OFFSET, hclass,
                              MemoryAttribute::NeedBarrierAndAtomic());
    builder_.StoreConstOffset(VariableType::INT32(), object, BigInt::LENGTH_OFFSET, builder_.Int32(length));
    builder_.StoreConstOffset(VariableType::INT32(), object, BigInt::BIT_FIELD_OFFSET, sign);
    builder_.StoreConstOffset(VariableType::INT32(), object, BigInt::DATA_OFFSET, value);
    GateRef ret = builder_.FinishAllocate(object);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

GateRef TypedNativeInlineLowering::BuildTaggedPointerOverflowInt32(GateRef value)
{
    GateRef intValue = builder_.ChangeFloat64ToInt32(
        builder_.CastInt64ToFloat64(builder_.ChangeTaggedPointerToInt64(value)));
    return builder_.BitOr(builder_.Int32LessThanOrEqual(intValue, builder_.Int32(INT32_MIN)),
                          builder_.Int32GreaterThanOrEqual(intValue, builder_.Int32(INT32_MAX)));
}

void TypedNativeInlineLowering::LowerStringCharCodeAt(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef thisValue = acc_.GetValueIn(gate, 0); // 0: first argument
    GateRef pos = acc_.GetValueIn(gate, 1); // 1: second argument
    GateRef glue = acc_.GetGlueFromArgList();
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label posIsValid(&builder_);
    Label posNotValid(&builder_);
    Label exit(&builder_);

    GateRef strLen = builder_.GetLengthFromString(thisValue);
    BRANCH_CIR(builder_.Int32UnsignedLessThan(pos, strLen), &posIsValid, &posNotValid);
    builder_.Bind(&posIsValid);
    {
        BuiltinsStringStubBuilder stringBuilder(builder_.GetCurrentEnvironment());
        result = stringBuilder.FastStringCharCodeAt(glue, thisValue, pos);
        builder_.Jump(&exit);
    }
    builder_.Bind(&posNotValid);
    {
        result = builder_.DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerStringSubstring(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef thisValue = acc_.GetValueIn(gate, 0); // 0: the first parameter
    GateRef startTag = acc_.GetValueIn(gate, 1); // 1: the second parameter
    auto argc = acc_.GetNumValueIn(gate);
    GateRef endTag;
    if (argc < 3) { // 3: the 2nd parameter of substring is optional
        endTag = builder_.GetLengthFromString(thisValue);
    } else {
        endTag = acc_.GetValueIn(gate, 2); // 2: the third parameter
    }
    GateRef glue = acc_.GetGlueFromArgList();
    DEFVALUE(result, (&builder_), VariableType::JS_POINTER(), builder_.Int32ToTaggedPtr(builder_.Int32(-1)));
    DEFVALUE(start, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(end, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(from, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(to, (&builder_), VariableType::INT32(), builder_.Int32(0));
    Label next(&builder_);
    Label countStart(&builder_);
    Label startTagIsInt(&builder_);
    Label startTagNotInt(&builder_);
    Label endTagIsInt(&builder_);
    Label endTagNotInt(&builder_);
    GateRef length = builder_.GetLengthFromString(thisValue);

    if (acc_.GetMachineType(startTag) == MachineType::I64) {
        BRANCH_CIR(builder_.TaggedIsInt(startTag), &startTagIsInt, &startTagNotInt);
        builder_.Bind(&startTagIsInt);
        start = builder_.GetInt32OfTInt(startTag);
        builder_.Jump(&next);
        builder_.Bind(&startTagNotInt);
        {
            start = builder_.SaturateTruncDoubleToInt32(glue, builder_.GetDoubleOfTDouble(startTag));
            builder_.Jump(&next);
        }
    } else {
        start = NumberToInt32(startTag);
        builder_.Jump(&next);
    }
    builder_.Bind(&next);
    {
        if (acc_.GetMachineType(endTag) == MachineType::I64) {
            BRANCH_CIR(builder_.TaggedIsInt(endTag), &endTagIsInt, &endTagNotInt);
            builder_.Bind(&endTagIsInt);
            end = builder_.GetInt32OfTInt(endTag);
            builder_.Jump(&countStart);
            builder_.Bind(&endTagNotInt);
            {
                end = builder_.SaturateTruncDoubleToInt32(glue, builder_.GetDoubleOfTDouble(endTag));
                builder_.Jump(&countStart);
            }
        } else {
            end = NumberToInt32(endTag);
            builder_.Jump(&countStart);
        }
    }
    builder_.Bind(&countStart);
    GateRef zero = builder_.Int32(0);
    GateRef finalStart = BuildIntMinMax<false>(BuildIntMinMax<true>(*start, zero), length);
    GateRef finalEnd = BuildIntMinMax<false>(BuildIntMinMax<true>(*end, zero), length);
    from = BuildIntMinMax<false>(finalStart, finalEnd);
    to = BuildIntMinMax<true>(finalStart, finalEnd);
    const CallSignature *cs = BuiltinsStubCSigns::Get(BuiltinsStubCSigns::ID::StringSubstring);
    BuiltinsStringStubBuilder stringBuilder(const_cast<CallSignature *>(cs), &env);
    GateRef len = builder_.Int32Sub(*to, *from);
    result = stringBuilder.GetSubString(glue, thisValue, *from, len);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerStringSubStr(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef thisValue = acc_.GetValueIn(gate, 0); // 0: the first parameter
    GateRef intStart = acc_.GetValueIn(gate, 1); // 1: the second parameter
    GateRef lengthTag = acc_.GetValueIn(gate, 2); // 2: the third parameter
    GateRef glue = acc_.GetGlueFromArgList();
    DEFVALUE(result, (&builder_), VariableType::JS_POINTER(), builder_.Undefined());
    DEFVALUE(start, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(end, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(resultLength, (&builder_), VariableType::INT32(), builder_.Int32(0));
    Label exit(&builder_);
    Label next(&builder_);
    Label intStartIsInt(&builder_);
    Label intStartNotInt(&builder_);
    Label lengthTagIsInt(&builder_);
    Label lengthTagNotInt(&builder_);
    Label startLessZero(&builder_);
    Label countStart(&builder_);
    Label countResultLength(&builder_);
    Label countResult(&builder_);
    GateRef length = builder_.GetLengthFromString(thisValue);

    if (acc_.GetMachineType(intStart) == MachineType::I64) {
        BRANCH_CIR(builder_.TaggedIsInt(intStart), &intStartIsInt, &intStartNotInt);
        builder_.Bind(&intStartIsInt);
        start = builder_.GetInt32OfTInt(intStart);
        builder_.Jump(&next);
        builder_.Bind(&intStartNotInt);
        {
            start = builder_.SaturateTruncDoubleToInt32(glue, builder_.GetDoubleOfTDouble(intStart));
            builder_.Jump(&next);
        }
    } else {
        start = NumberToInt32(intStart);
        builder_.Jump(&next);
    }
    builder_.Bind(&next);
    {
        if (acc_.GetMachineType(lengthTag) == MachineType::I64) {
            BRANCH_CIR(builder_.TaggedIsInt(lengthTag), &lengthTagIsInt, &lengthTagNotInt);
            builder_.Bind(&lengthTagIsInt);
            end = builder_.GetInt32OfTInt(lengthTag);
            builder_.Jump(&countStart);
            builder_.Bind(&lengthTagNotInt);
            {
                end = builder_.SaturateTruncDoubleToInt32(glue, builder_.GetDoubleOfTDouble(lengthTag));
                builder_.Jump(&countStart);
            }
        } else {
            end = NumberToInt32(lengthTag);
            builder_.Jump(&countStart);
        }
    }
    builder_.Bind(&countStart);
    GateRef zero = builder_.Int32(0);
    BRANCH_CIR(builder_.Int32LessThan(*start, zero), &startLessZero, &countResultLength);
    builder_.Bind(&startLessZero);
    {
        GateRef tempStart = *start;
        start = BuildIntMinMax<true>(builder_.Int32Add(length, tempStart), zero);
        builder_.Jump(&countResultLength);
    }
    builder_.Bind(&countResultLength);
    {
        resultLength = BuildIntMinMax<false>(BuildIntMinMax<true>(*end, zero), builder_.Int32Sub(length, *start));
        builder_.Jump(&countResult);
    }
    builder_.Bind(&countResult);
    {
        Label resLenLessOrEqualZero(&builder_);
        Label resLenNotLessOrEqualZero(&builder_);
        builder_.Branch(builder_.Int32LessThanOrEqual(*resultLength, zero),
            &resLenLessOrEqualZero, &resLenNotLessOrEqualZero);
        builder_.Bind(&resLenLessOrEqualZero);
        {
            result = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
            builder_.Jump(&exit);
        }
        builder_.Bind(&resLenNotLessOrEqualZero);
        {
            const CallSignature *cs = BuiltinsStubCSigns::Get(BuiltinsStubCSigns::ID::StringSubStr);
            BuiltinsStringStubBuilder stringBuilder(const_cast<CallSignature *>(cs), &env);
            result = stringBuilder.GetFastSubString(glue, thisValue, *start, *resultLength);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerStringSlice(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef thisValue = acc_.GetValueIn(gate, 0); // 0: the first parameter
    GateRef startTag = acc_.GetValueIn(gate, 1); // 1: the second parameter
    auto argc = acc_.GetNumValueIn(gate);
    GateRef endTag;
    if (argc < 3) { // 3: the 2nd parameter of slice is optional
        endTag = builder_.GetLengthFromString(thisValue);
    } else {
        endTag = acc_.GetValueIn(gate, 2); // 2: the third parameter
    }
    GateRef glue = acc_.GetGlueFromArgList();
    DEFVALUE(result, (&builder_), VariableType::JS_POINTER(), builder_.Undefined());
    DEFVALUE(start, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(end, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(from, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(to, (&builder_), VariableType::INT32(), builder_.Int32(0));
    Label exit(&builder_);
    Label next(&builder_);
    Label startTagIsInt(&builder_);
    Label startTagNotInt(&builder_);
    Label endTagIsInt(&builder_);
    Label endTagNotInt(&builder_);
    Label countStart(&builder_);
    Label countRes(&builder_);
    Label getTo(&builder_);
    Label startLessThanZero(&builder_);
    Label startNotLessThanZero(&builder_);
    GateRef length = builder_.GetLengthFromString(thisValue);

    if (acc_.GetMachineType(startTag) == MachineType::I64) {
        BRANCH_CIR(builder_.TaggedIsInt(startTag), &startTagIsInt, &startTagNotInt);
        builder_.Bind(&startTagIsInt);
        start = builder_.GetInt32OfTInt(startTag);
        builder_.Jump(&next);
        builder_.Bind(&startTagNotInt);
        {
            start = builder_.SaturateTruncDoubleToInt32(glue, builder_.GetDoubleOfTDouble(startTag));
            builder_.Jump(&next);
        }
    } else {
        start = NumberToInt32(startTag);
        builder_.Jump(&next);
    }
    builder_.Bind(&next);
    {
        if (acc_.GetMachineType(endTag) == MachineType::I64) {
            BRANCH_CIR(builder_.TaggedIsInt(endTag), &endTagIsInt, &endTagNotInt);
            builder_.Bind(&endTagIsInt);
            end = builder_.GetInt32OfTInt(endTag);
            builder_.Jump(&countStart);
            builder_.Bind(&endTagNotInt);
            end = builder_.SaturateTruncDoubleToInt32(glue, builder_.GetDoubleOfTDouble(endTag));
            builder_.Jump(&countStart);
        } else {
            end = NumberToInt32(endTag);
            builder_.Jump(&countStart);
        }
    }
    builder_.Bind(&countStart);
    GateRef zero = builder_.Int32(0);
    BRANCH_CIR(builder_.Int32LessThan(*start, zero), &startLessThanZero, &startNotLessThanZero);
    builder_.Bind(&startLessThanZero);
    {
        from = BuildIntMinMax<true>(builder_.Int32Add(length, *start), zero);
        builder_.Jump(&getTo);
    }
    builder_.Bind(&startNotLessThanZero);
    {
        from = BuildIntMinMax<false>(length, *start);
        builder_.Jump(&getTo);
    }
    builder_.Bind(&getTo);
    {
        Label endLessThanZero(&builder_);
        Label endNotLessThanZero(&builder_);
        BRANCH_CIR(builder_.Int32LessThan(*end, zero), &endLessThanZero, &endNotLessThanZero);
        builder_.Bind(&endLessThanZero);
        {
            to = BuildIntMinMax<true>(builder_.Int32Add(length, *end), zero);
            builder_.Jump(&countRes);
        }
        builder_.Bind(&endNotLessThanZero);
        {
            to = BuildIntMinMax<false>(length, *end);
            builder_.Jump(&countRes);
        }
        builder_.Bind(&countRes);
        {
            Label fromLessThanTo(&builder_);
            Label fromNotLessThanTo(&builder_);
            BRANCH_CIR(builder_.Int32LessThan(*from, *to), &fromLessThanTo, &fromNotLessThanTo);
            builder_.Bind(&fromLessThanTo);
            {
                const CallSignature *cs = BuiltinsStubCSigns::Get(BuiltinsStubCSigns::ID::StringSlice);
                BuiltinsStringStubBuilder stringBuilder(const_cast<CallSignature *>(cs), &env);
                GateRef len = builder_.Int32Sub(*to, *from);
                result = stringBuilder.GetSubString(glue, thisValue, *from, len);
                builder_.Jump(&exit);
            }
            builder_.Bind(&fromNotLessThanTo);
            {
                result = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

GateRef TypedNativeInlineLowering::NumberToInt32(GateRef gate)
{
    auto machineType = acc_.GetMachineType(gate);
    switch (machineType) {
        case MachineType::I32:
            return gate;
        case MachineType::F64:
            if (isLiteCG_) {
                return builder_.ChangeFloat64ToInt32(gate);
            } else {
                GateRef glue = acc_.GetGlueFromArgList();
                return builder_.SaturateTruncDoubleToInt32(glue, gate);
            }
        case MachineType::I1:
            return builder_.ZExtInt1ToInt32(gate);
        default:
            LOG_COMPILER(ERROR) << "Not Number Type";
            break;
    }
    return Circuit::NullGate();
}

void TypedNativeInlineLowering::LowerObjectIs(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateRef result = Circuit::NullGate();

    if (left == right) {
        result = builder_.TaggedTrue();
    } else if (TypeInfoAccessor::IsTrustedNotSameType(compilationEnv_, circuit_, chunk_, acc_, left, right)) {
        result = builder_.TaggedFalse();
    } else {
        GateRef glue = acc_.GetGlueFromArgList();
        GateRef boolRet = builder_.CallStub(glue, gate, CommonStubCSigns::SameValue, { glue, left, right});
        result = builder_.BooleanToTaggedBooleanPtr(boolRet);
    }

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerObjectGetPrototypeOf(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = Circuit::NullGate();

    // fast handle some primitive types
    if (TypeInfoAccessor::IsTrustedBooleanOrNumberOrStringType(compilationEnv_, circuit_, chunk_, acc_, value)) {
        GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env.Is32Bit()));
        GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        size_t index = -1;
        if (TypeInfoAccessor::IsTrustedBooleanType(acc_, value)) {
            index = GlobalEnv::BOOLEAN_PROTOTYPE_INDEX;
        } else if (TypeInfoAccessor::IsTrustedNumberType(acc_, value)) {
            index = GlobalEnv::NUMBER_PROTOTYPE_INDEX;
        } else {
            ASSERT(TypeInfoAccessor::IsTrustedStringType(compilationEnv_, circuit_, chunk_, acc_, value));
            index = GlobalEnv::STRING_PROTOTYPE_INDEX;
        }
        result = builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, index);
    } else {
        GateRef object = builder_.ToObject(glue, value);
        result = builder_.GetPrototype(glue, object);
    }

    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerObjectCreate(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef proto = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());

    Label exception(&builder_);
    Label create(&builder_);
    Label exit(&builder_);
    GateRef protoCheck = LogicAndBuilder(&env)
        .And(builder_.BoolNot(builder_.IsEcmaObject(proto)))
        .And(builder_.BoolNot(builder_.TaggedIsNull(proto)))
        .Done();
    protoCheck = LogicOrBuilder(&env).Or(protoCheck).Or(builder_.TaggedIsSharedObj(proto)).Done();
    BRANCH_CIR(protoCheck,  &exception, &create);
    builder_.Bind(&exception);
    {
        GateRef taggedId = builder_.Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotValidObject));
        builder_.CallRuntime(glue, RTSTUB_ID(ThrowTypeError), Gate::InvalidGateRef,
                             { builder_.Int32ToTaggedInt(taggedId) }, gate);
        result = builder_.ExceptionConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&create);
    {
        result = builder_.OrdinaryNewJSObjectCreate(glue, proto);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerObjectIsPrototypeOf(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedFalse());

    // 1. If Type(V) is not Object, return false.
    Label ecmaObject(&builder_);
    Label returnFalse(&builder_);
    Label returnException(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.IsEcmaObject(value), &ecmaObject, &returnFalse);
    builder_.Bind(&ecmaObject);
    {
        // 2. Let O be ? ToObject(this value).
        GateRef obj = builder_.ToObject(glue, thisValue);
        Label noPendingException(&builder_);
        BRANCH_CIR(builder_.HasPendingException(glue), &returnException, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            // 3. Repeat,
            //    a. Set V to ? V.[[GetPrototypeOf]]().
            //    b. If V is null, return false.
            //    c. If SameValue(O, V) is true, return true.
            DEFVALUE(proto, (&builder_), VariableType::JS_ANY(), value);
            Label loopHead(&builder_);
            Label loopEnd(&builder_);
            Label loopExit(&builder_);
            Label compare(&builder_);
            builder_.Jump(&loopHead);
            builder_.LoopBegin(&loopHead);
            {
                BRANCH_CIR(builder_.TaggedIsNotNull(*proto), &compare, &loopExit);
                builder_.Bind(&compare);
                {
                    proto = builder_.GetPrototype(glue, *proto);
                    Label noPendingException1(&builder_);
                    BRANCH_CIR(builder_.HasPendingException(glue), &returnException, &noPendingException1);
                    builder_.Bind(&noPendingException1);
                    {
                        Label sameValue(&builder_);
                        GateRef same = builder_.CallStub(glue, gate, CommonStubCSigns::SameValue,
                                                         { glue, obj, *proto });
                        BRANCH_CIR(same, &sameValue, &loopEnd);
                        builder_.Bind(&sameValue);
                        {
                            result = builder_.TaggedTrue();
                            builder_.Jump(&exit);
                        }
                    }
                }
            }
            builder_.Bind(&loopEnd);
            builder_.LoopEnd(&loopHead);
            builder_.Bind(&loopExit);
            {
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&returnFalse);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&returnException);
    {
        result = builder_.ExceptionConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerObjectHasOwnProperty(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef key = acc_.GetValueIn(gate, 1);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedFalse());
    Label callRuntime(&builder_);
    Label exit(&builder_);
    BuiltinsObjectStubBuilder objectStubBuilder(&env, glue);
    objectStubBuilder.HasOwnProperty(&result, &exit, &callRuntime, thisValue, key, gate);
    builder_.Bind(&callRuntime);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(ObjectPrototypeHasOwnProperty), Gate::InvalidGateRef,
                                      { thisValue, key }, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerReflectGetPrototypeOf(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.GetPrototype(glue, value);  // Do not ToObject
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerReflectGet(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef target = acc_.GetValueIn(gate, 0);
    GateRef key = acc_.GetValueIn(gate, 1);  // key is trusted string
    Label isEcmaObject(&builder_);
    Label notEcmaObject(&builder_);
    Label exit(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    // 1. If Type(target) is not Object, throw a TypeError exception.
    BRANCH_CIR(builder_.IsEcmaObject(target), &isEcmaObject, &notEcmaObject);
    builder_.Bind(&isEcmaObject);
    {
        result = builder_.CallStub(glue, gate, CommonStubCSigns::DeprecatedGetPropertyByName,  // no ic
                                   { glue, target, key });
        builder_.Jump(&exit);
    }
    builder_.Bind(&notEcmaObject);
    {
        GateRef taggedId = builder_.Int32(GET_MESSAGE_STRING_ID(TargetTypeNotObject));
        builder_.CallRuntime(glue, RTSTUB_ID(ThrowTypeError), Gate::InvalidGateRef,
                             { builder_.Int32ToTaggedInt(taggedId) }, gate);
        result = builder_.ExceptionConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerReflectHas(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef target = acc_.GetValueIn(gate, 0);
    GateRef key = acc_.GetValueIn(gate, 1);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedFalse());
    Label afterHasOwn(&builder_);
    Label callRuntime(&builder_);
    Label exit(&builder_);
    BuiltinsObjectStubBuilder objectStubBuilder(&env, glue);
    objectStubBuilder.HasOwnProperty(&result, &afterHasOwn, &callRuntime, target, key, gate);
    builder_.Bind(&afterHasOwn);
    {
        BRANCH_CIR(builder_.TaggedIsTrue(*result), &exit, &callRuntime);
    }
    builder_.Bind(&callRuntime);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(ReflectHas), Gate::InvalidGateRef, { target, key }, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerReflectConstruct(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef target = acc_.GetValueIn(gate, 0);
    GateRef thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::NewThisObjectChecked, { glue, target });
    Label callCtor(&builder_);
    Label callRuntime(&builder_);
    Label exit(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    BRANCH_CIR(builder_.TaggedIsHole(thisObj), &callRuntime, &callCtor);
    builder_.Bind(&callCtor);
    {
        std::vector<GateRef> args { glue, builder_.Int64(NUM_MANDATORY_JSFUNC_ARGS), builder_.IntPtr(0),
                                    target, target, thisObj };
        result = builder_.Construct(gate, args);
        builder_.Jump(&exit);
    }
    builder_.Bind(&callRuntime);
    {
        result = builder_.CallRuntime(glue, RTSTUB_ID(ReflectConstruct), Gate::InvalidGateRef, { target }, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}

void TypedNativeInlineLowering::LowerReflectApply(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef target = acc_.GetValueIn(gate, 0);
    GateRef thisValue = acc_.GetValueIn(gate, 1);
    GateRef argumentsList = acc_.GetValueIn(gate, 2);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(ReflectApply), Gate::InvalidGateRef,
                                          { target, thisValue, argumentsList }, gate);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerFunctionPrototypeApply(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef thisFunc = acc_.GetValueIn(gate, 0);
    GateRef thisArg = acc_.GetValueIn(gate, 1);
    GateRef argArray = acc_.GetValueIn(gate, 2);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(FunctionPrototypeApply), Gate::InvalidGateRef,
                                          { thisFunc, thisArg, argArray }, gate);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerFunctionPrototypeBind(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef target = acc_.GetValueIn(gate, 0);
    GateRef thisArg = acc_.GetValueIn(gate, 1);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(FunctionPrototypeBind), Gate::InvalidGateRef,
                                          { target, thisArg }, gate);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerFunctionPrototypeCall(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    std::vector<GateRef> args = acc_.GetValueIns(gate);
    GateRef result = builder_.CallRuntime(glue, RTSTUB_ID(FunctionPrototypeCall), Gate::InvalidGateRef, args, gate);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), result);
}

void TypedNativeInlineLowering::LowerArrayIncludesIndexOf(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label exit(&builder_);
    BuiltinsArrayStubBuilder stubBuilder(&env);

    using Indices = CircuitArgIndices::ArrayIncludesIndexOf;
    GateRef elements = acc_.GetValueIn(gate, Indices::ELEMENTS);
    GateRef target = acc_.GetValueIn(gate, Indices::TARGET);
    GateRef fromIndex = acc_.GetValueIn(gate, Indices::FROM_INDEX);
    GateRef len = acc_.GetValueIn(gate, Indices::LENGTH);
    GateRef callIDRef = acc_.GetValueIn(gate, Indices::CALL_ID);
    GateRef kindRef = acc_.GetValueIn(gate, Indices::ARRAY_KIND);

    ElementsKind kind = static_cast<ElementsKind>(acc_.GetConstantValue(kindRef));
    BuiltinsStubCSigns::ID callID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(callIDRef));
    auto options = stubBuilder.MakeIndexOfOptions(callID, false);
    Variable res = stubBuilder.MakeResultVariableDefaultNotFound(options);

    Label arrayIsNotEmpty(&builder_);
    Label searchRangeIsNotEmpty(&builder_);
    BRANCH_CIR_UNLIKELY(builder_.Int32Equal(builder_.Int32(0), len), &exit, &arrayIsNotEmpty);
    builder_.Bind(&arrayIsNotEmpty);
    if (options.reversedOrder) {
        BRANCH_CIR_UNLIKELY(builder_.Int64LessThan(fromIndex, builder_.Int64(0)), &exit, &searchRangeIsNotEmpty);
    } else {
        GateRef exitCond = builder_.Int64GreaterThanOrEqual(fromIndex, builder_.ZExtInt32ToInt64(len));
        BRANCH_CIR_UNLIKELY(exitCond, &exit, &searchRangeIsNotEmpty)
    }

    builder_.Bind(&searchRangeIsNotEmpty);
    // todo: Branches for mutant tagged array (with raw int32 or raw double values)?
    Label undefinedBranch(&builder_);
    Label targetIsNotUndefined(&builder_);
    BRANCH_CIR_UNLIKELY(builder_.TaggedIsUndefined(target), &undefinedBranch, &targetIsNotUndefined);
    builder_.Bind(&undefinedBranch);
    res = stubBuilder.IndexOfTaggedUndefined(elements, fromIndex, len, options);
    builder_.Jump(&exit);

    builder_.Bind(&targetIsNotUndefined);
    GateRef glue = acc_.GetGlueFromArgList();
    switch (kind) {
        case ElementsKind::INT:
        case ElementsKind::HOLE_INT:
            res = stubBuilder.IndexOfTaggedIntElements(elements, target, fromIndex, len, options);
            break;
        case ElementsKind::NUMBER:
        case ElementsKind::HOLE_NUMBER:
            res = stubBuilder.IndexOfTaggedNumber(elements, target, fromIndex, len, options, false);
            break;
        case ElementsKind::STRING:
            using StringElementsCondition = BuiltinsArrayStubBuilder::StringElementsCondition;
            res = stubBuilder.IndexOfStringElements(glue, elements, target, fromIndex, len, options,
                StringElementsCondition::MUST_BE_STRING);
            break;
        case ElementsKind::HOLE_STRING:
            res = stubBuilder.IndexOfStringElements(glue, elements, target, fromIndex, len, options,
                StringElementsCondition::MAY_BE_HOLE);
            break;
        case ElementsKind::OBJECT:
        case ElementsKind::HOLE_OBJECT:
            res = stubBuilder.IndexOfBigIntOrObjectElements(glue, elements, target, fromIndex, len, options);
            break;
        default:
            res = stubBuilder.IndexOfGeneric(glue, elements, target, fromIndex, len, options);
            break;
    }
    builder_.Jump(&exit);

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *res);
}

void TypedNativeInlineLowering::LowerArrayIteratorBuiltin(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef thisArray = acc_.GetValueIn(gate, 0);
    GateRef callIDRef = acc_.GetValueIn(gate, 1);
    BuiltinsStubCSigns::ID callID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(callIDRef));
    IterationKind iterationKind = GetArrayIterKindFromBuilin(callID);

    GateRef glue = acc_.GetGlueFromArgList();
    GateRef glueGlobalEnvOffset = builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env.Is32Bit()));
    GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef prototype = builder_.GetGlobalEnvValue(
        VariableType::JS_POINTER(), glueGlobalEnv, GlobalEnv::ARRAY_ITERATOR_PROTOTYPE_INDEX);

    GateRef iteratorHClass = builder_.GetGlobalConstantValue(ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX);
    GateRef offset = builder_.IntPtr(JSHClass::PROTOTYPE_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue, iteratorHClass, offset, prototype);

    GateRef result = AllocateArrayIterator(glue, thisArray, iteratorHClass, iterationKind);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

IterationKind TypedNativeInlineLowering::GetArrayIterKindFromBuilin(BuiltinsStubCSigns::ID callID)
{
    switch (callID) {
        case BuiltinsStubCSigns::ID::ArrayEntries:
            return IterationKind::KEY_AND_VALUE;
        case BuiltinsStubCSigns::ID::ArrayKeys:
            return IterationKind::KEY;
        case BuiltinsStubCSigns::ID::ArrayValues:
            return IterationKind::VALUE;
        default:
            UNREACHABLE();
    }
}

void TypedNativeInlineLowering::ReplaceGateWithPendingException(
    GateRef gate, GateRef glue, GateRef state, GateRef depend, GateRef value)
{
    GateRef condition = builder_.HasPendingException(glue);
    auto ifBranch = builder_.Branch(state, condition, 1, BranchWeight::DEOPT_WEIGHT, "checkException");
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);
    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(gate, success, exception, value);
}

void TypedNativeInlineLowering::LowerArrayForEach(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    GateRef usingThis = acc_.GetValueIn(gate, 2);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef length = builder_.GetLengthOfJSArray(thisValue);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label exit(&builder_);
    Label noPendingException(&builder_);
    Label merge(&builder_);
    DEFVALUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(propKey, (&builder_), VariableType::JS_ANY(), builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(*i)));
    DEFVALUE(value, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    BRANCH_CIR(builder_.Int32LessThan(*i, length), &loopHead, &exit);
    builder_.LoopBegin(&loopHead);
    {
        GateRef element = builder_.LoadConstOffset(VariableType::JS_POINTER(), thisValue, JSObject::ELEMENTS_OFFSET);
        value = builder_.GetValueFromTaggedArray(element, *i);
        GateRef nativeCall = builder_.CallInternal(gate,
                                                   {glue,
                                                    builder_.Int64(6),
                                                    builder_.IntPtr(0),
                                                    callBackFn,
                                                    builder_.Undefined(),
                                                    usingThis,
                                                    *value,
                                                    *propKey,
                                                    thisValue},
                                                   acc_.TryGetPcOffset(gate));
        builder_.SetDepend(nativeCall);
        BRANCH_CIR(builder_.HasPendingException(glue), &exit, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            i = builder_.Int32Add(*i, builder_.Int32(1));
            propKey = builder_.ToTaggedIntPtr(builder_.SExtInt32ToInt64(*i));
            BRANCH_CIR(builder_.Int32LessThan(*i, length), &loopEnd, &exit);
        }
    }
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&exit);
    ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), builder_.UndefineConstant());
}

void TypedNativeInlineLowering::LowerArrayFindOrFindIndex(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    BuiltinsArrayStubBuilder arrayBuilder(&env);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    GateRef usingThis = acc_.GetValueIn(gate, 2);
    GateRef builtinFunc = acc_.GetValueIn(gate, 3);
    auto builtinsID = static_cast<BuiltinsStubCSigns::ID>(acc_.GetConstantValue(builtinFunc));
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef length = builder_.GetLengthOfJSArray(thisValue);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label exit(&builder_);
    Label noFindElement(&builder_);
    Label findElement(&builder_);
    Label returnNotFind(&builder_);
    Label returnFind(&builder_);
    Label quit(&builder_);
    Label noPendingException(&builder_);
    DEFVALUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(value, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    DEFVALUE(res, (&builder_), VariableType::INT32(), builder_.Int32(-1));
    DEFVALUE(findRes, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    BRANCH_CIR(builder_.Int32LessThan(*i, length), &loopHead, &exit);
    builder_.LoopBegin(&loopHead);
    {
        GateRef element = builder_.LoadConstOffset(VariableType::JS_POINTER(), thisValue, JSObject::ELEMENTS_OFFSET);
        value = builder_.GetValueFromTaggedArray(element, *i);
        GateRef nativeCall = builder_.CallInternal(gate,
                                                   {glue,
                                                    builder_.Int64(6),
                                                    builder_.IntPtr(0),
                                                    callBackFn,
                                                    builder_.Undefined(),
                                                    usingThis,
                                                    *value,
                                                    builder_.Int32ToTaggedPtr(*i),
                                                    thisValue},
                                                   acc_.TryGetPcOffset(gate));
        builder_.SetDepend(nativeCall);
        BRANCH_CIR(builder_.HasPendingException(glue), &exit, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            BRANCH_CIR(builder_.TaggedIsTrue(builder_.FastToBoolean(nativeCall)), &findElement, &noFindElement);
            builder_.Bind(&noFindElement);
            {
                i = builder_.Int32Add(*i, builder_.Int32(1));
                BRANCH_CIR(builder_.Int32LessThan(*i, length), &loopEnd, &exit);
            }
        }
    }
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&findElement);
    {
        res = *i;
        findRes = *value;
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    if (builtinsID == BuiltinsStubCSigns::ID::ArrayFind) {
        ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *findRes);
    } else {
        ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *res);
    }
}

void TypedNativeInlineLowering::LowerArrayFilter(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    BuiltinsArrayStubBuilder builtinsArrayStubBuilder(&env);
    auto pcOffset = acc_.TryGetPcOffset(gate);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    GateRef usingThis = acc_.GetValueIn(gate, 2);
    GateRef frameState = acc_.GetValueIn(gate, 3);
    GateRef length = builder_.SExtInt32ToInt64(builder_.GetLengthOfJSArray(thisValue));
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef lengthOffset = builder_.IntPtr(JSArray::LENGTH_OFFSET);

    Label lengthIsZero(&builder_);
    Label lengthNotZero(&builder_);
    Label notHole(&builder_);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    Label returnTrue(&builder_);
    Label exit(&builder_);
    Label needTrim(&builder_);
    Label quit(&builder_);
    Label noPendingException(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    DEFVALUE(i, (&builder_), VariableType::INT64(), builder_.Int64(0));
    DEFVALUE(toIndex, (&builder_), VariableType::INT64(), builder_.Int64(0));
    DEFVALUE(propKey, (&builder_), VariableType::JS_ANY(), builder_.ToTaggedIntPtr(*i));
    DEFVALUE(kValue, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    BRANCH_CIR(builder_.Int64Equal(length, builder_.Int64(0)), &lengthIsZero, &lengthNotZero)
    builder_.Bind(&lengthIsZero);
    {
        NewObjectStubBuilder newBuilder(&env);
        result = newBuilder.CreateEmptyArray(glue);
        builder_.Jump(&quit);
    }
    builder_.Bind(&lengthNotZero);
    builder_.DeoptCheck(builder_.Int64LessThanOrEqual(length, builder_.Int64(JSObject::MAX_GAP)),
                        frameState,
                        DeoptType::ARRAYLENGTHOVERMAX);
    GateRef newArray = builtinsArrayStubBuilder.NewArray(glue, length);
    GateRef newArrayEles = builder_.GetElementsArray(newArray);
    builder_.Jump(&loopHead);
    builder_.LoopBegin(&loopHead);
    {
        kValue = builtinsArrayStubBuilder.GetTaggedValueWithElementsKind(glue, thisValue, *i);
        GateRef callJs = builder_.CallInternal(gate,
                                               {glue,
                                                builder_.Int64(6),
                                                builder_.IntPtr(0),
                                                callBackFn,
                                                builder_.Undefined(),
                                                usingThis,
                                                *kValue,
                                                *propKey,
                                                thisValue},
                                               pcOffset);
        builder_.SetDepend(callJs);
        BRANCH_CIR(builder_.HasPendingException(glue), &quit, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            BRANCH_CIR(builder_.TaggedIsTrue(builder_.FastToBoolean(callJs)), &returnTrue, &afterLoop);
            builder_.Bind(&returnTrue);
            {
                builtinsArrayStubBuilder.SetValueWithElementsKind(
                    glue,
                    newArray,
                    *kValue,
                    *toIndex,
                    builder_.Boolean(true),
                    builder_.Int32(Elements::ToUint(ElementsKind::NONE)));
                toIndex = builder_.Int64Add(*toIndex, builder_.Int64(1));
                builder_.Jump(&afterLoop);
            }
        }
        builder_.Bind(&afterLoop);
        {
            i = builder_.Int64Add(*i, builder_.Int64(1));
            propKey = builder_.ToTaggedIntPtr(*i);
            BRANCH_CIR(builder_.Int64LessThan(*i, length), &loopEnd, &exit);
        }
    }
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&exit);
    result = newArray;

    BRANCH_CIR(builder_.Int64LessThan(*toIndex, length), &needTrim, &quit);
    builder_.Bind(&needTrim);
    {
        GateRef trim = builder_.CallNGCRuntime(
            glue, RTSTUB_ID(ArrayTrim), Gate::InvalidGateRef, {glue, newArrayEles, *toIndex}, Circuit::NullGate());
        builder_.SetDepend(trim);
        builder_.Store(VariableType::INT32(), glue, newArray, lengthOffset, builder_.TruncInt64ToInt32(*toIndex));
        builder_.Jump(&quit);
    }
    builder_.Bind(&quit);
    ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerArrayMap(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    BuiltinsArrayStubBuilder builtinsArrayStubBuilder(&env);
    auto pcOffset = acc_.TryGetPcOffset(gate);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    GateRef usingThis = acc_.GetValueIn(gate, 2);
    GateRef frameState = acc_.GetValueIn(gate, 3);
    GateRef length = builder_.SExtInt32ToInt64(builder_.GetLengthOfJSArray(thisValue));
    GateRef glue = acc_.GetGlueFromArgList();

    Label lengthIsZero(&builder_);
    Label lengthNotZero(&builder_);
    Label notHole(&builder_);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label returnTrue(&builder_);
    Label exit(&builder_);
    Label needTrim(&builder_);
    Label finish(&builder_);
    Label noPendingException(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    DEFVALUE(i, (&builder_), VariableType::INT64(), builder_.Int64(0));
    DEFVALUE(toIndex, (&builder_), VariableType::INT64(), builder_.Int64(0));
    DEFVALUE(propKey, (&builder_), VariableType::JS_ANY(), builder_.ToTaggedIntPtr(*i));
    DEFVALUE(kValue, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    BRANCH_CIR(builder_.Int64Equal(length, builder_.Int64(0)), &lengthIsZero, &lengthNotZero)
    builder_.Bind(&lengthIsZero);
    {
        NewObjectStubBuilder newBuilder(&env);
        result = newBuilder.CreateEmptyArray(glue);
        builder_.Jump(&exit);
    }
    builder_.Bind(&lengthNotZero);
    builder_.DeoptCheck(builder_.Int64LessThanOrEqual(length, builder_.Int64(JSObject::MAX_GAP)),
                        frameState,
                        DeoptType::ARRAYLENGTHOVERMAX);
    GateRef newArray = builtinsArrayStubBuilder.NewArray(glue, length);
    builder_.Jump(&loopHead);
    builder_.LoopBegin(&loopHead);
    {
        kValue = builtinsArrayStubBuilder.GetTaggedValueWithElementsKind(glue, thisValue, *i);
        GateRef callJs = builder_.CallInternal(gate,
                                               {glue,
                                                builder_.Int64(6),
                                                builder_.IntPtr(0),
                                                callBackFn,
                                                builder_.Undefined(),
                                                usingThis,
                                                *kValue,
                                                *propKey,
                                                thisValue},
                                               pcOffset);
        builder_.SetDepend(callJs);
        BRANCH_CIR(builder_.HasPendingException(glue), &exit, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            builtinsArrayStubBuilder.SetValueWithElementsKind(
                glue,
                newArray,
                callJs,
                *toIndex,
                builder_.Boolean(true),
                builder_.Int32(Elements::ToUint(ElementsKind::NONE)));
            toIndex = builder_.Int64Add(*toIndex, builder_.Int64(1));
            
            i = builder_.Int64Add(*i, builder_.Int64(1));
            propKey = builder_.ToTaggedIntPtr(*i);
            BRANCH_CIR(builder_.Int64LessThan(*i, length), &loopEnd, &finish);
        }
    }
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&finish);
    {
        result = newArray;
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerArraySome(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    BuiltinsArrayStubBuilder builtinsArrayStubBuilder(&env);
    auto pcOffset = acc_.TryGetPcOffset(gate);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    GateRef usingThis = acc_.GetValueIn(gate, 2);
    GateRef length = builder_.SExtInt32ToInt64(builder_.GetLengthOfJSArray(thisValue));
    GateRef glue = acc_.GetGlueFromArgList();

    Label lengthIsZero(&builder_);
    Label lengthNotZero(&builder_);
    Label notHole(&builder_);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    Label exit(&builder_);
    Label findElement(&builder_);
    Label noPendingException(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedFalse());
    DEFVALUE(i, (&builder_), VariableType::INT64(), builder_.Int64(0));
    DEFVALUE(propKey, (&builder_), VariableType::JS_ANY(), builder_.ToTaggedIntPtr(*i));
    DEFVALUE(kValue, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    BRANCH_CIR(builder_.Int64Equal(length, builder_.Int64(0)), &lengthIsZero, &lengthNotZero)
    builder_.Bind(&lengthIsZero);
    {
        builder_.Jump(&exit);
    }
    builder_.Bind(&lengthNotZero);
    builder_.Jump(&loopHead);
    builder_.LoopBegin(&loopHead);
    kValue = builtinsArrayStubBuilder.GetTaggedValueWithElementsKind(glue, thisValue, *i);
    {
        GateRef callJs = builder_.CallInternal(gate,
                                               {glue,
                                                builder_.Int64(6),
                                                builder_.IntPtr(0),
                                                callBackFn,
                                                builder_.Undefined(),
                                                usingThis,
                                                *kValue,
                                                *propKey,
                                                thisValue},
                                               pcOffset);
        builder_.SetDepend(callJs);
        BRANCH_CIR(builder_.HasPendingException(glue), &exit, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            BRANCH_CIR(builder_.TaggedIsTrue(builder_.FastToBoolean(callJs)), &findElement, &afterLoop);
            builder_.Bind(&afterLoop);
            {
                i = builder_.Int64Add(*i, builder_.Int64(1));
                propKey = builder_.ToTaggedIntPtr(*i);
                BRANCH_CIR(builder_.Int64LessThan(*i, length), &loopEnd, &exit);
            }
        }
    }
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&findElement);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerArrayEvery(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    BuiltinsArrayStubBuilder builtinsArrayStubBuilder(&env);
    auto pcOffset = acc_.TryGetPcOffset(gate);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    GateRef usingThis = acc_.GetValueIn(gate, 2);
    GateRef length = builder_.SExtInt32ToInt64(builder_.GetLengthOfJSArray(thisValue));
    GateRef glue = acc_.GetGlueFromArgList();

    Label lengthIsZero(&builder_);
    Label lengthNotZero(&builder_);
    Label notHole(&builder_);
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    Label exit(&builder_);
    Label callResultNotTrue(&builder_);
    Label noPendingException(&builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedTrue());
    DEFVALUE(i, (&builder_), VariableType::INT64(), builder_.Int64(0));
    DEFVALUE(propKey, (&builder_), VariableType::JS_ANY(), builder_.ToTaggedIntPtr(*i));
    DEFVALUE(kValue, (&builder_), VariableType::JS_ANY(), builder_.Hole());
    BRANCH_CIR(builder_.Int64Equal(length, builder_.Int64(0)), &lengthIsZero, &lengthNotZero)
    builder_.Bind(&lengthIsZero);
    {
        builder_.Jump(&exit);
    }
    builder_.Bind(&lengthNotZero);
    builder_.Jump(&loopHead);
    builder_.LoopBegin(&loopHead);
    kValue = builtinsArrayStubBuilder.GetTaggedValueWithElementsKind(glue, thisValue, *i);
    {
        GateRef callJs = builder_.CallInternal(gate,
                                               {glue,
                                                builder_.Int64(6),
                                                builder_.IntPtr(0),
                                                callBackFn,
                                                builder_.Undefined(),
                                                usingThis,
                                                *kValue,
                                                *propKey,
                                                thisValue},
                                               pcOffset);
        builder_.SetDepend(callJs);
        BRANCH_CIR(builder_.HasPendingException(glue), &exit, &noPendingException);
        builder_.Bind(&noPendingException);
        {
            BRANCH_CIR(builder_.TaggedIsFalse(builder_.FastToBoolean(callJs)), &callResultNotTrue, &afterLoop);
            builder_.Bind(&afterLoop);
            {
                i = builder_.Int64Add(*i, builder_.Int64(1));
                propKey = builder_.ToTaggedIntPtr(*i);
                BRANCH_CIR(builder_.Int64LessThan(*i, length), &loopEnd, &exit);
            }
        }
    }
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&callResultNotTrue);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedNativeInlineLowering::LowerArrayPop(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label arraylengthNotZero(&builder_);
    Label exit(&builder_);
    Label isCOWArray(&builder_);
    Label getElements(&builder_);
    Label indexLessCapacity(&builder_);
    Label slowGetElement(&builder_);
    Label setArrayLength(&builder_);
    Label checkTrim(&builder_);
    Label needTrim(&builder_);
    Label noTrim(&builder_);
    Label isHole(&builder_);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef glue = acc_.GetGlueFromArgList();
    builder_.DeoptCheck(
        builder_.IsStableArrayLengthWriteable(thisValue), acc_.GetValueIn(gate, 1), DeoptType::ARRAYLENGTHNOTWRITABLE);
    GateRef arrayLength = builder_.GetLengthOfJSArray(thisValue);
    DEFVALUE(ret, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    BRANCH_CIR(builder_.Int32Equal(arrayLength, builder_.Int32(0)), &exit, &arraylengthNotZero);
    builder_.Bind(&arraylengthNotZero);
    {
        BRANCH_CIR(builder_.IsJsCOWArray(thisValue), &isCOWArray, &getElements);
        builder_.Bind(&isCOWArray);
        {
            builder_.CallRuntime(glue, RTSTUB_ID(CheckAndCopyArray), builder_.GetDepend(), {thisValue}, glue);
            builder_.Jump(&getElements);
        }
    }
    builder_.Bind(&getElements);
    {
        GateRef elements = builder_.GetElementsArray(thisValue);
        GateRef capacity = builder_.GetLengthOfTaggedArray(elements);
        GateRef index = builder_.Int32Sub(arrayLength, builder_.Int32(1));
        BRANCH_CIR(builder_.Int32LessThan(index, capacity), &indexLessCapacity, &setArrayLength);
        builder_.Bind(&indexLessCapacity);
        {
            GateRef result = builder_.GetValueFromTaggedArray(elements, index);
            BRANCH_CIR(builder_.TaggedIsHole(result), &slowGetElement, &checkTrim);
            builder_.Bind(&checkTrim);
            {
                ret = result;
                GateRef unused = builder_.Int32Sub(capacity, index);
                BRANCH_CIR(
                    builder_.Int32GreaterThan(unused, builder_.Int32(TaggedArray::MAX_END_UNUSED)), &needTrim, &noTrim);
            }
            builder_.Bind(&slowGetElement);
            {
                Label hasException(&builder_);
                Label notHasException(&builder_);
                GateRef element = builder_.CallStub(glue, gate, CommonStubCSigns::GetPropertyByIndex,
                                                    { glue, thisValue, index });
                BRANCH_CIR(builder_.HasPendingException(glue), &hasException, &notHasException);
                builder_.Bind(&hasException);
                {
                    ret = builder_.ExceptionConstant();
                    builder_.Jump(&exit);
                }
                builder_.Bind(&notHasException);
                {
                    ret = element;
                    builder_.Jump(&setArrayLength);
                }
            }
        }
        builder_.Bind(&needTrim);
        {
            builder_.CallNGCRuntime(glue,
                                    RTSTUB_ID(ArrayTrim),
                                    builder_.GetDepend(),
                                    {glue, elements, builder_.ZExtInt32ToInt64(index)},
                                    Circuit::NullGate());
            builder_.Jump(&setArrayLength);
        }
        builder_.Bind(&noTrim);
        {
            builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, builder_.HoleConstant());
            builder_.Jump(&setArrayLength);
        }
        builder_.Bind(&setArrayLength);
        {
            GateRef lengthOffset = builder_.IntPtr(JSArray::LENGTH_OFFSET);
            builder_.Store(VariableType::INT32(), glue, thisValue, lengthOffset, index);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *ret);
}

void TypedNativeInlineLowering::LowerArrayPush(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label grow(&builder_);
    Label setValue(&builder_);
    Label setLength(&builder_);
    Label exit(&builder_);
    BuiltinsArrayStubBuilder arrayBuilder(&env);
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef oldLength = builder_.GetLengthOfJSArray(thisValue);
    GateRef glue = acc_.GetGlueFromArgList();
    DEFVALUE(ret, (&builder_), VariableType::JS_ANY(), builder_.Int32ToTaggedPtr(thisValue));
    DEFVALUE(elements, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    GateRef newLength = builder_.Int32Add(oldLength, builder_.Int32(1));
    elements = builder_.GetElementsArray(thisValue);
    GateRef capacity = builder_.GetLengthOfTaggedArray(*elements);
    BRANCH_CIR(builder_.Int32GreaterThan(newLength, capacity), &grow, &setValue);
    builder_.Bind(&grow);
    {
        elements = builder_.CallStub(glue, gate, CommonStubCSigns::GrowElementsCapacity, {glue, thisValue, newLength});
        builder_.Jump(&setValue);
    }
    builder_.Bind(&setValue);
    {
        arrayBuilder.FastSetValueWithElementsKind(glue, thisValue, *elements, value, oldLength, ElementsKind::NONE);
        builder_.Jump(&setLength);
    }
    builder_.Bind(&setLength);
    {
        arrayBuilder.SetArrayLength(glue, thisValue, newLength);
        ret = arrayBuilder.IntToTaggedPtr(newLength);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *ret);
}

void TypedNativeInlineLowering::LowerArraySlice(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label exit(&builder_);
    Label checkIndexDone(&builder_);
    Label sliceCountIsZero(&builder_);
    Label sliceCountIsNotZero(&builder_);
    Label inThisEles(&builder_);
    Label outThisEles(&builder_);

    Label indexInRange(&builder_);
    Label setElementToNewArray(&builder_);

    GateRef thisArray = acc_.GetValueIn(gate, 0);
    GateRef startHandler = acc_.GetValueIn(gate, 1);
    GateRef endHandler = acc_.GetValueIn(gate, 2);
    GateRef frameState = acc_.GetValueIn(gate, 3);
    GateRef length = builder_.GetLengthOfJSArray(thisArray);
    GateRef glue = acc_.GetGlueFromArgList();
    DEFVALUE(start, (&builder_), VariableType::INT32(), builder_.Int32(0));
    DEFVALUE(end, (&builder_), VariableType::INT32(), length);
    DEFVALUE(res, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());

    CheckAndCalcuSliceIndex(length, startHandler, endHandler, &exit, &checkIndexDone, &res, &start, &end);
    builder_.Bind(&checkIndexDone);
    GateRef sliceCount = builder_.Int32Sub(*end, *start);
    builder_.Branch(builder_.Int32Equal(sliceCount, builder_.Int32(0)), &sliceCountIsZero, &sliceCountIsNotZero);
    builder_.Bind(&sliceCountIsZero);
    {
        NewObjectStubBuilder newObject(&env);
        res = newObject.CreateEmptyArray(glue);
        builder_.Jump(&exit);
    }
    builder_.Bind(&sliceCountIsNotZero);
    builder_.DeoptCheck(builder_.Int32LessThanOrEqual(sliceCount, builder_.Int32(JSObject::MAX_GAP)),
                        frameState,
                        DeoptType::ARRAYLENGTHOVERMAX);
    BuiltinsArrayStubBuilder arrayBuilder(&env);
    GateRef newArray = arrayBuilder.NewArray(glue, sliceCount);
    GateRef newArrayElements = builder_.GetElementsArray(newArray);
    GateRef thisElements = builder_.GetElementsArray(thisArray);
    GateRef thisElementsLen = builder_.GetLengthOfTaggedArray(thisElements);
    BRANCH_CIR(
        builder_.Int32GreaterThan(thisElementsLen, builder_.Int32Add(*start, sliceCount)), &inThisEles, &outThisEles);
    builder_.Bind(&inThisEles);
    {
        Label loopHead(&builder_);
        Label loopEnd(&builder_);
        Label copyElement(&builder_);
        Label loopExit(&builder_);
        Label afterLoop(&builder_);
        DEFVALUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
        builder_.Jump(&loopHead);
        builder_.LoopBegin(&loopHead);
        {
            BRANCH_CIR(builder_.Int32LessThan(*i, sliceCount), &copyElement, &loopExit);
            builder_.Bind(&copyElement);
            {
                GateRef copyPosition = builder_.Int32Add(*start, *i);
                GateRef ele = builder_.GetValueFromTaggedArray(thisElements, copyPosition);
                builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, newArrayElements, *i, ele);
                builder_.Jump(&afterLoop);
            }
        }
        builder_.Bind(&afterLoop);
        {
            i = builder_.Int32Add(*i, builder_.Int32(1));
            builder_.Jump(&loopEnd);
        }
        builder_.Bind(&loopEnd);
        builder_.LoopEnd(&loopHead);
        builder_.Bind(&loopExit);
        {
            res = newArray;
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&outThisEles);
    {
        Label loopHead(&builder_);
        Label loopEnd(&builder_);
        Label copyElement(&builder_);
        Label loopExit(&builder_);
        Label afterLoop(&builder_);
        DEFVALUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
        builder_.Jump(&loopHead);
        builder_.LoopBegin(&loopHead);
        {
            BRANCH_CIR(builder_.Int32LessThan(*i, sliceCount), &copyElement, &loopExit);
            builder_.Bind(&copyElement);
            {
                GateRef copyPosition = builder_.Int32Add(*i, *start);
                DEFVALUE(element, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
                BRANCH_CIR(
                    builder_.Int32GreaterThan(thisElementsLen, copyPosition), &indexInRange, &setElementToNewArray);
                builder_.Bind(&indexInRange);
                {
                    element = builder_.GetValueFromTaggedArray(thisElements, copyPosition);
                    builder_.Jump(&setElementToNewArray);
                }
                builder_.Bind(&setElementToNewArray);
                {
                    builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, newArrayElements, *i, *element);
                    builder_.Jump(&afterLoop);
                }
            }
            builder_.Bind(&afterLoop);
            {
                i = builder_.Int32Add(*i, builder_.Int32(1));
                builder_.Jump(&loopEnd);
            }
            builder_.Bind(&loopEnd);
            builder_.LoopEnd(&loopHead);
            builder_.Bind(&loopExit);
            {
                res = newArray;
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *res);
}

void TypedNativeInlineLowering::CheckAndCalcuSliceIndex(GateRef length,
                                                        GateRef startHandler,
                                                        GateRef endHandler,
                                                        Label* exit,
                                                        Label* checkIndexDone,
                                                        Variable* res,
                                                        Variable* start,
                                                        Variable* end)
{
    Label startUseZero(&builder_);
    Label startNotUndef(&builder_);
    Label returnEmptyArray(&builder_);
    Label startGreaterOrEqualZero(&builder_);
    Label startLessZero(&builder_);
    Label startLessLength(&builder_);
    Label startCheckDone(&builder_);
    Label startCalcu(&builder_);
    Label endUseLength(&builder_);
    Label endNotUndef(&builder_);
    Label endLessZero(&builder_);
    Label endGreaterZero(&builder_);
    Label endUseMixed(&builder_);
    Label endUseZero(&builder_);
    Label checkEndLessStart(&builder_);
    Label endUseHandler(&builder_);
    BRANCH_CIR(builder_.TaggedIsUndefined(startHandler), &startUseZero, &startNotUndef);
    builder_.Bind(&startNotUndef);
    {
        GateRef tempStart = builder_.GetInt32OfTInt(startHandler);
        BRANCH_CIR(builder_.Int32LessThan(tempStart, builder_.Int32(0)), &startLessZero, &startGreaterOrEqualZero);
        builder_.Bind(&startGreaterOrEqualZero);
        {
            BRANCH_CIR(builder_.Int32GreaterThanOrEqual(tempStart, length), &returnEmptyArray, &startLessLength);
            builder_.Bind(&startLessLength);
            {
                start->WriteVariable(tempStart);
                builder_.Jump(&startCheckDone);
            }
        }
        builder_.Bind(&startLessZero);
        {
            GateRef negativeLength = builder_.Int32Sub(builder_.Int32(0), length);
            BRANCH_CIR(builder_.Int32LessThan(tempStart, negativeLength), &startUseZero, &startCalcu);
            builder_.Bind(&startCalcu);
            {
                start->WriteVariable(builder_.Int32Add(tempStart, length));
                builder_.Jump(&startCheckDone);
            }
        }
        builder_.Bind(&startUseZero);
        {
            start->WriteVariable(builder_.Int32(0));
            builder_.Jump(&startCheckDone);
        }
    }
    builder_.Bind(&startCheckDone);
    {
        BRANCH_CIR(builder_.TaggedIsUndefined(endHandler), &endUseLength, &endNotUndef);
        builder_.Bind(&endNotUndef);
        {
            GateRef tempEnd = builder_.GetInt32OfTInt(endHandler);
            BRANCH_CIR(builder_.Int32LessThan(tempEnd, builder_.Int32(0)), &endLessZero, &endGreaterZero);
            builder_.Bind(&endLessZero);
            {
                GateRef endMixed = builder_.Int32Add(tempEnd, length);
                BRANCH_CIR(builder_.Int32LessThan(endMixed, builder_.Int32(0)), &endUseZero, &endUseMixed);
                builder_.Bind(&endUseMixed);
                {
                    end->WriteVariable(endMixed);
                    builder_.Jump(&checkEndLessStart);
                }
                builder_.Bind(&endUseZero);
                {
                    end->WriteVariable(builder_.Int32(0));
                    builder_.Jump(&checkEndLessStart);
                }
            }
            builder_.Bind(&endGreaterZero);
            {
                BRANCH_CIR(builder_.Int32GreaterThanOrEqual(tempEnd, length), &endUseLength, &endUseHandler);
                builder_.Bind(&endUseHandler);
                {
                    end->WriteVariable(tempEnd);
                    builder_.Jump(&checkEndLessStart);
                }
            }
        }
        builder_.Bind(&endUseLength);
        {
            end->WriteVariable(length);
            builder_.Jump(&checkEndLessStart);
        }
        builder_.Bind(&checkEndLessStart);
        {
            BRANCH_CIR(builder_.Int32GreaterThan(start->ReadVariable(), end->ReadVariable()),
                       &returnEmptyArray,
                       checkIndexDone);
        }
    }
    builder_.Bind(&returnEmptyArray);
    {
        NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment());
        res->WriteVariable(newBuilder.CreateEmptyArray(acc_.GetGlueFromArgList()));
        builder_.Jump(exit);
    }
}

void TypedNativeInlineLowering::LowerArraySort(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = acc_.GetGlueFromArgList();
    GateRef thisValue = acc_.GetValueIn(gate, 0);
    GateRef callBackFn = acc_.GetValueIn(gate, 1);
    BuiltinsArrayStubBuilder arrayStubBuilder(&env);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label slowPath(&env);
    Label exit(&env);
    arrayStubBuilder.SortAfterArgs(glue, thisValue, callBackFn, &result, &exit, &slowPath, gate);
    builder_.Bind(&slowPath);
    {
        result.WriteVariable(builder_.CallRuntime(glue, RTSTUB_ID(ArraySort),
                                                  Gate::InvalidGateRef, { thisValue }, gate));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetStateDepend(), *result);
}
}
