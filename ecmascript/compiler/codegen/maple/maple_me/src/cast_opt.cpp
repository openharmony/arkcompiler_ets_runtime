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
}  // namespace maple
