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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H
#include "reg_info.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"
#include "aarch64_abi.h"

namespace maplebe {

class AArch64RegInfo : public RegisterInfo {
public:
    explicit AArch64RegInfo(MapleAllocator &mallocator) : RegisterInfo(mallocator) {}

    ~AArch64RegInfo() override = default;

    // float 128-bit register has caller store, so pref alloced caller-save registers.
    bool IsPrefCallerSaveRegs(RegType type, uint32 size) const override
    {
        return (type == kRegTyFloat && size == k128BitSize);
    }

    // Additionally, only the bottom 64 bits of each value stored in virturl reg 8-15 need to be
    // preserved 8; it is the responsibility of the caller to preserve larger values.
    // Refer to Procedure Call Standard for the Arm 64-bit Architecture (AArch64) 2022Q3.  $6.1.2
    bool IsCallerSavePartRegister(regno_t regNO, uint32 size) const override
    {
        return (regNO >= V8 && regNO <= V15) && size == k128BitSize;
    }

    bool IsGPRegister(regno_t regNO) const override
    {
        return AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO));
    }

    regno_t GetIntRetReg(uint32 idx) override
    {
        CHECK_FATAL(idx < AArch64Abi::kNumIntParmRegs, "index out of range in IntRetReg");
        return AArch64Abi::intReturnRegs[idx];
    }
    regno_t GetFpRetReg(uint32 idx) override
    {
        CHECK_FATAL(idx < AArch64Abi::kNumFloatParmRegs, "index out of range in IntRetReg");
        return AArch64Abi::floatReturnRegs[idx];
    }
    bool IsAvailableReg(regno_t regNO) const override
    {
        return AArch64Abi::IsAvailableReg(static_cast<AArch64reg>(regNO));
    }
    uint32 GetIntRegsParmsNum() override
    {
        return AArch64Abi::kNumIntParmRegs;
    }
    uint32 GetFloatRegsParmsNum() override
    {
        return AArch64Abi::kNumFloatParmRegs;
    }
    uint32 GetIntRetRegsNum() override
    {
        return AArch64Abi::kNumIntParmRegs;
    }
    uint32 GetFpRetRegsNum() override
    {
        return AArch64Abi::kNumFloatParmRegs;
    }
    uint32 GetNormalUseOperandNum() override
    {
        return AArch64Abi::kNormalUseOperandNum;
    }
    uint32 GetIntParamRegIdx(regno_t regNO) const override
    {
        return static_cast<uint32>(regNO - *GetIntRegs().begin());
    }
    uint32 GetFpParamRegIdx(regno_t regNO) const override
    {
        return static_cast<uint32>(regNO - *GetFpRegs().begin());
    }
    regno_t GetLastParamsIntReg() override
    {
        return R7;
    }
    regno_t GetLastParamsFpReg() override
    {
        return V7;
    }
    uint32 GetAllRegNum() override
    {
        return kAllRegNum;
    }
    regno_t GetInvalidReg() override
    {
        return kRinvalid;
    }
    bool IsVirtualRegister(const RegOperand &regOpnd) override
    {
        return regOpnd.GetRegisterNumber() > kAllRegNum;
    }
    bool IsVirtualRegister(regno_t regno) override
    {
        return regno > kAllRegNum;
    }
    uint32 GetReservedSpillReg() override
    {
        return R16;
    }
    uint32 GetSecondReservedSpillReg() override
    {
        return R17;
    }

    bool IsReservedReg(regno_t regNO) const
    {
        return (regNO == R16);
    }

    void Init() override;
    void Fini() override;
    bool IsCalleeSavedReg(regno_t regno) const override;
    bool IsYieldPointReg(regno_t regNO) const override;
    bool IsUnconcernedReg(regno_t regNO) const override;
    bool IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) override;
    RegOperand *GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag = 0) override;
    Insn *BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
    Insn *BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
    MemOperand *GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize) override;
    MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, const RegNoPair &regNoPair, bool isDest,
                                                   Insn &insn, bool &isOutOfRange) override;
    void FreeSpillRegMem(regno_t vrNum) override;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REG_INFO_H */
