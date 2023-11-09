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
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::kungfu {
void BuiltinsArrayStubBuilder::Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if all the conditions below are satisfied:
    // (1) this is an empty array with constructor not reset (see ArraySpeciesCreate for details);
    // (2) At most one argument;
    // (3) all the arguments (if exists) are empty arrays.
    JsArrayRequirements reqThisValue;
    reqThisValue.defaultConstructor = true;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, reqThisValue), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    {
        Label atMostOneArg(env);
        Label argValIsEmpty(env);
        GateRef numArgsAsInt32 = TruncPtrToInt32(numArgs);
        Branch(Int32LessThanOrEqual(numArgsAsInt32, Int32(1)), &atMostOneArg, slowPath);
        Bind(&atMostOneArg);
        {
            Label exactlyOneArg(env);
            Branch(Int32Equal(numArgsAsInt32, Int32(0)), &argValIsEmpty, &exactlyOneArg);
            Bind(&exactlyOneArg);
            GateRef argVal = GetCallArg0(numArgs);
            JsArrayRequirements reqArgVal;
            Branch(IsJsArrayWithLengthLimit(glue, argVal, MAX_LENGTH_ZERO, reqArgVal), &argValIsEmpty, slowPath);
            // Creates an empty array on fast path
            Bind(&argValIsEmpty);
            NewObjectStubBuilder newBuilder(this);
            result->WriteVariable(newBuilder.CreateEmptyArray(glue));
            Jump(exit);
        }
    }
}

void BuiltinsArrayStubBuilder::Filter(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if all the conditions below are satisfied:
    // (1) this is an empty array with constructor not reset (see ArraySpeciesCreate for details);
    // (2) callbackFn is callable (otherwise a TypeError shall be thrown in the slow path)
    JsArrayRequirements req;
    req.defaultConstructor = true;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    {
        Label isCallable(env);
        Label isHeapObject(env);
        Branch(TaggedIsHeapObject(GetCallArg0(numArgs)), &isHeapObject, slowPath);
        Bind(&isHeapObject);
        Branch(IsCallable(GetCallArg0(numArgs)), &isCallable, slowPath);
        // Creates an empty array on fast path
        Bind(&isCallable);
        NewObjectStubBuilder newBuilder(this);
        result->WriteVariable(newBuilder.CreateEmptyArray(glue));
        Jump(exit);
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

void BuiltinsArrayStubBuilder::Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path if:
    // (1) this is an empty array with constructor not reset (see ArraySpeciesCreate for details);
    // (2) no arguments exist
    JsArrayRequirements req;
    req.defaultConstructor = true;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ZERO, req), &thisIsEmpty, slowPath);
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
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::Reverse([[maybe_unused]] GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsEmpty(env);
    // Fast path is this is an array of length 0 or 1
    JsArrayRequirements req;
    Branch(IsJsArrayWithLengthLimit(glue, thisValue, MAX_LENGTH_ONE, req), &thisIsEmpty, slowPath);
    Bind(&thisIsEmpty);
    // Returns thisValue on fast path
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
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *index, *value);
            Jump(&setLength);
        }
        Bind(&twoArg);
        {
            value = GetCallArg0(numArgs);
            index = Int32Add(oldLength, Int32(0));  // 0 slot index
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *index, *value);

            value = GetCallArg1(numArgs);
            index = Int32Add(oldLength, Int32(1));  // 1 slot index
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *index, *value);
            Jump(&setLength);
        }
    }
    Bind(&setLength);
    SetArrayLength(glue, thisValue, newLength);
    result->WriteVariable(IntToTaggedPtr(newLength));
    Jump(exit);
}

void BuiltinsArrayStubBuilder::Includes(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), TaggedFalse());
    Label isObject(env);
    Label atLeastOneArg(env);
    Label notFound(env);
    Label thisLenNotZero(env);
    Branch(TaggedIsObject(thisValue), &isObject, slowPath);
    Bind(&isObject);
    Branch(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &atLeastOneArg, slowPath);
    Bind(&atLeastOneArg);
    GateRef thisLen = GetArrayLength(thisValue);
    Branch(Int32Equal(thisLen, Int32(0)), &notFound, &thisLenNotZero);
    Bind(&thisLenNotZero);
    {
        DEFVARIABLE(fromIndex, VariableType::INT32(), Int32(0));
        Label getArgTwo(env);
        Label nextProcess(env);
        Label nextCheck(env);
        Branch(Int64Equal(numArgs, IntPtr(2)), &getArgTwo, &nextProcess); // 2: 2 parameters
        Bind(&getArgTwo);
        {
            Label secondArgIsNumber(env);
            GateRef fromIndexTemp = GetCallArg1(numArgs);
            Branch(TaggedIsNumber(fromIndexTemp), &secondArgIsNumber, slowPath);
            Bind(&secondArgIsNumber);
            fromIndex = GetInt32OfTInt(fromIndexTemp);
            Jump(&nextProcess);
        }
        Bind(&nextProcess);
        {
            Label setBackZero(env);
            Label calculateFrom(env);
            Branch(Int32GreaterThanOrEqual(*fromIndex, thisLen), &notFound, &nextCheck);
            Bind(&nextCheck);
            Branch(Int32LessThan(*fromIndex, Int32(0)), &setBackZero, &calculateFrom);
            Bind(&setBackZero);
            {
                fromIndex = Int32(0);
                Jump(&calculateFrom);
            }
            Bind(&calculateFrom);
            {
                DEFVARIABLE(from, VariableType::INT32(), Int32(0));
                Label isFromIndex(env);
                Label notOnlyFromIndex(env);
                Label startLoop(env);
                Branch(Int32GreaterThanOrEqual(*fromIndex, Int32(0)), &isFromIndex, &notOnlyFromIndex);
                Bind(&isFromIndex);
                {
                    from = *fromIndex;
                    Jump(&startLoop);
                }
                Bind(&notOnlyFromIndex); 
                {
                    Label isLenFromIndex(env);
                    Label isZero(env);
                    Branch(Int32GreaterThanOrEqual(Int32Add(thisLen, *fromIndex), Int32(0)), &isLenFromIndex, &isZero);
                    Bind(&isLenFromIndex);
                    {
                        from = Int32Add(thisLen, *fromIndex);
                        Jump(&startLoop);
                    }
                    Bind(&isZero);
                    {
                        from = Int32(0);
                        Jump(&startLoop);
                    }
                }
                Bind(&startLoop);
                {
                    GateRef searchElement = GetCallArg0(numArgs);
                    GateRef elements = GetElementsArray(thisValue);
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
                            GateRef value = GetValueFromTaggedArray(elements, *from);
                            Branch(BoolOr(TaggedIsHole(value), TaggedIsUndefined(value)), slowPath,
                                &notHoleOrUndefValue);
                            Bind(&notHoleOrUndefValue);
                            {
                                Label valueFound(env);
                                GateRef valueEqual = StubBuilder::SameValueZero(glue, searchElement, value);
                                Branch(valueEqual, &valueFound, &loopEnd);
                                Bind(&valueFound);
                                {
                                    result->WriteVariable(TaggedTrue());
                                    Jump(exit);
                                }
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
    Bind(&notFound);
    {
        result->WriteVariable(TaggedFalse());
        Jump(exit);
    }
}
}  // namespace panda::ecmascript::kungfu
