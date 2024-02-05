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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PROEPILOG_H

#include "proepilog.h"
#include "cg.h"
#include "operand.h"
#include "aarch64_cgfunc.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"

namespace maplebe {
using namespace maple;

class AArch64GenProEpilog : public GenProEpilog {
public:
    AArch64GenProEpilog(CGFunc &func, MemPool &memPool)
        : GenProEpilog(func), tmpAlloc(&memPool), exitBB2CallSitesMap(tmpAlloc.Adapter())
    {
        useFP = func.UseFP();
        if (func.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
            stackBaseReg = RFP;
        } else {
            stackBaseReg = useFP ? R29 : RSP;
        }
        exitBB2CallSitesMap.clear();
        AArch64CGFunc &aarchCGFunc = static_cast<AArch64CGFunc &>(func);
        const MapleVector<AArch64reg> &calleeSavedRegs = aarchCGFunc.GetCalleeSavedRegs();
        if (useFP) {
            storeFP = true;
        } else if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), RFP) != calleeSavedRegs.end()) {
            storeFP = true;
        } else if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), R29) != calleeSavedRegs.end()) {
            storeFP = true;
        }
        aarchCGFunc.SetStoreFP(storeFP);
    }
    ~AArch64GenProEpilog() override = default;

    bool TailCallOpt() override;
    bool NeedProEpilog() override;
    static MemOperand *SplitStpLdpOffsetForCalleeSavedWithAddInstruction(CGFunc &cgFunc, const MemOperand &mo,
                                                                         uint32 bitLen,
                                                                         AArch64reg baseReg = AArch64reg::kRinvalid);
    static void AppendInstructionPushPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty, int offset);
    static void AppendInstructionPushSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int offset);
    static void AppendInstructionPopSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int offset);
    static void AppendInstructionPopPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty, int offset);
    void Run() override;

private:
    void GenStackGuard(BB &);
    BB &GenStackGuardCheckInsn(BB &);
    bool HasLoop();
    bool OptimizeTailBB(BB &bb, MapleSet<Insn *> &callInsns, const BB &exitBB) const;
    void TailCallBBOpt(BB &bb, MapleSet<Insn *> &callInsns, BB &exitBB);
    bool InsertOpndRegs(Operand &opnd, std::set<regno_t> &vecRegs) const;
    bool InsertInsnRegs(Insn &insn, bool insetSource, std::set<regno_t> &vecSourceRegs, bool insertTarget,
                        std::set<regno_t> &vecTargetRegs);
    bool FindRegs(Operand &insn, std::set<regno_t> &vecRegs) const;
    bool BackwardFindDependency(BB &ifbb, std::set<regno_t> &vecReturnSourceReg, std::list<Insn *> &existingInsns,
                                std::list<Insn *> &moveInsns);
    BB *IsolateFastPath(BB &);
    void AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void GeneratePushRegs();
    void GeneratePushUnnamedVarargRegs();
    void AppendInstructionStackCheck(AArch64reg reg, RegType rty, int offset);
    void GenerateProlog(BB &);

    void GenerateRet(BB &bb);
    bool TestPredsOfRetBB(const BB &exitBB);
    void AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void GeneratePopRegs();
    void AppendJump(const MIRSymbol &func);
    void GenerateEpilog(BB &);
    void GenerateEpilogForCleanup(BB &);
    void ConvertToTailCalls(MapleSet<Insn *> &callInsnsMap);
    Insn &CreateAndAppendInstructionForAllocateCallFrame(int64 argsToStkPassSize, AArch64reg reg0, AArch64reg reg1,
                                                         RegType rty);
    Insn &AppendInstructionForAllocateOrDeallocateCallFrame(int64 argsToStkPassSize, AArch64reg reg0, AArch64reg reg1,
                                                            RegType rty, bool isAllocate);
    MapleMap<BB *, MapleSet<Insn *>> &GetExitBB2CallSitesMap()
    {
        return exitBB2CallSitesMap;
    }
    void SetCurTailcallExitBB(BB *bb)
    {
        curTailcallExitBB = bb;
    }
    BB *GetCurTailcallExitBB()
    {
        return curTailcallExitBB;
    }
    void SetFastPathReturnBB(BB *bb)
    {
        fastPathReturnBB = bb;
    }
    BB *GetFastPathReturnBB()
    {
        return fastPathReturnBB;
    }
    MapleAllocator tmpAlloc;
    static constexpr const int32 kOffset8MemPos = 8;
    static constexpr const int32 kOffset16MemPos = 16;
    MapleMap<BB *, MapleSet<Insn *>> exitBB2CallSitesMap;
    BB *curTailcallExitBB = nullptr;
    BB *fastPathReturnBB = nullptr;
    bool useFP = true;
    // To be compatible with previous code more easily，we use storeFP boolean to indicate the case
    // (1) use FP to address
    // (2) FP is clobbered
    // need to delete this and optimize the callee save process.
    bool storeFP = false;
    /* frame pointer(x29) is available as a general-purpose register if useFP is set as false */
    AArch64reg stackBaseReg = RFP;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PROEPILOG_H */
