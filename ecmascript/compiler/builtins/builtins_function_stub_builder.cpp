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

#include "ecmascript/compiler/builtins/builtins_function_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_object_stub_builder.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_arguments.h"

namespace panda::ecmascript::kungfu {

void BuiltinsFunctionStubBuilder::PrototypeApply(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label targetIsCallable(env);
    Label targetIsUndefined(env);
    Label targetNotUndefined(env);
    Label isHeapObject(env);
    //1. If IsCallable(func) is false, throw a TypeError exception
    BRANCH(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    {
        BRANCH(IsCallable(glue, thisValue), &targetIsCallable, slowPath);
        Bind(&targetIsCallable);
        {
            GateRef thisArg = GetCallArg0(numArgs);
            GateRef arrayObj = GetCallArg1(numArgs);
            // 2. If argArray is null or undefined, then
            BRANCH(TaggedIsUndefined(arrayObj), &targetIsUndefined, &targetNotUndefined);
            Bind(&targetIsUndefined);
            {
                // a. Return Call(func, thisArg).
                JSCallArgs callArgs(JSCallMode::CALL_GETTER);
                callArgs.callGetterArgs = { thisArg };
                CallStubBuilder callBuilder(this, glue, thisValue, Int32(0), 0, nullptr, Circuit::NullGate(), callArgs);
                res->WriteVariable(callBuilder.JSCallDispatch());
                Jump(exit);
            }
            Bind(&targetNotUndefined);
            {
                // 3. Let argList be CreateListFromArrayLike(argArray).
                GateRef elements = BuildArgumentsListFastElements(glue, arrayObj);
                Label targetIsHole(env);
                Label targetNotHole(env);
                BRANCH(TaggedIsHole(elements), &targetIsHole, &targetNotHole);
                Bind(&targetIsHole);
                {
                    NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
                    GateRef argList = newBuilder.CreateListFromArrayLike(glue, arrayObj);
                    // 4. ReturnIfAbrupt(argList).
                    Label isPendingException(env);
                    Label noPendingException(env);
                    BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
                    Bind(&isPendingException);
                    {
                        Jump(slowPath);
                    }
                    Bind(&noPendingException);
                    {
                        GateRef argsLength = GetLengthOfTaggedArray(argList);
                        GateRef argv = PtrAdd(argList, IntPtr(TaggedArray::DATA_OFFSET));
                        JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                        callArgs.callThisArgvWithReturnArgs = { argsLength, argv, thisArg };
                        CallStubBuilder callBuilder(this, glue, thisValue, argsLength, 0, nullptr, Circuit::NullGate(),
                            callArgs);
                        res->WriteVariable(callBuilder.JSCallDispatch());
                        Jump(exit);
                    }
                }
                Bind(&targetNotHole);
                {
                    // 6. Return Call(func, thisArg, argList).
                    Label taggedIsStableJsArg(env);
                    Label taggedNotStableJsArg(env);
                    BRANCH(IsStableJSArguments(glue, arrayObj), &taggedIsStableJsArg, &taggedNotStableJsArg);
                    Bind(&taggedIsStableJsArg);
                    {
                        GateRef hClass = LoadHClass(glue, arrayObj);
                        GateRef PropertyInlinedPropsOffset = IntPtr(JSArguments::LENGTH_INLINE_PROPERTY_INDEX);
                        GateRef result = GetPropertyInlinedProps(glue, arrayObj, hClass, PropertyInlinedPropsOffset);
                        GateRef length = TaggedGetInt(result);
                        GateRef argsLength = MakeArgListWithHole(glue, elements, length);
                        GateRef elementArgv = PtrAdd(elements, IntPtr(TaggedArray::DATA_OFFSET));
                        JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                        callArgs.callThisArgvWithReturnArgs = { argsLength, elementArgv, thisArg };
                        CallStubBuilder callBuilder(this, glue, thisValue, argsLength, 0, nullptr, Circuit::NullGate(),
                            callArgs);
                        res->WriteVariable(callBuilder.JSCallDispatch());
                        Jump(exit);
                    }
                    Bind(&taggedNotStableJsArg);
                    {
                        GateRef length = GetArrayLength(arrayObj);
                        GateRef argsLength = MakeArgListWithHole(glue, elements, length);
                        GateRef elementArgv = PtrAdd(elements, IntPtr(TaggedArray::DATA_OFFSET));
                        JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
                        callArgs.callThisArgvWithReturnArgs = { argsLength, elementArgv, thisArg };
                        CallStubBuilder callBuilder(this, glue, thisValue, argsLength, 0, nullptr, Circuit::NullGate(),
                            callArgs);
                        res->WriteVariable(callBuilder.JSCallDispatch());
                        Jump(exit);
                    }
                }
            }
        }
    }
}

void BuiltinsFunctionStubBuilder::PrototypeBind(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label targetIsHeapObject(env);
    Label targetIsCallable(env);
    Label targetIsJSFunctionOrBound(env);
    Label findInlineProperty(env);
    Label targetNameAndLengthNotChange(env);

    // 1. Let Target be the this value.
    GateRef target = thisValue;
    // 2. If IsCallable(Target) is false, throw a TypeError exception.
    BRANCH(TaggedIsHeapObject(target), &targetIsHeapObject, slowPath);
    Bind(&targetIsHeapObject);
    BRANCH(IsCallable(glue, target), &targetIsCallable, slowPath);
    Bind(&targetIsCallable);
    BRANCH(IsJSOrBoundFunction(glue, target), &targetIsJSFunctionOrBound, slowPath);
    Bind(&targetIsJSFunctionOrBound);
    {
        GateRef hclass = LoadHClass(glue, target);
        GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
        GateRef isPropsInRange = LogicAndBuilder(env)
            .And(Int32UnsignedLessThan(Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), inlineProps))
            .And(Int32UnsignedLessThan(Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX), inlineProps))
            .Done();
        BRANCH(isPropsInRange, &findInlineProperty, slowPath);
        Bind(&findInlineProperty);
        GateRef lengthProperty = GetPropertyInlinedProps(glue, target, hclass,
            Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX));
        GateRef nameProperty = GetPropertyInlinedProps(glue, target, hclass,
            Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX));
        GateRef functionLengthAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
            ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
        GateRef functionNameAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
            ConstantIndex::FUNCTION_NAME_ACCESSOR);
        GateRef isTargetNameAndLengthNotChange = LogicAndBuilder(env)
            .And(IntPtrEqual(functionLengthAccessor, lengthProperty))
            .And(IntPtrEqual(functionNameAccessor, nameProperty))
            .Done();
        BRANCH(isTargetNameAndLengthNotChange, &targetNameAndLengthNotChange, slowPath);
        Bind(&targetNameAndLengthNotChange);
        {
            Label numArgsMoreThan1(env);
            Label createTaggedArray(env);
            GateRef thisArg = GetCallArg0(numArgs);
            DEFVARIABLE(argsLength, VariableType::INT32(), Int32(0));
            BRANCH(Int64GreaterThan(numArgs, Int64(1)), &numArgsMoreThan1, &createTaggedArray);
            Bind(&numArgsMoreThan1);
            {
                argsLength = Int32Sub(TruncInt64ToInt32(numArgs), Int32(1));
                Jump(&createTaggedArray);
            }
            Bind(&createTaggedArray);
            // 3. Let args be a new (possibly empty) List consisting of all of the argument
            //    values provided after thisArg in order.
            GateRef argsArray = NewTaggedArrayFromArgs(glue, Int32(1), *argsLength, numArgs);
            // 4. Let F be BoundFunctionCreate(Target, thisArg, args).
            NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
            GateRef boundFunction = newBuilder.NewJSBoundFunction(glue, target, thisArg, argsArray);
            // use default name and length property because they are not changed
            res->WriteVariable(boundFunction);
            Jump(exit);
        }
    }
}

void BuiltinsFunctionStubBuilder::PrototypeCall(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label funcIsHeapObject(env);
    Label funcIsCallable(env);

    // 1. If IsCallable(func) is false, throw a TypeError exception.
    GateRef func = thisValue;
    BRANCH(TaggedIsHeapObject(func), &funcIsHeapObject, slowPath);
    Bind(&funcIsHeapObject);
    BRANCH(IsCallable(glue, func), &funcIsCallable, slowPath);
    Bind(&funcIsCallable);
    {
        Label call0(env);
        Label moreThan0(env);
        Label call1(env);
        Label moreThan1(env);
        Label call2(env);
        Label moreThan2(env);
        Label createTaggedArray(env);
        GateRef thisArg = GetCallArg0(numArgs);
        // 2. Let argList be an empty List.
        // 3. If this method was called with more than one argument then in left to right order,
        //    starting with the second argument, append each argument as the last element of argList.
        // 5. Return Call(func, thisArg, argList).
        BRANCH(Int64LessThanOrEqual(numArgs, Int64(1)), &call0, &moreThan0);  // 1: thisArg
        Bind(&call0);
        {
            JSCallArgs callArgs(JSCallMode::CALL_GETTER);
            callArgs.callGetterArgs = { thisArg };
            CallStubBuilder callBuilder(this, glue, func, Int32(0), 0, nullptr, Circuit::NullGate(), callArgs);
            res->WriteVariable(callBuilder.JSCallDispatch());
            Jump(exit);
        }
        Bind(&moreThan0);
        BRANCH(Int64Equal(numArgs, Int64(2)), &call1, &moreThan1);  // 2: thisArg + 1 arg
        Bind(&call1);
        {
            JSCallArgs callArgs(JSCallMode::CALL_SETTER);
            callArgs.callSetterArgs = { thisArg, GetCallArg1(numArgs) };
            CallStubBuilder callBuilder(this, glue, func, Int32(1), 0, nullptr, Circuit::NullGate(), callArgs);
            res->WriteVariable(callBuilder.JSCallDispatch());
            Jump(exit);
        }
        Bind(&moreThan1);
        BRANCH(Int64Equal(numArgs, Int64(3)), &call2, &moreThan2);  // 3: thisArg + 2 args
        Bind(&call2);
        {
            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG2_WITH_RETURN);
            callArgs.callThisArg2WithReturnArgs = { thisArg, GetCallArg1(numArgs), GetCallArg2(numArgs) };
            CallStubBuilder callBuilder(this, glue, func, Int32(2), 0, nullptr, Circuit::NullGate(),  // 2: call 2
                callArgs);
            res->WriteVariable(callBuilder.JSCallDispatch());
            Jump(exit);
        }
        Bind(&moreThan2);
        {
            // currently argv will not be used in builtins IR except constructor
            GateRef argsLength = Int32Sub(TruncInt64ToInt32(numArgs), Int32(1));  // 1: thisArg
            GateRef elementArgv = PtrAdd(GetArgv(), IntPtr(JSTaggedValue::TaggedTypeSize()));
            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARGV_WITH_RETURN);
            callArgs.callThisArgvWithReturnArgs = { argsLength, elementArgv, thisArg };
            CallStubBuilder callBuilder(this, glue, func, argsLength, 0, nullptr, Circuit::NullGate(), callArgs);
            res->WriteVariable(callBuilder.JSCallDispatch());
            Jump(exit);
        }
    }
}

// return elements
GateRef BuiltinsFunctionStubBuilder::BuildArgumentsListFastElements(GateRef glue, GateRef arrayObj)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(res, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label targetIsStableJSArguments(env);
    Label targetNotStableJSArguments(env);
    Label targetIsInt(env);
    Label hClassEqual(env);
    Label targetIsStableJSArray(env);
    Label targetNotStableJSArray(env);

    BRANCH(IsStableJSArguments(glue, arrayObj), &targetIsStableJSArguments, &targetNotStableJSArguments);
    Bind(&targetIsStableJSArguments);
    {
        GateRef hClass = LoadHClass(glue, arrayObj);
        GateRef globalEnv = GetCurrentGlobalEnv();
        GateRef argmentsClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ARGUMENTS_CLASS);
        BRANCH(Int64Equal(hClass, argmentsClass), &hClassEqual, &exit);
        Bind(&hClassEqual);
        {
            GateRef PropertyInlinedPropsOffset = IntPtr(JSArguments::LENGTH_INLINE_PROPERTY_INDEX);
            GateRef result = GetPropertyInlinedProps(glue, arrayObj, hClass, PropertyInlinedPropsOffset);
            BRANCH(TaggedIsInt(result), &targetIsInt, &exit);
            Bind(&targetIsInt);
            {
                res = GetElementsArray(glue, arrayObj);
                Label isMutantTaggedArray(env);
                BRANCH(IsMutantTaggedArray(glue, *res), &isMutantTaggedArray, &exit);
                Bind(&isMutantTaggedArray);
                {
                    NewObjectStubBuilder newBuilder(this);
                    GateRef elementsLength = GetLengthOfTaggedArray(*res);
                    GateRef newTaggedArgList = newBuilder.NewTaggedArray(glue, elementsLength);
                    DEFVARIABLE(index, VariableType::INT32(), Int32(0));
                    Label loopHead(env);
                    Label loopEnd(env);
                    Label afterLoop(env);
                    Label storeValue(env);
                    Jump(&loopHead);
                    LoopBegin(&loopHead);
                    {
                        BRANCH(Int32UnsignedLessThan(*index, elementsLength), &storeValue, &afterLoop);
                        Bind(&storeValue);
                        {
                            GateRef value = GetTaggedValueWithElementsKind(glue, arrayObj, *index);
                            SetValueToTaggedArray(VariableType::JS_ANY(), glue, newTaggedArgList, *index, value);
                            index = Int32Add(*index, Int32(1));
                            Jump(&loopEnd);
                        }
                    }
                    Bind(&loopEnd);
                    LoopEnd(&loopHead);
                    Bind(&afterLoop);
                    {
                        res = newTaggedArgList;
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&targetNotStableJSArguments);
    {
        BRANCH(IsStableJSArray(glue, arrayObj), &targetIsStableJSArray, &targetNotStableJSArray);
        Bind(&targetIsStableJSArray);
        {
            res = GetElementsArray(glue, arrayObj);
            Label isMutantTaggedArray(env);
            BRANCH(IsMutantTaggedArray(glue, *res), &isMutantTaggedArray, &exit);
            Bind(&isMutantTaggedArray);
            {
                NewObjectStubBuilder newBuilder(this);
                GateRef elementsLength = GetLengthOfTaggedArray(*res);
                GateRef newTaggedArgList = newBuilder.NewTaggedArray(glue, elementsLength);
                DEFVARIABLE(index, VariableType::INT32(), Int32(0));
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Label storeValue(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH(Int32UnsignedLessThan(*index, elementsLength), &storeValue, &afterLoop);
                    Bind(&storeValue);
                    {
                        GateRef value = GetTaggedValueWithElementsKind(glue, arrayObj, *index);
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, newTaggedArgList, *index, value);
                        index = Int32Add(*index, Int32(1));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead);
                Bind(&afterLoop);
                {
                    res = newTaggedArgList;
                    Jump(&exit);
                }
            }
        }
        Bind(&targetNotStableJSArray);
        {
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsFunctionStubBuilder::MakeArgListWithHole(GateRef glue, GateRef argv, GateRef length)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(res, VariableType::INT32(), length);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label exit(env);
    Label greatThanZero(env);
    Label lessThanZero(env);
    BRANCH(Int32GreaterThan(length, Int32(0)), &greatThanZero, &lessThanZero);
    Bind(&lessThanZero);
    {
        res = Int32(0);
        Jump(&exit);
    }
    Bind(&greatThanZero);
    GateRef argsLength = GetLengthOfTaggedArray(argv);
    Label lengthGreaterThanArgsLength(env);
    Label lengthLessThanArgsLength(env);
    BRANCH(Int32GreaterThan(length, argsLength), &lengthGreaterThanArgsLength, &lengthLessThanArgsLength);
    Bind(&lengthGreaterThanArgsLength);
    {
        res = argsLength;
        Jump(&lengthLessThanArgsLength);
    }
    Bind(&lengthLessThanArgsLength);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label targetIsHole(env);
        Label targetNotHole(env);
        BRANCH(Int32UnsignedLessThan(*i, *res), &loopHead, &exit);
        LoopBegin(&loopHead);
        {
            GateRef value = GetValueFromTaggedArray(glue, argv, *i);
            BRANCH(TaggedIsHole(value), &targetIsHole, &targetNotHole);
            Bind(&targetIsHole);
            {
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, argv, *i, Undefined());
                Jump(&targetNotHole);
            }
            Bind(&targetNotHole);
            i = Int32Add(*i, Int32(1));
            BRANCH(Int32UnsignedLessThan(*i, *res), &loopEnd, &exit);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsFunctionStubBuilder::NewTaggedArrayFromArgs(GateRef glue, GateRef startIndex, GateRef length,
                                                            GateRef numArgs)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    NewObjectStubBuilder newBuilder(this);
    res = newBuilder.NewTaggedArray(glue, length);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    BRANCH(Int32LessThan(*i, length), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    {
        Label valueArg0(env);
        Label valueNotArg0(env);
        Label valueArg1(env);
        Label valueNotArg1(env);
        Label valueArg2(env);
        Label valueNotArg2(env);
        Label valueSet(env);
        GateRef index = Int32Add(*i, startIndex);
        BRANCH(Int32Equal(index, Int32(0)), &valueArg0, &valueNotArg0);  // 0: get arg0
        Bind(&valueArg0);
        {
            value = GetCallArg0(numArgs);
            Jump(&valueSet);
        }
        Bind(&valueNotArg0);
        BRANCH(Int32Equal(index, Int32(1)), &valueArg1, &valueNotArg1);  // 1: get arg1
        Bind(&valueArg1);
        {
            value = GetCallArg1(numArgs);
            Jump(&valueSet);
        }
        Bind(&valueNotArg1);
        BRANCH(Int32Equal(index, Int32(2)), &valueArg2, &valueNotArg2);  // 2: get arg2
        Bind(&valueArg2);
        {
            value = GetCallArg2(numArgs);
            Jump(&valueSet);
        }
        Bind(&valueNotArg2);
        {
            // currently argv will not be used in builtins IR except constructor
            value = GetArgFromArgv(glue, ZExtInt32ToPtr(index));
            Jump(&valueSet);
        }
        Bind(&valueSet);
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, *res, *i, *value);
        i = Int32Add(*i, Int32(1));
        BRANCH(Int32LessThan(*i, length), &loopEnd, &afterLoop);
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);

    Bind(&afterLoop);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

void BuiltinsFunctionStubBuilder::InitializeSFunction(GateRef glue, GateRef func, GateRef kind, FunctionKind getKind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label hasAccessOrIsBaseConstructor(env);
    Label isBaseConstructor(env);
    Label isNotBaseConstructor(env);

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    GateRef hclass = LoadHClass(glue, func);

    if (JSFunction::IsNormalFunctionAndCanSkipWbWhenInitialization(getKind)) {
        SetProtoOrHClassToFunction(glue, func, Hole(), MemoryAttribute::NoBarrier());
        SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryAttribute::NoBarrier());
        if (JSFunction::HasAccessor(getKind)) {
            auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                       ConstantIndex::FUNCTION_NAME_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            Jump(&exit);
        }
    } else {
        SetLexicalEnvToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
        SetHomeObjectToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
        SetProtoOrHClassToFunction(glue, func, Hole(), MemoryAttribute::NoBarrier());
        SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryAttribute::NoBarrier());
        SetMethodToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());

        BRANCH(BitOr(HasAccessor(kind), IsBaseConstructorKind(kind)), &hasAccessOrIsBaseConstructor, &exit);
        Bind(&hasAccessOrIsBaseConstructor);
        {
            Branch(IsBaseConstructorKind(kind), &isBaseConstructor, &isNotBaseConstructor);
            Bind(&isBaseConstructor);
            {
                auto funcprotoAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                                ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcprotoAccessor,
                                        Int32(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
                Jump(&isNotBaseConstructor);
            }
            Bind(&isNotBaseConstructor);
            auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                       ConstantIndex::FUNCTION_NAME_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                    Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                    Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto emptyProfileTypeInfoCell = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::EMPTY_PROFILE_TYPE_INFO_CELL_INDEX);
    SetRawProfileTypeInfoToFunction(glue, func, emptyProfileTypeInfoCell, MemoryAttribute::NoBarrier());
    env->SubCfgExit();
    return;
}

void BuiltinsFunctionStubBuilder::InitializeJSFunction(GateRef glue, GateRef func, GateRef kind, FunctionKind getKind)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label hasProto(env);
    Label notProto(env);
    Label hasAccess(env);
    Label isBase(env);
    Label notBase(env);
    Label isGenerator(env);
    Label notClassConstructor(env);

    DEFVARIABLE(thisObj, VariableType::JS_ANY(), Undefined());
    GateRef hclass = LoadHClass(glue, func);

    if (JSFunction::IsNormalFunctionAndCanSkipWbWhenInitialization(getKind)) {
        SetProtoOrHClassToFunction(glue, func, Hole(), MemoryAttribute::NoBarrier());
        SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryAttribute::NoBarrier());
        if (JSFunction::HasPrototype(getKind)) {
            auto funcprotoAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                            ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
            if (getKind == FunctionKind::BASE_CONSTRUCTOR || getKind == FunctionKind::GENERATOR_FUNCTION ||
                getKind == FunctionKind::ASYNC_GENERATOR_FUNCTION) {
                SetPropertyInlinedProps(glue, func, hclass, funcprotoAccessor,
                                        Int32(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
                auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::FUNCTION_NAME_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryAttribute::NoBarrier());
                funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryAttribute::NoBarrier());
                if (getKind != FunctionKind::BASE_CONSTRUCTOR) {
                    thisObj = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(InitializeGeneratorFunction), {kind});
                    SetProtoOrHClassToFunction(glue, func, *thisObj);
                }
            } else if (!JSFunction::IsClassConstructor(getKind)) {
                CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                    RTSTUB_ID(FunctionDefineOwnProperty), {func, funcprotoAccessor, kind});
            }
            Jump(&exit);
        } else if (JSFunction::HasAccessor(getKind)) {
            auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                       ConstantIndex::FUNCTION_NAME_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                  ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
            SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                                    VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
            Jump(&exit);
        }
    } else {
        SetLexicalEnvToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
        SetHomeObjectToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
        SetProtoOrHClassToFunction(glue, func, Hole(), MemoryAttribute::NoBarrier());
        SetWorkNodePointerToFunction(glue, func, NullPtr(), MemoryAttribute::NoBarrier());
        SetMethodToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());

        BRANCH(HasPrototype(kind), &hasProto, &notProto);
        Bind(&hasProto);
        {
            auto funcprotoAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                            ConstantIndex::FUNCTION_PROTOTYPE_ACCESSOR);
            BRANCH(IsBaseKind(kind), &isBase, &notBase);
            Bind(&isBase);
            {
                SetPropertyInlinedProps(glue, func, hclass, funcprotoAccessor,
                                        Int32(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
                auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::FUNCTION_NAME_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryAttribute::NoBarrier());
                funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_ANY(),
                                        MemoryAttribute::NoBarrier());
                BRANCH(IsGeneratorKind(kind), &isGenerator, &exit);
                Bind(&isGenerator);
                {
                    thisObj = CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(InitializeGeneratorFunction), {kind});
                    SetProtoOrHClassToFunction(glue, func, *thisObj);
                    Jump(&exit);
                }
            }
            Bind(&notBase);
            {
                BRANCH(IsClassConstructorKind(kind), &exit, &notClassConstructor);
                Bind(&notClassConstructor);
                {
                    CallRuntimeWithGlobalEnv(glue, GetCurrentGlobalEnv(),
                        RTSTUB_ID(FunctionDefineOwnProperty), {func, funcprotoAccessor, kind});
                    Jump(&exit);
                }
            }
        }
        Bind(&notProto);
        {
            BRANCH(HasAccessor(kind), &hasAccess, &exit);
            Bind(&hasAccess);
            {
                auto funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::FUNCTION_NAME_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor, Int32(JSFunction::NAME_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
                funcAccessor = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                      ConstantIndex::FUNCTION_LENGTH_ACCESSOR);
                SetPropertyInlinedProps(glue, func, hclass, funcAccessor,
                                        Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX),
                                        VariableType::JS_ANY(), MemoryAttribute::NoBarrier());
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto emptyProfileTypeInfoCell = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                           ConstantIndex::EMPTY_PROFILE_TYPE_INFO_CELL_INDEX);
    SetRawProfileTypeInfoToFunction(glue, func, emptyProfileTypeInfoCell, MemoryAttribute::NoBarrier());
    env->SubCfgExit();
    return;
}

void BuiltinsFunctionStubBuilder::InitializeFunctionWithMethod(GateRef glue,
    GateRef func, GateRef method, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    SetCallableToBitfield(glue, hclass, true);
    SetMethodToFunction(glue, func, method);

    SetBitFieldToFunction(glue, func, Int32(0));
    SetMachineCodeToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());
    SetBaselineJitCodeToFunction(glue, func, Undefined(), MemoryAttribute::NoBarrier());

    Label isNativeMethod(env);
    Label checkAotStatus(env);
    Label hasCompiledStatus(env);
    Label tryInitFuncCodeEntry(env);
    BRANCH(IsNativeMethod(method), &isNativeMethod, &checkAotStatus);
    Bind(&isNativeMethod);
    {
        SetNativePointerToFunctionFromMethod(glue, func, method);
        Jump(&exit);
    }
    Bind(&checkAotStatus);
    BRANCH(IsAotWithCallField(method), &hasCompiledStatus, &tryInitFuncCodeEntry);
    Bind(&hasCompiledStatus);
    {
        SetCompiledCodeFlagToFunctionFromMethod(glue, func, method);
        Jump(&tryInitFuncCodeEntry);
    }
    // Notice: we set code entries for all function to deal with these situations
    // 1) AOT compiled method, set AOT compiled code entry
    // 2) define func with the deopted method, set the AOTToAsmInterpBridge
    Bind(&tryInitFuncCodeEntry);
    {
        SetCodeEntryToFunctionFromMethod(glue, func, method);
        Jump(&exit);
    }

    Bind(&exit);
    env->SubCfgExit();
    return;
}
}  // namespace panda::ecmascript::kungfu
