/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_ARRAY_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_ARRAY_STUB_BUILDER_H
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/stub_builder-inl.h"

namespace panda::ecmascript::kungfu {
class BuiltinsArrayStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsArrayStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~BuiltinsArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsArrayStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsArrayStubBuilder);
    void GenerateCircuit() override {}

    void Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

    void Filter(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

    void ForEach(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

    void IndexOf(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

    void LastIndexOf(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

    void Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

    void Reverse(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath);

private:
    static constexpr uint32_t MAX_LENGTH_ZERO = 0;
    static constexpr uint32_t MAX_LENGTH_ONE = 1;
    struct JsArrayRequirements {
        bool stable = false;
        bool defaultConstructor = false;
    };
    GateRef IsJsArrayWithLengthLimit(GateRef glue, GateRef object,
        uint32_t maxLength, JsArrayRequirements requirements);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_ARRAY_STUB_BUILDER_H
