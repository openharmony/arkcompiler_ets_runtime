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

namespace panda::ecmascript::kungfu {
void TSInlineLowering::RunTSInlineLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            TryInline(gate);
        }
    }
}

void TSInlineLowering::TryInline(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::CALLARG0_IMM8:
        case EcmaOpcode::CALLARG1_IMM8_V8:
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            TryInline(gate, false);
            break;
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
            TryInline(gate, true);
            break;
        default:
            break;
    }
}

void TSInlineLowering::TryInline(GateRef gate, bool isCallThis)
{
    // first elem is function in old isa
    size_t funcIndex = acc_.GetNumValueIn(gate) - 1;
    auto funcType = acc_.GetGateType(acc_.GetValueIn(gate, funcIndex));
    MethodLiteral* inlinedMethod = nullptr;
    if (tsManager_->IsFunctionTypeKind(funcType)) {
        GlobalTSTypeRef gt = funcType.GetGTRef();
        auto methodOffset = tsManager_->GetFuncMethodOffset(gt);
        if (methodOffset == 0 || ctx_->IsSkippedMethod(methodOffset)) {
            return;
        }
        inlinedMethod = ctx_->GetJSPandaFile()->FindMethodLiteral(methodOffset);
        if (!CheckParameter(gate, isCallThis, inlinedMethod)) {
            return;
        }
        auto &bytecodeInfo = ctx_->GetBytecodeInfo();
        auto &methodInfo = bytecodeInfo.GetMethodList().at(methodOffset);
        auto &methodPcInfos = bytecodeInfo.GetMethodPcInfos();
        auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
        if (methodPcInfo.pcOffsets.size() <= MAX_INLINE_BYTECODE_COUNT &&
            inlinedCall_ <= MAX_INLINE_CALL_ALLOWED) {
            auto success = FilterInlinedMethod(inlinedMethod, methodPcInfo.pcOffsets);
            if (success) {
                CircuitRootScope scope(circuit_);
                InlineCall(methodInfo, methodPcInfo, inlinedMethod);
                ReplaceCallInput(gate, isCallThis);
                inlinedCall_++;
            }
        }
    }

    if ((inlinedMethod != nullptr) && IsLogEnabled()) {
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

void TSInlineLowering::InlineCall(MethodInfo &methodInfo, MethodPcInfo &methodPCInfo, MethodLiteral* method)
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
                                   hasTyps, fullName, recordName, nullptr);
    {
        TimeScope timeScope("BytecodeToCircuit", methodName, method->GetMethodId().GetOffset(), log);
        builder.BytecodeToCircuit();
    }

    PassData data(&builder, circuit_, ctx_, log, fullName,
                  &methodInfo, hasTyps, recordName,
                  method, method->GetMethodId().GetOffset());
    PassRunner<PassData> pipeline(&data);
    pipeline.RunPass<TypeInferPass>();
    pipeline.RunPass<AsyncFunctionLoweringPass>();
}

bool TSInlineLowering::CheckParameter(GateRef gate, bool isCallThis, MethodLiteral* method)
{
    size_t numIns = acc_.GetNumValueIn(gate);
    size_t fixedInputsNum = isCallThis ? 2 : 1; // 2: calltarget and this

    uint32_t declaredNumArgs = method->GetNumArgsWithCallField();
    return declaredNumArgs == (numIns - fixedInputsNum);
}

void TSInlineLowering::ReplaceCallInput(GateRef gate, bool isCallThis)
{
    std::vector<GateRef> vec;
    GateRef glue = acc_.GetGlueFromArgList();
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
    vec.emplace_back(builder_.Int64(actualArgc)); // argc
    vec.emplace_back(callTarget);
    vec.emplace_back(builder_.Undefined()); // newTarget
    vec.emplace_back(thisObj);
    // -1: call Target
    for (size_t i = fixedInputsNum - 1; i < numIns - 1; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    LowerToInlineCall(gate, vec);
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

void TSInlineLowering::ReplaceEntryGate(GateRef callGate)
{
    auto stateEntry = acc_.GetStateRoot();
    auto dependEntry = acc_.GetDependRoot();

    GateRef callState = acc_.GetState(callGate);
    GateRef callDepend = acc_.GetDep(callGate);
    auto stateUse = acc_.Uses(stateEntry);
    for (auto stateUseIt = stateUse.begin(); stateUseIt != stateUse.end();) {
        stateUseIt = acc_.ReplaceIn(stateUseIt, callState);
    }

    auto dependUse = acc_.Uses(dependEntry);
    for (auto dependUseIt = dependUse.begin(); dependUseIt != dependUse.end();) {
        dependUseIt = acc_.ReplaceIn(dependUseIt, callDepend);
    }
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

void TSInlineLowering::LowerToInlineCall(GateRef callGate, const std::vector<GateRef> &args)
{
    // replace in value/args
    ArgumentAccessor argAcc(circuit_);
    ASSERT(argAcc.ArgsCount() == args.size());
    for (size_t i = 0; i < argAcc.ArgsCount(); i++) {
        GateRef arg = argAcc.ArgsAt(i);
        acc_.UpdateAllUses(arg, args.at(i));
        acc_.DeleteGate(arg);
    }
    // replace in depend and state
    ReplaceEntryGate(callGate);
    // replace use gate
    ReplaceReturnGate(callGate);
    // remove Useless root gates
    RemoveRoot();
}

void TSInlineLowering::RemoveRoot()
{
    GateRef circuitRoot = acc_.GetRoot(OpCode::CIRCUIT_ROOT);
    auto uses = acc_.Uses(circuitRoot);
    for (auto it = uses.begin(); it != uses.end();) {
        it = acc_.DeleteGate(it);
    }

    acc_.DeleteGate(circuitRoot);
}
}  // namespace panda::ecmascript
