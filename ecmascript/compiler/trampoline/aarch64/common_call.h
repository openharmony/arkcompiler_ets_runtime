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

#ifndef ECMASCRIPT_COMPILER_AARCH64_EXTENDED_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_AARCH64_EXTENDED_ASSEMBLER_H

#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/assembler/aarch64/extend_assembler.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript::aarch64 {

enum class FrameTransitionType : uint8_t {
    BASELINE_TO_OTHER,
    BASELINE_TO_BASELINE_CHECK,
    OTHER_TO_BASELINE_CHECK,
    OTHER_TO_OTHER
};

using Label = panda::ecmascript::Label;
class CommonCall {
public:
    static constexpr int FRAME_SLOT_SIZE = 8;
    static constexpr int DOUBLE_SLOT_SIZE = 16;
    static constexpr int TRIPLE_SLOT_SIZE = 24;
    static constexpr int QUADRUPLE_SLOT_SIZE = 32;
    static constexpr int QUINTUPLE_SLOT_SIZE = 40;
    static constexpr int OCTUPLE_SLOT_SIZE = 64;
    static constexpr int NONUPLE_SLOT_SIZE = 72;
    static constexpr int FRAME_SLOT_SIZE_LOG2 = 3;
    enum BuiltinsLeaveFrameArgId : unsigned {CODE_ADDRESS = 0, ENV, ARGC, ARGV};
    static inline int64_t GetStackArgOffSetToFp(unsigned argId)
    {
        //   +--------------------------+
        //   |       argv0              | calltarget , newtARGET, this, ....
        //   +--------------------------+ ---
        //   |       argc               |   ^
        //   |--------------------------|   arguments
        //   |       env                |   |
        //   |--------------------------|   |
        //   |       codeAddress        |   |
        //   |--------------------------|   |
        //   |       returnAddr         |   |
        //   |--------------------------| Fixed OptimizedBuiltinLeaveFrame
        //   |       callsiteFp         |   |
        //   |--------------------------|   |
        //   |       frameType          |   v
        //   +--------------------------+ ---
        // 16 : 16 means arguments offset to fp
        return 16 + static_cast<int64_t>(argId) * static_cast<int64_t>(FRAME_SLOT_SIZE);
    }
    static void PushUndefinedWithArgc(ExtendedAssembler *assembler, Register glue, Register argc, Register temp,
        Register fp, Label *next, Label *stackOverflow);
    static void PushArgsWithArgv(ExtendedAssembler *assembler, Register glue, Register argc, Register argv,
        Register op, Register fp, Label *next, Label *stackOverflow);
    static void PushArgsWithArgvInPair(ExtendedAssembler *assembler, Register argc, Register argv, Register padding,
        Register op1, Register op2, Label *next);
    static void PushAsmInterpBridgeFrame(ExtendedAssembler *assembler);
    static void PopAsmInterpBridgeFrame(ExtendedAssembler *assembler);
    static void StackOverflowCheck(ExtendedAssembler *assembler, Register glue, Register currentSlot, Register numArgs,
        Register op, Label *stackOverflow);
    static void PushLeaveFrame(ExtendedAssembler *assembler, Register glue);
    static void PopLeaveFrame(ExtendedAssembler *assembler);
};

class OptimizedCall : public CommonCall {
public:
    static void CallRuntime(ExtendedAssembler *assembler);

    static void DeoptPushAsmInterpBridgeFrame(ExtendedAssembler *assembler, Register context);

    static void JSFunctionEntry(ExtendedAssembler *assembler);

    static void OptimizedCallAndPushArgv(ExtendedAssembler *assembler);

    static void JSProxyCallInternalWithArgV(ExtendedAssembler *assembler);

    static void JSCall(ExtendedAssembler *assembler);

    static void CallOptimized(ExtendedAssembler *assembler);

    static void CallRuntimeWithArgv(ExtendedAssembler *assembler);

    static void JSCallWithArgV(ExtendedAssembler *assembler);

    static void JSCallWithArgVAndPushArgv(ExtendedAssembler *assembler);

    static void SuperCallWithArgV(ExtendedAssembler *assembler);

    static void AOTCallToAsmInterBridge(ExtendedAssembler *assembler);

    static void FastCallToAsmInterBridge(ExtendedAssembler *assembler);

    static void DeoptHandlerAsm(ExtendedAssembler *assembler);

    static void JSCallNew(ExtendedAssembler *assembler);

    static void GenJSCall(ExtendedAssembler *assembler, bool isNew);

    static void GenJSCallWithArgV(ExtendedAssembler *assembler, int id);
private:
    static void DeoptEnterAsmInterpOrBaseline(ExtendedAssembler *assembler);
    static void JSCallCheck(ExtendedAssembler *assembler, Register jsfunc, Register taggedValue,
                            Label *nonCallable, Label *notJSFunction);
    static void ThrowNonCallableInternal(ExtendedAssembler *assembler, Register sp);
    static void JSBoundFunctionCallInternal(ExtendedAssembler *assembler, Register glue,
                                            Register actualArgC, Register jsfunc, int stubId);
    static void OptimizedCallAsmInterpreter(ExtendedAssembler *assembler);
    static void PushMandatoryJSArgs(ExtendedAssembler *assembler, Register jsfunc,
                                    Register thisObj, Register newTarget, Register currentSp);
    static void PopJSFunctionArgs(ExtendedAssembler *assembler, Register expectedNumArgs, Register actualNumArgs);
    static void PushJSFunctionEntryFrame(ExtendedAssembler *assembler, Register prevFp);
    static void PopJSFunctionEntryFrame(ExtendedAssembler *assembler, Register glue);
    static void PushOptimizedUnfoldArgVFrame(ExtendedAssembler *assembler, Register callSiteSp);
    static void PopOptimizedUnfoldArgVFrame(ExtendedAssembler *assembler);
    static void IncreaseStackForArguments(ExtendedAssembler *assembler, Register argC, Register fp,
                                          int64_t numExtraArgs = 0);
    static void PushOptimizedArgsConfigFrame(ExtendedAssembler *assembler);
    static void PopOptimizedArgsConfigFrame(ExtendedAssembler *assembler);
    static void PushAsmBridgeFrame(ExtendedAssembler *assembler);
    static void PopAsmBridgeFrame(ExtendedAssembler *assembler);
    static void JSCallInternal(ExtendedAssembler *assembler, Register jsfunc, bool isNew = false);
    static void CallBuiltinTrampoline(ExtendedAssembler *assembler);
    static void CallBuiltinConstructorStub(ExtendedAssembler *assembler, Register builtinStub, Register argv,
                                           Register glue, Register temp);
    friend class OptimizedFastCall;
};

class OptimizedFastCall : public CommonCall {
public:
    static void OptimizedFastCallEntry(ExtendedAssembler *assembler);

    static void OptimizedFastCallAndPushArgv(ExtendedAssembler *assembler);

    static void JSFastCallWithArgV(ExtendedAssembler *assembler);

    static void JSFastCallWithArgVAndPushArgv(ExtendedAssembler *assembler);
};

class AsmInterpreterCall : public CommonCall {
public:
    static void AsmInterpreterEntry(ExtendedAssembler *assembler);

    static void AsmInterpEntryDispatch(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterp(ExtendedAssembler *assembler);

    static void GeneratorReEnterAsmInterpDispatch(ExtendedAssembler *assembler);

    static void PushCallThisRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallRangeAndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs3AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgs2AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArg1AndDispatch(ExtendedAssembler *assembler);

    static void PushCallArg0AndDispatch(ExtendedAssembler *assembler);

    static void PushCallThisArg0AndDispatch(ExtendedAssembler *assembler);

    static void PushCallThisArg1AndDispatch(ExtendedAssembler *assembler);

    static void PushCallThisArgs2AndDispatch(ExtendedAssembler *assembler);

    static void PushCallThisArgs3AndDispatch(ExtendedAssembler *assembler);

    static void PushCallThisRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallRangeAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallNewAndDispatchNative(ExtendedAssembler *assembler);

    static void PushNewTargetAndDispatchNative(ExtendedAssembler *assembler);

    static void PushCallNewAndDispatch(ExtendedAssembler *assembler);

    static void PushSuperCallAndDispatch(ExtendedAssembler *assembler);

    static void PushCallArgsAndDispatchNative(ExtendedAssembler *assembler);

    static void ResumeRspAndDispatch(ExtendedAssembler *assembler);

    static void ResumeRspAndReturn([[maybe_unused]] ExtendedAssembler *assembler);

    static void ResumeRspAndReturnBaseline([[maybe_unused]] ExtendedAssembler *assembler);

    static void ResumeCaughtFrameAndDispatch(ExtendedAssembler *assembler);

    static void ResumeUncaughtFrameAndReturn(ExtendedAssembler *assembler);

    static void ResumeRspAndRollback(ExtendedAssembler *assembler);

    static void CallGetter(ExtendedAssembler *assembler);

    static void CallSetter(ExtendedAssembler *assembler);

    static void CallContainersArgs2(ExtendedAssembler *assembler);

    static void CallContainersArgs3(ExtendedAssembler *assembler);

    static void CallReturnWithArgv([[maybe_unused]]ExtendedAssembler *assembler);

    static void CallGetterToBaseline(ExtendedAssembler *assembler);

    static void CallSetterToBaseline(ExtendedAssembler *assembler);

    static void CallContainersArgs2ToBaseline(ExtendedAssembler *assembler);

    static void CallContainersArgs3ToBaseline(ExtendedAssembler *assembler);

    static void CallReturnWithArgvToBaseline(ExtendedAssembler *assembler);

    static void ASMFastWriteBarrier(ExtendedAssembler *assembler);

    static void ASMFastSharedWriteBarrier(ExtendedAssembler *assembler, Label& needCall);
private:
    static void PushCallThis(ExtendedAssembler *assembler, JSCallMode mode,
                             Label *stackOverflow, FrameTransitionType type);

    static Register GetThisRegsiter(ExtendedAssembler *assembler, JSCallMode mode, Register defaultRegister);
    static Register GetNewTargetRegsiter(ExtendedAssembler *assembler, JSCallMode mode, Register defaultRegister);

    static void PushVregs(ExtendedAssembler *assembler, Label *stackOverflow, FrameTransitionType type);

    static void DispatchCall(ExtendedAssembler *assembler, Register pc, Register newSp,
                             Register acc = INVALID_REG, bool hasException = false);

    static void CallNativeInternal(ExtendedAssembler *assembler, Register nativeCode);

    static bool PushBuiltinFrame(ExtendedAssembler *assembler, Register glue,
        FrameType type, Register op, Register next);

    static void ThrowStackOverflowExceptionAndReturn(ExtendedAssembler *assembler, Register glue, Register fp,
        Register op);
    static void ThrowStackOverflowExceptionAndReturnToAsmInterpBridgeFrame(ExtendedAssembler *assembler, Register glue,
        Register fp, Register op);

    static void PushFrameState(ExtendedAssembler *assembler, Register prevSp, Register fp, Register currentSlot,
        Register callTarget, Register thisObj, Register method, Register pc, Register op);

    static void JSCallCommonEntry(ExtendedAssembler *assembler, JSCallMode mode, FrameTransitionType type);

    static void JSCallCommonFastPath(ExtendedAssembler *assembler, JSCallMode mode, Label *pushCallThis,
        Label *stackOverflow);
    static void JSCallCommonSlowPath(ExtendedAssembler *assembler, JSCallMode mode,
                                     Label *fastPathEntry, Label *pushCallThis, Label *stackOverflow);

    static void GetNumVregsFromCallField(ExtendedAssembler *assembler, Register callField, Register numVregs);

    static void GetDeclaredNumArgsFromCallField(ExtendedAssembler *assembler, Register callField,
        Register declaredNumArgs);

    static void SaveFpAndJumpSize(ExtendedAssembler *assembler, Immediate jumpSize);

    static void PushAsmInterpEntryFrame(ExtendedAssembler *assembler);

    static void PopAsmInterpEntryFrame(ExtendedAssembler *assembler);

    static void PushGeneratorFrameState(ExtendedAssembler *assembler, Register &prevSpRegister, Register &fpRegister,
        Register &currentSlotRegister, Register &callTargetRegister, Register &thisRegister, Register &methodRegister,
        Register &contextRegister, Register &pcRegister, Register &operatorRegister);

    static void CallBCStub(ExtendedAssembler *assembler, Register &newSp, Register &glue,
        Register &callTarget, Register &method, Register &pc, Register &temp);

    static void CallNativeEntry(ExtendedAssembler *assembler, bool isJsProxy);

    static void CallNativeWithArgv(ExtendedAssembler *assembler, bool callNew, bool hasNewTarget = false);
    static void PreserveMostCall(ExtendedAssembler* assembler);
    friend class OptimizedCall;
    friend class BaselineCall;
};

class BaselineCall : public CommonCall {
public:
    /* other call baseline: need to check whether baseline code exists */
    static void CallArg0AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallArg1AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallArgs2AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallArgs3AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallThisArg0AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallThisArg1AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallThisArgs2AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallThisArgs3AndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallRangeAndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallNewAndCheckToBaseline(ExtendedAssembler *assembler);
    static void SuperCallAndCheckToBaseline(ExtendedAssembler *assembler);
    static void CallThisRangeAndCheckToBaseline(ExtendedAssembler *assembler);
    /* baseline call other: need to save fp and lr */
    static void CallArg0AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallArg1AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallArgs2AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallArgs3AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArg0AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArg1AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArgs2AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArgs3AndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallRangeAndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallNewAndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void SuperCallAndDispatchFromBaseline(ExtendedAssembler *assembler);
    static void CallThisRangeAndDispatchFromBaseline(ExtendedAssembler *assembler);
    /* baseline call baseline: need to check whether baseline code exists and save fp and lr */
    static void CallArg0AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallArg1AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallArgs2AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallArgs3AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArg0AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArg1AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArgs2AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallThisArgs3AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallRangeAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallNewAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void SuperCallAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    static void CallThisRangeAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler);
    /* get baselineBuiltinFp when baselineBuiltin call the others */
    static void GetBaselineBuiltinFp(ExtendedAssembler *assembler);
};

}  // namespace panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_MODULE_X64_H
