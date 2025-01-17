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

#include "aarch64_cg.h"

namespace maplebe {
using namespace maple;

void AArch64RegInfo::Init()
{
    for (regno_t regNO = kRinvalid; regNO < kMaxRegNum; ++regNO) {
        /* when yieldpoint is enabled, x19 is reserved. */
        if (IsYieldPointReg(regNO) || IsReservedReg(regNO)) {
            continue;
        }
        if (regNO == R29 && GetCurrFunction()->UseFP()) {
            continue;
        }
        if (!AArch64Abi::IsAvailableReg(static_cast<AArch64reg>(regNO))) {
            continue;
        }
        if (AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO))) {
            AddToIntRegs(regNO);
        } else {
            AddToFpRegs(regNO);
        }
        AddToAllRegs(regNO);
    }
    return;
}

void AArch64RegInfo::Fini()
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    a64CGFunc->AddtoCalleeSaved(RFP);
    a64CGFunc->AddtoCalleeSaved(RLR);
    a64CGFunc->NoteFPLRAddedToCalleeSavedList();
}

bool AArch64RegInfo::IsSpillRegInRA(regno_t regNO, bool has3RegOpnd)
{
    return AArch64Abi::IsSpillRegInRA(static_cast<AArch64reg>(regNO), has3RegOpnd);
}
bool AArch64RegInfo::IsCalleeSavedReg(regno_t regno) const
{
    return AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regno));
}
bool AArch64RegInfo::IsYieldPointReg(regno_t regno) const
{
    /* when yieldpoint is enabled, x19 is reserved. */
    if (CGOptions::GetInstance().GenYieldPoint()) {
        return (static_cast<AArch64reg>(regno) == RYP);
    }
    return false;
}
bool AArch64RegInfo::IsUnconcernedReg(regno_t regNO) const
{
    /* RFP = 32, RLR = 31, RSP = 33, RZR = 34, ccReg */
    if ((regNO >= RLR && regNO <= RZR) || regNO == RFP || regNO == kRFLAG) {
        return true;
    }

    /* when yieldpoint is enabled, the RYP(x19) can not be used. */
    if (IsYieldPointReg(regNO)) {
        return true;
    }
    return false;
}

RegOperand *AArch64RegInfo::GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, maplebe::RegType kind, uint32 flag)
{
    AArch64CGFunc *aarch64CgFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    return &aarch64CgFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO), size, kind, flag);
}

Insn *AArch64RegInfo::BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd)
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    return &a64CGFunc->GetInsnBuilder()->BuildInsn(a64CGFunc->PickStInsn(regSize, stype), phyOpnd, memOpnd);
}

Insn *AArch64RegInfo::BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd)
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    return &a64CGFunc->GetInsnBuilder()->BuildInsn(a64CGFunc->PickLdInsn(regSize, stype), phyOpnd, memOpnd);
}

bool AArch64RegInfo::IsMovFromRegtoReg(MOperator mOp, Insn &insn)
{
    if (insn.IsMove() && (mOp == MOP_xmovrr || mOp == MOP_wmovrr)) {
        return true;
    }
    return false;
}

MemOperand *AArch64RegInfo::GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize)
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    return a64CGFunc->GetOrCreatSpillMem(vrNum, bitSize);
}
MemOperand *AArch64RegInfo::AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, const RegNoPair &regNoPair,
                                                               bool isDest, Insn &insn, bool &isOutOfRange)
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    return a64CGFunc->AdjustMemOperandIfOffsetOutOfRange(memOpnd, static_cast<AArch64reg>(regNoPair.first), isDest,
                                                         insn, static_cast<AArch64reg>(regNoPair.second), isOutOfRange);
}
void AArch64RegInfo::FreeSpillRegMem(regno_t vrNum)
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(GetCurrFunction());
    return a64CGFunc->FreeSpillRegMem(vrNum);
}
} /* namespace maplebe */
