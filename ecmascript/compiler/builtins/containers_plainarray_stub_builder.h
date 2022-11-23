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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_PLAINARRAY_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_PLAINARRAY_STUB_BUILDER_H
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/js_api/js_api_plain_array.h"

namespace panda::ecmascript::kungfu {
class ContainersPlainArrayStubBuilder : public StubBuilder {
public:
    explicit ContainersPlainArrayStubBuilder(StubBuilder *parent)
        : StubBuilder(parent) {}
    ~ContainersPlainArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ContainersPlainArrayStubBuilder);
    NO_COPY_SEMANTIC(ContainersPlainArrayStubBuilder);
    void GenerateCircuit() override {}
    GateRef GetSize(GateRef obj)
    {
        return Load(VariableType::INT32(), obj, IntPtr(JSAPIPlainArray::LENGTH_OFFSET));
    }

    GateRef Get(GateRef obj, GateRef index)
    {
        GateRef elementsOffset = IntPtr(JSAPIPlainArray::VALUES_OFFSET);
        GateRef elements = Load(VariableType::JS_POINTER(), obj, elementsOffset);
        return GetValueFromTaggedArray(elements, index);
    }

    GateRef GetKey(GateRef obj, GateRef index)
    {
        GateRef elementsOffset = IntPtr(JSAPIPlainArray::KEYS_OFFSET);
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
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_PLAINARRAY_STUB_BUILDER_H