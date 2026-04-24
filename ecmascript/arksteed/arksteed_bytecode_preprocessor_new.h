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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_PREPROCESSOR_NEW_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_PREPROCESSOR_NEW_H

#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/base/bit_set.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
class BytecodePreprocessorNew {
public:
    static constexpr uint32_t NULL_INDEX = static_cast<uint32_t>(-1);

    struct TryBlockInfo {
        // [startBcIndex, endBcIndex]
        uint32_t startBcIndex;
        uint32_t endBcIndex;
        ChunkVector<uint32_t> catchBcIndices;

        bool ContainsBytecode(uint32_t bcIndex) const;
    };

    struct BasicBlockInfo {
        // Index by the RPO order. NULL_INDEX if this block is dead (inaccessible from the start).
        uint32_t rpoIndex;
        // [startBcIndex, endBcIndex]
        uint32_t startBcIndex;
        uint32_t endBcIndex;
        // nullptr if this block is terminating (RETURN, THROW) or unconditional jump.
        const BasicBlockInfo *fallthroughBlock;
        // nullptr if this block is not a jump.
        const BasicBlockInfo *jumpBlock;
        // Header of the innermost loop that current block belongs to.
        // nullptr if this block does not belong to any loop.
        // If this block is already a loop header (IsLoopHeader() return true),
        // loopHeaderBlock points to the parent loop header if such parent exists, or nullptr otherwise.
        const BasicBlockInfo *loopHeaderBlock;
        // If this block is a loop header, then loopBackBlock is the one from which the loop jumps back to header.
        // nullptr if this block is not a loop header.
        const BasicBlockInfo *loopBackBlock;
        //
        ChunkVector<const BasicBlockInfo *> jumpPredecessors;
        // Empty list if one of the following happens:
        // (1) this block is never caught in the input bytecode, or
        // (2) no bytecode can throw exception in this block.
        ChunkVector<const BasicBlockInfo *> catchBlocks;

        bool ContainsBytecode(uint32_t bcIndex) const;
        bool HasFallthrough() const;
        bool IsJump() const;
        bool IsConditionalJump() const;
        bool IsDead() const;
        bool IsLoopHeader() const;
    };

    struct BytecodeInfo {
        // Offset bytes inside this method.
        uint32_t offset;
        // Which block it belongs to.
        uint32_t blockIndex;
        // Parsing result of current bytecode instruction.
        // For jump instructions, only opcode metadata is loaded into details.
        kungfu::BytecodeInfo details;
    };

    // Constructs & finishes the bytecode preprocessor
    BytecodePreprocessorNew(JitCompilationEnv *env, Chunk *chunk);

    uint32_t GetNumLiveBasicBlocks() const
    {
        return static_cast<uint32_t>(rpoList_.size());
    }

    uint32_t GetNumAllBasicBlocks() const
    {
        return static_cast<uint32_t>(basicBlocks_.size());
    }

    const BasicBlockInfo *GetBasicBlockByRPO(uint32_t rpoIndex) const
    {
        return rpoList_[rpoIndex];
    }

    const BasicBlockInfo *GetBasicBlockByBCOrder(uint32_t blockIndex) const
    {
        return &basicBlocks_[blockIndex];
    }

    const BytecodeInfo *GetBytecode(uint32_t bcIndex) const
    {
        return &bytecodes_[bcIndex];
    }

    VRegIDType GetNumLocalVRegs() const
    {
        return numLocalVRegs_;
    }

    VRegIDType GetNumParamVRegs() const
    {
        return numParamVRegs_;
    }

    Chunk *GetChunk() const
    {
        return tryBlocks_.get_allocator().chunk();
    }

    std::string Dump() const;
    std::string DumpBasicBlocksString() const;
    std::string DumpTryBlocksString() const;

private:
    uint32_t JumpTargetBcIndexOfBytecode(uint32_t bcIndex);

    uint32_t AppendSyntheticJump(uint32_t targetBlockIndex, uint32_t numJumpPredecessors);

    void CollectBytecodeInfo();
    void CollectTryCatchBlockInfo();
    void BuildBasicBlocks();
    void MarkBasicBlockStarts(kungfu::BitSet *isBlockStart, uint32_t bcCount);
    uint32_t CreateBasicBlocks(const kungfu::BitSet &isBlockStart, uint32_t bcCount);
    void InitializeBlockEdges(uint32_t blockCount);
    struct CanonicalizeLoopsDFS;
    void SplitCriticalEdges();
    void SetBasicBlockPointers();
    void LoopAnalysis();
    void MakeRPO();
    void ClearDeadPredecessors();

    JitCompilationEnv *env_;
    MethodLiteral *method_;

    VRegIDType numLocalVRegs_;
    VRegIDType numParamVRegs_;

    ChunkVector<TryBlockInfo> tryBlocks_;
    ChunkVector<BasicBlockInfo> basicBlocks_;
    ChunkVector<BytecodeInfo> bytecodes_;
    ChunkVector<const BasicBlockInfo *> rpoList_;

    // Auxiliary data
    ChunkVector<uint32_t> bcIndexOfOffset_;
    ChunkVector<uint32_t> jumpTargetBcIndices_;
    ChunkVector<uint32_t> loopHeaders_;
    ChunkVector<uint32_t> numJumpPredecessors_;
};

inline bool BytecodePreprocessorNew::TryBlockInfo::ContainsBytecode(uint32_t bcIndex) const
{
    return bcIndex >= startBcIndex && bcIndex <= endBcIndex;
}

inline bool BytecodePreprocessorNew::BasicBlockInfo::ContainsBytecode(uint32_t bcIndex) const
{
    return bcIndex >= startBcIndex && bcIndex <= endBcIndex;
}

inline bool BytecodePreprocessorNew::BasicBlockInfo::HasFallthrough() const
{
    return fallthroughBlock != nullptr;
}

inline bool BytecodePreprocessorNew::BasicBlockInfo::IsJump() const
{
    return jumpBlock != nullptr;
}

inline bool BytecodePreprocessorNew::BasicBlockInfo::IsConditionalJump() const
{
    return jumpBlock != nullptr && fallthroughBlock != nullptr;
}

inline bool BytecodePreprocessorNew::BasicBlockInfo::IsDead() const
{
    return rpoIndex == NULL_INDEX;
}

inline bool BytecodePreprocessorNew::BasicBlockInfo::IsLoopHeader() const
{
    return loopBackBlock != nullptr;
}
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_PREPROCESSOR_NEW_H
