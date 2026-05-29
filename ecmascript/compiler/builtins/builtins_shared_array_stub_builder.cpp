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

#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
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
    Label slowPath1(env);
    Label exit(env);

    // Check whether newTarget is undefined
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(glue, newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label fastGetHclass(env);
        Label initialHClassIsHClass(env);
        GateRef globalEnv = GetCurrentGlobalEnv();
        auto sharedArrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
            GlobalEnv::SHARED_ARRAY_FUNCTION_INDEX);
        BRANCH(Equal(sharedArrayFunc, newTarget), &fastGetHclass, &slowPath1);
        Bind(&fastGetHclass);
        GateRef initialHClass =
            Load(VariableType::JS_ANY(), glue, newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        DEFVARIABLE(arrayLength, VariableType::INT64(), Int64(0));
        BRANCH(IsJSHClass(glue, initialHClass), &initialHClassIsHClass, &slowPath1);
        Bind(&initialHClassIsHClass);
        {
            Label hasArg(env);
            Label arrayCreateNoArg(env);
            BRANCH(Int64Equal(numArgs, IntPtr(0)), &arrayCreateNoArg, &hasArg);
            Bind(&arrayCreateNoArg);
            {
                res = NewSharedArrayWithHClass(glue, initialHClass);
                Jump(&exit);
            }
            Bind(&hasArg);
            {
                Label multiArgLengthValid(env);
                // Set element in for loop
                BRANCH(Int64LessThan(numArgs, IntPtr(JSObject::MAX_GAP)), &multiArgLengthValid, &slowPath);
                Bind(&multiArgLengthValid);
                {
                    FastCreateSharedArrayWithArgv(glue, &res, numArgs, initialHClass, &exit, &slowPath);
                }
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
    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue, { glue, nativeCode, func, thisValue, numArgs, argv }, true);
        Jump(&exit);
    }

    Bind(&exit);
    Return(*res);
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::IsArray()
void BuiltinsSharedArrayStubBuilder::IsArray(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef obj = GetCallArg0(numArgs);

    Label isHeapObj(env);
    Label notHeapObj(env);
    BRANCH(TaggedIsHeapObject(obj), &isHeapObj, &notHeapObj);
    Bind(&isHeapObj);
    {
        Label isJSSharedArray(env);
        Label notJSSharedArray(env);
        GateRef objectType = GetObjectType(LoadHClass(glue, obj));
        BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_SHARED_ARRAY))),
            &isJSSharedArray, &notJSSharedArray);
        Bind(&isJSSharedArray);
        {
            result->WriteVariable(TaggedTrue());
            Jump(exit);
        }
        Bind(&notJSSharedArray);
        {
            BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_PROXY))), slowPath, &notHeapObj);
        }
    }
    Bind(&notHeapObj);
    {
        result->WriteVariable(TaggedFalse());
        Jump(exit);
    }
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

void BuiltinsSharedArrayStubBuilder::Push(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObj(env);
    Label checkNumArgs(env);
    Label pushImpl(env);

    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObj, slowPath);
    Bind(&isHeapObj);
    {
        BRANCH_LIKELY(IsJsSArray(glue, thisValue), &checkNumArgs, slowPath);
    }
    Bind(&checkNumArgs);
    {
        // now unsupport more than 2 args in ir
        BRANCH_LIKELY(Int32LessThanOrEqual(ChangeIntPtrToInt32(numArgs), Int32(2)), &pushImpl, slowPath);
    }
    Bind(&pushImpl);
    {
        *result = PushImpl(glue, thisValue, numArgs);
        Jump(exit);
    }
}

GateRef BuiltinsSharedArrayStubBuilder::PushImpl(GateRef glue, GateRef thisValue, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label canWriteExit(env);
    Label writeDoneExit(env);
    Label exit(env);
    Label hasScopeException(env);
    Label notScopeException(env);
    Label setLength(env);
    Label smallArgs(env);
    Label throwParamError(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    ConcurrentApiScopeCanWrite<JSSharedArray>(glue, thisValue, scopeEntered, &canWriteExit);
    Bind(&canWriteExit);
    BRANCH(HasPendingException(glue), &hasScopeException, &notScopeException);
    Bind(&hasScopeException);
    {
        res = Exception();
        Jump(&exit);
    }
    Bind(&notScopeException);
    GateRef oldSharedArrayLength = GetSharedArrayLength(thisValue);
    res = IntToTaggedPtr(oldSharedArrayLength);
    BRANCH_UNLIKELY(Int32Equal(ChangeIntPtrToInt32(numArgs), Int32(0)), &exit, &smallArgs);

    Bind(&smallArgs);
    GateRef newSharedArrayLength = Int32Add(oldSharedArrayLength, ChangeIntPtrToInt32(numArgs));

    DEFVARIABLE(elements, VariableType::JS_ANY(), GetElementsArray(glue, thisValue));
    GateRef oldElementCapacity = GetLengthOfTaggedArray(*elements);
    Label grow(env);
    Label setValue(env);
    BRANCH(Int32GreaterThan(newSharedArrayLength, oldElementCapacity), &grow, &setValue);
    Bind(&grow);
    {
        elements = GrowElementsCapacity(glue, thisValue, newSharedArrayLength);
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
            Label arg0ValidType(env);
            value = GetCallArg0(numArgs);
            BRANCH(TaggedIsSharedType(glue, *value), &arg0ValidType, &throwParamError);
            Bind(&arg0ValidType);
            index = Int32Add(oldSharedArrayLength, Int32(0)); // 0 slot index
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *index, *value);
            Jump(&setLength);
        }
        Bind(&twoArg);
        {
            Label arg0ValidType(env);
            Label arg1ValidType(env);
            DEFVARIABLE(index2, VariableType::INT32(), Int32(0));
            DEFVARIABLE(value2, VariableType::JS_ANY(), Undefined());
            value = GetCallArg0(numArgs);
            value2 = GetCallArg1(numArgs);
            BRANCH(TaggedIsSharedType(glue, *value), &arg0ValidType, &throwParamError);
            Bind(&arg0ValidType);
            BRANCH(TaggedIsSharedType(glue, *value2), &arg1ValidType, &throwParamError);
            Bind(&arg1ValidType);
            index = Int32Add(oldSharedArrayLength, Int32(0)); // 0 slot index
            index2 = Int32Add(oldSharedArrayLength, Int32(1)); // 1 slot index
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *index, *value);
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, *elements, *index2, *value2);
            Jump(&setLength);
        }
    }
    Bind(&setLength);
    {
        Label hasSetLengthException(env);
        Label notSetLengthException(env);
        SetSharedArrayLength(glue, thisValue, newSharedArrayLength);
        BRANCH(HasPendingException(glue), &hasSetLengthException, &notSetLengthException);
        Bind(&hasSetLengthException);
        {
            res = Exception();
            Jump(&exit);
        }
        Bind(&notSetLengthException);
        {
            res.WriteVariable(IntToTaggedPtr(newSharedArrayLength));
            Jump(&exit);
        }
    }
    Bind(&throwParamError);
    {
        int errorCode = containers::ErrorFlag::TYPE_ERROR;
        int errorMsg = GET_MESSAGE_STRING_ID(OnlyAcceptSendableValue1);
        ThrowContainerBusinessError(glue, errorCode, errorMsg);
        res = Exception();
        Jump(&exit);
    }
    Bind(&exit);
    ConcurrentApiScopeWriteDone<JSSharedArray>(glue, thisValue, scopeEntered, &writeDoneExit);
    Bind(&writeDoneExit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsSharedArrayStubBuilder::GrowElementsCapacity(GateRef glue, GateRef thisValue,
                                                             GateRef capacity)
{
    // Reference: JSObject::GrowElementsCapacity for IsJSSArray case
    // highGrowth = true
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(newElements, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(newCapacity, VariableType::INT32(), Int32(0));

    // 1. Get hint from TrackInfo and compute new capacity
    // Reference: JSObject::GrowElementsCapacity:98-100
    GateRef hint = GetSharedArrayHintLength(glue, thisValue);
    newCapacity = ComputeElementCapacityWithHint(capacity, hint);

    // 2. if (newCapacity == 0)
    Label updateCapacity(env);
    Label startCopyArray(env);
    BRANCH(Int32Equal(*newCapacity, Int32(0)), &updateCapacity, &startCopyArray);
    Bind(&updateCapacity);
    newCapacity = ComputeElementCapacityHighGrowth(capacity);
    Jump(&startCopyArray);

    Bind(&startCopyArray);
    GateRef oldElements = GetElementsArray(glue, thisValue);
    GateRef oldElementCapacity = GetLengthOfTaggedArray(oldElements);

    // Copy array with new capacity in SHARED_OLD_SPACE
    // Reference: ObjectFactory::CopyArray with MemSpaceType::SHARED_OLD_SPACE
    NewObjectStubBuilder newBuilder(this);
    newElements = newBuilder.CopyArray(glue, oldElements, oldElementCapacity, *newCapacity,
                                       RegionSpaceFlag::IN_SHARED_OLD_SPACE);

    // Set new elements to the object
    // Reference: obj->SetElements(thread, newElements)
    SetElementsArray(VariableType::JS_POINTER(), glue, thisValue, *newElements);
    Jump(&exit);

    Bind(&exit);
    auto ret = *newElements;
    env->SubCfgExit();
    return ret;
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::Pop()
// Entry point: type-check thisValue and dispatch to PopImpl or slowPath.
void BuiltinsSharedArrayStubBuilder::Pop(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObj(env);
    Label isSArray(env);

    // @ref BuiltinsSharedArray::Pop: ARRAY_CHECK_SHARED_ARRAY("The pop method cannot be bound.")
    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObj, slowPath);
    Bind(&isHeapObj);
    BRANCH_LIKELY(IsJsSArray(glue, thisValue), &isSArray, slowPath);
    Bind(&isSArray);
    {
        *result = PopImpl(glue, thisValue);
        Jump(exit);
    }
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::Pop() + PopInner()
// Core Pop logic inside ConcurrentApiScope<JSSharedArray, ModType::WRITE>.
GateRef BuiltinsSharedArrayStubBuilder::PopImpl(GateRef glue, GateRef thisValue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label canWriteExit(env);
    Label writeDoneExit(env);
    Label hasScopeException(env);
    Label notScopeException(env);
    Label lenIsZero(env);
    Label lenNotZero(env);
    Label localExit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    // @ref BuiltinsSharedArray::Pop: ConcurrentApiScope<JSSharedArray, ModType::WRITE> scope(thread, thisHandle)
    ConcurrentApiScopeCanWrite<JSSharedArray>(glue, thisValue, scopeEntered, &canWriteExit);
    Bind(&canWriteExit);
    BRANCH(HasPendingException(glue), &hasScopeException, &notScopeException);
    Bind(&hasScopeException);
    {
        // @ref BuiltinsSharedArray::Pop: RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread)
        res.WriteVariable(Exception());
        Jump(&localExit);
    }
    Bind(&notScopeException);

    // @ref PopInner: int64_t len = ArrayHelper::GetArrayLength(thread, thisHandle)
    GateRef len = GetSharedArrayLength(thisValue);

    // @ref PopInner: if (len == 0) { LengthSetter(thread, thisObjHandle, 0); return Undefined(); }
    BRANCH_UNLIKELY(Int32Equal(len, Int32(0)), &lenIsZero, &lenNotZero);
    Bind(&lenIsZero);
    {
        res.WriteVariable(Undefined());
        Jump(&localExit);
    }
    Bind(&lenNotZero);
    {
        // @ref PopInner: int64_t newLen = len - 1
        GateRef newLen = Int32Sub(len, Int32(1));

        // @ref PopInner: JSHandle<JSTaggedValue> element(thread, ElementAccessor::Get(thread, thisObjHandle, newLen))
        GateRef elements = GetElementsArray(glue, thisValue);
        GateRef element = GetTaggedValueWithElementsKind(glue, thisValue, newLen);

        // @ref PopInner: uint32_t capacity = ElementAccessor::GetElementsLength(thread, thisObjHandle)
        GateRef capacity = GetLengthOfTaggedArray(elements);

        // @ref PopInner: if (TaggedArray::ShouldTrim(capacity, newLen)) { elements->Trim(thread, newLen); }
        // ShouldTrim: (capacity - newLen > MAX_END_UNUSED), where MAX_END_UNUSED = 4
        Label needTrim(env);
        Label noTrim(env);
        Label setNewLen(env);
        GateRef unused = Int32Sub(capacity, newLen);
        BRANCH_UNLIKELY(Int32GreaterThan(unused, Int32(TaggedArray::MAX_END_UNUSED)), &needTrim, &noTrim);
        Bind(&needTrim);
        {
            // @ref TaggedArray::Trim(thread, newLen)
            CallNGCRuntime(glue, RTSTUB_ID(ArrayTrim), {glue, elements, ZExtInt32ToInt64(newLen)});
            Jump(&setNewLen);
        }
        Bind(&noTrim);
        {
            Jump(&setNewLen);
        }
        Bind(&setNewLen);
        {
            // @ref PopInner: JSSharedArray::Cast(*thisObjHandle)->SetArrayLength(thread, newLen)
            SetSharedArrayLength(glue, thisValue, newLen);
            res.WriteVariable(element);
            Jump(&localExit);
        }
    }
    Bind(&localExit);
    ConcurrentApiScopeWriteDone<JSSharedArray>(glue, thisValue, scopeEntered, &writeDoneExit);
    Bind(&writeDoneExit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::Find()
// Entry point: type-check thisValue and callback, then dispatch to FindImpl or slowPath.
void BuiltinsSharedArrayStubBuilder::Find(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isHeapObj(env);
    Label isSArray(env);

    // @ref BuiltinsSharedArray::Find: ARRAY_CHECK_SHARED_ARRAY("The find method cannot be bound.")
    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObj, slowPath);
    Bind(&isHeapObj);
    BRANCH_LIKELY(IsJsSArray(glue, thisValue), &isSArray, slowPath);
    Bind(&isSArray);
    {
        *result = FindImpl(glue, thisValue, numArgs);
        Jump(exit);
    }
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::Find()
// Core Find logic inside ConcurrentApiScope<JSSharedArray>.
GateRef BuiltinsSharedArrayStubBuilder::FindImpl(GateRef glue, GateRef thisValue, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label hasScopeException(env);
    Label notScopeException(env);
    Label callbackIsHeapObj(env);
    Label callbackIsCallable(env);
    Label notCallable(env);
    Label localExit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label canReadExit(env);
    Label readDoneExit(env);
    DEFVARIABLE(expectModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(desiredModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    // @ref BuiltinsSharedArray::Find: ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle)
    ConcurrentApiScopeCanRead<JSSharedArray>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &canReadExit);
    Bind(&canReadExit);
    BRANCH(HasPendingException(glue), &hasScopeException, &notScopeException);
    Bind(&hasScopeException);
    {
        // @ref BuiltinsSharedArray::Find: RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread)
        res.WriteVariable(Exception());
        Jump(&localExit);
    }
    Bind(&notScopeException);

    // @ref Find: int64_t len = ArrayHelper::GetLength(thread, thisHandle)
    GateRef len = GetSharedArrayLength(thisValue);

    // @ref Find: JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0)
    GateRef callbackFnHandle = GetCallArg0(numArgs);

    // @ref Find: if (!callbackFnHandle->IsCallable()) { throw TypeError }
    BRANCH_LIKELY(TaggedIsHeapObject(callbackFnHandle), &callbackIsHeapObj, &notCallable);
    Bind(&callbackIsHeapObj);
    BRANCH_LIKELY(IsCallable(glue, callbackFnHandle), &callbackIsCallable, &notCallable);
    Bind(&notCallable);
    {
        GateRef msgIntId = Int32(GET_MESSAGE_STRING_ID(PredicateNotCallable));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(msgIntId)});
        res.WriteVariable(Exception());
        Jump(&localExit);
    }
    Bind(&callbackIsCallable);

    // @ref Find: JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1)
    GateRef thisArgHandle = GetCallArg1(numArgs);

    // @ref Find: int64_t k = 0; while (k < len) { ... }
    DEFVARIABLE(k, VariableType::INT32(), Int32(0));
    DEFVARIABLE(kValue, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(key, VariableType::JS_ANY(), Undefined());
    Label loopHead(env);
    Label loopBody(env);
    Label loopEnd(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32LessThan(*k, len), &loopBody, &loopExit);
        Bind(&loopBody);
        {
            // @ref Find: kValue.Update(BuiltinsSharedArray::GetElementByKey(thread, thisObjHandle, k))
            // GetElementByKey delegates to ElementAccessor::Get which reads from the elements TaggedArray
            Label outOfRange(env);
            Label getKValue(env);
            Label updateKey(env);
            GateRef elements = GetElementsArray(glue, thisValue);
            GateRef elementsLen = GetLengthOfTaggedArray(elements);
            BRANCH_UNLIKELY(Int32LessThanOrEqual(elementsLen, *k), &outOfRange, &getKValue);
            Bind(&outOfRange);
            {
                kValue = Undefined();
                Jump(&updateKey);
            }
            Bind(&getKValue);
            {
                kValue = GetTaggedValueWithElementsKind(glue, thisValue, *k);
                Jump(&updateKey);
            }
            Bind(&updateKey);

            // @ref Find: key.Update(JSTaggedValue(k))
            key = IntToTaggedPtr(*k);

            // @ref Find: EcmaRuntimeCallInfo *info = NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, ...)
            // @ref Find: info->SetCallArg(kValue, key, thisHandle)
            // @ref Find: JSTaggedValue callResult = JSFunction::Call(info)
            Label hasException(env);
            Label notHasException(env);
            JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
            callArgs.callThisArg3WithReturnArgs = { thisArgHandle, *kValue, *key, thisValue };
            CallStubBuilder callBuilder(this, glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
                Circuit::NullGate(), callArgs);
            GateRef retValue = callBuilder.JSCallDispatch();

            // @ref Find: RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread)
            BRANCH_UNLIKELY(HasPendingException(glue), &hasException, &notHasException);
            Bind(&hasException);
            {
                res.WriteVariable(Exception());
                Jump(&localExit);
            }
            Bind(&notHasException);
            {
                // @ref Find: if (callResult.ToBoolean()) { return kValue; }
                Label found(env);
                BRANCH_NO_WEIGHT(TaggedIsTrue(FastToBoolean(glue, retValue)), &found, &loopEnd);
                Bind(&found);
                {
                    res.WriteVariable(*kValue);
                    Jump(&localExit);
                }
            }
        }
        Bind(&loopEnd);
        k = Int32Add(*k, Int32(1));
        LoopEnd(&loopHead);
    }
    Bind(&loopExit);
    {
        // @ref Find: return JSTaggedValue::Undefined()
        res.WriteVariable(Undefined());
        Jump(&localExit);
    }
    Bind(&localExit);
    ConcurrentApiScopeReadDone<JSSharedArray>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &readDoneExit);
    Bind(&readDoneExit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::IndexOf()
void BuiltinsSharedArrayStubBuilder::IndexOf(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = false;
    return IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
}

// @ref panda::ecmascript::builtins::BuiltinsSharedArray::LastIndexOf()
void BuiltinsSharedArrayStubBuilder::LastIndexOf(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    IndexOfOptions options;
    options.compType = ComparisonType::STRICT_EQUAL;
    options.returnType = IndexOfReturnType::TAGGED_FOUND_INDEX;
    options.holeAsUndefined = false;
    options.reversedOrder = true;
    return IndexOfOptimised(glue, thisValue, numArgs, result, exit, slowPath, options);
}

void BuiltinsSharedArrayStubBuilder::IndexOfOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath, IndexOfOptions options)
{
    auto env = GetEnvironment();
    Label isHeapObj(env);
    Label isSArray(env);
    Label targetNotMutant(env);
    Label indefOfImpl(env);
    Label hasFromIndex(env);

    // Check all slowpath before concurrentApiScope
    // 1. Check IsSharedArray
    // @ref BuiltinsSharedArray::IndexOf: ARRAY_CHECK_SHARED_ARRAY("The indexOf method cannot be bound.")
    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObj, slowPath);
    Bind(&isHeapObj);
    BRANCH_LIKELY(IsJsSArray(glue, thisValue), &isSArray, slowPath);
    Bind(&isSArray);

    // 2. Check IsEnableMutantArray
    BRANCH_UNLIKELY(IsEnableMutantArray(glue), slowPath, &targetNotMutant);
    Bind(&targetNotMutant);

    // 3. Check fromIndex is vaild
    BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(2)), &hasFromIndex, &indefOfImpl); // 2: 2 parameters
    Bind(&hasFromIndex);
    GateRef fromIndexTemp = GetCallArg1(numArgs);
    BRANCH_LIKELY(TaggedIsNumber(fromIndexTemp), &indefOfImpl, slowPath);

    // 4. IndexOfImpl
    Bind(&indefOfImpl);
    *result = IndexOfImpl(glue, thisValue, numArgs, options);
    Jump(exit);
}

GateRef BuiltinsSharedArrayStubBuilder::IndexOfImpl(GateRef glue, GateRef thisValue,
    GateRef numArgs, IndexOfOptions options)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label hasScopeException(env);
    Label notScopeExceptionOrNoCheck(env);
    Label thisLengthNotZero(env);
    Label targetNotByDefault(env);
    Label hasFromIndex(env);
    Label fromIndexDone(env);
    Label beginDispatching(env);
    Label notFound(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label canReadExit(env);
    Label readDoneExit(env);
    DEFVARIABLE(expectModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(desiredModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    // @ref BuiltinsSharedArray::IndexOf: ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle)
    // @ref BuiltinsSharedArray::LastIndexOf: ConcurrentApiScope<JSSharedArray> scope(thread, thisHandle)
    // Acquires READ lock on the shared array for thread-safe concurrent access.
    ConcurrentApiScopeCanRead<JSSharedArray>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &canReadExit);
    Bind(&canReadExit);
    BRANCH(HasPendingException(glue), &hasScopeException, &notScopeExceptionOrNoCheck);
    Bind(&hasScopeException);
    {
        // BuiltinsSharedArray::IndexOf c++ impl do not check PendingException after ConcurrentApiScope
        if ((options.returnType == IndexOfReturnType::TAGGED_FOUND_INDEX) &&
            (options.reversedOrder == false)) {
            Jump(&notScopeExceptionOrNoCheck);
        } else {
            // @ref BuiltinsSharedArray::LastIndexOf: RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread)
            res.WriteVariable(Exception());
            Jump(&exit);
        }
    }
    Bind(&notScopeExceptionOrNoCheck);

    GateRef thisLen = GetSharedArrayLength(thisValue);
    GateRef defaultFromIndex = options.reversedOrder ? Int64Sub(ZExtInt32ToInt64(thisLen), Int64(1)) : Int64(0);
    DEFVARIABLE(fromIndex, VariableType::INT64(), defaultFromIndex);
    DEFVARIABLE(target, VariableType::JS_ANY(), Undefined());

    ASM_ASSERT(GET_MESSAGE_STRING_ID(SharedArrayIndexOf), IsJsSArray(glue, thisValue));

    BRANCH_UNLIKELY(Int32Equal(thisLen, Int32(0)), &notFound, &thisLengthNotZero);
    Bind(&thisLengthNotZero);
    BRANCH(Int64GreaterThanOrEqual(numArgs, IntPtr(2)), &hasFromIndex, &fromIndexDone); // 2: 2 parameters
    Bind(&hasFromIndex);
    {
        GateRef fromIndexTemp = GetCallArg1(numArgs);

        ASM_ASSERT(GET_MESSAGE_STRING_ID(SharedArrayIndexOf), TaggedIsNumber(fromIndexTemp));

        fromIndex.WriteVariable(MakeFromIndex(fromIndexTemp, thisLen, options.reversedOrder));

        // @ref BuiltinsSharedArray::IndexOfStable: RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread)
        Label hasException(env);
        Label notException(env);
        BRANCH(HasPendingException(glue), &hasException, &notException);
        Bind(&hasException);
        {
            res.WriteVariable(Exception());
            Jump(&exit);
        }
        Bind(&notException);

        if (options.reversedOrder) {
            BRANCH(Int64LessThan(*fromIndex, Int64(0)), &notFound, &fromIndexDone);
        } else {
            BRANCH(Int64GreaterThanOrEqual(*fromIndex, ZExtInt32ToInt64(thisLen)), &notFound, &fromIndexDone);
        }
    }
    // Search range [startIndex, endIndex) is ensured to be non-empty
    Bind(&fromIndexDone);
    BRANCH_LIKELY(Int64GreaterThanOrEqual(numArgs, IntPtr(1)), &targetNotByDefault, &beginDispatching);
    Bind(&targetNotByDefault); // Otherwise, let searchElement be undefined
    target.WriteVariable(GetCallArg0(numArgs));
    Jump(&beginDispatching);

    Bind(&beginDispatching);
    GateRef elements = GetElementsArray(glue, thisValue);
    GateRef elementsKind = GetElementsKindFromHClass(LoadHClass(glue, thisValue));

    ASM_ASSERT(GET_MESSAGE_STRING_ID(SharedArrayIndexOf), BoolNot(IsEnableMutantArray(glue)));

    // Special-judges Undefined() first to make sure that Hole() is matched in includes().
    Label undefinedBranch(env);
    Label targetNotUndefined(env);
    BRANCH_UNLIKELY(TaggedIsUndefined(*target), &undefinedBranch, &targetNotUndefined);
    Bind(&undefinedBranch);
    res = IndexOfTaggedUndefined(glue, elements, *fromIndex, thisLen, options);
    Jump(&exit);

    Bind(&targetNotUndefined);
    Label intBranch(env);
    Label doubleBranch(env);
    Label stringBranch(env);
    Label stringOrHoleBranch(env);
    Label bigIntOrObjectBranch(env);
    Label genericBranch(env);

    constexpr int64_t caseKeys[] = {
        Elements::ToUint(ElementsKind::INT),
        Elements::ToUint(ElementsKind::HOLE_INT),
        Elements::ToUint(ElementsKind::NUMBER),
        Elements::ToUint(ElementsKind::HOLE_NUMBER),
        Elements::ToUint(ElementsKind::STRING),
        Elements::ToUint(ElementsKind::HOLE_STRING),
        Elements::ToUint(ElementsKind::OBJECT),
        Elements::ToUint(ElementsKind::HOLE_OBJECT),
    };
    Label *caseLabels[] = {
        &intBranch,
        &intBranch,
        &doubleBranch,
        &doubleBranch,
        &stringBranch,
        &stringOrHoleBranch,
        &bigIntOrObjectBranch,
        &bigIntOrObjectBranch,
    };
    static_assert(std::size(caseKeys) == std::size(caseLabels), "Size mismatch!");
    Switch(elementsKind, &genericBranch, caseKeys, caseLabels, std::size(caseKeys));

    Bind(&intBranch);
    res = IndexOfTaggedIntElements(glue, elements, *target, *fromIndex, thisLen, options);
    Jump(&exit);
    Bind(&doubleBranch);
    res = IndexOfTaggedNumber(glue, elements, *target, *fromIndex, thisLen, options, false);
    Jump(&exit);
    Bind(&stringBranch);
    res = IndexOfStringElements(
        glue, elements, *target, *fromIndex, thisLen, options, StringElementsCondition::MUST_BE_STRING);
    Jump(&exit);
    Bind(&stringOrHoleBranch);
    res = IndexOfStringElements(
        glue, elements, *target, *fromIndex, thisLen, options, StringElementsCondition::MAY_BE_HOLE);
    Jump(&exit);
    Bind(&bigIntOrObjectBranch);
    res = IndexOfBigIntOrObjectElements(glue, elements, *target, *fromIndex, thisLen, options);
    Jump(&exit);
    Bind(&genericBranch);
    res = IndexOfGeneric(glue, elements, *target, *fromIndex, thisLen, options);
    Jump(&exit);

    Bind(&notFound);
    if (options.returnType == IndexOfReturnType::TAGGED_FOUND_INDEX) {
        res.WriteVariable(IntToTaggedPtr(Int32(-1)));
    } else {
        ASSERT_PRINT(options.returnType == IndexOfReturnType::TAGGED_FOUND_OR_NOT, "Tagged return type only!");
        res.WriteVariable(TaggedFalse());
    }
    Jump(&exit);

    Bind(&exit);
    ConcurrentApiScopeReadDone<JSSharedArray>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &readDoneExit);
    Bind(&readDoneExit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
