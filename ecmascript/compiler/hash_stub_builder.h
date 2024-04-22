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

#ifndef ECMASCRIPT_COMPILER_HASH_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_HASH_STUB_BUILDER_H

#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {
class HashStubBuilder : public StubBuilder {
public:
    explicit HashStubBuilder(StubBuilder *parent, GateRef glue)
        : StubBuilder(parent), glue_(glue) {}
    ~HashStubBuilder() override = default;
    NO_MOVE_SEMANTIC(HashStubBuilder);
    NO_COPY_SEMANTIC(HashStubBuilder);
    void GenerateCircuit() override {}

    GateRef GetHash(GateRef key);
    
private:
    void CalcHashcodeForNumber(GateRef key, Variable *res, Label *exit);

    GateRef glue_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_HASH_STUB_BUILDER_H
