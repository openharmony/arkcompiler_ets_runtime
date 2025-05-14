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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_COLLATOR_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_COLLATOR_STUB_BUILDER_H
#include <ecmascript/js_collator.h>

#include "ecmascript/compiler/builtins/builtins_stubs.h"

namespace panda::ecmascript::kungfu {
class BuiltinsCollatorStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsCollatorStubBuilder(BuiltinsStubBuilder *parent, GateRef globalEnv)
        : BuiltinsStubBuilder(parent, globalEnv) {}
    ~BuiltinsCollatorStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsCollatorStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsCollatorStubBuilder);
    void GenerateCircuit() override {}
    void ResolvedOptions(GateRef glue, GateRef thisValue, GateRef numArgs, Variable* res, Label *exit, Label *slowPath);

private:
    template <typename BitType>
    GateRef GetBitField(GateRef collator);
    GateRef UsageOptionsToEcmaString(GateRef glue, GateRef usage);
    GateRef SensitivityOptionsToEcmaString(GateRef glue, GateRef sensitivity);
    GateRef CaseFirstOptionsToEcmaString(GateRef glue, GateRef sensitivity);

    static constexpr size_t SENSITIVITY_OPTION_CASES_NUM = 5;
    static int64_t SensitivityOptionCases[SENSITIVITY_OPTION_CASES_NUM];
    static ConstantIndex SensitivityOptionIndexes[SENSITIVITY_OPTION_CASES_NUM - 1];

    static constexpr size_t CASE_FIRST_OPTION_NUM = 4;
    static int64_t CaseFirstOptionCases[CASE_FIRST_OPTION_NUM];
    static ConstantIndex CaseFirstOptionIndexes[CASE_FIRST_OPTION_NUM];
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_COLLATOR_STUB_BUILDER_H