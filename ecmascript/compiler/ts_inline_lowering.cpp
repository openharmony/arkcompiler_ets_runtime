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
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/ts_inline_lowering.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_type.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "ecmascript/ts_types/ts_type_accessor.h"

namespace panda::ecmascript::kungfu {
void TSInlineLowering::RunTSInlineLowering()
{
    circuit_->AdvanceTime();
    ChunkQueue<CallGateInfo> workList(chunk_);
    UpdateWorkList(workList);

    while (!workList.empty()) {
        CallGateInfo info = workList.front();
        workList.pop();
        TryInline(info, workList);
    }
}

void TSInlineLowering::CandidateInlineCall(GateRef gate, ChunkQueue<CallGateInfo> &workList)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            CandidateAccessor(gate, workList, CallKind::CALL_GETTER);
            break;
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            CandidateAccessor(gate, workList, CallKind::CALL_SETTER);
            break;
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
            CandidateNormalCall(gate, workList, CallKind::CALL_THIS);
            break;
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            CandidateNormalCall(gate, workList, CallKind::CALL);
            break;
        default:
            break;
    }
}

void TSInlineLowering::TryInline(CallGateInfo &info, ChunkQueue<CallGateInfo> &workList)
{
    GateRef gate = info.GetCallGate();
    // inline doesn't support try-catch
    bool inTryCatch = FilterCallInTryCatch(gate);
    if (inTryCatch) {
        return;
    }

    MethodLiteral* inlinedMethod = nullptr;
    GlobalTSTypeRef gt = info.GetFuncGT();
    auto methodOffset = tsManager_->GetFuncMethodOffset(gt);
    if (methodOffset == 0 || ctx_->IsSkippedMethod(methodOffset)) {
        return;
    }
    if (IsRecursiveFunc(info, methodOffset)) {
        return;
    }
    inlinedMethod = ctx_->GetJSPandaFile()->FindMethodLiteral(methodOffset);
    if (!CheckParameter(gate, info, inlinedMethod)) {
        return;
    }
    auto &bytecodeInfo = ctx_->GetBytecodeInfo();
    auto &methodInfo = bytecodeInfo.GetMethodList().at(methodOffset);
    auto &methodPcInfos = bytecodeInfo.GetMethodPcInfos();
    auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
    GateRef frameState = GetFrameState(info);
    GateRef frameArgs = acc_.GetValueIn(frameState);
    size_t inlineCallCounts = GetOrInitialInlineCounts(frameArgs);
    if (IsSmallMethod(methodPcInfo.pcOffsets.size()) && !IsInlineCountsOverflow(inlineCallCounts)) {
        inlineSuccess_ = FilterInlinedMethod(inlinedMethod, methodPcInfo.pcOffsets);
        if (inlineSuccess_) {
            GateRef glue = acc_.GetGlueFromArgList();
            SetInitCallTargetAndConstPoolId(info);
            CircuitRootScope scope(circuit_);
            AnalyseFastAccessor(info, methodPcInfo.pcOffsets, methodOffset);
            if (!noCheck_) {
                InlineCheck(info);
            }
            InlineCall(methodInfo, methodPcInfo, inlinedMethod, info);
            ReplaceInput(info, glue, inlinedMethod);
            UpdateInlineCounts(frameArgs, inlineCallCounts);
            if (info.IsNormalCall()) {
                UpdateWorkList(workList);
            } else {
                lastCallId_ = circuit_->GetGateCount() - 1;
            }
        }
    }

    if ((inlinedMethod != nullptr) && IsLogEnabled() && inlineSuccess_) {
        inlineSuccess_ = false;
        auto jsPandaFile = ctx_->GetJSPandaFile();
        const std::string methodName(
            MethodLiteral::GetMethodName(jsPandaFile, inlinedMethod->GetMethodId()));
        std::string fileName = jsPandaFile->GetFileName();
        const std::string recordName(MethodLiteral::GetRecordName(jsPandaFile, inlinedMethod->GetMethodId()));
        std::string fullName = methodName + "@" + recordName + "@" + fileName;
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After inlining "
                           << "[" << fullName << "]"
                           << " Caller method "
                           << "[" << methodName_ << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

bool TSInlineLowering::FilterInlinedMethod(MethodLiteral* method, std::vector<const uint8_t*> pcOffsets)
{
    const JSPandaFile *jsPandaFile = ctx_->GetJSPandaFile();
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pf, method->GetMethodId());
    panda_file::CodeDataAccessor cda(*pf, mda.GetCodeId().value());
    if (cda.GetTriesSize() != 0) {
        return false;
    }
    for (size_t i = 0; i < pcOffsets.size(); i++) {
        auto pc = pcOffsets[i];
        auto ecmaOpcode = ctx_->GetByteCodes()->GetOpcode(pc);
        switch (ecmaOpcode) {
            case EcmaOpcode::GETUNMAPPEDARGS:
            case EcmaOpcode::SUSPENDGENERATOR_V8:
            case EcmaOpcode::RESUMEGENERATOR:
            case EcmaOpcode::COPYRESTARGS_IMM8:
            case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
            case EcmaOpcode::CREATEASYNCGENERATOROBJ_V8:
                return false;
            default:
                break;
        }
    }
    return true;
}

void TSInlineLowering::InlineCall(MethodInfo &methodInfo, MethodPcInfo &methodPCInfo, MethodLiteral* method,
                                  CallGateInfo &info)
{
    const JSPandaFile *jsPandaFile = ctx_->GetJSPandaFile();
    TSManager *tsManager = ctx_->GetTSManager();
    CompilerLog *log = ctx_->GetCompilerLog();
    CString recordName = MethodLiteral::GetRecordName(jsPandaFile, method->GetMethodId());
    bool hasTyps = jsPandaFile->HasTSTypes(recordName);
    if (!hasTyps) {
        return;
    }
    const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, method->GetMethodId()));
    std::string fileName = jsPandaFile->GetFileName();
    std::string fullName = methodName + "@" + std::string(recordName) + "@" + fileName;

    circuit_->InitRoot();
    BytecodeCircuitBuilder builder(jsPandaFile, method, methodPCInfo,
                                   tsManager, circuit_,
                                   ctx_->GetByteCodes(), true, IsLogEnabled(),
                                   enableTypeLowering_, fullName, recordName, ctx_->GetPfDecoder(), true,
                                   passOptions_->EnableOptTrackField());
    {
        if (enableTypeLowering_) {
            BuildFrameStateChain(info, builder);
        }
        TimeScope timeScope("BytecodeToCircuit", methodName, method->GetMethodId().GetOffset(), log);
        builder.BytecodeToCircuit();
    }

    PassData data(&builder, circuit_, ctx_, log, fullName,
                  &methodInfo, hasTyps, recordName,
                  method, method->GetMethodId().GetOffset(), nativeAreaAllocator_, ctx_->GetPfDecoder(), passOptions_);
    PassRunner<PassData> pipeline(&data);
    if (builder.EnableLoopOptimization()) {
        pipeline.RunPass<LoopOptimizationPass>();
    }
    pipeline.RunPass<TypeInferPass>();
    pipeline.RunPass<PGOTypeInferPass>();
}

bool TSInlineLowering::CheckParameter(GateRef gate, CallGateInfo &info, MethodLiteral* method)
{
    if (info.IsCallAccessor()) {
        return true;
    }
    size_t numIns = acc_.GetNumValueIn(gate);
    size_t fixedInputsNum = info.IsCallThis() ? 2 : 1; // 2: calltarget and this

    uint32_t declaredNumArgs = method->GetNumArgsWithCallField();
    return declaredNumArgs == (numIns - fixedInputsNum);
}

void TSInlineLowering::ReplaceCallInput(CallGateInfo &info, GateRef glue, MethodLiteral *method)
{
    GateRef gate = info.GetCallGate();
    bool isCallThis = info.IsCallThis();
    std::vector<GateRef> vec;
    size_t numIns = acc_.GetNumValueIn(gate);
    // 1: last one elem is function
    GateRef callTarget = acc_.GetValueIn(gate, numIns - 1);
    GateRef thisObj = Circuit::NullGate();
    size_t fixedInputsNum = 0;
    if (isCallThis) {
        fixedInputsNum = 2; // 2: call target and this
        thisObj = acc_.GetValueIn(gate, 0);
    } else {
        fixedInputsNum = 1; // 1: call target
        thisObj = builder_.Undefined();
    }
    // -1: callTarget
    size_t actualArgc = numIns + NUM_MANDATORY_JSFUNC_ARGS - fixedInputsNum;
    vec.emplace_back(glue); // glue
    if (!method->IsFastCall()) {
        vec.emplace_back(builder_.Int64(actualArgc)); // argc
    }
    vec.emplace_back(callTarget);
    if (!method->IsFastCall()) {
        vec.emplace_back(builder_.Undefined()); // newTarget
    }
    vec.emplace_back(thisObj);
    // -1: call Target
    for (size_t i = fixedInputsNum - 1; i < numIns - 1; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    LowerToInlineCall(info, vec, method);
}

void TSInlineLowering::ReplaceAccessorInput(CallGateInfo &info, GateRef glue, MethodLiteral *method)
{
    GateRef gate = info.GetCallGate();
    std::vector<GateRef> vec;
    GateRef thisObj = GetAccessorReceiver(gate);
    GateRef callTarget = Circuit::NullGate();
    // Fast accessor will not load getter or setter func
    if (EnableFastAccessor()) {
        callTarget = initCallTarget_;
    } else {
        callTarget = BuildAccessor(info);
    }
    size_t actualArgc = 0;
    if (info.IsCallGetter()) {
        actualArgc = NUM_MANDATORY_JSFUNC_ARGS;
    } else if (info.IsCallSetter()) {
        actualArgc = NUM_MANDATORY_JSFUNC_ARGS + 1;
    } else {
        UNREACHABLE();
    }

    vec.emplace_back(glue); // glue
    if (!method->IsFastCall()) {
        vec.emplace_back(builder_.Int64(actualArgc)); // argc
    }
    vec.emplace_back(callTarget);
    if (!method->IsFastCall()) {
        vec.emplace_back(builder_.Undefined()); // newTarget
    }
    vec.emplace_back(thisObj);

    if (info.IsCallSetter()) {
        vec.emplace_back(GetCallSetterValue(gate));
    }
    LowerToInlineCall(info, vec, method);
}

GateRef TSInlineLowering::BuildAccessor(CallGateInfo &info)
{
    GateRef gate = info.GetCallGate();
    GateRef depend = acc_.GetDep(gate);
    GateRef receiver = GetAccessorReceiver(gate);
    GateRef accessor = Circuit::NullGate();
    uint32_t plrData = GetPlrData(receiver, acc_.GetValueIn(gate, 1));
    if (info.IsCallGetter()) {
        accessor = circuit_->NewGate(circuit_->LoadGetter(), MachineType::I64,
                                     {depend, receiver, builder_.Int32(plrData)}, GateType::AnyType());
    } else {
        accessor = circuit_->NewGate(circuit_->LoadSetter(), MachineType::I64,
                                     {depend, receiver, builder_.Int32(plrData)}, GateType::AnyType());
    }
    acc_.ReplaceDependIn(gate, accessor);
    return accessor;
}

uint32_t TSInlineLowering::GetPlrData(GateRef receiver, GateRef constData)
{
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto prop = tsManager_->GetStringFromConstantPool(propIndex);
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetValueFromCache(hclassIndex).GetTaggedObject());
    PropertyLookupResult plr = JSHClass::LookupPropertyInAotHClass(tsManager_->GetThread(), hclass, prop);
    return plr.GetData();
}

void TSInlineLowering::ReplaceInput(CallGateInfo &info, GateRef glue, MethodLiteral *method)
{
    if (info.IsNormalCall()) {
        ReplaceCallInput(info, glue, method);
    } else {
        ASSERT(info.IsCallAccessor());
        ReplaceAccessorInput(info, glue, method);
    }
}

GateRef TSInlineLowering::MergeAllReturn(const std::vector<GateRef> &returnVector, GateRef &state, GateRef &depend)
{
    size_t numOfIns = returnVector.size();
    auto stateList = std::vector<GateRef>(numOfIns, Circuit::NullGate());
    auto dependList = std::vector<GateRef>(numOfIns + 1, Circuit::NullGate());
    auto vaueList = std::vector<GateRef>(numOfIns + 1, Circuit::NullGate());

    for (size_t i = 0; i < returnVector.size(); i++) {
        GateRef returnGate = returnVector.at(i);
        ASSERT(acc_.GetOpCode(acc_.GetState(returnGate)) != OpCode::IF_EXCEPTION);
        stateList[i] = acc_.GetState(returnGate);
        dependList[i + 1] = acc_.GetDep(returnGate);
        vaueList[i + 1] = acc_.GetValueIn(returnGate, 0);
        acc_.DeleteGate(returnGate);
    }

    state = circuit_->NewGate(circuit_->Merge(numOfIns), stateList);
    dependList[0] = state;
    vaueList[0] = state;
    depend = circuit_->NewGate(circuit_->DependSelector(numOfIns), dependList);
    return circuit_->NewGate(circuit_->ValueSelector(numOfIns), MachineType::I64, numOfIns + 1,
                             vaueList.data(), GateType::AnyType());
}

void TSInlineLowering::ReplaceEntryGate(GateRef callGate, GateRef callerFunc, GateRef inlineFunc, GateRef glue)
{
    auto stateEntry = acc_.GetStateRoot();
    auto dependEntry = acc_.GetDependRoot();

    GateRef callState = acc_.GetState(callGate);
    GateRef callDepend = acc_.GetDep(callGate);
    auto stateUse = acc_.Uses(stateEntry);

    // support inline trace
    GateRef newDep = Circuit::NullGate();
    if (traceInline_) {
        std::vector<GateRef> args{callerFunc, inlineFunc};
        newDep = TraceInlineFunction(glue, callDepend, args, callGate);
    } else {
        newDep = callDepend;
    }

    for (auto stateUseIt = stateUse.begin(); stateUseIt != stateUse.end();) {
        stateUseIt = acc_.ReplaceIn(stateUseIt, callState);
    }

    auto dependUse = acc_.Uses(dependEntry);
    for (auto dependUseIt = dependUse.begin(); dependUseIt != dependUse.end();) {
        dependUseIt = acc_.ReplaceIn(dependUseIt, newDep);
    }
}

GateRef TSInlineLowering::TraceInlineFunction(GateRef glue, GateRef depend, std::vector<GateRef> &args,
                                              GateRef callGate)
{
    size_t index = RTSTUB_ID(AotInlineTrace);
    GateRef result = builder_.NoLabelCallRuntime(glue, depend, index, args, callGate);
    return result;
}

void TSInlineLowering::ReplaceReturnGate(GateRef callGate)
{
    std::vector<GateRef> returnVector;
    acc_.GetReturnOuts(returnVector);

    GateRef value = Circuit::NullGate();
    GateRef state = Circuit::NullGate();
    GateRef depend = Circuit::NullGate();

    if (returnVector.size() == 1) {
        GateRef returnGate = returnVector.at(0);
        GateRef returnState = acc_.GetState(returnGate);
        depend = acc_.GetDep(returnGate);
        state = returnState;
        value = acc_.GetValueIn(returnGate, 0);
        acc_.DeleteGate(returnGate);
    } else {
        value = MergeAllReturn(returnVector, state, depend);
    }
    SupplementType(callGate, value);
    ReplaceHirAndDeleteState(callGate, state, depend, value);
}

void TSInlineLowering::ReplaceHirAndDeleteState(GateRef gate, GateRef state, GateRef depend, GateRef value)
{
    auto uses = acc_.Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        if (acc_.IsStateIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, state);
        } else if (acc_.IsDependIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, depend);
        } else if (acc_.IsValueIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, value);
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    acc_.DeleteGate(gate);
}

void TSInlineLowering::LowerToInlineCall(CallGateInfo &info, const std::vector<GateRef> &args, MethodLiteral* method)
{
    GateRef callGate = info.GetCallGate();
    // replace in value/args
    ArgumentAccessor argAcc(circuit_);
    ASSERT(argAcc.ArgsCount() == args.size());
    for (size_t i = 0; i < argAcc.ArgsCount(); i++) {
        GateRef arg = argAcc.ArgsAt(i);
        acc_.UpdateAllUses(arg, args.at(i));
        acc_.SetGateType(args.at(i), acc_.GetGateType(arg));
        acc_.DeleteGate(arg);
    }
    // replace in depend and state
    GateRef glue = args.at(static_cast<size_t>(CommonArgIdx::GLUE));
    GateRef inlineFunc;
    if (method->IsFastCall()) {
        inlineFunc = args.at(static_cast<size_t>(FastCallArgIdx::FUNC));
    } else {
        inlineFunc = args.at(static_cast<size_t>(CommonArgIdx::FUNC));
    }
    GateRef frameArgs = GetFrameArgs(info);
    GateRef callerFunc = acc_.GetValueIn(frameArgs, 0);
    ReplaceEntryGate(callGate, callerFunc, inlineFunc, glue);
    // replace use gate
    ReplaceReturnGate(callGate);
    // remove Useless root gates
    RemoveRoot();
}

void TSInlineLowering::InlineFuncCheck(const CallGateInfo &info)
{
    GateRef gate = info.GetCallGate();
    GateRef callState = acc_.GetState(gate);
    GateRef callDepend = acc_.GetDep(gate);
    ASSERT(acc_.HasFrameState(gate));
    GateRef frameState = acc_.GetFrameState(gate);
    size_t funcIndex = acc_.GetNumValueIn(gate) - 1;
    GateRef inlineFunc =  acc_.GetValueIn(gate, funcIndex);
    // Do not load from inlineFunc beacause type in inlineFunc could be modified by others
    GlobalTSTypeRef funcGt = info.GetFuncGT();
    auto methodOffset = tsManager_->GetFuncMethodOffset(funcGt);
    GateRef ret = circuit_->NewGate(circuit_->JSInlineTargetTypeCheck(info.GetType()),
        MachineType::I1, {callState, callDepend, inlineFunc, builder_.IntPtr(methodOffset), frameState},
        GateType::NJSValue());
    acc_.ReplaceStateIn(gate, ret);
    acc_.ReplaceDependIn(gate, ret);
}

void TSInlineLowering::InlineAccessorCheck(GateRef gate, GateRef receiver)
{
    GateRef callState = acc_.GetState(gate);
    GateRef callDepend = acc_.GetDep(gate);
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
    GateRef hclassIndexGate = builder_.IntPtr(hclassIndex);
    auto frameState = acc_.FindNearestFrameState(callDepend);
    GateRef ret = circuit_->NewGate(circuit_->InlineAccessorCheck(static_cast<size_t>(receiverType.Value())),
        MachineType::I1, {callState, callDepend, receiver, hclassIndexGate, frameState}, GateType::NJSValue());
    acc_.ReplaceStateIn(gate, ret);
    acc_.ReplaceDependIn(gate, ret);
}

void TSInlineLowering::InlineCheck(CallGateInfo &info)
{
    if (info.IsNormalCall()) {
        InlineFuncCheck(info);
    } else {
        ASSERT(info.IsCallAccessor());
        GateRef gate = info.GetCallGate();
        GateRef receiver = GetAccessorReceiver(gate);
        InlineAccessorCheck(gate, receiver);
    }
}

void TSInlineLowering::RemoveRoot()
{
    GateRef circuitRoot = acc_.GetCircuitRoot();
    auto uses = acc_.Uses(circuitRoot);
    for (auto it = uses.begin(); it != uses.end();) {
        it = acc_.DeleteGate(it);
    }

    acc_.DeleteGate(circuitRoot);
}

void TSInlineLowering::BuildFrameStateChain(CallGateInfo &info, BytecodeCircuitBuilder &builder)
{
    GateRef preFrameState = GetFrameState(info);
    ASSERT(acc_.GetOpCode(preFrameState) == OpCode::FRAME_STATE);
    builder.SetPreFrameState(preFrameState);
}

bool TSInlineLowering::FilterCallInTryCatch(GateRef gate)
{
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end(); ++it) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS || acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
            return true;
        }
    }
    return false;
}

void TSInlineLowering::SupplementType(GateRef callGate, GateRef targetGate)
{
    GateType callGateType = acc_.GetGateType(callGate);
    GateType targetGateType = acc_.GetGateType(targetGate);
    if (!callGateType.IsAnyType() && targetGateType.IsAnyType()) {
        acc_.SetGateType(targetGate, callGateType);
    }
}

void TSInlineLowering::UpdateWorkList(ChunkQueue<CallGateInfo> &workList)
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        if (acc_.GetId(gate) <= lastCallId_) {
            continue;
        }
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            CandidateInlineCall(gate, workList);
        }
    }
}

size_t TSInlineLowering::GetOrInitialInlineCounts(GateRef frameArgs)
{
    auto it = inlinedCallMap_.find(frameArgs);
    if (it == inlinedCallMap_.end()) {
        inlinedCallMap_[frameArgs] = 0;
    }
    return inlinedCallMap_[frameArgs];
}


bool TSInlineLowering::IsRecursiveFunc(CallGateInfo &info, size_t calleeMethodOffset)
{
    GateRef frameArgs = GetFrameArgs(info);
    GateRef caller = acc_.GetValueIn(frameArgs);
    auto funcType = acc_.GetGateType(caller);
    GlobalTSTypeRef gt = funcType.GetGTRef();
    if (!tsManager_->IsFunctionTypeKind(gt)) {
        return false;
    }
    auto callerMethodOffset = tsManager_->GetFuncMethodOffset(gt);
    return callerMethodOffset == calleeMethodOffset;
}

bool TSInlineLowering::IsAccessor(GateRef receiver, GateRef constData)
{
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto prop = tsManager_->GetStringFromConstantPool(propIndex);
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    if (tsManager_->IsClassInstanceTypeKind(receiverType)) {
        int hclassIndex = tsManager_->GetHClassIndexByInstanceGateType(receiverType);
        if (hclassIndex == -1) {
            return false;
        }
        JSHClass *hclass = JSHClass::Cast(tsManager_->GetValueFromCache(hclassIndex).GetTaggedObject());
        if (!hclass->HasTSSubtyping()) {
            return false;
        }
        PropertyLookupResult plr = JSHClass::LookupPropertyInAotHClass(tsManager_->GetThread(), hclass, prop);
        if (!plr.IsFound()) {
            return false;
        }

        return plr.IsAccessor();
    }
    return false;
}

GlobalTSTypeRef TSInlineLowering::GetAccessorFuncGT(GateRef receiver, GateRef constData, bool isCallSetter)
{
    GateType receiverType = acc_.GetGateType(receiver);
    receiverType = tsManager_->TryNarrowUnionType(receiverType);
    GlobalTSTypeRef classInstanceGT = receiverType.GetGTRef();
    GlobalTSTypeRef classGT = tsManager_->GetClassType(classInstanceGT);
    TSTypeAccessor tsTypeAcc(tsManager_, classGT);
    uint16_t propIndex = acc_.GetConstantValue(constData);
    auto prop = tsManager_->GetStringFromConstantPool(propIndex);
    GlobalTSTypeRef funcGT = tsTypeAcc.GetAccessorGT(prop, isCallSetter);
    return funcGT;
}

void TSInlineLowering::CandidateAccessor(GateRef gate, ChunkQueue<CallGateInfo> &workList, CallKind kind)
{
    GateRef receiver = GetAccessorReceiver(gate);
    GateRef constData = acc_.GetValueIn(gate, 1);
    if (IsAccessor(receiver, constData)) {
        GlobalTSTypeRef gt = GetAccessorFuncGT(receiver, constData, IsCallSetter(kind));
        if (!gt.IsDefault()) {
            workList.push(CallGateInfo(gate, kind, gt, 0));
            lastCallId_ = acc_.GetId(gate);
        }
    }
}

void TSInlineLowering::CandidateNormalCall(GateRef gate, ChunkQueue<CallGateInfo> &workList, CallKind kind)
{
    size_t funcIndex = acc_.GetNumValueIn(gate) - 1;
    auto funcType = acc_.GetGateType(acc_.GetValueIn(gate, funcIndex));
    if (tsManager_->IsFunctionTypeKind(funcType)) {
        GlobalTSTypeRef gt = funcType.GetGTRef();
        workList.push(CallGateInfo(gate, kind, gt, funcType.Value()));
        lastCallId_ = acc_.GetId(gate);
    }
}

GateRef TSInlineLowering::GetAccessorReceiver(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    if (UNLIKELY(ecmaOpcode == EcmaOpcode::STTHISBYNAME_IMM8_ID16 ||
                 ecmaOpcode == EcmaOpcode::STTHISBYNAME_IMM16_ID16)) {
        return argAcc_.GetFrameArgsIn(gate, FrameArgIdx::THIS_OBJECT);
    }
    return acc_.GetValueIn(gate, 2); // 2: receiver
}

GateRef TSInlineLowering::GetCallSetterValue(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    if (ecmaOpcode == EcmaOpcode::STTHISBYNAME_IMM8_ID16 ||
        ecmaOpcode == EcmaOpcode::STTHISBYNAME_IMM16_ID16) {
        return acc_.GetValueIn(gate, 2); // 2: value
    }
    return acc_.GetValueIn(gate, 3); // 3: value
}

GateRef TSInlineLowering::GetFrameState(CallGateInfo &info)
{
    GateRef gate = info.GetCallGate();
    if (info.IsNormalCall()) {
        return acc_.GetFrameState(gate);
    }
    ASSERT(info.IsCallAccessor());
    GateRef frameState = acc_.FindNearestFrameState(gate);
    return frameState;
}

GateRef TSInlineLowering::GetFrameArgs(CallGateInfo &info)
{
    GateRef frameState = GetFrameState(info);
    return acc_.GetValueIn(frameState);
}

void TSInlineLowering::SetInitCallTargetAndConstPoolId(CallGateInfo &info)
{
    if (initCallTarget_ == Circuit::NullGate()) {
        GateRef frameArgs = GetFrameArgs(info);
        initCallTarget_ = acc_.GetValueIn(frameArgs, 0);
        const JSPandaFile *pf = ctx_->GetJSPandaFile();
        initConstantPoolId_ = tsManager_->GetConstantPoolIDByMethodOffset(pf, initMethodOffset_);
    }
}

void TSInlineLowering::AnalyseFastAccessor(CallGateInfo &info, std::vector<const uint8_t*> pcOffsets,
                                           uint32_t inlineMethodOffset)
{
    isFastAccessor_ = false;
    if (!info.IsCallAccessor()) {
        return;
    }
    const JSPandaFile *pf = ctx_->GetJSPandaFile();
    int32_t constantpoolId = tsManager_->GetConstantPoolIDByMethodOffset(pf, inlineMethodOffset);
    if (constantpoolId == initConstantPoolId_) {
        for (size_t i = 0; i < pcOffsets.size(); i++) {
            auto pc = pcOffsets[i];
            auto ecmaOpcode = ctx_->GetByteCodes()->GetOpcode(pc);
            // These bytecodes require calltarget during the lowering process, so the acquisition of the accessor
            // function cannot be omitted.
            switch (ecmaOpcode) {
                case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
                case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
                case EcmaOpcode::STMODULEVAR_IMM8:
                case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
                case EcmaOpcode::DYNAMICIMPORT:
                case EcmaOpcode::LDLOCALMODULEVAR_IMM8:
                case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
                case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
                case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
                case EcmaOpcode::GETMODULENAMESPACE_IMM8:
                case EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
                case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
                case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
                case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
                case EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
                case EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
                case EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8:
                case EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8:
                case EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
                case EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
                case EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
                case EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8:
                case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
                case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
                case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
                case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
                case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
                case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
                    return;
                default:
                    break;
            }
        }
        isFastAccessor_ = true;
    }
}
}  // namespace panda::ecmascript
