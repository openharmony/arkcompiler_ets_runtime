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
#include "cfi_generator.h"
#if TARGAARCH64
#include "aarch64_cfi_generator.h"
#include "aarch64_cgfunc.h"
#endif

namespace maplebe {
Insn *GenCfi::FindStackDefInsn(BB &bb) const
{
    FOR_BB_INSNS(insn, &bb)
    {
        if (insn->IsStackDef()) {
            return insn;
        }
    }
    return nullptr;
}

Insn *GenCfi::FinsStackRevertInsn(BB &bb) const
{
    FOR_BB_INSNS_REV(insn, &bb)
    {
        if (insn->IsStackRevert()) {
            return insn;
        }
    }
    return nullptr;
}

Insn *GenCfi::InsertCFIDefCfaOffset(BB &bb, Insn &insn, int32 &cfiOffset)
{
    cfiOffset = AddtoOffsetFromCFA(cfiOffset);
    Insn &cfiInsn = cgFunc.GetInsnBuilder()
                        ->BuildCfiInsn(cfi::OP_CFI_def_cfa_offset)
                        .AddOpndChain(cgFunc.CreateCfiImmOperand(cfiOffset, k64BitSize));
    (void)bb.InsertInsnAfter(insn, cfiInsn);
    return &cfiInsn;
}

void GenCfi::GenerateStartDirective(BB &bb)
{
    Insn &startProcInsn = cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_startproc);
    if (bb.GetFirstInsn() != nullptr) {
        (void)bb.InsertInsnBefore(*bb.GetFirstInsn(), startProcInsn);
    } else {
        bb.AppendInsn(startProcInsn);
    }
}

void GenCfi::GenerateEndDirective(BB &bb)
{
    bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_endproc));
}

void GenCfi::GenerateRegisterStateDirective(BB &bb)
{
    if (&bb == cgFunc.GetLastBB() || bb.GetNext() == nullptr) {
        return;
    }

    BB *nextBB = bb.GetNext();
    do {
        if (nextBB == cgFunc.GetLastBB() || !nextBB->IsEmpty()) {
            break;
        }
        nextBB = nextBB->GetNext();
    } while (nextBB != nullptr);

    if (nextBB != nullptr && !nextBB->IsEmpty()) {
        bb.InsertInsnBegin(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_remember_state));
        nextBB->InsertInsnBegin(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_restore_state));
    }
}

void GenCfi::Run()
{
    auto *startBB = cgFunc.GetFirstBB();
    GenerateStartDirective(*startBB);

    if (cgFunc.GetHasProEpilogue()) {
        FOR_ALL_BB(bb, &cgFunc)
        {
            auto *stackDefInsn = FindStackDefInsn(*bb);
            if (stackDefInsn != nullptr) {
                GenerateRegisterSaveDirective(*bb, *stackDefInsn);
            }
            auto *stackRevertInsn = FinsStackRevertInsn(*bb);
            if (stackRevertInsn != nullptr) {
                GenerateRegisterStateDirective(*bb);
                GenerateRegisterRestoreDirective(*bb, *stackRevertInsn);
            }
        }
    }

    GenerateEndDirective(*(cgFunc.GetLastBB()));
    if (cgFunc.GetLastBB()->IsUnreachable()) {
        cgFunc.SetExitBBLost(true);
    }
}

bool CgGenCfi::PhaseRun(maplebe::CGFunc &f)
{
    GenCfi *genCfi = GetPhaseAllocator()->New<AArch64GenCfi>(f);
    genCfi->Run();
    return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenCfi, gencfi)
} /* namespace maplebe */
