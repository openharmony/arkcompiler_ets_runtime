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

#ifndef ECMASCRIPT_COMPILER_GATE_H
#define ECMASCRIPT_COMPILER_GATE_H

#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/hcr_gate_meta_data.h"
#include "ecmascript/compiler/mcr_gate_meta_data.h"
#include "ecmascript/compiler/lcr_gate_meta_data.h"
#include "ecmascript/compiler/type.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
using BitField = uint64_t;
using GateRef = int32_t; // for external users
using GateId = uint32_t;
using GateOp = uint8_t;
using GateMark = uint8_t;
using TimeStamp = uint8_t;
using SecondaryOp = uint8_t;
using OutIdx = uint32_t;
class Gate;
class BytecodeCircuitBuilder;

enum MarkCode : GateMark {
    NO_MARK = 0,
    PREVISIT,
    VISITED,
    FINISHED,
};

class Out {
public:
    Out() = default;
    void SetNextOut(const Out *ptr);
    [[nodiscard]] Out *GetNextOut();
    [[nodiscard]] const Out *GetNextOutConst() const;
    void SetPrevOut(const Out *ptr);
    [[nodiscard]] Out *GetPrevOut();
    [[nodiscard]] const Out *GetPrevOutConst() const;
    void SetIndex(OutIdx idx);
    [[nodiscard]] OutIdx GetIndex() const;
    [[nodiscard]] Gate *GetGate();
    [[nodiscard]] const Gate *GetGateConst() const;
    void SetPrevOutNull();
    [[nodiscard]] bool IsPrevOutNull() const;
    void SetNextOutNull();
    [[nodiscard]] bool IsNextOutNull() const;
    [[nodiscard]] bool IsStateEdge() const;
    ~Out() = default;

private:
    GateRef nextOut_;
    GateRef prevOut_;
    OutIdx idx_;
};

class In {
public:
    In() = default;
    void SetGate(const Gate *ptr);
    [[nodiscard]] Gate *GetGate();
    [[nodiscard]] const Gate *GetGateConst() const;
    void SetGateNull();
    [[nodiscard]] bool IsGateNull() const;
    ~In() = default;

private:
    GateRef gatePtr_;
};

// Gate structure
// for example:
// ```
// g0 = op0(...)
// g1 = op1(...)
// g2 = op2(g0, g1)
// g3 = op3(g2)
// g4 = op4(g2, g0, g1)
// g5 = op5(g3, g4)

// +---- out[1] ----+---- out[0] ----+-------- g2 --------+-- in[0] --+-- in[1] --+
// |                |                |                    |           |           |
// | next=null      | next=null      | ...                |           |           |
// | idx=1          | idx=0          |                    |    g0     |    g1     |
// | prev=g4.out[2] | prev=g4.out[1] | firstOut=g4.out[0] |           |           |
// |                |                |                    |           |           |
// +----------------+----------------+--------------------+-----------+-----------+
//       ^               ^
//       |               |
//       |               |
//       |               |          +---- out[0] ----+-------- g3 --------+-- in[0] --+
//       |               |          |                |                    |           |
//       |               |          | next=null      | ...                |           |
//       |               |          | idx=0          |                    |    g2     |
//       |               |          | prev=g4.out[0] | firstOut=g5.out[0] |           |
//       |               |          |                |                    |           |
//       |               |          +----------------+--------------------+-----------+
//       |               |               ^
//       |               |               |
//       |               |               |
//       V               V               V
// +---- out[2] ----+---- out[1] ----+---- out[0] ----+-------- g4 --------+-- in[0] --+-- in[1] --+-- in[2] --+
// |                |                |                |                    |           |           |           |
// | next=g2.out[1] | next=g2.out[0] | next=g3.out[0] | ...                |           |           |           |
// | idx=2          | idx=1          | idx=0          |                    |    g2     |    g0     |    g1     |
// | prev=null      | prev=null      | prev=null      | firstOut=g5.out[1] |           |           |           |
// |                |                |                |                    |           |           |           |
// +----------------+----------------+----------------+--------------------+-----------+-----------+-----------+
// ```

class Gate {
public:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    Gate(const GateMetaData* meta, GateId id, Gate *inList[], MachineType machineType, GateType type);
    static size_t GetGateSize(size_t numIns)
    {
        numIns = (numIns == 0) ? 1 : numIns;
        return numIns * (sizeof(In) + sizeof(Out)) + sizeof(Gate);
    }
    static size_t GetOutListSize(size_t numIns)
    {
        numIns = (numIns == 0) ? 1 : numIns;
        return numIns * sizeof(Out);
    }
    void NewIn(size_t idx, Gate *in);
    void ModifyIn(size_t idx, Gate *in);
    void DeleteIn(size_t idx);
    void DeleteGate();
    static constexpr GateRef InvalidGateRef = -1;
    [[nodiscard]] Out *GetOut(size_t idx);
    [[nodiscard]] Out *GetFirstOut();
    [[nodiscard]] const Out *GetOutConst(size_t idx) const;
    [[nodiscard]] const Out *GetFirstOutConst() const;
    // note: GetFirstOut() is not equal to GetOut(0)
    // note: behavior of GetFirstOut() is undefined when there are no Outs
    // note: use IsFirstOutNull() to check first if there may be no Outs
    void SetFirstOut(const Out *firstOut);
    void SetFirstOutNull();
    [[nodiscard]] bool IsFirstOutNull() const;
    [[nodiscard]] In *GetIn(size_t idx);
    [[nodiscard]] const In *GetInConst(size_t idx) const;
    [[nodiscard]] Gate *GetInGate(size_t idx);
    [[nodiscard]] const Gate *GetInGateConst(size_t idx) const;
    // note: behavior of GetInGate(idx) is undefined when Ins[idx] is deleted or not assigned
    // note: use IsInGateNull(idx) to check first if Ins[idx] may be deleted or not assigned
    [[nodiscard]] bool IsInGateNull(size_t idx) const;
    [[nodiscard]] OpCode GetOpCode() const;
    [[nodiscard]] GateId GetId() const;
    [[nodiscard]] size_t GetNumIns() const;
    [[nodiscard]] size_t GetStateCount() const;
    [[nodiscard]] size_t GetDependCount() const;
    [[nodiscard]] size_t GetInValueCount() const;
    [[nodiscard]] size_t GetInFrameStateCount() const;
    [[nodiscard]] size_t GetInValueStarts() const;
    [[nodiscard]] size_t GetRootCount() const;
    [[nodiscard]] size_t GetInFrameStateStarts() const;
    void Print(std::string bytecode = "", bool inListPreview = false, size_t highlightIdx = -1,
        std::string_view comment = "") const;
    std::string ToString(std::string bytecode = "", bool inListPreview = false, size_t highlightIdx = -1,
        std::string_view comment = "") const;
    void ShortPrint(std::string bytecode = "", bool inListPreview = false, size_t highlightIdx = -1) const;
    size_t PrintInGate(size_t numIns, size_t idx, size_t size, bool inListPreview, size_t highlightIdx,
                       std::ostringstream &log, bool isEnd = false) const;
    void PrintGateWithAdditionOp(std::string additionOp, std::string_view comment) const;
    void PrintWithBytecode(std::string_view comment) const;
    void CheckNullInput() const;
    void CheckStateInput() const;
    void CheckValueInput(bool isArch64) const;
    void CheckDependInput() const;
    void CheckRootInput() const;
    void CheckFrameStateInput() const;
    void CheckStateOutput(const std::string& methodName) const;
    std::string GetValueInAndOut(bool inListPreview = false, size_t highlightIdx = -1) const;
    void CheckBranchOutput() const;
    void CheckNOP() const;
    void CheckSelector() const;
    void CheckRelay() const;
    void Verify(bool isArch64, const std::string& methodName) const;
    [[nodiscard]] MarkCode GetMark(TimeStamp stamp) const;
    void SetMark(MarkCode mark, TimeStamp stamp);
    MachineType GetMachineType() const
    {
        return machineType_;
    }
    void SetMachineType(MachineType machineType)
    {
        machineType_ = machineType;
    }
    GateType GetGateType() const
    {
        return type_;
    }
    void SetGateType(GateType type)
    {
        type_ = type;
    }
    const GateMetaData* GetMetaData() const
    {
        return meta_;
    }

    const OneParameterMetaData* GetOneParameterMetaData() const
    {
        return OneParameterMetaData::Cast(meta_);
    }

    const StringMetaData* GetStringMetaData() const
    {
        ASSERT(meta_->IsStringType());
        return static_cast<const StringMetaData*>(meta_);
    }

    const JSBytecodeMetaData* GetJSBytecodeMetaData() const
    {
        return JSBytecodeMetaData::Cast(meta_);
    }

    const BoolMetaData* GetBoolMetaData() const
    {
        return BoolMetaData::Cast(meta_);
    }

    const TypedCallMetaData* GetTypedCallMetaData() const
    {
        return TypedCallMetaData::Cast(meta_);
    }

    const NewConstructMetaData* GetNewConstructMetaData() const
    {
        return NewConstructMetaData::Cast(meta_);
    }

    std::string MachineTypeStr(MachineType machineType) const;
    std::string GateTypeStr(GateType gateType) const;
    ~Gate() = default;

private:
    friend class Circuit;
    friend class GateAccessor;
    void CheckInputOpcode(size_t idx, OpCode expected) const;
    void CheckInputMachineType(size_t idx, MachineType expected, bool isArch64) const;
    void CheckNotInputMachineType(size_t idx, MachineType notExpected) const;
    void CheckState(size_t idx) const;
    void CheckGeneralState(size_t idx) const;
    void CheckFailed(std::string errorString, size_t highlightIdx) const;
    void SetMetaData(const GateMetaData* meta)
    {
        meta_ = meta;
    }
    uint64_t TryGetValue() const
    {
        if (meta_->IsOneParameterKind()) {
            return GetOneParameterMetaData()->GetValue();
        }
        return 0;
    }
    void DumpHeader(std::ostringstream &oss, const std::string& additionOp) const;

    void DumpInputs(std::ostringstream &oss, bool inListPreview, size_t highlightIdx) const;

    void DumpOutputs(std::ostringstream &oss, bool inListPreview) const;
    std::string GetBytecodeStr() const;

    // ...
    // out(2)
    // out(1)
    // out(0)
    const GateMetaData *meta_ { nullptr }; // uintptr_t
    GateId id_ { 0 }; // uint32_t
    GateType type_ { GateType::Empty() }; // uint32_t
    MachineType machineType_ { MachineType::NOVALUE }; // uint8_t
    TimeStamp stamp_ { 0 }; // uint8_t
    MarkCode mark_ { MarkCode::NO_MARK }; // uint8_t
    uint8_t bitField_ { 0 };
    GateRef firstOut_ { 0 }; // int32_t
    // in(0)
    // in(1)
    // in(2)
    // ...
};
} // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_GATE_H
