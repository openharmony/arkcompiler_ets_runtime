/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_SHARED_ARRAY_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_SHARED_ARRAY_STUB_BUILDER_H
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_stable_array.h"
#include "ecmascript/shared_objects/js_shared_array.h"

namespace panda::ecmascript::kungfu {
class BuiltinsSharedArrayStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsSharedArrayStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    explicit BuiltinsSharedArrayStubBuilder(Environment* env, GateRef globalEnv): BuiltinsStubBuilder(env, globalEnv) {}
    ~BuiltinsSharedArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsSharedArrayStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsSharedArrayStubBuilder);
    void GenerateCircuit() override {}

    void GenSharedArrayConstructor(GateRef glue, GateRef nativeCode, GateRef func,
        GateRef newTarget, GateRef thisValue, GateRef numArgs);

    void Create(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void FastCreateSharedArrayWithArgv(GateRef glue, Variable *res, GateRef argc,
        GateRef hclass, Label *exit, Label *slowPath);

    void FastCreateSharedArrayWithLen(GateRef glue, Variable *res, GateRef len,
        GateRef initValue, GateRef hclass, Label *exit, Label *slowPath);

    GateRef NewSharedArrayWithHClass(GateRef glue, GateRef hclass);

    void IsArray(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void Push(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void Pop(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void Find(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void LastIndexOf(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void IndexOf(GateRef glue, GateRef thisValue,
        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    GateRef GrowElementsCapacity(GateRef glue, GateRef thisValue, GateRef capacity);

    using ComparisonType = JSStableArray::ComparisonType;
    using IndexOfReturnType = JSStableArray::IndexOfReturnType;
    using IndexOfOptions = JSStableArray::IndexOfOptions;

    enum class StringElementsCondition {
        MUST_BE_STRING,
        MAY_BE_HOLE,
        MAY_BE_ANY,
    };
private:
    GateRef PushImpl(GateRef glue, GateRef thisValue, GateRef numArgs);
    GateRef PopImpl(GateRef glue, GateRef thisValue);
    GateRef FindImpl(GateRef glue, GateRef thisValue, GateRef numArgs);
    void IndexOfOptimised(GateRef glue, GateRef thisValue, GateRef numArgs,
        Variable *result, Label *exit, Label *slowPath, IndexOfOptions options);
    GateRef IndexOfImpl(GateRef glue, GateRef thisValue,
        GateRef numArgs, IndexOfOptions options);
    GateRef MakeFromIndex(GateRef input, GateRef length, bool reversedOrder);
    GateRef MakeFromIndexWithInt(GateRef intValue, GateRef length, bool reversedOrder);
    GateRef MakeFromIndexWithDouble(GateRef doubleValue, GateRef length, bool reversedOrder);
    GateRef IndexOfTaggedUndefined(
        GateRef glue, GateRef elements, GateRef fromIndex, GateRef len, IndexOfOptions options);

    template <class Predicate>
    GateRef IndexOfElements(
        GateRef glue, GateRef elements, Predicate predicate, GateRef fromIndex, GateRef len, IndexOfOptions options);
    Variable MakeResultVariableDefaultNotFound(IndexOfOptions options);
    GateRef IndexOfTaggedIntElements(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedZero(
        GateRef glue, GateRef elements, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedNumber(GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len,
        IndexOfOptions options, bool targetIsAlwaysDouble);
    GateRef IndexOfStringElements(GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len,
        IndexOfOptions options, StringElementsCondition condition);
    GateRef IndexOfBigIntOrObjectElements(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfGeneric(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfBigInt(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfObject(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedIntTarget(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfStringTarget(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfBigIntOrObjectTarget(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef StringEqual(
        GateRef glue, GateRef left, GateRef right, StringElementsCondition rightCondition);
    GateRef BigIntEqual(GateRef glue, GateRef left, GateRef right);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_SHARED_ARRAY_STUB_BUILDER_H
