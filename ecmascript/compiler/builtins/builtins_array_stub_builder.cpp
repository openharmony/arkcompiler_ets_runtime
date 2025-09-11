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
#if ENABLE_NEXT_OPTIMIZATION
    WithOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    DEFVARIABLE(relativeIndex, VariableType::INT64(), Int64(0));
    DEFVARIABLE(actualIndex, VariableType::INT64(), Int64(0));
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStableArray(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "with" always use ArrayCreate to create array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStableArray, slowPath);
    Bind(&isStableArray);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    GateRef index = GetCallArg0(numArgs);
    Label taggedIsInt(env);
    BRANCH(TaggedIsInt(index), &taggedIsInt, slowPath);
    Bind(&taggedIsInt);
    {
        relativeIndex = GetInt64OfTInt(index);
        DEFVARIABLE(value, VariableType::JS_ANY(), Hole());
        Label twoArg(env);
        Label ifOneArg(env);
        Label getIndex(env);
        // 2 : means there are two args
        BRANCH(Int64Equal(numArgs, IntPtr(2)), &twoArg, &ifOneArg);
        Bind(&twoArg);
        {
            value = GetCallArg1(numArgs);
            Jump(&getIndex);
        }
        Bind(&ifOneArg);
        {
            // 1 : means there are only one arg
            BRANCH(Int64Equal(numArgs, IntPtr(1)), &getIndex, slowPath);
        }
        Bind(&getIndex);
        {
            Label indexGreaterOrEqualZero(env);
            Label indexLessZero(env);
            Label next(env);
            Label notOutOfRange(env);
            BRANCH(Int64GreaterThanOrEqual(*relativeIndex, Int64(0)), &indexGreaterOrEqualZero, &indexLessZero);
            Bind(&indexGreaterOrEqualZero);
            {
                actualIndex = *relativeIndex;
                Jump(&next);
            }
            Bind(&indexLessZero);
            {
                actualIndex = Int64Add(thisLen, *relativeIndex);
                Jump(&next);
            }
            Bind(&next);
            {
                BRANCH(BitOr(Int64GreaterThanOrEqual(*actualIndex, thisLen), Int64LessThan(*actualIndex, Int64(0))),
                    slowPath, &notOutOfRange);
                Bind(&notOutOfRange);
                {
                    GateRef newArray = NewArray(glue, Int32(0));
                    GrowElementsCapacity(glue, newArray, TruncInt64ToInt32(thisLen));
                    DEFVARIABLE(k, VariableType::INT64(), Int64(0));
                    Label loopHead(env);
                    Label loopEnd(env);
                    Label loopExit(env);
                    Label loopNext(env);
                    Label replaceIndex(env);
                    Label notReplaceIndex(env);
                    Jump(&loopHead);
                    LoopBegin(&loopHead);
                    {
                        BRANCH(Int64LessThan(*k, thisLen), &loopNext, &loopExit);
                        Bind(&loopNext);
                        BRANCH(Int64Equal(*k, *actualIndex), &replaceIndex, &notReplaceIndex);
                        Bind(&replaceIndex);
                        {
                            SetValueWithElementsKind(glue, newArray, *value, *k, Boolean(true),
                                Int32(Elements::ToUint(ElementsKind::NONE)));
                            Jump(&loopEnd);
                        }
                        Bind(&notReplaceIndex);
                        {
                            GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, *k);
                            Label eleIsHole(env);
                            Label eleNotHole(env);
                            BRANCH(TaggedIsHole(ele), &eleIsHole, &eleNotHole);
                            Bind(&eleIsHole);
                            {
                                SetValueWithElementsKind(glue, newArray, Undefined(), *k, Boolean(true),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&eleNotHole);
                            {
                                SetValueWithElementsKind(glue, newArray, ele, *k, Boolean(true),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                        }
                    }
                    Bind(&loopEnd);
                    k = Int64Add(*k, Int64(1));
                    LoopEnd(&loopHead);
                    Bind(&loopExit);
                    SetArrayLength(glue, newArray, thisLen);
                    result->WriteVariable(newArray);
                    Jump(exit);
                }
            }
        }
    }
#endif
}

void BuiltinsArrayStubBuilder::Unshift(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    UnshiftOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStableJsArray(env);
    Label notOverRange(env);
    Label numNotEqualZero(env);
    Label numLessThanOrEqualThree(env);
    Label loopHead(env);
    Label next(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label grow(env);
    Label setValue(env);
    Label numEqual2(env);
    Label numEqual3(env);
    Label threeArgs(env);
    Label final(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(IsStableJSArray(glue, thisValue), &isStableJsArray, slowPath);
    Bind(&isStableJsArray);
    BRANCH(Int64GreaterThan(numArgs, IntPtr(0)), &numNotEqualZero, slowPath);
    Bind(&numNotEqualZero);
    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    GateRef argLen = ZExtInt32ToInt64(ChangeIntPtrToInt32(numArgs));
    GateRef newLen = Int64Add(thisLen, argLen);
    BRANCH(Int64GreaterThan(newLen, Int64(base::MAX_SAFE_INTEGER)), slowPath, &notOverRange);
    Bind(&notOverRange);
    // 3 : max param num
    BRANCH(Int64LessThanOrEqual(numArgs, IntPtr(3)), &numLessThanOrEqualThree, slowPath);
    Bind(&numLessThanOrEqualThree);
    {
        DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
        GateRef capacity = ZExtInt32ToInt64(GetLengthOfTaggedArray(*elements));
        BRANCH(Int64GreaterThan(newLen, capacity), &grow, &setValue);
        Bind(&grow);
        {
            elements = CallRuntime(glue, RTSTUB_ID(JSObjectGrowElementsCapacity), {thisValue, IntToTaggedInt(newLen)});
            Jump(&setValue);
        }
        Bind(&setValue);
        {
            DEFVARIABLE(fromKey, VariableType::INT64(), Int64Sub(thisLen, Int64(1)));
            DEFVARIABLE(toKey, VariableType::INT64(), Int64Sub(newLen, Int64(1)));
            DEFVARIABLE(ele, VariableType::JS_ANY(), Hole());
            Label eleIsHole(env);
            Label hasProperty(env);
            Label notHasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int64GreaterThanOrEqual(*fromKey, Int64(0)), &next, &loopExit);
                Bind(&next);
                {
                    ele = GetTaggedValueWithElementsKind(glue, thisValue, *fromKey);
                    BRANCH(TaggedIsHole(*ele), &eleIsHole, &notHasException0);
                    Bind(&eleIsHole);
                    {
                        GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                            { thisValue, IntToTaggedInt(*fromKey) });
                        BRANCH(TaggedIsTrue(hasProp), &hasProperty, &notHasProperty);
                        Bind(&hasProperty);
                        {
                            ele = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*fromKey),
                                ProfileOperation());
                            BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                            Bind(&hasException0);
                            {
                                result->WriteVariable(Exception());
                                Jump(exit);
                            }
                        }
                        Bind(&notHasProperty);
                        {
                            SetValueWithElementsKind(glue, thisValue, Hole(), *toKey, Boolean(false),
                                Int32(Elements::ToUint(ElementsKind::NONE)));
                            Jump(&loopEnd);
                        }
                    }
                    Bind(&notHasException0);
                    {
                        SetValueWithElementsKind(glue, thisValue, *ele, *toKey, Boolean(false),
                            Int32(Elements::ToUint(ElementsKind::NONE)));
                        Jump(&loopEnd);
                    }
                }
            }
            Bind(&loopEnd);
            fromKey = Int64Sub(*fromKey, Int64(1));
            toKey = Int64Sub(*toKey, Int64(1));
            LoopEnd(&loopHead);
            Bind(&loopExit);
            {
                GateRef value0 = GetCallArg0(numArgs);
                // 0 : the first Element position
                SetValueWithElementsKind(glue, thisValue, value0, Int64(0), Boolean(false),
                    Int32(Elements::ToUint(ElementsKind::NONE)));
                // 2 : the second param
                BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(2)), &numEqual2, &numEqual3);
                Bind(&numEqual2);
                {
                    GateRef value1 = GetCallArg1(numArgs);
                    // 1 : the second Element position
                    SetValueWithElementsKind(glue, thisValue, value1, Int64(1), Boolean(false),
                        Int32(Elements::ToUint(ElementsKind::NONE)));
                    Jump(&numEqual3);
                }
                Bind(&numEqual3);
                {
                    // 3 : the third param
                    BRANCH(Int64Equal(numArgs, IntPtr(3)), &threeArgs, &final);
                    Bind(&threeArgs);
                    GateRef value2 = GetCallArg2(numArgs);
                    // 2 : the third Element position
                    SetValueWithElementsKind(glue, thisValue, value2, Int64(2), Boolean(false),
                        Int32(Elements::ToUint(ElementsKind::NONE)));
                    Jump(&final);
                }
                Bind(&final);
                {
                    SetArrayLength(glue, thisValue, newLen);
                    result->WriteVariable(IntToTaggedPtr(newLen));
                    Jump(exit);
                }
            }
        }
    }
#endif
}

void BuiltinsArrayStubBuilder::Shift(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    ShiftOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label stableJSArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    // don't check constructor, "Shift" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &stableJSArray, slowPath);
    Bind(&stableJSArray);
    {
        Label isLengthWritable(env);
        BRANCH(IsArrayLengthWritable(glue, thisValue), &isLengthWritable, slowPath);
        Bind(&isLengthWritable);
        {
            GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
            Label lengthNotZero(env);
            BRANCH(Int64Equal(thisLen, Int64(0)), exit, &lengthNotZero);
            Bind(&lengthNotZero);
            {
                Label isJsCOWArray(env);
                Label getElements(env);
                BRANCH(IsJsCOWArray(glue, thisValue), &isJsCOWArray, &getElements);
                Bind(&isJsCOWArray);
                {
                    CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(CheckAndCopyArray), { thisValue });
                    Jump(&getElements);
                }
                Bind(&getElements);
                {
                    GateRef elements = GetElementsArray(glue, thisValue);
                    GateRef capacity = ZExtInt32ToInt64(GetLengthOfTaggedArray(elements));
                    GateRef index = Int64Sub(thisLen, Int64(1));
                    DEFVARIABLE(element, VariableType::JS_ANY(), Hole());
                    element = GetTaggedValueWithElementsKind(glue, thisValue, Int64(0));
                    Label hasException0(env);
                    Label taggedHole(env);
                    Label copyArray(env);
                    BRANCH(TaggedIsHole(*element), &taggedHole, &copyArray);
                    Bind(&taggedHole);
                    {
                        element = FastGetPropertyByIndex(glue, thisValue, Int32(0), ProfileOperation());
                        BRANCH(HasPendingException(glue), &hasException0, &copyArray);
                        Bind(&hasException0);
                        {
                            result->WriteVariable(Exception());
                            Jump(exit);
                        }
                    }
                    Bind(&copyArray);
                    {
                        DEFVARIABLE(fromKey, VariableType::INT64(), Int64(1));
                        DEFVARIABLE(toKey, VariableType::INT64(), Int64Sub(*fromKey, Int64(1)));
                        Label loopHead(env);
                        Label loopNext(env);
                        Label loopEnd(env);
                        Label loopExit(env);
                        Jump(&loopHead);
                        LoopBegin(&loopHead);
                        {
                            BRANCH(Int64LessThan(*fromKey, thisLen), &loopNext, &loopExit);
                            Bind(&loopNext);
                            {
                                GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, *fromKey);
                                SetValueWithElementsKind(glue, thisValue, ele, *toKey, Boolean(false),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                        }
                        Bind(&loopEnd);
                        fromKey = Int64Add(*fromKey, Int64(1));
                        toKey = Int64Add(*toKey, Int64(1));
                        LoopEnd(&loopHead);
                        Bind(&loopExit);
                        {
                            Label noTrim(env);
                            Label needTrim(env);
                            Label setNewLen(env);
                            GateRef unused = Int64Sub(capacity, index);
                            BRANCH(Int64GreaterThan(unused, Int64(TaggedArray::MAX_END_UNUSED)), &needTrim, &noTrim);
                            Bind(&needTrim);
                            {
                                CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, elements, index});
                                Jump(&setNewLen);
                            }
                            Bind(&noTrim);
                            {
                                SetValueWithElementsKind(glue, thisValue, Hole(), index, Boolean(false),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&setNewLen);
                            }
                            Bind(&setNewLen);
                            {
                                GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
                                Store(VariableType::INT32(), glue, thisValue, lengthOffset, index);

                                Label isNotHole(env);
                                BRANCH(TaggedIsHole(*element), exit, &isNotHole);
                                Bind(&isNotHole);
                                {
                                    result->WriteVariable(*element);
                                    Jump(exit);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#endif
}

void BuiltinsArrayStubBuilder::Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    ConcatOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    {
        Label isExtensible(env);
        // need check constructor, "Concat" should use ArraySpeciesCreate
        BRANCH(HasConstructor(glue, thisValue), slowPath, &isExtensible);
        Bind(&isExtensible);
        {
            Label numArgsOne(env);
            BRANCH(Int64Equal(numArgs, IntPtr(1)), &numArgsOne, slowPath);
            Bind(&numArgsOne);
            {
                GateRef arg0 = GetCallArg0(numArgs);
                Label allStableJsArray(env);
                GateRef isAllStableJsArray = LogicAndBuilder(env).And(IsStableJSArray(glue, thisValue))
                    .And(IsStableJSArray(glue, arg0)).Done();
                BRANCH(isAllStableJsArray, &allStableJsArray, slowPath);
                Bind(&allStableJsArray);
                {
                    GateRef maxArrayIndex = Int64(TaggedArray::MAX_ARRAY_INDEX);
                    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                    GateRef argLen = ZExtInt32ToInt64(GetArrayLength(arg0));
                    GateRef sumArrayLen = Int64Add(argLen, thisLen);
                    Label isEmptyArray(env);
                    Label notEmptyArray(env);
                    BRANCH(Int64Equal(sumArrayLen, Int64(0)), &isEmptyArray, &notEmptyArray);
                    Bind(&isEmptyArray);
                    {
                        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
                        result->WriteVariable(newBuilder.CreateEmptyArray(glue));
                        Jump(exit);
                    }
                    Bind(&notEmptyArray);
                    Label notOverFlow(env);
                    BRANCH(Int64GreaterThan(sumArrayLen, maxArrayIndex), slowPath, &notOverFlow);
                    Bind(&notOverFlow);
                    {
                        Label spreadable(env);
                        GateRef isAllConcatSpreadable = LogicAndBuilder(env).And(IsConcatSpreadable(glue, thisValue))
                            .And(IsConcatSpreadable(glue, arg0)).Done();
                        BRANCH(isAllConcatSpreadable, &spreadable, slowPath);
                        Bind(&spreadable);
                        {
                            Label setProperties(env);
                            GateRef globalEnv = GetCurrentGlobalEnv();
                            auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                GlobalEnv::ARRAY_FUNCTION_INDEX);
                            GateRef intialHClass = Load(VariableType::JS_ANY(), glue, arrayFunc,
                                IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                            NewObjectStubBuilder newBuilder(this, globalEnv);
                            newBuilder.SetParameters(glue, 0);
                            GateRef newArray = newBuilder.NewJSArrayWithSize(intialHClass, sumArrayLen);
                            BRANCH(TaggedIsException(newArray), exit, &setProperties);
                            Bind(&setProperties);
                            {
                                GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
                                Store(VariableType::INT32(), glue, newArray, lengthOffset,
                                    TruncInt64ToInt32(sumArrayLen));
                                GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                    ConstantIndex::ARRAY_LENGTH_ACCESSOR);
                                Store(VariableType::JS_ANY(), glue, newArray,
                                    IntPtr(JSArray::GetInlinedPropertyOffset(JSArray::LENGTH_INLINE_PROPERTY_INDEX)),
                                    accessor);
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
                                    BRANCH(Int64LessThan(*i, thisLen), &next, &loopExit);
                                    Bind(&next);
                                    GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, *i);
                                    #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
                                    SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                        Int32(Elements::ToUint(ElementsKind::GENERIC)));
                                    #else
                                    SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                        Int32(Elements::ToUint(ElementsKind::NONE)));
                                    #endif
                                    Jump(&loopEnd);
                                }
                                Bind(&loopEnd);
                                i = Int64Add(*i, Int64(1));
                                j = Int64Add(*j, Int64(1));
                                LoopEndWithCheckSafePoint(&loopHead, env, glue);
                                Bind(&loopExit);
                                Label loopHead1(env);
                                Label loopEnd1(env);
                                Label next1(env);
                                Label loopExit1(env);
                                Jump(&loopHead1);
                                LoopBegin(&loopHead1);
                                {
                                    BRANCH(Int64LessThan(*k, argLen), &next1, &loopExit1);
                                    Bind(&next1);
                                    GateRef ele = GetTaggedValueWithElementsKind(glue, arg0, *k);
                                    #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
                                    SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                        Int32(Elements::ToUint(ElementsKind::GENERIC)));
                                    #else
                                    SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                        Int32(Elements::ToUint(ElementsKind::NONE)));
                                    #endif
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
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
    ForEachOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "ForEach" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
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

    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(thisLen, VariableType::INT32(), Int32(0));
    thisLen = GetArrayLength(thisValue);
    Jump(&thisIsStable);

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
            Label hasException1(env);
            GateRef newLen = GetArrayLength(thisValue);
            BRANCH(BitAnd(IsStableJSArray(glue, thisValue), Int32Equal(*thisLen, newLen)),
                &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH(Int64GreaterThanOrEqual(*i, ZExtInt32ToInt64(*thisLen)), &loopExit, &next);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    { thisValue, IntToTaggedInt(*i) });
                BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
                Bind(&hasProperty);
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &callDispatch);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
            }
            Bind(&callDispatch);
            {
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &loopEnd);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
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
            Label hasException1(env);
            BRANCH(Int64GreaterThanOrEqual(*i, ZExtInt32ToInt64(*thisLen)), &loopExit, &next);
            Bind(&next);
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                { thisValue, IntToTaggedInt(*i) });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
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
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException1, &loopEnd);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(exit);
    }
#endif
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
                iterValue = GetTaggedValueWithElementsKind(glue, array, index);
                Jump(&checkNeedCreateEntry);
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
#if ENABLE_NEXT_OPTIMIZATION
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = false;
    return IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
#else
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if: (1) this is an empty array; (2) fromIndex is missing
    JsArrayRequirements req;
    BRANCH(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    {
        Label atMostOneArg(env);
        BRANCH(Int32LessThanOrEqual(TruncPtrToInt32(numArgs), Int32(1)), &atMostOneArg, slowPath);
        // Returns -1 on fast path
        Bind(&atMostOneArg);
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
#endif
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::LastIndexOf([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = true;
    return IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
#else
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if: (1) this is an empty array; (2) fromIndex is missing
    JsArrayRequirements req;
    BRANCH(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    {
        Label atMostOneArg(env);
        BRANCH(Int32LessThanOrEqual(TruncPtrToInt32(numArgs), Int32(1)), &atMostOneArg, slowPath);
        // Returns -1 on fast path
        Bind(&atMostOneArg);
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
#endif
}

void BuiltinsArrayStubBuilder::Pop(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    PopOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label stableJSArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    // don't check constructor, "Pop" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &stableJSArray, slowPath);
    Bind(&stableJSArray);

    Label isLengthWritable(env);
    BRANCH(IsArrayLengthWritable(glue, thisValue), &isLengthWritable, slowPath);
    Bind(&isLengthWritable);
    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));

    Label notZeroLen(env);
    BRANCH(Int64Equal(thisLen, Int64(0)), exit, &notZeroLen);
    Bind(&notZeroLen);
    Label isJsCOWArray(env);
    Label getElements(env);
    BRANCH(IsJsCOWArray(glue, thisValue), &isJsCOWArray, &getElements);
    Bind(&isJsCOWArray);
    {
        CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(CheckAndCopyArray), { thisValue });
        Jump(&getElements);
    }
    Bind(&getElements);
    GateRef elements = GetElementsArray(glue, thisValue);
    GateRef capacity = ZExtInt32ToInt64(GetLengthOfTaggedArray(elements));
    GateRef index = Int64Sub(thisLen, Int64(1));

    Label inRange(env);
    Label trimCheck(env);
    Label noTrimCheck(env);
    Label setNewLen(env);
    Label isHole(env);
    DEFVARIABLE(element, VariableType::JS_ANY(), Hole());
    BRANCH(Int64LessThan(index, capacity), &inRange, &trimCheck);
    Bind(&inRange);
    {
        element = GetTaggedValueWithElementsKind(glue, thisValue, index);
        Jump(&isHole);
    }
    Bind(&isHole);
    BRANCH(TaggedIsHole(*element), &noTrimCheck, &trimCheck);
    Bind(&noTrimCheck);
    {
        Label hasException0(env);
        Label notHasException0(env);
        element = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(index), ProfileOperation());
        BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
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
    BRANCH(Int64GreaterThan(unused, Int64(TaggedArray::MAX_END_UNUSED)), &needTrim, &noTrim);
    Bind(&needTrim);
    {
        CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, elements, index});
        Jump(&setNewLen);
    }
    Bind(&noTrim);
    {
        SetValueWithElementsKind(glue, thisValue, Hole(), index, Boolean(false),
                                 Int32(Elements::ToUint(ElementsKind::NONE)));
        Jump(&setNewLen);
    }
    Bind(&setNewLen);
    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, thisValue, lengthOffset, TruncInt64ToInt32(index));

    Label isNotHole(env);
    BRANCH(TaggedIsHole(*element), exit, &isNotHole);
    Bind(&isNotHole);
    {
        result->WriteVariable(*element);
        Jump(exit);
    }
#endif
}

void BuiltinsArrayStubBuilder::Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    SliceOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label noConstructor(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // need check constructor, "Slice" should use ArraySpeciesCreate
    BRANCH(HasConstructor(glue, thisValue), slowPath, &noConstructor);
    Bind(&noConstructor);

    Label thisIsEmpty(env);
    Label thisNotEmpty(env);
    // Fast path if:
    // (1) this is an empty array with constructor not reset (see ArraySpeciesCreate for details);
    // (2) no arguments exist
    JsArrayRequirements req;
    req.defaultConstructor = true;
    BRANCH(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, &thisNotEmpty);
    Bind(&thisIsEmpty);
    {
        Label noArgs(env);
        GateRef numArgsAsInt32 = TruncPtrToInt32(numArgs);
        BRANCH(Int32Equal(numArgsAsInt32, Int32(0)), &noArgs, slowPath);
        // Creates a new empty array on fast path
        Bind(&noArgs);
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        result->WriteVariable(newBuilder.CreateEmptyArray(glue));
        Jump(exit);
    }
    Bind(&thisNotEmpty);
    {
        Label stableJSArray(env);
        Label arrayLenNotZero(env);

        GateRef isThisStableJSArray = IsStableJSArray(glue, thisValue);
        BRANCH(isThisStableJSArray, &stableJSArray, slowPath);
        Bind(&stableJSArray);

        GateRef msg0 = GetCallArg0(numArgs);
        GateRef msg1 = GetCallArg1(numArgs);
        GateRef thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
        Label msg0Int(env);
        BRANCH(TaggedIsInt(msg0), &msg0Int, slowPath);
        Bind(&msg0Int);
        DEFVARIABLE(start, VariableType::INT64(), Int64(0));
        DEFVARIABLE(end, VariableType::INT64(), thisArrLen);

        GateRef argStart = SExtInt32ToInt64(TaggedGetInt(msg0));
        Label arg0LessZero(env);
        Label arg0NotLessZero(env);
        Label startDone(env);
        BRANCH(Int64LessThan(argStart, Int64(0)), &arg0LessZero, &arg0NotLessZero);
        Bind(&arg0LessZero);
        {
            Label tempGreaterZero(env);
            Label tempNotGreaterZero(env);
            GateRef tempStart = Int64Add(argStart, thisArrLen);
            BRANCH(Int64GreaterThan(tempStart, Int64(0)), &tempGreaterZero, &tempNotGreaterZero);
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
            BRANCH(Int64LessThan(argStart, thisArrLen), &argLessLen, &argNotLessLen);
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
            BRANCH(TaggedIsUndefined(msg1), &endDone, &msg1Def);
            Bind(&msg1Def);
            {
                Label msg1Int(env);
                BRANCH(TaggedIsInt(msg1), &msg1Int, slowPath);
                Bind(&msg1Int);
                {
                    GateRef argEnd = SExtInt32ToInt64(TaggedGetInt(msg1));
                    Label arg1LessZero(env);
                    Label arg1NotLessZero(env);
                    BRANCH(Int64LessThan(argEnd, Int64(0)), &arg1LessZero, &arg1NotLessZero);
                    Bind(&arg1LessZero);
                    {
                        Label tempGreaterZero(env);
                        Label tempNotGreaterZero(env);
                        GateRef tempEnd = Int64Add(argEnd, thisArrLen);
                        BRANCH(Int64GreaterThan(tempEnd, Int64(0)), &tempGreaterZero, &tempNotGreaterZero);
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
                        BRANCH(Int64LessThan(argEnd, thisArrLen), &argLessLen, &argNotLessLen);
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
                BRANCH(Int64LessThan(tempCnt, Int64(0)), &tempCntDone, &tempCntGreaterOrEqualZero);
                Bind(&tempCntGreaterOrEqualZero);
                {
                    count = tempCnt;
                    Jump(&tempCntDone);
                }
                Bind(&tempCntDone);
                {
                    Label notOverFlow(env);
                    BRANCH(Int64GreaterThan(*count, Int64(JSObject::MAX_GAP)), slowPath, &notOverFlow);
                    Bind(&notOverFlow);
                    {
                        GateRef newArray = NewArray(glue, *count);
                        GateRef thisEles = GetElementsArray(glue, thisValue);
                        GateRef thisElesLen = ZExtInt32ToInt64(GetLengthOfTaggedArray(thisEles));

                        Label inThisEles(env);
                        Label outThisEles(env);
                        BRANCH(Int64GreaterThan(thisElesLen, Int64Add(*start, *count)), &inThisEles, &outThisEles);
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
                                BRANCH(Int64LessThan(*idx, *count), &next, &loopExit);
                                Bind(&next);

                                GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, Int64Add(*idx, *start));
                                SetValueWithElementsKind(glue, newArray, ele, *idx, Boolean(true),
                                                         Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&loopEnd);
                            idx = Int64Add(*idx, Int64(1));
                            LoopEndWithCheckSafePoint(&loopHead, env, glue);
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
                                BRANCH(Int64LessThan(*idx, *count), &next, &loopExit);
                                Bind(&next);
                                GateRef index = Int64Add(*idx, *start);
                                DEFVARIABLE(ele, VariableType::JS_ANY(), Hole());

                                Label indexOutRange(env);
                                Label indexInRange(env);
                                Label setEle(env);
                                BRANCH(Int64GreaterThan(thisElesLen, index), &indexInRange, &indexOutRange);
                                Bind(&indexInRange);
                                {
                                    ele = GetTaggedValueWithElementsKind(glue, thisValue, index);
                                    Jump(&setEle);
                                }
                                Bind(&indexOutRange);
                                {
                                    ele = Hole();
                                    Jump(&setEle);
                                }
                                Bind(&setEle);
                                SetValueWithElementsKind(glue, newArray, *ele, *idx, Boolean(true),
                                                         Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&loopEnd);
                            idx = Int64Add(*idx, Int64(1));
                            LoopEndWithCheckSafePoint(&loopHead, env, glue);
                            Bind(&loopExit);
                            result->WriteVariable(newArray);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
#endif
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
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    Label argUndefined(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
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
#if ENABLE_NEXT_OPTIMIZATION
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
#else
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    GateRef len = ZExtInt32ToInt64(GetArrayLength(receiver));
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
        BRANCH(Int64LessThan(*i, len), &next, &loopExit);
        Bind(&next);
        DEFVARIABLE(beginIndex, VariableType::INT64(), Int64(0));
        DEFVARIABLE(endIndex, VariableType::INT64(), *i);
        Label presentValueIsHole(env);
        Label afterGettingpresentValue(env);
        Label presentValueHasProperty(env);
        Label presentValueHasException0(env);
        presentValue = GetTaggedValueWithElementsKind(glue, receiver, *i);
        BRANCH(TaggedIsHole(*presentValue), &presentValueIsHole, &afterGettingpresentValue);
        Bind(&presentValueIsHole);
        {
            GateRef presentValueHasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                RTSTUB_ID(HasProperty), { receiver, IntToTaggedInt(*i) });
            BRANCH(TaggedIsTrue(presentValueHasProp), &presentValueHasProperty, &afterGettingpresentValue);
            Bind(&presentValueHasProperty);
            {
                presentValue = FastGetPropertyByIndex(glue, receiver, TruncInt64ToInt32(*i), ProfileOperation(), hir);
                BRANCH(HasPendingException(glue), &presentValueHasException0, &afterGettingpresentValue);
                Bind(&presentValueHasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
            }
        }
        Bind(&afterGettingpresentValue);
        {
            Label loopHead1(env);
            Label loopEnd1(env);
            Label next1(env);
            Label loopExit1(env);
            Jump(&loopHead1);
            LoopBegin(&loopHead1);
            {
                Label middleValueIsHole(env);
                Label afterGettingmiddleValue(env);
                Label middleValueHasProperty(env);
                Label middleValueHasException0(env);
                BRANCH(Int64LessThan(*beginIndex, *endIndex), &next1, &loopExit1);
                Bind(&next1);
                GateRef sum = Int64Add(*beginIndex, *endIndex);
                GateRef middleIndex = Int64Div(sum, Int64(2)); // 2 : half
                middleValue = GetTaggedValueWithElementsKind(glue, receiver, middleIndex);
                BRANCH(TaggedIsHole(*middleValue), &middleValueIsHole, &afterGettingmiddleValue);
                Bind(&middleValueIsHole);
                {
                    GateRef middleValueHasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(HasProperty), { receiver, IntToTaggedInt(middleIndex) });
                    BRANCH(TaggedIsTrue(middleValueHasProp), &middleValueHasProperty, &afterGettingmiddleValue);
                    Bind(&middleValueHasProperty);
                    {
                        middleValue = FastGetPropertyByIndex(glue, receiver,
                            TruncInt64ToInt32(middleIndex), ProfileOperation(), hir);
                        BRANCH(HasPendingException(glue), &middleValueHasException0, &afterGettingmiddleValue);
                        Bind(&middleValueHasException0);
                        {
                            result->WriteVariable(Exception());
                            Jump(exit);
                        }
                    }
                }
                Bind(&afterGettingmiddleValue);
                {
                    Label isInt(env);
                    GateRef middleVal = *middleValue;
                    GateRef presentVal = *presentValue;
                    BRANCH(LogicAndBuilder(env).And(TaggedIsInt(middleVal)).And(TaggedIsInt(presentVal)).Done(),
                        &isInt, slowPath);
                    Bind(&isInt);
                    {
                        GateRef compareResult =
                            CallNGCRuntime(glue, RTSTUB_ID(IntLexicographicCompare), {*middleValue, *presentValue});
                        Label less0(env);
                        Label greater0(env);
                        BRANCH(Int32LessThanOrEqual(compareResult, Int32(0)), &less0, &greater0);
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
            }
            Bind(&loopEnd1);
            LoopEnd(&loopHead1);
            Bind(&loopExit1);

            Label shouldCopy(env);
            GateRef isGreater0 = Int64GreaterThanOrEqual(*endIndex, Int64(0));
            GateRef lessI = Int64LessThan(*endIndex, *i);
            BRANCH(BitAnd(isGreater0, lessI), &shouldCopy, &loopEnd);
            Bind(&shouldCopy);
            {
                DEFVARIABLE(j, VariableType::INT64(), *i);
                Label loopHead2(env);
                Label loopEnd2(env);
                Label next2(env);
                Label loopExit2(env);
                Jump(&loopHead2);
                LoopBegin(&loopHead2);
                {
                    Label previousValueIsHole(env);
                    Label afterGettingpreviousValue(env);
                    Label previousValueHasProperty(env);
                    Label previousValueHasException0(env);
                    BRANCH(Int64GreaterThan(*j, *endIndex), &next2, &loopExit2);
                    Bind(&next2);
                    previousValue = GetTaggedValueWithElementsKind(glue, receiver, Int64Sub(*j, Int64(1)));
                    BRANCH(TaggedIsHole(*previousValue), &previousValueIsHole, &afterGettingpreviousValue);
                    Bind(&previousValueIsHole);
                    {
                        GateRef previousValueHasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                            RTSTUB_ID(HasProperty), { receiver, IntToTaggedInt(Int64Sub(*j, Int64(1))) });
                        BRANCH(TaggedIsTrue(previousValueHasProp),
                            &previousValueHasProperty, &afterGettingpreviousValue);
                        Bind(&previousValueHasProperty);
                        {
                            previousValue = FastGetPropertyByIndex(glue, receiver,
                                TruncInt64ToInt32(Int64Sub(*j, Int64(1))), ProfileOperation(), hir);
                            BRANCH(HasPendingException(glue), &previousValueHasException0, &afterGettingpreviousValue);
                            Bind(&previousValueHasException0);
                            {
                                result->WriteVariable(Exception());
                                Jump(exit);
                            }
                        }
                    }
                    Bind(&afterGettingpreviousValue);
                    {
                        SetValueWithElementsKind(glue, receiver, *previousValue, *j, Boolean(isToSorted),
                            Int32(Elements::ToUint(ElementsKind::NONE)));
                        Jump(&loopEnd2);
                    }
                }
                Bind(&loopEnd2);
                j = Int64Sub(*j, Int64(1));
                LoopEnd(&loopHead2);
                Bind(&loopExit2);
                SetValueWithElementsKind(glue, receiver, *presentValue, *endIndex, Boolean(isToSorted),
                    Int32(Elements::ToUint(ElementsKind::NONE)));
                Jump(&loopEnd);
            }
        }
    }
    Bind(&loopEnd);
    i = Int64Add(*i, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    env->SubCfgExit();
    return receiver;
#endif
}

void BuiltinsArrayStubBuilder::Reduce(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisLen, VariableType::INT32(), Int32(0));
    Label isHeapObject(env);
    Label isJsArray(env);
    Label atLeastOneArg(env);
    Label callbackFnHandleHeapObject(env);
    Label callbackFnHandleCallable(env);
    Label noTypeError(env);

    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Reduce" won't create new array.
    thisLen = GetArrayLength(thisValue);
    BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
    Bind(&atLeastOneArg);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &callbackFnHandleHeapObject, slowPath);
    Bind(&callbackFnHandleHeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callbackFnHandleCallable, slowPath);
    Bind(&callbackFnHandleCallable);
    GateRef thisLenIsZero = Int32Equal(*thisLen, Int32(0));
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
                Label loopEnd(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH(Int32LessThan(*k, *thisLen), &next, &loopExit);
                    Bind(&next);
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
                            Label hasException1(env);
                            Label notHasException1(env);
                            BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                            Bind(&hasException1);
                            {
                                result->WriteVariable(Exception());
                                Jump(exit);
                            }
                            Bind(&notHasException1);
                            GateRef elements = GetElementsArray(glue, thisValue);
                            GateRef newLen = GetLengthOfTaggedArray(elements);
                            BRANCH(Int32LessThan(newLen, *thisLen), &changeThisLen, &updateCallResult);
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
                Label finish(env);
                Label callRT(env);
                BRANCH(Int32LessThan(*k, *thisLen), &callRT, &finish);
                Bind(&callRT);
                {
                    accumulator = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(JSArrayReduceUnStable), { thisValue, thisValue, IntToTaggedInt(*k),
                            IntToTaggedInt(*thisLen), *accumulator, callbackFnHandle });
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

void BuiltinsArrayStubBuilder::Reverse(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Reverse" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef thisLen = GetArrayLength(thisValue);
    GateRef thisArrLen = ZExtInt32ToInt64(thisLen);
#if ENABLE_NEXT_OPTIMIZATION
    Label useReversBarrier(env);
    Label noReverseBarrier(env);
    // 10 : length < 10 reverse item by item
    BRANCH(Int32LessThan(thisLen, Int32(10)), &noReverseBarrier, &useReversBarrier);
    Bind(&useReversBarrier);
    ReverseOptimised(glue, thisValue, thisLen, result, exit);
    Bind(&noReverseBarrier);
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
    ToReversedOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "ToReversed" always use ArrayCreate to create array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef thisArrLen = GetArrayLength(thisValue);
    GateRef receiver = NewArray(glue, Int32(0));
    GrowElementsCapacity(glue, receiver, thisArrLen);
    SetArrayLength(glue, receiver, thisArrLen);
    result->WriteVariable(DoReverse(glue, thisValue, receiver, Boolean(true), result, exit));
    Jump(exit);
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
    FindOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Find" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef callbackFnHandle = GetCallArg0(numArgs);
    Label arg0HeapObject(env);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    Label callable(env);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
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
        BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
        Bind(&next);
        GateRef kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
        BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
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
            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
            callArgs.callThisArg3WithReturnArgs = { argHandle, kValue, key, thisValue };
            CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                Circuit::NullGate(), callArgs);
            GateRef retValue = callBuilder.JSCallDispatch();
            BRANCH(HasPendingException(glue), &hasException, &notHasException);
            Bind(&hasException);
            {
                result->WriteVariable(retValue);
                Jump(exit);
            }
            Bind(&notHasException);
            {
                Label find(env);
                BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &loopEnd);
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
    LoopEndWithCheckSafePoint(&loopHead, env, glue);
    Bind(&loopExit);
    Jump(exit);
#endif
}

void BuiltinsArrayStubBuilder::FindIndex(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    FindIndexOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Shift" won't create new array.
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    Label arg0HeapObject(env);
    Label callable(env);
    Label stableJSArray(env);
    Label notStableJSArray(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    result->WriteVariable(IntToTaggedPtr(Int32(-1)));
    GateRef argHandle = GetCallArg1(numArgs);
    DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    BRANCH(IsStableJSArray(glue, thisValue), &stableJSArray, &notStableJSArray);
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
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            Label isHole(env);
            Label notHole(env);
            BRANCH(TaggedIsHole(*kValue), &isHole, &notHole);
            Bind(&isHole);
            {
                Label hasException0(env);
                Label notHasException0(env);
                GateRef res = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    Label resIsHole(env);
                    Label resNotHole(env);
                    BRANCH(TaggedIsHole(res), &resIsHole, &resNotHole);
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
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH(HasPendingException(glue), &hasException, &notHasException);
                Bind(&hasException);
                {
                    result->WriteVariable(retValue);
                    Jump(exit);
                }
                Bind(&notHasException);
                {
                    Label find(env);
                    BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &checkStable);
                    Bind(&find);
                    {
                        result->WriteVariable(key);
                        Jump(exit);
                    }
                }
                Bind(&checkStable);
                i = Int64Add(*i, Int64(1));
                BRANCH(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
            }
        }
        Bind(&loopEnd);
        thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
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
            BRANCH(Int64LessThan(*j, *thisArrLen), &next, &loopExit);
            Bind(&next);
            {
                Label hasException0(env);
                Label notHasException0(env);
                GateRef kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*j), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
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
                    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                    callArgs.callThisArg3WithReturnArgs = { argHandle, kValue, key, thisValue };
                    CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                        nullptr, Circuit::NullGate(), callArgs);
                    GateRef retValue = callBuilder.JSCallDispatch();
                    BRANCH(TaggedIsException(retValue), &hasException, &notHasException);
                    Bind(&hasException);
                    {
                        result->WriteVariable(retValue);
                        Jump(exit);
                    }
                    Bind(&notHasException);
                    {
                        Label find(env);
                        BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &find, &loopEnd);
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
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
        Bind(&loopExit);
        Jump(exit);
    }
#endif
}

#if ENABLE_NEXT_OPTIMIZATION
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
#else
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

    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    Label isLengthWritable(env);
    BRANCH(IsArrayLengthWritable(glue, thisValue), &isLengthWritable, slowPath);
    Bind(&isLengthWritable);

    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);

    GateRef oldLength = GetArrayLength(thisValue);
    *result = IntToTaggedPtr(oldLength);

    BRANCH(Int32Equal(ChangeIntPtrToInt32(numArgs), Int32(0)), exit, &checkSmallArgs);
    Bind(&checkSmallArgs);
    // now unsupport more than 2 args
    BRANCH(Int32LessThanOrEqual(ChangeIntPtrToInt32(numArgs), Int32(2)), &smallArgs, slowPath);
    Bind(&smallArgs);
    GateRef newLength = Int32Add(oldLength, ChangeIntPtrToInt32(numArgs));

    DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
    GateRef capacity = GetLengthOfTaggedArray(*elements);
    Label grow(env);
    Label setValue(env);
    BRANCH(Int32GreaterThan(newLength, capacity), &grow, &setValue);
    Bind(&grow);
    {
        elements = GrowElementsCapacity(glue, thisValue, newLength);
        Jump(&setValue);
    }
    Bind(&setValue);
    {
        Label oneArg(env);
        Label twoArg(env);
        DEFVARIABLE(index, VariableType::INT32(), Int32(0));
        DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
        BRANCH(Int64Equal(numArgs, IntPtr(1)), &oneArg, &twoArg);  // 1 one arg
        Bind(&oneArg);
        {
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0));  // 0 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            Jump(&setLength);
        }
        Bind(&twoArg);
        {
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0));  // 0 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            value = GetCallArg1(numArgs);
            index = Int32Add(oldLength, Int32(1));  // 1 slot index
            SetValueWithElementsKind(glue, thisValue, *value, *index, Boolean(true),
                                     Int32(Elements::ToUint(ElementsKind::NONE)));
            Jump(&setLength);
        }
    }
    Bind(&setLength);
    SetArrayLength(glue, thisValue, newLength);
    result->WriteVariable(IntToTaggedPtr(newLength));
    Jump(exit);
}
#endif

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
            GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ISCONCAT_SYMBOL_INDEX);
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
#if ENABLE_NEXT_OPTIMIZATION
    IndexOfOptions options;
    options.compType = ComparisonType::SAME_VALUE_ZERO;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_OR_NOT;
    options.holeAsUndefined = true;
    options.reversedOrder = false;

    IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
#else
    auto env = GetEnvironment();
    Label isDictMode(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStableJsArray(env);
    Label notFound(env);
    Label thisLenNotZero(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(IsStableJSArray(glue, thisValue), &isStableJsArray, slowPath);
    Bind(&isStableJsArray);
    GateRef thisLen = GetArrayLength(thisValue);
    BRANCH(Int32Equal(thisLen, Int32(0)), &notFound, &thisLenNotZero);
    Bind(&thisLenNotZero);
    {
        DEFVARIABLE(fromIndex, VariableType::INT32(), Int32(0));
        Label getArgTwo(env);
        Label nextProcess(env);
        BRANCH(Int64Equal(numArgs, IntPtr(2)), &getArgTwo, &nextProcess); // 2: 2 parameters
        Bind(&getArgTwo);
        {
            Label secondArgIsInt(env);
            GateRef fromIndexTemp = GetCallArg1(numArgs);
            BRANCH(TaggedIsInt(fromIndexTemp), &secondArgIsInt, slowPath);
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
            BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
            Bind(&atLeastOneArg);
            BRANCH(Int32GreaterThanOrEqual(*fromIndex, thisLen), &notFound, &nextCheck);
            Bind(&nextCheck);
            {
                GateRef negThisLen = Int32Sub(Int32(0), thisLen);
                BRANCH(Int32LessThan(*fromIndex, negThisLen), &setBackZero, &calculateFrom);
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
                    BRANCH(Int32GreaterThanOrEqual(*fromIndex, Int32(0)),
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
                        BRANCH(Int32GreaterThanOrEqual(lenFromIndexSum, Int32(0)), &isLenFromIndex, &startLoop);
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
                            BRANCH(Int32LessThan(*from, thisLen), &next, &loopExit);
                            Bind(&next);
                            {
                                Label notHoleOrUndefValue(env);
                                Label valueFound(env);
                                GateRef value = GetTaggedValueWithElementsKind(glue, thisValue, *from);
                                GateRef isHole = TaggedIsHole(value);
                                GateRef isUndef = TaggedIsUndefined(value);
                                BRANCH(BitOr(isHole, isUndef), slowPath, &notHoleOrUndefValue);
                                Bind(&notHoleOrUndefValue);
                                GateRef valueEqual = StubBuilder::SameValueZero(glue, searchElement, value);
                                BRANCH(valueEqual, &valueFound, &loopEnd);
                                Bind(&valueFound);
                                {
                                    result->WriteVariable(TaggedTrue());
                                    Jump(exit);
                                }
                            }
                        }
                        Bind(&loopEnd);
                        from = Int32Add(*from, Int32(1));
                        LoopEndWithCheckSafePoint(&loopHead, env, glue);
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
#endif
}

void BuiltinsArrayStubBuilder::From(GateRef glue, [[maybe_unused]] GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef item = GetCallArg0(numArgs);
    Label stringItem(env);
    BRANCH(TaggedIsString(glue, item), &stringItem, slowPath);
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
#if ENABLE_NEXT_OPTIMIZATION
    FillOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Label notCOWArray(env);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef arrayCls = LoadHClass(glue, thisValue);
    // 1. Let O be ToObject(this value).
    // 2 ReturnIfAbrupt(O).
    Label hasException(env);
    Label proNotCOWArray(env);
    GateRef prop = GetPropertiesFromJSObject(glue, thisValue);
    BRANCH(IsCOWArray(glue, prop), slowPath, &proNotCOWArray);
    Bind(&proNotCOWArray);
    // 3. Let len be ToLength(Get(O,"length")).
    GateRef value = GetCallArg0(numArgs);
    GateRef thisArrLen = GetLengthOfJSArray(thisValue);
    Label isDict(env);
    Label notDict(env);
    BRANCH(IsDictionaryElement(arrayCls), &isDict, &notDict);
    Bind(&isDict);
    {
        GateRef size = GetNumberOfElements(glue, thisValue);
        BRANCH(Int32GreaterThan(Int32Sub(thisArrLen, size),
            TruncInt64ToInt32(IntPtr(JSObject::MAX_GAP))), slowPath, &notDict);
    }
    Bind(&notDict);
    // 5. let relativeStart be ToInteger(start).
    GateRef startArg = GetCallArg1(numArgs);
    // 6 ReturnIfAbrupt(relativeStart).
    GateRef argStart = NumberGetInt(glue, ToNumber(glue, startArg));
    Label notHasException3(env);
    BRANCH(HasPendingException(glue), &hasException, &notHasException3);
    Bind(&notHasException3);
    // 7. If relativeStart < 0, let k be max((len + relativeStart),0); else let k be min(relativeStart, len).
    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    Label maxStart(env);
    Label minStart(env);
    Label startExit(env);
    BRANCH(Int32LessThan(argStart, Int32(0)), &maxStart, &minStart);
    Bind(&maxStart);
    {
        GateRef tempStart = Int32Add(argStart, thisArrLen);
        Label bind1(env);
        BRANCH(Int32GreaterThan(tempStart, Int32(0)), &bind1, &startExit);
        Bind(&bind1);
        {
            start = tempStart;
            Jump(&startExit);
        }
    }
    Bind(&minStart);
    {
        Label bind1(env);
        Label bind2(env);
        BRANCH(Int32LessThan(argStart, thisArrLen), &bind1, &bind2);
        Bind(&bind1);
        {
            start = argStart;
            Jump(&startExit);
        }
        Bind(&bind2);
        {
            start = thisArrLen;
            Jump(&startExit);
        }
    }
    Bind(&startExit);
    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    GateRef endArg = GetCallArg2(numArgs);
    DEFVARIABLE(argEnd, VariableType::INT32(), Int32(0));
    Label endArgIsUndefined(env);
    Label endArgNotUndefined(env);
    Label next1(env);
    BRANCH(TaggedIsUndefined(endArg), &endArgIsUndefined, &endArgNotUndefined);
    Bind(&endArgIsUndefined);
    {
        argEnd = thisArrLen;
        Jump(&next1);
    }
    Bind(&endArgNotUndefined);
    {
        argEnd = NumberGetInt(glue, ToNumber(glue, endArg));
        // 9. ReturnIfAbrupt(relativeEnd).
        BRANCH(HasPendingException(glue), &hasException, &next1);
    }
    Bind(&next1);

    // 10. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    DEFVARIABLE(end, VariableType::INT32(), Int32(0));
    Label maxEnd(env);
    Label minEnd(env);
    Label endExit(env);
    BRANCH(Int32LessThan(*argEnd, Int32(0)), &maxEnd, &minEnd);
    Bind(&maxEnd);
    {
        GateRef tempEnd = Int32Add(*argEnd, thisArrLen);
        Label bind1(env);
        Label bind2(env);
        BRANCH(Int32GreaterThan(tempEnd, Int32(0)), &bind1, &endExit);
        Bind(&bind1);
        {
            end = tempEnd;
            Jump(&endExit);
        }
    }
    Bind(&minEnd);
    {
        Label bind1(env);
        Label bind2(env);
        BRANCH(Int32LessThan(*argEnd, thisArrLen), &bind1, &bind2);
        Bind(&bind1);
        {
            end = *argEnd;
            Jump(&endExit);
        }
        Bind(&bind2);
        {
            end = thisArrLen;
            Jump(&endExit);
        }
    }
    Bind(&endExit);
    {
        Label newElements(env);
        Label defaultElements(env);
        Label startFill(env);
        GateRef elementKind = GetElementsKindFromHClass(arrayCls);
        TransitToElementsKind(glue, thisValue, value, elementKind);
        DEFVARIABLE(migratedValue, VariableType::JS_ANY(), value);
        DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
        GateRef mutant = IsMutantTaggedArray(glue, *elements);
        GateRef elementLen = GetLengthOfTaggedArray(*elements);
        BRANCH(Int32GreaterThanOrEqual(elementLen, *end), &defaultElements, &newElements);
        Bind(&defaultElements);
        {
            Label isMutant(env);
            BRANCH(mutant, &isMutant, &startFill);
            Bind(&isMutant);
            {
                migratedValue = ConvertTaggedValueWithElementsKind(glue, value, elementKind);
                Jump(&startFill);
            }
        }
        Bind(&newElements);
        {
            Label isMutant(env);
            Label notMutant(env);
            NewObjectStubBuilder newBuilder(this);
            BRANCH(mutant, &isMutant, &notMutant);
            Bind(&isMutant);
            {
                elements = newBuilder.NewMutantTaggedArray(glue, elementLen);
                migratedValue = ConvertTaggedValueWithElementsKind(glue, value, elementKind);
                Jump(&startFill);
            }
            Bind(&notMutant);
            {
                elements = newBuilder.NewTaggedArray(glue, elementLen);
                Jump(&startFill);
            }
        }
        Bind(&startFill);
        Label noBarrier(env);
        Label needBarrier(env);
        Label needRevise(env);
        Label noRevise(env);
        Label barrierExit(env);
        BRANCH(mutant, &noBarrier, &needBarrier);
        Bind(&noBarrier);
        {
            DEFVARIABLE(idx, VariableType::INT32(), *start);
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32LessThan(*idx, *end), &next, &loopExit);
                Bind(&next);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue,
                    *elements, *idx, *migratedValue, MemoryAttribute::NoBarrier());
                Jump(&loopEnd);
            }
            Bind(&loopEnd);
            idx = Int32Add(*idx, Int32(1));
            LoopEnd(&loopHead);
            Bind(&loopExit);
            Jump(&barrierExit);
        }
        Bind(&needBarrier);
        {
            DEFVARIABLE(idx, VariableType::INT32(), *start);
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32LessThan(*idx, *end), &next, &loopExit);
                Bind(&next);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *idx, *migratedValue);
                Jump(&loopEnd);
            }
            Bind(&loopEnd);
            idx = Int32Add(*idx, Int32(1));
            LoopEnd(&loopHead);
            Bind(&loopExit);
            Jump(&barrierExit);
        }
        Bind(&barrierExit);
        SetElementsArray(VariableType::JS_POINTER(), glue, thisValue, *elements);
        GateRef arrLen = GetLengthOfJSArray(thisValue);
        BRANCH(Int32LessThan(arrLen, *end), &needRevise, &noRevise);
        Bind(&needRevise);
        {
            SetArrayLength(glue, thisValue, *end);
            Jump(&noRevise);
        }
        Bind(&noRevise);
        result->WriteVariable(thisValue);
        Jump(exit);
    }
    Bind(&hasException);
    {
        result->WriteVariable(Exception());
        Jump(exit);
    }
#endif
}

void BuiltinsArrayStubBuilder::Splice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label defaultConstr(env);
    Label isGeneric(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // need check constructor, "Splice" should use ArraySpeciesCreate
    BRANCH(HasConstructor(glue, thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
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
#if ENABLE_NEXT_OPTIMIZATION
    ToSplicedOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label isGeneric(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "ToSpliced" always use ArrayCreate to create array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Label notCOWArray(env);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef thisLen = GetArrayLength(thisValue);
    Label lessThreeArg(env);
    DEFVARIABLE(actualStart, VariableType::INT32(), Int32(0));
    DEFVARIABLE(actualDeleteCount, VariableType::INT32(), Int32(0));
    DEFVARIABLE(newLen, VariableType::INT32(), Int32(0));
    DEFVARIABLE(insertCount, VariableType::INT32(), Int32(0));
    GateRef argc = ChangeIntPtrToInt32(numArgs);
    // 3: max arg count
    BRANCH(Int32LessThanOrEqual(argc, Int32(3)), &lessThreeArg, slowPath);
    Bind(&lessThreeArg);
    {
        Label checkOverFlow(env);
        Label greaterZero(env);
        Label greaterOne(env);
        Label checkGreaterOne(env);
        Label notOverFlow(env);
        Label copyAfter(env);
        // 0: judge the first arg exists
        BRANCH(Int32GreaterThan(argc, Int32(0)), &greaterZero, &checkGreaterOne);
        Bind(&greaterZero);
        {
            GateRef taggedStart = GetCallArg0(numArgs);
            Label taggedStartInt(env);
            BRANCH(TaggedIsInt(taggedStart), &taggedStartInt, slowPath);
            Bind(&taggedStartInt);
            {
                GateRef intStart = GetInt32OfTInt(taggedStart);
                actualStart = CalArrayRelativePos(intStart, thisLen);
                actualDeleteCount = Int32Sub(thisLen, *actualStart);
                Jump(&checkGreaterOne);
            }
        }
        Bind(&checkGreaterOne);
        {
            // 1: judge the second arg exists
            BRANCH(Int32GreaterThan(argc, Int32(1)), &greaterOne, &checkOverFlow);
            Bind(&greaterOne);
            {
                // 2: arg count which is not an item
                insertCount = Int32Sub(argc, Int32(2));
                GateRef argDeleteCount = GetCallArg1(numArgs);
                Label argDeleteCountInt(env);
                BRANCH(TaggedIsInt(argDeleteCount), &argDeleteCountInt, slowPath);
                Bind(&argDeleteCountInt);
                {
                    DEFVARIABLE(deleteCount, VariableType::INT32(), TaggedGetInt(argDeleteCount));
                    Label deleteCountLessZero(env);
                    Label calActualDeleteCount(env);
                    BRANCH(Int32LessThan(*deleteCount, Int32(0)), &deleteCountLessZero, &calActualDeleteCount);
                    Bind(&deleteCountLessZero);
                    {
                        deleteCount = Int32(0);
                        Jump(&calActualDeleteCount);
                    }
                    Bind(&calActualDeleteCount);
                    {
                        actualDeleteCount = *deleteCount;
                        Label lessArrayLen(env);
                        BRANCH(Int32LessThan(Int32Sub(thisLen, *actualStart), *deleteCount),
                            &lessArrayLen, &checkOverFlow);
                        Bind(&lessArrayLen);
                        {
                            actualDeleteCount = Int32Sub(thisLen, *actualStart);
                            Jump(&checkOverFlow);
                        }
                    }
                }
            }
            Bind(&checkOverFlow);
            {
                newLen = Int32Add(Int32Sub(thisLen, *actualDeleteCount), *insertCount);
                BRANCH(Int64GreaterThan(ZExtInt32ToInt64(*newLen), Int64(base::MAX_SAFE_INTEGER)),
                    slowPath, &notOverFlow);
                Bind(&notOverFlow);
                Label newLenEmpty(env);
                Label newLenNotEmpty(env);
                BRANCH(Int32Equal(*newLen, Int32(0)), &newLenEmpty, &newLenNotEmpty);
                Bind(&newLenEmpty);
                {
                    NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
                    result->WriteVariable(newBuilder.CreateEmptyArray(glue));
                    Jump(exit);
                }
                Bind(&newLenNotEmpty);
                {
                    Label copyBefore(env);
                    Label insertArg(env);
                    GateRef newArray = NewArray(glue, Int32(0));
                    GrowElementsCapacity(glue, newArray, *newLen);
                    DEFVARIABLE(oldIndex, VariableType::INT32(), Int32(0));
                    DEFVARIABLE(newIndex, VariableType::INT32(), Int32(0));
                    BRANCH(Int32GreaterThan(*actualStart, Int32(0)), &copyBefore, &insertArg);
                    Bind(&copyBefore);
                    {
                        Label loopHead(env);
                        Label loopEnd(env);
                        Label loopNext(env);
                        Label loopExit(env);
                        Label eleIsHole(env);
                        Label eleNotHole(env);
                        Jump(&loopHead);
                        LoopBegin(&loopHead);
                        {
                            BRANCH(Int32LessThan(*oldIndex, *actualStart), &loopNext, &loopExit);
                            Bind(&loopNext);
                            GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, *oldIndex);
                            BRANCH(TaggedIsHole(ele), &eleIsHole, &eleNotHole);
                            Bind(&eleIsHole);
                            {
                                SetValueWithElementsKind(glue, newArray, Undefined(), *newIndex, Boolean(true),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&eleNotHole);
                            {
                                SetValueWithElementsKind(glue, newArray, ele, *newIndex, Boolean(true),
                                    Int32(Elements::ToUint(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                        }
                        Bind(&loopEnd);
                        oldIndex = Int32Add(*oldIndex, Int32(1));
                        newIndex = Int32Add(*newIndex, Int32(1));
                        LoopEnd(&loopHead);
                        Bind(&loopExit);
                        Jump(&insertArg);
                    }
                    Bind(&insertArg);
                    {
                        Label insert(env);
                        BRANCH(Int32GreaterThan(*insertCount, Int32(0)), &insert, &copyAfter);
                        Bind(&insert);
                        {
                            GateRef insertNum = GetCallArg2(numArgs);
                            SetValueWithElementsKind(glue, newArray, insertNum, *newIndex, Boolean(true),
                                Int32(Elements::ToUint(ElementsKind::NONE)));
                            newIndex = Int32Add(*newIndex, Int32(1));
                            Jump(&copyAfter);
                        }
                    }
                    Bind(&copyAfter);
                    {
                        Label canCopyAfter(env);
                        Label setLength(env);
                        oldIndex = Int32Add(*actualStart, *actualDeleteCount);
                        BRANCH(Int32LessThan(*oldIndex, thisLen), &canCopyAfter, &setLength);
                        Bind(&canCopyAfter);
                        {
                            Label loopHead1(env);
                            Label loopNext1(env);
                            Label loopEnd1(env);
                            Label loopExit1(env);
                            Label ele1IsHole(env);
                            Label ele1NotHole(env);
                            Jump(&loopHead1);
                            LoopBegin(&loopHead1);
                            {
                                BRANCH(Int32LessThan(*oldIndex, thisLen), &loopNext1, &loopExit1);
                                Bind(&loopNext1);
                                GateRef ele1 = GetTaggedValueWithElementsKind(glue, thisValue, *oldIndex);
                                BRANCH(TaggedIsHole(ele1), &ele1IsHole, &ele1NotHole);
                                Bind(&ele1IsHole);
                                {
                                    SetValueWithElementsKind(glue, newArray, Undefined(), *newIndex, Boolean(true),
                                        Int32(Elements::ToUint(ElementsKind::NONE)));
                                    Jump(&loopEnd1);
                                }
                                Bind(&ele1NotHole);
                                {
                                    SetValueWithElementsKind(glue, newArray, ele1, *newIndex, Boolean(true),
                                        Int32(Elements::ToUint(ElementsKind::NONE)));
                                    Jump(&loopEnd1);
                                }
                            }
                            Bind(&loopEnd1);
                            oldIndex = Int32Add(*oldIndex, Int32(1));
                            newIndex = Int32Add(*newIndex, Int32(1));
                            LoopEnd(&loopHead1);
                            Bind(&loopExit1);
                            Jump(&setLength);
                        }
                        Bind(&setLength);
                        {
                            SetArrayLength(glue, newArray, *newLen);
                            result->WriteVariable(newArray);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
#endif
}

void BuiltinsArrayStubBuilder::CopyWithin(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    CopyWithinOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "CopyWithin" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    DEFVARIABLE(startPos, VariableType::INT64(), Int64(0));
    DEFVARIABLE(endPos, VariableType::INT64(), Int64(0));
    Label targetTagExists(env);
    Label targetTagIsInt(env);
    Label startTagExists(env);
    Label startTagIsInt(env);
    Label afterCallArg1(env);
    Label endTagExists(env);
    Label endTagIsInt(env);
    Label afterCallArg2(env);
    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), slowPath, &targetTagExists);
    Bind(&targetTagExists);
    GateRef targetTag = GetCallArg0(numArgs);
    BRANCH(TaggedIsInt(targetTag), &targetTagIsInt, slowPath);
    Bind(&targetTagIsInt);
    GateRef argTarget = SExtInt32ToInt64(TaggedGetInt(targetTag));
    BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &afterCallArg1, &startTagExists);
    Bind(&startTagExists);
    {
        GateRef startTag = GetCallArg1(numArgs);
        BRANCH(TaggedIsInt(startTag), &startTagIsInt, slowPath);
        Bind(&startTagIsInt);
        startPos = SExtInt32ToInt64(TaggedGetInt(startTag));
        Jump(&afterCallArg1);
    }
    Bind(&afterCallArg1);
    {
        endPos = thisLen;
        BRANCH(Int64GreaterThanOrEqual(IntPtr(2), numArgs), &afterCallArg2, &endTagExists); //2: 2 parameters
        Bind(&endTagExists);
        {
            GateRef endTag = GetCallArg2(numArgs);
            BRANCH(TaggedIsInt(endTag), &endTagIsInt, slowPath);
            Bind(&endTagIsInt);
            {
                endPos = SExtInt32ToInt64(TaggedGetInt(endTag));
                Jump(&afterCallArg2);
            }
        }
    }
    Bind(&afterCallArg2);
    {
        DEFVARIABLE(copyTo, VariableType::INT64(), Int64(0));
        DEFVARIABLE(copyFrom, VariableType::INT64(), Int64(0));
        DEFVARIABLE(copyEnd, VariableType::INT64(), Int64(0));
        DEFVARIABLE(count, VariableType::INT64(), Int64(0));
        DEFVARIABLE(direction, VariableType::INT64(), Int64(0));
        Label calculateCountBranch1(env);
        Label calculateCountBranch2(env);
        Label afterCalculateCount(env);
        Label needToAdjustParam(env);
        Label afterAdjustParam(env);
        copyTo = CalculatePositionWithLength(argTarget, thisLen);
        copyFrom = CalculatePositionWithLength(*startPos, thisLen);
        copyEnd = CalculatePositionWithLength(*endPos, thisLen);
        BRANCH(Int64LessThan(Int64Sub(*copyEnd, *copyFrom), Int64Sub(thisLen, *copyTo)),
            &calculateCountBranch1, &calculateCountBranch2);
        Bind(&calculateCountBranch1);
        {
            count = Int64Sub(*copyEnd, *copyFrom);
            Jump(&afterCalculateCount);
        }
        Bind(&calculateCountBranch2);
        {
            count = Int64Sub(thisLen, *copyTo);
            Jump(&afterCalculateCount);
        }

        Bind(&afterCalculateCount);
        {
            direction = Int64(1);
            GateRef copyFromVal = *copyFrom;
            GateRef copyToVal = *copyTo;
            GateRef countVal = *count;
            BRANCH(LogicAndBuilder(env).And(Int64LessThan(copyFromVal, copyToVal))
                .And(Int64LessThan(copyToVal, Int64Add(copyFromVal, countVal))).Done(),
                &needToAdjustParam, &afterAdjustParam);
            Bind(&needToAdjustParam);
            {
                direction = Int64(-1);
                copyFrom = Int64Sub(Int64Add(*copyFrom, *count), Int64(1));
                copyTo = Int64Sub(Int64Add(*copyTo, *count), Int64(1));
                Jump(&afterAdjustParam);
            }

            Bind(&afterAdjustParam);
            {
                DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Label loopExit(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    Label kValueIsHole(env);
                    Label setValue(env);
                    Label hasProperty(env);
                    Label setHole(env);
                    Label hasException0(env);
                    Label notHasException0(env);
                    BRANCH(Int64GreaterThan(*count, Int64(0)), &next, &loopExit);
                    Bind(&next);
                    kValue = GetTaggedValueWithElementsKind(glue, thisValue, *copyFrom);
                    BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &setValue);
                    Bind(&kValueIsHole);
                    GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                        { thisValue, IntToTaggedInt(*copyFrom) });
                    BRANCH(TaggedIsTrue(hasProp), &hasProperty, &setHole);

                    Bind(&hasProperty);
                    kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*copyFrom), ProfileOperation());
                    BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                    Bind(&hasException0);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                    Bind(&notHasException0);
                    BRANCH(TaggedIsHole(*kValue), &setHole, &setValue);
                    Bind(&setHole);
                    {
                        SetValueWithElementsKind(glue, thisValue, Hole(), *copyTo,
                            Boolean(true), Int32(Elements::ToUint(ElementsKind::DICTIONARY)));
                        Jump(&loopEnd);
                    }
                    Bind(&setValue);
                    {
                        SetValueWithElementsKind(glue, thisValue, *kValue, *copyTo,
                            Boolean(true), Int32(Elements::ToUint(ElementsKind::NONE)));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                copyFrom = Int64Add(*copyFrom, *direction);
                copyTo = Int64Add(*copyTo, *direction);
                count = Int64Sub(*count, Int64(1));
                LoopEnd(&loopHead);
                Bind(&loopExit);
                result->WriteVariable(thisValue);
                Jump(exit);
            }
        }
    }
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
    SomeOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Label isStability(env);
    Label notCOWArray(env);
    Label equalCls(env);
    Label isGeneric(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Some" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
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

    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    Jump(&thisIsStable);

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
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(IsStableJSArray(glue, thisValue), &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    { thisValue, IntToTaggedInt(*i) });
                BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
                Bind(&hasProperty);
                {
                    kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                    BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                    Bind(&hasException0);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                    Bind(&notHasException0);
                    {
                        BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
                    }
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
                    Label changeThisLen(env);
                    Label afterChangeLen(env);
                    Label retValueIsTrue(env);
                    BRANCH(Int64LessThan(*newLen, *thisArrLen), &changeThisLen, &afterChangeLen);
                    Bind(&changeThisLen);
                    {
                        thisArrLen = *newLen;
                        Jump(&afterChangeLen);
                    }
                    Bind(&afterChangeLen);
                    {
                        BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &retValueIsTrue, &loopEnd);
                        Bind(&retValueIsTrue);
                        {
                            result->WriteVariable(TaggedTrue());
                            Jump(exit);
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(TaggedFalse());
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
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                { thisValue, IntToTaggedInt(*i) });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
            Bind(&hasProperty);
            {
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
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
                            Label retValueIsTrue(env);
                            BRANCH(TaggedIsTrue(FastToBoolean(glue, retValue)), &retValueIsTrue, &loopEnd);
                            Bind(&retValueIsTrue);
                            {
                                result->WriteVariable(TaggedTrue());
                                Jump(exit);
                            }
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(TaggedFalse());
        Jump(exit);
    }
#endif
}

void BuiltinsArrayStubBuilder::Every(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
#if ENABLE_NEXT_OPTIMIZATION
    EveryOptimised(glue, thisValue, numArgs, result, exit, slowPath);
#else
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    Label arg0HeapObject(env);
    Label callable(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Every" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);

    Label thisIsStable(env);
    Label thisNotStable(env);
    GateRef argHandle = GetCallArg1(numArgs);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    Jump(&thisIsStable);

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
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(IsStableJSArray(glue, thisValue), &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    { thisValue, IntToTaggedInt(*i) });
                BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
                Bind(&hasProperty);
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
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
                    Label changeThisLen(env);
                    Label afterChangeLen(env);
                    Label retValueIsFalse(env);
                    BRANCH(Int64LessThan(*newLen, *thisArrLen), &changeThisLen, &afterChangeLen);
                    Bind(&changeThisLen);
                    {
                        thisArrLen = *newLen;
                        Jump(&afterChangeLen);
                    }
                    Bind(&afterChangeLen);
                    {
                        BRANCH(TaggedIsFalse(FastToBoolean(glue, retValue)), &retValueIsFalse, &loopEnd);
                        Bind(&retValueIsFalse);
                        result->WriteVariable(TaggedFalse());
                        Jump(exit);
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(TaggedTrue());
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
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                { thisValue, IntToTaggedInt(*i) });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
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
                BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
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
                    Label retValueIsFalse(env);
                    BRANCH(TaggedIsFalse(FastToBoolean(glue, retValue)), &retValueIsFalse, &loopEnd);
                    Bind(&retValueIsFalse);
                    result->WriteVariable(TaggedFalse());
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(TaggedTrue());
        Jump(exit);
    }
#endif
}

void BuiltinsArrayStubBuilder::ReduceRight(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label notCOWArray(env);
    Label equalCls(env);
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
#if ENABLE_NEXT_OPTIMIZATION
                GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                                 { glue, thisValue, IntToTaggedPtr(*k), GetCurrentGlobalEnv() });
#else
                GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    {thisValue, IntToTaggedInt(*k)});
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
            GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                             { glue, thisValue, IntToTaggedPtr(*k), GetCurrentGlobalEnv() });
#else
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                {thisValue, IntToTaggedInt(*k)});
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
                GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                                 { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
#else
                GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    {thisValue, IntToTaggedInt(*i)});
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
            GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                             { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
#else
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                {thisValue, IntToTaggedInt(*i)});
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
                GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                                 { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
#else
                GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                    {thisValue, IntToTaggedInt(*i)});
#endif
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
#if ENABLE_NEXT_OPTIMIZATION
            GateRef hasProp = CallCommonStub(glue, CommonStubCSigns::JSTaggedValueHasProperty,
                                             { glue, thisValue, IntToTaggedPtr(*i), GetCurrentGlobalEnv() });
#else
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                {thisValue, IntToTaggedInt(*i)});
#endif
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

void BuiltinsArrayStubBuilder::FlatMap(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Label isStability(env);
    Label notCOWArray(env);
    Label equalCls(env);
    Label isGeneric(env);
    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH_LIKELY(IsJsArray(glue, thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // need check constructor, "FlatMap" should use ArraySpeciesCreate
    BRANCH_LIKELY(HasConstructor(glue, thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(glue, thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    Label arg0HeapObject(env);
    Label callable(env);
    Label thisNotStable(env);
    Label doFlat(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH_LIKELY(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH_LIKELY(IsCallable(glue, callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    GateRef argHandle = GetCallArg1(numArgs);

    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    DEFVARIABLE(newArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
    DEFVARIABLE(flag, VariableType::BOOL(), Boolean(false));
    GateRef mappedArray = NewArray(glue, *thisArrLen);
    GateRef mappedElements = GetElementsArray(glue, mappedArray);
    // fast path for stable array
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
            Label changeNewArrLen(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &nextStep, &thisNotStable);
            Bind(&nextStep);
            BRANCH_LIKELY(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH_UNLIKELY(TaggedIsHole(*kValue), &kValueIsHole, &callDispatch);
            Bind(&kValueIsHole);
            {
                newArrLen = Int64Sub(*newArrLen, Int64(1));
                Jump(&loopEnd);
            }
            Bind(&callDispatch);
            {
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs, ProfileOperation(), false);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH_UNLIKELY(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException1);
                {
                    DEFVARIABLE(newLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
                    Label changeThisLen(env);
                    Label afterChangeLen(env);
                    Label retValueIsHeapObject(env);
                    Label retValueIsJsArray(env);
                    BRANCH_UNLIKELY(Int64LessThan(*newLen, *thisArrLen), &changeThisLen, &afterChangeLen);
                    Bind(&changeThisLen);
                    {
                        newArrLen = Int64Sub(*newArrLen, Int64Sub(*thisArrLen, *newLen));
                        thisArrLen = *newLen;
                        Jump(&afterChangeLen);
                    }
                    Bind(&afterChangeLen);
                    {
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, mappedElements, *i, retValue);
                        BRANCH(TaggedIsHeapObject(retValue), &retValueIsHeapObject, &loopEnd);
                        Bind(&retValueIsHeapObject);
                        {
                            BRANCH_NO_WEIGHT(IsJsArray(glue, retValue), &retValueIsJsArray, &loopEnd);
                        }
                        Bind(&retValueIsJsArray);
                        {
                            // newArray only contains non-hole elements
                            // but elementsLength is bigger than number of non-hole elements
                            // so should trim after flat
                            GateRef retElements = GetElementsArray(glue, retValue);
                            GateRef elementsLength = GetLengthOfTaggedArray(retElements);
                            newArrLen = Int64Sub(Int64Add(*newArrLen, elementsLength), Int64(1));
                            Jump(&loopEnd);
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&doFlat);
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
            Label changeNewArrLen(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                { thisValue, IntToTaggedInt(*i) });
            BRANCH(TaggedIsTrue(hasProp), &hasProperty, &changeNewArrLen);
            Bind(&hasProperty);
            {
                kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    BRANCH(TaggedIsHole(*kValue), &changeNewArrLen, &callDispatch);
                }
            }
            Bind(&changeNewArrLen);
            {
                newArrLen = Int64Sub(*newArrLen, Int64(1));
                Jump(&loopEnd);
            }
            Bind(&callDispatch);
            {
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { argHandle, *kValue, key, thisValue };
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH_UNLIKELY(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException1);
                {
                    Label retValueIsHeapObject(env);
                    Label retValueIsJsArray(env);
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, mappedElements, *i, retValue);
                    BRANCH(TaggedIsHeapObject(retValue), &retValueIsHeapObject, &loopEnd);
                    Bind(&retValueIsHeapObject);
                    {
                        BRANCH(IsJsArray(glue, retValue), &retValueIsJsArray, &loopEnd);
                    }
                    Bind(&retValueIsJsArray);
                    {
                        // newArray only contains non-hole elements
                        // but elementsLength is bigger than number of non-hole elements
                        // so should trim after flat
                        GateRef retElements = GetElementsArray(glue, retValue);
                        GateRef elementsLength = GetLengthOfTaggedArray(retElements);
                        newArrLen = Int64Sub(Int64Add(*newArrLen, elementsLength), Int64(1));
                        Jump(&loopEnd);
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&doFlat);
    }

    Bind(&doFlat);
    {
        i = Int64(0);
        DEFVARIABLE(j, VariableType::INT64(), Int64(0));
        DEFVARIABLE(retValueItem, VariableType::JS_ANY(), Hole());
        GateRef newArray = NewArray(glue, *newArrLen);
        Label loopHead2(env);
        Label loopEnd2(env);
        Label next2(env);
        Label loopExit2(env);
        Jump(&loopHead2);
        LoopBegin(&loopHead2);
        {
            Label nextStep(env);
            Label retValueIsHeapObject(env);
            Label retValueIsJsArray(env);
            Label retValueIsNotJsArray(env);
            BRANCH_LIKELY(Int64LessThan(*i, *thisArrLen), &next2, &loopExit2);
            Bind(&next2);
            GateRef retValue = GetValueFromTaggedArray(glue, mappedElements, *i);
            BRANCH(TaggedIsHole(retValue), &loopEnd2, &nextStep);
            Bind(&nextStep);
            BRANCH_NO_WEIGHT(TaggedIsHeapObject(retValue), &retValueIsHeapObject, &retValueIsNotJsArray);
            Bind(&retValueIsHeapObject);
            {
                Label checkProxy(env);
                Label isProxy(env);
                BRANCH_NO_WEIGHT(IsJsArray(glue, retValue), &retValueIsJsArray, &checkProxy);
                Bind(&checkProxy);
                BRANCH_UNLIKELY(IsJsProxy(glue, retValue), &isProxy, &retValueIsNotJsArray);
                Bind(&isProxy);
                {
                    flag = Boolean(true);
                    Jump(&retValueIsNotJsArray);
                }
                Bind(&retValueIsJsArray);
                {
                    Label retValueIsStableArray(env);
                    Label retValueNotStableArray(env);
                    GateRef retValueIsStable = IsStableJSArray(glue, retValue);
                    GateRef arrLen = ZExtInt32ToInt64(GetArrayLength(retValue));
                    DEFVARIABLE(k, VariableType::INT64(), Int64(0));
                    Label loopHead3(env);
                    Label loopEnd3(env);
                    Label next3(env);
                    Label loopExit3(env);
                    Label setValue(env);
                    Label itemExist(env);
                    Jump(&loopHead3);
                    LoopBegin(&loopHead3);
                    {
                        BRANCH_LIKELY(Int64LessThan(*k, arrLen), &next3, &loopExit3);
                        Bind(&next3);
                        BRANCH_LIKELY(retValueIsStable, &retValueIsStableArray, &retValueNotStableArray);
                        Bind(&retValueIsStableArray);
                        retValueItem = GetTaggedValueWithElementsKind(glue, retValue, *k);
                        BRANCH_NO_WEIGHT(TaggedIsHole(*retValueItem), &loopEnd3, &setValue);
                        Bind(&retValueNotStableArray);
                        GateRef hasProp = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(), RTSTUB_ID(HasProperty),
                            { retValue, IntToTaggedInt(*k) });
                        BRANCH_NO_WEIGHT(TaggedIsTrue(hasProp), &itemExist, &loopEnd3);
                        Bind(&itemExist);
                        retValueItem =
                            FastGetPropertyByIndex(glue, retValue, TruncInt64ToInt32(*k), ProfileOperation());
                        Jump(&setValue);
                        Bind(&setValue);
                        SetValueWithElementsKind(glue, newArray, *retValueItem, *j, Boolean(true),
                            Int32(Elements::ToUint(ElementsKind::NONE)));
                        j = Int64Add(*j, Int64(1));
                        Jump(&loopEnd3);
                    }
                    Bind(&loopEnd3);
                    k = Int64Add(*k, Int64(1));
                    LoopEnd(&loopHead3);
                    Bind(&loopExit3);
                    Jump(&loopEnd2);
                }
            }
            Bind(&retValueIsNotJsArray);
            {
                SetValueWithElementsKind(glue, newArray, retValue, *j, Boolean(true),
                    Int32(Elements::ToUint(ElementsKind::NONE)));
                j = Int64Add(*j, Int64(1));
                Jump(&loopEnd2);
            }
        }
        Bind(&loopEnd2);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead2);
        Bind(&loopExit2);
        Label reportHiEvent(env);
        Label next(env);
        BRANCH_UNLIKELY(*flag, &reportHiEvent, &next);
        Bind(&reportHiEvent);
        {
            CallRuntime(glue, RTSTUB_ID(ReportHiEvents), {
                Int32(static_cast<int32_t>(DFXHiSysEvent::IncompatibleType::ARRAY_FLATMAP))});
            Jump(&next);
        }
        Bind(&next);
        Label trim(env);
        Label noTrim(env);
        BRANCH(Int32GreaterThan(*newArrLen, *j), &trim, &noTrim);
        Bind(&trim);
        {
            GateRef elements = GetElementsArray(glue, newArray);
            CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, elements, *j});
            SetArrayLength(glue, newArray, TruncInt64ToInt32(*j));
            result->WriteVariable(newArray);
            Jump(exit);
        }
        Bind(&noTrim);
        result->WriteVariable(newArray);
        Jump(exit);
    }
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
