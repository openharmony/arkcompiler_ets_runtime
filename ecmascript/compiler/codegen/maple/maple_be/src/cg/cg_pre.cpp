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

#include "cg_pre.h"
#include "cg_dominance.h"
#include "aarch64_cg.h"

namespace maplebe {
/* Implement PRE in cgir */
void CGPre::ResetDS(CgPhiOcc *phiOcc)
{
    if (!phiOcc->IsDownSafe()) {
        return;
    }

    phiOcc->SetIsDownSafe(false);
    for (auto *phiOpnd : phiOcc->GetPhiOpnds()) {
        auto *defOcc = phiOpnd->GetDef();
        if (defOcc != nullptr && defOcc->GetOccType() == kOccPhiocc) {
            ResetDS(static_cast<CgPhiOcc *>(defOcc));
        }
    }
}

void CGPre::ComputeDS()
{
    for (auto phiIt = phiOccs.rbegin(); phiIt != phiOccs.rend(); ++phiIt) {
        auto *phiOcc = *phiIt;
        if (phiOcc->IsDownSafe()) {
            continue;
        }
        for (auto *phiOpnd : phiOcc->GetPhiOpnds()) {
            if (phiOpnd->HasRealUse()) {
                continue;
            }
            auto *defOcc = phiOpnd->GetDef();
            if (defOcc != nullptr && defOcc->GetOccType() == kOccPhiocc) {
                ResetDS(static_cast<CgPhiOcc *>(defOcc));
            }
        }
    }
}

/* based on ssapre->workCand's realOccs and dfPhiDfns (which now privides all
   the inserted phis), create the phi and phiOpnd occ nodes; link them all up in
   order of dt_preorder in ssapre->allOccs; the phi occ nodes are in addition
   provided in order of dt_preorder in ssapre->phiOccs */
void CGPre::CreateSortedOccs()
{
    // merge varPhiDfns to dfPhiDfns
    dfPhiDfns.insert(varPhiDfns.begin(), varPhiDfns.end());

    auto comparator = [this](const CgPhiOpndOcc *occA, const CgPhiOpndOcc *occB) -> bool {
        return dom->GetDtDfnItem(occA->GetBB()->GetId()) < dom->GetDtDfnItem(occB->GetBB()->GetId());
    };

    std::vector<CgPhiOpndOcc *> phiOpnds;
    for (auto dfn : dfPhiDfns) {
        uint32 bbId = dom->GetDtPreOrderItem(dfn);
        BB *bb = GetBB(bbId);
        auto *phiOcc = perCandMemPool->New<CgPhiOcc>(*bb, workCand->GetTheOperand(), perCandAllocator);
        phiOccs.push_back(phiOcc);

        for (BB *pred : bb->GetPreds()) {
            auto phiOpnd = perCandMemPool->New<CgPhiOpndOcc>(pred, workCand->GetTheOperand(), phiOcc);
            phiOpnds.push_back(phiOpnd);
            phiOcc->AddPhiOpnd(*phiOpnd);
            phiOpnd->SetPhiOcc(*phiOcc);
        }
    }
    std::sort(phiOpnds.begin(), phiOpnds.end(), comparator);

    auto realOccIt = workCand->GetRealOccs().begin();
    auto exitOccIt = exitOccs.begin();
    auto phiIt = phiOccs.begin();
    auto phiOpndIt = phiOpnds.begin();

    CgOccur *nextRealOcc = nullptr;
    if (realOccIt != workCand->GetRealOccs().end()) {
        nextRealOcc = *realOccIt;
    }

    CgOccur *nextExitOcc = nullptr;
    if (exitOccIt != exitOccs.end()) {
        nextExitOcc = *exitOccIt;
    }

    CgPhiOcc *nextPhiOcc = nullptr;
    if (phiIt != phiOccs.end()) {
        nextPhiOcc = *phiIt;
    }

    CgPhiOpndOcc *nextPhiOpndOcc = nullptr;
    if (phiOpndIt != phiOpnds.end()) {
        nextPhiOpndOcc = *phiOpndIt;
    }

    CgOccur *pickedOcc;  // the next picked occ in order of preorder traveral of dominator tree
    do {
        pickedOcc = nullptr;
        // the 4 kinds of occ must be checked in this order, so it will be right
        // if more than 1 has the same dfn
        if (nextPhiOcc != nullptr) {
            pickedOcc = nextPhiOcc;
        }
        if (nextRealOcc != nullptr && (pickedOcc == nullptr || dom->GetDtDfnItem(nextRealOcc->GetBB()->GetId()) <
                                                                   dom->GetDtDfnItem(pickedOcc->GetBB()->GetId()))) {
            pickedOcc = nextRealOcc;
        }
        if (nextExitOcc != nullptr && (pickedOcc == nullptr || dom->GetDtDfnItem(nextExitOcc->GetBB()->GetId()) <
                                                                   dom->GetDtDfnItem(pickedOcc->GetBB()->GetId()))) {
            pickedOcc = nextExitOcc;
        }
        if (nextPhiOpndOcc != nullptr && (pickedOcc == nullptr || dom->GetDtDfnItem(nextPhiOpndOcc->GetBB()->GetId()) <
                                                                      dom->GetDtDfnItem(pickedOcc->GetBB()->GetId()))) {
            pickedOcc = nextPhiOpndOcc;
        }
        if (pickedOcc != nullptr) {
            allOccs.push_back(pickedOcc);
            switch (pickedOcc->GetOccType()) {
                case kOccReal:
                case kOccUse:
                case kOccDef:
                case kOccStore:
                case kOccMembar: {
                    ++realOccIt;
                    if (realOccIt != workCand->GetRealOccs().end()) {
                        nextRealOcc = *realOccIt;
                    } else {
                        nextRealOcc = nullptr;
                    }
                    break;
                }
                case kOccExit: {
                    DEBUG_ASSERT(exitOccIt != exitOccs.end(), "must be valid iterator");
                    ++exitOccIt;
                    if (exitOccIt != exitOccs.end()) {
                        nextExitOcc = *exitOccIt;
                    } else {
                        nextExitOcc = nullptr;
                    }
                    break;
                }
                case kOccPhiocc: {
                    DEBUG_ASSERT(phiIt != phiOccs.end(), "must be valid iterator");
                    ++phiIt;
                    if (phiIt != phiOccs.end()) {
                        nextPhiOcc = *phiIt;
                    } else {
                        nextPhiOcc = nullptr;
                    }
                    break;
                }
                case kOccPhiopnd: {
                    ++phiOpndIt;
                    if (phiOpndIt != phiOpnds.end()) {
                        nextPhiOpndOcc = *phiOpndIt;
                    } else {
                        nextPhiOpndOcc = nullptr;
                    }
                    break;
                }
                default:
                    DEBUG_ASSERT(false, "CreateSortedOccs: unexpected occty");
                    break;
            }
        }
    } while (pickedOcc != nullptr);
}

CgOccur *CGPre::CreateRealOcc(Insn &insn, Operand &opnd, OccType occType)
{
    uint64 hashIdx = PreWorkCandHashTable::ComputeWorkCandHashIndex(opnd);
    PreWorkCand *wkCand = preWorkCandHashTable.GetWorkcandFromIndex(hashIdx);
    while (wkCand != nullptr) {
        Operand *currOpnd = wkCand->GetTheOperand();
        DEBUG_ASSERT(currOpnd != nullptr, "CreateRealOcc: found workcand with theMeExpr as nullptr");
        if (currOpnd == &opnd) {
            break;
        }
        wkCand = static_cast<PreWorkCand *>(wkCand->GetNext());
    }

    CgOccur *newOcc = nullptr;
    switch (occType) {
        case kOccDef:
            newOcc = ssaPreMemPool->New<CgDefOcc>(insn.GetBB(), &insn, &opnd);
            break;
        case kOccStore:
            newOcc = ssaPreMemPool->New<CgStoreOcc>(insn.GetBB(), &insn, &opnd);
            break;
        case kOccUse:
            newOcc = ssaPreMemPool->New<CgUseOcc>(insn.GetBB(), &insn, &opnd);
            break;
        default:
            CHECK_FATAL(false, "unsupported occur type");
            break;
    }

    if (wkCand != nullptr) {
        wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
        return newOcc;
    }

    // workcand not yet created; create a new one and add to worklist
    wkCand = ssaPreMemPool->New<PreWorkCand>(ssaPreAllocator, &opnd, GetPUIdx());
    workList.push_back(wkCand);
    wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
    // add to bucket at workcandHashTable[hashIdx]
    wkCand->SetNext(*preWorkCandHashTable.GetWorkcandFromIndex(hashIdx));
    preWorkCandHashTable.SetWorkCandAt(hashIdx, *wkCand);
    return newOcc;
}
}  // namespace maplebe
