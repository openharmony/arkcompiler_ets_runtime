/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_INTERPRETER_INTERPRETER_H
#define ECMASCRIPT_INTERPRETER_INTERPRETER_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/frames.h"
#include "ecmascript/method.h"
#include "ecmascript/require/js_cjs_module.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
class ConstantPool;
class ECMAObject;
class GeneratorContext;

using EcmaOpcode = BytecodeInstruction::Opcode;
const uint8_t EXCEPTION_OPCODE = static_cast<uint8_t>(EcmaOpcode::NOP) + 3;

class EcmaInterpreter {
public:
    static const int16_t METHOD_HOTNESS_THRESHOLD = 0x700;
    static const int16_t METHOD_HOTNESS_THRESHOLD_FACTOR = 10;
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };

    static inline JSTaggedValue Execute(EcmaRuntimeCallInfo *info);
    static inline JSTaggedValue ExecuteNative(EcmaRuntimeCallInfo *info);
    static EcmaRuntimeCallInfo* NewRuntimeCallInfo(
        JSThread *thread, JSHandle<JSTaggedValue> func, JSHandle<JSTaggedValue> thisObj,
        JSHandle<JSTaggedValue> newTarget, uint32_t numArgs, bool needCheckStack = true);
    static EcmaRuntimeCallInfo* ReBuildRuntimeCallInfo(
        JSThread *thread, EcmaRuntimeCallInfo* info, uint32_t numArgs, bool needCheckStack = true);
    static inline JSTaggedValue GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context);
    static inline JSTaggedValue GeneratorReEnterAot(JSThread *thread, JSHandle<GeneratorContext> context);
#ifndef EXCLUDE_C_INTERPRETER
    static inline void RunInternal(JSThread *thread, const uint8_t *pc, JSTaggedType *sp);
#endif
    static inline void InitStackFrame(JSThread *thread);
    static inline void InitStackFrame(EcmaContext *context);
    static inline size_t GetJumpSizeAfterCall(const uint8_t *prevPc);

    static inline JSTaggedValue GetRuntimeProfileTypeInfo(JSTaggedType *sp);
    static inline JSTaggedValue GetConstantPool(JSTaggedType *sp);
    static inline JSTaggedValue GetEcmaModule(JSTaggedType *sp);
    static inline bool UpdateHotnessCounter(JSThread* thread, JSTaggedType *sp, JSTaggedValue acc, int32_t offset);
    static inline void NotifyBytecodePcChanged(JSThread *thread);
    static inline void NotifyDebuggerStmt(JSThread *thread);
    static inline void MethodEntry(JSThread *thread);
    static inline void MethodExit(JSThread *thread);
    static inline const JSPandaFile *GetNativeCallPandafile(JSThread *thread);
    static inline JSTaggedValue GetCurrentEntryPoint(JSThread *thread);
    static inline JSTaggedValue GetFunction(JSTaggedType *sp);
    static inline JSTaggedValue GetNewTarget(JSTaggedType *sp);
    static inline JSTaggedValue GetThis(JSTaggedType *sp);
    static inline uint32_t GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx);
    static inline bool IsFastNewFrameEnter(JSFunction *ctor, JSHandle<Method> method);
    static inline bool IsFastNewFrameExit(JSTaggedType *sp);
    static inline int16_t GetHotnessCounter(uint32_t codeSize);
    static inline JSTaggedType *GetInterpreterFrameEnd(JSThread *thread, JSTaggedType *sp);
private:
    static inline void InitStackFrameForSP(JSTaggedType *prevSp);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_H
