/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_H
#define ECMASCRIPT_COMPILER_CIRCUIT_H

#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include "ecmascript/compiler/gate.h"
#include "ecmascript/frames.h"
#include "libpandabase/macros.h"
#include "securec.h"

namespace panda::ecmascript::kungfu {
class Circuit {  // note: calling NewGate could make all saved Gate* invalid
public:
    Circuit();
    ~Circuit();
    Circuit(Circuit const &circuit) = default;
    Circuit &operator=(Circuit const &circuit) = default;
    Circuit(Circuit &&circuit) = default;
    Circuit &operator=(Circuit &&circuit) = default;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    GateRef NewGate(OpCode op, MachineType bitValue, BitField bitfield, size_t numIns, const GateRef inList[],
                    GateType type, MarkCode mark = MarkCode::NO_MARK);
    GateRef NewGate(OpCode op, MachineType bitValue, BitField bitfield, const std::vector<GateRef> &inList,
                    GateType type, MarkCode mark = MarkCode::NO_MARK);
    GateRef NewGate(OpCode op, BitField bitfield, size_t numIns, const GateRef inList[], GateType type,
                    MarkCode mark = MarkCode::NO_MARK);
    GateRef NewGate(OpCode op, BitField bitfield, const std::vector<GateRef> &inList, GateType type,
                    MarkCode mark = MarkCode::NO_MARK);
    void PrintAllGates() const;
    void PrintAllGates(BytecodeCircuitBuilder &builder) const;
    void GetAllGates(std::vector<GateRef>& gates) const;
    static GateRef GetCircuitRoot(OpCode opcode);
    static GateRef NullGate();
    bool Verify(GateRef gate) const;
    panda::ecmascript::FrameType GetFrameType() const;
    void SetFrameType(panda::ecmascript::FrameType type);
    GateRef GetConstantGate(MachineType bitValue, BitField bitfield, GateType type);
    size_t GetGateCount() const;
    TimeStamp GetTime() const;
    void AdvanceTime() const;

private:
    static const size_t INITIAL_SPACE = 1U << 0U;  // this should be tuned
    static const size_t MAX_SPACE = 1U << 24U;     // this should be tuned
    static const size_t SCALE_RATE = 1U << 1U;     // this should be tuned

    void Print(GateRef gate) const;
    GateType GetGateType(GateRef gate) const;
    GateRef GetGateRef(const Gate *gate) const;
    MachineType GetMachineType(GateRef gate) const;
    void SetMark(GateRef gate, MarkCode mark) const;
    OpCode GetOpCode(GateRef gate) const;
    void SetMachineType(GateRef gate, MachineType machineType);
    void SetGateType(GateRef gate, GateType type);
    void SetOpCode(GateRef gate, OpCode opcode);
    GateId GetId(GateRef gate) const;
    void SetBitField(GateRef gate, BitField bitfield);
    void DeleteGate(GateRef gate);
    void DecreaseIn(GateRef gate, size_t idx);

    MarkCode GetMark(GateRef gate) const;
    BitField GetBitField(GateRef gate) const;
    void DeleteIn(GateRef gate, size_t idx);
    void ModifyIn(GateRef gate, size_t idx, GateRef in);
    void NewIn(GateRef gate, size_t idx, GateRef in);
    std::vector<GateRef> GetOutVector(GateRef gate) const;
    std::vector<GateRef> GetInVector(GateRef gate) const;
    bool IsFirstOutNull(GateRef gate) const;
    bool IsInGateNull(GateRef gate, size_t idx) const;
    GateRef GetIn(GateRef gate, size_t idx) const;
    bool IsSelector(GateRef gate) const;
    bool IsControlCase(GateRef gate) const;
    bool IsLoopHead(GateRef gate) const;
    void ResetAllGateTimeStamps() const;
    void SetSpaceDataSize(size_t sz);
    uint8_t *AllocateSpace(size_t gateSize);
    Gate *AllocateGateSpace(size_t numIns);
    size_t GetCircuitDataSize() const;
    const void *GetSpaceDataStartPtrConst() const;
    const void *GetSpaceDataEndPtrConst() const;
    const uint8_t *GetDataPtrConst(size_t offset) const;
    uint8_t *GetDataPtr(size_t offset);
    size_t GetSpaceDataSize() const;
    Gate *LoadGatePtr(GateRef shift);
    const Gate *LoadGatePtrConst(GateRef shift) const;

private:
    std::vector<uint8_t> space_ {};
    size_t circuitSize_;
    size_t gateCount_;
    TimeStamp time_;
    std::vector<uint8_t> dataSection_ {};
    std::map<std::tuple<MachineType, BitField, GateType>, GateRef> constantCache_ {};
    panda::ecmascript::FrameType frameType_ {panda::ecmascript::FrameType::OPTIMIZED_FRAME};

    friend class GateAccessor;
    friend class Verifier;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_H
