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

namespace panda::ecmascript::arksteed {

class ControlVertex;
class NonControlVertex;
class ValueVertex;
class PhiVertex;
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

    bool IsLoopHeader() const
    {
        return isLoopHeader_;
    }
    void SetIsLoopHeader(bool v)
    {
        isLoopHeader_ = v;
    }

    bool IsExceptionHandler() const
    {
        return isExceptionHandler_;
    }
    void SetIsExceptionHandler(bool v)
    {
        isExceptionHandler_ = v;
    }

    bool HasRegisterMergeState() const
    {
        return registerMergeState_ != nullptr;
    }

    RegisterMergeState *GetRegisterMergeState()
    {
        ASSERT(registerMergeState_ != nullptr);
        return registerMergeState_;
    }

    void SetRegisterMergeState(RegisterMergeState *state)
    {
        registerMergeState_ = state;
    }

    bool HasPhi() const
    {
        return !phis_.empty();
    }

    const ChunkVector<PhiVertex *> &GetPhis() const
    {
        return phis_;
    }
    ChunkVector<PhiVertex *> &GetPhis()
    {
        return phis_;
    }

    void AddPredecessor(BB *pred)
    {
        predecessors_.push_back(pred);
    }

    ChunkVector<BB *> &GetPredecessors()
    {
        return predecessors_;
    }
    const ChunkVector<BB *> &GetPredecessors() const
    {
        return predecessors_;
    }

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

    BB *GetPredecessor(uint32_t index)
    {
        return predecessors_[index];
    }
    const BB *GetPredecessor(uint32_t index) const
    {
        return predecessors_[index];
    }

    uint32_t PredecessorCount() const
    {
        return static_cast<uint32_t>(predecessors_.size());
    }

    Label *GetLabel()
    {
        return &label_;
    }

    uint32_t GetPredecessorId() const
    {
        return controlVertex_->Cast<UnconditionalControlVertex>()->GetPredecessorId();
    }
    void SetPredecessorId(uint32_t id)
    {
        controlVertex_->Cast<UnconditionalControlVertex>()->SetPredecessorId(id);
    }

    uint32_t GetFirstId() const
    {
        if (HasPhi()) {
            return GetPhis().front()->GetId();
        }
        return GetFirstNonPhiId();
    }

    uint32_t GetFirstNonPhiId() const
    {
        if (!vertices_.empty()) {
            return vertices_.front()->GetId();
        }
        return controlVertex_->GetId();
    }

    uint32_t GetFirstNonGapMoveId() const
    {
        if (HasPhi()) {
            return GetPhis().front()->GetId();
        }
        for (NonControlVertex *vertex : vertices_) {
            ASSERT(vertex != nullptr);
            VertexOpcode opcode = vertex->GetOpcode();
            if (opcode != VertexOpcode::GapMove && opcode != VertexOpcode::ConstantGapMove) {
                return vertex->GetId();
            }
        }
        return controlVertex_->GetId();
    }

private:
    explicit BB(Chunk *chunk)
        : id_(INVALID_BLOCK_ID),
          deferred_(false),
          isLoopHeader_(false),
          isExceptionHandler_(false),
          controlVertex_(nullptr),
          phis_(chunk),
          vertices_(chunk),
          predecessors_(chunk),
          registerMergeState_(nullptr)
    {}

    uint32_t id_;
    bool deferred_;
    bool isLoopHeader_;
    bool isExceptionHandler_;
    ControlVertex *controlVertex_;
    ChunkVector<PhiVertex *> phis_;
    ChunkVector<NonControlVertex *> vertices_;
    ChunkVector<BB *> predecessors_;
    RegisterMergeState *registerMergeState_;
    Label label_;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BB_H
