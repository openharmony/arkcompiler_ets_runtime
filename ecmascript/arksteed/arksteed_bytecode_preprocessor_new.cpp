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

#include "ecmascript/arksteed/arksteed_bytecode_preprocessor_new.h"

#include "code_data_accessor-inl.h"  // IWYU pragma: keep
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/base/bit_set.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "method_data_accessor-inl.h"  // IWYU pragma: keep

namespace panda::ecmascript::arksteed {
static kungfu::Bytecodes g_bytecodes;

using BasicBlockInfo = BytecodePreprocessorNew::BasicBlockInfo;

#define BLOCK_INDEX_FROM_PTR(ptr) (static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr)))
#define BLOCK_INDEX_TO_PTR(index) (reinterpret_cast<BasicBlockInfo *>(static_cast<uintptr_t>(index)))

namespace {
uint32_t NumParamVRegsOf(MethodLiteral *method)
{
    uint32_t res = method->GetNumArgsWithCallField();
    // to do: ArkSteed calling convention is unclear currently.
    res += method->IsFastCall() ? static_cast<uint32_t>(kungfu::FastCallArgIdx::NUM_OF_ARGS)
                                : static_cast<uint32_t>(kungfu::CommonArgIdx::NUM_OF_ARGS);
    return res;
}
}  // namespace

uint32_t BytecodePreprocessorNew::JumpTargetBcIndexOfBytecode(uint32_t bcIndex)
{
    // Used by READ_INST_*_0() macros below
    const uint8_t *pc = env_->GetMethodPcStart() + bytecodes_[bcIndex].offset;
    int32_t jumpOffset = 0;
    switch (bytecodes_[bcIndex].details.GetOpcode()) {
        case kungfu::EcmaOpcode::JEQZ_IMM8:
        case kungfu::EcmaOpcode::JNEZ_IMM8:
        case kungfu::EcmaOpcode::JMP_IMM8:
            jumpOffset = static_cast<int8_t>(READ_INST_8_0());
            break;
        case kungfu::EcmaOpcode::JNEZ_IMM16:
        case kungfu::EcmaOpcode::JEQZ_IMM16:
        case kungfu::EcmaOpcode::JMP_IMM16:
            jumpOffset = static_cast<int16_t>(READ_INST_16_0());
            break;
        case kungfu::EcmaOpcode::JMP_IMM32:
        case kungfu::EcmaOpcode::JNEZ_IMM32:
        case kungfu::EcmaOpcode::JEQZ_IMM32:
            jumpOffset = static_cast<int32_t>(READ_INST_32_0());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    uint32_t jumpTargetOffset = static_cast<uint32_t>(bytecodes_[bcIndex].offset + jumpOffset);
    return bcIndexOfOffset_[jumpTargetOffset];
}

// (1) This function should be called before SetBasicBlockPointers().
//     In this helper we store indices instead of pointers into the pointer fields.
//     See below for details.
// (2) This function adds elements to both bytecodes_ and basicBlocks_ vectors.
//     Be careful with pointer & reference invalidated.
uint32_t BytecodePreprocessorNew::AppendSyntheticJump(uint32_t targetBlockIndex, uint32_t numJumpPredecessors)
{
    constexpr uint8_t DUMMY_JUMP_OPCODE = static_cast<uint8_t>(kungfu::EcmaOpcode::JMP_IMM8);

    kungfu::BytecodeInfo details;
    details.SetMetaData(g_bytecodes.GetBytecodeMetaData(&DUMMY_JUMP_OPCODE));

    uint32_t fakeJumpBcIndex = static_cast<uint32_t>(bytecodes_.size());
    uint32_t fakeJumpBlockIndex = static_cast<uint32_t>(basicBlocks_.size());

    bytecodes_.emplace_back(BytecodeInfo{
        // Synthetic jump instruction. No meaningful offset value.
        .offset = NULL_INDEX,
        .blockIndex = fakeJumpBlockIndex,
        .details = std::move(details),
    });

    ASSERT(basicBlocks_.size() == numJumpPredecessors_.size());

    basicBlocks_.emplace_back(BasicBlockInfo{
        .rpoIndex = NULL_INDEX,  // To be initialized later
        // Only the synthetic jump instruction
        .startBcIndex = fakeJumpBcIndex,
        .endBcIndex = fakeJumpBcIndex,
        // No fallthrough
        .fallthroughBlock = BLOCK_INDEX_TO_PTR(NULL_INDEX),
        .jumpBlock = BLOCK_INDEX_TO_PTR(targetBlockIndex),
        // To be initialized later
        .loopHeaderBlock = nullptr,
        .loopBackBlock = BLOCK_INDEX_TO_PTR(NULL_INDEX),
        // To be initialized later
        .jumpPredecessors = ChunkVector<const BasicBlockInfo *>(GetChunk()),
        // No catch blocks_
        .catchBlocks = ChunkVector<const BasicBlockInfo *>(GetChunk()),
    });
    numJumpPredecessors_.emplace_back(numJumpPredecessors);
    return fakeJumpBlockIndex;
}

// -------- Initialization steps: called one-by-one --------

void BytecodePreprocessorNew::CollectBytecodeInfo()
{
    uint32_t bcSizeBytes = MethodLiteral::GetCodeSize(env_->GetJSPandaFile(), method_->GetMethodId());
    bytecodes_.reserve(bcSizeBytes);
    bcIndexOfOffset_.resize(bcSizeBytes + 1, NULL_INDEX);

    const uint8_t *startPc = env_->GetMethodPcStart();
    BytecodeInstruction bcIns(startPc);
    BytecodeInstruction bcInsLast = bcIns.JumpTo(bcSizeBytes);

    VRegIDType envVRegIndex = VRegOfEnv(numLocalVRegs_, numParamVRegs_).GetId();
    auto makeBytecodeDetails = [startPc, envVRegIndex](uint32_t curOffset) {
        kungfu::BytecodeInfo res;
        res.SetMetaData(g_bytecodes.GetBytecodeMetaData(startPc + curOffset));
        // For jump instructions, only the opcode metadata is loaded
        if (!res.GetMetaData().IsJump()) {
            kungfu::BytecodeInfo::InitBytecodeInfo(res, startPc + curOffset, curOffset, envVRegIndex);
        }
        return res;
    };

    for (; bcIns.GetAddress() != bcInsLast.GetAddress(); bcIns = bcIns.GetNext()) {
        const uint8_t *curPc = bcIns.GetAddress();
        uint32_t curOffset = static_cast<uint32_t>(curPc - startPc);
        bcIndexOfOffset_[curOffset] = static_cast<uint32_t>(bytecodes_.size());

        bytecodes_.emplace_back(BytecodeInfo{
            .offset = curOffset,
            .blockIndex = NULL_INDEX,
            .details = makeBytecodeDetails(curOffset),
        });
    }

    uint32_t bcCount = static_cast<uint32_t>(bytecodes_.size());
    // May be used in CollectTryCatchBlockInfo() when converting endBcIndex from offset
    bcIndexOfOffset_[bcSizeBytes] = bcCount;

    jumpTargetBcIndices_.resize(bcCount, NULL_INDEX);
    for (uint32_t i = 0; i < bcCount; i++) {
        if (bytecodes_[i].details.IsJump()) {
            jumpTargetBcIndices_[i] = JumpTargetBcIndexOfBytecode(i);
        }
    }
}

void BytecodePreprocessorNew::CollectTryCatchBlockInfo()
{
    const panda_file::File *pf = env_->GetJSPandaFile()->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, method_->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf, mda.GetCodeId().value());

    cda.EnumerateTryBlocks([this](panda_file::CodeDataAccessor::TryBlock &tryBlock) {
        // Half-open range [tryStartOffset, tryEndOffset) read from Panda file
        uint32_t tryStartOffset = tryBlock.GetStartPc();
        uint32_t tryEndOffset = tryBlock.GetStartPc() + tryBlock.GetLength();
        if (tryStartOffset == tryEndOffset) {
            return true;
        }
        // Converts to closed range [startBcIndex, endBcIndex]
        uint32_t startBcIndex = bcIndexOfOffset_[tryStartOffset];
        uint32_t endBcIndex = bcIndexOfOffset_[tryEndOffset] - 1;

        TryBlockInfo curInfoItem{startBcIndex, endBcIndex, ChunkVector<uint32_t>{GetChunk()}};
        tryBlocks_.emplace_back(std::move(curInfoItem));

        tryBlock.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catchBlock) {
            uint32_t pcOffset = catchBlock.GetHandlerPc();
            uint32_t catchBcIndex = bcIndexOfOffset_[pcOffset];
            tryBlocks_.back().catchBcIndices.emplace_back(catchBcIndex);
            return true;
        });
        return true;
    });
}

// Note: A hack is used before the graph is fully constructed,
//       in which block indices (in basicBlocks_ array) are stored into the const BasicBlockInfo* fields
//       to prevent pointer invalidation after vector relocation,
//       as new basic blocks_ will be added on splitting critical edges (see below).
//       Equivalent to union {
//           /* Before the graph is finished, we use this field temporarily */
//           uintptr_t jumpBlockIndex;
//           /* After the graph is finished, we use this field */
//           const BasicBlockInfo *jumpBlock;
//       } yet we do not expose this union to keep a clean, user-friendly interface design.

void BytecodePreprocessorNew::BuildBasicBlocks()
{
    uint32_t bcCount = static_cast<uint32_t>(bytecodes_.size());
    if (bcCount == 0) {
        return;
    }

    kungfu::BitSet isBlockStart(GetChunk(), bcCount);
    MarkBasicBlockStarts(&isBlockStart, bcCount);
    uint32_t blockCount = CreateBasicBlocks(isBlockStart, bcCount);
    InitializeBlockEdges(blockCount);
}

void BytecodePreprocessorNew::MarkBasicBlockStarts(kungfu::BitSet *isBlockStart, uint32_t bcCount)
{
    bool nextIsBlockStart = false;
    for (uint32_t i = 0; i < bcCount; i++) {
        if (nextIsBlockStart) {
            isBlockStart->SetBit(i);
            nextIsBlockStart = false;
        }
        const BytecodeInfo &curBcInfo = bytecodes_[i];
        if (curBcInfo.details.IsJump()) {
            isBlockStart->SetBit(jumpTargetBcIndices_[i]);
            nextIsBlockStart = true;
        } else if (curBcInfo.details.IsThrow() || curBcInfo.details.IsReturn()) {
            nextIsBlockStart = true;
        }
    }
    for (const TryBlockInfo &tryBlock : tryBlocks_) {
        isBlockStart->SetBit(tryBlock.startBcIndex);
        if (tryBlock.endBcIndex + 1 < bcCount) {
            isBlockStart->SetBit(tryBlock.endBcIndex + 1);
        }
        for (uint32_t catchBc : tryBlock.catchBcIndices) {
            isBlockStart->SetBit(catchBc);
        }
    }
}

uint32_t BytecodePreprocessorNew::CreateBasicBlocks(const kungfu::BitSet &isBlockStart, uint32_t bcCount)
{
    uint32_t startBcIndex = 0;
    uint32_t blockCount = 0;

    auto appendBasicBlock = [&](uint32_t nextStartBcIndex) {
        basicBlocks_.emplace_back(BasicBlockInfo{
            .rpoIndex = NULL_INDEX,  // To be initialized later
            .startBcIndex = startBcIndex,
            .endBcIndex = nextStartBcIndex - 1,
            // The following are all to be initialized later
            .fallthroughBlock = BLOCK_INDEX_TO_PTR(NULL_INDEX),
            .jumpBlock = BLOCK_INDEX_TO_PTR(NULL_INDEX),
            .loopHeaderBlock = nullptr,
            .loopBackBlock = BLOCK_INDEX_TO_PTR(NULL_INDEX),
            .jumpPredecessors = ChunkVector<const BasicBlockInfo *>(GetChunk()),
            .catchBlocks = ChunkVector<const BasicBlockInfo *>(GetChunk()),
        });
        startBcIndex = nextStartBcIndex;
        blockCount += 1;
    };

    bytecodes_[0].blockIndex = 0;
    for (uint32_t i = 1; i < bcCount; i++) {
        if (isBlockStart.TestBit(i)) {
            appendBasicBlock(i);
        }
        bytecodes_[i].blockIndex = blockCount;
    }
    appendBasicBlock(bcCount);

    // To be initialized later (in CanonicalizeLoopsDFS)
    numJumpPredecessors_.assign(blockCount, 0);
    return blockCount;
}

void BytecodePreprocessorNew::InitializeBlockEdges(uint32_t blockCount)
{
    for (uint32_t i = 0; i < blockCount; i++) {
        BasicBlockInfo &curBlock = basicBlocks_[i];
        ASSERT(curBlock.startBcIndex <= curBlock.endBcIndex && "Expects at least 1 bytecode instruction");

        const BytecodeInfo &lastBc = bytecodes_[curBlock.endBcIndex];
        bool isJump = lastBc.details.IsJump();
        bool isUnconditionalJump = isJump && !lastBc.details.IsCondJump();
        if (isJump) {
            uint32_t jumpTargetBcIndex = jumpTargetBcIndices_[curBlock.endBcIndex];
            uint32_t jumpTargetBlockIndex = bytecodes_[jumpTargetBcIndex].blockIndex;
            curBlock.jumpBlock = BLOCK_INDEX_TO_PTR(jumpTargetBlockIndex);
        }
        if (!lastBc.details.IsReturn() && !lastBc.details.IsThrow() && !isUnconditionalJump) {
            ASSERT(i + 1 < blockCount && "Malformed bytecode");
            curBlock.fallthroughBlock = BLOCK_INDEX_TO_PTR(i + 1);
        }
        bool throws = false;
        for (uint32_t j = curBlock.startBcIndex; j <= curBlock.endBcIndex; j++) {
            if (bytecodes_[j].details.IsGeneral() && !bytecodes_[j].details.NoThrow()) {
                throws = true;
                break;
            }
        }
        if (!throws) {
            continue;
        }
        for (const auto &tryBlock : tryBlocks_) {
            if (!tryBlock.ContainsBytecode(curBlock.startBcIndex)) {
                continue;
            }
            for (uint32_t catchBcIndex : tryBlock.catchBcIndices) {
                uint32_t catchBlockIndex = bytecodes_[catchBcIndex].blockIndex;
                curBlock.catchBlocks.emplace_back(BLOCK_INDEX_TO_PTR(catchBlockIndex));
            }
        }
    }
}

// Ensures all loops are in a canonical form where:
// (1) each loop ends with an unconditional jump block (named J) which jumps to the header;
// (2) J is the only block inside this loop which jumps to the header.
// Precondition: The input bytecode sequence forms reducible loops only.
struct BytecodePreprocessorNew::CanonicalizeLoopsDFS {
    // DFS states (white: unvisited, grey: in DFS stack; black: visited)
    enum : uint8_t { WHITE = 0, GREY = 1, BLACK = 2 };
    enum : uint8_t { NOT_REDIRECTED = 0, LOOP_BACK_REDIRECTED = 1, LOOP_ENTRY_REDIRECTED = 2 };

    BytecodePreprocessorNew *parent_;
    ChunkVector<BasicBlockInfo> &blocks_;
    ChunkVector<uint32_t> &numJumpPredecessors_;
    ChunkVector<uint8_t> colors_;
    ChunkVector<uint8_t> redirection_;
    ChunkVector<uint32_t> dfsPredecessor_;

    explicit CanonicalizeLoopsDFS(BytecodePreprocessorNew *parent)
        : parent_(parent),
          blocks_(parent->basicBlocks_),
          numJumpPredecessors_(parent->numJumpPredecessors_),
          colors_(blocks_.size(), WHITE, parent->GetChunk()),
          redirection_(blocks_.size(), NOT_REDIRECTED, parent->GetChunk()),
          dfsPredecessor_(blocks_.size(), NULL_INDEX, parent->GetChunk())
    {}

    void Run() &&
    {
        uint32_t blockCount = static_cast<uint32_t>(blocks_.size());
        for (uint32_t i = 0; i < blockCount; i++) {
            if (colors_[i] == WHITE) {
                DoDFS(i);
            }
        }
    }

    void DoDFS(uint32_t blockIndex)
    {
        colors_[blockIndex] = GREY;
        TryVisitSuccessor<0>(blockIndex);  // INDEX = 0: fallthrough
        TryVisitSuccessor<1>(blockIndex);  // INDEX = 1: jump
        colors_[blockIndex] = BLACK;
        // Accessible loop headers are added to the list by post-order,
        // so that inner loop is always before its parent_.
        if (BLOCK_INDEX_FROM_PTR(blocks_[blockIndex].loopBackBlock) != NULL_INDEX) {
            parent_->loopHeaders_.push_back(blockIndex);
        }
    }

    // (1) Be careful when you use pointers or references to items in basicBlocks_
    //     since they may be invalidated after relocation on adding basic blocks_ via AppendSyntheticJump().
    // (2) All BasicBlockInfo "pointers" are indices actually.
    //     Use BLOCK_INDEX_FROM_PTR and BLOCK_INDEX_TO_PTR to extract and store the index value.
    template <int INDEX>
    void TryVisitSuccessor(uint32_t blockIndex)
    {
        uint32_t toIndex = GetToIndex<INDEX>(blockIndex);
        if (toIndex == NULL_INDEX) {
            return;
        }
        if (colors_[toIndex] == GREY) {
            TryVisitGreySuccessor<INDEX>(blockIndex, toIndex);
        } else if (colors_[toIndex] == BLACK) {
            TryVisitBlackSuccessor<INDEX>(blockIndex, toIndex);
        } else {
            numJumpPredecessors_[toIndex] += 1;
            DoDFS(toIndex);
        }
        dfsPredecessor_[toIndex] = blockIndex;
    }

    template <int INDEX>
    void TryVisitGreySuccessor(uint32_t blockIndex, uint32_t toIndex)
    {
        // We encounter a loop. The header block is in the DFS stack.
        uint32_t prevLoopBackIndex = BLOCK_INDEX_FROM_PTR(blocks_[toIndex].loopBackBlock);
        if (LIKELY(prevLoopBackIndex == NULL_INDEX)) {
            // (1) blockIndex -> toIndex is the first loop back edge we've ever met.
            blocks_[toIndex].loopBackBlock = BLOCK_INDEX_TO_PTR(blockIndex);
            numJumpPredecessors_[toIndex] += 1;
        } else if (redirection_[toIndex] == NOT_REDIRECTED) {
            // (2) blockIndex -> toIndex is the second loop back edge.
            //     We need to create a common loop back block (index denoted as RB) and then
            //     redirect as blockIndex -> RB -> toIndex
            redirection_[toIndex] = LOOP_BACK_REDIRECTED;
            // RB -> toIndex. 2 : numPredecessors of RB (may be incremented later)
            uint32_t newLoopBackIndex = parent_->AppendSyntheticJump(toIndex, 2);
            blocks_[toIndex].loopBackBlock = BLOCK_INDEX_TO_PTR(newLoopBackIndex);
#ifndef NDEBUG
            LOG_COMPILER(DEBUG) << "Creates block #" << newLoopBackIndex << " as loop-back block of #" << toIndex;
#endif
            // prevLoopBackIndex -> RB
            BasicBlockInfo &prevLoopBackBlock = blocks_[prevLoopBackIndex];
            if (BLOCK_INDEX_FROM_PTR(prevLoopBackBlock.fallthroughBlock) == toIndex) {
                prevLoopBackBlock.fallthroughBlock = BLOCK_INDEX_TO_PTR(newLoopBackIndex);
            } else {
                ASSERT(BLOCK_INDEX_FROM_PTR(prevLoopBackBlock.jumpBlock) == toIndex);
                prevLoopBackBlock.jumpBlock = BLOCK_INDEX_TO_PTR(newLoopBackIndex);
            }
            // blockIndex -> RB
            RedirectTarget<INDEX>(blockIndex, newLoopBackIndex);
            // numJumpPredecessors_[toIndex] is not incremented
        } else {
            // (3) blockIndex -> toIndex is the third loop back edge or later.
            //     Redirects as blockIndex -> RB (created before)
            uint32_t loopBackBlockIndex = BLOCK_INDEX_FROM_PTR(blocks_[toIndex].loopBackBlock);
            RedirectTarget<INDEX>(blockIndex, loopBackBlockIndex);
            numJumpPredecessors_[loopBackBlockIndex] += 1;
        }
    }

    template <int INDEX>
    void TryVisitBlackSuccessor(uint32_t blockIndex, uint32_t toIndex)
    {
        if (BLOCK_INDEX_FROM_PTR(blocks_[toIndex].loopBackBlock) == NULL_INDEX) {
            numJumpPredecessors_[toIndex] += 1;
            return;
        }
        ASSERT(numJumpPredecessors_[toIndex] == 2);  // 2: One is entry and another is loop-back
        ASSERT(dfsPredecessor_[toIndex] != NULL_INDEX);

        if (redirection_[toIndex] != LOOP_ENTRY_REDIRECTED) {
            // (1) blockIndex -> toIndex is the second loop entry edge.
            //     We need to create a loop pre-header (index denoted as RE) and then
            //     redirect as blockIndex -> RE -> toIndex
            redirection_[toIndex] = LOOP_ENTRY_REDIRECTED;
            uint32_t prevEntryIndex = dfsPredecessor_[toIndex];
            // RE -> toIndex. 2 : numPredecessors or RE (may be incremented later)
            uint32_t preheaderIndex = parent_->AppendSyntheticJump(toIndex, 2);
            dfsPredecessor_[toIndex] = preheaderIndex;
#ifndef NDEBUG
            LOG_COMPILER(DEBUG) << "Creates block #" << preheaderIndex << " as pre-header block of #" << toIndex;
#endif
            // prevEntryIndex -> RE
            BasicBlockInfo &prevEntryBlock = blocks_[prevEntryIndex];
            if (BLOCK_INDEX_FROM_PTR(prevEntryBlock.fallthroughBlock) == toIndex) {
                prevEntryBlock.fallthroughBlock = BLOCK_INDEX_TO_PTR(preheaderIndex);
            } else {
                ASSERT(BLOCK_INDEX_FROM_PTR(prevEntryBlock.jumpBlock) == toIndex);
                prevEntryBlock.jumpBlock = BLOCK_INDEX_TO_PTR(preheaderIndex);
            }
            // blockIndex -> RE
            RedirectTarget<INDEX>(blockIndex, preheaderIndex);
        } else {
            // (2) blockIndex -> toIndex is the third loop entry edge or later.
            //     Redirects as blockIndex -> RE (created before)
            uint32_t preheaderIndex = dfsPredecessor_[toIndex];
            RedirectTarget<INDEX>(blockIndex, preheaderIndex);
            numJumpPredecessors_[preheaderIndex] += 1;
        }
    }

    template <int INDEX>
    uint32_t GetToIndex(uint32_t blockIndex) const
    {
        if constexpr (INDEX == 0) {
            return BLOCK_INDEX_FROM_PTR(blocks_[blockIndex].fallthroughBlock);
        } else {
            return BLOCK_INDEX_FROM_PTR(blocks_[blockIndex].jumpBlock);
        }
    }

    template <int INDEX>
    void RedirectTarget(uint32_t blockIndex, uint32_t targetIndex)
    {
        if constexpr (INDEX == 0) {
            blocks_[blockIndex].fallthroughBlock = BLOCK_INDEX_TO_PTR(targetIndex);
        } else {
            blocks_[blockIndex].jumpBlock = BLOCK_INDEX_TO_PTR(targetIndex);
        }
    }
};

void BytecodePreprocessorNew::SplitCriticalEdges()
{
    uint32_t blockCount = static_cast<uint32_t>(basicBlocks_.size());
    for (uint32_t i = 0; i < blockCount; i++) {
        uint32_t fallthroughIndex = BLOCK_INDEX_FROM_PTR(basicBlocks_[i].fallthroughBlock);
        uint32_t jumpIndex = BLOCK_INDEX_FROM_PTR(basicBlocks_[i].jumpBlock);

        if (fallthroughIndex == NULL_INDEX || jumpIndex == NULL_INDEX) {
            continue;
        }
        if (numJumpPredecessors_[fallthroughIndex] >= 2) {  // 2: critical edge threshold (needs split)
            // 1 : numJumpPredecessors_ = 1 (which is current block)
            uint32_t nextBlockIndex = AppendSyntheticJump(fallthroughIndex, 1);
#ifndef NDEBUG
            LOG_COMPILER(DEBUG) << "Edge-split (previously fallthrough): Block #" << i << " -> #" << nextBlockIndex
                                << " -> #" << fallthroughIndex;
#endif
            // Redirect loop-back block of the fallthrough target if necessary
            if (BLOCK_INDEX_FROM_PTR(basicBlocks_[fallthroughIndex].loopBackBlock) == i) {
                basicBlocks_[fallthroughIndex].loopBackBlock = BLOCK_INDEX_TO_PTR(nextBlockIndex);
            }
            basicBlocks_[i].fallthroughBlock = BLOCK_INDEX_TO_PTR(nextBlockIndex);
        }
        if (numJumpPredecessors_[jumpIndex] >= 2) {  // 2: critical edge threshold (needs split)
            // 1 : numJumpPredecessors_ = 1 (which is current block)
            uint32_t nextBlockIndex = AppendSyntheticJump(jumpIndex, 1);
#ifndef NDEBUG
            LOG_COMPILER(DEBUG) << "Edge-split (previously jump): Block #" << i << " -> #" << nextBlockIndex << " -> #"
                                << jumpIndex;
#endif
            // Redirect loop-back block of the jump target if necessary
            if (BLOCK_INDEX_FROM_PTR(basicBlocks_[jumpIndex].loopBackBlock) == i) {
                basicBlocks_[jumpIndex].loopBackBlock = BLOCK_INDEX_TO_PTR(nextBlockIndex);
            }
            basicBlocks_[i].jumpBlock = BLOCK_INDEX_TO_PTR(nextBlockIndex);
        }
    }
}

// Now the graph is completed. We can safely convert the block indices to corresponding pointers.
void BytecodePreprocessorNew::SetBasicBlockPointers()
{
    BasicBlockInfo *head = basicBlocks_.data();

    uint32_t blockCount = static_cast<uint32_t>(basicBlocks_.size());
    ASSERT(numJumpPredecessors_.size() == blockCount);
    for (uint32_t i = 0; i < blockCount; i++) {
        basicBlocks_[i].jumpPredecessors.assign(numJumpPredecessors_[i], nullptr);
    }
    ChunkVector<uint32_t> nextPredIndex(blockCount, 0, GetChunk());

    for (BasicBlockInfo &curBlock : basicBlocks_) {
        uint32_t loopBackIndex = BLOCK_INDEX_FROM_PTR(curBlock.loopBackBlock);
        curBlock.loopBackBlock = (loopBackIndex != NULL_INDEX) ? head + loopBackIndex : nullptr;
    }

    for (BasicBlockInfo &curBlock : basicBlocks_) {
        uint32_t fallthroughIndex = BLOCK_INDEX_FROM_PTR(curBlock.fallthroughBlock);
        if (fallthroughIndex != NULL_INDEX) {
            BasicBlockInfo *fallthroughBlock = head + fallthroughIndex;
            curBlock.fallthroughBlock = fallthroughBlock;
            // Order of predecessors except the loop-back block is unspecified
            fallthroughBlock->jumpPredecessors[nextPredIndex[fallthroughIndex]++] = &curBlock;
        } else {
            curBlock.fallthroughBlock = nullptr;
        }

        uint32_t jumpIndex = BLOCK_INDEX_FROM_PTR(curBlock.jumpBlock);
        if (jumpIndex != NULL_INDEX) {
            BasicBlockInfo *jumpBlock = head + jumpIndex;
            curBlock.jumpBlock = jumpBlock;
            // Whether curBlock -> jumpBlock is a loop-back edge
            if (jumpBlock->loopBackBlock == &curBlock) {
                // Source of loop-back edge must be an unconditional jump after edge-splitting
                ASSERT(curBlock.fallthroughBlock == nullptr);
                // Loop-back block is always the last predecessor of loop header
                jumpBlock->jumpPredecessors.back() = &curBlock;
            } else {
                // Order of other predecessors is unspecified
                jumpBlock->jumpPredecessors[nextPredIndex[jumpIndex]++] = &curBlock;
            }
        } else {
            curBlock.jumpBlock = nullptr;
        }

        for (const BasicBlockInfo *&catchBlock : curBlock.catchBlocks) {
            catchBlock = head + BLOCK_INDEX_FROM_PTR(catchBlock);
        }
    }
    // Validation (optimized out in Release build)
    for (uint32_t i = 0; i < blockCount; i++) {
        ASSERT(nextPredIndex[i] == numJumpPredecessors_[i] - basicBlocks_[i].IsLoopHeader());
    }
}

// Note: Const-qualified pointers are exposed in the header as we do not expect the user
//       to modify the fields via a non-const pointer.
//       Const-cast is needed during preprocessing to initialize the fields.
#define BLOCK_PTR_CONST_CAST(ptr) const_cast<BasicBlockInfo *>(ptr)

void BytecodePreprocessorNew::LoopAnalysis()
{
    auto dfs = [&](auto &dfs, BasicBlockInfo *cur, BasicBlockInfo *header) -> void {
        if (cur == header) {
            return;
        }
        cur->loopHeaderBlock = header;
        for (const BasicBlockInfo *pred : cur->jumpPredecessors) {
            if (pred->loopHeaderBlock == nullptr) {
                dfs(dfs, BLOCK_PTR_CONST_CAST(pred), header);
                continue;
            }
            if (pred->loopHeaderBlock != header) {
                // pred belongs to an inner loop
                const BasicBlockInfo *innerHeader = pred->loopHeaderBlock;
                if (innerHeader->loopHeaderBlock == nullptr) {
                    dfs(dfs, BLOCK_PTR_CONST_CAST(innerHeader), header);
                }
            }
        }
    };
    // Collects basic blocks_ in each loop.
    // If block B is a loop header, B->loopHeaderBlock is the header of its parent_ loop.
    for (uint32_t headerIndex : loopHeaders_) {
        BasicBlockInfo *curLoopHeader = &basicBlocks_[headerIndex];
        ASSERT(curLoopHeader->loopHeaderBlock == nullptr);
        ASSERT(curLoopHeader->loopBackBlock != nullptr);
        dfs(dfs, BLOCK_PTR_CONST_CAST(curLoopHeader->loopBackBlock), curLoopHeader);
    }
}

void BytecodePreprocessorNew::MakeRPO()
{
    if (basicBlocks_.empty()) {
        return;
    }
    uint32_t nextPostOrderIndex = 0;
    auto dfs = [&](auto &dfs, BasicBlockInfo *curBlock) -> void {
        constexpr uint32_t VISITING_TAG = static_cast<uint32_t>(-2);  // -2: visiting marker for DFS
        curBlock->rpoIndex = VISITING_TAG;

        for (size_t i = 1, n = curBlock->catchBlocks.size(); i <= n; i++) {
            const BasicBlockInfo *catchBlock = curBlock->catchBlocks[n - i];
            if (catchBlock->rpoIndex == NULL_INDEX) {
                dfs(dfs, BLOCK_PTR_CONST_CAST(catchBlock));
            }
        }

        BasicBlockInfo *fallthroughBlock = BLOCK_PTR_CONST_CAST(curBlock->fallthroughBlock);
        BasicBlockInfo *jumpBlock = BLOCK_PTR_CONST_CAST(curBlock->jumpBlock);

        bool fallthroughs = fallthroughBlock != nullptr && fallthroughBlock->rpoIndex == NULL_INDEX;
        bool jumps = jumpBlock != nullptr && jumpBlock->rpoIndex == NULL_INDEX;

        // We adjust the DFS order to improve the chance (yet do not guarantee)
        // that RPO index of blocks_ in a loop will be continuous.
        if (fallthroughs && jumps) {
            const BasicBlockInfo *curLoop = curBlock->IsLoopHeader() ? curBlock : curBlock->loopHeaderBlock;
            if (jumpBlock->loopHeaderBlock != curLoop) {
                // Jumps out of current loop (note: we assume that multi-level break does not exist in input)
                ASSERT(fallthroughBlock->loopHeaderBlock == curLoop);
                dfs(dfs, jumpBlock);
                dfs(dfs, fallthroughBlock);
            } else {
                dfs(dfs, fallthroughBlock);
                dfs(dfs, jumpBlock);
            }
        } else if (fallthroughs) {
            dfs(dfs, fallthroughBlock);
        } else if (jumps) {
            dfs(dfs, jumpBlock);
        }

        curBlock->rpoIndex = nextPostOrderIndex++;
        rpoList_.emplace_back(curBlock);
    };

    // Starts from the first block
    dfs(dfs, &basicBlocks_.front());
    // Post-order index -> Reversed post-order index
    for (BasicBlockInfo &curBlock : basicBlocks_) {
        if (curBlock.rpoIndex != NULL_INDEX) {
            curBlock.rpoIndex = nextPostOrderIndex - 1 - curBlock.rpoIndex;
        }
    }
    // Post-order -> Reversed post-order
    std::reverse(rpoList_.begin(), rpoList_.end());
}

void BytecodePreprocessorNew::ClearDeadPredecessors()
{
    if (rpoList_.size() == basicBlocks_.size()) {
        return;  // No dead basic blocks_.
    }
    auto pred = [](const BasicBlockInfo *block) { return block->rpoIndex == NULL_INDEX; };
    for (BasicBlockInfo &curBlock : basicBlocks_) {
        auto it = std::remove_if(curBlock.jumpPredecessors.begin(), curBlock.jumpPredecessors.end(), pred);
        curBlock.jumpPredecessors.erase(it, curBlock.jumpPredecessors.end());
    }
}

BytecodePreprocessorNew::BytecodePreprocessorNew(JitCompilationEnv *env, Chunk *chunk)
    : env_(env),
      method_(env->GetMethodLiteral()),
      numLocalVRegs_(method_->GetNumVregsWithCallField()),
      numParamVRegs_(NumParamVRegsOf(method_)),
      tryBlocks_(chunk),
      basicBlocks_(chunk),
      bytecodes_(chunk),
      rpoList_(chunk),
      bcIndexOfOffset_(chunk),
      jumpTargetBcIndices_(chunk),
      loopHeaders_(chunk),
      numJumpPredecessors_(chunk)
{
    CollectBytecodeInfo();
    CollectTryCatchBlockInfo();
    BuildBasicBlocks();
    CanonicalizeLoopsDFS(this).Run();
    SplitCriticalEdges();
    SetBasicBlockPointers();
    LoopAnalysis();
    MakeRPO();
    ClearDeadPredecessors();
}

namespace {
struct PrintIndex {
    uint32_t index_;
    explicit PrintIndex(uint32_t index) : index_(index) {}
};

struct PrintBasicBlockIndex {
    uint32_t index_;
    explicit PrintBasicBlockIndex(uint32_t index) : index_(index) {}
};

std::ostream &operator<<(std::ostream &out, PrintIndex printIndex)
{
    if (printIndex.index_ == BytecodePreprocessorNew::NULL_INDEX) {
        out << "nullptr";
    } else {
        out << printIndex.index_;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, PrintBasicBlockIndex printIndex)
{
    if (printIndex.index_ == BytecodePreprocessorNew::NULL_INDEX) {
        out << "nullptr";
    } else {
        out << "BB[" << printIndex.index_ << ']';
    }
    return out;
}
}  // namespace

std::string BytecodePreprocessorNew::Dump() const
{
    std::ostringstream out;
    out << "Bytecodes:";
    for (size_t i = 0, bcCount = bytecodes_.size(); i < bcCount; i++) {
        const BytecodeInfo &curBc = bytecodes_[i];
        out << "\n[" << std::setw(3) << i << "] opcode = ";  // 3: width for bytecode index
        out << kungfu::GetEcmaOpcodeStr(curBc.details.GetOpcode());

        out << ", offset = " << PrintIndex(curBc.offset);
        out << ", blockIndex = " << PrintIndex(curBc.blockIndex);
    }

    out << DumpBasicBlocksString();
    out << DumpTryBlocksString();
    return std::move(out).str();
}

std::string BytecodePreprocessorNew::DumpBasicBlocksString() const
{
    auto printBB = [this](const BasicBlockInfo *block) {
        return PrintBasicBlockIndex(block == nullptr ? NULL_INDEX : block - basicBlocks_.data());
    };

    std::ostringstream out;
    out << "\nBasic blocks_:";
    for (size_t i = 0, blockCount = basicBlocks_.size(); i < blockCount; i++) {
        const BasicBlockInfo &curBlock = basicBlocks_[i];
        // 2: width for block index
        out << "\n[" << std::setw(2) << i << "] rpoIndex = " << PrintIndex(curBlock.rpoIndex);
        out << ", startBcIndex = " << PrintIndex(curBlock.startBcIndex);
        out << ", endBcIndex = " << PrintIndex(curBlock.endBcIndex);
        out << "\n     fallthroughBlock = " << printBB(curBlock.fallthroughBlock);
        out << "\n     jumpBlock = " << printBB(curBlock.jumpBlock);
        out << "\n     loopHeaderBlock = " << printBB(curBlock.loopHeaderBlock);
        out << "\n     loopBackBlock = " << printBB(curBlock.loopBackBlock);
        out << "\n     jumpPredecessors = [";
        bool first = true;
        for (const BasicBlockInfo *predBlock : curBlock.jumpPredecessors) {
            first ? (void)(first = false) : (void)(out << ", ");
            out << printBB(predBlock);
        }
        out << "]\n     catchBlocks = [";
        first = true;
        for (const BasicBlockInfo *catchBlock : curBlock.catchBlocks) {
            first ? (void)(first = false) : (void)(out << ", ");
            out << printBB(catchBlock);
        }
        out << ']';
    }
    return std::move(out).str();
}

std::string BytecodePreprocessorNew::DumpTryBlocksString() const
{
    std::ostringstream out;
    out << "\nCatch blocks_:";
    for (size_t i = 0, tryBlockCount = tryBlocks_.size(); i < tryBlockCount; i++) {
        const TryBlockInfo &curTryBlock = tryBlocks_[i];
        // 2: width for try block index
        out << "\n[" << std::setw(2) << i << "] startBcIndex = " << curTryBlock.startBcIndex;
        out << "\n     endBcIndex = " << curTryBlock.endBcIndex;
        out << "\n     catchBcIndices = [";
        bool first = true;
        for (uint32_t bcIndex : curTryBlock.catchBcIndices) {
            first ? (void)(first = false) : (void)(out << ", ");
            out << bcIndex;
        }
        out << ']';
    }
    return std::move(out).str();
}

#undef BLOCK_INDEX_FROM_PTR
#undef BLOCK_INDEX_TO_PTR
}  // namespace panda::ecmascript::arksteed
