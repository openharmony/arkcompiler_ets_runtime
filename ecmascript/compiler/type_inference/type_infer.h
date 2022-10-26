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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_TYPE_INFER_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_TYPE_INFER_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class TypeInfer {
public:
    TypeInfer(BytecodeCircuitBuilder *builder, Circuit *circuit,
              const JSHandle<JSTaggedValue> &constantPool, TSManager *tsManager,
              LexEnvManager *lexEnvManager, size_t methodId, bool enableLog,
              const std::string& name)
        : builder_(builder), circuit_(circuit), constantPool_(constantPool), gateAccessor_(circuit),
          tsManager_(tsManager), lexEnvManager_(lexEnvManager), methodId_(methodId), enableLog_(enableLog),
          methodName_(name)
    {
    }

    ~TypeInfer() = default;

    NO_COPY_SEMANTIC(TypeInfer);
    NO_MOVE_SEMANTIC(TypeInfer);

    void TraverseCircuit();

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

private:
    bool UpdateType(GateRef gate, const GateType type);
    bool UpdateType(GateRef gate, const GlobalTSTypeRef &typeRef);
    bool ShouldInfer(const GateRef gate) const;
    bool Infer(GateRef gate);
    bool InferPhiGate(GateRef gate);
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
    bool InferLdObjByIndex(GateRef gate);
    bool InferLdGlobalVar(GateRef gate);
    bool InferReturnUndefined(GateRef gate);
    bool InferReturn(GateRef gate);
    bool InferLdObjByName(GateRef gate);
    bool InferNewObject(GateRef gate);
    bool SetStGlobalBcType(GateRef gate, bool hasIC = false);
    bool InferLdStr(GateRef gate);
    bool InferCallFunction(GateRef gate, bool isDeprecated = false);
    bool InferLdObjByValue(GateRef gate);
    bool InferGetNextPropName(GateRef gate);
    bool InferDefineGetterSetterByValue(GateRef gate);
    bool InferSuperCall(GateRef gate);
    bool InferTryLdGlobalByName(GateRef gate);
    bool InferLdLexVarDyn(GateRef gate);
    bool InferStLexVarDyn(GateRef gate);
    bool IsNewLexEnv(EcmaOpcode opcode) const;
    bool InferGetIterator(GateRef gate);
    bool InferLoopBeginPhiGate(GateRef gate);

    inline GlobalTSTypeRef GetPropType(const GateType &type, const JSTaggedValue propertyName) const
    {
        return tsManager_->GetPropType(type, propertyName);
    }

    inline GlobalTSTypeRef GetPropType(const GateType &type, const uint64_t key) const
    {
        return tsManager_->GetPropType(type, key);
    }

    inline bool ShouldInferWithLdObjByValue(const GateType &type) const
    {
        auto flag = tsManager_->IsObjectTypeKind(type) ||
                    tsManager_->IsClassTypeKind(type) ||
                    tsManager_->IsClassInstanceTypeKind(type);
        return flag;
    }

    inline bool ShouldInferWithLdObjByName(const GateType &type) const
    {
        return ShouldInferWithLdObjByValue(type) || tsManager_->IsIteratorInstanceTypeKind(type);
    }

    void PrintAllByteCodesTypes() const;
    void Verify() const;
    void TypeCheck(GateRef gate) const;
    void FilterAnyTypeGates() const;

    std::string CollectGateTypeLogInfo(GateRef gate, DebugInfoExtractor *debugExtractor,
                                       const std::string &logPreFix) const;

    BytecodeCircuitBuilder *builder_ {nullptr};
    Circuit *circuit_ {nullptr};
    JSHandle<ConstantPool> constantPool_;
    GateAccessor gateAccessor_;
    TSManager *tsManager_ {nullptr};
    LexEnvManager *lexEnvManager_ {nullptr};
    size_t methodId_ {0};
    bool enableLog_ {false};
    std::string methodName_;
    std::map<uint16_t, GateType> stringIdToGateType_;
    std::map<GateRef, bool> phiState_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_TYPE_INFER_H
