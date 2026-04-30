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

#include "ecmascript/arksteed/arksteed_bytecode_context.h"

#include "ecmascript/compiler/jit_compilation_env.h"

namespace panda::ecmascript::arksteed {
using namespace panda::ecmascript::kungfu;

void BytecodeContext::Initialize(MethodLiteral *method, JitCompilationEnv *env,
                                 const ArkSteedCompilationOptions *options)
{
    SetOptions(options);
    method_ = method;
    bytecodeStart_ = env->GetMethodPcStart();
    bytecodeSize_ = MethodLiteral::GetCodeSize(env->GetJSPandaFile(), method->GetMethodId());

    CollectPcOffsets();
    BuildBytecodeInfo(GetEnvVreg(method_->GetNumberVRegsForArkSteed()).GetId());
    CollectJumpTargets();
    CollectTryCatchBlockInfo(env);

    // Initialize preprocessor with collected data
    preprocessor_.Initialize(this);
    JumpLoopAnalysis();
}

void BytecodeContext::CollectPcOffsets()
{
    bcIndex_.resize(bytecodeSize_);
    BytecodeInstruction bcIns(bytecodeStart_);
    BytecodeInstruction bcInsLast = bcIns.JumpTo(bytecodeSize_);

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        const uint8_t *curPc = bcIns.GetAddress();
        bcIndex_[GetPcOffset(curPc)] = static_cast<uint32_t>(pcOffsets_.size());
        pcOffsets_.emplace_back(curPc);
        bcIns = bcIns.GetNext();
    }
}

void BytecodeContext::CollectJumpTargets()
{
    auto count = GetBytecodeCount();
    jumpTargetIndex_.resize(count);

    for (uint32_t bcIndex = 0; bcIndex < count; bcIndex++) {
        const BytecodeInfo &info = infoData_[bcIndex];
        if (!info.IsJump()) {
            jumpTargetIndex_[bcIndex] = INVALID_BC_INDEX;
            continue;
        }

        const uint8_t *pc = pcOffsets_[bcIndex];
        int32_t offset = 0;
        switch (info.GetOpcode()) {
            case EcmaOpcode::JEQZ_IMM8:
            case EcmaOpcode::JNEZ_IMM8:
            case EcmaOpcode::JMP_IMM8:
                offset = static_cast<int8_t>(READ_INST_8_0());
                break;
            case EcmaOpcode::JNEZ_IMM16:
            case EcmaOpcode::JEQZ_IMM16:
            case EcmaOpcode::JMP_IMM16:
                offset = static_cast<int16_t>(READ_INST_16_0());
                break;
            case EcmaOpcode::JMP_IMM32:
            case EcmaOpcode::JNEZ_IMM32:
            case EcmaOpcode::JEQZ_IMM32:
                offset = static_cast<int32_t>(READ_INST_32_0());
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                break;
        }
        uint32_t bcOffset = static_cast<uint32_t>(pc - bytecodeStart_) + offset;
        jumpTargetIndex_[bcIndex] = bcIndex_[bcOffset];
    }
}

void BytecodeContext::BuildBytecodeInfo(size_t envVregIdx)
{
    uint32_t size = pcOffsets_.size();
    ASSERT(size > 0);
    infoData_.resize(size);
    for (uint32_t index = 0; index < size; index++) {
        auto &info = infoData_[index];
        auto pc = pcOffsets_[index];
        info.SetMetaData(bytecodes_.GetBytecodeMetaData(pc));
        ASSERT(!info.GetMetaData().IsInvalid());
        BytecodeInfo::InitBytecodeInfo(info, pc, GetPcOffset(pc), envVregIdx);
    }
}

void BytecodeContext::JumpLoopAnalysis()
{
    auto &indexToRPO = preprocessor_.GetIndex2PostOrderList();
    const auto &postOrderList = preprocessor_.GetPostOrderList();
    const auto &indexToBlockId = preprocessor_.GetIndex2BlockId();
    maxLoopEndIndex_.resize(GetBytecodeCount(), INVALID_BC_INDEX);
    for (uint32_t i = 0; i < GetBytecodeCount(); ++i) {
        loopEndIndex_.push_back(chunk_->New<ChunkVector<uint32_t>>(chunk_));
    }

    auto getMaxRpoIndex = [&indexToRPO](uint32_t lhs, uint32_t rhs) -> uint32_t {
        if (lhs == INVALID_BC_INDEX) {
            return rhs;
        }
        if (rhs == INVALID_BC_INDEX) {
            return lhs;
        }
        return indexToRPO[lhs] < indexToRPO[rhs] ? rhs : lhs;
    };

    // First collect direct backedges for each loop header.
    for (size_t i = 0; i < postOrderList.size(); i++) {
        uint32_t index = postOrderList[i];
        const BytecodeInfo &info = infoData_[index];
        if (!info.IsJump()) {
            continue;
        }

        uint32_t targetIndex = GetJumpTargetBcIndex(index);
        if (indexToRPO[targetIndex] < indexToRPO[index]) {
            loopEndIndex_[targetIndex]->push_back(index);
            maxLoopEndIndex_[targetIndex] = index;
        }
    }

    // Then absorb later loop regions into earlier loop headers until the
    // analysis interval reaches a non-crossing closure:
    //
    //   E(L) = closure_max(D(L), { E(K) | H(L) < H(K) <= E(L) })
    for (size_t i = postOrderList.size(); i >= 1; i--) {
        uint32_t loopHeader = postOrderList[i - 1];
        if (maxLoopEndIndex_[loopHeader] == INVALID_BC_INDEX) {
            continue;
        }
        for (size_t k = i; k <= indexToRPO[maxLoopEndIndex_[loopHeader]]; k++) {
            if (maxLoopEndIndex_[postOrderList[k]] == INVALID_BC_INDEX) {
                continue;
            }
            if (indexToRPO[maxLoopEndIndex_[postOrderList[k]]] > indexToRPO[maxLoopEndIndex_[loopHeader]]) {
                maxLoopEndIndex_[loopHeader] = maxLoopEndIndex_[postOrderList[k]];
            }
        }
    }
    RebuildBytecodeWithJumpRedirection();
}

bool BytecodeContext::NeedInsertJumpForLoop(uint32_t loopHeaderIndex) const
{
    return loopEndIndex_[loopHeaderIndex]->size() > 1 ||
           (loopEndIndex_[loopHeaderIndex]->size() == 1 &&
            (infoData_[loopEndIndex_[loopHeaderIndex]->back()].IsCondJump() ||
             maxLoopEndIndex_[loopHeaderIndex] != loopEndIndex_[loopHeaderIndex]->back()));
}

void BytecodeContext::RebuildBytecodeWithJumpRedirection()
{
    auto &postOrderList = preprocessor_.GetPostOrderList();
    auto &indexToRPO = preprocessor_.GetIndex2PostOrderList();
    auto &predecessorCount = preprocessor_.GetPredecessorCount();
    uint32_t rpoSize = postOrderList.size();
    ChunkVector<uint32_t> insertCount(rpoSize, chunk_);
    uint32_t bytecodeCount = GetBytecodeCount();

    for (uint32_t i = 0; i < rpoSize; ++i) {
        uint32_t index = postOrderList[i];
        if (NeedInsertJumpForLoop(index)) {
            uint32_t maxLoopEndIndex = maxLoopEndIndex_[index];
            uint32_t maxLoopEndIndexRpo = indexToRPO[maxLoopEndIndex];
            ASSERT(maxLoopEndIndexRpo < rpoSize);
            insertCount[maxLoopEndIndexRpo]++;
        }
    }

    for (uint32_t i = 1; i < rpoSize; ++i) {
        insertCount[i] += insertCount[i - 1];
    }

    uint32_t totalInsertCount = insertCount.empty() ? 0 : insertCount.back();
    ChunkVector<uint32_t> newPostOrderList(rpoSize + totalInsertCount, chunk_);
    ChunkVector<uint32_t> newIndexToRPO(bytecodeCount + totalInsertCount, chunk_);
    predecessorCount.resize(bytecodeCount + totalInsertCount);
    jumpLoop_.resize(bytecodeCount + totalInsertCount);
    jumpTargetIndex_.resize(bytecodeCount + totalInsertCount);

    uint8_t jumpPc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM8);
    auto jumpMetaData = bytecodes_.GetBytecodeMetaData(&jumpPc);

    for (uint32_t i = 0; i < rpoSize; ++i) {
        uint32_t loopHeader = postOrderList[i];
        uint32_t loopHeaderRpo = i + insertCount[i];
        newPostOrderList[loopHeaderRpo] = loopHeader;
        newIndexToRPO[loopHeader] = loopHeaderRpo;

        if (loopEndIndex_[loopHeader]->empty()) {
            continue;
        } else if (!NeedInsertJumpForLoop(loopHeader)) {
            uint32_t loopEndIndex = maxLoopEndIndex_[loopHeader];
            uint32_t loopEndIndexRpo = indexToRPO[loopEndIndex];
            jumpLoop_[loopEndIndex] = true;
        } else {  // Insert new jump
            uint32_t maxLoopEndIndex = maxLoopEndIndex_[loopHeader];
            uint32_t maxLoopEndIndexRpo = indexToRPO[maxLoopEndIndex];
            uint32_t numOfNewLoopEnd = insertCount[maxLoopEndIndexRpo]--;
            uint32_t newLoopEndRpo = maxLoopEndIndexRpo + numOfNewLoopEnd;
            uint32_t newLoopEndIndex = bytecodeCount - 1 + numOfNewLoopEnd;
            newPostOrderList[newLoopEndRpo] = newLoopEndIndex;
            newIndexToRPO[newLoopEndIndex] = newLoopEndRpo;

            infoData_.push_back(BytecodeInfo());
            infoData_.back().SetMetaData(jumpMetaData);

            // Update jump target
            for (uint32_t j = 0; j < loopEndIndex_[loopHeader]->size(); ++j) {
                uint32_t jumpIndex = (*loopEndIndex_[loopHeader])[j];
                jumpTargetIndex_[jumpIndex] = newLoopEndIndex;
                predecessorCount[newLoopEndIndex]++;
                predecessorCount[loopHeader]--;
            }
            jumpTargetIndex_[newLoopEndIndex] = loopHeader;
            predecessorCount[loopHeader]++;

            loopEndIndex_[loopHeader]->clear();
            loopEndIndex_[loopHeader]->push_back(newLoopEndIndex);
            jumpLoop_[newLoopEndIndex] = true;
        }
    }

    postOrderList.clear();
    indexToRPO.clear();
    std::copy(newPostOrderList.begin(), newPostOrderList.end(), std::back_inserter(postOrderList));
    std::copy(newIndexToRPO.begin(), newIndexToRPO.end(), std::back_inserter(indexToRPO));
}

void BytecodeContext::CollectTryCatchBlockInfo(JitCompilationEnv *env)
{
    if (method_ == nullptr) {
        return;
    }

    auto pf = env->GetJSPandaFile()->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, method_->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf, mda.GetCodeId().value());

    cda.EnumerateTryBlocks([this](panda_file::CodeDataAccessor::TryBlock &tryBlock) {
        auto tryStartOffset = tryBlock.GetStartPc();
        auto tryEndOffset = tryBlock.GetStartPc() + tryBlock.GetLength();

        if (tryStartOffset == tryEndOffset) {
            return true;
        }

        auto startBcIndex = bcIndex_[tryStartOffset];
        auto endBcIndex = bcIndex_[tryEndOffset];
        exceptionInfo_.emplace_back(ExceptionItem{startBcIndex, endBcIndex, {}});
        tryBlock.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catchBlock) {
            auto pcOffset = catchBlock.GetHandlerPc();
            auto catchBcIndex = bcIndex_[pcOffset];
            exceptionInfo_.back().catchBcIndices.emplace_back(catchBcIndex);
            return true;
        });
        return true;
    });

    std::sort(exceptionInfo_.begin(), exceptionInfo_.end(), [](const ExceptionItem &a, const ExceptionItem &b) {
        return a.startBcIndex < b.startBcIndex;
    });

    hasTryCatch_ = !exceptionInfo_.empty();
}
}  // namespace panda::ecmascript::arksteed
