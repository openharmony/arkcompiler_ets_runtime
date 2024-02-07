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

#include "x64_memlayout.h"
#include "x64_cgfunc.h"
#include "becommon.h"
#include "mir_nodes.h"
#include "x64_call_conv.h"
#include "cg.h"

namespace maplebe {
using namespace maple;

uint32 X64MemLayout::ComputeStackSpaceRequirementForCall(StmtNode &stmt, int32 &aggCopySize, bool isIcall)
{
    /* instantiate a parm locator */
    X64CallConvImpl parmLocator(cgFunc->GetBecommon(), X64CallConvImpl::GetCallConvKind(stmt));
    uint32 sizeOfArgsToStkPass = 0;
    size_t i = 0;
    /* An indirect call's first operand is the invocation target */
    if (isIcall) {
        ++i;
    }

    aggCopySize = 0;
    for (uint32 anum = 0; i < stmt.NumOpnds(); ++i, ++anum) {
        BaseNode *opnd = stmt.Opnd(i);
        MIRType *ty = nullptr;
        if (opnd->GetPrimType() != PTY_agg) {
            ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<uint32>(opnd->GetPrimType())];
        } else {
            Opcode opndOpcode = opnd->GetOpCode();
            DEBUG_ASSERT(opndOpcode == OP_dread || opndOpcode == OP_iread, "opndOpcode should be OP_dread or OP_iread");
            if (opndOpcode == OP_dread) {
                DreadNode *dread = static_cast<DreadNode *>(opnd);
                MIRSymbol *sym = be.GetMIRModule().CurFunction()->GetLocalOrGlobalSymbol(dread->GetStIdx());
                ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
                if (dread->GetFieldID() != 0) {
                    DEBUG_ASSERT(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass ||
                                     ty->GetKind() == kTypeUnion,
                                 "expect struct or class");
                    if (ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeUnion) {
                        ty = static_cast<MIRStructType *>(ty)->GetFieldType(dread->GetFieldID());
                    } else {
                        ty = static_cast<MIRClassType *>(ty)->GetFieldType(dread->GetFieldID());
                    }
                }
            } else {
                /* OP_iread */
                IreadNode *iread = static_cast<IreadNode *>(opnd);
                ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
                DEBUG_ASSERT(ty->GetKind() == kTypePointer, "expect pointer");
                ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType *>(ty)->GetPointedTyIdx());
                if (iread->GetFieldID() != 0) {
                    DEBUG_ASSERT(ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeClass ||
                                     ty->GetKind() == kTypeUnion,
                                 "expect struct or class");
                    if (ty->GetKind() == kTypeStruct || ty->GetKind() == kTypeUnion) {
                        ty = static_cast<MIRStructType *>(ty)->GetFieldType(iread->GetFieldID());
                    } else {
                        ty = static_cast<MIRClassType *>(ty)->GetFieldType(iread->GetFieldID());
                    }
                }
            }
        }
        CCLocInfo ploc;
        aggCopySize += parmLocator.LocateNextParm(*ty, ploc);
        if (ploc.reg0 != 0) {
            continue; /* passed in register, so no effect on actual area */
        }
        sizeOfArgsToStkPass = RoundUp(ploc.memOffset + ploc.memSize, GetPointerSize());
    }

    return sizeOfArgsToStkPass;
}

void X64MemLayout::SetSizeAlignForTypeIdx(uint32 typeIdx, uint32 &size, uint32 &align) const
{
    align = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx)->GetAlign();
    size = GlobalTables::GetTypeTable().GetTypeFromTyIdx(typeIdx)->GetSize();
}

void X64MemLayout::LayoutVarargParams()
{
    uint32 nIntRegs = 0;
    uint32 nFpRegs = 0;
    X64CallConvImpl parmlocator(be);
    CCLocInfo ploc;
    MIRFunction *func = mirFunction;
    if (be.GetMIRModule().IsCModule() && func->GetAttr(FUNCATTR_varargs)) {
        for (uint32 i = 0; i < func->GetFormalCount(); i++) {
            if (i == 0) {
                if (be.HasFuncReturnType(*func)) {
                    TyIdx tidx = be.GetFuncReturnType(*func);
                    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(tidx.GetIdx())->GetSize() <= k16ByteSize) {
                        continue;
                    }
                }
            }
            MIRType *ty = func->GetNthParamType(i);
            parmlocator.LocateNextParm(*ty, ploc, i == 0, func);
            if (ploc.reg0 != kRinvalid) {
                /* The range here is R0 to R15. However, not all registers in the range are parameter registers.
                 * If necessary later, you can add parameter register checks. */
                if (ploc.reg0 >= R0 && ploc.reg0 <= R15) {
                    nIntRegs++;
                } else if (ploc.reg0 >= V0 && ploc.reg0 <= V7) {
                    nFpRegs++;
                }
            }
            if (ploc.reg1 != kRinvalid) {
                if (ploc.reg1 >= R0 && ploc.reg1 <= R15) {
                    nIntRegs++;
                } else if (ploc.reg1 >= V0 && ploc.reg1 <= V7) {
                    nFpRegs++;
                }
            }
            if (ploc.reg2 != kRinvalid) {
                if (ploc.reg2 >= R0 && ploc.reg2 <= R15) {
                    nIntRegs++;
                } else if (ploc.reg2 >= V0 && ploc.reg2 <= V7) {
                    nFpRegs++;
                }
            }
            if (ploc.reg3 != kRinvalid) {
                if (ploc.reg3 >= R0 && ploc.reg3 <= R15) {
                    nIntRegs++;
                } else if (ploc.reg2 >= V0 && ploc.reg2 <= V7) {
                    nFpRegs++;
                }
            }
        }

        SetSizeOfGRSaveArea((k6BitSize - nIntRegs) * GetPointerSize());
        SetSizeOfVRSaveArea((k6BitSize - nFpRegs) * GetPointerSize() * k2ByteSize);
    }
}

void X64MemLayout::LayoutFormalParams()
{
    X64CallConvImpl parmLocator(be);
    CCLocInfo ploc;
    for (size_t i = 0; i < mirFunction->GetFormalCount(); ++i) {
        MIRSymbol *sym = mirFunction->GetFormal(i);
        uint32 stIndex = sym->GetStIndex();
        X64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<X64SymbolAlloc>();
        SetSymAllocInfo(stIndex, *symLoc);
        if (i == 0) {
            // The function name here is not appropriate, it should be to determine
            // whether the function returns a structure less than 16 bytes. At this
            // time, the first parameter is a structure occupant, which has no
            // practical significance.
            if (be.HasFuncReturnType(*mirFunction)) {
                symLoc->SetMemSegment(GetSegArgsRegPassed());
                symLoc->SetOffset(GetSegArgsRegPassed().GetSize());
                continue;
            }
        }

        MIRType *ty = mirFunction->GetNthParamType(i);
        uint32 ptyIdx = ty->GetTypeIndex();
        parmLocator.LocateNextParm(*ty, ploc, i == 0, mirFunction);
        uint32 size = 0;
        uint32 align = 0;
        if (ploc.reg0 != kRinvalid) {
            if (!sym->IsPreg()) {
                SetSizeAlignForTypeIdx(ptyIdx, size, align);
                symLoc->SetMemSegment(GetSegArgsRegPassed());
                if (ty->GetPrimType() == PTY_agg &&
                    GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptyIdx)->GetSize() > k4ByteSize) {
                    /* struct param aligned on 8 byte boundary unless it is small enough */
                    align = GetPointerSize();
                }
                segArgsRegPassed.SetSize(static_cast<uint32>(RoundUp(segArgsRegPassed.GetSize(), align)));
                symLoc->SetOffset(segArgsRegPassed.GetSize());
                segArgsRegPassed.SetSize(segArgsRegPassed.GetSize() + size);
            }
        } else {
            SetSizeAlignForTypeIdx(ptyIdx, size, align);
            symLoc->SetMemSegment(GetSegArgsStkPassed());
            segArgsStkPassed.SetSize(static_cast<uint32>(RoundUp(segArgsStkPassed.GetSize(), align)));
            symLoc->SetOffset(segArgsStkPassed.GetSize());
            segArgsStkPassed.SetSize(segArgsStkPassed.GetSize() + size);
            segArgsStkPassed.SetSize(static_cast<uint32>(RoundUp(segArgsStkPassed.GetSize(), GetPointerSize())));
        }
    }
}

void X64MemLayout::LayoutLocalVariables()
{
    uint32 symTabSize = mirFunction->GetSymTab()->GetSymbolTableSize();
    for (uint32 i = 0; i < symTabSize; ++i) {
        MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(i);
        if (sym == nullptr || sym->GetStorageClass() != kScAuto || sym->IsDeleted()) {
            continue;
        }
        uint32 stIndex = sym->GetStIndex();
        TyIdx tyIdx = sym->GetTyIdx();
        X64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<X64SymbolAlloc>();
        SetSymAllocInfo(stIndex, *symLoc);
        CHECK_FATAL(!symLoc->IsRegister(), "expect not register");

        symLoc->SetMemSegment(segLocals);
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        uint32 align = ty->GetAlign();
        if (ty->GetPrimType() == PTY_agg && align < k8BitSize) {
            segLocals.SetSize(static_cast<uint32>(RoundUp(segLocals.GetSize(), k8BitSize)));
        } else {
            segLocals.SetSize(static_cast<uint32>(RoundUp(segLocals.GetSize(), align)));
        }
        symLoc->SetOffset(segLocals.GetSize());
        segLocals.SetSize(segLocals.GetSize() + GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->GetSize());
    }
}

void X64MemLayout::AssignSpillLocationsToPseudoRegisters()
{
    MIRPregTable *pregTab = cgFunc->GetFunction().GetPregTab();

    /* BUG: n_regs include index 0 which is not a valid preg index. */
    size_t nRegs = pregTab->Size();
    spillLocTable.resize(nRegs);
    for (size_t i = 1; i < nRegs; ++i) {
        PrimType pType = pregTab->PregFromPregIdx(i)->GetPrimType();
        X64SymbolAlloc *symLoc = memAllocator->GetMemPool()->New<X64SymbolAlloc>();
        symLoc->SetMemSegment(segLocals);
        segLocals.SetSize(RoundUp(segLocals.GetSize(), GetPrimTypeSize(pType)));
        symLoc->SetOffset(segLocals.GetSize());
        MIRType *mirTy = GlobalTables::GetTypeTable().GetTypeTable()[pType];
        segLocals.SetSize(segLocals.GetSize() + mirTy->GetSize());
        spillLocTable[i] = symLoc;
    }
}

void X64MemLayout::LayoutReturnRef(int32 &structCopySize, int32 &maxParmStackSize)
{
    segArgsToStkPass.SetSize(FindLargestActualArea(structCopySize));
    maxParmStackSize = static_cast<int32>(segArgsToStkPass.GetSize());
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
        AssignSpillLocationsToPseudoRegisters();
    }
    segLocals.SetSize(static_cast<uint32>(RoundUp(segLocals.GetSize(), GetPointerSize())));
}

void X64MemLayout::LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize)
{
    LayoutVarargParams();
    LayoutFormalParams();

    // Need to be aligned ?
    segArgsRegPassed.SetSize(RoundUp(segArgsRegPassed.GetSize(), GetPointerSize()));
    segArgsStkPassed.SetSize(RoundUp(segArgsStkPassed.GetSize(), GetPointerSize() + GetPointerSize()));

    /* allocate the local variables in the stack */
    LayoutLocalVariables();
    LayoutReturnRef(structCopySize, maxParmStackSize);

    // Need to adapt to the cc interface.
    structCopySize = 0;
    // Scenes with more than 6 parameters are not yet enabled.
    maxParmStackSize = 0;

    cgFunc->SetUseFP(cgFunc->UseFP() || static_cast<int32>(StackFrameSize()) > kMaxPimm32);
}

uint64 X64MemLayout::StackFrameSize() const
{
    uint64 total = Locals().GetSize() + segArgsRegPassed.GetSize() + segArgsToStkPass.GetSize() +
                   segGrSaveArea.GetSize() + segVrSaveArea.GetSize() + segSpillReg.GetSize() +
                   cgFunc->GetFunction().GetFrameReseverdSlot();  // frame reserved slot
    return RoundUp(total, stackPtrAlignment);
}

int32 X64MemLayout::GetGRSaveAreaBaseLoc()
{
    int32 total = static_cast<int32>(RoundUp(GetSizeOfGRSaveArea(), stackPtrAlignment));
    return total;
}

int32 X64MemLayout::GetVRSaveAreaBaseLoc()
{
    int32 total = static_cast<int32>(RoundUp(GetSizeOfGRSaveArea(), stackPtrAlignment) +
                                     RoundUp(GetSizeOfVRSaveArea(), stackPtrAlignment));
    return total;
}
} /* namespace maplebe */
