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
#include "ecmascript/compiler/type_recorder.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::kungfu {
enum class CommonArgIdx : uint8_t {
    GLUE = 0,
    LEXENV,
    ACTUAL_ARGC,
    FUNC,
    NEW_TARGET,
    THIS_OBJECT,
    NUM_OF_ARGS,
};

class ArgumentAccessor {
public:
    explicit ArgumentAccessor(
        Circuit *circuit, const MethodLiteral *methodLiteral = nullptr)
        : circuit_(circuit),
          method_(methodLiteral),
          argRoot_(circuit->GetArgRoot()),
          args_(0)
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
    GateRef GetCommonArgGate(const CommonArgIdx arg) const;
    GateRef ArgsAt(const size_t index) const
    {
        return args_.at(index);
    }
    size_t ArgsCount() const
    {
        return args_.size();
    }
    void FillArgsGateType(const TypeRecorder *typeRecorder);
    void CollectArgs();
    static size_t GetFixArgsNum()
    {
        return static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS) - static_cast<size_t>(CommonArgIdx::FUNC);
    }

private:
    size_t GetFunctionArgIndex(const size_t currentVreg, const bool haveFunc,
                               const bool haveNewTarget, const bool haveThis) const;
    GateRef GetTypedArgGate(const size_t argIndex) const;

    Circuit *circuit_ {nullptr};
    const MethodLiteral *method_ {nullptr};
    GateRef argRoot_;
    std::vector<GateRef> args_;
};
}
#endif  // ECMASCRIPT_COMPILER_ARGUMENT_ACCESSOR_H
