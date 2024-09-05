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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H

#include "cgfunc.h"
#include "call_conv.h"
#include "aarch64_abi.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"
#include "aarch64_memlayout.h"
#include "aarch64_reg_info.h"
#include "aarch64_optimize_common.h"
#include "aarch64_call_conv.h"

namespace maplebe {
class AArch64CGFunc : public CGFunc {
public:
    AArch64CGFunc(MIRModule &mod, CG &c, MIRFunction &f, BECommon &b, MemPool &memPool, StackMemPool &stackMp,
                  MapleAllocator &mallocator, uint32 funcId)
        : CGFunc(mod, c, f, b, memPool, stackMp, mallocator, funcId),
          calleeSavedRegs(mallocator.Adapter()),
          proEpilogSavedRegs(mallocator.Adapter()),
          phyRegOperandTable(mallocator.Adapter()),
          hashLabelOpndTable(mallocator.Adapter()),
          hashOfstOpndTable(mallocator.Adapter()),
          hashMemOpndTable(mallocator.Adapter()),
          memOpndsRequiringOffsetAdjustment(mallocator.Adapter()),
          memOpndsForStkPassedArguments(mallocator.Adapter()),
          immOpndsRequiringOffsetAdjustment(mallocator.Adapter()),
          immOpndsRequiringOffsetAdjustmentForRefloc(mallocator.Adapter()),
          hashCCTable(mallocator.Adapter())
    {
        CGFunc::SetMemlayout(*memPool.New<AArch64MemLayout>(b, f, mallocator));
        CGFunc::GetMemlayout()->SetCurrFunction(*this);
        CGFunc::SetTargetRegInfo(*memPool.New<AArch64RegInfo>(mallocator));
        CGFunc::GetTargetRegInfo()->SetCurrFunction(*this);
        if (f.GetAttr(FUNCATTR_varargs) || f.HasVlaOrAlloca()) {
            SetHasVLAOrAlloca(true);
        }
        SetHasAlloca(f.HasVlaOrAlloca());
        SetUseFP(true);
    }

    ~AArch64CGFunc() override = default;

    MOperator PickMovBetweenRegs(PrimType destType, PrimType srcType) const;

    regno_t NewVRflag() override
    {
        DEBUG_ASSERT(GetMaxRegNum() > kRFLAG, "CG internal error.");
        constexpr uint8 size = 4;
        if (GetMaxRegNum() <= kRFLAG) {
            IncMaxRegNum(kRFLAG + kVRegisterNumber);
            vReg.VRegTableResize(GetMaxRegNum());
        }
        vReg.VRegTableValuesSet(kRFLAG, kRegTyCc, size);
        return kRFLAG;
    }

    CCImpl *GetOrCreateLocator(CallConvKind cc);
    RegOperand &GetOrCreateResOperand(const BaseNode &parent, PrimType primType);

    // struct for delayed phy regs copy in param list
    struct RegMapForPhyRegCpy {
        RegMapForPhyRegCpy(RegOperand *dReg, PrimType dType, RegOperand *sReg, PrimType sType)
            : destReg(dReg), destType(dType), srcReg(sReg), srcType(sType)
        {
        }
        RegOperand *destReg;
        PrimType destType;
        RegOperand *srcReg;
        PrimType srcType;
    };

    void MergeReturn() override;
    void SelectDassign(DassignNode &stmt, Operand &opnd0) override;
    void SelectRegassign(RegassignNode &stmt, Operand &opnd0) override;
    void SelectIassign(IassignNode &stmt) override;
    void SelectReturn(Operand *opnd0) override;
    bool DoCallerEnsureValidParm(RegOperand &destOpnd, RegOperand &srcOpnd, PrimType formalPType);
    void SelectParmListPassByStack(const MIRType &mirType, Operand &opnd, uint32 memOffset, bool preCopyed,
                                   std::vector<Insn *> &insnForStackArgs);
    void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) override;
    void SelectCondGoto(LabelOperand &targetOpnd, Opcode jmpOp, Opcode cmpOp, Operand &opnd0, Operand &opnd1,
                        PrimType primType, bool signedCond);
    void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) override;
    void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) override;
    void SelectGoto(GotoNode &stmt) override;
    void SelectCall(CallNode &callNode) override;
    void SelectIcall(IcallNode &icallNode) override;
    void SelectIntrinsicCall(IntrinsiccallNode &intrinsicCallNode) override;
    Operand *SelectCclz(IntrinsicopNode &intrinsicopNode) override;
    void SelectComment(CommentNode &comment) override;

    Operand *SelectDread(const BaseNode &parent, AddrofNode &expr) override;
    RegOperand *SelectRegread(RegreadNode &expr) override;

    Operand *SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset = 0,
                         PrimType finalBitFieldDestType = kPtyInvalid) override;
    Operand *SelectIntConst(const MIRIntConst &intConst, const BaseNode &parent) override;
    Operand *HandleFmovImm(PrimType stype, int64 val, MIRConst &mirConst, const BaseNode &parent);
    Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) override;
    Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) override;

    void SelectAdd(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    Operand *SelectAdd(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    Operand *SelectShift(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    Operand *SelectSub(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectSub(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    Operand *SelectBand(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectBand(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    Operand *SelectBior(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectBior(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    Operand *SelectBxor(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectBxor(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;

    void SelectBxorShift(Operand &resOpnd, Operand *o0, Operand *o1, Operand &o2, PrimType primType);
    Operand *SelectMin(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectMin(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    Operand *SelectMax(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectMax(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    void SelectFMinFMax(Operand &resOpnd, Operand &o0, Operand &o1, bool is64Bits, bool isMin);
    void SelectCmpOp(Operand &resOpnd, Operand &o0, Operand &o1, Opcode opCode, PrimType primType,
                     const BaseNode &parent);

    Operand *SelectCmpOp(CompareNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;

    void SelectAArch64Cmp(Operand &o, Operand &i, bool isIntType, uint32 dsize);
    void SelectTargetFPCmpQuiet(Operand &o0, Operand &o1, uint32 dsize);
    void SelectAArch64CSet(Operand &o, CondOperand &cond, bool is64Bits);
    void SelectAArch64CSINV(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits);
    void SelectAArch64CSINC(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits);
    void SelectShift(Operand &resOpnd, Operand &o0, Operand &o1, ShiftDirection direct, PrimType primType);
    Operand *SelectMpy(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
    void SelectMpy(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
    /* method description contains method information which is metadata for reflection. */
    MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t regNO, bool isDest, Insn &insn,
                                                   AArch64reg regNum, bool &isOutOfRange);
    void SelectAddAfterInsn(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType, bool isDest, Insn &insn);
    bool IsImmediateOffsetOutOfRange(const MemOperand &memOpnd, uint32 bitLen);
    bool IsOperandImmValid(MOperator mOp, Operand *o, uint32 opndIdx) const;
    Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
    Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
    Operand *SelectAbsSub(Insn &lastInsn, const UnaryNode &node, Operand &newOpnd0);
    Operand *SelectAbs(UnaryNode &node, Operand &opnd0) override;
    Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) override;
    Operand *SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    void SelectNeg(Operand &dest, Operand &opnd0, PrimType primType);
    void SelectMvn(Operand &dest, Operand &opnd0, PrimType primType);
    Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
    Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) override;
    Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) override;
    Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
    void SelectAArch64Select(Operand &dest, Operand &opnd0, Operand &opnd1, CondOperand &cond, bool isIntType,
                             uint32 is64bits);
    void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) override;
    RegOperand &SelectCopy(Operand &src, PrimType stype, PrimType dtype) override;
    void SelectCopy(Operand &dest, PrimType dtype, Operand &src, PrimType stype, BaseNode *baseNode = nullptr);
    void SelectCopyImm(Operand &dest, PrimType dType, ImmOperand &src, PrimType sType);
    void SelectCopyImm(Operand &dest, ImmOperand &src, PrimType dtype);
    Operand &GetOrCreateRflag() override;
    MemOperand *GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize = k64BitSize) override;
    const Operand *GetRflag() const override;
    RegOperand &GetOrCreatevaryreg();
    RegOperand &CreateRegisterOperandOfType(PrimType primType);
    RegOperand &CreateRegisterOperandOfType(RegType regType, uint32 byteLen);
    RegOperand &CreateRflagOperand();
    RegOperand &GetOrCreateSpecialRegisterOperand(PregIdx sregIdx, PrimType primType);
    void FreeSpillRegMem(regno_t vrNum);
    RegOperand &GetOrCreatePhysicalRegisterOperand(AArch64reg regNO, uint32 size, RegType type, uint32 flag = 0);
    RegOperand *CreateVirtualRegisterOperand(regno_t vRegNO, uint32 size, RegType kind, uint32 flg = 0) const;
    RegOperand &CreateVirtualRegisterOperand(regno_t vregNO) override;
    RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vregNO) override;
    const LabelOperand *GetLabelOperand(LabelIdx labIdx) const override;
    LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) override;
    LabelOperand &GetOrCreateLabelOperand(BB &bb) override;

    bool DistanceCheck(const BB &bb, LabelIdx targLabIdx, uint32 targId, uint32 maxDistance) const;

    ImmOperand &CreateImmOperand(PrimType ptyp, int64 val) override
    {
        return CreateImmOperand(val, GetPrimTypeBitSize(ptyp), IsSignedInteger(ptyp));
    }

    /* create an integer immediate operand */
    ImmOperand &CreateImmOperand(int64 val, uint32 size, bool isSigned, VaryType varyType = kNotVary,
                                 bool isFmov = false) const
    {
        return *memPool->New<ImmOperand>(val, size, isSigned, varyType, isFmov);
    }

    ImmOperand &CreateImmOperand(Operand::OperandType type, int64 val, uint32 size, bool isSigned)
    {
        return *memPool->New<ImmOperand>(type, val, size, isSigned);
    }

    ListOperand *CreateListOpnd(MapleAllocator &allocator)
    {
        return memPool->New<ListOperand>(allocator);
    }

    OfstOperand &GetOrCreateOfstOpnd(uint64 offset, uint32 size);

    OfstOperand &CreateOfstOpnd(uint64 offset, uint32 size) const
    {
        return *memPool->New<OfstOperand>(offset, size);
    }

    OfstOperand &CreateOfstOpnd(const MIRSymbol &mirSymbol, int32 relocs) const
    {
        return *memPool->New<OfstOperand>(mirSymbol, 0, relocs);
    }

    OfstOperand &CreateOfstOpnd(const MIRSymbol &mirSymbol, int64 offset, int32 relocs) const
    {
        return *memPool->New<OfstOperand>(mirSymbol, 0, offset, relocs);
    }

    StImmOperand &CreateStImmOperand(const MIRSymbol &mirSymbol, int64 offset, int32 relocs) const
    {
        return *memPool->New<StImmOperand>(mirSymbol, offset, relocs);
    }

    RegOperand &GetOrCreateFramePointerRegOperand() override
    {
        return GetOrCreateStackBaseRegOperand();
    }

    RegOperand &GetOrCreateStackBaseRegOperand() override
    {
        return GetOrCreatePhysicalRegisterOperand(RFP, GetPointerSize() * kBitsPerByte, kRegTyInt);
    }

    MemOperand &GetOrCreateMemOpnd(const MIRSymbol &symbol, int64 offset, uint32 size, bool forLocalRef = false,
                                   bool needLow12 = false, RegOperand *regOp = nullptr);

    MemOperand &HashMemOpnd(MemOperand &tMemOpnd);

    MemOperand &GetOrCreateMemOpnd(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand *base,
                                   RegOperand *index, ImmOperand *offset, const MIRSymbol *st);

    MemOperand &GetOrCreateMemOpnd(MemOperand::AArch64AddressingMode, uint32 size, RegOperand *base, RegOperand *index,
                                   int32 shift, bool isSigned = false);

    MemOperand &GetOrCreateMemOpnd(MemOperand &oldMem);

    MemOperand &CreateMemOpnd(AArch64reg reg, int64 offset, uint32 size)
    {
        RegOperand &baseOpnd = GetOrCreatePhysicalRegisterOperand(reg, GetPointerSize() * kBitsPerByte, kRegTyInt);
        return CreateMemOpnd(baseOpnd, offset, size);
    }

    MemOperand &CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size);

    MemOperand &CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size, const MIRSymbol &sym);

    MemOperand &CreateMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0);

    MemOperand *CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0);

    CondOperand &GetCondOperand(ConditionCode op) const
    {
        return ccOperands[op];
    }

    BitShiftOperand *GetLogicalShiftLeftOperand(uint32 shiftAmount, bool is64bits) const;

    BitShiftOperand &CreateBitShiftOperand(BitShiftOperand::ShiftOp op, uint32 amount, int32 bitLen) const
    {
        return *memPool->New<BitShiftOperand>(op, amount, bitLen);
    }

    ExtendShiftOperand &CreateExtendShiftOperand(ExtendShiftOperand::ExtendOp op, uint32 amount, int32 bitLen) const
    {
        return *memPool->New<ExtendShiftOperand>(op, amount, bitLen);
    }

    Operand &GetOrCreateFuncNameOpnd(const MIRSymbol &symbol) const;
    RegOperand *GetBaseReg(const SymbolAlloc &symAlloc) override;
    int32 GetBaseOffset(const SymbolAlloc &symAlloc) override;

    Operand &CreateCommentOperand(const std::string &s) const
    {
        return *memPool->New<CommentOperand>(s, *memPool);
    }

    Operand &CreateCommentOperand(const MapleString &s) const
    {
        return *memPool->New<CommentOperand>(s.c_str(), *memPool);
    }

    void AddtoCalleeSaved(regno_t reg) override
    {
        if (!UseFP() && reg == R29) {
            reg = RFP;
        }
        if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), reg) != calleeSavedRegs.end()) {
            return;
        }
        calleeSavedRegs.emplace_back(static_cast<AArch64reg>(reg));
        DEBUG_ASSERT((AArch64isa::IsGPRegister(static_cast<AArch64reg>(reg)) ||
                      AArch64isa::IsFPSIMDRegister(static_cast<AArch64reg>(reg))),
                     "Int or FP registers are expected");
        if (AArch64isa::IsGPRegister(static_cast<AArch64reg>(reg))) {
            ++numIntregToCalleeSave;
        } else {
            ++numFpregToCalleeSave;
        }
    }

    uint32 SizeOfCalleeSaved() const
    {
        /* npairs = num / 2 + num % 2 */
        uint32 nPairs = (numIntregToCalleeSave >> 1) + (numIntregToCalleeSave & 0x1);
        nPairs += (numFpregToCalleeSave >> 1) + (numFpregToCalleeSave & 0x1);
        return (nPairs * (kAarch64IntregBytelen << 1));
    }

    void NoteFPLRAddedToCalleeSavedList()
    {
        fplrAddedToCalleeSaved = true;
    }

    bool IsFPLRAddedToCalleeSavedList() const
    {
        return fplrAddedToCalleeSaved;
    }

    bool IsIntrnCallForC() const
    {
        return isIntrnCallForC;
    }

    bool UsedStpSubPairForCallFrameAllocation() const
    {
        return usedStpSubPairToAllocateCallFrame;
    }
    void SetUsedStpSubPairForCallFrameAllocation(bool val)
    {
        usedStpSubPairToAllocateCallFrame = val;
    }

    const MapleVector<AArch64reg> &GetCalleeSavedRegs() const
    {
        return calleeSavedRegs;
    }

    MemOperand &CreateStkTopOpnd(uint32 offset, uint32 size);
    MemOperand *CreateStackMemOpnd(regno_t preg, int32 offset, uint32 size);
    MemOperand *CreateMemOperand(uint32 size, RegOperand &base, ImmOperand &ofstOp, bool isVolatile,
                                 MemOperand::AArch64AddressingMode mode = MemOperand::kAddrModeBOi) const;
    MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand &base,
                                 RegOperand *index, ImmOperand *offset, const MIRSymbol *symbol) const;
    MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand &base,
                                 RegOperand &index, ImmOperand *offset, const MIRSymbol &symbol, bool noExtend);
    MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 dSize, RegOperand &base,
                                 RegOperand &indexOpnd, uint32 shift, bool isSigned = false) const;
    MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 dSize, const MIRSymbol &sym);

    /* if offset < 0, allocation; otherwise, deallocation */
    MemOperand &CreateCallFrameOperand(int32 offset, uint32 size);

    void AppendCall(const MIRSymbol &func);
    Insn &AppendCall(const MIRSymbol &func, ListOperand &srcOpnds);

    static constexpr uint32 kDwarfFpRegBegin = 64;
    static constexpr int32 kBitLenOfShift64Bits =
        6; /* for 64 bits register, shift amount is 0~63, use 6 bits to store */
    static constexpr int32 kBitLenOfShift32Bits =
        5; /* for 32 bits register, shift amount is 0~31, use 5 bits to store */
    static constexpr int32 kHighestBitOf64Bits = 63; /* 63 is highest bit of a 64 bits number */
    static constexpr int32 kHighestBitOf32Bits = 31; /* 31 is highest bit of a 32 bits number */
    static constexpr int32 k16ValidBit = 16;

    /* CFI directives related stuffs */
    Operand &CreateCfiRegOperand(uint32 reg, uint32 size) override
    {
        /*
         * DWARF for ARM Architecture (ARM IHI 0040B) 3.1 Table 1
         * Having kRinvalid=0 (see arm32_isa.h) means
         * each register gets assigned an id number one greater than
         * its physical number
         */
        if (reg < V0) {
            return *memPool->New<cfi::RegOperand>((reg - R0), size);
        } else {
            return *memPool->New<cfi::RegOperand>((reg - V0) + kDwarfFpRegBegin, size);
        }
    }

    MOperator PickStInsn(uint32 bitSize, PrimType primType) const;
    MOperator PickLdInsn(uint32 bitSize, PrimType primType) const;
    MOperator PickExtInsn(PrimType dtype, PrimType stype) const;

    bool CheckIfSplitOffsetWithAdd(const MemOperand &memOpnd, uint32 bitLen) const;
    RegOperand *GetBaseRegForSplit(uint32 baseRegNum);

    MemOperand &SplitOffsetWithAddInstruction(const MemOperand &memOpnd, uint32 bitLen,
                                              uint32 baseRegNum = AArch64reg::kRinvalid, bool isDest = false,
                                              Insn *insn = nullptr, bool forPair = false);
    ImmOperand &SplitAndGetRemained(const MemOperand &memOpnd, uint32 bitLen, RegOperand *resOpnd, int64 ofstVal,
                                    bool isDest = false, Insn *insn = nullptr, bool forPair = false);
    MemOperand &CreateReplacementMemOperand(uint32 bitLen, RegOperand &baseReg, int64 offset);

    MemOperand &LoadStructCopyBase(const MIRSymbol &symbol, int64 offset, int datasize);

    int32 GetSplitBaseOffset() const
    {
        return splitStpldpBaseOffset;
    }
    void SetSplitBaseOffset(int32 val)
    {
        splitStpldpBaseOffset = val;
    }

    Insn &CreateCommentInsn(const std::string &comment)
    {
        Insn &insn = GetInsnBuilder()->BuildInsn(abstract::MOP_comment, InsnDesc::GetAbstractId(abstract::MOP_comment));
        insn.AddOperand(CreateCommentOperand(comment));
        return insn;
    }

    Insn &CreateCommentInsn(const MapleString &comment)
    {
        Insn &insn = GetInsnBuilder()->BuildInsn(abstract::MOP_comment, InsnDesc::GetAbstractId(abstract::MOP_comment));
        insn.AddOperand(CreateCommentOperand(comment));
        return insn;
    }

    Insn &CreateCfiRestoreInsn(uint32 reg, uint32 size)
    {
        return GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_restore).AddOpndChain(CreateCfiRegOperand(reg, size));
    }

    Insn &CreateCfiOffsetInsn(uint32 reg, int64 val, uint32 size)
    {
        return GetInsnBuilder()
            ->BuildCfiInsn(cfi::OP_CFI_offset)
            .AddOpndChain(CreateCfiRegOperand(reg, size))
            .AddOpndChain(CreateCfiImmOperand(val, size));
    }
    Insn &CreateCfiDefCfaInsn(uint32 reg, int64 val, uint32 size)
    {
        return GetInsnBuilder()
            ->BuildCfiInsn(cfi::OP_CFI_def_cfa)
            .AddOpndChain(CreateCfiRegOperand(reg, size))
            .AddOpndChain(CreateCfiImmOperand(val, size));
    }

    InsnVisitor *NewInsnModifier() override
    {
        return memPool->New<AArch64InsnVisitor>(*this);
    }

    RegType GetRegisterType(regno_t reg) const override;

    uint32 MaxCondBranchDistance() override
    {
        return AArch64Abi::kMaxInstrForCondBr;
    }

    void InsertJumpPad(Insn *insn) override;

    MapleVector<AArch64reg> &GetProEpilogSavedRegs()
    {
        return proEpilogSavedRegs;
    }

    uint32 GetDefaultAlignPow() const
    {
        return alignPow;
    }

    RegOperand &GetZeroOpnd(uint32 size) override;

    bool GetStoreFP() const
    {
        return storeFP;
    }
    void SetStoreFP(bool val)
    {
        storeFP = val;
    }

    LabelIdx GetLabelInInsn(Insn &insn) override
    {
        return static_cast<LabelOperand &>(insn.GetOperand(AArch64isa::GetJumpTargetIdx(insn))).GetLabelIndex();
    }

private:
    enum RelationOperator : uint8 { kAND, kIOR, kEOR };

    enum RelationOperatorOpndPattern : uint8 { kRegReg, kRegImm };

    enum RoundType : uint8 { kCeil, kFloor, kRound, kTrunc };

    static constexpr int32 kMaxMovkLslEntries = 8;
    using MovkLslOperandArray = std::array<BitShiftOperand, kMaxMovkLslEntries>;

    MapleVector<AArch64reg> calleeSavedRegs;
    MapleVector<AArch64reg> proEpilogSavedRegs;

    MapleUnorderedMap<phyRegIdx, RegOperand *> phyRegOperandTable; /* machine register operand table */
    MapleUnorderedMap<LabelIdx, LabelOperand *> hashLabelOpndTable;
    MapleUnorderedMap<OfstRegIdx, OfstOperand *> hashOfstOpndTable;
    MapleUnorderedMap<MemOperand, MemOperand *> hashMemOpndTable;
    /*
     * Local variables, formal parameters that are passed via registers
     * need offset adjustment after callee-saved registers are known.
     */
    MapleUnorderedMap<StIdx, MemOperand *> memOpndsRequiringOffsetAdjustment;
    MapleUnorderedMap<StIdx, MemOperand *> memOpndsForStkPassedArguments;
    MapleUnorderedMap<AArch64SymbolAlloc *, ImmOperand *> immOpndsRequiringOffsetAdjustment;
    MapleUnorderedMap<AArch64SymbolAlloc *, ImmOperand *> immOpndsRequiringOffsetAdjustmentForRefloc;
    MapleUnorderedMap<CallConvKind, CCImpl *> hashCCTable;

    Operand *rcc = nullptr;
    RegOperand *vary = nullptr;
    RegOperand *fsp = nullptr; /* used to point the address of local variables and formal parameters */

    static CondOperand ccOperands[kCcLast];
    static MovkLslOperandArray movkLslOperands;
    uint32 numIntregToCalleeSave = 0;
    uint32 numFpregToCalleeSave = 0;
    bool fplrAddedToCalleeSaved = false;
    bool isIntrnCallForC = false;
    bool usedStpSubPairToAllocateCallFrame = false;
    int32 splitStpldpBaseOffset = 0;
    uint32 alignPow = 5; /* function align pow defaults to 5   i.e. 2^5*/
    bool storeFP = false;

    MOperator PickJmpInsn(Opcode brOp, Opcode cmpOp, bool isFloat, bool isSigned) const;

    PrimType GetOperandTy(bool isIntty, uint32 dsize, bool isSigned) const
    {
        DEBUG_ASSERT(!isSigned || isIntty, "");
        return (isIntty ? ((dsize == k64BitSize) ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32))
                        : ((dsize == k64BitSize) ? PTY_f64 : PTY_f32));
    }

    RegOperand &LoadIntoRegister(Operand &o, bool isIntty, uint32 dsize, bool asSigned = false)
    {
        PrimType pTy;
        if (o.GetKind() == Operand::kOpdRegister && static_cast<RegOperand &>(o).GetRegisterType() == kRegTyFloat) {
            // f128 is a vector placeholder, no use for now
            pTy = dsize == k32BitSize ? PTY_f32 : PTY_f64;
        } else {
            pTy = GetOperandTy(isIntty, dsize, asSigned);
        }
        return LoadIntoRegister(o, pTy);
    }

    RegOperand &LoadIntoRegister(Operand &o, PrimType oty)
    {
        return (o.IsRegister() ? static_cast<RegOperand &>(o) : SelectCopy(o, oty, oty));
    }

    RegOperand &LoadIntoRegister(Operand &o, PrimType dty, PrimType sty)
    {
        return (o.IsRegister() ? static_cast<RegOperand &>(o) : SelectCopy(o, sty, dty));
    }

    struct ParamDesc {
        ParamDesc(MIRType *type, BaseNode *expr, uint32 ofst = 0, bool copyed = false)
            : mirType(type), argExpr(expr), offset(ofst), preCopyed(copyed)
        {
        }
        MIRType *mirType = nullptr;
        BaseNode *argExpr = nullptr;  // expr node
        uint32 offset = 0;            // agg offset, for preCopyed struct, RSP-based offset
        bool preCopyed = false;       // for large struct, pre copyed to strack
        bool isSpecialArg = false;    // such as : tls
    };

    std::pair<MIRFunction *, MIRFuncType *> GetCalleeFunction(StmtNode &naryNode) const;
    bool SelectParmListPreprocess(StmtNode &naryNode, size_t start, std::vector<ParamDesc> &argsDesc,
                                  const MIRFunction *callee = nullptr);
    void SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds, bool isCallNative = false);
    void SelectParmListNotC(StmtNode &naryNode, ListOperand &srcOpnds);
    void SelectRem(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType, bool isSigned, bool is64Bits);
    void SelectCvtInt2Int(const BaseNode *parent, Operand *&resOpnd, Operand *opnd0, PrimType fromType,
                          PrimType toType);
    void SelectCvtFloat2Float(Operand &resOpnd, Operand &opnd0, PrimType fromType, PrimType toType);
    void SelectCvtFloat2Int(Operand &resOpnd, Operand &opnd0, PrimType itype, PrimType ftype);
    void SelectCvtInt2Float(Operand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
    Operand *SelectRelationOperator(RelationOperator operatorCode, const BinaryNode &node, Operand &opnd0,
                                    Operand &opnd1, const BaseNode &parent);
    void SelectRelationOperator(RelationOperator operatorCode, Operand &resOpnd, Operand &opnd0, Operand &opnd1,
                                PrimType primType);
    MOperator SelectRelationMop(RelationOperator operatorType, RelationOperatorOpndPattern opndPattern, bool is64Bits,
                                bool IsBitmaskImmediate, bool isBitNumLessThan16) const;
    Operand *SelectMinOrMax(bool isMin, const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    Operand *SelectRoundOperator(RoundType roundType, const TypeCvtNode &node, Operand &opnd0, const BaseNode &parent);
    void SelectCopyMemOpnd(Operand &dest, PrimType dtype, uint32 dsize, Operand &src, PrimType stype);
    void SelectCopyRegOpnd(Operand &dest, PrimType dtype, Operand::OperandType opndType, uint32 dsize, Operand &src,
                           PrimType stype);
    bool GenerateCompareWithZeroInstruction(Opcode jmpOp, Opcode cmpOp, bool is64Bits, PrimType primType,
                                            LabelOperand &targetOpnd, Operand &opnd0);
    void SelectOverFlowCall(const IntrinsiccallNode &intrnNode);

    /* Helper functions for translating complex Maple IR instructions/inrinsics */
    void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opnd0);

    MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx i) override;

    bool IsStoreMop(MOperator mOp) const;
    bool IsImmediateValueInRange(MOperator mOp, int64 immVal, bool is64Bits, bool isIntactIndexed, bool isPostIndexed,
                                 bool isPreIndexed) const;

    MemOperand *CheckAndCreateExtendMemOpnd(PrimType ptype, const BaseNode &addrExpr, int64 offset);
    MemOperand &CreateNonExtendMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset);
    void SelectParmListWrapper(StmtNode &naryNode, ListOperand &srcOpnds, bool isCallNative);
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H */
