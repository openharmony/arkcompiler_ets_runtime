/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_shared_array_stub_builder.h"

#include "ecmascript/compiler/access_object_stub_builder.h"
#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/global_env.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/builtins/builtins_shared_array.h"

namespace panda::ecmascript::kungfu {

GateRef BuiltinsSharedArrayStubBuilder::NewSharedArrayWithHClass(GateRef glue, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    // 1. create SharedArray
    GateRef newArray = newBuilder.NewSObject(glue, hclass);
    result.WriteVariable(newArray);

    // 2. init SharedArray
    // 2.1 set shared array length
    GateRef lengthOffset = IntPtr(JSSharedArray::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, newArray, lengthOffset, Int32(0));

    // 2.2 set TrackInfo to Undefined
    GateRef trackInfoOffset = IntPtr(JSSharedArray::TRACK_INFO_OFFSET);
    Store(VariableType::JS_ANY(), glue, newArray, trackInfoOffset, Undefined());

    // 2.3 set ModRecord to 0
    GateRef modRecordOffset = IntPtr(JSSharedArray::MOD_RECORD_OFFSET);
    Store(VariableType::INT32(), glue, newArray, modRecordOffset, Int32(0));

    // 2.4 set length accessor
    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                              ConstantIndex::SHARED_ARRAY_LENGTH_ACCESSOR);
    GateRef lengthAccessorOffset = IntPtr(JSSharedArray::SIZE +
        JSSharedArray::LENGTH_INLINE_PROPERTY_INDEX * JSTaggedValue::TaggedTypeSize());
    Store(VariableType::JS_ANY(), glue, newArray, lengthAccessorOffset, accessor);

    // 3. set extensible to false
    SetExtensibleToBitfield(glue, newArray, false);
    Jump(&exit);
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

void BuiltinsSharedArrayStubBuilder::FastCreateSharedArrayWithArgv(GateRef glue, Variable *res, GateRef argc,
                                                                   GateRef hclass, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    // 1. new taggedArray in shared old space
    GateRef len = TruncInt64ToInt32(argc);
    GateRef elements = newBuilder.NewSTaggedArray(glue, len);

    // 2. set argv to taggedArray in for-loop
    DEFVARIABLE(k, VariableType::INT64(), Int64(0));
    DEFVARIABLE(value, VariableType::JS_ANY(), Undefined());
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label setValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int64LessThan(*k, argc), &setValue, &loopExit);
        Bind(&setValue);
        {
            Label isHole(env);
            Label notHole(env);
            value = GetArgFromArgv(glue, *k);
            // transfer Hole to Undefined
            BRANCH(TaggedIsHole(*value), &isHole, &notHole);
            Bind(&isHole);
            {
                value = TaggedUndefined();
                Jump(&notHole);
            }
            Bind(&notHole);
            {
                // check argv type using IsSharedType
                Label validType(env);
                Label invalidType(env);

                BRANCH(TaggedIsSharedType(glue, *value), &validType, &invalidType);
                Bind(&validType);
                {
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, TruncInt64ToInt32(*k), *value);
                    Jump(&loopEnd);
                }
                Bind(&invalidType);
                {
                    Jump(slowPath);
                }
            }
        }
        Bind(&loopEnd);
        {
            k = Int64Add(*k, Int64(1));
            LoopEndWithCheckSafePoint(&loopHead, env, glue);
        }
    }
    Bind(&loopExit);
    {
        // 3. create SharedArray
        GateRef newArray = newBuilder.NewSObject(glue, hclass);

        // 4. init SharedArray
        GateRef lengthOffset = IntPtr(JSSharedArray::LENGTH_OFFSET);
        Store(VariableType::INT32(), glue, newArray, lengthOffset, len);

        GateRef trackInfoOffset = IntPtr(JSSharedArray::TRACK_INFO_OFFSET);
        Store(VariableType::JS_ANY(), glue, newArray, trackInfoOffset, Undefined());

        GateRef modRecordOffset = IntPtr(JSSharedArray::MOD_RECORD_OFFSET);
        Store(VariableType::INT32(), glue, newArray, modRecordOffset, Int32(0));

        GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                  ConstantIndex::SHARED_ARRAY_LENGTH_ACCESSOR);
        GateRef lengthAccessorOffset = IntPtr(JSSharedArray::SIZE +
            JSSharedArray::LENGTH_INLINE_PROPERTY_INDEX * JSTaggedValue::TaggedTypeSize());
        Store(VariableType::JS_ANY(), glue, newArray, lengthAccessorOffset, accessor);

        // 5. set element to shared array
        SetElementsArray(VariableType::JS_POINTER(), glue, newArray, elements);

        // 6. set extensible to false
        SetExtensibleToBitfield(glue, newArray, false);

        res->WriteVariable(newArray);
        Jump(exit);
    }
}

// len and initValue should be valid
void BuiltinsSharedArrayStubBuilder::FastCreateSharedArrayWithLen(GateRef glue, Variable *res, GateRef len,
    GateRef initValue, GateRef hclass, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    // 1. new taggedArray in shared old space
    GateRef elements = newBuilder.NewSTaggedArray(glue, len);

    // 2. set argv to taggedArray in for-loop
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label setValue(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32LessThan(*k, len), &setValue, &loopExit);
        Bind(&setValue);
        {
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, *k, initValue);
            Jump(&loopEnd);
        }
        Bind(&loopEnd);
        {
            k = Int32Add(*k, Int32(1));
            LoopEndWithCheckSafePoint(&loopHead, env, glue);
        }
    }
    Bind(&loopExit);
    {
        // 3. create SharedArray
        GateRef newArray = newBuilder.NewSObject(glue, hclass);

        // 4. init SharedArray
        GateRef lengthOffset = IntPtr(JSSharedArray::LENGTH_OFFSET);
        Store(VariableType::INT32(), glue, newArray, lengthOffset, len);

        GateRef trackInfoOffset = IntPtr(JSSharedArray::TRACK_INFO_OFFSET);
        Store(VariableType::JS_ANY(), glue, newArray, trackInfoOffset, Undefined());

        GateRef modRecordOffset = IntPtr(JSSharedArray::MOD_RECORD_OFFSET);
        Store(VariableType::INT32(), glue, newArray, modRecordOffset, Int32(0));

        GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                  ConstantIndex::SHARED_ARRAY_LENGTH_ACCESSOR);
        GateRef lengthAccessorOffset = IntPtr(JSSharedArray::SIZE +
            JSSharedArray::LENGTH_INLINE_PROPERTY_INDEX * JSTaggedValue::TaggedTypeSize());
        Store(VariableType::JS_ANY(), glue, newArray, lengthAccessorOffset, accessor);

        // 5. set element to shared array
        SetElementsArray(VariableType::JS_POINTER(), glue, newArray, elements);

        // 6. set extensible to false
        SetExtensibleToBitfield(glue, newArray, false);

        res->WriteVariable(newArray);
        Jump(exit);
    }
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::ArrayConstructor()
void BuiltinsSharedArrayStubBuilder::GenSharedArrayConstructor(GateRef glue, GateRef nativeCode,
    GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsHeapObject(env);
    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label exit(env);

    // Check whether newTarget is undefined
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(glue, newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label fastGetHclass(env);
        Label intialHClassIsHClass(env);
        GateRef globalEnv = GetCurrentGlobalEnv();
        auto sharedArrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::SHARED_ARRAY_FUNCTION_INDEX);
        BRANCH(Equal(sharedArrayFunc, newTarget), &fastGetHclass, &slowPath);
        Bind(&fastGetHclass);
        GateRef intialHClass =
            Load(VariableType::JS_ANY(), glue, newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        DEFVARIABLE(arrayLength, VariableType::INT64(), Int64(0));
        BRANCH(IsJSHClass(glue, intialHClass), &intialHClassIsHClass, &slowPath);
        Bind(&intialHClassIsHClass);
        {
            Label hasArg(env);
            Label arrayCreateNoArg(env);
            BRANCH(Int64Equal(numArgs, IntPtr(0)), &arrayCreateNoArg, &hasArg);
            Bind(&arrayCreateNoArg);
            {
                res = NewSharedArrayWithHClass(glue, intialHClass);
                Jump(&exit);
            }
            Bind(&hasArg);
            {
                Label multiArgLengthValid(env);
                // Set element in for loop
                BRANCH(Int64LessThan(numArgs, IntPtr(JSObject::MAX_GAP)), &multiArgLengthValid, &slowPath);
                Bind(&multiArgLengthValid);
                {
                    FastCreateSharedArrayWithArgv(glue, &res, numArgs, intialHClass, &exit, &slowPath);
                }
            }
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

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::Create()
void BuiltinsSharedArrayStubBuilder::Create(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    Label argsValid(env);
    // 1. Check argument count, 2 means the expected number of input arguments
    BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(2)), &argsValid, slowPath);
    Bind(&argsValid);
    {
        // 2. Get first argument (length) and check if it's a number
        DEFVARIABLE(arrayLength, VariableType::INT32(), Int32(0));
        GateRef lengthValue = GetCallArg0(numArgs);
        Label lengthIsNumber(env);
        BRANCH(TaggedIsNumber(lengthValue), &lengthIsNumber, slowPath);
        Bind(&lengthIsNumber);
        {
            // 3. Check whether length is vaild
            Label lengthConverted(env);
            Label lengthIsInt(env);
            Label lengthIsDouble(env);
            BRANCH(TaggedIsInt(lengthValue), &lengthIsInt, &lengthIsDouble);
            Bind(&lengthIsInt);
            {
                Label validIntLength(env);
                GateRef intLen = GetInt64OfTInt(lengthValue);
                GateRef isGEZero = Int64GreaterThanOrEqual(intLen, Int64(0));
                GateRef isLEMaxLen = Int64LessThanOrEqual(intLen, Int64(JSSharedArray::MAX_ARRAY_INDEX));
                BRANCH(BitAnd(isGEZero, isLEMaxLen), &validIntLength, slowPath);
                Bind(&validIntLength);
                {
                    arrayLength = TruncInt64ToInt32(intLen);
                    Jump(&lengthConverted);
                }
            }
            Bind(&lengthIsDouble);
            {
                Label validDoubleLength(env);
                GateRef doubleLength = GetDoubleOfTDouble(lengthValue);
                GateRef doubleToInt = DoubleToInt(glue, doubleLength);
                GateRef intToDouble = CastInt64ToFloat64(SExtInt32ToInt64(doubleToInt));
                GateRef doubleEqual = DoubleEqual(doubleLength, intToDouble);
                GateRef doubleLEMaxLen =
                    DoubleLessThanOrEqual(doubleLength, Double(JSSharedArray::MAX_ARRAY_INDEX));
                BRANCH(BitAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, slowPath);
                Bind(&validDoubleLength);
                {
                    arrayLength = doubleToInt;
                    Jump(&lengthConverted);
                }
            }

            Bind(&lengthConverted);
            {
                // 4. Get second argument (initValue) and check if it's shared type
                GateRef initValue = GetCallArg1(numArgs);
                Label initValueValid(env);
                Label thisIsConstructor(env);
                Label arrayCreated(env);
                BRANCH(TaggedIsSharedType(glue, initValue), &initValueValid, slowPath);
                Bind(&initValueValid);
                GateRef globalEnv = GetCurrentGlobalEnv();
                GateRef sharedArrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                            GlobalEnv::SHARED_ARRAY_FUNCTION_INDEX);
                BRANCH(IsConstructor(glue, thisValue), &thisIsConstructor, &arrayCreated);
                Bind(&thisIsConstructor);
                BRANCH(Equal(thisValue, sharedArrayFunc), &arrayCreated, slowPath);
                Bind(&arrayCreated);
                GateRef initialHClass = Load(VariableType::JS_ANY(), glue, sharedArrayFunc,
                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                FastCreateSharedArrayWithLen(glue, result, *arrayLength, initValue, initialHClass, exit, slowPath);
            }
        }
    }
}

}  // namespace panda::ecmascript::kungfu
