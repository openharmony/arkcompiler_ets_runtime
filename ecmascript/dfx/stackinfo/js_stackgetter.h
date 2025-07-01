/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_STACKINFO_JS_STACKGETTER_H
#define ECMASCRIPT_DFX_STACKINFO_JS_STACKGETTER_H

#include <csignal>

#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/interpreter/frame_handler.h"

namespace panda::ecmascript {
enum class RunningState : size_t {
    OTHER = 0,
    GC,
    CINT,
    AINT,
    AOT,
    AINT_D,
    BUILTIN,
    NAPI,
    ARKUI_ENGINE,
    RUNTIME,
    JIT
};

struct MethodKey {
    void *methodIdentifier = nullptr;
    RunningState state = RunningState::OTHER;
    kungfu::DeoptType deoptType = kungfu::DeoptType::NONE;
    int lineNumber = 0;
    bool operator < (const MethodKey &methodKey) const
    {
        return state < methodKey.state ||
               (state == methodKey.state && methodIdentifier < methodKey.methodIdentifier) ||
               (state == methodKey.state && methodIdentifier == methodKey.methodIdentifier &&
               deoptType < methodKey.deoptType) ||
               (state == methodKey.state && methodIdentifier == methodKey.methodIdentifier &&
               deoptType == methodKey.deoptType && lineNumber < methodKey.lineNumber);
    }
};

struct NodeKey {
    struct MethodKey methodKey = {0};
    int parentId = 0;
    bool operator < (const NodeKey &nodeKey) const
    {
        return parentId < nodeKey.parentId ||
               (parentId == nodeKey.parentId && methodKey < nodeKey.methodKey);
    }
};

struct FrameInfoTemp {
    char codeType[20] = {0}; // 20:the maximum size of the codeType
    char functionName[100] = {0}; // 100:the maximum size of the functionName
    char recordName[500] = {0}; // 500:the maximum size of the recordName
    int columnNumber = -1;
    int lineNumber = -1;
    int scriptId = 0;
    char url[500] = {0}; // 500:the maximum size of the url
    struct MethodKey methodKey = {0};
};

class JsStackGetter {
public:
    static bool CheckFrameType(JSThread *thread, JSTaggedType *sp);
    static bool ParseMethodInfo(struct MethodKey &methodKey,
                                const FrameIterator &it,
                                const EcmaVM *vm,
                                FrameInfoTemp &codeEntry,
                                bool isCpuProfiler = false);
    static bool CheckAndCopy(char *dest, size_t length, const char *src);
    static void GetNativeStack(const EcmaVM *vm, const FrameIterator &it, char *functionName,
                               size_t size, bool isCpuProfiler);
    static RunningState GetRunningState(const FrameIterator &it, const EcmaVM *vm, bool isNative,
                                        bool topFrame, bool enableVMTag = false);
    static void GetNativeMethodCallPos(FrameIterator &it, const EcmaVM *vm, FrameInfoTemp &codeEntry);
    static void *GetMethodIdentifier(Method *method, const FrameIterator &it, const EcmaVM *vm);
    static void GetCallLineNumber(const FrameIterator &it, const EcmaVM *vm, int &LineNumber);
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKGETTER_H
