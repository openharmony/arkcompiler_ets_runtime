/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_PROXY_STUB_BUILDER_H
#define ECMASCRIPT_COMPILER_BUILTINS_PROXY_STUB_BUILDER_H
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/js_proxy.h"

namespace panda::ecmascript::kungfu {
class BuiltinsProxyStubBuilder : public BuiltinsStubBuilder {
public:
    explicit BuiltinsProxyStubBuilder(StubBuilder *parent)
        : BuiltinsStubBuilder(parent) {}
    BuiltinsProxyStubBuilder(StubBuilder *parent, GateRef glue)
        : BuiltinsStubBuilder(parent), glue_(glue) {}
    BuiltinsProxyStubBuilder(BuiltinsStubBuilder *parent, GateRef glue, GateRef thisValue, GateRef numArgs)
        : BuiltinsStubBuilder(parent), glue_(glue), thisValue_(thisValue), numArgs_(numArgs) {}
    ~BuiltinsProxyStubBuilder() override = default;
    NO_MOVE_SEMANTIC(BuiltinsProxyStubBuilder);
    NO_COPY_SEMANTIC(BuiltinsProxyStubBuilder);
    void GenerateCircuit() override {}
    void GenProxyConstructor(GateRef nativeCode, GateRef func, GateRef newTarget);
    GateRef GetProperty(GateRef proxy, GateRef key, GateRef receiver);
    GateRef SetProperty(GateRef proxy, GateRef key, GateRef value, GateRef receiver);

    void SetMethod(GateRef glue, GateRef proxy, GateRef method)
    {
        GateRef offset = IntPtr(JSProxy::METHOD_OFFSET);
        Store(VariableType::JS_ANY(), glue, proxy, offset, method);
    }

    void SetTarget(GateRef glue, GateRef proxy, GateRef target)
    {
        GateRef offset = IntPtr(JSProxy::TARGET_OFFSET);
        Store(VariableType::JS_ANY(), glue, proxy, offset, target);
    }

    GateRef GetHandler(GateRef proxy)
    {
        GateRef offset = IntPtr(JSProxy::HANDLER_OFFSET);
        return Load(VariableType::JS_ANY(), proxy, offset);
    }

    void SetHandler(GateRef glue, GateRef proxy, GateRef handler)
    {
        GateRef offset = IntPtr(JSProxy::HANDLER_OFFSET);
        Store(VariableType::JS_ANY(), glue, proxy, offset, handler);
    }

    void SetPrivateField(GateRef glue, GateRef proxy, GateRef privateField)
    {
        GateRef offset = IntPtr(JSProxy::PRIVATE_FIELD_OFFSET);
        Store(VariableType::JS_ANY(), glue, proxy, offset, privateField);
    }

    void SetIsRevoked(GateRef glue, GateRef proxy, GateRef value)
    {
        GateRef oldValue = ZExtInt1ToInt32(value);
        GateRef offset = IntPtr(JSProxy::BIT_FIELD_OFFSET);
        GateRef bitfield = Load(VariableType::INT32(), proxy, offset);
        GateRef mask = Int32LSL(
            Int32((1LU << JSProxy::IsRevokedBits::SIZE) - 1),
            Int32(JSProxy::IsRevokedBits::START_BIT));
        GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
            Int32LSL(oldValue, Int32(JSProxy::IsRevokedBits::START_BIT)));
        Store(VariableType::INT32(), glue, proxy, offset, newVal);
    }

private:
    GateRef glue_ { Circuit::NullGate() };
    GateRef thisValue_ { Circuit::NullGate() };
    GateRef numArgs_ { Circuit::NullGate() };
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTINS_PROXY_STUB_BUILDER_H
