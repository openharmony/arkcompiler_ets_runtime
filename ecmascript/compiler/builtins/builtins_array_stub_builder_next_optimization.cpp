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

#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/base/array_helper.h"

namespace panda::ecmascript::kungfu {
void BuiltinsArrayStubBuilder::UnshiftOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result,
                                                Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStableJsArray(env);
    Label notOverRange(env);
    Label numNotEqualZero(env);
    Label numLessThanOrEqualThree(env);
    Label grow(env);
    Label setValue(env);
    Label numEqual2(env);
    Label numEqual3(env);
    Label threeArgs(env);
    Label final(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
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
    GateRef capacity = ZExtInt32ToInt64(GetLengthOfTaggedArray(GetElementsArray(thisValue)));
    BRANCH(Int64GreaterThan(newLen, capacity), &grow, &setValue);
    Bind(&grow);
    {
        GrowElementsCapacity(glue, thisValue, TruncInt64ToInt32(newLen));
        Jump(&setValue);
    }
    Bind(&setValue);
    {
        Label directAdd(env);
        Label mutantArrayEnabled(env);
        GateRef elements = GetElementsArray(thisValue);
        GateRef arrayStart = GetDataPtrInTaggedArray(elements);
        GateRef moveTo = PtrAdd(arrayStart, PtrMul(numArgs, IntPtr(JSTaggedValue::TaggedTypeSize())));
        GateRef kind = GetElementsKindFromHClass(LoadHClass(thisValue));
        ArrayCopy(glue, elements, arrayStart, elements, moveTo, TruncInt64ToInt32(thisLen),
                  NeedBarrier(kind), SameArray);
        BRANCH_UNLIKELY(IsEnableMutantArray(glue), &mutantArrayEnabled, &directAdd);
        Bind(&directAdd);
        {
            GateRef arg0 = GetCallArg0(numArgs);
            GateRef arg1 = GetCallArg1(numArgs);
            GateRef arg2 = GetCallArg2(numArgs);
            DEFVARIABLE(newKind, VariableType::INT32(), kind);
            Label migrateElementsKind(env);
            int64_t argCount[THREE_ARGS] = {ONE_ARGS, TWO_ARGS, THREE_ARGS};
            Label labels[THREE_ARGS] = {Label(env), Label(env), Label(env)};
            Switch(numArgs, slowPath, argCount, labels, THREE_ARGS);
            Bind(&labels[Index2]);
            {
                newKind = Int32Or(TaggedToElementKind(arg0), *newKind);
                newKind = Int32Or(TaggedToElementKind(arg1), *newKind);
                newKind = Int32Or(TaggedToElementKind(arg2), *newKind);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(Index0), arg0);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(Index1), arg1);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(Index2), arg2);
                Jump(&migrateElementsKind);
            }
            Bind(&labels[Index1]);
            {
                newKind = Int32Or(TaggedToElementKind(arg0), *newKind);
                newKind = Int32Or(TaggedToElementKind(arg1), *newKind);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(Index0), arg0);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(Index1), arg1);
                Jump(&migrateElementsKind);
            }
            Bind(&labels[Index0]);
            {
                newKind = Int32Or(TaggedToElementKind(arg0), *newKind);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, Int32(Index0), arg0);
                Jump(&migrateElementsKind);
            }
            Bind(&migrateElementsKind);
            {
                newKind = FixElementsKind(*newKind);
                Label needTransition(env);
                BRANCH_UNLIKELY(Int32NotEqual(*newKind, kind), &needTransition, &final);
                Bind(&needTransition);
                {
                    CallRuntime(glue, RTSTUB_ID(UpdateHClassForElementsKind), { thisValue, *newKind });
                    Jump(&final);
                }
            }
        }
        Bind(&mutantArrayEnabled);
        {
            GateRef value0 = GetCallArg0(numArgs);
            // 0 : the first Element position
            SetValueWithElementsKind(glue, thisValue, value0, Int64(Index0), Boolean(false),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            // 2 : the second param
            BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(TWO_ARGS)), &numEqual2, &numEqual3);
            Bind(&numEqual2);
            {
                GateRef value1 = GetCallArg1(numArgs);
                // 1 : the second Element position
                SetValueWithElementsKind(glue, thisValue, value1, Int64(Index1), Boolean(false),
                                         Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                Jump(&numEqual3);
            }
            Bind(&numEqual3);
            {
                // 3 : the third param
                BRANCH(Int64Equal(numArgs, IntPtr(THREE_ARGS)), &threeArgs, &final);
                Bind(&threeArgs);
                GateRef value2 = GetCallArg2(numArgs);
                // 2 : the third Element position
                SetValueWithElementsKind(glue, thisValue, value2, Int64(Index2), Boolean(false),
                                         Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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

GateRef BuiltinsArrayStubBuilder::DoSortOptimised(GateRef glue, GateRef receiver, GateRef receiverState,
                                         Variable *result, Label *exit, Label *slowPath, GateRef hir)
{
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
            GateRef presentValueHasProp = CallRuntime(glue, RTSTUB_ID(HasProperty), {receiver, IntToTaggedInt(*i)});
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
                    GateRef middleValueHasProp = CallRuntime(glue, RTSTUB_ID(HasProperty),
                                                             {receiver, IntToTaggedInt(middleIndex)});
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
                    Label intOrDouble(env);
                    Label notIntAndDouble(env);
                    Label exchangeIndex(env);
                    GateRef middleVal = *middleValue;
                    GateRef presentVal = *presentValue;
                    DEFVARIABLE(compareResult, VariableType::INT32(), Int32(0));
                    GateRef intDoubleCheck = BitOr(BitAnd(TaggedIsInt(middleVal), TaggedIsInt(presentVal)),
                                                   BitAnd(TaggedIsDouble(middleVal), TaggedIsDouble(presentVal)));
                    BRANCH(intDoubleCheck, &intOrDouble, &notIntAndDouble);
                    Bind(&intOrDouble);
                    {
                        compareResult =
                            CallNGCRuntime(glue, RTSTUB_ID(FastArraySort), {*middleValue, *presentValue});
                        Jump(&exchangeIndex);
                    }
                    Bind(&notIntAndDouble);
                    {
                        Label isString(env);
                        GateRef strBool = LogicAndBuilder(env)
                                          .And(TaggedIsString(middleVal))
                                          .And(TaggedIsString(presentVal))
                                          .Done();
                        BRANCH(strBool, &isString, slowPath);
                        Bind(&isString);
                        {
                            compareResult = CallNGCRuntime(glue,
                                RTSTUB_ID(FastArraySortString), {glue, *middleValue, *presentValue});
                            Jump(&exchangeIndex);
                        }
                    }
                    Bind(&exchangeIndex);
                    {
                        Label less0(env);
                        Label greater0(env);
                        BRANCH(Int32LessThanOrEqual(*compareResult, Int32(0)), &less0, &greater0);
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
                Label receiverIsNew(env);
                Label receiverIsOrigin(env);
                Label receiverIsNew2(env);
                Label receiverIsOrigin2(env);
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
                        GateRef previousValueHasProp = CallRuntime(glue, RTSTUB_ID(HasProperty),
                                                                   {receiver, IntToTaggedInt(Int64Sub(*j, Int64(1)))});
                        BRANCH(TaggedIsTrue(previousValueHasProp),
                               &previousValueHasProperty, &afterGettingpreviousValue);
                        Bind(&previousValueHasProperty);
                        {
                            previousValue = FastGetPropertyByIndex(glue, receiver,
                                                                   TruncInt64ToInt32(Int64Sub(*j, Int64(1))),
                                                                   ProfileOperation(), hir);
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
                        BRANCH(receiverState, &receiverIsNew, &receiverIsOrigin);
                        Bind(&receiverIsNew);
                        SetValueWithElementsKind(glue, receiver, *previousValue, *j, Boolean(true),
                                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                        Jump(&loopEnd2);
                        Bind(&receiverIsOrigin);
                        SetValueWithElementsKind(glue, receiver, *previousValue, *j, Boolean(false),
                                                 Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                        Jump(&loopEnd2);
                    }
                }
                Bind(&loopEnd2);
                j = Int64Sub(*j, Int64(1));
                LoopEnd(&loopHead2);
                Bind(&loopExit2);
                BRANCH(receiverState, &receiverIsNew2, &receiverIsOrigin2);
                Bind(&receiverIsNew2);
                {
                    SetValueWithElementsKind(glue, receiver, *presentValue, *endIndex, Boolean(true),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
                Bind(&receiverIsOrigin2);
                {
                    SetValueWithElementsKind(glue, receiver, *presentValue, *endIndex, Boolean(false),
                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                    Jump(&loopEnd);
                }
            }
        }
    }
    Bind(&loopEnd);
    i = Int64Add(*i, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    env->SubCfgExit();
    return receiver;
}

GateRef BuiltinsArrayStubBuilder::DoSortOptimisedFast(GateRef glue, GateRef receiver,
    [[maybe_unused]] Variable *result, [[maybe_unused]] Label *exit,
    Label *slowPath, [[maybe_unused]] GateRef hir)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    GateRef len = ZExtInt32ToInt64(GetArrayLength(receiver));
    DEFVARIABLE(i, VariableType::INT64(), Int64(1));
    DEFVARIABLE(presentValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(middleValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(previousValue, VariableType::JS_ANY(), Undefined());
    GateRef elements = GetElementsArray(receiver);
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
        Label afterGettingpresentValue(env);
        presentValue = GetValueFromTaggedArray(elements, TruncInt64ToInt32(*i));
        Jump(&afterGettingpresentValue);
        Bind(&afterGettingpresentValue);
        {
            Label loopHead1(env);
            Label loopEnd1(env);
            Label next1(env);
            Label loopExit1(env);
            Jump(&loopHead1);
            LoopBegin(&loopHead1);
            {
                Label afterGettingmiddleValue(env);
                BRANCH(Int64LessThan(*beginIndex, *endIndex), &next1, &loopExit1);
                Bind(&next1);
                GateRef sum = Int64Add(*beginIndex, *endIndex);
                GateRef middleIndex = Int64Div(sum, Int64(2)); // 2 : half
                middleValue = GetValueFromTaggedArray(elements, TruncInt64ToInt32(middleIndex));
                Jump(&afterGettingmiddleValue);
                Bind(&afterGettingmiddleValue);
                {
                    Label intOrDouble(env);
                    Label notIntAndDouble(env);
                    Label exchangeIndex(env);
                    DEFVARIABLE(compareResult, VariableType::INT32(), Int32(0));

                    Label middleIsHole(env);
                    Label presentIsHole(env);
                    Label middleNotHole(env);
                    Label presentNotHole(env);
                    Label notHole(env);
                    Label middleIsUndefined(env);
                    Label presentIsUndefined(env);
                    Label middleNotUndefined(env);
                    Label presentNotUndefined(env);
                    Label notUndefined(env);
                    BRANCH(TaggedIsHole(*middleValue), &middleIsHole, &middleNotHole);
                    Bind(&middleIsHole);
                    {
                        Label presentNotHole0(env);
                        BRANCH(TaggedIsHole(*presentValue), &exchangeIndex, &presentNotHole0);
                        Bind(&presentNotHole0);
                        compareResult = Int32(1);
                        Jump(&exchangeIndex);
                    }
                    Bind(&middleNotHole);
                    BRANCH(TaggedIsHole(*presentValue), &presentIsHole, &presentNotHole);
                    Bind(&presentIsHole);
                    {
                        compareResult = Int32(-1);
                        Jump(&exchangeIndex);
                    }
                    Bind(&presentNotHole);
                    BRANCH(TaggedIsUndefined(*middleValue), &middleIsUndefined, &middleNotUndefined);
                    Bind(&middleIsUndefined);
                    {
                        Label presentNotUndefined0(env);
                        BRANCH(TaggedIsUndefined(*presentValue), &exchangeIndex, &presentNotUndefined0);
                        Bind(&presentNotUndefined0);
                        compareResult = Int32(1);
                        Jump(&exchangeIndex);
                    }
                    Bind(&middleNotUndefined);
                    BRANCH(TaggedIsUndefined(*presentValue), &presentIsUndefined, &presentNotUndefined);
                    Bind(&presentIsUndefined);
                    {
                        compareResult = Int32(-1);
                        Jump(&exchangeIndex);
                    }
                    Bind(&presentNotUndefined);
                    GateRef intDoubleCheck = BitOr(BitAnd(TaggedIsInt(*middleValue),
                                                          TaggedIsInt(*presentValue)),
                                                   BitAnd(TaggedIsDouble(*middleValue),
                                                          TaggedIsDouble(*presentValue)));
                    BRANCH(intDoubleCheck, &intOrDouble, &notIntAndDouble);
                    Bind(&intOrDouble);
                    {
                        compareResult =
                            CallNGCRuntime(glue, RTSTUB_ID(FastArraySort), {*middleValue, *presentValue});
                        Jump(&exchangeIndex);
                    }
                    Bind(&notIntAndDouble);
                    Label isString(env);
                    GateRef stringCheck = BitAnd(TaggedIsString(*middleValue),
                                                 TaggedIsString(*presentValue));
                    BRANCH(stringCheck, &isString, slowPath);
                    Bind(&isString);
                    {
                        compareResult = CallNGCRuntime(glue, RTSTUB_ID(FastArraySortString),
                            { glue, *middleValue, *presentValue });
                        Jump(&exchangeIndex);
                    }
                    Bind(&exchangeIndex);
                    {
                        Label less0(env);
                        Label greater0(env);
                        BRANCH(Int32LessThanOrEqual(*compareResult, Int32(0)), &less0, &greater0);
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
                Label receiverIsNew(env);
                Label receiverIsOrigin(env);
                Label receiverIsNew2(env);
                Label receiverIsOrigin2(env);
                Jump(&loopHead2);
                LoopBegin(&loopHead2);
                {
                    BRANCH(Int64GreaterThan(*j, *endIndex), &next2, &loopExit2);
                    Bind(&next2);
                    previousValue = GetValueFromTaggedArray(elements, TruncInt64ToInt32(Int64Sub(*j, Int64(1))));
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, TruncInt64ToInt32(*j),
                                          *previousValue);
                    Jump(&loopEnd2);
                }
                Bind(&loopEnd2);
                j = Int64Sub(*j, Int64(1));
                LoopEnd(&loopHead2);
                Bind(&loopExit2);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, TruncInt64ToInt32(*endIndex),
                                      *presentValue);
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
}

void BuiltinsArrayStubBuilder::CopyWithinOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label defaultConstr(env);
    Label isStability(env);
    Label isGeneric(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Label isJsCOWArray(env);
    Label getElements(env);
    BRANCH(IsJsCOWArray(thisValue), &isJsCOWArray, &getElements);
    Bind(&isJsCOWArray);
    {
        NewObjectStubBuilder newBuilder(this);
        GateRef elements = GetElementsArray(thisValue);
        GateRef capacity = GetLengthOfTaggedArray(elements);
        GateRef newElements = newBuilder.CopyArray(glue, elements, capacity, capacity);
        SetElementsArray(VariableType::JS_POINTER(), glue, thisValue, newElements);
        Jump(&getElements);
    }
    Bind(&getElements);
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
    BRANCH(Int64GreaterThanOrEqual(IntPtr(ONE_ARGS), numArgs), &afterCallArg1, &startTagExists);
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
        BRANCH(Int64GreaterThanOrEqual(IntPtr(TWO_ARGS), numArgs), &afterCallArg2, &endTagExists);
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
        Label countGreaterZero(env);
        Label countLessOrEqualZero(env);
        BRANCH(Int64GreaterThan(*count, Int64(0)), &countGreaterZero, &countLessOrEqualZero);
        Bind(&countLessOrEqualZero);
        {
            result->WriteVariable(thisValue);
            Jump(exit);
        }
        Bind(&countGreaterZero);
        Label enableMutantArray(env);
        Label disableMutantArray(env);
        GateRef isEnable = IsEnableMutantArray(glue);
        BRANCH(isEnable, &enableMutantArray, &disableMutantArray);
        Bind(&enableMutantArray);
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
                    BRANCH(TaggedIsHole(*kValue), &setHole, &setValue);
                    Bind(&setHole);
                    {
                        SetValueWithElementsKind(glue, thisValue, Hole(), *copyTo,
                            Boolean(true), Int32(static_cast<uint32_t>(ElementsKind::GENERIC)));
                        Jump(&loopEnd);
                    }
                    Bind(&setValue);
                    {
                        SetValueWithElementsKind(glue, thisValue, *kValue, *copyTo,
                            Boolean(true), Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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
        Bind(&disableMutantArray);
        {
            GateRef elements = GetElementsArray(thisValue);
            GateRef elementsPtr = GetDataPtrInTaggedArray(elements);
            GateRef kind = GetElementsKindFromHClass(LoadHClass(thisValue));
            GateRef src = PtrAdd(elementsPtr, PtrMul(*copyFrom, IntPtr(JSTaggedValue::TaggedTypeSize())));
            GateRef dest = PtrAdd(elementsPtr, PtrMul(*copyTo, IntPtr(JSTaggedValue::TaggedTypeSize())));
            ArrayCopy(glue, elements, src, elements, dest,
                      TruncInt64ToInt32(*count), NeedBarrier(kind), SameArray);
            result->WriteVariable(thisValue);
            Jump(exit);
        }
    }
}

void BuiltinsArrayStubBuilder::ToReversedOptimised(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
                                          Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "ToReversed" always use ArrayCreate to create array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    Label next(env);
    DEFVARIABLE(receiver, VariableType::JS_ANY(), Hole());
    GateRef kind = GetElementsKindFromHClass(LoadHClass(thisValue));
    GateRef thisArrLen = GetArrayLength(thisValue);
    Label newArrayIsTagged(env);
    Label reuseOldHClass(env);
    BRANCH_NO_WEIGHT(ElementsKindHasHole(kind), &newArrayIsTagged, &reuseOldHClass);
    Bind(&newArrayIsTagged);
    {
        // If the kind has hole, we know it must be transited to TAGGED kind;
        // There will be no hole in the new array because hole will be converted to undefined.
        GateRef newHClass = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                   ConstantIndex::ELEMENT_TAGGED_HCLASS_INDEX);
        receiver = NewEmptyArrayWithHClass(glue, newHClass, thisArrLen);
        Jump(&next);
    }
    Bind(&reuseOldHClass);
    {
        receiver = NewEmptyArrayWithHClass(glue, LoadHClass(thisValue), thisArrLen);
        Jump(&next);
    }
    Bind(&next);
    Label afterReverse(env);
    Label isIntOrNumber(env);
    Label notIntOrNumber(env);
    Label isTagged(env);
    Label isHoleOrIntOrNumber(env);
    GateRef intOrNumber = LogicOrBuilder(env)
                          .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::INT))))
                          .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::NUMBER))))
                          .Done();
    BRANCH_NO_WEIGHT(intOrNumber, &isIntOrNumber, &notIntOrNumber);
    Bind(&notIntOrNumber);
    {
        GateRef holeOrIntOrNumber = LogicOrBuilder(env)
                                    .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_INT))))
                                    .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_NUMBER))))
                                    .Done();
        BRANCH_NO_WEIGHT(holeOrIntOrNumber, &isHoleOrIntOrNumber, &isTagged);
    }
    Bind(&isTagged);
    {
        // The old array and new array are both TaggedArray, so load and store the element directly.
        // And barrier is needed.
        DoReverse(glue, thisValue, *receiver, true, false, MemoryAttribute::Default());
        Jump(&afterReverse);
    }
    Bind(&isIntOrNumber);
    {
        // The old array and new array are both MutantTaggedArray, so load and store the element directly.
        // And barrier is not needed.
        DoReverse(glue, thisValue, *receiver, false, false, MemoryAttribute::NoBarrier());
        Jump(&afterReverse);
    }
    Bind(&isHoleOrIntOrNumber);
    {
        // The old array is mutant, but new array is TaggedArray, so load the value from old array with
        // elements kind. And set it to new array directly, And barrier is not needed.
        DoReverse(glue, thisValue, *receiver, true, true, MemoryAttribute::NoBarrier());
        Jump(&afterReverse);
    }
    Bind(&afterReverse);
    result->WriteVariable(*receiver);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::DoReverse(GateRef glue, GateRef fromArray, GateRef toArray, bool holeToUndefined,
                                         bool getWithKind, MemoryAttribute mAttr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label loopExit(env);
    Label begin(env);
    Label body(env);
    Label endLoop(env);

    GateRef fromElements = GetElementsArray(fromArray);
    GateRef toElements = GetElementsArray(toArray);
    GateRef thisArrLen = GetArrayLength(fromArray);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    GateRef endIndex = Int32Sub(thisArrLen, Int32(1));
    Jump(&begin);
    LoopBegin(&begin);
    {
        BRANCH_LIKELY(Int32UnsignedLessThan(*index, thisArrLen), &body, &loopExit);
        Bind(&body);
        {
            GateRef toIndex = Int32Sub(endIndex, *index);
            // The old array and new array are both TaggedArray, so load and store the element directly.
            // And barrier is needed.
            GateRef value = getWithKind
                                ? GetTaggedValueWithElementsKind(glue, fromArray, *index)
                                : GetValueFromTaggedArray(fromElements, *index);
            if (holeToUndefined) {
                Label isHole(env);
                Label isNotHole(env);
                BRANCH_UNLIKELY(TaggedIsHole(value), &isHole, &isNotHole);
                Bind(&isHole);
                {
                    // The return value of toReversed() is never sparse.
                    // Empty slots become undefined in the returned array.
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, toElements, toIndex, Undefined(),
                                          MemoryAttribute::NoBarrier());
                    Jump(&endLoop);
                }
                Bind(&isNotHole);
            }
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, toElements, toIndex, value, mAttr);
            Jump(&endLoop);
        }
    }
    Bind(&endLoop);
    index = Int32Add(*index, Int32(1));
    LoopEnd(&begin);
    Bind(&loopExit);
    env->SubCfgExit();
}


// new an empty array, the length is zero, but with specific hclass,
GateRef BuiltinsArrayStubBuilder::NewEmptyArrayWithHClass(GateRef glue, GateRef hclass, GateRef newArrayLen)
{
#if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
    hclass = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX);
#endif
    // New an array with zero length.
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());
    Label exit(env);
    Label setProperties(env);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, Int32(0));
    result = newBuilder.NewEmptyJSArrayWithHClass(hclass);
    BRANCH(TaggedIsException(*result), &exit, &setProperties);
    Bind(&setProperties);
    {
        InitializeArray(glue, Int32(0), &result, hclass);
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    GrowElementsCapacity(glue, res, newArrayLen);
    SetArrayLength(glue, res, newArrayLen);
    env->SubCfgExit();
    return res;
}

void BuiltinsArrayStubBuilder::FastToSpliced(GateRef glue, GateRef thisValue, GateRef newArray, GateRef actualStart,
                                             GateRef actualDeleteCount, GateRef insertCount, GateRef insertValue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label copyBefore(env);
    Label copyAfter(env);
    Label insertArg(env);
    Label exit(env);
    GateRef srcElements = GetElementsArray(thisValue);
    GateRef dstElements = GetElementsArray(newArray);
    GateRef thisLength = GetLengthOfJSArray(thisValue);
    BRANCH(Int32GreaterThan(actualStart, Int32(0)), &copyBefore, &insertArg);
    Bind(&copyBefore);
    {
        GateRef srcStart = GetDataPtrInTaggedArray(srcElements);
        GateRef dstStart = GetDataPtrInTaggedArray(dstElements);
        ArrayCopyAndHoleToUndefined(glue, srcElements, srcStart, dstElements, dstStart, actualStart, Boolean(true));
        Jump(&insertArg);
    }
    Bind(&insertArg);
    {
        Label insert(env);
        BRANCH(Int32GreaterThan(insertCount, Int32(0)), &insert, &copyAfter);
        Bind(&insert);
        {
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, dstElements, actualStart, insertValue);
            Jump(&copyAfter);
        }
    }
    Bind(&copyAfter);
    {
        Label canCopyAfter(env);
        Label setLength(env);
        GateRef oldIndex = Int32Add(actualStart, actualDeleteCount);
        GateRef newIndex = Int32Add(actualStart, insertCount);
        BRANCH(Int32LessThan(oldIndex, thisLength), &canCopyAfter, &setLength);
        Bind(&canCopyAfter);
        {
            GateRef srcStart = GetDataPtrInTaggedArray(srcElements, oldIndex);
            GateRef dstStart = GetDataPtrInTaggedArray(dstElements, newIndex);
            GateRef afterLength = Int32Sub(thisLength, oldIndex);
            ArrayCopyAndHoleToUndefined(glue, srcElements, srcStart, dstElements, dstStart, afterLength, Boolean(true));
            newIndex = Int32Add(newIndex, afterLength);
            Jump(&setLength);
        }
        Bind(&setLength);
        {
            SetArrayLength(glue, newArray, newIndex);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsArrayStubBuilder::ToSplicedOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                         Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "ToSpliced" always use ArrayCreate to create array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Label notCOWArray(env);
    BRANCH(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
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
                    NewObjectStubBuilder newBuilder(this);
                    result->WriteVariable(newBuilder.CreateEmptyArray(glue));
                    Jump(exit);
                }
                Bind(&newLenNotEmpty);
                {
                    Label copyBefore(env);
                    Label insertArg(env);
                    Label mutantArrayToSpliced(env);
                    Label fastToSpliced(env);
                    BRANCH_UNLIKELY(IsEnableMutantArray(glue), &mutantArrayToSpliced, &fastToSpliced);
                    Bind(&fastToSpliced);
                    {
                        GateRef newHClass = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                                   ConstantIndex::ELEMENT_TAGGED_HCLASS_INDEX);
                        GateRef newArray = NewEmptyArrayWithHClass(glue, newHClass, *newLen);
                        FastToSpliced(glue, thisValue, newArray, *actualStart, *actualDeleteCount, *insertCount,
                                      GetCallArg2(numArgs));
                        result->WriteVariable(newArray);
                        Jump(exit);
                    }
                    Bind(&mutantArrayToSpliced);
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
                                                         Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&eleNotHole);
                            {
                                SetValueWithElementsKind(glue, newArray, ele, *newIndex, Boolean(true),
                                                         Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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
                                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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
                                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                    Jump(&loopEnd1);
                                }
                                Bind(&ele1NotHole);
                                {
                                    SetValueWithElementsKind(glue, newArray, ele1, *newIndex, Boolean(true),
                                                             Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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
}

void BuiltinsArrayStubBuilder::FindOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                             Variable *result, Label *exit, Label *slowPath)
{
    FindOrFindIndex(glue, thisValue, numArgs, result, exit, slowPath, Value);
}

void BuiltinsArrayStubBuilder::FindIndexOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                                  Variable *result, Label *exit, Label *slowPath)
{
    FindOrFindIndex(glue, thisValue, numArgs, result, exit, slowPath, Index);
}

void BuiltinsArrayStubBuilder::FindOrFindIndex(GateRef glue, GateRef thisValue, GateRef numArgs,
                                               Variable *result, Label *exit, Label *slowPath,
                                               IndexOrValue indexOrValue)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH_LIKELY(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);

    Label arg0HeapObject(env);
    Label callable(env);
    Label stableJSArray(env);
    Label notStableJSArray(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH_LIKELY(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH_LIKELY(IsCallable(callbackFnHandle), &callable, slowPath);
    Bind(&callable);

    result->WriteVariable(indexOrValue == Index ? IntToTaggedPtr(Int32(-1)) : Undefined());
    GateRef argHandle = GetCallArg1(numArgs);
    GateRef thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &stableJSArray, &notStableJSArray);
    Bind(&stableJSArray);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
            Label useUndefined(env);
            Label getValue(env);
            Label callback(env);
            BRANCH_NO_WEIGHT(Int64LessThan(*i, thisArrLen), &next, &loopExit);
            Bind(&next);
            BRANCH_LIKELY(Int64LessThan(*i,  ZExtInt32ToInt64(GetArrayLength(thisValue))), &getValue, &useUndefined);
            Bind(&getValue);
            {
                kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
                BRANCH_UNLIKELY(TaggedIsHole(*kValue), &useUndefined, &callback);
            }
            Bind(&useUndefined);
            {
                kValue = Undefined();
                Jump(&callback);
            }
            Bind(&callback);
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
                BRANCH_UNLIKELY(HasPendingException(glue), &hasException, &notHasException);
                Bind(&hasException);
                {
                    result->WriteVariable(retValue);
                    Jump(exit);
                }
                Bind(&notHasException);
                {
                    Label find(env);
                    BRANCH_NO_WEIGHT(TaggedIsTrue(FastToBoolean(retValue)), &find, &checkStable);
                    Bind(&find);
                    {
                        result->WriteVariable(indexOrValue == Index ? key : *kValue);
                        Jump(exit);
                    }
                }
                Bind(&checkStable);
                i = Int64Add(*i, Int64(1));
                BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &loopEnd, &notStableJSArray);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(exit);
    }
    Bind(&notStableJSArray);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH_NO_WEIGHT(Int64LessThan(*i, thisArrLen), &next, &loopExit);
            Bind(&next);
            {
                Label hasException0(env);
                Label notHasException0(env);
                GateRef kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
                BRANCH_UNLIKELY(HasPendingException(glue), &hasException0, &notHasException0);
                Bind(&hasException0);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException0);
                {
                    GateRef key = IntToTaggedPtr(*i);
                    Label hasException(env);
                    Label notHasException(env);
                    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                    callArgs.callThisArg3WithReturnArgs = { argHandle, kValue, key, thisValue };
                    CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                        nullptr, Circuit::NullGate(), callArgs);
                    GateRef retValue = callBuilder.JSCallDispatch();
                    BRANCH_UNLIKELY(TaggedIsException(retValue), &hasException, &notHasException);
                    Bind(&hasException);
                    {
                        result->WriteVariable(retValue);
                        Jump(exit);
                    }
                    Bind(&notHasException);
                    {
                        Label find(env);
                        BRANCH_NO_WEIGHT(TaggedIsTrue(FastToBoolean(retValue)), &find, &loopEnd);
                        Bind(&find);
                        {
                            result->WriteVariable(indexOrValue == Index ? key : kValue);
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
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::EveryOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                              Variable *result, Label *exit, Label *slowPath)
{
    result->WriteVariable(TaggedTrue());
    VisitAll(glue, thisValue, numArgs, result, exit, slowPath, kEvery);
};

void BuiltinsArrayStubBuilder::SomeOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                             Variable *result, Label *exit, Label *slowPath)
{
    result->WriteVariable(TaggedFalse());
    VisitAll(glue, thisValue, numArgs, result, exit, slowPath, kSome);
};

void BuiltinsArrayStubBuilder::ForEachOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                                Variable *result, Label *exit, Label *slowPath)
{
    result->WriteVariable(Undefined());
    VisitAll(glue, thisValue, numArgs, result, exit, slowPath, kForEach);
};

void BuiltinsArrayStubBuilder::VisitAll(GateRef glue, GateRef thisValue, GateRef numArgs,
                                        Variable *result, Label *exit, Label *slowPath, VisitKind visitKind)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label arg0HeapObject(env);
    Label callable(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "VisitKind" won't create new array.
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH(IsCallable(callbackFnHandle), &callable, slowPath);
    Bind(&callable);

    Label returnFalse(env);
    Label returnTrue(env);

    Label thisIsStable(env);
    Label thisNotStable(env);
    GateRef argHandle = GetCallArg1(numArgs);
    DEFVARIABLE(i, VariableType::INT64(), Int64(0));
    GateRef thisArrLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &thisIsStable, &thisNotStable);

    Bind(&thisIsStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label callDispatch(env);
            BRANCH_NO_WEIGHT(Int64LessThan(*i, thisArrLen), &next, exit);
            Bind(&next);
            kValue = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            BRANCH_UNLIKELY(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
            Bind(&callDispatch);
            {
                Label hasException(env);
                Label noException(env);
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = {argHandle, *kValue, key, thisValue};
                CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                                            Circuit::NullGate(), callArgs);
                GateRef retValue = callBuilder.JSCallDispatch();
                BRANCH_UNLIKELY(HasPendingException(glue), &hasException, &noException);
                Bind(&hasException);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&noException);
                {
                    Label checkLength(env);
                    Label checkStable(env);
                    if (visitKind == kEvery) {
                        BRANCH_NO_WEIGHT(TaggedIsFalse(FastToBoolean(retValue)), &returnFalse, &checkLength);
                    }
                    if (visitKind == kSome) {
                        BRANCH_NO_WEIGHT(TaggedIsTrue(FastToBoolean(retValue)), &returnTrue, &checkLength);
                    }
                    if (visitKind == kForEach) {
                        Jump(&checkLength);
                    }
                    Bind(&checkLength);
                    {
                        GateRef newLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
                        BRANCH_LIKELY(Int64LessThan(Int64Add(*i, Int64(1)), newLen), &checkStable, exit);
                    }
                    Bind(&checkStable);
                    {
                        Label changeToNotStable(env);
                        BRANCH_LIKELY(IsStableJSArray(glue, thisValue), &loopEnd, &changeToNotStable);
                        Bind(&changeToNotStable);
                        {
                            i = Int64Add(*i, Int64(1));
                            Jump(&thisNotStable);
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
    }

    Bind(&thisNotStable);
    {
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasProperty(env);
            Label hasException0(env);
            Label notHasException0(env);
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH_NO_WEIGHT(Int64LessThan(*i, thisArrLen), &next, exit);
            Bind(&next);
            GateRef hasProp = CallRuntime(glue, RTSTUB_ID(HasProperty), {thisValue, IntToTaggedInt(*i)});
            BRANCH_LIKELY(TaggedIsTrue(hasProp), &hasProperty, &loopEnd);
            Bind(&hasProperty);
            kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i), ProfileOperation());
            BRANCH_UNLIKELY(HasPendingException(glue), &hasException0, &notHasException0);
            Bind(&hasException0);
            {
                result->WriteVariable(Exception());
                Jump(exit);
            }
            Bind(&notHasException0);
            {
                BRANCH_UNLIKELY(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
                Bind(&callDispatch);
                GateRef key = Int64ToTaggedInt(*i);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = {argHandle, *kValue, key, thisValue};
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
                    if (visitKind == kEvery) {
                        BRANCH_NO_WEIGHT(TaggedIsFalse(FastToBoolean(retValue)), &returnFalse, &loopEnd);
                    }
                    if (visitKind == kSome) {
                        BRANCH_NO_WEIGHT(TaggedIsTrue(FastToBoolean(retValue)), &returnTrue, &loopEnd);
                    }
                    if (visitKind == kForEach) {
                        Jump(&loopEnd);
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
    }
    if (visitKind == kEvery) {
        Bind(&returnFalse);
        {
            result->WriteVariable(TaggedFalse());
            Jump(exit);
        }
    }
    if (visitKind == kSome) {
        Bind(&returnTrue);
        {
            result->WriteVariable(TaggedTrue());
            Jump(exit);
        }
    }
}

void BuiltinsArrayStubBuilder::PopOptimised(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
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
    BRANCH(IsJsCOWArray(thisValue), &isJsCOWArray, &getElements);
    Bind(&isJsCOWArray);
    {
        NewObjectStubBuilder newBuilder(this);
        GateRef elements = GetElementsArray(thisValue);
        GateRef capacity = GetLengthOfTaggedArray(elements);
        GateRef newElements = newBuilder.CopyArray(glue, elements, capacity, capacity);
        SetElementsArray(VariableType::JS_POINTER(), glue, thisValue, newElements);
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

    GateRef enableMutant = IsEnableMutantArray(glue);
    DEFVARIABLE(element, VariableType::JS_ANY(), Hole());
    BRANCH(Int64LessThan(index, capacity), &inRange, &trimCheck);
    Bind(&inRange);
    {
        Label enableMutantArray(env);
        Label disableMutantArray(env);
        BRANCH(enableMutant, &enableMutantArray, &disableMutantArray);
        Bind(&enableMutantArray);
        {
            element = GetTaggedValueWithElementsKind(glue, thisValue, index);
            Jump(&trimCheck);
        }
        Bind(&disableMutantArray);
        {
            element = GetValueFromTaggedArray(elements, TruncInt64ToInt32(index));
            Jump(&trimCheck);
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
        Label enableMutantArray(env);
        Label disableMutantArray(env);
        BRANCH(enableMutant, &enableMutantArray, &disableMutantArray);
        Bind(&enableMutantArray);
        {
            SetValueWithElementsKind(glue, thisValue, Hole(), index, Boolean(false),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            Jump(&setNewLen);
        }
        Bind(&disableMutantArray);
        {
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements,
                                  TruncInt64ToInt32(index), Hole(), MemoryAttribute::NoBarrier());
            Jump(&setNewLen);
        }
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
}

void BuiltinsArrayStubBuilder::SliceOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label noConstructor(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // need check constructor, "Slice" should use ArraySpeciesCreate
    BRANCH(HasConstructor(thisValue), slowPath, &noConstructor);
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
        NewObjectStubBuilder newBuilder(this);
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
                        Label mutantArrayEnabled(env);
                        Label notMutantArrayEnabled(env);
                        BRANCH_NO_WEIGHT(IsEnableMutantArray(glue), &mutantArrayEnabled, &notMutantArrayEnabled);
                        Bind(&mutantArrayEnabled);
                        {
                            GateRef newArray = NewArray(glue, *count);
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
                                                         Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&loopEnd);
                            idx = Int64Add(*idx, Int64(1));
                            LoopEnd(&loopHead, env, glue);
                            Bind(&loopExit);
                            result->WriteVariable(newArray);
                            Jump(exit);
                        }
                        Bind(&notMutantArrayEnabled);
                        {
                            GateRef kind = ComputeTaggedArrayElementKind(thisValue, *start, *end);
                            GateRef elements = GetElementsArray(thisValue);
                            NewObjectStubBuilder newBuilder(this);
                            newBuilder.SetGlue(glue);
                            GateRef destElements = newBuilder.NewTaggedArray(glue, TruncInt64ToInt32(*count));
                            GateRef sourceStart = GetDataPtrInTaggedArray(elements, *start);
                            GateRef dest = GetDataPtrInTaggedArray(destElements);
                            ArrayCopy(glue, elements, sourceStart, destElements, dest,
                                      TruncInt64ToInt32(*count), NeedBarrier(kind), DifferentArray);
                            GateRef array = newBuilder.CreateArrayFromList(glue, destElements, kind);
                            result->WriteVariable(array);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}
void BuiltinsArrayStubBuilder::ShiftOptimised(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
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
                BRANCH(IsJsCOWArray(thisValue), &isJsCOWArray, &getElements);
                Bind(&isJsCOWArray);
                {
                    NewObjectStubBuilder newBuilder(this);
                    GateRef elements = GetElementsArray(thisValue);
                    GateRef capacity = GetLengthOfTaggedArray(elements);
                    GateRef newElements = newBuilder.CopyArray(glue, elements, capacity, capacity);
                    SetElementsArray(VariableType::JS_POINTER(), glue, thisValue, newElements);
                    Jump(&getElements);
                }
                Bind(&getElements);
                {
                    GateRef enableMutant = IsEnableMutantArray(glue);
                    GateRef elements = GetElementsArray(thisValue);
                    GateRef capacity = ZExtInt32ToInt64(GetLengthOfTaggedArray(elements));
                    GateRef index = Int64Sub(thisLen, Int64(1));
                    Label enableMutantArray(env);
                    Label disableMutantArray(env);
                    Label elementExit(env);
                    Label copyExit(env);
                    DEFVARIABLE(element, VariableType::JS_ANY(), Hole());
                    BRANCH(enableMutant, &enableMutantArray, &disableMutantArray);
                    Bind(&enableMutantArray);
                    {
                        element = GetTaggedValueWithElementsKind(glue, thisValue, Int64(0));
                        Jump(&elementExit);
                    }
                    Bind(&disableMutantArray);
                    {
                        element = GetValueFromTaggedArray(elements, Int64(0));
                        Jump(&elementExit);
                    }
                    Bind(&elementExit);
                    GateRef kind = GetElementsKindFromHClass(LoadHClass(thisValue));
                    GateRef dest = GetDataPtrInTaggedArray(elements);
                    GateRef start = PtrAdd(dest, IntPtr(JSTaggedValue::TaggedTypeSize()));
                    ArrayCopy(glue, elements, start, elements, dest,
                              TruncInt64ToInt32(index), NeedBarrier(kind), SameArray);
                    Jump(&copyExit);
                    Bind(&copyExit);
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
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements,
                                                  TruncInt64ToInt32(index), Hole(), MemoryAttribute::NoBarrier());
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

GateRef BuiltinsArrayStubBuilder::CalEleKindForNewArrayNoHole(GateRef thisValue, GateRef thisLen,
                                                              GateRef actualIndex, GateRef insertVal)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef beforePartEleKind = ComputeTaggedArrayElementKind(thisValue, Int64(0), actualIndex);
    GateRef afterPartEleKind = ComputeTaggedArrayElementKind(thisValue, Int64Add(actualIndex, Int64(1)), thisLen);
    result = Int32Or(beforePartEleKind, TaggedToElementKind(insertVal));
    result = FixElementsKind(Int32Or(*result, afterPartEleKind));

    Label haveHole(env);
    GateRef isHaveHole = Int32Equal(Int32(1), Int32And(*result, Int32(static_cast<int32_t>(ElementsKind::HOLE))));
    BRANCH(isHaveHole, &haveHole, &exit);
    Bind(&haveHole);
    {
        result = Int32(static_cast<int32_t>(ElementsKind::TAGGED));
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsArrayStubBuilder::FastArrayWith(GateRef glue, GateRef thisValue, GateRef newArray,
                                             GateRef actualIndex, GateRef insertValue, GateRef newArrEleKind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    // copy elements before actualIndex
    GateRef srcElements = GetElementsArray(thisValue);
    GateRef dstElements = GetElementsArray(newArray);
    GateRef srcStart = GetDataPtrInTaggedArray(srcElements);
    GateRef dstStart = GetDataPtrInTaggedArray(dstElements);
    ArrayCopyAndHoleToUndefined(glue, srcElements, srcStart, dstElements,
                                dstStart, actualIndex, NeedBarrier(newArrEleKind));
    // set insertValue in new array
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, dstElements, actualIndex, insertValue);
    // copy elements before actualIndex
    GateRef copyAfterIdx = Int32Add(actualIndex, Int32(1));
    srcStart = GetDataPtrInTaggedArray(srcElements, copyAfterIdx);
    dstStart = GetDataPtrInTaggedArray(dstElements, copyAfterIdx);
    GateRef thisLength = GetLengthOfJSArray(thisValue);
    ArrayCopyAndHoleToUndefined(glue, srcElements, srcStart, dstElements, dstStart,
                                Int32Sub(thisLength, copyAfterIdx), NeedBarrier(newArrEleKind));

    SetArrayLength(glue, newArray, thisLength);
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsArrayStubBuilder::WithOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result,
                                             Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(relativeIndex, VariableType::INT64(), Int64(0));
    DEFVARIABLE(actualIndex, VariableType::INT64(), Int64(0));
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStableArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(IsStableJSArray(glue, thisValue), &isStableArray, slowPath);
    Bind(&isStableArray);
    // don't check constructor, "with" always use ArrayCreate to create array.

    // don't check COW array, "With" won't modify original array.

    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    GateRef index = GetCallArg0(numArgs);
    Label taggedIsInt(env);
    BRANCH(TaggedIsInt(index), &taggedIsInt, slowPath);
    Bind(&taggedIsInt);
    {
        relativeIndex = GetInt64OfTInt(index);
        DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
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
                    Label enableMutantArrayWith(env);
                    Label fastArrayWith(env);
                    BRANCH_UNLIKELY(IsEnableMutantArray(glue), &enableMutantArrayWith, &fastArrayWith);
                    Bind(&fastArrayWith);
                    {
                        GateRef newArrayEleKind = CalEleKindForNewArrayNoHole(thisValue, thisLen,
                                                                              *actualIndex, *value);
                        GateRef newHClass =  GetElementsKindHClass(glue, newArrayEleKind);
                        GateRef newArray = NewEmptyArrayWithHClass(glue, newHClass, TruncInt64ToInt32(thisLen));
                        FastArrayWith(glue, thisValue, newArray, TruncInt64ToInt32(*actualIndex),
                                      *value, newArrayEleKind);
                        result->WriteVariable(newArray);
                        Jump(exit);
                    }
                    Bind(&enableMutantArrayWith);
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
                                Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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
                                    Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                Jump(&loopEnd);
                            }
                            Bind(&eleNotHole);
                            {
                                SetValueWithElementsKind(glue, newArray, ele, *k, Boolean(true),
                                    Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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
}

void BuiltinsArrayStubBuilder::ConcatOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                               Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    {
        Label isExtensible(env);
        // need check constructor, "Concat" should use ArraySpeciesCreate
        BRANCH(HasConstructor(thisValue), slowPath, &isExtensible);
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
                        NewObjectStubBuilder newBuilder(this);
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
                            Label enabledMutantArray(env);
                            Label disableMutantArray(env);
                            BRANCH(IsEnableMutantArray(glue), &enabledMutantArray, &disableMutantArray);
                            Bind(&enabledMutantArray);
                            {
                                DoConcat(glue, thisValue, arg0, result, exit, thisLen, argLen, sumArrayLen);
                            }
                            Bind(&disableMutantArray);
                            {
                                GateRef kind1 = GetElementsKindFromHClass(LoadHClass(thisValue));
                                GateRef kind2 = GetElementsKindFromHClass(LoadHClass(arg0));
                                GateRef tmpKind = Int32Or(kind1, kind2);
                                GateRef newKind = FixElementsKind(tmpKind);
                                GateRef thisElements = GetElementsArray(thisValue);
                                GateRef argElements = GetElementsArray(arg0);
                                NewObjectStubBuilder newBuilder(this);
                                GateRef newElements = newBuilder.NewTaggedArray(glue, TruncInt64ToInt32(sumArrayLen));
                                GateRef dst1 = GetDataPtrInTaggedArray(newElements);
                                GateRef dst2 = PtrAdd(dst1, PtrMul(thisLen, IntPtr(JSTaggedValue::TaggedTypeSize())));
                                ArrayCopy(glue, thisElements, GetDataPtrInTaggedArray(thisElements),
                                          newElements, dst1, TruncInt64ToInt32(thisLen), NeedBarrier(kind1),
                                          DifferentArray);
                                ArrayCopy(glue, argElements, GetDataPtrInTaggedArray(argElements),
                                          newElements, dst2, TruncInt64ToInt32(argLen), NeedBarrier(kind2),
                                          DifferentArray);
                                GateRef array = newBuilder.CreateArrayFromList(glue, newElements, newKind);
                                result->WriteVariable(array);
                                Jump(exit);
                            }
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsArrayStubBuilder::DoConcat(GateRef glue, GateRef thisValue, GateRef arg0, Variable *result, Label *exit,
                                        GateRef thisLen, GateRef argLen, GateRef sumArrayLen)
{
    auto env = GetEnvironment();
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
    BRANCH(TaggedIsException(newArray), exit, &setProperties);
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
            BRANCH(Int64LessThan(*i, thisLen), &next, &loopExit);
            Bind(&next);
            GateRef ele = GetTaggedValueWithElementsKind(glue, thisValue, *i);
            #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
            SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                Int32(static_cast<uint32_t>(ElementsKind::GENERIC)));
            #else
            SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                Int32(static_cast<uint32_t>(ElementsKind::NONE)));
            #endif
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        j = Int64Add(*j, Int64(1));
        LoopEnd(&loopHead, env, glue);
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
                                     Int32(static_cast<uint32_t>(ElementsKind::GENERIC)));
            #else
            SetValueWithElementsKind(glue, newArray, ele, *j, Boolean(true),
                                     Int32(static_cast<uint32_t>(ElementsKind::NONE)));
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

void BuiltinsArrayStubBuilder::FillOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
                                             Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    Label notCOWArray(env);
    BRANCH(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);
    GateRef arrayCls = LoadHClass(thisValue);
    // 1. Let O be ToObject(this value).
    // 2 ReturnIfAbrupt(O).
    Label hasException(env);
    Label proNotCOWArray(env);
    GateRef prop = GetPropertiesFromJSObject(thisValue);
    BRANCH(IsCOWArray(prop), slowPath, &proNotCOWArray);
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
    Label startArgIsNumber(env);
    // 5. let relativeStart be ToInteger(start).
    GateRef startArg = GetCallArg1(numArgs);
    // 6 ReturnIfAbrupt(relativeStart).
    BRANCH(TaggedIsNumber(startArg), &startArgIsNumber, slowPath);
    Bind(&startArgIsNumber);
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
    Label endArgIsNumber(env);
    // 8. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    GateRef endArg = GetCallArg2(numArgs);
    BRANCH(TaggedIsNumber(endArg), &endArgIsNumber, slowPath);
    Bind(&endArgIsNumber);
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
        Label defaultElements(env);
        Label startFill(env);
        Label fatal(env);
        GateRef elementKind = GetElementsKindFromHClass(arrayCls);
        TransitToElementsKind(glue, thisValue, value, elementKind);
        DEFVARIABLE(migratedValue, VariableType::JS_ANY(), value);
        DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(thisValue));
        GateRef mutant = IsMutantTaggedArray(*elements);
        GateRef elementLen = GetLengthOfTaggedArray(*elements);
        BRANCH(Int32GreaterThanOrEqual(elementLen, *end), &defaultElements, &fatal);
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
        Bind(&fatal);
        {
            FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable))});
            Jump(exit);
        }
        Bind(&startFill);
        Label noBarrier(env);
        Label needBarrier(env);
        Label needRevise(env);
        Label noRevise(env);
        Label startLessEnd(env);
        Label barrierExit(env);
        BRANCH(Int32LessThan(*start, *end), &startLessEnd, &noRevise);
        Bind(&startLessEnd);
        {
            GateRef count = Int32Sub(*end, *start);
            BRANCH(mutant, &noBarrier, &needBarrier);
            Bind(&noBarrier);
            {
                FastFill(glue, *elements, *start, count, *migratedValue, false);
                Jump(&barrierExit);
            }
            Bind(&needBarrier);
            {
                FastFill(glue, *elements, *start, count, *migratedValue, true);
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
}

void BuiltinsArrayStubBuilder::FastFill(GateRef glue, GateRef element, GateRef start, GateRef count,
                                        GateRef value, bool needBarrier)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    GateRef dstAddr = GetDataPtrInTaggedArray(element, start);
    CallNGCRuntime(glue, RTSTUB_ID(FillObject), {TaggedCastToIntPtr(dstAddr), value, count});
    if (needBarrier) {
        CallCommonStub(glue, CommonStubCSigns::BatchBarrier,
            {glue, TaggedCastToIntPtr(element), TaggedCastToIntPtr(dstAddr), count});
    }
    Jump(&exit);
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsArrayStubBuilder::ReverseOptimised(GateRef glue, GateRef thisValue, Variable *result,
    Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStability(env);
    Label notCOWArray(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    // don't check constructor, "Reverse" won't create new array.
    BRANCH(IsStableJSArray(glue, thisValue), &isStability, slowPath);
    Bind(&isStability);
    BRANCH(IsJsCOWArray(thisValue), slowPath, &notCOWArray);
    Bind(&notCOWArray);

    GateRef thisLen = GetArrayLength(thisValue);
    GateRef hclass = LoadHClass(thisValue);
    GateRef kind = GetElementsKindFromHClass(hclass);
    Label shouldBarrier(env);
    Label noBarrier(env);
    Label afterReverse(env);
    GateRef element = GetElementsArray(thisValue);
    GateRef dstAddr = GetDataPtrInTaggedArray(element);
    CallNGCRuntime(glue, RTSTUB_ID(ReverseArray), {TaggedCastToIntPtr(dstAddr), thisLen});
    BRANCH(NeedBarrier(kind), &shouldBarrier, &afterReverse);
    Bind(&shouldBarrier);
    {
        CallCommonStub(glue, CommonStubCSigns::BatchBarrier,
            {glue, TaggedCastToIntPtr(element), TaggedCastToIntPtr(dstAddr), thisLen});
        Jump(&afterReverse);
    }
    Bind(&afterReverse);
    result->WriteVariable(thisValue);
    Jump(exit);
}

void BuiltinsArrayStubBuilder::IncludesIndexOfOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
    MethodKind mk, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isDictMode(env);
    Label isHeapObject(env);
    Label isJsArray(env);
    Label isStableJsArray(env);
    Label notFound(env);
    Label thisLenNotZero(env);
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH(IsJsArray(thisValue), &isJsArray, slowPath);
    Bind(&isJsArray);
    BRANCH(IsStableJSArray(glue, thisValue), &isStableJsArray, slowPath);
    Bind(&isStableJsArray);
    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    BRANCH(Int64Equal(thisLen, Int64(0)), &notFound, &thisLenNotZero);
    Bind(&thisLenNotZero);
    {
        DEFVARIABLE(fromIndex, VariableType::INT64(), Int64(0));
        Label getArgTwo(env);
        Label nextProcess(env);
        BRANCH(Int64Equal(numArgs, IntPtr(2)), &getArgTwo, &nextProcess); // 2: 2 parameters
        Bind(&getArgTwo);
        {
            Label secondArgIsInt(env);
            GateRef fromIndexTemp = GetCallArg1(numArgs);
            BRANCH(TaggedIsInt(fromIndexTemp), &secondArgIsInt, slowPath);
            Bind(&secondArgIsInt);
            fromIndex = SExtInt32ToInt64(TaggedGetInt(fromIndexTemp));
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
            BRANCH(Int64GreaterThanOrEqual(*fromIndex, thisLen), &notFound, &nextCheck);
            Bind(&nextCheck);
            {
                GateRef negThisLen = Int64Sub(Int64(0), thisLen);
                BRANCH(Int64LessThan(*fromIndex, negThisLen), &setBackZero, &calculateFrom);
                Bind(&setBackZero);
                {
                    fromIndex = Int64(0);
                    Jump(&calculateFrom);
                }
                Bind(&calculateFrom);
                {
                    DEFVARIABLE(from, VariableType::INT64(), Int64(0));
                    Label fromIndexGreaterOrEqualZero(env);
                    Label fromIndexLessThanZero(env);
                    Label startLoop(env);
                    BRANCH(Int64GreaterThanOrEqual(*fromIndex, Int64(0)),
                        &fromIndexGreaterOrEqualZero, &fromIndexLessThanZero);
                    Bind(&fromIndexGreaterOrEqualZero);
                    {
                        from = *fromIndex;
                        Jump(&startLoop);
                    }
                    Bind(&fromIndexLessThanZero);
                    {
                        Label isLenFromIndex(env);
                        GateRef lenFromIndexSum = Int64Add(thisLen, *fromIndex);
                        BRANCH(Int64GreaterThanOrEqual(lenFromIndexSum, Int64(0)), &isLenFromIndex, &startLoop);
                        Bind(&isLenFromIndex);
                        {
                            from = lenFromIndexSum;
                            Jump(&startLoop);
                        }
                    }
                    Bind(&startLoop);
                    {
                        GateRef searchElement = GetCallArg0(numArgs);
                        Label enabledMutantArray(env);
                        Label disabledMutantArray(env);
                        Label beforeExit(env);
                        BRANCH(IsEnableMutantArray(glue), &enabledMutantArray, &disabledMutantArray);
                        Bind(&enabledMutantArray);
                        {
                            Label loopHead(env);
                            Label loopEnd(env);
                            Label next(env);
                            Label loopExit(env);
                            Jump(&loopHead);
                            LoopBegin(&loopHead);
                            {
                                BRANCH(Int64LessThan(*from, thisLen), &next, &loopExit);
                                Bind(&next);
                                {
                                    DEFVARIABLE(valueEqual, VariableType::BOOL(), False());
                                    Label notHoleOrUndefValue(env);
                                    Label valueFound(env);
                                    GateRef value = GetTaggedValueWithElementsKind(glue, thisValue, *from);
                                    GateRef isHole = TaggedIsHole(value);
                                    GateRef isUndef = TaggedIsUndefined(value);
                                    BRANCH(BitOr(isHole, isUndef), slowPath, &notHoleOrUndefValue);
                                    Bind(&notHoleOrUndefValue);
                                    if (mk == M_INCLUDES) {
                                        valueEqual = SameValueZero(glue, searchElement, value);
                                    } else {
                                        valueEqual = FastStrictEqual(glue, searchElement, value, ProfileOperation());
                                    }
                                    BRANCH(*valueEqual, &valueFound, &loopEnd);
                                    Bind(&valueFound);
                                    {
                                        result->WriteVariable(IntToTaggedPtr(*from));
                                        Jump(&beforeExit);
                                    }
                                }
                            }
                            Bind(&loopEnd);
                            from = Int64Add(*from, Int64(1));
                            LoopEnd(&loopHead);
                            Bind(&loopExit);
                            result->WriteVariable(IntToTaggedPtr(Int32(-1)));
                            Jump(&beforeExit);
                        }
                        Bind(&disabledMutantArray);
                        {
                            Label isintOrHoleInt(env);
                            Label notintOrHoleInt(env);
                            Label isNumberOrHoleNumber(env);
                            Label notNumberOrHoleNumber(env);
                            Label isObjectOrHoleObject(env);
                            Label isString(env);
                            Label notString(env);
                            Label isObject(env);
                            Label isTagged(env);
                            Label isUndef(env);
                            Label notUndef(env);
                            GateRef elements = GetElementsArray(thisValue);
                            GateRef kind = GetElementsKindFromHClass(LoadHClass(thisValue));
                            BRANCH(TaggedIsUndefined(searchElement), &isUndef, &notUndef);
                            Bind(&isUndef);
                            {
                                Label containUdef(env);
                                Label notContainUdef(env);
                                GateRef taggedOrHoleTagged = LogicOrBuilder(env)
                                    .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::TAGGED))))
                                    .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_TAGGED))))
                                    .Done();
                                BRANCH_NO_WEIGHT(taggedOrHoleTagged, &containUdef, &notContainUdef);
                                Bind(&containUdef);
                                {
                                    if (mk == M_INCLUDES) {
                                        UndefinedHoleLoop(elements,
                                            *from, thisLen, UndefOrHole::K_ALL, result, &beforeExit);
                                    } else {
                                        UndefinedHoleLoop(elements,
                                            *from, thisLen, UndefOrHole::K_UNDEFINED, result, &beforeExit);
                                    }
                                }
                                Bind(&notContainUdef);
                                {
                                    if (mk == M_INCLUDES) {
                                        UndefinedHoleLoop(elements,
                                            *from, thisLen, UndefOrHole::K_HOLE, result, &beforeExit);
                                    } else {
                                        Jump(&notFound);
                                    }
                                }
                            }
                            Bind(&notUndef);
                            GateRef intOrHoleInt = LogicOrBuilder(env)
                                .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::INT))))
                                .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_INT))))
                                .Done();
                            BRANCH_NO_WEIGHT(intOrHoleInt, &isintOrHoleInt, &notintOrHoleInt);
                            Bind(&notintOrHoleInt);
                            {
                                GateRef numberOrHoleNumber = LogicOrBuilder(env)
                                    .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::NUMBER))))
                                    .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_NUMBER))))
                                    .Done();
                                BRANCH_NO_WEIGHT(numberOrHoleNumber, &isNumberOrHoleNumber, &notNumberOrHoleNumber);
                                Bind(&notNumberOrHoleNumber);
                                {
                                    GateRef stirngOrHoleString = LogicOrBuilder(env)
                                        .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::STRING))))
                                        .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_STRING))))
                                        .Done();
                                    BRANCH_NO_WEIGHT(stirngOrHoleString, &isString, &notString);
                                    Bind(&notString);
                                    GateRef objectOrHoleObject = LogicOrBuilder(env)
                                        .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::OBJECT))))
                                        .Or(Int32Equal(kind, Int32(static_cast<int32_t>(ElementsKind::HOLE_OBJECT))))
                                        .Done();
                                    BRANCH_NO_WEIGHT(objectOrHoleObject, &isObject, &isTagged);
                                }
                            }
                            Bind(&isintOrHoleInt);
                            {
                                IntIncludesIndexOf(elements, *from, searchElement, thisLen, result, &beforeExit);
                            }
                            Bind(&isNumberOrHoleNumber);
                            {
                                DoubleIncludesIndexOf(glue, elements, *from,
                                    searchElement, thisLen, mk, result, &beforeExit);
                            }
                            Bind(&isString);
                            {
                                GateRef equalResult = CallRuntime(glue, RTSTUB_ID(StringIndexOf),
                                    {thisValue, searchElement, Int64ToTaggedInt(*from), Int64ToTaggedInt(thisLen)});
                                result->WriteVariable(equalResult);
                                Jump(&beforeExit);
                            }
                            Bind(&isObject);
                            {
                                ObjectIncludesIndexOf(elements, *from, searchElement, thisLen, result, &beforeExit);
                            }
                            Bind(&isTagged);
                            {
                                GenericIncludesIndexOf(glue, thisValue,
                                                       *from, searchElement, thisLen, mk, result, &beforeExit);
                            }
                        }
                        Bind(&beforeExit);
                        {
                            Label resFound(env);
                            GateRef tempRes = GetInt32OfTInt(result->ReadVariable());
                            BRANCH(Int32Equal(tempRes, Int32(-1)), &notFound, &resFound);
                            Bind(&resFound);
                            {
                                if (mk == M_INCLUDES) {
                                    result->WriteVariable(TaggedTrue());
                                }
                                Jump(exit);
                            }
                        }
                    }
                }
            }
        }
    }
    Bind(&notFound);
    {
        if (mk == M_INCLUDES) {
            result->WriteVariable(TaggedFalse());
        } else {
            result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        }
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::IntIncludesIndexOf(GateRef elements, GateRef fromIndex, GateRef searchElement,
    GateRef thisLen, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label notFound(env);
    Label Found(env);
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label isDouble(env);
    Label isNumber(env);
    Label taggedIntEqual(env);
    Label isUndef(env);
    BRANCH(TaggedIsNumber(searchElement), &isNumber, &notFound);
    Bind(&isNumber);
    {
        DEFVARIABLE(searchValue, VariableType::JS_ANY(), searchElement);
        BRANCH_LIKELY(TaggedIsInt(searchElement), &taggedIntEqual, &isDouble);
        Bind(&isDouble);
        {
            GateRef doubleSearch = GetDoubleOfTDouble(searchElement);
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &notFound);
                Bind(&next);
                {
                    Label valueIsInt(env);
                    GateRef value = GetValueFromTaggedArray(elements, *from);
                    BRANCH(TaggedIsInt(value), &valueIsInt, &loopEnd);
                    Bind(&valueIsInt);
                    {
                        GateRef doubleVal = ChangeInt32ToFloat64(GetInt32OfTInt(value));
                        BRANCH(DoubleEqual(doubleVal, doubleSearch), &Found, &loopEnd);
                    }
                }
            }
            Bind(&loopEnd);
            from = Int64Add(*from, Int64(1));
            LoopEnd(&loopHead);
        }
        Bind(&taggedIntEqual);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &loopExit);
                Bind(&next);
                {
                    GateRef value = GetValueFromTaggedArray(elements, *from); // tint or hole.
                    BRANCH(Equal(value, *searchValue), &Found, &loopEnd);
                }
            }
            Bind(&loopEnd);
            from = Int64Add(*from, Int64(1));
            LoopEnd(&loopHead);
            Bind(&loopExit);
            Jump(&notFound);
        }
    }
    Bind(&Found);
    {
        result->WriteVariable(IntToTaggedPtr(*from));
        Jump(exit);
    }
    Bind(&notFound);
    {
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::DoubleIncludesIndexOf(GateRef glue, GateRef elements, GateRef fromIndex,
    GateRef searchElement, GateRef thisLen, MethodKind mk, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label isNumber(env);
    Label notNumber(env);
    BRANCH_LIKELY(TaggedIsNumber(searchElement), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        NumberLoop(glue, elements, fromIndex, searchElement, thisLen, mk, result, exit);
    }
    Bind(&notNumber);
    {
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::ObjectIncludesIndexOf(GateRef elements, GateRef fromIndex, GateRef searchElement,
    GateRef thisLen, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label Found(env);
    Label notFound(env);
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label isObject(env);
    Label isUndef(env);
    BRANCH(IsEcmaObject(searchElement), &isObject, &notFound);
    Bind(&isObject);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &loopExit);
            Bind(&next);
            {
                Label eleIsObject(env);
                GateRef value = GetValueFromTaggedArray(elements, *from); // object or hole.
                BRANCH(Equal(value, searchElement), &Found, &loopEnd);
                Bind(&Found);
                {
                    result->WriteVariable(IntToTaggedPtr(*from));
                    Jump(exit);
                }
            }
        }
        Bind(&loopEnd);
        from = Int64Add(*from, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&notFound);
    }
    Bind(&notFound);
    {
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::GenericIncludesIndexOf(GateRef glue, GateRef thisValue, GateRef fromIndex,
    GateRef searchElement, GateRef thisLen, MethodKind mk, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label Found(env);
    Label notFound(env);
    Label isUndef(env);
    Label notUndef(env);
    Label isNumber(env);
    Label notNumber(env);
    Label isString(env);
    Label notString(env);
    Label isBingInt(env);
    Label objectEqual(env);
    GateRef elements = GetElementsArray(thisValue);
    BRANCH_LIKELY(TaggedIsNumber(searchElement), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        NumberLoop(glue, elements, fromIndex, searchElement, thisLen, mk, result, exit);
    }
    Bind(&notNumber);
    {
        BRANCH(TaggedIsString(searchElement), &isString, &notString);
        Bind(&notString);
        BRANCH(TaggedIsBigInt(searchElement), &isBingInt, &objectEqual);
        Bind(&objectEqual); // Object Equal
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &notFound);
                Bind(&next);
                {
                    GateRef value = GetValueFromTaggedArray(elements, *from);
                    BRANCH(Equal(value, searchElement), &Found, &loopEnd);
                }
            }
            Bind(&loopEnd);
            from = Int64Add(*from, Int64(1));
            LoopEnd(&loopHead);
        }
        Bind(&isString); // String Equal
        {
            GateRef equalResult = CallRuntime(glue, RTSTUB_ID(StringIndexOf),
                {thisValue, searchElement, Int64ToTaggedInt(*from), Int64ToTaggedInt(thisLen)});
            result->WriteVariable(equalResult);
            Jump(exit);
        }
        Bind(&isBingInt); // BigInt Equal
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &notFound);
                Bind(&next);
                {
                    Label valueIsBigInt(env);
                    Label valueNotNumber(env);
                    Label valueNotEqual(env);
                    Label bigintEqual(env);
                    GateRef value = GetValueFromTaggedArray(elements, *from);
                    BRANCH(TaggedIsNumber(value), &loopEnd, &valueNotNumber);
                    Bind(&valueNotNumber);
                    BRANCH(TaggedIsBigInt(value), &valueIsBigInt, &loopEnd);
                    Bind(&valueIsBigInt);
                    {
                        BRANCH(Equal(value, searchElement), &Found, &bigintEqual);
                        Bind(&bigintEqual);
                        GateRef bigIntResult =
                            CallNGCRuntime(glue, RTSTUB_ID(BigIntSameValueZero), { value, searchElement });
                        BRANCH(bigIntResult, &Found, &loopEnd);
                    }
                }
            }
            Bind(&loopEnd);
            from = Int64Add(*from, Int64(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&Found);
    {
        result->WriteVariable(IntToTaggedPtr(*from));
        Jump(exit);
    }
    Bind(&notFound);
    {
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::UndefinedHoleLoop(GateRef elements, GateRef fromIndex, GateRef thisLen,
    UndefOrHole uoh, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    DEFVARIABLE(res, VariableType::BOOL(), False());
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &loopExit);
        Bind(&next);
        {
            Label valueFound(env);
            GateRef value = GetValueFromTaggedArray(elements, *from);
            if (uoh == UndefOrHole::K_UNDEFINED) {
                res = TaggedIsUndefined(value);
            } else if (uoh == UndefOrHole::K_HOLE) {
                res = TaggedIsHole(value);
            } else {
                res = BitOr(TaggedIsUndefined(value), TaggedIsHole(value));
            }
            BRANCH(*res, &valueFound, &loopEnd);
            Bind(&valueFound);
            {
                result->WriteVariable(IntToTaggedPtr(*from));
                Jump(exit);
            }
        }
    }
    Bind(&loopEnd);
    from = Int64Add(*from, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    result->WriteVariable(IntToTaggedPtr(Int32(-1)));
    Jump(exit);
}

void BuiltinsArrayStubBuilder::NaNLoop(GateRef elements, GateRef fromIndex,
    GateRef thisLen, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label isFound(env);
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &loopExit);
        Bind(&next);
        {
            Label eleIsDouble(env);
            GateRef value = GetValueFromTaggedArray(elements, *from);
            BRANCH(TaggedIsDouble(value), &eleIsDouble, &loopEnd);
            Bind(&eleIsDouble);
            BRANCH(DoubleIsNAN(GetDoubleOfTDouble(value)), &isFound, &loopEnd);
            Bind(&isFound);
            {
                result->WriteVariable(IntToTaggedPtr(*from));
                Jump(exit);
            }
        }
    }
    Bind(&loopEnd);
    from = Int64Add(*from, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    result->WriteVariable(IntToTaggedPtr(Int32(-1)));
    Jump(exit);
}

void BuiltinsArrayStubBuilder::NumberLoop(GateRef glue, GateRef elements, GateRef fromIndex, GateRef searchElement,
    GateRef thisLen, MethodKind mk, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label notFound(env);
    Label Found(env);
    Label isInt(env);
    Label isDouble(env);
    Label isNaN(env);
    Label notNaN(env);
    BRANCH(TaggedIsInt(searchElement), &isInt, &isDouble);
    Bind(&isInt);
    {
        Label isZero(env);
        Label notZero(env);
        GateRef searchInt = GetInt32OfTInt(searchElement);
        BRANCH(Int32Equal(searchInt, Int32(0)), &isZero, &notZero);
        Bind(&isZero);
        {
            ZeroLoop(elements, *from, thisLen, result, exit);
        }
        Bind(&notZero);
        GateRef searchDouble = ChangeInt32ToFloat64(searchInt);
        GateRef searchDoubleTagged = DoubleToTaggedDoublePtr(searchDouble);
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &notFound);
            Bind(&next);
            {
                Label eleNotInt(env);
                Label eleIsDouble(env);
                GateRef value = GetValueFromTaggedArray(elements, *from);
                BRANCH(Equal(value, searchElement), &Found, &eleNotInt);
                Bind(&eleNotInt);
                {
                    BRANCH(TaggedIsDouble(value), &eleIsDouble, &loopEnd);
                    Bind(&eleIsDouble);
                    BRANCH(Equal(value, searchDoubleTagged), &Found, &loopEnd);
                }
            }
        }
        Bind(&loopEnd);
        from = Int64Add(*from, Int64(1));
        LoopEnd(&loopHead);
    }
    Bind(&isDouble);
    {
        GateRef doubleUntagged = GetDoubleOfTDouble(searchElement);
        BRANCH_UNLIKELY(DoubleIsNAN(doubleUntagged), &isNaN, &notNaN);
        Bind(&isNaN);
        {
            if (mk == M_INCLUDES) {
                NaNLoop(elements, fromIndex, thisLen, result, exit);
            } else {
                Jump(&notFound);
            }
        }
        Bind(&notNaN);
        {
            Label isZero(env);
            Label notZero(env);
            BRANCH(DoubleEqual(doubleUntagged, Double(0.0)), &isZero, &notZero);
            Bind(&isZero);
            {
                ZeroLoop(elements, *from, thisLen, result, exit);
            }
            Bind(&notZero);
            GateRef intVal = DoubleToInt(glue, doubleUntagged);
            GateRef intToDoubleVal = ChangeInt32ToFloat64(intVal);
            Label withinInt32(env);
            Label notWithinInt32(env);
            BRANCH(DoubleEqual(doubleUntagged, intToDoubleVal), &withinInt32, &notWithinInt32);
            Bind(&withinInt32);
            {
                GateRef targetInt32 = IntToTaggedPtr(intVal);
                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &notFound);
                    Bind(&next);
                    {
                        GateRef value = GetValueFromTaggedArray(elements, *from);
                        GateRef equalRes = LogicOrBuilder(env)
                            .Or(Equal(value, searchElement))
                            .Or(Equal(value, targetInt32))
                            .Done();
                        BRANCH(equalRes, &Found, &loopEnd);
                    }
                }
                Bind(&loopEnd);
                from = Int64Add(*from, Int64(1));
                LoopEnd(&loopHead);
            }
            Bind(&notWithinInt32);
            {
                Label loopHead(env);
                Label loopEnd(env);
                Label next(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &notFound);
                    Bind(&next);
                    {
                        GateRef value = GetValueFromTaggedArray(elements, *from);
                        BRANCH(Equal(value, searchElement), &Found, &loopEnd);
                    }
                }
                Bind(&loopEnd);
                from = Int64Add(*from, Int64(1));
                LoopEnd(&loopHead);
            }
        }
    }
    Bind(&Found);
    {
        result->WriteVariable(IntToTaggedPtr(*from));
        Jump(exit);
    }
    Bind(&notFound);
    {
        result->WriteVariable(IntToTaggedPtr(Int32(-1)));
        Jump(exit);
    }
}

void BuiltinsArrayStubBuilder::ZeroLoop(GateRef elements, GateRef fromIndex,
    GateRef thisLen, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label isFound(env);
    DEFVARIABLE(from, VariableType::INT64(), fromIndex);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    GateRef valueZero = Int64ToTaggedPtr(Int64(JSTaggedValue::VALUE_ZERO));
    GateRef valuePosZero = Int64ToTaggedPtr(Int64(JSTaggedValue::VALUE_POSITIVE_ZERO));
    GateRef valueNegZero = Int64ToTaggedPtr(Int64(JSTaggedValue::VALUE_NEGATIVE_ZERO));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH_NO_WEIGHT(Int64LessThan(*from, thisLen), &next, &loopExit);
        Bind(&next);
        {
            GateRef value = GetValueFromTaggedArray(elements, *from);
            GateRef valueIsZero = LogicOrBuilder(env)
                                        .Or(Equal(value, valueZero))
                                        .Or(Equal(value, valuePosZero))
                                        .Or(Equal(value, valueNegZero))
                                        .Done();
            BRANCH(valueIsZero, &isFound, &loopEnd);
            Bind(&isFound);
            result->WriteVariable(IntToTaggedPtr(*from));
            Jump(exit);
        }
    }
    Bind(&loopEnd);
    from = Int64Add(*from, Int64(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    result->WriteVariable(IntToTaggedPtr(Int32(-1)));
    Jump(exit);
}
} // namespace panda::ecmascript::kungfu
