/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <sys/time.h>

#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/platform/aot_crash_info.h"
#include "ecmascript/platform/os.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"
#include "ecmascript/jit/jit.h"
#if defined(PANDA_TARGET_OHOS)
#include "ecmascript/extractortool/src/extractor.h"
#endif
#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif

namespace panda::ecmascript {
[[maybe_unused]] static bool g_needCheck = true;
std::unordered_map<EntityId, std::string> JsStackInfo::nameMap;
std::unordered_map<EntityId, std::vector<uint8>> JsStackInfo::machineCodeMap;
JSStackTrace *JSStackTrace::trace_ = nullptr;
std::mutex JSStackTrace::mutex_;

bool IsFastJitFunctionFrame(const FrameType frameType)
{
    return frameType == FrameType::FASTJIT_FUNCTION_FRAME || frameType == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
}

bool IsFastJitFunctionFrame(uintptr_t frameType)
{
    return static_cast<FrameType>(frameType) == FrameType::FASTJIT_FUNCTION_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
}

std::string JsStackInfo::BuildMethodTrace(Method *method, uint32_t pcOffset, LastBuilderCache &lastCache,
                                          bool enableStackSourceFile)
{
    std::string data;
    data.reserve(InitialLength);
    data.append("    at ");
    std::string name = method->ParseFunctionName();
    if (name.empty()) {
        data.append("anonymous (");
    } else {
        data.append(name).append(" (");
    }
    // source file
    DebugInfoExtractor *debugExtractor = nullptr;
    const JSPandaFile *pandaFile = method->GetJSPandaFile();
    if (pandaFile == lastCache.pf) {
        debugExtractor = lastCache.extractor;
    } else {
        debugExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(pandaFile);
        lastCache.pf = pandaFile;
        lastCache.extractor = debugExtractor;
    }
    if (enableStackSourceFile) {
        const std::string &sourceFile = debugExtractor->GetSourceFile(method->GetMethodId());
        if (sourceFile.empty()) {
            data.push_back('?');
        } else {
            data += sourceFile;
        }
    } else {
        data.append("hidden");
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
    data.append(")\n");
    return data;
}

std::string JsStackInfo::BuildInlinedMethodTrace(const JSPandaFile *pf, std::map<uint32_t, uint32_t> &methodOffsets)
{
    std::string data;
    std::map<uint32_t, uint32_t>::reverse_iterator it;
    for (it = methodOffsets.rbegin(); it != methodOffsets.rend(); it++) {
        uint32_t methodId = it->second;
        std::string name;
        if (methodId == 0) {
            name = "unknown";
        } else {
            name = std::string(MethodLiteral::GetMethodName(pf, EntityId(methodId)));
            if (name == "") {
                name = "anonymous";
            }
        }
        data.append("    at ");
        data.append(name);
        data.append(" (maybe inlined).");
        data.append(" depth: ");
        data.append(std::to_string(it->first));

        data.push_back('\n');
    }
    return data;
}

void JsStackInfo::DumpJitCode(JSThread *thread)
{
    JSTaggedType exception = thread->GetException().GetRawData();
    auto &jitCodeMaps = thread->GetJitCodeMaps();
    auto jitCode = jitCodeMaps.find(exception);
    if (jitCode == jitCodeMaps.end()) {
        return;
    }
    std::set<MachineCode*> memos;
    JsJitDumpElf jitDumpElf;
    jitDumpElf.Init();
    int64 idx = 0;
    size_t offset = 0;
    auto jitCodeVec = jitCodeMaps[exception];
    for (size_t i = 0; i < jitCodeVec->size(); i++) {
        auto item = (*jitCodeVec)[i];
        auto machineCode = std::get<0>(item);
        std::string methodName = std::get<1>(item);
        uintptr_t pcOffset = std::get<2>(item);
        auto res = memos.insert(machineCode);
        if (res.second) {
            LOG_ECMA(ERROR) << "jit : js crash at method : " << methodName << ", offset :" << pcOffset;
            char *funcAddr = reinterpret_cast<char *>(machineCode->GetFuncAddr());
            size_t len = machineCode->GetTextSize();
            std::vector<uint8> vec(len);
            if (memmove_s(vec.data(), len, funcAddr, len) != EOK) {
                LOG_ECMA(ERROR) << "Fail to get machineCode on function addr: " << funcAddr;
            }
            jitDumpElf.AppendData(vec);
            jitDumpElf.AppendSymbolToSymTab(idx++, offset, len, methodName);
            offset += len;
        }
    }
    std::string fileName = "jitCode-" + std::to_string(getpid());
    std::string realOutPath;
    std::string sanboxPath = panda::os::file::File::GetExtendedFilePath(AotCrashInfo::GetSandBoxPath());
    if (!ecmascript::RealPath(sanboxPath, realOutPath, false)) {
        return;
    }
    std::string outFile = realOutPath + "/" + fileName;
    int fd = open(outFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    jitDumpElf.WriteJitElfFile(fd);
    close(fd);
}

void AssembleJitCodeMap(JSThread *thread, const JSHandle<JSObject> &jsErrorObj, JSFunction *func, Method *method,
                        uintptr_t offset)
{
    ASSERT(!jsErrorObj.GetTaggedValue().IsUndefined());
    JSTaggedValue machineCodeTagVal = func->GetMachineCode();
    MachineCode *machineCode = MachineCode::Cast(machineCodeTagVal.GetTaggedObject());
    std::string methodName = method->ParseFunctionName();
    if (methodName.empty()) {
        methodName = "anonymous";
    }
    thread->SetJitCodeMap(jsErrorObj.GetTaggedValue().GetRawData(), machineCode, methodName, offset);
}

std::string JsStackInfo::BuildJsStackTrace(JSThread *thread, bool needNative, const JSHandle<JSObject> &jsErrorObj)
{
    std::string data;
    data.reserve(InitialDeeps * InitialLength);
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    uintptr_t baselineNativePc = 0;

    LastBuilderCache lastCache;
    for (; !it.Done(); it.Advance<GCVisitedFlag::HYBRID_STACK>()) {
        if (it.GetFrameType() == FrameType::BASELINE_BUILTIN_FRAME) {
            auto *frame = it.GetFrame<BaselineBuiltinFrame>();
            baselineNativePc = frame->GetReturnAddr();
            continue;
        }
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr) {
            continue;
        }
        if (!method->IsNativeWithCallField()) {
            uint32_t pcOffset = 0;
            if (it.GetFrameType() == FrameType::ASM_INTERPRETER_FRAME && baselineNativePc != 0) {
                // the pcOffste in baseline frame slot is always uint64::max(), so pcOffset should be computed
                JSHandle<JSFunction> function(thread, it.GetFunction());
                pcOffset = RuntimeStubs::RuntimeGetBytecodePcOfstForBaseline(function, baselineNativePc);
                baselineNativePc = 0;
            } else {
                pcOffset = it.GetBytecodeOffset();
            }
            data += BuildJsStackTraceInfo(thread, method, it, pcOffset, jsErrorObj, lastCache);
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

std::string JsStackInfo::BuildJsStackTraceInfo(JSThread *thread, Method *method, FrameIterator &it,
                                               uint32_t pcOffset, const JSHandle<JSObject> &jsErrorObj,
                                               LastBuilderCache &lastCache)
{
    const JSPandaFile *pf = method->GetJSPandaFile();
    std::map<uint32_t, uint32_t> methodOffsets = it.GetInlinedMethodInfo();
    FrameType frameType = it.GetFrameType();
    if (IsFastJitFunctionFrame(frameType)) {
        JSFunction *func = static_cast<JSFunction*>(it.GetFunction().GetTaggedObject());
        if (!jsErrorObj.GetTaggedValue().IsUndefined()) {
            AssembleJitCodeMap(thread, jsErrorObj, func, method, it.GetOptimizedReturnAddr());
        }
    }
    return BuildInlinedMethodTrace(pf, methodOffsets) +
           BuildMethodTrace(method, pcOffset, lastCache, thread->GetEnableStackSourceFile());
}

void JsStackInfo::BuildCrashInfo(bool isJsCrash, uintptr_t pc, JSThread *thread)
{
    if (JsStackInfo::loader == nullptr || JsStackInfo::options == nullptr) {
        return;
    }
    if (!JsStackInfo::loader->IsEnableAOT() && !Jit::GetInstance()->IsEnableFastJit() &&
        !JsStackInfo::options->IsEnablePGOProfiler()) {
        return;
    }
    ohos::RuntimeInfoType type;
    if (isJsCrash) {
        type = ohos::RuntimeInfoType::JS;
    } else if (pc != 0 && JsStackInfo::loader != nullptr && JsStackInfo::loader->InsideAOT(pc)) {
        type = ohos::RuntimeInfoType::AOT_CRASH;
    } else {
        type = ohos::RuntimeInfoType::OTHERS;
    }
    ohos::AotRuntimeInfo::GetInstance().BuildCrashRuntimeInfo(type);
    if (isJsCrash && thread != nullptr) {
        DumpJitCode(thread);
    }
}

std::vector<struct JsFrameInfo> JsStackInfo::BuildJsStackInfo(JSThread *thread, bool currentStack)
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
            jsFrame.push_back(std::move(frameInfo));
            if (currentStack) {
                return jsFrame;
            }
        } else {
            JSTaggedValue function = it.GetFunction();
            JSHandle<JSTaggedValue> extraInfoValue(
                thread, JSFunctionBase::Cast(function.GetTaggedObject())->GetFunctionExtraInfo());
            if (extraInfoValue->IsJSNativePointer()) {
                JSHandle<JSNativePointer> extraInfo(extraInfoValue);
                native = reinterpret_cast<uintptr_t *>(extraInfo->GetData());
            }
        }
    }
    return jsFrame;
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
        case FrameType::BASELINE_BUILTIN_FRAME:
            typeOffset = BaselineBuiltinFrame::GetTypeOffset();
            prevOffset = BaselineBuiltinFrame::GetPrevOffset();
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
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME:
            typeOffset = FASTJITFunctionFrame::GetTypeOffset();
            prevOffset = FASTJITFunctionFrame::GetPrevOffset();
            break;
        default:
            return false;
    }
    return true;
}

bool ArkFrameCheck(uintptr_t frameType)
{
    return static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_ENTRY_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::ASM_INTERPRETER_ENTRY_FRAME;
}

bool IsJsFunctionFrame(uintptr_t frameType)
{
    return static_cast<FrameType>(frameType) == FrameType::ASM_INTERPRETER_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::INTERPRETER_CONSTRUCTOR_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::INTERPRETER_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::INTERPRETER_FAST_NEW_FRAME;
}

bool IsNativeFunctionFrame(uintptr_t frameType)
{
    return static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::BASELINE_BUILTIN_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::ASM_BRIDGE_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::LEAVE_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::LEAVE_FRAME_WITH_ARGV ||
           static_cast<FrameType>(frameType) == FrameType::BUILTIN_CALL_LEAVE_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::BUILTIN_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::BUILTIN_ENTRY_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::BUILTIN_FRAME_WITH_ARGV ||
           static_cast<FrameType>(frameType) == FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::ASM_INTERPRETER_BRIDGE_FRAME;
}

bool IsAotFunctionFrame(uintptr_t frameType)
{
    return static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
           static_cast<FrameType>(frameType) == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
}

std::optional<MethodInfo> JSStackTrace::ReadMethodInfo(panda_file::MethodDataAccessor &mda)
{
    uintptr_t methodId = mda.GetMethodId().GetOffset();
    auto codeId = mda.GetCodeId();
    if (!codeId) {
        return std::nullopt;
    }
    panda_file::CodeDataAccessor cda(mda.GetPandaFile(), codeId.value());
    uint32_t codeSize = cda.GetCodeSize();
    uintptr_t codeBegin = reinterpret_cast<uintptr_t>(cda.GetInstructions());
    return std::make_optional<MethodInfo>(methodId, codeBegin, codeSize);
}

CVector<MethodInfo> JSStackTrace::ReadAllMethodInfos(std::shared_ptr<JSPandaFile> jsPandaFile)
{
    CVector<MethodInfo> result;
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "Failed to read all methods info.";
        return result;
    }
    const panda_file::File *pf = jsPandaFile->GetPandaFile();
    Span<const uint32_t> classIndexes = jsPandaFile->GetClasses();
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (jsPandaFile->IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(*pf, classId);
        cda.EnumerateMethods([&result, jsPandaFile](panda_file::MethodDataAccessor &mda) {
            auto info = JSStackTrace::ReadMethodInfo(mda);
            if (!info) {
                return;
            }
            result.push_back(info.value());
        });
    }

    std::sort(result.begin(), result.end());
    return result;
}

std::optional<CodeInfo> JSStackTrace::TranslateByteCodePc(uintptr_t realPc, const CVector<MethodInfo> &vec)
{
    if (vec.size() == 0) {
        LOG_ECMA(ERROR) << "Translate bytecode pc failed, vec is empty.";
        return std::nullopt;
    }
    int32_t left = 0;
    int32_t right = static_cast<int32_t>(vec.size()) - 1;
    for (; left <= right;) {
        int32_t mid = (left + right) / 2;
        bool isRight = realPc >= (vec[mid].codeBegin + vec[mid].codeSize);
        bool isLeft = realPc < vec[mid].codeBegin;
        // codeBegin <= realPc < codeBegin + codeSize
        if (!isRight && !isLeft) {
            return std::make_optional<CodeInfo>(realPc - vec[mid].codeBegin, vec[mid].methodId, vec[mid].codeSize);
        } else if (isRight) {
            left = mid + 1;
        } else {
            right = mid -1;
        }
    }
    LOG_ECMA(ERROR) << "Translate bytecode pc failed, pc: " << std::hex << realPc;
    return std::nullopt;
}

void SaveFuncName(EntityId entityId, const std::string &name)
{
    size_t length = 256; // maximum stack length
    if (JsStackInfo::nameMap.size() > length) {
        auto it = JsStackInfo::nameMap.begin();
        JsStackInfo::nameMap.erase(it);
    }
    JsStackInfo::nameMap.emplace(entityId, name);
}

template<typename T>
void ParseJsFrameInfo(JSPandaFile *jsPandaFile, DebugInfoExtractor *debugExtractor,
                      EntityId methodId, uintptr_t offset, T &jsFrame, SourceMap *sourceMap)
{
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "Parse jsFrame info failed, jsPandaFile is nullptr.";
        return;
    }
    std::string name = MethodLiteral::ParseFunctionName(jsPandaFile, methodId);
    name = name.empty() ? "anonymous" : name;
    SaveFuncName(methodId, name);
    std::string url = debugExtractor->GetSourceFile(methodId);

    // line number and column number
    int lineNumber = 0;
    int columnNumber = 0;
    auto callbackLineFunc = [&lineNumber](int32_t line) -> bool {
        lineNumber = line + 1;
        return true;
    };
    auto callbackColumnFunc = [&columnNumber](int32_t column) -> bool {
        columnNumber = column + 1;
        return true;
    };

    if (!debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, offset) ||
        !debugExtractor->MatchColumnWithOffset(callbackColumnFunc, methodId, offset)) {
        lineNumber = 0;
        columnNumber = 0;
    }

    if (sourceMap != nullptr) {
        sourceMap->TranslateUrlPositionBySourceMap(url, lineNumber, columnNumber);
    }

    size_t urlSize = url.size() + 1;
    size_t nameSize = name.size() + 1;
    if (strcpy_s(jsFrame.url, urlSize, url.c_str()) != EOK ||
        strcpy_s(jsFrame.functionName, nameSize, name.c_str()) != EOK) {
        LOG_ECMA(FATAL) << "jsFrame strcpy_s failed";
        UNREACHABLE();
    }
    jsFrame.line = lineNumber;
    jsFrame.column = columnNumber;
}

bool ArkParseJsFrameInfo(uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase, uintptr_t loadOffset,
                         uint8_t *data, uint64_t dataSize, uintptr_t extractorptr, JsFunction *jsFunction)
{
    if (data == nullptr) {
        LOG_ECMA(ERROR) << "Parse JSframe info failed, buffer is nullptr.";
        return false;
    }
    loadOffset = loadOffset % PageSize();
    auto extractor = reinterpret_cast<JSSymbolExtractor*>(extractorptr);
    if (extractor == nullptr) {
        LOG_ECMA(ERROR) << "Parse JSframe info failed, extractor is nullptr.";
        return false;
    }
    auto jsPandaFile = extractor->GetJSPandaFile(data, dataSize);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "Parse JSframe info failed, panda file is nullptr.";
        return false;
    }
    auto debugExtractor = extractor->GetDebugExtractor();
    auto methodInfos = extractor->GetMethodInfos();
    if (methodInfos.empty()) {
        LOG_ECMA(ERROR) << "Read all method info from JSPandaFile failed, methodInfos is empty.";
        return false;
    }
    uintptr_t realOffset = byteCodePc - mapBase - loadOffset;
    uintptr_t pfBasePtr = reinterpret_cast<uintptr_t>(jsPandaFile->GetBase());
    auto codeInfo = JSStackTrace::TranslateByteCodePc(realOffset + pfBasePtr, methodInfos);
    if (!codeInfo) {
        LOG_ECMA(ERROR) << std::hex << "Failed to get methodId, pc: " << byteCodePc;
        return false;
    }
    if (!methodId) {
        methodId = codeInfo->methodId;
    }
    auto offset = codeInfo->offset;
    ParseJsFrameInfo(jsPandaFile, debugExtractor, EntityId(methodId), offset, *jsFunction, extractor->GetSourceMap());

    jsFunction->codeBegin = byteCodePc - offset;
    jsFunction->codeSize = codeInfo->codeSize;
    return true;
}

bool ArkTranslateJsFrameInfo(uint8_t *data, size_t dataSize, JsFunction *jsFunction)
{
    SourceMap sourceMap;
    std::string strUrl = jsFunction->url;
    sourceMap.Init(data, dataSize);
    bool ret = sourceMap.TranslateUrlPositionBySourceMap(strUrl, jsFunction->line, jsFunction->column);
    size_t strUrlSize = strUrl.size() + 1;
    if (strcpy_s(jsFunction->url, strUrlSize, strUrl.c_str()) != EOK) {
        LOG_FULL(FATAL) << "strcpy_s failed";
        UNREACHABLE();
    }
    return ret;
}

uintptr_t GetBytecodeOffset(void *ctx, ReadMemFunc readMem, uintptr_t frameType, uintptr_t currentPtr)
{
    // currentPtr points to the frametype.
    uintptr_t bytecodePc = 0;
    FrameType type = static_cast<FrameType>(frameType);
    switch (type) {
        // return bytecode pc
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            currentPtr -= AsmInterpretedFrame::GetTypeOffset();
            currentPtr += AsmInterpretedFrame::GetPcOffset(false);
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            currentPtr -= InterpretedFrame::GetTypeOffset();
            currentPtr += InterpretedFrame::GetPcOffset(false);
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            currentPtr -= FASTJITFunctionFrame::GetTypeOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        // return returnaddr
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME: {
            currentPtr -= OptimizedJSFunctionFrame::GetTypeOffset();
            currentPtr += OptimizedJSFunctionFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::BUILTIN_FRAME:
        case FrameType::BUILTIN_ENTRY_FRAME: {
            currentPtr -= BuiltinFrame::GetTypeOffset();
            currentPtr += BuiltinFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV:
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME: {
            currentPtr -= BuiltinWithArgvFrame::GetTypeOffset();
            currentPtr += BuiltinWithArgvFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::BASELINE_BUILTIN_FRAME: {
            currentPtr -= BaselineBuiltinFrame::GetTypeOffset();
            currentPtr += BaselineBuiltinFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::ASM_BRIDGE_FRAME: {
            currentPtr -= AsmBridgeFrame::GetTypeOffset();
            currentPtr += AsmBridgeFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::LEAVE_FRAME: {
            currentPtr -= OptimizedLeaveFrame::GetTypeOffset();
            currentPtr += OptimizedLeaveFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::LEAVE_FRAME_WITH_ARGV: {
            currentPtr -= OptimizedWithArgvLeaveFrame::GetTypeOffset();
            currentPtr += OptimizedWithArgvLeaveFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME: {
            currentPtr -= OptimizedBuiltinLeaveFrame::GetTypeOffset();
            currentPtr += OptimizedBuiltinLeaveFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::OPTIMIZED_FRAME: {
            currentPtr -= OptimizedFrame::GetTypeOffset();
            currentPtr += OptimizedFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME: {
            currentPtr -= AsmInterpretedBridgeFrame::GetTypeOffset();
            currentPtr += AsmInterpretedBridgeFrame::GetReturnAddrOffset(false);
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            currentPtr -= OptimizedJSFunctionUnfoldArgVFrame::GetTypeOffset();
            currentPtr += OptimizedJSFunctionUnfoldArgVFrame::GetReturnAddrOffset();
            readMem(ctx, currentPtr, &bytecodePc);
            return bytecodePc;
        }
        default: {
            break;
        }
    }
    return 0;
}

uintptr_t ArkGetFunction(void *ctx, ReadMemFunc readMem, uintptr_t currentPtr, uintptr_t frameType)
{
    FrameType type = static_cast<FrameType>(frameType);
    uintptr_t funcAddr = currentPtr;
    switch (type) {
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            funcAddr -= OptimizedJSFunctionFrame::GetTypeOffset();
            funcAddr += OptimizedJSFunctionFrame::GetFunctionOffset();
            break;
        }
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            funcAddr -= AsmInterpretedFrame::GetTypeOffset();
            funcAddr += AsmInterpretedFrame::GetFunctionOffset(false);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            funcAddr -= InterpretedFrame::GetTypeOffset();
            funcAddr += InterpretedFrame::GetFunctionOffset();
            break;
        }
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            funcAddr -= FASTJITFunctionFrame::GetTypeOffset();
            funcAddr += FASTJITFunctionFrame::GetFunctionOffset();
            break;
        }
        default: {
            return 0;
        }
    }
    uintptr_t function = 0;
    if (!readMem(ctx, funcAddr, &function)) {
        return 0;
    }
    return function;
}

bool ArkCheckIsJSFunctionBaseOrJSProxy(void *ctx, ReadMemFunc readMem, uintptr_t objAddr, bool &isJSFunctionBase)
{
    bool isHeapObj = ((objAddr & JSTaggedValue::TAG_HEAPOBJECT_MASK) == 0U);
    bool isInvalidValue = (objAddr <= JSTaggedValue::INVALID_VALUE_LIMIT);
    if (isHeapObj && !isInvalidValue) {
        ASSERT_PRINT(((objAddr & JSTaggedValue::TAG_WEAK) == 0U),
                     "can not convert JSTaggedValue to HeapObject :" << std::hex << objAddr);
        uintptr_t hclassAddr = objAddr + TaggedObject::HCLASS_OFFSET;
        uintptr_t hclass = 0;
        if (!readMem(ctx, hclassAddr, &hclass)) {
            return false;
        }
        if (hclass != 0) {
            uintptr_t bitsAddr = reinterpret_cast<uintptr_t>(hclass + JSHClass::BIT_FIELD_OFFSET);
            uintptr_t bits = 0;
            if (!readMem(ctx, bitsAddr, &bits)) {
                return false;
            }
            JSType jsType = JSHClass::ObjectTypeBits::Decode(bits);
            isJSFunctionBase = (jsType >= JSType::JS_FUNCTION_BASE && jsType <= JSType::JS_BOUND_FUNCTION);
            bool isJSProxy = (jsType == JSType::JS_PROXY);
            return isJSFunctionBase || isJSProxy;
        }
    }
    return false;
}

uintptr_t ArkCheckAndGetMethod(void *ctx, ReadMemFunc readMem, uintptr_t value)
{
    bool isJSFunctionBase = 0;
    if (ArkCheckIsJSFunctionBaseOrJSProxy(ctx, readMem, value, isJSFunctionBase)) {
        if (isJSFunctionBase) {
            value += JSFunctionBase::METHOD_OFFSET;
        } else {
            value += JSProxy::METHOD_OFFSET;
        }
        uintptr_t method = 0;
        if (!readMem(ctx, value, &method)) {
            return 0;
        }
        return method;
    }
    return 0;
}

bool ArkGetMethodIdFromMethod(void *ctx, ReadMemFunc readMem, uintptr_t method, uintptr_t &methodId)
{
    uintptr_t methodLiteralAddr = method + Method::LITERAL_INFO_OFFSET;
    uintptr_t methodLiteral = 0;
    if (!readMem(ctx, methodLiteralAddr, &methodLiteral)) {
        return false;
    }
    methodId = MethodLiteral::MethodIdBits::Decode(methodLiteral);
    return true;
}

bool ArkGetMethodId(void *ctx, ReadMemFunc readMem, uintptr_t frameType, uintptr_t currentPtr, uintptr_t &methodId)
{
    uintptr_t function = ArkGetFunction(ctx, readMem, currentPtr, frameType);
    if (!function) {
        LOG_ECMA(DEBUG) << "Failed to get function";
        return false;
    }

    uintptr_t method = ArkCheckAndGetMethod(ctx, readMem, function);
    if (!method) {
        LOG_ECMA(DEBUG) << std::hex << "Failed to get method: " << function;
        return false;
    }

    if (!ArkGetMethodIdFromMethod(ctx, readMem, method, methodId)) {
        LOG_ECMA(DEBUG) << std::hex << "ArkGetJsFrameDebugInfo failed, method: " << method;
        return false;
    }
    return true;
}

bool ArkGetNextFrame(void *ctx, ReadMemFunc readMem, uintptr_t &currentPtr,
                     uintptr_t &frameType, uintptr_t &pc, uintptr_t *methodId)
{
    currentPtr -= sizeof(FrameType);
    if (!readMem(ctx, currentPtr, &frameType)) {
        return false;
    }
    if (ArkFrameCheck(frameType)) {
        return true;
    }
    bool ret = false;
    if (IsJsFunctionFrame(frameType)) {
        pc = GetBytecodeOffset(ctx, readMem, frameType, currentPtr);
        ret = true;
        if (methodId != nullptr) {
            ret = ArkGetMethodId(ctx, readMem, frameType, currentPtr, *methodId);
        }
    } else if (IsNativeFunctionFrame(frameType)) {
        pc = GetBytecodeOffset(ctx, readMem, frameType, currentPtr);
        ret = true;
    }

    uintptr_t typeOffset = 0;
    uintptr_t prevOffset = 0;
    if (!GetTypeOffsetAndPrevOffsetFromFrameType(frameType, typeOffset, prevOffset)) {
        return false;
    }
    currentPtr -= typeOffset;
    currentPtr += prevOffset;
    if (!readMem(ctx, currentPtr, &currentPtr)) {
        return false;
    }

    if (ret) {
        return true;
    }
    return ArkGetNextFrame(ctx, readMem, currentPtr, frameType, pc, methodId);
}

bool ArkGetMethodIdWithJit(ArkUnwindParam *arkUnwindParam, uintptr_t frameType, uintptr_t currentPtr)
{
    uintptr_t function = ArkGetFunction(arkUnwindParam->ctx, arkUnwindParam->readMem, currentPtr, frameType);
    if (!function) {
        LOG_ECMA(DEBUG) << "Failed to get function";
        return false;
    }

    uintptr_t method = ArkCheckAndGetMethod(arkUnwindParam->ctx, arkUnwindParam->readMem, function);
    if (!method) {
        LOG_ECMA(DEBUG) << std::hex << "Failed to get method: " << function;
        return false;
    }

    if (!ArkGetMethodIdFromMethod(arkUnwindParam->ctx, arkUnwindParam->readMem, method, *arkUnwindParam->methodId)) {
        LOG_ECMA(DEBUG) << std::hex << "ArkGetJsFrameDebugInfo failed, method: " << method;
        return false;
    }

    if (IsFastJitFunctionFrame(frameType)) {
        uintptr_t machineCode = 0;
        uintptr_t functionAddr = function + JSFunction::MACHINECODE_OFFSET;
        arkUnwindParam->readMem(arkUnwindParam->ctx, functionAddr, &machineCode);
        uintptr_t size = 0;
        uintptr_t funcAddr = 0;
        if (machineCode) {
            arkUnwindParam->readMem(arkUnwindParam->ctx, machineCode + MachineCode::INSTRSIZ_OFFSET, &size);
            arkUnwindParam->readMem(arkUnwindParam->ctx, machineCode + MachineCode::FUNCADDR_OFFSET, &funcAddr);
        }
        if (size && funcAddr) {
            // take the lower four bytes
            size &= 0xFFFFFFFF;
            std::vector<uint8> codeVec;
            for (size_t l = 0; l < size; l++) {
                uintptr_t tmp = 0;
                arkUnwindParam->readMem(arkUnwindParam->ctx, funcAddr + l, &tmp);
                codeVec.push_back(tmp);
            }
            arkUnwindParam->jitCache.push_back(*arkUnwindParam->methodId);
            JsStackInfo::machineCodeMap[EntityId(*arkUnwindParam->methodId)] = codeVec;
        }
    }
    return true;
}

bool ArkGetNextFrameWithJit(ArkUnwindParam *arkUnwindParam, uintptr_t &currentPtr, uintptr_t &frameType)
{
    currentPtr -= sizeof(FrameType);
    if (!arkUnwindParam->readMem(arkUnwindParam->ctx, currentPtr, &frameType)) {
        return false;
    }
    if (ArkFrameCheck(frameType)) {
        return true;
    }
    bool ret = false;
    if (IsJsFunctionFrame(frameType) ||
        IsFastJitFunctionFrame(frameType)) {
        *arkUnwindParam->pc = GetBytecodeOffset(arkUnwindParam->ctx, arkUnwindParam->readMem, frameType, currentPtr);
        ret = true;
        if (arkUnwindParam->methodId != nullptr) {
            ret = ArkGetMethodIdWithJit(arkUnwindParam, frameType, currentPtr);
        }
    } else if (IsNativeFunctionFrame(frameType)) {
        *arkUnwindParam->pc = GetBytecodeOffset(arkUnwindParam->ctx, arkUnwindParam->readMem, frameType, currentPtr);
        ret = true;
    }

    uintptr_t typeOffset = 0;
    uintptr_t prevOffset = 0;
    if (!GetTypeOffsetAndPrevOffsetFromFrameType(frameType, typeOffset, prevOffset)) {
        return false;
    }
    currentPtr -= typeOffset;
    currentPtr += prevOffset;
    if (!arkUnwindParam->readMem(arkUnwindParam->ctx, currentPtr, &currentPtr)) {
        return false;
    }

    if (ret) {
        return true;
    }
    return ArkGetNextFrameWithJit(arkUnwindParam, currentPtr, frameType);
}

bool ArkWriteJitCode([[maybe_unused]] void *ctx, [[maybe_unused]] ReadMemFunc readMem,
                     int fd, const uintptr_t *const jitCodeArray, const size_t jitSize)
{
    JsJitDumpElf jitDumpElf;
    jitDumpElf.Init();
    std::set<uintptr_t> memos;
    int64 idx = 0;
    size_t offset = 0;
    for (size_t i = 0; i < jitSize; i++) {
        uintptr_t methodId = jitCodeArray[i];
        auto res = memos.insert(methodId);
        if (res.second) {
            std::vector<uint8> codeVec = JsStackInfo::machineCodeMap[EntityId(methodId)];
            std::string name = JsStackInfo::nameMap[EntityId(methodId)];
            size_t len = codeVec.size();
            jitDumpElf.AppendData(codeVec);
            jitDumpElf.AppendSymbolToSymTab(idx++, offset, len, name);
            offset += len;
        }
    }
    jitDumpElf.WriteJitElfFile(fd);
    JsStackInfo::nameMap.clear();
    JsStackInfo::machineCodeMap.clear();
    return true;
}

bool StepArkWithRecordJit(ArkUnwindParam *arkUnwindParam)
{
    constexpr size_t FP_SIZE = sizeof(uintptr_t);
    uintptr_t currentPtr = *arkUnwindParam->fp;
    if (currentPtr == 0) {
        LOG_ECMA(ERROR) << "fp is nullptr in StepArkWithRecordJit()!";
        return false;
    }

    uintptr_t frameType = 0;
    if (ArkGetNextFrameWithJit(arkUnwindParam, currentPtr, frameType)) {
        if (ArkFrameCheck(frameType)) {
            currentPtr += sizeof(FrameType);
            *arkUnwindParam->sp = currentPtr;
            bool ret = arkUnwindParam->readMem(arkUnwindParam->ctx, currentPtr, arkUnwindParam->fp);
            currentPtr += FP_SIZE;
            ret &= arkUnwindParam->readMem(arkUnwindParam->ctx, currentPtr, arkUnwindParam->pc);
            *arkUnwindParam->isJsFrame = false;
            return ret;
        } else {
            *arkUnwindParam->fp = currentPtr;
            *arkUnwindParam->sp = currentPtr;
            // js && jit -> true, native -> false
            *arkUnwindParam->isJsFrame = IsJsFunctionFrame(frameType) ||
                IsFastJitFunctionFrame(frameType);
        }
    } else {
        LOG_ECMA(ERROR) << "ArkGetNextFrame failed, currentPtr: " << currentPtr << ", frameType: " << frameType;
        return false;
    }
    return true;
}

bool StepArk(void *ctx, ReadMemFunc readMem, uintptr_t *fp, uintptr_t *sp,
             uintptr_t *pc, uintptr_t *methodId, bool *isJsFrame)
{
    constexpr size_t FP_SIZE = sizeof(uintptr_t);
    uintptr_t currentPtr = *fp;
    if (currentPtr == 0) {
        LOG_ECMA(ERROR) << "fp is nullptr in StepArk()!";
        return false;
    }

    uintptr_t frameType = 0;
    if (ArkGetNextFrame(ctx, readMem, currentPtr, frameType, *pc, methodId)) {
        if (ArkFrameCheck(frameType)) {
            currentPtr += sizeof(FrameType);
            *sp = currentPtr;
            bool ret = readMem(ctx, currentPtr, fp);
            currentPtr += FP_SIZE;
            ret &= readMem(ctx, currentPtr, pc);
            *isJsFrame = false;
            return ret;
        } else {
            *fp = currentPtr;
            *sp = currentPtr;
            // js -> true, native -> false
            *isJsFrame = IsJsFunctionFrame(frameType);
        }
    } else {
        LOG_ECMA(ERROR) << std::hex << "ArkGetNextFrame failed, addr: " << currentPtr;
        return false;
    }

    return true;
}

uintptr_t ArkGetFunction(int pid, uintptr_t currentPtr, uintptr_t frameType)
{
    FrameType type = static_cast<FrameType>(frameType);
    uintptr_t funcAddr = currentPtr;
    switch (type) {
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
            funcAddr -= OptimizedJSFunctionFrame::GetTypeOffset();
            funcAddr += OptimizedJSFunctionFrame::GetFunctionOffset();
            break;
        }
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            funcAddr -= AsmInterpretedFrame::GetTypeOffset();
            funcAddr += AsmInterpretedFrame::GetFunctionOffset(false);
            break;
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            funcAddr -= InterpretedFrame::GetTypeOffset();
            funcAddr += InterpretedFrame::GetFunctionOffset();
            break;
        }
        case FrameType::INTERPRETER_BUILTIN_FRAME: {
            funcAddr -= InterpretedBuiltinFrame::GetTypeOffset();
            funcAddr += InterpretedBuiltinFrame::GetFunctionOffset();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV: {
            funcAddr += sizeof(FrameType);
            auto topAddress = funcAddr +
                (static_cast<int>(BuiltinWithArgvFrame::Index::StackArgsTopIndex) * sizeof(uintptr_t));
            uintptr_t argcAddress = static_cast<uintptr_t>(funcAddr + (static_cast<int>
                                    (BuiltinWithArgvFrame::Index::NumArgsIndex) * sizeof(uintptr_t)));
            if (!ReadUintptrFromAddr(pid, argcAddress, argcAddress, g_needCheck)) {
                return 0;
            }
            auto numberArgs = argcAddress + NUM_MANDATORY_JSFUNC_ARGS;
            funcAddr = topAddress - static_cast<uint32_t>(numberArgs) * sizeof(uintptr_t);
            break;
        }
        case FrameType::BUILTIN_ENTRY_FRAME:
        case FrameType::BUILTIN_FRAME: {
            funcAddr -= BuiltinFrame::GetTypeOffset();
            funcAddr += BuiltinFrame::GetStackArgsOffset();
            break;
        }
        case FrameType::BUILTIN_CALL_LEAVE_FRAME: {
            funcAddr -= OptimizedBuiltinLeaveFrame::GetTypeOffset();
            funcAddr += OptimizedBuiltinLeaveFrame::GetFunctionOffset();
            break;
        }
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            funcAddr -= FASTJITFunctionFrame::GetTypeOffset();
            funcAddr += FASTJITFunctionFrame::GetFunctionOffset();
            break;
        }
        case FrameType::BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME :
        case FrameType::OPTIMIZED_FRAME:
        case FrameType::OPTIMIZED_ENTRY_FRAME:
        case FrameType::ASM_BRIDGE_FRAME:
        case FrameType::LEAVE_FRAME:
        case FrameType::LEAVE_FRAME_WITH_ARGV:
        case FrameType::INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
        case FrameType::ASM_INTERPRETER_BRIDGE_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME: {
            return 0;
        }
        default: {
            LOG_FULL(FATAL) << "Unknown frame type: " << static_cast<uintptr_t>(type);
            UNREACHABLE();
        }
    }
    uintptr_t function = 0;
    if (!ReadUintptrFromAddr(pid, funcAddr, function, g_needCheck)) {
        return 0;
    }
    return function;
}

bool ArkCheckIsJSFunctionBaseOrJSProxy(int pid, uintptr_t objAddr, bool &isJSFunctionBase)
{
    bool isHeapObj = ((objAddr & JSTaggedValue::TAG_HEAPOBJECT_MASK) == 0U);
    bool isInvalidValue = (objAddr <= JSTaggedValue::INVALID_VALUE_LIMIT);
    if (isHeapObj && !isInvalidValue) {
        ASSERT_PRINT(((objAddr & JSTaggedValue::TAG_WEAK) == 0U),
                     "can not convert JSTaggedValue to HeapObject :" << std::hex << objAddr);
        uintptr_t hclassAddr = objAddr + TaggedObject::HCLASS_OFFSET;
        uintptr_t hclass = 0;
        if (!ReadUintptrFromAddr(pid, hclassAddr, hclass, g_needCheck)) {
            return false;
        }
        if (hclass != 0) {
            uintptr_t bitsAddr = reinterpret_cast<uintptr_t>(hclass + JSHClass::BIT_FIELD_OFFSET);
            uintptr_t bits = 0;
            if (!ReadUintptrFromAddr(pid, bitsAddr, bits, g_needCheck)) {
                return false;
            }
            JSType jsType = JSHClass::ObjectTypeBits::Decode(bits);
            isJSFunctionBase = (jsType >= JSType::JS_FUNCTION_BASE && jsType <= JSType::JS_BOUND_FUNCTION);
            bool isJSProxy = (jsType == JSType::JS_PROXY);
            return isJSFunctionBase || isJSProxy;
        }
    }
    return false;
}

uintptr_t ArkCheckAndGetMethod(int pid, uintptr_t value)
{
    bool isJSFunctionBase = 0;
    if (ArkCheckIsJSFunctionBaseOrJSProxy(pid, value, isJSFunctionBase)) {
        if (isJSFunctionBase) {
            value += JSFunctionBase::METHOD_OFFSET;
        } else {
            value += JSProxy::METHOD_OFFSET;
        }
        uintptr_t method = 0;
        if (!ReadUintptrFromAddr(pid, value, method, g_needCheck)) {
            return 0;
        }
        return method;
    }
    return 0;
}

bool ArkGetMethodIdandJSPandaFileAddr(int pid, uintptr_t method, uintptr_t &methodId, uintptr_t &jsPandaFileAddr)
{
    uintptr_t methodLiteralAddr = method + Method::LITERAL_INFO_OFFSET;
    uintptr_t methodLiteral = 0;
    if (!ReadUintptrFromAddr(pid, methodLiteralAddr, methodLiteral, g_needCheck)) {
        return false;
    }
    methodId = MethodLiteral::MethodIdBits::Decode(methodLiteral);
    uintptr_t constantpoolAddr = method + Method::CONSTANT_POOL_OFFSET;
    uintptr_t constantpool = 0;
    if (!ReadUintptrFromAddr(pid, constantpoolAddr, constantpool, g_needCheck)) {
        return false;
    }
    if (constantpool == JSTaggedValue::VALUE_UNDEFINED) {
        return false;
    }
    uintptr_t lengthAddr = constantpool + TaggedArray::LENGTH_OFFSET;
    uintptr_t length = 0;
    if (!ReadUintptrFromAddr(pid, lengthAddr, length, g_needCheck)) {
        return false;
    }
    jsPandaFileAddr = constantpool + TaggedArray::DATA_OFFSET +
                    JSTaggedValue::TaggedTypeSize() * (length - ConstantPool::JS_PANDA_FILE_INDEX);
    if (!ReadUintptrFromAddr(pid, jsPandaFileAddr, jsPandaFileAddr, g_needCheck)) {
        return false;
    }
    return true;
}

uint32_t ArkGetOffsetFromMethod(int pid, uintptr_t currentPtr, uintptr_t method)
{
    uintptr_t pc = 0;
    if (!ReadUintptrFromAddr(pid, currentPtr, pc, g_needCheck)) {
        return 0;
    }
    uintptr_t byteCodeArrayAddr = method + Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET;
    uintptr_t byteCodeArray = 0;
    if (!ReadUintptrFromAddr(pid, byteCodeArrayAddr, byteCodeArray, g_needCheck)) {
        return 0;
    }
    uintptr_t offset = pc - byteCodeArray;
    return static_cast<uint32_t>(offset);
}

uint32_t ArkGetBytecodeOffset(int pid, uintptr_t method, uintptr_t frameType, uintptr_t currentPtr)
{
    FrameType type = static_cast<FrameType>(frameType);
    switch (type) {
        case FrameType::ASM_INTERPRETER_FRAME:
        case FrameType::INTERPRETER_CONSTRUCTOR_FRAME: {
            currentPtr -= AsmInterpretedFrame::GetTypeOffset();
            currentPtr += AsmInterpretedFrame::GetPcOffset(false);
            return ArkGetOffsetFromMethod(pid, currentPtr, method);
        }
        case FrameType::INTERPRETER_FRAME:
        case FrameType::INTERPRETER_FAST_NEW_FRAME: {
            currentPtr -= InterpretedFrame::GetTypeOffset();
            currentPtr += InterpretedFrame::GetPcOffset(false);
            return ArkGetOffsetFromMethod(pid, currentPtr, method);
        }
        // aot need stackmaps
        case FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME:
        case FrameType::OPTIMIZED_JS_FUNCTION_FRAME:
        case FrameType::FASTJIT_FUNCTION_FRAME:
        case FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME: {
            break;
        }
        default: {
            break;
        }
    }
    return 0;
}

std::string ArkGetFilePath(std::string &fileName)
{
    auto lastSlash = fileName.rfind("/");
    if (lastSlash == std::string::npos) {
        LOG_ECMA(ERROR) << "ArkGetFilePath can't find fisrt /: " << fileName;
        return "";
    }
    if (lastSlash == 0) {
        LOG_ECMA(ERROR) << "ArkGetFilePath can't find second /: " << fileName;
        return "";
    }

    auto secondLastSlash = fileName.rfind("/", lastSlash - 1);
    if (secondLastSlash == std::string::npos) {
        LOG_ECMA(ERROR) << "ArkGetFilePath can't find second /: " << fileName;
        return "";
    }

    std::string mapPath = fileName.substr(secondLastSlash + 1);
    return mapPath;
}

bool ArkIsNativeWithCallField(int pid, uintptr_t method)
{
    uintptr_t callFieldAddr = method + Method::CALL_FIELD_OFFSET;
    uintptr_t callField = 0;
    if (!ReadUintptrFromAddr(pid, callFieldAddr, callField, g_needCheck)) {
        return true;
    }
    return Method::IsNativeBit::Decode(callField);
}

std::string ArkReadCStringFromAddr(int pid, uintptr_t descAddr)
{
    std::string name;
    bool key = true;
    while (key) {
        uintptr_t desc = 0;
        if (!ReadUintptrFromAddr(pid, descAddr, desc, g_needCheck)) {
            LOG_ECMA(ERROR) << "ArkReadCStringFromAddr failed, descAddr: " << descAddr;
            return name;
        }
        size_t shiftAmount = 8;
        for (size_t i = 0; i < sizeof(long); i++) {
            char bottomEightBits = static_cast<char>(desc);
            desc = desc >> shiftAmount;
            if (!bottomEightBits) {
                key = false;
                break;
            }
            name += bottomEightBits;
        }
        if (!key) {
            break;
        }
        descAddr += sizeof(long);
    }
    return name;
}

std::string ArkGetFileName(int pid, uintptr_t jsPandaFileAddr, std::string &hapPath)
{
    size_t size = sizeof(JSPandaFile) / sizeof(long);
    uintptr_t *jsPandaFilePart = new uintptr_t[size]();
    if (jsPandaFilePart == nullptr) {
        LOG_ECMA(FATAL) << "ArkGetFileName:jsPandaFilePart is nullptr";
    }
    for (size_t i = 0; i < size; i++) {
        if (!ReadUintptrFromAddr(pid, jsPandaFileAddr, jsPandaFilePart[i], g_needCheck)) {
            LOG_ECMA(ERROR) << "ArkGetFilePath failed, jsPandaFileAddr: " << jsPandaFileAddr;
            delete []jsPandaFilePart;
            return "";
        }
        jsPandaFileAddr += sizeof(long);
    }
    JSPandaFile *jsPandaFile = reinterpret_cast<JSPandaFile *>(jsPandaFilePart);

    uintptr_t hapPathAddr = reinterpret_cast<uintptr_t>(
        const_cast<char *>(jsPandaFile->GetJSPandaFileHapPath().c_str()));
    hapPath = ArkReadCStringFromAddr(pid, hapPathAddr);

    uintptr_t descAddr = reinterpret_cast<uintptr_t>(
        const_cast<char *>(jsPandaFile->GetJSPandaFileDesc().c_str()));
    delete []jsPandaFilePart;
    return ArkReadCStringFromAddr(pid, descAddr);
}

std::unique_ptr<uint8_t[]> ArkReadData([[maybe_unused]] const std::string &hapPath,
                                       [[maybe_unused]] const std::string &fileName,
                                       [[maybe_unused]] size_t &dataSize)
{
    std::unique_ptr<uint8_t[]> dataPtr = nullptr;
#if defined(PANDA_TARGET_OHOS)
    bool newCreate = false;
    std::shared_ptr<Extractor> extractor = ExtractorUtil::GetExtractor(
        ExtractorUtil::GetLoadFilePath(hapPath), newCreate);
    if (extractor == nullptr) {
        LOG_ECMA(ERROR) << "Ark read data failed, hapPath: " << hapPath;
        return dataPtr;
    }
    if (!extractor->ExtractToBufByName(fileName, dataPtr, dataSize)) {
        LOG_ECMA(ERROR) << "Ark read data failed, hap/hsp path: " << hapPath << ", file name: " << fileName;
        return dataPtr;
    }
#endif
    return dataPtr;
}

std::shared_ptr<JSPandaFile> OpenJSPandaFileByReadData(const std::string &hapPath, const std::string &fileName)
{
    size_t dataSize = 0;
    auto data = ArkReadData(hapPath, fileName, dataSize);
    if (data == nullptr) {
        return nullptr;
    }
    auto pf = panda_file::OpenPandaFileFromMemory(data.get(), dataSize);
    if (pf == nullptr) {
        return nullptr;
    }
    return std::make_shared<JSPandaFile>(pf.release(), fileName.c_str());
}

void ArkParseJsFrameDebugInfos([[maybe_unused]] const std::vector<JsFrameDebugInfo> &JsFrameDebugInfos,
                               [[maybe_unused]] size_t size, [[maybe_unused]] JsFrame *jsFrame,
                               [[maybe_unused]] size_t &jsFrameIndex)
{
#if defined(PANDA_TARGET_OHOS)
    jsFrameIndex = 0;
    size = JsFrameDebugInfos.size() > size ? size : JsFrameDebugInfos.size();
    std::unordered_map<std::string, std::shared_ptr<JSPandaFile>> jsPandaFileTable;
    for (size_t i = 0; i < size; ++i) {
        auto fileIter = jsPandaFileTable.find(JsFrameDebugInfos[i].hapPath);
        if (fileIter == jsPandaFileTable.end()) {
            auto jsPandaFile = OpenJSPandaFileByReadData(JsFrameDebugInfos[i].hapPath, JsFrameDebugInfos[i].filePath);
            if (jsPandaFile != nullptr) {
                jsPandaFileTable.emplace(JsFrameDebugInfos[i].hapPath, jsPandaFile);
                auto debugExtractor = std::make_unique<DebugInfoExtractor>(jsPandaFile.get());
                ParseJsFrameInfo(jsPandaFile.get(), debugExtractor.get(), JsFrameDebugInfos[i].methodId,
                    JsFrameDebugInfos[i].offset, jsFrame[jsFrameIndex]);
                jsFrameIndex++;
            }
        } else {
            auto jsPandaFile = fileIter->second;
            auto debugExtractor = std::make_unique<DebugInfoExtractor>(jsPandaFile.get());
            ParseJsFrameInfo(jsPandaFile.get(), debugExtractor.get(), JsFrameDebugInfos[i].methodId,
                JsFrameDebugInfos[i].offset, jsFrame[jsFrameIndex]);
            jsFrameIndex++;
        }
    }
#endif
}

bool ArkGetJsFrameDebugInfo(int pid, uintptr_t currentPtr, uintptr_t frameType,
                            std::vector<JsFrameDebugInfo> &JsFrameDebugInfos)
{
    uintptr_t function = ArkGetFunction(pid, currentPtr, frameType);
    if (!function) {
        return false;
    }

    uintptr_t method = ArkCheckAndGetMethod(pid, function);
    if (!method || ArkIsNativeWithCallField(pid, method)) {
        return false;
    }
    uintptr_t jsPandaFileAddr = 0;
    uintptr_t methodId = 0;
    if (!ArkGetMethodIdandJSPandaFileAddr(pid, method, methodId, jsPandaFileAddr)) {
        LOG_ECMA(ERROR) << "ArkGetJsFrameDebugInfo failed, method: " << method;
        return false;
    }
    uintptr_t offset = ArkGetBytecodeOffset(pid, method, frameType, currentPtr);
    std::string hapPath;
    std::string fileName = ArkGetFileName(pid, jsPandaFileAddr, hapPath);
    if (fileName.empty() || hapPath.empty()) {
        LOG_ECMA(DEBUG) << "ArkGetJsFrameDebugInfo get filename or hapPath failed, fileName: "
                        << fileName << ", hapPath: "<< hapPath;
        return false;
    }
    std::string filePath = ArkGetFilePath(fileName);
    if (filePath.empty()) {
        return false;
    }
    JsFrameDebugInfo JsFrameDebugInfo(EntityId(methodId), offset, hapPath, filePath);
    JsFrameDebugInfos.push_back(std::move(JsFrameDebugInfo));
    return true;
}

bool ArkGetNextFrame(int pid, uintptr_t frameType, uintptr_t &currentPtr)
{
    uintptr_t typeOffset = 0;
    uintptr_t prevOffset = 0;
    if (!GetTypeOffsetAndPrevOffsetFromFrameType(frameType, typeOffset, prevOffset)) {
        LOG_ECMA(ERROR) << "FrameType ERROR, addr: " << currentPtr << ", frameType: " << frameType;
        return false;
    }
    currentPtr -= typeOffset;
    currentPtr += prevOffset;
    if (!ReadUintptrFromAddr(pid, currentPtr, currentPtr, g_needCheck)) {
        return false;
    }
    if (currentPtr == 0) {
        LOG_ECMA(ERROR) << "currentPtr is nullptr in GetArkNativeFrameInfo()!";
        return false;
    }
    return true;
}

bool GetArkNativeFrameInfo([[maybe_unused]] int pid, [[maybe_unused]] uintptr_t *pc,
                           [[maybe_unused]] uintptr_t *fp, [[maybe_unused]] uintptr_t *sp,
                           [[maybe_unused]] JsFrame *jsFrame, [[maybe_unused]] size_t &size)
{
#if defined(PANDA_TARGET_OHOS)
    constexpr size_t FP_SIZE = sizeof(uintptr_t);
    uintptr_t currentPtr = *fp;
    if (pid == getpid()) {
        g_needCheck = false;
    }
    if (currentPtr == 0) {
        LOG_ECMA(ERROR) << "fp is nullptr in GetArkNativeFrameInfo()!";
        return false;
    }

    if (pid == getpid() && JsStackInfo::loader != nullptr &&
        !JsStackInfo::loader->InsideStub(*pc) && !JsStackInfo::loader->InsideAOT(*pc)) {
        LOG_ECMA(ERROR) << "invalid pc in StepArkManagedNativeFrame()!";
        return false;
    }

    std::vector<JsFrameDebugInfo> JsFrameDebugInfos;
    bool ret = true;
    while (true) {
        currentPtr -= sizeof(FrameType);
        uintptr_t frameType = 0;
        if (!ReadUintptrFromAddr(pid, currentPtr, frameType, g_needCheck)) {
            return false;
        }
        if (g_needCheck && (IsJsFunctionFrame(frameType) || IsAotFunctionFrame(frameType))) {
            ArkGetJsFrameDebugInfo(pid, currentPtr, frameType, JsFrameDebugInfos);
        } else if (ArkFrameCheck(frameType)) {
            currentPtr += sizeof(FrameType);
            *sp = currentPtr;
            ret &= ReadUintptrFromAddr(pid, currentPtr, *fp, g_needCheck);
            currentPtr += FP_SIZE;
            ret &= ReadUintptrFromAddr(pid, currentPtr, *pc, g_needCheck);
            break;
        }

        if (!ArkGetNextFrame(pid, frameType, currentPtr)) {
            return false;
        }
    }
    if (g_needCheck && !JsFrameDebugInfos.empty()) {
        ArkParseJsFrameDebugInfos(JsFrameDebugInfos, size, jsFrame, size);
    } else {
        size = 0;
    }
    return ret;
#else
    return false;
#endif
}

uint8_t* JSSymbolExtractor::GetData()
{
    return data_;
}

uintptr_t JSSymbolExtractor::GetLoadOffset()
{
    return loadOffset_;
}

uintptr_t JSSymbolExtractor::GetDataSize()
{
    return dataSize_;
}

bool JSSymbolExtractor::ParseHapFileData([[maybe_unused]] std::string& hapName)
{
    bool ret = false;
#if defined(PANDA_TARGET_OHOS)
    if (hapName.empty()) {
        LOG_ECMA(ERROR) << "Get file data failed, path empty.";
        return false;
    }
    bool newCreate = false;
    std::shared_ptr<Extractor> extractor = ExtractorUtil::GetExtractor(hapName, newCreate);
    if (extractor == nullptr) {
        LOG_ECMA(ERROR) << "GetExtractor failed, hap path: " << hapName;
        return false;
    }

    std::string pandaFilePath = "ets/modules.abc";
    auto data = extractor->GetSafeData(pandaFilePath);
    if (!data) {
        LOG_ECMA(ERROR) << "GetSafeData failed, hap path: " << hapName;
        return false;
    }

    data_ = data->GetDataPtr();
    dataSize_ = data->GetDataLen();
    loadOffset_ = static_cast<uintptr_t>(data->GetOffset());
    ret = true;
    auto zipFile = std::make_unique<ZipFile>(hapName);
    if (zipFile == nullptr || !zipFile->Open()) {
        return false;
    }
    auto &entrys = zipFile->GetAllEntries();
    if (ret) {
        std::string filePath = "ets/sourceMaps.map";
        if (entrys.find(filePath) == entrys.end()) {
            LOG_ECMA(INFO) << "Can't find sourceMaps.map in hap/hsp";
            return ret;
        }
        CreateSourceMap(hapName);
    }
#endif
    return ret;
}

bool ArkParseJSFileInfo([[maybe_unused]] uintptr_t byteCodePc, [[maybe_unused]] uintptr_t methodId,
                        [[maybe_unused]] uintptr_t mapBase, [[maybe_unused]] const char* filePath,
                        [[maybe_unused]] uintptr_t extractorptr, [[maybe_unused]] JsFunction *jsFunction)
{
    bool ret = false;
#if defined(PANDA_TARGET_OHOS)
    if (filePath == nullptr) {
        LOG_ECMA(ERROR) << "FilePath from dfx is nullptr.";
        return false;
    }
    auto extractor = reinterpret_cast<JSSymbolExtractor*>(extractorptr);
    if (extractor == nullptr) {
        LOG_ECMA(ERROR) << "Parse JSframe info failed, extractor is nullptr.";
        return false;
    }
    if (extractor->GetJSPandaFile() == nullptr) {
        std::string hapName = std::string(filePath);
        extractor->ParseHapFileData(hapName);
        extractor->CreateJSPandaFile();
    }
    ret = ArkParseJsFrameInfo(byteCodePc, methodId, mapBase, extractor->GetLoadOffset(),
            extractor->GetData(), extractor->GetDataSize(), extractorptr, jsFunction);
#endif
    return ret;
}

JSSymbolExtractor::~JSSymbolExtractor()
{
    if (sourceMap_ != nullptr) {
        sourceMap_.reset();
    }
    if (debugExtractor_ != nullptr) {
        debugExtractor_.reset();
    }
    if (jsPandaFile_ != nullptr) {
        jsPandaFile_.reset();
    }
    methodInfo_.clear();
}

JSSymbolExtractor* JSSymbolExtractor::Create()
{
    auto extractor = new JSSymbolExtractor();
    return extractor;
}

bool JSSymbolExtractor::Destory(JSSymbolExtractor *extractor)
{
    if (extractor == nullptr) {
        LOG_ECMA(ERROR) << "Destory ark symbol extractor failed, extractor is nullptr.";
        return false;
    }
    delete extractor;
    extractor = nullptr;
    return true;
}

CVector<MethodInfo> JSSymbolExtractor::GetMethodInfos()
{
    if (methodInfo_.empty()) {
        methodInfo_ = JSStackTrace::ReadAllMethodInfos(jsPandaFile_);
    }

    return methodInfo_;
}

JSPandaFile* JSSymbolExtractor::GetJSPandaFile(uint8_t *data, size_t dataSize)
{
    if (jsPandaFile_ == nullptr && data != nullptr) {
        CreateJSPandaFile(data, dataSize);
    }
    return jsPandaFile_.get();
}

void JSSymbolExtractor::CreateJSPandaFile()
{
    auto pf = panda_file::OpenPandaFileFromSecureMemory(data_, dataSize_);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "Failed to open panda file.";
        return;
    }
    jsPandaFile_ = std::make_shared<JSPandaFile>(pf.release(), "");
}

void JSSymbolExtractor::CreateJSPandaFile(uint8_t *data, size_t dataSize)
{
    auto pf = panda_file::OpenPandaFileFromSecureMemory(data, dataSize);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "Failed to open panda file.";
        return;
    }
    jsPandaFile_ = std::make_shared<JSPandaFile>(pf.release(), "");
}

SourceMap* JSSymbolExtractor::GetSourceMap(uint8_t *data, size_t dataSize)
{
    if (sourceMap_ == nullptr && data != nullptr) {
        JSSymbolExtractor::CreateSourceMap(data, dataSize);
    }
    return sourceMap_.get();
}

void JSSymbolExtractor::CreateSourceMap([[maybe_unused]] const std::string &hapPath)
{
#if defined(PANDA_TARGET_OHOS)
    if (sourceMap_ == nullptr) {
        sourceMap_ = std::make_shared<SourceMap>();
        sourceMap_->Init(hapPath);
    }
#endif
}

void JSSymbolExtractor::CreateSourceMap(uint8_t *data, size_t dataSize)
{
    sourceMap_ = std::make_shared<SourceMap>();
    sourceMap_->Init(data, dataSize);
}

DebugInfoExtractor* JSSymbolExtractor::GetDebugExtractor()
{
    if (debugExtractor_ == nullptr) {
        JSSymbolExtractor::CreateDebugExtractor();
    }
    return debugExtractor_.get();
}

void JSSymbolExtractor::CreateDebugExtractor()
{
    debugExtractor_ = std::make_unique<DebugInfoExtractor>(jsPandaFile_.get());
}

uintptr_t ArkCreateJSSymbolExtractor()
{
    auto extractor = JSSymbolExtractor::Create();
    auto extractorptr = reinterpret_cast<uintptr_t>(extractor);
    return extractorptr;
}

bool ArkDestoryJSSymbolExtractor(uintptr_t extractorptr)
{
    auto extractor = reinterpret_cast<JSSymbolExtractor*>(extractorptr);
    return JSSymbolExtractor::Destory(extractor);
}

JSStackTrace *JSStackTrace::GetInstance()
{
    if (trace_ == nullptr) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (trace_ == nullptr) {
            trace_ = new JSStackTrace();
        }
    }

    return trace_;
}

JSStackTrace::~JSStackTrace()
{
    methodInfo_.clear();
    methodInfos_.clear();
    jsPandaFiles_.clear();
}

bool JSStackTrace::AddMethodInfos(uintptr_t mapBase)
{
    auto pandaFile =
        JSPandaFileManager::GetInstance()->FindJSPandaFileByMapBase(mapBase);
    jsPandaFiles_[mapBase] = pandaFile;
    auto methodInfos = JSStackTrace::ReadAllMethodInfos(pandaFile);
    methodInfos_[mapBase] = std::move(methodInfos);
    if (pandaFile == nullptr) {
        LOG_ECMA(ERROR) << "Can't find JSPandaFile by mapBase: " << mapBase;
    }
    return true;
}

bool JSStackTrace::GetJsFrameInfo(uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase,
                                  uintptr_t loadOffset, JsFunction *jsFunction)
{
    bool ret = true;
    auto iter = methodInfos_.find(mapBase);
    if (iter == methodInfos_.end()) {
        ret = AddMethodInfos(mapBase);
    }
    loadOffset = loadOffset % PageSize();
    byteCodePc = byteCodePc - loadOffset;
    auto codeInfo = TranslateByteCodePc(byteCodePc, methodInfos_[mapBase]);
    if (!codeInfo) {
        LOG_ECMA(ERROR) << std::hex << "Failed to get methodId, pc: " << byteCodePc;
        return false;
    }
    if (!methodId) {
        methodId = codeInfo->methodId;
    }
    auto offset = codeInfo->offset;
    auto debugInfoExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFiles_[mapBase].get());
    ParseJsFrameInfo(jsPandaFiles_[mapBase].get(), debugInfoExtractor, EntityId(methodId), offset, *jsFunction);
    jsFunction->codeBegin = byteCodePc - offset;
    jsFunction->codeSize = codeInfo->codeSize;
    return ret;
}

void JSStackTrace::Destory()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (trace_ != nullptr) {
        delete trace_;
        trace_ = nullptr;
    }
}

bool ArkParseJsFrameInfoLocal(uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase,
                              uintptr_t loadOffset, JsFunction *jsFunction)
{
    auto trace = JSStackTrace::GetInstance();
    if (trace == nullptr) {
        LOG_ECMA(ERROR) << "JSStackTrace GetInstance failed.";
        return false;
    }
    return trace->GetJsFrameInfo(byteCodePc, methodId, mapBase, loadOffset, jsFunction);
}

void ArkDestoryLocal()
{
    JSStackTrace::Destory();
}

} // namespace panda::ecmascript

__attribute__((visibility("default"))) int ark_create_js_symbol_extractor(uintptr_t *extractorptr)
{
    *extractorptr = panda::ecmascript::ArkCreateJSSymbolExtractor();
    return 1;
}

__attribute__((visibility("default"))) int ark_destory_js_symbol_extractor(uintptr_t extractorptr)
{
    if (panda::ecmascript::ArkDestoryJSSymbolExtractor(extractorptr)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_destory_local()
{
    panda::ecmascript::ArkDestoryLocal();
    return 1;
}

__attribute__((visibility("default"))) int step_ark_with_record_jit(panda::ecmascript::ArkUnwindParam *arkUnwindParam)
{
    if (panda::ecmascript::StepArkWithRecordJit(arkUnwindParam)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_write_jit_code(
    void *ctx, panda::ecmascript::ReadMemFunc readMem, int fd, const uintptr_t *const jitCodeArray,
    const size_t jitSize)
{
    if (panda::ecmascript::ArkWriteJitCode(ctx, readMem, fd, jitCodeArray, jitSize)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int step_ark(
    void *ctx, panda::ecmascript::ReadMemFunc readMem, uintptr_t *fp, uintptr_t *sp,
    uintptr_t *pc, uintptr_t *methodId, bool *isJsFrame)
{
    if (panda::ecmascript::StepArk(ctx, readMem, fp, sp, pc, methodId, isJsFrame)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_parse_js_frame_info(
    uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase, uintptr_t loadOffset, uint8_t *data,
    uint64_t dataSize, uintptr_t extractorptr, panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkParseJsFrameInfo(byteCodePc, methodId, mapBase, loadOffset, data,
                                               dataSize, extractorptr, jsFunction)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_parse_js_file_info(
    uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase, const char* filePath, uintptr_t extractorptr,
    panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkParseJSFileInfo(byteCodePc, methodId, mapBase, filePath, extractorptr, jsFunction)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_translate_js_frame_info(
    uint8_t *data, size_t dataSize, panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkTranslateJsFrameInfo(data, dataSize, jsFunction)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int get_ark_native_frame_info(
    int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp,
    panda::ecmascript::JsFrame *jsFrame, size_t &size)
{
    if (panda::ecmascript::GetArkNativeFrameInfo(pid, pc, fp, sp, jsFrame, size)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_parse_js_frame_info_local(
    uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase, uintptr_t loadOffset,
    panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkParseJsFrameInfoLocal(byteCodePc, methodId, mapBase, loadOffset, jsFunction)) {
        return 1;
    }
    return -1;
}