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

#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/message_string.h"
#include "ecmascript/platform/os.h"
#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif

namespace panda::ecmascript {
std::string JsStackInfo::BuildMethodTrace(Method *method, uint32_t pcOffset)
{
    std::string data;
    data.append("    at ");
    std::string name = method->ParseFunctionName();
    if (name.empty()) {
        name = "anonymous";
    }
    data += name;
    data.append(" (");
    // source file
    DebugInfoExtractor *debugExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile());
    const std::string &sourceFile = debugExtractor->GetSourceFile(method->GetMethodId());
    if (sourceFile.empty()) {
        data.push_back('?');
    } else {
        data += sourceFile;
    }
    data.push_back(':');
    // line number and column number
    auto callbackLineFunc = [&data](int32_t line) -> bool {
        data += std::to_string(line + 1);
        data.push_back(':');
        return true;
    };
    auto callbackColumnFunc = [&data](int32_t column) -> bool {
        data += std::to_string(column + 1);
        return true;
    };
    panda_file::File::EntityId methodId = method->GetMethodId();
    if (!debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, pcOffset) ||
        !debugExtractor->MatchColumnWithOffset(callbackColumnFunc, methodId, pcOffset)) {
        data.push_back('?');
    }
    data.push_back(')');
    data.push_back('\n');
    return data;
}

std::string JsStackInfo::BuildJsStackTrace(JSThread *thread, bool needNative)
{
    std::string data;
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::HYBRID_STACK>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr) {
            continue;
        }
        if (!method->IsNativeWithCallField()) {
            auto pcOffset = it.GetBytecodeOffset();
            data += BuildMethodTrace(method, pcOffset);
        } else if (needNative) {
            auto addr = method->GetNativePointer();
            std::stringstream strm;
            strm << addr;
            data.append("    at native method (").append(strm.str()).append(")\n");
        }
    }
    if (data.empty()) {
#if defined(ENABLE_EXCEPTION_BACKTRACE)
        std::ostringstream stack;
        Backtrace(stack);
        data = stack.str();
#endif
    }
    return data;
}

std::vector<struct JsFrameInfo> JsStackInfo::BuildJsStackInfo(JSThread *thread)
{
    std::vector<struct JsFrameInfo> jsFrame;
    uintptr_t *native = nullptr;
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::HYBRID_STACK>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr) {
            continue;
        }
        struct JsFrameInfo frameInfo;
        if (native != nullptr) {
            frameInfo.nativePointer = native;
            native = nullptr;
        }
        if (!method->IsNativeWithCallField()) {
            std::string name = method->ParseFunctionName();
            if (name.empty()) {
                frameInfo.functionName = "anonymous";
            } else {
                frameInfo.functionName = name;
            }
            // source file
            DebugInfoExtractor *debugExtractor =
                JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile());
            const std::string &sourceFile = debugExtractor->GetSourceFile(method->GetMethodId());
            if (sourceFile.empty()) {
                frameInfo.fileName = "?";
            } else {
                frameInfo.fileName = sourceFile;
            }
            // line number and column number
            int lineNumber = 0;
            auto callbackLineFunc = [&frameInfo, &lineNumber](int32_t line) -> bool {
                lineNumber = line + 1;
                frameInfo.pos = std::to_string(lineNumber) + ":";
                return true;
            };
            auto callbackColumnFunc = [&frameInfo](int32_t column) -> bool {
                frameInfo.pos += std::to_string(column + 1);
                return true;
            };
            panda_file::File::EntityId methodId = method->GetMethodId();
            uint32_t offset = it.GetBytecodeOffset();
            if (!debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, offset) ||
                !debugExtractor->MatchColumnWithOffset(callbackColumnFunc, methodId, offset)) {
                frameInfo.pos = "?";
            }
            jsFrame.push_back(frameInfo);
        } else {
            JSTaggedValue function = it.GetFunction();
            JSHandle<JSTaggedValue> extraInfoValue(
                thread, JSFunction::Cast(function.GetTaggedObject())->GetFunctionExtraInfo());
            if (extraInfoValue->IsJSNativePointer()) {
                JSHandle<JSNativePointer> extraInfo(extraInfoValue);
                native = reinterpret_cast<uintptr_t *>(extraInfo->GetExternalPointer());
            }
        }
    }
    return jsFrame;
}

void CrashCallback(char *buf __attribute__((unused)), size_t len __attribute__((unused)),
                   void *ucontext __attribute__((unused)))
{
#if defined(__aarch64__) && !defined(PANDA_TARGET_MACOS) && !defined(PANDA_TARGET_IOS)
    if (ucontext == nullptr) {
        // should not happen
        return;
    }
    auto uctx = static_cast<ucontext_t *>(ucontext);
    uintptr_t pc = uctx->uc_mcontext.pc;
    uintptr_t fp = uctx->uc_mcontext.regs[29];  // 29: fp
    // 1. check pc is between ark code heap
    // 2. assemble crash info for ark code with signal-safe code
    // 3. do not do much things inside callback, stack size is limited
    // 4. do not use normal log
    if (JsStackInfo::loader == nullptr) {
        return;
    }
    if (!JsStackInfo::loader->InsideStub(pc) && !JsStackInfo::loader->InsideAOT(pc)) {
        return;
    }
    LOG_ECMA(ERROR) << std::hex << "CrashCallback pc:" << pc << " fp:" << fp;
    FrameIterator frame(reinterpret_cast<JSTaggedType *>(fp));
    bool isBuiltinStub = (frame.GetFrameType() == FrameType::OPTIMIZED_FRAME);
    Method *method = frame.CheckAndGetMethod();
    while (method == nullptr) {
        frame.Advance();
        if (frame.Done()) {
            break;
        }
        method = frame.CheckAndGetMethod();
    }
    std::string faultInfo;
    if (method != nullptr) {
        std::string methodName = method->GetMethodName();
        std::string recordName = method->GetRecordName().c_str();
        faultInfo = "Method Name:" + methodName + " Record Name:" + recordName;
    } else {
        faultInfo = "method is nullptr!";
    }
    if (isBuiltinStub) {
        uintptr_t func = uctx->uc_mcontext.regs[2];  // 2: func
        JSTaggedValue builtinMethod = JSFunction::Cast(reinterpret_cast<TaggedObject *>(func))->GetMethod();
        uint8_t builtinId = Method::Cast(builtinMethod.GetTaggedObject())->GetBuiltinId();
        size_t builtinStart = static_cast<size_t>(GET_MESSAGE_STRING_ID(CharCodeAt) - 1);  // 1: offset NONE
        std::string builtinStr = MessageString::GetMessageString(builtinStart + builtinId);
        faultInfo += " " + builtinStr;
    }
    if (memcpy_s(buf, len, faultInfo.c_str(), faultInfo.length()) != EOK) {
        LOG_ECMA(ERROR) << "memcpy_s fail in CrashCallback()!";  // not FATAL to avoid further crash
    }
#endif
}

bool ReadUintptrFromAddr(int pid, uintptr_t addr, uintptr_t &value, bool needCheckRegion)
{
    if (pid == getpid()) {
        if (needCheckRegion) {
            bool flag = false;
            auto callback = [addr, &flag](Region *region) {
                uintptr_t regionBegin = region->GetBegin();
                uintptr_t regionEnd = region->GetEnd();
                if (regionBegin <= addr && addr <= regionEnd) {
                    flag = true;
                }
            };
            if (JsStackInfo::loader != nullptr) {
                const Heap *heap = JsStackInfo::loader->GetHeap();
                if (heap != nullptr) {
                    heap->EnumerateRegions(callback);
                }
            }
            if (!flag) {
                LOG_ECMA(ERROR) << "addr not in Region, addr: " << addr;
                return false;
            }
        }
        value = *(reinterpret_cast<uintptr_t *>(addr));
        return true;
    }
    long *retAddr = reinterpret_cast<long *>(&value);
    // note: big endian
    for (size_t i = 0; i < sizeof(uintptr_t) / sizeof(long); i++) {
        *retAddr = PtracePeektext(pid, addr);
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
    FrameType type = static_cast<FrameType>(frameType);
    switch (type) {
        case FrameType::OPTIMIZED_FRAME:
            typeOffset = OptimizedFrame::GetTypeOffset();
            prevOffset = OptimizedFrame::GetPrevOffset();
            break;
        case FrameType::OPTIMIZED_ENTRY_FRAME:
            typeOffset = OptimizedEntryFrame::GetTypeOffset();
            prevOffset = OptimizedEntryFrame::GetLeaveFrameFpOffset();
            break;
        case FrameType::ASM_BRIDGE_FRAME:
            typeOffset = AsmBridgeFrame::GetTypeOffset();
            prevOffset = AsmBridgeFrame::GetPrevOffset();
            break;
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME:
            typeOffset = OptimizedJSFunctionUnfoldArgVFrame::GetTypeOffset();
            prevOffset = OptimizedJSFunctionUnfoldArgVFrame::GetPrevOffset();
            break;
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME:
            typeOffset = OptimizedJSFunctionFrame::GetTypeOffset();
            prevOffset = OptimizedJSFunctionFrame::GetPrevOffset();
            break;
        case FrameType::LEAVE_FRAME:
            typeOffset = OptimizedLeaveFrame::GetTypeOffset();
            prevOffset = OptimizedLeaveFrame::GetPrevOffset();
            break;
        case FrameType::LEAVE_FRAME_WITH_ARGV:
            typeOffset = OptimizedWithArgvLeaveFrame::GetTypeOffset();
            prevOffset = OptimizedWithArgvLeaveFrame::GetPrevOffset();
            break;
        case FrameType::BUILTIN_CALL_LEAVE_FRAME:
            typeOffset = OptimizedBuiltinLeaveFrame::GetTypeOffset();
            prevOffset = OptimizedBuiltinLeaveFrame::GetPrevOffset();
            break;
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME:
            typeOffset = InterpretedFrame::GetTypeOffset();
            prevOffset = InterpretedFrame::GetPrevOffset();
            break;
        case FrameType::INTERPRETER_BUILTIN_FRAME:
            typeOffset = InterpretedBuiltinFrame::GetTypeOffset();
            prevOffset = InterpretedBuiltinFrame::GetPrevOffset();
            break;
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
        case FrameType::ASM_INTERPRETER_FRAME:
            typeOffset = AsmInterpretedFrame::GetTypeOffset();
            prevOffset = AsmInterpretedFrame::GetPrevOffset();
            break;
        case FrameType::BUILTIN_FRAME:
        case FrameType::BUILTIN_ENTRY_FRAME:
            typeOffset = BuiltinFrame::GetTypeOffset();
            prevOffset = BuiltinFrame::GetPrevOffset();
            break;
        case FrameType::BUILTIN_FRAME_WITH_ARGV:
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME:
            typeOffset = BuiltinWithArgvFrame::GetTypeOffset();
            prevOffset = BuiltinWithArgvFrame::GetPrevOffset();
            break;
        case FrameType::INTERPRETER_ENTRY_FRAME:
            typeOffset = InterpretedEntryFrame::GetTypeOffset();
            prevOffset = InterpretedEntryFrame::GetPrevOffset();
            break;
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            typeOffset = AsmInterpretedEntryFrame::GetTypeOffset();
            prevOffset = AsmInterpretedEntryFrame::GetPrevOffset();
            break;
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
            typeOffset = AsmInterpretedBridgeFrame::GetTypeOffset();
            prevOffset = AsmInterpretedBridgeFrame::GetPrevOffset();
            break;
        default:
            return false;
    }
    return true;
}

bool StepArkManagedNativeFrame(int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp,
                               [[maybe_unused]] char *buf, [[maybe_unused]] size_t bufSize)
{
    constexpr size_t FP_SIZE = 8;
    constexpr size_t LR_SIZE = 8;
    uintptr_t currentPtr = *fp;
    if (currentPtr == 0) {
        LOG_ECMA(ERROR) << "fp is nullptr in StepArkManagedNativeFrame()!";
        return false;
    }
    if (pid == getpid() && JsStackInfo::loader != nullptr &&
        !JsStackInfo::loader->InsideStub(*pc) && !JsStackInfo::loader->InsideAOT(*pc)) {
        LOG_ECMA(ERROR) << "invalid pc in StepArkManagedNativeFrame()!";
        return false;
    }
    while (true) {
        currentPtr -= sizeof(FrameType);
        uintptr_t frameType = 0;
        if (!ReadUintptrFromAddr(pid, currentPtr, frameType, true)) {
            return false;
        }
        uintptr_t typeOffset = 0;
        uintptr_t prevOffset = 0;
        if (!GetTypeOffsetAndPrevOffsetFromFrameType(frameType, typeOffset, prevOffset)) {
            LOG_ECMA(ERROR) << "FrameType ERROR, addr: " << currentPtr << ", frameType: " << frameType;
            return false;
        }
        if (static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_ENTRY_FRAME ||
            static_cast<FrameType>(frameType) == FrameType::ASM_INTERPRETER_ENTRY_FRAME ||
            static_cast<FrameType>(frameType) == FrameType::BUILTIN_ENTRY_FRAME) {
            break;
        }
        currentPtr -= typeOffset;
        currentPtr += prevOffset;
        if (!ReadUintptrFromAddr(pid, currentPtr, currentPtr, true)) {
            return false;
        }
        if (currentPtr == 0) {
            LOG_ECMA(ERROR) << "currentPtr is nullptr in StepArkManagedNativeFrame()!";
            return false;
        }
    }
    currentPtr += sizeof(FrameType);
    *fp = currentPtr;
    currentPtr += FP_SIZE;
    if (!ReadUintptrFromAddr(pid, currentPtr, *pc, true)) {
        return false;
    }
    currentPtr += LR_SIZE;
    *sp = currentPtr;
    return true;
}

void CopyBytecodeInfoToBuffer(const char *prefix, uintptr_t fullBytecode, size_t &strIdx, char *outStr, size_t strLen)
{
    // note: big endian
    for (size_t i = 0; prefix[i] != '\0' && strIdx < strLen - 1; i++) {  // 1: last '\0'
        outStr[strIdx++] = prefix[i];
    }
    size_t start = static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleLdundefined));
    size_t bytecode = fullBytecode & 0xff;  // 0xff: last byte
    const char *bytecodeName = MessageString::GetMessageString(start + bytecode).c_str();
    for (size_t i = 0; bytecodeName[i] != '\0' && strIdx < strLen - 1; i++) {  // 1: last '\0'
        outStr[strIdx++] = bytecodeName[i];
    }
    if (start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleDeprecated)) ||
        start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleWide)) ||
        start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleThrow)) ||
        start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleCallRuntime))) {
        size_t startSecond = start;
        if (start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleDeprecated))) {
            startSecond = static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleDeprecatedLdlexenvPrefNone));
        } else if (start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleWide))) {
            startSecond = static_cast<size_t>(GET_MESSAGE_STRING_ID(
                HandleWideCreateobjectwithexcludedkeysPrefImm16V8V8));
        } else if (start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleThrow))) {
            startSecond = static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleThrowPrefNone));
        } else if (start + bytecode == static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleCallRuntime))) {
            startSecond = static_cast<size_t>(GET_MESSAGE_STRING_ID(HandleCallRuntimeNotifyConcurrentResultPrefNone));
        }
        size_t bytecodeSecond = (fullBytecode >> 8) & 0xff;  // 8, 0xff: second last byte
        const char *bytecodeNameSecond = MessageString::GetMessageString(startSecond + bytecodeSecond).c_str();
        if (strIdx < strLen - 1) {  // 1: last '\0'
            outStr[strIdx++] = '/';
        }
        for (size_t i = 0; bytecodeNameSecond[i] != '\0' && strIdx < strLen - 1; i++) {  // 1: last '\0'
            outStr[strIdx++] = bytecodeNameSecond[i];
        }
    }
    outStr[strIdx] = '\0';
}

bool GetArkJSHeapCrashInfo(int pid, uintptr_t *bytecodePc, uintptr_t *fp, bool outJSInfo, char *outStr, size_t strLen)
{
    // bytecodePc: X20 in ARM
    // fp: X29 in ARM
    // outJSInfo: not async-safe, more info
    uintptr_t currentPtr = *fp;
    if (currentPtr == 0) {
        LOG_ECMA(ERROR) << "fp is nullptr in GetArkJSHeapCrashInfo()!";
        return false;
    }
    currentPtr -= sizeof(FrameType);
    uintptr_t frameType = 0;
    if (!ReadUintptrFromAddr(pid, currentPtr, frameType, false)) {
        return false;
    }
    if (static_cast<FrameType>(frameType) != FrameType::ASM_INTERPRETER_FRAME) {
        return false;
    }
    size_t strIndex = 0;
    uintptr_t registerBytecode = 0;
    if (!ReadUintptrFromAddr(pid, *bytecodePc, registerBytecode, false)) {
        return false;
    }
    CopyBytecodeInfoToBuffer("RegisterBytecode:", registerBytecode, strIndex, outStr, strLen);
    uintptr_t typeOffset = MEMBER_OFFSET(AsmInterpretedFrame, base) + MEMBER_OFFSET(InterpretedFrameBase, type);
    uintptr_t pcOffset = MEMBER_OFFSET(AsmInterpretedFrame, pc);
    currentPtr -= typeOffset;
    currentPtr += pcOffset;
    uintptr_t framePc = 0;
    uintptr_t frameBytecode = 0;
    if (!ReadUintptrFromAddr(pid, currentPtr, framePc, false)) {
        return false;
    }
    if (!ReadUintptrFromAddr(pid, framePc, frameBytecode, false)) {
        return false;
    }
    CopyBytecodeInfoToBuffer(" FrameBytecode:", frameBytecode, strIndex, outStr, strLen);
    if (outJSInfo) {
        uintptr_t functionOffset = MEMBER_OFFSET(AsmInterpretedFrame, function);
        currentPtr -= pcOffset;
        currentPtr += functionOffset;
        uintptr_t functionAddress = 0;
        if (!ReadUintptrFromAddr(pid, currentPtr, functionAddress, false)) {
            return false;
        }
        JSTaggedValue functionValue(static_cast<JSTaggedType>(functionAddress));
        Method *method = ECMAObject::Cast(functionValue.GetTaggedObject())->GetCallTarget();
        auto bytecodeOffset = static_cast<uint32_t>(reinterpret_cast<uint8_t *>(*bytecodePc) -
                                                    method->GetBytecodeArray());
        std::string info = JsStackInfo::BuildMethodTrace(method, bytecodeOffset);
        const char *infoChar = info.c_str();
        if (strIndex < strLen - 1) {  // 1: last '\0'
            outStr[strIndex++] = ' ';
        }
        for (size_t i = 0; infoChar[i] != '\0' && strIndex < strLen - 1; i++) {  // 1: last '\0'
            outStr[strIndex++] = infoChar[i];
        }
        outStr[strIndex] = '\0';
    }
    return true;
}
} // namespace panda::ecmascript

__attribute__((visibility("default"))) int step_ark_managed_native_frame(
    int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp, char *buf, size_t buf_sz)
{
    if (panda::ecmascript::StepArkManagedNativeFrame(pid, pc, fp, sp, buf, buf_sz)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int get_ark_js_heap_crash_info(
    int pid, uintptr_t *x20, uintptr_t *fp, int outJsInfo, char *buf, size_t buf_sz)
{
    if (panda::ecmascript::GetArkJSHeapCrashInfo(pid, x20, fp, outJsInfo != 0, buf, buf_sz)) {
        return 1;
    }
    return -1;
}