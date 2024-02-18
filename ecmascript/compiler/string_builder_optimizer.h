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

#ifndef ECMASCRIPT_COMPILER_STRING_BUILDER_OPTIMIZER_H
#define ECMASCRIPT_COMPILER_STRING_BUILDER_OPTIMIZER_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/graph_linearizer.h"

namespace panda::ecmascript::kungfu {
class StringBuilderOptimizer {
public:
    StringBuilderOptimizer(Circuit* circuit,
                           bool enableLog,
                           const std::string& name,
                           CompilationConfig* cmpCfg,
                           Chunk* chunk)
        : circuit_(circuit),
          acc_(circuit),
          builder_(circuit, cmpCfg),
          graphLinearizer_(circuit, enableLog, name, chunk, true, true),
          dependEntry_(circuit->GetDependRoot()),
          status_(chunk),
          stringBuilders_(chunk),
          toVisit_(chunk),
          ends_(chunk),
          concatGates_(chunk) {}

    ~StringBuilderOptimizer() = default;

    void Run();
private:
    static constexpr int INVALID_ID = -1;
    enum State {
        UNVISITED = 0,
        BEGIN_STRING_BUILDER,
        IN_STRING_BUILDER,
        PENDING_PHI,
        CONFIRMED_IN_STRING_BUILDER,
        END_STRING_BUILDER,
        END_STRING_BUILDER_LOOP_PHI,
        INVALID_OPT,
        NUM_OF_STATE,
    };

    struct Status {
        int id {INVALID_ID};
        State state {State::INVALID_OPT};
    };

    Status& GetStatus(GateRef gate)
    {
        ASSERT(acc_.GetId(gate) < status_.size());
        return status_[acc_.GetId(gate)];
    }
    const Status& GetStatus(GateRef gate) const
    {
        ASSERT(acc_.GetId(gate) < status_.size());
        return status_[acc_.GetId(gate)];
    }
    void SetStatus(GateRef gate, State state, int id = INVALID_ID)
    {
        status_[acc_.GetId(gate)] = Status{id, state};
    }
    void UpdateStatus(GateRef gate, State state)
    {
        int id = state == State::INVALID_OPT ? INVALID_ID : GetStatus(gate).id;
        status_[acc_.GetId(gate)] = Status{id, state};
    }
    bool IsInvalidGate(GateRef gate) const
    {
        return GetStatus(gate).state == State::INVALID_OPT;
    }

    struct StringBuilder {
        GateRef start {Circuit::NullGate()};
        int id {INVALID_ID};
        bool has_loop_phi {false};
    };

    void VisitGraph();
    void FindBuilderBegin(GateRef gate);
    void FindInBuilder(GateRef gate);
    void FinalizeStringBuilders();
    void VisitGateUse(GateRef use);
    void StatusTransfer(GateRef gate);
    bool HasConcatOrPhiUse(GateRef gate);
    bool CheckStringAddUses(GateRef gate);
    bool IsLiteralString(GateRef gate);
    bool IsLoopHeader(GateRef gate);
    bool LoopContains(GateRef loopPhi, GateRef gate);
    bool PhiInputsAreConcatsOrPhi(GateRef phi);

    Circuit *circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    GraphLinearizer graphLinearizer_;
    GateRef dependEntry_;

    uint32_t stringBuilderCount_ {0};
    size_t currentIndex_ {0};
    GateId curStringAddId_ {0};

    ChunkVector<Status> status_;
    ChunkVector<StringBuilder> stringBuilders_;
    ChunkVector<GateRef> toVisit_;
    ChunkVector<GateRef> ends_;
    ChunkVector<GateRef> concatGates_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STRING_BUILDER_OPTIMIZER_H