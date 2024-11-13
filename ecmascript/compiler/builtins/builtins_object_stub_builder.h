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
    BuiltinsObjectStubBuilder(Environment *env, GateRef glue)
        : BuiltinsStubBuilder(env), glue_(glue) {}
    ~BuiltinsObjectStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsObjectStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsObjectStubBuilder);
    void GenerateCircuit() override {}

#define DECLARE_BUILTINS_OBJECT_STUB_BUILDER(method, ...)           \
    void method(Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_OBJECT_STUB_BUILDER(DECLARE_BUILTINS_OBJECT_STUB_BUILDER)
#undef DECLARE_BUILTINS_OBJECT_STUB_BUILDER

    GateRef CreateListFromArrayLike(GateRef glue, GateRef arrayObj);
    GateRef CreateArrayFromList(GateRef glue, GateRef elements);
    void HasOwnProperty(Variable *result, Label *exit, Label *slowPath, GateRef thisValue, GateRef prop,
                        GateRef hir = Circuit::NullGate());

private:
    GateRef GetNumKeysFromLayoutInfo(GateRef end, GateRef layoutInfo);
    GateRef GetNumKeysFromDictionary(GateRef array);
    GateRef CopyFromEnumCache(GateRef glue, GateRef elements);
    GateRef GetAllEnumKeys(GateRef glue, GateRef obj);
    GateRef GetEnumElementKeys(GateRef glue, GateRef obj);
    GateRef GetAllElementKeys(GateRef glue, GateRef obj, GateRef offset, GateRef array);
    GateRef GetAllPropertyKeys(GateRef glue, GateRef obj, GateRef offset, GateRef array);
    GateRef GetEnumElementEntries(GateRef glue, GateRef obj, Label *slowPath);
    GateRef GetEnumPropertyEntries(GateRef glue, GateRef obj, Label *slowPath);
    GateRef TestIntegrityLevel(GateRef glue, GateRef obj, GateRef level, Label *slowPath);
    GateRef ObjectSetPrototype(GateRef glue, GateRef obj, GateRef proto);
    GateRef IsNotSlowObjectKey(GateRef obj);
    void LayoutInfoGetAllEnumKeys(GateRef end, GateRef offset, GateRef array, GateRef layoutInfo);
    void AssignEnumElementProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void LayoutInfoAssignAllEnumProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void NameDictionaryAssignAllEnumProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source,
        GateRef properties);
    void SlowAssign(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void FastAssign(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void AssignAllEnumProperty(Variable *res, Label *funcExit, GateRef toAssign, GateRef source);
    void Assign(Variable *res, Label *nextIt, Label *funcExit, GateRef toAssign, GateRef source);

    GateRef glue_ {Circuit::NullGate()};
    GateRef thisValue_ {Circuit::NullGate()};
    GateRef numArgs_ {Circuit::NullGate()};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_OBJECT_STUB_BUILDER_H
