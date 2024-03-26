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

#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_LSRA_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_LSRA_H
#include "reg_alloc.h"
#include "cgfunc.h"
#include "optimize_common.h"
#include "loop.h"

namespace maplebe {
class LSRALinearScanRegAllocator : public RegAllocator {
    enum RegInCatch : uint8 {
        /*
         * RA do not want to allocate certain registers if a live interval is
         * only inside of catch blocks.
         */
        kRegCatchNotInit = 0, /* unitialized state */
        kRegNOtInCatch = 1,   /* interval is part or all outside of catch */
        kRegAllInCatch = 2,   /* inteval is completely inside catch */
    };

    enum RegInCleanup : uint8 {
        /* Similar to reg_in_catch_t */
        kRegCleanupNotInit = 0,       /* unitialized state */
        kRegAllInFirstbb = 1,         /* interval is all in the first bb */
        kRegAllOutCleanup = 2,        /* interval is all outside of cleanup, must in normal bb, may in first bb. */
        kRegInCleanupAndFirstbb = 3,  /* inteval is in cleanup and first bb. */
        kRegInCleanupAndNormalbb = 4, /* inteval is in cleanup and non-first bb. */
        kRegAllInCleanup = 5          /* inteval is inside cleanup, except for bb 1 */
    };

    class LinearRange {
    public:
        LinearRange() = default;

        LinearRange(uint32 start, uint32 end) : start(start), end(end) {}

        ~LinearRange() = default;

        uint32 GetStart() const
        {
            return start;
        }

        void SetStart(uint32 position)
        {
            start = position;
        }

        uint32 GetEnd() const
        {
            return end;
        }

        void SetEnd(uint32 position)
        {
            end = position;
        }

        uint32 GetEhStart() const
        {
            return ehStart;
        }

        void SetEhStart(uint32 position)
        {
            ehStart = position;
        }
    private:
        uint32 start = 0;
        uint32 end = 0;
        uint32 ehStart = 0;
    };

    class LiveInterval {
    public:
        explicit LiveInterval(MemPool &memPool)
            : memPool(&memPool),
              alloc(&memPool),
              ranges(alloc.Adapter()),
              usePositions(alloc.Adapter()),
              noReloadPos(alloc.Adapter()) {}

        virtual ~LiveInterval() = default;

        void AddRange(uint32 from, uint32 to);
        void AddUsePos(uint32 pos);

        LiveInterval *SplitAt(uint32 pos);

        LiveInterval *SplitBetween(const BB &startBB, const BB &endBB);

        void InitRangeFinder()
        {
            rangeFinder = ranges.begin();
        }

        MapleVector<LinearRange>::iterator FindPosRange(uint32 pos);

        uint32 GetSplitPos() const
        {
            return splitSafePos;
        }

        void SetSplitPos(uint32 position)
        {
            splitSafePos = position;
        }

        const Insn *GetIsCall() const
        {
            return isCall;
        }

        void SetIsCall(Insn &newIsCall)
        {
            isCall = &newIsCall;
        }

        uint32 GetPhysUse() const
        {
            return physUse;
        }

        void SetPhysUse(uint32 newPhysUse)
        {
            physUse = newPhysUse;
        }

        uint32 GetLastUse() const
        {
            return lastUse;
        }

        void SetLastUse(uint32 newLastUse)
        {
            lastUse = newLastUse;
        }

        uint32 GetRegNO() const
        {
            return regNO;
        }

        void SetRegNO(uint32 newRegNO)
        {
            regNO = newRegNO;
        }

        uint32 GetAssignedReg() const
        {
            return assignedReg;
        }

        void SetAssignedReg(uint32 newAssignedReg)
        {
            assignedReg = newAssignedReg;
        }

        uint32 GetFirstDef() const
        {
            return firstDef;
        }

        void SetFirstDef(uint32 newFirstDef)
        {
            firstDef = newFirstDef;
        }

        uint32 GetStackSlot() const
        {
            return stackSlot;
        }

        void SetStackSlot(uint32 newStkSlot)
        {
            stackSlot = newStkSlot;
        }

        RegType GetRegType() const
        {
            return regType;
        }

        void SetRegType(RegType newRegType)
        {
            regType = newRegType;
        }

        uint32 GetFirstAcrossedCall() const
        {
            return firstAcrossedCall;
        }

        void SetFirstAcrossedCall(uint32 newFirstAcrossedCall)
        {
            firstAcrossedCall = newFirstAcrossedCall;
        }

        bool IsEndByCall() const
        {
            return endByCall;
        }

        void SetEndByMov(bool isEndByMov)
        {
            endByMov = isEndByMov;
        }

        bool IsEndByMov() const
        {
            return endByMov;
        }

        void SetPrefer(uint32 preg)
        {
            prefer = preg;
        }

        uint32 GetPrefer() const
        {
            return prefer;
        }

        bool IsUseBeforeDef() const
        {
            return useBeforeDef;
        }

        void SetUseBeforeDef(bool newUseBeforeDef)
        {
            useBeforeDef = newUseBeforeDef;
        }

        bool IsShouldSave() const
        {
            return shouldSave;
        }

        void SetShouldSave(bool newShouldSave)
        {
            shouldSave = newShouldSave;
        }

        bool IsMultiUseInBB() const
        {
            return multiUseInBB;
        }

        void SetMultiUseInBB(bool newMultiUseInBB)
        {
            multiUseInBB = newMultiUseInBB;
        }

        bool IsThrowVal() const
        {
            return isThrowVal;
        }

        bool IsCallerSpilled() const
        {
            return isCallerSpilled;
        }

        void SetIsCallerSpilled(bool newIsCallerSpilled)
        {
            isCallerSpilled = newIsCallerSpilled;
        }

        bool IsMustAllocate() const
        {
            return mustAllocate;
        }

        void SetMustAllocate(bool newMustAllocate)
        {
            mustAllocate = newMustAllocate;
        }

        bool IsSplitForbidden() const
        {
            return splitForbidden;
        }

        void SetSplitForbid(bool isForbidden)
        {
            splitForbidden = isForbidden;
        }

        uint32 GetRefCount() const
        {
            return refCount;
        }

        void SetRefCount(uint32 newRefCount)
        {
            refCount = newRefCount;
        }

        void AddUsePositions(uint32 insertId)
        {
            (void)usePositions.push_back(insertId);
        }

        const MapleVector<uint32> &GetUsePositions() const
        {
            return usePositions;
        }

        LiveInterval *GetSplitNext()
        {
            return splitNext;
        }

        const LiveInterval *GetSplitNext() const
        {
            return splitNext;
        }

        const LiveInterval *GetSplitParent() const
        {
            return splitParent;
        }
        void SetSplitParent(LiveInterval &li)
        {
            splitParent = &li;
        }

        float GetPriority() const
        {
            return priority;
        }

        void SetPriority(float newPriority)
        {
            priority = newPriority;
        }

        const MapleVector<LinearRange> &GetRanges() const
        {
            return ranges;
        }

        MapleVector<LinearRange> &GetRanges()
        {
            return ranges;
        }

        size_t GetRangesSize() const
        {
            return ranges.size();
        }

        const LiveInterval *GetLiParent() const
        {
            return liveParent;
        }

        void SetLiParent(LiveInterval *newLiParent)
        {
            liveParent = newLiParent;
        }

        void SetLiParentChild(LiveInterval *child) const
        {
            liveParent->SetLiChild(child);
        }

        const LiveInterval *GetLiChild() const
        {
            return liveChild;
        }

        void SetLiChild(LiveInterval *newLiChild)
        {
            liveChild = newLiChild;
        }

        uint32 GetResultCount() const
        {
            return resultCount;
        }

        void SetResultCount(uint32 newResultCount)
        {
            resultCount = newResultCount;
        }

        void SetInCatchState()
        {
            /*
             * Once in REG_NOT_IN_CATCH, it is irreversible since once an interval
             * is not in a catch, it is not completely in a catch.
             */
            if (inCatchState == kRegNOtInCatch) {
                return;
            }
            inCatchState = kRegAllInCatch;
        }

        void SetNotInCatchState()
        {
            inCatchState = kRegNOtInCatch;
        }

        bool IsAllInCatch() const
        {
            return (inCatchState == kRegAllInCatch);
        }

        void SetInCleanupState()
        {
            switch (inCleanUpState) {
                case kRegCleanupNotInit:
                    inCleanUpState = kRegAllInCleanup;
                    break;
                case kRegAllInFirstbb:
                    inCleanUpState = kRegInCleanupAndFirstbb;
                    break;
                case kRegAllOutCleanup:
                    inCleanUpState = kRegInCleanupAndNormalbb;
                    break;
                case kRegInCleanupAndFirstbb:
                    break;
                case kRegInCleanupAndNormalbb:
                    break;
                case kRegAllInCleanup:
                    break;
                default:
                    DEBUG_ASSERT(false, "CG Internal error.");
                    break;
            }
        }

        void SetNotInCleanupState(bool isFirstBB)
        {
            switch (inCleanUpState) {
                case kRegCleanupNotInit: {
                    if (isFirstBB) {
                        inCleanUpState = kRegAllInFirstbb;
                    } else {
                        inCleanUpState = kRegAllOutCleanup;
                    }
                    break;
                }
                case kRegAllInFirstbb: {
                    if (!isFirstBB) {
                        inCleanUpState = kRegAllOutCleanup;
                    }
                    break;
                }
                case kRegAllOutCleanup:
                    break;
                case kRegInCleanupAndFirstbb: {
                    if (!isFirstBB) {
                        inCleanUpState = kRegInCleanupAndNormalbb;
                    }
                    break;
                }
                case kRegInCleanupAndNormalbb:
                    break;
                case kRegAllInCleanup: {
                    if (isFirstBB) {
                        inCleanUpState = kRegInCleanupAndFirstbb;
                    } else {
                        inCleanUpState = kRegInCleanupAndNormalbb;
                    }
                    break;
                }
                default:
                    DEBUG_ASSERT(false, "CG Internal error.");
                    break;
            }
        }

        bool IsAllInCleanupOrFirstBB() const
        {
            return (inCleanUpState == kRegAllInCleanup) || (inCleanUpState == kRegInCleanupAndFirstbb);
        }

        bool IsAllOutCleanup() const
        {
            return (inCleanUpState == kRegAllInFirstbb) || (inCleanUpState == kRegAllOutCleanup);
        }

        uint32 GetSpillSize() const
        {
            return spillSize;
        }

        void SetSpillSize(uint32 size)
        {
            spillSize = size;
        }

        uint32 GetMaxDefSize() const
        {
            return maxDefSize;
        }

        void SetMaxDefSize(uint32 size)
        {
            maxDefSize = size;
        }

        uint32 GetMaxUseSize() const
        {
            return maxUseSize;
        }

        void SetMaxUseSize(uint32 size)
        {
            maxUseSize = size;
        }

        bool IsNoNeedReloadPosition(uint32 insnId) const
        {
            return (noReloadPos.find(insnId) != noReloadPos.end());
        }

        void AddNoNeedReloadPosition(uint32 insnId)
        {
            noReloadPos.insert(insnId);
        }
    private:
        MemPool *memPool;
        MapleAllocator alloc;
        Insn *isCall = nullptr;
        uint32 firstDef = 0;
        uint32 lastUse = 0;
        uint32 splitSafePos = 0; /* splitNext's start positon */
        uint32 physUse = 0;
        uint32 regNO = 0;
        /* physical register, using cg defined reg based on R0/V0. */
        uint32 assignedReg = 0;
        uint32 stackSlot = -1;
        RegType regType = kRegTyUndef;
        uint32 spillSize = 0;               /* use min(maxDefSize, maxUseSize) */
        uint32 maxDefSize = 0;
        uint32 maxUseSize = 0;
        uint32 firstAcrossedCall = 0;
        bool endByCall = false;
        bool endByMov = false; /* do move coalesce */
        uint32 prefer = 0;     /* prefer register */
        bool useBeforeDef = false;
        bool shouldSave = false;
        bool multiUseInBB = false; /* vreg has more than 1 use in bb */
        bool isThrowVal = false;
        bool isCallerSpilled = false; /* only for R0(R1?) which are used for explicit incoming value of throwval; */
        bool mustAllocate = false;    /* The register cannot be spilled (clinit pair) */
        bool splitForbidden = false;  /* can not split if true */
        uint32 refCount = 0;
        float priority = 0.0;
        MapleVector<LinearRange> ranges;
        MapleVector<LinearRange>::iterator rangeFinder;
        MapleVector<uint32> usePositions;
        LiveInterval *splitNext = nullptr;         /* next split part */
        LiveInterval *splitParent = nullptr;       /* parent split part */
        LiveInterval *liveParent = nullptr;        /* Current li is in aother li's hole. */
        LiveInterval *liveChild = nullptr;         /* Another li is in current li's hole. */
        uint32 resultCount = 0;                    /* number of times this vreg has been written */
        uint8 inCatchState = kRegCatchNotInit;     /* part or all of live interval is outside of catch blocks */
        uint8 inCleanUpState = kRegCleanupNotInit; /* part or all of live interval is outside of cleanup blocks */
        MapleUnorderedSet<uint32> noReloadPos;       /* Should save reg need reload at this positions */
    };

    /* used to resolve Phi and Split */
    class MoveInfo {
    public:
        MoveInfo() = default;
        MoveInfo(LiveInterval &fromLi, LiveInterval &toLi, BB &bb, bool isEnd)
            : fromLi(&fromLi), toLi(&toLi), bb(&bb), isEnd(isEnd)
        {
        }
        ~MoveInfo() = default;

        void Init(LiveInterval &firstLi, LiveInterval &secondLi, BB &targetBB, bool endMove)
        {
            fromLi = &firstLi;
            toLi = &secondLi;
            bb = &targetBB;
            isEnd = endMove;
        }

        const LiveInterval *GetFromLi() const
        {
            return fromLi;
        }

        const LiveInterval *GetToLi() const
        {
            return toLi;
        }

        BB *GetBB()
        {
            return bb;
        }

        const BB *GetBB() const
        {
            return bb;
        }

        bool IsEndMove() const
        {
            return isEnd;
        }

        void Dump() const
        {
            LogInfo::MapleLogger() << "from:R" << fromLi->GetRegNO() << " to:R" << toLi->GetRegNO() << " in "
                                   << bb->GetId() << "\n";
        }

    private:
        LiveInterval *fromLi = nullptr;
        LiveInterval *toLi = nullptr;
        BB *bb = nullptr;
        bool isEnd = true;
    };

    struct ActiveCmp {
        bool operator()(const LiveInterval *lhs, const LiveInterval *rhs) const
        {
            CHECK_NULL_FATAL(lhs);
            CHECK_NULL_FATAL(rhs);
            /* elements considered equal if return false */
            if (lhs == rhs) {
                return false;
            }
            if (lhs->GetFirstDef() == rhs->GetFirstDef() && lhs->GetLastUse() == rhs->GetLastUse() &&
                lhs->GetRegNO() == rhs->GetRegNO() && lhs->GetRegType() == rhs->GetRegType() &&
                lhs->GetAssignedReg() == rhs->GetAssignedReg()) {
                return false;
            }
            if (lhs->GetFirstDef() == rhs->GetFirstDef() && lhs->GetLastUse() == rhs->GetLastUse() &&
                lhs->GetPhysUse() == rhs->GetPhysUse() && lhs->GetRegType() == rhs->GetRegType()) {
                return lhs->GetRegNO() < rhs->GetRegNO();
            }
            if (lhs->GetPhysUse() != 0 && rhs->GetPhysUse() != 0) {
                if (lhs->GetFirstDef() == rhs->GetFirstDef()) {
                    return lhs->GetPhysUse() < rhs->GetPhysUse();
                } else {
                    return lhs->GetFirstDef() < rhs->GetFirstDef();
                }
            }
            /* At this point, lhs != rhs */
            if (lhs->GetLastUse() == rhs->GetLastUse()) {
                return lhs->GetFirstDef() <= rhs->GetFirstDef();
            }
            return lhs->GetLastUse() < rhs->GetLastUse();
        }
    };

    class CallerSaveOpt {
    public:
        CallerSaveOpt(const MapleVector<LiveInterval*> &liveIntervalsArray, Bfs *bbSort) : bfs(bbSort)
        {
            for (auto *li : liveIntervalsArray) {
                if (li != nullptr && li->IsShouldSave()) {
                    callerSaves.emplace_back(li);
                }
            }
            localDefLiveIn.resize(callerSaves.size());
            localDefAfterInsn.resize(callerSaves.size());
        }

        void Run();
    private:
        bool firstTime = true;
        Bfs *bfs = nullptr;
        std::vector<LiveInterval*> callerSaves;
        std::vector<std::unordered_set<uint32>> localDefLiveIn;         // bbid: which is defined in livein
        std::vector<std::unordered_map<uint32, uint32>> localDefAfterInsn;  // bbid: def-insnid

        bool UpdateLocalDefWithBBLiveIn(const BB &bb);
        void CollectCallerNoNeedReloadByInsn(const Insn &insn);
    };

public:
    LSRALinearScanRegAllocator(CGFunc &cgFunc, MemPool &memPool, Bfs *bbSort, LoopAnalysis &loop)
        : RegAllocator(cgFunc, memPool),
          loopInfo(loop),
          liveIntervalsArray(alloc.Adapter()),
          initialQue(alloc.Adapter()),
          intParamQueue(alloc.Adapter()),
          fpParamQueue(alloc.Adapter()),
          callQueue(alloc.Adapter()),
          active(alloc.Adapter()),
          freeUntilPos(alloc.Adapter()),
          intCallerRegSet(alloc.Adapter()),
          intCalleeRegSet(alloc.Adapter()),
          intParamRegSet(alloc.Adapter()),
          intSpillRegSet(alloc.Adapter()),
          fpCallerRegSet(alloc.Adapter()),
          fpCalleeRegSet(alloc.Adapter()),
          fpParamRegSet(alloc.Adapter()),
          fpSpillRegSet(alloc.Adapter()),
          bfs(bbSort),
          liQue(alloc.Adapter()),
          splitPosMap(alloc.Adapter()),
          splitInsnMap(alloc.Adapter()),
          moveInfoVec(alloc.Adapter()),
          derivedRef2Base(alloc.Adapter())
    {
        for (uint32 i = 0; i < regInfo->GetIntRegs().size(); ++i) {
            intParamQueue.push_back(initialQue);
        }
        for (uint32 i = 0; i < regInfo->GetFpRegs().size(); ++i) {
            fpParamQueue.push_back(initialQue);
        }
        firstIntReg = *regInfo->GetIntRegs().begin();
        firstFpReg = *regInfo->GetFpRegs().begin();
    }
    ~LSRALinearScanRegAllocator() override = default;

    bool AllocateRegisters() override;
    bool CheckForReg(Operand &opnd, const Insn &insn, const LiveInterval &li, regno_t regNO, bool isDef) const;
    void PrintRegSet(const MapleSet<uint32> &set, const std::string &str) const;
    void PrintLiveInterval(const LiveInterval &li, const std::string &str) const;
    void PrintLiveRanges(const LiveInterval &li) const;
    void PrintAllLiveRanges() const;
    void PrintLiveRangesGraph() const;
    void SpillStackMapInfo();
    void PrintParamQueue(const std::string &str);
    void PrintCallQueue(const std::string &str) const;
    void PrintActiveList(const std::string &str, uint32 len = 0) const;
    void PrintActiveListSimple() const;
    void PrintLiveIntervals() const;
    void DebugCheckActiveList() const;
    void InitFreeRegPool();
    void RecordPhysRegs(const RegOperand &regOpnd, uint32 insnNum, bool isDef);
    void UpdateLiveIntervalState(const BB &bb, LiveInterval &li) const;
    void UpdateRegUsedInfo(LiveInterval &li, regno_t regNO);
    void SetupLiveInterval(Operand &opnd, Insn &insn, bool isDef, uint32 &nUses, uint32 regSize);
    void UpdateLiveIntervalByLiveIn(const BB &bb, uint32 insnNum);
    void UpdateParamLiveIntervalByLiveIn(const BB &bb, uint32 insnNum);
    void ComputeLiveIn(BB &bb, uint32 insnNum);
    void ComputeLiveOut(BB &bb, uint32 insnNum);
    void ComputeLiveIntervalForEachOperand(Insn &insn);
    void ComputeLiveInterval();
    void FindLowestPrioInActive(LiveInterval *&targetLi, LiveInterval *li = nullptr, RegType regType = kRegTyInt);
    void LiveIntervalAnalysis();
    void UpdateCallQueueAtRetirement(uint32 insnID);
    void UpdateActiveAllocateInfo(const LiveInterval &li);
    void UpdateParamAllocateInfo(const LiveInterval &li);
    void RetireActive(LiveInterval &li, uint32 insnID);
    void AssignPhysRegsForLi(LiveInterval &li);
    RegOperand *GetReplaceOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx, bool isDef);
    RegOperand *GetReplaceUdOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx);
    void SetAllocMode();
    void LinearScanRegAllocator();
    void FinalizeRegisters();
    void CollectReferenceMap();
    void SolveRegOpndDeoptInfo(const RegOperand &regOpnd, DeoptInfo &deoptInfo, int32 deoptVregNO) const;
    void SolveMemOpndDeoptInfo(const MemOperand &memOpnd, DeoptInfo &deoptInfo, int32 deoptVregNO) const;
    void CollectDeoptInfo();
    void SpillOperand(Insn &insn, Operand &opnd, bool isDef, uint32 spillIdx);
    regno_t HandleSpillForLi(LiveInterval &li);
    MemOperand *GetSpillMem(uint32 vregNO, bool isDest, Insn &insn, regno_t regNO, bool &isOutOfRange,
                            uint32 bitSize) const;
    void InsertCallerSave(Insn &insn, Operand &opnd, bool isDef, uint32 spillIdx);
    uint32 GetRegFromMask(uint32 mask, regno_t offset, const LiveInterval &li);
    uint32 FindAvailablePhyReg(LiveInterval &li);
    uint32 AssignPhysRegs(LiveInterval &li);
    void ComputeLoopLiveIntervalPriority(const LoopDesc &loop);
    void ComputeLoopLiveIntervalPriorityInInsn(const Insn &insn);
    void SetLiSpill(LiveInterval &li);
    void SetStackMapDerivedInfo();
    void CollectStackMapInfo();
    void ComputeLiveIntervalForCall(Insn &insn);
private:
    uint32 FindAvailablePhyRegByFastAlloc(LiveInterval &li);
    bool NeedSaveAcrossCall(LiveInterval &li);
    uint32 FindAvailablePhyRegAcrossCall(LiveInterval &li, bool isIntReg);
    uint32 FindAvailablePhyReg(LiveInterval &li, bool isIntReg);

    LoopAnalysis &loopInfo;
    regno_t firstIntReg = 0;
    regno_t firstFpReg = 0;

    /* Comparison function for LiveInterval */
    static constexpr uint32 kMaxSpillRegNum = 3;
    static constexpr uint32 kMaxFpSpill = 2;
    MapleVector<LiveInterval *> liveIntervalsArray;
    MapleQueue<LiveInterval *> initialQue;
    using SingleQue = MapleQueue<LiveInterval *>;
    MapleVector<SingleQue> intParamQueue;
    MapleVector<SingleQue> fpParamQueue;
    MapleQueue<uint32> callQueue;
    MapleSet<LiveInterval *, ActiveCmp> active;
    MapleSet<LiveInterval *, ActiveCmp>::iterator itFinded;
    MapleVector<uint32> freeUntilPos;

    /* Change these into vectors so it can be added and deleted easily. */
    MapleSet<regno_t> intCallerRegSet;   /* integer caller saved */
    MapleSet<regno_t> intCalleeRegSet;   /*         callee       */
    MapleSet<regno_t> intParamRegSet;    /*         parameters    */
    MapleVector<regno_t> intSpillRegSet; /* integer regs put aside for spills */

    /* and register */
    uint32 intCallerMask = 0;          /* bit mask for all possible caller int */
    uint32 intCalleeMask = 0;          /*                           callee     */
    uint32 intParamMask = 0;           /*     (physical-register)   parameter  */
    MapleSet<uint32> fpCallerRegSet;   /* float caller saved */
    MapleSet<uint32> fpCalleeRegSet;   /*       callee       */
    MapleSet<uint32> fpParamRegSet;    /*       parameter    */
    MapleVector<uint32> fpSpillRegSet; /* float regs put aside for spills */
    std::unordered_set<uint32> loopBBRegSet;
    Bfs *bfs = nullptr;
    uint32 fpCallerMask = 0;       /* bit mask for all possible caller fp */
    uint32 fpCalleeMask = 0;       /*                           callee    */
    uint32 fpParamMask = 0;        /*      (physical-register)  parameter */
    uint64 blockForbiddenMask = 0; /* bit mask for forbidden physical reg */
    uint32 debugSpillCnt = 0;
    std::vector<uint64> regUsedInBB;
    uint32 maxInsnNum = 0;
    regno_t minVregNum = 0xFFFFFFFF;
    regno_t maxVregNum = 0;
    bool fastAlloc = false;
    bool spillAll = false;
    bool needExtraSpillReg = false;
    uint64 spillCount = 0;
    uint64 reloadCount = 0;
    uint64 callerSaveSpillCount = 0;
    uint64 callerSaveReloadCount = 0;
    MapleQueue<LiveInterval *> liQue;
    MapleMultiMap<uint32, LiveInterval *> splitPosMap; /* LiveInterval split position */
    MapleUnorderedMap<uint32, Insn *> splitInsnMap;    /* split insn */
    MapleVector<MoveInfo> moveInfoVec;                 /* insertion of move(PHI&Split) is based on this */
    MapleUnorderedMap<regno_t, RegOperand*> derivedRef2Base;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_REG_ALLOC_LSRA_H */
