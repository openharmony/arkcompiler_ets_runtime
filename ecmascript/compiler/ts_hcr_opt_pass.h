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

#include "ecmascript/compiler/graph_visitor.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {
class TSHCROptPass : public GraphVisitor {
public:
    TSHCROptPass(Circuit* circuit, Chunk* chunk, PassContext *ctx, bool enableLog, const std::string &name)
        : GraphVisitor(circuit, chunk),
          builder_(circuit, ctx->GetCompilerConfig()),
          tsManager_(ctx->GetTSManager()),
          thread_(ctx->GetEcmaVM()->GetJSThread()),
          enableLog_(enableLog),
          methodName_(name) {}

    ~TSHCROptPass() = default;

    void Run();
    GateRef VisitGate(GateRef gate) override;

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    GateRef VisitTypedBinaryOp(GateRef gate);

    GateRef VisitStringBinOp(GateRef gate);
    GateRef VisitStringEqual(GateRef gate);
    bool IsSingleCharString(GateRef gate);
    GateRef ConvertStringEqualToConst(GateRef left, GateRef right);
    GateRef ConvertConstSingleCharToInt32(GateRef gate);
    GateRef ConvertToSingleCharComparison(GateRef left, GateRef right);

    CircuitBuilder builder_;
    TSManager *tsManager_ {nullptr};
    const JSThread *thread_ {nullptr};
    bool enableLog_ {false};
    std::string methodName_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_HCR_OPT_PASS_H
