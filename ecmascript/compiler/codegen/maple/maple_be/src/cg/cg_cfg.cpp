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
#if defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_insn.h"
#endif
#if TARGARM32
#include "arm32_insn.h"
#endif
#include "cg_option.h"
#include "mpl_logging.h"
#if TARGX86_64
#include "x64_cgfunc.h"
#include "cg.h"
#endif
#include "triple.h"
#include <cstdlib>

namespace {
using namespace maplebe;
bool CanBBThrow(const BB &bb)
{
    FOR_BB_INSNS_CONST(insn, &bb)
    {
        if (insn->IsTargetInsn() && insn->CanThrow()) {
            return true;
        }
    }
    return false;
}
}  // namespace

namespace maplebe {
void CGCFG::BuildCFG()
{
    /*
     * Second Pass:
     * Link preds/succs in the BBs
     */
    BB *firstBB = cgFunc->GetFirstBB();
    for (BB *curBB = firstBB; curBB != nullptr; curBB = curBB->GetNext()) {
        BB::BBKind kind = curBB->GetKind();
        switch (kind) {
            case BB::kBBIntrinsic:
                /*
                 * An intrinsic BB append a MOP_wcbnz instruction at the end, check
                 * AArch64CGFunc::SelectIntrinsicCall(IntrinsiccallNode *intrinsiccallNode) for details
                 */
                CHECK_NULL_FATAL(curBB->GetLastMachineInsn());
                if (!curBB->GetLastMachineInsn()->IsBranch()) {
                    break;
                }
                /* else fall through */
                [[clang::fallthrough]];
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
            case BB::kBBIgoto: {
                for (auto lidx :
                     CG::GetCurCGFunc()->GetMirModule().CurFunction()->GetLabelTab()->GetAddrTakenLabels()) {
                    BB *igotobb = cgFunc->GetBBFromLab2BBMap(lidx);
                    CHECK_FATAL(igotobb, "igotobb is null");
                    curBB->PushBackSuccs(*igotobb);
                    igotobb->PushBackPreds(*curBB);
                }
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
            case BB::kBBThrow:
                break;
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

        EHFunc *ehFunc = cgFunc->GetEHFunc();
        /* Check exception table. If curBB is in a try block, add catch BB to its succs */
        if (ehFunc != nullptr && ehFunc->GetLSDACallSiteTable() != nullptr) {
            /* Determine if insn in bb can actually except */
            if (CanBBThrow(*curBB)) {
                const MapleVector<LSDACallSite *> &callsiteTable = ehFunc->GetLSDACallSiteTable()->GetCallSiteTable();
                for (size_t i = 0; i < callsiteTable.size(); ++i) {
                    LSDACallSite *lsdaCallsite = callsiteTable[i];
                    BB *endTry = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLength.GetEndOffset()->GetLabelIdx());
                    BB *startTry = cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLength.GetStartOffset()->GetLabelIdx());
                    if (curBB->GetId() >= startTry->GetId() && curBB->GetId() <= endTry->GetId() &&
                        lsdaCallsite->csLandingPad.GetEndOffset() != nullptr) {
                        BB *landingPad =
                            cgFunc->GetBBFromLab2BBMap(lsdaCallsite->csLandingPad.GetEndOffset()->GetLabelIdx());
                        curBB->PushBackEhSuccs(*landingPad);
                        landingPad->PushBackEhPreds(*curBB);
                    }
                }
            }
        }
    }
    FindAndMarkUnreachable(*cgFunc);
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

void CGCFG::CheckCFGFreq()
{
    auto verifyBBFreq = [this](const BB *bb, uint32 succFreq) {
        uint32 res = bb->GetFrequency();
        if ((res != 0 && abs(static_cast<int>(res - succFreq)) / res > 1.0) || (res == 0 && res != succFreq)) {
            // Not included
            if (bb->GetSuccs().size() > 1 && bb->GetPreds().size() > 1) {
                return;
            }
            LogInfo::MapleLogger() << cgFunc->GetName() << " curBB: " << bb->GetId() << " freq: " << bb->GetFrequency()
                                   << std::endl;
            CHECK_FATAL(false, "Verifyfreq failure BB frequency!");
        }
    };
    FOR_ALL_BB(bb, cgFunc)
    {
        if (bb->IsUnreachable() || bb->IsCleanup()) {
            continue;
        }
        uint32 res = 0;
        if (bb->GetSuccs().size() > 1) {
            for (auto *succBB : bb->GetSuccs()) {
                res += succBB->GetFrequency();
                if (succBB->GetPreds().size() > 1) {
                    LogInfo::MapleLogger()
                        << cgFunc->GetName() << " critical edges: curBB: " << bb->GetId() << std::endl;
                    CHECK_FATAL(false, "The CFG has critical edges!");
                }
            }
            verifyBBFreq(bb, res);
        } else if (bb->GetSuccs().size() == 1) {
            auto *succBB = bb->GetSuccs().front();
            if (succBB->GetPreds().size() == 1) {
                verifyBBFreq(bb, succBB->GetFrequency());
            } else if (succBB->GetPreds().size() > 1) {
                for (auto *pred : succBB->GetPreds()) {
                    res += pred->GetFrequency();
                }
                verifyBBFreq(succBB, res);
            }
        }
    }
    LogInfo::MapleLogger() << "Check Frequency for " << cgFunc->GetName() << " success!\n";
}

InsnVisitor *CGCFG::insnVisitor;

void CGCFG::InitInsnVisitor(CGFunc &func) const
{
    insnVisitor = func.NewInsnModifier();
}

// Do custom deep copy
Insn *CGCFG::CloneInsn(Insn &originalInsn) const
{
    cgFunc->IncTotalNumberOfInstructions();
    return insnVisitor->CloneInsn(originalInsn);
}

RegOperand *CGCFG::CreateVregFromReg(const RegOperand &pReg) const
{
    return insnVisitor->CreateVregFromReg(pReg);
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
    // If the address of firstBB or secondBB is referenced by adrp_label insn,
    // it can not be merged
    if (first.IsAdrpLabel() || second.IsAdrpLabel()) {
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
    if (!merger.GetEhSuccs().empty()) {
#if DEBUG
        for (BB *bb : merger.GetEhSuccs()) {
            DEBUG_ASSERT((bb != &mergee), "CGCFG::MergeBB: Merging of EH bb");
        }
#endif
    }
    if (!mergee.GetEhSuccs().empty()) {
        for (BB *bb : mergee.GetEhSuccs()) {
            bb->RemoveEhPreds(mergee);
            bb->PushBackEhPreds(merger);
            merger.PushBackEhSuccs(*bb);
        }
    }
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
    mergee.ClearEhPreds();
    mergee.ClearEhSuccs();
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
        if (bb->IsCleanup() || InSwitchTable(bb->GetLabIdx(), func) || bb == func.GetFirstBB() ||
            bb == func.GetLastBB()) {
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
        for (BB *succBB : bb->GetEhSuccs()) {
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
        for (MapleList<BB *>::iterator predIt = tmpBB->GetEhPredsBegin(); predIt != tmpBB->GetEhPredsEnd(); ++predIt) {
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
    if (bb.IsCleanup() || InSwitchTable(bb.GetLabIdx(), func) || isFirstBBInfunc || isLastBBInfunc) {
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
        bool needFlush = !isFirstBBInfunc && !isLastBBInfunc && !it->IsCleanup() &&
                         (it->GetPreds().empty() || (it->GetPreds().size() == 1 && it->GetEhPreds().front() == it)) &&
                         it->GetEhPreds().empty() && !InSwitchTable(it->GetLabIdx(), *cgFunc) &&
                         !cgFunc->IsExitBB(*it) && (it->IsLabelTaken() == false);
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
            succ->RemoveEhPreds(*it);
        }
        it->ClearSuccs();
        for (BB *succ : it->GetEhSuccs()) {
            if (instackBBs.count(succ->GetId()) == 0) {
                toBeAnalyzedBBs.push(succ);
                (void)instackBBs.insert(succ->GetId());
            }
            succ->RemoveEhPreds(*it);
            succ->RemovePreds(*it);
        }
        it->ClearEhSuccs();
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
            if (preBB->GetKind() == BB::kBBIgoto) {
                if (sucBB != nullptr) {
                    sucBB->PushBackPreds(curBB);
                }
                return;
            }
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

    for (BB *ehSucc : curBB.GetEhSuccs()) {
        ehSucc->RemoveEhPreds(curBB);
    }
    for (BB *ehPred : curBB.GetEhPreds()) {
        ehPred->RemoveEhSuccs(curBB);
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
    /* remove callsite */
    EHFunc *ehFunc = cgFunc->GetEHFunc();
    /* only java try has ehFunc->GetLSDACallSiteTable */
    if (ehFunc != nullptr && ehFunc->GetLSDACallSiteTable() != nullptr) {
        ehFunc->GetLSDACallSiteTable()->RemoveCallSite(curBB);
    }
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

void CGCFG::RetargetJump(BB &srcBB, BB &targetBB) const
{
    insnVisitor->ModifyJumpTarget(srcBB, targetBB);
}

BB *CGCFG::GetTargetSuc(BB &curBB, bool branchOnly, bool isGotoIf)
{
    switch (curBB.GetKind()) {
        case BB::kBBGoto:
        case BB::kBBIntrinsic:
        case BB::kBBIf: {
            const Insn *origLastInsn = curBB.GetLastMachineInsn();
            ASSERT_NOT_NULL(origLastInsn);
            if (isGotoIf && (curBB.GetPrev() != nullptr) &&
                (curBB.GetKind() == BB::kBBGoto || curBB.GetKind() == BB::kBBIf) &&
                (curBB.GetPrev()->GetKind() == BB::kBBGoto || curBB.GetPrev()->GetKind() == BB::kBBIf)) {
                origLastInsn = curBB.GetPrev()->GetLastMachineInsn();
            }
            LabelIdx label = insnVisitor->GetJumpLabel(*origLastInsn);
            for (BB *bb : curBB.GetSuccs()) {
                if (bb->GetLabIdx() == label) {
                    return bb;
                }
            }
            break;
        }
        case BB::kBBIgoto: {
            for (Insn *insn = curBB.GetLastInsn(); insn != nullptr; insn = insn->GetPrev()) {
#if TARGAARCH64
                if (Triple::GetTriple().IsAarch64BeOrLe() &&
                    insn->GetMachineOpcode() == MOP_adrp_label) {
                    int64 label = static_cast<ImmOperand&>(insn->GetOperand(1)).GetValue();
                    for (BB *bb : curBB.GetSuccs()) {
                        if (bb->GetLabIdx() == static_cast<LabelIdx>(label)) {
                            return bb;
                        }
                    }
                }
#endif
            }
            /* can also be a MOP_xbr. */
            return nullptr;
        }
        case BB::kBBFallthru: {
            return (branchOnly ? nullptr : curBB.GetNext());
        }
        case BB::kBBThrow:
            return nullptr;
        default:
            return nullptr;
    }
    return nullptr;
}

bool CGCFG::InLSDA(LabelIdx label [[maybe_unused]], const EHFunc *ehFunc [[maybe_unused]])
{
    /* the function have no exception handle module */
    if (ehFunc == nullptr) {
        return false;
    }

    if ((label == 0) || ehFunc->GetLSDACallSiteTable() == nullptr) {
        return false;
    }
    if (label == ehFunc->GetLSDACallSiteTable()->GetCSTable().GetEndOffset()->GetLabelIdx() ||
        label == ehFunc->GetLSDACallSiteTable()->GetCSTable().GetStartOffset()->GetLabelIdx()) {
        return true;
    }
    return ehFunc->GetLSDACallSiteTable()->InCallSiteTable(label);
}

bool CGCFG::InSwitchTable(LabelIdx label, const CGFunc &func)
{
    if (label == 0) {
        return false;
    }
    return func.InSwitchTable(label);
}

bool CGCFG::IsCompareAndBranchInsn(const Insn &insn) const
{
    return insnVisitor->IsCompareAndBranchInsn(insn);
}

bool CGCFG::IsTestAndBranchInsn(const Insn &insn) const
{
    return insnVisitor->IsTestAndBranchInsn(insn);
}

bool CGCFG::IsAddOrSubInsn(const Insn &insn) const
{
    return insnVisitor->IsAddOrSubInsn(insn);
}

Insn *CGCFG::FindLastCondBrInsn(BB &bb) const
{
    if (bb.GetKind() != BB::kBBIf) {
        return nullptr;
    }
    FOR_BB_INSNS_REV(insn, (&bb))
    {
        if (insn->IsBranch()) {
            return insn;
        }
    }
    return nullptr;
}

void CGCFG::MarkLabelTakenBB() const
{
    if (cgFunc->GetMirModule().GetSrcLang() != kSrcLangC) {
        return;
    }
    for (BB *bb = cgFunc->GetFirstBB(); bb != nullptr; bb = bb->GetNext()) {
        if (cgFunc->GetFunction().GetLabelTab()->GetAddrTakenLabels().find(bb->GetLabIdx()) !=
            cgFunc->GetFunction().GetLabelTab()->GetAddrTakenLabels().end()) {
            cgFunc->SetHasTakenLabel();
            bb->SetLabelTaken();
        }
    }
}

/*
 * analyse the CFG to find the BBs that are not reachable from function entries
 * and delete them
 */
void CGCFG::UnreachCodeAnalysis() const
{
    if (cgFunc->GetMirModule().GetSrcLang() == kSrcLangC &&
        (cgFunc->HasTakenLabel() || (cgFunc->GetEHFunc() && cgFunc->GetEHFunc()->GetLSDAHeader()))) {
        return;
    }
    /*
     * Find all reachable BBs by dfs in cgfunc and mark their field<unreachable> false,
     * then all other bbs should be unreachable.
     */
    BB *firstBB = cgFunc->GetFirstBB();
    std::forward_list<BB *> toBeAnalyzedBBs;
    toBeAnalyzedBBs.push_front(firstBB);
    std::set<BB *, BBIdCmp> unreachBBs;

    BB *bb = firstBB;
    /* set all bb's unreacable to true */
    while (bb != nullptr) {
        /* Check if bb is the firstBB/cleanupBB/returnBB/lastBB of the function */
        if (bb->IsCleanup() || InSwitchTable(bb->GetLabIdx(), *cgFunc) || bb == cgFunc->GetFirstBB() ||
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
        for (BB *succBB : bb->GetEhSuccs()) {
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
        EHFunc *ehFunc = cgFunc->GetEHFunc();
        /* if unreachBB InLSDA ,replace unreachBB's label with nextReachableBB before remove it. */
        if (ehFunc != nullptr && ehFunc->NeedFullLSDA() && maplebe::CGCFG::InLSDA(unreachBB->GetLabIdx(), ehFunc)) {
            /* find next reachable BB */
            BB *nextReachableBB = nullptr;
            for (BB *curBB = unreachBB; curBB != nullptr; curBB = curBB->GetNext()) {
                if (!curBB->IsUnreachable()) {
                    nextReachableBB = curBB;
                    break;
                }
            }
            CHECK_FATAL(nextReachableBB != nullptr, "nextReachableBB not be nullptr");
            if (nextReachableBB->GetLabIdx() == 0) {
                LabelIdx labelIdx = cgFunc->CreateLabel();
                nextReachableBB->AddLabel(labelIdx);
                cgFunc->SetLab2BBMap(labelIdx, *nextReachableBB);
            }

            ehFunc->GetLSDACallSiteTable()->UpdateCallSite(*unreachBB, *nextReachableBB);
        }

        unreachBB->GetPrev()->SetNext(unreachBB->GetNext());
        cgFunc->GetCommonExitBB()->RemovePreds(*unreachBB);
        if (unreachBB->GetNext()) {
            unreachBB->GetNext()->SetPrev(unreachBB->GetPrev());
        }

        for (BB *sucBB : unreachBB->GetSuccs()) {
            sucBB->RemovePreds(*unreachBB);
        }
        for (BB *ehSucBB : unreachBB->GetEhSuccs()) {
            ehSucBB->RemoveEhPreds(*unreachBB);
        }

        unreachBB->ClearSuccs();
        unreachBB->ClearEhSuccs();

        cgFunc->ClearBBInVec(unreachBB->GetId());

        /* Clear insns in GOT Map. */
        cgFunc->ClearUnreachableGotInfos(*unreachBB);
        cgFunc->ClearUnreachableConstInfos(*unreachBB);
    }
}

void CGCFG::FindWillExitBBs(BB *bb, std::set<BB *, BBIdCmp> *visitedBBs)
{
    if (visitedBBs->count(bb) != 0) {
        return;
    }
    visitedBBs->insert(bb);
    for (BB *predbb : bb->GetPreds()) {
        FindWillExitBBs(predbb, visitedBBs);
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
            if (bb->GetKind() == BB::kBBGoto || bb->GetKind() == BB::kBBThrow) {
                // make this bb a predecessor of commonExitBB
                cgFunc->GetCommonExitBB()->PushBackPreds(*bb);
            }
        }
        bb = bb->GetNext();
    }
}

BB *CGCFG::FindLastRetBB()
{
    FOR_ALL_BB_REV(bb, cgFunc)
    {
        if (bb->GetKind() == BB::kBBReturn) {
            return bb;
        }
    }
    return nullptr;
}

void CGCFG::UpdatePredsSuccsAfterSplit(BB &pred, BB &succ, BB &newBB) const
{
    /* connext newBB -> succ */
    for (auto it = succ.GetPredsBegin(); it != succ.GetPredsEnd(); ++it) {
        if (*it == &pred) {
            auto origIt = it;
            succ.ErasePreds(it);
            if (origIt != succ.GetPredsBegin()) {
                --origIt;
                succ.InsertPred(origIt, newBB);
            } else {
                succ.PushFrontPreds(newBB);
            }
            break;
        }
    }
    newBB.PushBackSuccs(succ);

    /* connext pred -> newBB */
    for (auto it = pred.GetSuccsBegin(); it != pred.GetSuccsEnd(); ++it) {
        if (*it == &succ) {
            auto origIt = it;
            pred.EraseSuccs(it);
            if (origIt != succ.GetSuccsBegin()) {
                --origIt;
                pred.InsertSucc(origIt, newBB);
            } else {
                pred.PushFrontSuccs(newBB);
            }
            break;
        }
    }
    newBB.PushBackPreds(pred);

    /* maintain eh info */
    for (auto it = pred.GetEhSuccs().begin(); it != pred.GetEhSuccs().end(); ++it) {
        newBB.PushBackEhSuccs(**it);
    }
    for (auto it = pred.GetEhPredsBegin(); it != pred.GetEhPredsEnd(); ++it) {
        newBB.PushBackEhPreds(**it);
    }

    /* update phi */
    for (const auto phiInsnIt : succ.GetPhiInsns()) {
        auto &phiList = static_cast<PhiOperand &>(phiInsnIt.second->GetOperand(kInsnSecondOpnd));
        for (const auto phiOpndIt : phiList.GetOperands()) {
            uint32 fBBId = phiOpndIt.first;
            DEBUG_ASSERT(fBBId != 0, "GetFromBBID = 0");
            BB *predBB = cgFunc->GetBBFromID(fBBId);
            if (predBB == &pred) {
                phiList.UpdateOpnd(fBBId, newBB.GetId(), *phiOpndIt.second);
                break;
            }
        }
    }
}

#if TARGAARCH64
void CGCFG::BreakCriticalEdge(BB &pred, BB &succ) const
{
    LabelIdx newLblIdx = cgFunc->CreateLabel();
    BB *newBB = cgFunc->CreateNewBB(newLblIdx, false, BB::kBBGoto, pred.GetFrequency());
    newBB->SetCritical(true);
    bool isFallThru = pred.GetNext() == &succ;
    /* set prev, next */
    if (isFallThru) {
        BB *origNext = pred.GetNext();
        origNext->SetPrev(newBB);
        newBB->SetNext(origNext);
        pred.SetNext(newBB);
        newBB->SetPrev(&pred);
        newBB->SetKind(BB::kBBFallthru);
    } else {
        BB *exitBB = cgFunc->GetExitBBsVec().size() == 0 ? nullptr : cgFunc->GetExitBB(0);
        if (exitBB == nullptr || exitBB->IsUnreachable()) {
            if (cgFunc->GetLastBB()->IsUnreachable()) {
                // nowhere to connect the newBB, drop it
                cgFunc->ClearBBInVec(newBB->GetId());
                return;
            } else {
                cgFunc->GetLastBB()->PrependBB(*newBB);
            }
        } else {
            exitBB->AppendBB(*newBB);
            if (cgFunc->GetLastBB() == exitBB) {
                cgFunc->SetLastBB(*newBB);
            }
        }
        newBB->AppendInsn(
            cgFunc->GetInsnBuilder()->BuildInsn(MOP_xuncond, cgFunc->GetOrCreateLabelOperand(succ.GetLabIdx())));
    }

    /* update offset if succ is goto target */
    if (pred.GetKind() == BB::kBBIf) {
        Insn *brInsn = FindLastCondBrInsn(pred);
        DEBUG_ASSERT(brInsn != nullptr, "null ptr check");
        LabelOperand &brTarget = static_cast<LabelOperand &>(brInsn->GetOperand(AArch64isa::GetJumpTargetIdx(*brInsn)));
        if (brTarget.GetLabelIndex() == succ.GetLabIdx()) {
            brInsn->SetOperand(AArch64isa::GetJumpTargetIdx(*brInsn), cgFunc->GetOrCreateLabelOperand(newLblIdx));
        }
    } else if (pred.GetKind() == BB::kBBRangeGoto) {
        const MapleVector<LabelIdx> &labelVec = pred.GetRangeGotoLabelVec();
        for (size_t i = 0; i < labelVec.size(); ++i) {
            if (labelVec[i] == succ.GetLabIdx()) {
                /* single edge for multi jump target, so have to replace all. */
                pred.SetRangeGotoLabel(static_cast<uint32>(i), newLblIdx);
            }
        }
        cgFunc->UpdateEmitSt(pred, succ.GetLabIdx(), newLblIdx);
    } else {
        DEBUG_ASSERT(0, "unexpeced bb kind in BreakCriticalEdge");
    }

    /* update pred, succ */
    UpdatePredsSuccsAfterSplit(pred, succ, *newBB);
}
#endif

void CgHandleCFG::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.SetPreservedAll();
}

bool CgHandleCFG::PhaseRun(maplebe::CGFunc &f)
{
    CGCFG *cfg = f.GetMemoryPool()->New<CGCFG>(f);
    f.SetTheCFG(cfg);
    cfg->MarkLabelTakenBB();
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
