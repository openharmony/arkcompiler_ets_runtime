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
#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_INL_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_INL_H

#include "ecmascript/compiler/lcr_circuit_builder.h"
#include "ecmascript/compiler/mcr_circuit_builder.h"
#include "ecmascript/compiler/hcr_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/method.h"

namespace panda::ecmascript::kungfu {
// constant
GateRef CircuitBuilder::True()
{
    return Boolean(true);
}

GateRef CircuitBuilder::False()
{
    return Boolean(false);
}

GateRef CircuitBuilder::Undefined()
{
    return UndefineConstant();
}

GateRef CircuitBuilder::Hole()
{
    return HoleConstant();
}

GateRef CircuitBuilder::DoubleIsINF(GateRef x)
{
    GateRef infinity = Double(base::POSITIVE_INFINITY);
    GateRef negativeInfinity = Double(-base::POSITIVE_INFINITY);
    GateRef diff1 = DoubleEqual(x, infinity);
    GateRef diff2 = DoubleEqual(x, negativeInfinity);
    return BitOr(diff1, diff2);
}

GateRef CircuitBuilder::DoubleIsNanOrInf(GateRef x)
{
    return BitOr(DoubleIsNAN(x), DoubleIsINF(x));
}

// Js World
// cast operation

GateRef CircuitBuilder::GetGlobalConstantOffset(ConstantIndex index)
{
    return PtrMul(IntPtr(sizeof(JSTaggedValue)), IntPtr(static_cast<int>(index)));
}

GateRef CircuitBuilder::GetExpectedNumOfArgs(GateRef method)
{
    GateRef callFieldOffset = IntPtr(Method::CALL_FIELD_OFFSET);
    GateRef callfield = LoadWithoutBarrier(VariableType::INT64(), method, callFieldOffset);
    return Int64And(
        Int64LSR(callfield, Int64(MethodLiteral::NumArgsBits::START_BIT)),
        Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1));
}

int CircuitBuilder::NextVariableId()
{
    return env_->NextVariableId();
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit)
{
    BRANCH(Equal(result, ExceptionConstant()), fail, success);
    Bind(fail);
    {
        Jump(exit);
    }
}

void CircuitBuilder::HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal)
{
    BRANCH(Equal(result, exceptionVal), fail, success);
    Bind(fail);
    {
        Jump(exit);
    }
}

void CircuitBuilder::SubCfgEntry(Label *entry)
{
    ASSERT(env_ != nullptr);
    env_->SubCfgEntry(entry);
}

void CircuitBuilder::SubCfgExit()
{
    ASSERT(env_ != nullptr);
    env_->SubCfgExit();
}

GateRef CircuitBuilder::Return(GateRef value)
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return Return(control, depend, value);
}

GateRef CircuitBuilder::Return()
{
    auto control = GetCurrentLabel()->GetControl();
    auto depend = GetCurrentLabel()->GetDepend();
    return ReturnVoid(control, depend);
}

void CircuitBuilder::Bind(Label *label)
{
    label->Bind();
    env_->SetCurrentLabel(label);
}

void CircuitBuilder::Bind(Label *label, bool justSlowPath)
{
    if (!justSlowPath) {
        label->Bind();
        env_->SetCurrentLabel(label);
    }
}

GateRef CircuitBuilder::GetState() const
{
    return GetCurrentLabel()->GetControl();
}

GateRef CircuitBuilder::GetDepend() const
{
    return GetCurrentLabel()->GetDepend();
}

StateDepend CircuitBuilder::GetStateDepend() const
{
    return StateDepend(GetState(), GetDepend());
}

void CircuitBuilder::SetDepend(GateRef depend)
{
    GetCurrentLabel()->SetDepend(depend);
}

void CircuitBuilder::SetState(GateRef state)
{
    GetCurrentLabel()->SetControl(state);
}

Label *CircuitBuilder::GetCurrentLabel() const
{
    return GetCurrentEnvironment()->GetCurrentLabel();
}

LogicAndBuilder::LogicAndBuilder(Environment *env)
    : env_(env), builder_(env_->GetBuilder())
{
    subentry_ = new Label(env_);
    builder_->SubCfgEntry(subentry_);
    result_ = new Variable(env_, VariableType::BOOL(), builder_->NextVariableId(), builder_->False());
    exit_ = new Label(env_);
}

LogicAndBuilder::~LogicAndBuilder()
{
    ASSERT(exit_ != nullptr);
    delete exit_;
    exit_ = nullptr;
    ASSERT(result_ != nullptr);
    delete result_;
    result_ = nullptr;
    ASSERT(subentry_ != nullptr);
    delete subentry_;
    subentry_ = nullptr;
    for (auto &label : labels_) {
        ASSERT(label != nullptr);
        delete label;
        label = nullptr;
    }
}

LogicAndBuilder &LogicAndBuilder::And(GateRef check)
{
    auto checkTrue = new Label(env_);
    labels_.push_back(checkTrue);
    builder_->Branch(check, checkTrue, exit_);
    builder_->Bind(checkTrue);
    return *this;
}

GateRef LogicAndBuilder::Done()
{
    *result_ = builder_->True();
    builder_->Jump(exit_);
    builder_->Bind(exit_);
    auto ret = **result_;
    builder_->SubCfgExit();
    return ret;
}

LogicOrBuilder::LogicOrBuilder(Environment *env)
    : env_(env), builder_(env_->GetBuilder())
{
    subentry_ = new Label(env_);
    builder_->SubCfgEntry(subentry_);
    result_ = new Variable(env_, VariableType::BOOL(), builder_->NextVariableId(), builder_->True());
    exit_ = new Label(env_);
}

LogicOrBuilder::~LogicOrBuilder()
{
    ASSERT(exit_ != nullptr);
    delete exit_;
    exit_ = nullptr;
    ASSERT(result_ != nullptr);
    delete result_;
    result_ = nullptr;
    ASSERT(subentry_ != nullptr);
    delete subentry_;
    subentry_ = nullptr;
    for (auto &label : labels_) {
        ASSERT(label != nullptr);
        delete label;
        label = nullptr;
    }
}

LogicOrBuilder &LogicOrBuilder::Or(GateRef check)
{
    auto checkFalse = new Label(env_);
    labels_.push_back(checkFalse);
    builder_->Branch(check, exit_, checkFalse);
    builder_->Bind(checkFalse);
    return *this;
}

GateRef LogicOrBuilder::Done()
{
    *result_ = builder_->False();
    builder_->Jump(exit_);
    builder_->Bind(exit_);
    auto ret = **result_;
    builder_->SubCfgExit();
    return ret;
}
} // namespace panda::ecmascript::kungfu

#endif
