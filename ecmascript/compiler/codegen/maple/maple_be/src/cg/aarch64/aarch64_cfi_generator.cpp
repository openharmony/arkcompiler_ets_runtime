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
#include "aarch64_cfi_generator.h"
#include "aarch64_cgfunc.h"
namespace maplebe {
void AArch64GenCfi::GenerateRegisterSaveDirective(BB &bb, Insn &stackDefInsn)
{
    auto stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    auto argsToStkPassSize = static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass());
    int32 cfiOffset = stackFrameSize;
    auto *curInsn = InsertCFIDefCfaOffset(bb, stackDefInsn, cfiOffset);
    cfiOffset = static_cast<int32>(GetOffsetFromCFA() - argsToStkPassSize);
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);

    if (useFP) {
        curInsn = bb.InsertInsnAfter(*curInsn, aarchCGFunc.CreateCfiOffsetInsn(stackBaseReg, -cfiOffset, k64BitSize));
    }
    int32 RLROffset = static_cast<AArch64CGFunc &>(cgFunc).GetStoreFP() ? kOffset8MemPos : 0;
    curInsn = bb.InsertInsnAfter(
        *curInsn, aarchCGFunc.CreateCfiOffsetInsn(RLR, static_cast<int64>(-cfiOffset + RLROffset), k64BitSize));

    /* change CFA register and offset */
    if (useFP) {
        bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
        if ((argsToStkPassSize > 0) || isLmbc) {
            curInsn = bb.InsertInsnAfter(
                *curInsn, aarchCGFunc.CreateCfiDefCfaInsn(stackBaseReg,
                    static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize() -
                    static_cast<uint32>(argsToStkPassSize) + cgFunc.GetFunction().GetFrameReseverdSlot(),
                    k64BitSize));
        } else {
            curInsn =
                bb.InsertInsnAfter(*curInsn, cgFunc.GetInsnBuilder()
                                                 ->BuildCfiInsn(cfi::OP_CFI_def_cfa_register)
                                                 .AddOpndChain(cgFunc.CreateCfiRegOperand(stackBaseReg, k64BitSize)));
        }
    }

    if (CGOptions::IsNoCalleeCFI()) {
        return;
    }

    /* callee save register cfi offset */
    auto &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ? aarchCGFunc.GetCalleeSavedRegs()
                                                                     : aarchCGFunc.GetProEpilogSavedRegs();
    if (regsToSave.empty()) {
        return;
    }

    auto it = regsToSave.begin();
    // skip the RFP
    if (*it == RFP) {
        ++it;
    }
    CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
    ++it;
    int32 offset = cgFunc.GetMemlayout()->GetCalleeSaveBaseLoc();
    for (; it != regsToSave.end(); ++it) {
        AArch64reg reg = *it;
        stackFrameSize -= static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass());
        cfiOffset = stackFrameSize - offset;
        curInsn = bb.InsertInsnAfter(*curInsn, aarchCGFunc.CreateCfiOffsetInsn(reg, -cfiOffset, k64BitSize));
        /* On AArch64, kIntregBytelen == 8 */
        offset += static_cast<int32>(kAarch64IntregBytelen);
    }
}

void AArch64GenCfi::GenerateRegisterRestoreDirective(BB &bb, Insn &stackRevertInsn)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    auto &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ? aarchCGFunc.GetCalleeSavedRegs()
                                                                     : aarchCGFunc.GetProEpilogSavedRegs();
    auto *curInsn = &stackRevertInsn;

    if (!regsToSave.empty()) {
        auto it = regsToSave.begin();
        // skip the RFP
        if (*it == RFP) {
            ++it;
        }
        CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
        ++it;

        if (!CGOptions::IsNoCalleeCFI()) {
            for (; it != regsToSave.end(); ++it) {
                AArch64reg reg = *it;
                curInsn = bb.InsertInsnAfter(*curInsn, aarchCGFunc.CreateCfiRestoreInsn(reg, k64BitSize));
            }
        }

        if (useFP) {
            curInsn = bb.InsertInsnAfter(*curInsn, aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
        }
        curInsn = bb.InsertInsnAfter(*curInsn, aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
    }
    // in aarch64 R31 is sp
    bb.InsertInsnAfter(*curInsn, aarchCGFunc.CreateCfiDefCfaInsn(R31, 0, k64BitSize));
}
}  // namespace maplebe
