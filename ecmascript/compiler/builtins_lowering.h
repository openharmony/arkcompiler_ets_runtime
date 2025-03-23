/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTIN_LOWERING_H
#define ECMASCRIPT_COMPILER_BUILTIN_LOWERING_H

#include <string>
#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {
class BuiltinLowering {
public:
    BuiltinLowering(Circuit *circuit, CompilationConfig *cmpCfg, bool traceBuiltins)
        : circuit_(circuit), builder_(circuit, cmpCfg), acc_(circuit), traceBuiltins_(traceBuiltins)
    {}
    ~BuiltinLowering() = default;
    void LowerTypedCallBuitin(GateRef gate);
    GateRef LowerCallTargetCheck(Environment *env, GateRef gate);
    GateRef CheckPara(GateRef gate, GateRef funcCheck);
    void LowerTypedLocaleCompare(GateRef gate);
    void LowerTypedArraySort(GateRef gate);

private:
    void LowerTypedFloor(GateRef gate);
    GateRef TypedLocaleCompare(GateRef glue, GateRef gate, GateRef thisObj, GateRef thatObj);
    GateRef TypedFloor(GateRef gate);
    GateRef IntToTaggedIntPtr(GateRef x);
    GateRef LowerCallRuntime(GateRef glue, GateRef gate, int index, const std::vector<GateRef> &args,
                             bool useLabel = false);
    void ReplaceHirWithValue(GateRef hirGate, GateRef value);
    GateRef LowerCallTargetCheckDefault(GateRef gate, BuiltinsStubCSigns::ID id);
    GateRef LowerCallTargetCheckWithGlobalEnv(GateRef gate, BuiltinsStubCSigns::ID id);
    GateRef LowerCallTargetCheckWithDetector(GateRef gate, BuiltinsStubCSigns::ID id);
    GateRef LowerCallTargetCheckWithObjectType(GateRef gate, BuiltinsStubCSigns::ID id);
    void LowerTypedStringify(GateRef gate);
    void LowerBuiltinIterator(GateRef gate, BuiltinsStubCSigns::ID id);
    void LowerIteratorNext(GateRef gate, BuiltinsStubCSigns::ID id);
    void LowerIteratorReturn(GateRef gate, BuiltinsStubCSigns::ID id);
    void LowerNumberConstructor(GateRef gate);
    void LowerGlobalDecodeURIComponent(GateRef gate);
    void LowerCallBuiltinStub(GateRef gate, BuiltinsStubCSigns::ID id);
    void AddTraceLogs(GateRef gate, BuiltinsStubCSigns::ID id);

    Circuit *circuit_ {nullptr};
    CircuitBuilder builder_;
    GateAccessor acc_;
    bool traceBuiltins_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BUILTIN_LOWERING_H
