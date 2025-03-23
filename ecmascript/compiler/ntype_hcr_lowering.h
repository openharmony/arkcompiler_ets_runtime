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

#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/jspandafile/program_object.h"

namespace panda::ecmascript::kungfu {
class NTypeHCRLowering : public PassVisitor {
public:
    NTypeHCRLowering(Circuit *circuit, RPOVisitor *visitor, PassContext *ctx, const CString &recordName,
                     const MethodLiteral *methodLiteral, bool enablePgoSpace, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor),
          circuit_(circuit),
          acc_(circuit),
          compilationEnv_(ctx->GetCompilationEnv()),
          builder_(circuit, ctx->GetCompilerConfig()),
          dependEntry_(circuit->GetDependRoot()),
          jsPandaFile_(ctx->GetJSPandaFile()),
          recordName_(recordName),
          methodLiteral_(methodLiteral),
          profiling_(ctx->GetCompilerConfig()->IsProfiling()),
          enablePgoSpace_(enablePgoSpace),
          traceBc_(ctx->GetCompilerConfig()->IsTraceBC()),
          glue_(acc_.GetGlueFromArgList()) {}

    ~NTypeHCRLowering() = default;
    GateRef VisitGate(GateRef gate) override;
private:
    static constexpr int MAX_TAGGED_ARRAY_LENGTH = 50;
    void Lower(GateRef gate);
    void LowerCreateArray(GateRef gate, GateRef glue);
    void LowerCreateArrayWithBuffer(GateRef gate, GateRef glue);
    void LowerCreateArguments(GateRef gate, GateRef glue);
    void LowerCreateEmptyArray(GateRef gate, GateRef glue);
    void LowerCreateArrayWithOwn(GateRef gate, GateRef glue);
    void LowerStoreModuleVar(GateRef gate, GateRef glue);
    void LowerLdLocalModuleVar(GateRef gate);

    GateRef LoadFromConstPool(GateRef unsharedConstPool, size_t index, size_t valVecType);
    GateRef NewActualArgv(GateRef gate, GateRef glue);
    GateRef NewJSArrayLiteral(
        GateRef glue, GateRef gate, GateRef elements, GateRef length, uint32_t hintLength = 0);
    GateRef NewTaggedArray(size_t length, GateRef glue);
    GateRef CreateElementsWithLength(GateRef gate, GateRef glue, size_t arrayLength);
    GateRef LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                             bool useLabel = false);
    void ReplaceGateWithPendingException(GateRef gate, GateRef state, GateRef depend, GateRef value);

    GateRef GetFrameState(GateRef gate) const
    {
        return acc_.GetFrameState(gate);
    }

    JSTaggedValue GetConstantpoolValue(uint32_t cpId)
    {
        return compilationEnv_->FindConstpool(jsPandaFile_, cpId);
    }

    JSTaggedValue GetArrayLiteralValue(uint32_t cpId, uint32_t cpIdx)
    {
        JSTaggedValue cp = GetConstantpoolValue(cpId);
        JSTaggedValue unsharedCp = compilationEnv_->FindOrCreateUnsharedConstpool(cp);
        return compilationEnv_->GetArrayLiteralFromCache(unsharedCp, cpIdx, recordName_);
    }

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CompilationEnv *compilationEnv_ {nullptr};
    CircuitBuilder builder_;
    GateRef dependEntry_;
    const JSPandaFile *jsPandaFile_ {nullptr};
    const CString &recordName_;
    const MethodLiteral *methodLiteral_ {nullptr};
    panda_file::File::EntityId methodId_ {0};
    bool profiling_ {false};
    bool enablePgoSpace_ {false};
    bool traceBc_ {false};
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NTYPE_HCR_LOWERING_H
