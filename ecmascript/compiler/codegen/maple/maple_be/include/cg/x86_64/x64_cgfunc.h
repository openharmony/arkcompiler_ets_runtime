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

#ifndef MAPLEBE_INCLUDE_CG_X86_64_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_X86_64_CGFUNC_H

#include "cgfunc.h"
#include "x64_memlayout.h"
#include "x64_isa.h"
#include "x64_reg_info.h"
#include "x64_optimize_common.h"

namespace maplebe {
class X64CGFunc : public CGFunc {
public:
    X64CGFunc(MIRModule &mod, CG &c, MIRFunction &f, BECommon &b, MemPool &memPool, StackMemPool &stackMp,
              MapleAllocator &mallocator, uint32 funcId)
        : CGFunc(mod, c, f, b, memPool, stackMp, mallocator, funcId), calleeSavedRegs(mallocator.Adapter())
    {
        CGFunc::SetMemlayout(*memPool.New<X64MemLayout>(b, f, mallocator));
        CGFunc::GetMemlayout()->SetCurrFunction(*this);
        CGFunc::SetTargetRegInfo(*memPool.New<X64RegInfo>(mallocator, X64CallConvImpl::GetCallConvKind(f)));
        CGFunc::GetTargetRegInfo()->SetCurrFunction(*this);
    }
    /* null implementation yet */
    InsnVisitor *NewInsnModifier() override
    {
        return memPool->New<X64InsnVisitor>(*this);
    }
    void MergeReturn() override;
    void SelectDassign(DassignNode &stmt, Operand &opnd0) override;
    void SelectRegassign(RegassignNode &stmt, Operand &opnd0) override;
    void SelectIassign(IassignNode &stmt) override;
    void SelectReturn(Operand *opnd) override;
    void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) override;
    void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) override;
    void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) override;
    void SelectGoto(GotoNode &stmt) override;
    void SelectCall(CallNode &callNode) override;
    void SelectIcall(IcallNode &icallNode) override;
    void SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode) override;
    Operand *SelectCclz(IntrinsicopNode &intrinopNode) override;
    void SelectComment(CommentNode &comment) override;
    Operand *SelectDread(const BaseNode &parent, AddrofNode &expr) override;
    RegOperand *SelectRegread(RegreadNode &expr) override;
    Operand *SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset = 0,
                         PrimType finalBitFieldDestType = kPtyInvalid) override;
    Operand *SelectIntConst(const MIRIntConst &intConst, const BaseNode &parent) override;
    Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) override;
    Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) override;
    void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectAbs(UnaryNode &node, Operand &opnd0) override;
    Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) override;
    Operand *SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) override;
    Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) override;
    Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
    RegOperand &SelectCopy(Operand &src, PrimType srcType, PrimType dstType) override;
    void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) override;
    Operand &GetOrCreateRflag() override;
    const Operand *GetRflag() const override;
    const LabelOperand *GetLabelOperand(LabelIdx labIdx) const override;
    LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) override;
    LabelOperand &GetOrCreateLabelOperand(BB &bb) override;
    RegOperand &CreateVirtualRegisterOperand(regno_t vRegNO) override;
    RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vRegNO) override;
    RegOperand &GetOrCreateFramePointerRegOperand() override;
    RegOperand &GetOrCreateStackBaseRegOperand() override;
    RegOperand &GetZeroOpnd(uint32 size) override;
    Operand &CreateCfiRegOperand(uint32 reg, uint32 size) override;
    Operand &CreateImmOperand(PrimType primType, int64 val) override;
    MemOperand *GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize) override;
    MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx idx) override;

    int32 GetBaseOffset(const SymbolAlloc &symbolAlloc) override;
    RegOperand *GetBaseReg(const SymbolAlloc &symAlloc) override;

    void AddtoCalleeSaved(regno_t reg) override
    {
        const auto &[_, flag] = calleeSavedRegs.insert(static_cast<X64reg>(reg));
        DEBUG_ASSERT((IsGPRegister(static_cast<X64reg>(reg)) || IsFPSIMDRegister(static_cast<X64reg>(reg))),
                     "Int or FP registers are expected");
        if (flag) {
            if (IsGPRegister(static_cast<X64reg>(reg))) {
                ++numIntregToCalleeSave;
            } else {
                ++numFpregToCalleeSave;
            }
        }
    }

    const MapleSet<x64::X64reg> &GetCalleeSavedRegs() const
    {
        return calleeSavedRegs;
    }

    uint32 SizeOfCalleeSaved() const
    {
        uint32 size = numIntregToCalleeSave * kX64IntregBytelen + numFpregToCalleeSave * kX64FpregBytelen;
        return RoundUp(size, GetMemlayout()->GetStackPtrAlignment());
    }

    void FreeSpillRegMem(regno_t vrNum);

private:
    MapleSet<x64::X64reg> calleeSavedRegs;
    uint32 numIntregToCalleeSave = 0;
    uint32 numFpregToCalleeSave = 0;
};

class X64OpndDumpVisitor : public OpndDumpVisitor {
public:
    explicit X64OpndDumpVisitor(const OpndDesc &operandDesc) : OpndDumpVisitor(operandDesc) {};
    ~X64OpndDumpVisitor() override = default;

private:
    void Visit(RegOperand *v) final;
    void Visit(ImmOperand *v) final;
    void Visit(MemOperand *v) final;
    void Visit(ListOperand *v) final;
    void Visit(CondOperand *v) final;
    void Visit(CommentOperand *v) final;
    void Visit(StImmOperand *v) final;
    void Visit(BitShiftOperand *v) final;
    void Visit(ExtendShiftOperand *v) final;
    void Visit(LabelOperand *v) final;
    void Visit(FuncNameOperand *v) final;
    void Visit(PhiOperand *v) final;
    void DumpRegInfo(RegOperand &v);
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_X86_64_CGFUNC_H */
