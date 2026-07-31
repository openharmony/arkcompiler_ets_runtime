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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_NEW_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_NEW_H

#include "ecmascript/arksteed/arksteed_bytecode_preprocessor.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/base/bit_set.h"

namespace panda::ecmascript::arksteed {
class BytecodeAnalysis {
public:
    static constexpr uint32_t NULL_INDEX = BytecodePreprocessor::NULL_INDEX;

    explicit BytecodeAnalysis(const BytecodePreprocessor *parent);

    bool Run();

    const kungfu::BitSet &GetLiveInOfBlock(uint32_t blockRpoIndex) const
    {
        return blockLiveIn_[blockRpoIndex];
    }
    const kungfu::BitSet &GetLiveOutOfBlock(uint32_t blockRpoIndex) const
    {
        return blockLiveOut_[blockRpoIndex];
    }
    // If current block B is a loop header,
    // then KillSet(B) = Union of every KillSet(C) where C is inside the loop.
    const kungfu::BitSet &GetKillSetOfBlock(uint32_t blockRpoIndex) const
    {
        return killSet_[blockRpoIndex];
    }

    const kungfu::BitSet &GetLiveInOfBytecode(uint32_t bcIndex)
    {
        return bcLiveIn_[bcIndex];
    }
    const kungfu::BitSet &GetLiveOutOfBytecode(uint32_t bcIndex)
    {
        return bcLiveOut_[bcIndex];
    }

    VRegIDType GetNumLocalVRegs() const
    {
        return parent_->GetNumLocalVRegs();
    }
    VRegIDType GetNumParamVRegs() const
    {
        return parent_->GetNumParamVRegs();
    }
    VRegIDType GetNumVRegs() const
    {
        return numVRegs_;
    }

    Chunk *GetChunk() const
    {
        return parent_->GetChunk();
    }

    std::string Dump() const;

private:
    void AddUpwardExposedUses(const BytecodeInfo *info, const kungfu::BitSet &killed, kungfu::BitSet &ue);
    void AddUsedVRegs(const BytecodeInfo *info, kungfu::BitSet &bitset);
    void SetDefinedVRegs(const BytecodeInfo *info, kungfu::BitSet &bitset);
    void ClearDefinedVRegs(const BytecodeInfo *info, kungfu::BitSet &bitset);

    void InitializeUEAndKillSets();
    void InitializeLiveIn();
    void InitializeBytecodeLiveness();
    void ExpandKillSet();
    void FinalizeWithFixedParamsAndEnv();

    bool UpdateLiveness();
    void ComputeExceptionalUE(uint32_t blockIndex, kungfu::BitSet &exceptionalUE);

    VRegIDType AccIndex() const
    {
        return numVRegs_ - EXTRA_VREG_COUNT + ACC_EXTRA_INDEX;
    }
    VRegIDType LexicalEnvIndex() const
    {
        return numVRegs_ - EXTRA_VREG_COUNT + LEXICAL_ENV_EXTRA_INDEX;
    }
    VRegIDType ThisObjectIndex() const
    {
        return VRegOfParam(GetNumLocalVRegs(), THIS_OBJECT_PARAM_INDEX);
    }

    void SetAcc(kungfu::BitSet &bitset)
    {
        bitset.SetBit(AccIndex());
    }
    void SetVReg(kungfu::BitSet &bitset, VRegIDType vreg)
    {
        ASSERT(vreg < numVRegs_);
        bitset.SetBit(vreg);
    }

    void ClearAcc(kungfu::BitSet &bitset)
    {
        bitset.ClearBit(AccIndex());
    }
    void ClearVReg(kungfu::BitSet &bitset, VRegIDType vreg)
    {
        ASSERT(vreg < numVRegs_);
        bitset.ClearBit(vreg);
    }

    bool TestAcc(const kungfu::BitSet &bitset) const
    {
        return bitset.TestBit(AccIndex());
    }
    bool TestVReg(const kungfu::BitSet &bitset, VRegIDType vreg) const
    {
        ASSERT(vreg < numVRegs_);
        return bitset.TestBit(vreg);
    }

    std::string DumpBitset(const kungfu::BitSet &bitset) const;

    const BytecodePreprocessor *parent_;
    VRegIDType numVRegs_;
    ChunkVector<kungfu::BitSet> blockLiveIn_;
    ChunkVector<kungfu::BitSet> blockLiveOut_;
    ChunkVector<kungfu::BitSet> bcLiveIn_;
    ChunkVector<kungfu::BitSet> bcLiveOut_;
    // Upward-exposed virtual registers of basic block
    ChunkVector<kungfu::BitSet> ueSet_;
    // Killed virtial registers by definition of basic block
    ChunkVector<kungfu::BitSet> killSet_;
};
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_NEW_H
