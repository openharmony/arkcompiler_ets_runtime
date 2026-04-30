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

#include "ecmascript/arksteed/arksteed_bytecode_preprocessor.h"

#include "ecmascript/arksteed/arksteed_bytecode_context.h"
#include "ecmascript/arksteed/arksteed_bytecode_iterator.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
using namespace panda::ecmascript::kungfu;

void BytecodePreprocessor::Initialize(BytecodeContext *context)
{
    context_ = context;
    lastBcIndex_ = static_cast<uint32_t>(context_->GetBytecodeCount()) - 1;
    hasTryCatch_ = !context_->GetExceptionInfo().empty();
    // Init postOrderList
    postOrderList_.resize(lastBcIndex_ + 1);
    for (uint32_t i = 0; i <= lastBcIndex_; i++) {
        postOrderList_[i] = i;
    }
    BuildBasicBlocksAndReorderBytecode();
}

void BytecodePreprocessor::BuildBasicBlocksAndReorderBytecode()
{
    BuildBasicBlock();
    BuildCFGEdges();
    BuildPostOrderListAndReorderBytecode();
}

void BytecodePreprocessor::BuildBasicBlock()
{
    // Find block starts
    blockStarts_.insert(0);
    const auto &bytecodeInfo = context_->GetInfoData();

    auto markAsBlockStart = [this](uint32_t index) {
        ASSERT(index <= lastBcIndex_);
        blockStarts_.insert(index);
    };
    auto markNextBCAsBlockStart = [this](uint32_t curIndex) {
        if (curIndex + 1 <= lastBcIndex_) {
            blockStarts_.insert(curIndex + 1);
        }
    };

    for (uint32_t index = 0; index <= lastBcIndex_; ++index) {
        const auto &info = bytecodeInfo[index];
        if (info.IsJump()) {
            markAsBlockStart(context_->GetJumpTargetBcIndex(index));
            markNextBCAsBlockStart(index);
        } else if (info.IsReturn() || info.IsThrow()) {
            markNextBCAsBlockStart(index);
        }
    }

    MarkExceptionBlockStarts();
    CreateBlockInfoEntries();
}

void BytecodePreprocessor::MarkExceptionBlockStarts()
{
    if (!hasTryCatch_) {
        return;
    }
    for (const auto &exItem : context_->GetExceptionInfo()) {
        blockStarts_.insert(exItem.startBcIndex);
        if (exItem.endBcIndex <= lastBcIndex_) {
            blockStarts_.insert(exItem.endBcIndex);
        }
        for (uint32_t catchBcIndex : exItem.catchBcIndices) {
            blockStarts_.insert(catchBcIndex);
        }
    }
}

void BytecodePreprocessor::CreateBlockInfoEntries()
{
    blocksInfo_.reserve(blockStarts_.size());
    uint32_t blockId = 0;
    for (uint32_t start : blockStarts_) {
        blocksInfo_.emplace_back(BlockInfo(blockId++, start));
    }

    // Set end indices
    for (size_t i = 0; i < blocksInfo_.size(); ++i) {
        if (i + 1 < blocksInfo_.size()) {
            blocksInfo_[i].endBcIndex = blocksInfo_[i + 1].startBcIndex - 1;
        } else {
            blocksInfo_[i].endBcIndex = lastBcIndex_;
        }
    }

    index2BlockId_.resize(lastBcIndex_ + 1);
    for (const auto &block : blocksInfo_) {
        for (uint32_t bcIndex = block.startBcIndex; bcIndex <= block.endBcIndex; ++bcIndex) {
            index2BlockId_[bcIndex] = block.id;
        }
    }
}

void BytecodePreprocessor::BuildCFGEdges()
{
    const auto &bytecodeInfo = context_->GetInfoData();
    for (auto &block : blocksInfo_) {
        const auto &info = bytecodeInfo[block.endBcIndex];

        uint32_t nextBcIndex = block.endBcIndex + 1;
        if (info.IsJump()) {
            uint32_t targetIndex = context_->GetJumpTargetBcIndex(block.endBcIndex);
            ASSERT(targetIndex <= lastBcIndex_);
            uint32_t targetBlockId = FindBlockIdByBcIndex(targetIndex);
            block.jump.emplace_back(targetBlockId);

            if (info.IsCondJump() && nextBcIndex <= lastBcIndex_) {
                uint32_t nextBlockId = FindBlockIdByBcIndex(nextBcIndex);
                block.succ.emplace_back(nextBlockId);
            }
        } else if (!info.IsReturn() && !info.IsThrow()) {
            // Fall through to next block
            if (nextBcIndex <= lastBcIndex_) {
                uint32_t nextBlockId = FindBlockIdByBcIndex(nextBcIndex);
                block.succ.emplace_back(nextBlockId);
            }
        }

        // Handle try-catch edges
        if (!hasTryCatch_) {
            continue;
        }
        bool canThrow = false;
        for (uint32_t bcIndex = block.startBcIndex; bcIndex <= block.endBcIndex; ++bcIndex) {
            const auto &bcInfo = bytecodeInfo[bcIndex];
            if (bcInfo.IsGeneral() && !bcInfo.NoThrow()) {
                canThrow = true;
                break;
            }
        }
        if (!canThrow) {
            continue;
        }
        for (const auto &exItem : context_->GetExceptionInfo()) {
            if (block.startBcIndex < exItem.startBcIndex || block.startBcIndex >= exItem.endBcIndex) {
                continue;
            }
            for (uint32_t catchBcIndex : exItem.catchBcIndices) {
                uint32_t catchBlockId = FindBlockIdByBcIndex(catchBcIndex);
                block.catches.emplace_back(catchBlockId);
            }
        }
        block.SortCatches(blocksInfo_);
    }
}

void BytecodePreprocessor::BuildPostOrderListAndReorderBytecode()
{
    postOrderList_.clear();
    std::deque<size_t> pendingList;
    std::vector<bool> visited(blocksInfo_.size(), false);
    auto firstBlockId = 0;
    pendingList.emplace_back(firstBlockId);

    while (!pendingList.empty()) {
        size_t blockId = pendingList.back();
        bool changed = false;
        visited[blockId] = true;

        for (const auto &jumpId : blocksInfo_[blockId].jump) {
            if (!visited[jumpId]) {
                pendingList.emplace_back(jumpId);
                changed = true;
                break;
            }
        }
        if (changed) {
            continue;
        }

        // to do: (catch)

        for (const auto &succId : blocksInfo_[blockId].succ) {
            if (!visited[succId]) {
                pendingList.emplace_back(succId);
                changed = true;
                break;
            }
        }
        if (changed) {
            continue;
        }

        uint32_t startBcIndex = blocksInfo_[blockId].startBcIndex;
        uint32_t endBcIndex = blocksInfo_[blockId].endBcIndex;
        for (uint32_t index = endBcIndex;; --index) {
            postOrderList_.emplace_back(index);
            if (index == startBcIndex) {
                break;
            }
        }
        pendingList.pop_back();
    }

    std::reverse(postOrderList_.begin(), postOrderList_.end());

    // Build index2PostOrderList mapping
    index2PostOrderList_.resize(lastBcIndex_ + 1);
    for (size_t i = 0; i < postOrderList_.size(); ++i) {
        uint32_t index = postOrderList_[i];
        index2PostOrderList_[index] = static_cast<uint32_t>(i);
    }

    CalculatePredecessorCounts(visited);
#ifndef NDEBUG
    DumpPostOrderListAndCFG();
#endif
}

void BytecodePreprocessor::PrintJumpTarget(const BlockInfo &block)
{
    if (!block.jump.empty()) {
        std::string jumpStr = "jmp=[";
        for (size_t i = 0; i < block.jump.size(); ++i) {
            jumpStr += std::to_string(block.jump[i]);
            if (i < block.jump.size() - 1) {
                jumpStr += ",";
            }
        }
        jumpStr += "]";
        LOG_COMPILER(INFO) << "  " << jumpStr;
    }
}

void BytecodePreprocessor::PrintFallThroughSuccessors(const BlockInfo &block)
{
    if (!block.succ.empty()) {
        std::string succStr = "succ=[";
        for (size_t j = 0; j < block.succ.size(); ++j) {
            succStr += std::to_string(block.succ[j]);
            if (j < block.succ.size() - 1) {
                succStr += ",";
            }
        }
        succStr += "]";
        LOG_COMPILER(INFO) << "  " << succStr;
    }
}

void BytecodePreprocessor::DumpPostOrderListAndCFG()
{
    LOG_COMPILER(INFO) << "========== PostOrder List (RPO) ==========";
    LOG_COMPILER(INFO) << "Total bytecode indices: " << postOrderList_.size();

    // Build bcIndex to blockId map
    std::map<uint32_t, uint32_t> bcIndexToBlockId;
    for (const auto &block : blocksInfo_) {
        for (uint32_t idx = block.startBcIndex; idx <= block.endBcIndex; ++idx) {
            bcIndexToBlockId[idx] = block.id;
        }
    }

    const auto &bytecodeInfo = context_->GetInfoData();
    for (size_t i = 0; i < postOrderList_.size(); ++i) {
        uint32_t bcIndex = postOrderList_[i];
        const auto &info = bytecodeInfo[bcIndex];
        uint32_t blockId = bcIndexToBlockId[bcIndex];
        LOG_COMPILER(INFO) << "[" << i << "] bcIndex=" << bcIndex << " blockId=" << blockId
                           << " opcode=" << GetEcmaOpcodeStr(info.GetOpcode());
    }
    LOG_COMPILER(INFO) << "====================================";

    // Print basic block CFG information
    LOG_COMPILER(INFO) << "========== Basic Block CFG ==========";
    LOG_COMPILER(INFO) << "Total blocks: " << blocksInfo_.size();
    for (const auto &block : blocksInfo_) {
        LOG_COMPILER(INFO) << "Block[" << block.id << "] bcRange=[" << block.startBcIndex << "," << block.endBcIndex
                           << "]";
        PrintJumpTarget(block);
        PrintFallThroughSuccessors(block);
    }
    LOG_COMPILER(INFO) << "====================================";
}

void BytecodePreprocessor::CalculatePredecessorCounts(const std::vector<bool> &visited)
{
    predecessorCount_.resize(lastBcIndex_ + 1, 1);
    predecessorCount_[0] += 1;
    for (size_t i = 0; i < blocksInfo_.size(); ++i) {
        if (visited[i]) {
            predecessorCount_[blocksInfo_[i].startBcIndex] -= 1;
            for (const auto &jumpId : blocksInfo_[i].jump) {
                predecessorCount_[blocksInfo_[jumpId].startBcIndex] += 1;
            }
            for (const auto &succId : blocksInfo_[i].succ) {
                predecessorCount_[blocksInfo_[succId].startBcIndex] += 1;
            }
            // to do: catch blocks need to be handled later
        }
    }
}

bool BytecodePreprocessor::IsLogEnabled() const
{
    return context_->IsLogEnabled();
}

}  // namespace panda::ecmascript::arksteed
