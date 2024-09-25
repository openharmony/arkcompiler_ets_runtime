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
    AArch64GenProEpilog(CGFunc &func, MemPool &memPool) : GenProEpilog(func), tmpAlloc(&memPool)
    {
        useFP = func.UseFP();
        if (func.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
            stackBaseReg = RFP;
        } else {
            stackBaseReg = useFP ? R29 : RSP;
        }
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
    static MemOperand *SplitStpLdpOffsetForCalleeSavedWithAddInstruction(CGFunc &cgFunc, const MemOperand &mo,
                                                                         uint32 bitLen,
                                                                         AArch64reg baseRegNum = AArch64reg::kRinvalid);
    static void AppendInstructionPushPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset);
    static void AppendInstructionPushSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset);
    static void AppendInstructionPopSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset);
    static void AppendInstructionPopPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty, int32 offset);
    void Run() override;

private:
    void AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void GeneratePushRegs();
    void GenerateProlog(BB &bb);
    void GenerateSave();
    void GenerateFrameTypeSave(SaveInfo &frameInfo, int32 stackSize, int64 fpToSpDistance);
    void GenerateFunctionSave(SaveInfo &funcInfo, int32 stackSize, int64 fpToSpDistance);
    void GenerateRet(BB &bb);
    void AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty);
    void GeneratePopRegs();
    void GenerateEpilog(BB &bb);
    void AppendBBtoEpilog(BB &epilogBB, BB &newBB);
    Insn &CreateAndAppendInstructionForAllocateCallFrame(int64 fpToSpDistance, AArch64reg reg0, AArch64reg reg1,
                                                         RegType rty);
    Insn &AppendInstructionForAllocateOrDeallocateCallFrame(int64 fpToSpDistance, AArch64reg reg0, AArch64reg reg1,
                                                            RegType rty, bool isAllocate);
    MapleAllocator tmpAlloc;
    static constexpr const int32 kOffset8MemPos = 8;
    static constexpr const int32 kOffset16MemPos = 16;
    bool useFP = false;
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
