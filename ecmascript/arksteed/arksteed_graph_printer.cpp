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

#include "ecmascript/arksteed/arksteed_graph_printer.h"

#include <sstream>

#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_graph_labeller.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"
#include "ecmascript/base/bit_helper.h"
#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {

enum ConnectionLocation { TOP = 1 << 0, LEFT = 1 << 1, RIGHT = 1 << 2, BOTTOM = 1 << 3 };

struct Connection {
    void Connect(ConnectionLocation loc)
    {
        connected |= loc;
    }
    void AddHorizontal()
    {
        Connect(LEFT);
        Connect(RIGHT);
    }
    void AddVertical()
    {
        Connect(TOP);
        Connect(BOTTOM);
    }
    std::string ToString() const
    {
        switch (connected) {
            case 0:
                return " ";
            case TOP:
                return "╵";
            case LEFT:
                return "╴";
            case RIGHT:
                return "╶";
            case BOTTOM:
                return "╷";
            case TOP | LEFT:
                return "╯";
            case TOP | RIGHT:
                return "╰";
            case BOTTOM | LEFT:
                return "╮";
            case BOTTOM | RIGHT:
                return "╭";
            case TOP | BOTTOM:
                return "│";
            case LEFT | RIGHT:
                return "─";
            case TOP | BOTTOM | RIGHT:
                return "├";
            case TOP | BOTTOM | LEFT:
                return "┤";
            case TOP | LEFT | RIGHT:
                return "┴";
            case BOTTOM | LEFT | RIGHT:
                return "┬";
            case TOP | BOTTOM | LEFT | RIGHT:
                return "┼";
            default:
                return "?";
        }
    }
    uint8_t connected = 0;
};

GraphPrinter::GraphPrinter(Chunk *chunk, bool hasRegallocData)
    : chunk_(chunk),
      hasRegallocData_(hasRegallocData),
      totalVertices_(0),
      totalBlocks_(0),
      currentBlock_(nullptr),
      verticesInCurrentBlock_(0),
      blockOrder_(chunk),
      successorsMap_(chunk),
      loopHeaders_(chunk),
      targets_(chunk),
      activeArrows_(chunk)
{}

void GraphPrinter::PreProcessGraph(Graph *graph)
{
    totalVertices_ = 0;
    totalBlocks_ = graph->NumBlocks();
    targets_.clear();
    loopHeaders_.clear();
    blockOrder_.clear();
    successorsMap_.clear();
    activeArrows_.clear();

    for (BB *block : *graph) {
        blockOrder_.push_back(block);
        // Skip predecessor processing for blocks without register merge state
        if (block->HasRegisterMerge()) {
            const auto &predecessors = block->GetPredecessors();
            for (BB *pred : predecessors) {
                successorsMap_[pred].push_back(block);
            }
        }
    }

    LOG_COMPILER(INFO) << "ArkSteed IR Graph";
    LOG_COMPILER(INFO) << "Blocks: " << totalBlocks_;
    LOG_COMPILER(INFO) << "Parameters: " << graph->GetParameterCount();
    if (graph->HasRecursiveCalls()) {
        LOG_COMPILER(INFO) << "  (has recursive calls)";
    }
    if (graph->MayHaveUnreachableBlocks()) {
        LOG_COMPILER(INFO) << "  (may have unreachable blocks)";
    }
    LOG_COMPILER(INFO) << "";
}

void GraphPrinter::PostProcessGraph(Graph *graph)
{
    LOG_COMPILER(INFO) << "Total vertices: " << totalVertices_;
    LOG_COMPILER(INFO) << "Total blocks: " << totalBlocks_;

    // Print constants at the end since they don't belong to any specific block
    PrintConstants(graph);
}

void GraphPrinter::PrintConstants(Graph *graph)
{
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();

    if (!HasConstantsToPrint(graph)) {
        return;
    }

    LOG_COMPILER(INFO) << "";
    LOG_COMPILER(INFO) << "Constants:";

    PrintRootConstants(graph, labeller);
    PrintInt32Constants(graph, labeller);
    PrintIntPtrConstants(graph, labeller);
    PrintFloat64Constants(graph, labeller);
    PrintTaggedConstants(graph, labeller);
}

bool GraphPrinter::HasConstantsToPrint(Graph *graph) const
{
    return !graph->GetRootConstants().empty() || !graph->GetInt32Constants().empty() ||
           !graph->GetFloat64Constants().empty() || !graph->GetTaggedConstants().empty();
}

void GraphPrinter::PrintRootConstants(Graph *graph, ArkSteedGraphLabeller *labeller)
{
    for (const auto &[index, vertex] : graph->GetRootConstants()) {
        std::string line = "  ";
        if (labeller != nullptr) {
            line += labeller->GetVertexLabel(vertex, hasRegallocData_) + ": ";
        }
        line += "RootConstant ";
        switch (index) {
            case RootConstantVertex::RootIndex::UNDEFINED:
                line += "Undefined";
                break;
            case RootConstantVertex::RootIndex::NULL_VALUE:
                line += "Null";
                break;
            case RootConstantVertex::RootIndex::TRUE_VALUE:
                line += "True";
                break;
            case RootConstantVertex::RootIndex::FALSE_VALUE:
                line += "False";
                break;
            default:
                line += "Unknown(" + std::to_string(static_cast<int>(index)) + ")";
                break;
        }
        LOG_COMPILER(INFO) << line;
    }
}

void GraphPrinter::PrintInt32Constants(Graph *graph, ArkSteedGraphLabeller *labeller)
{
    for (const auto &[value, vertex] : graph->GetInt32Constants()) {
        std::string line = "  ";
        if (labeller != nullptr) {
            line += labeller->GetVertexLabel(vertex, hasRegallocData_) + ": ";
        }
        line += "Int32Constant " + std::to_string(value);
        LOG_COMPILER(INFO) << line;
    }
}

void GraphPrinter::PrintIntPtrConstants(Graph *graph, ArkSteedGraphLabeller *labeller)
{
    for (const auto &[value, vertex] : graph->GetIntPtrConstants()) {
        std::string prefix = "  ";
        if (labeller != nullptr) {
            prefix += labeller->GetVertexLabel(vertex, hasRegallocData_) + ": ";
        }
        LOG_COMPILER(INFO) << prefix << "IntPtrConstant 0x" << std::hex << value << std::dec;
    }
}

void GraphPrinter::PrintFloat64Constants(Graph *graph, ArkSteedGraphLabeller *labeller)
{
    for (const auto &[value, vertex] : graph->GetFloat64Constants()) {
        std::string line = "  ";
        if (labeller != nullptr) {
            line += labeller->GetVertexLabel(vertex, hasRegallocData_) + ": ";
        }
        line += "Float64Constant " + std::to_string(value);
        LOG_COMPILER(INFO) << line;
    }
}

void GraphPrinter::PrintTaggedConstants(Graph *graph, ArkSteedGraphLabeller *labeller)
{
    for (const auto &[value, vertex] : graph->GetTaggedConstants()) {
        std::string line = "  ";
        if (labeller != nullptr) {
            line += labeller->GetVertexLabel(vertex, hasRegallocData_) + ": ";
        }
        line += "TaggedConstant 0x";
        std::ostringstream hexStream;
        hexStream << std::hex << value;
        line += hexStream.str();
        line += DecodeTaggedValue(value);
        LOG_COMPILER(INFO) << line;
    }
}

std::string GraphPrinter::DecodeTaggedValue(uint64_t value) const
{
    using JSTaggedValue = ecmascript::JSTaggedValue;

    // Check for tagged int
    if ((value & JSTaggedValue::TAG_MARK) == JSTaggedValue::TAG_INT) {
        int32_t intValue = static_cast<int32_t>(value);
        return " (tagged int: " + std::to_string(intValue) + ")";
    }

    // Check for special values using switch
    switch (value) {
        case JSTaggedValue::VALUE_UNDEFINED:
            return " (tagged undefined)";
        case JSTaggedValue::VALUE_NULL:
            return " (tagged null)";
        case JSTaggedValue::VALUE_TRUE:
            return " (tagged true)";
        case JSTaggedValue::VALUE_FALSE:
            return " (tagged false)";
        case JSTaggedValue::VALUE_HOLE:
            return " (tagged hole)";
        case JSTaggedValue::VALUE_EXCEPTION:
            return " (tagged exception)";
        default:
            break;
    }

    // Check for other boolean values
    if ((value & JSTaggedValue::TAG_BOOLEAN_MASK) == JSTaggedValue::TAG_BOOLEAN_MASK) {
        return " (tagged boolean)";
    }

    // Check for tagged double
    // 48: double value occupies 48 bits in tagged representation
    if (value >= JSTaggedValue::DOUBLE_ENCODE_OFFSET && value < (JSTaggedValue::DOUBLE_ENCODE_OFFSET + (1ULL << 48))) {
        double doubleValue = base::bit_cast<double>(value - JSTaggedValue::DOUBLE_ENCODE_OFFSET);
        return " (tagged double: " + std::to_string(doubleValue) + ")";
    }

    // Check for heap object
    if ((value & JSTaggedValue::TAG_HEAPOBJECT_MASK) == JSTaggedValue::TAG_OBJECT) {
        return " (tagged heap object)";
    }

    // Unknown type
    return " (tagged unknown)";
}

void GraphPrinter::PrintPredecessors(BB *block)
{
    const auto &predecessors = block->GetPredecessors();
    if (!predecessors.empty()) {
        std::string predLine = GetArrowColumn(nullptr) + "  Predecessors: ";
        for (size_t i = 0; i < predecessors.size(); ++i) {
            if (i > 0) {
                predLine += ", ";
            }
            predLine += "B" + std::to_string(predecessors[i]->GetId());
        }
        LOG_COMPILER(INFO) << predLine;
    }
}

void GraphPrinter::PreProcessBlock(BB *block)
{
    currentBlock_ = block;
    verticesInCurrentBlock_ = 0;

    if (loopHeaders_.erase(block) > 0) {
        AddTarget(block, nullptr);
    }

    LOG_COMPILER(INFO) << GetArrowColumn(nullptr) <<
        "------------------------------------------------------------------------";

    const char *blockTypeName = block->IsLoopHeader()    ? "Loop Header"
                                : block->IsExceptionHandler() ? "Exception Handler"
                                                              : "Other";
    LOG_COMPILER(INFO) << PrintBlockArrows(block) << "Block " << block->GetId() << " (" << blockTypeName << ')';

    if (block->HasRegisterMerge()) {
        PrintPredecessors(block);
    }

    std::string controlLine = GetArrowColumn(nullptr) + "  Control vertex: ";
    if (ControlVertex *control = block->GetControlVertex()) {
        controlLine += OpcodeToString(control->GetOpcode());
    } else {
        controlLine += "(none)";
    }
    LOG_COMPILER(INFO) << controlLine;
}

void GraphPrinter::PostProcessBlock(BB *block)
{
    auto it = successorsMap_.find(block);
    if (it != successorsMap_.end() && !it->second.empty()) {
        const auto &successors = it->second;
        std::string succLine = GetArrowColumn(nullptr) + "  Successors: ";
        for (size_t i = 0; i < successors.size(); ++i) {
            if (i > 0) {
                succLine += ", ";
            }
            succLine += "B" + std::to_string(successors[i]->GetId());
        }
        LOG_COMPILER(INFO) << succLine;
    }

    LOG_COMPILER(INFO) << GetArrowColumn(nullptr) + "  Vertices in block: " << verticesInCurrentBlock_;

    // Print fallthrough arrow for adjacent block jumps
    if (hasFallthrough_) {
        LOG_COMPILER(INFO) << GetArrowColumn(nullptr) << "  ↓";
    }

    currentBlock_ = nullptr;
    hasFallthrough_ = false;
    if (activeArrows_.empty()) {
        targets_.clear();
    }
}

void GraphPrinter::ProcessVertex(NonControlVertex *vertex, [[maybe_unused]] const ArkSteedState &state)
{
    totalVertices_++;
    verticesInCurrentBlock_++;
    PrintVertex(vertex, nullptr);
}

void GraphPrinter::ProcessVertex(ControlVertex *vertex, const ArkSteedState &state)
{
    totalVertices_++;
    verticesInCurrentBlock_++;

    ChunkSet<size_t> arrowsStartingHere(chunk_);
    BB *nextBlock = state.NextBlock();
    hasFallthrough_ = false;

    VertexOpcode opcode = vertex->GetOpcode();
    switch (opcode) {
        case VertexOpcode::Jump: {
            BB *target = nullptr;
            if (JumpVertex *jump = vertex->TryCast<JumpVertex>()) {
                target = jump->Target();
            }
            if (target != nullptr) {
                bool added = AddTargetIfNotNext(target, nextBlock, currentBlock_, &arrowsStartingHere);
                if (!added) {
                    hasFallthrough_ = true;
                }
            }
            break;
        }
        case VertexOpcode::BranchIfTrue: {
            BB *ifTrue = nullptr;
            BB *ifFalse = nullptr;
            if (BranchIfTrueVertex *branch = vertex->TryCast<BranchIfTrueVertex>()) {
                ifTrue = branch->IfTrue();
                ifFalse = branch->IfFalse();
            }
            if (ifTrue != nullptr) {
                bool added = AddTargetIfNotNext(ifTrue, nextBlock, currentBlock_, &arrowsStartingHere);
                if (!added) {
                    hasFallthrough_ = true;
                }
            }
            if (ifFalse != nullptr) {
                bool added = AddTargetIfNotNext(ifFalse, nextBlock, currentBlock_, &arrowsStartingHere);
                if (!added) {
                    hasFallthrough_ = true;
                }
            }
            break;
        }
        default:
            break;
    }

    PrintVertex(vertex, &arrowsStartingHere);
}

std::string GraphPrinter::FormatVertexStubInfo(Vertex *vertex) const
{
    std::string line;
    if (vertex->Is<CallRuntimeVertex>()) {
        CallRuntimeVertex *callRuntime = vertex->Cast<CallRuntimeVertex>();
        line += " [" + kungfu::RuntimeStubCSigns::GetRTName(static_cast<int>(callRuntime->GetRuntimeId())) + "]";
    } else if (vertex->Is<CallCommonStubVertex>()) {
        CallCommonStubVertex *callStub = vertex->Cast<CallCommonStubVertex>();
        line += " [" + kungfu::CommonStubCSigns::GetName(callStub->GetStubId()) + "]";
    } else if (vertex->Is<ValueVertex>()) {
        ValueVertex *valueVertex = vertex->Cast<ValueVertex>();
        line += " [" + ValueRepresentationToString(valueVertex->GetValueRepresentation()) + "]";
    }
    return line;
}

std::string GraphPrinter::FormatVertexInputs(Vertex *vertex, ArkSteedGraphLabeller *labeller) const
{
    std::string line;
    for (int i = 0; i < vertex->GetInputCount(); ++i) {
        if (i > 0) {
            line += ", ";
        }
        ValueVertex *input = vertex->GetInput(i);
        if (input != nullptr && labeller != nullptr) {
            line += labeller->GetVertexLabel(input, hasRegallocData_);
        } else if (input != nullptr) {
            line += "v?";
        } else {
            line += "null";
        }
    }
    return line;
}

std::string GraphPrinter::FormatVertexAnnotations(Vertex *vertex) const
{
    std::string line;
    VertexProperties props = vertex->GetProperties();
    line += " {";
    if (props.CanEagerDeopt())
        line += " eager-deopt";
    else if (props.CanLazyDeopt())
        line += " lazy-deopt";
    else if (props.IsDeoptCheckpoint())
        line += " deopt-checkpoint";
    if (props.CanThrow()) {
        line += " can-throw";
    }
    if (props.CanRead()) {
        line += " can-read";
    }
    if (props.CanWrite()) {
        line += " can-write";
    }
    if (props.CanAllocate()) {
        line += " can-allocate";
    }
    line += " }";

    if (vertex->Is<ValueVertex>()) {
        auto *valueVertex = vertex->Cast<ValueVertex>();
        auto *regallocInfo = valueVertex->GetRegallocInfo();
        if (regallocInfo != nullptr) {
            line += " → ";
            if (regallocInfo->HasValidLiveRange()) {
                auto liveRange = regallocInfo->GetLiveRange();
                line += "live range: [" + std::to_string(liveRange.start) + "-" + std::to_string(liveRange.end) + "]";
            }
        }
    }

    if (vertex->Is<ControlVertex>()) {
        ControlVertex *controlVertex = vertex->Cast<ControlVertex>();
        line += FormatControlVertexTargets(controlVertex);
    }
    return line;
}

void GraphPrinter::PrintVertex(Vertex *vertex, ChunkSet<size_t> *arrowsStartingHere)
{
    std::string line = GetArrowColumn(arrowsStartingHere);
    ArkSteedGraphLabeller *labeller = GetCurrentGraphLabeller();
    line += "  ";
    if (labeller != nullptr) {
        line += labeller->GetVertexLabel(vertex, hasRegallocData_) + ": ";
    } else {
        line += "v?: ";
    }
    line += OpcodeToString(vertex->GetOpcode());
    line += FormatVertexStubInfo(vertex);
    line += "(";
    line += FormatVertexInputs(vertex, labeller);
    line += ")";
    line += FormatVertexAnnotations(vertex);
    LOG_COMPILER(INFO) << line;
}

std::string GraphPrinter::FormatControlVertexTargets(ControlVertex *vertex) const
{
    std::string result;
    VertexOpcode opcode = vertex->GetOpcode();

    switch (opcode) {
        case VertexOpcode::Jump: {
            BB *target = nullptr;
            if (JumpVertex *jump = vertex->TryCast<JumpVertex>()) {
                target = jump->Target();
            }
            if (target != nullptr) {
                result += " → B" + std::to_string(target->GetId());
            }
            break;
        }
        case VertexOpcode::BranchIfTrue: {
            BB *ifTrue = nullptr;
            BB *ifFalse = nullptr;
            if (BranchIfTrueVertex *branch = vertex->TryCast<BranchIfTrueVertex>()) {
                ifTrue = branch->IfTrue();
                ifFalse = branch->IfFalse();
            }
            if (ifTrue != nullptr && ifFalse != nullptr) {
                result += " → B" + std::to_string(ifTrue->GetId()) + " (true), B" + std::to_string(ifFalse->GetId()) +
                          " (false)";
            }
            break;
        }
        case VertexOpcode::Return: {
            result += " [return]";
            break;
        }
        case VertexOpcode::Throw: {
            result += " [throw]";
            break;
        }
        default:
            break;
    }

    return result;
}

std::string GraphPrinter::ValueRepresentationToString(ValueRepresentation repr) const
{
    switch (repr) {
        case ValueRepresentation::TAGGED:
            return "Tagged";
        case ValueRepresentation::INT32:
            return "Int32";
        case ValueRepresentation::UINT32:
            return "Uint32";
        case ValueRepresentation::FLOAT64:
            return "Float64";
        case ValueRepresentation::HOLEY_FLOAT64:
            return "HoleyFloat64";
        case ValueRepresentation::INT_PTR:
            return "IntPtr";
        case ValueRepresentation::NONE:
            return "None";
        default:
            return "Unknown";
    }
}

size_t GraphPrinter::AddTarget(BB *target, BB *currentBlock)
{
    bool isBackward = false;
    if (currentBlock != nullptr && target != nullptr) {
        size_t currentPos = static_cast<size_t>(-1);
        size_t targetPos = static_cast<size_t>(-1);

        for (size_t i = 0; i < blockOrder_.size(); ++i) {
            if (blockOrder_[i] == currentBlock) {
                currentPos = i;
            }
            if (blockOrder_[i] == target) {
                targetPos = i;
            }
        }

        if (currentPos != static_cast<size_t>(-1) && targetPos != static_cast<size_t>(-1)) {
            isBackward = targetPos < currentPos;
            if (isBackward) {
                loopHeaders_.insert(target);
            }
        }
    }

    ArrowTarget arrowTarget(target, isBackward);

    for (size_t i = 0; i < targets_.size(); ++i) {
        if (!targets_[i]) {
            targets_[i] = arrowTarget;
            return i;
        }
    }

    targets_.push_back(arrowTarget);
    return targets_.size() - 1;
}

bool GraphPrinter::AddTargetIfNotNext(BB *target, BB *nextBlock, BB *currentBlock, ChunkSet<size_t> *arrowsStarting)
{
    if (nextBlock == target) {
        return false;
    }
    size_t index = AddTarget(target, currentBlock);
    if (arrowsStarting != nullptr) {
        arrowsStarting->insert(index);
    }
    return true;
}

std::string GraphPrinter::PrintBlockArrows(BB* block)
{
    if (loopHeaders_.erase(block) > 0) {
        AddTarget(block, nullptr);
    }

    std::string arrowLine;
    ChunkVector<size_t> terminitedIndices(chunk_);
    int lineColor = -1;
    int currentColor = -1;
    bool sawEnd = false;

    for (size_t i = 0; i < targets_.size(); ++i) {
        int desiredColor = lineColor;
        Connection c;

        if (sawEnd) {
            c.AddHorizontal();
        }

        if (targets_[i] == block) {
            desiredColor = (i % 6) + 1;  // 6: number of available ANSI colors
            lineColor = desiredColor;
            c.Connect(RIGHT);
            if (targets_[i].isBackward) {
                c.Connect(BOTTOM);
            } else {
                c.Connect(TOP);
            }
            terminitedIndices.push_back(i);
            activeArrows_.erase(i);
            sawEnd = true;
        } else if (targets_[i] && activeArrows_.find(i) != activeArrows_.end()) {
            if (!targets_[i].isBackward) {
                desiredColor = (i % 6) + 1;  // 6: number of available ANSI colors
                c.AddVertical();
            }
        }

        if (currentColor != desiredColor && desiredColor != -1) {
            arrowLine += "\033[0;3";
            arrowLine += std::to_string(desiredColor);
            arrowLine += "m";
            currentColor = desiredColor;
        }

        arrowLine += c.ToString();
    }

    if (!terminitedIndices.empty()) {
        arrowLine += "►";
    }

    if (currentColor != -1) {
        arrowLine += "\033[0m";
    }

    for (size_t idx : terminitedIndices) {
        targets_[idx] = ArrowTarget();
    }

    return arrowLine;
}

std::string GraphPrinter::GetArrowColumn(ChunkSet<size_t> *arrowsStarting)
{
    std::string arrowColumn;
    int lineColor = -1;
    int currentColor = -1;
    bool sawStart = false;

    for (size_t i = 0; i < targets_.size(); ++i) {
        int desiredColor = lineColor;
        Connection c;

        if (sawStart) {
            c.AddHorizontal();
        }

        if (arrowsStarting != nullptr && arrowsStarting->find(i) != arrowsStarting->end()) {
            desiredColor = (i % 6) + 1;  // 6: number of available ANSI colors
            lineColor = desiredColor;
            c.Connect(RIGHT);
            if (i < targets_.size() && targets_[i].isBackward) {
                c.Connect(TOP);
            } else {
                c.Connect(BOTTOM);
            }
            activeArrows_.insert(i);
            sawStart = true;
        }

        if (c.connected == 0 && targets_[i] && activeArrows_.find(i) != activeArrows_.end()) {
            if (!targets_[i].isBackward) {
                desiredColor = (i % 6) + 1;  // 6: number of available ANSI colors
                c.AddVertical();
            }
        }

        if (currentColor != desiredColor && desiredColor != -1) {
            arrowColumn += "\033[0;3";
            arrowColumn += std::to_string(desiredColor);
            arrowColumn += "m";
            currentColor = desiredColor;
        }

        arrowColumn += c.ToString();
    }

    if (currentColor != -1) {
        arrowColumn += "\033[0m";
    }

    return arrowColumn;
}

}  // namespace panda::ecmascript::arksteed
