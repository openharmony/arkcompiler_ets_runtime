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

#include "ecmascript/arksteed/arksteed_bytecode_analysis.h"

namespace panda::ecmascript::arksteed {
using BasicBlockInfo = BytecodePreprocessorNew::BasicBlockInfo;

BytecodeAnalysisNew::BytecodeAnalysisNew(const BytecodePreprocessorNew *parent)
    : parent_(parent),
      numVRegs_(parent->GetNumVRegs()),
      liveIn_(parent->GetChunk()),
      liveOut_(parent->GetChunk()),
      ueSet_(parent->GetChunk()),
      killSet_(parent->GetChunk())
{}

bool BytecodeAnalysisNew::Run()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (auto *dest : {&liveIn_, &liveOut_, &ueSet_, &killSet_}) {
        dest->reserve(numBlocks);
        for (uint32_t i = 0; i < numBlocks; i++) {
            dest->emplace_back(GetChunk(), numVRegs_);
        }
    }

    InitializeUEAndKillSets();
    InitializeLiveIn();

    unsigned numIterations = 1;
    while (UpdateLiveness()) {
        numIterations++;
    }
    ExpandKillSet();
    FinalizeWithFixedParamsAndEnv();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Liveness analysis done. " << numIterations << " iterations used.";
#endif
    return true;
}

void BytecodeAnalysisNew::UpdateUpwardExposedSet(const BytecodeInfo *info, uint32_t blockIndex)
{
    if (info->AccIn() && !TestAcc(killSet_[blockIndex])) {
        SetAcc(ueSet_[blockIndex]);
    }
    VRegIDType lexicalEnv = numVRegs_ - EXTRA_VREG_COUNT + LEXICAL_ENV_EXTRA_INDEX;
    if (info->EnvIn() && !TestVReg(killSet_[blockIndex], lexicalEnv)) {
        SetVReg(ueSet_[blockIndex], lexicalEnv);
    }
    if (info->ThisObjectIn()) {
        VRegIDType thisObj = VRegOfParam(GetNumLocalVRegs(), THIS_OBJECT_PARAM_INDEX);
        SetVReg(ueSet_[blockIndex], thisObj);
    }
    for (size_t i = 0, n = info->inputs.size(); i < n; i++) {
        const auto &in = info->inputs[i];
        if (!std::holds_alternative<VirtualRegister>(in)) {
            continue;
        }
        VRegIDType vreg = std::get<VirtualRegister>(in).GetId();
        if (!TestVReg(killSet_[blockIndex], vreg)) {
            SetVReg(ueSet_[blockIndex], vreg);
        }
    }
}

void BytecodeAnalysisNew::UpdateKillSet(const BytecodeInfo *info, uint32_t blockIndex)
{
    if (info->AccOut()) {
        SetAcc(killSet_[blockIndex]);
    }
    if (info->EnvOut()) {
        VRegIDType lexicalEnv = numVRegs_ - EXTRA_VREG_COUNT + LEXICAL_ENV_EXTRA_INDEX;
        SetVReg(killSet_[blockIndex], lexicalEnv);
    }
    for (VRegIDType out : info->vregOut) {
        SetVReg(killSet_[blockIndex], out);
    }
}

void BytecodeAnalysisNew::ExpandKillSet()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(blockIndex);

        while (curBlock->loopHeaderBlock != nullptr) {
            uint32_t headerBlockIndex = curBlock->loopHeaderBlock->rpoIndex;
            killSet_[headerBlockIndex].Union(killSet_[blockIndex]);
            curBlock = curBlock->loopHeaderBlock;
        }
    }
}

void BytecodeAnalysisNew::InitializeUEAndKillSets()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(blockIndex);

        for (uint32_t bcIndex = curBlock->startBcIndex; bcIndex <= curBlock->endBcIndex; ++bcIndex) {
            const BytecodeInfo *curBc = parent_->GetBytecode(bcIndex);
            UpdateUpwardExposedSet(curBc, blockIndex);
            UpdateKillSet(curBc, blockIndex);
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

void BytecodeAnalysisNew::FinalizeWithFixedParamsAndEnv()
{
    VRegIDType callTarget = VRegOfParam(GetNumLocalVRegs(), CALL_TARGET_PARAM_INDEX);
    VRegIDType newTarget = VRegOfParam(GetNumLocalVRegs(), NEW_TARGET_PARAM_INDEX);

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        // These virtual registers may be used implicitly by GraphBuilder. Mark them as always-live.
        for (VRegIDType vregIndex : {callTarget, newTarget}) {
            liveIn_[blockIndex].SetBit(vregIndex);
            liveOut_[blockIndex].SetBit(vregIndex);
        }
    }
}

bool BytecodeAnalysisNew::UpdateLiveness()
{
    bool hasChange = false;
    kungfu::BitSet temp(GetChunk(), numVRegs_);

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t i = numBlocks - 1; i != static_cast<uint32_t>(-1); i--) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(i);
        temp.Reset();
        for (const BasicBlockInfo *succBlock : {curBlock->fallthroughBlock, curBlock->jumpBlock}) {
            if (succBlock != nullptr) {
                temp.Union(liveIn_[succBlock->rpoIndex]);
            }
        }
        if (curBlock->catchBlock != nullptr) {
            // Whether Acc will be used by some non-catch successor
            bool accWasLive = TestAcc(temp);
            temp.Union(liveIn_[curBlock->catchBlock->rpoIndex]);
            // Acc will be overwritten by the exception object.
            if (!accWasLive) {
                ClearAcc(temp);
            }
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
    out << "Liveness of Basic Blocks (labelled by RPO index):";

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t rpoIndex = 0; rpoIndex < numBlocks; rpoIndex++) {
        // 2: width for block index
        out << "\n[" << std::setw(2) << rpoIndex << "] UESet:   " << DumpBitset(ueSet_[rpoIndex]);
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
    for (VRegIDType i = 0; i < numVRegs_; i++) {
        if (!TestVReg(bitset, i)) {
            continue;
        }
        first ? (void)(first = false) : (void)(out << ", ");
        out << VRegDisplayString(i, GetNumLocalVRegs(), GetNumParamVRegs());
    }

    out << ']';
    return out.str();
}
}  // namespace panda::ecmascript::arksteed
