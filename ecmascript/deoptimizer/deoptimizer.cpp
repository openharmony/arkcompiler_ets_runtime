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
#include "ecmascript/deoptimizer/deoptimizer.h"

#include <cmath>

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/gate_meta_data.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/frames.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"

namespace panda::ecmascript {
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

void Deoptimizier::CollectVregs(const std::vector<kungfu::ARKDeopt>& deoptBundle, size_t shift)
{
    deoptVregs_.clear();
    for (size_t i = 0; i < deoptBundle.size(); i++) {
        ARKDeopt deopt = deoptBundle.at(i);
        JSTaggedType v;
        VRegId id = deopt.id;
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

void Deoptimizier::CollectDeoptBundleVec(std::vector<ARKDeopt>& deoptBundle)
{
    JSTaggedType *lastLeave = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    FrameIterator it(lastLeave, thread_);
    // note: last deopt bridge frame is generated by DeoptHandlerAsm, callee Regs is grow from this frame
    for (; !it.Done() && deoptBundle.empty(); it.Advance<GCVisitedFlag::VISITED>()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                frame->GetDeoptBundleInfo(it, deoptBundle);
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

bool Deoptimizier::CollectVirtualRegisters(Method* method, FrameWriter *frameWriter, size_t curDepth)
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
    if (!method->IsFastCall() && declaredNumArgs != actualNumArgs) {
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

void Deoptimizier::Dump(Method* method, kungfu::DeoptType type, size_t depth)
{
    if (traceDeopt_) {
        std::string checkType = DisplayItems(type);
        LOG_COMPILER(INFO) << "Check Type: " << checkType;
        std::string data = JsStackInfo::BuildJsStackTrace(thread_, true);
        LOG_COMPILER(INFO) << "Deoptimize" << data;
        const uint8_t *pc = method->GetBytecodeArray() + pc_.at(depth);
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
//   |       returnAddr_        |          stackContext
//   |      callFrameTop_       |               |
//   |       inlineDepth        |               v
//   |--------------------------| ---------------

JSTaggedType Deoptimizier::ConstructAsmInterpretFrame()
{
    FrameWriter frameWriter(this);
    // Push asm interpreter frame
    for (int32_t curDepth = static_cast<int32_t>(inlineDepth_); curDepth >= 0; curDepth--) {
        auto start = frameWriter.GetTop();
        JSTaggedValue callTarget = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::FUNC_INDEX));
        auto method = GetMethod(callTarget);
        if (!CollectVirtualRegisters(method, &frameWriter, curDepth)) {
            return JSTaggedValue::Exception().GetRawData();
        }
        AsmInterpretedFrame *statePtr = frameWriter.ReserveAsmInterpretedFrame();
        const uint8_t *resumePc = method->GetBytecodeArray() + pc_.at(curDepth);
        JSTaggedValue thisObj = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::THIS_OBJECT_INDEX));
        auto acc = GetDeoptValue(curDepth, static_cast<int32_t>(SpecVregIndex::ACC_INDEX));
        statePtr->function = callTarget;
        statePtr->acc = acc;
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

    frameWriter.PushRawValue(stackContext_.callerFp_);
    frameWriter.PushRawValue(stackContext_.returnAddr_);
    frameWriter.PushRawValue(stackContext_.callFrameTop_);
    frameWriter.PushRawValue(inlineDepth_);
    return reinterpret_cast<JSTaggedType>(frameWriter.GetTop());
}

void Deoptimizier::UpdateAndDumpDeoptInfo(kungfu::DeoptType type)
{
    // depth records the number of layers of nested calls when deopt occurs
    for (size_t i = 0; i <= inlineDepth_; i++) {
        JSTaggedValue CallTarget = GetDeoptValue(i, static_cast<int32_t>(SpecVregIndex::FUNC_INDEX));
        auto method = GetMethod(CallTarget);
        if (i == inlineDepth_) {
            Dump(method, type, i);
        }
        ASSERT(thread_ != nullptr);
        uint8_t deoptThreshold = method->GetDeoptThreshold();
        if (deoptThreshold > 0) {
            method->SetDeoptType(type);
            method->SetDeoptThreshold(--deoptThreshold);
        } else {
            FunctionKind kind = method->GetFunctionKind();
            ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
            JSHandle<ECMAObject> jsFunc(thread_, CallTarget);
            JSHandle<JSHClass> oldHclass(thread_, jsFunc->GetClass());
            // instead of hclass by non_optimized hclass when method ClearAOTFlags
            JSHandle<JSHClass> newHClass = factory->GetNonOptimizedHclass(oldHclass, kind);
            jsFunc->SynchronizedSetClass(*newHClass);
        }
    }
}

// call instructions need compute jumpSize
size_t Deoptimizier::GetCallSize(size_t curDepth, const uint8_t *resumePc)
{
    if (inlineDepth_ > 0 && curDepth != inlineDepth_) {
        auto op = BytecodeInstruction(resumePc).GetOpcode();
        size_t jumpSize = BytecodeInstruction::Size(op);
        return jumpSize;
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
}  // namespace panda::ecmascript
