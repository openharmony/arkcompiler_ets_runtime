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
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
class TSTypeLowering {
public:
    TSTypeLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg,
                   TSManager *tsManager, bool enableLog, const std::string& name,
                   const JSHandle<JSTaggedValue> &constantPool)
        : bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY))), tsManager_(tsManager),
          enableLog_(enableLog), methodName_(name), constantPool_(constantPool) {}

    ~TSTypeLowering() = default;

    void RunTSTypeLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
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
    void LowerTypedInc(GateRef gate);
    void LowerTypedDec(GateRef gate);
    void LowerTypeToNumeric(GateRef gate);
    void LowerPrimitiveTypeToNumber(GateRef gate);
    void LowerConditionJump(GateRef gate);
    void LowerTypedNeg(GateRef gate);
    void LowerTypedNot(GateRef gate);
    void LowerTypedLdObjByName(GateRef gate, GateRef thisObj, bool isThis = false);
    void LowerTypedStObjByName(GateRef gate, GateRef thisObj, bool isThis = false);
    void LowerTypedLdObjByIndex(GateRef gate);
    void LowerTypedStObjByIndex(GateRef gate);

    // TypeTrusted means the type of gate is already typecheck-passed, or the gate is constant and no need to check.
    bool IsTrustedType(GateRef gate) const;

    template<TypedBinOp Op>
    void SpeculateNumbers(GateRef gate);
    template<TypedUnOp Op>
    void SpeculateNumber(GateRef gate);
    void SpeculateConditionJump(GateRef gate);
    template<TypedUnOp Op>
    GateRef AppendOverflowCheck(GateRef typeCheck, GateRef intVal);
    BytecodeCircuitBuilder *bcBuilder_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_ {Gate::InvalidGateRef};
    TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
    std::string methodName_;
    JSHandle<ConstantPool> constantPool_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_TYPE_LOWERING_H
