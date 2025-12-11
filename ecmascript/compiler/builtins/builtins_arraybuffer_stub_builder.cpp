/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/compiler/builtins/builtins_arraybuffer_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
// BuiltinsArrayBuffer::GetValueFromBuffer
GateRef BuiltinsArrayBufferStubBuilder::GetValueFromBuffer(GateRef glue, GateRef pointer,
    GateRef byteIndex, GateRef isLittleEndian, DataViewType type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    // Handle INT8/UINT8 directly (byte order doesn't matter for single byte)
    switch (type) {
        case DataViewType::UINT8: {
            GateRef byte = LoadPrimitive(VariableType::INT8(), pointer, byteIndex);
            result = IntToTaggedPtr(ZExtInt8ToInt32(byte));
            Jump(&exit);
            break;
        }
        case DataViewType::INT8: {
            GateRef byte = LoadPrimitive(VariableType::INT8(), pointer, byteIndex);
            result = IntToTaggedPtr(SExtInt8ToInt32(byte));
            Jump(&exit);
            break;
        }
        case DataViewType::UINT16:
        case DataViewType::INT16:
        case DataViewType::UINT32:
        case DataViewType::INT32:
            result = GetValueFromBufferForInteger(glue, pointer, byteIndex, isLittleEndian, type);
            Jump(&exit);
            break;
        case DataViewType::FLOAT32:
        case DataViewType::FLOAT64:
            result = GetValueFromBufferForFloat(glue, pointer, byteIndex, isLittleEndian, type);
            Jump(&exit);
            break;
        default:
            LOG_ECMA(FATAL) << "Unsupported DataViewType";
            UNREACHABLE();
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// Handles INT16, UINT16, INT32, UINT32
GateRef BuiltinsArrayBufferStubBuilder::GetValueFromBufferForInteger(GateRef glue, GateRef pointer,
    GateRef offset, GateRef littleEndianHandle, DataViewType type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    uint32_t byteSize = 0;
    switch (type) {
        case DataViewType::INT16:
        case DataViewType::UINT16:
            byteSize = 2;  // 2: INT16/UINT16 size
            break;
        case DataViewType::INT32:
        case DataViewType::UINT32:
            byteSize = 4;  // 4: INT32/UINT32 size
            break;
        default:
            LOG_ECMA(FATAL) << "Invalid integer type";
            UNREACHABLE();
    }

    // Read bytes and assemble with endianness conversion
    GateRef value32 = ReadAndAssembleBytes(glue, pointer, offset, littleEndianHandle, byteSize);

    // Convert to tagged value based on type
    switch (type) {
        case DataViewType::INT16: {
            GateRef value16 = TruncInt32ToInt16(value32);
            GateRef signExtended = SExtInt16ToInt32(value16);
            result = IntToTaggedPtr(signExtended);
            Jump(&exit);
            break;
        }
        case DataViewType::UINT16: {
            GateRef value16 = TruncInt32ToInt16(value32);
            GateRef zeroExtended = ZExtInt16ToInt32(value16);
            result = IntToTaggedPtr(zeroExtended);
            Jump(&exit);
            break;
        }
        case DataViewType::INT32: {
            result = IntToTaggedPtr(value32);
            Jump(&exit);
            break;
        }
        case DataViewType::UINT32: {
            // UINT32: may overflow TaggedInt, convert to double if needed
            Label overflow(env);
            Label notOverflow(env);

            BRANCH(Int32UnsignedGreaterThan(value32, Int32(INT32_MAX)), &overflow, &notOverflow);
            Bind(&overflow);
            {
                result = DoubleToTaggedDoublePtr(ChangeUInt32ToFloat64(value32));
                Jump(&exit);
            }
            Bind(&notOverflow);
            {
                result = IntToTaggedPtr(value32);
                Jump(&exit);
            }
            break;
        }
        default:
            LOG_ECMA(FATAL) << "Unreachable";
            UNREACHABLE();
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// Handles FLOAT32, FLOAT64
GateRef BuiltinsArrayBufferStubBuilder::GetValueFromBufferForFloat(GateRef glue, GateRef pointer,
    GateRef offset, GateRef littleEndianHandle, DataViewType type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    switch (type) {
        case DataViewType::FLOAT32: {
            uint32_t byteSize = 4;  // 4: FLOAT32 size
            GateRef rawBits = ReadAndAssembleBytes(glue, pointer, offset, littleEndianHandle, byteSize);

            GateRef floatValue = CastInt32ToFloat32(rawBits);
            GateRef doubleValue = ExtFloat32ToDouble(floatValue);

            Label isNaN(env);
            Label notNaN(env);

            BRANCH_UNLIKELY(env->GetBuilder()->DoubleIsImpureNaN(doubleValue), &isNaN, &notNaN);
            Bind(&isNaN);
            {
                result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
                Jump(&exit);
            }
            Bind(&notNaN);
            {
                result = DoubleToTaggedDoublePtr(doubleValue);
                Jump(&exit);
            }
            break;
        }
        case DataViewType::FLOAT64: {
            GateRef b0 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, offset));
            GateRef b1 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(1)))); // 1
            GateRef b2 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(2)))); // 2
            GateRef b3 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(3)))); // 3
            GateRef b4 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(4)))); // 4
            GateRef b5 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(5)))); // 5
            GateRef b6 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(6)))); // 6
            GateRef b7 = ZExtInt8ToInt64(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(7)))); // 7
            Label littleEnd(env);
            Label bigEnd(env);
            Label assembleExit(env);
            DEFVARIABLE(rawBits64, VariableType::INT64(), Int64(0));

            BRANCH(TaggedIsTrue(littleEndianHandle), &littleEnd, &bigEnd);
            Bind(&littleEnd);
            {
                // Little endian: b0 | (b1 << 8) | ... | (b7 << 56)
                GateRef lower = Int64Or(Int64Or(b0, Int64LSL(b1, Int64(builtins::BITS_EIGHT))),
                                        Int64Or(Int64LSL(b2, Int64(2 * builtins::BITS_EIGHT)),  // 2: shift 16 bits
                                                Int64LSL(b3, Int64(builtins::BITS_TWENTY_FOUR))));
                GateRef higher = Int64Or(Int64Or(b4, Int64LSL(b5, Int64(builtins::BITS_EIGHT))),
                                         Int64Or(Int64LSL(b6, Int64(2 * builtins::BITS_EIGHT)),  // 2: shift 16 bits
                                                 Int64LSL(b7, Int64(builtins::BITS_TWENTY_FOUR))));
                rawBits64 = Int64Or(lower, Int64LSL(higher, Int64(32)));  // 32: shift for upper 32 bits
                Jump(&assembleExit);
            }
            Bind(&bigEnd);
            {
                // Big endian: (b0 << 56) | ... | b7
                GateRef higher = Int64Or(Int64Or(Int64LSL(b0, Int64(builtins::BITS_TWENTY_FOUR)),
                                                 Int64LSL(b1, Int64(2 * builtins::BITS_EIGHT))),  // 2: shift 16 bits
                                         Int64Or(Int64LSL(b2, Int64(builtins::BITS_EIGHT)), b3));
                GateRef lower = Int64Or(Int64Or(Int64LSL(b4, Int64(builtins::BITS_TWENTY_FOUR)),
                                                Int64LSL(b5, Int64(2 * builtins::BITS_EIGHT))),  // 2: shift 16 bits
                                        Int64Or(Int64LSL(b6, Int64(builtins::BITS_EIGHT)), b7));
                rawBits64 = Int64Or(Int64LSL(higher, Int64(32)), lower);  // 32: shift for upper 32 bits
                Jump(&assembleExit);
            }
            Bind(&assembleExit);

            // Convert raw bits to double
            GateRef doubleValue = CastInt64ToFloat64(*rawBits64);

            Label isNaN(env);
            Label notNaN(env);

            BRANCH_UNLIKELY(env->GetBuilder()->DoubleIsImpureNaN(doubleValue), &isNaN, &notNaN);
            Bind(&isNaN);
            {
                result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
                Jump(&exit);
            }
            Bind(&notNaN);
            {
                result = DoubleToTaggedDoublePtr(doubleValue);
                Jump(&exit);
            }
            break;
        }
        default:
            LOG_ECMA(FATAL) << "Invalid float type";
            UNREACHABLE();
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsArrayBufferStubBuilder::ReadAndAssembleBytes(GateRef glue, GateRef pointer,
    GateRef offset, GateRef littleEndianHandle, uint32_t byteSize)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label littleEnd(env);
    Label bigEnd(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    if (byteSize == 2) {  // 2: INT16/UINT16
        // Read 2 bytes
        GateRef b0 = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), pointer, offset));
        GateRef b1 = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(1)))); // 1

        BRANCH(TaggedIsTrue(littleEndianHandle), &littleEnd, &bigEnd);
        Bind(&littleEnd);
        {
            // Little endian: b0 | (b1 << 8) - corresponds to C++ line 469-470
            result = Int32Or(b0, Int32LSL(b1, Int32(builtins::BITS_EIGHT)));
            Jump(&exit);
        }
        Bind(&bigEnd);
        {
            // Big endian: (b0 << 8) | b1 - corresponds to C++ line 469-470 (reversed)
            result = Int32Or(Int32LSL(b0, Int32(builtins::BITS_EIGHT)), b1);
            Jump(&exit);
        }
    } else if (byteSize == 4) {  // 4: INT32/UINT32/FLOAT32
        // Read 4 bytes
        GateRef b0 = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), pointer, offset));
        GateRef b1 = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(1)))); // 1
        GateRef b2 = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(2)))); // 2
        GateRef b3 = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), pointer, Int32Add(offset, Int32(3)))); // 3

        BRANCH(TaggedIsTrue(littleEndianHandle), &littleEnd, &bigEnd);
        Bind(&littleEnd);
        {
            // Little endian: b0 | (b1 << 8) | (b2 << 16) | (b3 << 24) - corresponds to C++ line 473-476
            result = Int32Or(Int32Or(b0, Int32LSL(b1, Int32(builtins::BITS_EIGHT))),
                             Int32Or(Int32LSL(b2, Int32(2 * builtins::BITS_EIGHT)),  // 2: shift 16 bits
                                     Int32LSL(b3, Int32(builtins::BITS_TWENTY_FOUR))));
            Jump(&exit);
        }
        Bind(&bigEnd);
        {
            // Big endian: (b0 << 24) | (b1 << 16) | (b2 << 8) | b3 - corresponds to C++ line 473-476 (reversed)
            result = Int32Or(Int32Or(Int32LSL(b0, Int32(builtins::BITS_TWENTY_FOUR)),
                                     Int32LSL(b1, Int32(2 * builtins::BITS_EIGHT))),  // 2: shift 16 bits
                             Int32Or(Int32LSL(b2, Int32(builtins::BITS_EIGHT)), b3));
            Jump(&exit);
        }
    } else {
        LOG_ECMA(FATAL) << "Invalid byte size";
        UNREACHABLE();
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// BuiltinsArrayBuffer::GetDataPointFromBuffer
GateRef BuiltinsArrayBufferStubBuilder::GetDataPointFromBuffer(GateRef glue, GateRef arrBuf)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label isNull(env);
    Label exit(env);
    Label isByteArray(env);
    Label notByteArray(env);
    DEFVARIABLE(result, VariableType::NATIVE_POINTER(), IntPtr(0));
    BRANCH(IsByteArray(glue, arrBuf), &isByteArray, &notByteArray);
    Bind(&isByteArray);
    {
        result = ChangeTaggedPointerToInt64(PtrAdd(arrBuf, IntPtr(ByteArray::DATA_OFFSET)));
        Jump(&exit);
    }
    Bind(&notByteArray);
    {
        GateRef data = GetArrayBufferData(glue, arrBuf);
        result = GetExternalPointer(data);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
