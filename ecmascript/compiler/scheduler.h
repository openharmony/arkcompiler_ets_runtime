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

#ifndef ECMASCRIPT_COMPILER_SCHEDULER_H
#define ECMASCRIPT_COMPILER_SCHEDULER_H

#include "ecmascript/compiler/circuit.h"

namespace panda::ecmascript::kungfu {
class Scheduler {
public:
    using ControlFlowGraph = std::vector<std::vector<GateRef>>;

    static void CalculateDominatorTree(const Circuit *circuit, std::vector<GateRef>& bbGatesList,
                                       std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                       std::vector<size_t> &immDom);

    static void Run(const Circuit *circuit, ControlFlowGraph &result,
                    [[maybe_unused]] const std::string& methodName = "", bool enableLog = false);

    static bool CalculateSchedulingUpperBound(const Circuit *circuit,
                                              const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                              const std::function<bool(size_t, size_t)> &isAncestor,
                                              const std::vector<GateRef> &schedulableGatesList,
                                              std::unordered_map<GateRef, size_t> &upperBound);

    static void CalculateSchedulingLowerBound(const Circuit *circuit,
                                              const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                              const std::function<size_t(size_t, size_t)> &lowestCommonAncestor,
                                              std::unordered_map<GateRef, size_t> &lowerBound,
                                              std::vector<GateRef> *order = nullptr);

    static void Print(const ControlFlowGraph *cfg, const Circuit *circuit);

private:
    static void PrintUpperBoundError(const Circuit *circuit, GateRef curGate,
                                     GateRef predUpperBound, GateRef curUpperBound);
    static void CalculateFixedGatesList(const Circuit *circuit,
                                        const std::unordered_map<GateRef, size_t> &bbGatesAddrToIdx,
                                        std::vector<GateRef> &bbAndFixedGatesList);
};
};  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_SCHEDULER_H
