/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TOOLING_BACKEND_DEBUGGER_API_H
#define ECMASCRIPT_TOOLING_BACKEND_DEBUGGER_API_H

#include <functional>

#include "ecmascript/common.h"
#include "ecmascript/debugger/js_debugger_interface.h"
#include "ecmascript/debugger/js_pt_method.h"
#include "ecmascript/jspandafile/scope_info_extractor.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/lexical_env.h"

namespace panda {
namespace ecmascript {
class EcmaVM;
class FrameHandler;
class JSThread;
class Method;
class ModuleManager;
class NameDictionary;
class SourceTextModule;
class DebugInfoExtractor;
namespace tooling {
class JSDebugger;
}
}  // ecmascript
}  // panda

namespace panda::ecmascript::tooling {
enum StackState {
    CONTINUE,
    FAILED,
    SUCCESS,
};

class PUBLIC_API DebuggerApi {
public:
    // FrameHandler
    static uint32_t GetStackDepth(const EcmaVM *ecmaVm);
    static std::shared_ptr<FrameHandler> NewFrameHandler(const EcmaVM *ecmaVm);
    static bool StackWalker(const EcmaVM *ecmaVm, std::function<StackState(const FrameHandler *)> func);

    static uint32_t GetBytecodeOffset(const EcmaVM *ecmaVm);
    static uint32_t GetBytecodeOffset(const FrameHandler *frameHandler);
    static std::unique_ptr<PtMethod> GetMethod(const EcmaVM *ecmaVm);
    static Method *GetMethod(const FrameHandler *frameHandler);
    static bool IsNativeMethod(const EcmaVM *ecmaVm);
    static bool IsNativeMethod(const FrameHandler *frameHandler);
    static JSPandaFile *GetJSPandaFile(const EcmaVM *ecmaVm);

    static JSTaggedValue GetEnv(const FrameHandler *frameHandler);
    static JSTaggedType *GetSp(const FrameHandler *frameHandler);
    static int32_t GetVregIndex(const FrameHandler *frameHandler, std::string_view name);
    static Local<JSValueRef> GetVRegValue(const EcmaVM *ecmaVm,
                                          const FrameHandler *frameHandler, size_t index);
    static void SetVRegValue(FrameHandler *frameHandler, size_t index, Local<JSValueRef> value);

    static Local<JSValueRef> GetProperties(const EcmaVM *ecmaVm, const FrameHandler *frameHandler,
                                           int32_t level, uint32_t slot);
    static void SetProperties(const EcmaVM *vm, const FrameHandler *frameHandler, int32_t level,
                              uint32_t slot, Local<JSValueRef> value);
    static std::pair<int32_t, uint32_t> GetLevelSlot(const FrameHandler *frameHandler, std::string_view name);
    static Local<JSValueRef> GetGlobalValue(const EcmaVM *vm, Local<StringRef> name);
    static bool SetGlobalValue(const EcmaVM *vm, Local<StringRef> name, Local<JSValueRef> value);

    // JSThread
    static Local<JSValueRef> GetAndClearException(const EcmaVM *ecmaVm);
        static JSTaggedValue GetCurrentModule(const EcmaVM *ecmaVm);
    static JSHandle<JSTaggedValue> GetImportModule(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                                   std::string &name);
    static int32_t GetModuleVariableIndex(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                          std::string &name);
    static int32_t GetRequestModuleIndex(const EcmaVM *ecmaVm, const JSTaggedValue moduleRequest,
                                         const JSHandle<JSTaggedValue> &currentModule);
    static Local<JSValueRef> GetModuleValue(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                            std::string &name);
    static bool SetModuleValue(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                               std::string &name, Local<JSValueRef> value);
    static void InitializeExportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                          const JSHandle<JSTaggedValue> &currentModule);
    static void GetLocalExportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                        const JSHandle<JSTaggedValue> &currentModule, bool isImportStar);
    static void GetIndirectExportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                           const JSHandle<JSTaggedValue> &currentModule);
    static void GetImportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                   const JSHandle<JSTaggedValue> &currentModule);
    static void SetException(const EcmaVM *ecmaVm, Local<JSValueRef> exception);
    static void ClearException(const EcmaVM *ecmaVm);
    static bool IsExceptionCaught(const EcmaVM *ecmaVm);

    // NumberHelper
    static double StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix);

    // JSDebugger
    static JSDebugger *CreateJSDebugger(const EcmaVM *ecmaVm);
    static void DestroyJSDebugger(JSDebugger *debugger);
    static void RegisterHooks(JSDebugger *debugger, PtHooks *hooks);
    static bool SetBreakpoint(JSDebugger *debugger, const JSPtLocation &location,
        Local<FunctionRef> condFuncRef);
    static bool RemoveBreakpoint(JSDebugger *debugger, const JSPtLocation &location);
    static void RemoveAllBreakpoints(JSDebugger *debugger);
    static void HandleUncaughtException(const EcmaVM *ecmaVm, std::string &message);
    static Local<JSValueRef> EvaluateViaFuncCall(EcmaVM *ecmaVm, Local<FunctionRef> funcRef,
        std::shared_ptr<FrameHandler> &frameHandler);
    static Local<FunctionRef> GenerateFuncFromBuffer(const EcmaVM *ecmaVm, const void *buffer, size_t size,
        std::string_view entryPoint);

    // HotReload
    static DebugInfoExtractor *GetPatchExtractor(const EcmaVM *ecmaVm, const std::string &url);
    static const JSPandaFile *GetBaseJSPandaFile(const EcmaVM *ecmaVm, const JSPandaFile *jsPandaFile);
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_BACKEND_DEBUGGER_API_H
