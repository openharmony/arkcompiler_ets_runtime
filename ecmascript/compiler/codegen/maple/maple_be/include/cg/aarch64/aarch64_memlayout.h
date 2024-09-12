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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MEMLAYOUT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MEMLAYOUT_H

#include "memlayout.h"
#include "aarch64_abi.h"

namespace maplebe {
class AArch64SymbolAlloc : public SymbolAlloc {
public:
    AArch64SymbolAlloc() = default;

    ~AArch64SymbolAlloc() = default;

    void SetRegisters(AArch64reg r0, AArch64reg r1, AArch64reg r2, AArch64reg r3)
    {
        reg0 = r0;
        reg1 = r1;
        reg2 = r2;
        reg3 = r3;
    }

    inline bool IsRegister() const
    {
        return reg0 != kRinvalid;
    }

private:
    AArch64reg reg0 = kRinvalid;
    AArch64reg reg1 = kRinvalid;
    AArch64reg reg2 = kRinvalid;
    AArch64reg reg3 = kRinvalid;
};

/*
 * On AArch64, stack frames are structured as follows:
 *
 * The stack grows downward -- full descending (SP points
 * to a filled slot).
 *
 * Any of the parts of a frame is optional, i.e., it is
 * possible to write a caller-callee pair in such a way
 * that the particular part is absent in the frame.
 *
 * Before a call is made, the frame looks like:
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   | (we call them up-formals)
 * ||----------------------------|<- Stack Pointer
 * |                            |
 *
 * V1.
 * Right after a call is made
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   |
 * ||----------------------------|<- Stack Pointer
 * | PREV_FP, PREV_LR           |
 * ||----------------------------|<- Frame Pointer
 *
 * After the prologue has run,
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   |
 * ||----------------------------|
 * | PREV_FP, PREV_LR           |
 * ||----------------------------|<- Frame Pointer
 * | callee-saved registers     |
 * ||----------------------------|
 * | empty space. should have   |
 * | at least 16-byte alignment |
 * ||----------------------------|
 * | local variables            |
 * ||----------------------------|
 * | variable-sized local vars  |
 * | (VLAs)                     |
 * ||----------------------------|<- Stack Pointer
 *
 * callee-saved registers include
 *  1. R19-R28
 *  2. R8 if return value needs to be returned
 *     thru memory and callee wants to use R8
 *  3. we don't need to save R19 if it is used
 *     as base register for PIE.
 *  4. V8-V15
 *
 * V2. (this way, we may be able to save
 *     on SP modifying instruction)
 * Right after a call is made
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   |
 * ||----------------------------|<- Stack Pointer
 * |                            |
 * | empty space                |
 * |                            |
 * ||----------------------------|
 * | PREV_FP, PREV_LR           |
 * ||----------------------------|<- Frame Pointer
 *
 * After the prologue has run,
 * |                            |
 * ||----------------------------|
 * | args passed on the stack   |
 * ||----------------------------|
 * | callee-saved registers     |
 * | including those used for   |
 * | parameter passing          |
 * ||----------------------------|
 * | empty space. should have   |
 * | at least 16-byte alignment |
 * ||----------------------------|
 * | local variables            |
 * ||----------------------------|
 * | PREV_FP, PREV_LR           |
 * ||----------------------------|<- Frame Pointer
 * | variable-sized local vars  |
 * | (VLAs)                     |
 * ||----------------------------|
 * | args to pass through stack |
 * ||----------------------------|
 */
class AArch64MemLayout : public MemLayout {
public:
    AArch64MemLayout(BECommon &b, MIRFunction &f, MapleAllocator &mallocator)
        : MemLayout(b, f, mallocator, kAarch64StackPtrAlignment)
    {
    }

    ~AArch64MemLayout() override = default;

    /*
     * Returns stack space required for a call
     * which is used to pass arguments that cannot be
     * passed through registers
     */
    uint32 ComputeStackSpaceRequirementForCall(StmtNode &stmt, int32 &aggCopySize, bool isIcall) override;

    void LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize) override;

    void AssignSpillLocationsToPseudoRegisters() override {};

    uint64 StackFrameSize() const;

    uint32 RealStackFrameSize() const;

    const MemSegment &Locals() const
    {
        return segLocals;
    }

    uint32 GetSizeOfSpillReg() const
    {
        return segSpillReg.GetSize();
    }

    uint32 GetSizeOfLocals() const
    {
        return segLocals.GetSize();
    }

    uint32 GetSizeOfCold() const
    {
        return segCold.GetSize();
    }

    void SetSizeOfGRSaveArea(uint32 sz)
    {
        segGrSaveArea.SetSize(sz);
    }

    uint32 GetSizeOfGRSaveArea() const
    {
        return segGrSaveArea.GetSize();
    }

    inline void SetSizeOfVRSaveArea(uint32 sz)
    {
        segVrSaveArea.SetSize(sz);
    }

    uint32 GetSizeOfVRSaveArea() const
    {
        return segVrSaveArea.GetSize();
    }

    uint32 GetSizeOfRefLocals() const
    {
        return segRefLocals.GetSize();
    }

    uint64 GetSizeOfColdToStk() const;
    bool IsSegMentVaried(const MemSegment *seg) const;
    int32 GetCalleeSaveBaseLoc() const override;

private:
    MemSegment segRefLocals = MemSegment(kMsRefLocals);
    /* callee saved register R19-R28 (10) */
    MemSegment segLocals = MemSegment(kMsLocals); /* these are accessed via Frame Pointer */
    MemSegment segGrSaveArea = MemSegment(kMsGrSaveArea);
    MemSegment segVrSaveArea = MemSegment(kMsVrSaveArea);
    int32 fixStackSize = 0;
    void SetSizeAlignForTypeIdx(uint32 typeIdx, uint32 &size, uint32 &align) const;
    void LayoutFormalParams();
    void LayoutActualParams();
    void LayoutLocalVariables(std::vector<MIRSymbol *> &tempVar, std::vector<MIRSymbol *> &returnDelays);
    void LayoutEAVariales(std::vector<MIRSymbol *> &tempVar);
    void LayoutReturnRef(std::vector<MIRSymbol *> &returnDelays, int32 &structCopySize, int32 &maxParmStackSize);
    // layout small local near sp, otherwise, above spill region
    void LayoutLocalsInSize(const MIRSymbol &mirSym);

    SymbolAlloc *CreateSymbolAlloc() const override
    {
        return memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
    }
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MEMLAYOUT_H */
