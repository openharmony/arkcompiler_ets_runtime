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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H

#include "cg.h"
#include "regsaves.h"
#include "aarch64_cg.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {

/* Saved reg info.  This class is created to avoid the complexity of
   nested Maple Containers */
class SavedRegInfo {
public:
    bool insertAtLastMinusOne = false;
    explicit SavedRegInfo(MapleAllocator &alloc)
        : saveSet(alloc.Adapter()), restoreEntrySet(alloc.Adapter()), restoreExitSet(alloc.Adapter())
    {
    }

    bool ContainSaveReg(regno_t r)
    {
        if (saveSet.find(r) != saveSet.end()) {
            return true;
        }
        return false;
    }

    bool ContainEntryReg(regno_t r)
    {
        if (restoreEntrySet.find(r) != restoreEntrySet.end()) {
            return true;
        }
        return false;
    }

    bool ContainExitReg(regno_t r)
    {
        if (restoreExitSet.find(r) != restoreExitSet.end()) {
            return true;
        }
        return false;
    }

    void InsertSaveReg(regno_t r)
    {
        (void)saveSet.insert(r);
    }

    void InsertEntryReg(regno_t r)
    {
        (void)restoreEntrySet.insert(r);
    }

    void InsertExitReg(regno_t r)
    {
        (void)restoreExitSet.insert(r);
    }

    MapleSet<regno_t> &GetSaveSet()
    {
        return saveSet;
    }

    MapleSet<regno_t> &GetEntrySet()
    {
        return restoreEntrySet;
    }

    MapleSet<regno_t> &GetExitSet()
    {
        return restoreExitSet;
    }

    void RemoveSaveReg(regno_t r)
    {
        (void)saveSet.erase(r);
    }

private:
    MapleSet<regno_t> saveSet;
    MapleSet<regno_t> restoreEntrySet;
    MapleSet<regno_t> restoreExitSet;
};

class SavedBBInfo {
public:
    explicit SavedBBInfo(MapleAllocator &alloc) : bbList(alloc.Adapter()) {}

    MapleSet<BB *> &GetBBList()
    {
        return bbList;
    }

    void InsertBB(BB *bb)
    {
        (void)bbList.insert(bb);
    }

    void RemoveBB(BB *bb)
    {
        (void)bbList.erase(bb);
    }

private:
    MapleSet<BB *> bbList;
};

class AArch64RegSavesOpt : public RegSavesOpt {
public:
    AArch64RegSavesOpt(CGFunc &func, MemPool &pool, DomAnalysis &dom, PostDomAnalysis &pdom, LoopAnalysis &loop)
        : RegSavesOpt(func, pool),
          domInfo(&dom),
          pDomInfo(&pdom),
          loopInfo(loop),
          bbSavedRegs(alloc.Adapter()),
          regSavedBBs(alloc.Adapter()),
          regOffset(alloc.Adapter()),
          id2bb(alloc.Adapter())
    {
        bbSavedRegs.resize(func.NumBBs());
        regSavedBBs.resize(sizeof(CalleeBitsType) << k8BitShift);
        for (size_t i = 0; i < bbSavedRegs.size(); ++i) {
            bbSavedRegs[i] = nullptr;
        }
        for (size_t i = 0; i < regSavedBBs.size(); ++i) {
            regSavedBBs[i] = nullptr;
        }
    }
    ~AArch64RegSavesOpt() override = default;

    using CalleeBitsType = uint64;

    void InitData();
    void CollectLiveInfo(const BB &bb, const Operand &opnd, bool isDef, bool isUse);
    void GenerateReturnBBDefUse(const BB &bb);
    void ProcessCallInsnParam(BB &bb);
    void ProcessAsmListOpnd(const BB &bb, Operand &opnd, uint32 idx);
    void ProcessListOpnd(const BB &bb, Operand &opnd);
    void ProcessMemOpnd(const BB &bb, Operand &opnd);
    void ProcessCondOpnd(const BB &bb);
    void GetLocalDefUse();
    void PrintBBs() const;
    int CheckCriteria(BB *bb, regno_t reg) const;
    bool AlreadySavedInDominatorList(const BB *bb, regno_t reg) const;
    void DetermineCalleeSaveLocationsDoms();
    void DetermineCalleeSaveLocationsPre();
    bool DetermineCalleeRestoreLocations();
    int32 FindNextOffsetForCalleeSave() const;
    void InsertCalleeSaveCode();
    void InsertCalleeRestoreCode();
    void Verify(regno_t reg, BB *bb, std::set<BB *, BBIdCmp> *visited, uint32 *s, uint32 *r);
    void Run() override;

    DomAnalysis *GetDomInfo() const
    {
        return domInfo;
    }

    PostDomAnalysis *GetPostDomInfo() const
    {
        return pDomInfo;
    }

    Bfs *GetBfs() const
    {
        return bfs;
    }

    CalleeBitsType *GetCalleeBitsDef()
    {
        return calleeBitsDef;
    }

    CalleeBitsType *GetCalleeBitsUse()
    {
        return calleeBitsUse;
    }

    CalleeBitsType GetBBCalleeBits(CalleeBitsType *data, uint32 bid) const
    {
        return data[bid];
    }

    void SetCalleeBit(CalleeBitsType *data, uint32 bid, regno_t reg) const
    {
        CalleeBitsType mask = 1ULL << RegBitMap(reg);
        if ((GetBBCalleeBits(data, bid) & mask) == 0) {
            data[bid] = GetBBCalleeBits(data, bid) | mask;
        }
    }

    void ResetCalleeBit(CalleeBitsType *data, uint32 bid, regno_t reg) const
    {
        CalleeBitsType mask = 1ULL << RegBitMap(reg);
        data[bid] = GetBBCalleeBits(data, bid) & ~mask;
    }

    bool IsCalleeBitSet(CalleeBitsType *data, uint32 bid, regno_t reg) const
    {
        CalleeBitsType mask = 1ULL << RegBitMap(reg);
        return GetBBCalleeBits(data, bid) & mask;
    }

    /* AArch64 specific callee-save registers bit positions
        0       9  10                33   -- position
       R19 ..  R28 V8 .. V15 V16 .. V31   -- regs */
    uint32 RegBitMap(regno_t reg) const
    {
        uint32 r;
        if (reg <= R28) {
            r = (reg - R19);
        } else {
            r = ((R28 - R19) + 1) + (reg - V8);
        }
        return r;
    }

    regno_t ReverseRegBitMap(uint32 reg) const
    {
        constexpr uint32 floatRegisterBitOffset = 10;
        if (reg < floatRegisterBitOffset) {
            return static_cast<AArch64reg>(R19 + reg);
        } else {
            return static_cast<AArch64reg>((V8 + reg) - (R28 - R19 + 1));
        }
    }

    SavedRegInfo *GetbbSavedRegsEntry(uint32 bid)
    {
        if (bbSavedRegs[bid] == nullptr) {
            bbSavedRegs[bid] = memPool->New<SavedRegInfo>(alloc);
        }
        return bbSavedRegs[bid];
    }

    void SetId2bb(BB *bb)
    {
        id2bb[bb->GetId()] = bb;
    }

    BB *GetId2bb(uint32 bid)
    {
        return id2bb[bid];
    }

private:
    DomAnalysis *domInfo;
    PostDomAnalysis *pDomInfo;
    LoopAnalysis &loopInfo;
    Bfs *bfs = nullptr;
    CalleeBitsType *calleeBitsDef = nullptr;
    CalleeBitsType *calleeBitsUse = nullptr;
    MapleVector<SavedRegInfo *> bbSavedRegs; /* set of regs to be saved in a BB */
    MapleVector<SavedBBInfo *> regSavedBBs;  /* set of BBs to be saved for a reg */
    MapleMap<regno_t, uint32> regOffset;     /* save offset of each register */
    MapleMap<uint32, BB *> id2bb;            /* bbid to bb* mapping */
    bool oneAtaTime = false;
    regno_t oneAtaTimeReg = 0;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H */
