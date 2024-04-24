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

#include "ecmascript/compiler/trampoline/x64/common_call.h"

namespace panda::ecmascript::x64 {
#define __ assembler->
void BaselineCall::BaselineCallArg0(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallArg0));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG0, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallArg1(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallArg1));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG1, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallArgs2(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallArgs2));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG2, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallArgs3(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallArgs3));
    AsmInterpreterCall::JSCallCommonEntry(assembler, JSCallMode::CALL_ARG3, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallThisArg0(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallThisArg0));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG0, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallThisArg1(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallThisArg1));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG1, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallThisArgs2(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallThisArgs2));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG2, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallThisArgs3(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallThisArgs3));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_ARG3, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallRange(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallRange));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_WITH_ARGV, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallNew(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallNew));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineSuperCall(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineSuperCall));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::SUPER_CALL_WITH_ARGV, CompilerTierCheck::CHECK_BASELINE_CODE);
}

void BaselineCall::BaselineCallThisRange(ExtendedAssembler *assembler)
{
    __ BindAssemblerStub(RTSTUB_ID(BaselineCallThisRange));
    AsmInterpreterCall::JSCallCommonEntry(
        assembler, JSCallMode::CALL_THIS_WITH_ARGV, CompilerTierCheck::CHECK_BASELINE_CODE);
}
#undef __
}  // namespace panda::ecmascript::x64