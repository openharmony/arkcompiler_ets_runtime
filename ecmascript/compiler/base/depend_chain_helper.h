/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_DEPEND_CHAIN_HELPER_H
#define ECMASCRIPT_COMPILER_DEPEND_CHAIN_HELPER_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/later_elimination.h"
#include "ecmascript/compiler/range_guard.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class LaterElimination;
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
    uint32_t FoundIndexCheckedForLength(RangeGuard* rangeGuard, GateRef input);
    uint32_t FoundIndexCheckedForIndex(RangeGuard* rangeGuard, GateRef input);
    GateRef LookupNode(LaterElimination* elimination, GateRef gate);
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
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_DEPEND_CHAIN_HELPER_H