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

#ifndef ECMASCRIPT_COMPILER_TS_HCR_OPT_PASS_H
#define ECMASCRIPT_COMPILER_TS_HCR_OPT_PASS_H

#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/ecma_string-inl.h"

namespace panda::ecmascript::kungfu {
class TSHCROptPass : public PassVisitor {
public:
    TSHCROptPass(Circuit* circuit,
                 RPOVisitor *visitor,
                 Chunk* chunk,
                 PassContext *ctx,
                 bool enableLog,
                 const std::string &name)
        : PassVisitor(circuit, chunk, visitor),
          builder_(circuit, ctx->GetCompilerConfig()),
          compilationEnv_(ctx->GetCompilationEnv()),
          enableLog_(enableLog),
          methodName_(name)
    {
        if (ctx->GetCompilerConfig() != nullptr) {
            typedOpProfiling_ = ctx->GetCompilerConfig()->IsTypedOpProfiling();
        }
    }

    ~TSHCROptPass() = default;

    GateRef VisitGate(GateRef gate) override;

private:
    void AddProfiling(GateRef gate);

    bool IsTypedOpProfiling() const
    {
        return typedOpProfiling_;
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    JSTaggedValue GetStringFromConstantPool(uint32_t methodOffset, uint32_t cpIdx) const
    {
        return compilationEnv_->GetStringFromConstantPool(methodOffset, cpIdx);
    }

    GateRef VisitTypedBinaryOp(GateRef gate);

    GateRef VisitStringBinOp(GateRef gate);
    GateRef VisitStringEqual(GateRef gate);
    bool IsSingleCharString(GateRef gate);
    bool IsNotLoadStrOrStringLoadElement(GateRef gate);
    GateRef ConvertStringEqualToConst(GateRef left, GateRef right);
    GateRef ConvertConstSingleCharToInt32(GateRef gate);
    GateRef ConvertToSingleCharComparison(GateRef left, GateRef right);

    CircuitBuilder builder_;
    const CompilationEnv *compilationEnv_ {nullptr};
    bool enableLog_ {false};
    std::string methodName_;
    bool typedOpProfiling_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_HCR_OPT_PASS_H
