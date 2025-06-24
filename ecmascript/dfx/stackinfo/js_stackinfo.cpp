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


#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/platform/aot_crash_info.h"
#include "ecmascript/platform/file.h"
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
std::unordered_map<EntityId, std::string> JsStackInfo::nameMap;
std::unordered_map<EntityId, std::vector<uint8>> JsStackInfo::machineCodeMap;
JSStackTrace *JSStackTrace::trace_ = nullptr;
std::mutex JSStackTrace::mutex_;
size_t JSStackTrace::count_ = 0;

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
    FdsanExchangeOwnerTag(reinterpret_cast<fd_t>(fd));
    jitDumpElf.WriteJitElfFile(fd);
    Close(reinterpret_cast<fd_t>(fd));
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

std::string JsStackInfo::BuildJsStackTrace(JSThread *thread, bool needNative, const JSHandle<JSObject> &jsErrorObj,
                                           bool needNativeStack, uint32_t depth)
{
    std::string data;
    data.reserve(InitialDeeps * InitialLength);
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    uintptr_t baselineNativePc = 0;

    LastBuilderCache lastCache;
    for (; !it.Done() && depth > 0; it.Advance<GCVisitedFlag::HYBRID_STACK>()) {
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
            bool needBaselineSpecialHandling =
                (it.GetFrameType() == FrameType::ASM_INTERPRETER_FRAME && baselineNativePc != 0);
            if (needBaselineSpecialHandling) {
                // the pcOffste in baseline frame slot is always uint64::max(), so pcOffset should be computed
                JSHandle<JSFunction> function(thread, it.GetFunction());
                pcOffset = RuntimeStubs::RuntimeGetBytecodePcOfstForBaseline(function, baselineNativePc);
                baselineNativePc = 0;
            }
            data += BuildJsStackTraceInfo(thread, method, it, jsErrorObj, lastCache,
                                          needBaselineSpecialHandling, pcOffset);
            --depth;
        } else if (needNative) {
            auto addr = JSFunction::Cast(it.GetFunction().GetTaggedObject())->GetNativePointer();
            std::stringstream strm;
            strm << addr;
            data.append("    at native method (").append(strm.str()).append(")\n");
            --depth;
        }
    }
    if (data.empty() && needNativeStack) {
#if defined(ENABLE_EXCEPTION_BACKTRACE)
        std::ostringstream stack;
        Backtrace(stack, true);
        data = stack.str();
#endif
    }
    return data;
}

std::string JsStackInfo::BuildJsStackTraceInfo(JSThread *thread, Method *method, FrameIterator &it,
                                               const JSHandle<JSObject> &jsErrorObj,
                                               LastBuilderCache &lastCache,
                                               bool needBaselineSpecialHandling, uint32_t pcOffset)
{
    FrameType frameType = it.GetFrameType();
    if (IsFastJitFunctionFrame(frameType)) {
        JSFunction *func = static_cast<JSFunction*>(it.GetFunction().GetTaggedObject());
        if (!jsErrorObj.GetTaggedValue().IsUndefined()) {
            AssembleJitCodeMap(thread, jsErrorObj, func, method, it.GetOptimizedReturnAddr());
        }
    }
    std::vector<std::pair<JSTaggedType, uint32_t>> stackTraceInfos;
    it.GetStackTraceInfos(stackTraceInfos, needBaselineSpecialHandling, pcOffset);
    std::string data;
    for (auto &info : stackTraceInfos) {
        Method *methodInline = ECMAObject::Cast(reinterpret_cast<TaggedObject *>(info.first))->GetCallTarget();
        uint32_t pcOffsetInline = info.second;
        data += BuildMethodTrace(methodInline, pcOffsetInline, lastCache,
                                 thread->GetEnableStackSourceFile());
    }
    return data;
}

void JsStackInfo::BuildCrashInfo(bool isJsCrash, uintptr_t pc, JSThread *thread)
{
    if (JsStackInfo::loader == nullptr) {
        return;
    }
    if (!JsStackInfo::loader->IsEnableAOT() && !Jit::GetInstance()->IsEnableFastJit() &&
        !pgo::PGOProfilerManager::GetInstance()->IsEnable()) {
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
        }
    }
    return jsFrame;
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

    std::string packageName;
    if (sourceMap != nullptr) {
        sourceMap->TranslateUrlPositionBySourceMap(url, lineNumber, columnNumber, packageName);
    }

    size_t urlSize = url.size() + 1;
    size_t nameSize = name.size() + 1;
    size_t packageNameSize = packageName.size() + 1;
    if (strcpy_s(jsFrame.url, urlSize, url.c_str()) != EOK ||
        strcpy_s(jsFrame.functionName, nameSize, name.c_str()) != EOK ||
        strcpy_s(jsFrame.packageName, packageNameSize, packageName.c_str()) != EOK) {
        LOG_ECMA(FATAL) << "jsFrame strcpy_s failed";
        UNREACHABLE();
    }
    jsFrame.line = lineNumber;
    jsFrame.column = columnNumber;
}

bool ArkParseJsFrameInfo(uintptr_t byteCodePc, uintptr_t mapBase, uintptr_t loadOffset,
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
    auto offset = codeInfo->offset;
    ParseJsFrameInfo(jsPandaFile, debugExtractor, EntityId(codeInfo->methodId), offset,
                     *jsFunction, extractor->GetSourceMap());
    SaveFuncName(EntityId(codeInfo->methodId), jsFunction->functionName);
    jsFunction->codeBegin = byteCodePc - offset;
    jsFunction->codeSize = codeInfo->codeSize;
    return true;
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

uintptr_t ArkGetFunction(void *ctx, ReadMemFunc readMem, uintptr_t currentPtr)
{
    // only for jit frame
    uintptr_t funcAddr = currentPtr;
    funcAddr -= FASTJITFunctionFrame::GetTypeOffset();
    funcAddr += FASTJITFunctionFrame::GetFunctionOffset();
    uintptr_t function = 0;
    if (!readMem(ctx, funcAddr, &function)) {
        return 0;
    }
    return function;
}

bool ArkGetNextFrame(void *ctx, ReadMemFunc readMem, uintptr_t &currentPtr,
                     uintptr_t &frameType, uintptr_t &pc)
{
    currentPtr -= sizeof(FrameType);
    if (!readMem(ctx, currentPtr, &frameType)) {
        return false;
    }
    FrameIterator::TryRemoveLazyDeoptFlag(frameType);
    if (ArkFrameCheck(frameType)) {
        return true;
    }
    bool ret = false;
    if (IsJsFunctionFrame(frameType) || IsNativeFunctionFrame(frameType)) {
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
    return ArkGetNextFrame(ctx, readMem, currentPtr, frameType, pc);
}

bool ArkGetMethodIdWithJit(ArkUnwindParam *arkUnwindParam, uintptr_t currentPtr)
{
    // only for jit frame
    uintptr_t function = ArkGetFunction(arkUnwindParam->ctx, arkUnwindParam->readMem, currentPtr);
    if (!function) {
        LOG_ECMA(DEBUG) << "Failed to get function";
        return false;
    }
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
    return true;
}

bool ArkGetNextFrameWithJit(ArkUnwindParam *arkUnwindParam, uintptr_t &currentPtr, uintptr_t &frameType)
{
    currentPtr -= sizeof(FrameType);
    if (!arkUnwindParam->readMem(arkUnwindParam->ctx, currentPtr, &frameType)) {
        return false;
    }
    FrameIterator::TryRemoveLazyDeoptFlag(frameType);
    if (ArkFrameCheck(frameType)) {
        return true;
    }
    bool ret = false;
    if (IsJsFunctionFrame(frameType) ||
        IsNativeFunctionFrame(frameType)) {
        *arkUnwindParam->pc = GetBytecodeOffset(arkUnwindParam->ctx, arkUnwindParam->readMem, frameType, currentPtr);
        ret = true;
    } else if (IsFastJitFunctionFrame(frameType)) {
        *arkUnwindParam->pc = GetBytecodeOffset(arkUnwindParam->ctx, arkUnwindParam->readMem, frameType, currentPtr);
        ret = ArkGetMethodIdWithJit(arkUnwindParam, currentPtr);
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
        LOG_ECMA(ERROR) << "ArkGetNextFrameWithJit failed, currentPtr: " << currentPtr << ", frameType: " << frameType;
        return false;
    }
    return true;
}

bool StepArk(void *ctx, ReadMemFunc readMem, ArkStepParam *arkStepParam)
{
    constexpr size_t FP_SIZE = sizeof(uintptr_t);
    uintptr_t currentPtr = *arkStepParam->fp;
    if (currentPtr == 0) {
        return false;
    }

    uintptr_t frameType = 0;
    if (ArkGetNextFrame(ctx, readMem, currentPtr, frameType, *arkStepParam->pc)) {
        if (ArkFrameCheck(frameType)) {
            currentPtr += sizeof(FrameType);
            *arkStepParam->sp = currentPtr;
            bool ret = readMem(ctx, currentPtr, arkStepParam->fp);
            currentPtr += FP_SIZE;
            ret &= readMem(ctx, currentPtr, arkStepParam->pc);
            *arkStepParam->isJsFrame = false;
            return ret;
        } else {
            *arkStepParam->fp = currentPtr;
            *arkStepParam->sp = currentPtr;
            // js -> true, native -> false
            *arkStepParam->isJsFrame = IsJsFunctionFrame(frameType);
        }
    } else {
        return false;
    }

    return true;
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

bool ArkParseJSFileInfo([[maybe_unused]] uintptr_t byteCodePc, [[maybe_unused]] uintptr_t mapBase,
                        [[maybe_unused]] const char* filePath, [[maybe_unused]] uintptr_t extractorptr,
                        [[maybe_unused]] JsFunction *jsFunction)
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
    ret = ArkParseJsFrameInfo(byteCodePc, mapBase, extractor->GetLoadOffset(),
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
    jsPandaFile_ = std::make_shared<JSPandaFile>(pf.release(), "", CreateMode::DFX);
}

void JSSymbolExtractor::CreateJSPandaFile(uint8_t *data, size_t dataSize)
{
    auto pf = panda_file::OpenPandaFileFromSecureMemory(data, dataSize);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "Failed to open panda file.";
        return;
    }
    jsPandaFile_ = std::make_shared<JSPandaFile>(pf.release(), "", CreateMode::DFX);
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

void JSStackTrace::AddReference()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (count_ == 0) {
        trace_ = new JSStackTrace();
    }
    ++count_;
    LOG_ECMA(INFO) << "Add reference, count: " << count_;
}

void JSStackTrace::ReleaseReference()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (trace_ == nullptr) {
        return ;
    }
    --count_;
    LOG_ECMA(INFO) << "Release reference, count: " << count_;
    if (count_ == 0) {
        delete trace_;
        trace_ = nullptr;
    }
}

JSStackTrace::~JSStackTrace()
{
    {
        std::unique_lock<std::shared_mutex> lock(infosMutex_);
        methodInfos_.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(pfMutex_);
        jsPandaFiles_.clear();
    }
}

bool JSStackTrace::InitializeMethodInfo(uintptr_t mapBase)
{
    auto pandafile = FindJSpandaFile(mapBase);
    if (pandafile != nullptr) {
        return true;
    }
    pandafile =
        JSPandaFileManager::GetInstance()->FindJSPandaFileByMapBase(mapBase);
    if (pandafile == nullptr) {
        LOG_ECMA(ERROR) << "Find pandafile failed, mapBase: " << std::hex << mapBase;
        return false;
    }
    auto methodInfos = ReadAllMethodInfos(pandafile);
    SetMethodInfos(mapBase, methodInfos);
    SetJSpandaFile(mapBase, pandafile);
    return true;
}

std::shared_ptr<JSPandaFile> JSStackTrace::FindJSpandaFile(uintptr_t mapBase)
{
    std::shared_lock<std::shared_mutex> lock(pfMutex_);
    auto iter = jsPandaFiles_.find(mapBase);
    if (iter == jsPandaFiles_.end()) {
        return nullptr;
    }
    return iter->second;
}

void JSStackTrace::SetJSpandaFile(uintptr_t mapBase, std::shared_ptr<JSPandaFile> pandafile)
{
    std::unique_lock<std::shared_mutex> lock(pfMutex_);
    jsPandaFiles_.emplace(mapBase, pandafile);
}

const CVector<MethodInfo> &JSStackTrace::FindMethodInfos(uintptr_t mapBase)
{
    std::shared_lock<std::shared_mutex> lock(infosMutex_);
    auto iter = methodInfos_.find(mapBase);
    if (iter == methodInfos_.end()) {
        return methodInfo_;
    }
    return iter->second;
}

void JSStackTrace::SetMethodInfos(uintptr_t mapBase, CVector<MethodInfo> &infos)
{
    std::unique_lock<std::shared_mutex> lock(infosMutex_);
    methodInfos_.emplace(mapBase, std::move(infos));
}

bool JSStackTrace::GetJsFrameInfo(uintptr_t byteCodePc, uintptr_t mapBase,
                                  uintptr_t loadOffset, JsFunction *jsFunction)
{
    if (!InitializeMethodInfo(mapBase)) {
        return false;
    }
    loadOffset = loadOffset % PageSize();
    byteCodePc = byteCodePc - loadOffset;
    auto infos = FindMethodInfos(mapBase);
    auto codeInfo = TranslateByteCodePc(byteCodePc, infos);
    if (!codeInfo) {
        LOG_ECMA(ERROR) << std::hex << "Failed to get methodId, pc: " << byteCodePc;
        return false;
    }
    auto offset = codeInfo->offset;
    auto pandafile = FindJSpandaFile(mapBase);
    auto debugInfoExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(pandafile.get());
    ParseJsFrameInfo(pandafile.get(), debugInfoExtractor, EntityId(codeInfo->methodId), offset, *jsFunction);
    jsFunction->codeBegin = byteCodePc - offset;
    jsFunction->codeSize = codeInfo->codeSize;
    return true;
}

void ArkCreateLocal()
{
    JSStackTrace::AddReference();
}

bool ArkParseJsFrameInfoLocal(uintptr_t byteCodePc, uintptr_t mapBase,
                              uintptr_t loadOffset, JsFunction *jsFunction)
{
    auto trace = JSStackTrace::GetInstance();
    if (trace == nullptr) {
        LOG_ECMA(ERROR) << "singleton is null, need create first.";
        return false;
    }
    return trace->GetJsFrameInfo(byteCodePc, mapBase, loadOffset, jsFunction);
}

void ArkDestoryLocal()
{
    JSStackTrace::ReleaseReference();
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

__attribute__((visibility("default"))) int ark_destroy_local()
{
    panda::ecmascript::ArkDestoryLocal();
    return 1;
}

__attribute__((visibility("default"))) int ark_create_local()
{
    panda::ecmascript::ArkCreateLocal();
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
    void *ctx, panda::ecmascript::ReadMemFunc readMem, panda::ecmascript::ArkStepParam *arkStepParam)
{
    if (panda::ecmascript::StepArk(ctx, readMem, arkStepParam)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_parse_js_frame_info(
    uintptr_t byteCodePc, uintptr_t mapBase, uintptr_t loadOffset, uint8_t *data,
    uint64_t dataSize, uintptr_t extractorptr, panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkParseJsFrameInfo(byteCodePc, mapBase, loadOffset, data,
                                               dataSize, extractorptr, jsFunction)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_parse_js_file_info(
    uintptr_t byteCodePc, uintptr_t mapBase, const char* filePath, uintptr_t extractorptr,
    panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkParseJSFileInfo(byteCodePc, mapBase, filePath, extractorptr, jsFunction)) {
        return 1;
    }
    return -1;
}

__attribute__((visibility("default"))) int ark_parse_js_frame_info_local(
    uintptr_t byteCodePc, uintptr_t mapBase, uintptr_t loadOffset, panda::ecmascript::JsFunction *jsFunction)
{
    if (panda::ecmascript::ArkParseJsFrameInfoLocal(byteCodePc, mapBase, loadOffset, jsFunction)) {
        return 1;
    }
    return -1;
}
