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
#ifndef ECMASCRIPT_COMPILER_IC_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_IC_STUB_BUILDER_H
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {
enum class ICStubType { LOAD, STORE };
class ICStubBuilder : public StubBuilder {
public:
    ICStubBuilder(StubBuilder *parent, GateRef globalEnv)
        : StubBuilder(parent, globalEnv) {}
    ~ICStubBuilder() override = default;
    NO_MOVE_SEMANTIC(ICStubBuilder);
    NO_COPY_SEMANTIC(ICStubBuilder);
    void GenerateCircuit() override {}

    void SetParameters(GateRef glue, GateRef receiver, GateRef profileTypeInfo, GateRef value, GateRef slotId,
                       ProfileOperation callback = ProfileOperation())
    {
        glue_ = glue;
        receiver_ = receiver;
        profileTypeInfo_ = profileTypeInfo;
        value_ = value;
        slotId_ = slotId;
        callback_ = callback;
    }

    void SetParameters(GateRef glue, GateRef receiver, GateRef profileTypeInfo, GateRef value, GateRef slotId,
                       GateRef propKey, ProfileOperation callback = ProfileOperation())
    {
        SetParameters(glue, receiver, profileTypeInfo, value, slotId, callback);
        propKey_ = propKey;
    }

    void SetParameters(GateRef glue, GateRef receiver, GateRef profileTypeInfo, GateRef value, GateRef slotId,
                       GateRef megaStubCache, GateRef propKey, ProfileOperation callback = ProfileOperation())
    {
        SetParameters(glue, receiver, profileTypeInfo, value, slotId, propKey_, callback);
        propKey_ = propKey;
        megaStubCache_ = megaStubCache;
    }

    void LoadICByName(Variable* result, Label* tryFastPath, Label *slowPath, Label *success,
        ProfileOperation callback);
    void LoadICByNameWithMega(Variable *result, Label *tryFastPath, Label *slowPath, Label *success,
                              ProfileOperation callback);
    void StoreICByName(Variable *result, Label *tryFastPath, Label *slowPath, Label *success);
    void StoreICByNameWithMega(Variable *result, Label *tryFastPath, Label *slowPath, Label *success);
    void LoadICByValue(Variable* result, Label* tryFastPath, Label *slowPath, Label *success,
        ProfileOperation callback);
    void StoreICByValue(Variable* result, Label* tryFastPath, Label *slowPath, Label *success);
    void TryLoadGlobalICByName(Variable* result, Label* tryFastPath, Label *slowPath, Label *success);
    void TryStoreGlobalICByName(Variable* result, Label* tryFastPath, Label *slowPath, Label *success);
private:
    struct PrimitiveLoadICInfo {
        GateRef glue;
        GateRef glueGlobalEnv;
        GateRef profileFirstValue;
        Label *tryICHandler;
        Variable *cachedHandler;
    };
    void TryDesignatePrimitiveLoadIC(Label &tryDesignatePrimitive, Label &notDesignatePrimitive,
                                     PrimitiveType primitiveType, PrimitiveLoadICInfo info);
    void TryPrimitiveLoadIC(Variable* cachedHandler, Label *tryICHandler);
    template<ICStubType type>
    void NamedICAccessor(Variable* cachedHandler, Label *tryICHandler);
    template<ICStubType type>
    void NamedICAccessorWithMega(Variable* cachedHandler, Label *tryICHandler);
    void ValuedICAccessor(Variable* cachedHandler, Label *tryICHandler, Label* tryElementIC);
    void SetLabels(Label* tryFastPath, Label *slowPath, Label *success)
    {
        tryFastPath_ = tryFastPath;
        slowPath_ = slowPath;
        success_ = success;
    }

    GateRef glue_ {Circuit::NullGate()};
    GateRef receiver_ {0};
    GateRef profileTypeInfo_ {0};
    GateRef value_ {0};
    GateRef slotId_ {0};
    GateRef propKey_ {0};
    GateRef megaStubCache_ {0};
    ProfileOperation callback_;

    Label *tryFastPath_ {nullptr};
    Label *slowPath_ {nullptr};
    Label *success_ {nullptr};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_IC_STUB_BUILDER_H
