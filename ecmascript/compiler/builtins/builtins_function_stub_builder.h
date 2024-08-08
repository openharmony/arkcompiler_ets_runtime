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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_FUNCTION_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_FUNCTION_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"

namespace panda::ecmascript::kungfu {
class BuiltinsFunctionStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsFunctionStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    ~BuiltinsFunctionStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsFunctionStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsFunctionStubBuilder);
    void GenerateCircuit() override {}
#define DECLARE_BUILTINS_FUNCTION_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable* res, Label *exit, Label *slowPath);
BUILTINS_WITH_FUNCTION_STUB_BUILDER(DECLARE_BUILTINS_FUNCTION_STUB_BUILDER)
#undef DECLARE_BUILTINS_FUNCTION_STUB_BUILDER

    GateRef BuildArgumentsListFastElements(GateRef glue, GateRef arrayObj);

    void InitializeJSFunction(GateRef glue, GateRef func, GateRef kind,
                              FunctionKind getKind = FunctionKind::LAST_FUNCTION_KIND);

    void InitializeSFunction(GateRef glue, GateRef func, GateRef kind,
                             FunctionKind getKind = FunctionKind::LAST_FUNCTION_KIND);

    void InitializeFunctionWithMethod(GateRef glue, GateRef func, GateRef method,
                                      GateRef hclass);

private:
    GateRef MakeArgListWithHole(GateRef glue, GateRef argv, GateRef length);
    GateRef NewTaggedArrayFromArgs(GateRef glue, GateRef startIndex, GateRef length, GateRef numArgs);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_FUNCTION_STUB_BUILDER_H