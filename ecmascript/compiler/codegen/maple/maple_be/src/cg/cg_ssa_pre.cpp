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

#include "cgfunc.h"
#include "loop.h"
#include "cg_ssa_pre.h"

namespace maplebe {

// ================ Step 6: Code Motion ================
void SSAPre::CodeMotion()
{
    // pass 1 only doing insertion
    for (Occ *occ : allOccs) {
        if (occ->occTy != kAOccPhiOpnd) {
            continue;
        }
        PhiOpndOcc *phiOpndOcc = static_cast<PhiOpndOcc *>(occ);
        if (phiOpndOcc->insertHere) {
            DEBUG_ASSERT(loop->GetBBLoopParent(phiOpndOcc->cgbb->GetId()) == nullptr,
                         "cg_ssapre: save inserted inside loop");
            workCand->saveAtEntryBBs.insert(phiOpndOcc->cgbb->GetId());
        }
    }
    // pass 2 only doing deletion
    for (Occ *occ : realOccs) {
        if (occ->occTy != kAOccReal) {
            continue;
        }
        RealOcc *realOcc = static_cast<RealOcc *>(occ);
        if (!realOcc->redundant) {
            DEBUG_ASSERT(loop->GetBBLoopParent(realOcc->cgbb->GetId()) == nullptr,
                         "cg_ssapre: save in place inside loop");
            workCand->saveAtEntryBBs.insert(realOcc->cgbb->GetId());
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ output _______" << '\n';
        LogInfo::MapleLogger() << " saveAtEntryBBs: [";
        for (uint32 id : workCand->saveAtEntryBBs) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n\n";
    }
}

// ================ Step 5: Finalize ================
// for setting RealOcc's redundant flag and PhiOpndOcc's insertHere flag
void SSAPre::Finalize()
{
    std::vector<Occ *> availDefVec(classCount + 1, nullptr);
    // preorder traversal of dominator tree
    for (Occ *occ : allOccs) {
        size_t classId = static_cast<size_t>(occ->classId);
        switch (occ->occTy) {
            case kAOccPhi: {
                PhiOcc *phiOcc = static_cast<PhiOcc *>(occ);
                if (phiOcc->WillBeAvail()) {
                    availDefVec[classId] = phiOcc;
                }
                break;
            }
            case kAOccReal: {
                RealOcc *realOcc = static_cast<RealOcc *>(occ);
                if (availDefVec[classId] == nullptr || !availDefVec[classId]->IsDominate(dom, occ)) {
                    realOcc->redundant = false;
                    availDefVec[classId] = realOcc;
                } else {
                    realOcc->redundant = true;
                }
                break;
            }
            case kAOccPhiOpnd: {
                PhiOpndOcc *phiOpndOcc = static_cast<PhiOpndOcc *>(occ);
                const PhiOcc *phiOcc = phiOpndOcc->defPhiOcc;
                if (phiOcc->WillBeAvail()) {
                    if (phiOpndOcc->def == nullptr || (!phiOpndOcc->hasRealUse && phiOpndOcc->def->occTy == kAOccPhi &&
                                                       !static_cast<PhiOcc *>(phiOpndOcc->def)->WillBeAvail())) {
                        // insert a store
                        if (phiOpndOcc->cgbb->GetSuccs().size() != 1) {  // critical edge
                            workCand->saveAtProlog = true;
                            break;
                        }
                        phiOpndOcc->insertHere = true;
                    } else {
                        phiOpndOcc->def = availDefVec[classId];
                    }
                }
                break;
            }
            case kAOccExit:
                break;
            default:
                DEBUG_ASSERT(false, "Finalize: unexpected occ type");
                break;
        }
        if (workCand->saveAtProlog) {
            break;
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after finalize _______" << '\n';
        if (workCand->saveAtProlog) {
            LogInfo::MapleLogger() << "Giving up because of insertion at critical edge" << '\n';
            return;
        }
        for (Occ *occ : allOccs) {
            if (occ->occTy == kAOccReal) {
                RealOcc *realOcc = static_cast<RealOcc *>(occ);
                if (!realOcc->redundant) {
                    occ->Dump();
                    LogInfo::MapleLogger() << " non-redundant" << '\n';
                }
            } else if (occ->occTy == kAOccPhiOpnd) {
                PhiOpndOcc *phiOpndOcc = static_cast<PhiOpndOcc *>(occ);
                if (phiOpndOcc->insertHere) {
                    occ->Dump();
                    LogInfo::MapleLogger() << " insertHere" << '\n';
                }
            }
        }
    }
}

// ================ Step 4: WillBeAvail Computation ================

void SSAPre::ResetCanBeAvail(PhiOcc *phi) const
{
    phi->isCanBeAvail = false;
    // the following loop finds phi's uses and reset them
    for (PhiOcc *phiOcc : phiOccs) {
        for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
            if (phiOpndOcc->def != nullptr && phiOpndOcc->def == phi) {
                if (!phiOpndOcc->hasRealUse && !phiOcc->isDownsafe && phiOcc->isCanBeAvail) {
                    ResetCanBeAvail(phiOcc);
                }
            }
        }
    }
}

void SSAPre::ComputeCanBeAvail() const
{
    for (PhiOcc *phiOcc : phiOccs) {
        if (!phiOcc->isDownsafe && phiOcc->isCanBeAvail) {
            bool existNullUse = false;
            for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
                if (phiOpndOcc->def == nullptr) {
                    existNullUse = true;
                    break;
                }
            }
            if (existNullUse) {
                ResetCanBeAvail(phiOcc);
            }
        }
    }
}

void SSAPre::ResetLater(PhiOcc *phi) const
{
    phi->isLater = false;
    // the following loop finds phi's uses and reset them
    for (PhiOcc *phiOcc : phiOccs) {
        for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
            if (phiOpndOcc->def != nullptr && phiOpndOcc->def == phi) {
                if (phiOcc->isLater) {
                    ResetLater(phiOcc);
                }
            }
        }
    }
}

void SSAPre::ComputeLater() const
{
    for (PhiOcc *phiOcc : phiOccs) {
        phiOcc->isLater = phiOcc->isCanBeAvail;
    }
    for (PhiOcc *phiOcc : phiOccs) {
        if (phiOcc->isLater) {
            bool existNonNullUse = false;
            for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
                if (phiOpndOcc->def != nullptr && phiOpndOcc->hasRealUse) {
                    existNonNullUse = true;
                    break;
                }
            }
            if (existNonNullUse || phiOcc->speculativeDownsafe) {
                ResetLater(phiOcc);
            }
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after later computation _______" << '\n';
        for (PhiOcc *phiOcc : phiOccs) {
            phiOcc->Dump();
            if (phiOcc->isCanBeAvail) {
                LogInfo::MapleLogger() << " canbeAvail";
            }
            if (phiOcc->isLater) {
                LogInfo::MapleLogger() << " later";
            }
            if (phiOcc->isCanBeAvail && !phiOcc->isLater) {
                LogInfo::MapleLogger() << " will be Avail";
            }
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 3: Downsafe Computation ================
void SSAPre::ResetDownsafe(const PhiOpndOcc *phiOpnd) const
{
    if (phiOpnd->hasRealUse) {
        return;
    }
    Occ *defOcc = phiOpnd->def;
    if (defOcc == nullptr || defOcc->occTy != kAOccPhi) {
        return;
    }
    PhiOcc *defPhiOcc = static_cast<PhiOcc *>(defOcc);
    if (defPhiOcc->speculativeDownsafe) {
        return;
    }
    if (!defPhiOcc->isDownsafe) {
        return;
    }
    defPhiOcc->isDownsafe = false;
    for (PhiOpndOcc *phiOpndOcc : defPhiOcc->phiOpnds) {
        ResetDownsafe(phiOpndOcc);
    }
}

void SSAPre::ComputeDownsafe() const
{
    for (PhiOcc *phiOcc : phiOccs) {
        if (!phiOcc->isDownsafe) {
            // propagate not-Downsafe backward along use-def edges
            for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
                ResetDownsafe(phiOpndOcc);
            }
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after downsafe computation _______" << '\n';
        for (PhiOcc *phiOcc : phiOccs) {
            phiOcc->Dump();
            if (phiOcc->speculativeDownsafe) {
                LogInfo::MapleLogger() << " spec_downsafe /";
            }
            if (phiOcc->isDownsafe) {
                LogInfo::MapleLogger() << " downsafe";
            }
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 2: rename ================
static void PropagateSpeculativeDownsafe(const LoopAnalysis &loop, PhiOcc *phiOcc)
{
    if (phiOcc->speculativeDownsafe) {
        return;
    }
    phiOcc->isDownsafe = true;
    phiOcc->speculativeDownsafe = true;
    for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
        if (phiOpndOcc->def != nullptr && phiOpndOcc->def->occTy == kAOccPhi) {
            PhiOcc *nextPhiOcc = static_cast<PhiOcc *>(phiOpndOcc->def);
            if (loop.GetBBLoopParent(nextPhiOcc->cgbb->GetId()) != nullptr) {
                PropagateSpeculativeDownsafe(loop, nextPhiOcc);
            }
        }
    }
}

void SSAPre::Rename()
{
    std::stack<Occ *> occStack;
    classCount = 0;
    // iterate thru the occurrences in order of preorder traversal of dominator
    // tree
    for (Occ *occ : allOccs) {
        while (!occStack.empty() && !occStack.top()->IsDominate(dom, occ)) {
            occStack.pop();
        }
        switch (occ->occTy) {
            case kAOccExit:
                if (!occStack.empty()) {
                    Occ *topOcc = occStack.top();
                    if (topOcc->occTy == kAOccPhi) {
                        PhiOcc *phiTopOcc = static_cast<PhiOcc *>(topOcc);
                        if (!phiTopOcc->speculativeDownsafe) {
                            phiTopOcc->isDownsafe = false;
                        }
                    }
                }
                break;
            case kAOccPhi:
                // assign new class
                occ->classId = ++classCount;
                occStack.push(occ);
                break;
            case kAOccReal: {
                if (occStack.empty()) {
                    // assign new class
                    occ->classId = ++classCount;
                    occStack.push(occ);
                    break;
                }
                Occ *topOcc = occStack.top();
                occ->classId = topOcc->classId;
                if (topOcc->occTy == kAOccPhi) {
                    occStack.push(occ);
                    if (loop->GetBBLoopParent(occ->cgbb->GetId()) != nullptr) {
                        static_cast<PhiOcc *>(topOcc)->isDownsafe = true;
                        static_cast<PhiOcc *>(topOcc)->speculativeDownsafe = true;
                    }
                }
                break;
            }
            case kAOccPhiOpnd: {
                if (occStack.empty()) {
                    // leave classId as 0
                    break;
                }
                Occ *topOcc = occStack.top();
                occ->def = topOcc;
                occ->classId = topOcc->classId;
                if (topOcc->occTy == kAOccReal) {
                    static_cast<PhiOpndOcc *>(occ)->hasRealUse = true;
                }
                break;
            }
            default:
                DEBUG_ASSERT(false, "Rename: unexpected type of occurrence");
                break;
        }
    }
    // loop thru phiOccs to propagate speculativeDownsafe
    for (PhiOcc *phiOcc : phiOccs) {
        if (phiOcc->speculativeDownsafe) {
            for (PhiOpndOcc *phiOpndOcc : phiOcc->phiOpnds) {
                if (phiOpndOcc->def != nullptr && phiOpndOcc->def->occTy == kAOccPhi) {
                    PhiOcc *nextPhiOcc = static_cast<PhiOcc *>(phiOpndOcc->def);
                    if (loop->GetBBLoopParent(nextPhiOcc->cgbb->GetId()) != nullptr) {
                        PropagateSpeculativeDownsafe(*loop, nextPhiOcc);
                    }
                }
            }
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after rename _______" << '\n';
        for (Occ *occ : allOccs) {
            occ->Dump();
            if (occ->occTy == kAOccPhi) {
                PhiOcc *phiOcc = static_cast<PhiOcc *>(occ);
                if (phiOcc->speculativeDownsafe) {
                    LogInfo::MapleLogger() << " spec_downsafe /";
                }
            }
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 1: insert phis ================

// form pih occ based on the real occ in workCand->realOccs; result is
// stored in phiDfns
void SSAPre::FormPhis()
{
    for (Occ *occ : realOccs) {
        GetIterDomFrontier(occ->cgbb, &phiDfns);
    }
}

// form allOccs inclusive of real, phi, phiOpnd, exit occurrences;
// form phiOccs containing only the phis
void SSAPre::CreateSortedOccs()
{
    // form phiOpnd occs based on the preds of the phi occs; result is
    // stored in phiOpndDfns
    std::multiset<uint32> phiOpndDfns;
    for (uint32 dfn : phiDfns) {
        const BBId bbId = dom->GetDtPreOrderItem(dfn);
        BB *cgbb = cgFunc->GetAllBBs()[bbId];
        for (BB *pred : cgbb->GetPreds()) {
            (void)phiOpndDfns.insert(dom->GetDtDfnItem(pred->GetId()));
        }
    }
    std::unordered_map<BBId, std::forward_list<PhiOpndOcc *>> bb2PhiOpndMap;
    MapleVector<Occ *>::iterator realOccIt = realOccs.begin();
    MapleVector<ExitOcc *>::iterator exitOccIt = exitOccs.begin();
    MapleSet<uint32>::iterator phiDfnIt = phiDfns.begin();
    MapleSet<uint32>::iterator phiOpndDfnIt = phiOpndDfns.begin();
    Occ *nextRealOcc = nullptr;
    if (realOccIt != realOccs.end()) {
        nextRealOcc = *realOccIt;
    }
    ExitOcc *nextExitOcc = nullptr;
    if (exitOccIt != exitOccs.end()) {
        nextExitOcc = *exitOccIt;
    }
    PhiOcc *nextPhiOcc = nullptr;
    if (phiDfnIt != phiDfns.end()) {
        nextPhiOcc = preMp->New<PhiOcc>(cgFunc->GetAllBBs().at(dom->GetDtPreOrderItem(*phiDfnIt)), preAllocator);
    }
    PhiOpndOcc *nextPhiOpndOcc = nullptr;
    if (phiOpndDfnIt != phiOpndDfns.end()) {
        nextPhiOpndOcc = preMp->New<PhiOpndOcc>(cgFunc->GetAllBBs().at(dom->GetDtPreOrderItem(*phiOpndDfnIt)));
        auto it = bb2PhiOpndMap.find(dom->GetDtPreOrderItem(*phiOpndDfnIt));
        if (it == bb2PhiOpndMap.end()) {
            std::forward_list<PhiOpndOcc *> newlist = {nextPhiOpndOcc};
            bb2PhiOpndMap[dom->GetDtPreOrderItem(*phiOpndDfnIt)] = newlist;
        } else {
            it->second.push_front(nextPhiOpndOcc);
        }
    }
    Occ *pickedOcc = nullptr;  // the next picked occ in order of preorder traversal of dominator tree
    do {
        pickedOcc = nullptr;
        if (nextPhiOcc != nullptr) {
            pickedOcc = nextPhiOcc;
        }
        if (nextRealOcc != nullptr && (pickedOcc == nullptr || dom->GetDtDfnItem(nextRealOcc->cgbb->GetId()) <
                                                                   dom->GetDtDfnItem(pickedOcc->cgbb->GetId()))) {
            pickedOcc = nextRealOcc;
        }
        if (nextPhiOpndOcc != nullptr &&
            (pickedOcc == nullptr || *phiOpndDfnIt < dom->GetDtDfnItem(pickedOcc->cgbb->GetId()))) {
            pickedOcc = nextPhiOpndOcc;
        }
        if (nextExitOcc != nullptr && (pickedOcc == nullptr || dom->GetDtDfnItem(nextExitOcc->cgbb->GetId()) <
                                                                   dom->GetDtDfnItem(pickedOcc->cgbb->GetId()))) {
            pickedOcc = nextExitOcc;
        }
        if (pickedOcc != nullptr) {
            allOccs.push_back(pickedOcc);
            switch (pickedOcc->occTy) {
                case kAOccReal: {
                    // get the next real occ
                    CHECK_FATAL(realOccIt != realOccs.end(), "iterator check");
                    ++realOccIt;
                    if (realOccIt != realOccs.end()) {
                        nextRealOcc = *realOccIt;
                    } else {
                        nextRealOcc = nullptr;
                    }
                    break;
                }
                case kAOccExit: {
                    CHECK_FATAL(exitOccIt != exitOccs.end(), "iterator check");
                    ++exitOccIt;
                    if (exitOccIt != exitOccs.end()) {
                        nextExitOcc = *exitOccIt;
                    } else {
                        nextExitOcc = nullptr;
                    }
                    break;
                }
                case kAOccPhi: {
                    phiOccs.push_back(static_cast<PhiOcc *>(pickedOcc));
                    CHECK_FATAL(phiDfnIt != phiDfns.end(), "iterator check");
                    ++phiDfnIt;
                    if (phiDfnIt != phiDfns.end()) {
                        nextPhiOcc =
                            preMp->New<PhiOcc>(cgFunc->GetAllBBs().at(dom->GetDtPreOrderItem(*phiDfnIt)), preAllocator);
                    } else {
                        nextPhiOcc = nullptr;
                    }
                    break;
                }
                case kAOccPhiOpnd: {
                    CHECK_FATAL(phiOpndDfnIt != phiOpndDfns.end(), "iterator check");
                    ++phiOpndDfnIt;
                    if (phiOpndDfnIt != phiOpndDfns.end()) {
                        nextPhiOpndOcc =
                            preMp->New<PhiOpndOcc>(cgFunc->GetAllBBs().at(dom->GetDtPreOrderItem(*phiOpndDfnIt)));
                        auto it = bb2PhiOpndMap.find(dom->GetDtPreOrderItem(*phiOpndDfnIt));
                        if (it == bb2PhiOpndMap.end()) {
                            std::forward_list<PhiOpndOcc *> newlist = {nextPhiOpndOcc};
                            bb2PhiOpndMap[dom->GetDtPreOrderItem(*phiOpndDfnIt)] = newlist;
                        } else {
                            it->second.push_front(nextPhiOpndOcc);
                        }
                    } else {
                        nextPhiOpndOcc = nullptr;
                    }
                    break;
                }
                default:
                    DEBUG_ASSERT(false, "CreateSortedOccs: unexpected occTy");
                    break;
            }
        }
    } while (pickedOcc != nullptr);
    // initialize phiOpnd vector in each PhiOcc node and defPhiOcc in each PhiOpndOcc
    for (PhiOcc *phiOcc : phiOccs) {
        for (BB *pred : phiOcc->cgbb->GetPreds()) {
            PhiOpndOcc *phiOpndOcc = bb2PhiOpndMap[pred->GetId()].front();
            phiOcc->phiOpnds.push_back(phiOpndOcc);
            phiOpndOcc->defPhiOcc = phiOcc;
            bb2PhiOpndMap[pred->GetId()].pop_front();
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after phi insertion _______" << '\n';
        for (Occ *occ : allOccs) {
            occ->Dump();
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 0: Preparations ================

void SSAPre::PropagateNotAnt(BB *bb, std::set<BB *, BBIdCmp> *visitedBBs)
{
    if (visitedBBs->count(bb) != 0) {
        return;
    }
    visitedBBs->insert(bb);
    if (workCand->occBBs.count(bb->GetId()) != 0) {
        return;
    }
    fullyAntBBs[bb->GetId()] = false;
    for (BB *predbb : bb->GetPreds()) {
        PropagateNotAnt(predbb, visitedBBs);
    }
}

void SSAPre::FormRealsNExits()
{
    std::set<BB *, BBIdCmp> visitedBBs;
    if (asEarlyAsPossible) {
        for (BB *cgbb : cgFunc->GetExitBBsVec()) {
            if (!cgbb->IsUnreachable()) {
                PropagateNotAnt(cgbb, &visitedBBs);
            }
        }
    }

    for (uint32 i = 0; i < dom->GetDtPreOrderSize(); i++) {
        BBId bbid = dom->GetDtPreOrderItem(i);
        BB *cgbb = cgFunc->GetAllBBs()[bbid];
        if (asEarlyAsPossible) {
            if (fullyAntBBs[cgbb->GetId()]) {
                RealOcc *realOcc = preMp->New<RealOcc>(cgbb);
                realOccs.push_back(realOcc);
            }
        } else {
            if (workCand->occBBs.count(cgbb->GetId()) != 0) {
                RealOcc *realOcc = preMp->New<RealOcc>(cgbb);
                realOccs.push_back(realOcc);
            }
        }
        if (!cgbb->IsUnreachable() && (cgbb->NumSuccs() == 0 || cgbb->GetKind() == BB::kBBReturn)) {
            ExitOcc *exitOcc = preMp->New<ExitOcc>(cgbb);
            exitOccs.push_back(exitOcc);
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << "Placement Optimization for callee-save saves" << '\n';
        LogInfo::MapleLogger() << "-----------------------------------------------" << '\n';
        LogInfo::MapleLogger() << " _______ input _______" << '\n';
        LogInfo::MapleLogger() << " occBBs: [";
        for (uint32 id : workCand->occBBs) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n";
    }
}

void SSAPre::ApplySSAPre()
{
    FormRealsNExits();
    // #1 insert phis; results in allOccs and phiOccs
    FormPhis();  // result put in the set phi_bbs
    CreateSortedOccs();
    // #2 rename
    Rename();
    if (!phiOccs.empty()) {
        // #3 DownSafety
        ComputeDownsafe();
        // #4 CanBeAvail
        ComputeCanBeAvail();
        ComputeLater();
    }
    // #5 Finalize
    Finalize();
    if (!workCand->saveAtProlog) {
        // #6 Code Motion
        CodeMotion();
    }
}

void DoSavePlacementOpt(CGFunc *f, DomAnalysis *dom, LoopAnalysis *loop, SsaPreWorkCand *workCand)
{
    MemPool *tempMP = memPoolCtrler.NewMemPool("cg_ssa_pre", true);
    SSAPre cgssapre(f, dom, loop, tempMP, workCand, false /*asEarlyAsPossible*/, false /*enabledDebug*/);

    cgssapre.ApplySSAPre();

    memPoolCtrler.DeleteMemPool(tempMP);
}

}  // namespace maplebe
