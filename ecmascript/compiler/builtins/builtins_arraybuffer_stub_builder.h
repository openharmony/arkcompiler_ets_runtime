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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_ARRAYBUFFER_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_ARRAYBUFFER_STUB_BUILDER_H

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_arraybuffer.h"

namespace panda::ecmascript::kungfu {
class BuiltinsArrayBufferStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsArrayBufferStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    ~BuiltinsArrayBufferStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsArrayBufferStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsArrayBufferStubBuilder);
    void GenerateCircuit() override {}

    GateRef GetValueFromBuffer(GateRef glue, GateRef pointer, GateRef byteIndex,
                               GateRef isLittleEndian, DataViewType type);

    GateRef GetDataPointFromBuffer(GateRef glue, GateRef arrBuf);

private:

    GateRef GetArrayBufferByteLength(GateRef buffer)
    {
        GateRef offset = IntPtr(JSArrayBuffer::BYTE_LENGTH_OFFSET);
        return LoadPrimitive(VariableType::INT32(), buffer, offset);
    }

    GateRef GetExternalPointer(GateRef buffer)
    {
        GateRef offset = IntPtr(JSNativePointer::POINTER_OFFSET);
        return LoadPrimitive(VariableType::NATIVE_POINTER(), buffer, offset);
    }

    // For integer types: INT16, UINT16, INT32, UINT32
    GateRef GetValueFromBufferForInteger(GateRef glue, GateRef pointer, GateRef offset,
        GateRef littleEndianHandle, DataViewType type);

    // For floating point types: FLOAT32, FLOAT64
    GateRef GetValueFromBufferForFloat(GateRef glue, GateRef pointer, GateRef offset,
        GateRef littleEndianHandle, DataViewType type);

    // Read multi-byte value and handle endianness conversion
    GateRef ReadAndAssembleBytes(GateRef glue, GateRef pointer, GateRef offset,
        GateRef littleEndianHandle, uint32_t byteSize);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_ARRAYBUFFER_STUB_BUILDER_H
