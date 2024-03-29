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
#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {
GateRef BuiltinsTypedArrayStubBuilder::IsDetachedBuffer(GateRef buffer)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    BRANCH(IsByteArray(buffer), &isByteArray, &notByteArray);
    Bind(&isByteArray);
    {
        Jump(&exit);
    }
    Bind(&notByteArray);
    {
        GateRef dataSlot = GetArrayBufferData(buffer);
        BRANCH(TaggedIsNull(dataSlot), &isNull, &exit);
        Bind(&isNull);
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

GateRef BuiltinsTypedArrayStubBuilder::GetDataPointFromBuffer(GateRef arrBuf)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), arrBuf);
    BRANCH(IsByteArray(arrBuf), &isByteArray, &notByteArray);
    Bind(&isByteArray);
    {
        result = PtrAdd(*result, IntPtr(ByteArray::DATA_OFFSET));
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

}  // namespace panda::ecmascript::kungfu
