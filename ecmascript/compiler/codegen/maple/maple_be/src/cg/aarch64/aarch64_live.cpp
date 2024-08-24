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

#include "aarch64_live.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64LiveAnalysis::GenerateReturnBBDefUse(BB &bb) const
{
    PrimType returnType = cgFunc->GetFunction().GetReturnType()->GetPrimType();
    auto *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    if (IsPrimitiveFloat(returnType)) {
        Operand &phyOpnd =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(V0), k64BitSize, kRegTyFloat);
        CollectLiveInfo(bb, phyOpnd, false, true);
    } else if (IsPrimitiveInteger(returnType)) {
        Operand &phyOpnd =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R0), k64BitSize, kRegTyInt);
        CollectLiveInfo(bb, phyOpnd, false, true);
    }
}

void AArch64LiveAnalysis::ProcessCallInsnParam(BB &bb, const Insn &insn) const
{
    /* R0 ~ R7（R0 + 0  ~ R0 + 7） and V0 ~ V7 (V0 + 0 ~ V0 + 7) is parameter register */
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    auto *targetOpnd = insn.GetCallTargetOperand();
    CHECK_FATAL(targetOpnd != nullptr, "target is null in Insn::IsCallToFunctionThatNeverReturns");
    if (CGOptions::DoIPARA() && targetOpnd->IsFuncNameOpnd()) {
        FuncNameOperand *target = static_cast<FuncNameOperand *>(targetOpnd);
        const MIRSymbol *funcSt = target->GetFunctionSymbol();
        DEBUG_ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
        MIRFunction *func = funcSt->GetFunction();
        if (func != nullptr && func->IsReferedRegsValid()) {
            for (auto preg : func->GetReferedRegs()) {
                if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(preg))) {
                    continue;
                }
                RegOperand *opnd = &aarchCGFunc->GetOrCreatePhysicalRegisterOperand(
                    static_cast<AArch64reg>(preg), k64BitSize,
                    AArch64isa::IsFPSIMDRegister(static_cast<AArch64reg>(preg)) ? kRegTyFloat : kRegTyInt);
                CollectLiveInfo(bb, *opnd, true, false);
            }
            return;
        }
    }
    for (uint32 i = 0; i < 8; ++i) { // do with 8 reg (R0-R7, V0-V7)
        Operand &phyOpndR =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R0 + i), k64BitSize, kRegTyInt);
        CollectLiveInfo(bb, phyOpndR, true, false);
        Operand &phyOpndV =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(V0 + i), k64BitSize, kRegTyFloat);
        CollectLiveInfo(bb, phyOpndV, true, false);
    }
}

} /* namespace maplebe */
