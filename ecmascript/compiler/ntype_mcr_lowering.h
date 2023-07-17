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

#ifndef ECMASCRIPT_COMPILER_NTYPE_MCR_LOWERING_H
#define ECMASCRIPT_COMPILER_NTYPE_MCR_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/pass_manager.h"
namespace panda::ecmascript::kungfu {
class NTypeMCRLowering {
public:
    NTypeMCRLowering(Circuit *circuit, PassContext *ctx, TSManager *tsManager,
                     bool enableLog, const std::string& name)
        : circuit_(circuit),
          acc_(circuit),
          builder_(circuit, ctx->GetCompilerConfig()),
          dependEntry_(circuit->GetDependRoot()),
          tsManager_(tsManager),
          enableLog_(enableLog),
          profiling_(ctx->GetCompilerConfig()->IsProfiling()),
          traceBc_(ctx->GetCompilerConfig()->IsTraceBC()),
          methodName_(name),
          glue_(acc_.GetGlueFromArgList()) {}

    ~NTypeMCRLowering() = default;

    void RunNTypeMCRLowering();
private:
    static constexpr int MAX_TAGGED_ARRAY_LENGTH = 50;
    void Lower(GateRef gate);
    void LowerCreateArray(GateRef gate, GateRef glue);
    void LowerCreateEmptyArray(GateRef gate);
    void LowerCreateArrayWithOwn(GateRef gate, GateRef glue);
    void LowerStLexVar(GateRef gate);
    void LowerLdLexVar(GateRef gate);

    GateRef NewTaggedArray(size_t length);
    GateRef LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                             bool useLabel = false);

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
    bool profiling_ {false};
    bool traceBc_ {false};
    std::string methodName_;
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NTYPE_MCR_LOWERING_H
