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

#include "ecmascript/arksteed/arksteed_vertex.h"

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/arksteed/arksteed_regalloc_vertex_info.h"

namespace panda::ecmascript::arksteed {

#if !defined(NDEBUG)
namespace {
struct VertexDebugLabelState {
    bool compiling = false;
    uint32_t nextLabel = 1;
};
thread_local VertexDebugLabelState gVertexDebugLabelState;
}  // namespace

uint32_t NextVertexLabel()
{
    return gVertexDebugLabelState.nextLabel++;
}

VertexLabelScope::VertexLabelScope()
{
    ASSERT(!gVertexDebugLabelState.compiling);
    gVertexDebugLabelState.compiling = true;
    gVertexDebugLabelState.nextLabel = 1;
}

VertexLabelScope::~VertexLabelScope()
{
    gVertexDebugLabelState.compiling = false;
}
#else
VertexLabelScope::VertexLabelScope() = default;
VertexLabelScope::~VertexLabelScope() = default;
#endif

std::string FormatVertexLabel(const Vertex *vertex)
{
    if (vertex == nullptr) {
        return "<null>";
    }
#if !defined(NDEBUG)
    std::string label = "n" + std::to_string(vertex->label_);
    if (vertex->HasId()) {
        return label = "v" + std::to_string(vertex->GetId());
    }
    if (vertex->label_ != INVALID_VERTEX_ID) {
        return "n" + std::to_string(vertex->label_);
    }
    return "<unregistered>";
#else
    if (vertex->HasId()) {
        return "v" + std::to_string(vertex->GetId());
    }
    return "v?";
#endif
}

ValueLocation &ValueVertex::Result()
{
    return GetRegallocInfo()->GetResult();
}

const ValueLocation &ValueVertex::Result() const
{
    return GetRegallocInfo()->GetResult();
}

void ValueVertex::SetHint(InstructionOperand hint)
{
    auto *info = GetRegallocInfo();
    if (info->HasHint()) {
        return;
    }
    info->SetHint(hint);

    if (info->GetResult().IsUnallocated()) {
        UnallocatedState operand = UnallocatedState::Cast(info->GetResult().GetOperand());
        if (operand.HasSameAsInputPolicy()) {
            GetInput(operand.GetInputIndex())->SetHint(hint);
        }
    }

    if (GetOpcode() == VertexOpcode::Phi) {
        if (GetOwner()->IsLoopHeader()) {
            // input(0) = loop entry; input(1) = backedge! skip this one, 
            // invariant is that only 2 inputs exist for PhiVertex.
            if (GetInput(0) != nullptr) {
                GetInput(0)->SetHint(hint);
            }
        } else {
            for (uint32_t i = 0; i < GetInputCount(); i++) {
                if (GetInput(i) != nullptr) {
                    GetInput(i)->SetHint(hint);
                }
            }
        }
    }
}

InputLocation *Input::GetLocation() const
{
    return base_->GetRegallocInfo()->GetInputLocation(index_);
}

const InstructionOperand &Input::GetOperand() const
{
    return GetLocation()->GetOperand();
}

const InputLocation *ConstInput::GetLocation() const
{
    return base_->GetRegallocInfo()->GetInputLocation(index_);
}

const InstructionOperand &ConstInput::GetOperand() const
{
    return GetLocation()->GetOperand();
}

}  // namespace panda::ecmascript::arksteed
