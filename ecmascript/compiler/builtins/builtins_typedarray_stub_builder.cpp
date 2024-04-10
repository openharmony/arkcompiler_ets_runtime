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

#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"

#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/byte_array.h"
#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {
GateRef BuiltinsTypedArrayStubBuilder::GetDataPointFromBuffer(GateRef arrBuf)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::NATIVE_POINTER(), IntPtr(0));
    BRANCH(IsByteArray(arrBuf), &isByteArray, &notByteArray);
    Bind(&isByteArray);
    {
        result = ChangeByteArrayTaggedPointerToInt64(PtrAdd(arrBuf, IntPtr(ByteArray::DATA_OFFSET)));
        Jump(&exit);
    }
    Bind(&notByteArray);
    {
        GateRef data = GetArrayBufferData(arrBuf);
        result = GetExternalPointer(data);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::CheckTypedArrayIndexInRange(GateRef array, GateRef index)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label exit(env);
    Label indexIsvalid(env);
    Label indexNotLessZero(env);
    BRANCH(Int64LessThan(index, Int64(0)), &exit, &indexNotLessZero);
    Bind(&indexNotLessZero);
    {
        GateRef arrLen = GetArrayLength(array);
        BRANCH(Int64GreaterThanOrEqual(index, ZExtInt32ToInt64(arrLen)), &exit, &indexIsvalid);
        Bind(&indexIsvalid);
        {
            result = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::LoadTypedArrayElement(GateRef glue, GateRef array, GateRef key, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label notDetached(env);
    Label indexIsvalid(env);
    Label slowPath(env);
    GateRef buffer = GetViewedArrayBuffer(array);
    BRANCH(IsDetachedBuffer(buffer), &exit, &notDetached);
    Bind(&notDetached);
    {
        GateRef index = TryToElementsIndex(glue, key);
        BRANCH(CheckTypedArrayIndexInRange(array, index), &indexIsvalid, &exit);
        Bind(&indexIsvalid);
        {
            GateRef offset = GetByteOffset(array);
            result = GetValueFromBuffer(buffer, TruncInt64ToInt32(index), offset, jsType);
            BRANCH(TaggedIsNumber(*result), &exit, &slowPath);
        }
        Bind(&slowPath);
        {
            result = CallRuntime(glue, RTSTUB_ID(GetTypeArrayPropertyByIndex),
                { array, IntToTaggedInt(index), IntToTaggedInt(jsType) });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::StoreTypedArrayElement(GateRef glue, GateRef array, GateRef index, GateRef value,
                                                              GateRef jsType)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);
    Label notDetached(env);
    Label indexIsvalid(env);
    GateRef buffer = GetViewedArrayBuffer(array);
    BRANCH(IsDetachedBuffer(buffer), &exit, &notDetached);
    Bind(&notDetached);
    {
        BRANCH(CheckTypedArrayIndexInRange(array, index), &indexIsvalid, &exit);
        Bind(&indexIsvalid);
        {
            result = CallRuntime(glue, RTSTUB_ID(SetTypeArrayPropertyByIndex),
                { array, IntToTaggedInt(index), value, IntToTaggedInt(jsType) });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::FastGetPropertyByIndex(GateRef glue, GateRef array,
                                                              GateRef index, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label isDetached(env);
    Label notDetached(env);
    Label slowPath(env);
    Label indexIsvalid(env);
    
    GateRef buffer = GetViewedArrayBuffer(array);
    BRANCH(IsDetachedBuffer(buffer), &isDetached, &notDetached);
    Bind(&isDetached);
    {
        Jump(&slowPath);
    }
    Bind(&notDetached);
    {
        GateRef arrLen = GetArrayLength(array);
        BRANCH(Int32GreaterThanOrEqual(index, arrLen), &exit, &indexIsvalid);
        Bind(&indexIsvalid);
        {
            GateRef offset = GetByteOffset(array);
            result = GetValueFromBuffer(buffer, index, offset, jsType);
            BRANCH(TaggedIsNumber(*result), &exit, &slowPath);
        }
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(GetTypeArrayPropertyByIndex),
            { array, IntToTaggedInt(index), IntToTaggedInt(jsType)});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::FastCopyElementToArray(GateRef glue, GateRef typedArray, GateRef array)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    Label exit(env);
    Label isDetached(env);
    Label notDetached(env);
    Label slowPath(env);
    Label begin(env);
    Label storeValue(env);
    Label endLoop(env);

    GateRef buffer = GetViewedArrayBuffer(typedArray);
    BRANCH(IsDetachedBuffer(buffer), &isDetached, &notDetached);
    Bind(&isDetached);
    {
        result = False();
        Jump(&slowPath);
    }
    Bind(&notDetached);
    {
        GateRef arrLen = GetArrayLength(typedArray);
        GateRef offset = GetByteOffset(typedArray);
        GateRef hclass = LoadHClass(typedArray);
        GateRef jsType = GetObjectType(hclass);

        Jump(&begin);
        LoopBegin(&begin);
        {
            BRANCH(Int32UnsignedLessThan(*start, arrLen), &storeValue, &exit);
            Bind(&storeValue);
            {
                GateRef value = GetValueFromBuffer(buffer, *start, offset, jsType);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, *start, value);
                start = Int32Add(*start, Int32(1));
                Jump(&endLoop);
            }
            Bind(&endLoop);
            LoopEnd(&begin);
        }
    }
    Bind(&slowPath);
    {
        CallRuntime(glue, RTSTUB_ID(FastCopyElementToArray), { typedArray, array});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::GetValueFromBuffer(GateRef buffer, GateRef index, GateRef offset, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label defaultLabel(env);
    Label isInt8(env);
    Label notInt8(env);
    Label isInt16(env);
    Label notInt16(env);

    Label labelBuffer[3] = { Label(env), Label(env), Label(env) };
    Label labelBuffer1[3] = { Label(env), Label(env), Label(env) };
    Label labelBuffer2[3] = { Label(env), Label(env), Label(env) };
    int64_t valueBuffer[3] = {
        static_cast<int64_t>(JSType::JS_INT8_ARRAY), static_cast<int64_t>(JSType::JS_UINT8_ARRAY),
        static_cast<int64_t>(JSType::JS_UINT8_CLAMPED_ARRAY) };
    int64_t valueBuffer1[3] = {
        static_cast<int64_t>(JSType::JS_INT16_ARRAY), static_cast<int64_t>(JSType::JS_UINT16_ARRAY),
        static_cast<int64_t>(JSType::JS_INT32_ARRAY) };
    int64_t valueBuffer2[3] = {
        static_cast<int64_t>(JSType::JS_UINT32_ARRAY), static_cast<int64_t>(JSType::JS_FLOAT32_ARRAY),
        static_cast<int64_t>(JSType::JS_FLOAT64_ARRAY) };

    BRANCH(Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_UINT8_CLAMPED_ARRAY))),
        &isInt8, &notInt8);
    Bind(&isInt8);
    {
        // 3 : this switch has 3 case
        Switch(jsType, &defaultLabel, valueBuffer, labelBuffer, 3);
        Bind(&labelBuffer[0]);
        {
            GateRef byteIndex = Int32Add(index, offset);
            GateRef block = GetDataPointFromBuffer(buffer);
            GateRef re = Load(VariableType::INT8(), block, byteIndex);
            result = IntToTaggedPtr(SExtInt8ToInt32(re));
            Jump(&exit);
        }

        Bind(&labelBuffer[1]);
        {
            GateRef byteIndex = Int32Add(index, offset);
            GateRef block = GetDataPointFromBuffer(buffer);
            GateRef re = Load(VariableType::INT8(), block, byteIndex);
            result = IntToTaggedPtr(ZExtInt8ToInt32(re));
            Jump(&exit);
        }
        // 2 : index of this buffer
        Bind(&labelBuffer[2]);
        {
            GateRef byteIndex = Int32Add(index, offset);
            GateRef block = GetDataPointFromBuffer(buffer);
            GateRef re = Load(VariableType::INT8(), block, byteIndex);
            result = IntToTaggedPtr(ZExtInt8ToInt32(re));
            Jump(&exit);
        }
    }

    Bind(&notInt8);
    {
        BRANCH(Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_INT32_ARRAY))),
            &isInt16, &notInt16);
        Bind(&isInt16);
        {
            // 3 : this switch has 3 case
            Switch(jsType, &defaultLabel, valueBuffer1, labelBuffer1, 3);
            Bind(&labelBuffer1[0]);
            {
                GateRef byteIndex = Int32Add(Int32Mul(index, Int32(base::ElementSize::TWO)), offset);
                GateRef block = GetDataPointFromBuffer(buffer);
                GateRef re = Load(VariableType::INT16(), block, byteIndex);
                result = IntToTaggedPtr(SExtInt16ToInt32(re));
                Jump(&exit);
            }

            Bind(&labelBuffer1[1]);
            {
                GateRef byteIndex = Int32Add(Int32Mul(index, Int32(base::ElementSize::TWO)), offset);
                GateRef block = GetDataPointFromBuffer(buffer);
                GateRef re = Load(VariableType::INT16(), block, byteIndex);
                result = IntToTaggedPtr(ZExtInt16ToInt32(re));
                Jump(&exit);
            }
            // 2 : index of this buffer
            Bind(&labelBuffer1[2]);
            {
                GateRef byteIndex = Int32Add(Int32Mul(index, Int32(base::ElementSize::FOUR)), offset);
                GateRef block = GetDataPointFromBuffer(buffer);
                GateRef re = Load(VariableType::INT32(), block, byteIndex);
                result = IntToTaggedPtr(re);
                Jump(&exit);
            }
        }
        Bind(&notInt16);
        {
            // 3 : this switch has 3 case
            Switch(jsType, &defaultLabel, valueBuffer2, labelBuffer2, 3);
            Bind(&labelBuffer2[0]);
            {
                Label overflow(env);
                Label notOverflow(env);
                GateRef byteIndex = Int32Add(Int32Mul(index, Int32(base::ElementSize::FOUR)), offset);
                GateRef block = GetDataPointFromBuffer(buffer);
                GateRef re = Load(VariableType::INT32(), block, byteIndex);

                auto condition = Int32UnsignedGreaterThan(re, Int32(INT32_MAX));
                BRANCH(condition, &overflow, &notOverflow);
                Bind(&overflow);
                {
                    result = DoubleToTaggedDoublePtr(ChangeUInt32ToFloat64(re));
                    Jump(&exit);
                }
                Bind(&notOverflow);
                {
                    result = IntToTaggedPtr(re);
                    Jump(&exit);
                }
            }
            Bind(&labelBuffer2[1]);
            {
                GateRef byteIndex = Int32Add(Int32Mul(index, Int32(base::ElementSize::FOUR)), offset);
                GateRef block = GetDataPointFromBuffer(buffer);
                GateRef re = Load(VariableType::INT32(), block, byteIndex);
                result = DoubleToTaggedDoublePtr(ExtFloat32ToDouble(CastInt32ToFloat32(re)));
                Jump(&exit);
            }
            // 2 : index of this buffer
            Bind(&labelBuffer2[2]);
            {
                GateRef byteIndex = Int32Add(Int32Mul(index, Int32(base::ElementSize::EIGHT)), offset);
                GateRef block = GetDataPointFromBuffer(buffer);
                GateRef re = Load(VariableType::INT64(), block, byteIndex);
                result = DoubleToTaggedDoublePtr(CastInt64ToFloat64(re));
                Jump(&exit);
            }
        }
    }

    Bind(&defaultLabel);
    {
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsTypedArrayStubBuilder::CalculatePositionWithLength(GateRef position, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT64(), Int64(0));
    Label positionLessThanZero(env);
    Label positionNotLessThanZero(env);
    Label resultNotGreaterThanZero(env);
    Label positionLessThanLength(env);
    Label positionNotLessThanLength(env);
    Label afterCalculatePosition(env);

    BRANCH(Int64LessThan(position, Int64(0)), &positionLessThanZero, &positionNotLessThanZero);
    Bind(&positionLessThanZero);
    {
        result = Int64Add(position, length);
        BRANCH(Int64GreaterThan(*result, Int64(0)), &afterCalculatePosition, &resultNotGreaterThanZero);
        Bind(&resultNotGreaterThanZero);
        {
            result = Int64(0);
            Jump(&afterCalculatePosition);
        }
    }
    Bind(&positionNotLessThanZero);
    {
        BRANCH(Int64LessThan(position, length), &positionLessThanLength, &positionNotLessThanLength);
        Bind(&positionLessThanLength);
        {
            result = position;
            Jump(&afterCalculatePosition);
        }
        Bind(&positionNotLessThanLength);
        {
            result = length;
            Jump(&afterCalculatePosition);
        }
    }
    Bind(&afterCalculatePosition);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsTypedArrayStubBuilder::Some(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label ecmaObj(env);
    Label typedArray(env);
    Label thisExists(env);
    BRANCH(IsEcmaObject(thisValue), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    BRANCH(IsTypedArray(thisValue), &typedArray, slowPath);
    Bind(&typedArray);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);

    Label arg0HeapObject(env);
    Label callable(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    BRANCH(IsCallable(callbackFnHandle), &callable, slowPath);
    Bind(&callable);
    {
        GateRef argHandle = GetCallArg1(numArgs);
        DEFVARIABLE(i, VariableType::INT64(), Int64(0));
        DEFVARIABLE(thisArrLen, VariableType::INT64(), ZExtInt32ToInt64(GetArrayLength(thisValue)));
        DEFVARIABLE(kValue, VariableType::JS_ANY(), Hole());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Label hasException0(env);
            Label notHasException0(env);
            Label callDispatch(env);
            Label hasException1(env);
            Label notHasException1(env);
            BRANCH(Int64LessThan(*i, *thisArrLen), &next, &loopExit);
            Bind(&next);
            kValue = FastGetPropertyByIndex(glue, thisValue, TruncInt64ToInt32(*i),
                GetObjectType(LoadHClass(thisValue)));
            BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
            Bind(&hasException0);
            {
                result->WriteVariable(Exception());
                Jump(exit);
            }
            Bind(&notHasException0);
            {
                BRANCH(TaggedIsHole(*kValue), &loopEnd, &callDispatch);
            }
            Bind(&callDispatch);
            {
                GateRef key = Int64ToTaggedInt(*i);
                GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS),
                    0, Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                    { argHandle, *kValue, key, thisValue });
                BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                Bind(&hasException1);
                {
                    result->WriteVariable(Exception());
                    Jump(exit);
                }
                Bind(&notHasException1);
                {
                    Label retValueIsTrue(env);
                    BRANCH(TaggedIsTrue(FastToBoolean(retValue)), &retValueIsTrue, &loopEnd);
                    Bind(&retValueIsTrue);
                    {
                        result->WriteVariable(TaggedTrue());
                        Jump(exit);
                    }
                }
            }
        }
        Bind(&loopEnd);
        i = Int64Add(*i, Int64(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        result->WriteVariable(TaggedFalse());
        Jump(exit);
    }
}

void BuiltinsTypedArrayStubBuilder::Filter(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isEcmaObject(env);
    Label isTypedArray(env);
    Label isFastTypedArray(env);
    Label defaultConstr(env);
    Label prototypeIsEcmaObj(env);
    Label isProtoChangeMarker(env);
    Label accessorNotChanged(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(IsEcmaObject(thisValue), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    BRANCH(IsTypedArray(thisValue), &isTypedArray, slowPath);
    Bind(&isTypedArray);
    GateRef arrayType = GetObjectType(LoadHClass(thisValue));
    BRANCH(IsFastTypeArray(arrayType), &isFastTypedArray, slowPath);
    Bind(&isFastTypedArray);
    BRANCH(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);
    GateRef prototype = GetPrototypeFromHClass(LoadHClass(thisValue));
    BRANCH(IsEcmaObject(prototype), &prototypeIsEcmaObj, slowPath);
    Bind(&prototypeIsEcmaObj);
    GateRef marker = GetProtoChangeMarkerFromHClass(LoadHClass(prototype));
    Branch(TaggedIsProtoChangeMarker(marker), &isProtoChangeMarker, slowPath);
    Bind(&isProtoChangeMarker);
    Branch(GetAccessorHasChanged(marker), slowPath, &accessorNotChanged);
    Bind(&accessorNotChanged);

    Label arg0HeapObject(env);
    Label callable(env);
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &arg0HeapObject, slowPath);
    Bind(&arg0HeapObject);
    {
        BRANCH(IsCallable(callbackFnHandle), &callable, slowPath);
        Bind(&callable);
        {
            GateRef argHandle = GetCallArg1(numArgs);
            GateRef thisLen = GetArrayLength(thisValue);
            BuiltinsArrayStubBuilder arrayStubBuilder(this);
            GateRef kept = arrayStubBuilder.NewArray(glue, ZExtInt32ToInt64(thisLen));
            DEFVARIABLE(i, VariableType::INT32(), Int32(0));
            DEFVARIABLE(newArrayLen, VariableType::INT32(), Int32(0));
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Label hasException0(env);
                Label notHasException0(env);
                Label hasException1(env);
                Label notHasException1(env);
                Label retValueIsTrue(env);
                BRANCH(Int32LessThan(*i, thisLen), &next, &loopExit);
                Bind(&next);
                {
                    GateRef kValue = FastGetPropertyByIndex(glue, thisValue, *i, arrayType);
                    BRANCH(HasPendingException(glue), &hasException0, &notHasException0);
                    Bind(&hasException0);
                    {
                        result->WriteVariable(Exception());
                        Jump(exit);
                    }
                    Bind(&notHasException0);
                    {
                        GateRef key = IntToTaggedInt(*i);
                        GateRef retValue = JSCallDispatch(glue, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS),
                            0, Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN,
                            { argHandle, kValue, key, thisValue });
                        BRANCH(HasPendingException(glue), &hasException1, &notHasException1);
                        Bind(&hasException1);
                        {
                            result->WriteVariable(Exception());
                            Jump(exit);
                        }
                        Bind(&notHasException1);
                        {
                            BRANCH(TaggedIsTrue(FastToBoolean(retValue)), &retValueIsTrue, &loopEnd);
                            Bind(&retValueIsTrue);
                            {
                                arrayStubBuilder.SetValueWithElementsKind(glue, kept, kValue, *newArrayLen,
                                    Boolean(true), Int32(static_cast<uint32_t>(ElementsKind::NONE)));
                                newArrayLen = Int32Add(*newArrayLen, Int32(1));
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
            }
            Bind(&loopEnd);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&loopHead);
            Bind(&loopExit);

            NewObjectStubBuilder newBuilder(this);
            newBuilder.SetParameters(glue, 0);
            GateRef newArray = newBuilder.NewTypedArray(glue, thisValue, arrayType, TruncInt64ToInt32(*newArrayLen));
            i = Int32(0);
            Label loopHead2(env);
            Label loopEnd2(env);
            Label next2(env);
            Label loopExit2(env);
            Jump(&loopHead2);
            LoopBegin(&loopHead2);
            {
                BRANCH(Int32LessThan(*i, *newArrayLen), &next2, &loopExit2);
                Bind(&next2);
                {
                    GateRef kValue = arrayStubBuilder.GetTaggedValueWithElementsKind(kept, *i);
                    StoreTypedArrayElement(glue, newArray, ZExtInt32ToInt64(*i), kValue, arrayType);
                    Jump(&loopEnd2);
                }
            }
            Bind(&loopEnd2);
            i = Int32Add(*i, Int32(1));
            LoopEnd(&loopHead2);
            Bind(&loopExit2);

            result->WriteVariable(newArray);
            Jump(exit);
        }
    }
}

void BuiltinsTypedArrayStubBuilder::Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label isEcmaObject(env);
    Label isTypedArray(env);
    Label isFastTypedArray(env);
    Label defaultConstr(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(IsEcmaObject(thisValue), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    BRANCH(IsTypedArray(thisValue), &isTypedArray, slowPath);
    Bind(&isTypedArray);
    GateRef arrayType = GetObjectType(LoadHClass(thisValue));
    BRANCH(IsFastTypeArray(arrayType), &isFastTypedArray, slowPath);
    Bind(&isFastTypedArray);
    BRANCH(HasConstructor(thisValue), slowPath, &defaultConstr);
    Bind(&defaultConstr);

    DEFVARIABLE(startPos, VariableType::INT64(), Int64(0));
    DEFVARIABLE(endPos, VariableType::INT64(), Int64(0));
    DEFVARIABLE(newArrayLen, VariableType::INT64(), Int64(0));
    Label startTagExists(env);
    Label startTagIsInt(env);
    Label afterCallArg(env);
    Label endTagExists(env);
    Label endTagIsInt(env);
    Label adjustArrLen(env);
    Label newTypedArray(env);
    Label writeVariable(env);
    Label copyBuffer(env);
    GateRef thisLen = ZExtInt32ToInt64(GetArrayLength(thisValue));
    BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), slowPath, &startTagExists);
    Bind(&startTagExists);
    {
        GateRef startTag = GetCallArg0(numArgs);
        BRANCH(TaggedIsInt(startTag), &startTagIsInt, slowPath);
        Bind(&startTagIsInt);
        {
            startPos = SExtInt32ToInt64(TaggedGetInt(startTag));
            endPos = thisLen;
            BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &afterCallArg, &endTagExists);
            Bind(&endTagExists);
            {
                GateRef endTag = GetCallArg1(numArgs);
                BRANCH(TaggedIsInt(endTag), &endTagIsInt, slowPath);
                Bind(&endTagIsInt);
                {
                    endPos = SExtInt32ToInt64(TaggedGetInt(endTag));
                    Jump(&afterCallArg);
                }
            }
            Bind(&afterCallArg);
            {
                startPos = CalculatePositionWithLength(*startPos, thisLen);
                endPos = CalculatePositionWithLength(*endPos, thisLen);
                BRANCH(Int64GreaterThan(*endPos, *startPos), &adjustArrLen, &newTypedArray);
                Bind(&adjustArrLen);
                {
                    newArrayLen = Int64Sub(*endPos, *startPos);
                    Jump(&newTypedArray);
                }
            }
        }
    }
    Bind(&newTypedArray);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        GateRef newArray = newBuilder.NewTypedArray(glue, thisValue, arrayType, TruncInt64ToInt32(*newArrayLen));
        BRANCH(Int32Equal(TruncInt64ToInt32(*newArrayLen), Int32(0)), &writeVariable, &copyBuffer);
        Bind(&copyBuffer);
        {
            CallNGCRuntime(glue, RTSTUB_ID(CopyTypedArrayBuffer), { thisValue, newArray, TruncInt64ToInt32(*startPos),
                Int32(0), TruncInt64ToInt32(*newArrayLen), newBuilder.GetElementSizeFromType(glue, arrayType) });
            Jump(&writeVariable);
        }
        Bind(&writeVariable);
        {
            result->WriteVariable(newArray);
            Jump(exit);
        }
    }
}

void BuiltinsTypedArrayStubBuilder::SubArray(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label ecmaObj(env);
    Label typedArray(env);
    Label isNotZero(env);
    DEFVARIABLE(beginIndex, VariableType::INT32(), Int32(0));
    DEFVARIABLE(endIndex, VariableType::INT32(), Int32(0));
    DEFVARIABLE(newLength, VariableType::INT32(), Int32(0));

    BRANCH(IsEcmaObject(thisValue), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    BRANCH(IsTypedArray(thisValue), &typedArray, slowPath);
    Bind(&typedArray);

    GateRef objHclass = LoadHClass(thisValue);
    Label defaultConstructor(env);
    BRANCH(HasConstructorByHClass(objHclass), slowPath, &defaultConstructor);
    Bind(&defaultConstructor);
    GateRef arrayLen = GetArrayLength(thisValue);
    GateRef buffer = GetViewedArrayBuffer(thisValue);
    Label offHeap(env);
    BRANCH(BoolOr(IsJSObjectType(buffer, JSType::JS_ARRAY_BUFFER),
        IsJSObjectType(buffer, JSType::JS_SHARED_ARRAY_BUFFER)), &offHeap, slowPath);
    Bind(&offHeap);
    Label notDetached(env);
    BRANCH(IsDetachedBuffer(buffer), slowPath, &notDetached);
    Bind(&notDetached);

    Label intIndex(env);
    GateRef relativeBegin = GetCallArg0(numArgs);
    GateRef end = GetCallArg1(numArgs);
    BRANCH(TaggedIsInt(relativeBegin), &intIndex, slowPath);
    Bind(&intIndex);
    GateRef relativeBeginInt = GetInt32OfTInt(relativeBegin);
    beginIndex = CalArrayRelativePos(relativeBeginInt, arrayLen);

    Label undefEnd(env);
    Label defEnd(env);
    Label calNewLength(env);
    Label newArray(env);
    BRANCH(TaggedIsUndefined(end), &undefEnd, &defEnd);
    Bind(&undefEnd);
    {
        endIndex = arrayLen;
        Jump(&calNewLength);
    }
    Bind(&defEnd);
    {
        Label intEnd(env);
        BRANCH(TaggedIsInt(end), &intEnd, slowPath);
        Bind(&intEnd);
        {
            GateRef endVal = GetInt32OfTInt(end);
            endIndex = CalArrayRelativePos(endVal, arrayLen);
            Jump(&calNewLength);
        }
    }
    Bind(&calNewLength);
    {
        GateRef diffLen = Int32Sub(*endIndex, *beginIndex);
        Label diffLargeZero(env);
        BRANCH(Int32GreaterThan(diffLen, Int32(0)), &diffLargeZero, &newArray);
        Bind(&diffLargeZero);
        {
            newLength = diffLen;
            Jump(&newArray);
        }
    }
    Bind(&newArray);
    GateRef oldByteLength = Load(VariableType::INT32(), thisValue, IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET));
    BRANCH(Int32Equal(arrayLen, Int32(0)), slowPath, &isNotZero);
    Bind(&isNotZero);
    GateRef elementSize = Int32Div(oldByteLength, arrayLen);
    NewObjectStubBuilder newBuilder(this);
    *result = newBuilder.NewTaggedSubArray(glue, thisValue, elementSize, *newLength, *beginIndex, objHclass, buffer);
    Jump(exit);
}

void BuiltinsTypedArrayStubBuilder::GetByteLength([[maybe_unused]] GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label ecmaObj(env);
    Label typedArray(env);
    Label Detached(env);
    Label notDetached(env);
    BRANCH(IsEcmaObject(thisValue), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    BRANCH(IsTypedArray(thisValue), &typedArray, slowPath);
    Bind(&typedArray);
    GateRef buffer = GetViewedArrayBuffer(thisValue);
    BRANCH(IsDetachedBuffer(buffer), &Detached, &notDetached);
    Bind(&Detached);
    {
        *result = IntToTaggedPtr(Int32(0));
        Jump(exit);
    }
    Bind(&notDetached);
    {
        *result = IntToTaggedPtr(GetArrayLength(thisValue));
    }
    Jump(exit);
}

void BuiltinsTypedArrayStubBuilder::GetByteOffset([[maybe_unused]] GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label ecmaObj(env);
    Label typedArray(env);
    Label Detached(env);
    Label notDetached(env);
    BRANCH(IsEcmaObject(thisValue), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    BRANCH(IsTypedArray(thisValue), &typedArray, slowPath);
    Bind(&typedArray);
    GateRef buffer = GetViewedArrayBuffer(thisValue);
    BRANCH(IsDetachedBuffer(buffer), &Detached, &notDetached);
    Bind(&Detached);
    {
        *result = IntToTaggedPtr(Int32(0));
        Jump(exit);
    }
    Bind(&notDetached);
    {
        *result = IntToTaggedPtr(GetByteOffset(thisValue));
    }
    Jump(exit);
}

void BuiltinsTypedArrayStubBuilder::Set(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisExists(env);
    Label ecmaObj(env);
    Label typedArray(env);
    Label typedArrayIsFast(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    BRANCH(IsEcmaObject(thisValue), &ecmaObj, slowPath);
    Bind(&ecmaObj);
    BRANCH(IsTypedArray(thisValue), &typedArray, slowPath);
    Bind(&typedArray);
    BRANCH(IsFastTypeArray(GetObjectType(LoadHClass(thisValue))), &typedArrayIsFast, slowPath);
    Bind(&typedArrayIsFast);
    GateRef len = ZExtInt32ToInt64(GetArrayLength(thisValue));
    GateRef arrayType = GetObjectType(LoadHClass(thisValue));
    Label notEmptyArray(env);
    BRANCH(Int64Equal(len, Int64(0)), slowPath, &notEmptyArray);
    Bind(&notEmptyArray);
    DEFVARIABLE(realOffset, VariableType::INT64(), Int64(0));
    DEFVARIABLE(startIndex, VariableType::INT64(), Int64(0));
    Label hasOffset(env);
    Label hasNoOffset(env);
    Label numArgsIsInt(env);
    GateRef fromOffset = GetCallArg1(numArgs);
    BRANCH(TaggedIsUndefinedOrNull(fromOffset), &hasNoOffset, &hasOffset);
    Bind(&hasOffset);
    {
        Label taggedIsInt(env);
        BRANCH(TaggedIsInt(fromOffset), &taggedIsInt, slowPath);
        Bind(&taggedIsInt);
        GateRef fromIndexToTagged = SExtInt32ToInt64(TaggedGetInt(fromOffset));
        Label offsetNotLessZero(env);
        BRANCH(Int64LessThan(fromIndexToTagged, Int64(0)), slowPath, &offsetNotLessZero);
        Bind(&offsetNotLessZero);
        {
            realOffset = fromIndexToTagged;
            Jump(&hasNoOffset);
        }
    }
    Bind(&hasNoOffset);

    Label srcArrayIsEcmaObj(env);
    Label srcArrayIsTypedArray(env);
    Label srcArrayIsJsArray(env);
    GateRef srcArray = GetCallArg0(numArgs);
    BRANCH(IsEcmaObject(srcArray), &srcArrayIsEcmaObj, slowPath);
    Bind(&srcArrayIsEcmaObj);
    BRANCH(IsTypedArray(srcArray), &srcArrayIsTypedArray, slowPath);
    Bind(&srcArrayIsTypedArray);
    {
        GateRef srcType = GetObjectType(LoadHClass(srcArray));
        Label isFastTypeArray(env);
        BRANCH(IsFastTypeArray(srcType), &isFastTypeArray, slowPath);
        Bind(&isFastTypeArray);
        Label isNotSameValue(env);
        GateRef targetBuffer = GetViewedArrayBuffer(thisValue);
        GateRef srcBuffer = GetViewedArrayBuffer(srcArray);
        BRANCH(SameValue(glue, targetBuffer, srcBuffer), slowPath, &isNotSameValue);
        Bind(&isNotSameValue);
        Label isNotGreaterThanLen(env);
        GateRef srcLen = ZExtInt32ToInt64(GetArrayLength(srcArray));
        BRANCH(Int64GreaterThan(Int64Add(*realOffset, srcLen), len), slowPath, &isNotGreaterThanLen);
        Bind(&isNotGreaterThanLen);
        {
            Label isSameType(env);
            Label isNotSameType(env);
            BRANCH(Equal(srcType, arrayType), &isSameType, &isNotSameType);
            Bind(&isSameType);
            {
                NewObjectStubBuilder newBuilder(this);
                CallNGCRuntime(glue, RTSTUB_ID(CopyTypedArrayBuffer),
                    { srcArray, thisValue, Int32(0), TruncInt64ToInt32(*realOffset), TruncInt64ToInt32(srcLen),
                    newBuilder.GetElementSizeFromType(glue, arrayType) });
                Jump(exit);
            }
            Bind(&isNotSameType);
            {
                Label loopHead(env);
                Label loopEnd(env);
                Label loopExit(env);
                Label loopNext(env);
                Jump(&loopHead);
                LoopBegin(&loopHead);
                {
                    BRANCH(Int64LessThan(*startIndex, srcLen), &loopNext, &loopExit);
                    Bind(&loopNext);
                    {
                        GateRef srcValue = FastGetPropertyByIndex(glue, srcArray, TruncInt64ToInt32(*startIndex),
                            GetObjectType(LoadHClass(srcArray)));
                        StoreTypedArrayElement(glue, thisValue, *realOffset, srcValue,
                            GetObjectType(LoadHClass(thisValue)));
                        Jump(&loopEnd);
                    }
                }
                Bind(&loopEnd);
                startIndex = Int64Add(*startIndex, Int64(1));
                realOffset = Int64Add(*realOffset, Int64(1));
                LoopEnd(&loopHead);
                Bind(&loopExit);
                result->WriteVariable(Undefined());
                Jump(exit);
            }
        }
    }
}
}  // namespace panda::ecmascript::kungfu
