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

#ifndef ECMASCRIPT_COMPILER_ASYNC_FUNCTION_LOWRING_H_
#define ECMASCRIPT_COMPILER_ASYNC_FUNCTION_LOWRING_H_

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class AsyncFunctionLowering {
public:
    AsyncFunctionLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg,
                          bool enableLog, const std::string& name)
        : bcBuilder_(bcBuilder), circuit_(circuit), builder_(circuit, cmpCfg), enableLog_(enableLog),
          accessor_(circuit), argAccessor_(circuit), stateEntry_(GetEntryBBStateOut()),
          dependEntry_(GetEntryBBDependOut()), methodName_(name)
    {
    }

    ~AsyncFunctionLowering() = default;

    void ProcessAll();

    bool IsAsyncRelated() const;

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    void ProcessJumpTable();

    void RebuildGeneratorCfg(GateRef resumeGate, GateRef restoreOffsetGate, GateRef ifFalseCondition, GateRef newTarget,
                             GateRef &firstState);

    void UpdateValueSelector(GateRef prevLoopBeginGate, GateRef controlStateGate, GateRef prevBcOffsetPhiGate,
                             bool genNewValuePhiGate = true);

    void CheckResumeInLoopBody(GateRef stateInGate, bool &resumeInLoopBody);

    void ModifyStateInput(GateRef stateInGate, GateRef ifBranch, GateRef ifFalse);

    GateRef GetDependPhiFromLoopBegin(GateRef loopbegin) const;

    GateRef GetEntryBBStateOut() const;

    GateRef GetEntryBBDependOut() const;

    BytecodeCircuitBuilder *bcBuilder_;
    Circuit *circuit_;
    CircuitBuilder builder_;
    bool enableLog_ {false};
    GateAccessor accessor_;
    ArgumentAccessor argAccessor_;
    GateRef stateEntry_ {Circuit::NullGate()};
    GateRef dependEntry_ {Circuit::NullGate()};
    std::string methodName_;
};
}  // panda::ecmascript::kungfu

#endif // ECMASCRIPT_COMPILER_ASYNC_FUNCTION_LOWRING_H_
