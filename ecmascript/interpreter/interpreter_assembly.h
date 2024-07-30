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
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
using DispatchEntryPoint =
    void (*)(JSThread *, const uint8_t *, JSTaggedType *, JSTaggedValue, JSTaggedValue, JSTaggedValue, int16_t);
class ConstantPool;
class ECMAObject;
class GeneratorContext;
struct CallParams;

class InterpreterAssembly {
public:
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };
    static void InitStackFrame(JSThread *thread);
    static void InitStackFrame(EcmaContext *context);
    static JSTaggedValue Execute(EcmaRuntimeCallInfo *info);
    static JSTaggedValue GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context);
    static inline size_t GetJumpSizeAfterCall(const uint8_t *prevPc);
    static inline void MethodEntry(JSThread *thread, Method *method, JSTaggedValue env);

    static inline JSTaggedValue UpdateHotnessCounter(JSThread* thread, JSTaggedType *sp);
    static inline void InterpreterFrameCopyArgs(JSTaggedType *newSp, uint32_t numVregs, uint32_t numActualArgs,
                                                uint32_t numDeclaredArgs, bool haveExtraArgs = true);
    static JSTaggedValue GetFunction(JSTaggedType *sp);
    static JSTaggedValue GetNewTarget(JSTaggedType *sp);
    static JSTaggedValue GetThis(JSTaggedType *sp);
    static JSTaggedValue GetConstantPool(JSTaggedType *sp);
    static JSTaggedValue GetUnsharedConstpool(JSThread* thread, JSTaggedType *sp);
    static JSTaggedValue GetModule(JSTaggedType *sp);
    static JSTaggedValue GetProfileTypeInfo(JSTaggedType *sp);
    static uint32_t GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx);
    static JSTaggedType *GetAsmInterpreterFramePointer(AsmInterpretedFrame *state);

    static bool AssemblyIsFastNewFrameEnter(JSFunction *ctor, JSHandle<Method> method);

#ifndef EXCLUDE_C_INTERPRETER
#define DEF_HANDLER(name)                                                    \
    static void name(JSThread *thread, const uint8_t *pc, JSTaggedType *sp,  \
                     JSTaggedValue constpool, JSTaggedValue profileTypeInfo, \
                     JSTaggedValue acc, int16_t hotnessCounter);
    ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_HANDLER)
    ASM_INTERPRETER_SECOND_BC_STUB_ID_LIST(DEF_HANDLER)
#undef DEF_HANDLER
#endif
private:
    static void InitStackFrameForSP(JSTaggedType *prevSp);
};

#ifndef EXCLUDE_C_INTERPRETER
#define DEF_HANDLER(name) InterpreterAssembly::name,
static std::array<DispatchEntryPoint, BCStubEntries::BC_HANDLER_COUNT> asmDispatchTable {
    ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_HANDLER)
};
static std::array<DispatchEntryPoint, kungfu::BytecodeStubCSigns::NUM_OF_DEPRECATED_STUBS> deprecatedDispatchTable {
    ASM_INTERPRETER_DEPRECATED_STUB_LIST(DEF_HANDLER, DEF_HANDLER, DEF_HANDLER)
};
static std::array<DispatchEntryPoint, kungfu::BytecodeStubCSigns::NUM_OF_WIDE_STUBS> wideDispatchTable {
    ASM_INTERPRETER_WIDE_STUB_LIST(DEF_HANDLER, DEF_HANDLER, DEF_HANDLER)
};
static std::array<DispatchEntryPoint, kungfu::BytecodeStubCSigns::NUM_OF_THROW_STUBS> throwDispatchTable {
    ASM_INTERPRETER_THROW_STUB_LIST(DEF_HANDLER, DEF_HANDLER, DEF_HANDLER)
};
#undef DEF_HANDLER
#endif

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
