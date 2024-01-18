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

#if TARGAARCH64
#include "aarch64_ebo.h"
#elif TARGRISCV64
#include "riscv64_ebo.h"
#endif
#if TARGARM32
#include "arm32_ebo.h"
#endif
#include "securec.h"

#include "common_utils.h"
#include "optimize_common.h"

/*
 * The Optimizations include forward propagation, common expression elimination, constant folding,
 * dead code elimination and some target optimizations. The main entry of the optimization is run.
 * When the Optimization level is less than O2, it can only perform in single block. and in O2 it
 * can perform it a sequence of blocks.
 */
namespace maplebe {
using namespace maple;

#define EBO_DUMP CG_DEBUG_FUNC(*cgFunc)
#define EBO_DUMP_NEWPM CG_DEBUG_FUNC(f)
#define TRUE_OPND cgFunc->GetTrueOpnd()

constexpr uint32 kEboOpndHashLength = 521;
constexpr uint32 kEboMaxBBNums = 200;

/* Return the opndInfo for the first mem operand of insn. */
MemOpndInfo *Ebo::GetMemInfo(InsnInfo &insnInfo)
{
    Insn *insn = insnInfo.insn;
    CHECK_FATAL(insn != nullptr, "insnInfo.insn is nullptr!");
    CHECK_FATAL(insn->AccessMem(), "insn is not access memory!");
    uint32 opndNum = insn->GetOperandSize();
    if (insn->IsLoad()) {
        for (uint32 i = 0; i < opndNum; ++i) {
            if (insn->GetOperand(i).IsMemoryAccessOperand()) {
                return static_cast<MemOpndInfo *>(insnInfo.origOpnd[i]);
            }
        }
    } else if (insn->IsStore()) {
        int32 resId = 0;
        for (uint32 i = 0; i < opndNum; ++i) {
            if (insn->OpndIsDef(i)) {
                if (insn->GetOperand(i).IsMemoryAccessOperand()) {
                    return static_cast<MemOpndInfo *>(insnInfo.result[resId]);
                } else {
                    resId++;
                }
            }
        }
    }
    return nullptr;
}

bool Ebo::IsFrameReg(Operand &opnd) const
{
    if (!opnd.IsRegister()) {
        return false;
    }
    RegOperand &reg = static_cast<RegOperand &>(opnd);
    return cgFunc->IsFrameReg(reg);
}

Operand *Ebo::GetZeroOpnd(uint32 size) const
{
#if TARGAARCH64 || TARGRISCV64
    return size > k64BitSize ? nullptr : &cgFunc->GetZeroOpnd(size);
#else
    return nullptr;
#endif
}

bool Ebo::IsSaveReg(const Operand &opnd)
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const RegOperand &reg = static_cast<const RegOperand &>(opnd);
    return cgFunc->IsSaveReg(reg, *cgFunc->GetFunction().GetReturnType(), cgFunc->GetBecommon());
}

bool Ebo::IsPhysicalReg(const Operand &opnd) const
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const RegOperand &reg = static_cast<const RegOperand &>(opnd);
    return reg.IsPhysicalRegister();
}

bool Ebo::HasAssignedReg(const Operand &opnd) const
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const auto &reg = static_cast<const RegOperand &>(opnd);
    return reg.IsVirtualRegister() ? (!IsInvalidReg(reg)) : true;
}

bool Ebo::IsOfSameClass(const Operand &op0, const Operand &op1) const
{
    if (!op0.IsRegister() || !op1.IsRegister()) {
        return false;
    }
    const auto &reg0 = static_cast<const RegOperand &>(op0);
    const auto &reg1 = static_cast<const RegOperand &>(op1);
    return reg0.GetRegisterType() == reg1.GetRegisterType();
}

/* return true if opnd of bb is available. */
bool Ebo::OpndAvailableInBB(const BB &bb, OpndInfo *info)
{
    if (info == nullptr) {
        return false;
    }
    if (info->opnd == nullptr) {
        return false;
    }

    Operand *op = info->opnd;
    if (IsConstantImmOrReg(*op)) {
        return true;
    }

    int32 hashVal = 0;
    if (op->IsRegShift() || op->IsRegister()) {
        hashVal = -1;
    } else {
        hashVal = info->hashVal;
    }
    if (GetOpndInfo(*op, hashVal) != info) {
        return false;
    }
    /* global operands aren't supported at low levels of optimization. */
    if ((Globals::GetInstance()->GetOptimLevel() < CGOptions::kLevel2) && (&bb != info->bb)) {
        return false;
    }
    if (beforeRegAlloc && IsPhysicalReg(*op)) {
        return false;
    }
    return true;
}

bool Ebo::ForwardPropCheck(const Operand *opndReplace, const OpndInfo &opndInfo, const Operand &opnd, Insn &insn)
{
    if (opndReplace == nullptr) {
        return false;
    }
    if ((opndInfo.replacementInfo != nullptr) && opndInfo.replacementInfo->redefined) {
        return false;
    }
#if TARGARM32
    /* for arm32, disable forwardProp in strd insn. */
    if (insn.GetMachineOpcode() == MOP_strd) {
        return false;
    }
    if (opndInfo.mayReDef) {
        return false;
    }
#endif
    if (!(IsConstantImmOrReg(*opndReplace) ||
          ((OpndAvailableInBB(*insn.GetBB(), opndInfo.replacementInfo) || RegistersIdentical(opnd, *opndReplace)) &&
           (HasAssignedReg(opnd) == HasAssignedReg(*opndReplace))))) {
        return false;
    }
    /* if beforeRA, replace op should not be PhysicalRe */
    return !beforeRegAlloc || !IsPhysicalReg(*opndReplace);
}

bool Ebo::RegForwardCheck(Insn &insn, const Operand &opnd, const Operand *opndReplace, Operand &oldOpnd,
                          const OpndInfo *tmpInfo)
{
    if (IsConstantImmOrReg(opnd)) {
        return false;
    }
    if (!(!beforeRegAlloc || (HasAssignedReg(oldOpnd) == HasAssignedReg(*opndReplace)) || IsZeroRegister(opnd) ||
          !insn.IsMove())) {
        return false;
    }
    std::set<regno_t> defRegs = insn.GetDefRegs();
    if (!(defRegs.empty() ||
          ((opnd.IsRegister() && !defRegs.count(static_cast<const RegOperand &>(opnd).GetRegisterNumber())) ||
           !beforeRegAlloc))) {
        return false;
    }
    if (!(beforeRegAlloc || !IsFrameReg(oldOpnd))) {
        return false;
    }
    if (insn.GetBothDefUseOpnd() != kInsnMaxOpnd) {
        return false;
    }
    if (IsPseudoRet(insn)) {
        return false;
    }

    return ((IsOfSameClass(oldOpnd, *opndReplace) && (oldOpnd.GetSize() <= opndReplace->GetSize())) ||
            ((tmpInfo != nullptr) && IsMovToSIMDVmov(insn, *tmpInfo->insn)));
}

/* For Memory Operand, its info was stored in a hash table, this function is to compute its hash value. */
int32 Ebo::ComputeOpndHash(const Operand &opnd) const
{
    uint64 hashIdx = reinterpret_cast<uint64>(&opnd) >> k4ByteSize;
    return static_cast<int32>(hashIdx % kEboOpndHashLength);
}

/* Store the operand information. Store it to the vRegInfo if is register. otherwise put it to the hash table. */
void Ebo::SetOpndInfo(const Operand &opnd, OpndInfo *opndInfo, int32 hashVal)
{
    /* opnd is Register or RegShift */
    if (hashVal == -1) {
        const RegOperand &reg = GetRegOperand(opnd);
        vRegInfo[reg.GetRegisterNumber()] = opndInfo;
        return;
    }

    CHECK_FATAL(static_cast<uint64>(static_cast<int64>(hashVal)) < exprInfoTable.size(),
                "SetOpndInfo hashval outof range!");
    opndInfo->hashVal = hashVal;
    opndInfo->hashNext = exprInfoTable.at(hashVal);
    exprInfoTable.at(hashVal) = opndInfo;
}

/* Used to change the info of opnd from opndinfo to newinfo. */
void Ebo::UpdateOpndInfo(const Operand &opnd, OpndInfo &opndInfo, OpndInfo *newInfo, int32 hashVal)
{
    if (hashVal == -1) {
        const RegOperand &reg = GetRegOperand(opnd);
        vRegInfo[reg.GetRegisterNumber()] = newInfo;
        return;
    }
    DEBUG_ASSERT(static_cast<uint32>(hashVal) < exprInfoTable.size(), "SetOpndInfo hashval outof range!");
    OpndInfo *info = exprInfoTable.at(hashVal);
    if (newInfo != nullptr) {
        newInfo->hashNext = opndInfo.hashNext;
        opndInfo.hashNext = nullptr;
        if (info == &opndInfo) {
            exprInfoTable.at(hashVal) = newInfo;
            return;
        }
        while (info != nullptr) {
            if (info->hashNext == &opndInfo) {
                info->hashNext = newInfo;
                return;
            }
            info = info->hashNext;
        }
        return;
    }
    if (info == &opndInfo) {
        exprInfoTable.at(hashVal) = opndInfo.hashNext;
        return;
    }
    while (info != nullptr) {
        if (info->hashNext == &opndInfo) {
            info->hashNext = opndInfo.next;
            opndInfo.hashNext = nullptr;
            return;
        }
        info = info->hashNext;
    }
}

/* return true if op1 op2 is equal */
bool Ebo::OperandEqual(const Operand &op1, const Operand &op2) const
{
    if (&op1 == &op2) {
        return true;
    }
    if (op1.GetKind() != op2.GetKind()) {
        return false;
    }
    return OperandEqSpecial(op1, op2);
}

OpndInfo *Ebo::GetOpndInfo(const Operand &opnd, int32 hashVal) const
{
    if (hashVal < 0) {
        const RegOperand &reg = GetRegOperand(opnd);
        auto it = vRegInfo.find(reg.GetRegisterNumber());
        return it != vRegInfo.end() ? it->second : nullptr;
    }
    /* do not find prev memOpend */
    if (opnd.IsMemoryAccessOperand()) {
        return nullptr;
    }
    DEBUG_ASSERT(static_cast<uint32>(hashVal) < exprInfoTable.size(), "SetOpndInfo hashval outof range!");
    OpndInfo *info = exprInfoTable.at(hashVal);
    while (info != nullptr) {
        if (&opnd == info->opnd) {
            return info;
        }
        info = info->hashNext;
    }
    return nullptr;
}

/* Create a opndInfo for opnd. */
OpndInfo *Ebo::GetNewOpndInfo(BB &bb, Insn *insn, Operand &opnd, int32 hashVal)
{
    OpndInfo *opndInfo = nullptr;
    if (opnd.IsMemoryAccessOperand()) {
        opndInfo = eboMp->New<MemOpndInfo>(opnd);
    } else {
        opndInfo = eboMp->New<OpndInfo>(opnd);
    }
    /* Initialize the entry. */
    opndInfo->hashVal = hashVal;
    opndInfo->opnd = &opnd;
    opndInfo->bb = &bb;
    opndInfo->insn = insn;
    opndInfo->prev = lastOpndInfo;
    if (firstOpndInfo == nullptr) {
        firstOpndInfo = opndInfo;
    } else {
        lastOpndInfo->next = opndInfo;
    }
    lastOpndInfo = opndInfo;
    return opndInfo;
}

/* Update the use infomation for localOpnd because of its use insn currentInsn. */
OpndInfo *Ebo::OperandInfoUse(BB &currentBB, Operand &localOpnd)
{
    if (!(localOpnd.IsRegister() || localOpnd.IsRegShift()) && !localOpnd.IsMemoryAccessOperand()) {
        return nullptr;
    }
    int hashVal = 0;
    /* only arm32 has regShift */
    if (localOpnd.IsRegister() || localOpnd.IsRegShift()) {
        hashVal = -1;
    } else {
        hashVal = ComputeOpndHash(localOpnd);
    }
    OpndInfo *opndInfo = GetOpndInfo(localOpnd, hashVal);

    if (opndInfo == nullptr) {
        opndInfo = GetNewOpndInfo(currentBB, nullptr, localOpnd, hashVal);
        SetOpndInfo(localOpnd, opndInfo, hashVal);
    }
    IncRef(*opndInfo);
    return opndInfo;
}

/* return true if op0 is identical with op1 */
bool Ebo::RegistersIdentical(const Operand &op0, const Operand &op1) const
{
    if (&op0 == &op1) {
        return true;
    }
    if (!(op0.IsRegister() && op1.IsRegister())) {
        return false;
    }
    const RegOperand &reg0 = static_cast<const RegOperand &>(op0);
    const RegOperand &reg1 = static_cast<const RegOperand &>(op1);
    return ((reg0.IsPhysicalRegister() || !IsInvalidReg(reg0)) && (reg1.IsPhysicalRegister() || !IsInvalidReg(reg1)) &&
            (reg0.GetRegisterType() == reg1.GetRegisterType()) &&
            (reg0.GetRegisterNumber() == reg1.GetRegisterNumber()));
}

InsnInfo *Ebo::GetNewInsnInfo(Insn &insn)
{
    InsnInfo *insnInfo = eboMp->New<InsnInfo>(*eboMp, insn);
    insnInfo->prev = lastInsnInfo;
    if (firstInsnInfo == nullptr) {
        firstInsnInfo = insnInfo;
    } else {
        lastInsnInfo->next = insnInfo;
    }
    lastInsnInfo = insnInfo;
    insnInfo->next = nullptr;
    return insnInfo;
}

uint32 Ebo::ComputeHashVal(Insn &insn, const MapleVector<OpndInfo *> &opndInfos) const
{
    uint32 hashVal = 0;
    if (insn.AccessMem()) {
        hashVal = kEboDefaultMemHash;
        if (insn.NoAlias()) {
            hashVal = kEboNoAliasMemHash;
        }
        MemOperand *memOpnd = static_cast<MemOperand *>(insn.GetMemOpnd());
        if (memOpnd != nullptr) {
            Operand *baseReg = memOpnd->GetBaseRegister();
            if ((baseReg != nullptr) && IsFrameReg(*baseReg)) {
                hashVal = kEboSpillMemHash;
            }
        }
    } else if (Globals::GetInstance()->GetTarget()->IsEffectiveCopy(insn)) {
        hashVal = kEboCopyInsnHash;
    } else {
        uint32 opndNum = insn.GetOperandSize();
        hashVal = insn.GetMachineOpcode();
        for (uint32 i = 0; i < opndNum; ++i) {
            hashVal += static_cast<uint32>(reinterpret_cast<uintptr_t>(opndInfos.at(i)));
        }
        hashVal = static_cast<uint32>(kEboReservedInsnHash + EBO_EXP_INSN_HASH(hashVal));
    }
    return hashVal;
}

/* computeHashVal of insn */
void Ebo::HashInsn(Insn &insn, const MapleVector<OpndInfo *> &origInfo, const MapleVector<OpndInfo *> &opndInfos)
{
    uint32 hashVal = ComputeHashVal(insn, opndInfos);
    /* Create a new insnInfo entry and add the new insn to the hash table. */
    InsnInfo *insnInfo = GetNewInsnInfo(insn);
    insnInfo->bb = insn.GetBB();
    insnInfo->insn = &insn;
    insnInfo->hashIndex = hashVal;
    insnInfo->same = insnInfoTable.at(hashVal);

    if (!beforeRegAlloc) {
        if ((insn.IsCall() || insn.IsTailCall() || insn.IsAsmInsn()) && !insn.GetIsThrow()) {
            DefineCallerSaveRegisters(*insnInfo);
        } else if (IsClinitCheck(insn)) {
            DefineClinitSpecialRegisters(*insnInfo);
        }
    }
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        /* Copy all the opndInfo entries for the operands. */
        insnInfo->origOpnd.emplace_back(origInfo.at(i));
        insnInfo->optimalOpnd.emplace_back(opndInfos.at(i));
        /* Keep the result info. */
        if (!insn.OpndIsDef(i)) {
            continue;
        }
        auto genOpndInfoDef = [this, insnInfo](Operand &op) {
            OpndInfo *opndInfo = nullptr;
            if ((&op != TRUE_OPND) &&
                ((op.IsRegister() && (&op) != GetZeroOpnd(op.GetSize())) ||
                 (op.IsMemoryAccessOperand() && (static_cast<MemOperand &>(op)).GetBaseRegister() != nullptr))) {
                opndInfo = OperandInfoDef(*insnInfo->bb, *insnInfo->insn, op);
                opndInfo->insnInfo = insnInfo;
            }
            insnInfo->result.emplace_back(opndInfo);
        };
        Operand &op = insn.GetOperand(i);
        if (op.IsList() && !static_cast<ListOperand &>(op).GetOperands().empty()) {
            for (auto operand : static_cast<ListOperand &>(op).GetOperands()) {
                genOpndInfoDef(*operand);
            }
        } else {
            genOpndInfoDef(op);
        }
    }
    SetInsnInfo(hashVal, *insnInfo);
}

/* do decref of orig_info, refCount will be set to 0 */
void Ebo::RemoveUses(uint32 opndNum, const MapleVector<OpndInfo *> &origInfo)
{
    OpndInfo *info = nullptr;
    for (uint32 i = 0; i < opndNum; ++i) {
        info = origInfo.at(i);
        if (info != nullptr) {
            DecRef(*info);
            if (info->opnd->IsMemoryAccessOperand()) {
                MemOpndInfo *memInfo = static_cast<MemOpndInfo *>(info);
                OpndInfo *baseInfo = memInfo->GetBaseInfo();
                OpndInfo *offsetInfo = memInfo->GetOffsetInfo();
                if (baseInfo != nullptr) {
                    DecRef(*baseInfo);
                }
                if (offsetInfo != nullptr) {
                    DecRef(*offsetInfo);
                }
            }
        }
    }
}

OpndInfo *Ebo::BuildMemOpndInfo(BB &bb, Insn &insn, Operand &opnd, uint32 opndIndex)
{
    auto *memOpnd = static_cast<MemOperand *>(&opnd);
    Operand *base = memOpnd->GetBaseRegister();
    Operand *offset = memOpnd->GetOffset();
    OpndInfo *baseInfo = nullptr;
    OpndInfo *offsetInfo = nullptr;
    if (base != nullptr) {
        if (!memOpnd->IsIntactIndexed()) {
            baseInfo = OperandInfoUse(bb, *base);
            baseInfo = OperandInfoDef(bb, insn, *base);
            return baseInfo;
        } else {
            baseInfo = OperandInfoUse(bb, *base);
        }
        /* forward prop for base register. */
        if ((baseInfo != nullptr) && base->IsRegister()) {
            auto *baseReg = static_cast<RegOperand *>(base);
            Operand *replaceOpnd = baseInfo->replacementOpnd;
            OpndInfo *replaceInfo = baseInfo->replacementInfo;
            if ((replaceInfo != nullptr) && (replaceOpnd != nullptr) && !cgFunc->IsSPOrFP(*baseReg) &&
                (!beforeRegAlloc || (!IsPhysicalReg(*replaceOpnd) && !IsPhysicalReg(*base))) &&
                IsOfSameClass(*base, *replaceOpnd) && memOpnd->IsIntactIndexed() &&
                (base->GetSize() <= replaceOpnd->GetSize()) &&
                /* In case that replace opnd was redefined. */
                !replaceInfo->redefined) {
                MemOperand *newMem = static_cast<MemOperand *>(memOpnd->Clone(*cgFunc->GetMemoryPool()));
                CHECK_FATAL(newMem != nullptr, "newMem is null in Ebo::BuildAllInfo(BB *bb)");
                newMem->SetBaseRegister(*static_cast<RegOperand *>(replaceOpnd));
                insn.SetOperand(opndIndex, *newMem);
                DecRef(*baseInfo);
                IncRef(*replaceInfo);
                baseInfo = replaceInfo;
            }
        }
    }
    if ((offset != nullptr) && offset->IsRegister()) {
        offsetInfo = OperandInfoUse(bb, *offset);
    }
    OpndInfo *opndInfo = OperandInfoUse(bb, insn.GetOperand(opndIndex));
    CHECK_FATAL(opndInfo != nullptr, "opndInfo should not be null ptr");
    MemOpndInfo *memInfo = static_cast<MemOpndInfo *>(opndInfo);
    if (baseInfo != nullptr) {
        memInfo->SetBaseInfo(*baseInfo);
    }
    if (offsetInfo != nullptr) {
        memInfo->SetOffsetInfo(*offsetInfo);
    }
    return memInfo;
}

OpndInfo *Ebo::BuildOperandInfo(BB &bb, Insn &insn, Operand &opnd, uint32 opndIndex, MapleVector<OpndInfo *> &origInfos)
{
    if (opnd.IsList()) {
        ListOperand *listOpnd = static_cast<ListOperand *>(&opnd);
        for (auto op : listOpnd->GetOperands()) {
            OperandInfoUse(bb, *op);
        }
        return nullptr;
    }
    DEBUG_ASSERT(opndIndex < origInfos.size(), "SetOpndInfo hashval outof range!");
    if (opnd.IsConditionCode()) {
        Operand &rFlag = cgFunc->GetOrCreateRflag();
        OperandInfoUse(bb, rFlag);
        /* if operand is Opnd_cond, the orig_info store the info of rFlag. */
        OpndInfo *tempOpndInfo = GetOpndInfo(rFlag, -1);
        origInfos.at(opndIndex) = tempOpndInfo;
        return nullptr;
    }

    if (!(opnd.IsRegister() || opnd.IsRegShift()) && !opnd.IsMemoryAccessOperand()) {
        return nullptr;
    }

    if (opnd.IsMemoryAccessOperand()) {
        OpndInfo *memInfo = BuildMemOpndInfo(bb, insn, opnd, opndIndex);
        CHECK_FATAL(memInfo != nullptr, "build memopnd info failed in Ebo::BuildAllInfo");
        origInfos.at(opndIndex) = memInfo;
        return nullptr;
    }
    OpndInfo *opndInfo = OperandInfoUse(bb, opnd);
    origInfos.at(opndIndex) = opndInfo;
    return opndInfo;
}

bool Ebo::ForwardPropagateOpnd(Insn &insn, Operand *&opnd, uint32 opndIndex, OpndInfo *&opndInfo,
                               MapleVector<OpndInfo *> &origInfos)
{
    CHECK_FATAL(opnd != nullptr, "nullptr check");
    Operand *opndReplace = opndInfo->replacementOpnd;
    /* Don't propagate physical registers before register allocation. */
    if (beforeRegAlloc && (opndReplace != nullptr) && (IsPhysicalReg(*opndReplace) || IsPhysicalReg(*opnd))) {
        return false;
    }

    /* forward propagation of constants */
    CHECK_FATAL(opndIndex < origInfos.size(), "SetOpndInfo hashval outof range!");
    if (!ForwardPropCheck(opndReplace, *opndInfo, *opnd, insn)) {
        return false;
    }
    Operand *oldOpnd = opnd;
    opnd = opndInfo->replacementOpnd;
    opndInfo = opndInfo->replacementInfo;

    /* constant prop. */
    if (opnd->IsIntImmediate() && oldOpnd->IsRegister()) {
        if (DoConstProp(insn, opndIndex, *opnd)) {
            DecRef(*origInfos.at(opndIndex));
            /* Update the actual expression info. */
            origInfos.at(opndIndex) = opndInfo;
        }
    }
    /* move reg, wzr, store vreg, mem ==> store wzr, mem */
#if TARGAARCH64 || TARGRISCV64
    if (IsZeroRegister(*opnd) && opndIndex == 0 &&
        (insn.GetMachineOpcode() == MOP_wstr || insn.GetMachineOpcode() == MOP_xstr)) {
        if (EBO_DUMP) {
            LogInfo::MapleLogger() << "===replace operand " << opndIndex << " of insn: \n";
            insn.Dump();
            LogInfo::MapleLogger() << "the new insn is:\n";
        }
        insn.SetOperand(opndIndex, *opnd);
        DecRef(*origInfos.at(opndIndex));
        /* Update the actual expression info. */
        origInfos.at(opndIndex) = opndInfo;
        if (EBO_DUMP) {
            insn.Dump();
        }
    }
#endif
    /* forward prop for registers. */
    if (!RegForwardCheck(insn, *opnd, opndReplace, *oldOpnd, origInfos.at(opndIndex))) {
        return false;
    }
    /* Copies to and from the same register are not needed. */
    if (!beforeRegAlloc && Globals::GetInstance()->GetTarget()->IsEffectiveCopy(insn) &&
        (opndIndex == kInsnSecondOpnd) && RegistersIdentical(*opnd, insn.GetOperand(kInsnFirstOpnd))) {
        if (EBO_DUMP) {
            LogInfo::MapleLogger() << "===replace operand " << opndIndex << " of insn: \n";
            insn.Dump();
            LogInfo::MapleLogger() << "===Remove the new insn because Copies to and from the same register. \n";
        }
        return true;
    }
    if (static_cast<RegOperand *>(opnd)->GetRegisterNumber() == RSP) {
        /* Disallow optimization with stack pointer */
        return false;
    }

    if (EBO_DUMP) {
        LogInfo::MapleLogger() << "===replace operand " << opndIndex << " of insn: \n";
        insn.Dump();
        LogInfo::MapleLogger() << "the new insn is:\n";
    }
    DecRef(*origInfos.at(opndIndex));
    insn.SetOperand(opndIndex, *opnd);

    if (EBO_DUMP) {
        insn.Dump();
    }
    IncRef(*opndInfo);
    /* Update the actual expression info. */
    origInfos.at(opndIndex) = opndInfo;
    /* extend the live range of the replacement operand. */
    if ((opndInfo->bb != insn.GetBB()) && opnd->IsRegister()) {
        MarkOpndLiveIntoBB(*opnd, *insn.GetBB(), *opndInfo->bb);
    }
    return false;
}

/*
 * this func do only one of the following optimization:
 * 1. Remove DupInsns
 * 2. SpecialSequence OPT
 * 3. Remove Redundant "Load"
 * 4. Constant Fold
 */
void Ebo::SimplifyInsn(Insn &insn, bool &insnReplaced, bool opndsConstant, const MapleVector<Operand *> &opnds,
                       const MapleVector<OpndInfo *> &opndInfos, const MapleVector<OpndInfo *> &origInfos)
{
    if (insn.AccessMem()) {
        if (!insnReplaced) {
            insnReplaced = SpecialSequence(insn, origInfos);
        }
        return;
    }
    if (Globals::GetInstance()->GetTarget()->IsEffectiveCopy(insn)) {
        if (!insnReplaced) {
            insnReplaced = SpecialSequence(insn, opndInfos);
        }
        return;
    }
    if (!insnReplaced && !insn.HasSideEffects()) {
        uint32 opndNum = insn.GetOperandSize();
        if (opndsConstant && (opndNum > 1)) {
            if (!insn.GetDefRegs().empty()) {
                insnReplaced = Csel2Cset(insn, opnds);
            }
        }
        if (insnReplaced) {
            return;
        }
        if (opndNum > 1) {
            /* special case */
            if (!insn.GetDefRegs().empty() && ResIsNotDefAndUse(insn)) {
                if ((opndNum == kInsnFourthOpnd) && (insn.GetDefRegs().size() == 1) &&
                    (((kInsnSecondOpnd < opnds.size()) && (opnds[kInsnSecondOpnd] != nullptr) &&
                      IsConstantImmOrReg(*opnds[kInsnSecondOpnd])) ||
                     ((kInsnThirdOpnd < opnds.size()) && (opnds[kInsnThirdOpnd] != nullptr) &&
                      IsConstantImmOrReg(*opnds[kInsnThirdOpnd])))) {
                    insnReplaced = SimplifyConstOperand(insn, opnds, opndInfos);
                }
            }
            if (!insnReplaced) {
                insnReplaced = SpecialSequence(insn, origInfos);
            }
        }
    }
}

/*
 * this func do:
 * 1. delete DupInsn if SimplifyInsn failed.
 * 2. buildInsnInfo if delete DupInsn failed(func HashInsn do this).
 * 3. update replaceInfo.
 */
void Ebo::FindRedundantInsns(BB &bb, Insn *&insn, const Insn *prev, bool insnReplaced, MapleVector<Operand *> &opnds,
                             MapleVector<OpndInfo *> &opndInfos, const MapleVector<OpndInfo *> &origInfos)
{
    CHECK_FATAL(insn != nullptr, "nullptr check");
    if (!insnReplaced) {
        CHECK_FATAL(origInfos.size() != 0, "null ptr check");
        CHECK_FATAL(opndInfos.size() != 0, "null ptr check");
        HashInsn(*insn, origInfos, opndInfos);
        /* Processing the result of the insn. */
        if ((Globals::GetInstance()->GetTarget()->IsEffectiveCopy(*insn) || !insn->GetDefRegs().empty()) &&
            !insn->IsSpecialIntrinsic()) {
            Operand *res = &insn->GetOperand(kInsnFirstOpnd);
            if ((res != nullptr) && (res != TRUE_OPND) && (res != GetZeroOpnd(res->GetSize()))) {
                CHECK_FATAL(lastInsnInfo != nullptr, "lastInsnInfo is null!");
                OpndInfo *opndInfo = lastInsnInfo->result[0];
                /* Don't propagate for fmov insns. */
                if (Globals::GetInstance()->GetTarget()->IsEffectiveCopy(*insn) && (opndInfo != nullptr) &&
                    !IsFmov(*insn)) {
                    CHECK_FATAL(!opnds.empty(), "null container!");
                    opndInfo->replacementOpnd = opnds[kInsnSecondOpnd];
                    opndInfo->replacementInfo = opndInfos[kInsnSecondOpnd];
                } else if (insn->GetBothDefUseOpnd() != kInsnMaxOpnd && (opndInfo != nullptr)) {
                    opndInfo->replacementOpnd = nullptr;
                    opndInfo->replacementInfo = nullptr;
                }
            }
        }
        insn = insn->GetNext();
    } else {
        uint32 opndNum = insn->GetOperandSize();
        RemoveUses(opndNum, origInfos);
        /* If insn is replaced, reanalyze the new insn to have more opportunities. */
        insn = (prev == nullptr ? bb.GetFirstInsn() : prev->GetNext());
    }
}

void Ebo::PreProcessSpecialInsn(Insn &insn)
{
    DefineReturnUseRegister(insn);

    if (insn.IsCall() || insn.IsClinit()) {
        DefineCallUseSpecialRegister(insn);
    }
}

/*
 * this func do :
 * 1.build opereand info of bb;
 * 2.do Forward propagation after regalloc;
 * 3.simplify the insn,include Constant folding,redundant insns elimination.
 */
void Ebo::BuildAllInfo(BB &bb)
{
    if (EBO_DUMP) {
        LogInfo::MapleLogger() << "===Enter BuildOperandinfo of bb:" << bb.GetId() << "===\n";
    }
    Insn *insn = bb.GetFirstInsn();
    while ((insn != nullptr) && (insn != bb.GetLastInsn()->GetNext())) {
        if (!insn->IsTargetInsn()) {
            insn = insn->GetNext();
            continue;
        }
        PreProcessSpecialInsn(*insn);
        uint32 opndNum = insn->GetOperandSize();
        if (!insn->IsMachineInstruction() || opndNum == 0) {
            insn = insn->GetNext();
            continue;
        }
        MapleVector<Operand *> opnds(eboAllocator.Adapter());
        MapleVector<OpndInfo *> opndInfos(eboAllocator.Adapter());
        MapleVector<OpndInfo *> origInfos(eboAllocator.Adapter());
        Insn *prev = insn->GetPrev();
        bool insnReplaced = false;
        bool opndsConstant = true;
        /* start : Process all the operands. */
        for (uint32 i = 0; i < opndNum; ++i) {
            if (!insn->OpndIsUse(i)) {
                opnds.emplace_back(nullptr);
                opndInfos.emplace_back(nullptr);
                origInfos.emplace_back(nullptr);
                continue;
            }
            Operand *opnd = &(insn->GetOperand(i));
            opnds.emplace_back(opnd);
            opndInfos.emplace_back(nullptr);
            origInfos.emplace_back(nullptr);
            if (IsConstantImmOrReg(*opnd)) {
                continue;
            }
            OpndInfo *opndInfo = BuildOperandInfo(bb, *insn, *opnd, i, origInfos);
            if (opndInfo == nullptr) {
                continue;
            }

            /* Don't do propagation for special intrinsic insn. */
            if (!insn->IsSpecialIntrinsic()) {
                insnReplaced = ForwardPropagateOpnd(*insn, opnd, i, opndInfo, origInfos);
            }
            if (insnReplaced) {
                continue;
            }
            opnds.at(i) = opnd;
            opndInfos.at(i) = opndInfo;
            if (!IsConstantImmOrReg(*opnd)) {
                opndsConstant = false;
            }
        } /* End : Process all the operands. */
#if TARGARM32
        Arm32Insn *currArm32Insn = static_cast<Arm32Insn *>(insn);
        if (currArm32Insn->IsCondExecution()) {
            Operand &rFlag = cgFunc->GetOrCreateRflag();
            OperandInfoUse(bb, rFlag);
        }
#endif

        if (insnReplaced) {
            RemoveUses(opndNum, origInfos);
            Insn *temp = insn->GetNext();
            bb.RemoveInsn(*insn);
            insn = temp;
            continue;
        }

        /* simplify the insn. */
        if (!insn->IsSpecialIntrinsic()) {
            SimplifyInsn(*insn, insnReplaced, opndsConstant, opnds, opndInfos, origInfos);
        }
        FindRedundantInsns(bb, insn, prev, insnReplaced, opnds, opndInfos, origInfos);
    }
}

/* Decrement the use counts for the actual operands of an insnInfo. */
void Ebo::RemoveInsn(InsnInfo &info)
{
    Insn *insn = info.insn;
    CHECK_FATAL(insn != nullptr, "get insn in info failed in Ebo::RemoveInsn");
    uint32 opndNum = insn->GetOperandSize();
    OpndInfo *opndInfo = nullptr;
    for (uint32 i = 0; i < opndNum; i++) {
        if (!insn->OpndIsUse(i)) {
            continue;
        }
        opndInfo = info.origOpnd[i];
        if (opndInfo != nullptr) {
            DecRef(*opndInfo);
            Operand *opndTemp = opndInfo->opnd;
            if (opndTemp == nullptr) {
                continue;
            }
            if (opndTemp->IsMemoryAccessOperand()) {
                MemOpndInfo *memInfo = static_cast<MemOpndInfo *>(opndInfo);
                OpndInfo *baseInfo = memInfo->GetBaseInfo();
                OpndInfo *offInfo = memInfo->GetOffsetInfo();
                if (baseInfo != nullptr) {
                    DecRef(*baseInfo);
                }
                if (offInfo != nullptr) {
                    DecRef(*offInfo);
                }
            }
        }
    }
#if TARGARM32
    Arm32CGFunc *a32CGFunc = static_cast<Arm32CGFunc *>(cgFunc);
    auto &gotInfosMap = a32CGFunc->GetGotInfosMap();
    for (auto it = gotInfosMap.begin(); it != gotInfosMap.end();) {
        if (it->first == insn) {
            it = gotInfosMap.erase(it);
        } else {
            ++it;
        }
    }
    auto &constInfosMap = a32CGFunc->GetConstInfosMap();
    for (auto it = constInfosMap.begin(); it != constInfosMap.end();) {
        if (it->first == insn) {
            it = constInfosMap.erase(it);
        } else {
            ++it;
        }
    }
#endif
}

/* Mark opnd is live between def bb and into bb. */
void Ebo::MarkOpndLiveIntoBB(const Operand &opnd, BB &into, BB &def) const
{
    if (live == nullptr) {
        return;
    }
    if (&into == &def) {
        return;
    }
    CHECK_FATAL(opnd.IsRegister(), "expect register here.");
    const RegOperand &reg = static_cast<const RegOperand &>(opnd);
    into.SetLiveInBit(reg.GetRegisterNumber());
    def.SetLiveOutBit(reg.GetRegisterNumber());
}

/* return insn information if has insnInfo,else,return lastInsnInfo */
InsnInfo *Ebo::LocateInsnInfo(const OpndInfo &info)
{
    if (info.insn != nullptr) {
        if (info.insnInfo != nullptr) {
            return info.insnInfo;
        } else {
            InsnInfo *insnInfo = lastInsnInfo;
            int32 limit = 50;
            for (; (insnInfo != nullptr) && (limit != 0); insnInfo = insnInfo->prev, limit--) {
                if (insnInfo->insn == info.insn) {
                    return insnInfo;
                }
            }
        }
    }
    return nullptr;
}

/* redundant insns elimination */
void Ebo::RemoveUnusedInsns(BB &bb, bool normal)
{
    OpndInfo *opndInfo = nullptr;
    Operand *opnd = nullptr;

    if (firstInsnInfo == nullptr) {
        return;
    }

    for (InsnInfo *insnInfo = lastInsnInfo; insnInfo != nullptr; insnInfo = insnInfo->prev) {
        Insn *insn = insnInfo->insn;
        if ((insn == nullptr) || (insn->GetBB() == nullptr)) {
            continue;
        }
        /* stop looking for insn when it goes out of bb. */
        if (insn->GetBB() != &bb) {
            break;
        }

        uint32 resNum = insn->GetDefRegs().size();
        if (IsLastAndBranch(bb, *insn)) {
            goto insn_is_needed;
        }

        if (insn->IsClinit()) {
            goto insn_is_needed;
        }

        if ((resNum == 0) || IsGlobalNeeded(*insn) || insn->IsStore() || IsDecoupleStaticOp(*insn) ||
            insn->GetBothDefUseOpnd() != kInsnMaxOpnd) {
            goto insn_is_needed;
        }

        /* last insn of a 64x1 function is a float, 64x1 function may not be a float */
        if (cgFunc->GetFunction().GetAttr(FUNCATTR_oneelem_simd) && insnInfo == lastInsnInfo) {
            goto insn_is_needed;
        }

        if (insn->GetMachineOpcode() == MOP_asm || insn->IsAtomic()) {
            goto insn_is_needed;
        }

        /* Check all result that can be removed. */
        for (uint32 i = 0; i < resNum; ++i) {
            opndInfo = insnInfo->result[i];
            /* A couple of checks. */
            if (opndInfo == nullptr) {
                continue;
            }
            if ((opndInfo->bb != &bb) || (opndInfo->insn == nullptr)) {
                goto insn_is_needed;
            }
            opnd = opndInfo->opnd;
            if (opnd == GetZeroOpnd(opnd->GetSize())) {
                continue;
            }
            /* this part optimize some spacial case after RA. */
            if (!beforeRegAlloc && Globals::GetInstance()->GetTarget()->IsEffectiveCopy(*insn) && opndInfo &&
                insn->GetOperand(kInsnSecondOpnd).IsImmediate() && IsSameRedefine(bb, *insn, *opndInfo)) {
                goto can_be_removed;
            }
            /* end special case optimize */
            if ((beforeRegAlloc && IsPhysicalReg(*opnd)) || (IsSaveReg(*opnd) && !opndInfo->redefinedInBB)) {
                goto insn_is_needed;
            }
            /* Copies to and from the same register are not needed. */
            if (Globals::GetInstance()->GetTarget()->IsEffectiveCopy(*insn)) {
                if (HasAssignedReg(*opnd) && HasAssignedReg(insn->GetOperand(kInsnSecondOpnd)) &&
                    RegistersIdentical(*opnd, insn->GetOperand(kInsnSecondOpnd))) {
                    /* We may be able to get rid of the copy, but be sure that the operand is marked live into this
                     * block. */
                    if ((insnInfo->origOpnd[kInsnSecondOpnd] != nullptr) &&
                        (&bb != insnInfo->origOpnd[kInsnSecondOpnd]->bb)) {
                        MarkOpndLiveIntoBB(*opnd, bb, *insnInfo->origOpnd[kInsnSecondOpnd]->bb);
                    }
                    /* propagate use count for this opnd to it's input operand. */
                    if (opndInfo->same != nullptr) {
                        opndInfo->same->refCount += opndInfo->refCount;
                    }

                    /* remove the copy causes the previous def to reach the end of the block. */
                    if (!opndInfo->redefined && (opndInfo->same != nullptr)) {
                        opndInfo->same->redefined = false;
                        opndInfo->same->redefinedInBB = false;
                    }
                    goto can_be_removed;
                }
            }
            /* there must bo no direct references to the operand. */
            if (!normal || (opndInfo->refCount != 0)) {
                goto insn_is_needed;
            }
            /*
             * When O1, the vreg who live out of bb should be recognized.
             * The regs for clinit is also be marked to recognize it can't be deleted. so extend it to O2.
             */
            if (opnd->IsRegister()) {
                RegOperand *reg = static_cast<RegOperand *>(opnd);
                if (beforeRegAlloc && !reg->IsBBLocalVReg()) {
                    goto insn_is_needed;
                }
            }
            /* Volatile || sideeffect */
            if (opndInfo->insn->IsVolatile() || opndInfo->insn->HasSideEffects()) {
                goto insn_is_needed;
            }

            if (!opndInfo->redefinedInBB && LiveOutOfBB(*opnd, *opndInfo->bb)) {
                goto insn_is_needed;
            }

            if (opndInfo->redefinedInBB && opndInfo->redefinedInsn != nullptr &&
                opndInfo->redefinedInsn->GetBothDefUseOpnd() != kInsnMaxOpnd) {
                goto insn_is_needed;
            }
        }

        if (!normal || insnInfo->mustNotBeRemoved || insn->GetDoNotRemove()) {
            goto insn_is_needed;
        }
    can_be_removed:
        if (EBO_DUMP) {
            LogInfo::MapleLogger() << "< ==== Remove Unused insn in bb:" << bb.GetId() << "====\n";
            insn->Dump();
        }
        RemoveInsn(*insnInfo);
        bb.RemoveInsn(*insn);
        insnInfo->insn = nullptr;
        insnInfo->bb = nullptr;
        for (uint32 i = 0; i < resNum; i++) {
            opndInfo = insnInfo->result[i];
            if (opndInfo == nullptr) {
                continue;
            }
            if (opndInfo->redefined && (opndInfo->same != nullptr)) {
                OpndInfo *next = opndInfo->same;
                next->redefined = true;
                if (opndInfo->redefinedInBB && (opndInfo->same->bb == &bb)) {
                    next->redefinedInBB = true;
                }
            }
            if (!opndInfo->redefinedInBB && (opndInfo->same != nullptr) && (opndInfo->same->bb == &bb)) {
                opndInfo->same->redefinedInBB = false;
            }
            if (!opndInfo->redefined && (opndInfo->same != nullptr)) {
                opndInfo->same->redefined = false;
                opndInfo->same->redefinedInBB = false;
            }
        }
        optSuccess = true;
        continue;
    insn_is_needed:
        if (!bb.GetEhSuccs().empty()) {
            for (uint32 i = 0; i < resNum; i++) {
                opndInfo = insnInfo->result[i];
                if ((opndInfo != nullptr) && (opndInfo->opnd != nullptr) && (opndInfo->same != nullptr)) {
                    UpdateNextInfo(*opndInfo);
                }
            }
        }

        if (!bb.GetEhPreds().empty()) {
            for (uint32 i = 0; i < insn->GetOperandSize(); ++i) {
                opndInfo = insnInfo->origOpnd[i];
                if ((opndInfo != nullptr) && (opndInfo->opnd != nullptr) && (opndInfo->same != nullptr)) {
                    UpdateNextInfo(*opndInfo);
                }
                if ((opndInfo != nullptr) && opndInfo->opnd && (&bb != opndInfo->bb) && opndInfo->opnd->IsRegister()) {
                    MarkOpndLiveIntoBB(*opndInfo->opnd, bb, *opndInfo->bb);
                }
            }
        }
    } /* end proccess insnInfo in currBB */
}

void Ebo::UpdateNextInfo(const OpndInfo &opndInfo)
{
    OpndInfo *nextInfo = opndInfo.same;
    while (nextInfo != nullptr) {
        if (nextInfo->insn != nullptr) {
            InsnInfo *info = LocateInsnInfo(*nextInfo);
            if (info != nullptr) {
                info->mustNotBeRemoved = true;
            } else {
                /*
                 * Couldn't find the insnInfo entry.  Make sure that the operand has
                 * a use count so that the defining insn will not be deleted.
                 */
                nextInfo->refCount += opndInfo.refCount;
            }
        }
        nextInfo = nextInfo->same;
    }
}

/* back up to last saved OpndInfo */
void Ebo::BackupOpndInfoList(OpndInfo *saveLast)
{
    if (lastOpndInfo == saveLast) {
        return;
    }
    OpndInfo *opndInfo = lastOpndInfo;
    while (opndInfo != saveLast) {
        int32 hashVal = 0;
        if (opndInfo->opnd->IsRegister() || opndInfo->opnd->IsRegShift()) {
            hashVal = -1;
        } else {
            hashVal = opndInfo->hashVal;
        }
        UpdateOpndInfo(*opndInfo->opnd, *opndInfo, opndInfo->same, hashVal);
        opndInfo = opndInfo->prev;
    }
    if (saveLast != nullptr) {
        saveLast->next = nullptr;
        lastOpndInfo = saveLast;
    } else {
        firstOpndInfo = nullptr;
        lastOpndInfo = nullptr;
    }
}

/* back up to last saved insn */
void Ebo::BackupInsnInfoList(InsnInfo *saveLast)
{
    if (lastInsnInfo == saveLast) {
        return;
    }
    InsnInfo *insnInfo = lastInsnInfo;
    while (insnInfo != saveLast) {
        SetInsnInfo(insnInfo->hashIndex, *(insnInfo->same));
        insnInfo = insnInfo->prev;
    }
    if (saveLast != nullptr) {
        saveLast->next = nullptr;
        lastInsnInfo = saveLast;
    } else {
        firstInsnInfo = nullptr;
        lastInsnInfo = nullptr;
    }
}

/* add bb to eb ,and build operandinfo of bb */
void Ebo::AddBB2EB(BB &bb)
{
    OpndInfo *saveLastOpndInfo = lastOpndInfo;
    InsnInfo *saveLastInsnInfo = lastInsnInfo;
    SetBBVisited(bb);
    bbNum++;
    BuildAllInfo(bb);
    /* Stop adding BB to EB if the bbs in the current EB exceeds kEboMaxBBNums */
    if (bbNum < kEboMaxBBNums) {
        for (auto *bbSucc : bb.GetSuccs()) {
            if ((bbSucc->GetPreds().size() == 1) && IsNotVisited(*bbSucc)) {
                AddBB2EB(*bbSucc);
            }
        }
    }

    RemoveUnusedInsns(bb, true);
    /* Remove information about Operand's and Insn's in this block. */
    BackupOpndInfoList(saveLastOpndInfo);
    BackupInsnInfoList(saveLastInsnInfo);
    bbNum--;
}

/* Perform EBO */
void Ebo::EboProcess()
{
    FOR_ALL_BB(bb, cgFunc) {
        if (IsNotVisited(*bb)) {
            bbNum = 0;
            AddBB2EB(*bb);
        }
    }
}

/* Perform EBO on O1 which the optimization can only be in a single block. */
void Ebo::EboProcessSingleBB()
{
    FOR_ALL_BB(bb, cgFunc) {
        OpndInfo *saveLastOpndInfo = lastOpndInfo;
        InsnInfo *saveLastInsnInfo = lastInsnInfo;
        BuildAllInfo(*bb);
        RemoveUnusedInsns(*bb, true);
        /* Remove information about Operand's and Insn's in this block. */
        BackupOpndInfoList(saveLastOpndInfo);
        BackupInsnInfoList(saveLastInsnInfo);
    }
}

void Ebo::EboInit()
{
    visitedBBs.resize(cgFunc->NumBBs());
    for (uint32 i = 0; i < cgFunc->NumBBs(); ++i) {
        visitedBBs[i] = false;
    }
    exprInfoTable.resize(kEboMaxOpndHash);
    for (uint32 i = 0; i < kEboMaxOpndHash; ++i) {
        exprInfoTable.at(i) = nullptr;
    }
    insnInfoTable.resize(kEboMaxInsnHash);
    for (uint32 i = 0; i < kEboMaxInsnHash; ++i) {
        insnInfoTable.at(i) = nullptr;
    }
    if (!beforeRegAlloc) {
        BuildCallerSaveRegisters();
    }
    optSuccess = false;
}

/* perform EB optimizations right after instruction selection. */
void Ebo::Run()
{
    EboInit();
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2) {
        EboProcess();
    } else {
        EboProcessSingleBB(); /* Perform SingleBB Optimization when -O1. */
    }
    if (optSuccess && cgFunc->GetMirModule().IsCModule()) {
        Run();
    }
}

/* === new pm === */
bool CgEbo0::PhaseRun(maplebe::CGFunc &f)
{
    if (EBO_DUMP_NEWPM) {
        DotGenerator::GenerateDot("ebo0", f, f.GetMirModule());
    }
    LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
    MemPool *eboMp = GetPhaseMemPool();
    Ebo *ebo = nullptr;
#if TARGAARCH64 || TARGRISCV64
    ebo = eboMp->New<AArch64Ebo>(f, *eboMp, live, true, PhaseName());
#endif
#if TARGARM32
    ebo = eboMp->New<Arm32Ebo>(f, *eboMp, live, true, "ebo0");
#endif
    ebo->Run();
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
        live->ClearInOutDataInfo();
    }
    return true;
}

void CgEbo0::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.AddPreserved<CgLoopAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgEbo0, ebo)

bool CgEbo1::PhaseRun(maplebe::CGFunc &f)
{
    if (EBO_DUMP_NEWPM) {
        DotGenerator::GenerateDot(PhaseName(), f, f.GetMirModule(), true);
    }
    LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
    MemPool *eboMp = GetPhaseMemPool();
    Ebo *ebo = nullptr;
#if TARGAARCH64 || TARGRISCV64
    ebo = eboMp->New<AArch64Ebo>(f, *eboMp, live, true, PhaseName());
#endif
#if TARGARM32
    ebo = eboMp->New<Arm32Ebo>(f, *eboMp, live, true, PhaseName());
#endif
    ebo->Run();
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
        live->ClearInOutDataInfo();
    }
    return true;
}

void CgEbo1::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.AddPreserved<CgLoopAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgEbo1, ebo1)

bool CgPostEbo::PhaseRun(maplebe::CGFunc &f)
{
    if (EBO_DUMP_NEWPM) {
        DotGenerator::GenerateDot(PhaseName(), f, f.GetMirModule());
    }
    LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
    MemPool *eboMp = GetPhaseMemPool();
    Ebo *ebo = nullptr;
#if TARGAARCH64 || TARGRISCV64
    ebo = eboMp->New<AArch64Ebo>(f, *eboMp, live, false, PhaseName());
#endif
#if TARGARM32
    ebo = eboMp->New<Arm32Ebo>(f, *eboMp, live, false, PhaseName());
#endif
    ebo->Run();
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
        live->ClearInOutDataInfo();
    }
    return true;
}

void CgPostEbo::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.AddPreserved<CgLoopAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPostEbo, postebo)
} /* namespace maplebe */
