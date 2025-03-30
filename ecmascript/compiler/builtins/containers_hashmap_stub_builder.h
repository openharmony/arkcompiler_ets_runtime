/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_HASHMAP_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_HASHMAP_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_api/js_api_hashmap.h"

namespace panda::ecmascript::kungfu {
class ContainersHashMapStubBuilder : public BuiltinsStubBuilder {
public:
    explicit ContainersHashMapStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~ContainersHashMapStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersHashMapStubBuilder);
    NO_COPY_SEMANTIC(ContainersHashMapStubBuilder);
    void GenerateCircuit() override {}

#define DECLARE_CONTAINERS_HASHMAP_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_CONTAINERS_HASHMAP_STUB_BUILDER(DECLARE_CONTAINERS_HASHMAP_STUB_BUILDER)
#undef DECLARE_CONTAINERS_HASHMAP_STUB_BUILDER

    GateRef GetTableLength(GateRef obj)
    {
        GateRef tableOffset = IntPtr(JSAPIHashMap::HASHMAP_TABLE_INDEX);
        GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
        return GetLengthOfTaggedArray(table);
    }

    GateRef GetNode(GateRef obj, GateRef index)
    {
        GateRef tableOffset = IntPtr(JSAPIHashMap::HASHMAP_TABLE_INDEX);
        GateRef table = Load(VariableType::JS_POINTER(), obj, tableOffset);
        return GetValueFromTaggedArray(table, index);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_HASHMAP_STUB_BUILDER_H
