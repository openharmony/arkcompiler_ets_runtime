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

#ifndef ECMASCRIPT_COMPILER_BRANCH_ELIMINATION_H
#define ECMASCRIPT_COMPILER_BRANCH_ELIMINATION_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include <cstdint>

namespace panda::ecmascript::kungfu {
class BranchElimination : public PassVisitor {

public:
    BranchElimination(Circuit *circuit, RPOVisitor *visitor, bool enableLog,
                        Chunk *chunk)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), acc_(circuit),
          builder_(circuit), enableLog_(enableLog) {}
    ~BranchElimination() = default;
    void Print() const;
    GateRef VisitGate(GateRef gate);

private:
    inline bool IsBoolType(GateRef gate) const
    {
        return acc_.GetMachineType(gate) == MachineType::I1;
    }

    GateRef TryEliminateIfBranch(GateRef gate);

    void AddEliminatingCount()
    {
        eliminatingCount_++;
    }
    void AddTotalBranchCount()
    {
        totalBranchCount_++;
    }

    uint32_t GetEliminatingCount() const
    {
        return eliminatingCount_;
    }

    uint32_t GetTotalBranchCount() const
    {
        return totalBranchCount_;
    }

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    uint32_t eliminatingCount_{0};
    uint32_t totalBranchCount_{0};
    Circuit *circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    bool enableLog_;
};

} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_BRANCH_ELIMINATION_H
