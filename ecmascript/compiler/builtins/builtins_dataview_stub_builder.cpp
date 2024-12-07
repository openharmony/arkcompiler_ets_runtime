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

#include "ecmascript/compiler/builtins/builtins_dataview_stub_builder.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
template <DataViewType type>
void BuiltinsDataViewStubBuilder::SetTypedValue(GateRef glue, GateRef thisValue,
    GateRef numArgs, [[maybe_unused]] Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisIsHeapObject(env);
    Label thisIsDataView(env);
    Label indexIsInt(env);
    BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObject, slowPath);
    Bind(&thisIsHeapObject);
    BRANCH(IsDataView(thisValue), &thisIsDataView, slowPath);
    Bind(&thisIsDataView);
    GateRef indexTagged = GetCallArg0(numArgs);
    GateRef value = GetCallArg1(numArgs);
    BRANCH(TaggedIsInt(indexTagged), &indexIsInt, slowPath);
    Bind(&indexIsInt);
    {
        DEFVARIABLE(isLittleEndian, VariableType::JS_ANY(), TaggedFalse());
        Label indexIsValid(env);
        Label valueIsValid(env);
        Label checkOffset(env);
        Label getArrayBuffer(env);
        Label setValue(env);
        Label toBool(env);
        GateRef index = GetInt32OfTInt(indexTagged);
        BRANCH(Int32LessThan(index, Int32(0)), slowPath, &indexIsValid);
        Bind(&indexIsValid);
        {
            BRANCH(TaggedIsNumber(value), &valueIsValid, slowPath);
            Bind(&valueIsValid);
            GateRef littleEndianHandle; // need to init
            if constexpr (type == DataViewType::UINT8 || type == DataViewType::INT8) {
                littleEndianHandle = TaggedTrue();
            } else {
                littleEndianHandle = GetCallArg2(numArgs);
            }
            BRANCH(TaggedIsUndefined(littleEndianHandle), &getArrayBuffer, &toBool);
            Bind(&toBool);
            {
                isLittleEndian = FastToBoolean(littleEndianHandle, 1);
                Jump(&getArrayBuffer);
            }
            Bind(&getArrayBuffer);
            {
                GateRef buffer = GetViewedArrayBuffer(thisValue);
                BRANCH(IsDetachedBuffer(buffer), slowPath, &checkOffset);
                Bind(&checkOffset);
                {
                    GateRef size = ZExtInt32ToInt64(GetByteLength(thisValue));
                    GateRef elementSize = GetElementSize(type);
                    GateRef indexInt64 = ZExtInt32ToInt64(index);
                    BRANCH(Int64UnsignedGreaterThan(Int64Add(indexInt64, elementSize), size), slowPath, &setValue);
                    Bind(&setValue);
                    {
                        GateRef offset = ZExtInt32ToInt64(GetByteOffset(thisValue));
                        GateRef bufferIndex = TruncInt64ToInt32(Int64Add(indexInt64, offset));
                        BuiltinsTypedArrayStubBuilder builder(this);
                        GateRef pointer = builder.GetDataPointFromBuffer(buffer);
                        GateRef doubleValue = TaggedGetNumber(value);
                        if constexpr (type == DataViewType::INT32 || type == DataViewType::UINT32) {
                            SetValueInBufferForInt32(glue, pointer, bufferIndex,
                                                     DoubleToInt(glue, doubleValue), *isLittleEndian);
                        } else if constexpr (type == DataViewType::FLOAT32) {
                            GateRef flaotValue = TruncDoubleToFloat32(doubleValue);
                            SetValueInBufferForInt32(glue, pointer, bufferIndex,
                                                     CastFloat32ToInt32(flaotValue), *isLittleEndian);
                        } else if constexpr (type == DataViewType::FLOAT64) {
                            GateRef int64Value = CastDoubleToInt64(doubleValue);
                            SetValueInBufferForInt64(glue, pointer, bufferIndex,
                                                     int64Value, *isLittleEndian);
                        }
                        Jump(exit);
                    }
                }
            }
        }
    }
}

template void BuiltinsDataViewStubBuilder::SetTypedValue<DataViewType::INT32>(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath);
template void BuiltinsDataViewStubBuilder::SetTypedValue<DataViewType::FLOAT32>(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath);
template void BuiltinsDataViewStubBuilder::SetTypedValue<DataViewType::FLOAT64>(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath);

void BuiltinsDataViewStubBuilder::SetValueInBufferForInt32(GateRef glue, GateRef pointer, GateRef offset,
    GateRef value, GateRef littleEndianHandle)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    Label littleEnd(env);
    Label notLittleEnd(env);
    GateRef b0 = Int32And(value, Int32(0xFF));
    GateRef b1 = Int32And(Int32LSR(value, Int32(builtins::BITS_EIGHT)), Int32(0xFF));
    GateRef b2 = Int32And(Int32LSR(value, Int32(2 * builtins::BITS_EIGHT)), Int32(0xFF));
    GateRef b3 = Int32LSR(value, Int32(builtins::BITS_TWENTY_FOUR));
    
    BRANCH(TaggedIsTrue(littleEndianHandle), &littleEnd, &notLittleEnd);
    Bind(&littleEnd);
    {
        Store(VariableType::INT8(), glue, pointer, offset, TruncInt32ToInt8(b0));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(1)), TruncInt32ToInt8(b1));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::TWO)), TruncInt32ToInt8(b2));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::THREE)), TruncInt32ToInt8(b3));
        Jump(&exit);
    }
    Bind(&notLittleEnd);
    {
        Store(VariableType::INT8(), glue, pointer, offset, TruncInt32ToInt8(b3));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(1)), TruncInt32ToInt8(b2));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::TWO)), TruncInt32ToInt8(b1));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::THREE)), TruncInt32ToInt8(b0));
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void BuiltinsDataViewStubBuilder::SetValueInBufferForInt64(GateRef glue, GateRef pointer, GateRef offset,
    GateRef value, GateRef littleEndianHandle)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    Label littleEnd(env);
    Label notLittleEnd(env);
    GateRef lowerInt32 = TruncInt64ToInt32(Int64And(value, Int64(0xFFFFFFFF))); // NOLINT
    GateRef highInt32 = TruncInt64ToInt32(Int64LSR(Int64And(value, Int64(0xFFFFFFFF00000000)), Int64(32))); // NOLINT

    GateRef b0 = Int32And(lowerInt32, Int32(builtins::BITS_MASK_FF));
    GateRef b1 = Int32And(Int32LSR(lowerInt32, Int32(builtins::BITS_EIGHT)), Int32(builtins::BITS_MASK_FF));
    // 2: 2 * 8 bits
    GateRef b2 = Int32And(Int32LSR(lowerInt32, Int32(2 * builtins::BITS_EIGHT)), Int32(builtins::BITS_MASK_FF));
    GateRef b3 = Int32LSR(lowerInt32, Int32(builtins::BITS_TWENTY_FOUR));
    GateRef b4 = Int32And(highInt32, Int32(builtins::BITS_MASK_FF));
    GateRef b5 = Int32And(Int32LSR(highInt32, Int32(builtins::BITS_EIGHT)), Int32(builtins::BITS_MASK_FF));
    // 2: 2 * 8 bits
    GateRef b6 = Int32And(Int32LSR(highInt32, Int32(2 * builtins::BITS_EIGHT)), Int32(builtins::BITS_MASK_FF));
    GateRef b7 = Int32LSR(highInt32, Int32(builtins::BITS_TWENTY_FOUR));
    
    BRANCH(TaggedIsTrue(littleEndianHandle), &littleEnd, &notLittleEnd);
    Bind(&littleEnd);
    {
        Store(VariableType::INT8(), glue, pointer, offset, TruncInt32ToInt8(b0));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(1)), TruncInt32ToInt8(b1));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::TWO)), TruncInt32ToInt8(b2));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::THREE)), TruncInt32ToInt8(b3));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::FOUR)), TruncInt32ToInt8(b4));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::FIVE)), TruncInt32ToInt8(b5));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::SIX)), TruncInt32ToInt8(b6));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::SEVEN)), TruncInt32ToInt8(b7));
        Jump(&exit);
    }
    Bind(&notLittleEnd);
    {
        Store(VariableType::INT8(), glue, pointer, offset, TruncInt32ToInt8(b7));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(1)), TruncInt32ToInt8(b6));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::TWO)), TruncInt32ToInt8(b5));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::THREE)), TruncInt32ToInt8(b4));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::FOUR)), TruncInt32ToInt8(b3));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::FIVE)), TruncInt32ToInt8(b2));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::SIX)), TruncInt32ToInt8(b1));
        Store(VariableType::INT8(), glue, pointer, Int32Add(offset, Int32(OffsetIndex::SEVEN)), TruncInt32ToInt8(b0));
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef BuiltinsDataViewStubBuilder::GetElementSize(DataViewType type)
{
    GateRef size;
    switch (type) {
        case DataViewType::INT8:
        case DataViewType::UINT8:
        case DataViewType::UINT8_CLAMPED:
            size = Int64(1);
            break;
        case DataViewType::INT16:
        case DataViewType::UINT16:
            size = Int64(2);  // 2 means the length
            break;
        case DataViewType::INT32:
        case DataViewType::UINT32:
        case DataViewType::FLOAT32:
            size = Int64(4);  // 4 means the length
            break;
        case DataViewType::FLOAT64:
        case DataViewType::BIGINT64:
        case DataViewType::BIGUINT64:
            size = Int64(8);  // 8 means the length
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return size;
}
}  // namespace panda::ecmascript::kungfu
