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

    GateRef target = GetArgFromArgv(glue_, IntPtr(0), numArgs_, true);
    GateRef handler = GetArgFromArgv(glue_, IntPtr(1), numArgs_, true);

    BRANCH(IsEcmaObject(glue_, target), &targetIsEcma, &slowPath);
    Bind(&targetIsEcma);
    BRANCH(IsEcmaObject(glue_, handler), &handleIsEcma, &slowPath);
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

void BuiltinsProxyStubBuilder::CheckGetTrapResult(GateRef target, GateRef key, Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label callRuntime(env);
    Label isFoundData(env);
    Label isFoundAccessor(env);
    DEFVARIABLE(value, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(attr, VariableType::INT64(), Int64(0));
    TryGetOwnProperty(glue_, target, key, Circuit::NullGate(), &value, &attr,
                      &isFoundData, &isFoundAccessor, exit, &callRuntime);
    Bind(&isFoundData);
    {
        Label trapResultTypeError(env);
        GateRef rAttr = attr.ReadVariable();
        GateRef rValue = value.ReadVariable();
        GateRef rResult = result->ReadVariable();
        GateRef trapResultCheck = LogicOrBuilder(env).Or(IsConfigable(rAttr))
                                                     .Or(IsWritable(rAttr))
                                                     .Or(SameValue(glue_, rResult, rValue))
                                                     .Done();
        BRANCH(BoolNot(trapResultCheck), &trapResultTypeError, exit);
        Bind(&trapResultTypeError);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxyGetPropertyResultTypeError));
            CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
            result->WriteVariable(Exception());
            Jump(exit);
        }
    }
    Bind(&isFoundAccessor);
    {
        Label trapResultIsUndefined(env);
        Label trapResultIsNotUndefined(env);
        GateRef rAttr = attr.ReadVariable();
        GateRef rValue = value.ReadVariable();
        GateRef rResult = result->ReadVariable();
        GateRef getter = Load(VariableType::JS_ANY(), glue_, rValue, IntPtr(AccessorData::GETTER_OFFSET));
        GateRef trapResultCheck = LogicAndBuilder(env).And(BoolNot(IsConfigable(rAttr)))
                                                      .And(TaggedIsUndefined(getter))
                                                      .And(BoolNot(TaggedIsUndefined(rResult)))
                                                      .Done();
        BRANCH(trapResultCheck, &trapResultIsNotUndefined, exit);
        Bind(&trapResultIsNotUndefined);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxyGetPropertyResultNotUndefined));
            CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
            result->WriteVariable(Exception());
            Jump(exit);
        }
    }
    Bind(&callRuntime);
    {
        result->WriteVariable(
            CallRuntime(glue_, RTSTUB_ID(CheckGetTrapResult), { target, key, result->ReadVariable() }));
        Jump(exit);
    }
}

void BuiltinsProxyStubBuilder::CheckSetTrapResult(GateRef target, GateRef key, GateRef value,
                                                  Variable *result, Label *exit)
{
    auto env = GetEnvironment();
    Label callRuntime(env);
    Label isFoundData(env);
    Label isFoundAccessor(env);
    DEFVARIABLE(tValue, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(attr, VariableType::INT64(), Int64(0));
    TryGetOwnProperty(glue_, target, key, Circuit::NullGate(), &tValue, &attr,
                      &isFoundData, &isFoundAccessor, exit, &callRuntime);
    Bind(&isFoundData);
    {
        Label trapResultTypeError(env);
        GateRef rAttr = attr.ReadVariable();
        GateRef rValue = tValue.ReadVariable();
        GateRef trapResultCheck = LogicOrBuilder(env).Or(IsConfigable(rAttr))
                                                     .Or(IsWritable(rAttr))
                                                     .Or(SameValue(glue_, value, rValue))
                                                     .Done();
        BRANCH(BoolNot(trapResultCheck), &trapResultTypeError, exit);
        Bind(&trapResultTypeError);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxySetPropertyResultTypeError));
            CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
            result->WriteVariable(TaggedFalse());
            Jump(exit);
        }
    }
    Bind(&isFoundAccessor);
    {
        Label trapResultIsUndefined(env);
        Label trapResultIsNotUndefined(env);
        GateRef rAttr = attr.ReadVariable();
        GateRef rValue = tValue.ReadVariable();
        GateRef setter = Load(VariableType::JS_ANY(), glue_, rValue, IntPtr(AccessorData::SETTER_OFFSET));
        GateRef trapResultCheck = LogicAndBuilder(env).And(BoolNot(IsConfigable(rAttr)))
                                                      .And(TaggedIsUndefined(setter))
                                                      .Done();
        BRANCH(trapResultCheck, &trapResultIsNotUndefined, exit);
        Bind(&trapResultIsNotUndefined);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxySetPropertyResultNotAccessor));
            CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
            result->WriteVariable(TaggedFalse());
            Jump(exit);
        }
    }
    Bind(&callRuntime);
    {
        result->WriteVariable(CallRuntime(glue_, RTSTUB_ID(CheckSetTrapResult), { target, key, value }));
        Jump(exit);
    }
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
    GateRef handler = GetHandler(glue_, proxy);
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
        GateRef target = GetTarget(glue_, proxy);
        GateRef name = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
            ConstantIndex::GET_STRING_INDEX);
        GateRef trap = GetPropertyByName(glue_, handler, name);
        BRANCH(TaggedIsUndefinedOrNullOrHole(trap), &slowPath, &trapIsNotUndefinedOrNullOrHole);
        Bind(&trapIsNotUndefinedOrNullOrHole);
        {
            BRANCH(IsCallable(glue_, trap), &trapIsCallable, &slowPath);
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
            CheckGetTrapResult(target, key, &result, &exit);
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

GateRef BuiltinsProxyStubBuilder::SetProperty(GateRef proxy, GateRef key, GateRef value, GateRef receiver,
                                              bool mayThrow)
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
    GateRef handler = GetHandler(glue_, proxy);
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
        GateRef target = GetTarget(glue_, proxy);
        GateRef name = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
            ConstantIndex::SET_STRING_INDEX);
        GateRef trap = GetPropertyByName(glue_, handler, name);
        BRANCH(TaggedIsUndefinedOrNullOrHole(trap), &slowPath, &trapIsNotUndefinedOrNullOrHole);
        Bind(&trapIsNotUndefinedOrNullOrHole);
        {
            BRANCH(IsCallable(glue_, trap), &trapIsCallable, &slowPath);
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
                SetValueToTaggedArray(VariableType::JS_ANY(), glue_, argList, Int32(2), value);
                // param 3: receiver
                SetValueToTaggedArray(VariableType::JS_ANY(), glue_, argList, Int32(3), receiver);
                GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                callArgs.callThisArgvWithReturnArgs = { argsLength, argv, handler };
                CallStubBuilder callBuilder(this, glue_, trap, argsLength, 0, nullptr,
                    Circuit::NullGate(), callArgs);
                GateRef trapResult = callBuilder.JSCallDispatch();
                BRANCH(TaggedIsFalse(FastToBoolean(glue_, trapResult)), &trapResultIsFalse, &checkSetTrapResult);
                Bind(&trapResultIsFalse);
                {
                    if (mayThrow) {
                        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(ProxySetPropertyReturnFalse));
                        CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
                    }
                    result = TaggedFalse();
                    Jump(&exit);
                }
            }
        }
        Bind(&checkSetTrapResult);
        {
            CheckSetTrapResult(target, key, value, &result, &exit);
        }
        Bind(&slowPath);
        {
            result = CallRuntime(glue_, RTSTUB_ID(JSProxySetProperty),
                                 {proxy, key, value, receiver, mayThrow ? TaggedTrue() : TaggedFalse()});
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    return ret;
}
}  // namespace panda::ecmascript::kungfu
