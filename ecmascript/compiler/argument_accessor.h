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

#ifndef ECMASCRIPT_COMPILER_ARGUMENT_ACCESSOR_H
#define ECMASCRIPT_COMPILER_ARGUMENT_ACCESSOR_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::kungfu {
enum class CommonArgIdx : uint8_t {
    GLUE = 0,
    ACTUAL_ARGC,
    ACTUAL_ARGV,
    FUNC,
    NEW_TARGET,
    THIS_OBJECT,
    NUM_OF_ARGS,
};

enum class FastCallArgIdx : uint8_t {
    GLUE = 0,
    FUNC,
    THIS_OBJECT,
    NUM_OF_ARGS,
};

enum class FrameArgIdx : uint8_t {
    FUNC = 0,
    NEW_TARGET,
    THIS_OBJECT,
    ACTUAL_ARGC,
    ACTUAL_ARGV,
    SHARED_CONST_POOL,
    UNSHARED_CONST_POOL,
    NUM_OF_ARGS,
};

class ArgumentAccessor {
public:
    explicit ArgumentAccessor(
        Circuit *circuit, const MethodLiteral *methodLiteral = nullptr)
        : circuit_(circuit),
          method_(methodLiteral),
          argRoot_(circuit->GetArgRoot()),
          args_(0),
          frameArgs_{Circuit::NullGate()}
    {
        CollectArgs();
    }
    ~ArgumentAccessor() = default;

    void NewCommonArg(const CommonArgIdx argIndex, MachineType machineType, GateType gateType);
    void NewArg(const size_t argIndex);
    // method must be set
    size_t GetActualNumArgs() const;
    // method must be set
    GateRef GetArgGate(const size_t currentVreg) const;
    bool ArgGateNotExisted(const size_t currentVreg);
    GateRef ArgsAt(const size_t index) const
    {
        return args_.at(index);
    }
    size_t ArgsCount() const
    {
        return args_.size();
    }
    void CollectArgs();
    static size_t GetFixArgsNum()
    {
        return static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS) - static_cast<size_t>(CommonArgIdx::FUNC);
    }

    static size_t GetExtraArgsNum()
    {
        return static_cast<size_t>(CommonArgIdx::ACTUAL_ARGV) - static_cast<size_t>(CommonArgIdx::GLUE);
    }

    GateRef GetFrameArgs() const
    {
        return frameArgs_;
    }

    void SetFrameArgs(GateRef frameArgs)
    {
        frameArgs_ = frameArgs;
    }
    GateRef GetFrameArgsIn(GateRef gate, FrameArgIdx idx);

private:
    // Disables using this interface during lowering, only allows it to be used during building graph.
    GateRef GetCommonArgGate(const CommonArgIdx arg) const;
    size_t GetFunctionArgIndex(const size_t currentVreg, const bool haveFunc,
                               const bool haveNewTarget, const bool haveThis) const;

    Circuit *circuit_ {nullptr};
    const MethodLiteral *method_ {nullptr};
    GateRef argRoot_;
    std::vector<GateRef> args_;
    GateRef frameArgs_;

    friend class BytecodeCircuitBuilder;
    friend class AsyncFunctionLowering;
    friend class InitializationAnalysis;
};
}
#endif  // ECMASCRIPT_COMPILER_ARGUMENT_ACCESSOR_H
