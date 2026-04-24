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

#include "ecmascript/arksteed/arksteed_regalloc_vertex_info.h"

namespace panda::ecmascript::arksteed {

void Vertex::Print() const
{
    std::cout << "Vertex[" << GetId() << "]: " << OpcodeToString(GetOpcode());
    std::cout << " (inputs: " << GetInputCount() << ")";

    if (HasInputs()) {
        std::cout << " [";
        for (int i = 0; i < GetInputCount(); ++i) {
            const ValueVertex *input = GetInput(i);
            if (input != nullptr) {
                std::cout << input->GetId();
            } else {
                std::cout << "null";
            }
            if (i < GetInputCount() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]";
    }

    std::cout << std::endl;
}

void Vertex::ReduceInputCount(int num)
{
    ASSERT(GetOpcode() == VertexOpcode::Phi);
    ASSERT(GetInputCount() >= static_cast<int>(num));
    bitfield_ = InputCountField::Update(bitfield_, GetInputCount() - num);
}

ValueLocation &ValueVertex::Result()
{
    return GetRegallocInfo()->GetResult();
}

const ValueLocation &ValueVertex::Result() const
{
    return GetRegallocInfo()->GetResult();
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
