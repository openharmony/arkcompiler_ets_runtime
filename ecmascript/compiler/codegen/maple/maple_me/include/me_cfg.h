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

#ifndef MAPLE_ME_INCLUDE_ME_CFG_H
#define MAPLE_ME_INCLUDE_ME_CFG_H
#include "me_function.h"
#include "maple_phase.h"

namespace maple {
class MeCFG : public AnalysisResult {
    using BBPtrHolder = MapleVector<BB *>;

public:
    using value_type = BBPtrHolder::value_type;
    using size_type = BBPtrHolder::size_type;
    using difference_type = BBPtrHolder::difference_type;
    using pointer = BBPtrHolder::pointer;
    using const_pointer = BBPtrHolder::const_pointer;
    using reference = BBPtrHolder::reference;
    using const_reference = BBPtrHolder::const_reference;
    using iterator = BBPtrHolder::iterator;
    using const_iterator = BBPtrHolder::const_iterator;
    using reverse_iterator = BBPtrHolder::reverse_iterator;
    using const_reverse_iterator = BBPtrHolder::const_reverse_iterator;

    MeCFG(MemPool *memPool, MeFunction &f)
        : AnalysisResult(memPool),
          mecfgAlloc(memPool),
          func(f),
          patternSet(mecfgAlloc.Adapter()),
          bbVec(mecfgAlloc.Adapter()),
          labelBBIdMap(mecfgAlloc.Adapter()),
          bbTryNodeMap(mecfgAlloc.Adapter()),
          endTryBB2TryBB(mecfgAlloc.Adapter()),
          sccTopologicalVec(mecfgAlloc.Adapter()),
          sccOfBB(mecfgAlloc.Adapter()),
          backEdges(mecfgAlloc.Adapter())
    {
    }

    ~MeCFG() = default;

    void Verify() const;
    void VerifyLabels() const;
    void Dump() const;
    bool FindExprUse(const BaseNode &expr, StIdx stIdx) const;
    bool FindDef(const StmtNode &stmt, StIdx stIdx) const;
    BB *NewBasicBlock();
    BB &InsertNewBasicBlock(const BB &position, bool isInsertBefore = true);
    void DeleteBasicBlock(const BB &bb);
    BB *NextBB(const BB *bb);
    
    BB *PrevBB(const BB *bb);

    const MeFunction &GetFunc() const
    {
        return func;
    }

    bool GetHasDoWhile() const
    {
        return hasDoWhile;
    }

    void SetHasDoWhile(bool hdw)
    {
        hasDoWhile = hdw;
    }

    MapleAllocator &GetAlloc()
    {
        return mecfgAlloc;
    }

    void SetNextBBId(uint32 currNextBBId)
    {
        nextBBId = currNextBBId;
    }
    uint32 GetNextBBId() const
    {
        return nextBBId;
    }
    void DecNextBBId()
    {
        --nextBBId;
    }

    MapleVector<BB *> &GetAllBBs()
    {
        return bbVec;
    }

    iterator begin()
    {
        return bbVec.begin();
    }
    const_iterator begin() const
    {
        return bbVec.begin();
    }
    const_iterator cbegin() const
    {
        return bbVec.cbegin();
    }

    iterator end()
    {
        return bbVec.end();
    }
    const_iterator end() const
    {
        return bbVec.end();
    }
    const_iterator cend() const
    {
        return bbVec.cend();
    }

    reverse_iterator rbegin()
    {
        return bbVec.rbegin();
    }
    const_reverse_iterator rbegin() const
    {
        return bbVec.rbegin();
    }
    const_reverse_iterator crbegin() const
    {
        return bbVec.crbegin();
    }

    reverse_iterator rend()
    {
        return bbVec.rend();
    }
    const_reverse_iterator rend() const
    {
        return bbVec.rend();
    }
    const_reverse_iterator crend() const
    {
        return bbVec.crend();
    }

    reference front()
    {
        return bbVec.front();
    }

    reference back()
    {
        return bbVec.back();
    }

    const_reference front() const
    {
        return bbVec.front();
    }

    const_reference back() const
    {
        return bbVec.back();
    }

    bool empty() const
    {
        return bbVec.empty();
    }

    size_t size() const
    {
        return bbVec.size();
    }
    FilterIterator<const_iterator> valid_begin() const
    {
        return build_filter_iterator(begin(), [this](const_iterator it) {
            return FilterNullPtr<const_iterator>(it, end());
        });
    }

    FilterIterator<const_iterator> valid_end() const
    {
        return build_filter_iterator(end());
    }

    FilterIterator<const_reverse_iterator> valid_rbegin() const
    {
        return build_filter_iterator(rbegin(), [this](const_reverse_iterator it) {
            return FilterNullPtr<const_reverse_iterator>(it, rend());
        });
    }

    FilterIterator<const_reverse_iterator> valid_rend() const
    {
        return build_filter_iterator(rend());
    }

    const_iterator common_entry() const
    {
        return begin();
    }

    const_iterator context_begin() const
    {
        return ++(++begin());
    }

    const_iterator context_end() const
    {
        return end();
    }

    const_iterator common_exit() const
    {
        return ++begin();
    }

    uint32 NumBBs() const
    {
        return nextBBId;
    }

    uint32 ValidBBNum() const
    {
        uint32 num = 0;
        for (auto bbit = valid_begin(); bbit != valid_end(); ++bbit) {
            ++num;
        }
        return num;
    }

    const MapleUnorderedMap<LabelIdx, BB *> &GetLabelBBIdMap() const
    {
        return labelBBIdMap;
    }
    BB *GetLabelBBAt(LabelIdx idx) const
    {
        auto it = labelBBIdMap.find(idx);
        if (it != labelBBIdMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void SetLabelBBAt(LabelIdx idx, BB *bb)
    {
        labelBBIdMap[idx] = bb;
    }
    void EraseLabelBBAt(LabelIdx idx)
    {
        labelBBIdMap.erase(idx);
    }

    BB *GetBBFromID(BBId bbID) const
    {
        DEBUG_ASSERT(bbID < bbVec.size(), "array index out of range");
        return bbVec.at(bbID);
    }

    void NullifyBBByID(BBId bbID)
    {
        DEBUG_ASSERT(bbID < bbVec.size(), "array index out of range");
        bbVec.at(bbID) = nullptr;
    }

    BB *GetCommonEntryBB() const
    {
        return *common_entry();
    }

    BB *GetCommonExitBB() const
    {
        return *common_exit();
    }

    BB *GetFirstBB() const
    {
        if (GetCommonEntryBB()->GetUniqueSucc()) {
            return GetCommonEntryBB()->GetUniqueSucc();
        }
        return *(++(++valid_begin()));
    }

    BB *GetLastBB() const
    {
        return *valid_rbegin();
    }

    void SetBBTryNodeMap(BB &bb, StmtNode &tryStmt)
    {
        bbTryNodeMap[&bb] = &tryStmt;
    }

    const MapleUnorderedMap<BB *, StmtNode *> &GetBBTryNodeMap() const
    {
        return bbTryNodeMap;
    }

    const MapleUnorderedMap<BB *, BB *> &GetEndTryBB2TryBB() const
    {
        return endTryBB2TryBB;
    }

    const BB *GetTryBBFromEndTryBB(BB *endTryBB) const
    {
        auto it = endTryBB2TryBB.find(endTryBB);
        return it == endTryBB2TryBB.end() ? nullptr : it->second;
    }

    BB *GetTryBBFromEndTryBB(BB *endTryBB)
    {
        auto it = endTryBB2TryBB.find(endTryBB);
        return it == endTryBB2TryBB.end() ? nullptr : it->second;
    }

    void SetBBTryBBMap(BB *currBB, BB *tryBB)
    {
        endTryBB2TryBB[currBB] = tryBB;
    }
    void SetTryBBByOtherEndTryBB(BB *endTryBB, BB *otherTryBB)
    {
        endTryBB2TryBB[endTryBB] = endTryBB2TryBB[otherTryBB];
    }

    void SwapBBId(BB &bb1, BB &bb2);
    void ConstructBBFreqFromStmtFreq();
    void ConstructStmtFreq();
    void ConstructEdgeFreqFromBBFreq();
    void VerifyBBFreq();

private:
    std::string ConstructFileNameToDump(const std::string &prefix) const;
    bool IsStartTryBB(BB &meBB) const;
    void SetTryBlockInfo(const StmtNode *nextStmt, StmtNode *tryStmt, BB *lastTryBB, BB *curBB, BB *newBB);

    void VerifySCC();
    void SCCTopologicalSort(std::vector<SCCOfBBs *> &sccNodes);
    void BuildSCCDFS(BB &bb, uint32 &visitIndex, std::vector<SCCOfBBs *> &sccNodes, std::vector<uint32> &visitedOrder,
                     std::vector<uint32> &lowestOrder, std::vector<bool> &inStack, std::stack<uint32> &visitStack);

    MapleAllocator mecfgAlloc;
    MeFunction &func;
    MapleSet<LabelIdx> patternSet;
    BBPtrHolder bbVec;
    MapleUnorderedMap<LabelIdx, BB *> labelBBIdMap;
    MapleUnorderedMap<BB *, StmtNode *> bbTryNodeMap;  // maps isTry bb to its try stmt
    MapleUnorderedMap<BB *, BB *> endTryBB2TryBB;      // maps endtry bb to its try bb
    bool hasDoWhile = false;
    uint32 nextBBId = 0;

    // BB SCC
    MapleVector<SCCOfBBs *> sccTopologicalVec;
    uint32 numOfSCCs = 0;
    MapleVector<SCCOfBBs *> sccOfBB;
    MapleSet<std::pair<uint32, uint32>> backEdges;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_CFG_H
