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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H
#define MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H
#include "reg_info.h"
#include "x64_abi.h"
#include "x64_cg.h"

namespace maplebe {
class X64CallConvImpl;
static const std::map<regno_t, uint32> x64IntParamsRegIdx = {{x64::RAX, 0}, {x64::RDI, 1}, {x64::RSI, 2}, {x64::RDX, 3},
                                                             {x64::RCX, 4}, {x64::R8, 5},  {x64::R9, 6}};

class X64RegInfo : public RegisterInfo {
public:
    X64RegInfo(MapleAllocator &mallocator, CallConvKind callConv) : RegisterInfo(mallocator), callConv(callConv) {}

    ~X64RegInfo() override = default;

    void Init() override;
    void Fini() override;
    bool IsCalleeSavedReg(regno_t regno) const override;
    bool IsYieldPointReg(regno_t regNO) const override;
    bool IsUnconcernedReg(regno_t regNO) const override;
    RegOperand *GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag) override;
    Insn *BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
    Insn *BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) override;
    void FreeSpillRegMem(regno_t vrNum) override;
    MemOperand *GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize) override;
    MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, const RegNoPair &regNoPair, bool isDest,
                                                   Insn &insn, bool &isOutOfRange) override;
    bool IsGPRegister(regno_t regNO) const override
    {
        return x64::IsGPRegister(static_cast<X64reg>(regNO));
    }
    /* Refactor later: Integrate parameters and return Reg */
    uint32 GetIntRegsParmsNum() override
    {
        /*Parms: rdi, rsi, rdx, rcx, r8, r9; Ret: rax, rdx */
        return X64CallConvImpl::GetCallConvInfo(callConv).GetIntParamRegsNum() + 1;
    }
    uint32 GetIntRetRegsNum() override
    {
        return X64CallConvImpl::GetCallConvInfo(callConv).GetIntReturnRegsNum();
    }
    uint32 GetFpRetRegsNum() override
    {
        return X64CallConvImpl::GetCallConvInfo(callConv).GetFloatReturnRegsNum();
    }
    regno_t GetLastParamsIntReg() override
    {
        return static_cast<regno_t>(X64CallConvImpl::GetCallConvInfo(callConv).GetIntParamRegs().back());
    }
    uint32 GetNormalUseOperandNum() override
    {
        return 0;
    }
    regno_t GetIntRetReg(uint32 idx) override
    {
        CHECK_FATAL(idx <= GetIntRetRegsNum(), "index out of range in IntRetReg");
        return static_cast<regno_t>(X64CallConvImpl::GetCallConvInfo(callConv).GetIntReturnRegs()[idx]);
    }
    regno_t GetFpRetReg(uint32 idx) override
    {
        CHECK_FATAL(idx <= GetFpRetRegsNum(), "index out of range in IntRetReg");
        return static_cast<regno_t>(X64CallConvImpl::GetCallConvInfo(callConv).GetFloatReturnRegs()[idx]);
    }
    uint32 GetIntParamRegIdx(regno_t regNO) const override
    {
        CHECK_FATAL(GetIntRegs().size(), "should be init before");
        return static_cast<uint32>(regNO - *GetIntRegs().begin());
    }
    uint32 GetFpParamRegIdx(regno_t regNO) const override
    {
        CHECK_FATAL(GetFpRegs().size(), "should be init before");
        return static_cast<uint32>(regNO - *GetFpRegs().begin());
    }
    regno_t GetLastParamsFpReg() override
    {
        return x64::kRinvalid;
    }
    uint32 GetFloatRegsParmsNum() override
    {
        return X64CallConvImpl::GetCallConvInfo(callConv).GetFloatParamRegsNum();
    }
    uint32 GetFloatRegsRetsNum()
    {
        return X64CallConvImpl::GetCallConvInfo(callConv).GetFloatReturnRegsNum();
    }
    uint32 GetAllRegNum() override
    {
        return x64::kAllRegNum;
    }
    regno_t GetInvalidReg() override
    {
        return x64::kRinvalid;
    }
    bool IsAvailableReg(regno_t regNO) const override
    {
        return x64::IsAvailableReg(static_cast<X64reg>(regNO));
    }
    bool IsVirtualRegister(const RegOperand &regOpnd) override
    {
        return regOpnd.GetRegisterNumber() > x64::kAllRegNum;
    }
    bool IsVirtualRegister(regno_t regno) override
    {
        return regno > x64::kAllRegNum;
    }
    uint32 GetReservedSpillReg() override
    {
        return x64::kRinvalid;
    }
    uint32 GetSecondReservedSpillReg() override
    {
        return x64::kRinvalid;
    }
    bool IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) override
    {
        return x64::IsSpillRegInRA(static_cast<X64reg>(regNO), has3RegOpnd);
    }

private:
    CallConvKind callConv;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_REG_INFO_H */
