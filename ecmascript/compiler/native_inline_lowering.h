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
    explicit NativeInlineLowering(Circuit *circuit, CompilationConfig* cmpCfg, PassContext *ctx, bool enableLog,
                                  const std::string& name, Chunk *chunk)
        : circuit_(circuit),
          builder_(circuit, cmpCfg),
          acc_(circuit),
          glue_(acc_.GetGlueFromArgList()),
          enableLog_(enableLog),
          methodName_(name),
          nocheck_(ctx->GetCompilationEnv()->GetJSOptions().IsCompilerNoCheck()),
          traceInline_(ctx->GetCompilationEnv()->GetJSOptions().GetTraceInline()),
          compilationEnv_(ctx->GetCompilationEnv()),
          chunk_(chunk) {}
    ~NativeInlineLowering() = default;
    void RunNativeInlineLowering();

private:
    enum IncludesOrIndexOf : uint8_t { INCLUDES = 0, INDEXOF };
    std::optional<std::pair<size_t, bool>> GetCallInfo(GateRef gate);
    void TryInlineStringFromCharCode(GateRef gate, size_t argc, bool skipThis);
    void TryInlineStringSubstring(GateRef gate, size_t argc, bool skipThis);
    void TryInlineStringSubStr(GateRef gate, size_t argc, bool skipThis);
    void TryInlineStringSlice(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsFinite(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsInteger(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsNaN(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberParseFloat(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberParseInt(GateRef gate, size_t argc, bool skipThis);
    void TryInlineNumberIsSafeInteger(GateRef gate, size_t argc, bool skipThis);
    void TryInlineTypedArrayIteratorBuiltin(GateRef gate, BuiltinsStubCSigns::ID id,
                                            const GateMetaData* op, bool skipThis);
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
    void TryInlineMathAbsBuiltin(GateRef gate, size_t argc, bool skipThis);
    void TryInlineMathClz32Builtin(GateRef gate, size_t argc, bool skipThis);
    void TryInlineArrayBufferIsView(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineBigIntAsIntN(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineDataViewGet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineDataViewSet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void InlineStubBuiltin(GateRef gate, size_t builtinArgc, size_t realArgc, BuiltinsStubCSigns::ID id,
        const GateMetaData* op, bool skipThis);
    void TryInlineDateGetTime(GateRef gate, size_t argc, bool skipThis);
    void TryInlineWhitoutParamBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                      const GateMetaData* op, bool skipThis);
    void TryInlineObjectIs(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineObjectGetPrototypeOf(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineObjectGetProto(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineObjectCreate(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineObjectIsPrototypeOf(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineObjectHasOwnProperty(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineReflectGetPrototypeOf(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineReflectGet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineReflectHas(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineReflectConstruct(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineReflectApply(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineFunctionPrototypeApply(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineFunctionPrototypeBind(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineFunctionPrototypeCall(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineFunctionPrototypeHasInstance(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);

    void TryInlineBigIntConstructor(GateRef gate, size_t argc, bool skipThis);
    void ReplaceGateWithPendingException(GateRef hirGate, GateRef value);
    void AddTraceLogs(GateRef gate, BuiltinsStubCSigns::ID id);
    void TryInlineIndexOfIncludes(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayIterator(GateRef gate, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayForEach(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayFindOrFindIndex(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayFilter(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayMap(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArraySome(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayEvery(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArrayPop(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArraySlice(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);
    void TryInlineArraySort(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, bool skipThis);

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
    Chunk* chunk_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_BUILTIN_INLINE_H
