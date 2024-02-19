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

#include "aarch64_global.h"
#include "aarch64_reaching.h"
#include "aarch64_cg.h"
#include "aarch64_live.h"

namespace maplebe {
using namespace maple;
#define GLOBAL_DUMP CG_DEBUG_FUNC(cgFunc)

constexpr uint32 kExMOpTypeSize = 9;
constexpr uint32 kLsMOpTypeSize = 15;

MOperator exMOpTable[kExMOpTypeSize] = {MOP_undef,    MOP_xxwaddrrre, MOP_wwwaddrrre, MOP_xxwsubrrre, MOP_wwwsubrrre,
                                        MOP_xwcmnrre, MOP_wwcmnrre,   MOP_xwcmprre,   MOP_wwcmprre};
MOperator lsMOpTable[kLsMOpTypeSize] = {MOP_undef,    MOP_xaddrrrs, MOP_waddrrrs, MOP_xsubrrrs, MOP_wsubrrrs,
                                        MOP_xcmnrrs,  MOP_wcmnrrs,  MOP_xcmprrs,  MOP_wcmprrs,  MOP_xeorrrrs,
                                        MOP_weorrrrs, MOP_xinegrrs, MOP_winegrrs, MOP_xiorrrrs, MOP_wiorrrrs};

/* Optimize ExtendShiftOptPattern:
 * ==========================================================
 *           nosuffix  LSL   LSR   ASR      extrn   (def)
 * nosuffix |   F    | LSL | LSR | ASR |    extrn  |
 * LSL      |   F    | LSL |  F  |  F  |    extrn  |
 * LSR      |   F    |  F  | LSR |  F  |     F     |
 * ASR      |   F    |  F  |  F  | ASR |     F     |
 * exten    |   F    |  F  |  F  |  F  |exten(self)|
 * (use)
 * ===========================================================
 */
constexpr uint32 kExtenAddShift = 5;
ExtendShiftOptPattern::SuffixType doOptimize[kExtenAddShift][kExtenAddShift] = {
    {ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kLSL, ExtendShiftOptPattern::kLSR,
     ExtendShiftOptPattern::kASR, ExtendShiftOptPattern::kExten},
    {ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kLSL, ExtendShiftOptPattern::kNoSuffix,
     ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kExten},
    {ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kLSR,
     ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kNoSuffix},
    {ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kNoSuffix,
     ExtendShiftOptPattern::kASR, ExtendShiftOptPattern::kNoSuffix},
    {ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kNoSuffix,
     ExtendShiftOptPattern::kNoSuffix, ExtendShiftOptPattern::kExten}};

static bool IsZeroRegister(const Operand &opnd)
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const RegOperand *regOpnd = static_cast<const RegOperand *>(&opnd);
    return regOpnd->GetRegisterNumber() == RZR;
}

void AArch64GlobalOpt::Run()
{
    OptimizeManager optManager(cgFunc);
    bool hasSpillBarrier = (cgFunc.NumBBs() > kMaxBBNum) || (cgFunc.GetRD()->GetMaxInsnNO() > kMaxInsnNum);
    if (cgFunc.IsAfterRegAlloc()) {
        optManager.Optimize<SameRHSPropPattern>();
        optManager.Optimize<BackPropPattern>();
        return;
    }
    if (!hasSpillBarrier) {
        optManager.Optimize<ExtenToMovPattern>();
        optManager.Optimize<SameRHSPropPattern>();
        optManager.Optimize<BackPropPattern>();
        optManager.Optimize<ForwardPropPattern>();
        optManager.Optimize<CselPattern>();
        optManager.Optimize<CmpCsetPattern>();
        optManager.Optimize<RedundantUxtPattern>();
        optManager.Optimize<LocalVarSaveInsnPattern>();
    }
    optManager.Optimize<SameDefPattern>();
    optManager.Optimize<ExtendShiftOptPattern>();
    optManager.Optimize<AndCbzPattern>();
}

/* if used Operand in insn is defined by zero in all define insn, return true */
bool OptimizePattern::OpndDefByZero(Insn &insn, int32 useIdx) const
{
    DEBUG_ASSERT(insn.GetOperand(useIdx).IsRegister(), "the used Operand must be Register");
    /* Zero Register don't need be defined */
    if (IsZeroRegister(insn.GetOperand(static_cast<uint32>(useIdx)))) {
        return true;
    }

    InsnSet defInsns = cgFunc.GetRD()->FindDefForRegOpnd(insn, useIdx);
    if (defInsns.empty()) {
        return false;
    }
    for (auto &defInsn : defInsns) {
        if (!InsnDefZero(*defInsn)) {
            return false;
        }
    }
    return true;
}

/* if used Operand in insn is defined by one in all define insn, return true */
bool OptimizePattern::OpndDefByOne(Insn &insn, int32 useIdx) const
{
    DEBUG_ASSERT(insn.GetOperand(useIdx).IsRegister(), "the used Operand must be Register");
    /* Zero Register don't need be defined */
    if (IsZeroRegister(insn.GetOperand(static_cast<uint32>(useIdx)))) {
        return false;
    }
    InsnSet defInsns = cgFunc.GetRD()->FindDefForRegOpnd(insn, useIdx);
    if (defInsns.empty()) {
        return false;
    }
    for (auto &defInsn : defInsns) {
        if (!InsnDefOne(*defInsn)) {
            return false;
        }
    }
    return true;
}

/* if used Operand in insn is defined by one valid bit in all define insn, return true */
bool OptimizePattern::OpndDefByOneOrZero(Insn &insn, int32 useIdx) const
{
    if (IsZeroRegister(insn.GetOperand(static_cast<uint32>(useIdx)))) {
        return true;
    }

    InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, useIdx);
    if (defInsnSet.empty()) {
        return false;
    }

    for (auto &defInsn : defInsnSet) {
        if (!InsnDefOneOrZero(*defInsn)) {
            return false;
        }
    }
    return true;
}

/* if defined operand(must be first insn currently) in insn is const one, return true */
bool OptimizePattern::InsnDefOne(const Insn &insn)
{
    MOperator defMop = insn.GetMachineOpcode();
    switch (defMop) {
        case MOP_wmovri32:
        case MOP_xmovri64: {
            Operand &srcOpnd = insn.GetOperand(1);
            DEBUG_ASSERT(srcOpnd.IsIntImmediate(), "expects ImmOperand");
            ImmOperand &srcConst = static_cast<ImmOperand &>(srcOpnd);
            int64 srcConstValue = srcConst.GetValue();
            if (srcConstValue == 1) {
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

/* if defined operand(must be first insn currently) in insn is const zero, return true */
bool OptimizePattern::InsnDefZero(const Insn &insn)
{
    MOperator defMop = insn.GetMachineOpcode();
    switch (defMop) {
        case MOP_wmovri32:
        case MOP_xmovri64: {
            Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
            DEBUG_ASSERT(srcOpnd.IsIntImmediate(), "expects ImmOperand");
            ImmOperand &srcConst = static_cast<ImmOperand &>(srcOpnd);
            int64 srcConstValue = srcConst.GetValue();
            if (srcConstValue == 0) {
                return true;
            }
            return false;
        }
        case MOP_xmovrr:
        case MOP_wmovrr:
            return IsZeroRegister(insn.GetOperand(kInsnSecondOpnd));
        default:
            return false;
    }
}

/* if defined operand(must be first insn currently) in insn has only one valid bit, return true */
bool OptimizePattern::InsnDefOneOrZero(const Insn &insn)
{
    MOperator defMop = insn.GetMachineOpcode();
    switch (defMop) {
        case MOP_wcsetrc:
        case MOP_xcsetrc:
            return true;
        case MOP_wmovri32:
        case MOP_xmovri64: {
            Operand &defOpnd = insn.GetOperand(kInsnSecondOpnd);
            DEBUG_ASSERT(defOpnd.IsIntImmediate(), "expects ImmOperand");
            auto &defConst = static_cast<ImmOperand &>(defOpnd);
            int64 defConstValue = defConst.GetValue();
            if (defConstValue != 0 && defConstValue != 1) {
                return false;
            } else {
                return true;
            }
        }
        case MOP_xmovrr:
        case MOP_wmovrr: {
            return IsZeroRegister(insn.GetOperand(kInsnSecondOpnd));
        }
        case MOP_wlsrrri5:
        case MOP_xlsrrri6: {
            Operand &opnd2 = insn.GetOperand(kInsnThirdOpnd);
            DEBUG_ASSERT(opnd2.IsIntImmediate(), "expects ImmOperand");
            ImmOperand &opndImm = static_cast<ImmOperand &>(opnd2);
            int64 shiftBits = opndImm.GetValue();
            if (((defMop == MOP_wlsrrri5) && (shiftBits == k32BitSize - 1)) ||
                ((defMop == MOP_xlsrrri6) && (shiftBits == k64BitSize - 1))) {
                return true;
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

void ReplaceAsmListReg(const Insn *insn, uint32 index, uint32 regNO, Operand *newOpnd)
{
    MapleList<RegOperand *> *list = &static_cast<ListOperand &>(insn->GetOperand(index)).GetOperands();
    int32 size = static_cast<int32>(list->size());
    for (int i = 0; i < size; ++i) {
        RegOperand *opnd = static_cast<RegOperand *>(*(list->begin()));
        list->pop_front();
        if (opnd->GetRegisterNumber() == regNO) {
            list->push_back(static_cast<RegOperand *>(newOpnd));
        } else {
            list->push_back(opnd);
        }
    }
}

void OptimizePattern::ReplaceAllUsedOpndWithNewOpnd(const InsnSet &useInsnSet, uint32 regNO, Operand &newOpnd,
                                                    bool updateInfo) const
{
    for (auto useInsn : useInsnSet) {
        if (useInsn->GetMachineOpcode() == MOP_asm) {
            ReplaceAsmListReg(useInsn, kAsmInputListOpnd, regNO, &newOpnd);
        }
        const InsnDesc *md = useInsn->GetDesc();
        uint32 opndNum = useInsn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = useInsn->GetOperand(i);
            auto *regProp = md->opndMD[i];
            if (!regProp->IsRegUse() && !opnd.IsMemoryAccessOperand()) {
                continue;
            }

            if (opnd.IsRegister() && (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO)) {
                useInsn->SetOperand(i, newOpnd);
                if (updateInfo) {
                    cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
                }
            } else if (opnd.IsMemoryAccessOperand()) {
                MemOperand &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();
                MemOperand *newMem = nullptr;
                if (base != nullptr && (base->GetRegisterNumber() == regNO)) {
                    newMem = static_cast<MemOperand *>(opnd.Clone(*cgFunc.GetMemoryPool()));
                    CHECK_FATAL(newMem != nullptr, "null ptr check");
                    newMem->SetBaseRegister(*static_cast<RegOperand *>(&newOpnd));
                    useInsn->SetOperand(i, *newMem);
                    if (updateInfo) {
                        cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
                    }
                }
                if (index != nullptr && (index->GetRegisterNumber() == regNO)) {
                    newMem = static_cast<MemOperand *>(opnd.Clone(*cgFunc.GetMemoryPool()));
                    CHECK_FATAL(newMem != nullptr, "null ptr check");
                    newMem->SetIndexRegister(*static_cast<RegOperand *>(&newOpnd));
                    if (static_cast<RegOperand &>(newOpnd).GetValidBitsNum() != index->GetValidBitsNum()) {
                        newMem->UpdateExtend(MemOperand::kSignExtend);
                    }
                    useInsn->SetOperand(i, *newMem);
                    if (updateInfo) {
                        cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
                    }
                }
            }
        }
    }
}

bool ForwardPropPattern::CheckCondition(Insn &insn)
{
    if (!insn.IsMachineInstruction()) {
        return false;
    }
    if ((insn.GetMachineOpcode() != MOP_xmovrr) && (insn.GetMachineOpcode() != MOP_wmovrr) &&
        (insn.GetMachineOpcode() != MOP_xmovrr_uxtw)) {
        return false;
    }
    Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
    Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
    if (firstOpnd.GetSize() != secondOpnd.GetSize() && insn.GetMachineOpcode() != MOP_xmovrr_uxtw) {
        return false;
    }
    RegOperand &firstRegOpnd = static_cast<RegOperand &>(firstOpnd);
    RegOperand &secondRegOpnd = static_cast<RegOperand &>(secondOpnd);
    uint32 firstRegNO = firstRegOpnd.GetRegisterNumber();
    uint32 secondRegNO = secondRegOpnd.GetRegisterNumber();
    if (IsZeroRegister(firstRegOpnd) || !firstRegOpnd.IsVirtualRegister() || !secondRegOpnd.IsVirtualRegister()) {
        return false;
    }
    firstRegUseInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, firstRegNO, true);
    if (firstRegUseInsnSet.empty()) {
        return false;
    }
    InsnSet secondRegDefInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, secondRegNO, true);
    if (secondRegDefInsnSet.size() != 1 || RegOperand::IsSameReg(firstOpnd, secondOpnd)) {
        return false;
    }
    bool toDoOpt = true;
    for (auto useInsn : firstRegUseInsnSet) {
        if (!cgFunc.GetRD()->RegIsLiveBetweenInsn(secondRegNO, insn, *useInsn)) {
            toDoOpt = false;
            break;
        }
        /* part defined */
        if ((useInsn->GetMachineOpcode() == MOP_xmovkri16) || (useInsn->GetMachineOpcode() == MOP_wmovkri16)) {
            toDoOpt = false;
            break;
        }
        if (useInsn->GetMachineOpcode() == MOP_asm) {
            toDoOpt = false;
            break;
        }
        InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstRegNO, true);
        if (defInsnSet.size() > 1) {
            toDoOpt = false;
            break;
        } else if (defInsnSet.size() == 1 && *defInsnSet.begin() != &insn) {
            toDoOpt = false;
            break;
        }
    }
    return toDoOpt;
}

void ForwardPropPattern::Optimize(Insn &insn)
{
    Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
    Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
    RegOperand &firstRegOpnd = static_cast<RegOperand &>(firstOpnd);
    uint32 firstRegNO = firstRegOpnd.GetRegisterNumber();
    for (auto *useInsn : firstRegUseInsnSet) {
        if (useInsn->GetMachineOpcode() == MOP_asm) {
            ReplaceAsmListReg(useInsn, kAsmInputListOpnd, firstRegNO, &secondOpnd);
            cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
            continue;
        }
        const InsnDesc *md = useInsn->GetDesc();
        uint32 opndNum = useInsn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = useInsn->GetOperand(i);
            const OpndDesc *regProp = md->GetOpndDes(i);
            if (!regProp->IsRegUse() && !opnd.IsMemoryAccessOperand()) {
                continue;
            }

            if (opnd.IsRegister() && (static_cast<RegOperand &>(opnd).GetRegisterNumber() == firstRegNO)) {
                useInsn->SetOperand(i, secondOpnd);
                if (((useInsn->GetMachineOpcode() == MOP_xmovrr) || (useInsn->GetMachineOpcode() == MOP_wmovrr)) &&
                    (static_cast<RegOperand &>(useInsn->GetOperand(kInsnSecondOpnd)).IsVirtualRegister()) &&
                    (static_cast<RegOperand &>(useInsn->GetOperand(kInsnFirstOpnd)).IsVirtualRegister())) {
                    (void)modifiedBB.insert(useInsn->GetBB());
                }
                cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
            } else if (opnd.IsMemoryAccessOperand()) {
                MemOperand &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();
                MemOperand *newMem = nullptr;
                if (base != nullptr && (base->GetRegisterNumber() == firstRegNO)) {
                    newMem = static_cast<MemOperand *>(opnd.Clone(*cgFunc.GetMemoryPool()));
                    CHECK_FATAL(newMem != nullptr, "null ptr check");
                    newMem->SetBaseRegister(static_cast<RegOperand &>(secondOpnd));
                    useInsn->SetOperand(i, *newMem);
                    cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
                }
                if ((index != nullptr) && (index->GetRegisterNumber() == firstRegNO)) {
                    newMem = static_cast<MemOperand *>(opnd.Clone(*cgFunc.GetMemoryPool()));
                    CHECK_FATAL(newMem != nullptr, "null ptr check");
                    newMem->SetIndexRegister(static_cast<RegOperand &>(secondOpnd));
                    if (static_cast<RegOperand &>(secondOpnd).GetValidBitsNum() != index->GetValidBitsNum()) {
                        newMem->UpdateExtend(MemOperand::kSignExtend);
                    }
                    useInsn->SetOperand(i, *newMem);
                    cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
                }
            }
        }
    }
    insn.SetOperand(0, secondOpnd);
    cgFunc.GetRD()->UpdateInOut(*insn.GetBB(), true);
}

void ForwardPropPattern::RemoveMopUxtwToMov(Insn &insn)
{
    if (CGOptions::DoCGSSA()) {
        CHECK_FATAL(false, "check case in ssa");
    }
    auto &secondOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    auto &destOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    uint32 destRegNo = destOpnd.GetRegisterNumber();
    destOpnd.SetRegisterNumber(secondOpnd.GetRegisterNumber());
    auto *newOpnd = static_cast<RegOperand *>(destOpnd.Clone(*cgFunc.GetMemoryPool()));
    cgFunc.InsertExtendSet(secondOpnd.GetRegisterNumber());
    InsnSet regUseInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, destRegNo, true);
    if (regUseInsnSet.size() >= 1) {
        for (auto useInsn : regUseInsnSet) {
            uint32 optSize = useInsn->GetOperandSize();
            for (uint32 i = 0; i < optSize; i++) {
                DEBUG_ASSERT(useInsn->GetOperand(i).IsRegister(), "only design for register");
                if (destRegNo == static_cast<RegOperand &>(useInsn->GetOperand(i)).GetRegisterNumber()) {
                    useInsn->SetOperand(i, *newOpnd);
                }
            }
            cgFunc.GetRD()->InitGenUse(*useInsn->GetBB(), false);
        }
    }
    insn.GetBB()->RemoveInsn(insn);
}

void ForwardPropPattern::Init()
{
    firstRegUseInsnSet.clear();
}

void ForwardPropPattern::Run()
{
    bool secondTime = false;
    do {
        FOR_ALL_BB(bb, &cgFunc)
        {
            if (bb->IsUnreachable() || (secondTime && modifiedBB.find(bb) == modifiedBB.end())) {
                continue;
            }

            if (secondTime) {
                modifiedBB.erase(bb);
            }

            FOR_BB_INSNS(insn, bb)
            {
                Init();
                if (!CheckCondition(*insn)) {
                    if (insn->GetMachineOpcode() == MOP_xmovrr_uxtw) {
                        insn->SetMOP(AArch64CG::kMd[MOP_xuxtw64]);
                    }
                    continue;
                }
                if (insn->GetMachineOpcode() == MOP_xmovrr_uxtw) {
                    RemoveMopUxtwToMov(*insn);
                    continue;
                }
                Optimize(*insn);
            }
        }
        secondTime = true;
    } while (!modifiedBB.empty());
}

bool BackPropPattern::CheckAndGetOpnd(const Insn &insn)
{
    if (!insn.IsMachineInstruction()) {
        return false;
    }
    if (!cgFunc.IsAfterRegAlloc() && (insn.GetMachineOpcode() != MOP_xmovrr) &&
        (insn.GetMachineOpcode() != MOP_wmovrr)) {
        return false;
    }
    if (cgFunc.IsAfterRegAlloc() && (insn.GetMachineOpcode() != MOP_xmovrr) &&
        (insn.GetMachineOpcode() != MOP_wmovrr) && (insn.GetMachineOpcode() != MOP_xvmovs) &&
        (insn.GetMachineOpcode() != MOP_xvmovd)) {
        return false;
    }
    Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
    Operand &secondOpnd = insn.GetOperand(kInsnSecondOpnd);
    if (RegOperand::IsSameReg(firstOpnd, secondOpnd)) {
        return false;
    }
    if (firstOpnd.GetSize() != secondOpnd.GetSize()) {
        return false;
    }
    firstRegOpnd = &static_cast<RegOperand &>(firstOpnd);
    secondRegOpnd = &static_cast<RegOperand &>(secondOpnd);
    if (IsZeroRegister(*firstRegOpnd)) {
        return false;
    }
    if (!cgFunc.IsAfterRegAlloc() && (!secondRegOpnd->IsVirtualRegister() || !firstRegOpnd->IsVirtualRegister())) {
        return false;
    }
    firstRegNO = firstRegOpnd->GetRegisterNumber();
    secondRegNO = secondRegOpnd->GetRegisterNumber();
    return true;
}

bool BackPropPattern::DestOpndHasUseInsns(Insn &insn)
{
    BB &bb = *insn.GetBB();
    InsnSet useInsnSetOfFirstOpnd;
    bool findRes =
        cgFunc.GetRD()->FindRegUseBetweenInsn(firstRegNO, insn.GetNext(), bb.GetLastInsn(), useInsnSetOfFirstOpnd);
    if ((findRes && useInsnSetOfFirstOpnd.empty()) ||
        (!findRes && useInsnSetOfFirstOpnd.empty() && !bb.GetLiveOut()->TestBit(firstRegNO))) {
        return false;
    }
    return true;
}

bool BackPropPattern::DestOpndLiveOutToEHSuccs(Insn &insn) const
{
    BB &bb = *insn.GetBB();
    for (auto ehSucc : bb.GetEhSuccs()) {
        if (ehSucc->GetLiveIn()->TestBit(firstRegNO)) {
            return true;
        }
    }
    return false;
}

bool BackPropPattern::CheckSrcOpndDefAndUseInsns(Insn &insn)
{
    BB &bb = *insn.GetBB();
    /* secondOpnd is defined in other BB */
    std::vector<Insn *> defInsnVec =
        cgFunc.GetRD()->FindRegDefBetweenInsn(secondRegNO, bb.GetFirstInsn(), insn.GetPrev());
    if (defInsnVec.size() != 1) {
        return false;
    }
    defInsnForSecondOpnd = defInsnVec.back();
    /* part defined */
    if ((defInsnForSecondOpnd->GetMachineOpcode() == MOP_xmovkri16) ||
        (defInsnForSecondOpnd->GetMachineOpcode() == MOP_wmovkri16) ||
        (defInsnForSecondOpnd->GetMachineOpcode() == MOP_asm)) {
        return false;
    }
    if (AArch64isa::IsPseudoInstruction(defInsnForSecondOpnd->GetMachineOpcode()) || defInsnForSecondOpnd->IsCall()) {
        return false;
    }
    /* unconcerned regs. */
    if ((secondRegNO >= RLR && secondRegNO <= RZR) || secondRegNO == RFP) {
        return false;
    }
    if (defInsnForSecondOpnd->IsStore() || defInsnForSecondOpnd->IsLoad()) {
        auto *memOpnd = static_cast<MemOperand *>(defInsnForSecondOpnd->GetMemOpnd());
        if (memOpnd != nullptr && !memOpnd->IsIntactIndexed()) {
            return false;
        }
    }

    bool findFinish = cgFunc.GetRD()->FindRegUseBetweenInsn(secondRegNO, defInsnForSecondOpnd->GetNext(),
                                                            bb.GetLastInsn(), srcOpndUseInsnSet);
    if (!findFinish && bb.GetLiveOut()->TestBit(secondRegNO)) {
        return false;
    }
    if (cgFunc.IsAfterRegAlloc() && findFinish && srcOpndUseInsnSet.size() > 1) {
        /* use later before killed. */
        return false;
    }
    if (cgFunc.IsAfterRegAlloc()) {
        for (auto *usePoint : srcOpndUseInsnSet) {
            if (usePoint->IsCall()) {
                return false;
            }
        }
    }
    return true;
}

bool BackPropPattern::CheckSrcOpndDefAndUseInsnsGlobal(Insn &insn)
{
    /* secondOpnd is defined in other BB */
    InsnSet defInsnVec = cgFunc.GetRD()->FindDefForRegOpnd(insn, secondRegNO, true);
    if (defInsnVec.size() != 1) {
        return false;
    }
    defInsnForSecondOpnd = *(defInsnVec.begin());

    /* ensure that there is no fisrt RegNO def/use between insn and defInsnForSecondOpnd */
    std::vector<Insn *> defInsnVecFirst;

    if (insn.GetBB() != defInsnForSecondOpnd->GetBB()) {
        defInsnVecFirst = cgFunc.GetRD()->FindRegDefBetweenInsnGlobal(firstRegNO, defInsnForSecondOpnd, &insn);
    } else {
        defInsnVecFirst = cgFunc.GetRD()->FindRegDefBetweenInsn(firstRegNO, defInsnForSecondOpnd, insn.GetPrev());
    }
    if (!defInsnVecFirst.empty()) {
        return false;
    }
    /* part defined */
    if ((defInsnForSecondOpnd->GetMachineOpcode() == MOP_xmovkri16) ||
        (defInsnForSecondOpnd->GetMachineOpcode() == MOP_wmovkri16) ||
        (defInsnForSecondOpnd->GetMachineOpcode() == MOP_asm)) {
        return false;
    }

    if (defInsnForSecondOpnd->IsStore() || defInsnForSecondOpnd->IsLoad()) {
        auto *memOpnd = static_cast<MemOperand *>(defInsnForSecondOpnd->GetMemOpnd());
        if (memOpnd != nullptr && !memOpnd->IsIntactIndexed()) {
            return false;
        }
    }

    srcOpndUseInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(*defInsnForSecondOpnd, secondRegNO, true);
    /*
     * useInsn is not expected to have multiple definition
     * replaced opnd is not expected to have definition already
     */
    return CheckReplacedUseInsn(insn);
}

bool BackPropPattern::CheckPredefineInsn(Insn &insn)
{
    if (insn.GetPrev() == defInsnForSecondOpnd) {
        return true;
    }
    std::vector<Insn *> preDefInsnForFirstOpndVec;
    /* there is no predefine insn in current bb */
    if (!cgFunc.GetRD()->RegIsUsedOrDefBetweenInsn(firstRegNO, *defInsnForSecondOpnd, insn)) {
        return false;
    }
    return true;
}

bool BackPropPattern::CheckReplacedUseInsn(Insn &insn)
{
    for (auto *useInsn : srcOpndUseInsnSet) {
        if (useInsn->GetMemOpnd() != nullptr) {
            auto *a64MemOpnd = static_cast<MemOperand *>(useInsn->GetMemOpnd());
            if (!a64MemOpnd->IsIntactIndexed()) {
                if (a64MemOpnd->GetBaseRegister() != nullptr &&
                    a64MemOpnd->GetBaseRegister()->GetRegisterNumber() == secondRegNO) {
                    return false;
                }
            }
        }
        /* insn has been checked def */
        if (useInsn == &insn) {
            if (defInsnForSecondOpnd != useInsn->GetPrev() &&
                cgFunc.GetRD()->FindRegUseBetweenInsnGlobal(firstRegNO, defInsnForSecondOpnd, useInsn, insn.GetBB())) {
                return false;
            }
            continue;
        }
        auto checkOneDefOnly = [](const InsnSet &defSet, const Insn &oneDef, bool checkHasDef = false) -> bool {
            if (defSet.size() > 1) {
                return false;
            } else if (defSet.size() == 1) {
                if (&oneDef != *(defSet.begin())) {
                    return false;
                }
            } else {
                if (checkHasDef) {
                    CHECK_FATAL(false, "find def insn failed");
                }
            }
            return true;
        };
        /* ensure that the use insns to be replaced is defined by defInsnForSecondOpnd only */
        if (useInsn->IsMemAccess() &&
            static_cast<MemOperand *>(useInsn->GetMemOpnd())->GetIndexOpt() != MemOperand::kIntact) {
            return false;
        }
        InsnSet defInsnVecOfSrcOpnd = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, secondRegNO, true);
        if (!checkOneDefOnly(defInsnVecOfSrcOpnd, *defInsnForSecondOpnd, true)) {
            return false;
        }

        InsnSet defInsnVecOfFirstReg = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstRegNO, true);
        if (!checkOneDefOnly(defInsnVecOfFirstReg, insn)) {
            return false;
        }

        if (defInsnForSecondOpnd != useInsn->GetPrev() &&
            cgFunc.GetRD()->FindRegUseBetweenInsnGlobal(firstRegNO, defInsnForSecondOpnd, useInsn, insn.GetBB())) {
            return false;
        }
    }
    return true;
}

bool BackPropPattern::CheckRedefineInsn(Insn &insn)
{
    for (auto useInsn : srcOpndUseInsnSet) {
        Insn *startInsn = &insn;
        Insn *endInsn = useInsn;
        if (endInsn == startInsn) {
            if (cgFunc.GetRD()->RegIsUsedIncaller(firstRegNO, insn, *useInsn)) {
                return false;
            } else {
                continue;
            }
        }

        if (useInsn->GetBB() == insn.GetBB()) {
            if (useInsn->GetId() < insn.GetId()) {
                startInsn = useInsn;
                endInsn = &insn;
            }
        }
        if (!cgFunc.GetRD()->RegIsLiveBetweenInsn(firstRegNO, *startInsn, *endInsn, true, true)) {
            return false;
        }
        if (!cgFunc.GetRD()->RegIsLiveBetweenInsn(secondRegNO, *startInsn, *endInsn, true)) {
            return false;
        }
    }
    return true;
}

bool BackPropPattern::CheckCondition(Insn &insn)
{
    if (!CheckAndGetOpnd(insn)) {
        return false;
    }
    /* Unless there is a reason that dest can not live out the current BB */
    if (cgFunc.HasAsm() && !DestOpndHasUseInsns(insn)) {
        return false;
    }
    /* first register must not be live out to eh_succs */
    if (DestOpndLiveOutToEHSuccs(insn)) {
        return false;
    }
    if (globalProp) {
        if (!CheckSrcOpndDefAndUseInsnsGlobal(insn)) {
            return false;
        }
    } else {
        if (!CheckSrcOpndDefAndUseInsns(insn)) {
            return false;
        }
        if (!CheckPredefineInsn(insn)) {
            return false;
        }
        if (!CheckRedefineInsn(insn)) {
            return false;
        }
    }
    return true;
}

void BackPropPattern::Optimize(Insn &insn)
{
    Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
    ReplaceAllUsedOpndWithNewOpnd(srcOpndUseInsnSet, secondRegNO, firstOpnd, true);
    /* replace define insn */
    const InsnDesc *md = defInsnForSecondOpnd->GetDesc();
    uint32 opndNum = defInsnForSecondOpnd->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = defInsnForSecondOpnd->GetOperand(i);
        if (!md->opndMD[i]->IsRegDef() && !opnd.IsMemoryAccessOperand()) {
            continue;
        }

        if (opnd.IsRegister() && (static_cast<RegOperand &>(opnd).GetRegisterNumber() == secondRegNO)) {
            /* remove remat info */
            Operand &defOp = defInsnForSecondOpnd->GetOperand(i);
            CHECK_FATAL(defOp.IsRegister(), "unexpect def opnd type");
            auto &defRegOp = static_cast<RegOperand &>(defOp);
            MIRPreg *preg = static_cast<AArch64CGFunc &>(cgFunc).GetPseudoRegFromVirtualRegNO(
                defRegOp.GetRegisterNumber(), CGOptions::DoCGSSA());
            if (preg != nullptr) {
                preg->SetOp(OP_undef);
            }
            defInsnForSecondOpnd->SetOperand(i, firstOpnd);
            cgFunc.GetRD()->UpdateInOut(*defInsnForSecondOpnd->GetBB());
        } else if (opnd.IsMemoryAccessOperand()) {
            MemOperand &memOpnd = static_cast<MemOperand &>(opnd);
            RegOperand *base = memOpnd.GetBaseRegister();
            if (base != nullptr && memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi &&
                (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed()) && base->GetRegisterNumber() == secondRegNO) {
                MemOperand *newMem = static_cast<MemOperand *>(opnd.Clone(*cgFunc.GetMemoryPool()));
                CHECK_FATAL(newMem != nullptr, "null ptr check");
                newMem->SetBaseRegister(static_cast<RegOperand &>(firstOpnd));
                defInsnForSecondOpnd->SetOperand(i, *newMem);
                cgFunc.GetRD()->UpdateInOut(*defInsnForSecondOpnd->GetBB());
            }
        }
    }
    /* There is special implication when backward propagation is allowed for physical register R0.
     * This is a case that the calling func foo directly returns the result from the callee bar as follows:
     *
     * foo:
     * bl                                               bl // bar()
     * mov vreg, X0  //res = bar()        naive bkprop
     * ....          //X0 is not redefined    ====>        ....  //X0 may be reused as RA sees "X0 has not been used"
     * after bl mov X0, vreg                                              //In fact, X0 is implicitly used by foo. We
     * need to tell RA that X0 is live ret                                              ret
     *
     * To make RA simple, we tell RA to not use X0 by keeping "mov X0, X0". That is
     * foo:
     * bl //bar()
     * ....          // Perform backward prop X0 and force X0 cant be reused
     * mov X0, X0    // This can be easily remved later in peephole phase
     * ret
     */
    if (cgFunc.HasCall() && !(cgFunc.GetFunction().IsReturnVoid()) && (firstRegNO == R0) &&
        (static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == R0)) {
        /* Keep this instruction: mov R0, R0 */
        cgFunc.GetRD()->UpdateInOut(*insn.GetBB(), true);
        return;
    } else {
        insn.GetBB()->RemoveInsn(insn);
        cgFunc.GetRD()->UpdateInOut(*insn.GetBB(), true);
    }
}

void BackPropPattern::Init()
{
    firstRegOpnd = nullptr;
    secondRegOpnd = nullptr;
    firstRegNO = 0;
    secondRegNO = 0;
    srcOpndUseInsnSet.clear();
    defInsnForSecondOpnd = nullptr;
}

void BackPropPattern::Run()
{
    bool secondTime = false;
    std::set<BB *, BBIdCmp> modifiedBB;
    do {
        FOR_ALL_BB(bb, &cgFunc)
        {
            if (bb->IsUnreachable() || (secondTime && modifiedBB.find(bb) == modifiedBB.end())) {
                continue;
            }

            if (secondTime) {
                modifiedBB.erase(bb);
            }

            FOR_BB_INSNS_REV(insn, bb)
            {
                Init();
                if (!CheckCondition(*insn)) {
                    continue;
                }
                (void)modifiedBB.insert(bb);
                Optimize(*insn);
            }
        }
        secondTime = true;
    } while (!modifiedBB.empty());
}

bool CmpCsetPattern::CheckCondition(Insn &insn)
{
    nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr || !insn.IsMachineInstruction()) {
        return false;
    }

    MOperator firstMop = insn.GetMachineOpcode();
    MOperator secondMop = nextInsn->GetMachineOpcode();
    if (!(((firstMop == MOP_wcmpri) || (firstMop == MOP_xcmpri)) &&
          ((secondMop == MOP_wcsetrc) || (secondMop == MOP_xcsetrc)))) {
        return false;
    }

    /* get cmp_first operand */
    cmpFirstOpnd = &(insn.GetOperand(kInsnSecondOpnd));
    /* get cmp second Operand, ImmOperand must be 0 or 1 */
    cmpSecondOpnd = &(insn.GetOperand(kInsnThirdOpnd));
    DEBUG_ASSERT(cmpSecondOpnd->IsIntImmediate(), "expects ImmOperand");
    ImmOperand *cmpConstOpnd = static_cast<ImmOperand *>(cmpSecondOpnd);
    cmpConstVal = cmpConstOpnd->GetValue();
    /* get cset first Operand */
    csetFirstOpnd = &(nextInsn->GetOperand(kInsnFirstOpnd));
    if (((cmpConstVal != 0) && (cmpConstVal != 1)) || (cmpFirstOpnd->GetSize() != csetFirstOpnd->GetSize()) ||
        !OpndDefByOneOrZero(insn, 1)) {
        return false;
    }

    InsnSet useInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, 0, false);
    if (useInsnSet.size() > 1) {
        return false;
    }
    return true;
}

void CmpCsetPattern::Optimize(Insn &insn)
{
    Insn *csetInsn = nextInsn;
    BB &bb = *insn.GetBB();
    nextInsn = nextInsn->GetNextMachineInsn();
    /* get condition Operand */
    CondOperand &cond = static_cast<CondOperand &>(csetInsn->GetOperand(kInsnSecondOpnd));
    if (((cmpConstVal == 0) && (cond.GetCode() == CC_NE)) || ((cmpConstVal == 1) && (cond.GetCode() == CC_EQ))) {
        if (RegOperand::IsSameReg(*cmpFirstOpnd, *csetFirstOpnd)) {
            bb.RemoveInsn(insn);
            bb.RemoveInsn(*csetInsn);
        } else {
            MOperator mopCode = (cmpFirstOpnd->GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr;
            Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mopCode, *csetFirstOpnd, *cmpFirstOpnd);
            newInsn.SetId(insn.GetId());
            bb.ReplaceInsn(insn, newInsn);
            bb.RemoveInsn(*csetInsn);
        }
    } else if (((cmpConstVal == 1) && (cond.GetCode() == CC_NE)) || ((cmpConstVal == 0) && (cond.GetCode() == CC_EQ))) {
        MOperator mopCode = (cmpFirstOpnd->GetSize() == k64BitSize) ? MOP_xeorrri13 : MOP_weorrri12;
        constexpr int64 eorImm = 1;
        auto &aarch64CGFunc = static_cast<AArch64CGFunc &>(cgFunc);
        ImmOperand &one = aarch64CGFunc.CreateImmOperand(eorImm, k8BitSize, false);
        Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mopCode, *csetFirstOpnd, *cmpFirstOpnd, one);
        newInsn.SetId(insn.GetId());
        bb.ReplaceInsn(insn, newInsn);
        bb.RemoveInsn(*csetInsn);
    }
    cgFunc.GetRD()->UpdateInOut(bb, true);
}

void CmpCsetPattern::Init()
{
    cmpConstVal = 0;
    cmpFirstOpnd = nullptr;
    cmpSecondOpnd = nullptr;
    csetFirstOpnd = nullptr;
}

void CmpCsetPattern::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            Init();
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

bool CselPattern::CheckCondition(Insn &insn)
{
    MOperator mopCode = insn.GetMachineOpcode();
    if ((mopCode != MOP_xcselrrrc) && (mopCode != MOP_wcselrrrc)) {
        return false;
    }
    return true;
}

void CselPattern::Optimize(Insn &insn)
{
    BB &bb = *insn.GetBB();
    Operand &opnd0 = insn.GetOperand(kInsnFirstOpnd);
    Operand &cond = insn.GetOperand(kInsnFourthOpnd);
    MOperator newMop = ((opnd0.GetSize()) == k64BitSize ? MOP_xcsetrc : MOP_wcsetrc);
    Operand &rflag = cgFunc.GetOrCreateRflag();
    if (OpndDefByOne(insn, kInsnSecondOpnd) && OpndDefByZero(insn, kInsnThirdOpnd)) {
        Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newMop, opnd0, cond, rflag);
        newInsn.SetId(insn.GetId());
        bb.ReplaceInsn(insn, newInsn);
        cgFunc.GetRD()->InitGenUse(bb, false);
    } else if (OpndDefByZero(insn, kInsnSecondOpnd) && OpndDefByOne(insn, kInsnThirdOpnd)) {
        auto &originCond = static_cast<CondOperand &>(cond);
        ConditionCode inverseCondCode = GetReverseBasicCC(originCond.GetCode());
        if (inverseCondCode == kCcLast) {
            return;
        }
        auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
        CondOperand &inverseCond = aarchCGFunc.GetCondOperand(inverseCondCode);
        Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newMop, opnd0, inverseCond, rflag);
        newInsn.SetId(insn.GetId());
        bb.ReplaceInsn(insn, newInsn);
        cgFunc.GetRD()->InitGenUse(bb, false);
    }
}

void CselPattern::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn)
        {
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

uint32 RedundantUxtPattern::GetInsnValidBit(const Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    uint32 nRet;
    switch (mOp) {
        case MOP_wcsetrc:
        case MOP_xcsetrc:
            nRet = 1;
            break;
        case MOP_wldrb:
        case MOP_wldarb:
        case MOP_wldxrb:
        case MOP_wldaxrb:
            nRet = k8BitSize;
            break;
        case MOP_wldrh:
        case MOP_wldarh:
        case MOP_wldxrh:
        case MOP_wldaxrh:
            nRet = k16BitSize;
            break;
        case MOP_wmovrr:
        case MOP_wmovri32:
        case MOP_wldrsb:
        case MOP_wldrsh:
        case MOP_wldli:
        case MOP_wldr:
        case MOP_wldp:
        case MOP_wldar:
        case MOP_wmovkri16:
        case MOP_wmovzri16:
        case MOP_wmovnri16:
        case MOP_wldxr:
        case MOP_wldaxr:
        case MOP_wldaxp:
        case MOP_wcsincrrrc:
        case MOP_wcselrrrc:
        case MOP_wcsinvrrrc:
            nRet = k32BitSize;
            break;
        default:
            nRet = k64BitSize;
            break;
    }
    return nRet;
}

uint32 RedundantUxtPattern::GetMaximumValidBit(Insn &insn, uint8 index, InsnSet &visitedInsn) const
{
    InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, index);
    if (defInsnSet.empty()) {
        /* disable opt when there is no def point. */
        return k64BitSize;
    }

    uint32 validBit = 0;
    uint32 nMaxValidBit = 0;
    for (auto &defInsn : defInsnSet) {
        if (visitedInsn.find(defInsn) != visitedInsn.end()) {
            continue;
        }

        (void)visitedInsn.insert(defInsn);
        MOperator mOp = defInsn->GetMachineOpcode();
        if ((mOp == MOP_wmovrr) || (mOp == MOP_xmovrr)) {
            validBit = GetMaximumValidBit(*defInsn, 1, visitedInsn);
        } else {
            validBit = GetInsnValidBit(*defInsn);
        }

        nMaxValidBit = nMaxValidBit < validBit ? validBit : nMaxValidBit;
    }
    return nMaxValidBit;
}

bool RedundantUxtPattern::CheckCondition(Insn &insn)
{
    BB &bb = *insn.GetBB();
    InsnSet visitedInsn1;
    InsnSet visitedInsn2;
    if (!((insn.GetMachineOpcode() == MOP_xuxth32 &&
           GetMaximumValidBit(insn, kInsnSecondOpnd, visitedInsn1) <= k16BitSize) ||
          (insn.GetMachineOpcode() == MOP_xuxtb32 &&
           GetMaximumValidBit(insn, kInsnSecondOpnd, visitedInsn2) <= k8BitSize))) {
        return false;
    }

    Operand &firstOpnd = insn.GetOperand(kInsnFirstOpnd);
    secondOpnd = &(insn.GetOperand(kInsnSecondOpnd));
    if (RegOperand::IsSameReg(firstOpnd, *secondOpnd)) {
        bb.RemoveInsn(insn);
        /* update in/out */
        cgFunc.GetRD()->UpdateInOut(bb, true);
        return false;
    }
    useInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(insn, 0, false);
    RegOperand &firstRegOpnd = static_cast<RegOperand &>(firstOpnd);
    firstRegNO = firstRegOpnd.GetRegisterNumber();
    /* for uxth R1, V501, R1 is parameter register, this can't be optimized. */
    if (firstRegOpnd.IsPhysicalRegister()) {
        return false;
    }

    if (useInsnSet.empty()) {
        bb.RemoveInsn(insn);
        /* update in/out */
        cgFunc.GetRD()->UpdateInOut(bb, true);
        return false;
    }

    RegOperand *secondRegOpnd = static_cast<RegOperand *>(secondOpnd);
    DEBUG_ASSERT(secondRegOpnd != nullptr, "secondRegOpnd should not be nullptr");
    uint32 secondRegNO = secondRegOpnd->GetRegisterNumber();
    for (auto useInsn : useInsnSet) {
        InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstRegNO, true);
        if ((defInsnSet.size() > 1) || !(cgFunc.GetRD()->RegIsLiveBetweenInsn(secondRegNO, insn, *useInsn))) {
            return false;
        }
    }
    return true;
}

void RedundantUxtPattern::Optimize(Insn &insn)
{
    BB &bb = *insn.GetBB();
    ReplaceAllUsedOpndWithNewOpnd(useInsnSet, firstRegNO, *secondOpnd, true);
    bb.RemoveInsn(insn);
    cgFunc.GetRD()->UpdateInOut(bb, true);
}

void RedundantUxtPattern::Init()
{
    useInsnSet.clear();
    secondOpnd = nullptr;
}

void RedundantUxtPattern::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn)
        {
            Init();
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

bool LocalVarSaveInsnPattern::CheckFirstInsn(const Insn &firstInsn)
{
    MOperator mOp = firstInsn.GetMachineOpcode();
    if (mOp != MOP_xmovrr && mOp != MOP_wmovrr) {
        return false;
    }
    firstInsnSrcOpnd = &(firstInsn.GetOperand(kInsnSecondOpnd));
    RegOperand *firstInsnSrcReg = static_cast<RegOperand *>(firstInsnSrcOpnd);
    if (firstInsnSrcReg->GetRegisterNumber() != R0) {
        return false;
    }
    firstInsnDestOpnd = &(firstInsn.GetOperand(kInsnFirstOpnd));
    RegOperand *firstInsnDestReg = static_cast<RegOperand *>(firstInsnDestOpnd);
    if (firstInsnDestReg->IsPhysicalRegister()) {
        return false;
    }
    return true;
}

bool LocalVarSaveInsnPattern::CheckSecondInsn()
{
    MOperator mOp = secondInsn->GetMachineOpcode();
    if (mOp != MOP_wstr && mOp != MOP_xstr) {
        return false;
    }
    secondInsnSrcOpnd = &(secondInsn->GetOperand(kInsnFirstOpnd));
    if (!RegOperand::IsSameReg(*firstInsnDestOpnd, *secondInsnSrcOpnd)) {
        return false;
    }
    /* check memOperand is stack memOperand, and x0 is stored in localref var region */
    secondInsnDestOpnd = &(secondInsn->GetOperand(kInsnSecondOpnd));
    MemOperand *secondInsnDestMem = static_cast<MemOperand *>(secondInsnDestOpnd);
    RegOperand *baseReg = secondInsnDestMem->GetBaseRegister();
    RegOperand *indexReg = secondInsnDestMem->GetIndexRegister();
    if ((baseReg == nullptr) || !(cgFunc.IsFrameReg(*baseReg)) || (indexReg != nullptr)) {
        return false;
    }
    return true;
}

bool LocalVarSaveInsnPattern::CheckAndGetUseInsn(Insn &firstInsn)
{
    InsnSet useInsnSet = cgFunc.GetRD()->FindUseForRegOpnd(firstInsn, kInsnFirstOpnd, false);
    if (useInsnSet.size() != 2) { /* 2 for secondInsn and another useInsn */
        return false;
    }

    /* useInsnSet includes secondInsn and another useInsn */
    for (auto tmpUseInsn : useInsnSet) {
        if (tmpUseInsn->GetId() != secondInsn->GetId()) {
            useInsn = tmpUseInsn;
            break;
        }
    }
    return true;
}

bool LocalVarSaveInsnPattern::CheckLiveRange(const Insn &firstInsn)
{
    uint32 maxInsnNO = cgFunc.GetRD()->GetMaxInsnNO();
    uint32 useInsnID = useInsn->GetId();
    uint32 defInsnID = firstInsn.GetId();
    uint32 distance = useInsnID > defInsnID ? useInsnID - defInsnID : defInsnID - useInsnID;
    float liveRangeProportion = static_cast<float>(distance) / maxInsnNO;
    /* 0.3 is a balance for real optimization effect */
    if (liveRangeProportion < 0.3) {
        return false;
    }
    return true;
}

bool LocalVarSaveInsnPattern::CheckCondition(Insn &firstInsn)
{
    secondInsn = firstInsn.GetNext();
    if (secondInsn == nullptr) {
        return false;
    }
    /* check firstInsn is : mov vreg, R0; */
    if (!CheckFirstInsn(firstInsn)) {
        return false;
    }
    /* check the secondInsn is : str vreg, stackMem */
    if (!CheckSecondInsn()) {
        return false;
    }
    /* find the uses of the vreg */
    if (!CheckAndGetUseInsn(firstInsn)) {
        return false;
    }
    /* simulate live range using insn distance */
    if (!CheckLiveRange(firstInsn)) {
        return false;
    }
    RegOperand *firstInsnDestReg = static_cast<RegOperand *>(firstInsnDestOpnd);
    regno_t firstInsnDestRegNO = firstInsnDestReg->GetRegisterNumber();
    InsnSet defInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(*useInsn, firstInsnDestRegNO, true);
    if (defInsnSet.size() != 1) {
        return false;
    }
    DEBUG_ASSERT((*(defInsnSet.begin()))->GetId() == firstInsn.GetId(), "useInsn has only one define Insn : firstInsn");
    /* check whether the stack mem is changed or not */
    MemOperand *secondInsnDestMem = static_cast<MemOperand *>(secondInsnDestOpnd);
    int64 memOffset = secondInsnDestMem->GetOffsetImmediate()->GetOffsetValue();
    InsnSet memDefInsnSet = cgFunc.GetRD()->FindDefForMemOpnd(*useInsn, memOffset, true);
    if (memDefInsnSet.size() != 1) {
        return false;
    }
    if ((*(memDefInsnSet.begin()))->GetId() != secondInsn->GetId()) {
        return false;
    }
    /* check whether has call between use and def */
    if (!cgFunc.GetRD()->HasCallBetweenDefUse(firstInsn, *useInsn)) {
        return false;
    }
    return true;
}

void LocalVarSaveInsnPattern::Optimize(Insn &insn)
{
    /* insert ldr insn before useInsn */
    MOperator ldrOpCode = secondInsnSrcOpnd->GetSize() == k64BitSize ? MOP_xldr : MOP_wldr;
    Insn &ldrInsn = cgFunc.GetInsnBuilder()->BuildInsn(ldrOpCode, *secondInsnSrcOpnd, *secondInsnDestOpnd);
    ldrInsn.SetId(useInsn->GetId() - 1);
    useInsn->GetBB()->InsertInsnBefore(*useInsn, ldrInsn);
    cgFunc.GetRD()->UpdateInOut(*useInsn->GetBB(), true);
    secondInsn->SetOperand(kInsnFirstOpnd, *firstInsnSrcOpnd);
    BB *saveInsnBB = insn.GetBB();
    saveInsnBB->RemoveInsn(insn);
    cgFunc.GetRD()->UpdateInOut(*saveInsnBB, true);
}

void LocalVarSaveInsnPattern::Init()
{
    firstInsnSrcOpnd = nullptr;
    firstInsnDestOpnd = nullptr;
    secondInsnSrcOpnd = nullptr;
    secondInsnDestOpnd = nullptr;
    useInsn = nullptr;
    secondInsn = nullptr;
}

void LocalVarSaveInsnPattern::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        if (bb->IsCleanup()) {
            continue;
        }
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (!insn->IsCall()) {
                continue;
            }
            Insn *firstInsn = insn->GetNextMachineInsn();
            if (firstInsn == nullptr) {
                continue;
            }
            Init();
            if (!CheckCondition(*firstInsn)) {
                continue;
            }
            Optimize(*firstInsn);
        }
    }
}

void ExtendShiftOptPattern::SetExMOpType(const Insn &use)
{
    MOperator op = use.GetMachineOpcode();
    switch (op) {
        case MOP_xaddrrr:
        case MOP_xxwaddrrre:
        case MOP_xaddrrrs: {
            exMOpType = kExAdd;
            break;
        }
        case MOP_waddrrr:
        case MOP_wwwaddrrre:
        case MOP_waddrrrs: {
            exMOpType = kEwAdd;
            break;
        }
        case MOP_xsubrrr:
        case MOP_xxwsubrrre:
        case MOP_xsubrrrs: {
            exMOpType = kExSub;
            break;
        }
        case MOP_wsubrrr:
        case MOP_wwwsubrrre:
        case MOP_wsubrrrs: {
            exMOpType = kEwSub;
            break;
        }
        case MOP_xcmnrr:
        case MOP_xwcmnrre:
        case MOP_xcmnrrs: {
            exMOpType = kExCmn;
            break;
        }
        case MOP_wcmnrr:
        case MOP_wwcmnrre:
        case MOP_wcmnrrs: {
            exMOpType = kEwCmn;
            break;
        }
        case MOP_xcmprr:
        case MOP_xwcmprre:
        case MOP_xcmprrs: {
            exMOpType = kExCmp;
            break;
        }
        case MOP_wcmprr:
        case MOP_wwcmprre:
        case MOP_wcmprrs: {
            exMOpType = kEwCmp;
            break;
        }
        default: {
            exMOpType = kExUndef;
        }
    }
}

void ExtendShiftOptPattern::SetLsMOpType(const Insn &use)
{
    MOperator op = use.GetMachineOpcode();
    switch (op) {
        case MOP_xaddrrr:
        case MOP_xaddrrrs: {
            lsMOpType = kLxAdd;
            break;
        }
        case MOP_waddrrr:
        case MOP_waddrrrs: {
            lsMOpType = kLwAdd;
            break;
        }
        case MOP_xsubrrr:
        case MOP_xsubrrrs: {
            lsMOpType = kLxSub;
            break;
        }
        case MOP_wsubrrr:
        case MOP_wsubrrrs: {
            lsMOpType = kLwSub;
            break;
        }
        case MOP_xcmnrr:
        case MOP_xcmnrrs: {
            lsMOpType = kLxCmn;
            break;
        }
        case MOP_wcmnrr:
        case MOP_wcmnrrs: {
            lsMOpType = kLwCmn;
            break;
        }
        case MOP_xcmprr:
        case MOP_xcmprrs: {
            lsMOpType = kLxCmp;
            break;
        }
        case MOP_wcmprr:
        case MOP_wcmprrs: {
            lsMOpType = kLwCmp;
            break;
        }
        case MOP_xeorrrr:
        case MOP_xeorrrrs: {
            lsMOpType = kLxEor;
            break;
        }
        case MOP_weorrrr:
        case MOP_weorrrrs: {
            lsMOpType = kLwEor;
            break;
        }
        case MOP_xinegrr:
        case MOP_xinegrrs: {
            lsMOpType = kLxNeg;
            replaceIdx = kInsnSecondOpnd;
            break;
        }
        case MOP_winegrr:
        case MOP_winegrrs: {
            lsMOpType = kLwNeg;
            replaceIdx = kInsnSecondOpnd;
            break;
        }
        case MOP_xiorrrr:
        case MOP_xiorrrrs: {
            lsMOpType = kLxIor;
            break;
        }
        case MOP_wiorrrr:
        case MOP_wiorrrrs: {
            lsMOpType = kLwIor;
            break;
        }
        default: {
            lsMOpType = kLsUndef;
        }
    }
}

void ExtendShiftOptPattern::SelectExtendOrShift(const Insn &def)
{
    MOperator op = def.GetMachineOpcode();
    switch (op) {
        case MOP_xsxtb32:
        case MOP_xsxtb64:
            extendOp = ExtendShiftOperand::kSXTB;
            break;
        case MOP_xsxth32:
        case MOP_xsxth64:
            extendOp = ExtendShiftOperand::kSXTH;
            break;
        case MOP_xsxtw64:
            extendOp = ExtendShiftOperand::kSXTW;
            break;
        case MOP_xuxtb32:
            extendOp = ExtendShiftOperand::kUXTB;
            break;
        case MOP_xuxth32:
            extendOp = ExtendShiftOperand::kUXTH;
            break;
        case MOP_xuxtw64:
            extendOp = ExtendShiftOperand::kUXTW;
            break;
        case MOP_wlslrri5:
        case MOP_xlslrri6:
            shiftOp = BitShiftOperand::kLSL;
            break;
        case MOP_xlsrrri6:
        case MOP_wlsrrri5:
            shiftOp = BitShiftOperand::kLSR;
            break;
        case MOP_xasrrri6:
        case MOP_wasrrri5:
            shiftOp = BitShiftOperand::kASR;
            break;
        default: {
            extendOp = ExtendShiftOperand::kUndef;
            shiftOp = BitShiftOperand::kUndef;
        }
    }
}

/* first use must match SelectExtendOrShift */
bool ExtendShiftOptPattern::CheckDefUseInfo(Insn &use, uint32 size)
{
    auto &regOperand = static_cast<RegOperand &>(defInsn->GetOperand(kInsnFirstOpnd));
    Operand &defSrcOpnd = defInsn->GetOperand(kInsnSecondOpnd);
    CHECK_FATAL(defSrcOpnd.IsRegister(), "defSrcOpnd must be register!");
    auto &regDefSrc = static_cast<RegOperand &>(defSrcOpnd);
    if (regDefSrc.IsPhysicalRegister()) {
        return false;
    }
    /*
     * has Implict cvt
     *
     * avoid cases as following:
     *   lsr  x2, x2, #8
     *   ubfx w2, x2, #0, #32                lsr  x2, x2, #8
     *   eor  w0, w0, w2           ===>      eor  w0, w0, x2     ==\=>  eor w0, w0, w2, LSR #8
     *
     * the truncation causes the wrong value by shift right
     * shift left does not matter
     */
    auto &useDefOpnd = static_cast<RegOperand &>(use.GetOperand(kInsnFirstOpnd));
    if ((shiftOp != BitShiftOperand::kUndef || extendOp != ExtendShiftOperand::kUndef) &&
        (regDefSrc.GetSize() > regOperand.GetSize() || useDefOpnd.GetSize() != size)) {
        return false;
    }
    if ((shiftOp == BitShiftOperand::kLSR || shiftOp == BitShiftOperand::kASR) && (defSrcOpnd.GetSize() > size)) {
        return false;
    }
    regno_t defSrcRegNo = regDefSrc.GetRegisterNumber();
    /* check regDefSrc */
    InsnSet defSrcSet = cgFunc.GetRD()->FindDefForRegOpnd(use, defSrcRegNo, true);
    /* The first defSrcInsn must be closest to useInsn */
    if (defSrcSet.empty()) {
        return false;
    }
    Insn *defSrcInsn = *defSrcSet.begin();
    const InsnDesc *md = defSrcInsn->GetDesc();
    if ((size != regOperand.GetSize()) && md->IsMove()) {
        return false;
    }
    if (defInsn->GetBB() == use.GetBB()) {
        /* check replace reg def between defInsn and currInsn */
        Insn *tmpInsn = defInsn->GetNext();
        while (tmpInsn != &use) {
            if (tmpInsn == defSrcInsn || tmpInsn == nullptr) {
                return false;
            }
            tmpInsn = tmpInsn->GetNext();
        }
    } else { /* def use not in same BB */
        if (defSrcInsn->GetBB() != defInsn->GetBB()) {
            return false;
        }
        if (defSrcInsn->GetId() > defInsn->GetId()) {
            return false;
        }
    }
    /* case:
     * lsl w0, w0, #5
     * eor w0, w2, w0
     * --->
     * eor w0, w2, w0, lsl 5
     */
    if (defSrcInsn == defInsn) {
        InsnSet replaceRegUseSet = cgFunc.GetRD()->FindUseForRegOpnd(*defInsn, defSrcRegNo, true);
        if (replaceRegUseSet.size() != k1BitSize) {
            return false;
        }
        removeDefInsn = true;
    }
    return true;
}

/* Check whether ExtendShiftOptPattern optimization can be performed. */
ExtendShiftOptPattern::SuffixType ExtendShiftOptPattern::CheckOpType(const Operand &lastOpnd) const
{
    /* Assign values to useType and defType. */
    uint32 useType = ExtendShiftOptPattern::kNoSuffix;
    uint32 defType = shiftOp;
    if (extendOp != ExtendShiftOperand::kUndef) {
        defType = ExtendShiftOptPattern::kExten;
    }
    if (lastOpnd.IsOpdShift()) {
        BitShiftOperand lastShiftOpnd = static_cast<const BitShiftOperand &>(lastOpnd);
        useType = lastShiftOpnd.GetShiftOp();
    } else if (lastOpnd.IsOpdExtend()) {
        ExtendShiftOperand lastExtendOpnd = static_cast<const ExtendShiftOperand &>(lastOpnd);
        useType = ExtendShiftOptPattern::kExten;
        /* two insn is exten and exten ,value is exten(oneself) */
        if (useType == defType && extendOp != lastExtendOpnd.GetExtendOp()) {
            return ExtendShiftOptPattern::kNoSuffix;
        }
    }
    return doOptimize[useType][defType];
}

/* new Insn extenType:
 * =====================
 * (useMop)   (defMop) (newmop)
 * | nosuffix |  all  | all|
 * | exten    |  ex   | ex |
 * |  ls      |  ex   | ls |
 * |  asr     |  !asr | F  |
 * |  !asr    |  asr  | F  |
 * (useMop)   (defMop)
 * =====================
 */
void ExtendShiftOptPattern::ReplaceUseInsn(Insn &use, const Insn &def, uint32 amount)
{
    AArch64CGFunc &a64CGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    uint32 lastIdx = use.GetOperandSize() - k1BitSize;
    Operand &lastOpnd = use.GetOperand(lastIdx);
    ExtendShiftOptPattern::SuffixType optType = CheckOpType(lastOpnd);
    Operand *shiftOpnd = nullptr;
    if (optType == ExtendShiftOptPattern::kNoSuffix) {
        return;
    } else if (optType == ExtendShiftOptPattern::kExten) {
        replaceOp = exMOpTable[exMOpType];
        if (amount > k4BitSize) {
            return;
        }
        shiftOpnd = &a64CGFunc.CreateExtendShiftOperand(extendOp, amount, static_cast<int32>(k64BitSize));
    } else {
        replaceOp = lsMOpTable[lsMOpType];
        if (amount >= k32BitSize) {
            return;
        }
        shiftOpnd = &a64CGFunc.CreateBitShiftOperand(shiftOp, amount, static_cast<int32>(k64BitSize));
    }
    if (replaceOp == MOP_undef) {
        return;
    }

    Insn *replaceUseInsn = nullptr;
    Operand &firstOpnd = use.GetOperand(kInsnFirstOpnd);
    Operand *secondOpnd = &use.GetOperand(kInsnSecondOpnd);
    if (replaceIdx == kInsnSecondOpnd) { /* replace neg insn */
        secondOpnd = &def.GetOperand(kInsnSecondOpnd);
        replaceUseInsn = &cgFunc.GetInsnBuilder()->BuildInsn(replaceOp, firstOpnd, *secondOpnd, *shiftOpnd);
    } else {
        Operand &thirdOpnd = def.GetOperand(kInsnSecondOpnd);
        replaceUseInsn = &cgFunc.GetInsnBuilder()->BuildInsn(replaceOp, firstOpnd, *secondOpnd, thirdOpnd, *shiftOpnd);
    }
    use.GetBB()->ReplaceInsn(use, *replaceUseInsn);
    if (GLOBAL_DUMP) {
        LogInfo::MapleLogger() << ">>>>>>> In ExtendShiftOptPattern : <<<<<<<\n";
        LogInfo::MapleLogger() << "=======ReplaceInsn :\n";
        use.Dump();
        LogInfo::MapleLogger() << "=======NewInsn :\n";
        replaceUseInsn->Dump();
    }
    if (removeDefInsn) {
        if (GLOBAL_DUMP) {
            LogInfo::MapleLogger() << ">>>>>>> In ExtendShiftOptPattern : <<<<<<<\n";
            LogInfo::MapleLogger() << "=======RemoveDefInsn :\n";
            defInsn->Dump();
        }
        defInsn->GetBB()->RemoveInsn(*defInsn);
    }
    cgFunc.GetRD()->InitGenUse(*defInsn->GetBB(), false);
    cgFunc.GetRD()->UpdateInOut(*use.GetBB(), true);
    newInsn = replaceUseInsn;
    optSuccess = true;
}

/*
 * pattern1:
 * UXTB/UXTW X0, W1              <---- def x0
 * ....                          <---- (X0 not used)
 * AND/SUB/EOR X0, X1, X0        <---- use x0
 * ======>
 * AND/SUB/EOR X0, X1, W1 UXTB/UXTW
 *
 * pattern2:
 * LSL/LSR X0, X1, #8
 * ....(X0 not used)
 * AND/SUB/EOR X0, X1, X0
 * ======>
 * AND/SUB/EOR X0, X1, X1 LSL/LSR #8
 */
void ExtendShiftOptPattern::Optimize(Insn &insn)
{
    uint32 amount = 0;
    uint32 offset = 0;
    uint32 lastIdx = insn.GetOperandSize() - k1BitSize;
    Operand &lastOpnd = insn.GetOperand(lastIdx);
    if (lastOpnd.IsOpdShift()) {
        BitShiftOperand &lastShiftOpnd = static_cast<BitShiftOperand &>(lastOpnd);
        amount = lastShiftOpnd.GetShiftAmount();
    } else if (lastOpnd.IsOpdExtend()) {
        ExtendShiftOperand &lastExtendOpnd = static_cast<ExtendShiftOperand &>(lastOpnd);
        amount = lastExtendOpnd.GetShiftAmount();
    }
    if (shiftOp != BitShiftOperand::kUndef) {
        ImmOperand &immOpnd = static_cast<ImmOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
        offset = static_cast<uint32>(immOpnd.GetValue());
    }
    amount += offset;

    ReplaceUseInsn(insn, *defInsn, amount);
}

void ExtendShiftOptPattern::DoExtendShiftOpt(Insn &insn)
{
    Init();
    if (!CheckCondition(insn)) {
        return;
    }
    Optimize(insn);
    if (optSuccess) {
        DoExtendShiftOpt(*newInsn);
    }
}

/* check and set:
 * exMOpType, lsMOpType, extendOp, shiftOp, defInsn
 */
bool ExtendShiftOptPattern::CheckCondition(Insn &insn)
{
    SetLsMOpType(insn);
    SetExMOpType(insn);
    if ((exMOpType == kExUndef) && (lsMOpType == kLsUndef)) {
        return false;
    }
    RegOperand &regOperand = static_cast<RegOperand &>(insn.GetOperand(replaceIdx));
    if (regOperand.IsPhysicalRegister()) {
        return false;
    }
    regno_t regNo = regOperand.GetRegisterNumber();
    InsnSet regDefInsnSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, regNo, true);
    if (regDefInsnSet.size() != k1BitSize) {
        return false;
    }
    defInsn = *regDefInsnSet.begin();
    CHECK_FATAL((defInsn != nullptr), "defInsn is null!");

    SelectExtendOrShift(*defInsn);
    /* defInsn must be shift or extend */
    if ((extendOp == ExtendShiftOperand::kUndef) && (shiftOp == BitShiftOperand::kUndef)) {
        return false;
    }
    return CheckDefUseInfo(insn, regOperand.GetSize());
}

void ExtendShiftOptPattern::Init()
{
    replaceOp = MOP_undef;
    extendOp = ExtendShiftOperand::kUndef;
    shiftOp = BitShiftOperand::kUndef;
    defInsn = nullptr;
    replaceIdx = kInsnThirdOpnd;
    newInsn = nullptr;
    optSuccess = false;
    removeDefInsn = false;
    exMOpType = kExUndef;
    lsMOpType = kLsUndef;
}

void ExtendShiftOptPattern::Run()
{
    if (!cgFunc.GetMirModule().IsCModule()) {
        return;
    }
    FOR_ALL_BB_REV(bb, &cgFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            DoExtendShiftOpt(*insn);
        }
    }
}

void ExtenToMovPattern::Run()
{
    if (!cgFunc.GetMirModule().IsCModule()) {
        return;
    }
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

/* Check for Implicit uxtw */
bool ExtenToMovPattern::CheckHideUxtw(const Insn &insn, regno_t regno) const
{
    const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
    if (md->IsMove()) {
        return false;
    }
    uint32 optSize = insn.GetOperandSize();
    for (uint32 i = 0; i < optSize; ++i) {
        if (regno == static_cast<RegOperand &>(insn.GetOperand(i)).GetRegisterNumber()) {
            auto *curOpndDescription = md->GetOpndDes(i);
            if (curOpndDescription->IsDef() && curOpndDescription->GetSize() == k32BitSize) {
                return true;
            }
            break;
        }
    }
    return false;
}

bool ExtenToMovPattern::CheckUxtw(Insn &insn)
{
    if (insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize &&
        insn.GetOperand(kInsnSecondOpnd).GetSize() == k32BitSize) {
        DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "is not Register");
        regno_t regno = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
        InsnSet preDef = cgFunc.GetRD()->FindDefForRegOpnd(insn, kInsnSecondOpnd, false);
        if (preDef.empty()) {
            return false;
        }
        for (auto defInsn : preDef) {
            if (!CheckHideUxtw(*defInsn, regno)) {
                return false;
            }
        }
        replaceMop = MOP_xmovrr_uxtw;
        return true;
    }
    return false;
}

bool ExtenToMovPattern::CheckSrcReg(Insn &insn, regno_t srcRegNo, uint32 validNum)
{
    InsnSet srcDefSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, srcRegNo, true);
    for (auto defInsn : srcDefSet) {
        CHECK_FATAL((defInsn != nullptr), "defInsn is null!");
        MOperator mOp = defInsn->GetMachineOpcode();
        switch (mOp) {
            case MOP_wiorrri12:
            case MOP_weorrri12: {
                /* check immVal if mop is OR */
                ImmOperand &imm = static_cast<ImmOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
                auto bitNum = static_cast<uint32>(imm.GetValue());
                if ((bitNum >> validNum) != 0) {
                    return false;
                }
                break;
            }
            case MOP_wandrri12: {
                /* check defSrcReg */
                RegOperand &defSrcRegOpnd = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
                regno_t defSrcRegNo = defSrcRegOpnd.GetRegisterNumber();
                if (!CheckSrcReg(*defInsn, defSrcRegNo, validNum)) {
                    return false;
                }
                break;
            }
            case MOP_wandrrr: {
                /* check defSrcReg */
                RegOperand &defSrcRegOpnd1 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
                RegOperand &defSrcRegOpnd2 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
                regno_t defSrcRegNo1 = defSrcRegOpnd1.GetRegisterNumber();
                regno_t defSrcRegNo2 = defSrcRegOpnd2.GetRegisterNumber();
                if (!CheckSrcReg(*defInsn, defSrcRegNo1, validNum) && !CheckSrcReg(*defInsn, defSrcRegNo2, validNum)) {
                    return false;
                }
                break;
            }
            case MOP_wiorrrr:
            case MOP_weorrrr: {
                /* check defSrcReg */
                RegOperand &defSrcRegOpnd1 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
                RegOperand &defSrcRegOpnd2 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
                regno_t defSrcRegNo1 = defSrcRegOpnd1.GetRegisterNumber();
                regno_t defSrcRegNo2 = defSrcRegOpnd2.GetRegisterNumber();
                if (!CheckSrcReg(*defInsn, defSrcRegNo1, validNum) || !CheckSrcReg(*defInsn, defSrcRegNo2, validNum)) {
                    return false;
                }
                break;
            }
            case MOP_wldrb: {
                if (validNum != k8BitSize) {
                    return false;
                }
                break;
            }
            case MOP_wldrh: {
                if (validNum != k16BitSize) {
                    return false;
                }
                break;
            }
            default:
                return false;
        }
    }
    return true;
}

bool ExtenToMovPattern::BitNotAffected(Insn &insn, uint32 validNum)
{
    RegOperand &firstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    RegOperand &secondOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    regno_t desRegNo = firstOpnd.GetRegisterNumber();
    regno_t srcRegNo = secondOpnd.GetRegisterNumber();
    InsnSet desDefSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, desRegNo, true);
    /* desReg is not redefined */
    if (!desDefSet.empty()) {
        return false;
    }
    if (!CheckSrcReg(insn, srcRegNo, validNum)) {
        return false;
    }
    replaceMop = MOP_wmovrr;
    return true;
}

bool ExtenToMovPattern::CheckCondition(Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    switch (mOp) {
        case MOP_xuxtw64:
            return CheckUxtw(insn);
        case MOP_xuxtb32:
            return BitNotAffected(insn, k8BitSize);
        case MOP_xuxth32:
            return BitNotAffected(insn, k16BitSize);
        default:
            return false;
    }
}

/* No initialization required */
void ExtenToMovPattern::Init()
{
    replaceMop = MOP_undef;
}

void ExtenToMovPattern::Optimize(Insn &insn)
{
    insn.SetMOP(AArch64CG::kMd[replaceMop]);
}

void SameDefPattern::Run()
{
    FOR_ALL_BB_REV(bb, &cgFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (!CheckCondition(*insn) || !bb->GetEhPreds().empty()) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

void SameDefPattern::Init()
{
    currInsn = nullptr;
    sameInsn = nullptr;
}

bool SameDefPattern::CheckCondition(Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    if (insn.GetBB()->GetPreds().size() > k1BitSize) {
        return false;
    }
    if (insn.GetBB()->HasCall()) {
        return false;
    }
    return (mOp == MOP_wcmprr) || (mOp == MOP_xcmprr) || (mOp == MOP_xwcmprre) || (mOp == MOP_xcmprrs);
}

void SameDefPattern::Optimize(Insn &insn)
{
    InsnSet sameDefSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, 0, false);
    if (sameDefSet.size() != k1BitSize) {
        return;
    }
    Insn *sameDefInsn = *sameDefSet.begin();
    if (sameDefInsn == nullptr) {
        return;
    }
    currInsn = &insn;
    sameInsn = sameDefInsn;
    if (!IsSameDef()) {
        return;
    }
    if (GLOBAL_DUMP) {
        LogInfo::MapleLogger() << ">>>>>>> In SameDefPattern : <<<<<<<\n";
        LogInfo::MapleLogger() << "=======remove insn: \n";
        insn.Dump();
        LogInfo::MapleLogger() << "=======sameDef insn: \n";
        sameDefInsn->Dump();
    }
    insn.GetBB()->RemoveInsn(insn);
}

bool SameDefPattern::IsSameDef()
{
    if (!CheckCondition(*sameInsn)) {
        return false;
    }
    if (currInsn == sameInsn) {
        return false;
    }
    if (currInsn->GetMachineOpcode() != sameInsn->GetMachineOpcode()) {
        return false;
    }
    for (uint32 i = k1BitSize; i < currInsn->GetOperandSize(); ++i) {
        Operand &opnd0 = currInsn->GetOperand(i);
        Operand &opnd1 = sameInsn->GetOperand(i);
        if (!IsSameOperand(opnd0, opnd1)) {
            return false;
        }
    }
    return true;
}

bool SameDefPattern::IsSameOperand(Operand &opnd0, Operand &opnd1)
{
    if (opnd0.IsRegister()) {
        CHECK_FATAL(opnd1.IsRegister(), "must be RegOperand!");
        RegOperand &regOpnd0 = static_cast<RegOperand &>(opnd0);
        RegOperand &regOpnd1 = static_cast<RegOperand &>(opnd1);
        if (!RegOperand::IsSameReg(regOpnd0, regOpnd1)) {
            return false;
        }
        regno_t regNo = regOpnd0.GetRegisterNumber();
        /* src reg not redefined between sameInsn and currInsn */
        if (SrcRegIsRedefined(regNo)) {
            return false;
        }
    } else if (opnd0.IsOpdShift()) {
        CHECK_FATAL(opnd1.IsOpdShift(), "must be ShiftOperand!");
        BitShiftOperand &shiftOpnd0 = static_cast<BitShiftOperand &>(opnd0);
        BitShiftOperand &shiftOpnd1 = static_cast<BitShiftOperand &>(opnd1);
        if (shiftOpnd0.GetShiftAmount() != shiftOpnd1.GetShiftAmount()) {
            return false;
        }
    } else if (opnd0.IsOpdExtend()) {
        CHECK_FATAL(opnd1.IsOpdExtend(), "must be ExtendOperand!");
        ExtendShiftOperand &extendOpnd0 = static_cast<ExtendShiftOperand &>(opnd0);
        ExtendShiftOperand &extendOpnd1 = static_cast<ExtendShiftOperand &>(opnd1);
        if (extendOpnd0.GetShiftAmount() != extendOpnd1.GetShiftAmount()) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool SameDefPattern::SrcRegIsRedefined(regno_t regNo)
{
    AArch64ReachingDefinition *a64RD = static_cast<AArch64ReachingDefinition *>(cgFunc.GetRD());
    if (currInsn->GetBB() == sameInsn->GetBB()) {
        FOR_BB_INSNS(insn, currInsn->GetBB())
        {
            if (insn->GetMachineOpcode() == MOP_xbl) {
                return true;
            }
        }
        if (!a64RD->FindRegDefBetweenInsn(regNo, sameInsn, currInsn).empty()) {
            return true;
        }
    } else if (a64RD->HasRegDefBetweenInsnGlobal(regNo, *sameInsn, *currInsn)) {
        return true;
    }
    return false;
}

void AndCbzPattern::Init()
{
    prevInsn = nullptr;
}

bool AndCbzPattern::IsAdjacentArea(Insn &prev, Insn &curr) const
{
    if (prev.GetBB() == curr.GetBB()) {
        return true;
    }
    for (auto *succ : prev.GetBB()->GetSuccs()) {
        if (succ == curr.GetBB()) {
            return true;
        }
    }
    return false;
}

bool AndCbzPattern::CheckCondition(Insn &insn)
{
    auto *aarch64RD = static_cast<AArch64ReachingDefinition *>(cgFunc.GetRD());
    MOperator mOp = insn.GetMachineOpcode();
    if ((mOp != MOP_wcbz) && (mOp != MOP_xcbz) && (mOp != MOP_wcbnz) && (mOp != MOP_xcbnz)) {
        return false;
    }
    regno_t regNo = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    InsnSet defSet = cgFunc.GetRD()->FindDefForRegOpnd(insn, regNo, true);
    if (defSet.size() != k1BitSize) {
        return false;
    }
    prevInsn = *defSet.begin();
    if (prevInsn->GetMachineOpcode() != MOP_wandrri12 && prevInsn->GetMachineOpcode() != MOP_xandrri13) {
        return false;
    }
    if (!IsAdjacentArea(*prevInsn, insn)) {
        return false;
    }
    regno_t propRegNo = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
    if (prevInsn->GetBB() == insn.GetBB() && !(aarch64RD->FindRegDefBetweenInsn(propRegNo, prevInsn, &insn).empty())) {
        return false;
    }
    if (prevInsn->GetBB() != insn.GetBB() && aarch64RD->HasRegDefBetweenInsnGlobal(propRegNo, *prevInsn, insn)) {
        return false;
    }
    if (!(cgFunc.GetRD()->FindUseForRegOpnd(insn, regNo, true).empty())) {
        return false;
    }
    return true;
}

int64 AndCbzPattern::CalculateLogValue(int64 val) const
{
    return (__builtin_popcountll(static_cast<uint64>(val)) == 1) ? (__builtin_ffsll(val) - 1) : -1;
}

void AndCbzPattern::Optimize(Insn &insn)
{
    BB *bb = insn.GetBB();
    auto &aarchFunc = static_cast<AArch64CGFunc &>(cgFunc);
    auto &andImm = static_cast<ImmOperand &>(prevInsn->GetOperand(kInsnThirdOpnd));
    int64 tbzVal = CalculateLogValue(andImm.GetValue());
    if (tbzVal < 0) {
        return;
    }
    MOperator mOp = insn.GetMachineOpcode();
    MOperator newMop = MOP_undef;
    switch (mOp) {
        case MOP_wcbz:
            newMop = MOP_wtbz;
            break;
        case MOP_wcbnz:
            newMop = MOP_wtbnz;
            break;
        case MOP_xcbz:
            newMop = MOP_xtbz;
            break;
        case MOP_xcbnz:
            newMop = MOP_xtbnz;
            break;
        default:
            CHECK_FATAL(false, "must be cbz/cbnz");
            break;
    }
    auto &label = static_cast<LabelOperand &>(insn.GetOperand(kInsnSecondOpnd));
    ImmOperand &tbzImm = aarchFunc.CreateImmOperand(tbzVal, k8BitSize, false);
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newMop, prevInsn->GetOperand(kInsnSecondOpnd), tbzImm, label);
    if (!VERIFY_INSN(&newInsn)) {
        return;
    }
    newInsn.SetId(insn.GetId());
    bb->ReplaceInsn(insn, newInsn);
    if (GLOBAL_DUMP) {
        LogInfo::MapleLogger() << ">>>>>>> In AndCbzPattern : <<<<<<<\n";
        LogInfo::MapleLogger() << "=======PrevInsn :\n";
        LogInfo::MapleLogger() << "=======ReplaceInsn :\n";
        insn.Dump();
        LogInfo::MapleLogger() << "=======NewInsn :\n";
        newInsn.Dump();
    }
    cgFunc.GetRD()->UpdateInOut(*bb, true);
}

void AndCbzPattern::Run()
{
    Init();
    FOR_ALL_BB_REV(bb, &cgFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (!insn->IsMachineInstruction() || !CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

void SameRHSPropPattern::Init()
{
    prevInsn = nullptr;
    candidates = {MOP_waddrri12, MOP_xaddrri12, MOP_wsubrri12, MOP_xsubrri12,
                  MOP_wmovri32,  MOP_xmovri64,  MOP_wmovrr,    MOP_xmovrr};
}

bool SameRHSPropPattern::IsSameOperand(Operand *opnd1, Operand *opnd2) const
{
    if (opnd1 == nullptr && opnd2 == nullptr) {
        return true;
    } else if (opnd1 == nullptr || opnd2 == nullptr) {
        return false;
    }
    if (opnd1->IsRegister() && opnd2->IsRegister()) {
        return RegOperand::IsSameReg(*opnd1, *opnd2);
    } else if (opnd1->IsImmediate() && opnd2->IsImmediate()) {
        auto *immOpnd1 = static_cast<ImmOperand *>(opnd1);
        auto *immOpnd2 = static_cast<ImmOperand *>(opnd2);
        return (immOpnd1->GetSize() == immOpnd2->GetSize()) && (immOpnd1->GetValue() == immOpnd2->GetValue());
    }
    return false;
}

bool SameRHSPropPattern::FindSameRHSInsnInBB(Insn &insn)
{
    uint32 opndNum = insn.GetOperandSize();
    Operand *curRegOpnd = nullptr;
    Operand *curImmOpnd = nullptr;
    for (uint32 i = 0; i < opndNum; ++i) {
        if (insn.OpndIsDef(i)) {
            continue;
        }
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsRegister()) {
            curRegOpnd = &opnd;
        } else if (opnd.IsImmediate()) {
            auto &immOpnd = static_cast<ImmOperand &>(opnd);
            if (immOpnd.GetVary() == kUnAdjustVary) {
                return false;
            }
            curImmOpnd = &opnd;
        }
    }
    if (curRegOpnd == nullptr && curImmOpnd != nullptr && static_cast<ImmOperand *>(curImmOpnd)->IsZero()) {
        return false;
    }
    BB *bb = insn.GetBB();
    for (auto *cursor = insn.GetPrev(); cursor != nullptr && cursor != bb->GetFirstInsn(); cursor = cursor->GetPrev()) {
        if (!cursor->IsMachineInstruction()) {
            continue;
        }
        if (cursor->IsCall() && !cgFunc.IsAfterRegAlloc()) {
            return false;
        }
        if (cursor->GetMachineOpcode() != insn.GetMachineOpcode()) {
            continue;
        }
        uint32 candOpndNum = cursor->GetOperandSize();
        Operand *candRegOpnd = nullptr;
        Operand *candImmOpnd = nullptr;
        for (uint32 i = 0; i < candOpndNum; ++i) {
            Operand &opnd = cursor->GetOperand(i);
            if (cursor->OpndIsDef(i)) {
                continue;
            }
            if (opnd.IsRegister()) {
                candRegOpnd = &opnd;
            } else if (opnd.IsImmediate()) {
                auto &immOpnd = static_cast<ImmOperand &>(opnd);
                if (immOpnd.GetVary() == kUnAdjustVary) {
                    return false;
                }
                candImmOpnd = &opnd;
            }
        }
        if (IsSameOperand(curRegOpnd, candRegOpnd) && IsSameOperand(curImmOpnd, candImmOpnd)) {
            prevInsn = cursor;
            return true;
        }
    }
    return false;
}

bool SameRHSPropPattern::CheckCondition(Insn &insn)
{
    if (!insn.IsMachineInstruction()) {
        return false;
    }
    MOperator mOp = insn.GetMachineOpcode();
    if (std::find(candidates.begin(), candidates.end(), mOp) == candidates.end()) {
        return false;
    }
    if (!FindSameRHSInsnInBB(insn)) {
        return false;
    }
    CHECK_FATAL(prevInsn->GetOperand(kInsnFirstOpnd).IsRegister(), "prevInsn first operand must be register");
    if (prevInsn->GetOperand(kInsnSecondOpnd).IsRegister() &&
        RegOperand::IsSameReg(prevInsn->GetOperand(kInsnFirstOpnd), prevInsn->GetOperand(kInsnSecondOpnd))) {
        return false;
    }
    uint32 opndNum = prevInsn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = prevInsn->GetOperand(i);
        if (!opnd.IsRegister()) {
            continue;
        }
        regno_t regNO = static_cast<RegOperand &>(opnd).GetRegisterNumber();
        if (!(cgFunc.GetRD()->FindRegDefBetweenInsn(regNO, prevInsn->GetNext(), insn.GetPrev()).empty())) {
            return false;
        }
    }
    return true;
}

void SameRHSPropPattern::Optimize(Insn &insn)
{
    BB *bb = insn.GetBB();
    Operand &destOpnd = insn.GetOperand(kInsnFirstOpnd);
    uint32 bitSize = static_cast<RegOperand &>(destOpnd).GetSize();
    MOperator mOp = (bitSize == k64BitSize ? MOP_xmovrr : MOP_wmovrr);
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, destOpnd, prevInsn->GetOperand(kInsnFirstOpnd));
    newInsn.SetId(insn.GetId());
    bb->ReplaceInsn(insn, newInsn);
    if (GLOBAL_DUMP) {
        LogInfo::MapleLogger() << ">>>>>>> In SameRHSPropPattern : <<<<<<<\n";
        LogInfo::MapleLogger() << "=======PrevInsn :\n";
        LogInfo::MapleLogger() << "======= ReplaceInsn :\n";
        insn.Dump();
        LogInfo::MapleLogger() << "======= NewInsn :\n";
        newInsn.Dump();
    }
    cgFunc.GetRD()->UpdateInOut(*bb, true);
}

void SameRHSPropPattern::Run()
{
    Init();
    FOR_ALL_BB_REV(bb, &cgFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}
} /* namespace maplebe */
