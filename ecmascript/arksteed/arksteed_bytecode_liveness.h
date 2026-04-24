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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_LIVENESS_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_LIVENESS_H

#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/base/bit_set.h"

namespace panda::ecmascript::arksteed {
using namespace panda::ecmascript::kungfu;

class LoopInfo {
public:
    LoopInfo(Chunk *chunk, uint32_t headerIndex, uint32_t endIndex, VRegIDType numLocal, VRegIDType numParams)
        : headerIndex_(headerIndex), endIndex_(endIndex), defSet_(chunk, NumVRegs(numLocal, numParams))
    {}

    uint32_t HeaderIndex() const
    {
        return headerIndex_;
    }
    uint32_t EndIndex() const
    {
        return endIndex_;
    }

    bool ContainsBytecode(uint32_t bcIndex) const
    {
        return bcIndex >= headerIndex_ && bcIndex < endIndex_;
    }

    void AddDef(VirtualRegister reg)
    {
        defSet_.SetBit(reg.GetId());
    }
    void AddDefsOf(const LoopInfo &other)
    {
        defSet_.Union(other.defSet_);
    }

    bool IsDefinedInThisLoop(VirtualRegister reg) const
    {
        return defSet_.TestBit(reg.GetId());
    }
    bool NumVRegsDefinedInThisLoop() const
    {
        return defSet_.Count();
    }

private:
    uint32_t headerIndex_;
    uint32_t endIndex_;
    // Set of virtual registers which are assigned in this loop,
    kungfu::BitSet defSet_;
};

class LivenessBitSet {
public:
    LivenessBitSet(Chunk *chunk, VRegIDType numLocal, VRegIDType numParams)
        : liveness_(chunk, arksteed::NumVRegs(numLocal, numParams)), numLocal_(numLocal), numParams_(numParams)
    {
        // Params are always live by default
        for (VRegIDType i = 0; i < numParams_; i++) {
            liveness_.SetBit(VRegOfParam(numLocal, i).GetId());
        }
        // Env is always live by default
        liveness_.SetBit(VRegOfEnv(numLocal, numParams).GetId());
    }

    // Use CopyFrom() instead.
    NO_COPY_SEMANTIC(LivenessBitSet);
    DEFAULT_MOVE_SEMANTIC(LivenessBitSet);

#define LIVENESS_FOR_VREG_FN(Name, expr, ...) \
    void Set##Name(__VA_ARGS__)               \
    {                                         \
        liveness_.SetBit(expr);               \
    }                                         \
    void Clear##Name(__VA_ARGS__)             \
    {                                         \
        liveness_.ClearBit(expr);             \
    }                                         \
    bool Test##Name(__VA_ARGS__) const        \
    {                                         \
        return liveness_.TestBit(expr);       \
    }

    // Set(index), Clear(index), Test(index)
    // Set(reg),   Clear(reg),   Test(reg)
    LIVENESS_FOR_VREG_FN(/* Name */, index, VRegIDType index)
    LIVENESS_FOR_VREG_FN(/* Name */, reg.GetId(), VirtualRegister reg)
    // SetLocal(index), ClearLocal(index), TestLocal(index)
    LIVENESS_FOR_VREG_FN(Local, VRegOfLocal(index).GetId(), VRegIDType index)
    // SetParam(index), ClearParam(index), TestParam(index)
    LIVENESS_FOR_VREG_FN(Param, VRegOfParam(numLocal_, index).GetId(), VRegIDType index)
    // SetAcc(), ClearAcc(), TestAcc()
    LIVENESS_FOR_VREG_FN(Acc, VRegOfAcc(numLocal_, numParams_).GetId())
#undef LIVENESS_FOR_VREG_FN

    void CopyFrom(const LivenessBitSet &other)
    {
        liveness_.CopyFrom(other.liveness_);
        numLocal_ = other.numLocal_;
        numParams_ = other.numParams_;
    }

    void Union(const LivenessBitSet &other)
    {
        ASSERT(numLocal_ == other.numLocal_ && numParams_ == other.numParams_);
        liveness_.Union(other.liveness_);
    }

    bool UnionWithChanged(const LivenessBitSet &other)
    {
        ASSERT(numLocal_ == other.numLocal_ && numParams_ == other.numParams_);
        return liveness_.UnionWithChanged(other.liveness_);
    }

    void Reset()
    {
        liveness_.Reset();
    }

    VRegIDType NumLocalVRegs() const
    {
        return numLocal_;
    }
    VRegIDType NumParamVRegs() const
    {
        return numParams_;
    }
    VRegIDType NumVRegs() const
    {
        // Calls the free function NumVRegs()
        return arksteed::NumVRegs(numLocal_, numParams_);
    }

    VRegIDType NumLiveVRegs() const
    {
        return static_cast<VRegIDType>(liveness_.Count());
    }

    std::string Dump() const;

private:
    kungfu::BitSet liveness_;
    VRegIDType numLocal_;
    VRegIDType numParams_;
};

struct LivenessInfo {
    LivenessInfo(Chunk *chunk, VRegIDType numLocal, VRegIDType numParams)
        : liveIn(chunk, numLocal, numParams), liveOut(chunk, numLocal, numParams)
    {}

    LivenessBitSet &GetLiveIn()
    {
        return liveIn;
    }
    LivenessBitSet &GetLiveOut()
    {
        return liveOut;
    }
    const LivenessBitSet &GetLiveIn() const
    {
        return liveIn;
    }
    const LivenessBitSet &GetLiveOut() const
    {
        return liveOut;
    }

private:
    LivenessBitSet liveIn;
    LivenessBitSet liveOut;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_LIVENESS_H
