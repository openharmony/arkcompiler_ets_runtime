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
#include "ecmascript/arksteed/arksteed_opcode.h"
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
    NO_COPY_SEMANTIC(BB);
    NO_MOVE_SEMANTIC(BB);

    static BB *New(Chunk *chunk)
    {
        void *memory = chunk->Allocate(sizeof(BB));
        ASSERT(memory != nullptr);
        return new (memory) BB(chunk);
    }

    uint32_t GetId() const
    {
        return id_;
    }

    void SetId(uint32_t id)
    {
        id_ = id;
    }

    bool IsDeferred() const
    {
        return deferred_;
    }

    void SetDeferred(bool deferred)
    {
        deferred_ = deferred;
    }

    ControlVertex *GetControlVertex() const
    {
        return controlVertex_;
    }

    void SetControlVertex(ControlVertex *vertex)
    {
        controlVertex_ = vertex;
    }

    // Note: phis_ is only maintained by GraphBuilderNew. Unused in the old implementation.
    void AddPhiVertex(PhiVertex *vertex)
    {
        phis_.push_back(vertex);
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

    bool IsLoopHeader() const { return isLoopHeader_; }
    void SetLoopHeader(bool v) { isLoopHeader_ = v; }

    bool IsExceptionHandler() const { return isExceptionHandler_; }
    void SetExceptionHandler(bool v) { isExceptionHandler_ = v; }

    bool HasRegisterMerge() const { return registerMergeState_ != nullptr; }

    RegisterMergeState *GetRegisterMergeState()
    {
        ASSERT(registerMergeState_ != nullptr);
        return registerMergeState_;
    }

    void SetRegisterMergeState(RegisterMergeState *state) { registerMergeState_ = state; }

    bool HasPhi() const { return !phis_.empty(); }

    const ChunkVector<PhiVertex *> &GetPhis() const { return phis_; }
    ChunkVector<PhiVertex *> &GetPhis() { return phis_; }

    void AddPredecessor(BB *pred) { predecessors_.push_back(pred); }

    Span<BB *> GetPredecessors() { return {predecessors_.data(), predecessors_.size()}; }
    Span<const BB *const> GetPredecessors() const { return {predecessors_.data(), predecessors_.size()}; }

    template <class Callback>
    void ForEachPredecessor(Callback callback) const
    {
        for (const BB *predecessor : GetPredecessors()) {
            callback(const_cast<BB *>(predecessor));
        }
    }

    template <class Callback>
    void ForEachSuccessor(Callback callback) const
    {
        ControlVertex *control = GetControlVertex();
        if (auto *jump = control->TryCast<UnconditionalControlVertex>(); jump != nullptr) {
            callback(jump->Target());
        } else if (auto *branch = control->TryCast<BranchControlVertex>(); branch != nullptr) {
            callback(branch->IfTrue());
            callback(branch->IfFalse());
        }
    }

    BB *GetPredecessor(uint32_t index) { return predecessors_[index]; }
    const BB *GetPredecessor(uint32_t index) const { return predecessors_[index]; }

    uint32_t PredecessorCount() const { return predecessorCount_; }
    void SetPredecessorCount(uint32_t n) { predecessorCount_ = n; }

    void SetSinglePredecessor(BB *predecessor) { predecessor_ = predecessor; }

    Label *GetLabel()
    {
        return &label_;
    }

    int GetPredecessorId() const;
    void SetPredecessorId(int id);

    uint32_t GetFirstId() const;
    uint32_t GetFirstNonPhiId() const;
    uint32_t GetFirstNonGapMoveId() const;

private:
    explicit BB(Chunk *chunk)
        : id_(INVALID_BLOCK_ID),
          deferred_(false),
          controlVertex_(nullptr),
          phis_(chunk),
          vertices_(chunk),
          predecessors_(chunk),
          isLoopHeader_(false),
          isExceptionHandler_(false),
          registerMergeState_(nullptr),
          predecessorCount_(0),
          predecessor_(nullptr)
    {}

    uint32_t id_;
    bool deferred_;
    ControlVertex *controlVertex_;
    ChunkVector<PhiVertex *> phis_;
    ChunkVector<NonControlVertex *> vertices_;
    ChunkVector<BB *> predecessors_;
    bool isLoopHeader_;
    bool isExceptionHandler_;
    RegisterMergeState *registerMergeState_;
    uint32_t predecessorCount_ = 0;
    BB *predecessor_ = nullptr;
    Label label_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BB_H
