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

#include "lower.h"
#include <string>
#include <cinttypes>
#include <vector>
#include "mir_symbol.h"
#include "mir_function.h"
#include "cg_option.h"
#include "switch_lowerer.h"
#include "intrinsic_op.h"
#include "mir_builder.h"
#include "opcode_info.h"
#include "rt.h"
#include "securec.h"
#include "string_utils.h"

namespace maplebe {

using namespace maple;

#define TARGARM32 0

BaseNode *CGLowerer::LowerIaddrof(const IreadNode &iaddrof)
{
    if (iaddrof.GetFieldID() == 0) {
        return iaddrof.Opnd(0);
    }
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrof.GetTyIdx());
    MIRPtrType *pointerTy = static_cast<MIRPtrType *>(type);
    CHECK_FATAL(pointerTy != nullptr, "LowerIaddrof: expect a pointer type at iaddrof node");
    MIRStructType *structTy =
        static_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerTy->GetPointedTyIdx()));
    CHECK_FATAL(structTy != nullptr, "LowerIaddrof: non-zero fieldID for non-structure");
    int32 offset = beCommon.GetFieldOffset(*structTy, iaddrof.GetFieldID()).first;
    if (offset == 0) {
        return iaddrof.Opnd(0);
    }
    uint32 loweredPtrType = static_cast<uint32>(GetLoweredPtrType());
    MIRIntConst *offsetConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        offset, *GlobalTables::GetTypeTable().GetTypeTable().at(loweredPtrType));
    BaseNode *offsetNode = mirModule.CurFuncCodeMemPool()->New<ConstvalNode>(offsetConst);
    offsetNode->SetPrimType(GetLoweredPtrType());

    BinaryNode *addNode = mirModule.CurFuncCodeMemPool()->New<BinaryNode>(OP_add);
    addNode->SetPrimType(GetLoweredPtrType());
    addNode->SetBOpnd(iaddrof.Opnd(0), 0);
    addNode->SetBOpnd(offsetNode, 1);
    return addNode;
}

StmtNode *CGLowerer::WriteBitField(const std::pair<int32, int32> &byteBitOffsets, const MIRBitFieldType *fieldType,
                                   BaseNode *baseAddr, BaseNode *rhs, BlockNode *block)
{
    auto bitSize = fieldType->GetFieldSize();
    auto primType = fieldType->GetPrimType();
    auto byteOffset = byteBitOffsets.first;
    auto bitOffset = byteBitOffsets.second;
    auto *builder = mirModule.GetMIRBuilder();
    auto *bitField = builder->CreateExprIreadoff(primType, byteOffset, baseAddr);
    auto primTypeBitSize = GetPrimTypeBitSize(primType);
    if ((static_cast<uint32>(bitOffset) + bitSize) <= primTypeBitSize) {
        if (CGOptions::IsBigEndian()) {
            bitOffset =
                (static_cast<int64>(beCommon.GetTypeSize(fieldType->GetTypeIndex()) * kBitsPerByte) - bitOffset) -
                bitSize;
        }
        auto depositBits = builder->CreateExprDepositbits(OP_depositbits, primType, static_cast<uint32>(bitOffset),
                                                          bitSize, bitField, rhs);
        return builder->CreateStmtIassignoff(primType, byteOffset, baseAddr, depositBits);
    }
    // if space not enough in the unit with size of primType, we would make an extra assignment from next bound
    auto bitsRemained = (bitOffset + bitSize) - primTypeBitSize;
    auto bitsExtracted = primTypeBitSize - static_cast<uint32>(bitOffset);
    if (CGOptions::IsBigEndian()) {
        bitOffset = 0;
    }
    auto *depositedLowerBits = builder->CreateExprDepositbits(OP_depositbits, primType, static_cast<uint32>(bitOffset),
                                                              bitsExtracted, bitField, rhs);
    auto *assignedLowerBits = builder->CreateStmtIassignoff(primType, byteOffset, baseAddr, depositedLowerBits);
    block->AddStatement(assignedLowerBits);
    auto *extractedHigherBits =
        builder->CreateExprExtractbits(OP_extractbits, primType, bitsExtracted, bitsRemained, rhs);
    auto *bitFieldRemained =
        builder->CreateExprIreadoff(primType, byteOffset + static_cast<int32>(GetPrimTypeSize(primType)), baseAddr);
    auto *depositedHigherBits = builder->CreateExprDepositbits(OP_depositbits, primType, 0, bitsRemained,
                                                               bitFieldRemained, extractedHigherBits);
    auto *assignedHigherBits = builder->CreateStmtIassignoff(
        primType, byteOffset + static_cast<int32>(GetPrimTypeSize(primType)), baseAddr, depositedHigherBits);
    return assignedHigherBits;
}

BaseNode *CGLowerer::ReadBitField(const std::pair<int32, int32> &byteBitOffsets, const MIRBitFieldType *fieldType,
                                  BaseNode *baseAddr)
{
    auto bitSize = fieldType->GetFieldSize();
    auto primType = fieldType->GetPrimType();
    auto byteOffset = byteBitOffsets.first;
    auto bitOffset = byteBitOffsets.second;
    auto *builder = mirModule.GetMIRBuilder();
    auto *bitField = builder->CreateExprIreadoff(primType, byteOffset, baseAddr);
    auto primTypeBitSize = GetPrimTypeBitSize(primType);
    if ((static_cast<uint32>(bitOffset) + bitSize) <= primTypeBitSize) {
        if (CGOptions::IsBigEndian()) {
            bitOffset =
                (static_cast<int64>(beCommon.GetTypeSize(fieldType->GetTypeIndex()) * kBitsPerByte) - bitOffset) -
                bitSize;
        }
        return builder->CreateExprExtractbits(OP_extractbits, primType, static_cast<uint32>(bitOffset), bitSize,
                                              bitField);
    }
    // if space not enough in the unit with size of primType, the result would be binding of two exprs of load
    auto bitsRemained = (bitOffset + bitSize) - primTypeBitSize;
    if (CGOptions::IsBigEndian()) {
        bitOffset = 0;
    }
    auto *extractedLowerBits = builder->CreateExprExtractbits(OP_extractbits, primType, static_cast<uint32>(bitOffset),
                                                              bitSize - bitsRemained, bitField);
    auto *bitFieldRemained =
        builder->CreateExprIreadoff(primType, byteOffset + static_cast<int32>(GetPrimTypeSize(primType)), baseAddr);
    auto *result = builder->CreateExprDepositbits(OP_depositbits, primType, bitSize - bitsRemained, bitsRemained,
                                                  extractedLowerBits, bitFieldRemained);
    return result;
}

BaseNode *CGLowerer::LowerDreadBitfield(DreadNode &dread)
{
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "CurFunction should not be nullptr");
    auto *symbol = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dread.GetStIdx());
    DEBUG_ASSERT(symbol != nullptr, "symbol should not be nullptr");
    auto *structTy = static_cast<MIRStructType *>(symbol->GetType());
    auto fTyIdx = structTy->GetFieldTyIdx(dread.GetFieldID());
    auto *fType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fTyIdx));
    if (fType->GetKind() != kTypeBitField) {
        return &dread;
    }
    auto *builder = mirModule.GetMIRBuilder();
    auto *baseAddr = builder->CreateExprAddrof(0, dread.GetStIdx());
    auto byteBitOffsets = beCommon.GetFieldOffset(*structTy, dread.GetFieldID());
    return ReadBitField(byteBitOffsets, static_cast<MIRBitFieldType *>(fType), baseAddr);
}

BaseNode *CGLowerer::LowerIreadBitfield(IreadNode &iread)
{
    uint32 index = iread.GetTyIdx();
    MIRPtrType *pointerTy = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(index));
    MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerTy->GetPointedTyIdx());
    /* Here pointed type can be Struct or JArray */
    MIRStructType *structTy = nullptr;
    if (pointedTy->GetKind() != kTypeJArray) {
        structTy = static_cast<MIRStructType *>(pointedTy);
    } else {
        structTy = static_cast<MIRJarrayType *>(pointedTy)->GetParentType();
    }
    TyIdx fTyIdx = structTy->GetFieldTyIdx(iread.GetFieldID());
    MIRType *fType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fTyIdx));
    if (fType->GetKind() != kTypeBitField) {
        return &iread;
    }
    auto byteBitOffsets = beCommon.GetFieldOffset(*structTy, iread.GetFieldID());
    return ReadBitField(byteBitOffsets, static_cast<MIRBitFieldType *>(fType), iread.Opnd(0));
}

// input node must be cvt, retype, zext or sext
BaseNode *CGLowerer::LowerCastExpr(BaseNode &expr)
{
    return &expr;
}

#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
BlockNode *CGLowerer::LowerReturnStructUsingFakeParm(NaryStmtNode &retNode)
{
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    for (size_t i = 0; i < retNode.GetNopndSize(); ++i) {
        retNode.SetOpnd(LowerExpr(retNode, *retNode.GetNopndAt(i), *blk), i);
    }
    BaseNode *opnd0 = retNode.Opnd(0);
    if (!(opnd0 && opnd0->GetPrimType() == PTY_agg)) {
        /* It is possible function never returns and have a dummy return const instead of a struct. */
        maple::LogInfo::MapleLogger(kLlWarn) << "return struct should have a kid" << std::endl;
    }

    MIRFunction *curFunc = GetCurrentFunc();
    MIRSymbol *retSt = curFunc->GetFormal(0);
    MIRPtrType *retTy = static_cast<MIRPtrType *>(retSt->GetType());
    IassignNode *iassign = mirModule.CurFuncCodeMemPool()->New<IassignNode>();
    iassign->SetTyIdx(retTy->GetTypeIndex());
    DEBUG_ASSERT(opnd0 != nullptr, "opnd0 should not be nullptr");
    iassign->SetFieldID(0);
    iassign->SetRHS(opnd0);
    if (retSt->IsPreg()) {
        RegreadNode *regNode = mirModule.GetMIRBuilder()->CreateExprRegread(
            GetLoweredPtrType(), curFunc->GetPregTab()->GetPregIdxFromPregno(retSt->GetPreg()->GetPregNo()));
        iassign->SetOpnd(regNode, 0);
    } else {
        AddrofNode *dreadNode = mirModule.CurFuncCodeMemPool()->New<AddrofNode>(OP_dread);
        dreadNode->SetPrimType(GetLoweredPtrType());
        dreadNode->SetStIdx(retSt->GetStIdx());
        iassign->SetOpnd(dreadNode, 0);
    }
    blk->AddStatement(iassign);
    retNode.GetNopnd().clear();
    retNode.SetNumOpnds(0);
    blk->AddStatement(&retNode);
    return blk;
}

#endif /* TARGARM32 || TARGAARCH64 || TARGX86_64 */

BlockNode *CGLowerer::LowerReturn(NaryStmtNode &retNode)
{
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    if (retNode.NumOpnds() != 0) {
        BaseNode *expr = retNode.Opnd(0);
        Opcode opr = expr->GetOpCode();
        if (opr == OP_dread) {
            AddrofNode *retExpr = static_cast<AddrofNode *>(expr);
            MIRFunction *mirFunc = mirModule.CurFunction();
            MIRSymbol *sym = mirFunc->GetLocalOrGlobalSymbol(retExpr->GetStIdx());
            if (sym->GetAttr(ATTR_localrefvar)) {
                mirFunc->InsertMIRSymbol(sym);
            }
        }
    }
    for (size_t i = 0; i < retNode.GetNopndSize(); ++i) {
        retNode.SetOpnd(LowerExpr(retNode, *retNode.GetNopndAt(i), *blk), i);
    }
    blk->AddStatement(&retNode);
    return blk;
}

StmtNode *CGLowerer::LowerDassignBitfield(DassignNode &dassign, BlockNode &newBlk)
{
    dassign.SetRHS(LowerExpr(dassign, *dassign.GetRHS(), newBlk));
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "CurFunction should not be nullptr");
    MIRSymbol *symbol = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dassign.GetStIdx());
    MIRStructType *structTy = static_cast<MIRStructType *>(symbol->GetType());
    CHECK_FATAL(structTy != nullptr, "LowerDassignBitfield: non-zero fieldID for non-structure");
    TyIdx fTyIdx = structTy->GetFieldTyIdx(dassign.GetFieldID());
    CHECK_FATAL(fTyIdx != 0u, "LowerDassignBitField: field id out of range for the structure");
    MIRType *fType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fTyIdx));
    if (fType->GetKind() != kTypeBitField) {
        return &dassign;
    }
    auto *builder = mirModule.GetMIRBuilder();
    auto *baseAddr = builder->CreateExprAddrof(0, dassign.GetStIdx());
    auto byteBitOffsets = beCommon.GetFieldOffset(*structTy, dassign.GetFieldID());
    return WriteBitField(byteBitOffsets, static_cast<MIRBitFieldType *>(fType), baseAddr, dassign.GetRHS(), &newBlk);
}

StmtNode *CGLowerer::LowerIassignBitfield(IassignNode &iassign, BlockNode &newBlk)
{
    DEBUG_ASSERT(iassign.Opnd(0) != nullptr, "iassign.Opnd(0) should not be nullptr");
    iassign.SetOpnd(LowerExpr(iassign, *iassign.Opnd(0), newBlk), 0);
    iassign.SetRHS(LowerExpr(iassign, *iassign.GetRHS(), newBlk));

    CHECK_FATAL(iassign.GetTyIdx() < GlobalTables::GetTypeTable().GetTypeTable().size(),
                "LowerIassignBitField: subscript out of range");
    uint32 index = iassign.GetTyIdx();
    MIRPtrType *pointerTy = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(index));
    CHECK_FATAL(pointerTy != nullptr, "LowerIassignBitField: type in iassign should be pointer type");
    MIRType *pointedTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerTy->GetPointedTyIdx());
    /*
     * Here pointed type can be Struct or JArray
     * We should seriously consider make JArray also a Struct type
     */
    MIRStructType *structTy = nullptr;
    if (pointedTy->GetKind() != kTypeJArray) {
        structTy = static_cast<MIRStructType *>(pointedTy);
    } else {
        structTy = static_cast<MIRJarrayType *>(pointedTy)->GetParentType();
    }

    TyIdx fTyIdx = structTy->GetFieldTyIdx(iassign.GetFieldID());
    MIRType *fType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fTyIdx));
    if (fType->GetKind() != kTypeBitField) {
        return &iassign;
    }
    auto byteBitOffsets = beCommon.GetFieldOffset(*structTy, iassign.GetFieldID());
    auto *bitFieldType = static_cast<MIRBitFieldType *>(fType);
    return WriteBitField(byteBitOffsets, bitFieldType, iassign.Opnd(0), iassign.GetRHS(), &newBlk);
}

void CGLowerer::LowerIassign(IassignNode &iassign, BlockNode &newBlk)
{
    StmtNode *newStmt = nullptr;
    if (iassign.GetFieldID() != 0) {
        newStmt = LowerIassignBitfield(iassign, newBlk);
    } else {
        LowerStmt(iassign, newBlk);
        newStmt = &iassign;
    }
    newBlk.AddStatement(newStmt);
}

static GStrIdx NewAsmTempStrIdx()
{
    static uint32 strIdxCount = 0;  // to create unique temporary symbol names
    std::string asmTempStr("asm_tempvar");
    asmTempStr += std::to_string(++strIdxCount);
    return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(asmTempStr);
}

void CGLowerer::LowerAsmStmt(AsmNode *asmNode, BlockNode *newBlk)
{
    for (size_t i = 0; i < asmNode->NumOpnds(); i++) {
        BaseNode *opnd = LowerExpr(*asmNode, *asmNode->Opnd(i), *newBlk);
        if (opnd->NumOpnds() == 0) {
            asmNode->SetOpnd(opnd, i);
            continue;
        }
        // introduce a temporary to store the expression tree operand
        TyIdx tyIdxUsed = static_cast<TyIdx>(opnd->GetPrimType());
        if (opnd->op == OP_iread) {
            IreadNode *ireadNode = static_cast<IreadNode *>(opnd);
            tyIdxUsed = ireadNode->GetType()->GetTypeIndex();
        }
        StmtNode *assignNode = nullptr;
        BaseNode *readOpnd = nullptr;
        PrimType type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdxUsed)->GetPrimType();
        if ((type != PTY_agg) && CGOptions::GetInstance().GetOptimizeLevel() >= CGOptions::kLevel2) {
            DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "curFunction should not be nullptr");
            PregIdx pregIdx = mirModule.CurFunction()->GetPregTab()->CreatePreg(type);
            assignNode = mirBuilder->CreateStmtRegassign(type, pregIdx, opnd);
            readOpnd = mirBuilder->CreateExprRegread(type, pregIdx);
        } else {
            MIRSymbol *st = mirModule.GetMIRBuilder()->CreateSymbol(tyIdxUsed, NewAsmTempStrIdx(), kStVar, kScAuto,
                                                                    mirModule.CurFunction(), kScopeLocal);
            assignNode = mirModule.GetMIRBuilder()->CreateStmtDassign(*st, 0, opnd);
            readOpnd = mirBuilder->CreateExprDread(*st);
        }
        newBlk->AddStatement(assignNode);
        asmNode->SetOpnd(readOpnd, i);
    }
    newBlk->AddStatement(asmNode);
}

BaseNode *CGLowerer::NeedRetypeWhenLowerCallAssigned(PrimType pType)
{
    BaseNode *retNode = mirModule.GetMIRBuilder()->CreateExprRegread(pType, -kSregRetval0);
    if (IsPrimitiveInteger(pType) && GetPrimTypeBitSize(pType) <= k32BitSize) {
        auto newPty = IsPrimitiveUnsigned(pType) ? PTY_u64 : PTY_i64;
        retNode = mirModule.GetMIRBuilder()->CreateExprTypeCvt(OP_cvt, newPty, pType, *retNode);
    }
    return retNode;
}

DassignNode *CGLowerer::SaveReturnValueInLocal(StIdx stIdx, uint16 fieldID)
{
    MIRSymbol *var;
    if (stIdx.IsGlobal()) {
        var = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    } else {
        DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
        var = GetCurrentFunc()->GetSymbolTabItem(stIdx.Idx());
    }
    CHECK_FATAL(var != nullptr, "var should not be nullptr");
    PrimType pType;
    if (var->GetAttr(ATTR_oneelem_simd)) {
        pType = PTY_f64;
    } else {
        pType = GlobalTables::GetTypeTable().GetTypeTable().at(var->GetTyIdx())->GetPrimType();
    }
    auto *regRead = NeedRetypeWhenLowerCallAssigned(pType);
    return mirModule.GetMIRBuilder()->CreateStmtDassign(*var, fieldID, regRead);
}

/* to lower call (including icall) and intrinsicall statements */
void CGLowerer::LowerCallStmt(StmtNode &stmt, StmtNode *&nextStmt, BlockNode &newBlk, MIRType *retty, bool uselvar,
                              bool isIntrinAssign)
{
    StmtNode *newStmt = nullptr;
    if (stmt.GetOpCode() == OP_intrinsiccall) {
        auto &intrnNode = static_cast<IntrinsiccallNode &>(stmt);
        newStmt = LowerIntrinsiccall(intrnNode, newBlk);
    } else {
        /* We note the function has a user-defined (i.e., not an intrinsic) call. */
        DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
        GetCurrentFunc()->SetHasCall();
        newStmt = &stmt;
    }

    if (newStmt == nullptr) {
        return;
    }

    if (newStmt->GetOpCode() == OP_call || newStmt->GetOpCode() == OP_icall || newStmt->GetOpCode() == OP_icallproto) {
        newStmt = LowerCall(static_cast<CallNode &>(*newStmt), nextStmt, newBlk, retty, uselvar);
    }
    newStmt->SetSrcPos(stmt.GetSrcPos());
    newBlk.AddStatement(newStmt);
}

StmtNode *CGLowerer::GenCallNode(const StmtNode &stmt, PUIdx &funcCalled, CallNode &origCall)
{
    CallNode *newCall = nullptr;
    if (stmt.GetOpCode() == OP_callassigned) {
        newCall = mirModule.GetMIRBuilder()->CreateStmtCall(origCall.GetPUIdx(), origCall.GetNopnd());
    } else if (stmt.GetOpCode() == OP_virtualcallassigned) {
        newCall = mirModule.GetMIRBuilder()->CreateStmtVirtualCall(origCall.GetPUIdx(), origCall.GetNopnd());
    } else if (stmt.GetOpCode() == OP_superclasscallassigned) {
        newCall = mirModule.GetMIRBuilder()->CreateStmtSuperclassCall(origCall.GetPUIdx(), origCall.GetNopnd());
    }
    CHECK_FATAL(newCall != nullptr, "nullptr is not expected");
    newCall->SetDeoptBundleInfo(origCall.GetDeoptBundleInfo());
    newCall->SetSrcPos(stmt.GetSrcPos());
    funcCalled = origCall.GetPUIdx();
    CHECK_FATAL((newCall->GetOpCode() == OP_call || newCall->GetOpCode() == OP_interfacecall),
                "virtual call or super class call are not expected");
    if (newCall->GetOpCode() == OP_interfacecall) {
        std::cerr << "interfacecall found\n";
    }
    newCall->SetStmtAttrs(stmt.GetStmtAttrs());
    return newCall;
}

StmtNode *CGLowerer::GenIntrinsiccallNode(const StmtNode &stmt, PUIdx &funcCalled, bool &handledAtLowerLevel,
                                          IntrinsiccallNode &origCall)
{
    StmtNode *newCall = nullptr;
    handledAtLowerLevel = IsIntrinsicCallHandledAtLowerLevel(origCall.GetIntrinsic());
    if (handledAtLowerLevel) {
        /* If the lower level can handle the intrinsic, just let it pass through. */
        newCall = &origCall;
    } else {
        PUIdx bFunc = GetBuiltinToUse(origCall.GetIntrinsic());
        if (bFunc != kFuncNotFound) {
            newCall = mirModule.GetMIRBuilder()->CreateStmtCall(bFunc, origCall.GetNopnd());
            CHECK_FATAL(newCall->GetOpCode() == OP_call, "intrinsicnode except intrinsiccall is not expected");
        } else {
            if (stmt.GetOpCode() == OP_intrinsiccallassigned) {
                newCall =
                    mirModule.GetMIRBuilder()->CreateStmtIntrinsicCall(origCall.GetIntrinsic(), origCall.GetNopnd());
                CHECK_FATAL(newCall->GetOpCode() == OP_intrinsiccall,
                            "intrinsicnode except intrinsiccall is not expected");
            } else if (stmt.GetOpCode() == OP_xintrinsiccallassigned) {
                newCall =
                    mirModule.GetMIRBuilder()->CreateStmtXintrinsicCall(origCall.GetIntrinsic(), origCall.GetNopnd());
                CHECK_FATAL(newCall->GetOpCode() == OP_intrinsiccall,
                            "intrinsicnode except intrinsiccall is not expected");
            } else {
                newCall = mirModule.GetMIRBuilder()->CreateStmtIntrinsicCall(origCall.GetIntrinsic(),
                                                                             origCall.GetNopnd(), origCall.GetTyIdx());
                CHECK_FATAL(newCall->GetOpCode() == OP_intrinsiccallwithtype,
                            "intrinsicnode except OP_intrinsiccallwithtype is not expected");
            }
        }
        newCall->SetSrcPos(stmt.GetSrcPos());
        funcCalled = bFunc;
    }
    return newCall;
}

StmtNode *CGLowerer::GenIcallNode(PUIdx &funcCalled, IcallNode &origCall)
{
    IcallNode *newCall = nullptr;
    if (origCall.GetOpCode() == OP_icallassigned) {
        newCall = mirModule.GetMIRBuilder()->CreateStmtIcall(origCall.GetNopnd());
    } else {
        newCall = mirModule.GetMIRBuilder()->CreateStmtIcallproto(origCall.GetNopnd(), origCall.GetRetTyIdx());
        newCall->SetRetTyIdx(static_cast<IcallNode &>(origCall).GetRetTyIdx());
    }
    newCall->SetDeoptBundleInfo(origCall.GetDeoptBundleInfo());
    newCall->SetStmtAttrs(origCall.GetStmtAttrs());
    newCall->SetSrcPos(origCall.GetSrcPos());
    CHECK_FATAL(newCall != nullptr, "nullptr is not expected");
    funcCalled = kFuncNotFound;
    return newCall;
}

BlockNode *CGLowerer::GenBlockNode(StmtNode &newCall, const CallReturnVector &p2nRets, const Opcode &opcode,
                                   const PUIdx &funcCalled, bool handledAtLowerLevel, bool uselvar)
{
    BlockNode *blk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    blk->AddStatement(&newCall);
    if (!handledAtLowerLevel) {
        CHECK_FATAL(p2nRets.size() <= 1, "make sure p2nRets size <= 1");
        /* Create DassignStmt to save kSregRetval0. */
        StmtNode *dStmt = nullptr;
        MIRType *retType = nullptr;
        if (p2nRets.size() == 1) {
            MIRSymbol *sym = nullptr;
            StIdx stIdx = p2nRets[0].first;
            if (stIdx.IsGlobal()) {
                sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
            } else {
                DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
                sym = GetCurrentFunc()->GetSymbolTabItem(stIdx.Idx());
            }
            bool sizeIs0 = false;
            if (sym != nullptr) {
                retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
                if (beCommon.GetTypeSize(retType->GetTypeIndex().GetIdx()) == 0) {
                    sizeIs0 = true;
                }
            }
            if (!sizeIs0) {
                RegFieldPair regFieldPair = p2nRets[0].second;
                if (!regFieldPair.IsReg()) {
                    uint16 fieldID = static_cast<uint16>(regFieldPair.GetFieldID());
                    DassignNode *dn = SaveReturnValueInLocal(stIdx, fieldID);
                    CHECK_FATAL(dn->GetFieldID() == 0, "make sure dn's fieldID return 0");
                    LowerDassign(*dn, *blk);
                    CHECK_FATAL(&newCall == blk->GetLast() || newCall.GetNext() == blk->GetLast(), "");
                    dStmt = (&newCall == blk->GetLast()) ? nullptr : blk->GetLast();
                    CHECK_FATAL(newCall.GetNext() == dStmt, "make sure newCall's next equal dStmt");
                } else {
                    PregIdx pregIdx = static_cast<PregIdx>(regFieldPair.GetPregIdx());
                    DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
                    MIRPreg *mirPreg = GetCurrentFunc()->GetPregTab()->PregFromPregIdx(pregIdx);
                    PrimType pType = mirPreg->GetPrimType();
                    RegreadNode *regNode = mirModule.GetMIRBuilder()->CreateExprRegread(pType, -kSregRetval0);
                    RegassignNode *regAssign;

                    regAssign = mirModule.GetMIRBuilder()->CreateStmtRegassign(
                        mirPreg->GetPrimType(), regFieldPair.GetPregIdx(), regNode);
                    blk->AddStatement(regAssign);
                    dStmt = regAssign;
                }
            }
        }
        blk->ResetBlock();
        /* if VerboseCG, insert a comment */
        if (ShouldAddAdditionalComment()) {
            CommentNode *cmnt = mirModule.CurFuncCodeMemPool()->New<CommentNode>(mirModule);
            cmnt->SetComment(kOpcodeInfo.GetName(opcode).c_str());
            if (funcCalled == kFuncNotFound) {
                cmnt->Append(" : unknown");
            } else {
                cmnt->Append(" : ");
                cmnt->Append(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(funcCalled)->GetName());
            }
            blk->AddStatement(cmnt);
        }
        CHECK_FATAL(dStmt == nullptr || dStmt->GetNext() == nullptr, "make sure dStmt or dStmt's next is nullptr");
        LowerCallStmt(newCall, dStmt, *blk, retType, uselvar, opcode == OP_intrinsiccallassigned);
        if (!uselvar && dStmt != nullptr) {
            dStmt->SetSrcPos(newCall.GetSrcPos());
            blk->AddStatement(dStmt);
        }
    }
    return blk;
}

BlockNode *CGLowerer::LowerIntrinsiccallAassignedToAssignStmt(IntrinsiccallNode &intrinsicCall)
{
    auto *builder = mirModule.GetMIRBuilder();
    auto *block = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    auto intrinsicID = intrinsicCall.GetIntrinsic();
    auto &opndVector = intrinsicCall.GetNopnd();
    auto returnPair = intrinsicCall.GetReturnVec().begin();
    auto regFieldPair = returnPair->second;
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "CurFunction should not be nullptr");
    if (regFieldPair.IsReg()) {
        auto regIdx = regFieldPair.GetPregIdx();
        auto primType = mirModule.CurFunction()->GetPregItem(static_cast<PregIdx>(regIdx))->GetPrimType();
        auto intrinsicOp = builder->CreateExprIntrinsicop(intrinsicID, OP_intrinsicop, primType, TyIdx(0), opndVector);
        auto regAssign = builder->CreateStmtRegassign(primType, regIdx, intrinsicOp);
        block->AddStatement(regAssign);
    } else {
        auto fieldID = regFieldPair.GetFieldID();
        auto stIdx = returnPair->first;
        DEBUG_ASSERT(mirModule.CurFunction()->GetLocalOrGlobalSymbol(stIdx) != nullptr, "nullptr check");
        auto *type = mirModule.CurFunction()->GetLocalOrGlobalSymbol(stIdx)->GetType();
        auto intrinsicOp = builder->CreateExprIntrinsicop(intrinsicID, OP_intrinsicop, *type, opndVector);
        auto dAssign = builder->CreateStmtDassign(stIdx, fieldID, intrinsicOp);
        block->AddStatement(dAssign);
    }
    return LowerBlock(*block);
}

BlockNode *CGLowerer::LowerCallAssignedStmt(StmtNode &stmt, bool uselvar)
{
    StmtNode *newCall = nullptr;
    CallReturnVector *p2nRets = nullptr;
    PUIdx funcCalled = kFuncNotFound;
    bool handledAtLowerLevel = false;
    switch (stmt.GetOpCode()) {
        case OP_callassigned: {
            auto &origCall = static_cast<CallNode &>(stmt);
            newCall = GenCallNode(stmt, funcCalled, origCall);
            p2nRets = &origCall.GetReturnVec();
            static_cast<CallNode *>(newCall)->SetReturnVec(*p2nRets);
            MIRFunction *curFunc = mirModule.CurFunction();
            curFunc->SetLastFreqMap(newCall->GetStmtID(),
                                    static_cast<uint32>(curFunc->GetFreqFromLastStmt(stmt.GetStmtID())));
            break;
        }
        case OP_intrinsiccallassigned: {
            BlockNode *blockNode = LowerIntrinsiccallToIntrinsicop(stmt);
            if (blockNode) {
                return blockNode;
            }
            IntrinsiccallNode &intrincall = static_cast<IntrinsiccallNode &>(stmt);
            auto intrinsicID = intrincall.GetIntrinsic();
            if (IntrinDesc::intrinTable[intrinsicID].IsAtomic()) {
                return LowerIntrinsiccallAassignedToAssignStmt(intrincall);
            }
            newCall = GenIntrinsiccallNode(stmt, funcCalled, handledAtLowerLevel, intrincall);
            p2nRets = &intrincall.GetReturnVec();
            static_cast<IntrinsiccallNode *>(newCall)->SetReturnVec(*p2nRets);
            break;
        }
        case OP_intrinsiccallwithtypeassigned: {
            BlockNode *blockNode = LowerIntrinsiccallToIntrinsicop(stmt);
            if (blockNode) {
                return blockNode;
            }
            auto &origCall = static_cast<IntrinsiccallNode &>(stmt);
            newCall = GenIntrinsiccallNode(stmt, funcCalled, handledAtLowerLevel, origCall);
            p2nRets = &origCall.GetReturnVec();
            static_cast<IntrinsiccallNode *>(newCall)->SetReturnVec(*p2nRets);
            break;
        }
        case OP_icallprotoassigned:
        case OP_icallassigned: {
            auto &origCall = static_cast<IcallNode &>(stmt);
            newCall = GenIcallNode(funcCalled, origCall);
            p2nRets = &origCall.GetReturnVec();
            static_cast<IcallNode *>(newCall)->SetReturnVec(*p2nRets);
            break;
        }
        default:
            CHECK_FATAL(false, "NIY");
            return nullptr;
    }

    /* transfer srcPosition location info */
    newCall->SetSrcPos(stmt.GetSrcPos());
    return GenBlockNode(*newCall, *p2nRets, stmt.GetOpCode(), funcCalled, handledAtLowerLevel, uselvar);
}

BlockNode *CGLowerer::LowerIntrinsiccallToIntrinsicop(StmtNode &stmt)
{
    IntrinsiccallNode &intrinCall = static_cast<IntrinsiccallNode &>(stmt);
    auto intrinsicID = intrinCall.GetIntrinsic();
    if (IntrinDesc::intrinTable[intrinsicID].IsAtomic()) {
        return LowerIntrinsiccallAassignedToAssignStmt(intrinCall);
    }
    return nullptr;
}

#if TARGAARCH64
static PrimType IsStructElementSame(MIRType *ty)
{
    if (ty->GetKind() == kTypeArray) {
        MIRArrayType *arrtype = static_cast<MIRArrayType *>(ty);
        MIRType *pty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrtype->GetElemTyIdx());
        if (pty->GetKind() == kTypeArray || pty->GetKind() == kTypeStruct) {
            return IsStructElementSame(pty);
        }
        return pty->GetPrimType();
    } else if (ty->GetKind() == kTypeStruct) {
        MIRStructType *sttype = static_cast<MIRStructType *>(ty);
        FieldVector fields = sttype->GetFields();
        PrimType oldtype = PTY_void;
        for (uint32 fcnt = 0; fcnt < fields.size(); ++fcnt) {
            TyIdx fieldtyidx = fields[fcnt].second.first;
            MIRType *fieldty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldtyidx);
            PrimType ptype = IsStructElementSame(fieldty);
            if (oldtype != PTY_void && oldtype != ptype) {
                return PTY_void;
            } else {
                oldtype = ptype;
            }
        }
        return oldtype;
    } else {
        return ty->GetPrimType();
    }
}
#endif

// return true if successfully lowered
bool CGLowerer::LowerStructReturn(BlockNode &newBlk, StmtNode &stmt, bool &lvar)
{
    CallReturnVector *p2nrets = stmt.GetCallReturnVector();
    if (p2nrets->size() == 0) {
        return false;
    }
    CallReturnPair retPair = (*p2nrets)[0];
    if (retPair.second.IsReg()) {
        return false;
    }
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "CurFunction should not be nullptr");
    MIRSymbol *retSym = mirModule.CurFunction()->GetLocalOrGlobalSymbol(retPair.first);
    if (retSym->GetType()->GetPrimType() != PTY_agg) {
        return false;
    }

    if (IsReturnInMemory(*retSym->GetType())) {
        lvar = true;
    } else if (!LowerStructReturnInRegs(newBlk, stmt, *retSym)) {
        return false;
    }
    return true;
}

bool CGLowerer::LowerStructReturnInRegs(BlockNode &newBlk, StmtNode &stmt, const MIRSymbol &retSym)
{
    // lower callassigned -> call
    if (stmt.GetOpCode() == OP_callassigned) {
        auto &callNode = static_cast<CallNode &>(stmt);
        for (size_t i = 0; i < callNode.GetNopndSize(); ++i) {
            auto *newOpnd = LowerExpr(callNode, *callNode.GetNopndAt(i), newBlk);
            callNode.SetOpnd(newOpnd, i);
        }
        auto *callStmt = mirModule.GetMIRBuilder()->CreateStmtCall(callNode.GetPUIdx(), callNode.GetNopnd());
        callStmt->SetSrcPos(callNode.GetSrcPos());
        newBlk.AddStatement(callStmt);
    } else if (stmt.GetOpCode() == OP_icallassigned || stmt.GetOpCode() == OP_icallprotoassigned) {
        auto &icallNode = static_cast<IcallNode &>(stmt);
        for (size_t i = 0; i < icallNode.GetNopndSize(); ++i) {
            auto *newOpnd = LowerExpr(icallNode, *icallNode.GetNopndAt(i), newBlk);
            icallNode.SetOpnd(newOpnd, i);
        }
        IcallNode *icallStmt = nullptr;
        if (stmt.GetOpCode() == OP_icallassigned) {
            icallStmt = mirModule.GetMIRBuilder()->CreateStmtIcall(icallNode.GetNopnd());
        } else {
            icallStmt = mirModule.GetMIRBuilder()->CreateStmtIcallproto(icallNode.GetNopnd(), icallNode.GetRetTyIdx());
        }
        icallStmt->SetSrcPos(icallNode.GetSrcPos());
        newBlk.AddStatement(icallStmt);
    } else {
        return false;
    }

    if (Triple::GetTriple().IsAarch64BeOrLe()) {
#if TARGAARCH64
        PrimType primType = PTY_begin;
        size_t elemNum = 0;
        if (IsHomogeneousAggregates(*retSym.GetType(), primType, elemNum)) {
            LowerStructReturnInFpRegs(newBlk, stmt, retSym, primType, elemNum);
        } else {
            LowerStructReturnInGpRegs(newBlk, stmt, retSym);
        }
#endif
    } else {
        LowerStructReturnInGpRegs(newBlk, stmt, retSym);
    }
    return true;
}

// struct passed in gpregs, lowered into
//  call &foo
//  regassign u64 %1 (regread u64 %%retval0)
//  regassign ptr %2 (addrof ptr $s)
//  iassign <* u64> 0 (regread ptr %2, regread u64 %1)
void CGLowerer::LowerStructReturnInGpRegs(BlockNode &newBlk, const StmtNode &stmt, const MIRSymbol &symbol)
{
    auto size = static_cast<uint32>(symbol.GetType()->GetSize());
    if (size == 0) {
        return;
    }
    // save retval0, retval1
    PregIdx pIdx1R = 0;
    PregIdx pIdx2R = 0;
    DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
    auto genRetvalSave = [this, &newBlk](PregIdx &pIdx, SpecialReg sreg) {
        auto *regreadNode = mirBuilder->CreateExprRegread(PTY_u64, -sreg);
        pIdx = GetCurrentFunc()->GetPregTab()->CreatePreg(PTY_u64);
        auto *aStmt = mirBuilder->CreateStmtRegassign(PTY_u64, pIdx, regreadNode);
        newBlk.AddStatement(aStmt);
    };
    genRetvalSave(pIdx1R, kSregRetval0);
    if (size > k8ByteSize) {
        genRetvalSave(pIdx2R, kSregRetval1);
    }
    // save &s
    BaseNode *regAddr = mirBuilder->CreateExprAddrof(0, symbol);
    LowerTypePtr(*regAddr);
    PregIdx pIdxL = GetCurrentFunc()->GetPregTab()->CreatePreg(GetLoweredPtrType());
    auto *aStmt = mirBuilder->CreateStmtRegassign(PTY_a64, pIdxL, regAddr);
    newBlk.AddStatement(aStmt);

    // str retval to &s
    for (uint32 curSize = 0; curSize < size;) {
        // calc addr
        BaseNode *addrNode = mirBuilder->CreateExprRegread(GetLoweredPtrType(), pIdxL);
        if (curSize != 0) {
            MIRType *addrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetLoweredPtrType());
            addrNode =
                mirBuilder->CreateExprBinary(OP_add, *addrType, addrNode, mirBuilder->CreateIntConst(curSize, PTY_i32));
        }

        PregIdx pIdxR = (curSize < k8ByteSize) ? pIdx1R : pIdx2R;
        uint32 strSize = size - curSize;
        // gen str retval to &s + offset
        auto genStrRetval2Memory = [this, &newBlk, &addrNode, &curSize, &pIdxR](PrimType primType) {
            uint32 shiftSize = (curSize * kBitsPerByte) % k64BitSize;
            if (CGOptions::IsBigEndian()) {
                shiftSize = k64BitSize - GetPrimTypeBitSize(primType) + shiftSize;
            }
            BaseNode *regreadExp = mirBuilder->CreateExprRegread(PTY_u64, pIdxR);
            if (shiftSize != 0) {
                MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_u64);
                regreadExp = mirBuilder->CreateExprBinary(OP_lshr, *type, regreadExp,
                                                          mirBuilder->CreateIntConst(shiftSize, PTY_i32));
            }
            auto *pointedType = GlobalTables::GetTypeTable().GetPrimType(primType);
            auto *iassignStmt = mirBuilder->CreateStmtIassign(*beCommon.BeGetOrCreatePointerType(*pointedType), 0,
                                                              addrNode, regreadExp);
            newBlk.AddStatement(iassignStmt);
            curSize += GetPrimTypeSize(primType);
        };
        if (strSize >= k8ByteSize) {
            genStrRetval2Memory(PTY_u64);
        } else if (strSize >= k4ByteSize) {
            genStrRetval2Memory(PTY_u32);
        } else if (strSize >= k2ByteSize) {
            genStrRetval2Memory(PTY_u16);
        } else {
            genStrRetval2Memory(PTY_u8);
        }
    }
}

// struct passed in fpregs, lowered into
//  call &foo
//  regassign f64 %1 (regread f64 %%retval0)
//  regassign ptr %2 (addrof ptr $s)
//  iassign <* f64> 0 (regread ptr %2, regread f64 %1)
void CGLowerer::LowerStructReturnInFpRegs(BlockNode &newBlk, const StmtNode &stmt, const MIRSymbol &symbol,
                                          PrimType primType, size_t elemNum)
{
    // save retvals
    static constexpr std::array sregs = {kSregRetval0, kSregRetval1, kSregRetval2, kSregRetval3};
    std::vector<PregIdx> pIdxs(sregs.size(), 0);
    DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
    for (uint32 i = 0; i < elemNum; ++i) {
        auto *regreadNode = mirBuilder->CreateExprRegread(primType, -sregs[i]);
        pIdxs[i] = GetCurrentFunc()->GetPregTab()->CreatePreg(primType);
        auto *aStmt = mirBuilder->CreateStmtRegassign(primType, pIdxs[i], regreadNode);
        newBlk.AddStatement(aStmt);
    }

    // save &s
    BaseNode *regAddr = mirBuilder->CreateExprAddrof(0, symbol);
    LowerTypePtr(*regAddr);
    PregIdx pIdxL = GetCurrentFunc()->GetPregTab()->CreatePreg(GetLoweredPtrType());
    auto *aStmt = mirBuilder->CreateStmtRegassign(PTY_a64, pIdxL, regAddr);
    newBlk.AddStatement(aStmt);

    // str retvals to &s
    for (uint32 i = 0; i < elemNum; ++i) {
        uint32 offsetSize = i * GetPrimTypeSize(primType);
        BaseNode *addrNode = mirBuilder->CreateExprRegread(GetLoweredPtrType(), pIdxL);
        // addr add offset
        if (offsetSize != 0) {
            MIRType *addrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetLoweredPtrType());
            addrNode = mirBuilder->CreateExprBinary(OP_add, *addrType, addrNode,
                                                    mirBuilder->CreateIntConst(offsetSize, PTY_i32));
        }
        // gen iassigen to addr
        auto *pointedType = GlobalTables::GetTypeTable().GetPrimType(primType);
        auto *iassignStmt = mirBuilder->CreateStmtIassign(*beCommon.BeGetOrCreatePointerType(*pointedType), 0, addrNode,
                                                          mirBuilder->CreateExprRegread(PTY_u64, pIdxs[i]));
        newBlk.AddStatement(iassignStmt);
    }
}

void CGLowerer::LowerStmt(StmtNode &stmt, BlockNode &newBlk)
{
    for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        DEBUG_ASSERT(stmt.Opnd(i) != nullptr, "null ptr check");
        stmt.SetOpnd(LowerExpr(stmt, *stmt.Opnd(i), newBlk), i);
    }
}

void CGLowerer::LowerSwitchOpnd(StmtNode &stmt, BlockNode &newBlk)
{
    BaseNode *opnd = LowerExpr(stmt, *stmt.Opnd(0), newBlk);
    if (CGOptions::GetInstance().GetOptimizeLevel() >= CGOptions::kLevel2 && opnd->GetOpCode() != OP_regread) {
        PrimType ptyp = stmt.Opnd(0)->GetPrimType();
        DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
        PregIdx pIdx = GetCurrentFunc()->GetPregTab()->CreatePreg(ptyp);
        RegassignNode *regAss = mirBuilder->CreateStmtRegassign(ptyp, pIdx, opnd);
        newBlk.AddStatement(regAss);
        GetCurrentFunc()->SetLastFreqMap(regAss->GetStmtID(),
                                         static_cast<uint32>(GetCurrentFunc()->GetFreqFromLastStmt(stmt.GetStmtID())));
        stmt.SetOpnd(mirBuilder->CreateExprRegread(ptyp, pIdx), 0);
    } else {
        stmt.SetOpnd(LowerExpr(stmt, *stmt.Opnd(0), newBlk), 0);
    }
}

BlockNode *CGLowerer::LowerBlock(BlockNode &block)
{
    BlockNode *newBlk = mirModule.CurFuncCodeMemPool()->New<BlockNode>();
    BlockNode *tmpBlockNode = nullptr;
    std::vector<StmtNode *> abortNode;
    if (block.GetFirst() == nullptr) {
        return newBlk;
    }

    StmtNode *nextStmt = block.GetFirst();
    do {
        StmtNode *stmt = nextStmt;
        nextStmt = stmt->GetNext();
        stmt->SetNext(nullptr);
        currentBlock = newBlk;

        switch (stmt->GetOpCode()) {
            case OP_switch: {
                LowerSwitchOpnd(*stmt, *newBlk);
                auto switchMp = std::make_unique<ThreadLocalMemPool>(memPoolCtrler, "switchlowere");
                MapleAllocator switchAllocator(switchMp.get());
                SwitchLowerer switchLowerer(mirModule, static_cast<SwitchNode &>(*stmt), switchAllocator);
                BlockNode *blk = switchLowerer.LowerSwitch();
                if (blk->GetFirst() != nullptr) {
                    newBlk->AppendStatementsFromBlock(*blk);
                }
                needBranchCleanup = true;
                break;
            }
            case OP_block:
                tmpBlockNode = LowerBlock(static_cast<BlockNode &>(*stmt));
                CHECK_FATAL(tmpBlockNode != nullptr, "nullptr is not expected");
                newBlk->AppendStatementsFromBlock(*tmpBlockNode);
                break;
            case OP_dassign: {
                LowerDassign(static_cast<DassignNode &>(*stmt), *newBlk);
                break;
            }
            case OP_regassign: {
                LowerRegassign(static_cast<RegassignNode &>(*stmt), *newBlk);
                break;
            }
            case OP_iassign: {
                LowerIassign(static_cast<IassignNode &>(*stmt), *newBlk);
                break;
            }
            case OP_callassigned:
            case OP_icallassigned:
            case OP_icallprotoassigned: {
                // pass the addr of lvar if this is a struct call assignment
                bool lvar = false;
                // nextStmt could be changed by the call to LowerStructReturn
                if (!LowerStructReturn(*newBlk, *stmt, lvar)) {
                    newBlk->AppendStatementsFromBlock(*LowerCallAssignedStmt(*stmt, lvar));
                }
                break;
            }
            case OP_virtualcallassigned:
            case OP_superclasscallassigned:
            case OP_intrinsiccallassigned:
            case OP_xintrinsiccallassigned:
            case OP_intrinsiccallwithtypeassigned:
                newBlk->AppendStatementsFromBlock(*LowerCallAssignedStmt(*stmt));
                break;
            case OP_intrinsiccall:
            case OP_call:
            case OP_icall:
            case OP_icallproto:
#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
                // nextStmt could be changed by the call to LowerStructReturn
                LowerCallStmt(*stmt, nextStmt, *newBlk);
#else
                LowerStmt(*stmt, *newBlk);
#endif
                break;
            case OP_return: {
#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
                DEBUG_ASSERT(GetCurrentFunc() != nullptr, "GetCurrentFunc should not be nullptr");
                if (GetCurrentFunc()->IsFirstArgReturn() && stmt->NumOpnds() > 0) {
                    newBlk->AppendStatementsFromBlock(
                        *LowerReturnStructUsingFakeParm(static_cast<NaryStmtNode &>(*stmt)));
                } else {
#endif
                    NaryStmtNode *retNode = static_cast<NaryStmtNode *>(stmt);
                    if (retNode->GetNopndSize() == 0) {
                        newBlk->AddStatement(stmt);
                    } else {
                        tmpBlockNode = LowerReturn(*retNode);
                        CHECK_FATAL(tmpBlockNode != nullptr, "nullptr is not expected");
                        newBlk->AppendStatementsFromBlock(*tmpBlockNode);
                    }
#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
                }
#endif
                break;
            }
            case OP_comment:
                newBlk->AddStatement(stmt);
                break;
            case OP_asm: {
                LowerAsmStmt(static_cast<AsmNode *>(stmt), newBlk);
                break;
            }
            default:
                LowerStmt(*stmt, *newBlk);
                newBlk->AddStatement(stmt);
                break;
        }
        CHECK_FATAL(beCommon.GetSizeOfTypeSizeTable() == GlobalTables::GetTypeTable().GetTypeTableSize(), "Error!");
    } while (nextStmt != nullptr);
    for (auto node : abortNode) {
        newBlk->AddStatement(node);
    }
    return newBlk;
}

MIRType *CGLowerer::GetArrayNodeType(BaseNode &baseNode)
{
    MIRType *baseType = nullptr;
    auto curFunc = mirModule.CurFunction();
    DEBUG_ASSERT(curFunc != nullptr, "curFunc should not be nullptr");
    if (baseNode.GetOpCode() == OP_regread) {
        RegreadNode *rrNode = static_cast<RegreadNode *>(&baseNode);
        MIRPreg *pReg = curFunc->GetPregTab()->PregFromPregIdx(rrNode->GetRegIdx());
        if (pReg->IsRef()) {
            baseType = pReg->GetMIRType();
        }
    }
    if (baseNode.GetOpCode() == OP_dread) {
        DreadNode *dreadNode = static_cast<DreadNode *>(&baseNode);
        MIRSymbol *symbol = curFunc->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
        baseType = symbol->GetType();
    }
    MIRType *arrayElemType = nullptr;
    if (baseType != nullptr) {
        MIRType *stType =
            GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<MIRPtrType *>(baseType)->GetPointedTyIdx());
        while (stType->GetKind() == kTypeJArray) {
            MIRJarrayType *baseType1 = static_cast<MIRJarrayType *>(stType);
            MIRType *elemType = baseType1->GetElemType();
            if (elemType->GetKind() == kTypePointer) {
                const TyIdx &index = static_cast<MIRPtrType *>(elemType)->GetPointedTyIdx();
                stType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(index);
            } else {
                stType = elemType;
            }
        }

        arrayElemType = stType;
    }
    return arrayElemType;
}

StmtNode *CGLowerer::LowerCall(CallNode &callNode, StmtNode *&nextStmt, BlockNode &newBlk, MIRType *retTy, bool uselvar)
{
    /*
     * nextStmt in-out
     * call $foo(constval u32 128)
     * dassign %jlt (dread agg %%retval)
     */
    bool isArrayStore = false;

    if (callNode.GetOpCode() == OP_call) {
        MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
        if ((calleeFunc->GetName() == "MCC_WriteRefField") && (callNode.Opnd(1)->GetOpCode() == OP_iaddrof)) {
            IreadNode *addrExpr = static_cast<IreadNode *>(callNode.Opnd(1));
            if (addrExpr->Opnd(0)->GetOpCode() == OP_array) {
                isArrayStore = true;
            }
        }
    }

    for (size_t i = 0; i < callNode.GetNopndSize(); ++i) {
        BaseNode *newOpnd = LowerExpr(callNode, *callNode.GetNopndAt(i), newBlk);
        callNode.SetOpnd(newOpnd, i);
    }

    if (isArrayStore && checkLoadStore) {
        bool needCheckStore = true;
        MIRType *arrayElemType = GetArrayNodeType(*callNode.Opnd(0));
        MIRType *valueRealType = GetArrayNodeType(*callNode.Opnd(kNodeThirdOpnd));
        if ((arrayElemType != nullptr) && (valueRealType != nullptr) && (arrayElemType->GetKind() == kTypeClass) &&
            static_cast<MIRClassType *>(arrayElemType)->IsFinal() && (valueRealType->GetKind() == kTypeClass) &&
            static_cast<MIRClassType *>(valueRealType)->IsFinal() &&
            valueRealType->GetTypeIndex() == arrayElemType->GetTypeIndex()) {
            needCheckStore = false;
        }

        if (needCheckStore) {
            MIRFunction *fn =
                mirModule.GetMIRBuilder()->GetOrCreateFunction("MCC_Reflect_Check_Arraystore", TyIdx(PTY_void));
            DEBUG_ASSERT(fn->GetFuncSymbol() != nullptr, "fn->GetFuncSymbol() should not be nullptr");
            fn->GetFuncSymbol()->SetAppearsInCode(true);
            beCommon.UpdateTypeTable(*fn->GetMIRFuncType());
            fn->AllocSymTab();
            MapleVector<BaseNode *> args(mirModule.GetMIRBuilder()->GetCurrentFuncCodeMpAllocator()->Adapter());
            args.emplace_back(callNode.Opnd(0));
            args.emplace_back(callNode.Opnd(kNodeThirdOpnd));
            StmtNode *checkStoreStmt = mirModule.GetMIRBuilder()->CreateStmtCall(fn->GetPuidx(), args);
            newBlk.AddStatement(checkStoreStmt);
        }
    }

    DassignNode *dassignNode = nullptr;
    if ((nextStmt != nullptr) && (nextStmt->GetOpCode() == OP_dassign)) {
        dassignNode = static_cast<DassignNode *>(nextStmt);
    }

    /* if nextStmt is not a dassign stmt, return */
    if (dassignNode == nullptr) {
        return &callNode;
    }

    if (!uselvar && retTy && beCommon.GetTypeSize(retTy->GetTypeIndex().GetIdx()) <= k16ByteSize) {
        /* return structure fitting in one or two regs. */
        return &callNode;
    }

    MIRType *retType = nullptr;
    if (callNode.op == OP_icall || callNode.op == OP_icallproto) {
        if (retTy == nullptr) {
            return &callNode;
        } else {
            retType = retTy;
        }
    }

    if (retType == nullptr) {
        MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
        retType = calleeFunc->GetReturnType();
        if (calleeFunc->IsReturnStruct() && (retType->GetPrimType() == PTY_void)) {
            MIRPtrType *pretType = static_cast<MIRPtrType *>((calleeFunc->GetNthParamType(0)));
            CHECK_FATAL(pretType != nullptr, "nullptr is not expected");
            retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pretType->GetPointedTyIdx());
            CHECK_FATAL((retType->GetKind() == kTypeStruct) || (retType->GetKind() == kTypeUnion),
                        "make sure retType is a struct type");
        }
    }

    /* if return type is not of a struct, return */
    if ((retType->GetKind() != kTypeStruct) && (retType->GetKind() != kTypeUnion)) {
        return &callNode;
    }

    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "curFunction should not be nullptr");
    MIRSymbol *dsgnSt = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
    CHECK_FATAL(dsgnSt->GetType()->IsStructType(), "expects a struct type");
    MIRStructType *structTy = static_cast<MIRStructType *>(dsgnSt->GetType());
    if (structTy == nullptr) {
        return &callNode;
    }

    RegreadNode *regReadNode = nullptr;
    if (dassignNode->Opnd(0)->GetOpCode() == OP_regread) {
        regReadNode = static_cast<RegreadNode *>(dassignNode->Opnd(0));
    }
    if (regReadNode == nullptr || (regReadNode->GetRegIdx() != -kSregRetval0)) {
        return &callNode;
    }

    MapleVector<BaseNode *> newNopnd(mirModule.CurFuncCodeMemPoolAllocator()->Adapter());
    AddrofNode *addrofNode = mirModule.CurFuncCodeMemPool()->New<AddrofNode>(OP_addrof);
    addrofNode->SetPrimType(GetLoweredPtrType());
    addrofNode->SetStIdx(dsgnSt->GetStIdx());
    addrofNode->SetFieldID(0);

    if (callNode.op == OP_icall || callNode.op == OP_icallproto) {
        auto ond = callNode.GetNopnd().begin();
        newNopnd.emplace_back(*ond);
        newNopnd.emplace_back(addrofNode);
        for (++ond; ond != callNode.GetNopnd().end(); ++ond) {
            newNopnd.emplace_back(*ond);
        }
    } else {
        newNopnd.emplace_back(addrofNode);
        for (auto *opnd : callNode.GetNopnd()) {
            newNopnd.emplace_back(opnd);
        }
    }

    callNode.SetNOpnd(newNopnd);
    callNode.SetNumOpnds(static_cast<uint8>(newNopnd.size()));
    CHECK_FATAL(nextStmt != nullptr, "nullptr is not expected");
    nextStmt = nextStmt->GetNext();
    return &callNode;
}

void CGLowerer::LowerTypePtr(BaseNode &node) const
{
    if ((node.GetPrimType() == PTY_ptr) || (node.GetPrimType() == PTY_ref)) {
        node.SetPrimType(GetLoweredPtrType());
    }

    if (kOpcodeInfo.IsTypeCvt(node.GetOpCode())) {
        auto &cvt = static_cast<TypeCvtNode &>(node);
        if ((cvt.FromType() == PTY_ptr) || (cvt.FromType() == PTY_ref)) {
            cvt.SetFromType(GetLoweredPtrType());
        }
    } else if (kOpcodeInfo.IsCompare(node.GetOpCode())) {
        auto &cmp = static_cast<CompareNode &>(node);
        if ((cmp.GetOpndType() == PTY_ptr) || (cmp.GetOpndType() == PTY_ref)) {
            cmp.SetOpndType(GetLoweredPtrType());
        }
    }
}

void CGLowerer::LowerEntry(MIRFunction &func)
{
    // determine if needed to insert fake parameter to return struct for current function
    if (func.IsReturnStruct()) {
        MIRType *retType = func.GetReturnType();
#if TARGAARCH64
        if (Triple::GetTriple().GetArch() == Triple::ArchType::aarch64) {
            PrimType pty = IsStructElementSame(retType);
            if (pty == PTY_f32 || pty == PTY_f64 || IsPrimitiveVector(pty)) {
                func.SetStructReturnedInRegs();
                return;
            }
        }
#endif
        if (retType->GetPrimType() != PTY_agg) {
            return;
        }
        if (retType->GetSize() > k16ByteSize) {
            func.SetFirstArgReturn();
            func.GetMIRFuncType()->SetFirstArgReturn();
        } else {
            func.SetStructReturnedInRegs();
        }
    }
    if (func.IsFirstArgReturn() && func.GetReturnType()->GetPrimType() != PTY_void) {
        MIRSymbol *retSt = func.GetSymTab()->CreateSymbol(kScopeLocal);
        retSt->SetStorageClass(kScFormal);
        retSt->SetSKind(kStVar);
        std::string retName(".return.");
        MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
        DEBUG_ASSERT(funcSt != nullptr, "null ptr check");
        retName += funcSt->GetName();
        retSt->SetNameStrIdx(retName);
        MIRType *pointType = beCommon.BeGetOrCreatePointerType(*func.GetReturnType());

        retSt->SetTyIdx(pointType->GetTypeIndex());
        std::vector<MIRSymbol *> formals;
        formals.emplace_back(retSt);
        for (uint32 i = 0; i < func.GetFormalCount(); ++i) {
            auto formal = func.GetFormal(i);
            formals.emplace_back(formal);
        }
        func.SetFirstArgReturn();

        beCommon.AddElementToFuncReturnType(func, func.GetReturnTyIdx());

        func.UpdateFuncTypeAndFormalsAndReturnType(formals, TyIdx(PTY_void), true);
        auto *funcType = func.GetMIRFuncType();
        DEBUG_ASSERT(funcType != nullptr, "null ptr check");
        funcType->SetFirstArgReturn();
        beCommon.AddTypeSizeAndAlign(funcType->GetTypeIndex(), GetPrimTypeSize(funcType->GetPrimType()));
    }
}

void CGLowerer::LowerPseudoRegs(const MIRFunction &func) const
{
    for (uint32 i = 1; i < func.GetPregTab()->Size(); ++i) {
        MIRPreg *ipr = func.GetPregTab()->PregFromPregIdx(i);
        PrimType primType = ipr->GetPrimType();
        if (primType == PTY_u1) {
            ipr->SetPrimType(PTY_u32);
        }
    }
}

void CGLowerer::CleanupBranches(MIRFunction &func) const
{
    BlockNode *block = func.GetBody();
    StmtNode *prev = nullptr;
    StmtNode *next = nullptr;
    for (StmtNode *curr = block->GetFirst(); curr != nullptr; curr = next) {
        next = curr->GetNext();
        if (next != nullptr) {
            CHECK_FATAL(curr == next->GetPrev(), "unexpected node");
        }
        if ((next != nullptr) && (prev != nullptr) && (curr->GetOpCode() == OP_goto)) {
            /*
             * Skip until find a label.
             * Note that the CURRent 'goto' statement may be the last statement
             * when discounting comment statements.
             * Make sure we don't lose any comments.
             */
            StmtNode *cmtB = nullptr;
            StmtNode *cmtE = nullptr;
            bool isCleanable = true;
            while ((next != nullptr) && (next->GetOpCode() != OP_label)) {
                if ((next->GetOpCode() == OP_endtry)) {
                    isCleanable = false;
                    break;
                }
                next = next->GetNext();
            }
            if ((next != nullptr) && (!isCleanable)) {
                prev = next->GetPrev();
                continue;
            }

            next = curr->GetNext();

            while ((next != nullptr) && (next->GetOpCode() != OP_label)) {
                if (next->GetOpCode() == OP_comment) {
                    if (cmtB == nullptr) {
                        cmtB = next;
                        cmtE = next;
                    } else {
                        CHECK_FATAL(cmtE != nullptr, "cmt_e is null in CGLowerer::CleanupBranches");
                        cmtE->SetNext(next);
                        next->SetPrev(cmtE);
                        cmtE = next;
                    }
                }
                next = next->GetNext();
            }

            curr->SetNext(next);

            if (next != nullptr) {
                next->SetPrev(curr);
            }

            StmtNode *insertAfter = nullptr;

            if ((next != nullptr) &&
                ((static_cast<GotoNode *>(curr))->GetOffset() == (static_cast<LabelNode *>(next))->GetLabelIdx())) {
                insertAfter = prev;
                prev->SetNext(next); /* skip goto statement (which is pointed by curr) */
                next->SetPrev(prev);
                curr = next;            /* make curr point to the label statement */
                next = next->GetNext(); /* advance next to the next statement of the label statement */
            } else {
                insertAfter = curr;
            }

            /* insert comments before 'curr' */
            if (cmtB != nullptr) {
                CHECK_FATAL(cmtE != nullptr, "nullptr is not expected");
                StmtNode *iaNext = insertAfter->GetNext();
                if (iaNext != nullptr) {
                    iaNext->SetPrev(cmtE);
                }
                cmtE->SetNext(iaNext);

                insertAfter->SetNext(cmtB);
                cmtB->SetPrev(insertAfter);

                if (insertAfter == curr) {
                    curr = cmtE;
                }
            }
            if (next == nullptr) {
                func.GetBody()->SetLast(curr);
            }
        }
        prev = curr;
    }
    CHECK_FATAL(func.GetBody()->GetLast() == prev, "make sure the return value of GetLast equal prev");
}

inline bool IsAccessingTheSameMemoryLocation(const DassignNode &dassign, const RegreadNode &rRead,
                                             const CGLowerer &cgLowerer)
{
    StIdx stIdx = cgLowerer.GetSymbolReferredToByPseudoRegister(rRead.GetRegIdx());
    return ((dassign.GetStIdx() == stIdx) && (dassign.GetFieldID() == 0));
}

inline bool IsAccessingTheSameMemoryLocation(const DassignNode &dassign, const DreadNode &dread)
{
    return ((dassign.GetStIdx() == dread.GetStIdx()) && (dassign.GetFieldID() == dread.GetFieldID()));
}

inline bool IsDassignNOP(const DassignNode &dassign)
{
    if (dassign.GetRHS()->GetOpCode() == OP_dread) {
        return IsAccessingTheSameMemoryLocation(dassign, static_cast<DreadNode &>(*dassign.GetRHS()));
    }
    return false;
}

inline bool IsConstvalZero(const BaseNode &n)
{
    return ((n.GetOpCode() == OP_constval) && static_cast<const ConstvalNode &>(n).GetConstVal()->IsZero());
}

#define NEXT_ID(x) ((x) + 1)
#define INTRN_FIRST_SYNC_ENTER NEXT_ID(INTRN_LAST)
#define INTRN_SECOND_SYNC_ENTER NEXT_ID(INTRN_FIRST_SYNC_ENTER)
#define INTRN_THIRD_SYNC_ENTER NEXT_ID(INTRN_SECOND_SYNC_ENTER)
#define INTRN_FOURTH_SYNC_ENTER NEXT_ID(INTRN_THIRD_SYNC_ENTER)
#define INTRN_YNC_EXIT NEXT_ID(INTRN_FOURTH_SYNC_ENTER)

std::vector<std::pair<CGLowerer::BuiltinFunctionID, PUIdx>> CGLowerer::builtinFuncIDs;

LabelIdx CGLowerer::GetLabelIdx(MIRFunction &curFunc) const
{
    std::string suffix = std::to_string(curFunc.GetLabelTab()->GetLabelTableSize());
    GStrIdx labelStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("__label_BC_" + suffix);
    LabelIdx labIdx = curFunc.GetLabelTab()->AddLabel(labelStrIdx);
    return labIdx;
}

BaseNode *CGLowerer::LowerExpr(BaseNode &parent, BaseNode &expr, BlockNode &blkNode)
{
    bool isCvtU1Expr = (expr.GetOpCode() == OP_cvt && expr.GetPrimType() == PTY_u1 &&
                        static_cast<TypeCvtNode &>(expr).FromType() != PTY_u1);
    if (expr.GetPrimType() == PTY_u1) {
        expr.SetPrimType(PTY_u8);
    }

    if (expr.GetOpCode() == OP_iread && expr.Opnd(0)->GetOpCode() == OP_array) {
        BaseNode *node = LowerExpr(expr, *expr.Opnd(0), blkNode);
        if (node->GetOpCode() == OP_intrinsicop) {
            auto *binNode = static_cast<IntrinsicopNode *>(node);
            return binNode;
        } else {
            expr.SetOpnd(node, 0);
        }
    } else {
        for (size_t i = 0; i < expr.NumOpnds(); ++i) {
            expr.SetOpnd(LowerExpr(expr, *expr.Opnd(i), blkNode), i);
        }
    }
    // Convert `cvt u1 xx <expr>` to `ne u8 xx (<expr>, constval xx 0)`
    // No need to convert `cvt u1 u1 <expr>`
    if (isCvtU1Expr) {
        auto &cvtExpr = static_cast<TypeCvtNode &>(expr);
        PrimType fromType = cvtExpr.FromType();
        auto *fromMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(fromType));
        // We use u8 instead of u1 because codegen can't recognize u1
        auto *toMIRType = GlobalTables::GetTypeTable().GetUInt8();
        auto *zero = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *fromMIRType);
        auto *converted = mirBuilder->CreateExprCompare(OP_ne, *toMIRType, *fromMIRType, cvtExpr.Opnd(0),
                                                        mirBuilder->CreateConstval(zero));
        return converted;
    }
    switch (expr.GetOpCode()) {
        case OP_array: {
            DEBUG_ASSERT(false, "unsupported OP_array");
            return &expr;
        }

        case OP_dread:
            return LowerDread(static_cast<DreadNode &>(expr), blkNode);

        case OP_addrof:
            return LowerAddrof(static_cast<AddrofNode &>(expr));

        case OP_iread:
            return LowerIread(static_cast<IreadNode &>(expr));

        case OP_iaddrof:
            return LowerIaddrof(static_cast<IreadNode &>(expr));

        case OP_cvt:
        case OP_retype:
        case OP_zext:
        case OP_sext:
            return LowerCastExpr(expr);
        default:
            return &expr;
    }
}

BaseNode *CGLowerer::LowerDread(DreadNode &dread, const BlockNode &block)
{
    /* use PTY_u8 for boolean type in dread/iread */
    if (dread.GetPrimType() == PTY_u1) {
        dread.SetPrimType(PTY_u8);
    }
    return (dread.GetFieldID() == 0 ? LowerDreadToThreadLocal(dread, block) : LowerDreadBitfield(dread));
}

void CGLowerer::LowerRegassign(RegassignNode &regNode, BlockNode &newBlk)
{
    BaseNode *rhsOpnd = regNode.Opnd(0);
    regNode.SetOpnd(LowerExpr(regNode, *rhsOpnd, newBlk), 0);
    newBlk.AddStatement(&regNode);
}

BaseNode *CGLowerer::ExtractSymbolAddress(const StIdx &stIdx)
{
    auto builder = mirModule.GetMIRBuilder();
    return builder->CreateExprAddrof(0, stIdx);
}

BaseNode *CGLowerer::LowerDreadToThreadLocal(BaseNode &expr, const BlockNode &block)
{
    auto *result = &expr;
    if (expr.GetOpCode() != maple::OP_dread) {
        return result;
    }
    uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    auto dread = static_cast<DreadNode &>(expr);
    StIdx stIdx = dread.GetStIdx();
    if (!stIdx.IsGlobal()) {
        return result;
    }
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    CHECK_FATAL(symbol != nullptr, "symbol should not be nullptr");

    if (symbol->IsThreadLocal()) {
        //  iread <* u32> 0 (regread u64 %addr)
        auto addr = ExtractSymbolAddress(stIdx);
        auto ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*symbol->GetType());
        auto iread = mirModule.GetMIRBuilder()->CreateExprIread(*symbol->GetType(), *ptrType, dread.GetFieldID(), addr);
        result = iread;
    }
    uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    if (newTypeTableSize != oldTypeTableSize) {
        beCommon.AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
    return result;
}

StmtNode *CGLowerer::LowerDassignToThreadLocal(StmtNode &stmt, const BlockNode &block)
{
    StmtNode *result = &stmt;
    if (stmt.GetOpCode() != maple::OP_dassign) {
        return result;
    }
    uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    auto dAssign = static_cast<DassignNode &>(stmt);
    StIdx stIdx = dAssign.GetStIdx();
    if (!stIdx.IsGlobal()) {
        return result;
    }
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    DEBUG_ASSERT(symbol != nullptr, "symbol should not be nullptr");
    if (symbol->IsThreadLocal()) {
        //  iassign <* u32> 0 (regread u64 %addr, dread u32 $x)
        auto addr = ExtractSymbolAddress(stIdx);
        auto ptrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*symbol->GetType());
        auto iassign =
            mirModule.GetMIRBuilder()->CreateStmtIassign(*ptrType, dAssign.GetFieldID(), addr, dAssign.GetRHS());
        result = iassign;
    }
    uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    if (newTypeTableSize != oldTypeTableSize) {
        beCommon.AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
    return result;
}

void CGLowerer::LowerDassign(DassignNode &dsNode, BlockNode &newBlk)
{
    StmtNode *newStmt = nullptr;
    BaseNode *rhs = nullptr;
    Opcode op = dsNode.GetRHS()->GetOpCode();
    if (dsNode.GetFieldID() != 0) {
        newStmt = LowerDassignBitfield(dsNode, newBlk);
    } else if (op == OP_intrinsicop) {
        IntrinsicopNode *intrinNode = static_cast<IntrinsicopNode *>(dsNode.GetRHS());
        MIRType *retType = IntrinDesc::intrinTable[intrinNode->GetIntrinsic()].GetReturnType();
        CHECK_FATAL(retType != nullptr, "retType should not be nullptr");
        if (retType->GetKind() == kTypeStruct) {
            newStmt = LowerIntrinsicopDassign(dsNode, *intrinNode, newBlk);
        } else {
            rhs = LowerExpr(dsNode, *intrinNode, newBlk);
            dsNode.SetRHS(rhs);
            CHECK_FATAL(dsNode.GetRHS() != nullptr, "dsNode->rhs is null in CGLowerer::LowerDassign");
            if (!IsDassignNOP(dsNode)) {
                newStmt = &dsNode;
            }
        }
    } else {
        rhs = LowerExpr(dsNode, *dsNode.GetRHS(), newBlk);
        dsNode.SetRHS(rhs);
        newStmt = &dsNode;
    }

    if (newStmt != nullptr) {
        newBlk.AddStatement(LowerDassignToThreadLocal(*newStmt, newBlk));
    }
}

StmtNode *CGLowerer::LowerIntrinsicopDassign(const DassignNode &dsNode, IntrinsicopNode &intrinNode, BlockNode &newBlk)
{
    for (size_t i = 0; i < intrinNode.GetNumOpnds(); ++i) {
        DEBUG_ASSERT(intrinNode.Opnd(i) != nullptr, "intrinNode.Opnd(i) should not be nullptr");
        intrinNode.SetOpnd(LowerExpr(intrinNode, *intrinNode.Opnd(i), newBlk), i);
    }
    MIRIntrinsicID intrnID = intrinNode.GetIntrinsic();
    IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrnID];
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    const std::string name = intrinDesc->name;
    CHECK_FATAL(intrinDesc->name != nullptr, "intrinDesc's name should not be nullptr");
    st->SetNameStrIdx(name);
    st->SetStorageClass(kScText);
    st->SetSKind(kStFunc);
    MIRFunction *fn = mirModule.GetMemPool()->New<MIRFunction>(&mirModule, st->GetStIdx());
    MapleVector<BaseNode *> &nOpnds = intrinNode.GetNopnd();
    st->SetFunction(fn);
    std::vector<TyIdx> fnTyVec;
    std::vector<TypeAttrs> fnTaVec;
    CHECK_FATAL(intrinDesc->IsJsOp(), "intrinDesc should be JsOp");
    /* setup parameters */
    for (uint32 i = 0; i < nOpnds.size(); ++i) {
        fnTyVec.emplace_back(GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_a32)->GetTypeIndex());
        fnTaVec.emplace_back(TypeAttrs());
        BaseNode *addrNode = beCommon.GetAddressOfNode(*nOpnds[i]);
        CHECK_FATAL(addrNode != nullptr, "addrNode should not be nullptr");
        nOpnds[i] = addrNode;
    }
    DEBUG_ASSERT(mirModule.CurFunction() != nullptr, "curFunction should not be nullptr");
    MIRSymbol *dst = mirModule.CurFunction()->GetLocalOrGlobalSymbol(dsNode.GetStIdx());
    MIRType *ty = dst->GetType();
    MIRType *fnType = beCommon.BeGetOrCreateFunctionType(ty->GetTypeIndex(), fnTyVec, fnTaVec);
    st->SetTyIdx(fnType->GetTypeIndex());
    fn->SetMIRFuncType(static_cast<MIRFuncType *>(fnType));
    fn->SetReturnTyIdx(ty->GetTypeIndex());
    CHECK_FATAL(ty->GetKind() == kTypeStruct, "ty's kind should be struct type");
    CHECK_FATAL(dsNode.GetFieldID() == 0, "dsNode's filedId should equal");
    AddrofNode *addrofNode = mirBuilder->CreateAddrof(*dst, PTY_a32);
    MapleVector<BaseNode *> newOpnd(mirModule.CurFuncCodeMemPoolAllocator()->Adapter());
    newOpnd.emplace_back(addrofNode);
    (void)newOpnd.insert(newOpnd.end(), nOpnds.begin(), nOpnds.end());
    CallNode *callStmt = mirModule.CurFuncCodeMemPool()->New<CallNode>(mirModule, OP_call);
    callStmt->SetPUIdx(st->GetFunction()->GetPuidx());
    callStmt->SetNOpnd(newOpnd);
    return callStmt;
}

StmtNode *CGLowerer::LowerDefaultIntrinsicCall(IntrinsiccallNode &intrincall, MIRSymbol &st, MIRFunction &fn)
{
    MIRIntrinsicID intrnID = intrincall.GetIntrinsic();
    IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrnID];
    std::vector<TyIdx> funcTyVec;
    std::vector<TypeAttrs> fnTaVec;
    MapleVector<BaseNode *> &nOpnds = intrincall.GetNopnd();
    MIRType *retTy = intrinDesc->GetReturnType();
    CHECK_FATAL(retTy != nullptr, "retTy should not be nullptr");
    if (retTy->GetKind() == kTypeStruct) {
        funcTyVec.emplace_back(beCommon.BeGetOrCreatePointerType(*retTy)->GetTypeIndex());
        fnTaVec.emplace_back(TypeAttrs());
        fn.SetReturnStruct();
    }
    for (uint32 i = 0; i < nOpnds.size(); ++i) {
        MIRType *argTy = intrinDesc->GetArgType(i);
        CHECK_FATAL(argTy != nullptr, "argTy should not be nullptr");
        if (argTy->GetKind() == kTypeStruct) {
            funcTyVec.emplace_back(GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_a32)->GetTypeIndex());
            fnTaVec.emplace_back(TypeAttrs());
            BaseNode *addrNode = beCommon.GetAddressOfNode(*nOpnds[i]);
            CHECK_FATAL(addrNode != nullptr, "can not get address");
            nOpnds[i] = addrNode;
        } else {
            funcTyVec.emplace_back(argTy->GetTypeIndex());
            fnTaVec.emplace_back(TypeAttrs());
        }
    }
    MIRType *funcType = beCommon.BeGetOrCreateFunctionType(retTy->GetTypeIndex(), funcTyVec, fnTaVec);
    st.SetTyIdx(funcType->GetTypeIndex());
    fn.SetMIRFuncType(static_cast<MIRFuncType *>(funcType));
    if (retTy->GetKind() == kTypeStruct) {
        fn.SetReturnTyIdx(static_cast<TyIdx>(PTY_void));
    } else {
        fn.SetReturnTyIdx(retTy->GetTypeIndex());
    }
    return static_cast<CallNode *>(mirBuilder->CreateStmtCall(fn.GetPuidx(), nOpnds));
}

StmtNode *CGLowerer::LowerIntrinsiccall(IntrinsiccallNode &intrincall, BlockNode &newBlk)
{
    MIRIntrinsicID intrnID = intrincall.GetIntrinsic();
    for (size_t i = 0; i < intrincall.GetNumOpnds(); ++i) {
        intrincall.SetOpnd(LowerExpr(intrincall, *intrincall.Opnd(i), newBlk), i);
    }
    CHECK_FATAL(intrnID != INTRN_MPL_CLEAR_STACK, "unsupported intrinsic");
    if (intrnID == INTRN_C_va_start) {
        return &intrincall;
    }
    if (intrnID == maple::INTRN_C___builtin_division_exception) {
        return &intrincall;
    }
    IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[intrnID];
    if (intrinDesc->IsSpecial() || intrinDesc->IsAtomic()) {
        /* For special intrinsics we leave them to CGFunc::SelectIntrinsicCall() */
        return &intrincall;
    }
    /* default lowers intrinsic call to real function call. */
    MIRSymbol *st = GlobalTables::GetGsymTable().CreateSymbol(kScopeGlobal);
    CHECK_FATAL(intrinDesc->name != nullptr, "intrinsic's name should not be nullptr");
    const std::string name = intrinDesc->name;
    st->SetNameStrIdx(name);
    st->SetStorageClass(kScText);
    st->SetSKind(kStFunc);
    MIRFunction *fn = mirBuilder->GetOrCreateFunction(intrinDesc->name, TyIdx(0));
    beCommon.UpdateTypeTable(*fn->GetMIRFuncType());
    fn->AllocSymTab();
    st->SetFunction(fn);
    st->SetAppearsInCode(true);
    return LowerDefaultIntrinsicCall(intrincall, *st, *fn);
}

PUIdx CGLowerer::GetBuiltinToUse(BuiltinFunctionID id) const
{
    /*
     * use std::vector & linear search as the number of entries is small.
     * we may revisit it if the number of entries gets larger.
     */
    for (const auto &funcID : builtinFuncIDs) {
        if (funcID.first == id) {
            return funcID.second;
        }
    }
    return kFuncNotFound;
}

bool CGLowerer::IsIntrinsicCallHandledAtLowerLevel(MIRIntrinsicID intrinsic) const
{
    switch (intrinsic) {
        case INTRN_MPL_ATOMIC_EXCHANGE_PTR:
        // js
        case INTRN_ADD_WITH_OVERFLOW:
        case INTRN_SUB_WITH_OVERFLOW:
        case INTRN_MUL_WITH_OVERFLOW:
            return true;
        default: {
            return false;
        }
    }
}

void CGLowerer::LowerFunc(MIRFunction &func)
{
    labelIdx = 0;
    SetCurrentFunc(&func);
    LowerEntry(func);
    LowerPseudoRegs(func);
    BlockNode *origBody = func.GetBody();
    CHECK_FATAL(origBody != nullptr, "origBody should not be nullptr");

    BlockNode *newBody = LowerBlock(*origBody);
    func.SetBody(newBody);
    if (needBranchCleanup) {
        CleanupBranches(func);
    }

    uint32 oldTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    uint32 newTypeTableSize = GlobalTables::GetTypeTable().GetTypeTableSize();
    if (newTypeTableSize != oldTypeTableSize) {
        beCommon.AddNewTypeAfterBecommon(oldTypeTableSize, newTypeTableSize);
    }
}
} /* namespace maplebe */
