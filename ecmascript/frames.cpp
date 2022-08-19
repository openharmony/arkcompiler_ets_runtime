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

#include "ecmascript/frames.h"

#include "ecmascript/ark_stackmap_parser.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/file_loader.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/llvm_stackmap_parser.h"
#include "ecmascript/interpreter/frame_handler.h"

#if defined(PANDA_TARGET_UNIX) && !defined(PANDA_TARGET_MACOS)
#include <sys/ptrace.h>
#else
#define ptrace(PTRACE_PEEKTEXT, pid, addr, NULL) static_cast<long>(-1)
#endif

namespace panda::ecmascript {
FrameIterator::FrameIterator(JSTaggedType *sp, const JSThread *thread) : current_(sp), thread_(thread)
{
    if (thread != nullptr) {
        arkStackMapParser_ = thread->GetEcmaVM()->GetFileLoader()->GetStackMapParser();
    }
}

int FrameIterator::ComputeDelta() const
{
    return fpDeltaPrevFrameSp_;
}

int FrameIterator::GetCallSiteDelta(uintptr_t returnAddr) const
{
    auto callsiteInfo = CalCallSiteInfo(returnAddr);
    int delta = std::get<2>(callsiteInfo); // 2:delta index
    return delta;
}

std::tuple<uint64_t, uint8_t *, int> FrameIterator::CalCallSiteInfo(uintptr_t retAddr) const
{
    auto loader = thread_->GetEcmaVM()->GetFileLoader();
    const std::vector<AOTModulePackInfo>& aotPackInfos = loader->GetPackInfos();
    std::tuple<uint64_t, uint8_t *, int> callsiteInfo;

    StubModulePackInfo stubInfo = loader->GetStubPackInfo();
    bool ans = stubInfo.CalCallSiteInfo(retAddr, callsiteInfo);
    if (ans) {
        return callsiteInfo;
    }
    // aot
    for (auto &info : aotPackInfos) {
        ans = info.CalCallSiteInfo(retAddr, callsiteInfo);
        if (ans) {
            return callsiteInfo;
        }
    }
    return callsiteInfo;
}

template <GCVisitedFlag GCVisit>
void FrameIterator::Advance()
{
    ASSERT(!Done());
    FrameType t = GetFrameType();
    [[maybe_unused]] bool needCalCallSiteInfo = false;
    switch (t) {
        case FrameType::OPTIMIZED_FRAME : {
            auto frame = GetFrame<OptimizedFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(optimizedReturnAddr_);
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME : {
            auto frame = GetFrame<OptimizedEntryFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionUnfoldArgVFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = frame->GetPrevFrameSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(optimizedReturnAddr_);
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV : {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedBuiltinLeaveFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME : {
            auto frame = GetFrame<InterpretedFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_BUILTIN_FRAME: {
            auto frame = GetFrame<InterpretedBuiltinFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME : {
            auto frame = GetFrame<AsmInterpretedFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME:
        case FrameType::BUILTIN_ENTRY_FRAME : {
            auto frame = GetFrame<BuiltinFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = frame->GetReturnAddr();
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV : {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = frame->GetReturnAddr();
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<InterpretedEntryFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<AsmInterpretedEntryFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            if constexpr (GCVisit == GCVisitedFlag::VISITED) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp(optimizedReturnAddr_);
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        default: {
            UNREACHABLE();
        }
    }
    if constexpr (GCVisit == GCVisitedFlag::VISITED) {
        if (!needCalCallSiteInfo) {
            return;
        }
        uint64_t textStart;
        std::tie(textStart, stackMapAddr_, fpDeltaPrevFrameSp_) = CalCallSiteInfo(optimizedReturnAddr_);
        ASSERT(optimizedReturnAddr_ >= textStart);
        optimizedReturnAddr_ = optimizedReturnAddr_ - textStart;
    }
}
template void FrameIterator::Advance<GCVisitedFlag::VISITED>();
template void FrameIterator::Advance<GCVisitedFlag::IGNORED>();

uintptr_t FrameIterator::GetPrevFrameCallSiteSp([[maybe_unused]] uintptr_t curPc) const
{
    if (Done()) {
        return 0;
    }
    auto type = GetFrameType();
    switch (type) {
        case FrameType::LEAVE_FRAME: {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME: {
            auto frame = GetFrame<OptimizedBuiltinLeaveFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV: {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::BUILTIN_FRAME: {
            auto frame = GetFrame<BuiltinFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME: {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            return frame->GetCallSiteSp();
        }
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            ASSERT(thread_ != nullptr);
            auto callSiteSp = reinterpret_cast<uintptr_t>(current_) + fpDeltaPrevFrameSp_;
            return callSiteSp;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionUnfoldArgVFrame>();
            return frame->GetPrevFrameSp();
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME : {
            auto callSiteSp = OptimizedJSFunctionFrame::ComputeArgsConfigFrameSp(current_);
            return callSiteSp;
        }
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::INTERPRETER_BUILTIN_FRAME:
        case FrameType::INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME: {
            return 0;
        }
        default: {
            LOG_ECMA(FATAL) << "frame type error!";
        }
    }
}

uintptr_t FrameIterator::GetPrevFrame() const
{
    FrameType type = GetFrameType();
    uintptr_t end = 0U;
    switch (type) {
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto prevFrame = GetFrame<InterpretedFrame>();
            end = ToUintPtr(prevFrame);
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME: {
            auto prevFrame = GetFrame<InterpretedEntryFrame>();
            end = ToUintPtr(prevFrame);
            break;
        }
        case FrameType::INTERPRETER_BUILTIN_FRAME: {
            auto prevFrame = GetFrame<InterpretedBuiltinFrame>();
            end = ToUintPtr(prevFrame);
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "frame type error!";
        }
    }
    return end;
}

bool FrameIterator::IteratorStackMap(const RootVisitor &visitor, const RootBaseAndDerivedVisitor &derivedVisitor) const
{
    ASSERT(arkStackMapParser_ != nullptr);
    return arkStackMapParser_->IteratorStackMap(visitor, derivedVisitor, optimizedReturnAddr_,
        reinterpret_cast<uintptr_t>(current_), optimizedCallSiteSp_, stackMapAddr_);
}

ARK_INLINE void OptimizedFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &visitor,
    [[maybe_unused]] const RootRangeVisitor &rangeVisitor,
    const RootBaseAndDerivedVisitor &derivedVisitor) const
{
    bool ret = it.IteratorStackMap(visitor, derivedVisitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

void FrameIterator::CollectBCOffsetInfo(kungfu::ConstInfo &info) const
{
    arkStackMapParser_->GetConstInfo(optimizedReturnAddr_, info, stackMapAddr_);
}

ARK_INLINE JSTaggedType* OptimizedJSFunctionFrame::GetArgv(const FrameIterator &it) const
{
    uintptr_t *preFrameSp = ComputePrevFrameSp(it);
    return GetArgv(preFrameSp);
}

ARK_INLINE uintptr_t* OptimizedJSFunctionFrame::ComputePrevFrameSp(const FrameIterator &it) const
{
    const JSTaggedType *sp = it.GetSp();
    int delta = it.ComputeDelta();
    ASSERT((delta > 0) && (delta % sizeof(uintptr_t) == 0));
    uintptr_t *preFrameSp = reinterpret_cast<uintptr_t *>(const_cast<JSTaggedType *>(sp))
            + delta / sizeof(uintptr_t);
    return preFrameSp;
}


void OptimizedJSFunctionFrame::CollectBCOffsetInfo(const FrameIterator &it, kungfu::ConstInfo &info) const
{
    it.CollectBCOffsetInfo(info);
}

ARK_INLINE void OptimizedJSFunctionFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor,
    const RootBaseAndDerivedVisitor &derivedVisitor) const
{
    OptimizedJSFunctionFrame *frame = OptimizedJSFunctionFrame::GetFrameFromSp(it.GetSp());
    uintptr_t *envPtr = reinterpret_cast<uintptr_t *>(frame);
    uintptr_t envslot = ToUintPtr(envPtr);
    visitor(Root::ROOT_FRAME, ObjectSlot(envslot));

    uintptr_t *preFrameSp = frame->ComputePrevFrameSp(it);

    auto argc = frame->GetArgc(preFrameSp);
    JSTaggedType *argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(preFrameSp));
    if (argc > 0) {
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + argc);
        rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }

    bool ret = it.IteratorStackMap(visitor, derivedVisitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

ARK_INLINE void AsmInterpretedFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor,
    const RootBaseAndDerivedVisitor &derivedVisitor) const
{
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(it.GetSp());
    uintptr_t start = ToUintPtr(it.GetSp());
    uintptr_t end = ToUintPtr(frame->GetCurrentFramePointer());
    rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    if (frame->pc != nullptr) {
        visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
    }

    std::set<uintptr_t> slotAddrs;
    bool ret = it.IteratorStackMap(visitor, derivedVisitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

ARK_INLINE void InterpretedFrame::GCIterate(const FrameIterator &it,
                                            const RootVisitor &visitor,
                                            const RootRangeVisitor &rangeVisitor) const
{
    auto sp = it.GetSp();
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp);
    if (frame->function == JSTaggedValue::Hole()) {
        return;
    }

    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    uintptr_t start = ToUintPtr(sp);
    const JSThread *thread = it.GetThread();
    FrameIterator prevIt(prevSp, thread);
    uintptr_t end = prevIt.GetPrevFrame();

    rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));

    // pc == nullptr, init InterpretedFrame & native InterpretedFrame.
    if (frame->pc != nullptr) {
        visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->constpool)));
        visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->profileTypeInfo)));
    }
}

ARK_INLINE void InterpretedBuiltinFrame::GCIterate(const FrameIterator &it,
                                                   const RootVisitor &visitor,
                                                   const RootRangeVisitor &rangeVisitor) const
{
    auto sp = it.GetSp();
    InterpretedBuiltinFrame *frame = InterpretedBuiltinFrame::GetFrameFromSp(sp);
    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    const JSThread *thread = it.GetThread();
    FrameIterator prevIt(prevSp, thread);

    uintptr_t start = ToUintPtr(sp + 2); // 2: numArgs & thread.
    uintptr_t end = prevIt.GetPrevFrame();
    rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    visitor(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
}

ARK_INLINE void OptimizedLeaveFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(&frame->argc + 1);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void OptimizedWithArgvLeaveFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedWithArgvLeaveFrame *frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        uintptr_t* argvPtr = reinterpret_cast<uintptr_t *>(&frame->argc + 1);
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(*argvPtr);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void OptimizedBuiltinLeaveFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedBuiltinLeaveFrame *frame = OptimizedBuiltinLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(&frame->argc + 1);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void BuiltinWithArgvFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor) const
{
    const JSTaggedType *sp = it.GetSp();
    auto frame = BuiltinWithArgvFrame::GetFrameFromSp(sp);
    auto argc = static_cast<uint32_t>(frame->GetNumArgs()) + NUM_MANDATORY_JSFUNC_ARGS;
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void BuiltinFrame::GCIterate(const FrameIterator &it,
    const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor) const
{
    const JSTaggedType *sp = it.GetSp();
    auto frame = BuiltinFrame::GetFrameFromSp(sp);
    // no need to visit stack map for entry frame
    if (frame->type == FrameType::BUILTIN_ENTRY_FRAME) {
        // only visit function
        visitor(Root::ROOT_FRAME, ObjectSlot(frame->GetStackArgsAddress()));
        return;
    }
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    auto argc = frame->GetNumArgs();
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void InterpretedEntryFrame::GCIterate(const FrameIterator &it,
    [[maybe_unused]] const RootVisitor &visitor,
    const RootRangeVisitor &rangeVisitor) const
{
    const JSTaggedType* sp = it.GetSp();
    InterpretedEntryFrame *frame = InterpretedEntryFrame::GetFrameFromSp(sp);
    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    if (prevSp == nullptr) {
        return;
    }

    const JSThread *thread = it.GetThread();
    FrameIterator prevIt(prevSp, thread);
    uintptr_t start = ToUintPtr(sp + 2); // 2: numArgs & thread.
    uintptr_t end = prevIt.GetPrevFrame();
    rangeVisitor(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

bool ReadUintptrFromAddr(int pid, uintptr_t addr, uintptr_t &value)
{
    if (pid == getpid()) {
        value = *(reinterpret_cast<uintptr_t *>(addr));
        return true;
    }
    long *retAddr = reinterpret_cast<long *>(&value);
    // note: big endian
    for (size_t i = 0; i < sizeof(uintptr_t) / sizeof(long); i++) {
        *retAddr = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
        if (*retAddr == -1) {
            LOG_ECMA(ERROR) << "ReadFromAddr ERROR, addr: " << addr;
            return false;
        }
        addr += sizeof(long);
        retAddr++;
    }
    return true;
}

bool GetTypeOffsetAndPrevOffsetFromFrameType(uintptr_t frameType, uintptr_t &typeOffset, uintptr_t &prevOffset)
{
    switch (frameType) {
        case (uintptr_t)(FrameType::OPTIMIZED_FRAME):
            typeOffset = OptimizedFrame::GetTypeOffset();
            prevOffset = OptimizedFrame::GetPrevOffset();
            break;
        case (uintptr_t)(FrameType::OPTIMIZED_ENTRY_FRAME):
            typeOffset = MEMBER_OFFSET(OptimizedEntryFrame, type);
            prevOffset = MEMBER_OFFSET(OptimizedEntryFrame, preLeaveFrameFp);
            break;
        case (uintptr_t)(FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME):
            typeOffset = OptimizedJSFunctionUnfoldArgVFrame::GetTypeOffset();
            prevOffset = OptimizedJSFunctionUnfoldArgVFrame::GetPrevOffset();
            break;
        case (uintptr_t)(FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME):
        case (uintptr_t)(FrameType::OPTIMIZED_JS_FUNCTION_FRAME):
            typeOffset = OptimizedJSFunctionFrame::GetTypeOffset();
            prevOffset = OptimizedJSFunctionFrame::GetPrevOffset();
            break;
        case (uintptr_t)(FrameType::LEAVE_FRAME):
            typeOffset = MEMBER_OFFSET(OptimizedLeaveFrame, type);
            prevOffset = MEMBER_OFFSET(OptimizedLeaveFrame, callsiteFp);
            break;
        case (uintptr_t)(FrameType::LEAVE_FRAME_WITH_ARGV):
            typeOffset = MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, type);
            prevOffset = MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, callsiteFp);
            break;
        case (uintptr_t)(FrameType::BUILTIN_CALL_LEAVE_FRAME):
            typeOffset = OptimizedBuiltinLeaveFrame::GetTypeOffset();
            prevOffset = OptimizedBuiltinLeaveFrame::GetPrevOffset();
            break;
        case (uintptr_t)(FrameType::INTERPRETER_FRAME):
        case (uintptr_t)(FrameType::INTERPRETER_FAST_NEW_FRAME):
            typeOffset = MEMBER_OFFSET(InterpretedFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, type);
            prevOffset = MEMBER_OFFSET(InterpretedFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, prev);
            break;
        case (uintptr_t)(FrameType::INTERPRETER_BUILTIN_FRAME):
            typeOffset = MEMBER_OFFSET(InterpretedBuiltinFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, type);
            prevOffset = MEMBER_OFFSET(InterpretedBuiltinFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, prev);
            break;
        case (uintptr_t)(FrameType::INTERPRETER_CONSTRUCTOR_FRAME):
        case (uintptr_t)(FrameType::ASM_INTERPRETER_FRAME):
            typeOffset = MEMBER_OFFSET(AsmInterpretedFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, type);
            prevOffset = MEMBER_OFFSET(AsmInterpretedFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, prev);
            break;
        case (uintptr_t)(FrameType::BUILTIN_FRAME):
        case (uintptr_t)(FrameType::BUILTIN_ENTRY_FRAME):
            typeOffset = MEMBER_OFFSET(BuiltinFrame, type);
            prevOffset = MEMBER_OFFSET(BuiltinFrame, prevFp);
            break;
        case (uintptr_t)(FrameType::BUILTIN_FRAME_WITH_ARGV):
            typeOffset = MEMBER_OFFSET(BuiltinWithArgvFrame, type);
            prevOffset = MEMBER_OFFSET(BuiltinWithArgvFrame, prevFp);
            break;
        case (uintptr_t)(FrameType::INTERPRETER_ENTRY_FRAME):
            typeOffset = MEMBER_OFFSET(InterpretedEntryFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, type);
            prevOffset = MEMBER_OFFSET(InterpretedEntryFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, prev);
            break;
        case (uintptr_t)(FrameType::ASM_INTERPRETER_ENTRY_FRAME):
            typeOffset = MEMBER_OFFSET(AsmInterpretedEntryFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, type);
            prevOffset = MEMBER_OFFSET(AsmInterpretedEntryFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, prev);
            break;
        case (uintptr_t)(FrameType::ASM_INTERPRETER_BRIDGE_FRAME):
            typeOffset = MEMBER_OFFSET(AsmInterpretedBridgeFrame, entry) +
                         MEMBER_OFFSET(AsmInterpretedEntryFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, type);
            prevOffset = MEMBER_OFFSET(AsmInterpretedBridgeFrame, entry) +
                         MEMBER_OFFSET(AsmInterpretedEntryFrame, base) +
                         MEMBER_OFFSET(InterpretedFrameBase, prev);
            break;
        default:
            return false;
    }
    return true;
}

bool StepArkManagedNativeFrame(int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp,
                               [[maybe_unused]] char *buf, [[maybe_unused]] size_t bufSize)
{
    uintptr_t currentPtr = *fp;
    while (true) {
        currentPtr -= sizeof(FrameType);
        uintptr_t frameType = 0;
        if (!ReadUintptrFromAddr(pid, currentPtr, frameType)) {
            return false;
        }
        uintptr_t typeOffset = 0;
        uintptr_t prevOffset = 0;
        if (!GetTypeOffsetAndPrevOffsetFromFrameType(frameType, typeOffset, prevOffset)) {
            LOG_ECMA(ERROR) << "FrameType ERROR, addr: " << currentPtr << ", frameType: " << frameType;
            return false;
        }
        if (frameType == (uintptr_t)(FrameType::ASM_INTERPRETER_ENTRY_FRAME)) {
            break;
        }
        currentPtr -= typeOffset;
        currentPtr += prevOffset;
        if (!ReadUintptrFromAddr(pid, currentPtr, currentPtr)) {
            return false;
        }
    }
    currentPtr += sizeof(FrameType);
    *fp = currentPtr;
    currentPtr += 8;  // 8: size of fp
    if (!ReadUintptrFromAddr(pid, currentPtr, *pc)) {
        return false;
    }
    currentPtr += 8;  // 8: size of lr
    *sp = currentPtr;
    return true;
}
}  // namespace panda::ecmascript

__attribute__((visibility("default"))) int step_ark_managed_native_frame(
    int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp, char *buf, size_t buf_sz)
{
    if (panda::ecmascript::StepArkManagedNativeFrame(pid, pc, fp, sp, buf, buf_sz)) {
        return 1;
    }
    return -1;
}
