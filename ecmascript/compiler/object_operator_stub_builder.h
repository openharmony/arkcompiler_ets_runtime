/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_OBJECT_OPERATOR_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_OBJECT_OPERATOR_STUB_BUILDER_H

#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/compiler/circuit_builder_helper.h"

namespace panda::ecmascript::kungfu {
class ObjectOperatorStubBuilder : public StubBuilder {
public:
    ObjectOperatorStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : StubBuilder(parent, globalEnv) {}
    ~ObjectOperatorStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ObjectOperatorStubBuilder);
    NO_COPY_SEMANTIC(ObjectOperatorStubBuilder);
    void GenerateCircuit() override {}

    enum class LookupKind {
        KIND_HAS_PROPERTY = 0,
        KIND_GET_PROPERTY,
        KIND_SET_PROPERTY,
    };

    enum class StartLookupType {
        HAS_RECEIVER = 0,
        NO_RECEIVER,
    };

    struct ObjectOperatorOptions {
        LookupKind lookupKind = LookupKind::KIND_HAS_PROPERTY;
        OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    };

    struct ObjectOperatorResult {
        Variable *metaData { nullptr };     // INT_32
        Variable *elemKey { nullptr };      // INT_32
        Variable *propKey { nullptr };      // JS_ANY
        Variable *index { nullptr };        // INT_32
        Variable *value { nullptr };        // JS_ANY
        Variable *holder { nullptr };       // JS_ANY
        Variable *receiver { nullptr };     // JS_ANY
        Variable *attr { nullptr };         // INT_64

        GateRef GetHolder() const
        {
            return holder->ReadVariable();
        }

        GateRef GetReceiver() const
        {
            return receiver->ReadVariable();
        }

        GateRef GetMetaData() const
        {
            return metaData->ReadVariable();
        }

        GateRef GetPropKey() const
        {
            return propKey->ReadVariable();
        }

        GateRef GetElemKey() const
        {
            return elemKey->ReadVariable();
        }

        GateRef GetIndex() const
        {
            return index->ReadVariable();
        }

        GateRef GetValue() const
        {
            return value->ReadVariable();
        }

        GateRef GetAttr() const
        {
            return attr->ReadVariable();
        }
    };

    void TryFastHandleStringKey(GateRef key, Variable *propKey, Variable *elemKey,
                                Label *isProperty, Label *isElement, Label *tryFailed);
    void HandleKey(GateRef glue, GateRef key, Variable *propKey,
                   Variable *elemKey, Label *isProperty, Label *isElement,
                   Label *hasException, GateRef hir = Circuit::NullGate());
    template <bool keyIsElement>
    void UpdateHolder(GateRef glue, ObjectOperatorResult &results, GateRef key,
                      Label *holderUpdated);
    template <StartLookupType startLookupType>
    void StartLookup(GateRef glue, GateRef key, Label *exit,
                     const ObjectOperatorOptions &options,
                     ObjectOperatorResult &results,
                     GateRef hir = Circuit::NullGate());

    void InitializeOperatorResults(ObjectOperatorResult &result);
    void FinalizeOperatorResults(ObjectOperatorResult &result);

    GateRef IsFound(Variable *metaData);
    GateRef IsElement(ObjectOperatorResult &opResult);
    GateRef IsAccessorDescriptor(ObjectOperatorResult &opResult);

private:
    static constexpr int32_t MAX_META_DATA = 0x7FFFFFFF;
    static constexpr int32_t IS_FOUND_BIT = 0x1;
    static constexpr int32_t IS_ON_PROTOTYPE_BIT = 0x2;
    static constexpr int32_t HAS_RECEIVER_BIT = 0x3;

    void SetFound(ObjectOperatorResult &opResult);
    void SetFound(ObjectOperatorResult &opResult, GateRef value, GateRef index, GateRef attr);
    void SetIsOnProtoType(ObjectOperatorResult &opResult);
    void SetHasReceiver(ObjectOperatorResult &opResult);

    template <bool keyIsElement>
    void CheckValidIndexOrKeyIsLength(GateRef glue, GateRef key, GateRef holder, GateRef receiver,
                                      Label *checkSucc, Label *checkFail);

    template <bool keyIsElement>
    void LookupProperty(GateRef glue, GateRef key, Label *exit,
                        const ObjectOperatorOptions &options, ObjectOperatorResult &results,
                        GateRef hir = Circuit::NullGate());

    template <bool keyIsElement>
    void TryLookupInProtoChain(GateRef glue, GateRef key, Label *exit,
                               const ObjectOperatorOptions &options, ObjectOperatorResult &results,
                               GateRef hir = Circuit::NullGate());
    void LookupPropertyInlinedProps(GateRef glue, GateRef obj, GateRef key, Label *exit,
                                    const ObjectOperatorOptions &options, ObjectOperatorResult &results,
                                    GateRef hir = Circuit::NullGate());
    void LookupElementInlinedProps(GateRef glue, GateRef obj, GateRef elementIdx, Label *exit,
                                   const ObjectOperatorOptions &options, ObjectOperatorResult &results,
                                   GateRef hir = Circuit::NullGate());
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_OBJECT_OPERATOR_STUB_BUILDER_H
