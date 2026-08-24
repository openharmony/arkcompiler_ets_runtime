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

#include "ecmascript/arksteed/arksteed_dump_helper.h"
#include "ecmascript/arksteed/arksteed_graph.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/arksteed/arksteed_opcode_list.h"

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

GraphPrinter::GraphPrinter(Chunk *chunk, bool withColors)
    : chunk_(chunk),
      withColors_(withColors),
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
        if (block->HasRegisterMergeState()) {
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
    if (!HasConstantsToPrint(graph)) {
        return;
    }

    LOG_COMPILER(INFO) << "";
    LOG_COMPILER(INFO) << "Constants:";

    PrintInt32Constants(graph);
    PrintIntPtrConstants(graph);
    PrintFloat64Constants(graph);
    PrintTaggedConstants(graph);
}

bool GraphPrinter::HasConstantsToPrint(Graph *graph) const
{
    return !graph->GetInt32Constants().empty() ||
           !graph->GetFloat64Constants().empty() || !graph->GetTaggedConstants().empty();
}

void GraphPrinter::PrintInt32Constants(Graph *graph)
{
    for (const auto &[value, vertex] : graph->GetInt32Constants()) {
        std::string line = "  ";
        line += FormatVertexLabel(vertex) + ": ";
        line += "Int32Constant " + std::to_string(value);
        LOG_COMPILER(INFO) << line;
    }
}

void GraphPrinter::PrintIntPtrConstants(Graph *graph)
{
    for (const auto &[value, vertex] : graph->GetIntPtrConstants()) {
        std::string prefix = "  ";
        prefix += FormatVertexLabel(vertex) + ": ";
        LOG_COMPILER(INFO) << prefix << "IntPtrConstant 0x" << std::hex << value << std::dec;
    }
}

void GraphPrinter::PrintFloat64Constants(Graph *graph)
{
    for (const auto &[value, vertex] : graph->GetFloat64Constants()) {
        std::string line = "  ";
        line += FormatVertexLabel(vertex) + ": ";
        line += "Float64Constant " + std::to_string(value);
        LOG_COMPILER(INFO) << line;
    }
}

void GraphPrinter::PrintTaggedConstants(Graph *graph)
{
    for (const auto &[value, vertex] : graph->GetTaggedConstants()) {
        std::string line = "  ";
        line += FormatVertexLabel(vertex) + ": ";
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
    JSTaggedValue tagged(value);
    if (tagged.IsInt()) {
        return " (tagged int: " + std::to_string(tagged.GetInt()) + ")";
    }
    if (tagged.IsDouble()) {
        return " (tagged double: " + std::to_string(tagged.GetDouble()) + ")";
    }
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
    if (tagged.IsObject()) {
        return " (tagged heap object)";
    }
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

    const char *blockTypeName =
        block->IsLoopHeader() ? "Loop Header" : block->IsExceptionHandler() ? "Exception Handler" : "Other";
    LOG_COMPILER(INFO) << PrintBlockArrows(block) << "Block " << block->GetId()
                       << " (" << blockTypeName << ')';
    if (block->HasRegisterMergeState()) {
        PrintPredecessors(block);
    }
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
        case VertexOpcode::BranchIfTrue:
        case VertexOpcode::BranchIfInt32Compare:
        case VertexOpcode::BranchIfFloat64Compare:
        case VertexOpcode::BranchIfReferenceEqual:
        case VertexOpcode::BranchIfObjectType: {
            BB *ifTrue = nullptr;
            BB *ifFalse = nullptr;
            if (BranchControlVertex *branch = vertex->TryCast<BranchControlVertex>()) {
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

std::string GraphPrinter::FormatVertexAnnotations(Vertex *vertex) const
{
    std::string line;
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
    line += "  ";
    line += vertex->Dump(withColors_);
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
        case VertexOpcode::BranchIfTrue:
        case VertexOpcode::BranchIfInt32Compare:
        case VertexOpcode::BranchIfFloat64Compare:
        case VertexOpcode::BranchIfReferenceEqual:
        case VertexOpcode::BranchIfObjectType: {
            BB *ifTrue = nullptr;
            BB *ifFalse = nullptr;
            if (BranchControlVertex *branch = vertex->TryCast<BranchControlVertex>()) {
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

namespace {
constexpr size_t NULL_COLOR_INDEX = static_cast<size_t>(-1);
struct ArrowRun {
    size_t color;
    std::string glyphs;
};

void AppendRun(std::vector<ArrowRun> *runs, size_t color, std::string glyphs)
{
    if (glyphs.empty()) {
        return;
    }
    if (!runs->empty() && runs->back().color == color) {
        runs->back().glyphs += std::move(glyphs);
        return;
    }
    runs->push_back(ArrowRun {color, std::move(glyphs)});
}

std::string RenderArrowRuns(const std::vector<ArrowRun> &runs, bool withColors)
{
    std::ostringstream out;
    for (const ArrowRun &run : runs) {
        if (run.color == NULL_COLOR_INDEX) {
            out << run.glyphs;
            continue;
        }
        WITH_ANSI_COLOR_SCOPE(NthFaintColor(out, run.color, withColors)) {
            out << run.glyphs;
        }
    }
    return out.str();
}
}  // namespace

std::string GraphPrinter::PrintBlockArrows(BB* block)
{
    if (loopHeaders_.erase(block) > 0) {
        AddTarget(block, nullptr);
    }

    std::vector<ArrowRun> runs;
    ChunkVector<size_t> terminitedIndices(chunk_);
    size_t lineColor = NULL_COLOR_INDEX;
    bool sawEnd = false;

    for (size_t i = 0; i < targets_.size(); ++i) {
        size_t desiredColor = lineColor;
        Connection c;

        if (sawEnd) {
            c.AddHorizontal();
        }

        if (targets_[i] == block) {
            desiredColor = i % AnsiColorScope::NUM_FAINT_COLORS;
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
                desiredColor = i % AnsiColorScope::NUM_FAINT_COLORS;
                c.AddVertical();
            }
        }

        AppendRun(&runs, desiredColor, c.ToString());
    }

    if (!terminitedIndices.empty()) {
        AppendRun(&runs, lineColor, "►");
    }

    for (size_t idx : terminitedIndices) {
        targets_[idx] = ArrowTarget();
    }

    return RenderArrowRuns(runs, withColors_);
}

std::string GraphPrinter::GetArrowColumn(ChunkSet<size_t> *arrowsStarting)
{
    std::vector<ArrowRun> runs;
    size_t lineColor = NULL_COLOR_INDEX;
    bool sawStart = false;

    for (size_t i = 0; i < targets_.size(); ++i) {
        size_t desiredColor = lineColor;
        Connection c;

        if (sawStart) {
            c.AddHorizontal();
        }

        if (arrowsStarting != nullptr && arrowsStarting->find(i) != arrowsStarting->end()) {
            desiredColor = i % AnsiColorScope::NUM_FAINT_COLORS;
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
                desiredColor = i % AnsiColorScope::NUM_FAINT_COLORS;
                c.AddVertical();
            }
        }

        AppendRun(&runs, desiredColor, c.ToString());
    }

    return RenderArrowRuns(runs, withColors_);
}

}  // namespace panda::ecmascript::arksteed
