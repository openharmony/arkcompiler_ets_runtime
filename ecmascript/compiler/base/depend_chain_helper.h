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
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class DependChains : public ChunkObject {
public:
    struct Node {
        Node(GateRef gate, Node* next) : gate(gate), next(next) {}
        GateRef gate;
        Node *next;
    };

    struct DependChainIterator {
    public:
        DependChainIterator(Node* node) : node_(node) {}

        DependChainIterator& operator++()
        {
            ASSERT(node_ != nullptr);
            node_ = node_->next;
            return *this;
        }

        bool operator!=(const DependChainIterator& that) const
        {
            return node_ != that.node_;
        }

        GateRef GetCurrentGate()
        {
            return node_->gate;
        }
    private:
        Node* node_;
    };

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
    
    GateRef GetHeadGate()
    {
        return head_->gate;
    }

    DependChainIterator begin()
    {
        return DependChainIterator(head_);
    }

    DependChainIterator end()
    {
        return DependChainIterator(nullptr);
    }

private:
    Node *head_{nullptr};
    size_t size_ {0};
    Chunk* chunk_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_DEPEND_CHAIN_HELPER_H