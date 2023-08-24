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
enum CallKind : uint8_t {
    CALL,
    CALL_THIS,
    CALL_SETTER,
    CALL_GETTER,
    INVALID
};
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

class CallGateInfo {
public:
    explicit CallGateInfo(GateRef call, CallKind kind, GlobalTSTypeRef gt, uint32_t type)
        : call_(call), kind_(kind), gt_(gt), type_(type)
    {
    }

    ~CallGateInfo() = default;

    GateRef GetCallGate() const
    {
        return call_;
    }

    bool IsCallThis() const
    {
        return kind_ == CallKind::CALL_THIS;
    }

    bool IsNormalCall() const
    {
        return kind_ == CallKind::CALL || kind_ == CallKind::CALL_THIS;
    }

    bool IsCallAccessor() const
    {
        return kind_ == CallKind::CALL_SETTER || kind_ == CallKind::CALL_GETTER;
    }

    bool IsCallGetter() const
    {
        return kind_ == CallKind::CALL_GETTER;
    }
    
    bool IsCallSetter() const
    {
        return kind_ == CallKind::CALL_SETTER;
    }

    GlobalTSTypeRef GetFuncGT() const
    {
        return gt_;
    }

    uint32_t GetType() const
    {
        return type_;
    }

private:
    GateRef call_ {Circuit::NullGate()};
    CallKind kind_ {CallKind::INVALID};
    GlobalTSTypeRef gt_;
    uint32_t type_;
};

class TSInlineLowering {
public:
    static constexpr size_t MAX_INLINE_CALL_ALLOWED = 5;
    TSInlineLowering(Circuit *circuit, PassContext *ctx, bool enableLog, const std::string& name,
                     NativeAreaAllocator* nativeAreaAllocator, PassOptions *options, uint32_t methodOffset)
        : circuit_(circuit),
          acc_(circuit),
          builder_(circuit, ctx->GetCompilerConfig()),
          tsManager_(ctx->GetTSManager()),
          ctx_(ctx),
          passOptions_(options),
          enableLog_(enableLog),
          methodName_(name),
          enableTypeLowering_(ctx->GetEcmaVM()->GetJSOptions().IsEnableTypeLowering()),
          traceInline_(ctx->GetEcmaVM()->GetJSOptions().GetTraceInline()),
          maxInlineBytecodesCount_(ctx->GetEcmaVM()->GetJSOptions().GetMaxInlineBytecodes()),
          nativeAreaAllocator_(nativeAreaAllocator),
          noCheck_(ctx->GetEcmaVM()->GetJSOptions().IsCompilerNoCheck()),
          chunk_(circuit->chunk()),
          inlinedCallMap_(circuit->chunk()),
          argAcc_(circuit),
          initMethodOffset_(methodOffset) {}

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

    bool IsSmallMethod(size_t bcSize) const
    {
        return bcSize <= maxInlineBytecodesCount_;
    }

    bool IsInlineCountsOverflow(size_t inlineCount) const
    {
        return inlineCount >= MAX_INLINE_CALL_ALLOWED;
    }

    void UpdateInlineCounts(GateRef frameArgs, size_t inlineCallCounts)
    {
        inlinedCallMap_[frameArgs] = ++inlineCallCounts;
    }

    bool EnableFastAccessor() const
    {
        return isFastAccessor_ && !traceInline_;
    }

    bool IsCallSetter(CallKind kind) const
    {
        return kind == CallKind::CALL_SETTER;
    }

    void CandidateInlineCall(GateRef gate, ChunkQueue<CallGateInfo> &workList);
    void TryInline(CallGateInfo &info, ChunkQueue<CallGateInfo> &workList);
    bool FilterInlinedMethod(MethodLiteral* method, std::vector<const uint8_t*> pcOffsets);
    bool FilterCallInTryCatch(GateRef gate);
    void InlineCall(MethodInfo &methodInfo, MethodPcInfo &methodPCInfo, MethodLiteral* method, CallGateInfo &info);
    void ReplaceCallInput(CallGateInfo &info, GateRef glue, MethodLiteral *method);
    void ReplaceEntryGate(GateRef callGate, GateRef callerFunc, GateRef inlineFunc, GateRef glue);
    void ReplaceReturnGate(GateRef callGate);
    void ReplaceHirAndDeleteState(GateRef gate, GateRef state, GateRef depend, GateRef value);
    GateRef MergeAllReturn(const std::vector<GateRef> &returnVector, GateRef &state, GateRef &depend);
    bool CheckParameter(GateRef gate, CallGateInfo &info, MethodLiteral* method);
    void LowerToInlineCall(CallGateInfo &info, const std::vector<GateRef> &args, MethodLiteral* method);
    void RemoveRoot();
    void BuildFrameStateChain(CallGateInfo &info, BytecodeCircuitBuilder &builder);
    GateRef TraceInlineFunction(GateRef glue, GateRef depend, std::vector<GateRef> &args, GateRef callGate);
    void InlineFuncCheck(const CallGateInfo &info);
    void SupplementType(GateRef callGate, GateRef targetGate);
    void UpdateWorkList(ChunkQueue<CallGateInfo> &workList);
    size_t GetOrInitialInlineCounts(GateRef frameArgs);
    bool IsRecursiveFunc(CallGateInfo &info, size_t calleeMethodOffset);
    bool IsAccessor(GateRef receiver, GateRef constData);
    GlobalTSTypeRef GetAccessorFuncType(GateRef receiver, GateRef constData);
    void CandidateAccessor(GateRef gate, ChunkQueue<CallGateInfo> &workList, CallKind kind);
    void CandidateNormalCall(GateRef gate, ChunkQueue<CallGateInfo> &workList, CallKind kind);
    void InlineAccessorCheck(GateRef gate, GateRef receiver);
    void InlineCheck(CallGateInfo &info);
    GateRef GetAccessorReceiver(GateRef gate);
    GateRef GetFrameArgs(CallGateInfo &info);
    void ReplaceAccessorInput(CallGateInfo &info, GateRef glue, MethodLiteral *method);
    void ReplaceInput(CallGateInfo &info, GateRef glue, MethodLiteral *method);
    GateRef BuildAccessor(CallGateInfo &info);
    uint32_t GetPlrData(GateRef receiver, GateRef constData);
    GateRef GetCallSetterValue(GateRef gate);
    GlobalTSTypeRef GetAccessorFuncGT(GateRef receiver, GateRef constData, bool isCallSetter);
    GateRef GetFrameState(CallGateInfo &info);
    void SetInitCallTargetAndConstPoolId(CallGateInfo &info);
    void AnalyseFastAccessor(CallGateInfo &info, std::vector<const uint8_t*> pcOffsets, uint32_t inlineMethodOffset);

    Circuit *circuit_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    TSManager *tsManager_ {nullptr};
    PassContext *ctx_ {nullptr};
    PassOptions *passOptions_ {nullptr};
    bool enableLog_ {false};
    std::string methodName_;
    bool enableTypeLowering_ {false};
    bool inlineSuccess_ {false};
    bool traceInline_ {false};
    size_t maxInlineBytecodesCount_ {0};
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    bool noCheck_ {false};
    Chunk* chunk_ {nullptr};
    ChunkMap<GateRef, size_t> inlinedCallMap_;
    size_t lastCallId_ {0};
    ArgumentAccessor argAcc_;
    uint32_t initMethodOffset_ {0};
    int32_t initConstantPoolId_ {0};
    GateRef initCallTarget_ {Circuit::NullGate()};
    bool isFastAccessor_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_INLINE_LOWERING_H
