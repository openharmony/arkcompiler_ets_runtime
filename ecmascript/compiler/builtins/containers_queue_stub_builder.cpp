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

#include "ecmascript/compiler/builtins/containers_queue_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
void ContainersQueueStubBuilder::ForEach(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result,
                                         Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(thisObj, VariableType::JS_ANY(), thisValue);
    DEFVARIABLE(thisArg, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::INT64(), Int64(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(length, VariableType::INT32(), Int32(0));
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
    Label valueIsJSAPIQueue(env);
    Label valueNotJSAPIQueue(env);
    Label objIsJSProxy(env);
    Label objNotJSProxy(env);
    Label objIsJSAPIQueue(env);
    Label thisArgUndefined(env);
    Label thisArgNotUndefined(env);
    Label callbackUndefined(env);
    Label callbackNotUndefined(env);
    Label nextCount(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label afterLoop(env);
    Label thisValueIsHeapObj(env);
    GateRef callbackFnHandle;
    BRANCH(TaggedIsHeapObject(thisValue), &thisValueIsHeapObj, slowPath);
    Bind(&thisValueIsHeapObj);
    BRANCH(IsJSAPIQueue(*thisObj), &valueIsJSAPIQueue, &valueNotJSAPIQueue);
    Bind(&valueNotJSAPIQueue);
    {
        BRANCH(IsJsProxy(*thisObj), &objIsJSProxy, &objNotJSProxy);
        Bind(&objIsJSProxy);
        {
            GateRef tempObj = GetTarget(*thisObj);
            BRANCH(IsJSAPIQueue(tempObj), &objIsJSAPIQueue, slowPath);
            Bind(&objIsJSAPIQueue);
            {
                thisObj = tempObj;
                Jump(&valueIsJSAPIQueue);
            }
        }
        Bind(&objNotJSProxy);
        Jump(slowPath);
    }
    Bind(&valueIsJSAPIQueue);
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
        length = GetArrayLength(*thisObj);
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
                kValue = Get(*thisObj, *index);
                index = GetNextPosition(*thisObj, *index);
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
        LoopEndWithCheckSafePoint(&loopHead, env, glue);
    }
    Bind(&afterLoop);
    Jump(exit);
}
}
