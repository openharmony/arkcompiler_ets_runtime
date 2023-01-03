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

#ifndef ECMASCRIPT_COMPILER_CHECK_ELIMINATION_H
#define ECMASCRIPT_COMPILER_CHECK_ELIMINATION_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
class CheckElimination {
public:
    CheckElimination(Circuit *circuit, CompilationConfig *cmpCfg,
        bool enableLog, const std::string& name)
        : circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
        enableLog_(enableLog), methodName_(name) {}

    ~CheckElimination() = default;

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

    bool IsTrustedType(GateRef gate) const;
    bool IsCheckWithTwoIns(GateRef gate) const;
    bool IsCheckWithOneIn(GateRef gate) const;
    bool IsPrimitiveTypeCheck(GateRef gate) const;
    void TrustedTypePropagate(std::queue<GateRef>& workList, const std::vector<GateRef>& checkList);
    void RemoveCheck(GateRef gate);
    void RemovePassedCheck();
    void RemoveTypeTrustedCheck();

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    bool enableLog_ {false};
    std::string methodName_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_CHECK_ELIMINATION_H