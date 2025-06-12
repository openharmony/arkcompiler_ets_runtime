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

#ifndef MAPLEBE_INCLUDE_CG_IRBUILDER_H
#define MAPLEBE_INCLUDE_CG_IRBUILDER_H

#include "reg_info.h"
#include "insn.h"
#include "operand.h"

namespace maplebe {
class InsnBuilder {
public:
    explicit InsnBuilder(MemPool &memPool) : mp(&memPool) {}
    virtual ~InsnBuilder()
    {
        mp = nullptr;
    }

    template <class Target>
    Insn &BuildInsn(MOperator opCode)
    {
        return BuildInsn(opCode, Target::kMd[opCode]);
    }
    Insn &BuildInsn(MOperator opCode, const InsnDesc &idesc);
    Insn &BuildInsn(MOperator opCode, Operand &o0);
    Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1);
    Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2);
    Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3);
    Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3, Operand &o4);
    Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3, Operand &o4, Operand &o5);
    Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2,
                    Operand &o3, Operand &o4, Operand &o5, Operand &o6);
    Insn &BuildInsn(MOperator opCode, std::vector<Operand *> &opnds);

    Insn &BuildCfiInsn(MOperator opCode);

    uint32 GetCreatedInsnNum() const
    {
        return createdInsnNum;
    }

    void SetDebugComment(const MapleString* comment)
    {
        currDedugComment = comment;
    }

    void ClearDebugComment()
    {
        currDedugComment = nullptr;
    }

protected:
    MemPool *mp;

private:
    void IncreaseInsnNum()
    {
        createdInsnNum++;
    }
    uint32 createdInsnNum = 0;
    const MapleString *currDedugComment { nullptr };
};

constexpr uint32 baseVirtualRegNO = 200; /* avoid conflicts between virtual and physical */
class OperandBuilder {
public:
    explicit OperandBuilder(MemPool &mp, uint32 mirPregNum = 0) : alloc(&mp)
    {
        virtualReg.SetCount(mirPregNum);
    }

    /* create an operand in cgfunc when no mempool is supplied */
    ImmOperand &CreateImm(uint32 size, int64 value, MemPool *mp = nullptr);
    ImmOperand &CreateImm(uint32 size, int64 value, bool isSigned, MemPool *mp = nullptr);
    ImmOperand &CreateImm(const MIRSymbol &symbol, int64 offset, int32 relocs, MemPool *mp = nullptr);
    OfstOperand &CreateOfst(int64 offset, uint32 size, MemPool *mp = nullptr);
    MemOperand &CreateMem(uint32 size, MemPool *mp = nullptr);
    MemOperand &CreateMem(RegOperand &baseOpnd, int64 offset, uint32 size, MemPool *mp = nullptr);
    MemOperand &CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &ofstOperand, MemPool *mp = nullptr);
    MemOperand &CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &ofstOperand, const MIRSymbol &symbol,
                          MemPool *mp = nullptr);
    BitShiftOperand &CreateBitShift(BitShiftOperand::ShiftOp op, uint32 amount, uint32 bitLen, MemPool *mp = nullptr);
    RegOperand &CreateVReg(uint32 size, RegType type, MemPool *mp = nullptr);
    RegOperand &CreateVReg(regno_t vRegNO, uint32 size, RegType type, MemPool *mp = nullptr);
    RegOperand &CreatePReg(regno_t pRegNO, uint32 size, RegType type, MemPool *mp = nullptr);
    ListOperand &CreateList(MemPool *mp = nullptr);
    FuncNameOperand &CreateFuncNameOpnd(MIRSymbol &symbol, MemPool *mp = nullptr);
    LabelOperand &CreateLabel(const char *parent, LabelIdx idx, MemPool *mp = nullptr);

    uint32 GetCurrentVRegNum() const
    {
        return virtualReg.GetCount();
    }

protected:
    MapleAllocator alloc;

private:
    VregInfo virtualReg;
    /* reg bank for multiple use */
};
}  // namespace maplebe
#endif  // MAPLEBE_INCLUDE_CG_IRBUILDER_H
