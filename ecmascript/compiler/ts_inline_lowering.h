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
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/type_info_accessors.h"

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
    static constexpr size_t MAX_INLINE_CALL_ALLOWED = 6;
    TSInlineLowering(Circuit *circuit, PassContext *ctx, bool enableLog, const std::string &name,
                     NativeAreaAllocator *nativeAreaAllocator, PassOptions *options, uint32_t methodOffset,
                     CallMethodFlagMap *callMethodFlagMap)
        : circuit_(circuit),
          compilationEnv_(ctx->GetCompilationEnv()),
          acc_(circuit),
          glue_(acc_.GetGlueFromArgList()),
          builder_(circuit, ctx->GetCompilerConfig()),
          ptManager_(ctx->GetPTManager()),
          ctx_(ctx),
          passOptions_(options),
          enableLog_(enableLog),
          methodName_(name),
          enableTypeLowering_(ctx->GetCompilationEnv()->GetJSOptions().IsEnableTypeLowering()),
          traceInline_(ctx->GetCompilationEnv()->GetJSOptions().GetTraceInline()),
          maxInlineBytecodesCount_(ctx->GetCompilationEnv()->GetJSOptions().GetMaxInlineBytecodes()),
          nativeAreaAllocator_(nativeAreaAllocator),
          noCheck_(ctx->GetCompilationEnv()->GetJSOptions().IsCompilerNoCheck()),
          chunk_(circuit->chunk()),
          inlinedCallMap_(circuit->chunk()),
          argAcc_(circuit),
          initMethodOffset_(methodOffset),
          callMethodFlagMap_(callMethodFlagMap) {}

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

    bool IsCallSetter(CallKind kind) const
    {
        return kind == CallKind::CALL_SETTER;
    }

    void CollectInlineInfo();
    void GetInlinedMethodId(GateRef gate);
    void CandidateInlineCall(GateRef gate, ChunkQueue<InlineTypeInfoAccessor> &workList);
    void TryInline(InlineTypeInfoAccessor &info, ChunkQueue<InlineTypeInfoAccessor> &workList);
    bool FilterInlinedMethod(MethodLiteral* method, std::vector<const uint8_t*> pcOffsets);
    bool FilterCallInTryCatch(GateRef gate);
    void InlineCall(
        MethodInfo &methodInfo, MethodPcInfo &methodPCInfo, MethodLiteral *method, InlineTypeInfoAccessor &info);
    void ReplaceCallInput(InlineTypeInfoAccessor &info, GateRef glue, MethodLiteral *method);
    void ReplaceEntryGate(GateRef callGate, GateRef callerFunc, GateRef inlineFunc, GateRef glue);
    void ReplaceReturnGate(GateRef callGate);
    void ReplaceHirAndDeleteState(GateRef gate, GateRef state, GateRef depend, GateRef value);
    GateRef MergeAllReturn(const std::vector<GateRef> &returnVector, GateRef &state, GateRef &depend);
    bool CheckParameter(GateRef gate, InlineTypeInfoAccessor &info, MethodLiteral* method);
    void LowerToInlineCall(InlineTypeInfoAccessor &info, const std::vector<GateRef> &args, MethodLiteral* method);
    void RemoveRoot();
    void BuildFrameStateChain(InlineTypeInfoAccessor &info, BytecodeCircuitBuilder &builder);
    GateRef TraceInlineFunction(GateRef glue, GateRef depend, std::vector<GateRef> &args, GateRef callGate);
    void InlineFuncCheck(const InlineTypeInfoAccessor &info);
    void SupplementType(GateRef callGate, GateRef targetGate);
    void UpdateWorkList(ChunkQueue<InlineTypeInfoAccessor> &workList);
    size_t GetOrInitialInlineCounts(GateRef frameArgs);
    bool IsRecursiveFunc(InlineTypeInfoAccessor &info, size_t calleeMethodOffset);
    void CandidateAccessor(GateRef gate, ChunkQueue<InlineTypeInfoAccessor> &workList, CallKind kind);
    void CandidateNormalCall(GateRef gate, ChunkQueue<InlineTypeInfoAccessor> &workList, CallKind kind);
    void InlineAccessorCheck(const InlineTypeInfoAccessor &info);
    void InlineAccessorCheckInJIT(const InlineTypeInfoAccessor &info);
    void InlineCheck(InlineTypeInfoAccessor &info);
    GateRef GetAccessorReceiver(GateRef gate);
    GateRef GetFrameArgs(InlineTypeInfoAccessor &info);
    void ReplaceAccessorInput(InlineTypeInfoAccessor &info, GateRef glue, MethodLiteral *method);
    void ReplaceInput(InlineTypeInfoAccessor &info, GateRef glue, MethodLiteral *method);
    GateRef BuildAccessor(InlineTypeInfoAccessor &info);
    GateRef GetCallSetterValue(GateRef gate);
    GateRef GetFrameState(InlineTypeInfoAccessor &info);
    void SetInitCallTargetAndConstPoolId(InlineTypeInfoAccessor &info);
    uint32_t GetAccessorConstpoolId(InlineTypeInfoAccessor &info);
    bool CalleePFIProcess(uint32_t methodOffset);
    void UpdateCallMethodFlagMap(uint32_t methodOffset, const MethodLiteral *method);

    Circuit *circuit_ {nullptr};
    CompilationEnv *compilationEnv_ {nullptr};
    GateAccessor acc_;
    GateRef glue_;
    CircuitBuilder builder_;
    PGOTypeManager *ptManager_ {nullptr};
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
    uint32_t initConstantPoolId_ {0};
    GateRef initCallTarget_ {Circuit::NullGate()};
    CallMethodFlagMap *callMethodFlagMap_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_INLINE_LOWERING_H
