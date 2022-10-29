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

#ifndef ECMASCRIPT_COMPILER_GUARD_ELIMINATING_H
#define ECMASCRIPT_COMPILER_GUARD_ELIMINATING_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
class GuardEliminating {
public:
    GuardEliminating(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg,
        bool enableLog, const std::string& name)
        : bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
        enableLog_(enableLog), methodName_(name) {}
    
    ~GuardEliminating() = default;

    void Run();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    bool HasGuard(GateRef gate) const;
    void ProcessTwoConditions(GateRef gate, std::set<GateRef> &conditionSet);
    void ProcessOneCondition(GateRef gate, std::set<GateRef> &conditionSet);
    void RemoveConditionFromSet(GateRef condition, std::set<GateRef> &conditionSet);
    bool IsTrustedType(GateRef gate) const;
    void TrustedTypePropagate(std::queue<GateRef>& workList, const std::vector<GateRef>& checkList);
    void RemoveOneTrusted(GateRef gate);
    void RemoveTwoTrusted(GateRef gate);
    void RemoveTrustedTypeCheck(const std::vector<GateRef>& guardedList);
    BytecodeCircuitBuilder *bcBuilder_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    bool enableLog_ {false};
    std::string methodName_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_GUARD_ELIMINATING_H