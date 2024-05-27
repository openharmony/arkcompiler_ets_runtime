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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EBO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EBO_H

#include "ebo.h"
#include "aarch64_operand.h"
#include "aarch64_cgfunc.h"

namespace maplebe {

class AArch64Ebo : public Ebo {
public:
    AArch64Ebo(CGFunc &func, MemPool &memPool, LiveAnalysis *live, bool before, const std::string &phase)
        : Ebo(func, memPool, live, before, phase), callerSaveRegTable(eboAllocator.Adapter())
    {
        a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    }

    enum ExtOpTable : uint8;

    ~AArch64Ebo() override = default;

protected:
    MapleVector<RegOperand *> callerSaveRegTable;
    AArch64CGFunc *a64CGFunc;
    int32 GetOffsetVal(const MemOperand &mem) const override;
    OpndInfo *OperandInfoDef(BB &currentBB, Insn &currentInsn, Operand &localOpnd) override;
    const RegOperand &GetRegOperand(const Operand &opnd) const override;
    bool IsGlobalNeeded(Insn &insn) const override;
    bool IsDecoupleStaticOp(Insn &insn) const override;
    bool OperandEqSpecial(const Operand &op1, const Operand &op2) const override;
    void BuildCallerSaveRegisters() override;
    void DefineAsmRegisters(InsnInfo &insnInfo) override;
    void DefineCallerSaveRegisters(InsnInfo &insnInfo) override;
    void DefineReturnUseRegister(Insn &insn) override;
    void DefineCallUseSpecialRegister(Insn &insn) override;
    void DefineClinitSpecialRegisters(InsnInfo &insnInfo) override;
    bool CombineExtensionAndLoad(Insn *insn, const MapleVector<OpndInfo *> &origInfos, ExtOpTable idx, bool is64Bits);
    bool IsMovToSIMDVmov(Insn &insn, const Insn &replaceInsn) const override;
    bool IsPseudoRet(Insn &insn) const override;
    bool IsAdd(const Insn &insn) const override;
    bool IsFmov(const Insn &insn) const override;
    bool IsClinitCheck(const Insn &insn) const override;
    bool IsLastAndBranch(BB &bb, Insn &insn) const override;
    bool IsSameRedefine(BB &bb, Insn &insn, OpndInfo &opndInfo) const override;
    bool ResIsNotDefAndUse(Insn &insn) const override;
    bool LiveOutOfBB(const Operand &opnd, const BB &bb) const override;
    bool IsInvalidReg(const RegOperand &opnd) const override;
    bool IsZeroRegister(const Operand &opnd) const override;
    bool IsConstantImmOrReg(const Operand &opnd) const override;
    bool OperandLiveAfterInsn(const RegOperand &regOpnd, Insn &insn) const;
    bool ValidPatternForCombineExtAndLoad(OpndInfo *prevOpndInfo, Insn *insn, MOperator newMop, MOperator oldMop,
                                          const RegOperand &opnd);

private:
    /* The number of elements in callerSaveRegTable must less then 45. */
    static constexpr int32 kMaxCallerSaveReg = 45;
    MOperator ExtLoadSwitchBitSize(MOperator lowMop) const;
    bool CheckCondCode(const CondOperand &cond) const;
    bool CombineMultiplyAdd(Insn *insn, const Insn *prevInsn, InsnInfo *insnInfo, Operand *addOpnd, bool is64bits,
                            bool isFp) const;
    bool CheckCanDoMadd(Insn *insn, OpndInfo *opndInfo, int32 pos, bool is64bits, bool isFp);
    bool CombineMultiplySub(Insn *insn, OpndInfo *opndInfo, bool is64bits, bool isFp) const;
    bool CombineMultiplyNeg(Insn *insn, OpndInfo *opndInfo, bool is64bits, bool isFp) const;
    bool SimplifyBothConst(BB &bb, Insn &insn, const ImmOperand &immOperand0, const ImmOperand &immOperand1,
                           uint32 opndSize) const;
    ConditionCode GetReverseCond(const CondOperand &cond) const;
    bool CombineLsrAnd(Insn &insn, const OpndInfo &opndInfo, bool is64bits, bool isFp) const;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_EBO_H */
