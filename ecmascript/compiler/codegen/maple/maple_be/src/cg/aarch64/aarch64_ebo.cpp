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
            val += static_cast<int>(offset->GetSymbol()->GetStIdx().Idx());
        }
    }
    return val;
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
        CHECK_FATAL(nextInsn->GetOperandSize() >= 1, "value overflow");
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

} /* namespace maplebe */
