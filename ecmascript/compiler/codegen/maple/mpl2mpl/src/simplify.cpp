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

#include "triple.h"
#include "simplify.h"
#include <functional>
#include <initializer_list>
#include <iostream>
#include <algorithm>
#include "constantfold.h"
#include "mpl_logging.h"

namespace maple {

namespace {

constexpr char kFuncNameOfMemset[] = "memset";
constexpr char kFuncNameOfMemcpy[] = "memcpy";
constexpr char kFuncNameOfMemsetS[] = "memset_s";
constexpr char kFuncNameOfMemcpyS[] = "memcpy_s";

// Truncate the constant field of 'union' if it's written as scalar type (e.g. int),
// but accessed as bit-field type with smaller size.
//
// Return the truncated constant or nullptr if the constant doesn't need to be truncated.
MIRConst *TruncateUnionConstant(const MIRStructType &unionType, MIRConst *fieldCst, const MIRType &unionFieldType)
{
    if (unionType.GetKind() != kTypeUnion) {
        return nullptr;
    }

    auto *bitFieldType = safe_cast<MIRBitFieldType>(unionFieldType);
    auto *intCst = safe_cast<MIRIntConst>(fieldCst);

    if (!bitFieldType || !intCst) {
        return nullptr;
    }

    bool isBigEndian = Triple::GetTriple().IsBigEndian();

    IntVal val = intCst->GetValue();
    uint8 bitSize = bitFieldType->GetFieldSize();

    if (bitSize >= val.GetBitWidth()) {
        return nullptr;
    }

    if (isBigEndian) {
        val = val.LShr(val.GetBitWidth() - bitSize);
    } else {
        val = val & ((uint64(1) << bitSize) - 1);
    }

    return GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, fieldCst->GetType());
}

}  // namespace

// If size (in byte) is bigger than this threshold, we won't expand memop
const uint32 SimplifyMemOp::thresholdMemsetExpand = 512;
const uint32 SimplifyMemOp::thresholdMemcpyExpand = 512;
const uint32 SimplifyMemOp::thresholdMemsetSExpand = 1024;
const uint32 SimplifyMemOp::thresholdMemcpySExpand = 1024;
static const uint32 kMaxMemoryBlockSizeToAssign = 8;  // in byte

bool Simplify::IsMathSqrt(const std::string funcName)
{
    return false;
}

bool Simplify::IsMathAbs(const std::string funcName)
{
    return false;
}

bool Simplify::IsMathMax(const std::string funcName)
{
    return false;
}

bool Simplify::IsMathMin(const std::string funcName)
{
    return false;
}

bool Simplify::SimplifyMathMethod(const StmtNode &stmt, BlockNode &block)
{
    if (stmt.GetOpCode() != OP_callassigned) {
        return false;
    }
    auto &cnode = static_cast<const CallNode &>(stmt);
    MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(cnode.GetPUIdx());
    DEBUG_ASSERT(calleeFunc != nullptr, "null ptr check");
    const std::string &funcName = calleeFunc->GetName();
    if (funcName.empty()) {
        return false;
    }
    if (!mirMod.IsCModule()) {
        return false;
    }
    if (cnode.GetNumOpnds() == 0 || cnode.GetReturnVec().empty()) {
        return false;
    }

    auto *opnd0 = cnode.Opnd(0);
    DEBUG_ASSERT(opnd0 != nullptr, "null ptr check");
    auto *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(opnd0->GetPrimType());

    BaseNode *opExpr = nullptr;
    if (IsMathSqrt(funcName) && !IsPrimitiveFloat(opnd0->GetPrimType())) {
        opExpr = builder->CreateExprUnary(OP_sqrt, *type, opnd0);
    } else if (IsMathAbs(funcName)) {
        opExpr = builder->CreateExprUnary(OP_abs, *type, opnd0);
    } else if (IsMathMax(funcName)) {
        opExpr = builder->CreateExprBinary(OP_max, *type, opnd0, cnode.Opnd(1));
    } else if (IsMathMin(funcName)) {
        opExpr = builder->CreateExprBinary(OP_min, *type, opnd0, cnode.Opnd(1));
    }
    if (opExpr != nullptr) {
        auto stIdx = cnode.GetNthReturnVec(0).first;
        auto *dassign = builder->CreateStmtDassign(stIdx, 0, opExpr);
        block.ReplaceStmt1WithStmt2(&stmt, dassign);
        return true;
    }
    return false;
}

void Simplify::SimplifyCallAssigned(StmtNode &stmt, BlockNode &block)
{
    if (SimplifyMathMethod(stmt, block)) {
        return;
    }
    simplifyMemOp.SetDebug(dump);
    simplifyMemOp.SetFunction(currFunc);
    if (simplifyMemOp.AutoSimplify(stmt, block, false)) {
        return;
    }
}

constexpr uint32 kUpperLimitOfFieldNum = 10;
static MIRStructType *GetDassignedStructType(const DassignNode *dassign, MIRFunction *func)
{
    const auto &lhsStIdx = dassign->GetStIdx();
    auto lhsSymbol = func->GetLocalOrGlobalSymbol(lhsStIdx);
    auto lhsAggType = lhsSymbol->GetType();
    if (!lhsAggType->IsStructType()) {
        return nullptr;
    }
    if (lhsAggType->GetKind() == kTypeUnion) {  // no need to split union's field
        return nullptr;
    }
    auto lhsFieldID = dassign->GetFieldID();
    if (lhsFieldID != 0) {
        CHECK_FATAL(lhsAggType->IsStructType(), "only struct has non-zero fieldID");
        lhsAggType = static_cast<MIRStructType *>(lhsAggType)->GetFieldType(lhsFieldID);
        if (!lhsAggType->IsStructType()) {
            return nullptr;
        }
        if (lhsAggType->GetKind() == kTypeUnion) {  // no need to split union's field
            return nullptr;
        }
    }
    if (static_cast<MIRStructType *>(lhsAggType)->NumberOfFieldIDs() > kUpperLimitOfFieldNum) {
        return nullptr;
    }
    return static_cast<MIRStructType *>(lhsAggType);
}

static MIRStructType *GetIassignedStructType(const IassignNode *iassign)
{
    auto ptrTyIdx = iassign->GetTyIdx();
    auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx);
    CHECK_FATAL(ptrType->IsMIRPtrType(), "must be pointer type");
    auto aggTyIdx = static_cast<MIRPtrType *>(ptrType)->GetPointedTyIdxWithFieldID(iassign->GetFieldID());
    auto *lhsAggType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(aggTyIdx);
    if (!lhsAggType->IsStructType()) {
        return nullptr;
    }
    if (lhsAggType->GetKind() == kTypeUnion) {
        return nullptr;
    }
    if (static_cast<MIRStructType *>(lhsAggType)->NumberOfFieldIDs() > kUpperLimitOfFieldNum) {
        return nullptr;
    }
    return static_cast<MIRStructType *>(lhsAggType);
}

static MIRStructType *GetReadedStructureType(const DreadNode *dread, const MIRFunction *func)
{
    const auto &rhsStIdx = dread->GetStIdx();
    auto rhsSymbol = func->GetLocalOrGlobalSymbol(rhsStIdx);
    auto rhsAggType = rhsSymbol->GetType();
    auto rhsFieldID = dread->GetFieldID();
    if (rhsFieldID != 0) {
        CHECK_FATAL(rhsAggType->IsStructType(), "only struct has non-zero fieldID");
        rhsAggType = static_cast<MIRStructType *>(rhsAggType)->GetFieldType(rhsFieldID);
    }
    if (!rhsAggType->IsStructType()) {
        return nullptr;
    }
    return static_cast<MIRStructType *>(rhsAggType);
}

static MIRStructType *GetReadedStructureType(const IreadNode *iread, const MIRFunction *)
{
    auto rhsPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
    CHECK_FATAL(rhsPtrType->IsMIRPtrType(), "must be pointer type");
    auto rhsAggType = static_cast<MIRPtrType *>(rhsPtrType)->GetPointedType();
    auto rhsFieldID = iread->GetFieldID();
    if (rhsFieldID != 0) {
        CHECK_FATAL(rhsAggType->IsStructType(), "only struct has non-zero fieldID");
        rhsAggType = static_cast<MIRStructType *>(rhsAggType)->GetFieldType(rhsFieldID);
    }
    if (!rhsAggType->IsStructType()) {
        return nullptr;
    }
    return static_cast<MIRStructType *>(rhsAggType);
}

template <class RhsType, class AssignType>
static StmtNode *SplitAggCopy(const AssignType *assignNode, MIRStructType *structureType, BlockNode *block,
                              MIRFunction *func)
{
    auto *readNode = static_cast<RhsType *>(assignNode->GetRHS());
    auto rhsFieldID = readNode->GetFieldID();
    auto *rhsAggType = GetReadedStructureType(readNode, func);
    if (structureType != rhsAggType) {
        return nullptr;
    }

    for (FieldID id = 1; id <= static_cast<FieldID>(structureType->NumberOfFieldIDs()); ++id) {
        MIRType *fieldType = structureType->GetFieldType(id);
        if (fieldType->GetSize() == 0) {
            continue;  // field size is zero for empty struct/union;
        }
        if (fieldType->GetKind() == kTypeBitField && static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
            continue;  // bitfield size is zero
        }
        auto *newDassign = assignNode->CloneTree(func->GetCodeMemPoolAllocator());
        newDassign->SetFieldID(assignNode->GetFieldID() + id);
        auto *newRHS = static_cast<RhsType *>(newDassign->GetRHS());
        newRHS->SetFieldID(rhsFieldID + id);
        newRHS->SetPrimType(fieldType->GetPrimType());
        block->InsertAfter(assignNode, newDassign);
        if (fieldType->IsMIRUnionType()) {
            id += static_cast<FieldID>(fieldType->NumberOfFieldIDs());
        }
    }
    auto newAssign = assignNode->GetNext();
    block->RemoveStmt(assignNode);
    return newAssign;
}

static StmtNode *SplitDassignAggCopy(DassignNode *dassign, BlockNode *block, MIRFunction *func)
{
    auto *rhs = dassign->GetRHS();
    if (rhs->GetPrimType() != PTY_agg) {
        return nullptr;
    }

    auto *lhsAggType = GetDassignedStructType(dassign, func);
    if (lhsAggType == nullptr) {
        return nullptr;
    }

    if (rhs->GetOpCode() == OP_dread) {
        auto *lhsSymbol = func->GetLocalOrGlobalSymbol(dassign->GetStIdx());
        auto *rhsSymbol = func->GetLocalOrGlobalSymbol(static_cast<DreadNode *>(rhs)->GetStIdx());
        if (!lhsSymbol->IsLocal() && !rhsSymbol->IsLocal()) {
            return nullptr;
        }

        return SplitAggCopy<DreadNode>(dassign, lhsAggType, block, func);
    } else if (rhs->GetOpCode() == OP_iread) {
        return SplitAggCopy<IreadNode>(dassign, lhsAggType, block, func);
    }
    return nullptr;
}

static StmtNode *SplitIassignAggCopy(IassignNode *iassign, BlockNode *block, MIRFunction *func)
{
    auto rhs = iassign->GetRHS();
    if (rhs->GetPrimType() != PTY_agg) {
        return nullptr;
    }

    auto *lhsAggType = GetIassignedStructType(iassign);
    if (lhsAggType == nullptr) {
        return nullptr;
    }

    if (rhs->GetOpCode() == OP_dread) {
        return SplitAggCopy<DreadNode>(iassign, lhsAggType, block, func);
    } else if (rhs->GetOpCode() == OP_iread) {
        return SplitAggCopy<IreadNode>(iassign, lhsAggType, block, func);
    }
    return nullptr;
}

bool UseGlobalVar(const BaseNode *expr)
{
    if (expr->GetOpCode() == OP_addrof || expr->GetOpCode() == OP_dread) {
        StIdx stIdx = static_cast<const AddrofNode *>(expr)->GetStIdx();
        if (stIdx.IsGlobal()) {
            return true;
        }
    }
    for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
        if (UseGlobalVar(expr->Opnd(i))) {
            return true;
        }
    }
    return false;
}

void Simplify::ProcessStmt(StmtNode &stmt)
{
    switch (stmt.GetOpCode()) {
        case OP_callassigned: {
            SimplifyCallAssigned(stmt, *currBlock);
            break;
        }
        case OP_intrinsiccall: {
            simplifyMemOp.SetDebug(dump);
            simplifyMemOp.SetFunction(currFunc);
            (void)simplifyMemOp.AutoSimplify(stmt, *currBlock, false);
            break;
        }
        case OP_dassign: {
            auto *newStmt = SplitDassignAggCopy(static_cast<DassignNode *>(&stmt), currBlock, currFunc);
            if (newStmt) {
                ProcessBlock(*newStmt);
            }
            break;
        }
        case OP_iassign: {
            auto *newStmt = SplitIassignAggCopy(static_cast<IassignNode *>(&stmt), currBlock, currFunc);
            if (newStmt) {
                ProcessBlock(*newStmt);
            }
            break;
        }
        case OP_if:
        case OP_while:
        case OP_dowhile: {
            auto unaryStmt = static_cast<UnaryStmtNode &>(stmt);
            unaryStmt.SetRHS(SimplifyExpr(*unaryStmt.GetRHS()));
            return;
        }
        default: {
            break;
        }
    }
    for (size_t i = 0; i < stmt.NumOpnds(); ++i) {
        if (stmt.Opnd(i)) {
            stmt.SetOpnd(SimplifyExpr(*stmt.Opnd(i)), i);
        }
    }
}

BaseNode *Simplify::SimplifyExpr(BaseNode &expr)
{
    switch (expr.GetOpCode()) {
        case OP_dread: {
            auto &dread = static_cast<DreadNode &>(expr);
            return ReplaceExprWithConst(dread);
        }
        default: {
            for (auto i = 0; i < expr.GetNumOpnds(); i++) {
                if (expr.Opnd(i)) {
                    expr.SetOpnd(SimplifyExpr(*expr.Opnd(i)), i);
                }
            }
            break;
        }
    }
    return &expr;
}

BaseNode *Simplify::ReplaceExprWithConst(DreadNode &dread)
{
    auto stIdx = dread.GetStIdx();
    auto fieldId = dread.GetFieldID();
    auto *symbol = currFunc->GetLocalOrGlobalSymbol(stIdx);
    auto *symbolConst = symbol->GetKonst();
    if (!currFunc->GetModule()->IsCModule() || !symbolConst || !stIdx.IsGlobal() ||
        !IsSymbolReplaceableWithConst(*symbol)) {
        return &dread;
    }
    if (fieldId != 0) {
        symbolConst = GetElementConstFromFieldId(fieldId, symbolConst);
    }
    if (!symbolConst || !IsConstRepalceable(*symbolConst)) {
        return &dread;
    }
    return currFunc->GetModule()->GetMIRBuilder()->CreateConstval(symbolConst);
}

bool Simplify::IsSymbolReplaceableWithConst(const MIRSymbol &symbol) const
{
    return (symbol.GetStorageClass() == kScFstatic && !symbol.HasPotentialAssignment()) ||
           symbol.GetAttrs().GetAttr(ATTR_const);
}

bool Simplify::IsConstRepalceable(const MIRConst &mirConst) const
{
    switch (mirConst.GetKind()) {
        case kConstInt:
        case kConstFloatConst:
        case kConstDoubleConst:
        case kConstFloat128Const:
        case kConstLblConst:
            return true;
        default:
            return false;
    }
}

MIRConst *Simplify::GetElementConstFromFieldId(FieldID fieldId, MIRConst *mirConst)
{
    FieldID currFieldId = 1;
    MIRConst *resultConst = nullptr;
    auto originAggConst = static_cast<MIRAggConst *>(mirConst);
    auto originAggType = static_cast<MIRStructType &>(originAggConst->GetType());
    bool hasReached = false;
    std::function<void(MIRConst *)> traverseAgg = [&](MIRConst *currConst) {
        auto *currAggConst = safe_cast<MIRAggConst>(currConst);
        ASSERT_NOT_NULL(currAggConst);
        auto *currAggType = safe_cast<MIRStructType>(currAggConst->GetType());
        ASSERT_NOT_NULL(currAggType);
        for (size_t iter = 0; iter < currAggType->GetFieldsSize() && !hasReached; ++iter) {
            size_t constIdx = currAggType->GetKind() == kTypeUnion ? 1 : iter + 1;
            auto *fieldConst = currAggConst->GetAggConstElement(constIdx);
            auto *fieldType = originAggType.GetFieldType(currFieldId);

            if (currFieldId == fieldId) {
                if (auto *truncCst = TruncateUnionConstant(*currAggType, fieldConst, *fieldType)) {
                    resultConst = truncCst;
                } else {
                    resultConst = fieldConst;
                }

                hasReached = true;
                return;
            }

            ++currFieldId;
            if (fieldType->GetKind() == kTypeUnion || fieldType->GetKind() == kTypeStruct) {
                traverseAgg(fieldConst);
            }
        }
    };
    traverseAgg(mirConst);
    CHECK_FATAL(hasReached, "const not found");
    return resultConst;
}

void Simplify::Finish() {}

// Join `num` `byte`s into a number
// Example:
//   byte   num                output
//   0x0a    2                 0x0a0a
//   0x12    4             0x12121212
//   0xff    8     0xffffffffffffffff
static uint64 JoinBytes(int byte, uint32 num)
{
    CHECK_FATAL(num <= 8, "not support"); // just support num less or equal 8, see comment above
    uint64 realByte = static_cast<uint64>(byte % 256);
    if (realByte == 0) {
        return 0;
    }
    uint64 result = 0;
    for (uint32 i = 0; i < num; ++i) {
        result += (realByte << (i * k8BitSize));
    }
    return result;
}

static BaseNode *ConstructConstvalNode(uint64 val, PrimType primType, MIRBuilder &mirBuilder)
{
    PrimType constPrimType = primType;
    if (IsPrimitiveFloat(primType)) {
        constPrimType = GetIntegerPrimTypeBySizeAndSign(GetPrimTypeBitSize(primType), false);
    }
    MIRType *constType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(constPrimType));
    MIRConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(val, *constType);
    BaseNode *ret = mirBuilder.CreateConstval(mirConst);
    if (IsPrimitiveFloat(primType)) {
        MIRType *floatType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(primType));
        ret = mirBuilder.CreateExprRetype(*floatType, constPrimType, ret);
    }
    return ret;
}

static BaseNode *ConstructConstvalNode(int64 byte, uint64 num, PrimType primType, MIRBuilder &mirBuilder)
{
    auto val = JoinBytes(byte, static_cast<uint32>(num));
    return ConstructConstvalNode(val, primType, mirBuilder);
}

// Input total size of memory, split the memory into several blocks, the max block size is 8 bytes
// Example:
//   input        output
//     40     [ 8, 8, 8, 8, 8 ]
//     31     [ 8, 8, 8, 4, 2, 1 ]
static void SplitMemoryIntoBlocks(size_t totalMemorySize, std::vector<uint32> &blocks)
{
    size_t leftSize = totalMemorySize;
    size_t curBlockSize = kMaxMemoryBlockSizeToAssign;  // max block size in byte
    while (curBlockSize > 0) {
        size_t n = leftSize / curBlockSize;
        blocks.insert(blocks.end(), n, curBlockSize);
        leftSize -= (n * curBlockSize);
        curBlockSize = curBlockSize >> 1;
    }
}

static bool IsComplexExpr(const BaseNode *expr, MIRFunction &func)
{
    Opcode op = expr->GetOpCode();
    if (op == OP_regread) {
        return false;
    }
    if (op == OP_dread) {
        auto *symbol = func.GetLocalOrGlobalSymbol(static_cast<const DreadNode *>(expr)->GetStIdx());
        if (symbol->IsGlobal() || symbol->GetStorageClass() == kScPstatic) {
            return true;  // dread global/static var is complex expr because it will be lowered to adrp + add
        } else {
            return false;
        }
    }
    if (op == OP_addrof) {
        auto *symbol = func.GetLocalOrGlobalSymbol(static_cast<const AddrofNode *>(expr)->GetStIdx());
        if (symbol->IsGlobal() || symbol->GetStorageClass() == kScPstatic) {
            return true;  // addrof global/static var is complex expr because it will be lowered to adrp + add
        } else {
            return false;
        }
    }
    return true;
}

BaseNode *MemEntry::BuildAsRhsExpr(MIRFunction &func) const
{
    BaseNode *expr = nullptr;
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    if (addrExpr->GetOpCode() == OP_addrof) {
        // We prefer dread to iread
        // consider iaddrof if possible
        auto *addrof = static_cast<AddrofNode *>(addrExpr);
        auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
        expr = mirBuilder->CreateExprDread(*memType, addrof->GetFieldID(), *symbol);
    } else {
        MIRType *structPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
        expr = mirBuilder->CreateExprIread(*memType, *structPtrType, 0, addrExpr);
    }
    return expr;
}

static void InsertAndMayPrintStmt(BlockNode &block, const StmtNode &anchor, bool debug, StmtNode *stmt)
{
    if (stmt == nullptr) {
        return;
    }
    block.InsertBefore(&anchor, stmt);
    if (debug) {
        stmt->Dump(0);
    }
}

static BaseNode *TryToExtractComplexExpr(BaseNode *expr, MIRFunction &func, BlockNode &block, const StmtNode &anchor,
                                         bool debug)
{
    if (!IsComplexExpr(expr, func)) {
        return expr;
    }
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    auto pregIdx = func.GetPregTab()->CreatePreg(PTY_ptr);
    StmtNode *regassign = mirBuilder->CreateStmtRegassign(PTY_ptr, pregIdx, expr);
    InsertAndMayPrintStmt(block, anchor, debug, regassign);
    auto *extractedExpr = mirBuilder->CreateExprRegread(PTY_ptr, pregIdx);
    return extractedExpr;
}

void MemEntry::ExpandMemsetLowLevel(int64 byte, uint64 size, MIRFunction &func, StmtNode &stmt, BlockNode &block,
                                    MemOpKind memOpKind, bool debug, ErrorNumber errorNumber) const
{
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    std::vector<uint32> blocks;
    SplitMemoryIntoBlocks(size, blocks);
    int32 offset = 0;
    // If blocks.size() > 1 and `dst` is not a leaf node,
    // we should extract common expr to avoid redundant expression
    BaseNode *realDstExpr = addrExpr;
    if (blocks.size() > 1) {
        realDstExpr = TryToExtractComplexExpr(addrExpr, func, block, stmt, debug);
    }
    BaseNode *readConst = nullptr;
    // rhs const is big, extract it to avoid redundant expression
    bool shouldExtractRhs = blocks.size() > 1 && (byte & 0xff) != 0;
    for (auto curSize : blocks) {
        // low level memset expand result:
        //   iassignoff <prim-type> <offset> (dstAddrExpr, constval <prim-type> xx)
        PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
        BaseNode *rhsExpr = ConstructConstvalNode(byte, curSize, constType, *mirBuilder);
        if (shouldExtractRhs) {
            // we only need to extract u64 const once
            PregIdx pregIdx = func.GetPregTab()->CreatePreg(constType);
            auto *constAssign = mirBuilder->CreateStmtRegassign(constType, pregIdx, rhsExpr);
            InsertAndMayPrintStmt(block, stmt, debug, constAssign);
            readConst = mirBuilder->CreateExprRegread(constType, pregIdx);
            shouldExtractRhs = false;
        }
        if (readConst != nullptr && curSize == kMaxMemoryBlockSizeToAssign) {
            rhsExpr = readConst;
        }
        auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, offset, realDstExpr, rhsExpr);
        InsertAndMayPrintStmt(block, stmt, debug, iassignoff);
        if (debug) {
            ASSERT_NOT_NULL(iassignoff);
            iassignoff->Dump(0);
        }
        offset += static_cast<int32>(curSize);
    }
    // handle memset return val
    auto *retAssign = GenMemopRetAssign(stmt, func, true, memOpKind, errorNumber);
    InsertAndMayPrintStmt(block, stmt, debug, retAssign);
    // return ERRNO_INVAL if memset_s dest is NULL
    block.RemoveStmt(&stmt);
}

// handle memset, memcpy return val
StmtNode *MemEntry::GenMemopRetAssign(StmtNode &stmt, MIRFunction &func, bool isLowLevel, MemOpKind memOpKind,
                                      ErrorNumber errorNumber)
{
    if (stmt.GetOpCode() != OP_call && stmt.GetOpCode() != OP_callassigned) {
        return nullptr;
    }
    auto &callStmt = static_cast<CallNode &>(stmt);
    const auto &retVec = callStmt.GetReturnVec();
    if (retVec.empty()) {
        return nullptr;
    }
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    BaseNode *rhs = callStmt.Opnd(0);  // for memset, memcpy
    if (memOpKind == MEM_OP_memset_s || memOpKind == MEM_OP_memcpy_s) {
        // memset_s and memcpy_s must return an errorNumber
        MIRType *constType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_i32));
        MIRConst *mirConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(errorNumber, *constType);
        rhs = mirBuilder->CreateConstval(mirConst);
    }
    if (!retVec[0].second.IsReg()) {
        auto *retAssign = mirBuilder->CreateStmtDassign(retVec[0].first, 0, rhs);
        return retAssign;
    } else {
        PregIdx pregIdx = retVec[0].second.GetPregIdx();
        auto pregType = func.GetPregTab()->GetPregTableItem(static_cast<uint32>(pregIdx))->GetPrimType();
        auto *retAssign = mirBuilder->CreateStmtRegassign(pregType, pregIdx, rhs);
        if (isLowLevel) {
            retAssign->GetRHS()->SetPrimType(pregType);
        }
        return retAssign;
    }
}

MemOpKind SimplifyMemOp::ComputeMemOpKind(StmtNode &stmt)
{
    if (stmt.GetOpCode() == OP_intrinsiccall) {
        auto intrinsicID = static_cast<IntrinsiccallNode &>(stmt).GetIntrinsic();
        if (intrinsicID == INTRN_C_memset) {
            return MEM_OP_memset;
        } else if (intrinsicID == INTRN_C_memcpy) {
            return MEM_OP_memcpy;
        }
    }
    // lowered memop function (such as memset) may be a call, not callassigned
    if (stmt.GetOpCode() != OP_callassigned && stmt.GetOpCode() != OP_call) {
        return MEM_OP_unknown;
    }
    auto &callStmt = static_cast<CallNode &>(stmt);
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
    const char *funcName = func->GetName().c_str();
    if (strcmp(funcName, kFuncNameOfMemset) == 0) {
        return MEM_OP_memset;
    }
    if (strcmp(funcName, kFuncNameOfMemcpy) == 0) {
        return MEM_OP_memcpy;
    }
    if (strcmp(funcName, kFuncNameOfMemsetS) == 0) {
        return MEM_OP_memset_s;
    }
    if (strcmp(funcName, kFuncNameOfMemcpyS) == 0) {
        return MEM_OP_memcpy_s;
    }
    return MEM_OP_unknown;
}

bool SimplifyMemOp::AutoSimplify(StmtNode &stmt, BlockNode &block, bool isLowLevel)
{
    return false;
}
}  // namespace maple
