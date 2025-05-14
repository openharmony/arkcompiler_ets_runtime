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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_LINKEDLIST_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_LINKEDLIST_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_api/js_api_linked_list.h"

namespace panda::ecmascript::kungfu {
class ContainersLinkedListStubBuilder : public BuiltinsStubBuilder {
public:
    explicit ContainersLinkedListStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    ~ContainersLinkedListStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersLinkedListStubBuilder);
    NO_COPY_SEMANTIC(ContainersLinkedListStubBuilder);
    void GenerateCircuit() override {}

#define DECLARE_CONTAINERS_LIST_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_CONTAINERS_LIST_STUB_BUILDER(DECLARE_CONTAINERS_LIST_STUB_BUILDER)
#undef DECLARE_CONTAINERS_LIST_STUB_BUILDER

    GateRef GetTableLength(GateRef glue, GateRef obj)
    {
        GateRef tableOffset = IntPtr(JSAPILinkedList::DOUBLE_LIST_OFFSET);
        GateRef table = Load(VariableType::JS_POINTER(), glue, obj, tableOffset);
        GateRef value = GetValueFromTaggedArray(glue, table, Int32(TaggedDoubleList::NUMBER_OF_NODE_INDEX));
        return TaggedGetInt(value);
    }

    GateRef GetNode(GateRef glue, GateRef obj, GateRef index)
    {
        GateRef tableOffset = IntPtr(JSAPILinkedList::DOUBLE_LIST_OFFSET);
        GateRef table = Load(VariableType::JS_POINTER(), glue, obj, tableOffset);
        return GetValueFromTaggedArray(glue, table, index);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_LINKEDLIST_STUB_BUILDER_H
