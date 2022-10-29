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

#ifndef ECMASCRIPT_COMPILER_GUARD_LOWERING_PASS_H
#define ECMASCRIPT_COMPILER_GUARD_LOWERING_PASS_H

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/circuit_builder.h"

namespace panda::ecmascript::kungfu {
class GuardLowering {
public:
    GuardLowering(BytecodeCircuitBuilder *builder, CompilationConfig *cmpCfg, Circuit *circuit, std::string name,
        bool enableLog)
        : bcBuilder_(builder), builder_(circuit, cmpCfg), circuit_(circuit),
          acc_(circuit), methodName_(name), enableLog_(enableLog) {}
    
    ~GuardLowering() = default;

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

    void LowerGuard(GateRef gate);
    BytecodeCircuitBuilder *bcBuilder_ {nullptr};
    CircuitBuilder builder_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    std::string methodName_;
    bool enableLog_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_GUARD_LOWERING_PASS_H