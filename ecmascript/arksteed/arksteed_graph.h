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
          int32Constants_(chunk),
          int64Constants_(chunk),
          float64Constants_(chunk),
          taggedConstants_(chunk),
          heapConstants_(chunk),
          maxCallStackArgs_(0),
          taggedStackSlots_(0),
          untaggedStackSlots_(0),
          maxBlockId_(0),
          hasRecursiveCalls_(false),
          mayHaveUnreachableBlocks_(false)
    {}

    // ========================================= Constant Accessors =========================================

    ValueVertex *GetInt32Constant(int32_t value)
    {
        return GetOrAddNewConstantVertex(int32Constants_, value);
    }

    ValueVertex *GetInt64Constant(int64_t value)
    {
        return GetOrAddNewConstantVertex(int64Constants_, value);
    }

    // TODO: adaptation for 32-bit platform — forwards to GetInt64Constant for now
    ValueVertex *GetIntPtrConstant(intptr_t value)
    {
        return GetInt64Constant(static_cast<int64_t>(value));
    }

    ValueVertex *GetFloat64Constant(double value)
    {
        return GetOrAddNewConstantVertex(float64Constants_, value);
    }

    ValueVertex *GetTaggedConstant(uint64_t value)
    {
        return GetOrAddNewConstantVertex(taggedConstants_, value);
    }

    ValueVertex *GetHeapConstant(uint32_t handleIndex, uint16_t staticNodeType)
    {
        auto it = heapConstants_.find(handleIndex);
        if (it != heapConstants_.end()) {
            ASSERT(it->second->GetStaticNodeType() == staticNodeType);
            return it->second;
        }
        HeapConstantVertex *vertex = Vertex::New<HeapConstantVertex>(chunk_, 0, handleIndex, staticNodeType);
        heapConstants_.emplace(handleIndex, vertex);
        return vertex;
    }

    const ChunkMap<int32_t, Int32ConstantVertex *> &GetInt32Constants() const
    {
        return int32Constants_;
    }

    const ChunkMap<int64_t, Int64ConstantVertex *> &GetInt64Constants() const
    {
        return int64Constants_;
    }

    // TODO: adaptation for 32-bit platform — forwards to GetInt64Constants for now
    const ChunkMap<int64_t, Int64ConstantVertex *> &GetIntPtrConstants() const
    {
        return GetInt64Constants();
    }

    const ChunkMap<double, Float64ConstantVertex *> &GetFloat64Constants() const
    {
        return float64Constants_;
    }

    const ChunkMap<uint64_t, TaggedConstantVertex *> &GetTaggedConstants() const
    {
        return taggedConstants_;
    }

    const ChunkMap<uint32_t, HeapConstantVertex *> &GetHeapConstants() const
    {
        return heapConstants_;
    }

    BB *operator[](uint32_t i)
    {
        return blocks_[i];
    }

    const BB *operator[](uint32_t i) const
    {
        return blocks_[i];
    }

    uint32_t NumBlocks() const
    {
        return static_cast<uint32_t>(blocks_.size());
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

    ValueVertex *GetParameter(uint32_t index) const
    {
        if (index >= parameters_.size()) {
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
    void SetMaxCallStackArgs(uint32_t args)
    {
        maxCallStackArgs_ = args;
    }

    uint32_t GetMaxCallStackArgs() const
    {
        return maxCallStackArgs_;
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

    void SetReuseStackSlots(bool reuse)
    {
        reuseStackSlots_ = reuse;
    }

    bool GetReuseStackSlots() const
    {
        return reuseStackSlots_;
    }

private:
    template <typename VertexT, typename T>
    VertexT *GetOrAddNewConstantVertex(ChunkMap<T, VertexT *> &container, T constant)
    {
        auto it = container.find(constant);
        if (it != container.end()) {
            return it->second;
        }
        VertexT *vertex = Vertex::New<VertexT>(chunk_, 0, constant);
        container.emplace(constant, vertex);
        return vertex;
    }

    Chunk *chunk_;
    ChunkVector<BB *> blocks_;
    ChunkVector<ValueVertex *> parameters_;
    ChunkMap<int32_t, Int32ConstantVertex *> int32Constants_;
    ChunkMap<int64_t, Int64ConstantVertex *> int64Constants_;
    ChunkMap<double, Float64ConstantVertex *> float64Constants_;
    ChunkMap<uint64_t, TaggedConstantVertex *> taggedConstants_;
    ChunkMap<uint32_t, HeapConstantVertex *> heapConstants_;
    uint32_t maxCallStackArgs_ = 0;
    uint32_t taggedStackSlots_ = 0;
    uint32_t untaggedStackSlots_ = 0;
    uint32_t maxBlockId_;
    bool hasRecursiveCalls_;
    bool mayHaveUnreachableBlocks_;
    bool reuseStackSlots_ = true;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_GRAPH_H
