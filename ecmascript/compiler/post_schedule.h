/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_POST_SCHEDULE_H
#define ECMASCRIPT_COMPILER_POST_SCHEDULE_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class PostSchedule {
public:
    using ControlFlowGraph = std::vector<std::vector<GateRef>>;

    PostSchedule(Circuit *circuit, bool enableLog, const std::string &name, Chunk *chunk, bool fastBarrier = false)
        : enableLog_(enableLog), methodName_(name), chunk_(chunk), circuit_(circuit), builder_(circuit), acc_(circuit),
          fastBarrier_(fastBarrier)
    {
    }

    void Run(ControlFlowGraph &result);

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void GenerateExtraBB(ControlFlowGraph &cfg);

    bool VisitHeapAlloc(GateRef gate, ControlFlowGraph &cfg, size_t bbIdx, size_t instIdx);
    bool VisitStore(GateRef gate, ControlFlowGraph &cfg, size_t bbIdx, size_t instIdx);
    bool VisitLoad(GateRef gate, ControlFlowGraph &cfg, size_t bbIdx, size_t instIdx);

    void ReplaceGateDirectly(std::vector<GateRef> &gates, ControlFlowGraph &cfg, size_t bbIdx, size_t instIdx);
    void ScheduleEndBB(std::vector<GateRef> &gates, ControlFlowGraph &cfg, size_t bbIdx, size_t instIdx);
    void ScheduleNewBB(std::vector<GateRef> &gates, ControlFlowGraph &cfg, size_t bbIdx);
    void ScheduleCurrentBB(const std::vector<GateRef> &gates, ControlFlowGraph &cfg, size_t bbIdx, size_t instIdx);

    void LoweringHeapAllocAndPrepareScheduleGate(GateRef gate,
                                                 std::vector<GateRef> &currentBBGates,
                                                 std::vector<GateRef> &successBBGates,
                                                 std::vector<GateRef> &failBBGates,
                                                 std::vector<GateRef> &endBBGates,
                                                 int64_t flag);
    void LoweringHeapAllocate(GateRef gate,
                              std::vector<GateRef> &currentBBGates,
                              std::vector<GateRef> &successBBGates,
                              std::vector<GateRef> &failBBGates,
                              std::vector<GateRef> &endBBGates,
                              int64_t flag);

    void LoweringStoreNoBarrierAndPrepareScheduleGate(GateRef gate, std::vector<GateRef> &currentBBGates);
    void LoweringStoreWithBarrierAndPrepareScheduleGate(GateRef gate, std::vector<GateRef> &currentBBGates);
    void LoweringStoreUnknownBarrierAndPrepareScheduleGate(GateRef gate,
                                                           std::vector<GateRef> &currentBBGates,
                                                           std::vector<GateRef> &barrierBBGates,
                                                           std::vector<GateRef> &endBBGates);

    void LoweringLoadNoBarrierAndPrepareScheduleGate(GateRef gate, std::vector<GateRef> &currentBBGates);
    void LoweringLoadWithBarrierAndPrepareScheduleGate(GateRef gate, std::vector<GateRef> &currentBBGates);

    void PrepareToScheduleNewGate(GateRef gate, std::vector<GateRef> &gates);
    MemoryAttribute::Barrier GetBarrierKind(GateRef gate);
    void ReplaceBBState(ControlFlowGraph &cfg, size_t bbIdx, std::vector<GateRef> &currentBBGates,
                        std::vector<GateRef> &endBBGates);
    MemoryAttribute::ShareFlag GetShareKind(panda::ecmascript::kungfu::GateRef gate);

    int SelectBarrier(MemoryAttribute::ShareFlag share, const CallSignature*& cs, std::string_view& comment);

    void PrintGraph(const char* title, ControlFlowGraph &cfg);

    bool enableLog_ {false};
    std::string methodName_;
    [[maybe_unused]]Chunk* chunk_ {nullptr};
    Circuit* circuit_ {nullptr};
    CircuitBuilder builder_;
    GateAccessor acc_;
    bool fastBarrier_ {false};
};
};  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_POST_SCHEDULE_H
