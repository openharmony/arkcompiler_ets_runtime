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


#ifndef ECMASCRIPT_LEXICAL_ENV_SPECIALIZATION_H
#define ECMASCRIPT_LEXICAL_ENV_SPECIALIZATION_H

#include "ecmascript/compiler/base/depend_chain_helper.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/combined_pass_visitor.h"

namespace panda::ecmascript::kungfu {
class DependChains;
class LexicalEnvSpecializationPass : public PassVisitor {
public:
    LexicalEnvSpecializationPass(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk, bool enableLog)
        : PassVisitor(circuit, chunk, visitor), chunk_(chunk), dependChains_(chunk), specializeId_(chunk),
        notdomStlexvar_(chunk), notDomCall_(chunk_), enableLog_(enableLog), acc_(circuit) {}

    ~LexicalEnvSpecializationPass() = default;

    void Initialize() override;
    GateRef VisitGate(GateRef gate) override;
    bool SearchStLexVar(GateRef gate, GateRef ldLexVar, GateRef &result);
    void PrintSpecializeId();
private:
    DependChains* GetDependChain(GateRef dependIn)
    {
        size_t idx = acc_.GetId(dependIn);
        ASSERT(idx <= circuit_->GetMaxGateId());
        return dependChains_[idx];
    }

    GateRef VisitDependEntry(GateRef gate);
    GateRef UpdateDependChain(GateRef gate, DependChains* dependInfo);
    GateRef TrySpecializeLdLexVar(GateRef gate);
    GateRef VisitOther(GateRef gate);
    GateRef VisitDependSelector(GateRef gate);
    bool CheckStLexVar(GateRef gate, GateRef ldLexVar);
    bool caclulateDistanceToTarget(GateRef startEnv, GateRef targetEnv, int32_t &dis);
    void HasNotdomStLexVarOrCall(GateRef gate, GateRef next);
    bool HasNotDomStLexvar(GateRef gate);
    bool HasNotDomCall(GateRef gate);
    bool HasNotDomIllegalOp(GateRef gate);
    void LookUpNotDomStLexVarOrCall(GateRef current, GateRef next);
    GateRef LookupStLexvarNode(DependChains* dependChain, GateRef gate);

    Chunk *chunk_ {nullptr};
    ChunkVector<DependChains*> dependChains_;
    ChunkVector<size_t> specializeId_;
    ChunkMap<GateRef, GateRef> notdomStlexvar_;
    ChunkMap<GateRef, GateRef> notDomCall_;
    bool enableLog_ {false};
    GateAccessor acc_;
};

class GetEnvSpecializationPass : public PassVisitor {
public:
    GetEnvSpecializationPass(Circuit* circuit, RPOVisitor* visitor, Chunk* chunk)
        : PassVisitor(circuit, chunk, visitor),
          acc_(circuit) {}
    ~GetEnvSpecializationPass() = default;

    GateRef VisitGate(GateRef gate) override;
private:
    GateRef TryGetReplaceEnv(GateRef func);

    GateAccessor acc_;
};
}
#endif // ECMASCRIPT_LEXICAL_ENV_SPECIALIZATION_H