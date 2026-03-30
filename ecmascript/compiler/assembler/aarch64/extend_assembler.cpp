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

#include "ecmascript/compiler/assembler/aarch64/extend_assembler.h"


namespace panda::ecmascript::aarch64 {
Register ExtendedAssembler::ghcJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT] = {
    x19, fp, x20, x21, x22, x23, x24, x25, x26
};
Register ExtendedAssembler::cppJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT] = {
    x0, fp, x1, x2, x3, x4, x5, x6, x7
};

void ExtendedAssembler::CalleeSave()
{
    Stp(x27, x28, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(x25, x26, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(x23, x24, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(x21, x22, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(x19, x20, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));

    Stp(d14, d15, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(d12, d13, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(d10, d11, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Stp(d8, d9, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
}

void ExtendedAssembler::CalleeRestore()
{
    Ldp(d8, d9, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(d10, d11, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(d12, d13, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(d14, d15, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x19, x20, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x21, x22, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x23, x24, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x25, x26, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x27, x28, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
}

void ExtendedAssembler::CalleeRestoreNoReservedRegister()
{
    Ldp(d8, d9, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(d10, d11, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(d12, d13, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(d14, d15, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x19, x20, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x21, x22, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x23, x24, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x25, x26, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
    Ldp(x27, xzr, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
}

void ExtendedAssembler::CallAssemblerStub(int id, bool isTail)
{
    Label *target = module_->GetFunctionLabel(id);
    isTail ? B(target) : Bl(target);
}

void ExtendedAssembler::BindAssemblerStub(int id)
{
    Label *target = module_->GetFunctionLabel(id);
    Bind(target);
    auto callSigns = module_->GetCSigns();
    auto cs = callSigns[id];
    isGhcCallingConv_ = cs->GetCallConv() == kungfu::CallSignature::CallConv::GHCCallConv;
}

void ExtendedAssembler::PushFpAndLr()
{
    Stp(fp, lr, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
}

void ExtendedAssembler::SaveFpAndLr()
{
    Stp(fp, lr, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Mov(fp, sp);
}

void ExtendedAssembler::RestoreFpAndLr()
{
    Ldp(fp, lr, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
}

void ExtendedAssembler::PushLrAndFp()
{
    Stp(lr, fp, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
}

void ExtendedAssembler::SaveLrAndFp()
{
    Stp(lr, fp, MemoryOperand(sp, -PAIR_SLOT_SIZE, PREINDEX));
    Mov(fp, sp);
}

void ExtendedAssembler::RestoreLrAndFp()
{
    Ldp(lr, fp, MemoryOperand(sp, PAIR_SLOT_SIZE, POSTINDEX));
}

void ExtendedAssembler::PushArgc(int32_t argc, Register op, Register fpReg)
{
    Mov(op, Immediate(JSTaggedValue(argc).GetRawData()));
    Str(op, MemoryOperand(fpReg, -8, PREINDEX));  // -8: 8 bytes
}

void ExtendedAssembler::PushArgc(Register argc, Register op, Register fpReg)
{
    Orr(op, argc, LogicalImmediate::Create(JSTaggedValue::TAG_INT, X_REG_SIZE));
    Str(op, MemoryOperand(fpReg, -8, PREINDEX));  // -8: 8 bytes
}

void ExtendedAssembler::Align16(Register fpReg)
{
    Label aligned;
    Tst(fpReg, LogicalImmediate::Create(0xf, X_REG_SIZE));  // 0xf: 0x1111
    B(Condition::EQ, &aligned);
    // 8: frame slot size
    Sub(fpReg, fpReg, Immediate(8));
    Bind(&aligned);
}

void ExtendedAssembler::UpdateGlueAndReadBarrier(Register glueReg)
{
#ifdef ENABLE_CMC_IR_FIX_REGISTER
    Ldr(x28, MemoryOperand(glueReg, JSThread::GlueData::GetBarrierAndGlueOffset(false)));
#endif
}
}  // namespace panda::ecmascript::aarch64