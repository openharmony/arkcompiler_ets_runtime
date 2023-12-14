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

#include "cast_opt.h"
#include "irmap.h"
#include "mir_builder.h"
#include "constantfold.h"

namespace maple {
// This is not safe because handlefunction may ignore necessary implicit intTrunc, so we skip it now.
static constexpr bool mapleSimplifyZextU32 = false;

CastKind GetCastKindByTwoType(PrimType fromType, PrimType toType)
{
    // This is a workaround, we don't optimize `cvt u1 xx <expr>` because it will be converted to
    // `ne u1 xx (<expr>, constval xx 0)`. There is no `cvt u1 xx <expr>` in the future.
    if (toType == PTY_u1 && fromType != PTY_u1) {
        return CAST_unknown;
    }
    const uint32 fromTypeBitSize = GetPrimTypeActualBitSize(fromType);
    const uint32 toTypeBitSize = GetPrimTypeActualBitSize(toType);
    // Both integer, ptr/ref/a64/u64... are also integer
    if (IsPrimitiveInteger(fromType) && IsPrimitiveInteger(toType)) {
        if (toTypeBitSize == fromTypeBitSize) {
            return CAST_retype;
        } else if (toTypeBitSize < fromTypeBitSize) {
            return CAST_intTrunc;
        } else {
            return IsSignedInteger(fromType) ? CAST_sext : CAST_zext;
        }
    }
    // Both fp
    if (IsPrimitiveFloat(fromType) && IsPrimitiveFloat(toType)) {
        if (toTypeBitSize == fromTypeBitSize) {
            return CAST_retype;
        } else if (toTypeBitSize < fromTypeBitSize) {
            return CAST_fpTrunc;
        } else {
            return CAST_fpExt;
        }
    }
    // int2fp
    if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType)) {
        return CAST_int2fp;
    }
    // fp2int
    if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
        return CAST_fp2int;
    }
    return CAST_unknown;
}

BaseNode *MapleCastOpt::CreateMapleExprByCastKind(MIRBuilder &mirBuilder, CastKind castKind, PrimType srcType,
                                                  PrimType dstType, BaseNode *opnd, TyIdx dstTyIdx)
{
    if (castKind == CAST_zext) {
        return mirBuilder.CreateExprExtractbits(OP_zext, dstType, 0, GetPrimTypeActualBitSize(srcType), opnd);
    } else if (castKind == CAST_sext) {
        return mirBuilder.CreateExprExtractbits(OP_sext, dstType, 0, GetPrimTypeActualBitSize(srcType), opnd);
    } else if (castKind == CAST_retype && srcType == opnd->GetPrimType()) {
        // If srcType is different from opnd->primType, we should create cvt instead of retype.
        // Because CGFunc::SelectRetype always use opnd->primType as srcType.
        CHECK_FATAL(dstTyIdx != 0u, "must specify valid tyIdx for retype");
        MIRType *dstMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(dstTyIdx);
        return mirBuilder.CreateExprRetype(*dstMIRType, srcType, opnd);
    } else {
        return mirBuilder.CreateExprTypeCvt(OP_cvt, dstType, srcType, *opnd);
    }
}

// This interface is conservative, which means that some op are explicit type cast but
// the interface returns false.
bool CastOpt::IsExplicitCastOp(Opcode op)
{
    if (op == OP_retype || op == OP_cvt || op == OP_zext || op == OP_sext) {
        return true;
    }
    return false;
}

// This interface is conservative, which means that some op are implicit type cast but
// the interface returns false.
bool CastOpt::IsImplicitCastOp(Opcode op)
{
    if (op == OP_iread || op == OP_regread || op == OP_dread) {
        return true;
    }
    return false;
}

bool CastOpt::IsCompareOp(Opcode op)
{
    if (op == OP_eq || op == OP_ne || op == OP_ge || op == OP_gt || op == OP_le || op == OP_lt) {
        return true;
    }
    return false;
}

// If the computed castInfo.kind is CAST_unknown, the computed castInfo is invalid
// castInfo.expr should be valid
// input: castInfo.expr
// output: castInfo.kind, castInfo.srcType, castInfo.dstType
template <typename T>
void CastOpt::DoComputeCastInfo(CastInfo<T> &castInfo, bool isMeExpr)
{
    Opcode op = castInfo.GetOp();
    PrimType dstType = castInfo.GetPrimType();
    PrimType srcType = PTY_begin;
    CastKind castKind = CAST_unknown;
    switch (op) {
        case OP_zext:
        case OP_sext: {
            size_t sizeBit = castInfo.GetBitsSize();
            // The code can be improved
            // exclude: sext ixx 1 <expr> because there is no i1 type
            if (sizeBit == 1 && op == OP_sext) {
                break;
            }
            if (sizeBit == k1BitSize || sizeBit == k8BitSize || sizeBit == k16BitSize || sizeBit == k32BitSize ||
                sizeBit == k64BitSize) {
                srcType = GetIntegerPrimTypeBySizeAndSign(sizeBit, op == OP_sext);
                if (srcType == PTY_begin) {
                    break;  // invalid integer type
                }
                castKind = (op == OP_sext ? CAST_sext : CAST_zext);
            }
            break;
        }
        case OP_retype: {
            // retype's opndType is invalid, we use opnd's primType
            srcType = castInfo.GetOpndType();
            if (GetPrimTypeActualBitSize(dstType) != GetPrimTypeActualBitSize(srcType)) {
                // Example: retype u8 <u8> (iread i32 <* i8> 0 ...)
                // In the above example, dstType is u8, but we get srcType i32 from iread.
                // We won't optimize such retype unless we can get real opnd type for all kinds of opnds.
                // We will improve it if possible.
                break;
            }
            castKind = CAST_retype;
            break;
        }
        case OP_cvt: {
            srcType = castInfo.GetOpndType();
            if (srcType == PTY_u1 && dstType != PTY_u1) {
                srcType = PTY_u8;  // From the codegen view, `cvt xx u1` is always same as `cvt xx u8`
            }
            castKind = GetCastKindByTwoType(srcType, dstType);
            break;
        }
        case OP_iread:
        case OP_dread:
        case OP_regread: {
            if (op == OP_dread && isMeExpr) {
                break;
            }
            srcType = castInfo.GetOpndType();
            // Only consider regread/iread/dread with implicit integer extension
            if (IsPrimitiveInteger(srcType) && IsPrimitiveInteger(dstType) &&
                GetPrimTypeActualBitSize(srcType) < GetPrimTypeActualBitSize(dstType)) {
                castKind = (IsSignedInteger(srcType) ? CAST_sext : CAST_zext);
            }
            break;
        }
        default:
            break;
    }
    castInfo.kind = castKind;
    castInfo.srcType = srcType;
    castInfo.dstType = dstType;
}

static constexpr auto kNumCastKinds = static_cast<uint32>(CAST_unknown);
static const uint8 castMatrix[kNumCastKinds][kNumCastKinds] = {
    // i        i  f  f     r  -+
    // t        n  p  t  f  e   |
    // r  z  s  t  2  r  p  t   +- secondCastKind
    // u  e  e  2  i  u  e  y   |
    // n  x  x  f  n  n  x  p   |
    // c  t  t  p  t  c  t  e  -+
    {1, 0, 0, 0, 99, 99, 99, 3},   // intTrunc   -+
    {8, 9, 9, 10, 99, 99, 99, 3},  // zext        |
    {8, 0, 9, 0, 99, 99, 99, 3},   // sext        |
    {99, 99, 99, 99, 0, 0, 0, 4},  // int2fp      |
    {0, 0, 0, 0, 99, 99, 99, 0},   // fp2int      +- firstCastKind
    {99, 99, 99, 99, 0, 0, 0, 4},  // fpTrunc     |
    {99, 99, 99, 99, 2, 8, 2, 4},  // fpExt       |
    {5, 7, 7, 11, 6, 6, 6, 1},     // retype     -+
};

// This function determines whether to eliminate a cast pair according to castMatrix
// Input is a cast pair like this:
//   secondCastKind dstType midType2 (firstCastKind midType1 srcType)
// If the function returns a valid resultCastKind, the cast pair can be optimized to:
//   resultCastKind dstType srcType
// If the cast pair can NOT be eliminated, -1 will be returned.
// ATTENTION: This function may modify srcType
int CastOpt::IsEliminableCastPair(CastKind firstCastKind, CastKind secondCastKind, PrimType dstType, PrimType midType2,
                                  PrimType midType1, PrimType &srcType)
{
    int castCase = castMatrix[firstCastKind][secondCastKind];
    uint32 srcSize = GetPrimTypeActualBitSize(srcType);
    uint32 midSize1 = GetPrimTypeActualBitSize(midType1);
    uint32 midSize2 = GetPrimTypeActualBitSize(midType2);
    uint32 dstSize = GetPrimTypeActualBitSize(dstType);

    switch (castCase) {
        case 0: {
            // Not allowed
            return -1;
        }
        case 1: { // 1 st case in castMatrix, see the comments above
            // first intTrunc, then intTrunc
            // Example: cvt u16 u32 (cvt u32 u64)  ==>  cvt u16 u64
            // first retype, then retype
            // Example: retype i64 u64 (retype u64 ptr)  ==>  retype i64 ptr
            return firstCastKind;
        }
        case 2: { // 2 nd case in castMatrix, see the comments above
            // first fpExt, then fpExt
            // Example: cvt f128 f64 (cvt f64 f32)  ==>  cvt f128 f32
            // first fpExt, then fp2int
            // Example: cvt i64 f64 (cvt f64 f32)  ==>  cvt i64 f32
            return secondCastKind;
        }
        case 3: { // 3 rd case in castMatrix, see the comments above
            if (IsPrimitiveInteger(dstType)) {
                return firstCastKind;
            }
            return -1;
        }
        case 4: { // 4 th case in castMatrix, see the comments above
            if (IsPrimitiveFloat(dstType)) {
                return firstCastKind;
            }
            return -1;
        }
        case 5: { // 5 th case in castMatrix, see the comments above
            if (IsPrimitiveInteger(srcType)) {
                return secondCastKind;
            }
            return -1;
        }
        case 6: { // 6 th case in castMatrix, see the comments above
            if (IsPrimitiveFloat(srcType)) {
                return secondCastKind;
            }
            return -1;
        }
        case 7: { // 7 th case in castMatrix, see the comments above
            // first integer retype, then sext/zext
            if (IsPrimitiveInteger(srcType) && dstSize >= midSize1) {
                CHECK_FATAL(srcSize == midSize1, "must be");
                if (midSize2 >= srcSize) {
                    return secondCastKind;
                }
                // Example: zext u64 8 (retype u32 i32)  ==>  zext u64 8
                srcType = midType2;
                return secondCastKind;
            }
            return -1;
        }
        case 8: { // 8 th case in castMatrix, see the comments above
            if (srcSize == dstSize) {
                return CAST_retype;
            } else if (srcSize < dstSize) {
                return firstCastKind;
            } else {
                return secondCastKind;
            }
        }
            // For integer extension pair
        case 9: { // 9 th case in castMatrix, see the comments above
            // first zext, then sext
            // Extreme example: sext i32 16 (zext u64 8)  ==> zext i32 8
            if (firstCastKind != secondCastKind && midSize2 <= midSize1) {
                if (midSize2 > srcSize) {
                    // The first extension works. After the first zext, the most significant bit must be 0, so the
                    // second sext is actually a zext. Example: sext i64 16 (zext u32 8)  ==> zext i64 8
                    return firstCastKind;
                }
                // midSize2 <= srcSize
                // The first extension didn't work
                // Example: sext i64 8 (zext u32 16)  ==> sext i64 8
                // Example: sext i16 8 (zext u32 16)  ==> sext i16 8
                srcType = midType2;
                return secondCastKind;
            }

            // first zext, then zext
            // first sext, then sext
            // Example: sext i32 8 (sext i32 8)  ==>  sext i32 8
            // Example: zext u16 1 (zext u32 8)  ==>  zext u16 1    it's ok
            // midSize2 < srcSize:
            // Example: zext u64 8 (zext u32 16)  ==> zext u64 8
            // Example: sext i64 8 (sext i32 16)  ==> sext i64 8
            // Example: zext i32 1 (zext u32 8)  ==> zext i32 1
            // Wrong example (midSize2 > midSize1): zext u64 32 (zext u16 8)  =[x]=>  zext u64 8
            if (firstCastKind == secondCastKind && midSize2 <= midSize1) {
                if (midSize2 < srcSize) {
                    srcType = midType2;
                }
                return secondCastKind;
            }
            return -1;
        }
        case 10: { // 10 th case in castMatrix, see the comments above
            // first zext, then int2fp
            if (IsSignedInteger(midType2)) {
                return secondCastKind;
            }
            // To improved: consider unsigned
            return -1;
        }
        case 11: { // 11 st case in castMatrix, see the comments above
            // first retype, then int2fp
            if (IsPrimitiveInteger(srcType)) {
                if (IsSignedInteger(srcType) != IsSignedInteger(midType1)) {
                    // If sign diffs, use toType of retype
                    // Example: cvt f64 i64 (retype i64 u64)  ==>  cvt f64 i64
                    srcType = midType1;
                }
                return secondCastKind;
            }
            return -1;
        }
        case 99: { // 99 is this last case in castMatrix, see the comments above
            CHECK_FATAL(false, "invalid cast pair");
        }
        default: {
            CHECK_FATAL(false, "can not be here, is castMatrix wrong?");
        }
    }
}

void MapleCastOpt::ComputeCastInfo(BaseNodeCastInfo &castInfo)
{
    DoComputeCastInfo(castInfo, false);
}

BaseNode *MapleCastOpt::TransformCvtU1ToNe(MIRBuilder &mirBuilder, const TypeCvtNode *cvtExpr)
{
    PrimType fromType = cvtExpr->FromType();
    auto *fromMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fromType));
    // We use u8 instead of u1 because codegen can't recognize u1
    auto *toMIRType = GlobalTables::GetTypeTable().GetUInt8();
    auto *zero = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *fromMIRType);
    auto *converted = mirBuilder.CreateExprCompare(OP_ne, *toMIRType, *fromMIRType, cvtExpr->Opnd(0),
                                                   mirBuilder.CreateConstval(zero));
    return converted;
}

BaseNode *MapleCastOpt::SimplifyCast(MIRBuilder &mirBuilder, BaseNode *expr)
{
    Opcode op = expr->GetOpCode();
    if (!IsExplicitCastOp(op)) {
        return nullptr;
    }
    auto *opnd = expr->Opnd(0);

    // Convert `cvt u1 xx <expr>` to `ne u1 xx (<expr>, constval xx 0)`
    if (op == OP_cvt && expr->GetPrimType() == PTY_u1 &&
        static_cast<TypeCvtNode *>(expr)->FromType() != PTY_u1) {  // No need to convert `cvt u1 u1 <expr>`
        return TransformCvtU1ToNe(mirBuilder, static_cast<TypeCvtNode *>(expr));
    }

    // If the opnd is a iread/regread, it's OK because it may be a implicit zext or sext
    Opcode opndOp = opnd->GetOpCode();
    if (!IsExplicitCastOp(opndOp) && !IsImplicitCastOp(opndOp)) {
        // only 1 cast
        // Exmaple: cvt i32 i64 (add i32)  ==>  add i32
        BaseNodeCastInfo castInfo(expr);
        ComputeCastInfo(castInfo);
        return SimplifyCastSingle(mirBuilder, castInfo);
    }
    BaseNodeCastInfo firstCastInfo(opnd);
    ComputeCastInfo(firstCastInfo);
    BaseNodeCastInfo secondCastInfo(expr);
    ComputeCastInfo(secondCastInfo);
    auto *simplified1 = SimplifyCastPair(mirBuilder, firstCastInfo, secondCastInfo);
    BaseNode *simplified2 = nullptr;
    if (simplified1 != nullptr && IsExplicitCastOp(simplified1->GetOpCode())) {
        // Simplify cast further
        secondCastInfo.expr = simplified1;
        ComputeCastInfo(secondCastInfo);
        simplified2 = SimplifyCastSingle(mirBuilder, secondCastInfo);
    }
    if (simplified2 != nullptr) {
        return simplified2;
    }
    if (simplified1 == nullptr) {
        BaseNodeCastInfo castInfo(expr);
        ComputeCastInfo(castInfo);
        return SimplifyCastSingle(mirBuilder, castInfo);
    }
    return simplified1;
}

BaseNode *MapleCastOpt::SimplifyCastSingle(MIRBuilder &mirBuilder, const BaseNodeCastInfo &castInfo)
{
    if (castInfo.IsInvalid()) {
        return nullptr;
    }
    auto *castExpr = static_cast<BaseNode *>(castInfo.expr);
    auto *opnd = castExpr->Opnd(0);
    Opcode op = castExpr->GetOpCode();
    Opcode opndOp = opnd->GetOpCode();
    // cast to integer + compare  ==>  compare
    if (IsPrimitiveInteger(castInfo.dstType) && IsCompareOp(opndOp)) {
        // exclude the following castExpr:
        //   sext xx 1 <expr>
        bool excluded = (op == OP_sext && static_cast<ExtractbitsNode *>(castExpr)->GetBitsSize() == 1);
        if (!excluded) {
            opnd->SetPrimType(castExpr->GetPrimType());
            return opnd;
        }
    }
    // cast + const  ==>  const
    if (castInfo.kind != CAST_retype && opndOp == OP_constval) {
        ConstantFold cf(*theMIRModule);
        MIRConst *cst = cf.FoldTypeCvtMIRConst(*static_cast<ConstvalNode *>(opnd)->GetConstVal(), castInfo.srcType,
                                               castInfo.dstType);
        if (cst != nullptr) {
            return mirBuilder.CreateConstval(cst);
        }
    }
    if (mapleSimplifyZextU32) {
        // zextTo32 + read  ==>  read 32
        if (castInfo.kind == CAST_zext && (opndOp == OP_iread || opndOp == OP_regread || opndOp == OP_dread)) {
            uint32 dstSize = GetPrimTypeActualBitSize(castInfo.dstType);
            if (dstSize == k32BitSize && IsUnsignedInteger(castInfo.dstType) &&
                IsUnsignedInteger(opnd->GetPrimType()) &&
                GetPrimTypeActualBitSize(castInfo.srcType) == GetPrimTypeActualBitSize(opnd->GetPrimType())) {
                opnd->SetPrimType(castInfo.dstType);
                return opnd;
            }
        }
    }
    if (castInfo.dstType == opnd->GetPrimType() &&
        GetPrimTypeActualBitSize(castInfo.srcType) >= GetPrimTypeActualBitSize(opnd->GetPrimType())) {
        return opnd;
    }
    return nullptr;
}

BaseNode *MapleCastOpt::SimplifyCastPair(MIRBuilder &mirBuidler, const BaseNodeCastInfo &firstCastInfo,
                                         const BaseNodeCastInfo &secondCastInfo)
{
    if (firstCastInfo.IsInvalid()) {
        // We can NOT eliminate the first cast, try to simplify the second cast individually
        return SimplifyCastSingle(mirBuidler, secondCastInfo);
    }
    if (secondCastInfo.IsInvalid()) {
        return nullptr;
    }
    PrimType srcType = firstCastInfo.srcType;
    PrimType origSrcType = srcType;
    PrimType midType1 = firstCastInfo.dstType;
    PrimType midType2 = secondCastInfo.srcType;
    PrimType dstType = secondCastInfo.dstType;
    int result = IsEliminableCastPair(firstCastInfo.kind, secondCastInfo.kind, dstType, midType2, midType1, srcType);
    if (result == -1) {
        return SimplifyCastSingle(mirBuidler, secondCastInfo);
    }
    auto resultCastKind = CastKind(result);
    auto *firstCastExpr = static_cast<BaseNode *>(firstCastInfo.expr);
    auto *secondCastExpr = static_cast<BaseNode *>(secondCastInfo.expr);

    // To improved: do more powerful optimization for firstCastImplicit
    bool isFirstCastImplicit = !IsExplicitCastOp(firstCastExpr->GetOpCode());
    if (isFirstCastImplicit) {
        // Wrong example: zext u32 u8 (iread u32 <* u16>)  =[x]=>  iread u32 <* u16>
        // srcType may be modified, we should use origSrcType
        if (resultCastKind != CAST_unknown && dstType == midType1 &&
            GetPrimTypeActualBitSize(midType2) >= GetPrimTypeActualBitSize(origSrcType)) {
            return firstCastExpr;
        } else {
            return nullptr;
        }
    }

    auto *toCastExpr = firstCastExpr->Opnd(0);
    // Example: retype u32 <u32> (dread u32 %x)  ==>  dread u32 %x
    // Example: retype ptr <* <$Foo>> (dread ptr %p)  ==>  dread ptr %p
    if (resultCastKind == CAST_retype && srcType == dstType) {
        if (toCastExpr->GetPrimType() != dstType) {
            // Wrong example: retype i16 i16 (regread i32 %1)  =[x]=> regread i32 %1
            //       instead: ==> cvt i16 i32 (regread i32 %1)
            return mirBuidler.CreateExprTypeCvt(OP_cvt, dstType, toCastExpr->GetPrimType(), *toCastExpr);
        }
        return toCastExpr;
    }

    TyIdx dstTyIdx(0);
    if (resultCastKind == CAST_retype) {
        // result retype is generated from `retype t1 t2 (retype t3 t4)`
        if (secondCastExpr->GetOpCode() == OP_retype) {
            dstTyIdx = static_cast<RetypeNode *>(secondCastExpr)->GetTyIdx();
        } else {
            dstTyIdx = TyIdx(dstType);
        }
    }
    return CreateMapleExprByCastKind(mirBuidler, resultCastKind, srcType, dstType, toCastExpr, dstTyIdx);
}
}  // namespace maple
