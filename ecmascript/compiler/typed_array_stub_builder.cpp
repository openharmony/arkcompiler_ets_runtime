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

#include "ecmascript/compiler/typed_array_stub_builder.h"

#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/byte_array.h"

namespace panda::ecmascript::kungfu {
GateRef TypedArrayStubBuilder::IsDetachedBuffer(GateRef buffer)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Branch(IsByteArray(buffer), &isByteArray, &notByteArray);
    Bind(&isByteArray);
    {
        Jump(&exit);
    }
    Bind(&notByteArray);
    {
        GateRef dataSlot = GetArrayBufferData(buffer);
        Branch(TaggedIsNull(dataSlot), &isNull, &exit);
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

GateRef TypedArrayStubBuilder::GetDataPointFromBuffer(GateRef arrBuf)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), arrBuf);
    Branch(IsByteArray(arrBuf), &isByteArray, &notByteArray);
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

GateRef TypedArrayStubBuilder::FastGetPropertyByIndex(GateRef glue, GateRef array, GateRef index, GateRef jsType)
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
    Branch(IsDetachedBuffer(buffer), &isDetached, &notDetached);
    Bind(&isDetached);
    {
        Jump(&slowPath);
    }
    Bind(&notDetached);
    {
        GateRef arrLen = GetArrayLength(array);
        Branch(Int32GreaterThanOrEqual(index, arrLen), &exit, &indexIsvalid);
        Bind(&indexIsvalid);
        {
            GateRef offset = GetByteOffset(array);
            result = GetValueFromBuffer(buffer, index, offset, jsType);
            Jump(&exit);
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

GateRef TypedArrayStubBuilder::FastCopyElementToArray(GateRef glue, GateRef typedArray, GateRef array)
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
    Branch(IsDetachedBuffer(buffer), &isDetached, &notDetached);
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
            Branch(Int32UnsignedLessThan(*start, arrLen), &storeValue, &exit);
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

GateRef TypedArrayStubBuilder::GetValueFromBuffer(GateRef buffer, GateRef index, GateRef offset, GateRef jsType)
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

    Branch(Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_UINT8_CLAMPED_ARRAY))),
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
        Branch(Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_INT32_ARRAY))),
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
                Branch(condition, &overflow, &notOverflow);
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
}  // namespace panda::ecmascript::kungfu