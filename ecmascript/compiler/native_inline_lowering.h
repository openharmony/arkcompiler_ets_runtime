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

#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/type_info_accessors.h"
#include "ecmascript/js_dataview.h"

namespace panda::ecmascript::kungfu {
class NativeInlineLowering {
public:
    explicit NativeInlineLowering(Circuit *circuit, PassContext *ctx, bool enableLog, const std::string& name)
        : circuit_(circuit),
          builder_(circuit),
          acc_(circuit),
          glue_(acc_.GetGlueFromArgList()),
          enableLog_(enableLog),
          methodName_(name),
          nocheck_(ctx->GetCompilationEnv()->GetJSOptions().IsCompilerNoCheck()),
          traceInline_(ctx->GetCompilationEnv()->GetJSOptions().GetTraceInline()),
          compilationEnv_(ctx->GetCompilationEnv()) {}
    ~NativeInlineLowering() = default;
    void RunNativeInlineLowering();

private:
    std::optional<std::pair<size_t, bool>> GetCallInfo(GateRef gate);
    void TryInlineStringFromCharCode(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsFinite(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsInteger(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsNaN(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsSafeInteger(GateRef gate, size_t argc, bool skipThis);
    void TryInlineMathUnaryBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op,
                                   bool skipThis);
    void TryInlineMathBinaryBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op,
                                    bool skipThis);
    void TryInlineMathImulBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op,
                                  bool skipThis);
    void TryInlineGlobalFiniteBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op,
                                      bool skipThis);
    void TryInlineGlobalNanBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op,
                                   bool skipThis);
    void TryInlineMathMinMaxBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op,
                                    double defaultValue, bool skipThis);
    void TryInlineMathClz32Builtin(GateRef gate, size_t argc, bool skipThis);
    void TryInlineArrayBufferIsView(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineDataViewGet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id);
    void TryInlineDataViewSet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id);
    void InlineStubBuiltin(GateRef gate, size_t builtinArgc, size_t realArgc, BuiltinsStubCSigns::ID id,
        const GateMetaData* op, bool skipThis);
    void TryInlineDateGetTime(GateRef gate, size_t argc, bool skipThis);

    void AddTraceLogs(GateRef gate, BuiltinsStubCSigns::ID id);

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

    bool EnableTrace() const
    {
        return traceInline_;
    }

private:
    Circuit *circuit_ {nullptr};
    CircuitBuilder builder_;
    GateAccessor acc_;
    GateRef glue_;
    bool enableLog_;
    std::string methodName_;
    bool nocheck_;
    bool traceInline_;
    const CompilationEnv *compilationEnv_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_BUILTIN_INLINE_H
