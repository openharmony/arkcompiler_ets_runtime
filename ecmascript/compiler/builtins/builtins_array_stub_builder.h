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

namespace panda::ecmascript::kungfu {
class BuiltinsArrayStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsArrayStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    explicit BuiltinsArrayStubBuilder(Environment* env): BuiltinsStubBuilder(env) {}
    ~BuiltinsArrayStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsArrayStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsArrayStubBuilder);
    void GenerateCircuit() override {}

#define DECLARE_BUILTINS_ARRAY_STUB_BUILDER(method, ...)           \
    void method(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);
BUILTINS_WITH_ARRAY_STUB_BUILDER(DECLARE_BUILTINS_ARRAY_STUB_BUILDER)
#undef DECLARE_BUILTINS_ARRAY_STUB_BUILDER

    void Sort(GateRef glue, GateRef thisValue, GateRef numArgs, Variable *result, Label *exit, Label *slowPath);

    void SortAfterArgs(GateRef glue, GateRef thisValue, GateRef callbackFnHandle,
                       Variable *result, Label *exit, Label *slowPath, GateRef hir = Circuit::NullGate());

    void GenArrayConstructor(GateRef glue, GateRef nativeCode, GateRef func,
        GateRef newTarget, GateRef thisValue, GateRef numArgs);

    void FastCreateArrayWithArgv(GateRef glue, Variable *res, GateRef argc, GateRef hclass, Label *exit);
    
    void ElementsKindHclassCompare(GateRef glue, GateRef arrayCls, Label *matchCls, Label *slowPath);

    GateRef IsConcatSpreadable(GateRef glue, GateRef obj);

    GateRef NewArray(GateRef glue, GateRef count);

    GateRef CalculatePositionWithLength(GateRef position, GateRef length);

    GateRef DoReverse(GateRef glue, GateRef thisValue, GateRef receiver, GateRef receiverState,
        Variable *result, Label *exit);

    GateRef DoSort(GateRef glue, GateRef receiver, GateRef receiverState,
        Variable *result, Label *exit, Label *slowPath, GateRef hir = Circuit::NullGate());

    void FastReverse(GateRef glue, GateRef thisValue, GateRef len,
                     ElementsKind kind, Variable *result, Label *exit);
private:
    static constexpr uint32_t MAX_LENGTH_ZERO = 0;
    static constexpr uint32_t MAX_LENGTH_ONE = 1;
    struct JsArrayRequirements {
        bool stable = false;
        bool defaultConstructor = false;
    };
    GateRef IsJsArrayWithLengthLimit(GateRef glue, GateRef object,
        uint32_t maxLength, JsArrayRequirements requirements);
    GateRef CreateSpliceDeletedArray(GateRef glue, GateRef thisValue, GateRef actualDeleteCount,
        GateRef intialHClass, GateRef start);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_ARRAY_STUB_BUILDER_H
