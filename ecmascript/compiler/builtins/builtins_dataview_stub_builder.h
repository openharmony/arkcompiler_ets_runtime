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
    template <DataViewType type>
    void SetTypedValue(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* res, Label *exit, Label *slowPath);
    void SetValueInBufferForInt32(GateRef glue, GateRef pointer, GateRef offset,
        GateRef value, GateRef littleEndianHandle);
    void SetValueInBufferForInt64(GateRef glue, GateRef pointer, GateRef offset,
        GateRef value, GateRef littleEndianHandle);
    GateRef GetElementSize(DataViewType type);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_DATAVIEW_STUB_BUILDER_H