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

#include "x64_cgfunc.h"
#include "assembler/operand.h"

namespace maplebe {
void X64CGFunc::MergeReturn()
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectDassign(DassignNode &stmt, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectRegassign(RegassignNode &stmt, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassign(IassignNode &stmt)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectReturn(Operand *opnd)
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
void X64CGFunc::SelectIcall(IcallNode &icallNode)
{
    CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectCclz(IntrinsicopNode &intrinopNode)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
void X64CGFunc::SelectComment(CommentNode &comment)
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
Operand *X64CGFunc::SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset,
                                PrimType finalBitFieldDestType)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
}
Operand *X64CGFunc::SelectIntConst(const MIRIntConst &intConst, const BaseNode &parent)
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
void X64CGFunc::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    CHECK_FATAL(false, "NIY");
    return nullptr;
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
RegOperand &X64CGFunc::SelectCopy(Operand &src, PrimType srcType, PrimType dstType)
{
    CHECK_FATAL(false, "NIY");
    RegOperand *a;
    return *a;
}
void X64CGFunc::SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0)
{
    CHECK_FATAL(false, "NIY");
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
Operand &X64CGFunc::CreateImmOperand(PrimType primType, int64 val)
{
    CHECK_FATAL(false, "NIY");
    Operand *a;
    return *a;
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
    constexpr const int32 sizeofFplr = 2 * kX64IntregBytelen;
    // baseOffset is the offset of this symbol based on the rbp position.
    int32 baseOffset = symAlloc->GetOffset();
    MemSegmentKind sgKind = symAlloc->GetMemSegment()->GetMemSegmentKind();
    auto *memLayout = static_cast<X64MemLayout *>(this->GetMemlayout());
    if (sgKind == kMsSpillReg) {
        /* spill = -(Locals + ArgsReg + baseOffset + ReseverdSlot + kSizeOfPtr) */
        return -(static_cast<int32>(memLayout->GetSizeOfLocals()) +
            static_cast<int32>(memLayout->SizeOfArgsRegisterPassed()) + baseOffset +
            GetFunction().GetFrameReseverdSlot() + static_cast<int32>(GetPointerSize()));
    } else if (sgKind == kMsLocals) {
        /* Locals = baseOffset - (ReseverdSlot + Locals + ArgsReg) */
        return baseOffset - (GetFunction().GetFrameReseverdSlot() + static_cast<int32>(memLayout->GetSizeOfLocals()) +
            static_cast<int32>(memLayout->SizeOfArgsRegisterPassed()));
    } else if (sgKind == kMsArgsRegPassed) {
        /* ArgsReg = baseOffset - ReseverdSlot  - ArgsReg */
        return baseOffset - static_cast<int32_t>(GetFunction().GetFrameReseverdSlot()) -
            static_cast<int32_t>(memLayout->SizeOfArgsRegisterPassed());
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
                spillRegMemOperands.emplace(std::pair<regno_t, MemOperand *>(vrNum, memOpnd));
                return memOpnd;
            }
        }

        RegOperand &baseOpnd = GetOrCreateStackBaseRegOperand();
        int32 offset = GetOrCreatSpillRegLocation(vrNum, memBitSize / kBitsPerByte);
        MemOperand *memOpnd = &GetOpndBuilder()->CreateMem(baseOpnd, offset, memBitSize);
        spillRegMemOperands.emplace(std::pair<regno_t, MemOperand *>(vrNum, memOpnd));
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
        LogInfo::MapleLogger() << "DEF ";
    }
    if (regDesc->IsRegUse()) {
        LogInfo::MapleLogger() << "USE ";
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
