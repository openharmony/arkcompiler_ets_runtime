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

#ifndef ECMASCRIPT_COMPILER_TYPED_ARRAY_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_TYPED_ARRAY_STUB_BUILDER_H

#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_typed_array.h"

namespace panda::ecmascript::kungfu {
class TypedArrayStubBuilder : public StubBuilder {
public:
    explicit TypedArrayStubBuilder(StubBuilder *parent)
        : StubBuilder(parent) {}
    ~TypedArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(TypedArrayStubBuilder);
    NO_COPY_SEMANTIC(TypedArrayStubBuilder);
    void GenerateCircuit() override {}
    GateRef FastGetPropertyByIndex(GateRef glue, GateRef array, GateRef index, GateRef jsType);
    GateRef FastCopyElementToArray(GateRef glue, GateRef typedArray, GateRef array);
    GateRef GetValueFromBuffer(GateRef buffer, GateRef index, GateRef offset, GateRef jsType);
    GateRef IsDetachedBuffer(GateRef buffer);
    GateRef GetDataPointFromBuffer(GateRef arrBuf);
    GateRef GetViewedArrayBuffer(GateRef array)
    {
        GateRef offset = IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
        return Load(VariableType::JS_ANY(), array, offset);
    }

    GateRef GetArrayLength(GateRef array)
    {
        GateRef offset = IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET);
        return Load(VariableType::INT32(), array, offset);
    }

    GateRef GetByteOffset(GateRef array)
    {
        GateRef offset = IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET);
        return Load(VariableType::INT32(), array, offset);
    }

    GateRef GetArrayBufferData(GateRef buffer)
    {
        GateRef offset = IntPtr(JSArrayBuffer::DATA_OFFSET);
        return Load(VariableType::JS_ANY(), buffer, offset);
    }

    GateRef GetArrayBufferByteLength(GateRef buffer)
    {
        GateRef offset = IntPtr(JSArrayBuffer::BYTE_LENGTH_OFFSET);
        return Load(VariableType::INT32(), buffer, offset);
    }

    GateRef GetExternalPointer(GateRef buffer)
    {
        GateRef offset = IntPtr(JSNativePointer::POINTER_OFFSET);
        return Load(VariableType::JS_ANY(), buffer, offset);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPED_ARRAY_STUB_BUILDER_H