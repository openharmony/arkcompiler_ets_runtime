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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_OBJECT_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_OBJECT_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"

namespace panda::ecmascript::kungfu {
class BuiltinsObjectStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsObjectStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    BuiltinsObjectStubBuilder(BuiltinsStubBuilder *parent, GateRef glue, GateRef thisValue, GateRef numArgs)
        : BuiltinsStubBuilder(parent), glue_(glue), thisValue_(thisValue), numArgs_(numArgs) {}
    ~BuiltinsObjectStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsObjectStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsObjectStubBuilder);
    void GenerateCircuit() override {}
    GateRef CreateListFromArrayLike(GateRef glue, GateRef arrayObj);
    void ToString(Variable *result, Label *exit, Label *slowPath);
    void Create(Variable *result, Label *exit, Label *slowPath);
    void Assign(Variable *result, Label *exit, Label *slowPath);

private:
    GateRef OrdinaryNewJSObjectCreate(GateRef proto);
    GateRef TransProtoWithoutLayout(GateRef hClass, GateRef proto);
    void AssignEnumElementProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void LayoutInfoAssignAllEnumProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void NameDictionaryAssignAllEnumProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source,
        GateRef properties);
    void SlowAssign(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void FastAssign(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void AssignAllEnumProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void Assign(Variable *res, Label *nextIt, Label *funcExit, GateRef toAssign, GateRef source);

    GateRef glue_;
    GateRef thisValue_;
    GateRef numArgs_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_OBJECT_STUB_BUILDER_H
