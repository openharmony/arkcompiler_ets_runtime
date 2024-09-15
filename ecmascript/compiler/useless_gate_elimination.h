/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_USELESS_GATE_ELIMINATION_H
#define ECMASCRIPT_COMPILER_USELESS_GATE_ELIMINATION_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {

class UselessGateElimination {
public:
    UselessGateElimination(Circuit* circuit, bool enableLog, std::string name)
        : enableLog_(enableLog), methodName_(name), circuit_(circuit),
          acc_(circuit) {}
    void Run();
private:
    void PushGate(GateRef gate);
    void InitList();
    void MarkGate();
    void ReplaceDead(GateRef gate);
    void EliminateUnmarkedGate();
    std::string GetMethodName();
    bool enableLog_;
    std::string methodName_;
    Circuit* circuit_;
    GateAccessor acc_;
    std::vector<GateRef> workList_;
    std::vector<GateRef> gateList_;
};

}
#endif