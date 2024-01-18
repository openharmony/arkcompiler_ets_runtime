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

#ifndef MAPLEBE_INCLUDE_CG_EBO_H
#define MAPLEBE_INCLUDE_CG_EBO_H

#include "cg_phase.h"
#include "cgbb.h"
#include "live.h"
#include "loop.h"

namespace maplebe {
namespace {
constexpr uint32 kEboDefaultMemHash = 0;
constexpr uint32 kEboNoAliasMemHash = 1;
constexpr uint32 kEboSpillMemHash = 2;
constexpr uint32 kEboCopyInsnHash = 3;
constexpr uint32 kEboReservedInsnHash = 4;
constexpr uint32 kEboMaxExpInsnHash = 1024;
constexpr uint32 kEboMaxOpndHash = 521;
constexpr uint32 kEboMaxInsnHash = kEboReservedInsnHash + kEboMaxExpInsnHash;
};  // namespace

#define EBO_EXP_INSN_HASH(val) ((kEboMaxExpInsnHash - 1ULL) & (static_cast<uint64>(val) >> 6))

/* forward decls */
class InsnInfo;

struct OpndInfo {
    explicit OpndInfo(Operand &opnd) : opnd(&opnd) {}

    virtual ~OpndInfo() = default;

    int32 hashVal = 0; /* Mem operand is placed in hash table, this is the hashVal of it, and otherwise -1. */
    Operand *opnd;     /* Operand */
    Operand *replacementOpnd = nullptr;  /* Rename opnd with this new name. */
    OpndInfo *replacementInfo = nullptr; /* Rename opnd with this info. */
    BB *bb = nullptr;                    /* The Definining bb. */
    Insn *insn = nullptr;                /* The Defining insn. */
    InsnInfo *insnInfo = nullptr;
    bool redefinedInBB = false;    /* A following definition exisit in bb. */
    bool redefined = false;        /* A following definition exisit. */
    Insn *redefinedInsn = nullptr; /* Next defined insn if redefinedInBB is true */
#if TARGARM32
    bool mayReDef = false;
#endif
    OpndInfo *same = nullptr; /* Other definitions of the same operand. */
    OpndInfo *prev = nullptr;
    OpndInfo *next = nullptr;
    OpndInfo *hashNext = nullptr;
    int32 refCount = 0; /* Number of references to the operand. */
};

struct MemOpndInfo : public OpndInfo {
    explicit MemOpndInfo(Operand &opnd) : OpndInfo(opnd) {}

    ~MemOpndInfo() override = default;

    OpndInfo *GetBaseInfo() const
    {
        return base;
    }

    OpndInfo *GetOffsetInfo() const
    {
        return offset;
    }

    void SetBaseInfo(OpndInfo &baseInfo)
    {
        base = &baseInfo;
    }

    void SetOffsetInfo(OpndInfo &offInfo)
    {
        offset = &offInfo;
    }

private:
    OpndInfo *base = nullptr;
    OpndInfo *offset = nullptr;
};

class InsnInfo {
public:
    InsnInfo(MemPool &memPool, Insn &insn)
        : alloc(&memPool),
          bb(insn.GetBB()),
          insn(&insn),
          result(alloc.Adapter()),
          origOpnd(alloc.Adapter()),
          optimalOpnd(alloc.Adapter())
    {
    }

    virtual ~InsnInfo() = default;
    MapleAllocator alloc;
    uint32 hashIndex = 0;
    bool mustNotBeRemoved = false; /* Some condition requires this insn. */
    BB *bb;                        /* The defining bb. */
    Insn *insn;                    /* The defining insn. */
    InsnInfo *same = nullptr;      /* Other insns with the same hash value. */
    InsnInfo *prev = nullptr;
    InsnInfo *next = nullptr;
    MapleVector<OpndInfo *> result; /* Result array. */
    MapleVector<OpndInfo *> origOpnd;
    MapleVector<OpndInfo *> optimalOpnd;
};

class Ebo {
public:
    Ebo(CGFunc &func, MemPool &memPool, LiveAnalysis *live, bool before, const std::string &phase)
        : cgFunc(&func),
          beforeRegAlloc(before),
          phaseName(phase),
          live(live),
          eboMp(&memPool),
          eboAllocator(&memPool),
          visitedBBs(eboAllocator.Adapter()),
          vRegInfo(eboAllocator.Adapter()),
          exprInfoTable(eboAllocator.Adapter()),
          insnInfoTable(eboAllocator.Adapter())
    {
    }

    virtual ~Ebo() = default;

    MemOpndInfo *GetMemInfo(InsnInfo &insnInfo);
    void SetInsnInfo(uint32 hashVal, InsnInfo &info)
    {
        DEBUG_ASSERT(hashVal < insnInfoTable.size(), "hashVal out of insnInfoTable range");
        insnInfoTable.at(hashVal) = &info;
    }

    void IncRef(OpndInfo &info) const
    {
        ++info.refCount;
    }

    void DecRef(OpndInfo &info) const
    {
        --info.refCount;
    }
    bool IsSaveReg(const Operand &opnd);
    bool IsFrameReg(Operand &opnd) const;
    bool OperandEqual(const Operand &op1, const Operand &op2) const;
    Operand *GetZeroOpnd(uint32 size) const;
    bool IsPhysicalReg(const Operand &opnd) const;
    bool HasAssignedReg(const Operand &opnd) const;
    bool IsOfSameClass(const Operand &op0, const Operand &op1) const;
    bool OpndAvailableInBB(const BB &bb, OpndInfo *info);
    bool ForwardPropCheck(const Operand *opndReplace, const OpndInfo &opndInfo, const Operand &opnd, Insn &insn);
    bool RegForwardCheck(Insn &insn, const Operand &opnd, const Operand *opndReplace, Operand &oldOpnd,
                         const OpndInfo *tmpInfo);
    bool IsNotVisited(const BB &bb)
    {
        return !visitedBBs.at(bb.GetId());
    };

    void SetBBVisited(const BB &bb)
    {
        visitedBBs.at(bb.GetId()) = true;
    };

    void UpdateOpndInfo(const Operand &opnd, OpndInfo &opndInfo, OpndInfo *newInfo, int32 hashVal);
    void SetOpndInfo(const Operand &opnd, OpndInfo *opndInfo, int32 hashVal);
    bool RegistersIdentical(const Operand &op0, const Operand &op1) const;
    OpndInfo *GetOpndInfo(const Operand &opnd, int32 hashVal) const;
    OpndInfo *GetNewOpndInfo(BB &bb, Insn *insn, Operand &opnd, int32 hashVal);
    OpndInfo *OperandInfoUse(BB &currentBB, Operand &localOpnd);
    InsnInfo *GetNewInsnInfo(Insn &insn);
    int32 ComputeOpndHash(const Operand &opnd) const;
    uint32 ComputeHashVal(Insn &insn, const MapleVector<OpndInfo *> &opndInfos) const;
    void MarkOpndLiveIntoBB(const Operand &opnd, BB &into, BB &def) const;
    void RemoveInsn(InsnInfo &info);
    void RemoveUses(uint32 opndNum, const MapleVector<OpndInfo *> &origInfo);
    void HashInsn(Insn &insn, const MapleVector<OpndInfo *> &origInfo, const MapleVector<OpndInfo *> &opndInfos);
    void BuildAllInfo(BB &bb);
    InsnInfo *LocateInsnInfo(const OpndInfo &info);
    void RemoveUnusedInsns(BB &bb, bool normal);
    void UpdateNextInfo(const OpndInfo &opndInfo);
    void BackupOpndInfoList(OpndInfo *saveLast);
    void BackupInsnInfoList(InsnInfo *saveLast);
    void AddBB2EB(BB &bb);
    void EboInit();
    void EboProcessSingleBB();
    void EboProcess();
    void Run();
    std::string PhaseName() const
    {
        return phaseName;
    }

protected:
    CGFunc *cgFunc;
    bool beforeRegAlloc; /* True if perform Ebo before register allocation. */
    virtual OpndInfo *OperandInfoDef(BB &currentBB, Insn &currentInsn, Operand &localOpnd) = 0;
    virtual const RegOperand &GetRegOperand(const Operand &opnd) const = 0;
    virtual bool IsGlobalNeeded(Insn &insn) const = 0;
    virtual bool IsDecoupleStaticOp(Insn &insn) const = 0;
    virtual bool IsFmov(const Insn &insn) const = 0;
    virtual bool SpecialSequence(Insn &insn, const MapleVector<OpndInfo *> &origInfos) = 0;
    virtual bool DoConstProp(Insn &insn, uint32 i, Operand &opnd) = 0;
    virtual bool Csel2Cset(Insn &insn, const MapleVector<Operand *> &opnds) = 0;
    virtual bool SimplifyConstOperand(Insn &insn, const MapleVector<Operand *> &opnds,
                                      const MapleVector<OpndInfo *> &opndInfo) = 0;
    virtual int32 GetOffsetVal(const MemOperand &mem) const = 0;
    virtual bool OperandEqSpecial(const Operand &op1, const Operand &op2) const = 0;
    virtual void BuildCallerSaveRegisters() = 0;
    virtual void DefineAsmRegisters(InsnInfo &insnInfo) = 0;
    virtual void DefineCallerSaveRegisters(InsnInfo &insnInfo) = 0;
    virtual void DefineReturnUseRegister(Insn &insn) = 0;
    virtual void DefineCallUseSpecialRegister(Insn &insn) = 0;
    virtual void DefineClinitSpecialRegisters(InsnInfo &insnInfo) = 0;
    virtual bool IsMovToSIMDVmov(Insn &insn, const Insn &replaceInsn) const = 0;
    virtual bool IsPseudoRet(Insn &insn) const = 0;
    virtual bool ChangeLdrMop(Insn &insn, const Operand &opnd) const = 0;
    virtual bool IsAdd(const Insn &insn) const = 0;
    virtual bool IsClinitCheck(const Insn &insn) const = 0;
    virtual bool IsLastAndBranch(BB &bb, Insn &insn) const = 0;
    virtual bool IsSameRedefine(BB &bb, Insn &insn, OpndInfo &opndInfo) const = 0;
    virtual bool ResIsNotDefAndUse(Insn &insn) const = 0;
    virtual bool LiveOutOfBB(const Operand &opnd, const BB &bb) const = 0;
    virtual bool IsInvalidReg(const RegOperand &opnd) const = 0;
    virtual bool IsZeroRegister(const Operand &opnd) const = 0;
    virtual bool IsConstantImmOrReg(const Operand &opnd) const = 0;
    OpndInfo *BuildMemOpndInfo(BB &bb, Insn &insn, Operand &opnd, uint32 opndIndex);
    OpndInfo *BuildOperandInfo(BB &bb, Insn &insn, Operand &opnd, uint32 opndIndex, MapleVector<OpndInfo *> &origInfos);
    bool ForwardPropagateOpnd(Insn &insn, Operand *&opnd, uint32 opndIndex, OpndInfo *&opndInfo,
                              MapleVector<OpndInfo *> &origInfos);
    void SimplifyInsn(Insn &insn, bool &insnReplaced, bool opndsConstant, const MapleVector<Operand *> &opnds,
                      const MapleVector<OpndInfo *> &opndInfos, const MapleVector<OpndInfo *> &origInfos);
    void FindRedundantInsns(BB &bb, Insn *&insn, const Insn *prev, bool insnReplaced, MapleVector<Operand *> &opnds,
                            MapleVector<OpndInfo *> &opndInfos, const MapleVector<OpndInfo *> &origInfos);
    void PreProcessSpecialInsn(Insn &insn);

    std::string phaseName;
    LiveAnalysis *live;
    uint32 bbNum = 0; /* bb numbers for an extend block. */
    MemPool *eboMp;
    MapleAllocator eboAllocator;
    MapleVector<bool> visitedBBs;
    OpndInfo *firstOpndInfo = nullptr;
    OpndInfo *lastOpndInfo = nullptr;
    InsnInfo *firstInsnInfo = nullptr;
    InsnInfo *lastInsnInfo = nullptr;
    MapleUnorderedMap<uint32, OpndInfo *> vRegInfo;
    MapleVector<OpndInfo *> exprInfoTable;
    MapleVector<InsnInfo *> insnInfoTable;
    bool optSuccess = false;
};

MAPLE_FUNC_PHASE_DECLARE(CgEbo0, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE(CgEbo1, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE(CgPostEbo, maplebe::CGFunc)
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_EBO_H */
