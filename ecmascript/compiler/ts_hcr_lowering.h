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

#ifndef ECMASCRIPT_COMPILER_TS_HCR_LOWERING_H
#define ECMASCRIPT_COMPILER_TS_HCR_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/object_access_helper.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {
class TSHCRLowering {
public:
    TSHCRLowering(Circuit *circuit, PassContext *ctx,
                   bool enableLog, bool enableTypeLog,
                   const std::string& name)
        : circuit_(circuit),
          acc_(circuit),
          builder_(circuit, ctx->GetCompilerConfig()),
          dependEntry_(circuit->GetDependRoot()),
          tsManager_(ctx->GetTSManager()),
          enableLog_(enableLog),
          enableTypeLog_(enableTypeLog),
          profiling_(ctx->GetCompilerConfig()->IsProfiling()),
          verifyVTable_(ctx->GetCompilerConfig()->IsVerifyVTbale()),
          traceBc_(ctx->GetCompilerConfig()->IsTraceBC()),
          methodName_(name),
          glue_(acc_.GetGlueFromArgList()),
          argAcc_(circuit),
          pgoTypeLog_(circuit),
          noCheck_(ctx->GetEcmaVM()->GetJSOptions().IsCompilerNoCheck()),
          thread_(ctx->GetEcmaVM()->GetJSThread()) {}

    ~TSHCRLowering() = default;

    bool RunTSHCRLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsTypeLogEnabled() const
    {
        return enableTypeLog_;
    }

    bool IsProfiling() const
    {
        return profiling_;
    }

    bool IsVerifyVTbale() const
    {
        return verifyVTable_;
    }

    bool IsTraceBC() const
    {
        return traceBc_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void Lower(GateRef gate);
    template<TypedBinOp Op>
    void LowerTypedBinOp(GateRef gate);
    void LowerTypedMod(GateRef gate);
    void LowerTypedDiv(GateRef gate);
    void LowerTypedStrictEq(GateRef gate);
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
    void LowerConditionJump(GateRef gate, bool flag);
    void LowerTypedNeg(GateRef gate);
    void LowerTypedNot(GateRef gate);

    void LowerTypedLdObjByName(GateRef gate);
    void LowerTypedStObjByName(GateRef gate, bool isThis);
    using AccessMode = ObjectAccessHelper::AccessMode;
    void LowerNamedAccess(GateRef gate, GateRef receiver, AccessMode accessMode, JSTaggedValue key, GateRef value);
    GateRef BuildNamedPropertyAccess(GateRef hir, ObjectAccessHelper accessHelper, PropertyLookupResult plr);
    void BuildNamedPropertyAccessVerifier(GateRef gate, GateRef receiver, AccessMode mode, GateRef value);
    bool TryLowerTypedLdObjByNameForBuiltin(GateRef gate, GateType receiverType, JSTaggedValue key);
    bool TryLowerTypedLdObjByNameForBuiltinMethod(GateRef gate, GateType receiverType, JSTaggedValue key);
    void LowerTypedLdArrayLength(GateRef gate);
    void LowerTypedLdTypedArrayLength(GateRef gate);
    void LowerTypedLdStringLength(GateRef gate);

    void LowerTypedLdObjByIndex(GateRef gate);
    void LowerTypedStObjByIndex(GateRef gate);
    void LowerTypedLdObjByValue(GateRef gate, bool isThis);
    void LowerTypedStObjByValue(GateRef gate);
    void LowerTypedIsTrueOrFalse(GateRef gate, bool flag);
    void LowerTypedNewObjRange(GateRef gate);
    void LowerTypedSuperCall(GateRef gate);

    void LowerTypedCallArg0(GateRef gate);
    void LowerTypedCallArg1(GateRef gate);
    void LowerTypedCallArg2(GateRef gate);
    void LowerTypedCallArg3(GateRef gate);
    void LowerTypedCallrange(GateRef gate);
    void LowerTypedCallthis0(GateRef gate);
    void LowerTypedCallthis1(GateRef gate);
    void LowerTypedCallthis2(GateRef gate);
    void LowerTypedCallthis3(GateRef gate);
    void LowerTypedCallthisrange(GateRef gate);
    void LowerTypedCall(GateRef gate, GateRef func, GateRef actualArgc, GateType funcType, uint32_t argc);
    void LowerTypedThisCall(GateRef gate, GateRef func, GateRef actualArgc, uint32_t argc);
    bool IsLoadVtable(GateRef func);
    bool CanOptimizeAsFastCall(GateRef func);
    void CheckCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
        GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall);
    void CheckCallTargetFromDefineFuncAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
        GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall, bool isNoGC);
    void CheckThisCallTargetAndLowerCall(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
        GateType funcType, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall);
    void CheckCallThisCallTarget(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
                                 GateType funcType, bool isNoGC);
    void CheckFastCallThisCallTarget(GateRef gate, GateRef func, GlobalTSTypeRef funcGt,
                                     GateType funcType, bool isNoGC);
    void LowerFastCall(GateRef gate, GateRef func, const std::vector<GateRef> &argsFastCall, bool isNoGC);
    void LowerCall(GateRef gate, GateRef func, const std::vector<GateRef> &args, bool isNoGC);
    GateRef LoadStringByIndex(GateRef receiver, GateRef propKey);
    GateRef LoadJSArrayByIndex(GateRef receiver, GateRef propKey, ElementsKind kind);
    GateRef LoadTypedArrayByIndex(GateRef receiver, GateRef propKey);
    void StoreJSArrayByIndex(GateRef receiver, GateRef propKey, GateRef value, ElementsKind kind);
    void StoreTypedArrayByIndex(GateRef receiver, GateRef propKey, GateRef value);
    bool IsCreateArray(GateRef receiver);

    // TypeTrusted means the type of gate is already PrimitiveTypeCheck-passed,
    // or the gate is constant and no need to check.
    bool IsTrustedType(GateRef gate) const;
    bool HasNumberType(GateRef gate, GateRef value) const;
    bool HasNumberType(GateRef gate, GateRef left, GateRef right) const;

    void AddBytecodeCount(EcmaOpcode op);
    void DeleteBytecodeCount(EcmaOpcode op);
    void AddHitBytecodeCount();

    template<TypedBinOp Op>
    void SpeculateNumbers(GateRef gate);
    template<TypedUnOp Op>
    void SpeculateNumber(GateRef gate);
    void SpeculateConditionJump(GateRef gate, bool flag);
    void SpeculateCallBuiltin(GateRef gate, GateRef func, const std::vector<GateRef> &args,
                              BuiltinsStubCSigns::ID id, bool isThrow);
    BuiltinsStubCSigns::ID GetBuiltinId(BuiltinTypeId id, GateRef func);
    void DeleteConstDataIfNoUser(GateRef gate);

    void AddProfiling(GateRef gate);

    bool Uncheck() const
    {
        return noCheck_;
    }

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_ {Gate::InvalidGateRef};
    TSManager *tsManager_ {nullptr};
    bool enableLog_ {false};
    bool enableTypeLog_ {false};
    bool profiling_ {false};
    bool verifyVTable_ {false};
    bool traceBc_ {false};
    size_t allJSBcCount_ {0};
    size_t allNonTypedOpCount_ {0};
    size_t hitTypedOpCount_ {0};
    std::string methodName_;
    GateRef glue_ {Circuit::NullGate()};
    ArgumentAccessor argAcc_;
    EcmaOpcode currentOp_ {static_cast<EcmaOpcode>(0xff)};
    PGOTypeLogList pgoTypeLog_;
    std::unordered_map<EcmaOpcode, uint32_t> bytecodeMap_;
    std::unordered_map<EcmaOpcode, uint32_t> bytecodeHitTimeMap_;
    bool noCheck_ {false};
    const JSThread *thread_ {nullptr};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_HCR_LOWERING_H
