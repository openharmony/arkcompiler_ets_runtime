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

#include "ecmascript/compiler/trampoline/aarch64/common_call.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/compiler/assembler/aarch64/extend_assembler.h"

namespace panda::ecmascript::aarch64 {
using Label = panda::ecmascript::Label;
#define __ assembler->

/* other->baseline */
void BaselineCall::CallArg0AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArg0AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0,
                                          FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallArg1AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArg1AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1,
                                          FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallArgs2AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArgs2AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2,
                                          FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallArgs3AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArgs3AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3,
                                          FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArg0AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArg0AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG0, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArg1AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArg1AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG1, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArgs2AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArgs2AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG2, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArgs3AndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArgs3AndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG3, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallRangeAndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallRangeAndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_WITH_ARGV, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallNewAndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallNewAndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::SuperCallAndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SuperCallAndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::SUPER_CALL_WITH_ARGV, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisRangeAndCheckToBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisRangeAndCheckToBaseline));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_WITH_ARGV, FrameTransitionType::OTHER_TO_BASELINE_CHECK);
}

/* baseline->other */
void BaselineCall::CallArg0AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArg0AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0, FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallArg1AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArg1AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1, FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallArgs2AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArgs2AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2, FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallArgs3AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArgs3AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3, FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallThisArg0AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArg0AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG0,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallThisArg1AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArg1AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG1,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallThisArgs2AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArgs2AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG2,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallThisArgs3AndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArgs3AndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallRangeAndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallRangeAndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallNewAndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallNewAndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::SuperCallAndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SuperCallAndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::SUPER_CALL_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

void BaselineCall::CallThisRangeAndDispatchFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisRangeAndDispatchFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_OTHER);
}

/* baseline->baseline */
void BaselineCall::CallArg0AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArg0AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallArg1AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArg1AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallArgs2AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArgs2AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallArgs3AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallArgs3AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallRangeAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallRangeAndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisRangeAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisRangeAndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArg0AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArg0AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG0,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArg1AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArg1AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG1,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArgs2AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArgs2AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG2,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallThisArgs3AndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallThisArgs3AndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_THIS_ARG3,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::CallNewAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(CallNewAndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::SuperCallAndCheckToBaselineFromBaseline(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(SuperCallAndCheckToBaselineFromBaseline));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::SUPER_CALL_WITH_ARGV,
                                          FrameTransitionType::BASELINE_TO_BASELINE_CHECK);
}

void BaselineCall::GetBaselineBuiltinFp(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(GetBaselineBuiltinFp));
    Register ret = X0;
    Register fp = X29;
    __ Mov(ret, fp);
    __ Ret();
}
#undef __
}  // namespace panda::ecmascript::aarch64