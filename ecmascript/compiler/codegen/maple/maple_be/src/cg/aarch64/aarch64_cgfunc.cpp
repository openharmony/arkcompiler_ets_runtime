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
#include "mpl_atomic.h"
#include "metadata_layout.h"
#include "emit.h"
#include "simplify.h"
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

MOperator ldIsAcq[kSignedDimension][kIntByteSizeDimension] = {
    /* unsigned == 0 */
    {MOP_wldarb, MOP_wldarh, MOP_wldar, MOP_xldar},
    /* signed == 1 */
    {MOP_wldarb, MOP_wldarh, MOP_wldar, MOP_xldar}};

MOperator stIsRel[kSignedDimension][kIntByteSizeDimension] = {
    /* unsigned == 0 */
    {MOP_wstlrb, MOP_wstlrh, MOP_wstlr, MOP_xstlr},
    /* signed == 1 */
    {MOP_wstlrb, MOP_wstlrh, MOP_wstlr, MOP_xstlr}};

MOperator ldFs[kFloatByteSizeDimension] = {MOP_sldr, MOP_dldr, MOP_qldr};
MOperator stFs[kFloatByteSizeDimension] = {MOP_sstr, MOP_dstr, MOP_qstr};

MOperator ldFsAcq[kFloatByteSizeDimension] = {MOP_undef, MOP_undef, MOP_undef};
MOperator stFsRel[kFloatByteSizeDimension] = {MOP_undef, MOP_undef, MOP_undef};

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

MOperator PickLdStInsn(bool isLoad, uint32 bitSize, PrimType primType, AArch64isa::MemoryOrdering memOrd)
{
    DEBUG_ASSERT(__builtin_popcount(static_cast<uint32>(memOrd)) <= 1, "must be kMoNone or kMoAcquire");
    DEBUG_ASSERT(bitSize >= k8BitSize, "PTY_u1 should have been lowered?");
    DEBUG_ASSERT(__builtin_popcount(bitSize) == 1, "PTY_u1 should have been lowered?");
    if (isLoad) {
        DEBUG_ASSERT((memOrd == AArch64isa::kMoNone) || (memOrd == AArch64isa::kMoAcquire) ||
                         (memOrd == AArch64isa::kMoAcquireRcpc) || (memOrd == AArch64isa::kMoLoacquire),
                     "unknown Memory Order");
    } else {
        DEBUG_ASSERT((memOrd == AArch64isa::kMoNone) || (memOrd == AArch64isa::kMoRelease) ||
                         (memOrd == AArch64isa::kMoLorelease),
                     "unknown Memory Order");
    }

    /* __builtin_ffs(x) returns: 0 -> 0, 1 -> 1, 2 -> 2, 4 -> 3, 8 -> 4 */
    if ((IsPrimitiveInteger(primType) || primType == PTY_agg) && !IsPrimitiveVector(primType)) {
        MOperator(*table)[kIntByteSizeDimension];
        if (isLoad) {
            table = (memOrd == AArch64isa::kMoAcquire) ? ldIsAcq : ldIs;
        } else {
            table = (memOrd == AArch64isa::kMoRelease) ? stIsRel : stIs;
        }

        int32 signedUnsigned = IsUnsignedInteger(primType) ? 0 : 1;
        if (primType == PTY_agg) {
            CHECK_FATAL(bitSize >= k8BitSize, " unexpect agg size");
            bitSize = static_cast<uint32>(RoundUp(bitSize, k8BitSize));
            DEBUG_ASSERT((bitSize & (bitSize - 1)) == 0, "bitlen error");
        }

        /* __builtin_ffs(x) returns: 8 -> 4, 16 -> 5, 32 -> 6, 64 -> 7 */
        if (primType == PTY_i128 || primType == PTY_u128) {
            bitSize = k64BitSize;
        }
        uint32 size = static_cast<uint32>(__builtin_ffs(static_cast<int32>(bitSize))) - k4BitSize;
        DEBUG_ASSERT(size <= 3, "wrong bitSize");  // size must <= 3
        return table[signedUnsigned][size];
    } else {
        MOperator *table = nullptr;
        if (isLoad) {
            table = (memOrd == AArch64isa::kMoAcquire) ? ldFsAcq : ldFs;
        } else {
            table = (memOrd == AArch64isa::kMoRelease) ? stFsRel : stFs;
        }

        /* __builtin_ffs(x) returns: 32 -> 6, 64 -> 7, 128 -> 8 */
        uint32 size = static_cast<uint32>(__builtin_ffs(static_cast<int32>(bitSize))) - k6BitSize;
        DEBUG_ASSERT(size <= k2BitSize, "size must be 0 to 2");
        return table[size];
    }
}
}  // namespace

bool IsBlkassignForPush(const BlkassignoffNode &bNode)
{
    BaseNode *dest = bNode.Opnd(0);
    bool spBased = false;
    if (dest->GetOpCode() == OP_regread) {
        RegreadNode &node = static_cast<RegreadNode &>(*dest);
        if (-node.GetRegIdx() == kSregSp) {
            spBased = true;
        }
    }
    return spBased;
}

void AArch64CGFunc::SetMemReferenceOfInsn(Insn &insn, BaseNode *baseNode)
{
    if (baseNode == nullptr) {
        return;
    }
    MIRFunction &mirFunction = GetFunction();
    MemReferenceTable *memRefTable = mirFunction.GetMemReferenceTable();
    // In O0 mode, we do not run the ME, and the referenceTable is nullptr
    if (memRefTable == nullptr) {
        return;
    }
    MemDefUsePart &memDefUsePart = memRefTable->GetMemDefUsePart();
    if (memDefUsePart.find(baseNode) != memDefUsePart.end()) {
        insn.SetReferenceOsts(memDefUsePart.find(baseNode)->second);
    }
}

MIRStructType *AArch64CGFunc::GetLmbcStructArgType(BaseNode &stmt, size_t argNo) const
{
    MIRType *ty = nullptr;
    if (stmt.GetOpCode() == OP_call) {
        CallNode &callNode = static_cast<CallNode &>(stmt);
        MIRFunction *callFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
        if (callFunc->GetFormalCount() < (argNo + 1UL)) {
            return nullptr; /* formals less than actuals */
        }
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(callFunc->GetFormalDefVec()[argNo].formalTyIdx);
    } else if (stmt.GetOpCode() == OP_icallproto) {
        argNo--; /* 1st opnd of icallproto is funcname, skip it relative to param list */
        IcallNode &icallproto = static_cast<IcallNode &>(stmt);
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(icallproto.GetRetTyIdx());
        MIRFuncType *fType = nullptr;
        if (type->IsMIRPtrType()) {
            fType = static_cast<MIRPtrType *>(type)->GetPointedFuncType();
        } else {
            fType = static_cast<MIRFuncType *>(type);
        }
        CHECK_FATAL(fType != nullptr, "invalid fType");
        if (fType->GetParamTypeList().size() < (argNo + 1UL)) {
            return nullptr;
        }
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fType->GetNthParamType(argNo));
    }
    CHECK_FATAL(ty && ty->IsStructType(), "lmbc agg arg error");
    return static_cast<MIRStructType *>(ty);
}

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

MOperator AArch64CGFunc::PickLdInsn(uint32 bitSize, PrimType primType, AArch64isa::MemoryOrdering memOrd) const
{
    return PickLdStInsn(true, bitSize, primType, memOrd);
}

MOperator AArch64CGFunc::PickStInsn(uint32 bitSize, PrimType primType, AArch64isa::MemoryOrdering memOrd) const
{
    return PickLdStInsn(false, bitSize, primType, memOrd);
}

MOperator AArch64CGFunc::PickExtInsn(PrimType dtype, PrimType stype) const
{
    int32 sBitSize = static_cast<int32>(GetPrimTypeBitSize(stype));
    int32 dBitSize = static_cast<int32>(GetPrimTypeBitSize(dtype));
    /* __builtin_ffs(x) returns: 0 -> 0, 1 -> 1, 2 -> 2, 4 -> 3, 8 -> 4 */
    if (IsPrimitiveInteger(stype) && IsPrimitiveInteger(dtype)) {
        MOperator(*table)[kIntByteSizeDimension];
        table = IsUnsignedInteger(stype) ? uextIs : extIs;
        if (stype == PTY_i128 || stype == PTY_u128) {
            sBitSize = static_cast<int32>(k64BitSize);
        }
        /* __builtin_ffs(x) returns: 8 -> 4, 16 -> 5, 32 -> 6, 64 -> 7 */
        uint32 row = static_cast<uint32>(__builtin_ffs(sBitSize)) - k4BitSize;
        DEBUG_ASSERT(row <= k3BitSize, "wrong bitSize");
        if (dtype == PTY_i128 || dtype == PTY_u128) {
            dBitSize = static_cast<int32>(k64BitSize);
        }
        uint32 col = static_cast<uint32>(__builtin_ffs(dBitSize)) - k4BitSize;
        DEBUG_ASSERT(col <= k3BitSize, "wrong bitSize");
        return table[row][col];
    }
    CHECK_FATAL(0, "extend not primitive integer");
    return MOP_undef;
}

MOperator AArch64CGFunc::PickMovBetweenRegs(PrimType destType, PrimType srcType) const
{
    if (IsPrimitiveVector(destType) && IsPrimitiveVector(srcType)) {
        return GetPrimTypeSize(srcType) == k8ByteSize ? MOP_vmovuu : MOP_vmovvv;
    }
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
    if (IsPrimitiveInteger(destType) && IsPrimitiveVector(srcType)) {
        return GetPrimTypeSize(srcType) == k8ByteSize
                   ? MOP_vwmovru
                   : GetPrimTypeSize(destType) <= k4ByteSize ? MOP_vwmovrv : MOP_vxmovrv;
    }
    CHECK_FATAL(false, "unexpected operand primtype for mov");
    return MOP_undef;
}

MOperator AArch64CGFunc::PickMovInsn(const RegOperand &lhs, const RegOperand &rhs) const
{
    CHECK_FATAL(lhs.GetRegisterType() == rhs.GetRegisterType(), "PickMovInsn: unequal kind NYI");
    CHECK_FATAL(lhs.GetSize() == rhs.GetSize(), "PickMovInsn: unequal size NYI");
    DEBUG_ASSERT(((lhs.GetSize() < k64BitSize) || (lhs.GetRegisterType() == kRegTyFloat)),
                 "should split the 64 bits or more mov");
    if (lhs.GetRegisterType() == kRegTyInt) {
        return MOP_wmovrr;
    }
    if (lhs.GetRegisterType() == kRegTyFloat) {
        return (lhs.GetSize() <= k32BitSize) ? MOP_xvmovs : MOP_xvmovd;
    }
    DEBUG_ASSERT(false, "PickMovInsn: kind NYI");
    return MOP_undef;
}

void AArch64CGFunc::SelectLoadAcquire(Operand &dest, PrimType dtype, Operand &src, PrimType stype,
                                      AArch64isa::MemoryOrdering memOrd, bool isDirect)
{
    DEBUG_ASSERT(src.GetKind() == Operand::kOpdMem, "Just checking");
    DEBUG_ASSERT(memOrd != AArch64isa::kMoNone, "Just checking");

    uint32 ssize = isDirect ? src.GetSize() : GetPrimTypeBitSize(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    MOperator mOp = PickLdInsn(ssize, stype, memOrd);

    Operand *newSrc = &src;
    auto &memOpnd = static_cast<MemOperand &>(src);
    OfstOperand *immOpnd = memOpnd.GetOffsetImmediate();
    int32 offset = static_cast<int32>(immOpnd->GetOffsetValue());
    RegOperand *origBaseReg = memOpnd.GetBaseRegister();
    if (offset != 0) {
        RegOperand &resOpnd = CreateRegisterOperandOfType(PTY_i64);
        DEBUG_ASSERT(origBaseReg != nullptr, "nullptr check");
        SelectAdd(resOpnd, *origBaseReg, *immOpnd, PTY_i64);
        newSrc = &CreateReplacementMemOperand(ssize, resOpnd, 0);
    }

    std::string key;
    if (isDirect && GetCG()->GenerateVerboseCG()) {
        key = GenerateMemOpndVerbose(src);
    }

    /* Check if the right load-acquire instruction is available. */
    if (mOp != MOP_undef) {
        Insn &insn = GetInsnBuilder()->BuildInsn(mOp, dest, *newSrc);
        if (isDirect && GetCG()->GenerateVerboseCG()) {
            insn.SetComment(key);
        }
        GetCurBB()->AppendInsn(insn);
    } else {
        if (IsPrimitiveFloat(stype)) {
            /* Uses signed integer version ldar followed by a floating-point move(fmov).  */
            DEBUG_ASSERT(stype == dtype, "Just checking");
            PrimType itype = (stype == PTY_f32) ? PTY_i32 : PTY_i64;
            RegOperand &regOpnd = CreateRegisterOperandOfType(itype);
            Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, itype, memOrd), regOpnd, *newSrc);
            if (isDirect && GetCG()->GenerateVerboseCG()) {
                insn.SetComment(key);
            }
            GetCurBB()->AppendInsn(insn);
            mOp = (stype == PTY_f32) ? MOP_xvmovsr : MOP_xvmovdr;
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, dest, regOpnd));
        } else {
            /* Use unsigned version ldarb/ldarh followed by a sign-extension instruction(sxtb/sxth).  */
            DEBUG_ASSERT((ssize == k8BitSize) || (ssize == k16BitSize), "Just checking");
            PrimType utype = (ssize == k8BitSize) ? PTY_u8 : PTY_u16;
            Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, utype, memOrd), dest, *newSrc);
            if (isDirect && GetCG()->GenerateVerboseCG()) {
                insn.SetComment(key);
            }
            GetCurBB()->AppendInsn(insn);
            mOp = ((dsize == k32BitSize) ? ((ssize == k8BitSize) ? MOP_xsxtb32 : MOP_xsxth32)
                                         : ((ssize == k8BitSize) ? MOP_xsxtb64 : MOP_xsxth64));
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, dest, dest));
        }
    }
}

void AArch64CGFunc::SelectStoreRelease(Operand &dest, PrimType dtype, Operand &src, PrimType stype,
                                       AArch64isa::MemoryOrdering memOrd, bool isDirect)
{
    DEBUG_ASSERT(dest.GetKind() == Operand::kOpdMem, "Just checking");

    uint32 dsize = isDirect ? dest.GetSize() : GetPrimTypeBitSize(stype);
    MOperator mOp = PickStInsn(dsize, stype, memOrd);

    Operand *newDest = &dest;
    MemOperand *memOpnd = static_cast<MemOperand *>(&dest);
    OfstOperand *immOpnd = memOpnd->GetOffsetImmediate();
    int32 offset = static_cast<int32>(immOpnd->GetOffsetValue());
    RegOperand *origBaseReg = memOpnd->GetBaseRegister();
    if (offset != 0) {
        RegOperand &resOpnd = CreateRegisterOperandOfType(PTY_i64);
        DEBUG_ASSERT(origBaseReg != nullptr, "nullptr check");
        SelectAdd(resOpnd, *origBaseReg, *immOpnd, PTY_i64);
        newDest = &CreateReplacementMemOperand(dsize, resOpnd, 0);
    }

    std::string key;
    if (isDirect && GetCG()->GenerateVerboseCG()) {
        key = GenerateMemOpndVerbose(dest);
    }

    /* Check if the right store-release instruction is available. */
    if (mOp != MOP_undef) {
        Insn &insn = GetInsnBuilder()->BuildInsn(mOp, src, *newDest);
        if (isDirect && GetCG()->GenerateVerboseCG()) {
            insn.SetComment(key);
        }
        GetCurBB()->AppendInsn(insn);
    } else {
        /* Use a floating-point move(fmov) followed by a stlr.  */
        DEBUG_ASSERT(IsPrimitiveFloat(stype), "must be float type");
        CHECK_FATAL(stype == dtype, "Just checking");
        PrimType itype = (stype == PTY_f32) ? PTY_i32 : PTY_i64;
        RegOperand &regOpnd = CreateRegisterOperandOfType(itype);
        mOp = (stype == PTY_f32) ? MOP_xvmovrs : MOP_xvmovrd;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, regOpnd, src));
        Insn &insn = GetInsnBuilder()->BuildInsn(PickStInsn(dsize, itype, memOrd), regOpnd, *newDest);
        if (isDirect && GetCG()->GenerateVerboseCG()) {
            insn.SetComment(key);
        }
        GetCurBB()->AppendInsn(insn);
    }
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

std::string AArch64CGFunc::GenerateMemOpndVerbose(const Operand &src) const
{
    DEBUG_ASSERT(src.GetKind() == Operand::kOpdMem, "Just checking");
    const MIRSymbol *symSecond = static_cast<const MemOperand *>(&src)->GetSymbol();
    if (symSecond != nullptr) {
        std::string key;
        MIRStorageClass sc = symSecond->GetStorageClass();
        if (sc == kScFormal) {
            key = "param: ";
        } else if (sc == kScAuto) {
            key = "local var: ";
        } else {
            key = "global: ";
        }
        key += symSecond->GetName();
        return key;
    }
    return "";
}

void AArch64CGFunc::SelectCopyMemOpnd(Operand &dest, PrimType dtype, uint32 dsize, Operand &src, PrimType stype)
{
    AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone;
    const MIRSymbol *sym = static_cast<MemOperand *>(&src)->GetSymbol();
    if ((sym != nullptr) && (sym->GetStorageClass() == kScGlobal) && sym->GetAttr(ATTR_memory_order_acquire)) {
        memOrd = AArch64isa::kMoAcquire;
    }

    if (memOrd != AArch64isa::kMoNone) {
        AArch64CGFunc::SelectLoadAcquire(dest, dtype, src, stype, memOrd, true);
        return;
    }
    Insn *insn = nullptr;
    uint32 ssize = src.GetSize();
    PrimType regTy = PTY_void;
    RegOperand *loadReg = nullptr;
    MOperator mop = MOP_undef;
    if (IsPrimitiveFloat(stype) || IsPrimitiveVector(stype)) {
        CHECK_FATAL(dsize == ssize, "dsize %u expect equals ssize %u", dtype, ssize);
        insn = &GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, stype), dest, src);
    } else {
        if (stype == PTY_agg && dtype == PTY_agg) {
            mop = MOP_undef;
        } else {
            mop = PickExtInsn(dtype, stype);
        }
        if (ssize == (GetPrimTypeSize(dtype) * kBitsPerByte) || mop == MOP_undef) {
            insn = &GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, stype), dest, src);
        } else {
            regTy = dsize == k64BitSize ? dtype : PTY_i32;
            loadReg = &CreateRegisterOperandOfType(regTy);
            insn = &GetInsnBuilder()->BuildInsn(PickLdInsn(ssize, stype), *loadReg, src);
        }
    }

    if (GetCG()->GenerateVerboseCG()) {
        insn->SetComment(GenerateMemOpndVerbose(src));
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

void AArch64CGFunc::SplitMovImmOpndInstruction(int64 immVal, RegOperand &destReg, Insn *curInsn)
{
    bool useMovz = BetterUseMOVZ(immVal);
    bool useMovk = false;
    /* get lower 32 bits of the immediate */
    uint64 chunkLval = static_cast<uint64>(immVal) & 0xFFFFFFFFULL;
    /* get upper 32 bits of the immediate */
    uint64 chunkHval = (static_cast<uint64>(immVal) >> k32BitSize) & 0xFFFFFFFFULL;
    int32 maxLoopTime = 4;

    if (chunkLval == chunkHval) {
        /* compute lower 32 bits, and then copy to higher 32 bits, so only 2 chunks need be processed */
        maxLoopTime = 2;
    }

    uint64 sa = 0;
    auto *bb = (curInsn != nullptr) ? curInsn->GetBB() : GetCurBB();
    for (int64 i = 0; i < maxLoopTime; ++i, sa += k16BitSize) {
        /* create an imm opereand which represents the i-th 16-bit chunk of the immediate */
        uint64 chunkVal = (static_cast<uint64>(immVal) >> sa) & 0x0000FFFFULL;
        if (useMovz ? (chunkVal == 0) : (chunkVal == 0x0000FFFFULL)) {
            continue;
        }
        ImmOperand &src16 = CreateImmOperand(static_cast<int64>(chunkVal), k16BitSize, false);
        BitShiftOperand *lslOpnd = GetLogicalShiftLeftOperand(sa, true);
        Insn *newInsn = nullptr;
        if (!useMovk) {
            /* use movz or movn */
            if (!useMovz) {
                src16.BitwiseNegate();
            }
            MOperator mOpCode = useMovz ? MOP_xmovzri16 : MOP_xmovnri16;
            newInsn = &GetInsnBuilder()->BuildInsn(mOpCode, destReg, src16, *lslOpnd);
            useMovk = true;
        } else {
            newInsn = &GetInsnBuilder()->BuildInsn(MOP_xmovkri16, destReg, src16, *lslOpnd);
        }
        if (curInsn != nullptr) {
            bb->InsertInsnBefore(*curInsn, *newInsn);
        } else {
            bb->AppendInsn(*newInsn);
        }
    }

    if (maxLoopTime == 2) {  // compute lower 32 bits, and copy to higher 32 bits, so only 2 chunks need be processed
        /* copy lower 32 bits to higher 32 bits */
        ImmOperand &immOpnd = CreateImmOperand(k32BitSize, k8BitSize, false);
        Insn &insn = GetInsnBuilder()->BuildInsn(MOP_xbfirri6i6, destReg, destReg, immOpnd, immOpnd);
        if (curInsn != nullptr) {
            bb->InsertInsnBefore(*curInsn, insn);
        } else {
            bb->AppendInsn(insn);
        }
    }
}

void AArch64CGFunc::SelectCopyRegOpnd(Operand &dest, PrimType dtype, Operand::OperandType opndType, uint32 dsize,
                                      Operand &src, PrimType stype)
{
    if (opndType != Operand::kOpdMem) {
        if (!CGOptions::IsArm64ilp32()) {
            DEBUG_ASSERT(stype != PTY_a32, "");
        }
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickMovBetweenRegs(dtype, stype), dest, src));
        return;
    }
    AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone;
    const MIRSymbol *sym = static_cast<MemOperand *>(&dest)->GetSymbol();
    if ((sym != nullptr) && (sym->GetStorageClass() == kScGlobal) && sym->GetAttr(ATTR_memory_order_release)) {
        memOrd = AArch64isa::kMoRelease;
    }

    if (memOrd != AArch64isa::kMoNone) {
        AArch64CGFunc::SelectStoreRelease(dest, dtype, src, stype, memOrd, true);
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
    if (!GetMirModule().IsCModule()) {
        isInRange = IsImmediateValueInRange(strMop, immVal, is64Bits, isIntactIndexed, isPostIndexed, isPreIndexed);
    } else {
        isInRange = IsOperandImmValid(strMop, memOpnd, kInsnSecondOpnd);
    }
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
            if (opnd0Type == Operand::kOpdRegister && IsPrimitiveVector(stype)) {
                /* check vector reg to vector reg move */
                CHECK_FATAL(IsPrimitiveVector(dtype), "invalid vectreg to vectreg move");
                MOperator mop = (dsize <= k64BitSize) ? MOP_vmovuu : MOP_vmovvv;
                VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
                vInsn.AddOpndChain(dest).AddOpndChain(src);
                auto *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(dsize >> k3ByteSize, k8BitSize);
                auto *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(dsize >> k3ByteSize, k8BitSize);
                vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpecSrc);
                GetCurBB()->AppendInsn(vInsn);
                break;
            }
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

/*
 * When immediate of str/ldr is over 256bits, it should be aligned according to the reg byte size.
 * Here we split the offset into (512 * n) and +/-(new Offset) when misaligned, to make sure that
 * the new offet is always under 256 bits.
 */
MemOperand &AArch64CGFunc::ConstraintOffsetToSafeRegion(uint32 bitLen, const MemOperand &memOpnd,
                                                        const MIRSymbol *symbol)
{
    auto it = hashMemOpndTable.find(memOpnd);
    if (it != hashMemOpndTable.end()) {
        hashMemOpndTable.erase(memOpnd);
    }
    MemOperand::AArch64AddressingMode addrMode = memOpnd.GetAddrMode();
    int32 offsetValue = static_cast<int32>(memOpnd.GetOffsetImmediate()->GetOffsetValue());
    RegOperand *baseReg = memOpnd.GetBaseRegister();
    RegOperand *resOpnd = GetBaseRegForSplit(kRinvalid);
    MemOperand *newMemOpnd = nullptr;
    if (addrMode == MemOperand::kAddrModeBOi) {
        int32 val256 = k256BitSizeInt; /* const val is unsigned */
        int32 val512 = k512BitSizeInt;
        int32 multiplier = (offsetValue / val512) + static_cast<int32>(offsetValue % val512 > val256);
        int32 addMount = multiplier * val512;
        int32 newOffset = offsetValue - addMount;
        ImmOperand &immAddMount = CreateImmOperand(addMount, k64BitSize, true);
        if (memOpnd.GetOffsetImmediate()->GetVary() == kUnAdjustVary) {
            immAddMount.SetVary(kUnAdjustVary);
        }
        SelectAdd(*resOpnd, *baseReg, immAddMount, PTY_i64);
        newMemOpnd = &CreateReplacementMemOperand(bitLen, *resOpnd, newOffset);
    } else if (addrMode == MemOperand::kAddrModeLo12Li) {
        CHECK_FATAL(symbol != nullptr, "must have symbol");
        StImmOperand &stImmOpnd = CreateStImmOperand(*symbol, offsetValue, 0);
        SelectAdd(*resOpnd, *baseReg, stImmOpnd, PTY_i64);
        newMemOpnd = &CreateReplacementMemOperand(bitLen, *resOpnd, 0);
    }
    CHECK_FATAL(newMemOpnd != nullptr, "create memOpnd failed");
    newMemOpnd->SetStackMem(memOpnd.IsStackMem());
    return *newMemOpnd;
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

/* Extract the address of memOpnd.
 *  1. memcpy need address from memOpnd
 *  2. Used for SelectDassign when do optimization for volatile store, because the stlr instruction only allow
 *     store to the memory addrress with the register base offset 0.
 *     STLR <Wt>, [<Xn|SP>{,#0}], 32-bit variant (size = 10)
 *     STLR <Xt>, [<Xn|SP>{,#0}], 64-bit variant (size = 11)
 */
RegOperand *AArch64CGFunc::ExtractMemBaseAddr(const MemOperand &memOpnd)
{
    const MIRSymbol *sym = memOpnd.GetSymbol();
    MemOperand::AArch64AddressingMode mode = memOpnd.GetAddrMode();
    if (mode == MemOperand::kAddrModeLiteral) {
        return nullptr;
    }
    RegOperand *baseOpnd = memOpnd.GetBaseRegister();
    OfstOperand *offsetOpnd = memOpnd.GetOffsetImmediate();
    int64 offset = (offsetOpnd == nullptr ? 0 : offsetOpnd->GetOffsetValue());
    DEBUG_ASSERT(baseOpnd != nullptr, "nullptr check");
    RegOperand &resultOpnd =
        CreateRegisterOperandOfType(baseOpnd->GetRegisterType(), baseOpnd->GetSize() / kBitsPerByte);
    bool is64Bits = (baseOpnd->GetSize() == k64BitSize);
    if (mode == MemOperand::kAddrModeLo12Li) {
        StImmOperand &stImm = CreateStImmOperand(*sym, offset, 0);
        Insn &addInsn = GetInsnBuilder()->BuildInsn(MOP_xadrpl12, resultOpnd, *baseOpnd, stImm);
        addInsn.SetComment("new add insn");
        GetCurBB()->AppendInsn(addInsn);
    } else if (mode == MemOperand::kAddrModeBOi) {
        if (offsetOpnd->GetOffsetValue() != 0) {
            MOperator mOp = is64Bits ? MOP_xaddrri12 : MOP_waddrri12;
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resultOpnd, *baseOpnd, *offsetOpnd));
        } else {
            return baseOpnd;
        }
    } else {
        CHECK_FATAL(mode == MemOperand::kAddrModeBOrX, "unexpect addressing mode.");
        RegOperand *regOpnd = static_cast<const MemOperand *>(&memOpnd)->GetIndexRegister();
        MOperator mOp = is64Bits ? MOP_xaddrrr : MOP_waddrrr;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resultOpnd, *baseOpnd, *regOpnd));
    }
    return &resultOpnd;
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
    if (fieldId != 0) {
        MIRStructType *structType = static_cast<MIRStructType *>(symbol->GetType());
        DEBUG_ASSERT(structType != nullptr, "SelectDassign: non-zero fieldID for non-structure");
        offset = GetBecommon().GetFieldOffset(*structType, fieldId).first;
        parmCopy = IsParamStructCopy(*symbol);
    }
    uint32 regSize = GetPrimTypeBitSize(rhsPType);
    MIRType *type = symbol->GetType();
    Operand &stOpnd = LoadIntoRegister(opnd0, IsPrimitiveInteger(rhsPType) || IsPrimitiveVectorInteger(rhsPType),
                                       regSize, IsSignedInteger(type->GetPrimType()));
    MOperator mOp = MOP_undef;
    if ((type->GetKind() == kTypeStruct) || (type->GetKind() == kTypeUnion)) {
        MIRStructType *structType = static_cast<MIRStructType *>(type);
        type = structType->GetFieldType(fieldId);
    } else if (type->GetKind() == kTypeClass) {
        MIRClassType *classType = static_cast<MIRClassType *>(type);
        type = classType->GetFieldType(fieldId);
    }

    uint32 dataSize = GetPrimTypeBitSize(type->GetPrimType());
    if (type->GetPrimType() == PTY_agg) {
        dataSize = GetPrimTypeBitSize(PTY_a64);
    }
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
                  (type->GetKind() == kTypeFunction) || (type->GetKind() == kTypeStruct) ||
                  (type->GetKind() == kTypeUnion) || (type->GetKind() == kTypeArray)),
                 "NYI dassign type");
    PrimType ptyp = type->GetPrimType();
    if (ptyp == PTY_agg) {
        ptyp = PTY_a64;
    }

    AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone;
    if (isVolStore) {
        RegOperand *baseOpnd = ExtractMemBaseAddr(*memOpnd);
        if (baseOpnd != nullptr) {
            memOpnd = &CreateMemOpnd(*baseOpnd, 0, dataSize);
            memOrd = AArch64isa::kMoRelease;
            isVolStore = false;
        }
    }

    memOpnd =
        memOpnd->IsOffsetMisaligned(dataSize) ? &ConstraintOffsetToSafeRegion(dataSize, *memOpnd, symbol) : memOpnd;
    if (symbol->GetAsmAttr() != UStrIdx(0) && symbol->GetStorageClass() != kScPstatic &&
        symbol->GetStorageClass() != kScFstatic) {
        std::string regDesp = GlobalTables::GetUStrTable().GetStringFromStrIdx(symbol->GetAsmAttr());
        RegOperand &specifiedOpnd = GetOrCreatePhysicalRegisterOperand(regDesp);
        SelectCopy(specifiedOpnd, type->GetPrimType(), opnd0, rhsPType);
    } else if (memOrd == AArch64isa::kMoNone) {
        mOp = PickStInsn(GetPrimTypeBitSize(ptyp), ptyp);
        Insn &insn = GetInsnBuilder()->BuildInsn(mOp, stOpnd, *memOpnd);
        if (GetCG()->GenerateVerboseCG()) {
            insn.SetComment(GenerateMemOpndVerbose(*memOpnd));
        }
        GetCurBB()->AppendInsn(insn);
    } else {
        AArch64CGFunc::SelectStoreRelease(*memOpnd, ptyp, stOpnd, ptyp, memOrd, true);
    }
}

void AArch64CGFunc::SelectDassignoff(DassignoffNode &stmt, Operand &opnd0)
{
    MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(stmt.stIdx);
    int64 offset = stmt.offset;
    uint32 size = GetPrimTypeSize(stmt.GetPrimType()) * k8ByteSize;
    MOperator mOp = (size == k16BitSize)
                        ? MOP_wstrh
                        : ((size == k32BitSize) ? MOP_wstr : ((size == k64BitSize) ? MOP_xstr : MOP_undef));
    CHECK_FATAL(mOp != MOP_undef, "illegal size for dassignoff");
    MemOperand *memOpnd = &GetOrCreateMemOpnd(*symbol, offset, size);
    if ((memOpnd->GetMemVaryType() == kNotVary) &&
        (IsImmediateOffsetOutOfRange(*memOpnd, size) || (offset % k8BitSize != 0))) {
        memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, size);
    }
    Operand &stOpnd = LoadIntoRegister(opnd0, true, size, false);
    memOpnd = memOpnd->IsOffsetMisaligned(size) ? &ConstraintOffsetToSafeRegion(size, *memOpnd, symbol) : memOpnd;
    Insn &insn = GetInsnBuilder()->BuildInsn(mOp, stOpnd, *memOpnd);
    GetCurBB()->AppendInsn(insn);
}

void AArch64CGFunc::SelectAssertNull(UnaryStmtNode &stmt)
{
    Operand *opnd0 = HandleExpr(stmt, *stmt.Opnd(0));
    RegOperand &baseReg = LoadIntoRegister(*opnd0, PTY_a64);
    auto &zwr = GetZeroOpnd(k32BitSize);
    auto &mem = CreateMemOpnd(baseReg, 0, k32BitSize);
    Insn &loadRef = GetInsnBuilder()->BuildInsn(MOP_wldr, zwr, mem);
    loadRef.SetDoNotRemove(true);
    if (GetCG()->GenerateVerboseCG()) {
        loadRef.SetComment("null pointer check");
    }
    GetCurBB()->AppendInsn(loadRef);
}

void AArch64CGFunc::SelectAbort()
{
    RegOperand &inOpnd = GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
    auto &mem = CreateMemOpnd(inOpnd, 0, k64BitSize);
    Insn &movXzr = GetInsnBuilder()->BuildInsn(MOP_xmovri64, inOpnd, CreateImmOperand(0, k64BitSize, false));
    Insn &loadRef = GetInsnBuilder()->BuildInsn(MOP_wldr, GetZeroOpnd(k64BitSize), mem);
    loadRef.SetDoNotRemove(true);
    movXzr.SetDoNotRemove(true);
    GetCurBB()->AppendInsn(movXzr);
    GetCurBB()->AppendInsn(loadRef);
    SetCurBBKind(BB::kBBReturn);
    GetExitBBsVec().emplace_back(GetCurBB());
}

static std::string GetRegPrefixFromPrimType(PrimType pType, uint32 size, const std::string &constraint)
{
    std::string regPrefix = "";
    /* memory access check */
    if (constraint.find("m") != std::string::npos || constraint.find("Q") != std::string::npos) {
        regPrefix += "[";
    }
    if (IsPrimitiveVector(pType)) {
        regPrefix += "v";
    } else if (IsPrimitiveInteger(pType)) {
        if (size == k32BitSize) {
            regPrefix += "w";
        } else {
            regPrefix += "x";
        }
    } else {
        if (size == k32BitSize) {
            regPrefix += "s";
        } else {
            regPrefix += "d";
        }
    }
    return regPrefix;
}

void AArch64CGFunc::SelectAsm(AsmNode &node)
{
    SetHasAsm();
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
        if (GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
            CHECK_FATAL(false, "NIY, lsra unsupported inline asm!");
        }
    }
    Operand *asmString = &CreateStringOperand(node.asmString);
    ListOperand *listInputOpnd = CreateListOpnd(*GetFuncScopeAllocator());
    ListOperand *listOutputOpnd = CreateListOpnd(*GetFuncScopeAllocator());
    ListOperand *listClobber = CreateListOpnd(*GetFuncScopeAllocator());
    ListConstraintOperand *listInConstraint = memPool->New<ListConstraintOperand>(*GetFuncScopeAllocator());
    ListConstraintOperand *listOutConstraint = memPool->New<ListConstraintOperand>(*GetFuncScopeAllocator());
    ListConstraintOperand *listInRegPrefix = memPool->New<ListConstraintOperand>(*GetFuncScopeAllocator());
    ListConstraintOperand *listOutRegPrefix = memPool->New<ListConstraintOperand>(*GetFuncScopeAllocator());
    std::list<std::pair<Operand *, PrimType>> rPlusOpnd;
    bool noReplacement = false;
    if (node.asmString.find('$') == std::string::npos) {
        /* no replacements */
        noReplacement = true;
    }
    /* input constraints should be processed before OP_asm instruction */
    for (size_t i = 0; i < node.numOpnds; ++i) {
        /* process input constraint */
        std::string str = GlobalTables::GetUStrTable().GetStringFromStrIdx(node.inputConstraints[i]);
        bool isOutputTempNode = false;
        if (str[0] == '+') {
            isOutputTempNode = true;
        }
        listInConstraint->stringList.push_back(static_cast<StringOperand *>(&CreateStringOperand(str)));
        /* process input operands */
        switch (node.Opnd(i)->op) {
            case OP_dread: {
                DreadNode &dread = static_cast<DreadNode &>(*node.Opnd(i));
                Operand *inOpnd = SelectDread(node, dread);
                PrimType pType = dread.GetPrimType();
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }
            case OP_addrof: {
                auto &addrofNode = static_cast<AddrofNode &>(*node.Opnd(i));
                Operand *inOpnd = SelectAddrof(addrofNode, node);
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                PrimType pType = addrofNode.GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }
            case OP_addrofoff: {
                auto &addrofoffNode = static_cast<AddrofoffNode &>(*node.Opnd(i));
                Operand *inOpnd = SelectAddrofoff(addrofoffNode, node);
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                PrimType pType = addrofoffNode.GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }
            case OP_ireadoff: {
                auto *ireadoff = static_cast<IreadoffNode *>(node.Opnd(i));
                Operand *inOpnd = SelectIreadoff(node, *ireadoff);
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                PrimType pType = ireadoff->GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }
            case OP_ireadfpoff: {
                auto *ireadfpoff = static_cast<IreadFPoffNode *>(node.Opnd(i));
                Operand *inOpnd = SelectIreadfpoff(node, *ireadfpoff);
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                PrimType pType = ireadfpoff->GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }
            case OP_iread: {
                auto *iread = static_cast<IreadNode *>(node.Opnd(i));
                Operand *inOpnd = SelectIread(node, *iread);
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                PrimType pType = iread->GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }
            case OP_add: {
                auto *addNode = static_cast<BinaryNode *>(node.Opnd(i));
                Operand *inOpnd = SelectAdd(*addNode, *HandleExpr(*addNode, *addNode->Opnd(0)),
                                            *HandleExpr(*addNode, *addNode->Opnd(1)), node);
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(*inOpnd));
                PrimType pType = addNode->GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd->GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(inOpnd, pType));
                }
                break;
            }

            case OP_constval: {
                CHECK_FATAL(!isOutputTempNode, "Unexpect");
                auto &constNode = static_cast<ConstvalNode &>(*node.Opnd(i));
                CHECK_FATAL(constNode.GetConstVal()->GetKind() == kConstInt,
                            "expect MIRIntConst does not support float yet");
                MIRIntConst *mirIntConst = safe_cast<MIRIntConst>(constNode.GetConstVal());
                CHECK_FATAL(mirIntConst != nullptr, "just checking");
                int64 scale = mirIntConst->GetExtValue();
                if (str.find("r") != std::string::npos) {
                    bool isSigned = scale < 0;
                    ImmOperand &immOpnd = CreateImmOperand(scale, k64BitSize, isSigned);
                    /* set default type as a 64 bit reg */
                    PrimType pty = isSigned ? PTY_i64 : PTY_u64;
                    auto &tempReg = static_cast<Operand &>(CreateRegisterOperandOfType(pty));
                    SelectCopy(tempReg, pty, immOpnd, isSigned ? PTY_i64 : PTY_u64);
                    listInputOpnd->PushOpnd(static_cast<RegOperand &>(tempReg));
                    listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                        &CreateStringOperand(GetRegPrefixFromPrimType(pty, tempReg.GetSize(), str))));
                } else {
                    RegOperand &inOpnd = GetOrCreatePhysicalRegisterOperand(RZR, k64BitSize, kRegTyInt);
                    listInputOpnd->PushOpnd(static_cast<RegOperand &>(inOpnd));

                    listInRegPrefix->stringList.push_back(
                        static_cast<StringOperand *>(&CreateStringOperand("i" + std::to_string(scale))));
                }
                break;
            }
            case OP_regread: {
                auto &regreadNode = static_cast<RegreadNode &>(*node.Opnd(i));
                PregIdx pregIdx = regreadNode.GetRegIdx();
                RegOperand &inOpnd = GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
                listInputOpnd->PushOpnd(static_cast<RegOperand &>(inOpnd));
                MIRPreg *preg = GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
                PrimType pType = preg->GetPrimType();
                listInRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(pType, inOpnd.GetSize(), str))));
                if (isOutputTempNode) {
                    rPlusOpnd.emplace_back(std::make_pair(&static_cast<Operand &>(inOpnd), pType));
                }
                break;
            }
            default:
                CHECK_FATAL(0, "Inline asm input expression not handled");
        }
    }
    std::vector<Operand *> intrnOpnds;
    intrnOpnds.emplace_back(asmString);
    intrnOpnds.emplace_back(listOutputOpnd);
    intrnOpnds.emplace_back(listClobber);
    intrnOpnds.emplace_back(listInputOpnd);
    intrnOpnds.emplace_back(listOutConstraint);
    intrnOpnds.emplace_back(listInConstraint);
    intrnOpnds.emplace_back(listOutRegPrefix);
    intrnOpnds.emplace_back(listInRegPrefix);
    Insn *asmInsn = &GetInsnBuilder()->BuildInsn(MOP_asm, intrnOpnds);
    GetCurBB()->AppendInsn(*asmInsn);

    /* process listOutputOpnd */
    for (size_t i = 0; i < node.asmOutputs.size(); ++i) {
        bool isOutputTempNode = false;
        RegOperand *rPOpnd = nullptr;
        /* process output constraint */
        std::string str = GlobalTables::GetUStrTable().GetStringFromStrIdx(node.outputConstraints[i]);

        listOutConstraint->stringList.push_back(static_cast<StringOperand *>(&CreateStringOperand(str)));
        if (str[0] == '+') {
            CHECK_FATAL(!rPlusOpnd.empty(), "Need r+ operand");
            rPOpnd = static_cast<RegOperand *>((rPlusOpnd.begin()->first));
            listOutputOpnd->PushOpnd(*rPOpnd);
            listOutRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                &CreateStringOperand(GetRegPrefixFromPrimType(rPlusOpnd.begin()->second, rPOpnd->GetSize(), str))));
            if (!rPlusOpnd.empty()) {
                rPlusOpnd.pop_front();
            }
            isOutputTempNode = true;
        }
        if (str.find("Q") != std::string::npos || str.find("m") != std::string::npos) {
            continue;
        }
        /* process output operands */
        StIdx stIdx = node.asmOutputs[i].first;
        RegFieldPair regFieldPair = node.asmOutputs[i].second;
        if (regFieldPair.IsReg()) {
            PregIdx pregIdx = static_cast<PregIdx>(regFieldPair.GetPregIdx());
            MIRPreg *mirPreg = mirModule.CurFunction()->GetPregTab()->PregFromPregIdx(pregIdx);
            RegOperand *outOpnd = isOutputTempNode
                                      ? rPOpnd
                                      : &GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
            PrimType srcType = mirPreg->GetPrimType();
            PrimType destType = srcType;
            if (GetPrimTypeBitSize(destType) < k32BitSize) {
                destType = IsSignedInteger(destType) ? PTY_i32 : PTY_u32;
            }
            RegType rtype = GetRegTyFromPrimTy(srcType);
            RegOperand &opnd0 = isOutputTempNode
                                    ? GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx))
                                    : CreateVirtualRegisterOperand(NewVReg(rtype, GetPrimTypeSize(srcType)));
            SelectCopy(opnd0, destType, *outOpnd, srcType);
            if (!isOutputTempNode) {
                listOutputOpnd->PushOpnd(static_cast<RegOperand &>(*outOpnd));
                listOutRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                    &CreateStringOperand(GetRegPrefixFromPrimType(srcType, outOpnd->GetSize(), str))));
            }
        } else {
            MIRSymbol *var;
            if (stIdx.IsGlobal()) {
                var = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
            } else {
                var = mirModule.CurFunction()->GetSymbolTabItem(stIdx.Idx());
            }
            CHECK_FATAL(var != nullptr, "var should not be nullptr");
            if (!noReplacement || var->GetAsmAttr() != UStrIdx(0)) {
                RegOperand *outOpnd = nullptr;
                PrimType pty = GlobalTables::GetTypeTable().GetTypeTable().at(var->GetTyIdx())->GetPrimType();
                if (var->GetAsmAttr() != UStrIdx(0)) {
                    std::string regDesp = GlobalTables::GetUStrTable().GetStringFromStrIdx(var->GetAsmAttr());
                    outOpnd = &GetOrCreatePhysicalRegisterOperand(regDesp);
                } else {
                    RegType rtype = GetRegTyFromPrimTy(pty);
                    outOpnd =
                        isOutputTempNode ? rPOpnd : &CreateVirtualRegisterOperand(NewVReg(rtype, GetPrimTypeSize(pty)));
                }
                SaveReturnValueInLocal(node.asmOutputs, i, PTY_a64, *outOpnd, node);
                if (!isOutputTempNode) {
                    listOutputOpnd->PushOpnd(static_cast<RegOperand &>(*outOpnd));
                    listOutRegPrefix->stringList.push_back(static_cast<StringOperand *>(
                        &CreateStringOperand(GetRegPrefixFromPrimType(pty, outOpnd->GetSize(), str))));
                }
            }
        }
    }
    if (noReplacement) {
        return;
    }

    /* process listClobber */
    for (size_t i = 0; i < node.clobberList.size(); ++i) {
        std::string str = GlobalTables::GetUStrTable().GetStringFromStrIdx(node.clobberList[i]);
        auto regno = static_cast<regno_t>(str[1] - '0');
        if (str[2] >= '0' && str[2] <= '9') {  // if third char (index 2) is num, add to regno
            regno = regno * kDecimalMax + static_cast<uint32>((str[2] - '0'));
        }
        RegOperand *reg;
        switch (str[0]) {
            case 'w': {
                reg = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regno + R0), k32BitSize, kRegTyInt);
                listClobber->PushOpnd(*reg);
                break;
            }
            case 'x': {
                reg = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regno + R0), k64BitSize, kRegTyInt);
                listClobber->PushOpnd(*reg);
                break;
            }
            case 's': {
                reg = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regno + V0), k32BitSize, kRegTyFloat);
                listClobber->PushOpnd(*reg);
                break;
            }
            case 'd': {
                reg = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regno + V0), k64BitSize, kRegTyFloat);
                listClobber->PushOpnd(*reg);
                break;
            }
            case 'v': {
                reg = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regno + V0), k64BitSize, kRegTyFloat);
                listClobber->PushOpnd(*reg);
                break;
            }
            case 'c': {
                asmInsn->SetAsmDefCondCode();
                break;
            }
            case 'm': {
                asmInsn->SetAsmModMem();
                break;
            }
            default:
                CHECK_FATAL(0, "Inline asm clobber list not handled");
        }
    }
}

void AArch64CGFunc::SelectRegassign(RegassignNode &stmt, Operand &opnd0)
{
    if (GetCG()->IsLmbc()) {
        PrimType lhsSize = stmt.GetPrimType();
        PrimType rhsSize = stmt.Opnd(0)->GetPrimType();
        if (lhsSize != rhsSize && stmt.Opnd(0)->GetOpCode() == OP_ireadoff) {
            Insn *prev = GetCurBB()->GetLastMachineInsn();
            if (prev && (prev->GetMachineOpcode() == MOP_wldrsb || prev->GetMachineOpcode() == MOP_wldrsh)) {
                opnd0.SetSize(GetPrimTypeBitSize(stmt.GetPrimType()));
                prev->SetMOP(AArch64CG::kMd[prev->GetMachineOpcode() == MOP_wldrsb ? MOP_xldrsb : MOP_xldrsh]);
            } else if (prev && (prev->GetMachineOpcode() == MOP_wldr && stmt.GetPrimType() == PTY_i64)) {
                opnd0.SetSize(GetPrimTypeBitSize(stmt.GetPrimType()));
                prev->SetMOP(AArch64CG::kMd[MOP_xldrsw]);
            }
        }
    }
    RegOperand *regOpnd = nullptr;
    PregIdx pregIdx = stmt.GetRegIdx();
    if (IsSpecialPseudoRegister(pregIdx)) {
        if (GetCG()->IsLmbc() && stmt.GetPrimType() == PTY_agg) {
            if (static_cast<RegOperand &>(opnd0).IsOfIntClass()) {
                regOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, PTY_i64);
            } else if (opnd0.GetSize() <= k4ByteSize) {
                regOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, PTY_f32);
            } else {
                regOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, PTY_f64);
            }
        } else {
            regOpnd = &GetOrCreateSpecialRegisterOperand(-pregIdx, stmt.GetPrimType());
        }
    } else {
        regOpnd = &GetOrCreateVirtualRegisterOperand(GetVirtualRegNOFromPseudoRegIdx(pregIdx));
    }
    /* look at rhs */
    PrimType rhsType = stmt.Opnd(0)->GetPrimType();
    if (GetCG()->IsLmbc() && rhsType == PTY_agg) {
        /* This occurs when a call returns a small struct */
        /* The subtree should already taken care of the agg type that is in excess of 8 bytes */
        rhsType = PTY_i64;
    }
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

MemOperand *AArch64CGFunc::GenFormalMemOpndWithSymbol(const MIRSymbol &sym, int64 offset)
{
    MemOperand *memOpnd = nullptr;
    if (IsParamStructCopy(sym)) {
        memOpnd = &GetOrCreateMemOpnd(sym, 0, k64BitSize);
        RegOperand *vreg = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
        Insn &ldInsn = GetInsnBuilder()->BuildInsn(PickLdInsn(k64BitSize, PTY_i64), *vreg, *memOpnd);
        GetCurBB()->AppendInsn(ldInsn);
        return CreateMemOperand(k64BitSize, *vreg, CreateImmOperand(offset, k32BitSize, false), sym.IsVolatile());
    }
    return &GetOrCreateMemOpnd(sym, offset, k64BitSize);
}

MemOperand *AArch64CGFunc::FixLargeMemOpnd(MemOperand &memOpnd, uint32 align)
{
    MemOperand *lhsMemOpnd = &memOpnd;
    if ((lhsMemOpnd->GetMemVaryType() == kNotVary) && IsImmediateOffsetOutOfRange(*lhsMemOpnd, align * kBitsPerByte)) {
        RegOperand *addReg = &CreateRegisterOperandOfType(PTY_i64);
        lhsMemOpnd = &SplitOffsetWithAddInstruction(*lhsMemOpnd, align * k8BitSize, addReg->GetRegisterNumber());
    }
    return lhsMemOpnd;
}

MemOperand *AArch64CGFunc::FixLargeMemOpnd(MOperator mOp, MemOperand &memOpnd, uint32 dSize, uint32 opndIdx)
{
    auto *a64MemOpnd = &memOpnd;
    if ((a64MemOpnd->GetMemVaryType() == kNotVary) && !IsOperandImmValid(mOp, &memOpnd, opndIdx)) {
        if (opndIdx == kInsnSecondOpnd) {
            a64MemOpnd = &SplitOffsetWithAddInstruction(*a64MemOpnd, dSize);
        } else if (opndIdx == kInsnThirdOpnd) {
            a64MemOpnd =
                &SplitOffsetWithAddInstruction(*a64MemOpnd, dSize, AArch64reg::kRinvalid, false, nullptr, true);
        } else {
            CHECK_FATAL(false, "NYI");
        }
    }
    return a64MemOpnd;
}

MemOperand *AArch64CGFunc::GenLargeAggFormalMemOpnd(const MIRSymbol &sym, uint32 align, int64 offset, bool needLow12)
{
    MemOperand *memOpnd;
    if (sym.GetStorageClass() == kScFormal && GetBecommon().GetTypeSize(sym.GetTyIdx()) > k16ByteSize) {
        /* formal of size of greater than 16 is copied by the caller and the pointer to it is passed. */
        /* otherwise it is passed in register and is accessed directly. */
        memOpnd = &GetOrCreateMemOpnd(sym, 0, align * kBitsPerByte);
        RegOperand *vreg = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
        Insn &ldInsn = GetInsnBuilder()->BuildInsn(PickLdInsn(k64BitSize, PTY_i64), *vreg, *memOpnd);
        GetCurBB()->AppendInsn(ldInsn);
        memOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k64BitSize, vreg, nullptr,
                                      &GetOrCreateOfstOpnd(static_cast<uint64>(offset), k32BitSize), nullptr);
    } else {
        memOpnd = &GetOrCreateMemOpnd(sym, offset, align * kBitsPerByte, false, needLow12);
    }
    return FixLargeMemOpnd(*memOpnd, align);
}

RegOperand *AArch64CGFunc::PrepareMemcpyParamOpnd(bool isLo12, const MIRSymbol &symbol, int64 offsetVal,
                                                  RegOperand &BaseReg)
{
    RegOperand *tgtAddr = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    if (isLo12) {
        StImmOperand &stImm = CreateStImmOperand(symbol, 0, 0);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrpl12, *tgtAddr, BaseReg, stImm));
    } else {
        ImmOperand &imm = CreateImmOperand(offsetVal, k64BitSize, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xaddrri12, *tgtAddr, BaseReg, imm));
    }
    return tgtAddr;
}

RegOperand *AArch64CGFunc::PrepareMemcpyParamOpnd(int64 offset, Operand &exprOpnd)
{
    RegOperand *tgtAddr = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    OfstOperand *ofstOpnd = &GetOrCreateOfstOpnd(static_cast<uint64>(offset), k32BitSize);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xaddrri12, *tgtAddr, exprOpnd, *ofstOpnd));
    return tgtAddr;
}

RegOperand *AArch64CGFunc::PrepareMemcpyParamOpnd(uint64 copySize)
{
    RegOperand *vregMemcpySize = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    ImmOperand *sizeOpnd = &CreateImmOperand(static_cast<int64>(copySize), k64BitSize, false);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wmovri32, *vregMemcpySize, *sizeOpnd));
    return vregMemcpySize;
}

Insn *AArch64CGFunc::AggtStrLdrInsert(bool bothUnion, Insn *lastStrLdr, Insn &newStrLdr)
{
    if (bothUnion) {
        if (lastStrLdr == nullptr) {
            GetCurBB()->AppendInsn(newStrLdr);
        } else {
            GetCurBB()->InsertInsnAfter(*lastStrLdr, newStrLdr);
        }
    } else {
        GetCurBB()->AppendInsn(newStrLdr);
    }
    return &newStrLdr;
}

MemOperand *AArch64CGFunc::SelectRhsMemOpnd(BaseNode &rhsStmt, bool &isRefField)
{
    MemOperand *rhsMemOpnd = nullptr;
    if (rhsStmt.GetOpCode() == OP_dread) {
        MemRWNodeHelper rhsMemHelper(rhsStmt, GetFunction(), GetBecommon());
        auto *rhsSymbol = rhsMemHelper.GetSymbol();
        rhsMemOpnd = GenFormalMemOpndWithSymbol(*rhsSymbol, rhsMemHelper.GetByteOffset());
        isRefField = rhsMemHelper.IsRefField();
    } else {
        DEBUG_ASSERT(rhsStmt.GetOpCode() == OP_iread, "SelectRhsMemOpnd: NYI");
        auto &rhsIread = static_cast<IreadNode &>(rhsStmt);
        MemRWNodeHelper rhsMemHelper(rhsIread, GetFunction(), GetBecommon());
        auto *rhsAddrOpnd = HandleExpr(rhsIread, *rhsIread.Opnd(0));
        auto &rhsAddr = LoadIntoRegister(*rhsAddrOpnd, rhsIread.Opnd(0)->GetPrimType());
        auto &rhsOfstOpnd = CreateImmOperand(rhsMemHelper.GetByteOffset(), k32BitSize, false);
        rhsMemOpnd = CreateMemOperand(k64BitSize, rhsAddr, rhsOfstOpnd, rhsIread.IsVolatile());
        isRefField = rhsMemHelper.IsRefField();
    }
    return rhsMemOpnd;
}

MemOperand *AArch64CGFunc::SelectRhsMemOpnd(BaseNode &rhsStmt)
{
    bool isRefField = false;
    return SelectRhsMemOpnd(rhsStmt, isRefField);
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
    } else if (cc == kGHC) {
        res = memPool->New<GHCCC>(GetBecommon());
    } else {
        CHECK_FATAL(false, "unsupported yet");
    }
    hashCCTable[cc] = res;
    return res;
}
void AArch64CGFunc::SelectAggDassign(DassignNode &stmt)
{
    DEBUG_ASSERT(stmt.Opnd(0) != nullptr, "null ptr check");
    MemRWNodeHelper lhsMemHelper(stmt, GetFunction(), GetBecommon());
    auto *lhsSymbol = lhsMemHelper.GetSymbol();
    auto *lhsMemOpnd = GenFormalMemOpndWithSymbol(*lhsSymbol, lhsMemHelper.GetByteOffset());

    bool isRefField = false;
    auto *rhsMemOpnd = SelectRhsMemOpnd(*stmt.GetRHS(), isRefField);
    SelectMemCopy(*lhsMemOpnd, *rhsMemOpnd, lhsMemHelper.GetMemSize(), isRefField, &stmt, stmt.GetRHS());
}

static MIRType *GetPointedToType(const MIRPtrType &pointerType)
{
    MIRType *aType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType.GetPointedTyIdx());
    if (aType->GetKind() == kTypeArray) {
        MIRArrayType *arrayType = static_cast<MIRArrayType *>(aType);
        return GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
    }
    if (aType->GetKind() == kTypeFArray || aType->GetKind() == kTypeJArray) {
        MIRFarrayType *farrayType = static_cast<MIRFarrayType *>(aType);
        return GlobalTables::GetTypeTable().GetTypeFromTyIdx(farrayType->GetElemTyIdx());
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
    AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone;

    if (stmt.GetFieldID() != 0) {
        MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        MIRStructType *structType = nullptr;
        if (pointedTy->GetKind() != kTypeJArray) {
            structType = static_cast<MIRStructType *>(pointedTy);
        } else {
            /* it's a Jarray type. using it's parent's field info: java.lang.Object */
            structType = static_cast<MIRJarrayType *>(pointedTy)->GetParentType();
        }
        DEBUG_ASSERT(structType != nullptr, "SelectIassign: non-zero fieldID for non-structure");
        pointedType = structType->GetFieldType(stmt.GetFieldID());
        offset = GetBecommon().GetFieldOffset(*structType, stmt.GetFieldID()).first;
        isRefField = GetBecommon().IsRefField(*structType, stmt.GetFieldID());
    } else {
        pointedType = GetPointedToType(*pointerType);
        if (GetFunction().IsJava() && (pointedType->GetKind() == kTypePointer)) {
            MIRType *nextPointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
                static_cast<MIRPtrType *>(pointedType)->GetPointedTyIdx());
            if (nextPointedType->GetKind() != kTypeScalar) {
                isRefField = true; /* write into an object array or a high-dimensional array */
            }
        }
    }

    PrimType styp = stmt.GetRHS()->GetPrimType();
    Operand *valOpnd = HandleExpr(stmt, *stmt.GetRHS());
    Operand &srcOpnd = LoadIntoRegister(*valOpnd, (IsPrimitiveInteger(styp) || IsPrimitiveVectorInteger(styp)),
                                        GetPrimTypeBitSize(styp));

    PrimType destType = pointedType->GetPrimType();
    if (destType == PTY_agg) {
        destType = PTY_a64;
    }
    if (IsPrimitiveVector(styp)) { /* a vector type */
        destType = styp;
    }
    DEBUG_ASSERT(stmt.Opnd(0) != nullptr, "null ptr check");
    MemOperand &memOpnd = CreateMemOpnd(destType, stmt, *stmt.Opnd(0), offset);
    auto dataSize = GetPrimTypeBitSize(destType);
    memOpnd = memOpnd.IsOffsetMisaligned(dataSize) ? ConstraintOffsetToSafeRegion(dataSize, memOpnd, nullptr) : memOpnd;
    if (isVolStore && memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi) {
        memOrd = AArch64isa::kMoRelease;
        isVolStore = false;
    }

    if (memOrd == AArch64isa::kMoNone) {
        SelectCopy(memOpnd, destType, srcOpnd, destType);
    } else {
        AArch64CGFunc::SelectStoreRelease(memOpnd, destType, srcOpnd, destType, memOrd, false);
    }
    if (GetCurBB() && GetCurBB()->GetLastMachineInsn()) {
        GetCurBB()->GetLastMachineInsn()->MarkAsAccessRefField(isRefField);
    }
}

void AArch64CGFunc::SelectIassignoff(IassignoffNode &stmt)
{
    int32 offset = stmt.GetOffset();
    PrimType destType = stmt.GetPrimType();

    MemOperand &memOpnd = CreateMemOpnd(destType, stmt, *stmt.GetBOpnd(0), offset);
    auto dataSize = GetPrimTypeBitSize(destType);
    memOpnd = memOpnd.IsOffsetMisaligned(dataSize) ? ConstraintOffsetToSafeRegion(dataSize, memOpnd, nullptr) : memOpnd;
    Operand *valOpnd = HandleExpr(stmt, *stmt.GetBOpnd(1));
    Operand &srcOpnd = LoadIntoRegister(*valOpnd, true, GetPrimTypeBitSize(destType));
    SelectCopy(memOpnd, destType, srcOpnd, destType);
}

MemOperand *AArch64CGFunc::GenLmbcFpMemOperand(int32 offset, uint32 byteSize, AArch64reg baseRegno)
{
    MemOperand *memOpnd;
    RegOperand *rfp = &GetOrCreatePhysicalRegisterOperand(baseRegno, k64BitSize, kRegTyInt);
    uint32 bitlen = byteSize * kBitsPerByte;
    if (offset < 0 && offset < kNegative256BitSize) {
        RegOperand *baseOpnd = &CreateRegisterOperandOfType(PTY_a64);
        ImmOperand &immOpnd = CreateImmOperand(offset, k32BitSize, true);
        Insn &addInsn = GetInsnBuilder()->BuildInsn(MOP_xaddrri12, *baseOpnd, *rfp, immOpnd);
        GetCurBB()->AppendInsn(addInsn);
        OfstOperand *offsetOpnd = &CreateOfstOpnd(0, k32BitSize);
        memOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, bitlen, baseOpnd, nullptr, offsetOpnd, nullptr);
    } else {
        OfstOperand *offsetOpnd = &CreateOfstOpnd(static_cast<uint64>(static_cast<int64>(offset)), k32BitSize);
        memOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, bitlen, rfp, nullptr, offsetOpnd, nullptr);
    }
    memOpnd->SetStackMem(true);
    return memOpnd;
}

void AArch64CGFunc::SelectIassignfpoff(IassignFPoffNode &stmt, Operand &opnd)
{
    int32 offset = stmt.GetOffset();
    PrimType primType = stmt.GetPrimType();
    MIRType *rType = GetLmbcCallReturnType();
    bool isPureFpStruct = false;
    uint32 numRegs = 0;
    if (rType && rType->GetPrimType() == PTY_agg && opnd.IsRegister() &&
        static_cast<RegOperand &>(opnd).IsPhysicalRegister()) {
        CHECK_FATAL(rType->GetSize() <= k16BitSize, "SelectIassignfpoff invalid agg size");
        uint32 fpSize;
        numRegs = FloatParamRegRequired(static_cast<MIRStructType *>(rType), fpSize);
        if (numRegs) {
            primType = (fpSize == k4ByteSize) ? PTY_f32 : PTY_f64;
            isPureFpStruct = true;
        }
    }
    uint32 byteSize = GetPrimTypeSize(primType);
    uint32 bitlen = byteSize * kBitsPerByte;
    if (isPureFpStruct) {
        for (uint32 i = 0; i < numRegs; ++i) {
            MemOperand *memOpnd = GenLmbcFpMemOperand(offset + static_cast<int32>(i * byteSize), byteSize);
            RegOperand &srcOpnd = GetOrCreatePhysicalRegisterOperand(AArch64reg(V0 + i), bitlen, kRegTyFloat);
            MOperator mOp = PickStInsn(bitlen, primType);
            Insn &store = GetInsnBuilder()->BuildInsn(mOp, srcOpnd, *memOpnd);
            GetCurBB()->AppendInsn(store);
        }
    } else {
        Operand &srcOpnd = LoadIntoRegister(opnd, primType);
        MemOperand *memOpnd = GenLmbcFpMemOperand(offset, byteSize);
        MOperator mOp = PickStInsn(bitlen, primType);
        Insn &store = GetInsnBuilder()->BuildInsn(mOp, srcOpnd, *memOpnd);
        GetCurBB()->AppendInsn(store);
    }
}

/* Load and assign to a new register. To be moved to the correct call register OR stack
   location in LmbcSelectParmList */
void AArch64CGFunc::SelectIassignspoff(PrimType pTy, int32 offset, Operand &opnd)
{
    if (GetLmbcArgInfo() == nullptr) {
        LmbcArgInfo *p = memPool->New<LmbcArgInfo>(*GetFuncScopeAllocator());
        SetLmbcArgInfo(p);
    }
    uint32 byteLen = GetPrimTypeSize(pTy);
    uint32 bitLen = byteLen * kBitsPerByte;
    RegType regTy = GetRegTyFromPrimTy(pTy);
    int32 curRegArgs = GetLmbcArgsInRegs(regTy);
    if (curRegArgs < static_cast<int32>(k8ByteSize)) {
        RegOperand *res = &CreateVirtualRegisterOperand(NewVReg(regTy, byteLen));
        SelectCopy(*res, pTy, opnd, pTy);
        SetLmbcArgInfo(res, pTy, offset, 1);
    } else {
        /* Move into allocated space */
        Operand &memOpd = CreateMemOpnd(RSP, offset, byteLen);
        Operand &reg = LoadIntoRegister(opnd, pTy);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickStInsn(bitLen, pTy), reg, memOpd));
    }
    IncLmbcArgsInRegs(regTy); /* num of args in registers */
    IncLmbcTotalArgs();       /* num of args */
}

/* Search for CALL/ICALL/ICALLPROTO node, must be called from a blkassignoff node */
MIRType *AArch64CGFunc::LmbcGetAggTyFromCallSite(StmtNode *stmt, std::vector<TyIdx> **parmList) const
{
    for (; stmt != nullptr; stmt = stmt->GetNext()) {
        if (stmt->GetOpCode() == OP_call || stmt->GetOpCode() == OP_icallproto) {
            break;
        }
    }
    CHECK_FATAL(stmt && (stmt->GetOpCode() == OP_call || stmt->GetOpCode() == OP_icallproto),
                "blkassign sp not followed by call");
    uint32 nargs = GetLmbcTotalArgs();
    MIRType *ty = nullptr;
    if (stmt->GetOpCode() == OP_call) {
        CallNode *callNode = static_cast<CallNode *>(stmt);
        MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
        if (fn->GetFormalCount() > 0) {
            ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fn->GetFormalDefVec()[nargs].formalTyIdx);
        }
        *parmList = &fn->GetParamTypes();
        // would return null if the actual parameter is bogus
    } else if (stmt->GetOpCode() == OP_icallproto) {
        IcallNode *icallproto = static_cast<IcallNode *>(stmt);
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(icallproto->GetRetTyIdx());
        MIRFuncType *fType = static_cast<MIRFuncType *>(type);
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fType->GetNthParamType(nargs));
        *parmList = &fType->GetParamTypeList();
    } else {
        CHECK_FATAL(stmt->GetOpCode() == OP_icallproto, "LmbcGetAggTyFromCallSite:: unexpected call operator");
    }
    return ty;
}

/* return true if blkassignoff for return, false otherwise */
bool AArch64CGFunc::LmbcSmallAggForRet(const BaseNode &bNode, const Operand *src, int32 offset, bool skip1)
{
    PrimType pTy;
    uint32 size = 0;
    AArch64reg regno = static_cast<AArch64reg>(static_cast<const RegOperand *>(src)->GetRegisterNumber());
    MIRFunction *func = &GetFunction();

    if (!func->IsReturnStruct()) {
        return false;
    }
    /* This blkassignoff is for struct return? */
    uint32 loadSize;
    uint32 numRegs = 0;
    if (static_cast<const StmtNode &>(bNode).GetNext()->GetOpCode() == OP_return) {
        MIRStructType *ty = static_cast<MIRStructType *>(func->GetReturnType());
        uint32 tySize = GetBecommon().GetTypeSize(ty->GetTypeIndex());
        uint32 fpregs = FloatParamRegRequired(ty, size);
        if (fpregs > 0) {
            /* pure floating point in agg */
            numRegs = fpregs;
            pTy = (size == k4ByteSize) ? PTY_f32 : PTY_f64;
            loadSize = GetPrimTypeSize(pTy) * kBitsPerByte;
            for (uint32 i = 0; i < fpregs; i++) {
                int32 s = (i == 0) ? 0 : static_cast<int32>(i * size);
                int64 newOffset = static_cast<int64>(s) + static_cast<int64>(offset);
                MemOperand &mem = CreateMemOpnd(regno, newOffset, size * kBitsPerByte);
                AArch64reg reg = static_cast<AArch64reg>(V0 + i);
                RegOperand *res = &GetOrCreatePhysicalRegisterOperand(reg, loadSize, kRegTyFloat);
                SelectCopy(*res, pTy, mem, pTy);
            }
        } else {
            constexpr uint8 numRegMax = 2;
            /* int/float mixed */
            numRegs = numRegMax;
            pTy = PTY_i64;
            constexpr uint32 oneByte = 1;
            constexpr uint32 twoByte = 2;
            constexpr uint32 fourByte = 4;
            size = k4ByteSize;
            switch (tySize) {
                case oneByte:
                    pTy = PTY_i8;
                    break;
                case twoByte:
                    pTy = PTY_i16;
                    break;
                case fourByte:
                    pTy = PTY_i32;
                    break;
                default:
                    size = k8ByteSize; /* pTy remains i64 */
                    break;
            }
            loadSize = GetPrimTypeSize(pTy) * kBitsPerByte;
            if (!skip1) {
                MemOperand &mem = CreateMemOpnd(regno, offset, size * kBitsPerByte);
                RegOperand &res1 = GetOrCreatePhysicalRegisterOperand(R0, loadSize, kRegTyInt);
                SelectCopy(res1, pTy, mem, pTy);
            }
            if (tySize > static_cast<int32>(k8ByteSize)) {
                int32 newOffset = offset + static_cast<int32>(k8ByteSize);
                MemOperand &newMem = CreateMemOpnd(regno, newOffset, size * kBitsPerByte);
                RegOperand &res2 = GetOrCreatePhysicalRegisterOperand(R1, loadSize, kRegTyInt);
                SelectCopy(res2, pTy, newMem, pTy);
            }
        }
        bool intReg = fpregs == 0;
        for (uint32 i = 0; i < numRegs; i++) {
            AArch64reg preg = static_cast<AArch64reg>((intReg ? R0 : V0) + i);
            MOperator mop = intReg ? MOP_pseudo_ret_int : MOP_pseudo_ret_float;
            RegOperand &dest = GetOrCreatePhysicalRegisterOperand(preg, loadSize, intReg ? kRegTyInt : kRegTyFloat);
            Insn &pseudo = GetInsnBuilder()->BuildInsn(mop, dest);
            GetCurBB()->AppendInsn(pseudo);
        }
        return true;
    }
    return false;
}

/* return true if blkassignoff for return, false otherwise */
bool AArch64CGFunc::LmbcSmallAggForCall(BlkassignoffNode &bNode, const Operand *src, std::vector<TyIdx> **parmList)
{
    AArch64reg regno = static_cast<AArch64reg>(static_cast<const RegOperand *>(src)->GetRegisterNumber());
    if (IsBlkassignForPush(bNode)) {
        PrimType pTy = PTY_i64;
        MIRStructType *ty = static_cast<MIRStructType *>(LmbcGetAggTyFromCallSite(&bNode, parmList));
        uint32 size = 0;
        uint32 fpregs = ty ? FloatParamRegRequired(ty, size) : 0; /* fp size determined */
        if (fpregs > 0) {
            /* pure floating point in agg */
            pTy = (size == k4ByteSize) ? PTY_f32 : PTY_f64;
            for (uint32 i = 0; i < fpregs; i++) {
                int32 s = (i == 0) ? 0 : static_cast<int32>(i * size);
                MemOperand &mem = CreateMemOpnd(regno, s, size * kBitsPerByte);
                RegOperand *res = &CreateVirtualRegisterOperand(NewVReg(kRegTyFloat, size));
                SelectCopy(*res, pTy, mem, pTy);
                SetLmbcArgInfo(res, pTy, 0, static_cast<int32>(fpregs));
                IncLmbcArgsInRegs(kRegTyFloat);
            }
            IncLmbcTotalArgs();
            return true;
        } else if (bNode.blockSize <= static_cast<int32>(k16ByteSize)) {
            /* integer/mixed types in register/s */
            size = k4ByteSize;
            switch (bNode.blockSize) {
                case k1ByteSize:
                    pTy = PTY_i8;
                    break;
                case k2ByteSize:
                    pTy = PTY_i16;
                    break;
                case k4ByteSize:
                    pTy = PTY_i32;
                    break;
                default:
                    size = k8ByteSize; /* pTy remains i64 */
                    break;
            }
            MemOperand &mem = CreateMemOpnd(regno, 0, size * kBitsPerByte);
            RegOperand *res = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, size));
            SelectCopy(*res, pTy, mem, pTy);
            SetLmbcArgInfo(res, pTy, bNode.offset,
                           bNode.blockSize > static_cast<int32>(k8ByteSize) ? kThirdReg : kSecondReg);
            IncLmbcArgsInRegs(kRegTyInt);
            if (bNode.blockSize > static_cast<int32>(k8ByteSize)) {
                MemOperand &newMem = CreateMemOpnd(regno, k8ByteSize, size * kBitsPerByte);
                RegOperand *newRes = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, size));
                SelectCopy(*newRes, pTy, newMem, pTy);
                SetLmbcArgInfo(newRes, pTy, bNode.offset + k8ByteSizeInt, kThirdReg);
                IncLmbcArgsInRegs(kRegTyInt);
            }
            IncLmbcTotalArgs();
            return true;
        }
    }
    return false;
}

/* This function is incomplete and may be removed when Lmbc IR is changed
   to have the lowerer figures out the address of the large agg to reside */
uint32 AArch64CGFunc::LmbcFindTotalStkUsed(std::vector<TyIdx> *paramList)
{
    AArch64CallConvImpl parmlocator(GetBecommon());
    CCLocInfo pLoc;
    for (TyIdx tyIdx : *paramList) {
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
        (void)parmlocator.LocateNextParm(*ty, pLoc);
    }
    return 0;
}

/* All arguments passed as registers */
uint32 AArch64CGFunc::LmbcTotalRegsUsed()
{
    if (GetLmbcArgInfo() == nullptr) {
        return 0; /* no arg */
    }
    MapleVector<int32> &regs = GetLmbcCallArgNumOfRegs();
    MapleVector<PrimType> &types = GetLmbcCallArgTypes();
    uint32 iCnt = 0;
    uint32 fCnt = 0;
    for (uint32 i = 0; i < regs.size(); i++) {
        if (IsPrimitiveInteger(types[i])) {
            if ((iCnt + static_cast<uint32>(regs[i])) <= k8ByteSize) {
                iCnt += static_cast<uint32>(regs[i]);
            };
        } else {
            if ((fCnt + static_cast<uint32>(regs[i])) <= k8ByteSize) {
                fCnt += static_cast<uint32>(regs[i]);
            };
        }
    }
    return iCnt + fCnt;
}

/* If blkassignoff for argument, this function loads the agg arguments into
   virtual registers, disregard if there is sufficient physicall call
   registers. Argument > 16-bytes are copied to preset space and ptr
   result is loaded into virtual register.
   If blassign is not for argument, this function simply memcpy */
void AArch64CGFunc::SelectBlkassignoff(BlkassignoffNode &bNode, Operand *src)
{
    CHECK_FATAL(src->GetKind() == Operand::kOpdRegister, "blkassign src type not in register");
    std::vector<TyIdx> *parmList;
    if (GetLmbcArgInfo() == nullptr) {
        LmbcArgInfo *p = memPool->New<LmbcArgInfo>(*GetFuncScopeAllocator());
        SetLmbcArgInfo(p);
    }
    if (LmbcSmallAggForRet(bNode, src)) {
        return;
    } else if (LmbcSmallAggForCall(bNode, src, &parmList)) {
        return;
    }
    Operand *dest = HandleExpr(bNode, *bNode.Opnd(0));
    RegOperand *regResult = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    /* memcpy for agg assign OR large agg for arg/ret */
    int32 offset = bNode.offset;
    if (IsBlkassignForPush(bNode)) {
        /* large agg for call, addr to be pushed in SelectCall */
        offset = GetLmbcTotalStkUsed();
        if (offset < 0) {
            /* length of ALL stack based args for this call, this location is where the
               next large agg resides, its addr will then be passed */
            offset = static_cast<int32_t>(LmbcFindTotalStkUsed(parmList) + LmbcTotalRegsUsed());
        }
        SetLmbcTotalStkUsed(offset + bNode.blockSize); /* next use */
        SetLmbcArgInfo(regResult, PTY_i64, 0, 1);      /* 1 reg for ptr */
        IncLmbcArgsInRegs(kRegTyInt);
        IncLmbcTotalArgs();
        /* copy large agg arg to offset below */
    }
    std::vector<Operand *> opndVec;
    opndVec.push_back(regResult);                                                                        /* result */
    opndVec.push_back(PrepareMemcpyParamOpnd(offset, *dest));                                            /* param 0 */
    opndVec.push_back(src);                                                                              /* param 1 */
    opndVec.push_back(PrepareMemcpyParamOpnd(static_cast<uint64>(static_cast<int64>(bNode.blockSize)))); /* param 2 */
    SelectLibCall("memcpy", opndVec, PTY_a64, PTY_a64);
}

void AArch64CGFunc::SelectAggIassign(IassignNode &stmt, Operand &addrOpnd)
{
    DEBUG_ASSERT(stmt.Opnd(0) != nullptr, "null ptr check");
    auto &lhsAddrOpnd = LoadIntoRegister(addrOpnd, stmt.Opnd(0)->GetPrimType());
    MemRWNodeHelper lhsMemHelper(stmt, GetFunction(), GetBecommon());
    auto &lhsOfstOpnd = CreateImmOperand(lhsMemHelper.GetByteOffset(), k32BitSize, false);
    auto *lhsMemOpnd = CreateMemOperand(k64BitSize, lhsAddrOpnd, lhsOfstOpnd, stmt.AssigningVolatile());

    bool isRefField = false;
    MemOperand *rhsMemOpnd = SelectRhsMemOpnd(*stmt.GetRHS(), isRefField);
    SelectMemCopy(*lhsMemOpnd, *rhsMemOpnd, lhsMemHelper.GetMemSize(), isRefField, &stmt, stmt.GetRHS());
}

void AArch64CGFunc::SelectReturnSendOfStructInRegs(BaseNode *x)
{
    if (x->GetOpCode() != OP_dread && x->GetOpCode() != OP_iread) {
        // dummy return of 0 inserted by front-end at absence of return
        DEBUG_ASSERT(x->GetOpCode() == OP_constval, "NIY: unexpected return operand");
        uint32 typeSize = GetPrimTypeBitSize(x->GetPrimType());
        RegOperand &dest = GetOrCreatePhysicalRegisterOperand(R0, typeSize, kRegTyInt);
        ImmOperand &src = CreateImmOperand(0, typeSize, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wmovri32, dest, src));
        return;
    }

    MemRWNodeHelper helper(*x, GetFunction(), GetBecommon());
    bool isRefField = false;
    auto *addrOpnd = SelectRhsMemOpnd(*x, isRefField);

    // generate move to regs for agg return
    AArch64CallConvImpl parmlocator(GetBecommon());
    CCLocInfo retMatch;
    parmlocator.LocateRetVal(*helper.GetMIRType(), retMatch);
    if (retMatch.GetReg0() == kRinvalid) {
        return;
    }

    uint32 offset = 0;
    std::vector<RegOperand *> result;
    // load memOpnd to return reg
    // ldr x0, [base]
    // ldr x1, [base + 8]
    auto generateReturnValToRegs = [this, &result, &offset, isRefField, &addrOpnd](regno_t regno, PrimType primType) {
        bool isFpReg = IsPrimitiveFloat(primType) || IsPrimitiveVector(primType);
        RegType regType = isFpReg ? kRegTyFloat : kRegTyInt;
        if (CGOptions::IsBigEndian() && !isFpReg) {
            primType = PTY_u64;
        } else if (GetPrimTypeSize(primType) <= k4ByteSize) {
            primType = isFpReg ? PTY_f32 : PTY_u32;
        }
        auto &phyReg =
            GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regno), GetPrimTypeBitSize(primType), regType);
        DEBUG_ASSERT(addrOpnd->IsMemoryAccessOperand(), "NIY, must be mem opnd");
        auto *newMemOpnd =
            &GetMemOperandAddOffset(static_cast<MemOperand &>(*addrOpnd), offset, GetPrimTypeBitSize(primType));
        MOperator ldMop = PickLdInsn(GetPrimTypeBitSize(primType), primType);
        Insn &ldInsn = GetInsnBuilder()->BuildInsn(ldMop, phyReg, *newMemOpnd);
        ldInsn.MarkAsAccessRefField(isRefField);
        GetCurBB()->AppendInsn(ldInsn);
        if (!VERIFY_INSN(&ldInsn)) {
            SPLIT_INSN(&ldInsn, this);
        }

        offset += GetPrimTypeSize(primType);
        result.push_back(&phyReg);
    };
    generateReturnValToRegs(retMatch.GetReg0(), retMatch.GetPrimTypeOfReg0());
    if (retMatch.GetReg1() != kRinvalid) {
        generateReturnValToRegs(retMatch.GetReg1(), retMatch.GetPrimTypeOfReg1());
    }
    if (retMatch.GetReg2() != kRinvalid) {
        generateReturnValToRegs(retMatch.GetReg2(), retMatch.GetPrimTypeOfReg2());
    }
    if (retMatch.GetReg3() != kRinvalid) {
        generateReturnValToRegs(retMatch.GetReg3(), retMatch.GetPrimTypeOfReg3());
    }
}

Operand *AArch64CGFunc::SelectDread(const BaseNode &parent, DreadNode &expr)
{
    MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());
    auto itr = stIdx2OverflowResult.find(expr.GetStIdx());
    if (itr != stIdx2OverflowResult.end()) {
        /* add_with_overflow / sub_with_overflow:
         * reg1: param1
         * reg2: param2
         * adds/subs reg3, reg1, reg2
         * cset reg4, vs
         * result is saved in std::pair<RegOperand*, RegOperand*>(reg3, reg4)
         */
        if (expr.GetFieldID() == 1) {
            return itr->second.first;
        } else {
            DEBUG_ASSERT(expr.GetFieldID() == 2, "only has 2 fileds for intrinsic overflow call result");
            return itr->second.second;
        }
    }
    if (symbol->IsEhIndex()) {
        CHECK_FATAL(false, "should not go here");
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_i32));
        /* use the second register return by __builtin_eh_return(). */
        AArch64CallConvImpl retLocator(GetBecommon());
        CCLocInfo retMech;
        retLocator.LocateRetVal(*type, retMech);
        retLocator.SetupSecondRetReg(*type, retMech);
        return &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(retMech.GetReg1()), k64BitSize, kRegTyInt);
    }

    PrimType symType = symbol->GetType()->GetPrimType();
    uint32 offset = 0;
    bool parmCopy = false;
    if (expr.GetFieldID() != 0) {
        MIRStructType *structType = static_cast<MIRStructType *>(symbol->GetType());
        DEBUG_ASSERT(structType != nullptr, "SelectDread: non-zero fieldID for non-structure");
        symType = structType->GetFieldType(expr.GetFieldID())->GetPrimType();
        offset = static_cast<uint32>(GetBecommon().GetFieldOffset(*structType, expr.GetFieldID()).first);
        parmCopy = IsParamStructCopy(*symbol);
    }

    uint32 dataSize = GetPrimTypeBitSize(symType);
    uint32 aggSize = 0;
    PrimType resultType = expr.GetPrimType();
    if (symType == PTY_agg) {
        if (expr.GetPrimType() == PTY_agg) {
            aggSize = static_cast<uint32>(GetBecommon().GetTypeSize(symbol->GetType()->GetTypeIndex().GetIdx()));
            dataSize = ((expr.GetFieldID() == 0) ? GetPointerSize() : aggSize) << k8BitShift;
            resultType = PTY_u64;
            symType = resultType;
        } else {
            dataSize = GetPrimTypeBitSize(expr.GetPrimType());
        }
    }
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
    /* a local register variable defined with a specified register */
    if (symbol->GetAsmAttr() != UStrIdx(0) && symbol->GetStorageClass() != kScPstatic &&
        symbol->GetStorageClass() != kScFstatic) {
        std::string regDesp = GlobalTables::GetUStrTable().GetStringFromStrIdx(symbol->GetAsmAttr());
        RegOperand &specifiedOpnd = GetOrCreatePhysicalRegisterOperand(regDesp);
        return &specifiedOpnd;
    }
    memOpnd =
        memOpnd->IsOffsetMisaligned(dataSize) ? &ConstraintOffsetToSafeRegion(dataSize, *memOpnd, symbol) : memOpnd;
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

void AArch64CGFunc::SelectAddrof(Operand &result, StImmOperand &stImm, FieldID field)
{
    const MIRSymbol *symbol = stImm.GetSymbol();
    if (symbol->GetStorageClass() == kScAuto) {
        SetStackProtectInfo(kAddrofStack);
    }
    if ((symbol->GetStorageClass() == kScAuto) || (symbol->GetStorageClass() == kScFormal)) {
        if (!CGOptions::IsQuiet()) {
            maple::LogInfo::MapleLogger(kLlErr)
                << "Warning: we expect AddrOf with StImmOperand is not used for local variables";
        }
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(GetMemlayout()->GetSymAllocInfo(symbol->GetStIndex()));
        ImmOperand *offset = nullptr;
        if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
            offset = &CreateImmOperand(GetBaseOffset(*symLoc) + stImm.GetOffset(), k64BitSize, false, kUnAdjustVary);
        } else if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsRefLocals) {
            auto it = immOpndsRequiringOffsetAdjustmentForRefloc.find(symLoc);
            if (it != immOpndsRequiringOffsetAdjustmentForRefloc.end()) {
                offset = (*it).second;
            } else {
                offset = &CreateImmOperand(GetBaseOffset(*symLoc) + stImm.GetOffset(), k64BitSize, false);
                immOpndsRequiringOffsetAdjustmentForRefloc[symLoc] = offset;
            }
        } else if (mirModule.IsJavaModule()) {
            auto it = immOpndsRequiringOffsetAdjustment.find(symLoc);
            if ((it != immOpndsRequiringOffsetAdjustment.end()) && (symbol->GetType()->GetPrimType() != PTY_agg)) {
                offset = (*it).second;
            } else {
                offset = &CreateImmOperand(GetBaseOffset(*symLoc) + stImm.GetOffset(), k64BitSize, false);
                if (symbol->GetType()->GetKind() != kTypeClass) {
                    immOpndsRequiringOffsetAdjustment[symLoc] = offset;
                }
            }
        } else {
            /* Do not cache modified symbol location */
            offset = &CreateImmOperand(GetBaseOffset(*symLoc) + stImm.GetOffset(), k64BitSize, false);
        }

        SelectAdd(result, *GetBaseReg(*symLoc), *offset, PTY_u64);
        if (GetCG()->GenerateVerboseCG()) {
            /* Add a comment */
            Insn *insn = GetCurBB()->GetLastInsn();
            std::string comm = "local/formal var: ";
            comm += symbol->GetName();
            insn->SetComment(comm);
        }
    } else if (symbol->IsThreadLocal()) {
        SelectAddrofThreadLocal(result, stImm);
        return;
    } else {
        Operand *srcOpnd = &result;
        if (!IsAfterRegAlloc()) {
            // Create a new vreg/preg for the upper bits of the address
            PregIdx pregIdx = GetFunction().GetPregTab()->CreatePreg(PTY_a64);
            MIRPreg *tmpPreg = GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
            regno_t vRegNO = NewVReg(kRegTyInt, GetPrimTypeSize(PTY_a64));
            RegOperand &tmpreg = GetOrCreateVirtualRegisterOperand(vRegNO);

            // Register this vreg mapping
            RegisterVregMapping(vRegNO, pregIdx);

            // Store rematerialization info in the preg
            tmpPreg->SetOp(OP_addrof);
            tmpPreg->rematInfo.sym = symbol;
            tmpPreg->fieldID = field;
            tmpPreg->addrUpper = true;

            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrp, tmpreg, stImm));
            srcOpnd = &tmpreg;
        } else {
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrp, result, stImm));
        }
        if (CGOptions::IsPIC() && symbol->NeedPIC()) {
            /* ldr     x0, [x0, #:got_lo12:Ljava_2Flang_2FSystem_3B_7Cout] */
            OfstOperand &offset = CreateOfstOpnd(*stImm.GetSymbol(), stImm.GetOffset(), stImm.GetRelocs());

            auto size = GetPointerSize() * kBitsPerByte;
            MemOperand &memOpnd = GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, size, static_cast<RegOperand *>(srcOpnd),
                                                     nullptr, &offset, nullptr);
            GetCurBB()->AppendInsn(
                GetInsnBuilder()->BuildInsn(size == k64BitSize ? MOP_xldr : MOP_wldr, result, memOpnd));

            if (stImm.GetOffset() > 0) {
                ImmOperand &immOpnd = CreateImmOperand(stImm.GetOffset(), result.GetSize(), false);
                SelectAdd(result, result, immOpnd, PTY_u64);
            }
        } else {
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrpl12, result, *srcOpnd, stImm));
        }
    }
}

void AArch64CGFunc::SelectAddrof(Operand &result, MemOperand &memOpnd, FieldID field)
{
    const MIRSymbol *symbol = memOpnd.GetSymbol();
    if (symbol->GetStorageClass() == kScAuto) {
        auto *offsetOpnd = static_cast<OfstOperand *>(memOpnd.GetOffsetImmediate());
        Operand &immOpnd = CreateImmOperand(offsetOpnd->GetOffsetValue(), PTY_u32, false);
        DEBUG_ASSERT(memOpnd.GetBaseRegister() != nullptr, "nullptr check");
        SelectAdd(result, *memOpnd.GetBaseRegister(), immOpnd, PTY_u32);
        SetStackProtectInfo(kAddrofStack);
    } else if (!IsAfterRegAlloc()) {
        // Create a new vreg/preg for the upper bits of the address
        PregIdx pregIdx = GetFunction().GetPregTab()->CreatePreg(PTY_a64);
        MIRPreg *tmpPreg = GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
        regno_t vRegNO = NewVReg(kRegTyInt, GetPrimTypeSize(PTY_a64));
        RegOperand &tmpreg = GetOrCreateVirtualRegisterOperand(vRegNO);

        // Register this vreg mapping
        RegisterVregMapping(vRegNO, pregIdx);

        // Store rematerialization info in the preg
        tmpPreg->SetOp(OP_addrof);
        tmpPreg->rematInfo.sym = symbol;
        tmpPreg->fieldID = field;
        tmpPreg->addrUpper = true;

        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrp, tmpreg, memOpnd));
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrpl12, result, tmpreg, memOpnd));
    } else {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrp, result, memOpnd));
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrpl12, result, result, memOpnd));
    }
}

Operand *AArch64CGFunc::SelectAddrof(AddrofNode &expr, const BaseNode &parent, bool isAddrofoff)
{
    MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());
    int32 offset = 0;
    AddrofoffNode &addrofoffExpr = static_cast<AddrofoffNode &>(static_cast<BaseNode &>(expr));
    if (isAddrofoff) {
        offset = addrofoffExpr.offset;
    } else {
        if (expr.GetFieldID() != 0) {
            MIRStructType *structType = static_cast<MIRStructType *>(symbol->GetType());
            /* with array of structs, it is possible to have nullptr */
            if (structType != nullptr) {
                offset = GetBecommon().GetFieldOffset(*structType, expr.GetFieldID()).first;
            }
        }
    }
    if ((symbol->GetStorageClass() == kScFormal) && (symbol->GetSKind() == kStVar) &&
        ((!isAddrofoff && expr.GetFieldID() != 0) ||
         (GetBecommon().GetTypeSize(symbol->GetType()->GetTypeIndex().GetIdx()) > k16ByteSize))) {
        /*
         * Struct param is copied on the stack by caller if struct size > 16.
         * Else if size < 16 then struct param is copied into one or two registers.
         */
        RegOperand *stackAddr = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
        /* load the base address of the struct copy from stack. */
        SelectAddrof(*stackAddr, CreateStImmOperand(*symbol, 0, 0));
        Operand *structAddr;
        if (!IsParamStructCopy(*symbol)) {
            isAggParamInReg = true;
            structAddr = stackAddr;
        } else {
            OfstOperand *offopnd = &CreateOfstOpnd(0, k32BitSize);
            MemOperand *mo = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, GetPointerSize() * kBitsPerByte, stackAddr,
                                                 nullptr, offopnd, nullptr);
            structAddr = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xldr, *structAddr, *mo));
        }
        if (offset == 0) {
            return structAddr;
        } else {
            /* add the struct offset to the base address */
            Operand *result = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
            ImmOperand *imm = &CreateImmOperand(PTY_a64, offset);
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xaddrri12, *result, *structAddr, *imm));
            return result;
        }
    }
    PrimType ptype = expr.GetPrimType();
    Operand &result = GetOrCreateResOperand(parent, ptype);
    if (symbol->IsReflectionClassInfo() && !symbol->IsReflectionArrayClassInfo() && !GetCG()->IsLibcore()) {
        /*
         * Turn addrof __cinf_X  into a load of _PTR__cinf_X
         * adrp    x1, _PTR__cinf_Ljava_2Flang_2FSystem_3B
         * ldr     x1, [x1, #:lo12:_PTR__cinf_Ljava_2Flang_2FSystem_3B]
         */
        std::string ptrName = namemangler::kPtrPrefixStr + symbol->GetName();
        MIRType *ptrType = GlobalTables::GetTypeTable().GetPtr();
        symbol = GetMirModule().GetMIRBuilder()->GetOrCreateGlobalDecl(ptrName, *ptrType);
        symbol->SetStorageClass(kScFstatic);

        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_adrp_ldr, result, CreateStImmOperand(*symbol, 0, 0)));
        /* make it un rematerializable. */
        MIRPreg *preg = GetPseudoRegFromVirtualRegNO(static_cast<RegOperand &>(result).GetRegisterNumber());
        if (preg) {
            preg->SetOp(OP_undef);
        }
        return &result;
    }

    SelectAddrof(result, CreateStImmOperand(*symbol, offset, 0), isAddrofoff ? 0 : expr.GetFieldID());
    return &result;
}

Operand *AArch64CGFunc::SelectAddrofoff(AddrofoffNode &expr, const BaseNode &parent)
{
    return SelectAddrof(static_cast<AddrofNode &>(static_cast<BaseNode &>(expr)), parent, true);
}

Operand &AArch64CGFunc::SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent)
{
    uint32 instrSize = static_cast<uint32>(expr.SizeOfInstr());
    PrimType primType = (instrSize == k8ByteSize)
                            ? PTY_u64
                            : (instrSize == k4ByteSize) ? PTY_u32 : (instrSize == k2ByteSize) ? PTY_u16 : PTY_u8;
    Operand &operand = GetOrCreateResOperand(parent, primType);
    MIRFunction *mirFunction = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(expr.GetPUIdx());
    SelectAddrof(operand, CreateStImmOperand(*mirFunction->GetFuncSymbol(), 0, 0));
    return operand;
}

/* For an entire aggregate that can fit inside a single 8 byte register.  */
PrimType AArch64CGFunc::GetDestTypeFromAggSize(uint32 bitSize) const
{
    PrimType primType;
    switch (bitSize) {
        case k8BitSize: {
            primType = PTY_u8;
            break;
        }
        case k16BitSize: {
            primType = PTY_u16;
            break;
        }
        case k32BitSize: {
            primType = PTY_u32;
            break;
        }
        case k64BitSize: {
            primType = PTY_u64;
            break;
        }
        default:
            CHECK_FATAL(false, "aggregate of unhandled size");
    }
    return primType;
}

Operand &AArch64CGFunc::SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent)
{
    /* adrp reg, label-id */
    uint32 instrSize = static_cast<uint32>(expr.SizeOfInstr());
    PrimType primType = (instrSize == k8ByteSize)
                            ? PTY_u64
                            : (instrSize == k4ByteSize) ? PTY_u32 : (instrSize == k2ByteSize) ? PTY_u16 : PTY_u8;
    Operand &dst = GetOrCreateResOperand(parent, primType);
    Operand &immOpnd = CreateImmOperand(expr.GetOffset(), k64BitSize, false);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_adrp_label, dst, immOpnd));
    return dst;
}

Operand *AArch64CGFunc::SelectIreadoff(const BaseNode &parent, IreadoffNode &ireadoff)
{
    auto offset = ireadoff.GetOffset();
    auto primType = ireadoff.GetPrimType();
    auto bitSize = GetPrimTypeBitSize(primType);
    auto *baseAddr = ireadoff.Opnd(0);
    auto *result = &CreateRegisterOperandOfType(primType);
    auto *addrOpnd = HandleExpr(ireadoff, *baseAddr);
    ASSERT_NOT_NULL(addrOpnd);
    if (primType == PTY_agg && parent.GetOpCode() == OP_regassign) {
        auto &memOpnd = CreateMemOpnd(LoadIntoRegister(*addrOpnd, PTY_a64), offset, bitSize);
        auto mop = PickLdInsn(64, PTY_a64);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, *result, memOpnd));
        auto &regAssignNode = static_cast<const RegassignNode &>(parent);
        PregIdx pIdx = regAssignNode.GetRegIdx();
        CHECK_FATAL(IsSpecialPseudoRegister(pIdx), "SelectIreadfpoff of agg");
        (void)LmbcSmallAggForRet(parent, addrOpnd, offset, true);
        // result not used
    } else {
        auto &memOpnd = CreateMemOpnd(LoadIntoRegister(*addrOpnd, PTY_a64), offset, bitSize);
        auto mop = PickLdInsn(bitSize, primType);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mop, *result, memOpnd));
    }
    return result;
}

RegOperand *AArch64CGFunc::GenLmbcParamLoad(int32 offset, uint32 byteSize, RegType regType, PrimType primType,
                                            AArch64reg baseRegno)
{
    MemOperand *memOpnd = GenLmbcFpMemOperand(offset, byteSize, baseRegno);
    RegOperand *result = &GetOrCreateVirtualRegisterOperand(NewVReg(regType, byteSize));
    MOperator mOp = PickLdInsn(byteSize * kBitsPerByte, primType);
    Insn &load = GetInsnBuilder()->BuildInsn(mOp, *result, *memOpnd);
    GetCurBB()->AppendInsn(load);
    return result;
}

RegOperand *AArch64CGFunc::LmbcStructReturnLoad(int32 offset)
{
    RegOperand *result = nullptr;
    MIRFunction &func = GetFunction();
    CHECK_FATAL(func.IsReturnStruct(), "LmbcStructReturnLoad: not struct return");
    MIRType *ty = func.GetReturnType();
    uint32 sz = GetBecommon().GetTypeSize(ty->GetTypeIndex());
    uint32 fpSize;
    uint32 numFpRegs = FloatParamRegRequired(static_cast<MIRStructType *>(ty), fpSize);
    if (numFpRegs > 0) {
        PrimType pType = (fpSize <= k4ByteSize) ? PTY_f32 : PTY_f64;
        for (int32 i = (numFpRegs - kOneRegister); i > 0; --i) {
            result = GenLmbcParamLoad(offset + (i * static_cast<int32>(fpSize)), fpSize, kRegTyFloat, pType);
            AArch64reg regNo = static_cast<AArch64reg>(V0 + static_cast<uint32>(i));
            RegOperand *reg = &GetOrCreatePhysicalRegisterOperand(regNo, fpSize * kBitsPerByte, kRegTyFloat);
            SelectCopy(*reg, pType, *result, pType);
            Insn &pseudo = GetInsnBuilder()->BuildInsn(MOP_pseudo_ret_float, *reg);
            GetCurBB()->AppendInsn(pseudo);
        }
        result = GenLmbcParamLoad(offset, fpSize, kRegTyFloat, pType);
    } else if (sz <= k4ByteSize) {
        result = GenLmbcParamLoad(offset, k4ByteSize, kRegTyInt, PTY_u32);
    } else if (sz <= k8ByteSize) {
        result = GenLmbcParamLoad(offset, k8ByteSize, kRegTyInt, PTY_i64);
    } else if (sz <= k16ByteSize) {
        result = GenLmbcParamLoad(offset + k8ByteSizeInt, k8ByteSize, kRegTyInt, PTY_i64);
        RegOperand *r1 = &GetOrCreatePhysicalRegisterOperand(R1, k8ByteSize * kBitsPerByte, kRegTyInt);
        SelectCopy(*r1, PTY_i64, *result, PTY_i64);
        Insn &pseudo = GetInsnBuilder()->BuildInsn(MOP_pseudo_ret_int, *r1);
        GetCurBB()->AppendInsn(pseudo);
        result = GenLmbcParamLoad(offset, k8ByteSize, kRegTyInt, PTY_i64);
    }
    return result;
}

Operand *AArch64CGFunc::SelectIreadfpoff(const BaseNode &parent, IreadFPoffNode &ireadoff)
{
    uint32 offset = static_cast<uint32>(ireadoff.GetOffset());
    PrimType primType = ireadoff.GetPrimType();
    uint32 bytelen = GetPrimTypeSize(primType);
    uint32 bitlen = bytelen * kBitsPerByte;
    RegType regty = GetRegTyFromPrimTy(primType);
    RegOperand *result = nullptr;
    if (offset >= 0) {
        LmbcFormalParamInfo *info = GetLmbcFormalParamInfo(static_cast<uint32>(offset));
        if (info->GetPrimType() == PTY_agg) {
            if (info->IsOnStack()) {
                result = GenLmbcParamLoad(info->GetOnStackOffset(), GetPrimTypeSize(PTY_a64), kRegTyInt, PTY_a64);
                regno_t baseRegno = result->GetRegisterNumber();
                result = GenLmbcParamLoad(offset - static_cast<int32>(info->GetOffset()), bytelen, regty, primType,
                                          (AArch64reg)baseRegno);
            } else if (primType == PTY_agg) {
                CHECK_FATAL(parent.GetOpCode() == OP_regassign, "SelectIreadfpoff of agg");
                result = LmbcStructReturnLoad(offset);
            } else {
                result = GenLmbcParamLoad(offset, bytelen, regty, primType);
            }
        } else {
            CHECK_FATAL(primType == info->GetPrimType(), "Incorrect primtype");
            CHECK_FATAL(offset == info->GetOffset(), "Incorrect offset");
            if (info->GetRegNO() == 0 || !info->HasRegassign()) {
                result = GenLmbcParamLoad(offset, bytelen, regty, primType);
            } else {
                result = &GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(info->GetRegNO()), bitlen, regty);
            }
        }
    } else {
        if (primType == PTY_agg) {
            CHECK_FATAL(parent.GetOpCode() == OP_regassign, "SelectIreadfpoff of agg");
            result = LmbcStructReturnLoad(offset);
        } else {
            result = GenLmbcParamLoad(offset, bytelen, regty, primType);
        }
    }
    return result;
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
    AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone;

    if (expr.GetFieldID() != 0) {
        MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        MIRStructType *structType = nullptr;
        if (pointedTy->GetKind() != kTypeJArray) {
            structType = static_cast<MIRStructType *>(pointedTy);
        } else {
            /* it's a Jarray type. using it's parent's field info: java.lang.Object */
            structType = static_cast<MIRJarrayType *>(pointedTy)->GetParentType();
        }

        DEBUG_ASSERT(structType != nullptr, "SelectIread: non-zero fieldID for non-structure");
        pointedType = structType->GetFieldType(expr.GetFieldID());
        offset = GetBecommon().GetFieldOffset(*structType, expr.GetFieldID()).first;
        isRefField = GetBecommon().IsRefField(*structType, expr.GetFieldID());
    } else {
        pointedType = GetPointedToType(*pointerType);
        if (GetFunction().IsJava() && (pointedType->GetKind() == kTypePointer)) {
            MIRType *nextPointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
                static_cast<MIRPtrType *>(pointedType)->GetPointedTyIdx());
            if (nextPointedType->GetKind() != kTypeScalar) {
                isRefField = true; /* read from an object array, or an high-dimentional array */
            }
        }
    }

    RegType regType = GetRegTyFromPrimTy(expr.GetPrimType());
    uint32 regSize = GetPrimTypeSize(expr.GetPrimType());
    if (expr.GetFieldID() == 0 && pointedType->GetPrimType() == PTY_agg) {
        /* Maple IR can passing small struct to be loaded into a single register. */
        if (regType == kRegTyFloat) {
            /* regsize is correct */
        } else {
            uint32 sz = GetBecommon().GetTypeSize(pointedType->GetTypeIndex().GetIdx());
            regSize = (sz <= k4ByteSize) ? k4ByteSize : k8ByteSize;
        }
    } else if (regSize < k4ByteSize) {
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

    uint32 bitSize = 0;
    if ((pointedType->GetKind() == kTypeStructIncomplete) || (pointedType->GetKind() == kTypeClassIncomplete) ||
        (pointedType->GetKind() == kTypeInterfaceIncomplete)) {
        bitSize = GetPrimTypeBitSize(expr.GetPrimType());
        maple::LogInfo::MapleLogger(kLlErr) << "Warning: objsize is zero! \n";
    } else {
        if (pointedType->IsStructType()) {
            MIRStructType *structType = static_cast<MIRStructType *>(pointedType);
            /* size << 3, that is size * 8, change bytes to bits */
            bitSize = std::min(structType->GetSize(), static_cast<size_t>(GetPointerSize())) << 3;
        } else {
            bitSize = GetPrimTypeBitSize(destType);
        }
        if (regType == kRegTyFloat) {
            destType = expr.GetPrimType();
            bitSize = GetPrimTypeBitSize(destType);
        } else if (destType == PTY_agg) {
            switch (bitSize) {
                case k8BitSize:
                    destType = PTY_u8;
                    break;
                case k16BitSize:
                    destType = PTY_u16;
                    break;
                case k32BitSize:
                    destType = PTY_u32;
                    break;
                case k64BitSize:
                    destType = PTY_u64;
                    break;
                default:
                    destType = PTY_u64;  // when eval agg . a way to round up
                    DEBUG_ASSERT(bitSize == 0, " round up empty agg ");
                    bitSize = k64BitSize;
                    break;
            }
        }
    }

    PrimType memType = (finalBitFieldDestType == kPtyInvalid ? destType : finalBitFieldDestType);
    MemOperand *memOpnd = CreateMemOpndOrNull(memType, expr, *expr.Opnd(0),
                                              static_cast<int64>(static_cast<int>(offset) + extraOffset), memOrd);
    if (aggParamReg != nullptr) {
        isAggParamInReg = false;
        return aggParamReg;
    }
    DEBUG_ASSERT(memOpnd != nullptr, "memOpnd should not be nullptr");
    if (isVolLoad && (memOpnd->GetAddrMode() == MemOperand::kAddrModeBOi)) {
        memOrd = AArch64isa::kMoAcquire;
        isVolLoad = false;
    }

    memOpnd =
        memOpnd->IsOffsetMisaligned(bitSize) ? &ConstraintOffsetToSafeRegion(bitSize, *memOpnd, nullptr) : memOpnd;
    if (memOrd == AArch64isa::kMoNone) {
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
                                                                       insn.GetOperand(0),
                                                                       CreateImmOperand(1, kMaxImmVal5Bits, false)));
                }

                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, insn.GetOperand(0), insn.GetOperand(0)));
            }
        }
    } else {
        if ((memOpnd->GetMemVaryType() == kNotVary) && IsImmediateOffsetOutOfRange(*memOpnd, bitSize)) {
            memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, bitSize);
        }
        AArch64CGFunc::SelectLoadAcquire(*result, destType, *memOpnd, destType, memOrd, false);
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

Operand *AArch64CGFunc::SelectIntConst(const MIRIntConst &intConst)
{
    return &CreateImmOperand(intConst.GetExtValue(), GetPrimTypeSize(intConst.GetType().GetPrimType()) * kBitsPerByte,
                             false);
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

template <typename T>
Operand *SelectStrLiteral(T &c, AArch64CGFunc &cgFunc)
{
    std::string labelStr;
    if (c.GetKind() == kConstStrConst) {
        labelStr += ".LUstr_";
    } else if (c.GetKind() == kConstStr16Const) {
        labelStr += ".LUstr16_";
    } else {
        CHECK_FATAL(false, "Unsupported literal type");
    }
    labelStr += std::to_string(c.GetValue());

    MIRSymbol *labelSym =
        GlobalTables::GetGsymTable().GetSymbolFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(labelStr));
    if (labelSym == nullptr) {
        labelSym = cgFunc.GetMirModule().GetMIRBuilder()->CreateGlobalDecl(labelStr, c.GetType());
        labelSym->SetStorageClass(kScFstatic);
        labelSym->SetSKind(kStConst);
        /* c may be local, we need a global node here */
        labelSym->SetKonst(cgFunc.NewMirConst(c));
    }

    if (c.GetPrimType() == PTY_ptr) {
        StImmOperand &stOpnd = cgFunc.CreateStImmOperand(*labelSym, 0, 0);
        RegOperand &addrOpnd = cgFunc.CreateRegisterOperandOfType(PTY_a64);
        cgFunc.SelectAddrof(addrOpnd, stOpnd);
        return &addrOpnd;
    }
    CHECK_FATAL(false, "Unsupported const string type");
    return nullptr;
}

Operand *AArch64CGFunc::SelectStrConst(MIRStrConst &strConst)
{
    return SelectStrLiteral(strConst, *this);
}

Operand *AArch64CGFunc::SelectStr16Const(MIRStr16Const &str16Const)
{
    return SelectStrLiteral(str16Const, *this);
}

static inline void AppendInstructionTo(Insn &i, CGFunc &f)
{
    f.GetCurBB()->AppendInsn(i);
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

void AArch64CGFunc::SelectIgoto(Operand *opnd0)
{
    Operand *srcOpnd = opnd0;
    if (opnd0->GetKind() == Operand::kOpdMem) {
        Operand *dst = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xldr, *dst, *opnd0));
        srcOpnd = dst;
    }
    GetCurBB()->SetKind(BB::kBBIgoto);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xbr, *srcOpnd));
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
            /* Special cases, i.e., comparing with zero
             * Do not perform optimization for C, unlike Java which has no unsigned int.
             */
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

    if (opnd0.IsRegister() && (static_cast<RegOperand *>(&opnd0)->GetValidBitsNum() == 1) &&
        (condNode->GetOpCode() == OP_lior)) {
        ImmOperand &condBit = CreateImmOperand(0, k8BitSize, false);
        if (stmt.GetOpCode() == OP_brtrue) {
            GetCurBB()->AppendInsn(
                GetInsnBuilder()->BuildInsn(MOP_wtbnz, static_cast<RegOperand &>(opnd0), condBit, targetOpnd));
        } else {
            GetCurBB()->AppendInsn(
                GetInsnBuilder()->BuildInsn(MOP_wtbz, static_cast<RegOperand &>(opnd0), condBit, targetOpnd));
        }
        return;
    }

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
    if (!IsPrimitiveVector(dtype)) {
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
    } else {
        /* vector operands */
        resOpnd =
            SelectVectorBinOp(dtype, &opnd0, node.Opnd(0)->GetPrimType(), &opnd1, node.Opnd(1)->GetPrimType(), OP_add);
    }
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

Operand *AArch64CGFunc::SelectMadd(BinaryNode &node, Operand &opndM0, Operand &opndM1, Operand &opnd1,
                                   const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    bool isSigned = IsSignedInteger(dtype);
    uint32 dsize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (dsize == k64BitSize);
    /* promoted type */
    PrimType primType = is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, primType);
    SelectMadd(resOpnd, opndM0, opndM1, opnd1, primType);
    return &resOpnd;
}

void AArch64CGFunc::SelectMadd(Operand &resOpnd, Operand &opndM0, Operand &opndM1, Operand &opnd1, PrimType primType)
{
    Operand::OperandType opndM0Type = opndM0.GetKind();
    Operand::OperandType opndM1Type = opndM1.GetKind();
    Operand::OperandType opnd1Type = opnd1.GetKind();
    uint32 dsize = GetPrimTypeBitSize(primType);
    bool is64Bits = (dsize == k64BitSize);

    if (opndM0Type != Operand::kOpdRegister) {
        SelectMadd(resOpnd, SelectCopy(opndM0, primType, primType), opndM1, opnd1, primType);
        return;
    } else if (opndM1Type != Operand::kOpdRegister) {
        SelectMadd(resOpnd, opndM0, SelectCopy(opndM1, primType, primType), opnd1, primType);
        return;
    } else if (opnd1Type != Operand::kOpdRegister) {
        SelectMadd(resOpnd, opndM0, opndM1, SelectCopy(opnd1, primType, primType), primType);
        return;
    }

    DEBUG_ASSERT(IsPrimitiveInteger(primType), "NYI MAdd");
    MOperator mOp = is64Bits ? MOP_xmaddrrrr : MOP_wmaddrrrr;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opndM0, opndM1, opnd1));
}

Operand &AArch64CGFunc::SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent)
{
    BaseNode *opnd0 = node.Opnd(0);
    BaseNode *opnd1 = node.Opnd(1);
    DEBUG_ASSERT(opnd1->GetOpCode() == OP_constval, "Internal error, opnd1->op should be OP_constval.");

    switch (opnd0->op) {
        case OP_regread: {
            RegreadNode *regreadNode = static_cast<RegreadNode *>(opnd0);
            return *SelectRegread(*regreadNode);
        }
        case OP_addrof: {
            AddrofNode *addrofNode = static_cast<AddrofNode *>(opnd0);
            MIRSymbol &symbol = *mirModule.CurFunction()->GetLocalOrGlobalSymbol(addrofNode->GetStIdx());
            DEBUG_ASSERT(addrofNode->GetFieldID() == 0, "For debug SelectCGArrayElemAdd.");

            Operand &result = GetOrCreateResOperand(parent, PTY_a64);

            /* OP_constval */
            ConstvalNode *constvalNode = static_cast<ConstvalNode *>(opnd1);
            MIRConst *mirConst = constvalNode->GetConstVal();
            MIRIntConst *mirIntConst = static_cast<MIRIntConst *>(mirConst);
            SelectAddrof(result, CreateStImmOperand(symbol, mirIntConst->GetExtValue(), 0));

            return result;
        }
        default:
            CHECK_FATAL(0, "Internal error, cannot handle opnd0.");
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
    if (!IsPrimitiveVector(dtype)) {
        /* promoted type */
        PrimType primType =
            isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
        resOpnd = &GetOrCreateResOperand(parent, primType);
        SelectSub(*resOpnd, opnd0, opnd1, primType);
    } else {
        /* vector operands */
        resOpnd =
            SelectVectorBinOp(dtype, &opnd0, node.Opnd(0)->GetPrimType(), &opnd1, node.Opnd(1)->GetPrimType(), OP_sub);
    }
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
    if (!IsPrimitiveVector(dtype)) {
        /* promoted type */
        PrimType primType =
            isFloat ? dtype : ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
        resOpnd = &GetOrCreateResOperand(parent, primType);
        SelectMpy(*resOpnd, opnd0, opnd1, primType);
    } else {
        resOpnd =
            SelectVectorBinOp(dtype, &opnd0, node.Opnd(0)->GetPrimType(), &opnd1, node.Opnd(1)->GetPrimType(), OP_mul);
    }
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
            if (((static_cast<uint64>(headVal) - 1) & (static_cast<uint64>(headVal) - 2)) ==
                0) {  // 2 see comment above
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
    CHECK_FATAL(!IsPrimitiveVector(dtype), "NYI DIV vector operands");
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
    CHECK_FATAL(!IsPrimitiveVector(dtype), "NYI DIV vector operands");

    /* promoted type */
    PrimType primType = ((is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32)));
    RegOperand &resOpnd = GetOrCreateResOperand(parent, primType);
    SelectRem(resOpnd, opnd0, opnd1, primType, isSigned, is64Bits);
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectLand(BinaryNode &node, Operand &lhsOpnd, Operand &rhsOpnd, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    DEBUG_ASSERT(IsPrimitiveInteger(primType), "Land should be integer type");
    bool is64Bits = (GetPrimTypeBitSize(primType) == k64BitSize);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, is64Bits ? PTY_u64 : PTY_u32);
    /*
     * OP0 band Op1
     * cmp  OP0, 0     # compare X0 with 0, sets Z bit
     * ccmp OP1, 0, 4 //==0100b, ne     # if(OP0!=0) cmp Op1 and 0, else NZCV <- 0100 makes OP0==0
     * cset RES, ne     # if Z==1(i.e., OP0==0||OP1==0) RES<-0, RES<-1
     */
    Operand &opnd0 = LoadIntoRegister(lhsOpnd, primType);
    SelectAArch64Cmp(opnd0, CreateImmOperand(0, primType, false), true, GetPrimTypeBitSize(primType));
    Operand &opnd1 = LoadIntoRegister(rhsOpnd, primType);
    constexpr int64 immValue4 = 4;  // integer as comment above
    SelectAArch64CCmp(opnd1, CreateImmOperand(0, primType, false), CreateImmOperand(immValue4, PTY_u8, false),
                      GetCondOperand(CC_NE), is64Bits);
    SelectAArch64CSet(resOpnd, GetCondOperand(CC_NE), is64Bits);
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectLor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent,
                                  bool parentIsBr)
{
    PrimType primType = node.GetPrimType();
    DEBUG_ASSERT(IsPrimitiveInteger(primType), "Lior should be integer type");
    bool is64Bits = (GetPrimTypeBitSize(primType) == k64BitSize);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, is64Bits ? PTY_u64 : PTY_u32);
    /*
     * OP0 band Op1
     * cmp  OP0, 0     # compare X0 with 0, sets Z bit
     * ccmp OP1, 0, 0 //==0100b, eq     # if(OP0==0,eq) cmp Op1 and 0, else NZCV <- 0000 makes OP0!=0
     * cset RES, ne     # if Z==1(i.e., OP0==0&&OP1==0) RES<-0, RES<-1
     */
    if (parentIsBr && !is64Bits && opnd0.IsRegister() && (static_cast<RegOperand *>(&opnd0)->GetValidBitsNum() == 1) &&
        opnd1.IsRegister() && (static_cast<RegOperand *>(&opnd1)->GetValidBitsNum() == 1)) {
        uint32 mOp = MOP_wiorrrr;
        static_cast<RegOperand &>(resOpnd).SetValidBitsNum(1);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, resOpnd, opnd0, opnd1));
    } else {
        SelectBior(resOpnd, opnd0, opnd1, primType);
        SelectAArch64Cmp(resOpnd, CreateImmOperand(0, primType, false), true, GetPrimTypeBitSize(primType));
        SelectAArch64CSet(resOpnd, GetCondOperand(CC_NE), is64Bits);
    }
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
    if ((opcode == OP_cmpl) || (opcode == OP_cmpg)) {
        DEBUG_ASSERT(isFloat, "incorrect operand types");
        SelectTargetFPCmpQuiet(opnd0, *opnd1, GetPrimTypeBitSize(primType));
        SelectAArch64CSINV(resOpnd, xzr, xzr, GetCondOperand(CC_GE), (dsize == k64BitSize));
        SelectAArch64CSINC(resOpnd, resOpnd, xzr, GetCondOperand(CC_LE), (dsize == k64BitSize));
        if (opcode == OP_cmpl) {
            SelectAArch64CSINV(resOpnd, resOpnd, xzr, GetCondOperand(CC_VC), (dsize == k64BitSize));
        } else {
            SelectAArch64CSINC(resOpnd, resOpnd, xzr, GetCondOperand(CC_VC), (dsize == k64BitSize));
        }
        return;
    }

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
        (static_cast<ImmOperand *>(opnd1)->GetValue() == 0) && parent.GetOpCode() != OP_select && !isFloat) {
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
    RegOperand *resOpnd = nullptr;
    if (!IsPrimitiveVector(node.GetPrimType())) {
        resOpnd = &GetOrCreateResOperand(parent, node.GetPrimType());
        SelectCmpOp(*resOpnd, opnd0, opnd1, node.GetOpCode(), node.GetOpndType(), parent);
    } else {
        resOpnd = SelectVectorCompare(&opnd0, node.Opnd(0)->GetPrimType(), &opnd1, node.Opnd(1)->GetPrimType(),
                                      node.GetOpCode());
    }
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

void AArch64CGFunc::SelectAArch64CCmp(Operand &o, Operand &i, Operand &nzcv, CondOperand &cond, bool is64Bits)
{
    uint32 mOpCode = is64Bits ? MOP_xccmpriic : MOP_wccmpriic;
    Operand &rflag = GetOrCreateRflag();
    std::vector<Operand *> opndVec;
    opndVec.push_back(&rflag);
    opndVec.push_back(&o);
    opndVec.push_back(&i);
    opndVec.push_back(&nzcv);
    opndVec.push_back(&cond);
    opndVec.push_back(&rflag);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOpCode, opndVec));
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
    RegOperand *resOpnd = nullptr;
    if (!IsPrimitiveVector(dtype)) {
        PrimType primType =
            is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32); /* promoted type */
        resOpnd = &GetOrCreateResOperand(parent, primType);
        SelectRelationOperator(operatorCode, *resOpnd, opnd0, opnd1, primType);
    } else {
        /* vector operations */
        resOpnd = SelectVectorBitwiseOp(dtype, &opnd0, node.Opnd(0)->GetPrimType(), &opnd1, node.Opnd(1)->GetPrimType(),
                                        (operatorCode == kAND) ? OP_band : (operatorCode == kIOR ? OP_bior : OP_bxor));
    }
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

    bool isOneElemVector = false;
    BaseNode *expr = node.Opnd(0);
    if (expr->GetOpCode() == OP_dread) {
        MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(static_cast<DreadNode *>(expr)->GetStIdx());
        isOneElemVector = symbol->GetAttr(ATTR_oneelem_simd);
    }

    Operand *opd0 = &opnd0;
    PrimType otyp0 = expr->GetPrimType();
    if (IsPrimitiveVector(dtype) && opnd0.IsConstImmediate()) {
        opd0 = SelectVectorFromScalar(dtype, opd0, node.Opnd(0)->GetPrimType());
        otyp0 = dtype;
    }

    if (IsPrimitiveVector(dtype) && opnd1.IsConstImmediate()) {
        int64 sConst = static_cast<ImmOperand &>(opnd1).GetValue();
        resOpnd = SelectVectorShiftImm(dtype, opd0, &opnd1, static_cast<int32>(sConst), opcode);
    } else if ((IsPrimitiveVector(dtype) || isOneElemVector) && !opnd1.IsConstImmediate()) {
        resOpnd = SelectVectorShift(dtype, opd0, otyp0, &opnd1, node.Opnd(1)->GetPrimType(), opcode);
    } else {
        PrimType primType =
            isFloat ? dtype : (is64Bits ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32));
        resOpnd = &GetOrCreateResOperand(parent, primType);
        ShiftDirection direct = (opcode == OP_lshr) ? kShiftLright : ((opcode == OP_ashr) ? kShiftAright : kShiftLeft);
        SelectShift(*resOpnd, opnd0, opnd1, direct, primType);
    }

    if (dtype == PTY_i16) {
        MOperator exOp = is64Bits ? MOP_xsxth64 : MOP_xsxth32;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(exOp, *resOpnd, *resOpnd));
    } else if (dtype == PTY_i8) {
        MOperator exOp = is64Bits ? MOP_xsxtb64 : MOP_xsxtb32;
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(exOp, *resOpnd, *resOpnd));
    }
    return resOpnd;
}

Operand *AArch64CGFunc::SelectRor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    uint32 dsize = GetPrimTypeBitSize(dtype);
    PrimType primType = (dsize == k64BitSize) ? PTY_u64 : PTY_u32;
    RegOperand *resOpnd = &GetOrCreateResOperand(parent, primType);
    Operand *firstOpnd = &LoadIntoRegister(opnd0, primType);
    MOperator mopRor = (dsize == k64BitSize) ? MOP_xrorrrr : MOP_wrorrrr;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopRor, *resOpnd, *firstOpnd, opnd1));
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
    if (IsPrimitiveVector(dtyp)) {
        return SelectVectorAbs(dtyp, &opnd0);
    } else if (IsPrimitiveFloat(dtyp)) {
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
    DEBUG_ASSERT(IsPrimitiveInteger(dtype) || IsPrimitiveVectorInteger(dtype), "bnot expect integer or NYI");
    uint32 bitSize = GetPrimTypeBitSize(dtype);
    bool is64Bits = (bitSize == k64BitSize);
    bool isSigned = IsSignedInteger(dtype);
    RegOperand *resOpnd = nullptr;
    if (!IsPrimitiveVector(dtype)) {
        /* promoted type */
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
    } else {
        /* vector operand */
        resOpnd = SelectVectorNot(dtype, &opnd0);
    }
    return resOpnd;
}

Operand *AArch64CGFunc::SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    auto bitWidth = (GetPrimTypeBitSize(dtype));
    RegOperand *resOpnd = nullptr;
    resOpnd = &GetOrCreateResOperand(parent, dtype);
    Operand &newOpnd0 = LoadIntoRegister(opnd0, dtype);
    uint32 mopBswap = bitWidth == 64 ? MOP_xrevrr : (bitWidth == 32 ? MOP_wrevrr : MOP_wrevrr16);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBswap, *resOpnd, newOpnd0));
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
    RegOperand *srcVecRegOperand = static_cast<RegOperand *>(&srcOpnd);
    if (srcVecRegOperand && srcVecRegOperand->IsRegister() && (srcVecRegOperand->GetSize() == k128BitSize)) {
        if ((bitSize == k8BitSize || bitSize == k16BitSize || bitSize == k32BitSize || bitSize == k64BitSize) &&
            (bitOffset % bitSize) == k0BitSize) {
            uint32 lane = bitOffset / bitSize;
            PrimType srcVecPtype;
            if (bitSize == k64BitSize) {
                srcVecPtype = PTY_v2u64;
            } else if (bitSize == k32BitSize) {
                srcVecPtype = PTY_v4u32;
            } else if (bitSize == k16BitSize) {
                srcVecPtype = PTY_v8u16;
            } else {
                srcVecPtype = PTY_v16u8;
            }
            RegOperand *resRegOperand =
                SelectVectorGetElement(node.GetPrimType(), &srcOpnd, srcVecPtype, static_cast<int32>(lane));
            return resRegOperand;
        } else {
            CHECK_FATAL(false, "NYI");
        }
    }
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

/*
 *  operand fits in MOVK if
 *     is64Bits && bitOffset == 0, 16, 32, 48 && bSize == 16
 *  or is32Bits && bitOffset == 0, 16 && bSize == 16
 *  imm range of aarch64-movk is [0 - 65536] imm16
 */
inline bool IsMoveWideKeepable(int64 offsetVal, uint32 bitOffset, uint32 bitSize, bool is64Bits)
{
    DEBUG_ASSERT(is64Bits || (bitOffset < k32BitSize), "");
    bool isOutOfRange = offsetVal < 0;
    if (!isOutOfRange) {
        isOutOfRange = (static_cast<unsigned long int>(offsetVal) >> k16BitSize) > 0;
    }
    if (isOutOfRange) {
        return false;
    }
    return ((bitOffset == k0BitSize || bitOffset == k16BitSize) ||
            (is64Bits && (bitOffset == k32BitSize || bitOffset == k48BitSize))) &&
           bitSize == k16BitSize;
}

/* we use the fact that A ^ B ^ A == B, A ^ 0 = A */
Operand *AArch64CGFunc::SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    uint32 bitOffset = node.GetBitsOffset();
    uint32 bitSize = node.GetBitsSize();
    PrimType regType = node.GetPrimType();
    bool is64Bits = GetPrimTypeBitSize(regType) == k64BitSize;
    /*
     * if operand 1 is immediate and fits in MOVK, use it
     * MOVK Wd, #imm{, LSL #shift} ; 32-bit general registers
     * MOVK Xd, #imm{, LSL #shift} ; 64-bit general registers
     */
    if (opnd1.IsIntImmediate() &&
        IsMoveWideKeepable(static_cast<ImmOperand &>(opnd1).GetValue(), bitOffset, bitSize, is64Bits)) {
        RegOperand &resOpnd = GetOrCreateResOperand(parent, regType);
        SelectCopy(resOpnd, regType, opnd0, regType);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn((is64Bits ? MOP_xmovkri16 : MOP_wmovkri16), resOpnd, opnd1,
                                                           *GetLogicalShiftLeftOperand(bitOffset, is64Bits)));
        return &resOpnd;
    } else {
        Operand &movOpnd = LoadIntoRegister(opnd1, regType);
        uint32 mopBfi = is64Bits ? MOP_xbfirri6i6 : MOP_wbfirri5i5;
        ImmOperand &immOpnd1 = CreateImmOperand(bitOffset, k8BitSize, false);
        ImmOperand &immOpnd2 = CreateImmOperand(bitSize, k8BitSize, false);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopBfi, opnd0, movOpnd, immOpnd1, immOpnd2));
        return &opnd0;
    }
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
    if (!IsPrimitiveVector(dtype)) {
        PrimType primType;
        if (IsPrimitiveFloat(dtype)) {
            primType = dtype;
        } else {
            primType = is64Bits ? (PTY_i64) : (PTY_i32); /* promoted type */
        }
        resOpnd = &GetOrCreateResOperand(parent, primType);
        SelectNeg(*resOpnd, opnd0, primType);
    } else {
        /* vector operand */
        resOpnd = SelectVectorNeg(dtype, &opnd0);
    }
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

Operand *AArch64CGFunc::SelectRecip(UnaryNode &node, Operand &src, const BaseNode &parent)
{
    /*
     * fconsts s15, #112
     * fdivs s0, s15, s0
     */
    PrimType dtype = node.GetPrimType();
    if (!IsPrimitiveFloat(dtype)) {
        DEBUG_ASSERT(false, "should be float type");
        return nullptr;
    }
    Operand &opnd0 = LoadIntoRegister(src, dtype);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, dtype);
    Operand *one = nullptr;
    if (GetPrimTypeBitSize(dtype) == k64BitSize) {
        MIRDoubleConst *c = memPool->New<MIRDoubleConst>(1.0, *GlobalTables::GetTypeTable().GetTypeTable().at(PTY_f64));
        one = SelectDoubleConst(*c, node);
    } else if (GetPrimTypeBitSize(dtype) == k32BitSize) {
        MIRFloatConst *c = memPool->New<MIRFloatConst>(1.0f, *GlobalTables::GetTypeTable().GetTypeTable().at(PTY_f32));
        one = SelectFloatConst(*c, node);
    } else {
        CHECK_FATAL(false, "we don't support half-precision fp operations yet");
    }
    SelectDiv(resOpnd, *one, opnd0, dtype);
    return &resOpnd;
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
        case PTY_a32:
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

Operand *AArch64CGFunc::SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrnNode, std::string name)
{
    BaseNode *argexpr = intrnNode.Opnd(0);
    PrimType ptype = argexpr->GetPrimType();
    Operand *opnd = HandleExpr(intrnNode, *argexpr);
    if (intrnNode.GetIntrinsic() == INTRN_C_ffs) {
        DEBUG_ASSERT(intrnNode.GetPrimType() == PTY_i32, "Unexpect Size");
        return SelectAArch64ffs(*opnd, ptype);
    }
    if (opnd->IsMemoryAccessOperand()) {
        RegOperand &ldDest = CreateRegisterOperandOfType(ptype);
        Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype), ptype), ldDest, *opnd);
        GetCurBB()->AppendInsn(insn);
        opnd = &ldDest;
    }
    std::vector<Operand *> opndVec;
    RegOperand *dst = &CreateRegisterOperandOfType(ptype);
    opndVec.push_back(dst);  /* result */
    opndVec.push_back(opnd); /* param 0 */
    SelectLibCall(name, opndVec, ptype, ptype);

    return dst;
}

Operand *AArch64CGFunc::SelectIntrinsicOpWithNParams(IntrinsicopNode &intrnNode, PrimType retType,
                                                     const std::string &name)
{
    MapleVector<BaseNode *> argNodes = intrnNode.GetNopnd();
    std::vector<Operand *> opndVec;
    std::vector<PrimType> opndTypes;
    RegOperand *retOpnd = &CreateRegisterOperandOfType(retType);
    opndVec.push_back(retOpnd);
    opndTypes.push_back(retType);

    for (BaseNode *argexpr : argNodes) {
        PrimType ptype = argexpr->GetPrimType();
        Operand *opnd = HandleExpr(intrnNode, *argexpr);
        if (opnd->IsMemoryAccessOperand()) {
            RegOperand &ldDest = CreateRegisterOperandOfType(ptype);
            Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype), ptype), ldDest, *opnd);
            GetCurBB()->AppendInsn(insn);
            opnd = &ldDest;
        }
        opndVec.push_back(opnd);
        opndTypes.push_back(ptype);
    }
    SelectLibCallNArg(name, opndVec, opndTypes, retType, false);

    return retOpnd;
}

/* According to  gcc.target/aarch64/ffs.c */
Operand *AArch64CGFunc::SelectAArch64ffs(Operand &argOpnd, PrimType argType)
{
    RegOperand &destOpnd = LoadIntoRegister(argOpnd, argType);
    uint32 argSize = GetPrimTypeBitSize(argType);
    DEBUG_ASSERT((argSize == k64BitSize || argSize == k32BitSize), "Unexpect arg type");
    /* cmp */
    ImmOperand &zeroOpnd = CreateImmOperand(0, argSize, false);
    Operand &rflag = GetOrCreateRflag();
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(argSize == k64BitSize ? MOP_xcmpri : MOP_wcmpri, rflag, destOpnd, zeroOpnd));
    /* rbit */
    RegOperand *tempResReg = &CreateVirtualRegisterOperand(NewVReg(kRegTyInt, GetPrimTypeSize(argType)));
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(argSize == k64BitSize ? MOP_xrbit : MOP_wrbit, *tempResReg, destOpnd));
    /* clz */
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(argSize == k64BitSize ? MOP_xclz : MOP_wclz, *tempResReg, *tempResReg));
    /* csincc */
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(argSize == k64BitSize ? MOP_xcsincrrrc : MOP_wcsincrrrc,
                                                       *tempResReg, GetZeroOpnd(k32BitSize), *tempResReg,
                                                       GetCondOperand(CC_EQ), rflag));
    return tempResReg;
}

Operand *AArch64CGFunc::SelectRoundLibCall(RoundType roundType, const TypeCvtNode &node, Operand &opnd0)
{
    PrimType ftype = node.FromType();
    PrimType rtype = node.GetPrimType();
    bool is64Bits = (ftype == PTY_f64);
    std::vector<Operand *> opndVec;
    RegOperand *resOpnd;
    if (is64Bits) {
        resOpnd = &GetOrCreatePhysicalRegisterOperand(D0, k64BitSize, kRegTyFloat);
    } else {
        resOpnd = &GetOrCreatePhysicalRegisterOperand(S0, k32BitSize, kRegTyFloat);
    }
    opndVec.push_back(resOpnd);
    RegOperand &regOpnd0 = LoadIntoRegister(opnd0, ftype);
    opndVec.push_back(&regOpnd0);
    std::string libName;
    if (roundType == kCeil) {
        libName.assign(is64Bits ? "ceil" : "ceilf");
    } else if (roundType == kFloor) {
        libName.assign(is64Bits ? "floor" : "floorf");
    } else {
        libName.assign(is64Bits ? "round" : "roundf");
    }
    SelectLibCall(libName, opndVec, ftype, rtype);

    return resOpnd;
}

Operand *AArch64CGFunc::SelectRoundOperator(RoundType roundType, const TypeCvtNode &node, Operand &opnd0,
                                            const BaseNode &parent)
{
    PrimType itype = node.GetPrimType();
    if ((mirModule.GetSrcLang() == kSrcLangC) && ((itype == PTY_f64) || (itype == PTY_f32))) {
        SelectRoundLibCall(roundType, node, opnd0);
    }
    PrimType ftype = node.FromType();
    DEBUG_ASSERT(((ftype == PTY_f64) || (ftype == PTY_f32)), "wrong float type");
    bool is64Bits = (ftype == PTY_f64);
    RegOperand &resOpnd = GetOrCreateResOperand(parent, itype);
    RegOperand &regOpnd0 = LoadIntoRegister(opnd0, ftype);
    MOperator mop = MOP_undef;
    if (roundType == kCeil) {
        mop = is64Bits ? MOP_xvcvtps : MOP_vcvtps;
    } else if (roundType == kFloor) {
        mop = is64Bits ? MOP_xvcvtms : MOP_vcvtms;
    } else {
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

Operand *AArch64CGFunc::SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    return SelectRoundOperator(kRound, node, opnd0, parent);
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
    if (IsPrimitiveVector(fromType) || IsPrimitiveVector(toType)) {
        return &LoadIntoRegister(opnd0, toType);
    }
    Operand::OperandType opnd0Type = opnd0.GetKind();
    RegOperand *resOpnd = &CreateRegisterOperandOfType(toType);
    if (IsPrimitiveInteger(fromType) || IsPrimitiveFloat(fromType)) {
        bool isFromInt = IsPrimitiveInteger(fromType);
        bool is64Bits = GetPrimTypeBitSize(fromType) == k64BitSize;
        PrimType itype =
            isFromInt ? ((GetPrimTypeBitSize(fromType) == k64BitSize) ? (IsSignedInteger(fromType) ? PTY_i64 : PTY_u64)
                                                                      : (IsSignedInteger(fromType) ? PTY_i32 : PTY_u32))
                      : (is64Bits ? PTY_f64 : PTY_f32);

        /*
         * if source operand is in memory,
         * simply read it as a value of 'toType 'into the dest operand
         * and return
         */
        if (opnd0Type == Operand::kOpdMem) {
            resOpnd = &SelectCopy(opnd0, toType, toType);
            return resOpnd;
        }
        /* according to aarch64 encoding format, convert int to float expression */
        bool isImm = false;
        ImmOperand *imm = static_cast<ImmOperand *>(&opnd0);
        uint64 val = static_cast<uint64>(imm->GetValue());
        uint64 canRepreset = is64Bits ? (val & 0xffffffffffff) : (val & 0x7ffff);
        uint32 val1 = is64Bits ? (val >> 61) & 0x3 : (val >> 29) & 0x3;
        uint32 val2 = is64Bits ? (val >> 54) & 0xff : (val >> 25) & 0x1f;
        bool isSame = is64Bits ? ((val2 == 0) || (val2 == 0xff)) : ((val2 == 0) || (val2 == 0x1f));
        canRepreset = (canRepreset == 0) && ((val1 & 0x1) ^ ((val1 & 0x2) >> 1)) && isSame;
        Operand *newOpnd0 = &opnd0;
        if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType) && canRepreset) {
            uint64 temp1 = is64Bits ? (val >> 63) << 7 : (val >> 31) << 7;
            uint64 temp2 = is64Bits ? val >> 48 : val >> 19;
            int64 imm8 = (temp2 & 0x7f) | temp1;
            newOpnd0 = &CreateImmOperand(imm8, k8BitSize, false, kNotVary, true);
            isImm = true;
        } else {
            newOpnd0 = &LoadIntoRegister(opnd0, itype);
        }
        if ((IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) ||
            (IsPrimitiveFloat(toType) && IsPrimitiveInteger(fromType))) {
            MOperator mopFmov = (isImm ? (is64Bits ? MOP_xdfmovri : MOP_wsfmovri) : isFromInt)
                                    ? (is64Bits ? MOP_xvmovdr : MOP_xvmovsr)
                                    : (is64Bits ? MOP_xvmovrd : MOP_xvmovrs);
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mopFmov, *resOpnd, *newOpnd0));
            return resOpnd;
        } else {
            return newOpnd0;
        }
    } else {
        CHECK_FATAL(false, "NYI retype");
    }
    return nullptr;
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
    if (fromType == PTY_i128 || fromType == PTY_u128) {
        fsize = k64BitSize;
    }
    uint32 tsize = GetPrimTypeBitSize(toType);
    if (toType == PTY_i128 || toType == PTY_u128) {
        tsize = k64BitSize;
    }
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
            if ((mirModule.IsJavaModule()) || (IsSignedInteger(fromType) == IsSignedInteger(toType)) ||
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
    } else if (IsPrimitiveVector(toType) || IsPrimitiveVector(fromType)) {
        CHECK_FATAL(IsPrimitiveVector(toType) && IsPrimitiveVector(fromType), "Invalid vector cvt operands");
        SelectVectorCvt(resOpnd, toType, &opnd0, fromType);
    } else { /* both are float type */
        SelectCvtFloat2Float(*resOpnd, opnd0, fromType, toType);
    }
    return resOpnd;
}

Operand *AArch64CGFunc::SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType ftype = node.FromType();
    bool is64Bits = (GetPrimTypeBitSize(node.GetPrimType()) == k64BitSize);
    PrimType itype = (is64Bits) ? (IsSignedInteger(node.GetPrimType()) ? PTY_i64 : PTY_u64)
                                : (IsSignedInteger(node.GetPrimType()) ? PTY_i32 : PTY_u32); /* promoted type */
    RegOperand &resOpnd = GetOrCreateResOperand(parent, itype);
    SelectCvtFloat2Int(resOpnd, opnd0, itype, ftype);
    return &resOpnd;
}

void AArch64CGFunc::SelectSelect(Operand &resOpnd, Operand &condOpnd, Operand &trueOpnd, Operand &falseOpnd,
                                 PrimType dtype, PrimType ctype, bool hasCompare, ConditionCode cc)
{
    DEBUG_ASSERT(&resOpnd != &condOpnd, "resOpnd cannot be the same as condOpnd");
    bool isIntType = IsPrimitiveInteger(dtype);
    DEBUG_ASSERT((IsPrimitiveInteger(dtype) || IsPrimitiveFloat(dtype)), "unknown type for select");
    // making condOpnd and cmpInsn closer will provide more opportunity for opt
    Operand &newTrueOpnd = LoadIntoRegister(trueOpnd, dtype);
    Operand &newFalseOpnd = LoadIntoRegister(falseOpnd, dtype);
    Operand &newCondOpnd = LoadIntoRegister(condOpnd, ctype);
    if (hasCompare) {
        SelectAArch64Cmp(newCondOpnd, CreateImmOperand(0, ctype, false), true, GetPrimTypeBitSize(ctype));
        cc = CC_NE;
    }
    Operand &newResOpnd = LoadIntoRegister(resOpnd, dtype);
    SelectAArch64Select(newResOpnd, newTrueOpnd, newFalseOpnd, GetCondOperand(cc), isIntType,
                        GetPrimTypeBitSize(dtype));
}

Operand *AArch64CGFunc::SelectSelect(TernaryNode &expr, Operand &cond, Operand &trueOpnd, Operand &falseOpnd,
                                     const BaseNode &parent, bool hasCompare)
{
    PrimType dtype = expr.GetPrimType();
    PrimType ctype = expr.Opnd(0)->GetPrimType();

    ConditionCode cc = CC_NE;
    Opcode opcode = expr.Opnd(0)->GetOpCode();
    PrimType cmpType = static_cast<CompareNode *>(expr.Opnd(0))->GetOpndType();
    bool isFloat = false;
    bool unsignedIntegerComparison = false;
    if (!IsPrimitiveVector(cmpType)) {
        isFloat = IsPrimitiveFloat(cmpType);
        unsignedIntegerComparison = !isFloat && !IsSignedInteger(cmpType);
    } else {
        isFloat = IsPrimitiveVectorFloat(cmpType);
        unsignedIntegerComparison = !isFloat && IsPrimitiveUnSignedVector(cmpType);
    }
    switch (opcode) {
        case OP_eq:
            cc = CC_EQ;
            break;
        case OP_ne:
            cc = CC_NE;
            break;
        case OP_le:
            cc = unsignedIntegerComparison ? CC_LS : CC_LE;
            break;
        case OP_ge:
            cc = unsignedIntegerComparison ? CC_HS : CC_GE;
            break;
        case OP_gt:
            cc = unsignedIntegerComparison ? CC_HI : CC_GT;
            break;
        case OP_lt:
            cc = unsignedIntegerComparison ? CC_LO : CC_LT;
            break;
        default:
            hasCompare = true;
            break;
    }
    if (!IsPrimitiveVector(dtype)) {
        RegOperand &resOpnd = GetOrCreateResOperand(parent, dtype);
        SelectSelect(resOpnd, cond, trueOpnd, falseOpnd, dtype, ctype, hasCompare, cc);
        return &resOpnd;
    } else {
        return SelectVectorSelect(cond, dtype, trueOpnd, falseOpnd);
    }
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

Operand *AArch64CGFunc::SelectLazyLoad(Operand &opnd0, PrimType primType)
{
    DEBUG_ASSERT(opnd0.IsRegister(), "wrong type.");
    RegOperand &resOpnd = CreateRegisterOperandOfType(primType);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_lazy_ldr, resOpnd, opnd0));
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType)
{
    StImmOperand &srcOpnd = CreateStImmOperand(st, offset, 0);
    RegOperand &resOpnd = CreateRegisterOperandOfType(primType);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_lazy_ldr_static, resOpnd, srcOpnd));
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType)
{
    StImmOperand &srcOpnd = CreateStImmOperand(st, offset, 0);
    RegOperand &resOpnd = CreateRegisterOperandOfType(primType);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_arrayclass_cache_ldr, resOpnd, srcOpnd));
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectAlloca(UnaryNode &node, Operand &opnd0)
{
    if (!CGOptions::IsArm64ilp32()) {
        DEBUG_ASSERT((node.GetPrimType() == PTY_a64), "wrong type");
    }
    if (GetCG()->IsLmbc()) {
        SetHasVLAOrAlloca(true);
    }
    PrimType stype = node.Opnd(0)->GetPrimType();
    Operand *resOpnd = &opnd0;
    if (GetPrimTypeBitSize(stype) < GetPrimTypeBitSize(PTY_u64)) {
        resOpnd = &CreateRegisterOperandOfType(PTY_u64);
        SelectCvtInt2Int(nullptr, resOpnd, &opnd0, stype, PTY_u64);
    }

    RegOperand &aliOp = CreateRegisterOperandOfType(PTY_u64);

    SelectAdd(aliOp, *resOpnd, CreateImmOperand(kAarch64StackPtrAlignment - 1, k64BitSize, true), PTY_u64);
    Operand &shifOpnd = CreateImmOperand(__builtin_ctz(kAarch64StackPtrAlignment), k64BitSize, true);
    SelectShift(aliOp, aliOp, shifOpnd, kShiftLright, PTY_u64);
    SelectShift(aliOp, aliOp, shifOpnd, kShiftLeft, PTY_u64);
    Operand &spOpnd = GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    SelectSub(spOpnd, spOpnd, aliOp, PTY_u64);
    int64 allocaOffset = GetMemlayout()->SizeOfArgsToStackPass();
    if (GetCG()->IsLmbc()) {
        allocaOffset -= kDivide2 * k8ByteSize;
    }
    if (allocaOffset > 0) {
        RegOperand &resallo = CreateRegisterOperandOfType(PTY_u64);
        SelectAdd(resallo, spOpnd, CreateImmOperand(allocaOffset, k64BitSize, true), PTY_u64);
        return &resallo;
    } else {
        return &SelectCopy(spOpnd, PTY_u64, PTY_u64);
    }
}

Operand *AArch64CGFunc::SelectMalloc(UnaryNode &node, Operand &opnd0)
{
    PrimType retType = node.GetPrimType();
    DEBUG_ASSERT((retType == PTY_a64), "wrong type");

    std::vector<Operand *> opndVec;
    RegOperand &resOpnd = CreateRegisterOperandOfType(retType);
    opndVec.emplace_back(&resOpnd);
    opndVec.emplace_back(&opnd0);
    /* Use calloc to make sure allocated memory is zero-initialized */
    const std::string &funcName = "calloc";
    PrimType srcPty = PTY_u64;
    if (opnd0.GetSize() <= k32BitSize) {
        srcPty = PTY_u32;
    }
    Operand &opnd1 = CreateImmOperand(1, srcPty, false);
    opndVec.emplace_back(&opnd1);
    SelectLibCall(funcName, opndVec, srcPty, retType);
    return &resOpnd;
}

Operand *AArch64CGFunc::SelectGCMalloc(GCMallocNode &node)
{
    PrimType retType = node.GetPrimType();
    DEBUG_ASSERT((retType == PTY_a64), "wrong type");

    /* Get the size and alignment of the type. */
    TyIdx tyIdx = node.GetTyIdx();
    uint64 size = GetBecommon().GetTypeSize(tyIdx);
    uint8 align = RTSupport::GetRTSupportInstance().GetObjectAlignment();

    /* Generate the call to MCC_NewObj */
    Operand &opndSize = CreateImmOperand(static_cast<int64>(size), k64BitSize, false);
    Operand &opndAlign = CreateImmOperand(align, k64BitSize, false);

    RegOperand &resOpnd = CreateRegisterOperandOfType(retType);

    std::vector<Operand *> opndVec {&resOpnd, &opndSize, &opndAlign};

    const std::string &funcName = "MCC_NewObj";
    SelectLibCall(funcName, opndVec, PTY_u64, retType);

    return &resOpnd;
}

Operand *AArch64CGFunc::SelectJarrayMalloc(JarrayMallocNode &node, Operand &opnd0)
{
    PrimType retType = node.GetPrimType();
    DEBUG_ASSERT((retType == PTY_a64), "wrong type");

    /* Extract jarray type */
    TyIdx tyIdx = node.GetTyIdx();
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    DEBUG_ASSERT(type != nullptr, "nullptr check");
    CHECK_FATAL(type->GetKind() == kTypeJArray, "expect MIRJarrayType");
    auto jaryType = static_cast<MIRJarrayType *>(type);
    uint64 fixedSize = RTSupport::GetRTSupportInstance().GetArrayContentOffset();
    uint8 align = RTSupport::GetRTSupportInstance().GetObjectAlignment();

    MIRType *elemType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(jaryType->GetElemTyIdx());
    PrimType elemPrimType = elemType->GetPrimType();
    uint64 elemSize = GetPrimTypeSize(elemPrimType);

    /* Generate the cal to MCC_NewObj_flexible */
    Operand &opndFixedSize = CreateImmOperand(PTY_u64, static_cast<int64>(fixedSize));
    Operand &opndElemSize = CreateImmOperand(PTY_u64, static_cast<int64>(elemSize));

    Operand *opndNElems = &opnd0;

    Operand *opndNElems64 = &static_cast<Operand &>(CreateRegisterOperandOfType(PTY_u64));
    SelectCvtInt2Int(nullptr, opndNElems64, opndNElems, PTY_u32, PTY_u64);

    Operand &opndAlign = CreateImmOperand(PTY_u64, align);

    RegOperand &resOpnd = CreateRegisterOperandOfType(retType);

    std::vector<Operand *> opndVec {&resOpnd, &opndFixedSize, &opndElemSize, opndNElems64, &opndAlign};

    const std::string &funcName = "MCC_NewObj_flexible";
    SelectLibCall(funcName, opndVec, PTY_u64, retType);

    /* Generate the store of the object length field */
    MemOperand &opndArrayLengthField =
        CreateMemOpnd(resOpnd, static_cast<int64>(RTSupport::GetRTSupportInstance().GetArrayLengthOffset()), k4BitSize);
    RegOperand *regOpndNElems = &SelectCopy(*opndNElems, PTY_u32, PTY_u32);
    DEBUG_ASSERT(regOpndNElems != nullptr, "null ptr check!");
    SelectCopy(opndArrayLengthField, PTY_u32, *regOpndNElems, PTY_u32);

    return &resOpnd;
}

bool AArch64CGFunc::IsRegRematCand(const RegOperand &reg) const
{
    MIRPreg *preg = GetPseudoRegFromVirtualRegNO(reg.GetRegisterNumber(), CGOptions::DoCGSSA());
    if (preg != nullptr && preg->GetOp() != OP_undef) {
        if (preg->GetOp() == OP_constval && cg->GetRematLevel() >= kRematConst) {
            return true;
        } else if (preg->GetOp() == OP_addrof && cg->GetRematLevel() >= kRematAddr) {
            return true;
        } else if (preg->GetOp() == OP_iread && cg->GetRematLevel() >= kRematDreadGlobal) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void AArch64CGFunc::ClearRegRematInfo(const RegOperand &reg) const
{
    MIRPreg *preg = GetPseudoRegFromVirtualRegNO(reg.GetRegisterNumber(), CGOptions::DoCGSSA());
    if (preg != nullptr && preg->GetOp() != OP_undef) {
        preg->SetOp(OP_undef);
    }
}

bool AArch64CGFunc::IsRegSameRematInfo(const RegOperand &regDest, const RegOperand &regSrc) const
{
    MIRPreg *pregDest = GetPseudoRegFromVirtualRegNO(regDest.GetRegisterNumber(), CGOptions::DoCGSSA());
    MIRPreg *pregSrc = GetPseudoRegFromVirtualRegNO(regSrc.GetRegisterNumber(), CGOptions::DoCGSSA());
    if (pregDest != nullptr && pregDest == pregSrc) {
        if (pregDest->GetOp() == OP_constval && cg->GetRematLevel() >= kRematConst) {
            return true;
        } else if (pregDest->GetOp() == OP_addrof && cg->GetRematLevel() >= kRematAddr) {
            return true;
        } else if (pregDest->GetOp() == OP_iread && cg->GetRematLevel() >= kRematDreadGlobal) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void AArch64CGFunc::ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn, regno_t destNO)
{
    auto opndNum = static_cast<int32>(insn.GetOperandSize());
    for (int i = opndNum - 1; i >= 0; --i) {
        Operand &opnd = insn.GetOperand(static_cast<uint32>(i));
        if (opnd.IsList()) {
            std::list<RegOperand *> tempRegStore;
            auto &opndList = static_cast<ListOperand &>(opnd).GetOperands();
            bool needReplace = false;
            for (auto it = opndList.cbegin(), end = opndList.cend(); it != end; ++it) {
                auto *regOpnd = *it;
                if (regOpnd->GetRegisterNumber() == destNO) {
                    needReplace = true;
                    tempRegStore.push_back(&regSrc);
                } else {
                    tempRegStore.push_back(regOpnd);
                }
            }
            if (needReplace) {
                opndList.clear();
                for (auto &newOpnd : std::as_const(tempRegStore)) {
                    static_cast<ListOperand &>(opnd).PushOpnd(*newOpnd);
                }
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            RegOperand *baseRegOpnd = memOpnd.GetBaseRegister();
            RegOperand *indexRegOpnd = memOpnd.GetIndexRegister();
            MemOperand *newMem = static_cast<MemOperand *>(memOpnd.Clone(*GetMemoryPool()));
            if ((baseRegOpnd != nullptr && baseRegOpnd->GetRegisterNumber() == destNO) ||
                (indexRegOpnd != nullptr && indexRegOpnd->GetRegisterNumber() == destNO)) {
                if (baseRegOpnd != nullptr && baseRegOpnd->GetRegisterNumber() == destNO) {
                    newMem->SetBaseRegister(regSrc);
                }
                if (indexRegOpnd != nullptr && indexRegOpnd->GetRegisterNumber() == destNO) {
                    auto *newRegSrc = static_cast<RegOperand *>(regSrc.Clone(*GetMemoryPool()));
                    newRegSrc->SetSize(indexRegOpnd->GetSize());  // retain the original size
                    newMem->SetIndexRegister(*newRegSrc);
                }
                insn.SetMemOpnd(newMem);
            }
        } else if (opnd.IsRegister()) {
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            if (regOpnd.GetRegisterNumber() == destNO) {
                DEBUG_ASSERT(regOpnd.GetRegisterNumber() != kRFLAG, "both condi and reg");
                if (regOpnd.GetBaseRefOpnd() != nullptr) {
                    regSrc.SetBaseRefOpnd(*regOpnd.GetBaseRefOpnd());
                    ReplaceRegReference(regOpnd.GetRegisterNumber(), regSrc.GetRegisterNumber());
                }
                CHECK_FATAL(regOpnd.GetSize() == regSrc.GetSize(), "NIY");
                insn.SetOperand(static_cast<uint32>(i), regSrc);
            }
            if (!IsStackMapComputed() && regOpnd.GetBaseRefOpnd() != nullptr &&
                regOpnd.GetBaseRefOpnd()->GetRegisterNumber() == destNO) {
                regOpnd.SetBaseRefOpnd(regSrc);
                ReplaceRegReference(regOpnd.GetBaseRefOpnd()->GetRegisterNumber(), regSrc.GetRegisterNumber());
            }
        }
    }
    if (!IsStackMapComputed() && insn.GetStackMap() != nullptr) {
        for (auto [deoptVreg, opnd] : insn.GetStackMap()->GetDeoptInfo().GetDeoptBundleInfo()) {
            if (!opnd->IsRegister()) {
                continue;
            }
            auto &regOpnd = static_cast<RegOperand &>(*opnd);
            if (regOpnd.GetRegisterNumber() == destNO) {
                insn.GetStackMap()->GetDeoptInfo().ReplaceDeoptBundleInfo(deoptVreg, regSrc);
                ReplaceRegReference(regOpnd.GetRegisterNumber(), regSrc.GetRegisterNumber());
            }
        }
    }
}

void AArch64CGFunc::CleanupDeadMov(bool dumpInfo)
{
    /* clean dead mov. */
    FOR_ALL_BB(bb, this)
    {
        FOR_BB_INSNS_SAFE(insn, bb, ninsn)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (insn->GetMachineOpcode() == MOP_xmovrr || insn->GetMachineOpcode() == MOP_wmovrr ||
                insn->GetMachineOpcode() == MOP_xvmovs || insn->GetMachineOpcode() == MOP_xvmovd) {
                RegOperand &regDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
                RegOperand &regSrc = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
                if (!regSrc.IsVirtualRegister() || !regDest.IsVirtualRegister()) {
                    continue;
                }

                if (regSrc.GetRegisterNumber() == regDest.GetRegisterNumber()) {
                    bb->RemoveInsn(*insn);
                } else if (insn->IsPhiMovInsn() && dumpInfo) {
                    LogInfo::MapleLogger() << "fail to remove mov: " << regDest.GetRegisterNumber() << " <- "
                                           << regSrc.GetRegisterNumber() << std::endl;
                }
            }
        }
    }
}

void AArch64CGFunc::GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realSaveRegs)
{
    auto *targetOpnd = insn.GetCallTargetOperand();
    CHECK_FATAL(targetOpnd != nullptr, "target is null in AArch64Insn::IsCallToFunctionThatNeverReturns");
    if (CGOptions::DoIPARA() && targetOpnd->IsFuncNameOpnd()) {
        FuncNameOperand *target = static_cast<FuncNameOperand *>(targetOpnd);
        const MIRSymbol *funcSt = target->GetFunctionSymbol();
        DEBUG_ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
        MIRFunction *func = funcSt->GetFunction();
        if (func != nullptr && func->IsReferedRegsValid()) {
            for (auto preg : func->GetReferedRegs()) {
                if (AArch64Abi::IsCallerSaveReg(static_cast<AArch64reg>(preg))) {
                    realSaveRegs.insert(preg);
                }
            }
            return;
        }
    }
    for (uint32 i = R0; i <= kMaxRegNum; ++i) {
        if (AArch64Abi::IsCallerSaveReg(static_cast<AArch64reg>(i))) {
            realSaveRegs.insert(i);
        }
    }
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

bool AArch64CGFunc::IsFrameReg(const RegOperand &opnd) const
{
    if (opnd.GetRegisterNumber() == RFP) {
        return true;
    } else {
        return false;
    }
}

bool AArch64CGFunc::IsSaveReg(const RegOperand &reg, MIRType &mirType, BECommon &cgBeCommon)
{
    CCImpl &retLocator = *GetOrCreateLocator(GetCurCallConvKind());
    CCLocInfo retMechanism;
    retLocator.LocateRetVal(mirType, retMechanism);
    if (retMechanism.GetRegCount() > 0) {
        return reg.GetRegisterNumber() == retMechanism.GetReg0() || reg.GetRegisterNumber() == retMechanism.GetReg1() ||
               reg.GetRegisterNumber() == retMechanism.GetReg2() || reg.GetRegisterNumber() == retMechanism.GetReg3();
    }
    return false;
}

bool AArch64CGFunc::IsSPOrFP(const RegOperand &opnd) const
{
    const RegOperand &regOpnd = static_cast<const RegOperand &>(opnd);
    regno_t regNO = opnd.GetRegisterNumber();
    return (regOpnd.IsPhysicalRegister() &&
            (regNO == RSP || regNO == RFP || (regNO == R29 && CGOptions::UseFramePointer())));
}

bool AArch64CGFunc::IsReturnReg(const RegOperand &opnd) const
{
    regno_t regNO = opnd.GetRegisterNumber();
    return (regNO == R0) || (regNO == V0);
}

/*
 * This function returns true to indicate that the clean up code needs to be generated,
 * otherwise it does not need. In GCOnly mode, it always returns false.
 */
bool AArch64CGFunc::NeedCleanup()
{
    if (CGOptions::IsGCOnly()) {
        return false;
    }
    AArch64MemLayout *layout = static_cast<AArch64MemLayout *>(GetMemlayout());
    if (layout->GetSizeOfRefLocals() > 0) {
        return true;
    }
    for (uint32 i = 0; i < GetFunction().GetFormalCount(); i++) {
        TypeAttrs ta = GetFunction().GetNthParamAttr(i);
        if (ta.GetAttr(ATTR_localrefvar)) {
            return true;
        }
    }

    return false;
}

/*
 * bb must be the cleanup bb.
 * this function must be invoked before register allocation.
 * extended epilogue is specific for fast exception handling and is made up of
 * clean up code and epilogue.
 * clean up code is generated here while epilogue is generated in GeneratePrologEpilog()
 */
void AArch64CGFunc::GenerateCleanupCodeForExtEpilog(BB &bb)
{
    DEBUG_ASSERT(GetLastBB()->GetPrev()->GetFirstStmt() == GetCleanupLabel(), "must be");

    if (NeedCleanup()) {
        /* this is necessary for code insertion. */
        SetCurBB(bb);

        RegOperand &regOpnd0 =
            GetOrCreatePhysicalRegisterOperand(R0, GetPointerSize() * kBitsPerByte, GetRegTyFromPrimTy(PTY_a64));
        RegOperand &regOpnd1 =
            GetOrCreatePhysicalRegisterOperand(R1, GetPointerSize() * kBitsPerByte, GetRegTyFromPrimTy(PTY_a64));
        /* allocate 16 bytes to store reg0 and reg1 (each reg has 8 bytes) */
        MemOperand &frameAlloc = CreateCallFrameOperand(-16, GetPointerSize() * kBitsPerByte);
        Insn &allocInsn = GetInsnBuilder()->BuildInsn(MOP_xstp, regOpnd0, regOpnd1, frameAlloc);
        allocInsn.SetDoNotRemove(true);
        AppendInstructionTo(allocInsn, *this);

        /* invoke MCC_CleanupLocalStackRef(). */
        HandleRCCall(false);
        /* deallocate 16 bytes which used to store reg0 and reg1 */
        MemOperand &frameDealloc = CreateCallFrameOperand(16, GetPointerSize() * kBitsPerByte);
        GenRetCleanup(cleanEANode, true);
        Insn &deallocInsn = GetInsnBuilder()->BuildInsn(MOP_xldp, regOpnd0, regOpnd1, frameDealloc);
        deallocInsn.SetDoNotRemove(true);
        AppendInstructionTo(deallocInsn, *this);
        /* Update cleanupbb since bb may have been splitted */
        SetCleanupBB(*GetCurBB());
    }
}

/*
 * bb must be the cleanup bb.
 * this function must be invoked before register allocation.
 */
void AArch64CGFunc::GenerateCleanupCode(BB &bb)
{
    DEBUG_ASSERT(GetLastBB()->GetPrev()->GetFirstStmt() == GetCleanupLabel(), "must be");
    if (!NeedCleanup()) {
        return;
    }

    /* this is necessary for code insertion. */
    SetCurBB(bb);

    /* R0 is lived-in for clean-up code, save R0 before invocation */
    RegOperand &livein = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_a64));

    if (!GetCG()->GenLocalRC()) {
        /* by pass local RC operations. */
    } else if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
        regno_t vreg = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
        RegOperand &backupRegOp = CreateVirtualRegisterOperand(vreg);
        backupRegOp.SetRegNotBBLocal();
        SelectCopy(backupRegOp, PTY_a64, livein, PTY_a64);

        /* invoke MCC_CleanupLocalStackRef(). */
        HandleRCCall(false);
        SelectCopy(livein, PTY_a64, backupRegOp, PTY_a64);
    } else {
        /*
         * Register Allocation for O0 can not handle this case, so use a callee saved register directly.
         * If yieldpoint is enabled, we use R20 instead R19.
         */
        AArch64reg backupRegNO = GetCG()->GenYieldPoint() ? R20 : R19;
        RegOperand &backupRegOp =
            GetOrCreatePhysicalRegisterOperand(backupRegNO, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
        SelectCopy(backupRegOp, PTY_a64, livein, PTY_a64);
        /* invoke MCC_CleanupLocalStackRef(). */
        HandleRCCall(false);
        SelectCopy(livein, PTY_a64, backupRegOp, PTY_a64);
    }

    /* invoke _Unwind_Resume */
    std::string funcName("_Unwind_Resume");
    MIRSymbol *sym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    sym->SetNameStrIdx(funcName);
    sym->SetStorageClass(kScText);
    sym->SetSKind(kStFunc);
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    srcOpnds->PushOpnd(livein);
    AppendCall(*sym, *srcOpnds);
    /*
     * this instruction is unreachable, but we need it as the return address of previous
     * "bl _Unwind_Resume" for stack unwinding.
     */
    Insn &nop = GetInsnBuilder()->BuildInsn(MOP_xblr, livein, *srcOpnds);
    GetCurBB()->AppendInsn(nop);
    GetCurBB()->SetHasCall();

    /* Update cleanupbb since bb may have been splitted */
    SetCleanupBB(*GetCurBB());
}

uint32 AArch64CGFunc::FloatParamRegRequired(MIRStructType *structType, uint32 &fpSize)
{
    AArch64CallConvImpl parmlocator(GetBecommon());
    return parmlocator.FloatParamRegRequired(*structType, fpSize);
}

/*
 * Map param registers to formals.  For small structs passed in param registers,
 * create a move to vreg since lmbc IR does not create a regassign for them.
 */
void AArch64CGFunc::AssignLmbcFormalParams()
{
    PrimType primType;
    uint32 offset;
    regno_t intReg = R0;
    regno_t fpReg = V0;
    for (auto param : GetLmbcParamVec()) {
        primType = param->GetPrimType();
        offset = param->GetOffset();
        if (param->IsReturn()) {
            param->SetRegNO(R8);
        } else if (IsPrimitiveInteger(primType)) {
            if (intReg > R7) {
                param->SetRegNO(0);
            } else {
                param->SetRegNO(intReg);
                if (!param->HasRegassign()) {
                    uint32 bytelen = GetPrimTypeSize(primType);
                    uint32 bitlen = bytelen * kBitsPerByte;
                    MemOperand *mOpnd = GenLmbcFpMemOperand(static_cast<int32>(offset), bytelen);
                    RegOperand &src = GetOrCreatePhysicalRegisterOperand(AArch64reg(intReg), bitlen, kRegTyInt);
                    MOperator mOp = PickStInsn(bitlen, primType);
                    Insn &store = GetInsnBuilder()->BuildInsn(mOp, src, *mOpnd);
                    GetCurBB()->AppendInsn(store);
                }
                intReg++;
            }
        } else if (IsPrimitiveFloat(primType)) {
            if (fpReg > V7) {
                param->SetRegNO(0);
            } else {
                param->SetRegNO(fpReg);
                if (!param->HasRegassign()) {
                    uint32 bytelen = GetPrimTypeSize(primType);
                    uint32 bitlen = bytelen * kBitsPerByte;
                    MemOperand *mOpnd = GenLmbcFpMemOperand(static_cast<int32>(offset), bytelen);
                    RegOperand &src = GetOrCreatePhysicalRegisterOperand(AArch64reg(fpReg), bitlen, kRegTyFloat);
                    MOperator mOp = PickStInsn(bitlen, primType);
                    Insn &store = GetInsnBuilder()->BuildInsn(mOp, src, *mOpnd);
                    GetCurBB()->AppendInsn(store);
                }
                fpReg++;
            }
        } else if (primType == PTY_agg) {
            if (param->IsPureFloat()) {
                uint32 numFpRegs = param->GetNumRegs();
                if ((fpReg + numFpRegs - kOneRegister) > V7) {
                    param->SetRegNO(0);
                } else {
                    param->SetRegNO(fpReg);
                    param->SetNumRegs(numFpRegs);
                    fpReg += numFpRegs;
                }
            } else if (param->GetSize() > k16ByteSize) {
                if (intReg > R7) {
                    param->SetRegNO(0);
                } else {
                    param->SetRegNO(intReg);
                    param->SetIsOnStack();
                    param->SetOnStackOffset(((intReg - R0 + fpReg) - V0) * k8ByteSize);
                    uint32 bytelen = GetPrimTypeSize(PTY_a64);
                    uint32 bitlen = bytelen * kBitsPerByte;
                    MemOperand *mOpnd = GenLmbcFpMemOperand(static_cast<int32>(param->GetOnStackOffset()), bytelen);
                    RegOperand &src = GetOrCreatePhysicalRegisterOperand(AArch64reg(intReg), bitlen, kRegTyInt);
                    MOperator mOp = PickStInsn(bitlen, PTY_a64);
                    Insn &store = GetInsnBuilder()->BuildInsn(mOp, src, *mOpnd);
                    GetCurBB()->AppendInsn(store);
                    intReg++;
                }
            } else if (param->GetSize() <= k8ByteSize) {
                if (intReg > R7) {
                    param->SetRegNO(0);
                } else {
                    param->SetRegNO(intReg);
                    param->SetNumRegs(kOneRegister);
                    intReg++;
                }
            } else {
                /* size > 8 && size <= 16 */
                if ((intReg + kOneRegister) > R7) {
                    param->SetRegNO(0);
                } else {
                    param->SetRegNO(intReg);
                    param->SetNumRegs(kTwoRegister);
                    intReg += kTwoRegister;
                }
            }
            if (param->GetRegNO() != 0) {
                for (uint32 i = 0; i < param->GetNumRegs(); ++i) {
                    PrimType pType = PTY_i64;
                    RegType rType = kRegTyInt;
                    uint32 rSize = k8ByteSize;
                    if (param->IsPureFloat()) {
                        rType = kRegTyFloat;
                        if (param->GetFpSize() <= k4ByteSize) {
                            pType = PTY_f32;
                            rSize = k4ByteSize;
                        } else {
                            pType = PTY_f64;
                        }
                    }
                    regno_t vreg = NewVReg(rType, rSize);
                    RegOperand &dest = GetOrCreateVirtualRegisterOperand(vreg);
                    RegOperand &src = GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(param->GetRegNO() + i),
                                                                         rSize * kBitsPerByte, rType);
                    SelectCopy(dest, pType, src, pType);
                    if (param->GetVregNO() == 0) {
                        param->SetVregNO(vreg);
                    }
                    Operand *memOpd = &CreateMemOpnd(RFP, offset + (i * rSize), rSize);
                    GetCurBB()->AppendInsn(
                        GetInsnBuilder()->BuildInsn(PickStInsn(rSize * kBitsPerByte, pType), dest, *memOpd));
                }
            }
        } else {
            CHECK_FATAL(false, "lmbc formal primtype not handled");
        }
    }
}

void AArch64CGFunc::LmbcGenSaveSpForAlloca()
{
    if (GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc || !HasVLAOrAlloca()) {
        return;
    }
    Operand &spOpnd = GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
    regno_t regno = NewVReg(kRegTyInt, GetPointerSize());
    RegOperand &spSaveOpnd = CreateVirtualRegisterOperand(regno);
    SetSpSaveReg(regno);
    Insn &save = GetInsnBuilder()->BuildInsn(MOP_xmovrr, spSaveOpnd, spOpnd);
    GetFirstBB()->AppendInsn(save);
    for (auto *retBB : GetExitBBsVec()) {
        Insn &restore = GetInsnBuilder()->BuildInsn(MOP_xmovrr, spOpnd, spSaveOpnd);
        retBB->AppendInsn(restore);
        restore.SetFrameDef(true);
    }
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
    AArch64reg reg;
    if (GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        reg = RSP;
    } else {
        reg = RFP;
    }
    MemOperand *memOp = CreateStackMemOpnd(reg, static_cast<int32>(offset), size);
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

void AArch64CGFunc::GenSaveMethodInfoCode(BB &bb)
{
    if (GetCG()->UseFastUnwind()) {
        BB *formerCurBB = GetCurBB();
        GetDummyBB()->ClearInsns();
        SetCurBB(*GetDummyBB());
        /*
         * FUNCATTR_bridge for function: Ljava_2Flang_2FString_3B_7CcompareTo_7C_28Ljava_2Flang_2FObject_3B_29I, to
         * exclude this funciton this function is a bridge function generated for Java Genetic
         */
        if ((GetFunction().GetAttr(FUNCATTR_native) || GetFunction().GetAttr(FUNCATTR_fast_native)) &&
            !GetFunction().GetAttr(FUNCATTR_critical_native) && !GetFunction().GetAttr(FUNCATTR_bridge)) {
            RegOperand &fpReg = GetOrCreatePhysicalRegisterOperand(RFP, GetPointerSize() * kBitsPerByte, kRegTyInt);

            ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
            RegOperand &parmRegOpnd1 = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt);
            srcOpnds->PushOpnd(parmRegOpnd1);
            Operand &immOpnd = CreateImmOperand(0, k64BitSize, false);
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadri64, parmRegOpnd1, immOpnd));
            RegOperand &parmRegOpnd2 = GetOrCreatePhysicalRegisterOperand(R1, k64BitSize, kRegTyInt);
            srcOpnds->PushOpnd(parmRegOpnd2);
            SelectCopy(parmRegOpnd2, PTY_a64, fpReg, PTY_a64);

            MIRSymbol *sym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
            std::string funcName("MCC_SetRiskyUnwindContext");
            sym->SetNameStrIdx(funcName);

            sym->SetStorageClass(kScText);
            sym->SetSKind(kStFunc);
            AppendCall(*sym, *srcOpnds);
            bb.SetHasCall();
        }

        bb.InsertAtBeginning(*GetDummyBB());
        SetCurBB(*formerCurBB);
    }
}

bool AArch64CGFunc::HasStackLoadStore()
{
    FOR_ALL_BB(bb, this)
    {
        FOR_BB_INSNS(insn, bb)
        {
            uint32 opndNum = insn->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                Operand &opnd = insn->GetOperand(i);
                if (opnd.IsMemoryAccessOperand()) {
                    auto &memOpnd = static_cast<MemOperand &>(opnd);
                    Operand *base = memOpnd.GetBaseRegister();

                    if ((base != nullptr) && base->IsRegister()) {
                        RegOperand *regOpnd = static_cast<RegOperand *>(base);
                        RegType regType = regOpnd->GetRegisterType();
                        uint32 regNO = regOpnd->GetRegisterNumber();
                        if (((regType != kRegTyCc) && ((regNO == RFP) || (regNO == RSP))) || (regType == kRegTyVary)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void AArch64CGFunc::GenerateYieldpoint(BB &bb)
{
    /* ldr wzr, [RYP]  # RYP hold address of the polling page. */
    auto &wzr = GetZeroOpnd(k32BitSize);
    auto &pollingPage = CreateMemOpnd(RYP, 0, k32BitSize);
    auto &yieldPoint = GetInsnBuilder()->BuildInsn(MOP_wldr, wzr, pollingPage);
    if (GetCG()->GenerateVerboseCG()) {
        yieldPoint.SetComment("yieldpoint");
    }
    bb.AppendInsn(yieldPoint);
}

Operand &AArch64CGFunc::GetTargetRetOperand(PrimType primType, int32 sReg)
{
    DEBUG_ASSERT(!IsInt128Ty(primType), "NIY");
    if (IsSpecialPseudoRegister(-sReg)) {
        return GetOrCreateSpecialRegisterOperand(sReg, primType);
    }
    bool useFpReg = !IsPrimitiveInteger(primType) || IsPrimitiveVectorFloat(primType);
    uint32 bitSize = GetPrimTypeBitSize(primType);
    bitSize = bitSize <= k32BitSize ? k32BitSize : bitSize;
    return GetOrCreatePhysicalRegisterOperand(useFpReg ? V0 : R0, bitSize, GetRegTyFromPrimTy(primType));
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
    DEBUG_ASSERT(GetCurBB()->GetPrev()->GetFirstStmt() == GetCleanupLabel(), "must be");

    uint32 exitBBSize = GetExitBBsVec().size();
    if (exitBBSize == 0) {
        return;
    }
    if ((exitBBSize == 1) && GetExitBB(0) == GetCurBB()) {
        return;
    }
    if (exitBBSize == 1) {
        BB *onlyExitBB = GetExitBB(0);
        BB *onlyExitBBNext = onlyExitBB->GetNext();
        StmtNode *stmt = onlyExitBBNext->GetFirstStmt();
        /* only deal with the return_BB in the middle */
        if (stmt != GetCleanupLabel()) {
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
    GetCleanupBB()->PrependBB(*retBB);

    GetExitBBsVec().clear();
    GetExitBBsVec().emplace_back(retBB);
}

void AArch64CGFunc::HandleRetCleanup(NaryStmtNode &retNode)
{
    if (!GetCG()->GenLocalRC()) {
        /* handle local rc is disabled. */
        return;
    }

    constexpr uint8 opSize = 11;
    Opcode ops[opSize] = {OP_label, OP_goto,      OP_brfalse, OP_brtrue, OP_return, OP_call,
                          OP_icall, OP_rangegoto, OP_catch,   OP_try,    OP_endtry};
    std::set<Opcode> branchOp(ops, ops + opSize);

    /* get cleanup intrinsic */
    bool found = false;
    StmtNode *cleanupNode = retNode.GetPrev();
    cleanEANode = nullptr;
    while (cleanupNode != nullptr) {
        if (branchOp.find(cleanupNode->GetOpCode()) != branchOp.end()) {
            if (cleanupNode->GetOpCode() == OP_call) {
                CallNode *callNode = static_cast<CallNode *>(cleanupNode);
                MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
                MIRSymbol *fsym = GetFunction().GetLocalOrGlobalSymbol(fn->GetStIdx(), false);
                if ((fsym->GetName() == "MCC_DecRef_NaiveRCFast") || (fsym->GetName() == "MCC_IncRef_NaiveRCFast") ||
                    (fsym->GetName() == "MCC_IncDecRef_NaiveRCFast") || (fsym->GetName() == "MCC_LoadRefStatic") ||
                    (fsym->GetName() == "MCC_LoadRefField") || (fsym->GetName() == "MCC_LoadReferentField") ||
                    (fsym->GetName() == "MCC_LoadRefField_NaiveRCFast") ||
                    (fsym->GetName() == "MCC_LoadVolatileField") ||
                    (fsym->GetName() == "MCC_LoadVolatileStaticField") || (fsym->GetName() == "MCC_LoadWeakField") ||
                    (fsym->GetName() == "MCC_CheckObjMem")) {
                    cleanupNode = cleanupNode->GetPrev();
                    continue;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if (cleanupNode->GetOpCode() == OP_intrinsiccall) {
            IntrinsiccallNode *tempNode = static_cast<IntrinsiccallNode *>(cleanupNode);
            if ((tempNode->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS) ||
                (tempNode->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP)) {
                GenRetCleanup(tempNode);
                if (cleanEANode != nullptr) {
                    GenRetCleanup(cleanEANode, true);
                }
                found = true;
                break;
            }
            if (tempNode->GetIntrinsic() == INTRN_MPL_CLEANUP_NORETESCOBJS) {
                cleanEANode = tempNode;
            }
        }
        cleanupNode = cleanupNode->GetPrev();
    }

    if (!found) {
        MIRSymbol *retRef = nullptr;
        if (retNode.NumOpnds() != 0) {
            retRef = GetRetRefSymbol(*static_cast<NaryStmtNode &>(retNode).Opnd(0));
        }
        HandleRCCall(false, retRef);
    }
}

bool AArch64CGFunc::GenRetCleanup(const IntrinsiccallNode *cleanupNode, bool forEA)
{
#undef CC_DEBUG_INFO

#ifdef CC_DEBUG_INFO
    LogInfo::MapleLogger() << "==============" << GetFunction().GetName() << "==============" << '\n';
#endif

    if (cleanupNode == nullptr) {
        return false;
    }

    int32 minByteOffset = INT_MAX;
    int32 maxByteOffset = 0;

    int32 skipIndex = -1;
    MIRSymbol *skipSym = nullptr;
    size_t refSymNum = 0;
    if (cleanupNode->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS) {
        refSymNum = cleanupNode->GetNopndSize();
        if (refSymNum < 1) {
            return true;
        }
    } else if (cleanupNode->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP) {
        refSymNum = cleanupNode->GetNopndSize();
        /* refSymNum == 0, no local refvars; refSymNum == 1 and cleanup skip, so nothing to do */
        if (refSymNum <= 1) {
            return true;
        }
        BaseNode *skipExpr = cleanupNode->Opnd(refSymNum - 1);

        CHECK_FATAL(skipExpr->GetOpCode() == OP_dread, "should be dread");
        DreadNode *refNode = static_cast<DreadNode *>(skipExpr);
        skipSym = GetFunction().GetLocalOrGlobalSymbol(refNode->GetStIdx());

        refSymNum -= 1;
    } else if (cleanupNode->GetIntrinsic() == INTRN_MPL_CLEANUP_NORETESCOBJS) {
        refSymNum = cleanupNode->GetNopndSize();
        /* the number of operands of intrinsic call INTRN_MPL_CLEANUP_NORETESCOBJS must be more than 1 */
        if (refSymNum <= 1) {
            return true;
        }
        BaseNode *skipexpr = cleanupNode->Opnd(0);
        CHECK_FATAL(skipexpr->GetOpCode() == OP_dread, "should be dread");
        DreadNode *refnode = static_cast<DreadNode *>(skipexpr);
        skipSym = GetFunction().GetLocalOrGlobalSymbol(refnode->GetStIdx());
    }

    /* now compute the offset range */
    std::vector<int32> offsets;
    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(this->GetMemlayout());
    for (size_t i = 0; i < refSymNum; ++i) {
        BaseNode *argExpr = cleanupNode->Opnd(i);
        CHECK_FATAL(argExpr->GetOpCode() == OP_dread, "should be dread");
        DreadNode *refNode = static_cast<DreadNode *>(argExpr);
        MIRSymbol *refSymbol = GetFunction().GetLocalOrGlobalSymbol(refNode->GetStIdx());
        if (memLayout->GetSymAllocTable().size() <= refSymbol->GetStIndex()) {
            ERR(kLncErr, "access memLayout->GetSymAllocTable() failed");
            return false;
        }
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(memLayout->GetSymAllocInfo(refSymbol->GetStIndex()));
        int32 tempOffset = GetBaseOffset(*symLoc);
        offsets.emplace_back(tempOffset);
#ifdef CC_DEBUG_INFO
        LogInfo::MapleLogger() << "refsym " << refSymbol->GetName() << " offset " << tempOffset << '\n';
#endif
        minByteOffset = (minByteOffset > tempOffset) ? tempOffset : minByteOffset;
        maxByteOffset = (maxByteOffset < tempOffset) ? tempOffset : maxByteOffset;
    }

    /* get the skip offset */
    int32 skipOffset = -1;
    if (skipSym != nullptr) {
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(memLayout->GetSymAllocInfo(skipSym->GetStIndex()));
        CHECK_FATAL(GetBaseOffset(*symLoc) < std::numeric_limits<int32>::max(), "out of range");
        skipOffset = GetBaseOffset(*symLoc);
        offsets.emplace_back(skipOffset);

#ifdef CC_DEBUG_INFO
        LogInfo::MapleLogger() << "skip " << skipSym->GetName() << " offset " << skipOffset << '\n';
#endif

        skipIndex = symLoc->GetOffset() / kAarch64OffsetAlign;
    }

    /* call runtime cleanup */
    if (minByteOffset < INT_MAX) {
        int32 refLocBase = memLayout->GetRefLocBaseLoc();
        uint32 refNum = memLayout->GetSizeOfRefLocals() / kAarch64OffsetAlign;
        CHECK_FATAL((static_cast<uint32_t>(refLocBase) + (refNum - 1) * kAarch64IntregBytelen) <
            static_cast<uint32_t>(std::numeric_limits<int32>::max()), "out of range");
        int32 refLocEnd = refLocBase + static_cast<int32>((refNum - 1) * kAarch64IntregBytelen);
        int32 realMin = minByteOffset < refLocBase ? refLocBase : minByteOffset;
        int32 realMax = maxByteOffset > refLocEnd ? refLocEnd : maxByteOffset;
        if (forEA) {
            std::sort(offsets.begin(), offsets.end());
            int32 prev = offsets[0];
            for (size_t i = 1; i < offsets.size(); i++) {
                CHECK_FATAL((offsets[i] == prev) || ((offsets[i] - prev) == kAarch64IntregBytelen), "must be");
                prev = offsets[i];
            }
            CHECK_FATAL((refLocBase - prev) == kAarch64IntregBytelen, "must be");
            realMin = minByteOffset;
            realMax = maxByteOffset;
        }
#ifdef CC_DEBUG_INFO
        LogInfo::MapleLogger() << " realMin " << realMin << " realMax " << realMax << '\n';
#endif
        if (realMax < realMin) {
            /* maybe there is a cleanup intrinsic bug, use CHECK_FATAL instead? */
            CHECK_FATAL(false, "must be");
        }

        /* optimization for little slot cleanup */
        if (realMax == realMin && !forEA) {
            RegOperand &phyOpnd = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
            Operand &stackLoc = CreateStkTopOpnd(static_cast<uint32>(realMin), GetPointerSize() * kBitsPerByte);
            Insn &ldrInsn = GetInsnBuilder()->BuildInsn(PickLdInsn(k64BitSize, PTY_a64), phyOpnd, stackLoc);
            GetCurBB()->AppendInsn(ldrInsn);

            ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
            srcOpnds->PushOpnd(phyOpnd);
            MIRSymbol *callSym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
            std::string funcName("MCC_DecRef_NaiveRCFast");
            callSym->SetNameStrIdx(funcName);
            callSym->SetStorageClass(kScText);
            callSym->SetSKind(kStFunc);
            Insn &callInsn = AppendCall(*callSym, *srcOpnds);
            callInsn.SetRefSkipIdx(skipIndex);
            GetCurBB()->SetHasCall();
            /* because of return stmt is often the last stmt */
            GetCurBB()->SetFrequency(frequency);

            return true;
        }
        ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());

        ImmOperand &beginOpnd = CreateImmOperand(realMin, k64BitSize, true);
        regno_t vRegNO0 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
        RegOperand &vReg0 = CreateVirtualRegisterOperand(vRegNO0);
        RegOperand &fpOpnd = GetOrCreateStackBaseRegOperand();
        SelectAdd(vReg0, fpOpnd, beginOpnd, PTY_i64);

        RegOperand &parmRegOpnd1 = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
        srcOpnds->PushOpnd(parmRegOpnd1);
        SelectCopy(parmRegOpnd1, PTY_a64, vReg0, PTY_a64);

        uint32 realRefNum = static_cast<uint32>((realMax - realMin) / kAarch64OffsetAlign + 1);

        ImmOperand &countOpnd = CreateImmOperand(realRefNum, k64BitSize, true);

        RegOperand &parmRegOpnd2 = GetOrCreatePhysicalRegisterOperand(R1, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
        srcOpnds->PushOpnd(parmRegOpnd2);
        SelectCopyImm(parmRegOpnd2, countOpnd, PTY_i64);

        MIRSymbol *funcSym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
        if ((skipSym != nullptr) && (skipOffset >= realMin) && (skipOffset <= realMax)) {
            /* call cleanupskip */
            uint32 stOffset = (skipOffset - realMin) / kAarch64OffsetAlign;
            ImmOperand &retLoc = CreateImmOperand(stOffset, k64BitSize, true);

            RegOperand &parmRegOpnd3 = GetOrCreatePhysicalRegisterOperand(R2, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
            srcOpnds->PushOpnd(parmRegOpnd3);
            SelectCopyImm(parmRegOpnd3, retLoc, PTY_i64);

            std::string funcName;
            if (forEA) {
                funcName = "MCC_CleanupNonRetEscObj";
            } else {
                funcName = "MCC_CleanupLocalStackRefSkip_NaiveRCFast";
            }
            funcSym->SetNameStrIdx(funcName);
#ifdef CC_DEBUG_INFO
            LogInfo::MapleLogger() << "num " << real_ref_num << " skip loc " << stOffset << '\n';
#endif
        } else {
            /* call cleanup */
            CHECK_FATAL(!forEA, "must be");
            std::string funcName("MCC_CleanupLocalStackRef_NaiveRCFast");
            funcSym->SetNameStrIdx(funcName);
#ifdef CC_DEBUG_INFO
            LogInfo::MapleLogger() << "num " << real_ref_num << '\n';
#endif
        }

        funcSym->SetStorageClass(kScText);
        funcSym->SetSKind(kStFunc);
        Insn &callInsn = AppendCall(*funcSym, *srcOpnds);
        callInsn.SetRefSkipIdx(skipIndex);
        GetCurBB()->SetHasCall();
        GetCurBB()->SetFrequency(frequency);
    }
    return true;
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

RegOperand &AArch64CGFunc::GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd)
{
    regno_t regNO = regOpnd.GetRegisterNumber();
    auto it = maplebe::VregInfo::vRegOperandTable.find(regNO);
    if (it != maplebe::VregInfo::vRegOperandTable.end()) {
        it->second->SetSize(regOpnd.GetSize());
        it->second->SetRegisterNumber(regNO);
        it->second->SetRegisterType(regOpnd.GetRegisterType());
        it->second->SetValidBitsNum(regOpnd.GetValidBitsNum());
        return *it->second;
    } else {
        auto *newRegOpnd = static_cast<RegOperand *>(regOpnd.Clone(*memPool));
        regno_t newRegNO = newRegOpnd->GetRegisterNumber();
        if (newRegNO >= GetMaxRegNum()) {
            SetMaxRegNum(newRegNO + kRegIncrStepLen);
            vReg.VRegTableResize(GetMaxRegNum());
        }
        maplebe::VregInfo::vRegOperandTable[newRegNO] = newRegOpnd;
        VirtualRegNode *vregNode = memPool->New<VirtualRegNode>(newRegOpnd->GetRegisterType(), newRegOpnd->GetSize());
        vReg.VRegTableElementSet(newRegNO, vregNode);
        vReg.SetCount(GetMaxRegNum());
        return *newRegOpnd;
    }
}

void AArch64CGFunc::HandleRCCall(bool begin, const MIRSymbol *retRef)
{
    if (!GetCG()->GenLocalRC() && !begin) {
        /* handle local rc is disabled. */
        return;
    }

    AArch64MemLayout *memLayout = static_cast<AArch64MemLayout *>(this->GetMemlayout());
    int32 refNum = static_cast<int32>(memLayout->GetSizeOfRefLocals() / kAarch64OffsetAlign);
    if (!refNum) {
        if (begin) {
            GenerateYieldpoint(*GetCurBB());
            yieldPointInsn = GetCurBB()->GetLastInsn();
        }
        return;
    }

    /* no MCC_CleanupLocalStackRefSkip when ret_ref is the only ref symbol */
    if ((refNum == 1) && (retRef != nullptr)) {
        if (begin) {
            GenerateYieldpoint(*GetCurBB());
            yieldPointInsn = GetCurBB()->GetLastInsn();
        }
        return;
    }
    CHECK_FATAL(refNum < 0xFFFF, "not enough room for size.");
    int32 refLocBase = memLayout->GetRefLocBaseLoc();
    CHECK_FATAL((refLocBase >= 0) && (refLocBase < 0xFFFF), "not enough room for offset.");
    int32 formalRef = 0;
    /* avoid store zero to formal localrefvars. */
    if (begin) {
        for (uint32 i = 0; i < GetFunction().GetFormalCount(); ++i) {
            if (GetFunction().GetNthParamAttr(i).GetAttr(ATTR_localrefvar)) {
                refNum--;
                formalRef++;
            }
        }
    }
    /*
     * if the number of local refvar is less than 12, use stp or str to init local refvar
     * else call function MCC_InitializeLocalStackRef to init.
     */
    if (begin && (refNum <= kRefNum12) &&
        ((refLocBase + kAarch64IntregBytelen * (refNum - 1)) < kStpLdpImm64UpperBound)) {
        int32 pairNum = refNum / kDivide2;
        int32 singleNum = refNum % kDivide2;
        const int32 pairRefBytes = 16; /* the size of each pair of ref is 16 bytes */
        int32 ind = 0;
        while (ind < pairNum) {
            int32 offset = memLayout->GetRefLocBaseLoc() + kAarch64IntregBytelen * formalRef + pairRefBytes * ind;
            Operand &zeroOp = GetZeroOpnd(k64BitSize);
            Operand &stackLoc = CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerSize() * kBitsPerByte);
            Insn &setInc = GetInsnBuilder()->BuildInsn(MOP_xstp, zeroOp, zeroOp, stackLoc);
            GetCurBB()->AppendInsn(setInc);
            ind++;
        }
        if (singleNum > 0) {
            int32 offset = memLayout->GetRefLocBaseLoc() + kAarch64IntregBytelen * formalRef +
                           kAarch64IntregBytelen * (refNum - 1);
            Operand &zeroOp = GetZeroOpnd(k64BitSize);
            Operand &stackLoc = CreateStkTopOpnd(static_cast<uint32>(offset), GetPointerSize() * kBitsPerByte);
            Insn &setInc = GetInsnBuilder()->BuildInsn(MOP_xstr, zeroOp, stackLoc);
            GetCurBB()->AppendInsn(setInc);
        }
        /* Insert Yield Point just after localrefvar are initialized. */
        GenerateYieldpoint(*GetCurBB());
        yieldPointInsn = GetCurBB()->GetLastInsn();
        return;
    }

    /* refNum is 1 and refvar is not returned, this refvar need to call MCC_DecRef_NaiveRCFast. */
    if ((refNum == 1) && !begin && (retRef == nullptr)) {
        RegOperand &phyOpnd = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
        Operand &stackLoc =
            CreateStkTopOpnd(static_cast<uint32>(memLayout->GetRefLocBaseLoc()), GetPointerSize() * kBitsPerByte);
        Insn &ldrInsn = GetInsnBuilder()->BuildInsn(PickLdInsn(k64BitSize, PTY_a64), phyOpnd, stackLoc);
        GetCurBB()->AppendInsn(ldrInsn);

        ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
        srcOpnds->PushOpnd(phyOpnd);
        MIRSymbol *callSym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
        std::string funcName("MCC_DecRef_NaiveRCFast");
        callSym->SetNameStrIdx(funcName);
        callSym->SetStorageClass(kScText);
        callSym->SetSKind(kStFunc);

        AppendCall(*callSym, *srcOpnds);
        GetCurBB()->SetHasCall();
        if (frequency != 0) {
            GetCurBB()->SetFrequency(frequency);
        }
        return;
    }

    /* refNum is 2 and one of refvar is returned, only another one is needed to call MCC_DecRef_NaiveRCFast. */
    if ((refNum == 2) && !begin && retRef != nullptr) {
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(memLayout->GetSymAllocInfo(retRef->GetStIndex()));
        int32 stOffset = symLoc->GetOffset() / kAarch64OffsetAlign;
        RegOperand &phyOpnd = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
        Operand *stackLoc = nullptr;
        if (stOffset == 0) {
            /* just have to Dec the next one. */
            stackLoc = &CreateStkTopOpnd(static_cast<uint32>(memLayout->GetRefLocBaseLoc()) + kAarch64IntregBytelen,
                                         GetPointerSize() * kBitsPerByte);
        } else {
            /* just have to Dec the current one. */
            stackLoc =
                &CreateStkTopOpnd(static_cast<uint32>(memLayout->GetRefLocBaseLoc()), GetPointerSize() * kBitsPerByte);
        }
        Insn &ldrInsn = GetInsnBuilder()->BuildInsn(PickLdInsn(k64BitSize, PTY_a64), phyOpnd, *stackLoc);
        GetCurBB()->AppendInsn(ldrInsn);

        ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
        srcOpnds->PushOpnd(phyOpnd);
        MIRSymbol *callSym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
        std::string funcName("MCC_DecRef_NaiveRCFast");
        callSym->SetNameStrIdx(funcName);
        callSym->SetStorageClass(kScText);
        callSym->SetSKind(kStFunc);
        Insn &callInsn = AppendCall(*callSym, *srcOpnds);
        callInsn.SetRefSkipIdx(stOffset);
        GetCurBB()->SetHasCall();
        if (frequency != 0) {
            GetCurBB()->SetFrequency(frequency);
        }
        return;
    }

    bool needSkip = false;
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());

    ImmOperand *beginOpnd =
        &CreateImmOperand(memLayout->GetRefLocBaseLoc() + kAarch64IntregBytelen * formalRef, k64BitSize, true);
    ImmOperand *countOpnd = &CreateImmOperand(refNum, k64BitSize, true);
    int32 refSkipIndex = -1;
    if (!begin && retRef != nullptr) {
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(memLayout->GetSymAllocInfo(retRef->GetStIndex()));
        int32 stOffset = symLoc->GetOffset() / kAarch64OffsetAlign;
        refSkipIndex = stOffset;
        if (stOffset == 0) {
            /* ret_ref at begin. */
            beginOpnd = &CreateImmOperand(memLayout->GetRefLocBaseLoc() + kAarch64IntregBytelen, k64BitSize, true);
            countOpnd = &CreateImmOperand(refNum - 1, k64BitSize, true);
        } else if (stOffset == (refNum - 1)) {
            /* ret_ref at end. */
            countOpnd = &CreateImmOperand(refNum - 1, k64BitSize, true);
        } else {
            needSkip = true;
        }
    }

    regno_t vRegNO0 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
    RegOperand &vReg0 = CreateVirtualRegisterOperand(vRegNO0);
    RegOperand &fpOpnd = GetOrCreateStackBaseRegOperand();
    SelectAdd(vReg0, fpOpnd, *beginOpnd, PTY_i64);

    RegOperand &parmRegOpnd1 = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
    srcOpnds->PushOpnd(parmRegOpnd1);
    SelectCopy(parmRegOpnd1, PTY_a64, vReg0, PTY_a64);

    regno_t vRegNO1 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
    RegOperand &vReg1 = CreateVirtualRegisterOperand(vRegNO1);
    SelectCopyImm(vReg1, *countOpnd, PTY_i64);

    RegOperand &parmRegOpnd2 = GetOrCreatePhysicalRegisterOperand(R1, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
    srcOpnds->PushOpnd(parmRegOpnd2);
    SelectCopy(parmRegOpnd2, PTY_a64, vReg1, PTY_a64);

    MIRSymbol *sym = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    if (begin) {
        std::string funcName("MCC_InitializeLocalStackRef");
        sym->SetNameStrIdx(funcName);
        CHECK_FATAL(countOpnd->GetValue() > 0, "refCount should be greater than 0.");
        refCount = static_cast<uint32>(countOpnd->GetValue());
        beginOffset = beginOpnd->GetValue();
    } else if (!needSkip) {
        std::string funcName("MCC_CleanupLocalStackRef_NaiveRCFast");
        sym->SetNameStrIdx(funcName);
    } else {
        CHECK_NULL_FATAL(retRef);
        if (retRef->GetStIndex() >= memLayout->GetSymAllocTable().size()) {
            CHECK_FATAL(false, "index out of range in AArch64CGFunc::HandleRCCall");
        }
        AArch64SymbolAlloc *symLoc =
            static_cast<AArch64SymbolAlloc *>(memLayout->GetSymAllocInfo(retRef->GetStIndex()));
        int32 stOffset = symLoc->GetOffset() / kAarch64OffsetAlign;
        ImmOperand &retLoc = CreateImmOperand(stOffset, k64BitSize, true);

        regno_t vRegNO2 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
        RegOperand &vReg2 = CreateVirtualRegisterOperand(vRegNO2);
        SelectCopyImm(vReg2, retLoc, PTY_i64);

        RegOperand &parmRegOpnd3 = GetOrCreatePhysicalRegisterOperand(R2, k64BitSize, GetRegTyFromPrimTy(PTY_a64));
        srcOpnds->PushOpnd(parmRegOpnd3);
        SelectCopy(parmRegOpnd3, PTY_a64, vReg2, PTY_a64);

        std::string funcName("MCC_CleanupLocalStackRefSkip_NaiveRCFast");
        sym->SetNameStrIdx(funcName);
    }
    sym->SetStorageClass(kScText);
    sym->SetSKind(kStFunc);

    Insn &callInsn = AppendCall(*sym, *srcOpnds);
    callInsn.SetRefSkipIdx(refSkipIndex);
    if (frequency != 0) {
        GetCurBB()->SetFrequency(frequency);
    }
    GetCurBB()->SetHasCall();
    if (begin) {
        /* Insert Yield Point just after localrefvar are initialized. */
        GenerateYieldpoint(*GetCurBB());
        yieldPointInsn = GetCurBB()->GetLastInsn();
    }
}

void AArch64CGFunc::SelectLibMemCopy(RegOperand &destOpnd, RegOperand &srcOpnd, uint32 structSize)
{
    std::vector<Operand *> opndVec;
    opndVec.push_back(&CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize)));  // result
    opndVec.push_back(&destOpnd);                                                      // param 0
    opndVec.push_back(&srcOpnd);                                                       // param 1
    opndVec.push_back(&CreateImmOperand(structSize, k64BitSize, false));               // param 2
    SelectLibCall("memcpy", opndVec, PTY_a64, PTY_a64);
}

void AArch64CGFunc::SelectInsnMemCopy(const MemOperand &destOpnd, const MemOperand &srcOpnd, uint32 size,
                                      bool isRefField, BaseNode *destNode, BaseNode *srcNode)
{
    auto createMemCopy = [this, isRefField, destNode, srcNode](MemOperand &dest, MemOperand &src, uint32 byteSize) {
        Insn *ldInsn = nullptr;
        Insn *stInsn = nullptr;

        if (byteSize == k16ByteSize) {  // ldp/stp
            auto &tmpOpnd1 = CreateRegisterOperandOfType(PTY_u64);
            auto &tmpOpnd2 = CreateRegisterOperandOfType(PTY_u64);
            // ldr srcMem to tmpReg
            ldInsn = &GetInsnBuilder()->BuildInsn(MOP_xldp, tmpOpnd1, tmpOpnd2, src);
            // str tempReg to destMem
            stInsn = &GetInsnBuilder()->BuildInsn(MOP_xstp, tmpOpnd1, tmpOpnd2, dest);
        } else {
            PrimType primType =
                (byteSize == k8ByteSize)
                    ? PTY_u64
                    : (byteSize == k4ByteSize)
                          ? PTY_u32
                          : (byteSize == k2ByteSize) ? PTY_u16 : (byteSize == k1ByteSize) ? PTY_u8 : PTY_unknown;
            DEBUG_ASSERT(primType != PTY_unknown, "NIY, unkown byte size");
            auto &tmpOpnd = CreateRegisterOperandOfType(kRegTyInt, byteSize);
            // ldr srcMem to tmpReg
            ldInsn = &GetInsnBuilder()->BuildInsn(PickLdInsn(byteSize * kBitsPerByte, primType), tmpOpnd, src);
            // str tempReg to destMem
            stInsn = &GetInsnBuilder()->BuildInsn(PickStInsn(byteSize * kBitsPerByte, primType), tmpOpnd, dest);
        }
        // Set memory reference info
        SetMemReferenceOfInsn(*ldInsn, srcNode);
        ldInsn->MarkAsAccessRefField(isRefField);
        GetCurBB()->AppendInsn(*ldInsn);
        if (!VERIFY_INSN(ldInsn)) {
            SPLIT_INSN(ldInsn, this);
        }

        // Set memory reference info
        SetMemReferenceOfInsn(*stInsn, destNode);
        GetCurBB()->AppendInsn(*stInsn);
        if (!VERIFY_INSN(stInsn)) {
            SPLIT_INSN(stInsn, this);
        }
    };

    // Insn can be reduced when sizeNotCoverd = 3 | 5 | 6 | 7
    for (uint32 offset = 0; offset < size;) {
        auto sizeNotCoverd = size - offset;
        uint32 copyByteSize = 0;
        if (destOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li &&
            srcOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li &&
            sizeNotCoverd >= k16ByteSize) {  // ldp/stp unsupport lo12li mem
            copyByteSize = k16ByteSize;
        } else if (sizeNotCoverd >= k8ByteSize) {
            copyByteSize = k8ByteSize;
        } else if (destOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li &&
                   srcOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li && sizeNotCoverd > k4ByteSize &&
                   offset >= k8ByteSize - sizeNotCoverd) {
            copyByteSize = k8ByteSize;  // 5: w + b -> x ; 6: w + h -> x ; 7: w + h + b -> x
            offset -= (k8ByteSize - sizeNotCoverd);
        } else if (sizeNotCoverd >= k4ByteSize) {
            copyByteSize = k4ByteSize;
        } else if (destOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li &&
                   srcOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li && sizeNotCoverd > k2ByteSize &&
                   offset >= k4ByteSize - sizeNotCoverd) {
            copyByteSize = k4ByteSize;  // 3: h + b -> w
            offset -= (k4ByteSize - sizeNotCoverd);
        } else if (sizeNotCoverd >= k2ByteSize) {
            copyByteSize = k2ByteSize;
        } else {
            copyByteSize = k1ByteSize;
        }

        auto &srcMem = GetMemOperandAddOffset(srcOpnd, offset, copyByteSize * kBitsPerByte);
        auto &destMem = GetMemOperandAddOffset(destOpnd, offset, copyByteSize * kBitsPerByte);
        createMemCopy(destMem, srcMem, copyByteSize);

        offset += copyByteSize;
    }
}

void AArch64CGFunc::SelectMemCopy(const MemOperand &destOpnd, const MemOperand &srcOpnd, uint32 size, bool isRefField,
                                  BaseNode *destNode, BaseNode *srcNode)
{
    if (size > kParmMemcpySize) {
        SelectLibMemCopy(*ExtractMemBaseAddr(destOpnd), *ExtractMemBaseAddr(srcOpnd), size);
        return;
    }
    SelectInsnMemCopy(destOpnd, srcOpnd, size, isRefField, destNode, srcNode);
}

void AArch64CGFunc::SelectParmListPreprocessForAggregate(BaseNode &argExpr, int32 &structCopyOffset,
                                                         std::vector<ParamDesc> &argsDesc, bool isArgUnused)
{
    MemRWNodeHelper memHelper(argExpr, GetFunction(), GetBecommon());
    auto mirSize = memHelper.GetMemSize();
    if (mirSize <= k16BitSize) {
        argsDesc.emplace_back(memHelper.GetMIRType(), &argExpr, memHelper.GetByteOffset());
        return;
    }

    PrimType baseType = PTY_begin;
    size_t elemNum = 0;
    if (IsHomogeneousAggregates(*memHelper.GetMIRType(), baseType, elemNum)) {
        // B.3 If the argument type is an HFA or an HVA, then the argument is used unmodified.
        argsDesc.emplace_back(memHelper.GetMIRType(), &argExpr, memHelper.GetByteOffset());
        return;
    }

    // B.4  If the argument type is a Composite Type that is larger than 16 bytes,
    //      then the argument is copied to memory allocated by the caller
    //      and the argument is replaced by a pointer to the copy.
    structCopyOffset = static_cast<int32>(RoundUp(static_cast<uint32>(structCopyOffset), k8ByteSize));

    // pre copy
    auto *rhsMemOpnd = SelectRhsMemOpnd(argExpr);
    if (!isArgUnused) {
        auto &spReg = GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
        auto &offsetOpnd = CreateImmOperand(structCopyOffset, k64BitSize, false);
        auto *destMemOpnd = CreateMemOperand(k64BitSize, spReg, offsetOpnd, false);

        auto copySize = CGOptions::IsBigEndian() ? static_cast<uint32>(RoundUp(mirSize, k8ByteSize)) : mirSize;
        SelectMemCopy(*destMemOpnd, *rhsMemOpnd, copySize);
    }

    (void)argsDesc.emplace_back(memHelper.GetMIRType(), nullptr, static_cast<uint32>(structCopyOffset), true);
    structCopyOffset += static_cast<int32>(RoundUp(mirSize, k8ByteSize));
}

// Stage B - Pre-padding and extension of arguments
bool AArch64CGFunc::SelectParmListPreprocess(StmtNode &naryNode, size_t start, std::vector<ParamDesc> &argsDesc,
                                             const MIRFunction *callee)
{
    bool hasSpecialArg = false;
    int32 structCopyOffset = GetMaxParamStackSize() - GetStructCopySize();
    for (size_t i = start; i < naryNode.NumOpnds(); ++i) {
        BaseNode *argExpr = naryNode.Opnd(i);
        DEBUG_ASSERT(argExpr != nullptr, "not null check");
        PrimType primType = argExpr->GetPrimType();
        DEBUG_ASSERT(primType != PTY_void, "primType should not be void");
        if (primType == PTY_agg) {
            SelectParmListPreprocessForAggregate(*argExpr, structCopyOffset, argsDesc,
                                                 (callee && callee->GetFuncDesc().IsArgUnused(i)));
        } else {
            auto *mirType = GlobalTables::GetTypeTable().GetPrimType(primType);
            (void)argsDesc.emplace_back(mirType, argExpr);
        }

        if (MarkParmListCall(*argExpr)) {
            argsDesc.rbegin()->isSpecialArg = true;
            hasSpecialArg = true;
        }
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

void AArch64CGFunc::SelectParmListSmallStruct(const MIRType &mirType, const CCLocInfo &ploc, Operand &addr,
                                              ListOperand &srcOpnds, bool isSpecialArg,
                                              std::vector<RegMapForPhyRegCpy> &regMapForTmpBB)
{
    uint32 offset = 0;
    DEBUG_ASSERT(addr.IsMemoryAccessOperand(), "NIY, must be mem opnd");
    uint64 size = mirType.GetSize();
    auto &memOpnd = static_cast<MemOperand &>(addr);
    // ldr memOpnd to parmReg
    auto loadParamFromMem = [this, &offset, &memOpnd, &srcOpnds, &size, isSpecialArg,
                             &regMapForTmpBB](AArch64reg regno, PrimType primType) {
        auto &phyReg =
            GetOrCreatePhysicalRegisterOperand(regno, GetPrimTypeBitSize(primType), GetRegTyFromPrimTy(primType));
        bool isFpReg = !IsPrimitiveInteger(primType) || IsPrimitiveVectorFloat(primType);
        if (!CGOptions::IsBigEndian() && !isFpReg && (size - offset < k8ByteSize)) {
            // load exact size agg (BigEndian not support yet)
            RegOperand *valOpnd = nullptr;
            for (uint32 exactOfst = 0; exactOfst < (size - offset);) {
                PrimType exactPrimType;
                auto loadSize = size - offset - exactOfst;
                if (loadSize >= k4ByteSize) {
                    exactPrimType = PTY_u32;
                } else if (loadSize >= k2ByteSize) {
                    exactPrimType = PTY_u16;
                } else {
                    exactPrimType = PTY_u8;
                }
                auto ldBitSize = GetPrimTypeBitSize(exactPrimType);
                auto *ldOpnd = &GetMemOperandAddOffset(memOpnd, exactOfst + offset, ldBitSize);
                auto ldMop = PickLdInsn(ldBitSize, exactPrimType);
                auto &tmpValOpnd = CreateVirtualRegisterOperand(NewVReg(kRegTyInt, GetPrimTypeSize(exactPrimType)));
                Insn &ldInsn = GetInsnBuilder()->BuildInsn(ldMop, tmpValOpnd, *ldOpnd);
                GetCurBB()->AppendInsn(ldInsn);
                if (!VERIFY_INSN(&ldInsn)) {
                    SPLIT_INSN(&ldInsn, this);
                }

                if (exactOfst != 0) {
                    auto &shiftOpnd = CreateImmOperand(exactOfst * kBitsPerByte, k32BitSize, false);
                    SelectShift(tmpValOpnd, tmpValOpnd, shiftOpnd, kShiftLeft, primType);
                }
                if (valOpnd) {
                    SelectBior(*valOpnd, *valOpnd, tmpValOpnd, primType);
                } else {
                    valOpnd = &tmpValOpnd;
                }
                exactOfst += GetPrimTypeSize(exactPrimType);
            }
            if (isSpecialArg) {
                regMapForTmpBB.emplace_back(RegMapForPhyRegCpy(&phyReg, primType, valOpnd, primType));
            } else {
                SelectCopy(phyReg, primType, *valOpnd, primType);
            }
        } else {
            auto *ldOpnd = &GetMemOperandAddOffset(memOpnd, offset, GetPrimTypeBitSize(primType));
            auto ldMop = PickLdInsn(GetPrimTypeBitSize(primType), primType);
            if (isSpecialArg) {
                RegOperand *tmpReg = &CreateRegisterOperandOfType(primType);
                Insn &tmpLdInsn = GetInsnBuilder()->BuildInsn(ldMop, *tmpReg, *ldOpnd);
                regMapForTmpBB.emplace_back(RegMapForPhyRegCpy(&phyReg, primType, tmpReg, primType));
                GetCurBB()->AppendInsn(tmpLdInsn);
                if (!VERIFY_INSN(&tmpLdInsn)) {
                    SPLIT_INSN(&tmpLdInsn, this);
                }
            } else {
                Insn &ldInsn = GetInsnBuilder()->BuildInsn(ldMop, phyReg, *ldOpnd);
                GetCurBB()->AppendInsn(ldInsn);
                if (!VERIFY_INSN(&ldInsn)) {
                    SPLIT_INSN(&ldInsn, this);
                }
            }
        }
        srcOpnds.PushOpnd(phyReg);
        offset += GetPrimTypeSize(primType);
    };
    loadParamFromMem(static_cast<AArch64reg>(ploc.reg0), ploc.primTypeOfReg0);
    if (ploc.reg1 != kRinvalid) {
        loadParamFromMem(static_cast<AArch64reg>(ploc.reg1), ploc.primTypeOfReg1);
    }
    if (ploc.reg2 != kRinvalid) {
        loadParamFromMem(static_cast<AArch64reg>(ploc.reg2), ploc.primTypeOfReg2);
    }
    if (ploc.reg3 != kRinvalid) {
        loadParamFromMem(static_cast<AArch64reg>(ploc.reg3), ploc.primTypeOfReg3);
    }
}

void AArch64CGFunc::SelectParmListPassByStack(const MIRType &mirType, Operand &opnd, uint32 memOffset, bool preCopyed,
                                              std::vector<Insn *> &insnForStackArgs)
{
    if (!preCopyed && mirType.GetPrimType() == PTY_agg) {
        DEBUG_ASSERT(opnd.IsMemoryAccessOperand(), "NIY, must be mem opnd");
        auto &actOpnd = CreateMemOpnd(RSP, memOffset, k64BitSize);
        auto mirSize = CGOptions::IsBigEndian() ? static_cast<uint32>(RoundUp(mirType.GetSize(), k8ByteSize))
                                                : static_cast<uint32>(mirType.GetSize());
        // can't use libmemcpy, it will modify parm reg
        SelectInsnMemCopy(actOpnd, static_cast<MemOperand &>(opnd), mirSize);
        return;
    }

    if (IsInt128Ty(mirType.GetPrimType())) {
        DEBUG_ASSERT(!preCopyed, "NIY");
        MemOperand &mem = CreateMemOpnd(RSP, memOffset, GetPrimTypeBitSize(PTY_u128));
        mem.SetStackArgMem(true);
        SelectCopy(mem, PTY_u128, opnd, PTY_u128);
        return;
    }

    PrimType primType = preCopyed ? PTY_a64 : mirType.GetPrimType();
    CHECK_FATAL(primType != PTY_i128 && primType != PTY_u128, "NIY, i128 is unsupported");
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

/* preprocess call in parmlist */
bool AArch64CGFunc::MarkParmListCall(BaseNode &expr)
{
    if (!CGOptions::IsPIC()) {
        return false;
    }
    switch (expr.GetOpCode()) {
        case OP_addrof: {
            auto &addrNode = static_cast<AddrofNode &>(expr);
            MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(addrNode.GetStIdx());
            if (symbol->IsThreadLocal()) {
                return true;
            }
            break;
        }
        default: {
            for (auto i = 0; i < expr.GetNumOpnds(); i++) {
                if (expr.Opnd(i)) {
                    if (MarkParmListCall(*expr.Opnd(i))) {
                        return true;
                    }
                }
            }
            break;
        }
    }
    return false;
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
        } else if (mirType->GetPrimType() == PTY_agg) {
            opnd = SelectRhsMemOpnd(*argsDesc[i].argExpr);
        } else {  // base type, clac true val
            opnd = &LoadIntoRegister(*HandleExpr(naryNode, *argsDesc[i].argExpr), mirType->GetPrimType());
        }
        parmLocator.LocateNextParm(*mirType, ploc, (i == 0), calleeType);

        // skip unused args
        if (callee && callee->GetFuncDesc().IsArgUnused(i)) {
            continue;
        }

        if (ploc.reg0 != kRinvalid) {  // load to the register.
            if (mirType->GetPrimType() == PTY_agg && !preCopyed) {
                SelectParmListSmallStruct(*mirType, ploc, *opnd, srcOpnds, argsDesc[i].isSpecialArg, regMapForTmpBB);
            } else if (IsInt128Ty(mirType->GetPrimType())) {
                SelectParmListForInt128(*opnd, srcOpnds, ploc, argsDesc[i].isSpecialArg, regMapForTmpBB);
            } else {
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
            }
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
/*
 * for MCC_DecRefResetPair(addrof ptr %Reg17_R5592, addrof ptr %Reg16_R6202) or
 * MCC_ClearLocalStackRef(addrof ptr %Reg17_R5592), the parameter (addrof ptr xxx) is converted to asm as follow:
 * add vreg, x29, #imm
 * mov R0/R1, vreg
 * this function is used to prepare parameters, the generated vreg is returned, and #imm is saved in offsetValue.
 */
Operand *AArch64CGFunc::SelectClearStackCallParam(const AddrofNode &expr, int64 &offsetValue)
{
    MIRSymbol *symbol = GetMirModule().CurFunction()->GetLocalOrGlobalSymbol(expr.GetStIdx());
    PrimType ptype = expr.GetPrimType();
    regno_t vRegNO = NewVReg(kRegTyInt, GetPrimTypeSize(ptype));
    Operand &result = CreateVirtualRegisterOperand(vRegNO);
    CHECK_FATAL(expr.GetFieldID() == 0, "the fieldID of parameter in clear stack reference call must be 0");
    if (!CGOptions::IsQuiet()) {
        maple::LogInfo::MapleLogger(kLlErr)
            << "Warning: we expect AddrOf with StImmOperand is not used for local variables";
    }
    auto *symLoc = static_cast<AArch64SymbolAlloc *>(GetMemlayout()->GetSymAllocInfo(symbol->GetStIndex()));
    ImmOperand *offset = nullptr;
    if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsArgsStkPassed) {
        offset = &CreateImmOperand(GetBaseOffset(*symLoc), k64BitSize, false, kUnAdjustVary);
    } else if (symLoc->GetMemSegment()->GetMemSegmentKind() == kMsRefLocals) {
        auto it = immOpndsRequiringOffsetAdjustmentForRefloc.find(symLoc);
        if (it != immOpndsRequiringOffsetAdjustmentForRefloc.end()) {
            offset = (*it).second;
        } else {
            offset = &CreateImmOperand(GetBaseOffset(*symLoc), k64BitSize, false);
            immOpndsRequiringOffsetAdjustmentForRefloc[symLoc] = offset;
        }
    } else {
        CHECK_FATAL(false, "the symLoc of parameter in clear stack reference call is unreasonable");
    }
    DEBUG_ASSERT(offset != nullptr, "offset should not be nullptr");
    offsetValue = offset->GetValue();
    SelectAdd(result, *GetBaseReg(*symLoc), *offset, PTY_u64);
    if (GetCG()->GenerateVerboseCG()) {
        /* Add a comment */
        Insn *insn = GetCurBB()->GetLastInsn();
        std::string comm = "local/formal var: ";
        comm += symbol->GetName();
        insn->SetComment(comm);
    }
    return &result;
}

/* select paramters for MCC_DecRefResetPair and MCC_ClearLocalStackRef function */
void AArch64CGFunc::SelectClearStackCallParmList(const StmtNode &naryNode, ListOperand &srcOpnds,
                                                 std::vector<int64> &stackPostion)
{
    CHECK_FATAL(false, "should not go here");
    AArch64CallConvImpl parmLocator(GetBecommon());
    CCLocInfo ploc;
    for (size_t i = 0; i < naryNode.NumOpnds(); ++i) {
        MIRType *ty = nullptr;
        BaseNode *argExpr = naryNode.Opnd(i);
        PrimType primType = argExpr->GetPrimType();
        DEBUG_ASSERT(primType != PTY_void, "primType check");
        /* use alloc */
        CHECK_FATAL(primType != PTY_agg, "the type of argument is unreasonable");
        ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<uint32>(primType)];
        CHECK_FATAL(argExpr->GetOpCode() == OP_addrof, "the argument of clear stack call is unreasonable");
        auto *expr = static_cast<AddrofNode *>(argExpr);
        int64 offsetValue = 0;
        Operand *opnd = SelectClearStackCallParam(*expr, offsetValue);
        stackPostion.emplace_back(offsetValue);
        auto *expRegOpnd = static_cast<RegOperand *>(opnd);
        parmLocator.LocateNextParm(*ty, ploc);
        CHECK_FATAL(ploc.reg0 != 0, "the parameter of ClearStackCall must be passed by register");
        CHECK_FATAL(expRegOpnd != nullptr, "null ptr check");
        RegOperand &parmRegOpnd = GetOrCreatePhysicalRegisterOperand(
            static_cast<AArch64reg>(ploc.reg0), expRegOpnd->GetSize(), GetRegTyFromPrimTy(primType));
        SelectCopy(parmRegOpnd, primType, *expRegOpnd, primType);
        srcOpnds.PushOpnd(parmRegOpnd);
        DEBUG_ASSERT(ploc.reg1 == 0, "SelectCall NYI");
    }
}

/*
 * intrinsify Unsafe.getAndAddInt and Unsafe.getAndAddLong
 * generate an intrinsic instruction instead of a function call
 * intrinsic_get_add_int w0, xt, ws, ws, x1, x2, w3, label
 */
void AArch64CGFunc::IntrinsifyGetAndAddInt(ListOperand &srcOpnds, PrimType pty)
{
    MapleList<RegOperand *> &opnds = srcOpnds.GetOperands();
    /* Unsafe.getAndAddInt has more than 4 parameters */
    DEBUG_ASSERT(opnds.size() >= 4, "ensure the operands number");
    auto iter = opnds.begin();
    RegOperand *objOpnd = *(++iter);
    RegOperand *offOpnd = *(++iter);
    RegOperand *deltaOpnd = *(++iter);
    auto &retVal = static_cast<RegOperand &>(GetTargetRetOperand(pty, -1));
    LabelIdx labIdx = CreateLabel();
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(labIdx);
    RegOperand &tempOpnd0 = CreateRegisterOperandOfType(PTY_i64);
    RegOperand &tempOpnd1 = CreateRegisterOperandOfType(pty);
    RegOperand &tempOpnd2 = CreateRegisterOperandOfType(PTY_i32);
    MOperator mOp = (pty == PTY_i64) ? MOP_get_and_addL : MOP_get_and_addI;
    std::vector<Operand *> intrnOpnds;
    intrnOpnds.emplace_back(&retVal);
    intrnOpnds.emplace_back(&tempOpnd0);
    intrnOpnds.emplace_back(&tempOpnd1);
    intrnOpnds.emplace_back(&tempOpnd2);
    intrnOpnds.emplace_back(objOpnd);
    intrnOpnds.emplace_back(offOpnd);
    intrnOpnds.emplace_back(deltaOpnd);
    intrnOpnds.emplace_back(&targetOpnd);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, intrnOpnds));
}

/*
 * intrinsify Unsafe.getAndSetInt and Unsafe.getAndSetLong
 * generate an intrinsic instruction instead of a function call
 */
void AArch64CGFunc::IntrinsifyGetAndSetInt(ListOperand &srcOpnds, PrimType pty)
{
    MapleList<RegOperand *> &opnds = srcOpnds.GetOperands();
    /* Unsafe.getAndSetInt has 4 parameters */
    DEBUG_ASSERT(opnds.size() == 4, "ensure the operands number");
    auto iter = opnds.begin();
    RegOperand *objOpnd = *(++iter);
    RegOperand *offOpnd = *(++iter);
    RegOperand *newValueOpnd = *(++iter);
    auto &retVal = static_cast<RegOperand &>(GetTargetRetOperand(pty, -1));
    LabelIdx labIdx = CreateLabel();
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(labIdx);
    RegOperand &tempOpnd0 = CreateRegisterOperandOfType(PTY_i64);
    RegOperand &tempOpnd1 = CreateRegisterOperandOfType(PTY_i32);

    MOperator mOp = (pty == PTY_i64) ? MOP_get_and_setL : MOP_get_and_setI;
    std::vector<Operand *> intrnOpnds;
    intrnOpnds.emplace_back(&retVal);
    intrnOpnds.emplace_back(&tempOpnd0);
    intrnOpnds.emplace_back(&tempOpnd1);
    intrnOpnds.emplace_back(objOpnd);
    intrnOpnds.emplace_back(offOpnd);
    intrnOpnds.emplace_back(newValueOpnd);
    intrnOpnds.emplace_back(&targetOpnd);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, intrnOpnds));
}

/*
 * intrinsify Unsafe.compareAndSwapInt and Unsafe.compareAndSwapLong
 * generate an intrinsic instruction instead of a function call
 */
void AArch64CGFunc::IntrinsifyCompareAndSwapInt(ListOperand &srcOpnds, PrimType pty)
{
    MapleList<RegOperand *> &opnds = srcOpnds.GetOperands();
    /* Unsafe.compareAndSwapInt has more than 5 parameters */
    DEBUG_ASSERT(opnds.size() >= 5, "ensure the operands number");
    auto iter = opnds.begin();
    RegOperand *objOpnd = *(++iter);
    RegOperand *offOpnd = *(++iter);
    RegOperand *expectedValueOpnd = *(++iter);
    RegOperand *newValueOpnd = *(++iter);
    auto &retVal = static_cast<RegOperand &>(GetTargetRetOperand(PTY_i64, -1));
    RegOperand &tempOpnd0 = CreateRegisterOperandOfType(PTY_i64);
    RegOperand &tempOpnd1 = CreateRegisterOperandOfType(pty);
    LabelIdx labIdx1 = CreateLabel();
    LabelOperand &label1Opnd = GetOrCreateLabelOperand(labIdx1);
    LabelIdx labIdx2 = CreateLabel();
    LabelOperand &label2Opnd = GetOrCreateLabelOperand(labIdx2);
    MOperator mOp = (pty == PTY_i32) ? MOP_compare_and_swapI : MOP_compare_and_swapL;
    std::vector<Operand *> intrnOpnds;
    intrnOpnds.emplace_back(&retVal);
    intrnOpnds.emplace_back(&tempOpnd0);
    intrnOpnds.emplace_back(&tempOpnd1);
    intrnOpnds.emplace_back(objOpnd);
    intrnOpnds.emplace_back(offOpnd);
    intrnOpnds.emplace_back(expectedValueOpnd);
    intrnOpnds.emplace_back(newValueOpnd);
    intrnOpnds.emplace_back(&label1Opnd);
    intrnOpnds.emplace_back(&label2Opnd);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, intrnOpnds));
}

/*
 * the lowest bit of count field is used to indicate whether or not the string is compressed
 * if the string is not compressed, jump to jumpLabIdx
 */
RegOperand *AArch64CGFunc::CheckStringIsCompressed(BB &bb, RegOperand &str, int32 countOffset, PrimType countPty,
                                                   LabelIdx jumpLabIdx)
{
    MemOperand &memOpnd = CreateMemOpnd(str, countOffset, str.GetSize());
    uint32 bitSize = GetPrimTypeBitSize(countPty);
    MOperator loadOp = PickLdInsn(bitSize, countPty);
    RegOperand &countOpnd = CreateRegisterOperandOfType(countPty);
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(loadOp, countOpnd, memOpnd));
    ImmOperand &immValueOne = CreateImmOperand(countPty, 1);
    RegOperand &countLowestBitOpnd = CreateRegisterOperandOfType(countPty);
    MOperator andOp = bitSize == k64BitSize ? MOP_xandrri13 : MOP_wandrri12;
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(andOp, countLowestBitOpnd, countOpnd, immValueOne));
    RegOperand &wzr = GetZeroOpnd(bitSize);
    MOperator cmpOp = (bitSize == k64BitSize) ? MOP_xcmprr : MOP_wcmprr;
    Operand &rflag = GetOrCreateRflag();
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(cmpOp, rflag, wzr, countLowestBitOpnd));
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(MOP_beq, rflag, GetOrCreateLabelOperand(jumpLabIdx)));
    bb.SetKind(BB::kBBIf);
    return &countOpnd;
}

/*
 * count field stores the length shifted one bit to the left
 * if the length is less than eight, jump to jumpLabIdx
 */
RegOperand *AArch64CGFunc::CheckStringLengthLessThanEight(BB &bb, RegOperand &countOpnd, PrimType countPty,
                                                          LabelIdx jumpLabIdx)
{
    RegOperand &lengthOpnd = CreateRegisterOperandOfType(countPty);
    uint32 bitSize = GetPrimTypeBitSize(countPty);
    MOperator lsrOp = (bitSize == k64BitSize) ? MOP_xlsrrri6 : MOP_wlsrrri5;
    ImmOperand &immValueOne = CreateImmOperand(countPty, 1);
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(lsrOp, lengthOpnd, countOpnd, immValueOne));
    constexpr int kConstIntEight = 8;
    ImmOperand &immValueEight = CreateImmOperand(countPty, kConstIntEight);
    MOperator cmpImmOp = (bitSize == k64BitSize) ? MOP_xcmpri : MOP_wcmpri;
    Operand &rflag = GetOrCreateRflag();
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(cmpImmOp, rflag, lengthOpnd, immValueEight));
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(MOP_blt, rflag, GetOrCreateLabelOperand(jumpLabIdx)));
    bb.SetKind(BB::kBBIf);
    return &lengthOpnd;
}

void AArch64CGFunc::GenerateIntrnInsnForStrIndexOf(BB &bb, RegOperand &srcString, RegOperand &patternString,
                                                   RegOperand &srcCountOpnd, RegOperand &patternLengthOpnd,
                                                   PrimType countPty, LabelIdx jumpLabIdx)
{
    RegOperand &srcLengthOpnd = CreateRegisterOperandOfType(countPty);
    ImmOperand &immValueOne = CreateImmOperand(countPty, 1);
    uint32 bitSize = GetPrimTypeBitSize(countPty);
    MOperator lsrOp = (bitSize == k64BitSize) ? MOP_xlsrrri6 : MOP_wlsrrri5;
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(lsrOp, srcLengthOpnd, srcCountOpnd, immValueOne));
#ifdef USE_32BIT_REF
    const int64 stringBaseObjSize = 16; /* shadow(4)+monitor(4)+count(4)+hash(4) */
#else
    const int64 stringBaseObjSize = 20; /* shadow(8)+monitor(4)+count(4)+hash(4) */
#endif /* USE_32BIT_REF */
    PrimType pty = (srcString.GetSize() == k64BitSize) ? PTY_i64 : PTY_i32;
    ImmOperand &immStringBaseOffset = CreateImmOperand(pty, stringBaseObjSize);
    MOperator addOp = (pty == PTY_i64) ? MOP_xaddrri12 : MOP_waddrri12;
    RegOperand &srcStringBaseOpnd = CreateRegisterOperandOfType(pty);
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(addOp, srcStringBaseOpnd, srcString, immStringBaseOffset));
    RegOperand &patternStringBaseOpnd = CreateRegisterOperandOfType(pty);
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(addOp, patternStringBaseOpnd, patternString, immStringBaseOffset));
    auto &retVal = static_cast<RegOperand &>(GetTargetRetOperand(PTY_i32, -1));
    std::vector<Operand *> intrnOpnds;
    intrnOpnds.emplace_back(&retVal);
    intrnOpnds.emplace_back(&srcStringBaseOpnd);
    intrnOpnds.emplace_back(&srcLengthOpnd);
    intrnOpnds.emplace_back(&patternStringBaseOpnd);
    intrnOpnds.emplace_back(&patternLengthOpnd);
    const uint32 tmpRegOperandNum = 6;
    for (uint32 i = 0; i < tmpRegOperandNum - 1; ++i) {
        RegOperand &tmpOpnd = CreateRegisterOperandOfType(PTY_i64);
        intrnOpnds.emplace_back(&tmpOpnd);
    }
    intrnOpnds.emplace_back(&CreateRegisterOperandOfType(PTY_i32));
    const uint32 labelNum = 7;
    for (uint32 i = 0; i < labelNum; ++i) {
        LabelIdx labIdx = CreateLabel();
        LabelOperand &labelOpnd = GetOrCreateLabelOperand(labIdx);
        intrnOpnds.emplace_back(&labelOpnd);
    }
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(MOP_string_indexof, intrnOpnds));
    bb.AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xuncond, GetOrCreateLabelOperand(jumpLabIdx)));
    bb.SetKind(BB::kBBGoto);
}

/*
 * intrinsify String.indexOf
 * generate an intrinsic instruction instead of a function call if both the source string and the specified substring
 * are compressed and the length of the substring is not less than 8, i.e.
 * bl  String.indexOf, srcString, patternString ===>>
 *
 * ldr srcCountOpnd, [srcString, offset]
 * and srcCountLowestBitOpnd, srcCountOpnd, #1
 * cmp wzr, srcCountLowestBitOpnd
 * beq Label.call
 * ldr patternCountOpnd, [patternString, offset]
 * and patternCountLowestBitOpnd, patternCountOpnd, #1
 * cmp wzr, patternCountLowestBitOpnd
 * beq Label.call
 * lsr patternLengthOpnd, patternCountOpnd, #1
 * cmp patternLengthOpnd, #8
 * blt Label.call
 * lsr srcLengthOpnd, srcCountOpnd, #1
 * add srcStringBaseOpnd, srcString, immStringBaseOffset
 * add patternStringBaseOpnd, patternString, immStringBaseOffset
 * intrinsic_string_indexof retVal, srcStringBaseOpnd, srcLengthOpnd, patternStringBaseOpnd, patternLengthOpnd,
 *                          tmpOpnd1, tmpOpnd2, tmpOpnd3, tmpOpnd4, tmpOpnd5, tmpOpnd6,
 *                          label1, label2, label3, lable3, label4, label5, label6, label7
 * b   Label.joint
 * Label.call:
 * bl  String.indexOf, srcString, patternString
 * Label.joint:
 */
void AArch64CGFunc::IntrinsifyStringIndexOf(ListOperand &srcOpnds, const MIRSymbol &funcSym)
{
    MapleList<RegOperand *> &opnds = srcOpnds.GetOperands();
    /* String.indexOf opnd size must be more than 2 */
    DEBUG_ASSERT(opnds.size() >= 2, "ensure the operands number");
    auto iter = opnds.begin();
    RegOperand *srcString = *iter;
    RegOperand *patternString = *(++iter);
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(namemangler::kJavaLangStringStr);
    MIRType *type =
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(gStrIdx));
    auto stringType = static_cast<MIRStructType *>(type);
    CHECK_FATAL(stringType != nullptr, "Ljava_2Flang_2FString_3B type can not be null");
    FieldID fieldID = GetMirModule().GetMIRBuilder()->GetStructFieldIDFromFieldNameParentFirst(stringType, "count");
    MIRType *fieldType = stringType->GetFieldType(fieldID);
    PrimType countPty = fieldType->GetPrimType();
    int32 offset = GetBecommon().GetFieldOffset(*stringType, fieldID).first;
    LabelIdx callBBLabIdx = CreateLabel();
    RegOperand *srcCountOpnd = CheckStringIsCompressed(*GetCurBB(), *srcString, offset, countPty, callBBLabIdx);

    BB *srcCompressedBB = CreateNewBB();
    GetCurBB()->AppendBB(*srcCompressedBB);
    RegOperand *patternCountOpnd =
        CheckStringIsCompressed(*srcCompressedBB, *patternString, offset, countPty, callBBLabIdx);

    BB *patternCompressedBB = CreateNewBB();
    RegOperand *patternLengthOpnd =
        CheckStringLengthLessThanEight(*patternCompressedBB, *patternCountOpnd, countPty, callBBLabIdx);

    BB *intrinsicBB = CreateNewBB();
    LabelIdx jointLabIdx = CreateLabel();
    GenerateIntrnInsnForStrIndexOf(*intrinsicBB, *srcString, *patternString, *srcCountOpnd, *patternLengthOpnd,
                                   countPty, jointLabIdx);

    BB *callBB = CreateNewBB();
    callBB->AddLabel(callBBLabIdx);
    SetLab2BBMap(callBBLabIdx, *callBB);
    SetCurBB(*callBB);
    Insn &callInsn = AppendCall(funcSym, srcOpnds);
    MIRType *retType = funcSym.GetFunction()->GetReturnType();
    if (retType != nullptr) {
        callInsn.SetRetSize(static_cast<uint32>(retType->GetSize()));
    }
    GetFunction().SetHasCall();

    BB *jointBB = CreateNewBB();
    jointBB->AddLabel(jointLabIdx);
    SetLab2BBMap(jointLabIdx, *jointBB);
    srcCompressedBB->AppendBB(*patternCompressedBB);
    patternCompressedBB->AppendBB(*intrinsicBB);
    intrinsicBB->AppendBB(*callBB);
    callBB->AppendBB(*jointBB);
    SetCurBB(*jointBB);
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
    if (GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        SetLmbcCallReturnType(nullptr);
        if (fn->IsFirstArgReturn()) {
            MIRPtrType *ptrTy = static_cast<MIRPtrType *>(
                GlobalTables::GetTypeTable().GetTypeFromTyIdx(fn->GetFormalDefVec()[0].formalTyIdx));
            MIRType *sTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTy->GetPointedTyIdx());
            SetLmbcCallReturnType(sTy);
        } else {
            MIRType *ty = fn->GetReturnType();
            SetLmbcCallReturnType(ty);
        }
    }

    SelectParmListWrapper(callNode, *srcOpnds, false);

    const std::string &funcName = fsym->GetName();
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 &&
        funcName == "Ljava_2Flang_2FString_3B_7CindexOf_7C_28Ljava_2Flang_2FString_3B_29I") {
        GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(funcName);
        MIRSymbol *st = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(strIdx, true);
        IntrinsifyStringIndexOf(*srcOpnds, *st);
        return;
    }

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
    if (GetMirModule().IsCModule()) { /* do not mark abort BB in C at present */
        if (fsym->GetName() == "__builtin_unreachable") {
            GetCurBB()->ClearInsns();
            GetCurBB()->SetUnreachable(true);
        }
        return;
    }
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

void AArch64CGFunc::HandleCatch()
{
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel1) {
        regno_t regNO = uCatch.regNOCatch;
        RegOperand &vregOpnd = GetOrCreateVirtualRegisterOperand(regNO);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(
            MOP_xmovrr, vregOpnd, GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt)));
    } else {
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(
            PickStInsn(uCatch.opndCatch->GetSize(), PTY_a64),
            GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt), *uCatch.opndCatch));
    }
}

void AArch64CGFunc::SelectMembar(StmtNode &membar)
{
    switch (membar.GetOpCode()) {
        case OP_membaracquire:
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_dmb_ishld, AArch64CG::kMd[MOP_dmb_ishld]));
            break;
        case OP_membarrelease:
        case OP_membarstoreload:
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_dmb_ish, AArch64CG::kMd[MOP_dmb_ish]));
            break;
        case OP_membarstorestore:
            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_dmb_ishst, AArch64CG::kMd[MOP_dmb_ishst]));
            break;
        default:
            DEBUG_ASSERT(false, "NYI");
            break;
    }
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
        case kSregGp: {
            MIRSymbol *sym = GetCG()->GetGP();
            if (sym == nullptr) {
                sym = GetFunction().GetSymTab()->CreateSymbol(kScopeLocal);
                std::string strBuf("__file__local__GP");
                sym->SetNameStrIdx(GetMirModule().GetMIRBuilder()->GetOrCreateStringIndex(strBuf));
                GetCG()->SetGP(sym);
            }
            RegOperand &result = GetOrCreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
            SelectAddrof(result, CreateStImmOperand(*sym, 0, 0));
            return result;
        }
        case kSregThrownval: { /* uses x0 == R0 */
            DEBUG_ASSERT(uCatch.regNOCatch > 0, "regNOCatch should greater than 0.");
            if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
                RegOperand &regOpnd = GetOrCreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8BitSize));
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(PickLdInsn(uCatch.opndCatch->GetSize(), PTY_a64),
                                                                   regOpnd, *uCatch.opndCatch));
                return regOpnd;
            } else {
                return GetOrCreateVirtualRegisterOperand(uCatch.regNOCatch);
            }
        }
        case kSregMethodhdl:
            if (methodHandleVreg == regno_t(-1)) {
                methodHandleVreg = NewVReg(kRegTyInt, k8BitSize);
            }
            return GetOrCreateVirtualRegisterOperand(methodHandleVreg);
        default:
            break;
    }

    // process the 128-bit return value
    if (IsInt128Ty(primType)) {
        Operand &low = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_u64));
        Operand &high = GetOrCreatePhysicalRegisterOperand(R1, k64BitSize, GetRegTyFromPrimTy(PTY_u64));
        return CombineInt128({low, high});
    }

    bool useFpReg = !IsPrimitiveInteger(primType) || IsPrimitiveVectorFloat(primType);
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

RegOperand &AArch64CGFunc::GetOrCreatePhysicalRegisterOperand(std::string &asmAttr)
{
    DEBUG_ASSERT(!asmAttr.empty(), "Get inline asm string failed in GetOrCreatePhysicalRegisterOperand");
    RegType rKind = kRegTyUndef;
    uint32 rSize = 0;
    /* Get Register Type and Size */
    switch (asmAttr[0]) {
        case 'x': {
            rKind = kRegTyInt;
            rSize = k64BitSize;
            break;
        }
        case 'w': {
            rKind = kRegTyInt;
            rSize = k32BitSize;
            break;
        }
        default: {
            LogInfo::MapleLogger() << "Unsupport asm string : " << asmAttr << "\n";
            CHECK_FATAL(false, "Have not support this kind of register ");
        }
    }
    AArch64reg rNO = kRinvalid;
    /* Get Register Number */
    uint32 regNumPos = 1;
    char numberChar = asmAttr[regNumPos++];
    if (numberChar >= '0' && numberChar <= '9') {
        uint32 val = static_cast<uint32>(numberChar - '0');
        if (regNumPos < asmAttr.length()) {
            char numberCharSecond = asmAttr[regNumPos++];
            DEBUG_ASSERT(regNumPos == asmAttr.length(), "Invalid asm attribute");
            if (numberCharSecond >= '0' && numberCharSecond <= '9') {
                val = val * kDecimalMax + static_cast<uint32>((numberCharSecond - '0'));
            }
        }
        rNO = static_cast<AArch64reg>(static_cast<uint32>(R0) + val);
        if (val > (kAsmInputRegPrefixOpnd + 1)) {
            LogInfo::MapleLogger() << "Unsupport asm string : " << asmAttr << "\n";
            CHECK_FATAL(false, "have not support this kind of register ");
        }
    } else if (numberChar == 0) {
        return CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    } else {
        CHECK_FATAL(false, "Unexpect input in GetOrCreatePhysicalRegisterOperand");
    }
    return GetOrCreatePhysicalRegisterOperand(rNO, rSize, rKind);
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

uint32 AArch64CGFunc::GetAggCopySize(uint32 offset1, uint32 offset2, uint32 alignment) const
{
    /* Generating a larger sized mem op than alignment if allowed by aggregate starting address */
    uint32 offsetAlign1 = (offset1 == 0) ? k8ByteSize : offset1;
    uint32 offsetAlign2 = (offset2 == 0) ? k8ByteSize : offset2;
    uint32 alignOffset =
        1U << (std::min(__builtin_ffs(static_cast<int>(offsetAlign1)), __builtin_ffs(static_cast<int>(offsetAlign2))) -
               1);
    if (alignOffset == k8ByteSize || alignOffset == k4ByteSize || alignOffset == k2ByteSize) {
        return alignOffset;
    } else if (alignOffset > k8ByteSize) {
        return k8ByteSize;
    } else {
        return alignment;
    }
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

void AArch64CGFunc::SelectAddrofAfterRa(Operand &result, StImmOperand &stImm, std::vector<Insn *> &rematInsns)
{
    const MIRSymbol *symbol = stImm.GetSymbol();
    DEBUG_ASSERT((symbol->GetStorageClass() != kScAuto) || (symbol->GetStorageClass() != kScFormal), "");
    Operand *srcOpnd = &result;
    rematInsns.emplace_back(&GetInsnBuilder()->BuildInsn(MOP_xadrp, result, stImm));
    if (CGOptions::IsPIC() && symbol->NeedPIC()) {
        /* ldr     x0, [x0, #:got_lo12:Ljava_2Flang_2FSystem_3B_7Cout] */
        OfstOperand &offset = CreateOfstOpnd(*stImm.GetSymbol(), stImm.GetOffset(), stImm.GetRelocs());
        MemOperand &memOpnd = GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, GetPointerSize() * kBitsPerByte,
                                                 static_cast<RegOperand *>(srcOpnd), nullptr, &offset, nullptr);
        rematInsns.emplace_back(
            &GetInsnBuilder()->BuildInsn(memOpnd.GetSize() == k64BitSize ? MOP_xldr : MOP_wldr, result, memOpnd));

        if (stImm.GetOffset() > 0) {
            ImmOperand &immOpnd = CreateImmOperand(stImm.GetOffset(), result.GetSize(), false);
            rematInsns.emplace_back(&GetInsnBuilder()->BuildInsn(MOP_xaddrri12, result, result, immOpnd));
            return;
        }
    } else {
        rematInsns.emplace_back(&GetInsnBuilder()->BuildInsn(MOP_xadrpl12, result, *srcOpnd, stImm));
    }
}

MemOperand &AArch64CGFunc::GetOrCreateMemOpndAfterRa(const MIRSymbol &symbol, int32 offset, uint32 size, bool needLow12,
                                                     RegOperand *regOp, std::vector<Insn *> &rematInsns)
{
    MIRStorageClass storageClass = symbol.GetStorageClass();
    if ((storageClass == kScGlobal) || (storageClass == kScExtern)) {
        StImmOperand &stOpnd = CreateStImmOperand(symbol, offset, 0);
        RegOperand &stAddrOpnd = *regOp;
        SelectAddrofAfterRa(stAddrOpnd, stOpnd, rematInsns);
        /* MemOperand::AddrMode_B_OI */
        return *CreateMemOperand(MemOperand::kAddrModeBOi, size, stAddrOpnd, nullptr,
                                 &GetOrCreateOfstOpnd(0, k32BitSize), &symbol);
    } else if ((storageClass == kScPstatic) || (storageClass == kScFstatic)) {
        if (symbol.GetSKind() == kStConst) {
            DEBUG_ASSERT(offset == 0, "offset should be 0 for constant literals");
            return *CreateMemOperand(MemOperand::kAddrModeLiteral, size, symbol);
        } else {
            if (needLow12) {
                StImmOperand &stOpnd = CreateStImmOperand(symbol, offset, 0);
                RegOperand &stAddrOpnd = *regOp;
                SelectAddrofAfterRa(stAddrOpnd, stOpnd, rematInsns);
                return *CreateMemOperand(MemOperand::kAddrModeBOi, size, stAddrOpnd, nullptr,
                                         &GetOrCreateOfstOpnd(0, k32BitSize), &symbol);
            } else {
                StImmOperand &stOpnd = CreateStImmOperand(symbol, offset, 0);
                RegOperand &stAddrOpnd = *regOp;
                /* adrp    x1, _PTR__cinf_Ljava_2Flang_2FSystem_3B */
                Insn &insn = GetInsnBuilder()->BuildInsn(MOP_xadrp, stAddrOpnd, stOpnd);
                rematInsns.emplace_back(&insn);
                /* ldr     x1, [x1, #:lo12:_PTR__cinf_Ljava_2Flang_2FSystem_3B] */
                return *CreateMemOperand(MemOperand::kAddrModeLo12Li, size, stAddrOpnd, nullptr,
                                         &GetOrCreateOfstOpnd(static_cast<uint64>(offset), k32BitSize), &symbol);
            }
        }
    } else {
        CHECK_FATAL(false, "NYI");
    }
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
            } else if (mirModule.IsJavaModule()) {
                return *(it->second);
            } else {
                Operand *offOpnd = (it->second)->GetOffset();
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
            if ((symbol.GetType()->GetKind() != kTypeClass) && !forLocalRef) {
                memOpndsRequiringOffsetAdjustment[idx] = res;
            }
            return *res;
        }
    } else if ((storageClass == kScGlobal) || (storageClass == kScExtern)) {
        StImmOperand &stOpnd = CreateStImmOperand(symbol, offset, 0);
        if (!regOp) {
            regOp = static_cast<RegOperand *>(&CreateRegisterOperandOfType(PTY_u64));
        }
        RegOperand &stAddrOpnd = *regOp;
        SelectAddrof(stAddrOpnd, stOpnd);
        /* MemOperand::AddrMode_B_OI */
        return *CreateMemOperand(MemOperand::kAddrModeBOi, size, stAddrOpnd, nullptr,
                                 &GetOrCreateOfstOpnd(0, k32BitSize), &symbol);
    } else if ((storageClass == kScPstatic) || (storageClass == kScFstatic)) {
        return CreateMemOpndForStatic(symbol, offset, size, needLow12, regOp);
    } else {
        CHECK_FATAL(false, "NYI");
    }
}

MemOperand &AArch64CGFunc::CreateMemOpndForStatic(const MIRSymbol &symbol, int64 offset, uint32 size, bool needLow12,
                                                  RegOperand *regOp)
{
    if (symbol.GetSKind() == kStConst) {
        DEBUG_ASSERT(offset == 0, "offset should be 0 for constant literals");
        return *CreateMemOperand(MemOperand::kAddrModeLiteral, size, symbol);
    } else {
        /* not guaranteed align for uninitialized symbol */
        if (needLow12 || (!symbol.IsConst() && CGOptions::IsPIC())) {
            StImmOperand &stOpnd = CreateStImmOperand(symbol, offset, 0);
            if (!regOp) {
                regOp = static_cast<RegOperand *>(&CreateRegisterOperandOfType(PTY_u64));
            }
            RegOperand &stAddrOpnd = *regOp;
            SelectAddrof(stAddrOpnd, stOpnd);
            return *CreateMemOperand(MemOperand::kAddrModeBOi, size, stAddrOpnd, nullptr,
                                     &GetOrCreateOfstOpnd(0, k32BitSize), &symbol);
        } else {
            StImmOperand &stOpnd = CreateStImmOperand(symbol, offset, 0);
            if (!regOp) {
                regOp = static_cast<RegOperand *>(&CreateRegisterOperandOfType(PTY_u64));
            }
            RegOperand &stAddrOpnd = *regOp;
            /* adrp    x1, _PTR__cinf_Ljava_2Flang_2FSystem_3B */
            Insn &insn = GetInsnBuilder()->BuildInsn(MOP_xadrp, stAddrOpnd, stOpnd);
            GetCurBB()->AppendInsn(insn);
            if (GetCG()->GetOptimizeLevel() == CGOptions::kLevel0 ||
                ((size == k64BitSize) && (offset % static_cast<int64>(k8BitSizeInt) != 0)) ||
                ((size == k32BitSize) && (offset % static_cast<int64>(k4BitSizeInt) != 0)) ||
                ((size == k16BitSize) && (offset % static_cast<int64>(k2BitSizeInt) != 0))) {
                GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xadrpl12, stAddrOpnd, stAddrOpnd, stOpnd));
                return *CreateMemOperand(MemOperand::kAddrModeBOi, size, stAddrOpnd, nullptr,
                                         &GetOrCreateOfstOpnd(static_cast<uint64>(0), k32BitSize), nullptr);
            }

            /* ldr     x1, [x1, #:lo12:_PTR__cinf_Ljava_2Flang_2FSystem_3B] */
            return *CreateMemOperand(MemOperand::kAddrModeLo12Li, size, stAddrOpnd, nullptr,
                                     &GetOrCreateOfstOpnd(static_cast<uint64>(offset), k32BitSize), &symbol);
        }
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

RegOperand &AArch64CGFunc::GenStructParamIndex(RegOperand &base, const BaseNode &indexExpr, int shift,
                                               PrimType baseType)
{
    RegOperand *index = &LoadIntoRegister(*HandleExpr(indexExpr, *(indexExpr.Opnd(0))), PTY_a64);
    RegOperand *srcOpnd = &CreateRegisterOperandOfType(PTY_a64);
    ImmOperand *imm = &CreateImmOperand(PTY_a64, shift);
    SelectShift(*srcOpnd, *index, *imm, kShiftLeft, PTY_a64);
    RegOperand *result = &CreateRegisterOperandOfType(PTY_a64);
    SelectAdd(*result, base, *srcOpnd, PTY_a64);

    OfstOperand *offopnd = &CreateOfstOpnd(0, k32BitSize);
    MemOperand &mo = GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k64BitSize, result, nullptr, offopnd, nullptr);
    RegOperand &structAddr = CreateVirtualRegisterOperand(NewVReg(kRegTyInt, k8ByteSize));
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(baseType), baseType), structAddr, mo));
    return structAddr;
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
MemOperand *AArch64CGFunc::CheckAndCreateExtendMemOpnd(PrimType ptype, const BaseNode &addrExpr, int64 offset,
                                                       AArch64isa::MemoryOrdering memOrd)
{
    aggParamReg = nullptr;
    if (memOrd != AArch64isa::kMoNone || addrExpr.GetOpCode() != OP_add || offset != 0) {
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
    if (isAggParamInReg) {
        aggParamReg = &GenStructParamIndex(base, *indexExpr, shift, ptype);
        return nullptr;
    }
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
    Insn *lastInsn = GetCurBB() == nullptr ? nullptr : GetCurBB()->GetLastMachineInsn();
    if ((addrExpr.GetOpCode() == OP_CG_array_elem_add) && (offset == 0) && lastInsn &&
        (lastInsn->GetMachineOpcode() == MOP_xadrpl12) &&
        (&lastInsn->GetOperand(kInsnFirstOpnd) == &lastInsn->GetOperand(kInsnSecondOpnd))) {
        Operand &opnd = lastInsn->GetOperand(kInsnThirdOpnd);
        StImmOperand &stOpnd = static_cast<StImmOperand &>(opnd);

        OfstOperand &ofstOpnd = GetOrCreateOfstOpnd(static_cast<uint64>(stOpnd.GetOffset()), k32BitSize);
        MemOperand &tmpMemOpnd =
            GetOrCreateMemOpnd(MemOperand::kAddrModeLo12Li, GetPrimTypeBitSize(ptype),
                               static_cast<RegOperand *>(addrOpnd), nullptr, &ofstOpnd, stOpnd.GetSymbol());
        if (GetCurBB() && GetCurBB()->GetLastMachineInsn()) {
            GetCurBB()->RemoveInsn(*GetCurBB()->GetLastMachineInsn());
        }
        return tmpMemOpnd;
    } else {
        OfstOperand &ofstOpnd = GetOrCreateOfstOpnd(static_cast<uint64>(offset), k64BitSize);
        return GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, GetPrimTypeBitSize(ptype),
                                  static_cast<RegOperand *>(addrOpnd), nullptr, &ofstOpnd, nullptr);
    }
}

/*
 * Create a memory operand with specified data type and memory ordering, making
 * use of aarch64 extend register addressing mode when possible.
 */
MemOperand &AArch64CGFunc::CreateMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset,
                                         AArch64isa::MemoryOrdering memOrd)
{
    MemOperand *memOpnd = CheckAndCreateExtendMemOpnd(ptype, addrExpr, offset, memOrd);
    if (memOpnd != nullptr) {
        return *memOpnd;
    }
    return CreateNonExtendMemOpnd(ptype, parent, addrExpr, offset);
}

MemOperand *AArch64CGFunc::CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset,
                                               AArch64isa::MemoryOrdering memOrd)
{
    MemOperand *memOpnd = CheckAndCreateExtendMemOpnd(ptype, addrExpr, offset, memOrd);
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

/* the first operand in opndvec is return opnd */
void AArch64CGFunc::SelectLibCall(const std::string &funcName, std::vector<Operand *> &opndVec, PrimType primType,
                                  PrimType retPrimType, bool is2ndRet)
{
    std::vector<PrimType> pt;
    pt.push_back(retPrimType);
    for (size_t i = 0; i < opndVec.size(); ++i) {
        pt.push_back(primType);
    }
    SelectLibCallNArg(funcName, opndVec, pt, retPrimType, is2ndRet);
    return;
}

void AArch64CGFunc::SelectLibCallNArg(const std::string &funcName, std::vector<Operand *> &opndVec,
                                      std::vector<PrimType> pt, PrimType retPrimType, bool is2ndRet)
{
    std::string newName = funcName;
    // Check whether we have a maple version of libcall and we want to use it instead.
    if (!CGOptions::IsDuplicateAsmFileEmpty() && asmMap.find(funcName) != asmMap.end()) {
        newName = asmMap.at(funcName);
    }
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    st->SetNameStrIdx(newName);
    st->SetStorageClass(kScExtern);
    st->SetSKind(kStFunc);
    /* setup the type of the callee function */
    std::vector<TyIdx> vec;
    std::vector<TypeAttrs> vecAt;
    for (size_t i = 1; i < opndVec.size(); ++i) {
        (void)vec.emplace_back(GlobalTables::GetTypeTable().GetTypeTable()[static_cast<size_t>(pt[i])]->GetTypeIndex());
        vecAt.emplace_back(TypeAttrs());
    }

    MIRType *retType = GlobalTables::GetTypeTable().GetTypeTable().at(static_cast<size_t>(retPrimType));
    st->SetTyIdx(GetBecommon().BeGetOrCreateFunctionType(retType->GetTypeIndex(), vec, vecAt)->GetTypeIndex());

    if (GetCG()->GenerateVerboseCG()) {
        const std::string &comment = "lib call : " + newName;
        GetCurBB()->AppendInsn(CreateCommentInsn(comment));
    }
    // only create c lib call here
    AArch64CallConvImpl parmLocator(GetBecommon());
    CCLocInfo ploc;
    /* setup actual parameters */
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    for (size_t i = 1; i < opndVec.size(); ++i) {
        DEBUG_ASSERT(pt[i] != PTY_void, "primType check");
        MIRType *ty;
        ty = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<size_t>(pt[i])];
        Operand *stOpnd = opndVec[i];
        if (stOpnd->GetKind() != Operand::kOpdRegister) {
            stOpnd = &SelectCopy(*stOpnd, pt[i], pt[i]);
        }
        RegOperand *expRegOpnd = static_cast<RegOperand *>(stOpnd);
        parmLocator.LocateNextParm(*ty, ploc);
        if (ploc.reg0 != 0) { /* load to the register */
            RegOperand &parmRegOpnd = GetOrCreatePhysicalRegisterOperand(
                static_cast<AArch64reg>(ploc.reg0), expRegOpnd->GetSize(), GetRegTyFromPrimTy(pt[i]));
            SelectCopy(parmRegOpnd, pt[i], *expRegOpnd, pt[i]);
            srcOpnds->PushOpnd(parmRegOpnd);
        }
        DEBUG_ASSERT(ploc.reg1 == 0, "SelectCall NYI");
    }

    MIRSymbol *sym = GetFunction().GetLocalOrGlobalSymbol(st->GetStIdx(), false);
    Insn &callInsn = AppendCall(*sym, *srcOpnds);
    MIRType *callRetType = GlobalTables::GetTypeTable().GetTypeTable().at(static_cast<int32>(retPrimType));
    if (callRetType != nullptr) {
        callInsn.SetRetSize(static_cast<uint32>(callRetType->GetSize()));
        callInsn.SetIsCallReturnUnsigned(IsUnsignedInteger(callRetType->GetPrimType()));
    }
    GetFunction().SetHasCall();
    /* get return value */
    Operand *opnd0 = opndVec[0];
    CCLocInfo retMech;
    parmLocator.LocateRetVal(*(GlobalTables::GetTypeTable().GetTypeTable().at(retPrimType)), retMech);
    if (retMech.GetRegCount() <= 0) {
        CHECK_FATAL(false, "should return from register");
    }
    if (!opnd0->IsRegister()) {
        CHECK_FATAL(false, "nyi");
    }
    RegOperand *regOpnd = static_cast<RegOperand *>(opnd0);
    AArch64reg regNum = static_cast<AArch64reg>(is2ndRet ? retMech.GetReg1() : retMech.GetReg0());
    if (regOpnd->GetRegisterNumber() != regNum) {
        RegOperand &retOpnd =
            GetOrCreatePhysicalRegisterOperand(regNum, regOpnd->GetSize(), GetRegTyFromPrimTy(retPrimType));
        SelectCopy(*opnd0, retPrimType, retOpnd, retPrimType);
    }
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
        int32 baseOffset;
        if (GetCG()->IsLmbc()) {
            baseOffset = static_cast<int32>(symAlloc->GetOffset()) +
                         static_cast<int32>(memLayout->GetSizeOfRefLocals() +
                                            memLayout->SizeOfArgsToStackPass()); /* SP relative */
        } else {
            baseOffset = static_cast<int32>(memLayout->GetSizeOfLocals() + memLayout->GetSizeOfRefLocals()) +
                         static_cast<int32>(symAlloc->GetOffset());
        }
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsRefLocals) {
        int32 baseOffset = static_cast<int32>(symAlloc->GetOffset()) + static_cast<int32>(memLayout->GetSizeOfLocals());
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsLocals) {
        if (GetCG()->IsLmbc()) {
            CHECK_FATAL(false, "invalid lmbc's locals");
        }
        int32 baseOffset = symAlloc->GetOffset();
        return baseOffset + sizeofFplr;
    } else if (sgKind == kMsSpillReg) {
        int32 baseOffset;
        if (GetCG()->IsLmbc()) {
            baseOffset = static_cast<int32>(symAlloc->GetOffset()) +
                         static_cast<int32>(memLayout->SizeOfArgsRegisterPassed() + memLayout->GetSizeOfRefLocals() +
                                            memLayout->SizeOfArgsToStackPass());
        } else {
            baseOffset = static_cast<int32>(symAlloc->GetOffset()) +
                         static_cast<int32>(memLayout->SizeOfArgsRegisterPassed() + memLayout->GetSizeOfLocals() +
                                            memLayout->GetSizeOfRefLocals());
        }
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

void AArch64CGFunc::DBGFixCallFrameLocationOffsets()
{
    unsigned idx = 0;
    for (DBGExprLoc *el : GetDbgCallFrameLocations(true)) {
        if (el && el->GetSimpLoc() && el->GetSimpLoc()->GetDwOp() == DW_OP_fbreg) {
            SymbolAlloc *symloc = static_cast<SymbolAlloc *>(el->GetSymLoc());
            int32 offset = GetBaseOffset(*symloc) - ((idx < AArch64Abi::kNumIntParmRegs) ? GetDbgCallFrameOffset() : 0);
            el->SetFboffset(offset);
        }
        idx++;
    }
    for (DBGExprLoc *el : GetDbgCallFrameLocations(false)) {
        if (el->GetSimpLoc()->GetDwOp() == DW_OP_fbreg) {
            SymbolAlloc *symloc = static_cast<SymbolAlloc *>(el->GetSymLoc());
            int32 offset = GetBaseOffset(*symloc) - GetDbgCallFrameOffset();
            el->SetFboffset(offset);
        }
    }
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

MIRPreg *AArch64CGFunc::GetPseudoRegFromVirtualRegNO(const regno_t vRegNO, bool afterSSA) const
{
    PregIdx pri = afterSSA ? VRegNOToPRegIdx(vRegNO) : GetPseudoRegIdxFromVirtualRegNO(vRegNO);
    if (pri == -1)
        return nullptr;
    return GetFunction().GetPregTab()->PregFromPregIdx(pri);
}

/* Get the number of return register of current function. */
AArch64reg AArch64CGFunc::GetReturnRegisterNumber()
{
    CCImpl &retLocator = *GetOrCreateLocator(GetCurCallConvKind());
    CCLocInfo retMech;
    retLocator.LocateRetVal(*(GetFunction().GetReturnType()), retMech);
    if (retMech.GetRegCount() > 0) {
        return static_cast<AArch64reg>(retMech.GetReg0());
    }
    return kRinvalid;
}

bool AArch64CGFunc::CanLazyBinding(const Insn &ldrInsn) const
{
    Operand &memOpnd = ldrInsn.GetOperand(1);
    auto &aarchMemOpnd = static_cast<MemOperand &>(memOpnd);
    if (aarchMemOpnd.GetAddrMode() != MemOperand::kAddrModeLo12Li) {
        return false;
    }

    const MIRSymbol *sym = aarchMemOpnd.GetSymbol();
    CHECK_FATAL(sym != nullptr, "sym can't be nullptr");
    if (sym->IsMuidFuncDefTab() || sym->IsMuidFuncUndefTab() || sym->IsMuidDataDefTab() || sym->IsMuidDataUndefTab() ||
        (sym->IsReflectionClassInfo() && !sym->IsReflectionArrayClassInfo())) {
        return true;
    }

    return false;
}

/*
 *  add reg, reg, __PTR_C_STR_...
 *  ldr reg1, [reg]
 *  =>
 *  ldr reg1, [reg, #:lo12:__Ptr_C_STR_...]
 */
void AArch64CGFunc::ConvertAdrpl12LdrToLdr()
{
    FOR_ALL_BB(bb, this)
    {
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn)
        {
            nextInsn = insn->GetNextMachineInsn();
            if (nextInsn == nullptr) {
                break;
            }
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            /* check first insn */
            MOperator thisMop = insn->GetMachineOpcode();
            if (thisMop != MOP_xadrpl12) {
                continue;
            }
            /* check second insn */
            MOperator nextMop = nextInsn->GetMachineOpcode();
            if (!(((nextMop >= MOP_wldrsb) && (nextMop <= MOP_dldp)) ||
                  ((nextMop >= MOP_wstrb) && (nextMop <= MOP_dstp)))) {
                continue;
            }

            /* Check if base register of nextInsn and the dest operand of insn are identical. */
            MemOperand *memOpnd = static_cast<MemOperand *>(nextInsn->GetMemOpnd());
            CHECK_FATAL(memOpnd != nullptr, "memOpnd can't be nullptr");

            /* Only for AddrMode_B_OI addressing mode. */
            if (memOpnd->GetAddrMode() != MemOperand::kAddrModeBOi) {
                continue;
            }

            /* Only for intact memory addressing. */
            if (!memOpnd->IsIntactIndexed()) {
                continue;
            }

            auto &regOpnd = static_cast<RegOperand &>(insn->GetOperand(0));

            /* Check if dest operand of insn is idential with base register of nextInsn. */
            RegOperand *baseReg = memOpnd->GetBaseRegister();
            CHECK_FATAL(baseReg != nullptr, "baseReg can't be nullptr");
            if (baseReg->GetRegisterNumber() != regOpnd.GetRegisterNumber()) {
                continue;
            }

            StImmOperand &stImmOpnd = static_cast<StImmOperand &>(insn->GetOperand(kInsnThirdOpnd));
            OfstOperand &ofstOpnd = GetOrCreateOfstOpnd(
                static_cast<uint64>(stImmOpnd.GetOffset() + memOpnd->GetOffsetImmediate()->GetOffsetValue()),
                k32BitSize);
            RegOperand &newBaseOpnd = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
            MemOperand &newMemOpnd = GetOrCreateMemOpnd(MemOperand::kAddrModeLo12Li, memOpnd->GetSize(), &newBaseOpnd,
                                                        nullptr, &ofstOpnd, stImmOpnd.GetSymbol());
            nextInsn->SetOperand(1, newMemOpnd);
            bb->RemoveInsn(*insn);
        }
    }
}

/*
 * adrp reg1, __muid_func_undef_tab..
 * ldr reg2, [reg1, #:lo12:__muid_func_undef_tab..]
 * =>
 * intrinsic_adrp_ldr reg2, __muid_func_undef_tab...
 */
void AArch64CGFunc::ConvertAdrpLdrToIntrisic()
{
    FOR_ALL_BB(bb, this)
    {
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn)
        {
            nextInsn = insn->GetNextMachineInsn();
            if (nextInsn == nullptr) {
                break;
            }
            if (!insn->IsMachineInstruction()) {
                continue;
            }

            MOperator firstMop = insn->GetMachineOpcode();
            MOperator secondMop = nextInsn->GetMachineOpcode();
            if (!((firstMop == MOP_xadrp) && ((secondMop == MOP_wldr) || (secondMop == MOP_xldr)))) {
                continue;
            }

            if (CanLazyBinding(*nextInsn)) {
                bb->ReplaceInsn(
                    *insn, GetInsnBuilder()->BuildInsn(MOP_adrp_ldr, nextInsn->GetOperand(0), insn->GetOperand(1)));
                bb->RemoveInsn(*nextInsn);
            }
        }
    }
}

void AArch64CGFunc::ProcessLazyBinding()
{
    ConvertAdrpl12LdrToLdr();
    ConvertAdrpLdrToIntrisic();
}

/*
 * Generate global long call
 *  adrp  VRx, symbol
 *  ldr VRx, [VRx, #:lo12:symbol]
 *  blr VRx
 *
 * Input:
 *  insn       : insert new instruction after the 'insn'
 *  func       : the symbol of the function need to be called
 *  srcOpnds   : list operand of the function need to be called
 *  isCleanCall: when generate clean call insn, set isCleanCall as true
 * Return: the 'blr' instruction
 */
Insn &AArch64CGFunc::GenerateGlobalLongCallAfterInsn(const MIRSymbol &func, ListOperand &srcOpnds)
{
    MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(func.GetStIdx());
    symbol->SetStorageClass(kScGlobal);
    RegOperand &tmpReg = CreateRegisterOperandOfType(PTY_u64);
    StImmOperand &stOpnd = CreateStImmOperand(*symbol, 0, 0);
    OfstOperand &offsetOpnd = CreateOfstOpnd(*symbol, 0);
    Insn &adrpInsn = GetInsnBuilder()->BuildInsn(MOP_xadrp, tmpReg, stOpnd);
    GetCurBB()->AppendInsn(adrpInsn);
    MemOperand &memOrd = GetOrCreateMemOpnd(MemOperand::kAddrModeLo12Li, GetPointerSize() * kBitsPerByte,
                                            static_cast<RegOperand *>(&tmpReg), nullptr, &offsetOpnd, symbol);
    Insn &ldrInsn = GetInsnBuilder()->BuildInsn(memOrd.GetSize() == k64BitSize ? MOP_xldr : MOP_wldr, tmpReg, memOrd);
    GetCurBB()->AppendInsn(ldrInsn);

    Insn &callInsn = GetInsnBuilder()->BuildInsn(MOP_xblr, tmpReg, srcOpnds);
    GetCurBB()->AppendInsn(callInsn);
    GetCurBB()->SetHasCall();
    return callInsn;
}

/*
 * Generate local long call
 *  adrp  VRx, symbol
 *  add VRx, VRx, #:lo12:symbol
 *  blr VRx
 *
 * Input:
 *  insn       : insert new instruction after the 'insn'
 *  func       : the symbol of the function need to be called
 *  srcOpnds   : list operand of the function need to be called
 *  isCleanCall: when generate clean call insn, set isCleanCall as true
 * Return: the 'blr' instruction
 */
Insn &AArch64CGFunc::GenerateLocalLongCallAfterInsn(const MIRSymbol &func, ListOperand &srcOpnds)
{
    RegOperand &tmpReg = CreateRegisterOperandOfType(PTY_u64);
    StImmOperand &stOpnd = CreateStImmOperand(func, 0, 0);
    Insn &adrpInsn = GetInsnBuilder()->BuildInsn(MOP_xadrp, tmpReg, stOpnd);
    GetCurBB()->AppendInsn(adrpInsn);
    Insn &addInsn = GetInsnBuilder()->BuildInsn(MOP_xadrpl12, tmpReg, tmpReg, stOpnd);
    GetCurBB()->AppendInsn(addInsn);
    Insn *callInsn = &GetInsnBuilder()->BuildInsn(MOP_xblr, tmpReg, srcOpnds);
    GetCurBB()->AppendInsn(*callInsn);
    GetCurBB()->SetHasCall();
    return *callInsn;
}

Insn &AArch64CGFunc::AppendCall(const MIRSymbol &sym, ListOperand &srcOpnds)
{
    Insn *callInsn = nullptr;
    if (CGOptions::IsLongCalls()) {
        MIRFunction *mirFunc = sym.GetFunction();
        if (IsDuplicateAsmList(sym) || (mirFunc && mirFunc->GetAttr(FUNCATTR_local))) {
            callInsn = &GenerateLocalLongCallAfterInsn(sym, srcOpnds);
        } else {
            callInsn = &GenerateGlobalLongCallAfterInsn(sym, srcOpnds);
        }
    } else {
        Operand &targetOpnd = GetOrCreateFuncNameOpnd(sym);
        callInsn = &GetInsnBuilder()->BuildInsn(MOP_xbl, targetOpnd, srcOpnds);
        GetCurBB()->AppendInsn(*callInsn);
        GetCurBB()->SetHasCall();
    }
    return *callInsn;
}

bool AArch64CGFunc::IsDuplicateAsmList(const MIRSymbol &sym) const
{
    if (CGOptions::IsDuplicateAsmFileEmpty()) {
        return false;
    }

    const std::string &name = sym.GetName();
    if ((name == "strlen") || (name == "strncmp") || (name == "memcpy") || (name == "memmove") || (name == "strcmp") ||
        (name == "memcmp") || (name == "memcmpMpl")) {
        return true;
    }
    return false;
}

void AArch64CGFunc::SelectMPLProfCounterInc(const IntrinsiccallNode &intrnNode)
{
    if (Options::profileGen) {
        DEBUG_ASSERT(intrnNode.NumOpnds() == 1, "must be 1 operand");
        BaseNode *arg1 = intrnNode.Opnd(0);
        DEBUG_ASSERT(arg1 != nullptr, "nullptr check");
        regno_t vRegNO1 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
        RegOperand &vReg1 = CreateVirtualRegisterOperand(vRegNO1);
        vReg1.SetRegNotBBLocal();
        static const MIRSymbol *bbProfileTab = nullptr;

        // Ref: MeProfGen::InstrumentFunc on ctrTbl namiLogicalShiftLeftOperandng
        std::string ctrTblName = namemangler::kprefixProfCtrTbl + GetMirModule().GetFileName() + "_" + GetName();
        std::replace(ctrTblName.begin(), ctrTblName.end(), '.', '_');
        std::replace(ctrTblName.begin(), ctrTblName.end(), '-', '_');
        std::replace(ctrTblName.begin(), ctrTblName.end(), '/', '_');

        if (!bbProfileTab || bbProfileTab->GetName() != ctrTblName) {
            bbProfileTab = GetMirModule().GetMIRBuilder()->GetGlobalDecl(ctrTblName);
            CHECK_FATAL(bbProfileTab != nullptr, "expect counter table");
        }

        ConstvalNode *constvalNode = static_cast<ConstvalNode *>(arg1);
        MIRConst *mirConst = constvalNode->GetConstVal();
        DEBUG_ASSERT(mirConst != nullptr, "nullptr check");
        CHECK_FATAL(mirConst->GetKind() == kConstInt, "expect MIRIntConst type");
        MIRIntConst *mirIntConst = safe_cast<MIRIntConst>(mirConst);
        int64 offset = static_cast<int64>(GetPrimTypeSize(PTY_u64)) * mirIntConst->GetExtValue();

        if (!CGOptions::IsQuiet()) {
            maple::LogInfo::MapleLogger(kLlInfo) << "At counter table offset: " << offset << std::endl;
        }
        MemOperand *memOpnd = &GetOrCreateMemOpnd(*bbProfileTab, offset, k64BitSize);
        if (IsImmediateOffsetOutOfRange(*memOpnd, k64BitSize)) {
            memOpnd = &SplitOffsetWithAddInstruction(*memOpnd, k64BitSize);
        }
        Operand *reg = &SelectCopy(*memOpnd, PTY_u64, PTY_u64);
        ImmOperand &one = CreateImmOperand(1, k64BitSize, false);
        SelectAdd(*reg, *reg, one, PTY_u64);
        SelectCopy(*memOpnd, PTY_u64, *reg, PTY_u64);
        return;
    }

    DEBUG_ASSERT(intrnNode.NumOpnds() == 1, "must be 1 operand");
    BaseNode *arg1 = intrnNode.Opnd(0);
    DEBUG_ASSERT(arg1 != nullptr, "nullptr check");
    regno_t vRegNO1 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
    RegOperand &vReg1 = CreateVirtualRegisterOperand(vRegNO1);
    vReg1.SetRegNotBBLocal();
    static const MIRSymbol *bbProfileTab = nullptr;
    if (!bbProfileTab) {
        std::string bbProfileName = namemangler::kBBProfileTabPrefixStr + GetMirModule().GetFileNameAsPostfix();
        bbProfileTab = GetMirModule().GetMIRBuilder()->GetGlobalDecl(bbProfileName);
        CHECK_FATAL(bbProfileTab != nullptr, "expect bb profile tab");
    }
    ConstvalNode *constvalNode = static_cast<ConstvalNode *>(arg1);
    MIRConst *mirConst = constvalNode->GetConstVal();
    DEBUG_ASSERT(mirConst != nullptr, "nullptr check");
    CHECK_FATAL(mirConst->GetKind() == kConstInt, "expect MIRIntConst type");
    MIRIntConst *mirIntConst = safe_cast<MIRIntConst>(mirConst);
    int64 idx = static_cast<int64>(GetPrimTypeSize(PTY_u32)) * mirIntConst->GetExtValue();
    if (!CGOptions::IsQuiet()) {
        maple::LogInfo::MapleLogger(kLlErr) << "Id index " << idx << std::endl;
    }
    StImmOperand &stOpnd = CreateStImmOperand(*bbProfileTab, idx, 0);
    Insn &newInsn = GetInsnBuilder()->BuildInsn(MOP_counter, vReg1, stOpnd);
    newInsn.SetDoNotRemove(true);
    GetCurBB()->AppendInsn(newInsn);
}

void AArch64CGFunc::SelectMPLClinitCheck(const IntrinsiccallNode &intrnNode)
{
    DEBUG_ASSERT(intrnNode.NumOpnds() == 1, "must be 1 operand");
    BaseNode *arg = intrnNode.Opnd(0);
    Operand *stOpnd = nullptr;
    bool bClinitSeperate = false;
    DEBUG_ASSERT(CGOptions::IsPIC(), "must be doPIC");
    if (arg->GetOpCode() == OP_addrof) {
        AddrofNode *addrof = static_cast<AddrofNode *>(arg);
        MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(addrof->GetStIdx());
        DEBUG_ASSERT(symbol->GetName().find(CLASSINFO_PREFIX_STR) == 0, "must be a symbol with __classinfo__");

        if (!symbol->IsMuidDataUndefTab()) {
            std::string ptrName = namemangler::kPtrPrefixStr + symbol->GetName();
            MIRType *ptrType = GlobalTables::GetTypeTable().GetPtr();
            symbol = GetMirModule().GetMIRBuilder()->GetOrCreateGlobalDecl(ptrName, *ptrType);
            bClinitSeperate = true;
            symbol->SetStorageClass(kScFstatic);
        }
        stOpnd = &CreateStImmOperand(*symbol, 0, 0);
    } else {
        arg = arg->Opnd(0);
        BaseNode *arg0 = arg->Opnd(0);
        BaseNode *arg1 = arg->Opnd(1);
        DEBUG_ASSERT(arg0 != nullptr, "nullptr check");
        DEBUG_ASSERT(arg1 != nullptr, "nullptr check");
        DEBUG_ASSERT(arg0->GetOpCode() == OP_addrof, "expect the operand to be addrof");
        AddrofNode *addrof = static_cast<AddrofNode *>(arg0);
        MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(addrof->GetStIdx());
        DEBUG_ASSERT(addrof->GetFieldID() == 0, "For debug SelectMPLClinitCheck.");
        ConstvalNode *constvalNode = static_cast<ConstvalNode *>(arg1);
        MIRConst *mirConst = constvalNode->GetConstVal();
        DEBUG_ASSERT(mirConst != nullptr, "nullptr check");
        CHECK_FATAL(mirConst->GetKind() == kConstInt, "expect MIRIntConst type");
        MIRIntConst *mirIntConst = safe_cast<MIRIntConst>(mirConst);
        stOpnd = &CreateStImmOperand(*symbol, mirIntConst->GetExtValue(), 0);
    }

    regno_t vRegNO2 = NewVReg(GetRegTyFromPrimTy(PTY_a64), GetPrimTypeSize(PTY_a64));
    RegOperand &vReg2 = CreateVirtualRegisterOperand(vRegNO2);
    vReg2.SetRegNotBBLocal();
    if (bClinitSeperate) {
        /* Seperate MOP_clinit to MOP_adrp_ldr + MOP_clinit_tail. */
        Insn &newInsn = GetInsnBuilder()->BuildInsn(MOP_adrp_ldr, vReg2, *stOpnd);
        GetCurBB()->AppendInsn(newInsn);
        newInsn.SetDoNotRemove(true);
        Insn &insn = GetInsnBuilder()->BuildInsn(MOP_clinit_tail, vReg2);
        insn.SetDoNotRemove(true);
        GetCurBB()->AppendInsn(insn);
    } else {
        Insn &newInsn = GetInsnBuilder()->BuildInsn(MOP_clinit, vReg2, *stOpnd);
        GetCurBB()->AppendInsn(newInsn);
    }
}
void AArch64CGFunc::GenCVaStartIntrin(RegOperand &opnd, uint32 stkSize)
{
    /* FPLR only pushed in regalloc() after intrin function */
    Operand &stkOpnd = GetOrCreatePhysicalRegisterOperand(RFP, k64BitSize, kRegTyInt);

    /* __stack */
    ImmOperand *offsOpnd;
    int64 coldToStk = static_cast<int64>(static_cast<AArch64MemLayout *>(GetMemlayout())->GetSizeOfColdToStk());
    if (GetMirModule().GetFlavor() != MIRFlavor::kFlavorLmbc) {
        // unvary reset StackFrameSize
        // unvary to stk bot = coldToStk
        offsOpnd = &CreateImmOperand(coldToStk, k64BitSize, true, kUnAdjustVary); /* isvary reset StackFrameSize */
    } else {
        offsOpnd = &CreateImmOperand(0, k64BitSize, true);
    }
    ImmOperand *offsOpnd2 = &CreateImmOperand(stkSize, k64BitSize, false);
    RegOperand &vReg = CreateVirtualRegisterOperand(NewVReg(kRegTyInt, GetPrimTypeSize(GetLoweredPtrType())));
    if (stkSize) {
        SelectAdd(vReg, *offsOpnd, *offsOpnd2, GetLoweredPtrType());
        SelectAdd(vReg, stkOpnd, vReg, GetLoweredPtrType());
    } else {
        SelectAdd(vReg, stkOpnd, *offsOpnd, GetLoweredPtrType()); /* stack pointer */
    }
    OfstOperand *offOpnd = &GetOrCreateOfstOpnd(0, k64BitSize); /* va_list ptr */
    /* mem operand in va_list struct (lhs) */
    MemOperand *strOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k64BitSize, &opnd, nullptr, offOpnd,
                                              static_cast<MIRSymbol *>(nullptr));
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(vReg.GetSize() == k64BitSize ? MOP_xstr : MOP_wstr, vReg, *strOpnd));

    /* __gr_top   ; it's the same as __stack before the 1st va_arg */
    if (CGOptions::IsArm64ilp32()) {
        offOpnd = &GetOrCreateOfstOpnd(GetPointerSize(), k64BitSize);
    } else {
        offOpnd = &GetOrCreateOfstOpnd(k8BitSize, k64BitSize);
    }
    strOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k64BitSize, &opnd, nullptr, offOpnd,
                                  static_cast<MIRSymbol *>(nullptr));
    SelectAdd(vReg, stkOpnd, *offsOpnd, GetLoweredPtrType());
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(vReg.GetSize() == k64BitSize ? MOP_xstr : MOP_wstr, vReg, *strOpnd));

    /* __vr_top */
    int32 grAreaSize = static_cast<int32>(static_cast<AArch64MemLayout *>(GetMemlayout())->GetSizeOfGRSaveArea());
    if (CGOptions::IsArm64ilp32()) {
        offsOpnd2 = &CreateImmOperand(static_cast<int64>(RoundUp(static_cast<uint64>(grAreaSize), k8ByteSize << 1)),
                                      k64BitSize, false);
    } else {
        offsOpnd2 = &CreateImmOperand(
            static_cast<int64>(RoundUp(static_cast<uint64>(grAreaSize), GetPointerSize() << 1)), k64BitSize, false);
    }
    SelectSub(vReg, *offsOpnd, *offsOpnd2, GetLoweredPtrType()); /* if 1st opnd is register => sub */
    SelectAdd(vReg, stkOpnd, vReg, GetLoweredPtrType());
    offOpnd = &GetOrCreateOfstOpnd(GetPointerSize() << 1, k64BitSize);
    strOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k64BitSize, &opnd, nullptr, offOpnd,
                                  static_cast<MIRSymbol *>(nullptr));
    GetCurBB()->AppendInsn(
        GetInsnBuilder()->BuildInsn(vReg.GetSize() == k64BitSize ? MOP_xstr : MOP_wstr, vReg, *strOpnd));

    /* __gr_offs */
    int32 offs = 0 - grAreaSize;
    offsOpnd = &CreateImmOperand(offs, k32BitSize, false);
    RegOperand *tmpReg = &CreateRegisterOperandOfType(PTY_i32); /* offs value to be assigned (rhs) */
    SelectCopyImm(*tmpReg, *offsOpnd, PTY_i32);
    // write __gr_offs : offset from gr_top to next GP register arg
    // accroding to typedef struct va_list
    // the field offset of __gr_offs is 3 pointer from beginning
    offOpnd = &GetOrCreateOfstOpnd(GetPointerSize() * 3, k32BitSize);
    strOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k32BitSize, &opnd, nullptr, offOpnd,
                                  static_cast<MIRSymbol *>(nullptr));
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wstr, *tmpReg, *strOpnd));

    /* __vr_offs */
    offs =
        static_cast<int32>(UINT32_MAX - (static_cast<AArch64MemLayout *>(GetMemlayout())->GetSizeOfVRSaveArea() - 1UL));
    offsOpnd = &CreateImmOperand(offs, k32BitSize, false);
    tmpReg = &CreateRegisterOperandOfType(PTY_i32);
    SelectCopyImm(*tmpReg, *offsOpnd, PTY_i32);
    // write __vr_offs : offset from vr_top to next FP/SIMD register arg
    // accroding to typedef struct va_list
    // the field offset of __vr_offs is 3 pointer + sizeof(int) from beginning
    offOpnd = &GetOrCreateOfstOpnd((GetPointerSize() * 3 + sizeof(int32)), k32BitSize);
    strOpnd = &GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k32BitSize, &opnd, nullptr, offOpnd,
                                  static_cast<MIRSymbol *>(nullptr));
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wstr, *tmpReg, *strOpnd));
}

void AArch64CGFunc::SelectCVaStart(const IntrinsiccallNode &intrnNode)
{
    DEBUG_ASSERT(intrnNode.NumOpnds() == 2, "must be 2 operands");  // must be 2 operands
    /* 2 operands, but only 1 needed. Don't need to emit code for second operand
     *
     * va_list is a passed struct with an address, load its address
     */
    isIntrnCallForC = true;
    BaseNode *argExpr = intrnNode.Opnd(0);
    Operand *opnd = HandleExpr(intrnNode, *argExpr);
    RegOperand &opnd0 = LoadIntoRegister(*opnd, GetLoweredPtrType()); /* first argument of intrinsic */

    /* Find beginning of unnamed arg on stack.
     * Ex. void foo(int i1, int i2, ... int i8, struct S r, struct S s, ...)
     *     where struct S has size 32, address of r and s are on stack but they are named.
     */
    AArch64CallConvImpl parmLocator(GetBecommon());
    CCLocInfo pLoc;
    uint32 stkSize = 0;
    uint32 inReg = 0;
    for (uint32 i = 0; i < GetFunction().GetFormalCount(); i++) {
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetFunction().GetNthParamTyIdx(i));
        parmLocator.LocateNextParm(*ty, pLoc);
        if (pLoc.reg0 == kRinvalid) { /* on stack */
            stkSize = static_cast<uint32_t>(pLoc.memOffset + pLoc.memSize);
        } else {
            inReg++;
        }
    }
    if (GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
        stkSize += (inReg * k8ByteSize);
    }
    if (CGOptions::IsArm64ilp32()) {
        stkSize = static_cast<uint32>(RoundUp(stkSize, k8ByteSize));
    } else {
        stkSize = static_cast<uint32>(RoundUp(stkSize, GetPointerSize()));
    }

    GenCVaStartIntrin(opnd0, stkSize);

    return;
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
    RegOperand &resReg = CreateRegisterOperandOfType(type);
    RegOperand &resReg2 = CreateRegisterOperandOfType(PTY_u8);
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
    // store back
    auto *retVals = &intrnNode.GetReturnVec();
    auto &pair = retVals->at(0);
    stIdx2OverflowResult[pair.first] = std::pair<RegOperand *, RegOperand *>(&resReg, &resReg2);
    return;
}

/*
 * intrinsiccall C___Atomic_store_N(ptr, val, memorder))
 * ====> *ptr = val
 * let ptr -> x0
 * let val -> x1
 * implement to asm: str/stlr x1, [x0]
 * a store-release would replace str if memorder is not 0
 */
void AArch64CGFunc::SelectCAtomicStoreN(const IntrinsiccallNode &intrinsiccallNode)
{
    auto primType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(intrinsiccallNode.GetTyIdx())->GetPrimType();
    auto *addr = HandleExpr(intrinsiccallNode, *intrinsiccallNode.Opnd(0));
    auto *value = HandleExpr(intrinsiccallNode, *intrinsiccallNode.Opnd(1));
    auto *memOrderOpnd = intrinsiccallNode.Opnd(kInsnThirdOpnd);
    std::memory_order memOrder = std::memory_order_seq_cst;
    if (memOrderOpnd->IsConstval()) {
        auto *memOrderConst = static_cast<MIRIntConst *>(static_cast<ConstvalNode *>(memOrderOpnd)->GetConstVal());
        memOrder = static_cast<std::memory_order>(memOrderConst->GetExtValue());
    }
    SelectAtomicStore(*value, *addr, primType, PickMemOrder(memOrder, false));
}

void AArch64CGFunc::SelectAtomicStore(Operand &srcOpnd, Operand &addrOpnd, PrimType primType,
                                      AArch64isa::MemoryOrdering memOrder)
{
    auto &memOpnd = CreateMemOpnd(LoadIntoRegister(addrOpnd, PTY_a64), 0, k64BitSize);
    auto mOp = PickStInsn(GetPrimTypeBitSize(primType), primType, memOrder);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, LoadIntoRegister(srcOpnd, primType), memOpnd));
}

void AArch64CGFunc::SelectAddrofThreadLocal(Operand &result, StImmOperand &stImm)
{
    if (CGOptions::IsPIC()) {
        SelectCTlsGlobalDesc(result, stImm);
    } else {
        SelectCTlsLocalDesc(result, stImm);
    }
    if (stImm.GetOffset() > 0) {
        auto &immOpnd = CreateImmOperand(stImm.GetOffset(), result.GetSize(), false);
        SelectAdd(result, result, immOpnd, PTY_u64);
    }
}

void AArch64CGFunc::SelectCTlsLocalDesc(Operand &result, StImmOperand &stImm)
{
    auto tpidr = &CreateCommentOperand("tpidr_el0");
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_mrs, result, *tpidr));
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_tls_desc_rel, result, result, stImm));
}

void AArch64CGFunc::SelectCTlsGlobalDesc(Operand &result, StImmOperand &stImm)
{
    /* according to AArch64 Machine Directives */
    auto &r0opnd = GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, GetRegTyFromPrimTy(PTY_u64));
    RegOperand *tlsAddr = &CreateRegisterOperandOfType(PTY_u64);
    RegOperand *specialFunc = &CreateRegisterOperandOfType(PTY_u64);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_tls_desc_call, r0opnd, *tlsAddr, stImm));
    /* release tls address */
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_pseduo_tls_release, *tlsAddr));
    //  mrs xn, tpidr_el0
    //  add x0, x0, xn
    auto tpidr = &CreateCommentOperand("tpidr_el0");
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_mrs, *specialFunc, *tpidr));
    SelectAdd(result, r0opnd, *specialFunc, PTY_u64);
}

void AArch64CGFunc::SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode)
{
    MIRIntrinsicID intrinsic = intrinsiccallNode.GetIntrinsic();

    if (GetCG()->GenerateVerboseCG()) {
        std::string comment = GetIntrinsicName(intrinsic);
        GetCurBB()->AppendInsn(CreateCommentInsn(comment));
    }

    /*
     * At this moment, we eagerly evaluates all argument expressions.  In theory,
     * there could be intrinsics that extract meta-information of variables, such as
     * their locations, rather than computing their values.  Applications
     * include building stack maps that help runtime libraries to find the values
     * of local variables (See @stackmap in LLVM), in which case knowing their
     * locations will suffice.
     */
    if (intrinsic == INTRN_MPL_CLINIT_CHECK) { /* special case */
        SelectMPLClinitCheck(intrinsiccallNode);
        return;
    }
    if (intrinsic == INTRN_MPL_PROF_COUNTER_INC) { /* special case */
        SelectMPLProfCounterInc(intrinsiccallNode);
        return;
    }
    if ((intrinsic == INTRN_MPL_CLEANUP_LOCALREFVARS) || (intrinsic == INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP) ||
        (intrinsic == INTRN_MPL_CLEANUP_NORETESCOBJS)) {
        return;
    }
    // js
    if (intrinsic == INTRN_ADD_WITH_OVERFLOW || intrinsic == INTRN_SUB_WITH_OVERFLOW ||
        intrinsic == INTRN_MUL_WITH_OVERFLOW) {
        SelectOverFlowCall(intrinsiccallNode);
        return;
    }
    switch (intrinsic) {
        case INTRN_C_va_start:
            SelectCVaStart(intrinsiccallNode);
            return;
        case INTRN_C___sync_lock_release_1:
            SelectCSyncLockRelease(intrinsiccallNode, PTY_u8);
            return;
        case INTRN_C___sync_lock_release_2:
            SelectCSyncLockRelease(intrinsiccallNode, PTY_u16);
            return;
        case INTRN_C___sync_lock_release_4:
            SelectCSyncLockRelease(intrinsiccallNode, PTY_u32);
            return;
        case INTRN_C___sync_lock_release_8:
            SelectCSyncLockRelease(intrinsiccallNode, PTY_u64);
            return;
        case INTRN_C___atomic_store_n:
            SelectCAtomicStoreN(intrinsiccallNode);
            return;
        case INTRN_vector_zip_v8u8:
        case INTRN_vector_zip_v8i8:
        case INTRN_vector_zip_v4u16:
        case INTRN_vector_zip_v4i16:
        case INTRN_vector_zip_v2u32:
        case INTRN_vector_zip_v2i32:
            SelectVectorZip(intrinsiccallNode.Opnd(0)->GetPrimType(),
                            HandleExpr(intrinsiccallNode, *intrinsiccallNode.Opnd(0)),
                            HandleExpr(intrinsiccallNode, *intrinsiccallNode.Opnd(1)));
            return;
        case INTRN_C_stack_save:
            return;
        case INTRN_C_stack_restore:
            return;
        case INTRN_C___builtin_division_exception:
            SelectCDIVException();
            return;
        default:
            break;
    }
    std::vector<Operand *> operands; /* Temporary.  Deallocated on return. */
    ListOperand *srcOpnds = CreateListOpnd(*GetFuncScopeAllocator());
    for (size_t i = 0; i < intrinsiccallNode.NumOpnds(); i++) {
        BaseNode *argExpr = intrinsiccallNode.Opnd(i);
        Operand *opnd = HandleExpr(intrinsiccallNode, *argExpr);
        operands.emplace_back(opnd);
        if (!opnd->IsRegister()) {
            opnd = &LoadIntoRegister(*opnd, argExpr->GetPrimType());
        }
        RegOperand *expRegOpnd = static_cast<RegOperand *>(opnd);
        srcOpnds->PushOpnd(*expRegOpnd);
    }
    CallReturnVector *retVals = &intrinsiccallNode.GetReturnVec();

    switch (intrinsic) {
        case INTRN_MPL_ATOMIC_EXCHANGE_PTR: {
            BB *origFtBB = GetCurBB()->GetNext();
            Operand *loc = operands[kInsnFirstOpnd];
            Operand *newVal = operands[kInsnSecondOpnd];
            Operand *memOrd = operands[kInsnThirdOpnd];

            MemOrd ord = OperandToMemOrd(*memOrd);
            bool isAcquire = MemOrdIsAcquire(ord);
            bool isRelease = MemOrdIsRelease(ord);

            const PrimType kValPrimType = PTY_a64;

            RegOperand &locReg = LoadIntoRegister(*loc, PTY_a64);
            /* Because there is no live analysis when -O1 */
            if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
                locReg.SetRegNotBBLocal();
            }
            MemOperand &locMem = GetOrCreateMemOpnd(MemOperand::kAddrModeBOi, k64BitSize, &locReg, nullptr,
                                                    &GetOrCreateOfstOpnd(0, k32BitSize), nullptr);
            RegOperand &newValReg = LoadIntoRegister(*newVal, PTY_a64);
            if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
                newValReg.SetRegNotBBLocal();
            }
            GetCurBB()->SetKind(BB::kBBFallthru);

            LabelIdx retryLabIdx = CreateLabeledBB(intrinsiccallNode);

            RegOperand *oldVal = SelectLoadExcl(kValPrimType, locMem, isAcquire);
            if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
                oldVal->SetRegNotBBLocal();
            }
            RegOperand *succ = SelectStoreExcl(kValPrimType, locMem, newValReg, isRelease);
            if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
                succ->SetRegNotBBLocal();
            }

            GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wcbnz, *succ, GetOrCreateLabelOperand(retryLabIdx)));
            GetCurBB()->SetKind(BB::kBBIntrinsic);
            GetCurBB()->SetNext(origFtBB);

            SaveReturnValueInLocal(*retVals, 0, kValPrimType, *oldVal, intrinsiccallNode);
            break;
        }
        case INTRN_GET_AND_ADDI: {
            IntrinsifyGetAndAddInt(*srcOpnds, PTY_i32);
            break;
        }
        case INTRN_GET_AND_ADDL: {
            IntrinsifyGetAndAddInt(*srcOpnds, PTY_i64);
            break;
        }
        case INTRN_GET_AND_SETI: {
            IntrinsifyGetAndSetInt(*srcOpnds, PTY_i32);
            break;
        }
        case INTRN_GET_AND_SETL: {
            IntrinsifyGetAndSetInt(*srcOpnds, PTY_i64);
            break;
        }
        case INTRN_COMP_AND_SWAPI: {
            IntrinsifyCompareAndSwapInt(*srcOpnds, PTY_i32);
            break;
        }
        case INTRN_COMP_AND_SWAPL: {
            IntrinsifyCompareAndSwapInt(*srcOpnds, PTY_i64);
            break;
        }
        case INTRN_C___atomic_exchange_n: {
            Operand *oldVal = SelectCAtomicExchangeN(intrinsiccallNode);
            auto primType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(intrinsiccallNode.GetTyIdx())->GetPrimType();
            uint32 regSize = GetPrimTypeBitSize(primType);
            SelectCopy(GetOrCreatePhysicalRegisterOperand(R0, regSize, kRegTyInt), primType, *oldVal, primType);
            break;
        }
        default: {
            CHECK_FATAL(false, "Intrinsic %d: %s not implemented by the AArch64 CG.", intrinsic,
                        GetIntrinsicName(intrinsic));
            break;
        }
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

Operand *AArch64CGFunc::SelectCctz(IntrinsicopNode &intrnNode)
{
    BaseNode *argexpr = intrnNode.Opnd(0);
    PrimType ptype = argexpr->GetPrimType();
    Operand *opnd = HandleExpr(intrnNode, *argexpr);

    RegOperand &ldDest = CreateRegisterOperandOfType(ptype);
    if (opnd->IsMemoryAccessOperand()) {
        Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype), ptype), ldDest, *opnd);
        GetCurBB()->AppendInsn(insn);
        opnd = &ldDest;
    } else if (opnd->IsImmediate()) {
        SelectCopyImm(ldDest, *static_cast<ImmOperand *>(opnd), ptype);
        opnd = &ldDest;
    }

    MOperator clzmop;
    MOperator rbitmop;
    if (GetPrimTypeSize(ptype) == k4ByteSize) {
        clzmop = MOP_wclz;
        rbitmop = MOP_wrbit;
    } else {
        clzmop = MOP_xclz;
        rbitmop = MOP_xrbit;
    }
    RegOperand &dst1 = CreateRegisterOperandOfType(ptype);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(rbitmop, dst1, *opnd));
    RegOperand &dst2 = CreateRegisterOperandOfType(ptype);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(clzmop, dst2, dst1));
    return &dst2;
}

Operand *AArch64CGFunc::SelectCpopcount(IntrinsicopNode &intrnNode)
{
    CHECK_FATAL(false, "%s NIY", intrnNode.GetIntrinDesc().name);
    return nullptr;
}

Operand *AArch64CGFunc::SelectCparity(IntrinsicopNode &intrnNode)
{
    CHECK_FATAL(false, "%s NIY", intrnNode.GetIntrinDesc().name);
    return nullptr;
}

Operand *AArch64CGFunc::SelectCclrsb(IntrinsicopNode &intrnNode)
{
    BaseNode *argexpr = intrnNode.Opnd(0);
    PrimType ptype = argexpr->GetPrimType();
    Operand *opnd = HandleExpr(intrnNode, *argexpr);

    RegOperand &ldDest = CreateRegisterOperandOfType(ptype);
    if (opnd->IsMemoryAccessOperand()) {
        Insn &insn = GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype), ptype), ldDest, *opnd);
        GetCurBB()->AppendInsn(insn);
        opnd = &ldDest;
    } else if (opnd->IsImmediate()) {
        SelectCopyImm(ldDest, *static_cast<ImmOperand *>(opnd), ptype);
        opnd = &ldDest;
    }

    bool is32Bit = (GetPrimTypeSize(ptype) == k4ByteSize);
    RegOperand &res = CreateRegisterOperandOfType(ptype);
    SelectMvn(res, *opnd, ptype);
    SelectAArch64Cmp(*opnd, GetZeroOpnd(is32Bit ? k32BitSize : k64BitSize), true, is32Bit ? k32BitSize : k64BitSize);
    SelectAArch64Select(*opnd, res, *opnd, GetCondOperand(CC_LT), true, is32Bit ? k32BitSize : k64BitSize);
    MOperator clzmop = (is32Bit ? MOP_wclz : MOP_xclz);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(clzmop, *opnd, *opnd));
    SelectSub(*opnd, *opnd, CreateImmOperand(1, is32Bit ? k32BitSize : k64BitSize, true), ptype);
    return opnd;
}

Operand *AArch64CGFunc::SelectCisaligned(IntrinsicopNode &intrnNode)
{
    BaseNode *argexpr0 = intrnNode.Opnd(0);
    PrimType ptype0 = argexpr0->GetPrimType();
    Operand *opnd0 = HandleExpr(intrnNode, *argexpr0);

    RegOperand &ldDest0 = CreateRegisterOperandOfType(ptype0);
    if (opnd0->IsMemoryAccessOperand()) {
        GetCurBB()->AppendInsn(
            GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype0), ptype0), ldDest0, *opnd0));
        opnd0 = &ldDest0;
    } else if (opnd0->IsImmediate()) {
        SelectCopyImm(ldDest0, *static_cast<ImmOperand *>(opnd0), ptype0);
        opnd0 = &ldDest0;
    }

    BaseNode *argexpr1 = intrnNode.Opnd(1);
    PrimType ptype1 = argexpr1->GetPrimType();
    Operand *opnd1 = HandleExpr(intrnNode, *argexpr1);

    RegOperand &ldDest1 = CreateRegisterOperandOfType(ptype1);
    if (opnd1->IsMemoryAccessOperand()) {
        GetCurBB()->AppendInsn(
            GetInsnBuilder()->BuildInsn(PickLdInsn(GetPrimTypeBitSize(ptype1), ptype1), ldDest1, *opnd1));
        opnd1 = &ldDest1;
    } else if (opnd1->IsImmediate()) {
        SelectCopyImm(ldDest1, *static_cast<ImmOperand *>(opnd1), ptype1);
        opnd1 = &ldDest1;
    }
    // mov w4, #1
    RegOperand &reg0 = CreateRegisterOperandOfType(PTY_i32);
    SelectCopyImm(reg0, CreateImmOperand(1, k32BitSize, true), PTY_i32);
    // sxtw x4, w4
    MOperator mOp = MOP_xsxtw64;
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, reg0, reg0));
    // sub x3, x3, x4
    SelectSub(*opnd1, *opnd1, reg0, ptype1);
    // and x2, x2, x3
    SelectBand(*opnd0, *opnd0, *opnd1, ptype1);
    // mov w3, #0
    // sxtw x3, w3
    // cmp x2, x3
    SelectAArch64Cmp(*opnd0, GetZeroOpnd(k64BitSize), true, k64BitSize);
    // cset w2, EQ
    SelectAArch64CSet(*opnd0, GetCondOperand(CC_EQ), false);
    return opnd0;
}

void AArch64CGFunc::SelectArithmeticAndLogical(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType,
                                               Opcode op)
{
    switch (op) {
        case OP_add:
            SelectAdd(resOpnd, opnd0, opnd1, primType);
            break;
        case OP_sub:
            SelectSub(resOpnd, opnd0, opnd1, primType);
            break;
        case OP_band:
            SelectBand(resOpnd, opnd0, opnd1, primType);
            break;
        case OP_bior:
            SelectBior(resOpnd, opnd0, opnd1, primType);
            break;
        case OP_bxor:
            SelectBxor(resOpnd, opnd0, opnd1, primType);
            break;
        default:
            CHECK_FATAL(false, "unconcerned opcode for arithmetical and logical insns");
            break;
    }
}

Operand *AArch64CGFunc::SelectAArch64CSyncFetch(const IntrinsicopNode &intrinopNode, Opcode op, bool fetchBefore)
{
    auto primType = intrinopNode.GetPrimType();
    /* Create BB which includes atomic built_in function */
    LabelIdx atomicBBLabIdx = CreateLabel();
    BB *atomicBB = CreateNewBB();
    atomicBB->SetKind(BB::kBBIf);
    atomicBB->SetAtomicBuiltIn();
    atomicBB->AddLabel(atomicBBLabIdx);
    SetLab2BBMap(static_cast<int32>(atomicBBLabIdx), *atomicBB);
    GetCurBB()->AppendBB(*atomicBB);
    /* keep variables inside same BB */
    if (GetCG()->GetOptimizeLevel() == CGOptions::kLevel0) {
        SetCurBB(*atomicBB);
    }
    /* handle built_in args */
    Operand *addrOpnd = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnFirstOpnd));
    Operand *valueOpnd = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnSecondOpnd));
    addrOpnd = &LoadIntoRegister(*addrOpnd, intrinopNode.GetNopndAt(kInsnFirstOpnd)->GetPrimType());
    valueOpnd = &LoadIntoRegister(*valueOpnd, intrinopNode.GetNopndAt(kInsnSecondOpnd)->GetPrimType());
    if (GetCG()->GetOptimizeLevel() != CGOptions::kLevel0) {
        SetCurBB(*atomicBB);
    }
    /* load from pointed address */
    auto primTypeP2Size = GetPrimTypeP2Size(primType);
    auto *regLoaded = &CreateRegisterOperandOfType(primType);
    auto &memOpnd = CreateMemOpnd(*static_cast<RegOperand *>(addrOpnd), 0, GetPrimTypeBitSize(primType));
    auto mOpLoad = PickLoadStoreExclInsn(primTypeP2Size, false, false);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(mOpLoad, *regLoaded, memOpnd));
    /* update loaded value */
    auto *regOperated = &CreateRegisterOperandOfType(primType);
    SelectArithmeticAndLogical(*regOperated, *regLoaded, *valueOpnd, primType, op);
    /* store to pointed address */
    auto *accessStatus = &CreateRegisterOperandOfType(PTY_u32);
    auto mOpStore = PickLoadStoreExclInsn(primTypeP2Size, true, true);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(mOpStore, *accessStatus, *regOperated, memOpnd));
    /* check the exclusive accsess status */
    auto &atomicBBOpnd = GetOrCreateLabelOperand(*atomicBB);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wcbnz, *accessStatus, atomicBBOpnd));

    /* Data Memory Barrier */
    BB *nextBB = CreateNewBB();
    atomicBB->AppendBB(*nextBB);
    SetCurBB(*nextBB);
    nextBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_dmb_ish, AArch64CG::kMd[MOP_dmb_ish]));
    return fetchBefore ? regLoaded : regOperated;
}

Operand *AArch64CGFunc::SelectCSyncCmpSwap(const IntrinsicopNode &intrinopNode, bool retBool)
{
    PrimType primType = intrinopNode.GetNopndAt(kInsnSecondOpnd)->GetPrimType();
    LabelIdx atomicBBLabIdx = CreateLabel();
    /* Create BB which includes atomic built_in function */
    BB *atomicBB = CreateNewBB();
    atomicBB->SetKind(BB::kBBIf);
    atomicBB->SetAtomicBuiltIn();
    atomicBB->AddLabel(atomicBBLabIdx);
    SetLab2BBMap(static_cast<int32>(atomicBBLabIdx), *atomicBB);
    GetCurBB()->AppendBB(*atomicBB);
    /* keep variables inside same BB in O0 to avoid obtaining parameter value across BB blocks */
    if (GetCG()->GetOptimizeLevel() == CGOptions::kLevel0) {
        SetCurBB(*atomicBB);
    }
    /* handle built_in args */
    Operand *addrOpnd = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnFirstOpnd));
    Operand *oldVal = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnSecondOpnd));

    PrimType nodePrimType = intrinopNode.GetPrimType();
    uint32 nodePrimTypeSize = GetPrimTypeSize(nodePrimType);
    // ensure the value is not changed after loading with extention
    if (IsSignedInteger(nodePrimType) && nodePrimTypeSize < k4ByteSize && GetCurBB() &&
        GetCurBB()->GetLastMachineInsn() && GetCurBB()->GetLastMachineInsn()->IsLoad()) {
        int64 maxIntVal = nodePrimTypeSize == k1ByteSize ? kMax8UnsignedImm : kMax16UnsignedImm;
        Operand &lastDestOpnd = GetCurBB()->GetLastMachineInsn()->GetOperand(kInsnFirstOpnd);
        DEBUG_ASSERT(lastDestOpnd.IsRegister(), "last insn's 1st operand is not register.");
        ImmOperand &immOpnd = CreateImmOperand(primType, maxIntVal);
        Insn &andInsn = GetInsnBuilder()->BuildInsn(MOP_wandrri12, lastDestOpnd, lastDestOpnd, immOpnd);
        GetCurBB()->AppendInsn(andInsn);
    }

    Operand *newVal = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnThirdOpnd));
    if (GetCG()->GetOptimizeLevel() != CGOptions::kLevel0) {
        SetCurBB(*atomicBB);
    }

    uint32 nodePrimTypeP2Size = GetPrimTypeP2Size(nodePrimType);
    /* ldxr */
    auto *regLoaded = &CreateRegisterOperandOfType(primType);
    auto &memOpnd = CreateMemOpnd(LoadIntoRegister(*addrOpnd, primType), 0, GetPrimTypeBitSize(primType));
    auto mOpLoad = PickLoadStoreExclInsn(nodePrimTypeP2Size, false, false);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(mOpLoad, *regLoaded, memOpnd));
    Operand *regExtend = &CreateRegisterOperandOfType(primType);
    PrimType targetType = (oldVal->GetSize() <= k32BitSize) ? (IsSignedInteger(primType) ? PTY_i32 : PTY_u32)
                                                            : (IsSignedInteger(primType) ? PTY_i64 : PTY_u64);
    SelectCvtInt2Int(nullptr, regExtend, regLoaded, primType, targetType);
    /* cmp */
    SelectAArch64Cmp(*regExtend, *oldVal, true, oldVal->GetSize());
    /* bne */
    Operand &rflag = GetOrCreateRflag();
    LabelIdx nextBBLableIdx = CreateLabel();
    LabelOperand &targetOpnd = GetOrCreateLabelOperand(nextBBLableIdx);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_bne, rflag, targetOpnd));
    /* stlxr */
    BB *stlxrBB = CreateNewBB();
    stlxrBB->SetKind(BB::kBBIf);
    atomicBB->AppendBB(*stlxrBB);
    SetCurBB(*stlxrBB);
    auto *accessStatus = &CreateRegisterOperandOfType(PTY_u32);
    auto &newRegVal = LoadIntoRegister(*newVal, primType);
    auto mOpStore = PickLoadStoreExclInsn(nodePrimTypeP2Size, true, true);
    stlxrBB->AppendInsn(GetInsnBuilder()->BuildInsn(mOpStore, *accessStatus, newRegVal, memOpnd));
    /* cbnz ==> check the exclusive accsess status */
    auto &atomicBBOpnd = GetOrCreateLabelOperand(*atomicBB);
    stlxrBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wcbnz, *accessStatus, atomicBBOpnd));
    /* Data Memory Barrier */
    BB *nextBB = CreateNewBB();
    nextBB->AddLabel(nextBBLableIdx);
    nextBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_dmb_ish, AArch64CG::kMd[MOP_dmb_ish]));
    /* special handle for boolean return type */
    if (intrinopNode.GetPrimType() == PTY_u1) {
        nextBB->AppendInsn(
            GetInsnBuilder()->BuildInsn(MOP_wandrri12, *regLoaded, *regLoaded, CreateImmOperand(PTY_u32, 1)));
    }
    SetLab2BBMap(static_cast<int32>(nextBBLableIdx), *nextBB);
    stlxrBB->AppendBB(*nextBB);
    SetCurBB(*nextBB);
    /* bool version return true if the comparison is successful and newval is written */
    if (retBool) {
        auto *retOpnd = &CreateRegisterOperandOfType(PTY_u32);
        SelectAArch64CSet(*retOpnd, GetCondOperand(CC_EQ), false);
        return retOpnd;
    }
    /* type version return the contents of *addrOpnd before the operation */
    return regLoaded;
}

Operand *AArch64CGFunc::SelectCSyncFetch(IntrinsicopNode &intrinopNode, Opcode op, bool fetchBefore)
{
    return SelectAArch64CSyncFetch(intrinopNode, op, fetchBefore);
}

Operand *AArch64CGFunc::SelectCSyncBoolCmpSwap(IntrinsicopNode &intrinopNode)
{
    return SelectCSyncCmpSwap(intrinopNode, true);
}

Operand *AArch64CGFunc::SelectCSyncValCmpSwap(IntrinsicopNode &intrinopNode)
{
    return SelectCSyncCmpSwap(intrinopNode);
}

Operand *AArch64CGFunc::SelectCSyncLockTestSet(IntrinsicopNode &intrinopNode, PrimType pty)
{
    auto primType = intrinopNode.GetPrimType();
    /*handle builtin args */
    Operand *addrOpnd = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnFirstOpnd));
    Operand *valueOpnd = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnSecondOpnd));
    addrOpnd = &LoadIntoRegister(*addrOpnd, intrinopNode.GetNopndAt(kInsnFirstOpnd)->GetPrimType());
    valueOpnd = &LoadIntoRegister(*valueOpnd, intrinopNode.GetNopndAt(kInsnSecondOpnd)->GetPrimType());

    /* Create BB which includes atomic built_in function */
    LabelIdx atomicBBLabIdx = CreateLabel();
    BB *atomicBB = CreateNewBB();
    atomicBB->SetKind(BB::kBBIf);
    atomicBB->SetAtomicBuiltIn();
    atomicBB->AddLabel(atomicBBLabIdx);
    SetLab2BBMap(static_cast<int32>(atomicBBLabIdx), *atomicBB);
    GetCurBB()->AppendBB(*atomicBB);
    SetCurBB(*atomicBB);
    /* load from pointed address */
    auto primTypeP2Size = GetPrimTypeP2Size(primType);
    auto *regLoaded = &CreateRegisterOperandOfType(primType);
    auto &memOpnd = CreateMemOpnd(*static_cast<RegOperand *>(addrOpnd), 0, GetPrimTypeBitSize(primType));
    auto mOpLoad = PickLoadStoreExclInsn(primTypeP2Size, false, false);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(mOpLoad, *regLoaded, memOpnd));
    /* store to pointed address */
    auto *accessStatus = &CreateRegisterOperandOfType(PTY_u32);
    auto mOpStore = PickLoadStoreExclInsn(primTypeP2Size, true, false);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(mOpStore, *accessStatus, *valueOpnd, memOpnd));
    /* check the exclusive accsess status */
    auto &atomicBBOpnd = GetOrCreateLabelOperand(*atomicBB);
    atomicBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_wcbnz, *accessStatus, atomicBBOpnd));

    /* Data Memory Barrier */
    BB *nextBB = CreateNewBB();
    atomicBB->AppendBB(*nextBB);
    SetCurBB(*nextBB);
    nextBB->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_dmb_ish, AArch64CG::kMd[MOP_dmb_ish]));
    return regLoaded;
}

void AArch64CGFunc::SelectCSyncLockRelease(const IntrinsiccallNode &intrinsiccall, PrimType primType)
{
    auto *addrOpnd = HandleExpr(intrinsiccall, *intrinsiccall.GetNopndAt(kInsnFirstOpnd));
    auto primTypeBitSize = GetPrimTypeBitSize(primType);
    auto mOp = PickStInsn(primTypeBitSize, primType, AArch64isa::kMoRelease);
    auto &zero = GetZeroOpnd(primTypeBitSize);
    auto &memOpnd = CreateMemOpnd(LoadIntoRegister(*addrOpnd, primType), 0, primTypeBitSize);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, zero, memOpnd));
}

Operand *AArch64CGFunc::SelectCSyncSynchronize(IntrinsicopNode &intrinopNode)
{
    (void)intrinopNode;
    CHECK_FATAL(false, "have not implement SelectCSyncSynchronize yet");
    return nullptr;
}

AArch64isa::MemoryOrdering AArch64CGFunc::PickMemOrder(std::memory_order memOrder, bool isLdr) const
{
    switch (memOrder) {
        case std::memory_order_relaxed:
            return AArch64isa::kMoNone;
        case std::memory_order_consume:
        case std::memory_order_acquire:
            return isLdr ? AArch64isa::kMoAcquire : AArch64isa::kMoNone;
        case std::memory_order_release:
            return isLdr ? AArch64isa::kMoNone : AArch64isa::kMoRelease;
        case std::memory_order_acq_rel:
        case std::memory_order_seq_cst:
            return isLdr ? AArch64isa::kMoAcquire : AArch64isa::kMoRelease;
        default:
            CHECK_FATAL(false, "unexpected memorder");
            return AArch64isa::kMoNone;
    }
}

/*
 * regassign %1 (intrinsicop C___Atomic_Load_N(ptr, memorder))
 * ====> %1 = *ptr
 * let %1 -> x0
 * let ptr -> x1
 * implement to asm: ldr/ldar x0, [x1]
 * a load-acquire would replace ldr if memorder is not 0
 */
Operand *AArch64CGFunc::SelectCAtomicLoadN(IntrinsicopNode &intrinsicopNode)
{
    auto *addrOpnd = HandleExpr(intrinsicopNode, *intrinsicopNode.Opnd(0));
    auto *memOrderOpnd = intrinsicopNode.Opnd(1);
    auto primType = intrinsicopNode.GetPrimType();
    std::memory_order memOrder = std::memory_order_seq_cst;
    if (memOrderOpnd->IsConstval()) {
        auto *memOrderConst = static_cast<MIRIntConst *>(static_cast<ConstvalNode *>(memOrderOpnd)->GetConstVal());
        memOrder = static_cast<std::memory_order>(memOrderConst->GetExtValue());
    }
    return SelectAtomicLoad(*addrOpnd, primType, PickMemOrder(memOrder, true));
}

/*
 * regassign %1 (intrinsicop C___Atomic_exchange_n(ptr, val, memorder))
 * ====> %1 = *ptr; *ptr = val;
 * let %1 -> x0
 * let ptr -> x1
 * let val -> x2
 * implement to asm:
 * ldr/ldar x0, [x1]
 * str/stlr x2, [x1]
 * a load-acquire would replace ldr if acquire needed
 * a store-relase would replace str if release needed
 */
Operand *AArch64CGFunc::SelectCAtomicExchangeN(const IntrinsiccallNode &intrinsiccallNode)
{
    auto primType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(intrinsiccallNode.GetTyIdx())->GetPrimType();
    auto *addrOpnd = HandleExpr(intrinsiccallNode, *intrinsiccallNode.Opnd(0));
    auto *valueOpnd = HandleExpr(intrinsiccallNode, *intrinsiccallNode.Opnd(1));
    auto *memOrderOpnd = intrinsiccallNode.Opnd(kInsnThirdOpnd);

    /* slect memry order */
    std::memory_order memOrder = std::memory_order_seq_cst;
    if (memOrderOpnd->IsConstval()) {
        auto *memOrderConst = static_cast<MIRIntConst *>(static_cast<ConstvalNode *>(memOrderOpnd)->GetConstVal());
        memOrder = static_cast<std::memory_order>(memOrderConst->GetExtValue());
    }
    auto *result = SelectAtomicLoad(*addrOpnd, primType, PickMemOrder(memOrder, true));
    SelectAtomicStore(*valueOpnd, *addrOpnd, primType, PickMemOrder(memOrder, false));
    return result;
}

Operand *AArch64CGFunc::SelectAtomicLoad(Operand &addrOpnd, PrimType primType, AArch64isa::MemoryOrdering memOrder)
{
    auto mOp = PickLdInsn(GetPrimTypeBitSize(primType), primType, memOrder);
    auto &memOpnd = CreateMemOpnd(LoadIntoRegister(addrOpnd, PTY_a64), 0, k64BitSize);
    auto *resultOpnd = &CreateRegisterOperandOfType(primType);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, *resultOpnd, memOpnd));
    return resultOpnd;
}

Operand *AArch64CGFunc::SelectCReturnAddress(IntrinsicopNode &intrinopNode)
{
    if (intrinopNode.GetIntrinsic() == INTRN_C__builtin_extract_return_addr) {
        DEBUG_ASSERT(intrinopNode.GetNumOpnds() == 1, "expect one parameter");
        Operand *addrOpnd = HandleExpr(intrinopNode, *intrinopNode.GetNopndAt(kInsnFirstOpnd));
        return &LoadIntoRegister(*addrOpnd, PTY_a64);
    } else if (intrinopNode.GetIntrinsic() == INTRN_C__builtin_return_address) {
        BaseNode *argexpr0 = intrinopNode.Opnd(0);
        while (!argexpr0->IsLeaf()) {
            argexpr0 = argexpr0->Opnd(0);
        }
        CHECK_FATAL(argexpr0->IsConstval(), "Invalid argument of __builtin_return_address");
        auto &constNode = static_cast<ConstvalNode &>(*argexpr0);
        DEBUG_ASSERT(constNode.GetConstVal()->GetKind() == kConstInt, "expect MIRIntConst does not support float yet");
        MIRIntConst *mirIntConst = safe_cast<MIRIntConst>(constNode.GetConstVal());
        DEBUG_ASSERT(mirIntConst != nullptr, "nullptr checking");
        int64 scale = mirIntConst->GetExtValue();
        /*
         * Do not support getting return address with a nonzero argument
         * inline / tail call opt will destory this behavior
         */
        CHECK_FATAL(scale == 0, "Do not support recursion");
        Operand *resReg = &static_cast<Operand &>(CreateRegisterOperandOfType(PTY_i64));
        SelectCopy(*resReg, PTY_i64, GetOrCreatePhysicalRegisterOperand(RLR, k64BitSize, kRegTyInt), PTY_i64);
        return resReg;
    }
    return nullptr;
}

Operand *AArch64CGFunc::SelectCalignup(IntrinsicopNode &intrnNode)
{
    return SelectAArch64align(intrnNode, true);
}

Operand *AArch64CGFunc::SelectCaligndown(IntrinsicopNode &intrnNode)
{
    return SelectAArch64align(intrnNode, false);
}

Operand *AArch64CGFunc::SelectAArch64align(const IntrinsicopNode &intrnNode, bool isUp)
{
    /* Handle Two args */
    BaseNode *argexpr0 = intrnNode.Opnd(0);
    PrimType ptype0 = argexpr0->GetPrimType();
    Operand *opnd0 = HandleExpr(intrnNode, *argexpr0);
    PrimType resultPtype = intrnNode.GetPrimType();
    RegOperand &ldDest0 = LoadIntoRegister(*opnd0, ptype0);

    BaseNode *argexpr1 = intrnNode.Opnd(1);
    PrimType ptype1 = argexpr1->GetPrimType();
    Operand *opnd1 = HandleExpr(intrnNode, *argexpr1);
    RegOperand &arg1 = LoadIntoRegister(*opnd1, ptype1);
    DEBUG_ASSERT(IsPrimitiveInteger(ptype0) && IsPrimitiveInteger(ptype1), "align integer type only");
    Operand *ldDest1 = &static_cast<Operand &>(CreateRegisterOperandOfType(ptype0));
    SelectCvtInt2Int(nullptr, ldDest1, &arg1, ptype1, ptype0);

    Operand *resultReg = &static_cast<Operand &>(CreateRegisterOperandOfType(ptype0));
    Operand &immReg = CreateImmOperand(1, GetPrimTypeBitSize(ptype0), true);
    /* Do alignment  x0 -- value to be aligned   x1 -- alignment */
    if (isUp) {
        /* add res, x0, x1 */
        SelectAdd(*resultReg, ldDest0, *ldDest1, ptype0);
        /* sub res, res, 1 */
        SelectSub(*resultReg, *resultReg, immReg, ptype0);
    }
    Operand *tempReg = &static_cast<Operand &>(CreateRegisterOperandOfType(ptype0));
    /* sub temp, x1, 1 */
    SelectSub(*tempReg, *ldDest1, immReg, ptype0);
    /* mvn temp, temp */
    SelectMvn(*tempReg, *tempReg, ptype0);
    /* and res, res, temp */
    if (isUp) {
        SelectBand(*resultReg, *resultReg, *tempReg, ptype0);
    } else {
        SelectBand(*resultReg, ldDest0, *tempReg, ptype0);
    }
    if (resultPtype != ptype0) {
        SelectCvtInt2Int(&intrnNode, resultReg, resultReg, ptype0, resultPtype);
    }
    return resultReg;
}

void AArch64CGFunc::SelectCDIVException()
{
    uint32 breakImm = 1000;
    ImmOperand &immOpnd = CreateImmOperand(breakImm, maplebe::k16BitSize, false);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_brk, immOpnd));
}

/*
 * NOTE: consider moving the following things into aarch64_cg.cpp  They may
 * serve not only inrinsics, but other MapleIR instructions as well.
 * Do it as if we are adding a label in straight-line assembly code.
 */
LabelIdx AArch64CGFunc::CreateLabeledBB(StmtNode &stmt)
{
    LabelIdx labIdx = CreateLabel();
    BB *newBB = StartNewBBImpl(false, stmt);
    newBB->AddLabel(labIdx);
    SetLab2BBMap(labIdx, *newBB);
    SetCurBB(*newBB);
    return labIdx;
}

/* Save value into the local variable for the index-th return value; */
void AArch64CGFunc::SaveReturnValueInLocal(CallReturnVector &retVals, size_t index, PrimType primType, Operand &value,
                                           StmtNode &parentStmt)
{
    CallReturnPair &pair = retVals.at(index);
    BB tempBB(static_cast<uint32>(-1), *GetFuncScopeAllocator());
    BB *realCurBB = GetCurBB();
    CHECK_FATAL(!pair.second.IsReg(), "NYI");
    Operand *destOpnd = &value;
    /* for O0 ,corss-BB var is not support, do extra store/load but why new BB */
    if (GetCG()->GetOptimizeLevel() == CGOptions::kLevel0) {
        MIRSymbol *symbol = GetFunction().GetLocalOrGlobalSymbol(pair.first);
        MIRType *sPty = symbol->GetType();
        PrimType ty = symbol->GetType()->GetPrimType();
        if (sPty->GetKind() == kTypeStruct || sPty->GetKind() == kTypeUnion) {
            MIRStructType *structType = static_cast<MIRStructType *>(sPty);
            ty = structType->GetFieldType(pair.second.GetFieldID())->GetPrimType();
        } else if (sPty->GetKind() == kTypeClass) {
            CHECK_FATAL(false, "unsuppotr type for inlineasm / intrinsic");
        }
        RegOperand &tempReg = CreateVirtualRegisterOperand(NewVReg(GetRegTyFromPrimTy(ty), GetPrimTypeSize(ty)));
        SelectCopy(tempReg, ty, value, ty);
        destOpnd = &tempReg;
    }
    SetCurBB(tempBB);
    SelectDassign(pair.first, pair.second.GetFieldID(), primType, *destOpnd);

    CHECK_FATAL(realCurBB->GetNext() == nullptr, "current BB must has not nextBB");
    realCurBB->SetLastStmt(parentStmt);
    realCurBB->SetNext(StartNewBBImpl(true, parentStmt));
    realCurBB->GetNext()->SetKind(BB::kBBFallthru);
    realCurBB->GetNext()->SetPrev(realCurBB);

    realCurBB->GetNext()->InsertAtBeginning(*GetCurBB());
    /* restore it */
    SetCurBB(*realCurBB->GetNext());
}

/* The following are translation of LL/SC and atomic RMW operations */
MemOrd AArch64CGFunc::OperandToMemOrd(Operand &opnd) const
{
    CHECK_FATAL(opnd.IsImmediate(), "Memory order must be an int constant.");
    auto immOpnd = static_cast<ImmOperand *>(&opnd);
    int32 val = immOpnd->GetValue();
    CHECK_FATAL(val >= 0, "val must be non-negtive");
    return MemOrdFromU32(static_cast<uint32>(val));
}

/*
 * Generate ldxr or ldaxr instruction.
 * byte_p2x: power-of-2 size of operand in bytes (0: 1B, 1: 2B, 2: 4B, 3: 8B).
 */
MOperator AArch64CGFunc::PickLoadStoreExclInsn(uint32 byteP2Size, bool store, bool acqRel) const
{
    CHECK_FATAL(byteP2Size < kIntByteSizeDimension, "Illegal argument p2size: %d", byteP2Size);

    static MOperator operators[4][2][2] = {{{MOP_wldxrb, MOP_wldaxrb}, {MOP_wstxrb, MOP_wstlxrb}},
                                           {{MOP_wldxrh, MOP_wldaxrh}, {MOP_wstxrh, MOP_wstlxrh}},
                                           {{MOP_wldxr, MOP_wldaxr}, {MOP_wstxr, MOP_wstlxr}},
                                           {{MOP_xldxr, MOP_xldaxr}, {MOP_xstxr, MOP_xstlxr}}};

    MOperator optr = operators[byteP2Size][store][acqRel];
    CHECK_FATAL(optr != MOP_undef, "Unsupported type p2size: %d", byteP2Size);

    return optr;
}

RegOperand *AArch64CGFunc::SelectLoadExcl(PrimType valPrimType, MemOperand &loc, bool acquire)
{
    uint32 p2size = GetPrimTypeP2Size(valPrimType);

    RegOperand &result = CreateRegisterOperandOfType(valPrimType);
    MOperator mOp = PickLoadStoreExclInsn(p2size, false, acquire);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, result, loc));

    return &result;
}

RegOperand *AArch64CGFunc::SelectStoreExcl(PrimType valPty, MemOperand &loc, RegOperand &newVal, bool release)
{
    uint32 p2size = GetPrimTypeP2Size(valPty);

    /* the result (success/fail) is to be stored in a 32-bit register */
    RegOperand &result = CreateRegisterOperandOfType(PTY_u32);

    MOperator mOp = PickLoadStoreExclInsn(p2size, true, release);
    GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(mOp, result, newVal, loc));

    return &result;
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

RegOperand *AArch64CGFunc::AdjustOneElementVectorOperand(PrimType oType, RegOperand *opnd)
{
    RegOperand *resCvt = &CreateRegisterOperandOfType(oType);
    Insn *insnCvt = &GetInsnBuilder()->BuildInsn(MOP_xvmovrd, *resCvt, *opnd);
    GetCurBB()->AppendInsn(*insnCvt);
    return resCvt;
}

RegOperand *AArch64CGFunc::SelectOneElementVectorCopy(Operand *src, PrimType sType)
{
    RegOperand *res = &CreateRegisterOperandOfType(PTY_f64);
    SelectCopy(*res, PTY_f64, *src, sType);
    static_cast<RegOperand *>(res)->SetIF64Vec();
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorAbs(PrimType rType, Operand *o1)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */

    MOperator mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vabsvv : MOP_vabsuu;
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorAddLong(PrimType rType, Operand *o1, Operand *o2, PrimType otyp, bool isLow)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result type */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(otyp); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(otyp); /* vector operand 2 */
    MOperator mOp;
    if (isLow) {
        mOp = IsUnsignedInteger(rType) ? MOP_vuaddlvuu : MOP_vsaddlvuu;
    } else {
        mOp = IsUnsignedInteger(rType) ? MOP_vuaddl2vvv : MOP_vsaddl2vvv;
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorAddWiden(Operand *o1, PrimType otyp1, Operand *o2, PrimType otyp2, bool isLow)
{
    RegOperand *res = &CreateRegisterOperandOfType(otyp1); /* restype is same as o1 */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(otyp1);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(otyp1); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(otyp2); /* vector operand 2 */

    MOperator mOp;
    if (isLow) {
        mOp = IsUnsignedInteger(otyp1) ? MOP_vuaddwvvu : MOP_vsaddwvvu;
    } else {
        mOp = IsUnsignedInteger(otyp1) ? MOP_vuaddw2vvv : MOP_vsaddw2vvv;
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorImmMov(PrimType rType, Operand *src, PrimType sType)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpec = GetMemoryPool()->New<VectorRegSpec>(rType);
    int64 val = static_cast<ImmOperand *>(src)->GetValue();
    /* copy the src imm operand to a reg if out of range */
    if ((GetVecEleSize(rType) >= k64BitSize) || (GetPrimTypeSize(sType) > k4ByteSize && val != 0) ||
        (val < kMinImmVal || val > kMaxImmVal)) {
        Operand *reg = &CreateRegisterOperandOfType(sType);
        SelectCopy(*reg, sType, *src, sType);
        return SelectVectorRegMov(rType, reg, sType);
    }

    MOperator mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vmovvi : MOP_vmovui;
    if (GetVecEleSize(rType) == k8BitSize && val < 0) {
        src = &CreateImmOperand(static_cast<uint8>(val), k8BitSize, true);
    } else if (val < 0) {
        src = &CreateImmOperand(-(val + 1), k8BitSize, true);
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vnotvi : MOP_vnotui;
    }

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src);
    vInsn.PushRegSpecEntry(vecSpec);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorRegMov(PrimType rType, Operand *src, PrimType sType)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpec = GetMemoryPool()->New<VectorRegSpec>(rType);

    MOperator mOp;
    if (GetPrimTypeSize(sType) > k4ByteSize) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vxdupvr : MOP_vxdupur;
    } else {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vwdupvr : MOP_vwdupur;
    }

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src);
    vInsn.PushRegSpecEntry(vecSpec);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorFromScalar(PrimType rType, Operand *src, PrimType sType)
{
    if (!IsPrimitiveVector(rType)) {
        return SelectOneElementVectorCopy(src, sType);
    } else if (src->IsConstImmediate()) {
        return SelectVectorImmMov(rType, src, sType);
    } else {
        return SelectVectorRegMov(rType, src, sType);
    }
}

RegOperand *AArch64CGFunc::SelectVectorDup(PrimType rType, Operand *src, bool getLow)
{
    PrimType oType = rType;
    rType = FilterOneElementVectorType(oType);
    RegOperand *res = &CreateRegisterOperandOfType(rType);
    VectorRegSpec *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(k2ByteSize, k64BitSize, getLow ? 0 : 1);

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(MOP_vduprv, AArch64CG::kMd[MOP_vduprv]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src);
    vInsn.PushRegSpecEntry(vecSpecSrc);
    GetCurBB()->AppendInsn(vInsn);
    if (oType != rType) {
        res = AdjustOneElementVectorOperand(oType, res);
        static_cast<RegOperand *>(res)->SetIF64Vec();
    }
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorGetElement(PrimType rType, Operand *src, PrimType sType, int32 lane)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType);                        /* result operand */
    VectorRegSpec *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(sType, lane); /* vector operand */

    MOperator mop;
    if (!IsPrimitiveVector(sType)) {
        mop = MOP_xmovrr;
    } else if (GetPrimTypeBitSize(rType) >= k64BitSize) {
        mop = MOP_vxmovrv;
    } else {
        mop = (GetPrimTypeBitSize(sType) > k64BitSize) ? MOP_vwmovrv : MOP_vwmovru;
    }

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src);
    vInsn.PushRegSpecEntry(vecSpecSrc);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

/* adalp o1, o2 instruction accumulates into o1, overwriting the original operand.
   Hence we perform c = vadalp(a,b) as
       T tmp = a;
       return tmp+b;
   The return value of vadalp is then assigned to c, leaving value of a intact.
 */
RegOperand *AArch64CGFunc::SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2)
{
    VectorRegSpec *vecSpecDest;
    RegOperand *res;

    if (!IsPrimitiveVector(sty1)) {
        RegOperand *resF = SelectOneElementVectorCopy(src1, sty1);
        res = &CreateRegisterOperandOfType(PTY_f64);
        SelectCopy(*res, PTY_f64, *resF, PTY_f64);
        vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(k1ByteSize, k64BitSize);
    } else {
        res = &CreateRegisterOperandOfType(sty1); /* result type same as sty1 */
        SelectCopy(*res, sty1, *src1, sty1);
        vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(sty1);
    }
    VectorRegSpec *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(sty2);

    MOperator mop;
    if (IsUnsignedInteger(sty1)) {
        mop = GetPrimTypeSize(sty1) > k8ByteSize ? MOP_vupadalvv : MOP_vupadaluu;
    } else {
        mop = GetPrimTypeSize(sty1) > k8ByteSize ? MOP_vspadalvv : MOP_vspadaluu;
    }

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpecSrc);
    GetCurBB()->AppendInsn(vInsn);
    if (!IsPrimitiveVector(sty1)) {
        res = AdjustOneElementVectorOperand(sty1, res);
    }
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType)
{
    PrimType oType = rType;
    rType = FilterOneElementVectorType(oType);
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(sType); /* source operand */

    if (rType == PTY_f64) {
        vecSpecDest->vecLaneMax = 1;
    }

    MOperator mop;
    if (IsUnsignedInteger(sType)) {
        mop = GetPrimTypeSize(sType) > k8ByteSize ? MOP_vupaddvv : MOP_vupadduu;
    } else {
        mop = GetPrimTypeSize(sType) > k8ByteSize ? MOP_vspaddvv : MOP_vspadduu;
    }

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src);
    /* dest pushed first, popped first */
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpecSrc);
    GetCurBB()->AppendInsn(vInsn);
    if (oType != rType) {
        res = AdjustOneElementVectorOperand(oType, res);
    }
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorSetElement(Operand *eOpnd, PrimType eType, Operand *vOpnd, PrimType vType,
                                                  int32 lane)
{
    if (!IsPrimitiveVector(vType)) {
        return SelectOneElementVectorCopy(eOpnd, eType);
    }
    RegOperand *reg = &CreateRegisterOperandOfType(eType); /* vector element type */
    SelectCopy(*reg, eType, *eOpnd, eType);
    VectorRegSpec *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(vType, lane); /* vector operand == result */

    MOperator mOp;
    if (GetPrimTypeSize(eType) > k4ByteSize) {
        mOp = GetPrimTypeSize(vType) > k8ByteSize ? MOP_vxinsvr : MOP_vxinsur;
    } else {
        mOp = GetPrimTypeSize(vType) > k8ByteSize ? MOP_vwinsvr : MOP_vwinsur;
    }

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*vOpnd).AddOpndChain(*reg);
    vInsn.PushRegSpecEntry(vecSpecSrc);
    GetCurBB()->AppendInsn(vInsn);
    return static_cast<RegOperand *>(vOpnd);
}

RegOperand *AArch64CGFunc::SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType);
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpecOpd1 = GetMemoryPool()->New<VectorRegSpec>(oTy);
    VectorRegSpec *vecSpecOpd2 = GetMemoryPool()->New<VectorRegSpec>(oTy); /* same opnd types */

    MOperator mop;
    if (isLow) {
        mop = IsPrimitiveUnSignedVector(rType) ? MOP_vuabdlvuu : MOP_vsabdlvuu;
    } else {
        mop = IsPrimitiveUnSignedVector(rType) ? MOP_vuabdl2vvv : MOP_vsabdl2vvv;
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpecOpd1).PushRegSpecEntry(vecSpecOpd2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorMerge(PrimType rType, Operand *o1, Operand *o2, int32 index)
{
    if (!IsPrimitiveVector(rType)) {
        static_cast<RegOperand *>(o1)->SetIF64Vec();
        return static_cast<RegOperand *>(o1); /* 64x1_t, index equals 0 */
    }
    RegOperand *res = &CreateRegisterOperandOfType(rType);
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpecOpd1 = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpecOpd2 = GetMemoryPool()->New<VectorRegSpec>(rType);

    ImmOperand *imm = &CreateImmOperand(index, k8BitSize, true);

    MOperator mOp = (GetPrimTypeSize(rType) > k8ByteSize) ? MOP_vextvvvi : MOP_vextuuui;
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2).AddOpndChain(*imm);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpecOpd1).PushRegSpecEntry(vecSpecOpd2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorReverse(PrimType rType, Operand *src, PrimType sType, uint32 size)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpecSrc = GetMemoryPool()->New<VectorRegSpec>(sType); /* vector operand */

    MOperator mOp;
    if (GetPrimTypeBitSize(rType) == k128BitSize) {
        mOp = size >= k64BitSize ? MOP_vrev64qq : (size >= k32BitSize ? MOP_vrev32qq : MOP_vrev16qq);
    } else if (GetPrimTypeBitSize(rType) == k64BitSize) {
        mOp = size >= k64BitSize ? MOP_vrev64dd : (size >= k32BitSize ? MOP_vrev32dd : MOP_vrev16dd);
    } else {
        CHECK_FATAL(false, "should not be here");
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*src);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpecSrc);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorSum(PrimType rType, Operand *o1, PrimType oType)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* uint32_t result */
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oType);
    RegOperand *iOpnd = &CreateRegisterOperandOfType(oType); /* float intermediate result */
    uint32 eSize = GetVecEleSize(oType);                     /* vector opd in bits */
    bool is16ByteVec = GetPrimTypeSize(oType) >= k16ByteSize;
    MOperator mOp;
    if (is16ByteVec) {
        mOp = eSize <= k8BitSize
                  ? MOP_vbaddvrv
                  : (eSize <= k16BitSize ? MOP_vhaddvrv : (eSize <= k32BitSize ? MOP_vsaddvrv : MOP_vdaddvrv));
    } else {
        mOp = eSize <= k8BitSize ? MOP_vbaddvru : (eSize <= k16BitSize ? MOP_vhaddvru : MOP_vsaddvru);
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*iOpnd).AddOpndChain(*o1);
    vInsn.PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);

    mOp = eSize > k32BitSize ? MOP_vxmovrv : MOP_vwmovrv;
    VectorInsn &vInsn2 = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    auto *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(oType);
    vInsn2.AddOpndChain(*res).AddOpndChain(*iOpnd);
    vecSpec2->vecLane = 0;
    vInsn2.PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn2);
    return res;
}

void AArch64CGFunc::PrepareVectorOperands(Operand **o1, PrimType &oty1, Operand **o2, PrimType &oty2)
{
    /* Only 1 operand can be non vector, otherwise it's a scalar operation, wouldn't come here */
    if (IsPrimitiveVector(oty1) == IsPrimitiveVector(oty2)) {
        return;
    }
    PrimType origTyp = !IsPrimitiveVector(oty2) ? oty2 : oty1;
    Operand *opd = !IsPrimitiveVector(oty2) ? *o2 : *o1;
    PrimType rType = !IsPrimitiveVector(oty2) ? oty1 : oty2; /* Type to dup into */
    RegOperand *res = &CreateRegisterOperandOfType(rType);
    VectorRegSpec *vecSpec = GetMemoryPool()->New<VectorRegSpec>(rType);

    bool immOpnd = false;
    if (opd->IsConstImmediate()) {
        int64 val = static_cast<ImmOperand *>(opd)->GetValue();
        if (val >= kMinImmVal && val <= kMaxImmVal && GetVecEleSize(rType) < k64BitSize) {
            immOpnd = true;
        } else {
            RegOperand *regOpd = &CreateRegisterOperandOfType(origTyp);
            SelectCopyImm(*regOpd, origTyp, static_cast<ImmOperand &>(*opd), origTyp);
            opd = static_cast<Operand *>(regOpd);
        }
    }

    /* need dup to vector operand */
    MOperator mOp;
    if (immOpnd) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vmovvi : MOP_vmovui; /* a const */
    } else {
        if (GetPrimTypeSize(origTyp) > k4ByteSize) {
            mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vxdupvr : MOP_vxdupur;
        } else {
            mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vwdupvr : MOP_vwdupur; /* a scalar var */
        }
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*opd);
    vInsn.PushRegSpecEntry(vecSpec);
    GetCurBB()->AppendInsn(vInsn);
    if (!IsPrimitiveVector(oty2)) {
        *o2 = static_cast<Operand *>(res);
        oty2 = rType;
    } else {
        *o1 = static_cast<Operand *>(res);
        oty1 = rType;
    }
}

void AArch64CGFunc::SelectVectorCvt(Operand *res, PrimType rType, Operand *o1, PrimType oType)
{
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oType); /* vector operand 1 */

    MOperator mOp;
    VectorInsn *insn;
    if (GetPrimTypeSize(rType) > GetPrimTypeSize(oType)) {
        /* expand, similar to vmov_XX() intrinsics */
        mOp = IsUnsignedInteger(rType) ? MOP_vushllvvi : MOP_vshllvvi;
        ImmOperand *imm = &CreateImmOperand(0, k8BitSize, true);
        insn = &GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
        insn->AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*imm);
    } else if (GetPrimTypeSize(rType) < GetPrimTypeSize(oType)) {
        /* extract, similar to vqmovn_XX() intrinsics */
        insn = &GetInsnBuilder()->BuildVectorInsn(MOP_vxtnuv, AArch64CG::kMd[MOP_vxtnuv]);
        insn->AddOpndChain(*res).AddOpndChain(*o1);
    } else {
        CHECK_FATAL(0, "Invalid cvt between 2 operands of the same size");
    }
    insn->PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(*insn);
}

RegOperand *AArch64CGFunc::SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc)
{
    if (IsUnsignedInteger(oty1) && (opc != OP_eq && opc != OP_ne)) {
        return nullptr; /* no unsigned instr for zero */
    }
    RegOperand *res = &CreateRegisterOperandOfType(oty1); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(oty1);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oty1); /* vector operand 1 */

    MOperator mOp;
    switch (opc) {
        case OP_eq:
        case OP_ne:
            mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vzcmeqvv : MOP_vzcmequu;
            break;
        case OP_gt:
            mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vzcmgtvv : MOP_vzcmgtuu;
            break;
        case OP_ge:
            mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vzcmgevv : MOP_vzcmgeuu;
            break;
        case OP_lt:
            mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vzcmltvv : MOP_vzcmltuu;
            break;
        case OP_le:
            mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vzcmlevv : MOP_vzcmleuu;
            break;
        default:
            CHECK_FATAL(0, "Invalid cc in vector compare");
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    if (opc == OP_ne) {
        res = SelectVectorNot(oty1, res);
    }
    return res;
}

/* Neon compare intrinsics always return unsigned vector, MapleIR for comparison always return
   signed.  Using type of 1st operand for operation here */
RegOperand *AArch64CGFunc::SelectVectorCompare(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, Opcode opc)
{
    if (o2->IsConstImmediate() && static_cast<ImmOperand *>(o2)->GetValue() == 0) {
        RegOperand *zeroCmp = SelectVectorCompareZero(o1, oty1, o2, opc);
        if (zeroCmp != nullptr) {
            return zeroCmp;
        }
    }
    PrepareVectorOperands(&o1, oty1, &o2, oty2);
    DEBUG_ASSERT(oty1 == oty2, "vector operand type mismatch");

    RegOperand *res = &CreateRegisterOperandOfType(oty1); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(oty1);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oty1); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(oty2); /* vector operand 2 */

    MOperator mOp;
    switch (opc) {
        case OP_eq:
        case OP_ne:
            mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vcmeqvvv : MOP_vcmequuu;
            break;
        case OP_lt:
        case OP_gt:
            if (IsUnsignedInteger(oty1)) {
                mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vcmhivvv : MOP_vcmhiuuu;
            } else {
                mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vcmgtvvv : MOP_vcmgtuuu;
            }
            break;
        case OP_le:
        case OP_ge:
            if (IsUnsignedInteger(oty1)) {
                mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vcmhsvvv : MOP_vcmhsuuu;
            } else {
                mOp = GetPrimTypeSize(oty1) > k8ByteSize ? MOP_vcmgevvv : MOP_vcmgeuuu;
            }
            break;
        default:
            CHECK_FATAL(0, "Invalid cc in vector compare");
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    if (opc == OP_lt || opc == OP_le) {
        vInsn.AddOpndChain(*res).AddOpndChain(*o2).AddOpndChain(*o1);
    } else {
        vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    }
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    if (opc == OP_ne) {
        res = SelectVectorNot(oty1, res);
    }
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorShift(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
                                             Opcode opc)
{
    PrepareVectorOperands(&o1, oty1, &o2, oty2);
    PrimType resultType = rType;
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 2 */

    if (!IsPrimitiveVector(rType)) {
        o1 = &SelectCopy(*o1, rType, PTY_f64);
        o2 = &SelectCopy(*o2, rType, PTY_f64);
        resultType = PTY_f64;
    }
    RegOperand *res = &CreateRegisterOperandOfType(resultType); /* result operand */

    /* signed and unsigned shl(v,v) both use sshl or ushl, they are the same */
    MOperator mOp;
    if (IsPrimitiveUnsigned(rType)) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vushlvvv : MOP_vushluuu;
    } else {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vshlvvv : MOP_vshluuu;
    }

    if (opc != OP_shl) {
        o2 = SelectVectorNeg(rType, o2);
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

uint32 ValidShiftConst(PrimType rType)
{
    switch (rType) {
        case PTY_v8u8:
        case PTY_v8i8:
        case PTY_v16u8:
        case PTY_v16i8:
            return k8BitSize;
        case PTY_v4u16:
        case PTY_v4i16:
        case PTY_v8u16:
        case PTY_v8i16:
            return k16BitSize;
        case PTY_v2u32:
        case PTY_v2i32:
        case PTY_v4u32:
        case PTY_v4i32:
            return k32BitSize;
        case PTY_v2u64:
        case PTY_v2i64:
            return k64BitSize;
        default:
            CHECK_FATAL(0, "Invalid Shift operand type");
    }
    return 0;
}

RegOperand *AArch64CGFunc::SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc)
{
    auto resultType = FilterOneElementVectorType(rType);
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */

    if (!imm->IsConstImmediate()) {
        CHECK_FATAL(0, "VectorUShiftImm has invalid shift const");
    }
    uint32 shift = static_cast<uint32>(ValidShiftConst(rType));
    bool needDup = false;
    if (opc == OP_shl) {
        if ((shift == k8BitSize && (sVal < 0 || static_cast<uint32>(sVal) >= shift)) ||
            (shift == k16BitSize && (sVal < 0 || static_cast<uint32>(sVal) >= shift)) ||
            (shift == k32BitSize && (sVal < 0 || static_cast<uint32>(sVal) >= shift)) ||
            (shift == k64BitSize && (sVal < 0 || static_cast<uint32>(sVal) >= shift))) {
            needDup = true;
        }
    } else {
        if ((shift == k8BitSize && (sVal < 1 || static_cast<uint32>(sVal) > shift)) ||
            (shift == k16BitSize && (sVal < 1 || static_cast<uint32>(sVal) > shift)) ||
            (shift == k32BitSize && (sVal < 1 || static_cast<uint32>(sVal) > shift)) ||
            (shift == k64BitSize && (sVal < 1 || static_cast<uint32>(sVal) > shift))) {
            needDup = true;
        }
    }
    if (needDup) {
        /* Dup constant to vector reg */
        SelectCopy(*res, resultType, *imm, imm->GetSize() == k64BitSize ? PTY_u64 : PTY_u32);
        res = SelectVectorShift(rType, o1, rType, res, rType, opc);
        return res;
    }
    MOperator mOp;
    if (GetPrimTypeSize(rType) > k8ByteSize) {
        if (IsUnsignedInteger(rType)) {
            mOp = opc == OP_shl ? MOP_vushlvvi : MOP_vushrvvi;
        } else {
            mOp = opc == OP_shl ? MOP_vushlvvi : MOP_vshrvvi;
        }
    } else {
        if (IsUnsignedInteger(rType)) {
            mOp = opc == OP_shl ? MOP_vushluui : MOP_vushruui;
        } else {
            mOp = opc == OP_shl ? MOP_vushluui : MOP_vshruui;
        }
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*imm);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorTableLookup(PrimType rType, Operand *o1, Operand *o2)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType);                   /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType); /* 8B or 16B */
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType);    /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(rType);    /* vector operand 2 */
    vecSpec1->compositeOpnds = 1;                                            /* composite operand */

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(MOP_vtbl1vvv, AArch64CG::kMd[MOP_vtbl1vvv]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2, Operand *o3,
                                            PrimType oTyp3)
{
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oTyp1); /* operand 1 and result */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(oTyp2); /* vector operand 2 */
    VectorRegSpec *vecSpec3 = GetMemoryPool()->New<VectorRegSpec>(oTyp3); /* vector operand 2 */

    MOperator mop = IsPrimitiveUnSignedVector(oTyp1) ? MOP_vumaddvvv : MOP_vsmaddvvv;
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
    vInsn.AddOpndChain(*o1).AddOpndChain(*o2).AddOpndChain(*o3);
    vInsn.PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2).PushRegSpecEntry(vecSpec3);
    GetCurBB()->AppendInsn(vInsn);
    return static_cast<RegOperand *>(o1);
}

RegOperand *AArch64CGFunc::SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
                                            bool isLow)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oTyp1); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(oTyp2); /* vector operand 1 */

    MOperator mop;
    if (isLow) {
        mop = IsPrimitiveUnSignedVector(rType) ? MOP_vumullvvv : MOP_vsmullvvv;
    } else {
        mop = IsPrimitiveUnSignedVector(rType) ? MOP_vumull2vvv : MOP_vsmull2vvv;
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mop, AArch64CG::kMd[mop]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
                                             Opcode opc)
{
    PrepareVectorOperands(&o1, oty1, &o2, oty2);
    DEBUG_ASSERT(oty1 == oty2, "vector operand type mismatch");

    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oty1); /* source operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(oty2); /* source operand 2 */

    MOperator mOp;
    if (opc == OP_add) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vaddvvv : MOP_vadduuu;
    } else if (opc == OP_sub) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vsubvvv : MOP_vsubuuu;
    } else if (opc == OP_mul) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vmulvvv : MOP_vmuluuu;
    } else {
        CHECK_FATAL(0, "Invalid opcode for SelectVectorBinOp");
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    /* dest pushed first, popped first */
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
                                                 Opcode opc)
{
    PrepareVectorOperands(&o1, oty1, &o2, oty2);
    DEBUG_ASSERT(oty1 == oty2, "vector operand type mismatch");

    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */

    MOperator mOp;
    if (opc == OP_band) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vandvvv : MOP_vanduuu;
    } else if (opc == OP_bior) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vorvvv : MOP_voruuu;
    } else if (opc == OP_bxor) {
        mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vxorvvv : MOP_vxoruuu;
    } else {
        CHECK_FATAL(0, "Invalid opcode for SelectVectorBitwiseOp");
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(otyp); /* vector operand */

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(MOP_vxtnuv, AArch64CG::kMd[MOP_vxtnuv]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2)
{
    (void)oty1;                                      /* 1st opnd was loaded already, type no longer needed */
    RegOperand *res = static_cast<RegOperand *>(o1); /* o1 is also the result */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(oty2); /* vector opnd2 */

    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(MOP_vxtn2uv, AArch64CG::kMd[MOP_vxtn2uv]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorNot(PrimType rType, Operand *o1)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */

    MOperator mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vnotvv : MOP_vnotuu;
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1);
    vInsn.PushRegSpecEntry(vecSpecDest).PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorNeg(PrimType rType, Operand *o1)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */

    MOperator mOp = GetPrimTypeSize(rType) > k8ByteSize ? MOP_vnegvv : MOP_vneguu;
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1);
    vInsn.PushRegSpecEntry(vecSpecDest);
    vInsn.PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

/*
 * Called internally for auto-vec, no intrinsics for now
 */
RegOperand *AArch64CGFunc::SelectVectorSelect(Operand &cond, PrimType rType, Operand &o0, Operand &o1)
{
    rType = GetPrimTypeSize(rType) > k8ByteSize ? PTY_v16u8 : PTY_v8u8;
    RegOperand *res = &CreateRegisterOperandOfType(rType);
    SelectCopy(*res, rType, cond, rType);
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(rType);

    uint32 mOp = GetPrimTypeBitSize(rType) > k64BitSize ? MOP_vbslvvv : MOP_vbsluuu;
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(o0).AddOpndChain(o1);
    vInsn.PushRegSpecEntry(vecSpecDest);
    vInsn.PushRegSpecEntry(vecSpec1);
    vInsn.PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oType, Operand *o2,
                                                    bool isLow)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(oType); /* vector operand 1 */

    ImmOperand *imm = static_cast<ImmOperand *>(o2);
    MOperator mOp;
    if (isLow) {
        mOp = MOP_vshrnuvi;
    } else {
        CHECK_FATAL(0, "NYI: vshrn_high_");
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*imm);
    vInsn.PushRegSpecEntry(vecSpecDest);
    vInsn.PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

RegOperand *AArch64CGFunc::SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1, Operand *o2,
                                                PrimType otyp2, bool isLow, bool isWide)
{
    RegOperand *res = &CreateRegisterOperandOfType(resType); /* result reg */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(resType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(otyp1); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(otyp2); /* vector operand 2 */

    MOperator mOp;
    if (!isWide) {
        if (isLow) {
            mOp = IsUnsignedInteger(otyp1) ? MOP_vusublvuu : MOP_vssublvuu;
        } else {
            mOp = IsUnsignedInteger(otyp1) ? MOP_vusubl2vvv : MOP_vssubl2vvv;
        }
    } else {
        if (isLow) {
            mOp = IsUnsignedInteger(otyp1) ? MOP_vusubwvvu : MOP_vssubwvvu;
        } else {
            mOp = IsUnsignedInteger(otyp1) ? MOP_vusubw2vvv : MOP_vssubw2vvv;
        }
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn.PushRegSpecEntry(vecSpecDest);
    vInsn.PushRegSpecEntry(vecSpec1);
    vInsn.PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn);
    return res;
}

void AArch64CGFunc::SelectVectorZip(PrimType rType, Operand *o1, Operand *o2)
{
    RegOperand *res1 = &CreateRegisterOperandOfType(rType); /* result operand 1 */
    RegOperand *res2 = &CreateRegisterOperandOfType(rType); /* result operand 2 */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 1 */
    VectorRegSpec *vecSpec2 = GetMemoryPool()->New<VectorRegSpec>(rType); /* vector operand 2 */

    VectorInsn &vInsn1 = GetInsnBuilder()->BuildVectorInsn(MOP_vzip1vvv, AArch64CG::kMd[MOP_vzip1vvv]);
    vInsn1.AddOpndChain(*res1).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn1.PushRegSpecEntry(vecSpecDest);
    vInsn1.PushRegSpecEntry(vecSpec1);
    vInsn1.PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn1);

    VectorInsn &vInsn2 = GetInsnBuilder()->BuildVectorInsn(MOP_vzip2vvv, AArch64CG::kMd[MOP_vzip2vvv]);
    vInsn2.AddOpndChain(*res2).AddOpndChain(*o1).AddOpndChain(*o2);
    vInsn2.PushRegSpecEntry(vecSpecDest);
    vInsn2.PushRegSpecEntry(vecSpec1);
    vInsn2.PushRegSpecEntry(vecSpec2);
    GetCurBB()->AppendInsn(vInsn2);

    if (GetPrimTypeSize(rType) <= k16ByteSize) {
        Operand *preg1 = &GetOrCreatePhysicalRegisterOperand(V0, k64BitSize, kRegTyFloat);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xvmovd, *preg1, *res1));
        Operand *preg2 = &GetOrCreatePhysicalRegisterOperand(V1, k64BitSize, kRegTyFloat);
        GetCurBB()->AppendInsn(GetInsnBuilder()->BuildInsn(MOP_xvmovd, *preg2, *res2));
    }
}

RegOperand *AArch64CGFunc::SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow)
{
    RegOperand *res = &CreateRegisterOperandOfType(rType); /* result operand */
    VectorRegSpec *vecSpecDest = GetMemoryPool()->New<VectorRegSpec>(rType);
    VectorRegSpec *vecSpec1 = GetMemoryPool()->New<VectorRegSpec>(otyp); /* vector operand */

    MOperator mOp;
    if (isLow) {
        mOp = IsPrimitiveUnSignedVector(rType) ? MOP_vuxtlvu : MOP_vsxtlvu;
    } else {
        mOp = IsPrimitiveUnSignedVector(rType) ? MOP_vuxtl2vv : MOP_vsxtl2vv;
    }
    VectorInsn &vInsn = GetInsnBuilder()->BuildVectorInsn(mOp, AArch64CG::kMd[mOp]);
    vInsn.AddOpndChain(*res).AddOpndChain(*o1);
    vInsn.PushRegSpecEntry(vecSpecDest);
    vInsn.PushRegSpecEntry(vecSpec1);
    GetCurBB()->AppendInsn(vInsn);
    return res;
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

AArch64CGFunc::SplittedInt128 AArch64CGFunc::SplitInt128(Operand &opnd)
{
    DEBUG_ASSERT(opnd.IsRegister(), "expected register opnd");
    auto vecTy = PTY_v2u64;
    auto scTy = PTY_u64;
    Operand &low = *SelectVectorGetElement(scTy, &opnd, vecTy, 0);
    Operand &high = *SelectVectorGetElement(scTy, &opnd, vecTy, 1);
    return {low, high};
}

RegOperand &AArch64CGFunc::CombineInt128(const SplittedInt128 parts)
{
    RegOperand &resOpnd = CreateRegisterOperandOfType(PTY_v2u64);
    CombineInt128(resOpnd, parts);
    return resOpnd;
}

void AArch64CGFunc::CombineInt128(Operand &resOpnd, const SplittedInt128 parts)
{
    auto vecTy = PTY_v2u64;
    auto scTy = PTY_u64;
    auto *tmpOpnd = SelectVectorFromScalar(vecTy, &parts.low, scTy);
    SelectCopy(resOpnd, vecTy, *SelectVectorSetElement(&parts.high, scTy, tmpOpnd, vecTy, 1), vecTy);
}

void AArch64CGFunc::SelectParmListForInt128(Operand &opnd, ListOperand &srcOpnds, const CCLocInfo &ploc,
                                            bool isSpecialArg, std::vector<RegMapForPhyRegCpy> &regMapForTmpBB)
{
    DEBUG_ASSERT(ploc.reg0 != kRinvalid && ploc.reg1 != kRinvalid, "");

    auto splitOpnd = SplitInt128(opnd);
    auto reg0 = static_cast<AArch64reg>(ploc.reg0);
    auto reg1 = static_cast<AArch64reg>(ploc.reg1);
    RegOperand &low = GetOrCreatePhysicalRegisterOperand(reg0, k64BitSize, kRegTyInt);
    RegOperand &high = GetOrCreatePhysicalRegisterOperand(reg1, k64BitSize, kRegTyInt);

    auto cpyTy = PTY_u64;
    if (isSpecialArg) {
        regMapForTmpBB.emplace_back(RegMapForPhyRegCpy(&low, cpyTy, static_cast<RegOperand *>(&splitOpnd.low), cpyTy));
        regMapForTmpBB.emplace_back(
            RegMapForPhyRegCpy(&high, cpyTy, static_cast<RegOperand *>(&splitOpnd.high), cpyTy));
    } else {
        SelectCopy(low, cpyTy, splitOpnd.low, cpyTy);
        SelectCopy(high, cpyTy, splitOpnd.high, cpyTy);
    }

    srcOpnds.PushOpnd(low);
    srcOpnds.PushOpnd(high);
}
} /* namespace maplebe */
