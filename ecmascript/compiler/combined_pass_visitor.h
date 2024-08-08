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

#ifndef ECMASCRIPT_COMPILER_COMBINED_PASS_VISITOR_H
#define ECMASCRIPT_COMPILER_COMBINED_PASS_VISITOR_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {

class RPOVisitor {
public:
    virtual ~RPOVisitor() = default;
    virtual int32_t GetGateOrder(GateRef gate) = 0;
    virtual void SetGateOrder(GateRef gate, int32_t orderId) = 0;
    virtual void Resize(int32_t size, int32_t num) = 0;
    virtual void ReVisitGate(GateRef gate) = 0;
    virtual void LogicallyReplaceGate(GateRef gate, GateRef replacement) = 0;
    virtual void RelaxStateAndDepend(GateRef gate) = 0;
    virtual void ReplaceGate(GateRef gate, GateRef replacement) = 0;
    virtual void ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement) = 0;
};

class PassVisitor {
public:
    PassVisitor(Circuit* circuit, Chunk* chunk, RPOVisitor* visitor)
        : circuit_(circuit), acc_(circuit), chunk_(chunk), visitor_(visitor) {}
    virtual ~PassVisitor() = default;

    virtual GateRef VisitGate(GateRef gate) = 0;
    virtual void Initialize() {}
    virtual void Finalize() {}
protected:
    void ReplaceGate(GateRef gate, GateRef replacement)
    {
        visitor_->ReplaceGate(gate, replacement);
    }
    void ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement)
    {
        visitor_->ReplaceGate(gate, stateDepend, replacement);
    }
    Circuit* circuit_ {nullptr};
    GateAccessor acc_;
    Chunk* chunk_ {nullptr};
    RPOVisitor* visitor_;
};

class CombinedPassVisitor : public RPOVisitor {
public:
    CombinedPassVisitor(Circuit* circuit, bool enableLog, const std::string& name, Chunk* chunk)
        : enableLog_(enableLog), methodName_(name), circuit_(circuit), acc_(circuit),
        chunk_(chunk), workList_(chunk), changedList_(chunk), orderList_(chunk), passList_(chunk) {}
    virtual ~CombinedPassVisitor() = default;
    void AddPass(PassVisitor* pass);

    int32_t GetGateOrder(GateRef gate) override;
    void SetGateOrder(GateRef gate, int32_t orderId) override;
    void Resize(int32_t size, int32_t num) override;

    void VisitGraph();
    GateRef VisitGate(GateRef gate);
    void ReVisitGate(GateRef gate) override;
    void LogicallyReplaceGate(GateRef gate, GateRef replacement) override;
    void RelaxStateAndDepend(GateRef gate) override;
    void ReplaceGate(GateRef gate, GateRef replacement) override;
    void ReplaceGate(GateRef gate, StateDepend stateDepend, GateRef replacement) override;
    void PrintLog(const std::string& phaseName);
    void VistDependSelectorForLoop(GateRef gate);

protected:

    void VisitTopGate(Edge& current);

    void PushGate(GateRef gate, size_t index)
    {
        workList_.push_back(Edge{gate, index});
        acc_.SetMark(gate, MarkCode::VISITED);
    }

    void PushChangedGate(GateRef gate)
    {
        changedList_.push_back(gate);
        acc_.SetMark(gate, MarkCode::PREVISIT);
    }

    void PopGate(GateRef gate)
    {
        workList_.pop_back();
        acc_.SetMark(gate, MarkCode::FINISHED);
    }

    Chunk *GetChunk() const
    {
        return chunk_;
    }
    void PrintStack();

private:
    bool enableLog_ {false};
    std::string methodName_;
    Circuit* circuit_ {nullptr};
    GateAccessor acc_;
    Chunk* chunk_ {nullptr};
    ChunkDeque<Edge> workList_;
    ChunkDeque<GateRef> changedList_;
    ChunkVector<int32_t> orderList_;
    ChunkVector<PassVisitor*> passList_;
    uint32_t orderCount_ {0};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMBINED_PASS_VISITOR_H
