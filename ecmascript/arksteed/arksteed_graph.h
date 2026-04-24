/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ARKSTEED_GRAPH_H
#define ECMASCRIPT_ARKSTEED_GRAPH_H

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_vertex.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {

class Graph {
public:
    NO_COPY_SEMANTIC(Graph);
    NO_MOVE_SEMANTIC(Graph);

    static Graph *New(Chunk *chunk)
    {
        return chunk->New<Graph>(chunk);
    }

    explicit Graph(Chunk *chunk)
        : chunk_(chunk),
          blocks_(chunk),
          parameters_(chunk),
          rootConstants_(chunk),
          int32Constants_(chunk),
          intPtrConstants_(chunk),
          float64Constants_(chunk),
          taggedConstants_(chunk),
          maxCallStackArgs_(0),
          maxDeoptedStackSize_(0),
          taggedStackSlots_(0),
          untaggedStackSlots_(0),
          maxBlockId_(0),
          hasRecursiveCalls_(false),
          mayHaveUnreachableBlocks_(false)
    {}

    // ========================================= Constant Accessors =========================================

    ValueVertex *GetRootConstant(RootConstantVertex::RootIndex index)
    {
        return GetOrAddNewConstantVertex(rootConstants_, index);
    }

    ValueVertex *GetInt32Constant(int32_t value)
    {
        return GetOrAddNewConstantVertex(int32Constants_, value);
    }

    ValueVertex *GetIntPtrConstant(intptr_t value)
    {
        return GetOrAddNewConstantVertex(intPtrConstants_, value);
    }

    ValueVertex *GetFloat64Constant(double value)
    {
        return GetOrAddNewConstantVertex(float64Constants_, value);
    }

    ValueVertex *GetTaggedConstant(uint64_t value)
    {
        return GetOrAddNewConstantVertex(taggedConstants_, value);
    }

    const ChunkMap<RootConstantVertex::RootIndex, RootConstantVertex *> &GetRootConstants() const
    {
        return rootConstants_;
    }

    const ChunkMap<int32_t, Int32ConstantVertex *> &GetInt32Constants() const
    {
        return int32Constants_;
    }

    const ChunkMap<intptr_t, IntPtrConstantVertex *> &GetIntPtrConstants() const
    {
        return intPtrConstants_;
    }

    const ChunkMap<double, Float64ConstantVertex *> &GetFloat64Constants() const
    {
        return float64Constants_;
    }

    const ChunkMap<uint64_t, TaggedConstantVertex *> &GetTaggedConstants() const
    {
        return taggedConstants_;
    }

    BB *operator[](int i)
    {
        return blocks_[i];
    }

    const BB *operator[](int i) const
    {
        return blocks_[i];
    }

    int NumBlocks() const
    {
        return static_cast<int>(blocks_.size());
    }

    void Add(BB *block)
    {
        if (block->GetId() == INVALID_BLOCK_ID) {
            block->SetId(maxBlockId_++);
        }
        blocks_.push_back(block);
    }

    void RemoveUnreachableBlocks();

    using BlockIterator = ChunkVector<BB *>::iterator;
    using ConstBlockIterator = ChunkVector<BB *>::const_iterator;

    BlockIterator begin()
    {
        return blocks_.begin();
    }

    BlockIterator end()
    {
        return blocks_.end();
    }

    ConstBlockIterator begin() const
    {
        return blocks_.begin();
    }

    ConstBlockIterator end() const
    {
        return blocks_.end();
    }

    BB *LastBlock() const
    {
        return blocks_.back();
    }

    // Parameters
    void AddParameter(ValueVertex *param)
    {
        parameters_.push_back(param);
    }

    ValueVertex *GetParameter(int index) const
    {
        if (index < 0 || index >= static_cast<int>(parameters_.size())) {
            return nullptr;
        }
        return parameters_[index];
    }

    size_t GetParameterCount() const
    {
        return parameters_.size();
    }

    // Block ID management
    uint32_t NewBlockId()
    {
        return maxBlockId_++;
    }

    uint32_t MaxBlockId() const
    {
        return maxBlockId_;
    }

    // Flags
    bool HasRecursiveCalls() const
    {
        return hasRecursiveCalls_;
    }

    void SetHasRecursiveCalls(bool value)
    {
        hasRecursiveCalls_ = value;
    }

    bool MayHaveUnreachableBlocks() const
    {
        return mayHaveUnreachableBlocks_;
    }

    void SetMayHaveUnreachableBlocks(bool value)
    {
        mayHaveUnreachableBlocks_ = value;
    }

    Chunk *GetChunk() const
    {
        return chunk_;
    }

    // Max call stack args for code generation
    void SetMaxCallStackArgs(int args)
    {
        maxCallStackArgs_ = args;
    }

    int GetMaxCallStackArgs() const
    {
        return maxCallStackArgs_;
    }

    void SetMaxDeoptedStackSize(int size)
    {
        maxDeoptedStackSize_ = size;
    }

    int GetMaxDeoptedStackSize() const
    {
        return maxDeoptedStackSize_;
    }

    void SetTaggedStackSlots(uint32_t slots)
    {
        taggedStackSlots_ = slots;
    }

    uint32_t GetTaggedStackSlots() const
    {
        return taggedStackSlots_;
    }

    void SetUntaggedStackSlots(uint32_t slots)
    {
        untaggedStackSlots_ = slots;
    }

    uint32_t GetUntaggedStackSlots() const
    {
        return untaggedStackSlots_;
    }

    // Debugging
    void Print() const;

private:
    Chunk *chunk_;
    ChunkVector<BB *> blocks_;
    ChunkVector<ValueVertex *> parameters_;
    ChunkMap<RootConstantVertex::RootIndex, RootConstantVertex *> rootConstants_;
    ChunkMap<int32_t, Int32ConstantVertex *> int32Constants_;
    ChunkMap<intptr_t, IntPtrConstantVertex *> intPtrConstants_;
    ChunkMap<double, Float64ConstantVertex *> float64Constants_;
    ChunkMap<uint64_t, TaggedConstantVertex *> taggedConstants_;
    int maxCallStackArgs_ = 0;
    int maxDeoptedStackSize_ = 0;
    uint32_t taggedStackSlots_ = 0;
    uint32_t untaggedStackSlots_ = 0;

    template <typename VertexT, typename T>
    VertexT *GetOrAddNewConstantVertex(ChunkMap<T, VertexT *> &container, T constant)
    {
        auto it = container.find(constant);
        if (it != container.end()) {
            return it->second;
        }
        VertexT *vertex = Vertex::New<VertexT>(chunk_, 0, constant);
        ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
        if (labeller != nullptr) {
            labeller->RegisterVertex(vertex);
        }
        container.emplace(constant, vertex);
        return vertex;
    }

    uint32_t maxBlockId_;
    bool hasRecursiveCalls_;
    bool mayHaveUnreachableBlocks_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_H
