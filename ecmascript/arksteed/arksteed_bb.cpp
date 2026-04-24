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

#include "arksteed_bb.h"

#include "arksteed_framestate.h"
#include "arksteed_opcode.h"

namespace panda::ecmascript::arksteed {

bool BB::HasPhi() const
{
    return HasState() && state_->HasPhi();
}

const ChunkVector<PhiVertex *> &BB::GetPhis() const
{
    return state_->Phis();
}

ChunkVector<PhiVertex *> &BB::GetPhis()
{
    return state_->Phis();
}

int BB::GetPredecessorId() const
{
    return controlVertex_->Cast<UnconditionalControlVertex>()->GetPredecessorId();
}

void BB::SetPredecessorId(int id)
{
    controlVertex_->Cast<UnconditionalControlVertex>()->SetPredecessorId(id);
}

uint32_t BB::GetFirstId() const
{
    if (HasPhi()) {
        return GetPhis().front()->GetId();
    }
    return GetFirstNonPhiId();
}

uint32_t BB::GetFirstNonPhiId() const
{
    for (NonControlVertex *vertex : vertices_) {
        if (vertex != nullptr) {
            return vertex->GetId();
        }
    }
    return controlVertex_->GetId();
}

uint32_t BB::GetFirstNonGapMoveId() const
{
    // If has phi vertices, return the first phi's id
    if (HasPhi()) {
        return GetPhis().front()->GetId();
    }
    for (NonControlVertex *vertex : vertices_) {
        if (vertex != nullptr) {
            VertexOpcode opcode = vertex->GetOpcode();
            // Skip GapMove and ConstantGapMove
            if (opcode != VertexOpcode::GapMove && opcode != VertexOpcode::ConstantGapMove) {
                return vertex->GetId();
            }
        }
    }
    // If all vertices are gap moves, return the control vertex id
    return controlVertex_->GetId();
}

Span<BB *> BB::GetPredecessors()
{
    if (type_ != MERGE) {
        return {&predecessor_, 1};
    }
    ASSERT(HasState());
    return state_->Predecessors();
}

Span<const BB *const> BB::GetPredecessors() const
{
    if (type_ != MERGE) {
        return {&predecessor_, 1};
    }
    ASSERT(HasState());
    const MergePointFrameState *constState = state_;
    return constState->Predecessors();
}

BB *BB::GetPredecessor(uint32_t index)
{
    if (type_ != MERGE) {
        ASSERT(index == 0);
        return predecessor_;
    }
    ASSERT(HasState());
    return state_->PredecessorAt(index);
}

const BB *BB::GetPredecessor(uint32_t index) const
{
    return const_cast<BB *>(this)->GetPredecessor(index);
}

}  // namespace panda::ecmascript::arksteed