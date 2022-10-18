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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/containers_vector_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_api/js_api_vector.h"

namespace panda::ecmascript::kungfu {
// enumerate container functions that use function call
enum class ContainersType : uint8_t {
    VECTOR_FOREACH = 0,
    VECTOR_REPLACEALLELEMENTS,
};

class ContainersStubBuilder : public BuiltinsStubBuilder {
public:
    explicit ContainersStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~ContainersStubBuilder() = default;
    NO_MOVE_SEMANTIC(ContainersStubBuilder);
    NO_COPY_SEMANTIC(ContainersStubBuilder);
    void GenerateCircuit() override {}

    void ContainersCommonFuncCall(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable* result, Label *exit, Label *slowPath, ContainersType type);

    GateRef IsContainer(GateRef obj, ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
                return IsJSAPIVector(obj);
            default:
                UNREACHABLE();
        }
        return False();
    }

    bool IsReplaceAllElements(ContainersType type)
    {
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
                return false;
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
                return true;
            default:
                UNREACHABLE();
        }
        return false;
    }

    void ContainerSet(GateRef glue, GateRef obj, GateRef index, GateRef value, ContainersType type)
    {
        ContainersVectorStubBuilder vectorBuilder(this);
        switch (type) {
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
                vectorBuilder.Set(glue, obj, index, value);
                break;
            default:
                UNREACHABLE();
        }
    }

    GateRef ContainerGetSize(GateRef obj, ContainersType type)
    {
        ContainersVectorStubBuilder vectorBuilder(this);
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
                return vectorBuilder.GetSize(obj);
            default:
                UNREACHABLE();
        }
        return False();
    }

    GateRef ContainerGetValue(GateRef obj, GateRef index, ContainersType type)
    {
        ContainersVectorStubBuilder vectorBuilder(this);
        switch (type) {
            case ContainersType::VECTOR_FOREACH:
            case ContainersType::VECTOR_REPLACEALLELEMENTS:
                return vectorBuilder.Get(obj, index);
            default:
                UNREACHABLE();
        }
        return False();
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_CONTAINERS_STUB_BUILDER_H