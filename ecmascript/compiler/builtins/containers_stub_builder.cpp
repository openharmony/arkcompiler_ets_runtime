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

#include "ecmascript/compiler/builtins/containers_stub_builder.h"

#include "ecmascript/compiler/call_stub_builder.h"

namespace panda::ecmascript::kungfu {
// common IR for containers apis that use function call
void ContainersStubBuilder::ContainersCommonFuncCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::INT64(), Int64(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    Label valueIsJSAPIVector(env);
    Label valueNotJSAPIVector(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPIVector(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    BRANCH(IsContainer(*thisObj, type), &valueIsJSAPIVector, &valueNotJSAPIVector);
    Bind(&valueNotJSAPIVector);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsContainer(tempObj, type), &objIsJSAPIVector, slowPath);
            Bind(&objIsJSAPIVector);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPIVector);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPIVector);
    {
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            Label isHeapObj(env);
            callbackFnHandle = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(callbackFnHandle), &isHeapObj, slowPath);
            Bind(&isHeapObj);
            BRANCH(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1(numArgs);
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        length = ContainerGetSize(*thisObj, type);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label lenChange(env);
            Label hasException(env);
            Label notHasException(env);
            Label setValue(env);
            BRANCH(Int32LessThan(*k, *length), &next, &afterLoop);
            Bind(&next);
            {
                kValue = ContainerGetValue(*thisObj, *k, type);
                if (IsPlainArray(type)) {
                    key = PlainArrayGetKey(*thisObj, *k);
                } else {
                    key = IntToTaggedInt(*k);
                }
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { *thisArg, *kValue, *key, *thisObj };
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
                GateRef tempLen = ContainerGetSize(*thisObj, type);
                BRANCH(Int32NotEqual(tempLen, *length), &lenChange, &setValue);
                Bind(&lenChange);
                if (!IsArrayListReplaceAllelements(type)) {
                    length = tempLen;
                }
                BRANCH(Int32GreaterThanOrEqual(*k, *length), &afterLoop, &setValue);
                Bind(&setValue);
                if (IsReplaceAllElements(type)) {
                    ContainerSet(glue, *thisObj, *k, retValue, type);
                }
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        k = Int32Add(*k, Int32(1));
        LoopEnd(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}

void ContainersStubBuilder::QueueCommonFuncCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::INT64(), Int64(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label valueIsJSAPIVector(env);
    Label valueNotJSAPIVector(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPIVector(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    BRANCH(IsContainer(*thisObj, type), &valueIsJSAPIVector, &valueNotJSAPIVector);
    Bind(&valueNotJSAPIVector);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsContainer(tempObj, type), &objIsJSAPIVector, slowPath);
            Bind(&objIsJSAPIVector);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPIVector);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPIVector);
    {
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            Label isHeapObj(env);
            callbackFnHandle = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(callbackFnHandle), &isHeapObj, slowPath);
            Bind(&isHeapObj);
            BRANCH(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1(numArgs);
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        length = ContainerGetSize(*thisObj, type);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label lenChange(env);
            Label hasException(env);
            Label notHasException(env);
            Label setValue(env);
            BRANCH(Int32LessThan(*k, *length), &next, &afterLoop);
            Bind(&next);
            {
                kValue = ContainerGetValue(*thisObj, *index, type);
                index = QueueGetNextPosition(*thisObj, *index);
                key = IntToTaggedInt(*k);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { *thisArg, *kValue, *key, *thisObj };
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
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        k = Int32Add(*k, Int32(1));
        LoopEnd(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}

void ContainersStubBuilder::DequeCommonFuncCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::INT64(), Int64(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(first, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label valueIsJSAPIVector(env);
    Label valueNotJSAPIVector(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPIVector(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    BRANCH(IsContainer(*thisObj, type), &valueIsJSAPIVector, &valueNotJSAPIVector);
    Bind(&valueNotJSAPIVector);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsContainer(tempObj, type), &objIsJSAPIVector, slowPath);
            Bind(&objIsJSAPIVector);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPIVector);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPIVector);
    {
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            Label isHeapObj(env);
            callbackFnHandle = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(callbackFnHandle), &isHeapObj, slowPath);
            Bind(&isHeapObj);
            BRANCH(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1(numArgs);
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        ContainersDequeStubBuilder dequeBuilder(this);
        first = dequeBuilder.GetFirst(*thisObj);
        GateRef last = dequeBuilder.GetLast(*thisObj);
        GateRef capacity = dequeBuilder.GetElementsLength(*thisObj);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label lenChange(env);
            Label hasException(env);
            Label notHasException(env);
            Label setValue(env);
            BRANCH(Int32NotEqual(*first, last), &next, &afterLoop);
            Bind(&next);
            {
                kValue = ContainerGetValue(*thisObj, *index, type);
                key = IntToTaggedInt(*index);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { *thisArg, *kValue, *key, *thisObj };
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
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        first = Int32Mod(Int32Add(*first, Int32(1)), capacity);
        index = Int32Add(*index, Int32(1));
        LoopEnd(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}

void ContainersStubBuilder::ContainersLightWeightCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label valueIsJSAPILightWeight(env);
    Label valueNotJSAPILightWeight(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPILightWeight(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    BRANCH(IsContainer(*thisObj, type), &valueIsJSAPILightWeight, &valueNotJSAPILightWeight);
    Bind(&valueNotJSAPILightWeight);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsContainer(tempObj, type), &objIsJSAPILightWeight, slowPath);
            Bind(&objIsJSAPILightWeight);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPILightWeight);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPILightWeight);
    {
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            Label isHeapObj(env);
            callbackFnHandle = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(callbackFnHandle), &isHeapObj, slowPath);
            Bind(&isHeapObj);
            BRANCH(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1(numArgs);
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        length = ContainerGetSize(*thisObj, type);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label lenChange(env);
            Label hasException(env);
            Label notHasException(env);
            BRANCH(Int32LessThan(*index, *length), &next, &afterLoop);
            Bind(&next);
            {
                value = ContainerGetValue(*thisObj, *index, type);
                key = ContainerGetKey(*thisObj, *index, type);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { *thisArg, *value, *key, *thisObj };
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
                GateRef currentLen = ContainerGetSize(*thisObj, type);
                BRANCH(Int32NotEqual(currentLen, *length), &lenChange, &loopEnd);
                Bind(&lenChange);
                length = currentLen;
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        index = Int32Add(*index, Int32(1));
        LoopEnd(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}

void ContainersStubBuilder::ContainersHashCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(node, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label valueIsJSAPIHash(env);
    Label valueNotJSAPIHash(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPIHash(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label nodeNotHole(env);
    Label nodeIsLinked(env);
    Label nodeIsRBTree(env);
    Label loopLinked(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    BRANCH(IsContainer(*thisObj, type), &valueIsJSAPIHash, &valueNotJSAPIHash);
    Bind(&valueNotJSAPIHash);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsContainer(tempObj, type), &objIsJSAPIHash, slowPath);
            Bind(&objIsJSAPIHash);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPIHash);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPIHash);
    {
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            Label isHeapObj(env);
            callbackFnHandle = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(callbackFnHandle), &isHeapObj, &notCall);
            Bind(&isHeapObj);
            BRANCH(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1(numArgs);
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        length = ContainerGetSize(*thisObj, type);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasExceptionLinked(env);
            Label notHasExceptionLinked(env);
            Label hasExceptionRBTree(env);
            Label notHasExceptionRBTree(env);
            BRANCH(Int32LessThan(*index, *length), &next, &afterLoop);
            Bind(&next);
            {
                node = ContainerGetNode(*thisObj, *index, type);
                BRANCH(TaggedIsHole(*node), &loopEnd, &nodeNotHole);
                Bind(&nodeNotHole);
                BRANCH(IsLinkedNode(*node), &nodeIsLinked, &nodeIsRBTree);
                LoopBegin(&nodeIsLinked);
                {
                    value = Load(VariableType::JS_POINTER(), *node, IntPtr(
                        type == ContainersType::HASHSET_FOREACH ? LinkedNode::KEY_OFFSET : LinkedNode::VALUE_OFFSET));
                    key = Load(VariableType::JS_POINTER(), *node, IntPtr(LinkedNode::KEY_OFFSET));
                    JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                    callArgs.callThisArg3WithReturnArgs = { *thisArg, *value, *key, *thisObj };
                    CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
                        nullptr, Circuit::NullGate(), callArgs);
                    GateRef retValue = callBuilder.JSCallDispatch();
                    BRANCH(HasPendingException(glue), &hasExceptionLinked, &notHasExceptionLinked);
                    Bind(&hasExceptionLinked);
                    {
                        result->WriteVariable(retValue);
                        Jump(exit);
                    }
                    Bind(&notHasExceptionLinked);
                    node = Load(VariableType::JS_POINTER(), *node, IntPtr(LinkedNode::NEXT_OFFSET));
                    BRANCH(TaggedIsHole(*node), &loopEnd, &loopLinked);
                }
                Bind(&loopLinked);
                LoopEnd(&nodeIsLinked);
                Bind(&nodeIsRBTree);
                {
                    GateRef retValue = CallRuntime(glue, RTSTUB_ID(ContainerRBTreeForEach),
                                                   { *node, callbackFnHandle, *thisArg, *thisObj,
                                                     IntToTaggedInt(Int32(static_cast<int32_t>(type))) });
                    BRANCH(HasPendingException(glue), &hasExceptionRBTree, &notHasExceptionRBTree);
                    Bind(&hasExceptionRBTree);
                    {
                        result->WriteVariable(retValue);
                        Jump(exit);
                    }
                    Bind(&notHasExceptionRBTree);
                    Jump(&loopEnd);
                }
            }
        }
        Bind(&loopEnd);
        index = Int32Add(*index, Int32(1));
        LoopEnd(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}

void ContainersStubBuilder::ContainersLinkedListCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* result, Label *exit, Label *slowPath, ContainersType type)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(valueNode, VariableType::INT32(), Int32(0));
    DEFVARIABLE(key, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label valueIsJSAPILinkedList(env);
    Label valueNotJSAPILinkedList(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPILinkedList(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label valueNotHole(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    GateRef callbackFnHandle;
    BRANCH(IsContainer(*thisObj, type), &valueIsJSAPILinkedList, &valueNotJSAPILinkedList);
    Bind(&valueNotJSAPILinkedList);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsContainer(tempObj, type), &objIsJSAPILinkedList, slowPath);
            Bind(&objIsJSAPILinkedList);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPILinkedList);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPILinkedList);
    {
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &callbackUndefined, &callbackNotUndefined);
        Bind(&callbackUndefined);
        Jump(slowPath);
        Bind(&callbackNotUndefined);
        {
            Label isCall(env);
            Label notCall(env);
            Label isHeapObj(env);
            callbackFnHandle = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(callbackFnHandle), &isHeapObj, &notCall);
            Bind(&isHeapObj);
            BRANCH(IsCallable(callbackFnHandle), &isCall, &notCall);
            Bind(&notCall);
            Jump(slowPath);
            Bind(&isCall);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &thisArgUndefined, &thisArgNotUndefined);
                Bind(&thisArgUndefined);
                Jump(&nextCount);
                Bind(&thisArgNotUndefined);
                thisArg = GetCallArg1(numArgs);
                Jump(&nextCount);
            }
        }
    }
    Bind(&nextCount);
    {
        length = ContainerGetSize(*thisObj, type);
        valueNode = Int32(TaggedList<TaggedArray>::ELEMENTS_START_INDEX);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasException(env);
            Label notHasException(env);
            BRANCH(Int32LessThan(*index, *length), &next, &afterLoop);
            Bind(&next);
            {
                valueNode = TaggedGetInt(ContainerGetNode(*thisObj,
                    Int32Add(*valueNode, Int32(TaggedList<TaggedArray>::NEXT_PTR_OFFSET)), type));
                value = ContainerGetNode(*thisObj, *valueNode, type);
                BRANCH(TaggedIsHole(*value), &loopEnd, &valueNotHole);
                Bind(&valueNotHole);
                key = IntToTaggedInt(*index);
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { *thisArg, *value, *key, *thisObj };
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
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        index = Int32Add(*index, Int32(1));
        LoopEnd(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}
}  // namespace panda::ecmascript::kungfu
