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

#ifndef ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H
#define ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {
class TSTypeLowering {
public:
    TSTypeLowering(Circuit *circuit, PassInfo *info,
                   bool enableLog, const std::string& name)
        : circuit_(circuit), acc_(circuit), builder_(circuit, info->GetCompilerConfig()),
          dependEntry_(circuit->GetDependRoot()),
          tsManager_(info->GetTSManager()),
          enableLog_(enableLog),
          profiling_(info->GetCompilerConfig()->IsProfiling()),
          methodName_(name), glue_(acc_.GetGlueFromArgList()) {}

    ~TSTypeLowering() = default;

    void RunTSTypeLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsProfiling() const
    {
        return profiling_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void Lower(GateRef gate);
    void VerifyGuard() const;
    void DeleteGates(GateRef hir, std::vector<GateRef> &unusedGate);
    void ReplaceHIRGate(GateRef hir, GateRef outir, GateRef state, GateRef depend,
                        std::vector<GateRef> &unuseGate);
    void LowerTypedAdd(GateRef gate);
    void LowerTypedSub(GateRef gate);
    void LowerTypedMul(GateRef gate);
    void LowerTypedMod(GateRef gate);
    void LowerTypedLess(GateRef gate);
    void LowerTypedLessEq(GateRef gate);
    void LowerTypedGreater(GateRef gate);
    void LowerTypedGreaterEq(GateRef gate);
    void LowerTypedDiv(GateRef gate);
    void LowerTypedEq(GateRef gate);
    void LowerTypedNotEq(GateRef gate);
    void LowerTypedShl(GateRef gate);
    void LowerTypedShr(GateRef gate);
    void LowerTypedAshr(GateRef gate);
    void LowerTypedAnd(GateRef gate);
    void LowerTypedOr(GateRef gate);
    void LowerTypedXor(GateRef gate);
    void LowerTypedInc(GateRef gate);
    void LowerTypedDec(GateRef gate);
    void LowerTypeToNumeric(GateRef gate);
    void LowerPrimitiveTypeToNumber(GateRef gate);
    void LowerConditionJump(GateRef gate);
    void LowerTypedNeg(GateRef gate);
    void LowerTypedNot(GateRef gate);
    void LowerTypedLdObjByName(GateRef gate);
    void LowerTypedLdArrayLength(GateRef gate);
    void LowerTypedStObjByName(GateRef gate, bool isThis);
    void LowerTypedLdObjByIndex(GateRef gate);
    void LowerTypedStObjByIndex(GateRef gate);
    void LowerTypedLdObjByValue(GateRef gate, bool isThis);
    void LowerTypedIsTrueOrFalse(GateRef gate, bool flag);
    void LowerTypedNewObjRange(GateRef gate);
    void LowerTypedSuperCall(GateRef gate, GateRef ctor, GateRef newTarget);

    void LowerCallThis1Imm8V8V8(GateRef gate);
    bool CheckParam(GateRef gate, bool isCallThis, MethodLiteral* method);

    // TypeTrusted means the type of gate is already PrimitiveTypeCheck-passed, or the gate is constant and no need to check.
    bool IsTrustedType(GateRef gate) const;
    bool NeedInt32OverflowCheck(TypedUnOp op) const;

    template<TypedBinOp Op>
    void SpeculateNumbers(GateRef gate);
    template<TypedUnOp Op>
    void SpeculateNumber(GateRef gate);
    void SpeculateConditionJump(GateRef gate);
    void SpeculateCallBuiltin(GateRef gate, BuiltinsStubCSigns::ID Op);
    BuiltinsStubCSigns::ID GetBuiltinId(GateRef func, GateRef receiver);

    void AddProfiling(GateRef gate);
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_ {Gate::InvalidGateRef};
    TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
    bool profiling_ {false};
    std::string methodName_;
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H
