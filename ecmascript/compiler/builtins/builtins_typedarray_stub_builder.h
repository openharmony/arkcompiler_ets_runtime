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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_TYPEDARRAY_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_TYPEDARRAY_STUB_BUILDER_H

#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_typed_array.h"

namespace panda::ecmascript::kungfu {
class BuiltinsTypedArrayStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsTypedArrayStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~BuiltinsTypedArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsTypedArrayStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsTypedArrayStubBuilder);
    void GenerateCircuit() override {}
    GateRef FastGetPropertyByIndex(GateRef glue, GateRef array, GateRef index, GateRef jsType);
    GateRef FastCopyElementToArray(GateRef glue, GateRef typedArray, GateRef array);
    GateRef LoadTypedArrayElement(GateRef glue, GateRef array, GateRef key, GateRef jsType);
    GateRef StoreTypedArrayElement(GateRef glue, GateRef array, GateRef index, GateRef value, GateRef jsType);
    GateRef CheckTypedArrayIndexInRange(GateRef array, GateRef index);
    GateRef GetValueFromBuffer(GateRef buffer, GateRef index, GateRef offset, GateRef jsType);
    GateRef GetDataPointFromBuffer(GateRef arrBuf);
    GateRef CalculatePositionWithLength(GateRef position, GateRef length);

#define DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef numArgs, GateRef end, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_TYPEDARRAY_STUB_BUILDER(DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER)
#undef DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER

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

    GateRef GetArrayBufferByteLength(GateRef buffer)
    {
        GateRef offset = IntPtr(JSArrayBuffer::BYTE_LENGTH_OFFSET);
        return Load(VariableType::INT32(), buffer, offset);
    }

    GateRef GetExternalPointer(GateRef buffer)
    {
        GateRef offset = IntPtr(JSNativePointer::POINTER_OFFSET);
        return Load(VariableType::NATIVE_POINTER(), buffer, offset);
    }
private:
    GateRef ChangeByteArrayTaggedPointerToInt64(GateRef x)
    {
        return GetEnvironment()->GetBuilder()->ChangeTaggedPointerToInt64(x);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_TYPEDARRAY_STUB_BUILDER_H