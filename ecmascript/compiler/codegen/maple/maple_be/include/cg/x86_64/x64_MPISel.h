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

#ifndef MAPLEBE_INCLUDE_X64_MPISEL_H
#define MAPLEBE_INCLUDE_X64_MPISEL_H

#include "isel.h"
#include "x64_call_conv.h"

namespace maplebe {
class X64MPIsel : public MPISel {
public:
    X64MPIsel(MemPool &mp, MapleAllocator &allocator, CGFunc &f) : MPISel(mp, allocator, f) {}
    ~X64MPIsel() override = default;
    void SelectReturn(NaryStmtNode &retNode, Operand &opnd) override;
    void SelectReturn() override;
    void SelectCall(CallNode &callNode) override;
    void SelectIcall(IcallNode &icallNode) override;
    Operand &ProcessReturnReg(PrimType primType, int32 sReg) override;
    Operand &GetTargetRetOperand(PrimType primType, int32 sReg) override;
    Operand *SelectFloatingConst(MIRConst &floatingConst, PrimType primType) const override;
    void SelectGoto(GotoNode &stmt) override;
    void SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode) override;
    void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) override;
    void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode, Operand &opnd0) override;
    Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectLnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    /* Create the operand interface directly */
    MemOperand &CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0);
    Operand *SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;

private:
    MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId = 0) const override;
    MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) const override;
    Insn &AppendCall(x64::X64MOP_t mOp, Operand &targetOpnd, ListOperand &paramOpnds, ListOperand &retOpnds);
    void SelectCalleeReturn(MIRType *retType, ListOperand &retOpnds);

    void SelectOverFlowCall(const IntrinsiccallNode &intrnNode);
    void SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds, uint32 &fpNum);
    void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);

    /* lt/le in float is replaced by gt/ge on swaped operands */
    void SelectCmp(Operand &opnd0, Operand &opnd1, PrimType primType, bool isSwap = false);
    void SelectCmpFloatEq(RegOperand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primResType,
                          PrimType primOpndType);
    void SelectCmpResult(RegOperand &resOpnd, Opcode opCode, PrimType primType, PrimType primOpndType);
    void SelectSelect(Operand &resOpnd, Operand &trueOpnd, Operand &falseOpnd, PrimType primType, Opcode cmpOpcode,
                      PrimType cmpPrimType);

    Operand *SelectDivRem(RegOperand &opnd0, RegOperand &opnd1, PrimType primType, Opcode opcode);
    RegOperand &GetTargetBasicPointer(PrimType primType) override;
    void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    void SelectPseduoForReturn(std::vector<RegOperand *> &retRegs);
    RegOperand &SelectSpecialRegread(PregIdx pregIdx, PrimType primType) override;
    void SelectRetypeFloat(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) override;

    /* save param pass by reg */
    std::vector<std::tuple<RegOperand *, Operand *, PrimType>> paramPassByReg;
};
}  // namespace maplebe

#endif /* MAPLEBE_INCLUDE_X64_MPISEL_H */
