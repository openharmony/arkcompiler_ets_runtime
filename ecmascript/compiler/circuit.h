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
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/compiler/gate_meta_data_builder.h"
#include "ecmascript/frames.h"

#include "libpandabase/macros.h"
#include "securec.h"

namespace panda::ecmascript::kungfu {
class DebugInfo;
enum class VisitState : uint8_t {
    UNVISITED,
    PENDING,
    VISITED
};

class Circuit {  // note: calling NewGate could make all saved Gate* invalid
public:
    explicit Circuit(NativeAreaAllocator* allocator, DebugInfo* dInfo = nullptr, const char* funcName = nullptr,
                     bool isArch64 = true);
    ~Circuit();
    NO_COPY_SEMANTIC(Circuit);
    NO_MOVE_SEMANTIC(Circuit);

    GateRef NewGate(const GateMetaData *meta, const std::vector<GateRef> &inList, const char* comment = nullptr);
    GateRef NewGate(const GateMetaData *meta, MachineType machineType, GateType type, const char* comment = nullptr);
    GateRef NewGate(const GateMetaData *meta, MachineType machineType,
        const std::initializer_list<GateRef>& args, GateType type, const char* comment = nullptr);
    GateRef NewGate(const GateMetaData *meta, MachineType machineType, size_t numIns,
                    const GateRef inList[], GateType type, const char* comment = nullptr);
    void PrintAllGates() const;
    void PrintAllGatesWithBytecode() const;
    void GetAllGates(std::vector<GateRef>& gates) const;
    static GateRef NullGate();
    void Verify(GateRef gate) const;
    panda::ecmascript::FrameType GetFrameType() const;
    void SetFrameType(panda::ecmascript::FrameType type);
    GateRef GetConstantGate(MachineType machineType, uint64_t value, GateType type);
    void ClearConstantCache(MachineType machineType, uint64_t value, GateType type);
    GateRef GetConstantStringGate(MachineType machineType, const std::string &str, GateType type);
    GateRef NewArg(MachineType machineType, size_t index, GateType type, GateRef argRoot);
    GateRef GetInitialEnvGate(GateRef jsFunc);
    size_t GetGateCount() const;
    TimeStamp GetTime() const;
    void AdvanceTime() const;
    void SetArch(bool isArch64)
    {
        isArch64_ = isArch64;
    }
    bool IsArch64() const
    {
        return isArch64_;
    }
    void InitRoot();
    GateRef GetRoot() const
    {
        return root_;
    }
    void SetRoot(GateRef root)
    {
        root_ = root;
    }

    Chunk* chunk()
    {
        return &chunk_;
    }

    GateMetaBuilder *GetMetaBuilder()
    {
        return &metaBuilder_;
    }

    GateRef GetStateRoot() const;
    GateRef GetDependRoot() const;
    GateRef GetArgRoot() const;
    GateRef GetReturnRoot() const;

#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
    const GateMetaData* NAME()                  \
    {                                           \
        return metaBuilder_.NAME();             \
    }
    IMMUTABLE_META_DATA_CACHE_LIST(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
    const GateMetaData* NAME(size_t value)      \
    {                                           \
        return metaBuilder_.NAME(value);        \
    }
    GATE_META_DATA_LIST_WITH_SIZE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
    const GateMetaData* NAME(uint64_t value)    \
    {                                           \
        return metaBuilder_.NAME(value);        \
    }
    GATE_META_DATA_LIST_WITH_ONE_PARAMETER(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META(NAME, OP, R, S, D, V) \
    const GateMetaData* NAME(bool value)        \
    {                                           \
        return metaBuilder_.NAME(value);        \
    }
    GATE_META_DATA_LIST_WITH_BOOL(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META_WITH_BOOL_VALUE_IN(NAME, OP, R, S, D, V)          \
    const GateMetaData* NAME(size_t value, bool flag)                       \
    {                                                                       \
        return metaBuilder_.NAME(value, flag);                              \
    }
    GATE_META_DATA_LIST_WITH_BOOL_VALUE_IN(DECLARE_GATE_META_WITH_BOOL_VALUE_IN)
#undef DECLARE_GATE_META_WITH_BOOL_VALUE_IN

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                    \
    const GateMetaData* NAME(uint64_t value, uint64_t pcOffset)    \
    {                                                              \
        return metaBuilder_.NAME(value, pcOffset);                 \
    }
    GATE_META_DATA_LIST_WITH_PC_OFFSET(DECLARE_GATE_META)
#undef DECLARE_GATE_META

#define DECLARE_GATE_META_FOR_CALL(NAME, OP, R, S, D, V)                        \
    const GateMetaData* NAME(uint64_t value, uint64_t pcOffset, bool noGC)      \
    {                                                                           \
        return metaBuilder_.NAME(value, pcOffset, noGC);                        \
    }
    GATE_META_DATA_LIST_FOR_CALL(DECLARE_GATE_META_FOR_CALL)
#undef DECLARE_GATE_META_FOR_CALL

#define DECLARE_GATE_META(NAME, OP, R, S, D, V)                    \
    const GateMetaData* NAME(uint64_t pcOffset) const              \
    {                                                              \
        return metaBuilder_.NAME(pcOffset);                        \
    }
    GATE_META_DATA_LIST_WITH_PC_OFFSET_FIXED_VALUE(DECLARE_GATE_META)
#undef DECLARE_GATE_META

    const GateMetaData* Nop()
    {
        return metaBuilder_.Nop();
    }

    const GateMetaData* JSBytecode(size_t valuesIn, EcmaOpcode opcode,
        uint32_t pcOffset, bool writable, bool hasFrameState)
    {
        GateFlags writableFlags = writable ? GateFlags::NONE_FLAG : GateFlags::NO_WRITE;
        GateFlags frameStateFlags = hasFrameState ? GateFlags::HAS_FRAME_STATE : GateFlags::NONE_FLAG;
        GateFlags flags = static_cast<GateFlags>(writableFlags | frameStateFlags);
        return metaBuilder_.JSBytecode(valuesIn, opcode, pcOffset, flags);
    }

    const GateMetaData* TypedBinaryOp(uint64_t value, TypedBinOp binOp, PGOSampleType type)
    {
        return metaBuilder_.TypedBinaryOp(value, binOp, type);
    }

    const GateMetaData* TypedCallTargetCheckOp(uint32_t numIns, uint64_t value, TypedCallTargetCheckOp checkOp)
    {
        return metaBuilder_.TypedCallTargetCheckOp(numIns, value, checkOp);
    }

    GateRef DeadGate()
    {
        if (dead_ == NullGate()) {
            dead_ = NewGate(Dead(), MachineType::NOVALUE, GateType::Empty());
        }
        return dead_;
    }

    size_t GetMaxGateId() const
    {
        ASSERT(gateCount_ != 0);
        return static_cast<size_t>(gateCount_ - 1);
    }

    bool IsOptimizedJSFunctionFrame() const
    {
        return frameType_ == panda::ecmascript::FrameType::OPTIMIZED_JS_FUNCTION_FRAME
            || frameType_ == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
    }

    bool GetDebugInfo(GateRef g, size_t &index) const;

private:
    static const size_t CIRCUIT_SPACE = 1U << 30U;  // 1GB

    void Print(GateRef gate) const;
    GateType GetGateType(GateRef gate) const;
    GateRef GetGateRef(const Gate *gate) const;
    MachineType GetMachineType(GateRef gate) const;
    void SetMark(GateRef gate, MarkCode mark) const;
    OpCode GetOpCode(GateRef gate) const;
    void SetMachineType(GateRef gate, MachineType machineType);
    void SetGateType(GateRef gate, GateType type);
    GateId GetId(GateRef gate) const;
    void DeleteGate(GateRef gate);
    void DecreaseIn(GateRef gate, size_t idx);

    MarkCode GetMark(GateRef gate) const;
    void DeleteIn(GateRef gate, size_t idx);
    void ModifyIn(GateRef gate, size_t idx, GateRef in);
    void NewIn(GateRef gate, size_t idx, GateRef in);
    std::vector<GateRef> GetOutVector(GateRef gate) const;
    bool IsFirstOutNull(GateRef gate) const;
    bool IsInGateNull(GateRef gate, size_t idx) const;
    GateRef GetIn(GateRef gate, size_t idx) const;
    bool IsValueSelector(GateRef gate) const;
    bool IsSelector(GateRef gate) const;
    bool IsControlCase(GateRef gate) const;
    bool IsLoopHead(GateRef gate) const;
    void ResetAllGateTimeStamps() const;
    uint8_t *AllocateSpace(size_t gateSize);
    Gate *AllocateGateSpace(size_t numIns);
    size_t GetCircuitDataSize() const;
    const void *GetSpaceDataStartPtrConst() const;
    const void *GetSpaceDataEndPtrConst() const;
    const uint8_t *GetDataPtrConst(size_t offset) const;
    uint8_t *GetDataPtr(size_t offset);
    Gate *LoadGatePtr(GateRef shift);
    const Gate *LoadGatePtrConst(GateRef shift) const;
    bool AddComment(GateRef g, const char* str);
    const GateMetaData *GetMetaData(GateRef gate) const
    {
        return LoadGatePtrConst(gate)->GetMetaData();
    }

private:
    void* space_ {nullptr};
    size_t circuitSize_ {0};
    size_t gateCount_ {0};
    TimeStamp time_;
    std::map<std::tuple<MachineType, BitField, GateType>, GateRef> constantCache_ {};
    std::map<std::pair<BitField, GateRef>, GateRef> constantDataCache_ {};
    std::map<GateRef, GateRef> initialEnvCache_ {};
    panda::ecmascript::FrameType frameType_ {panda::ecmascript::FrameType::OPTIMIZED_FRAME};
    bool isArch64_ { false };

    Chunk chunk_;
    GateRef root_ { NullGate() };
    GateRef dead_ { NullGate() };
    GateRef replaceable_ { NullGate() };
    GateMetaBuilder metaBuilder_;
    ChunkMap<GateRef, size_t> gateToDInfo_;
    DebugInfo* debugInfo_ {nullptr};
#ifndef NDEBUG
    ChunkVector<GateRef> allGates_;
#endif

    friend class GateAccessor;
    friend class ConstGateAccessor;
    friend class Verifier;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_H
