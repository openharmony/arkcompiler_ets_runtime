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

#ifndef ECMASCRIPT_COMPILER_OPERATIONS_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_OPERATIONS_STUB_BUILDER_H
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {
class OperationsStubBuilder : public StubBuilder {
public:
    explicit OperationsStubBuilder(StubBuilder *parent)
        : StubBuilder(parent) {}
    OperationsStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : StubBuilder(parent, globalEnv) {}
    ~OperationsStubBuilder() = default;
    NO_MOVE_SEMANTIC(OperationsStubBuilder);
    NO_COPY_SEMANTIC(OperationsStubBuilder);
    void GenerateCircuit() override {}
    // unary op
    GateRef Inc(GateRef glue, GateRef value, ProfileOperation callback = ProfileOperation());
    GateRef Dec(GateRef glue, GateRef value, ProfileOperation callback = ProfileOperation());
    GateRef Neg(GateRef glue, GateRef value, ProfileOperation callback = ProfileOperation());
    GateRef Not(GateRef glue, GateRef value, ProfileOperation callback = ProfileOperation());

    // binary op
    GateRef Equal(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef NotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef StrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef StrictNotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Less(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef LessEq(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Greater(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef GreaterEq(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Add(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Sub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Mul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Div(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Mod(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Shl(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Shr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Ashr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef And(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Or(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
    GateRef Xor(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = ProfileOperation());
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_OPERATIONS_STUB_BUILDER_H
