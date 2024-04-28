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
#ifndef ECMASCRIPT_COMPILER_ACCESS_OBJECT_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_ACCESS_OBJECT_STUB_BUILDER_H

#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/profiler_operation.h"
#include "ecmascript/compiler/stub_builder.h"

namespace panda::ecmascript::kungfu {
class AccessObjectStubBuilder : public StubBuilder {
public:
    explicit AccessObjectStubBuilder(StubBuilder *parent) : StubBuilder(parent)
    {
        jsFunc_ = Circuit::NullGate();
    }
    explicit AccessObjectStubBuilder(StubBuilder *parent, GateRef jsFunc)
        : StubBuilder(parent), jsFunc_(jsFunc) {}
    ~AccessObjectStubBuilder() override = default;
    NO_MOVE_SEMANTIC(AccessObjectStubBuilder);
    NO_COPY_SEMANTIC(AccessObjectStubBuilder);
    void GenerateCircuit() override {}

    GateRef LoadObjByName(GateRef glue, GateRef receiver, GateRef prop, const StringIdInfo &info,
                          GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback = ProfileOperation());
    GateRef DeprecatedLoadObjByName(GateRef glue, GateRef receiver, GateRef propKey);
    GateRef StoreObjByName(GateRef glue, GateRef receiver, GateRef prop, const StringIdInfo &info, GateRef value,
        GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback = ProfileOperation());
    GateRef LoadPrivatePropertyByName(GateRef glue,
                                      GateRef receiver,
                                      GateRef key,
                                      GateRef profileTypeInfo,
                                      GateRef slotId,
                                      ProfileOperation callback);
    GateRef StorePrivatePropertyByName(GateRef glue,
                                       GateRef receiver,
                                       GateRef key,
                                       GateRef value,
                                       GateRef profileTypeInfo,
                                       GateRef slotId,
                                       ProfileOperation callback = ProfileOperation());
    GateRef LoadObjByValue(GateRef glue, GateRef receiver, GateRef key, GateRef profileTypeInfo, GateRef slotId,
        ProfileOperation callback = ProfileOperation());
    GateRef StoreObjByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef profileTypeInfo,
                            GateRef slotId, ProfileOperation callback = ProfileOperation());
    GateRef StoreOwnByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value, GateRef profileTypeInfo,
                            GateRef slotId, ProfileOperation callback = ProfileOperation());
    GateRef DeprecatedLoadObjByValue(GateRef glue, GateRef receiver, GateRef key);
    GateRef TryLoadGlobalByName(GateRef glue, GateRef prop, const StringIdInfo &info,
                                GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback);
    GateRef TryStoreGlobalByName(GateRef glue, GateRef prop, const StringIdInfo &info,
                                 GateRef value, GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback);
    GateRef LoadGlobalVar(GateRef glue, GateRef prop, const StringIdInfo &info,
                          GateRef profileTypeInfo, GateRef slotId, ProfileOperation callback);
    GateRef StoreGlobalVar(GateRef glue, GateRef prop, const StringIdInfo &info,
                           GateRef value, GateRef profileTypeInfo, GateRef slotId);
    GateRef StOwnByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value);
    GateRef StOwnByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value);
    GateRef StOwnByName(GateRef glue, GateRef receiver, GateRef key, GateRef value);
    GateRef StOwnByValueWithNameSet(GateRef glue, GateRef receiver, GateRef key, GateRef value);
    GateRef StOwnByNameWithNameSet(GateRef glue, GateRef receiver, GateRef key, GateRef value);
    GateRef StObjByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value);
    GateRef LdObjByIndex(GateRef glue, GateRef receiver, GateRef index);

private:
    GateRef ResolvePropKey(GateRef glue, GateRef prop, const StringIdInfo &info);
    GateRef jsFunc_ { Circuit::NullGate() };
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_ACCESS_OBJECT_STUB_BUILDER_H
