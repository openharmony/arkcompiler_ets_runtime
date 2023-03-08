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
    ~OperationsStubBuilder() = default;
    NO_MOVE_SEMANTIC(OperationsStubBuilder);
    NO_COPY_SEMANTIC(OperationsStubBuilder);
    void GenerateCircuit() override {}
    // unary op
    GateRef Inc(GateRef glue, GateRef value, ProfileOperation callback = nullptr);
    GateRef Dec(GateRef glue, GateRef value, ProfileOperation callback = nullptr);
    GateRef Neg(GateRef glue, GateRef value, ProfileOperation callback = nullptr);
    GateRef Not(GateRef glue, GateRef value, ProfileOperation callback = nullptr);

    // binary op
    GateRef Equal(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef NotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef StrictEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef StrictNotEqual(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Less(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef LessEq(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Greater(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef GreaterEq(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Add(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Sub(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Mul(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Div(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Mod(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Shl(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Shr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Ashr(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef And(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Or(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
    GateRef Xor(GateRef glue, GateRef left, GateRef right, ProfileOperation callback = nullptr);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_OPERATIONS_STUB_BUILDER_H
