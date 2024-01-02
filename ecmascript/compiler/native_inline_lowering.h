/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTIN_INLINE_H
#define ECMASCRIPT_COMPILER_BUILTIN_INLINE_H

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/type_info_accessors.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class NativeInlineLowering {
public:
    explicit NativeInlineLowering(Circuit *circuit, PassContext *ctx, bool enableLog, const std::string& name)
        : circuit_(circuit),
          builder_(circuit),
          acc_(circuit),
          glue_(acc_.GetGlueFromArgList()),
          tsManager_(ctx->GetTSManager()),
          enableLog_(enableLog),
          methodName_(name),
          nocheck_(ctx->GetEcmaVM()->GetJSOptions().IsCompilerNoCheck()),
          thread_(ctx->GetEcmaVM()->GetJSThread()) {}
    ~NativeInlineLowering() = default;
    void RunNativeInlineLowering();

private:
    void RunArrayForeachInline(GateRef gate);
    void ArrayForeachCall(GateRef gate, GateRef thisObj, GateRef callBack, GateRef thisArg, Label *exit);
    void TryInlineNativeCallThis1(GateRef gate);
    void TryInlineBuiltinsArrayFunc(GateRef gate);
    bool IsCreateArray(GateRef gate);
    GateRef LoadArrayElement(ElementsKind kind, GateRef gate, GateRef index);
    void ReplaceHirDirectly(GateRef gate, GateRef value);
    GateRef LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args, bool useLabel);
    GateRef NativeCallTS(GateRef gate, GateRef depend, const std::vector<GateRef> &args);
    void TryInlineStringFromCharCode(GateRef gate, CallThis1TypeInfoAccessor &tacc);
    bool EnableLog() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    bool Uncheck() const
    {
        return nocheck_;
    }

    Circuit *circuit_ {nullptr};
    CircuitBuilder builder_;
    GateAccessor acc_;
    GateRef glue_;
    TSManager *tsManager_;
    bool enableLog_;
    std::string methodName_;
    bool nocheck_;
    const JSThread *thread_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_BUILTIN_INLINE_H