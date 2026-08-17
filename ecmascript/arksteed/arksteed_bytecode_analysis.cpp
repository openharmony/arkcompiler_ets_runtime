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
using BasicBlockInfo = BytecodePreprocessor::BasicBlockInfo;

BytecodeAnalysis::BytecodeAnalysis(const BytecodePreprocessor *parent)
    : parent_(parent),
      numVRegs_(parent->GetNumVRegs()),
      blockLiveIn_(parent->GetChunk()),
      blockLiveOut_(parent->GetChunk()),
      bcLiveIn_(parent->GetChunk()),
      bcLiveOut_(parent->GetChunk()),
      ueSet_(parent->GetChunk()),
      killSet_(parent->GetChunk())
{}

bool BytecodeAnalysis::Run()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (auto *dest : {&blockLiveIn_, &blockLiveOut_, &ueSet_, &killSet_}) {
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
    InitializeBytecodeLiveness();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Liveness analysis done. " << numIterations << " iterations used.";
#endif
    return true;
}

void BytecodeAnalysis::AddUpwardExposedUses(const BytecodeInfo *info, const kungfu::BitSet &killed, kungfu::BitSet &ue)
{
    if (info->AccIn() && !TestAcc(killed)) {
        SetAcc(ue);
    }
    if (info->EnvIn() && !TestVReg(killed, LexicalEnvIndex())) {
        SetVReg(ue, LexicalEnvIndex());
    }
    if (info->ThisObjectIn()) {
        SetVReg(ue, ThisObjectIndex());
    }
    for (size_t i = 0, n = info->inputs.size(); i < n; i++) {
        const auto &in = info->inputs[i];
        if (!std::holds_alternative<VirtualRegister>(in)) {
            continue;
        }
        VRegIDType vreg = std::get<VirtualRegister>(in).GetId();
        if (!TestVReg(killed, vreg)) {
            SetVReg(ue, vreg);
        }
    }
}

void BytecodeAnalysis::AddUsedVRegs(const BytecodeInfo *info, kungfu::BitSet &bitset)
{
    if (info->AccIn()) {
        SetAcc(bitset);
    }
    if (info->EnvIn()) {
        SetVReg(bitset, LexicalEnvIndex());
    }
    if (info->ThisObjectIn()) {
        SetVReg(bitset, ThisObjectIndex());
    }
    for (size_t i = 0, n = info->inputs.size(); i < n; i++) {
        const auto &in = info->inputs[i];
        if (!std::holds_alternative<VirtualRegister>(in)) {
            continue;
        }
        SetVReg(bitset, std::get<VirtualRegister>(in).GetId());
    }
}

void BytecodeAnalysis::SetDefinedVRegs(const BytecodeInfo *info, kungfu::BitSet &bitset)
{
    if (info->AccOut()) {
        SetAcc(bitset);
    }
    if (info->EnvOut()) {
        SetVReg(bitset, LexicalEnvIndex());
    }
    for (VRegIDType out : info->vregOut) {
        SetVReg(bitset, out);
    }
}

void BytecodeAnalysis::ClearDefinedVRegs(const BytecodeInfo *info, kungfu::BitSet &bitset)
{
    if (info->AccOut()) {
        ClearAcc(bitset);
    }
    if (info->EnvOut()) {
        ClearVReg(bitset, LexicalEnvIndex());
    }
    for (VRegIDType out : info->vregOut) {
        ClearVReg(bitset, out);
    }
}

void BytecodeAnalysis::ExpandKillSet()
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

void BytecodeAnalysis::InitializeUEAndKillSets()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(blockIndex);

        for (uint32_t bcIndex = curBlock->startBcIndex; bcIndex <= curBlock->endBcIndex; ++bcIndex) {
            const BytecodeInfo *curBc = parent_->GetBytecode(bcIndex);
            AddUpwardExposedUses(curBc, killSet_[blockIndex], ueSet_[blockIndex]);
            SetDefinedVRegs(curBc, killSet_[blockIndex]);
        }
    }
}

void BytecodeAnalysis::InitializeLiveIn()
{
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        blockLiveIn_[blockIndex].CopyFrom(ueSet_[blockIndex]);  // Initially LiveIn(B) <- UESet(B)
    }
}

void BytecodeAnalysis::FinalizeWithFixedParamsAndEnv()
{
    VRegIDType callTarget = VRegOfParam(GetNumLocalVRegs(), CALL_TARGET_PARAM_INDEX);
    VRegIDType newTarget = VRegOfParam(GetNumLocalVRegs(), NEW_TARGET_PARAM_INDEX);

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        // These virtual registers may be used implicitly by GraphBuilder. Mark them as always-live.
        for (VRegIDType vregIndex : {callTarget, newTarget}) {
            blockLiveIn_[blockIndex].SetBit(vregIndex);
            blockLiveOut_[blockIndex].SetBit(vregIndex);
        }
    }
}

void BytecodeAnalysis::InitializeBytecodeLiveness()
{
    uint32_t numBytecodes = parent_->GetNumBytecodes();
    bcLiveIn_.reserve(numBytecodes);
    bcLiveOut_.reserve(numBytecodes);
    for (uint32_t bcIndex = 0; bcIndex < numBytecodes; bcIndex++) {
        bcLiveIn_.emplace_back(GetChunk(), numVRegs_);
        bcLiveOut_.emplace_back(GetChunk(), numVRegs_);
    }

    kungfu::BitSet live(GetChunk(), numVRegs_);

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(blockIndex);
        if (curBlock->startBcIndex > curBlock->endBcIndex) {
            continue;
        }
        live.CopyFrom(blockLiveOut_[blockIndex]);
        // Reverse scan: compute per-bytecode liveness
        for (uint32_t bcIndex = curBlock->endBcIndex + 1; bcIndex-- > curBlock->startBcIndex; ) {
            const BytecodeInfo *curBc = parent_->GetBytecode(bcIndex);
            bcLiveOut_[bcIndex].CopyFrom(live);
            ClearDefinedVRegs(curBc, live);
            AddUsedVRegs(curBc, live);
            bcLiveIn_[bcIndex].CopyFrom(live);
        }
    }
}

bool BytecodeAnalysis::UpdateLiveness()
{
    bool hasChange = false;
    kungfu::BitSet temp(GetChunk(), numVRegs_);
    kungfu::BitSet newLiveIn(GetChunk(), numVRegs_);
    kungfu::BitSet exceptionalUE(GetChunk(), numVRegs_);

    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t i = numBlocks - 1; i != static_cast<uint32_t>(-1); i--) {
        const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(i);
        temp.Reset();
        for (const BasicBlockInfo *succBlock : {curBlock->fallthroughBlock, curBlock->jumpBlock}) {
            if (succBlock != nullptr) {
                temp.Union(blockLiveIn_[succBlock->rpoIndex]);
            }
        }

        if (!temp.Equals(blockLiveOut_[i])) {
            hasChange = true;
            blockLiveOut_[i].CopyFrom(temp);
        }

        newLiveIn.CopyFrom(blockLiveOut_[i]);
        newLiveIn.Exclude(killSet_[i]);
        newLiveIn.Union(ueSet_[i]);

        exceptionalUE.Reset();
        ComputeExceptionalUE(i, exceptionalUE);
        newLiveIn.Union(exceptionalUE);

        if (!newLiveIn.Equals(blockLiveIn_[i])) {
            hasChange = true;
            blockLiveIn_[i].CopyFrom(newLiveIn);
        }
    }
    return hasChange;
}

void BytecodeAnalysis::ComputeExceptionalUE(uint32_t blockIndex, kungfu::BitSet &exceptionalUE)
{
    const BasicBlockInfo *curBlock = parent_->GetBasicBlockByRPO(blockIndex);
    if (curBlock->catchBlock == nullptr) {
        return;
    }
    kungfu::BitSet killedBefore(GetChunk(), numVRegs_);
    kungfu::BitSet liveAtThrow(GetChunk(), numVRegs_);
    for (uint32_t bcIndex = curBlock->startBcIndex; bcIndex <= curBlock->endBcIndex; ++bcIndex) {
        const BytecodeInfo *curBc = parent_->GetBytecode(bcIndex);
        if (curBc->IsGeneral() && !curBc->NoThrow()) {
            liveAtThrow.CopyFrom(blockLiveIn_[curBlock->catchBlock->rpoIndex]);
            liveAtThrow.Exclude(killedBefore);
            ClearAcc(liveAtThrow);
            exceptionalUE.Union(liveAtThrow);
        }
        SetDefinedVRegs(curBc, killedBefore);
    }
}

std::string BytecodeAnalysis::Dump() const
{
    std::ostringstream out;
    out << "Liveness of Basic Blocks (labelled by RPO index):";
    uint32_t numBlocks = parent_->GetNumLiveBasicBlocks();
    for (uint32_t rpoIndex = 0; rpoIndex < numBlocks; rpoIndex++) {
        // 2: width for block index
        out << "\n[" << std::setw(2) << rpoIndex << "] UESet:   " << DumpBitset(ueSet_[rpoIndex]);
        out << "\n     KillSet: " << DumpBitset(killSet_[rpoIndex]);
        out << "\n     LiveIn:  " << DumpBitset(blockLiveIn_[rpoIndex]);
        out << "\n     LiveOut: " << DumpBitset(blockLiveOut_[rpoIndex]);
    }
    return out.str();
}

std::string BytecodeAnalysis::DumpBitset(const kungfu::BitSet &bitset) const
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
