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

#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H

#include "reg_alloc.h"
#include "operand.h"
#include "cgfunc.h"

namespace maplebe {
class DefaultO0RegAllocator : public RegAllocator {
public:
    DefaultO0RegAllocator(CGFunc &cgFunc, MemPool &memPool)
        : RegAllocator(cgFunc, memPool),
          calleeSaveUsed(alloc.Adapter()),
          availRegSet(alloc.Adapter()),
          regMap(std::less<uint32>(), alloc.Adapter()),
          liveReg(std::less<uint8>(), alloc.Adapter()),
          allocatedSet(std::less<Operand *>(), alloc.Adapter()),
          regLiveness(alloc.Adapter()),
          rememberRegs(alloc.Adapter())
    {
        availRegSet.resize(regInfo->GetAllRegNum());
    }

    ~DefaultO0RegAllocator() override
    {
        regInfo = nullptr;
    }

    bool AllocateRegisters() override;

    void InitAvailReg();

    bool AllocatePhysicalRegister(const RegOperand &opnd);
    void ReleaseReg(regno_t reg);
    void ReleaseReg(const RegOperand &regOpnd);
    void GetPhysicalRegisterBank(RegType regType, regno_t &start, regno_t &end) const;
    void AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx);
    void AllocHandleDest(Insn &insn, Operand &opnd, uint32 idx);
    void AllocHandleSrcList(Insn &insn, Operand &opnd, uint32 idx);
    void AllocHandleSrc(Insn &insn, Operand &opnd, uint32 idx);
    void SaveCalleeSavedReg(const RegOperand &opnd);

protected:
    Operand *HandleRegOpnd(Operand &opnd);
    Operand *HandleMemOpnd(Operand &opnd);
    Operand *AllocSrcOpnd(Operand &opnd);
    Operand *AllocDestOpnd(Operand &opnd, const Insn &insn);
    uint32 GetRegLivenessId(regno_t regNo);
    bool CheckRangesOverlap(const std::pair<uint32, uint32> &range1,
                            const MapleVector<std::pair<uint32, uint32>> &ranges2) const;
    void SetupRegLiveness(BB *bb);
    void SetupRegLiveness(MemOperand &opnd, uint32 insnId);
    void SetupRegLiveness(ListOperand &opnd, uint32 insnId, bool isDef);
    void SetupRegLiveness(RegOperand &opnd, uint32 insnId, bool isDef);
	
    MapleSet<regno_t> calleeSaveUsed;
    MapleVector<bool> availRegSet;
    MapleMap<uint32, regno_t> regMap; /* virtual-register-to-physical-register map */
    MapleSet<uint8> liveReg;          /* a set of currently live physical registers */
    MapleSet<Operand *> allocatedSet; /* already allocated */
    MapleMap<regno_t, MapleVector<std::pair<uint32, uint32>>> regLiveness;
    MapleVector<regno_t> rememberRegs;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_REG_ALLOC_BASIC_H */
