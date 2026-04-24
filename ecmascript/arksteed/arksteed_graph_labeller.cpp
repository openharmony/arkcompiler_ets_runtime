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

#include "ecmascript/arksteed/arksteed_graph_labeller.h"

#include "ecmascript/arksteed/arksteed_bb.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::arksteed {

// ---------------------------------------------------------------------
// Thread-local labeller storage
// ---------------------------------------------------------------------

thread_local ArkSteedGraphLabeller *g_threadGraphLabeller = nullptr;

// ---------------------------------------------------------------------
// ArkSteedGraphLabellerScope implementation
// ---------------------------------------------------------------------

ArkSteedGraphLabellerScope::ArkSteedGraphLabellerScope(ArkSteedGraphLabeller *labeller) : labeller_(labeller)
{
    ASSERT(labeller_ != nullptr);
    ASSERT(g_threadGraphLabeller == nullptr);
    g_threadGraphLabeller = labeller_;
}

ArkSteedGraphLabellerScope::~ArkSteedGraphLabellerScope()
{
    ASSERT(g_threadGraphLabeller == labeller_);
    g_threadGraphLabeller = nullptr;
}

// ---------------------------------------------------------------------
// Debug printing implementations (only when enabled)
// ---------------------------------------------------------------------

#ifdef ARKSTEED_ENABLE_GRAPH_PRINTER

std::string PrintVertex::BuildOutputString() const
{
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    ASSERT(labeller != nullptr);

    std::string result = labeller->GetVertexLabel(vertex_, hasRegallocData_);
    result += " [" + std::string(OpcodeToString(vertex_->GetOpcode()));

    // Add value representation for value vertices
    if (vertex_->IsValueVertex()) {
        const ValueVertex *value_vertex = static_cast<const ValueVertex *>(vertex_);
        result += " repr=";
        switch (value_vertex->GetValueRepresentation()) {
            case ValueRepresentation::TAGGED:
                result += "tagged";
                break;
            case ValueRepresentation::INT32:
                result += "int32";
                break;
            case ValueRepresentation::UINT32:
                result += "uint32";
                break;
            case ValueRepresentation::FLOAT64:
                result += "float64";
                break;
            case ValueRepresentation::HOLEY_FLOAT64:
                result += "holey_float64";
                break;
            case ValueRepresentation::INT_PTR:
                result += "int_ptr";
                break;
            case ValueRepresentation::NONE:
                result += "none";
                break;
            default:
                result += "unknown";
                break;
        }
    }

    // Add input count
    result += " inputs=" + std::to_string(vertex_->GetInputCount());

    // Add provenance if available
    const ArkSteedGraphLabeller::Provenance &provenance = labeller->GetVertexProvenance(vertex_);
    if (provenance.IsValid()) {
        result += " bc=" + std::to_string(provenance.bytecodeOffset);
    }

    result += "]";
    return result;
}

void PrintVertex::Print(std::ostream &os) const
{
    (void)os;
    if (vertex_ == nullptr) {
        LOG_COMPILER(INFO) << "<null vertex>";
        return;
    }

    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    if (labeller == nullptr) {
        // No labeller active - print basic information
        LOG_COMPILER(INFO) << "<no labeller> (" << OpcodeToString(vertex_->GetOpcode()) << ")";
        return;
    }

    LOG_COMPILER(INFO) << BuildOutputString();
}

void PrintVertexLabel::Print(std::ostream &os) const
{
    (void)os;
    if (vertex_ == nullptr) {
        LOG_COMPILER(INFO) << "<null>";
        return;
    }

    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    if (labeller == nullptr) {
        // No labeller active
        LOG_COMPILER(INFO) << "<no labeller>";
        return;
    }

    LOG_COMPILER(INFO) << labeller->GetVertexLabel(vertex_, false);
}

#endif  // ARKSTEED_ENABLE_GRAPH_PRINTER

}  // namespace panda::ecmascript::arksteed