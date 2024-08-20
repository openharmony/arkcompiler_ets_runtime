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

#include "aarch64_args.h"
#include <fstream>
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

namespace maplebe {
using namespace maple;

void AArch64MoveRegArgs::Run()
{
    BB *formerCurBB = aarFunc->GetCurBB();
    MoveVRegisterArgs();
    MoveRegisterArgs();
    aarFunc->SetCurBB(*formerCurBB);
}

void AArch64MoveRegArgs::MoveRegisterArgs() const
{
    aarFunc->GetDummyBB()->ClearInsns();
    aarFunc->SetCurBB(*aarFunc->GetDummyBB());

    auto &mirFunc = aarFunc->GetFunction();
    CCImpl &parmlocator =
        *static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateLocator(CCImpl::GetCallConvKind(cgFunc->GetFunction()));
    CCLocInfo ploc;
    for (uint32 i = 0; i < mirFunc.GetFormalCount(); ++i) {
        MIRType *ty = mirFunc.GetNthParamType(i);
        parmlocator.LocateNextParm(*ty, ploc, i == 0, mirFunc.GetMIRFuncType());
        if (ploc.reg0 == kRinvalid) {
            continue;
        }
        auto *sym = mirFunc.GetFormal(i);
        if (sym->IsPreg()) {
            continue;
        }
        auto *symLoc = aarFunc->GetMemlayout()->GetSymAllocInfo(sym->GetStIndex());
        auto *baseOpnd = aarFunc->GetBaseReg(*symLoc);
        auto offset = aarFunc->GetBaseOffset(*symLoc);
        auto generateStrInsn = [this, baseOpnd, &offset, sym, symLoc](AArch64reg reg, PrimType primType) {
            RegOperand &regOpnd = aarFunc->GetOrCreatePhysicalRegisterOperand(reg, GetPrimTypeBitSize(primType),
                                                                              aarFunc->GetRegTyFromPrimTy(primType));
            OfstOperand &ofstOpnd =
                aarFunc->CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(offset)), k32BitSize);
            AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(aarFunc->GetMemlayout());
            if (memLayout->IsSegMentVaried(symLoc->GetMemSegment())) {
                ofstOpnd.SetVary(kUnAdjustVary);
            }
            auto *memOpnd = aarFunc->CreateMemOperand(GetPrimTypeBitSize(primType), *baseOpnd, ofstOpnd, false);

            MOperator mOp = aarFunc->PickStInsn(GetPrimTypeBitSize(primType), primType);
            Insn &insn = aarFunc->GetInsnBuilder()->BuildInsn(mOp, regOpnd, *memOpnd);
            if (aarFunc->GetCG()->GenerateVerboseCG()) {
                insn.SetComment(std::string("store param: ").append(sym->GetName()));
            }
            aarFunc->GetCurBB()->AppendInsn(insn);
            offset += static_cast<int32>(GetPrimTypeSize(primType));
        };
        generateStrInsn(static_cast<AArch64reg>(ploc.GetReg0()), ploc.GetPrimTypeOfReg0());
        if (ploc.GetReg1() != kRinvalid) {
            generateStrInsn(static_cast<AArch64reg>(ploc.GetReg1()), ploc.GetPrimTypeOfReg1());
        }
        if (ploc.GetReg2() != kRinvalid) {
            generateStrInsn(static_cast<AArch64reg>(ploc.GetReg2()), ploc.GetPrimTypeOfReg2());
        }
        if (ploc.GetReg3() != kRinvalid) {
            generateStrInsn(static_cast<AArch64reg>(ploc.GetReg3()), ploc.GetPrimTypeOfReg3());
        }
    }

    aarFunc->GetFirstBB()->InsertAtBeginning(*aarFunc->GetDummyBB());
}

void AArch64MoveRegArgs::MoveLocalRefVarToRefLocals(MIRSymbol &mirSym) const
{
    AArch64CGFunc *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
    PrimType stype = mirSym.GetType()->GetPrimType();
    uint32 byteSize = GetPrimTypeSize(stype);
    uint32 bitSize = byteSize * kBitsPerByte;
    MemOperand &memOpnd = aarFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize, true);
    RegOperand *regOpnd = nullptr;
    if (mirSym.IsPreg()) {
        PregIdx pregIdx = aarFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(
            static_cast<uint32>(mirSym.GetPreg()->GetPregNo()));
        regOpnd = &aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    } else {
        regOpnd = &aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->NewVReg(kRegTyInt, k8ByteSize));
    }
    Insn &insn =
        aarFunc->GetInsnBuilder()->BuildInsn(aarFunc->PickLdInsn(GetPrimTypeBitSize(stype), stype), *regOpnd, memOpnd);
    MemOperand &memOpnd1 = aarFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize, false);
    Insn &insn1 =
        aarFunc->GetInsnBuilder()->BuildInsn(aarFunc->PickStInsn(GetPrimTypeBitSize(stype), stype), *regOpnd, memOpnd1);
    aarFunc->GetCurBB()->InsertInsnBegin(insn1);
    aarFunc->GetCurBB()->InsertInsnBegin(insn);
}

void AArch64MoveRegArgs::LoadStackArgsToVReg(MIRSymbol &mirSym) const
{
    AArch64CGFunc *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
    PrimType stype = mirSym.GetType()->GetPrimType();
    uint32 byteSize = GetPrimTypeSize(stype);
    uint32 bitSize = byteSize * kBitsPerByte;
    MemOperand &memOpnd = aarFunc->GetOrCreateMemOpnd(mirSym, 0, bitSize);
    PregIdx pregIdx =
        aarFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(static_cast<uint32>(mirSym.GetPreg()->GetPregNo()));
    RegOperand &dstRegOpnd =
        aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    Insn &insn = aarFunc->GetInsnBuilder()->BuildInsn(aarFunc->PickLdInsn(GetPrimTypeBitSize(stype), stype), dstRegOpnd,
                                                      memOpnd);

    if (aarFunc->GetCG()->GenerateVerboseCG()) {
        std::string key = "param: %%";
        key += std::to_string(mirSym.GetPreg()->GetPregNo());
        DEBUG_ASSERT(mirSym.GetStorageClass() == kScFormal, "vreg parameters should be kScFormal type.");
        insn.SetComment(key);
    }

    aarFunc->GetCurBB()->InsertInsnBegin(insn);
}

void AArch64MoveRegArgs::MoveArgsToVReg(const CCLocInfo &ploc, MIRSymbol &mirSym) const
{
    // when args parameter type i128, reg1 will be used.
    // but, i128 is not supported in back-end
    CHECK_FATAL(ploc.reg2 == kRinvalid, "NIY");
    RegType regType = (ploc.reg0 < V0) ? kRegTyInt : kRegTyFloat;
    PrimType sType = mirSym.GetType()->GetPrimType();
    uint32 byteSize = GetPrimTypeSize(sType);
    uint32 srcBitSize = ((byteSize < k4ByteSize) ? k4ByteSize : byteSize) * kBitsPerByte;
    PregIdx pregIdx =
        aarFunc->GetFunction().GetPregTab()->GetPregIdxFromPregno(static_cast<uint32>(mirSym.GetPreg()->GetPregNo()));
    RegOperand &dstRegOpnd =
        aarFunc->GetOrCreateVirtualRegisterOperand(aarFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    dstRegOpnd.SetSize(srcBitSize);
    RegOperand &srcRegOpnd =
        aarFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(ploc.reg0), srcBitSize, regType);
    DEBUG_ASSERT(mirSym.GetStorageClass() == kScFormal, "should be args");
    MOperator mOp = aarFunc->PickMovBetweenRegs(sType, sType);
    Insn &insn = CreateMoveArgsToVRegInsn(mOp, dstRegOpnd, srcRegOpnd, sType);
    if (aarFunc->GetCG()->GenerateVerboseCG()) {
        std::string str = "param: %%";
        str += std::to_string(mirSym.GetPreg()->GetPregNo());
        insn.SetComment(str);
    }
    aarFunc->GetCurBB()->InsertInsnBegin(insn);
}

Insn &AArch64MoveRegArgs::CreateMoveArgsToVRegInsn(MOperator mOp, RegOperand &destOpnd, RegOperand &srcOpnd,
                                                   PrimType primType) const
{
    if (mOp == MOP_wmovrr || mOp == MOP_xmovrr) {
        /* callee ensure the validbit of parameters of function */
        switch (primType) {
            case PTY_u1: {
                ImmOperand &lsbOpnd = aarFunc->CreateImmOperand(maplebe::k0BitSize, srcOpnd.GetSize(), false);
                ImmOperand &widthOpnd = aarFunc->CreateImmOperand(maplebe::k1BitSize, srcOpnd.GetSize(), false);
                bool is64Bit = (srcOpnd.GetSize() == maplebe::k64BitSize);
                return aarFunc->GetInsnBuilder()->BuildInsn(is64Bit ? MOP_xubfxrri6i6 : MOP_wubfxrri5i5, destOpnd,
                                                            srcOpnd, lsbOpnd, widthOpnd);
            }
            case PTY_u8:
            case PTY_i8:
                return aarFunc->GetInsnBuilder()->BuildInsn(MOP_xuxtb32, destOpnd, srcOpnd);
            case PTY_u16:
            case PTY_i16:
                return aarFunc->GetInsnBuilder()->BuildInsn(MOP_xuxth32, destOpnd, srcOpnd);
            case PTY_u32:
            case PTY_i32: {
                destOpnd.SetValidBitsNum(maplebe::k64BitSize);
                return aarFunc->GetInsnBuilder()->BuildInsn(MOP_xuxtw64, destOpnd, srcOpnd);
            }
            default:
                return aarFunc->GetInsnBuilder()->BuildInsn(mOp, destOpnd, srcOpnd);
        }
    } else {
        return aarFunc->GetInsnBuilder()->BuildInsn(mOp, destOpnd, srcOpnd);
    }
}

void AArch64MoveRegArgs::MoveVRegisterArgs()
{
    aarFunc->GetDummyBB()->ClearInsns();
    aarFunc->SetCurBB(*aarFunc->GetDummyBB());
    CCImpl &parmlocator =
        *static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateLocator(CCImpl::GetCallConvKind(cgFunc->GetFunction()));
    CCLocInfo ploc;

    auto &mirFunc = aarFunc->GetFunction();
    for (size_t i = 0; i < mirFunc.GetFormalCount(); ++i) {
        MIRType *ty = mirFunc.GetNthParamType(i);
        parmlocator.LocateNextParm(*ty, ploc, (i == 0), mirFunc.GetMIRFuncType());
        MIRSymbol *sym = mirFunc.GetFormal(i);

        /* load locarefvar formals to store in the reflocals. */
        if (mirFunc.GetNthParamAttr(i).GetAttr(ATTR_localrefvar) && ploc.reg0 == kRinvalid) {
            MoveLocalRefVarToRefLocals(*sym);
        }

        if (!sym->IsPreg()) {
            continue;
        }

        if (ploc.reg0 == kRinvalid) {
            /* load stack parameters to the vreg. */
            LoadStackArgsToVReg(*sym);
        } else {
            MoveArgsToVReg(ploc, *sym);
        }
    }

    aarFunc->GetFirstBB()->InsertAtBeginning(*aarFunc->GetDummyBB());
}
} /* namespace maplebe */
