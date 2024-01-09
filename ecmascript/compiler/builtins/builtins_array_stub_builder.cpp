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

#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/base/array_helper.h"

namespace panda::ecmascript::kungfu {
void BuiltinsArrayStubBuilder::Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    {
        Label isExtensible(env);
        Branch(HasConstructor(thisValue), slowPath, &isExtensible);
        Bind(&isExtensible);
        {
            Label numArgsOne(env);
            Branch(Int64Equal(numArgs, IntPtr(1)), &numArgsOne, slowPath);
            Bind(&numArgsOne);
            {
                GateRef arg0 = GetCallArg0(numArgs);
                Label allEcmaObject(env);
                Label allStableJsArray(env);
                GateRef isThisEcmaObject = IsEcmaObject(thisValue);
                GateRef isArgEcmaObject = IsEcmaObject(arg0);
                Branch(BoolAnd(isThisEcmaObject, isArgEcmaObject), &allEcmaObject, slowPath);
                Bind(&allEcmaObject);
                {
                    GateRef isThisStableJSArray = IsStableJSArray(glue, thisValue);
                    GateRef isArgStableJSArray = IsStableJSArray(glue, arg0);
                    Branch(BoolAnd(isThisStableJSArray, isArgStableJSArray), &allStableJsArray, slowPath);
                    Bind(&allStableJsArray);
                    {
                        GateRef maxArrayIndex = Int64(TaggedArray::MAX_ARRAY_INDEX);
                        GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                        GateRef argLen = ZExtInt32ToInt64(GetArrayLength(arg0));
                        GateRef sumArrayLen = Int64Add(argLen, thisLen);
                        Label notOverFlow(env);
                        Branch(Int64GreaterThan(sumArrayLen, maxArrayIndex), slowPath, &notOverFlow);
                        Bind(&notOverFlow);
                        {
                            Label spreadable(env);
                            GateRef isSpreadable = IsConcatSpreadable(glue, thisValue);
                            GateRef argisSpreadable = IsConcatSpreadable(glue, arg0);
                            Branch(BoolAnd(isSpreadable, argisSpreadable), &spreadable, slowPath);
                            Bind(&spreadable);
                            {
                                Label setProperties(env);
                                GateRef glueGlobalEnvOffset =
                                    IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
                                GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
                                auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                    GlobalEnv::ARRAY_FUNCTION_INDEX);
                                GateRef intialHClass = Load(VariableType::JS_ANY(), arrayFunc,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                                NewObjectStubBuilder newBuilder(this);
                                newBuilder.SetParameters(glue, 0);
                                GateRef newArray = newBuilder.NewJSArrayWithSize(intialHClass, sumArrayLen);
                                Branch(TaggedIsException(newArray), exit, &setProperties);
                                Bind(&setProperties);
                                {
                                    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
                                    Store(VariableType::INT32(), glue, newArray, lengthOffset,
                                        TruncInt64ToInt32(sumArrayLen));
                                    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                        ConstantIndex::ARRAY_LENGTH_ACCESSOR);
                                    SetPropertyInlinedProps(glue, newArray, intialHClass, accessor,
                                        Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
                                    SetExtensibleToBitfield(glue, newArray, true);
                                    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
                                    DEFVARIABLE(j, VariableType::INT64(), Int64(0));
                                    DEFVARIABLE(k, VariableType::INT64(), Int64(0));
                                    Label loopHead(env);
                                    Label loopEnd(env);
                                    Label next(env);
                                    Label loopExit(env);
                                    Jump(&loopHead);
                                    LoopBegin(&loopHead);
                                    {
                                        Branch(Int64LessThan(*i, thisLen), &next, &loopExit);
                                        Bind(&next);
                                        GateRef ele = GetTaggedValueWithElementsKind(thisValue, *i);
                                        SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                        Jump(&loopEnd);
                                    }
                                    Bind(&loopEnd);
                                    i = Int64Add(*i, Int64(1));
                                    j = Int64Add(*j, Int64(1));
                                    LoopEnd(&loopHead);
                                    Bind(&loopExit);
                                    Label loopHead1(env);
                                    Label loopEnd1(env);
                                    Label next1(env);
                                    Label loopExit1(env);
                                    Jump(&loopHead1);
                                    LoopBegin(&loopHead1);
                                    {
                                        Branch(Int64LessThan(*k, argLen), &next1, &loopExit1);
                                        Bind(&next1);
                                        GateRef ele = GetTaggedValueWithElementsKind(arg0, *k);
                                        SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                        Jump(&loopEnd1);
                                    }
                                    Bind(&loopEnd1);
                                    k = Int64Add(*k, Int64(1));
                                    j = Int64Add(*j, Int64(1));
                                    LoopEnd(&loopHead1);
                                    Bind(&loopExit1);
                                    result->WriteVariable(newArray);
                                    Jump(exit);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsArrayStubBuilder::Filter(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    GateRef thisValueIsJSArray = IsJsArray(thisValue);
    GateRef protoIsJsArray = IsJsArray(StubBuilder::GetPrototype(glue, thisValue));
    Branch(BoolAnd(thisValueIsJSArray, protoIsJsArray), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);

    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Label argOHeapObject(env);
    Label callable(env);
    Label notOverFlow(env);
    Branch(TaggedIsHeapObject(callbackFnHandle), &argOHeapObject, slowPath);
    Bind(&argOHeapObject);
    Branch(IsCallable(callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef len = ZExtInt32ToInt64(GetArrayLength(thisValue));
    Branch(Int64GreaterThan(len, Int64(JSObject::MAX_GAP)), slowPath, &notOverFlow);
    Bind(&notOverFlow);

    GateRef argHandle = GetCallArg1(numArgs);
    GateRef newArray = NewArray(glue, len);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    GateRef newArrayEles = GetElementsArray(newArray);
    Label stableJSArray(env);
    Label notStableJSArray(env);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(toIndex, VariableType::INT64(), Int64(0));
    Branch(IsStableJSArray(glue, thisValue), &stableJSArray, slowPath);
    Bind(&stableJSArray);
    {
        DEFVARIABLE(thisArrLenVar, VariableType::INT64(), len);
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int64LessThan(*i, *thisArrLenVar), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(thisValue, *i);
            Label kValueIsHole(env);
            Label kValueNotHole(env);
            Label arrayValueIsHole(env);
            Label arrayValueNotHole(env);
            Label hasProperty(env);
            Branch(TaggedIsHole(*kValue), &arrayValueIsHole, &arrayValueNotHole);
            Bind(&arrayValueIsHole);
            {
                GateRef hasProp = CallRuntime(glue, RTSTUB_ID(HasProperty), { thisValue, IntToTaggedInt(*i) });
                Branch(TaggedIsTrue(hasProp), &hasProperty, &arrayValueNotHole);
                Bind(&hasProperty);
                Label hasException0(env);
                Label notHasException0(env);
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                Branch(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    Jump(&arrayValueNotHole);
                }
            }
            Bind(&arrayValueNotHole);
            Branch(TaggedIsHole(*kValue), &kValueIsHole, &kValueNotHole);
            Bind(&kValueNotHole);
            {
                GateRef key = Int64ToTaggedInt(*i);
                Label checkArray(env);
                GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                                                  Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                                                  { argHandle, *kValue, key, thisValue });
                Label find(env);
                Label hasException1(env);
                Label notHasException1(env);
                Branch(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException1);
                Branch(TaggedIsTrue(FastToBoolean(retValue)), &find, &checkArray);
                Bind(&find);
                {
                    SetValueWithElementsKind(glue, newArray, *kValue, *toIndex, Boolean(true),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    toIndex = Int64Add(*toIndex, Int64(1));
                    Jump(&checkArray);
                }
                Bind(&checkArray);
                {
                    Label lenChange(env);
                    GateRef tmpArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                    Branch(Int64LessThan(tmpArrLen, *thisArrLenVar), &lenChange, &kValueIsHole);
                    Bind(&lenChange);
                    {
                        thisArrLenVar = tmpArrLen;
                        Jump(&kValueIsHole);
                    }
                }
            }
            Bind(&kValueIsHole);
            i = Int64Add(*i, Int64(1));
            Branch(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&notStableJSArray);
    }
    Bind(&notStableJSArray);
    {
        Label finish(env);
        Label callRT(env);
        Branch(Int32LessThan(*i, len), &callRT, &finish);
        Bind(&callRT);
        {
            CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, newArrayEles, *toIndex});
            Store(VariableType::INT32(), glue, newArray, lengthOffset, TruncInt64ToInt32(*toIndex));
            GateRef ret = CallRuntime(glue, RTSTUB_ID(JSArrayFilterUnStable), { argHandle, thisValue,
                IntToTaggedInt(*i), IntToTaggedInt(len), IntToTaggedInt(*toIndex), newArray, callbackFnHandle });
            result->WriteVariable(ret);
            Jump(exit);
        }
        Bind(&finish);
        {
            result->WriteVariable(newArray);
            Label needTrim(env);
            Branch(Int64LessThan(*toIndex, len), &needTrim, exit);
            Bind(&needTrim);
            CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, newArrayEles, *toIndex});
            Store(VariableType::INT32(), glue, newArray, lengthOffset, TruncInt64ToInt32(*toIndex));
            Jump(exit);
        }
    }
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::ForEach([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    [[maybe_unused]] Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    Label isHeapObject(env);
    // Fast path if all the conditions below are satisfied:
    // (1) this is an empty array with constructor not reset (see ArraySpeciesCreate for details);
    // (2) callbackFn is callable (otherwise a TypeError shall be thrown in the slow path)
    JsArrayRequirements req;
    req.defaultConstructor = true;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    // Do nothing on fast path
    Branch(TaggedIsHeapObject(GetCallArg0(numArgs)), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsCallable(GetCallArg0(numArgs)), exit, slowPath);
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::IndexOf([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if: (1) this is an empty array; (2) fromIndex is missing
    JsArrayRequirements req;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    {
        Label atMostOneArg(env);
        Branch(Int32LessThanOrEqual(TruncPtrToInt32(numArgs), Int32(1)), &atMostOneArg, slowPath);
        // Returns -1 on fast path
        Bind(&atMostOneArg);
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::LastIndexOf([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if: (1) this is an empty array; (2) fromIndex is missing
    JsArrayRequirements req;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    {
        Label atMostOneArg(env);
        Branch(Int32LessThanOrEqual(TruncPtrToInt32(numArgs), Int32(1)), &atMostOneArg, slowPath);
        // Returns -1 on fast path
        Bind(&atMostOneArg);
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::Pop(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label stableJSArray(env);
    Label isDeufaltConstructor(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(HasConstructor(thisValue), slowPath, &isDeufaltConstructor);
    Bind(&isDeufaltConstructor);
    GateRef isThisEcmaObject = IsEcmaObject(thisValue);
    GateRef isThisStableJSArray = IsStableJSArray(glue, thisValue);
    Branch(BoolAnd(isThisEcmaObject, isThisStableJSArray), &stableJSArray, slowPath);
    Bind(&stableJSArray);
    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));

    Label notZeroLen(env);
    Branch(Int64Equal(thisLen, Int64(0)), exit, &notZeroLen);
    Bind(&notZeroLen);
    Label isJsCOWArray(env);
    Label getElements(env);
    Branch(IsJsCOWArray(thisValue), &isJsCOWArray, &getElements);
    Bind(&isJsCOWArray);
    {
        CallRuntime(glue, RTSTUB_ID(CheckAndCopyArray), { thisValue });
        Jump(&getElements);
    }
    Bind(&getElements);
    GateRef elements = GetElementsArray(thisValue);
    GateRef capacity = ZExtInt32ToInt64(GetLengthOfTaggedArray(elements));
    GateRef index = Int64Sub(thisLen, Int64(1));

    Label inRange(env);
    Label trimCheck(env);
    Label noTrimCheck(env);
    Label setNewLen(env);
    Label isHole(env);
    DEFVARIABLE(element, VariableType::JS_ANY(), Hole());
    Branch(Int64LessThan(index, capacity), &inRange, &trimCheck);
    Bind(&inRange);
    {
        element = GetTaggedValueWithElementsKind(thisValue, index);
        Jump(&isHole);
    }
    Bind(&isHole);
    Branch(TaggedIsHole(*element), &noTrimCheck, &trimCheck);
    Bind(&noTrimCheck);
    {
        Label hasException0(env);
        Label notHasException0(env);
        element = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(index), ProfileOperation());
        Branch(HasPendingException(glue), &hasException0, &notHasException0);
        Bind(&hasException0);
        {
            result->WriteVariable(Exception());
            Jump(exit);
        }
        Bind(&notHasException0);
        {
            Jump(&setNewLen);
        }
    }
    Bind(&trimCheck);
    // ShouldTrim check
    // (oldLength - newLength > MAX_END_UNUSED)
    Label noTrim(env);
    Label needTrim(env);
    GateRef unused = Int64Sub(capacity, index);
    Branch(Int64GreaterThan(unused, Int64(TaggedArray::MAX_END_UNUSED)), &needTrim, &noTrim);
    Bind(&needTrim);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, elements, index});
        Jump(&setNewLen);
    }
    Bind(&noTrim);
    {
        SetValueWithElementsKind(glue, thisValue, Hole(), index, Boolean(false),
                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
        Jump(&setNewLen);
    }
    Bind(&setNewLen);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, thisValue, lengthOffset, TruncInt64ToInt32(index));

    Label isNotHole(env);
    Branch(TaggedIsHole(*element), exit, &isNotHole);
    Bind(&isNotHole);
    {
        result->WriteVariable(*element);
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label noConstructor(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &noConstructor);
    Bind(&noConstructor);

    Label thisIsEmpty(env);
    Label thisNotEmpty(env);
    // Fast path if:
    // (1) this is an empty array with constructor not reset (see ArraySpeciesCreate for details);
    // (2) no arguments exist
    JsArrayRequirements req;
    req.defaultConstructor = true;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, &thisNotEmpty);
    Bind(&thisIsEmpty);
    {
        Label noArgs(env);
        GateRef numArgsAsInt32 = TruncPtrToInt32(numArgs);
        Branch(Int32Equal(numArgsAsInt32, Int32(0)), &noArgs, slowPath);
        // Creates a new empty array on fast path
        Bind(&noArgs);
        NewObjectStubBuilder newBuilder(this);
        result->WriteVariable(newBuilder.CreateEmptyArray(glue));
        Jump(exit);
    }
    Bind(&thisNotEmpty);
    {
        Label stableJSArray(env);
        Label arrayLenNotZero(env);

        GateRef isThisStableJSArray = IsStableJSArray(glue, thisValue);
        Branch(isThisStableJSArray, &stableJSArray, slowPath);
        Bind(&stableJSArray);

        GateRef msg0 = GetCallArg0(numArgs);
        GateRef msg1 = GetCallArg1(numArgs);
        GateRef thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
        Label msg0Int(env);
        Branch(TaggedIsInt(msg0), &msg0Int, slowPath);
        Bind(&msg0Int);
        DEFVARIABLE(start, VariableType::INT64(), Int64(0));
        DEFVARIABLE(end, VariableType::INT64(), thisArrLen);

        GateRef argStart = SExtInt32ToInt64(TaggedGetInt(msg0));
        Label arg0LessZero(env);
        Label arg0NotLessZero(env);
        Label startDone(env);
        Branch(Int64LessThan(argStart, Int64(0)), &arg0LessZero, &arg0NotLessZero);
        Bind(&arg0LessZero);
        {
            Label tempGreaterZero(env);
            Label tempNotGreaterZero(env);
            GateRef tempStart = Int64Add(argStart, thisArrLen);
            Branch(Int64GreaterThan(tempStart, Int64(0)), &tempGreaterZero, &tempNotGreaterZero);
            Bind(&tempGreaterZero);
            {
                start = tempStart;
                Jump(&startDone);
            }
            Bind(&tempNotGreaterZero);
            {
                Jump(&startDone);
            }
        }
        Bind(&arg0NotLessZero);
        {
            Label argLessLen(env);
            Label argNotLessLen(env);
            Branch(Int64LessThan(argStart, thisArrLen), &argLessLen, &argNotLessLen);
            Bind(&argLessLen);
            {
                start = argStart;
                Jump(&startDone);
            }
            Bind(&argNotLessLen);
            {
                start = thisArrLen;
                Jump(&startDone);
            }
        }
        Bind(&startDone);
        {
            Label endDone(env);
            Label msg1Def(env);
            Branch(TaggedIsUndefined(msg1), &endDone, &msg1Def);
            Bind(&msg1Def);
            {
            Label msg1Int(env);
            Branch(TaggedIsInt(msg1), &msg1Int, slowPath);
            Bind(&msg1Int);
            {
                GateRef argEnd = SExtInt32ToInt64(TaggedGetInt(msg1));
                Label arg1LessZero(env);
                Label arg1NotLessZero(env);
                Branch(Int64LessThan(argEnd, Int64(0)), &arg1LessZero, &arg1NotLessZero);
                Bind(&arg1LessZero);
                {
                    Label tempGreaterZero(env);
                    Label tempNotGreaterZero(env);
                    GateRef tempEnd = Int64Add(argEnd, thisArrLen);
                    Branch(Int64GreaterThan(tempEnd, Int64(0)), &tempGreaterZero, &tempNotGreaterZero);
                    Bind(&tempGreaterZero);
                    {
                        end = tempEnd;
                        Jump(&endDone);
                    }
                    Bind(&tempNotGreaterZero);
                    {
                        end = Int64(0);
                        Jump(&endDone);
                    }
                }
                Bind(&arg1NotLessZero);
                {
                    Label argLessLen(env);
                    Label argNotLessLen(env);
                    Branch(Int64LessThan(argEnd, thisArrLen), &argLessLen, &argNotLessLen);
                    Bind(&argLessLen);
                    {
                        end = argEnd;
                        Jump(&endDone);
                    }
                    Bind(&argNotLessLen);
                    {
                        end = thisArrLen;
                        Jump(&endDone);
                    }
                }
            }
        }
            Bind(&endDone);
            {
                DEFVARIABLE(count, VariableType::INT64(), Int64(0));
                GateRef tempCnt = Int64Sub(*end, *start);
                Label tempCntGreaterOrEqualZero(env);
                Label tempCntDone(env);
                Branch(Int64LessThan(tempCnt, Int64(0)), &tempCntDone, &tempCntGreaterOrEqualZero);
                Bind(&tempCntGreaterOrEqualZero);
                {
                    count = tempCnt;
                    Jump(&tempCntDone);
                }
                Bind(&tempCntDone);
                {
                    Label notOverFlow(env);
                    Branch(Int64GreaterThan(*count, Int64(JSObject::MAX_GAP)), slowPath, &notOverFlow);
                    Bind(&notOverFlow);
                    {
            GateRef newArray = NewArray(glue, *count);
            GateRef thisEles = GetElementsArray(thisValue);
            GateRef thisElesLen = ZExtInt32ToInt64(GetLengthOfTaggedArray(thisEles));

            Label inThisEles(env);
            Label outThisEles(env);
            Branch(Int64GreaterThan(thisElesLen, Int64Add(*start, *count)), &inThisEles, &outThisEles);
            Bind(&inThisEles);
            {
                DEFVARIABLE(idx, VariableType::INT64(), Int64(0));
                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int64LessThan(*idx, *count), &next, &loopExit);
                    Bind(&next);

                    GateRef ele = GetTaggedValueWithElementsKind(thisValue, Int64Add(*idx, *start));
                    SetValueWithElementsKind(glue, newArray, ele, *idx, Boolean(true),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                idx = Int64Add(*idx, Int64(1));
                LoopEnd(&loopHead);
                Bind(&loopExit);
                result->WriteVariable(newArray);
                Jump(exit);
            }
            Bind(&outThisEles);
            {
                DEFVARIABLE(idx, VariableType::INT64(), Int64(0));
                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int64LessThan(*idx, *count), &next, &loopExit);
                    Bind(&next);
                    GateRef index = Int64Add(*idx, *start);
                    DEFVARIABLE(ele, VariableType::JS_ANY(), Hole());

                    Label indexOutRange(env);
                    Label indexInRange(env);
                    Label setEle(env);
                    Branch(Int64GreaterThan(thisElesLen, index), &indexInRange, &indexOutRange);
                    Bind(&indexInRange);
                    {
                        ele = GetTaggedValueWithElementsKind(thisValue, index);
                        Jump(&setEle);
                    }
                    Bind(&indexOutRange);
                    {
                        ele = Hole();
                        Jump(&setEle);
                    }
                    Bind(&setEle);
                    SetValueWithElementsKind(glue, newArray, *ele, *idx, Boolean(true),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                idx = Int64Add(*idx, Int64(1));
                LoopEnd(&loopHead);
                Bind(&loopExit);
                result->WriteVariable(newArray);
                Jump(exit);
            }
        }
                }
            }
        }
    }
}

void BuiltinsArrayStubBuilder::Sort(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label defaultConstr(env);
    Label notCOWArray(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    Branch(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Branch(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    Label argUndefined(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    GateRef isUndefined = TaggedIsUndefined(callbackFnHandle);
    Branch(isUndefined, &argUndefined, slowPath);
    Bind(&argUndefined);
    {
        Label isStableJSArray(env);
        GateRef stableArray = IsStableJSArray(glue, thisValue);
        Branch(BoolAnd(stableArray, isUndefined), &isStableJSArray, slowPath);
        Bind(&isStableJSArray);
        {
            GateRef len = ZExtInt32ToInt64(GetArrayLength(thisValue));
            DEFVARIABLE(i, VariableType::INT64(), Int64(1));
            DEFVARIABLE(presentValue, VariableType::JS_ANY(), Undefined());
            DEFVARIABLE(middleValue, VariableType::JS_ANY(), Undefined());
            DEFVARIABLE(previousValue, VariableType::JS_ANY(), Undefined());
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Branch(Int64LessThan(*i, len), &next, &loopExit);
                Bind(&next);
                DEFVARIABLE(beginIndex, VariableType::INT64(), Int64(0));
                DEFVARIABLE(endIndex, VariableType::INT64(), *i);
                presentValue = GetTaggedValueWithElementsKind(thisValue, *i);
                Label loopHead1(env);
                Label loopEnd1(env);
                Label next1(env);
                Label loopExit1(env);
                Jump(&loopHead1);
                LoopBegin(&loopHead1);
                {
                    Branch(Int64LessThan(*beginIndex, *endIndex), &next1, &loopExit1);
                    Bind(&next1);
                    GateRef sum = Int64Add(*beginIndex, *endIndex);
                    GateRef middleIndex = Int64Div(sum, Int64(2)); // 2 : half
                    middleValue = GetTaggedValueWithElementsKind(thisValue, middleIndex);
                    Label isInt(env);
                    Branch(BoolAnd(TaggedIsInt(*middleValue), TaggedIsInt(*presentValue)), &isInt, slowPath);
                    Bind(&isInt);
                    {
                        GateRef compareResult =
                            CallNGCRuntime(glue, RTSTUB_ID(FastArraySort), {*middleValue, *presentValue});
                        Label less0(env);
                        Label greater0(env);
                        Branch(Int32LessThanOrEqual(compareResult, Int32(0)), &less0, &greater0);
                        Bind(&greater0);
                        {
                            endIndex = middleIndex;
                            Jump(&loopEnd1);
                        }
                        Bind(&less0);
                        {
                            beginIndex = middleIndex;
                            beginIndex = Int64Add(*beginIndex, Int64(1));
                            Jump(&loopEnd1);
                        }
                    }
                }
                Bind(&loopEnd1);
                LoopEnd(&loopHead1);
                Bind(&loopExit1);

                Label shouldCopy(env);
                GateRef isGreater0 = Int64GreaterThanOrEqual(*endIndex, Int64(0));
                GateRef lessI = Int64LessThan(*endIndex, *i);
                Branch(BoolAnd(isGreater0, lessI), &shouldCopy, &loopEnd);
                Bind(&shouldCopy);{
                    DEFVARIABLE(j, VariableType::INT64(), *i);
                    Label loopHead2(env);
                    Label loopEnd2(env);
                    Label next2(env);
                    Label loopExit2(env);
                    Jump(&loopHead2);
                    LoopBegin(&loopHead2);
                    {
                        Branch(Int64GreaterThan(*j, *endIndex), &next2, &loopExit2);
                        Bind(&next2);
                        previousValue = GetTaggedValueWithElementsKind(thisValue, Int64Sub(*j, Int64(1)));
                        SetValueWithElementsKind(glue, thisValue, *previousValue, *j, Boolean(false),
                                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                        Jump(&loopEnd2);
                        }
                    Bind(&loopEnd2);
                    j = Int64Sub(*j, Int64(1));
                    LoopEnd(&loopHead2);
                    Bind(&loopExit2);
                    SetValueWithElementsKind(glue, thisValue, *presentValue, *endIndex, Boolean(false),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
            }
            Bind(&loopEnd);
            i = Int64Add(*i, Int64(1));
            LoopEnd(&loopHead);
            Bind(&loopExit);
            result->WriteVariable(thisValue);
            Jump(exit);
        }
    }
}

void BuiltinsArrayStubBuilder::Reduce(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisLen, VariableType::INT32(), Int32(0));
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Label atLeastOneArg(env);
    Label callbackFnHandleHeapObject(env);
    Label callbackFnHandleCallable(env);
    Label noTypeError(env);

    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    thisLen = GetArrayLength(thisValue);
    Branch(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
    Bind(&atLeastOneArg);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Branch(TaggedIsHeapObject(callbackFnHandle), &callbackFnHandleHeapObject, slowPath);
    Bind(&callbackFnHandleHeapObject);
    Branch(IsCallable(callbackFnHandle), &callbackFnHandleCallable, slowPath);
    Bind(&callbackFnHandleCallable);
    GateRef thisLenIsZero = Int32Equal(*thisLen, Int32(0));
    GateRef numArgsLessThanTwo = Int64LessThan(numArgs, IntPtr(2));
    Branch(BoolAnd(thisLenIsZero, numArgsLessThanTwo), slowPath, &noTypeError);
    Bind(&noTypeError);
    {
        DEFVARIABLE(accumulator, VariableType::JS_ANY(), Undefined());
        DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    
        Label updateAccumulator(env);
        Label checkForStableJSArray(env);
    
        Branch(Int64Equal(numArgs, IntPtr(2)), &updateAccumulator, slowPath); // 2: provide initialValue param
        Bind(&updateAccumulator);
        {
            accumulator = GetCallArg1(numArgs);
            Jump(&checkForStableJSArray);
        }
        Bind(&checkForStableJSArray);
        {
            Label isStableJSArray(env);
            Label notStableJSArray(env);
            Branch(IsStableJSArray(glue, thisValue), &isStableJSArray, &notStableJSArray);
            Bind(&isStableJSArray);
            {
                GateRef argsLength = Int32(4); // 4: «accumulator, kValue, k, thisValue»
                NewObjectStubBuilder newBuilder(this);
                GateRef argList = newBuilder.NewTaggedArray(glue, argsLength);
                Label loopHead(env);
                Label next(env);
                Label loopEnd(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int32LessThan(*k, *thisLen), &next, &loopExit);
                    Bind(&next);
                    {
                        Label updateK(env);
                        Label notHole(env);
                        Label changeThisLen(env);
                        Label updateCallResult(env);
                        GateRef elements = GetElementsArray(thisValue);
                        GateRef kValue = GetTaggedValueWithElementsKind(thisValue, *k);
                        Branch(TaggedIsHole(kValue), &loopEnd, &notHole);
                        Bind(&notHole);
                        {
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(0), *accumulator);
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(1), kValue);
                            // 2 : parameter location
                            SetValueToTaggedArray(VariableType::INT32(), glue, argList, Int32(2), IntToTaggedInt(*k));
                            // 3 : parameter location
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(3), thisValue);
                            GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                            GateRef callResult = JSCallDispatch(glue, callbackFnHandle, argsLength, 0,
                                Circuit::NullGate(), JSCallMode::CALL_THIS_ARGV_WITH_RETURN,
                                {argsLength, argv, Undefined()});
                            Label hasException1(env);
                            Label notHasException1(env);
                            Branch(HasPendingException(glue), &hasException1, &notHasException1);
                            Bind(&hasException1);
                            {
                                result->WriteVariable(Exception());
                                Jump(exit);
                            }
                            Bind(&notHasException1);
                            GateRef newLen = GetLengthOfTaggedArray(elements);
                            Branch(Int32LessThan(newLen, *thisLen), &changeThisLen, &updateCallResult);
                            Bind(&changeThisLen);
                            {
                                thisLen = newLen;
                                Jump(&updateCallResult);
                            }
                            Bind(&updateCallResult);
                            {
                                accumulator = callResult;
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
                Bind(&loopEnd);
                {
                    k = Int32Add(*k, Int32(1));

                    Label isStableJSArray1(env);
                    Label notStableJSArray1(env);
                    Branch(IsStableJSArray(glue, thisValue), &isStableJSArray1, &notStableJSArray1);
                    Bind(&notStableJSArray1);
                    {
                        Jump(&loopExit);
                    }
                    Bind(&isStableJSArray1);
                    LoopEnd(&loopHead);
                }
                Bind(&loopExit);
                Jump(&notStableJSArray);
            }
            Bind(&notStableJSArray);
            {
                Label finish(env);
                Label callRT(env);
                Branch(Int32LessThan(*k, *thisLen), &callRT, &finish);
                Bind(&callRT);
                {
                    accumulator = CallRuntime(glue, RTSTUB_ID(JSArrayReduceUnStable), { thisValue, thisValue,
                        IntToTaggedInt(*k), IntToTaggedInt(*thisLen), *accumulator, callbackFnHandle });
                    Jump(&finish);
                }
                Bind(&finish);
                {
                    result->WriteVariable(*accumulator);
                    Jump(exit);
                }
            }
        }
    }
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::Reverse(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label defaultConstr(env);
    Label notCOWArray(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    Branch(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Branch(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(j, VariableType::INT64(),  Int64Sub(thisArrLen, Int64(1)));

    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label arrayValue(env);
        Label valueEqual(env);
        Branch(Int64LessThan(*i, *j), &next, &loopExit);
        Bind(&next);
        {
            GateRef lower = GetTaggedValueWithElementsKind(thisValue, *i);
            GateRef upper = GetTaggedValueWithElementsKind(thisValue, *j);
            SetValueWithElementsKind(glue, thisValue, upper, *i, Boolean(false),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            SetValueWithElementsKind(glue, thisValue, lower, *j, Boolean(false),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    i = Int64Add(*i, Int64(1));
    j = Int64Sub(*j, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    result->WriteVariable(thisValue);
    Jump(exit);
}

GateRef BuiltinsArrayStubBuilder::IsJsArrayWithLengthLimit(GateRef glue, GateRef object,
    uint32_t maxLength, JsArrayRequirements requirements)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label stabilityCheckPassed(env);
    Label defaultConstructorCheckPassed(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());

    Branch(TaggedIsHeapObject(object), &isHeapObject, &exit);
    Bind(&isHeapObject);
    Branch(IsJsArray(object), &isJsArray, &exit);
    Bind(&isJsArray);
    if (requirements.stable) {
        Branch(IsStableJSArray(glue, object), &stabilityCheckPassed, &exit);
    } else {
        Jump(&stabilityCheckPassed);
    }
    Bind(&stabilityCheckPassed);
    if (requirements.defaultConstructor) {
        // If HasConstructor bit is set to 1, then the constructor has been modified.
        Branch(HasConstructor(object), &exit, &defaultConstructorCheckPassed);
    } else {
        Jump(&defaultConstructorCheckPassed);
    }
    Bind(&defaultConstructorCheckPassed);
    result.WriteVariable(Int32UnsignedLessThanOrEqual(GetArrayLength(object), Int32(maxLength)));
    Jump(&exit);
    Bind(&exit);
    GateRef ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsArrayStubBuilder::Values(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    ConstantIndex iterClassIdx = ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX;
    GateRef iteratorHClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, iterClassIdx);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef prototype = GetGlobalEnvValue(VariableType::JS_POINTER(), glueGlobalEnv,
                                          GlobalEnv::ARRAY_ITERATOR_PROTOTYPE_INDEX);
    SetPrototypeToHClass(VariableType::JS_POINTER(), glue, iteratorHClass, prototype);
    GateRef iter = newBuilder.NewJSObject(glue, iteratorHClass);
    SetIteratedArrayOfArrayIterator(glue, iter, thisValue);
    SetNextIndexOfArrayIterator(glue, iter, Int32(0));
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::VALUE));
    SetBitFieldOfArrayIterator(glue, iter, kind);
    result->WriteVariable(iter);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::Find(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label defaultConstr(env);
    Label notCOWArray(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    Branch(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Branch(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Label arg0HeapObject(env);
    Branch(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    Label callable(env);
    Branch(IsCallable(callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef argHandle = GetCallArg1(numArgs);
    DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label hasException0(env);
        Label notHasException0(env);
        Branch(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
        Bind(&next);
        GateRef kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
        Branch(HasPendingException(glue), &hasException0, &notHasException0);
        Bind(&hasException0);
        {
            result->WriteVariable(Exception());
            Jump(exit);
        }
        Bind(&notHasException0);
        {
            GateRef key = Int64ToTaggedInt(*i);
            Label hasException(env);
            Label notHasException(env);
            GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN, { argHandle, kValue, key, thisValue });
            Branch(HasPendingException(glue), &hasException, &notHasException);
            Bind(&hasException);
            {
                result->WriteVariable(retValue);
                Jump(exit);
            }
            Bind(&notHasException);
            {
                Label find(env);
                Branch(TaggedIsTrue(FastToBoolean(retValue)), &find, &loopEnd);
                Bind(&find);
                {
                    result->WriteVariable(kValue);
                    Jump(exit);
                }
            }
        }
    }
    Bind(&loopEnd);
    thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    i = Int64Add(*i, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::FindIndex(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Label notCOWArray(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    Branch(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    Label arg0HeapObject(env);
    Label callable(env);
    Label stableJSArray(env);
    Label notStableJSArray(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Branch(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    Branch(IsCallable(callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    result->WriteVariable(IntToTaggedPtr(Int32(-1)));
    GateRef argHandle = GetCallArg1(numArgs);
    DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    Branch(IsStableJSArray(glue, thisValue), &stableJSArray, &notStableJSArray);
    Bind(&stableJSArray);
    {
        DEFVARIABLE(i, VariableType::INT64(), Int64(0));
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(thisValue, *i);
            Label isHole(env);
            Label notHole(env);
            Branch(TaggedIsHole(*kValue), &isHole, &notHole);
            Bind(&isHole);
            {
                Label hasException0(env);
                Label notHasException0(env);
                GateRef res = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                Branch(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    Label resIsHole(env);
                    Label resNotHole(env);
                    Branch(TaggedIsHole(res), &resIsHole, &resNotHole);
                    Bind(&resIsHole);
                    {
                        kValue = Undefined();
                        Jump(&notHole);
                    }
                    Bind(&resNotHole);{
                        kValue = res;
                        Jump(&notHole);
                    }
                }
            }
            Bind(&notHole);
            {
                GateRef key = IntToTaggedPtr(*i);
                Label hasException(env);
                Label notHasException(env);
                Label checkStable(env);
                GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                    Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                    { argHandle, *kValue, key, thisValue });
                Branch(TaggedIsException(retValue), &hasException, &notHasException);
                Bind(&hasException);
                {
                    result->WriteVariable(retValue);
                    Jump(exit);
                }
                Bind(&notHasException);
                {
                    Label find(env);
                    Branch(TaggedIsTrue(FastToBoolean(retValue)), &find, &checkStable);
                    Bind(&find);
                    {
                        result->WriteVariable(key);
                        Jump(exit);
                    }
                }
                Bind(&checkStable);
                i = Int64Add(*i, Int64(1));
                Branch(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
            }
        }
        Bind(&loopEnd);
        thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(exit);
    }
    Bind(&notStableJSArray);
    {
        DEFVARIABLE(j, VariableType::INT64(), Int64(0));
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
            Branch(Int64LessThan(*j, *thisArrLen), &next, &loopExit);
            Bind(&next);
            {
                Label hasException0(env);
                Label notHasException0(env);
                GateRef kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*j), ProfileOperation());
                Branch(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    GateRef key = IntToTaggedPtr(*j);
                    Label hasException(env);
                    Label notHasException(env);
                    GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS),
                        0, Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                        { argHandle, kValue, key, thisValue });
                    Branch(TaggedIsException(retValue), &hasException, &notHasException);
                    Bind(&hasException);
                    {
                        result->WriteVariable(retValue);
                        Jump(exit);
                    }
                    Bind(&notHasException);
                    {
                        Label find(env);
                        Branch(TaggedIsTrue(FastToBoolean(retValue)), &find, &loopEnd);
                        Bind(&find);
                        {
                            result->WriteVariable(key);
                            Jump(exit);
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
        j = Int64Add(*j, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::Push(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label setLength(env);
    Label smallArgs(env);
    Label checkSmallArgs(env);

    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);

    Branch(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);

    GateRef oldLength = GetArrayLength(thisValue);
    *result = IntToTaggedPtr(oldLength);

    Branch(Int32Equal(ChangeIntPtrToInt32(numArgs), Int32(0)), exit, &checkSmallArgs);
    Bind(&checkSmallArgs);
    // now unsupport more than 2 args
    Branch(Int32LessThanOrEqual(ChangeIntPtrToInt32(numArgs), Int32(2)), &smallArgs, slowPath);
    Bind(&smallArgs);
    GateRef newLength = Int32Add(oldLength, ChangeIntPtrToInt32(numArgs));

    DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(thisValue));
    GateRef capacity = GetLengthOfTaggedArray(*elements);
    Label grow(env);
    Label setValue(env);
    Branch(Int32GreaterThan(newLength, capacity), &grow, &setValue);
    Bind(&grow);
    {
        elements =
            CallRuntime(glue, RTSTUB_ID(JSObjectGrowElementsCapacity), { thisValue, IntToTaggedInt(newLength) });
        Jump(&setValue);
    }
    Bind(&setValue);
    {
        Label oneArg(env);
        Label twoArg(env);
        DEFVARIABLE(index, VariableType::INT32(), Int32(0));
        DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
        Branch(Int64Equal(numArgs, IntPtr(1)), &oneArg, &twoArg);  // 1 one arg
        Bind(&oneArg);
        {
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0));  // 0 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            Jump(&setLength);
        }
        Bind(&twoArg);
        {
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0));  // 0 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            value = GetCallArg1(numArgs);
            index = Int32Add(oldLength, Int32(1));  // 1 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            Jump(&setLength);
        }
    }
    Bind(&setLength);
    SetArrayLength(glue, thisValue, newLength);
    result->WriteVariable(IntToTaggedPtr(newLength));
    Jump(exit);
}

GateRef BuiltinsArrayStubBuilder::IsConcatSpreadable(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label isEcmaObj(env);
    Branch(IsEcmaObject(obj), &isEcmaObj, &exit);
    Bind(&isEcmaObj);
    {
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        GateRef isConcatsprKey =
            GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ISCONCAT_SYMBOL_INDEX);
        AccessObjectStubBuilder builder(this);
        GateRef spreadable =
            builder.LoadObjByValue(glue, obj, isConcatsprKey, Undefined(), Int32(0), ProfileOperation());
        Label isDefined(env);
        Label isUnDefined(env);
        Branch(TaggedIsUndefined(spreadable), &isUnDefined, &isDefined);
        Bind(&isUnDefined);
        {
            Label IsArray(env);
            Branch(IsJsArray(obj), &IsArray, &exit);
            Bind(&IsArray);
            result = True();
            Jump(&exit);
        }
        Bind(&isDefined);
        {
            result = TaggedIsTrue(spreadable);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef BuiltinsArrayStubBuilder::NewArray(GateRef glue, GateRef count)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());
    Label exit(env);
    Label setProperties(env);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef intialHClass = Load(VariableType::JS_ANY(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    result = newBuilder.NewJSArrayWithSize(intialHClass, count);
    Branch(TaggedIsException(*result), &exit, &setProperties);
    Bind(&setProperties);
    {
        GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
        Store(VariableType::INT32(), glue, *result, lengthOffset, TruncInt64ToInt32(count));
        GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
        SetPropertyInlinedProps(glue, *result, intialHClass, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
        SetExtensibleToBitfield(glue, *result, true);
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

void BuiltinsArrayStubBuilder::Includes(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isDictMode(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label notFound(env);
    Label thisLenNotZero(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(IsDictionaryMode(thisValue), &isDictMode, slowPath);
    Bind(&isDictMode);
    GateRef thisLen = GetArrayLength(thisValue);
    Branch(Int32Equal(thisLen, Int32(0)), &notFound, &thisLenNotZero);
    Bind(&thisLenNotZero);
    {
        DEFVARIABLE(fromIndex, VariableType::INT32(), Int32(0));
        Label getArgTwo(env);
        Label nextProcess(env);
        Branch(Int64Equal(numArgs, IntPtr(2)), &getArgTwo, &nextProcess); // 2: 2 parameters
        Bind(&getArgTwo);
        {
            Label secondArgIsInt(env);
            GateRef fromIndexTemp = GetCallArg1(numArgs);
            Branch(TaggedIsInt(fromIndexTemp), &secondArgIsInt, slowPath);
            Bind(&secondArgIsInt);
            fromIndex = GetInt32OfTInt(fromIndexTemp);
            Jump(&nextProcess);
        }
        Bind(&nextProcess);
        {
            Label atLeastOneArg(env);
            Label setBackZero(env);
            Label calculateFrom(env);
            Label nextCheck(env);
            Branch(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
            Bind(&atLeastOneArg);
            Branch(Int32GreaterThanOrEqual(*fromIndex, thisLen), &notFound, &nextCheck);
            Bind(&nextCheck);
            {
                GateRef negThisLen = Int32Sub(Int32(0), thisLen);
                Branch(Int32LessThan(*fromIndex, negThisLen), &setBackZero, &calculateFrom);
                Bind(&setBackZero);
                {
                    fromIndex = Int32(0);
                    Jump(&calculateFrom);
                }
                Bind(&calculateFrom);
                {
                    DEFVARIABLE(from, VariableType::INT32(), Int32(0));
                    Label fromIndexGreaterOrEqualZero(env);
                    Label fromIndexLessThanZero(env);
                    Label startLoop(env);
                    Branch(Int32GreaterThanOrEqual(*fromIndex, Int32(0)),
                        &fromIndexGreaterOrEqualZero, &fromIndexLessThanZero);
                    Bind(&fromIndexGreaterOrEqualZero);
                    {
                        from = *fromIndex;
                        Jump(&startLoop);
                    }
                    Bind(&fromIndexLessThanZero);
                    {
                        Label isLenFromIndex(env);
                        GateRef lenFromIndexSum = Int32Add(thisLen, *fromIndex);
                        Branch(Int32GreaterThanOrEqual(lenFromIndexSum, Int32(0)), &isLenFromIndex, &startLoop);
                        Bind(&isLenFromIndex);
                        {
                            from = lenFromIndexSum;
                            Jump(&startLoop);
                        }
                    }
                    Bind(&startLoop);
                    {
                        GateRef searchElement = GetCallArg0(numArgs);
                        Label loopHead(env);
                        Label loopEnd(env);
                        Label next(env);
                        Label loopExit(env);
                        Jump(&loopHead);
                        LoopBegin(&loopHead);
                        {
                            Branch(Int32LessThan(*from, thisLen), &next, &loopExit);
                            Bind(&next);
                            {
                                Label notHoleOrUndefValue(env);
                                Label valueFound(env);
                                GateRef value = GetTaggedValueWithElementsKind(thisValue, *from);
                                GateRef isHole = TaggedIsHole(value);
                                GateRef isUndef = TaggedIsUndefined(value);
                                Branch(BoolOr(isHole, isUndef), slowPath, &notHoleOrUndefValue);
                                Bind(&notHoleOrUndefValue);
                                GateRef valueEqual = StubBuilder::SameValueZero(glue, searchElement, value);
                                Branch(valueEqual, &valueFound, &loopEnd);
                                Bind(&valueFound);
                                {
                                    result->WriteVariable(TaggedTrue());
                                    Jump(exit);
                                }
                            }
                        }
                        Bind(&loopEnd);
                        from = Int32Add(*from, Int32(1));
                        LoopEnd(&loopHead);
                        Bind(&loopExit);
                        Jump(&notFound);
                    }
                }
            }
        }
    }
    Bind(&notFound);
    {
        result->WriteVariable(TaggedFalse());
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::From(GateRef glue, [[maybe_unused]] GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef item = GetCallArg0(numArgs);
    Label stringItem(env);
    Branch(TaggedIsString(item), &stringItem, slowPath);
    Bind(&stringItem);
    Label undefFn(env);
    GateRef fn = GetCallArg1(numArgs);
    Branch(TaggedIsUndefined(fn), &undefFn, slowPath);
    Bind(&undefFn);
    GateRef strLen = GetLengthFromString(item);
    Label lessStrLen(env);
    Branch(Int32LessThan(strLen, Int32(builtins::StringToListResultCache::MAX_STRING_LENGTH)), &lessStrLen, slowPath);
    Bind(&lessStrLen);
    GateRef cacheArray = CallNGCRuntime(glue, RTSTUB_ID(GetStringToListCacheArray), { glue });

    Label cacheDef(env);
    Branch(TaggedIsUndefined(cacheArray), slowPath, &cacheDef);
    Bind(&cacheDef);
    {
        GateRef hash = GetHashcodeFromString(glue, item);
        GateRef entry = Int32And(hash, Int32Sub(Int32(builtins::StringToListResultCache::CACHE_SIZE), Int32(1)));
        GateRef index = Int32Mul(entry, Int32(builtins::StringToListResultCache::ENTRY_SIZE));
        GateRef cacheStr = GetValueFromTaggedArray(cacheArray,
            Int32Add(index, Int32(builtins::StringToListResultCache::STRING_INDEX)));
        Label cacheStrDef(env);
        Branch(TaggedIsUndefined(cacheStr), slowPath, &cacheStrDef);
        Bind(&cacheStrDef);
        Label strEqual(env);
        Label strSlowEqual(env);
        // cache str is intern
        Branch(Equal(cacheStr, item), &strEqual, &strSlowEqual);
        Bind(&strSlowEqual);
        Branch(FastStringEqual(glue, cacheStr, item), &strEqual, slowPath);
        Bind(&strEqual);

        GateRef cacheResArray = GetValueFromTaggedArray(cacheArray,
            Int32Add(index, Int32(builtins::StringToListResultCache::ARRAY_INDEX)));
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
        GateRef intialHClass = Load(VariableType::JS_ANY(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        GateRef newArray = newBuilder.NewJSObject(glue, intialHClass);
        Store(VariableType::INT32(), glue, newArray, IntPtr(JSArray::LENGTH_OFFSET), strLen);
        GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
        SetPropertyInlinedProps(glue, newArray, intialHClass, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
        SetExtensibleToBitfield(glue, newArray, true);

        SetElementsArray(VariableType::JS_ANY(), glue, newArray, cacheResArray);
        *result = newArray;
        Jump(exit);
    }
}

GateRef BuiltinsArrayStubBuilder::CreateSpliceDeletedArray(GateRef glue, GateRef thisValue, GateRef actualDeleteCount,
    GateRef arrayCls, GateRef start)
{
    auto env = GetEnvironment();
    Label subentry(env);
    Label exit(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::BOOL(), False());

    // new delete array
    DEFVARIABLE(srcElements, VariableType::JS_ANY(), GetElementsArray(thisValue));
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    GateRef newArray = newBuilder.NewJSArrayWithSize(arrayCls, actualDeleteCount);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, newArray, lengthOffset, TruncInt64ToInt32(actualDeleteCount));
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    SetPropertyInlinedProps(glue, newArray, arrayCls, accessor, Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
    SetExtensibleToBitfield(glue, newArray, true);
    result = newArray;

    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*i, actualDeleteCount), &next, &loopExit);
        Bind(&next);
        Label setHole(env);
        Label setSrc(env);
        Branch(Int32GreaterThanOrEqual(Int32Add(*i, start),
            GetLengthOfTaggedArray(*srcElements)), &setHole, &setSrc);
        Bind(&setHole);
        {
            SetValueWithElementsKind(glue, newArray, Hole(), *i, Boolean(true),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            Jump(&loopEnd);
        }
        Bind(&setSrc);
        {
            GateRef val = GetTaggedValueWithElementsKind(thisValue, Int32Add(start, *i));
            SetValueWithElementsKind(glue, newArray, val, *i, Boolean(true),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(&exit);

    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

void BuiltinsArrayStubBuilder::Splice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label defaultConstr(env);
    Branch(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    Branch(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Branch(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    Branch(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Label notCOWArray(env);
    Branch(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef intialHClass = Load(VariableType::JS_ANY(), arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    Label equalCls(env);
    GateRef arrayCls = LoadHClass(thisValue);
    Branch(Equal(intialHClass, arrayCls), &equalCls, slowPath);
    Bind(&equalCls);

    GateRef arrayLen = GetArrayLength(thisValue);
    Label lessThreeArg(env);

    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    DEFVARIABLE(insertCount, VariableType::INT32(), Int32(0));
    DEFVARIABLE(actualDeleteCount, VariableType::INT32(), Int32(0));
    GateRef argc = ChangeIntPtrToInt32(numArgs);
    Branch(Int32LessThanOrEqual(argc, Int32(3)), &lessThreeArg, slowPath); // 3 : three arg
    Bind(&lessThreeArg);
    {
        Label checkOverflow(env);
        Label greaterZero(env);
        Label greaterOne(env);
        Label checkGreaterOne(env);
        Label notOverflow(env);
        Branch(Int32GreaterThan(argc, Int32(0)), &greaterZero, &checkGreaterOne);
        Bind(&greaterZero);
        GateRef taggedStart = GetCallArg0(numArgs);
        Label taggedStartInt(env);
        Branch(TaggedIsInt(taggedStart), &taggedStartInt, slowPath);
        Bind(&taggedStartInt);
        {
            GateRef intStart = GetInt32OfTInt(taggedStart);
            start = CalArrayRelativePos(intStart, arrayLen);
        }
        actualDeleteCount = Int32Sub(arrayLen, *start);
        Jump(&checkGreaterOne);
        Bind(&checkGreaterOne);
        Branch(Int32GreaterThan(argc, Int32(1)), &greaterOne, &checkOverflow);
        Bind(&greaterOne);
        insertCount = Int32Sub(argc, Int32(2)); // 2 :  two args
        GateRef argDeleteCount = GetCallArg1(numArgs);
        Label argDeleteCountInt(env);
        Branch(TaggedIsInt(argDeleteCount), &argDeleteCountInt, slowPath);
        Bind(&argDeleteCountInt);
        DEFVARIABLE(deleteCount, VariableType::INT32(), TaggedGetInt(argDeleteCount));
        Label deleteCountLessZero(env);
        Label calActualDeleteCount(env);
        Branch(Int32LessThan(*deleteCount, Int32(0)), &deleteCountLessZero, &calActualDeleteCount);
        Bind(&deleteCountLessZero);
        deleteCount = Int32(0);
        Jump(&calActualDeleteCount);
        Bind(&calActualDeleteCount);
        actualDeleteCount = *deleteCount;
        Label lessArrayLen(env);
        Branch(Int32LessThan(Int32Sub(arrayLen, *start), *deleteCount), &lessArrayLen, &checkOverflow);
        Bind(&lessArrayLen);
        actualDeleteCount = Int32Sub(arrayLen, *start);
        Jump(&checkOverflow);
        Bind(&checkOverflow);
        Branch(Int64GreaterThan(Int64Sub(Int64Add(ZExtInt32ToInt64(arrayLen), ZExtInt32ToInt64(*insertCount)),
            ZExtInt32ToInt64(*actualDeleteCount)), Int64(base::MAX_SAFE_INTEGER)), slowPath, &notOverflow);
        Bind(&notOverflow);

        *result = CreateSpliceDeletedArray(glue, thisValue, *actualDeleteCount, intialHClass, *start);

        // insert Val
        DEFVARIABLE(srcElements, VariableType::JS_ANY(), GetElementsArray(thisValue));
        GateRef oldCapacity = GetLengthOfTaggedArray(*srcElements);
        GateRef newCapacity = Int32Add(Int32Sub(arrayLen, *actualDeleteCount), *insertCount);
        Label grow(env);
        Label copy(env);
        Branch(Int32GreaterThan(newCapacity, oldCapacity), &grow, &copy);
        Bind(&grow);
        {
            srcElements =
                CallRuntime(glue, RTSTUB_ID(JSObjectGrowElementsCapacity), { thisValue, IntToTaggedInt(newCapacity) });
            Jump(&copy);
        }
        Bind(&copy);
        GateRef srcElementsLen = GetLengthOfTaggedArray(*srcElements);
        Label insertLessDelete(env);
        Label insertGreaterDelete(env);
        Label insertCountVal(env);
        Label setArrayLen(env);
        Label trimCheck(env);
        Branch(Int32LessThan(*insertCount, *actualDeleteCount), &insertLessDelete, &insertGreaterDelete);
        Bind(&insertLessDelete);
        {
            {
                DEFVARIABLE(i, VariableType::INT32(), *start);
                DEFVARIABLE(ele, VariableType::JS_ANY(), Hole());

                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int32LessThan(*i, Int32Sub(arrayLen, *actualDeleteCount)), &next, &loopExit);
                    Bind(&next);
                    ele = Hole();
                    Label getSrcEle(env);
                    Label setEle(env);
                    Branch(Int32LessThan(Int32Add(*i, *actualDeleteCount), srcElementsLen), &getSrcEle, &setEle);
                    Bind(&getSrcEle);
                    {
                        ele = GetTaggedValueWithElementsKind(thisValue, Int32Add(*i, *actualDeleteCount));
                        Jump(&setEle);
                    }
                    Bind(&setEle);
                    {
                        Label setIndexLessLen(env);
                        Branch(Int32LessThan(Int32Add(*i, *insertCount), srcElementsLen), &setIndexLessLen, &loopEnd);
                        Bind(&setIndexLessLen);
                        {
                            SetValueWithElementsKind(glue, thisValue, *ele, Int32Add(*i, *insertCount), Boolean(true),
                                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                            Jump(&loopEnd);
                        }
                    }
                }
                Bind(&loopEnd);
                i = Int32Add(*i, Int32(1));
                LoopEnd(&loopHead);
                Bind(&loopExit);
                Jump(&trimCheck);
            }

            Label trim(env);
            Label noTrim(env);
            Bind(&trimCheck);
            Branch(BoolAnd(Int32GreaterThan(oldCapacity, newCapacity),
                Int32GreaterThan(Int32Sub(newCapacity, oldCapacity),
                Int32(TaggedArray::MAX_END_UNUSED))), &trim, &noTrim);
            Bind(&trim);
            {
                CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, *srcElements, ZExtInt32ToInt64(newCapacity)});
                Jump(&insertCountVal);
            }
            Bind(&noTrim);
            {
                DEFVARIABLE(idx, VariableType::INT32(), newCapacity);
                Label loopHead1(env);
                Label loopEnd1(env);
                Label next1(env);
                Label loopExit1(env);
                Jump(&loopHead1);
                LoopBegin(&loopHead1);
                {
                    Branch(Int32LessThan(*idx, arrayLen), &next1, &loopExit1);
                    Bind(&next1);

                    Label setHole(env);
                    Branch(Int32LessThan(*idx, srcElementsLen), &setHole, &loopEnd1);
                    Bind(&setHole);
                    {
                        SetValueWithElementsKind(glue, thisValue, Hole(), *idx, Boolean(true),
                                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                        Jump(&loopEnd1);
                    }
                }
                Bind(&loopEnd1);
                idx = Int32Add(*idx, Int32(1));
                LoopEnd(&loopHead1);
                Bind(&loopExit1);
                Jump(&insertCountVal);
            }
            Bind(&insertGreaterDelete);
            {
                DEFVARIABLE(j, VariableType::INT32(), Int32Sub(arrayLen, *actualDeleteCount));
                DEFVARIABLE(ele, VariableType::JS_ANY(), Hole());
                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Branch(Int32GreaterThan(*j, *start), &next, &loopExit);
                    Bind(&next);
                    ele = GetTaggedValueWithElementsKind(thisValue, Int32Sub(Int32Add(*j, *actualDeleteCount),
                                                                             Int32(1)));
                    Label setEle(env);
                    Label isHole(env);
                    Branch(TaggedIsHole(*ele), &isHole, &setEle);
                    Bind(&isHole);
                    {
                        ele = Undefined();
                        Jump(&setEle);
                    }
                    Bind(&setEle);
                    SetValueWithElementsKind(glue, thisValue, *ele, Int32Sub(Int32Add(*j, *insertCount), Int32(1)),
                                             Boolean(true), Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                j = Int32Sub(*j, Int32(1));
                LoopEnd(&loopHead);
                Bind(&loopExit);
                Jump(&insertCountVal);
            }
            Bind(&insertCountVal);
            {
                Label threeArgs(env);
                Branch(Int32Equal(ChangeIntPtrToInt32(numArgs), Int32(3)), &threeArgs, &setArrayLen); // 3 : three arg
                Bind(&threeArgs);
                {
                    GateRef e = GetCallArg2(numArgs);
                    SetValueWithElementsKind(glue, thisValue, e, *start, Boolean(true),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&setArrayLen);
                }
            }
            Bind(&setArrayLen);
            SetArrayLength(glue, thisValue, newCapacity);
            Jump(exit);
        }
    }
}
}  // namespace panda::ecmascript::kungfu
