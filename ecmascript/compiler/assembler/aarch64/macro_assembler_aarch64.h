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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_MACRO_ASSEMBLER_AARCH64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_MACRO_ASSEMBLER_AARCH64_H

#include "ecmascript/compiler/assembler/macro_assembler.h"
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"

namespace panda::ecmascript::kungfu {
class MacroAssemblerAArch64 : public MacroAssembler {
public:
    MacroAssemblerAArch64() : MacroAssembler(), assembler(&chunk) {}
    ~MacroAssemblerAArch64() = default;
    uint8_t *GetBegin() const override
    {
        return assembler.GetBegin();
    }

    size_t GetBufferCurrentSize() const override
    {
        return assembler.GetCurrentPosition();
    }

    void SetBaselineFlag()
    {
        assembler.SetDoCodeSign();
    }

    void Move(const StackSlotOperand &dstStackSlot, Immediate value) override;
    void Move(const StackSlotOperand &dstStackSlot, const StackSlotOperand &srcStackSlot) override;
    void Cmp(const StackSlotOperand &stackSlot, Immediate value) override;
    void Bind(JumpLabel &label) override;
    void Jz(JumpLabel &label) override;
    void Jnz(JumpLabel &label) override;
    void Jump(JumpLabel &label) override;
    void SaveReturnRegister(const StackSlotOperand &dstStackSlot) override;
    void CallBuiltin(Address funcAddress,
                     const std::vector<MacroParameter> &parameters) override;

private:
    aarch64::AssemblerAarch64 assembler;
    const uint32_t PARAM_REGISTER_COUNT = 8;
    const std::vector<aarch64::Register> registerParamVec {
        aarch64::Register(aarch64::X0), aarch64::Register(aarch64::X1), aarch64::Register(aarch64::X2),
        aarch64::Register(aarch64::X3), aarch64::Register(aarch64::X4), aarch64::Register(aarch64::X5),
        aarch64::Register(aarch64::X6), aarch64::Register(aarch64::X7) };
    const aarch64::Register GLUE_REGISTER = aarch64::Register(aarch64::X19);   // same with ghc callconv
    const aarch64::Register LOCAL_SCOPE_REGISTER = aarch64::Register(aarch64::X11);
    const aarch64::Register RETURN_REGISTER = aarch64::Register(aarch64::X0);
    void MovParameterIntoParamReg(MacroParameter param, aarch64::Register paramReg);
    void PickLoadStoreInsn(aarch64::Register reg, aarch64::MemoryOperand memOpnd, bool isLoad = true);
    bool IsMoveWidableImmediate(uint64_t val, uint32_t bitLen);
    bool IsBitmaskImmediate(uint64_t val, uint32_t bitLen);
    bool BetterUseMOVZ(uint64_t val);
    bool IsSingleInstructionMovable(uint64_t imm, uint32_t size);
    void CopyImm(aarch64::Register destReg, int64_t imm, uint32_t size);
    void CopyImmSize64(aarch64::Register destReg, uint64_t srcVal);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_MACRO_ASSEMBLER_AARCH64_H
