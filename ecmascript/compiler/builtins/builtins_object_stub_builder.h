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
    ~BuiltinsObjectStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsObjectStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsObjectStubBuilder);
    void GenerateCircuit() override {}
    GateRef CreateListFromArrayLike(GateRef glue, GateRef arrayObj);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_OBJECT_STUB_BUILDER_H