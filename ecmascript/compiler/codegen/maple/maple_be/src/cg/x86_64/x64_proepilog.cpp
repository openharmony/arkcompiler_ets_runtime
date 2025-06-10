/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "x64_proepilog.h"
#include "x64_cg.h"

namespace maplebe {
using namespace maple;
bool X64GenProEpilog::NeedProEpilog()
{
    return true;
}
void X64GenProEpilog::GenerateCalleeSavedRegs(bool isPush)
{
    X64CGFunc &x64cgFunc = static_cast<X64CGFunc &>(cgFunc);
    const auto &calleeSavedRegs = x64cgFunc.GetCalleeSavedRegs();
    if (calleeSavedRegs.empty()) {
        return;
    }
    /* CalleeSave(0) = -(FrameSize + CalleeReg - ArgsStk) */
    X64MemLayout *memLayout = static_cast<X64MemLayout *>(cgFunc.GetMemlayout());
    int64 offset = -static_cast<int64>((memLayout->StackFrameSize() +
        static_cast<X64CGFunc &>(cgFunc).SizeOfCalleeSaved() - memLayout->SizeOfArgsToStackPass()));
    RegOperand &baseReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
    std::vector<std::pair<uint16, int32>> calleeRegAndOffsetVec;
    for (const auto &reg : calleeSavedRegs) {
        RegType regType = IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        uint32 regByteSize = IsGPRegister(reg) ? kX64IntregBytelen : kX64FpregBytelen;
        uint32 regSize = regByteSize * kBitsPerByte;
        DEBUG_ASSERT((regSize == k32BitSize || regSize == k64BitSize), "only supported 32/64-bits");
        RegOperand &calleeReg = cgFunc.GetOpndBuilder()->CreatePReg(reg, regSize, regType);
        calleeRegAndOffsetVec.push_back(
            std::pair<uint16, int32>(static_cast<uint16>(reg) - 1, static_cast<int32>(offset)));
        MemOperand &memOpnd = cgFunc.GetOpndBuilder()->CreateMem(baseReg, offset, regSize);
        if (isPush) {
            GeneratePushCalleeSavedRegs(calleeReg, memOpnd, regSize);
        } else {
            GeneratePopCalleeSavedRegs(calleeReg, memOpnd, regSize);
        }
        offset += static_cast<int64>(regByteSize);
    }
    const auto &emitMemoryManager = CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemoryManager.codeSpace != nullptr) {
        emitMemoryManager.funcCalleeOffsetSaver(emitMemoryManager.codeSpace, cgFunc.GetName(), calleeRegAndOffsetVec);
        const int32 fp2SPDelta = 16;  // FP + return address;
        emitMemoryManager.funcFpSPDeltaSaver(emitMemoryManager.codeSpace, cgFunc.GetName(), fp2SPDelta);
    }
}

void X64GenProEpilog::GeneratePushCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize)
{
    MOperator mMovrmOp = (regSize == k32BitSize) ? x64::MOP_movl_r_m : x64::MOP_movq_r_m;
    Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrmOp, X64CG::kMd[mMovrmOp]);
    copyInsn.AddOpndChain(regOpnd).AddOpndChain(memOpnd);
    cgFunc.GetCurBB()->AppendInsn(copyInsn);
}

void X64GenProEpilog::GeneratePopCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize)
{
    MOperator mMovrmOp = (regSize == k32BitSize) ? x64::MOP_movl_m_r : x64::MOP_movq_m_r;
    Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrmOp, X64CG::kMd[mMovrmOp]);
    copyInsn.AddOpndChain(memOpnd).AddOpndChain(regOpnd);
    cgFunc.GetCurBB()->AppendInsn(copyInsn);
}

void X64GenProEpilog::GenerateProlog(BB &bb)
{
    auto &x64CGFunc = static_cast<X64CGFunc &>(cgFunc);
    BB *formerCurBB = cgFunc.GetCurBB();
    x64CGFunc.GetDummyBB()->ClearInsns();
    x64CGFunc.GetDummyBB()->SetIsProEpilog(true);
    cgFunc.SetCurBB(*x64CGFunc.GetDummyBB());

    /* push %rbp */
    MOperator mPushrOp = x64::MOP_pushq_r;
    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mPushrOp, X64CG::kMd[mPushrOp]);
    RegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
    pushInsn.AddOpndChain(opndFpReg);
    cgFunc.GetCurBB()->AppendInsn(pushInsn);

    /* mov %rsp, %rbp */
    MOperator mMovrrOp = x64::MOP_movq_r_r;
    Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrrOp, X64CG::kMd[mMovrrOp]);
    RegOperand &opndSpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RSP, k64BitSize, kRegTyInt);
    copyInsn.AddOpndChain(opndSpReg).AddOpndChain(opndFpReg);
    cgFunc.GetCurBB()->AppendInsn(copyInsn);

    /* sub $framesize, %rsp */
    if (cgFunc.GetFunction().HasCall() || cgFunc.HasVLAOrAlloca()) {
        MOperator mSubirOp = x64::MOP_subq_i_r;
        Insn &subInsn = cgFunc.GetInsnBuilder()->BuildInsn(mSubirOp, X64CG::kMd[mSubirOp]);
        auto *memLayout = static_cast<X64MemLayout *>(cgFunc.GetMemlayout());
        int64 trueFrameSize =
            static_cast<int64>(memLayout->StackFrameSize() + static_cast<X64CGFunc &>(cgFunc).SizeOfCalleeSaved());
        ImmOperand &opndImm = cgFunc.GetOpndBuilder()->CreateImm(k32BitSize, trueFrameSize);
        subInsn.AddOpndChain(opndImm).AddOpndChain(opndSpReg);
        cgFunc.GetCurBB()->AppendInsn(subInsn);
    }

    GenerateCalleeSavedRegs(true);

    bb.InsertAtBeginning(*x64CGFunc.GetDummyBB());
    x64CGFunc.GetDummyBB()->SetIsProEpilog(false);
    cgFunc.SetCurBB(*formerCurBB);
}

void X64GenProEpilog::GenerateEpilog(BB &bb)
{
    auto &x64CGFunc = static_cast<X64CGFunc &>(cgFunc);
    BB *formerCurBB = cgFunc.GetCurBB();
    x64CGFunc.GetDummyBB()->ClearInsns();
    x64CGFunc.GetDummyBB()->SetIsProEpilog(true);
    cgFunc.SetCurBB(*x64CGFunc.GetDummyBB());

    GenerateCalleeSavedRegs(false);

    if (cgFunc.GetFunction().HasCall() || cgFunc.HasVLAOrAlloca()) {
        /*
         * leave  equal with
         * mov rsp rbp
         * pop rbp
         */
        MOperator mLeaveOp = x64::MOP_leaveq;
        Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mLeaveOp, X64CG::kMd[mLeaveOp]);
        cgFunc.GetCurBB()->AppendInsn(popInsn);
    } else {
        /* pop %rbp */
        MOperator mPopOp = x64::MOP_popq_r;
        Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mPopOp, X64CG::kMd[mPopOp]);
        RegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
        pushInsn.AddOpndChain(opndFpReg);
        cgFunc.GetCurBB()->AppendInsn(pushInsn);
    }
    /* ret */
    MOperator mRetOp = x64::MOP_retq;
    Insn &retInsn = cgFunc.GetInsnBuilder()->BuildInsn(mRetOp, X64CG::kMd[mRetOp]);
    cgFunc.GetCurBB()->AppendInsn(retInsn);

    bb.AppendBBInsns(*x64CGFunc.GetDummyBB());
    x64CGFunc.GetDummyBB()->SetIsProEpilog(false);
    cgFunc.SetCurBB(*formerCurBB);
}

void X64GenProEpilog::Run()
{
    if (cgFunc.GetFunction().IsDeoptFunc()) {   // deopt function does not need prologue/epilogue
        return;
    }
    GenerateProlog(*(cgFunc.GetFirstBB()));
    GenerateEpilog(*(cgFunc.GetLastBB()));
}
} /* namespace maplebe */
