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

#include "ecmascript/compiler/builtins/builtins_proxy_stub_builder.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {
void BuiltinsProxyStubBuilder::GenProxyConstructor(GateRef nativeCode, GateRef func, GateRef newTarget)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label thisCollectionObj(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    Label handleIsEcma(env);
    Label targetIsEcma(env);
    Label newTargetNotUndefined(env);
    BRANCH(TaggedIsUndefined(newTarget), &slowPath, &newTargetNotUndefined);
    Bind(&newTargetNotUndefined);

    GateRef target = GetArgFromArgv(IntPtr(0), numArgs_, true);
    GateRef handler = GetArgFromArgv(IntPtr(1), numArgs_, true);

    BRANCH(IsEcmaObject(target), &targetIsEcma, &slowPath);
    Bind(&targetIsEcma);
    BRANCH(IsEcmaObject(handler), &handleIsEcma, &slowPath);
    Bind(&handleIsEcma);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue_, IntPtr(0));
        res = newBuilder.NewJSProxy(glue_, target, handler);
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue_, { glue_, nativeCode, func, thisValue_, numArgs_, argv }, true);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

GateRef BuiltinsProxyStubBuilder::GetProperty(GateRef proxy, GateRef key, GateRef receiver)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label callExit(env);
    Label checkGetTrapResult(env);
    Label exit(env);
    Label handlerIsNull(env);
    Label handlerIsNotNull(env);
    Label slowPath(env);
    Label trapIsCallable(env);
    Label trapIsNotUndefinedOrNullOrHole(env);
    GateRef handler = GetHandler(proxy);
    BRANCH(TaggedIsNull(handler), &handlerIsNull, &handlerIsNotNull);
    Bind(&handlerIsNull);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxyGetPropertyHandlerIsNull));
        CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result = Exception();
        Jump(&exit);
    }
    Bind(&handlerIsNotNull);
    {
        GateRef target = GetTarget(proxy);
        GateRef name = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
            ConstantIndex::GET_STRING_INDEX);
        GateRef trap = GetPropertyByName(glue_, handler, name, ProfileOperation(), True());
        BRANCH(TaggedIsUndefinedOrNullOrHole(trap), &slowPath, &trapIsNotUndefinedOrNullOrHole);
        Bind(&trapIsNotUndefinedOrNullOrHole);
        {
            BRANCH(IsCallable(trap), &trapIsCallable, &slowPath);
            Bind(&trapIsCallable);
            {
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
                callArgs.callThisArg3WithReturnArgs = { handler, target, key, receiver };
                CallStubBuilder callBuilder(this, glue_, trap,
                    Int32(3), 0, nullptr, Circuit::NullGate(), callArgs); // 3 : three arg
                result = callBuilder.JSCallDispatch();
                Jump(&checkGetTrapResult);
            }
        }
        Bind(&checkGetTrapResult);
        {
            result = CallRuntime(glue_, RTSTUB_ID(CheckGetTrapResult), { target, key, *result });
            Jump(&exit);
        }
        Bind(&slowPath);
        {
            result = CallRuntime(glue_, RTSTUB_ID(JSProxyGetProperty), { proxy, key, receiver });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    return ret;
}

GateRef BuiltinsProxyStubBuilder::SetProperty(GateRef proxy, GateRef key, GateRef value, GateRef receiver)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedTrue());
    Label trapResultIsFalse(env);
    Label callExit(env);
    Label checkSetTrapResult(env);
    Label exit(env);
    Label handlerIsNull(env);
    Label handlerIsNotNull(env);
    Label slowPath(env);
    Label trapIsCallable(env);
    Label trapIsNotUndefinedOrNullOrHole(env);
    GateRef handler = GetHandler(proxy);
    BRANCH(TaggedIsNull(handler), &handlerIsNull, &handlerIsNotNull);
    Bind(&handlerIsNull);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxySetPropertyHandlerIsNull));
        CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&handlerIsNotNull);
    {
        GateRef target = GetTarget(proxy);
        GateRef name = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
            ConstantIndex::SET_STRING_INDEX);
        GateRef trap = GetPropertyByName(glue_, handler, name, ProfileOperation(), True());
        BRANCH(TaggedIsUndefinedOrNullOrHole(trap), &slowPath, &trapIsNotUndefinedOrNullOrHole);
        Bind(&trapIsNotUndefinedOrNullOrHole);
        {
            BRANCH(IsCallable(trap), &trapIsCallable, &slowPath);
            Bind(&trapIsCallable);
            {
                GateRef argsLength = Int32(4);
                NewObjectStubBuilder newBuilder(this);
                GateRef argList = newBuilder.NewTaggedArray(glue_, argsLength);
                // param 0: target
                SetValueToTaggedArray(VariableType::JS_ANY(), glue_, argList, Int32(0), target);
                // param 1: key
                SetValueToTaggedArray(VariableType::JS_ANY(), glue_, argList, Int32(1), key);
                // param 2: value
                SetValueToTaggedArray(VariableType::INT32(), glue_, argList, Int32(2), value);
                // param 3: receiver
                SetValueToTaggedArray(VariableType::JS_ANY(), glue_, argList, Int32(3), receiver);
                GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                callArgs.callThisArgvWithReturnArgs = { argsLength, argv, handler };
                CallStubBuilder callBuilder(this, glue_, trap, argsLength, 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef trapResult = callBuilder.JSCallDispatch();
                BRANCH(TaggedIsFalse(FastToBoolean(trapResult)), &trapResultIsFalse, &checkSetTrapResult);
                Bind(&trapResultIsFalse);
                {
                    GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxySetPropertyReturnFalse));
                    CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
                    result = TaggedFalse();
                    Jump(&exit);
                }
            }
        }
        Bind(&checkSetTrapResult);
        {
            result = CallRuntime(glue_, RTSTUB_ID(CheckSetTrapResult), { target, key, value });
            Jump(&exit);
        }
        Bind(&slowPath);
        {
            result = CallRuntime(glue_, RTSTUB_ID(JSProxySetProperty), { proxy, key, value, receiver });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    return ret;
}
}  // namespace panda::ecmascript::kungfu
