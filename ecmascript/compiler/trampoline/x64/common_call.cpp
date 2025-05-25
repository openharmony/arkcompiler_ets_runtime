/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

void CommonCall::CopyArgumentWithArgV(ExtendedAssembler *assembler, Register argc, Register argV)
{
    Label loopBeginning;
    Register arg = __ AvailableRegister1();
    __ Bind(&loopBeginning);
    __ Movq(Operand(argV, argc, Scale::Times8, -FRAME_SLOT_SIZE), arg); // -8: stack index
    __ Pushq(arg);
    __ Subq(1, argc);
    __ Ja(&loopBeginning);
}

void CommonCall::PushAsmInterpBridgeFrame(ExtendedAssembler *assembler)
{
    // construct asm interpreter bridge frame
    __ Pushq(static_cast<int64_t>(FrameType::ASM_INTERPRETER_BRIDGE_FRAME));
    __ Pushq(rbp);
    __ Pushq(0);    // pc
    __ Leaq(Operand(rsp, 24), rbp);  // 24: skip pc, prevSp and frame type
    __ PushAlignBytes();
    if (!assembler->FromInterpreterHandler()) {
        __ PushCppCalleeSaveRegisters();
    }
}

void CommonCall::GetArgvAtStack(ExtendedAssembler *assembler)
{
    Register r13 = __ CppJSCallAvailableRegister1();
    Register r14 = __ CppJSCallAvailableRegister2();
    __ Movq(Operand(rbp, FRAME_SLOT_SIZE), r13);
    __ Movq(Operand(rbp, 2 * FRAME_SLOT_SIZE), r14);  // 2: skip second argv
}

void CommonCall::PopAsmInterpBridgeFrame(ExtendedAssembler *assembler)
{
    if (!assembler->FromInterpreterHandler()) {
        __ PopCppCalleeSaveRegisters();
    }
    __ PopAlignBytes();
    __ Addq(8, rsp);   // 8: skip pc
    __ Popq(rbp);
    __ Addq(8, rsp);  // 8: skip frame type
}

void CommonCall::PushUndefinedWithArgc(ExtendedAssembler *assembler, Register argc)
{
    Label loopBeginning;
    __ Bind(&loopBeginning);
    __ Pushq(JSTaggedValue::Undefined().GetRawData());
    __ Subq(1, argc);
    __ Ja(&loopBeginning);
}

void CommonCall::PushArgsWithArgvAndCheckStack(ExtendedAssembler *assembler, Register glue, Register argc,
    Register argv, Register op1, Register op2, Label *stackOverflow)
{
    ASSERT(stackOverflow != nullptr);
    StackOverflowCheck(assembler, glue, argc, op1, op2, stackOverflow);
    Register opArgc = argc;
    Register op = op1;
    if (op1 != op2) {
        // use op2 as opArgc and will not change argc register
        opArgc = op2;
        __ Movq(argc, opArgc);
    }
    Label loopBeginning;
    __ Bind(&loopBeginning);
    __ Movq(Operand(argv, opArgc, Times8, -8), op);  // 8: 8 bytes   argv crash rdx=0x8
    __ Pushq(op);
    __ Subq(1, opArgc);
    __ Ja(&loopBeginning);
}

void CommonCall::StackOverflowCheck(ExtendedAssembler *assembler, Register glue, Register numArgs, Register op1,
    Register op2, Label *stackOverflow)
{
    Register temp1 = op1;
    Register temp2 = op2;
    if (op1 == op2) {
        // reuse glue as an op register for temporary
        __ Pushq(glue);
        temp2 = glue;
    }
    __ Movq(Operand(glue, JSThread::GlueData::GetStackLimitOffset(false)), temp1);
    __ Movq(rsp, temp2);
    __ Subq(temp1, temp2);
    __ Movl(numArgs, temp1);
    __ Shlq(3, temp1);  // 3: each arg occupies 8 bytes
    __ Cmpq(temp1, temp2);
    if (op1 == op2) {
        __ Popq(glue);
    }
    __ Jle(stackOverflow);
}
#undef __
}  // namespace panda::ecmascript::x64
