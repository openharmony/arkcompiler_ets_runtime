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

#ifndef ECMASCRIPT_COMPILER_TS_INLINE_LOWERING_H
#define ECMASCRIPT_COMPILER_TS_INLINE_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::kungfu {
class CircuitRootScope {
public:
    explicit CircuitRootScope(Circuit *circuit)
        : circuit_(circuit), root_(circuit->GetRoot())
    {
    }

    ~CircuitRootScope()
    {
        circuit_->SetRoot(root_);
    }

private:
    Circuit *circuit_ {nullptr};
    GateRef root_ { 0 };
};

class TSInlineLowering {
public:
    static constexpr size_t MAX_INLINE_BYTECODE_COUNT = 10;
    static constexpr size_t MAX_INLINE_CALL_ALLOWED = 5;
    TSInlineLowering(Circuit *circuit, PassInfo *info, bool enableLog, const std::string& name)
        : circuit_(circuit), acc_(circuit), builder_(circuit, info->GetCompilerConfig()),
          tsManager_(info->GetTSManager()), info_(info), enableLog_(enableLog), methodName_(name) {}

    ~TSInlineLowering() = default;

    void RunTSInlineLowering();

private:
    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void TryInline(GateRef gate);
    void TryInline(GateRef gate, bool isCallThis);
    bool FilterInlinedMethod(MethodLiteral* method, std::vector<const uint8_t*> pcOffsets);
    void InlineCall(MethodInfo &methodInfo, MethodPcInfo &methodPCInfo, MethodLiteral* method);
    CString GetRecordName(const JSPandaFile *jsPandaFile,
        panda_file::File::EntityId methodIndex);
    void ReplaceCallInput(GateRef gate, bool isCallThis);

    void ReplaceEntryGate(GateRef callGate);
    void ReplaceReturnGate(GateRef callGate);

    void ReplaceHirAndDeleteState(GateRef gate, GateRef state, GateRef depend, GateRef value);

    GateRef MergeAllReturn(const std::vector<GateRef> &returnVector,
        GateRef &state, GateRef &depend, size_t numOfIns);
    bool CheckParameter(GateRef gate, bool isCallThis, MethodLiteral* method);

    void LowerToInlineCall(GateRef gate, const std::vector<GateRef> &args);

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    TSManager *tsManager_ {nullptr};
    PassInfo *info_ {nullptr};
    bool enableLog_ {false};
    std::string methodName_;
    size_t inlinedCall_ { 0 };
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_INLINE_LOWERING_H