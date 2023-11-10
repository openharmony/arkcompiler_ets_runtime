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
#include "cg_ssu_pre.h"

namespace maplebe {

// ================ Step 6: Code Motion ================
void SSUPre::CodeMotion()
{
    // pass 1 only donig insertion
    for (SOcc *occ : allOccs) {
        if (occ->occTy != kSOccLambdaRes) {
            continue;
        }
        SLambdaResOcc *lambdaResOcc = static_cast<SLambdaResOcc *>(occ);
        if (lambdaResOcc->insertHere) {
            workCand->restoreAtEntryBBs.insert(lambdaResOcc->cgbb->GetId());
        }
    }
    // pass 2 only doing deletion
    for (SOcc *occ : realOccs) {
        if (occ->occTy != kSOccReal) {
            continue;
        }
        SRealOcc *realOcc = static_cast<SRealOcc *>(occ);
        if (!realOcc->redundant) {
            if (realOcc->cgbb->IsWontExit()) {
                workCand->restoreAtEpilog = true;
                break;
            }
            workCand->restoreAtExitBBs.insert(realOcc->cgbb->GetId());
        }
    }
    if (enabledDebug) {
        if (workCand->restoreAtEpilog) {
            LogInfo::MapleLogger() << "Giving up because of restore inside infinite loop" << '\n';
            return;
        }
        LogInfo::MapleLogger() << " _______ output _______" << '\n';
        LogInfo::MapleLogger() << " restoreAtEntryBBs: [";
        for (uint32 id : workCand->restoreAtEntryBBs) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n restoreAtExitBBs: [";
        for (uint32 id : workCand->restoreAtExitBBs) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n\n";
    }
}

// ================ Step 5: Finalize ================
// for setting SRealOcc's redundant flag and SLambdaResOcc's insertHere flag
void SSUPre::Finalize()
{
    std::vector<SOcc *> anticipatedDefVec(classCount + 1, nullptr);
    // preorder traversal of post-dominator tree
    for (SOcc *occ : allOccs) {
        size_t classId = static_cast<size_t>(occ->classId);
        switch (occ->occTy) {
            case kSOccLambda: {
                auto *lambdaOcc = static_cast<SLambdaOcc *>(occ);
                if (lambdaOcc->WillBeAnt()) {
                    anticipatedDefVec[classId] = lambdaOcc;
                }
                break;
            }
            case kSOccReal: {
                auto *realOcc = static_cast<SRealOcc *>(occ);
                if (anticipatedDefVec[classId] == nullptr || !anticipatedDefVec[classId]->IsPostDominate(pdom, occ)) {
                    realOcc->redundant = false;
                    anticipatedDefVec[classId] = realOcc;
                } else {
                    realOcc->redundant = true;
                }
                break;
            }
            case kSOccLambdaRes: {
                auto *lambdaResOcc = static_cast<SLambdaResOcc *>(occ);
                const SLambdaOcc *lambdaOcc = lambdaResOcc->useLambdaOcc;
                if (lambdaOcc->WillBeAnt()) {
                    if (lambdaResOcc->use == nullptr ||
                        (!lambdaResOcc->hasRealUse && lambdaResOcc->use->occTy == kSOccLambda &&
                         !static_cast<SLambdaOcc *>(lambdaResOcc->use)->WillBeAnt())) {
                        // insert a store
                        if (lambdaResOcc->cgbb->GetPreds().size() != 1) {  // critical edge
                            workCand->restoreAtEpilog = true;
                            break;
                        }
                        lambdaResOcc->insertHere = true;
                    } else {
                        lambdaResOcc->use = anticipatedDefVec[classId];
                    }
                }
                break;
            }
            case kSOccEntry:
            case kSOccKill:
                break;
            default:
                DEBUG_ASSERT(false, "Finalize: unexpected occ type");
                break;
        }
        if (workCand->restoreAtEpilog) {
            break;
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after finalize _______" << '\n';
        if (workCand->restoreAtEpilog) {
            LogInfo::MapleLogger() << "Giving up because of insertion at critical edge" << '\n';
            return;
        }
        for (SOcc *occ : allOccs) {
            if (occ->occTy == kSOccReal) {
                SRealOcc *realOcc = static_cast<SRealOcc *>(occ);
                if (!realOcc->redundant) {
                    occ->Dump();
                    LogInfo::MapleLogger() << " non-redundant" << '\n';
                }
            } else if (occ->occTy == kSOccLambdaRes) {
                SLambdaResOcc *lambdaResOcc = static_cast<SLambdaResOcc *>(occ);
                if (lambdaResOcc->insertHere) {
                    occ->Dump();
                    LogInfo::MapleLogger() << " insertHere" << '\n';
                }
            }
        }
    }
}

// ================ Step 4: WillBeAnt Computation ================

void SSUPre::ResetCanBeAnt(SLambdaOcc *lambda) const
{
    lambda->isCanBeAnt = false;
    // the following loop finds lambda's defs and reset them
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        for (SLambdaResOcc *lambdaResOcc : lambdaOcc->lambdaRes) {
            if (lambdaResOcc->use != nullptr && lambdaResOcc->use == lambda) {
                if (!lambdaResOcc->hasRealUse && !lambdaOcc->isUpsafe && lambdaOcc->isCanBeAnt) {
                    ResetCanBeAnt(lambdaOcc);
                }
            }
        }
    }
}

void SSUPre::ComputeCanBeAnt() const
{
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        if (!lambdaOcc->isUpsafe && lambdaOcc->isCanBeAnt) {
            bool existNullUse = false;
            for (SLambdaResOcc *lambdaResOcc : lambdaOcc->lambdaRes) {
                if (lambdaResOcc->use == nullptr) {
                    existNullUse = true;
                    break;
                }
            }
            if (existNullUse) {
                ResetCanBeAnt(lambdaOcc);
            }
        }
    }
}

void SSUPre::ResetEarlier(SLambdaOcc *lambda) const
{
    lambda->isEarlier = false;
    // the following loop finds lambda's defs and reset them
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        for (SLambdaResOcc *lambdaResOcc : lambdaOcc->lambdaRes) {
            if (lambdaResOcc->use != nullptr && lambdaResOcc->use == lambda) {
                if (lambdaOcc->isEarlier) {
                    ResetEarlier(lambdaOcc);
                }
            }
        }
    }
}

void SSUPre::ComputeEarlier() const
{
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        lambdaOcc->isEarlier = lambdaOcc->isCanBeAnt;
    }
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        if (lambdaOcc->isEarlier) {
            bool existNonNullUse = false;
            for (SLambdaResOcc *lambdaResOcc : lambdaOcc->lambdaRes) {
                if (lambdaResOcc->use != nullptr && lambdaResOcc->hasRealUse) {
                    existNonNullUse = true;
                    break;
                }
            }
            if (existNonNullUse) {
                ResetEarlier(lambdaOcc);
            }
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after earlier computation _______" << '\n';
        for (SLambdaOcc *lambdaOcc : lambdaOccs) {
            lambdaOcc->Dump();
            if (lambdaOcc->isCanBeAnt) {
                LogInfo::MapleLogger() << " canbeant";
            }
            if (lambdaOcc->isEarlier) {
                LogInfo::MapleLogger() << " earlier";
            }
            if (lambdaOcc->isCanBeAnt && !lambdaOcc->isEarlier) {
                LogInfo::MapleLogger() << " will be ant";
            }
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 3: Upsafe Computation ================
void SSUPre::ResetUpsafe(const SLambdaResOcc *lambdaRes) const
{
    if (lambdaRes->hasRealUse) {
        return;
    }
    SOcc *useOcc = lambdaRes->use;
    if (useOcc == nullptr || useOcc->occTy != kSOccLambda) {
        return;
    }
    auto *useLambdaOcc = static_cast<SLambdaOcc *>(useOcc);
    if (!useLambdaOcc->isUpsafe) {
        return;
    }
    useLambdaOcc->isUpsafe = false;
    for (SLambdaResOcc *lambdaResOcc : useLambdaOcc->lambdaRes) {
        ResetUpsafe(lambdaResOcc);
    }
}

void SSUPre::ComputeUpsafe() const
{
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        if (!lambdaOcc->isUpsafe) {
            // propagate not-upsafe forward along def-use edges
            for (SLambdaResOcc *lambdaResOcc : lambdaOcc->lambdaRes) {
                ResetUpsafe(lambdaResOcc);
            }
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after upsafe computation _______" << '\n';
        for (SLambdaOcc *lambdaOcc : lambdaOccs) {
            lambdaOcc->Dump();
            if (lambdaOcc->isUpsafe) {
                LogInfo::MapleLogger() << " upsafe";
            }
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 2: rename ================
void SSUPre::Rename()
{
    std::stack<SOcc *> occStack;
    classCount = 0;
    // iterate thru the occurrences in order of preorder traversal of
    // post-dominator tree
    for (SOcc *occ : allOccs) {
        while (!occStack.empty() && !occStack.top()->IsPostDominate(pdom, occ)) {
            occStack.pop();
        }
        switch (occ->occTy) {
            case kSOccKill:
                if (!occStack.empty()) {
                    SOcc *topOcc = occStack.top();
                    if (topOcc->occTy == kSOccLambda) {
                        static_cast<SLambdaOcc *>(topOcc)->isUpsafe = false;
                    }
                }
                occStack.push(occ);
                break;
            case kSOccEntry:
                if (!occStack.empty()) {
                    SOcc *topOcc = occStack.top();
                    if (topOcc->occTy == kSOccLambda) {
                        static_cast<SLambdaOcc *>(topOcc)->isUpsafe = false;
                    }
                }
                break;
            case kSOccLambda:
                // assign new class
                occ->classId = ++classCount;
                occStack.push(occ);
                break;
            case kSOccReal: {
                if (occStack.empty()) {
                    // assign new class
                    occ->classId = ++classCount;
                    occStack.push(occ);
                    break;
                }
                SOcc *topOcc = occStack.top();
                if (topOcc->occTy == kSOccKill) {
                    // assign new class
                    occ->classId = ++classCount;
                    occStack.push(occ);
                    break;
                }
                DEBUG_ASSERT(topOcc->occTy == kSOccLambda || topOcc->occTy == kSOccReal,
                             "Rename: unexpected top-of-stack occ");
                occ->classId = topOcc->classId;
                if (topOcc->occTy == kSOccLambda) {
                    occStack.push(occ);
                }
                break;
            }
            case kSOccLambdaRes: {
                if (occStack.empty()) {
                    // leave classId as 0
                    break;
                }
                SOcc *topOcc = occStack.top();
                if (topOcc->occTy == kSOccKill) {
                    // leave classId as 0
                    break;
                }
                DEBUG_ASSERT(topOcc->occTy == kSOccLambda || topOcc->occTy == kSOccReal,
                             "Rename: unexpected top-of-stack occ");
                occ->use = topOcc;
                occ->classId = topOcc->classId;
                if (topOcc->occTy == kSOccReal) {
                    static_cast<SLambdaResOcc *>(occ)->hasRealUse = true;
                }
                break;
            }
            default:
                DEBUG_ASSERT(false, "Rename: unexpected type of occurrence");
                break;
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after rename _______" << '\n';
        for (SOcc *occ : allOccs) {
            occ->Dump();
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 1: insert lambdas ================

// form lambda occ based on the real occ in workCand->realOccs; result is
// stored in lambdaDfns
void SSUPre::FormLambdas()
{
    for (SOcc *occ : realOccs) {
        if (occ->occTy == kSOccKill) {
            continue;
        }
        GetIterPdomFrontier(occ->cgbb, &lambdaDfns);
    }
}

// form allOccs inclusive of real, kill, lambda, lambdaRes, entry occurrences;
// form lambdaOccs containing only the lambdas
void SSUPre::CreateSortedOccs()
{
    // form lambdaRes occs based on the succs of the lambda occs; result is
    // stored in lambdaResDfns
    std::multiset<uint32> lambdaResDfns;
    for (uint32 dfn : lambdaDfns) {
        const BBId bbId = pdom->GetPdtPreOrderItem(dfn);
        BB *cgbb = cgFunc->GetAllBBs()[bbId];
        for (BB *succ : cgbb->GetSuccs()) {
            (void)lambdaResDfns.insert(pdom->GetPdtDfnItem(succ->GetId()));
        }
    }
    std::unordered_map<BBId, std::forward_list<SLambdaResOcc *>> bb2LambdaResMap;
    MapleVector<SOcc *>::iterator realOccIt = realOccs.begin();
    MapleVector<SEntryOcc *>::iterator entryOccIt = entryOccs.begin();
    MapleSet<uint32>::iterator lambdaDfnIt = lambdaDfns.begin();
    MapleSet<uint32>::iterator lambdaResDfnIt = lambdaResDfns.begin();
    SOcc *nextRealOcc = nullptr;
    if (realOccIt != realOccs.end()) {
        nextRealOcc = *realOccIt;
    }
    SEntryOcc *nextEntryOcc = nullptr;
    if (entryOccIt != entryOccs.end()) {
        nextEntryOcc = *entryOccIt;
    }
    SLambdaOcc *nextLambdaOcc = nullptr;
    if (lambdaDfnIt != lambdaDfns.end()) {
        nextLambdaOcc =
            spreMp->New<SLambdaOcc>(cgFunc->GetAllBBs().at(pdom->GetPdtPreOrderItem(*lambdaDfnIt)), spreAllocator);
    }
    SLambdaResOcc *nextLambdaResOcc = nullptr;
    if (lambdaResDfnIt != lambdaResDfns.end()) {
        nextLambdaResOcc =
            spreMp->New<SLambdaResOcc>(cgFunc->GetAllBBs().at(pdom->GetPdtPreOrderItem(*lambdaResDfnIt)));
        auto it = bb2LambdaResMap.find(pdom->GetPdtPreOrderItem(*lambdaResDfnIt));
        if (it == bb2LambdaResMap.end()) {
            std::forward_list<SLambdaResOcc *> newlist = {nextLambdaResOcc};
            bb2LambdaResMap[pdom->GetPdtPreOrderItem(*lambdaResDfnIt)] = newlist;
        } else {
            it->second.push_front(nextLambdaResOcc);
        }
    }
    SOcc *pickedOcc = nullptr;  // the next picked occ in order of preorder traversal of post-dominator tree
    do {
        pickedOcc = nullptr;
        if (nextLambdaOcc != nullptr) {
            pickedOcc = nextLambdaOcc;
        }
        if (nextRealOcc != nullptr && (pickedOcc == nullptr || pdom->GetPdtDfnItem(nextRealOcc->cgbb->GetId()) <
                                                                   pdom->GetPdtDfnItem(pickedOcc->cgbb->GetId()))) {
            pickedOcc = nextRealOcc;
        }
        if (nextLambdaResOcc != nullptr &&
            (pickedOcc == nullptr || *lambdaResDfnIt < pdom->GetPdtDfnItem(pickedOcc->cgbb->GetId()))) {
            pickedOcc = nextLambdaResOcc;
        }
        if (nextEntryOcc != nullptr && (pickedOcc == nullptr || pdom->GetPdtDfnItem(nextEntryOcc->cgbb->GetId()) <
                                                                    pdom->GetPdtDfnItem(pickedOcc->cgbb->GetId()))) {
            pickedOcc = nextEntryOcc;
        }
        if (pickedOcc != nullptr) {
            allOccs.push_back(pickedOcc);
            switch (pickedOcc->occTy) {
                case kSOccReal:
                case kSOccKill: {
                    // get the next real/kill occ
                    CHECK_FATAL(realOccIt != realOccs.end(), "iterator check");
                    ++realOccIt;
                    if (realOccIt != realOccs.end()) {
                        nextRealOcc = *realOccIt;
                    } else {
                        nextRealOcc = nullptr;
                    }
                    break;
                }
                case kSOccEntry: {
                    CHECK_FATAL(entryOccIt != entryOccs.end(), "iterator check");
                    ++entryOccIt;
                    if (entryOccIt != entryOccs.end()) {
                        nextEntryOcc = *entryOccIt;
                    } else {
                        nextEntryOcc = nullptr;
                    }
                    break;
                }
                case kSOccLambda: {
                    lambdaOccs.push_back(static_cast<SLambdaOcc *>(pickedOcc));
                    CHECK_FATAL(lambdaDfnIt != lambdaDfns.end(), "iterator check");
                    ++lambdaDfnIt;
                    if (lambdaDfnIt != lambdaDfns.end()) {
                        nextLambdaOcc = spreMp->New<SLambdaOcc>(
                            cgFunc->GetAllBBs().at(pdom->GetPdtPreOrderItem(*lambdaDfnIt)), spreAllocator);
                    } else {
                        nextLambdaOcc = nullptr;
                    }
                    break;
                }
                case kSOccLambdaRes: {
                    CHECK_FATAL(lambdaResDfnIt != lambdaResDfns.end(), "iterator check");
                    ++lambdaResDfnIt;
                    if (lambdaResDfnIt != lambdaResDfns.end()) {
                        nextLambdaResOcc = spreMp->New<SLambdaResOcc>(
                            cgFunc->GetAllBBs().at(pdom->GetPdtPreOrderItem(*lambdaResDfnIt)));
                        auto it = bb2LambdaResMap.find(pdom->GetPdtPreOrderItem(*lambdaResDfnIt));
                        if (it == bb2LambdaResMap.end()) {
                            std::forward_list<SLambdaResOcc *> newlist = {nextLambdaResOcc};
                            bb2LambdaResMap[pdom->GetPdtPreOrderItem(*lambdaResDfnIt)] = newlist;
                        } else {
                            it->second.push_front(nextLambdaResOcc);
                        }
                    } else {
                        nextLambdaResOcc = nullptr;
                    }
                    break;
                }
                default:
                    DEBUG_ASSERT(false, "CreateSortedOccs: unexpected occTy");
                    break;
            }
        }
    } while (pickedOcc != nullptr);
    // initialize lambdaRes vector in each SLambdaOcc node and useLambdaOcc in each SLambdaResOcc
    for (SLambdaOcc *lambdaOcc : lambdaOccs) {
        for (BB *succ : lambdaOcc->cgbb->GetSuccs()) {
            SLambdaResOcc *lambdaResOcc = bb2LambdaResMap[succ->GetId()].front();
            lambdaOcc->lambdaRes.push_back(lambdaResOcc);
            lambdaResOcc->useLambdaOcc = lambdaOcc;
            bb2LambdaResMap[succ->GetId()].pop_front();
        }
    }
    if (enabledDebug) {
        LogInfo::MapleLogger() << " _______ after lambda insertion _______" << '\n';
        for (SOcc *occ : allOccs) {
            occ->Dump();
            LogInfo::MapleLogger() << '\n';
        }
    }
}

// ================ Step 0: Preparations ================

void SSUPre::PropagateNotAvail(BB *bb, std::set<BB *, BBIdCmp> *visitedBBs)
{
    if (visitedBBs->count(bb) != 0) {
        return;
    }
    visitedBBs->insert(bb);
    if (workCand->occBBs.count(bb->GetId()) != 0 || workCand->saveBBs.count(bb->GetId()) != 0) {
        return;
    }
    fullyAvailBBs[bb->GetId()] = false;
    for (BB *succbb : bb->GetSuccs()) {
        PropagateNotAvail(succbb, visitedBBs);
    }
}

void SSUPre::FormReals()
{
    if (!asLateAsPossible) {
        for (uint32 i = 0; i < pdom->GetPdtPreOrderSize(); i++) {
            BBId bbid = pdom->GetPdtPreOrderItem(i);
            BB *cgbb = cgFunc->GetAllBBs()[bbid];
            if (workCand->saveBBs.count(cgbb->GetId()) != 0) {
                SRealOcc *realOcc = spreMp->New<SRealOcc>(cgbb);
                realOccs.push_back(realOcc);
                SKillOcc *killOcc = spreMp->New<SKillOcc>(cgbb);
                realOccs.push_back(killOcc);
            } else if (workCand->occBBs.count(cgbb->GetId()) != 0) {
                SRealOcc *realOcc = spreMp->New<SRealOcc>(cgbb);
                realOccs.push_back(realOcc);
            }
        }
    } else {
        std::set<BB *, BBIdCmp> visitedBBs;
        fullyAvailBBs[cgFunc->GetCommonExitBB()->GetId()] = false;
        PropagateNotAvail(cgFunc->GetFirstBB(), &visitedBBs);
        for (uint32 i = 0; i < pdom->GetPdtPreOrderSize(); i++) {
            BBId bbid = pdom->GetPdtPreOrderItem(i);
            BB *cgbb = cgFunc->GetAllBBs()[bbid];
            if (fullyAvailBBs[cgbb->GetId()]) {
                SRealOcc *realOcc = spreMp->New<SRealOcc>(cgbb);
                realOccs.push_back(realOcc);
                if (workCand->saveBBs.count(cgbb->GetId()) != 0) {
                    SKillOcc *killOcc = spreMp->New<SKillOcc>(cgbb);
                    realOccs.push_back(killOcc);
                }
            }
        }
    }

    if (enabledDebug) {
        LogInfo::MapleLogger() << "Placement Optimization for callee-save restores" << '\n';
        LogInfo::MapleLogger() << "-----------------------------------------------" << '\n';
        LogInfo::MapleLogger() << " _______ input _______" << '\n';
        LogInfo::MapleLogger() << " occBBs: [";
        for (uint32 id : workCand->occBBs) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n saveBBs: [";
        for (uint32 id : workCand->saveBBs) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "]\n";
    }
}

void SSUPre::ApplySSUPre()
{
    FormReals();
    // #1 insert lambdas; results in allOccs and lambdaOccs
    FormLambdas();  // result put in the set lambda_bbs
    CreateSortedOccs();
    // #2 rename
    Rename();
    if (!lambdaOccs.empty()) {
        // #3 UpSafety
        ComputeUpsafe();
        // #4 CanBeAnt
        ComputeCanBeAnt();
        ComputeEarlier();
    }
    // #5 Finalize
    Finalize();
    if (!workCand->restoreAtEpilog) {
        // #6 Code Motion
        CodeMotion();
    }
}

void DoRestorePlacementOpt(CGFunc *f, PostDomAnalysis *pdom, SPreWorkCand *workCand)
{
    MemPool *tempMP = memPoolCtrler.NewMemPool("cg_ssu_pre", true);
    SSUPre cgssupre(f, pdom, tempMP, workCand, true /*asLateAsPossible*/, false /*enabledDebug*/);

    cgssupre.ApplySSUPre();

    memPoolCtrler.DeleteMemPool(tempMP);
}

}  // namespace maplebe
