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

#include "aarch64_regsaves.h"
#include "aarch64_cg.h"
#include "aarch64_live.h"
#include "aarch64_cg.h"
#include "aarch64_proepilog.h"
#include "cg_dominance.h"
#include "cg_ssa_pre.h"
#include "cg_ssu_pre.h"

namespace maplebe {

#define RS_DUMP GetEnabledDebug()
#define RS_EXTRA (RS_DUMP && true)
#define mLog LogInfo::MapleLogger()
#define threshold 8
#define ONE_REG_AT_A_TIME 0

using BBId = uint32;

void AArch64RegSavesOpt::InitData()
{
    calleeBitsDef = cgFunc->GetMemoryPool()->NewArray<CalleeBitsType>(cgFunc->NumBBs());
    errno_t retDef = memset_s(calleeBitsDef, cgFunc->NumBBs() * sizeof(CalleeBitsType), 0,
                              cgFunc->NumBBs() * sizeof(CalleeBitsType));
    calleeBitsUse = cgFunc->GetMemoryPool()->NewArray<CalleeBitsType>(cgFunc->NumBBs());
    errno_t retUse = memset_s(calleeBitsUse, cgFunc->NumBBs() * sizeof(CalleeBitsType), 0,
                              cgFunc->NumBBs() * sizeof(CalleeBitsType));
    CHECK_FATAL(retDef == EOK && retUse == EOK, "memset_s of calleesBits failed");

    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    const MapleVector<AArch64reg> &sp = aarchCGFunc->GetCalleeSavedRegs();
    if (!sp.empty()) {
        if (std::find(sp.begin(), sp.end(), RFP) != sp.end()) {
            aarchCGFunc->GetProEpilogSavedRegs().push_back(RFP);
        }
        if (std::find(sp.begin(), sp.end(), RLR) != sp.end()) {
            aarchCGFunc->GetProEpilogSavedRegs().push_back(RLR);
        }
    }

    for (auto bb : bfs->sortedBBs) {
        SetId2bb(bb);
    }
}

void AArch64RegSavesOpt::CollectLiveInfo(const BB &bb, const Operand &opnd, bool isDef, bool isUse)
{
    if (!opnd.IsRegister()) {
        return;
    }
    const RegOperand &regOpnd = static_cast<const RegOperand &>(opnd);
    regno_t regNO = regOpnd.GetRegisterNumber();
    if (!AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regNO)) || (regNO >= R29 && regNO <= R31)) {
        return; /* check only callee-save registers */
    }
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyVary) {
        return;
    }
    if (isDef) {
        /* First def */
        if (!IsCalleeBitSet(GetCalleeBitsDef(), bb.GetId(), regNO)) {
            SetCalleeBit(GetCalleeBitsDef(), bb.GetId(), regNO);
        }
    }
    if (isUse) {
        /* Last use */
        SetCalleeBit(GetCalleeBitsUse(), bb.GetId(), regNO);
    }
}

void AArch64RegSavesOpt::GenerateReturnBBDefUse(const BB &bb)
{
    PrimType returnType = cgFunc->GetFunction().GetReturnType()->GetPrimType();
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    if (IsPrimitiveFloat(returnType)) {
        Operand &phyOpnd =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(V0), k64BitSize, kRegTyFloat);
        CollectLiveInfo(bb, phyOpnd, false, true);
    } else if (IsPrimitiveInteger(returnType)) {
        Operand &phyOpnd =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R0), k64BitSize, kRegTyInt);
        CollectLiveInfo(bb, phyOpnd, false, true);
    }
}

void AArch64RegSavesOpt::ProcessAsmListOpnd(const BB &bb, Operand &opnd, uint32 idx)
{
    bool isDef = false;
    bool isUse = false;
    switch (idx) {
        case kAsmOutputListOpnd:
        case kAsmClobberListOpnd: {
            isDef = true;
            break;
        }
        case kAsmInputListOpnd: {
            isUse = true;
            break;
        }
        default:
            return;
    }
    ListOperand &listOpnd = static_cast<ListOperand &>(opnd);
    for (auto op : listOpnd.GetOperands()) {
        CollectLiveInfo(bb, *op, isDef, isUse);
    }
}

void AArch64RegSavesOpt::ProcessListOpnd(const BB &bb, Operand &opnd)
{
    ListOperand &listOpnd = static_cast<ListOperand &>(opnd);
    for (auto op : listOpnd.GetOperands()) {
        CollectLiveInfo(bb, *op, false, true);
    }
}

void AArch64RegSavesOpt::ProcessMemOpnd(const BB &bb, Operand &opnd)
{
    auto &memOpnd = static_cast<MemOperand &>(opnd);
    Operand *base = memOpnd.GetBaseRegister();
    Operand *offset = memOpnd.GetIndexRegister();
    if (base != nullptr) {
        CollectLiveInfo(bb, *base, !memOpnd.IsIntactIndexed(), true);
    }
    if (offset != nullptr) {
        CollectLiveInfo(bb, *offset, false, true);
    }
}

void AArch64RegSavesOpt::ProcessCondOpnd(const BB &bb)
{
    Operand &rflag = cgFunc->GetOrCreateRflag();
    CollectLiveInfo(bb, rflag, false, true);
}

/* Record in each local BB the 1st def and the last use of a callee-saved
   register  */
void AArch64RegSavesOpt::GetLocalDefUse()
{
    for (auto bbp : bfs->sortedBBs) {
        BB &bb = *bbp;
        if (bb.GetKind() == BB::kBBReturn) {
            GenerateReturnBBDefUse(bb);
        }
        if (bb.IsEmpty()) {
            continue;
        }

        FOR_BB_INSNS(insn, &bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }

            bool isAsm = (insn->GetMachineOpcode() == MOP_asm);
            const InsnDesc *md = insn->GetDesc();
            uint32 opndNum = insn->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                Operand &opnd = insn->GetOperand(i);
                auto *regProp = md->opndMD[i];
                bool isDef = regProp->IsRegDef();
                bool isUse = regProp->IsRegUse();
                if (opnd.IsList()) {
                    if (isAsm) {
                        ProcessAsmListOpnd(bb, opnd, i);
                    } else {
                        ProcessListOpnd(bb, opnd);
                    }
                } else if (opnd.IsMemoryAccessOperand()) {
                    ProcessMemOpnd(bb, opnd);
                } else if (opnd.IsConditionCode()) {
                    ProcessCondOpnd(bb);
                } else {
                    CollectLiveInfo(bb, opnd, isDef, isUse);
                }
            } /* for all operands */
        }     /* for all insns */
    }         /* for all sortedBBs */

    if (RS_DUMP) {
        for (uint32 i = 0; i < cgFunc->NumBBs(); ++i) {
            mLog << i << " : " << calleeBitsDef[i] << " " << calleeBitsUse[i] << "\n";
            ;
        }
    }
}

void AArch64RegSavesOpt::PrintBBs() const
{
    mLog << "RegSaves LiveIn/Out of BFS nodes:\n";
    for (auto *bb : bfs->sortedBBs) {
        mLog << "< === > ";
        mLog << bb->GetId();
        mLog << " pred:[";
        for (auto *predBB : bb->GetPreds()) {
            mLog << " " << predBB->GetId();
        }
        mLog << "] succs:[";
        for (auto *succBB : bb->GetSuccs()) {
            mLog << " " << succBB->GetId();
        }
        mLog << "]\n LiveIn of [" << bb->GetId() << "]: ";
        for (auto liveIn : bb->GetLiveInRegNO()) {
            mLog << liveIn << " ";
        }
        mLog << "\n LiveOut of [" << bb->GetId() << "]: ";
        for (auto liveOut : bb->GetLiveOutRegNO()) {
            mLog << liveOut << " ";
        }
        mLog << "\n";
    }
}

/* 1st def MUST not have preceding save in dominator list. Each dominator
   block must not have livein or liveout of the register */
int32 AArch64RegSavesOpt::CheckCriteria(BB *bb, regno_t reg) const
{
    /* Already a site to save */
    SavedRegInfo *sp = bbSavedRegs[bb->GetId()];
    if (sp != nullptr && sp->ContainSaveReg(reg)) {
        return 1; // 1 means reg saved here, skip!
    }

    /* This preceding block has livein OR liveout of reg */
    MapleSet<regno_t> &liveIn = bb->GetLiveInRegNO();
    MapleSet<regno_t> &liveOut = bb->GetLiveOutRegNO();
    if (liveIn.find(reg) != liveIn.end() || liveOut.find(reg) != liveOut.end()) {
        return 2; // 2 means has livein/out, skip!
    }

    return 0;
}

/* Return true if reg is already to be saved in its dominator list */
bool AArch64RegSavesOpt::AlreadySavedInDominatorList(const BB *bb, regno_t reg) const
{
    BB *aBB = GetDomInfo()->GetDom(bb->GetId());

    if (RS_DUMP) {
        mLog << "Checking dom list starting " << bb->GetId() << " for saved R" << (reg - 1) << ":\n  ";
    }
    while (!aBB->GetPreds().empty()) { /* can't go beyond prolog */
        if (RS_DUMP) {
            mLog << aBB->GetId() << " ";
        }
        if (int t = CheckCriteria(aBB, reg)) {
            if (RS_DUMP) {
                if (t == 1) {
                    mLog << " --R" << (reg - 1) << " saved here, skip!\n";
                } else {
                    mLog << " --R" << (reg - 1) << " has livein/out, skip!\n";
                }
            }
            return true; /* previously saved, inspect next reg */
        }
        aBB = GetDomInfo()->GetDom(aBB->GetId());
    }
    return false; /* not previously saved, to save at bb */
}

/* Determine callee-save regs save locations and record them in bbSavedRegs.
   Save is needed for a 1st def callee-save register at its dominator block
   outside any loop. */
void AArch64RegSavesOpt::DetermineCalleeSaveLocationsDoms()
{
    if (RS_DUMP) {
        mLog << "Determining regsave sites using dom list for " << cgFunc->GetName() << ":\n";
    }
    for (auto *bb : bfs->sortedBBs) {
        if (RS_DUMP) {
            mLog << "BB: " << bb->GetId() << "\n";
        }
        CalleeBitsType c = GetBBCalleeBits(GetCalleeBitsDef(), bb->GetId());
        if (c == 0) {
            continue;
        }
        CalleeBitsType mask = 1;
        for (uint32 i = 0; i < static_cast<uint32>(sizeof(CalleeBitsType) << k8BitShift); ++i) {
            if ((c & mask) != 0) {
                MapleSet<regno_t> &liveIn = bb->GetLiveInRegNO();
                regno_t reg = ReverseRegBitMap(i);
                if (oneAtaTime && oneAtaTimeReg != reg) {
                    mask <<= 1;
                    continue;
                }
                if (liveIn.find(reg) == liveIn.end()) { /* not livein */
                    BB *bbDom = bb;                     /* start from current BB */
                    bool done = false;
                    while (bbDom->GetLoop() != nullptr) {
                        bbDom = GetDomInfo()->GetDom(bbDom->GetId());
                        if (CheckCriteria(bbDom, reg)) {
                            done = true;
                            break;
                        }
                        DEBUG_ASSERT(bbDom, "Can't find dominator for save location");
                    }
                    if (done) {
                        mask <<= 1;
                        continue;
                    }

                    /* Check if a dominator of bbDom was already a location to save */
                    if (AlreadySavedInDominatorList(bbDom, reg)) {
                        mask <<= 1;
                        continue; /* no need to save again, next reg */
                    }

                    /* Check if the newly found block is a dominator of block(s) in the current
                       to be saved list. If so, remove these blocks from bbSavedRegs */
                    uint32 creg = i;
                    SavedBBInfo *sp = regSavedBBs[creg];
                    if (sp == nullptr) {
                        regSavedBBs[creg] = memPool->New<SavedBBInfo>(alloc);
                    } else {
                        for (BB *sbb : sp->GetBBList()) {
                            for (BB *abb = sbb; !abb->GetPreds().empty();) {
                                if (abb->GetId() == bbDom->GetId()) {
                                    /* Found! Don't plan to save in abb */
                                    sp->RemoveBB(sbb);
                                    bbSavedRegs[sbb->GetId()]->RemoveSaveReg(reg);
                                    if (RS_DUMP) {
                                        mLog << " --R" << (reg - 1) << " save removed from BB" << sbb->GetId() << "\n";
                                    }
                                    break;
                                }
                                abb = GetDomInfo()->GetDom(abb->GetId());
                            }
                        }
                    }
                    regSavedBBs[creg]->InsertBB(bbDom);

                    uint32 bid = bbDom->GetId();
                    if (RS_DUMP) {
                        mLog << " --R" << (reg - 1);
                        mLog << " to save in " << bid << "\n";
                    }
                    SavedRegInfo *ctx = GetbbSavedRegsEntry(bid);
                    if (!ctx->ContainSaveReg(reg)) {
                        ctx->InsertSaveReg(reg);
                    }
                }
            }
            mask <<= 1;
            CalleeBitsType t = c;
            t >>= 1;
            if (t == 0) {
                break; /* short cut */
            }
        }
    }
}

void AArch64RegSavesOpt::DetermineCalleeSaveLocationsPre()
{
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    MapleAllocator sprealloc(memPool);
    if (RS_DUMP) {
        mLog << "Determining regsave sites using ssa_pre for " << cgFunc->GetName() << ":\n";
    }
    const MapleVector<AArch64reg> &callees = aarchCGFunc->GetCalleeSavedRegs();
    for (auto reg : callees) {
        if (reg >= R29 && reg < V8) {
            continue; /* save/restore in prologue, epilogue */
        }
        if (oneAtaTime && oneAtaTimeReg != reg) {
            continue;
        }

        SsaPreWorkCand wkCand(&sprealloc);
        for (uint32 bid = 1; bid < static_cast<uint32>(bbSavedRegs.size()); ++bid) {
            /* Set the BB occurrences of this callee-saved register */
            if (IsCalleeBitSet(GetCalleeBitsDef(), bid, reg) || IsCalleeBitSet(GetCalleeBitsUse(), bid, reg)) {
                (void)wkCand.occBBs.insert(bid);
            }
        }
        DoSavePlacementOpt(cgFunc, GetDomInfo(), &wkCand);
        if (wkCand.saveAtEntryBBs.empty()) {
            /* something gone wrong, skip this reg */
            wkCand.saveAtProlog = true;
        }
        if (wkCand.saveAtProlog) {
            /* Save cannot be applied, skip this reg and place save/restore
               in prolog/epilog */
            MapleVector<AArch64reg> &pe = aarchCGFunc->GetProEpilogSavedRegs();
            if (std::find(pe.begin(), pe.end(), reg) == pe.end()) {
                pe.push_back(reg);
            }
            if (RS_DUMP) {
                mLog << "Save R" << (reg - 1) << " n/a, do in Pro/Epilog\n";
            }
            continue;
        }
        if (!wkCand.saveAtEntryBBs.empty()) {
            for (uint32 entBB : wkCand.saveAtEntryBBs) {
                if (RS_DUMP) {
                    std::string r = reg <= R28 ? "r" : "v";
                    mLog << "BB " << entBB << " save: " << r << (reg - 1) << "\n";
                }
                GetbbSavedRegsEntry(entBB)->InsertSaveReg(reg);
            }
        }
    }
}

/* Determine calleesave regs restore locations by calling ssu-pre,
   previous bbSavedRegs memory is cleared and restore locs recorded in it */
bool AArch64RegSavesOpt::DetermineCalleeRestoreLocations()
{
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    MapleAllocator sprealloc(memPool);
    if (RS_DUMP) {
        mLog << "Determining Callee Restore Locations:\n";
    }
    const MapleVector<AArch64reg> &callees = aarchCGFunc->GetCalleeSavedRegs();
    for (auto reg : callees) {
        if (reg >= R29 && reg < V8) {
            continue; /* save/restore in prologue, epilogue */
        }
        if (oneAtaTime && oneAtaTimeReg != reg) {
            MapleVector<AArch64reg> &pe = aarchCGFunc->GetProEpilogSavedRegs();
            if (std::find(pe.begin(), pe.end(), reg) == pe.end()) {
                pe.push_back(reg);
            }
            continue;
        }

        SPreWorkCand wkCand(&sprealloc);
        for (uint32 bid = 1; bid < static_cast<uint32>(bbSavedRegs.size()); ++bid) {
            /* Set the saved BB locations of this callee-saved register */
            SavedRegInfo *sp = bbSavedRegs[bid];
            if (sp != nullptr) {
                if (sp->ContainSaveReg(reg)) {
                    (void)wkCand.saveBBs.insert(bid);
                }
            }
            /* Set the BB occurrences of this callee-saved register */
            if (IsCalleeBitSet(GetCalleeBitsDef(), bid, reg) || IsCalleeBitSet(GetCalleeBitsUse(), bid, reg)) {
                (void)wkCand.occBBs.insert(bid);
            }
        }
        DoRestorePlacementOpt(cgFunc, GetPostDomInfo(), &wkCand);
        if (wkCand.saveBBs.empty()) {
            /* something gone wrong, skip this reg */
            wkCand.restoreAtEpilog = true;
        }
        /* splitted empty block for critical edge present, skip function */
        MapleSet<uint32> rset = wkCand.restoreAtEntryBBs;
        for (auto bbid : wkCand.restoreAtExitBBs) {
            (void)rset.insert(bbid);
        }
        for (auto bbid : rset) {
            BB *bb = GetId2bb(bbid);
            if (bb->GetKind() == BB::kBBGoto && bb->NumInsn() == 1) {
                aarchCGFunc->GetProEpilogSavedRegs().clear();
                const MapleVector<AArch64reg> &calleesNew = aarchCGFunc->GetCalleeSavedRegs();
                for (auto areg : calleesNew) {
                    aarchCGFunc->GetProEpilogSavedRegs().push_back(areg);
                }
                return false;
            }
        }
        if (wkCand.restoreAtEpilog) {
            /* Restore cannot b3 applied, skip this reg and place save/restore
               in prolog/epilog */
            for (size_t bid = 1; bid < bbSavedRegs.size(); ++bid) {
                SavedRegInfo *sp = bbSavedRegs[bid];
                if (sp != nullptr && !sp->GetSaveSet().empty()) {
                    if (sp->ContainSaveReg(reg)) {
                        sp->RemoveSaveReg(reg);
                    }
                }
            }
            MapleVector<AArch64reg> &pe = aarchCGFunc->GetProEpilogSavedRegs();
            if (std::find(pe.begin(), pe.end(), reg) == pe.end()) {
                pe.push_back(reg);
            }
            if (RS_DUMP) {
                mLog << "Restore R" << (reg - 1) << " n/a, do in Pro/Epilog\n";
            }
            continue;
        }
        if (!wkCand.restoreAtEntryBBs.empty() || !wkCand.restoreAtExitBBs.empty()) {
            for (uint32 entBB : wkCand.restoreAtEntryBBs) {
                if (RS_DUMP) {
                    std::string r = reg <= R28 ? "r" : "v";
                    mLog << "BB " << entBB << " restore: " << r << (reg - 1) << "\n";
                }
                GetbbSavedRegsEntry(entBB)->InsertEntryReg(reg);
            }
            for (uint32 exitBB : wkCand.restoreAtExitBBs) {
                BB *bb = GetId2bb(exitBB);
                if (bb->GetKind() == BB::kBBIgoto) {
                    CHECK_FATAL(false, "igoto detected");
                }
                Insn *lastInsn = bb->GetLastInsn();
                if (lastInsn != nullptr && lastInsn->IsBranch() &&
                    (!lastInsn->GetOperand(0).IsRegister() || /* not a reg OR */
                     (!AArch64Abi::IsCalleeSavedReg(          /* reg but not cs */
                                                    static_cast<AArch64reg>(
                                                        static_cast<RegOperand &>(lastInsn->GetOperand(0))
                                                            .GetRegisterNumber()))))) {
                    /* To insert in this block - 1 instr */
                    SavedRegInfo *sp = GetbbSavedRegsEntry(exitBB);
                    sp->InsertExitReg(reg);
                    sp->insertAtLastMinusOne = true;
                } else if (bb->GetSuccs().size() > 1) {
                    for (BB *sbb : bb->GetSuccs()) {
                        if (sbb->GetPreds().size() > 1) {
                            CHECK_FATAL(false, "critical edge detected");
                        }
                        /* To insert at all succs */
                        GetbbSavedRegsEntry(sbb->GetId())->InsertEntryReg(reg);
                    }
                } else {
                    /* otherwise, BB_FT etc */
                    GetbbSavedRegsEntry(exitBB)->InsertExitReg(reg);
                }
                if (RS_DUMP) {
                    std::string r = reg <= R28 ? "R" : "V";
                    mLog << "BB " << exitBB << " restore: " << r << (reg - 1) << "\n";
                }
            }
        }
    }
    return true;
}

int32 AArch64RegSavesOpt::FindNextOffsetForCalleeSave() const
{
    int32 offset = static_cast<int32>(
        static_cast<AArch64MemLayout *>(cgFunc->GetMemlayout())->RealStackFrameSize() -
        (static_cast<AArch64CGFunc *>(cgFunc)->SizeOfCalleeSaved() - (kDivide2 * kAarch64IntregBytelen) /* FP/LR */) -
        cgFunc->GetMemlayout()->SizeOfArgsToStackPass() - cgFunc->GetFunction().GetFrameReseverdSlot());

    if (cgFunc->GetFunction().GetAttr(FUNCATTR_varargs)) {
        /* GR/VR save areas are above the callee save area */
        AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc->GetMemlayout());
        int saveareasize = static_cast<int>(RoundUp(ml->GetSizeOfGRSaveArea(), GetPointerSize() * k2BitSize) +
                                            RoundUp(ml->GetSizeOfVRSaveArea(), GetPointerSize() * k2BitSize));
        offset -= saveareasize;
    }
    return offset;
}

void AArch64RegSavesOpt::InsertCalleeSaveCode()
{
    uint32 bid = 0;
    BB *saveBB = cgFunc->GetCurBB();
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);

    if (RS_DUMP) {
        mLog << "Inserting Save: \n";
    }
    int32 offset = FindNextOffsetForCalleeSave();
    offset +=
        static_cast<int32>((aarchCGFunc->GetProEpilogSavedRegs().size() - 2) << 3);  // 2 for R29,RLR 3 for 8 bytes
    for (BB *bb : bfs->sortedBBs) {
        bid = bb->GetId();
        aarchCGFunc->SetSplitBaseOffset(0);
        if (bbSavedRegs[bid] != nullptr && !bbSavedRegs[bid]->GetSaveSet().empty()) {
            aarchCGFunc->GetDummyBB()->ClearInsns();
            cgFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
            AArch64reg intRegFirstHalf = kRinvalid;
            AArch64reg fpRegFirstHalf = kRinvalid;
            for (auto areg : bbSavedRegs[bid]->GetSaveSet()) {
                AArch64reg reg = static_cast<AArch64reg>(areg);
                RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
                AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
                std::string r = reg <= R28 ? "R" : "V";
                /* If reg not seen before, record offset and then update */
                if (regOffset.find(areg) == regOffset.end()) {
                    regOffset[areg] = static_cast<uint32>(offset);
                    offset += static_cast<int32>(kAarch64IntregBytelen);
                }
                if (firstHalf == kRinvalid) {
                    /* 1st half in reg pair */
                    firstHalf = reg;
                    if (RS_DUMP && reg > 0) {
                        mLog << r << (reg - 1) << " save in BB" << bid << "  Offset = " << regOffset[reg] << "\n";
                    }
                } else {
                    if (regOffset[reg] == (regOffset[firstHalf] + k8ByteSize)) {
                        /* firstHalf & reg consecutive, make regpair */
                        AArch64GenProEpilog::AppendInstructionPushPair(*cgFunc, firstHalf, reg, regType,
                                                                       static_cast<int32>(regOffset[firstHalf]));
                    } else if (regOffset[firstHalf] == (regOffset[reg] + k8ByteSize)) {
                        /* reg & firstHalf consecutive, make regpair */
                        AArch64GenProEpilog::AppendInstructionPushPair(*cgFunc, reg, firstHalf, regType,
                                                                       static_cast<int32>(regOffset[reg]));
                    } else {
                        /* regs cannot be paired */
                        AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc, firstHalf, regType,
                                                                         static_cast<int32>(regOffset[firstHalf]));
                        AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc, reg, regType,
                                                                         static_cast<int32>(regOffset[reg]));
                    }
                    firstHalf = kRinvalid;
                    if (RS_DUMP) {
                        mLog << r << (reg - 1) << " save in BB" << bid << "  Offset = " << regOffset[reg] << "\n";
                    }
                }
            }

            if (intRegFirstHalf != kRinvalid) {
                AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc, intRegFirstHalf, kRegTyInt,
                                                                 static_cast<int32>(regOffset[intRegFirstHalf]));
            }

            if (fpRegFirstHalf != kRinvalid) {
                AArch64GenProEpilog::AppendInstructionPushSingle(*cgFunc, fpRegFirstHalf, kRegTyFloat,
                                                                 static_cast<int32>(regOffset[fpRegFirstHalf]));
            }
            bb->InsertAtBeginning(*aarchCGFunc->GetDummyBB());
        }
    }
    cgFunc->SetCurBB(*saveBB);
}

/* DFS to verify the save/restore are in pair(s) within a path */
void AArch64RegSavesOpt::Verify(regno_t reg, BB *bb, std::set<BB *, BBIdCmp> *visited, BBId *s, BBId *r)
{
    (void)visited->insert(bb);
    BBId bid = bb->GetId();
    if (RS_EXTRA) {
        mLog << bid << ","; /* path trace can be long */
    }

    if (bbSavedRegs[bid]) {
        bool entryRestoreMet = false;
        if (bbSavedRegs[bid]->ContainEntryReg(reg)) {
            if (RS_EXTRA) {
                mLog << "[^" << bid << "],";  // entry restore found
            }
            if (*s == 0) {
                mLog << "Alert: nR@" << bid << " found w/o save\n";
                return;
            }
            /* complete s/xR found, continue */
            mLog << "(" << *s << "," << bid << ") ";
            *r = bid;
            entryRestoreMet = true;
        }
        if (bbSavedRegs[bid]->ContainSaveReg(reg)) {
            if (RS_EXTRA) {
                mLog << "[" << bid << "],";  // save found
            }
            if (*s != 0 && !entryRestoreMet) {
                /* another save found before last save restored */
                mLog << "Alert: save@" << bid << " found after save@" << *s << "\n";
                return;
            }
            if (entryRestoreMet) {
                *r = 0;
            }
            *s = bid;
        }
        if (bbSavedRegs[bid]->ContainExitReg(reg)) {
            if (RS_EXTRA) {
                mLog << "[" << bid << "$],";  // exit restore found
            }
            if (*s == 0) {
                mLog << "Alert: xR@" << bid << " found w/o save\n";
                return;
            }
            /* complete s/xR found, continue */
            mLog << "(" << *s << "," << bid << ") ";
            *r = bid;
        }
    }

    if (bb->GetSuccs().size() == 0) {
        if (*s != 0 && *r == 0) {
            mLog << "Alert: save@" << *s << " w/o restore reaches end";
        }
        mLog << " <Path " << *s << "->" << bid << " ended>\n";
        *r = 0;
    }
    for (BB *sBB : bb->GetSuccs()) {
        if (visited->count(sBB) == 0) {
            Verify(reg, sBB, visited, s, r);
        }
    }
    if (*s == bid) {
        /* clear only when returned from previous calls to the orig save site */
        /* clear savebid since all of its succs already visited */
        *s = 0;
    }
    if (*r == bid) {
        /* clear restorebid if all of its preds already visited */
        bool clear = true;
        for (BB *pBB : bb->GetPreds()) {
            if (visited->count(pBB) == 0) {
                clear = false;
                break;
            }
        }
        if (clear) {
            *r = 0;
        }
    }
}

void AArch64RegSavesOpt::InsertCalleeRestoreCode()
{
    uint32 bid = 0;
    BB *saveBB = cgFunc->GetCurBB();
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);

    if (RS_DUMP) {
        mLog << "Inserting Restore: \n";
    }
    int32 offset = FindNextOffsetForCalleeSave();
    for (BB *bb : bfs->sortedBBs) {
        bid = bb->GetId();
        aarchCGFunc->SetSplitBaseOffset(0);
        SavedRegInfo *sp = bbSavedRegs[bid];
        if (sp != nullptr) {
            if (sp->GetEntrySet().empty() && sp->GetExitSet().empty()) {
                continue;
            }

            aarchCGFunc->GetDummyBB()->ClearInsns();
            cgFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
            for (auto areg : sp->GetEntrySet()) {
                AArch64reg reg = static_cast<AArch64reg>(areg);
                offset = static_cast<int32>(regOffset[areg]);
                if (RS_DUMP) {
                    std::string r = reg <= R28 ? "R" : "V";
                    mLog << r << (reg - 1) << " entry restore in BB " << bid << "  Saved Offset = " << offset << "\n";
                    if (RS_EXTRA) {
                        mLog << "  for save @BB [ ";
                        for (size_t b = 1; b < bbSavedRegs.size(); ++b) {
                            if (bbSavedRegs[b] != nullptr && bbSavedRegs[b]->ContainSaveReg(reg)) {
                                mLog << b << " ";
                            }
                        }
                        mLog << "]\n";
                    }
                }

                /* restore is always the same from saved offset */
                RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
                AArch64GenProEpilog::AppendInstructionPopSingle(*cgFunc, reg, regType, offset);
            }
            FOR_BB_INSNS(insn, aarchCGFunc->GetDummyBB()) {
                insn->SetDoNotRemove(true); /* do not let ebo remove these restores */
            }
            bb->InsertAtBeginning(*aarchCGFunc->GetDummyBB());

            aarchCGFunc->GetDummyBB()->ClearInsns();
            cgFunc->SetCurBB(*aarchCGFunc->GetDummyBB());
            for (auto areg : sp->GetExitSet()) {
                AArch64reg reg = static_cast<AArch64reg>(areg);
                offset = static_cast<int32>(regOffset[areg]);
                if (RS_DUMP) {
                    std::string r = reg <= R28 ? "R" : "V";
                    mLog << r << (reg - 1) << " exit restore in BB " << bid << " Offset = " << offset << "\n";
                    mLog << "  for save @BB [ ";
                    for (size_t b = 1; b < bbSavedRegs.size(); ++b) {
                        if (bbSavedRegs[b] != nullptr && bbSavedRegs[b]->ContainSaveReg(reg)) {
                            mLog << b << " ";
                        }
                    }
                    mLog << "]\n";
                }

                /* restore is always single from saved offset */
                RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
                AArch64GenProEpilog::AppendInstructionPopSingle(*cgFunc, reg, regType, offset);
            }
            FOR_BB_INSNS(insn, aarchCGFunc->GetDummyBB()) {
                insn->SetDoNotRemove(true);
            }
            if (sp->insertAtLastMinusOne) {
                bb->InsertAtEndMinus1(*aarchCGFunc->GetDummyBB());
            } else {
                bb->InsertAtEnd(*aarchCGFunc->GetDummyBB());
            }
        }
    }
    cgFunc->SetCurBB(*saveBB);
}

/* Callee-save registers save/restore placement optimization */
void AArch64RegSavesOpt::Run()
{
    if (Globals::GetInstance()->GetOptimLevel() <= CGOptions::kLevel1) {
        return;
    }

#if ONE_REG_AT_A_TIME
    /* only do reg placement on the following register, others in pro/epilog */
    oneAtaTime = true;
    oneAtaTimeReg = R25;
#endif

    Bfs localBfs(*cgFunc, *memPool);
    bfs = &localBfs;
    bfs->ComputeBlockOrder();
    if (RS_DUMP) {
        mLog << "##Calleeregs Placement for: " << cgFunc->GetName() << "\n";
        PrintBBs();
    }

#ifdef REDUCE_COMPLEXITY
    CGOptions::EnableRegSavesOpt();
    for (auto bb : bfs->sortedBBs) {
        if (bb->GetSuccs().size() > threshold) {
            CGOptions::DisableRegSavesOpt();
            return;
        }
    }
#endif

    /* Determined 1st def and last use of all callee-saved registers used
       for all BBs */
    InitData();
    GetLocalDefUse();

    /* Determine save sites at dominators of 1st def with no live-in and
       not within loop */
    if (CGOptions::UseSsaPreSave()) {
        DetermineCalleeSaveLocationsPre();
    } else {
        DetermineCalleeSaveLocationsDoms();
    }

    /* Determine restore sites */
    if (!DetermineCalleeRestoreLocations()) {
        return;
    }

#ifdef VERIFY
    /* Verify saves/restores are in pair */
    if (RS_DUMP) {
        std::vector<regno_t> rlist = {R19, R20, R21, R22, R23, R24, R25, R26, R27, R28};
        for (auto reg : rlist) {
            mLog << "Verify calleeregs_placement data for R" << (reg - 1) << ":\n";
            std::set<BB *, BBIdCmp> visited;
            uint32 saveBid = 0;
            uint32 restoreBid = 0;
            Verify(reg, cgFunc->GetFirstBB(), &visited, &saveBid, &restoreBid);
            mLog << "\nVerify Done\n";
        }
    }
#endif

    /* Generate callee save instrs at found sites */
    InsertCalleeSaveCode();

    /* Generate callee restores at found sites */
    InsertCalleeRestoreCode();
}
} /* namespace maplebe */
