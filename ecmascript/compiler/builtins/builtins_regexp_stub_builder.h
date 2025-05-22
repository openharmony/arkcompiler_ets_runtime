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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_REGEXP_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_REGEXP_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"

namespace panda::ecmascript::kungfu {

class BuiltinsRegExpStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsRegExpStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    BuiltinsRegExpStubBuilder(CallSignature *callSignature, Environment *env, GateRef globalEnv)
        : BuiltinsStubBuilder(callSignature, env, globalEnv) {}
    explicit BuiltinsRegExpStubBuilder(Environment* env, GateRef globalEnv): BuiltinsStubBuilder(env, globalEnv) {}
    ~BuiltinsRegExpStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsRegExpStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsRegExpStubBuilder);
    void GenerateCircuit() override {}
    GateRef TryToFastGetProperty(GateRef glue, GateRef thisValue, ConstantIndex constIndex);

#define DECLARE_BUILTINS_REGEXP_STUB_BUILDER(method, ...) \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *res, Label *exit, Label *slowPath);
BUILTINS_WITH_REGEXP_STUB_BUILDER(DECLARE_BUILTINS_REGEXP_STUB_BUILDER)
#undef DECLARE_BUILTINS_REGEXP_STUB_BUILDER
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_REGEXP_STUB_BUILDER_H
