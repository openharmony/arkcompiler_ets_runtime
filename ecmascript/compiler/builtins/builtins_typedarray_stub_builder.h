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

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_typed_array.h"

#define BUILTIN_COMPILER_TYPED_ARRAY_TYPES(V)                      \
    V(INT8,           Int8,             0)                         \
    V(UINT8,          Uint8,            1)                         \
    V(UINT8_CLAMPED,  Uint8Clamped,     2)                         \
    V(INT16,          Int16,            3)                         \
    V(UINT16,         Uint16,           4)                         \
    V(INT32,          Int32,            5)                         \
    V(UINT32,         Uint32,           6)                         \
    V(FLOAT32,        Float32,          7)                         \
    V(FLOAT64,        Float64,          8)                         \
    V(BIGINT64,       BigInt64,         9)                         \
    V(BIGUINT64,      BigUint64,        10)

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
    void DoSort(GateRef glue, GateRef receiver, Variable *result, Label *exit, Label *slowPath);
    void FastSetPropertyByIndex(GateRef glue, GateRef value, GateRef array, GateRef index, GateRef jsType);
    void SetValueToBuffer(GateRef glue, GateRef value, GateRef buffer, GateRef index,
                          GateRef offset, GateRef jsType, Label *slowPath);
    void GenTypedArrayConstructor(GateRef glue, GateRef nativeCode, GateRef func,
        GateRef newTarget, GateRef thisValue, GateRef numArgs, GateRef constructorName, const DataViewType arrayType);
    GateRef AllocateTypedArray(GateRef glue, GateRef constructorName, GateRef func, GateRef newTarget,
                               GateRef length, GateRef lengthTagged, const DataViewType arrayType);
    GateRef AllocateTypedArray(GateRef glue, GateRef constructorName,
                               GateRef func, GateRef newTarget, const DataViewType arrayType);
    GateRef AllocateTypedArrayBuffer(GateRef glue, GateRef typedArray, GateRef length, GateRef lengthTagged,
                                     const DataViewType arrayType);
    void CreateFromArrayBuffer(Variable *result, GateRef glue, GateRef numArgs,
                               Label *exit, Label *slowPath, const DataViewType arrayType);
    void SetArrayBufferProperties(GateRef glue, GateRef typedArray,
                                  GateRef newByteLength, GateRef offset, GateRef arrayLength);
    GateRef GetOnHeapHclassFromType(GateRef glue, GateRef objHclass, const DataViewType arrayType);
    void FastCopyElementFromArray(Variable *result, GateRef glue, GateRef array,
        Label *exit, Label *slowPath, const DataViewType arrayType);
    void FastCopyFromArrayToTypedArray(GateRef glue, GateRef array, Variable *result,
        GateRef targetOffset, GateRef srcLength, Label *check, Label *slowPath,
        const DataViewType arrayType, bool typedArrayFromCtor);
    void CopyElementsToArrayBuffer(GateRef glue, GateRef srcLength, GateRef array,
        GateRef buffer, GateRef targetByteIndex, const DataViewType arrayType, bool getWithKind);
    void FastSetValueInBuffer(GateRef glue, GateRef buffer,
        GateRef byteIndex, GateRef val, const DataViewType arrayType);
    void SetValueInBufferForByte(GateRef glue, GateRef val, GateRef pointer, GateRef byteIndex);
    template<typename T>
    void SetValueInBufferForInteger(GateRef glue, GateRef val, GateRef pointer, GateRef byteIndex);
    template<typename T>
    void SetValueInBufferForFloat(GateRef glue, GateRef val, GateRef pointer, GateRef byteIndex);

#define DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef numArgs, GateRef end, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_TYPEDARRAY_STUB_BUILDER(DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER)
#undef DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER

    void GetByteLength(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);
    void GetByteOffset(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);
    void Of(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

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
private:
    void BuildArrayIterator(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath, IterationKind iteratorKind);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_TYPEDARRAY_STUB_BUILDER_H