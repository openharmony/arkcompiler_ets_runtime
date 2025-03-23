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

namespace panda::ecmascript::kungfu {

class ObjectOperatorStubBuilder : public StubBuilder {
public:
    explicit ObjectOperatorStubBuilder(StubBuilder *parent)
        : StubBuilder(parent) {}
    explicit ObjectOperatorStubBuilder(Environment *env)
        : StubBuilder(env) {}
    ~ObjectOperatorStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ObjectOperatorStubBuilder);
    NO_COPY_SEMANTIC(ObjectOperatorStubBuilder);
    void GenerateCircuit() override {}

    void TryFastHandleStringKey(GateRef key, Variable *propKey, Variable *elemKey,
                                Label *isProperty, Label *isElement, Label *tryFailed);
    void HandleKey(GateRef glue, GateRef key, Variable *propKey, Variable *elemKey, Label *isProperty, Label *isElement,
                   Label *hasException, GateRef hir = Circuit::NullGate());
    template <bool keyIsElement>
    void CheckValidIndexOrKeyIsLength(GateRef glue, GateRef key, GateRef obj, Label *checkSucc, Label *checkFail);
    template <bool keyIsElement>
    void UpdateHolder(GateRef glue, Variable *holder, GateRef key, Label *holderUpdated);
    template <bool keyIsElement>
    void LookupProperty(GateRef glue, Variable *holder, GateRef key, Label *isJSProxy, Label *ifFound, Label *notFound,
                        GateRef hir = Circuit::NullGate());
    template <bool keyIsElement>
    void TryLookupInProtoChain(GateRef glue, Variable *holder, GateRef key, Label *ifFound, Label *notFound,
                               Label *isJSProxy, GateRef hir = Circuit::NullGate());
    GateRef LookupPropertyInlinedProps(GateRef glue, GateRef obj, GateRef key, GateRef hir = Circuit::NullGate());
    GateRef LookupElementInlinedProps(GateRef glue, GateRef obj, GateRef key, GateRef hir = Circuit::NullGate());
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_OBJECT_OPERATOR_STUB_BUILDER_H
