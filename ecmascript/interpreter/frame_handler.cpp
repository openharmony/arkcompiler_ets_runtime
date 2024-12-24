
/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/interpreter/frame_handler.h"

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"

namespace panda::ecmascript {
FrameHandler::FrameHandler(const JSThread *thread)
    : sp_(const_cast<JSTaggedType *>(thread->GetCurrentFrame())), thread_(thread)
{
    AdvanceToJSFrame();
}

ARK_INLINE void FrameHandler::AdvanceToJSFrame()
{
    if (!thread_->IsAsmInterpreter()) {
        return;
    }
    FrameIterator it(sp_, thread_);
    for (; !it.Done(); it.Advance()) {
        FrameType t = it.GetFrameType();
        if (IsBaselineBuiltinFrame(t)) {
            FindAndSetBaselineNativePc(it);
        }
        if (IsJSFrame(t) || IsJSEntryFrame(t)) {
            break;
        }
    }
    sp_ = it.GetSp();
}

ARK_INLINE void FrameHandler::PrevJSFrame()
{
    if (!thread_->IsAsmInterpreter()) {
        FrameIterator it(sp_, thread_);
        if (IsBaselineBuiltinFrame(it.GetFrameType())) {
            FindAndSetBaselineNativePc(it);
        }
        it.Advance();
        sp_ = it.GetSp();
        return;
    }
    AdvanceToJSFrame();
    FrameIterator it(sp_, thread_);
    FrameType t = it.GetFrameType();
    if (t == FrameType::ASM_INTERPRETER_FRAME) {
        auto frame = it.GetFrame<AsmInterpretedFrame>();
        if (thread_->IsAsmInterpreter()) {
            fp_ = frame->GetCurrentFramePointer();
        }
    }
    it.Advance();
    sp_ = it.GetSp();
    AdvanceToJSFrame();
}

JSTaggedType* FrameHandler::GetPrevJSFrame()
{
    PrevJSFrame();
    return GetSp();
}

uint32_t FrameHandler::GetNumberArgs()
{
    if (thread_->IsAsmInterpreter()) {
        auto *frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
        return static_cast<uint32_t>(frame->GetCurrentFramePointer() - sp_);
    }
    ASSERT(IsInterpretedFrame());
    JSTaggedType *prevSp = nullptr;
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        prevSp = frame->GetPrevFrameFp();
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        prevSp = frame->GetPrevFrameFp();
    }
    auto prevSpEnd = reinterpret_cast<JSTaggedType*>(GetInterpretedFrameEnd(prevSp));
    return static_cast<uint32_t>(prevSpEnd - sp_);
}

JSTaggedValue FrameHandler::GetVRegValue(size_t index) const
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return JSTaggedValue(sp_[index]);
}

void FrameHandler::SetVRegValue(size_t index, JSTaggedValue value)
{
    ASSERT(IsInterpretedFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    sp_[index] = value.GetRawData();
}

void FrameHandler::FindAndSetBaselineNativePc(FrameIterator it)
{
    ASSERT(IsBaselineBuiltinFrame(it.GetFrameType()));
    auto *frame = it.GetFrame<BaselineBuiltinFrame>();
    baselineNativePc_ = frame->GetReturnAddr();
}

JSTaggedValue FrameHandler::GetAcc() const
{
    ASSERT(IsInterpretedFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->acc;
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->acc;
    }
}

uint32_t FrameHandler::GetBytecodeOffset() const
{
    ASSERT(IsInterpretedFrame());
    auto pc = GetPc();
    if (reinterpret_cast<uintptr_t>(pc) != std::numeric_limits<uintptr_t>::max()) {
        // interpreter frame
        Method *method = GetMethod();
        auto offset = pc - method->GetBytecodeArray();
        return static_cast<uint32_t>(offset);
    } else {
        // baseline frame
        uintptr_t curNativePc = GetBaselineNativePc();
        ASSERT(curNativePc != 0);
        LOG_BASELINEJIT(DEBUG) << "current native pc in UpFrame: " << std::hex <<
        reinterpret_cast<void*>(curNativePc);
        JSHandle<JSTaggedValue> funcVal = JSHandle<JSTaggedValue>(thread_, GetFunction());
        JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(funcVal);
        uint32_t curBytecodePcOfst = RuntimeStubs::RuntimeGetBytecodePcOfstForBaseline(func, curNativePc);
        return curBytecodePcOfst;
    }
}

Method *FrameHandler::GetMethod() const
{
    ASSERT(IsJSFrame());
    auto function = GetFunction();
    return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget();
}

const JSPandaFile* FrameHandler::GetJSPandaFile() const
{
    auto method = GetMethod();
    return method->GetJSPandaFile();
}

std::string FrameHandler::GetFileName() const
{
    auto pandaFile = GetJSPandaFile();
    ASSERT(pandaFile != nullptr);
    return pandaFile->GetJSPandaFileDesc().c_str();
}

uint32_t FrameHandler::GetAbcId() const
{
    std::string abcName = GetFileName();
    pgo::PGOProfilerManager* pm = pgo::PGOProfilerManager::GetInstance();
    uint32_t abcId;
    if (!pm->GetPandaFileId(CString(abcName), abcId) && !abcName.empty()) {
        LOG_ECMA(ERROR) << "Get method abc id failed. abcName: " << abcName;
    }
    return abcId;
}

uint32_t FrameHandler::GetMethodId() const
{
    return GetMethod()->GetMethodId().GetOffset();
}

Method *FrameHandler::CheckAndGetMethod() const
{
    ASSERT(IsJSFrame());
    auto function = GetFunction();
    if (function.IsJSFunctionBase() || function.IsJSProxy()) {
        return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget();
    }
    return nullptr;
}

JSTaggedValue FrameHandler::GetThis() const
{
    ASSERT(IsInterpretedFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->thisObj;
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->thisObj;
    }
}

JSTaggedValue FrameHandler::GetFunction() const
{
    ASSERT(IsJSFrame());
    if (thread_->IsAsmInterpreter()) {
        FrameType type = GetFrameType();
        switch (type) {
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
                auto frame = AsmInterpretedFrame::GetFrameFromSp(sp_);
                return frame->function;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV: {
                auto *frame = BuiltinWithArgvFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME: {
                auto *frame = BuiltinFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto *frame = OptimizedJSFunctionFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::FASTJIT_FUNCTION_FRAME:
            case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
                auto *frame = FASTJITFunctionFrame::GetFrameFromSp(sp_);
                return frame->GetFunction();
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME :
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME:
            case FrameType::INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
            case FrameType::INTERPRETER_BUILTIN_FRAME:
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::BASELINE_BUILTIN_FRAME:
            case FrameType::ASM_BRIDGE_FRAME:
            case FrameType::LEAVE_FRAME:
            case FrameType::LEAVE_FRAME_WITH_ARGV:
            case FrameType::BUILTIN_CALL_LEAVE_FRAME:
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            default: {
                LOG_FULL(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    } else {
        FrameType type = GetFrameType();
        if (type == FrameType::INTERPRETER_FRAME || type == FrameType::INTERPRETER_FAST_NEW_FRAME) {
            auto *frame = InterpretedFrame::GetFrameFromSp(sp_);
            return frame->function;
        } else {
            auto *frame = InterpretedBuiltinFrame::GetFrameFromSp(sp_);
            return frame->function;
        }
    }
}

const uint8_t *FrameHandler::GetPc() const
{
    ASSERT(IsJSFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->GetPc();
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->GetPc();
    }
}

ConstantPool *FrameHandler::GetConstpool() const
{
    ASSERT(IsInterpretedFrame());
    auto method = GetMethod();
    JSTaggedValue constpool = method->GetConstantPool();
    return ConstantPool::Cast(constpool.GetTaggedObject());
}

JSTaggedValue FrameHandler::GetEnv() const
{
    ASSERT(IsInterpretedFrame());
    FrameIterator it(sp_, thread_);
    if (IsAsmInterpretedFrame()) {
        auto *frame = it.GetFrame<AsmInterpretedFrame>();
        return frame->GetEnv();
    } else {
        auto *frame = it.GetFrame<InterpretedFrame>();
        return frame->env;
    }
}

void FrameHandler::DumpStack(std::ostream &os) const
{
    size_t i = 0;
    FrameHandler frameHandler(thread_);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        os << "[" << i++
        << "]:" << frameHandler.GetMethod()->ParseFunctionName()
        << "\n";
    }
}

void FrameHandler::DumpPC(std::ostream &os, const uint8_t *pc) const
{
    FrameHandler frameHandler(thread_);
    ASSERT(frameHandler.HasFrame());
    // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions, bugprone-narrowing-conversions)
    int offset = pc - frameHandler.GetMethod()->GetBytecodeArray();
    os << "offset: " << offset << "\n";
}

ARK_INLINE uintptr_t FrameHandler::GetInterpretedFrameEnd(JSTaggedType *prevSp) const
{
    uintptr_t end = 0U;
    FrameIterator it(prevSp, thread_);
    FrameType type = it.GetFrameType();
    switch (type) {
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto frame = it.GetFrame<AsmInterpretedFrame>();
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto frame = it.GetFrame<InterpretedFrame>();
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME: {
            auto frame = it.GetFrame<InterpretedEntryFrame>();
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::INTERPRETER_BUILTIN_FRAME: {
            auto frame = it.GetFrame<InterpretedBuiltinFrame>();
            end = ToUintPtr(frame);
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME :
        case FrameType::BUILTIN_FRAME_WITH_ARGV:
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::BUILTIN_FRAME:
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::ASM_BRIDGE_FRAME:
        case FrameType::LEAVE_FRAME:
        case FrameType::BASELINE_BUILTIN_FRAME:
        case FrameType::LEAVE_FRAME_WITH_ARGV:
        case FrameType::BUILTIN_CALL_LEAVE_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
        default: {
            LOG_FULL(FATAL) << "frame type error!";
            UNREACHABLE();
        }
    }
    return end;
}

void FrameHandler::IterateAssembleStack(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
    const RootBaseAndDerivedVisitor &derivedVisitor)
{
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
    IterateFrameChain(current, visitor, rangeVisitor, derivedVisitor);
}

// We seperate InterpretedEntryFrame from assemble stack when asm interpreter is enable.
// To protect EcmaRuntimeCallInfo on InterpretedEntryFrame, we iterate InterpretedEntryFrame on thread sp individually.
// And only InterpretedEntryFrame is on thread sp when asm interpreter is enable.
void FrameHandler::IterateEcmaRuntimeCallInfo(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor)
{
    ASSERT(thread_->IsAsmInterpreter());
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    for (FrameIterator it(current, thread_); !it.Done(); it.Advance()) {
        ASSERT(it.GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
        auto frame = it.GetFrame<InterpretedEntryFrame>();
        frame->GCIterate(it, visitor, rangeVisitor);
    }
}

void FrameHandler::Iterate(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
    const RootBaseAndDerivedVisitor &derivedVisitor)
{
    if (thread_->IsAsmInterpreter()) {
        IterateEcmaRuntimeCallInfo(visitor, rangeVisitor);
        IterateAssembleStack(visitor, rangeVisitor, derivedVisitor);
        return;
    }
    JSTaggedType *current = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    FrameType frameType = FrameHandler::GetFrameType(current);
    if (frameType != FrameType::INTERPRETER_ENTRY_FRAME) {
        auto leaveFrame = const_cast<JSTaggedType *>(thread_->GetLastLeaveFrame());
        if (leaveFrame != nullptr) {
            current = leaveFrame;
        }
    }
    // lazy assignment: only Iterate need arkStackMapParser_ in order to high improve performance
    if (arkStackMapParser_ == nullptr) {
        arkStackMapParser_ =
            const_cast<JSThread *>(thread_)->GetEcmaVM()->GetAOTFileManager()->GetStackMapParser();
    }
    IterateFrameChain(current, visitor, rangeVisitor, derivedVisitor);
}

void FrameHandler::IterateFrameChain(JSTaggedType *start, const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor, const RootBaseAndDerivedVisitor &derivedVisitor) const
{
    JSTaggedType *current = start;
    // if the current frame type is BASELINE_BUILTIN_FRAME, the upper frame must be BaselineFrame.
    // isBaselineFrame is used to differentiate the AsmInterpterFrame and BaselineFrame
    bool isBaselineFrame = false;
    for (FrameIterator it(current, thread_); !it.Done(); it.Advance<GCVisitedFlag::VISITED>()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_FRAME: {
                auto frame = it.GetFrame<OptimizedFrame>();
                frame->GCIterate(it, visitor, rangeVisitor, derivedVisitor);
                break;
            }
            case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                frame->GCIterate(it, visitor, rangeVisitor, derivedVisitor, type);
                break;
            }
            case FrameType::BASELINE_BUILTIN_FRAME: {
                isBaselineFrame = true;
                auto frame = it.GetFrame<BaselineBuiltinFrame>();
                frame->GCIterate(it, visitor, rangeVisitor, derivedVisitor);
                break;
            }
            case FrameType::FASTJIT_FUNCTION_FRAME:
            case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
                auto frame = it.GetFrame<FASTJITFunctionFrame>();
                frame->GCIterate(it, visitor, rangeVisitor, derivedVisitor, type);
                break;
            }
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
                auto frame = it.GetFrame<AsmInterpretedFrame>();
                frame->GCIterate(it, visitor, rangeVisitor, derivedVisitor, isBaselineFrame);
                isBaselineFrame = false;
                break;
            }
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME: {
                auto frame = it.GetFrame<InterpretedFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::INTERPRETER_BUILTIN_FRAME: {
                auto frame = it.GetFrame<InterpretedBuiltinFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::LEAVE_FRAME: {
                auto frame = it.GetFrame<OptimizedLeaveFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::LEAVE_FRAME_WITH_ARGV: {
                auto frame = it.GetFrame<OptimizedWithArgvLeaveFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::BUILTIN_CALL_LEAVE_FRAME: {
                auto frame = it.GetFrame<OptimizedBuiltinLeaveFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV: {
                auto frame = it.GetFrame<BuiltinWithArgvFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME: {
                auto frame = it.GetFrame<BuiltinFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::INTERPRETER_ENTRY_FRAME: {
                auto frame = it.GetFrame<InterpretedEntryFrame>();
                frame->GCIterate(it, visitor, rangeVisitor);
                break;
            }
            case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            case FrameType::ASM_BRIDGE_FRAME:
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_BRIDGE_FRAME: {
                break;
            }
            default: {
                LOG_FULL(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    }
}
}  // namespace panda::ecmascript
