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

#include "aarch64_proepilog.h"
#include "aarch64_cg.h"
#include "cg_option.h"
#include "cgfunc.h"

#define PROEPILOG_DUMP CG_DEBUG_FUNC(cgFunc)
namespace maplebe {
using namespace maple;

namespace {
enum RegsPushPop : uint8 { kRegsPushOp, kRegsPopOp };

enum PushPopType : uint8 { kPushPopSingle = 0, kPushPopPair = 1 };

MOperator pushPopOps[kRegsPopOp + 1][kRegTyFloat + 1][kPushPopPair + 1] = {{
                                                                               /* push */
                                                                               {0}, /* undef */
                                                                               {
                                                                                   /* kRegTyInt */
                                                                                   MOP_xstr, /* single */
                                                                                   MOP_xstp, /* pair   */
                                                                               },
                                                                               {
                                                                                   /* kRegTyFloat */
                                                                                   MOP_dstr, /* single */
                                                                                   MOP_dstp, /* pair   */
                                                                               },
                                                                           },
                                                                           {
                                                                               /* pop */
                                                                               {0}, /* undef */
                                                                               {
                                                                                   /* kRegTyInt */
                                                                                   MOP_xldr, /* single */
                                                                                   MOP_xldp, /* pair   */
                                                                               },
                                                                               {
                                                                                   /* kRegTyFloat */
                                                                                   MOP_dldr, /* single */
                                                                                   MOP_dldp, /* pair   */
                                                                               },
                                                                           }};

inline void AppendInstructionTo(Insn &insn, CGFunc &func)
{
    func.GetCurBB()->AppendInsn(insn);
}
}  // namespace

MemOperand *AArch64GenProEpilog::SplitStpLdpOffsetForCalleeSavedWithAddInstruction(CGFunc &cgFunc, const MemOperand &mo,
                                                                                   uint32 bitLen, AArch64reg baseRegNum)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CHECK_FATAL(mo.GetAddrMode() == MemOperand::kAddrModeBOi, "mode should be kAddrModeBOi");
    OfstOperand *ofstOp = mo.GetOffsetImmediate();
    int32 offsetVal = static_cast<int32>(ofstOp->GetOffsetValue());
    CHECK_FATAL(offsetVal > 0, "offsetVal should be greater than 0");
    CHECK_FATAL((static_cast<uint32>(offsetVal) & 0x7) == 0, "(offsetVal & 0x7) should be equal to 0");
    /*
     * Offset adjustment due to FP/SP has already been done
     * in AArch64GenProEpilog::GeneratePushRegs() and AArch64GenProEpilog::GeneratePopRegs()
     */
    RegOperand &br = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(baseRegNum, bitLen, kRegTyInt);
    ImmOperand &immAddEnd = aarchCGFunc.CreateImmOperand(offsetVal, k64BitSize, true);
    RegOperand *origBaseReg = mo.GetBaseRegister();
    aarchCGFunc.SelectAdd(br, *origBaseReg, immAddEnd, PTY_i64);

    return &aarchCGFunc.CreateReplacementMemOperand(bitLen, br, 0);
}

void AArch64GenProEpilog::AppendInstructionPushPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty,
                                                    int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
    Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());

    uint32 dataSize = GetPointerBitSize();
    CHECK_FATAL(offset >= 0, "offset must >= 0");
    if (offset > kStpLdpImm64UpperBound) {
        o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc, *static_cast<MemOperand *>(o2), dataSize, R16);
    }
    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
    // Identify that the instruction is not alias with any other memory instructions.
    auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
    memDefUse->SetIndependent();
    pushInsn.SetReferenceOsts(memDefUse);
    std::string comment = "SAVE CALLEE REGISTER PAIR";
    pushInsn.SetComment(comment);
    AppendInstructionTo(pushInsn, cgFunc);
}

void AArch64GenProEpilog::AppendInstructionPushSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPushOp][rty][kPushPopSingle];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, GetPointerBitSize(), rty);
    Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());

    MemOperand *aarchMemO1 = static_cast<MemOperand *>(o1);
    uint32 dataSize = GetPointerBitSize();
    if (aarchMemO1->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
        o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R16);
    }

    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o1);
    // Identify that the instruction is not alias with any other memory instructions.
    auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
    memDefUse->SetIndependent();
    pushInsn.SetReferenceOsts(memDefUse);
    std::string comment = "SAVE CALLEE REGISTER";
    pushInsn.SetComment(comment);
    AppendInstructionTo(pushInsn, cgFunc);
}

Insn &AArch64GenProEpilog::AppendInstructionForAllocateOrDeallocateCallFrame(int64 fpToSpDistance, AArch64reg reg0,
                                                                             AArch64reg reg1, RegType rty,
                                                                             bool isAllocate)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopPair];
    uint8 size;
    if (CGOptions::IsArm64ilp32()) {
        size = k8ByteSize;
    } else {
        size = GetPointerSize();
    }
    if (fpToSpDistance <= kStrLdrImm64UpperBound - kOffset8MemPos) {
        mOp = isAllocate ? pushPopOps[kRegsPushOp][rty][kPushPopSingle] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
        MemOperand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), size * kBitsPerByte);
        if (storeFP) {
            RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, size * kBitsPerByte, rty);
            Insn &insn1 = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o2);
            AppendInstructionTo(insn1, cgFunc);
        }
        RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, size * kBitsPerByte, rty);
        o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance + size), size * kBitsPerByte);
        Insn &insn2 = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
        AppendInstructionTo(insn2, cgFunc);
        return insn2;
    } else {
        RegOperand &oo = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R9, size * kBitsPerByte, kRegTyInt);
        ImmOperand &io1 = aarchCGFunc.CreateImmOperand(fpToSpDistance, k64BitSize, true);
        aarchCGFunc.SelectCopyImm(oo, io1, PTY_i64);
        RegOperand &rsp = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, size * kBitsPerByte, kRegTyInt);
        MemOperand *mo = aarchCGFunc.CreateMemOperand(MemOperand::kAddrModeBOrX, size * kBitsPerByte, rsp, oo, 0);
        if (storeFP) {
            RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, size * kBitsPerByte, rty);
            Insn &insn1 = cgFunc.GetInsnBuilder()->BuildInsn(isAllocate ? MOP_xstr : MOP_xldr, o0, *mo);
            AppendInstructionTo(insn1, cgFunc);
        }
        ImmOperand &io2 = aarchCGFunc.CreateImmOperand(size, k64BitSize, true);
        aarchCGFunc.SelectAdd(oo, oo, io2, PTY_i64);
        RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, size * kBitsPerByte, rty);
        mo = aarchCGFunc.CreateMemOperand(MemOperand::kAddrModeBOrX, size * kBitsPerByte, rsp, oo, 0);
        Insn &insn2 = cgFunc.GetInsnBuilder()->BuildInsn(isAllocate ? MOP_xstr : MOP_xldr, o1, *mo);
        AppendInstructionTo(insn2, cgFunc);
        return insn2;
    }
}

Insn &AArch64GenProEpilog::CreateAndAppendInstructionForAllocateCallFrame(int64 fpToSpDistance, AArch64reg reg0,
                                                                          AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = (storeFP || fpToSpDistance > kStrLdrPerPostUpperBound)
                        ? pushPopOps[kRegsPushOp][rty][kPushPopPair]
                        : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
    Insn *allocInsn = nullptr;
    if (fpToSpDistance > kStpLdpImm64UpperBound) {
        allocInsn = &AppendInstructionForAllocateOrDeallocateCallFrame(fpToSpDistance, reg0, reg1, rty, true);
    } else {
        Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
        Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
        Operand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), GetPointerBitSize());
        allocInsn = (storeFP || fpToSpDistance > kStrLdrPerPostUpperBound)
                        ? &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2)
                        : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
        AppendInstructionTo(*allocInsn, cgFunc);
    }
    return *allocInsn;
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame"));
    }

    Insn *ipoint = nullptr;
    /*
     * stackFrameSize includes the size of args to stack-pass
     * if a function has neither VLA nor alloca.
     */
    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    /*
     * ldp/stp's imm should be within -512 and 504;
     * if stp's imm > 512, we fall back to the stp-sub version
     */
    bool useStpSub = false;
    int64 offset = 0;
    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        /*
         * stack_frame_size == size of formal parameters + callee-saved (including FP/RL)
         *                     + size of local vars
         *                     + size of actuals
         * (when passing more than 8 args, its caller's responsibility to
         *  allocate space for it. size of actuals represent largest such size in the function.
         */
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
        ipoint = cgFunc.GetCurBB()->GetLastInsn();
    } else {
        if (stackFrameSize > kStpLdpImm64UpperBound) {
            useStpSub = true;
            offset = kOffset16MemPos;
            stackFrameSize -= offset;
        } else {
            offset = stackFrameSize;
        }
        MOperator mOp = (storeFP || offset > kStrLdrPerPostUpperBound) ? pushPopOps[kRegsPushOp][rty][kPushPopPair]
                                                                       : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
        RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
        RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
        MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(static_cast<int32>(-offset), GetPointerBitSize());
        ipoint = (storeFP || offset > kStrLdrPerPostUpperBound) ? &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2)
                                                                : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, o2);
        AppendInstructionTo(*ipoint, cgFunc);
    }

    ipoint->SetStackDef(true);

    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        CHECK_FATAL(!useStpSub, "Invalid assumption");
        ipoint = &CreateAndAppendInstructionForAllocateCallFrame(fpToSpDistance, reg0, reg1, rty);
    }

    CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
    if (useStpSub) {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
        ipoint = cgFunc.GetCurBB()->GetLastInsn();
        aarchCGFunc.SetUsedStpSubPairForCallFrameAllocation(true);
        ipoint->SetStackDef(true);
    }
}

void AArch64GenProEpilog::AppendInstructionAllocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame for debugging"));
    }

    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance =
        (cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot());

    Insn *ipoint = nullptr;

    if (fpToSpDistance > 0) {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
        ipoint = cgFunc.GetCurBB()->GetLastInsn();
        ipoint->SetStackDef(true);
        ipoint = &CreateAndAppendInstructionForAllocateCallFrame(fpToSpDistance, reg0, reg1, rty);
        CHECK_FATAL(ipoint != nullptr, "ipoint should not be nullptr at this point");
    } else {
        bool useStpSub = false;

        if (stackFrameSize > kStpLdpImm64UpperBound) {
            useStpSub = true;
            RegOperand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
            ImmOperand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
            aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
            ipoint = cgFunc.GetCurBB()->GetLastInsn();
            ipoint->SetStackDef(true);
        } else {
            MOperator mOp = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound)
                                ? pushPopOps[kRegsPushOp][rty][kPushPopPair]
                                : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
            RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
            RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
            MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(-stackFrameSize, GetPointerBitSize());
            ipoint = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound)
                         ? &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2)
                         : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, o2);
            AppendInstructionTo(*ipoint, cgFunc);
            ipoint->SetStackDef(true);
        }

        if (useStpSub) {
            MOperator mOp =
                storeFP ? pushPopOps[kRegsPushOp][rty][kPushPopPair] : pushPopOps[kRegsPushOp][rty][kPushPopSingle];
            RegOperand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
            RegOperand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
            MemOperand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, 0, GetPointerBitSize());
            ipoint = storeFP ? &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2)
                             : &cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
            AppendInstructionTo(*ipoint, cgFunc);
        }
    }
}

void AArch64GenProEpilog::GenerateFrameTypeSave(SaveInfo& frameTypeInfo, int32 stackSize, int64 fpToSpDistance)
{
    if (!frameTypeInfo.shouldSave) {
        return;
    }
    CHECK_FATAL(frameTypeInfo.offset < 0, "must be!!");
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    //mov
    auto &x10Opnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R10, k64BitSize, kRegTyInt);
    auto immOpnd = &aarchCGFunc.CreateImmOperand(frameTypeInfo.idx, k64BitSize, true);
    aarchCGFunc.SelectCopyImm(x10Opnd, *immOpnd, PTY_i64);
    // store
    Operand *o1 = aarchCGFunc.CreateStackMemOpnd(RSP,
        static_cast<uint32>(frameTypeInfo.offset) + fpToSpDistance, GetPointerBitSize());
    MemOperand *mem = static_cast<MemOperand *>(o1);
    uint32 dataSize = GetPointerBitSize();
    if (mem->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*mem, dataSize)) {
        o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*mem, dataSize, R16);
    }

    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(MOP_xstr, x10Opnd, *o1);
    AppendInstructionTo(pushInsn, cgFunc);
}

void AArch64GenProEpilog::GenerateFunctionSave(SaveInfo& funcInfo, int32 stackSize, int64 fpToSpDistance)
{
    if (!funcInfo.shouldSave) {
        return;
    }
    CHECK_FATAL(funcInfo.offset < 0, "must be!!");
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    auto &mirFunc = aarchCGFunc.GetFunction();
    CCLocInfo ploc;
    CCImpl &parmlocator = *aarchCGFunc.GetOrCreateLocator(CCImpl::GetCallConvKind(aarchCGFunc.GetFunction()));
    auto &x10Opnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R10, k64BitSize, kRegTyInt);
    CHECK_FATAL(static_cast<size_t>(funcInfo.idx) <= mirFunc.GetFormalCount(), "should be in range");
    for (size_t i = 0; i < mirFunc.GetFormalCount(); ++i) {
        MIRType *ty = mirFunc.GetNthParamType(i);
        parmlocator.LocateNextParm(*ty, ploc, (i == 0), mirFunc.GetMIRFuncType());
        if (i != static_cast<size_t>(funcInfo.idx)) {
            continue;
        }
        if (ploc.reg0 == kRinvalid) {
            Operand* o1 = aarchCGFunc.CreateStackMemOpnd(RSP, ploc.memOffset + stackSize, k64BitSize);
            uint32 dataSize = GetPointerBitSize();
            if (ploc.memOffset + stackSize > kStpLdpImm64UpperBound) {
                o1 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc,
                    static_cast<MemOperand &>(*o1), dataSize, R16);
            }
            Insn &ldrInsn = cgFunc.GetInsnBuilder()->BuildInsn(MOP_xldr, x10Opnd, *o1);
            AppendInstructionTo(ldrInsn, cgFunc);
        } else {
            auto &funcOpnd =
                aarchCGFunc.GetOrCreatePhysicalRegisterOperand((AArch64reg)ploc.GetReg0(), k64BitSize, kRegTyInt);
            aarchCGFunc.SelectCopy(x10Opnd, ploc.GetPrimTypeOfReg0(), funcOpnd, ploc.GetPrimTypeOfReg0());
        }
    }
    Operand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP,
        static_cast<uint32>(funcInfo.offset) + fpToSpDistance, GetPointerBitSize());
    MemOperand *mem2 = static_cast<MemOperand *>(o2);
    uint32 dataSize = GetPointerBitSize();
    if (mem2->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*mem2, dataSize)) {
        o2 = &aarchCGFunc.SplitOffsetWithAddInstruction(*mem2, dataSize, R16);
    }
    Insn &pushInsn2 = cgFunc.GetInsnBuilder()->BuildInsn(MOP_xstr, x10Opnd, *o2);
    AppendInstructionTo(pushInsn2, cgFunc);
}

void AArch64GenProEpilog::GenerateSave()
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    auto &mirFunc = aarchCGFunc.GetFunction();
    auto &frameTypeInfo =  mirFunc.GetFrameTypeInfo();
    auto &funcInfo = mirFunc.GetFuncInfo();

    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    GenerateFrameTypeSave(frameTypeInfo, stackFrameSize, fpToSpDistance);
    GenerateFunctionSave(funcInfo, stackFrameSize, fpToSpDistance);
}

/*
 *  From AArch64 Reference Manual
 *  C1.3.3 Load/Store Addressing Mode
 *  ...
 *  When stack alignment checking is enabled by system software and
 *  the base register is the SP, the current stack pointer must be
 *  initially quadword aligned, that is aligned to 16 bytes. Misalignment
 *  generates a Stack Alignment fault.  The offset does not have to
 *  be a multiple of 16 bytes unless the specific Load/Store instruction
 *  requires this. SP cannot be used as a register offset.
 */
void AArch64GenProEpilog::GeneratePushRegs()
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    const MapleVector<AArch64reg> &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty())
                                                    ? aarchCGFunc.GetCalleeSavedRegs()
                                                    : aarchCGFunc.GetProEpilogSavedRegs();

    CHECK_FATAL(!regsToSave.empty(), "FP/LR not added to callee-saved list?");

    AArch64reg intRegFirstHalf = kRinvalid;
    AArch64reg fpRegFirstHalf = kRinvalid;

    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("save callee-saved registers"));
    }

    /*
     * Even if we don't use RFP, since we push a pair of registers in one instruction
     * and the stack needs be aligned on a 16-byte boundary, push RFP as well if function has a call
     * Make sure this is reflected when computing callee_saved_regs.size()
     */
    if (!currCG->GenerateDebugFriendlyCode()) {
        AppendInstructionAllocateCallFrame(R29, RLR, kRegTyInt);
    } else {
        AppendInstructionAllocateCallFrameDebug(R29, RLR, kRegTyInt);
    }
    GenerateSave();
    if (useFP) {
        if (currCG->GenerateVerboseCG()) {
            cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("copy SP to FP"));
        }
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
        int64 fpToSpDistance =
            (cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot());
        bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
        if ((fpToSpDistance > 0) || isLmbc) {
            Operand *immOpnd;
            if (isLmbc) {
                int32 size =
                    static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
                immOpnd = &aarchCGFunc.CreateImmOperand(size, k32BitSize, true);
            } else {
                immOpnd = &aarchCGFunc.CreateImmOperand(fpToSpDistance, k32BitSize, true);
            }
            if (!isLmbc || cgFunc.SeenFP() || cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
                aarchCGFunc.SelectAdd(fpOpnd, spOpnd, *immOpnd, PTY_u64);
            }
            cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
        } else {
            aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
            cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
        }
    }

    MapleVector<AArch64reg>::const_iterator it = regsToSave.begin();
    // skip the RFP & RLR
    if (*it == RFP) {
        ++it;
    }
    CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
    ++it;

    // callee save offset
    // fp - callee save base = RealStackFrameSize - [GR,16] - [VR,16] - [cold,16] - [callee] - stack protect + 16(fplr)
    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    int32 offset = 0;
    if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        offset = static_cast<int32>((memLayout->RealStackFrameSize() - aarchCGFunc.SizeOfCalleeSaved()) -
                                    memLayout->GetSizeOfLocals());
    } else {
        offset = (static_cast<int32>(memLayout->RealStackFrameSize()) -
                  static_cast<int32>(static_cast<int32>(aarchCGFunc.SizeOfCalleeSaved()) -
                                     (kDivide2 * static_cast<int32>(kAarch64IntregBytelen))) - /* for FP/LR */
                  static_cast<int32>(memLayout->SizeOfArgsToStackPass()) -
                  static_cast<int32>(cgFunc.GetFunction().GetFrameReseverdSlot()));
    }

    offset -= static_cast<int32>(RoundUp(memLayout->GetSizeOfSegCold(), k16BitSize));

    std::vector<std::pair<uint16, int32>> calleeRegAndOffsetVec;
    for (; it != regsToSave.end(); ++it) {
        AArch64reg reg = *it;
        // skip the RFP
        if (reg == RFP) {
            continue;
        }
        CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");
        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
        if (firstHalf == kRinvalid) {
            /* remember it */
            firstHalf = reg;
        } else {
            uint16 reg0NO = (regType == kRegTyInt) ?
                static_cast<uint16>(firstHalf - 1) : static_cast<uint16>(firstHalf  - V8 + 72);
            uint16 reg1NO = (regType == kRegTyInt) ? static_cast<uint16>(reg - 1) : static_cast<uint16>(reg - V8 + 72);
            calleeRegAndOffsetVec.push_back(std::pair<uint16, int32>(reg0NO, offset));
            calleeRegAndOffsetVec.push_back(std::pair<uint16, int32>(reg1NO, offset + k8ByteSize));
            AppendInstructionPushPair(cgFunc, firstHalf, reg, regType, offset);
            AArch64isa::GetNextOffsetCalleeSaved(offset);
            firstHalf = kRinvalid;
        }
    }

    if (intRegFirstHalf != kRinvalid) {
        uint16 regNO = static_cast<uint16>(intRegFirstHalf - 1);
        calleeRegAndOffsetVec.push_back(std::pair<uint16, int32>(regNO, offset));
        AppendInstructionPushSingle(cgFunc, intRegFirstHalf, kRegTyInt, offset);
        AArch64isa::GetNextOffsetCalleeSaved(offset);
    }

    if (fpRegFirstHalf != kRinvalid) {
        uint16 regNO = static_cast<uint16>(fpRegFirstHalf - V8 + 72);
        calleeRegAndOffsetVec.push_back(std::pair<uint16, int32>(regNO, offset));
        AppendInstructionPushSingle(cgFunc, fpRegFirstHalf, kRegTyFloat, offset);
        AArch64isa::GetNextOffsetCalleeSaved(offset);
    }

    const auto &emitMemoryManager = CGOptions::GetInstance().GetEmitMemoryManager();
    if (emitMemoryManager.codeSpace != nullptr) {
        emitMemoryManager.funcCalleeOffsetSaver(emitMemoryManager.codeSpace, cgFunc.GetName(), calleeRegAndOffsetVec);
        int64 fpToCurSpDistance =
            (cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot());
        int32 fp2PrevFrameSPDelta =
            static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize()) -
	        fpToCurSpDistance;
        emitMemoryManager.funcFpSPDeltaSaver(emitMemoryManager.codeSpace, cgFunc.GetName(), fp2PrevFrameSPDelta);
    }
}

void AArch64GenProEpilog::GenerateProlog(BB &bb)
{
    if (!cgFunc.GetHasProEpilogue()) {
        return;
    }
    if (PROEPILOG_DUMP) {
        LogInfo::MapleLogger() << "generate prolog at BB " << bb.GetId() << "\n";
    }

    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    BB *formerCurBB = cgFunc.GetCurBB();
    aarchCGFunc.GetDummyBB()->ClearInsns();
    cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

    const MapleVector<AArch64reg> &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty())
                                                    ? aarchCGFunc.GetCalleeSavedRegs()
                                                    : aarchCGFunc.GetProEpilogSavedRegs();
    if (!regsToSave.empty()) {
        /*
         * Among other things, push the FP & LR pair.
         * FP/LR are added to the callee-saved list in AllocateRegisters()
         * We add them to the callee-saved list regardless of UseFP() being true/false.
         * Activation Frame is allocated as part of pushing FP/LR pair
         */
        GeneratePushRegs();
    } else {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        int32 stackFrameSize =
            static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
        if (stackFrameSize > 0) {
            if (currCG->GenerateVerboseCG()) {
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("allocate activation frame"));
            }
            Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
            aarchCGFunc.SelectSub(spOpnd, spOpnd, immOpnd, PTY_u64);
            cgFunc.GetCurBB()->GetLastInsn()->SetStackDef(true);
        }
        if (currCG->GenerateVerboseCG()) {
            cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("copy SP to FP"));
        }
        if (useFP) {
            Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
            bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
            int64 fpToSpDistance =
                cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
            if ((fpToSpDistance > 0) || isLmbc) {
                Operand *immOpnd;
                if (isLmbc) {
                    int32 size = static_cast<int32>(
                        static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
                    immOpnd = &aarchCGFunc.CreateImmOperand(size, k32BitSize, true);
                } else {
                    immOpnd = &aarchCGFunc.CreateImmOperand(fpToSpDistance, k32BitSize, true);
                }
                aarchCGFunc.SelectAdd(fpOpnd, spOpnd, *immOpnd, PTY_u64);
                cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
            } else {
                aarchCGFunc.SelectCopy(fpOpnd, PTY_u64, spOpnd, PTY_u64);
                cgFunc.GetCurBB()->GetLastInsn()->SetFrameDef(true);
            }
        }
    }
    bb.InsertAtBeginning(*aarchCGFunc.GetDummyBB());
    cgFunc.SetCurBB(*formerCurBB);
}

void AArch64GenProEpilog::GenerateRet(BB &bb)
{
    auto *lastInsn = bb.GetLastMachineInsn();
    if (lastInsn != nullptr && (lastInsn->IsTailCall() || lastInsn->IsBranch())) {
        return;
    }
    bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildInsn<AArch64CG>(MOP_xret));
}

void AArch64GenProEpilog::AppendInstructionPopSingle(CGFunc &cgFunc, AArch64reg reg, RegType rty, int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopSingle];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg, GetPointerBitSize(), rty);
    Operand *o1 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());
    MemOperand *aarchMemO1 = static_cast<MemOperand *>(o1);
    uint32 dataSize = GetPointerBitSize();
    if (aarchMemO1->GetMemVaryType() == kNotVary && aarchCGFunc.IsImmediateOffsetOutOfRange(*aarchMemO1, dataSize)) {
        o1 = &aarchCGFunc.SplitOffsetWithAddInstruction(*aarchMemO1, dataSize, R16);
    }

    Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, *o1);
    // Identify that the instruction is not alias with any other memory instructions.
    auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
    memDefUse->SetIndependent();
    popInsn.SetReferenceOsts(memDefUse);
    popInsn.SetComment("RESTORE");
    cgFunc.GetCurBB()->AppendInsn(popInsn);
}

void AArch64GenProEpilog::AppendInstructionPopPair(CGFunc &cgFunc, AArch64reg reg0, AArch64reg reg1, RegType rty,
                                                   int32 offset)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
    Operand *o2 = &aarchCGFunc.CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerBitSize());

    uint32 dataSize = GetPointerBitSize();
    CHECK_FATAL(offset >= 0, "offset must >= 0");
    if (offset > kStpLdpImm64UpperBound) {
        o2 = SplitStpLdpOffsetForCalleeSavedWithAddInstruction(cgFunc, static_cast<MemOperand &>(*o2), dataSize, R16);
    }
    Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
    // Identify that the instruction is not alias with any other memory instructions.
    auto *memDefUse = cgFunc.GetFuncScopeAllocator()->New<MemDefUse>(*cgFunc.GetFuncScopeAllocator());
    memDefUse->SetIndependent();
    popInsn.SetReferenceOsts(memDefUse);
    popInsn.SetComment("RESTORE RESTORE");
    cgFunc.GetCurBB()->AppendInsn(popInsn);
}

void AArch64GenProEpilog::AppendInstructionDeallocateCallFrame(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    /*
     * ldp/stp's imm should be within -512 and 504;
     * if ldp's imm > 504, we fall back to the ldp-add version
     */
    bool useLdpAdd = false;
    int32 offset = 0;

    Operand *o2 = nullptr;
    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), GetPointerBitSize());
    } else {
        if (stackFrameSize > kStpLdpImm64UpperBound) {
            useLdpAdd = true;
            offset = kOffset16MemPos;
            stackFrameSize -= offset;
        } else {
            offset = stackFrameSize;
        }
        o2 = &aarchCGFunc.CreateCallFrameOperand(offset, GetPointerBitSize());
    }

    if (useLdpAdd) {
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    }

    if (!cgFunc.HasVLAOrAlloca() && fpToSpDistance > 0) {
        CHECK_FATAL(!useLdpAdd, "Invalid assumption");
        if (fpToSpDistance > kStpLdpImm64UpperBound) {
            (void)AppendInstructionForAllocateOrDeallocateCallFrame(fpToSpDistance, reg0, reg1, rty, false);
        } else {
            Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
        }
        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    } else {
        Insn &deallocInsn = cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2);
        cgFunc.GetCurBB()->AppendInsn(deallocInsn);
    }
    cgFunc.GetCurBB()->GetLastInsn()->SetStackRevert(true);
}

void AArch64GenProEpilog::AppendInstructionDeallocateCallFrameDebug(AArch64reg reg0, AArch64reg reg1, RegType rty)
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    MOperator mOp = pushPopOps[kRegsPopOp][rty][kPushPopPair];
    Operand &o0 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg0, GetPointerBitSize(), rty);
    Operand &o1 = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(reg1, GetPointerBitSize(), rty);
    int32 stackFrameSize =
        static_cast<int32>(static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize());
    int64 fpToSpDistance = cgFunc.GetMemlayout()->SizeOfArgsToStackPass() + cgFunc.GetFunction().GetFrameReseverdSlot();
    /*
     * ldp/stp's imm should be within -512 and 504;
     * if ldp's imm > 504, we fall back to the ldp-add version
     */
    bool isLmbc = (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc);
    if (cgFunc.HasVLAOrAlloca() || fpToSpDistance == 0 || isLmbc) {
        int32 lmbcOffset = 0;
        if (!isLmbc) {
            stackFrameSize -= fpToSpDistance;
        } else {
            lmbcOffset = fpToSpDistance - (kDivide2 * k8ByteSize);
        }
        if (stackFrameSize > kStpLdpImm64UpperBound || isLmbc) {
            Operand *o2 = aarchCGFunc.CreateStackMemOpnd(RSP, (isLmbc ? lmbcOffset : 0), GetPointerBitSize());
            mOp = storeFP ? pushPopOps[kRegsPopOp][rty][kPushPopPair] : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
            Insn &deallocInsn = storeFP ? cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2)
                                        : cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
            Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
            Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
            aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
        } else {
            MemOperand &o2 = aarchCGFunc.CreateCallFrameOperand(stackFrameSize, GetPointerBitSize());
            mOp = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound) ? pushPopOps[kRegsPopOp][rty][kPushPopPair]
                                                                         : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
            Insn &deallocInsn = (storeFP || stackFrameSize > kStrLdrPerPostUpperBound)
                                    ? cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, o2)
                                    : cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
        }
    } else {
        Operand *o2;
        o2 = aarchCGFunc.CreateStackMemOpnd(RSP, static_cast<int32>(fpToSpDistance), GetPointerBitSize());
        if (fpToSpDistance > kStpLdpImm64UpperBound) {
            (void)AppendInstructionForAllocateOrDeallocateCallFrame(fpToSpDistance, reg0, reg1, rty, false);
        } else {
            mOp = (storeFP || fpToSpDistance > kStrLdrPerPostUpperBound) ? pushPopOps[kRegsPopOp][rty][kPushPopPair]
                                                                         : pushPopOps[kRegsPopOp][rty][kPushPopSingle];
            Insn &deallocInsn = (storeFP || fpToSpDistance > kStrLdrPerPostUpperBound)
                                    ? cgFunc.GetInsnBuilder()->BuildInsn(mOp, o0, o1, *o2)
                                    : cgFunc.GetInsnBuilder()->BuildInsn(mOp, o1, *o2);
            cgFunc.GetCurBB()->AppendInsn(deallocInsn);
        }

        Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
        aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
    }
    cgFunc.GetCurBB()->GetLastInsn()->SetStackRevert(true);
}

void AArch64GenProEpilog::GeneratePopRegs()
{
    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();

    const MapleVector<AArch64reg> &regsToRestore = (aarchCGFunc.GetProEpilogSavedRegs().empty())
                                                       ? aarchCGFunc.GetCalleeSavedRegs()
                                                       : aarchCGFunc.GetProEpilogSavedRegs();

    CHECK_FATAL(!regsToRestore.empty(), "FP/LR not added to callee-saved list?");

    AArch64reg intRegFirstHalf = kRinvalid;
    AArch64reg fpRegFirstHalf = kRinvalid;

    if (currCG->GenerateVerboseCG()) {
        cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("restore callee-saved registers"));
    }

    MapleVector<AArch64reg>::const_iterator it = regsToRestore.begin();
    /*
     * Even if we don't use FP, since we push a pair of registers
     * in a single instruction (i.e., stp) and the stack needs be aligned
     * on a 16-byte boundary, push FP as well if the function has a call.
     * Make sure this is reflected when computing calleeSavedRegs.size()
     * skip the first two registers
     */
    // skip the RFP & RLR
    if (*it == RFP) {
        ++it;
    }
    CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
    ++it;

    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout());
    int32 offset;
    if (cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        offset = static_cast<int32>((memLayout->RealStackFrameSize() - aarchCGFunc.SizeOfCalleeSaved()) -
                                    memLayout->GetSizeOfLocals());
    } else {
        offset = (static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize() -
                  (aarchCGFunc.SizeOfCalleeSaved() - (kDivide2 * kAarch64IntregBytelen))) -
                 memLayout->SizeOfArgsToStackPass() -
                 cgFunc.GetFunction().GetFrameReseverdSlot();
    }
    offset -= static_cast<int32>(RoundUp(memLayout->GetSizeOfSegCold(), k16BitSize));

    /*
     * We are using a cleared dummy block; so insertPoint cannot be ret;
     * see GenerateEpilog()
     */
    for (; it != regsToRestore.end(); ++it) {
        AArch64reg reg = *it;
        if (reg == RFP) {
            continue;
        }
        CHECK_FATAL(reg != RLR, "stray RLR in callee_saved_list?");

        RegType regType = AArch64isa::IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
        AArch64reg &firstHalf = AArch64isa::IsGPRegister(reg) ? intRegFirstHalf : fpRegFirstHalf;
        if (firstHalf == kRinvalid) {
            /* remember it */
            firstHalf = reg;
        } else {
            /* flush the pair */
            AppendInstructionPopPair(cgFunc, firstHalf, reg, regType, offset);
            AArch64isa::GetNextOffsetCalleeSaved(offset);
            firstHalf = kRinvalid;
        }
    }

    if (intRegFirstHalf != kRinvalid) {
        AppendInstructionPopSingle(cgFunc, intRegFirstHalf, kRegTyInt, offset);
        AArch64isa::GetNextOffsetCalleeSaved(offset);
    }

    if (fpRegFirstHalf != kRinvalid) {
        AppendInstructionPopSingle(cgFunc, fpRegFirstHalf, kRegTyFloat, offset);
        AArch64isa::GetNextOffsetCalleeSaved(offset);
    }

    if (!currCG->GenerateDebugFriendlyCode()) {
        AppendInstructionDeallocateCallFrame(R29, RLR, kRegTyInt);
    } else {
        AppendInstructionDeallocateCallFrameDebug(R29, RLR, kRegTyInt);
    }
}

void AArch64GenProEpilog::AppendBBtoEpilog(BB &epilogBB, BB &newBB)
{
    FOR_BB_INSNS(insn, &newBB)
    {
        insn->SetDoNotRemove(true);
    }
    auto *lastInsn = epilogBB.GetLastMachineInsn();
    if (lastInsn != nullptr && (lastInsn->IsTailCall() || lastInsn->IsBranch())) {
        epilogBB.RemoveInsn(*lastInsn);
        epilogBB.AppendBBInsns(newBB);
        epilogBB.AppendInsn(*lastInsn);
    } else {
        epilogBB.AppendBBInsns(newBB);
    }
}

void AArch64GenProEpilog::GenerateEpilog(BB &bb)
{
    if (!cgFunc.GetHasProEpilogue()) {
        return;
    }
    if (PROEPILOG_DUMP) {
        LogInfo::MapleLogger() << "generate epilog at BB " << bb.GetId() << "\n";
    }

    auto &aarchCGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    CG *currCG = cgFunc.GetCG();
    BB *formerCurBB = cgFunc.GetCurBB();
    aarchCGFunc.GetDummyBB()->ClearInsns();
    cgFunc.SetCurBB(*aarchCGFunc.GetDummyBB());

    Operand &spOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    Operand &fpOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);

    if (cgFunc.HasVLAOrAlloca() && cgFunc.GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
        aarchCGFunc.SelectCopy(spOpnd, PTY_u64, fpOpnd, PTY_u64);
    }

    const MapleVector<AArch64reg> &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty())
                                                    ? aarchCGFunc.GetCalleeSavedRegs()
                                                    : aarchCGFunc.GetProEpilogSavedRegs();
    if (!regsToSave.empty()) {
        GeneratePopRegs();
    } else {
        auto stackFrameSize = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->RealStackFrameSize();
        if (stackFrameSize > 0) {
            if (currCG->GenerateVerboseCG()) {
                cgFunc.GetCurBB()->AppendInsn(aarchCGFunc.CreateCommentInsn("pop up activation frame"));
            }

            if (cgFunc.HasVLAOrAlloca()) {
                auto size = static_cast<AArch64MemLayout *>(cgFunc.GetMemlayout())->GetSegArgsToStkPass().GetSize();
                stackFrameSize = stackFrameSize < size ? 0 : stackFrameSize - size;
            }

            if (stackFrameSize > 0) {
                Operand &immOpnd = aarchCGFunc.CreateImmOperand(stackFrameSize, k32BitSize, true);
                aarchCGFunc.SelectAdd(spOpnd, spOpnd, immOpnd, PTY_u64);
                aarchCGFunc.GetCurBB()->GetLastInsn()->SetStackRevert(true);
            }
        }
    }

    AppendBBtoEpilog(bb, *cgFunc.GetCurBB());
    if (cgFunc.GetCurBB()->GetHasCfi()) {
        bb.SetHasCfi();
    }

    cgFunc.SetCurBB(*formerCurBB);
}

void AArch64GenProEpilog::Run()
{
    CHECK_FATAL(cgFunc.GetFunction().GetBody()->GetFirst()->GetOpCode() == OP_label,
                "The first statement should be a label");
    // update exitBB
    if (cgFunc.IsExitBBsVecEmpty()) {
        cgFunc.PushBackExitBBsVec(*cgFunc.GetLastBB()->GetPrev());
    }
    cgFunc.SetHasProEpilogue(true);

    // not run proepilog analysis or analysis failed, insert proepilog at firstBB and exitBB
    GenerateProlog(*(cgFunc.GetFirstBB()));
    for (auto *exitBB : cgFunc.GetExitBBsVec()) {
        GenerateEpilog(*exitBB);
    }

    // insert ret insn for exitBB
    for (auto *exitBB : cgFunc.GetExitBBsVec()) {
        if (cgFunc.GetHasProEpilogue()) {
            GenerateRet(*exitBB);
        }
    }
}
} /* namespace maplebe */
