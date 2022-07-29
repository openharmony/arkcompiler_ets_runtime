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

#include "ecmascript/compiler/trampoline/x64/common_call.h"

#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/runtime_call_id.h"

#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::x64 {
#define __ assembler->

void CommonCall::CopyArgumentWithArgV(ExtendedAssembler *assembler, Register argc, Register argV)
{
    Label loopBeginning;
    Register arg = __ AvailableRegister1();
    __ Bind(&loopBeginning);
    __ Movq(Operand(argV, argc, Scale::Times8, -8), arg); // -8: stack index
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
#undef __
}  // namespace panda::ecmascript::x64
