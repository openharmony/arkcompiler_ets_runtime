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

#ifndef ECMASCRIPT_ARKSTEED_BB_H
#define ECMASCRIPT_ARKSTEED_BB_H

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"
#include "libpandabase/macros.h"
#include "utils/span.h"

namespace panda::ecmascript::arksteed {

class ControlVertex;
class NonControlVertex;
class ValueVertex;
class PhiVertex;
class MergePointFrameState;
class RegisterMergeState;

constexpr uint32_t INVALID_BLOCK_ID = static_cast<uint32_t>(-1);

// Basic block class for ArkSteed IR
class BB {
public:
    enum BlockType : uint8_t { MERGE, EDGE_SPLIT, DEFAULT };

    NO_COPY_SEMANTIC(BB);
    NO_MOVE_SEMANTIC(BB);

    static BB *New(Chunk *chunk)
    {
        void *memory = chunk->Allocate(sizeof(BB));
        ASSERT(memory != nullptr);
        return new (memory) BB(chunk);
    }

    static BB *NewForMergePoint(Chunk *chunk, MergePointFrameState *state)
    {
        void *memory = chunk->Allocate(sizeof(BB));
        ASSERT(memory != nullptr);
        return new (memory) BB(chunk, state);
    }

    static BB *NewForEdgeSplit(Chunk *chunk, RegisterMergeState *state)
    {
        void *memory = chunk->Allocate(sizeof(BB));
        ASSERT(memory != nullptr);
        return new (memory) BB(chunk, state);
    }

    uint32_t GetId() const
    {
        return id_;
    }

    void SetId(uint32_t id)
    {
        id_ = id;
    }

    ControlVertex *GetControlVertex() const
    {
        return controlVertex_;
    }

    void SetControlVertex(ControlVertex *vertex)
    {
        controlVertex_ = vertex;
    }

    void AddVertex(NonControlVertex *vertex)
    {
        vertices_.push_back(vertex);
    }

    void ReplaceVertices(const std::vector<NonControlVertex *> &newVertices)
    {
        vertices_.assign(newVertices.begin(), newVertices.end());
    }

    size_t GetVertexCount() const
    {
        return vertices_.size();
    }

    const ChunkVector<NonControlVertex *> &GetVertices() const
    {
        return vertices_;
    }

    ChunkVector<NonControlVertex *> &GetVertices()
    {
        return vertices_;
    }

    bool HasState() const
    {
        if (type_ == MERGE) {
            ASSERT(state_ != nullptr);
            return true;
        }
        return false;
    }

    MergePointFrameState *GetState() const
    {
        ASSERT(type_ == MERGE);
        return state_;
    }

    void SetState(MergePointFrameState *state)
    {
        type_ = MERGE;
        this->state_ = state;
    }

    bool HasPhi() const;

    const ChunkVector<PhiVertex *> &GetPhis() const;
    ChunkVector<PhiVertex *> &GetPhis();

    Span<BB *> GetPredecessors();
    Span<const BB *const> GetPredecessors() const;

    BB *GetPredecessor(uint32_t index);
    const BB *GetPredecessor(uint32_t index) const;

    void SetPredecessor(BB *predecessor)
    {
        ASSERT(type_ != MERGE);  // Expects EDGE_SPLIT or DEFAULT
        predecessor_ = predecessor;
    }

    Label *GetLabel()
    {
        return &label_;
    }

    int GetPredecessorId() const;
    void SetPredecessorId(int id);

    uint32_t GetFirstId() const;
    uint32_t GetFirstNonPhiId() const;
    uint32_t GetFirstNonGapMoveId() const;

    BlockType GetBlockType() const
    {
        return type_;
    }

    RegisterMergeState *GetEdgeSplitBlockRegisterState()
    {
        ASSERT(type_ == EDGE_SPLIT);
        ASSERT(mergeState_ != nullptr);
        return mergeState_;
    }

    void SetEdgeSplitBlockRegisterState(RegisterMergeState *registerState)
    {
        ASSERT(type_ == EDGE_SPLIT);
        mergeState_ = registerState;
    }

private:
    explicit BB(Chunk *chunk)
        : id_(INVALID_BLOCK_ID), controlVertex_(nullptr), vertices_(chunk), state_(nullptr), type_(DEFAULT)
    {}

    BB(Chunk *chunk, MergePointFrameState *state)
        : id_(INVALID_BLOCK_ID), controlVertex_(nullptr), vertices_(chunk), state_(state), type_(MERGE)
    {
        ASSERT(state_ != nullptr);
    }

    BB(Chunk *chunk, RegisterMergeState *state)
        : id_(INVALID_BLOCK_ID), controlVertex_(nullptr), vertices_(chunk), mergeState_(state), type_(EDGE_SPLIT)
    {
        ASSERT(mergeState_ != nullptr);
    }

    uint32_t id_;
    ControlVertex *controlVertex_;
    ChunkVector<NonControlVertex *> vertices_;
    union {
        MergePointFrameState *state_;
        RegisterMergeState *mergeState_;
    };
    BB *predecessor_ = nullptr;
    BlockType type_;
    Label label_;
};

// A singly linked list that temporarily stores predecessors referring to this block.
// All predecessors point to the same target block after Bind() is called.
class BBRef {
public:
    BBRef() : nextRef_(nullptr) {}
    explicit BBRef(BB *basicBlock) : basicBlock_(basicBlock) {}
    explicit BBRef(BBRef *head) : BBRef()
    {
        MoveToListHead(head);
    }

    void Bind(BB *basicBlock)
    {
        BBRef *nextRef = SetToBlockAndReturnNext(basicBlock);
        while (nextRef != nullptr) {
            nextRef = nextRef->SetToBlockAndReturnNext(basicBlock);
        }
    }

    BBRef *MoveToListHead(BBRef *head)
    {
        BBRef *oldNextRef = head->nextRef_;
        nextRef_ = oldNextRef;
        head->nextRef_ = this;
        return oldNextRef;
    }

    BBRef *SetToBlockAndReturnNext(BB *basicBlock)
    {
        BBRef *ref = nextRef_;
        basicBlock_ = basicBlock;
        return ref;
    }

    BBRef *Reset()
    {
        BBRef *ref = nextRef_;
        nextRef_ = nullptr;
        return ref;
    }

    BB *BlockRef() const
    {
        return basicBlock_;
    }

    void SetBlockRef(BB *basicBlock)
    {
        basicBlock_ = basicBlock;
    }

private:
    union {
        BB *basicBlock_;
        BBRef *nextRef_;
    };
};
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BB_H
