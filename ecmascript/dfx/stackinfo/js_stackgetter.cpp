/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/stackinfo/js_stackgetter.h"

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
class CpuProfiler;
class SamplesRecord;

bool JsStackGetter::CheckFrameType(JSThread *thread, JSTaggedType *sp)
{
    FrameType type = FrameHandler::GetFrameType(sp);
    if (type > FrameType::FRAME_TYPE_LAST || type < FrameType::FRAME_TYPE_FIRST) {
        return false;
    }

    FrameIterator iterator(sp, thread);
    iterator.Advance();
    JSTaggedType *preSp = iterator.GetSp();
    if (preSp == nullptr) {
        return true;
    }
#if defined(PANDA_TARGET_64)
    if (thread->IsAsmInterpreter() && !thread->IsLegalSp(reinterpret_cast<uintptr_t>(preSp))) {
        return false;
    }
#endif
    type = FrameHandler::GetFrameType(preSp);
    if (type > FrameType::FRAME_TYPE_LAST || type < FrameType::FRAME_TYPE_FIRST) {
        return false;
    }
    return true;
}

bool JsStackGetter::ParseMethodInfo(struct MethodKey &methodKey,
                                    const FrameIterator &it,
                                    const EcmaVM *vm,
                                    FrameInfoTemp &codeEntry,
                                    bool isCpuProfiler)
{
    auto method = it.CheckAndGetMethod();
    if (method == nullptr) {
        return false;
    }
    codeEntry.methodKey = methodKey;
    if (method->IsNativeWithCallField()) {
        FrameIterator itNext(it.GetSp(), it.GetThread());
        itNext.Advance<GCVisitedFlag::IGNORED>();
        GetNativeMethodCallPos(itNext, vm, codeEntry);
        GetNativeStack(vm, it, codeEntry.functionName, sizeof(codeEntry.functionName), isCpuProfiler);
    } else {
        const JSPandaFile *jsPandaFile = method->GetJSPandaFile(vm->GetJSThreadNoCheck());
        if (jsPandaFile == nullptr) {
            return false;
        }
        EntityId methodId = reinterpret_cast<MethodLiteral *>(methodKey.methodIdentifier)->GetMethodId();
        // function name
        const char *functionName = MethodLiteral::GetMethodName(jsPandaFile, methodId, true);
        uint8_t length = strlen(functionName);
        if (length != 0 && functionName[0] == '#') {
            uint8_t index = length - 1;
            while (functionName[index] != '#') {
                index--;
            }
            functionName += (index + 1);  // #...#functionName
        }
        if (strlen(functionName) == 0) {
            functionName = "anonymous";
        }
        if (!CheckAndCopy(codeEntry.functionName, sizeof(codeEntry.functionName), functionName)) {
            return false;
        }
        uint8_t specialIndex = strlen(codeEntry.functionName) - 1;
        while (specialIndex > 0 && codeEntry.functionName[specialIndex] != '^') {
            specialIndex--;
        }
        if (codeEntry.functionName[specialIndex] == '^') {
            codeEntry.functionName[specialIndex] = '\0';  // #...#functionName^1
        }
        // record name
        const char *recordName = MethodLiteral::GetRecordNameWithSymbol(jsPandaFile, methodId);
        if (strlen(recordName) != 0) {
            if (!CheckAndCopy(codeEntry.recordName, sizeof(codeEntry.recordName), recordName)) {
                return false;
            }
        }

        DebugInfoExtractor *debugExtractor =
            JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
        if (debugExtractor == nullptr) {
            return false;
        }
        // source file
        const std::string &sourceFile = debugExtractor->GetSourceFile(methodId);
        if (!sourceFile.empty()) {
            if (!CheckAndCopy(codeEntry.url, sizeof(codeEntry.url), sourceFile.c_str())) {
                return false;
            }
        }
        // line number and clomn number
        codeEntry.lineNumber = debugExtractor->GetFristLine(methodId);
        codeEntry.columnNumber = debugExtractor->GetFristColumn(methodId);
    }
    return true;
}

bool JsStackGetter::CheckAndCopy(char *dest, size_t length, const char *src)
{
    int srcLength = strlen(src);
    if (length <= static_cast<size_t>(srcLength) || strcpy_s(dest, srcLength + 1, src) != EOK) {
        LOG_ECMA(ERROR) << "JsStackGetter strcpy_s failed, maybe srcLength more than destLength";
        return false;
    }
    dest[srcLength] = '\0';
    return true;
}

void JsStackGetter::GetNativeStack(const EcmaVM *vm, const FrameIterator &it, char *functionName, size_t size,
                                   bool isCpuProfiler)
{
    std::stringstream stream;
    JSFunction* function = JSFunction::Cast(it.GetFunction().GetTaggedObject());
    // it not allow thread check here, if enable thread check, it maybe deadlock in IsInThreadPool
    JSThread *thread = vm->GetJSThreadNoCheck();
    JSTaggedValue extraInfoValue = function->GetNativeFunctionExtraInfo(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    JSHandle<JSTaggedValue> func(thread, function);
    JSHandle<JSTaggedValue> funcNameValue = JSObject::GetProperty(thread, func, nameKey).GetValue();
    std::string methodNameStr;
    if (funcNameValue->IsString()) {
        JSHandle<EcmaString> methodName(funcNameValue);
        methodNameStr = EcmaStringAccessor(methodName).ToStdString(thread);
    }
    // napi method
    if (isCpuProfiler && function->IsCallNapi() && extraInfoValue.CheckIsJSNativePointer()) {
        JSNativePointer *extraInfo = JSNativePointer::Cast(extraInfoValue.GetTaggedObject());
        auto cb = vm->GetNativePtrGetter();
        if (cb != nullptr  && extraInfo != nullptr) {
            if (!vm->GetJSThreadNoCheck()->CpuProfilerCheckJSTaggedType(extraInfoValue.GetRawData())) {
                return;
            }
            auto addr = cb(reinterpret_cast<void *>(extraInfo->GetData()));
            stream << addr;
            if (!CheckAndCopy(functionName, size, methodNameStr.c_str())) {
                return;
            }
            const uint8_t methodNameStrLength = methodNameStr.size();
            if (!CheckAndCopy(functionName + methodNameStrLength, size - methodNameStrLength, "(")) {
                return;
            }
            const uint8_t napiBeginLength = 1; // 1:the length of "("
            if (!CheckAndCopy(functionName + methodNameStrLength + napiBeginLength,
                size - methodNameStrLength - napiBeginLength, stream.str().c_str())) {
                return;
            }
            uint8_t srcLength = stream.str().size();
            CheckAndCopy(functionName + methodNameStrLength + napiBeginLength + srcLength,
                size - methodNameStrLength - napiBeginLength - srcLength, ")");
            return;
        }
    }
    CheckAndCopy(functionName, size, methodNameStr.c_str());
}

RunningState JsStackGetter::GetRunningState(const FrameIterator &it, const EcmaVM *vm,
                                            bool isNative, bool topFrame,
                                            bool enableVMTag)
{
    JSThread *thread = vm->GetAssociatedJSThread();
    JSFunction* function = JSFunction::Cast(it.GetFunction().GetTaggedObject());

    if (enableVMTag) {
        if (topFrame) {
            if (thread->GetGcState()) {
                return RunningState::GC;
            }
            if (isNative && (it.IsLeaveFrame() || thread->GetRuntimeState())) {
                return RunningState::RUNTIME;
            }
        }

        if (function->IsCallNapi()) {
            return RunningState::NAPI;
        }
        if (isNative) {
            return function->GetNativeFunctionExtraInfo(thread).CheckIsJSNativePointer() ? RunningState::ARKUI_ENGINE :
                RunningState::BUILTIN;
        }
        if (it.IsFastJitFunctionFrame()) {
            return RunningState::JIT;
        }
        if (it.IsOptimizedJSFunctionFrame()) {
            return RunningState::AOT;
        }
        if (thread->IsAsmInterpreter()) {
            // For Methods that is compiled in AOT but deoptimized at runtime, we mark it as AINT-D
            Method *method = Method::Cast(function->GetMethod(thread));
            MethodLiteral *methodLiteral = method->GetMethodLiteral(thread);
            if (methodLiteral != nullptr && MethodLiteral::IsAotWithCallField(methodLiteral->GetCallField())) {
                return RunningState::AINT_D;
            }
            return RunningState::AINT;
        }
        return RunningState::CINT;
    }

    if (topFrame) {
        if (function->IsCallNapi()) {
            return RunningState::NAPI;
        }
        if (thread->GetGcState()) {
            return RunningState::GC;
        }
    }

    if (function->IsCallNapi()) {
        return RunningState::NAPI;
    }
    if (isNative) {
        return function->GetNativeFunctionExtraInfo(thread).CheckIsJSNativePointer() ? RunningState::ARKUI_ENGINE :
            RunningState::BUILTIN;
    }

    return RunningState::OTHER;
}

void JsStackGetter::GetNativeMethodCallPos(FrameIterator &it, const EcmaVM *vm, FrameInfoTemp &codeEntry)
{
    JSThread *thread = vm->GetJSThreadNoCheck();
    auto nextMethod = it.CheckAndGetMethod();
    if (nextMethod == nullptr) {
        return ;
    }
    JSFunction* function = JSFunction::Cast(it.GetFunction().GetTaggedObject());
    JSTaggedValue extraInfoValue = function->GetNativeFunctionExtraInfo(thread);
    if (!extraInfoValue.CheckIsJSNativePointer() && nextMethod->GetJSPandaFile(thread) != nullptr) {
        DebugInfoExtractor *debugExtractor =
            JSPandaFileManager::GetInstance()->GetJSPtExtractor(nextMethod->GetJSPandaFile(thread));
        if (debugExtractor == nullptr) {
            return;
        }
        MethodLiteral *methodLiteral = nextMethod->GetMethodLiteral(thread);
        if (methodLiteral == nullptr) {
            return;
        }
        panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
        const std::string &sourceFile = debugExtractor->GetSourceFile(methodId);
        const char *tempVariable;
        if (sourceFile.empty()) {
            tempVariable = "";
        } else {
            tempVariable = sourceFile.c_str();
        }
        if (!CheckAndCopy(codeEntry.url, sizeof(codeEntry.url), tempVariable)) {
            return;
        }
        int lineNumber = 0;
        auto callbackLineFunc = [&lineNumber](int32_t line) -> bool {
            lineNumber = line + 1;
            return true;
        };
        int columnNumber = 0;
        auto callbackColumnFunc = [&columnNumber](int32_t column) -> bool {
            columnNumber += column + 1;
            return true;
        };
        uint32_t offset = it.GetBytecodeOffset();
        if (!debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, offset)) {
            lineNumber = 0;
        }
        if (!debugExtractor->MatchColumnWithOffset(callbackColumnFunc, methodId, offset)) {
            columnNumber = 0;
        }
        codeEntry.lineNumber = lineNumber;
        codeEntry.columnNumber = columnNumber;
    }
}

void *JsStackGetter::GetMethodIdentifier(Method *method, const FrameIterator &it, const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThreadNoCheck();
    JSFunction* function = JSFunction::Cast(it.GetFunction().GetTaggedObject());
    JSTaggedValue extraInfoValue = function->GetNativeFunctionExtraInfo(thread);
    if (method->IsNativeWithCallField()) {
        if (extraInfoValue.CheckIsJSNativePointer()) {
            JSNativePointer *extraInfo = JSNativePointer::Cast(extraInfoValue.GetTaggedObject());
            return reinterpret_cast<void *>(extraInfo->GetData());
        }
        return function->GetNativePointer();
    }

    MethodLiteral *methodLiteral = method->GetMethodLiteral(thread);
    return reinterpret_cast<void *>(methodLiteral);
}
void JsStackGetter::GetCallLineNumber(const FrameIterator &it, const EcmaVM *vm, int &LineNumber)
{
    JSThread *thread = vm->GetJSThreadNoCheck();
    FrameIterator itNext(it.GetSp(), it.GetThread());
    itNext.Advance<GCVisitedFlag::IGNORED>();
    auto nextMethod = itNext.CheckAndGetMethod();
    if (nextMethod == nullptr) {
        return ;
    }
    JSFunction* function = JSFunction::Cast(itNext.GetFunction().GetTaggedObject());
    JSTaggedValue extraInfoValue = function->GetNativeFunctionExtraInfo(thread);
    if (!extraInfoValue.CheckIsJSNativePointer() && nextMethod->GetJSPandaFile(thread) != nullptr) {
        DebugInfoExtractor *debugExtractor =
            JSPandaFileManager::GetInstance()->GetJSPtExtractor(nextMethod->GetJSPandaFile(thread));
        if (debugExtractor == nullptr) {
            return;
        }
        MethodLiteral *methodLiteral = nextMethod->GetMethodLiteral(thread);
        if (methodLiteral == nullptr) {
            return;
        }
        panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
        int lineNum = 0;
        auto callbackLineFunc = [&lineNum](int32_t line) -> bool {
            lineNum = line + 1;
            return true;
        };
        uint32_t offset = itNext.GetBytecodeOffset();
        if (!debugExtractor->MatchLineWithOffset(callbackLineFunc, methodId, offset)) {
            lineNum = 0;
        }
        LineNumber = lineNum;
    }
}
} // namespace panda::ecmascript
