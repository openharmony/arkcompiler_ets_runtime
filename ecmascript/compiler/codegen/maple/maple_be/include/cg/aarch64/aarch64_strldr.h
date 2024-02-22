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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H

#include "strldr.h"
#include "aarch64_reaching.h"
#include "aarch64_operand.h"

namespace maplebe {
using namespace maple;
enum MemPropMode : uint8 { kUndef, kPropBase, kPropOffset, kPropSignedExtend, kPropUnsignedExtend, kPropShift };

class AArch64StoreLoadOpt : public StoreLoadOpt {
public:
    AArch64StoreLoadOpt(CGFunc &func, MemPool &memPool, LoopAnalysis &loop)
        : StoreLoadOpt(func, memPool), localAlloc(&memPool), loopInfo(loop), str2MovMap(localAlloc.Adapter())
    {
    }
    ~AArch64StoreLoadOpt() override = default;
    void Run() final;
    void DoStoreLoadOpt();
    void DoLoadZeroToMoveTransfer(const Insn &strInsn, short strSrcIdx, const InsnSet &memUseInsnSet) const;
    void DoLoadToMoveTransfer(Insn &strInsn, short strSrcIdx, short memSeq, const InsnSet &memUseInsnSet);
    bool CheckStoreOpCode(MOperator opCode) const;
    static bool CheckNewAmount(const Insn &insn, uint32 newAmount);

private:
    void StrLdrIndexModeOpt(Insn &currInsn);
    bool CheckReplaceReg(Insn &defInsn, Insn &currInsn, InsnSet &replaceRegDefSet, regno_t replaceRegNo);
    bool CheckDefInsn(Insn &defInsn, Insn &currInsn);
    bool CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx);
    MemOperand *HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal);
    MemOperand *SelectReplaceMem(Insn &defInsn, Insn &curInsn, RegOperand &base, Operand *offset);
    MemOperand *SelectReplaceExt(const Insn &defInsn, RegOperand &base, bool isSigned);
    bool CanDoMemProp(const Insn *insn);
    bool CanDoIndexOpt(const MemOperand &MemOpnd);
    void MemPropInit();
    void SelectPropMode(const MemOperand &currMemOpnd);
    int64 GetOffsetForNewIndex(Insn &defInsn, Insn &insn, regno_t baseRegNO, uint32 memOpndSize);
    MemOperand *SelectIndexOptMode(Insn &insn, const MemOperand &curMemOpnd);
    bool ReplaceMemOpnd(Insn &insn, regno_t regNo, RegOperand &base, Operand *offset);
    void MemProp(Insn &insn);
    void ProcessStrPair(Insn &insn);
    void ProcessStr(Insn &insn);
    void GenerateMoveLiveInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd, Insn &ldrInsn, Insn &strInsn,
                              short memSeq);
    void GenerateMoveDeadInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd, Insn &ldrInsn, Insn &strInsn,
                              short memSeq);
    bool HasMemBarrier(const Insn &ldrInsn, const Insn &strInsn) const;
    bool IsAdjacentBB(Insn &defInsn, Insn &curInsn) const;
    MapleAllocator localAlloc;
    LoopAnalysis &loopInfo;
    /* the max number of mov insn to optimize. */
    static constexpr uint8 kMaxMovNum = 2;
    MapleMap<Insn *, Insn *[kMaxMovNum]> str2MovMap;
    MemPropMode propMode = kUndef;
    uint32 amount = 0;
    bool removeDefInsn = false;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_STRLDR_H */
