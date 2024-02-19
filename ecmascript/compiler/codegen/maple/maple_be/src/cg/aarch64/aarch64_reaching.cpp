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

#include "aarch64_reaching.h"
#include "aarch64_cg.h"
namespace maplebe {
/* MCC_ClearLocalStackRef clear 1 stack slot, and MCC_DecRefResetPair clear 2 stack slot,
 * the stack positins cleared are recorded in callInsn->clearStackOffset
 */
constexpr short kFirstClearMemIndex = 0;
constexpr short kSecondClearMemIndex = 1;

/* insert pseudo insn for parameters definition */
void AArch64ReachingDefinition::InitStartGen()
{
    BB *bb = cgFunc->GetFirstBB();

    /* Parameters should be define first. */
    CCImpl &parmLocator = *static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateLocator(cgFunc->GetCurCallConvKind());
    CCLocInfo pLoc;
    for (uint32 i = 0; i < cgFunc->GetFunction().GetFormalCount(); ++i) {
        MIRType *type = cgFunc->GetFunction().GetNthParamType(i);
        (void)parmLocator.LocateNextParm(*type, pLoc, i == 0, cgFunc->GetFunction().GetMIRFuncType());
        if (pLoc.reg0 == 0) {
            /* If is a large frame, parameter addressing mode is based vreg:Vra. */
            continue;
        }

        uint64 symSize = cgFunc->GetBecommon().GetTypeSize(type->GetTypeIndex());
        if ((cgFunc->GetMirModule().GetSrcLang() == kSrcLangC) && (symSize > k8ByteSize)) {
            /* For C structure passing in one or two registers. */
            symSize = k8ByteSize;
        }
        RegType regType = (pLoc.reg0 < V0) ? kRegTyInt : kRegTyFloat;
        uint32 srcBitSize = ((symSize < k4ByteSize) ? k4ByteSize : symSize) * kBitsPerByte;

        MOperator mOp;
        if (regType == kRegTyInt) {
            if (srcBitSize <= k32BitSize) {
                mOp = MOP_pseudo_param_def_w;
            } else {
                mOp = MOP_pseudo_param_def_x;
            }
        } else {
            if (srcBitSize <= k32BitSize) {
                mOp = MOP_pseudo_param_def_s;
            } else {
                mOp = MOP_pseudo_param_def_d;
            }
        }

        AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);

        RegOperand &regOpnd =
            aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(pLoc.reg0), srcBitSize, regType);
        Insn &pseudoInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd);
        bb->InsertInsnBegin(pseudoInsn);
        pseudoInsns.emplace_back(&pseudoInsn);
        if (pLoc.reg1) {
            RegOperand &regOpnd1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(pLoc.reg1),
                                                                                   srcBitSize, regType);
            Insn &pseudoInsn1 = cgFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd1);
            bb->InsertInsnBegin(pseudoInsn1);
            pseudoInsns.emplace_back(&pseudoInsn1);
        }
        if (pLoc.reg2) {
            RegOperand &regOpnd2 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(pLoc.reg2),
                                                                                   srcBitSize, regType);
            Insn &pseudoInsn1 = cgFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd2);
            bb->InsertInsnBegin(pseudoInsn1);
            pseudoInsns.emplace_back(&pseudoInsn1);
        }
        if (pLoc.reg3) {
            RegOperand &regOpnd3 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(pLoc.reg3),
                                                                                   srcBitSize, regType);
            Insn &pseudoInsn1 = cgFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd3);
            bb->InsertInsnBegin(pseudoInsn1);
            pseudoInsns.emplace_back(&pseudoInsn1);
        }

        {
            /*
             * define memory address since store param may be transfered to stp and which with the short offset range.
             *  we can not get the correct definition before RA.
             *  example:
             *   add  x8, sp, #712
             *   stp  x0, x1, [x8]    // store param: _this Reg40_R313644
             *   stp  x2, x3, [x8,#16]    // store param: Reg41_R333743 Reg42_R333622
             *   stp  x4, x5, [x8,#32]    // store param: Reg43_R401297 Reg44_R313834
             *   str  x7, [x8,#48]    // store param: Reg46_R401297
             */
            MIRSymbol *sym = cgFunc->GetFunction().GetFormal(i);
            if (!sym->IsPreg()) {
                MIRSymbol *firstSym = cgFunc->GetFunction().GetFormal(i);
                const AArch64SymbolAlloc *firstSymLoc =
                    static_cast<AArch64SymbolAlloc *>(cgFunc->GetMemlayout()->GetSymAllocInfo(firstSym->GetStIndex()));
                int32 stOffset = cgFunc->GetBaseOffset(*firstSymLoc);
                MIRType *firstType = cgFunc->GetFunction().GetNthParamType(i);
                uint32 firstSymSize = cgFunc->GetBecommon().GetTypeSize(firstType->GetTypeIndex());
                uint32 firstStackSize = firstSymSize < k4ByteSize ? k4ByteSize : firstSymSize;

                MemOperand *memOpnd = aarchCGFunc->CreateStackMemOpnd(RFP, stOffset, firstStackSize * kBitsPerByte);
                MOperator mopTemp = firstStackSize <= k4ByteSize ? MOP_pseudo_param_store_w : MOP_pseudo_param_store_x;
                Insn &pseudoInsnTemp = cgFunc->GetInsnBuilder()->BuildInsn(mopTemp, *memOpnd);
                bb->InsertInsnBegin(pseudoInsnTemp);
                pseudoInsns.emplace_back(&pseudoInsnTemp);
            }
        }
    }

    /* if function has "bl  MCC_InitializeLocalStackRef", should define corresponding memory. */
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);

    for (uint32 i = 0; i < a64CGFunc->GetRefCount(); ++i) {
        MemOperand *memOpnd = a64CGFunc->CreateStackMemOpnd(
            RFP, static_cast<int32>(a64CGFunc->GetBeginOffset() + i * k8BitSize), k64BitSize);
        Insn &pseudoInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_ref_init_x, *memOpnd);

        bb->InsertInsnBegin(pseudoInsn);
        pseudoInsns.emplace_back(&pseudoInsn);
    }
}

/* insert pseudoInsns for ehBB, R0 and R1 are defined in pseudoInsns */
void AArch64ReachingDefinition::InitEhDefine(BB &bb)
{
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);

    /* Insert MOP_pseudo_eh_def_x R1. */
    RegOperand &regOpnd1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R1, k64BitSize, kRegTyInt);
    Insn &pseudoInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_eh_def_x, regOpnd1);
    bb.InsertInsnBegin(pseudoInsn);
    pseudoInsns.emplace_back(&pseudoInsn);

    /* insert MOP_pseudo_eh_def_x R0. */
    RegOperand &regOpnd2 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt);
    Insn &newPseudoInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_eh_def_x, regOpnd2);
    bb.InsertInsnBegin(newPseudoInsn);
    pseudoInsns.emplace_back(&newPseudoInsn);
}

/* insert pseudoInsns for return value R0/V0 */
void AArch64ReachingDefinition::AddRetPseudoInsn(BB &bb)
{
    AArch64reg regNO = static_cast<AArch64CGFunc *>(cgFunc)->GetReturnRegisterNumber();
    if (regNO == kInvalidRegNO) {
        return;
    }

    if (regNO == R0) {
        RegOperand &regOpnd =
            static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreatePhysicalRegisterOperand(regNO, k64BitSize, kRegTyInt);
        Insn &retInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_ret_int, regOpnd);
        bb.AppendInsn(retInsn);
        pseudoInsns.emplace_back(&retInsn);
    } else if (regNO == V0) {
        RegOperand &regOpnd =
            static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreatePhysicalRegisterOperand(regNO, k64BitSize, kRegTyFloat);
        Insn &retInsn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_ret_float, regOpnd);
        bb.AppendInsn(retInsn);
        pseudoInsns.emplace_back(&retInsn);
    }
}

void AArch64ReachingDefinition::AddRetPseudoInsns()
{
    uint32 exitBBSize = cgFunc->GetExitBBsVec().size();
    if (exitBBSize == 0) {
        if (cgFunc->GetLastBB()->GetPrev()->GetFirstStmt() == cgFunc->GetCleanupLabel() &&
            cgFunc->GetLastBB()->GetPrev()->GetPrev()) {
            AddRetPseudoInsn(*cgFunc->GetLastBB()->GetPrev()->GetPrev());
        } else {
            AddRetPseudoInsn(*cgFunc->GetLastBB()->GetPrev());
        }
    } else {
        for (uint32 i = 0; i < exitBBSize; ++i) {
            AddRetPseudoInsn(*cgFunc->GetExitBB(i));
        }
    }
}

void AArch64ReachingDefinition::GenAllAsmDefRegs(BB &bb, Insn &insn, uint32 index)
{
    for (auto reg : static_cast<ListOperand &>(insn.GetOperand(index)).GetOperands()) {
        regGen[bb.GetId()]->SetBit(static_cast<RegOperand *>(reg)->GetRegisterNumber());
    }
}

void AArch64ReachingDefinition::GenAllAsmUseRegs(BB &bb, Insn &insn, uint32 index)
{
    for (auto reg : static_cast<ListOperand &>(insn.GetOperand(index)).GetOperands()) {
        regUse[bb.GetId()]->SetBit(static_cast<RegOperand *>(reg)->GetRegisterNumber());
    }
}

/* all caller saved register are modified by call insn */
void AArch64ReachingDefinition::GenAllCallerSavedRegs(BB &bb, Insn &insn)
{
    if (CGOptions::DoIPARA()) {
        std::set<regno_t> callerSaveRegs;
        cgFunc->GetRealCallerSaveRegs(insn, callerSaveRegs);
        for (auto i : callerSaveRegs) {
            regGen[bb.GetId()]->SetBit(i);
        }
    } else {
        for (uint32 i = R0; i <= V31; ++i) {
            if (AArch64Abi::IsCallerSaveReg(static_cast<AArch64reg>(i))) {
                regGen[bb.GetId()]->SetBit(i);
            }
        }
    }
}

/* reg killed killed by call insn */
bool AArch64ReachingDefinition::IsRegKilledByCallInsn(const Insn &insn, regno_t regNO) const
{
    if (CGOptions::DoIPARA()) {
        std::set<regno_t> callerSaveRegs;
        cgFunc->GetRealCallerSaveRegs(insn, callerSaveRegs);
        return callerSaveRegs.find(regNO) != callerSaveRegs.end();
    } else {
        return AArch64Abi::IsCallerSaveReg(static_cast<AArch64reg>(regNO));
    }
}

bool AArch64ReachingDefinition::KilledByCallBetweenInsnInSameBB(const Insn &startInsn, const Insn &endInsn,
                                                                regno_t regNO) const
{
    DEBUG_ASSERT(startInsn.GetBB() == endInsn.GetBB(), "two insns must be in same bb");
    if (CGOptions::DoIPARA()) {
        for (const Insn *insn = &startInsn; insn != endInsn.GetNext(); insn = insn->GetNext()) {
            if (insn->IsMachineInstruction() && insn->IsCall() && IsRegKilledByCallInsn(*insn, regNO)) {
                return true;
            }
        }
        return false;
    } else {
        return HasCallBetweenInsnInSameBB(startInsn, endInsn);
    }
}
/*
 * find definition for register between startInsn and endInsn.
 * startInsn and endInsn is not in same BB
 * make sure that in path between startBB and endBB there is no redefine.
 */
std::vector<Insn *> AArch64ReachingDefinition::FindRegDefBetweenInsnGlobal(uint32 regNO, Insn *startInsn,
                                                                           Insn *endInsn) const
{
    DEBUG_ASSERT(startInsn->GetBB() != endInsn->GetBB(), "call FindRegDefBetweenInsn please");
    std::vector<Insn *> defInsnVec;
    if (startInsn == nullptr || endInsn == nullptr) {
        return defInsnVec;
    }
    /* check startBB */
    BB *startBB = startInsn->GetBB();
    std::vector<Insn *> startBBdefInsnVec = FindRegDefBetweenInsn(regNO, startInsn->GetNext(), startBB->GetLastInsn());
    if (startBBdefInsnVec.size() == 1) {
        defInsnVec.emplace_back(*startBBdefInsnVec.begin());
    }
    if (startBBdefInsnVec.size() > 1 || (startBBdefInsnVec.empty() && regOut[startBB->GetId()]->TestBit(regNO))) {
        defInsnVec.emplace_back(startInsn);
        defInsnVec.emplace_back(endInsn);
        return defInsnVec;
    }
    if (IsCallerSavedReg(regNO) && startInsn->GetNext() != nullptr &&
        KilledByCallBetweenInsnInSameBB(*startInsn->GetNext(), *startBB->GetLastInsn(), regNO)) {
        defInsnVec.emplace_back(startInsn);
        defInsnVec.emplace_back(endInsn);
        return defInsnVec;
    }
    /* check endBB */
    BB *endBB = endInsn->GetBB();
    std::vector<Insn *> endBBdefInsnVec = FindRegDefBetweenInsn(regNO, endBB->GetFirstInsn(), endInsn->GetPrev());
    if (endBBdefInsnVec.size() == 1) {
        defInsnVec.emplace_back(*endBBdefInsnVec.begin());
    }
    if (endBBdefInsnVec.size() > 1 || (endBBdefInsnVec.empty() && regIn[endBB->GetId()]->TestBit(regNO))) {
        defInsnVec.emplace_back(startInsn);
        defInsnVec.emplace_back(endInsn);
        return defInsnVec;
    }
    if (IsCallerSavedReg(regNO) && endInsn->GetPrev() != nullptr &&
        KilledByCallBetweenInsnInSameBB(*endBB->GetFirstInsn(), *endInsn->GetPrev(), regNO)) {
        defInsnVec.emplace_back(startInsn);
        defInsnVec.emplace_back(endInsn);
        return defInsnVec;
    }
    InsnSet defInsnSet;
    std::vector<VisitStatus> visitedBB(kMaxBBNum, kNotVisited);
    visitedBB[endBB->GetId()] = kNormalVisited;
    visitedBB[startBB->GetId()] = kNormalVisited;
    std::list<bool> pathStatus;
    if (DFSFindRegInfoBetweenBB(*startBB, *endBB, regNO, visitedBB, pathStatus, kDumpRegIn)) {
        defInsnVec.emplace_back(endInsn);
    }
    return defInsnVec;
}

static bool IsRegInAsmList(Insn *insn, uint32 index, uint32 regNO, InsnSet &insnSet)
{
    for (auto reg : static_cast<ListOperand &>(insn->GetOperand(index)).GetOperands()) {
        if (static_cast<RegOperand *>(reg)->GetRegisterNumber() == regNO) {
            insnSet.insert(insn);
            return true;
        }
    }
    return false;
}

void AArch64ReachingDefinition::FindRegDefInBB(uint32 regNO, BB &bb, InsnSet &defInsnSet) const
{
    if (!regGen[bb.GetId()]->TestBit(regNO)) {
        return;
    }

    FOR_BB_INSNS(insn, (&bb))
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        if (insn->GetMachineOpcode() == MOP_asm) {
            if (IsRegInAsmList(insn, kAsmOutputListOpnd, regNO, defInsnSet)) {
                continue;
            }
            IsRegInAsmList(insn, kAsmClobberListOpnd, regNO, defInsnSet);
            continue;
        }
        if (insn->IsCall() && IsRegKilledByCallInsn(*insn, regNO)) {
            (void)defInsnSet.insert(insn);
            continue;
        }
        if (insn->IsRegDefined(regNO)) {
            (void)defInsnSet.insert(insn);
        }
    }
}

/* check whether call insn changed the stack status or not. */
bool AArch64ReachingDefinition::CallInsnClearDesignateStackRef(const Insn &callInsn, int64 offset) const
{
    return offset == callInsn.GetClearStackOffset(kFirstClearMemIndex) ||
           offset == callInsn.GetClearStackOffset(kSecondClearMemIndex);
}

/*
 * find definition for stack memory operand between startInsn and endInsn.
 * startInsn and endInsn must be in same BB and startInsn and endInsn are included
 * special case:
 *   MCC_ClearLocalStackRef clear designate stack position, the designate stack position is thought defined
 *    for example:
 *      add x0, x29, #24
 *      bl MCC_ClearLocalStackRef
 */
std::vector<Insn *> AArch64ReachingDefinition::FindMemDefBetweenInsn(uint32 offset, const Insn *startInsn,
                                                                     Insn *endInsn) const
{
    std::vector<Insn *> defInsnVec;
    if (startInsn == nullptr || endInsn == nullptr) {
        return defInsnVec;
    }

    DEBUG_ASSERT(startInsn->GetBB() == endInsn->GetBB(), "two insns must be in a same BB");
    DEBUG_ASSERT(endInsn->GetId() >= startInsn->GetId(), "two insns must be in a same BB");
    if (!memGen[startInsn->GetBB()->GetId()]->TestBit(offset / kMemZoomSize)) {
        return defInsnVec;
    }

    for (Insn *insn = endInsn; insn != nullptr && insn != startInsn->GetPrev(); insn = insn->GetPrev()) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        if (insn->GetMachineOpcode() == MOP_asm) {
            if (insn->IsAsmModMem()) {
                defInsnVec.emplace_back(insn);
                return defInsnVec;
            }
            continue;
        }

        if (insn->IsCall()) {
            if (CallInsnClearDesignateStackRef(*insn, offset)) {
                defInsnVec.emplace_back(insn);
                return defInsnVec;
            }
            continue;
        }

        if (!(insn->IsStore() || AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode()))) {
            continue;
        }

        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);

            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();

                if (base == nullptr || !IsFrameReg(*base) || index != nullptr) {
                    break;
                }

                if (!insn->IsSpillInsn() && cgFunc->IsAfterRegAlloc()) {
                    break;
                }

                DEBUG_ASSERT(memOpnd.GetOffsetImmediate() != nullptr, "offset must be a immediate value");
                int64 memOffset = memOpnd.GetOffsetImmediate()->GetOffsetValue();
                if ((offset == memOffset) ||
                    (insn->IsStorePair() && offset == memOffset + GetEachMemSizeOfPair(insn->GetMachineOpcode()))) {
                    defInsnVec.emplace_back(insn);
                    return defInsnVec;
                }
            }
        }
    }
    return defInsnVec;
}

void AArch64ReachingDefinition::FindMemDefInBB(uint32 offset, BB &bb, InsnSet &defInsnSet) const
{
    if (!memGen[bb.GetId()]->TestBit(offset / kMemZoomSize)) {
        return;
    }

    FOR_BB_INSNS(insn, (&bb))
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        if (insn->IsCall()) {
            if (insn->GetMachineOpcode() == MOP_asm) {
                if (insn->IsAsmModMem()) {
                    (void)defInsnSet.insert(insn);
                }
                continue;
            }
            if (CallInsnClearDesignateStackRef(*insn, offset)) {
                (void)defInsnSet.insert(insn);
            }
            continue;
        }

        if (!(insn->IsStore() || AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode()))) {
            continue;
        }

        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();

                if (base == nullptr || !IsFrameReg(*base) || index != nullptr) {
                    break;
                }

                DEBUG_ASSERT(memOpnd.GetOffsetImmediate() != nullptr, "offset must be a immediate value");
                int64 memOffset = memOpnd.GetOffsetImmediate()->GetOffsetValue();
                if (offset == memOffset) {
                    (void)defInsnSet.insert(insn);
                    break;
                }
                if (insn->IsStorePair() && offset == memOffset + GetEachMemSizeOfPair(insn->GetMachineOpcode())) {
                    (void)defInsnSet.insert(insn);
                    break;
                }
            }
        }
    }
}

/*
 * find defininition for register Iteratively.
 *  input:
 *    startBB: find definnition starting from startBB
 *    regNO: the No of register to be find
 *    visitedBB: record these visited BB
 *    defInsnSet: insn defining register is saved in this set
 */
void AArch64ReachingDefinition::DFSFindDefForRegOpnd(const BB &startBB, uint32 regNO,
                                                     std::vector<VisitStatus> &visitedBB, InsnSet &defInsnSet) const
{
    std::vector<Insn *> defInsnVec;
    for (auto predBB : startBB.GetPreds()) {
        if (visitedBB[predBB->GetId()] != kNotVisited) {
            continue;
        }
        visitedBB[predBB->GetId()] = kNormalVisited;
        if (regGen[predBB->GetId()]->TestBit(regNO) || (regNO == kRFLAG && predBB->HasCall())) {
            defInsnVec.clear();
            defInsnVec = FindRegDefBetweenInsn(regNO, predBB->GetFirstInsn(), predBB->GetLastInsn());
            defInsnSet.insert(defInsnVec.begin(), defInsnVec.end());
        } else if (regIn[predBB->GetId()]->TestBit(regNO)) {
            DFSFindDefForRegOpnd(*predBB, regNO, visitedBB, defInsnSet);
        }
    }

    for (auto predEhBB : startBB.GetEhPreds()) {
        if (visitedBB[predEhBB->GetId()] == kEHVisited) {
            continue;
        }
        visitedBB[predEhBB->GetId()] = kEHVisited;
        if (regGen[predEhBB->GetId()]->TestBit(regNO) || (regNO == kRFLAG && predEhBB->HasCall())) {
            FindRegDefInBB(regNO, *predEhBB, defInsnSet);
        }

        if (regIn[predEhBB->GetId()]->TestBit(regNO)) {
            DFSFindDefForRegOpnd(*predEhBB, regNO, visitedBB, defInsnSet);
        }
    }
}

/*
 * find defininition for stack memory iteratively.
 *  input:
 *    startBB: find definnition starting from startBB
 *    offset: the offset of memory to be find
 *    visitedBB: record these visited BB
 *    defInsnSet: insn defining register is saved in this set
 */
void AArch64ReachingDefinition::DFSFindDefForMemOpnd(const BB &startBB, uint32 offset,
                                                     std::vector<VisitStatus> &visitedBB, InsnSet &defInsnSet) const
{
    std::vector<Insn *> defInsnVec;
    for (auto predBB : startBB.GetPreds()) {
        if (visitedBB[predBB->GetId()] != kNotVisited) {
            continue;
        }
        visitedBB[predBB->GetId()] = kNormalVisited;
        if (memGen[predBB->GetId()]->TestBit(offset / kMemZoomSize)) {
            defInsnVec.clear();
            defInsnVec = FindMemDefBetweenInsn(offset, predBB->GetFirstInsn(), predBB->GetLastInsn());
            DEBUG_ASSERT(!defInsnVec.empty(), "opnd must be defined in this bb");
            defInsnSet.insert(defInsnVec.begin(), defInsnVec.end());
        } else if (memIn[predBB->GetId()]->TestBit(offset / kMemZoomSize)) {
            DFSFindDefForMemOpnd(*predBB, offset, visitedBB, defInsnSet);
        }
    }

    for (auto predEhBB : startBB.GetEhPreds()) {
        if (visitedBB[predEhBB->GetId()] == kEHVisited) {
            continue;
        }
        visitedBB[predEhBB->GetId()] = kEHVisited;
        if (memGen[predEhBB->GetId()]->TestBit(offset / kMemZoomSize)) {
            FindMemDefInBB(offset, *predEhBB, defInsnSet);
        }

        if (memIn[predEhBB->GetId()]->TestBit(offset / kMemZoomSize)) {
            DFSFindDefForMemOpnd(*predEhBB, offset, visitedBB, defInsnSet);
        }
    }
}

/*
 * find defininition for register.
 *  input:
 *    insn: the insn in which register is used
 *    indexOrRegNO: the index of register in insn or the No of register to be find
 *    isRegNO: if indexOrRegNO is index, this argument is false, else is true
 *  return:
 *    the set of definition insns for register
 */
InsnSet AArch64ReachingDefinition::FindDefForRegOpnd(Insn &insn, uint32 indexOrRegNO, bool isRegNO) const
{
    uint32 regNO = indexOrRegNO;
    if (!isRegNO) {
        Operand &opnd = insn.GetOperand(indexOrRegNO);
        auto &regOpnd = static_cast<RegOperand &>(opnd);
        regNO = regOpnd.GetRegisterNumber();
    }

    std::vector<Insn *> defInsnVec;
    if (regGen[insn.GetBB()->GetId()]->TestBit(regNO)) {
        defInsnVec = FindRegDefBetweenInsn(regNO, insn.GetBB()->GetFirstInsn(), insn.GetPrev());
    }
    InsnSet defInsnSet;
    if (!defInsnVec.empty()) {
        defInsnSet.insert(defInsnVec.begin(), defInsnVec.end());
        return defInsnSet;
    }
    std::vector<VisitStatus> visitedBB(kMaxBBNum, kNotVisited);
    if (insn.GetBB()->IsCleanup()) {
        DFSFindDefForRegOpnd(*insn.GetBB(), regNO, visitedBB, defInsnSet);
        if (defInsnSet.empty()) {
            FOR_ALL_BB(bb, cgFunc)
            {
                if (bb->IsCleanup()) {
                    continue;
                }
                if (regGen[bb->GetId()]->TestBit(regNO)) {
                    FindRegDefInBB(regNO, *bb, defInsnSet);
                }
            }
        }
    } else {
        DFSFindDefForRegOpnd(*insn.GetBB(), regNO, visitedBB, defInsnSet);
    }
    return defInsnSet;
}

bool AArch64ReachingDefinition::FindRegUseBetweenInsnGlobal(uint32 regNO, Insn *startInsn, Insn *endInsn,
                                                            BB *movBB) const
{
    if (startInsn == nullptr || endInsn == nullptr) {
        return false;
    }
    if (startInsn->GetBB() == endInsn->GetBB()) {
        if (startInsn->GetNextMachineInsn() == endInsn) {
            return false;
        } else {
            return FindRegUsingBetweenInsn(regNO, startInsn->GetNextMachineInsn(), endInsn->GetPreviousMachineInsn());
        }
    } else {
        /* check Start BB */
        BB *startBB = startInsn->GetBB();
        if (FindRegUsingBetweenInsn(regNO, startInsn->GetNextMachineInsn(), startBB->GetLastInsn())) {
            return true;
        }
        /* check End BB */
        BB *endBB = endInsn->GetBB();
        if (FindRegUsingBetweenInsn(regNO, endBB->GetFirstInsn(), endInsn->GetPreviousMachineInsn())) {
            return true;
        }
        /* Global : startBB cannot dominate BB which it doesn't dominate before */
        if (startBB == movBB) {
            return false; /* it will not change dominate */
        }
        std::vector<VisitStatus> visitedBB(kMaxBBNum, kNotVisited);
        visitedBB[movBB->GetId()] = kNormalVisited;
        visitedBB[startBB->GetId()] = kNormalVisited;
        if (DFSFindRegDomianBetweenBB(*startBB, regNO, visitedBB)) {
            return true;
        }
    }
    return false;
}

bool AArch64ReachingDefinition::HasRegDefBetweenInsnGlobal(uint32 regNO, Insn &startInsn, Insn &endInsn) const
{
    CHECK_FATAL((startInsn.GetBB() != endInsn.GetBB()), "Is same BB!");
    /* check Start BB */
    BB *startBB = startInsn.GetBB();
    auto startInsnSet = FindRegDefBetweenInsn(regNO, startInsn.GetNext(), startBB->GetLastInsn());
    if (!startInsnSet.empty()) {
        return true;
    }
    /* check End BB */
    BB *endBB = endInsn.GetBB();
    auto endInsnSet = FindRegDefBetweenInsn(regNO, endBB->GetFirstInsn(), endInsn.GetPrev());
    if (!endInsnSet.empty()) {
        return true;
    }
    if (!startBB->GetSuccs().empty()) {
        for (auto *succ : startBB->GetSuccs()) {
            if (succ == endBB) {
                return (!startInsnSet.empty() && !endInsnSet.empty());
            }
        }
    }
    /* check bb Between start and end */
    std::vector<VisitStatus> visitedBB(kMaxBBNum, kNotVisited);
    visitedBB[startBB->GetId()] = kNormalVisited;
    visitedBB[endBB->GetId()] = kNormalVisited;
    return DFSFindRegDefBetweenBB(*startBB, *endBB, regNO, visitedBB);
}

bool AArch64ReachingDefinition::DFSFindRegDefBetweenBB(const BB &startBB, const BB &endBB, uint32 regNO,
                                                       std::vector<VisitStatus> &visitedBB) const
{
    if (&startBB == &endBB) {
        return false;
    }
    for (auto succBB : startBB.GetSuccs()) {
        if (visitedBB[succBB->GetId()] != kNotVisited) {
            continue;
        }
        visitedBB[succBB->GetId()] = kNormalVisited;
        if (regGen[succBB->GetId()]->TestBit(regNO)) {
            return true;
        }
        if (DFSFindRegDefBetweenBB(*succBB, endBB, regNO, visitedBB)) {
            return true;
        }
    }
    return false;
}

bool AArch64ReachingDefinition::DFSFindRegDomianBetweenBB(const BB startBB, uint32 regNO,
                                                          std::vector<VisitStatus> &visitedBB) const
{
    for (auto succBB : startBB.GetSuccs()) {
        if (visitedBB[succBB->GetId()] != kNotVisited) {
            continue;
        }
        visitedBB[succBB->GetId()] = kNormalVisited;
        if (regIn[succBB->GetId()]->TestBit(regNO)) {
            return true;
        } else if (regGen[succBB->GetId()]->TestBit(regNO)) {
            continue;
        }
        if (DFSFindRegDomianBetweenBB(*succBB, regNO, visitedBB)) {
            return true;
        }
    }
    CHECK_FATAL(startBB.GetEhSuccs().empty(), "C Module have no eh");
    return false;
}

bool AArch64ReachingDefinition::DFSFindRegInfoBetweenBB(const BB startBB, const BB &endBB, uint32 regNO,
                                                        std::vector<VisitStatus> &visitedBB,
                                                        std::list<bool> &pathStatus, DumpType infoType) const
{
    for (auto succBB : startBB.GetSuccs()) {
        if (succBB == &endBB) {
            for (auto status : pathStatus) {
                if (!status) {
                    return true;
                }
            }
            continue;
        }
        if (visitedBB[succBB->GetId()] != kNotVisited) {
            continue;
        }
        visitedBB[succBB->GetId()] = kNormalVisited;
        /* path is no clean check regInfo */
        bool isPathClean = true;
        switch (infoType) {
            case kDumpRegUse: {
                isPathClean = !regUse[succBB->GetId()]->TestBit(regNO);
                break;
            }
            case kDumpRegGen: {
                isPathClean = !regGen[succBB->GetId()]->TestBit(regNO);
                break;
            }
            case kDumpRegIn: {
                isPathClean = !(regIn[succBB->GetId()]->TestBit(regNO) || regGen[succBB->GetId()]->TestBit(regNO));
                break;
            }
            default:
                CHECK_FATAL(false, "NIY");
        }
        pathStatus.emplace_back(isPathClean);
        if (DFSFindRegInfoBetweenBB(*succBB, endBB, regNO, visitedBB, pathStatus, infoType)) {
            return true;
        }
        pathStatus.pop_back();
    }
    CHECK_FATAL(startBB.GetEhSuccs().empty(), "C Module have no eh");
    return false;
}

bool AArch64ReachingDefinition::FindRegUsingBetweenInsn(uint32 regNO, Insn *startInsn, const Insn *endInsn) const
{
    if (startInsn == nullptr || endInsn == nullptr) {
        return false;
    }

    DEBUG_ASSERT(startInsn->GetBB() == endInsn->GetBB(), "two insns must be in a same BB");
    for (Insn *insn = startInsn; insn != nullptr && insn != endInsn->GetNext(); insn = insn->GetNext()) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (insn->GetMachineOpcode() == MOP_asm) {
            InsnSet Temp;
            if (IsRegInAsmList(insn, kAsmInputListOpnd, regNO, Temp) ||
                IsRegInAsmList(insn, kAsmOutputListOpnd, regNO, Temp)) {
                return true;
            }
            continue;
        }
        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            if (opnd.IsList()) {
                auto &listOpnd = static_cast<ListOperand &>(opnd);
                for (auto listElem : listOpnd.GetOperands()) {
                    RegOperand *regOpnd = static_cast<RegOperand *>(listElem);
                    DEBUG_ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
                    if (regNO == regOpnd->GetRegisterNumber()) {
                        return true;
                    }
                }
                continue;
            }

            auto *regProp = md->opndMD[i];
            if (!regProp->IsUse() && !opnd.IsMemoryAccessOperand()) {
                continue;
            }

            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();
                if ((base != nullptr && base->GetRegisterNumber() == regNO) ||
                    (index != nullptr && index->GetRegisterNumber() == regNO)) {
                    return true;
                }
            } else if (opnd.IsConditionCode()) {
                Operand &rflagOpnd = cgFunc->GetOrCreateRflag();
                RegOperand &rflagReg = static_cast<RegOperand &>(rflagOpnd);
                if (rflagReg.GetRegisterNumber() == regNO) {
                    return true;
                }
            } else if (opnd.IsRegister() && (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO)) {
                return true;
            }
        }
    }
    return false;
}

/*
 * find insn using register between startInsn and endInsn.
 * startInsn and endInsn must be in same BB and startInsn and endInsn are included
 */
bool AArch64ReachingDefinition::FindRegUseBetweenInsn(uint32 regNO, Insn *startInsn, Insn *endInsn,
                                                      InsnSet &regUseInsnSet) const
{
    bool findFinish = false;
    if (startInsn == nullptr || endInsn == nullptr) {
        return findFinish;
    }

    DEBUG_ASSERT(startInsn->GetBB() == endInsn->GetBB(), "two insns must be in a same BB");
    for (Insn *insn = startInsn; insn != nullptr && insn != endInsn->GetNext(); insn = insn->GetNext()) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (insn->GetMachineOpcode() == MOP_asm) {
            IsRegInAsmList(insn, kAsmInputListOpnd, regNO, regUseInsnSet);
            if (IsRegInAsmList(insn, kAsmOutputListOpnd, regNO, regUseInsnSet)) {
                break;
            }
            continue;
        }
        /* if insn is call and regNO is caller-saved register, then regNO will not be used later */
        if (insn->IsCall() && IsRegKilledByCallInsn(*insn, regNO)) {
            findFinish = true;
        }

        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            if (opnd.IsList()) {
                auto &listOpnd = static_cast<ListOperand &>(opnd);
                for (auto listElem : listOpnd.GetOperands()) {
                    RegOperand *regOpnd = static_cast<RegOperand *>(listElem);
                    DEBUG_ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
                    if (regNO == regOpnd->GetRegisterNumber()) {
                        (void)regUseInsnSet.insert(insn);
                    }
                }
                continue;
            } else if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *baseOpnd = memOpnd.GetBaseRegister();
                if (baseOpnd != nullptr && (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi) &&
                    (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed()) && baseOpnd->GetRegisterNumber() == regNO) {
                    findFinish = true;
                }
            }

            auto *regProp = md->opndMD[i];
            if (regProp->IsDef() && opnd.IsRegister() &&
                (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO)) {
                findFinish = true;
            }

            if (!regProp->IsUse() && !opnd.IsMemoryAccessOperand()) {
                continue;
            }

            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();
                if ((base != nullptr && base->GetRegisterNumber() == regNO) ||
                    (index != nullptr && index->GetRegisterNumber() == regNO)) {
                    (void)regUseInsnSet.insert(insn);
                }
            } else if (opnd.IsConditionCode()) {
                Operand &rflagOpnd = cgFunc->GetOrCreateRflag();
                RegOperand &rflagReg = static_cast<RegOperand &>(rflagOpnd);
                if (rflagReg.GetRegisterNumber() == regNO) {
                    (void)regUseInsnSet.insert(insn);
                }
            } else if (opnd.IsRegister() && (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO)) {
                (void)regUseInsnSet.insert(insn);
            }
        }

        if (findFinish) {
            break;
        }
    }
    return findFinish;
}

/*
 * find insn using stack memory operand between startInsn and endInsn.
 * startInsn and endInsn must be in same BB and startInsn and endInsn are included
 */
bool AArch64ReachingDefinition::FindMemUseBetweenInsn(uint32 offset, Insn *startInsn, const Insn *endInsn,
                                                      InsnSet &memUseInsnSet) const
{
    bool findFinish = false;
    if (startInsn == nullptr || endInsn == nullptr) {
        return findFinish;
    }

    DEBUG_ASSERT(startInsn->GetBB() == endInsn->GetBB(), "two insns must be in a same BB");
    DEBUG_ASSERT(endInsn->GetId() >= startInsn->GetId(), "end ID must be greater than or equal to start ID");

    for (Insn *insn = startInsn; insn != nullptr && insn != endInsn->GetNext(); insn = insn->GetNext()) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        if (insn->IsCall()) {
            if (insn->GetMachineOpcode() == MOP_asm) {
                return true;
            }
            if (CallInsnClearDesignateStackRef(*insn, offset)) {
                return true;
            }
            continue;
        }

        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            if (!opnd.IsMemoryAccessOperand()) {
                continue;
            }

            auto &memOpnd = static_cast<MemOperand &>(opnd);
            RegOperand *base = memOpnd.GetBaseRegister();
            if (base == nullptr || !IsFrameReg(*base)) {
                continue;
            }

            DEBUG_ASSERT(memOpnd.GetIndexRegister() == nullptr, "offset must not be Register for frame MemOperand");
            DEBUG_ASSERT(memOpnd.GetOffsetImmediate() != nullptr, "offset must be a immediate value");
            int64 memOffset = memOpnd.GetOffsetImmediate()->GetValue();

            if (insn->IsStore() || AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode())) {
                if (memOffset == offset) {
                    findFinish = true;
                    continue;
                }
                if (insn->IsStorePair() && offset == memOffset + GetEachMemSizeOfPair(insn->GetMachineOpcode())) {
                    findFinish = true;
                    continue;
                }
            }

            if (!md->opndMD[i]->IsUse()) {
                continue;
            }

            if (offset == memOffset) {
                (void)memUseInsnSet.insert(insn);
            } else if (insn->IsLoadPair() && offset == memOffset + GetEachMemSizeOfPair(insn->GetMachineOpcode())) {
                (void)memUseInsnSet.insert(insn);
            }
        }

        if (findFinish) {
            break;
        }
    }
    return findFinish;
}

/* find all definition for stack memory operand insn.opnd[index] */
InsnSet AArch64ReachingDefinition::FindDefForMemOpnd(Insn &insn, uint32 indexOrOffset, bool isOffset) const
{
    InsnSet defInsnSet;
    int64 memOffSet = 0;
    if (!isOffset) {
        Operand &opnd = insn.GetOperand(indexOrOffset);
        DEBUG_ASSERT(opnd.IsMemoryAccessOperand(), "opnd must be MemOperand");

        auto &memOpnd = static_cast<MemOperand &>(opnd);
        RegOperand *base = memOpnd.GetBaseRegister();
        RegOperand *indexReg = memOpnd.GetIndexRegister();

        if (base == nullptr || !IsFrameReg(*base) || indexReg) {
            return defInsnSet;
        }
        DEBUG_ASSERT(memOpnd.GetOffsetImmediate() != nullptr, "offset must be a immediate value");
        memOffSet = memOpnd.GetOffsetImmediate()->GetOffsetValue();
    } else {
        memOffSet = indexOrOffset;
    }
    std::vector<Insn *> defInsnVec;
    if (memGen[insn.GetBB()->GetId()]->TestBit(static_cast<uint32>(memOffSet / kMemZoomSize))) {
        defInsnVec = FindMemDefBetweenInsn(memOffSet, insn.GetBB()->GetFirstInsn(), insn.GetPrev());
    }

    if (!defInsnVec.empty()) {
        defInsnSet.insert(defInsnVec.begin(), defInsnVec.end());
        return defInsnSet;
    }
    std::vector<VisitStatus> visitedBB(kMaxBBNum, kNotVisited);
    if (insn.GetBB()->IsCleanup()) {
        DFSFindDefForMemOpnd(*insn.GetBB(), memOffSet, visitedBB, defInsnSet);
        if (defInsnSet.empty()) {
            FOR_ALL_BB(bb, cgFunc)
            {
                if (bb->IsCleanup()) {
                    continue;
                }

                if (memGen[bb->GetId()]->TestBit(static_cast<uint32>(memOffSet / kMemZoomSize))) {
                    FindMemDefInBB(memOffSet, *bb, defInsnSet);
                }
            }
        }
    } else {
        DFSFindDefForMemOpnd(*insn.GetBB(), memOffSet, visitedBB, defInsnSet);
    }

    return defInsnSet;
}

/*
 * find all insn using stack memory operand insn.opnd[index]
 * secondMem is used to represent the second stack memory opernad in store pair insn
 */
InsnSet AArch64ReachingDefinition::FindUseForMemOpnd(Insn &insn, uint8 index, bool secondMem) const
{
    Operand &opnd = insn.GetOperand(index);
    DEBUG_ASSERT(opnd.IsMemoryAccessOperand(), "opnd must be MemOperand");
    auto &memOpnd = static_cast<MemOperand &>(opnd);
    RegOperand *base = memOpnd.GetBaseRegister();

    InsnSet useInsnSet;
    if (base == nullptr || !IsFrameReg(*base)) {
        return useInsnSet;
    }

    DEBUG_ASSERT(memOpnd.GetIndexRegister() == nullptr, "IndexRegister no nullptr");
    DEBUG_ASSERT(memOpnd.GetOffsetImmediate() != nullptr, "offset must be a immediate value");
    int64 memOffSet = memOpnd.GetOffsetImmediate()->GetOffsetValue();
    if (secondMem) {
        DEBUG_ASSERT(insn.IsStorePair(), "second MemOperand can only be defined in stp insn");
        memOffSet += GetEachMemSizeOfPair(insn.GetMachineOpcode());
    }
    /* memOperand may be redefined in current BB */
    bool findFinish = FindMemUseBetweenInsn(memOffSet, insn.GetNext(), insn.GetBB()->GetLastInsn(), useInsnSet);
    std::vector<bool> visitedBB(kMaxBBNum, false);
    if (findFinish || !memOut[insn.GetBB()->GetId()]->TestBit(static_cast<uint32>(memOffSet / kMemZoomSize))) {
        if (insn.GetBB()->GetEhSuccs().size() != 0) {
            DFSFindUseForMemOpnd(*insn.GetBB(), memOffSet, visitedBB, useInsnSet, true);
        }
    } else {
        DFSFindUseForMemOpnd(*insn.GetBB(), memOffSet, visitedBB, useInsnSet, false);
    }
    if (!insn.GetBB()->IsCleanup() && firstCleanUpBB) {
        if (memUse[firstCleanUpBB->GetId()]->TestBit(static_cast<uint32>(memOffSet / kMemZoomSize))) {
            findFinish = FindMemUseBetweenInsn(memOffSet, firstCleanUpBB->GetFirstInsn(), firstCleanUpBB->GetLastInsn(),
                                               useInsnSet);
            if (findFinish ||
                !memOut[firstCleanUpBB->GetId()]->TestBit(static_cast<uint32>(memOffSet / kMemZoomSize))) {
                return useInsnSet;
            }
        }
        DFSFindUseForMemOpnd(*firstCleanUpBB, memOffSet, visitedBB, useInsnSet, false);
    }
    return useInsnSet;
}

/*
 * initialize bb.gen and bb.use
 * if it is not computed in first time, bb.gen and bb.use must be cleared firstly
 */
void AArch64ReachingDefinition::InitGenUse(BB &bb, bool firstTime)
{
    if (!firstTime && (mode & kRDRegAnalysis)) {
        regGen[bb.GetId()]->ResetAllBit();
        regUse[bb.GetId()]->ResetAllBit();
    }
    if (!firstTime && (mode & kRDMemAnalysis)) {
        memGen[bb.GetId()]->ResetAllBit();
        memUse[bb.GetId()]->ResetAllBit();
    }

    if (bb.IsEmpty()) {
        return;
    }

    FOR_BB_INSNS(insn, (&bb))
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }

        if (insn->GetMachineOpcode() == MOP_asm) {
            GenAllAsmDefRegs(bb, *insn, kAsmOutputListOpnd);
            GenAllAsmDefRegs(bb, *insn, kAsmClobberListOpnd);
            GenAllAsmUseRegs(bb, *insn, kAsmInputListOpnd);
            continue;
        }
        if (insn->IsCall() || insn->IsTailCall()) {
            GenAllCallerSavedRegs(bb, *insn);
            InitMemInfoForClearStackCall(*insn);
        }

        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            auto *regProp = md->opndMD[i];
            if (opnd.IsList() && (mode & kRDRegAnalysis)) {
                DEBUG_ASSERT(regProp->IsUse(), "ListOperand is used in insn");
                InitInfoForListOpnd(bb, opnd);
            } else if (opnd.IsMemoryAccessOperand()) {
                InitInfoForMemOperand(*insn, opnd, regProp->IsDef());
            } else if (opnd.IsConditionCode() && (mode & kRDRegAnalysis)) {
                DEBUG_ASSERT(regProp->IsUse(), "condition code is used in insn");
                InitInfoForConditionCode(bb);
            } else if (opnd.IsRegister() && (mode & kRDRegAnalysis)) {
                InitInfoForRegOpnd(bb, opnd, regProp->IsDef());
            }
        }
    }
}

void AArch64ReachingDefinition::InitMemInfoForClearStackCall(Insn &callInsn)
{
    if (!(mode & kRDMemAnalysis) || !callInsn.IsClearDesignateStackCall()) {
        return;
    }
    int64 firstOffset = callInsn.GetClearStackOffset(kFirstClearMemIndex);
    constexpr int64 defaultValOfClearMemOffset = -1;
    if (firstOffset != defaultValOfClearMemOffset) {
        memGen[callInsn.GetBB()->GetId()]->SetBit(firstOffset / kMemZoomSize);
    }
    int64 secondOffset = callInsn.GetClearStackOffset(kSecondClearMemIndex);
    if (secondOffset != defaultValOfClearMemOffset) {
        memGen[callInsn.GetBB()->GetId()]->SetBit(static_cast<uint32>(secondOffset / kMemZoomSize));
    }
}

void AArch64ReachingDefinition::InitInfoForMemOperand(Insn &insn, Operand &opnd, bool isDef)
{
    DEBUG_ASSERT(opnd.IsMemoryAccessOperand(), "opnd must be MemOperand");
    MemOperand &memOpnd = static_cast<MemOperand &>(opnd);
    RegOperand *base = memOpnd.GetBaseRegister();
    RegOperand *index = memOpnd.GetIndexRegister();

    if (base == nullptr) {
        return;
    }
    if ((mode & kRDMemAnalysis) && IsFrameReg(*base)) {
        if (index != nullptr) {
            SetAnalysisMode(kRDRegAnalysis);
            return;
        }
        CHECK_FATAL(index == nullptr, "Existing [x29 + index] Memory Address");
        DEBUG_ASSERT(memOpnd.GetOffsetImmediate(), "offset must be a immediate value");
        int64 offsetVal = memOpnd.GetOffsetImmediate()->GetOffsetValue();
        if ((offsetVal % kMemZoomSize) != 0) {
            SetAnalysisMode(kRDRegAnalysis);
        }

        if (!isDef) {
            memUse[insn.GetBB()->GetId()]->SetBit(offsetVal / kMemZoomSize);
            if (insn.IsLoadPair()) {
                int64 nextMemOffset = offsetVal + GetEachMemSizeOfPair(insn.GetMachineOpcode());
                memUse[insn.GetBB()->GetId()]->SetBit(nextMemOffset / kMemZoomSize);
            }
        } else if (isDef) {
            memGen[insn.GetBB()->GetId()]->SetBit(offsetVal / kMemZoomSize);
            if (insn.IsStorePair()) {
                int64 nextMemOffset = offsetVal + GetEachMemSizeOfPair(insn.GetMachineOpcode());
                memGen[insn.GetBB()->GetId()]->SetBit(nextMemOffset / kMemZoomSize);
            }
        }
    }

    if ((mode & kRDRegAnalysis) != 0) {
        regUse[insn.GetBB()->GetId()]->SetBit(base->GetRegisterNumber());
        if (index != nullptr) {
            regUse[insn.GetBB()->GetId()]->SetBit(index->GetRegisterNumber());
        }
        if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi && (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
            /* Base operand has changed. */
            regGen[insn.GetBB()->GetId()]->SetBit(base->GetRegisterNumber());
        }
    }
}

void AArch64ReachingDefinition::InitInfoForListOpnd(const BB &bb, Operand &opnd)
{
    ListOperand *listOpnd = static_cast<ListOperand *>(&opnd);
    for (auto listElem : listOpnd->GetOperands()) {
        RegOperand *regOpnd = static_cast<RegOperand *>(listElem);
        DEBUG_ASSERT(regOpnd != nullptr, "used Operand in call insn must be Register");
        regUse[bb.GetId()]->SetBit(regOpnd->GetRegisterNumber());
    }
}

void AArch64ReachingDefinition::InitInfoForConditionCode(const BB &bb)
{
    Operand &rflagOpnd = cgFunc->GetOrCreateRflag();
    RegOperand &rflagReg = static_cast<RegOperand &>(rflagOpnd);
    regUse[bb.GetId()]->SetBit(rflagReg.GetRegisterNumber());
}

void AArch64ReachingDefinition::InitInfoForRegOpnd(const BB &bb, Operand &opnd, bool isDef)
{
    RegOperand *regOpnd = static_cast<RegOperand *>(&opnd);
    if (!isDef) {
        regUse[bb.GetId()]->SetBit(regOpnd->GetRegisterNumber());
    } else {
        regGen[bb.GetId()]->SetBit(regOpnd->GetRegisterNumber());
    }
}

int32 AArch64ReachingDefinition::GetStackSize() const
{
    const int sizeofFplr = kDivide2 * kAarch64IntregBytelen;
    return static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc->GetMemlayout())->RealStackFrameSize() +
                              sizeofFplr);
}

bool AArch64ReachingDefinition::IsCallerSavedReg(uint32 regNO) const
{
    return AArch64Abi::IsCallerSaveReg(static_cast<AArch64reg>(regNO));
}

int64 AArch64ReachingDefinition::GetEachMemSizeOfPair(MOperator opCode) const
{
    switch (opCode) {
        case MOP_wstp:
        case MOP_sstp:
        case MOP_wstlxp:
        case MOP_wldp:
        case MOP_xldpsw:
        case MOP_sldp:
        case MOP_wldaxp:
            return kWordByteNum;
        case MOP_xstp:
        case MOP_dstp:
        case MOP_xstlxp:
        case MOP_xldp:
        case MOP_dldp:
        case MOP_xldaxp:
            return kDoubleWordByteNum;
        default:
            return 0;
    }
}
} /* namespace maplebe */
