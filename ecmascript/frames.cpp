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

#include "ecmascript/frames.h"

#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/stackmap/ark_stackmap_parser.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"

namespace panda::ecmascript {
FrameIterator::FrameIterator(JSTaggedType *sp, const JSThread *thread) : current_(sp), thread_(thread)
{
    if (thread != nullptr) {
        arkStackMapParser_ =
            const_cast<JSThread *>(thread)->GetEcmaVM()->GetAOTFileManager()->GetStackMapParser();
    }
    if ((current_ != nullptr) && (GetFrameType() == FrameType::BASELINE_BUILTIN_FRAME)) {
        auto frame = GetFrame<BaselineBuiltinFrame>();
        baselineNativePc_ = frame->GetReturnAddr();
    }
}

int FrameIterator::ComputeDelta(const Method *method) const
{
    if (method == nullptr || isJITFrame_) {
        return fpDeltaPrevFrameSp_;
    }
    return method->GetFpDelta();
}

Method *FrameIterator::CheckAndGetMethod() const
{
    auto function = GetFunction();
    if (function.CheckIsJSFunctionBase() || function.CheckIsJSProxy()) {
        return ECMAObject::Cast(function.GetTaggedObject())->GetCallTarget(thread_);
    }
    return nullptr;
}

JSTaggedValue FrameIterator::GetFunction() const
{
    FrameType type = GetFrameType();
    switch (type) {
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            return frame->GetFunction();
        }
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto frame = GetFrame<AsmInterpretedFrame>();
            return frame->function;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto frame = GetFrame<InterpretedFrame>();
            return frame->function;
        }
        case FrameType::INTERPRETER_BUILTIN_FRAME: {
            auto frame = GetFrame<InterpretedBuiltinFrame>();
            return frame->function;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV: {
            auto *frame = BuiltinWithArgvFrame::GetFrameFromSp(GetSp());
            return frame->GetFunction();
        }
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::BUILTIN_FRAME: {
            auto *frame = BuiltinFrame::GetFrameFromSp(GetSp());
            return frame->GetFunction();
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME: {
            auto *frame = OptimizedBuiltinLeaveFrame::GetFrameFromSp(GetSp());
            return JSTaggedValue(*(frame->GetArgv()));
        }
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            auto frame = FASTJITFunctionFrame::GetFrameFromSp(GetSp());
            return frame->GetFunction();
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME :
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::ASM_BRIDGE_FRAME:
        case FrameType::LEAVE_FRAME:
        case FrameType::BASELINE_BUILTIN_FRAME:
        case FrameType::LEAVE_FRAME_WITH_ARGV:
        case FrameType::INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            return JSTaggedValue::Undefined();
        }
        default: {
            LOG_FULL(FATAL) << "Unknown frame type: " << static_cast<uintptr_t>(type);
            UNREACHABLE();
        }
    }
}

AOTFileInfo::CallSiteInfo FrameIterator::TryCalCallSiteInfoFromMachineCode(uintptr_t retAddr) const
{
    FrameType type = GetFrameType();
    if (type == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME ||
        type == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
        type == FrameType::FASTJIT_FUNCTION_FRAME ||
        type == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME) {
        auto machineCode = thread_->GetEcmaVM()->GetHeap()->GetMachineCodeObject(retAddr);
        ASSERT(machineCode != nullptr);
        const_cast<FrameIterator*>(this)->machineCode_ = reinterpret_cast<JSTaggedType>(machineCode);
        return reinterpret_cast<MachineCode*>(machineCode_)->CalCallSiteInfo();
    }
    return {};
}

std::pair<AOTFileInfo::CallSiteInfo, bool> FrameIterator::CalCallSiteInfo(uintptr_t retAddr, bool isDeopt) const
{
    auto callSiteInfo = const_cast<JSThread*>(thread_)->GetEcmaVM()->CalCallSiteInfo(retAddr, isDeopt);
    if (std::get<1>(callSiteInfo) != nullptr) { // 1 : stackMapAddr
        return std::make_pair(callSiteInfo, false);
    }
    // try get jit code
    callSiteInfo = TryCalCallSiteInfoFromMachineCode(retAddr);
    return std::make_pair(callSiteInfo, true);
}

template <GCVisitedFlag GCVisit>
void FrameIterator::Advance()
{
    ASSERT(!Done());
    FrameType rawT = GetRawFrameType();
    FrameType t = GetFrameType();
    bool needCalCallSiteInfo = false;
    // If need to get return addr from frame, the flag euqal true.
    bool needCheckLazyDeoptFrame = false;
    constexpr bool GCVisitFlag = (GCVisit == GCVisitedFlag::VISITED ||
                                 GCVisit == GCVisitedFlag::HYBRID_STACK ||
                                 GCVisit == GCVisitedFlag::DEOPT);
    switch (t) {
        case FrameType::OPTIMIZED_FRAME : {
            auto frame = GetFrame<OptimizedFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_ENTRY_FRAME : {
            auto frame = GetFrame<OptimizedEntryFrame>();
            if constexpr (GCVisitFlag) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BASELINE_BUILTIN_FRAME: {
            auto frame = GetFrame<BaselineBuiltinFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = 0;
                optimizedReturnAddr_ = 0;
            }
            baselineNativePc_ = frame->GetReturnAddr();
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmBridgeFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionUnfoldArgVFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = frame->GetPrevFrameSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV : {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedBuiltinLeaveFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME : {
            auto frame = GetFrame<InterpretedFrame>();
            if constexpr (GCVisitFlag) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_BUILTIN_FRAME: {
            auto frame = GetFrame<InterpretedBuiltinFrame>();
            if constexpr (GCVisitFlag) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME : {
            auto frame = GetFrame<AsmInterpretedFrame>();
            if constexpr (GCVisitFlag) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME : {
            auto frame = GetFrame<BuiltinFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_ENTRY_FRAME : {
            auto frame = GetFrame<BuiltinFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = false;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV : {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME : {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<InterpretedEntryFrame>();
            if constexpr (GCVisitFlag) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME : {
            auto frame = GetFrame<AsmInterpretedEntryFrame>();
            if constexpr (GCVisitFlag) {
                optimizedReturnAddr_ = 0;
                optimizedCallSiteSp_ = 0;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            auto frame = GetFrame<FASTJITFunctionFrame>();
            if constexpr (GCVisitFlag) {
                optimizedCallSiteSp_ = GetPrevFrameCallSiteSp();
                optimizedReturnAddr_ = frame->GetReturnAddr();
                needCalCallSiteInfo = true;
                needCheckLazyDeoptFrame = true;
            }
            current_ = frame->GetPrevFrameFp();
            break;
        }
        default: {
            if constexpr (GCVisit == GCVisitedFlag::HYBRID_STACK) {
                current_ = nullptr;
                break;
            }
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }

    needCheckLazyDeoptFrame &= IsLazyDeoptFrameType(rawT);
    if (UNLIKELY(needCheckLazyDeoptFrame)) {
        optimizedReturnAddr_ = const_cast<JSThread *>(thread_)->GetCallSiteReturnAddr(optimizedCallSiteSp_);
    }
    
    if constexpr (GCVisitFlag) {
        if (!needCalCallSiteInfo) {
            return;
        }
        uint64_t textStart = 0;
        AOTFileInfo::CallSiteInfo callSiteInfo;
        if constexpr (GCVisit == GCVisitedFlag::DEOPT) {
            std::tie(callSiteInfo, isJITFrame_) = CalCallSiteInfo(optimizedReturnAddr_, true);
        } else {
            std::tie(callSiteInfo, isJITFrame_) = CalCallSiteInfo(optimizedReturnAddr_, false);
        }
        std::tie(textStart, stackMapAddr_, fpDeltaPrevFrameSp_, calleeRegInfo_) = callSiteInfo;
        ASSERT(optimizedReturnAddr_ >= textStart);
        optimizedReturnAddr_ = optimizedReturnAddr_ - textStart;
    }
}
template void FrameIterator::Advance<GCVisitedFlag::VISITED>();
template void FrameIterator::Advance<GCVisitedFlag::IGNORED>();
template void FrameIterator::Advance<GCVisitedFlag::HYBRID_STACK>();
template void FrameIterator::Advance<GCVisitedFlag::DEOPT>();

uintptr_t *FrameIterator::GetReturnAddrAddress() const
{
    FrameType type = GetFrameType();
    switch (type) {
        case FrameType::OPTIMIZED_FRAME : {
            auto frame = GetFrame<OptimizedFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::BASELINE_BUILTIN_FRAME: {
            auto frame = GetFrame<BaselineBuiltinFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::ASM_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmBridgeFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionUnfoldArgVFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            auto frame = GetFrame<OptimizedJSFunctionFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedLeaveFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV : {
            auto frame = GetFrame<OptimizedWithArgvLeaveFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME : {
            auto frame = GetFrame<OptimizedBuiltinLeaveFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::BUILTIN_FRAME:
        case FrameType::BUILTIN_ENTRY_FRAME : {
            auto frame = GetFrame<BuiltinFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV :
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME : {
            auto frame = GetFrame<BuiltinWithArgvFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME : {
            auto frame = GetFrame<AsmInterpretedBridgeFrame>();
            return const_cast<uintptr_t *>(frame->GetReturnAddrAddress());
        }
        default:
            break;
    }
    return nullptr;
}

uintptr_t FrameIterator::GetPrevFrameCallSiteSp() const
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
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME: {
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
        case FrameType::BASELINE_BUILTIN_FRAME:  // maybe we can store fpDelta somewhere else for these 2 cases
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            ASSERT(thread_ != nullptr);
            auto callSiteSp = reinterpret_cast<uintptr_t>(current_) + fpDeltaPrevFrameSp_;
            return callSiteSp;
        }
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            ASSERT(thread_ != nullptr);
            auto method = CheckAndGetMethod();
            ASSERT(method != nullptr);
            int delta = ComputeDelta(method);
            auto callSiteSp = reinterpret_cast<uintptr_t>(current_) + delta;
            return callSiteSp;
        }
        case FrameType::ASM_BRIDGE_FRAME: {
            auto frame = GetFrame<AsmBridgeFrame>();
            return frame->GetCallSiteSp();
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
            break;
        }
        default: {
            LOG_FULL(FATAL) << "Unknown frame type: " << static_cast<uintptr_t>(type);
        }
    }
    return 0;
}

uint32_t FrameIterator::GetBaselineBytecodeOffset() const
{
    ASSERT(baselineNativePc_ != 0);
    LOG_BASELINEJIT(DEBUG) << "current native pc in UpFrame: " << std::hex <<
        reinterpret_cast<void*>(baselineNativePc_);
    auto *frame = this->GetFrame<AsmInterpretedFrame>();
    JSHandle<JSTaggedValue> funcVal = JSHandle<JSTaggedValue>(thread_, frame->function);
    JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(funcVal);
    uint32_t curBytecodePcOfst = RuntimeStubs::RuntimeGetBytecodePcOfstForBaseline(
        const_cast<JSThread*>(thread_), func, baselineNativePc_);
    return curBytecodePcOfst;
}

uint32_t FrameIterator::GetBytecodeOffset() const
{
    FrameType type = this->GetFrameType();
    switch (type) {
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto *frame = this->GetFrame<AsmInterpretedFrame>();
            auto pc = frame->GetPc();
            if (reinterpret_cast<uintptr_t>(pc) == std::numeric_limits<uintptr_t>::max()) {
                // for baselinejit
                return GetBaselineBytecodeOffset();
            } else {
                Method *method = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget(thread_);
                auto offset = pc - method->GetBytecodeArray();
                return static_cast<uint32_t>(offset);
            }
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto *frame = this->GetFrame<InterpretedFrame>();
            Method *method = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget(thread_);
            auto offset = frame->GetPc() - method->GetBytecodeArray();
            return static_cast<uint32_t>(offset);
        }
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            [[fallthrough]];
        }
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            [[fallthrough]];
        }
        default: {
            break;
        }
    }
    return 0;
}

void FrameIterator::GetStackTraceInfos(std::vector<std::pair<JSTaggedType, uint32_t>> &stackTraceInfos,
                                       bool needBaselineSpecialHandling, uint32_t pcOffset) const
{
    FrameType type = this->GetFrameType();
    switch (type) {
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            auto *frame = this->GetFrame<AsmInterpretedFrame>();
            uint32_t offset;
            if (UNLIKELY(needBaselineSpecialHandling)) {
                offset = pcOffset;
            } else {
                Method *method = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget(thread_);
                offset = frame->GetPc() - method->GetBytecodeArray();
            }
            stackTraceInfos.push_back(std::make_pair(frame->function.GetRawData(), static_cast<uint32_t>(offset)));
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            auto *frame = this->GetFrame<InterpretedFrame>();
            Method *method = ECMAObject::Cast(frame->function.GetTaggedObject())->GetCallTarget(thread_);
            auto offset = frame->GetPc() - method->GetBytecodeArray();
            stackTraceInfos.push_back(std::make_pair(frame->function.GetRawData(), static_cast<uint32_t>(offset)));
            break;
        }
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME:
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            CollectStackTraceInfos(stackTraceInfos);
            break;
        }
        default: {
            break;
        }
    }
}

void FrameIterator::CollectStackTraceInfos(std::vector<std::pair<JSTaggedType, uint32_t>> &info) const
{
    uintptr_t callsiteSp = GetCallSiteSp();
    uintptr_t callsiteFp = reinterpret_cast<uintptr_t>(GetSp());
    arkStackMapParser_->CollectStackTraceInfos(optimizedReturnAddr_, info, callsiteSp, callsiteFp, stackMapAddr_);
}

size_t FrameIterator::GetInlineDepth() const
{
    return arkStackMapParser_->GetInlineDepth(optimizedReturnAddr_, stackMapAddr_);
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
            LOG_FULL(FATAL) << "Unknown frame type: " << static_cast<uintptr_t>(type);
        }
    }
    return end;
}

bool FrameIterator::IteratorStackMapAndDeopt(RootVisitor &visitor) const
{
    ASSERT(arkStackMapParser_ != nullptr);
    if (!stackMapAddr_) {  // enter by assembler, no stack map
        return true;
    }

    return arkStackMapParser_->IteratorStackMapAndDeopt(visitor, optimizedReturnAddr_,
        reinterpret_cast<uintptr_t>(current_), optimizedCallSiteSp_, stackMapAddr_);
}

ARK_INLINE void OptimizedFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    bool ret = it.IteratorStackMapAndDeopt(visitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

ARK_INLINE void BaselineBuiltinFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    bool ret = it.IteratorStackMapAndDeopt(visitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

void FrameIterator::CollectArkDeopt(std::vector<kungfu::ARKDeopt>& deopts) const
{
    arkStackMapParser_->GetArkDeopt(optimizedReturnAddr_, stackMapAddr_, deopts);
}

ARK_INLINE JSTaggedType* OptimizedJSFunctionFrame::GetArgv(const FrameIterator &it) const
{
    uintptr_t *preFrameSp = ComputePrevFrameSp(it);
    return GetArgv(preFrameSp);
}

ARK_INLINE uintptr_t* OptimizedJSFunctionFrame::ComputePrevFrameSp(const FrameIterator &it) const
{
    const JSTaggedType *sp = it.GetSp();
    Method *method = it.CheckAndGetMethod();
    ASSERT(method != nullptr);
    int delta = it.ComputeDelta(method);
    ASSERT((delta > 0) && (delta % sizeof(uintptr_t) == 0));
    uintptr_t *preFrameSp = reinterpret_cast<uintptr_t *>(const_cast<JSTaggedType *>(sp)) + delta / sizeof(uintptr_t);
    return preFrameSp;
}

ARK_INLINE void OptimizedJSFunctionFrame::GCIterate(const FrameIterator &it,
    RootVisitor &visitor, FrameType frameType) const
{
    OptimizedJSFunctionFrame *frame = OptimizedJSFunctionFrame::GetFrameFromSp(it.GetSp());
    uintptr_t *jsFuncPtr = reinterpret_cast<uintptr_t *>(frame);
    uintptr_t jsFuncSlot = ToUintPtr(jsFuncPtr);
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(jsFuncSlot));
    if (frameType == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
        uintptr_t *preFrameSp = frame->ComputePrevFrameSp(it);
        auto argc = frame->GetArgc(preFrameSp);
        JSTaggedType *argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(preFrameSp));
        if (argc > 0) {
            uintptr_t start = ToUintPtr(argv); // argv
            uintptr_t end = ToUintPtr(argv + argc);
            visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
        }
    }

    auto machineCodeSlot = ObjectSlot(ToUintPtr(it.GetMachineCodeSlot()));
    if (machineCodeSlot.GetTaggedType() != JSTaggedValue::VALUE_UNDEFINED) {
        visitor.VisitRoot(Root::ROOT_FRAME, machineCodeSlot);
    }

    bool ret = it.IteratorStackMapAndDeopt(visitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

void OptimizedJSFunctionFrame::GetDeoptBundleInfo(const FrameIterator &it, std::vector<kungfu::ARKDeopt>& deopts) const
{
    it.CollectArkDeopt(deopts);
}

void OptimizedJSFunctionFrame::GetFuncCalleeRegAndOffset(
    const FrameIterator &it, kungfu::CalleeRegAndOffsetVec &ret) const
{
    it.GetCalleeRegAndOffsetVec(ret);
}

ARK_INLINE JSTaggedType* FASTJITFunctionFrame::GetArgv(const FrameIterator &it) const
{
    uintptr_t *preFrameSp = ComputePrevFrameSp(it);
    return GetArgv(preFrameSp);
}

ARK_INLINE uintptr_t* FASTJITFunctionFrame::ComputePrevFrameSp(const FrameIterator &it) const
{
    const JSTaggedType *sp = it.GetSp();
    int delta = it.ComputeDelta();
    ASSERT((delta > 0) && (delta % sizeof(uintptr_t) == 0));
    uintptr_t *preFrameSp = reinterpret_cast<uintptr_t *>(const_cast<JSTaggedType *>(sp)) + delta / sizeof(uintptr_t);
    return preFrameSp;
}

ARK_INLINE void FASTJITFunctionFrame::GCIterate(const FrameIterator &it,
    RootVisitor &visitor, FrameType frameType) const
{
    FASTJITFunctionFrame *frame = FASTJITFunctionFrame::GetFrameFromSp(it.GetSp());
    uintptr_t jsFuncSlot = GetFuncAddrFromSp(it.GetSp());
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(jsFuncSlot));
    if (frameType == FrameType::FASTJIT_FUNCTION_FRAME) {
        uintptr_t *preFrameSp = frame->ComputePrevFrameSp(it);
        auto argc = frame->GetArgc(preFrameSp);
        JSTaggedType *argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(preFrameSp));
        if (argc > 0) {
            uintptr_t start = ToUintPtr(argv); // argv
            uintptr_t end = ToUintPtr(argv + argc);
            visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
        }
    }

    auto machineCodeSlot = ObjectSlot(ToUintPtr(it.GetMachineCodeSlot()));
    if (machineCodeSlot.GetTaggedType() != JSTaggedValue::VALUE_UNDEFINED) {
        visitor.VisitRoot(Root::ROOT_FRAME, machineCodeSlot);
    }

    bool ret = it.IteratorStackMapAndDeopt(visitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

void FASTJITFunctionFrame::GetDeoptBundleInfo(const FrameIterator &it, std::vector<kungfu::ARKDeopt>& deopts) const
{
    it.CollectArkDeopt(deopts);
}

void FASTJITFunctionFrame::GetFuncCalleeRegAndOffset(
    const FrameIterator &it, kungfu::CalleeRegAndOffsetVec &ret) const
{
    it.GetCalleeRegAndOffsetVec(ret);
}

ARK_INLINE void AsmInterpretedFrame::GCIterate(const FrameIterator &it,
    RootVisitor &visitor, bool isBaselineFrame) const
{
    AsmInterpretedFrame *frame = AsmInterpretedFrame::GetFrameFromSp(it.GetSp());
    uintptr_t start = ToUintPtr(it.GetSp());
    uintptr_t end = ToUintPtr(frame->GetCurrentFramePointer());
    visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->thisObj)));
    if (frame->pc != nullptr || isBaselineFrame) {
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
    }

    if (isBaselineFrame) {
        return;
    }
    bool ret = it.IteratorStackMapAndDeopt(visitor);
    if (!ret) {
#ifndef NDEBUG
        LOG_ECMA(DEBUG) << " stackmap don't found returnAddr " << it.GetOptimizedReturnAddr();
#endif
    }
}

ARK_INLINE void InterpretedFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    auto sp = it.GetSp();
    InterpretedFrame *frame = InterpretedFrame::GetFrameFromSp(sp);
    if (frame->function.IsHole()) {
        return;
    }

    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    uintptr_t start = ToUintPtr(sp);
    const JSThread *thread = it.GetThread();
    FrameIterator prevIt(prevSp, thread);
    uintptr_t end = prevIt.GetPrevFrame();

    visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->thisObj)));

    // pc == nullptr, init InterpretedFrame & native InterpretedFrame.
    if (frame->pc != nullptr) {
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->acc)));
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->constpool)));
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->env)));
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->profileTypeInfo)));
    }
}

ARK_INLINE void InterpretedBuiltinFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    auto sp = it.GetSp();
    InterpretedBuiltinFrame *frame = InterpretedBuiltinFrame::GetFrameFromSp(sp);
    JSTaggedType *prevSp = frame->GetPrevFrameFp();
    const JSThread *thread = it.GetThread();
    FrameIterator prevIt(prevSp, thread);

    uintptr_t start = ToUintPtr(sp + 2); // 2: numArgs & thread.
    uintptr_t end = prevIt.GetPrevFrame();
    visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(ToUintPtr(&frame->function)));
}

ARK_INLINE void OptimizedLeaveFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedLeaveFrame *frame = OptimizedLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(&frame->argc + 1);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void OptimizedWithArgvLeaveFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedWithArgvLeaveFrame *frame = OptimizedWithArgvLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        uintptr_t* argvPtr = reinterpret_cast<uintptr_t *>(&frame->argc + 1);
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(*argvPtr);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void OptimizedBuiltinLeaveFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    const JSTaggedType *sp = it.GetSp();
    OptimizedBuiltinLeaveFrame *frame = OptimizedBuiltinLeaveFrame::GetFrameFromSp(sp);
    if (frame->argc > 0) {
        JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(&frame->argc + 1);
        uintptr_t start = ToUintPtr(argv); // argv
        uintptr_t end = ToUintPtr(argv + frame->argc);
        visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
    }
}

ARK_INLINE void BuiltinWithArgvFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    const JSTaggedType *sp = it.GetSp();
    auto frame = BuiltinWithArgvFrame::GetFrameFromSp(sp);
    auto argc = static_cast<uint32_t>(frame->GetNumArgs()) + NUM_MANDATORY_JSFUNC_ARGS;
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void BuiltinFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
{
    const JSTaggedType *sp = it.GetSp();
    auto frameType = it.GetFrameType();
    auto frame = BuiltinFrame::GetFrameFromSp(sp);
    // no need to visit stack map for entry frame
    if (frameType == FrameType::BUILTIN_ENTRY_FRAME) {
        // only visit function
        visitor.VisitRoot(Root::ROOT_FRAME, ObjectSlot(frame->GetStackArgsAddress()));
        return;
    }
    JSTaggedType *argv = reinterpret_cast<JSTaggedType *>(frame->GetStackArgsAddress());
    auto argc = frame->GetNumArgs();
    uintptr_t start = ToUintPtr(argv);
    uintptr_t end = ToUintPtr(argv + argc);
    visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}

ARK_INLINE void InterpretedEntryFrame::GCIterate(const FrameIterator &it, RootVisitor &visitor) const
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
    visitor.VisitRangeRoot(Root::ROOT_FRAME, ObjectSlot(start), ObjectSlot(end));
}
}  // namespace panda::ecmascript
