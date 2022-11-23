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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STACK_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STACK_STUB_BUILDER_H
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_api/js_api_stack.h"

namespace panda::ecmascript::kungfu {
class ContainersStackStubBuilder : public StubBuilder {
public:
    explicit ContainersStackStubBuilder(StubBuilder *parent)
        : StubBuilder(parent) {}
    ~ContainersStackStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersStackStubBuilder);
    NO_COPY_SEMANTIC(ContainersStackStubBuilder);
    void GenerateCircuit() override {}
    GateRef GetSize(GateRef obj)
    {
        GateRef top = Load(VariableType::INT32(), obj, IntPtr(JSAPIStack::TOP_OFFSET));
        return Int32Add(top, Int32(1));
    }

    GateRef Get(GateRef obj, GateRef index)
    {
        GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
        GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
        return GetValueFromTaggedArray(elements, index);
    }
    void Set(GateRef glue, GateRef obj, GateRef index, GateRef value)
    {
        GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
        GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, value);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STACK_STUB_BUILDER_H