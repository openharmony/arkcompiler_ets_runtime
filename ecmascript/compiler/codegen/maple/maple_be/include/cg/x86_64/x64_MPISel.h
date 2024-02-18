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
    Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent) override;
    Operand *SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) override;
    Operand *SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) override;
    Operand *SelectFloatingConst(MIRConst &floatingConst, PrimType primType) const override;
    void SelectGoto(GotoNode &stmt) override;
    void SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode) override;
    void SelectAggIassign(IassignNode &stmt, Operand &AddrOpnd, Operand &opndRhs) override;
    void SelectAggDassign(maplebe::MirTypeInfo &lhsInfo, MemOperand &symbolMem, Operand &opndRhs) override;
    void SelectAggCopy(MemOperand &lhs, MemOperand &rhs, uint32 copySize) override;
    void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) override;
    void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode, Operand &opnd0) override;
    void SelectIgoto(Operand &opnd0) override;
    Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectLnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectSelect(TernaryNode &expr, Operand &cond, Operand &trueOpnd, Operand &falseOpnd,
                          const BaseNode &parent) override;
    Operand *SelectStrLiteral(ConststrNode &constStr) override;
    void SelectIntAggCopyReturn(MemOperand &symbolMem, uint64 aggSize) override;
    /* Create the operand interface directly */
    MemOperand &CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0);
    Operand *SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectCexp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
    void SelectAsm(AsmNode &node) override;
    Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;

private:
    MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId = 0) const override;
    MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) const override;
    Insn &AppendCall(x64::X64MOP_t mOp, Operand &targetOpnd, ListOperand &paramOpnds, ListOperand &retOpnds);
    void SelectCalleeReturn(MIRType *retType, ListOperand &retOpnds);

    /* Inline function implementation of va_start */
    void GenCVaStartIntrin(RegOperand &opnd, uint32 stkSize);

    /* Subclass private instruction selector function */
    void SelectCVaStart(const IntrinsiccallNode &intrnNode);
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
    RegOperand &GetTargetStackPointer(PrimType primType) override;
    RegOperand &GetTargetBasicPointer(PrimType primType) override;
    std::tuple<Operand *, size_t, MIRType *> GetMemOpndInfoFromAggregateNode(BaseNode &argExpr);
    void SelectParmListForAggregate(BaseNode &argExpr, X64CallConvImpl &parmLocator, bool isArgUnused);
    void CreateCallStructParamPassByReg(MemOperand &memOpnd, regno_t regNo, uint32 parmNum);
    void CreateCallStructParamPassByStack(MemOperand &addrOpnd, int32 symSize, int32 baseOffset);
    void SelectAggCopyReturn(const MIRSymbol &symbol, MIRType &symbolType, uint64 symbolSize);
    bool IsParamStructCopy(const MIRSymbol &symbol);
    void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    void SelectLibCall(const std::string &funcName, std::vector<Operand *> &opndVec, PrimType primType,
                       Operand *retOpnd, PrimType retType);
    void SelectLibCallNArg(const std::string &funcName, std::vector<Operand *> &opndVec, std::vector<PrimType> pt,
                           Operand *retOpnd, PrimType retType);
    void SelectPseduoForReturn(std::vector<RegOperand *> &retRegs);
    RegOperand *PrepareMemcpyParm(MemOperand &memOperand, MOperator mOp);
    RegOperand *PrepareMemcpyParm(uint64 copySize);
    RegOperand &SelectSpecialRegread(PregIdx pregIdx, PrimType primType) override;
    void SelectRetypeFloat(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) override;

    /* save param pass by reg */
    std::vector<std::tuple<RegOperand *, Operand *, PrimType>> paramPassByReg;
};
}  // namespace maplebe

#endif /* MAPLEBE_INCLUDE_X64_MPISEL_H */
