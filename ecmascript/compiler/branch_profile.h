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

#ifndef ECMASCRIPT_COMPILER_BRANCH_PROFILE_H
#define ECMASCRIPT_COMPILER_BRANCH_PROFILE_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {

class BranchProfile : public PassVisitor {
public:
    BranchProfile(Circuit *circuit, PassContext *ctx, RPOVisitor *visitor, Chunk *chunk)
        : PassVisitor(circuit, chunk, visitor), circuit_ (circuit),
          builder_(circuit, ctx->GetCompilerConfig()) {}
    ~BranchProfile() = default;
    GateRef VisitGate(GateRef gate) override;
    void AddBranchProfiling(GateRef gate);
private:
    Circuit *circuit_;
    [[maybe_unused]] CircuitBuilder builder_;
    [[maybe_unused]] GateRef glue_ { Circuit::NullGate() };
};

}

#endif