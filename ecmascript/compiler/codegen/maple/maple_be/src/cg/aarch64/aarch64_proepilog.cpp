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

#include "aarch64_proepilog.h"
#include "aarch64_cg.h"
#include "cg_option.h"
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

namespace {
constexpr int32 kSoeChckOffset = 8192;

enum RegsPushPop : uint8 { kRegsPushOp, kRegsPopOp };

enum PushPopType : uint8 { kPushPopSingle = 0, kPushPopPair = 1 };

MOperator pushPopOps[kRegsPopOp + 1][kRegTyFloat + 1][kPushPopPair + 1] = {{
                                                                               /* push */
                                                                               {0}, /* undef */
                                                                               {
                                                                                   /* kRegTyInt */
                                                                                   MOP_xstr, /* single */
                                                                                   MOP_xstp, /* pair   */
                                                                               },
                                                                               {
                                                                                   /* kRegTyFloat */
                                                                                   MOP_dstr, /* single */
                                                                                   MOP_dstp, /* pair   */
                                                                               },
                                                                           },
                                                                           {
                                                                               /* pop */
                                                                               {0}, /* undef */
                                                                               {
                                                                                   /* kRegTyInt */
                                                                                   MOP_xldr, /* single */
                                                                                   MOP_xldp, /* pair   */
                                                                               },
                                                                               {
                                                                                   /* kRegTyFloat */
                                                                                   MOP_dldr, /* single */
                                                                                   MOP_dldp, /* pair   */
                                                                               },
                                                                           }};

inline void AppendInstructionTo(Insn &insn, CGFunc &func)
{
    func.GetCurBB()->AppendInsn(insn);
}
}  // namespace

bool AArch64GenProEpilog::HasLoop()
{
    FOR_ALL_BB(bb, &cgFunc) {
        if (bb->IsBackEdgeDest()) {
            return true;
        }
        FOR_BB_INSNS_REV(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (insn->HasLoop()) {
                return true;
            }
        }
    }
    return false;
}

/*
 *  Remove redundant mov and mark optimizable bl/blr insn in the BB.
 *  Return value: true to call this modified block again.
 */
bool AArch64GenProEpilog::OptimizeTailBB(BB &bb, MapleSet<Insn *> &callInsns, const BB &exitBB) const
{
    if (bb.NumInsn() == 1 &&
        (bb.GetLastInsn()->GetMachineOpcode() != MOP_xbr && bb.GetLastInsn()->GetMachineOpcode() != MOP_xblr &&
         bb.GetLastInsn()->GetMachineOpcode() != MOP_xbl && bb.GetLastInsn()->GetMachineOpcode() != MOP_xuncond)) {
        return false;
    }
    FOR_BB_INSNS_REV_SAFE(insn, &bb, prev_insn) {
        if (!insn->IsMachineInstruction() || AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode())) {
            continue;
        }
        MOperator insnMop = insn->GetMachineOpcode();
        switch (insnMop) {
            case MOP_xldr:
            case MOP_xldp:
            case MOP_dldr:
            case MOP_dldp: {
                if (bb.GetKind() == BB::kBBReturn) {
                    RegOperand &reg = static_cast<RegOperand &>(insn->GetOperand(0));
                    if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(reg.GetRegisterNumber()))) {
                        break; /* inserted restore from calleeregs-placement, ignore */
                    }
                }
                return false;
            }
            case MOP_wmovrr:
            case MOP_xmovrr: {
                CHECK_FATAL(insn->GetOperand(0).IsRegister(), "operand0 is not register");
                CHECK_FATAL(insn->GetOperand(1).IsRegister(), "operand1 is not register");
                auto &reg1 = static_cast<RegOperand &>(insn->GetOperand(0));
                auto &reg2 = static_cast<RegOperand &>(insn->GetOperand(1));

                if (reg1.GetRegisterNumber() != R0 || reg2.GetRegisterNumber() != R0) {
                    return false;
                }

                bb.RemoveInsn(*insn);
                break;
            }
            case MOP_xblr: {
                if (insn->GetOperand(0).IsRegister()) {
                    RegOperand &reg = static_cast<RegOperand &>(insn->GetOperand(0));
                    if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(reg.GetRegisterNumber()))) {
                        return false; /* can't tailcall, register will be overwritten by restore */
                    }
                }
                /* flow through */
            }
                [[clang::fallthrough]];
            case MOP_xbl: {
                callInsns.insert(insn);
                return false;
            }
            case MOP_xuncond: {
                LabelOperand &bLab = static_cast<LabelOperand &>(insn->GetOperand(0));
                if (exitBB.GetLabIdx() == bLab.GetLabelIndex()) {
                    break;
                }
                return false;
            }
            default:
                return false;
        }
    }

    return true;
}

/* Recursively invoke this function for all predecessors of exitBB */
void AArch64GenProEpilog::TailCallBBOpt(BB &bb, MapleSet<Insn *> &callInsns, BB &exitBB)
{
    /* callsite also in the return block as in "if () return; else foo();"
       call in the exit block */
    if (!bb.IsEmpty() && !OptimizeTailBB(bb, callInsns, exitBB)) {
        return;
    }

    for (auto tmpBB : bb.GetPreds()) {
        if (tmpBB->GetSuccs().size() != 1 || !tmpBB->GetEhSuccs().empty() ||
            (tmpBB->GetKind() != BB::kBBFallthru && tmpBB->GetKind() != BB::kBBGoto)) {
            continue;
        }

        if (OptimizeTailBB(*tmpBB, callInsns, exitBB)) {
            TailCallBBOpt(*tmpBB, callInsns, exitBB);
        }
    }
}

/*
 *  If a function without callee-saved register, and end with a function call,
 *  then transfer bl/blr to b/br.
 *  Return value: true if function do not need Prologue/Epilogue. false otherwise.
 */
bool AArch64GenProEpilog::TailCallOpt()
{
    /* Count how many call insns in the whole function. */
    uint32 nCount = 0;
    bool hasGetStackClass = false;

    FOR_ALL_BB(bb, &cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (insn->IsMachineInstruction() && insn->IsCall()) {
                ++nCount;
            }
        }
    }
    if ((nCount > 0 && cgFunc.GetFunction().GetAttr(FUNCATTR_interface)) || hasGetStackClass) {
        return false;
    }

    if (nCount == 0) {
        // no bl instr in any bb
        return true;
    }

    size_t exitBBSize = cgFunc.GetExitBBsVec().size();
    /* For now to reduce complexity */

    BB *exitBB = nullptr;
    if (exitBBSize == 0) {
        if (cgFunc.GetLastBB()->GetPrev()->GetFirstStmt() == cgFunc.GetCleanupLabel() &&
            cgFunc.GetLastBB()->GetPrev()->GetPrev() != nullptr) {
            exitBB = cgFunc.GetLastBB()->GetPrev()->GetPrev();
        } else {
            exitBB = cgFunc.GetLastBB()->GetPrev();
        }
    } else {
        exitBB = cgFunc.GetExitBBsVec().front();
    }
    uint32 i = 1;
    size_t optCount = 0;
    do {
        MapleSet<Insn *> callInsns(tmpAlloc.Adapter());
        TailCallBBOpt(*exitBB, callInsns, *exitBB);
        if (callInsns.size() != 0) {
            optCount += callInsns.size();
            (void)exitBB2CallSitesMap.emplace(exitBB, callInsns);
        }
        if (i < exitBBSize) {
            exitBB = cgFunc.GetExitBBsVec()[i];
            ++i;
        } else {
            break;
        }
    } while (1);

    /* regular calls exist in function */
    if (nCount != optCount) {
        return false;
    }
    return true;
}

static bool IsAddOrSubOp(MOperator mOp)
{
    switch (mOp) {
        case MOP_xaddrrr:
        case MOP_xaddrrrs:
        case MOP_xxwaddrrre:
        case MOP_xaddrri24:
        case MOP_xaddrri12:
        case MOP_xsubrrr:
        case MOP_xsubrrrs:
        case MOP_xxwsubrrre:
        case MOP_xsubrri12:
            return true;
        default:
            return false;
    }
}

/* tailcallopt cannot be used if stack address of this function is taken and passed,
   not checking the passing for now, just taken */
static bool IsStackAddrTaken(CGFunc &cgFunc)
{
    FOR_ALL_BB(bb, &cgFunc) {
        FOR_BB_INSNS_REV(insn, bb) {
            if (IsAddOrSubOp(insn->GetMachineOpcode())) {
                for (uint32 i = 0; i < insn->GetOperandSize(); i++) {
                    if (insn->GetOperand(i).IsRegister()) {
                        RegOperand &reg = static_cast<RegOperand &>(insn->GetOperand(i));
                        if (reg.GetRegisterNumber() == R29 || reg.GetRegisterNumber() == R31 ||
                            reg.GetRegisterNumber() == RSP) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool AArch64GenProEpilog::NeedProEpilog()
{
    if (cgFunc.GetMirModule().GetSrcLang() != kSrcLangC) {
        return true;
    } else if (cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) || cgFunc.HasVLAOrAlloca()) {
        return true;
    }
    bool funcHasCalls = false;
    if (cgFunc.GetCG()->DoTailCall() && !IsStackAddrTaken(cgFunc) && !stackProtect) {
        funcHasCalls = !TailCallOpt();  // return value == "no call instr/only or 1 tailcall"
    } else {
        FOR_ALL_BB(bb, &cgFunc) {
            FOR_BB_INSNS_REV(insn, bb) {
                if (insn->IsMachineInstruction() && insn->IsCall()) {
                    funcHasCalls = true;
                }
            }
        }
    }
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    const MapleVector<AArch64reg> &regsToRestore =
        (!CGOptions::DoRegSavesOpt()) ? aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
    size_t calleeSavedRegSize = kTwoRegister;
    CHECK_FATAL(regsToRestore.size() >= calleeSavedRegSize, "Forgot FP and LR ?");
    if (funcHasCalls || regsToRestore.size() > calleeSavedRegSize || aarchCGFunc.HasStackLoadStore() ||
        static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->GetSizeOfLocals() > 0 ||
        cgFunc.GetFunction().GetAttr(FUNCATTR_callersensitive)) {
        return true;
    }
    return false;
}

void AArch64GenProEpilog::GenStackGuard(BB &bb)
{
    if (!stackProtect) {
        return;
    }
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    BB *formerCurBB = cgFunc.GetCurBB();
    aarchCGFunc.GetDummyBB()->ClearInsns();
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(true);
    cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

    MIRSymbol *stkGuardSym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_guard")));
    StImmOperand &stOpnd = aarchCGFunc.CreateStImmOperand(*stkGuardSym, 0, 0);
    RegOperand &stAddrOpnd =
        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, GetPointerSize() * kBitsPerByte, kRegTyInt);
    aarchCGFunc.SelectAddrof(stAddrOpnd, stOpnd);

    MemOperand *guardMemOp =
        aarchCGFunc.CreateMemOperand(MemOperand::kAddrModeBOi, GetPointerSize() * kBitsPerByte, stAddrOpnd, nullptr,
                                     &aarchCGFunc.GetOrCreateOfstOpnd(0, k32BitSize), stkGuardSym);
    MOperator mOp = aarchCGFunc.PickLdInsn(k64BitSize, PTY_u64);
    Insn &insn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, stAddrOpnd, *guardMemOp);
    insn.SetDoNotRemove(true);
    cgFunc.GetCurBB()->AppendInsn(insn);

    uint64 vArea = 0;
    if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
        AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
        if (ml->GetSizeOfGRSaveArea() > 0) {
            vArea += RoundUp(ml->GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
        }
        if (ml->GetSizeOfVRSaveArea() > 0) {
            vArea += RoundUp(ml->GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
        }
    }

    int32 stkSize = static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    if (useFP) {
        stkSize -=
            (static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->SizeOfArgsToStackPass()) +
             cgFunc.GetFunction().GetFrameReseverdSlot());
    }
    int32 memSize = (stkSize - kOffset8MemPos) - static_cast<int32>(vArea);
    MemOperand *downStk = aarchCGFunc.CreateStackMemOpnd(stackBaseReg, memSize, GetPointerSize() * kBitsPerByte);
    if (downStk->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*downStk, k64BitSize)) {
        downStk = &aarchCGFunc.SplitOffsetWithAddInstruction(*downStk, k64BitSize, R10);
    }
    mOp = aarchCGFunc.PickStInsn(GetPointerSize() * kBitsPerByte, PTY_u64);
    Insn &tmpInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, stAddrOpnd, *downStk);
    tmpInsn.SetDoNotRemove(true);
    cgFunc.GetCurBB()->AppendInsn(tmpInsn);

    bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(false);
    cgFunc.SetCurBB(*formerCurBB);
}

BB &AArch64GenProEpilog::GenStackGuardCheckInsn(BB &bb)
{
    if (!stackProtect) {
        return bb;
    }

    BB *formerCurBB = cgFunc.GetCurBB();
    cgFunc.GetDummyBB()->ClearInsns();
    cgFunc.SetCurBB(*(cgFunc.GetDummyBB()));
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);

    const MIRSymbol *stkGuardSym = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_guard")));
    StImmOperand &stOpnd = aarchCGFunc.CreateStImmOperand(*stkGuardSym, 0, 0);
    RegOperand &stAddrOpnd =
        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, GetPointerSize() * kBitsPerByte, kRegTyInt);
    aarchCGFunc.SelectAddrof(stAddrOpnd, stOpnd);

    MemOperand *guardMemOp =
        aarchCGFunc.CreateMemOperand(MemOperand::kAddrModeBOi, GetPointerSize() * kBitsPerByte, stAddrOpnd, nullptr,
                                     &aarchCGFunc.GetOrCreateOfstOpnd(0, k32BitSize), stkGuardSym);
    MOperator mOp = aarchCGFunc.PickLdInsn(k64BitSize, PTY_u64);
    Insn &insn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, stAddrOpnd, *guardMemOp);
    insn.SetDoNotRemove(true);
    cgFunc.GetCurBB()->AppendInsn(insn);

    uint64 vArea = 0;
    if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
        AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
        if (ml->GetSizeOfGRSaveArea() > 0) {
            vArea += RoundUp(ml->GetSizeOfGRSaveArea(), kAarch64StackPtrAlignment);
        }
        if (ml->GetSizeOfVRSaveArea() > 0) {
            vArea += RoundUp(ml->GetSizeOfVRSaveArea(), kAarch64StackPtrAlignment);
        }
    }

    RegOperand &checkOp =
        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R10, GetPointerSize() * kBitsPerByte, kRegTyInt);
    int32 stkSize = static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    if (useFP) {
        stkSize -=
            (static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->SizeOfArgsToStackPass()) +
             cgFunc.GetFunction().GetFrameReseverdSlot());
    }
    int32 memSize = (stkSize - kOffset8MemPos) - static_cast<int32>(vArea);
    MemOperand *downStk = aarchCGFunc.CreateStackMemOpnd(stackBaseReg, memSize, GetPointerSize() * kBitsPerByte);
    if (downStk->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*downStk, k64BitSize)) {
        downStk = &aarchCGFunc.SplitOffsetWithAddInstruction(*downStk, k64BitSize, R10);
    }
    mOp = aarchCGFunc.PickLdInsn(GetPointerSize() * kBitsPerByte, PTY_u64);
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, checkOp, *downStk);
    newInsn.SetDoNotRemove(true);
    cgFunc.GetCurBB()->AppendInsn(newInsn);

    cgFunc.SelectBxor(stAddrOpnd, stAddrOpnd, checkOp, PTY_u64);
    LabelIdx failLable = aarchCGFunc.CreateLabel();
    aarchCGFunc.SelectCondGoto(aarchCGFunc.GetOrCreateLabelOperand(failLable), OP_brtrue, OP_eq, stAddrOpnd,
                               aarchCGFunc.CreateImmOperand(0, k64BitSize, false), PTY_u64, false);

    bb.AppendBBInsns(*(cgFunc.GetCurBB()));

    LabelIdx nextBBLableIdx = aarchCGFunc.CreateLabel();
    BB *nextBB = aarchCGFunc.CreateNewBB(nextBBLableIdx, bb.IsUnreachable(), BB::kBBFallthru, bb.GetFrequency());
    bb.AppendBB(*nextBB);
    bb.PushBackSuccs(*nextBB);
    nextBB->PushBackPreds(bb);
    cgFunc.SetCurBB(*nextBB);
    MIRSymbol *failFunc = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(
        GlobalTables::GetStrTable().GetStrIdxFromName(std::string("__stack_chk_fail")));
    ListOperand *srcOpnds = aarchCGFunc.CreateListOpnd(*cgFunc.GetFuncScopeAllocator());
    Insn &callInsn = aarchCGFunc.AppendCall(*failFunc, *srcOpnds);
    callInsn.SetDoNotRemove(true);

    BB *newBB = cgFunc.CreateNewBB(failLable, bb.IsUnreachable(), bb.GetKind(), bb.GetFrequency());
    nextBB->AppendBB(*newBB);
    if (cgFunc.GetLastBB() == &bb) {
        cgFunc.SetLastBB(*newBB);
    }
    bb.PushBackSuccs(*newBB);
    nextBB->PushBackSuccs(*newBB);
    newBB->PushBackPreds(*nextBB);
    newBB->PushBackPreds(bb);

    bb.SetKind(BB::kBBIf);
    cgFunc.SetCurBB(*formerCurBB);
    return *newBB;
}

bool AArch64GenProEpilog::InsertOpndRegs(Operand &op, std::set<regno_t> &vecRegs) const
{
    Operand *opnd = &op;
    CHECK_FATAL(opnd != nullptr, "opnd is nullptr in InsertRegs");
    if (opnd->IsList()) {
        MapleList<RegOperand *> pregList = static_cast<ListOperand *>(opnd)->GetOperands();
        for (auto *preg : pregList) {
            if (preg != nullptr) {
                vecRegs.insert(preg->GetRegisterNumber());
            }
        }
    }
    if (opnd->IsMemoryAccessOperand()) { /* the registers of kOpdMem are complex to be detected */
        RegOperand *baseOpnd = static_cast<MemOperand *>(opnd)->GetBaseRegister();
        if (baseOpnd != nullptr) {
            vecRegs.insert(baseOpnd->GetRegisterNumber());
        }
        RegOperand *indexOpnd = static_cast<MemOperand *>(opnd)->GetIndexRegister();
        if (indexOpnd != nullptr) {
            vecRegs.insert(indexOpnd->GetRegisterNumber());
        }
    }
    if (opnd->IsRegister()) {
        RegOperand *preg = static_cast<RegOperand *>(opnd);
        if (preg != nullptr) {
            vecRegs.insert(preg->GetRegisterNumber());
        }
    }
    return true;
}

bool AArch64GenProEpilog::InsertInsnRegs(Insn &insn, bool insertSource, std::set<regno_t> &vecSourceRegs,
                                         bool insertTarget, std::set<regno_t> &vecTargetRegs)
{
    Insn *curInsn = &insn;
    for (uint32 o = 0; o < curInsn->GetOperandSize(); ++o) {
        Operand &opnd = curInsn->GetOperand(o);
        if (insertSource && curInsn->OpndIsUse(o)) {
            InsertOpndRegs(opnd, vecSourceRegs);
        }
        if (insertTarget && curInsn->OpndIsDef(o)) {
            InsertOpndRegs(opnd, vecTargetRegs);
        }
    }
    return true;
}

bool AArch64GenProEpilog::FindRegs(Operand &op, std::set<regno_t> &vecRegs) const
{
    Operand *opnd = &op;
    if (opnd == nullptr || vecRegs.empty()) {
        return false;
    }
    if (opnd->IsList()) {
        MapleList<RegOperand *> pregList = static_cast<ListOperand *>(opnd)->GetOperands();
        for (auto *preg : pregList) {
            if (preg->GetRegisterNumber() == R29 || vecRegs.find(preg->GetRegisterNumber()) != vecRegs.end()) {
                return true; /* the opReg will overwrite or reread the vecRegs */
            }
        }
    }
    if (opnd->IsMemoryAccessOperand()) { /* the registers of kOpdMem are complex to be detected */
        RegOperand *baseOpnd = static_cast<MemOperand *>(opnd)->GetBaseRegister();
        RegOperand *indexOpnd = static_cast<MemOperand *>(opnd)->GetIndexRegister();
        if ((baseOpnd != nullptr && baseOpnd->GetRegisterNumber() == R29) ||
            (indexOpnd != nullptr && indexOpnd->GetRegisterNumber() == R29)) {
            return true; /* Avoid modifying data on the stack */
        }
        if ((baseOpnd != nullptr && vecRegs.find(baseOpnd->GetRegisterNumber()) != vecRegs.end()) ||
            (indexOpnd != nullptr && vecRegs.find(indexOpnd->GetRegisterNumber()) != vecRegs.end())) {
            return true;
        }
    }
    if (opnd->IsRegister()) {
        RegOperand *regOpnd = static_cast<RegOperand *>(opnd);
        if (regOpnd->GetRegisterNumber() == R29 || vecRegs.find(regOpnd->GetRegisterNumber()) != vecRegs.end()) {
            return true; /* dst is a target register, result_dst is a target register */
        }
    }
    return false;
}

bool AArch64GenProEpilog::BackwardFindDependency(BB &ifbb, std::set<regno_t> &vecReturnSourceRegs,
                                                 std::list<Insn *> &existingInsns, std::list<Insn *> &moveInsns)
{
    /*
     * Pattern match,(*) instruction are moved down below branch.
     *   ********************
     *   curInsn: <instruction> <target> <source>
     *   <existingInsns> in predBB
     *   <existingInsns> in ifBB
     *   <existingInsns> in returnBB
     *   *********************
     *                        list: the insns can be moved into the coldBB
     * (1) the instruction is neither a branch nor a call, except for the ifbb.GetLastInsn()
     *     As long as a branch insn exists,
     *     the fast path finding fails and the return value is false,
     *     but the code sinking can be continued.
     * (2) the predBB is not a ifBB,
     *     As long as a ifBB in preds exists,
     *     the code sinking fails,
     *     but fast path finding can be continued.
     * (3) the targetRegs of insns in existingInsns can neither be reread or overwrite
     * (4) the sourceRegs of insns in existingInsns can not be overwrite
     * (5) the sourceRegs of insns in returnBB can neither be reread or overwrite
     * (6) the targetRegs and sourceRegs cannot be R29 R30, to protect the stack
     * (7) modified the reg when:
     *     --------------
     *     curInsn: move R2,R1
     *     <existingInsns>: <instruction>s <target>s <source>s
     *                      <instruction>s <target>s <source-R2>s
     *                      -> <instruction>s <target>s <source-R1>s
     *     ------------
     *     (a) all targets cannot be R1, all sources cannot be R1
     *         all targets cannot be R2, all return sources cannot be R2
     *     (b) the targetRegs and sourceRegs cannot be list or MemoryAccess
     *     (c) no ifBB in preds, no branch insns
     *     (d) the bits of source-R2 must be equal to the R2
     *     (e) replace the R2 with R1
     */
    BB *pred = &ifbb;
    std::set<regno_t> vecTargetRegs; /* the targrtRegs of existingInsns */
    std::set<regno_t> vecSourceRegs; /* the soureRegs of existingInsns */
    bool ifPred = false;             /* Indicates whether a ifBB in pred exists */
    bool bl = false;                 /* Indicates whether a branch insn exists */
    do {
        FOR_BB_INSNS_REV(insn, pred) {
            /* code sinking */
            if (insn->IsImmaterialInsn()) {
                moveInsns.push_back(insn);
                continue;
            }
            /* code sinking */
            if (!insn->IsMachineInstruction()) {
                moveInsns.push_back(insn);
                continue;
            }
            /* code sinking fails, the insns must be retained in the ifBB */
            if (ifPred || insn == ifbb.GetLastInsn() || insn->IsBranch() || insn->IsCall() || insn->IsStore() ||
                insn->IsStorePair()) {
                /* fast path finding fails */
                if (insn != ifbb.GetLastInsn() &&
                    (insn->IsBranch() || insn->IsCall() || insn->IsStore() || insn->IsStorePair())) {
                    bl = true;
                }
                InsertInsnRegs(*insn, true, vecSourceRegs, true, vecTargetRegs);
                existingInsns.push_back(insn);
                continue;
            }
            bool allow = true; /* whether allow this insn move into the codeBB */
            for (uint32 o = 0; allow && o < insn->GetOperandSize(); ++o) {
                Operand &opnd = insn->GetOperand(o);
                if (insn->OpndIsDef(o)) {
                    allow = allow & !FindRegs(opnd, vecTargetRegs);
                    allow = allow & !FindRegs(opnd, vecSourceRegs);
                    allow = allow & !FindRegs(opnd, vecReturnSourceRegs);
                }
                if (insn->OpndIsUse(o)) {
                    allow = allow & !FindRegs(opnd, vecTargetRegs);
                }
            }
            /* if a result_dst not allowed, this insn can be allowed on the condition of mov Rx,R0/R1,
             * and tje existing insns cannot be blr
             * RLR 31, RFP 32, RSP 33, RZR 34 */
            if (!ifPred && !bl && !allow &&
                (insn->GetMachineOpcode() == MOP_xmovrr || insn->GetMachineOpcode() == MOP_wmovrr)) {
                Operand *resultOpnd = &(insn->GetOperand(0));
                Operand *srcOpnd = &(insn->GetOperand(1));
                regno_t resultNO = static_cast<RegOperand *>(resultOpnd)->GetRegisterNumber();
                regno_t srcNO = static_cast<RegOperand *>(srcOpnd)->GetRegisterNumber();
                if (!FindRegs(*resultOpnd, vecTargetRegs) && !FindRegs(*srcOpnd, vecTargetRegs) &&
                    !FindRegs(*srcOpnd, vecSourceRegs) && !FindRegs(*srcOpnd, vecReturnSourceRegs) &&
                    (srcNO < RLR || srcNO > RZR)) {
                    allow = true; /* allow on the conditional mov Rx,Rxx */
                    for (auto *exit : existingInsns) {
                        /* the registers of kOpdMem are complex to be detected */
                        for (uint32 o = 0; o < exit->GetOperandSize(); ++o) {
                            if (!exit->OpndIsUse(o)) {
                                continue;
                            }
                            Operand *opd = &(exit->GetOperand(o));
                            if (opd->IsList() || opd->IsMemoryAccessOperand()) {
                                allow = false;
                                break;
                            }
                            /* Distinguish between 32-bit regs and 64-bit regs */
                            if (opd->IsRegister() && static_cast<RegOperand *>(opd)->GetRegisterNumber() == resultNO &&
                                opd != resultOpnd) {
                                allow = false;
                                break;
                            }
                        }
                    }
                }
                /* replace the R2 with R1 */
                if (allow) {
                    for (auto *exit : existingInsns) {
                        for (uint32 o = 0; o < exit->GetOperandSize(); ++o) {
                            if (!exit->OpndIsUse(o)) {
                                continue;
                            }
                            Operand *opd = &(exit->GetOperand(o));
                            if (opd->IsRegister() && (opd == resultOpnd)) {
                                exit->SetOperand(o, *srcOpnd);
                            }
                        }
                    }
                }
            }
            if (!allow) { /* all result_dsts are not target register */
                /* code sinking fails */
                InsertInsnRegs(*insn, true, vecSourceRegs, true, vecTargetRegs);
                existingInsns.push_back(insn);
            } else {
                moveInsns.push_back(insn);
            }
        }
        if (pred->GetPreds().empty()) {
            break;
        }
        if (!ifPred) {
            for (auto *tmPred : pred->GetPreds()) {
                pred = tmPred;
                /* try to find the BB without branch */
                if (tmPred->GetKind() == BB::kBBGoto || tmPred->GetKind() == BB::kBBFallthru) {
                    ifPred = false;
                    break;
                } else {
                    ifPred = true;
                }
            }
        }
    } while (pred != nullptr);
    for (std::set<regno_t>::iterator it = vecTargetRegs.begin(); it != vecTargetRegs.end(); ++it) {
        if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(*it))) { /* flag register */
            return false;
        }
    }
    return !bl;
}

BB *AArch64GenProEpilog::IsolateFastPath(BB &bb)
{
    /*
     * Detect "if (cond) return" fast path, and move extra instructions
     * to the slow path.
     * Must match the following block structure. BB1 can be a series of
     * single-pred/single-succ blocks.
     *     BB1 ops1 cmp-br to BB3        BB1 cmp-br to BB3
     *     BB2 ops2 br to retBB    ==>   BB2 ret
     *     BB3 slow path                 BB3 ops1 ops2
     * if the detect is successful, BB3 will be used to generate prolog stuff.
     */
    if (bb.GetPrev() != nullptr) {
        return nullptr;
    }
    BB *ifBB = nullptr;
    BB *returnBB = nullptr;
    BB *coldBB = nullptr;
    {
        BB *curBB = &bb;
        /* Look for straight line code */
        while (1) {
            if (!curBB->GetEhSuccs().empty()) {
                return nullptr;
            }
            if (curBB->GetSuccs().size() == 1) {
                if (curBB->HasCall()) {
                    return nullptr;
                }
                BB *succ = curBB->GetSuccs().front();
                if (succ->GetPreds().size() != 1 || !succ->GetEhPreds().empty()) {
                    return nullptr;
                }
                curBB = succ;
            } else if (curBB->GetKind() == BB::kBBIf) {
                ifBB = curBB;
                break;
            } else {
                return nullptr;
            }
        }
    }
    /* targets of if bb can only be reached by if bb */
    {
        CHECK_FATAL(!ifBB->GetSuccs().empty(), "null succs check!");
        BB *first = ifBB->GetSuccs().front();
        BB *second = ifBB->GetSuccs().back();
        if (first->GetPreds().size() != 1 || !first->GetEhPreds().empty()) {
            return nullptr;
        }
        if (second->GetPreds().size() != 1 || !second->GetEhPreds().empty()) {
            return nullptr;
        }
        /* One target of the if bb jumps to a return bb */
        if (first->GetKind() != BB::kBBGoto && first->GetKind() != BB::kBBFallthru) {
            return nullptr;
        }
        if (first->GetSuccs().size() != 1) {
            return nullptr;
        }
        if (first->GetSuccs().front()->GetKind() != BB::kBBReturn) {
            return nullptr;
        }
        if (first->GetSuccs().front()->GetPreds().size() != 1) {
            return nullptr;
        }
        if (first->GetSuccs().front()->NumInsn() > kInsnNum2) { /* avoid a insn is used to debug */
            return nullptr;
        }
        if (second->GetSuccs().empty()) {
            return nullptr;
        }
        returnBB = first;
        coldBB = second;
    }
    /* Search backward looking for dependencies for the cond branch */
    std::list<Insn *> existingInsns; /* the insns must be retained in the ifBB (and the return BB) */
    std::list<Insn *> moveInsns;     /* instructions to be moved to coldbb */
    /*
     * The control flow matches at this point.
     * Make sure the SourceRegs of the insns in returnBB (vecReturnSourceReg) cannot be overwrite.
     * the regs in insns have three forms: list, MemoryAccess, or Register.
     */
    CHECK_FATAL(returnBB != nullptr, "null ptr check");
    std::set<regno_t> vecReturnSourceRegs;
    FOR_BB_INSNS_REV(insn, returnBB) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (insn->IsBranch() || insn->IsCall() || insn->IsStore() || insn->IsStorePair()) {
            return nullptr;
        }
        InsertInsnRegs(*insn, true, vecReturnSourceRegs, false, vecReturnSourceRegs);
        existingInsns.push_back(insn);
    }
    FOR_BB_INSNS_REV(insn, returnBB->GetSuccs().front()) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (insn->IsBranch() || insn->IsCall() || insn->IsStore() || insn->IsStorePair()) {
            return nullptr;
        }
        InsertInsnRegs(*insn, true, vecReturnSourceRegs, false, vecReturnSourceRegs);
        existingInsns.push_back(insn);
    }
    /*
     * The mv is the 1st move using the parameter register leading to the branch
     * The ld is the load using the parameter register indirectly for the branch
     * The depMv is the move which preserves the result of the load but might
     *    destroy a parameter register which will be moved below the branch.
     */
    bool fast = BackwardFindDependency(*ifBB, vecReturnSourceRegs, existingInsns, moveInsns);
    /* move extra instructions to the slow path */
    if (!fast) {
        return nullptr;
    }
    for (auto in : moveInsns) {
        in->GetBB()->RemoveInsn(*in);
        CHECK_FATAL(coldBB != nullptr, "null ptr check");
        static_cast<void>(coldBB->InsertInsnBegin(*in));
    }
    /* All instructions are in the right place, replace branch to ret bb to just ret. */
    /* Remove the lastInsn of gotoBB */
    if (returnBB->GetKind() == BB::kBBGoto) {
        returnBB->RemoveInsn(*returnBB->GetLastInsn());
    }
    BB *tgtBB = returnBB->GetSuccs().front();
    CHECK_FATAL(tgtBB != nullptr, "null ptr check");
    FOR_BB_INSNS(insn, tgtBB) {
        returnBB->AppendInsn(*insn); /* add the insns such as MOP_xret */
    }
    returnBB->AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_xret));
    /* bb is now a retbb and has no succ. */
    returnBB->SetKind(BB::kBBReturn);
    auto predIt = std::find(tgtBB->GetPredsBegin(), tgtBB->GetPredsEnd(), returnBB);
    tgtBB->ErasePreds(predIt);
    tgtBB->ClearInsns();
    returnBB->ClearSuccs();
    if (tgtBB->GetPrev() != nullptr && tgtBB->GetNext() != nullptr) {
        tgtBB->GetPrev()->SetNext(tgtBB->GetNext());
        tgtBB->GetNext()->SetPrev(tgtBB->GetPrev());
    }
    SetFastPathReturnBB(tgtBB);
    return coldBB;
}

MemOperand *AArch64GenProEpilog::SplitStpLdpOffsetForCalleeSavedWithAddInstruction(CGFunc &cgFunc, const MemOperand &mo,
                                                                                   uint32 bitLen, AArch64reg baseRegNum)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CHECK_FATAL(mo.GetAddrMode() == MemOperand::kAddrModeBOi, "mode should be kAddrModeBOi");
    OfstOperand *ofstOp = mo.GetOffsetImmediate();
    int32 offsetVal = static_cast<int32>(ofstOp->GetOffsetValue());
    CHECK_FATAL(offsetVal > 0, "offsetVal should be greater than 0");
    CHECK_FATAL((static_cast<uint32>(offsetVal) & 0x7) == 0, "(offsetVal & 0x7) should be equal to 0");
    /*
     * Offset adjustment due to FP/SP has already been done
     * in AArch64GenProEpilog::GeneratePushRegs() and AArch64GenProEpilog::GeneratePopRegs()
     */
    RegOperand &br = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(baseRegNum, bitLen, kRegTyInt);
    if (aarchCGFunc.GetSplitBaseOffset() == 0) {
        aarchCGFunc.SetSplitBaseOffset(offsetVal); /* remember the offset; don't forget to clear it */
        ImmOperand &immAddEnd = aarchCGFunc.CreateImmOperand(offsetVal, k64BitSize, true);
        RegOperand *origBaseReg = mo.GetBaseRegister();
        aarchCGFunc.SelectAdd(br, *origBaseReg, immAddEnd, PTY_i64);
    }
    offsetVal = offsetVal - aarchCGFunc.GetSplitBaseOffset();
    return &aarchCGFunc.CreateReplacementMemOperand(bitLen, br, offsetVal);
}

void AArch64GenProEpilog::AppendInstructionPushPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty,
                                                    int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
    Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerSize() * kBitsPerByte);

    uint32 dataSize = GetPointerSize() * kBitsPerByte;
    CHECK_FATAL(offset >= 0, "offset must >= 0");
    if (offset > kStpLdpImm64UpperBound) {
        o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc, *static_cast<MemOperand *>(o2), dataSize, R16);
    }
    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
    std::string comment = "SAVE CALLEE REGISTER PAIR";
    pushInsn.SetComment(comment);
    AppendInstructionTo(pushInsn, cgFunc);

    /* Append CFi code */
    if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
        int32 stackFrameSize =
            static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
        stackFrameSize -= (static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass()) +
                           cgFunc.GetFunction().GetFrameReseverdSlot());
        int32 cfiOffset = stackFrameSize - offset;
        BB *curBB = cgFunc.GetCurBB();
        Insn *newInsn = curBB->InsertInsnAfter(pushInsn, aarchCGFunc.CreateCfiOffsetInsn(reg0, -cfiOffset, k64BitSize));
        curBB->InsertInsnAfter(*newInsn,
                               aarchCGFunc.CreateCfiOffsetInsn(reg1, -cfiOffset + kOffset8MemPos, k64BitSize));
    }
}

void AArch64GenProEpilog::AppendInstructionPushSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopSingle];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, GetPointerSize() * kBitsPerByte, rty);
    Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerSize() * kBitsPerByte);

    MemOperand *aarchMemO1 = static_cast<MemOperand *>(o1);
    uint32 dataSize = GetPointerSize() * kBitsPerByte;
    if (aarchMemO1->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
        o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R9);
    }

    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o1);
    std::string comment = "SAVE CALLEE REGISTER";
    pushInsn.SetComment(comment);
    AppendInstructionTo(pushInsn, cgFunc);

    /* Append CFI code */
    if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
        int32 stackFrameSize =
            static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
        stackFrameSize -= (static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass()) +
                           cgFunc.GetFunction().GetFrameReseverdSlot());
        int32 cfiOffset = stackFrameSize - offset;
        cgFunc.GetCurBB()->InsertInsnAfter(pushInsn, aarchCGFunc.CreateCfiOffsetInsn(reg, -cfiOffset, k64BitSize));
    }
}

Insn &AArch64GenProEpilog::AppendInstructionForAllocateOrDeallocateCallFrame(int64 fpToSpDistance, AArch64reg reg0,
                                                                             AArch64reg reg1, RegType rty,
                                                                             bool isAllocate)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopPair];
    uint8 size;
    if (CGOptions::IsArm64ilp32()) {
        size = k8ByteSize;
    } else {
        size = GetPointerSize();
    }
    if (fpToSpDistance <= kStrLdrImm64UpperBound - kOffset8MemPos) {
        mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopSingle] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
        RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, size * kBitsPerByte, rty);
        MemOperand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), size * kBitsPerByte);
        Insn &insn1 = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o2);
        AppendInstructionTo(insn1, cgFunc);
        RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, size * kBitsPerByte, rty);
        o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance + size), size * kBitsPerByte);
        Insn &insn2 = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
        AppendInstructionTo(insn2, cgFunc);
        return insn2;
    } else {
        RegOperand &oo = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, size * kBitsPerByte, kRegTyInt);
        ImmOperand &io1 = aarchCGFunc.CreateImmOperand(fpToSpDistance, k64BitSize, true);
        aarchCGFunc.SelectCopyImm(oo, io1, PTY_i64);
        RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, size * kBitsPerByte, rty);
        RegOperand &rsp = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, size * kBitsPerByte, kRegTyInt);
        MemOperand *mo = aarchCGFunc.CreateMemOperand(MemOperand::kAddrModeBOrX, size * kBitsPerByte, rsp, oo, 0);
        Insn &insn1 = cgFunc.GetInsnBuilder()->BuildInsn(isAllocate ? MOP_xstr : MOP_xldr, o0, *mo);
        AppendInstructionTo(insn1, cgFunc);
        ImmOperand &io2 = aarchCGFunc.CreateImmOperand(size, k64BitSize, true);
        aarchCGFunc.SelectAdd(oo, oo, io2, PTY_i64);
        RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, size * kBitsPerByte, rty);
        mo = aarchCGFunc.CreateMemOperand(MemOperand::kAddrModeBOrX, size * kBitsPerByte, rsp, oo, 0);
        Insn &insn2 = cgFunc.GetInsnBuilder()->BuildInsn(isAllocate ? MOP_xstr : MOP_xldr, o1, *mo);
        AppendInstructionTo(insn2, cgFunc);
        return insn2;
    }
}

Insn &AArch64GenProEpilog::CreateAndAppendInstructionForAllocateCallFrame(int64 fpToSpDistance, AArch64reg reg0,
                                                                          AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
    Insn *allocInsn = nullptr;
    if (fpToSpDistance > kStpLdpImm64UpperBound) {
        allocInsn = &AppendInstructionForAllocateOrDeallocateCallFrame(fpToSpDistance, reg0, reg1, rty, true);
    } else {
        Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
        Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
        Operand *o2 =
            aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), GetPointerSize() * kBitsPerByte);
        allocInsn = &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
        AppendInstructionTo(*allocInsn, cgFunc);
    }
    if (currCG->NeedInsertInstrumentationFunction()) {
        aarchCGFunc.AppendCall(*currCG->GetInstrumentationFunction());
    } else if (currCG->InstrumentWithDebugTraceCall()) {
        aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
    } else if (currCG->InstrumentWithProfile()) {
        aarchCGFunc.AppendCall(*currCG->GetProfileFunction());
    }
    return *allocInsn;
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame"));
    }

    Insn *ipoint = nullptr;
    /*
     * stackFrameSize includes the size of args to stack-pass
     * if a function has neither VLA nor alloca.
     */
    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    /*
     * ldp/stp's imm should be within -512 and 504;
     * if stp's imm > 512, we fall back to the stp-sub version
     */
    bool useStpSub = false;
    int64 offset = 0;
    int32 cfiOffset = 0;
    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        /*
         * stack_frame_size == size of formal parameters + callee-saved (including FP/RL)
         *                     + size of local vars
         *                     + size of actuals
         * (when passing more than 8 args, its caller's responsibility to
         *  allocate space for it. size of actuals represent largest such size in the function.
         */
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
        ipoint = cgFunc.GetCurBB()->GetLastInsn();
        cfiOffset = stackFrameSize;
    } else {
        if (stackFrameSize > kStpLdpImm64UpperBound) {
            useStpSub = true;
            offset = kOffset16MemPos;
            stackFrameSize -= offset;
        } else {
            offset = stackFrameSize;
        }
        MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
        RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
        RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
        MemOperand &o2 =
            aarchCGFunc.CreateCallFrameOperand(static_cast<int32>(-offset), GetPointerSize() * kBitsPerByte);
        ipoint = &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2);
        AppendInstructionTo(*ipoint, cgFunc);
        cfiOffset = offset;
        if (currCG->NeedInsertInstrumentationFunction()) {
            aarchCGFunc.AppendCall(*currCG->GetInstrumentationFunction());
        } else if (currCG->InstrumentWithDebugTraceCall()) {
            aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
        } else if (currCG->InstrumentWithProfile()) {
            aarchCGFunc.AppendCall(*currCG->GetProfileFunction());
        }
    }

    ipoint = InsertCFIDefCfaOffset(cfiOffset, *ipoint);

    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        CHECK_FATAL(!useStpSub, "Invalid assumption");
        ipoint = &CreateAndAppendInstructionForAllocateCallFrame(fpToSpDistance, reg0, reg1, rty);
    }

    if (useStpSub) {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
        ipoint = cgFunc.GetCurBB()->GetLastInsn();
        aarchCGFunc.SetUsedStpSubPairForCallFrameAllocation(true);
    }

    CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
    int32 cfiOffsetSecond = 0;
    if (useStpSub) {
        cfiOffsetSecond = stackFrameSize;
        ipoint = InsertCFIDefCfaOffset(cfiOffsetSecond, *ipoint);
    }
    cfiOffsetSecond = GetOffsetFromCFA();
    if (!cgFunc.HasVLAOrAlloca()) {
        cfiOffsetSecond -= fpToSpDistance;
    }
    if (cgFunc.GenCfi()) {
        BB *curBB = cgFunc.GetCurBB();
        if (useFP) {
            ipoint = curBB->InsertInsnAfter(
                *ipoint, aarchCGFunc.CreateCfiOffsetInsn(stackBaseReg, -cfiOffsetSecond, k64BitSize));
        }
        curBB->InsertInsnAfter(*ipoint,
                               aarchCGFunc.CreateCfiOffsetInsn(RLR, -cfiOffsetSecond + kOffset8MemPos, k64BitSize));
    }
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame for debugging"));
    }

    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance =
        (cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot());

    Insn *ipoint = nullptr;
    int32 cfiOffset = 0;

    if (fpToSpDistance > 0) {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
        ipoint = cgFunc.GetCurBB()->GetLastInsn();
        cfiOffset = stackFrameSize;
        (void)InsertCFIDefCfaOffset(cfiOffset, *ipoint);
        if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
            fpToSpDistance -= (kDivide2 * k8ByteSize);
        }
        ipoint = &CreateAndAppendInstructionForAllocateCallFrame(fpToSpDistance, reg0, reg1, rty);
        CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
        cfiOffset = GetOffsetFromCFA();
        cfiOffset -= fpToSpDistance;
    } else {
        bool useStpSub = false;

        if (stackFrameSize > kStpLdpImm64UpperBound) {
            useStpSub = true;
            RegOperand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
            ImmOperand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
            aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
            ipoint = cgFunc.GetCurBB()->GetLastInsn();
            cfiOffset = stackFrameSize;
            ipoint = InsertCFIDefCfaOffset(cfiOffset, *ipoint);
        } else {
            MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
            RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
            RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
            MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(-stackFrameSize, GetPointerSize() * kBitsPerByte);
            ipoint = &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2);
            AppendInstructionTo(*ipoint, cgFunc);
            cfiOffset = stackFrameSize;
            ipoint = InsertCFIDefCfaOffset(cfiOffset, *ipoint);
        }

        if (useStpSub) {
            MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
            RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
            RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
            MemOperand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, 0, GetPointerSize() * kBitsPerByte);
            ipoint = &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
            AppendInstructionTo(*ipoint, cgFunc);
        }

        if (currCG->NeedInsertInstrumentationFunction()) {
            aarchCGFunc.AppendCall(*currCG->GetInstrumentationFunction());
        } else if (currCG->InstrumentWithDebugTraceCall()) {
            aarchCGFunc.AppendCall(*currCG->GetDebugTraceEnterFunction());
        } else if (currCG->InstrumentWithProfile()) {
            aarchCGFunc.AppendCall(*currCG->GetProfileFunction());
        }

        CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
        cfiOffset = GetOffsetFromCFA();
    }
    if (cgFunc.GenCfi()) {
        BB *curBB = cgFunc.GetCurBB();
        if (useFP) {
            ipoint =
                curBB->InsertInsnAfter(*ipoint, aarchCGFunc.CreateCfiOffsetInsn(stackBaseReg, -cfiOffset, k64BitSize));
        }
        curBB->InsertInsnAfter(*ipoint, aarchCGFunc.CreateCfiOffsetInsn(RLR, -cfiOffset + kOffset8MemPos, k64BitSize));
    }
}

/*
 *  From AArch64 Reference Manual
 *  C1.3.3 Load/Store Addressing Mode
 *  ...
 *  When stack alignment checking is enabled by system software and
 *  the base register is the SP, the current stack pointer must be
 *  initially quadword aligned, that is aligned to 16 bytes. Misalignment
 *  generates a Stack Alignment fault.  The offset does not have to
 *  be a multiple of 16 bytes unless the specific Load/Store instruction
 *  requires this. SP cannot be used as a register offset.
 */
void AArch64GenProEpilog::GeneratePushRegs()
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    const MapleVector<AArch64reg> &regsToSave =
        (!CGOptions::DoRegSavesOpt()) ? aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();

    CHECK_FATAL(!regsToSave.empty(), "FP/LR not added to callee-saved list?");

    AArch64reg intRegFirstHalf = kRinvalid;
    AArch64reg fpRegFirstHalf = kRinvalid;

    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("save callee-saved registers"));
    }

    /*
     * Even if we don't use RFP, since we push a pair of registers in one instruction
     * and the stack needs be aligned on a 16-byte boundary, push RFP as well if function has a call
     * Make sure this is reflected when computing callee_saved_regs.size()
     */
    if (!currCG->GenerateDebugFriendlyCode()) {
        AppendInstructionAllocateCallFrame(R29, RLR, kRegTyInt);
    } else {
        AppendInstructionAllocateCallFrameDebug(R29, RLR, kRegTyInt);
    }

    if (useFP) {
        if (currCG->GenerateVerboseCG()) {
            cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("copy SP to FP"));
        }
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
        int64 fpToSpDistance =
            (cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot());
        bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
        if ((fpToSpDistance > 0) || isLmbc) {
            Operand *immOpnd;
            if (isLmbc) {
                int32 size =
                    static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
                immOpnd = &aarchCGFunc.CreateImmOperand(size, k32BitSize, true);
            } else {
                immOpnd = &aarchCGFunc.CreateImmOperand(fpToSpDistance, k32BitSize, true);
            }
            if (!isLmbc || cgFunc.SeenFP() || cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
                aarchCGFunc.SelectAdd(fpOpnd, spOpnd, *immOpnd, PTY_u64);
            }
            cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
            if (cgFunc.GenCfi()) {
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(
                    stackBaseReg,
                    static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize() - fpToSpDistance,
                    k64BitSize));
            }
        } else {
            aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
            cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
            if (cgFunc.GenCfi()) {
                cgFunc.GetCurBB()->AppendInsn(
                    cgFunc.GetInsnBuilder()
                        ->BuildCfiInsn(cfi::OP_CFI_def_cfa_register)
                        .AddOpndChain(aarchCGFunc.CreateCfiRegOperand(stackBaseReg, k64BitSize)));
            }
        }
    }

    MapleVector<AArch64reg>::const_iterator it = regsToSave.begin();
    /* skip the first two registers */
    CHECK_FATAL(*it == RFP, "The first callee saved reg is expected to be RFP");
    ++it;
    CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
    ++it;

    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    int32 offset;
    if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        offset = static_cast<int32>((memLayout->RealStackFrameSize() - aarchCGFunc.SizeOfCalleeSaved()) -
                                    memLayout->GetSizeOfLocals());
    } else {
        offset = (static_cast<int32>(memLayout->RealStackFrameSize() -
                                     (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen))) - /* for FP/LR */
                  memLayout->SizeOfArgsToStackPass() -
                  cgFunc.GetFunction().GetFrameReseverdSlot());
    }

    if (cgFunc.GetCG()->IsStackProtectorStrong() || cgFunc.GetCG()->IsStackProtectorAll()) {
        offset -= kAarch64StackPtrAlignment;
    }

    if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
        /* GR/VR save areas are above the callee save area */
        AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
        auto saveareasize = static_cast<int32>(RoundUp(ml->GetSizeOfGRSaveArea(), GetPointerSize() * k2BitSize) +
                                               RoundUp(ml->GetSizeOfVRSaveArea(), GetPointerSize() * k2BitSize));
        offset -= saveareasize;
    }

    std::vector<std::pair<uint16, int32>> calleeRegAndOffsetVec;
    for (; it != regsToSave.end(); ++it) {
        AArch64reg reg = *it;
        CHECK_FATAL(reg != RFP, "stray RFP in callee_saved_list?");
        CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");
        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
        if (firstHalf == kRinvalid) {
            /* remember it */
            firstHalf = reg;
            /* for int callee-saved register: x19->19,x20->20 ...
               for float callee-saved register: d8->72, d9->73 ..., d15->79
            */
            uint16 regNO = (regType == kRegTyInt) ? static_cast<uint16>(reg - 1) : static_cast<uint16>(reg - V8 + 72);
            calleeRegAndOffsetVec.push_back(std::pair<uint16, int32>(regNO, offset));
        } else {
            uint16 regNO = (regType == kRegTyInt) ? static_cast<uint16>(reg - 1) : static_cast<uint16>(reg - V8 + 72);
            calleeRegAndOffsetVec.push_back(std::pair<uint16, int32>(regNO, offset + k8ByteSize));
            AppendInstructionPushPair(cgFunc, firstHalf, reg, regType, offset);
            GetNextOffsetCalleeSaved(offset);
            firstHalf = kRinvalid;
        }
    }

    if (intRegFirstHalf != kRinvalid) {
        AppendInstructionPushSingle(cgFunc, intRegFirstHalf, kRegTyInt, offset);
        GetNextOffsetCalleeSaved(offset);
    }

    if (fpRegFirstHalf != kRinvalid) {
        AppendInstructionPushSingle(cgFunc, fpRegFirstHalf, kRegTyFloat, offset);
        GetNextOffsetCalleeSaved(offset);
    }

    /*
     * in case we split stp/ldp instructions,
     * so that we generate a load-into-base-register instruction
     * for pop pairs as well.
     */
    aarchCGFunc.SetSplitBaseOffset(0);

    const auto &emitMemoryManager = CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemoryManager.codeSpace != nullptr) {
        emitMemoryManager.funcCalleeOffsetSaver(emitMemoryManager.codeSpace, cgFunc.GetName(), calleeRegAndOffsetVec);
        int64 fpToCurSpDistance =
            (cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot());
        int32 fp2PrevFrameSPDelta =
            static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize() - fpToCurSpDistance;
        emitMemoryManager.funcFpSPDeltaSaver(emitMemoryManager.codeSpace, cgFunc.GetName(), fp2PrevFrameSPDelta);
    }
}

void AArch64GenProEpilog::GeneratePushUnnamedVarargRegs()
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
        AArch64MemLayout *memlayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
        uint8 size;
        if (CGOptions::IsArm64ilp32()) {
            size = k8ByteSize;
        } else {
            size = GetPointerSize();
        }
        uint32 dataSizeBits = size * kBitsPerByte;
        uint32 offset;
        if (cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
            offset = static_cast<uint32>(memlayout->GetGRSaveAreaBaseLoc()); /* SP reference */
            if (memlayout->GetSizeOfGRSaveArea() % kAarch64StackPtrAlignment) {
                offset += size; /* End of area should be aligned. Hole between VR and GR area */
            }
        } else {
            offset = (UINT32_MAX - memlayout->GetSizeOfGRSaveArea()) + 1; /* FP reference */
            if (memlayout->GetSizeOfGRSaveArea() % kAarch64StackPtrAlignment) {
                offset -= size;
            }
        }
        uint32 grSize = (UINT32_MAX - offset) + 1;
        uint32 start_regno = k8BitSize - (memlayout->GetSizeOfGRSaveArea() / size);
        DEBUG_ASSERT(start_regno <= k8BitSize, "Incorrect starting GR regno for GR Save Area");
        for (uint32 i = start_regno + static_cast<uint32>(R0); i < static_cast<uint32>(R8); i++) {
            uint32 tmpOffset = 0;
            if (CGOptions::IsBigEndian()) {
                if ((dataSizeBits >> k8BitShift) < k8BitSize) {
                    tmpOffset += k8BitSize - (dataSizeBits >> k8BitShift);
                }
            }
            Operand *stackLoc;
            if (cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
                stackLoc = &aarchCGFunc.CreateStkTopOpnd(offset + tmpOffset, dataSizeBits);
            } else {
                stackLoc = aarchCGFunc.GenLmbcFpMemOperand(offset, size);
            }
            RegOperand &reg =
                aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyInt);
            Insn &inst =
                cgFunc.GetInsnBuilder()->BuildInsn(aarchCGFunc.PickStInsn(dataSizeBits, PTY_i64), reg, *stackLoc);
            cgFunc.GetCurBB()->AppendInsn(inst);
            offset += size;
        }
        if (!CGOptions::UseGeneralRegOnly()) {
            if (cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
                offset = static_cast<uint32>(memlayout->GetVRSaveAreaBaseLoc());
            } else {
                offset = (UINT32_MAX - (memlayout->GetSizeOfVRSaveArea() + grSize)) + 1;
            }
            start_regno = k8BitSize - (memlayout->GetSizeOfVRSaveArea() / (size * k2BitSize));
            DEBUG_ASSERT(start_regno <= k8BitSize, "Incorrect starting GR regno for VR Save Area");
            for (uint32 i = start_regno + static_cast<uint32>(V0); i < static_cast<uint32>(V8); i++) {
                uint32 tmpOffset = 0;
                if (CGOptions::IsBigEndian()) {
                    if ((dataSizeBits >> k8BitShift) < k16BitSize) {
                        tmpOffset += k16BitSize - (dataSizeBits >> k8BitShift);
                    }
                }
                Operand *stackLoc;
                if (cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
                    stackLoc = &aarchCGFunc.CreateStkTopOpnd(offset + tmpOffset, dataSizeBits);
                } else {
                    stackLoc = aarchCGFunc.GenLmbcFpMemOperand(offset, size);
                }
                RegOperand &reg =
                    aarchCGFunc.GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(i), k64BitSize, kRegTyFloat);
                Insn &inst =
                    cgFunc.GetInsnBuilder()->BuildInsn(aarchCGFunc.PickStInsn(dataSizeBits, PTY_f64), reg, *stackLoc);
                cgFunc.GetCurBB()->AppendInsn(inst);
                offset += (size * k2BitSize);
            }
        }
    }
}

void AArch64GenProEpilog::AppendInstructionStackCheck(AArch64reg reg, RegType rty, int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    /* sub x16, sp, #0x2000 */
    auto &x16Opnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, k64BitSize, rty);
    auto &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, rty);
    auto &imm1 = aarchCGFunc.CreateImmOperand(offset, k64BitSize, true);
    aarchCGFunc.SelectSub(x16Opnd, spOpnd, imm1, PTY_u64);

    /* ldr wzr, [x16] */
    auto &wzr = cgFunc.GetZeroOpnd(k32BitSize);
    auto &refX16 = aarchCGFunc.CreateMemOpnd(reg, 0, k64BitSize);
    auto &soeInstr = cgFunc.GetInsnBuilder()->BuildInsn(MOP_wldr, wzr, refX16);
    if (currCG->GenerateVerboseCG()) {
        soeInstr.SetComment("soerror");
    }
    soeInstr.SetDoNotRemove(true);
    AppendInstructionTo(soeInstr, cgFunc);
}

void AArch64GenProEpilog::GenerateProlog(BB &bb)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    BB *formerCurBB = cgFunc.GetCurBB();
    aarchCGFunc.GetDummyBB()->ClearInsns();
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(true);
    cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());
    if (!cgFunc.GetHasProEpilogue()) {
        return;
    }

    // insert .loc for function
    if (currCG->GetCGOptions().WithLoc() &&
        (!currCG->GetMIRModule()->IsCModule() || currCG->GetMIRModule()->IsWithDbgInfo())) {
        MIRFunction *func = &cgFunc.GetFunction();
        MIRSymbol *fSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
        if (currCG->GetCGOptions().WithSrc()) {
            uint32 tempmaxsize = static_cast<uint32>(currCG->GetMIRModule()->GetSrcFileInfo().size());
            uint32 endfilenum = currCG->GetMIRModule()->GetSrcFileInfo()[tempmaxsize - 1].second;
            if (fSym->GetSrcPosition().FileNum() != 0 && fSym->GetSrcPosition().FileNum() <= endfilenum) {
                Operand *o0 = cgFunc.CreateDbgImmOperand(fSym->GetSrcPosition().FileNum());
                int64_t lineNum = fSym->GetSrcPosition().LineNum();
                if (lineNum == 0) {
                    if (cgFunc.GetFunction().GetAttr(FUNCATTR_native)) {
                        lineNum = 0xffffe;
                    } else {
                        lineNum = 0xffffd;
                    }
                }
                Operand *o1 = cgFunc.CreateDbgImmOperand(lineNum);
                Insn &loc =
                    cgFunc.GetInsnBuilder()->BuildDbgInsn(mpldbg::OP_DBG_loc).AddOpndChain(*o0).AddOpndChain(*o1);
                cgFunc.GetCurBB()->AppendInsn(loc);
            }
        } else {
            Operand *o0 = cgFunc.CreateDbgImmOperand(1);
            Operand *o1 = cgFunc.CreateDbgImmOperand(fSym->GetSrcPosition().MplLineNum());
            Insn &loc = cgFunc.GetInsnBuilder()->BuildDbgInsn(mpldbg::OP_DBG_loc).AddOpndChain(*o0).AddOpndChain(*o1);
            cgFunc.GetCurBB()->AppendInsn(loc);
        }
    }

    const MapleVector<AArch64reg> &regsToSave =
        (!CGOptions::DoRegSavesOpt()) ? aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
    if (!regsToSave.empty()) {
        /*
         * Among other things, push the FP & LR pair.
         * FP/LR are added to the callee-saved list in AllocateRegisters()
         * We add them to the callee-saved list regardless of UseFP() being true/false.
         * Activation Frame is allocated as part of pushing FP/LR pair
         */
        GeneratePushRegs();
    } else {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        int32 stackFrameSize =
            static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
        if (stackFrameSize > 0) {
            if (currCG->GenerateVerboseCG()) {
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame"));
            }
            Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
            aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);

            int32 offset = stackFrameSize;
            (void)InsertCFIDefCfaOffset(offset, *(cgFunc.GetCurBB()->GetLastInsn()));
        }
        if (currCG->GenerateVerboseCG()) {
            cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("copy SP to FP"));
        }
        if (useFP) {
            Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
            bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
            int64 fpToSpDistance =
                cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
            if ((fpToSpDistance > 0) || isLmbc) {
                Operand *immOpnd;
                if (isLmbc) {
                    int32 size = static_cast<int32>(
                        static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
                    immOpnd = &aarchCGFunc.CreateImmOperand(size, k32BitSize, true);
                } else {
                    immOpnd = &aarchCGFunc.CreateImmOperand(fpToSpDistance, k32BitSize, true);
                }
                aarchCGFunc.SelectAdd(fpOpnd, spOpnd, *immOpnd, PTY_u64);
                cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
                if (cgFunc.GenCfi()) {
                    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(
                        stackBaseReg,
                        static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize() - fpToSpDistance,
                        k64BitSize));
                }
            } else {
                aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
                cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
                if (cgFunc.GenCfi()) {
                    cgFunc.GetCurBB()->AppendInsn(
                        cgFunc.GetInsnBuilder()
                            ->BuildCfiInsn(cfi::OP_CFI_def_cfa_register)
                            .AddOpndChain(aarchCGFunc.CreateCfiRegOperand(stackBaseReg, k64BitSize)));
                }
            }
        }
    }
    GeneratePushUnnamedVarargRegs();
    if (currCG->DoCheckSOE()) {
        AppendInstructionStackCheck(R16, kRegTyInt, kSoeChckOffset);
    }
    bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
    cgFunc.SetCurBB(*formerCurBB);
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(false);
}

void AArch64GenProEpilog::GenerateRet(BB &bb)
{
    bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_xret));
}

/*
 * If all the preds of exitBB made the TailcallOpt(replace blr/bl with br/b), return true, we don't create ret insn.
 * Otherwise, return false, create the ret insn.
 */
bool AArch64GenProEpilog::TestPredsOfRetBB(const BB &exitBB)
{
    AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    if (cgFunc.GetMirModule().IsCModule() &&
        (cgFunc.GetFunction().GetAttr(FUNCATTR_varargs) || ml->GetSizeOfLocals() > 0 || cgFunc.HasVLAOrAlloca())) {
        return false;
    }
    for (auto tmpBB : exitBB.GetPreds()) {
        Insn *firstInsn = tmpBB->GetFirstInsn();
        if ((firstInsn == nullptr || tmpBB->IsCommentBB()) && (!tmpBB->GetPreds().empty())) {
            if (!TestPredsOfRetBB(*tmpBB)) {
                return false;
            }
        } else {
            Insn *lastInsn = tmpBB->GetLastInsn();
            if (lastInsn == nullptr) {
                return false;
            }
            MOperator insnMop = lastInsn->GetMachineOpcode();
            if (insnMop != MOP_tail_call_opt_xbl && insnMop != MOP_tail_call_opt_xblr) {
                return false;
            }
        }
    }
    return true;
}

void AArch64GenProEpilog::AppendInstructionPopSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopSingle];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, GetPointerSize() * kBitsPerByte, rty);
    Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerSize() * kBitsPerByte);
    MemOperand *aarchMemO1 = static_cast<MemOperand *>(o1);
    uint32 dataSize = GetPointerSize() * kBitsPerByte;
    if (aarchMemO1->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
        o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R9);
    }

    Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o1);
    popInsn.SetComment("RESTORE");
    cgFunc.GetCurBB()->AppendInsn(popInsn);

    /* Append CFI code. */
    if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(reg, k64BitSize));
    }
}

void AArch64GenProEpilog::AppendInstructionPopPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty,
                                                   int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
    Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerSize() * kBitsPerByte);

    uint32 dataSize = GetPointerSize() * kBitsPerByte;
    CHECK_FATAL(offset >= 0, "offset must >= 0");
    if (offset > kStpLdpImm64UpperBound) {
        o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc, static_cast<MemOperand &>(*o2), dataSize, R16);
    }
    Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
    popInsn.SetComment("RESTORE RESTORE");
    cgFunc.GetCurBB()->AppendInsn(popInsn);

    /* Append CFI code */
    if (cgFunc.GenCfi() && !CGOptions::IsNoCalleeCFI()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(reg0, k64BitSize));
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(reg1, k64BitSize));
    }
}

void AArch64GenProEpilog::AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    /*
     * ldp/stp's imm should be within -512 and 504;
     * if ldp's imm > 504, we fall back to the ldp-add version
     */
    bool useLdpAdd = false;
    int32 offset = 0;

    Operand *o2 = nullptr;
    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), GetPointerSize() * kBitsPerByte);
    } else {
        if (stackFrameSize > kStpLdpImm64UpperBound) {
            useLdpAdd = true;
            offset = kOffset16MemPos;
            stackFrameSize -= offset;
        } else {
            offset = stackFrameSize;
        }
        o2 = &aarchCGFunc.CreateCallFrameOperand(offset, GetPointerSize() * kBitsPerByte);
    }

    if (useLdpAdd) {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
        if (cgFunc.GenCfi()) {
            int64 cfiOffset = GetOffsetFromCFA();
            BB *curBB = cgFunc.GetCurBB();
            curBB->InsertInsnAfter(*(curBB->GetLastInsn()),
                                   aarchCGFunc.CreateCfiDefCfaInsn(RSP, cfiOffset - stackFrameSize, k64BitSize));
        }
    }

    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        CHECK_FATAL(!useLdpAdd, "Invalid assumption");
        if (fpToSpDistance > kStpLdpImm64UpperBound) {
            (void)AppendInstructionForAllocateOrDeallocateCallFrame(fpToSpDistance, reg0, reg1, rty, false);
        } else {
            Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
        }
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    } else {
        Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
        cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }

    if (cgFunc.GenCfi()) {
        /* Append CFI restore */
        if (useFP) {
            cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
        }
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
    }
}

void AArch64GenProEpilog::AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerSize() * kBitsPerByte, rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerSize() * kBitsPerByte, rty);
    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    /*
     * ldp/stp's imm should be within -512 and 504;
     * if ldp's imm > 504, we fall back to the ldp-add version
     */
    bool isLmbc = (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc);
    if (cgFunc.HasVLAOrAlloca() || fpToSpDistance == 0 || isLmbc) {
        int lmbcOffset = 0;
        if (!isLmbc) {
            stackFrameSize -= fpToSpDistance;
        } else {
            lmbcOffset = fpToSpDistance - (kDivide2 * k8ByteSize);
        }
        if (stackFrameSize > kStpLdpImm64UpperBound || isLmbc) {
            Operand *o2;
            o2 = aarchCGFunc.CreateStackMemOpnd(RSP, (isLmbc ? lmbcOffset : 0), GetPointerSize() * kBitsPerByte);
            Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
            if (cgFunc.GenCfi()) {
                /* Append CFI restore */
                if (useFP) {
                    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
                }
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
            }
            Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
            Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
            aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
        } else {
            MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(stackFrameSize, GetPointerSize() * kBitsPerByte);
            Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
            if (cgFunc.GenCfi()) {
                if (useFP) {
                    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
                }
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
            }
        }
    } else {
        Operand *o2;
        o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), GetPointerSize() * kBitsPerByte);
        if (fpToSpDistance > kStpLdpImm64UpperBound) {
            (void)AppendInstructionForAllocateOrDeallocateCallFrame(fpToSpDistance, reg0, reg1, rty, false);
        } else {
            Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
        }

        if (cgFunc.GenCfi()) {
            if (useFP) {
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
            }
            cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
        }
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    }
}

void AArch64GenProEpilog::GeneratePopRegs()
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();

    const MapleVector<AArch64reg> &regsToRestore =
        (!CGOptions::DoRegSavesOpt()) ? aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();

    CHECK_FATAL(!regsToRestore.empty(), "FP/LR not added to callee-saved list?");

    AArch64reg intRegFirstHalf = kRinvalid;
    AArch64reg fpRegFirstHalf = kRinvalid;

    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("restore callee-saved registers"));
    }

    MapleVector<AArch64reg>::const_iterator it = regsToRestore.begin();
    /*
     * Even if we don't use FP, since we push a pair of registers
     * in a single instruction (i.e., stp) and the stack needs be aligned
     * on a 16-byte boundary, push FP as well if the function has a call.
     * Make sure this is reflected when computing calleeSavedRegs.size()
     * skip the first two registers
     */
    CHECK_FATAL(*it == RFP, "The first callee saved reg is expected to be RFP");
    ++it;
    CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
    ++it;

    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    int32 offset;
    if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        offset = static_cast<int32>((memLayout->RealStackFrameSize() - aarchCGFunc.SizeOfCalleeSaved()) -
                                    memLayout->GetSizeOfLocals());
    } else {
        offset = (static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize() -
                  (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kIntregBytelen))) - /* for FP/LR */
                 memLayout->SizeOfArgsToStackPass() -
                 cgFunc.GetFunction().GetFrameReseverdSlot();
    }

    if (cgFunc.GetCG()->IsStackProtectorStrong() || cgFunc.GetCG()->IsStackProtectorAll()) {
        offset -= kAarch64StackPtrAlignment;
    }

    if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
        /* GR/VR save areas are above the callee save area */
        AArch64MemLayout *ml = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
        auto saveareasize = static_cast<int32>(RoundUp(ml->GetSizeOfGRSaveArea(), GetPointerSize() * k2BitSize) +
                                               RoundUp(ml->GetSizeOfVRSaveArea(), GetPointerSize() * k2BitSize));
        offset -= saveareasize;
    }

    /*
     * We are using a cleared dummy block; so insertPoint cannot be ret;
     * see GenerateEpilog()
     */
    for (; it != regsToRestore.end(); ++it) {
        AArch64reg reg = *it;
        CHECK_FATAL(reg != RFP, "stray RFP in callee_saved_list?");
        CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");

        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
        if (firstHalf == kRinvalid) {
            /* remember it */
            firstHalf = reg;
        } else {
            /* flush the pair */
            AppendInstructionPopPair(cgFunc, firstHalf, reg, regType, offset);
            GetNextOffsetCalleeSaved(offset);
            firstHalf = kRinvalid;
        }
    }

    if (intRegFirstHalf != kRinvalid) {
        AppendInstructionPopSingle(cgFunc, intRegFirstHalf, kRegTyInt, offset);
        GetNextOffsetCalleeSaved(offset);
    }

    if (fpRegFirstHalf != kRinvalid) {
        AppendInstructionPopSingle(cgFunc, fpRegFirstHalf, kRegTyFloat, offset);
        GetNextOffsetCalleeSaved(offset);
    }

    if (!currCG->GenerateDebugFriendlyCode()) {
        AppendInstructionDeallocateCallFrame(R29, RLR, kRegTyInt);
    } else {
        AppendInstructionDeallocateCallFrameDebug(R29, RLR, kRegTyInt);
    }

    if (cgFunc.GenCfi()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(RSP, 0, k64BitSize));
    }
    /*
     * in case we split stp/ldp instructions,
     * so that we generate a load-into-base-register instruction
     * for the next function, maybe? (seems not necessary, but...)
     */
    aarchCGFunc.SetSplitBaseOffset(0);
}

void AArch64GenProEpilog::AppendJump(const MIRSymbol &funcSymbol)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    Operand &targetOpnd = aarchCGFunc.GetOrCreateFuncNameOpnd(funcSymbol);
    cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd));
}

void AArch64GenProEpilog::GenerateEpilog(BB &bb)
{
    if (!cgFunc.GetHasProEpilogue()) {
        if (bb.GetPreds().empty() || !TestPredsOfRetBB(bb)) {
            GenerateRet(bb);
        }
        return;
    }

    /* generate stack protected instruction */
    BB &epilogBB = GenStackGuardCheckInsn(bb);

    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    BB *formerCurBB = cgFunc.GetCurBB();
    aarchCGFunc.GetDummyBB()->ClearInsns();
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(true);
    cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);

    if (cgFunc.HasVLAOrAlloca() && cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
        aarchCGFunc.SelectCopy(spOpnd, PTY_u64, fpOpnd, PTY_u64);
    }

    /* Hack: exit bb should always be reachable, since we need its existance for ".cfi_remember_state" */
    if (&epilogBB != cgFunc.GetLastBB() && epilogBB.GetNext() != nullptr) {
        BB *nextBB = epilogBB.GetNext();
        do {
            if (nextBB == cgFunc.GetLastBB() || !nextBB->IsEmpty()) {
                break;
            }
            nextBB = nextBB->GetNext();
        } while (nextBB != nullptr);
        if (nextBB != nullptr && !nextBB->IsEmpty() && cgFunc.GenCfi()) {
            cgFunc.GetCurBB()->AppendInsn(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_remember_state));
            cgFunc.GetCurBB()->SetHasCfi();
            nextBB->InsertInsnBefore(*nextBB->GetFirstInsn(),
                                     cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_restore_state));
            nextBB->SetHasCfi();
        }
    }

    const MapleVector<AArch64reg> &regsToSave =
        (!CGOptions::DoRegSavesOpt()) ? aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
    if (!regsToSave.empty()) {
        GeneratePopRegs();
    } else {
        auto stackFrameSize = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize();
        if (stackFrameSize > 0) {
            if (currCG->GenerateVerboseCG()) {
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("pop up activation frame"));
            }

            if (cgFunc.HasVLAOrAlloca()) {
                auto size = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->GetSegArgsToStkPass().GetSize();
                stackFrameSize = stackFrameSize < size ? 0 : stackFrameSize - size;
            }

            if (stackFrameSize > 0) {
                Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
                aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
                if (cgFunc.GenCfi()) {
                    cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCfiDefCfaInsn(RSP, 0, k64BitSize));
                }
            }
        }
    }

    if (currCG->InstrumentWithDebugTraceCall()) {
        AppendJump(*(currCG->GetDebugTraceExitFunction()));
    }

    GenerateRet(*(cgFunc.GetCurBB()));
    epilogBB.AppendBBInsns(*cgFunc.GetCurBB());
    if (cgFunc.GetCurBB()->GetHasCfi()) {
        epilogBB.SetHasCfi();
    }

    cgFunc.SetCurBB(*formerCurBB);
    aarchCGFunc.GetDummyBB()->SetIsProEpilog(false);
}

void AArch64GenProEpilog::GenerateEpilogForCleanup(BB &bb)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CHECK_FATAL(!cgFunc.GetExitBBsVec().empty(), "exit bb size is zero!");
    if (cgFunc.GetExitBB(0)->IsUnreachable()) {
        /* if exitbb is unreachable then exitbb can not be generated */
        GenerateEpilog(bb);
    } else if (aarchCGFunc.NeedCleanup()) { /* bl to the exit epilogue */
        LabelOperand &targetOpnd = aarchCGFunc.GetOrCreateLabelOperand(cgFunc.GetExitBB(0)->GetLabIdx());
        bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd));
    }
}

void AArch64GenProEpilog::ConvertToTailCalls(MapleSet<Insn *> &callInsnsMap)
{
    BB *exitBB = GetCurTailcallExitBB();

    /* ExitBB is filled only by now. If exitBB has restore of SP indicating extra stack space has
       been allocated, such as a function call with more than 8 args, argument with large aggr etc */
    FOR_BB_INSNS(insn, exitBB) {
        if (insn->GetMachineOpcode() == MOP_xaddrri12 || insn->GetMachineOpcode() == MOP_xaddrri24) {
            RegOperand &reg = static_cast<RegOperand &>(insn->GetOperand(0));
            if (reg.GetRegisterNumber() == RSP) {
                return;
            }
        }
    }

    /* Replace all of the call insns. */
    for (Insn *callInsn : callInsnsMap) {
        MOperator insnMop = callInsn->GetMachineOpcode();
        switch (insnMop) {
            case MOP_xbl: {
                callInsn->SetMOP(AArch64CG::kMd[MOP_tail_call_opt_xbl]);
                break;
            }
            case MOP_xblr: {
                callInsn->SetMOP(AArch64CG::kMd[MOP_tail_call_opt_xblr]);
                break;
            }
            default:
                CHECK_FATAL(false, "Internal error.");
                break;
        }
        BB *bb = callInsn->GetBB();
        if (bb->GetKind() == BB::kBBGoto) {
            bb->SetKind(BB::kBBFallthru);
            if (bb->GetLastInsn()->GetMachineOpcode() == MOP_xuncond) {
                bb->RemoveInsn(*bb->GetLastInsn());
            }
        }
        for (auto sBB : bb->GetSuccs()) {
            bb->RemoveSuccs(*sBB);
            sBB->RemovePreds(*bb);
            break;
        }
    }

    /* copy instrs from exit block */
    for (Insn *callInsn : callInsnsMap) {
        BB *toBB = callInsn->GetBB();
        BB *fromBB = exitBB;
        if (toBB == fromBB) {
            /* callsite also in the return exit block, just change the return to branch */
            Insn *lastInsn = toBB->GetLastInsn();
            if (lastInsn->GetMachineOpcode() == MOP_xret) {
                Insn *newInsn = cgFunc.GetTheCFG()->CloneInsn(*callInsn);
                toBB->ReplaceInsn(*lastInsn, *newInsn);
                for (Insn *insn = callInsn->GetNextMachineInsn(); insn != newInsn; insn = insn->GetNextMachineInsn()) {
                    insn->SetDoNotRemove(true);
                }
                toBB->RemoveInsn(*callInsn);
                return;
            }
            CHECK_FATAL(0, "Tailcall in incorrect block");
        }
        FOR_BB_INSNS_SAFE(insn, fromBB, next) {
            if (insn->IsCfiInsn() || (insn->IsMachineInstruction() && insn->GetMachineOpcode() != MOP_xret)) {
                Insn *newInsn = cgFunc.GetTheCFG()->CloneInsn(*insn);
                newInsn->SetDoNotRemove(true);
                toBB->InsertInsnBefore(*callInsn, *newInsn);
            }
        }
    }

    /* remove instrs in exit block */
    BB *bb = exitBB;
    if (bb->GetPreds().size() > 0) {
        return; /* exit block still needed by other non-tailcall blocks */
    }
    Insn &junk = cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_pseudo_none);
    bb->AppendInsn(junk);
    FOR_BB_INSNS_SAFE(insn, bb, next) {
        if (insn->GetMachineOpcode() != MOP_pseudo_none) {
            bb->RemoveInsn(*insn);
        }
    }
}

void AArch64GenProEpilog::Run()
{
    CHECK_FATAL(cgFunc.GetFunction().GetBody()->GetFirst()->GetOpCode() == OP_label,
                "The first statement should be a label");
    NeedStackProtect();
    cgFunc.SetHasProEpilogue(NeedProEpilog());
    if (cgFunc.GetHasProEpilogue()) {
        GenStackGuard(*(cgFunc.GetFirstBB()));
    }
    BB *proLog = nullptr;
    if (cgFunc.GetCG()->DoPrologueEpilogue() && Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel2) {
        /* There are some O2 dependent assumptions made */
        proLog = IsolateFastPath(*(cgFunc.GetFirstBB()));
    }

    if (cgFunc.IsExitBBsVecEmpty()) {
        if (cgFunc.GetLastBB()->GetPrev()->GetFirstStmt() == cgFunc.GetCleanupLabel() &&
            cgFunc.GetLastBB()->GetPrev()->GetPrev()) {
            cgFunc.PushBackExitBBsVec(*cgFunc.GetLastBB()->GetPrev()->GetPrev());
        } else {
            cgFunc.PushBackExitBBsVec(*cgFunc.GetLastBB()->GetPrev());
        }
    }

    if (proLog != nullptr) {
        GenerateProlog(*proLog);
        proLog->SetFastPath(true);
        cgFunc.GetFirstBB()->SetFastPath(true);
    } else {
        GenerateProlog(*(cgFunc.GetFirstBB()));
    }

    for (auto *exitBB : cgFunc.GetExitBBsVec()) {
        if (GetFastPathReturnBB() != exitBB) {
            GenerateEpilog(*exitBB);
        }
    }

    if (cgFunc.GetFunction().IsJava()) {
        GenerateEpilogForCleanup(*(cgFunc.GetCleanupBB()));
    }

    if (cgFunc.GetMirModule().IsCModule() && !exitBB2CallSitesMap.empty()) {
        cgFunc.GetTheCFG()->InitInsnVisitor(cgFunc);
        for (auto pair : exitBB2CallSitesMap) {
            BB *curExitBB = pair.first;
            MapleSet<Insn *> &callInsnsMap = pair.second;
            SetCurTailcallExitBB(curExitBB);
            ConvertToTailCalls(callInsnsMap);
        }
    }
}
} /* namespace maplebe */
