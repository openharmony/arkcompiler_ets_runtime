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

#ifndef ECMASCRIPT_COMPILER_STUB_H
#define ECMASCRIPT_COMPILER_STUB_H

#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/call_signature.h"

namespace panda::ecmascript::kungfu {
class StubBuilder;
class Stub {
public:
    Stub(const CallSignature *callSignature, Circuit *circuit);
    ~Stub();
    NO_MOVE_SEMANTIC(Stub);
    NO_COPY_SEMANTIC(Stub);

    const CallSignature *GetCallSignature() const
    {
        return callSignature_;
    }

    const std::string &GetMethodName() const
    {
        return callSignature_->GetName();
    }

    Circuit *GetCircuit()
    {
        return env_.GetCircuit();
    }

    void GenerateCircuit(const CompilationConfig *cfg);

private:
    void InitializeArguments();
    const CallSignature *callSignature_;
    CircuitBuilder builder_;
    GateAccessor acc_;
    Environment env_;
    StubBuilder *stubBuilder_ {nullptr};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_H
