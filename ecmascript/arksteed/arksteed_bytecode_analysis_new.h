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

#include "ecmascript/arksteed/arksteed_bytecode_preprocessor_new.h"
#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/base/bit_set.h"

namespace panda::ecmascript::arksteed {
class BytecodeAnalysisNew {
public:
    static constexpr uint32_t NULL_INDEX = BytecodePreprocessorNew::NULL_INDEX;

    explicit BytecodeAnalysisNew(const BytecodePreprocessorNew *parent);

    bool Run();

    const kungfu::BitSet &GetLiveIn(uint32_t blockRpoIndex) const
    {
        return liveIn_[blockRpoIndex];
    }
    const kungfu::BitSet &GetLiveOut(uint32_t blockRpoIndex) const
    {
        return liveOut_[blockRpoIndex];
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
        return arksteed::NumVRegs(GetNumLocalVRegs(), GetNumParamVRegs());
    }

    Chunk *GetChunk() const
    {
        return parent_->GetChunk();
    }

    std::string Dump() const;

private:
    void UpwardExposedSet(const BytecodePreprocessorNew::BytecodeInfo *info, uint32_t blockIndex);
    void KillSet(const BytecodePreprocessorNew::BytecodeInfo *info, uint32_t blockIndex);
    void InitializeUEAndKillSets();
    void InitializeLiveIn();
    void FinalizeWithFixedParamsAndEnv();

    bool UpdateLiveness();
    void UpdateLiveIn(uint32_t blockIndex);

    void SetEnv(kungfu::BitSet &bitset)
    {
        bitset.SetBit(accIndex_ - 1);
    }
    void SetAcc(kungfu::BitSet &bitset)
    {
        bitset.SetBit(accIndex_);
    }
    void SetVReg(kungfu::BitSet &bitset, VRegIDType vreg)
    {
        ASSERT(vreg < accIndex_);
        bitset.SetBit(vreg);
    }

    void ClearAcc(kungfu::BitSet &bitset)
    {
        bitset.ClearBit(accIndex_);
    }

    bool TestEnv(const kungfu::BitSet &bitset) const
    {
        return bitset.TestBit(accIndex_ - 1);
    }
    bool TestAcc(const kungfu::BitSet &bitset) const
    {
        return bitset.TestBit(accIndex_);
    }
    bool TestVReg(const kungfu::BitSet &bitset, VRegIDType vreg) const
    {
        ASSERT(vreg < accIndex_);
        return bitset.TestBit(vreg);
    }

    std::string DumpBitset(const kungfu::BitSet &bitset) const;

    const BytecodePreprocessorNew *parent_;
    VRegIDType accIndex_;
    ChunkVector<kungfu::BitSet> liveIn_;
    ChunkVector<kungfu::BitSet> liveOut_;
    // Upward-exposed virtual registers
    ChunkVector<kungfu::BitSet> ueSet_;
    // Killed virtial registers by definition
    ChunkVector<kungfu::BitSet> killSet_;
};
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_ANALYSIS_NEW_H
