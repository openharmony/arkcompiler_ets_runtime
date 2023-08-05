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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_METHOD_TYPE_INFER_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_METHOD_TYPE_INFER_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
enum InferState : uint8_t {
    // loop-begin gate has been infered to non-any type
    NORMAL_INFERED = 0,
    // loop-begin gate has been fixed to any type manually and it should not be infered to other types
    ANY_INFERED,
    // the number-types of loop-begin gate under this state should be promoted to number type
    NUMBER_INFERED,
};

class MethodTypeInfer {
public:
    MethodTypeInfer(BytecodeCircuitBuilder *builder, Circuit *circuit,
                    PassContext *ctx, size_t methodId, bool enableLog,
                    const std::string &name, const CString &recordName,
                    MethodInfo *methodInfo, const MethodLiteral *methodLiteral,
                    bool enableGlobalTypeInfer);
    ~MethodTypeInfer() = default;

    NO_COPY_SEMANTIC(MethodTypeInfer);
    NO_MOVE_SEMANTIC(MethodTypeInfer);

    std::pair<GateType, uint32_t> TraverseInfer();
    void CheckAndPrint();
    void SetNamespaceArgType(GateType type);

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

private:
    static constexpr int PERCENT_LENS = 2;
    static constexpr int HUNDRED_TIME = 100;

    const std::string &GetMethodName() const
    {
        return methodName_;
    }

    // savePreType: save the previous type, which is true by default
    bool UpdateType(GateRef gate, const GateType type, bool savePreType = true);
    bool UpdateType(GateRef gate, const GlobalTSTypeRef &typeRef, bool savePreType = true);
    GateType HandleTypeCompatibility(const GateType preType, const GateType type) const;
    bool ShouldInfer(const GateRef gate) const;
    bool Infer(GateRef gate);
    bool InferPhiGate(GateRef gate);
    bool SetIntType(GateRef gate);
    bool SetNumberType(GateRef gate);
    bool SetBigIntType(GateRef gate);
    bool SetBooleanType(GateRef gate);
    bool InferLdUndefined(GateRef gate);
    bool InferLdNull(GateRef gate);
    bool InferLdai(GateRef gate);
    bool InferFLdai(GateRef gate);
    bool InferLdSymbol(GateRef gate);
    bool InferThrow(GateRef gate);
    bool InferTypeOf(GateRef gate);
    bool InferAdd2(GateRef gate);
    bool InferSub2(GateRef gate);
    bool InferMul2(GateRef gate);
    bool InferDiv2(GateRef gate);
    bool InferIncDec(GateRef gate);
    bool InferToNumberic(GateRef gate);
    bool InferLdObjByIndex(GateRef gate);
    bool InferLdGlobalVar(GateRef gate);
    bool InferReturnUndefined(GateRef gate);
    bool InferReturn(GateRef gate);
    bool InferLdObjByName(GateRef gate);
    bool InferNewObject(GateRef gate);
    bool SetStGlobalBcType(GateRef gate, bool hasIC = false);
    bool InferLdStr(GateRef gate);
    bool InferCallFunction(GateRef gate);
    bool InferCallMethod(GateRef gate);
    bool InferLdObjByValue(GateRef gate);
    bool InferGetNextPropName(GateRef gate);
    bool InferDefineGetterSetterByValue(GateRef gate);
    bool InferSuperCall(GateRef gate);
    bool InferSuperPropertyByName(GateRef gate);
    bool InferSuperPropertyByValue(GateRef gate);
    bool InferTryLdGlobalByName(GateRef gate);
    bool InferLdLexVarDyn(GateRef gate);
    bool InferStLexVarDyn(GateRef gate);
    bool InferStModuleVar(GateRef gate);
    bool InferLdLocalModuleVar(GateRef gate);
    bool InferLdExternalModuleVar(GateRef gate);
    bool InferStObjByName(GateRef gate);
    bool IsNewLexEnv(EcmaOpcode opcode) const;
    bool InferGetIterator(GateRef gate);
    bool InferLoopBeginPhiGate(GateRef gate);
    bool GetObjPropWithName(GateRef gate, GateType objType, uint64_t index);
    bool GetSuperProp(GateRef gate, uint64_t index, bool isString = true);
    GlobalTSTypeRef ConvertPrimitiveToBuiltin(const GateType &gateType);
    GlobalTSTypeRef GetPropType(const GateType type, const JSTaggedValue propertyName) const;
    GlobalTSTypeRef GetPropType(const GateType type, const uint64_t key) const;
    void UpdateQueueForLoopPhi();

    inline bool ShouldInferWithLdObjByValue(const GateType &type) const
    {
        auto flag = tsManager_->IsObjectTypeKind(type) ||
                    tsManager_->IsClassTypeKind(type) ||
                    tsManager_->IsClassInstanceTypeKind(type);
        return flag;
    }

    inline bool ShouldInferWithLdObjByName(const GateType &type) const
    {
        return ShouldInferWithLdObjByValue(type) || tsManager_->IsIteratorInstanceTypeKind(type) ||
            tsManager_->IsInterfaceTypeKind(type) || tsManager_->IsNamespaceTypeKind(type);
    }

    inline bool ShouldConvertToBuiltinArray(const GateType &type) const
    {
        return tsManager_->IsArrayTypeKind(type) && tsManager_->IsBuiltinsDTSEnabled();
    }

    void PrintTypeAnnotation() const;
    void PrintByteCodesWithTypes() const;
    void PrintCircuitWithTypes() const;
    void Verify() const;
    void VerifyTypePercent();
    void TypeCheck(GateRef gate) const;

    std::string CollectGateTypeLogInfo(GateRef gate, DebugInfoExtractor *debugExtractor,
                                       const std::string &logPreFix) const;

    const BytecodeInfo &GetByteCodeInfo(const GateRef gate) const
    {
        const auto bcIndex = jsgateToBytecode_.at(gate);
        return builder_->GetBytecodeInfo(bcIndex);
    }

    std::pair<GateType, uint32_t> SetAndReturnNamespaceObjType(GateRef gate);
    GlobalTSTypeRef TryGetNamespaceType(GateRef gate) const;
    bool IsNamespace(GateRef gate) const;
    bool CheckNamespaceFunc(GateRef func) const;

    bool IsByteCodeGate(const GateRef gate) const
    {
        return jsgateToBytecode_.find(gate) != jsgateToBytecode_.end();
    }

    BytecodeCircuitBuilder *builder_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor gateAccessor_;
    TSManager *tsManager_ {nullptr};
    PassContext *ctx_ {nullptr};
    LexEnvManager *lexEnvManager_ {nullptr};
    size_t methodId_ {0};
    bool enableLog_ {false};
    std::string methodName_;
    std::map<uint16_t, GateType> stringIdToGateType_;
    std::unordered_map<GateRef, uint32_t> jsgateToBytecode_ {};
    std::map<GateRef, InferState> loopPhiState_ {};
    const CString &recordName_;
    size_t shouldInferNum_ {0};
    size_t normalInferNum_ {0};
    MethodInfo *methodInfo_ {nullptr};
    const MethodLiteral *methodLiteral_ {nullptr};
    std::vector<bool> inQueue_;
    std::unordered_set<GateRef> needInferGates_ {};
    std::queue<GateRef> pendingQueue_ {};
    bool needUpdateForLoopPhi_ {true};
    bool enableGlobalTypeInfer_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_METHOD_TYPE_INFER_H
