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

#include "try_catch.h"
namespace maplebe {
BBT *TryCatchBlocksLower::CreateNewBB(StmtNode *first, StmtNode *last)
{
    BBT *newBB = memPool.New<BBT>(first, last, &memPool);
    bbList.emplace_back(newBB);
    return newBB;
}

BBT *TryCatchBlocksLower::FindTargetBBlock(LabelIdx idx, const std::vector<BBT *> &bbs)
{
    for (auto &target : bbs) {
        if (target->GetLabelIdx() == idx) {
            return target;
        }
    }
    return nullptr;
}

/* returns the first statement that is moved in into the try block. If none is moved in, nullptr is returned */
StmtNode *TryCatchBlocksLower::MoveCondGotoIntoTry(BBT &jtBB, BBT &condbrBB, const MapleVector<BBT *> &labeledBBsInTry)
{
    StmtNode *firstStmtMovedIn = nullptr;
    const MapleVector<BBT *> &bbs = labeledBBsInTry;
    StmtNode *jtStmt = jtBB.GetKeyStmt();
#if DEBUG
    StmtNode *js = jtBB.GetFirstStmt();
    while (js->GetOpCode() != OP_try) {
        js = js->GetNext();
    }
    CHECK_FATAL(js == jtStmt, "make sure js equal jtStmt");
#endif
    StmtNode *ts = jtBB.GetFirstStmt()->GetPrev();
    while ((ts != nullptr) && (ts->GetOpCode() == OP_comment)) {
        ts = ts->GetPrev();
    }

    if (ts != nullptr && ts->IsCondBr()) {
        CHECK_FATAL(ts->GetNext() == jtBB.GetFirstStmt(), "make sure ts's next equal jtBB's firstStmt");
        StmtNode *firstStmtNode = jtBB.GetFirstStmt();
        /* [ jtbb_b..jtstmt ]; either jtbb_b is a comment or jtbb_b == jtstmt */
        LabelIdx id = static_cast<CondGotoNode *>(ts)->GetOffset();
        for (auto &lbb : bbs) {
            if (lbb->GetLabelIdx() == id) {
                /*
                 * this cond goto jumps into the try block; let the try block enclose it
                 * first find the preceding comment statements if any
                 */
                StmtNode *brS = ts;
                while ((ts->GetPrev() != nullptr) && (ts->GetPrev()->GetOpCode() == OP_comment)) {
                    ts = ts->GetPrev();
                }
                StmtNode *secondStmtNode = ts; /* beginning statement of branch block */
                /* [ brbb_b..br_s ]; either brbb_b is a comment or brbb_b == br_s */
                firstStmtNode->SetPrev(secondStmtNode->GetPrev());
                if (secondStmtNode->GetPrev()) {
                    secondStmtNode->GetPrev()->SetNext(firstStmtNode);
                }
                jtStmt->GetNext()->SetPrev(brS);
                brS->SetNext(jtStmt->GetNext());
                secondStmtNode->SetPrev(jtStmt);
                jtStmt->SetNext(secondStmtNode);
                condbrBB.SetLastStmt(*firstStmtNode->GetPrev());
                CHECK_FATAL(condbrBB.GetFallthruBranch() == &jtBB, "make sure condbrBB's fallthruBranch equal &jtBB");
                condbrBB.SetFallthruBranch(&jtBB);
                condbrBB.SetCondJumpBranch(nullptr);
                firstStmtMovedIn = secondStmtNode;
                break;
            }
        }
    }
    return firstStmtMovedIn;
}

/* collect catchbb->fallthru(0-n) into currBBThread, when encounter a new catch, return it, else return nullptr */
BBT *TryCatchBlocksLower::CollectCatchAndFallthruUntilNextCatchBB(BBT *&lowerBB, uint32 &nextEnclosedIdx,
                                                                  std::vector<BBT *> &currBBThread)
{
    MapleVector<BBT *> &enclosedBBs = tryEndTryBlock.GetEnclosedBBs();
    BBT *endTryBB = tryEndTryBlock.GetEndTryBB();

    BBT *nextBBThreadHead = nullptr;
    while (lowerBB->GetFallthruBranch() != nullptr) {
        lowerBB = lowerBB->GetFallthruBranch();
        ++nextEnclosedIdx;
        if (lowerBB->IsEndTry()) {
            CHECK_FATAL(lowerBB == endTryBB, "lowerBB should equal endTryBB");
            break;
        }

        for (uint32 j = 0; j < enclosedBBs.size(); ++j) {
            if (enclosedBBs[j] == lowerBB) {
                enclosedBBs[j] = nullptr;
                break;
            }
        }
        if (lowerBB->IsCatch()) {
            nextBBThreadHead = lowerBB;
            break;
        }
        currBBThread.emplace_back(lowerBB);
    }

    if (nextBBThreadHead == nullptr && lowerBB->GetFallthruBranch() == nullptr && lowerBB != endTryBB &&
        nextEnclosedIdx < enclosedBBs.size() && enclosedBBs[nextEnclosedIdx]) {
        /*
         * Using a loop to find the next_bb_thread_head when it's a catch_BB or a normal_BB which
         * is after a catch_BB. Other condition, push_back into the curr_bb_thread.
         */
        do {
            lowerBB = enclosedBBs[nextEnclosedIdx];
            enclosedBBs[nextEnclosedIdx++] = nullptr;
            BBT *head = currBBThread.front();
            if (head->IsCatch() || lowerBB->IsCatch()) {
                nextBBThreadHead = lowerBB;
                break;
            }
            currBBThread.emplace_back(lowerBB);
        } while (nextEnclosedIdx < enclosedBBs.size());
    }

    return nextBBThreadHead;
}

void TryCatchBlocksLower::ProcessThreadTail(BBT &threadTail, BBT *const &nextBBThreadHead, bool hasMoveEndTry)
{
    BBT *endTryBB = tryEndTryBlock.GetEndTryBB();
    StmtNode *newEndTry = endTryBB->GetKeyStmt()->CloneTree(mirModule.GetCurFuncCodeMPAllocator());
    newEndTry->SetPrev(threadTail.GetLastStmt());
    newEndTry->SetNext(threadTail.GetLastStmt()->GetNext());
    if (bodyEndWithEndTry && hasMoveEndTry) {
        if (threadTail.GetLastStmt()->GetNext()) {
            threadTail.GetLastStmt()->GetNext()->SetPrev(newEndTry);
        }
    } else {
        CHECK_FATAL(threadTail.GetLastStmt()->GetNext() != nullptr,
                    "the next of threadTail's lastStmt should not be nullptr");
        threadTail.GetLastStmt()->GetNext()->SetPrev(newEndTry);
    }
    threadTail.GetLastStmt()->SetNext(newEndTry);

    threadTail.SetLastStmt(*newEndTry);
    if (hasMoveEndTry && nextBBThreadHead == nullptr) {
        body.SetLast(threadTail.GetLastStmt());
    }
}

/* Wrap this catch block with try-endtry block */
void TryCatchBlocksLower::WrapCatchWithTryEndTryBlock(std::vector<BBT *> &currBBThread, BBT *&nextBBThreadHead,
                                                      uint32 &nextEnclosedIdx, bool hasMoveEndTry)
{
    BBT *endTryBB = tryEndTryBlock.GetEndTryBB();
    StmtNode *tryStmt = tryEndTryBlock.GetTryStmtNode();
    MapleVector<BBT *> &enclosedBBs = tryEndTryBlock.GetEnclosedBBs();
    for (auto &e : currBBThread) {
        CHECK_FATAL(!e->IsTry(), "expect e is not try");
    }
    BBT *threadHead = currBBThread.front();
    if (threadHead->IsCatch()) {
        StmtNode *jcStmt = threadHead->GetKeyStmt();
        CHECK_FATAL(jcStmt->GetNext() != nullptr, "jcStmt's next should not be nullptr");
        TryNode *jtCopy = static_cast<TryNode *>(tryStmt)->CloneTree(mirModule.GetCurFuncCodeMPAllocator());
        jtCopy->SetNext(jcStmt->GetNext());
        jtCopy->SetPrev(jcStmt);
        jcStmt->GetNext()->SetPrev(jtCopy);
        jcStmt->SetNext(jtCopy);

        BBT *threadTail = currBBThread.back();

        /* for this endtry stmt, we don't need to create a basic block */
        ProcessThreadTail(*threadTail, static_cast<BBT *const &>(nextBBThreadHead), hasMoveEndTry);
    } else {
        /* For cases try->catch->normal_bb->normal_bb->endtry, Combine normal bb first. */
        while (nextEnclosedIdx < enclosedBBs.size()) {
            if (nextBBThreadHead != nullptr) {
                if (nextBBThreadHead->IsCatch()) {
                    break;
                }
            }
            BBT *ebbSecond = enclosedBBs[nextEnclosedIdx];
            enclosedBBs[nextEnclosedIdx++] = nullptr;
            CHECK_FATAL(ebbSecond != endTryBB, "ebbSecond should not equal endTryBB");
            if (ebbSecond->IsCatch()) {
                nextBBThreadHead = ebbSecond;
                break;
            }
            currBBThread.emplace_back(ebbSecond);
        }
        /* normal bb. */
        StmtNode *stmt = threadHead->GetFirstStmt();

        TryNode *jtCopy = static_cast<TryNode *>(tryStmt)->CloneTree(mirModule.GetCurFuncCodeMPAllocator());
        jtCopy->SetNext(stmt);
        jtCopy->SetPrev(stmt->GetPrev());
        stmt->GetPrev()->SetNext(jtCopy);
        stmt->SetPrev(jtCopy);
        threadHead->SetFirstStmt(*jtCopy);

        BBT *threadTail = currBBThread.back();

        /* for this endtry stmt, we don't need to create a basic block */
        ProcessThreadTail(*threadTail, static_cast<BBT *const &>(nextBBThreadHead), hasMoveEndTry);
    }
}

/*
 * We have the following case.
 * bb_head -> bb_1 -> .. bb_n -> endtry_bb -> succ
 * For this particular case, we swap EndTry bb and curr_bb_thread, because the bblock that contains the endtry
 * statement does not contain any other statements!!
 */
void TryCatchBlocksLower::SwapEndTryBBAndCurrBBThread(const std::vector<BBT *> &currBBThread, bool &hasMoveEndTry,
                                                      const BBT *nextBBThreadHead)
{
    BBT *endTryBB = tryEndTryBlock.GetEndTryBB();
    CHECK_FATAL(endTryBB->GetFirstStmt()->GetOpCode() == OP_comment ||
                    endTryBB->GetFirstStmt()->GetOpCode() == OP_endtry,
                "the opcode of endTryBB's firstStmt should be OP_comment or OP_endtry");
    CHECK_FATAL(endTryBB->GetLastStmt()->GetOpCode() == OP_endtry,
                "the opcode of endTryBB's lastStmt should be OP_endtry");

    /* we move endtry_bb before bb_head */
    BBT *threadHead = currBBThread.front();
    CHECK_FATAL(threadHead->GetFirstStmt()->GetPrev() != nullptr,
                "the prev of threadHead's firstStmt should not nullptr");
    CHECK_FATAL(threadHead->GetFirstStmt()->GetOpCode() == OP_comment ||
                    threadHead->GetFirstStmt()->GetOpCode() == OP_label,
                "the opcode of threadHead's firstStmt should be OP_comment or OP_label");
    CHECK_FATAL(threadHead->GetFirstStmt()->GetPrev()->GetNext() == threadHead->GetFirstStmt(),
                "the next of the prev of threadHead's firstStmt should equal threadHead's firstStmt");

    endTryBB->GetFirstStmt()->GetPrev()->SetNext(endTryBB->GetLastStmt()->GetNext());
    if (endTryBB->GetLastStmt()->GetNext() != nullptr) {
        endTryBB->GetLastStmt()->GetNext()->SetPrev(endTryBB->GetFirstStmt()->GetPrev());
    }

    threadHead->GetFirstStmt()->GetPrev()->SetNext(endTryBB->GetFirstStmt());
    endTryBB->GetFirstStmt()->SetPrev(threadHead->GetFirstStmt()->GetPrev());

    endTryBB->GetLastStmt()->SetNext(threadHead->GetFirstStmt());
    threadHead->GetFirstStmt()->SetPrev(endTryBB->GetLastStmt());

    CHECK_FATAL(endTryBB->GetCondJumpBranch() == nullptr, "endTryBB's condJumpBranch must be nullptr");
    endTryBB->SetFallthruBranch(nullptr);
    if (bodyEndWithEndTry) {
        hasMoveEndTry = true;
        if (nextBBThreadHead == nullptr) {
            body.SetLast(currBBThread.back()->GetLastStmt());
        }
    }
}

void TryCatchBlocksLower::ProcessEnclosedBBBetweenTryEndTry()
{
    MapleVector<BBT *> &enclosedBBs = tryEndTryBlock.GetEnclosedBBs();
    MapleVector<BBT *> &labeledBBsInTry = tryEndTryBlock.GetLabeledBBsInTry();

    for (uint32 i = 0; i < enclosedBBs.size(); ++i) {
        BBT *lowerBB = enclosedBBs[i];
        if (lowerBB == nullptr) {
            continue; /* we may have removed the element */
        }
        if (!lowerBB->IsLabeled()) {
            continue;
        }
        labeledBBsInTry.emplace_back(lowerBB);
    }
}

void TryCatchBlocksLower::ConnectRemainBB()
{
    MapleVector<BBT *> &enclosedBBs = tryEndTryBlock.GetEnclosedBBs();
    BBT *startTryBB = tryEndTryBlock.GetStartTryBB();
    BBT *endTryBB = tryEndTryBlock.GetEndTryBB();
    size_t nEnclosedBBs = enclosedBBs.size();
    size_t k = 0;
    while ((k < nEnclosedBBs) && (enclosedBBs[k] == nullptr)) {
        ++k;
    }

    if (k < nEnclosedBBs) {
        BBT *prevBB = enclosedBBs[k];

        startTryBB->GetLastStmt()->SetNext(prevBB->GetFirstStmt());
        prevBB->GetFirstStmt()->SetPrev(startTryBB->GetLastStmt());

        for (++k; k < nEnclosedBBs; ++k) {
            BBT *lowerBB = enclosedBBs[k];
            if (lowerBB == nullptr) {
                continue;
            }
            prevBB->GetLastStmt()->SetNext(lowerBB->GetFirstStmt());
            lowerBB->GetFirstStmt()->SetPrev(prevBB->GetLastStmt());
            prevBB = lowerBB;
        }

        prevBB->GetLastStmt()->SetNext(endTryBB->GetFirstStmt());
        endTryBB->GetFirstStmt()->SetPrev(prevBB->GetLastStmt());
    } else {
        startTryBB->GetLastStmt()->SetNext(endTryBB->GetFirstStmt());
        endTryBB->GetFirstStmt()->SetPrev(startTryBB->GetLastStmt());
    }
}

BBT *TryCatchBlocksLower::FindInsertAfterBB()
{
    BBT *insertAfter = tryEndTryBlock.GetEndTryBB();
    CHECK_FATAL(tryEndTryBlock.GetEndTryBB()->GetLastStmt()->GetOpCode() == OP_endtry, "LowerBB type check");
    BBT *iaOpenTry = nullptr;
    while (insertAfter->GetFallthruBranch() != nullptr || iaOpenTry != nullptr) {
        if (insertAfter->GetFallthruBranch() != nullptr) {
            insertAfter = insertAfter->GetFallthruBranch();
        } else {
            CHECK_FATAL(iaOpenTry != nullptr, "iaOpenTry should not be nullptr");
            insertAfter = firstStmtToBBMap[insertAfter->GetLastStmt()->GetNext()];
            CHECK_FATAL(!insertAfter->IsTry(), "insertAfter should not be try");
        }

        if (insertAfter->IsTry()) {
            iaOpenTry = insertAfter;
        } else if (insertAfter->IsEndTry()) {
            iaOpenTry = nullptr;
        }
    }
    return insertAfter;
}

void TryCatchBlocksLower::PlaceRelocatedBB(BBT &insertAfter)
{
    StmtNode *iaLast = insertAfter.GetLastStmt();
    CHECK_FATAL(iaLast != nullptr, "iaLast should not nullptr");

    StmtNode *iaNext = iaLast->GetNext();
    if (iaNext == nullptr) {
        CHECK_FATAL(body.GetLast() == iaLast, "body's last should equal iaLast");
    }
    BBT *prevBB = &insertAfter;
    MapleVector<BBT *> &bbsToRelocate = tryEndTryBlock.GetBBsToRelocate();
    for (auto &rbb : bbsToRelocate) {
        prevBB->GetLastStmt()->SetNext(rbb->GetFirstStmt());
        rbb->GetFirstStmt()->SetPrev(prevBB->GetLastStmt());
        prevBB = rbb;
    }
    prevBB->GetLastStmt()->SetNext(iaNext);
    if (iaNext != nullptr) {
        iaNext->SetPrev(prevBB->GetLastStmt());
    } else {
        /* !ia_next means we started with insert_after that was the last bblock Refer to the above CHECK_FATAL. */
        body.SetLast(prevBB->GetLastStmt());
        body.GetLast()->SetNext(nullptr);
    }
}

void TryCatchBlocksLower::CheckTryCatchPattern() const
{
    StmtNode *openJt = nullptr;
    for (StmtNode *stmt = body.GetFirst(); stmt; stmt = stmt->GetNext()) {
        switch (stmt->GetOpCode()) {
            case OP_try:
                openJt = stmt;
                break;
            case OP_endtry:
                openJt = nullptr;
                break;
            case OP_catch:
                if (openJt != nullptr) {
                    CatchNode *jcn = static_cast<CatchNode *>(stmt);
                    for (uint32 i = 0; i < jcn->Size(); ++i) {
                        MIRType *type =
                            GlobalTables::GetTypeTable().GetTypeFromTyIdx(jcn->GetExceptionTyIdxVecElement(i));
                        MIRPtrType *ptr = static_cast<MIRPtrType *>(type);
                        type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptr->GetPointedTyIdx());
                        CHECK_FATAL(type->GetPrimType() == PTY_void, "type's primType should be PTY_void");
                    }
                }
                break;
            default:
                break;
        }
    }
}
} /* namespace maplebe */