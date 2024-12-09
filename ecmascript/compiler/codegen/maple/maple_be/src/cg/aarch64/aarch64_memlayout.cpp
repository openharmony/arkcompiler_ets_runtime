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

#include "aarch64_memlayout.h"
#include "aarch64_cgfunc.h"
#include "becommon.h"
#include "mir_nodes.h"

namespace maplebe {
using namespace maple;

/*
 *  Returns stack space required for a call
 *  which is used to pass arguments that cannot be
 *  passed through registers
 */
uint32 AArch64MemLayout::ComputeStackSpaceRequirementForCall(StmtNode &stmt, int32 &aggCopySize, bool isIcall)
{
    /* instantiate a parm locator */
    CHECK_FATAL(cgFunc != nullptr, "nullptr check");
    CCImpl &parmLocator = *static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateLocator(CCImpl::GetCallConvKind(stmt));
    uint32 sizeOfArgsToStkPass = 0;
    uint32 i = 0;
    /* An indirect call's first operand is the invocation target */
    if (isIcall) {
        ++i;
    }

    if (stmt.GetOpCode() == OP_call) {
        CallNode *callNode = static_cast<CallNode *>(&stmt);
        MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
        CHECK_FATAL(fn != nullptr, "get MIRFunction failed");
        CHECK_NULL_FATAL(be.GetMIRModule().CurFunction());
    }

    aggCopySize = 0;
    for (uint32 anum = 0; i < stmt.NumOpnds(); ++i, ++anum) {
        BaseNode *opnd = stmt.Opnd(i);
        MIRType *ty = nullptr;
        ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<uint32>(opnd->GetPrimType())];
        CCLocInfo ploc;
        aggCopySize += static_cast<int32>(parmLocator.LocateNextParm(*ty, ploc));
        if (ploc.reg0 != 0) {
            continue; /* passed in register, so no effect on actual area */
        }
        sizeOfArgsToStkPass = static_cast<uint32>(
            RoundUp(static_cast<uint32>(ploc.memOffset + ploc.memSize), static_cast<uint64>(GetPointerSize())));
    }
    return sizeOfArgsToStkPass;
}

void AArch64MemLayout::SetSizeAlignForTypeIdx(uint32 typeIdx, uint32 &size, uint32 &align) const
{
    auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx);
    
    align = mirType->GetAlign();
    size = static_cast<uint32>(mirType->GetSize());
}

void AArch64MemLayout::LayoutFormalParams()
{
    CCImpl &parmLocator = *static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateLocator(cgFunc->GetCurCallConvKind());
    CCLocInfo ploc;
    for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
        MIRSymbol *sym = mirFunction->GetFormal(i);
        uint32 stIndex = sym->GetStIndex();
        AArch64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
        SetSymAllocInfo(stIndex, *symLoc);
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(mirFunction->GetFormalDefVec()[i].formalTyIdx);
        CHECK_FATAL(ty != nullptr, "nullptr check");
        uint32 ptyIdx = ty->GetTypeIndex();
        parmLocator.LocateNextParm(*ty, ploc, i == 0, mirFunction->GetMIRFuncType());
        if (ploc.reg0 != kRinvalid) { /* register */
            symLoc->SetRegisters(static_cast<AArch64reg>(ploc.reg0), static_cast<AArch64reg>(ploc.reg1),
                                 static_cast<AArch64reg>(ploc.reg2), static_cast<AArch64reg>(ploc.reg3));
            if (!sym->IsPreg()) {
                uint32 size;
                uint32 align;
                SetSizeAlignForTypeIdx(ptyIdx, size, align);
                symLoc->SetMemSegment(GetSegArgsRegPassed());
                /* the type's alignment requirement may be smaller than a registser's byte size */
                segArgsRegPassed.SetSize(static_cast<uint32>(RoundUp(segArgsRegPassed.GetSize(), align)));
                symLoc->SetOffset(segArgsRegPassed.GetSize());
                segArgsRegPassed.SetSize(segArgsRegPassed.GetSize() + size);
            }
        } else { /* stack */
            uint32 size;
            uint32 align;
            SetSizeAlignForTypeIdx(ptyIdx, size, align);
            symLoc->SetMemSegment(GetSegArgsStkPassed());
            segArgsStkPassed.SetSize(static_cast<uint32>(RoundUp(segArgsStkPassed.GetSize(), align)));
            symLoc->SetOffset(segArgsStkPassed.GetSize());
            segArgsStkPassed.SetSize(segArgsStkPassed.GetSize() + size);
            /* We need it as dictated by the AArch64 ABI $5.4.2 C12 */
            segArgsStkPassed.SetSize(static_cast<uint32>(RoundUp(segArgsStkPassed.GetSize(), GetPointerSize())));
        }
    }
}

// stack frame layout V2.1
// stack frame -> layout out some local variable on cold zone
// 1. layout small variables near sp
// 2. layout cold variables in cold area
// ||----------------------------|
// | args passed on the stack    |
// ||----------------------------|
// | GR saved area | 16 byte align|
// ||---------------------------|
// | VR saved area | 16 byte align |
// ||---------------------------- |
// | stack protect area | total 16 byte|
// ||----------------------------|
// | cold area |  16 byte align  |
// ||----------------------------| <- unadjustvary base
// | callee saved               |
// ||----------------------------|
// | spill                      |
// ||----------------------------|
// | reg saved                  |
// ||----------------------------|
// | local variables             |
// ||----------------------------|
// | PREV_FP, PREV_LR           |
// ||----------------------------|<- Frame Pointer
// | variable-sized local vars  |
// | (VLAs)                     |
// ||----------------------------|
// | args to pass through stack |
// ||----------------------------|

void AArch64MemLayout::LayoutLocalVariables(std::vector<MIRSymbol *> &tempVar, std::vector<MIRSymbol *> &returnDelays)
{
    uint32 symTabSize = mirFunction->GetSymTab()->GetSymbolTableSize();
    for (uint32 i = 0; i < symTabSize; ++i) {
        MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(i);
        if (sym == nullptr || sym->GetStorageClass() != kScAuto || sym->IsDeleted()) {
            continue;
        }
        uint32 stIndex = sym->GetStIndex();
        TyIdx tyIdx = sym->GetTyIdx();
        auto *symLoc = memAllocator->GetMemPool()->New<AArch64SymbolAlloc>();
        SetSymAllocInfo(stIndex, *symLoc);
        CHECK_FATAL(!symLoc->IsRegister(), "expect not register");
        symLoc->SetMemSegment(segLocals);
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        uint32 align = ty->GetAlign();
        segLocals.SetSize(static_cast<uint32>(RoundUp(segLocals.GetSize(), align)));
        symLoc->SetOffset(segLocals.GetSize());
        segLocals.SetSize(segLocals.GetSize() +
                            static_cast<uint32>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetSize()));
    }
}

void AArch64MemLayout::LayoutReturnRef(std::vector<MIRSymbol *> &returnDelays, int32 &structCopySize,
                                       int32 &maxParmStackSize)
{
    segArgsToStkPass.SetSize(FindLargestActualArea(structCopySize));
    maxParmStackSize = static_cast<int32>(segArgsToStkPass.GetSize());
    segRefLocals.SetSize(static_cast<uint32>(RoundUp(segRefLocals.GetSize(), GetPointerSize())));
    segLocals.SetSize(static_cast<uint32>(RoundUp(segLocals.GetSize(), GetPointerSize())));
}

void AArch64MemLayout::LayoutActualParams()
{
    for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
        MIRSymbol *sym = mirFunction->GetFormal(i);
        if (sym->IsPreg()) {
            continue;
        }
        uint32 stIndex = sym->GetStIndex();
        AArch64SymbolAlloc *symLoc = static_cast<AArch64SymbolAlloc *>(GetSymAllocInfo(stIndex));
        if (symLoc->GetMemSegment() == &GetSegArgsRegPassed()) { /* register */
            /*
             *  In O0, we store parameters passed via registers into memory.
             *  So, each of such parameter needs to get assigned storage in stack.
             *  If a function parameter is never accessed in the function body,
             *  and if we don't create its memory operand here, its offset gets
             *  computed when the instruction to store its value into stack
             *  is generated in the prologue when its memory operand is created.
             *  But, the parameter would see a different StackFrameSize than
             *  the parameters that are accessed in the body, because
             *  the size of the storage for FP/LR is added to the stack frame
             *  size in between.
             *  To make offset assignment easier, we create a memory operand
             *  for each of function parameters in advance.
             *  This has to be done after all of formal parameters and local
             *  variables get assigned their respecitve storage, i.e.
             *  CallFrameSize (discounting callee-saved and FP/LR) is known.
             */
            MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(mirFunction->GetFormalDefVec()[i].formalTyIdx);
            uint32 ptyIdx = ty->GetTypeIndex();
            static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateMemOpnd(
                *sym, 0, GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx)->GetAlign() * kBitsPerByte);
        }
    }
}

void AArch64MemLayout::LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize)
{
    LayoutFormalParams();
    /*
     * We do need this as LDR/STR with immediate
     * requires imm be aligned at a 8/4-byte boundary,
     * and local varirables may need 8-byte alignment.
     */
    if (CGOptions::IsArm64ilp32()) {
        segArgsRegPassed.SetSize(RoundUp(segArgsRegPassed.GetSize(), k8ByteSize));
        /* we do need this as SP has to be aligned at a 16-bytes bounardy */
        segArgsStkPassed.SetSize(RoundUp(segArgsStkPassed.GetSize(), k8ByteSize + k8ByteSize));
    } else {
        segArgsRegPassed.SetSize(static_cast<uint32>(RoundUp(segArgsRegPassed.GetSize(), GetPointerSize())));
        segArgsStkPassed.SetSize(
            static_cast<uint32>(RoundUp(segArgsStkPassed.GetSize(), GetPointerSize() + GetPointerSize())));
    }
    /* allocate the local variables in the stack */
    std::vector<MIRSymbol *> eaTempVar;
    std::vector<MIRSymbol *> retDelays;
    LayoutLocalVariables(eaTempVar, retDelays);

    /* handle ret_ref sym now */
    LayoutReturnRef(retDelays, structCopySize, maxParmStackSize);

    /*
     * for the actual arguments that cannot be pass through registers
     * need to allocate space for caller-save registers
     */
    LayoutActualParams();

    fixStackSize = static_cast<int32>(RealStackFrameSize());
    cgFunc->SetUseFP(cgFunc->UseFP() || fixStackSize > kMaxPimm32);
}
// from cold area to bottom of stk
// [cold,16] + [GR, 16] + [VR, 16] + stack protect (if has)
uint64 AArch64MemLayout::GetSizeOfColdToStk() const
{
    uint64 total = 0;
    auto coldsize = RoundUp(GetSizeOfSegCold(), k16BitSize);
    total += coldsize;
    return total;
}

bool AArch64MemLayout::IsSegMentVaried(const MemSegment *seg) const
{
    if (seg->GetMemSegmentKind() == kMsArgsStkPassed || seg->GetMemSegmentKind() == kMsCold) {
        return true;
    }
    return false;
}

uint64 AArch64MemLayout::StackFrameSize() const
{
    // regpassed + calleesaved + reflocals + locals + spill + cold + args to callee
    uint64 total = segArgsRegPassed.GetSize() + static_cast<AArch64CGFunc *>(cgFunc)->SizeOfCalleeSaved() +
                   GetSizeOfRefLocals() + Locals().GetSize() + GetSizeOfSpillReg() +
                   cgFunc->GetFunction().GetFrameReseverdSlot();

    auto coldsize = RoundUp(GetSizeOfSegCold(), k16BitSize);
    total += coldsize;
    total += segArgsToStkPass.GetSize();
    return RoundUp(total, kAarch64StackPtrAlignment);
}

// [regpass] + [callee save] + [reflocal] + [local] + [spill] + [cold,16] + [GR,16] + [VR,16] + stack protect (if has)
uint32 AArch64MemLayout::RealStackFrameSize() const
{
    auto size = StackFrameSize();
    return static_cast<uint32>(size);
}
// fp - callee base =
// RealStackFrameSize - [GR,16] - [VR,16] - [cold,16] - ([callee] - 16(fplr)) - stack protect - stkpass
// callsave area size includes fp lr, real callee save area size is callee save size - fplr
// fp lr located on top of args pass area.
int32 AArch64MemLayout::GetCalleeSaveBaseLoc() const
{
    DEBUG_ASSERT(cgFunc != nullptr, "cgfunc shound not be nullptr");
    uint32 offset = RealStackFrameSize() - static_cast<AArch64CGFunc *>(cgFunc)->SizeOfCalleeSaved();
    offset = (offset - SizeOfArgsToStackPass()) + kAarch64SizeOfFplr - cgFunc->GetFunction().GetFrameReseverdSlot();
    offset -= static_cast<uint32>(RoundUp(GetSizeOfSegCold(), k16BitSize));
    return static_cast<int32>(offset);
}
} /* namespace maplebe */
