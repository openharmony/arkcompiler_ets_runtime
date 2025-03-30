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

#include "cg_cfg.h"
#if TARGAARCH64
#include "aarch64_insn.h"
#endif
#if TARGX86_64
#include "x64_cgfunc.h"
#include "cg.h"
#endif

namespace maplebe {
void CGCFG::BuildCFG()
{
    if (cgFunc->HasBuiltCfg()) {
        return;
    }
    /*
     * Second Pass:
     * Link preds/succs in the BBs
     */
    BB *firstBB = cgFunc->GetFirstBB();
    for (BB *curBB = firstBB; curBB != nullptr; curBB = curBB->GetNext()) {
        BB::BBKind kind = curBB->GetKind();
        switch (kind) {
            case BB::kBBIf: {
                BB *fallthruBB = curBB->GetNext();
                curBB->PushBackSuccs(*fallthruBB);
                fallthruBB->PushBackPreds(*curBB);
                Insn *branchInsn = curBB->GetLastMachineInsn();
                CHECK_FATAL(branchInsn != nullptr, "machine instruction must be exist in ifBB");
                DEBUG_ASSERT(branchInsn->IsCondBranch(), "must be a conditional branch generated from an intrinsic");
                /* Assume the last non-null operand is the branch target */
                uint32 opSz = branchInsn->GetOperandSize();
                DEBUG_ASSERT(opSz >= 1, "lastOpndIndex's opnd is greater than 1");
                Operand &lastOpnd = branchInsn->GetOperand(opSz - 1u);
                DEBUG_ASSERT(lastOpnd.IsLabelOpnd(), "label Operand must be exist in branch insn");
                auto &labelOpnd = static_cast<LabelOperand &>(lastOpnd);
                BB *brToBB = cgFunc->GetBBFromLab2BBMap(labelOpnd.GetLabelIndex());
                if (fallthruBB->GetId() != brToBB->GetId()) {
                    curBB->PushBackSuccs(*brToBB);
                    brToBB->PushBackPreds(*curBB);
                }
                break;
            }
            case BB::kBBGoto: {
                Insn *insn = curBB->GetLastMachineInsn();
                if (insn == nullptr) {
                    curBB->SetKind(BB::kBBFallthru);
                    continue;
                }
                CHECK_FATAL(insn != nullptr, "machine insn must be exist in gotoBB");
                DEBUG_ASSERT(insn->IsUnCondBranch(), "insn must be a unconditional branch insn");
                LabelIdx labelIdx = static_cast<LabelOperand &>(insn->GetOperand(0)).GetLabelIndex();
                BB *gotoBB = cgFunc->GetBBFromLab2BBMap(labelIdx);
                CHECK_FATAL(gotoBB != nullptr, "gotoBB is null");
                curBB->PushBackSuccs(*gotoBB);
                gotoBB->PushBackPreds(*curBB);
                break;
            }
            case BB::kBBRangeGoto: {
                std::set<BB *, BBIdCmp> bbs;
                for (auto labelIdx : curBB->GetRangeGotoLabelVec()) {
                    BB *gotoBB = cgFunc->GetBBFromLab2BBMap(labelIdx);
                    bbs.insert(gotoBB);
                }
                for (auto gotoBB : bbs) {
                    curBB->PushBackSuccs(*gotoBB);
                    gotoBB->PushBackPreds(*curBB);
                }
                break;
            }
            case BB::kBBFallthru: {
                BB *fallthruBB = curBB->GetNext();
                if (fallthruBB != nullptr) {
                    curBB->PushBackSuccs(*fallthruBB);
                    fallthruBB->PushBackPreds(*curBB);
                }
                break;
            }
            default:
                break;
        } /* end switch */
    }
    FindAndMarkUnreachable(*cgFunc);
    cgFunc->SetHasBuiltCfg(true);
}

void CGCFG::CheckCFG()
{
    FOR_ALL_BB(bb, cgFunc)
    {
        for (BB *sucBB : bb->GetSuccs()) {
            bool found = false;
            for (BB *sucPred : sucBB->GetPreds()) {
                if (sucPred == bb) {
                    if (!found) {
                        found = true;
                    } else {
                        LogInfo::MapleLogger()
                            << "dup pred " << sucPred->GetId() << " for sucBB " << sucBB->GetId() << "\n";
                        CHECK_FATAL_FALSE("CG_CFG check failed !");
                    }
                }
            }
            if (!found) {
                LogInfo::MapleLogger() << "non pred for sucBB " << sucBB->GetId() << " for BB " << bb->GetId() << "\n";
                CHECK_FATAL_FALSE("CG_CFG check failed !");
            }
        }
    }
    FOR_ALL_BB(bb, cgFunc)
    {
        for (BB *predBB : bb->GetPreds()) {
            bool found = false;
            for (BB *predSucc : predBB->GetSuccs()) {
                if (predSucc == bb) {
                    if (!found) {
                        found = true;
                    } else {
                        LogInfo::MapleLogger()
                            << "dup succ " << predSucc->GetId() << " for predBB " << predBB->GetId() << "\n";
                        CHECK_FATAL_FALSE("CG_CFG check failed !");
                    }
                }
            }
            if (!found) {
                LogInfo::MapleLogger() << "non succ for predBB " << predBB->GetId() << " for BB " << bb->GetId()
                                       << "\n";
                CHECK_FATAL_FALSE("CG_CFG check failed !");
            }
        }
    }
}

InsnVisitor *CGCFG::insnVisitor;

void CGCFG::InitInsnVisitor(CGFunc &func) const
{
    insnVisitor = func.NewInsnModifier();
}

/*
 * return true if:
 * mergee has only one predecessor which is merger,
 * or mergee has other comments only predecessors & merger is soloGoto
 * mergee can't have cfi instruction when postcfgo.
 */
bool CGCFG::BBJudge(const BB &first, const BB &second) const
{
    if (first.GetKind() == BB::kBBReturn || second.GetKind() == BB::kBBReturn) {
        return false;
    }
    if (&first == &second) {
        return false;
    }
    if (second.GetPreds().size() == 1 && second.GetPreds().front() == &first) {
        return true;
    }
    for (BB *bb : second.GetPreds()) {
        if (bb != &first && !AreCommentAllPreds(*bb)) {
            return false;
        }
    }
    return first.IsSoloGoto();
}

/*
 * Check if a given BB mergee can be merged into BB merger.
 * Returns true if:
 * 1. mergee has only one predecessor which is merger, or mergee has
 *   other comments only predecessors.
 * 2. merge has only one successor which is mergee.
 * 3. mergee can't have cfi instruction when postcfgo.
 */
bool CGCFG::CanMerge(const BB &merger, const BB &mergee) const
{
    if (!BBJudge(merger, mergee)) {
        return false;
    }
    if (mergee.GetFirstInsn() != nullptr && mergee.GetFirstInsn()->IsCfiInsn()) {
        return false;
    }
    return (merger.GetSuccs().size() == 1) && (merger.GetSuccs().front() == &mergee);
}

/* Check if the given BB contains only comments and all its predecessors are comments */
bool CGCFG::AreCommentAllPreds(const BB &bb)
{
    if (!bb.IsCommentBB()) {
        return false;
    }
    for (BB *pred : bb.GetPreds()) {
        if (!AreCommentAllPreds(*pred)) {
            return false;
        }
    }
    return true;
}

/* Merge sucBB into curBB. */
void CGCFG::MergeBB(BB &merger, BB &mergee, CGFunc &func)
{
    BB *prevLast = mergee.GetPrev();
    MergeBB(merger, mergee);
    if (func.GetLastBB()->GetId() == mergee.GetId()) {
        func.SetLastBB(*prevLast);
    }
    if (mergee.GetKind() == BB::kBBReturn) {
        auto retIt = func.GetExitBBsVec().begin();
        while (retIt != func.GetExitBBsVec().end()) {
            if (*retIt == &mergee) {
                (void)func.EraseExitBBsVec(retIt);
                break;
            } else {
                ++retIt;
            }
        }
        func.PushBackExitBBsVec(merger);
    }
    if (mergee.GetKind() == BB::kBBNoReturn) {
        auto noRetIt = func.GetNoRetCallBBVec().begin();
        while (noRetIt != func.GetNoRetCallBBVec().end()) {
            if (*noRetIt == &mergee) {
                (void)func.EraseNoReturnCallBB(noRetIt);
                break;
            } else {
                ++noRetIt;
            }
        }
        func.PushBackNoReturnCallBBsVec(merger);
    }
    if (mergee.GetKind() == BB::kBBRangeGoto) {
        func.AddEmitSt(merger.GetId(), *func.GetEmitSt(mergee.GetId()));
        func.DeleteEmitSt(mergee.GetId());
    }
}

void CGCFG::MergeBB(BB &merger, BB &mergee)
{
    if (merger.GetKind() == BB::kBBGoto && merger.GetLastMachineInsn() != nullptr) {
        if (!merger.GetLastMachineInsn()->IsBranch()) {
            CHECK_FATAL(false, "unexpected insn kind");
        }
        merger.RemoveInsn(*merger.GetLastMachineInsn());
    }
    merger.AppendBBInsns(mergee);
    if (mergee.GetPrev() != nullptr) {
        mergee.GetPrev()->SetNext(mergee.GetNext());
    }
    if (mergee.GetNext() != nullptr) {
        mergee.GetNext()->SetPrev(mergee.GetPrev());
    }
    merger.RemoveSuccs(mergee);
    for (BB *bb : mergee.GetSuccs()) {
        bb->RemovePreds(mergee);
        bb->PushBackPreds(merger);
        merger.PushBackSuccs(*bb);
    }
    merger.SetKind(mergee.GetKind());
    mergee.SetNext(nullptr);
    mergee.SetPrev(nullptr);
    mergee.ClearPreds();
    mergee.ClearSuccs();
    mergee.SetFirstInsn(nullptr);
    mergee.SetLastInsn(nullptr);
}

/*
 * Find all reachable BBs by dfs in cgfunc and mark their field<unreachable> false, then all other bbs should be
 * unreachable.
 */
void CGCFG::FindAndMarkUnreachable(CGFunc &func)
{
    BB *firstBB = func.GetFirstBB();
    std::stack<BB *> toBeAnalyzedBBs;
    toBeAnalyzedBBs.push(firstBB);
    std::unordered_set<uint32> instackBBs;

    BB *bb = firstBB;
    /* set all bb's unreacable to true */
    while (bb != nullptr) {
        /* Check if bb is the first or the last BB of the function */
        if (InSwitchTable(bb->GetLabIdx(), func) || bb == func.GetFirstBB() || bb == func.GetLastBB()) {
            toBeAnalyzedBBs.push(bb);
        } else if (bb->IsLabelTaken() == false) {
            bb->SetUnreachable(true);
        }
        bb = bb->GetNext();
    }

    /* do a dfs to see which bbs are reachable */
    while (!toBeAnalyzedBBs.empty()) {
        bb = toBeAnalyzedBBs.top();
        toBeAnalyzedBBs.pop();
        (void)instackBBs.insert(bb->GetId());

        bb->SetUnreachable(false);

        for (BB *succBB : bb->GetSuccs()) {
            if (instackBBs.count(succBB->GetId()) == 0) {
                toBeAnalyzedBBs.push(succBB);
                (void)instackBBs.insert(succBB->GetId());
            }
        }
    }
    FOR_ALL_BB(tmpBB, &func)
    {
        for (MapleList<BB *>::iterator predIt = tmpBB->GetPredsBegin(); predIt != tmpBB->GetPredsEnd(); ++predIt) {
            if ((*predIt)->IsUnreachable()) {
                tmpBB->ErasePreds(predIt);
            }
        }
    }
}

/*
 * Theoretically, every time you remove from a bb's preds, you should consider invoking this method.
 *
 * @param bb
 * @param func
 */
void CGCFG::FlushUnReachableStatusAndRemoveRelations(BB &bb, const CGFunc &func) const
{
    /* Check if bb is the first or the last BB of the function */
    bool isFirstBBInfunc = (&bb == func.GetFirstBB());
    bool isLastBBInfunc = (&bb == func.GetLastBB());
    if (InSwitchTable(bb.GetLabIdx(), func) || isFirstBBInfunc || isLastBBInfunc) {
        return;
    }
    std::stack<BB *> toBeAnalyzedBBs;
    toBeAnalyzedBBs.push(&bb);
    std::set<uint32> instackBBs;
    BB *it = nullptr;
    while (!toBeAnalyzedBBs.empty()) {
        it = toBeAnalyzedBBs.top();
        (void)instackBBs.insert(it->GetId());
        toBeAnalyzedBBs.pop();
        /* Check if bb is the first or the last BB of the function */
        isFirstBBInfunc = (it == func.GetFirstBB());
        isLastBBInfunc = (it == func.GetLastBB());
        bool needFlush = !isFirstBBInfunc && !isLastBBInfunc && it->GetPreds().empty() &&
                         !InSwitchTable(it->GetLabIdx(), *cgFunc) && !cgFunc->IsExitBB(*it) &&
                         (it->IsLabelTaken() == false);
        if (!needFlush) {
            continue;
        }
        it->SetUnreachable(true);
        it->SetFirstInsn(nullptr);
        it->SetLastInsn(nullptr);
        for (BB *succ : it->GetSuccs()) {
            if (instackBBs.count(succ->GetId()) == 0) {
                toBeAnalyzedBBs.push(succ);
                (void)instackBBs.insert(succ->GetId());
            }
            succ->RemovePreds(*it);
        }
        it->ClearSuccs();
    }
}

void CGCFG::RemoveBB(BB &curBB, bool isGotoIf) const
{
    if (!curBB.IsUnreachable()) {
        BB *sucBB = CGCFG::GetTargetSuc(curBB, false, isGotoIf);
        if (sucBB != nullptr) {
            sucBB->RemovePreds(curBB);
        }
        BB *fallthruSuc = nullptr;
        if (isGotoIf) {
            for (BB *succ : curBB.GetSuccs()) {
                if (succ == sucBB) {
                    continue;
                }
                fallthruSuc = succ;
                break;
            }
            DEBUG_ASSERT(fallthruSuc == curBB.GetNext(), "fallthru succ should be its next bb.");
            if (fallthruSuc != nullptr) {
                fallthruSuc->RemovePreds(curBB);
            }
        }

        for (BB *preBB : curBB.GetPreds()) {
            /*
             * If curBB is the target of its predecessor, change
             * the jump target.
             */
            if (&curBB == GetTargetSuc(*preBB, true, isGotoIf)) {
                LabelIdx targetLabel;
                if (curBB.GetNext()->GetLabIdx() == 0) {
                    targetLabel = insnVisitor->GetCGFunc()->CreateLabel();
                    curBB.GetNext()->SetLabIdx(targetLabel);
                    cgFunc->SetLab2BBMap(targetLabel, *curBB.GetNext());
                } else {
                    targetLabel = curBB.GetNext()->GetLabIdx();
                }
                insnVisitor->ModifyJumpTarget(targetLabel, *preBB);
            }
            if (fallthruSuc != nullptr && !fallthruSuc->IsPredecessor(*preBB)) {
                preBB->PushBackSuccs(*fallthruSuc);
                fallthruSuc->PushBackPreds(*preBB);
            }
            if (sucBB != nullptr && !sucBB->IsPredecessor(*preBB)) {
                preBB->PushBackSuccs(*sucBB);
                sucBB->PushBackPreds(*preBB);
            }
            preBB->RemoveSuccs(curBB);
        }
    }

    if (curBB.GetNext() != nullptr) {
        cgFunc->GetCommonExitBB()->RemovePreds(curBB);
        curBB.GetNext()->RemovePreds(curBB);
        curBB.GetNext()->SetPrev(curBB.GetPrev());
    } else {
        cgFunc->SetLastBB(*curBB.GetPrev());
    }
    if (curBB.GetPrev() != nullptr) {
        curBB.GetPrev()->SetNext(curBB.GetNext());
    } else {
        cgFunc->SetFirstBB(*curBB.GetNext());
    }
    cgFunc->ClearBBInVec(curBB.GetId());
}

void CGCFG::UpdateCommonExitBBInfo()
{
    BB *commonExitBB = cgFunc->GetCommonExitBB();
    ASSERT_NOT_NULL(commonExitBB);
    commonExitBB->ClearPreds();
    for (BB *exitBB : cgFunc->GetExitBBsVec()) {
        if (!exitBB->IsUnreachable()) {
            commonExitBB->PushBackPreds(*exitBB);
        }
    }
    for (BB *noRetBB : cgFunc->GetNoRetCallBBVec()) {
        if (!noRetBB->IsUnreachable()) {
            commonExitBB->PushBackPreds(*noRetBB);
        }
    }
    WontExitAnalysis();
}

BB *CGCFG::GetTargetSuc(BB &curBB, bool branchOnly, bool isGotoIf)
{
    switch (curBB.GetKind()) {
        case BB::kBBGoto:
        case BB::kBBIf: {
            const Insn *origLastInsn = curBB.GetLastMachineInsn();
            ASSERT_NOT_NULL(origLastInsn);
            if (isGotoIf && (curBB.GetPrev() != nullptr) &&
                (curBB.GetKind() == BB::kBBGoto || curBB.GetKind() == BB::kBBIf) &&
                (curBB.GetPrev()->GetKind() == BB::kBBGoto || curBB.GetPrev()->GetKind() == BB::kBBIf)) {
                origLastInsn = curBB.GetPrev()->GetLastMachineInsn();
            }
            ASSERT_NOT_NULL(origLastInsn);
            LabelIdx label = insnVisitor->GetJumpLabel(*origLastInsn);
            for (BB *bb : curBB.GetSuccs()) {
                if (bb->GetLabIdx() == label) {
                    return bb;
                }
            }
            break;
        }
        case BB::kBBFallthru: {
            return (branchOnly ? nullptr : curBB.GetNext());
        }
        default:
            return nullptr;
    }
    return nullptr;
}

bool CGCFG::InSwitchTable(LabelIdx label, const CGFunc &func)
{
    if (label == 0) {
        return false;
    }
    return func.InSwitchTable(label);
}

/*
 * analyse the CFG to find the BBs that are not reachable from function entries
 * and delete them
 */
void CGCFG::UnreachCodeAnalysis() const
{
    BB *firstBB = cgFunc->GetFirstBB();
    std::forward_list<BB *> toBeAnalyzedBBs;
    toBeAnalyzedBBs.push_front(firstBB);
    std::set<BB *, BBIdCmp> unreachBBs;

    BB *bb = firstBB;
    /* set all bb's unreacable to true */
    while (bb != nullptr) {
        /* Check if bb is the firstBB/cleanupBB/returnBB/lastBB of the function */
        if (InSwitchTable(bb->GetLabIdx(), *cgFunc) || bb == cgFunc->GetFirstBB() ||
            bb == cgFunc->GetLastBB() || bb->GetKind() == BB::kBBReturn) {
            toBeAnalyzedBBs.push_front(bb);
        } else {
            (void)unreachBBs.insert(bb);
        }
        if (bb->IsLabelTaken() == false) {
            bb->SetUnreachable(true);
        }
        bb = bb->GetNext();
    }

    /* do a dfs to see which bbs are reachable */
    while (!toBeAnalyzedBBs.empty()) {
        bb = toBeAnalyzedBBs.front();
        toBeAnalyzedBBs.pop_front();
        if (!bb->IsUnreachable()) {
            continue;
        }
        bb->SetUnreachable(false);
        for (BB *succBB : bb->GetSuccs()) {
            toBeAnalyzedBBs.push_front(succBB);
            unreachBBs.erase(succBB);
        }
    }

    /* remove unreachable bb */
    std::set<BB *, BBIdCmp>::iterator it;
    for (it = unreachBBs.begin(); it != unreachBBs.end(); it++) {
        BB *unreachBB = *it;
        DEBUG_ASSERT(unreachBB != nullptr, "unreachBB must not be nullptr");
        for (auto exitBB = cgFunc->GetExitBBsVec().begin(); exitBB != cgFunc->GetExitBBsVec().end(); ++exitBB) {
            if (*exitBB == unreachBB) {
                cgFunc->GetExitBBsVec().erase(exitBB);
                break;
            }
        }
        unreachBB->GetPrev()->SetNext(unreachBB->GetNext());
        cgFunc->GetCommonExitBB()->RemovePreds(*unreachBB);
        if (unreachBB->GetNext()) {
            unreachBB->GetNext()->SetPrev(unreachBB->GetPrev());
        }

        for (BB *sucBB : unreachBB->GetSuccs()) {
            sucBB->RemovePreds(*unreachBB);
        }

        unreachBB->ClearSuccs();

        cgFunc->ClearBBInVec(unreachBB->GetId());
    }
}

void CGCFG::FindWillExitBBs(BB *bb, std::set<BB *, BBIdCmp> *visitedBBs)
{
    std::queue<BB *> allBBs;
    allBBs.push(bb);
    while (!allBBs.empty()) {
        BB *curBB = allBBs.front();
        allBBs.pop();
        if (visitedBBs->count(curBB) != 0) {
            continue;
        }
        visitedBBs->insert(curBB);
        for (auto *predBB : curBB->GetPreds()) {
            allBBs.push(predBB);
        }
    }
}

/*
 * analyse the CFG to find the BBs that will not reach any function exit; these
 * are BBs inside infinite loops; mark their wontExit flag and create
 * artificial edges from them to commonExitBB
 */
void CGCFG::WontExitAnalysis()
{
    std::set<BB *, BBIdCmp> visitedBBs;
    FindWillExitBBs(cgFunc->GetCommonExitBB(), &visitedBBs);
    BB *bb = cgFunc->GetFirstBB();
    while (bb != nullptr) {
        if (visitedBBs.count(bb) == 0) {
            bb->SetWontExit(true);
            if (bb->GetKind() == BB::kBBGoto) {
                // make this bb a predecessor of commonExitBB
                cgFunc->GetCommonExitBB()->PushBackPreds(*bb);
            }
        }
        bb = bb->GetNext();
    }
}

void CgHandleCFG::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.SetPreservedAll();
}

bool CgHandleCFG::PhaseRun(maplebe::CGFunc &f)
{
    CGCFG *cfg = f.GetMemoryPool()->New<CGCFG>(f);
    f.SetTheCFG(cfg);
    /* build control flow graph */
    f.GetTheCFG()->BuildCFG();
    f.HandleFuncCfg(cfg);

    /* analysis unreachable code */
    f.GetTheCFG()->UnreachCodeAnalysis();
    f.EraseUnreachableStackMapInsns();
    return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgHandleCFG, handlecfg)

} /* namespace maplebe */
