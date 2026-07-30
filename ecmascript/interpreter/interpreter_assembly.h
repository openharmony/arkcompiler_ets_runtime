/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
#define ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H

#include "ecmascript/base/config.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/frames.h"
#include "ecmascript/method.h"
#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
using EcmaOpcode = BytecodeInstruction::Opcode;
class ConstantPool;
class ECMAObject;
class GeneratorContext;

class InterpreterAssembly {
public:
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };
    static void InitStackFrame(JSThread *thread);
    static JSTaggedValue Execute(EcmaRuntimeCallInfo *info);
    static JSTaggedValue GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context);
    static inline void MethodEntry(JSThread *thread, Method *method, JSTaggedValue env);

    static JSTaggedValue GetFunction(JSTaggedType *sp);
    static JSTaggedValue GetNewTarget(JSThread *thread, JSTaggedType *sp);
    static JSTaggedValue GetThis(JSTaggedType *sp);
    static JSTaggedValue GetConstantPool(JSThread *thread, JSTaggedType *sp);
    static JSTaggedValue GetUnsharedConstpool(JSThread *thread, JSTaggedType *sp);
    static JSTaggedValue GetModule(JSThread *thread, JSTaggedType *sp);
    static JSTaggedValue GetProfileTypeInfo(JSThread *thread, JSTaggedType *sp);
    static uint32_t GetNumArgs(JSThread *thread, JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx);
    static JSTaggedType *GetAsmInterpreterFramePointer(AsmInterpretedFrame *state);
    static PUBLIC_API int64_t GetCallSize(EcmaOpcode opcode);

private:
    static void InitStackFrameForSP(JSTaggedType *prevSp);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
