/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#include "deoptimizer.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/frames.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"

namespace panda::ecmascript {
using DeoptFromAOTType = JSTaggedType (*)(uintptr_t glue, JSTaggedType* sp,
    uintptr_t vregsSp, uintptr_t stateSp, JSTaggedType callTarget);


class FrameWriter {
public:
    FrameWriter(Deoptimizier *deoptimizier) : thread_(deoptimizier->GetThread())
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

private:
    JSThread *thread_ {nullptr};
    JSTaggedType *start_;
    JSTaggedType *top_;
};

void Deoptimizier::CollectVregs(const std::vector<kungfu::ARKDeopt>& deoptBundle)
{
    vregs_.clear();
    for (size_t i = 0; i < deoptBundle.size(); i++) {
        kungfu::ARKDeopt deopt = deoptBundle.at(i);
        JSTaggedType v;
        kungfu::OffsetType id = deopt.Id;
        if (std::holds_alternative<kungfu::DwarfRegAndOffsetType>(deopt.value)) {
            ASSERT(deopt.kind == kungfu::LocationTy::Kind::INDIRECT);
            auto value = std::get<kungfu::DwarfRegAndOffsetType>(deopt.value);
            kungfu::DwarfRegType dwarfReg = value.first;
            kungfu::OffsetType offset = value.second;
            ASSERT (dwarfReg == GCStackMapRegisters::FP || dwarfReg == GCStackMapRegisters::SP);
            uintptr_t addr;
            if (dwarfReg == GCStackMapRegisters::SP) {
                addr = context_.callsiteSp + offset;
            } else {
                addr = context_.callsiteFp + offset;
            }
            v = *(reinterpret_cast<JSTaggedType *>(addr));
        } else if (std::holds_alternative<kungfu::LargeInt>(deopt.value)) {
            ASSERT(deopt.kind == kungfu::LocationTy::Kind::CONSTANTNDEX);
            v = JSTaggedType(static_cast<int64_t>(std::get<kungfu::LargeInt>(deopt.value)));
        } else {
            ASSERT(std::holds_alternative<kungfu::OffsetType>(deopt.value));
            ASSERT(deopt.kind == kungfu::LocationTy::Kind::CONSTANT);
            v = JSTaggedType(static_cast<int64_t>(std::get<kungfu::OffsetType>(deopt.value)));
        }
        if (id != static_cast<kungfu::OffsetType>(SpecVregIndex::PC_INDEX)) {
            vregs_[id] = JSTaggedValue(v);
        } else {
            pc_ = static_cast<uint32_t>(v);
        }
    }
}

void Deoptimizier::CollectDeoptBundleVec(std::vector<kungfu::ARKDeopt>& deoptBundle)
{
    JSTaggedType *lastLeave = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    FrameIterator it(lastLeave, thread_);
    for (; !it.Done() && deoptBundle.empty(); it.Advance<GCVisitedFlag::VISITED>()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                frame->GetDeoptBundleInfo(it, deoptBundle);
                kungfu::CalleeRegAndOffsetVec calleeRegInfo;
                frame->GetFuncCalleeRegAndOffset(it, calleeRegInfo);
                context_.calleeRegAndOffset = calleeRegInfo;
                context_.callsiteSp = it.GetCallSiteSp();
                context_.callsiteFp = reinterpret_cast<uintptr_t>(it.GetSp());
                context_.preFrameSp = frame->ComputePrevFrameSp(it);
                context_.returnAddr = frame->GetReturnAddr();
                uint64_t argc = frame->GetArgc(context_.preFrameSp);
                auto argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(context_.preFrameSp));
                if (argc > 0) {
                    ASSERT(argc >= FIXED_NUM_ARGS);
                    callTarget_ = JSTaggedValue(argv[0]);
                }
                AotArgvs_ = argv;
                // +1 skip rbp
                stackContext_.callFrameTop_ = reinterpret_cast<uintptr_t>(it.GetSp()) + sizeof(uintptr_t);
                stackContext_.callerFp_ = reinterpret_cast<uintptr_t>(frame->GetPrevFrameFp());
                break;
            }
            case FrameType::OPTIMIZED_FRAME: {
                auto sp = reinterpret_cast<uintptr_t*>(it.GetSp());
                sp--; // skip type
                calleeRegAddr_ = sp - numCalleeRegs_;
                break;
            }
            case FrameType::LEAVE_FRAME:
                break;
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
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
    kungfu::CalleeReg callreg;
    for (auto &it: context_.calleeRegAndOffset) {
        auto reg = it.first;
        auto offset = it.second;
        uintptr_t value = *(reinterpret_cast<uintptr_t *>(context_.callsiteFp + offset));
        int order = callreg.FindCallRegOrder(reg);
        calleeRegAddr_[order] = value;
    }
}

JSTaggedType Deoptimizier::ConstructAsmInterpretFrame()
{
    ASSERT(thread_ != nullptr);
    auto fun = callTarget_;
    FrameWriter frameWriter(this);
    // Push asm interpreter frame
    auto method = GetMethod(callTarget_);
    auto numVRegs = method->GetNumberVRegs();
    if (!frameWriter.Reserve(numVRegs)) {
        return JSTaggedValue::Exception().GetRawData();
    }

    for (int32_t i = numVRegs - 1; i >= 0; i--) {
        JSTaggedValue value = JSTaggedValue::Undefined();
        if (vregs_.find(static_cast<kungfu::OffsetType>(i)) != vregs_.end()) {
            value = vregs_.at(static_cast<kungfu::OffsetType>(i));
        }
        frameWriter.PushValue(value.GetRawData());
    }
    const uint8_t *resumePc = method->GetBytecodeArray() + pc_;
    AsmInterpretedFrame *statePtr = frameWriter.ReserveAsmInterpretedFrame();
    JSTaggedValue env = JSTaggedValue(GetArgv(CommonArgIdx::LEXENV));
    JSTaggedValue thisObj = JSTaggedValue(GetArgv(CommonArgIdx::THIS));
    auto acc = vregs_.at(static_cast<kungfu::OffsetType>(SpecVregIndex::ACC_INDEX));
    statePtr->function = fun;
    statePtr->acc = acc;
    statePtr->env = env;
    statePtr->callSize = 0;
    statePtr->fp = 0;  // need update
    statePtr->thisObj = thisObj;
    statePtr->pc = resumePc;
    statePtr->base.prev = reinterpret_cast<JSTaggedType *>(stackContext_.callFrameTop_);
    statePtr->base.type = FrameType::ASM_INTERPRETER_FRAME;

    // construct stack context
    auto start = frameWriter.GetStart();
    auto end = frameWriter.GetTop();
    auto outputCount = start - end;

    RelocateCalleeSave();

    frameWriter.PushRawValue(stackContext_.callerFp_);
    frameWriter.PushRawValue(stackContext_.callFrameTop_);
    frameWriter.PushRawValue(outputCount);
    // PushCppCalleeSaveRegisters
    for (int32_t i = numCalleeRegs_ - 1; i >= 0; i--) {
        frameWriter.PushRawValue(calleeRegAddr_[i]);
    }
    return reinterpret_cast<JSTaggedType>(frameWriter.GetTop());
}
}  // namespace panda::ecmascript