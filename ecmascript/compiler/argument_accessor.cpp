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

#include "ecmascript/compiler/argument_accessor.h"

namespace panda::ecmascript::kungfu {
void ArgumentAccessor::NewCommonArg(const CommonArgIdx argIndex, MachineType machineType, GateType gateType)
{
    circuit_->NewArg(machineType, static_cast<size_t>(argIndex), gateType, argRoot_);
}

void ArgumentAccessor::NewArg(const size_t argIndex)
{
    circuit_->NewArg(MachineType::I64, argIndex, GateType::TaggedValue(), argRoot_);
}

// method must be set
size_t ArgumentAccessor::GetActualNumArgs() const
{
    ASSERT(method_ != nullptr);
    auto numArgs = method_->GetNumArgsWithCallField();
    return static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS) + numArgs;
}

// method must be set
GateRef ArgumentAccessor::GetArgGate(const size_t currentVreg) const
{
    ASSERT(method_ != nullptr);
    const size_t offsetArgs = method_->GetNumVregsWithCallField();
    ASSERT(currentVreg >= offsetArgs && currentVreg < offsetArgs + method_->GetNumArgs());
    auto reg = currentVreg - offsetArgs;
    auto haveFunc = method_->HaveFuncWithCallField();
    auto haveNewTarget = method_->HaveNewTargetWithCallField();
    auto haveThis = method_->HaveThisWithCallField();
    auto index = GetFunctionArgIndex(reg, haveFunc, haveNewTarget, haveThis);
    return args_.at(index);
}

bool ArgumentAccessor::ArgGateNotExisted(const size_t currentVreg)
{
    const size_t offsetArgs = method_->GetNumVregsWithCallField();
    if (currentVreg < offsetArgs || currentVreg >= offsetArgs + method_->GetNumArgs()) {
        return true;
    }
    return false;
}

GateRef ArgumentAccessor::GetTypedArgGate(const size_t argIndex) const
{
    if (argIndex == static_cast<size_t>(TypedArgIdx::FUNC)) {
        return GetCommonArgGate(CommonArgIdx::FUNC);
    }
    if (argIndex == static_cast<size_t>(TypedArgIdx::NEW_TARGET)) {
        return GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    }
    if (argIndex == static_cast<size_t>(TypedArgIdx::THIS_OBJECT)) {
        return GetCommonArgGate(CommonArgIdx::THIS_OBJECT);
    }
    return args_.at(argIndex - static_cast<size_t>(TypedArgIdx::NUM_OF_TYPED_ARGS) +
        static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS));
}

GateRef ArgumentAccessor::GetCommonArgGate(const CommonArgIdx arg) const
{
    return args_.at(static_cast<size_t>(arg));
}

size_t ArgumentAccessor::GetFunctionArgIndex(const size_t currentVreg, const bool haveFunc,
                                             const bool haveNewTarget, const bool haveThis) const
{
    size_t numCommonArgs = haveFunc + haveNewTarget + haveThis;
    // 2: number of common args
    if (numCommonArgs == 2) {
        if (!haveFunc && currentVreg == 0) {
            return static_cast<size_t>(CommonArgIdx::NEW_TARGET);
        }
        if (!haveFunc && currentVreg == 1) {
            return static_cast<size_t>(CommonArgIdx::THIS_OBJECT);
        }
        if (!haveNewTarget && currentVreg == 0) {
            return static_cast<size_t>(CommonArgIdx::FUNC);
        }
        if (!haveNewTarget && currentVreg == 1) {
            return static_cast<size_t>(CommonArgIdx::THIS_OBJECT);
        }
        if (!haveThis && currentVreg == 0) {
            return static_cast<size_t>(CommonArgIdx::FUNC);
        }
        if (!haveThis && currentVreg == 1) {
            return static_cast<size_t>(CommonArgIdx::NEW_TARGET);
        }
    }
    // 1: number of common args, 0: the index of currentVreg
    if (numCommonArgs == 1 && currentVreg == 0) {
        if (haveFunc) {
            return static_cast<size_t>(CommonArgIdx::FUNC);
        }
        if (haveNewTarget) {
            return static_cast<size_t>(CommonArgIdx::NEW_TARGET);
        }
        if (haveThis) {
            return static_cast<size_t>(CommonArgIdx::THIS_OBJECT);
        }
    }
    return currentVreg - numCommonArgs + static_cast<size_t>(CommonArgIdx::NUM_OF_ARGS);
}

void ArgumentAccessor::FillArgsGateType(const TypeRecorder *typeRecorder)
{
    ASSERT(method_ != nullptr);
    GateAccessor gateAcc(circuit_);
    const size_t numOfTypedArgs = method_->GetNumArgsWithCallField() +
        static_cast<size_t>(TypedArgIdx::NUM_OF_TYPED_ARGS);
    for (uint32_t argIndex = 0; argIndex < numOfTypedArgs; argIndex++) {
        auto argType = typeRecorder->GetArgType(argIndex);
        if (!argType.IsAnyType()) {
            auto gate = GetTypedArgGate(argIndex);
            gateAcc.SetGateType(gate, argType);
        }
    }
}

void ArgumentAccessor::CollectArgs()
{
    if (args_.size() == 0) {
        GateAccessor(circuit_).GetArgsOuts(args_);
        std::reverse(args_.begin(), args_.end());
        if (method_ == nullptr) {
            return;
        }
        if (method_->IsFastCall() && args_.size() > 2) { // 2: mean have func and glue
            GateRef actualArgcGate = circuit_->GetConstantGate(MachineType::I64, 0, GateType::NJSValue());
            GateRef newTargetGate = circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED,
                GateType::UndefinedType());
            args_.insert(args_.begin() + 1, actualArgcGate);
            args_.insert(args_.begin() + 3, newTargetGate); // 3: newtarget index
        }
    }
}

GateRef ArgumentAccessor::GetFrameArgsIn(GateRef gate, FrameArgIdx idx)
{
    GateAccessor gateAcc(circuit_);
    ASSERT(gateAcc.GetOpCode(gate) == OpCode::JS_BYTECODE || gateAcc.GetOpCode(gate) == OpCode::FRAME_STATE);
    GateRef frameArgs = Circuit::NullGate();
    if (gateAcc.GetOpCode(gate) == OpCode::JS_BYTECODE) {
        frameArgs = gateAcc.GetFrameState(gate);
        ASSERT(gateAcc.GetOpCode(frameArgs) == OpCode::FRAME_ARGS);
    } else {
        frameArgs = gateAcc.GetValueIn(gate, 0); // 0: frame args
    }
    return gateAcc.GetValueIn(frameArgs, static_cast<size_t>(idx));
}
}  // namespace panda::ecmascript::kungfu
