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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_DATAVIEW_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_DATAVIEW_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_dataview.h"
namespace panda::ecmascript::kungfu {
class BuiltinsDataViewStubBuilder : public BuiltinsStubBuilder {
public:
    enum OffsetIndex : uint8_t { ZERO = 0, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN};
    explicit BuiltinsDataViewStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    ~BuiltinsDataViewStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsDataViewStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsDataViewStubBuilder);
    void GenerateCircuit() override {}

#define DATAVIEW_SET_TYPES(V)    \
    V(Int32,  INT32)             \
    V(Float32, FLOAT32)          \
    V(Float64, FLOAT64)

    // Set methods
#define DECLARE_DATAVIEW_SET_METHOD(name, type)                      \
    void Set##name(GateRef glue, GateRef thisValue, GateRef numArgs, \
        Variable* res, Label *exit, Label *slowPath);

    DATAVIEW_SET_TYPES(DECLARE_DATAVIEW_SET_METHOD)
#undef DECLARE_DATAVIEW_SET_METHOD

// V(MethodName, DataViewType)
#define DATAVIEW_GET_TYPES(V)       \
    V(Float32,   FLOAT32)           \
    V(Float64,   FLOAT64)           \
    V(Int8,      INT8)              \
    V(Int16,     INT16)             \
    V(Int32,     INT32)             \
    V(Uint8,     UINT8)             \
    V(Uint16,    UINT16)            \
    V(Uint32,    UINT32)

    // Get methods
#define DECLARE_DATAVIEW_GET_METHOD(name, type)                      \
    void Get##name(GateRef glue, GateRef thisValue, GateRef numArgs, \
        Variable* res, Label *exit, Label *slowPath);

    DATAVIEW_GET_TYPES(DECLARE_DATAVIEW_GET_METHOD)
#undef DECLARE_DATAVIEW_GET_METHOD

    GateRef GetElementSize(DataViewType type);

private:
    template <DataViewType type>
    void GetTypedValue(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* res, Label *exit, Label *slowPath);

    // Set methods
    template <DataViewType type>
    void SetTypedValue(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* res, Label *exit, Label *slowPath);
    void SetValueInBufferForInt32(GateRef glue, GateRef pointer, GateRef offset,
        GateRef value, GateRef littleEndianHandle);
    void SetValueInBufferForInt64(GateRef glue, GateRef pointer, GateRef offset,
        GateRef value, GateRef littleEndianHandle);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_DATAVIEW_STUB_BUILDER_H