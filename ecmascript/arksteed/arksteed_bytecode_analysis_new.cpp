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

#include "ecmascript/arksteed/arksteed_bytecode_analysis_new.h"

namespace panda::ecmascript::arksteed {
using BasicBlockInfo = BytecodePreprocessorNew::BasicBlockInfo;
using BytecodeInfo = BytecodePreprocessorNew::BytecodeInfo;

BytecodeAnalysisNew::BytecodeAnalysisNew(const BytecodePreprocessorNew *parent)
    : parent_(parent),
      accIndex_(VRegOfAcc(parent_->GetNumLocalVRegs(), parent_->GetNumParamVRegs()).GetId()),
      liveIn_(parent->GetChunk()),
      liveOut_(parent->GetChunk()),
      ueSet_(parent->GetChunk()),
      killSet_(parent->GetChunk())
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (auto *dest : {&liveIn_, &liveOut_, &ueSet_, &killSet_}) {
        dest->reserve(numBlocks);
        for (uint32_t i = 0; i < numBlocks; i++) {
            dest->emplace_back(GetChunk(), accIndex_ + 1);
        }
    }

    InitializeUEAndKillSets();
    InitializeLiveIn();

    unsigned numIterations = 1;
    while (UpdateLiveness()) {
        numIterations++;
    }
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Liveness analysis done. " << numIterations << " iterations used.";
#endif
}

void BytecodeAnalysisNew::UpwardExposedSet(const BytecodeInfo *info, uint32_t blockIndex)
{
    if (info->details.AccIn() && !TestAcc(killSet_[blockIndex])) {
        SetAcc(ueSet_[blockIndex]);
    }
    for (size_t i = 0, n = info->details.inputs.size(); i < n; i++) {
        const auto &in = info->details.inputs[i];
        if (!std::holds_alternative<VirtualRegister>(in)) {
            continue;
        }
        VRegIDType vreg = std::get<VirtualRegister>(in).GetId();
        if (!TestVReg(killSet_[blockIndex], vreg)) {
            SetVReg(ueSet_[blockIndex], vreg);
        }
    }
}

void BytecodeAnalysisNew::KillSet(const BytecodeInfo *info, uint32_t blockIndex)
{
    if (info->details.AccOut()) {
        SetAcc(killSet_[blockIndex]);
    }
    for (VRegIDType out : info->details.vregOut) {
        SetVReg(killSet_[blockIndex], out);
    }
}

void BytecodeAnalysisNew::InitializeUEAndKillSets()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(blockIndex);

        for (uint32_t bcIndex = curBlock->startBcIndex; bcIndex <= curBlock->endBcIndex; ++bcIndex) {
            const BytecodeInfo *curBc = parent_->GetBytecode(bcIndex);
            UpwardExposedSet(curBc, blockIndex);
            KillSet(curBc, blockIndex);
        }
    }
}

void BytecodeAnalysisNew::InitializeLiveIn()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        liveIn_[blockIndex].CopyFrom(ueSet_[blockIndex]);  // Initially LiveIn(B) <- UESet(B)
    }
}

bool BytecodeAnalysisNew::UpdateLiveness()
{
    bool hasChange = false;
    kungfu::BitSet temp(GetChunk(), accIndex_ + 1);

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t i = numBlocks - 1; i != static_cast<uint32_t>(-1); i--) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(i);
        temp.Reset();
        for (const BasicBlockInfo *succBlock : {curBlock->fallthroughBlock, curBlock->jumpBlock}) {
            if (succBlock != nullptr) {
                temp.Union(liveIn_[succBlock->rpoIndex]);
            }
        }
        bool accWasLive = TestAcc(temp);  // Whether Acc will be used by some non-catch successor
        for (const BasicBlockInfo *catchBlock : curBlock->catchBlocks) {
            temp.Union(liveIn_[curBlock->rpoIndex]);
        }
        // Acc will be overwritten by the exception object.
        if (!accWasLive) {
            ClearAcc(temp);
        }
        if (temp.Equals(liveOut_[i])) {
            continue;  // No change
        }
        hasChange = true;
        liveOut_[i].CopyFrom(temp);
        UpdateLiveIn(i);
    }
    return hasChange;
}

// LiveIn(B) = UESet(B) ⋃ (LiveOut(B) - KillSet(B))
void BytecodeAnalysisNew::UpdateLiveIn(uint32_t blockIndex)
{
    liveIn_[blockIndex].CopyFrom(liveOut_[blockIndex]);
    liveIn_[blockIndex].Exclude(killSet_[blockIndex]);
    liveIn_[blockIndex].Union(ueSet_[blockIndex]);
}

std::string BytecodeAnalysisNew::Dump() const
{
    std::ostringstream out;
    out << "Liveness of Basic Block:";

    uint32_t numBlocks = parent_->GetNumAllBasicBlocks();
    for (uint32_t i = 0; i < numBlocks; i++) {
        uint32_t rpoIndex = parent_->GetBasicBlockByBCOrder(i)->rpoIndex;
        if (rpoIndex == BytecodePreprocessorNew::NULL_INDEX) {
            continue;
        }
        out << "\n[" << std::setw(2) << i << "] UESet:   " << DumpBitset(ueSet_[rpoIndex]);  // 2: width for block index
        out << "\n     KillSet: " << DumpBitset(killSet_[rpoIndex]);
        out << "\n     LiveIn:  " << DumpBitset(liveIn_[rpoIndex]);
        out << "\n     LiveOut: " << DumpBitset(liveOut_[rpoIndex]);
    }
    return out.str();
}

std::string BytecodeAnalysisNew::DumpBitset(const kungfu::BitSet &bitset) const
{
    std::ostringstream out;
    out << '[';

    bool first = true;
    for (VRegIDType i = 0; i < GetNumLocalVRegs(); i++) {
        if (!TestVReg(bitset, i)) {
            continue;
        }
        first ? (void)(first = false) : (void)(out << ", ");
        out << 'v' << i;
    }
    for (VRegIDType i = 0; i < GetNumParamVRegs(); i++) {
        if (!TestVReg(bitset, GetNumLocalVRegs() + i)) {
            continue;
        }
        first ? (void)(first = false) : (void)(out << ", ");
        out << 'a' << i;
    }
    if (TestVReg(bitset, accIndex_ - 1)) {
        first ? (void)(first = false) : (void)(out << ", ");
        out << "env";
    }
    if (TestAcc(bitset)) {
        first ? (void)(first = false) : (void)(out << ", ");
        out << "acc";
    }

    out << ']';
    return out.str();
}
}  // namespace panda::ecmascript::arksteed
