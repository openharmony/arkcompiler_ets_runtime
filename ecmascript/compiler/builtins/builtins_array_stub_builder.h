/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_stable_array.h"

namespace panda::ecmascript::kungfu {
class BuiltinsArrayStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsArrayStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    explicit BuiltinsArrayStubBuilder(Environment* env, GateRef globalEnv): BuiltinsStubBuilder(env, globalEnv) {}
    ~BuiltinsArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsArrayStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsArrayStubBuilder);
    void GenerateCircuit() override {}

#define DECLARE_BUILTINS_ARRAY_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_ARRAY_STUB_BUILDER(DECLARE_BUILTINS_ARRAY_STUB_BUILDER)
#undef DECLARE_BUILTINS_ARRAY_STUB_BUILDER

    void Sort(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void ArrayIteratorNext(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                           Label *slowPath);

    void SortAfterArgs(GateRef glue, GateRef thisValue, GateRef callbackFnHandle,
                       Variable *result, Label *exit, Label *slowPath, GateRef hir = Circuit::NullGate());

    void GenArrayConstructor(GateRef glue, GateRef nativeCode, GateRef func,
        GateRef newTarget, GateRef thisValue, GateRef numArgs);

    void FastCreateArrayWithArgv(GateRef glue, Variable *res, GateRef argc, GateRef hclass, Label *exit);
    
    void ElementsKindHclassCompare(GateRef glue, GateRef arrayCls, Label *matchCls, Label *slowPath);

    GateRef IsConcatSpreadable(GateRef glue, GateRef obj);

    void InitializeArray(GateRef glue, GateRef count, Variable *result);

    GateRef NewArray(GateRef glue, GateRef count);

    GateRef NewArrayWithHClass(GateRef glue, GateRef hclass, GateRef newArrayLen);

    GateRef CalculatePositionWithLength(GateRef position, GateRef length);

    GateRef DoSort(GateRef glue, GateRef receiver, bool isToSorted,
        Variable *result, Label *exit, Label *slowPath, GateRef hir = Circuit::NullGate());

    void FastReverse(GateRef glue, GateRef thisValue, GateRef len,
                     ElementsKind kind, Variable *result, Label *exit);

    using ComparisonType = JSStableArray::ComparisonType;
    using IndexOfReturnType = JSStableArray::IndexOfReturnType;
    using IndexOfOptions = JSStableArray::IndexOfOptions;

    enum class StringElementsCondition {
        MUST_BE_STRING,
        MAY_BE_HOLE,
        MAY_BE_ANY,
    };

    static IndexOfOptions MakeIndexOfOptions(BuiltinsStubCSigns::ID id, bool resultIsTagged);
    Variable MakeResultVariableDefaultNotFound(IndexOfOptions options);
    // Result is int64 which falls in range
    //      [ 0, len]     when reversedOrder == false (i.e. indexOf, includes)
    //   or [-1, len - 1] when reversedOrder == true  (i.e. lastIndexOf)
    GateRef MakeFromIndex(GateRef input, GateRef len, bool reversedOrder);
    // For all the IndexOf* members below: fromIndex is int64; len is int32.
    GateRef IndexOfTaggedUndefined(
        GateRef glue, GateRef elements, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedZero(
        GateRef glue, GateRef elements, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedIntElements(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedIntTarget(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfTaggedNumber(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len,
        IndexOfOptions options, bool targetIsAlwaysDouble);
    GateRef IndexOfStringElements(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len,
        IndexOfOptions options, StringElementsCondition condition);
    GateRef IndexOfStringTarget(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfBigInt(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    // Including true, false, null.
    GateRef IndexOfObject(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfBigIntOrObjectElements(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfBigIntOrObjectTarget(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);
    GateRef IndexOfGeneric(
        GateRef glue, GateRef elements, GateRef target, GateRef fromIndex, GateRef len, IndexOfOptions options);

private:
    static constexpr uint32_t MAX_LENGTH_ZERO = 0;
    static constexpr uint32_t MAX_LENGTH_ONE = 1;
    static constexpr uint32_t ONE_ARGS = 1;
    static constexpr uint32_t TWO_ARGS = 2;
    static constexpr uint32_t THREE_ARGS = 3;
    static constexpr uint32_t Index0 = 0;
    static constexpr uint32_t Index1 = 1;
    static constexpr uint32_t Index2 = 2;
    struct JsArrayRequirements {
        bool stable = false;
        bool defaultConstructor = false;
    };

    GateRef IsJsArrayWithLengthLimit(GateRef glue, GateRef object,
        uint32_t maxLength, JsArrayRequirements requirements);
    GateRef CreateSpliceDeletedArray(GateRef glue, GateRef thisValue, GateRef actualDeleteCount, GateRef start);
    void DoReverse(GateRef glue, GateRef fromArray, GateRef toArray, bool holeToUndefined, bool getWithKind,
                   MemoryAttribute mAttr);
    GateRef DoReverse(GateRef glue, GateRef thisValue, GateRef receiver, GateRef receiverState, Variable *result,
                      Label *exit);
    void FastToSpliced(GateRef glue, GateRef thisValue, GateRef newArray, GateRef actualStart,
                       GateRef actualDeleteCount, GateRef insertCount, GateRef insertValue);
    void ToSplicedOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                            Label *slowPath);
    void UnshiftOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                          Label *slowPath);
    void SliceOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                        Label *slowPath);
    GateRef DoSortOptimised(GateRef glue, GateRef receiver, GateRef receiverState, Variable *result, Label *exit,
                            Label *slowPath, GateRef hir);
    GateRef DoSortOptimisedFast(GateRef glue, GateRef receiver, Variable *result, Label *exit,
                            Label *slowPath, GateRef hir);
    void ToReversedOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                             Label *slowPath);

    void FindOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);

    void FindIndexOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                            Label *slowPath);

    struct Option {
        enum MethodKind {
            MethodFind,
            MethodFindIndex,
            MethodSome,
            MethodEvery,
            MethodForEach,
        };
        enum CompatibleMode {
            Standard,
            Compatible5_0_0,
        };
        MethodKind kind;
        CompatibleMode mode;
    };

    void FindOrFindIndex(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                         Label *slowPath, const Option option);

    void EveryOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);

    void ForEachOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                          Label *slowPath);

    void SomeOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);

    void VisitAll(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                  Label *slowPath, const Option option);
    void ConcatOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                         Label *slowPath);
    void DoConcat(GateRef glue, GateRef thisValue, GateRef arg0, Variable *result, Label *exit, GateRef thisLen,
                  GateRef argLen, GateRef sumArrayLen);
    void FillOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);
    void FastFill(GateRef glue, GateRef element, GateRef start, GateRef count, GateRef value, bool needBarrier);
    void PopOptimised(GateRef glue, GateRef thisValue,
                      GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
    void ReverseOptimised(GateRef glue, GateRef thisValue, GateRef thisLen, Variable *result, Label *exit);
    void ShiftOptimised(GateRef glue, GateRef thisValue,
                        GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
    void IndexOfOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result,
                          Label *exit, Label *slowPath, IndexOfOptions options);
    void CopyWithinOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                             Label *slowPath);
    GateRef CalEleKindForNewArrayNoHole(GateRef glue, GateRef thisValue, GateRef thisLen, GateRef actualIndex,
        GateRef insertVal);
    void FastArrayWith(GateRef glue, GateRef thisValue, GateRef newArray, GateRef actualIndex, GateRef insertValue,
                       GateRef newArrEleKind);
    void WithOptimised(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit,
                       Label *slowPath);

    // Helpers of includes, indexOf and lastIndexOf
    GateRef MakeFromIndexWithInt(GateRef intValue, GateRef length, bool reversedOrder);
    GateRef MakeFromIndexWithDouble(GateRef doubleValue, GateRef length, bool reversedOrder);
    template <class Predicate>
    GateRef IndexOfElements(GateRef glue, GateRef elements, Predicate predicate, GateRef fromIndex, GateRef len,
                            IndexOfOptions options);
    GateRef StringEqual(GateRef glue, GateRef left, GateRef right, StringElementsCondition rightCondition);
    GateRef BigIntEqual(GateRef glue, GateRef left, GateRef right);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_ARRAY_STUB_BUILDER_H
