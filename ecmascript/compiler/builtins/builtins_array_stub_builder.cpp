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
            ProfileOperation callback;
            result->WriteVariable(newBuilder.CreateEmptyArray(glue, callback));
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
        Branch(IsCallable(GetCallArg0(numArgs)), &isCallable, slowPath);
        // Creates an empty array on fast path
        Bind(&isCallable);
        NewObjectStubBuilder newBuilder(this);
        ProfileOperation callback;
        result->WriteVariable(newBuilder.CreateEmptyArray(glue, callback));
        Jump(exit);
    }
}

// Note: unused arguments are reserved for further development
void BuiltinsArrayStubBuilder::ForEach([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
    [[maybe_unused]] Variable *result, Label *exit, Label *slowPath)
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
    // Do nothing on fast path
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
        ProfileOperation callback;
        result->WriteVariable(newBuilder.CreateEmptyArray(glue, callback));
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
}  // namespace panda::ecmascript::kungfu
