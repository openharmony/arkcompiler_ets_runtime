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

#include <x86_64/x64_cg.h>
#include "x64_cgfunc.h"
#include "x64_memlayout.h"
#include "x64_isa.h"
#include "assembler/operand.h"

namespace maplebe {
/* null implementation yet */
void X64CGFunc::GenSaveMethodInfoCode(BB &bb)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::GenerateCleanupCode(BB &bb)
{
    CHECK_FATAL(false, "NIY");
}
bool X64CGFunc::NeedCleanup()
{
    CHECK_FATAL(false, "NIY");
    return false;
}
void X64CGFunc::GenerateCleanupCodeForExtEpilog(BB &bb)
{
    CHECK_FATAL(false, "NIY");
}
uint32 X64CGFunc::FloatParamRegRequired(MIRStructType *structType, uint32 &fpSize)
{
    CHECK_FATAL(false, "NIY");
    return 0;
}
void X64CGFunc::AssignLmbcFormalParams()
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::LmbcGenSaveSpForAlloca()
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::MergeReturn()
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::HandleRCCall(bool begin, const MIRSymbol *retRef)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::HandleRetCleanup(NaryStmtNode &retNode)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectDassign(DassignNode &stmt, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectDassignoff(DassignoffNode &stmt, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectRegassign(RegassignNode &stmt, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAbort()
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAssertNull(UnaryStmtNode &stmt)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAsm(AsmNode &node)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAggDassign(DassignNode &stmt)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassign(IassignNode &stmt)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassignoff(IassignoffNode &stmt)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassignfpoff(IassignFPoffNode &stmt, Operand &opnd)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassignspoff(PrimType pTy, int32 offset, Operand &opnd)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectBlkassignoff(BlkassignoffNode &bNode, Operand *src)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAggIassign(IassignNode &stmt, Operand &lhsAddrOpnd)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectReturnSendOfStructInRegs(BaseNode *x)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectReturn(Operand *opnd)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIgoto(Operand *opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectGoto(GotoNode &stmt)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCall(CallNode &callNode)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIcall(IcallNode &icallNode, Operand &fptrOpnd)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIntrinCall(IntrinsiccallNode &intrinsiccallNode)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrinopNode, std::string name)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCclz(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCctz(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCpopcount(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCparity(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCclrsb(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCisaligned(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCalignup(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCaligndown(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCSyncBoolCmpSwap(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCSyncValCmpSwap(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCSyncLockTestSet(IntrinsicopNode &intrinopNode, PrimType pty)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCSyncFetch(IntrinsicopNode &intrinsicopNode, Opcode op, bool fetchBefore)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCSyncSynchronize(IntrinsicopNode &intrinsicopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCAtomicLoadN(IntrinsicopNode &intrinsicopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCAtomicExchangeN(IntrinsicopNode &intrinsicopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCReturnAddress(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectMembar(StmtNode &membar)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectComment(CommentNode &comment)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::HandleCatch()
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectDread(const BaseNode &parent, AddrofNode &expr)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectRegread(RegreadNode &expr)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectAddrof(AddrofNode &expr, const BaseNode &parent, bool isAddrofoff)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectAddrofoff(AddrofoffNode &expr, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}

Operand &X64CGFunc::SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
Operand &X64CGFunc::SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
Operand *X64CGFunc::SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset,
                                PrimType finalBitFieldDestType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectIreadoff(const BaseNode &parent, IreadoffNode &ireadoff)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectIreadfpoff(const BaseNode &parent, IreadFPoffNode &ireadoff)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectIntConst(const MIRIntConst &intConst)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectStrConst(MIRStrConst &strConst)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectStr16Const(MIRStr16Const &strConst)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectMadd(Operand &resOpnd, Operand &opndM0, Operand &opndM1, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMadd(BinaryNode &node, Operand &opndM0, Operand &opndM1, Operand &opnd1,
                               const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectRor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand &X64CGFunc::SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
Operand *X64CGFunc::SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectLand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectLor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent, bool parentIsBr)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectAbs(UnaryNode &node, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectRecip(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectRetype(TypeCvtNode &node, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectSelect(TernaryNode &node, Operand &cond, Operand &opnd0, Operand &opnd1,
                                 const BaseNode &parent, bool hasCompare)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectMalloc(UnaryNode &call, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand &X64CGFunc::SelectCopy(Operand &src, PrimType srcType, PrimType dstType)
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
Operand *X64CGFunc::SelectAlloca(UnaryNode &call, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectGCMalloc(GCMallocNode &call)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectJarrayMalloc(JarrayMallocNode &call, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectLazyLoad(Operand &opnd0, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::GenerateYieldpoint(BB &bb)
{
    CHECK_FATAL(false, "NIY");
}
Operand &X64CGFunc::ProcessReturnReg(PrimType primType, int32 sReg)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
Operand &X64CGFunc::GetOrCreateRflag()
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
const Operand *X64CGFunc::GetRflag() const
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
const Operand *X64CGFunc::GetFloatRflag() const
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
const LabelOperand *X64CGFunc::GetLabelOperand(LabelIdx labIdx) const
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
LabelOperand &X64CGFunc::GetOrCreateLabelOperand(LabelIdx labIdx)
{
    std::string lableName = ".L." + std::to_string(GetUniqueID()) + "__" + std::to_string(labIdx);
    return GetOpndBuilder()->CreateLabel(lableName.c_str(), labIdx);
}
LabelOperand &X64CGFunc::GetOrCreateLabelOperand(BB &bb)
{
    CHECK_FATAL(false, "NIY");
    LabelOperand *a;
    return *a;
}
RegOperand &X64CGFunc::CreateVirtualRegisterOperand(regno_t vRegNO)
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
RegOperand &X64CGFunc::GetOrCreateVirtualRegisterOperand(regno_t vRegNO)
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
RegOperand &X64CGFunc::GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd)
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
RegOperand &X64CGFunc::GetOrCreateFramePointerRegOperand()
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
RegOperand &X64CGFunc::GetOrCreateStackBaseRegOperand()
{
    return GetOpndBuilder()->CreatePReg(x64::RBP, GetPointerSize() * kBitsPerByte, kRegTyInt);
}
RegOperand &X64CGFunc::GetZeroOpnd(uint32 size)
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
Operand &X64CGFunc::CreateCfiRegOperand(uint32 reg, uint32 size)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
Operand &X64CGFunc::GetTargetRetOperand(PrimType primType, int32 sReg)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
Operand &X64CGFunc::CreateImmOperand(PrimType primType, int64 val)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
}
void X64CGFunc::ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn, regno_t regno)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::CleanupDeadMov(bool dump)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realCallerSave)
{
    CHECK_FATAL(false, "NIY");
}
bool X64CGFunc::IsFrameReg(const RegOperand &opnd) const
{
    CHECK_FATAL(false, "NIY");
    return false;
}
RegOperand *X64CGFunc::SelectVectorAddLong(PrimType rTy, Operand *o1, Operand *o2, PrimType oty, bool isLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorAddWiden(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, bool isLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorAbs(PrimType rType, Operand *o1)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
                                         Opcode opc)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
                                             Opcode opc)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorCompare(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, Opcode opc)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorFromScalar(PrimType pType, Operand *opnd, PrimType sType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorDup(PrimType rType, Operand *src, bool getLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorGetElement(PrimType rType, Operand *src, PrimType sType, int32 lane)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2, Operand *o3,
                                        PrimType oTyp3)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorMerge(PrimType rTyp, Operand *o1, Operand *o2, int32 iNum)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
                                        bool isLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNeg(PrimType rType, Operand *o1)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNot(PrimType rType, Operand *o1)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorReverse(PrimType rtype, Operand *src, PrimType stype, uint32 size)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorSetElement(Operand *eOp, PrimType eTyp, Operand *vOpd, PrimType vTyp, int32 lane)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorShift(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
                                         Opcode opc)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oType, Operand *o2, bool isLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1, Operand *o2, PrimType otyp2,
                                            bool isLow, bool isWide)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorSum(PrimType rtype, Operand *o1, PrimType oType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorTableLookup(PrimType rType, Operand *o1, Operand *o2)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
RegOperand *X64CGFunc::SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectIntrinsicOpWithNParams(IntrinsicopNode &intrinopNode, PrimType retType,
                                                 const std::string &name)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::ProcessLazyBinding()
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::DBGFixCallFrameLocationOffsets()
{
    CHECK_FATAL(false, "NIY");
}
MemOperand *X64CGFunc::GetPseudoRegisterSpillMemoryOperand(PregIdx idx)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
int32 X64CGFunc::GetBaseOffset(const SymbolAlloc &symbolAlloc)
{
    const auto *symAlloc = static_cast<const X64SymbolAlloc *>(&symbolAlloc);
    /* Call Frame layout of X64
     * Refer to layout in x64_memlayout.h.
     * Do Not change this unless you know what you do
     * memlayout like this
     * rbp position
     * prologue slots --
     * ArgsReg          |
     * Locals           | -- FrameSize
     * Spill            |
     * ArgsStk        --
     */
    constexpr const int32 sizeofFplr = 2 * kIntregBytelen;
    // baseOffset is the offset of this symbol based on the rbp position.
    int32 baseOffset = symAlloc->GetOffset();
    MemSegmentKind sgKind = symAlloc->GetMemSegment()->GetMemSegmentKind();
    auto *memLayout = static_cast<X64MemLayout *>(this->GetMemlayout());
    if (sgKind == kMsSpillReg) {
        /* spill = -(Locals + ArgsReg + baseOffset + ReseverdSlot + kSizeOfPtr) */
        return -(memLayout->GetSizeOfLocals() + memLayout->SizeOfArgsRegisterPassed() + baseOffset +
                 GetFunction().GetFrameReseverdSlot() + GetPointerSize());
    } else if (sgKind == kMsLocals) {
        /* Locals = baseOffset - (ReseverdSlot + Locals + ArgsReg) */
        return baseOffset - (GetFunction().GetFrameReseverdSlot() + memLayout->GetSizeOfLocals() +
                             memLayout->SizeOfArgsRegisterPassed());
    } else if (sgKind == kMsArgsRegPassed) {
        /* ArgsReg = baseOffset - ReseverdSlot  - ArgsReg */
        return baseOffset - GetFunction().GetFrameReseverdSlot() - memLayout->SizeOfArgsRegisterPassed();
    } else if (sgKind == kMsArgsStkPassed) {
        return baseOffset + sizeofFplr;
    } else {
        CHECK_FATAL(false, "sgKind check");
    }
    return 0;
}

RegOperand *X64CGFunc::GetBaseReg(const maplebe::SymbolAlloc &symAlloc)
{
    MemSegmentKind sgKind = symAlloc.GetMemSegment()->GetMemSegmentKind();
    DEBUG_ASSERT(((sgKind == kMsArgsRegPassed) || (sgKind == kMsLocals) || (sgKind == kMsRefLocals) ||
                  (sgKind == kMsArgsToStkPass) || (sgKind == kMsArgsStkPassed)),
                 "NIY");
    if (sgKind == kMsLocals || sgKind == kMsArgsRegPassed || sgKind == kMsArgsStkPassed) {
        return &GetOpndBuilder()->CreatePReg(x64::RBP, GetPointerSize() * kBitsPerByte, kRegTyInt);
    } else {
        CHECK_FATAL(false, "NIY sgKind");
    }
    return nullptr;
}

void X64CGFunc::FreeSpillRegMem(regno_t vrNum)
{
    MemOperand *memOpnd = nullptr;

    auto p = spillRegMemOperands.find(vrNum);
    if (p != spillRegMemOperands.end()) {
        memOpnd = p->second;
    }

    if ((memOpnd == nullptr) && IsVRegNOForPseudoRegister(vrNum)) {
        auto pSecond = pRegSpillMemOperands.find(GetPseudoRegIdxFromVirtualRegNO(vrNum));
        if (pSecond != pRegSpillMemOperands.end()) {
            memOpnd = pSecond->second;
        }
    }

    if (memOpnd == nullptr) {
        DEBUG_ASSERT(false, "free spillreg have no mem");
        return;
    }

    uint32 size = memOpnd->GetSize();
    MapleUnorderedMap<uint32, SpillMemOperandSet *>::iterator iter;
    if ((iter = reuseSpillLocMem.find(size)) != reuseSpillLocMem.end()) {
        iter->second->Add(*memOpnd);
    } else {
        reuseSpillLocMem[size] = memPool->New<SpillMemOperandSet>(*GetFuncScopeAllocator());
        reuseSpillLocMem[size]->Add(*memOpnd);
    }
}

MemOperand *X64CGFunc::GetOrCreatSpillMem(regno_t vrNum, uint32 memSize)
{
    /* NOTES: must used in RA, not used in other place. */
    if (IsVRegNOForPseudoRegister(vrNum)) {
        auto p = pRegSpillMemOperands.find(GetPseudoRegIdxFromVirtualRegNO(vrNum));
        if (p != pRegSpillMemOperands.end()) {
            return p->second;
        }
    }

    auto p = spillRegMemOperands.find(vrNum);
    if (p == spillRegMemOperands.end()) {
        uint32 memBitSize = k64BitSize;
        auto it = reuseSpillLocMem.find(memBitSize);
        if (it != reuseSpillLocMem.end()) {
            MemOperand *memOpnd = it->second->GetOne();
            if (memOpnd != nullptr) {
                spillRegMemOperands.emplace(std::pair<regno_t, MemOperand*>(vrNum, memOpnd));
                return memOpnd;
            }
        }

        RegOperand &baseOpnd = GetOrCreateStackBaseRegOperand();
        int32 offset = GetOrCreatSpillRegLocation(vrNum, memBitSize / kBitsPerByte);
        MemOperand *memOpnd = &GetOpndBuilder()->CreateMem(baseOpnd, offset, memBitSize);
        spillRegMemOperands.emplace(std::pair<regno_t, MemOperand*>(vrNum, memOpnd));
        return memOpnd;
    } else {
        return p->second;
    }
}

void X64OpndDumpVisitor::Visit(maplebe::RegOperand *v)
{
    DumpOpndPrefix();
    LogInfo::MapleLogger() << "reg ";
    DumpRegInfo(*v);
    DumpSize(*v);
    DumpReferenceInfo(*v);
    const OpndDesc *regDesc = GetOpndDesc();
    LogInfo::MapleLogger() << " [";
    if (regDesc->IsRegDef()) {
        LogInfo::MapleLogger() << "DEF,";
    }
    if (regDesc->IsRegUse()) {
        LogInfo::MapleLogger() << "USE,";
    }
    LogInfo::MapleLogger() << "]";
    DumpOpndSuffix();
}

void X64OpndDumpVisitor::Visit(CommentOperand *v)
{
    LogInfo::MapleLogger() << ":#" << v->GetComment();
}

void X64OpndDumpVisitor::Visit(maplebe::ImmOperand *v)
{
    DumpOpndPrefix();
    LogInfo::MapleLogger() << "imm ";
    LogInfo::MapleLogger() << v->GetValue();
    DumpSize(*v);
    DumpReferenceInfo(*v);
    DumpOpndSuffix();
}

void X64OpndDumpVisitor::Visit(maplebe::MemOperand *v)
{
    DumpOpndPrefix();
    LogInfo::MapleLogger() << "mem ";
    if (v->GetBaseRegister() != nullptr) {
        DumpRegInfo(*v->GetBaseRegister());
        if (v->GetOffsetOperand() != nullptr) {
            LogInfo::MapleLogger() << " + " << v->GetOffsetOperand()->GetValue();
        }
    }
    DumpSize(*v);
    DumpReferenceInfo(*v);
    DumpOpndSuffix();
}
void X64OpndDumpVisitor::DumpRegInfo(maplebe::RegOperand &v)
{
    if (v.GetRegisterNumber() > baseVirtualRegNO) {
        LogInfo::MapleLogger() << "V" << v.GetRegisterNumber();
    } else {
        uint8 regType = -1;
        switch (v.GetSize()) {
            case k8BitSize:
                /* use lower 8-bits */
                regType = X64CG::kR8LowList;
                break;
            case k16BitSize:
                regType = X64CG::kR16List;
                break;
            case k32BitSize:
                regType = X64CG::kR32List;
                break;
            case k64BitSize:
                regType = X64CG::kR64List;
                break;
            default:
                CHECK_FATAL(false, "unkown reg size");
                break;
        }
        assembler::Reg reg = assembler::kRegArray[regType][v.GetRegisterNumber()];
        LogInfo::MapleLogger() << "%" << assembler::kRegStrMap.at(reg);
    }
}

void X64OpndDumpVisitor::Visit(maplebe::FuncNameOperand *v)
{
    DumpOpndPrefix();
    LogInfo::MapleLogger() << "funcname ";
    LogInfo::MapleLogger() << v->GetName();
    DumpSize(*v);
    DumpReferenceInfo(*v);
    DumpOpndSuffix();
}

void X64OpndDumpVisitor::Visit(maplebe::ListOperand *v)
{
    DumpOpndPrefix();
    LogInfo::MapleLogger() << "list ";

    MapleList<RegOperand *> opndList = v->GetOperands();
    for (auto it = opndList.begin(); it != opndList.end();) {
        (*it)->Dump();
        LogInfo::MapleLogger() << (++it == opndList.end() ? "" : " ,");
    }
    DumpSize(*v);
    DumpOpndSuffix();
}

void X64OpndDumpVisitor::Visit(maplebe::LabelOperand *v)
{
    DumpOpndPrefix();
    LogInfo::MapleLogger() << "label ";
    LogInfo::MapleLogger() << v->GetLabelIndex();
    DumpSize(*v);
    DumpOpndSuffix();
}

void X64OpndDumpVisitor::Visit(PhiOperand *v)
{
    CHECK_FATAL(false, "NIY");
}

void X64OpndDumpVisitor::Visit(CondOperand *v)
{
    CHECK_FATAL(false, "do not use this operand, it will be eliminated soon");
}
void X64OpndDumpVisitor::Visit(StImmOperand *v)
{
    CHECK_FATAL(false, "do not use this operand, it will be eliminated soon");
}
void X64OpndDumpVisitor::Visit(BitShiftOperand *v)
{
    CHECK_FATAL(false, "do not use this operand, it will be eliminated soon");
}
void X64OpndDumpVisitor::Visit(ExtendShiftOperand *v)
{
    CHECK_FATAL(false, "do not use this operand, it will be eliminated soon");
}
}  // namespace maplebe
