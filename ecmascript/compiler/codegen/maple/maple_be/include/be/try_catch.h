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

#ifndef MAPLEBE_INCLUDE_BE_TRY_CATCH_H
#define MAPLEBE_INCLUDE_BE_TRY_CATCH_H
#include "bbt.h"
/* MapleIR headers. */
#include "mir_nodes.h"
#include "mir_lower.h"

namespace maplebe {
using namespace maple;
class TryEndTryBlock {
public:
    explicit TryEndTryBlock(MemPool &memPool)
        : allocator(&memPool),
          enclosedBBs(allocator.Adapter()),
          labeledBBsInTry(allocator.Adapter()),
          bbsToRelocate(allocator.Adapter())
    {
    }

    ~TryEndTryBlock() = default;

    void Init()
    {
        startTryBB = nullptr;
        endTryBB = nullptr;
        tryStmt = nullptr;
        enclosedBBs.clear();
        labeledBBsInTry.clear();
        bbsToRelocate.clear();
    }

    void Reset(BBT &startBB)
    {
        startTryBB = &startBB;
        CHECK_NULL_FATAL(startTryBB->GetKeyStmt());
        tryStmt = startTryBB->GetKeyStmt();
        CHECK_FATAL(tryStmt->GetOpCode() == OP_try, "expect OPT_try");
        endTryBB = nullptr;
        enclosedBBs.clear();
        labeledBBsInTry.clear();
        bbsToRelocate.clear();
    }

    void SetStartTryBB(BBT *bb)
    {
        startTryBB = bb;
    }

    BBT *GetStartTryBB()
    {
        return startTryBB;
    }

    void SetEndTryBB(BBT *bb)
    {
        endTryBB = bb;
    }

    BBT *GetEndTryBB()
    {
        return endTryBB;
    }

    StmtNode *GetTryStmtNode()
    {
        return tryStmt;
    }

    MapleVector<BBT *> &GetEnclosedBBs()
    {
        return enclosedBBs;
    }

    size_t GetEnclosedBBsSize() const
    {
        return enclosedBBs.size();
    }

    const BBT *GetEnclosedBBsElem(size_t index) const
    {
        DEBUG_ASSERT(index < enclosedBBs.size(), "out of range");
        return enclosedBBs[index];
    }

    void PushToEnclosedBBs(BBT &bb)
    {
        enclosedBBs.emplace_back(&bb);
    }

    MapleVector<BBT *> &GetLabeledBBsInTry()
    {
        return labeledBBsInTry;
    }

    MapleVector<BBT *> &GetBBsToRelocate()
    {
        return bbsToRelocate;
    }

private:
    MapleAllocator allocator;
    BBT *startTryBB = nullptr;
    BBT *endTryBB = nullptr;
    StmtNode *tryStmt = nullptr;
    MapleVector<BBT *> enclosedBBs;
    MapleVector<BBT *> labeledBBsInTry;
    MapleVector<BBT *> bbsToRelocate;
};

class TryCatchBlocksLower {
public:
    TryCatchBlocksLower(MemPool &memPool, BlockNode &body, MIRModule &mirModule)
        : memPool(memPool),
          allocator(&memPool),
          body(body),
          mirModule(mirModule),
          tryEndTryBlock(memPool),
          bbList(allocator.Adapter()),
          prevBBOfTry(allocator.Adapter()),
          firstStmtToBBMap(allocator.Adapter()),
          catchesSeenSoFar(allocator.Adapter())
    {
    }

    ~TryCatchBlocksLower() = default;
    void CheckTryCatchPattern() const;

    void SetGenerateEHCode(bool val)
    {
        generateEHCode = val;
    }

private:
    MemPool &memPool;
    MapleAllocator allocator;
    BlockNode &body;
    MIRModule &mirModule;
    TryEndTryBlock tryEndTryBlock;
    bool bodyEndWithEndTry = false;
    bool generateEHCode = false;
    MapleVector<BBT *> bbList;
    MapleUnorderedMap<BBT *, BBT *> prevBBOfTry;
    MapleUnorderedMap<StmtNode *, BBT *> firstStmtToBBMap;
    MapleVector<BBT *> catchesSeenSoFar;

    void ProcessEnclosedBBBetweenTryEndTry();
    void ConnectRemainBB();
    BBT *FindInsertAfterBB();
    void PlaceRelocatedBB(BBT &insertAfter);
    BBT *CreateNewBB(StmtNode *first, StmtNode *last);
    BBT *CollectCatchAndFallthruUntilNextCatchBB(BBT *&ebb, uint32 &nextEnclosedIdx, std::vector<BBT *> &currBBThread);
    void WrapCatchWithTryEndTryBlock(std::vector<BBT *> &currBBThread, BBT *&nextBBThreadHead, uint32 &nextEnclosedIdx,
                                     bool hasMoveEndTry);
    void SwapEndTryBBAndCurrBBThread(const std::vector<BBT *> &currBBThread, bool &hasMoveEndTry,
                                     const BBT *nextBBThreadHead);
    void ProcessThreadTail(BBT &threadTail, BBT *const &nextBBThreadHead, bool hasMoveEndTry);
    static StmtNode *MoveCondGotoIntoTry(BBT &jtBB, BBT &condbrBB, const MapleVector<BBT *> &labeledBBsInTry);
    static BBT *FindTargetBBlock(LabelIdx idx, const std::vector<BBT *> &bbs);
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_BE_TRY_CATCH_H */