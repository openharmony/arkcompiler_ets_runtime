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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H

#include "cg.h"
#include "ra_opt.h"
#include "aarch64_cg.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {
class X0OptInfo {
public:
    X0OptInfo() : movSrc(nullptr), replaceReg(0), renameInsn(nullptr), renameOpnd(nullptr), renameReg(0) {}
    ~X0OptInfo() = default;

    inline RegOperand *GetMovSrc() const
    {
        return movSrc;
    }

    inline regno_t GetReplaceReg() const
    {
        return replaceReg;
    }

    inline Insn *GetRenameInsn() const
    {
        return renameInsn;
    }

    inline Operand *GetRenameOpnd() const
    {
        return renameOpnd;
    }

    inline regno_t GetRenameReg() const
    {
        return renameReg;
    }

    inline void SetMovSrc(RegOperand *srcReg)
    {
        movSrc = srcReg;
    }

    inline void SetReplaceReg(regno_t regno)
    {
        replaceReg = regno;
    }

    inline void SetRenameInsn(Insn *insn)
    {
        renameInsn = insn;
    }

    inline void ResetRenameInsn()
    {
        renameInsn = nullptr;
    }

    inline void SetRenameOpnd(Operand *opnd)
    {
        renameOpnd = opnd;
    }

    inline void SetRenameReg(regno_t regno)
    {
        renameReg = regno;
    }

private:
    RegOperand *movSrc;
    regno_t replaceReg;
    Insn *renameInsn;
    Operand *renameOpnd;
    regno_t renameReg;
};

class RaX0Opt {
public:
    explicit RaX0Opt(CGFunc *func) : cgFunc(func) {}
    ~RaX0Opt() = default;

    bool PropagateX0CanReplace(Operand *opnd, regno_t replaceReg) const;
    bool PropagateRenameReg(Insn *insn, const X0OptInfo &optVal) const;
    bool PropagateX0DetectX0(const Insn *insn, X0OptInfo &optVal) const;
    bool PropagateX0DetectRedefine(const InsnDesc *md, const Insn *ninsn, const X0OptInfo &optVal, uint32 index) const;
    bool PropagateX0Optimize(const BB *bb, const Insn *insn, X0OptInfo &optVal);
    bool PropagateX0ForCurrBb(BB *bb, const X0OptInfo &optVal);
    void PropagateX0ForNextBb(BB *nextBb, const X0OptInfo &optVal);
    void PropagateX0();

private:
    CGFunc *cgFunc;
};

class VregRenameInfo {
public:
    VregRenameInfo() = default;

    ~VregRenameInfo() = default;

    BB *firstBBLevelSeen = nullptr;
    BB *lastBBLevelSeen = nullptr;
    uint32 numDefs = 0;
    uint32 numUses = 0;
    uint32 numInnerDefs = 0;
    uint32 numInnerUses = 0;
    uint32 largestUnusedDistance = 0;
    uint8 innerMostloopLevelSeen = 0;
};

class VregRename {
public:
    VregRename(CGFunc *func, MemPool *pool, LoopAnalysis &loop)
        : cgFunc(func), memPool(pool), alloc(pool), loopInfo(loop), renameInfo(alloc.Adapter())
    {
        renameInfo.resize(cgFunc->GetMaxRegNum());
        ccRegno = static_cast<RegOperand *>(&cgFunc->GetOrCreateRflag())->GetRegisterNumber();
    };
    ~VregRename() = default;

    void PrintRenameInfo(regno_t regno) const;
    void PrintAllRenameInfo() const;

    void RenameFindLoopVregs(const LoopDesc &loop);
    void RenameFindVregsToRename(const LoopDesc &loop);
    bool IsProfitableToRename(const VregRenameInfo *info) const;
    void RenameProfitableVreg(RegOperand &ropnd, const LoopDesc &loop);
    void RenameGetFuncVregInfo();
    void UpdateVregInfo(regno_t reg, BB *bb, bool isInner, bool isDef);
    void VregLongLiveRename();

    CGFunc *cgFunc;
    MemPool *memPool;
    MapleAllocator alloc;
    LoopAnalysis &loopInfo;
    Bfs *bfs = nullptr;
    MapleVector<VregRenameInfo *> renameInfo;
    uint32 maxRegnoSeen = 0;
    regno_t ccRegno;
};

class AArch64RaOpt : public RaOpt {
public:
    AArch64RaOpt(CGFunc &func, MemPool &pool, DomAnalysis &dom, LoopAnalysis &loop) : RaOpt(func, pool, dom, loop) {}
    ~AArch64RaOpt() override = default;
    void Run() override;

private:
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64RAOPT_H */
