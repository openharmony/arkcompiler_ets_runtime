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
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {
class BytecodeCircuitBuilder;
struct BytecodeRegion;

class FrameStateInfo {
public:
    explicit FrameStateInfo(size_t numVregs) :
        values_(numVregs) {}

    void SetValuesAt(size_t index, GateRef gate)
    {
        ASSERT(index < values_.size());
        values_[index] = gate;
    }

    GateRef ValuesAt(size_t index) const
    {
        ASSERT(index < values_.size());
        return values_[index];
    }

    void CopyFrom(FrameStateInfo *other)
    {
        values_.assign(other->values_.begin(), other->values_.end());
    }
private:
    // [numVRegs_] [extra args] [numArgs_] [accumulator]
    std::vector<GateRef> values_ {};
};

class FrameStateBuilder {
public:
    FrameStateBuilder(BytecodeCircuitBuilder *builder,
        Circuit *circuit, const MethodLiteral *literal);
    ~FrameStateBuilder();

    void BuildFrameState();
private:
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
    }
    void UpdateAccumulator(GateRef gate)
    {
        UpdateVirtualRegister(accumulatorIndex_, gate);
    }
    void BindGuard(GateRef gate, size_t pcOffset, FrameStateInfo *stateInfo);
    void BindGuard(size_t size);
    void UpdateVirtualRegister(size_t id, size_t index, GateRef gate);
    GateRef FrameState(size_t pcOffset, FrameStateInfo *stateInfo);

    FrameStateInfo *CreateEmptyStateInfo();
    void BuildPostOrderList(size_t size);
    bool ComputeLiveOut(size_t bbId);
    void ComputeLiveState();
    void ComputeLiveOutBC(const BytecodeInfo &bytecodeInfo);
    bool MergeIntoPredBC(const uint8_t* predPc);
    bool MergeIntoPredBB(BytecodeRegion *bb, BytecodeRegion *predBb);
    FrameStateInfo *GetOrOCreateStateInfo(const uint8_t * pc)
    {
        auto currentInfo = stateInfos_[pc];
        if (currentInfo == nullptr) {
            currentInfo = CreateEmptyStateInfo();
            stateInfos_[pc] = currentInfo;
        }
        return currentInfo;
    }
    GateRef GetPhiComponent(BytecodeRegion *bb, BytecodeRegion *predBb, GateRef phi);

    BytecodeCircuitBuilder *builder_{nullptr};
    FrameStateInfo *liveOutResult_{nullptr};
    size_t numVregs_ {0};
    size_t accumulatorIndex_ {0};
    Circuit *circuit_ {nullptr};
    GateAccessor gateAcc_;
    ArgumentAccessor argAcc_;
    std::map<const uint8_t *, FrameStateInfo *> stateInfos_;
    std::vector<size_t> postOrderList_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_FRAME_STATE_H