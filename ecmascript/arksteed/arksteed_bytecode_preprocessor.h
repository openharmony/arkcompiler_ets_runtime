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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_PREPROCESSOR_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_PREPROCESSOR_H

#include "ecmascript/arksteed/arksteed_compiler.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/chunk_containers.h"
#include "libpandafile/bytecode_instruction.h"

namespace panda::ecmascript::arksteed {
using namespace panda::ecmascript::kungfu;

// Basic block information
struct BlockInfo {
    uint32_t id;                    // Block ID
    uint32_t startBcIndex;          // Start bytecode index
    uint32_t endBcIndex;            // End bytecode index
    std::vector<uint32_t> jump;     // Jump target block IDs
    std::vector<uint32_t> succ;     // Fall-through successor block IDs
    std::vector<uint32_t> catches;  // Catch block IDs (for try blocks)

    BlockInfo() : id(0), startBcIndex(0), endBcIndex(0) {}

    BlockInfo(uint32_t id, uint32_t start) : id(id), startBcIndex(start), endBcIndex(0) {}

    // Sort catch blocks by startBcIndex (execution order)
    void SortCatches(const ChunkVector<BlockInfo> &blocksInfo)
    {
        if (catches.empty()) {
            return;
        }
        std::sort(catches.begin(), catches.end(), [&blocksInfo](uint32_t a, uint32_t b) {
            return blocksInfo[a].startBcIndex < blocksInfo[b].startBcIndex;
        });
    }
};

class BytecodeContext;  // Forward declaration

class BytecodePreprocessor {
public:
    explicit BytecodePreprocessor(Chunk *chunk)
        : blockStarts_(chunk),
          blocksInfo_(chunk),
          index2BlockId_(chunk),
          postOrderList_(chunk),
          index2PostOrderList_(chunk),
          predecessorCount_(chunk)
    {}
    ~BytecodePreprocessor() = default;

    void Initialize(BytecodeContext *context);

    // Block information accessors
    const BlockInfo &GetBlockById(uint32_t blockId) const
    {
        return blocksInfo_[blockId];
    }

    size_t GetBlockCount() const
    {
        return blocksInfo_.size();
    }

    const ChunkVector<uint32_t> &GetIndex2BlockId() const
    {
        return index2BlockId_;
    }

    const ChunkVector<uint32_t> &GetPostOrderList() const
    {
        return postOrderList_;
    }

    ChunkVector<uint32_t> &GetPostOrderList()
    {
        return postOrderList_;
    }

    const ChunkVector<uint32_t> &GetIndex2PostOrderList() const
    {
        return index2PostOrderList_;
    }

    const ChunkVector<uint32_t> &GetPredecessorCount() const
    {
        return predecessorCount_;
    }

    ChunkVector<uint32_t> &GetPredecessorCount()
    {
        return predecessorCount_;
    }

    ChunkVector<uint32_t> &GetIndex2PostOrderList()
    {
        return index2PostOrderList_;
    }

    uint32_t FindBlockIdByBcIndex(uint32_t bcIndex) const
    {
        // to do: Refactor this algorithm to O(1) complexity
        auto it =
            std::upper_bound(blocksInfo_.begin(), blocksInfo_.end(), bcIndex, [](uint32_t idx, const BlockInfo &block) {
                return idx < block.startBcIndex;
            });
        ASSERT(it != blocksInfo_.begin());
        return (it - 1)->id;
    }

    bool IsLogEnabled() const;

private:
    void BuildBasicBlocksAndReorderBytecode();
    void BuildBasicBlock();
    void MarkExceptionBlockStarts();
    void CreateBlockInfoEntries();
    void BuildCFGEdges();
    void BuildPostOrderListAndReorderBytecode();
    void CalculatePredecessorCounts(const std::vector<bool> &visited);
    void PrintJumpTarget(const BlockInfo &block);
    void PrintFallThroughSuccessors(const BlockInfo &block);
    void DumpPostOrderListAndCFG();

    BytecodeContext *context_ = nullptr;  // Reference to BytecodeContext (not owned)

    ChunkSet<uint32_t> blockStarts_;
    ChunkVector<BlockInfo> blocksInfo_;
    ChunkVector<uint32_t> index2BlockId_;
    ChunkVector<uint32_t> postOrderList_;
    ChunkVector<uint32_t> index2PostOrderList_;
    ChunkVector<uint32_t> predecessorCount_;

    uint32_t lastBcIndex_ = 0;
    bool hasTryCatch_ = false;  // True if method has try-catch blocks
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_PREPROCESSOR_H
