/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
BuiltinsArrayStubBuilder::IndexOfOptions BuiltinsArrayStubBuilder::MakeIndexOfOptions(
    BuiltinsStubCSigns::ID id, bool resultIsTagged)
{
    IndexOfOptions options;
    switch (id) {
        case BuiltinsStubCSigns::ID::ArrayIndexOf:
        case BuiltinsStubCSigns::ID::ArrayLastIndexOf:
            options.compType = ComparisonType::STRICT_EQUAL;
            options.returnType = resultIsTagged
                ? IndexOfReturnType::TAGGED_FOUND_INDEX
                : IndexOfReturnType::UNTAGGED_FOUND_INDEX;
            options.holeAsUndefined = false;
            options.reversedOrder = (id == BuiltinsStubCSigns::ID::ArrayLastIndexOf);
            break;
        case BuiltinsStubCSigns::ID::ArrayIncludes:
            options.compType = ComparisonType::SAME_VALUE_ZERO;
            options.returnType = resultIsTagged
                ? IndexOfReturnType::TAGGED_FOUND_OR_NOT
                : IndexOfReturnType::UNTAGGED_FOUND_OR_NOT;
            options.holeAsUndefined = true;
            options.reversedOrder = false;
            break;
        default:
            LOG_ECMA(FATAL) << "Implementation Error: Unreachable branch.";
            UNREACHABLE();
    }
    return options;
}

GateRef BuiltinsArrayStubBuilder::MakeFromIndex(GateRef input, GateRef length, bool reversedOrder)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), TaggedIsNumber(input));

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label inputIsTaggedInt(env);
    Label inputIsTaggedDouble(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT64(), Int64(0));

    BRANCH_LIKELY(TaggedIsInt(input), &inputIsTaggedInt, &inputIsTaggedDouble);
    Bind(&inputIsTaggedInt);
    result.WriteVariable(MakeFromIndexWithInt(TaggedGetInt(input), length, reversedOrder));
    Jump(&exit);

    Bind(&inputIsTaggedDouble);
    result.WriteVariable(MakeFromIndexWithDouble(GetDoubleOfTDouble(input), length, reversedOrder));
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::MakeFromIndexWithInt(GateRef intValue, GateRef length, bool reversedOrder)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label reachesLength(env);
    Label lessThanLength(env);
    Label isNegative(env);
    Label plusLengthIsNegative(env);
    Label plusLengthIsNonNegative(env);
    Label exit(env);

    GateRef i64Value = SExtInt32ToInt64(intValue);
    GateRef i64Length = ZExtInt32ToInt64(length);
    DEFVARIABLE(result, VariableType::INT64(), i64Value);

    BRANCH_UNLIKELY(Int64GreaterThanOrEqual(i64Value, i64Length), &reachesLength, &lessThanLength);
    Bind(&reachesLength);
    if (reversedOrder) {
        result.WriteVariable(Int64Sub(i64Length, Int64(1))); // lastIndexOf
    } else {
        result.WriteVariable(i64Length); // indexOf, includes
    }
    Jump(&exit);
    // No change to result if it falls in range [0, length - 1]
    Bind(&lessThanLength);
    BRANCH(Int64LessThan(i64Value, Int64(0)), &isNegative, &exit);

    Bind(&isNegative);
    GateRef i64PlusLength = Int64Add(i64Value, i64Length);
    BRANCH(Int64LessThan(i64PlusLength, Int64(0)), &plusLengthIsNegative, &plusLengthIsNonNegative);

    Bind(&plusLengthIsNegative);
    if (reversedOrder) {
        result.WriteVariable(Int64(-1)); // lastIndexOf
    } else {
        result.WriteVariable(Int64(0)); // indexOf, includes
    }
    Jump(&exit);

    Bind(&plusLengthIsNonNegative);
    result.WriteVariable(i64PlusLength);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::MakeFromIndexWithDouble(GateRef doubleValue, GateRef length, bool reversedOrder)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label isNotNAN(env);
    Label reachesLength(env);
    Label lessThanLength(env);
    Label isNonNegativeAfterTruncation(env);
    Label isNegative(env);
    Label plusLengthIsNonNegative(env);
    Label plusLengthIsNegative(env);
    Label exit(env);

    DEFVARIABLE(result, VariableType::INT64(), Int64(0));

    BRANCH_UNLIKELY(DoubleIsNAN(doubleValue), &exit, &isNotNAN); // NaN -> 0
    Bind(&isNotNAN);
    GateRef f64Length = ChangeUInt32ToFloat64(length);
    BRANCH(DoubleGreaterThanOrEqual(doubleValue, f64Length), &reachesLength, &lessThanLength);

    Bind(&reachesLength); // Including the case where doubleValue is +inf
    GateRef i64Length = ZExtInt32ToInt64(length);
    if (reversedOrder) {
        result.WriteVariable(Int64Sub(i64Length, Int64(1))); // lastIndexOf
    } else {
        result.WriteVariable(i64Length); // indexOf, includes
    }
    Jump(&exit);

    Bind(&lessThanLength);
    GateRef trunkValue = DoubleTrunc(doubleValue);
    BRANCH(DoubleGreaterThanOrEqual(trunkValue, Double(0.0)), &isNonNegativeAfterTruncation, &isNegative);

    Bind(&isNonNegativeAfterTruncation);
    result.WriteVariable(ZExtInt32ToInt64(ChangeFloat64ToInt32(trunkValue)));
    Jump(&exit);

    Bind(&isNegative);
    GateRef plusLength = DoubleAdd(trunkValue, f64Length);
    // If doubleValue + length < 0, then result is 0. (Including the case where doubleValue is -inf)
    BRANCH(DoubleGreaterThanOrEqual(plusLength, Double(0.0)), &plusLengthIsNonNegative, &plusLengthIsNegative);

    Bind(&plusLengthIsNonNegative);
    result.WriteVariable(ZExtInt32ToInt64(ChangeFloat64ToInt32(plusLength)));
    Jump(&exit);

    Bind(&plusLengthIsNegative);
    if (reversedOrder) {
        result.WriteVariable(Int64(-1)); // lastIndexOf
    } // indexOf, includes: result is 0
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

Variable BuiltinsArrayStubBuilder::MakeResultVariableDefaultNotFound(IndexOfOptions options)
{
    VariableType type;
    GateRef value;
    switch (options.returnType) {
        case IndexOfReturnType::TAGGED_FOUND_INDEX:
            type = VariableType::JS_ANY();
            value = IntToTaggedPtr(Int32(-1));
            break;
        case IndexOfReturnType::TAGGED_FOUND_OR_NOT:
            type = VariableType::JS_ANY();
            value = TaggedFalse();
            break;
        case IndexOfReturnType::UNTAGGED_FOUND_INDEX:
            type = VariableType::INT32();
            value = Int32(-1);
            break;
        case IndexOfReturnType::UNTAGGED_FOUND_OR_NOT:
            type = VariableType::BOOL();
            value = False();
            break;
        default:
            LOG_ECMA(FATAL) << "Implementation Error: Unreachable branch.";
            UNREACHABLE();
    }
    return Variable(GetEnvironment(), type, NextVariableId(), value);
}

template <class Predicate>
GateRef BuiltinsArrayStubBuilder::IndexOfElements(
    GateRef elements, Predicate predicate, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    static_assert(std::is_invocable_r_v<GateRef, Predicate, GateRef>, "Invalid call signature.");
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label loopHead(env);
    Label loopBodyBegin(env);
    Label loopBodyEnd(env);
    Label loopExitFound(env);
    Label exit(env);

    GateRef sizeBytesPerElement = IntPtr(JSTaggedValue::TaggedTypeSize());
    // Decomposed from StubBuilder::GetValueFromTaggedArray
    auto indexToOffset = [this, sizeBytesPerElement](GateRef i64Index) {
        return PtrAdd(
            PtrMul(sizeBytesPerElement, ChangeInt64ToIntPtr(i64Index)),
            IntPtr(TaggedArray::DATA_OFFSET));
    };

    GateRef boundOffset;
    GateRef initialOffset;
    if (options.reversedOrder) {
        // lastIndexOf: Closed range [0, fromIndex]
        boundOffset = IntPtr(TaggedArray::DATA_OFFSET);
        initialOffset = indexToOffset(fromIndex);
    } else {
        // indexOf: Half-closed range [fromIndex, len)
        initialOffset = indexToOffset(fromIndex);
        boundOffset = indexToOffset(ZExtInt32ToInt64(len));
    }
    DEFVARIABLE(curOffset, VariableType::NATIVE_POINTER(), initialOffset);
    Variable result = MakeResultVariableDefaultNotFound(options);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef continues = options.reversedOrder
            ? IntPtrGreaterThanOrEqual(*curOffset, boundOffset)
            : IntPtrLessThan(*curOffset, boundOffset);
        BRANCH_LIKELY(continues, &loopBodyBegin, &exit);

        Bind(&loopBodyBegin);
        GateRef curValue = Load(VariableType::JS_ANY(), elements, *curOffset);
        GateRef curValueMatches = std::invoke(predicate, curValue);
        BRANCH_UNLIKELY(curValueMatches, &loopExitFound, &loopBodyEnd);

        Bind(&loopBodyEnd);
        if (options.reversedOrder) {
            curOffset = PtrSub(*curOffset, sizeBytesPerElement);
        } else {
            curOffset = PtrAdd(*curOffset, sizeBytesPerElement);
        }
    }
    LoopEnd(&loopHead);

    Bind(&loopExitFound);
    switch (options.returnType) {
        case IndexOfReturnType::TAGGED_FOUND_INDEX:
        case IndexOfReturnType::UNTAGGED_FOUND_INDEX: {
            GateRef iptrIndex = IntPtrDiv(
                PtrSub(*curOffset, IntPtr(TaggedArray::DATA_OFFSET)),
                sizeBytesPerElement);
            // Note: We assume the size of a stable array will never exceed 2^31 - 1 (i.e. max of signed int32).
            GateRef i32Index = ChangeIntPtrToInt32(iptrIndex);
            GateRef resultIndex = (options.returnType == IndexOfReturnType::TAGGED_FOUND_INDEX)
                ? IntToTaggedPtr(i32Index)
                : i32Index;
            result.WriteVariable(resultIndex);
            break;
        }
        case IndexOfReturnType::TAGGED_FOUND_OR_NOT:
            result.WriteVariable(TaggedTrue());
            break;
        case IndexOfReturnType::UNTAGGED_FOUND_OR_NOT:
            result.WriteVariable(True());
            break;
        default:
            LOG_ECMA(FATAL) << "Implementation Error: Unreachable branch.";
            UNREACHABLE();
    }
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::StringEqual(
    GateRef glue, GateRef left, GateRef right, StringElementsCondition rightCondition)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label rightIsString(env);
    Label addrEquals(env);
    Label addrNotEquals(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());

    if (rightCondition == StringElementsCondition::MAY_BE_ANY) {
        BRANCH(TaggedIsString(right), &rightIsString, &exit);
        Bind(&rightIsString);
    } else if (rightCondition == StringElementsCondition::MAY_BE_HOLE) {
        BRANCH(TaggedIsNotHole(right), &rightIsString, &exit);
        Bind(&rightIsString);
    }
    BRANCH_UNLIKELY(Equal(left, right), &addrEquals, &addrNotEquals);
    Bind(&addrEquals);
    result.WriteVariable(True());
    Jump(&exit);
    Bind(&addrNotEquals);
    BuiltinsStringStubBuilder stringBuilder(env);
    result.WriteVariable(stringBuilder.FastStringEqualWithoutRTStub(glue, left, right));
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::BigIntEqual(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(predResult, VariableType::BOOL(), False());

    Label addrEquals(env);
    Label addrNotEquals(env);
    Label rightIsBigInt(env);
    Label exit(env);

    BRANCH(Equal(left, right), &addrEquals, &addrNotEquals);
    Bind(&addrEquals);
    predResult.WriteVariable(True());
    Jump(&exit);
    Bind(&addrNotEquals);
    BRANCH(TaggedIsBigInt(right), &rightIsBigInt, &exit);
    Bind(&rightIsBigInt);
    predResult.WriteVariable(CallNGCRuntime(glue, RTSTUB_ID(BigIntSameValueZero), { left, right }));
    Jump(&exit);

    Bind(&exit);
    GateRef ret = predResult.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfTaggedUndefined(
    GateRef elements, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    if (options.holeAsUndefined) {
        return IndexOfElements(elements, [this](GateRef curValue) {
            return BitOr(TaggedIsUndefined(curValue), TaggedIsHole(curValue));
        }, fromIndex, len, options);
    }
    return IndexOfElements(elements, [this](GateRef curValue) {
        return TaggedIsUndefined(curValue);
    }, fromIndex, len, options);
}

GateRef BuiltinsArrayStubBuilder::IndexOfTaggedZero(
    GateRef elements, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    auto env = GetEnvironment();
    return IndexOfElements(elements, [this, env](GateRef curValue) {
        GateRef underlying = ChangeTaggedPointerToInt64(curValue);
        return LogicOrBuilder(env)
            .Or(Equal(Int64(JSTaggedValue::VALUE_ZERO), underlying))
            .Or(Equal(Int64(JSTaggedValue::VALUE_POSITIVE_ZERO), underlying))
            .Or(Equal(Int64(JSTaggedValue::VALUE_NEGATIVE_ZERO), underlying))
            .Done();
    }, fromIndex, len, options);
}

GateRef BuiltinsArrayStubBuilder::IndexOfTaggedIntElements(
    GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    // assume: ElementsKind is INT or HOLE_INT
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), BoolNot(TaggedIsUndefined(target)));

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label targetIsInt(env);
    Label targetIsNotInt(env);
    Label targetIsDouble(env);
    Label targetIsWithinInt32(env);
    Label exit(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    BRANCH_LIKELY(TaggedIsInt(target), &targetIsInt, &targetIsNotInt);
    Bind(&targetIsInt);
    result = IndexOfElements(elements, [this, ti32 = target](GateRef curValue) {
        return Equal(curValue, ti32);
    }, fromIndex, len, options);
    Jump(&exit);

    Bind(&targetIsNotInt);
    BRANCH_LIKELY(TaggedIsDouble(target), &targetIsDouble, &exit);
    Bind(&targetIsDouble);
    GateRef doubleValue = GetDoubleOfTDouble(target);
    BRANCH(DoubleIsWithinInt32(doubleValue), &targetIsWithinInt32, &exit);

    Bind(&targetIsWithinInt32);
    GateRef taggedInt32 = IntToTaggedPtr(ChangeFloat64ToInt32(doubleValue));
    result = IndexOfElements(elements, [this, ti32 = taggedInt32](GateRef curValue) {
        return Equal(curValue, ti32);
    }, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfTaggedIntTarget(
    GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), TaggedIsInt(target));

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label isZero(env);
    Label isNotZero(env);
    Label exit(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    GateRef int32Value = TaggedGetInt(target);
    BRANCH(Int32Equal(Int32(0), int32Value), &isZero, &isNotZero);
    Bind(&isZero);
    result = IndexOfTaggedZero(elements, fromIndex, len, options);
    Jump(&exit);
    Bind(&isNotZero);
    GateRef taggedDouble = DoubleToTaggedDoublePtr(ChangeInt32ToFloat64(int32Value));
    result = IndexOfElements(elements, [this, ti32 = target, tf64 = taggedDouble](GateRef curValue) {
        return BitOr(Equal(curValue, ti32), Equal(curValue, tf64));
    }, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfTaggedNumber(
    GateRef elements, GateRef target, GateRef fromIndex, GateRef len,
    IndexOfOptions options, bool targetIsAlwaysDouble)
{
    if (targetIsAlwaysDouble) {
        // No constraints to ElementsKind
        ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), TaggedIsDouble(target));
    } else {
        // assume: ElementsKind is NUMBER or HOLE_NUMBER
        ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), BoolNot(TaggedIsUndefined(target)));
    }

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label targetIsWithinInt32(env);
    Label targetDone(env);
    Label targetIsNaN(env);
    Label targetIsNotNaN(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    DEFVARIABLE(doubleValue, VariableType::FLOAT64(),  Double(0.0));
    DEFVARIABLE(taggedFloat, VariableType::JS_ANY(),  Undefined());
    DEFVARIABLE(taggedInt32, VariableType::JS_ANY(), Undefined());

    if (!targetIsAlwaysDouble) {
        Label targetIsInt(env);
        Label targetIsNotInt(env);
        Label targetIsDouble(env);

        BRANCH(TaggedIsInt(target), &targetIsInt, &targetIsNotInt);
        Bind(&targetIsInt);
        doubleValue = ChangeInt32ToFloat64(TaggedGetInt(target));
        taggedFloat = DoubleToTaggedDoublePtr(*doubleValue);
        taggedInt32 = target;
        Jump(&targetDone);

        Bind(&targetIsNotInt);
        BRANCH_LIKELY(TaggedIsDouble(target), &targetIsDouble, &exit);
        Bind(&targetIsDouble);
    }
    // If doubleValue is not within int32, then ti32 is left Undefined().
    doubleValue = GetDoubleOfTDouble(target);
    taggedFloat = target;
    BRANCH(DoubleIsWithinInt32(*doubleValue), &targetIsWithinInt32, &targetDone);
    Bind(&targetIsWithinInt32);
    taggedInt32 = IntToTaggedPtr(ChangeFloat64ToInt32(*doubleValue));
    Jump(&targetDone);

    Bind(&targetDone);
    BRANCH_UNLIKELY(DoubleIsNAN(*doubleValue), &targetIsNaN, &targetIsNotNaN);

    Bind(&targetIsNaN);
    if (options.compType == ComparisonType::SAME_VALUE_ZERO) {
        result = IndexOfElements(elements, [this](GateRef curValue) {
            return DoubleIsNAN(GetDoubleOfTDouble(curValue));
        }, fromIndex, len, options);
    }
    Jump(&exit);

    Bind(&targetIsNotNaN);
    Label isZero(env);
    Label isNotZero(env);
    Label withinInt32Branch(env);
    Label notWithinInt32Branch(env);

    BRANCH(DoubleEqual(Double(0.0), *doubleValue), &isZero, &isNotZero);
    Bind(&isZero);
    result = IndexOfTaggedZero(elements, fromIndex, len, options);
    Jump(&exit);
    Bind(&isNotZero);
    // If target is not within int32, then taggedInt32 is left Undefined().
    BRANCH(TaggedIsInt(*taggedInt32), &withinInt32Branch, &notWithinInt32Branch);
    Bind(&withinInt32Branch);
    {
        auto predicate = [this, ti32 = *taggedInt32, tf64 = *taggedFloat](GateRef curValue) {
            return BitOr(Equal(curValue, ti32), Equal(curValue, tf64));
        };
        result = IndexOfElements(elements, predicate, fromIndex, len, options);
        Jump(&exit);
    }
    Bind(&notWithinInt32Branch);
    result = IndexOfElements(elements, [this, tf64 = *taggedFloat](GateRef curValue) {
        return Equal(curValue, tf64);
    }, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfStringElements(
    GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len,
    IndexOfOptions options, StringElementsCondition condition)
{
    // assume: ElementsKind is STRING or HOLE_STRING
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), BoolNot(TaggedIsUndefined(target)));

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label targetIsString(env);
    Label exit(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    BRANCH_LIKELY(TaggedIsString(target), &targetIsString, &exit);
    Bind(&targetIsString);
    result = IndexOfElements(elements, [this, glue, condition, target](GateRef curValue) {
        return StringEqual(glue, target, curValue, condition);
    }, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfStringTarget(
    GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), TaggedIsString(target));
    return IndexOfElements(elements, [this, glue, target](GateRef curValue) {
        return StringEqual(glue, target, curValue, StringElementsCondition::MAY_BE_ANY);
    }, fromIndex, len, options);
}

GateRef BuiltinsArrayStubBuilder::IndexOfBigInt(
    GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), TaggedIsBigInt(target));
    return IndexOfElements(elements, [this, glue, target](GateRef curValue) {
        return BigIntEqual(glue, target, curValue);
    }, fromIndex, len, options);
}

GateRef BuiltinsArrayStubBuilder::IndexOfObject(
    GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    return IndexOfElements(elements, [this, target](GateRef curValue) {
        return Equal(target, curValue);
    }, fromIndex, len, options);
}

GateRef BuiltinsArrayStubBuilder::IndexOfBigIntOrObjectElements(
    GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    // assume: ElementsKind is OBJECT or HOLE_OBJECT
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), BoolNot(TaggedIsUndefined(target)));

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label targetIsBigInt(env);
    Label targetIsNotBigInt(env);
    Label targetIsObject(env);
    Label exit(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    BRANCH(TaggedIsBigInt(target), &targetIsBigInt, &targetIsNotBigInt);
    Bind(&targetIsBigInt);
    result = IndexOfBigInt(glue, elements, target, fromIndex, len, options);
    Jump(&exit);
    Bind(&targetIsNotBigInt);
    BRANCH(TaggedIsObject(target), &targetIsObject, &exit);
    // Including true, false, null
    Bind(&targetIsObject);
    result = IndexOfObject(elements, target, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfBigIntOrObjectTarget(
    GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label targetIsBigInt(env);
    Label targetIsObject(env);
    Label exit(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    BRANCH(TaggedIsBigInt(target), &targetIsBigInt, &targetIsObject);
    Bind(&targetIsBigInt);
    result = IndexOfBigInt(glue, elements, target, fromIndex, len, options);
    Jump(&exit);
    // Including true, false, null
    Bind(&targetIsObject);
    result = IndexOfObject(elements, target, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayStubBuilder::IndexOfGeneric(
    GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ArrayIndexOf), BoolNot(TaggedIsUndefined(target)));

    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Variable result = MakeResultVariableDefaultNotFound(options);

    Label intBranch(env);
    Label targetNotInt(env);
    BRANCH(TaggedIsInt(target), &intBranch, &targetNotInt);
    Bind(&intBranch);
    result = IndexOfTaggedIntTarget(elements, target, fromIndex, len, options);
    Jump(&exit);

    Bind(&targetNotInt);
    Label doubleBranch(env);
    Label targetNotNumber(env);
    BRANCH(TaggedIsDouble(target), &doubleBranch, &targetNotNumber);
    Bind(&doubleBranch);
    result = IndexOfTaggedNumber(elements, target, fromIndex, len, options, true);
    Jump(&exit);

    Bind(&targetNotNumber);
    Label stringBranch(env);
    Label bigIntOrObjectBranch(env);
    BRANCH(TaggedIsString(target), &stringBranch, &bigIntOrObjectBranch);
    Bind(&stringBranch);
    result = IndexOfStringTarget(glue, elements, target, fromIndex, len, options);
    Jump(&exit);

    Bind(&bigIntOrObjectBranch);
    result = IndexOfBigIntOrObjectTarget(glue, elements, target, fromIndex, len, options);
    Jump(&exit);

    Bind(&exit);
    GateRef ret = result.ReadVariable();
    env->SubCfgExit();
    return ret;
}
} // namespace panda::ecmascript::kungfu
