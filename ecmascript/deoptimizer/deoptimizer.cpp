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
#include "ecmascript/deoptimizer/deoptimizer.h"


#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/js_tagged_value_internals.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"

namespace panda::ecmascript {

extern "C" uintptr_t GetDeoptHandlerAsmOffset(bool isArch32)
{
    return JSThread::GlueData::GetRTStubEntriesOffset(isArch32) +
        RTSTUB_ID(DeoptHandlerAsm) * RuntimeStubs::RT_STUB_FUNC_SIZE;
}

extern "C" uintptr_t GetFixedReturnAddr(uintptr_t argGlue, uintptr_t prevCallSiteSp)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    uintptr_t fixed = thread->GetAndClearCallSiteReturnAddr(prevCallSiteSp);
    if (fixed == 0) {
        LOG_ECMA(FATAL) << "ilegal return addr found";
    }
    return fixed;
}

class FrameWriter {
public:
    explicit FrameWriter(Deoptimizier *deoptimizier) : thread_(deoptimizier->GetThread())
    {
        JSTaggedType *prevSp = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
        start_ = top_ = EcmaInterpreter::GetInterpreterFrameEnd(thread_, prevSp);
    }

    void PushValue(JSTaggedType value)
    {
        *(--top_) = value;
    }

    void PushRawValue(uintptr_t value)
    {
        *(--top_) = value;
    }

    bool Reserve(size_t size)
    {
        return !thread_->DoStackOverflowCheck(top_ - size);
    }

    AsmInterpretedFrame *ReserveAsmInterpretedFrame()
    {
        auto frame = AsmInterpretedFrame::GetFrameFromSp(top_);
        top_ = reinterpret_cast<JSTaggedType *>(frame);
        return frame;
    }

    JSTaggedType *GetStart() const
    {
        return start_;
    }

    JSTaggedType *GetTop() const
    {
        return top_;
    }

    JSTaggedType *GetFirstFrame() const
    {
        return firstFrame_;
    }

    void RecordFirstFrame()
    {
        firstFrame_ = top_;
    }

    void ReviseValueByIndex(JSTaggedType value, size_t index)
    {
        ASSERT(index < static_cast<size_t>(start_ - top_));
        *(top_ + index) = value;
    }

private:
    JSThread *thread_ {nullptr};
    JSTaggedType *start_ {nullptr};
    JSTaggedType *top_ {nullptr};
    JSTaggedType *firstFrame_ {nullptr};
};

Deoptimizier::Deoptimizier(JSThread *thread, size_t depth, kungfu::DeoptType type)
    : thread_(thread), inlineDepth_(depth), type_(static_cast<uint32_t>(type))
{
    CalleeReg callreg;
    numCalleeRegs_ = static_cast<size_t>(callreg.GetCallRegNum());
    JSRuntimeOptions options = thread_->GetEcmaVM()->GetJSOptions();
    traceDeopt_ = options.GetTraceDeopt();
}

void Deoptimizier::CollectVregs(const std::vector<kungfu::ARKDeopt>& deoptBundle, size_t shift)
{
    deoptVregs_.clear();
    for (size_t i = 0; i < deoptBundle.size(); i++) {
        ARKDeopt deopt = deoptBundle.at(i);
        JSTaggedType v;
        VRegId id = deopt.id;
        if (static_cast<OffsetType>(id) == static_cast<OffsetType>(SpecVregIndex::INLINE_DEPTH)) {
            continue;
        }
        if (std::holds_alternative<DwarfRegAndOffsetType>(deopt.value)) {
            ASSERT(deopt.kind == LocationTy::Kind::INDIRECT);
            auto value = std::get<DwarfRegAndOffsetType>(deopt.value);
            DwarfRegType dwarfReg = value.first;
            OffsetType offset = value.second;
            ASSERT (dwarfReg == GCStackMapRegisters::FP || dwarfReg == GCStackMapRegisters::SP);
            uintptr_t addr;
            if (dwarfReg == GCStackMapRegisters::SP) {
                addr = context_.callsiteSp + offset;
            } else {
                addr = context_.callsiteFp + offset;
            }
            v = *(reinterpret_cast<JSTaggedType *>(addr));
        } else if (std::holds_alternative<LargeInt>(deopt.value)) {
            ASSERT(deopt.kind == LocationTy::Kind::CONSTANTNDEX);
            v = JSTaggedType(static_cast<int64_t>(std::get<LargeInt>(deopt.value)));
        } else {
            ASSERT(std::holds_alternative<IntType>(deopt.value));
            ASSERT(deopt.kind == LocationTy::Kind::CONSTANT);
            v = JSTaggedType(static_cast<int64_t>(std::get<IntType>(deopt.value)));
        }
        size_t curDepth = DecodeDeoptDepth(id, shift);
        OffsetType vregId = static_cast<OffsetType>(DecodeVregIndex(id, shift));
        if (vregId != static_cast<OffsetType>(SpecVregIndex::PC_OFFSET_INDEX)) {
            deoptVregs_.insert({{curDepth, vregId}, JSHandle<JSTaggedValue>(thread_, JSTaggedValue(v))});
        } else {
            pc_.insert({curDepth, static_cast<size_t>(v)});
        }
    }
}

// when AOT trigger deopt, frame layout as the following
// * OptimizedJSFunctionFrame layout description as the following:
//               +--------------------------+ ---------------
//               |        ......            |               ^
//               |        ......            |       callerFunction
//               |        ......            |               |
//               |--------------------------|               |
//               |        args              |               v
//               +--------------------------+ ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   OptimizedJSFunction  FrameType:OPTIMIZED_JS_FUNCTION_FRAME
//               |       frameType          |               |
//               |--------------------------|               |
//               |       call-target        |               |
//               |--------------------------|               |
//               |       lexEnv             |               |
//               |--------------------------|               |
//               |       ...........        |               v
//               +--------------------------+ ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   __llvm_deoptimize  FrameType:OPTIMIZED_FRAME
//               |       frameType          |               |
//               |--------------------------|               |
//               |       No CalleeSave      |               |
//               |       Registers          |               v
//               +--------------------------+ ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|   DeoptHandlerAsm  FrameType:ASM_BRIDGE_FRAME
//               |       frameType          |               |
//               |--------------------------|               |
//               |       glue               |               |
//               |--------------------------|               |
//               | CalleeSave Registers     |               v
//               +--------------------------+ ---------------
//               |       .........          |               ^
//               |       .........          |     CallRuntime   FrameType:LEAVE_FRAME
//               |       .........          |               |
//               |       .........          |               v
//               |--------------------------| ---------------

// After gathering the necessary information(After Call Runtime), frame layout after constructing
// asminterpreterframe is shown as the following:
//               +----------------------------------+---------+
//               |        ......                    |         ^
//               |        ......                    |   callerFunction
//               |        ......                    |         |
//               |----------------------------------|         |
//               |        args                      |         v
//               +----------------------------------+---------+
//               |       returnAddr                 |         ^
//               |----------------------------------|         |
//               |       frameType                  |         |
//               |----------------------------------|    ASM_INTERPRETER_BRIDGE_FRAME
//               |       callsiteFp                 |         |
//               |----------------------------------|         |
//               |       ...........                |         v
//               +----------------------------------+---------+
//               |       returnAddr                 |
//               |----------------------------------|
//               |    argv[n-1]                     |
//               |----------------------------------|
//               |    ......                        |
//               |----------------------------------|
//               |    thisArg [maybe not exist]     |
//               |----------------------------------|
//               |    newTarget [maybe not exist]   |
//               |----------------------------------|
//               |    ......                        |
//               |----------------------------------|
//               |    Vregs [not exist in native]   |
//               +----------------------------------+--------+
//               |        .  .  .   .               |        ^
//               |     InterpretedFrameBase         |        |
//               |        .  .  .   .               |        |
//               |----------------------------------|        |
//               |    pc(bytecode addr)             |        |
//               |----------------------------------|        |
//               |    sp(current stack pointer)     |        |
//               |----------------------------------|   AsmInterpretedFrame 0
//               |    callSize                      |        |
//               |----------------------------------|        |
//               |    env                           |        |
//               |----------------------------------|        |
//               |    acc                           |        |
//               |----------------------------------|        |
//               |    thisObj                       |        |
//               |----------------------------------|        |
//               |    call-target                   |        v
//               +----------------------------------+--------+
//               |    argv[n-1]                     |
//               |----------------------------------|
//               |    ......                        |
//               |----------------------------------|
//               |    thisArg [maybe not exist]     |
//               |----------------------------------|
//               |    newTarget [maybe not exist]   |
//               |----------------------------------|
//               |    ......                        |
//               |----------------------------------|
//               |    Vregs [not exist in native]   |
//               +----------------------------------+--------+
//               |        .  .  .   .               |        ^
//               |     InterpretedFrameBase         |        |
//               |        .  .  .   .               |        |
//               |----------------------------------|        |
//               |    pc(bytecode addr)             |        |
//               |----------------------------------|        |
//               |    sp(current stack pointer)     |        |
//               |----------------------------------|   AsmInterpretedFrame 1
//               |    callSize                      |        |
//               |----------------------------------|        |
//               |    env                           |        |
//               |----------------------------------|        |
//               |    acc                           |        |
//               |----------------------------------|        |
//               |    thisObj                       |        |
//               |----------------------------------|        |
//               |    call-target                   |        v
//               +----------------------------------+--------+
//               |        .  .  .   .               |        ^
//               |        .  .  .   .               |   AsmInterpretedFrame n
//               |        .  .  .   .               |        v
//               +----------------------------------+--------+

template<class T>
void Deoptimizier::AssistCollectDeoptBundleVec(FrameIterator &it, T &frame)
{
    CalleeRegAndOffsetVec calleeRegInfo;
    frame->GetFuncCalleeRegAndOffset(it, calleeRegInfo);
    context_.calleeRegAndOffset = calleeRegInfo;
    context_.callsiteSp = it.GetCallSiteSp();
    context_.callsiteFp = reinterpret_cast<uintptr_t>(it.GetSp());
    auto preFrameSp = frame->ComputePrevFrameSp(it);
    frameArgc_ = frame->GetArgc(preFrameSp);
    frameArgvs_ = frame->GetArgv(preFrameSp);
    stackContext_.callFrameTop_ = it.GetPrevFrameCallSiteSp();
    stackContext_.returnAddr_ = frame->GetReturnAddr();
    stackContext_.callerFp_ = reinterpret_cast<uintptr_t>(frame->GetPrevFrameFp());
    stackContext_.isFrameLazyDeopt_ = it.IsLazyDeoptFrameType();
}

void Deoptimizier::CollectDeoptBundleVec(std::vector<ARKDeopt>& deoptBundle)
{
    JSTaggedType *lastLeave = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    FrameIterator it(lastLeave, thread_);
    // note: last deopt bridge frame is generated by DeoptHandlerAsm, callee Regs is grow from this frame
    for (; !it.Done() && deoptBundle.empty(); it.Advance<GCVisitedFlag::DEOPT>()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                frame->GetDeoptBundleInfo(it, deoptBundle);
                AssistCollectDeoptBundleVec(it, frame);
                break;
            }
            case FrameType::FASTJIT_FUNCTION_FRAME:
            case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
                auto frame = it.GetFrame<FASTJITFunctionFrame>();
                frame->GetDeoptBundleInfo(it, deoptBundle);
                AssistCollectDeoptBundleVec(it, frame);
                break;
            }
            case FrameType::ASM_BRIDGE_FRAME: {
                auto sp = reinterpret_cast<uintptr_t*>(it.GetSp());
                static constexpr size_t TYPE_GLUE_SLOT = 2; // 2: skip type & glue
                sp -= TYPE_GLUE_SLOT;
                calleeRegAddr_ = sp - numCalleeRegs_;
                break;
            }
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::LEAVE_FRAME:
                break;
            default: {
                LOG_FULL(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    }
    ASSERT(!it.Done());
}

Method* Deoptimizier::GetMethod(JSTaggedValue &target)
{
    ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(target.GetTaggedObject());
    ASSERT(callTarget != nullptr);
    Method *method = callTarget->GetCallTarget();
    return method;
}

void Deoptimizier::RelocateCalleeSave()
{
    CalleeReg callreg;
    for (auto &it: context_.calleeRegAndOffset) {
        auto reg = it.first;
        auto offset = it.second;
        uintptr_t value = *(reinterpret_cast<uintptr_t *>(context_.callsiteFp + offset));
        int order = callreg.FindCallRegOrder(reg);
        calleeRegAddr_[order] = value;
    }
}

bool Deoptimizier::CollectVirtualRegisters(JSTaggedValue callTarget, Method *method, FrameWriter *frameWriter,
    size_t curDepth)
{
    int32_t actualNumArgs = 0;
    int32_t declaredNumArgs = 0;
    if (curDepth == 0) {
        actualNumArgs = static_cast<int32_t>(GetDeoptValue(curDepth,
            static_cast<int32_t>(SpecVregIndex::ACTUAL_ARGC_INDEX)).GetInt());
        declaredNumArgs = static_cast<int32_t>(method->GetNumArgsWithCallField());
    } else {
        // inline method actualNumArgs equal to declaredNumArgs
        actualNumArgs = static_cast<int32_t>(method->GetNumArgsWithCallField());
        declaredNumArgs = static_cast<int32_t>(method->GetNumArgsWithCallField());
    }

    int32_t callFieldNumVregs = static_cast<int32_t>(method->GetNumVregsWithCallField());

    // layout of frame:
    // [maybe argc] [actual args] [reserved args] [call field virtual regs]

    // [maybe argc]
    bool isFastCall = JSFunctionBase::IsFastCallFromCallTarget(callTarget);
    if (!isFastCall && declaredNumArgs != actualNumArgs) {
        auto value = JSTaggedValue(actualNumArgs);
        frameWriter->PushValue(value.GetRawData());
    }
    int32_t virtualIndex = declaredNumArgs + callFieldNumVregs +
        static_cast<int32_t>(method->GetNumRevervedArgs()) - 1;
    if (!frameWriter->Reserve(static_cast<size_t>(virtualIndex))) {
        return false;
    }
    for (int32_t i = static_cast<int32_t>(declaredNumArgs - 1); i >= 0; i--) {
        JSTaggedValue value = JSTaggedValue::Undefined();
        // deopt value
        if (HasDeoptValue(curDepth, virtualIndex)) {
            value = GetDeoptValue(curDepth, virtualIndex);
        }
        frameWriter->PushValue(value.GetRawData());
        virtualIndex--;
    }

    // [reserved args]
    if (method->HaveThisWithCallField()) {
        JSTaggedValue value = deoptVregs_.at(
            {curDepth, static_cast<OffsetType>(SpecVregIndex::THIS_OBJECT_INDEX)}).GetTaggedValue();
        frameWriter->PushValue(value.GetRawData());
        virtualIndex--;
    }
    if (method->HaveNewTargetWithCallField()) {
        JSTaggedValue value = deoptVregs_.at(
            {curDepth, static_cast<OffsetType>(SpecVregIndex::NEWTARGET_INDEX)}).GetTaggedValue();
        frameWriter->PushValue(value.GetRawData());
        virtualIndex--;
    }
    if (method->HaveFuncWithCallField()) {
        JSTaggedValue value = deoptVregs_.at(
            {curDepth, static_cast<OffsetType>(SpecVregIndex::FUNC_INDEX)}).GetTaggedValue();
        frameWriter->PushValue(value.GetRawData());
        virtualIndex--;
    }

    // [call field virtual regs]
    for (int32_t i = virtualIndex; i >= 0; i--) {
        JSTaggedValue value = GetDeoptValue(curDepth, virtualIndex);
        frameWriter->PushValue(value.GetRawData());
        virtualIndex--;
    }
    // revise correct a0 - aN virtual regs , for example: ldobjbyname key; sta a2; update value to a2
    //         +--------------------------+            ^
    //         |       aN                 |            |
    //         +--------------------------+            |
    //         |       ...                |            |
    //         +--------------------------+            |
    //         |       a2(this)           |            |
    //         +--------------------------+   revise correct vreg
    //         |       a1(newtarget)      |            |
    //         +--------------------------+            |
    //         |       a0(func)           |            |
    //         |--------------------------|            v
    //         |       v0 - vN            |
    //  sp --> |--------------------------|
    int32_t vregsAndArgsNum = declaredNumArgs + callFieldNumVregs +
        static_cast<int32_t>(method->GetNumRevervedArgs());
    for (int32_t i = callFieldNumVregs; i < vregsAndArgsNum; i++) {
        JSTaggedValue value = JSTaggedValue::Undefined();
        if (HasDeoptValue(curDepth, i)) {
            value = GetDeoptValue(curDepth, i);
            frameWriter->ReviseValueByIndex(value.GetRawData(), i);
        }
    }
    return true;
}

void Deoptimizier::Dump(JSTaggedValue callTarget, kungfu::DeoptType type, size_t depth)
{
    if (thread_->IsPGOProfilerEnable()) {
        JSFunction *function = JSFunction::Cast(callTarget);
        auto profileTypeInfo = function->GetProfileTypeInfo();
        if (profileTypeInfo.IsUndefined()) {
            SlowRuntimeStub::NotifyInlineCache(thread_, function);
        }
    }
    if (traceDeopt_) {
        std::string checkType = DisplayItems(type);
        LOG_TRACE(INFO) << "Check Type: " << checkType;
        std::string data = JsStackInfo::BuildJsStackTrace(thread_, true);
        LOG_COMPILER(INFO) << "Deoptimize" << data;
        const uint8_t *pc = GetMethod(callTarget)->GetBytecodeArray() + pc_.at(depth);
        BytecodeInstruction inst(pc);
        LOG_COMPILER(INFO) << inst;
    }
}

std::string Deoptimizier::DisplayItems(DeoptType type)
{
    const std::map<DeoptType, const char *> strMap = {
#define DEOPT_NAME_MAP(NAME, TYPE) {DeoptType::TYPE, #NAME},
        GATE_META_DATA_DEOPT_REASON(DEOPT_NAME_MAP)
#undef DEOPT_NAME_MAP
    };
    if (strMap.count(type) > 0) {
        return strMap.at(type);
    }
    return "DeoptType-" + std::to_string(static_cast<uint8_t>(type));
}

// layout of frameWriter
//   |--------------------------| --------------> start(n)
//   |          args            |
//   |          this            |
//   |        newTarget         |
//   |       callTarget         |
//   |          vregs           |
//   |---------------------------
//   |       ASM Interpreter    |
//   +--------------------------+ --------------> end(n)
//   |        outputcounts      |          outputcounts = end(n) - start(n)
//   |--------------------------| --------------> start(n-1)
//   |          args            |
//   |          this            |
//   |        newTarget         |
//   |       callTarget         |
//   |          vregs           |
//   |-------------------------------------------
//   |       ASM Interpreter    |
//   +--------------------------+ --------------> end(n-1)
//   |        outputcounts      |           outputcounts = end(n-1) - start(n-1)
//   |--------------------------| --------------> start(n-1)
//   |       ......             |
//   +--------------------------+ ---------------
//   |        callerFp_         |               ^
//   |       returnAddr_        |               |
//   |      callFrameTop_       |          stackContext
//   |       inlineDepth_       |               |
//   |       hasException_      |               |
//   |      isFrameLazyDeopt_   |               v
//   |--------------------------| ---------------

JSTaggedType Deoptimizier::ConstructAsmInterpretFrame(JSHandle<JSTaggedValue> maybeAcc)
{
    FrameWriter frameWriter(this);
    // Push asm interpreter frame
    for (int32_t curDepth = static_cast<int32_t>(inlineDepth_); curDepth >= 0; curDepth--) {
        auto start = frameWriter.GetTop();
        JSTaggedValue callTarget = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::FUNC_INDEX));
        auto method = GetMethod(callTarget);
        if (!CollectVirtualRegisters(callTarget, method, &frameWriter, curDepth)) {
            return JSTaggedValue::Exception().GetRawData();
        }
        AsmInterpretedFrame *statePtr = frameWriter.ReserveAsmInterpretedFrame();
        const uint8_t *resumePc = method->GetBytecodeArray() + pc_.at(curDepth);
        JSTaggedValue thisObj = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::THIS_OBJECT_INDEX));
        JSTaggedValue acc = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::ACC_INDEX));
        statePtr->function = callTarget;
        statePtr->acc = acc;

        if (UNLIKELY(curDepth == static_cast<int32_t>(inlineDepth_) &&
            type_ == static_cast<uint32_t>(kungfu::DeoptType::LAZYDEOPT))) {
            ProcessLazyDeopt(maybeAcc, resumePc, statePtr);
        }

        statePtr->env = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::ENV_INDEX));
        statePtr->callSize = GetCallSize(curDepth, resumePc);
        statePtr->fp = 0;  // need update
        statePtr->thisObj = thisObj;
        statePtr->pc = resumePc;
        // -uintptr_t skip lr
        if (curDepth == 0) {
            statePtr->base.prev = reinterpret_cast<JSTaggedType *>(stackContext_.callFrameTop_ - sizeof(uintptr_t));
        } else {
            statePtr->base.prev = 0; // need update
        }

        statePtr->base.type = FrameType::ASM_INTERPRETER_FRAME;

        // construct stack context
        auto end = frameWriter.GetTop();
        auto outputCount = start - end;
        frameWriter.PushRawValue(outputCount);
    }

    RelocateCalleeSave();

    frameWriter.PushRawValue(stackContext_.isFrameLazyDeopt_);
    frameWriter.PushRawValue(thread_->HasPendingException());
    frameWriter.PushRawValue(stackContext_.callerFp_);
    frameWriter.PushRawValue(stackContext_.returnAddr_);
    frameWriter.PushRawValue(stackContext_.callFrameTop_);
    frameWriter.PushRawValue(inlineDepth_);
    return reinterpret_cast<JSTaggedType>(frameWriter.GetTop());
}

// static
void Deoptimizier::ResetJitHotness(JSThread *thread, JSFunction *jsFunc)
{
    if (jsFunc->GetMachineCode().IsMachineCodeObject()) {
        JSTaggedValue profileTypeInfoVal = jsFunc->GetProfileTypeInfo();
        if (!profileTypeInfoVal.IsUndefined()) {
            ProfileTypeInfo *profileTypeInfo = ProfileTypeInfo::Cast(profileTypeInfoVal.GetTaggedObject());
            profileTypeInfo->SetJitHotnessCnt(0);
            constexpr uint16_t thresholdStep = 4;
            constexpr uint16_t thresholdLimit = ProfileTypeInfo::JIT_DISABLE_FLAG / thresholdStep;
            uint16_t threshold = profileTypeInfo->GetJitHotnessThreshold();
            threshold = threshold >= thresholdLimit ? ProfileTypeInfo::JIT_DISABLE_FLAG : threshold * thresholdStep;
            profileTypeInfo->SetJitHotnessThreshold(threshold);
            ProfileTypeInfoCell::Cast(jsFunc->GetRawProfileTypeInfo())->SetMachineCode(thread, JSTaggedValue::Hole());
            Method *method = Method::Cast(jsFunc->GetMethod().GetTaggedObject());
            LOG_JIT(DEBUG) << "reset jit hotness for func: " << method->GetMethodName() << ", threshold:" << threshold;
        }
    }
}

// static
void Deoptimizier::ClearCompiledCodeStatusWhenDeopt(JSThread *thread, JSFunction *func,
                                                    Method *method, kungfu::DeoptType type)
{
    method->SetDeoptType(type);
    if (func->GetMachineCode().IsMachineCodeObject()) {
        Jit::GetInstance()->GetJitDfx()->SetJitDeoptCount();
    }
    if (func->IsCompiledCode()) {
        bool isFastCall = func->IsCompiledFastCall();  // get this flag before clear it
        uintptr_t entry =
            isFastCall ? thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_FastCallToAsmInterBridge)
                       : thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_AOTCallToAsmInterBridge);
        func->SetCodeEntry(entry);
        method->ClearAOTStatusWhenDeopt(entry);
        func->ClearCompiledCodeFlags();
        ResetJitHotness(thread, func);
        func->ClearMachineCode(thread);
    }  // Do not change the func code entry if the method is not aot or deopt has happened already
}

void Deoptimizier::UpdateAndDumpDeoptInfo(kungfu::DeoptType type)
{
    // depth records the number of layers of nested calls when deopt occurs
    for (size_t i = 0; i <= inlineDepth_; i++) {
        JSTaggedValue callTarget = GetDeoptValue(i, static_cast<int32_t>(SpecVregIndex::FUNC_INDEX));
        auto func = JSFunction::Cast(callTarget.GetTaggedObject());
        if (func->GetMachineCode().IsMachineCodeObject()) {
            MachineCode *machineCode = MachineCode::Cast(func->GetMachineCode().GetTaggedObject());
            if (type != kungfu::DeoptType::OSRLOOPEXIT &&
                machineCode->GetOSROffset() != MachineCode::INVALID_OSR_OFFSET) {
                machineCode->SetOsrDeoptFlag(true);
            }
        }
        auto method = GetMethod(callTarget);
        if (i == inlineDepth_) {
            Dump(callTarget, type, i);
        }
        ASSERT(thread_ != nullptr);
        uint8_t deoptThreshold = method->GetDeoptThreshold();
        if (deoptThreshold > 0) {
            method->SetDeoptType(type);
            method->SetDeoptThreshold(--deoptThreshold);
        } else {
            ClearCompiledCodeStatusWhenDeopt(thread_, func, method, type);
        }
    }
}

// call instructions need compute jumpSize
int64_t Deoptimizier::GetCallSize(size_t curDepth, const uint8_t *resumePc)
{
    if (inlineDepth_ > 0 && curDepth != inlineDepth_) {
        auto op = BytecodeInstruction(resumePc).GetOpcode();
        return InterpreterAssembly::GetCallSize(op);
    }
    return 0;
}

int32_t Deoptimizier::EncodeDeoptVregIndex(int32_t index, size_t depth, size_t shift)
{
    if (index >= 0) {
        return (index << shift) | depth;
    }
    return -((-index << shift) | depth);
}

size_t Deoptimizier::ComputeShift(size_t depth)
{
    size_t shift = 0;
    if (depth != 0) {
        shift = std::floor(std::log2(depth)) + 1;
    }
    return shift;
}

int32_t Deoptimizier::DecodeVregIndex(OffsetType id, size_t shift)
{
    if (id >= 0) {
        return id >> shift;
    }
    return -((-id) >> shift);
}

size_t Deoptimizier::DecodeDeoptDepth(OffsetType id, size_t shift)
{
    size_t mask = (1 << shift) - 1;
    if (id >= 0) {
        return id & mask;
    }
    return (-id) & mask;
}

// static
size_t Deoptimizier::GetInlineDepth(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::VISITED>()) {
        if (!it.IsOptimizedJSFunctionFrame()) {
            continue;
        }
        return it.GetInlineDepth();
    }
    return 0;
}

// static
void Deoptimizier::ReplaceReturnAddrWithLazyDeoptTrampline(JSThread *thread,
                                                           uintptr_t *returnAddraddress,
                                                           FrameType *prevFrameTypeAddress,
                                                           uintptr_t prevFrameCallSiteSp)
{
    ASSERT(returnAddraddress != nullptr);
    uintptr_t lazyDeoptTrampoline = thread->GetRTInterface(kungfu::RuntimeStubCSigns::ID_LazyDeoptEntry);
    uintptr_t oldPc = *returnAddraddress;
    *returnAddraddress = lazyDeoptTrampoline;
    ASSERT(oldPc != 0);
    ASSERT(oldPc != lazyDeoptTrampoline);
    thread->AddToCallsiteSpToReturnAddrTable(prevFrameCallSiteSp, oldPc);

    FrameIterator::DecodeAsLazyDeoptFrameType(prevFrameTypeAddress);
}

// static
bool Deoptimizier::IsNeedLazyDeopt(const FrameIterator &it)
{
    if (!it.IsOptimizedJSFunctionFrame()) {
        return false;
    }
    auto function = it.GetFunction();
    return function.CheckIsJSFunctionBase() && !JSFunction::Cast(function)->IsCompiledCode();
}

// static
void Deoptimizier::PrepareForLazyDeopt(JSThread *thread)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    uintptr_t *prevReturnAddrAddress = nullptr;
    FrameType *prevFrameTypeAddress;
    uintptr_t prevFrameCallSiteSp = 0;
    for (; !it.Done(); it.Advance<GCVisitedFlag::VISITED>()) {
        if (IsNeedLazyDeopt(it)) {
            ReplaceReturnAddrWithLazyDeoptTrampline(
                thread, prevReturnAddrAddress, prevFrameTypeAddress, prevFrameCallSiteSp);
        }
        prevReturnAddrAddress = it.GetReturnAddrAddress();
        prevFrameTypeAddress = it.GetFrameTypeAddress();
        prevFrameCallSiteSp = it.GetPrevFrameCallSiteSp();
    }
}

/**
 * [Lazy Deoptimization Handling]
 * This scenario specifically occurs during lazy deoptimization.
 *
 * Typical Trigger:
 * When bytecode operations (LDOBJBYNAME) invoke accessors that induce
 * lazy deoptimization of the current function's caller.
 *
 * Key Differences from Eager Deoptimization:
 * Lazy deoptimization happens *after* bytecode execution completes.
 * When return to DeoptHandlerAsm:
 * 1. Bytecode processing remains incomplete
 * 2. Post-processing must handle:
 *    a. Program Counter (PC) adjustment
 *    b. Accumulator (ACC) state overwrite
 *    c. Handling pending exceptions
 *
 * Critical Constraint:
 * Any JIT-compiled call that may trigger lazy deoptimization MUST be the final
 * call in this bytecode.
 * This ensures no intermediate state remains after lazy deoptimization.
 */
void Deoptimizier::ProcessLazyDeopt(JSHandle<JSTaggedValue> maybeAcc, const uint8_t* &resumePc,
                                    AsmInterpretedFrame *statePtr)
{
    if (NeedOverwriteAcc(resumePc)) {
        statePtr->acc = maybeAcc.GetTaggedValue();
    }
    
    // Todo: add check constructor

    if (!thread_->HasPendingException()) {
        EcmaOpcode curOpcode = kungfu::Bytecodes::GetOpcode(resumePc);
        // Avoid adding the PC when a pending exception exists.
        // Prevents ExceptionHandler from failing to identify try-catch blocks.
        resumePc += (BytecodeInstruction::Size(curOpcode));
    }
}

bool Deoptimizier::NeedOverwriteAcc(const uint8_t *pc) const
{
    BytecodeInstruction inst(pc);
    if (inst.HasFlag(BytecodeInstruction::Flags::ACC_WRITE)) {
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
