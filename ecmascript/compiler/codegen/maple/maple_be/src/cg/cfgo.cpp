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

#include "cfgo.h"
#include "cgbb.h"
#include "cg.h"
#include "loop.h"
#include "mpl_logging.h"

/*
 * This phase traverses all basic block of cgFunc and finds special
 * basic block patterns, like continuous fallthrough basic block, continuous
 * uncondition jump basic block, unreachable basic block and empty basic block,
 * then do basic mergering, basic block placement transformations,
 * unnecessary jumps elimination, and remove unreachable or empty basic block.
 * This optimization is done on control flow graph basis.
 */
namespace maplebe {
using namespace maple;

#define CFGO_DUMP_NEWPM CG_DEBUG_FUNC(f)

/* return true if to is put after from and there is no real insns between from and to, */
bool ChainingPattern::NoInsnBetween(const BB &from, const BB &to) const
{
    const BB *bb = nullptr;
    for (bb = from.GetNext(); bb != nullptr && bb != &to && bb != cgFunc->GetLastBB(); bb = bb->GetNext()) {
        if (!bb->IsEmptyOrCommentOnly() || bb->IsUnreachable() || bb->GetKind() != BB::kBBFallthru) {
            return false;
        }
    }
    return (bb == &to);
}

/* return true if insns in bb1 and bb2 are the same except the last goto insn. */
bool ChainingPattern::DoSameThing(const BB &bb1, const Insn &last1, const BB &bb2, const Insn &last2) const
{
    const Insn *insn1 = bb1.GetFirstMachineInsn();
    const Insn *insn2 = bb2.GetFirstMachineInsn();
    while (insn1 != nullptr && insn1 != last1.GetNextMachineInsn() && insn2 != nullptr &&
           insn2 != last2.GetNextMachineInsn()) {
        if (!insn1->IsMachineInstruction()) {
            insn1 = insn1->GetNext();
            continue;
        }
        if (!insn2->IsMachineInstruction()) {
            insn2 = insn2->GetNext();
            continue;
        }
        if (insn1->GetMachineOpcode() != insn2->GetMachineOpcode()) {
            return false;
        }
        uint32 opndNum = insn1->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &op1 = insn1->GetOperand(i);
            Operand &op2 = insn2->GetOperand(i);
            if (&op1 == &op2) {
                continue;
            }
            if (!op1.Equals(op2)) {
                return false;
            }
        }
        insn1 = insn1->GetNextMachineInsn();
        insn2 = insn2->GetNextMachineInsn();
    }
    return (insn1 == last1.GetNextMachineInsn() && insn2 == last2.GetNextMachineInsn());
}

/*
 * BB2 can be merged into BB1, if
 *   1. BB1's kind is fallthrough;
 *   2. BB2 has only one predecessor which is BB1 and BB2 is not the lastbb
 *   3. BB2 is neither catch BB nor switch case BB
 */
bool ChainingPattern::MergeFallthuBB(BB &curBB)
{
    BB *sucBB = curBB.GetNext();
    if (sucBB == nullptr || IsLabelInSwitchTable(sucBB->GetLabIdx()) ||
        !cgFunc->GetTheCFG()->CanMerge(curBB, *sucBB)) {
        return false;
    }
    if (curBB.IsAtomicBuiltInBB() || sucBB->IsAtomicBuiltInBB()) {
        return false;
    }
    Log(curBB.GetId());
    if (checkOnly) {
        return false;
    }
    if (sucBB == cgFunc->GetLastBB()) {
        cgFunc->SetLastBB(curBB);
    }
    cgFunc->GetTheCFG()->MergeBB(curBB, *sucBB, *cgFunc);
    keepPosition = true;
    return true;
}

bool ChainingPattern::MergeGotoBB(BB &curBB, BB &sucBB)
{
    Log(curBB.GetId());
    if (checkOnly) {
        return false;
    }
    cgFunc->GetTheCFG()->MergeBB(curBB, sucBB, *cgFunc);
    keepPosition = true;
    return true;
}

bool ChainingPattern::MoveSuccBBAsCurBBNext(BB &curBB, BB &sucBB)
{
    /*
     * without the judge below, there is
     * Assembler Error: CFI state restore without previous remember
     */
    if (sucBB.GetHasCfi() || (sucBB.GetFirstInsn() != nullptr && sucBB.GetFirstInsn()->IsCfiInsn())) {
        return false;
    }
    Log(curBB.GetId());
    if (checkOnly) {
        return false;
    }
    /* put sucBB as curBB's next. */
    DEBUG_ASSERT(sucBB.GetPrev() != nullptr, "the target of current goto BB will not be the first bb");
    sucBB.GetPrev()->SetNext(sucBB.GetNext());
    if (sucBB.GetNext() != nullptr) {
        sucBB.GetNext()->SetPrev(sucBB.GetPrev());
    }
    sucBB.SetNext(curBB.GetNext());
    DEBUG_ASSERT(curBB.GetNext() != nullptr, "current goto BB will not be the last bb");
    curBB.GetNext()->SetPrev(&sucBB);
    if (sucBB.GetId() == cgFunc->GetLastBB()->GetId()) {
        cgFunc->SetLastBB(*(sucBB.GetPrev()));
    } else if (curBB.GetId() == cgFunc->GetLastBB()->GetId()) {
        cgFunc->SetLastBB(sucBB);
    }
    sucBB.SetPrev(&curBB);
    curBB.SetNext(&sucBB);
    if (curBB.GetLastMachineInsn() != nullptr) {
        curBB.RemoveInsn(*curBB.GetLastMachineInsn());
    }
    curBB.SetKind(BB::kBBFallthru);
    return true;
}

bool ChainingPattern::RemoveGotoInsn(BB &curBB, BB &sucBB)
{
    Log(curBB.GetId());
    if (checkOnly) {
        return false;
    }
    if (&sucBB != curBB.GetNext()) {
        DEBUG_ASSERT(curBB.GetNext() != nullptr, "nullptr check");
        curBB.ReplaceSucc(sucBB, *curBB.GetNext());
        curBB.GetNext()->PushBackPreds(curBB);
        sucBB.RemovePreds(curBB);
    }
    if (curBB.GetLastMachineInsn() != nullptr) {
        curBB.RemoveInsn(*curBB.GetLastMachineInsn());
    }
    curBB.SetKind(BB::kBBFallthru);
    return true;
}

bool ChainingPattern::ClearCurBBAndResetTargetBB(BB &curBB, BB &sucBB)
{
    if (curBB.GetHasCfi() || (curBB.GetFirstInsn() != nullptr && curBB.GetFirstInsn()->IsCfiInsn())) {
        return false;
    }
    Insn *brInsn = nullptr;
    for (brInsn = curBB.GetLastMachineInsn(); brInsn != nullptr; brInsn = brInsn->GetPrev()) {
        if (brInsn->IsUnCondBranch()) {
            break;
        }
    }
    DEBUG_ASSERT(brInsn != nullptr, "goto BB has no branch");
    BB *newTarget = sucBB.GetPrev();
    DEBUG_ASSERT(newTarget != nullptr, "get prev bb failed in ChainingPattern::ClearCurBBAndResetTargetBB");
    Insn *last1 = newTarget->GetLastMachineInsn();
    if (newTarget->GetKind() == BB::kBBGoto) {
        Insn *br = nullptr;
        for (br = newTarget->GetLastMachineInsn();
             newTarget->GetFirstInsn() != nullptr && br != newTarget->GetFirstInsn()->GetPrev(); br = br->GetPrev()) {
            DEBUG_ASSERT(br != nullptr, "br should not be nullptr");
            if (br->IsUnCondBranch()) {
                break;
            }
        }
        DEBUG_ASSERT(br != nullptr, "goto BB has no branch");
        last1 = br->GetPreviousMachineInsn();
    }
    if (last1 == nullptr) {
        return false;
    }
    ASSERT_NOT_NULL(brInsn);
    if (brInsn->GetPreviousMachineInsn()) {
        if (!DoSameThing(*newTarget, *last1, curBB, *brInsn->GetPreviousMachineInsn())) {
            return false;
        }
    }

    Log(curBB.GetId());
    if (checkOnly) {
        return false;
    }

    LabelIdx tgtLabIdx = newTarget->GetLabIdx();
    if (newTarget->GetLabIdx() == MIRLabelTable::GetDummyLabel()) {
        tgtLabIdx = cgFunc->CreateLabel();
        newTarget->AddLabel(tgtLabIdx);
        cgFunc->SetLab2BBMap(tgtLabIdx, *newTarget);
    }
    LabelOperand &brTarget = cgFunc->GetOrCreateLabelOperand(tgtLabIdx);
    brInsn->SetOperand(0, brTarget);
    curBB.RemoveInsnSequence(*curBB.GetFirstInsn(), *brInsn->GetPrev());

    curBB.RemoveFromSuccessorList(sucBB);
    curBB.PushBackSuccs(*newTarget);
    sucBB.RemoveFromPredecessorList(curBB);
    newTarget->PushBackPreds(curBB);

    sucBB.GetPrev()->SetUnreachable(false);
    keepPosition = true;
    return true;
}

/*
 * Following optimizations are performed:
 * 1. Basic block merging
 * 2. unnecessary jumps elimination
 * 3. Remove duplicates Basic block.
 */
bool ChainingPattern::Optimize(BB &curBB)
{
    if (curBB.IsUnreachable()) {
        return false;
    }

    if (curBB.GetKind() == BB::kBBFallthru) {
        return MergeFallthuBB(curBB);
    }

    if (curBB.GetKind() == BB::kBBGoto && !curBB.IsEmpty()) {
        Insn *last = curBB.GetLastMachineInsn();
        if (last->IsTailCall()) {
            return false;
        }

        BB *sucBB = CGCFG::GetTargetSuc(curBB);
        /*
         * BB2 can be merged into BB1, if
         *   1. BB1 ends with a goto;
         *   2. BB2 has only one predecessor which is BB1
         *   3. BB2 is of goto kind. Otherwise, the original fall through will be broken
         *   4. BB2 is neither catch BB nor switch case BB
         */
        if (sucBB == nullptr) {
            return false;
        }
        if (sucBB->GetKind() == BB::kBBGoto && !IsLabelInSwitchTable(sucBB->GetLabIdx()) &&
            cgFunc->GetTheCFG()->CanMerge(curBB, *sucBB)) {
            // BB9(curBB)   BB1
            //  \           /
            //   BB5(sucBB, gotoBB)
            // for this case, should not merge BB5, BB9
            if (sucBB->GetPreds().size() > 1) {
                return false;
            }
            return MergeGotoBB(curBB, *sucBB);
        } else if (sucBB != &curBB && curBB.GetNext() != sucBB && sucBB != cgFunc->GetLastBB() &&
                   !sucBB->IsPredecessor(*sucBB->GetPrev()) &&
                   !(sucBB->GetNext() != nullptr && sucBB->GetNext()->IsPredecessor(*sucBB)) &&
                   !IsLabelInSwitchTable(sucBB->GetLabIdx()) && curBB.GetNext() != nullptr) {
            return MoveSuccBBAsCurBBNext(curBB, *sucBB);
        }
        /*
         * Last goto instruction can be removed, if:
         *  1. The goto target is physically the next one to current BB.
         */
        else if (sucBB == curBB.GetNext() ||
                 (NoInsnBetween(curBB, *sucBB) && !IsLabelInSwitchTable(curBB.GetNext()->GetLabIdx()))) {
            return RemoveGotoInsn(curBB, *sucBB);
        }
        /*
         * Clear curBB and target it to sucBB->GetPrev()
         *  if sucBB->GetPrev() and curBB's insns are the same.
         *
         * curBB:           curBB:
         *   insn_x0          b prevbb
         *   b sucBB        ...
         * ...         ==>  prevbb:
         * prevbb:            insn_x0
         *   insn_x0        sucBB:
         * sucBB:
         */
        else if (sucBB != curBB.GetNext() && !curBB.IsSoloGoto() && !IsLabelInSwitchTable(curBB.GetLabIdx()) &&
                 sucBB->GetKind() == BB::kBBReturn && sucBB->GetPreds().size() > 1 && sucBB->GetPrev() != nullptr &&
                 sucBB->IsPredecessor(*sucBB->GetPrev()) &&
                 (sucBB->GetPrev()->GetKind() == BB::kBBFallthru || sucBB->GetPrev()->GetKind() == BB::kBBGoto)) {
            return ClearCurBBAndResetTargetBB(curBB, *sucBB);
        }
    }
    return false;
}

/*
 * 1. relocate goto BB
 * Found pattern             (1) ftBB->GetPreds().size() == 1
 * curBB:                      curBB: cond1_br target
 *       ...            ==>    brBB:
 *       cond_br brBB           ...
 * ftBB:                       targetBB: (ftBB,targetBB)
 *       goto target         (2) ftBB->GetPreds().size() > 1
 * brBB:                       curBB : cond1_br ftBB
 *       ...                   brBB:
 * targetBB                      ...
 *                            ftBB
 *                            targetBB
 *
 * 2. loopHeaderBB:              loopHeaderBB:
 *      ...                        ...
 *      cond_br loopExit:          cond_br loopHeaderBB
 *    ftBB:                      ftBB:
 *      goto loopHeaderBB:         goto loopExit
 */
bool FlipBRPattern::Optimize(BB &curBB)
{
    if (curBB.IsUnreachable()) {
        return false;
    }
    if (curBB.GetKind() == BB::kBBIf && !curBB.IsEmpty()) {
        BB *ftBB = curBB.GetNext();
        DEBUG_ASSERT(ftBB != nullptr, "ftBB is null in  FlipBRPattern::Optimize");
        BB *brBB = CGCFG::GetTargetSuc(curBB);
        DEBUG_ASSERT(brBB != nullptr, "brBB is null in  FlipBRPattern::Optimize");
        /* Check if it can be optimized */
        if (ftBB->GetKind() == BB::kBBGoto && ftBB->GetNext() == brBB) {
            Insn *curBBBranchInsn = nullptr;
            for (curBBBranchInsn = curBB.GetLastMachineInsn(); curBBBranchInsn != nullptr;
                 curBBBranchInsn = curBBBranchInsn->GetPrev()) {
                if (curBBBranchInsn->IsBranch()) {
                    break;
                }
            }
            DEBUG_ASSERT(curBBBranchInsn != nullptr, "FlipBRPattern: curBB has no branch");
            Insn *brInsn = nullptr;
            for (brInsn = ftBB->GetLastMachineInsn(); brInsn != nullptr; brInsn = brInsn->GetPrev()) {
                if (brInsn->IsUnCondBranch()) {
                    break;
                }
            }
            DEBUG_ASSERT(brInsn != nullptr, "FlipBRPattern: ftBB has no branch");

            /* Reverse the branch */
            ASSERT_NOT_NULL(curBBBranchInsn);
            uint32 targetIdx = GetJumpTargetIdx(*curBBBranchInsn);
            MOperator mOp = FlipConditionOp(curBBBranchInsn->GetMachineOpcode());
            if (mOp == 0) {
                return false;
            }
            auto it = ftBB->GetSuccsBegin();
            BB *tgtBB = *it;
            if (ftBB->GetPreds().size() == 1 &&
                (ftBB->IsSoloGoto() ||
                 (!IsLabelInSwitchTable(tgtBB->GetLabIdx()) && cgFunc->GetTheCFG()->CanMerge(*ftBB, *tgtBB)))) {
                curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
                ASSERT_NOT_NULL(brInsn);
                Operand &brTarget = brInsn->GetOperand(GetJumpTargetIdx(*brInsn));
                curBBBranchInsn->SetOperand(targetIdx, brTarget);
                /* Insert ftBB's insn at the beginning of tgtBB. */
                if (!ftBB->IsSoloGoto()) {
                    ftBB->RemoveInsn(*brInsn);
                    tgtBB->InsertAtBeginning(*ftBB);
                }
                /* Patch pred and succ lists */
                ftBB->EraseSuccs(it);
                ftBB->PushBackSuccs(*brBB);
                it = curBB.GetSuccsBegin();
                CHECK_FATAL(*it != nullptr, "nullptr check");
                if (*it == brBB) {
                    curBB.ReplaceSucc(it, *tgtBB);
                } else {
                    ++it;
                    curBB.ReplaceSucc(it, *tgtBB);
                }
                for (it = tgtBB->GetPredsBegin(); it != tgtBB->GetPredsEnd(); ++it) {
                    if (*it == ftBB) {
                        tgtBB->ErasePreds(it);
                        break;
                    }
                }
                tgtBB->PushBackPreds(curBB);
                for (it = brBB->GetPredsBegin(); it != brBB->GetPredsEnd(); ++it) {
                    if (*it == &curBB) {
                        brBB->ErasePreds(it);
                        break;
                    }
                }
                brBB->PushFrontPreds(*ftBB);
                /* Remove instructions from ftBB so curBB falls thru to brBB */
                ftBB->SetFirstInsn(nullptr);
                ftBB->SetLastInsn(nullptr);
                ftBB->SetKind(BB::kBBFallthru);
            } else if (!IsLabelInSwitchTable(ftBB->GetLabIdx()) && !tgtBB->IsPredecessor(*tgtBB->GetPrev())) {
                curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
                LabelIdx tgtLabIdx = ftBB->GetLabIdx();
                if (ftBB->GetLabIdx() == MIRLabelTable::GetDummyLabel()) {
                    tgtLabIdx = cgFunc->CreateLabel();
                    ftBB->AddLabel(tgtLabIdx);
                    cgFunc->SetLab2BBMap(tgtLabIdx, *ftBB);
                }
                LabelOperand &brTarget = cgFunc->GetOrCreateLabelOperand(tgtLabIdx);
                curBBBranchInsn->SetOperand(targetIdx, brTarget);
                curBB.SetNext(brBB);
                brBB->SetPrev(&curBB);
                ftBB->SetPrev(tgtBB->GetPrev());
                tgtBB->GetPrev()->SetNext(ftBB);
                ftBB->SetNext(tgtBB);
                tgtBB->SetPrev(ftBB);

                ftBB->RemoveInsn(*brInsn);
                ftBB->SetKind(BB::kBBFallthru);
            }
        } else if (GetPhase() == kCfgoPostRegAlloc && ftBB->GetKind() == BB::kBBGoto &&
                   loopInfo.GetBBLoopParent(curBB.GetId()) != nullptr &&
                   loopInfo.GetBBLoopParent(curBB.GetId()) == loopInfo.GetBBLoopParent(ftBB->GetId()) &&
                   ftBB->IsSoloGoto() &&
                   &(loopInfo.GetBBLoopParent(ftBB->GetId())->GetHeader()) == *(ftBB->GetSuccsBegin()) &&
                   !loopInfo.GetBBLoopParent(curBB.GetId())->Has(
                       (curBB.GetSuccs().front() == ftBB) ? *curBB.GetSuccs().back() : *curBB.GetSuccs().front())) {
            Insn *curBBBranchInsn = nullptr;
            for (curBBBranchInsn = curBB.GetLastMachineInsn(); curBBBranchInsn != nullptr;
                 curBBBranchInsn = curBBBranchInsn->GetPrev()) {
                if (curBBBranchInsn->IsBranch()) {
                    break;
                }
            }
            DEBUG_ASSERT(curBBBranchInsn != nullptr, "FlipBRPattern: curBB has no branch");
            Insn *brInsn = nullptr;
            for (brInsn = ftBB->GetLastMachineInsn(); brInsn != nullptr; brInsn = brInsn->GetPrev()) {
                if (brInsn->IsUnCondBranch()) {
                    break;
                }
            }
            DEBUG_ASSERT(brInsn != nullptr, "FlipBRPattern: ftBB has no branch");
            uint32 condTargetIdx = GetJumpTargetIdx(*curBBBranchInsn);
            LabelOperand &condTarget = static_cast<LabelOperand &>(curBBBranchInsn->GetOperand(condTargetIdx));
            MOperator mOp = FlipConditionOp(curBBBranchInsn->GetMachineOpcode());
            if (mOp == 0) {
                return false;
            }
            uint32 gotoTargetIdx = GetJumpTargetIdx(*brInsn);
            LabelOperand &gotoTarget = static_cast<LabelOperand &>(brInsn->GetOperand(gotoTargetIdx));
            curBBBranchInsn->SetMOP(cgFunc->GetCG()->GetTargetMd(mOp));
            curBBBranchInsn->SetOperand(condTargetIdx, gotoTarget);
            brInsn->SetOperand(gotoTargetIdx, condTarget);
            auto it = ftBB->GetSuccsBegin();
            BB *loopHeadBB = *it;
            curBB.ReplaceSucc(*brBB, *loopHeadBB);
            brBB->RemovePreds(curBB);
            ftBB->ReplaceSucc(*loopHeadBB, *brBB);
            loopHeadBB->RemovePreds(*ftBB);

            loopHeadBB->PushBackPreds(curBB);
            brBB->PushBackPreds(*ftBB);
        }
    }
    return false;
}

/* remove a basic block that contains nothing */
bool EmptyBBPattern::Optimize(BB &curBB)
{
    // Can not remove the BB whose address is referenced by adrp_label insn
    if (curBB.IsUnreachable()) {
        return false;
    }
    /* Empty bb and it's not a cleanupBB/returnBB/lastBB/catchBB. */
    if (curBB.GetPrev() == nullptr || curBB.IsCleanup() || &curBB == cgFunc->GetLastBB() ||
        curBB.GetKind() == BB::kBBReturn || IsLabelInSwitchTable(curBB.GetLabIdx())) {
        return false;
    }
    if (curBB.GetFirstInsn() == nullptr && curBB.GetLastInsn() == nullptr) {
        // empty BB
        Log(curBB.GetId());
        if (checkOnly) {
            return false;
        }

        BB *sucBB = CGCFG::GetTargetSuc(curBB);
        if (sucBB == nullptr || sucBB->IsCleanup()) {
            return false;
        }
        cgFunc->GetTheCFG()->RemoveBB(curBB);
        /* removeBB may do nothing. since no need to repeat, always ret false here. */
        return false;
    } else if (!curBB.HasMachineInsn()) {
        // BB only has dbg insn
        Log(curBB.GetId());
        if (checkOnly) {
            return false;
        }
        BB *sucBB = CGCFG::GetTargetSuc(curBB);
        if (sucBB == nullptr || sucBB->IsCleanup()) {
            return false;
        }
        // For Now We try to sink first conservatively.
        // All dbg insns should not be dropped. Later hoist or copy case should be considered.
        if (curBB.NumSuccs() == 1) {
            BB *succBB = curBB.GetSuccs().front();
            succBB->InsertAtBeginning(curBB);
            cgFunc->GetTheCFG()->RemoveBB(curBB);
        }
        return false;
    }
    return false;
}

/*
 * remove unreachable BB
 * condition:
 *   1. unreachable BB can't have cfi instruction when postcfgo.
 */
bool UnreachBBPattern::Optimize(BB &curBB)
{
    if (curBB.IsUnreachable()) {
        Log(curBB.GetId());
        if (checkOnly) {
            return false;
        }
        /* if curBB in exitbbsvec,return false. */
        if (cgFunc->IsExitBB(curBB)) {
            // In C some bb follow noreturn calls should remain unreachable
            curBB.SetUnreachable(cgFunc->GetMirModule().GetSrcLang() == kSrcLangC);
            return false;
        }

        if (curBB.GetHasCfi() || (curBB.GetFirstInsn() != nullptr && curBB.GetFirstInsn()->IsCfiInsn())) {
            return false;
        }

        // Indicates whether the curBB is removed
        bool isRemoved = true;
        if (curBB.GetPrev() != nullptr) {
            curBB.GetPrev()->SetNext(curBB.GetNext());
        }
        if (curBB.GetNext() != nullptr) {
            curBB.GetNext()->SetPrev(curBB.GetPrev());
        } else {
            cgFunc->SetLastBB(*(curBB.GetPrev()));
        }
        if (cgFunc->GetFirstBB() == cgFunc->GetLastBB() && cgFunc->GetFirstBB() != nullptr) {
            isRemoved = false;
        }

        /* flush after remove; */
        for (BB *bb : curBB.GetSuccs()) {
            bb->RemovePreds(curBB);
            cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(*bb, *cgFunc);
        }
        curBB.ClearSuccs();
        // return always be false
        if (!isRemoved) {
            return false;
        }
    }
    return false;
}

void CgCfgo::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLoopAnalysis>();
}

bool CgCfgo::PhaseRun(maplebe::CGFunc &f)
{
    auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
    CFGOptimizer *cfgOptimizer = f.GetCG()->CreateCFGOptimizer(*GetPhaseMemPool(), f, *loopInfo);
    DEBUG_ASSERT(cfgOptimizer != nullptr, "nullptr check");
    if (f.IsAfterRegAlloc()) {
        cfgOptimizer->SetPhase(kCfgoPostRegAlloc);
    }
    const std::string &funcClass = f.GetFunction().GetBaseClassName();
    const std::string &funcName = f.GetFunction().GetBaseFuncName();
    const std::string &name = funcClass + funcName;
    if (CFGO_DUMP_NEWPM) {
        DotGenerator::GenerateDot("before-cfgo", f, f.GetMirModule());
    }
    cfgOptimizer->Run(name);
    if (CFGO_DUMP_NEWPM) {
        f.GetTheCFG()->CheckCFG();
        DotGenerator::GenerateDot("after-cfgo", f, f.GetMirModule());
    }
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCfgo, cfgo)
} /* namespace maplebe */
