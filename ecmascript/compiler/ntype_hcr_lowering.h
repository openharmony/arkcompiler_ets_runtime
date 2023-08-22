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

#ifndef ECMASCRIPT_COMPILER_NTYPE_HCR_LOWERING_H
#define ECMASCRIPT_COMPILER_NTYPE_HCR_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {
class NTypeHCRLowering : public PassVisitor {
public:
    NTypeHCRLowering(Circuit *circuit, RPOVisitor *visitor, PassContext *ctx, TSManager *tsManager,
                    const MethodLiteral *methodLiteral, const CString &recordName, Chunk *chunk)
        : PassVisitor(circuit, chunk, visitor),
          circuit_(circuit),
          acc_(circuit),
          builder_(circuit, ctx->GetCompilerConfig()),
          recordName_(recordName),
          tsManager_(tsManager),
          jsPandaFile_(ctx->GetJSPandaFile()),
          methodLiteral_(methodLiteral),
          profiling_(ctx->GetCompilerConfig()->IsProfiling()),
          traceBc_(ctx->GetCompilerConfig()->IsTraceBC()),
          glue_(acc_.GetGlueFromArgList()) {}

    ~NTypeHCRLowering() = default;

    GateRef VisitGate(GateRef gate) override;
private:
    void Lower(GateRef gate);
    void LowerNTypedCreateEmptyArray(GateRef gate);
    void LowerNTypedCreateArrayWithBuffer(GateRef gate);
    void LowerNTypedStownByIndex(GateRef gate);
    void LowerNTypedStOwnByName(GateRef gate);
    void LowerLdLexVar(GateRef gate);
    void LowerStLexVar(GateRef gate);
    void LowerThrowUndefinedIfHoleWithName(GateRef gate);
    uint64_t GetBcAbsoluteOffset(GateRef gate) const;

    bool IsProfiling() const
    {
        return profiling_;
    }

    bool IsTraceBC() const
    {
        return traceBc_;
    }

    void AddProfiling(GateRef gate);
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    const CString &recordName_;
    TSManager *tsManager_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    const MethodLiteral *methodLiteral_ {nullptr};
    bool profiling_ {false};
    bool traceBc_ {false};
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NTYPE_HCR_LOWERING_H
