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
#include "ecmascript/base/config.h"
#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"

#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/platform/dfx_hisys_event.h"

namespace panda::ecmascript::kungfu {

void BuiltinsArrayStubBuilder::ElementsKindHclassCompare(GateRef glue, GateRef arrayCls,
    Label *matchCls, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isGeneric(env);
    GateRef elementsKind = GetElementsKindFromHClass(arrayCls);
    GateRef notGeneric = NotEqual(elementsKind, Int32(Elements::ToUint(ElementsKind::GENERIC)));
    BRANCH(notGeneric, matchCls, &isGeneric);
    Bind(&isGeneric);
    {
        GateRef globalEnv = GetCurrentGlobalEnv();
        GateRef intialHClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                 static_cast<size_t>(GlobalEnvField::ELEMENT_HOLE_TAGGED_HCLASS_INDEX));
        BRANCH(Equal(intialHClass, arrayCls), matchCls, slowPath);
    }
}

void BuiltinsArrayStubBuilder::With(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    WithOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Unshift(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    UnshiftOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Shift(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    ShiftOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    ConcatOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Filter(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isprototypeJsArray(env);
    Label defaultConstr(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    GateRef prototype = StubBuilder::GetPrototype(glue, thisValue);
    GateRef protoIsJSArray =
        LogicAndBuilder(env).And(TaggedIsHeapObject(prototype)).And(IsJsArray(glue, prototype)).Done();
    BRANCH(protoIsJSArray, &isprototypeJsArray, slowPath);
    Bind(&isprototypeJsArray);
    // need check constructor, "Filter" should use ArraySpeciesCreate
    BRANCH(HasConstructor(glue, thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);

    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Label argOHeapObject(env);
    Label callable(env);
    Label notOverFlow(env);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &argOHeapObject, slowPath);
    Bind(&argOHeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef len = ZExtInt32ToInt64(GetArrayLength(thisValue));
    Label isEmptyArray(env);
    Label notEmptyArray(env);
    BRANCH(Int64Equal(len, Int64(0)), &isEmptyArray, &notEmptyArray);
    Bind(&isEmptyArray);
    {
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        result->WriteVariable(newBuilder.CreateEmptyArray(glue));
        Jump(exit);
    }
    Bind(&notEmptyArray);
    BRANCH(Int64GreaterThan(len, Int64(JSObject::MAX_GAP)), slowPath, &notOverFlow);
    Bind(&notOverFlow);

    GateRef argHandle = GetCallArg1(numArgs);
    GateRef kind = GetElementsKindFromHClass(LoadHClass(glue, thisValue));
    GateRef newHClass =  GetElementsKindHClass(glue, kind);
    GateRef newArray = NewArrayWithHClass(glue, newHClass, TruncInt64ToInt32(len));
    GateRef newArrayEles = GetElementsArray(glue, newArray);
    Label stableJSArray(env);
    Label notStableJSArray(env);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(toIndex, VariableType::INT64(), Int64(0));
    BRANCH(IsStableJSArray(glue, thisValue), &stableJSArray, slowPath);
    Bind(&stableJSArray);
    {
        DEFVARIABLE(thisArrLenVar, VariableType::INT64(), len);
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label enabledMutantArray(env);
        Label disabledMutantArray(env);
        BRANCH(IsEnableMutantArray(glue), &enabledMutantArray, &disabledMutantArray);
        Bind(&enabledMutantArray);
        {
            Label kValueIsHole(env);
            Label kValueNotHole(env);
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int64LessThan(*i, *thisArrLenVar), &next, &loopExit);
                Bind(&next);
                kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
                BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &kValueNotHole);
                Bind(&kValueNotHole);
                {
                    GateRef key = Int64ToTaggedInt(*i);
                    Label checkArray(env);
                    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                    callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                    CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS),
                        0, nullptr, Circuit::NullGate(), callArgs);
                    GateRef retValue = callBuilder.JSCallDispatch();
                    Label find(env);
                    Label hasException1(env);
                    Label notHasException1(env);
                    BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                    Bind(&hasException1);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                    Bind(&notHasException1);
                    BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &checkArray);
                    Bind(&find);
                    {
                        SetValueWithElementsKind(glue, newArray, *kValue, *toIndex, Boolean(true),
                                                 Int32(Elements::ToUint(ElementsKind::NONE)));
                        toIndex = Int64Add(*toIndex, Int64(1));
                        Jump(&checkArray);
                    }
                    Bind(&checkArray);
                    {
                        Label lenChange(env);
                        GateRef tmpArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                        BRANCH(Int64LessThan(tmpArrLen, *thisArrLenVar), &lenChange, &kValueIsHole);
                        Bind(&lenChange);
                        {
                            thisArrLenVar = tmpArrLen;
                            Jump(&kValueIsHole);
                        }
                    }
                }
                Bind(&kValueIsHole);
                i = Int64Add(*i, Int64(1));
                BRANCH(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
            }
            Bind(&loopEnd);
            LoopEnd(&loopHead);
            Bind(&loopExit);
            Jump(&notStableJSArray);
        }
        Bind(&disabledMutantArray);
        {
            Label kValueIsHole(env);
            Label kValueNotHole(env);
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int64LessThan(*i, *thisArrLenVar), &next, &loopExit);
                Bind(&next);
                GateRef elements = GetElementsArray(glue, thisValue);
                kValue = GetValueFromTaggedArray(glue, elements, *i);
                BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &kValueNotHole);
                Bind(&kValueNotHole);
                {
                    GateRef key = Int64ToTaggedInt(*i);
                    Label checkArray(env);
                    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                    callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                    CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                        nullptr, Circuit::NullGate(), callArgs);
                    GateRef retValue = callBuilder.JSCallDispatch();
                    Label find(env);
                    Label hasException1(env);
                    Label notHasException1(env);
                    BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                    Bind(&hasException1);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                    Bind(&notHasException1);
                    BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &checkArray);
                    Bind(&find);
                    {
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, newArrayEles, *toIndex, *kValue);
                        toIndex = Int64Add(*toIndex, Int64(1));
                        Jump(&checkArray);
                    }
                    Bind(&checkArray);
                    {
                        Label lenChange(env);
                        GateRef tmpArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                        BRANCH(Int64LessThan(tmpArrLen, *thisArrLenVar), &lenChange, &kValueIsHole);
                        Bind(&lenChange);
                        {
                            thisArrLenVar = tmpArrLen;
                            Jump(&kValueIsHole);
                        }
                    }
                }
                Bind(&kValueIsHole);
                i = Int64Add(*i, Int64(1));
                BRANCH(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
            }
            Bind(&loopEnd);
            LoopEnd(&loopHead);
            Bind(&loopExit);
            Jump(&notStableJSArray);
        }
    }
    Bind(&notStableJSArray);
    {
        Label finish(env);
        Label callRT(env);
        BRANCH(Int64LessThan(*i, len), &callRT, &finish);
        Bind(&callRT);
        {
            CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, newArrayEles, *toIndex});
            Store(VariableType::INT32(), glue, newArray, IntPtr(JSArray::LENGTH_OFFSET), TruncInt64ToInt32(*toIndex));
            GateRef ret = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(JSArrayFilterUnStable),
                { argHandle, thisValue, IntToTaggedInt(*i), IntToTaggedInt(len),
                    IntToTaggedInt(*toIndex), newArray, callbackFnHandle });
            result->WriteVariable(ret);
            Jump(exit);
        }
        Bind(&finish);
        {
            result->WriteVariable(newArray);
            Label needTrim(env);
            BRANCH(Int64LessThan(*toIndex, len), &needTrim, exit);
            Bind(&needTrim);
            CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, newArrayEles, *toIndex});
            Store(VariableType::INT32(), glue, newArray, IntPtr(JSArray::LENGTH_OFFSET), TruncInt64ToInt32(*toIndex));
            Jump(exit);
        }
    }
}

void BuiltinsArrayStubBuilder::Map(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isprototypeJsArray(env);
    Label defaultConstr(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    GateRef prototype = StubBuilder::GetPrototype(glue, thisValue);
    GateRef protoIsJSArray =
        LogicAndBuilder(env).And(TaggedIsHeapObject(prototype)).And(IsJsArray(glue, prototype)).Done();
    BRANCH(protoIsJSArray, &isprototypeJsArray, slowPath);
    Bind(&isprototypeJsArray);
    // need check constructor, "Map" should use ArraySpeciesCreate
    BRANCH(HasConstructor(glue, thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);

    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Label argOHeapObject(env);
    Label callable(env);
    Label notOverFlow(env);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &argOHeapObject, slowPath);
    Bind(&argOHeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef len = ZExtInt32ToInt64(GetArrayLength(thisValue));
    Label isEmptyArray(env);
    Label notEmptyArray(env);
    BRANCH(Int64Equal(len, Int64(0)), &isEmptyArray, &notEmptyArray);
    Bind(&isEmptyArray);
    {
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        result->WriteVariable(newBuilder.CreateEmptyArray(glue));
        Jump(exit);
    }
    Bind(&notEmptyArray);
    BRANCH(Int64GreaterThan(len, Int64(JSObject::MAX_GAP)), slowPath, &notOverFlow);
    Bind(&notOverFlow);

    GateRef argHandle = GetCallArg1(numArgs);
    GateRef newArray = NewArray(glue, len);
    Label stableJSArray(env);
    Label notStableJSArray(env);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    BRANCH(IsStableJSArray(glue, thisValue), &stableJSArray, slowPath);
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
            BRANCH(Int64LessThan(*i, *thisArrLenVar), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            Label kValueIsHole(env);
            Label kValueNotHole(env);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &kValueNotHole);
            Bind(&kValueNotHole);
            {
                GateRef key = Int64ToTaggedInt(*i);
                Label checkArray(env);
                JSCallArgs callArgs = (JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = {argHandle, *kValue, key, thisValue};
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                Label hasException1(env);
                Label notHasException1(env);
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException1);
                SetValueWithElementsKind(glue, newArray, retValue, *i, Boolean(true),
                                         Int32(Elements::ToUint(ElementsKind::NONE)));
                Label lenChange(env);
                GateRef tmpArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                BRANCH(Int64LessThan(tmpArrLen, *thisArrLenVar), &lenChange, &kValueIsHole);
                Bind(&lenChange);
                {
                    thisArrLenVar = tmpArrLen;
                    Jump(&kValueIsHole);
                }
            }
            Bind(&kValueIsHole);
            i = Int64Add(*i, Int64(1));
            BRANCH(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&notStableJSArray);
    }
    Bind(&notStableJSArray);
    {
        GateRef ret = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(JSArrayMapUnStable),
            { argHandle, thisValue, IntToTaggedInt(*i), IntToTaggedInt(len), newArray, callbackFnHandle });
        result->WriteVariable(ret);
        Jump(exit);
    }
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::ForEach([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    [[maybe_unused]] Variable *result, Label *exit, Label *slowPath)
{
    ForEachOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::ArrayIteratorNext(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(iterValue, VariableType::JS_ANY(), Undefined());

    Label thisIsArrayIterator(env);
    Label arrayNotUndefined(env);
    Label isStableJsArray(env);
    Label NotStableJsArray(env);
    Label isTypedArray(env);
    Label isArrayLikeObject(env);
    Label throwException(env);
    Label checkNeedCreateEntry(env);
    Label iterDone(env);
    Label createIterResult(env);

    GateRef array = Circuit::NullGate();
    GateRef index = Circuit::NullGate();

    BRANCH(TaggedIsArrayIterator(glue, thisValue), &thisIsArrayIterator, slowPath);
    Bind(&thisIsArrayIterator);
    {
        array = Load(VariableType::JS_POINTER(), glue, thisValue, IntPtr(JSArrayIterator::ITERATED_ARRAY_OFFSET));
        BRANCH(TaggedIsUndefined(array), &iterDone, &arrayNotUndefined);
    }
    Bind(&arrayNotUndefined);
    {
        index = LoadPrimitive(VariableType::INT32(), thisValue, IntPtr(JSArrayIterator::NEXT_INDEX_OFFSET));
        BRANCH(IsStableJSArray(glue, array), &isStableJsArray, &NotStableJsArray);
    }
    Bind(&isStableJsArray);
    {
        Label indexIsValid(env);
        Label kindIsNotKey(env);
        Label eleIsHole(env);
        Label hasProperty(env);
        GateRef length = GetArrayLength(array);
        BRANCH(Int32UnsignedLessThan(index, length), &indexIsValid, &iterDone);
        Bind(&indexIsValid);
        {
            IncreaseArrayIteratorIndex(glue, thisValue, index);
            iterValue = IntToTaggedPtr(index);
            BRANCH(Int32Equal(GetArrayIterationKind(thisValue), Int32(static_cast<int32_t>(IterationKind::KEY))),
                   &createIterResult, &kindIsNotKey);
            Bind(&kindIsNotKey);
            {
                Label resultIsHole(env);
                iterValue = GetTaggedValueWithElementsKind(glue, array, index);
                BRANCH(TaggedIsHole(*iterValue), &resultIsHole, &checkNeedCreateEntry);
                Bind(&resultIsHole);
                {
                    iterValue = Undefined();
                    Jump(&checkNeedCreateEntry);
                }
            }
        }
    }
    Bind(&NotStableJsArray);
    BRANCH(IsTypedArray(glue, array), &isTypedArray, &isArrayLikeObject);
    Bind(&isTypedArray);
    {
        Label indexIsValid(env);
        Label kindIsNotKey(env);
        BuiltinsTypedArrayStubBuilder typedArrayBuilder(this, GetCurrentGlobalEnv());
        GateRef length = typedArrayBuilder.GetArrayLength(array);
        BRANCH(Int32UnsignedLessThan(index, length), &indexIsValid, &iterDone);
        Bind(&indexIsValid);
        {
            IncreaseArrayIteratorIndex(glue, thisValue, index);
            iterValue = IntToTaggedPtr(index);
            BRANCH(Int32Equal(GetArrayIterationKind(thisValue), Int32(static_cast<int32_t>(IterationKind::KEY))),
                   &createIterResult, &kindIsNotKey);
            Bind(&kindIsNotKey);
            {
                iterValue = typedArrayBuilder.FastGetPropertyByIndex(glue, array, index,
                                                                     GetObjectType(LoadHClass(glue, array)));
                BRANCH(HasPendingException(glue), &throwException, &checkNeedCreateEntry);
            }
        }
    }
    Bind(&isArrayLikeObject);
    {
        Label indexIsValid(env);
        Label kindIsNotKey(env);
        Label propertyToLength(env);
        Label noPendingException(env);
        GateRef rawLength = Circuit::NullGate();
        GateRef lengthString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::LENGTH_STRING_INDEX);
        GateRef value = FastGetPropertyByName(glue, array, lengthString, ProfileOperation());
        BRANCH(HasPendingException(glue), &throwException, &propertyToLength);
        Bind(&propertyToLength);
        {
            rawLength = ToLength(glue, value);
            BRANCH(HasPendingException(glue), &throwException, &noPendingException);
        }
        Bind(&noPendingException);
        {
            GateRef length = GetInt32OfTNumber(rawLength);
            BRANCH(Int32UnsignedLessThan(index, length), &indexIsValid, &iterDone);
            Bind(&indexIsValid);
            {
                IncreaseArrayIteratorIndex(glue, thisValue, index);
                iterValue = IntToTaggedPtr(index);
                BRANCH(Int32Equal(GetArrayIterationKind(thisValue), Int32(static_cast<int32_t>(IterationKind::KEY))),
                       &createIterResult, &kindIsNotKey);
                Bind(&kindIsNotKey);
                {
                    iterValue = FastGetPropertyByIndex(glue, array, index, ProfileOperation());
                    BRANCH(HasPendingException(glue), &throwException, &checkNeedCreateEntry);
                }
            }
        }
    }
    Bind(&throwException);
    {
        result->WriteVariable(Exception());
        Jump(exit);
    }
    Bind(&checkNeedCreateEntry);
    {
        Label kindIsEntry(env);
        BRANCH(Int32Equal(GetArrayIterationKind(thisValue), Int32(static_cast<int32_t>(IterationKind::KEY_AND_VALUE))),
               &kindIsEntry, &createIterResult);
        Bind(&kindIsEntry);
        {
            Label afterCreate(env);
            NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
            GateRef elements = newBuilder.NewTaggedArray(glue, Int32(2)); // 2: length of array
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(0), IntToTaggedPtr(index)); // 0: key
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(1), *iterValue); // 1: value
            iterValue = newBuilder.CreateArrayFromList(glue, elements,
                Int32(Elements::ToUint(ElementsKind::TAGGED)));
            Jump(&createIterResult);
        }
    }
    Bind(&createIterResult);
    {
        Label afterCreate(env);
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        newBuilder.CreateJSIteratorResult(glue, &res, *iterValue, TaggedFalse(), &afterCreate);
        Bind(&afterCreate);
        result->WriteVariable(*res);
        Jump(exit);
    }
    Bind(&iterDone);
    {
        SetIteratedArrayOfArrayIterator(glue, thisValue, Undefined());
        Label afterCreate(env);
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        newBuilder.CreateJSIteratorResult(glue, &res, Undefined(), TaggedTrue(), &afterCreate);
        Bind(&afterCreate);
        result->WriteVariable(*res);
        Jump(exit);
    }
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::IndexOf([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = false;
    return IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::LastIndexOf([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = true;
    return IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
}

void BuiltinsArrayStubBuilder::Pop(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    PopOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    SliceOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Sort(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    SortAfterArgs(glue, thisValue, callbackFnHandle, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::SortAfterArgs(GateRef glue, GateRef thisValue,
    GateRef callbackFnHandle, Variable *result, Label *exit, Label *slowPath, GateRef hir)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    Label argUndefined(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Sort" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    BRANCH(TaggedIsUndefined(callbackFnHandle), &argUndefined, slowPath);
    Bind(&argUndefined);
    result->WriteVariable(DoSort(glue, thisValue, false, result, exit, slowPath, hir));
    Jump(exit);
}

void BuiltinsArrayStubBuilder::ToSorted(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isStability(env);
    Label notCOWArray(env);
    Label argUndefined(env);
    // don't check constructor, "Sort" always use ArrayCreate to create array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsUndefined(callbackFnHandle), &argUndefined, slowPath);
    Bind(&argUndefined);

    GateRef thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    GateRef receiver = NewArray(glue, thisArrLen);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int64LessThan(*i, thisArrLen), &next, &loopExit);
        Bind(&next);
        {
            GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            SetValueWithElementsKind(glue, receiver, ele, *i, Boolean(true),
                Int32(Elements::ToUint(ElementsKind::NONE)));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    i = Int64Add(*i, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    result->WriteVariable(DoSort(glue, receiver, true, result, exit, slowPath));
    Jump(exit);
}

GateRef BuiltinsArrayStubBuilder::DoSort(GateRef glue, GateRef receiver, bool isToSorted,
    Variable *result, Label *exit, Label *slowPath, GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit1(env);
    Label fastPath(env);
    Label slowPath1(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    if (isToSorted) {
        res = DoSortOptimised(glue, receiver, Boolean(true), result, exit, slowPath, hir);
        Jump(&exit1);
    } else {
        BRANCH(IsEnableMutantArray(glue), &slowPath1, &fastPath);
        Bind(&slowPath1);
        {
            res = DoSortOptimised(glue, receiver, Boolean(false), result, exit, slowPath, hir);
            Jump(&exit1);
        }
        Bind(&fastPath);
        {
            res = DoSortOptimisedFast(glue, receiver, result, exit, slowPath, hir);
            Jump(&exit1);
        }
    }
    Bind(&exit1);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

void BuiltinsArrayStubBuilder::Reduce(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label atLeastOneArg(env);
    Label callbackFnHandleHeapObject(env);
    Label callbackFnHandleCallable(env);
    Label noTypeError(env);
    Label writeResult(env);

    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Reduce" won't create new array.
    BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
    Bind(&atLeastOneArg);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &callbackFnHandleHeapObject, slowPath);
    Bind(&callbackFnHandleHeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callbackFnHandleCallable, slowPath);
    Bind(&callbackFnHandleCallable);
    GateRef thisLen = GetArrayLength(thisValue);
    GateRef thisLenIsZero = Int32Equal(thisLen, Int32(0));
    GateRef numArgsLessThanTwo = Int64LessThan(numArgs, IntPtr(2));
    BRANCH(BitAnd(thisLenIsZero, numArgsLessThanTwo), slowPath, &noTypeError);
    Bind(&noTypeError);
    {
        DEFVARIABLE(accumulator, VariableType::JS_ANY(), Undefined());
        DEFVARIABLE(k, VariableType::INT32(), Int32(0));

        Label updateAccumulator(env);
        Label checkForStableJSArray(env);

        BRANCH(Int64Equal(numArgs, IntPtr(2)), &updateAccumulator, slowPath); // 2: provide initialValue param
        Bind(&updateAccumulator);
        {
            accumulator = GetCallArg1(numArgs);
            Jump(&checkForStableJSArray);
        }
        Bind(&checkForStableJSArray);
        {
            Label isStableJSArray(env);
            Label notStableJSArray(env);
            BRANCH(IsStableJSArray(glue, thisValue), &isStableJSArray, &notStableJSArray);
            Bind(&isStableJSArray);
            {
                GateRef argsLength = Int32(4); // 4: «accumulator, kValue, k, thisValue»
                NewObjectStubBuilder newBuilder(this);
                GateRef argList = newBuilder.NewTaggedArray(glue, argsLength);
                Label loopHead(env);
                Label next(env);
                Label doCall(env);
                Label loopEnd(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH_LIKELY(Int32UnsignedLessThan(*k, thisLen), &next, &loopExit);
                    Bind(&next);
                    // thisObj.length may change and needs to be rechecked.
                    // If the thisObj is stableArray and k >= current length,
                    // thisObj will have no chance to change and we can directly exit the entire loop.
                    BRANCH_LIKELY(Int32UnsignedLessThan(*k, GetArrayLength(thisValue)), &doCall, &writeResult);
                    Bind(&doCall);
                    {
                        Label updateK(env);
                        Label notHole(env);
                        Label changeThisLen(env);
                        Label updateCallResult(env);
                        GateRef kValue = GetTaggedValueWithElementsKind(glue, thisValue, *k);
                        BRANCH(TaggedIsHole(kValue), &loopEnd, &notHole);
                        Bind(&notHole);
                        {
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(0), *accumulator);
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(1), kValue);
                            // 2 : parameter location
                            SetValueToTaggedArray(VariableType::INT32(), glue, argList, Int32(2), IntToTaggedInt(*k));
                            // 3 : parameter location
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(3), thisValue);
                            GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                            callArgs.callThisArgvWithReturnArgs = { argsLength, argv, Undefined() };
                            CallStubBuilder callBuilder(this, glue, callbackFnHandle, argsLength, 0, nullptr,
                                Circuit::NullGate(), callArgs);
                            GateRef callResult = callBuilder.JSCallDispatch();
                            Label hasException(env);
                            Label notHasException(env);
                            BRANCH(HasPendingException(glue), &hasException, &notHasException);
                            Bind(&hasException);
                            {
                                result->WriteVariable(Exception());
                                Jump(exit);
                            }
                            Bind(&notHasException);
                            accumulator = callResult;
                            Jump(&loopEnd);
                        }
                    }
                }
                Bind(&loopEnd);
                {
                    k = Int32Add(*k, Int32(1));

                    Label isStableJSArray1(env);
                    Label notStableJSArray1(env);
                    BRANCH(IsStableJSArray(glue, thisValue), &isStableJSArray1, &notStableJSArray1);
                    Bind(&notStableJSArray1);
                    {
                        Jump(&loopExit);
                    }
                    Bind(&isStableJSArray1);
                    LoopEndWithCheckSafePoint(&loopHead, env, glue);
                }
                Bind(&loopExit);
                Jump(&notStableJSArray);
            }
            Bind(&notStableJSArray);
            {
                Label callRT(env);
                BRANCH(Int32UnsignedLessThan(*k, thisLen), &callRT, &writeResult);
                Bind(&callRT);
                {
                    accumulator = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(JSArrayReduceUnStable), { thisValue, thisValue, IntToTaggedInt(*k),
                            IntToTaggedInt(thisLen), *accumulator, callbackFnHandle });
                    Jump(&writeResult);
                }
            }
        }
        Bind(&writeResult);
        result->WriteVariable(*accumulator);
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::Reverse(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isStability(env);
    Label notCOWArray(env);
    // don't check constructor, "Reverse" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef thisLen = GetArrayLength(thisValue);
    GateRef thisArrLen = ZExtInt32ToInt64(thisLen);
    Label useReversBarrier(env);
    Label noReverseBarrier(env);
    // 10 : length < 10 reverse item by item
    BRANCH(Int32LessThan(thisLen, Int32(10)), &noReverseBarrier, &useReversBarrier);
    Bind(&useReversBarrier);
    ReverseOptimised(glue, thisValue, thisLen, result, exit);
    Bind(&noReverseBarrier);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(j, VariableType::INT64(),  Int64Sub(thisArrLen, Int64(1)));

    GateRef hclass = LoadHClass(glue, thisValue);
    GateRef kind = GetElementsKindFromHClass(hclass);
    Label isInt(env);
    Label isNotInt(env);
    Label notFastKind(env);
    GateRef isMutantArrayEnabled = IsEnableMutantArray(glue);
    GateRef checkIntKind = LogicAndBuilder(env)
        .And(isMutantArrayEnabled)
        .And(Int32GreaterThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::INT))))
        .And(Int32LessThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::HOLE_INT))))
        .Done();
    BRANCH(checkIntKind, &isInt, &isNotInt);
    Bind(&isInt);
    {
        FastReverse(glue, thisValue, thisArrLen, ElementsKind::INT, result, exit);
    }
    Bind(&isNotInt);
    {
        Label isNumber(env);
        Label isNotNumber(env);
        GateRef checkNumberKind = LogicAndBuilder(env)
            .And(isMutantArrayEnabled)
            .And(Int32GreaterThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::NUMBER))))
            .And(Int32LessThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER))))
            .Done();
        BRANCH(checkNumberKind, &isNumber, &isNotNumber);
        Bind(&isNumber);
        {
            FastReverse(glue, thisValue, thisArrLen, ElementsKind::NUMBER, result, exit);
        }
        Bind(&isNotNumber);
        {
            FastReverse(glue, thisValue, thisArrLen, ElementsKind::TAGGED, result, exit);
        }
    }
}

void BuiltinsArrayStubBuilder::FastReverse(GateRef glue, GateRef thisValue, GateRef len,
    ElementsKind kind, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(j, VariableType::INT64(),  Int64Sub(len, Int64(1)));
    GateRef elements = GetElementsArray(glue, thisValue);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label arrayValue(env);
        Label valueEqual(env);
        BRANCH(Int64LessThan(*i, *j), &next, &loopExit);
        Bind(&next);
        {
            if (kind == ElementsKind::INT || kind == ElementsKind::NUMBER) {
                GateRef lower = GetValueFromMutantTaggedArray(elements, *i);
                GateRef upper = GetValueFromMutantTaggedArray(elements, *j);
                FastSetValueWithElementsKind(glue, thisValue, elements, upper, *i, kind);
                FastSetValueWithElementsKind(glue, thisValue, elements, lower, *j, kind);
                Jump(&loopEnd);
            } else {
                GateRef lower = GetValueFromTaggedArray(glue, elements, *i);
                GateRef upper = GetValueFromTaggedArray(glue, elements, *j);
                FastSetValueWithElementsKind(glue, thisValue, elements, upper, *i, kind);
                FastSetValueWithElementsKind(glue, thisValue, elements, lower, *j, kind);
                Jump(&loopEnd);
            }
        }
    }
    Bind(&loopEnd);
    i = Int64Add(*i, Int64(1));
    j = Int64Sub(*j, Int64(1));
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&loopExit);
    result->WriteVariable(thisValue);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::ToReversed(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
                                          Variable* result, Label* exit, Label* slowPath)
{
    ToReversedOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

GateRef BuiltinsArrayStubBuilder::DoReverse(GateRef glue, GateRef thisValue, GateRef receiver,
    GateRef receiverState, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(j, VariableType::INT64(), Int64Sub(ZExtInt32ToInt64(GetArrayLength(thisValue)), Int64(1)));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        DEFVARIABLE(lower, VariableType::JS_ANY(), Hole());
        DEFVARIABLE(upper, VariableType::JS_ANY(), Hole());
        Label lowerValueIsHole(env);
        Label afterGettingLower(env);
        Label lowerHasProperty(env);
        Label lowerHasException0(env);
        Label upperValueIsHole(env);
        Label afterGettingUpper(env);
        Label upperHasProperty(env);
        Label upperHasException0(env);
        Label receiverIsNew(env);
        Label receiverIsOrigin(env);
        Label lowerIsHole(env);
        Label lowerIsNotHole(env);
        Label dealWithUpper(env);
        Label upperIsHole(env);
        Label upperIsNotHole(env);
        BRANCH(Int64LessThanOrEqual(*i, *j), &next, &loopExit);
        Bind(&next);
        {
            lower = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH(TaggedIsHole(*lower), &lowerValueIsHole, &afterGettingLower);
            Bind(&lowerValueIsHole);
            {
                GateRef lowerHasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    { thisValue, IntToTaggedInt(*i) });
                BRANCH(TaggedIsTrue(lowerHasProp), &lowerHasProperty, &afterGettingLower);
                Bind(&lowerHasProperty);
                {
                    lower = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                    BRANCH(HasPendingException(glue), &lowerHasException0, &afterGettingLower);
                    Bind(&lowerHasException0);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                }
            }
            Bind(&afterGettingLower);
            {
                upper = GetTaggedValueWithElementsKind(glue, thisValue, *j);
                BRANCH(TaggedIsHole(*upper), &upperValueIsHole, &afterGettingUpper);
                Bind(&upperValueIsHole);
                {
                    GateRef upperHasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(HasProperty), { thisValue, IntToTaggedInt(*j) });
                    BRANCH(TaggedIsTrue(upperHasProp), &upperHasProperty, &afterGettingUpper);
                    Bind(&upperHasProperty);
                    {
                        upper = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*j), ProfileOperation());
                        BRANCH(HasPendingException(glue), &upperHasException0, &afterGettingUpper);
                    }
                    Bind(&upperHasException0);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                }
                Bind(&afterGettingUpper);
                {
                    BRANCH(receiverState, &receiverIsNew, &receiverIsOrigin);
                    Bind(&receiverIsNew);
                    {
                        BRANCH(TaggedIsHole(*lower), &lowerIsHole, &lowerIsNotHole);
                        Bind(&lowerIsHole);
                        {
                            SetValueWithElementsKind(glue, receiver, Undefined(), *j, Boolean(true),
                                Int32(Elements::ToUint(ElementsKind::NONE)));
                            Jump(&dealWithUpper);
                        }
                        Bind(&lowerIsNotHole);
                        {
                            SetValueWithElementsKind(glue, receiver, *lower, *j, Boolean(true),
                                Int32(Elements::ToUint(ElementsKind::NONE)));
                            Jump(&dealWithUpper);
                        }
                        Bind(&dealWithUpper);
                        {
                            BRANCH(TaggedIsHole(*upper), &upperIsHole, &upperIsNotHole);
                            Bind(&upperIsHole);
                            {
                                SetValueWithElementsKind(glue, receiver, Undefined(), *i, Boolean(true),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&upperIsNotHole);
                            {
                                SetValueWithElementsKind(glue, receiver, *upper, *i, Boolean(true),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                        }
                    }
                    Bind(&receiverIsOrigin);
                    {
                        SetValueWithElementsKind(glue, receiver, *upper, *i, Boolean(false),
                            Int32(Elements::ToUint(ElementsKind::NONE)));
                        SetValueWithElementsKind(glue, receiver, *lower, *j, Boolean(false),
                            Int32(Elements::ToUint(ElementsKind::NONE)));
                        Jump(&loopEnd);
                    }
                }
            }
        }
    }
    Bind(&loopEnd);
    i = Int64Add(*i, Int64(1));
    j = Int64Sub(*j, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    env->SubCfgExit();
    return receiver;
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

    BRANCH(TaggedIsHeapObject(object), &isHeapObject, &exit);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, object), &isJsArray, &exit);
    Bind(&isJsArray);
    if (requirements.stable) {
        BRANCH(IsStableJSArray(glue, object), &stabilityCheckPassed, &exit);
    } else {
        Jump(&stabilityCheckPassed);
    }
    Bind(&stabilityCheckPassed);
    if (requirements.defaultConstructor) {
        // If HasConstructor bit is set to 1, then the constructor has been modified.
        BRANCH(HasConstructor(glue, object), &exit, &defaultConstructorCheckPassed);
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
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Values" won't create new array.
    ConstantIndex iterClassIdx = ConstantIndex::JS_ARRAY_ITERATOR_CLASS_INDEX;
    GateRef iteratorHClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, iterClassIdx);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef prototype = GetGlobalEnvValue(VariableType::JS_POINTER(), glue, globalEnv,
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
    FindOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::FindIndex(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    FindIndexOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Push(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isStability(env);
    Label setLength(env);
    Label smallArgs(env);
    Label checkSmallArgs(env);
    Label isLengthWritable(env);

    BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH_LIKELY(IsArrayLengthWritable(glue, thisValue), &isLengthWritable, slowPath);
    Bind(&isLengthWritable);
    GateRef oldLength = GetArrayLength(thisValue);
    *result = IntToTaggedPtr(oldLength);
    BRANCH_UNLIKELY(Int32Equal(ChangeIntPtrToInt32(numArgs), Int32(0)), exit, &checkSmallArgs);
    Bind(&checkSmallArgs);
    // now unsupport more than 2 args
    BRANCH_LIKELY(Int32LessThanOrEqual(ChangeIntPtrToInt32(numArgs), Int32(2)), &smallArgs, slowPath);
    Bind(&smallArgs);

    GateRef newLength = Int32Add(oldLength, ChangeIntPtrToInt32(numArgs));

    DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
    GateRef capacity = GetLengthOfTaggedArray(*elements);
    Label grow(env);
    Label setValue(env);
    BRANCH_UNLIKELY(Int32GreaterThan(newLength, capacity), &grow, &setValue);
    Bind(&grow);
    {
        elements = CallCommonStub(glue, CommonStubCSigns::GrowElementsCapacity,
            {glue, thisValue, GetCurrentGlobalEnv(), newLength});
        Jump(&setValue);
    }
    Bind(&setValue);
    {
        Label oneArg(env);
        Label twoArg(env);
        DEFVARIABLE(index, VariableType::INT32(), Int32(0));
        DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
        BRANCH_LIKELY(Int64Equal(numArgs, IntPtr(1)), &oneArg, &twoArg); // 1 one arg
        Bind(&oneArg);
        {
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0)); // 0 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            Jump(&setLength);
        }
        Bind(&twoArg);
        {
            DEFVARIABLE(index2, VariableType::INT32(), Int32(0));
            DEFVARIABLE(value2, VariableType::JS_ANY(), Undefined());
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0)); // 0 slot index
            value2 = GetCallArg1(numArgs);
            index2 = Int32Add(oldLength, Int32(1)); // 1 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            SetValueWithElementsKind(glue, thisValue, *value2, *index2, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
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
    BRANCH(IsEcmaObject(glue, obj), &isEcmaObj, &exit);
    Bind(&isEcmaObj);
    {
        GateRef globalEnv = GetCurrentGlobalEnv();
        GateRef isConcatsprKey =
            GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ISCONCAT_SYMBOL_INDEX);
        AccessObjectStubBuilder builder(this, globalEnv);
        GateRef spreadable =
            builder.LoadObjByValue(glue, obj, isConcatsprKey, Undefined(), Int32(0), ProfileOperation());
        Label isDefined(env);
        Label isUnDefined(env);
        BRANCH(TaggedIsUndefined(spreadable), &isUnDefined, &isDefined);
        Bind(&isUnDefined);
        {
            Label IsArray(env);
            BRANCH(IsJsArray(glue, obj), &IsArray, &exit);
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

void BuiltinsArrayStubBuilder::InitializeArray(GateRef glue, GateRef count, Variable* result)
{
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, result->ReadVariable(), lengthOffset, TruncInt64ToInt32(count));
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    Store(VariableType::JS_ANY(), glue, result->ReadVariable(),
          IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)), accessor);
    SetExtensibleToBitfield(glue, result->ReadVariable(), true);
}

GateRef BuiltinsArrayStubBuilder::NewArray(GateRef glue, GateRef count)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());
    Label exit(env);
    Label setProperties(env);
    GateRef globalEnv = GetCurrentGlobalEnv();
    auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    GateRef intialHClass = Load(VariableType::JS_ANY(), glue, arrayFunc, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(this, globalEnv);
    newBuilder.SetParameters(glue, 0);
    result = newBuilder.NewJSArrayWithSize(intialHClass, count);
    BRANCH(TaggedIsException(*result), &exit, &setProperties);
    Bind(&setProperties);
    {
        InitializeArray(glue, count, &result);
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
    IndexOfOptions options;
    options.compType = ComparisonType::SAME_VALUE_ZERO;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_OR_NOT;
    options.holeAsUndefined = true;
    options.reversedOrder = false;

    IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
}

void BuiltinsArrayStubBuilder::From(GateRef glue, [[maybe_unused]] GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef item = GetCallArg0(numArgs);
    Label stringItem(env);
    GateRef isUtf8String = LogicAndBuilder(env).And(TaggedIsString(glue, item)).And(IsUtf8String(item)).Done();
    BRANCH(isUtf8String, &stringItem, slowPath);
    Bind(&stringItem);
    Label undefFn(env);
    GateRef fn = GetCallArg1(numArgs);
    BRANCH(TaggedIsUndefined(fn), &undefFn, slowPath);
    Bind(&undefFn);
    GateRef strLen = GetLengthFromString(item);
    Label lessStrLen(env);
    BRANCH(Int32LessThan(strLen, Int32(builtins::StringToListResultCache::MAX_STRING_LENGTH)), &lessStrLen, slowPath);
    Bind(&lessStrLen);
    GateRef globalEnv = GetCurrentGlobalEnv();
    auto cacheArray =
        GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::STRING_TO_LIST_RESULT_CACHE_INDEX);

    Label cacheDef(env);
    BRANCH(TaggedIsUndefined(cacheArray), slowPath, &cacheDef);
    Bind(&cacheDef);
    {
        GateRef hash = GetHashcodeFromString(glue, item);
        GateRef entry = Int32And(hash, Int32Sub(Int32(builtins::StringToListResultCache::CACHE_SIZE), Int32(1)));
        GateRef index = Int32Mul(entry, Int32(builtins::StringToListResultCache::ENTRY_SIZE));
        GateRef cacheStr = GetValueFromTaggedArray(glue, cacheArray,
            Int32Add(index, Int32(builtins::StringToListResultCache::STRING_INDEX)));
        Label cacheStrDef(env);
        BRANCH(TaggedIsUndefined(cacheStr), slowPath, &cacheStrDef);
        Bind(&cacheStrDef);
        Label strEqual(env);
        Label strSlowEqual(env);
        // cache str is intern
        BRANCH(Equal(cacheStr, item), &strEqual, &strSlowEqual);
        Bind(&strSlowEqual);
        BRANCH(FastStringEqual(glue, cacheStr, item), &strEqual, slowPath);
        Bind(&strEqual);

        GateRef cacheResArray = GetValueFromTaggedArray(glue, cacheArray,
            Int32Add(index, Int32(builtins::StringToListResultCache::ARRAY_INDEX)));
        auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
        GateRef intialHClass = Load(VariableType::JS_ANY(), glue, arrayFunc,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        GateRef newArray = newBuilder.NewJSObject(glue, intialHClass);
        Store(VariableType::INT32(), glue, newArray, IntPtr(JSArray::LENGTH_OFFSET), strLen);
        GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
        Store(VariableType::JS_ANY(), glue, newArray,
              IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)), accessor);
        SetExtensibleToBitfield(glue, newArray, true);

        SetElementsArray(VariableType::JS_ANY(), glue, newArray, cacheResArray);
        *result = newArray;
        Jump(exit);
    }
}

GateRef BuiltinsArrayStubBuilder::CreateSpliceDeletedArray(GateRef glue, GateRef thisValue, GateRef actualDeleteCount,
                                                           GateRef start)
{
    auto env = GetEnvironment();
    Label subentry(env);
    Label exit(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    // new delete array
    DEFVARIABLE(srcElements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
    GateRef newArray = NewArray(glue, actualDeleteCount);
    result = newArray;

    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32LessThan(*i, actualDeleteCount), &next, &loopExit);
        Bind(&next);
        Label setHole(env);
        Label setSrc(env);
        BRANCH(Int32GreaterThanOrEqual(Int32Add(*i, start),
            GetLengthOfTaggedArray(*srcElements)), &setHole, &setSrc);
        Bind(&setHole);
        {
            SetValueWithElementsKind(glue, newArray, Hole(), *i, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            Jump(&loopEnd);
        }
        Bind(&setSrc);
        {
            GateRef val = GetTaggedValueWithElementsKind(glue, thisValue, Int32Add(start, *i));
            SetValueWithElementsKind(glue, newArray, val, *i, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&loopExit);
    Jump(&exit);

    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

void BuiltinsArrayStubBuilder::Fill(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    FillOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Splice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isStability(env);
    Label defaultConstr(env);
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    // need check constructor, "Splice" should use ArraySpeciesCreate
    BRANCH(HasConstructor(glue, thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    Label notCOWArray(env);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef arrayLen = GetArrayLength(thisValue);
    Label lessThreeArg(env);

    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    DEFVARIABLE(insertCount, VariableType::INT32(), Int32(0));
    DEFVARIABLE(actualDeleteCount, VariableType::INT32(), Int32(0));
    GateRef argc = ChangeIntPtrToInt32(numArgs);
    BRANCH(Int32LessThanOrEqual(argc, Int32(3)), &lessThreeArg, slowPath); // 3 : three arg
    Bind(&lessThreeArg);
    {
        Label checkOverflow(env);
        Label greaterZero(env);
        Label greaterOne(env);
        Label checkGreaterOne(env);
        Label notOverflow(env);
        BRANCH(Int32GreaterThan(argc, Int32(0)), &greaterZero, &checkGreaterOne);
        Bind(&greaterZero);
        GateRef taggedStart = GetCallArg0(numArgs);
        Label taggedStartInt(env);
        BRANCH(TaggedIsInt(taggedStart), &taggedStartInt, slowPath);
        Bind(&taggedStartInt);
        {
            GateRef intStart = GetInt32OfTInt(taggedStart);
            start = CalArrayRelativePos(intStart, arrayLen);
        }
        actualDeleteCount = Int32Sub(arrayLen, *start);
        Jump(&checkGreaterOne);
        Bind(&checkGreaterOne);
        BRANCH(Int32GreaterThan(argc, Int32(1)), &greaterOne, &checkOverflow);
        Bind(&greaterOne);
        insertCount = Int32Sub(argc, Int32(2)); // 2 :  two args
        GateRef argDeleteCount = GetCallArg1(numArgs);
        Label argDeleteCountInt(env);
        BRANCH(TaggedIsInt(argDeleteCount), &argDeleteCountInt, slowPath);
        Bind(&argDeleteCountInt);
        DEFVARIABLE(deleteCount, VariableType::INT32(), TaggedGetInt(argDeleteCount));
        Label deleteCountLessZero(env);
        Label calActualDeleteCount(env);
        BRANCH(Int32LessThan(*deleteCount, Int32(0)), &deleteCountLessZero, &calActualDeleteCount);
        Bind(&deleteCountLessZero);
        deleteCount = Int32(0);
        Jump(&calActualDeleteCount);
        Bind(&calActualDeleteCount);
        actualDeleteCount = *deleteCount;
        Label lessArrayLen(env);
        BRANCH(Int32LessThan(Int32Sub(arrayLen, *start), *deleteCount), &lessArrayLen, &checkOverflow);
        Bind(&lessArrayLen);
        actualDeleteCount = Int32Sub(arrayLen, *start);
        Jump(&checkOverflow);
        Bind(&checkOverflow);
        BRANCH(Int64GreaterThan(Int64Sub(Int64Add(ZExtInt32ToInt64(arrayLen), ZExtInt32ToInt64(*insertCount)),
            ZExtInt32ToInt64(*actualDeleteCount)), Int64(base::MAX_SAFE_INTEGER)), slowPath, &notOverflow);
        Bind(&notOverflow);
        *result = CreateSpliceDeletedArray(glue, thisValue, *actualDeleteCount, *start);
        // insert Val
        DEFVARIABLE(srcElements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
        GateRef oldCapacity = GetLengthOfTaggedArray(*srcElements);
        GateRef newCapacity = Int32Add(Int32Sub(arrayLen, *actualDeleteCount), *insertCount);
        Label grow(env);
        Label copy(env);
        BRANCH(Int32GreaterThan(newCapacity, oldCapacity), &grow, &copy);
        Bind(&grow);
        {
            srcElements = GrowElementsCapacity(glue, thisValue, newCapacity);
            Jump(&copy);
        }
        Bind(&copy);
        GateRef srcElementsLen = GetLengthOfTaggedArray(*srcElements);
        Label insertLessDelete(env);
        Label insertGreaterDelete(env);
        Label insertCountVal(env);
        Label setArrayLen(env);
        Label trimCheck(env);
        BRANCH(Int32LessThan(*insertCount, *actualDeleteCount), &insertLessDelete, &insertGreaterDelete);
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
                    BRANCH(Int32LessThan(*i, Int32Sub(arrayLen, *actualDeleteCount)), &next, &loopExit);
                    Bind(&next);
                    ele = Hole();
                    Label getSrcEle(env);
                    Label setEle(env);
                    BRANCH(Int32LessThan(Int32Add(*i, *actualDeleteCount), srcElementsLen), &getSrcEle, &setEle);
                    Bind(&getSrcEle);
                    {
                        ele = GetTaggedValueWithElementsKind(glue, thisValue, Int32Add(*i, *actualDeleteCount));
                        Jump(&setEle);
                    }
                    Bind(&setEle);
                    {
                        Label setIndexLessLen(env);
                        BRANCH(Int32LessThan(Int32Add(*i, *insertCount), srcElementsLen), &setIndexLessLen, &loopEnd);
                        Bind(&setIndexLessLen);
                        {
                            SetValueWithElementsKind(glue, thisValue, *ele, Int32Add(*i, *insertCount), Boolean(true),
                                                     Int32(Elements::ToUint(ElementsKind::NONE)));
                            Jump(&loopEnd);
                        }
                    }
                }
                Bind(&loopEnd);
                i = Int32Add(*i, Int32(1));
                LoopEndWithCheckSafePoint(&loopHead, env, glue);
                Bind(&loopExit);
                Jump(&trimCheck);
            }

            Label trim(env);
            Label noTrim(env);
            Bind(&trimCheck);
            GateRef needTrim = LogicAndBuilder(env)
                .And(Int32GreaterThan(oldCapacity, newCapacity))
                .And(Int32GreaterThan(Int32Sub(oldCapacity, newCapacity), Int32(TaggedArray::MAX_END_UNUSED)))
                .Done();
            BRANCH(needTrim, &trim, &noTrim);
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
                    BRANCH(Int32LessThan(*idx, arrayLen), &next1, &loopExit1);
                    Bind(&next1);

                    Label setHole(env);
                    BRANCH(Int32LessThan(*idx, srcElementsLen), &setHole, &loopEnd1);
                    Bind(&setHole);
                    {
                        SetValueWithElementsKind(glue, thisValue, Hole(), *idx, Boolean(true),
                                                 Int32(Elements::ToUint(ElementsKind::NONE)));
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
                    BRANCH(Int32GreaterThan(*j, *start), &next, &loopExit);
                    Bind(&next);
                    ele = GetTaggedValueWithElementsKind(glue, thisValue, Int32Sub(Int32Add(*j, *actualDeleteCount),
                        Int32(1)));
                    SetValueWithElementsKind(glue, thisValue, *ele, Int32Sub(Int32Add(*j, *insertCount), Int32(1)),
                                             Boolean(true), Int32(Elements::ToUint(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
                Bind(&loopEnd);
                j = Int32Sub(*j, Int32(1));
                LoopEndWithCheckSafePoint(&loopHead, env, glue);
                Bind(&loopExit);
                Jump(&insertCountVal);
            }
            Bind(&insertCountVal);
            {
                Label threeArgs(env);
                BRANCH(Int32Equal(ChangeIntPtrToInt32(numArgs), Int32(3)), &threeArgs, &setArrayLen); // 3 : three arg
                Bind(&threeArgs);
                {
                    GateRef e = GetCallArg2(numArgs);
                    SetValueWithElementsKind(glue, thisValue, e, *start, Boolean(true),
                                             Int32(Elements::ToUint(ElementsKind::NONE)));
                    Jump(&setArrayLen);
                }
            }
            Bind(&setArrayLen);
            SetArrayLength(glue, thisValue, newCapacity);
            Jump(exit);
        }
    }
}

void BuiltinsArrayStubBuilder::ToSpliced(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    ToSplicedOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::CopyWithin(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    CopyWithinOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

GateRef BuiltinsArrayStubBuilder::CalculatePositionWithLength(GateRef position, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT64(), Int64(0));
    Label positionLessThanZero(env);
    Label positionNotLessThanZero(env);
    Label resultNotGreaterThanZero(env);
    Label positionLessThanLength(env);
    Label positionNotLessThanLength(env);
    Label afterCalculatePosition(env);

    BRANCH(Int64LessThan(position, Int64(0)), &positionLessThanZero, &positionNotLessThanZero);
    Bind(&positionLessThanZero);
    {
        result = Int64Add(position, length);
        BRANCH(Int64GreaterThan(*result, Int64(0)), &afterCalculatePosition, &resultNotGreaterThanZero);
        Bind(&resultNotGreaterThanZero);
        result = Int64(0);
        Jump(&afterCalculatePosition);
    }
    Bind(&positionNotLessThanZero);
    {
        BRANCH(Int64LessThan(position, length), &positionLessThanLength, &positionNotLessThanLength);
        Bind(&positionLessThanLength);
        {
            result = position;
            Jump(&afterCalculatePosition);
        }
        Bind(&positionNotLessThanLength);
        {
            result = length;
            Jump(&afterCalculatePosition);
        }
    }
    Bind(&afterCalculatePosition);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsArrayStubBuilder::Some(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    SomeOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::Every(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    EveryOptimised(glue, thisValue, numArgs, result, exit, slowPath);
}

void BuiltinsArrayStubBuilder::ReduceRight(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label notCOWArray(env);
    Label isGeneric(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    // don't check constructor, "ReduceRight" won't create new array.
    Bind(&isJsArray);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    DEFVARIABLE(thisLen, VariableType::INT32(), Int32(0));
    DEFVARIABLE(accumulator, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    Label atLeastOneArg(env);
    Label callbackFnHandleHeapObject(env);
    Label callbackFnHandleCallable(env);
    Label updateAccumulator(env);
    Label thisIsStable(env);
    Label thisNotStable(env);
    thisLen = GetArrayLength(thisValue);
    BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
    Bind(&atLeastOneArg);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &callbackFnHandleHeapObject, slowPath);
    Bind(&callbackFnHandleHeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callbackFnHandleCallable, slowPath);
    Bind(&callbackFnHandleCallable);
    GateRef numArgsLessThanTwo = Int64LessThan(numArgs, IntPtr(2));                 // 2: callbackFn initialValue
    k = Int32Sub(*thisLen, Int32(1));
    BRANCH(numArgsLessThanTwo, slowPath, &updateAccumulator);           // 2: callbackFn initialValue
    Bind(&updateAccumulator);
    accumulator = GetCallArg1(numArgs);
    BRANCH(IsStableJSArray(glue, thisValue), &thisIsStable, &thisNotStable);

    Bind(&thisIsStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        GateRef argsLength = Int32(4);
        NewObjectStubBuilder newBuilder(this);
        GateRef argList = newBuilder.NewTaggedArray(glue, argsLength);
        Label loopHead(env);
        Label next(env);
        Label loopEnd(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label nextStep(env);
            Label kValueIsHole(env);
            Label callDispatch(env);
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label hasException1(env);
            Label notHasException1(env);
            GateRef thisLenVal = *thisLen;
            BRANCH(LogicAndBuilder(env).And(IsStableJSArray(glue, thisValue))
                .And(Int32Equal(thisLenVal, GetArrayLength(thisValue))).Done(),
                &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH(Int32GreaterThanOrEqual(*k, Int32(0)), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *k);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                                 { glue, thisValue, IntToTaggedPtr(*k), GetCurrentGlobalEnv() });
                BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
                Bind(&hasProperty);
                kValue = FastGetPropertyByIndex(glue, thisValue, *k, ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                result->WriteVariable(Exception());
                Jump(exit);
                Bind(&notHasException0);
                BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
            }
            Bind(&callDispatch);
            {
                // callback param 0: accumulator
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(0), *accumulator);
                // callback param 1: currentValue
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(1), *kValue);
                // callback param 2: index
                SetValueToTaggedArray(VariableType::INT32(), glue, argList, Int32(2), IntToTaggedInt(*k));
                // callback param 3: array
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(3), thisValue);
                GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                callArgs.callThisArgvWithReturnArgs = { argsLength, argv, Undefined() };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, argsLength, 0, nullptr, Circuit::NullGate(),
                    callArgs);
                GateRef callResult = callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }

                Bind(&notHasException1);
                {
                    accumulator = callResult;
                    Jump(&loopEnd);
                }
            }
        }
        Bind(&loopEnd);
        k = Int32Sub(*k, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(*accumulator);
        Jump(exit);
    }

    Bind(&thisNotStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        GateRef argsLength = Int32(4);
        NewObjectStubBuilder newBuilder(this);
        GateRef argList = newBuilder.NewTaggedArray(glue, argsLength);
        Label loopHead(env);
        Label next(env);
        Label loopEnd(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int32GreaterThanOrEqual(*k, Int32(0)), &next, &loopExit);
            Bind(&next);
            GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                             { glue, thisValue, IntToTaggedPtr(*k), GetCurrentGlobalEnv() });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
            Bind(&hasProperty);
            kValue = FastGetPropertyByIndex(glue, thisValue, *k, ProfileOperation());
            BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
            Bind(&hasException0);
            result->WriteVariable(Exception());
            Jump(exit);
            Bind(&notHasException0);
            BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
            Bind(&callDispatch);
            {
                // callback param 0: accumulator
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(0), *accumulator);
                // callback param 1: currentValue
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(1), *kValue);
                // callback param 2: index
                SetValueToTaggedArray(VariableType::INT32(), glue, argList, Int32(2), IntToTaggedInt(*k));
                // callback param 3: array
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argList, Int32(3), thisValue);
                GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                callArgs.callThisArgvWithReturnArgs = { argsLength, argv, Undefined() };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, argsLength, 0, nullptr, Circuit::NullGate(),
                    callArgs);
                GateRef callResult = callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }

                Bind(&notHasException1);
                {
                    accumulator = callResult;
                    Jump(&loopEnd);
                }
            }
        }
        Bind(&loopEnd);
        k = Int32Sub(*k, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(*accumulator);
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::FindLastIndex(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    // don't check constructor, "FindLastIndex" won't create new array.
    Bind(&isJsArray);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    Label arg0HeapObject(env);
    Label callable(env);
    Label thisIsStable(env);
    Label thisNotStable(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef argHandle = GetCallArg1(numArgs);

    DEFVARIABLE(i, VariableType::INT64(), Int64Sub(ZExtInt32ToInt64(GetArrayLength(thisValue)), Int64(1)));
    BRANCH(IsStableJSArray(glue, thisValue), &thisIsStable, &thisNotStable);

    Bind(&thisIsStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label nextStep(env);
            Label kValueIsHole(env);
            Label callDispatch(env);
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label useUndefined(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(IsStableJSArray(glue, thisValue), &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH(Int64LessThan(*i, Int64(0)), &loopExit, &next);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                                 { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
                BRANCH(TaggedIsTrue(hasProp), &hasProperty, &useUndefined);
                Bind(&hasProperty);
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    BRANCH(TaggedIsHole(*kValue), &useUndefined, &callDispatch);
                    Bind(&useUndefined);
                    kValue = Undefined();
                    Jump(&callDispatch);
                }
            }
            Bind(&callDispatch);
            {
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }

                Bind(&notHasException1);
                {
                    DEFVARIABLE(newLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
                    Label checkRetValue(env);
                    Label find(env);
                    BRANCH(Int64LessThan(*newLen, Int64Add(*i, Int64(1))), &thisNotStable, &checkRetValue);
                    Bind(&checkRetValue);
                    BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &loopEnd);
                    Bind(&find);
                    result->WriteVariable(IntToTaggedPtr(*i));
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Sub(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }

    Bind(&thisNotStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label useUndefined(env);
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int64LessThan(*i, Int64(0)), &loopExit, &next);
            Bind(&next);
            GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                             { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &useUndefined);
            Bind(&hasProperty);
            kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
            BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
            Bind(&hasException0);
            {
                result->WriteVariable(Exception());
                Jump(exit);
            }
            Bind(&notHasException0);
            {
                BRANCH(TaggedIsHole(*kValue), &useUndefined, &callDispatch);
                Bind(&useUndefined);
                kValue = Undefined();
                Jump(&callDispatch);
                Bind(&callDispatch);
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }

                Bind(&notHasException1);
                {
                    Label find(env);
                    BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &loopEnd);
                    Bind(&find);
                    result->WriteVariable(IntToTaggedPtr(*i));
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Sub(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::FindLast(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "FindLast" won't create new array.
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    Label arg0HeapObject(env);
    Label callable(env);
    Label thisIsStable(env);
    Label thisNotStable(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef argHandle = GetCallArg1(numArgs);

    DEFVARIABLE(i, VariableType::INT64(), Int64Sub(ZExtInt32ToInt64(GetArrayLength(thisValue)), Int64(1)));
    BRANCH(IsStableJSArray(glue, thisValue), &thisIsStable, &thisNotStable);

    Bind(&thisIsStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label nextStep(env);
            Label kValueIsHole(env);
            Label callDispatch(env);
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label useUndefined(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(IsStableJSArray(glue, thisValue), &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH(Int64LessThan(*i, Int64(0)), &loopExit, &next);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                                 { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
                BRANCH(TaggedIsTrue(hasProp), &hasProperty, &useUndefined);
                Bind(&hasProperty);
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    BRANCH(TaggedIsHole(*kValue), &useUndefined, &callDispatch);
                    Bind(&useUndefined);
                    kValue = Undefined();
                    Jump(&callDispatch);
                }
            }
            Bind(&callDispatch);
            {
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }

                Bind(&notHasException1);
                {
                    DEFVARIABLE(newLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
                    Label checkRetValue(env);
                    Label find(env);
                    BRANCH(Int64LessThan(*newLen, Int64Add(*i, Int64(1))), &thisNotStable, &checkRetValue);
                    Bind(&checkRetValue);
                    BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &loopEnd);
                    Bind(&find);
                    result->WriteVariable(*kValue);
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Sub(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(exit);
    }

    Bind(&thisNotStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label useUndefined(env);
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int64LessThan(*i, Int64(0)), &loopExit, &next);
            Bind(&next);
            GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                             { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &useUndefined);
            Bind(&hasProperty);
            kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
            BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
            Bind(&hasException0);
            {
                result->WriteVariable(Exception());
                Jump(exit);
            }
            Bind(&notHasException0);
            {
                BRANCH(TaggedIsHole(*kValue), &useUndefined, &callDispatch);
                Bind(&useUndefined);
                {
                    kValue = Undefined();
                    Jump(&callDispatch);
                }
                Bind(&callDispatch);
                {
                    GateRef key = Int64ToTaggedInt(*i);
                    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                    callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                    CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                        nullptr, Circuit::NullGate(), callArgs);
                    GateRef retValue = callBuilder.JSCallDispatch();
                    BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                    Bind(&hasException1);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }

                    Bind(&notHasException1);
                    {
                        Label find(env);
                        BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &loopEnd);
                        Bind(&find);
                        result->WriteVariable(*kValue);
                        Jump(exit);
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Sub(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::FlatMap(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isStable(env);
    Label defaultConstr(env);
    Label notJSCOWArray(env);
    Label mapFuncIsHeapObject(env);
    Label mapFuncIsCallable(env);
    BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &isStable, slowPath);
    Bind(&isStable);
    // need check constructor, "FlatMap" should use ArraySpeciesCreate
    BRANCH_LIKELY(HasConstructor(glue, thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notJSCOWArray);
    Bind(&notJSCOWArray);
    GateRef mapFunc = GetCallArg0(numArgs);
    BRANCH_LIKELY(TaggedIsHeapObject(mapFunc), &mapFuncIsHeapObject, slowPath);
    Bind(&mapFuncIsHeapObject);
    BRANCH_LIKELY(IsCallable(glue, mapFunc), &mapFuncIsCallable, slowPath);
    Bind(&mapFuncIsCallable);
    GateRef argHandle = GetCallArg1(numArgs);

    GateRef srcLength = GetArrayLength(thisValue);
    GateRef resArray = NewArray(glue, srcLength);
    DEFVARIABLE(resElements, VariableType::JS_POINTER(), GetElementsArray(glue, resArray));
    DEFVARIABLE(resElementsCap, VariableType::INT32(), GetLengthOfTaggedArray(*resElements));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(targetIndex, VariableType::INT32(), Int32(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
    
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Label next(env);
        BRANCH_LIKELY(Int32UnsignedLessThan(*i, srcLength), &next, &loopExit);
        Bind(&next);
        Label isStableInLoop(env);
        Label notStableInLoop(env);
        // array may be modified, recheck if the array is still stable
        BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &isStableInLoop, &notStableInLoop);
        Bind(&notStableInLoop);
        {
            // set current elements to resArray and CallRuntime
            SetArrayLength(glue, resArray, *targetIndex);
            SetElementsArray(VariableType::JS_POINTER(), glue, resArray, *resElements);
            GateRef runtimeResult = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                RTSTUB_ID(ContinueFlatMapBeforeCall), {thisValue, resArray, mapFunc, argHandle,
                IntUnsignedToTaggedInt(*targetIndex), IntUnsignedToTaggedInt(*i), IntUnsignedToTaggedInt(srcLength)});
            result->WriteVariable(runtimeResult);
            Jump(exit);
        }
        Bind(&isStableInLoop);
        Label getElement(env);
        // array may be modified, and the current length needs to be checked
        BRANCH_LIKELY(Int32UnsignedLessThan(*i, GetArrayLength(thisValue)), &getElement, &loopEnd);
        Bind(&getElement);
        kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
        // call mapFunc
        Label callDispatch(env);
        BRANCH_UNLIKELY(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
        Bind(&callDispatch);
        GateRef key = IntUnsignedToTaggedInt(*i);
        JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
        callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
        CallStubBuilder callBuilder(this, glue, mapFunc, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
            Circuit::NullGate(), callArgs, ProfileOperation(), false);
        GateRef element = callBuilder.JSCallDispatch();
        Label hasException(env);
        Label noException(env);
        BRANCH_UNLIKELY(HasPendingException(glue), &hasException, &noException);
        Bind(&hasException);
        {
            result->WriteVariable(Exception());
            Jump(exit);
        }
        Bind(&noException);
        // do flat
        Label elementIsHeapObj(env);
        Label elementNotHeapObj(env);
        Label flatInnerArray(env);
        Label setElementToRes(env);
        BRANCH(TaggedIsHeapObject(element), &elementIsHeapObj, &elementNotHeapObj);
        Bind(&elementIsHeapObj);
        {
            Label elementNotJSArray(env);
            BRANCH(IsJsArray(glue, element), &flatInnerArray, &elementNotJSArray);
            Bind(&elementNotJSArray);
            {
                Label elementIsProxy(env);
                BRANCH_UNLIKELY(IsJsProxy(glue, element), &elementIsProxy, &setElementToRes);
                Bind(&elementIsProxy);
                // set current elements to resArray and CallRuntime
                SetArrayLength(glue, resArray, *targetIndex);
                SetElementsArray(VariableType::JS_POINTER(), glue, resArray, *resElements);
                GateRef runtimeResult = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                    RTSTUB_ID(ContinueFlatMapAfterCall), {thisValue, resArray, mapFunc, argHandle,
                    IntUnsignedToTaggedInt(*targetIndex), IntUnsignedToTaggedInt(*i),
                    IntUnsignedToTaggedInt(srcLength), element});
                result->WriteVariable(runtimeResult);
                Jump(exit);
            }
        }
        Bind(&elementNotHeapObj);
        {
            BRANCH(TaggedIsHole(element), &loopEnd, &setElementToRes);
        }
        Bind(&flatInnerArray);
        {
            Label exception(env);
            FlatInnerArray(glue, resElements, targetIndex, resElementsCap, element, &loopEnd, &exception);
            Bind(&exception);
            result->WriteVariable(Exception());
            Jump(exit);
        }
        Bind(&setElementToRes);
        {
            // Due to space limitations, targetIndex > MAX_SAFE_INTEGER is impossible, so we skip this check
            Label setTarget(env);
            Label needExpend(env);
            BRANCH_LIKELY(Int32UnsignedLessThan(*targetIndex, *resElementsCap), &setTarget, &needExpend);
            Bind(&needExpend);
            resElementsCap = ComputeElementCapacity(*resElementsCap);
            NewObjectStubBuilder newBuilder(this);
            resElements = newBuilder.ExtendArrayWithOptimizationCheck(glue, *resElements, *resElementsCap);
            Jump(&setTarget);
            Bind(&setTarget);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *resElements, *targetIndex, element);
            targetIndex = Int32Add(*targetIndex, Int32(1));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);

    Label needTrim(env);
    Label setElements(env);
    GateRef unused = Int32Sub(*resElementsCap, *targetIndex);
    BRANCH(Int32UnsignedGreaterThan(unused, Int32(TaggedArray::MAX_END_UNUSED)), &needTrim, &setElements);
    Bind(&needTrim);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, *resElements, ZExtInt32ToInt64(*targetIndex)});
        Jump(&setElements);
    }
    Bind(&setElements);
    SetArrayLength(glue, resArray, *targetIndex);
    SetElementsArray(VariableType::JS_POINTER(), glue, resArray, *resElements);
    result->WriteVariable(resArray);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::FlatInnerArray(GateRef glue, Variable &resElements, Variable &targetIndex,
                                              Variable &resElementsCap, GateRef value, Label *exit, Label *exception)
{
    auto env = GetEnvironment();
    Label isStable(env);
    Label notStable(env);
    Label out(env);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    GateRef valueLength = GetArrayLength(value);
    BRANCH(IsStableJSArray(glue, value), &isStable, &notStable);
    Bind(&isStable);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label next(env);
            BRANCH_LIKELY(Int32UnsignedLessThan(*i, valueLength), &next, &loopExit);
            Bind(&next);
            GateRef kValue = GetTaggedValueWithElementsKind(glue, value, *i);
            Label setElement(env);
            BRANCH(TaggedIsHole(kValue), &loopEnd, &setElement);
            Bind(&setElement);
            {
                Label setTarget(env);
                Label needExpend(env);
                BRANCH_LIKELY(Int32UnsignedLessThan(*targetIndex, *resElementsCap), &setTarget, &needExpend);
                Bind(&needExpend);
                resElementsCap = ComputeElementCapacity(*resElementsCap);
                NewObjectStubBuilder newBuilder(this);
                resElements = newBuilder.ExtendArrayWithOptimizationCheck(glue, *resElements, *resElementsCap);
                Jump(&setTarget);
                Bind(&setTarget);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *resElements, *targetIndex, kValue);
                targetIndex = Int32Add(*targetIndex, Int32(1));
            }
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&out);
    }
    Bind(&notStable);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label next(env);
            Label hasProperty(env);
            Label noException1(env);
            Label noException2(env);
            BRANCH_LIKELY(Int32UnsignedLessThan(*i, valueLength), &next, &loopExit);
            Bind(&next);
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                                                       { value, IntToTaggedInt(*i) });
            BRANCH_UNLIKELY(HasPendingException(glue), exception, &noException1);
            Bind(&noException1);
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
            Bind(&hasProperty);
            {
                GateRef kValue = FastGetPropertyByIndex(glue, value, *i, ProfileOperation());
                BRANCH_UNLIKELY(HasPendingException(glue), exception, &noException2);
                Bind(&noException2);
                {
                    Label setTarget(env);
                    Label needExpend(env);
                    BRANCH_LIKELY(Int32UnsignedLessThan(*targetIndex, *resElementsCap), &setTarget, &needExpend);
                    Bind(&needExpend);
                    resElementsCap = ComputeElementCapacity(*resElementsCap);
                    NewObjectStubBuilder newBuilder(this);
                    resElements = newBuilder.ExtendArrayWithOptimizationCheck(glue, *resElements, *resElementsCap);
                    Jump(&setTarget);
                    Bind(&setTarget);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, *resElements, *targetIndex, kValue);
                    targetIndex = Int32Add(*targetIndex, Int32(1));
                }
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&out);
    }
    Bind(&out);
    {
        Jump(exit);
    }
}


void BuiltinsArrayStubBuilder::FastCreateArrayWithArgv(GateRef glue, Variable *res, GateRef argc,
                                                       GateRef hclass, Label *exit)
{
    auto env = GetEnvironment();
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    // create elements from argv
    GateRef len = TruncInt64ToInt32(argc);
    GateRef elements = newBuilder.NewTaggedArray(glue, len);

    // set value
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(newHClass, VariableType::JS_ANY(), hclass);
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
#if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
    DEFVARIABLE(elementKind, VariableType::INT32(), Int32(Elements::ToUint(ElementsKind::NONE)));
#else
    DEFVARIABLE(elementKind, VariableType::INT32(), Int32(Elements::ToUint(ElementsKind::NONE)));
#endif
    Label loopHead(env);
    Label loopEnd(env);
    Label setValue(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int64LessThan(*i, argc), &setValue, &loopExit);
        Bind(&setValue);
        Label isHole(env);
        Label notHole(env);
        value = GetArgFromArgv(glue, *i);
        BRANCH(TaggedIsHole(*value), &isHole, &notHole);
        Bind(&isHole);
        value = TaggedUndefined();
        Jump(&notHole);
        Bind(&notHole);
        elementKind = Int32Or(TaggedToElementKind(glue, *value), *elementKind);
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, *i, *value);
        i = Int64Add(*i, Int64(1));
        Jump(&loopEnd);
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&loopExit);
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef noneHClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
        static_cast<size_t>(GlobalEnvField::ELEMENT_NONE_HCLASS_INDEX));
    Label useElementsKindHClass(env);
    Label createArray(env);
    GateRef newHClassVal = *newHClass;
    // if the newHClass is not noneHClass, means the elementskind is not enable or "Array" is modified, then do not use
    // specific hclass for elementskind.
    BRANCH_LIKELY(Equal(newHClassVal, noneHClass), &useElementsKindHClass, &createArray);
    Bind(&useElementsKindHClass);
    {
        // elementKind may be an invalid kind, but use it to index the hclass is supported.
        newHClass = GetElementsKindHClass(glue, *elementKind);
        Jump(&createArray);
    }
    Bind(&createArray);
    // create array object
    GateRef arr = newBuilder.NewJSObject(glue, *newHClass);
    res->WriteVariable(arr);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, arr, lengthOffset, len);
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    Store(VariableType::JS_ANY(), glue, arr,
          IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)), accessor);
    SetExtensibleToBitfield(glue, arr, true);
    SetElementsArray(VariableType::JS_POINTER(), glue, arr, elements);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::GenArrayConstructor(GateRef glue, GateRef nativeCode,
    GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsHeapObject(env);
    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(glue, newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label fastGetHclass(env);
        Label intialHClassIsHClass(env);
        GateRef globalEnv = GetCurrentGlobalEnv();
        auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                           GlobalEnv::ARRAY_FUNCTION_INDEX);
        BRANCH(Equal(arrayFunc, newTarget), &fastGetHclass, &slowPath1);
        Bind(&fastGetHclass);
        GateRef intialHClass =
            Load(VariableType::JS_ANY(), glue, newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        DEFVARIABLE(arrayLength, VariableType::INT64(), Int64(0));
        BRANCH(IsJSHClass(glue, intialHClass), &intialHClassIsHClass, &slowPath1);
        Bind(&intialHClassIsHClass);
        {
            Label noArg(env);
            Label hasArg(env);
            Label arrayCreate(env);
            BRANCH(Int64Equal(numArgs, IntPtr(0)), &noArg, &hasArg);
            Bind(&noArg);
            {
                Jump(&arrayCreate);
            }
            Bind(&hasArg);
            {
                Label hasOneArg(env);
                Label multiArg(env);
                BRANCH(Int64Equal(numArgs, IntPtr(1)), &hasOneArg, &multiArg);
                Bind(&hasOneArg);
                {
                    Label argIsNumber(env);
                    GateRef arg0 = GetArgFromArgv(glue, IntPtr(0), numArgs, true);
                    BRANCH(TaggedIsNumber(arg0), &argIsNumber, &slowPath);
                    Bind(&argIsNumber);
                    {
                        Label argIsInt(env);
                        Label argIsDouble(env);
                        BRANCH(TaggedIsInt(arg0), &argIsInt, &argIsDouble);
                        Bind(&argIsInt);
                        {
                            Label validIntLength(env);
                            GateRef intLen = GetInt64OfTInt(arg0);
                            GateRef isGEZero = Int64GreaterThanOrEqual(intLen, Int64(0));
                            GateRef isLEMaxLen = Int64LessThanOrEqual(intLen, Int64(JSArray::MAX_ARRAY_INDEX));
                            BRANCH(BitAnd(isGEZero, isLEMaxLen), &validIntLength, &slowPath);
                            Bind(&validIntLength);
                            {
                                arrayLength = intLen;
                                Jump(&arrayCreate);
                            }
                        }
                        Bind(&argIsDouble);
                        {
                            Label validDoubleLength(env);
                            GateRef doubleLength = GetDoubleOfTDouble(arg0);
                            GateRef doubleToInt = DoubleToInt(glue, doubleLength);
                            GateRef intToDouble = CastInt64ToFloat64(SExtInt32ToInt64(doubleToInt));
                            GateRef doubleEqual = DoubleEqual(doubleLength, intToDouble);
                            GateRef doubleLEMaxLen =
                                DoubleLessThanOrEqual(doubleLength, Double(JSArray::MAX_ARRAY_INDEX));
                            BRANCH(BitAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, &slowPath);
                            Bind(&validDoubleLength);
                            {
                                arrayLength = SExtInt32ToInt64(doubleToInt);
                                Jump(&arrayCreate);
                            }
                        }
                    }
                }
                Bind(&multiArg);
                {
                    Label lengthValid(env);
                    BRANCH(Int64LessThan(numArgs, IntPtr(JSObject::MAX_GAP)), &lengthValid, &slowPath);
                    Bind(&lengthValid);
                    {
                        FastCreateArrayWithArgv(glue, &res, numArgs, intialHClass, &exit);
                    }
                }
            }
            Bind(&arrayCreate);
            {
                Label lengthValid(env);
                BRANCH(Int64GreaterThan(*arrayLength, Int64(JSObject::MAX_GAP)), &slowPath, &lengthValid);
                Bind(&lengthValid);
                {
                    NewObjectStubBuilder newBuilder(this, globalEnv);
                    newBuilder.SetParameters(glue, 0);
                    res = newBuilder.NewJSArrayWithSize(intialHClass, *arrayLength);
                    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
                    Store(VariableType::INT32(), glue, *res, lengthOffset, TruncInt64ToInt32(*arrayLength));
                    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                              ConstantIndex::ARRAY_LENGTH_ACCESSOR);
                    Store(VariableType::JS_ANY(), glue, *res,
                          IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)), accessor);
                    SetExtensibleToBitfield(glue, *res, true);
                    Jump(&exit);
                }
            }
        }
        Bind(&slowPath1);
        {
            GateRef argv = GetArgv();
            res = CallBuiltinRuntimeWithNewTarget(glue,
                { glue, nativeCode, func, thisValue, numArgs, argv, newTarget });
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue, { glue, nativeCode, func, thisValue, numArgs, argv }, true);
        Jump(&exit);
    }

    Bind(&exit);
    Return(*res);
}

void BuiltinsArrayStubBuilder::IsArray(GateRef glue, [[maybe_unused]] GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs);
    Label isHeapObj(env);
    Label notHeapObj(env);
    BRANCH(TaggedIsHeapObject(obj), &isHeapObj, &notHeapObj);
    Bind(&isHeapObj);
    {
        Label isJSArray(env);
        Label notJSArray(env);
        GateRef objectType = GetObjectType(LoadHClass(glue, obj));
        BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARRAY))), &isJSArray, &notJSArray);
        Bind(&isJSArray);
        {
            result->WriteVariable(TaggedTrue());
            Jump(exit);
        }
        Bind(&notJSArray);
        BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_PROXY))), slowPath, &notHeapObj);
    }
    Bind(&notHeapObj);
    {
        result->WriteVariable(TaggedFalse());
        Jump(exit);
    }
}
}  // namespace panda::ecmascript::kungfu
