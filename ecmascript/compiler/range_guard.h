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

#ifndef ECMASCRIPT_COMPILER_RANGE_GUARD_H
#define ECMASCRIPT_COMPILER_RANGE_GUARD_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_visitor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class RangeGuard;

class DependChains : public ChunkObject {
public:
    DependChains(Chunk* chunk) : chunk_(chunk) {}
    ~DependChains() = default;

    DependChains* UpdateNode(GateRef gate);
    bool Equals(DependChains* that);
    void Merge(DependChains* that);
    void CopyFrom(DependChains *other)
    {
        head_ = other->head_;
        size_ = other->size_;
    }
    bool FoundIndexChecked(RangeGuard* rangeGuard, GateRef input);
private:
    struct Node {
        Node(GateRef gate, Node* next) : gate(gate), next(next) {}
        GateRef gate;
        Node *next;
    };

    Node *head_{nullptr};
    size_t size_ {0};
    Chunk* chunk_;
};

class RangeGuard : public GraphVisitor {
public:
    RangeGuard(Circuit *circuit, bool enableLog, const std::string& name, Chunk* chunk, PassContext *ctx)
        : GraphVisitor(circuit, chunk), circuit_(circuit), builder_(circuit, ctx->GetCompilerConfig()), enableLog_(enableLog),
        methodName_(name), dependChains_(chunk) {}

    ~RangeGuard() = default;

    void Run();

    GateRef VisitGate(GateRef gate) override;
    bool CheckInputSource(GateRef lhs, GateRef rhs);
    bool CheckIndexCheckInput(GateRef lhs, GateRef rhs);
private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    DependChains* GetDependChain(GateRef dependIn)
    {
        size_t idx = acc_.GetId(dependIn);
        ASSERT(idx <= circuit_->GetMaxGateId());
        return dependChains_[idx];
    }

    GateRef VisitDependEntry(GateRef gate);
    GateRef UpdateDependChain(GateRef gate, DependChains* dependInfo);
    GateRef TryApplyTypedArrayRangeGuard(DependChains* dependInfo, GateRef gate, GateRef input);
    GateRef TryApplyRangeGuardGate(GateRef gate);
    GateRef TraverseOthers(GateRef gate);
    GateRef TraverseDependSelector(GateRef gate);

    Circuit* circuit_;
    CircuitBuilder builder_;
    bool enableLog_ {false};
    std::string methodName_;
    ChunkVector<DependChains*> dependChains_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_RANGE_GUARD_H