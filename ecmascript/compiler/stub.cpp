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
#include "ecmascript/compiler/stub.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/compiler/call_signature.h"

namespace panda::ecmascript::kungfu {
Stub::Stub(const CallSignature *callSignature, Circuit *circuit)
    : callSignature_(callSignature),
      builder_(circuit),
      acc_(circuit),
      env_(callSignature->GetParametersCount(), &builder_)
{
    ASSERT(callSignature_->HasConstructor());
    stubBuilder_ = static_cast<StubBuilder*>(callSignature_->GetConstructor()(&env_));
}

void Stub::InitializeArguments()
{
    auto argLength = callSignature_->GetParametersCount();
    auto paramsType = callSignature_->GetParametersType();
    for (size_t i = 0; i < argLength; i++) {
        GateRef argument = env_.GetArgument(i);
        if (paramsType[i] == VariableType::NATIVE_POINTER()) {
            auto type = env_.IsArch64Bit() ? MachineType::I64 : MachineType::I32;
            acc_.SetMachineType(argument, type);
        } else {
            acc_.SetMachineType(argument, paramsType[i].GetMachineType());
        }
        acc_.SetGateType(argument, paramsType[i].GetGateType());
    }
}

void Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    env_.SetCompilationConfig(cfg);
    InitializeArguments();
    stubBuilder_->GenerateCircuit();
}

Stub::~Stub()
{
    if (stubBuilder_ != nullptr) {
        delete stubBuilder_;
        stubBuilder_ = nullptr;
    }
}
}  // namespace panda::ecmascript::kungfu