/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_FRAME_STATE_H
#define ECMASCRIPT_COMPILER_FRAME_STATE_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/base/bit_set.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {
class BytecodeCircuitBuilder;
struct BytecodeRegion;

class FrameStateInfo {
public:
    explicit FrameStateInfo(Chunk* chunk, size_t numVregs)
        : values_(numVregs, chunk), liveout_(chunk, numVregs) {}

    void SetValuesAt(size_t index, GateRef gate)
    {
        ASSERT(index < values_.size());
        values_[index] = gate;
    }

    void SetBit(size_t index)
    {
        liveout_.SetBit(index);
    }

    void ClearBit(size_t index)
    {
        liveout_.ClearBit(index);
    }

    GateRef ValuesAt(size_t index) const
    {
        ASSERT(index < values_.size());
        return values_[index];
    }

    void CopyFrom(FrameStateInfo *other)
    {
        values_.assign(other->values_.begin(), other->values_.end());
        liveout_.CopyFrom(other->liveout_);
    }

    bool MergeLiveout(FrameStateInfo *other)
    {
        return liveout_.UnionWithChanged(other->liveout_);
    }
private:
    // [numVRegs_] [extra args] [numArgs_] [accumulator]
    ChunkVector<GateRef> values_;
    BitSet liveout_;
};

class FrameStateBuilder {
public:
    FrameStateBuilder(BytecodeCircuitBuilder *builder,
        Circuit *circuit, const MethodLiteral *literal);
    ~FrameStateBuilder();

    void BuildFrameState();
private:
    static constexpr size_t FIXED_ARGS = 2; // ac & env
    GateRef ValuesAt(size_t index) const
    {
        return liveOutResult_->ValuesAt(index);
    }

    GateRef ValuesAtAccumulator() const
    {
        return ValuesAt(accumulatorIndex_);
    }
    void UpdateVirtualRegister(size_t index, GateRef gate)
    {
        liveOutResult_->SetValuesAt(index, gate);
        if (gate == Circuit::NullGate()) {
            liveOutResult_->ClearBit(index);
        } else {
            liveOutResult_->SetBit(index);
        }
    }
    void UpdateAccumulator(GateRef gate)
    {
        UpdateVirtualRegister(accumulatorIndex_, gate);
    }
    void BindStateSplit(GateRef state, GateRef depend, GateRef frameState);
    void BindStateSplit(GateRef gate, GateRef frameState);
    void BindBBStateSplit();
    void UpdateVirtualRegister(size_t id, size_t index, GateRef gate);
    GateRef BuildFrameStateGate(size_t pcOffset, GateRef frameValues, FrameStateOutput output);
    GateRef BuildFrameValues(FrameStateInfo *stateInfo);

    FrameStateInfo *CreateEmptyStateInfo();
    void BuildPostOrderList(size_t size);
    bool ComputeLiveOut(size_t bbId);
    void ComputeLiveState();
    void ComputeLiveOutBC(uint32_t index, const BytecodeInfo &bytecodeInfo, size_t bbId);
    bool IsAsyncResolveOrSusp(const BytecodeInfo &bytecodeInfo);
    bool MergeIntoPredBC(uint32_t predPc, size_t diff);
    bool MergeIntoPredBB(BytecodeRegion *bb, BytecodeRegion *predBb);
    size_t LoopExitCount(BytecodeRegion *bb, BytecodeRegion *bbNext);
    GateRef TryGetLoopExitValue(GateRef value, size_t diff);
    FrameStateInfo *GetOrOCreateBCEndStateInfo(uint32_t bcIndex)
    {
        auto currentInfo = bcEndStateInfos_[bcIndex];
        if (currentInfo == nullptr) {
            currentInfo = CreateEmptyStateInfo();
            bcEndStateInfos_[bcIndex] = currentInfo;
        }
        return currentInfo;
    }
    FrameStateInfo *GetEntryBBBeginStateInfo()
    {
        auto entry = CreateEmptyStateInfo();
        auto first = bbBeginStateInfos_.at(1);  // 1: first block
        for (size_t i = 0; i < numVregs_; ++i) {
            auto value = first->ValuesAt(i);
            if (value == Circuit::NullGate()) {
                continue;
            }
            if (gateAcc_.IsValueSelector(value)) {
                value = gateAcc_.GetValueIn(value);
            }
            entry->SetValuesAt(i, value);
        }
        return entry;
    }
    FrameStateInfo *GetBBBeginStateInfo(size_t bbId)
    {
        if (bbId == 0) {    // 0: entry block
            return GetEntryBBBeginStateInfo();
        }
        return bbBeginStateInfos_.at(bbId);
    }
    void UpdateVirtualRegistersOfSuspend(GateRef gate);
    void UpdateVirtualRegistersOfResume(GateRef gate);
    void SaveBBBeginStateInfo(size_t bbId);
    FrameStateInfo *GetFrameInfoBefore(BytecodeRegion &bb, uint32_t bcId);
    FrameStateInfo *GetFrameInfoAfter(uint32_t bcId);
    GateRef GetPreBBInput(BytecodeRegion *bb, BytecodeRegion *predBb, GateRef gate);
    GateRef GetPhiComponent(BytecodeRegion *bb, BytecodeRegion *predBb, GateRef phi);
    void BuildFrameState(BytecodeRegion& bb, const BytecodeInfo &bytecodeInfo, size_t index);
    void BuildStateSplitAfter(size_t index, BytecodeRegion& bb);
    void BuildStateSplitBefore(BytecodeRegion& bb, size_t index);
    bool ShouldInsertFrameStateBefore(BytecodeRegion& bb, size_t index);
    void BuildCallFrameState(size_t index, BytecodeRegion& bb);
    size_t GetNearestNextIndex(size_t index, BytecodeRegion& bb) const;

    BytecodeCircuitBuilder *builder_{nullptr};
    FrameStateInfo *liveOutResult_{nullptr};
    size_t numVregs_ {0};
    size_t accumulatorIndex_ {0};
    Circuit *circuit_ {nullptr};
    GateAccessor gateAcc_;
    ChunkVector<FrameStateInfo *> bcEndStateInfos_;
    ChunkVector<FrameStateInfo *> bbBeginStateInfos_;
    ChunkVector<size_t> postOrderList_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_FRAME_STATE_H
