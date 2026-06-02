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
{}

bool BytecodeAnalysisNew::Run()
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
    FinalizeWithFixedParamsAndEnv();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Liveness analysis done. " << numIterations << " iterations used.";
#endif
    return true;
}

void BytecodeAnalysisNew::UpwardExposedSet(const BytecodeInfo *info, uint32_t blockIndex)
{
    if (info->details.AccIn() && !TestAcc(killSet_[blockIndex])) {
        SetAcc(ueSet_[blockIndex]);
    }
    if (info->details.EnvIn() && !TestEnv(killSet_[blockIndex])) {
        SetEnv(ueSet_[blockIndex]);
    }
    if (info->details.ThisObjectIn()) {
        SetVReg(ueSet_[blockIndex], GetNumLocalVRegs() + 2);  // 2 : a2, which is this object
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
    if (info->details.EnvOut()) {
        SetEnv(killSet_[blockIndex]);
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

void BytecodeAnalysisNew::FinalizeWithFixedParamsAndEnv()
{
    VRegIDType firstParamVReg = GetNumLocalVRegs();
    VRegIDType callTarget = firstParamVReg;     //     a0, which is call target
    VRegIDType newTarget = firstParamVReg + 1;  // 1 : a1, which is new target
    VRegIDType env = firstParamVReg + GetNumParamVRegs();

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        // These virtual registers may be used implicitly by GraphBuilder,
        // even if EnvIn(), HasFuncIn() or HasNewTargetIn() is false
        for (VRegIDType vregIndex : {callTarget, newTarget, env}) {
            liveIn_[blockIndex].SetBit(vregIndex);
            liveOut_[blockIndex].SetBit(vregIndex);
        }
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
