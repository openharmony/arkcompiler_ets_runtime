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
#include "ecmascript/object_factory.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript {
class ConstantPool;
class ECMAObject;
class GeneratorContext;

using EcmaOpcode = BytecodeInstruction::Opcode;
const uint8_t EXCEPTION_OPCODE = static_cast<uint8_t>(EcmaOpcode::NOP) + 8;

class EcmaInterpreter {
public:
    static const int16_t METHOD_HOTNESS_THRESHOLD = 0x700;
    static const int16_t METHOD_HOTNESS_THRESHOLD_FACTOR = 10;
    static const int16_t CANCEL_METHOD_HOTNESS_THRESHOLD = 0;
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };

    static JSTaggedValue Execute(EcmaRuntimeCallInfo *info);
    static JSTaggedValue ExecuteNative(EcmaRuntimeCallInfo *info);
    static EcmaRuntimeCallInfo* NewRuntimeCallInfo(
        JSThread *thread, JSTaggedValue func, JSTaggedValue thisObj, JSTaggedValue newTarget,
        uint32_t numArgs, StackCheck needCheckStack = StackCheck::YES);
    static EcmaRuntimeCallInfo* NewRuntimeCallInfo(
        JSThread *thread, JSHandle<JSTaggedValue> func, JSHandle<JSTaggedValue> thisObj,
        JSHandle<JSTaggedValue> newTarget, uint32_t numArgs, StackCheck needCheckStack = StackCheck::YES);
    static EcmaRuntimeCallInfo* ReBuildRuntimeCallInfo(
        JSThread *thread, EcmaRuntimeCallInfo* info, int numArgs, StackCheck needCheckStack = StackCheck::YES);
    static JSTaggedValue GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context);
    static JSTaggedValue GeneratorReEnterAot(JSThread *thread, JSHandle<GeneratorContext> context);
#ifndef EXCLUDE_C_INTERPRETER
    static void RunInternal(JSThread *thread, const uint8_t *pc, JSTaggedType *sp);
#endif
    static void InitStackFrame(JSThread *thread);
    static void InitStackFrame(EcmaContext *context);
    static size_t GetJumpSizeAfterCall(const uint8_t *prevPc);

    static JSTaggedValue GetRuntimeProfileTypeInfo(JSTaggedType *sp);
    static JSTaggedValue GetConstantPool(JSTaggedType *sp);
    static JSTaggedValue GetUnsharedConstpool(JSThread* thread, JSTaggedType *sp);
    static JSTaggedValue GetEcmaModule(JSTaggedType *sp);
    static bool UpdateHotnessCounter(JSThread* thread, JSTaggedType *sp, JSTaggedValue acc, int32_t offset);
    static void NotifyBytecodePcChanged(JSThread *thread);
    static void NotifyDebuggerStmt(JSThread *thread);
    static void MethodEntry(JSThread *thread);
    static void MethodExit(JSThread *thread);
    static const JSPandaFile *GetNativeCallPandafile(JSThread *thread);
    static std::pair<CString, CString> GetCurrentEntryPoint(JSThread *thread);
    static JSTaggedValue GetFunction(JSTaggedType *sp);
    static JSTaggedValue GetNewTarget(JSTaggedType *sp);
    static JSTaggedValue GetThis(JSTaggedType *sp);
    static uint32_t GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx);
    static bool IsFastNewFrameEnter(JSFunction *ctor, JSHandle<Method> method);
    static bool IsFastNewFrameExit(JSTaggedType *sp);
    static int16_t GetHotnessCounter(uint32_t codeSize, bool cancelThreshold);
    static JSTaggedType *GetInterpreterFrameEnd(JSThread *thread, JSTaggedType *sp);
    static void UpdateProfileTypeInfoCellToFunction(JSThread *thread, JSHandle<JSFunction> &function,
                                                    JSTaggedValue profileTypeInfo, uint16_t slotId);
private:
    static void InitStackFrameForSP(JSTaggedType *prevSp);
    static EcmaRuntimeCallInfo* NewRuntimeCallInfoBase(
        JSThread *thread, JSTaggedType func, JSTaggedType thisObj, JSTaggedType newTarget,
        uint32_t numArgs, StackCheck needCheckStack = StackCheck::YES);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_H
