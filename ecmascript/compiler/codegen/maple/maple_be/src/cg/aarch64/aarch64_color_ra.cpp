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

#include "aarch64_color_ra.h"
#include <iostream>
#include <fstream>
#include "aarch64_cg.h"
#include "mir_lower.h"
#include "securec.h"
/*
 * Based on concepts from Chow and Hennessey.
 * Phases are as follows:
 *   Prepass to collect local BB information.
 *     Compute local register allocation demands for global RA.
 *   Compute live ranges.
 *     Live ranges LR represented by a vector of size #BBs.
 *     for each cross bb vreg, a bit is set in the vector.
 *   Build interference graph with basic block as granularity.
 *     When intersection of two LRs is not null, they interfere.
 *   Separate unconstrained and constrained LRs.
 *     unconstrained - LR with connect edges less than available colors.
 *                     These LR can always be colored.
 *     constrained - not uncontrained.
 *   Split LR based on priority cost
 *     Repetitive adding BB from original LR to new LR until constrained.
 *     Update all LR the new LR interferes with.
 *   Color the new LR
 *     Each LR has a forbidden list, the registers cannot be assigned
 *     Coalesce move using preferred color first.
 *   Mark the remaining uncolorable LR after split as spill.
 *   Local register allocate.
 *   Emit and insert spills.
 */
namespace maplebe {
#define CLANG (cgFunc->GetMirModule().IsCModule())

void LiveUnit::PrintLiveUnit() const
{
    LogInfo::MapleLogger() << "[" << begin << "," << end << "]"
                           << "<D" << defNum << "U" << useNum << ">";
    if (!hasCall) {
        /* Too many calls, so only print when there is no call. */
        LogInfo::MapleLogger() << " nc";
    }
    if (needReload) {
        LogInfo::MapleLogger() << " rlod";
    }
    if (needRestore) {
        LogInfo::MapleLogger() << " rstr";
    }
}

template <typename Func>
void GraphColorRegAllocator::ForEachBBArrElem(const uint64 *vec, Func functor) const
{
    for (uint32 iBBArrElem = 0; iBBArrElem < bbBuckets; ++iBBArrElem) {
        for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
            if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
                functor(iBBArrElem * kU64 + bBBArrElem);
            }
        }
    }
}

template <typename Func>
void GraphColorRegAllocator::ForEachBBArrElemWithInterrupt(const uint64 *vec, Func functor) const
{
    for (uint32 iBBArrElem = 0; iBBArrElem < bbBuckets; ++iBBArrElem) {
        for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
            if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
                if (functor(iBBArrElem * kU64 + bBBArrElem)) {
                    return;
                }
            }
        }
    }
}

template <typename Func>
void GraphColorRegAllocator::ForEachRegArrElem(const uint64 *vec, Func functor) const
{
    for (uint32 iBBArrElem = 0; iBBArrElem < regBuckets; ++iBBArrElem) {
        for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
            if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
                functor(iBBArrElem * kU64 + bBBArrElem);
            }
        }
    }
}

void GraphColorRegAllocator::PrintLiveUnitMap(const LiveRange &lr) const
{
    LogInfo::MapleLogger() << "\n\tlu:";
    for (uint32 i = 0; i < cgFunc->NumBBs(); ++i) {
        if (!IsBitArrElemSet(lr.GetBBMember(), i)) {
            continue;
        }
        auto lu = lr.GetLuMap().find(i);
        if (lu != lr.GetLuMap().end() && (lu->second->GetDefNum() || lu->second->GetUseNum())) {
            LogInfo::MapleLogger() << "(" << i << " ";
            lu->second->PrintLiveUnit();
            LogInfo::MapleLogger() << ")";
        }
    }
    LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLiveRangeConflicts(const LiveRange &lr) const
{
    LogInfo::MapleLogger() << "\n\tinterfere(" << lr.GetNumBBConflicts() << "): ";
    for (uint32 i = 0; i < regBuckets; ++i) {
        uint64 chunk = lr.GetBBConflictElem(i);
        for (uint64 bit = 0; bit < kU64; ++bit) {
            if (chunk & (1ULL << bit)) {
                regno_t newNO = i * kU64 + bit;
                LogInfo::MapleLogger() << newNO << ",";
            }
        }
    }
    LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLiveBBBit(const LiveRange &lr) const
{
    LogInfo::MapleLogger() << "live_bb(" << lr.GetNumBBMembers() << "): ";
    for (uint32 i = 0; i < cgFunc->NumBBs(); ++i) {
        if (IsBitArrElemSet(lr.GetBBMember(), i)) {
            LogInfo::MapleLogger() << i << " ";
        }
    }
    LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLiveRange(const LiveRange &lr, const std::string &str) const
{
    LogInfo::MapleLogger() << str << "\n";

    LogInfo::MapleLogger() << "R" << lr.GetRegNO();
    if (lr.GetRegType() == kRegTyInt) {
        LogInfo::MapleLogger() << "(I)";
    } else if (lr.GetRegType() == kRegTyFloat) {
        LogInfo::MapleLogger() << "(F)";
    } else {
        LogInfo::MapleLogger() << "(U)";
    }
    if (lr.GetSpillSize() == k32) {
        LogInfo::MapleLogger() << "S32";
    } else if (lr.GetSpillSize() == k64) {
        LogInfo::MapleLogger() << "S64";
    } else {
        LogInfo::MapleLogger() << "S0(nodef)";
    }
    LogInfo::MapleLogger() << "\tnumCall " << lr.GetNumCall();
    LogInfo::MapleLogger() << "\tpriority " << lr.GetPriority();
    LogInfo::MapleLogger() << "\tforbidden: ";
    for (regno_t preg = kInvalidRegNO; preg < kMaxRegNum; preg++) {
        if (lr.GetForbidden(preg)) {
            LogInfo::MapleLogger() << preg << ",";
        }
    }
    LogInfo::MapleLogger() << "\tpregveto: ";
    for (regno_t preg = kInvalidRegNO; preg < kMaxRegNum; preg++) {
        if (lr.GetPregveto(preg)) {
            LogInfo::MapleLogger() << preg << ",";
        }
    }
    if (lr.IsSpilled()) {
        LogInfo::MapleLogger() << " spilled";
    }
    if (lr.GetSplitLr()) {
        LogInfo::MapleLogger() << " split";
    }
    LogInfo::MapleLogger() << "\top: " << kOpcodeInfo.GetName(lr.GetOp());
    LogInfo::MapleLogger() << "\n";
    PrintLiveBBBit(lr);
    PrintLiveRangeConflicts(lr);
    PrintLiveUnitMap(lr);
    if (lr.GetSplitLr()) {
        PrintLiveRange(*lr.GetSplitLr(), "===>Split LR");
    }
}

void GraphColorRegAllocator::PrintLiveRanges() const
{
    LogInfo::MapleLogger() << "PrintLiveRanges: size = " << lrMap.size() << "\n";
    for (auto it : lrMap) {
        PrintLiveRange(*it.second, "");
    }
    LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLocalRAInfo(const std::string &str) const
{
    LogInfo::MapleLogger() << str << "\n";
    for (uint32 id = 0; id < cgFunc->NumBBs(); ++id) {
        LocalRaInfo *lraInfo = localRegVec[id];
        if (lraInfo == nullptr) {
            continue;
        }
        LogInfo::MapleLogger() << "bb " << id << " def ";
        for (const auto &defCntPair : lraInfo->GetDefCnt()) {
            LogInfo::MapleLogger() << "[" << defCntPair.first << ":" << defCntPair.second << "],";
        }
        LogInfo::MapleLogger() << "\n";
        LogInfo::MapleLogger() << "use ";
        for (const auto &useCntPair : lraInfo->GetUseCnt()) {
            LogInfo::MapleLogger() << "[" << useCntPair.first << ":" << useCntPair.second << "],";
        }
        LogInfo::MapleLogger() << "\n";
    }
}

void GraphColorRegAllocator::PrintBBAssignInfo() const
{
    for (size_t id = 0; id < bfs->sortedBBs.size(); ++id) {
        uint32 bbID = bfs->sortedBBs[id]->GetId();
        BBAssignInfo *bbInfo = bbRegInfo[bbID];
        if (bbInfo == nullptr) {
            continue;
        }
        LogInfo::MapleLogger() << "BBinfo(" << id << ")";
        LogInfo::MapleLogger() << " lra-needed int " << bbInfo->GetIntLocalRegsNeeded();
        LogInfo::MapleLogger() << " fp " << bbInfo->GetFpLocalRegsNeeded();
        LogInfo::MapleLogger() << " greg-used ";
        for (regno_t regNO = kInvalidRegNO; regNO < kMaxRegNum; ++regNO) {
            if (bbInfo->GetGlobalsAssigned(regNO)) {
                LogInfo::MapleLogger() << regNO << ",";
            }
        }
        LogInfo::MapleLogger() << "\n";
    }
}

void GraphColorRegAllocator::PrintBBs() const
{
    for (auto *bb : bfs->sortedBBs) {
        LogInfo::MapleLogger() << "\n< === > ";
        LogInfo::MapleLogger() << bb->GetId();
        LogInfo::MapleLogger() << " succs:";
        for (auto *succBB : bb->GetSuccs()) {
            LogInfo::MapleLogger() << " " << succBB->GetId();
        }
        LogInfo::MapleLogger() << " eh_succs:";
    }
    LogInfo::MapleLogger() << "\n";
}

uint32 GraphColorRegAllocator::MaxIntPhysRegNum() const
{
    return (R28 - R0);
}

uint32 GraphColorRegAllocator::MaxFloatPhysRegNum() const
{
    return (V31 - V0);
}

bool GraphColorRegAllocator::IsReservedReg(AArch64reg regNO) const
{
    if (!doMultiPass || cgFunc->GetMirModule().GetSrcLang() != kSrcLangC) {
        return (regNO == R16) || (regNO == R17);
    } else {
        return (regNO == R16);
    }
}

void GraphColorRegAllocator::InitFreeRegPool()
{
    /*
     *  ==== int regs ====
     *  FP 29, LR 30, SP 31, 0 to 7 parameters

     *  MapleCG defines 32 as ZR (zero register)
     *  use 8 if callee does not return large struct ? No
     *  16 and 17 are intra-procedure call temp, can be caller saved
     *  18 is platform reg, still use it
     */
    uint32 intNum = 0;
    uint32 fpNum = 0;
    for (regno_t regNO = kRinvalid; regNO < kMaxRegNum; ++regNO) {
        if (!AArch64Abi::IsAvailableReg(static_cast<AArch64reg>(regNO))) {
            continue;
        }

#ifdef RESERVED_REGS
        /* r16,r17 are used besides ra. */
        if (IsReservedReg(static_cast<AArch64reg>(regNO))) {
            continue;
        }
#endif /* RESERVED_REGS */

        if (AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO))) {
            /* when yieldpoint is enabled, x19 is reserved. */
            if (IsYieldPointReg(static_cast<AArch64reg>(regNO))) {
                continue;
            }
            if (regNO == R29) {
                if (!cgFunc->UseFP()) {
                    (void)intCalleeRegSet.insert(regNO - R0);
                    ++intNum;
                }
                continue;
            }
            if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regNO))) {
                (void)intCalleeRegSet.insert(regNO - R0);
            } else {
                (void)intCallerRegSet.insert(regNO - R0);
            }
            ++intNum;
        } else {
            if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regNO))) {
                (void)fpCalleeRegSet.insert(regNO - V0);
            } else {
                (void)fpCallerRegSet.insert(regNO - V0);
            }
            ++fpNum;
        }
    }
    intRegNum = intNum;
    fpRegNum = fpNum;
}

void GraphColorRegAllocator::InitCCReg()
{
    Operand &opnd = cgFunc->GetOrCreateRflag();
    auto &tmpRegOp = static_cast<RegOperand &>(opnd);
    ccReg = tmpRegOp.GetRegisterNumber();
}

bool GraphColorRegAllocator::IsYieldPointReg(regno_t regNO) const
{
    if (cgFunc->GetCG()->GenYieldPoint()) {
        return (regNO == RYP);
    }
    return false;
}

bool GraphColorRegAllocator::IsUnconcernedReg(regno_t regNO) const
{
    /* RFP = 32, RLR = 31, RSP = 33, RZR = 34 */
    if ((regNO >= RLR && regNO <= RZR) || regNO == RFP || regNO == ccReg) {
        return true;
    }

    /* when yieldpoint is enabled, the RYP(x19) can not be used. */
    if (IsYieldPointReg(static_cast<AArch64reg>(regNO))) {
        return true;
    }

    return false;
}

bool GraphColorRegAllocator::IsUnconcernedReg(const RegOperand &regOpnd) const
{
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return true;
    }
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (regNO == RZR) {
        return true;
    }
    return IsUnconcernedReg(regNO);
}

/*
 *  Based on live analysis, the live-in and live-out set determines
 *  the bit to be set in the LR vector, which is of size #BBs.
 *  If a vreg is in the live-in and live-out set, it is live in the BB.
 *
 *  Also keep track if a LR crosses a call.  If a LR crosses a call, it
 *  interferes with all caller saved registers.  Add all caller registers
 *  to the LR's forbidden list.
 *
 *  Return created LiveRange object
 *
 *  maybe need extra info:
 *  Add info for setjmp.
 *  Add info for defBB, useBB, index in BB for def and use
 *  Add info for startingBB and endingBB
 */
LiveRange *GraphColorRegAllocator::NewLiveRange()
{
    LiveRange *lr = memPool->New<LiveRange>(alloc);

    if (bbBuckets == 0) {
        bbBuckets = (cgFunc->NumBBs() / kU64) + 1;
    }
    lr->SetBBBuckets(bbBuckets);
    lr->InitBBMember(*memPool, bbBuckets);
    if (regBuckets == 0) {
        regBuckets = (cgFunc->GetMaxRegNum() / kU64) + 1;
    }
    lr->SetRegBuckets(regBuckets);
    lr->InitBBConflict(*memPool, regBuckets);
    lr->InitPregveto();
    lr->InitForbidden();
    return lr;
}

/* Create local info for LR.  return true if reg is not local. */
bool GraphColorRegAllocator::CreateLiveRangeHandleLocal(regno_t regNO, const BB &bb, bool isDef)
{
    if (FindIn(bb.GetLiveInRegNO(), regNO) || FindIn(bb.GetLiveOutRegNO(), regNO)) {
        return true;
    }
    /*
     *  register not in globals for the bb, so it is local.
     *  Compute local RA info.
     */
    LocalRaInfo *lraInfo = localRegVec[bb.GetId()];
    if (lraInfo == nullptr) {
        lraInfo = memPool->New<LocalRaInfo>(alloc);
        localRegVec[bb.GetId()] = lraInfo;
    }
    if (isDef) {
        /* movk is handled by different id for use/def in the same insn. */
        lraInfo->SetDefCntElem(regNO, lraInfo->GetDefCntElem(regNO) + 1);
    } else {
        lraInfo->SetUseCntElem(regNO, lraInfo->GetUseCntElem(regNO) + 1);
    }
    /* lr info is useful for lra, so continue lr info */
    return false;
}

LiveRange *GraphColorRegAllocator::CreateLiveRangeAllocateAndUpdate(regno_t regNO, const BB &bb, bool isDef,
                                                                    uint32 currId)
{
    LiveRange *lr = GetLiveRange(regNO);
    if (lr == nullptr) {
        lr = NewLiveRange();
        lr->SetID(currId);

        LiveUnit *lu = memPool->New<LiveUnit>();
        lr->SetElemToLuMap(bb.GetId(), *lu);
        lu->SetBegin(currId);
        lu->SetEnd(currId);
        if (isDef) {
            /* means no use after def for reg, chances for ebo opt */
            for (const auto &pregNO : pregLive) {
                lr->InsertElemToPregveto(pregNO);
            }
        }
    } else {
        LiveUnit *lu = lr->GetLiveUnitFromLuMap(bb.GetId());
        if (lu == nullptr) {
            lu = memPool->New<LiveUnit>();
            lr->SetElemToLuMap(bb.GetId(), *lu);
            lu->SetBegin(currId);
            lu->SetEnd(currId);
        }
        if (lu->GetBegin() > currId) {
            lu->SetBegin(currId);
        }
    }

    if (CLANG) {
        auto *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
        MIRPreg *preg = a64CGFunc->GetPseudoRegFromVirtualRegNO(regNO, CGOptions::DoCGSSA());
        if (preg) {
            switch (preg->GetOp()) {
                case OP_constval:
                    lr->SetRematerializable(preg->rematInfo.mirConst);
                    break;
                case OP_addrof:
                case OP_dread:
                    lr->SetRematerializable(preg->GetOp(), preg->rematInfo.sym, preg->fieldID, preg->addrUpper);
                    break;
                case OP_undef:
                    break;
                default:
                    DEBUG_ASSERT(false, "Unexpected op in Preg");
            }
        }
    }

    return lr;
}

void GraphColorRegAllocator::CreateLiveRange(regno_t regNO, const BB &bb, bool isDef, uint32 currId, bool updateCount)
{
    bool isNonLocal = CreateLiveRangeHandleLocal(regNO, bb, isDef);

    if (!isDef) {
        --currId;
    }

    LiveRange *lr = CreateLiveRangeAllocateAndUpdate(regNO, bb, isDef, currId);
    lr->SetRegNO(regNO);
    lr->SetIsNonLocal(isNonLocal);
    if (isDef) {
        (void)vregLive.erase(regNO);
#ifdef OPTIMIZE_FOR_PROLOG
        if (doOptProlog && updateCount) {
            if (lr->GetNumDefs() == 0) {
                lr->SetFrequency(lr->GetFrequency() + bb.GetFrequency());
            }
            lr->IncNumDefs();
        }
#endif /* OPTIMIZE_FOR_PROLOG */
    } else {
        (void)vregLive.insert(regNO);
#ifdef OPTIMIZE_FOR_PROLOG
        if (doOptProlog && updateCount) {
            if (lr->GetNumUses() == 0) {
                lr->SetFrequency(lr->GetFrequency() + bb.GetFrequency());
            }
            lr->IncNumUses();
        }
#endif /* OPTIMIZE_FOR_PROLOG */
    }
    for (const auto &pregNO : pregLive) {
        lr->InsertElemToPregveto(pregNO);
    }

    /* only handle it in live_in and def point? */
    uint32 bbID = bb.GetId();
    lr->SetMemberBitArrElem(bbID);

    lrMap[regNO] = lr;
}

bool GraphColorRegAllocator::SetupLiveRangeByOpHandlePhysicalReg(const RegOperand &regOpnd, Insn &insn, regno_t regNO,
                                                                 bool isDef)
{
    if (!regOpnd.IsPhysicalRegister()) {
        return false;
    }
    LocalRaInfo *lraInfo = localRegVec[insn.GetBB()->GetId()];
    if (lraInfo == nullptr) {
        lraInfo = memPool->New<LocalRaInfo>(alloc);
        localRegVec[insn.GetBB()->GetId()] = lraInfo;
    }

    if (isDef) {
        if (FindNotIn(pregLive, regNO)) {
            for (const auto &vRegNO : vregLive) {
                if (IsUnconcernedReg(vRegNO)) {
                    continue;
                }
                lrMap[vRegNO]->InsertElemToPregveto(regNO);
            }
        }
        pregLive.erase(regNO);
        if (lraInfo != nullptr) {
            lraInfo->SetDefCntElem(regNO, lraInfo->GetDefCntElem(regNO) + 1);
        }
    } else {
        (void)pregLive.insert(regNO);
        for (const auto &vregNO : vregLive) {
            if (IsUnconcernedReg(vregNO)) {
                continue;
            }
            LiveRange *lr = lrMap[vregNO];
            lr->InsertElemToPregveto(regNO);
        }

        if (lraInfo != nullptr) {
            lraInfo->SetUseCntElem(regNO, lraInfo->GetUseCntElem(regNO) + 1);
        }
    }
    return true;
}

/*
 *  add pregs to forbidden list of lr. If preg is in
 *  the live list, then it is forbidden for other vreg on the list.
 */
void GraphColorRegAllocator::SetupLiveRangeByOp(Operand &op, Insn &insn, bool isDef, uint32 &numUses)
{
    if (!op.IsRegister()) {
        return;
    }
    auto &regOpnd = static_cast<RegOperand &>(op);
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (IsUnconcernedReg(regOpnd)) {
        if (GetLiveRange(regNO) != nullptr) {
            DEBUG_ASSERT(false, "Unconcerned reg");
            lrMap.erase(regNO);
        }
        return;
    }
    if (SetupLiveRangeByOpHandlePhysicalReg(regOpnd, insn, regNO, isDef)) {
        return;
    }

    CreateLiveRange(regNO, *insn.GetBB(), isDef, insn.GetId(), true);

    LiveRange *lr = GetLiveRange(regNO);
    DEBUG_ASSERT(lr != nullptr, "lr should not be nullptr");
    if (isDef) {
        lr->SetSpillSize((regOpnd.GetSize() <= k32) ? k32 : k64);
    }
    if (lr->GetRegType() == kRegTyUndef) {
        lr->SetRegType(regOpnd.GetRegisterType());
    }
    if (isDef) {
        lr->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->IncDefNum();
        lr->AddRef(insn.GetBB()->GetId(), insn.GetId(), kIsDef);
    } else {
        lr->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->IncUseNum();
        lr->AddRef(insn.GetBB()->GetId(), insn.GetId(), kIsUse);
        ++numUses;
    }
#ifdef MOVE_COALESCE
    if (insn.GetMachineOpcode() == MOP_xmovrr || insn.GetMachineOpcode() == MOP_wmovrr) {
        RegOperand &opnd1 = static_cast<RegOperand &>(insn.GetOperand(1));
        if (opnd1.GetRegisterNumber() < kAllRegNum && !IsUnconcernedReg(opnd1)) {
            lr->InsertElemToPrefs(opnd1.GetRegisterNumber() - R0);
        }
        RegOperand &opnd0 = static_cast<RegOperand &>(insn.GetOperand(0));
        if (opnd0.GetRegisterNumber() < kAllRegNum) {
            lr->InsertElemToPrefs(opnd0.GetRegisterNumber() - R0);
        }
    }
#endif /*  MOVE_COALESCE */
    if (!insn.IsSpecialIntrinsic() && insn.GetBothDefUseOpnd() != kInsnMaxOpnd) {
        lr->SetDefUse();
    }
}

/* handle live range for bb->live_out */
void GraphColorRegAllocator::SetupLiveRangeByRegNO(regno_t liveOut, BB &bb, uint32 currPoint)
{
    if (IsUnconcernedReg(liveOut)) {
        return;
    }
    if (liveOut >= kAllRegNum) {
        (void)vregLive.insert(liveOut);
        CreateLiveRange(liveOut, bb, false, currPoint, false);
        return;
    }

    (void)pregLive.insert(liveOut);
    for (const auto &vregNO : vregLive) {
        LiveRange *lr = lrMap[vregNO];
        lr->InsertElemToPregveto(liveOut);
    }

    /* See if phys reg is livein also. Then assume it span the entire bb. */
    if (!FindIn(bb.GetLiveInRegNO(), liveOut)) {
        return;
    }
    LocalRaInfo *lraInfo = localRegVec[bb.GetId()];
    if (lraInfo == nullptr) {
        lraInfo = memPool->New<LocalRaInfo>(alloc);
        localRegVec[bb.GetId()] = lraInfo;
    }
    /* Make it a large enough so no locals can be allocated. */
    lraInfo->SetUseCntElem(liveOut, kMaxUint16);
}

void GraphColorRegAllocator::ClassifyOperand(std::unordered_set<regno_t> &pregs, std::unordered_set<regno_t> &vregs,
                                             const Operand &opnd) const
{
    if (!opnd.IsRegister()) {
        return;
    }
    auto &regOpnd = static_cast<const RegOperand &>(opnd);
    regno_t regNO = regOpnd.GetRegisterNumber();
    if (IsUnconcernedReg(regNO)) {
        return;
    }
    if (regOpnd.IsPhysicalRegister()) {
        (void)pregs.insert(regNO);
    } else {
        (void)vregs.insert(regNO);
    }
}

void GraphColorRegAllocator::ComputeLiveRangesForEachDefOperand(Insn &insn, bool &multiDef)
{
    uint32 numDefs = 0;
    uint32 numUses = 0;
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        if (insn.GetMachineOpcode() == MOP_asm && (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd)) {
            for (auto opnd : static_cast<ListOperand &>(insn.GetOperand(i)).GetOperands()) {
                SetupLiveRangeByOp(*static_cast<RegOperand *>(opnd), insn, true, numUses);
                ++numDefs;
            }
            continue;
        }
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            if (!memOpnd.IsIntactIndexed()) {
                SetupLiveRangeByOp(opnd, insn, true, numUses);
                ++numDefs;
            }
        }
        if (!md->GetOpndDes(i)->IsRegDef()) {
            continue;
        }
        SetupLiveRangeByOp(opnd, insn, true, numUses);
        ++numDefs;
    }
    DEBUG_ASSERT(numUses == 0, "should only be def opnd");
    if (numDefs > 1) {
        multiDef = true;
        needExtraSpillReg = true;
    }
}

void GraphColorRegAllocator::ComputeLiveRangesForEachUseOperand(Insn &insn)
{
    uint32 numUses = 0;
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        if (insn.GetMachineOpcode() == MOP_asm && i == kAsmInputListOpnd) {
            for (auto opnd : static_cast<ListOperand &>(insn.GetOperand(i)).GetOperands()) {
                SetupLiveRangeByOp(*static_cast<RegOperand *>(opnd), insn, false, numUses);
            }
            continue;
        }
        if (md->GetOpndDes(i)->IsRegDef() && !md->GetOpndDes(i)->IsRegUse()) {
            continue;
        }
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsList()) {
            auto &listOpnd = static_cast<ListOperand &>(opnd);
            for (auto op : listOpnd.GetOperands()) {
                SetupLiveRangeByOp(*op, insn, false, numUses);
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            Operand *offset = memOpnd.GetIndexRegister();
            if (base != nullptr) {
                SetupLiveRangeByOp(*base, insn, false, numUses);
            }
            if (offset != nullptr) {
                SetupLiveRangeByOp(*offset, insn, false, numUses);
            }
        } else {
            SetupLiveRangeByOp(opnd, insn, false, numUses);
        }
    }
    if (numUses >= AArch64Abi::kNormalUseOperandNum || insn.GetMachineOpcode() == MOP_lazy_ldr) {
        needExtraSpillReg = true;
    }
}

void GraphColorRegAllocator::ComputeLiveRangesUpdateIfInsnIsCall(const Insn &insn)
{
    if (!insn.IsCall()) {
        return;
    }
    /* def the return value */
    pregLive.erase(R0);
    pregLive.erase(V0);

    /* active the parametes */
    Operand &opnd1 = insn.GetOperand(1);
    if (opnd1.IsList()) {
        auto &srcOpnds = static_cast<ListOperand &>(opnd1);
        for (auto regOpnd : srcOpnds.GetOperands()) {
            DEBUG_ASSERT(!regOpnd->IsVirtualRegister(), "not be a virtual register");
            auto physicalReg = static_cast<AArch64reg>(regOpnd->GetRegisterNumber());
            (void)pregLive.insert(physicalReg);
        }
    }
}

void GraphColorRegAllocator::ComputeLiveRangesUpdateLiveUnitInsnRange(BB &bb, uint32 currPoint)
{
    for (auto lin : bb.GetLiveInRegNO()) {
        if (lin < kAllRegNum) {
            continue;
        }
        LiveRange *lr = GetLiveRange(lin);
        if (lr == nullptr) {
            continue;
        }
        auto lu = lr->FindInLuMap(bb.GetId());
        DEBUG_ASSERT(lu != lr->EndOfLuMap(), "container empty check");
        if (bb.GetFirstInsn()) {
            lu->second->SetBegin(bb.GetFirstInsn()->GetId());
        } else {
            /* since bb is empty, then use pointer as is */
            lu->second->SetBegin(currPoint);
        }
        CHECK_FATAL(lu->second->GetBegin() >= 1, "value overflow");
        lu->second->SetBegin(lu->second->GetBegin() - 1);
    }
}

bool GraphColorRegAllocator::UpdateInsnCntAndSkipUseless(Insn &insn, uint32 &currPoint) const
{
    insn.SetId(currPoint);
    if (insn.IsImmaterialInsn() || !insn.IsMachineInstruction()) {
        --currPoint;
        return true;
    }
    return false;
}

void GraphColorRegAllocator::UpdateCallInfo(uint32 bbId, uint32 currPoint, const Insn &insn)
{
    auto *targetOpnd = insn.GetCallTargetOperand();
    CHECK_FATAL(targetOpnd != nullptr, "target is null in Insn::IsCallToFunctionThatNeverReturns");
    if (CGOptions::DoIPARA() && targetOpnd->IsFuncNameOpnd()) {
        FuncNameOperand *target = static_cast<FuncNameOperand *>(targetOpnd);
        const MIRSymbol *funcSt = target->GetFunctionSymbol();
        DEBUG_ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
        MIRFunction *func = funcSt->GetFunction();
        if (func != nullptr && func->IsReferedRegsValid()) {
            for (auto preg : func->GetReferedRegs()) {
                if (AArch64Abi::IsCallerSaveReg(static_cast<AArch64reg>(preg))) {
                    for (auto vregNO : vregLive) {
                        LiveRange *lr = lrMap[vregNO];
                        lr->InsertElemToCallDef(preg);
                    }
                }
            }
        } else {
            for (auto vregNO : vregLive) {
                LiveRange *lr = lrMap[vregNO];
                lr->SetCrossCall();
            }
        }
    } else {
        for (auto vregNO : vregLive) {
            LiveRange *lr = lrMap[vregNO];
            lr->SetCrossCall();
        }
    }
    for (auto vregNO : vregLive) {
        LiveRange *lr = lrMap[vregNO];
        lr->IncNumCall();
        lr->AddRef(bbId, currPoint, kIsCall);

        auto lu = lr->FindInLuMap(bbId);
        if (lu != lr->EndOfLuMap()) {
            lu->second->SetHasCall(true);
        }
    }
}

void GraphColorRegAllocator::SetLrMustAssign(const RegOperand *regOpnd)
{
    regno_t regNO = regOpnd->GetRegisterNumber();
    LiveRange *lr = GetLiveRange(regNO);
    if (lr != nullptr) {
        lr->SetMustAssigned();
        lr->SetIsNonLocal(true);
    }
}

void GraphColorRegAllocator::SetupMustAssignedLiveRanges(const Insn &insn)
{
    if (!insn.IsSpecialIntrinsic()) {
        return;
    }
    if (insn.GetMachineOpcode() == MOP_asm) {
        for (auto regOpnd : static_cast<ListOperand &>(insn.GetOperand(kAsmOutputListOpnd)).GetOperands()) {
            SetLrMustAssign(regOpnd);
        }
        for (auto regOpnd : static_cast<ListOperand &>(insn.GetOperand(kAsmInputListOpnd)).GetOperands()) {
            SetLrMustAssign(regOpnd);
        }
        return;
    }
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand *opnd = &insn.GetOperand(i);
        if (!opnd->IsRegister()) {
            continue;
        }
        auto regOpnd = static_cast<RegOperand *>(opnd);
        SetLrMustAssign(regOpnd);
    }
}

/* Create a common stack space for spilling with need_spill */
MemOperand *GraphColorRegAllocator::CreateSpillMem(uint32 spillIdx, SpillMemCheck check)
{
    if (spillIdx >= spillMemOpnds.size()) {
        return nullptr;
    }

    if (operandSpilled[spillIdx]) {
        /* For this insn, spill slot already used, need to find next available slot. */
        uint32 i;
        for (i = spillIdx + 1; i < kSpillMemOpndNum; ++i) {
            if (!operandSpilled[i]) {
                break;
            }
        }
        CHECK_FATAL(i < kSpillMemOpndNum, "no more available spill mem slot");
        spillIdx = i;
    }
    if (check == kSpillMemPost) {
        operandSpilled[spillIdx] = true;
    }

    if (spillMemOpnds[spillIdx] == nullptr) {
        regno_t reg = cgFunc->NewVReg(kRegTyInt, sizeof(int64));
        auto *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
        spillMemOpnds[spillIdx] = a64CGFunc->GetOrCreatSpillMem(reg);
    }
    return spillMemOpnds[spillIdx];
}

bool GraphColorRegAllocator::IsLocalReg(regno_t regNO) const
{
    LiveRange *lr = GetLiveRange(regNO);
    if (lr == nullptr) {
        LogInfo::MapleLogger() << "unexpected regNO" << regNO;
        return true;
    }
    return IsLocalReg(*lr);
}

bool GraphColorRegAllocator::IsLocalReg(const LiveRange &lr) const
{
    return !lr.GetSplitLr() && (lr.GetNumBBMembers() == 1) && !lr.IsNonLocal();
}

bool GraphColorRegAllocator::CheckOverlap(uint64 val, uint32 i, LiveRange &lr1, LiveRange &lr2) const
{
    regno_t lr1RegNO = lr1.GetRegNO();
    regno_t lr2RegNO = lr2.GetRegNO();
    for (uint32 x = 0; x < kU64; ++x) {
        if ((val & (1ULL << x)) != 0) {
            uint32 lastBitSet = i * kU64 + x;
            /*
             * begin and end should be in the bb info (LU)
             * Need to rethink this if.
             * Under some circumstance, lr->begin can occur after lr->end.
             */
            auto lu1 = lr1.FindInLuMap(lastBitSet);
            auto lu2 = lr2.FindInLuMap(lastBitSet);
            if (lu1 != lr1.EndOfLuMap() && lu2 != lr2.EndOfLuMap() &&
                !((lu1->second->GetBegin() < lu2->second->GetBegin() &&
                   lu1->second->GetEnd() < lu2->second->GetBegin()) ||
                  (lu2->second->GetBegin() < lu1->second->GetEnd() &&
                   lu2->second->GetEnd() < lu1->second->GetBegin()))) {
                lr1.SetConflictBitArrElem(lr2RegNO);
                lr2.SetConflictBitArrElem(lr1RegNO);
                return true;
            }
        }
    }
    return false;
}

void GraphColorRegAllocator::CheckInterference(LiveRange &lr1, LiveRange &lr2) const
{
    uint64 bitArr[bbBuckets];
    for (uint32 i = 0; i < bbBuckets; ++i) {
        bitArr[i] = lr1.GetBBMember()[i] & lr2.GetBBMember()[i];
    }

    for (uint32 i = 0; i < bbBuckets; ++i) {
        uint64 val = bitArr[i];
        if (val == 0) {
            continue;
        }
        if (CheckOverlap(val, i, lr1, lr2)) {
            break;
        }
    }
}

void GraphColorRegAllocator::SetBBInfoGlobalAssigned(uint32 bbID, regno_t regNO)
{
    DEBUG_ASSERT(bbID < bbRegInfo.size(), "index out of range in GraphColorRegAllocator::SetBBInfoGlobalAssigned");
    BBAssignInfo *bbInfo = bbRegInfo[bbID];
    if (bbInfo == nullptr) {
        bbInfo = memPool->New<BBAssignInfo>(alloc);
        bbRegInfo[bbID] = bbInfo;
        bbInfo->InitGlobalAssigned();
    }
    bbInfo->InsertElemToGlobalsAssigned(regNO);
}

bool GraphColorRegAllocator::HaveAvailableColor(const LiveRange &lr, uint32 num) const
{
    return ((lr.GetRegType() == kRegTyInt && num < intRegNum) || (lr.GetRegType() == kRegTyFloat && num < fpRegNum));
}

MapleVector<LiveRange *>::iterator GraphColorRegAllocator::GetHighPriorityLr(MapleVector<LiveRange *> &lrSet) const
{
    auto it = lrSet.begin();
    auto highestIt = it;
    LiveRange *startLr = *it;
    float maxPrio = startLr->GetPriority();
    ++it;
    for (; it != lrSet.end(); ++it) {
        LiveRange *lr = *it;
        if (lr->GetPriority() > maxPrio) {
            maxPrio = lr->GetPriority();
            highestIt = it;
        }
    }
    return highestIt;
}

void GraphColorRegAllocator::UpdateForbiddenForNeighbors(const LiveRange &lr) const
{
    auto updateForbidden = [&lr, this](regno_t regNO) {
        LiveRange *newLr = GetLiveRange(regNO);
        DEBUG_ASSERT(newLr != nullptr, "newLr should not be nullptr");
        if (!newLr->GetPregveto(lr.GetAssignedRegNO())) {
            newLr->InsertElemToForbidden(lr.GetAssignedRegNO());
        }
    };
    ForEachRegArrElem(lr.GetBBConflict(), updateForbidden);
}

void GraphColorRegAllocator::UpdatePregvetoForNeighbors(const LiveRange &lr) const
{
    auto updatePregveto = [&lr, this](regno_t regNO) {
        LiveRange *newLr = GetLiveRange(regNO);
        DEBUG_ASSERT(newLr != nullptr, "newLr should not be nullptr");
        newLr->InsertElemToPregveto(lr.GetAssignedRegNO());
        newLr->EraseElemFromForbidden(lr.GetAssignedRegNO());
    };
    ForEachRegArrElem(lr.GetBBConflict(), updatePregveto);
}

/*
 *  For cases with only one def/use and crosses a call.
 *  It might be more beneficial to spill vs save/restore in prolog/epilog.
 *  But if the callee register is already used, then it is ok to reuse it again.
 *  Or in certain cases, just use the callee.
 */
bool GraphColorRegAllocator::ShouldUseCallee(LiveRange &lr, const MapleSet<regno_t> &calleeUsed,
                                             const MapleVector<LiveRange *> &delayed) const
{
    if (FindIn(calleeUsed, lr.GetAssignedRegNO())) {
        return true;
    }
    if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(lr.GetAssignedRegNO())) &&
        (calleeUsed.size() % kDivide2) != 0) {
        return true;
    }
    if (delayed.size() > 1 && calleeUsed.empty()) {
        /* If there are more than 1 vreg that can benefit from callee, use callee */
        return true;
    }
    lr.SetAssignedRegNO(0);
    return false;
}

void GraphColorRegAllocator::AddCalleeUsed(regno_t regNO, RegType regType)
{
    DEBUG_ASSERT(AArch64isa::IsPhysicalRegister(regNO), "regNO should be physical register");
    bool isCalleeReg = AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regNO));
    if (isCalleeReg) {
        if (regType == kRegTyInt) {
            (void)intCalleeUsed.insert(regNO);
        } else {
            (void)fpCalleeUsed.insert(regNO);
        }
    }
}

regno_t GraphColorRegAllocator::TryToAssignCallerSave(const LiveRange &lr) const
{
    regno_t base;
    RegType regType = lr.GetRegType();
    const MapleSet<uint32> *currRegSet = nullptr;
    if (regType == kRegTyInt) {
        currRegSet = &intCallerRegSet;
        base = R0;
    } else {
        currRegSet = &fpCallerRegSet;
        base = V0;
    }

    regno_t reg = 0;
#ifdef MOVE_COALESCE
    if (lr.GetNumCall() == 0 || (lr.GetNumDefs() + lr.GetNumUses() <= kRegNum2)) {
        for (const auto &it : lr.GetPrefs()) {
            reg = it + base;
            if ((FindIn(*currRegSet, reg)) && !lr.GetForbidden(reg) && !lr.GetPregveto(reg) && !lr.GetCallDef(reg)) {
                return reg;
            }
        }
    }
#endif /*  MOVE_COALESCE */
    for (const auto &it : *currRegSet) {
        reg = it + base;
        if (!lr.GetForbidden(reg) && !lr.GetPregveto(reg) && !lr.GetCallDef(reg)) {
            return reg;
        }
    }
    return 0;
}

/*
 * If forbidden list has more registers than max of all BB's local reg
 *  requirement, then LR can be colored.
 *  Update LR's color if success, return true, else return false.
 */
bool GraphColorRegAllocator::AssignColorToLr(LiveRange &lr, bool isDelayed)
{
    if (lr.GetAssignedRegNO() > 0) {
        /* Already assigned. */
        return true;
    }
    if (!HaveAvailableColor(lr, lr.GetForbiddenSize() + lr.GetPregvetoSize())) {
        if (needDump) {
            LogInfo::MapleLogger() << "assigned fail to R" << lr.GetRegNO() << "\n";
        }
        return false;
    }
    if (needDump) {
        LogInfo::MapleLogger() << "assigned " << lr.GetAssignedRegNO() << " to R" << lr.GetRegNO() << "\n";
    }
    if (lr.GetAssignedRegNO() == 0) {
        return false;
    }
#ifdef OPTIMIZE_FOR_PROLOG
    if (doOptProlog && isDelayed) {
        if ((lr.GetRegType() == kRegTyInt && !ShouldUseCallee(lr, intCalleeUsed, intDelayed)) ||
            (lr.GetRegType() == kRegTyFloat && !ShouldUseCallee(lr, fpCalleeUsed, fpDelayed))) {
            return false;
        }
    }
#endif /* OPTIMIZE_FOR_PROLOG */

    AddCalleeUsed(lr.GetAssignedRegNO(), lr.GetRegType());

    UpdateForbiddenForNeighbors(lr);
    ForEachBBArrElem(lr.GetBBMember(),
                     [&lr, this](uint32 bbID) { SetBBInfoGlobalAssigned(bbID, lr.GetAssignedRegNO()); });
    return true;
}

void GraphColorRegAllocator::PruneLrForSplit(LiveRange &lr, BB &bb, bool remove,
                                             std::set<LoopDesc*, LoopDesc::LoopDescCmp> &candidateInLoop,
                                             std::set<LoopDesc*, LoopDesc::LoopDescCmp> &defInLoop)
{
    if (bb.GetInternalFlag1()) {
        /* already visited */
        return;
    }

    bb.SetInternalFlag1(true);
    auto lu = lr.FindInLuMap(bb.GetId());
    uint32 defNum = 0;
    uint32 useNum = 0;
    if (lu != lr.EndOfLuMap()) {
        defNum = lu->second->GetDefNum();
        useNum = lu->second->GetUseNum();
    }

    auto *loop = loopInfo.GetBBLoopParent(bb.GetId());
    if (remove) {
        /* In removal mode, has not encountered a ref yet. */
        if (defNum == 0 && useNum == 0) {
            if (loop != nullptr && FindIn(candidateInLoop, loop)) {
                /*
                 * Upward search has found a loop.  Regardless of def/use
                 *  The loop members must be included in the new LR.
                 */
                remove = false;
            } else {
                /* No ref in this bb. mark as potential remove. */
                bb.SetInternalFlag2(true);
                return;
            }
        } else {
            /* found a ref, no more removal of bb and preds. */
            remove = false;
        }
    }

    if (loop != nullptr) {
        /* With a def in loop, cannot prune that loop */
        if (defNum > 0) {
            (void)defInLoop.insert(loop);
        }
        /* bb in loop, need to make sure of loop carried dependency */
        (void)candidateInLoop.insert(loop);
    }
    for (auto pred : bb.GetPreds()) {
        if (loop == nullptr || !loop->Has(*pred)) {
            PruneLrForSplit(lr, *pred, remove, candidateInLoop, defInLoop);
        }
    }
}

void GraphColorRegAllocator::FindBBSharedInSplit(LiveRange &lr,
                                                 const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &candidateInLoop,
                                                 std::set<LoopDesc*, LoopDesc::LoopDescCmp> &defInLoop)
{
    /* A loop might be split into two.  Need to see over the entire LR if there is a def in the loop. */
    auto FindBBSharedFunc = [&lr, &candidateInLoop, &defInLoop, this](uint32 bbID) {
        BB *bb = bbVec[bbID];
        auto *loop = loopInfo.GetBBLoopParent(bb->GetId());
        if (loop != nullptr && FindIn(candidateInLoop, loop)) {
            auto lu = lr.FindInLuMap(bb->GetId());
            if (lu != lr.EndOfLuMap() && lu->second->GetDefNum() > 0) {
                (void)defInLoop.insert(loop);
            }
        }
    };
    ForEachBBArrElem(lr.GetBBMember(), FindBBSharedFunc);
}

/*
 *  Backward traversal of the top part of the split LR.
 *  Prune the part of the LR that has no downward exposing references.
 *  Take into account of loops and loop carried dependencies.
 *  The candidate bb to be removed, if in a loop, store that info.
 *  If a LR crosses a loop, even if the loop has no def/use, it must
 *  be included in the new LR.
 */
void GraphColorRegAllocator::ComputeBBForNewSplit(LiveRange &newLr, LiveRange &origLr)
{
    /*
     *  The candidate bb to be removed, if in a loop, store that info.
     *  If a LR crosses a loop, even if the loop has no def/use, it must
     *  be included in the new LR.
     */
    std::set<LoopDesc*, LoopDesc::LoopDescCmp> candidateInLoop;
    /* If a bb has a def and is in a loop, store that info. */
    std::set<LoopDesc*, LoopDesc::LoopDescCmp> defInLoop;
    std::set<BB *, SortedBBCmpFunc> smember;
    ForEachBBArrElem(newLr.GetBBMember(), [this, &smember](uint32 bbID) { (void)smember.insert(bbVec[bbID]); });
    for (auto bbIt = smember.rbegin(); bbIt != smember.rend(); ++bbIt) {
        BB *bb = *bbIt;
        if (bb->GetInternalFlag1() != 0) {
            continue;
        }
        PruneLrForSplit(newLr, *bb, true, candidateInLoop, defInLoop);
    }
    FindBBSharedInSplit(origLr, candidateInLoop, defInLoop);
    auto pruneTopLr = [this, &newLr, &candidateInLoop, &defInLoop](uint32 bbID) {
        BB *bb = bbVec[bbID];
        if (bb->GetInternalFlag2() != 0) {
            auto *loop = loopInfo.GetBBLoopParent(bb->GetId());
            if (loop != nullptr && FindIn(candidateInLoop, loop)) {
                return;
            }
            if (loop != nullptr || FindNotIn(defInLoop, loop)) {
                /* defInLoop should be a subset of candidateInLoop.  remove. */
                newLr.UnsetMemberBitArrElem(bbID);
            }
        }
    };
    ForEachBBArrElem(newLr.GetBBMember(), pruneTopLr); /* prune the top LR. */
}

bool GraphColorRegAllocator::UseIsUncovered(const BB &bb, const BB &startBB, std::vector<bool> &visitedBB)
{
    CHECK_FATAL(bb.GetId() < visitedBB.size(), "index out of range");
    visitedBB[bb.GetId()] = true;
    for (auto pred : bb.GetPreds()) {
        if (visitedBB[pred->GetId()]) {
            continue;
        }
        if (pred->GetLevel() <= startBB.GetLevel()) {
            return true;
        }
        if (UseIsUncovered(*pred, startBB, visitedBB)) {
            return true;
        }
    }
    return false;
}

void GraphColorRegAllocator::FindUseForSplit(LiveRange &lr, SplitBBInfo &bbInfo, bool &remove,
                                             std::set<LoopDesc*, LoopDesc::LoopDescCmp> &candidateInLoop,
                                             std::set<LoopDesc*, LoopDesc::LoopDescCmp> &defInLoop)
{
    BB *bb = bbInfo.GetCandidateBB();
    const BB *startBB = bbInfo.GetStartBB();
    if (bb->GetInternalFlag1() != 0) {
        /* already visited */
        return;
    }
    for (auto pred : bb->GetPreds()) {
        if (pred->GetInternalFlag1() == 0) {
            return;
        }
    }

    bb->SetInternalFlag1(true);
    auto lu = lr.FindInLuMap(bb->GetId());
    uint32 defNum = 0;
    uint32 useNum = 0;
    if (lu != lr.EndOfLuMap()) {
        defNum = lu->second->GetDefNum();
        useNum = lu->second->GetUseNum();
    }

    std::vector<bool> visitedBB(cgFunc->GetAllBBs().size(), false);
    auto *loop = loopInfo.GetBBLoopParent(bb->GetId());
    if (remove) {
        /* In removal mode, has not encountered a ref yet. */
        if (defNum == 0 && useNum == 0) {
            /* No ref in this bb. mark as potential remove. */
            bb->SetInternalFlag2(true);
            if (loop != nullptr) {
                /* bb in loop, need to make sure of loop carried dependency */
                (void)candidateInLoop.insert(loop);
            }
        } else {
            /* found a ref, no more removal of bb and preds. */
            remove = false;
            /* A potential point for a upward exposing use. (might be a def). */
            lu->second->SetNeedReload(true);
        }
    } else if ((defNum > 0 || useNum > 0) && UseIsUncovered(*bb, *startBB, visitedBB)) {
        lu->second->SetNeedReload(true);
    }

    /* With a def in loop, cannot prune that loop */
    if (loop != nullptr && defNum > 0) {
        (void)defInLoop.insert(loop);
    }

    for (auto succ : bb->GetSuccs()) {
        if (loop && loop->Has(*succ)) {
            bbInfo.SetCandidateBB(*succ);
            FindUseForSplit(lr, bbInfo, remove, candidateInLoop, defInLoop);
        }
    }
}

void GraphColorRegAllocator::ClearLrBBFlags(const std::set<BB *, SortedBBCmpFunc> &member) const
{
    for (auto bb : member) {
        bb->SetInternalFlag1(0);
        bb->SetInternalFlag2(0);
        for (auto pred : bb->GetPreds()) {
            pred->SetInternalFlag1(0);
            pred->SetInternalFlag2(0);
        }
    }
}

/*
 *  Downward traversal of the bottom part of the split LR.
 *  Prune the part of the LR that has no upward exposing references.
 *  Take into account of loops and loop carried dependencies.
 */
void GraphColorRegAllocator::ComputeBBForOldSplit(LiveRange &newLr, LiveRange &origLr)
{
    /* The candidate bb to be removed, if in a loop, store that info. */
    std::set<LoopDesc*, LoopDesc::LoopDescCmp> candidateInLoop;
    /* If a bb has a def and is in a loop, store that info. */
    std::set<LoopDesc*, LoopDesc::LoopDescCmp> defInLoop;
    SplitBBInfo bbInfo;
    bool remove = true;

    std::set<BB *, SortedBBCmpFunc> smember;
    ForEachBBArrElem(origLr.GetBBMember(), [this, &smember](uint32 bbID) { (void)smember.insert(bbVec[bbID]); });
    ClearLrBBFlags(smember);
    for (auto bb : smember) {
        if (bb->GetInternalFlag1() != 0) {
            continue;
        }
        for (auto pred : bb->GetPreds()) {
            pred->SetInternalFlag1(true);
        }
        bbInfo.SetCandidateBB(*bb);
        bbInfo.SetStartBB(*bb);
        FindUseForSplit(origLr, bbInfo, remove, candidateInLoop, defInLoop);
    }
    FindBBSharedInSplit(newLr, candidateInLoop, defInLoop);
    auto pruneLrFunc = [&origLr, &defInLoop, this](uint32 bbID) {
        BB *bb = bbVec[bbID];
        if (bb->GetInternalFlag2() != 0) {
            auto *loop = loopInfo.GetBBLoopParent(bb->GetId());
            if (loop != nullptr && FindNotIn(defInLoop, loop)) {
                origLr.UnsetMemberBitArrElem(bbID);
            }
        }
    };
    ForEachBBArrElem(origLr.GetBBMember(), pruneLrFunc);
}

/*
 *  There is at least one available color for this BB from the neighbors
 *  minus the ones reserved for local allocation.
 *  bbAdded : The new BB to be added into the split LR if color is available.
 *  conflictRegs : Reprent the LR before adding the bbAdded.  These are the
 *                 forbidden regs before adding the new BBs.
 *  Side effect : Adding the new forbidden regs from bbAdded into
 *                conflictRegs if the LR can still be colored.
 */
bool GraphColorRegAllocator::LrCanBeColored(const LiveRange &lr, const BB &bbAdded,
                                            std::unordered_set<regno_t> &conflictRegs)
{
    RegType type = lr.GetRegType();

    std::unordered_set<regno_t> newConflict;
    auto updateConflictFunc = [&bbAdded, &conflictRegs, &newConflict, &lr, this](regno_t regNO) {
        /* check the real conflict in current bb */
        LiveRange *conflictLr = lrMap[regNO];
        /*
         *  If the bb to be added to the new LR has an actual
         *  conflict with another LR, and if that LR has already
         *  assigned a color that is not in the conflictRegs,
         *  then add it as a newConflict.
         */
        if (IsBitArrElemSet(conflictLr->GetBBMember(), bbAdded.GetId())) {
            regno_t confReg = conflictLr->GetAssignedRegNO();
            if ((confReg > 0) && FindNotIn(conflictRegs, confReg) && !lr.GetPregveto(confReg)) {
                (void)newConflict.insert(confReg);
            }
        } else if (conflictLr->GetSplitLr() != nullptr &&
                   IsBitArrElemSet(conflictLr->GetSplitLr()->GetBBMember(), bbAdded.GetId())) {
            /*
             * The after split LR is split into pieces, and this ensures
             * the after split color is taken into consideration.
             */
            regno_t confReg = conflictLr->GetSplitLr()->GetAssignedRegNO();
            if ((confReg > 0) && FindNotIn(conflictRegs, confReg) && !lr.GetPregveto(confReg)) {
                (void)newConflict.insert(confReg);
            }
        }
    };
    ForEachRegArrElem(lr.GetBBConflict(), updateConflictFunc);

    size_t numRegs = newConflict.size() + lr.GetPregvetoSize() + conflictRegs.size();

    bool canColor = false;
    if (type == kRegTyInt) {
        if (numRegs < intRegNum) {
            canColor = true;
        }
    } else if (numRegs < fpRegNum) {
        canColor = true;
    }

    if (canColor) {
        for (auto regNO : newConflict) {
            (void)conflictRegs.insert(regNO);
        }
    }

    /* Update all the registers conflicting when adding thew new bb. */
    return canColor;
}

/* Support function for LR split.  Move one BB from LR1 to LR2. */
void GraphColorRegAllocator::MoveLrBBInfo(LiveRange &oldLr, LiveRange &newLr, BB &bb) const
{
    /* initialize backward traversal flag for the bb pruning phase */
    bb.SetInternalFlag1(false);
    /* initialize bb removal marker */
    bb.SetInternalFlag2(false);
    /* Insert BB into new LR */
    uint32 bbID = bb.GetId();
    newLr.SetMemberBitArrElem(bbID);

    /* Move LU from old LR to new LR */
    auto luIt = oldLr.FindInLuMap(bb.GetId());
    if (luIt != oldLr.EndOfLuMap()) {
        newLr.SetElemToLuMap(luIt->first, *(luIt->second));
        oldLr.EraseLuMap(luIt);
    }

    /* Remove BB from old LR */
    oldLr.UnsetMemberBitArrElem(bbID);
}

/* Is the set of loops inside the loop? */
bool GraphColorRegAllocator::ContainsLoop(const LoopDesc &loop,
                                          const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &loops) const
{
    for (const auto *lp : loops) {
        while (lp != nullptr) {
            if (lp == &loop) {
                return true;
            }
            lp = lp->GetParentLoop();
        }
    }
    return false;
}

void GraphColorRegAllocator::GetAllLrMemberLoops(LiveRange &lr, std::set<LoopDesc*, LoopDesc::LoopDescCmp> &loops)
{
    auto GetLrMemberFunc = [&loops, this](uint32 bbID) {
        auto *loop = loopInfo.GetBBLoopParent(bbID);
        if (loop != nullptr) {
            (void)loops.insert(loop);
        }
    };
    ForEachBBArrElem(lr.GetBBMember(), GetLrMemberFunc);
}

bool GraphColorRegAllocator::SplitLrShouldSplit(LiveRange &lr)
{
    if (lr.GetSplitLr() != nullptr || lr.GetNumBBMembers() == 1) {
        return false;
    }
    /* Need to split within the same hierarchy */
    uint32 loopID = 0xFFFFFFFF; /* loopID is initialized the maximum value，and then be assigned in function */
    bool needSplit = true;
    auto setNeedSplit = [&needSplit, &loopID, this](uint32 bbID) -> bool {
        auto *loop = loopInfo.GetBBLoopParent(bbID);
        if (loopID == 0xFFFFFFFF) {
            if (loop != nullptr) {
                loopID = loop->GetHeader().GetId();
            } else {
                loopID = 0;
            }
        } else if ((loop != nullptr && loop->GetHeader().GetId() != loopID) || (loop == nullptr && loopID != 0)) {
            needSplit = false;
            return true;
        }
        return false;
    };
    ForEachBBArrElemWithInterrupt(lr.GetBBMember(), setNeedSplit);
    return needSplit;
}

/*
 * When a BB in the LR has no def or use in it, then potentially
 * there is no conflict within these BB for the new LR, since
 * the new LR will need to spill the defs which terminates the
 * new LR unless there is a use later which extends the new LR.
 * There is no need to compute conflicting register set unless
 * there is a def or use.
 * It is assumed that the new LR is extended to the def or use.
 * Initially newLr is empty, then add bb if can be colored.
 * Return true if there is a split.
 */
bool GraphColorRegAllocator::SplitLrFindCandidateLr(LiveRange &lr, LiveRange &newLr,
                                                    std::unordered_set<regno_t> &conflictRegs)
{
    if (needDump) {
        LogInfo::MapleLogger() << "start split lr for vreg " << lr.GetRegNO() << "\n";
    }
    std::set<BB *, SortedBBCmpFunc> smember;
    ForEachBBArrElem(lr.GetBBMember(), [&smember, this](uint32 bbID) { (void)smember.insert(bbVec[bbID]); });
    for (auto bb : smember) {
        if (!LrCanBeColored(lr, *bb, conflictRegs)) {
            break;
        }
        MoveLrBBInfo(lr, newLr, *bb);
    }

    /* return ture if split is successful */
    return newLr.GetNumBBMembers() != 0;
}

void GraphColorRegAllocator::SplitLrHandleLoops(LiveRange &lr, LiveRange &newLr,
                                                const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &origLoops,
                                                const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &newLoops)
{
    /*
     * bb in loops might need a reload due to loop carried dependency.
     * Compute this before pruning the LRs.
     * if there is no re-definition, then reload is not necessary.
     * Part of the new LR region after the last reference is
     * no longer in the LR.  Remove those bb.
     */
    ComputeBBForNewSplit(newLr, lr);

    /* With new LR, recompute conflict. */
    auto recomputeConflict = [&lr, &newLr, this](uint32 bbID) {
        auto lrFunc = [&newLr, &bbID, this](regno_t regNO) {
            LiveRange *confLrVec = lrMap[regNO];
            if (IsBitArrElemSet(confLrVec->GetBBMember(), bbID) ||
                (confLrVec->GetSplitLr() != nullptr && IsBitArrElemSet(confLrVec->GetSplitLr()->GetBBMember(), bbID))) {
                /*
                 * New LR getting the interference does not mean the
                 * old LR can remove the interference.
                 * Old LR's interference will be handled at the end of split.
                 */
                newLr.SetConflictBitArrElem(regNO);
            }
        };
        ForEachRegArrElem(lr.GetBBConflict(), lrFunc);
    };
    ForEachBBArrElem(newLr.GetBBMember(), recomputeConflict);

    /* update bb/loop same as for new LR. */
    ComputeBBForOldSplit(newLr, lr);
    /* Update the conflict interference for the original LR later. */
    for (auto loop : newLoops) {
        if (!ContainsLoop(*loop, origLoops)) {
            continue;
        }
        for (auto bbId : loop->GetLoopBBs()) {
            if (!IsBitArrElemSet(newLr.GetBBMember(), bbId)) {
                continue;
            }
            LiveUnit *lu = newLr.GetLiveUnitFromLuMap(bbId);
            if (lu->GetUseNum() != 0) {
                lu->SetNeedReload(true);
            }
        }
    }
}

void GraphColorRegAllocator::SplitLrFixNewLrCallsAndRlod(LiveRange &newLr,
                                                         const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &origLoops)
{
    /* If a 2nd split loop is before the bb in 1st split bb. */
    newLr.SetNumCall(0);
    auto fixCallsAndRlod = [&newLr, &origLoops, this](uint32 bbID) {
        BB *bb = bbVec[bbID];
        for (auto loop : origLoops) {
            if (loop->GetHeader().GetLevel() >= bb->GetLevel()) {
                continue;
            }
            LiveUnit *lu = newLr.GetLiveUnitFromLuMap(bbID);
            if (lu->GetUseNum() != 0) {
                lu->SetNeedReload(true);
            }
        }
        LiveUnit *lu = newLr.GetLiveUnitFromLuMap(bbID);
        if (lu->HasCall()) {
            newLr.IncNumCall();
        }
    };
    ForEachBBArrElem(newLr.GetBBMember(), fixCallsAndRlod);
}

void GraphColorRegAllocator::SplitLrFixOrigLrCalls(LiveRange &lr) const
{
    lr.SetNumCall(0);
    auto fixOrigCalls = [&lr](uint32 bbID) {
        LiveUnit *lu = lr.GetLiveUnitFromLuMap(bbID);
        if (lu->HasCall()) {
            lr.IncNumCall();
        }
    };
    ForEachBBArrElem(lr.GetBBMember(), fixOrigCalls);
}

void GraphColorRegAllocator::SplitLrUpdateInterference(LiveRange &lr)
{
    /*
     * newLr is now a separate LR from the original lr.
     * Update the interference info.
     * Also recompute the forbidden info
     */
    lr.ClearForbidden();
    auto updateInterfrence = [&lr, this](regno_t regNO) {
        LiveRange *confLrVec = lrMap[regNO];
        if (IsBBsetOverlap(lr.GetBBMember(), confLrVec->GetBBMember(), bbBuckets)) {
            /* interfere */
            if (confLrVec->GetAssignedRegNO() && !lr.GetPregveto(confLrVec->GetAssignedRegNO())) {
                lr.InsertElemToForbidden(confLrVec->GetAssignedRegNO());
            }
        } else {
            /* no interference */
            lr.UnsetConflictBitArrElem(regNO);
        }
    };
    ForEachRegArrElem(lr.GetBBConflict(), updateInterfrence);
}

void GraphColorRegAllocator::SplitLrUpdateRegInfo(const LiveRange &origLr, LiveRange &newLr,
                                                  std::unordered_set<regno_t> &conflictRegs) const
{
    for (regno_t regNO = kInvalidRegNO; regNO < kMaxRegNum; ++regNO) {
        if (origLr.GetPregveto(regNO)) {
            newLr.InsertElemToPregveto(regNO);
        }
    }
    for (auto regNO : conflictRegs) {
        if (!newLr.GetPregveto(regNO)) {
            newLr.InsertElemToForbidden(regNO);
        }
    }
}

void GraphColorRegAllocator::SplitLrErrorCheckAndDebug(const LiveRange &origLr) const
{
    if (origLr.GetNumBBMembers() == 0) {
        DEBUG_ASSERT(origLr.GetNumBBConflicts() == 0, "Error: member and conflict not match");
    }
}

/*
 * Pick a starting BB, then expand to maximize the new LR.
 * Return the new LR.
 */
void GraphColorRegAllocator::SplitLr(LiveRange &lr)
{
    if (!SplitLrShouldSplit(lr)) {
        return;
    }
    LiveRange *newLr = NewLiveRange();
    /*
     * For the new LR, whenever a BB with either a def or
     * use is added, then add the registers that the neighbor
     * is using to the conflict register set indicating that these
     * registers cannot be used for the new LR's color.
     */
    std::unordered_set<regno_t> conflictRegs;
    if (!SplitLrFindCandidateLr(lr, *newLr, conflictRegs)) {
        return;
    }
#ifdef REUSE_SPILLMEM
    /* Copy the original conflict vector for spill reuse optimization */
    lr.SetOldConflict(memPool->NewArray<uint64>(regBuckets));
    for (uint32 i = 0; i < regBuckets; ++i) {
        lr.SetBBConflictElem(static_cast<int32>(i), lr.GetBBConflictElem(static_cast<int32>(i)));
    }
#endif /* REUSE_SPILLMEM */

    std::set<LoopDesc*, LoopDesc::LoopDescCmp> newLoops;
    std::set<LoopDesc*, LoopDesc::LoopDescCmp> origLoops;
    GetAllLrMemberLoops(*newLr, newLoops);
    GetAllLrMemberLoops(lr, origLoops);
    SplitLrHandleLoops(lr, *newLr, origLoops, newLoops);
    SplitLrFixNewLrCallsAndRlod(*newLr, origLoops);
    SplitLrFixOrigLrCalls(lr);

    SplitLrUpdateRegInfo(lr, *newLr, conflictRegs);

    /* At this point, newLr should be unconstrained. */
    lr.SetSplitLr(*newLr);

    newLr->SetRegNO(lr.GetRegNO());
    newLr->SetRegType(lr.GetRegType());
    newLr->SetID(lr.GetID());
    newLr->CopyRematerialization(lr);
    SplitLrUpdateInterference(lr);

    AddCalleeUsed(newLr->GetAssignedRegNO(), newLr->GetRegType());

    /* For the new LR, update assignment for local RA */
    ForEachBBArrElem(newLr->GetBBMember(),
                     [&newLr, this](uint32 bbID) { SetBBInfoGlobalAssigned(bbID, newLr->GetAssignedRegNO()); });

    UpdatePregvetoForNeighbors(*newLr);

    SplitLrErrorCheckAndDebug(lr);
}

void GraphColorRegAllocator::ColorForOptPrologEpilog()
{
#ifdef OPTIMIZE_FOR_PROLOG
    if (!doOptProlog) {
        return;
    }
    for (auto lr : intDelayed) {
        if (!AssignColorToLr(*lr, true)) {
            lr->SetSpilled(true);
        }
    }
    for (auto lr : fpDelayed) {
        if (!AssignColorToLr(*lr, true)) {
            lr->SetSpilled(true);
        }
    }
#endif
}

/*
 *  From the sorted list of constrained LRs, pick the most profitable LR.
 *  Split the LR into LRnew1 LRnew2 where LRnew1 has the maximum number of
 *  BB and is colorable.
 *  The starting BB for traversal must have a color available.
 *
 *  Assign a color, update neighbor's forbidden list.
 *
 *  Update the conflict graph by change the interference list.
 *  In the case of both LRnew1 and LRnew2 conflicts with a BB, this BB's
 *  #neightbors increased.  If this BB was unconstrained, must check if
 *  it is still unconstrained.  Move to constrained if necessary.
 *
 *  Color the unconstrained LRs.
 */
void GraphColorRegAllocator::SplitAndColorForEachLr(MapleVector<LiveRange *> &targetLrVec)
{
    while (!targetLrVec.empty()) {
        auto highestIt = GetHighPriorityLr(targetLrVec);
        LiveRange *lr = *highestIt;
        /* check those lrs in lr->sconflict which is in unconstrained whether it turns to constrined */
        if (highestIt != targetLrVec.end()) {
            targetLrVec.erase(highestIt);
        } else {
            DEBUG_ASSERT(false, "Error: not in targetLrVec");
        }
        if (AssignColorToLr(*lr)) {
            continue;
        }
#ifdef USE_SPLIT
        SplitLr(*lr);
#endif /* USE_SPLIT */
        /*
         * When LR is spilled, it potentially has no conflicts as
         * each def/use is spilled/reloaded.
         */
#ifdef COLOR_SPLIT
        if (!AssignColorToLr(*lr)) {
#endif /* COLOR_SPLIT */
            lr->SetSpilled(true);
            hasSpill = true;
#ifdef COLOR_SPLIT
        }
#endif /* COLOR_SPLIT */
    }
}

void GraphColorRegAllocator::SplitAndColor()
{
    /* handle mustAssigned */
    if (needDump) {
        LogInfo::MapleLogger() << " starting mustAssigned : \n";
    }
    SplitAndColorForEachLr(mustAssigned);

    if (needDump) {
        LogInfo::MapleLogger() << " starting unconstrainedPref : \n";
    }
    /* assign color for unconstained */
    SplitAndColorForEachLr(unconstrainedPref);

    if (needDump) {
        LogInfo::MapleLogger() << " starting constrained : \n";
    }
    /* handle constrained */
    SplitAndColorForEachLr(constrained);

    if (needDump) {
        LogInfo::MapleLogger() << " starting unconstrained : \n";
    }
    /* assign color for unconstained */
    SplitAndColorForEachLr(unconstrained);

#ifdef OPTIMIZE_FOR_PROLOG
    if (doOptProlog) {
        ColorForOptPrologEpilog();
    }
#endif /* OPTIMIZE_FOR_PROLOG */
}

void GraphColorRegAllocator::HandleLocalRegAssignment(regno_t regNO, LocalRegAllocator &localRa, bool isInt)
{
    /* vreg, get a reg for it if not assigned already. */
    if (!localRa.IsInRegAssigned(regNO, isInt) && !localRa.isInRegSpilled(regNO, isInt)) {
        /* find an available phys reg */
        bool founded = false;
        LiveRange *lr = lrMap[regNO];
        regno_t maxIntReg = R0 + MaxIntPhysRegNum();
        regno_t maxFpReg = V0 + MaxFloatPhysRegNum();
        regno_t startReg = isInt ? R0 : V0;
        regno_t endReg = isInt ? maxIntReg : maxFpReg;
        for (uint32 preg = startReg; preg <= endReg; ++preg) {
            if (!localRa.IsPregAvailable(preg, isInt)) {
                continue;
            }
            if (lr->GetNumCall() != 0 && !AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(preg))) {
                continue;
            }
            if (lr->GetPregveto(preg)) {
                continue;
            }
            regno_t assignedReg = preg;
            localRa.ClearPregs(assignedReg, isInt);
            localRa.SetPregUsed(assignedReg, isInt);
            localRa.SetRegAssigned(regNO, isInt);
            localRa.SetRegAssignmentMap(isInt, regNO, assignedReg);
            lr->SetAssignedRegNO(assignedReg);
            founded = true;
            break;
        }
        if (!founded) {
            localRa.SetRegSpilled(regNO, isInt);
            lr->SetSpilled(true);
        }
    }
}

void GraphColorRegAllocator::UpdateLocalRegDefUseCount(regno_t regNO, LocalRegAllocator &localRa, bool isDef,
                                                       bool isInt) const
{
    auto usedIt = localRa.GetUseInfo().find(regNO);
    if (usedIt != localRa.GetUseInfo().end() && !isDef) {
        /* reg use, decrement count */
        DEBUG_ASSERT(usedIt->second > 0, "Incorrect local ra info");
        localRa.SetUseInfoElem(regNO, usedIt->second - 1);
        if (!AArch64isa::IsPhysicalRegister(static_cast<AArch64reg>(regNO)) && localRa.IsInRegAssigned(regNO, isInt)) {
            localRa.IncUseInfoElem(localRa.GetRegAssignmentItem(isInt, regNO));
        }
        if (needDump) {
            LogInfo::MapleLogger() << "\t\treg " << regNO << " update #use to " << localRa.GetUseInfoElem(regNO)
                                   << "\n";
        }
    }

    auto defIt = localRa.GetDefInfo().find(regNO);
    if (defIt != localRa.GetDefInfo().end() && isDef) {
        /* reg def, decrement count */
        DEBUG_ASSERT(defIt->second > 0, "Incorrect local ra info");
        localRa.SetDefInfoElem(regNO, defIt->second - 1);
        if (!AArch64isa::IsPhysicalRegister(static_cast<AArch64reg>(regNO)) && localRa.IsInRegAssigned(regNO, isInt)) {
            localRa.IncDefInfoElem(localRa.GetRegAssignmentItem(isInt, regNO));
        }
        if (needDump) {
            LogInfo::MapleLogger() << "\t\treg " << regNO << " update #def to " << localRa.GetDefInfoElem(regNO)
                                   << "\n";
        }
    }
}

void GraphColorRegAllocator::UpdateLocalRegConflict(regno_t regNO, LocalRegAllocator &localRa, bool isInt)
{
    LiveRange *lr = lrMap[regNO];
    if (lr->GetNumBBConflicts() == 0) {
        return;
    }
    if (!localRa.IsInRegAssigned(regNO, isInt)) {
        return;
    }
    regno_t preg = localRa.GetRegAssignmentItem(isInt, regNO);
    ForEachRegArrElem(lr->GetBBConflict(), [&preg, this](regno_t regNO) { lrMap[regNO]->InsertElemToPregveto(preg); });
}

void GraphColorRegAllocator::HandleLocalRaDebug(regno_t regNO, const LocalRegAllocator &localRa, bool isInt) const
{
    LogInfo::MapleLogger() << "HandleLocalReg " << regNO << "\n";
    LogInfo::MapleLogger() << "\tregUsed:";
    uint64 regUsed = localRa.GetPregUsed(isInt);
    regno_t base = isInt ? R0 : V0;
    regno_t end = isInt ? (RLR - R0) : (V31 - V0);

    for (uint32 i = 0; i <= end; ++i) {
        if ((regUsed & (1ULL << i)) != 0) {
            LogInfo::MapleLogger() << " " << (i + base);
        }
    }
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << "\tregs:";
    uint64 regs = localRa.GetPregs(isInt);
    for (uint32 regnoInLoop = 0; regnoInLoop <= end; ++regnoInLoop) {
        if ((regs & (1ULL << regnoInLoop)) != 0) {
            LogInfo::MapleLogger() << " " << (regnoInLoop + base);
        }
    }
    LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::HandleLocalReg(Operand &op, LocalRegAllocator &localRa, const BBAssignInfo *bbInfo,
                                            bool isDef, bool isInt)
{
    if (!op.IsRegister()) {
        return;
    }
    auto &regOpnd = static_cast<RegOperand &>(op);
    regno_t regNO = regOpnd.GetRegisterNumber();

    if (IsUnconcernedReg(regOpnd)) {
        return;
    }

    /* is this a local register ? */
    if (regNO >= kAllRegNum && !IsLocalReg(regNO)) {
        return;
    }

    if (needDump) {
        HandleLocalRaDebug(regNO, localRa, isInt);
    }

    if (regOpnd.IsPhysicalRegister()) {
        /* conflict with preg is record in lr->pregveto and BBAssignInfo->globalsAssigned */
        UpdateLocalRegDefUseCount(regNO, localRa, isDef, isInt);
        /* See if it is needed by global RA */
        if (localRa.GetUseInfoElem(regNO) == 0 && localRa.GetDefInfoElem(regNO) == 0) {
            if (bbInfo && !bbInfo->GetGlobalsAssigned(regNO)) {
                /* This phys reg is now available for assignment for a vreg */
                localRa.SetPregs(regNO, isInt);
                if (needDump) {
                    LogInfo::MapleLogger() << "\t\tlast ref, phys-reg " << regNO << " now available\n";
                }
            }
        }
    } else {
        HandleLocalRegAssignment(regNO, localRa, isInt);
        UpdateLocalRegDefUseCount(regNO, localRa, isDef, isInt);
        UpdateLocalRegConflict(regNO, localRa, isInt);
        if (localRa.GetUseInfoElem(regNO) == 0 && localRa.GetDefInfoElem(regNO) == 0 &&
            localRa.IsInRegAssigned(regNO, isInt)) {
            /* last ref of vreg, release assignment */
            localRa.SetPregs(localRa.GetRegAssignmentItem(isInt, regNO), isInt);
            if (needDump) {
                LogInfo::MapleLogger() << "\t\tlast ref, release reg " << localRa.GetRegAssignmentItem(isInt, regNO)
                                       << " for " << regNO << "\n";
            }
        }
    }
}

void GraphColorRegAllocator::LocalRaRegSetEraseReg(LocalRegAllocator &localRa, regno_t regNO) const
{
    bool isInt = AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO));
    if (localRa.IsPregAvailable(regNO, isInt)) {
        localRa.ClearPregs(regNO, isInt);
    }
}

bool GraphColorRegAllocator::LocalRaInitRegSet(LocalRegAllocator &localRa, uint32 bbID)
{
    bool needLocalRa = false;
    /* Note physical regs start from R0, V0. */
    localRa.InitPregs(MaxIntPhysRegNum(), MaxFloatPhysRegNum(), cgFunc->GetCG()->GenYieldPoint(), intSpillRegSet,
                      fpSpillRegSet);

    localRa.ClearUseInfo();
    localRa.ClearDefInfo();
    LocalRaInfo *lraInfo = localRegVec[bbID];
    DEBUG_ASSERT(lraInfo != nullptr, "lraInfo not be nullptr");
    for (const auto &useCntPair : lraInfo->GetUseCnt()) {
        regno_t regNO = useCntPair.first;
        if (regNO >= kAllRegNum) {
            needLocalRa = true;
        }
        localRa.SetUseInfoElem(useCntPair.first, useCntPair.second);
    }
    for (const auto &defCntPair : lraInfo->GetDefCnt()) {
        regno_t regNO = defCntPair.first;
        if (regNO >= kAllRegNum) {
            needLocalRa = true;
        }
        localRa.SetDefInfoElem(defCntPair.first, defCntPair.second);
    }
    return needLocalRa;
}

void GraphColorRegAllocator::LocalRaInitAllocatableRegs(LocalRegAllocator &localRa, uint32 bbID)
{
    BBAssignInfo *bbInfo = bbRegInfo[bbID];
    if (bbInfo != nullptr) {
        for (regno_t regNO = kInvalidRegNO; regNO < kMaxRegNum; ++regNO) {
            if (bbInfo->GetGlobalsAssigned(regNO)) {
                LocalRaRegSetEraseReg(localRa, regNO);
            }
        }
    }
}

void GraphColorRegAllocator::LocalRaForEachDefOperand(const Insn &insn, LocalRegAllocator &localRa,
                                                      const BBAssignInfo *bbInfo)
{
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        /* handle def opnd */
        if (!md->GetOpndDes(i)->IsRegDef()) {
            continue;
        }
        auto &regOpnd = static_cast<RegOperand &>(opnd);
        bool isInt = (regOpnd.GetRegisterType() == kRegTyInt);
        HandleLocalReg(opnd, localRa, bbInfo, true, isInt);
    }
}

void GraphColorRegAllocator::LocalRaForEachUseOperand(const Insn &insn, LocalRegAllocator &localRa,
                                                      const BBAssignInfo *bbInfo)
{
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsList()) {
            continue;
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            Operand *offset = memOpnd.GetIndexRegister();
            if (base != nullptr) {
                HandleLocalReg(*base, localRa, bbInfo, false, true);
            }
            if (!memOpnd.IsIntactIndexed()) {
                HandleLocalReg(*base, localRa, bbInfo, true, true);
            }
            if (offset != nullptr) {
                HandleLocalReg(*offset, localRa, bbInfo, false, true);
            }
        } else if (md->GetOpndDes(i)->IsRegUse()) {
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            bool isInt = (regOpnd.GetRegisterType() == kRegTyInt);
            HandleLocalReg(opnd, localRa, bbInfo, false, isInt);
        }
    }
}

void GraphColorRegAllocator::LocalRaPrepareBB(BB &bb, LocalRegAllocator &localRa)
{
    BBAssignInfo *bbInfo = bbRegInfo[bb.GetId()];
    FOR_BB_INSNS(insn, &bb) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        /*
         * Use reverse operand order, assuming use first then def for allocation.
         * need to free the use resource so it can be reused for def.
         */
        LocalRaForEachUseOperand(*insn, localRa, bbInfo);
        LocalRaForEachDefOperand(*insn, localRa, bbInfo);
    }
}

void GraphColorRegAllocator::LocalRaFinalAssignment(const LocalRegAllocator &localRa, BBAssignInfo &bbInfo)
{
    for (const auto &intRegAssignmentMapPair : localRa.GetIntRegAssignmentMap()) {
        regno_t regNO = intRegAssignmentMapPair.second;
        if (needDump) {
            LogInfo::MapleLogger() << "[" << intRegAssignmentMapPair.first << "," << regNO << "],";
        }
        /* Might need to get rid of this copy. */
        bbInfo.SetRegMapElem(intRegAssignmentMapPair.first, regNO);
        AddCalleeUsed(regNO, kRegTyInt);
    }
    for (const auto &fpRegAssignmentMapPair : localRa.GetFpRegAssignmentMap()) {
        regno_t regNO = fpRegAssignmentMapPair.second;
        if (needDump) {
            LogInfo::MapleLogger() << "[" << fpRegAssignmentMapPair.first << "," << regNO << "],";
        }
        /* Might need to get rid of this copy. */
        bbInfo.SetRegMapElem(fpRegAssignmentMapPair.first, regNO);
        AddCalleeUsed(regNO, kRegTyFloat);
    }
}

void GraphColorRegAllocator::LocalRaDebug(const BB &bb, const LocalRegAllocator &localRa) const
{
    LogInfo::MapleLogger() << "bb " << bb.GetId() << " local ra INT need " << localRa.GetNumIntPregUsed() << " regs\n";
    LogInfo::MapleLogger() << "bb " << bb.GetId() << " local ra FP need " << localRa.GetNumFpPregUsed() << " regs\n";
    LogInfo::MapleLogger() << "\tpotential assignments:";
    for (auto it : localRa.GetIntRegAssignmentMap()) {
        LogInfo::MapleLogger() << "[" << it.first << "," << it.second << "],";
    }
    for (auto it : localRa.GetFpRegAssignmentMap()) {
        LogInfo::MapleLogger() << "[" << it.first << "," << it.second << "],";
    }
    LogInfo::MapleLogger() << "\n";
}

/*
 * When do_allocate is false, it is prepass:
 * Traverse each BB, keep track of the number of registers required
 * for local registers in the BB.  Communicate this to global RA.
 *
 * When do_allocate is true:
 * Allocate local registers for each BB based on unused registers
 * from global RA.  Spill if no register available.
 */
void GraphColorRegAllocator::LocalRegisterAllocator(bool doAllocate)
{
    if (needDump) {
        if (doAllocate) {
            LogInfo::MapleLogger() << "LRA allocation start\n";
            PrintBBAssignInfo();
        } else {
            LogInfo::MapleLogger() << "LRA preprocessing start\n";
        }
    }
    LocalRegAllocator *localRa = memPool->New<LocalRegAllocator>(*cgFunc, alloc);
    for (auto *bb : bfs->sortedBBs) {
        uint32 bbID = bb->GetId();

        LocalRaInfo *lraInfo = localRegVec[bb->GetId()];
        if (lraInfo == nullptr) {
            /* No locals to allocate */
            continue;
        }

        localRa->ClearLocalRaInfo();
        bool needLocalRa = LocalRaInitRegSet(*localRa, bbID);
        if (!needLocalRa) {
            /* Only physical regs in bb, no local ra needed. */
            continue;
        }

        if (doAllocate) {
            LocalRaInitAllocatableRegs(*localRa, bbID);
        }

        LocalRaPrepareBB(*bb, *localRa);

        BBAssignInfo *bbInfo = bbRegInfo[bb->GetId()];
        if (bbInfo == nullptr) {
            bbInfo = memPool->New<BBAssignInfo>(alloc);
            bbRegInfo[bbID] = bbInfo;
            bbInfo->InitGlobalAssigned();
        }
        bbInfo->SetIntLocalRegsNeeded(localRa->GetNumIntPregUsed());
        bbInfo->SetFpLocalRegsNeeded(localRa->GetNumFpPregUsed());

        if (doAllocate) {
            if (needDump) {
                LogInfo::MapleLogger() << "\tbb(" << bb->GetId() << ")final local ra assignments:";
            }
            LocalRaFinalAssignment(*localRa, *bbInfo);
            if (needDump) {
                LogInfo::MapleLogger() << "\n";
            }
        } else if (needDump) {
            LocalRaDebug(*bb, *localRa);
        }
    }
}

MemOperand *GraphColorRegAllocator::GetConsistentReuseMem(const uint64 *conflict,
                                                          const std::set<MemOperand *> &usedMemOpnd, uint32 size,
                                                          RegType regType)
{
    std::set<LiveRange *, SetLiveRangeCmpFunc> sconflict;
    regno_t regNO;
    for (uint32 i = 0; i < regBuckets; ++i) {
        for (uint32 b = 0; b < kU64; ++b) {
            if ((conflict[i] & (1ULL << b)) != 0) {
                continue;
            }
            regNO = i * kU64 + b;
            if (regNO >= numVregs) {
                break;
            }
            if (GetLiveRange(regNO) != nullptr) {
                (void)sconflict.insert(lrMap[regNO]);
            }
        }
    }

    for (auto *noConflictLr : sconflict) {
        if (noConflictLr == nullptr || noConflictLr->GetRegType() != regType || noConflictLr->GetSpillSize() != size) {
            continue;
        }
        if (usedMemOpnd.find(noConflictLr->GetSpillMem()) == usedMemOpnd.end()) {
            return noConflictLr->GetSpillMem();
        }
    }
    return nullptr;
}

MemOperand *GraphColorRegAllocator::GetCommonReuseMem(const uint64 *conflict, const std::set<MemOperand *> &usedMemOpnd,
                                                      uint32 size, RegType regType)
{
    regno_t regNO;
    for (uint32 i = 0; i < regBuckets; ++i) {
        for (uint32 b = 0; b < kU64; ++b) {
            if ((conflict[i] & (1ULL << b)) != 0) {
                continue;
            }
            regNO = i * kU64 + b;
            if (regNO >= numVregs) {
                break;
            }
            LiveRange *noConflictLr = GetLiveRange(regNO);
            if (noConflictLr == nullptr || noConflictLr->GetRegType() != regType ||
                noConflictLr->GetSpillSize() != size) {
                continue;
            }
            if (usedMemOpnd.find(noConflictLr->GetSpillMem()) == usedMemOpnd.end()) {
                return noConflictLr->GetSpillMem();
            }
        }
    }
    return nullptr;
}

/* See if any of the non-conflict LR is spilled and use its memOpnd. */
MemOperand *GraphColorRegAllocator::GetReuseMem(uint32 vregNO, uint32 size, RegType regType)
{
    if (cgFunc->GetMirModule().GetSrcLang() != kSrcLangC) {
        return nullptr;
    }
    if (IsLocalReg(vregNO)) {
        return nullptr;
    }

    LiveRange *lr = lrMap[vregNO];
    const uint64 *conflict;
    if (lr->GetSplitLr() != nullptr) {
        /*
         * For split LR, the vreg liveness is optimized, but for spill location
         * the stack location needs to be maintained for the entire LR.
         */
        return nullptr;
    } else {
        conflict = lr->GetBBConflict();
    }

    std::set<MemOperand *> usedMemOpnd;
    auto updateMemOpnd = [&usedMemOpnd, this](regno_t regNO) {
        if (regNO >= numVregs) {
            return;
        }
        LiveRange *lrInner = GetLiveRange(regNO);
        if (lrInner && lrInner->GetSpillMem() != nullptr) {
            (void)usedMemOpnd.insert(lrInner->GetSpillMem());
        }
    };
    ForEachRegArrElem(conflict, updateMemOpnd);
    uint32 regSize = (size <= k32) ? k32 : k64;
    /*
     * This is to order the search so memOpnd given out is consistent.
     * When vreg#s do not change going through VtableImpl.mpl file
     * then this can be simplified.
     */
#ifdef CONSISTENT_MEMOPND
    return GetConsistentReuseMem(conflict, usedMemOpnd, regSize, regType);
#else  /* CONSISTENT_MEMOPND */
    return GetCommonReuseMem(conflict, usedMemOpnd, regSize, regType);
#endif /* CONSISTENT_MEMOPNDi */
}

MemOperand *GraphColorRegAllocator::GetSpillMem(uint32 vregNO, bool isDest, Insn &insn, AArch64reg regNO,
                                                bool &isOutOfRange) const
{
    auto *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    MemOperand *memOpnd = a64CGFunc->GetOrCreatSpillMem(vregNO);
    return (a64CGFunc->AdjustMemOperandIfOffsetOutOfRange(memOpnd, vregNO, isDest, insn, regNO, isOutOfRange));
}

void GraphColorRegAllocator::SpillOperandForSpillPre(Insn &insn, const Operand &opnd, RegOperand &phyOpnd,
                                                     uint32 spillIdx, bool needSpill)
{
    if (!needSpill) {
        return;
    }
    auto &regOpnd = static_cast<const RegOperand &>(opnd);
    uint32 regNO = regOpnd.GetRegisterNumber();
    LiveRange *lr = lrMap[regNO];

    auto *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);

    MemOperand *spillMem = CreateSpillMem(spillIdx, kSpillMemPre);
    DEBUG_ASSERT(spillMem != nullptr, "spillMem nullptr check");

    uint32 regSize = regOpnd.GetSize();
    PrimType stype;
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyInt) {
        stype = (regSize <= k32) ? PTY_i32 : PTY_i64;
    } else {
        stype = (regSize <= k32) ? PTY_f32 : PTY_f64;
    }

    if (a64CGFunc->IsImmediateOffsetOutOfRange(*spillMem, k64)) {
        regno_t pregNO = R16;
        spillMem =
            &a64CGFunc->SplitOffsetWithAddInstruction(*spillMem, k64, static_cast<AArch64reg>(pregNO), false, &insn);
    }
    Insn &stInsn =
        cgFunc->GetInsnBuilder()->BuildInsn(a64CGFunc->PickStInsn(spillMem->GetSize(), stype), phyOpnd, *spillMem);
    std::string comment = " SPILL for spill vreg: " + std::to_string(regNO) + " op:" + kOpcodeInfo.GetName(lr->GetOp());
    stInsn.SetComment(comment);
    insn.GetBB()->InsertInsnBefore(insn, stInsn);
}

MemOperand *GraphColorRegAllocator::GetSpillOrReuseMem(LiveRange &lr, uint32 regSize, bool &isOutOfRange, Insn &insn,
                                                       bool isDef)
{
    (void)regSize;
    MemOperand *memOpnd = nullptr;
    if (lr.GetSpillMem() != nullptr) {
        /* the saved memOpnd cannot be out-of-range */
        memOpnd = lr.GetSpillMem();
    } else {
#ifdef REUSE_SPILLMEM
        memOpnd = GetReuseMem(lr.GetRegNO(), regSize, lr.GetRegType());
        if (memOpnd != nullptr) {
            lr.SetSpillMem(*memOpnd);
            lr.SetSpillSize((regSize <= k32) ? k32 : k64);
        } else {
#endif /* REUSE_SPILLMEM */
            regno_t baseRegNO;
            if (!isDef && lr.GetRegNO() == kRegTyInt) {
                /* src will use its' spill reg as baseRegister when offset out-of-range
                 * add x16, x29, #max-offset  //out-of-range
                 * ldr x16, [x16, #offset]    //reload
                 * mov xd, x16
                 */
                baseRegNO = lr.GetSpillReg();
                if (baseRegNO > RLAST_INT_REG) {
                    baseRegNO = R16;
                }
            } else {
                /* dest will use R16 as baseRegister when offset out-of-range
                 * mov x16, xs
                 * add x17, x29, #max-offset  //out-of-range
                 * str x16, [x17, #offset]    //spill
                 */
                baseRegNO = R16;
            }
            DEBUG_ASSERT(baseRegNO != kRinvalid, "invalid base register number");
            memOpnd = GetSpillMem(lr.GetRegNO(), isDef, insn, static_cast<AArch64reg>(baseRegNO), isOutOfRange);
            /* dest's spill reg can only be R15 and R16 () */
            if (isOutOfRange && isDef) {
                DEBUG_ASSERT(lr.GetSpillReg() != R16, "can not find valid memopnd's base register");
            }
#ifdef REUSE_SPILLMEM
            if (isOutOfRange == 0) {
                lr.SetSpillMem(*memOpnd);
                lr.SetSpillSize((regSize <= k32) ? k32 : k64);
            }
        }
#endif /* REUSE_SPILLMEM */
    }
    return memOpnd;
}

/* Try to find available reg for spill. */
bool GraphColorRegAllocator::SetAvailableSpillReg(std::unordered_set<regno_t> &cannotUseReg, LiveRange &lr,
                                                  uint64 &usedRegMask)
{
    bool isInt = (lr.GetRegType() == kRegTyInt);
    regno_t base = isInt ? R0 : V0;
    uint32 pregInterval = isInt ? 0 : (V0 - R30);
    MapleSet<uint32> &callerRegSet = isInt ? intCallerRegSet : fpCallerRegSet;
    MapleSet<uint32> &calleeRegSet = isInt ? intCalleeRegSet : fpCalleeRegSet;

    for (const auto &it : callerRegSet) {
        regno_t spillReg = it + base;
        if (cannotUseReg.find(spillReg) == cannotUseReg.end() &&
            (usedRegMask & (1ULL << (spillReg - pregInterval))) == 0) {
            lr.SetAssignedRegNO(spillReg);
            usedRegMask |= 1ULL << (spillReg - pregInterval);
            return true;
        }
    }
    for (const auto &it : calleeRegSet) {
        regno_t spillReg = it + base;
        if (cannotUseReg.find(spillReg) == cannotUseReg.end() &&
            (usedRegMask & (1ULL << (spillReg - pregInterval))) == 0) {
            lr.SetAssignedRegNO(spillReg);
            usedRegMask |= 1ULL << (spillReg - pregInterval);
            return true;
        }
    }
    return false;
}

void GraphColorRegAllocator::CollectCannotUseReg(std::unordered_set<regno_t> &cannotUseReg, const LiveRange &lr,
                                                 Insn &insn)
{
    /* Find the bb in the conflict LR that actually conflicts with the current bb. */
    for (regno_t regNO = kRinvalid; regNO < kMaxRegNum; ++regNO) {
        if (lr.GetPregveto(regNO)) {
            (void)cannotUseReg.insert(regNO);
        }
    }
    auto updateCannotUse = [&insn, &cannotUseReg, this](regno_t regNO) {
        LiveRange *conflictLr = lrMap[regNO];
        /*
         * conflictLr->GetAssignedRegNO() might be zero
         * caller save will be inserted so the assigned reg can be released actually
         */
        if ((conflictLr->GetAssignedRegNO() > 0) && IsBitArrElemSet(conflictLr->GetBBMember(), insn.GetBB()->GetId())) {
            if (!AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(conflictLr->GetAssignedRegNO())) &&
                conflictLr->GetNumCall() && !conflictLr->GetProcessed()) {
                return;
            }
            (void)cannotUseReg.insert(conflictLr->GetAssignedRegNO());
        }
    };
    ForEachRegArrElem(lr.GetBBConflict(), updateCannotUse);
#ifdef USE_LRA
    if (!doLRA) {
        return;
    }
    BBAssignInfo *bbInfo = bbRegInfo[insn.GetBB()->GetId()];
    if (bbInfo != nullptr) {
        for (const auto &regMapPair : bbInfo->GetRegMap()) {
            (void)cannotUseReg.insert(regMapPair.second);
        }
    }
#endif /* USE_LRA */
}

regno_t GraphColorRegAllocator::PickRegForSpill(uint64 &usedRegMask, RegType regType, uint32 spillIdx,
                                                bool &needSpillLr)
{
    regno_t base;
    regno_t spillReg;
    uint32 pregInterval;
    bool isIntReg = (regType == kRegTyInt);
    if (isIntReg) {
        base = R0;
        pregInterval = 0;
    } else {
        base = V0;
        pregInterval = V0 - R30;
    }

    /* Temporary find a unused reg to spill */
    uint32 maxPhysRegNum = isIntReg ? MaxIntPhysRegNum() : MaxFloatPhysRegNum();
    for (spillReg = (maxPhysRegNum + base); spillReg > base; --spillReg) {
        if (spillReg >= k64BitSize) {
            spillReg = k64BitSize - 1;
        }
        if ((usedRegMask & (1ULL << (spillReg - pregInterval))) == 0) {
            usedRegMask |= (1ULL << (spillReg - pregInterval));
            needSpillLr = true;
            return spillReg;
        }
    }

    DEBUG_ASSERT(false, "can not find spillReg");
    return 0;
}

/* return true if need extra spill */
bool GraphColorRegAllocator::SetRegForSpill(LiveRange &lr, Insn &insn, uint32 spillIdx, uint64 &usedRegMask, bool isDef)
{
    std::unordered_set<regno_t> cannotUseReg;
    /* SPILL COALESCE */
    if (!isDef && (insn.GetMachineOpcode() == MOP_xmovrr || insn.GetMachineOpcode() == MOP_wmovrr)) {
        auto &ropnd = static_cast<RegOperand &>(insn.GetOperand(0));
        if (ropnd.IsPhysicalRegister()) {
            lr.SetAssignedRegNO(ropnd.GetRegisterNumber());
            return false;
        }
    }

    CollectCannotUseReg(cannotUseReg, lr, insn);

    if (SetAvailableSpillReg(cannotUseReg, lr, usedRegMask)) {
        return false;
    }

    bool needSpillLr = false;
    if (!lr.GetAssignedRegNO()) {
        /*
         * All regs are assigned and none are free.
         * Pick a reg to spill and reuse for this spill.
         * Need to make sure the reg picked is not assigned to this insn,
         * else there will be conflict.
         */
        RegType regType = lr.GetRegType();
        regno_t spillReg = PickRegForSpill(usedRegMask, regType, spillIdx, needSpillLr);
        if (insn.GetMachineOpcode() == MOP_lazy_ldr && spillReg == R17) {
            CHECK_FATAL(false, "register IP1(R17) may be changed when lazy_ldr");
        }
        lr.SetAssignedRegNO(spillReg);
    }
    return needSpillLr;
}

/* get spill reg and check if need extra spill */
bool GraphColorRegAllocator::GetSpillReg(Insn &insn, LiveRange &lr, const uint32 &spillIdx, uint64 &usedRegMask,
                                         bool isDef)
{
    bool needSpillLr = false;
    /*
     * Find a spill reg for the BB among interfereing LR.
     * Without LRA, this info is very inaccurate.  It will falsely interfere
     * with all locals which the spill might not be interfering.
     * For now, every instance of the spill requires a brand new reg assignment.
     */
    if (needDump) {
        LogInfo::MapleLogger() << "LR-regNO " << lr.GetRegNO() << " spilled, finding a spill reg\n";
    }
    if (insn.IsBranch() || insn.IsCall() || (insn.GetMachineOpcode() == MOP_clinit_tail) ||
        (insn.GetNext() && isDef && insn.GetNext()->GetMachineOpcode() == MOP_clinit_tail)) {
        /*
         * When a cond branch reg is spilled, it cannot
         * restore the value after the branch since it can be the target from other br.
         * To do it properly, it will require creating a intermediate bb for the reload.
         * Use x16, it is taken out from available since it is used as a global in the system.
         */
        lr.SetAssignedRegNO(R16);
    } else {
        lr.SetAssignedRegNO(0);
        needSpillLr = SetRegForSpill(lr, insn, spillIdx, usedRegMask, isDef);
        AddCalleeUsed(lr.GetAssignedRegNO(), lr.GetRegType());
    }
    return needSpillLr;
}

// find prev use/def after prev call
bool GraphColorRegAllocator::EncountPrevRef(const BB &pred, LiveRange &lr, bool isDef, std::vector<bool> &visitedMap)
{
    if (!visitedMap[pred.GetId()] && lr.FindInLuMap(pred.GetId()) != lr.EndOfLuMap()) {
        LiveUnit *lu = lr.GetLiveUnitFromLuMap(pred.GetId());
        if (lu->GetDefNum() || lu->GetUseNum() || lu->HasCall()) {
            MapleMap<uint32, uint32> refs = lr.GetRefs(pred.GetId());
            auto it = refs.rbegin();
            bool findPrevRef = (it->second & kIsCall) == 0;
            return findPrevRef;
        }
        if (lu->HasCall()) {
            return false;
        }
    }
    visitedMap[pred.GetId()] = true;
    bool found = true;
    for (auto predBB : pred.GetPreds()) {
        if (!visitedMap[predBB->GetId()]) {
            found &= EncountPrevRef(*predBB, lr, isDef, visitedMap);
        }
    }
    return found;
}

bool GraphColorRegAllocator::FoundPrevBeforeCall(Insn &insn, LiveRange &lr, bool isDef)
{
    bool hasFind = true;
    std::vector<bool> visitedMap(bbVec.size() + 1, false);
    for (auto pred : insn.GetBB()->GetPreds()) {
        hasFind &= EncountPrevRef(*pred, lr, isDef, visitedMap);
        if (!hasFind) {
            return false;
        }
    }
    return insn.GetBB()->GetPreds().size() == 0 ? false : true;
}

// find next def before next call ?  and no next use
bool GraphColorRegAllocator::EncountNextRef(const BB &succ, LiveRange &lr, bool isDef, std::vector<bool> &visitedMap)
{
    if (lr.FindInLuMap(succ.GetId()) != lr.EndOfLuMap()) {
        LiveUnit *lu = lr.GetLiveUnitFromLuMap(succ.GetId());
        bool findNextDef = false;
        if (lu->GetDefNum() || lu->HasCall()) {
            MapleMap<uint32, uint32> refs = lr.GetRefs(succ.GetId());
            for (auto it = refs.begin(); it != refs.end(); ++it) {
                if ((it->second & kIsDef) != 0) {
                    findNextDef = true;
                    break;
                }
                if ((it->second & kIsCall) != 0) {
                    break;
                }
                if ((it->second & kIsUse) != 0) {
                    continue;
                }
            }
            return findNextDef;
        }
        if (lu->HasCall()) {
            return false;
        }
    }
    visitedMap[succ.GetId()] = true;
    bool found = true;
    for (auto succBB : succ.GetSuccs()) {
        if (!visitedMap[succBB->GetId()]) {
            found &= EncountNextRef(*succBB, lr, isDef, visitedMap);
            if (!found) {
                return false;
            }
        }
    }
    return found;
}

bool GraphColorRegAllocator::FoundNextBeforeCall(Insn &insn, LiveRange &lr, bool isDef)
{
    bool haveFind = true;
    std::vector<bool> visitedMap(bbVec.size() + 1, false);
    for (auto succ : insn.GetBB()->GetSuccs()) {
        haveFind &= EncountNextRef(*succ, lr, isDef, visitedMap);
        if (!haveFind) {
            return false;
        }
    }
    return insn.GetBB()->GetSuccs().size() > 0;
}

bool GraphColorRegAllocator::HavePrevRefInCurBB(Insn &insn, LiveRange &lr, bool &contSearch) const
{
    LiveUnit *lu = lr.GetLiveUnitFromLuMap(insn.GetBB()->GetId());
    bool findPrevRef = false;
    if (lu->GetDefNum() || lu->GetUseNum() || lu->HasCall()) {
        MapleMap<uint32, uint32> refs = lr.GetRefs(insn.GetBB()->GetId());
        for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
            if (it->first >= insn.GetId()) {
                continue;
            }
            if ((it->second & kIsCall) != 0) {
                contSearch = false;
                break;
            }
            if (((it->second & kIsUse) != 0) || ((it->second & kIsDef) != 0)) {
                findPrevRef = true;
                contSearch = false;
                break;
            }
        }
    }
    return findPrevRef;
}

bool GraphColorRegAllocator::HaveNextDefInCurBB(Insn &insn, LiveRange &lr, bool &contSearch) const
{
    LiveUnit *lu = lr.GetLiveUnitFromLuMap(insn.GetBB()->GetId());
    bool findNextDef = false;
    if (lu->GetDefNum() || lu->GetUseNum() || lu->HasCall()) {
        MapleMap<uint32, uint32> refs = lr.GetRefs(insn.GetBB()->GetId());
        for (auto it = refs.begin(); it != refs.end(); ++it) {
            if (it->first <= insn.GetId()) {
                continue;
            }
            if ((it->second & kIsCall) != 0) {
                contSearch = false;
                break;
            }
            if ((it->second & kIsDef) != 0) {
                findNextDef = true;
                contSearch = false;
            }
        }
    }
    return findNextDef;
}

bool GraphColorRegAllocator::NeedCallerSave(Insn &insn, LiveRange &lr, bool isDef)
{
    if (doLRA) {
        return true;
    }
    if (lr.HasDefUse()) {
        return true;
    }

    bool contSearch = true;
    bool needed = true;
    if (isDef) {
        needed = !HaveNextDefInCurBB(insn, lr, contSearch);
    } else {
        needed = !HavePrevRefInCurBB(insn, lr, contSearch);
    }
    if (!contSearch) {
        return needed;
    }

    if (isDef) {
        needed = true;
    } else {
        needed = !FoundPrevBeforeCall(insn, lr, isDef);
    }
    return needed;
}

void GraphColorRegAllocator::MarkUsedRegs(Operand &opnd, uint64 &usedRegMask)
{
    auto &regOpnd = static_cast<RegOperand &>(opnd);
    uint32 pregInterval = (regOpnd.GetRegisterType() == kRegTyInt) ? 0 : (V0 - R30);
    uint32 vregNO = regOpnd.GetRegisterNumber();
    LiveRange *lr = GetLiveRange(vregNO);
    if (lr != nullptr) {
        if (lr->IsSpilled()) {
            lr->SetAssignedRegNO(0);
        }
        if (lr->GetAssignedRegNO() != 0) {
            usedRegMask |= (1ULL << (lr->GetAssignedRegNO() - pregInterval));
        }
        if (lr->GetSplitLr() && lr->GetSplitLr()->GetAssignedRegNO()) {
            usedRegMask |= (1ULL << (lr->GetSplitLr()->GetAssignedRegNO() - pregInterval));
        }
    }
}

void CallerSavePre::DumpWorkCandAndOcc()
{
    if (workCand->GetTheOperand()->IsRegister()) {
        LogInfo::MapleLogger() << "Cand R";
        LogInfo::MapleLogger() << static_cast<RegOperand *>(workCand->GetTheOperand())->GetRegisterNumber() << '\n';
    } else {
        LogInfo::MapleLogger() << "Cand Index" << workCand->GetIndex() << '\n';
    }
    for (CgOccur *occ : allOccs) {
        occ->Dump();
        LogInfo::MapleLogger() << '\n';
    }
}

void CallerSavePre::ComputeAvail()
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto *phiOcc : phiOccs) {
            if (phiOcc->IsNotAvailable()) {
                continue;
            }
            size_t killedCnt = 0;
            for (auto *opndOcc : phiOcc->GetPhiOpnds()) {
                auto defOcc = opndOcc->GetDef();
                if (defOcc == nullptr) {
                    continue;
                }
                // for not move load too far from use site, set not-fully-available-phi killing availibity of phiOpnd
                if ((defOcc->GetOccType() == kOccPhiocc && !static_cast<CgPhiOcc *>(defOcc)->IsFullyAvailable()) ||
                    defOcc->GetOccType() == kOccStore) {
                    ++killedCnt;
                    opndOcc->SetHasRealUse(false);
                    // opnd at back-edge is killed, set phi not avail
                    if (dom->Dominate(*phiOcc->GetBB(), *opndOcc->GetBB())) {
                        killedCnt = phiOcc->GetPhiOpnds().size();
                        break;
                    }
                    if (opndOcc->GetBB()->IsSoloGoto() &&
                        loopInfo.GetBBLoopParent(opndOcc->GetBB()->GetId()) != nullptr) {
                        killedCnt = phiOcc->GetPhiOpnds().size();
                        break;
                    }
                    continue;
                }
            }
            if (killedCnt == phiOcc->GetPhiOpnds().size()) {
                changed |= !phiOcc->IsNotAvailable();
                phiOcc->SetAvailability(kNotAvailable);
            } else if (killedCnt > 0) {
                changed |= !phiOcc->IsPartialAvailable();
                phiOcc->SetAvailability(kPartialAvailable);
            } else {
            }  // fully available is default state
        }
    }
}

void CallerSavePre::ComputeVarAndDfPhis()
{
    dfPhiDfns.clear();
    PreWorkCand *workCand = GetWorkCand();
    for (auto *realOcc : workCand->GetRealOccs()) {
        BB *defBB = realOcc->GetBB();
        GetIterDomFrontier(defBB, &dfPhiDfns);
    }
}

void CallerSavePre::ApplySSAPRE()
{
    // #0 build worklist
    uint32 cnt = 0;
    constexpr uint32 preLimit = UINT32_MAX;
    while (!workList.empty()) {
        ++cnt;
        if (cnt == preLimit) {
            beyondLimit = true;
        }
        workCand = workList.front();
        workCand->SetIndex(static_cast<int32>(cnt));
        workLr = regAllocator->GetLiveRange(static_cast<RegOperand *>(workCand->GetTheOperand())->GetRegisterNumber());
        DEBUG_ASSERT(workLr != nullptr, "exepected non null lr");
        workList.pop_front();
        if (workCand->GetRealOccs().empty()) {
            continue;
        }

        allOccs.clear();
        phiOccs.clear();
        // #1 Insert PHI; results in allOccs and phiOccs
        ComputeVarAndDfPhis();
        CreateSortedOccs();
        if (workCand->GetRealOccs().empty()) {
            continue;
        }
        // #2 Rename
        ComputeDS();
        ComputeAvail();
        DEBUG_ASSERT(workLr->GetProcessed() == false, "exepected unprocessed");
        workLr->SetProcessed();
    }
}

void GraphColorRegAllocator::OptCallerSave()
{
    CallerSavePre callerSavePre(this, *cgFunc, domInfo, loopInfo, *memPool, *memPool, kLoadPre, UINT32_MAX);
    callerSavePre.SetDump(needDump);
    callerSavePre.ApplySSAPRE();
}

bool GraphColorRegAllocator::LrGetBadReg(const LiveRange &lr) const
{
    if (lr.IsSpilled()) {
        return true;
    }
    if (lr.GetNumCall() != 0 && !AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(lr.GetAssignedRegNO()))) {
        return true;
    }
    return false;
}

void GraphColorRegAllocator::MarkCalleeSaveRegs()
{
    for (auto regNO : intCalleeUsed) {
        static_cast<AArch64CGFunc *>(cgFunc)->AddtoCalleeSaved(static_cast<AArch64reg>(regNO));
    }
    for (auto regNO : fpCalleeUsed) {
        static_cast<AArch64CGFunc *>(cgFunc)->AddtoCalleeSaved(static_cast<AArch64reg>(regNO));
    }
}
} /* namespace maplebe */
