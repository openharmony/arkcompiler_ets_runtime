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

#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_X64_MACRO_ASSEMBLER_X64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_X64_MACRO_ASSEMBLER_X64_H

#include "ecmascript/compiler/assembler/macro_assembler.h"
#include "ecmascript/compiler/assembler/x64/assembler_x64.h"

namespace panda::ecmascript::kungfu {
class MacroAssemblerX64 : public MacroAssembler {
public:
    explicit MacroAssemblerX64() : MacroAssembler(), assembler(&chunk) {}
    ~MacroAssemblerX64() = default;
    uint8_t *GetBegin() const override
    {
        return assembler.GetBegin();
    }

    size_t GetBufferCurrentSize() const override
    {
        return assembler.GetCurrentPosition();
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
    x64::AssemblerX64 assembler;
    static constexpr uint32_t PARAM_REGISTER_COUNT = 6;
    const std::vector<x64::Register> registerParamVec {
        x64::rdi, x64::rsi, x64::rdx, x64::rcx, x64::r8, x64::r9 };
    static constexpr x64::Register GLUE_REGISTER = x64::r13;   // same with ghc callconv
    static constexpr x64::Register LOCAL_SCOPE_REGISTER = x64::r10;
    static constexpr x64::Register RETURN_REGISTER = x64::rax;
    void MovParameterIntoParamReg(MacroParameter param, x64::Register paramReg);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_X64_MACRO_ASSEMBLER_X64_H
