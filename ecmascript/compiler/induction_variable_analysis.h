/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_INDUCTION_VARIABLE_ANALYSIS_H
#define ECMASCRIPT_COMPILER_INDUCTION_VARIABLE_ANALYSIS_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {
class InductionVariableAnalysis {
public:
    InductionVariableAnalysis(Circuit* circuit, PassContext* ctx, bool enableLog,
                              const std::string& name, Chunk* chunk, bool isTraced)
        : enableLog_(enableLog), methodName_(name), circuit_(circuit),
          builder_(circuit, ctx->GetCompilerConfig()), acc_(circuit),
          graphLinearizer_(circuit, enableLog, name, chunk, false, true), isTraced_(isTraced) {}
    
    void Run();
private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsTraced() const
    {
        return isTraced_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }
    bool IsIntConstant(GateRef gate) const;
    bool IsInductionVariable(GateRef gate) const;
    bool IsLessOrGreaterCmp(GateRef gate) const;
    std::pair<int32_t, int32_t> GetStartAndStride(GateRef gate) const;
    bool TryGetLoopTimes(const GraphLinearizer::LoopInfo& loop, int32_t& loopTimes) const;
    int32_t GetIntFromTaggedConstant(GateRef gate) const;
    void CollectInductionSelector();
    void TryReplaceOutOfLoopUses(GateRef gate, const GraphLinearizer::LoopInfo& loop, const int32_t result);
    void ReplaceInductionVariable(const GraphLinearizer::LoopInfo& loop, const int32_t loopTimes);
    bool enableLog_ {false};
    std::string methodName_;
    Circuit* circuit_ {nullptr};
    CircuitBuilder builder_;
    GateAccessor acc_;
    GraphLinearizer graphLinearizer_;
    bool isTraced_;
};

}
#endif