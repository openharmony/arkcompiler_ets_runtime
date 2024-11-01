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

#include "aarch64_cg.h"
#include "aarch64_cgfunc.h"
#include <vector>
#include <cstdint>
#include <sys/stat.h>
#include <atomic>
#include "cfi.h"
#include "mpl_logging.h"
#include "rt.h"
#include "opcode_info.h"
#include "mir_builder.h"
#include "mir_symbol_builder.h"
#include "metadata_layout.h"
#include "emit.h"
#include <algorithm>

namespace maplebe {
using namespace maple;
CondOperand AArch64CGFunc::ccOperands[kCcLast] = {
    CondOperand(CC_EQ), CondOperand(CC_NE), CondOperand(CC_CS), CondOperand(CC_HS), CondOperand(CC_CC),
    CondOperand(CC_LO), CondOperand(CC_MI), CondOperand(CC_PL), CondOperand(CC_VS), CondOperand(CC_VC),
    CondOperand(CC_HI), CondOperand(CC_LS), CondOperand(CC_GE), CondOperand(CC_LT), CondOperand(CC_GT),
    CondOperand(CC_LE), CondOperand(CC_AL),
};

namespace {
constexpr int32 kSignedDimension = 2;        /* signed and unsigned */
constexpr int32 kIntByteSizeDimension = 4;   /* 1 byte, 2 byte, 4 bytes, 8 bytes */
constexpr int32 kFloatByteSizeDimension = 3; /* 4 bytes, 8 bytes, 16 bytes(vector) */
constexpr int32 kShiftAmount12 = 12;         /* for instruction that can use shift, shift amount must be 0 or 12 */

MOperator ldIs[kSignedDimension][kIntByteSizeDimension] = {
    /* unsigned == 0 */
    {MOP_wldrb, MOP_wldrh, MOP_wldr, MOP_xldr},
    /* signed == 1 */
    {MOP_wldrsb, MOP_wldrsh, MOP_wldr, MOP_xldr}};

MOperator stIs[kSignedDimension][kIntByteSizeDimension] = {
    /* unsigned == 0 */
    {MOP_wstrb, MOP_wstrh, MOP_wstr, MOP_xstr},
    /* signed == 1 */
    {MOP_wstrb, MOP_wstrh, MOP_wstr, MOP_xstr}};

MOperator ldFs[kFloatByteSizeDimension] = {MOP_sldr, MOP_dldr, MOP_qldr};
MOperator stFs[kFloatByteSizeDimension] = {MOP_sstr, MOP_dstr, MOP_qstr};

/* extended to unsigned ints */
MOperator uextIs[kIntByteSizeDimension][kIntByteSizeDimension] = {
    /*  u8         u16          u32          u64      */
    {MOP_undef, MOP_xuxtb32, MOP_xuxtb32, MOP_xuxtb32}, /* u8/i8   */
    {MOP_undef, MOP_undef, MOP_xuxth32, MOP_xuxth32},   /* u16/i16 */
    {MOP_undef, MOP_undef, MOP_xuxtw64, MOP_xuxtw64},   /* u32/i32 */
    {MOP_undef, MOP_undef, MOP_undef, MOP_undef}        /* u64/u64 */
};

/* extended to signed ints */
MOperator extIs[kIntByteSizeDimension][kIntByteSizeDimension] = {
    /*  i8         i16          i32          i64      */
    {MOP_undef, MOP_xsxtb32, MOP_xsxtb32, MOP_xsxtb64}, /* u8/i8   */
    {MOP_undef, MOP_undef, MOP_xsxth32, MOP_xsxth64},   /* u16/i16 */
    {MOP_undef, MOP_undef, MOP_undef, MOP_xsxtw64},     /* u32/i32 */
    {MOP_undef, MOP_undef, MOP_undef, MOP_undef}        /* u64/u64 */
};

MOperator PickLdStInsn(bool isLoad, uint32 bitSize, PrimType primType)
{
    DEBUG_ASSERT(bitSize >= k8BitSize, "PTY_u1 should have been lowered?");
    DEBUG_ASSERT(__builtin_popcount(bitSize) == 1, "PTY_u1 should have been lowered?");

    /* __builtin_ffs(x) returns: 0 -> 0, 1 -> 1, 2 -> 2, 4 -> 3, 8 -> 4 */
    if ((IsPrimitiveInteger(primType))) {
        auto *table = isLoad ? ldIs : stIs;
        int32 signedUnsigned = IsUnsignedInteger(primType) ? 0 : 1;

        /* __builtin_ffs(x) returns: 8 -> 4, 16 -> 5, 32 -> 6, 64 -> 7 */
        uint32 size = static_cast<uint32>(__builtin_ffs(static_cast<int32>(bitSize))) - k4BitSize;
        DEBUG_ASSERT(size <= 3, "wrong bitSize");  // size must <= 3
        return table[signedUnsigned][size];
    } else {
        MOperator *table = isLoad ? ldFs : stFs;
        /* __builtin_ffs(x) returns: 32 -> 6, 64 -> 7, 128 -> 8 */
        uint32 size = static_cast<uint32>(__builtin_ffs(static_cast<int32>(bitSize))) - k6BitSize;
        DEBUG_ASSERT(size <= k2BitSize, "size must be 0 to 2");
        return table[size];
    }
}
}  // namespace

RegOperand &AArch64CGFunc::GetOrCreateResOperand(const BaseNode &parent, PrimType primType)
{
    RegOperand *resOpnd = nullptr;
    if (parent.GetOpCode() == OP_regassign) {
        auto &regAssignNode = static_cast<const RegassignNode &>(parent);
        PregIdx pregIdx = regAssignNode.GetRegIdx();
        if (IsSpecialPseudoRegister(pregIdx)) {
            /* if it is one of special registers */
            resOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, primType);
        } else {
            resOpnd = &GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
        }
    } else {
        resOpnd = &CreateRegisterOperandOfType(primType);
    }
    return *resOpnd;
}

MOperator AArch64CGFunc::PickLdInsn(uint32 bitSize, PrimType primType) const
{
    return PickLdStInsn(true, bitSize, primType);
}

MOperator AArch64CGFunc::PickStInsn(uint32 bitSize, PrimType primType) const
{
    return PickLdStInsn(false, bitSize, primType);
}

MOperator AArch64CGFunc::PickExtInsn(PrimType dtype, PrimType stype) const
{
    int32 sBitSize = static_cast<int32>(GetPrimTypeBitSize(stype));
    int32 dBitSize = static_cast<int32>(GetPrimTypeBitSize(dtype));
    /* __builtin_ffs(x) returns: 0 -> 0, 1 -> 1, 2 -> 2, 4 -> 3, 8 -> 4 */
    if (IsPrimitiveInteger(stype) && IsPrimitiveInteger(dtype)) {
        MOperator(*table)[kIntByteSizeDimension];
        table = IsUnsignedInteger(stype) ? uextIs : extIs;
        /* __builtin_ffs(x) returns: 8 -> 4, 16 -> 5, 32 -> 6, 64 -> 7 */
        uint32 row = static_cast<uint32>(__builtin_ffs(sBitSize)) - k4BitSize;
        DEBUG_ASSERT(row <= k3BitSize, "wrong bitSize");
        uint32 col = static_cast<uint32>(__builtin_ffs(dBitSize)) - k4BitSize;
        DEBUG_ASSERT(col <= k3BitSize, "wrong bitSize");
        return table[row][col];
    }
    CHECK_FATAL(0, "extend not primitive integer");
    return MOP_undef;
}

MOperator AArch64CGFunc::PickMovBetweenRegs(PrimType destType, PrimType srcType) const
{
    if (IsPrimitiveInteger(destType) && IsPrimitiveInteger(srcType)) {
        return GetPrimTypeSize(srcType) <= k4ByteSize ? MOP_wmovrr : MOP_xmovrr;
    }
    if (IsPrimitiveFloat(destType) && IsPrimitiveFloat(srcType)) {
        return GetPrimTypeSize(srcType) <= k4ByteSize ? MOP_xvmovs : MOP_xvmovd;
    }
    if (IsPrimitiveInteger(destType) && IsPrimitiveFloat(srcType)) {
        return GetPrimTypeSize(srcType) <= k4ByteSize ? MOP_xvmovrs : MOP_xvmovrd;
    }
    if (IsPrimitiveFloat(destType) && IsPrimitiveInteger(srcType)) {
        return GetPrimTypeSize(srcType) <= k4ByteSize ? MOP_xvmovsr : MOP_xvmovdr;
    }
    CHECK_FATAL(false, "unexpected operand primtype for mov");
    return MOP_undef;
}

void AArch64CGFunc::SelectCopyImm(Operand &dest, PrimType dType, ImmOperand &src, PrimType sType)
{
    if (IsPrimitiveInteger(dType) != IsPrimitiveInteger(sType)) {
        RegOperand &tempReg = CreateRegisterOperandOfType(sType);
        SelectCopyImm(tempReg, src, sType);
        SelectCopy(dest, dType, tempReg, sType);
    } else {
        SelectCopyImm(dest, src, sType);
    }
}

void AArch64CGFunc::SelectCopyImm(Operand &dest, ImmOperand &src, PrimType dtype)
{
    uint32 dsize = GetPrimTypeBitSize(dtype);
    // If the type size of the parent node is smaller than the type size of the child node,
    // the number of child node needs to be truncated.
    if (dsize < src.GetSize()) {
        uint64 value = static_cast<uint64>(src.GetValue());
        uint64 mask = (1UL << dsize) - 1;
        int64 newValue = static_cast<int64>(value & mask);
        src.SetValue(newValue);
    }
    DEBUG_ASSERT(IsPrimitiveInteger(dtype), "The type of destination operand must be Integer");
    DEBUG_ASSERT(((dsize == k8BitSize) || (dsize == k16BitSize) || (dsize == k32BitSize) || (dsize == k64BitSize)),
                 "The destination operand must be >= 8-bit");
    if (src.GetSize() == k32BitSize && dsize == k64BitSize && src.IsSingleInstructionMovable()) {
        auto tempReg = CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k32BitSize), k32BitSize, kRegTyInt);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wmovri32, *tempReg, src));
        SelectCopy(dest, dtype, *tempReg, PTY_u32);
        return;
    }
    if (src.IsSingleInstructionMovable()) {
        MOperator mOp = (dsize <= k32BitSize) ? MOP_wmovri32 : MOP_xmovri64;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, dest, src));
        return;
    }
    uint64 srcVal = static_cast<uint64>(src.GetValue());
    /* using mov/movk to load the immediate value */
    if (dsize == k8BitSize) {
        /* compute lower 8 bits value */
        if (dtype == PTY_u8) {
            /* zero extend */
            srcVal = (srcVal << k56BitSize) >> k56BitSize;
            dtype = PTY_u16;
        } else {
            /* sign extend */
            srcVal = (static_cast<int64>(srcVal) << k56BitSize) >> k56BitSize;
            dtype = PTY_i16;
        }
        dsize = k16BitSize;
    }
    if (dsize == k16BitSize) {
        if (dtype == PTY_u16) {
            /* check lower 16 bits and higher 16 bits respectively */
            DEBUG_ASSERT((srcVal & 0x0000FFFFULL) != 0, "unexpected value");
            DEBUG_ASSERT(((srcVal >> k16BitSize) & 0x0000FFFFULL) == 0, "unexpected value");
            DEBUG_ASSERT((srcVal & 0x0000FFFFULL) != 0xFFFFULL, "unexpected value");
            /* create an imm opereand which represents lower 16 bits of the immediate */
            ImmOperand &srcLower = CreateImmOperand(static_cast<int64>(srcVal & 0x0000FFFFULL), k16BitSize, false);
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wmovri32, dest, srcLower));
            return;
        } else {
            /* sign extend and let `dsize == 32` case take care of it */
            srcVal = (static_cast<int64>(srcVal) << k48BitSize) >> k48BitSize;
            dsize = k32BitSize;
        }
    }
    if (dsize == k32BitSize) {
        /* check lower 16 bits and higher 16 bits respectively */
        DEBUG_ASSERT((srcVal & 0x0000FFFFULL) != 0, "unexpected val");
        DEBUG_ASSERT(((srcVal >> k16BitSize) & 0x0000FFFFULL) != 0, "unexpected val");
        DEBUG_ASSERT((srcVal & 0x0000FFFFULL) != 0xFFFFULL, "unexpected val");
        DEBUG_ASSERT(((srcVal >> k16BitSize) & 0x0000FFFFULL) != 0xFFFFULL, "unexpected val");
        /* create an imm opereand which represents lower 16 bits of the immediate */
        ImmOperand &srcLower = CreateImmOperand(static_cast<int64>(srcVal & 0x0000FFFFULL), k16BitSize, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wmovri32, dest, srcLower));
        /* create an imm opereand which represents upper 16 bits of the immediate */
        ImmOperand &srcUpper =
            CreateImmOperand(static_cast<int64>((srcVal >> k16BitSize) & 0x0000FFFFULL), k16BitSize, false);
        BitShiftOperand *lslOpnd = GetLogicalShiftLeftOperand(k16BitSize, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wmovkri16, dest, srcUpper, *lslOpnd));
    } else {
        /*
         * partition it into 4 16-bit chunks
         * if more 0's than 0xFFFF's, use movz as the initial instruction.
         * otherwise, movn.
         */
        bool useMovz = BetterUseMOVZ(srcVal);
        bool useMovk = false;
        /* get lower 32 bits of the immediate */
        uint64 chunkLval = srcVal & 0xFFFFFFFFULL;
        /* get upper 32 bits of the immediate */
        uint64 chunkHval = (srcVal >> k32BitSize) & 0xFFFFFFFFULL;
        int32 maxLoopTime = 4;

        if (chunkLval == chunkHval) {
            /* compute lower 32 bits, and then copy to higher 32 bits, so only 2 chunks need be processed */
            maxLoopTime = 2;
        }

        uint64 sa = 0;

        for (int64 i = 0; i < maxLoopTime; ++i, sa += k16BitSize) {
            /* create an imm opereand which represents the i-th 16-bit chunk of the immediate */
            uint64 chunkVal = (srcVal >> (static_cast<uint64>(sa))) & 0x0000FFFFULL;
            if (useMovz ? (chunkVal == 0) : (chunkVal == 0x0000FFFFULL)) {
                continue;
            }
            ImmOperand &src16 = CreateImmOperand(static_cast<int64>(chunkVal), k16BitSize, false);
            BitShiftOperand *lslOpnd = GetLogicalShiftLeftOperand(sa, true);
            if (!useMovk) {
                /* use movz or movn */
                if (!useMovz) {
                    src16.BitwiseNegate();
                }
                GetCurBB()->AppendInsn(
                    GetInsnBuilder()->BuildInsn(useMovz ? MOP_xmovzri16 : MOP_xmovnri16, dest, src16, *lslOpnd));
                useMovk = true;
            } else {
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xmovkri16, dest, src16, *lslOpnd));
            }
        }

        if (maxLoopTime == 2) { /* as described above, only 2 chunks need be processed */
            /* copy lower 32 bits to higher 32 bits */
            ImmOperand &immOpnd = CreateImmOperand(k32BitSize, k8BitSize, false);
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xbfirri6i6, dest, dest, immOpnd, immOpnd));
        }
    }
}

void AArch64CGFunc::SelectCopyMemOpnd(Operand &dest, PrimType dtype, uint32 dsize, Operand &src, PrimType stype)
{
    Insn *insn = nullptr;
    uint32 ssize = src.GetSize();
    PrimType regTy = PTY_void;
    RegOperand *loadReg = nullptr;
    MOperator mop = MOP_undef;
    if (IsPrimitiveFloat(stype)) {
        CHECK_FATAL(dsize == ssize, "dsize %u expect equals ssize %u", dtype, ssize);
        insn = &GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, stype), dest, src);
    } else {
        mop = PickExtInsn(dtype, stype);
        if (ssize == (GetPrimTypeSize(dtype) * kBitsPerByte) || mop == MOP_undef) {
            insn = &GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, stype), dest, src);
        } else {
            regTy = dsize == k64BitSize ? dtype : PTY_i32;
            loadReg = &CreateRegisterOperandOfType(regTy);
            insn = &GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, stype), *loadReg, src);
        }
    }

    GetCurBB()->AppendInsn(*insn);
    if (regTy != PTY_void && mop != MOP_undef) {
        DEBUG_ASSERT(loadReg != nullptr, "loadReg should not be nullptr");
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, dest, *loadReg));
    }
}

bool AArch64CGFunc::IsImmediateValueInRange(MOperator mOp, int64 immVal, bool is64Bits, bool isIntactIndexed,
                                            bool isPostIndexed, bool isPreIndexed) const
{
    bool isInRange = false;
    switch (mOp) {
        case MOP_xstr:
        case MOP_wstr:
            isInRange =
                (isIntactIndexed &&
                 ((!is64Bits && (immVal >= kStrAllLdrAllImmLowerBound) && (immVal <= kStrLdrImm32UpperBound)) ||
                  (is64Bits && (immVal >= kStrAllLdrAllImmLowerBound) && (immVal <= kStrLdrImm64UpperBound)))) ||
                ((isPostIndexed || isPreIndexed) && (immVal >= kStrLdrPerPostLowerBound) &&
                 (immVal <= kStrLdrPerPostUpperBound));
            break;
        case MOP_wstrb:
            isInRange =
                (isIntactIndexed && (immVal >= kStrAllLdrAllImmLowerBound) && (immVal <= kStrbLdrbImmUpperBound)) ||
                ((isPostIndexed || isPreIndexed) && (immVal >= kStrLdrPerPostLowerBound) &&
                 (immVal <= kStrLdrPerPostUpperBound));
            break;
        case MOP_wstrh:
            isInRange =
                (isIntactIndexed && (immVal >= kStrAllLdrAllImmLowerBound) && (immVal <= kStrhLdrhImmUpperBound)) ||
                ((isPostIndexed || isPreIndexed) && (immVal >= kStrLdrPerPostLowerBound) &&
                 (immVal <= kStrLdrPerPostUpperBound));
            break;
        default:
            break;
    }
    return isInRange;
}

bool AArch64CGFunc::IsStoreMop(MOperator mOp) const
{
    switch (mOp) {
        case MOP_sstr:
        case MOP_dstr:
        case MOP_qstr:
        case MOP_xstr:
        case MOP_wstr:
        case MOP_wstrb:
        case MOP_wstrh:
            return true;
        default:
            return false;
    }
}

void AArch64CGFunc::SelectCopyRegOpnd(Operand &dest, PrimType dtype, Operand::OperandType opndType, uint32 dsize,
                                      Operand &src, PrimType stype)
{
    if (opndType != Operand::kOpdMem) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickMovBetweenRegs(dtype, stype), dest, src));
        return;
    }
    bool is64Bits = (dest.GetSize() == k64BitSize) ? true : false;
    MOperator strMop = PickStInsn(dsize, stype);
    if (!dest.IsMemoryAccessOperand()) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(strMop, src, dest));
        return;
    }

    MemOperand *memOpnd = static_cast<MemOperand *>(&dest);
    DEBUG_ASSERT(memOpnd != nullptr, "memOpnd should not be nullptr");
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeLo12Li) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(strMop, src, dest));
        return;
    }
    if (memOpnd->GetOffsetOperand() == nullptr) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(strMop, src, dest));
        return;
    }
    ImmOperand *immOpnd = static_cast<ImmOperand *>(memOpnd->GetOffsetOperand());
    DEBUG_ASSERT(immOpnd != nullptr, "immOpnd should not be nullptr");
    int64 immVal = immOpnd->GetValue();
    bool isIntactIndexed = memOpnd->IsIntactIndexed();
    bool isPostIndexed = memOpnd->IsPostIndexed();
    bool isPreIndexed = memOpnd->IsPreIndexed();
    DEBUG_ASSERT(!isPostIndexed, "memOpnd should not be post-index type");
    DEBUG_ASSERT(!isPreIndexed, "memOpnd should not be pre-index type");
    bool isInRange = false;
    isInRange = IsImmediateValueInRange(strMop, immVal, is64Bits, isIntactIndexed, isPostIndexed, isPreIndexed);
    bool isMopStr = IsStoreMop(strMop);
    if (isInRange || !isMopStr) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(strMop, src, dest));
        return;
    }
    DEBUG_ASSERT(memOpnd->GetBaseRegister() != nullptr, "nullptr check");
    if (isIntactIndexed) {
        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, dsize);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(strMop, src, *memOpnd));
    } else if (isPostIndexed || isPreIndexed) {
        RegOperand &reg = CreateRegisterOperandOfType(PTY_i64);
        MOperator mopMov = MOP_xmovri64;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopMov, reg, *immOpnd));
        MOperator mopAdd = MOP_xaddrrr;
        MemOperand &newDest =
            GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, GetPrimTypeBitSize(dtype), memOpnd->GetBaseRegister(), nullptr,
                               &GetOrCreateOfstOpnd(0, k32BitSize), nullptr);
        Insn &insn1 = GetInsnBuilder()->BuildInsn(strMop, src, newDest);
        Insn &insn2 = GetInsnBuilder()->BuildInsn(mopAdd, *newDest.GetBaseRegister(), *newDest.GetBaseRegister(), reg);
        if (isPostIndexed) {
            GetCurBB()->AppendInsn(insn1);
            GetCurBB()->AppendInsn(insn2);
        } else {
            /* isPreIndexed */
            GetCurBB()->AppendInsn(insn2);
            GetCurBB()->AppendInsn(insn1);
        }
    }
}

void AArch64CGFunc::SelectCopy(Operand &dest, PrimType dtype, Operand &src, PrimType stype, BaseNode *baseNode)
{
    DEBUG_ASSERT(dest.IsRegister() || dest.IsMemoryAccessOperand(), "");
    uint32 dsize = GetPrimTypeBitSize(dtype);
    if (dest.IsRegister()) {
        dsize = dest.GetSize();
    }
    Operand::OperandType opnd0Type = dest.GetKind();
    Operand::OperandType opnd1Type = src.GetKind();
    DEBUG_ASSERT(((dsize >= src.GetSize()) || (opnd0Type == Operand::kOpdRegister) || (opnd0Type == Operand::kOpdMem)),
                 "NYI");
    DEBUG_ASSERT(((opnd0Type == Operand::kOpdRegister) || (src.GetKind() == Operand::kOpdRegister)),
                 "either src or dest should be register");

    switch (opnd1Type) {
        case Operand::kOpdMem:
            SelectCopyMemOpnd(dest, dtype, dsize, src, stype);
            break;
        case Operand::kOpdOffset:
        case Operand::kOpdImmediate:
            SelectCopyImm(dest, dtype, static_cast<ImmOperand &>(src), stype);
            break;
        case Operand::kOpdFPImmediate:
            CHECK_FATAL(static_cast<ImmOperand &>(src).GetValue() == 0, "NIY");
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn((dsize == k32BitSize) ? MOP_xvmovsr : MOP_xvmovdr, dest,
                                                               GetZeroOpnd(dsize)));
            break;
        case Operand::kOpdRegister: {
            if (dest.IsRegister()) {
                RegOperand &desReg = static_cast<RegOperand &>(dest);
                RegOperand &srcReg = static_cast<RegOperand &>(src);
                if (desReg.GetRegisterNumber() == srcReg.GetRegisterNumber()) {
                    break;
                }
            }
            SelectCopyRegOpnd(dest, dtype, opnd0Type, dsize, src, stype);
            break;
        }
        default:
            CHECK_FATAL(false, "NYI");
    }
}

/* This function copies src to a register, the src can be an imm, mem or a label */
RegOperand &AArch64CGFunc::SelectCopy(Operand &src, PrimType stype, PrimType dtype)
{
    RegOperand &dest = CreateRegisterOperandOfType(dtype);
    SelectCopy(dest, dtype, src, stype);
    return dest;
}

/*
 * We need to adjust the offset of a stack allocated local variable
 * if we store FP/SP before any other local variables to save an instruction.
 * See AArch64CGFunc::OffsetAdjustmentForFPLR() in aarch64_cgfunc.cpp
 *
 * That is when we !UsedStpSubPairForCallFrameAllocation().
 *
 * Because we need to use the STP/SUB instruction pair to store FP/SP 'after'
 * local variables when the call frame size is greater that the max offset
 * value allowed for the STP instruction (we cannot use STP w/ prefix, LDP w/
 * postfix), if UsedStpSubPairForCallFrameAllocation(), we don't need to
 * adjust the offsets.
 */
bool AArch64CGFunc::IsImmediateOffsetOutOfRange(const MemOperand &memOpnd, uint32 bitLen)
{
    DEBUG_ASSERT(bitLen >= k8BitSize, "bitlen error");
    DEBUG_ASSERT(bitLen <= k128BitSize, "bitlen error");

    if (bitLen >= k8BitSize) {
        bitLen = static_cast<uint32>(RoundUp(bitLen, k8BitSize));
    }
    DEBUG_ASSERT((bitLen & (bitLen - 1)) == 0, "bitlen error");

    MemOperand::AArch64AddressingMode mode = memOpnd.GetAddrMode();
    if ((mode == MemOperand::kAddrModeBOi) && memOpnd.IsIntactIndexed()) {
        OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
        int32 offsetValue = ofstOpnd ? static_cast<int32>(ofstOpnd->GetOffsetValue()) : 0;
        if (ofstOpnd && ofstOpnd->GetVary() == kUnAdjustVary) {
            offsetValue +=
                static_cast<int32>(static_cast<AArch64MemLayout *>(GetMemlayout())->RealStackFrameSize() + 0xff);
        }
        offsetValue += kAarch64IntregBytelen << 1; /* Refer to the above comment */
        return MemOperand::IsPIMMOffsetOutOfRange(offsetValue, bitLen);
    } else {
        return false;
    }
}

// This api is used to judge whether opnd is legal for mop.
// It is implemented by calling verify api of mop (InsnDesc -> Verify).
bool AArch64CGFunc::IsOperandImmValid(MOperator mOp, Operand *o, uint32 opndIdx) const
{
    const InsnDesc *md = &AArch64CG::kMd[mOp];
    auto *opndProp = md->opndMD[opndIdx];
    MemPool *localMp = memPoolCtrler.NewMemPool("opnd verify mempool", true);
    auto *localAlloc = new MapleAllocator(localMp);
    MapleVector<Operand *> testOpnds(md->opndMD.size(), localAlloc->Adapter());
    testOpnds[opndIdx] = o;
    bool flag = true;
    Operand::OperandType opndTy = opndProp->GetOperandType();
    if (opndTy == Operand::kOpdMem) {
        auto *memOpnd = static_cast<MemOperand *>(o);
        CHECK_FATAL(memOpnd != nullptr, "memOpnd should not be nullptr");
        if (memOpnd->GetAddrMode() == MemOperand::kAddrModeBOrX &&
            (!memOpnd->IsPostIndexed() && !memOpnd->IsPreIndexed())) {
            delete localAlloc;
            memPoolCtrler.DeleteMemPool(localMp);
            return true;
        }
        OfstOperand *ofStOpnd = memOpnd->GetOffsetImmediate();
        int64 offsetValue = ofStOpnd ? ofStOpnd->GetOffsetValue() : 0LL;
        if (md->IsLoadStorePair() || (memOpnd->GetAddrMode() == MemOperand::kAddrModeBOi)) {
            flag = md->Verify(testOpnds);
        } else if (memOpnd->GetAddrMode() == MemOperand::kAddrModeLo12Li) {
            if (offsetValue == 0) {
                flag = md->Verify(testOpnds);
            } else {
                flag = false;
            }
        } else if (memOpnd->IsPostIndexed() || memOpnd->IsPreIndexed()) {
            flag = (offsetValue <= static_cast<int64>(k256BitSizeInt) && offsetValue >= kNegative256BitSize);
        }
    } else if (opndTy == Operand::kOpdImmediate) {
        flag = md->Verify(testOpnds);
    }
    delete localAlloc;
    memPoolCtrler.DeleteMemPool(localMp);
    return flag;
}

MemOperand &AArch64CGFunc::CreateReplacementMemOperand(uint32 bitLen, RegOperand &baseReg, int64 offset)
{
    return CreateMemOpnd(baseReg, offset, bitLen);
}

bool AArch64CGFunc::CheckIfSplitOffsetWithAdd(const MemOperand &memOpnd, uint32 bitLen) const
{
    if (memOpnd.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd.IsIntactIndexed()) {
        return false;
    }
    OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
    int32 opndVal = static_cast<int32>(ofstOpnd->GetOffsetValue());
    int32 maxPimm = memOpnd.GetMaxPIMM(bitLen);
    int32 q0 = opndVal / maxPimm;
    int32 addend = q0 * maxPimm;
    int32 r0 = opndVal - addend;
    int32 alignment = static_cast<int32_t>(memOpnd.GetImmediateOffsetAlignment(bitLen));
    int32 r1 = static_cast<uint32>(r0) & ((1u << static_cast<uint32>(alignment)) - 1);
    addend = addend + r1;
    return (addend > 0);
}

RegOperand *AArch64CGFunc::GetBaseRegForSplit(uint32 baseRegNum)
{
    RegOperand *resOpnd = nullptr;
    if (baseRegNum == AArch64reg::kRinvalid) {
        resOpnd = &CreateRegisterOperandOfType(PTY_i64);
    } else if (AArch64isa::IsPhysicalRegister(baseRegNum)) {
        resOpnd = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(baseRegNum),
                                                      GetPointerSize() * kBitsPerByte, kRegTyInt);
    } else {
        resOpnd = &GetOrCreateVirtualRegisterOperand(baseRegNum);
    }
    return resOpnd;
}

ImmOperand &AArch64CGFunc::SplitAndGetRemained(const MemOperand &memOpnd, uint32 bitLen, RegOperand *resOpnd,
                                               int64 ofstVal, bool isDest, Insn *insn, bool forPair)
{
    auto it = hashMemOpndTable.find(memOpnd);
    if (it != hashMemOpndTable.end()) {
        hashMemOpndTable.erase(memOpnd);
    }
    /*
     * opndVal == Q0 * 32760(16380) + R0
     * R0 == Q1 * 8(4) + R1
     * ADDEND == Q0 * 32760(16380) + R1
     * NEW_OFFSET = Q1 * 8(4)
     * we want to generate two instructions:
     * ADD TEMP_REG, X29, ADDEND
     * LDR/STR TEMP_REG, [ TEMP_REG, #NEW_OFFSET ]
     */
    int32 maxPimm = 0;
    if (!forPair) {
        maxPimm = MemOperand::GetMaxPIMM(bitLen);
    } else {
        maxPimm = MemOperand::GetMaxPairPIMM(bitLen);
    }
    DEBUG_ASSERT(maxPimm != 0, "get max pimm failed");

    int64 q0 = ofstVal / maxPimm + (ofstVal < 0 ? -1 : 0);
    int64 addend = q0 * maxPimm;
    int64 r0 = ofstVal - addend;
    int64 alignment = MemOperand::GetImmediateOffsetAlignment(bitLen);
    auto q1 = static_cast<int64>(static_cast<uint64>(r0) >> static_cast<uint64>(alignment));
    auto r1 = static_cast<int64>(static_cast<uint64>(r0) & ((1u << static_cast<uint64>(alignment)) - 1));
    auto remained = static_cast<int64>(static_cast<uint64>(q1) << static_cast<uint64>(alignment));
    addend = addend + r1;
    if (addend > 0) {
        int64 suffixClear = 0xfff;
        if (forPair) {
            suffixClear = 0xff;
        }
        int64 remainedTmp = remained + (addend & suffixClear);
        if (!MemOperand::IsPIMMOffsetOutOfRange(static_cast<int32>(remainedTmp), bitLen) &&
            ((static_cast<uint64>(remainedTmp) & ((1u << static_cast<uint64>(alignment)) - 1)) == 0)) {
            remained = remainedTmp;
            addend = (addend & ~suffixClear);
        }
    }
    ImmOperand &immAddend = CreateImmOperand(addend, k64BitSize, true);
    if (memOpnd.GetOffsetImmediate()->GetVary() == kUnAdjustVary) {
        immAddend.SetVary(kUnAdjustVary);
    }
    return immAddend;
}

MemOperand &AArch64CGFunc::SplitOffsetWithAddInstruction(const MemOperand &memOpnd, uint32 bitLen, uint32 baseRegNum,
                                                         bool isDest, Insn *insn, bool forPair)
{
    DEBUG_ASSERT((memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi), "expect kAddrModeBOi memOpnd");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "expect intactIndexed memOpnd");
    OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
    int64 ofstVal = ofstOpnd->GetOffsetValue();
    RegOperand *resOpnd = GetBaseRegForSplit(baseRegNum);
    ImmOperand &immAddend = SplitAndGetRemained(memOpnd, bitLen, resOpnd, ofstVal, isDest, insn, forPair);
    int64 remained = (ofstVal - immAddend.GetValue());
    RegOperand *origBaseReg = memOpnd.GetBaseRegister();
    DEBUG_ASSERT(origBaseReg != nullptr, "nullptr check");
    if (insn == nullptr) {
        SelectAdd(*resOpnd, *origBaseReg, immAddend, PTY_i64);
    } else {
        SelectAddAfterInsn(*resOpnd, *origBaseReg, immAddend, PTY_i64, isDest, *insn);
    }
    MemOperand &newMemOpnd = CreateReplacementMemOperand(bitLen, *resOpnd, remained);
    newMemOpnd.SetStackMem(memOpnd.IsStackMem());
    return newMemOpnd;
}

void AArch64CGFunc::SelectDassign(DassignNode &stmt, Operand &opnd0)
{
    SelectDassign(stmt.GetStIdx(), stmt.GetFieldID(), stmt.GetRHS()->GetPrimType(), opnd0);
}

/*
 * NOTE: I divided SelectDassign so that we can create "virtual" assignments
 * when selecting other complex Maple IR instructions. For example, the atomic
 * exchange and other intrinsics will need to assign its results to local
 * variables. Such Maple IR instructions are pltform-specific (e.g.
 * atomic_exchange can be implemented as one single machine intruction on x86_64
 * and ARMv8.1, but ARMv8.0 needs an LL/SC loop), therefore they cannot (in
 * principle) be lowered at BELowerer or CGLowerer.
 */
void AArch64CGFunc::SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opnd0)
{
    MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(stIdx);
    int32 offset = 0;
    bool parmCopy = false;
    uint32 regSize = GetPrimTypeBitSize(rhsPType);
    MIRType *type = symbol->GetType();
    Operand &stOpnd = LoadIntoRegister(opnd0, IsPrimitiveInteger(rhsPType),
                                       regSize, IsSignedInteger(type->GetPrimType()));
    MOperator mOp = MOP_undef;

    uint32 dataSize = GetPrimTypeBitSize(type->GetPrimType());
    MemOperand *memOpnd = nullptr;
    if (parmCopy) {
        memOpnd = &LoadStructCopyBase(*symbol, offset, static_cast<int>(dataSize));
    } else {
        memOpnd = &GetOrCreateMemOpnd(*symbol, offset, dataSize);
    }
    if ((memOpnd->GetMemVaryType() == kNotVary) && IsImmediateOffsetOutOfRange(*memOpnd, dataSize)) {
        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, dataSize);
    }

    /* In bpl mode, a func symbol's type is represented as a MIRFuncType instead of a MIRPtrType (pointing to
     * MIRFuncType), so we allow `kTypeFunction` to appear here */
    DEBUG_ASSERT(((type->GetKind() == kTypeScalar) || (type->GetKind() == kTypePointer) ||
        (type->GetKind() == kTypeFunction) || (type->GetKind() == kTypeArray)), "NYI dassign type");
    PrimType ptyp = type->GetPrimType();

    mOp = PickStInsn(GetPrimTypeBitSize(ptyp), ptyp);
    Insn &insn = GetInsnBuilder()->BuildInsn(mOp, stOpnd, *memOpnd);
    GetCurBB()->AppendInsn(insn);
}

void AArch64CGFunc::SelectRegassign(RegassignNode &stmt, Operand &opnd0)
{
    RegOperand *regOpnd = nullptr;
    PregIdx pregIdx = stmt.GetRegIdx();
    if (IsSpecialPseudoRegister(pregIdx)) {
        regOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, stmt.GetPrimType());
    } else {
        regOpnd = &GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    }
    /* look at rhs */
    PrimType rhsType = stmt.Opnd(0)->GetPrimType();
    DEBUG_ASSERT(regOpnd != nullptr, "null ptr check!");
    Operand *srcOpnd = &opnd0;
    if (GetPrimTypeSize(stmt.GetPrimType()) > GetPrimTypeSize(rhsType) && IsPrimitiveInteger(rhsType)) {
        CHECK_FATAL(IsPrimitiveInteger(stmt.GetPrimType()), "NIY");
        srcOpnd = &CreateRegisterOperandOfType(stmt.GetPrimType());
        SelectCvtInt2Int(nullptr, srcOpnd, &opnd0, rhsType, stmt.GetPrimType());
    }
    SelectCopy(*regOpnd, stmt.GetPrimType(), *srcOpnd, rhsType, stmt.GetRHS());

    if (GetCG()->GenerateVerboseCG()) {
        if (GetCurBB()->GetLastInsn()) {
            GetCurBB()->GetLastInsn()->AppendComment(" regassign %" + std::to_string(pregIdx) + "; ");
        } else if (GetCurBB()->GetPrev()->GetLastInsn()) {
            GetCurBB()->GetPrev()->GetLastInsn()->AppendComment(" regassign %" + std::to_string(pregIdx) + "; ");
        }
    }

    if ((Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) && (pregIdx >= 0)) {
        MemOperand *dest = GetPseudoRegisterSpillMemoryOperand(pregIdx);
        PrimType stype = GetTypeFromPseudoRegIdx(pregIdx);
        MIRPreg *preg = GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
        uint32 srcBitLength = GetPrimTypeSize(preg->GetPrimType()) * kBitsPerByte;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickStInsn(srcBitLength, stype), *regOpnd, *dest));
    } else if (regOpnd->GetRegisterNumber() == R0 || regOpnd->GetRegisterNumber() == R1) {
        Insn &pseudo = GetInsnBuilder()->BuildInsn(MOP_pseudo_ret_int, *regOpnd);
        GetCurBB()->AppendInsn(pseudo);
    } else if (regOpnd->GetRegisterNumber() >= V0 && regOpnd->GetRegisterNumber() <= V3) {
        Insn &pseudo = GetInsnBuilder()->BuildInsn(MOP_pseudo_ret_float, *regOpnd);
        GetCurBB()->AppendInsn(pseudo);
    }
    if (stmt.GetPrimType() == PTY_ref) {
        regOpnd->SetIsReference(true);
        AddReferenceReg(regOpnd->GetRegisterNumber());
    }
    if (pregIdx > 0) {
        // special MIRPreg is not supported
        SetPregIdx2Opnd(pregIdx, *regOpnd);
    }
    const auto &derived2BaseRef = GetFunction().GetDerived2BaseRef();
    auto itr = derived2BaseRef.find(pregIdx);
    if (itr != derived2BaseRef.end()) {
        auto *opnd = GetOpndFromPregIdx(itr->first);
        CHECK_FATAL(opnd != nullptr, "pregIdx has not been assigned Operand");
        auto &derivedRegOpnd = static_cast<RegOperand &>(*opnd);
        opnd = GetOpndFromPregIdx(itr->second);
        CHECK_FATAL(opnd != nullptr, "pregIdx has not been assigned Operand");
        auto &baseRegOpnd = static_cast<RegOperand &>(*opnd);
        derivedRegOpnd.SetBaseRefOpnd(baseRegOpnd);
    }
}

CCImpl *AArch64CGFunc::GetOrCreateLocator(CallConvKind cc)
{
    auto it = hashCCTable.find(cc);
    if (it != hashCCTable.end()) {
        it->second->Init();
        return it->second;
    }
    CCImpl *res = nullptr;
    if (cc == kCCall) {
        res = memPool->New<AArch64CallConvImpl>(GetBecommon());
    } else if (cc == kWebKitJS) {
        res = memPool->New<AArch64WebKitJSCC>(GetBecommon());
    } else {
        CHECK_FATAL(false, "unsupported yet");
    }
    hashCCTable[cc] = res;
    return res;
}

static MIRType *GetPointedToType(const MIRPtrType &pointerType)
{
    MIRType *aType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType.GetPointedTyIdx());
    if (aType->GetKind() == kTypeArray) {
        MIRArrayType *arrayType = static_cast<MIRArrayType *>(aType);
        return GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
    }
    return aType;
}

void AArch64CGFunc::SelectIassign(IassignNode &stmt)
{
    int32 offset = 0;
    MIRPtrType *pointerType = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(stmt.GetTyIdx()));
    DEBUG_ASSERT(pointerType != nullptr, "expect a pointer type at iassign node");
    MIRType *pointedType = nullptr;
    bool isRefField = false;
    pointedType = GetPointedToType(*pointerType);

    PrimType styp = stmt.GetRHS()->GetPrimType();
    Operand *valOpnd = HandleExpr(stmt, *stmt.GetRHS());
    Operand &srcOpnd = LoadIntoRegister(*valOpnd, (IsPrimitiveInteger(styp)),
                                        GetPrimTypeBitSize(styp));

    PrimType destType = pointedType->GetPrimType();
    DEBUG_ASSERT(stmt.Opnd(0) != nullptr, "null ptr check");
    MemOperand &memOpnd = CreateMemOpnd(destType, stmt, *stmt.Opnd(0), offset);
    SelectCopy(memOpnd, destType, srcOpnd, destType);
    if (GetCurBB() && GetCurBB()->GetLastMachineInsn()) {
        GetCurBB()->GetLastMachineInsn()->MarkAsAccessRefField(isRefField);
    }
}

Operand *AArch64CGFunc::SelectDread(const BaseNode &parent, DreadNode &expr)
{
    MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());

    PrimType symType = symbol->GetType()->GetPrimType();
    uint32 offset = 0;
    bool parmCopy = false;

    uint32 dataSize = GetPrimTypeBitSize(symType);
    uint32 aggSize = 0;
    PrimType resultType = expr.GetPrimType();
    MemOperand *memOpnd = nullptr;
    if (aggSize > k8ByteSize) {
        if (parent.op == OP_eval) {
            if (symbol->GetAttr(ATTR_volatile)) {
                /* Need to generate loads for the upper parts of the struct. */
                Operand &dest = GetZeroOpnd(k64BitSize);
                uint32 numLoads = static_cast<uint32>(RoundUp(aggSize, k64BitSize) / k64BitSize);
                for (uint32 o = 0; o < numLoads; ++o) {
                    if (parmCopy) {
                        memOpnd = &LoadStructCopyBase(*symbol, offset + o * GetPointerSize(), GetPointerSize());
                    } else {
                        memOpnd = &GetOrCreateMemOpnd(*symbol, offset + o * GetPointerSize(), GetPointerSize());
                    }
                    if (IsImmediateOffsetOutOfRange(*memOpnd, GetPointerSize())) {
                        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, GetPointerSize());
                    }
                    SelectCopy(dest, PTY_u64, *memOpnd, PTY_u64);
                }
            } else {
                /* No side-effects.  No need to generate anything for eval. */
            }
        } else {
            if (expr.GetFieldID() != 0) {
                CHECK_FATAL(false, "SelectDread: Illegal agg size");
            }
        }
    }
    if (parmCopy) {
        memOpnd = &LoadStructCopyBase(*symbol, offset, static_cast<int>(dataSize));
    } else {
        memOpnd = &GetOrCreateMemOpnd(*symbol, offset, dataSize);
    }
    if ((memOpnd->GetMemVaryType() == kNotVary) && IsImmediateOffsetOutOfRange(*memOpnd, dataSize)) {
        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, dataSize);
    }

    RegOperand &resOpnd = GetOrCreateResOperand(parent, symType);
    SelectCopy(resOpnd, resultType, *memOpnd, symType);
    return &resOpnd;
}

RegOperand *AArch64CGFunc::SelectRegread(RegreadNode &expr)
{
    PregIdx pregIdx = expr.GetRegIdx();
    if (IsSpecialPseudoRegister(pregIdx)) {
        /* if it is one of special registers */
        return &GetOrCreateSpecialRegisterOperand(-pregIdx, expr.GetPrimType());
    }
    RegOperand &reg = GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    if (GetOpndFromPregIdx(pregIdx) == nullptr) {
        SetPregIdx2Opnd(pregIdx, reg);
    }
    if (expr.GetPrimType() == PTY_ref) {
        reg.SetIsReference(true);
        AddReferenceReg(reg.GetRegisterNumber());
    }
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
        MemOperand *src = GetPseudoRegisterSpillMemoryOperand(pregIdx);
        MIRPreg *preg = GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
        PrimType stype = preg->GetPrimType();
        uint32 srcBitLength = GetPrimTypeSize(stype) * kBitsPerByte;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickLdInsn(srcBitLength, stype), reg, *src));
    }
    return &reg;
}

Operand *AArch64CGFunc::SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset,
                                    PrimType finalBitFieldDestType)
{
    int32 offset = 0;
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(expr.GetTyIdx());
    MIRPtrType *pointerType = static_cast<MIRPtrType *>(type);
    DEBUG_ASSERT(pointerType != nullptr, "expect a pointer type at iread node");
    MIRType *pointedType = nullptr;
    bool isRefField = false;

    pointedType = GetPointedToType(*pointerType);

    RegType regType = GetRegTyFromPrimTy(expr.GetPrimType());
    uint32 regSize = GetPrimTypeSize(expr.GetPrimType());
    if (regSize < k4ByteSize) {
        regSize = k4ByteSize; /* 32-bit */
    }
    Operand *result = nullptr;
    constexpr int regSizeMax = 8;
    if (parent.GetOpCode() == OP_eval && regSize <= regSizeMax) {
        /* regSize << 3, that is regSize * 8, change bytes to bits */
        result = &GetZeroOpnd(regSize << 3);
    } else {
        result = &GetOrCreateResOperand(parent, expr.GetPrimType());
    }

    PrimType destType = pointedType->GetPrimType();

    uint32 bitSize = GetPrimTypeBitSize(destType);
    if (regType == kRegTyFloat) {
        destType = expr.GetPrimType();
        bitSize = GetPrimTypeBitSize(destType);
    }

    PrimType memType = (finalBitFieldDestType == kPtyInvalid ? destType : finalBitFieldDestType);
    MemOperand *memOpnd = CreateMemOpndOrNull(memType, expr, *expr.Opnd(0),
                                              static_cast<int64>(static_cast<int>(offset) + extraOffset));
    if (aggParamReg != nullptr) {
        isAggParamInReg = false;
        return aggParamReg;
    }
    DEBUG_ASSERT(memOpnd != nullptr, "memOpnd should not be nullptr");
    MOperator mOp = 0;
    if (finalBitFieldDestType == kPtyInvalid) {
        mOp = PickLdInsn(bitSize, destType);
    } else {
        mOp = PickLdInsn(GetPrimTypeBitSize(finalBitFieldDestType), finalBitFieldDestType);
    }
    if ((memOpnd->GetMemVaryType() == kNotVary) && !IsOperandImmValid(mOp, memOpnd, 1)) {
        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, bitSize);
    }
    Insn &insn = GetInsnBuilder()->BuildInsn(mOp, *result, *memOpnd);
    if (parent.GetOpCode() == OP_eval && result->IsRegister() &&
        static_cast<RegOperand *>(result)->GetRegisterNumber() == RZR) {
        insn.SetComment("null-check");
    }
    GetCurBB()->AppendInsn(insn);

    if (parent.op != OP_eval) {
        const InsnDesc *md = &AArch64CG::kMd[insn.GetMachineOpcode()];
        auto *prop = md->GetOpndDes(0);
        if ((prop->GetSize()) < insn.GetOperand(0).GetSize()) {
            switch (destType) {
                case PTY_i8:
                    mOp = MOP_xsxtb64;
                    break;
                case PTY_i16:
                    mOp = MOP_xsxth64;
                    break;
                case PTY_i32:
                    mOp = MOP_xsxtw64;
                    break;
                case PTY_u1:
                case PTY_u8:
                    mOp = MOP_xuxtb32;
                    break;
                case PTY_u16:
                    mOp = MOP_xuxth32;
                    break;
                case PTY_u32:
                    mOp = MOP_xuxtw64;
                    break;
                default:
                    break;
            }
            if (destType == PTY_u1) {
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wandrri12, insn.GetOperand(0),
                    insn.GetOperand(0), CreateImmOperand(1, kMaxImmVal5Bits, false)));
            }

            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, insn.GetOperand(0), insn.GetOperand(0)));
        }
    }
    if (GetCurBB() && GetCurBB()->GetLastMachineInsn()) {
        GetCurBB()->GetLastMachineInsn()->MarkAsAccessRefField(isRefField);
    }
    return result;
}

Operand *AArch64CGFunc::SelectIntConst(const MIRIntConst &intConst, const BaseNode &parent)
{
    auto primType = intConst.GetType().GetPrimType();
    if (kOpcodeInfo.IsCompare(parent.GetOpCode())) {
        primType = static_cast<const CompareNode &>(parent).GetOpndType();
    }
    return &CreateImmOperand(intConst.GetExtValue(), GetPrimTypeBitSize(primType), false);
}

Operand *AArch64CGFunc::HandleFmovImm(PrimType stype, int64 val, MIRConst &mirConst, const BaseNode &parent)
{
    Operand *result;
    bool is64Bits = (GetPrimTypeBitSize(stype) == k64BitSize);
    uint64 canRepreset = is64Bits ? (val & 0xffffffffffff) : (val & 0x7ffff);
    uint32 val1 = is64Bits ? (val >> 61) & 0x3 : (val >> 29) & 0x3;
    uint32 val2 = is64Bits ? (val >> 54) & 0xff : (val >> 25) & 0x1f;
    bool isSame = is64Bits ? ((val2 == 0) || (val2 == 0xff)) : ((val2 == 0) || (val2 == 0x1f));
    canRepreset = (canRepreset == 0) && ((val1 & 0x1) ^ ((val1 & 0x2) >> 1)) && isSame;
    if (canRepreset) {
        uint64 temp1 = is64Bits ? (val >> 63) << 7 : (val >> 31) << 7;
        uint64 temp2 = is64Bits ? val >> 48 : val >> 19;
        int64 imm8 = (temp2 & 0x7f) | temp1;
        Operand *newOpnd0 = &CreateImmOperand(imm8, k8BitSize, true, kNotVary, true);
        result = &GetOrCreateResOperand(parent, stype);
        MOperator mopFmov = (is64Bits ? MOP_xdfmovri : MOP_wsfmovri);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopFmov, *result, *newOpnd0));
    } else {
        Operand *newOpnd0 = &CreateImmOperand(val, GetPrimTypeSize(stype) * kBitsPerByte, false);
        PrimType itype = (stype == PTY_f32) ? PTY_i32 : PTY_i64;
        RegOperand &regOpnd = LoadIntoRegister(*newOpnd0, itype);

        result = &GetOrCreateResOperand(parent, stype);
        MOperator mopFmov = (is64Bits ? MOP_xvmovdr : MOP_xvmovsr);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopFmov, *result, regOpnd));
    }
    return result;
}

Operand *AArch64CGFunc::SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent)
{
    PrimType stype = floatConst.GetType().GetPrimType();
    int32 val = floatConst.GetIntValue();
    /* according to aarch64 encoding format, convert int to float expression */
    Operand *result;
    result = HandleFmovImm(stype, val, floatConst, parent);
    return result;
}

Operand *AArch64CGFunc::SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent)
{
    PrimType stype = doubleConst.GetType().GetPrimType();
    int64 val = doubleConst.GetIntValue();
    /* according to aarch64 encoding format, convert int to float expression */
    Operand *result;
    result = HandleFmovImm(stype, val, doubleConst, parent);
    return result;
}

/*
 * Returns the number of leading 0-bits in x, starting at the most significant bit position.
 * If x is 0, the result is -1.
 */
static int32 GetHead0BitNum(int64 val)
{
    uint32 bitNum = 0;
    for (; bitNum < k64BitSize; bitNum++) {
        if ((0x8000000000000000ULL >> static_cast<uint32>(bitNum)) & static_cast<uint64>(val)) {
            break;
        }
    }
    if (bitNum == k64BitSize) {
        return -1;
    }
    return bitNum;
}

/*
 * Returns the number of trailing 0-bits in x, starting at the least significant bit position.
 * If x is 0, the result is -1.
 */
static int32 GetTail0BitNum(int64 val)
{
    uint32 bitNum = 0;
    for (; bitNum < k64BitSize; bitNum++) {
        if ((static_cast<uint64>(1) << static_cast<uint32>(bitNum)) & static_cast<uint64>(val)) {
            break;
        }
    }
    if (bitNum == k64BitSize) {
        return -1;
    }
    return bitNum;
}

/*
 * If the input integer is power of 2, return log2(input)
 * else return -1
 */
static inline int32 GetLog2(uint64 val)
{
    if (__builtin_popcountll(val) == 1) {
        return __builtin_ffsll(static_cast<int64>(val)) - 1;
    }
    return -1;
}

MOperator AArch64CGFunc::PickJmpInsn(Opcode brOp, Opcode cmpOp, bool isFloat, bool isSigned) const
{
    switch (cmpOp) {
        case OP_ne:
            return (brOp == OP_brtrue) ? MOP_bne : MOP_beq;
        case OP_eq:
            return (brOp == OP_brtrue) ? MOP_beq : MOP_bne;
        case OP_lt:
            return (brOp == OP_brtrue) ? (isSigned ? MOP_blt : MOP_blo)
                                       : (isFloat ? MOP_bpl : (isSigned ? MOP_bge : MOP_bhs));
        case OP_le:
            return (brOp == OP_brtrue) ? (isSigned ? MOP_ble : MOP_bls)
                                       : (isFloat ? MOP_bhi : (isSigned ? MOP_bgt : MOP_bhi));
        case OP_gt:
            return (brOp == OP_brtrue) ? (isFloat ? MOP_bgt : (isSigned ? MOP_bgt : MOP_bhi))
                                       : (isSigned ? MOP_ble : MOP_bls);
        case OP_ge:
            return (brOp == OP_brtrue) ? (isFloat ? MOP_bpl : (isSigned ? MOP_bge : MOP_bhs))
                                       : (isSigned ? MOP_blt : MOP_blo);
        default:
            CHECK_FATAL(false, "PickJmpInsn error");
    }
}

bool AArch64CGFunc::GenerateCompareWithZeroInstruction(Opcode jmpOp, Opcode cmpOp, bool is64Bits, PrimType primType,
                                                       LabelOperand &targetOpnd, Operand &opnd0)
{
    bool finish = true;
    MOperator mOpCode = MOP_undef;
    switch (cmpOp) {
        case OP_ne: {
            if (jmpOp == OP_brtrue) {
                mOpCode = is64Bits ? MOP_xcbnz : MOP_wcbnz;
            } else {
                mOpCode = is64Bits ? MOP_xcbz : MOP_wcbz;
            }
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, opnd0, targetOpnd));
            break;
        }
        case OP_eq: {
            if (jmpOp == OP_brtrue) {
                mOpCode = is64Bits ? MOP_xcbz : MOP_wcbz;
            } else {
                mOpCode = is64Bits ? MOP_xcbnz : MOP_wcbnz;
            }
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, opnd0, targetOpnd));
            break;
        }
        /*
         * TBZ/TBNZ instruction have a range of +/-32KB, need to check if the jump target is reachable in a later
         * phase. If the branch target is not reachable, then we change tbz/tbnz into combination of ubfx and
         * cbz/cbnz, which will clobber one extra register. With LSRA under O2, we can use of the reserved registers
         * for that purpose.
         */
        case OP_lt: {
            if (primType == PTY_u64 || primType == PTY_u32) {
                return false;
            }
            ImmOperand &signBit =
                CreateImmOperand(is64Bits ? kHighestBitOf64Bits : kHighestBitOf32Bits, k8BitSize, false);
            if (jmpOp == OP_brtrue) {
                mOpCode = is64Bits ? MOP_xtbnz : MOP_wtbnz;
            } else {
                mOpCode = is64Bits ? MOP_xtbz : MOP_wtbz;
            }
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, opnd0, signBit, targetOpnd));
            break;
        }
        case OP_ge: {
            if (primType == PTY_u64 || primType == PTY_u32) {
                return false;
            }
            ImmOperand &signBit =
                CreateImmOperand(is64Bits ? kHighestBitOf64Bits : kHighestBitOf32Bits, k8BitSize, false);
            if (jmpOp == OP_brtrue) {
                mOpCode = is64Bits ? MOP_xtbz : MOP_wtbz;
            } else {
                mOpCode = is64Bits ? MOP_xtbnz : MOP_wtbnz;
            }
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, opnd0, signBit, targetOpnd));
            break;
        }
        default:
            finish = false;
            break;
    }
    return finish;
}

void AArch64CGFunc::SelectCondGoto(LabelOperand &targetOpnd, Opcode jmpOp, Opcode cmpOp, Operand &origOpnd0,
                                   Operand &origOpnd1, PrimType primType, bool signedCond)
{
    Operand *opnd0 = &origOpnd0;
    Operand *opnd1 = &origOpnd1;
    opnd0 = &LoadIntoRegister(origOpnd0, primType);

    bool is64Bits = GetPrimTypeBitSize(primType) == k64BitSize;
    bool isFloat = IsPrimitiveFloat(primType);
    Operand &rflag = GetOrCreateRflag();
    if (isFloat) {
        opnd1 = &LoadIntoRegister(origOpnd1, primType);
        MOperator mOp =
            is64Bits ? MOP_dcmperr : ((GetPrimTypeBitSize(primType) == k32BitSize) ? MOP_scmperr : MOP_hcmperr);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, rflag, *opnd0, *opnd1));
    } else {
        bool isImm = ((origOpnd1.GetKind() == Operand::kOpdImmediate) || (origOpnd1.GetKind() == Operand::kOpdOffset));
        if ((origOpnd1.GetKind() != Operand::kOpdRegister) && !isImm) {
            opnd1 = &SelectCopy(origOpnd1, primType, primType);
        }
        MOperator mOp = is64Bits ? MOP_xcmprr : MOP_wcmprr;

        if (isImm) {
            if (static_cast<ImmOperand *>(opnd1)->IsZero() &&
                (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0)) {
                bool finish = GenerateCompareWithZeroInstruction(jmpOp, cmpOp, is64Bits, primType, targetOpnd, *opnd0);
                if (finish) {
                    return;
                }
            }

            /*
             * aarch64 assembly takes up to 24-bits immediate, generating
             * either cmp or cmp with shift 12 encoding
             */
            ImmOperand *immOpnd = static_cast<ImmOperand *>(opnd1);
            if (immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits)) {
                mOp = is64Bits ? MOP_xcmpri : MOP_wcmpri;
            } else {
                opnd1 = &SelectCopy(*opnd1, primType, primType);
            }
        }
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, rflag, *opnd0, *opnd1));
    }

    bool isSigned = IsPrimitiveInteger(primType) ? IsSignedInteger(primType) : (signedCond ? true : false);
    MOperator jmpOperator = PickJmpInsn(jmpOp, cmpOp, isFloat, isSigned);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(jmpOperator, rflag, targetOpnd));
}

/*
 *   brtrue @label0 (ge u8 i32 (
 *   cmp i32 i64 (dread i64 %Reg2_J, dread i64 %Reg4_J),
 *   constval i32 0))
 *  ===>
 *   cmp r1, r2
 *   bge Cond, label0
 */
void AArch64CGFunc::SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &expr)
{
    DEBUG_ASSERT(expr.GetOpCode() == OP_cmp, "unexpect opcode");
    Operand *opnd0 = HandleExpr(expr, *expr.Opnd(0));
    Operand *opnd1 = HandleExpr(expr, *expr.Opnd(1));
    CompareNode *node = static_cast<CompareNode *>(&expr);
    bool isFloat = IsPrimitiveFloat(node->GetOpndType());
    opnd0 = &LoadIntoRegister(*opnd0, node->GetOpndType());
    /*
     * most of FP constants are passed as MemOperand
     * except 0.0 which is passed as kOpdFPImmediate
     */
    Operand::OperandType opnd1Type = opnd1->GetKind();
    if ((opnd1Type != Operand::kOpdImmediate) && (opnd1Type != Operand::kOpdFPImmediate) &&
        (opnd1Type != Operand::kOpdOffset)) {
        opnd1 = &LoadIntoRegister(*opnd1, node->GetOpndType());
    }
    SelectAArch64Cmp(*opnd0, *opnd1, !isFloat, GetPrimTypeBitSize(node->GetOpndType()));
    /* handle condgoto now. */
    LabelIdx labelIdx = stmt.GetOffset();
    BaseNode *condNode = stmt.Opnd(0);
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(labelIdx);
    Opcode cmpOp = condNode->GetOpCode();
    PrimType pType = static_cast<CompareNode *>(condNode)->GetOpndType();
    isFloat = IsPrimitiveFloat(pType);
    Operand &rflag = GetOrCreateRflag();
    bool isSigned =
        IsPrimitiveInteger(pType) ? IsSignedInteger(pType) : (IsSignedInteger(condNode->GetPrimType()) ? true : false);
    MOperator jmpOp = PickJmpInsn(stmt.GetOpCode(), cmpOp, isFloat, isSigned);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(jmpOp, rflag, targetOpnd));
}

/*
 * Special case:
 * brfalse(ge (cmpg (op0, op1), 0) ==>
 * fcmp op1, op2
 * blo
 */
void AArch64CGFunc::SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &expr)
{
    auto &cmpNode = static_cast<CompareNode &>(expr);
    Operand *opnd0 = HandleExpr(cmpNode, *cmpNode.Opnd(0));
    Operand *opnd1 = HandleExpr(cmpNode, *cmpNode.Opnd(1));
    PrimType operandType = cmpNode.GetOpndType();
    opnd0 = opnd0->IsRegister() ? static_cast<RegOperand *>(opnd0) : &SelectCopy(*opnd0, operandType, operandType);
    Operand::OperandType opnd1Type = opnd1->GetKind();
    if ((opnd1Type != Operand::kOpdImmediate) && (opnd1Type != Operand::kOpdFPImmediate) &&
        (opnd1Type != Operand::kOpdOffset)) {
        opnd1 = opnd1->IsRegister() ? static_cast<RegOperand *>(opnd1) : &SelectCopy(*opnd1, operandType, operandType);
    }
#ifdef DEBUG
    bool isFloat = IsPrimitiveFloat(operandType);
    if (!isFloat) {
        DEBUG_ASSERT(false, "incorrect operand types");
    }
#endif
    SelectTargetFPCmpQuiet(*opnd0, *opnd1, GetPrimTypeBitSize(operandType));
    Operand &rFlag = GetOrCreateRflag();
    LabelIdx tempLabelIdx = stmt.GetOffset();
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(tempLabelIdx);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_blo, rFlag, targetOpnd));
}

void AArch64CGFunc::SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1)
{
    /*
     * handle brfalse/brtrue op, opnd0 can be a compare node or non-compare node
     * such as a dread for example
     */
    LabelIdx labelIdx = stmt.GetOffset();
    BaseNode *condNode = stmt.Opnd(0);
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(labelIdx);
    Opcode cmpOp;

    PrimType pType;
    if (kOpcodeInfo.IsCompare(condNode->GetOpCode())) {
        cmpOp = condNode->GetOpCode();
        pType = static_cast<CompareNode *>(condNode)->GetOpndType();
    } else {
        /* not a compare node; dread for example, take its pType */
        cmpOp = OP_ne;
        pType = condNode->GetPrimType();
    }
    bool signedCond = IsSignedInteger(pType) || IsPrimitiveFloat(pType);
    SelectCondGoto(targetOpnd, stmt.GetOpCode(), cmpOp, opnd0, opnd1, pType, signedCond);
}

void AArch64CGFunc::SelectGoto(GotoNode &stmt)
{
    Operand &targetOpnd = GetOrCreateLabelOperand(stmt.GetOffset());
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd));
    GetCurBB()->SetKind(BB::kBBGoto);
}

Operand *AArch64CGFunc::SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(dtype);
    RegOperand *resOpnd = nullptr;
    /* promoted type */
    PrimType primType =
        isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
    if (parent.GetOpCode() == OP_regassign) {
        auto &regAssignNode = static_cast<const RegassignNode &>(parent);
        PregIdx pregIdx = regAssignNode.GetRegIdx();
        if (IsSpecialPseudoRegister(pregIdx)) {
            resOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, dtype);
        } else {
            resOpnd = &GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
        }
    } else {
        resOpnd = &CreateRegisterOperandOfType(primType);
    }
    SelectAdd(*resOpnd, opnd0, opnd1, primType);
    return resOpnd;
}

void AArch64CGFunc::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    Operand::OperandType opnd0Type = opnd0.GetKind();
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    if (opnd0Type != Operand::kOpdRegister) {
        /* add #imm, #imm */
        if (opnd1Type != Operand::kOpdRegister) {
            SelectAdd(resOpnd, SelectCopy(opnd0, primType, primType), opnd1, primType);
            return;
        }
        /* add #imm, reg */
        SelectAdd(resOpnd, opnd1, opnd0, primType); /* commutative */
        return;
    }
    /* add reg, reg */
    if (opnd1Type == Operand::kOpdRegister) {
        DEBUG_ASSERT(IsPrimitiveFloat(primType) || IsPrimitiveInteger(primType), "NYI add");
        MOperator mOp =
            IsPrimitiveFloat(primType) ? (is64Bits ? MOP_dadd : MOP_sadd) : (is64Bits ? MOP_xaddrrr : MOP_waddrrr);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, opnd1));
        return;
    } else if (opnd1Type == Operand::kOpdStImmediate) {
        CHECK_FATAL(is64Bits, "baseReg of mem in aarch64 must be 64bit size");
        /* add reg, reg, #:lo12:sym+offset */
        StImmOperand &stImmOpnd = static_cast<StImmOperand &>(opnd1);
        Insn &newInsn = GetInsnBuilder()->BuildInsn(MOP_xadrpl12, resOpnd, opnd0, stImmOpnd);
        GetCurBB()->AppendInsn(newInsn);
        return;
    } else if (!((opnd1Type == Operand::kOpdImmediate) || (opnd1Type == Operand::kOpdOffset))) {
        /* add reg, otheregType */
        SelectAdd(resOpnd, opnd0, SelectCopy(opnd1, primType, primType), primType);
        return;
    } else {
        /* add reg, #imm */
        ImmOperand *immOpnd = static_cast<ImmOperand *>(&opnd1);
        if (immOpnd->IsNegative()) {
            immOpnd->Negate();
            SelectSub(resOpnd, opnd0, *immOpnd, primType);
            return;
        }
        if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0)) {
            /*
             * ADD Wd|WSP, Wn|WSP, #imm{, shift} ; 32-bit general registers
             * ADD Xd|SP,  Xn|SP,  #imm{, shift} ; 64-bit general registers
             * imm : 0 ~ 4095, shift: none, LSL #0, or LSL #12
             * aarch64 assembly takes up to 24-bits, if the lower 12 bits is all 0
             */
            MOperator mOpCode = MOP_undef;
            Operand *newOpnd0 = &opnd0;
            if (!(immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) ||
                  immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
                /* process higher 12 bits */
                ImmOperand &immOpnd2 =
                    CreateImmOperand(static_cast<int64>(static_cast<uint64>(immOpnd->GetValue()) >> kMaxImmVal12Bits),
                                     immOpnd->GetSize(), immOpnd->IsSignedValue());
                mOpCode = is64Bits ? MOP_xaddrri24 : MOP_waddrri24;
                Operand *tmpRes = IsAfterRegAlloc() ? &resOpnd : &CreateRegisterOperandOfType(primType);
                BitShiftOperand &shiftopnd = CreateBitShiftOperand(BitShiftOperand::kLSL, kShiftAmount12, k64BitSize);
                Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, *tmpRes, opnd0, immOpnd2, shiftopnd);
                GetCurBB()->AppendInsn(newInsn);
                immOpnd->ModuloByPow2(static_cast<int32>(kMaxImmVal12Bits));
                newOpnd0 = tmpRes;
            }
            /* process lower 12  bits */
            mOpCode = is64Bits ? MOP_xaddrri12 : MOP_waddrri12;
            Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, *newOpnd0, *immOpnd);
            GetCurBB()->AppendInsn(newInsn);
            return;
        }
        /* load into register */
        int64 immVal = immOpnd->GetValue();
        int32 tail0bitNum = GetTail0BitNum(immVal);
        int32 head0bitNum = GetHead0BitNum(immVal);
        const int32 bitNum = (k64BitSizeInt - head0bitNum) - tail0bitNum;
        RegOperand &regOpnd = CreateRegisterOperandOfType(primType);
        if (isAfterRegAlloc) {
            RegType regty = GetRegTyFromPrimTy(primType);
            uint32 bytelen = GetPrimTypeSize(primType);
            regOpnd = GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16), bytelen, regty);
        }
        regno_t regNO0 = static_cast<RegOperand &>(opnd0).GetRegisterNumber();
        /* addrrrs do not support sp */
        if (bitNum <= k16ValidBit && regNO0 != RSP) {
            int64 newImm = (static_cast<uint64>(immVal) >> static_cast<uint32>(tail0bitNum)) & 0xFFFF;
            ImmOperand &immOpnd1 = CreateImmOperand(newImm, k16BitSize, false);
            SelectCopyImm(regOpnd, immOpnd1, primType);
            uint32 mopBadd = is64Bits ? MOP_xaddrrrs : MOP_waddrrrs;
            int32 bitLen = is64Bits ? kBitLenOfShift64Bits : kBitLenOfShift32Bits;
            BitShiftOperand &bitShiftOpnd =
                CreateBitShiftOperand(BitShiftOperand::kLSL, static_cast<uint32>(tail0bitNum), bitLen);
            Insn &newInsn = GetInsnBuilder()->BuildInsn(mopBadd, resOpnd, opnd0, regOpnd, bitShiftOpnd);
            GetCurBB()->AppendInsn(newInsn);
            return;
        }

        SelectCopyImm(regOpnd, *immOpnd, primType);
        MOperator mOpCode = is64Bits ? MOP_xaddrrr : MOP_waddrrr;
        Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, opnd0, regOpnd);
        GetCurBB()->AppendInsn(newInsn);
    }
}

void AArch64CGFunc::SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(primType);
    Operand *opnd0Bak = &LoadIntoRegister(opnd0, primType);
    if (opnd1Type == Operand::kOpdRegister) {
        MOperator mOp = isFloat ? (is64Bits ? MOP_dsub : MOP_ssub) : (is64Bits ? MOP_xsubrrr : MOP_wsubrrr);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, *opnd0Bak, opnd1));
        return;
    }

    if ((opnd1Type != Operand::kOpdImmediate) && (opnd1Type != Operand::kOpdOffset)) {
        SelectSub(resOpnd, *opnd0Bak, SelectCopy(opnd1, primType, primType), primType);
        return;
    }

    ImmOperand *immOpnd = static_cast<ImmOperand *>(&opnd1);
    if (immOpnd->IsNegative()) {
        immOpnd->Negate();
        SelectAdd(resOpnd, *opnd0Bak, *immOpnd, primType);
        return;
    }

    int64 higher12BitVal = static_cast<int64>(static_cast<uint64>(immOpnd->GetValue()) >> kMaxImmVal12Bits);
    if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0) && higher12BitVal + 1 <= kMaxPimm8) {
        /*
         * SUB Wd|WSP, Wn|WSP, #imm{, shift} ; 32-bit general registers
         * SUB Xd|SP,  Xn|SP,  #imm{, shift} ; 64-bit general registers
         * imm : 0 ~ 4095, shift: none, LSL #0, or LSL #12
         * aarch64 assembly takes up to 24-bits, if the lower 12 bits is all 0
         * large offset is treated as sub (higher 12 bits + 4096) + add
         * it gives opportunities for combining add + ldr due to the characteristics of aarch64's load/store
         */
        MOperator mOpCode = MOP_undef;
        bool isSplitSub = false;
        if (!(immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
            isSplitSub = true;
            /* process higher 12 bits */
            ImmOperand &immOpnd2 = CreateImmOperand(higher12BitVal + 1, immOpnd->GetSize(), immOpnd->IsSignedValue());

            mOpCode = is64Bits ? MOP_xsubrri24 : MOP_wsubrri24;
            BitShiftOperand &shiftopnd = CreateBitShiftOperand(BitShiftOperand::kLSL, kShiftAmount12, k64BitSize);
            Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, *opnd0Bak, immOpnd2, shiftopnd);
            GetCurBB()->AppendInsn(newInsn);
            immOpnd->ModuloByPow2(static_cast<int64>(kMaxImmVal12Bits));
            immOpnd->SetValue(static_cast<int64>(kMax12UnsignedImm) - immOpnd->GetValue());
            opnd0Bak = &resOpnd;
        }
        /* process lower 12 bits */
        mOpCode = isSplitSub ? (is64Bits ? MOP_xaddrri12 : MOP_waddrri12) : (is64Bits ? MOP_xsubrri12 : MOP_wsubrri12);
        Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, *opnd0Bak, *immOpnd);
        GetCurBB()->AppendInsn(newInsn);
        return;
    }

    /* load into register */
    int64 immVal = immOpnd->GetValue();
    int32 tail0bitNum = GetTail0BitNum(immVal);
    int32 head0bitNum = GetHead0BitNum(immVal);
    const int32 bitNum = (k64BitSizeInt - head0bitNum) - tail0bitNum;
    RegOperand &regOpnd = CreateRegisterOperandOfType(primType);
    if (isAfterRegAlloc) {
        RegType regty = GetRegTyFromPrimTy(primType);
        uint32 bytelen = GetPrimTypeSize(primType);
        regOpnd = GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R16), bytelen, regty);
    }

    if (bitNum <= k16ValidBit) {
        int64 newImm = (static_cast<uint64>(immVal) >> static_cast<uint32>(tail0bitNum)) & 0xFFFF;
        ImmOperand &immOpnd1 = CreateImmOperand(newImm, k16BitSize, false);
        SelectCopyImm(regOpnd, immOpnd1, primType);
        uint32 mopBsub = is64Bits ? MOP_xsubrrrs : MOP_wsubrrrs;
        int32 bitLen = is64Bits ? kBitLenOfShift64Bits : kBitLenOfShift32Bits;
        BitShiftOperand &bitShiftOpnd =
            CreateBitShiftOperand(BitShiftOperand::kLSL, static_cast<uint32>(tail0bitNum), bitLen);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBsub, resOpnd, *opnd0Bak, regOpnd, bitShiftOpnd));
        return;
    }

    SelectCopyImm(regOpnd, *immOpnd, primType);
    MOperator mOpCode = is64Bits ? MOP_xsubrrr : MOP_wsubrrr;
    Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, *opnd0Bak, regOpnd);
    GetCurBB()->AppendInsn(newInsn);
}

Operand *AArch64CGFunc::SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(dtype);
    RegOperand *resOpnd = nullptr;
    PrimType primType =
        isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
    resOpnd = &GetOrCreateResOperand(parent, primType);
    SelectSub(*resOpnd, opnd0, opnd1, primType);
    return resOpnd;
}

Operand *AArch64CGFunc::SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(dtype);
    RegOperand *resOpnd = nullptr;
    PrimType primType =
        isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
    resOpnd = &GetOrCreateResOperand(parent, primType);
    SelectMpy(*resOpnd, opnd0, opnd1, primType);
    return resOpnd;
}

void AArch64CGFunc::SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    Operand::OperandType opnd0Type = opnd0.GetKind();
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);

    if (((opnd0Type == Operand::kOpdImmediate) || (opnd0Type == Operand::kOpdOffset) ||
         (opnd1Type == Operand::kOpdImmediate) || (opnd1Type == Operand::kOpdOffset)) &&
        IsPrimitiveInteger(primType)) {
        ImmOperand *imm = ((opnd0Type == Operand::kOpdImmediate) || (opnd0Type == Operand::kOpdOffset))
                              ? static_cast<ImmOperand *>(&opnd0)
                              : static_cast<ImmOperand *>(&opnd1);
        Operand *otherOp =
            ((opnd0Type == Operand::kOpdImmediate) || (opnd0Type == Operand::kOpdOffset)) ? &opnd1 : &opnd0;
        int64 immValue = llabs(imm->GetValue());
        if (immValue != 0 && (static_cast<uint64>(immValue) & (static_cast<uint64>(immValue) - 1)) == 0) {
            /* immValue is 1 << n */
            if (otherOp->GetKind() != Operand::kOpdRegister) {
                otherOp = &SelectCopy(*otherOp, primType, primType);
            }
            int64 shiftVal = __builtin_ffsll(immValue);
            ImmOperand &shiftNum = CreateImmOperand(shiftVal - 1, dsize, false);
            SelectShift(resOpnd, *otherOp, shiftNum, kShiftLeft, primType);
            bool reachSignBit = (is64Bits && (shiftVal == k64BitSize)) || (!is64Bits && (shiftVal == k32BitSize));
            if (imm->GetValue() < 0 && !reachSignBit) {
                SelectNeg(resOpnd, resOpnd, primType);
            }

            return;
        } else if (immValue > 2) {  // immValue should larger than 2
            uint32 zeroNum = static_cast<uint32>(__builtin_ffsll(immValue) - 1);
            int64 headVal = static_cast<uint64>(immValue) >> zeroNum;
            /*
             * if (headVal - 1) & (headVal - 2) == 0, that is (immVal >> zeroNum) - 1 == 1 << n
             * otherOp * immVal = (otherOp * (immVal >> zeroNum) * (1 << zeroNum)
             * = (otherOp * ((immVal >> zeroNum) - 1) + otherOp) * (1 << zeroNum)
             */
            CHECK_FATAL(static_cast<uint64>(headVal) >= 2, "value overflow");
            // 2 see comment above
            if (((static_cast<uint64>(headVal) - 1) & (static_cast<uint64>(headVal) - 2)) == 0) {
                if (otherOp->GetKind() != Operand::kOpdRegister) {
                    otherOp = &SelectCopy(*otherOp, primType, primType);
                }
                ImmOperand &shiftNum1 = CreateImmOperand(__builtin_ffsll(headVal - 1) - 1, dsize, false);
                RegOperand &tmpOpnd = CreateRegisterOperandOfType(primType);
                SelectShift(tmpOpnd, *otherOp, shiftNum1, kShiftLeft, primType);
                SelectAdd(resOpnd, *otherOp, tmpOpnd, primType);
                ImmOperand &shiftNum2 = CreateImmOperand(zeroNum, dsize, false);
                SelectShift(resOpnd, resOpnd, shiftNum2, kShiftLeft, primType);
                if (imm->GetValue() < 0) {
                    SelectNeg(resOpnd, resOpnd, primType);
                }

                return;
            }
        }
    }

    if ((opnd0Type != Operand::kOpdRegister) && (opnd1Type != Operand::kOpdRegister)) {
        SelectMpy(resOpnd, SelectCopy(opnd0, primType, primType), opnd1, primType);
    } else if ((opnd0Type == Operand::kOpdRegister) && (opnd1Type != Operand::kOpdRegister)) {
        SelectMpy(resOpnd, opnd0, SelectCopy(opnd1, primType, primType), primType);
    } else if ((opnd0Type != Operand::kOpdRegister) && (opnd1Type == Operand::kOpdRegister)) {
        SelectMpy(resOpnd, opnd1, opnd0, primType);
    } else {
        DEBUG_ASSERT(IsPrimitiveFloat(primType) || IsPrimitiveInteger(primType), "NYI Mpy");
        MOperator mOp =
            IsPrimitiveFloat(primType) ? (is64Bits ? MOP_xvmuld : MOP_xvmuls) : (is64Bits ? MOP_xmulrrr : MOP_wmulrrr);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, opnd1));
    }
}

void AArch64CGFunc::SelectDiv(Operand &resOpnd, Operand &origOpnd0, Operand &opnd1, PrimType primType)
{
    Operand &opnd0 = LoadIntoRegister(origOpnd0, primType);
    Operand::OperandType opnd0Type = opnd0.GetKind();
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);

    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
        if (((opnd1Type == Operand::kOpdImmediate) || (opnd1Type == Operand::kOpdOffset)) &&
            IsSignedInteger(primType)) {
            ImmOperand *imm = static_cast<ImmOperand *>(&opnd1);
            int64 immValue = llabs(imm->GetValue());
            if ((immValue != 0) && (static_cast<uint64>(immValue) & (static_cast<uint64>(immValue) - 1)) == 0) {
                if (immValue == 1) {
                    if (imm->GetValue() > 0) {
                        uint32 mOp = is64Bits ? MOP_xmovrr : MOP_wmovrr;
                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0));
                    } else {
                        SelectNeg(resOpnd, opnd0, primType);
                    }

                    return;
                }
                int32 shiftNumber = __builtin_ffsll(immValue) - 1;
                ImmOperand &shiftNum = CreateImmOperand(shiftNumber, dsize, false);
                Operand &tmpOpnd = CreateRegisterOperandOfType(primType);
                SelectShift(tmpOpnd, opnd0, CreateImmOperand(dsize - 1, dsize, false), kShiftAright, primType);
                uint32 mopBadd = is64Bits ? MOP_xaddrrrs : MOP_waddrrrs;
                int32 bitLen = is64Bits ? kBitLenOfShift64Bits : kBitLenOfShift32Bits;
                BitShiftOperand &shiftOpnd = CreateBitShiftOperand(BitShiftOperand::kLSR, dsize - shiftNumber, bitLen);
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBadd, tmpOpnd, opnd0, tmpOpnd, shiftOpnd));
                SelectShift(resOpnd, tmpOpnd, shiftNum, kShiftAright, primType);
                if (imm->GetValue() < 0) {
                    SelectNeg(resOpnd, resOpnd, primType);
                }

                return;
            }
        } else if (((opnd1Type == Operand::kOpdImmediate) || (opnd1Type == Operand::kOpdOffset)) &&
                   IsUnsignedInteger(primType)) {
            ImmOperand *imm = static_cast<ImmOperand *>(&opnd1);
            if (imm->GetValue() != 0) {
                if ((imm->GetValue() > 0) &&
                    ((static_cast<uint64>(imm->GetValue()) & (static_cast<uint64>(imm->GetValue()) - 1)) == 0)) {
                    ImmOperand &shiftNum = CreateImmOperand(__builtin_ffsll(imm->GetValue()) - 1, dsize, false);
                    SelectShift(resOpnd, opnd0, shiftNum, kShiftLright, primType);

                    return;
                } else if (imm->GetValue() < 0) {
                    SelectAArch64Cmp(opnd0, *imm, true, dsize);
                    SelectAArch64CSet(resOpnd, GetCondOperand(CC_CS), is64Bits);

                    return;
                }
            }
        }
    }

    if (opnd0Type != Operand::kOpdRegister) {
        SelectDiv(resOpnd, SelectCopy(opnd0, primType, primType), opnd1, primType);
    } else if (opnd1Type != Operand::kOpdRegister) {
        SelectDiv(resOpnd, opnd0, SelectCopy(opnd1, primType, primType), primType);
    } else {
        DEBUG_ASSERT(IsPrimitiveFloat(primType) || IsPrimitiveInteger(primType), "NYI Div");
        MOperator mOp = IsPrimitiveFloat(primType)
                            ? (is64Bits ? MOP_ddivrrr : MOP_sdivrrr)
                            : (IsSignedInteger(primType) ? (is64Bits ? MOP_xsdivrrr : MOP_wsdivrrr)
                                                         : (is64Bits ? MOP_xudivrrr : MOP_wudivrrr));
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, opnd1));
    }
}

Operand *AArch64CGFunc::SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(dtype);
    /* promoted type */
    PrimType primType =
        isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
    RegOperand &resOpnd = GetOrCreateResOperand(parent, primType);
    SelectDiv(resOpnd, opnd0, opnd1, primType);
    return &resOpnd;
}

void AArch64CGFunc::SelectRem(Operand &resOpnd, Operand &lhsOpnd, Operand &rhsOpnd, PrimType primType, bool isSigned,
                              bool is64Bits)
{
    Operand &opnd0 = LoadIntoRegister(lhsOpnd, primType);
    Operand &opnd1 = LoadIntoRegister(rhsOpnd, primType);

    DEBUG_ASSERT(IsPrimitiveInteger(primType), "Wrong type for REM");
    /*
     * printf("%d \n", 29 % 7 );
     * -> 1
     * printf("%u %d \n", (unsigned)-7, (unsigned)(-7) % 7 );
     * -> 4294967289 4
     * printf("%d \n", (-7) % 7 );
     * -> 0
     * printf("%d \n", 237 % -7 );
     * 6->
     * printf("implicit i->u conversion %d \n", ((unsigned)237) % -7 );
     * implicit conversion 237

     * http://stackoverflow.com/questions/35351470/obtaining-remainder-using-single-aarch64-instruction
     * input: x0=dividend, x1=divisor
     * udiv|sdiv x2, x0, x1
     * msub x3, x2, x1, x0  -- multply-sub : x3 <- x0 - x2*x1
     * result: x2=quotient, x3=remainder
     *
     * allocate temporary register
     */
    RegOperand &temp = CreateRegisterOperandOfType(primType);
    /*
     * mov     w1, #2
     * sdiv    wTemp, w0, w1
     * msub    wRespond, wTemp, w1, w0
     * ========>
     * asr     wTemp, w0, #31
     * lsr     wTemp, wTemp, #31  (#30 for 4, #29 for 8, ...)
     * add     wRespond, w0, wTemp
     * and     wRespond, wRespond, #1   (#3 for 4, #7 for 8, ...)
     * sub     wRespond, wRespond, w2
     *
     * if divde by 2
     * ========>
     * lsr     wTemp, w0, #31
     * add     wRespond, w0, wTemp
     * and     wRespond, wRespond, #1
     * sub     wRespond, wRespond, w2
     *
     * for unsigned rem op, just use and
     */
    if ((Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2)) {
        ImmOperand *imm = nullptr;
        Insn *movImmInsn = GetCurBB()->GetLastMachineInsn();
        if (movImmInsn &&
            ((movImmInsn->GetMachineOpcode() == MOP_wmovri32) || (movImmInsn->GetMachineOpcode() == MOP_xmovri64)) &&
            movImmInsn->GetOperand(0).Equals(opnd1)) {
            /*
             * mov w1, #2
             * rem res, w0, w1
             */
            imm = static_cast<ImmOperand *>(&movImmInsn->GetOperand(kInsnSecondOpnd));
        } else if (opnd1.IsImmediate()) {
            /*
             * rem res, w0, #2
             */
            imm = static_cast<ImmOperand *>(&opnd1);
        }
        /* positive or negative do not have effect on the result */
        int64 dividor = 0;
        if (imm && (imm->GetValue() != LONG_MIN)) {
            dividor = abs(imm->GetValue());
        }
        const int64 Log2OfDividor = GetLog2(static_cast<uint64>(dividor));
        if ((dividor != 0) && (Log2OfDividor > 0)) {
            if (is64Bits) {
                CHECK_FATAL(Log2OfDividor < k64BitSize, "imm out of bound");
                if (isSigned) {
                    ImmOperand &rightShiftValue = CreateImmOperand(k64BitSize - Log2OfDividor, k64BitSize, isSigned);
                    if (Log2OfDividor != 1) {
                        /* 63->shift ALL , 32 ->32bit register */
                        ImmOperand &rightShiftAll = CreateImmOperand(63, k64BitSize, isSigned);
                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xasrrri6, temp, opnd0, rightShiftAll));

                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xlsrrri6, temp, temp, rightShiftValue));
                    } else {
                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xlsrrri6, temp, opnd0, rightShiftValue));
                    }
                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xaddrrr, resOpnd, opnd0, temp));
                    ImmOperand &remBits = CreateImmOperand(dividor - 1, k64BitSize, isSigned);
                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xandrri13, resOpnd, resOpnd, remBits));
                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xsubrrr, resOpnd, resOpnd, temp));
                    return;
                } else if (imm && imm->GetValue() > 0) {
                    ImmOperand &remBits = CreateImmOperand(dividor - 1, k64BitSize, isSigned);
                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xandrri13, resOpnd, opnd0, remBits));
                    return;
                }
            } else {
                CHECK_FATAL(Log2OfDividor < k32BitSize, "imm out of bound");
                if (isSigned) {
                    ImmOperand &rightShiftValue = CreateImmOperand(k32BitSize - Log2OfDividor, k32BitSize, isSigned);
                    if (Log2OfDividor != 1) {
                        /* 31->shift ALL , 32 ->32bit register */
                        ImmOperand &rightShiftAll = CreateImmOperand(31, k32BitSize, isSigned);
                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wasrrri5, temp, opnd0, rightShiftAll));

                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wlsrrri5, temp, temp, rightShiftValue));
                    } else {
                        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wlsrrri5, temp, opnd0, rightShiftValue));
                    }

                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_waddrrr, resOpnd, opnd0, temp));
                    ImmOperand &remBits = CreateImmOperand(dividor - 1, k32BitSize, isSigned);
                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wandrri12, resOpnd, resOpnd, remBits));

                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wsubrrr, resOpnd, resOpnd, temp));
                    return;
                } else if (imm && imm->GetValue() > 0) {
                    ImmOperand &remBits = CreateImmOperand(dividor - 1, k32BitSize, isSigned);
                    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wandrri12, resOpnd, opnd0, remBits));
                    return;
                }
            }
        }
    }

    uint32 mopDiv = is64Bits ? (isSigned ? MOP_xsdivrrr : MOP_xudivrrr) : (isSigned ? MOP_wsdivrrr : MOP_wudivrrr);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopDiv, temp, opnd0, opnd1));

    uint32 mopSub = is64Bits ? MOP_xmsubrrrr : MOP_wmsubrrrr;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopSub, resOpnd, temp, opnd1, opnd0));
}

Operand *AArch64CGFunc::SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    DEBUG_ASSERT(IsPrimitiveInteger(dtype), "wrong type for rem");
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);

    /* promoted type */
    PrimType primType = ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
    RegOperand &resOpnd = GetOrCreateResOperand(parent, primType);
    SelectRem(resOpnd, opnd0, opnd1, primType, isSigned, is64Bits);
    return &resOpnd;
}

void AArch64CGFunc::SelectCmpOp(Operand &resOpnd, Operand &lhsOpnd, Operand &rhsOpnd, Opcode opcode, PrimType primType,
                                const BaseNode &parent)
{
    uint32 dsize = resOpnd.GetSize();
    bool isFloat = IsPrimitiveFloat(primType);
    Operand &opnd0 = LoadIntoRegister(lhsOpnd, primType);

    /*
     * most of FP constants are passed as MemOperand
     * except 0.0 which is passed as kOpdFPImmediate
     */
    Operand::OperandType opnd1Type = rhsOpnd.GetKind();
    Operand *opnd1 = &rhsOpnd;
    if ((opnd1Type != Operand::kOpdImmediate) && (opnd1Type != Operand::kOpdFPImmediate) &&
        (opnd1Type != Operand::kOpdOffset)) {
        opnd1 = &LoadIntoRegister(rhsOpnd, primType);
    }

    bool unsignedIntegerComparison = !isFloat && !IsSignedInteger(primType);
    /*
     * OP_cmp, OP_cmpl, OP_cmpg
     * <cmp> OP0, OP1  ; fcmp for OP_cmpl/OP_cmpg, cmp/fcmpe for OP_cmp
     * CSINV RES, WZR, WZR, GE
     * CSINC RES, RES, WZR, LE
     * if OP_cmpl, CSINV RES, RES, WZR, VC (no overflow)
     * if OP_cmpg, CSINC RES, RES, WZR, VC (no overflow)
     */
    RegOperand &xzr = GetZeroOpnd(dsize);
    if (opcode == OP_cmp) {
        SelectAArch64Cmp(opnd0, *opnd1, !isFloat, GetPrimTypeBitSize(primType));
        if (unsignedIntegerComparison) {
            SelectAArch64CSINV(resOpnd, xzr, xzr, GetCondOperand(CC_HS), (dsize == k64BitSize));
            SelectAArch64CSINC(resOpnd, resOpnd, xzr, GetCondOperand(CC_LS), (dsize == k64BitSize));
        } else {
            SelectAArch64CSINV(resOpnd, xzr, xzr, GetCondOperand(CC_GE), (dsize == k64BitSize));
            SelectAArch64CSINC(resOpnd, resOpnd, xzr, GetCondOperand(CC_LE), (dsize == k64BitSize));
        }
        return;
    }

    // lt u8 i32 ( xxx, 0 ) => get sign bit
    if ((opcode == OP_lt) && opnd0.IsRegister() && opnd1->IsImmediate() &&
        (static_cast<ImmOperand *>(opnd1)->GetValue() == 0) && !isFloat) {
        bool is64Bits = (opnd0.GetSize() == k64BitSize);
        if (!unsignedIntegerComparison) {
            int32 bitLen = is64Bits ? kBitLenOfShift64Bits : kBitLenOfShift32Bits;
            ImmOperand &shiftNum = CreateImmOperand(is64Bits ? kHighestBitOf64Bits : kHighestBitOf32Bits,
                                                    static_cast<uint32>(bitLen), false);
            MOperator mOpCode = is64Bits ? MOP_xlsrrri6 : MOP_wlsrrri5;
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, opnd0, shiftNum));
            return;
        }
        ImmOperand &constNum = CreateImmOperand(0, is64Bits ? k64BitSize : k32BitSize, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(is64Bits ? MOP_xmovri64 : MOP_wmovri32, resOpnd, constNum));
        return;
    }
    SelectAArch64Cmp(opnd0, *opnd1, !isFloat, GetPrimTypeBitSize(primType));

    ConditionCode cc = CC_EQ;
    // need to handle unordered situation here.
    switch (opcode) {
        case OP_eq:
            cc = CC_EQ;
            break;
        case OP_ne:
            cc = isFloat ? CC_MI : CC_NE;
            break;
        case OP_le:
            cc = isFloat ? CC_LS : unsignedIntegerComparison ? CC_LS : CC_LE;
            break;
        case OP_ge:
            cc = unsignedIntegerComparison ? CC_HS : CC_GE;
            break;
        case OP_gt:
            cc = unsignedIntegerComparison ? CC_HI : CC_GT;
            break;
        case OP_lt:
            cc = isFloat ? CC_MI : unsignedIntegerComparison ? CC_LO : CC_LT;
            break;
        default:
            CHECK_FATAL(false, "illegal logical operator");
    }
    SelectAArch64CSet(resOpnd, GetCondOperand(cc), (dsize == k64BitSize));
    if (isFloat && opcode == OP_ne) {
        SelectAArch64CSINC(resOpnd, resOpnd, xzr, GetCondOperand(CC_LE), (dsize == k64BitSize));
    }
}

Operand *AArch64CGFunc::SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    RegOperand *resOpnd = &GetOrCreateResOperand(parent, node.GetPrimType());
    SelectCmpOp(*resOpnd, opnd0, opnd1, node.GetOpCode(), node.GetOpndType(), parent);
    return resOpnd;
}

void AArch64CGFunc::SelectTargetFPCmpQuiet(Operand &o0, Operand &o1, uint32 dsize)
{
    MOperator mOpCode = 0;
    if (o1.GetKind() == Operand::kOpdFPImmediate) {
        CHECK_FATAL(static_cast<ImmOperand &>(o0).GetValue() == 0, "NIY");
        mOpCode = (dsize == k64BitSize) ? MOP_dcmpqri : (dsize == k32BitSize) ? MOP_scmpqri : MOP_hcmpqri;
    } else if (o1.GetKind() == Operand::kOpdRegister) {
        mOpCode = (dsize == k64BitSize) ? MOP_dcmpqrr : (dsize == k32BitSize) ? MOP_scmpqrr : MOP_hcmpqrr;
    } else {
        CHECK_FATAL(false, "unsupported operand type");
    }
    Operand &rflag = GetOrCreateRflag();
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, rflag, o0, o1));
}

void AArch64CGFunc::SelectAArch64Cmp(Operand &o0, Operand &o1, bool isIntType, uint32 dsize)
{
    MOperator mOpCode = 0;
    Operand *newO1 = &o1;
    if (isIntType) {
        if ((o1.GetKind() == Operand::kOpdImmediate) || (o1.GetKind() == Operand::kOpdOffset)) {
            ImmOperand *immOpnd = static_cast<ImmOperand *>(&o1);
            /*
             * imm : 0 ~ 4095, shift: none, LSL #0, or LSL #12
             * aarch64 assembly takes up to 24-bits, if the lower 12 bits is all 0
             */
            if (immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits)) {
                mOpCode = (dsize == k64BitSize) ? MOP_xcmpri : MOP_wcmpri;
            } else {
                /* load into register */
                PrimType ptype = (dsize == k64BitSize) ? PTY_i64 : PTY_i32;
                newO1 = &SelectCopy(o1, ptype, ptype);
                mOpCode = (dsize == k64BitSize) ? MOP_xcmprr : MOP_wcmprr;
            }
        } else if (o1.GetKind() == Operand::kOpdRegister) {
            mOpCode = (dsize == k64BitSize) ? MOP_xcmprr : MOP_wcmprr;
        } else {
            CHECK_FATAL(false, "unsupported operand type");
        }
    } else { /* float */
        if (o1.GetKind() == Operand::kOpdFPImmediate) {
            CHECK_FATAL(static_cast<ImmOperand &>(o1).GetValue() == 0, "NIY");
            mOpCode = (dsize == k64BitSize) ? MOP_dcmperi : ((dsize == k32BitSize) ? MOP_scmperi : MOP_hcmperi);
        } else if (o1.GetKind() == Operand::kOpdRegister) {
            mOpCode = (dsize == k64BitSize) ? MOP_dcmperr : ((dsize == k32BitSize) ? MOP_scmperr : MOP_hcmperr);
        } else {
            CHECK_FATAL(false, "unsupported operand type");
        }
    }
    DEBUG_ASSERT(mOpCode != 0, "mOpCode undefined");
    Operand &rflag = GetOrCreateRflag();
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, rflag, o0, *newO1));
}

void AArch64CGFunc::SelectAArch64CSet(Operand &r, CondOperand &cond, bool is64Bits)
{
    MOperator mOpCode = is64Bits ? MOP_xcsetrc : MOP_wcsetrc;
    Operand &rflag = GetOrCreateRflag();
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, r, cond, rflag));
}

void AArch64CGFunc::SelectAArch64CSINV(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits)
{
    MOperator mOpCode = is64Bits ? MOP_xcsinvrrrc : MOP_wcsinvrrrc;
    Operand &rflag = GetOrCreateRflag();
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, res, o0, o1, cond, rflag));
}

void AArch64CGFunc::SelectAArch64CSINC(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits)
{
    MOperator mOpCode = is64Bits ? MOP_xcsincrrrc : MOP_wcsincrrrc;
    Operand &rflag = GetOrCreateRflag();
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, res, o0, o1, cond, rflag));
}

Operand *AArch64CGFunc::SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    return SelectRelationOperator(kAND, node, opnd0, opnd1, parent);
}

void AArch64CGFunc::SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectRelationOperator(kAND, resOpnd, opnd0, opnd1, primType);
}

Operand *AArch64CGFunc::SelectRelationOperator(RelationOperator operatorCode, const BinaryNode &node, Operand &opnd0,
                                               Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    PrimType primType =
        is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32); /* promoted type */
    RegOperand *resOpnd = &GetOrCreateResOperand(parent, primType);
    SelectRelationOperator(operatorCode, *resOpnd, opnd0, opnd1, primType);
    return resOpnd;
}

MOperator AArch64CGFunc::SelectRelationMop(RelationOperator operatorCode, RelationOperatorOpndPattern opndPattern,
                                           bool is64Bits, bool isBitmaskImmediate, bool isBitNumLessThan16) const
{
    MOperator mOp = MOP_undef;
    if (opndPattern == kRegReg) {
        switch (operatorCode) {
            case kAND:
                mOp = is64Bits ? MOP_xandrrr : MOP_wandrrr;
                break;
            case kIOR:
                mOp = is64Bits ? MOP_xiorrrr : MOP_wiorrrr;
                break;
            case kEOR:
                mOp = is64Bits ? MOP_xeorrrr : MOP_weorrrr;
                break;
            default:
                break;
        }
        return mOp;
    }
    /* opndPattern == KRegImm */
    if (isBitmaskImmediate) {
        switch (operatorCode) {
            case kAND:
                mOp = is64Bits ? MOP_xandrri13 : MOP_wandrri12;
                break;
            case kIOR:
                mOp = is64Bits ? MOP_xiorrri13 : MOP_wiorrri12;
                break;
            case kEOR:
                mOp = is64Bits ? MOP_xeorrri13 : MOP_weorrri12;
                break;
            default:
                break;
        }
        return mOp;
    }
    /* normal imm value */
    if (isBitNumLessThan16) {
        switch (operatorCode) {
            case kAND:
                mOp = is64Bits ? MOP_xandrrrs : MOP_wandrrrs;
                break;
            case kIOR:
                mOp = is64Bits ? MOP_xiorrrrs : MOP_wiorrrrs;
                break;
            case kEOR:
                mOp = is64Bits ? MOP_xeorrrrs : MOP_weorrrrs;
                break;
            default:
                break;
        }
        return mOp;
    }
    return mOp;
}

void AArch64CGFunc::SelectRelationOperator(RelationOperator operatorCode, Operand &resOpnd, Operand &opnd0,
                                           Operand &opnd1, PrimType primType)
{
    Operand::OperandType opnd0Type = opnd0.GetKind();
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    /* op #imm. #imm */
    if ((opnd0Type != Operand::kOpdRegister) && (opnd1Type != Operand::kOpdRegister)) {
        SelectRelationOperator(operatorCode, resOpnd, SelectCopy(opnd0, primType, primType), opnd1, primType);
        return;
    }
    /* op #imm, reg -> op reg, #imm */
    if ((opnd0Type != Operand::kOpdRegister) && (opnd1Type == Operand::kOpdRegister)) {
        SelectRelationOperator(operatorCode, resOpnd, opnd1, opnd0, primType);
        return;
    }
    /* op reg, reg */
    if ((opnd0Type == Operand::kOpdRegister) && (opnd1Type == Operand::kOpdRegister)) {
        DEBUG_ASSERT(IsPrimitiveInteger(primType), "NYI band");
        MOperator mOp = SelectRelationMop(operatorCode, kRegReg, is64Bits, false, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, opnd1));
        return;
    }
    /* op reg, #imm */
    if ((opnd0Type == Operand::kOpdRegister) && (opnd1Type != Operand::kOpdRegister)) {
        if (!((opnd1Type == Operand::kOpdImmediate) || (opnd1Type == Operand::kOpdOffset))) {
            SelectRelationOperator(operatorCode, resOpnd, opnd0, SelectCopy(opnd1, primType, primType), primType);
            return;
        }

        ImmOperand *immOpnd = static_cast<ImmOperand *>(&opnd1);
        if (immOpnd->IsZero()) {
            if (operatorCode == kAND) {
                uint32 mopMv = is64Bits ? MOP_xmovrr : MOP_wmovrr;
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopMv, resOpnd, GetZeroOpnd(dsize)));
            } else if ((operatorCode == kIOR) || (operatorCode == kEOR)) {
                SelectCopy(resOpnd, primType, opnd0, primType);
            }
        } else if ((immOpnd->IsAllOnes()) || (!is64Bits && immOpnd->IsAllOnes32bit())) {
            if (operatorCode == kAND) {
                SelectCopy(resOpnd, primType, opnd0, primType);
            } else if (operatorCode == kIOR) {
                uint32 mopMovn = is64Bits ? MOP_xmovnri16 : MOP_wmovnri16;
                ImmOperand &src16 = CreateImmOperand(0, k16BitSize, false);
                BitShiftOperand *lslOpnd = GetLogicalShiftLeftOperand(0, is64Bits);
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopMovn, resOpnd, src16, *lslOpnd));
            } else if (operatorCode == kEOR) {
                SelectMvn(resOpnd, opnd0, primType);
            }
        } else if (immOpnd->IsBitmaskImmediate()) {
            MOperator mOp = SelectRelationMop(operatorCode, kRegImm, is64Bits, true, false);
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, opnd1));
        } else {
            int64 immVal = immOpnd->GetValue();
            int32 tail0BitNum = GetTail0BitNum(immVal);
            int32 head0BitNum = GetHead0BitNum(immVal);
            const int32 bitNum = (k64BitSizeInt - head0BitNum) - tail0BitNum;
            RegOperand &regOpnd = CreateRegisterOperandOfType(primType);

            if (bitNum <= k16ValidBit) {
                int64 newImm = (static_cast<uint64>(immVal) >> static_cast<uint32>(tail0BitNum)) & 0xFFFF;
                ImmOperand &immOpnd1 = CreateImmOperand(newImm, k32BitSize, false);
                SelectCopyImm(regOpnd, immOpnd1, primType);
                MOperator mOp = SelectRelationMop(operatorCode, kRegImm, is64Bits, false, true);
                int32 bitLen = is64Bits ? kBitLenOfShift64Bits : kBitLenOfShift32Bits;
                BitShiftOperand &shiftOpnd =
                    CreateBitShiftOperand(BitShiftOperand::kLSL, static_cast<uint32>(tail0BitNum), bitLen);
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, regOpnd, shiftOpnd));
            } else {
                SelectCopyImm(regOpnd, *immOpnd, primType);
                MOperator mOp = SelectRelationMop(operatorCode, kRegReg, is64Bits, false, false);
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, regOpnd));
            }
        }
    }
}

Operand *AArch64CGFunc::SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    return SelectRelationOperator(kIOR, node, opnd0, opnd1, parent);
}

void AArch64CGFunc::SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectRelationOperator(kIOR, resOpnd, opnd0, opnd1, primType);
}

Operand *AArch64CGFunc::SelectMinOrMax(bool isMin, const BinaryNode &node, Operand &opnd0, Operand &opnd1,
                                       const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(dtype);
    /* promoted type */
    PrimType primType = isFloat ? dtype : (is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32));
    RegOperand &resOpnd = GetOrCreateResOperand(parent, primType);
    SelectMinOrMax(isMin, resOpnd, opnd0, opnd1, primType);
    return &resOpnd;
}

void AArch64CGFunc::SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    if (IsPrimitiveInteger(primType)) {
        RegOperand &regOpnd0 = LoadIntoRegister(opnd0, primType);
        Operand &regOpnd1 = LoadIntoRegister(opnd1, primType);
        SelectAArch64Cmp(regOpnd0, regOpnd1, true, dsize);
        Operand &newResOpnd = LoadIntoRegister(resOpnd, primType);
        if (isMin) {
            CondOperand &cc = IsSignedInteger(primType) ? GetCondOperand(CC_LT) : GetCondOperand(CC_LO);
            SelectAArch64Select(newResOpnd, regOpnd0, regOpnd1, cc, true, dsize);
        } else {
            CondOperand &cc = IsSignedInteger(primType) ? GetCondOperand(CC_GT) : GetCondOperand(CC_HI);
            SelectAArch64Select(newResOpnd, regOpnd0, regOpnd1, cc, true, dsize);
        }
    } else if (IsPrimitiveFloat(primType)) {
        RegOperand &regOpnd0 = LoadIntoRegister(opnd0, primType);
        RegOperand &regOpnd1 = LoadIntoRegister(opnd1, primType);
        SelectFMinFMax(resOpnd, regOpnd0, regOpnd1, is64Bits, isMin);
    } else {
        CHECK_FATAL(false, "NIY type max or min");
    }
}

Operand *AArch64CGFunc::SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    return SelectMinOrMax(true, node, opnd0, opnd1, parent);
}

void AArch64CGFunc::SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectMinOrMax(true, resOpnd, opnd0, opnd1, primType);
}

Operand *AArch64CGFunc::SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    return SelectMinOrMax(false, node, opnd0, opnd1, parent);
}

void AArch64CGFunc::SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectMinOrMax(false, resOpnd, opnd0, opnd1, primType);
}

void AArch64CGFunc::SelectFMinFMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, bool is64Bits, bool isMin)
{
    uint32 mOpCode = isMin ? (is64Bits ? MOP_xfminrrr : MOP_wfminrrr) : (is64Bits ? MOP_xfmaxrrr : MOP_wfmaxrrr);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, opnd0, opnd1));
}

Operand *AArch64CGFunc::SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    return SelectRelationOperator(kEOR, node, opnd0, opnd1, parent);
}

void AArch64CGFunc::SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectRelationOperator(kEOR, resOpnd, opnd0, opnd1, primType);
}

Operand *AArch64CGFunc::SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    bool isFloat = IsPrimitiveFloat(dtype);
    RegOperand *resOpnd = nullptr;
    Opcode opcode = node.GetOpCode();

    PrimType primType =
        isFloat ? dtype : (is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32));
    resOpnd = &GetOrCreateResOperand(parent, primType);
    ShiftDirection direct = (opcode == OP_lshr) ? kShiftLright : ((opcode == OP_ashr) ? kShiftAright : kShiftLeft);
    SelectShift(*resOpnd, opnd0, opnd1, direct, primType);

    if (dtype == PTY_i16) {
        MOperator exOp = is64Bits ? MOP_xsxth64 : MOP_xsxth32;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(exOp, *resOpnd, *resOpnd));
    } else if (dtype == PTY_i8) {
        MOperator exOp = is64Bits ? MOP_xsxtb64 : MOP_xsxtb32;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(exOp, *resOpnd, *resOpnd));
    }
    return resOpnd;
}

void AArch64CGFunc::SelectBxorShift(Operand &resOpnd, Operand *opnd0, Operand *opnd1, Operand &opnd2, PrimType primType)
{
    opnd0 = &LoadIntoRegister(*opnd0, primType);
    opnd1 = &LoadIntoRegister(*opnd1, primType);
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    MOperator mopBxor = is64Bits ? MOP_xeorrrrs : MOP_weorrrrs;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBxor, resOpnd, *opnd0, *opnd1, opnd2));
}

void AArch64CGFunc::SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, ShiftDirection direct,
                                PrimType primType)
{
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    Operand *firstOpnd = &LoadIntoRegister(opnd0, primType);

    MOperator mopShift;
    if ((opnd1Type == Operand::kOpdImmediate) || (opnd1Type == Operand::kOpdOffset)) {
        ImmOperand *immOpnd1 = static_cast<ImmOperand *>(&opnd1);
        const int64 kVal = immOpnd1->GetValue();
        const uint32 kShiftamt = is64Bits ? kHighestBitOf64Bits : kHighestBitOf32Bits;
        if (kVal == 0) {
            SelectCopy(resOpnd, primType, *firstOpnd, primType);
            return;
        }
        /* e.g. a >> -1 */
        if ((kVal < 0) || (kVal > kShiftamt)) {
            SelectShift(resOpnd, *firstOpnd, SelectCopy(opnd1, primType, primType), direct, primType);
            return;
        }
        switch (direct) {
            case kShiftLeft:
                mopShift = is64Bits ? MOP_xlslrri6 : MOP_wlslrri5;
                break;
            case kShiftAright:
                mopShift = is64Bits ? MOP_xasrrri6 : MOP_wasrrri5;
                break;
            case kShiftLright:
                mopShift = is64Bits ? MOP_xlsrrri6 : MOP_wlsrrri5;
                break;
        }
    } else if (opnd1Type != Operand::kOpdRegister) {
        SelectShift(resOpnd, *firstOpnd, SelectCopy(opnd1, primType, primType), direct, primType);
        return;
    } else {
        switch (direct) {
            case kShiftLeft:
                mopShift = is64Bits ? MOP_xlslrrr : MOP_wlslrrr;
                break;
            case kShiftAright:
                mopShift = is64Bits ? MOP_xasrrrr : MOP_wasrrrr;
                break;
            case kShiftLright:
                mopShift = is64Bits ? MOP_xlsrrrr : MOP_wlsrrrr;
                break;
        }
    }

    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopShift, resOpnd, *firstOpnd, opnd1));
}

Operand *AArch64CGFunc::SelectAbsSub(Insn &lastInsn, const UnaryNode &node, Operand &newOpnd0)
{
    PrimType dtyp = node.GetPrimType();
    bool is64Bits = (GetPrimTypeBitSize(dtyp) == k64BitSize);
    /* promoted type */
    PrimType primType = is64Bits ? (PTY_i64) : (PTY_i32);
    RegOperand &resOpnd = CreateRegisterOperandOfType(primType);
    uint32 mopCsneg = is64Bits ? MOP_xcnegrrrc : MOP_wcnegrrrc;
    /* ABS requires the operand be interpreted as a signed integer */
    CondOperand &condOpnd = GetCondOperand(CC_MI);
    MOperator newMop = AArch64isa::GetMopSub2Subs(lastInsn);
    Operand &rflag = GetOrCreateRflag();
    std::vector<Operand *> opndVec;
    opndVec.push_back(&rflag);
    for (uint32 i = 0; i < lastInsn.GetOperandSize(); i++) {
        opndVec.push_back(&lastInsn.GetOperand(i));
    }
    Insn *subsInsn = &GetInsnBuilder()->BuildInsn(newMop, opndVec);
    GetCurBB()->ReplaceInsn(lastInsn, *subsInsn);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopCsneg, resOpnd, newOpnd0, condOpnd, rflag));
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectAbs(UnaryNode &node, Operand &opnd0)
{
    PrimType dtyp = node.GetPrimType();
    if (IsPrimitiveFloat(dtyp)) {
        CHECK_FATAL(GetPrimTypeBitSize(dtyp) >= k32BitSize, "We don't support hanf-word FP operands yet");
        bool is64Bits = (GetPrimTypeBitSize(dtyp) == k64BitSize);
        Operand &newOpnd0 = LoadIntoRegister(opnd0, dtyp);
        RegOperand &resOpnd = CreateRegisterOperandOfType(dtyp);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(is64Bits ? MOP_dabsrr : MOP_sabsrr, resOpnd, newOpnd0));
        return &resOpnd;
    } else {
        bool is64Bits = (GetPrimTypeBitSize(dtyp) == k64BitSize);
        /* promoted type */
        PrimType primType = is64Bits ? (PTY_i64) : (PTY_i32);
        Operand &newOpnd0 = LoadIntoRegister(opnd0, primType);
        Insn *lastInsn = GetCurBB()->GetLastMachineInsn();
        if (lastInsn != nullptr && AArch64isa::IsSub(*lastInsn)) {
            Operand &dest = lastInsn->GetOperand(kInsnFirstOpnd);
            Operand &opd1 = lastInsn->GetOperand(kInsnSecondOpnd);
            Operand &opd2 = lastInsn->GetOperand(kInsnThirdOpnd);
            regno_t absReg = static_cast<RegOperand &>(newOpnd0).GetRegisterNumber();
            if ((dest.IsRegister() && static_cast<RegOperand &>(dest).GetRegisterNumber() == absReg) ||
                (opd1.IsRegister() && static_cast<RegOperand &>(opd1).GetRegisterNumber() == absReg) ||
                (opd2.IsRegister() && static_cast<RegOperand &>(opd2).GetRegisterNumber() == absReg)) {
                return SelectAbsSub(*lastInsn, node, newOpnd0);
            }
        }
        RegOperand &resOpnd = CreateRegisterOperandOfType(primType);
        SelectAArch64Cmp(newOpnd0, CreateImmOperand(0, is64Bits ? PTY_u64 : PTY_u32, false), true,
                         GetPrimTypeBitSize(dtyp));
        uint32 mopCsneg = is64Bits ? MOP_xcsnegrrrc : MOP_wcsnegrrrc;
        /* ABS requires the operand be interpreted as a signed integer */
        CondOperand &condOpnd = GetCondOperand(CC_GE);
        Operand &rflag = GetOrCreateRflag();
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopCsneg, resOpnd, newOpnd0, newOpnd0, condOpnd, rflag));
        return &resOpnd;
    }
}

Operand *AArch64CGFunc::SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    DEBUG_ASSERT(IsPrimitiveInteger(dtype), "bnot expect integer or NYI");
    uint32 bitSize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (bitSize == k64BitSize);
    bool isSigned = IsSignedInteger(dtype);
    RegOperand *resOpnd = nullptr;
    PrimType primType = is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32);
    resOpnd = &GetOrCreateResOperand(parent, primType);

    Operand &newOpnd0 = LoadIntoRegister(opnd0, primType);

    uint32 mopBnot = is64Bits ? MOP_xnotrr : MOP_wnotrr;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBnot, *resOpnd, newOpnd0));
    /* generate and resOpnd, resOpnd, 0x1/0xFF/0xFFFF for PTY_u1/PTY_u8/PTY_u16 */
    int64 immValue = 0;
    if (bitSize == k1BitSize) {
        immValue = 1;
    } else if (bitSize == k8BitSize) {
        immValue = 0xFF;
    } else if (bitSize == k16BitSize) {
        immValue = 0xFFFF;
    }
    if (immValue != 0) {
        ImmOperand &imm = CreateImmOperand(PTY_u32, immValue);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wandrri12, *resOpnd, *resOpnd, imm));
    }
    return resOpnd;
}

Operand *AArch64CGFunc::SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint8 bitOffset = node.GetBitsOffset();
    uint8 bitSize = node.GetBitsSize();
    bool is64Bits = (GetPrimTypeBitSize(dtype) == k64BitSize);
    CHECK_FATAL(!is64Bits, "dest opnd should not be 64bit");
    PrimType destType = GetIntegerPrimTypeBySizeAndSign(bitSize, isSigned);
    Operand *result =
        SelectIread(parent, *static_cast<IreadNode *>(node.Opnd(0)), static_cast<int>(bitOffset / k8BitSize), destType);
    return result;
}

Operand *AArch64CGFunc::SelectExtractbits(ExtractbitsNode &node, Operand &srcOpnd, const BaseNode &parent)
{
    uint8 bitOffset = node.GetBitsOffset();
    uint8 bitSize = node.GetBitsSize();
    PrimType dtype = node.GetPrimType();
    RegOperand &resOpnd = GetOrCreateResOperand(parent, dtype);
    bool isSigned =
        (node.GetOpCode() == OP_sext) ? true : (node.GetOpCode() == OP_zext) ? false : IsSignedInteger(dtype);
    bool is64Bits = (GetPrimTypeBitSize(dtype) == k64BitSize);
    uint32 immWidth = is64Bits ? kMaxImmVal13Bits : kMaxImmVal12Bits;
    Operand &opnd0 = LoadIntoRegister(srcOpnd, dtype);
    if (bitOffset == 0) {
        if (!isSigned && (bitSize < immWidth)) {
            SelectBand(resOpnd, opnd0,
                       CreateImmOperand(static_cast<int64>((static_cast<uint64>(1) << bitSize) - 1), immWidth, false),
                       dtype);
            return &resOpnd;
        } else {
            MOperator mOp = MOP_undef;
            if (bitSize == k8BitSize) {
                mOp = is64Bits ? (isSigned ? MOP_xsxtb64 : MOP_undef)
                               : (isSigned ? MOP_xsxtb32 : (opnd0.GetSize() == k32BitSize ? MOP_xuxtb32 : MOP_undef));
            } else if (bitSize == k16BitSize) {
                mOp = is64Bits ? (isSigned ? MOP_xsxth64 : MOP_undef)
                               : (isSigned ? MOP_xsxth32 : (opnd0.GetSize() == k32BitSize ? MOP_xuxth32 : MOP_undef));
            } else if (bitSize == k32BitSize) {
                mOp = is64Bits ? (isSigned ? MOP_xsxtw64 : MOP_xuxtw64) : MOP_wmovrr;
            }
            if (mOp != MOP_undef) {
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0));
                return &resOpnd;
            }
        }
    }
    uint32 mopBfx =
        is64Bits ? (isSigned ? MOP_xsbfxrri6i6 : MOP_xubfxrri6i6) : (isSigned ? MOP_wsbfxrri5i5 : MOP_wubfxrri5i5);
    ImmOperand &immOpnd1 = CreateImmOperand(bitOffset, k8BitSize, false);
    ImmOperand &immOpnd2 = CreateImmOperand(bitSize, k8BitSize, false);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBfx, resOpnd, opnd0, immOpnd1, immOpnd2));
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectLnot(UnaryNode &node, Operand &srcOpnd, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    RegOperand &resOpnd = GetOrCreateResOperand(parent, dtype);
    bool is64Bits = (GetPrimTypeBitSize(dtype) == k64BitSize);
    Operand &opnd0 = LoadIntoRegister(srcOpnd, dtype);
    SelectAArch64Cmp(opnd0, CreateImmOperand(0, is64Bits ? PTY_u64 : PTY_u32, false), true, GetPrimTypeBitSize(dtype));
    SelectAArch64CSet(resOpnd, GetCondOperand(CC_EQ), is64Bits);
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool is64Bits = (GetPrimTypeBitSize(dtype) == k64BitSize);
    RegOperand *resOpnd = nullptr;
    PrimType primType;
    if (IsPrimitiveFloat(dtype)) {
        primType = dtype;
    } else {
        primType = is64Bits ? (PTY_i64) : (PTY_i32); /* promoted type */
    }
    resOpnd = &GetOrCreateResOperand(parent, primType);
    SelectNeg(*resOpnd, opnd0, primType);
    return resOpnd;
}

void AArch64CGFunc::SelectNeg(Operand &dest, Operand &srcOpnd, PrimType primType)
{
    Operand &opnd0 = LoadIntoRegister(srcOpnd, primType);
    bool is64Bits = (GetPrimTypeBitSize(primType) == k64BitSize);
    MOperator mOp;
    if (IsPrimitiveFloat(primType)) {
        mOp = is64Bits ? MOP_xfnegrr : MOP_wfnegrr;
    } else {
        mOp = is64Bits ? MOP_xinegrr : MOP_winegrr;
    }
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, dest, opnd0));
}

void AArch64CGFunc::SelectMvn(Operand &dest, Operand &src, PrimType primType)
{
    Operand &opnd0 = LoadIntoRegister(src, primType);
    bool is64Bits = (GetPrimTypeBitSize(primType) == k64BitSize);
    MOperator mOp;
    DEBUG_ASSERT(!IsPrimitiveFloat(primType), "Instruction 'mvn' do not have float version.");
    mOp = is64Bits ? MOP_xnotrr : MOP_wnotrr;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, dest, opnd0));
}

Operand *AArch64CGFunc::SelectSqrt(UnaryNode &node, Operand &src, const BaseNode &parent)
{
    /*
     * gcc generates code like below for better accurate
     * fsqrts  s15, s0
     * fcmps s15, s15
     * fmstat
     * beq .L4
     * push  {r3, lr}
     * bl  sqrtf
     * pop {r3, pc}
     * .L4:
     * fcpys s0, s15
     * bx  lr
     */
    PrimType dtype = node.GetPrimType();
    if (!IsPrimitiveFloat(dtype)) {
        DEBUG_ASSERT(false, "should be float type");
        return nullptr;
    }
    bool is64Bits = (GetPrimTypeBitSize(dtype) == k64BitSize);
    Operand &opnd0 = LoadIntoRegister(src, dtype);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, dtype);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(is64Bits ? MOP_vsqrtd : MOP_vsqrts, resOpnd, opnd0));
    return &resOpnd;
}

void AArch64CGFunc::SelectCvtFloat2Int(Operand &resOpnd, Operand &srcOpnd, PrimType itype, PrimType ftype)
{
    bool is64BitsFloat = (ftype == PTY_f64);
    MOperator mOp = 0;

    DEBUG_ASSERT(((ftype == PTY_f64) || (ftype == PTY_f32)), "wrong from type");
    Operand &opnd0 = LoadIntoRegister(srcOpnd, ftype);
    switch (itype) {
        case PTY_i32:
            mOp = !is64BitsFloat ? MOP_vcvtrf : MOP_vcvtrd;
            break;
        case PTY_u32:
            mOp = !is64BitsFloat ? MOP_vcvturf : MOP_vcvturd;
            break;
        case PTY_i64:
            mOp = !is64BitsFloat ? MOP_xvcvtrf : MOP_xvcvtrd;
            break;
        case PTY_u64:
        case PTY_a64:
            mOp = !is64BitsFloat ? MOP_xvcvturf : MOP_xvcvturd;
            break;
        default:
            CHECK_FATAL(false, "unexpected type");
    }
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0));
}

void AArch64CGFunc::SelectCvtInt2Float(Operand &resOpnd, Operand &origOpnd0, PrimType toType, PrimType fromType)
{
    DEBUG_ASSERT((toType == PTY_f32) || (toType == PTY_f64), "unexpected type");
    bool is64BitsFloat = (toType == PTY_f64);
    MOperator mOp = 0;
    uint32 fsize = GetPrimTypeBitSize(fromType);

    PrimType itype = (GetPrimTypeBitSize(fromType) == k64BitSize) ? (IsSignedInteger(fromType) ? PTY_i64 : PTY_u64)
                                                                  : (IsSignedInteger(fromType) ? PTY_i32 : PTY_u32);

    Operand *opnd0 = &LoadIntoRegister(origOpnd0, itype);

    /* need extension before cvt */
    DEBUG_ASSERT(opnd0->IsRegister(), "opnd should be a register operand");
    Operand *srcOpnd = opnd0;
    if (IsSignedInteger(fromType) && (fsize < k32BitSize)) {
        srcOpnd = &CreateRegisterOperandOfType(itype);
        mOp = (fsize == k8BitSize) ? MOP_xsxtb32 : MOP_xsxth32;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *srcOpnd, *opnd0));
    }

    switch (itype) {
        case PTY_i32:
            mOp = !is64BitsFloat ? MOP_vcvtfr : MOP_vcvtdr;
            break;
        case PTY_u32:
            mOp = !is64BitsFloat ? MOP_vcvtufr : MOP_vcvtudr;
            break;
        case PTY_i64:
            mOp = !is64BitsFloat ? MOP_xvcvtfr : MOP_xvcvtdr;
            break;
        case PTY_u64:
            mOp = !is64BitsFloat ? MOP_xvcvtufr : MOP_xvcvtudr;
            break;
        default:
            CHECK_FATAL(false, "unexpected type");
    }
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, *srcOpnd));
}

Operand *AArch64CGFunc::SelectRoundOperator(RoundType roundType, const TypeCvtNode &node, Operand &opnd0,
                                            const BaseNode &parent)
{
    PrimType itype = node.GetPrimType();
    PrimType ftype = node.FromType();
    DEBUG_ASSERT(((ftype == PTY_f64) || (ftype == PTY_f32)), "wrong float type");
    bool is64Bits = (ftype == PTY_f64);
    bool isFloat = (ftype == PTY_f64) || (ftype == PTY_f32);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, itype);
    RegOperand &regOpnd0 = LoadIntoRegister(opnd0, ftype);
    MOperator mop = MOP_undef;
    if (roundType == kCeil) {
        if (isFloat) {
            mop = is64Bits ? MOP_dfrintprr : MOP_sfrintprr;
        } else {
            mop = is64Bits ? MOP_xvcvtps : MOP_vcvtps;
        }
    } else if (roundType == kFloor) {
        if (isFloat) {
            mop = is64Bits ? MOP_dfrintmrr : MOP_sfrintmrr;
        } else {
            mop = is64Bits ? MOP_xvcvtms : MOP_vcvtms;
        }
    } else if (roundType == kTrunc) {
        if (isFloat) {
            mop = is64Bits ? MOP_dfrintzrr : MOP_sfrintzrr;
        } else {
            CHECK_FATAL(false, "not support here!");
        }
    } else {
        CHECK_FATAL(!isFloat, "not support float here!");
        mop = is64Bits ? MOP_xvcvtas : MOP_vcvtas;
    }
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, resOpnd, regOpnd0));
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    return SelectRoundOperator(kCeil, node, opnd0, parent);
}

/* float to int floor */
Operand *AArch64CGFunc::SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    return SelectRoundOperator(kFloor, node, opnd0, parent);
}

static bool LIsPrimitivePointer(PrimType ptype)
{
    return ((ptype >= PTY_ptr) && (ptype <= PTY_a64));
}

Operand *AArch64CGFunc::SelectRetype(TypeCvtNode &node, Operand &opnd0)
{
    PrimType fromType = node.Opnd(0)->GetPrimType();
    PrimType toType = node.GetPrimType();
    DEBUG_ASSERT(GetPrimTypeSize(fromType) == GetPrimTypeSize(toType), "retype bit widith doesn' match");
    if (LIsPrimitivePointer(fromType) && LIsPrimitivePointer(toType)) {
        return &LoadIntoRegister(opnd0, toType);
    }
    // if source operand is in memory,
    // simply read it as a value of 'toType 'into the dest operand and return
    if (opnd0.IsMemoryAccessOperand()) {
        return &SelectCopy(opnd0, toType, toType);
    }

    bool isFromInt = IsPrimitiveInteger(fromType);
    bool is64Bits = GetPrimTypeBitSize(fromType) == k64BitSize;
    bool isImm = false;
    Operand *newOpnd0 = &opnd0;
    if (opnd0.IsImmediate()) {
        // according to aarch64 encoding format, convert int to float expression
        ImmOperand *imm = static_cast<ImmOperand *>(&opnd0);
        uint64 val = static_cast<uint64>(imm->GetValue());
        uint64 canRepreset = is64Bits ? (val & 0xffffffffffff) : (val & 0x7ffff);
        uint32 val1 = is64Bits ? (val >> 61) & 0x3 : (val >> 29) & 0x3;
        uint32 val2 = is64Bits ? (val >> 54) & 0xff : (val >> 25) & 0x1f;
        bool isSame = is64Bits ? ((val2 == 0) || (val2 == 0xff)) : ((val2 == 0) || (val2 == 0x1f));
        canRepreset = (canRepreset == 0) && ((val1 & 0x1) ^ ((val1 & 0x2) >> 1)) && isSame;
        if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType) && canRepreset) {
            uint64 temp1 = is64Bits ? (val >> 63) << 7 : (val >> 31) << 7;
            uint64 temp2 = is64Bits ? val >> 48 : val >> 19;
            int64 imm8 = (temp2 & 0x7f) | temp1;
            newOpnd0 = &CreateImmOperand(imm8, k8BitSize, false, kNotVary, true);
            isImm = true;
        }
    }
    if (!isImm) {
        bool isSigned = IsSignedInteger(fromType);
        PrimType itype = isFromInt ? (is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32))
                                   : (is64Bits ? PTY_f64 : PTY_f32);
        newOpnd0 = &LoadIntoRegister(opnd0, itype);
    }
    if ((IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) ||
        (IsPrimitiveFloat(toType) && IsPrimitiveInteger(fromType))) {
        MOperator mopFmov =  isImm ? (is64Bits ? MOP_xdfmovri : MOP_wsfmovri)
                                   : (isFromInt ? (is64Bits ? MOP_xvmovdr : MOP_xvmovsr)
                                                : (is64Bits ? MOP_xvmovrd : MOP_xvmovrs));
        RegOperand *resOpnd = &CreateRegisterOperandOfType(toType);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopFmov, *resOpnd, *newOpnd0));
        return resOpnd;
    }
    return newOpnd0;
}

void AArch64CGFunc::SelectCvtFloat2Float(Operand &resOpnd, Operand &srcOpnd, PrimType fromType, PrimType toType)
{
    Operand &opnd0 = LoadIntoRegister(srcOpnd, fromType);
    MOperator mOp = 0;
    switch (toType) {
        case PTY_f32: {
            CHECK_FATAL(fromType == PTY_f64, "unexpected cvt from type");
            mOp = MOP_xvcvtfd;
            break;
        }
        case PTY_f64: {
            CHECK_FATAL(fromType == PTY_f32, "unexpected cvt from type");
            mOp = MOP_xvcvtdf;
            break;
        }
        default:
            CHECK_FATAL(false, "unexpected cvt to type");
    }

    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0));
}

/*
 * This should be regarded only as a reference.
 *
 * C11 specification.
 * 6.3.1.3 Signed and unsigned integers
 * 1 When a value with integer type is converted to another integer
 *  type other than _Bool, if the value can be represented by the
 *  new type, it is unchanged.
 * 2 Otherwise, if the new type is unsigned, the value is converted
 *  by repeatedly adding or subtracting one more than the maximum
 *  value that can be represented in the new type until the value
 *  is in the range of the new type.60)
 * 3 Otherwise, the new type is signed and the value cannot be
 *  represented in it; either the result is implementation-defined
 *  or an implementation-defined signal is raised.
 */
void AArch64CGFunc::SelectCvtInt2Int(const BaseNode *parent, Operand *&resOpnd, Operand *opnd0, PrimType fromType,
                                     PrimType toType)
{
    uint32 fsize = GetPrimTypeBitSize(fromType);
    uint32 tsize = GetPrimTypeBitSize(toType);
    bool isExpand = tsize > fsize;
    bool is64Bit = (tsize == k64BitSize);
    if ((parent != nullptr) && opnd0->IsIntImmediate() &&
        ((parent->GetOpCode() == OP_band) || (parent->GetOpCode() == OP_bior) || (parent->GetOpCode() == OP_bxor) ||
         (parent->GetOpCode() == OP_ashr) || (parent->GetOpCode() == OP_lshr) || (parent->GetOpCode() == OP_shl))) {
        ImmOperand *simm = static_cast<ImmOperand *>(opnd0);
        DEBUG_ASSERT(simm != nullptr, "simm is nullptr in AArch64CGFunc::SelectCvtInt2Int");
        bool isSign = false;
        int64 origValue = simm->GetValue();
        int64 newValue = origValue;
        int64 signValue = 0;
        if (!isExpand) {
            /* 64--->32 */
            if (fsize > tsize) {
                if (IsSignedInteger(toType)) {
                    if (origValue < 0) {
                        signValue = static_cast<int64>(0xFFFFFFFFFFFFFFFFLL & (1ULL << static_cast<uint32>(tsize)));
                    }
                    newValue = static_cast<int64>(
                        (static_cast<uint64>(origValue) & ((1ULL << static_cast<uint32>(tsize)) - 1u)) |
                        static_cast<uint64>(signValue));
                } else {
                    newValue = static_cast<uint64>(origValue) & ((1ULL << static_cast<uint32>(tsize)) - 1u);
                }
            }
        }
        if (IsSignedInteger(toType)) {
            isSign = true;
        }
        resOpnd = &static_cast<Operand &>(CreateImmOperand(newValue, GetPrimTypeSize(toType) * kBitsPerByte, isSign));
        return;
    }
    if (isExpand) { /* Expansion */
        /* if cvt expr's parent is add,and,xor and some other,we can use the imm version */
        PrimType primType = ((fsize == k64BitSize) ? (IsSignedInteger(fromType) ? PTY_i64 : PTY_u64)
                                                   : (IsSignedInteger(fromType) ? PTY_i32 : PTY_u32));
        opnd0 = &LoadIntoRegister(*opnd0, primType);

        if (IsSignedInteger(fromType)) {
            DEBUG_ASSERT((is64Bit || (fsize == k8BitSize || fsize == k16BitSize)), "incorrect from size");

            MOperator mOp =
                (is64Bit ? ((fsize == k8BitSize) ? MOP_xsxtb64 : ((fsize == k16BitSize) ? MOP_xsxth64 : MOP_xsxtw64))
                         : ((fsize == k8BitSize) ? MOP_xsxtb32 : MOP_xsxth32));
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *resOpnd, *opnd0));
        } else {
            /* Unsigned */
            auto mOp =
                (is64Bit ? ((fsize == k8BitSize) ? MOP_xuxtb32 : ((fsize == k16BitSize) ? MOP_xuxth32 : MOP_xuxtw64))
                         : ((fsize == k8BitSize) ? MOP_xuxtb32 : MOP_xuxth32));
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *resOpnd, LoadIntoRegister(*opnd0, fromType)));
        }
    } else { /* Same size or truncate */
#ifdef CNV_OPTIMIZE
        /*
         * No code needed for aarch64 with same reg.
         * Just update regno.
         */
        RegOperand *reg = static_cast<RegOperand *>(resOpnd);
        reg->regNo = static_cast<RegOperand *>(opnd0)->regNo;
#else
        /*
         *  This is not really needed if opnd0 is result from a load.
         * Hopefully the FE will get rid of the redundant conversions for loads.
         */
        PrimType primType = ((fsize == k64BitSize) ? (IsSignedInteger(fromType) ? PTY_i64 : PTY_u64)
                                                   : (IsSignedInteger(fromType) ? PTY_i32 : PTY_u32));
        opnd0 = &LoadIntoRegister(*opnd0, primType);

        if (fsize > tsize) {
            if (tsize == k8BitSize) {
                MOperator mOp = IsSignedInteger(toType) ? MOP_xsxtb32 : MOP_xuxtb32;
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *resOpnd, *opnd0));
            } else if (tsize == k16BitSize) {
                MOperator mOp = IsSignedInteger(toType) ? MOP_xsxth32 : MOP_xuxth32;
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *resOpnd, *opnd0));
            } else {
                MOperator mOp = IsSignedInteger(toType) ? MOP_xsbfxrri6i6 : MOP_xubfxrri6i6;
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *resOpnd, *opnd0,
                                                                   CreateImmOperand(0, k8BitSize, false),
                                                                   CreateImmOperand(tsize, k8BitSize, false)));
            }
        } else {
            /* same size, so resOpnd can be set */
            if ((IsSignedInteger(fromType) == IsSignedInteger(toType)) ||
                (GetPrimTypeSize(toType) >= k4BitSize)) {
                resOpnd = opnd0;
            } else if (IsUnsignedInteger(toType)) {
                MOperator mop;
                switch (toType) {
                    case PTY_u8:
                        mop = MOP_xuxtb32;
                        break;
                    case PTY_u16:
                        mop = MOP_xuxth32;
                        break;
                    default:
                        CHECK_FATAL(0, "Unhandled unsigned convert");
                }
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, *resOpnd, *opnd0));
            } else {
                /* signed target */
                uint32 size = GetPrimTypeSize(toType);
                MOperator mop;
                switch (toType) {
                    case PTY_i8:
                        mop = (size > k4BitSize) ? MOP_xsxtb64 : MOP_xsxtb32;
                        break;
                    case PTY_i16:
                        mop = (size > k4BitSize) ? MOP_xsxth64 : MOP_xsxth32;
                        break;
                    default:
                        CHECK_FATAL(0, "Unhandled unsigned convert");
                }
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, *resOpnd, *opnd0));
            }
        }
#endif
    }
}

Operand *AArch64CGFunc::SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0)
{
    PrimType fromType = node.FromType();
    PrimType toType = node.GetPrimType();
    if (fromType == toType) {
        return &opnd0; /* noop */
    }
    Operand *resOpnd = &GetOrCreateResOperand(parent, toType);
    if (IsPrimitiveFloat(toType) && IsPrimitiveInteger(fromType)) {
        SelectCvtInt2Float(*resOpnd, opnd0, toType, fromType);
    } else if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
        SelectCvtFloat2Int(*resOpnd, opnd0, toType, fromType);
    } else if (IsPrimitiveInteger(fromType) && IsPrimitiveInteger(toType)) {
        SelectCvtInt2Int(&parent, resOpnd, &opnd0, fromType, toType);
    } else { /* both are float type */
        SelectCvtFloat2Float(*resOpnd, opnd0, fromType, toType);
    }
    return resOpnd;
}

Operand *AArch64CGFunc::SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType ftype = node.FromType();
    PrimType nodeType = node.GetPrimType();
    bool is64Bits = (GetPrimTypeBitSize(node.GetPrimType()) == k64BitSize);
    bool isFloat = (IsPrimitiveFloat(nodeType));
    if (isFloat) {
        CHECK_FATAL(nodeType == PTY_f32 || nodeType == PTY_f64, "only support f32, f64");
        return SelectRoundOperator(kTrunc, node, opnd0, parent);
    }
    PrimType itype = (is64Bits) ? (IsSignedInteger(node.GetPrimType()) ? PTY_i64 : PTY_u64)
                                : (IsSignedInteger(node.GetPrimType()) ? PTY_i32 : PTY_u32); /* promoted type */
    RegOperand &resOpnd = GetOrCreateResOperand(parent, itype);
    SelectCvtFloat2Int(resOpnd, opnd0, itype, ftype);
    return &resOpnd;
}

/*
 * syntax: select <prim-type> (<opnd0>, <opnd1>, <opnd2>)
 * <opnd0> must be of integer type.
 * <opnd1> and <opnd2> must be of the type given by <prim-type>.
 * If <opnd0> is not 0, return <opnd1>.  Otherwise, return <opnd2>.
 */
void AArch64CGFunc::SelectAArch64Select(Operand &dest, Operand &o0, Operand &o1, CondOperand &cond, bool isIntType,
                                        uint32 dsize)
{
    uint32 mOpCode =
        isIntType ? ((dsize == k64BitSize) ? MOP_xcselrrrc : MOP_wcselrrrc)
                  : ((dsize == k64BitSize) ? MOP_dcselrrrc : ((dsize == k32BitSize) ? MOP_scselrrrc : MOP_hcselrrrc));
    Operand &rflag = GetOrCreateRflag();
    if (o1.IsImmediate()) {
        uint32 movOp = (dsize == k64BitSize ? MOP_xmovri64 : MOP_wmovri32);
        RegOperand &movDest =
            CreateVirtualRegisterOperand(NewVReg(kRegTyInt, (dsize == k64BitSize) ? k8ByteSize : k4ByteSize));
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(movOp, movDest, o1));
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, dest, o0, movDest, cond, rflag));
        return;
    }
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, dest, o0, o1, cond, rflag));
}

void AArch64CGFunc::SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd)
{
    const SmallCaseVector &switchTable = rangeGotoNode.GetRangeGotoTable();
    MIRType *etype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64));
    /*
     * we store 8-byte displacement ( jump_label - offset_table_address )
     * in the table. Refer to AArch64Emit::Emit() in aarch64emit.cpp
     */
    std::vector<uint64> sizeArray;
    sizeArray.emplace_back(switchTable.size());
    MIRArrayType *arrayType = memPool->New<MIRArrayType>(etype->GetTypeIndex(), sizeArray);
    MIRAggConst *arrayConst = memPool->New<MIRAggConst>(mirModule, *arrayType);
    for (const auto &itPair : switchTable) {
        LabelIdx labelIdx = itPair.second;
        GetCurBB()->PushBackRangeGotoLabel(labelIdx);
        MIRConst *mirConst = memPool->New<MIRLblConst>(labelIdx, GetFunction().GetPuidx(), *etype);
        arrayConst->AddItem(mirConst, 0);
    }

    MIRSymbol *lblSt = GetFunction().GetSymTab()->CreateSymbol(kScopeLocal);
    lblSt->SetStorageClass(kScFstatic);
    lblSt->SetSKind(kStConst);
    lblSt->SetTyIdx(arrayType->GetTypeIndex());
    lblSt->SetKonst(arrayConst);
    std::string lblStr(".LB_");
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(GetFunction().GetStIdx().Idx());
    CHECK_FATAL(funcSt != nullptr, "funcSt should not be nullptr");
    uint32 labelIdxTmp = GetLabelIdx();
    lblStr += funcSt->GetName();
    lblStr += std::to_string(labelIdxTmp++);
    SetLabelIdx(labelIdxTmp);
    lblSt->SetNameStrIdx(lblStr);
    AddEmitSt(GetCurBB()->GetId(), *lblSt);

    PrimType itype = rangeGotoNode.Opnd(0)->GetPrimType();
    Operand &opnd0 = LoadIntoRegister(srcOpnd, itype);

    regno_t vRegNO = NewVReg(kRegTyInt, 8u);
    RegOperand *addOpnd = &CreateVirtualRegisterOperand(vRegNO);

    int32 minIdx = switchTable[0].first;
    SelectAdd(*addOpnd, opnd0,
              CreateImmOperand(-static_cast<int64>(minIdx) - static_cast<int64>(rangeGotoNode.GetTagOffset()),
                               GetPrimTypeBitSize(itype), true),
              itype);

    /* contains the index */
    if (addOpnd->GetSize() != GetPrimTypeBitSize(PTY_u64)) {
        addOpnd = static_cast<RegOperand *>(&SelectCopy(*addOpnd, PTY_u64, PTY_u64));
    }

    RegOperand &baseOpnd = CreateRegisterOperandOfType(PTY_u64);
    StImmOperand &stOpnd = CreateStImmOperand(*lblSt, 0, 0);

    /* load the address of the switch table */
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrp, baseOpnd, stOpnd));
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrpl12, baseOpnd, baseOpnd, stOpnd));

    /* load the displacement into a register by accessing memory at base + index*8 */
    Operand *disp = CreateMemOperand(MemOperand::kAddrModeBOrX, k64BitSize, baseOpnd, *addOpnd, k8BitShift);
    RegOperand &tgt = CreateRegisterOperandOfType(PTY_a64);
    SelectAdd(tgt, baseOpnd, *disp, PTY_u64);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xbr, tgt));
}

RegOperand &AArch64CGFunc::GetZeroOpnd(uint32 bitLen)
{
    /*
     * It is possible to have a bitLen < 32, eg stb.
     * Set it to 32 if it is less than 32.
     */
    if (bitLen < k32BitSize) {
        bitLen = k32BitSize;
    }
    DEBUG_ASSERT((bitLen == k32BitSize || bitLen == k64BitSize), "illegal bit length = %d", bitLen);
    return (bitLen == k32BitSize) ? GetOrCreatePhysicalRegisterOperand(RZR, k32BitSize, kRegTyInt)
                                  : GetOrCreatePhysicalRegisterOperand(RZR, k64BitSize, kRegTyInt);
}

/* if offset < 0, allocation; otherwise, deallocation */
MemOperand &AArch64CGFunc::CreateCallFrameOperand(int32 offset, uint32 size)
{
    MemOperand *memOpnd = CreateStackMemOpnd(RSP, offset, size);
    memOpnd->SetIndexOpt((offset < 0) ? MemOperand::kPreIndex : MemOperand::kPostIndex);
    return *memOpnd;
}

BitShiftOperand *AArch64CGFunc::GetLogicalShiftLeftOperand(uint32 shiftAmount, bool is64bits) const
{
    /* num(0, 16, 32, 48) >> 4 is num1(0, 1, 2, 3), num1 & (~3) == 0 */
    DEBUG_ASSERT((!shiftAmount || ((shiftAmount >> 4) & ~static_cast<uint32>(3)) == 0),
                 "shift amount should be one of 0, 16, 32, 48");
    /* movkLslOperands[4]~movkLslOperands[7] is for 64 bits */
    return &movkLslOperands[(shiftAmount >> 4) + (is64bits ? 4 : 0)];
}

AArch64CGFunc::MovkLslOperandArray AArch64CGFunc::movkLslOperands = {
    BitShiftOperand(BitShiftOperand::kLSL, 0, 4),
    BitShiftOperand(BitShiftOperand::kLSL, 16, 4),
    BitShiftOperand(BitShiftOperand::kLSL, static_cast<uint32>(-1), 0), /* invalid entry */
    BitShiftOperand(BitShiftOperand::kLSL, static_cast<uint32>(-1), 0), /* invalid entry */
    BitShiftOperand(BitShiftOperand::kLSL, 0, 6),
    BitShiftOperand(BitShiftOperand::kLSL, 16, 6),
    BitShiftOperand(BitShiftOperand::kLSL, 32, 6),
    BitShiftOperand(BitShiftOperand::kLSL, 48, 6),
};

MemOperand &AArch64CGFunc::CreateStkTopOpnd(uint32 offset, uint32 size)
{
    MemOperand *memOp = CreateStackMemOpnd(RFP, static_cast<int32>(offset), size);
    return *memOp;
}

MemOperand *AArch64CGFunc::CreateStackMemOpnd(regno_t preg, int32 offset, uint32 size)
{
    auto *memOp =
        memPool->New<MemOperand>(memPool->New<RegOperand>(preg, k64BitSize, kRegTyInt),
                                 &CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(offset)), k32BitSize), size);
    if (preg == RFP || preg == RSP) {
        memOp->SetStackMem(true);
    }
    return memOp;
}

/* Mem mod BOI || PreIndex || PostIndex */
MemOperand *AArch64CGFunc::CreateMemOperand(uint32 size, RegOperand &base, ImmOperand &ofstOp, bool isVolatile,
                                            MemOperand::AArch64AddressingMode mode) const
{
    auto *memOp = memPool->New<MemOperand>(size, base, ofstOp, mode);
    memOp->SetVolatile(isVolatile);
    if (base.GetRegisterNumber() == RFP || base.GetRegisterNumber() == RSP) {
        memOp->SetStackMem(true);
    }
    return memOp;
}

MemOperand *AArch64CGFunc::CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand &base,
                                            RegOperand *index, ImmOperand *offset, const MIRSymbol *symbol) const
{
    auto *memOp = memPool->New<MemOperand>(mode, size, base, index, offset, symbol);
    if (base.GetRegisterNumber() == RFP || base.GetRegisterNumber() == RSP) {
        memOp->SetStackMem(true);
    }
    return memOp;
}

MemOperand *AArch64CGFunc::CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand &base,
                                            RegOperand &index, ImmOperand *offset, const MIRSymbol &symbol,
                                            bool noExtend)
{
    auto *memOp = memPool->New<MemOperand>(mode, size, base, index, offset, symbol, noExtend);
    if (base.GetRegisterNumber() == RFP || base.GetRegisterNumber() == RSP) {
        memOp->SetStackMem(true);
    }
    return memOp;
}

MemOperand *AArch64CGFunc::CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 dSize, RegOperand &base,
                                            RegOperand &indexOpnd, uint32 shift, bool isSigned) const
{
    auto *memOp = memPool->New<MemOperand>(mode, dSize, base, indexOpnd, shift, isSigned);
    if (base.GetRegisterNumber() == RFP || base.GetRegisterNumber() == RSP) {
        memOp->SetStackMem(true);
    }
    return memOp;
}

MemOperand *AArch64CGFunc::CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 dSize, const MIRSymbol &sym)
{
    auto *memOp = memPool->New<MemOperand>(mode, dSize, sym);
    return memOp;
}

RegOperand &AArch64CGFunc::CreateRegisterOperandOfType(PrimType primType)
{
    RegType regType = GetRegTyFromPrimTy(primType);
    uint32 byteLength = GetPrimTypeSize(primType);
    return CreateRegisterOperandOfType(regType, byteLength);
}

RegOperand &AArch64CGFunc::CreateRegisterOperandOfType(RegType regty, uint32 byteLen)
{
    /* BUG: if half-precision floating point operations are supported? */
    /* AArch64 has 32-bit and 64-bit registers only */
    if (byteLen < k4ByteSize) {
        byteLen = k4ByteSize;
    }
    regno_t vRegNO = NewVReg(regty, byteLen);
    return CreateVirtualRegisterOperand(vRegNO);
}

RegOperand &AArch64CGFunc::CreateRflagOperand()
{
    /* AArch64 has Status register that is 32-bit wide. */
    regno_t vRegNO = NewVRflag();
    return CreateVirtualRegisterOperand(vRegNO);
}

void AArch64CGFunc::MergeReturn()
{
    uint32 exitBBSize = GetExitBBsVec().size();
    if (exitBBSize == 0) {
        return;
    }
    if ((exitBBSize == 1) && GetExitBB(0) == GetCurBB()) {
        return;
    }
    if (exitBBSize == 1) {
        BB *onlyExitBB = GetExitBB(0);
        LabelIdx labidx = CreateLabel();
        BB *retBB = CreateNewBB(labidx, onlyExitBB->IsUnreachable(), BB::kBBReturn, onlyExitBB->GetFrequency());
        onlyExitBB->AppendBB(*retBB);
        /* modify the original return BB. */
        DEBUG_ASSERT(onlyExitBB->GetKind() == BB::kBBReturn, "Error: suppose to merge multi return bb");
        onlyExitBB->SetKind(BB::kBBFallthru);

        GetExitBBsVec().pop_back();
        GetExitBBsVec().emplace_back(retBB);
        return;
    }

    LabelIdx labidx = CreateLabel();
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(labidx);
    uint32 freq = 0;
    for (auto *tmpBB : GetExitBBsVec()) {
        DEBUG_ASSERT(tmpBB->GetKind() == BB::kBBReturn, "Error: suppose to merge multi return bb");
        tmpBB->SetKind(BB::kBBGoto);
        tmpBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd));
        freq += tmpBB->GetFrequency();
    }
    BB *retBB = CreateNewBB(labidx, false, BB::kBBReturn, freq);
    GetLastBB()->PrependBB(*retBB);
    GetExitBBsVec().clear();
    GetExitBBsVec().emplace_back(retBB);
}

RegOperand *AArch64CGFunc::CreateVirtualRegisterOperand(regno_t vRegNO, uint32 size, RegType kind, uint32 flg) const
{
    RegOperand *res = memPool->New<RegOperand>(vRegNO, size, kind, flg);
    maplebe::VregInfo::vRegOperandTable[vRegNO] = res;
    return res;
}

RegOperand &AArch64CGFunc::CreateVirtualRegisterOperand(regno_t vRegNO)
{
    DEBUG_ASSERT((vReg.vRegOperandTable.find(vRegNO) == vReg.vRegOperandTable.end()), "already exist");
    DEBUG_ASSERT(vRegNO < vReg.VRegTableSize(), "index out of range");
    uint8 bitSize = static_cast<uint8>((static_cast<uint32>(vReg.VRegTableGetSize(vRegNO))) * kBitsPerByte);
    RegOperand *res = CreateVirtualRegisterOperand(vRegNO, bitSize, vReg.VRegTableGetType(vRegNO));
    return *res;
}

RegOperand &AArch64CGFunc::GetOrCreateVirtualRegisterOperand(regno_t vRegNO)
{
    auto it = maplebe::VregInfo::vRegOperandTable.find(vRegNO);
    return (it != maplebe::VregInfo::vRegOperandTable.end()) ? *(it->second) : CreateVirtualRegisterOperand(vRegNO);
}

// Stage B - Pre-padding and extension of arguments
bool AArch64CGFunc::SelectParmListPreprocess(StmtNode &naryNode, size_t start, std::vector<ParamDesc> &argsDesc,
                                             const MIRFunction *callee)
{
    bool hasSpecialArg = false;
    for (size_t i = start; i < naryNode.NumOpnds(); ++i) {
        BaseNode *argExpr = naryNode.Opnd(i);
        DEBUG_ASSERT(argExpr != nullptr, "not null check");
        PrimType primType = argExpr->GetPrimType();
        DEBUG_ASSERT(primType != PTY_void, "primType should not be void");
        auto *mirType = GlobalTables::GetTypeTable().GetPrimType(primType);
        (void)argsDesc.emplace_back(mirType, argExpr);
    }
    return hasSpecialArg;
}

std::pair<MIRFunction *, MIRFuncType *> AArch64CGFunc::GetCalleeFunction(StmtNode &naryNode) const
{
    MIRFunction *callee = nullptr;
    MIRFuncType *calleeType = nullptr;
    if (dynamic_cast<CallNode *>(&naryNode) != nullptr) {
        auto calleePuIdx = static_cast<CallNode &>(naryNode).GetPUIdx();
        callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePuIdx);
        calleeType = callee->GetMIRFuncType();
    } else if (naryNode.GetOpCode() == OP_icallproto) {
        auto *iCallNode = &static_cast<IcallNode &>(naryNode);
        MIRType *protoType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iCallNode->GetRetTyIdx());
        if (protoType->IsMIRPtrType()) {
            calleeType = static_cast<MIRPtrType *>(protoType)->GetPointedFuncType();
        } else if (protoType->IsMIRFuncType()) {
            calleeType = static_cast<MIRFuncType *>(protoType);
        }
    }
    return {callee, calleeType};
}

void AArch64CGFunc::SelectParmListPassByStack(const MIRType &mirType, Operand &opnd, uint32 memOffset, bool preCopyed,
                                              std::vector<Insn *> &insnForStackArgs)
{
    PrimType primType = preCopyed ? PTY_a64 : mirType.GetPrimType();
    auto &valReg = LoadIntoRegister(opnd, primType);
    auto &actMemOpnd = CreateMemOpnd(RSP, memOffset, GetPrimTypeBitSize(primType));
    Insn &strInsn = GetInsnBuilder()->BuildInsn(PickStInsn(GetPrimTypeBitSize(primType), primType), valReg, actMemOpnd);
    actMemOpnd.SetStackArgMem(true);
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel2 && insnForStackArgs.size() < kShiftAmount12) {
        (void)insnForStackArgs.emplace_back(&strInsn);
    } else {
        GetCurBB()->AppendInsn(strInsn);
    }
}

/*
   SelectParmList generates an instrunction for each of the parameters
   to load the parameter value into the corresponding register.
   We return a list of registers to the call instruction because
   they may be needed in the register allocation phase.
 */
void AArch64CGFunc::SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds, bool isCallNative)
{
    size_t opndIdx = 0;
    // the first opnd of ICallNode is not parameter of function
    if (naryNode.GetOpCode() == OP_icall || naryNode.GetOpCode() == OP_icallproto || isCallNative) {
        opndIdx++;
    }
    auto [callee, calleeType] = GetCalleeFunction(naryNode);

    std::vector<ParamDesc> argsDesc;
    std::vector<RegMapForPhyRegCpy> regMapForTmpBB;
    bool hasSpecialArg = SelectParmListPreprocess(naryNode, opndIdx, argsDesc, callee);
    BB *curBBrecord = GetCurBB();
    BB *tmpBB = nullptr;
    if (hasSpecialArg) {
        tmpBB = CreateNewBB();
    }

    AArch64CallConvImpl parmLocator(GetBecommon());
    CCLocInfo ploc;
    std::vector<Insn *> insnForStackArgs;

    for (size_t i = 0; i < argsDesc.size(); ++i) {
        if (hasSpecialArg) {
            DEBUG_ASSERT(tmpBB, "need temp bb for lower priority args");
            SetCurBB(argsDesc[i].isSpecialArg ? *curBBrecord : *tmpBB);
        }

        auto *mirType = argsDesc[i].mirType;

        // get param opnd, for unpreCody agg, opnd must be mem opnd
        Operand *opnd = nullptr;
        auto preCopyed = argsDesc[i].preCopyed;
        if (preCopyed) {                     // preCopyed agg, passed by address
            naryNode.SetMayTailcall(false);  // has preCopyed arguments, don't do tailcall
            opnd = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
            auto &spReg = GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
            SelectAdd(*opnd, spReg, CreateImmOperand(argsDesc[i].offset, k64BitSize, false), PTY_a64);
        } else {  // base type, clac true val
            opnd = &LoadIntoRegister(*HandleExpr(naryNode, *argsDesc[i].argExpr), mirType->GetPrimType());
        }
        parmLocator.LocateNextParm(*mirType, ploc, (i == 0), calleeType);

        // skip unused args
        if (callee && callee->GetFuncDesc().IsArgUnused(i)) {
            continue;
        }

        if (ploc.reg0 != kRinvalid) {  // load to the register.
            CHECK_FATAL(ploc.reg1 == kRinvalid, "NIY");
            auto &phyReg = GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(ploc.reg0),
                                                              GetPrimTypeBitSize(ploc.primTypeOfReg0),
                                                              GetRegTyFromPrimTy(ploc.primTypeOfReg0));
            DEBUG_ASSERT(opnd->IsRegister(), "NIY, must be reg");
            if (!DoCallerEnsureValidParm(phyReg, static_cast<RegOperand &>(*opnd), ploc.primTypeOfReg0)) {
                if (argsDesc[i].isSpecialArg) {
                    regMapForTmpBB.emplace_back(RegMapForPhyRegCpy(
                        &phyReg, ploc.primTypeOfReg0, static_cast<RegOperand *>(opnd), ploc.primTypeOfReg0));
                } else {
                    SelectCopy(phyReg, ploc.primTypeOfReg0, *opnd, ploc.primTypeOfReg0);
                }
            }
            srcOpnds.PushOpnd(phyReg);
            continue;
        }

        // store to the memory segment for stack-passsed arguments.
        if (CGOptions::IsBigEndian() && ploc.memSize < static_cast<int32>(k8ByteSize)) {
            ploc.memOffset = ploc.memOffset + static_cast<int32>(k4ByteSize);
        }
        SelectParmListPassByStack(*mirType, *opnd, static_cast<uint32>(ploc.memOffset), preCopyed, insnForStackArgs);
    }
    // if we have stack-passed arguments, don't do tailcall
    parmLocator.InitCCLocInfo(ploc);
    if (ploc.memOffset != 0) {
        naryNode.SetMayTailcall(false);
    }
    if (hasSpecialArg) {
        DEBUG_ASSERT(tmpBB, "need temp bb for lower priority args");
        SetCurBB(*tmpBB);
        for (auto it : regMapForTmpBB) {
            SelectCopy(*it.destReg, it.destType, *it.srcReg, it.srcType);
        }
        curBBrecord->InsertAtEnd(*tmpBB);
        SetCurBB(*curBBrecord);
    }
    for (auto &strInsn : insnForStackArgs) {
        GetCurBB()->AppendInsn(*strInsn);
    }
}

bool AArch64CGFunc::DoCallerEnsureValidParm(RegOperand &destOpnd, RegOperand &srcOpnd, PrimType formalPType)
{
    Insn *insn = nullptr;
    switch (formalPType) {
        case PTY_u1: {
            ImmOperand &lsbOpnd = CreateImmOperand(maplebe::k0BitSize, srcOpnd.GetSize(), false);
            ImmOperand &widthOpnd = CreateImmOperand(maplebe::k1BitSize, srcOpnd.GetSize(), false);
            bool is64Bit = (srcOpnd.GetSize() == maplebe::k64BitSize);
            insn = &GetInsnBuilder()->BuildInsn(is64Bit ? MOP_xubfxrri6i6 : MOP_wubfxrri5i5, destOpnd, srcOpnd, lsbOpnd,
                                                widthOpnd);
            break;
        }
        case PTY_u8:
        case PTY_i8:
            insn = &GetInsnBuilder()->BuildInsn(MOP_xuxtb32, destOpnd, srcOpnd);
            break;
        case PTY_u16:
        case PTY_i16:
            insn = &GetInsnBuilder()->BuildInsn(MOP_xuxth32, destOpnd, srcOpnd);
            break;
        default:
            break;
    }
    if (insn != nullptr) {
        GetCurBB()->AppendInsn(*insn);
        return true;
    }
    return false;
}

void AArch64CGFunc::SelectParmListNotC(StmtNode &naryNode, ListOperand &srcOpnds)
{
    size_t i = 0;
    if (naryNode.GetOpCode() == OP_icall || naryNode.GetOpCode() == OP_icallproto) {
        i++;
    }

    CCImpl &parmLocator = *GetOrCreateLocator(CCImpl::GetCallConvKind(naryNode));
    CCLocInfo ploc;
    std::vector<Insn *> insnForStackArgs;
    uint32 stackArgsCount = 0;
    for (uint32 pnum = 0; i < naryNode.NumOpnds(); ++i, ++pnum) {
        MIRType *ty = nullptr;
        BaseNode *argExpr = naryNode.Opnd(i);
        DEBUG_ASSERT(argExpr != nullptr, "argExpr should not be nullptr");
        PrimType primType = argExpr->GetPrimType();
        DEBUG_ASSERT(primType != PTY_void, "primType should not be void");
        /* use alloca  */
        ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<uint32>(primType)];
        RegOperand *expRegOpnd = nullptr;
        Operand *opnd = HandleExpr(naryNode, *argExpr);
        if (!opnd->IsRegister()) {
            opnd = &LoadIntoRegister(*opnd, primType);
        }
        expRegOpnd = static_cast<RegOperand *>(opnd);

        parmLocator.LocateNextParm(*ty, ploc);
        PrimType destPrimType = primType;
        if (ploc.reg0 != kRinvalid) { /* load to the register. */
            CHECK_FATAL(expRegOpnd != nullptr, "null ptr check");
            RegOperand &parmRegOpnd = GetOrCreatePhysicalRegisterOperand(
                static_cast<AArch64reg>(ploc.reg0), expRegOpnd->GetSize(), GetRegTyFromPrimTy(destPrimType));
            SelectCopy(parmRegOpnd, destPrimType, *expRegOpnd, primType);
            srcOpnds.PushOpnd(parmRegOpnd);
        } else { /* store to the memory segment for stack-passsed arguments. */
            if (CGOptions::IsBigEndian()) {
                if (GetPrimTypeBitSize(primType) < k64BitSize) {
                    ploc.memOffset = ploc.memOffset + static_cast<int32>(k4BitSize);
                }
            }
            MemOperand &actMemOpnd = CreateMemOpnd(RSP, ploc.memOffset, GetPrimTypeBitSize(primType));
            Insn &strInsn = GetInsnBuilder()->BuildInsn(PickStInsn(GetPrimTypeBitSize(primType), primType), *expRegOpnd,
                                                        actMemOpnd);
            actMemOpnd.SetStackArgMem(true);
            if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel1 && stackArgsCount < kShiftAmount12) {
                (void)insnForStackArgs.emplace_back(&strInsn);
                stackArgsCount++;
            } else {
                GetCurBB()->AppendInsn(strInsn);
            }
        }
        DEBUG_ASSERT(ploc.reg1 == 0, "SelectCall NYI");
    }
    for (auto &strInsn : insnForStackArgs) {
        GetCurBB()->AppendInsn(*strInsn);
    }
}

// based on call conv, choose how to prepare args
void AArch64CGFunc::SelectParmListWrapper(StmtNode &naryNode, ListOperand &srcOpnds, bool isCallNative)
{
    if (CCImpl::GetCallConvKind(naryNode) == kCCall) {
        SelectParmList(naryNode, srcOpnds, isCallNative);
    } else if (CCImpl::GetCallConvKind(naryNode) == kWebKitJS || CCImpl::GetCallConvKind(naryNode) == kGHC) {
        SelectParmListNotC(naryNode, srcOpnds);
    } else {
        CHECK_FATAL(false, "niy");
    }
}

void AArch64CGFunc::SelectCall(CallNode &callNode)
{
    MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
    MIRSymbol *fsym = GetFunction().GetLocalOrGlobalSymbol(fn->GetStIdx(), false);
    MIRType *retType = fn->GetReturnType();

    if (GetCG()->GenerateVerboseCG()) {
        const std::string &comment = fsym->GetName();
        GetCurBB()->AppendInsn(CreateCommentInsn(comment));
    }

    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    SelectParmListWrapper(callNode, *srcOpnds, false);

    Insn &callInsn = AppendCall(*fsym, *srcOpnds);
    GetCurBB()->SetHasCall();
    if (retType != nullptr) {
        callInsn.SetRetSize(static_cast<uint32>(retType->GetSize()));
        callInsn.SetIsCallReturnUnsigned(IsUnsignedInteger(retType->GetPrimType()));
    }
    const auto &deoptBundleInfo = callNode.GetDeoptBundleInfo();
    for (const auto &elem : deoptBundleInfo) {
        auto valueKind = elem.second.GetMapleValueKind();
        if (valueKind == MapleValue::kPregKind) {
            auto *opnd = GetOpndFromPregIdx(elem.second.GetPregIdx());
            CHECK_FATAL(opnd != nullptr, "pregIdx has not been assigned Operand");
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else if (valueKind == MapleValue::kConstKind) {
            auto *opnd = SelectIntConst(static_cast<const MIRIntConst &>(elem.second.GetConstValue()), callNode);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else {
            CHECK_FATAL(false, "not supported currently");
        }
    }
    AppendStackMapInsn(callInsn);

    /* check if this call use stack slot to return */
    if (fn->IsFirstArgReturn()) {
        SetStackProtectInfo(kRetureStackSlot);
    }

    GetFunction().SetHasCall();
}

void AArch64CGFunc::SelectIcall(IcallNode &icallNode)
{
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    SelectParmListWrapper(icallNode, *srcOpnds, false);

    Operand *srcOpnd = HandleExpr(icallNode, *icallNode.GetNopndAt(0));
    Operand *fptrOpnd = srcOpnd;
    if (fptrOpnd->GetKind() != Operand::kOpdRegister) {
        PrimType ty = icallNode.Opnd(0)->GetPrimType();
        fptrOpnd = &SelectCopy(*srcOpnd, ty, ty);
    }
    DEBUG_ASSERT(fptrOpnd->IsRegister(), "SelectIcall: function pointer not RegOperand");
    RegOperand *regOpnd = static_cast<RegOperand *>(fptrOpnd);
    Insn &callInsn = GetInsnBuilder()->BuildInsn(MOP_xblr, *regOpnd, *srcOpnds);

    MIRType *retType = icallNode.GetCallReturnType();
    if (retType != nullptr) {
        callInsn.SetRetSize(static_cast<uint32>(retType->GetSize()));
        callInsn.SetIsCallReturnUnsigned(IsUnsignedInteger(retType->GetPrimType()));
    }

    /* check if this icall use stack slot to return */
    CallReturnVector *p2nrets = &icallNode.GetReturnVec();
    if (p2nrets->size() == k1ByteSize) {
        StIdx stIdx = (*p2nrets)[0].first;
        CHECK_NULL_FATAL(mirModule.CurFunction());
        MIRSymbol *sym = GetBecommon().GetMIRModule().CurFunction()->GetSymTab()->GetSymbolFromStIdx(stIdx.Idx());
        if (sym != nullptr && (GetBecommon().GetTypeSize(sym->GetTyIdx().GetIdx()) > k16ByteSize)) {
            SetStackProtectInfo(kRetureStackSlot);
        }
    }

    GetCurBB()->AppendInsn(callInsn);
    GetCurBB()->SetHasCall();
    DEBUG_ASSERT(GetCurBB()->GetLastMachineInsn()->IsCall(), "lastInsn should be a call");
    GetFunction().SetHasCall();
    const auto &deoptBundleInfo = icallNode.GetDeoptBundleInfo();
    for (const auto &elem : deoptBundleInfo) {
        auto valueKind = elem.second.GetMapleValueKind();
        if (valueKind == MapleValue::kPregKind) {
            auto *opnd = GetOpndFromPregIdx(elem.second.GetPregIdx());
            CHECK_FATAL(opnd != nullptr, "pregIdx has not been assigned Operand");
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else if (valueKind == MapleValue::kConstKind) {
            auto *opnd = SelectIntConst(static_cast<const MIRIntConst &>(elem.second.GetConstValue()), icallNode);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else {
            CHECK_FATAL(false, "not supported currently");
        }
    }
    AppendStackMapInsn(callInsn);
}

void AArch64CGFunc::SelectComment(CommentNode &comment)
{
    GetCurBB()->AppendInsn(CreateCommentInsn(comment.GetComment()));
}

void AArch64CGFunc::SelectReturn(Operand *opnd0)
{
    bool is64x1vec = GetFunction().GetAttr(FUNCATTR_oneelem_simd) ? true : false;
    MIRType *floatType = GlobalTables::GetTypeTable().GetDouble();
    MIRType *retTyp = is64x1vec ? floatType : GetFunction().GetReturnType();
    CCImpl &retLocator = *GetOrCreateLocator(GetCurCallConvKind());
    CCLocInfo retMech;
    retLocator.LocateRetVal(*retTyp, retMech);
    if ((retMech.GetRegCount() > 0) && (opnd0 != nullptr)) {
        RegType regTyp = is64x1vec ? kRegTyFloat : GetRegTyFromPrimTy(retMech.GetPrimTypeOfReg0());
        PrimType oriPrimType = is64x1vec ? GetFunction().GetReturnType()->GetPrimType() : retMech.GetPrimTypeOfReg0();
        AArch64reg retReg = static_cast<AArch64reg>(retMech.GetReg0());
        if (opnd0->IsRegister()) {
            RegOperand *regOpnd = static_cast<RegOperand *>(opnd0);
            if (regOpnd->GetRegisterNumber() != retMech.GetReg0()) {
                RegOperand &retOpnd = GetOrCreatePhysicalRegisterOperand(retReg, regOpnd->GetSize(), regTyp);
                SelectCopy(retOpnd, retMech.GetPrimTypeOfReg0(), *regOpnd, oriPrimType);
            }
        } else if (opnd0->IsMemoryAccessOperand()) {
            auto *memopnd = static_cast<MemOperand *>(opnd0);
            RegOperand &retOpnd =
                GetOrCreatePhysicalRegisterOperand(retReg, GetPrimTypeBitSize(retMech.GetPrimTypeOfReg0()), regTyp);
            MOperator mOp = PickLdInsn(memopnd->GetSize(), retMech.GetPrimTypeOfReg0());
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, retOpnd, *memopnd));
        } else if (opnd0->IsConstImmediate()) {
            ImmOperand *immOpnd = static_cast<ImmOperand *>(opnd0);
            if (!is64x1vec) {
                RegOperand &retOpnd =
                    GetOrCreatePhysicalRegisterOperand(retReg, GetPrimTypeBitSize(retMech.GetPrimTypeOfReg0()),
                                                       GetRegTyFromPrimTy(retMech.GetPrimTypeOfReg0()));
                SelectCopy(retOpnd, retMech.GetPrimTypeOfReg0(), *immOpnd, retMech.GetPrimTypeOfReg0());
            } else {
                PrimType rType = GetFunction().GetReturnType()->GetPrimType();
                RegOperand *reg = &CreateRegisterOperandOfType(rType);
                SelectCopy(*reg, rType, *immOpnd, rType);
                RegOperand &retOpnd = GetOrCreatePhysicalRegisterOperand(retReg, GetPrimTypeBitSize(PTY_f64),
                                                                         GetRegTyFromPrimTy(PTY_f64));
                Insn &insn = GetInsnBuilder()->BuildInsn(MOP_xvmovdr, retOpnd, *reg);
                GetCurBB()->AppendInsn(insn);
            }
        } else {
            CHECK_FATAL(false, "nyi");
        }
    }
    GetExitBBsVec().emplace_back(GetCurBB());
}

RegOperand &AArch64CGFunc::GetOrCreateSpecialRegisterOperand(PregIdx sregIdx, PrimType primType)
{
    switch (sregIdx) {
        case kSregSp:
            return GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        case kSregFp:
            return GetOrCreatePhysicalRegisterOperand(RFP, k64BitSize, kRegTyInt);
        default:
            break;
    }

    bool useFpReg = !IsPrimitiveInteger(primType);
    AArch64reg pReg = RLAST_INT_REG;
    switch (sregIdx) {
        case kSregRetval0:
            pReg = useFpReg ? V0 : R0;
            break;
        case kSregRetval1:
            pReg = useFpReg ? V1 : R1;
            break;
        case kSregRetval2:
            pReg = V2;
            break;
        case kSregRetval3:
            pReg = V3;
            break;
        default:
            DEBUG_ASSERT(false, "Special pseudo registers NYI");
            break;
    }
    uint32 bitSize = GetPrimTypeBitSize(primType);
    bitSize = bitSize <= k32BitSize ? k32BitSize : bitSize;
    auto &phyOpnd = GetOrCreatePhysicalRegisterOperand(pReg, bitSize, GetRegTyFromPrimTy(primType));
    return SelectCopy(phyOpnd, primType, primType);  // most opt only deal vreg, so return a vreg
}

RegOperand &AArch64CGFunc::GetOrCreatePhysicalRegisterOperand(AArch64reg regNO, uint32 size, RegType kind, uint32 flag)
{
    uint64 aarch64PhyRegIdx = regNO;
    DEBUG_ASSERT(flag == 0, "Do not expect flag here");
    if (size <= k32BitSize) {
        size = k32BitSize;
        aarch64PhyRegIdx = aarch64PhyRegIdx << 1;
    } else if (size <= k64BitSize) {
        size = k64BitSize;
        aarch64PhyRegIdx = (aarch64PhyRegIdx << 1) + 1;
    } else {
        size = (size == k128BitSize) ? k128BitSize : k64BitSize;
        aarch64PhyRegIdx = aarch64PhyRegIdx << k4BitShift;
    }
    RegOperand *phyRegOpnd = nullptr;
    auto phyRegIt = phyRegOperandTable.find(aarch64PhyRegIdx);
    if (phyRegIt != phyRegOperandTable.end()) {
        phyRegOpnd = phyRegOperandTable[aarch64PhyRegIdx];
    } else {
        phyRegOpnd = memPool->New<RegOperand>(regNO, size, kind, flag);
        phyRegOperandTable.emplace(aarch64PhyRegIdx, phyRegOpnd);
    }
    return *phyRegOpnd;
}

const LabelOperand *AArch64CGFunc::GetLabelOperand(LabelIdx labIdx) const
{
    const MapleUnorderedMap<LabelIdx, LabelOperand *>::const_iterator it = hashLabelOpndTable.find(labIdx);
    if (it != hashLabelOpndTable.end()) {
        return it->second;
    }
    return nullptr;
}

LabelOperand &AArch64CGFunc::GetOrCreateLabelOperand(LabelIdx labIdx)
{
    MapleUnorderedMap<LabelIdx, LabelOperand *>::iterator it = hashLabelOpndTable.find(labIdx);
    if (it != hashLabelOpndTable.end()) {
        return *(it->second);
    }
    LabelOperand *res = memPool->New<LabelOperand>(GetShortFuncName().c_str(), labIdx, *memPool);
    hashLabelOpndTable[labIdx] = res;
    return *res;
}

LabelOperand &AArch64CGFunc::GetOrCreateLabelOperand(BB &bb)
{
    LabelIdx labelIdx = bb.GetLabIdx();
    if (labelIdx == MIRLabelTable::GetDummyLabel()) {
        labelIdx = CreateLabel();
        bb.AddLabel(labelIdx);
        SetLab2BBMap(labelIdx, bb);
    }
    return GetOrCreateLabelOperand(labelIdx);
}

OfstOperand &AArch64CGFunc::GetOrCreateOfstOpnd(uint64 offset, uint32 size)
{
    uint64 aarch64OfstRegIdx = offset;
    aarch64OfstRegIdx = (aarch64OfstRegIdx << 1);
    if (size == k64BitSize) {
        ++aarch64OfstRegIdx;
    }
    DEBUG_ASSERT(size == k32BitSize || size == k64BitSize, "ofStOpnd size check");
    auto it = hashOfstOpndTable.find(aarch64OfstRegIdx);
    if (it != hashOfstOpndTable.end()) {
        return *it->second;
    }
    OfstOperand *res = &CreateOfstOpnd(offset, size);
    hashOfstOpndTable[aarch64OfstRegIdx] = res;
    return *res;
}

MemOperand &AArch64CGFunc::GetOrCreateMemOpnd(const MIRSymbol &symbol, int64 offset, uint32 size, bool forLocalRef,
                                              bool needLow12, RegOperand *regOp)
{
    MIRStorageClass storageClass = symbol.GetStorageClass();
    if ((storageClass == kScAuto) || (storageClass == kScFormal)) {
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(GetMemlayout()->GetSymAllocInfo(symbol.GetStIndex()));
        if (forLocalRef) {
            auto p = GetMemlayout()->GetLocalRefLocMap().find(symbol.GetStIdx());
            CHECK_FATAL(p != GetMemlayout()->GetLocalRefLocMap().end(), "sym loc should have been defined");
            symLoc = static_cast<AArch64SymbolAlloc *>(p->second);
        }
        DEBUG_ASSERT(symLoc != nullptr, "sym loc should have been defined");
        /* At this point, we don't know which registers the callee needs to save. */
        DEBUG_ASSERT((IsFPLRAddedToCalleeSavedList() || (SizeOfCalleeSaved() == 0)),
                     "CalleeSaved won't be known until after Register Allocation");
        StIdx idx = symbol.GetStIdx();
        auto it = memOpndsRequiringOffsetAdjustment.find(idx);
        DEBUG_ASSERT((!IsFPLRAddedToCalleeSavedList() ||
                      ((it != memOpndsRequiringOffsetAdjustment.end()) || (storageClass == kScFormal))),
                     "Memory operand of this symbol should have been added to the hash table");
        int32 stOffset = GetBaseOffset(*symLoc);
        if (it != memOpndsRequiringOffsetAdjustment.end()) {
            if (GetMemlayout()->IsLocalRefLoc(symbol)) {
                if (!forLocalRef) {
                    return *(it->second);
                }
            } else {
                Operand *offOpnd = (it->second)->GetOffset();
                DEBUG_ASSERT(static_cast<OfstOperand *>(offOpnd) != nullptr,
                    "static cast of offOpnd should not be nullptr");
                if (((static_cast<OfstOperand *>(offOpnd))->GetOffsetValue() == (stOffset + offset)) &&
                    (it->second->GetSize() == size)) {
                    return *(it->second);
                }
            }
        }
        it = memOpndsForStkPassedArguments.find(idx);
        if (it != memOpndsForStkPassedArguments.end()) {
            if (GetMemlayout()->IsLocalRefLoc(symbol)) {
                if (!forLocalRef) {
                    return *(it->second);
                }
            } else {
                return *(it->second);
            }
        }

        RegOperand *baseOpnd = static_cast<RegOperand *>(GetBaseReg(*symLoc));
        int32 totalOffset = stOffset + static_cast<int32>(offset);
        /* needs a fresh copy of ImmOperand as we may adjust its offset at a later stage. */
        OfstOperand *offsetOpnd = nullptr;
        if (CGOptions::IsBigEndian()) {
            if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed && size < k64BitSize) {
                offsetOpnd = &CreateOfstOpnd(k4BitSize + static_cast<uint32>(totalOffset), k64BitSize);
            } else {
                offsetOpnd = &CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(totalOffset)), k64BitSize);
            }
        } else {
            offsetOpnd = &CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(totalOffset)), k64BitSize);
        }
        if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed &&
            MemOperand::IsPIMMOffsetOutOfRange(totalOffset, size)) {
            ImmOperand *offsetOprand;
            offsetOprand = &CreateImmOperand(totalOffset, k64BitSize, true, kUnAdjustVary);
            Operand *resImmOpnd = &SelectCopy(*offsetOprand, PTY_i64, PTY_i64);
            return *CreateMemOperand(MemOperand::kAddrModeBOrX, size, *baseOpnd, static_cast<RegOperand &>(*resImmOpnd),
                                     nullptr, symbol, true);
        } else {
            if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
                offsetOpnd->SetVary(kUnAdjustVary);
            }
            MemOperand *res = CreateMemOperand(MemOperand::kAddrModeBOi, size, *baseOpnd, nullptr, offsetOpnd, &symbol);
            if (!forLocalRef) {
                memOpndsRequiringOffsetAdjustment[idx] = res;
            }
            return *res;
        }
    } else {
        CHECK_FATAL(false, "NYI");
    }
}

MemOperand &AArch64CGFunc::HashMemOpnd(MemOperand &tMemOpnd)
{
    auto it = hashMemOpndTable.find(tMemOpnd);
    if (it != hashMemOpndTable.end()) {
        return *(it->second);
    }
    auto *res = memPool->New<MemOperand>(tMemOpnd);
    hashMemOpndTable[tMemOpnd] = res;
    return *res;
}

MemOperand &AArch64CGFunc::GetOrCreateMemOpnd(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand *base,
                                              RegOperand *index, ImmOperand *offset, const MIRSymbol *st)
{
    DEBUG_ASSERT(base != nullptr, "nullptr check");
    MemOperand tMemOpnd(mode, size, *base, index, offset, st);
    if (base->GetRegisterNumber() == RFP || base->GetRegisterNumber() == RSP) {
        tMemOpnd.SetStackMem(true);
    }
    return HashMemOpnd(tMemOpnd);
}

MemOperand &AArch64CGFunc::GetOrCreateMemOpnd(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand *base,
                                              RegOperand *index, int32 shift, bool isSigned)
{
    DEBUG_ASSERT(base != nullptr, "nullptr check");
    MemOperand tMemOpnd(mode, size, *base, *index, shift, isSigned);
    if (base->GetRegisterNumber() == RFP || base->GetRegisterNumber() == RSP) {
        tMemOpnd.SetStackMem(true);
    }
    return HashMemOpnd(tMemOpnd);
}

MemOperand &AArch64CGFunc::GetOrCreateMemOpnd(MemOperand &oldMem)
{
    return HashMemOpnd(oldMem);
}

/* offset: base offset from FP or SP */
MemOperand &AArch64CGFunc::CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size)
{
    OfstOperand &offsetOpnd = CreateOfstOpnd(static_cast<uint64>(offset), k32BitSize);
    /* do not need to check bit size rotate of sign immediate */
    bool checkSimm = (offset > kMinSimm64 && offset < kMaxSimm64Pair);
    if (!checkSimm && !ImmOperand::IsInBitSizeRot(kMaxImmVal12Bits, offset)) {
        Operand *resImmOpnd = &SelectCopy(CreateImmOperand(offset, k32BitSize, true), PTY_i32, PTY_i32);
        return *CreateMemOperand(MemOperand::kAddrModeBOrX, size, baseOpnd, static_cast<RegOperand *>(resImmOpnd),
                                 nullptr, nullptr);
    } else {
        return *CreateMemOperand(MemOperand::kAddrModeBOi, size, baseOpnd, nullptr, &offsetOpnd, nullptr);
    }
}

/* offset: base offset + #:lo12:Label+immediate */
MemOperand &AArch64CGFunc::CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size, const MIRSymbol &sym)
{
    OfstOperand &offsetOpnd = CreateOfstOpnd(static_cast<uint64>(offset), k32BitSize);
    DEBUG_ASSERT(ImmOperand::IsInBitSizeRot(kMaxImmVal12Bits, offset), "");
    return *CreateMemOperand(MemOperand::kAddrModeBOi, size, baseOpnd, nullptr, &offsetOpnd, &sym);
}

/*
 *  case 1: iread a64 <* <* void>> 0 (add a64 (
 *  addrof a64 $__reg_jni_func_tab$$libcore_all_bytecode,
 *  mul a64 (
 *    cvt a64 i32 (constval i32 21),
 *    constval a64 8)))
 *
 * case 2 : iread u32 <* u8> 0 (add a64 (regread a64 %61, constval a64 3))
 * case 3 : iread u32 <* u8> 0 (add a64 (regread a64 %61, regread a64 %65))
 * case 4 : iread u32 <* u8> 0 (add a64 (cvt a64 i32(regread  %n)))
 */
MemOperand *AArch64CGFunc::CheckAndCreateExtendMemOpnd(PrimType ptype, const BaseNode &addrExpr, int64 offset)
{
    aggParamReg = nullptr;
    if (addrExpr.GetOpCode() != OP_add || offset != 0) {
        return nullptr;
    }
    BaseNode *baseExpr = addrExpr.Opnd(0);
    BaseNode *addendExpr = addrExpr.Opnd(1);

    if (baseExpr->GetOpCode() == OP_regread) {
        /* case 2 */
        if (addendExpr->GetOpCode() == OP_constval) {
            DEBUG_ASSERT(addrExpr.GetNumOpnds() == kOpndNum2, "Unepect expr operand in CheckAndCreateExtendMemOpnd");
            ConstvalNode *constOfstNode = static_cast<ConstvalNode *>(addendExpr);
            DEBUG_ASSERT(constOfstNode->GetConstVal()->GetKind() == kConstInt, "expect MIRIntConst");
            MIRIntConst *intOfst = safe_cast<MIRIntConst>(constOfstNode->GetConstVal());
            CHECK_FATAL(intOfst != nullptr, "just checking");
            /* discard large offset and negative offset */
            if (intOfst->GetExtValue() > INT32_MAX || intOfst->IsNegative()) {
                return nullptr;
            }
            uint32 scale = static_cast<uint32>(intOfst->GetExtValue());
            OfstOperand &ofstOpnd = GetOrCreateOfstOpnd(scale, k32BitSize);
            uint32 dsize = GetPrimTypeBitSize(ptype);
            MemOperand *memOpnd =
                &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, GetPrimTypeBitSize(ptype),
                                    SelectRegread(*static_cast<RegreadNode *>(baseExpr)), nullptr, &ofstOpnd, nullptr);
            return IsOperandImmValid(PickLdInsn(dsize, ptype), memOpnd, kInsnSecondOpnd) ? memOpnd : nullptr;
            /* case 3 */
        } else if (addendExpr->GetOpCode() == OP_regread) {
            CHECK_FATAL(addrExpr.GetNumOpnds() == kOpndNum2, "Unepect expr operand in CheckAndCreateExtendMemOpnd");
            if (GetPrimTypeSize(baseExpr->GetPrimType()) != GetPrimTypeSize(addendExpr->GetPrimType())) {
                return nullptr;
            }

            auto *baseReg = SelectRegread(*static_cast<RegreadNode *>(baseExpr));
            auto *indexReg = SelectRegread(*static_cast<RegreadNode *>(addendExpr));
            MemOperand *memOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, GetPrimTypeBitSize(ptype), baseReg,
                                                      indexReg, nullptr, nullptr);
            if (CGOptions::IsArm64ilp32() && IsSignedInteger(addendExpr->GetPrimType())) {
                memOpnd->SetExtend(memOpnd->GetExtend() | MemOperand::ExtendInfo::kSignExtend);
            }
            return memOpnd;
            /* case 4 */
        } else if (addendExpr->GetOpCode() == OP_cvt && addendExpr->GetNumOpnds() == 1) {
            int shiftAmount = 0;
            BaseNode *cvtRegreadNode = addendExpr->Opnd(kInsnFirstOpnd);
            if (cvtRegreadNode->GetOpCode() == OP_regread && cvtRegreadNode->IsLeaf()) {
                uint32 fromSize = GetPrimTypeBitSize(cvtRegreadNode->GetPrimType());
                uint32 toSize = GetPrimTypeBitSize(addendExpr->GetPrimType());

                if (toSize < fromSize) {
                    return nullptr;
                }

                MemOperand *memOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, GetPrimTypeBitSize(ptype),
                                                          SelectRegread(*static_cast<RegreadNode *>(baseExpr)),
                                                          SelectRegread(*static_cast<RegreadNode *>(cvtRegreadNode)),
                                                          shiftAmount, toSize != fromSize);
                return memOpnd;
            }
        }
    }
    if (addendExpr->GetOpCode() != OP_mul || !IsPrimitiveInteger(ptype)) {
        return nullptr;
    }
    BaseNode *indexExpr, *scaleExpr;
    indexExpr = addendExpr->Opnd(0);
    scaleExpr = addendExpr->Opnd(1);
    if (scaleExpr->GetOpCode() != OP_constval) {
        return nullptr;
    }
    ConstvalNode *constValNode = static_cast<ConstvalNode *>(scaleExpr);
    CHECK_FATAL(constValNode->GetConstVal()->GetKind() == kConstInt, "expect MIRIntConst");
    MIRIntConst *mirIntConst = safe_cast<MIRIntConst>(constValNode->GetConstVal());
    CHECK_FATAL(mirIntConst != nullptr, "just checking");
    int32 scale = mirIntConst->GetExtValue();
    if (scale < 0) {
        return nullptr;
    }
    uint32 unsignedScale = static_cast<uint32>(scale);
    if (unsignedScale != GetPrimTypeSize(ptype) || indexExpr->GetOpCode() != OP_cvt) {
        return nullptr;
    }
    /* 8 is 1 << 3; 4 is 1 << 2; 2 is 1 << 1; 1 is 1 << 0 */
    int32 shift = (unsignedScale == 8) ? 3 : ((unsignedScale == 4) ? 2 : ((unsignedScale == 2) ? 1 : 0));
    RegOperand &base = static_cast<RegOperand &>(LoadIntoRegister(*HandleExpr(addrExpr, *baseExpr), PTY_a64));
    TypeCvtNode *typeCvtNode = static_cast<TypeCvtNode *>(indexExpr);
    PrimType fromType = typeCvtNode->FromType();
    PrimType toType = typeCvtNode->GetPrimType();
    MemOperand *memOpnd = nullptr;
    if ((fromType == PTY_i32) && (toType == PTY_a64)) {
        RegOperand &index =
            static_cast<RegOperand &>(LoadIntoRegister(*HandleExpr(*indexExpr, *indexExpr->Opnd(0)), PTY_i32));
        memOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, GetPrimTypeBitSize(ptype), &base, &index, shift, true);
    } else if ((fromType == PTY_u32) && (toType == PTY_a64)) {
        RegOperand &index =
            static_cast<RegOperand &>(LoadIntoRegister(*HandleExpr(*indexExpr, *indexExpr->Opnd(0)), PTY_u32));
        memOpnd =
            &GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, GetPrimTypeBitSize(ptype), &base, &index, shift, false);
    }
    return memOpnd;
}

MemOperand &AArch64CGFunc::CreateNonExtendMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr,
                                                  int64 offset)
{
    Operand *addrOpnd = nullptr;
    if ((addrExpr.GetOpCode() == OP_add || addrExpr.GetOpCode() == OP_sub) &&
        addrExpr.Opnd(1)->GetOpCode() == OP_constval) {
        addrOpnd = HandleExpr(addrExpr, *addrExpr.Opnd(0));
        ConstvalNode *constOfstNode = static_cast<ConstvalNode *>(addrExpr.Opnd(1));
        DEBUG_ASSERT(constOfstNode->GetConstVal()->GetKind() == kConstInt, "expect MIRIntConst");
        MIRIntConst *intOfst = safe_cast<MIRIntConst>(constOfstNode->GetConstVal());
        CHECK_FATAL(intOfst != nullptr, "just checking");
        offset = (addrExpr.GetOpCode() == OP_add) ? offset + intOfst->GetSXTValue() : offset - intOfst->GetSXTValue();
    } else {
        addrOpnd = HandleExpr(parent, addrExpr);
    }
    addrOpnd = static_cast<RegOperand *>(&LoadIntoRegister(*addrOpnd, PTY_a64));
    {
        OfstOperand &ofstOpnd = GetOrCreateOfstOpnd(static_cast<uint64>(offset), k64BitSize);
        return GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, GetPrimTypeBitSize(ptype),
                                  static_cast<RegOperand *>(addrOpnd), nullptr, &ofstOpnd, nullptr);
    }
}

/*
 * Create a memory operand with specified data type and memory ordering, making
 * use of aarch64 extend register addressing mode when possible.
 */
MemOperand &AArch64CGFunc::CreateMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset)
{
    MemOperand *memOpnd = CheckAndCreateExtendMemOpnd(ptype, addrExpr, offset);
    if (memOpnd != nullptr) {
        return *memOpnd;
    }
    return CreateNonExtendMemOpnd(ptype, parent, addrExpr, offset);
}

MemOperand *AArch64CGFunc::CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset)
{
    MemOperand *memOpnd = CheckAndCreateExtendMemOpnd(ptype, addrExpr, offset);
    if (memOpnd != nullptr) {
        return memOpnd;
    } else if (aggParamReg != nullptr) {
        return nullptr;
    }
    return &CreateNonExtendMemOpnd(ptype, parent, addrExpr, offset);
}

Operand &AArch64CGFunc::GetOrCreateFuncNameOpnd(const MIRSymbol &symbol) const
{
    return *memPool->New<FuncNameOperand>(symbol);
}

Operand &AArch64CGFunc::GetOrCreateRflag()
{
    if (rcc == nullptr) {
        rcc = &CreateRflagOperand();
    }
    return *rcc;
}

const Operand *AArch64CGFunc::GetRflag() const
{
    return rcc;
}

RegOperand &AArch64CGFunc::GetOrCreatevaryreg()
{
    if (vary == nullptr) {
        regno_t vRegNO = NewVReg(kRegTyVary, k8ByteSize);
        vary = &CreateVirtualRegisterOperand(vRegNO);
    }
    return *vary;
}

RegOperand *AArch64CGFunc::GetBaseReg(const SymbolAlloc &symAlloc)
{
    MemSegmentKind sgKind = symAlloc.GetMemSegment()->GetMemSegmentKind();
    DEBUG_ASSERT(((sgKind == kMsArgsRegPassed) || (sgKind == kMsLocals) || (sgKind == kMsRefLocals) ||
                  (sgKind == kMsArgsToStkPass) || (sgKind == kMsArgsStkPassed)),
                 "NYI");

    if (sgKind == kMsArgsStkPassed || sgKind == kMsCold) {
        return &GetOrCreatevaryreg();
    }

    if (fsp == nullptr) {
        fsp = &GetOrCreatePhysicalRegisterOperand(RFP, GetPointerSize() * kBitsPerByte, kRegTyInt);
    }
    return fsp;
}

int32 AArch64CGFunc::GetBaseOffset(const SymbolAlloc &symbolAlloc)
{
    const AArch64SymbolAlloc *symAlloc = static_cast<const AArch64SymbolAlloc *>(&symbolAlloc);
    // Call Frame layout of AArch64
    // Refer to V2 in aarch64_memlayout.h.
    // Do Not change this unless you know what you do
    // O2 mode refer to V2.1 in  aarch64_memlayout.cpp
    const int32 sizeofFplr = static_cast<int32>(2 * kAarch64IntregBytelen);
    MemSegmentKind sgKind = symAlloc->GetMemSegment()->GetMemSegmentKind();
    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(this->GetMemlayout());
    if (sgKind == kMsArgsStkPassed) { /* for callees */
        int32 offset = static_cast<int32>(symAlloc->GetOffset());
        offset += static_cast<int32>(memLayout->GetSizeOfColdToStk());
        return offset;
    } else if (sgKind == kMsCold) {
        int offset = static_cast<int32>(symAlloc->GetOffset());
        return offset;
    } else if (sgKind == kMsArgsRegPassed) {
        int32 baseOffset = static_cast<int32>(memLayout->GetSizeOfLocals() + memLayout->GetSizeOfRefLocals()) +
                           static_cast<int32>(symAlloc->GetOffset());
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsRefLocals) {
        int32 baseOffset = static_cast<int32>(symAlloc->GetOffset()) + static_cast<int32>(memLayout->GetSizeOfLocals());
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsLocals) {
        int32 baseOffset = symAlloc->GetOffset();
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsSpillReg) {
        int32 baseOffset = static_cast<int32>(symAlloc->GetOffset()) +
                           static_cast<int32>(memLayout->SizeOfArgsRegisterPassed() + memLayout->GetSizeOfLocals() +
                                              memLayout->GetSizeOfRefLocals());
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsArgsToStkPass) { /* this is for callers */
        return static_cast<int32>(symAlloc->GetOffset());
    } else {
        CHECK_FATAL(false, "sgKind check");
    }
    return 0;
}

void AArch64CGFunc::AppendCall(const MIRSymbol &funcSymbol)
{
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    AppendCall(funcSymbol, *srcOpnds);
}

void AArch64CGFunc::SelectAddAfterInsn(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType, bool isDest,
                                       Insn &insn)
{
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);
    DEBUG_ASSERT(opnd0.GetKind() == Operand::kOpdRegister, "Spill memory operand should based on register");
    DEBUG_ASSERT((opnd1.GetKind() == Operand::kOpdImmediate || opnd1.GetKind() == Operand::kOpdOffset),
                 "Spill memory operand should be with a immediate offset.");

    ImmOperand *immOpnd = static_cast<ImmOperand *>(&opnd1);

    MOperator mOpCode = MOP_undef;
    Insn *curInsn = &insn;
    /* lower 24 bits has 1, higher bits are all 0 */
    if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0)) {
        /* lower 12 bits and higher 12 bits both has 1 */
        Operand *newOpnd0 = &opnd0;
        if (!(immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
            /* process higher 12 bits */
            ImmOperand &immOpnd2 =
                CreateImmOperand(static_cast<int64>(static_cast<uint64>(immOpnd->GetValue()) >> kMaxImmVal12Bits),
                                 immOpnd->GetSize(), immOpnd->IsSignedValue());
            mOpCode = is64Bits ? MOP_xaddrri24 : MOP_waddrri24;
            BitShiftOperand &shiftopnd = CreateBitShiftOperand(BitShiftOperand::kLSL, kShiftAmount12, k64BitSize);
            Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, opnd0, immOpnd2, shiftopnd);
            DEBUG_ASSERT(IsOperandImmValid(mOpCode, &immOpnd2, kInsnThirdOpnd), "immOpnd2 appears invalid");
            if (isDest) {
                insn.GetBB()->InsertInsnAfter(insn, newInsn);
            } else {
                insn.GetBB()->InsertInsnBefore(insn, newInsn);
            }
            /* get lower 12 bits value */
            immOpnd->ModuloByPow2(static_cast<int32>(kMaxImmVal12Bits));
            newOpnd0 = &resOpnd;
            curInsn = &newInsn;
        }
        /* process lower 12 bits value */
        mOpCode = is64Bits ? MOP_xaddrri12 : MOP_waddrri12;
        Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, *newOpnd0, *immOpnd);
        DEBUG_ASSERT(IsOperandImmValid(mOpCode, immOpnd, kInsnThirdOpnd), "immOpnd appears invalid");
        if (isDest) {
            insn.GetBB()->InsertInsnAfter(*curInsn, newInsn);
        } else {
            insn.GetBB()->InsertInsnBefore(insn, newInsn);
        }
    } else {
        /* load into register */
        RegOperand &movOpnd = GetOrCreatePhysicalRegisterOperand(R16, dsize, kRegTyInt);
        mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
        Insn &movInsn = GetInsnBuilder()->BuildInsn(mOpCode, movOpnd, *immOpnd);
        mOpCode = is64Bits ? MOP_xaddrrr : MOP_waddrrr;
        Insn &newInsn = GetInsnBuilder()->BuildInsn(mOpCode, resOpnd, opnd0, movOpnd);
        if (isDest) {
            (void)insn.GetBB()->InsertInsnAfter(insn, newInsn);
            (void)insn.GetBB()->InsertInsnAfter(insn, movInsn);
        } else {
            (void)insn.GetBB()->InsertInsnBefore(insn, movInsn);
            (void)insn.GetBB()->InsertInsnBefore(insn, newInsn);
        }
    }
}

MemOperand *AArch64CGFunc::AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t vrNum, bool isDest,
                                                              Insn &insn, AArch64reg regNum, bool &isOutOfRange)
{
    if (vrNum >= vReg.VRegTableSize()) {
        CHECK_FATAL(false, "index out of range in AArch64CGFunc::AdjustMemOperandIfOffsetOutOfRange");
    }
    uint32 dataSize = GetOrCreateVirtualRegisterOperand(vrNum).GetSize();
    if (IsImmediateOffsetOutOfRange(*memOpnd, dataSize) && CheckIfSplitOffsetWithAdd(*memOpnd, dataSize)) {
        isOutOfRange = true;
        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, dataSize, regNum, isDest, &insn);
    } else {
        isOutOfRange = false;
    }
    return memOpnd;
}

void AArch64CGFunc::FreeSpillRegMem(regno_t vrNum)
{
    MemOperand *memOpnd = nullptr;

    auto p = spillRegMemOperands.find(vrNum);
    if (p != spillRegMemOperands.end()) {
        memOpnd = p->second;
    }

    if ((memOpnd == nullptr) && IsVRegNOForPseudoRegister(vrNum)) {
        auto pSecond = pRegSpillMemOperands.find(GetPseudoRegIdxFromVirtualRegNO(vrNum));
        if (pSecond != pRegSpillMemOperands.end()) {
            memOpnd = pSecond->second;
        }
    }

    if (memOpnd == nullptr) {
        DEBUG_ASSERT(false, "free spillreg have no mem");
        return;
    }

    uint32 size = memOpnd->GetSize();
    MapleUnorderedMap<uint32, SpillMemOperandSet *>::iterator iter;
    if ((iter = reuseSpillLocMem.find(size)) != reuseSpillLocMem.end()) {
        iter->second->Add(*memOpnd);
    } else {
        reuseSpillLocMem[size] = memPool->New<SpillMemOperandSet>(*GetFuncScopeAllocator());
        reuseSpillLocMem[size]->Add(*memOpnd);
    }
}

MemOperand *AArch64CGFunc::GetOrCreatSpillMem(regno_t vrNum, uint32 memSize)
{
    /* NOTES: must used in RA, not used in other place. */
    if (IsVRegNOForPseudoRegister(vrNum)) {
        auto p = pRegSpillMemOperands.find(GetPseudoRegIdxFromVirtualRegNO(vrNum));
        if (p != pRegSpillMemOperands.end()) {
            return p->second;
        }
    }

    auto p = spillRegMemOperands.find(vrNum);
    if (p == spillRegMemOperands.end()) {
        if (vrNum >= vReg.VRegTableSize()) {
            CHECK_FATAL(false, "index out of range in AArch64CGFunc::FreeSpillRegMem");
        }
        uint32 memBitSize = (memSize <= k32BitSize) ? k32BitSize : (memSize <= k64BitSize) ? k64BitSize : k128BitSize;
        auto it = reuseSpillLocMem.find(memBitSize);
        if (it != reuseSpillLocMem.end()) {
            MemOperand *memOpnd = it->second->GetOne();
            if (memOpnd != nullptr) {
                (void)spillRegMemOperands.emplace(std::pair<regno_t, MemOperand *>(vrNum, memOpnd));
                return memOpnd;
            }
        }

        RegOperand &baseOpnd = GetOrCreateStackBaseRegOperand();
        int64 offset = GetOrCreatSpillRegLocation(vrNum, memBitSize / kBitsPerByte);
        MemOperand *memOpnd = nullptr;
        OfstOperand *offsetOpnd = &CreateOfstOpnd(static_cast<uint64>(offset), k64BitSize);
        memOpnd = CreateMemOperand(MemOperand::kAddrModeBOi, memBitSize, baseOpnd, nullptr, offsetOpnd, nullptr);
        (void)spillRegMemOperands.emplace(std::pair<regno_t, MemOperand *>(vrNum, memOpnd));
        return memOpnd;
    } else {
        return p->second;
    }
}

MemOperand *AArch64CGFunc::GetPseudoRegisterSpillMemoryOperand(PregIdx i)
{
    MapleUnorderedMap<PregIdx, MemOperand *>::iterator p;
    if (GetCG()->GetOptimizeLevel() == CGOptions::kLevel0) {
        p = pRegSpillMemOperands.end();
    } else {
        p = pRegSpillMemOperands.find(i);
    }
    if (p != pRegSpillMemOperands.end()) {
        return p->second;
    }
    int64 offset = GetPseudoRegisterSpillLocation(i);
    MIRPreg *preg = GetFunction().GetPregTab()->PregFromPregIdx(i);
    uint32 bitLen = GetPrimTypeSize(preg->GetPrimType()) * kBitsPerByte;
    RegOperand &base = GetOrCreateFramePointerRegOperand();

    OfstOperand &ofstOpnd = GetOrCreateOfstOpnd(static_cast<uint64>(offset), k32BitSize);
    MemOperand &memOpnd = GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, bitLen, &base, nullptr, &ofstOpnd, nullptr);
    if (IsImmediateOffsetOutOfRange(memOpnd, bitLen)) {
        MemOperand &newMemOpnd = SplitOffsetWithAddInstruction(memOpnd, bitLen);
        (void)pRegSpillMemOperands.emplace(std::pair<PregIdx, MemOperand *>(i, &newMemOpnd));
        return &newMemOpnd;
    }
    (void)pRegSpillMemOperands.emplace(std::pair<PregIdx, MemOperand *>(i, &memOpnd));
    return &memOpnd;
}

Insn &AArch64CGFunc::AppendCall(const MIRSymbol &sym, ListOperand &srcOpnds)
{
    Insn *callInsn = nullptr;
    Operand &targetOpnd = GetOrCreateFuncNameOpnd(sym);
    callInsn = &GetInsnBuilder()->BuildInsn(MOP_xbl, targetOpnd, srcOpnds);
    GetCurBB()->AppendInsn(*callInsn);
    GetCurBB()->SetHasCall();
    return *callInsn;
}

// output
// add_with_overflow/ sub_with_overflow:
//    w1: parm1
//    w2: parm2
//    adds/subs w0, w1, w2
//    cset w3, vs

// mul_with_overflow:
//    w1: parm1
//    w2: parm2
//    smull x0, w0, w1
//    cmp   x0, w0, sxtw
//    cset  w4, ne
void AArch64CGFunc::SelectOverFlowCall(const IntrinsiccallNode &intrnNode)
{
    DEBUG_ASSERT(intrnNode.NumOpnds() == 2, "must be 2 operands");  // must be 2 operands
    MIRIntrinsicID intrinsic = intrnNode.GetIntrinsic();
    PrimType type = intrnNode.Opnd(0)->GetPrimType();
    PrimType type2 = intrnNode.Opnd(1)->GetPrimType();
    CHECK_FATAL(type == PTY_i32 || type == PTY_u32, "only support i32 or u32 here");
    CHECK_FATAL(type2 == PTY_i32 || type2 == PTY_u32, "only support i32 or u32 here");
    // deal with parms
    RegOperand &opnd0 = LoadIntoRegister(*HandleExpr(intrnNode, *intrnNode.Opnd(0)),
                                         intrnNode.Opnd(0)->GetPrimType()); /* first argument of intrinsic */
    RegOperand &opnd1 = LoadIntoRegister(*HandleExpr(intrnNode, *intrnNode.Opnd(1)),
                                         intrnNode.Opnd(1)->GetPrimType()); /* first argument of intrinsic */
    auto *retVals = &intrnNode.GetReturnVec();
    CHECK_FATAL(retVals->size() == k2ByteSize, "there must be two return values");
    PregIdx pregIdx = (*retVals)[0].second.GetPregIdx();
    PregIdx pregIdx2 = (*retVals)[1].second.GetPregIdx();
    RegOperand &resReg = GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    RegOperand &resReg2 = GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx2));
    Operand &rflag = GetOrCreateRflag();
    // arith operation with set flag
    if (intrinsic == INTRN_ADD_WITH_OVERFLOW) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_waddsrrr, rflag, resReg, opnd0, opnd1));
        SelectAArch64CSet(resReg2, GetCondOperand(CC_VS), false);
    } else if (intrinsic == INTRN_SUB_WITH_OVERFLOW) {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wsubsrrr, rflag, resReg, opnd0, opnd1));
        SelectAArch64CSet(resReg2, GetCondOperand(CC_VS), false);
    } else if (intrinsic == INTRN_MUL_WITH_OVERFLOW) {
        // smull
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xsmullrrr, resReg, opnd0, opnd1));
        Operand &sxtw = CreateExtendShiftOperand(ExtendShiftOperand::kSXTW, 0, k3BitSize);
        Insn &cmpInsn = GetInsnBuilder()->BuildInsn(MOP_xwcmprre, rflag, resReg, resReg, sxtw);
        GetCurBB()->AppendInsn(cmpInsn);
        SelectAArch64CSet(resReg2, GetCondOperand(CC_NE), false);
    } else {
        CHECK_FATAL(false, "niy");
    }
}

RegOperand &AArch64CGFunc::LoadOpndIntoPhysicalRegister(const IntrinsiccallNode &intrnNode, uint32 index)
{
    auto &opnd = *intrnNode.Opnd(index);
    auto ptyp = opnd.GetPrimType();
    RegOperand &opndReg = LoadIntoRegister(*HandleExpr(intrnNode, opnd), ptyp);
    AArch64reg regId;
    switch (index - 1) {
        case kFirstReg:
            regId = static_cast<AArch64reg>(R0);
            break;
        case kSecondReg:
            regId = static_cast<AArch64reg>(R1);
            break;
        case kThirdReg:
            regId = static_cast<AArch64reg>(R2);
            break;
        case kFourthReg:
            regId = static_cast<AArch64reg>(R3);
            break;
        case kFifthReg:
            regId = static_cast<AArch64reg>(R4);
            break;
        case kSixthReg:
            regId = static_cast<AArch64reg>(R5);
            break;
        default:
            CHECK_FATAL_FALSE("Unreachable!");
    }
    RegOperand &realReg = GetOrCreatePhysicalRegisterOperand(regId, opndReg.GetSize(), GetRegTyFromPrimTy(ptyp));
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickMovBetweenRegs(ptyp, ptyp), realReg, opndReg));
    return realReg;
}


void AArch64CGFunc::SelectPureCall(const IntrinsiccallNode &intrnNode)
{
    DEBUG_ASSERT(intrnNode.NumOpnds() == 7, "must be 7 operands");  // must be 7 operands
    // deal with parms
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    auto &callee = *intrnNode.Opnd(0);
    auto ptyp = callee.GetPrimType();
    RegOperand &calleeReg = LoadIntoRegister(*HandleExpr(intrnNode, callee), ptyp);
    uint32 i = 1;
    for (; i < kSeventhReg; i++) {
        srcOpnds->PushOpnd(LoadOpndIntoPhysicalRegister(intrnNode, i));
    }
    // R15 is used in asm call
    srcOpnds->PushOpnd(GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R15),
    GetPointerSize() * kBitsPerByte, kRegTyInt));
    Insn &callInsn = GetInsnBuilder()->BuildInsn(MOP_pure_call, calleeReg, *srcOpnds);
    GetCurBB()->AppendInsn(callInsn);
}

void AArch64CGFunc::SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode)
{
    MIRIntrinsicID intrinsic = intrinsiccallNode.GetIntrinsic();

    if (GetCG()->GenerateVerboseCG()) {
        std::string comment = GetIntrinsicName(intrinsic);
        GetCurBB()->AppendInsn(CreateCommentInsn(comment));
    }
    if (intrinsic == INTRN_ADD_WITH_OVERFLOW || intrinsic == INTRN_SUB_WITH_OVERFLOW ||
        intrinsic == INTRN_MUL_WITH_OVERFLOW) {
        SelectOverFlowCall(intrinsiccallNode);
        return;
    }
    if (intrinsic == maple::INTRN_JS_PURE_CALL) {
        SelectPureCall(intrinsiccallNode);
        return;
    }
}

Operand *AArch64CGFunc::SelectCclz(IntrinsicopNode &intrnNode)
{
    BaseNode *argexpr = intrnNode.Opnd(0);
    PrimType ptype = argexpr->GetPrimType();
    Operand *opnd = HandleExpr(intrnNode, *argexpr);
    MOperator mop;

    RegOperand &ldDest = CreateRegisterOperandOfType(ptype);
    if (opnd->IsMemoryAccessOperand()) {
        Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype), ptype), ldDest, *opnd);
        GetCurBB()->AppendInsn(insn);
        opnd = &ldDest;
    } else if (opnd->IsImmediate()) {
        SelectCopyImm(ldDest, *static_cast<ImmOperand *>(opnd), ptype);
        opnd = &ldDest;
    }

    if (GetPrimTypeSize(ptype) == k4ByteSize) {
        mop = MOP_wclz;
    } else {
        mop = MOP_xclz;
    }
    RegOperand &dst = CreateRegisterOperandOfType(ptype);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, dst, *opnd));
    return &dst;
}

RegType AArch64CGFunc::GetRegisterType(regno_t reg) const
{
    if (AArch64isa::IsPhysicalRegister(reg)) {
        return AArch64isa::GetRegType(static_cast<AArch64reg>(reg));
    } else if (reg == kRFLAG) {
        return kRegTyCc;
    } else {
        return CGFunc::GetRegisterType(reg);
    }
}

MemOperand &AArch64CGFunc::LoadStructCopyBase(const MIRSymbol &symbol, int64 offset, int dataSize)
{
    /* For struct formals > 16 bytes, this is the pointer to the struct copy. */
    /* Load the base pointer first. */
    RegOperand *vreg = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    MemOperand *baseMemOpnd = &GetOrCreateMemOpnd(symbol, 0, k64BitSize);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickLdInsn(k64BitSize, PTY_i64), *vreg, *baseMemOpnd));
    /* Create the indirect load mem opnd from the base pointer. */
    return CreateMemOpnd(*vreg, offset, static_cast<uint32>(dataSize));
}

/* For long branch, insert an unconditional branch.
 * From                      To
 *   cond_br targe_label       reverse_cond_br fallthru_label
 *   fallthruBB                unconditional br target_label
 *                             fallthru_label:
 *                             fallthruBB
 */
void AArch64CGFunc::InsertJumpPad(Insn *insn)
{
    BB *bb = insn->GetBB();
    DEBUG_ASSERT(bb, "instruction has no bb");
    DEBUG_ASSERT(bb->GetKind() == BB::kBBIf || bb->GetKind() == BB::kBBGoto,
                 "instruction is in neither if bb nor goto bb");
    if (bb->GetKind() == BB::kBBGoto) {
        return;
    }
    DEBUG_ASSERT(bb->NumSuccs() == k2ByteSize, "if bb should have 2 successors");

    BB *longBrBB = CreateNewBB();

    BB *fallthruBB = bb->GetNext();
    LabelIdx fallthruLBL = fallthruBB->GetLabIdx();
    if (fallthruLBL == 0) {
        fallthruLBL = CreateLabel();
        SetLab2BBMap(static_cast<int32>(fallthruLBL), *fallthruBB);
        fallthruBB->AddLabel(fallthruLBL);
    }

    BB *targetBB;
    if (bb->GetSuccs().front() == fallthruBB) {
        targetBB = bb->GetSuccs().back();
    } else {
        targetBB = bb->GetSuccs().front();
    }
    LabelIdx targetLBL = targetBB->GetLabIdx();
    if (targetLBL == 0) {
        targetLBL = CreateLabel();
        SetLab2BBMap(static_cast<int32>(targetLBL), *targetBB);
        targetBB->AddLabel(targetLBL);
    }

    // Adjustment on br and CFG
    bb->RemoveSuccs(*targetBB);
    bb->PushBackSuccs(*longBrBB);
    bb->SetNext(longBrBB);
    // reverse cond br targeting fallthruBB
    uint32 targetIdx = AArch64isa::GetJumpTargetIdx(*insn);
    MOperator mOp = AArch64isa::FlipConditionOp(insn->GetMachineOpcode());
    insn->SetMOP(AArch64CG::kMd[mOp]);
    LabelOperand &fallthruBBLBLOpnd = GetOrCreateLabelOperand(fallthruLBL);
    insn->SetOperand(targetIdx, fallthruBBLBLOpnd);

    longBrBB->PushBackPreds(*bb);
    longBrBB->PushBackSuccs(*targetBB);
    LabelOperand &targetLBLOpnd = GetOrCreateLabelOperand(targetLBL);
    longBrBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xuncond, targetLBLOpnd));
    longBrBB->SetPrev(bb);
    longBrBB->SetNext(fallthruBB);
    longBrBB->SetKind(BB::kBBGoto);

    fallthruBB->SetPrev(longBrBB);

    targetBB->RemovePreds(*bb);
    targetBB->PushBackPreds(*longBrBB);
}

/* Check the distance between the first insn of BB with the lable(targ_labidx)
 * and the insn with targ_id. If the distance greater than maxDistance
 * return false.
 */
bool AArch64CGFunc::DistanceCheck(const BB &bb, LabelIdx targLabIdx, uint32 targId, uint32 maxDistance) const
{
    for (auto *tBB : bb.GetSuccs()) {
        if (tBB->GetLabIdx() != targLabIdx) {
            continue;
        }
        Insn *tInsn = tBB->GetFirstInsn();
        while (tInsn == nullptr || !tInsn->IsMachineInstruction()) {
            if (tInsn == nullptr) {
                tBB = tBB->GetNext();
                if (tBB == nullptr) { /* tailcallopt may make the target block empty */
                    return true;
                }
                tInsn = tBB->GetFirstInsn();
            } else {
                tInsn = tInsn->GetNext();
            }
        }
        uint32 tmp = (tInsn->GetId() > targId) ? (tInsn->GetId() - targId) : (targId - tInsn->GetId());
        return (tmp < maxDistance);
    }
    CHECK_FATAL(false, "CFG error");
}
} /* namespace maplebe */
