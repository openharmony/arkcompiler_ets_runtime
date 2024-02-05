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

#include "aarch64_ebo.h"
#include "aarch64_cg.h"
#include "mpl_logging.h"
#include "aarch64_utils.h"

namespace maplebe {
using namespace maple;
#define EBO_DUMP CG_DEBUG_FUNC(*cgFunc)

enum AArch64Ebo::ExtOpTable : uint8 { AND, SXTB, SXTH, SXTW, ZXTB, ZXTH, ZXTW, ExtTableSize };

namespace {

using PairMOperator = MOperator[2];

constexpr uint8 insPairsNum = 5;

PairMOperator extInsnPairTable[ExtTableSize][insPairsNum] = {
    /* {origMop, newMop} */
    {{MOP_wldrb, MOP_wldrb},
     {MOP_wldrsh, MOP_wldrb},
     {MOP_wldrh, MOP_wldrb},
     {MOP_xldrsw, MOP_wldrb},
     {MOP_wldr, MOP_wldrb}}, /* AND */
    {{MOP_wldrb, MOP_wldrsb},
     {MOP_wldr, MOP_wldrsb},
     {MOP_undef, MOP_undef},
     {MOP_undef, MOP_undef},
     {MOP_undef, MOP_undef}}, /* SXTB */
    {{MOP_wldrh, MOP_wldrsh},
     {MOP_wldrb, MOP_wldrb},
     {MOP_wldrsb, MOP_wldrsb},
     {MOP_wldrsh, MOP_wldrsh},
     {MOP_undef, MOP_undef}}, /* SXTH */
    {{MOP_wldrh, MOP_wldrh},
     {MOP_wldrsh, MOP_wldrsh},
     {MOP_wldrb, MOP_wldrb},
     {MOP_wldrsb, MOP_wldrsb},
     {MOP_wldr, MOP_xldrsw}}, /* SXTW */
    {{MOP_wldrb, MOP_wldrb},
     {MOP_wldrsb, MOP_wldrb},
     {MOP_undef, MOP_undef},
     {MOP_undef, MOP_undef},
     {MOP_undef, MOP_undef}}, /* ZXTB */
    {{MOP_wldrh, MOP_wldrh},
     {MOP_wldrb, MOP_wldrb},
     {MOP_wldr, MOP_wldrh},
     {MOP_undef, MOP_undef},
     {MOP_undef, MOP_undef}}, /* ZXTH */
    {{MOP_wldr, MOP_wldr},
     {MOP_wldrh, MOP_wldrh},
     {MOP_wldrb, MOP_wldrb},
     {MOP_undef, MOP_undef},
     {MOP_undef, MOP_undef}} /* ZXTW */
};

}  // anonymous namespace

MOperator AArch64Ebo::ExtLoadSwitchBitSize(MOperator lowMop) const
{
    switch (lowMop) {
        case MOP_wldrsb:
            return MOP_xldrsb;
        case MOP_wldrsh:
            return MOP_xldrsh;
        default:
            break;
    }
    return lowMop;
}

bool AArch64Ebo::IsFmov(const Insn &insn) const
{
    return ((insn.GetMachineOpcode() >= MOP_xvmovsr) && (insn.GetMachineOpcode() <= MOP_xvmovrd));
}

bool AArch64Ebo::IsAdd(const Insn &insn) const
{
    return ((insn.GetMachineOpcode() >= MOP_xaddrrr) && (insn.GetMachineOpcode() <= MOP_ssub));
}

bool AArch64Ebo::IsInvalidReg(const RegOperand &opnd) const
{
    return (opnd.GetRegisterNumber() == AArch64reg::kRinvalid);
}

bool AArch64Ebo::IsZeroRegister(const Operand &opnd) const
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const RegOperand *regOpnd = static_cast<const RegOperand *>(&opnd);
    return regOpnd->GetRegisterNumber() == RZR;
}

bool AArch64Ebo::IsConstantImmOrReg(const Operand &opnd) const
{
    if (opnd.IsConstImmediate()) {
        return true;
    }
    return IsZeroRegister(opnd);
}

bool AArch64Ebo::IsClinitCheck(const Insn &insn) const
{
    MOperator mOp = insn.GetMachineOpcode();
    return ((mOp == MOP_clinit) || (mOp == MOP_clinit_tail));
}

bool AArch64Ebo::IsDecoupleStaticOp(Insn &insn) const
{
    if (insn.GetMachineOpcode() == MOP_lazy_ldr_static) {
        Operand *opnd1 = &insn.GetOperand(kInsnSecondOpnd);
        CHECK_FATAL(opnd1 != nullptr, "opnd1 is null!");
        auto *stImmOpnd = static_cast<StImmOperand *>(opnd1);
        return StringUtils::StartsWith(stImmOpnd->GetName(), namemangler::kDecoupleStaticValueStr);
    }
    return false;
}

static bool IsYieldPoint(Insn &insn)
{
    /*
     * It is a yieldpoint if loading from a dedicated
     * register holding polling page address:
     * ldr  wzr, [RYP]
     */
    if (insn.IsLoad() && !insn.IsLoadLabel()) {
        auto mem = static_cast<MemOperand *>(insn.GetMemOpnd());
        return (mem != nullptr && mem->GetBaseRegister() != nullptr &&
                mem->GetBaseRegister()->GetRegisterNumber() == RYP);
    }
    return false;
}

/* retrun true if insn is globalneeded */
bool AArch64Ebo::IsGlobalNeeded(Insn &insn) const
{
    /* Calls may have side effects. */
    if (insn.IsCall()) {
        return true;
    }

    /* Intrinsic call should not be removed. */
    if (insn.IsSpecialIntrinsic()) {
        return true;
    }

    /* Clinit should not be removed. */
    if (IsClinitCheck(insn)) {
        return true;
    }

    /* Yieldpoints should not be removed by optimizer. */
    if (cgFunc->GetCG()->GenYieldPoint() && IsYieldPoint(insn)) {
        return true;
    }

    std::set<uint32> defRegs = insn.GetDefRegs();
    for (auto defRegNo : defRegs) {
        if (defRegNo == RZR || defRegNo == RSP || (defRegNo == RFP && CGOptions::UseFramePointer())) {
            return true;
        }
    }
    return false;
}

/* in aarch64,resOp will not be def and use in the same time */
bool AArch64Ebo::ResIsNotDefAndUse(Insn &insn) const
{
    (void)insn;
    return true;
}

/* Return true if opnd live out of bb. */
bool AArch64Ebo::LiveOutOfBB(const Operand &opnd, const BB &bb) const
{
    CHECK_FATAL(opnd.IsRegister(), "expect register here.");
    /* when optimize_level < 2, there is need to anlyze live range. */
    if (live == nullptr) {
        return false;
    }
    bool isLiveOut = false;
    if (bb.GetLiveOut()->TestBit(static_cast<RegOperand const *>(&opnd)->GetRegisterNumber())) {
        isLiveOut = true;
    }
    return isLiveOut;
}

bool AArch64Ebo::IsLastAndBranch(BB &bb, Insn &insn) const
{
    return (bb.GetLastInsn() == &insn) && insn.IsBranch();
}

bool AArch64Ebo::IsSameRedefine(BB &bb, Insn &insn, OpndInfo &opndInfo) const
{
    MOperator mOp = insn.GetMachineOpcode();
    if (!(mOp == MOP_wmovri32 || mOp == MOP_xmovri64 || mOp == MOP_wsfmovri || mOp == MOP_xdfmovri)) {
        return false;
    }
    OpndInfo *sameInfo = opndInfo.same;
    if (sameInfo == nullptr || sameInfo->insn == nullptr || sameInfo->bb != &bb ||
        sameInfo->insn->GetMachineOpcode() != mOp) {
        return false;
    }
    Insn *prevInsn = sameInfo->insn;
    if (!prevInsn->GetOperand(kInsnSecondOpnd).IsImmediate()) {
        return false;
    }
    auto &sameOpnd = static_cast<ImmOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
    auto &opnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (sameOpnd.GetValue() == opnd.GetValue()) {
        sameInfo->refCount += opndInfo.refCount;
        return true;
    }
    return false;
}

const RegOperand &AArch64Ebo::GetRegOperand(const Operand &opnd) const
{
    CHECK_FATAL(opnd.IsRegister(), "aarch64 shoud not have regShiftOp! opnd is not register!");
    const auto &res = static_cast<const RegOperand &>(opnd);
    return res;
}

/* Create infomation for local_opnd from its def insn current_insn. */
OpndInfo *AArch64Ebo::OperandInfoDef(BB &currentBB, Insn &currentInsn, Operand &localOpnd)
{
    int32 hashVal = localOpnd.IsRegister() ? -1 : ComputeOpndHash(localOpnd);
    OpndInfo *opndInfoPrev = GetOpndInfo(localOpnd, hashVal);
    OpndInfo *opndInfo = GetNewOpndInfo(currentBB, &currentInsn, localOpnd, hashVal);
    if (localOpnd.IsMemoryAccessOperand()) {
        MemOpndInfo *memInfo = static_cast<MemOpndInfo *>(opndInfo);
        MemOperand *mem = static_cast<MemOperand *>(&localOpnd);
        Operand *base = mem->GetBaseRegister();
        Operand *offset = mem->GetOffset();
        if (base != nullptr && base->IsRegister()) {
            memInfo->SetBaseInfo(*OperandInfoUse(currentBB, *base));
        }
        if (offset != nullptr && offset->IsRegister()) {
            memInfo->SetOffsetInfo(*OperandInfoUse(currentBB, *offset));
        }
    }
    opndInfo->same = opndInfoPrev;
    if ((opndInfoPrev != nullptr)) {
        opndInfoPrev->redefined = true;
        if (opndInfoPrev->bb == &currentBB) {
            opndInfoPrev->redefinedInBB = true;
            opndInfoPrev->redefinedInsn = &currentInsn;
        }
        UpdateOpndInfo(localOpnd, *opndInfoPrev, opndInfo, hashVal);
    } else {
        SetOpndInfo(localOpnd, opndInfo, hashVal);
    }
    return opndInfo;
}

void AArch64Ebo::DefineClinitSpecialRegisters(InsnInfo &insnInfo)
{
    Insn *insn = insnInfo.insn;
    CHECK_FATAL(insn != nullptr, "nullptr of currInsnInfo");
    RegOperand &phyOpnd1 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
    OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, phyOpnd1);
    opndInfo->insnInfo = &insnInfo;

    RegOperand &phyOpnd2 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(R17, k64BitSize, kRegTyInt);
    opndInfo = OperandInfoDef(*insn->GetBB(), *insn, phyOpnd2);
    opndInfo->insnInfo = &insnInfo;
}

void AArch64Ebo::BuildCallerSaveRegisters()
{
    callerSaveRegTable.clear();
    RegOperand &phyOpndR0 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt);
    RegOperand &phyOpndV0 = a64CGFunc->GetOrCreatePhysicalRegisterOperand(V0, k64BitSize, kRegTyFloat);
    callerSaveRegTable.emplace_back(&phyOpndR0);
    callerSaveRegTable.emplace_back(&phyOpndV0);
    for (uint32 i = R1; i <= R18; i++) {
        RegOperand &phyOpnd =
            a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
        callerSaveRegTable.emplace_back(&phyOpnd);
    }
    for (uint32 i = V1; i <= V7; i++) {
        RegOperand &phyOpnd =
            a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
        callerSaveRegTable.emplace_back(&phyOpnd);
    }
    for (uint32 i = V16; i <= V31; i++) {
        RegOperand &phyOpnd =
            a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
        callerSaveRegTable.emplace_back(&phyOpnd);
    }
    CHECK_FATAL(callerSaveRegTable.size() < kMaxCallerSaveReg,
                "number of elements in callerSaveRegTable must less then 45!");
}

void AArch64Ebo::DefineAsmRegisters(InsnInfo &insnInfo)
{
    Insn *insn = insnInfo.insn;
    DEBUG_ASSERT(insn->GetMachineOpcode() == MOP_asm, "insn should be a call insn.");
    ListOperand &outList =
        const_cast<ListOperand &>(static_cast<const ListOperand &>(insn->GetOperand(kAsmOutputListOpnd)));
    for (auto opnd : outList.GetOperands()) {
        OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
        opndInfo->insnInfo = &insnInfo;
    }
    ListOperand &clobberList =
        const_cast<ListOperand &>(static_cast<const ListOperand &>(insn->GetOperand(kAsmClobberListOpnd)));
    for (auto opnd : clobberList.GetOperands()) {
        OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
        opndInfo->insnInfo = &insnInfo;
    }
    ListOperand &inList =
        const_cast<ListOperand &>(static_cast<const ListOperand &>(insn->GetOperand(kAsmInputListOpnd)));
    for (auto opnd : inList.GetOperands()) {
        OperandInfoUse(*(insn->GetBB()), *opnd);
    }
}

void AArch64Ebo::DefineCallerSaveRegisters(InsnInfo &insnInfo)
{
    Insn *insn = insnInfo.insn;
    if (insn->IsAsmInsn()) {
        DefineAsmRegisters(insnInfo);
        return;
    }
    DEBUG_ASSERT(insn->IsCall() || insn->IsTailCall(), "insn should be a call insn.");
    if (CGOptions::DoIPARA()) {
        auto *targetOpnd = insn->GetCallTargetOperand();
        CHECK_FATAL(targetOpnd != nullptr, "target is null in Insn::IsCallToFunctionThatNeverReturns");
        if (targetOpnd->IsFuncNameOpnd()) {
            FuncNameOperand *target = static_cast<FuncNameOperand *>(targetOpnd);
            const MIRSymbol *funcSt = target->GetFunctionSymbol();
            DEBUG_ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
            MIRFunction *func = funcSt->GetFunction();
            if (func != nullptr && func->IsReferedRegsValid()) {
                for (auto preg : func->GetReferedRegs()) {
                    if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(preg))) {
                        continue;
                    }
                    RegOperand *opnd = &a64CGFunc->GetOrCreatePhysicalRegisterOperand(
                        static_cast<AArch64reg>(preg), k64BitSize,
                        AArch64isa::IsFPSIMDRegister(static_cast<AArch64reg>(preg)) ? kRegTyFloat : kRegTyInt);
                    OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
                    opndInfo->insnInfo = &insnInfo;
                }
                return;
            }
        }
    }
    for (auto opnd : callerSaveRegTable) {
        OpndInfo *opndInfo = OperandInfoDef(*insn->GetBB(), *insn, *opnd);
        opndInfo->insnInfo = &insnInfo;
    }
}

void AArch64Ebo::DefineReturnUseRegister(Insn &insn)
{
    if (insn.GetMachineOpcode() != MOP_xret) {
        return;
    }
    /* Define scalar callee save register and FP, LR. */
    for (uint32 i = R19; i <= R30; i++) {
        RegOperand &phyOpnd =
            a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
        OperandInfoUse(*insn.GetBB(), phyOpnd);
    }

    /* Define SP */
    RegOperand &phyOpndSP =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(RSP), k64BitSize, kRegTyInt);
    OperandInfoUse(*insn.GetBB(), phyOpndSP);

    /* Define FP callee save registers. */
    for (uint32 i = V8; i <= V15; i++) {
        RegOperand &phyOpnd =
            a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
        OperandInfoUse(*insn.GetBB(), phyOpnd);
    }
}

void AArch64Ebo::DefineCallUseSpecialRegister(Insn &insn)
{
    if (insn.GetMachineOpcode() == MOP_asm) {
        return;
    }
    AArch64reg fpRegNO = RFP;
    if (!beforeRegAlloc && cgFunc->UseFP()) {
        fpRegNO = R29;
    }
    /* Define FP, LR. */
    RegOperand &phyOpndFP = a64CGFunc->GetOrCreatePhysicalRegisterOperand(fpRegNO, k64BitSize, kRegTyInt);
    OperandInfoUse(*insn.GetBB(), phyOpndFP);
    RegOperand &phyOpndLR =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(RLR), k64BitSize, kRegTyInt);
    OperandInfoUse(*insn.GetBB(), phyOpndLR);

    /* Define SP */
    RegOperand &phyOpndSP =
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(RSP), k64BitSize, kRegTyInt);
    OperandInfoUse(*insn.GetBB(), phyOpndSP);
}

/* return true if op1 == op2 */
bool AArch64Ebo::OperandEqSpecial(const Operand &op1, const Operand &op2) const
{
    switch (op1.GetKind()) {
        case Operand::kOpdRegister: {
            const RegOperand &reg1 = static_cast<const RegOperand &>(op1);
            const RegOperand &reg2 = static_cast<const RegOperand &>(op2);
            return reg1 == reg2;
        }
        case Operand::kOpdImmediate: {
            const ImmOperand &imm1 = static_cast<const ImmOperand &>(op1);
            const ImmOperand &imm2 = static_cast<const ImmOperand &>(op2);
            return imm1 == imm2;
        }
        case Operand::kOpdOffset: {
            const OfstOperand &ofst1 = static_cast<const OfstOperand &>(op1);
            const OfstOperand &ofst2 = static_cast<const OfstOperand &>(op2);
            return ofst1 == ofst2;
        }
        case Operand::kOpdStImmediate: {
            const StImmOperand &stImm1 = static_cast<const StImmOperand &>(op1);
            const StImmOperand &stImm2 = static_cast<const StImmOperand &>(op2);
            return stImm1 == stImm2;
        }
        case Operand::kOpdMem: {
            const MemOperand &mem1 = static_cast<const MemOperand &>(op1);
            const MemOperand &mem2 = static_cast<const MemOperand &>(op2);
            if (mem1.GetAddrMode() == mem2.GetAddrMode()) {
                DEBUG_ASSERT(mem1.GetBaseRegister() != nullptr, "nullptr check");
                DEBUG_ASSERT(mem2.GetBaseRegister() != nullptr, "nullptr check");
            }
            return ((mem1.GetAddrMode() == mem2.GetAddrMode()) &&
                    OperandEqual(*(mem1.GetBaseRegister()), *(mem2.GetBaseRegister())) &&
                    OperandEqual(*(mem1.GetIndexRegister()), *(mem2.GetIndexRegister())) &&
                    OperandEqual(*(mem1.GetOffsetOperand()), *(mem2.GetOffsetOperand())) &&
                    (mem1.GetSymbol() == mem2.GetSymbol()) && (mem1.GetSize() == mem2.GetSize()));
        }
        default: {
            return false;
        }
    }
}

int32 AArch64Ebo::GetOffsetVal(const MemOperand &memOpnd) const
{
    OfstOperand *offset = memOpnd.GetOffsetImmediate();
    int32 val = 0;
    if (offset != nullptr) {
        val += static_cast<int>(offset->GetOffsetValue());

        if (offset->IsSymOffset() || offset->IsSymAndImmOffset()) {
            val += offset->GetSymbol()->GetStIdx().Idx();
        }
    }
    return val;
}

/*
 * move vreg1, #1
 * move vreg2, vreg1
 * ===>
 * move vreg1, #1
 * move vreg2, #1
 * return true if do simplify successfully.
 */
bool AArch64Ebo::DoConstProp(Insn &insn, uint32 idx, Operand &opnd)
{
    ImmOperand *src = static_cast<ImmOperand *>(&opnd);
    const InsnDesc *md = &AArch64CG::kMd[(insn.GetMachineOpcode())];
    /* avoid the invalid case "cmp wzr, #0"/"add w1, wzr, #100" */
    Operand &destOpnd = insn.GetOperand(idx);
    if (src->IsZero() && destOpnd.IsRegister() &&
        (static_cast<RegOperand &>(destOpnd).GetRegisterType() == kRegTyInt) &&
        (insn.IsStore() || insn.IsMove() || md->IsCondDef())) {
        insn.SetOperand(idx, *GetZeroOpnd(src->GetSize()));
        return true;
    }
    MOperator mopCode = insn.GetMachineOpcode();
    switch (mopCode) {
        case MOP_xmovrr:
        case MOP_wmovrr: {
            DEBUG_ASSERT(idx == kInsnSecondOpnd, "src const for move must be the second operand.");
            MOperator mOp = (mopCode == MOP_wmovrr) ? MOP_wmovri32 : MOP_xmovri64;
            uint32 targetSize = insn.GetOperand(idx).GetSize();
            if (src->GetSize() != targetSize) {
                src = static_cast<ImmOperand *>(src->Clone(*cgFunc->GetMemoryPool()));
                CHECK_FATAL(src != nullptr, "pointer result is null");
                src->SetSize(targetSize);
            }
            if (src->IsSingleInstructionMovable() && (insn.GetOperand(kInsnFirstOpnd).GetSize() == targetSize)) {
                if (EBO_DUMP) {
                    LogInfo::MapleLogger() << " Do constprop:Prop constval " << src->GetValue() << "into insn:\n";
                    insn.Dump();
                }
                insn.SetMOP(AArch64CG::kMd[mOp]);
                src = static_cast<ImmOperand *>(src->Clone(*cgFunc->GetMemoryPool()));
                CHECK_FATAL(src != nullptr, "pointer result is null");
                src->SetSize(insn.GetOperand(idx).GetSize());
                if (insn.GetOperandSize(kInsnSecondOpnd) == k32BitSize) {
                    uint64 newVal = static_cast<uint64>(src->GetValue()) & UINT32_MAX;
                    src->SetValue(static_cast<int64>(newVal));
                }
                insn.SetOperand(kInsnSecondOpnd, *src);
                if (EBO_DUMP) {
                    LogInfo::MapleLogger() << " after constprop the insn is:\n";
                    insn.Dump();
                }
                return true;
            }
            break;
        }
        case MOP_xaddrrr:
        case MOP_waddrrr:
        case MOP_xsubrrr:
        case MOP_wsubrrr: {
            if ((idx != kInsnThirdOpnd) || !src->IsInBitSize(kMaxImmVal24Bits, 0) ||
                !(src->IsInBitSize(kMaxImmVal12Bits, 0) || src->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
                return false;
            }
            Operand &result = insn.GetOperand(0);
            bool is64Bits = (result.GetSize() == k64BitSize);
            if (EBO_DUMP) {
                LogInfo::MapleLogger() << " Do constprop:Prop constval " << src->GetValue() << "into insn:\n";
                insn.Dump();
            }
            if (src->IsZero()) {
                MOperator mOp = is64Bits ? MOP_xmovrr : MOP_wmovrr;
                insn.SetMOP(AArch64CG::kMd[mOp]);
                insn.PopBackOperand();
                if (EBO_DUMP) {
                    LogInfo::MapleLogger() << " after constprop the insn is:\n";
                    insn.Dump();
                }
                return true;
            }
            insn.SetOperand(kInsnThirdOpnd, *src);
            if ((mopCode == MOP_xaddrrr) || (mopCode == MOP_waddrrr)) {
                is64Bits ? insn.SetMOP(AArch64CG::kMd[MOP_xaddrri12]) : insn.SetMOP(AArch64CG::kMd[MOP_waddrri12]);
            } else if ((mopCode == MOP_xsubrrr) || (mopCode == MOP_wsubrrr)) {
                is64Bits ? insn.SetMOP(AArch64CG::kMd[MOP_xsubrri12]) : insn.SetMOP(AArch64CG::kMd[MOP_wsubrri12]);
            }
            if (EBO_DUMP) {
                LogInfo::MapleLogger() << " after constprop the insn is:\n";
                insn.Dump();
            }
            return true;
        }
        default:
            break;
    }
    return false;
}

/* optimize csel to cset */
bool AArch64Ebo::Csel2Cset(Insn &insn, const MapleVector<Operand *> &opnds)
{
    MOperator opCode = insn.GetMachineOpcode();
    /* csel ->cset */
    if ((opCode == MOP_wcselrrrc) || (opCode == MOP_xcselrrrc)) {
        Operand *res = &insn.GetOperand(kInsnFirstOpnd);
        DEBUG_ASSERT(res != nullptr, "expect a register");
        DEBUG_ASSERT(res->IsRegister(), "expect a register");
        /* only do integers */
        RegOperand *reg = static_cast<RegOperand *>(res);
        if ((res == nullptr) || (!reg->IsOfIntClass())) {
            return false;
        }
        Operand *op0 = opnds.at(kInsnSecondOpnd);
        Operand *op1 = opnds.at(kInsnThirdOpnd);
        ImmOperand *imm0 = nullptr;
        ImmOperand *imm1 = nullptr;
        if (op0->IsImmediate()) {
            imm0 = static_cast<ImmOperand *>(op0);
        }
        if (op1->IsImmediate()) {
            imm1 = static_cast<ImmOperand *>(op1);
        }

        bool reverse =
            (imm1 != nullptr) && imm1->IsOne() && (((imm0 != nullptr) && imm0->IsZero()) || IsZeroRegister(*op0));
        if (((imm0 != nullptr) && imm0->IsOne() && (((imm1 != nullptr) && imm1->IsZero()) || IsZeroRegister(*op1))) ||
            reverse) {
            if (EBO_DUMP) {
                LogInfo::MapleLogger() << "change csel insn :\n";
                insn.Dump();
            }
            AArch64CGFunc *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
            Operand &condOperand = insn.GetOperand(kInsnFourthOpnd);
            Operand &rflag = aarFunc->GetOrCreateRflag();
            if (!reverse) {
                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
                    (opCode == MOP_xcselrrrc) ? MOP_xcsetrc : MOP_wcsetrc, *res, condOperand, rflag);
                insn.GetBB()->ReplaceInsn(insn, newInsn);
                if (EBO_DUMP) {
                    LogInfo::MapleLogger() << "to cset insn ====>\n";
                    newInsn.Dump();
                }
            } else {
                auto &cond = static_cast<CondOperand &>(condOperand);
                if (!CheckCondCode(cond)) {
                    return false;
                }
                CondOperand &reverseCond = a64CGFunc->GetCondOperand(GetReverseCond(cond));
                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(
                    (opCode == MOP_xcselrrrc) ? MOP_xcsetrc : MOP_wcsetrc, *res, reverseCond, rflag);
                insn.GetBB()->ReplaceInsn(insn, newInsn);
                if (EBO_DUMP) {
                    LogInfo::MapleLogger() << "to cset insn ====>\n";
                    newInsn.Dump();
                }
            }
            return true;
        }
    }
    return false;
}

/* Look at an expression that has a constant operand and attempt to simplify the computations. */
bool AArch64Ebo::SimplifyConstOperand(Insn &insn, const MapleVector<Operand *> &opnds,
                                      const MapleVector<OpndInfo *> &opndInfo)
{
    BB *bb = insn.GetBB();
    bool result = false;
    if (insn.GetOperandSize() <= 1) {
        return false;
    }
    DEBUG_ASSERT(opnds.size() > 1, "opnds size must greater than 1");
    Operand *op0 = opnds[kInsnSecondOpnd];
    Operand *op1 = opnds[kInsnThirdOpnd];
    Operand *res = &insn.GetOperand(kInsnFirstOpnd);
    CHECK_FATAL(res != nullptr, "null ptr check");
    uint32 opndSize = insn.GetDesc()->GetOperandSize();
    bool op0IsConstant = IsConstantImmOrReg(*op0) && !IsConstantImmOrReg(*op1);
    bool op1IsConstant = !IsConstantImmOrReg(*op0) && IsConstantImmOrReg(*op1);
    bool bothConstant = IsConstantImmOrReg(*op0) && IsConstantImmOrReg(*op1);
    ImmOperand *immOpnd = nullptr;
    Operand *op = nullptr;
    int32 idx0 = kInsnSecondOpnd;
    if (op0IsConstant) {
        // cannot convert zero reg (r30) to a immOperand
        immOpnd = IsZeroRegister(*op0) ? &a64CGFunc->CreateImmOperand(0, op0->GetSize(), false)
                                       : static_cast<ImmOperand *>(op0);
        op = op1;
        if (op->IsMemoryAccessOperand()) {
            op = &(insn.GetOperand(kInsnThirdOpnd));
        }
        idx0 = kInsnThirdOpnd;
    } else if (op1IsConstant) {
        // cannot convert zero reg (r30) to a immOperand
        immOpnd = IsZeroRegister(*op1) ? &a64CGFunc->CreateImmOperand(0, op1->GetSize(), false)
                                       : static_cast<ImmOperand *>(op1);
        op = op0;
        if (op->IsMemoryAccessOperand()) {
            op = &(insn.GetOperand(kInsnSecondOpnd));
        }
    } else if (bothConstant) {
        ImmOperand *immOpnd0 = IsZeroRegister(*op0) ? &a64CGFunc->CreateImmOperand(0, op0->GetSize(), false)
                                                    : static_cast<ImmOperand *>(op0);
        ImmOperand *immOpnd1 = IsZeroRegister(*op1) ? &a64CGFunc->CreateImmOperand(0, op1->GetSize(), false)
                                                    : static_cast<ImmOperand *>(op1);
        return SimplifyBothConst(*insn.GetBB(), insn, *immOpnd0, *immOpnd1, opndSize);
    }
    CHECK_FATAL(immOpnd != nullptr, "constant operand required!");
    CHECK_FATAL(op != nullptr, "constant operand required!");
    /* For orr insn and one of the opnd is zero
     * orr resOp, imm1, #0  |  orr resOp, #0, imm1
     * =======>
     * mov resOp, imm1 */
    if (((insn.GetMachineOpcode() == MOP_wiorrri12) || (insn.GetMachineOpcode() == MOP_xiorrri13)) &&
        immOpnd->IsZero()) {
        MOperator mOp = opndSize == k64BitSize ? MOP_xmovrr : MOP_wmovrr;
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *res, *op);
        bb->ReplaceInsn(insn, newInsn);
        return true;
    }
    /* For the imm is 0. Then replace the insn by a move insn. */
    if (((insn.GetMachineOpcode() >= MOP_xaddrrr) && (insn.GetMachineOpcode() <= MOP_sadd) && immOpnd->IsZero()) ||
        (op1IsConstant && (insn.GetMachineOpcode() >= MOP_xsubrrr) && (insn.GetMachineOpcode() <= MOP_ssub) &&
         immOpnd->IsZero())) {
        Insn &newInsn =
            cgFunc->GetInsnBuilder()->BuildInsn(opndSize == k64BitSize ? MOP_xmovrr : MOP_wmovrr, *res, *op);
        bb->ReplaceInsn(insn, newInsn);
        return true;
    }

    if ((insn.GetMachineOpcode() == MOP_xaddrrr) || (insn.GetMachineOpcode() == MOP_waddrrr)) {
        if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0)) {
            /*
             * ADD Wd|WSP, Wn|WSP, #imm{, shift} ; 32-bit general registers
             * ADD Xd|SP,  Xn|SP,  #imm{, shift} ; 64-bit general registers
             * imm : 0 ~ 4095, shift: none, LSL #0, or LSL #12
             * aarch64 assembly takes up to 24-bits, if the lower 12 bits is all 0
             */
            if (immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits)) {
                MOperator mOp = opndSize == k64BitSize ? MOP_xaddrri12 : MOP_waddrri12;
                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *res, *op, *immOpnd);
                bb->ReplaceInsn(insn, newInsn);
                result = true;
            }
        }
    }
    /* Look for the sequence which can be simpified. */
    if (result || (insn.GetMachineOpcode() == MOP_xaddrri12) || (insn.GetMachineOpcode() == MOP_waddrri12)) {
        Insn *prev = opndInfo[idx0]->insn;
        if ((prev != nullptr) &&
            ((prev->GetMachineOpcode() == MOP_xaddrri12) || (prev->GetMachineOpcode() == MOP_waddrri12))) {
            OpndInfo *prevInfo0 = opndInfo[idx0]->insnInfo->origOpnd[kInsnSecondOpnd];
            /* if prevop0 has been redefined. skip this optimiztation. */
            if (prevInfo0->redefined) {
                return result;
            }
            /* Implicit conversion */
            if (insn.GetOperand(kInsnFirstOpnd).GetSize() != insn.GetOperand(kInsnSecondOpnd).GetSize()) {
                return result;
            }
            Operand &prevOpnd0 = prev->GetOperand(kInsnSecondOpnd);
            ImmOperand &imm0 = static_cast<ImmOperand &>(prev->GetOperand(kInsnThirdOpnd));
            int64_t val = imm0.GetValue() + immOpnd->GetValue();
            ImmOperand &imm1 = a64CGFunc->CreateImmOperand(val, opndSize, imm0.IsSignedValue());
            if (imm1.IsInBitSize(kMaxImmVal24Bits, 0) &&
                (imm1.IsInBitSize(kMaxImmVal12Bits, 0) || imm1.IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
                MOperator mOp = (opndSize == k64BitSize ? MOP_xaddrri12 : MOP_waddrri12);
                bb->ReplaceInsn(insn, cgFunc->GetInsnBuilder()->BuildInsn(mOp, *res, prevOpnd0, imm1));
                result = true;
            }
        }
    }
    return result;
}

ConditionCode AArch64Ebo::GetReverseCond(const CondOperand &cond) const
{
    switch (cond.GetCode()) {
        case CC_NE:
            return CC_EQ;
        case CC_EQ:
            return CC_NE;
        case CC_LT:
            return CC_GE;
        case CC_GE:
            return CC_LT;
        case CC_GT:
            return CC_LE;
        case CC_LE:
            return CC_GT;
        default:
            CHECK_FATAL(0, "Not support yet.");
    }
    return kCcLast;
}

/* return true if cond == CC_LE */
bool AArch64Ebo::CheckCondCode(const CondOperand &cond) const
{
    switch (cond.GetCode()) {
        case CC_NE:
        case CC_EQ:
        case CC_LT:
        case CC_GE:
        case CC_GT:
        case CC_LE:
            return true;
        default:
            return false;
    }
}

bool AArch64Ebo::SimplifyBothConst(BB &bb, Insn &insn, const ImmOperand &immOperand0, const ImmOperand &immOperand1,
                                   uint32 opndSize) const
{
    MOperator mOp = insn.GetMachineOpcode();
    int64 val = 0;
    /* do not support negative const simplify yet */
    if (immOperand0.GetValue() < 0 || immOperand1.GetValue() < 0) {
        return false;
    }
    uint64 opndValue0 = static_cast<uint64>(immOperand0.GetValue());
    uint64 opndValue1 = static_cast<uint64>(immOperand1.GetValue());
    switch (mOp) {
        case MOP_weorrri12:
        case MOP_weorrrr:
        case MOP_xeorrri13:
        case MOP_xeorrrr:
            val = static_cast<int64>(opndValue0 ^ opndValue1);
            break;
        case MOP_wandrri12:
        case MOP_waddrri24:
        case MOP_wandrrr:
        case MOP_xandrri13:
        case MOP_xandrrr:
            val = static_cast<int64>(opndValue0 & opndValue1);
            break;
        case MOP_wiorrri12:
        case MOP_wiorrrr:
        case MOP_xiorrri13:
        case MOP_xiorrrr:
            val = static_cast<int64>(opndValue0 | opndValue1);
            break;
        default:
            return false;
    }
    Operand *res = &insn.GetOperand(kInsnFirstOpnd);
    ImmOperand *immOperand = &a64CGFunc->CreateImmOperand(val, opndSize, false);
    if (!immOperand->IsSingleInstructionMovable()) {
        DEBUG_ASSERT(res->IsRegister(), " expect a register operand");
        static_cast<AArch64CGFunc *>(cgFunc)->SplitMovImmOpndInstruction(val, *(static_cast<RegOperand *>(res)), &insn);
        bb.RemoveInsn(insn);
    } else {
        MOperator newmOp = opndSize == k64BitSize ? MOP_xmovri64 : MOP_wmovri32;
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newmOp, *res, *immOperand);
        if (!VERIFY_INSN(&newInsn)) {  // insn needs to be split, so we do not implement the opt.
            return false;
        }
        bb.ReplaceInsn(insn, newInsn);
    }
    return true;
}

bool AArch64Ebo::OperandLiveAfterInsn(const RegOperand &regOpnd, Insn &insn) const
{
    for (Insn *nextInsn = insn.GetNext(); nextInsn != nullptr; nextInsn = nextInsn->GetNext()) {
        if (!nextInsn->IsMachineInstruction()) {
            continue;
        }
        int32 lastOpndId = static_cast<int32>(nextInsn->GetOperandSize() - 1);
        for (int32 i = lastOpndId; i >= 0; --i) {
            Operand &opnd = nextInsn->GetOperand(static_cast<uint32>(i));
            if (opnd.IsMemoryAccessOperand()) {
                auto &mem = static_cast<MemOperand &>(opnd);
                Operand *base = mem.GetBaseRegister();
                Operand *offset = mem.GetOffset();

                if (base != nullptr && base->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(base);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return true;
                    }
                }
                if (offset != nullptr && offset->IsRegister()) {
                    auto *tmpRegOpnd = static_cast<RegOperand *>(offset);
                    if (tmpRegOpnd->GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
                        return true;
                    }
                }
            }

            if (!opnd.IsRegister()) {
                continue;
            }
            auto &tmpRegOpnd = static_cast<RegOperand &>(opnd);
            if (tmpRegOpnd.GetRegisterNumber() != regOpnd.GetRegisterNumber()) {
                continue;
            }
            auto *regProp = nextInsn->GetDesc()->opndMD[static_cast<uint32>(i)];
            bool isUse = regProp->IsUse();
            /* if noUse Redefined, no need to check live-out. */
            return isUse;
        }
    }
    return LiveOutOfBB(regOpnd, *insn.GetBB());
}

bool AArch64Ebo::ValidPatternForCombineExtAndLoad(OpndInfo *prevOpndInfo, Insn *insn, MOperator newMop,
                                                  MOperator oldMop, const RegOperand &opnd)
{
    if (newMop == oldMop) {
        return true;
    }
    if (prevOpndInfo == nullptr || prevOpndInfo->refCount > 1) {
        return false;
    }
    if (OperandLiveAfterInsn(opnd, *insn)) {
        return false;
    }
    Insn *prevInsn = prevOpndInfo->insn;
    MemOperand *memOpnd = static_cast<MemOperand *>(prevInsn->GetMemOpnd());
    DEBUG_ASSERT(!prevInsn->IsStorePair(), "do not do this opt for str pair");
    DEBUG_ASSERT(!prevInsn->IsLoadPair(), "do not do this opt for ldr pair");
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeBOi &&
        !a64CGFunc->IsOperandImmValid(newMop, prevInsn->GetMemOpnd(), kInsnSecondOpnd)) {
        return false;
    }
    uint32 shiftAmount = memOpnd->ShiftAmount();
    if (shiftAmount == 0) {
        return true;
    }
    const InsnDesc *md = &AArch64CG::kMd[newMop];
    uint32 memSize = md->GetOperandSize() / k8BitSize;
    uint32 validShiftAmount = memSize == 8 ? 3 : memSize == 4 ? 2 : memSize == 2 ? 1 : 0;
    if (shiftAmount != validShiftAmount) {
        return false;
    }
    return true;
}

bool AArch64Ebo::CombineExtensionAndLoad(Insn *insn, const MapleVector<OpndInfo *> &origInfos, ExtOpTable idx,
                                         bool is64bits)
{
    if (!beforeRegAlloc) {
        return false;
    }
    OpndInfo *opndInfo = origInfos[kInsnSecondOpnd];
    if (opndInfo == nullptr) {
        return false;
    }
    Insn *prevInsn = opndInfo->insn;
    if (prevInsn == nullptr) {
        return false;
    }

    MOperator prevMop = prevInsn->GetMachineOpcode();
    DEBUG_ASSERT(prevMop != MOP_undef, "Invalid opcode of instruction!");
    PairMOperator *begin = &extInsnPairTable[idx][0];
    PairMOperator *end = &extInsnPairTable[idx][insPairsNum];
    auto pairIt = std::find_if(begin, end, [prevMop](const PairMOperator insPair) { return prevMop == insPair[0]; });
    if (pairIt == end) {
        return false;
    }

    auto &res = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
    OpndInfo *prevOpndInfo = GetOpndInfo(res, -1);
    MOperator newPreMop = (*pairIt)[1];
    DEBUG_ASSERT(newPreMop != MOP_undef, "Invalid opcode of instruction!");
    if (!ValidPatternForCombineExtAndLoad(prevOpndInfo, insn, newPreMop, prevMop, res)) {
        return false;
    }
    auto *newMemOp = GetOrCreateMemOperandForNewMOP(*cgFunc, *prevInsn, newPreMop);
    if (newMemOp == nullptr) {
        return false;
    }
    prevInsn->SetMemOpnd(newMemOp);
    if (is64bits && idx <= SXTW && idx >= SXTB) {
        newPreMop = ExtLoadSwitchBitSize(newPreMop);
        auto &prevDstOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
        prevDstOpnd.SetSize(k64BitSize);
        prevDstOpnd.SetValidBitsNum(k64BitSize);
    }
    prevInsn->SetMOP(AArch64CG::kMd[newPreMop]);
    MOperator movOp = is64bits ? MOP_xmovrr : MOP_wmovrr;
    if (insn->GetMachineOpcode() == MOP_wandrri12 || insn->GetMachineOpcode() == MOP_xandrri13) {
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(movOp, insn->GetOperand(kInsnFirstOpnd),
                                                            insn->GetOperand(kInsnSecondOpnd));
        insn->GetBB()->ReplaceInsn(*insn, newInsn);
    } else {
        insn->SetMOP(AArch64CG::kMd[movOp]);
    }
    return true;
}

bool AArch64Ebo::CombineMultiplyAdd(Insn *insn, const Insn *prevInsn, InsnInfo *insnInfo, Operand *addOpnd,
                                    bool is64bits, bool isFp) const
{
    /* don't use register if it was redefined. */
    OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
    OpndInfo *opndInfo2 = insnInfo->origOpnd[kInsnThirdOpnd];
    if (((opndInfo1 != nullptr) && opndInfo1->redefined) || ((opndInfo2 != nullptr) && opndInfo2->redefined)) {
        return false;
    }
    Operand &res = insn->GetOperand(kInsnFirstOpnd);
    Operand &opnd1 = prevInsn->GetOperand(kInsnSecondOpnd);
    Operand &opnd2 = prevInsn->GetOperand(kInsnThirdOpnd);
    /* may overflow */
    if ((prevInsn->GetOperand(kInsnFirstOpnd).GetSize() == k32BitSize) && is64bits) {
        return false;
    }
    MOperator mOp = isFp ? (is64bits ? MOP_dmadd : MOP_smadd) : (is64bits ? MOP_xmaddrrrr : MOP_wmaddrrrr);
    insn->GetBB()->ReplaceInsn(*insn, cgFunc->GetInsnBuilder()->BuildInsn(mOp, res, opnd1, opnd2, *addOpnd));
    return true;
}

bool AArch64Ebo::CheckCanDoMadd(Insn *insn, OpndInfo *opndInfo, int32 pos, bool is64bits, bool isFp)
{
    if ((opndInfo == nullptr) || (opndInfo->insn == nullptr)) {
        return false;
    }
    if (!cgFunc->GetMirModule().IsCModule()) {
        return false;
    }
    Insn *insn1 = opndInfo->insn;
    InsnInfo *insnInfo = opndInfo->insnInfo;
    if (insnInfo == nullptr) {
        return false;
    }
    Operand &addOpnd = insn->GetOperand(static_cast<uint32>(pos));
    MOperator opc1 = insn1->GetMachineOpcode();
    if ((isFp && ((opc1 == MOP_xvmuld) || (opc1 == MOP_xvmuls))) ||
        (!isFp && ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)))) {
        return CombineMultiplyAdd(insn, insn1, insnInfo, &addOpnd, is64bits, isFp);
    }
    return false;
}

bool AArch64Ebo::CombineMultiplySub(Insn *insn, OpndInfo *opndInfo, bool is64bits, bool isFp) const
{
    if ((opndInfo == nullptr) || (opndInfo->insn == nullptr)) {
        return false;
    }
    if (!cgFunc->GetMirModule().IsCModule()) {
        return false;
    }
    Insn *insn1 = opndInfo->insn;
    InsnInfo *insnInfo = opndInfo->insnInfo;
    if (insnInfo == nullptr) {
        return false;
    }
    Operand &subOpnd = insn->GetOperand(kInsnSecondOpnd);
    MOperator opc1 = insn1->GetMachineOpcode();
    if ((isFp && ((opc1 == MOP_xvmuld) || (opc1 == MOP_xvmuls))) ||
        (!isFp && ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)))) {
        /* don't use register if it was redefined. */
        OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
        OpndInfo *opndInfo2 = insnInfo->origOpnd[kInsnThirdOpnd];
        if (((opndInfo1 != nullptr) && opndInfo1->redefined) || ((opndInfo2 != nullptr) && opndInfo2->redefined)) {
            return false;
        }
        Operand &res = insn->GetOperand(kInsnFirstOpnd);
        Operand &opnd1 = insn1->GetOperand(kInsnSecondOpnd);
        Operand &opnd2 = insn1->GetOperand(kInsnThirdOpnd);
        /* may overflow */
        if ((insn1->GetOperand(kInsnFirstOpnd).GetSize() == k32BitSize) && is64bits) {
            return false;
        }
        MOperator mOp = isFp ? (is64bits ? MOP_dmsub : MOP_smsub) : (is64bits ? MOP_xmsubrrrr : MOP_wmsubrrrr);
        insn->GetBB()->ReplaceInsn(*insn, cgFunc->GetInsnBuilder()->BuildInsn(mOp, res, opnd1, opnd2, subOpnd));
        return true;
    }
    return false;
}

bool CheckInsnRefField(const Insn &insn, size_t opndIndex)
{
    if (insn.IsAccessRefField() && insn.AccessMem()) {
        Operand &opnd0 = insn.GetOperand(opndIndex);
        if (opnd0.IsRegister()) {
            return true;
        }
    }
    return false;
}

bool AArch64Ebo::CombineMultiplyNeg(Insn *insn, OpndInfo *opndInfo, bool is64bits, bool isFp) const
{
    if ((opndInfo == nullptr) || (opndInfo->insn == nullptr)) {
        return false;
    }
    if (!cgFunc->GetMirModule().IsCModule()) {
        return false;
    }
    Operand &res = insn->GetOperand(kInsnFirstOpnd);
    Operand &src = insn->GetOperand(kInsnSecondOpnd);
    if (res.GetSize() != src.GetSize()) {
        return false;
    }
    Insn *insn1 = opndInfo->insn;
    InsnInfo *insnInfo = opndInfo->insnInfo;
    CHECK_NULL_FATAL(insnInfo);
    MOperator opc1 = insn1->GetMachineOpcode();
    if ((isFp && ((opc1 == MOP_xvmuld) || (opc1 == MOP_xvmuls))) ||
        (!isFp && ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)))) {
        /* don't use register if it was redefined. */
        OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
        OpndInfo *opndInfo2 = insnInfo->origOpnd[kInsnThirdOpnd];
        if (((opndInfo1 != nullptr) && opndInfo1->redefined) || ((opndInfo2 != nullptr) && opndInfo2->redefined)) {
            return false;
        }
        Operand &opnd1 = insn1->GetOperand(kInsnSecondOpnd);
        Operand &opnd2 = insn1->GetOperand(kInsnThirdOpnd);
        MOperator mOp = isFp ? (is64bits ? MOP_dnmul : MOP_snmul) : (is64bits ? MOP_xmnegrrr : MOP_wmnegrrr);
        insn->GetBB()->ReplaceInsn(*insn, cgFunc->GetInsnBuilder()->BuildInsn(mOp, res, opnd1, opnd2));
        return true;
    }
    return false;
}

bool AArch64Ebo::CombineLsrAnd(Insn &insn, const OpndInfo &opndInfo, bool is64bits, bool isFp) const
{
    if (opndInfo.insn == nullptr) {
        return false;
    }
    if (!cgFunc->GetMirModule().IsCModule()) {
        return false;
    }
    AArch64CGFunc *aarchFunc = static_cast<AArch64CGFunc *>(cgFunc);
    Insn *prevInsn = opndInfo.insn;
    InsnInfo *insnInfo = opndInfo.insnInfo;
    if (insnInfo == nullptr) {
        return false;
    }
    CHECK_NULL_FATAL(insnInfo);
    MOperator opc1 = prevInsn->GetMachineOpcode();
    if (!isFp && ((opc1 == MOP_xlsrrri6) || (opc1 == MOP_wlsrrri5))) {
        /* don't use register if it was redefined. */
        OpndInfo *opndInfo1 = insnInfo->origOpnd[kInsnSecondOpnd];
        if ((opndInfo1 != nullptr) && opndInfo1->redefined) {
            return false;
        }
        Operand &res = insn.GetOperand(kInsnFirstOpnd);
        Operand &opnd1 = prevInsn->GetOperand(kInsnSecondOpnd);
        int64 immVal1 = static_cast<ImmOperand &>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
        Operand &immOpnd1 = is64bits ? aarchFunc->CreateImmOperand(immVal1, kMaxImmVal6Bits, false)
                                     : aarchFunc->CreateImmOperand(immVal1, kMaxImmVal5Bits, false);
        int64 immVal2 = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
        int64 immV2 = __builtin_ffsll(immVal2 + 1) - 1;
        if (immVal1 + immV2 < k1BitSize || (is64bits && immVal1 + immV2 > k64BitSize) ||
            (!is64bits && immVal1 + immV2 > k32BitSize)) {
            return false;
        }
        Operand &immOpnd2 = is64bits ? aarchFunc->CreateImmOperand(immV2, kMaxImmVal6Bits, false)
                                     : aarchFunc->CreateImmOperand(immV2, kMaxImmVal5Bits, false);
        MOperator mOp = (is64bits ? MOP_xubfxrri6i6 : MOP_wubfxrri5i5);
        insn.GetBB()->ReplaceInsn(insn, cgFunc->GetInsnBuilder()->BuildInsn(mOp, res, opnd1, immOpnd1, immOpnd2));
        return true;
    }
    return false;
}

/* Do some special pattern */
bool AArch64Ebo::SpecialSequence(Insn &insn, const MapleVector<OpndInfo *> &origInfos)
{
    MOperator opCode = insn.GetMachineOpcode();
    AArch64CGFunc *aarchFunc = static_cast<AArch64CGFunc *>(cgFunc);
    switch (opCode) {
        /*
         * mov R503, R0
         * mov R0, R503
         *  ==> mov R0, R0
         */
        case MOP_wmovrr:
        case MOP_xmovrr: {
            OpndInfo *opndInfo = origInfos[kInsnSecondOpnd];
            if (opndInfo == nullptr) {
                return false;
            }
            Insn *prevInsn = opndInfo->insn;
            if ((prevInsn != nullptr) && (prevInsn->GetMachineOpcode() == opCode) &&
                (prevInsn == insn.GetPreviousMachineInsn()) &&
                !RegistersIdentical(prevInsn->GetOperand(kInsnFirstOpnd), prevInsn->GetOperand(kInsnSecondOpnd)) &&
                !RegistersIdentical(insn.GetOperand(kInsnFirstOpnd), insn.GetOperand(kInsnSecondOpnd))) {
                Operand &reg1 = insn.GetOperand(kInsnFirstOpnd);
                Operand &reg2 = prevInsn->GetOperand(kInsnSecondOpnd);
                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(insn.GetMachineOpcode(), reg1, reg2);
                insn.GetBB()->ReplaceInsn(insn, newInsn);
                return true;
            }
            break;
        }
        /*
         * Extension elimination.  Look for load extension pair.  There are two cases.
         * 1) extension size == load size -> change the load type or eliminate the extension
         * 2) extension size >  load size -> possibly eliminating the extension
         *
         * Example of 1)
         *  ldrb x1, []      or  ldrb x1, []      or   ldrsb x1, []      or   ldrsb x1, []
         *  sxtb x1, x1          zxtb x1, x1           sxtb  x1, x1           zxtb  x1, x1
         * ===> ldrsb x1, []     ===> ldrb x1, []      ===> ldrsb x1, []      ===> ldrb x1, []
         *      mov   x1, x1          mov  x1, x1           mov   x1, x1           mov  x1, x1
         *
         * Example of 2)
         *  ldrb x1, []      or  ldrb x1, []   or   ldrsb x1, []     or   ldrsb x1, []
         *  sxth x1, x1          zxth x1, x1        sxth  x1, x1          zxth  x1, x1
         * ===> ldrb x1, []     ===> ldrb x1, []   ===> ldrsb x1, []     ===> no change
         *      mov x1, x1           mov  x1, x1        mov   x1, x1
         */
        case MOP_wandrri12: {
            bool doAndOpt = false;
            if (static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue() == 0xff) {
                doAndOpt = CombineExtensionAndLoad(&insn, origInfos, AND, false);
            }
            if (doAndOpt) {
                return doAndOpt;
            }
            /*
             *  lsr     d0, d1, #6
             *  and     d0, d0, #1
             * ===> ubfx d0, d1, #6, #1
             */
            int64 immValue = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
            if (!beforeRegAlloc && immValue != 0 &&
                (static_cast<uint64>(immValue) & (static_cast<uint64>(immValue) + 1)) == 0) {
                /* immValue is (1 << n - 1) */
                OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
                return CombineLsrAnd(insn, *opndInfo, false, false);
            }
            break;
        }
        case MOP_xandrri13: {
            bool doAndOpt = false;
            if (static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue() == 0xff) {
                doAndOpt = CombineExtensionAndLoad(&insn, origInfos, AND, true);
            }
            if (doAndOpt) {
                return doAndOpt;
            }
            /*
             *  lsr     d0, d1, #6
             *  and     d0, d0, #1
             * ===> ubfx d0, d1, #6, #1
             */
            int64 immValue = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
            if (!beforeRegAlloc && immValue != 0 &&
                (static_cast<uint64>(immValue) & (static_cast<uint64>(immValue) + 1)) == 0) {
                /* immValue is (1 << n - 1) */
                OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
                return CombineLsrAnd(insn, *opndInfo, true, false);
            }
            break;
        }
        case MOP_xsxtb32:
            return CombineExtensionAndLoad(&insn, origInfos, SXTB, false);
        case MOP_xsxtb64:
            return CombineExtensionAndLoad(&insn, origInfos, SXTB, true);
        case MOP_xsxth32:
            return CombineExtensionAndLoad(&insn, origInfos, SXTH, false);
        case MOP_xsxth64:
            return CombineExtensionAndLoad(&insn, origInfos, SXTH, true);
        case MOP_xsxtw64:
            return CombineExtensionAndLoad(&insn, origInfos, SXTW, true);
        case MOP_xuxtb32:
            return CombineExtensionAndLoad(&insn, origInfos, ZXTB, false);
        case MOP_xuxth32:
            return CombineExtensionAndLoad(&insn, origInfos, ZXTH, false);
        case MOP_xuxtw64:
            return CombineExtensionAndLoad(&insn, origInfos, ZXTW, true);
        /*
         *  lsl     x1, x1, #3
         *  add     x0, x0, x1
         * ===> add x0, x0, x1, 3
         *
         *  mul     x1, x1, x2
         *  add     x0, x0, x1     or   add   x0, x1, x0
         * ===> madd x0, x1, x2, x0
         */
        case MOP_xaddrrr:
        case MOP_waddrrr: {
            bool is64bits = (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize);
            OpndInfo *opndInfo = origInfos.at(kInsnThirdOpnd);
            if ((opndInfo != nullptr) && (opndInfo->insn != nullptr)) {
                Insn *insn1 = opndInfo->insn;
                InsnInfo *insnInfo1 = opndInfo->insnInfo;
                if (insnInfo1 == nullptr) {
                    return false;
                }
                Operand &op0 = insn.GetOperand(kInsnSecondOpnd);
                MOperator opc1 = insn1->GetMachineOpcode();
                if ((opc1 == MOP_xlslrri6) || (opc1 == MOP_wlslrri5)) {
                    /* don't use register if it was redefined. */
                    if (cgFunc->GetMirModule().IsCModule()) {
                        /* global opt will do this pattern when is CMoudle */
                        return false;
                    }
                    OpndInfo *opndInfo1 = insnInfo1->origOpnd[kInsnSecondOpnd];
                    if ((opndInfo1 != nullptr) && opndInfo1->redefined) {
                        return false;
                    }
                    Operand &res = insn.GetOperand(kInsnFirstOpnd);
                    Operand &opnd1 = insn1->GetOperand(kInsnSecondOpnd);
                    auto &immOpnd = static_cast<ImmOperand &>(insn1->GetOperand(kInsnThirdOpnd));
                    uint32 xLslrriBitLen = 6;
                    uint32 wLslrriBitLen = 5;
                    Operand &shiftOpnd = aarchFunc->CreateBitShiftOperand(
                        BitShiftOperand::kLSL, static_cast<uint32>(immOpnd.GetValue()),
                        static_cast<int32>((opCode == MOP_xlslrri6) ? xLslrriBitLen : wLslrriBitLen));
                    MOperator mOp = (is64bits ? MOP_xaddrrrs : MOP_waddrrrs);
                    insn.GetBB()->ReplaceInsn(insn,
                                              cgFunc->GetInsnBuilder()->BuildInsn(mOp, res, op0, opnd1, shiftOpnd));
                    return true;
                } else if ((opc1 == MOP_xmulrrr) || (opc1 == MOP_wmulrrr)) {
                    return CombineMultiplyAdd(&insn, insn1, insnInfo1, &op0, is64bits, false);
                }
            }
            opndInfo = origInfos.at(kInsnSecondOpnd);
            return CheckCanDoMadd(&insn, opndInfo, kInsnThirdOpnd, is64bits, false);
        }
        /*
         *  fmul     d1, d1, d2
         *  fadd     d0, d0, d1     or   add   d0, d1, d0
         * ===> fmadd d0, d1, d2, d0
         */
        case MOP_dadd:
        case MOP_sadd: {
            if (!CGOptions::IsFastMath()) {
                return false;
            }
            bool is64bits = (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize);
            OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
            if (CheckCanDoMadd(&insn, opndInfo, kInsnThirdOpnd, is64bits, true)) {
                return true;
            }
            opndInfo = origInfos.at(kInsnThirdOpnd);
            if (CheckCanDoMadd(&insn, opndInfo, kInsnSecondOpnd, is64bits, true)) {
                return true;
            }
            break;
        }
        /*
         *  mul     x1, x1, x2
         *  sub     x0, x0, x1
         * ===> msub x0, x1, x2, x0
         */
        case MOP_xsubrrr:
        case MOP_wsubrrr: {
            bool is64bits = (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize);
            OpndInfo *opndInfo = origInfos.at(kInsnThirdOpnd);
            if (CombineMultiplySub(&insn, opndInfo, is64bits, false)) {
                return true;
            }
            break;
        }
        /*
         *  fmul     d1, d1, d2
         *  fsub     d0, d0, d1
         * ===> fmsub d0, d1, d2, d0
         */
        case MOP_dsub:
        case MOP_ssub: {
            if (!CGOptions::IsFastMath()) {
                return false;
            }
            bool is64bits = (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize);
            OpndInfo *opndInfo = origInfos.at(kInsnThirdOpnd);
            if (CombineMultiplySub(&insn, opndInfo, is64bits, true)) {
                return true;
            }
            break;
        }
        /*
         *  mul     x1, x1, x2
         *  neg     x0, x1
         * ===> mneg x0, x1, x2
         */
        case MOP_xinegrr:
        case MOP_winegrr: {
            bool is64bits = (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize);
            OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
            if (CombineMultiplyNeg(&insn, opndInfo, is64bits, false)) {
                return true;
            }
            break;
        }
        /*
         *  fmul     d1, d1, d2
         *  fneg     d0, d1
         * ===> fnmul d0, d1, d2
         */
        case MOP_wfnegrr:
        case MOP_xfnegrr: {
            if (!CGOptions::IsFastMath()) {
                return false;
            }
            bool is64bits = (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize);
            OpndInfo *opndInfo = origInfos.at(kInsnSecondOpnd);
            if (CombineMultiplyNeg(&insn, opndInfo, is64bits, true)) {
                return true;
            }
            break;
        }
        case MOP_xcsetrc:
        case MOP_wcsetrc: {
            /* i.   cmp     x0, x1
             *      cset    w0, EQ     ===> cmp x0, x1
             *      cmp     w0, #0          cset w0, EQ
             *      cset    w0, NE
             *
             * ii.  cmp     x0, x1
             *      cset    w0, EQ     ===> cmp x0, x1
             *      cmp     w0, #0          cset w0, NE
             *      cset    w0, EQ
             *
             *  a.< -1 : 0x20ff25e0 > < 0 > cmp(226) (opnd0: vreg:C105 class: [CC]) (opnd1: vreg:R104 class: [I])
             * (opnd2: vreg:R106 class: [I]) b.< -1 : 0x20ff60a0 > < 0 > cset(72) (opnd0: vreg:R101 class: [I]) (opnd1:
             * CC: EQ) c.< -1*  : 0x20ff3870 > < 0 > cmp(223) (opnd0: vreg:C105 class: [CC]) (opnd1: vreg:R101 class:
             * [I]) (opnd2: imm:0) d.< *  -1 : 0x20ff3908 > < 0 > cset(72) (opnd0: vreg:R107 class: [I]) (opnd1: CC: NE)
             *  d1.< -1 : 0x20ff3908 > < 0 > *  cset(72) (opnd0: vreg:R107 class: [I]) (opnd1: CC: EQ) i, d
             *  ===> mov R107 R101 ii, a,b,c,d1 ===> a,b,cset Rxx
             *  NE, c, mov R107 Rxx
             */
            auto &cond = static_cast<CondOperand &>(insn.GetOperand(kInsnSecondOpnd));
            if ((cond.GetCode() != CC_NE) && (cond.GetCode() != CC_EQ)) {
                return false;
            }
            bool reverse = (cond.GetCode() == CC_EQ);
            OpndInfo *condInfo = origInfos[kInsnSecondOpnd];
            if ((condInfo != nullptr) && condInfo->insn) {
                Insn *cmp1 = condInfo->insn;
                if ((cmp1->GetMachineOpcode() == MOP_xcmpri) || (cmp1->GetMachineOpcode() == MOP_wcmpri)) {
                    InsnInfo *cmpInfo1 = condInfo->insnInfo;
                    CHECK_FATAL(cmpInfo1 != nullptr, "pointor cmpInfo1 is null");
                    OpndInfo *info0 = cmpInfo1->origOpnd[kInsnSecondOpnd];
                    /* if R101 was not redefined. */
                    if ((info0 != nullptr) && (info0->insnInfo != nullptr) && (info0->insn != nullptr) &&
                        (reverse || !info0->redefined) && cmp1->GetOperand(kInsnThirdOpnd).IsImmediate()) {
                        Insn *csetInsn = info0->insn;
                        MOperator opc1 = csetInsn->GetMachineOpcode();
                        if (((opc1 == MOP_xcsetrc) || (opc1 == MOP_wcsetrc)) &&
                            static_cast<ImmOperand &>(cmp1->GetOperand(kInsnThirdOpnd)).IsZero()) {
                            CondOperand &cond1 = static_cast<CondOperand &>(csetInsn->GetOperand(kInsnSecondOpnd));
                            if (!CheckCondCode(cond1)) {
                                return false;
                            }
                            if (EBO_DUMP) {
                                LogInfo::MapleLogger()
                                    << "< === do specical condition optimization, replace insn  ===> \n";
                                insn.Dump();
                            }
                            Operand *result = &insn.GetOperand(kInsnFirstOpnd);
                            CHECK_FATAL(result != nullptr, "pointor result is null");
                            uint32 size = result->GetSize();
                            if (reverse) {
                                /* After regalloction, we can't create a new register. */
                                if (!beforeRegAlloc) {
                                    return false;
                                }
                                AArch64CGFunc *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
                                Operand &r = aarFunc->CreateRegisterOperandOfType(
                                    static_cast<RegOperand *>(result)->GetRegisterType(), size / kBitsPerByte);
                                /* after generate a new vreg, check if the size of DataInfo is big enough */
                                EnlargeSpaceForLA(*csetInsn);
                                CondOperand &cond2 = aarFunc->GetCondOperand(GetReverseCond(cond1));
                                Operand &rflag = aarFunc->GetOrCreateRflag();
                                Insn &newCset = cgFunc->GetInsnBuilder()->BuildInsn(
                                    result->GetSize() == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc, r, cond2, rflag);
                                /* new_cset use the same cond as cset_insn. */
                                IncRef(*info0->insnInfo->origOpnd[kInsnSecondOpnd]);
                                csetInsn->GetBB()->InsertInsnAfter(*csetInsn, newCset);
                                MOperator mOp = (result->GetSize() == k64BitSize ? MOP_xmovrr : MOP_wmovrr);
                                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *result, r);
                                insn.GetBB()->ReplaceInsn(insn, newInsn);
                                if (EBO_DUMP) {
                                    LogInfo::MapleLogger() << "< === with new insn ===> \n";
                                    newInsn.Dump();
                                }
                            } else {
                                Operand *result1 = &csetInsn->GetOperand(kInsnFirstOpnd);
                                MOperator mOp = ((result->GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr);
                                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *result, *result1);
                                insn.GetBB()->ReplaceInsn(insn, newInsn);
                                if (EBO_DUMP) {
                                    LogInfo::MapleLogger() << "< === with new insn ===> \n";
                                    newInsn.Dump();
                                }
                            }
                            return true;
                        }
                    }
                }
            }
        } /* end case MOP_wcsetrc */
            [[clang::fallthrough]];
        default:
            break;
    }
    return false;
}

/*
 *  *iii. mov w16, v10.s[1]   //  FMOV from simd 105   ---> replace_insn
 *      mov w1, w16     ----->insn
 *      ==>
 *      mov w1, v10.s[1]
 */
bool AArch64Ebo::IsMovToSIMDVmov(Insn &insn, const Insn &replaceInsn) const
{
    if (insn.GetMachineOpcode() == MOP_wmovrr && replaceInsn.GetMachineOpcode() == MOP_xvmovrv) {
        insn.SetMOP(AArch64CG::kMd[replaceInsn.GetMachineOpcode()]);
        return true;
    }
    return false;
}

bool AArch64Ebo::IsPseudoRet(Insn &insn) const
{
    MOperator mop = insn.GetMachineOpcode();
    if (mop == MOP_pseudo_ret_int || mop == MOP_pseudo_ret_float) {
        return true;
    }
    return false;
}

bool AArch64Ebo::ChangeLdrMop(Insn &insn, const Operand &opnd) const
{
    DEBUG_ASSERT(insn.IsLoad(), "expect insn is load in ChangeLdrMop");
    DEBUG_ASSERT(opnd.IsRegister(), "expect opnd is a register in ChangeLdrMop");

    const RegOperand *regOpnd = static_cast<const RegOperand *>(&opnd);
    if (static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterType() != regOpnd->GetRegisterType()) {
        return false;
    }

    if (static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetIndexRegister()) {
        return false;
    }

    bool bRet = true;
    if (regOpnd->GetRegisterType() == kRegTyFloat) {
        switch (insn.GetMachineOpcode()) {
            case MOP_wldrb:
                insn.SetMOP(AArch64CG::kMd[MOP_bldr]);
                break;
            case MOP_wldrh:
                insn.SetMOP(AArch64CG::kMd[MOP_hldr]);
                break;
            case MOP_wldr:
                insn.SetMOP(AArch64CG::kMd[MOP_sldr]);
                break;
            case MOP_xldr:
                insn.SetMOP(AArch64CG::kMd[MOP_dldr]);
                break;
            case MOP_wldli:
                insn.SetMOP(AArch64CG::kMd[MOP_sldli]);
                break;
            case MOP_xldli:
                insn.SetMOP(AArch64CG::kMd[MOP_dldli]);
                break;
            case MOP_wldrsb:
            case MOP_wldrsh:
            default:
                bRet = false;
                break;
        }
    } else if (regOpnd->GetRegisterType() == kRegTyInt) {
        switch (insn.GetMachineOpcode()) {
            case MOP_bldr:
                insn.SetMOP(AArch64CG::kMd[MOP_wldrb]);
                break;
            case MOP_hldr:
                insn.SetMOP(AArch64CG::kMd[MOP_wldrh]);
                break;
            case MOP_sldr:
                insn.SetMOP(AArch64CG::kMd[MOP_wldr]);
                break;
            case MOP_dldr:
                insn.SetMOP(AArch64CG::kMd[MOP_xldr]);
                break;
            case MOP_sldli:
                insn.SetMOP(AArch64CG::kMd[MOP_wldli]);
                break;
            case MOP_dldli:
                insn.SetMOP(AArch64CG::kMd[MOP_xldli]);
                break;
            default:
                bRet = false;
                break;
        }
    } else {
        DEBUG_ASSERT(false, "Internal error.");
    }
    return bRet;
}
} /* namespace maplebe */
