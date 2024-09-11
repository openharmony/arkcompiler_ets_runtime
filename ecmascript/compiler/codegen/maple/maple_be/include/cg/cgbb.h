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

#ifndef MAPLEBE_INCLUDE_CG_CGBB_H
#define MAPLEBE_INCLUDE_CG_CGBB_H

#include "isa.h"
#include "insn.h"
#include "sparse_datainfo.h"

/* Maple IR headers */
#include "mir_nodes.h"
#include "mir_symbol.h"

/* Maple MP header */
#include "mempool_allocator.h"
#include "maple_phase_manager.h"
#include "base_graph_node.h"

namespace maplebe {
/* For get bb */
#define FIRST_BB_OF_FUNC(FUNC) ((FUNC)->GetFirstBB())
#define LAST_BB_OF_FUNC(FUNC) ((FUNC)->GetLastBB())

/* For iterating over basic blocks. */
#define FOR_BB_BETWEEN(BASE, FROM, TO, DIR) for (BB * (BASE) = (FROM); (BASE) != (TO); (BASE) = (BASE)->DIR())
#define FOR_BB_BETWEEN_CONST(BASE, FROM, TO, DIR) \
    for (const BB *(BASE) = (FROM); (BASE) != (TO); (BASE) = (BASE)->DIR())

#define FOR_ALL_BB_CONST(BASE, FUNC) FOR_BB_BETWEEN_CONST(BASE, FIRST_BB_OF_FUNC(FUNC), nullptr, GetNext)
#define FOR_ALL_BB(BASE, FUNC) FOR_BB_BETWEEN(BASE, FIRST_BB_OF_FUNC(FUNC), nullptr, GetNext)
#define FOR_ALL_BB_REV(BASE, FUNC) FOR_BB_BETWEEN(BASE, LAST_BB_OF_FUNC(FUNC), nullptr, GetPrev)

/* For get insn */
#define FIRST_INSN(BLOCK) (BLOCK)->GetFirstInsn()
#define LAST_INSN(BLOCK) (BLOCK)->GetLastInsn()
#define NEXT_INSN(INSN) (INSN)->GetNext()
#define PREV_INSN(INSN) (INSN)->GetPrev()

/* For iterating over insns in basic block. */
#define FOR_INSN_BETWEEN(INSN, FROM, TO, DIR) \
    for (Insn * (INSN) = (FROM); (INSN) != nullptr && (INSN) != (TO); (INSN) = (INSN)->DIR)

#define FOR_BB_INSNS(INSN, BLOCK) for (Insn * (INSN) = FIRST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetNext())
#define FOR_BB_INSNS_CONST(INSN, BLOCK) \
    for (const Insn *(INSN) = FIRST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetNext())

#define FOR_BB_INSNS_REV(INSN, BLOCK) \
    for (Insn * (INSN) = LAST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetPrev())
#define FOR_BB_INSNS_REV_CONST(INSN, BLOCK) \
    for (const Insn *(INSN) = LAST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetPrev())

/* For iterating over insns in basic block when we might remove the current insn. */
#define FOR_BB_INSNS_SAFE(INSN, BLOCK, NEXT)                                                                 \
    for (Insn * (INSN) = FIRST_INSN(BLOCK), *(NEXT) = (INSN) ? NEXT_INSN(INSN) : nullptr; (INSN) != nullptr; \
         (INSN) = (NEXT), (NEXT) = (INSN) ? NEXT_INSN(INSN) : nullptr)

#define FOR_BB_INSNS_REV_SAFE(INSN, BLOCK, NEXT)                                                            \
    for (Insn * (INSN) = LAST_INSN(BLOCK), *(NEXT) = (INSN) ? PREV_INSN(INSN) : nullptr; (INSN) != nullptr; \
         (INSN) = (NEXT), (NEXT) = (INSN) ? PREV_INSN(INSN) : nullptr)

class CGFunc;
class CDGNode;

using BBID = uint32;

class BB : public maple::BaseGraphNode {
public:
    static constexpr int32 kUnknownProb = -1;
    static constexpr uint32 kBBIfSuccsSize = 2;
    enum BBKind : uint8 {
        kBBFallthru, /* default */
        kBBIf,       /* conditional branch */
        kBBGoto,     /* unconditional branch */
        kBBReturn,
        kBBNoReturn,
        kBBRangeGoto,
        kBBLast
    };

    BB(BBID bbID, MapleAllocator &mallocator)
        : id(bbID),
          kind(kBBFallthru), /* kBBFallthru default kind */
          labIdx(MIRLabelTable::GetDummyLabel()),
          preds(mallocator.Adapter()),
          succs(mallocator.Adapter()),
          succsProb(mallocator.Adapter()),
          liveInRegNO(mallocator.Adapter()),
          liveOutRegNO(mallocator.Adapter()),
          callInsns(mallocator.Adapter()),
          rangeGotoLabelVec(mallocator.Adapter()),
          phiInsnList(mallocator.Adapter())
    {
    }

    virtual ~BB() = default;

    virtual BB *Clone(MemPool &memPool) const
    {
        BB *bb = memPool.Clone<BB>(*this);
        return bb;
    }

    void Dump() const;
    bool IsCommentBB() const;
    bool IsEmptyOrCommentOnly() const;
    bool IsSoloGoto() const;

    bool IsEmpty() const
    {
        if (lastInsn == nullptr) {
            CHECK_FATAL(firstInsn == nullptr, "firstInsn must be nullptr");
            return true;
        } else {
            CHECK_FATAL(firstInsn != nullptr, "firstInsn must not be nullptr");
            return false;
        }
    }

    const std::string &GetKindName() const
    {
        DEBUG_ASSERT(kind < kBBLast, "out of range in GetKindName");
        return bbNames[kind];
    }

    void SetKind(BBKind bbKind)
    {
        kind = bbKind;
    }

    BBKind GetKind() const
    {
        return kind;
    }

    void AddLabel(LabelIdx idx)
    {
        labIdx = idx;
    }

    void AppendBB(BB &bb)
    {
        bb.prev = this;
        bb.next = next;
        if (next != nullptr) {
            next->prev = &bb;
        }
        succsProb[&bb] = kUnknownProb;
        next = &bb;
    }

    void PrependBB(BB &bb)
    {
        bb.next = this;
        bb.prev = this->prev;
        if (this->prev != nullptr) {
            this->prev->next = &bb;
        }
        this->prev = &bb;
        succsProb[&bb] = kUnknownProb;
    }

    Insn *InsertInsnBefore(Insn &existing, Insn &newInsn);

    /* returns newly inserted instruction */
    Insn *InsertInsnAfter(Insn &existing, Insn &newInsn);

    void InsertInsnBegin(Insn &insn)
    {
        if (lastInsn == nullptr) {
            firstInsn = lastInsn = &insn;
            insn.SetNext(nullptr);
            insn.SetPrev(nullptr);
            insn.SetBB(this);
        } else {
            InsertInsnBefore(*firstInsn, insn);
        }
    }

    void AppendInsn(Insn &insn)
    {
        if (firstInsn != nullptr && lastInsn != nullptr) {
            InsertInsnAfter(*lastInsn, insn);
        } else {
            firstInsn = lastInsn = &insn;
            insn.SetNext(nullptr);
            insn.SetPrev(nullptr);
            insn.SetBB(this);
        }
        internalFlag1++;
    }

    void ReplaceInsn(Insn &insn, Insn &newInsn);

    void RemoveInsn(Insn &insn);

    void RemoveInsnSequence(Insn &insn, const Insn &nextInsn);

    /* append all insns from bb into this bb */
    void AppendBBInsns(BB &bb);

    /* append all insns from bb into this bb */
    void InsertAtBeginning(BB &bb);
    void InsertAtEnd(BB &bb);

    /* clear BB but don't remove insns of this */
    void ClearInsns()
    {
        firstInsn = lastInsn = nullptr;
    }

    uint32 NumPreds() const
    {
        return static_cast<uint32>(preds.size());
    }

    bool IsPredecessor(const BB &predBB)
    {
        for (const BB *bb : preds) {
            if (bb == &predBB) {
                return true;
            }
        }
        return false;
    }

    void RemoveFromPredecessorList(const BB &bb)
    {
        for (auto i = preds.begin(); i != preds.end(); ++i) {
            if (*i == &bb) {
                preds.erase(i);
                return;
            }
        }
        CHECK_FATAL(false, "request to remove a non-existent element?");
    }

    void RemoveFromSuccessorList(const BB &bb)
    {
        for (auto i = succs.begin(); i != succs.end(); ++i) {
            if (*i == &bb) {
                succs.erase(i);
                succsProb.erase(&bb);
                return;
            }
        }
        CHECK_FATAL(false, "request to remove a non-existent element?");
    }

    uint32 NumSuccs() const
    {
        return static_cast<uint32>(succs.size());
    }

    bool HasCall() const
    {
        return hasCall;
    }

    void SetHasCall()
    {
        hasCall = true;
    }

    /* Number of instructions excluding DbgInsn and comments */
    int32 NumInsn() const;
    uint32 GetId() const
    {
        return id;
    }
    uint32 GetID() const
    {
        return id;
    }
    uint32 GetLevel() const
    {
        return level;
    }
    void SetLevel(uint32 arg)
    {
        level = arg;
    }
    uint32 GetFrequency() const
    {
        return frequency;
    }
    void SetFrequency(uint32 arg)
    {
        frequency = arg;
    }
    bool IsInColdSection() const
    {
        return inColdSection;
    }
    void SetColdSection()
    {
        inColdSection = true;
    }
    BB *GetNext()
    {
        return next;
    }
    const BB *GetNext() const
    {
        return next;
    }
    BB *GetPrev()
    {
        return prev;
    }
    const BB *GetPrev() const
    {
        return prev;
    }
    void SetNext(BB *arg)
    {
        next = arg;
    }
    void SetPrev(BB *arg)
    {
        prev = arg;
    }
    LabelIdx GetLabIdx() const
    {
        return labIdx;
    }
    void SetLabIdx(LabelIdx arg)
    {
        labIdx = arg;
    }
    StmtNode *GetFirstStmt()
    {
        return firstStmt;
    }
    const StmtNode *GetFirstStmt() const
    {
        return firstStmt;
    }
    void SetFirstStmt(StmtNode &arg)
    {
        firstStmt = &arg;
    }
    StmtNode *GetLastStmt()
    {
        return lastStmt;
    }
    const StmtNode *GetLastStmt() const
    {
        return lastStmt;
    }
    void SetLastStmt(StmtNode &arg)
    {
        lastStmt = &arg;
    }
    Insn *GetFirstInsn()
    {
        return firstInsn;
    }
    const Insn *GetFirstInsn() const
    {
        return firstInsn;
    }

    void SetFirstInsn(Insn *arg)
    {
        firstInsn = arg;
    }
    Insn *GetFirstMachineInsn()
    {
        FOR_BB_INSNS(insn, this) {
            if (insn->IsMachineInstruction()) {
                return insn;
            }
        }
        return nullptr;
    }

    const Insn *GetFirstMachineInsn() const
    {
        FOR_BB_INSNS_CONST(insn, this) {
            if (insn->IsMachineInstruction()) {
                return insn;
            }
        }
        return nullptr;
    }

    Insn *GetLastMachineInsn()
    {
        FOR_BB_INSNS_REV(insn, this) {
            if (insn->IsMachineInstruction() && !insn->IsPseudo()) {
                return insn;
            }
        }
        return nullptr;
    }

    const Insn *GetLastMachineInsn() const
    {
        FOR_BB_INSNS_REV_CONST(insn, this) {
            if (insn->IsMachineInstruction() && !insn->IsPseudo()) {
                return insn;
            }
        }
        return nullptr;
    }

    Insn *GetLastInsn()
    {
        return lastInsn;
    }
    const Insn *GetLastInsn() const
    {
        return lastInsn;
    }
    void SetLastInsn(Insn *arg)
    {
        lastInsn = arg;
    }
    bool IsLastInsn(const Insn *insn) const
    {
        return (lastInsn == insn);
    }
    void InsertPred(const MapleList<BB *>::iterator &it, BB &bb)
    {
        preds.insert(it, &bb);
    }
    void InsertSucc(const MapleList<BB *>::iterator &it, BB &bb, int32 prob = kUnknownProb)
    {
        succs.insert(it, &bb);
        succsProb[&bb] = prob;
    }
    const MapleList<BB *> &GetPreds() const
    {
        return preds;
    }
    std::size_t GetPredsSize() const
    {
        return preds.size();
    }
    const MapleList<BB *> &GetSuccs() const
    {
        return succs;
    }
    std::size_t GetSuccsSize() const
    {
        return succs.size();
    }

    // override interface of BaseGraphNode
    const std::string GetIdentity() final
    {
        return "BBId: " + std::to_string(GetID());
    }
    void GetOutNodes(std::vector<BaseGraphNode *> &outNodes) const final
    {
        outNodes.resize(succs.size(), nullptr);
        std::copy(succs.begin(), succs.end(), outNodes.begin());
    }

    void GetOutNodes(std::vector<BaseGraphNode *> &outNodes) final
    {
        static_cast<const BB *>(this)->GetOutNodes(outNodes);
    }

    void GetInNodes(std::vector<BaseGraphNode *> &inNodes) const final
    {
        inNodes.resize(preds.size(), nullptr);
        std::copy(preds.begin(), preds.end(), inNodes.begin());
    }

    void GetInNodes(std::vector<BaseGraphNode *> &inNodes) final
    {
        static_cast<const BB *>(this)->GetInNodes(inNodes);
    }
    MapleList<BB *>::iterator GetPredsBegin()
    {
        return preds.begin();
    }
    MapleList<BB *>::iterator GetSuccsBegin()
    {
        return succs.begin();
    }
    MapleList<BB *>::iterator GetPredsEnd()
    {
        return preds.end();
    }
    MapleList<BB *>::iterator GetSuccsEnd()
    {
        return succs.end();
    }
    void PushBackPreds(BB &bb)
    {
        preds.push_back(&bb);
    }
    void PushBackSuccs(BB &bb, int32 prob = kUnknownProb)
    {
        succs.push_back(&bb);
        succsProb[&bb] = prob;
    }
    void PushFrontPreds(BB &bb)
    {
        preds.push_front(&bb);
    }
    void PushFrontSuccs(BB &bb, int32 prob = kUnknownProb)
    {
        succs.push_front(&bb);
        succsProb[&bb] = prob;
    }
    void ErasePreds(MapleList<BB *>::iterator it)
    {
        preds.erase(it);
    }
    void EraseSuccs(MapleList<BB *>::const_iterator it)
    {
        succs.erase(it);
    }
    void RemovePreds(BB &bb)
    {
        preds.remove(&bb);
    }
    void RemoveSuccs(BB &bb)
    {
        succs.remove(&bb);
        succsProb.erase(&bb);
    }
    void ReplaceSucc(const MapleList<BB *>::const_iterator it, BB &newBB)
    {
        int prob = succsProb[*it];
        EraseSuccs(it);
        PushBackSuccs(newBB, prob);
    }

    void ReplaceSucc(BB &oldBB, BB &newBB)
    {
        int prob = succsProb[&oldBB];
        RemoveSuccs(oldBB);
        PushBackSuccs(newBB, prob);
    }
    void ClearPreds()
    {
        preds.clear();
    }
    void ClearSuccs()
    {
        succs.clear();
        succsProb.clear();
    }
    const MapleSet<regno_t> &GetLiveInRegNO() const
    {
        return liveInRegNO;
    }
    MapleSet<regno_t> &GetLiveInRegNO()
    {
        return liveInRegNO;
    }
    void InsertLiveInRegNO(regno_t arg)
    {
        (void)liveInRegNO.insert(arg);
    }
    void EraseLiveInRegNO(MapleSet<regno_t>::iterator it)
    {
        liveInRegNO.erase(it);
    }
    void EraseLiveInRegNO(regno_t arg)
    {
        liveInRegNO.erase(arg);
    }
    void ClearLiveInRegNO()
    {
        liveInRegNO.clear();
    }
    const MapleSet<regno_t> &GetLiveOutRegNO() const
    {
        return liveOutRegNO;
    }
    MapleSet<regno_t> &GetLiveOutRegNO()
    {
        return liveOutRegNO;
    }
    void InsertLiveOutRegNO(regno_t arg)
    {
        (void)liveOutRegNO.insert(arg);
    }
    void EraseLiveOutRegNO(MapleSet<regno_t>::iterator it)
    {
        liveOutRegNO.erase(it);
    }
    void ClearLiveOutRegNO()
    {
        liveOutRegNO.clear();
    }
    bool GetLiveInChange() const
    {
        return liveInChange;
    }
    void SetLiveInChange(bool arg)
    {
        liveInChange = arg;
    }
    bool GetCritical() const
    {
        return isCritical;
    }
    void SetCritical(bool arg)
    {
        isCritical = arg;
    }
    bool GetInsertUse() const
    {
        return insertUse;
    }
    void SetInsertUse(bool arg)
    {
        insertUse = arg;
    }
    bool IsUnreachable() const
    {
        return unreachable;
    }
    void SetUnreachable(bool arg)
    {
        unreachable = arg;
    }
    bool IsWontExit() const
    {
        return wontExit;
    }
    void SetWontExit(bool arg)
    {
        wontExit = arg;
    }
    void SetFastPath(bool arg)
    {
        fastPath = arg;
    }
    bool IsCatch() const
    {
        return isCatch;
    }
    void SetIsCatch(bool arg)
    {
        isCatch = arg;
    }
    bool IsCleanup() const
    {
        return isCleanup;
    }
    void SetIsCleanup(bool arg)
    {
        isCleanup = arg;
    }
    bool IsProEpilog() const
    {
        return isProEpilog;
    }
    void SetIsProEpilog(bool arg)
    {
        isProEpilog = arg;
    }
    bool IsLabelTaken() const
    {
        return labelTaken;
    }
    void SetLabelTaken()
    {
        labelTaken = true;
    }
    bool GetHasCfi() const
    {
        return hasCfi;
    }
    void SetHasCfi()
    {
        hasCfi = true;
    }
    long GetInternalFlag1() const
    {
        return internalFlag1;
    }
    void SetInternalFlag1(long arg)
    {
        internalFlag1 = arg;
    }
    long GetInternalFlag2() const
    {
        return internalFlag2;
    }
    void SetInternalFlag2(long arg)
    {
        internalFlag2 = arg;
    }
    long GetInternalFlag3() const
    {
        return internalFlag3;
    }
    void SetInternalFlag3(long arg)
    {
        internalFlag3 = arg;
    }
    bool IsAtomicBuiltInBB() const
    {
        return isAtomicBuiltIn;
    }
    void SetAtomicBuiltIn()
    {
        isAtomicBuiltIn = true;
    }
    const MapleList<Insn *> &GetCallInsns() const
    {
        return callInsns;
    }
    void PushBackCallInsns(Insn &insn)
    {
        callInsns.push_back(&insn);
    }
    void ClearCallInsns()
    {
        callInsns.clear();
    }
    const MapleVector<LabelIdx> &GetRangeGotoLabelVec() const
    {
        return rangeGotoLabelVec;
    }
    void SetRangeGotoLabel(uint32 index, LabelIdx labelIdx)
    {
        rangeGotoLabelVec[index] = labelIdx;
    }
    void PushBackRangeGotoLabel(LabelIdx labelIdx)
    {
        rangeGotoLabelVec.emplace_back(labelIdx);
    }
    void AddPhiInsn(regno_t regNO, Insn &insn)
    {
        DEBUG_ASSERT(!phiInsnList.count(regNO), "repeat phiInsn");
        phiInsnList.emplace(std::pair<regno_t, Insn *>(regNO, &insn));
    }
    void RemovePhiInsn(regno_t regNO)
    {
        DEBUG_ASSERT(phiInsnList.count(regNO), "no such insn");
        phiInsnList.erase(regNO);
    }
    bool HasPhiInsn(regno_t regNO)
    {
        return phiInsnList.find(regNO) != phiInsnList.end();
    }
    MapleMap<regno_t, Insn *> &GetPhiInsns()
    {
        return phiInsnList;
    }
    const Insn *GetFirstLoc() const
    {
        return firstLoc;
    }
    void SetFirstLoc(const Insn &arg)
    {
        firstLoc = &arg;
    }
    const Insn *GetLastLoc() const
    {
        return lastLoc;
    }
    void SetLastLoc(const Insn *arg)
    {
        lastLoc = arg;
    }
    SparseDataInfo *GetLiveIn()
    {
        return liveIn;
    }
    const SparseDataInfo *GetLiveIn() const
    {
        return liveIn;
    }
    void SetLiveIn(SparseDataInfo &arg)
    {
        liveIn = &arg;
    }
    void SetLiveInBit(uint32 arg) const
    {
        liveIn->SetBit(arg);
    }
    void SetLiveInInfo(const SparseDataInfo &arg) const
    {
        *liveIn = arg;
    }
    bool LiveInOrBits(const SparseDataInfo &arg) const
    {
        return liveIn->OrBits(arg);
    }
    void LiveInEnlargeCapacity(uint32 arg) const
    {
        liveIn->EnlargeCapacityToAdaptSize(arg);
    }
    void LiveInClearDataInfo()
    {
        liveIn->ClearDataInfo();
        liveIn = nullptr;
    }
    SparseDataInfo *GetLiveOut()
    {
        return liveOut;
    }
    const SparseDataInfo *GetLiveOut() const
    {
        return liveOut;
    }
    void SetLiveOut(SparseDataInfo &arg)
    {
        liveOut = &arg;
    }
    void SetLiveOutBit(uint32 arg) const
    {
        liveOut->SetBit(arg);
    }
    bool LiveOutOrBits(const SparseDataInfo &arg) const
    {
        return liveOut->OrBits(arg);
    }
    void LiveOutEnlargeCapacity(uint32 arg) const
    {
        liveOut->EnlargeCapacityToAdaptSize(arg);
    }
    void LiveOutClearDataInfo()
    {
        liveOut->ClearDataInfo();
        liveOut = nullptr;
    }
    const SparseDataInfo *GetDef() const
    {
        return def;
    }
    void SetDef(SparseDataInfo &arg)
    {
        def = &arg;
    }
    void SetDefBit(uint32 arg) const
    {
        def->SetBit(arg);
    }
    void DefResetAllBit() const
    {
        def->ResetAllBit();
    }
    void DefResetBit(uint32 arg) const
    {
        def->ResetBit(arg);
    }
    void DefClearDataInfo()
    {
        def->ClearDataInfo();
        def = nullptr;
    }
    const SparseDataInfo *GetUse() const
    {
        return use;
    }
    void SetUse(SparseDataInfo &arg)
    {
        use = &arg;
    }
    void SetUseBit(uint32 arg) const
    {
        use->SetBit(arg);
    }
    void UseResetAllBit() const
    {
        use->ResetAllBit();
    }
    void UseResetBit(uint32 arg) const
    {
        use->ResetBit(arg);
    }
    void UseClearDataInfo()
    {
        use->ClearDataInfo();
        use = nullptr;
    }
    void SetNeedAlign(bool flag)
    {
        needAlign = flag;
    }
    bool IsBBNeedAlign() const
    {
        return needAlign;
    }
    void SetAlignPower(uint32 power)
    {
        alignPower = power;
    }
    uint32 GetAlignPower() const
    {
        return alignPower;
    }
    void SetAlignNopNum(uint32 num)
    {
        alignNopNum = num;
    }
    uint32 GetAlignNopNum() const
    {
        return alignNopNum;
    }

    CDGNode *GetCDGNode()
    {
        return cdgNode;
    }
    void SetCDGNode(CDGNode *node)
    {
        cdgNode = node;
    }

    // Check if a given BB mergee can be merged into this BB in the sense of control flow
    // The condition is looser than CanMerge, since we only need this for debug info.
    bool MayFoldInCfg(const BB &mergee) const
    {
        bool onePred = (mergee.GetPreds().size() == 1) && (mergee.GetPreds().front() == this);
        bool oneSucc = (GetSuccs().size() == 1) && (GetSuccs().front() == &mergee);
        return oneSucc && onePred;
    }

    // This means two bb are close to each other in cfg. Only for debug info.
    // May add more conditions in the future.
    bool IsCloseTo(const BB &tar) const
    {
        return MayFoldInCfg(tar);
    }

    int32 GetEdgeProb(const BB &bb) const
    {
        return succsProb.find(&bb)->second;
    }

    bool HasMachineInsn()
    {
        FOR_BB_INSNS(insn, this) {
            if (insn->IsMachineInstruction()) {
                return true;
            }
        }
        return false;
    }

    SCCNode<BB> *GetSCCNode()
    {
        return sccNode;
    }
    void SetSCCNode(SCCNode<BB> *scc)
    {
        sccNode = scc;
    }

private:
    static const std::string bbNames[kBBLast];
    uint32 id;
    uint32 level = 0;
    uint32 frequency = 0;
    BB *prev = nullptr; /* Doubly linked list of BBs; */
    BB *next = nullptr;
    /* They represent the order in which blocks are to be emitted. */
    BBKind kind = kBBFallthru; /* The BB's last statement (i.e. lastStmt) determines */
    /* what type this BB has. By default, kBbFallthru */
    LabelIdx labIdx;
    StmtNode *firstStmt = nullptr;
    StmtNode *lastStmt = nullptr;
    Insn *firstInsn = nullptr; /* the first instruction */
    Insn *lastInsn = nullptr;  /* the last instruction */
    MapleList<BB *> preds;     /* preds, succs represent CFG */
    MapleList<BB *> succs;
    MapleMap<const BB *, int32> succsProb;
    bool inColdSection = false; /* for bb splitting */

    /* this is for live in out analysis */
    MapleSet<regno_t> liveInRegNO;
    MapleSet<regno_t> liveOutRegNO;
    bool liveInChange = false;
    bool isCritical = false;
    bool insertUse = false;
    bool hasCall = false;
    bool unreachable = false;
    bool wontExit = false;
    bool fastPath = false;
    bool isCatch = false; /* part of the catch bb, true does might also mean it is unreachable */
    /*
     * Since isCatch is set early and unreachable detected later, there
     * are some overlap here.
     */
    bool isCleanup = false;   /* true if the bb is cleanup bb. otherwise, false. */
    bool isProEpilog = false; /* Temporary tag for modifying prolog/epilog bb. */
    bool labelTaken = false;  /* Block label is taken indirectly and can be used to jump to it. */
    bool hasCfi = false;      /* bb contain cfi directive. */
    /*
     * Different meaning for each data flow analysis.
     * For HandleFunction(), rough estimate of num of insn created.
     * For cgbb.cpp, track insn count during code selection.
     * For cgbb.cpp, bb is traversed during BFS ordering.
     * For aarchregalloc.cpp, the bb is part of cleanup at end of function.
     * For aarchcolorra.cpp, the bb is part of cleanup at end of function.
     *                       also used for live range splitting.
     * For live analysis, it indicates if bb is cleanupbb.
     */
    long internalFlag1 = 0;

    /*
     * Different meaning for each data flow analysis.
     * For cgbb.cpp, bb is levelized to be 1 more than largest predecessor.
     * For aarchcolorra.cpp, used for live range splitting pruning of bb.
     */
    long internalFlag2 = 0;

    /*
     * Different meaning for each data flow analysis.
     * For cgfunc.cpp, it temporarily marks for catch bb discovery.
     * For live analysis, it indicates if bb is visited.
     * For peephole, used for live-out checking of bb.
     */
    long internalFlag3 = 0;
    MapleList<Insn *> callInsns;
    MapleVector<LabelIdx> rangeGotoLabelVec;

    /* bb support for SSA analysis */
    MapleMap<regno_t, Insn *> phiInsnList;

    /* includes Built-in functions for atomic memory access */
    bool isAtomicBuiltIn = false;

    const Insn *firstLoc = nullptr;
    const Insn *lastLoc = nullptr;
    SparseDataInfo *liveIn = nullptr;
    SparseDataInfo *liveOut = nullptr;
    SparseDataInfo *def = nullptr;
    SparseDataInfo *use = nullptr;

    bool needAlign = false;
    uint32 alignPower = 0;
    uint32 alignNopNum = 0;

    CDGNode *cdgNode = nullptr;
    SCCNode<BB> *sccNode = nullptr;
};                             /* class BB */

struct BBIdCmp {
    bool operator()(const BB *lhs, const BB *rhs) const
    {
        CHECK_FATAL(lhs != nullptr, "null ptr check");
        CHECK_FATAL(rhs != nullptr, "null ptr check");
        return (lhs->GetId() < rhs->GetId());
    }
};

class Bfs {
public:
    Bfs(CGFunc &cgFunc, MemPool &memPool)
        : cgfunc(&cgFunc),
          memPool(&memPool),
          alloc(&memPool),
          cycleSuccs(alloc.Adapter()),
          visitedBBs(alloc.Adapter()),
          sortedBBs(alloc.Adapter())
    {
    }
    ~Bfs() = default;

    void SeekCycles();
    bool AllPredBBVisited(const BB &bb, long &level) const;
    BB *MarkStraightLineBBInBFS(BB *bb);
    BB *SearchForStraightLineBBs(BB &bb);
    void BFS(BB &curBB);
    void ComputeBlockOrder();

    CGFunc *cgfunc;
    MemPool *memPool;
    MapleAllocator alloc;
    MapleVector<MapleSet<BBID>> cycleSuccs;  // bb's succs in cycle
    MapleVector<bool> visitedBBs;
    MapleVector<BB *> sortedBBs;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgBBSort, CGFunc)
Bfs *GetResult()
{
    return bfs;
}
Bfs *bfs = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_MODULE_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_CGBB_H */
